#ifndef __SUB_OUT_H__
#define __SUB_OUT_H__

#include "gx_avout.h"
#include "gx_subtitle.h"

extern GxSubOutClass gx_SubOutBase;

#define GetGxSubOutClassFromObject(s) ((GxSubOutClass*)(GetGxClassFromObject(s)))
#define GXSOUT(s)                       ((GxSubOut*)(s))
#define GXSOUTCLASS(s)                  ((GxSubOutClass *)(s))

#endif
