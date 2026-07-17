#ifndef __STDDEF_H__
#define __STDDEF_H__

typedef signed ptrdiff_t;
#ifdef CONFIG_ENABLE_UBI
#include <linux/stddef.h>
#else
#define offsetof(s, m) (size_t)&(((s *)0)->m)
#endif

#endif
