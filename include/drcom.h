/*
  libdrcom - Library for communicating with DrCOM 2133 Broadband Access Server
  Copyright (C) 2005 William Poetra Yoga Hadisoeseno <williampoetra@yahoo.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef DRCOM_H
#define DRCOM_H

#include <stdint.h>

/* Use a simple handle */

struct drcom_handle
{
  uint8_t *conf;
  uint8_t *socks;
  uint8_t *info;
  uint8_t *host;
  uint8_t *auth;
  uint8_t *keepalive;
  uint8_t *response;
  void (* msgprint)(const char *);
};

/* Used by drcomcd to initialize drcom.o */

struct drcom_session_info
{
  uint8_t *auth;
  uint32_t hostip;
  uint32_t servip;
  uint16_t hostport;
  uint16_t servport;
  uint32_t dnsp;
  uint32_t dnss;
};

/* Functions */

struct drcom_handle *drcom_create_handle(void);
int drcom_destroy_handle(struct drcom_handle *);

int drcom_init(struct drcom_handle *, void (*msgprint)(const char *));
int drcom_cleanup(struct drcom_handle *);
int drcom_login(struct drcom_handle *, int);
int drcom_logout(struct drcom_handle *, int);
int drcom_passwd(struct drcom_handle *, char *, int);
int drcom_keepalive(struct drcom_handle *);
int drcom_watchport(struct drcom_handle *);

struct drcom_session_info *drcom_get_session_info(struct drcom_handle *);
int drcom_get_authlen(void);

#endif

