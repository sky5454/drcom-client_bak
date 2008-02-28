#include "drcom_linux.h"

#include <linux/socket.h>
#include <net/sock.h>
#include <linux/net.h>
#include <net/inet_common.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/timer.h>

#ifdef DRCOM_USE_JPROBE
#include <linux/kallsyms.h>
#include <linux/kprobes.h>
#endif

#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "../include/daemon_kernel.h"

extern struct drcom_status_data *status_data;
extern struct drcom_auth_data *auth_data;
extern struct drcom_iface_data *iface_data;
extern struct drcom_except_data *except_data;
extern pid_t *pid_data;
extern char *autologout_data;

/* Use only for constants */

#define n_to_ip(a,b,c,d) \
	__constant_htonl( (((u_int32_t) a) << 24) \
	+ (((u_int32_t) b) << 16) \
	+ (((u_int32_t) c) << 8) \
	+ ((u_int32_t) d) )

typedef int (*ACCEPT_PROC) (struct socket *sock, struct socket *newsock, int flags); 
typedef int (*CONNECT_PROC)(struct socket *, struct sockaddr *, int, int);
typedef int (*SENDMSG_PROC)(struct kiocb *iocb, struct socket *, struct msghdr *, size_t);
typedef int (*RECVMSG_PROC)(struct kiocb *iocb, struct socket *sock,
				struct msghdr *msg, size_t size, int flags);

static ACCEPT_PROC system_stream_accept;
static CONNECT_PROC system_stream_connect;
static SENDMSG_PROC system_stream_sendmsg;
static SENDMSG_PROC system_dgram_sendmsg;
static RECVMSG_PROC system_common_recvmsg;

static struct proto_ops sys0_inet_stream_ops;
static struct proto_ops sys1_inet_stream_ops;
static struct proto_ops sys_inet_dgram_ops;

static int need_auth(u_int32_t saddr, u_int32_t daddr)
{
	int i;

	/* Check source */
	if (saddr != 0 && saddr != iface_data->hostip)
		goto normal;

	/* DNS servers */
	if ((daddr == iface_data->dnsp) || (daddr == iface_data->dnss))
		goto normal;

	/* Internal network */
	if (
#include "except/private.c"
		 ||
#include "except/common.c"
		 )
		goto normal;

	i = 0;
	while ((except_data[i].addr != 0)
				 && (except_data[i].mask != 0))
		if ((daddr & except_data[i].mask)
				== (except_data[i].addr & except_data[i].mask))
			goto normal;
		else
			++i;

	return 1;

normal:
	return 0;
}

#define DRCOM_KEEPALIVE_TIMEOUT	(2*60*HZ)

static struct timer_list drcom_keepalive_timer;
extern char *autologout_data;

static void keepalive_timer_func(unsigned long ul)
{
	if(*pid_data)
		kill_proc(*pid_data, SIGUSR1, 1);
}

void drcom_update_keepalive_timer(void)
{
	if(*autologout_data == '1')
		mod_timer(&drcom_keepalive_timer, jiffies+DRCOM_KEEPALIVE_TIMEOUT);
}

void drcom_del_keepalive_timer(void)
{
	del_timer_sync(&drcom_keepalive_timer);
}

static int drcom_common_recvmsg(struct kiocb *iocb, struct socket *sock,
				struct msghdr *msg, size_t size, int flags)
{
	drcom_update_keepalive_timer();

	return system_common_recvmsg(iocb, sock, msg, size, flags);
}

static int drcom_stream_sendmsg(struct kiocb *iocb, struct socket *sock, 
				struct msghdr *msg, size_t size)
{
	drcom_update_keepalive_timer();

	return system_stream_sendmsg(iocb, sock, msg, size);
}

static DECLARE_MUTEX(auth_mutex);

static int drcom_stream_sendmsg_auth(struct kiocb *iocb, struct socket *sock, 
				struct msghdr *msg, size_t size)
{
	struct iovec iov, *old_iovec;
	int ret, n_iovec;
	mm_segment_t old_fs;

	/* Yes, this is really bottleneck, 
	 * but do we have other way? 
	 */
	if(down_interruptible(&auth_mutex))
		return -ERESTARTSYS;

	if(sock->ops != &sys0_inet_stream_ops){ /* we are the loser */
		up(&auth_mutex);
		return drcom_stream_sendmsg(iocb, sock, msg, size);
	}

	/* This is much simpler than udp */
	iov.iov_base = auth_data->auth;
	iov.iov_len = 16;

	/* Swap the iovecs (note: remember to swap msg_iovlen, too */
	n_iovec = msg->msg_iovlen;
	old_iovec = msg->msg_iov;

	/* Prepare for sending the auth packet */
	msg->msg_iovlen = 1;
	msg->msg_iov = &iov;

	/* Bypass address checking in copy_from_user() */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = system_stream_sendmsg(iocb, sock, msg, 16);
	set_fs(old_fs);

	/* Prepare for sending actual data */
	msg->msg_iov = old_iovec;
	msg->msg_iovlen = n_iovec;

	if (ret == 16){
		sock->ops = &sys1_inet_stream_ops;
		drcom_update_keepalive_timer();
		up(&auth_mutex);
		return system_stream_sendmsg(iocb, sock, msg, size);
	}else{
		up(&auth_mutex);
		return ret;
	}
}

static int drcom_stream_connect(struct socket *sock, struct sockaddr *uaddr, 
				int addr_len, int flags)
{
	if (!need_auth(0, ((struct sockaddr_in *) uaddr)->sin_addr.s_addr)){
		if(down_interruptible(&auth_mutex))
			return -ERESTARTSYS;

		if(try_module_get(inet_stream_ops.owner)){
			module_put(sock->ops->owner);
			sock->ops = &inet_stream_ops;
		}

		up(&auth_mutex);

	}else{
		drcom_update_keepalive_timer();
	}

	return system_stream_connect(sock, uaddr, addr_len, flags);
}

static int drcom_stream_accept(struct socket * sock, struct socket * newsock, int flag)
{
	int ret;
	struct sockaddr_in uaddr;
	int adrsize = sizeof(uaddr);
	int gtr;

	ret = system_stream_accept(sock, newsock, flag);
	if(ret)
		return ret;

	gtr = inet_stream_ops.getname(newsock, (struct sockaddr*)&uaddr, &adrsize, 2);
	if(gtr != 0)
		return ret;

	if(!need_auth(0, uaddr.sin_addr.s_addr)){
		if(try_module_get(inet_stream_ops.owner)){
			module_put(newsock->ops->owner);
			newsock->ops = &inet_stream_ops;
		}
	}else
		drcom_update_keepalive_timer();

	return ret;
}

static int drcom_udp_get_addrs(u_int32_t *saddr, u_int32_t *daddr, 
				struct sock *sk, struct msghdr *msg)
{
	struct sockaddr_in *usin;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,9)
	struct inet_opt *inet = inet_sk(sk);
#else
	struct inet_sock *inet = inet_sk(sk);
#endif

	/* Get the address */
	/* Stolen from linux-2.4.30/net/ipv4/udp.c */
	/* ... but modified, of course */

	if (msg->msg_name)
	{
		usin = msg->msg_name;
		if (msg->msg_namelen != sizeof(struct sockaddr_in)
			|| (usin->sin_family != AF_INET && usin->sin_family != AF_UNSPEC)
			|| usin->sin_port == 0)
			return -EINVAL;

		*daddr = usin->sin_addr.s_addr;
	}
	else
	{
		if (sk->sk_state != TCP_ESTABLISHED)
			return -EDESTADDRREQ;

		*daddr = inet->daddr;
	}

	*saddr = inet->saddr;

	return 0;
}

static int drcom_dgram_sendmsg(struct kiocb *iocb, struct socket *sock, 
				struct msghdr *msg, size_t size)
{
	u_int32_t saddr, daddr;
	struct iovec auth_iovec1[2], *auth_iovec=NULL, *old_iovec;
	mm_segment_t old_fs;
	int ret, r;
	unsigned int i;

	if (status_data->status != '1')
		goto normal;

	drcom_update_keepalive_timer();

	/* FIXME: Something's wrong here... */
	r = drcom_udp_get_addrs(&saddr, &daddr, sock->sk, msg);
	if (r)
		return r;

	if (!need_auth(saddr, daddr))
		goto normal;

	/* Does any packet have zero iovecs? */
	if (msg->msg_iovlen == 0)
		goto normal;

	/* Save the original iovec pointer */
	old_iovec = msg->msg_iov;

	/* Optimize: usually, msg_iovlen is 1 */
	if (msg->msg_iovlen == 1)
	{
		auth_iovec1[0].iov_base = auth_data->auth;
		auth_iovec1[0].iov_len = 16;
		auth_iovec1[1].iov_base = msg->msg_iov[0].iov_base;
		auth_iovec1[1].iov_len = msg->msg_iov[0].iov_len;
		msg->msg_iov = auth_iovec1;
	}
	else
	{
#ifdef DEBUG
	printk("msg_iovlen: %d\n", msg->msg_iovlen);
#endif
		auth_iovec = kmalloc((msg->msg_iovlen+1) * sizeof(struct iovec), GFP_USER);

		auth_iovec[0].iov_base = auth_data->auth;
		auth_iovec[0].iov_len = 16;

		for (i = 0; i < msg->msg_iovlen; ++i)
		{
			auth_iovec[i+1].iov_base = msg->msg_iov[i].iov_base;
			auth_iovec[i+1].iov_len = msg->msg_iov[i].iov_len;
		}

		msg->msg_iov = auth_iovec;
	}

	/* Now that we have appended the authentication data, increment the
		 byte counts so that the packet will be sent correctly */
	++(msg->msg_iovlen);
	size += 16;

	/* This was the missing piece -- by using set_fs(KERNEL_DS),
		 we bypass the check in copy_from_user(), which is called later on
		 down the stack */
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	ret = system_dgram_sendmsg(iocb, sock, msg, size);
	set_fs(old_fs);

	if(msg->msg_iov == auth_iovec)
		kfree(auth_iovec);

	/* Restore the original iovec array and the byte counts */
	msg->msg_iov = old_iovec;
	--(msg->msg_iovlen);
	if (ret >= 16)
		ret -= 16;

	return ret;

normal:
	return system_dgram_sendmsg(iocb, sock, msg, size);
}

#ifdef DRCOM_USE_JPROBE
static void jsock_init_data(struct socket *sock, struct sock *sk)
{
	if (status_data->status != '1')
		goto out;

	if(sock && sock->ops == &inet_stream_ops){
		sock->ops = &sys0_inet_stream_ops;
	}
	if(sock && sock->ops == &inet_dgram_ops){
		sock->ops = &sys_inet_dgram_ops;
	}
out:
	jprobe_return();
}

struct jfunc_obj{
	const char *funcname;
	struct jprobe jp;
};

#define JFUNC(func) \
	{ .funcname = #func, .jp = { .entry = (kprobe_opcode_t*)j##func }}

static struct jfunc_obj jfunc_objs[] = {
	JFUNC(sock_init_data),
};

#define MAX_JFUNC (sizeof(jfunc_objs)/sizeof(jfunc_objs[0]))

static int init_jfunc(void)
{
	int ret;
	int i;
	struct jfunc_obj *e;

	for (i = 0, e = jfunc_objs; i < MAX_JFUNC; i++, e++) {
		e = &jfunc_objs[i];
		e->jp.kp.addr = (kprobe_opcode_t *)kallsyms_lookup_name(e->funcname);

		if (e->jp.kp.addr) {
			ret = register_jprobe(&e->jp);
			if(ret != 0){
				printk(KERN_DEBUG "failed to register jprobe\n");
				goto out;
			}
		} else {
			printk(KERN_DEBUG "couldn't find %s to plant jprobe\n",
			 e->funcname);
		}
	}

	return 0;

out:
	for(; i > 0; i--){
		if (jfunc_objs[i].jp.kp.addr)
			unregister_jprobe(&jfunc_objs[i].jp);
	}
	return -1;
}

static void cleanup_jfunc(void)
{
	int i;
	for (i = 0; i < MAX_JFUNC; i++) {
		if (jfunc_objs[i].jp.kp.addr)
			unregister_jprobe(&jfunc_objs[i].jp);
	}
}
#else

#include <linux/security.h>

static void (*sys_socket_post_create)(struct socket *, int, int, int, int);

static void drcom_socket_post_create(struct socket *sock, int family,
				int type, int protocol, int kern)
{
	if(status_data->status != '1')
		goto out;

	if(sock && sock->ops == &inet_stream_ops){
		if(try_module_get(sys0_inet_stream_ops.owner)){
			module_put(sock->ops->owner);
			sock->ops = &sys0_inet_stream_ops;
		}
	}
	if(sock && sock->ops == &inet_dgram_ops){
		if(try_module_get(sys_inet_dgram_ops.owner)){
			module_put(sock->ops->owner);
			sock->ops = &sys_inet_dgram_ops;
		}
	}
out:	
	sys_socket_post_create(sock, family, type, protocol, kern);
}

static int init_hijack(void)
{
	if(security_ops == NULL)
		return -1;

	sys_socket_post_create = security_ops->socket_post_create;
	security_ops->socket_post_create = drcom_socket_post_create;

	return 0;
}

static void cleanup_hijack(void)
{
	if(security_ops && security_ops->socket_post_create == drcom_socket_post_create){
		security_ops->socket_post_create = sys_socket_post_create;
	}
}

#endif

int drcom_init_proto(void)
{
	system_stream_accept = inet_stream_ops.accept;
	system_stream_connect = inet_stream_ops.connect;
	system_stream_sendmsg = inet_stream_ops.sendmsg;
	system_dgram_sendmsg = inet_dgram_ops.sendmsg;

	system_common_recvmsg = inet_stream_ops.recvmsg;

	memcpy(&sys0_inet_stream_ops, &inet_stream_ops, sizeof(struct proto_ops));
	memcpy(&sys1_inet_stream_ops, &inet_stream_ops, sizeof(struct proto_ops));

	sys0_inet_stream_ops.accept = drcom_stream_accept;
	sys0_inet_stream_ops.connect = drcom_stream_connect;
	sys0_inet_stream_ops.sendmsg = drcom_stream_sendmsg_auth;
	sys0_inet_stream_ops.recvmsg = drcom_common_recvmsg;
	sys0_inet_stream_ops.owner = THIS_MODULE;

	sys1_inet_stream_ops.accept = drcom_stream_accept;
	sys1_inet_stream_ops.connect = drcom_stream_connect;
	sys1_inet_stream_ops.sendmsg = drcom_stream_sendmsg;
	sys1_inet_stream_ops.recvmsg = drcom_common_recvmsg;
	sys1_inet_stream_ops.owner = THIS_MODULE;

	memcpy(&sys_inet_dgram_ops, &inet_dgram_ops, sizeof(struct proto_ops));
	sys_inet_dgram_ops.sendmsg = drcom_dgram_sendmsg;
	sys_inet_dgram_ops.recvmsg = drcom_common_recvmsg;
	sys_inet_dgram_ops.owner = THIS_MODULE;

	init_timer(&drcom_keepalive_timer);
	drcom_keepalive_timer.data=0;
	drcom_keepalive_timer.function=keepalive_timer_func;

#ifdef DRCOM_USE_JPROBE
	return init_jfunc();
#else
	return init_hijack();
#endif
}

int drcom_cleanup_proto(void)
{
	/* it is possible to have pending keepalive timer,
	 * even when ref of THIS_MODULE reduced to 0
	 */
	drcom_del_keepalive_timer();

#ifdef DRCOM_USE_JPROBE
	cleanup_jfunc();
#else
	cleanup_hijack();
#endif
	return 0;
}

