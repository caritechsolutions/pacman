/*
 * =====================================================================================
 *
 *       Filename:  app_osd_subtitle.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2020年10月30日 19时08分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  luyong,
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_OSD_SUBTITLE_H__
#define __APP_OSD_SUBTITLE_H__

#include "gui_core/gdi_core.h"
#include "module/subtitle/gx_subtitle_module.h"
#include "module/player/gxplayer_module.h"

typedef enum{
    OSDSUBT_FONT_MAIN = 0,
    OSDSUBT_FONT_PROG,
    OSDSUBT_FONT_22,
}OsdSubtFont;

typedef enum{
    OSDSUBT_ENCODING_ISO8859 = 0,
    OSDSUBT_ENCODING_WINDOWS125X,
    OSDSUBT_ENCODING_UTF8,
}OsdSubtEncoding;

typedef enum{
    UNKONWN_SUBT = 0,
    PIXEL_SUBT,
    TEXT_SUBT,
}OsdSubtType;

typedef enum{
    ISO6937 = 0,
    ISO885901,
    ISO885902,
    ISO885903,
    ISO885904,
    ISO885905,
    ISO885906,
    ISO885907,
    ISO885908,
    ISO885909,
    ISO885910,
    ISO885911,
    ISO885912,
    ISO885913,
    ISO885914,
    ISO885915,
    ISO885916,
}OsdSubtIsoCode;

typedef enum{
    WINDOWS1250 = 0,
    WINDOWS1251,
    WINDOWS1252,
    WINDOWS1253,
    WINDOWS1254,
    WINDOWS1255,
    WINDOWS1256,
    WINDOWS1257,
    WINDOWS1258,
    WINDOWS1259,
}OsdSubtWinCode;

typedef struct _OsdSubtColor
{
    uint32_t         forecolor;
    uint32_t         backcolor;
    uint32_t         defcolor;
}OsdSubtColor;

typedef struct OsdSubtCodePage
{
    OsdSubtIsoCode isocode;
    OsdSubtWinCode wincode;
}OsdSubtCodePage;

extern int32_t app_osd_subt_init(void);
extern void app_osd_subt_deinit(void);
extern int32_t app_osd_subt_pause(void);
extern int32_t app_osd_subt_resume(void);
extern int32_t app_osd_subt_encoding_get(void);
extern int32_t app_osd_subt_font_get(void);
extern int32_t app_osd_subt_type_get(void);
extern void app_osd_subt_color_set(OsdSubtColor color);
extern void app_osd_subt_font_set(OsdSubtFont font);
extern void app_osd_subt_encoding_set(OsdSubtEncoding encoding);
extern void app_osd_subt_codepage_set(OsdSubtCodePage codepage);
extern void app_osd_subt_type_set(OsdSubtType type);
#endif

