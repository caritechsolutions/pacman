#include "gxtype.h"
#include <gxcore.h>
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_show_pixel.h"
#include "gx_avflow.h"

void GxSubtitle_ModuleRegisterShowText(GxSubtitleShowTextOps *ops)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxSubtitleShowTextRegister(ops);
}

void GxSubtitle_ModuleUnRegisterShowText(void)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxSubtitleShowTextUnRegister();
}

void GxSubtitle_ModuleRegisterShowPixel(GxSubtitleShowPixelOps *ops)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxSubtitleShowPixelRegister(ops);
}

void GxSubtitle_ModuleUnRegisterShowPixel(void)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	GxSubtitleShowPixelUnRegister();
}
