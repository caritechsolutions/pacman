#include "libzvbi/libzvbi.h"
#include "atsc_cc_dec.h"
#include "gx_subtitle_module.h"
#include "gx_subtitle_config.h"
#include "gx_subtitle_show_pixel.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_atsccc.h"
#include "gx_atsc_cc.h"
#include "gx_mem.h"

#define ATSC_CC_CHAR_WIDTH  12
#define ATSC_CC_CHAR_HEIGHT 10

#define ATSC_CC_R(rgba)   (((rgba) >> 0) & 0xFF)
#define ATSC_CC_G(rgba)   (((rgba) >> 8) & 0xFF)
#define ATSC_CC_B(rgba)   (((rgba) >> 16) & 0xFF)
#define ATSC_CC_A(rgba)   (((rgba) >> 24) & 0xFF)
#define ATSC_CC_NB_COLORS 40
#define ATSC_CC_FULL_TRANSPARENT_C  (2 * ATSC_CC_NB_COLORS + 1)
#define ATSC_CC_SEMI_TRANSPARENT_C  (2 * ATSC_CC_NB_COLORS + 2)
#define ATSC_CC_WELL_TRANSPARENT_C  (2 * ATSC_CC_NB_COLORS + 3)

typedef struct {
	handle_t                dec_handle;
	handle_t                dec_mutex;
	handle_t                show_handle;
	handle_t                show_mutex;
	GxSubtitleShowCanvas    cv;
	GX_SUBTITLE_ATSCCC_MODE mode;
	GX_SUBTITLE_ATSCCC_INFO info;
} AtscccContext;

static AtscccContext *ACtx = NULL;

static GX_SUBTITLE_ATSCCC_SERVICE atsc_cc_transfer_service(vbi_pgno pgno)
{
	switch(pgno) {
	case 1:
		return GX_SUBTITLE_ATSC_608_CC1;
	case 2:
		return GX_SUBTITLE_ATSC_608_CC2;
	case 3:
		return GX_SUBTITLE_ATSC_608_CC3;
	case 4:
		return GX_SUBTITLE_ATSC_608_CC4;
	case 9:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE1;
	case 10:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE2;
	case 11:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE3;
	case 12:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE4;
	case 13:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE5;
	case 14:
		return GX_SUBTITLE_ATSC_708_DTV_SERVICE6;
	default:
		return GX_SUBTITLE_ATSC_608_CC1;
	}
}

static void atsc_cc_add_service(AtscccContext *ctx, GX_SUBTITLE_ATSCCC_SERVICE service)
{
	unsigned int i = 0;

	if (ctx->info.num >= ATSCCC_NUM)
		return;

	for (i = 0; i < ctx->info.num; i++) {
		if (service == ctx->info.service[i])
			break;
	}

	if (i >= ctx->info.num) {
		ctx->info.service[ctx->info.num] = service;
		ctx->info.num++;
	}

	return;
}

static int atsc_cc_check_service(GX_SUBTITLE_ATSCCC_SERVICE service, GX_SUBTITLE_ATSCCC_SERVICE curService)
{
	if (curService == GX_SUBTITLE_ATSCCC_CC_AUTO)
		return 1;

	if (curService == GX_SUBTITLE_ATSCCC_CC_INVAILD)
		return 0;

	return ((service == curService) ? 1: 0);
}

static void atsc_cc_fix_transparency(AtscccContext *ctx,
		unsigned char *bitmap, vbi_page *page, int chop_top, int resx, int resy)
{
	int iy;

	// Hack for transparency, inspired by VLC code...
	for (iy = 0; iy < resy; iy++) {
		uint8_t *pixel = bitmap + iy * resx;
		vbi_char *vc = page->text + (iy / ATSC_CC_CHAR_HEIGHT + chop_top) * page->columns;
		vbi_char *vcnext = vc + page->columns;
		for (; vc < vcnext; vc++) {
			uint8_t *pixelnext = pixel + ATSC_CC_CHAR_WIDTH;
			switch (vc->opacity) {
			case VBI_TRANSPARENT_SPACE:
				memset(pixel, ATSC_CC_WELL_TRANSPARENT_C, ATSC_CC_CHAR_WIDTH);
				break;
			case VBI_OPAQUE:
				break;
			case VBI_SEMI_TRANSPARENT:
				if (subtitleConfig.bgColor.user) {
					for(; pixel < pixelnext; pixel++)
						if (*pixel == vc->background)
							*pixel = ATSC_CC_SEMI_TRANSPARENT_C;
				}
				break;
			case VBI_TRANSPARENT_FULL:
				for(; pixel < pixelnext; pixel++)
					if (*pixel == vc->background)
						*pixel = ATSC_CC_FULL_TRANSPARENT_C;
				break;
			}
			pixel = pixelnext;
		}
	}
}

static int atscc_cc_pixel_handler(vbi_page *page, void *priv, ATSC_CC_DISPLAY *disp)
{
	AtscccContext *ctx = (AtscccContext *)priv;
	int resx = (page->columns * ATSC_CC_CHAR_WIDTH);
	int resy = (page->rows * ATSC_CC_CHAR_HEIGHT);
	unsigned int i = 0, j = 0;
	vbi_char *vc    = page->text;
	vbi_char *vcend = page->text + (page->rows * page->columns);
	unsigned char *bitmap = NULL;
	unsigned int *drawBuffer = NULL;
	unsigned int bgColor  = subtitleConfig.bgColor.color;
	GxSubtitleShowDisplay pos;
	GX_SUBTITLE_ATSCCC_SERVICE service = atsc_cc_transfer_service(page->pgno);

	atsc_cc_add_service(ctx, service);
	if (!atsc_cc_check_service(service, ctx->info.cur_service))
		return 0;

	for (; vc < vcend; vc++) {
		if (vc->opacity != VBI_TRANSPARENT_SPACE)
			break;
	}

	GxCore_MutexUnlock(ctx->dec_mutex);
	GxCore_MutexLock(ctx->show_mutex);
	if (vc >= vcend) {
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		GxCore_MutexUnlock(ctx->show_mutex);
		GxCore_MutexLock(ctx->dec_mutex);
		return 0;
	}

	bitmap = (unsigned char *)av_mallocz(resx * resy);
	if (bitmap == NULL) {
		gxlogd("warning: %s %d malloc fail (resx %x, resy %x)\n",
				__FUNCTION__, __LINE__, resx, resy);
		if (ctx->show_handle)
			GxSubtitleShowPixelClean(ctx->show_handle);
		GxCore_MutexUnlock(ctx->show_mutex);
		GxCore_MutexLock(ctx->dec_mutex);
		return -1;
	}
	drawBuffer = GxSubtitleShowPixelAllocARGB888Buffer(ctx->show_handle, resx, resy);

	vbi_draw_vt_page_region(page, VBI_PIXFMT_PAL8, bitmap, resx,
			0, 0, page->columns, page->rows, 1, 1);
	atsc_cc_fix_transparency(ctx, bitmap, page, 0, resx, resy);
	for (i = 0; i < resx; i++) {
		for (j = 0; j < resy; j++) {
			int ci = bitmap[i + resx * j];
			unsigned char a = 0x00, r = 0x00, g = 0x00, b = 0x00;

			if (ci == ATSC_CC_WELL_TRANSPARENT_C) {
				r = 0x00;
				g = 0x00;
				b = 0x00;
				a = 0x00;
			} else if (ci == ATSC_CC_SEMI_TRANSPARENT_C) {
				r = (bgColor >> 16) & 0xff;
				g = (bgColor >> 8 ) & 0xff;
				b = (bgColor      ) & 0xff;
				a = (bgColor >> 24) & 0xff;
			} else {
				r = ATSC_CC_R(page->color_map[ci]);
				g = ATSC_CC_G(page->color_map[ci]);
				b = ATSC_CC_B(page->color_map[ci]);
				a = ATSC_CC_A(page->color_map[ci]);
			}

			drawBuffer[(i + resx * j)] = ((a << 24) | (r << 16) | (g << 8) | (b));
		}
	}

	pos.autoPos = 0;
	pos.xl = ctx->cv.w * disp->xl / 100;
	pos.yt = ctx->cv.h * disp->yt / 100;
	pos.xr = ctx->cv.w * disp->xr / 100;
	pos.yb = ctx->cv.h * disp->yb / 100;

	GxSubtitleShowPixelSetDisplayPos (ctx->show_handle, &pos);
	GxSubtitleShowPixelSetScreenColor(ctx->show_handle, 0);
	GxSubtitleShowPixelDrawARGB8888Buffer(ctx->show_handle, drawBuffer, 0);
	GxSubtitleShowPixelFreeARGB8888Bufeer(ctx->show_handle, drawBuffer);
	av_free(bitmap);
	GxCore_MutexUnlock(ctx->show_mutex);
	GxCore_MutexLock(ctx->dec_mutex);

	return 0;
}

static int atscc_cc_text_handler(vbi_page *page, void *priv, ATSC_CC_DISPLAY *disp)
{
	unsigned int i = 0, j = 0;
	AtscccContext *ctx = (AtscccContext *)priv;
	GX_SUBTITLE_ATSCCC_SERVICE service = atsc_cc_transfer_service(page->pgno);
	GxSubtitleShowDisplay  pos;
	GxSubtitleShowTextPage txt;

	atsc_cc_add_service(ctx, service);
	if (!atsc_cc_check_service(service, ctx->info.cur_service))
		return 0;

	GxCore_MutexUnlock(ctx->dec_mutex);
	GxCore_MutexLock(ctx->show_mutex);
	pos.autoPos = 0;
	pos.xl = ctx->cv.w * disp->xl / 100;
	pos.yt = ctx->cv.h * disp->yt / 100;
	pos.xr = ctx->cv.w * disp->xr / 100;
	pos.yb = ctx->cv.h * disp->yb / 100;

	GxSubtitleShowTextSetDisplayPos (ctx->show_handle, &pos);
	GxSubtitleShowTextSetScreenColor(ctx->show_handle, 0);

	txt.attr = 0;
	txt.chars.rows    = page->rows;
	txt.chars.columns = page->columns;
	txt.chars.text    = (GxSubtitleShowTextChar *)av_mallocz(
			(page->rows * page->columns * sizeof(GxSubtitleShowTextChar)));
	for (i = 0; i < page->rows; i++) {
		for (j = 0; j < page->columns; j++) {
			unsigned int pos = i * page->columns + j;
			txt.chars.text[pos].underline  = page->text[pos].underline;
			txt.chars.text[pos].bold       = page->text[pos].bold;
			txt.chars.text[pos].italic     = page->text[pos].italic;
			txt.chars.text[pos].conceal    = page->text[pos].conceal;
			txt.chars.text[pos].size       = page->text[pos].size;
			txt.chars.text[pos].unicode    = page->text[pos].unicode;
			txt.chars.text[pos].foreground = page->text[pos].foreground;
			txt.chars.text[pos].background = page->text[pos].background;
		}
	}
	GxSubtitleShowTextDraw(ctx->show_handle, &txt);
	av_free(txt.chars.text);
	GxCore_MutexUnlock(ctx->show_mutex);
	GxCore_MutexLock(ctx->dec_mutex);

	return 0;
}

handle_t GxAtsccc_Open(GxAtsccc_Param* param)
{
	AtscccContext *ctx = (AtscccContext *)av_mallocz(sizeof(AtscccContext));
	GxSubtitleShowCanvas cv;
	GxSubtitleShowScreen sr;

	GxCore_MutexCreate(&ctx->dec_mutex);
	GxCore_MutexCreate(&ctx->show_mutex);
	ctx->mode = subtitleConfig.atscMode;
	memcpy(&cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
	memcpy(&sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
	GxCore_MutexLock(ctx->show_mutex);
	ctx->dec_handle  = AtscccCreate();
	if (ctx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
		ctx->show_handle = GxSubtitleShowPixelOpen(&cv, &sr);
	else if (ctx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
		ctx->show_handle = GxSubtitleShowTextOpen(&cv, &sr);
	GxCore_MutexUnlock(ctx->show_mutex);
	ctx->cv = cv;
	ctx->info.cur_service = param->id;

	ACtx = ctx;
	return (handle_t)(ctx);
}

int32_t GxAtsccc_StreamSet(handle_t handle, GX_SUBTITLE_ATSCCC_SERVICE id)
{
	return 0;
}

int32_t GxAtsccc_Close(handle_t handle)
{
	AtscccContext *ctx = (AtscccContext *)handle;

	if (!ctx) return 0;

	GxCore_MutexLock(ctx->dec_mutex);
	if (ctx->dec_handle) {
		AtscccDestory(ctx->dec_handle);
		ctx->dec_handle = 0;
	}
	GxCore_MutexUnlock(ctx->dec_mutex);

	GxCore_MutexLock(ctx->show_mutex);
	if (ctx->show_handle) {
		if (ctx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
			GxSubtitleShowPixelClose(ctx->show_handle);
		else if (ctx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
			GxSubtitleShowTextClose(ctx->show_handle);
		ctx->show_handle = 0;
	}
	GxCore_MutexUnlock(ctx->show_mutex);

	GxCore_MutexDelete(ctx->dec_mutex);
	GxCore_MutexDelete(ctx->show_mutex);
	av_free(ctx);

	ACtx = NULL;
	return 0;
}

int32_t GxAtsccc_Start(handle_t handle)
{
	AtscccContext *ctx = (AtscccContext *)handle;

	if (!ctx) return 0;

	if (ctx->dec_handle) {
		if (ctx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
			AtscccRegisterEvent(ctx->dec_handle, atscc_cc_pixel_handler, (void *)(ctx));
		else if (ctx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
			AtscccRegisterEvent(ctx->dec_handle, atscc_cc_text_handler, (void *)(ctx));
	}

	return 0;
}

int32_t GxAtsccc_Stop(handle_t handle)
{
	return 0;
}

int32_t GxAtsccc_Clear(handle_t handle)
{
	AtscccContext *ctx = (AtscccContext *)handle;

	if (!ctx) return 0;

	GxCore_MutexLock(ctx->show_mutex);
	if (ctx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
		GxSubtitleShowPixelClean(ctx->show_handle);
	else if (ctx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
		GxSubtitleShowTextClean(ctx->show_handle);
	GxCore_MutexUnlock(ctx->show_mutex);
	return 0;
}

int32_t GxAtsccc_Show(handle_t handle)
{
	if (ACtx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
		GxSubtitleShowPixelShow();
	else if (ACtx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
		GxSubtitleShowTextShow();
	return 0;
}

int32_t GxAtsccc_Hide(handle_t handle)
{
	if (ACtx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
		GxSubtitleShowPixelHide();
	else if (ACtx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
		GxSubtitleShowTextHide();
	return 0;
}

int32_t GxAtsccc_SendData(handle_t handle, uint8_t* data, int length)
{
	AtscccContext *ctx = (AtscccContext *)handle;

	if (!ctx) return 0;

	GxCore_MutexLock(ctx->dec_mutex);
	if (ctx->dec_handle)
		AtscccDecode(ctx->dec_handle, data, length);
	GxCore_MutexUnlock(ctx->dec_mutex);

	return 0;
}

int32_t GxAtsccc_GetServiceInfo(GX_SUBTITLE_ATSCCC_INFO *info)
{
	if (!ACtx) return -1;

	GxCore_MutexLock(ACtx->dec_mutex);
	memcpy(info, &ACtx->info, sizeof(GX_SUBTITLE_ATSCCC_INFO));
	GxCore_MutexUnlock(ACtx->dec_mutex);

	return 0;
}

int32_t GxAtsccc_SetCurService(GX_SUBTITLE_ATSCCC_SERVICE service)
{
	if (!ACtx) return -1;

	GxCore_MutexLock(ACtx->dec_mutex);
	if (ACtx->info.cur_service != service) {
		GxCore_MutexUnlock(ACtx->dec_mutex);
		GxCore_MutexLock(ACtx->show_mutex);
		if (ACtx->mode == GX_SUBTITLE_ATSC_PIXEL_SHOW)
			GxSubtitleShowPixelClean(ACtx->show_handle);
		else if (ACtx->mode == GX_SUBTITLE_ATSC_TEXT_SHOW)
			GxSubtitleShowTextClean(ACtx->show_handle);
		GxCore_MutexUnlock(ACtx->show_mutex);
		GxCore_MutexLock(ACtx->dec_mutex);
	}
	ACtx->info.cur_service = service;
	GxCore_MutexUnlock(ACtx->dec_mutex);

	return 0;
}
