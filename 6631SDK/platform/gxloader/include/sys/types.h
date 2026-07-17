#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#ifdef CONFIG_ENABLE_UBI
#include "util.h"
#include <linux/types.h>
#include <linux/compat.h>

typedef unsigned char		uchar;
typedef int pthread_key_t;

#ifdef DEBUG
#define debug(fmt,args...)     printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif /* DEBUG */

#else
#include "util.h"

typedef unsigned short umode_t;
typedef long long off_t;
typedef int pthread_key_t;
typedef unsigned long int uintptr_t;

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

#ifndef NULL
#define NULL	0
#endif

typedef int bool;

enum {
	false	= 0,
	true	= 1
};

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(__GNUC__)
__extension__ typedef __signed__ long long __s64;
__extension__ typedef unsigned long long __u64;
#endif

typedef unsigned int		__kernel_size_t;
typedef int			__kernel_ssize_t;

typedef long			__kernel_time_t;
#ifndef _TIME_T
#define _TIME_T
typedef __kernel_time_t		time_t;
#endif

#ifdef __GNUC__
//typedef long long		__kernel_loff_t;
typedef long		__kernel_loff_t;
#endif

/*
 * The following typedefs are also protected by individual ifdefs for
 * historical reasons:
 */
#ifndef size_t
#define size_t __kernel_size_t
#endif

#ifndef ssize_t
#define ssize_t __kernel_ssize_t
#endif

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __kernel_loff_t		loff_t;
#endif

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

typedef unsigned char		uchar;

/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char unchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef		__u8		u_int8_t;
typedef		__s8		int8_t;
typedef		__u16		u_int16_t;
typedef		__s16		int16_t;
typedef		__u32		u_int32_t;
typedef		__s32		int32_t;

typedef		__u8		uint8_t;
typedef		__u16		uint16_t;
typedef		__u32		uint32_t;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef		__u64		uint64_t;
typedef		__u64		u_int64_t;
typedef		__s64		int64_t;
#endif

#define BITS_PER_LONG 32

/* Dma addresses are 32-bits wide.  */

typedef u32 dma_addr_t;

typedef unsigned long phys_addr_t;
typedef unsigned long phys_size_t;

#define __user
#define __iomem

#define ndelay(x)	udelay(1)
#define printk	printf

#define KERN_EMERG
#define KERN_ALERT
#define KERN_CRIT
#define KERN_ERR
#define KERN_WARNING
#define KERN_NOTICE
#define KERN_INFO
#define KERN_DEBUG
#define GFP_KERNEL  0

#define kmalloc(size, flags)	malloc(size)
//#define kzalloc(size, flags)	calloc(size, 1)
//#define kzalloc(size, flags)	malloc(size)
#define vmalloc(size)		malloc(size)
#define kfree(ptr)		free(ptr)
#define vfree(ptr)		free(ptr)
#define kstrdup(str, flags)     strdup(str)

#define DECLARE_WAITQUEUE(...)	do { } while (0)
#define add_wait_queue(...)	do { } while (0)
#define remove_wait_queue(...)	do { } while (0)

#define KERNEL_VERSION(a,b,c)	(((a) << 16) + ((b) << 8) + (c))

#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

/*
 * General Purpose Utilities
 */
#define min(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
	 (__x < __y) ? __x : __y; })

#define max(X, Y)				\
	({ typeof (X) __x = (X), __y = (Y);	\
	 (__x > __y) ? __x : __y; })

#define min3(x, y, z) min((typeof(x))min(x, y), z)
#define max3(x, y, z) max((typeof(x))max(x, y), z)

#define MIN(x, y)  min(x, y)
#define MAX(x, y)  max(x, y)

#define TRUE8   0x5A
#define FALSE8  0xA5

#ifndef BUG
#define BUG() do { \
	printf("U-Boot BUG at %s:%d!\n", __FILE__, __LINE__); \
} while (0)

#define BUG_ON(condition) do { if (condition) BUG(); } while(0)
#endif /* BUG */

#ifdef	DEBUG
#define debug(fmt,args...)	printf (fmt ,##args)
#define debugX(level,fmt,args...) if (DEBUG>=level) printf(fmt,##args);
#else
#define debug(fmt,args...)
#define debugX(level,fmt,args...)
#endif	/* DEBUG */

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x)	__builtin_expect(!!(x), 0)

#define PAGE_SIZE	4096

#define __cpu_to_le16(x) ((__u16)(x))
#define cpu_to_le16 __cpu_to_le16

#endif

#endif

