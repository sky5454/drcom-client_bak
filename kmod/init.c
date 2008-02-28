#include "drcom_linux.h"

#include "../include/config.h"

extern int drcom_init_proc(void);
extern void drcom_cleanup_proc(void);
extern int drcom_init_proto(void);
extern void drcom_cleanup_proto(void);

static int __init drcom_init(void)
{
  drcom_init_proc();
  drcom_init_proto();
  return 0;
}

static void __exit drcom_exit(void)
{
  drcom_cleanup_proto();
  drcom_cleanup_proc();
  return;
}

module_init(drcom_init)
module_exit(drcom_exit)

MODULE_LICENSE("GPL");
MODULE_AUTHOR("William Poetra Yoga Hadisoeseno/Modified by wheelz");
MODULE_DESCRIPTION("Provides authentication for drcom");

