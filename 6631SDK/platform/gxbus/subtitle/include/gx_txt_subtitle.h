#ifndef __GX_TXT_SUBTITLE_H__
#define __GX_TXT_SUBTITLE_H__

#define GX_TXT_MAX_SUB_LINE         (12)

typedef enum {
	GX_TXT_RENDER_SPP,
	GX_TXT_RENDER_OSD,
} GxTxtSubtitleRender;

typedef struct {
	unsigned int        x;
	unsigned int        y;
	unsigned int        width;
	unsigned int        height;
} GxTxtSubtitleWindow;

typedef enum {
	GX_TXT_SUB_ENC_UTF8     ,
	GX_TXT_SUB_ENC_ANSI     ,
	GX_TXT_SUB_ENC_UCS      ,
	GX_TXT_SUB_ENC_SPU
} GxTxtSubtitleEncode;

typedef struct gx_txt_sub_page {
	unsigned int        lines;
	GxTxtSubtitleEncode encode;
	unsigned int        start;
	unsigned int        end;
	char*               text[GX_TXT_MAX_SUB_LINE];

	struct gx_txt_sub_page*  next;
} GxTxtSubtitleContent;

extern int  GxTxtSubtitle_Init   (GxTxtSubtitleWindow *rect, GxTxtSubtitleRender render);
extern void GxTxtSubtitle_Destory(int handle);
extern int  GxTxtSubtitle_Show   (int handle);
extern int  GxTxtSubtitle_Hide   (int handle);
extern int  GxTxtSubtitle_Draw   (int handle, GxTxtSubtitleContent *content, int track, GxTxtSubtitleEncode encode);
extern void GxTxtSubtitle_Clean  (int handle);

#endif

