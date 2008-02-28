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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "log.h"

#include "private.h"

/*
	type is one of: (all LSB)
	- 0x0001 request
	- 0x0002 challenge
	- 0x0103 login
	- 0x0106 logout
	- 0x0109 passwd
	- 0x0004 success
	- 0x0005 failure
	The length of buf is determined by type.

	If an unexpected packet is recv'ed, we just ignore it.
*/

int _send_dialog_packet(struct drcom_socks *socks, void *buf, uint16_t type)
{
	struct drcom_request *request=NULL;
	int len, r;

	switch (type) {
	case PKT_REQUEST:
		len = sizeof(struct drcom_request);
		request = (struct drcom_request *) malloc(sizeof(struct drcom_request));
		if(request == NULL){
			logerr("_send_dialog_packet:malloc failed\n");
			return -1;
		}
		request->host_header.pkt_type = PKT_REQUEST;
		request->host_header.len = 4;
		memset(&request->host_header.checksum, 0, 16);
		request->host_header.checksum[0] = 1;
		buf = (void *) request;
		loginfo("send: PKT_REQUEST\n");
		break;
	case PKT_LOGIN: 
		len = sizeof(struct drcom_login_packet); 
		loginfo("send: PKT_LOGIN\n");
		break;
	case PKT_LOGOUT: 
		len = sizeof(struct drcom_logout_packet); 
		loginfo("send: PKT_LOGOUT\n");
		break;
	case PKT_PASSWORD_CHANGE: 
		len = sizeof(struct drcom_passwd_packet); 
		loginfo("send: PKT_PASSWD_CHANGE\n");
		break;
	default: 
		loginfo("send: Unknown PKT\n");
		return -2; 
		break;
	}

	r = sendto(socks->sockfd, buf, len, 0,
			(struct sockaddr *) &socks->servaddr_in,
			sizeof(struct sockaddr));

	if(request)
		free(request);

	if (r != len){
		logerr("send:failed\n");
		return -1;
	}

	return 0;
}

int _recv_dialog_packet(struct drcom_socks *socks, void *buf, uint16_t type)
{
	fd_set readfds;
	int len, r;

	switch (type) {
	case PKT_CHALLENGE: len = sizeof(struct drcom_challenge); break;
	case PKT_ACK_SUCCESS:
	case PKT_ACK_FAILURE: len = sizeof(struct drcom_acknowledgement); break;
	default: return -2; break;
	}

	while(1){
		struct timeval t;

		FD_ZERO(&readfds);
		FD_SET(socks->sockfd, &readfds);
		t.tv_sec = 2;
		t.tv_usec = 0;
		r = select(socks->sockfd+1, &readfds, NULL, NULL, &t);
		if(r < 0){
			if(errno == EINTR)
				continue;
			logerr("select() returned negative(errno=%d)\n", errno);
			return -1;
		}
		if(r==0){
			logerr("select() timeout\n");
			return -1;
		}

		if(FD_ISSET(socks->sockfd, &readfds)){
			r = recvfrom(socks->sockfd, buf, len, 0,
				(struct sockaddr *) &socks->servaddr_in,
		 		(unsigned int *) &len);

			if (r < 0)
				return -1;

			switch(*((uint16_t*)buf)){
			case PKT_CHALLENGE:
				loginfo("recv: PKT_CHALLENGE\n");
				break;
			case PKT_ACK_SUCCESS:
				loginfo("recv: PKT_ACK_SUCCESS\n");
				break;
			case PKT_ACK_FAILURE:
				loginfo("recv: PKT_ACK_FAILURE\n");
				break;
			default:
				loginfo("recv: Unknown PKT %x\n", (*((uint16_t*)buf)));
				break;
			}
			if (*((uint16_t *) buf) == PKT_CHALLENGE) {
				if (type == PKT_CHALLENGE)
					return r;
			} else if ((*((uint16_t *) buf) == PKT_ACK_SUCCESS)
					 || (*((uint16_t *) buf) == PKT_ACK_FAILURE))
			{
				if ((type == PKT_ACK_SUCCESS)
					 || (type == PKT_ACK_FAILURE))
					 return r;
			}

			/* If we got here, that means we encountered an unknown packet */
			loginfo("recved unknown server packet\n");
			{
			int i;
			unsigned char *p = buf;
			loginfo("len = %d\n", r);
			for (i=1;i<r;i++){
				loginfo("%02x ", (int)p[i-1]);
				if(i%16==0)
					loginfo("\n");
			}
			loginfo("\n");
		}
		
		return -1;
	}
	/* is it possible? */
	return -1;
	}/*while*/
	return -1;
}

