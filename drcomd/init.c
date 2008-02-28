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

#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "private.h"

/* drcom_init
	Calls _readconf to initialize the variables for passing to the server, then
	initializes sockets and returns.
*/

int server_sock_init(struct drcom_handle *h);
void server_sock_destroy(struct drcom_handle *h);

int drcom_init(struct drcom_handle *h, void (*msgprint)(const char *))
{
	struct drcom_conf *conf = (struct drcom_conf *) h->conf;
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;
	struct drcom_info *info = (struct drcom_info *) h->info;
	struct drcom_host *host = (struct drcom_host *) h->host;
	int r;

	/* Read config file and actually initialize host and info
		 with the config data */
	r = _readconf(conf, info, host);
	if (r)
		return r;

	/* Initialize sockets */

	socks->hostaddr_in.sin_family = AF_INET;
	socks->hostaddr_in.sin_port = htons(info->hostport);
	socks->hostaddr_in.sin_addr.s_addr = info->hostip;
	memset(socks->hostaddr_in.sin_zero, 0, sizeof(socks->hostaddr_in.sin_zero));

	socks->servaddr_in.sin_family = AF_INET;
	socks->servaddr_in.sin_port = htons(info->servport);
	socks->servaddr_in.sin_addr.s_addr = info->servip;
	memset(socks->servaddr_in.sin_zero, 0, sizeof(socks->servaddr_in.sin_zero));

	h->msgprint = msgprint;

	return 0;
}

void server_sock_destroy(struct drcom_handle *h)
{
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;

	if(socks->sockfd >= 0)
		close(socks->sockfd);
	socks->sockfd = -1;
}

int server_sock_init(struct drcom_handle *h)
{
	int r;
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;

	socks->sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	if (socks->sockfd == -1)
		return -1;

	{
	int on = 1;
	r = setsockopt(socks->sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));
	if( r < 0 )
		return -1;
	}

	r = bind(socks->sockfd, 
			(struct sockaddr *)&socks->hostaddr_in, sizeof(struct sockaddr));
	if (r == -1)
		return -1;

	return 0;
}

