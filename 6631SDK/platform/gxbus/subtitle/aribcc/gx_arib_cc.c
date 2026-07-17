/*
 * @file        gxaribcc.c
 * @brief
 * @version     1.1.0
 * @date        11/04/2016 08:25:26 AM
 * @author
 *
 */
#include "gxcore.h"
#include "gx_mem.h"
#include "gx_arib_cc.h"
#include "gx_subtitle_module.h"
#include "arib_cc_dec.h"
#include "gx_txt_subtitle.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_config.h"


typedef struct {
	void                 *arib;
	GxSubtitleShowCanvas  cv;
	GxSubtitleShowScreen  sr;
	unsigned int          color;
	GxSubtitleShowFont    font;
	handle_t              showHandle;
} GxAribccContext;

static GxAribccContext *ctx = NULL;

static int arib_cc_text_handler(void *priv, GxSubtitleShowTextPage *page)
{
	GxSubtitleShowDisplay pos;

	if (!page->lines.lines) {
		GxSubtitleShowTextClean(ctx->showHandle);
		return 0;
	}

	ctx->color = subtitleConfig.bgColor.user ?
		subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
	GxSubtitleShowTextSetScreenColor(ctx->showHandle, ctx->color);

	ctx->font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
	ctx->font.str   = subtitleConfig.ftLib.str;
	GxSubtitleShowTextSetFont(ctx->showHandle, &ctx->font);

	pos.xl = subtitleConfig.canvas.width / 4;
	pos.xr = 0;
	pos.yt = subtitleConfig.canvas.height * 5 / 6;
	pos.yb = 0;
	pos.autoPos = 0;
	GxSubtitleShowTextSetDisplayPos(ctx->showHandle, &pos);
	page->attr = 1;
	return GxSubtitleShowTextDraw(ctx->showHandle, page);
}

handle_t GxAribcc_Open(GxAribcc_Param* param)
{
	ctx = (GxAribccContext *)av_mallocz(sizeof(GxAribccContext));
	if (!ctx) return 0;

	memcpy(&ctx->cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
	memcpy(&ctx->sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
	ctx->showHandle = GxSubtitleShowTextOpen(&ctx->cv, &ctx->sr);

	ctx->color = subtitleConfig.bgColor.user ?
		subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
	GxSubtitleShowTextSetScreenColor(ctx->showHandle, ctx->color);

	ctx->font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
	ctx->font.str   = subtitleConfig.ftLib.str;
	GxSubtitleShowTextSetFont(ctx->showHandle, &ctx->font);

	ctx->arib = AribccCreate();

	return (handle_t)ctx;
}

int32_t GxAribcc_Synchronize(handle_t handle, int32_t milliseconds)
{
	return 0;
}


int32_t GxAribcc_StreamSet(handle_t handle, GxAribcc_Stream* stream)
{
	return 0;
}


int32_t GxAribcc_Close(handle_t handle)
{
	if (!ctx) return 0;

	AribccDestory(ctx->arib);

	GxSubtitleShowTextClose(ctx->showHandle);

	return 0;
}


int32_t GxAribcc_Start(handle_t handle)
{
	if (!ctx) return 0;

	AribccRegisterEvent(ctx->arib, (void *)ctx, arib_cc_text_handler);
	return 0;
}


int32_t GxAribcc_Stop(handle_t handle, int freeze)
{
	return 0;
}


int32_t GxAribcc_Clear(handle_t handle)
{
	GxSubtitleShowTextClean(ctx->showHandle);
	return 0;
}


int32_t GxAribcc_Show(handle_t handle)
{
	return GxSubtitleShowTextShow();
}


int32_t GxAribcc_SetNtsc(handle_t handle, bool flag)
{

	return -1;
}


int32_t GxAribcc_Hide(handle_t handle)
{
	return GxSubtitleShowTextHide();
}


int32_t GxAribcc_SendESData(handle_t handle, uint8_t* packet, int packet_length)
{
	AribccDecode(ctx->arib, packet, packet_length);

	return 0;
}

