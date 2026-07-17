#ifndef _GX_VO_H_
#define _GX_VO_H_

#include "gx_avout.h"


extern GxVideoOutClass gx_VideoOutBase;

#define GetGxVideoOutClassFromObject(s)     ((GxVideoOutClass*)(GetGxClassFromObject(s)))
#define GXVOUT(s)                   		((GxVideoOut*)(s))
#define GXVOUTCLASS(s)            			((GxVideoOutClass *)(s))


#endif

