#ifndef _GX_AO_H_
#define _GX_AO_H_

#include "gx_avout.h"
#include "gx_system.h"

extern GxAudioOutClass gx_AudioOutBase;

#define GetGxAudioOutClassFromObject(s) ((GxAudioOutClass*)(GetGxClassFromObject(s)))
#define GXAOUT(s)                       ((GxAudioOut*)(s))
#define GXAOUTCLASS(s)                  ((GxAudioOutClass *)(s))

#endif

