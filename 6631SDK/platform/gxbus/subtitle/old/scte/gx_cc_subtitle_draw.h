/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2020.02.27		  zhouhm 			 creation
*****************************************************************************/

#ifndef __GX_CC_SUBTITLE_DRAW__H__
#define __GX_CC_SUBTITLE_DRAW__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>
#include "gxos/gxcore_os.h"
#include "gxcore.h"
#include "gui_core.h"
#include "gx_cc_subtitle_common.h"
#include "gx_atsc_cc.h"
#include "gxscte_subtitle_inner.h"
#include "gxatsc_cc_inner.h"


extern int g_device_handle;
extern int vpu_handle;
extern GuiCore gui;
extern void* GxAvdev_GetLayerMainSurface(int dev, int vpu,int layer);
extern int hd_enable_img(int flag);
extern int hd_enable_video(int flag);
extern GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);
extern int hd_zoom_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
extern void* GXGDI_SetFont(const unsigned char *name);
extern int gdi_text_len(void *string);
extern void* GXGDI_GetFont(void);
extern int GXGDI_GetFontHeight(void* pfont);

extern GuiSurface *spp_cc_subtitle_surface;
#define CC_SUPPORT_SPP_LAYER (0)
#define CC_SUPPORT_OSD_LAYER (1)
#define CC_SUPPORT_LAYER CC_SUPPORT_SPP_LAYER
//#define CC_SUPPORT_LAYER CC_SUPPORT_OSD_LAYER

int32_t gx_color_type_ycbcr(COLOR_TYPE color_type,uint32_t* color,uint32_t* y,uint32_t* cb,uint32_t* cr);
int32_t gx_ycbcr_rgb(uint8_t y,uint8_t cb,uint8_t cr,uint8_t* r,uint8_t* g,uint8_t* b);
int gx_clear_cc_subtitle_spp(GXGDI_Rect rect,GuiSurface *surface);
int32_t gx_get_font_width_height(const unsigned char *name,char* show_char,int32_t *height,int32_t *width);
GuiSurface *gx_create_font_surface(GAL_Rect* create_rect,int bpp, GxColorFormat color_format,uint32_t y,uint32_t cb,uint32_t cr,OPACITY_TYPE opacity);
int32_t gx_init_cc_subtitle_spp(GAL_Rect create_rect);
int32_t gx_close_cc_subtitle_spp(void);

int32_t gx_atsc_cc_get_font_height(CC_PEN_SIZE pen_size,char* show_char,int32_t *height,int32_t *width);
int gx_atsc_cc_scroll(uint8_t window_id,CC_DIRECTION scroll_direction);
int32_t gx_atsc_cc_delete_to_end_of_low(uint8_t window_id);
int32_t gx_atsc_cc_clear_last_character(uint8_t window_id);
int32_t gx_atsc_cc_draw_character(char* show_char,uint8_t window_id);
int gx_atsc_cc_clear_window(uint8_t window_id,GuiSurface *surface);
int gx_atsc_cc_display_window(uint8_t window_id);
int gx_atsc_cc_delete_window(uint8_t window_id,uint8_t clear_flag);
int32_t gx_atsc_cc_init_row_para(uint8_t window_id,uint16_t row_count,int32_t font_height);
int gx_atsc_cc_creat_window(uint8_t window_id,atsc_cc_window_define_t window_old,uint16_t row_count,uint16_t column_count);
int gx_atsc_cc_608_parse(char* data, int cc_count);
int gx_atsc_cc_608_unit_parse(char* data);
int gx_atsc_cc_708_parse(char* data, int cc_count);
int gx_atsc_cc_close(void);

int gx_atsc_cc_608_preamble_address_codes_action(uint8_t first_code_pair,uint8_t second_code_pair);
int gx_atsc_cc_608_background_foreground_attributes_action(BACK_FORE_GROUND_CODES_T back_fore_ground_code);

#ifdef __cplusplus
}
#endif
#endif /*__GX_CC_SUBTITLE_DRAW__H__*/

