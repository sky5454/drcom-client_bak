#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#include <drcom.h>
#include "config.h"
#include "drcom_types.h"
#include "client_daemon.h"

#include "daemon.h"
#include "log.h"

#define READ_END	0
#define WRITE_END	1

extern int server_sock_init(struct drcom_handle*);
extern void server_sock_destroy(struct drcom_handle*);

void msgprint(const char *);
void report_result(int, int, int);
void * daemon_watchport(void *);
void * daemon_keepalive(void *);
void module_start_auth(struct drcom_handle *);
void module_stop_auth(void);

/*
	 0: idle
	 1: logged in
	 2: busy
*/
static int status = 0;
static int sigusr1_pipe[2] = {-1,-1};
static pthread_t th_watchport = 0, th_keepalive = 0;

static void unblock_sigusr1(void)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

static void block_sigusr1(void)
{
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigaddset(&set, SIGTERM);
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}

static void sigusr1_handler (int sig)
{
	write(sigusr1_pipe[WRITE_END], &sig, sizeof(int));
}

static int setup_sig_handlers(void)
{
	struct sigaction sa;
	int retval;
	
	retval = pipe(sigusr1_pipe);
	if (retval < 0) {
		logerr("error getting pipes: %s", strerror(errno));
		goto exit;
	}

	retval = fcntl(sigusr1_pipe[READ_END], F_GETFL, 0);
	if (retval < 0) {
		logerr("error fcntl on read pipe: %s", strerror(errno));
		goto exit;
	}
	retval = fcntl(sigusr1_pipe[READ_END], F_SETFL, retval | O_NONBLOCK);
	if (retval < 0) {
		logerr("error fcntl on read pipe: %s", strerror(errno));
		goto exit;
	}

	retval = fcntl(sigusr1_pipe[WRITE_END], F_GETFL, 0);
	if (retval < 0) {
		logerr("error fcntl on write pipe: %s", strerror(errno));
		goto exit;
	}
	retval = fcntl(sigusr1_pipe[WRITE_END], F_SETFL, retval | O_NONBLOCK);
	if (retval < 0) {
		logerr("error fcntl on write pipe: %s", strerror(errno));
		goto exit;
	}

	memset(&sa, 0x00, sizeof(sa));
	sa.sa_handler = sigusr1_handler;
	sa.sa_flags = SA_RESTART;
	sigemptyset (&sa.sa_mask);
	sigaction (SIGUSR1, &sa, NULL);
	sigaction (SIGTERM, &sa, NULL);

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);

	return 0;

exit:
	return -1;
}

static void do_force_logout(struct drcom_handle *h, int sig)
{
	int r;

	if (status == 1) {
		loginfo("SIGUSR1/SIGTERM caught, force logout.\n");

		status = 2;
		/* Stop the threads here, since they might interfere with
		the logout process */
		module_stop_auth();
		pthread_cancel(th_keepalive);
		pthread_cancel(th_watchport);
		pthread_join(th_keepalive, NULL);
		pthread_join(th_watchport, NULL);
		/* Now try to log out */
		r = drcom_logout(h, 0);
		if(r){
			/* If logout failed, that means we are still logged in,
			so re-create the threads and continue authentication */
			module_start_auth(h);
			pthread_create(&th_watchport, NULL, daemon_watchport, h);
			pthread_create(&th_keepalive, NULL, daemon_keepalive, h);
		}	
		status = (!r) ? 0 : 1;

		if(status == 0)
			server_sock_destroy(h);
	}
	if(sig == SIGTERM){
		loginfo("received SIGTERM, let's exit... ");
		exit(1);
	}
}

void drcomcd_daemon(int s)
{
	int s2, r;
	struct drcom_handle *h;
	unsigned char buf1[DRCOMCD_BUF_LEN];
	struct drcomcd_hdr *cd_hdr = (struct drcomcd_hdr *) buf1;
	struct drcomcd_login *cd_login = (struct drcomcd_login *) (buf1 + sizeof(*cd_hdr));
	struct drcomcd_logout *cd_logout = (struct drcomcd_logout *) (buf1 + sizeof(*cd_hdr));
	struct drcomcd_passwd *cd_passwd = (struct drcomcd_passwd *) (buf1 + sizeof(*cd_hdr));

	if(setup_sig_handlers()<0){
		logerr("sig handlers not setup, exit.\n");
		exit(1);
	}

	/* Initialize the handle for the lifetime of the daemon */
	h = drcom_create_handle();
	drcom_init(h, &msgprint);

	loginfo("drcomd started.\n");

	while (1) {
		int maxfd;
		fd_set readfds;

		FD_ZERO(&readfds);
		FD_SET(s, &readfds);
		FD_SET(sigusr1_pipe[READ_END], &readfds);
		
		maxfd = s;
		if(maxfd < sigusr1_pipe[READ_END])
			maxfd = sigusr1_pipe[READ_END];

		unblock_sigusr1();
		r = select(maxfd+1, &readfds, NULL,NULL, NULL);
		if(r<0){
			if(errno != EINTR)
				logerr("signal caught\n");
			continue;
		}
		if(FD_ISSET(sigusr1_pipe[READ_END], &readfds)){
			char buf[256];
			int *sig = (int*)buf;

			read(sigusr1_pipe[READ_END], &buf, sizeof(buf));
			do_force_logout(h, *sig);
		}
		if(!FD_ISSET(s, &readfds))
			continue;

		block_sigusr1();
		s2 = accept(s, NULL, NULL);
		if (s2 == -1 && errno != EINTR) {
			logerr("daemon: accept failed: %s", strerror(errno));
			continue;
		}

		{
		fd_set	rfds;
		struct timeval t;

		FD_ZERO(&rfds);
		FD_SET(s2, &rfds);
		t.tv_sec = 2;
		t.tv_usec = 0;
		r = select(s2+1, &rfds, NULL, NULL, &t);
		if(r<=0){
			logerr("accepted, but no data\n");
			close(s2);
			continue;
		}

		if(FD_ISSET(s2, &rfds)){
			r = recv(s2, buf1, DRCOMCD_BUF_LEN, 0);
			if (r < 0)
				logerr("daemon: recv: %s", strerror(errno));
			if (r < 1 || cd_hdr->signature != DRCOM_SIGNATURE) {
				logerr("Unknown signature\n");
				close(s2);
				continue;
			}
		}else{
			close(s2);
			continue;
		}
		}

		switch (cd_hdr->type) {
		case DRCOMCD_LOGIN:
			if (status == 0) {
				status = 2;
				r = server_sock_init(h);
				if(r!=0){
					status = 0;
					break;
				}
				r = drcom_login(h, cd_login->timeout);
				status = (!r) ? 1 : 0;
			} else {
				report_result(s2, DRCOMCD_FAILURE, DRCOMCD_REASON_BUSY);
				continue;
			}

			/* Result-dependent task */
			if (status == 1) {
				module_start_auth(h);
				pthread_create(&th_watchport,NULL,daemon_watchport, h);
				pthread_create(&th_keepalive,NULL,daemon_keepalive, h);
			}else
				server_sock_destroy(h);

			break;

		case DRCOMCD_LOGOUT:
			if (status == 1) {
				status = 2;
				/* Stop the threads here, since they might interfere with
				 the logout process */
				module_stop_auth();
				pthread_cancel(th_keepalive);
				pthread_cancel(th_watchport);
				pthread_join(th_keepalive, NULL);
				pthread_join(th_watchport, NULL);
				/* Now try to log out */
				r = drcom_logout(h, cd_logout->timeout);
				if(r){
				/* If logout failed, that means we are still logged in,
				 so re-create the threads and continue authentication */
					module_start_auth(h);
					pthread_create(&th_watchport, 
						NULL, daemon_watchport, h);
					pthread_create(&th_keepalive, 
						NULL, daemon_keepalive, h);
				}
				status = (!r) ? 0 : 1;
				if(status == 0)
					server_sock_destroy(h);
			} else {
				report_result(s2, DRCOMCD_FAILURE, DRCOMCD_REASON_BUSY);
				continue;
			}

			break;

		case DRCOMCD_PASSWD:
			if (status == 0) {
				status = 2;
				r = drcom_passwd(h, 
					cd_passwd->newpasswd, cd_passwd->timeout);
				status = 0;
			} else {
				report_result(s2, DRCOMCD_FAILURE, DRCOMCD_REASON_BUSY);
				continue;
			}

			break;

		default: continue; break;
		}

		if (r)
			report_result(s2, DRCOMCD_FAILURE, DRCOMCD_REASON_NO_REASON);
		else
			report_result(s2, DRCOMCD_SUCCESS, DRCOMCD_REASON_NO_REASON);
	}

	return;
}
/*
#include <iconv.h>
#include <wchar.h>
*/
void msgprint(const char *s)
{
/*
	char buf[1024];
	size_t buflen;
	size_t sbuflen;
	iconv_t it;
	it = iconv_open("UTF-8", "GB18030");
	sbuflen = strlen(s);
	buflen=1024;
	if(iconv(it, &buf, &buflen, &s, &sbuflen)<0)
		strncpy(buf, s, 1024);
	iconv_close(it);
	fwprintf(stderr, "%s\n", buf);
*/
	loginfo("%s\n", s);
	return;
}

void report_result(int s2, int result, int reason)
{
	unsigned char buf2[DRCOMCD_BUF_LEN];
	struct drcomcd_hdr *dc_hdr = (struct drcomcd_hdr *) buf2;
	struct drcomcd_result *dc_result = (struct drcomcd_result *) (buf2 + sizeof(*dc_hdr));
	int r;

	dc_hdr->signature = DRCOM_SIGNATURE;
	dc_hdr->type = result;
	dc_result->reason = reason;

	r = send(s2, buf2, sizeof(*dc_hdr) + sizeof(*dc_result), 0);
	if (r < 0)
		logerr("daemon: send: %s", strerror(errno));

	close(s2);

	return;
}

void * daemon_watchport(void *arg)
{
	block_sigusr1();
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	drcom_watchport((struct drcom_handle *) arg);
	loginfo("watchport returns\n");
	return NULL;
}

void * daemon_keepalive(void *arg)
{
	block_sigusr1();
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	sleep(9);
	drcom_keepalive((struct drcom_handle *) arg);
	loginfo("keepalive returns\n");
	return NULL;
}

void module_start_auth(struct drcom_handle *h)
{
	struct drcom_status_data status_data;
	struct drcom_auth_data auth_data;
	struct drcom_iface_data iface_data;
	struct drcom_except_data except_data[MAX_EXCEPT_ITEMS];
	struct drcom_session_info *s;
	int authlen, r;
	FILE *f;

	s = drcom_get_session_info(h);
	authlen = drcom_get_authlen();

	/* struct drcom_auth is already packed, so no worries here */
	memcpy(auth_data.auth, s->auth, authlen);

	/* memset(drcom_iface_data, 0, 4 * sizeof(struct addrport)); */

	iface_data.hostip = s->hostip;
	iface_data.hostport = s->hostport;
	iface_data.servip = s->servip;
	iface_data.servport = s->servport;
	iface_data.dnsp = s->dnsp;
	iface_data.dnss = s->dnss;

	/* memset(drcom_except_data, 0, 20 * sizeof(struct addrmask)); */

	except_data[0].addr = 0;
	except_data[0].mask = 0;

	/* Actually write to the files */

	f = fopen(DRCOM_MODULE_AUTH, "w");
	if (!f) {
		logerr("Writing authentication data: %s", strerror(errno));
		return;
	}
	r = fwrite(&auth_data, AUTH_LEN, 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_AUTH ": %s", strerror(errno));
		return;
	}
	fclose(f);

	f = fopen(DRCOM_MODULE_IFACE, "w");
	if (!f) {
		logerr("Writing interface info: %s", strerror(errno));
		return;
	}
	r = fwrite(&iface_data, IFACE_LEN, 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_IFACE ": %s", strerror(errno));
		return;
	}
	fclose(f);

	f = fopen(DRCOM_MODULE_EXCEPT, "w");
	if (!f) {
		logerr("Writing exceptions: %s", strerror(errno));
		return;
	}
	r = fwrite(&except_data, EXCEPT_LEN, MAX_EXCEPT_ITEMS, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_EXCEPT ": %s", strerror(errno));
		return;
	}
	fclose(f);

#define DRCOM_MODULE_PID "/proc/drcom/pid"
	{
		pid_t pid = getpid();

		f = fopen(DRCOM_MODULE_PID, "w");
		if (!f) {
			logerr("Writing pid info " DRCOM_MODULE_PID ": %s", strerror(errno));
			return;
		}
		r = fwrite(&pid, sizeof(pid_t), 1, f);
		if (r == 0 && ferror(f)) {
			logerr("Writing to " DRCOM_MODULE_PID ": %s", strerror(errno));
			return;
		}
		fclose(f);
	}

#define DRCOM_MODULE_AUTOLOGOUT "/proc/drcom/autologout"
	{
	struct drcom_conf *conf = (struct drcom_conf*)h->conf;
	char al = conf->autologout?'1':'0';

	f = fopen(DRCOM_MODULE_AUTOLOGOUT, "w");
	if (!f) {
		logerr("Writing " DRCOM_MODULE_AUTOLOGOUT ": %s", strerror(errno));
		return;
	}
	r = fwrite(&al, sizeof(char), 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing " DRCOM_MODULE_AUTOLOGOUT ": %s", strerror(errno));
		return;
	}
	fclose(f);
	}

	/* Now tell drcom.o to start authenticating */

	status_data.status = STATUS_LOGIN;

	f = fopen(DRCOM_MODULE_STATUS, "w");
	if (!f) {
		logerr("Starting authentication: %s", strerror(errno));
		return;
	}
	r = fwrite(&status_data, STATUS_LEN, 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_STATUS ": %s", strerror(errno));
		return;
	}
	fclose(f);

	return;
}

void module_stop_auth()
{
	struct drcom_status_data status_data;
	FILE *f;
	int r;

	status_data.status = STATUS_NOTLOGIN;

	f = fopen(DRCOM_MODULE_STATUS, "w");
	if (!f) {
		logerr("Stopping authentication: %s", strerror(errno));
		return;
	}

	r = fwrite(&status_data, STATUS_LEN, 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_STATUS ": %s", strerror(errno));
		return;
	}

	loginfo("daemon: Stopped authentication\n");

	fclose(f);

	return;
}

