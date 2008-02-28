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
#include <ctype.h>
#include <sys/ioctl.h>
#include <netinet/in.h>

#include "drcomd.h"
#include "daemon_server.h"
#include "log.h"

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

static void dump_packet(unsigned char *pkt, int len)
{
	char left[64];
	char right[20];
	char tmp[8];
	int n = 0;
	unsigned char *p=pkt;

	left[0]='\0';
	loginfo("------dump packet(size=%d)------\n", len);
	while (p < pkt+len) {
		if (n%2==1)
			sprintf(tmp, "%02X ", *p);
		else
			sprintf(tmp, "%02X", *p);
		strcat(left, tmp);
		if (isprint(*p))
			right[n] = *p;
		else
			right[n] = '.';
		n = (n+1)%16;
		if (n==0){
			right[16] = '\0';
			loginfo("%s  %s\n", left, right);
			left[0]='\0';
		}
		p++;
	}

	if(n!=0){
		right[n] = '\0';
		loginfo("%s  %s\n", left, right);
	}
}

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
		memset(request, 0, sizeof(struct drcom_request));

		request->host_header.pkt_type = PKT_REQUEST;
		request->host_header.zero = 0;
		request->host_header.len = 4;
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
	unsigned char *p=NULL;
	fd_set readfds;
	int len, r;
	uint16_t recv_type;
	int pkt_size;

        switch (type) {
        case PKT_CHALLENGE: len = sizeof(struct drcom_challenge); break;
        case PKT_ACK_SUCCESS:
        case PKT_ACK_FAILURE: len = sizeof(struct drcom_acknowledgement); break;
        default: return -2; break;
        }

	while(1){
		struct timeval t;
		int addrlen;

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

		if(!FD_ISSET(socks->sockfd, &readfds))
			continue;

		r = ioctl(socks->sockfd, FIONREAD, &pkt_size);
		if(r != 0){
	                logerr("_recv_dialog_packet: ioctl()");
			return -1;
		}
		if(pkt_size == 0)
			continue;

		p = (unsigned char*)malloc(pkt_size);
		if (p==NULL){
			logerr("malloc failed\n");
			return -1;
		}

		addrlen = sizeof(struct sockaddr);
		r = recvfrom(socks->sockfd, p, pkt_size, 0,
			(struct sockaddr *) &socks->servaddr_in,
	 		(unsigned int *) &addrlen);

		if (r < 0){
			logerr("recvfrom: %s\n", strerror(errno));
			free(p);
			return -1;
		}

		if(len > pkt_size)
			len = pkt_size;

		memcpy(buf, p, len);

		/* FIXME: check len */
		break;
	}

	recv_type = *((uint16_t*)buf);

	switch(recv_type){
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
		/* If we got here, it means we encountered an unknown packet */
		loginfo("recved unknown server packet\n");
		dump_packet(p, pkt_size);
		break;
	}

	if(p)
		free(p);

	if(type == PKT_CHALLENGE){
		if(recv_type == PKT_CHALLENGE)
			return r;
		else
			return -1;
	}else if(recv_type == PKT_ACK_SUCCESS || recv_type == PKT_ACK_FAILURE)
		return r;
	
	return -1;
}

