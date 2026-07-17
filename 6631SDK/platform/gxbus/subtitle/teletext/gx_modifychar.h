#ifndef __GX_MODIFYCHAR_H__
#define __GX_MODIFYCHAR_H__

#include "gx_subtitle_magazine.h"

typedef struct {
	unsigned int           count;
	GX_SUBTITLE_VBI_MCHAR  mchar[32];
}nation_modifychar;

extern int GxMagazine_CZEModifyChar(void);

#endif
