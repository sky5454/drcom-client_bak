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

#include <stdlib.h>
#include <string.h>

#include "private.h"

struct drcom_session_info *drcom_get_session_info(struct drcom_handle *h)
{
	struct drcom_session_info *s = 
		(struct drcom_session_info *) malloc(sizeof(struct drcom_session_info));
	struct drcom_info *info = (struct drcom_info *) h->info;
	struct drcom_host *host = (struct drcom_host *) h->host;
	u_int8_t *auth = (u_int8_t *) malloc(DRCOM_AUTH_LEN);

	memcpy(auth, h->auth, DRCOM_AUTH_LEN);
	s->auth = auth;
	s->hostip = info->hostip;
	s->servip = info->servip;
	s->hostport = info->hostport;
	s->servport = info->servport;
	s->dnsp = host->dnsp;
	s->dnss = host->dnss;

	return s;
}

int drcom_get_authlen()
{
	return DRCOM_AUTH_LEN;
}

