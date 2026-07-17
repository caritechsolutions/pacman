#ifndef __TIME_INTERFACE_H__
#define __TIME_INTERFACE_H__

#include <types.h>

struct gx_time_ops {
    u32 (*init)(void);
    u64 (*get_us)(void);
};

#endif
