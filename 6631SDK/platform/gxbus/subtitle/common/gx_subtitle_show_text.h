#ifndef __GX_SUBTITLE_SHOW_TEXT_H__
#define __GX_SUBTITLE_SHOW_TEXT_H__

#include "gx_subtitle_module.h"

handle_t GxSubtitleShowTextOpen  (GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr);
void     GxSubtitleShowTextClose (handle_t handle);
int      GxSubtitleShowTextDraw  (handle_t handle, GxSubtitleShowTextPage *page);
int      GxSubtitleShowTextClean (handle_t handle);
int      GxSubtitleShowTextSetScreenColor(handle_t handle, unsigned int color);
int      GxSubtitleShowTextSetDisplayPos (handle_t handle, GxSubtitleShowDisplay *pos);
int      GxSubtitleShowTextSetFont       (handle_t handle, GxSubtitleShowFont *font);

int      GxSubtitleShowTextShow  (void);
int      GxSubtitleShowTextHide  (void);
int      GxSubtitleShowTextScreen(GxSubtitleShowScreen *screen);

void     GxSubtitleShowTextRegister  (GxSubtitleShowTextOps *ops);
void     GxSubtitleShowTextUnRegister(void);

#endif
