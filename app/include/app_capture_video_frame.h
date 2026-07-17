#ifndef __APP_CAPTURE_VIDEO_FRAME_H__
#define __APP_CAPTURE_VIDEO_FRAME_H__
#include "gdi_core.h"

extern void app_zoom_frame(unsigned char *bgr_buffer, GAL_Rect *src, GAL_Rect *dst, char *path);
extern void app_capture_video_frame_stop(void);
extern int app_caputre_video_frame(bool async, const char *path, int width, int height);
extern void app_convert_yuv420_to_rgb(GxVpuProperty_CreateSurface *surface, unsigned char *dst_buffer, int width, int height, int is_bgr);
extern GxVpuProperty_CreateSurface *app_create_capture_surface(GXGDI_Rect *rect, GxColorFormat format, unsigned int color_depth);
extern int app_capture_video_layer(GxVpuProperty_CreateSurface *CreateSurface);
extern int app_destroy_capture_surface(GxVpuProperty_CreateSurface *CreateSurface);
extern void app_save_bgr_data_as_bmp(char *temp_path, unsigned char *bgr, int width, int height);

#endif
