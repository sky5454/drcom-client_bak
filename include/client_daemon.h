#ifndef CLIENT_DAEMON
#define CLIENT_DAEMON

#include <stdint.h>
#include <time.h>

/* Signature */

#define DRCOM_SIGNATURE 0xd4c0

/* The packet header */

struct drcomcd_hdr
{
  uint16_t signature; /* must be 0xd4c0 */
  uint16_t type;
};

/* The basic operations */

#define DRCOMCD_LOGIN    0x0103
#define DRCOMCD_LOGOUT   0x0106
#define DRCOMCD_PASSWD   0x0109

/* The data sent by drcomc */

struct drcomcd_login
{
  int authenticate;
  int timeout;
};

struct drcomcd_logout
{
  int timeout;
};

struct drcomcd_passwd
{
  char newpasswd[16];
  int timeout;
};

/* Replies from drcomcd */

#define DRCOMCD_SUCCESS  0x0004
#define DRCOMCD_FAILURE  0x0005

/* The explanation */

struct drcomcd_result
{
#define DRCOMCD_REASON_NO_REASON 0
#define DRCOMCD_REASON_TIMEOUT 1
#define DRCOMCD_REASON_REJECTED 2
#define DRCOMCD_REASON_BUSY 3
#define DRCOMCD_REASON_UNKNOWN 4
  int reason;
};

/* The maximum size of packets on the socket */

#define DRCOMCD_BUF_LEN 100

#endif

