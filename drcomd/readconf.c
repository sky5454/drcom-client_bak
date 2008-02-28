/*
	libdrcom - Library for communicating with DrCOM 2133 Broadband Access Server
	Copyright (C) 2005 William Poetra Yoga Hadisoeseno <williampoetra@yahoo.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA	02111-1307	USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"

#include "private.h"

#define __OPTLEN 256

#define __istidy(x) \
	((x >= 'a' && x <= 'z') \
	|| (x >= 'A' && x <= 'Z') \
	|| (x >= '0' && x <= '9') \
	|| (x == '_' || x == '=' || x == '.' || x == ',' || x == ':'))

/*
	Some kind of checklist:
	+ 0 means option was never encountered in the config file
	+ 1 means option found, value specified and valid
	+ 2 means option found, value specified and invalid
	+ 3 means option found, but no value specified
	+ 4 means option found more than once
*/
struct _opt_checklist
{
	u_int8_t username;
	u_int8_t password;
	u_int8_t mac;
	u_int8_t nic[4];
	u_int8_t dnsp;
	u_int8_t dnss;
	u_int8_t dhcp;
	u_int8_t hostip;
	u_int8_t servip;
	u_int8_t hostport;
	u_int8_t servport;
	u_int8_t hostname;
	u_int8_t winver;
	u_int8_t winbuild;
	u_int8_t servicepack;
};

int __stripcomments(char *);
int __tidyup(char *);
int __optname(char *, char *);
int __optval(char *, char *);
int __parseopt(struct drcom_conf *, char *, struct _opt_checklist *opts);
int __fillopts(struct drcom_conf *, struct drcom_info *, struct drcom_host *, struct _opt_checklist *);

/* _readconf
	A routine for parsing the config file.
*/

int _readconf(struct drcom_conf *conf, struct drcom_info *info, struct drcom_host *host)
{
	FILE *dotconf;
	char buf[__OPTLEN], *s;
	struct _opt_checklist opts = {0,0,0,{0,0,0,0},0,0,0,0,0,0,0,0,0,0,0};
	int lineno = 0, r = 0;

	dotconf = fopen(DRCOM_CONF, "r");
	if (!dotconf)
		return -1;

	while (1)
	{
		s = fgets(buf, __OPTLEN, dotconf);
		if (s == NULL)
			break;

		r = __parseopt(conf, buf, &opts);

		++lineno;
		switch(r) {
		case 0:
			dbg("Line %d parsed successfully\n", lineno);
			break;
		case 1: dbg("Line %d skipped.\n", lineno); break;
		case 2: logerr("Error processing config file at line %d.\n", lineno);
			break;
		default: break;
		}

		if (r < 0 || r > 1)
			break;
	}

	/* Even if there was an error, we should close the file first */
	fclose(dotconf);

	if (r < 0 || r > 1)
		return -1;

	r = __fillopts(conf, info, host, &opts);
#ifdef DEBUG_CONF
	dbg("username: %s\n", info->username);
	dbg("password: %s\n", info->password);
	dbg("mac: %02x:%02x:%02x:%02x:%02x:%02x\n", info->mac[0], info->mac[1],
		 info->mac[2], info->mac[3], info->mac[4], info->mac[5]);
	dbg("nic0: %s\n", inet_ntoa(*((struct in_addr *) &(info->nic[0]))));
	dbg("nic1: %s\n", inet_ntoa(*((struct in_addr *) &(info->nic[1]))));
	dbg("nic2: %s\n", inet_ntoa(*((struct in_addr *) &(info->nic[2]))));
	dbg("nic3: %s\n", inet_ntoa(*((struct in_addr *) &(info->nic[3]))));
	dbg("hostip: %s\n", inet_ntoa(*((struct in_addr *) &(info->hostip))));
	dbg("servip: %s\n", inet_ntoa(*((struct in_addr *) &(info->servip))));
	dbg("hostport: %d\n", info->hostport);
	dbg("servport: %d\n", info->servport);

	dbg("hostname: %s\n", host->hostname);
	dbg("dnsp: %s\n", inet_ntoa(*((struct in_addr *) &(host->dnsp))));
	dbg("dhcp: %s\n", inet_ntoa(*((struct in_addr *) &(host->dhcp))));
	dbg("dnss: %s\n", inet_ntoa(*((struct in_addr *) &(host->dnss))));
	dbg("winver_major: %d\n", host->winver_major);
	dbg("winver_minor: %d\n", host->winver_minor);
	dbg("winver_build: %d\n", host->winver_build);
	dbg("servicepack: %s\n", host->servicepack);
#endif
	if (r)
	{
		logerr("Error digesting configuration!\n");
		return -1;
	}

	return 0;
}

int __stripcomments(char *buf)
{
	char *c;

	c = strchr(buf, '#');
	if (c != NULL)
		*c = '\0';

	return (c - buf + 1);
}

int __tidyup(char *buf)
{
	int i, len, tidylen, inquote;

	len = strlen(buf);
	tidylen = 0;
	inquote = 0;
	for (i = 0; i < len; ++i)
	{
		if (__istidy(buf[i]) || (inquote && (buf[i] != '"')))
		{
			buf[tidylen] = buf[i];
			++tidylen;
		}
		else if (buf[i] == '"')
		{
			inquote = (inquote) ? 0 : 1;
		}
	}
	buf[tidylen] = '\0';

	return tidylen;
}

int __optname(char *buf, char *optname)
{
	char *t;
	int len;

	memset(optname, 0, 256);

	t = strchr(buf, '=');
	if (t != NULL)
	{
		len = t - buf;
		strncpy(optname, buf, len);
	}
	else
	{
		len = -1;
		optname[0] = '\0';
	}

	return len;
}

int __optval(char *buf, char *optval)
{
	char *t;
	int len;

	memset(optval, 0, 256);

	t = strchr(buf, '=');
	if (t != NULL)
	{
		len = strlen(buf) - (t - buf + 1);
		strncpy(optval, t + 1, len);
	}
	else
	{
		len = -1;
		optval[0] = '\0';
	}

	return len;
}

int __parseopt(struct drcom_conf *conf, char *buf, struct _opt_checklist *opts)
{
/* define them here because they are __parseopt()-specific */
#define __optprefix(y, n) \
	(strncmp(optname, y, n) == 0)
#define __isopt(y, n) \
	(strncmp(optname, y, n) == 0 && optname_len == n)

	int len, optname_len, optval_len, r;
	long int l;
	unsigned int _mac[6];
	char optname[256], optval[256];
	struct in_addr ip = {0};

	len = strlen(buf);
	if (len > 256)
	{
		len = 256;
		buf[len] = '\0';
	}

	len = __stripcomments(buf);
	if (len == 0)
		goto skip;
	len = __tidyup(buf);
	if (len == 0)
		goto skip;

	optname_len = __optname(buf, optname);
	if (optname_len < 0)
		goto err;
	optval_len = __optval(buf, optval);
	if (optval_len < 0)
		goto err;

	if (optname_len == 0)
	{ goto err; }
	else if (__isopt("username", 8))
	{
		if (opts->username != 0) { opts->username = 4; goto ok; }
		if (optval_len == 0) { opts->username = 3; goto ok; }
		memset(conf->username, 0, 36);
		strncpy(conf->username, optval, 36);
		opts->username = 1; goto ok;
	}
	else if (__isopt("password", 8))
	{
		if (opts->password != 0) { opts->password = 4; goto ok; }
		if (optval_len == 0) { opts->password = 3; goto ok; }
		memset(conf->password, 0, 16);
		strncpy(conf->password, optval, 16);
		opts->password = 1; goto ok;
	}
	else if (__isopt("mac", 3))
	{
		if (opts->mac != 0) { opts->mac = 4; goto ok; }
		if (optval_len == 0) { opts->mac = 3; goto ok; }
		else
		{
			r = sscanf(optval, "%02x:%02x:%02x:%02x:%02x:%02x",
				 &_mac[0], &_mac[1], &_mac[2], &_mac[3], &_mac[4], &_mac[5]);
			if (r < 6) { opts->mac = 2; goto err; }
			conf->mac[0] = _mac[0]; conf->mac[1] = _mac[1]; conf->mac[2] = _mac[2];
			conf->mac[3] = _mac[3]; conf->mac[4] = _mac[4]; conf->mac[5] = _mac[5];
			opts->mac = 1; goto ok;
		}
	}
	else if (__optprefix("nic", 3))
	{
		if (optname_len > 4) goto err;
		l = optname[3] - '0';
		if (l >= 0 && l <= 3)
		{
			if (opts->nic[l] != 0) { opts->nic[l] = 4; goto ok; }
			if (optval_len == 0) { opts->nic[l] = 3; goto ok; }
			r = inet_pton(AF_INET, optval, &ip);
			if (r == 0) { opts->nic[l] = 2; goto err; }
			conf->nic[l] = ip.s_addr;
			opts->nic[l] = 1;
		}
		else goto err;
	}
	else if (__optprefix("dns", 3))
	{
		if (optname_len > 4) goto err;
		switch (optname[3])
		{
			case 'p':
				if (opts->dnsp != 0) { opts->dnsp = 4; goto ok; }
				if (optval_len == 0) { opts->dnsp = 3; goto ok; }
				r = inet_pton(AF_INET, optval, &ip);
				if (r == 0) { opts->dnsp = 2; goto err; }
				conf->dnsp = ip.s_addr;
				opts->dnsp =	1; goto ok;
				break;
			case 's':
				if (opts->dnss != 0) { opts->dnss = 4; goto ok; }
				if (optval_len == 0) { opts->dnss = 3; goto ok; }
				r = inet_pton(AF_INET, optval, &ip);
				if (r == 0) { opts->dnss = 2; goto err; }
				conf->dnss = ip.s_addr;
				opts->dnss =	1; goto ok;
				break;
			default: goto err; break;
		}
	}
	else if (__isopt("dhcp", 4))
	{
		if (opts->dhcp != 0) { opts->dhcp = 4; goto ok; }
		if (optval_len == 0) { opts->dhcp = 3; goto ok; }
		r = inet_pton(AF_INET, optval, &ip);
		if (r == 0) { opts->dhcp = 2; goto err; }
		conf->dhcp = ip.s_addr;
		opts->dhcp = 1; goto ok;
	}
	else if (__isopt("hostip", 6))
	{
		if (opts->hostip != 0) { opts->hostip = 4; goto ok; }
		if (optval_len == 0) { opts->hostip = 3; goto ok; }
		r = inet_pton(AF_INET, optval, &ip);
		if (r == 0)
		{ opts->hostip = (_getaddr(optval, &conf->hostip) == 0) ? 1 : 2; goto ok; }
		conf->hostip = ip.s_addr;
		opts->hostip = 1;
	}
	else if (__isopt("servip", 6))
	{
		if (opts->servip != 0) { opts->servip = 4; goto ok; }
		if (optval_len == 0) { opts->servip = 3; goto ok; }
		r = inet_pton(AF_INET, optval, &ip);
		if (r == 0) { opts->servip = 2; goto err; }
		conf->servip = ip.s_addr;
		opts->servip = 1;
	}
	else if (__isopt("hostport", 8))
	{
		if (opts->hostport != 0) { opts->hostport = 4; goto ok; }
		if (optval_len == 0) { opts->hostport = 3; goto ok; }
		l = strtol(optval, (char **) NULL, 0);
		if ((l == LONG_MIN || l == LONG_MAX) || l > 0xffff)
		{ opts->hostport = 2; goto err; }
		conf->hostport = (u_int16_t) l;
		opts->hostport = 1; goto ok;
	}
	else if (__isopt("servport", 8))
	{
		if (opts->servport != 0) { opts->servport = 4; goto ok; }
		if (optval_len == 0) { opts->servport = 3; goto ok; }
		l = strtol(optval, (char **) NULL, 0);
		if ((l == LONG_MIN || l == LONG_MAX) || l > 0xffff)
		{ opts->servport = 2; goto err; }
		conf->servport = (u_int16_t) l;
		opts->servport = 1; goto ok;
	}
	else if (__isopt("hostname", 8))
	{
		if (opts->hostname != 0) { opts->hostname = 4; goto ok; }
		if (optval_len == 0) { opts->hostname = 3; goto ok; }
		memset(conf->hostname, 0, 32);
		strncpy(conf->hostname, optval, 32);
		opts->hostname = 1; goto ok;
	}
	else if (__isopt("winver", 6))
	{
		if (opts->winver != 0) { opts->winver = 4; goto ok; }
		if (optval_len == 0) { opts->winver = 3; goto ok; }
		r = sscanf(optval, "%u.%u", &conf->winver_major, &conf->winver_minor);
		if (r < 2) { opts->winver = 2; goto err; }
		opts->winver = 1; goto ok;
	}
	else if (__isopt("winbuild", 8))
	{
		if (opts->winbuild != 0) { opts->winbuild = 4; goto ok; }
		if (optval_len == 0) { opts->winbuild = 3; goto ok; }
		r = sscanf(optval, "%u", &conf->winver_build);
		if (r < 1) { opts->winbuild = 2; goto err; }
		opts->winbuild = 1; goto ok;
	}
	else if (__isopt("servicepack", 11))
	{
		if (opts->servicepack != 0) { opts->servicepack = 4; goto ok; }
		if (optval_len == 0) { opts->servicepack = 3; goto ok; }
		memset(conf->servicepack, 0, 32);
		strncpy(conf->servicepack, optval, 32);
		opts->servicepack = 1; goto ok;
	}
	else if(__isopt("autologout", 10))
	{
		if(optval_len != 0 && optval[0] == '1' ){
			conf->autologout = 1; goto ok; 
		}else{
			conf->autologout =0; goto ok;
		}
	}else { goto err; }

ok:
	return 0;

skip:
	return 1;

err:
	return 2;
}

int __fillopts(struct drcom_conf *conf, struct drcom_info *info, struct drcom_host *host, struct _opt_checklist *opts)
{
	/* Check the options one by one */

	if (opts->username == 1)
		memcpy(info->username, conf->username, 36);
	else
		return -1;

	if (opts->password == 1)
		memcpy(info->password, conf->password, 16);
	else
		return -1;

	if (opts->mac == 1)
		memcpy(info->mac, conf->mac, 6);
	else if (opts->mac == 0 || opts->mac == 3)
		memset(info->mac, 0, 6);
	else
		return -1;

	if (opts->nic[0] == 1)
		info->nic[0] = conf->nic[0];
	else
		return -1;

	if (opts->nic[1] == 1)
		info->nic[1] = conf->nic[1];
	else if (opts->nic[1] == 0 || opts->nic[1] == 3)
		info->nic[1] = 0;
	else
		return -1;

	if (opts->nic[2] == 1)
		info->nic[2] = conf->nic[2];
	else if (opts->nic[2] == 0 || opts->nic[2] == 3)
		info->nic[2] = 0;
	else
		return -1;

	if (opts->nic[3] == 1)
		info->nic[3] = conf->nic[3];
	else if (opts->nic[3] == 0 || opts->nic[3] == 3)
		info->nic[3] = 0;
	else
		return -1;

	if (opts->dnsp == 1)
		host->dnsp = conf->dnsp;
	else
		return -1;

	if (opts->dnss == 1)
		host->dnss = conf->dnss;
	else
		return -1;

	if (opts->dhcp == 1)
		host->dhcp = conf->dhcp;
	else if (opts->dhcp == 0 || opts->dhcp == 3)
		host->dhcp = 0xffffffff;	/* Like Windows XP */
	else
		return -1;

	/* No need to check for validity of conf->nic[0] here,
		 since it's already checked */
	if (opts->hostip == 1)
		info->hostip = conf->hostip;
	else if (opts->hostip == 0 || opts->hostip == 3)
		info->hostip = conf->nic[0];
	else
		return -1;

	if (opts->servip == 1)
		info->servip = conf->servip;
	else if (opts->servip == 0 || opts->servip == 3)
		info->servip = 0x01010101; /* 1.1.1.1 */
	else
		return -1;

	if (opts->hostport == 1)
		info->hostport = conf->hostport;
	else if (opts->hostport == 0 || opts->hostport == 3)
		info->hostport = 0xf000; /* 61440 */
	else
		return -1;

	if (opts->servport == 1)
		info->servport = conf->servport;
	else if (opts->servport == 0 || opts->servport == 3)
		info->servport = 0xf000; /* 61440 */
	else
		return -1;

	if (opts->hostname == 1)
		memcpy(host->hostname, conf->hostname, 32);
	else if (opts->hostname == 0 || opts->hostname == 3)
	{
		memset(host->hostname, 0, 32);
		strncpy(host->hostname, "localhost", 32);	/* Does Windows XP use this? */
	}
	else
		return -1;

	if (opts->winver == 1)
	{
		host->winver_major = conf->winver_major;
		host->winver_minor = conf->winver_minor;
	}
	else if (opts->winver == 0 || opts->winver == 3)
	{
		host->winver_major = 5;	/* Windows XP's version */
		host->winver_minor = 1;	/* is NT 5.1 */
	}
	else
		return -1;

	if (opts->winbuild == 1)
		host->winver_build = conf->winver_build;
	else if (opts->winbuild == 0 || opts->winbuild == 3)
		host->winver_build = 2600;	/* Windows XP is of build 2600 */
	else
		return -1;

	if (opts->servicepack == 1)
		memcpy(host->servicepack, conf->servicepack, 32);
	else if (opts->servicepack == 0 || opts->servicepack == 3)
	{
		memset(host->servicepack, 0, 32);
		strncpy(host->servicepack, "Service Pack 2", 32); /* The latest XP sp */
	}
	else
		return -1;

	/* Finally, the easy ones */
	host->zero0[0] = 0;
	host->zero0[1] = 0;
	host->unknown0 = 148;
	host->unknown1 = 2;

	return 0;
}

