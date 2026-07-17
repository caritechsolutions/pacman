/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2020.03.02		  zhouhm 			 creation
*****************************************************************************/
#include "gxtype.h"
#include "gxos/gxcore_os.h"
#include "gxcore.h"
#include "gui_core.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gxavdev.h"
#include "../scte/gxatsc_cc_inner.h"
#include "../scte/gx_cc_subtitle_common.h"
#include "../scte/gx_cc_subtitle_draw.h"


uint8_t current_window_id=0;
extern GuiSurface *spp_char_window[CC_WINDOW_MAX];
extern GuiSurface *layer_char_window[CC_WINDOW_MAX];
extern char *layer_name[CC_WINDOW_MAX];

int gx_atsc_cc_708_set_current_window(uint8_t window_id)
{
	current_window_id = window_id;
	
	DRIVER_ATSC_CC_DBG(("atsc cc 708 set current  window id=%d\n",window_id));
	return 0;	
}

int gx_atsc_cc_708_clear_window(uint8_t window_id)
{
	uint8_t i=0;

	if (FALSE == window_define[window_id].flag)
		return -1;

	gx_atsc_cc_clear_window(window_id,spp_char_window[window_id]);
	if (TRUE == window_define[window_id].visible)
		{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);
#else
			gx_atsc_cc_clear_window(window_id,layer_char_window[window_id]);
#endif
		}

	for (i=0;i<CC_WINDOW_MAX;i++)
		{
			if (i == window_id)
				continue;
			if (TRUE == window_define[i].visible)
				gx_atsc_cc_display_window(i);
		}
	DRIVER_ATSC_CC_DBG(("atsc cc 708 Clear Window  id=%d\n",window_id));
	return 0;
}

int gx_atsc_cc_708_display_window(uint8_t window_id)
{
	if (FALSE == window_define[window_id].flag)
		return -1;

	if (FALSE == window_define[window_id].visible)
		gx_atsc_cc_display_window(window_id);
		
	window_define[window_id].visible=TRUE;
	
	DRIVER_ATSC_CC_DBG(("atsc cc 708 display Window id=%d\n",window_id));
	return 0;
}

int gx_atsc_cc_708_hide_window(uint8_t window_id)
{
	uint8_t i=0;

	if (FALSE == window_define[window_id].flag)
		return -1;

	if (TRUE == window_define[window_id].visible)
		{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);
#else
			gx_atsc_cc_clear_window(window_id,layer_char_window[window_id]);
#endif
		}
	window_define[window_id].visible=FALSE;

	for (i=0;i<CC_WINDOW_MAX;i++)
		{
			if (TRUE == window_define[i].visible)
				gx_atsc_cc_display_window(i);
		}
	
	DRIVER_ATSC_CC_DBG(("atsc cc 708 hide Window id=%d\n",window_id));
	return 0;
}

int gx_atsc_cc_708_toggle_window(uint8_t window_id)
{
	if (FALSE == window_define[window_id].flag)
		return -1;
	
	if (FALSE == window_define[window_id].visible)
		gx_atsc_cc_708_display_window(window_id);
	else
		gx_atsc_cc_708_hide_window(window_id);
	
	DRIVER_ATSC_CC_DBG(("atsc cc 708 toggle Window id=%d\n",window_id));
	return 0;
}

int gx_atsc_cc_708_delete_window(uint8_t window_id)
{
	uint8_t i=0;

	if (FALSE == window_define[window_id].flag)
		return -1;
	gx_atsc_cc_delete_window(window_id,TRUE);

	for (i=0;i<CC_WINDOW_MAX;i++)
		{
			if (TRUE == window_define[i].visible)
				gx_atsc_cc_display_window(i);
		}	
	
	DRIVER_ATSC_CC_DBG(("atsc cc 708 delete Window id=%d\n",window_id));
	return 0;
}

int gx_atsc_cc_708_delay(uint8_t tenthsofseconds)
{
	DRIVER_ATSC_CC_DBG(("atsc cc 708 delay %d ms\n",tenthsofseconds*100));
	return 0;
}

int gx_atsc_cc_708_delay_cancle(void)
{
	DRIVER_ATSC_CC_DBG(("atsc cc 708 delay cancle\n"));
	return 0;
}

int gx_atsc_cc_708_reset(void)
{
	DRIVER_ATSC_CC_DBG(("atsc cc 708 reset\n"));
	return 0;
}


int gx_atsc_cc_708_set_pen_attributes(atsc_cc_pen_attribute_t pen_attr)
{
	return 0;
}

int gx_atsc_cc_708_set_pen_color(atsc_cc_pen_color_t color)
{
	return 0;
}


int gx_atsc_cc_708_set_pen_location(atsc_cc_pen_location_t location)
{
	int32_t font_height=0;
	int32_t font_width=0;
	if (FALSE == window_define[current_window_id].flag)
		return -1;
	
	gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
	cc_row_para[current_window_id][pen_location.row].width= pen_location.column*font_width;
	if ( cc_row_para[current_window_id][pen_location.row].width > cc_row_para[current_window_id][pen_location.row].max_width )
		cc_row_para[current_window_id][pen_location.row].max_width = cc_row_para[current_window_id][pen_location.row].width;
	cc_row_para[current_window_id][pen_location.row].height= font_height;
	return 0;
}

int gx_atsc_cc_708_set_window_attributes(atsc_cc_window_attribute_t window_attr,uint8_t window_id)
{
	return 0;
}

int gx_atsc_cc_708_define_window(atsc_cc_window_define_t window_old,atsc_cc_window_define_t window,uint8_t window_id)
{
	uint16_t row_count=window_define[window_id].row_count;
	uint16_t column_count=window_define[window_id].column_count;
	window_define[window_id].row_count = window_old.row_count;
	window_define[window_id].column_count = window_old.column_count;
	gx_atsc_cc_creat_window(window_id,window_old,row_count,column_count);
	window_define[window_id].row_count=row_count;
	window_define[window_id].column_count=column_count;
	
	return 0;
}

int gx_atsc_cc_708_c0_code_set(C0_CODE_SET_T c0_code,uint8_t window_id)
{
	switch(c0_code)
		{
			case C0_NULL:
				break;
			case C0_ETX:
				break;
			case C0_BS:		
				gx_atsc_cc_clear_last_character(window_id);
				break;
			case C0_FF:
				gx_atsc_cc_708_clear_window(window_id);
				pen_location.column=0;
				DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
				pen_location.row=0;
				DRIVER_ATSC_CC_DBG(("pen_location.row=%d\n",pen_location.row));	
				gx_atsc_cc_708_set_pen_location(pen_location);
				break;
			case C0_CR:
				switch(window_attribute[window_id/*current_window_id*/].scroll_direction)
					{
						case BOTTOM_TO_TOP:
							gx_atsc_cc_scroll(current_window_id,BOTTOM_TO_TOP);
							break;
						default:
							break;
					}
				break;
			case C0_HCR:
				break;
			case C0_ETX1:
				break;
			case C0_P16:
				break;
			default:
				break;
		}
	return 0;
}

int gx_atsc_cc_708_g2_code_set(G2_CODE_SET_T g2_code)
{
	switch(g2_code)
		{
			case G2_TSP:
				{
					OPACITY_TYPE opacity=pen_color.background_opacity;
					pen_color.background_opacity=OPACITY_TRANSPARENT;
//					memset(show_character,0,sizeof(show_character));
//					show_character[0]=' ';
					gx_atsc_cc_draw_character(" ",current_window_id);
					pen_color.background_opacity=opacity;
				}
				break;
			case G2_NBTSP:
				{
					OPACITY_TYPE opacity=pen_color.background_opacity;
					pen_color.background_opacity=OPACITY_TRANSPARENT;
//					memset(show_character,0,sizeof(show_character));
//					show_character[0]=' ';
					gx_atsc_cc_draw_character(" ",current_window_id);
					pen_color.background_opacity=opacity;
				}
				break;
			case G2_30:
				{
					COLOR_TYPE color=pen_color.background_color;
					pen_color.background_color=pen_color.foreground_color;
//					memset(show_character,0,sizeof(show_character));
//					show_character[0]=' ';
					gx_atsc_cc_draw_character(" ",current_window_id);
					pen_color.background_color=color;
				}
				break;
			default:
				break;
		}
	return 0;
}



