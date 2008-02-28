#include "drcom_linux.h"

/* for procfs functions and variables */
#include <linux/proc_fs.h>
/* for copy_to_user & copy_from_user */
#include <asm/uaccess.h>

#include "../include/daemon_kernel.h"

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? a : b)
#endif

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? a : b)
#endif

extern void drcom_update_keepalive_timer(void);
extern void drcom_del_keepalive_timer(void);

struct proc_dir_entry *proc_drcom,
			*proc_drcom_status,
			*proc_drcom_auth,
			*proc_drcom_iface,
			*proc_drcom_except,
			*proc_drcom_pid,
			*proc_drcom_autologout;

struct drcom_status_data _status_data[1], *status_data = _status_data;
struct drcom_auth_data _auth_data[1], *auth_data = _auth_data;
struct drcom_iface_data _iface_data[1], *iface_data = _iface_data;
struct drcom_except_data _except_data[MAX_EXCEPT_ITEMS + 1], *except_data = _except_data;

#define MAX_PID_LEN 	sizeof(pid_t)
pid_t pid_value, *pid_data = &pid_value;

#define MAX_AUTOLOGOUT_LEN	1
char _autologout, *autologout_data = &_autologout;

static int read_all(char *page, int count, int *eof, void *data, int maxlen)
{
	int len = MIN(count, maxlen);

	int i;
	for (i = 0; i < len; ++i)
		page[i] = *((char *) data + i);

	*eof = 1;

	return len;
}

static int write_all(const char *buffer, unsigned long count, void *data, int maxlen)
{
	int len = MIN(count, (unsigned long) maxlen);

	int i;
	for (i = 0; i < len; ++i)
		*((char *) data + i) = buffer[i];

	return len;
}

static int status_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, sizeof(struct drcom_status_data));
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int status_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;

	len = write_all(buffer, count, data, sizeof(struct drcom_status_data));
	if(status_data->status == '1')
		drcom_update_keepalive_timer();
	else if(status_data->status =='0')
		drcom_del_keepalive_timer();
	return len;
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

static int auth_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, sizeof(struct drcom_auth_data));
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int auth_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	return write_all(buffer, count, data, sizeof(struct drcom_auth_data));
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

static int iface_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, sizeof(struct drcom_iface_data));
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int iface_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	return write_all(buffer, count, data, sizeof(struct drcom_iface_data));
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

static int except_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, MAX_EXCEPT_LEN);
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int except_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	return write_all(buffer, count, data, MAX_EXCEPT_LEN);
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

static int pid_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, sizeof(pid_t));
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int pid_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;
	len = write_all(buffer, count, data, sizeof(pid_t));
	return len;
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

static int autologout_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	return read_all(page, count, eof, data, sizeof(char));
	/* Turn off the warnings and still get the same object file */
	off = 0; start = 0;
}

static int autologout_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;
	len = write_all(buffer, count, data, sizeof(char));
	return len;
	/* Turn off the warnings and still get the same object file */
	file = 0;
}

int drcom_init_proc(void)
{
	proc_drcom = proc_mkdir("drcom", &proc_root);

	proc_drcom_status = create_proc_entry("status", 0644, proc_drcom);
	proc_drcom_auth = create_proc_entry("auth", 0644, proc_drcom);
	proc_drcom_iface = create_proc_entry("iface", 0644, proc_drcom);
	proc_drcom_except = create_proc_entry("except", 0644, proc_drcom);
	proc_drcom_pid = create_proc_entry("pid", 0644, proc_drcom);
	proc_drcom_autologout = create_proc_entry("autologout", 0644, proc_drcom);

	proc_drcom_status->read_proc = &status_read;
	proc_drcom_status->write_proc = &status_write;
	proc_drcom_status->data = status_data;

	proc_drcom_auth->read_proc = &auth_read;
	proc_drcom_auth->write_proc = &auth_write;
	proc_drcom_auth->data = auth_data;

	proc_drcom_iface->read_proc = &iface_read;
	proc_drcom_iface->write_proc = &iface_write;
	proc_drcom_iface->data = iface_data;

	proc_drcom_except->read_proc = &except_read;
	proc_drcom_except->write_proc = &except_write;
	proc_drcom_except->data = except_data;

	proc_drcom_pid->read_proc = &pid_read;
	proc_drcom_pid->write_proc = &pid_write;
	proc_drcom_pid->data = pid_data;

	proc_drcom_autologout->read_proc = &autologout_read;
	proc_drcom_autologout->write_proc = &autologout_write;
	proc_drcom_autologout->data = autologout_data;

	/* no need to initialize others */

	status_data->status = STATUS_NOTLOGIN;
	pid_value = 0;

	except_data[MAX_EXCEPT_ITEMS].addr = 0;
	except_data[MAX_EXCEPT_ITEMS].mask = 0;

	return 0;
}

int drcom_cleanup_proc(void)
{
	remove_proc_entry("autologout", proc_drcom);
	remove_proc_entry("pid", proc_drcom);
	remove_proc_entry("except", proc_drcom);
	remove_proc_entry("iface", proc_drcom);
	remove_proc_entry("auth", proc_drcom);
	remove_proc_entry("status", proc_drcom);
	remove_proc_entry("drcom", &proc_root);

	return 0;
}
/*
int get_proc_bufs(struct drcom_status_data **proto_status,
		struct drcom_auth_data **proto_auth,
		struct drcom_iface_data **proto_iface,
		struct drcom_except_data **proto_except)
{
	*proto_status = status_data;
	*proto_auth = auth_data;
	*proto_iface = iface_data;
	*proto_except = except_data;

	return 0;
}
*/
