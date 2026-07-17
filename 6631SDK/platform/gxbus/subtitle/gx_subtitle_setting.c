#include "gxtype.h"
#include <gxcore.h>
#include "module/subtitle/gx_subtitle_setting.h"
#include "gx_subtitle_config.h"
#include "gx_mem.h"
#include "gx_avflow.h"

#define ARGB(a, r, g, b) ((a<<24)|(r<<16)|(g<<8)|(b))

GX_SUBTITLE_CONFIG subtitleConfig = {
	.screen       = {0, 0, 1280, 720},
	.canvas       = {1280, 720},  //display window
	.limitSrcCV   = {1920, 1080}, //limit source data window
	.defBGColor   = ARGB(0xFF, 0x00, 0x00, 0x00),
	.defFTColor   = ARGB(0xFF, 0xFF, 0xFF, 0xFF),
	.defDisPos    = {GX_SUBTITLE_SPP, 0, 10, 10, 10, 30},
	.bgColor      = {0, ARGB(0x00, 0x00, 0x00, 0x00)},
	.ftColor      = {0, ARGB(0xFF, 0xFF, 0xFF, 0xFF)},
	.disPos       = {GX_SUBTITLE_SPP, 0, 10, 10, 10, 10},
	.ftLib        = {{'\0'}},
	.ttxLang      = {{"eng"}, GX_SUBTITLE_LANG_AUTO},
	.dvbClut      = {.type = GX_SUBTITLE_DVB_COMPUTE_CLUT, },
	.atscMode     = GX_SUBTITLE_ATSC_PIXEL_SHOW,
	.magAlpha     = {0, 0xff},
	.yOffset      = 0,
};

int GxSubtitle_ConfigCanvas(GX_SUBTITLE_CANVAS cv)
{
	av_flow_log("[Flow] - [%s %d]: %d %d\n",
			__func__, __LINE__, cv.width, cv.height);
	subtitleConfig.canvas.width  = cv.width  ? cv.width  : 1280;
	subtitleConfig.canvas.height = cv.height ? cv.height : 720;

	return 0;
}

int GxSubtitle_ConfigLimitSrcCV(GX_SUBTITLE_CANVAS cv)
{
	av_flow_log("[Flow] - [%s %d]: %d %d\n",
			__func__, __LINE__, cv.width, cv.height);
	subtitleConfig.limitSrcCV.width  = cv.width  ? cv.width  : 1920;
	subtitleConfig.limitSrcCV.height = cv.height ? cv.height : 1080;

	return 0;
}

int GxSubtitle_ConfigScreen(GX_SUBTITLE_SCREEN scr)
{
	av_flow_log("[Flow] - [%s %d]: %d %d %d %d\n",
			__func__, __LINE__, scr.x, scr.y, scr.width, scr.height);
	memcpy(&subtitleConfig.screen, &scr, sizeof(GX_SUBTITLE_SCREEN));

	return 0;
}

int GxSubtitle_SetDisplay(GX_SUBTITLE_DISPLAY dis)
{
	av_flow_log("[Flow] - [%s %d]: %d %d %d %d %d\n",
			__func__, __LINE__, dis.user, dis.xl, dis.xr, dis.yt, dis.yb);
	memcpy(&subtitleConfig.disPos, &dis, sizeof(GX_SUBTITLE_DISPLAY));

	return 0;
}

int GxSubtitle_SetDisplayDefaultYB(unsigned int yb)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, yb);
	subtitleConfig.defDisPos.yb = yb;
	return 0;
}

int GxSubtitle_SetDisplayYOffset(int y_offset)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, y_offset);
	subtitleConfig.yOffset = y_offset;
	return 0;
}

int GxSubtitle_SetFontLib(GX_SUBTITLE_FONT_LIB lib)
{
	av_flow_log("[Flow] - [%s %d]: %s\n", __func__, __LINE__, lib.str);
	memcpy(&subtitleConfig.ftLib, &lib, sizeof(GX_SUBTITLE_FONT_LIB));

	return 0;
}

int GxSubtitle_SetLang(GX_SUBTITLE_LANG lang)
{
	av_flow_log("[Flow] - [%s %d]: %s %d\n", __func__, __LINE__, lang.str, lang.force_flag);
	memcpy(&subtitleConfig.ttxLang, &lang, sizeof(GX_SUBTITLE_LANG));

	return 0;
}

int GxSubtitle_SetBackColor(GX_SUBTITLE_BACK_COLOR color)
{
	av_flow_log("[Flow] - [%s %d]: %d %d\n", __func__, __LINE__, color.user, color.color);
	memcpy(&subtitleConfig.bgColor, &color, sizeof(GX_SUBTITLE_BACK_COLOR));

	return 0;
}

int GxSubtitle_SetFontColor(GX_SUBTITLE_FONT_COLOR color)
{
	av_flow_log("[Flow] - [%s %d]: %d %d\n", __func__, __LINE__, color.user, color.color);
	memcpy(&subtitleConfig.ftColor, &color, sizeof(GX_SUBTITLE_FONT_COLOR));

	return 0;
}

int GxSubtitle_SetDvbSubtClut(GX_SUBTITLE_DVB_CLUT *clut)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	memcpy(&subtitleConfig.dvbClut, clut, sizeof(GX_SUBTITLE_DVB_CLUT));

	return 0;
}

int GxSubtitle_ConfigAtscccMode(GX_SUBTITLE_ATSCCC_MODE mode)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, mode);
	subtitleConfig.atscMode = mode;
	return 0;
}
