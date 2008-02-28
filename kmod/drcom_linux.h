#ifndef DRCOM_LINUX_H
#define DRCOM_LINUX_H

/* remember to define these in the makefile (just for 2.4 kernels)
#define MODULE
#define __KERNEL__
*/

#if (!(defined(__KERNEL__) && defined(MODULE)))
#error Please define __KERNEL__ and MODULE
#endif

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

#ifndef UTS_RELEASE
#include <linux/version.h>
#endif

#if ((!defined(KERNEL_VERSION)) || (LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)))
#error Kernels older than 2.4.0 are not supported (yet).
#endif

#define MAJOR_MINOR (LINUX_VERSION_CODE & 0x00ffff00)
#define REVISION (LINUX_VERSION_CODE & 0x000000ff)
#define MAJOR_MINOR_IS(a,b) (MAJOR_MINOR == KERNEL_VERSION(a,b,0))

#if (!(MAJOR_MINOR_IS(2,4) || MAJOR_MINOR_IS(2,6)))
#error Currently only 2.4.x and 2.6.x kernels are supported.
#endif

#endif

