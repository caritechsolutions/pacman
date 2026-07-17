#include "gxcore.h"
#include "gx_txt_subtitle.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_config.h"

int GxTxtSubtitle_Init(GxTxtSubtitleWindow *rect, GxTxtSubtitleRender render)
{
	handle_t handle;
	GxSubtitleShowCanvas cv;
	GxSubtitleShowScreen sr;
	unsigned int color;
	GxSubtitleShowFont    font;

	memcpy(&cv, &subtitleConfig.canvas, sizeof(GxSubtitleShowCanvas));
	memcpy(&sr, &subtitleConfig.screen, sizeof(GxSubtitleShowScreen));
	handle = GxSubtitleShowTextOpen(&cv, &sr);

	color = subtitleConfig.bgColor.user ?
		subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
	GxSubtitleShowTextSetScreenColor(handle, color);

	font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
	font.str   = subtitleConfig.ftLib.str;
	GxSubtitleShowTextSetFont(handle, &font);

	return handle;
}

void GxTxtSubtitle_Destory(int handle)
{
	return GxSubtitleShowTextClose(handle);
}

int GxTxtSubtitle_Show(int handle)
{
	return GxSubtitleShowTextShow();
}

int GxTxtSubtitle_Hide(int handle)
{
	return GxSubtitleShowTextHide();
}

int GxTxtSubtitle_Draw(int handle, GxTxtSubtitleContent *content, int track, GxTxtSubtitleEncode encode)
{
	unsigned int i = 0;
	GxSubtitleShowTextPage txt;
	GxSubtitleShowDisplay pos;
	unsigned int color;
	GxSubtitleShowFont    font;

	if (!subtitleConfig.disPos.user) {
		pos.xl = subtitleConfig.defDisPos.xl;
		pos.xr = subtitleConfig.defDisPos.xr;
		pos.yt = subtitleConfig.defDisPos.yt;
		pos.yb = subtitleConfig.defDisPos.yb;
		pos.autoPos = 1;
	} else {
		pos.xl = subtitleConfig.disPos.xl;
		pos.xr = subtitleConfig.disPos.xr;
		pos.yt = subtitleConfig.disPos.yt;
		pos.yb = subtitleConfig.disPos.yb;
		pos.autoPos = 0;
	}
	GxSubtitleShowTextSetDisplayPos(handle, &pos);

	color = subtitleConfig.bgColor.user ?
		subtitleConfig.bgColor.color : subtitleConfig.defBGColor;
	GxSubtitleShowTextSetScreenColor(handle, color);

	font.color = subtitleConfig.ftColor.user ? subtitleConfig.ftColor.color : subtitleConfig.defFTColor;
	font.str   = subtitleConfig.ftLib.str;
	GxSubtitleShowTextSetFont(handle, &font);

	txt.attr = 1;
	txt.lines.lines   = content->lines;
	for (i = 0; i < txt.lines.lines; i++)
		txt.lines.text[i] = content->text[i];

	return GxSubtitleShowTextDraw(handle, &txt);
}

void GxTxtSubtitle_Clean(int handle)
{
	GxSubtitleShowTextClean(handle);
}

