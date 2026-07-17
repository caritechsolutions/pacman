#ifndef __HD_GAL_H__
#define __HD_GAL_H__

#ifndef NO_OS
#include "gui_private.h"
#include "gxcore.h"
#include "IMG.h"
#include "gui_core.h"
#include "gal.h"

__BEGIN_DECLS

int hd_gxavdev_init(void);
int hd_init(void);
int hd_flip(void);
int hd_quit(void);

GxHwMallocObj *hd_malloc(GxHwMallocObj **p, size_t size);
GxHwMallocObj *hd_malloc_nocache(GxHwMallocObj **p, size_t size);
GxHwMallocObj *hd_malloc_main(GxHwMallocObj **p, size_t size, int flag);
void hd_free(GxHwMallocObj *p);
void hd_check(GxHwMallocObj *p);


void hd_color2formatcolor(GuiSurface *surface, int color, GxColor *value);
void hd_putpixel       (GuiSurface *surface, int x, int y, unsigned int pixel);
int  hd_image_rect     (GuiSurface *surface, GAL_Rect * dstrect, unsigned int color);
int  hd_img_copy       (GuiSurface *surface, char *buffer, int x, int y, int x_size, int y_size, int component);
int  hd_set_clut       (GuiSurface *surface, Color * colors, int firstcolor, int ncolors);
int  hd_set_alpha      (GuiSurface *surface, int alpha);
int  hd_jpeg_decode    (GuiSurface *surface, const char *pJPEG, int x0, int y0, int screen_width, int screen_height);
int hd_video_size(GxLayerID layer, int *width, int *height);
int hd_capture(GxLayerID layer, GuiSurface *surface, GAL_Rect *rect);

int hd_img_clear(void);
int hd_enable_img(int flag);

int hd_enable_video(int flag);
int hd_set_video_size(int x, int y, int x_size, int y_size);
int hd_set_image_size(int x, int y, int x_size, int y_size);
int hd_set_interface_size(int x, int y, int x_size, int y_size);
int hd_resolution(GAL_Rect *rect);

int hd_jpeg_stop(void);

GuiSurface *jpeg_surface(const char *path, int original_width, int original_height, int width, int height);
image_desc *jpeg_image_descriptor(const char *path,
	                          image_desc *img_desc,
	                          int original_width,
	                          int original_height,
	                          int width,
	                          int height);
int hd_img_desc_gradiant(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h, ImagePlayMode mode);

int hd_new_blit_list(void);
int hd_add_blit_element(GuiSurface *surface);
int hd_end_blit_list(void);
int hd_commit_blit(void);
int hd_change_surface(void **src0, void **src1);

void hd_free_layer_surface(LayerSurface layer);
void hd_check_layer(void);

int hd_lock(void);
int hd_unlock(void);


#else
#include "gxtype.h"
#include "gal.h"
#include "IMG.h"
__BEGIN_DECLS

#endif /*NO_OS*/
GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);
GuiSurface *hd_get_surface(GAL_Rect *rect, int bpp, void *buffer, int force);
GuiSurface *hd_get_idle_surface(GuiSurface *surface);
int  hd_char           (GuiSurface *surface, char *pData, int x, int y, int xsize, int ysize, int byteperline, int color);
int  hd_fillrect       (GuiSurface *surface, GAL_Rect * dstrect, unsigned int color);
void hd_free_surface   (GuiSurface *surface);
int  hd_color2index    (GuiSurface *surface, int color);
int  hd_color2index0   (GuiSurface *surface, int color);
int  hd_read_surface   (GuiSurface *surface, unsigned int *width, unsigned int *height, void **buffer);
int  hd_stretch_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int  hd_stretch_surface1(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int  hd_copy_surface   (GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int  hd_copy_surface1   (GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int  hd_tile_surface   (GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int hd_copy_surface_gradiant(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, ImagePlayMode mode, unsigned int steps);

int hd_sync(void);

image_desc *hd_jpeg_load(image_desc *jpg_desc, const char *pFile);

int  hd_img_desc       (GuiSurface *surface, const image_desc * pDesc, int x, int y, int w, int h);
int hd_img_desc_mirror(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h, GxReverseMode mode);

void hd_color2formatcolor(GuiSurface *surface, int color, GxColor *value);
void hd_set_gui_transparent(unsigned int color);
int hd_get_gui_transparent(void);
int hd_set_osd_transparent(unsigned int color, unsigned short opacity);
void hd_image_pixel    (GuiSurface *surface, int x, int y, unsigned int pixel);

int hd_new_blit_list(void);
int hd_add_blit_element(GuiSurface *surface);
int hd_end_blit_list(void);
int hd_commit_blit(void);
void hd_free(GxHwMallocObj *p);
int hd_update_blit(void);
int hd_osd_enable(int flag);
int hd_enable_osd(int flag);
GuiSurface *image_desc_surface(const image_desc *desc);
int hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
int hd_mirror_image_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxReverseMode mode);
static inline int is_ksurface(GuiSurface *surface) {
	return (surface->hw & KSURFACE) ? 1 : 0;
}

static inline int get_color_format(int format, int bpp) {
	switch(bpp) {
		case 0:
			return GX_COLOR_FMT_YCBCR422;
		case 1:
			return GX_COLOR_FMT_CLUT1;
		case 2:
			return GX_COLOR_FMT_YCBCRA6442;
		case 3:
			return GX_COLOR_FMT_Y8;
		case 4:
			return GX_COLOR_FMT_YCBCR420;
		case 8:
			return GX_COLOR_FMT_CLUT8;
		case 16:
			return GX_COLOR_FMT_RGB565;
		case 24:
			return GX_COLOR_FMT_RGB888;
		case 32:
			return GX_COLOR_FMT_ARGB8888;
		default:
			break;
	}
	return format;
}
__END_DECLS

#endif

