#ifndef DRCOM_TYPES_H
#define DRCOM_TYPES_H

#include <linux/types.h>

/* I've got a feeling we don't need to pack the structures...
   So they're not packed for now */

struct drcom_status_data
{
#define STATUS_NOTLOGIN '0'
#define STATUS_LOGIN '1'
  uint8_t status;
};

struct drcom_auth_data
{
  uint8_t auth[16];
};

struct drcom_iface_data
{
  uint32_t hostip;
  uint16_t hostport;
  uint32_t servip;
  uint16_t servport;
  uint32_t dnsp;
  uint32_t dnss;
};

struct drcom_except_data
{
  uint32_t addr;
  uint32_t mask;
};

#endif

