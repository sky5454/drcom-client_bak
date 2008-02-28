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

#include "md5.h"

#include "private.h"

void _build_passwd_packet(struct drcom_passwd_packet *, struct drcom_info *, struct drcom_challenge *, char *);

/* drcom_passwd
	Changes password.
*/

int drcom_passwd(struct drcom_handle *h, char *newpassword, int timeout)
{
	struct drcom_socks *socks = (struct drcom_socks *) h->socks;
	struct drcom_info *info = (struct drcom_info *) h->info;
	struct drcom_challenge challenge;
	struct drcom_passwd_packet passwd_packet;
	struct drcom_acknowledgement acknowledgement;

	_send_dialog_packet(socks, NULL, PKT_REQUEST);

	_recv_dialog_packet(socks, &challenge, PKT_CHALLENGE);

	_build_passwd_packet(&passwd_packet, info, &challenge, newpassword);
	_send_dialog_packet(socks, &passwd_packet, PKT_PASSWORD_CHANGE);

	_recv_dialog_packet(socks, &acknowledgement, PKT_ACK_SUCCESS);

	if (acknowledgement.serv_header.pkt_type == PKT_ACK_SUCCESS)
		return 0;
	else
		return -1;
}

void _build_passwd_packet(struct drcom_passwd_packet *passwd_packet, struct drcom_info *info, struct drcom_challenge *challenge, char *newpassword)
{
	unsigned char s[32], t[22], d[16];
	int i, l;

	/* header */
	passwd_packet->host_header.pkt_type = PKT_PASSWORD_CHANGE;
	passwd_packet->host_header.zero = 0;
	passwd_packet->host_header.len = sizeof(struct drcom_passwd_packet);
	memset(t, 0, 22);
	memcpy(t, &passwd_packet->host_header.pkt_type, 2);
	memcpy(t + 2, &challenge->challenge, 4);
	l = strlen(info->password);
	strncpy((char *) (t + 6), info->password, 16);
	MD5(t, l + 6, d);
	memcpy(passwd_packet->host_header.checksum, d, 16);

	/* username */
	memset(passwd_packet->username, 0, 16);
	strncpy(passwd_packet->username, info->username, 16);

	memset(s, 0, 32);
	memcpy(s, passwd_packet->host_header.checksum, 16);
	l = strlen(info->password);
	strncpy((char *) (s + 16), info->password, 16);
	MD5(s, 16 + l, d);
	memcpy(passwd_packet->checksum1_xor, d, 16);
	for (i = 0; i < 16; ++i)
		passwd_packet->checksum1_xor[i] ^= newpassword[i];

	/* unknown */
	passwd_packet->unknown0 = 0x12;
	passwd_packet->unknown1 = 0x16;
	passwd_packet->unknown2 = 0x04;
	passwd_packet->unknown3 = 0x00;

	return;
}

