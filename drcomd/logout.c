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
#include <string.h>

#include "md5.h"

#include "log.h"
#include "private.h"

void _build_logout_packet(struct drcom_logout_packet *, struct drcom_info *, struct drcom_challenge *, struct drcom_auth *);

/* drcom_logout
	Logs out.
*/

int drcom_logout(struct drcom_handle *h, int timeout)
{
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;
	struct drcom_info *info = (struct drcom_info *) h->info;
	struct drcom_auth *auth = (struct drcom_auth *) h->auth;
	struct drcom_challenge challenge;
	struct drcom_logout_packet logout_packet;
	struct drcom_acknowledgement acknowledgement;
	int retry = 0;

try_it_again_1:
	retry++;
	if(retry > 3)
		return -1;
	_send_dialog_packet(socks, NULL, PKT_REQUEST);
	if(_recv_dialog_packet(socks, &challenge, PKT_CHALLENGE)<0){
		logerr("_recv_dialog_package(PKT_CHALLENGE) failed\n");
		goto try_it_again_1;
	}

	_build_logout_packet(&logout_packet, info, &challenge, auth);

	retry = 0;
try_it_again_2:
	retry++;
	if(retry > 3)
		return -1;
	_send_dialog_packet(socks, &logout_packet, PKT_LOGOUT);
	if(_recv_dialog_packet(socks, &acknowledgement, PKT_ACK_SUCCESS)<0){
		logerr("_recv_dialog_package(PKT_ACK_SUCCESS) failed\n");
		goto try_it_again_2;
	}

	if (acknowledgement.serv_header.pkt_type == PKT_ACK_SUCCESS){
		loginfo("You have used %u minutes, and %uK bytes\n", 
			acknowledgement.time_usage, acknowledgement.vol_usage);
		return 0;
	} else
		return -1;
}

void _build_logout_packet(struct drcom_logout_packet *logout_packet, 
			struct drcom_info *info, struct drcom_challenge *challenge, 
			struct drcom_auth *auth)
{
	unsigned char t[22], d[16];
	int i, passwd_len;

	/* header */
	logout_packet->host_header.pkt_type = PKT_LOGOUT;
	logout_packet->host_header.zero = 0;
	logout_packet->host_header.len = strlen(info->username) + 
					sizeof(struct drcom_host_header);
	memset(t, 0, 22);
	memcpy(t, &logout_packet->host_header.pkt_type, 2);
	memcpy(t + 2, &challenge->challenge, 4);
	passwd_len = strlen(info->password);
	strncpy((char *) (t + 6), info->password, 16);
	MD5(t, passwd_len + 6, d);
	memcpy(logout_packet->host_header.checksum, d, 16);

	/* username */
	memset(logout_packet->username, 0, 36);
	strncpy(logout_packet->username, info->username, 36);

	/* unknown, maybe just a signature? */
	logout_packet->unknown0 = 0x18;

	/* mac */
	logout_packet->mac_code = 1;
	memcpy(logout_packet->mac_xor, info->mac, 6);
	for (i = 0; i < 6; ++i)
		logout_packet->mac_xor[i] ^= logout_packet->host_header.checksum[i];

	/* auth data */
	memcpy(&logout_packet->auth_info, auth, sizeof(struct drcom_auth));

	return;
}

