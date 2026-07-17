#include "gxtype.h"
#include <gxcore.h>
#include "gui_core.h"
#include "gx_mem.h"
#include "module/subtitle/gx_subtitle_setting.h"
#include "gx_subtitle_show_pixel.h"
#include "gx_gdi_layer_spp.h"

//#define DEBUG_OSD_SHOW_PIXEL

typedef struct {
	unsigned int showing;
	int                 pixelWidth;   //total pixel width
	int                 pixelHeight;  //total pixel height
	GuiSurface          *ycbcr6442Surface;
	GuiSurface          *argb8888Surface;
	GXGDI_Rect          drawRect;
	GuiSurface          *srcSurface;
	GAL_Rect            srcRect;
	GxSubtitleShowDisplay pos;
	unsigned int          screenColor;
	GxSubtitleShowCanvas  cv;
} GxSubtitleShowPixel;

static unsigned int showPixelCount = 0;
extern GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);
extern int         hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);

static int show_pixel_create_argb(GxSubtitleShowPixel *showPixel, GAL_Rect *dstRect)
{
	if ((showPixel->pixelWidth != dstRect->w) || (showPixel->pixelHeight != dstRect->h)) {
		if (showPixel->argb8888Surface) {
			hd_free_surface(showPixel->argb8888Surface);
			showPixel->argb8888Surface = NULL;
		}

		GXGDI_Begin();
		showPixel->argb8888Surface = hd_get_surface0(dstRect, 32, GX_COLOR_FMT_ARGB8888, NULL, TRUE);
		GXGDI_End();
		if (NULL == showPixel->argb8888Surface) {
			gxlogd("%s %d, Show Pixel Create Surface Failed\n", __FUNCTION__, __LINE__);
			return -1;
		}

		showPixel->pixelWidth  = dstRect->w;
		showPixel->pixelHeight = dstRect->h;
	}

	return 0;
}

static int show_pixel_destory_argb(GxSubtitleShowPixel *showPixel)
{
	if (showPixel->argb8888Surface){
		hd_free_surface(showPixel->argb8888Surface);
		showPixel->argb8888Surface = NULL;
	}

	return 0;
}

#ifdef DEBUG_OSD_SHOW_PIXEL
static void show_pixel_draw_surface_0(GxSubtitleShowPixel *showPixel, GuiSurface *srcSurface, GAL_Rect *srcRect)
{
	unsigned int xl_offset = showPixel->pos.xl;
	unsigned int xr_offset = showPixel->pos.xr;
	unsigned int yt_offset = showPixel->pos.yt;
	unsigned int yb_offset = showPixel->pos.yb;
	GXGDI_Rect dstRect;

	dstRect.x = xl_offset;
	dstRect.y = yt_offset;
	dstRect.w = showPixel->cv.w - (xl_offset + xr_offset);
	dstRect.h = showPixel->cv.h - (yt_offset + yb_offset);

	GXGDI_Begin();
	extern GuiSurface *screen;
	if ((srcRect->w == dstRect.w) && (srcRect->h == dstRect.h))
		GXGDI_Blit(srcSurface, (GXGDI_Rect *)&srcRect, screen, (GXGDI_Rect *)&dstRect, GDI_BLIT_COPY);
	else
		hd_zoom_surface(srcSurface, srcRect, screen, (GAL_Rect *)&dstRect);
	GXGDI_End();

	return;
}
#else
static void show_pixel_draw_surface_0(GxSubtitleShowPixel *showPixel, GuiSurface *srcSurface, GAL_Rect *srcRect)
{
	unsigned int xl_offset = showPixel->pos.xl;
	unsigned int xr_offset = showPixel->pos.xr;
	unsigned int yt_offset = showPixel->pos.yt;
	unsigned int yb_offset = showPixel->pos.yb;
	GAL_Rect   surfaceRect;
	GXGDI_Rect dstRect;

	surfaceRect.x = surfaceRect.y = 0;
	surfaceRect.w = showPixel->cv.w;
	surfaceRect.h = showPixel->cv.h;

	if ((xl_offset + xr_offset) > surfaceRect.w) {
		gxlogd("warning: %s %d, (xl_offset:%d xr_offset:%d win_w:%d)\n",
				__FUNCTION__, __LINE__, xl_offset, xr_offset, surfaceRect.w);
		return;
	}

	if ((yt_offset + yb_offset) > surfaceRect.h) {
		gxlogd("warning: %s %d, (xl_offset:%d xr_offset:%d win_w:%d)\n",
				__FUNCTION__, __LINE__, yt_offset, yb_offset, surfaceRect.h);
		return;
	}
	dstRect.x = xl_offset;
	dstRect.y = yt_offset;
	dstRect.w = surfaceRect.w - (xl_offset + xr_offset);
	dstRect.h = surfaceRect.h - (yt_offset + yb_offset);

	if (0 != show_pixel_create_argb(showPixel, &surfaceRect)) {
		gxlogd("warning: %s %d, surface create error\n", __FUNCTION__, __LINE__);
		return;
	}

	GXGDI_Begin();
	GXGDI_FillRect(showPixel->argb8888Surface, (GXGDI_Rect *)&surfaceRect, showPixel->screenColor);
	GXGDI_End();
	GXGDI_Begin();
	if ((srcRect->w == dstRect.w) && (srcRect->h == dstRect.h)) {
		srcSurface->alpha = 0;
		GXGDI_Blit(srcSurface, (GXGDI_Rect *)srcRect, showPixel->argb8888Surface, &dstRect, GDI_BLIT_COPY);
	} else
		hd_zoom_surface(srcSurface, srcRect, showPixel->argb8888Surface, (GAL_Rect *)&dstRect);
	GXGDI_End();

	showPixel->drawRect.x = surfaceRect.x;
	showPixel->drawRect.y = surfaceRect.y;
	showPixel->drawRect.w = surfaceRect.w;
	showPixel->drawRect.h = surfaceRect.h;
	GXGDI_LayerSppDraw(showPixel->argb8888Surface, (GXGDI_Rect *)&surfaceRect, (GXGDI_Rect *)&surfaceRect);

	return;
}

static void show_pixel_draw_surface_1(GxSubtitleShowPixel *showPixel, GuiSurface *srcSurface, GAL_Rect *srcRect)
{
	unsigned int xl_offset = showPixel->pos.xl;
	unsigned int xr_offset = showPixel->pos.xr;
	unsigned int yt_offset = showPixel->pos.yt;
	unsigned int yb_offset = showPixel->pos.yb;
	GAL_Rect dstRect;

	if ((xl_offset + xr_offset) > showPixel->cv.w) {
		gxloge("xl_offset:%d xr_offset:%d win_w:%d\n", xl_offset, xr_offset, showPixel->cv.w);
		return;
	}

	if ((yt_offset + yb_offset) > showPixel->cv.h) {
		gxloge("xl_offset:%d xr_offset:%d win_w:%d\n", yt_offset, yb_offset, showPixel->cv.h);
		return;
	}

	dstRect.x = dstRect.y = 0;
	dstRect.w = showPixel->cv.w - (xl_offset + xr_offset);
	dstRect.h = showPixel->cv.h - (yt_offset + yb_offset);
	if (0 != show_pixel_create_argb(showPixel, &dstRect)) {
		gxlogd("warning: %s %d, surface create error\n", __FUNCTION__, __LINE__);
		return;
	}

	GXGDI_Begin();
	if ((srcRect->w == dstRect.w) && (srcRect->h == dstRect.h)) {
		srcSurface->alpha = 0;
		GXGDI_Blit(srcSurface, (GXGDI_Rect *)srcRect,
				showPixel->argb8888Surface, (GXGDI_Rect *)&dstRect, GDI_BLIT_COPY);
	} else
		hd_zoom_surface(srcSurface, srcRect, showPixel->argb8888Surface, (GAL_Rect *)&dstRect);
	GXGDI_End();

	showPixel->drawRect.x = xl_offset;
	showPixel->drawRect.y = yt_offset;
	showPixel->drawRect.w = dstRect.w;
	showPixel->drawRect.h = dstRect.h;
	GXGDI_LayerSppDraw(showPixel->argb8888Surface, (GXGDI_Rect *)&dstRect, &showPixel->drawRect);

	return;
}
#endif

static void show_pixel_clean(GxSubtitleShowPixel *showPixel)
{
	GXGDI_LayerSppClean(&showPixel->drawRect);
	return;
}

static handle_t GXGDI_ShowPixelOpen(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	GxSubtitleShowPixel *showPixel = NULL;

	if (showPixelCount == 0) {
		if (GXGDI_LayerSppCreate(cv, sr) < 0) {
			gxloge("%s %d, Show SPP Create Failed\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}

	showPixel = (GxSubtitleShowPixel *)av_mallocz(sizeof(GxSubtitleShowPixel));
	if (NULL == showPixel) {
		gxloge("%s %d, Show Pixel Create Failed\n", __FUNCTION__, __LINE__);
		return 0;
	}

	showPixelCount++;
	memcpy(&showPixel->cv, cv, sizeof(GxSubtitleShowCanvas));

	return (handle_t)showPixel;
}

static void GXGDI_ShowPixelClose(handle_t handle)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return;
	}
	if (showPixelCount <= 0) {
		gxlogi("%s %d, showPixelCount is NULL\n", __FUNCTION__, __LINE__);
		return;
	}

	if (showPixel->showing)
		show_pixel_clean(showPixel);

	show_pixel_destory_argb (showPixel);

	av_free((void*)showPixel);
	showPixelCount--;

	if (showPixelCount == 0)
		GXGDI_LayerSppDestory();

	return;
}

static int GXGDI_ShowPixelSetDisplayPos(handle_t handle, GxSubtitleShowDisplay *pos)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	showPixel->pos    = *pos;

	return 0;
}

static int GXGDI_ShowPixelSetScreenColor(handle_t handle, unsigned int color)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	showPixel->screenColor = color;

	return 0;
}

static unsigned int *GXGDI_ShowPixelAllocARGB8888Buffer(handle_t handle, unsigned int width, unsigned int height)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return NULL;
	}

	showPixel->srcRect.x = 0;
	showPixel->srcRect.y = 0;
	showPixel->srcRect.w = width;
	showPixel->srcRect.h = height;
	showPixel->srcSurface = hd_get_surface0(&showPixel->srcRect, 32, GX_COLOR_FMT_ARGB8888, NULL, TRUE);
	if (!showPixel->srcSurface) {
		gxlogd("%s %d, surface create failed(%d %d)\n",
				__FUNCTION__, __LINE__, width, height);
		return NULL;
	}
	GXGDI_Begin();
	GXGDI_FillRect(showPixel->srcSurface, (GXGDI_Rect *)&showPixel->srcRect, 0x00000000);
	GXGDI_End();

	return (unsigned int *)(showPixel->srcSurface->data);
}

static int GXGDI_ShowPixelFreeARGB8888Buffer(handle_t handle, unsigned int *buffer)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	hd_free_surface(showPixel->srcSurface);

	showPixel->srcSurface = NULL;

	return 0;
}

static int GXGDI_ShowPixelDrawARGB8888Buffer(handle_t handle, unsigned int *buffer, int full_screen)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (!showPixel) {
		gxlogd("%s %d, Show Pixel Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GXGDI_Lock();
#ifdef DEBUG_OSD_SHOW_PIXEL
	show_pixel_draw_surface_0(showPixel, showPixel->srcSurface, &showPixel->srcRect);
#else
	if (full_screen)
		show_pixel_draw_surface_0(showPixel, showPixel->srcSurface, &showPixel->srcRect);
	else
		show_pixel_draw_surface_1(showPixel, showPixel->srcSurface, &showPixel->srcRect);
#endif

	GXGDI_Unlock();

	showPixel->showing = 1;

	return 0;
}

static int GXGDI_ShowPixelClean(handle_t handle)
{
	GxSubtitleShowPixel *showPixel = (GxSubtitleShowPixel *)handle;

	if (showPixel->showing)
		show_pixel_clean(showPixel);

	showPixel->showing = 0;

	return 0;
}

static int GXGDI_ShowPixelShow(void)
{
	if (showPixelCount)
		GXGDI_LayerSppShow();

	return 0;
}

static int GXGDI_ShowPixelHide(void)
{
	if (showPixelCount)
		GXGDI_LayerSppHide();

	return 0;
}

static int GXGDI_ShowPixelSetScreen(GxSubtitleShowScreen *screen)
{
	if (showPixelCount) {
		GXGDI_Rect rect;

		rect.x = screen->x;
		rect.y = screen->y;
		rect.w = screen->w;
		rect.h = screen->h;
		return GXGDI_LayerSppScreen(&rect);
	}

	return 0;
}

GxSubtitleShowPixelOps gdiPixelOps = {
	.name     = "GXGDI Show Pixel",
	.open     = GXGDI_ShowPixelOpen,
	.close    = GXGDI_ShowPixelClose,
	.draw     = NULL,
	.clean    = GXGDI_ShowPixelClean,
	.show     = GXGDI_ShowPixelShow,
	.hide     = GXGDI_ShowPixelHide,
	.setDisplayPos  = GXGDI_ShowPixelSetDisplayPos,
	.setScreenColor = GXGDI_ShowPixelSetScreenColor,
	.setScreen      = GXGDI_ShowPixelSetScreen,
	.allocARGB8888Buffer = GXGDI_ShowPixelAllocARGB8888Buffer,
	.freeARGB8888Buffer  = GXGDI_ShowPixelFreeARGB8888Buffer,
	.drawARGB8888Buffer  = GXGDI_ShowPixelDrawARGB8888Buffer,
};
