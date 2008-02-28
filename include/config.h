#ifndef CONFIG_H
#define CONFIG_H

/* #define DEBUG */

#define STATUS_LEN (sizeof(struct drcom_status_data))
#define AUTH_LEN (sizeof(struct drcom_auth_data))
#define IFACE_LEN (sizeof(struct drcom_iface_data))
#define EXCEPT_LEN (sizeof(struct drcom_except_data))

#define MAX_EXCEPT_ITEMS 20
#define MAX_EXCEPT_LEN (EXCEPT_LEN * MAX_EXCEPT_ITEMS)

/* The socket name */
#define DRCOMCD_SOCK "/var/run/drcomcd"

/* Log file */
#define DRCOMCD_LOG_FILE "/var/log/drcomcd"

/* The module name */
#define DRCOM_MODULE "drcom"

/* Files used by the module */
#define DRCOM_MODULE_PATH "/proc/drcom"
#define DRCOM_MODULE_STATUS DRCOM_MODULE_PATH"/status"
#define DRCOM_MODULE_AUTH DRCOM_MODULE_PATH"/auth"
#define DRCOM_MODULE_IFACE DRCOM_MODULE_PATH"/iface"
#define DRCOM_MODULE_EXCEPT DRCOM_MODULE_PATH"/except"

/* Turn on debugging mode */
/* conflicts with linux kernel headers */
/*#define DRCOM_DEBUG*/

#endif

