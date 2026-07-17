#include "gxtype.h"
#include <gxcore.h>
#include "gui_core.h"
#include "gx_mem.h"
#include "module/subtitle/gx_subtitle_setting.h"
#include "gx_subtitle_show_text.h"
#include "gx_gdi_layer_spp.h"

#define ARGB(a, r, g, b) ((a<<24)|(r<<16)|(g<<8)|(b))

typedef struct {
	unsigned int line;
	struct {
		unsigned int lineWidth;
		unsigned int lineHeight;
	} rect[MAX_PAGE_TEXT_LINES];
} GxSubtitleShowTextLines;

typedef struct {
	unsigned int showing;
	int                 device;
	int                 module;
	int                 fontWidth;   //total font width
	int                 fontHeight;  //total font height
	GuiSurface          *argb8888Surface;
	GXGDI_Rect          drawRect;
	unsigned int        bgColor;
	GxSubtitleShowCanvas    cv;
	GxSubtitleShowDisplay   pos;
	GxSubtitleShowFont      ft;
	GxSubtitleShowTextLines lines;
} GxSubtitleShowText;

static unsigned int showTextCount = 0;
extern int gdi_text_len(void *string);
extern GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);

static int show_text_get_rect(GxSubtitleShowText *showText, GxSubtitleShowTextPage *page)
{
	void* font = NULL;
	unsigned int i = 0, max_w = 0;
	unsigned int width = 0, height = 0;

	if ((page->lines.lines <= 0) ||
			(page->lines.lines > MAX_PAGE_TEXT_LINES)) {
		gxlogd("%s %d, Show Text Line Zerros\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GXGDI_SetFont((const unsigned char *)showText->ft.str);
	font = GXGDI_GetFont();
	if (NULL == font) {
		gxlogd("%s %d, Show Text Font Error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	showText->lines.line = page->lines.lines;
	for (i = 0; i < page->lines.lines; i++) {
		if (page->lines.text[i]) {
			unsigned int w = gdi_text_len((void*)page->lines.text[i]);

			showText->lines.rect[i].lineWidth  = (w / 4 + 1) * 4;
			showText->lines.rect[i].lineHeight = (GXGDI_GetFontHeight(font) / 2  + 1) * 2;
			max_w = (max_w > w) ? max_w : w;
		}
	}

	width  = (max_w / 4 + 1) * 4;
	height = (GXGDI_GetFontHeight(font) / 2  + 1) * 2 * page->lines.lines;

	if ((width > 1920) || (height > 1080)) {
		gxlogd("w %d, h %d\n", width, height);
		for (i = 0; i < page->lines.lines; i++)
			gxlogd("%d - 0x%p: %s\n", i, page->lines.text[i], page->lines.text[i]);
		return -1;
	}

	if (showText->pos.autoPos) {
		showText->fontWidth  = width;
		showText->fontHeight = height;
	} else {
		showText->fontWidth  = (width  > showText->fontWidth ) ? width  : showText->fontWidth;
		showText->fontHeight = (height > showText->fontHeight) ? height : showText->fontHeight;
	}

	return 0;
}


static int show_text_create_surface(GxSubtitleShowText *showText, GxSubtitleShowTextPage *page)
{
	unsigned int i = 0;
	GAL_Rect gdiRect;
	GXGDI_Rect fillRect;

	if (showText->argb8888Surface) {
		gxlogd("%s %d, Show Text Surface Existed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (0 != show_text_get_rect(showText, page)) {
		gxlogd("%s %d, Show Text Get Surface Failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	gdiRect.x = gdiRect.y = 0;
	gdiRect.w = showText->fontWidth;
	gdiRect.h = showText->fontHeight;
	showText->argb8888Surface = hd_get_surface0(&gdiRect, 32, GX_COLOR_FMT_ARGB8888, NULL, TRUE);
	if (NULL == showText->argb8888Surface) {
		gxlogd("%s %d, Show Text Create Surface Failed\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (showText->pos.autoPos) {
		fillRect.x = 0x0;
		fillRect.y = 0x0;
		fillRect.w = showText->fontWidth;
		fillRect.h = showText->fontHeight;
		GXGDI_FillRect(showText->argb8888Surface, &fillRect, showText->bgColor);
	} else {
		fillRect.x = 0x0;
		fillRect.y = 0x0;
		fillRect.w = showText->fontWidth;
		fillRect.h = showText->fontHeight;
		GXGDI_FillRect(showText->argb8888Surface, &fillRect, 0x00000000);

		if (showText->bgColor != 0x00000000) {
			fillRect.x = 0x0;
			fillRect.y = 0x0;
			for (i = 0; i < showText->lines.line; i++) {
				fillRect.w = showText->lines.rect[i].lineWidth;
				fillRect.h = showText->lines.rect[i].lineHeight;
				GXGDI_FillRect(showText->argb8888Surface, &fillRect, showText->bgColor);
				fillRect.y += fillRect.h;
			}
		}
	}

	return 0;
}

static int show_text_destory_surface(GxSubtitleShowText *showText)
{
	if (showText->argb8888Surface){
		hd_free_surface(showText->argb8888Surface);
		showText->argb8888Surface = NULL;
	}

	return 0;
}

static void show_text_draw(GxSubtitleShowText *showText, GxSubtitleShowTextPage *page)
{
	int ret = 0, i = 0;
	int fontHeightPerLine = 0, fontWidthPerLine = 0;
	GXGDI_Rect srcRect = {0}, dstRect = {0};

	GXGDI_Begin();
	ret = show_text_create_surface(showText, page);
	GXGDI_End();
	if (0 != ret) {
		gxlogd("%s %d, Show Text Create Surface Failed\n", __FUNCTION__, __LINE__);
		return;
	}

	GXGDI_Begin();
	fontWidthPerLine  = showText->fontWidth;
	fontHeightPerLine = showText->fontHeight / page->lines.lines;
	for (i = 0; i < page->lines.lines; i++) {
		GXGDI_Rect fontRect = {0};

		fontRect.x = 0;
		fontRect.y = fontHeightPerLine * i;
		fontRect.w = fontWidthPerLine;
		fontRect.h = fontHeightPerLine;
		GXGDI_DrawString(showText->argb8888Surface, page->lines.text[i],
				&fontRect, 0x0, showText->ft.color, GDI_STRING_PARAGRAPH);
	}
	GXGDI_End();

	GXGDI_Begin();
	srcRect.x = srcRect.y = 0;
	srcRect.w = showText->fontWidth;
	srcRect.h = showText->fontHeight;
	if (showText->pos.autoPos) {
		dstRect.x = (showText->cv.w - showText->fontWidth) / 2;
		dstRect.y = (showText->cv.h - showText->fontHeight - showText->pos.yb);
	} else {
		dstRect.x = showText->pos.xl;
		dstRect.y = showText->pos.yt;
	}
	dstRect.w = showText->fontWidth;
	dstRect.h = showText->fontHeight;
	showText->drawRect = dstRect;
	GXGDI_LayerSppDraw(showText->argb8888Surface, &srcRect, &dstRect);
	GXGDI_End();

	GXGDI_Begin();
	show_text_destory_surface(showText);
	GXGDI_End();

	return;
}

static void show_text_clean(GxSubtitleShowText *showText)
{
	GXGDI_LayerSppClean(&showText->drawRect);
	return;
}

static handle_t GXGDI_ShowTextOpen(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	GxSubtitleShowText *showText = NULL;

	if (showTextCount == 0) {
		if (GXGDI_LayerSppCreate(cv, sr) < 0) {
			gxlogd("%s %d, GXGDI_LayerSppCreate Failed\n", __FUNCTION__, __LINE__);
			return 0;
		}
	}

	showText = (GxSubtitleShowText *)av_mallocz(sizeof(GxSubtitleShowText));
	if (!showText) {
		gxloge("%s %d, Show SPP Create Failed\n", __FUNCTION__, __LINE__);
		return 0;
	}

	if (0 != GXGDI_LayerSppGetModule(&showText->device, &showText->module)) {
		gxloge("%s %d, Show Text Get Module Failed\n", __FUNCTION__, __LINE__);
		av_free((void *)showText);
		return 0;
	}

	showTextCount++;
	memcpy(&showText->cv, cv, sizeof(GxSubtitleShowCanvas));

	return (handle_t)showText;
}

static void GXGDI_ShowTextClose(handle_t handle)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return;
	}
	if (showTextCount <= 0) {
		gxlogi("%s %d, showTextCount is NULL\n", __FUNCTION__, __LINE__);
		return;
	}

	if (showText->showing)
		show_text_clean(showText);

	if (showText->ft.str) {
		av_free((void *)showText->ft.str);
		showText->ft.str = NULL;
	}

	av_free((void*)showText);
	showTextCount--;

	if (showTextCount == 0)
		GXGDI_LayerSppDestory();

	return;
}

static int GXGDI_ShowTextDraw(handle_t handle, GxSubtitleShowTextPage *txt)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GXGDI_Lock();
	if (txt->attr)
		show_text_draw(showText, txt);
	GXGDI_Unlock();

	showText->showing = 1;

	return 0;
}

static int GXGDI_ShowTextClean(handle_t handle)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	GXGDI_Lock();
	if (showText->showing)
		show_text_clean(showText);
	GXGDI_Unlock();

	showText->showing = 0;

	return 0;
}

static int GXGDI_ShowTextSetScreenColor(handle_t handle, unsigned int color)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	showText->bgColor = color;

	return 0;
}

static int GXGDI_ShowTextSetDisplayPos(handle_t handle, GxSubtitleShowDisplay *pos)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	showText->pos = *pos;

	return 0;
}

static int GXGDI_ShowTextSetFont(handle_t handle, GxSubtitleShowFont *font)
{
	GxSubtitleShowText *showText = (GxSubtitleShowText *)handle;

	if (!showText) {
		gxlogd("%s %d, Show Text Not Open\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (showText->ft.str) {
		if (0 != strcmp(font->str, showText->ft.str)) {
			av_free((void *)showText->ft.str);
			showText->ft.str = font->str ? av_strdup(font->str) : NULL;
		}
	} else
		showText->ft.str = font->str ? av_strdup(font->str) : NULL;

	showText->ft.color = font->color;

	return 0;
}

static int GXGDI_ShowTextShow(void)
{
	if (showTextCount)
		GXGDI_LayerSppShow();

	return 0;
}

static int GXGDI_ShowTextHide(void)
{
	if (showTextCount)
		GXGDI_LayerSppHide();

	return 0;
}

static int GXGDI_ShowTextSetScreen(GxSubtitleShowScreen *screen)
{
	if (showTextCount) {
		GXGDI_Rect rect;

		rect.x = screen->x;
		rect.y = screen->y;
		rect.w = screen->w;
		rect.h = screen->h;
		return GXGDI_LayerSppScreen(&rect);
	}

	return 0;
}

GxSubtitleShowTextOps gdiTextOps = {
	.name   = "GXGDI Show Text",
	.open   = GXGDI_ShowTextOpen,
	.close  = GXGDI_ShowTextClose,
	.draw   = GXGDI_ShowTextDraw,
	.clean  = GXGDI_ShowTextClean,
	.show   = GXGDI_ShowTextShow,
	.hide   = GXGDI_ShowTextHide,
	.setScreenColor = GXGDI_ShowTextSetScreenColor,
	.setScreen      = GXGDI_ShowTextSetScreen,
	.setDisplayPos  = GXGDI_ShowTextSetDisplayPos,
	.setFont        = GXGDI_ShowTextSetFont,
};
