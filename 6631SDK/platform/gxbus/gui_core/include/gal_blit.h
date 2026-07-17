#ifndef __GAL_BLIT_H__
#define __GAL_BLIT_H__

#ifndef NO_OS
#include "IMG.h"
#include "hd_gal.h"
#include "gxcore.h"
#else /*NO_OS*/
#endif /*NO_OS*/
#include "gal.h"

__BEGIN_DECLS

void *gal_get_surface(GAL_Rect *rect, int bpp);

int gal_copy_surface(void *src, GAL_Rect *src_rect, void *dst, GAL_Rect *dst_rect);

int gal_mirror_image_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxReverseMode mode);

void gal_free_surface(void *surface);

__END_DECLS

#endif  /*__GAL_BLIT_H__*/


