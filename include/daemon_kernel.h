#ifndef DAEMON_KERNEL_H_
#define DAEMON_KERNEL_H_

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

#define MAX_EXCEPT_ITEMS 20
#define MAX_EXCEPT_LEN (sizeof(struct drcom_except_data) * MAX_EXCEPT_ITEMS)

/* The module name */
#define DRCOM_MODULE "drcom"

/* Files used by the module */
#define DRCOM_MODULE_PATH "/proc/drcom"
#define DRCOM_MODULE_STATUS DRCOM_MODULE_PATH"/status"
#define DRCOM_MODULE_AUTH DRCOM_MODULE_PATH"/auth"
#define DRCOM_MODULE_IFACE DRCOM_MODULE_PATH"/iface"
#define DRCOM_MODULE_EXCEPT DRCOM_MODULE_PATH"/except"
#define DRCOM_MODULE_PID DRCOM_MODULE_PATH"/pid"
#define DRCOM_MODULE_AUTOLOGOUT DRCOM_MODULE_PATH"/autologout"

#endif

