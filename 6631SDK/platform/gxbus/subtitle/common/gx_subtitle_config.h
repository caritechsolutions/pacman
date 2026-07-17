#ifndef __GX_SUBTITLE_CONFIG_1_H__
#define __GX_SUBTITLE_CONFIG_1_H__

typedef struct {
	unsigned int            defBGColor;
	unsigned int            defFTColor;
	GX_SUBTITLE_DISPLAY     defDisPos;
	GX_SUBTITLE_SCREEN      screen;
	GX_SUBTITLE_CANVAS      canvas;
	GX_SUBTITLE_CANVAS      limitSrcCV;
	GX_SUBTITLE_BACK_COLOR  bgColor;
	GX_SUBTITLE_FONT_COLOR  ftColor;
	GX_SUBTITLE_DISPLAY     disPos;
	GX_SUBTITLE_FONT_LIB    ftLib;
	GX_SUBTITLE_LANG        ttxLang;
	GX_SUBTITLE_DVB_CLUT    dvbClut;
	GX_SUBTITLE_ATSCCC_MODE atscMode;
	GX_SUBTITLE_MAG_ALPHA   magAlpha;
	int                     yOffset;
} GX_SUBTITLE_CONFIG;

extern GX_SUBTITLE_CONFIG subtitleConfig;

#endif
