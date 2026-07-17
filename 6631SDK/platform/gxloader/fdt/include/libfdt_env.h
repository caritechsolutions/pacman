#ifndef _LIBFDT_ENV_H
#define _LIBFDT_ENV_H

//#include "linux/compiler.h"
//#include "linux/types.h"
#include "sys/types.h"
#include "string.h"
#include "util.h"

#define strtoul simple_strtoull

extern struct fdt_header *working_fdt;  /* Pointer to the working fdt */

typedef __u16 fdt16_t;
typedef __u32 fdt32_t;
typedef __u64 fdt64_t;

#define uswap_32(x) \
	((((x) & 0xff000000) >> 24) | \
	(((x) & 0x00ff0000) >>  8) | \
	(((x) & 0x0000ff00) <<  8) | \
	(((x) & 0x000000ff) << 24))
#define uswap_64(x) \
	((((x) & 0xff00000000000000) >> 56) | \
	(((x) & 0x00ff000000000000) >> 40) | \
	(((x) & 0x0000ff0000000000) >> 24) | \
	(((x) & 0x000000ff00000000) >>  8) | \
	(((x) & 0x00000000ff000000) <<  8) | \
	(((x) & 0x0000000000ff0000) << 24) | \
	(((x) & 0x000000000000ff00) << 40) | \
	(((x) & 0x00000000000000ff) << 56))


#ifdef CONFIG_ENABLE_UBI
#include <linux/byteorder/generic.h>
#else
#define be32_to_cpu(x)    uswap_32(x)
#define cpu_to_be32(x)    uswap_32(x)
#define be64_to_cpu(x)    uswap_64(x)
#define cpu_to_be64(x)    uswap_64(x)
#endif


#define fdt32_to_cpu(x)		be32_to_cpu(x)
#define cpu_to_fdt32(x)		cpu_to_be32(x)
#define fdt64_to_cpu(x)		be64_to_cpu(x)
#define cpu_to_fdt64(x)		cpu_to_be64(x)

#if 0
#ifdef __UBOOT__
#include <vsprintf.h>

#define strtoul(cp, endp, base)	simple_strtoul(cp, endp, base)
#endif

#endif

/* adding a ramdisk needs 0x44 bytes in version 2008.10 */
#define FDT_RAMDISK_OVERHEAD	0x80

#endif /* _LIBFDT_ENV_H */
