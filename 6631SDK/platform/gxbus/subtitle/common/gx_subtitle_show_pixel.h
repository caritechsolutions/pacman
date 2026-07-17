#ifndef __GX_SUBTITLE_SHOW_PIXEL_H__
#define __GX_SUBTITLE_SHOW_PIXEL_H__

#include "gx_subtitle_setting.h"
#include "gx_subtitle_module.h"

void GxSubtitleShowPixelRegister(GxSubtitleShowPixelOps *ops);
void GxSubtitleShowPixelUnRegister(void);

handle_t GxSubtitleShowPixelOpen  (GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr);
void     GxSubtitleShowPixelClose (handle_t handle);
int      GxSubtitleShowPixelDraw  (handle_t handle, GxSubtitleShowPixelPage *page);
int      GxSubtitleShowPixelClean (handle_t handle);
int      GxSubtitleShowPixelShow  (void);
int      GxSubtitleShowPixelHide  (void);

int      GxSubtitleShowPixelSetScreen     (GxSubtitleShowScreen *screen);
int      GxSubtitleShowPixelSetScreenColor(handle_t handle, unsigned int color);
int      GxSubtitleShowPixelSetDisplayPos (handle_t handle, GxSubtitleShowDisplay *pos);

unsigned int *GxSubtitleShowPixelAllocARGB888Buffer(handle_t handle, unsigned int width, unsigned int height);
int           GxSubtitleShowPixelFreeARGB8888Bufeer(handle_t handle, unsigned int *buffer);
int           GxSubtitleShowPixelDrawARGB8888Buffer(handle_t handle, unsigned int *buffer, int full_screen);
#endif
