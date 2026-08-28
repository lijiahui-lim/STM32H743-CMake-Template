/*
 * YAFFS2 configuration for STM32H743 + FreeRTOS
 * Must be included before any YAFFS2 header via yportenv.h.
 */

#ifndef YAFFS_CFG_H
#define YAFFS_CFG_H

#include <stdint.h>
#include <stddef.h>

/* Use direct (POSIX-like) interface */
#define CONFIG_YAFFS_DIRECT		1
#define CONFIG_YAFFS_YAFFS2		1
#define CONFIG_YAFFS_SHORT_NAMES_IN_RAM 1

/* Types and definitions provided by yportenv.h */
#define CONFIG_YAFFS_DEFINES_TYPES    1
#define CONFIG_YAFFS_PROVIDE_DEFS     1
#define CONFIG_YAFFSFS_PROVIDE_VALUES 1

/* 64-bit time (default) */

/* Disable trace output (printf not available on UART by default) */
#define CONFIG_YAFFS_NO_TRACE 1

/* loff_t not available on bare-metal, define it */
#ifndef _LOFF_T_DEFINED
#define _LOFF_T_DEFINED
typedef long long loff_t;
#endif

#ifndef _OFF_T_DEFINED
#define _OFF_T_DEFINED
typedef long off_t;
#endif

#endif /* YAFFS_CFG_H */
