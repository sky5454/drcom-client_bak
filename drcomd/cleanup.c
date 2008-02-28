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
#include <sys/socket.h>

#include "private.h"

/* drcom_cleanup
	Cleanup code, frees socket resources.
*/

int drcom_cleanup(struct drcom_handle *h)
{
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;
	int r;

	r = shutdown(socks->sockfd, SHUT_RDWR);
	if (r)
		return r;

	r = close(socks->sockfd);
	if (r)
		return r;

	return 0;
}

