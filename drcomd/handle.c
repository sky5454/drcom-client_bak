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

#include "private.h"

/* drcom_create_handle
	Returns a struct drcom_handle pointer
*/

struct drcom_handle *drcom_create_handle(void)
{
	struct drcom_handle *h;

	h = (struct drcom_handle *) malloc(sizeof(struct drcom_handle));
	h->conf = (u_int8_t *) malloc(sizeof(struct drcom_conf));
	h->socks = (u_int8_t *) malloc(DRCOM_SOCKS_LEN);
	h->info = (u_int8_t *) malloc(DRCOM_INFO_LEN);
	h->host = (u_int8_t *) malloc(DRCOM_HOST_LEN);
	h->auth = (u_int8_t *) malloc(DRCOM_AUTH_LEN);
	h->keepalive = (u_int8_t *) malloc(DRCOM_HOST_MSG_LEN);
	h->response = (u_int8_t *) malloc(DRCOM_HOST_MSG_LEN);
	h->msgprint = NULL;

	return h;
}

/* drcom_destroy_handle
	Frees up resources used by handler
*/

int drcom_destroy_handle(struct drcom_handle *h)
{
	free(h->conf);
	free(h->socks);
	free(h->info);
	free(h->host);
	free(h->auth);
	free(h->keepalive);
	free(h->response);
	h->msgprint = NULL;
	free(h);

	return 0;
}

