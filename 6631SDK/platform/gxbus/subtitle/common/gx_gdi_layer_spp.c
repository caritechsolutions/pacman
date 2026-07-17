#include "gxtype.h"
#include <gxcore.h>
#include "gui_core.h"
#include "gxavdev.h"
#include "gx_mem.h"
#include "module/subtitle/gx_subtitle_setting.h"
#include "gx_subtitle_module.h"
#include "gx_gdi_layer_spp.h"

#define SPP_COLOR_BPP     (16) //16bits
#define SPP_COLOR_FORMAT  GX_COLOR_FMT_YCBCRA6442

typedef struct {
	void       *rawSurfaceBuffer;
	GxAvRect   rawViewRect;
	int        rawOnTop;
	int        rawEnable;
	GuiSurface *newSurface;
	int        device;
	int        module;
	GxSubtitleShowCanvas cv;
	GxSubtitleShowScreen sr;
} GXGDI_LayerSpp;

GXGDI_LayerSpp *spp = NULL;
extern GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);
extern int         hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);

static void layer_spp_remain_surface(void)
{
	spp->rawSurfaceBuffer = GxAvdev_GetLayerMainSurface(spp->device, spp->module, GX_LAYER_SPP);
	GxAvdev_GetLayerViewport(spp->device, spp->module, GX_LAYER_SPP, &spp->rawViewRect);
	GxAvdev_GetLayerTop     (spp->device, spp->module, GX_LAYER_SPP, &spp->rawOnTop);
	spp->rawEnable = GxAvdev_GetLayerEnable(spp->device, spp->module, GX_LAYER_SPP);
}

static void layer_spp_recovery_surface(void)
{
	GxAvdev_SetLayerMainSurface(spp->device, spp->module, GX_LAYER_SPP, spp->rawSurfaceBuffer);
	GxAvdev_SetLayerTop     (spp->device, spp->module, GX_LAYER_SPP, spp->rawOnTop);
	GxAvdev_SetLayerViewport(spp->device, spp->module, GX_LAYER_SPP, &spp->rawViewRect);
	GxAvdev_LayerEnable     (spp->device, spp->module, GX_LAYER_SPP, spp->rawEnable);
}

int GXGDI_LayerSppCreate(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	int ret = 0;
	GxAvRect   viewRect ={0};
	GAL_Rect   gdiRect  = {0};
	GXGDI_Rect srcRect  = {0};

	if (spp) {
		gxlogd("%s %d, Spp Layer Exist\n", __FUNCTION__, __LINE__);
		return -1;
	}

	spp = (GXGDI_LayerSpp *)av_mallocz(sizeof(GXGDI_LayerSpp));
	if (!spp) {
		gxloge("%s %d, Spp Layer Malloc Failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	memcpy(&spp->cv, cv, sizeof(GxSubtitleShowCanvas));
	memcpy(&spp->sr, sr, sizeof(GxSubtitleShowScreen));

	spp->device = GxAvdev_CreateDevice(0);
	spp->module = GxAvdev_OpenModule(spp->device, GXAV_MOD_VPU, 0);

	gdiRect.x = gdiRect.y = 0;
	gdiRect.w = spp->cv.w;
	gdiRect.h = spp->cv.h;
	spp->newSurface = hd_get_surface0(&gdiRect,
			SPP_COLOR_BPP, SPP_COLOR_FORMAT, NULL, TRUE);
	if (NULL == spp->newSurface) {
		gxlogd("%s %d, Spp Surface Create Failed\n", __FUNCTION__, __LINE__);
		return -1;
	} else {
		srcRect.x = srcRect.y = 0;
		srcRect.w = spp->cv.w;
		srcRect.h = spp->cv.h;
		GXGDI_Begin();
		GXGDI_FillRect(spp->newSurface, &srcRect, 0x10808000);
		GXGDI_End();
	}

	layer_spp_remain_surface();

	ret = GxAvdev_LayerEnable(spp->device, spp->module, GX_LAYER_SPP, FALSE);
	if (ret != 0) {
		gxlogd("%s %d, Spp Disable Failed\n", __FUNCTION__, __LINE__);
		goto error_create;
	}

	ret = GxAvdev_SetLayerMainSurface(spp->device,
			spp->module, GX_LAYER_SPP, spp->newSurface->hw_surface);
	if (ret != 0) {
		gxlogd("%s %d, Spp Set Main Surface Failed\n", __FUNCTION__, __LINE__);
		goto error_create;
	}

	viewRect.x      = spp->sr.x;
	viewRect.y      = spp->sr.y;
	viewRect.width  = spp->sr.w;
	viewRect.height = spp->sr.h;
	ret = GxAvdev_SetLayerViewport(spp->device, spp->module, GX_LAYER_SPP, &viewRect);
	if (ret != 0) {
		gxlogd("%s %d, Spp Set ViewPort Failed\n", __FUNCTION__, __LINE__);
		goto error_create;
	}

	ret = GxAvdev_SetLayerTop(spp->device, spp->module, GX_LAYER_SPP, 1);
	if (ret != 0) {
		gxlogd("%s %d, Spp OnTop Failed\n", __FUNCTION__, __LINE__);
		goto error_create;
	}

	ret = GxAvdev_LayerEnable(spp->device, spp->module, GX_LAYER_SPP, TRUE);
	if (ret != 0) {
		gxlogd("%s %d, Spp Enable Failed\n", __FUNCTION__, __LINE__);
		goto error_create;
	}

	return 0;

error_create:
	layer_spp_recovery_surface();
	hd_free_surface(spp->newSurface);
	GxAvdev_CloseModule(spp->device, spp->module);
	GxAvdev_DestroyDevice(spp->device);
	av_free(spp);
	spp = NULL;

	return -1;
}

int GXGDI_LayerSppDestory(void)
{
	if (!spp) {
		gxlogd("%s %d, Spp Not Create\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GxAvdev_LayerEnable(spp->device, spp->module, GX_LAYER_SPP, FALSE);
	layer_spp_recovery_surface();
	hd_free_surface(spp->newSurface);
	GxAvdev_CloseModule(spp->device, spp->module);
	GxAvdev_DestroyDevice(spp->device);
	av_free(spp);
	spp = NULL;

	return 0;
}

int GXGDI_LayerSppDraw(GuiSurface *surface, GXGDI_Rect *srcRect, GXGDI_Rect *dstRect)
{
	if (!spp) {
		gxloge("Spp Not Create\n");
		return -1;
	}

	if ((dstRect->x + dstRect->w) > spp->cv.w) {
		gxloge("Spp Draw X More Than Canvas(%d + %d > %d)\n", dstRect->x, dstRect->w, spp->cv.w);
		return -1;
	}

	if ((dstRect->y + dstRect->h) > spp->cv.h) {
		gxloge("Spp Draw Y More Than Canvas(%d + %d > %d)\n", dstRect->y, dstRect->h, spp->cv.h);
		return -1;
	}

	GXGDI_Begin();
	if ((srcRect->w == dstRect->w) && (srcRect->h == dstRect->h)) {
		surface->alpha = 0;
		GXGDI_Blit(surface, srcRect, spp->newSurface, dstRect, GDI_BLIT_COPY);
	} else
		hd_zoom_surface(surface, (GAL_Rect *)srcRect, spp->newSurface, (GAL_Rect *)dstRect);
	GXGDI_End();

	return 0;
}

int GXGDI_LayerSppClean(GXGDI_Rect *dstRect)
{
	if (!spp) {
		gxloge("Spp Not Create\n");
		return -1;
	}

	if ((dstRect->x + dstRect->w) > spp->cv.w) {
		gxloge("Spp Draw X More Than Canvas(%d + %d > %d)\n", dstRect->x, dstRect->w, spp->cv.w);
		return -1;
	}

	if ((dstRect->y + dstRect->h) > spp->cv.h) {
		gxloge("Spp Draw Y More Than Canvas(%d + %d > %d)\n", dstRect->y, dstRect->h, spp->cv.h);
		return -1;
	}

	GXGDI_Begin();
	GXGDI_FillRect(spp->newSurface, dstRect, 0x10808000);
	GXGDI_End();

	return 0;
}

int GXGDI_LayerSppShow(void)
{
	if (!spp) {
		gxlogd("%s %d, Spp Not Create\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GxAvdev_LayerEnable(spp->device, spp->module, GX_LAYER_SPP, TRUE);

	return 0;
}

int GXGDI_LayerSppHide(void)
{
	if (!spp) {
		gxlogd("%s %d, Spp Not Create\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GxAvdev_LayerEnable(spp->device, spp->module, GX_LAYER_SPP, FALSE);

	return 0;
}

int GXGDI_LayerSppScreen(GXGDI_Rect *rect)
{
	int ret = 0;
	GxAvRect viewRect ={0};

	if (!spp) {
		gxlogd("%s %d, Spp Not Create\n", __FUNCTION__, __LINE__);
		return -1;
	}

	spp->sr.x = viewRect.x      = rect->x;
	spp->sr.y = viewRect.y      = rect->y;
	spp->sr.w = viewRect.width  = rect->w;
	spp->sr.h = viewRect.height = rect->h;;
	ret = GxAvdev_SetLayerViewport(spp->device, spp->module, GX_LAYER_SPP, &viewRect);
	if (ret != 0) {
		gxlogd("%s %d, Spp Set ViewPort Failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

int GXGDI_LayerSppGetModule(int *dev, int *module)
{
	if (!spp) {
		gxlogd("%s %d, Spp Not Create\n", __FUNCTION__, __LINE__);
		return -1;
	}

	*dev    = spp->device;
	*module = spp->module;

	return 0;
}
