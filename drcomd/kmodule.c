#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "drcomd.h"
#include "daemon_kernel.h"

#include "log.h"

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
	authlen = sizeof(struct drcom_auth_data);

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
	r = fwrite(&auth_data, sizeof(struct drcom_auth_data), 1, f);
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
	r = fwrite(&iface_data, sizeof(struct drcom_iface_data), 1, f);
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
	r = fwrite(&except_data, sizeof(struct drcom_except_data), MAX_EXCEPT_ITEMS, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_EXCEPT ": %s", strerror(errno));
		return;
	}
	fclose(f);

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
	r = fwrite(&status_data, sizeof(struct drcom_status_data), 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_STATUS ": %s", strerror(errno));
		return;
	}
	fclose(f);

	return;
}

void module_stop_auth(void)
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

	r = fwrite(&status_data, sizeof(struct drcom_status_data), 1, f);
	if (r == 0 && ferror(f)) {
		logerr("Writing to " DRCOM_MODULE_STATUS ": %s", strerror(errno));
		return;
	}

	loginfo("daemon: Stopped authentication\n");

	fclose(f);

	return;
}


