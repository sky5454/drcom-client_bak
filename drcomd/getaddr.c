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

#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#include <linux/if.h>
#include <linux/sockios.h>

#include "private.h"

int _getaddr(char *name, u_int32_t *addr)
{
	int r, s = socket(PF_INET, SOCK_DGRAM, 0);
	struct ifreq ifr;

	strncpy(ifr.ifr_name, name, IFNAMSIZ);
	r = ioctl(s, SIOCGIFADDR, &ifr);
	if (r)
		return -errno;

	*addr = ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;

	return 0;
}

