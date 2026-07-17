/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.02.02		  zhouhm 			 creation
*****************************************************************************/
#include "gxtype.h"
#include "gxos/gxcore_os.h"
#include "gxcore.h"
//#include "gui_core.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gxavdev.h"
#include "../scte/gxatsc_cc_inner.h"
#include "../scte/gx_cc_subtitle_common.h"
#include "../scte/gx_cc_subtitle_draw.h"
#include "gx_mem.h"
#include "gx_subtitle_atsccc.h"

uint8_t last_sequence_number[8]={0xFF};
extern uint8_t current_window_id;
extern int gx_atsc_cc_708_set_current_window(uint8_t window_id);
extern int gx_atsc_cc_708_clear_window(uint8_t window_id);
extern int gx_atsc_cc_708_display_window(uint8_t window_id);
extern int gx_atsc_cc_708_hide_window(uint8_t window_id);
extern int gx_atsc_cc_708_toggle_window(uint8_t window_id);
extern int gx_atsc_cc_708_delete_window(uint8_t window_id);
extern int gx_atsc_cc_708_delay(uint8_t tenthsofseconds);
extern int gx_atsc_cc_708_delay_cancle(void);
extern int gx_atsc_cc_708_reset(void);
extern int gx_atsc_cc_708_set_pen_attributes(atsc_cc_pen_attribute_t pen_attr);
extern int gx_atsc_cc_708_define_window(atsc_cc_window_define_t window_old,atsc_cc_window_define_t window,uint8_t window_id);
extern int gx_atsc_cc_708_set_pen_location(atsc_cc_pen_location_t location);
extern int gx_atsc_cc_708_set_pen_color(atsc_cc_pen_color_t color);
extern int gx_atsc_cc_708_set_window_attributes(atsc_cc_window_attribute_t window_attr,uint8_t window_id);
extern int32_t gx_atsc_cc_clear_last_character(uint8_t window_id);
extern int32_t gx_atsc_cc_draw_character(char* show_char,uint8_t window_id);
extern int gx_atsc_cc_708_c0_code_set(C0_CODE_SET_T c0_code,uint8_t window_id);
extern int gx_atsc_cc_708_g2_code_set(G2_CODE_SET_T g2_code);
extern int gx_atsc_cc_unicode_to_utf8_one(unsigned long unic, char *pOutput);
extern int gx_atsc_cc_add(GX_SUBTITLE_ATSCCC_SERVICE service);

static COLOR_TYPE gx_atsc_cc_708_rgb_to_color(uint8_t r, uint8_t g,uint8_t b)
{
	uint8_t red=r&0x02;
	uint8_t green=g&0x02;	
	uint8_t blue=b&0x02;	
	
	if ((0 == red)&&(0 == green)&&(0 == blue))
		{
			return COLOR_BLACK;		
		}
	
	if ((2 == red)&&(2 == green)&&(2 == blue))
		{
			return COLOR_WHITE;		
		}

	if ((2 == red)&&(0 == green)&&(0 == blue))
		{
			return COLOR_RED; 	
		}

	if ((0 == red)&&(2 == green)&&(0 == blue))
		{
			return COLOR_GREEN; 	
		}

	if ((0 == red)&&(0 == green)&&(2 == blue))
		{
			return COLOR_BLUE; 	
		}

	if ((2 == red)&&(2 == green)&&(0 == blue))
		{
			return COLOR_YELLOW;	
		}

	if ((2 == red)&&(0 == green)&&(2 == blue))
		{
			return COLOR_MAGENTA;	
		}

	if ((0 == red)&&(2 == green)&&(2 == blue))
		{
			return COLOR_CYAN;	
		}	
	
	return COLOR_BLACK; 	
}

int gx_atsc_cc_708_parse_pen_predefined_attributes(uint8_t window_id)
{
	pen_attribute.pen_size=PEN_STANDARD;
	pen_attribute.pen_offset=OFFSET_NORMAL;
	pen_attribute.italic=FALSE;
	pen_attribute.underline=FALSE;
	pen_attribute.edge_type=EDGE_TYPE_NONE;				
	switch(window_define[window_id].pen_style)
		{
			case 1:
				pen_attribute.font_tag=FONT_TAG_DEFAULT;					
				break;
			case 2:
				pen_attribute.font_tag=FONT_TAG_MONOSPACED_SERIF;
				break;
			case 3:
				pen_attribute.font_tag=FONT_TAG_PROPORTIONAL_SERIF;
				break;
			case 4:
				pen_attribute.font_tag=FONT_TAG_MONOSPACED_SANSERIF;
				break;
			case 5:
				pen_attribute.font_tag=FONT_TAG_PROPORTIONAL_SANSERIF;
				break;
			case 6:
				pen_attribute.font_tag=FONT_TAG_MONOSPACED_SANSERIF;
				break;
			case 7:
				pen_attribute.font_tag=FONT_TAG_PROPORTIONAL_SANSERIF;
				break;
			default:
				break;
}
	return 0;
}


int gx_atsc_cc_708_parse_pen_attributes(char* data)
{
	uint8_t para=0;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 set pen attributes\n"));
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR((" para NULL error\n"));
			return -1;
		}

	para=data[0];
	pen_attribute.flag=TRUE;
	pen_attribute.pen_size=para&0x3;
	switch(pen_attribute.pen_size)
		{
			case PEN_SMALL:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen size SMALL \n"));
				break;
			case PEN_STANDARD:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen size STANDARD \n"));
				break;
			case PEN_LARGE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributespen size LARGE \n"));
				break;
			case PEN_INVALID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen size  ILLEGAL_VAL \n"));
				break;
			default:
				break;
		}
	pen_attribute.pen_offset=(para&0x0c)>>2;
	switch(pen_attribute.pen_offset)
		{
			case OFFSET_SUBSCRIPT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen offset SUBSCRIPT\n"));
				break;
			case OFFSET_NORMAL:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen offset NORMAL\n"));
				break;
			case OFFSET_SUPERSCRIPT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen offset SUPERSCRIPT\n"));
				break;
			case OFFSET_INVALID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen offset ILLEGAL_VAL\n"));
				break;
			default:
				break;
		}
	pen_attribute.text_tag=(para>>4)&0x0f;
	switch(pen_attribute.text_tag)
		{
			case TEXT_TAG_DIALOG:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag dialog\n"));
				break;
			case TEXT_TAG_SOURCE_OR_SPEAKER_ID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag source_or_speaker_id\n"));
				break;
			case TEXT_TAG_ELECTRONICALLY_REPRODUCED_VOICE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag electronically_reproduced_voice\n"));
				break;
			case TEXT_TAG_DIALOG_IN_OTHER_LANGUAGE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag dialog_in_other_language\n"));
				break;
			case TEXT_TAG_VOICEOVER:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag voiceover\n"));
				break;
			case TEXT_TAG_AUDIBLE_TRANSLATION:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag audible_translation\n"));
				break;
			case TEXT_TAG_SUBTITLE_TRANSLATION:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag subtitle_translation\n"));
				break;
			case TEXT_TAG_VOICE_QUALITY_DESCRIPTION:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag voice_quality_description\n"));
				break;
			case TEXT_TAG_SONG_LYRICS:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag song_lyrics\n"));
				break;
			case TEXT_TAG_SONG_EFFECT_DESCRIPTION:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag sound_effect_description\n"));
				break;
			case TEXT_TAG_MUSICAL_SCORE_DESCRIPTION:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributespen text_tag musical_score_description\n"));
				break;
			case TEXT_TAG_OATH:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag  oath\n"));
				break;
			case TEXT_TAG_UNDEFINED_0:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag  undefined_0\n"));
				break;
			case TEXT_TAG_UNDEFINED_1:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag  undefined_1\n"));
				break;
			case TEXT_TAG_UNDEFINED_2:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag  undefined_2\n"));
				break;
			case TEXT_TAG_INVISIBLE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen text_tag  invisible\n"));
				break;
			default:
				break;
		}
	para=data[1];
	pen_attribute.font_tag=para&0x7;
	switch(pen_attribute.font_tag)
		{
			case FONT_TAG_DEFAULT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag default\n"));
				break;
			case FONT_TAG_MONOSPACED_SERIF:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag monospaced_serif\n"));
				break;
			case FONT_TAG_PROPORTIONAL_SERIF:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag proportional_serif\n"));
				break;
			case FONT_TAG_MONOSPACED_SANSERIF:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag monospaced_sanserif\n"));
				break;
			case FONT_TAG_PROPORTIONAL_SANSERIF:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag proportional_sanserif\n"));
				break;
			case FONT_TAG_CASUAL:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag casual\n"));
				break;
			case FONT_TAG_CURSIVE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag cursive\n"));
				break;
			case FONT_TAG_SMALL_CAPS:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag smallcaps\n"));
				break;
			default:
				break;
		}
	pen_attribute.edge_type=(para&0x38)>>3;
	switch(pen_attribute.edge_type)
		{
			case EDGE_TYPE_NONE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type	NONE\n"));
				break;
			case EDGE_TYPE_RAISED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type RAISED\n"));
				break;
			case EDGE_TYPE_DEPRESSED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type DEPRESSED\n"));
				break;
			case EDGE_TYPE_UNIFORM:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type UNIFORM\n"));
				break;
			case EDGE_TYPE_LEFT_DROP_SHADOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type LEFT_DROP_SHADOW\n"));
				break;
			case EDGE_TYPE_RIGHT_DROP_SHADOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type RIGHT_DROP_SHADOW\n"));
				break;
			case EDGE_TYPE_ILLEGAL_VAL0:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type ILLEGAL_VAL0\n"));
				break;
			case EDGE_TYPE_ILLEGAL_VAL1:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen edge_type	ILLEGAL_VAL1\n"));
				break;
			default:
				break;
		}
	pen_attribute.underline=(para&0x40)>>6;
//	DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag underline=%d\n",pen_attribute.underline)); 													
	pen_attribute.italic=(para&0x80)>>7;
//	DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenAttributes pen font_tag italic=%d\n",pen_attribute.italic));

	return 0;
}

int gx_atsc_cc_708_parse_pen_predefined_color(uint8_t window_id)
{		
	pen_color.foreground_color = COLOR_WHITE;
	pen_color.foreground_opacity = OPACITY_SOLID;
	pen_color.background_color = COLOR_WHITE;
	pen_color.background_opacity = OPACITY_SOLID;
	pen_color.edge_color = OPACITY_SOLID;
	switch(window_define[window_id].pen_style)
		{
			case 1:	
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case 2:
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case 3:
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case 4:
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case 5:
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case 6:
				pen_color.background_opacity = OPACITY_TRANSPARENT;
				break;
			case 7:
				pen_color.background_opacity = OPACITY_TRANSPARENT;
				break;
			default:
				break;
		}
	return 0;
}


int gx_atsc_cc_708_parse_pen_color(char* data)
{
	uint8_t para=0;	
	uint8_t r=0,g=0,b=0;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 set pen color\n"));
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR((" para NULL error\n"));
			return -1;
		}
	para=data[0];	
	b = para&0x3;
	g = (para>>2)&0x3;
	r = (para>>4)&0x3;
	pen_color.flag=TRUE;
	pen_color.foreground_color = gx_atsc_cc_708_rgb_to_color(r,g,b);
	switch(pen_color.foreground_color)
		{
			 case COLOR_BLACK:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_BLACK\n"));
				break;
			 case COLOR_WHITE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_WHITE\n"));
				break;
			 case COLOR_GREEN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_GREEN\n"));
				break;
			 case COLOR_BLUE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_BLUE\n"));
				break;
			 case COLOR_CYAN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_CYAN\n"));
				break;
			 case COLOR_RED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_RED\n"));
				break;
			 case COLOR_YELLOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_YELLOW\n"));
				break;
			 case COLOR_MAGENTA:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_color COLOR_MAGENTA\n"));
				break;
			default:
				break;
		}
	pen_color.foreground_opacity = (para>>6)&0x3;
	switch(pen_color.foreground_opacity)
		{
			case OPACITY_SOLID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_opacity SOLID\n"));																
				break;
			case OPACITY_FLASH:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_opacity	FLASH\n"));
				break;
			case OPACITY_TRANSLUCENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_opacity	TRANSLUCENT\n"));
				break;
			case OPACITY_TRANSPARENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor foreground_opacity	TRANSPARENT\n"));
				break;
			default:
				break;
		}
	para=data[1];
	b = para&0x3;
	g = (para>>2)&0x3;
	r = (para>>4)&0x3;
	pen_color.background_color = gx_atsc_cc_708_rgb_to_color(r,g,b);
	switch(pen_color.background_color)
		{
			 case COLOR_BLACK:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_BLACK\n"));
				break;
			 case COLOR_WHITE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_WHITE\n"));
				break;
			 case COLOR_GREEN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_GREEN\n"));
				break;
			 case COLOR_BLUE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_BLUE\n"));
				break;
			 case COLOR_CYAN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_CYAN\n"));
				break;
			 case COLOR_RED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_RED\n"));
				break;
			 case COLOR_YELLOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_YELLOW\n"));
				break;
			 case COLOR_MAGENTA:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_color COLOR_MAGENTA\n"));
				break;
			default:
				break;
		}
	pen_color.background_opacity = (para>>6)&0x3;
	switch(pen_color.background_opacity)
		{
			case OPACITY_SOLID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_opacity SOLID\n"));																
				break;
			case OPACITY_FLASH:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_opacity	FLASH\n"));
				break;
			case OPACITY_TRANSLUCENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_opacity	TRANSLUCENT\n"));
				break;
			case OPACITY_TRANSPARENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor background_opacity	TRANSPARENT\n"));
				break;
			default:
				break;
		}
	para=data[2];
	b = para&0x3;
	g = (para>>2)&0x3;
	r = (para>>4)&0x3;
	pen_color.edge_color = gx_atsc_cc_708_rgb_to_color(r,g,b);
	switch(pen_color.edge_color)
		{
			 case COLOR_BLACK:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_BLACK\n"));
				break;
			 case COLOR_WHITE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_WHITE\n"));
				break;
			 case COLOR_GREEN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_GREEN\n"));
				break;
			 case COLOR_BLUE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_BLUE\n"));
				break;
			 case COLOR_CYAN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_CYAN\n"));
				break;
			 case COLOR_RED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_RED\n"));
				break;
			 case COLOR_YELLOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_YELLOW\n"));
				break;
			 case COLOR_MAGENTA:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenColor edge_color COLOR_MAGENTA\n"));
				break;
			default:
				break;
		}
	return 0;
	
}


int gx_atsc_cc_708_parse_pen_location(char* data)
{
	uint8_t para=0;	

	DRIVER_ATSC_CC_DBG(("atsc cc 708 set pen location\n"));
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR((" para NULL error\n"));
			return -1;
		}
	
	para=data[0];
	pen_location.row=para&0x0f;
	para=data[1];
	pen_location.column=para&0x3f;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 SetPenLocation row = %d column=%d\n",
		pen_location.row,pen_location.column));
	return 0;
}

int gx_atsc_cc_708_parse_window_predefined_attributes(uint8_t window_id)
{
	window_attribute[window_id].display_effect=DISPLAY_EFFECT_SNAP;
	window_attribute[window_id].effect_direction=LEFT_TO_RIGHT;
	window_attribute[window_id].effect_speed=0;
	window_attribute[window_id].fill_color=COLOR_BLACK;
	window_attribute[window_id].border_type=BORAD_TYPE_NONE;
	switch(window_define[window_id].window_style)
		{
			case 0:
			case 1:	
				window_attribute[window_id].justify=JUSTIFY_LEFT;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=FALSE;
				window_attribute[window_id].fill_opacity=OPACITY_SOLID;
				break;
			case 2:
				window_attribute[window_id].justify=JUSTIFY_LEFT;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=FALSE;
				window_attribute[window_id].fill_opacity=OPACITY_TRANSPARENT;
				break;
			case 3:
				window_attribute[window_id].justify=JUSTIFY_CENTER;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=FALSE;
				window_attribute[window_id].fill_opacity=OPACITY_SOLID;
				break;
			case 4:
				window_attribute[window_id].justify=JUSTIFY_LEFT;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=TRUE;
				window_attribute[window_id].fill_opacity=OPACITY_SOLID;
				break;
			case 5:
				window_attribute[window_id].justify=JUSTIFY_LEFT;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=TRUE;
				window_attribute[window_id].fill_opacity=OPACITY_TRANSPARENT;
				break;
			case 6:
				window_attribute[window_id].justify=JUSTIFY_CENTER;
				window_attribute[window_id].print_direction=LEFT_TO_RIGHT;
				window_attribute[window_id].scroll_direction=BOTTOM_TO_TOP;
				window_attribute[window_id].word_wrap=TRUE;
				window_attribute[window_id].fill_opacity=OPACITY_SOLID;
				break;
			case 7:
				window_attribute[window_id].justify=JUSTIFY_LEFT;
				window_attribute[window_id].print_direction=TOP_TO_BOTTOM;
				window_attribute[window_id].scroll_direction=RIGHT_TO_LEFT;
				window_attribute[window_id].word_wrap=FALSE;
				window_attribute[window_id].fill_opacity=OPACITY_SOLID;
				break;
			default:
				break;
		}
	return 0;
}


int gx_atsc_cc_708_parse_window_attributes(char* data,uint8_t window_id)
{
	uint8_t para=0;	
	uint8_t border_type=0;
	uint8_t justify=0;
	uint8_t r=0,g=0,b=0;

	DRIVER_ATSC_CC_DBG(("atsc cc 708 set window attributes id=%d\n",window_id));
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR((" para NULL error\n"));
			return -1;
		}

	if (0 != window_define[window_id].window_style)
		return 0;
	
	para=data[0];
	b = para&0x3;
	g = (para>>2)&0x3;
	r = (para>>4)&0x3;
	window_attribute[window_id].flag=TRUE;
	window_attribute[window_id].fill_color = gx_atsc_cc_708_rgb_to_color(r,g,b);
	switch(window_attribute[window_id].fill_color)
		{
			 case COLOR_BLACK:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_BLACK\n"));
				break;
			 case COLOR_WHITE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_WHITE\n"));
				break;
			 case COLOR_GREEN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_GREEN\n"));
				break;
			 case COLOR_BLUE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_BLUE\n"));
				break;
			 case COLOR_CYAN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_CYAN\n"));
				break;
			 case COLOR_RED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_RED\n"));
				break;
			 case COLOR_YELLOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_YELLOW\n"));
				break;
			 case COLOR_MAGENTA:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_color COLOR_MAGENTA\n"));
				break;
			default:
				break;
		}
	window_attribute[window_id].fill_opacity = (para>>6)&0x3;
	switch(window_attribute[window_id].fill_opacity)
		{
			case OPACITY_SOLID:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_opacity=SOLID\n")); 															
				break;
			case OPACITY_FLASH:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_opacity=FLASH\n")); 
				break;
			case OPACITY_TRANSLUCENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_opacity=TRANSLUCENT\n"));
				break;
			case OPACITY_TRANSPARENT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes fill_opacity=TRANSPARENT\n"));
				break;
			default:
				break;
		}
	para=data[1];
	b = para&0x3;
	g = (para>>2)&0x3;
	r = (para>>4)&0x3;
	window_attribute[window_id].border_color = gx_atsc_cc_708_rgb_to_color(r,g,b);
	switch(window_attribute[window_id].border_color)
		{
			 case COLOR_BLACK:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_BLACK\n"));
				break;
			 case COLOR_WHITE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_WHITE\n"));
				break;
			 case COLOR_GREEN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_GREEN\n"));
				break;
			 case COLOR_BLUE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_BLUE\n"));
				break;
			 case COLOR_CYAN:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_CYAN\n"));
				break;
			 case COLOR_RED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color  COLOR_RED\n"));
				break;
			 case COLOR_YELLOW:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_YELLOW\n"));
				break;
			 case COLOR_MAGENTA:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_color COLOR_MAGENTA\n"));
				break;
			default:
				break;
		}
	border_type = (para>>6)&0x3;
	para=data[2];
	justify=para&0x3;
	window_attribute[window_id].scroll_direction=(para>>2)&0x3;
	switch(window_attribute[window_id].scroll_direction)
		{
			case LEFT_TO_RIGHT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes scroll_direction  LEFT_TO_RIGHT\n"));
				break;
			case RIGHT_TO_LEFT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes scroll_direction  RIGHT_TO_LEFT\n"));
				break;
			case TOP_TO_BOTTOM:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes scroll_direction  TOP_TO_BOTTOM\n"));
				break;
			case BOTTOM_TO_TOP:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes scroll_direction  BOTTOM_TO_TOP\n"));
				break;
			default:
				break;
		}
	window_attribute[window_id].print_direction=(para>>4)&0x3;
	switch(window_attribute[window_id].print_direction)
		{
			case LEFT_TO_RIGHT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes print_direction LEFT_TO_RIGHT\n"));
				break;
			case RIGHT_TO_LEFT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes print_direction RIGHT_TO_LEFT\n"));
				break;
			case TOP_TO_BOTTOM:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes print_direction TOP_TO_BOTTOM\n"));
				break;
			case BOTTOM_TO_TOP:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes print_direction BOTTOM_TO_TOP\n"));
				break;
			default:
				break;
		}
	switch(justify) 
		{
			case 0:
				if ((LEFT_TO_RIGHT == window_attribute[window_id].print_direction)||(RIGHT_TO_LEFT == window_attribute[window_id].print_direction))
					{
						window_attribute[window_id].justify=JUSTIFY_LEFT;
//						DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify LEFT\n"));
					}
				else
					{
						window_attribute[window_id].justify=JUSTIFY_TOP;
//						DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify TOP\n"));
					}
				break;
			case 1:
				if ((LEFT_TO_RIGHT == window_attribute[window_id].print_direction)||(RIGHT_TO_LEFT == window_attribute[window_id].print_direction))
					{
						window_attribute[window_id].justify=JUSTIFY_RIGHT;
//						DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify RIGHT\n"));
					}
				else
					{
						window_attribute[window_id].justify=JUSTIFY_BOTTOM;
//						DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify BOTTOM\n")); 
					}
				break;
			case 2:
				window_attribute[window_id].justify=JUSTIFY_CENTER;
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify CENTER\n")); 
				break;
			case 3:
				window_attribute[window_id].justify=JUSTIFY_FULL;
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes justify FULL\n")); 
				break;
			default:
				break;
		}
	window_attribute[window_id].word_wrap=(para>>6)&0x1;
//	DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes word_wrap=%d\n",window_attribute[window_id].word_wrap));
	border_type=((para>>7)&0x1)<<2|border_type;
	window_attribute[window_id].border_type=border_type;
	switch(window_attribute[window_id].border_type)
		{
			case BORAD_TYPE_NONE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type NONE\n"));																
				break;
			case BORAD_TYPE_RAISED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type RAISED\n"));
				break;
			case BORAD_TYPE_DEPRESSED:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type DEPRESSED\n"));
				break;
			case BORAD_TYPE_UNIFORM:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type UNIFORM\n"));
				break;
			case BORAD_TYPE_SHADOW_LEFT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type SHADOW_LEFT\n"));
				break;
			case BORAD_TYPE_SHADOW_RIGHT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes border_type SHADOW_RIGHT\n"));
				break;
			default:
				break;
		}
	para=data[3];
	window_attribute[window_id].display_effect=para&0x3;
	switch(window_attribute[window_id].display_effect)
		{
			case DISPLAY_EFFECT_SNAP:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes display_effect SNAP\n"));
				break;
			case DISPLAY_EFFECT_FADE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes display_effect FADE\n"));
				break;
			case DISPLAY_EFFECT_WIPE:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes display_effect WIPE\n"));
				break;
			default:
				break;
		}
	window_attribute[window_id].effect_direction=(para>>2)&0x3;
	switch(window_attribute[window_id].effect_direction)
		{
			case LEFT_TO_RIGHT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes effect_direction LEFT_TO_RIGHT\n"));
				break;
			case RIGHT_TO_LEFT:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes effect_direction RIGHT_TO_LEFT\n"));
				break;
			case TOP_TO_BOTTOM:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes effect_direction TOP_TO_BOTTOM\n"));
				break;
			case BOTTOM_TO_TOP:
//				DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes effect_direction BOTTOM_TO_TOP\n"));
				break;
			default:
				break;
		}
	window_attribute[window_id].effect_speed=(para>>4)&0xf;
//	DRIVER_ATSC_CC_DBG(("atsc cc 708 SetWindowAttributes effect_speed =%d\n",window_attribute[window_id].effect_speed));

	return 0;
}

int gx_atsc_cc_708_parse_define_window(char* data,uint8_t window_id)
{
	uint16_t row_count=0;
	uint16_t column_count=0;
	uint8_t para=0;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 define window id=%d\n",window_id));
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR((" para NULL error\n"));
			return -1;
		}
	current_window_id = window_id;
	para=data[0];
	window_define[window_id].flag=TRUE;
	window_define[window_id].display_priority=para&0x7;
	window_define[window_id].column_lock=(para&0x08)>>3;
	window_define[window_id].row_lock=(para&0x10)>>4;
	window_define[window_id].visible=(para&0x20)>>5;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow display_priority=%d column_lock=%d row_lock=%d visible=%d\n",
		window_define[window_id].display_priority,window_define[window_id].column_lock,window_define[window_id].row_lock,window_define[window_id].visible)); 
	para=data[1];
	window_define[window_id].anchor_vertical = para&0x7f;
	window_define[window_id].relative_positioning=(para&0x80)>>7;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow anchor_vertical=%d relative_positioning=%d\n",
		window_define[window_id].anchor_vertical,window_define[window_id].relative_positioning));	

	para=data[2];
	window_define[window_id].anchor_horizontal=para;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow anchor_horizontal=%d\n",
		window_define[window_id].anchor_horizontal));
	
	para=data[3];
	row_count=para&0x0f;
	row_count+=1;
	window_define[window_id].anchor_ID=(para&0xf0)>>4;
	switch(window_define[window_id].anchor_ID)
		{
			case ANCHOR_ID_UPPER_LEFT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=UPPER_LEFT\n",
					row_count));																
				break;
			case ANCHOR_ID_UPPER_CENTER:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=UPPER_CENTER\n",
					row_count));		
				break;
			case ANCHOR_ID_UPPER_RIGHT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=UPPER_RIGHT\n",
					row_count));		
				break;
			case ANCHOR_ID_MIDDLE_LEFT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=MIDDLE_LEFT\n",
					row_count));	
				break;
			case ANCHOR_ID_MIDDLE_CENTER:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=MIDDLE_CENTER\n",
					row_count));																
				break;
			case ANCHOR_ID_MIDDLE_RIGHT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=MIDDLE_RIGHT\n",
					row_count));																
				break;
			case ANCHOR_ID_LOWER_LEFT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=LOWER_LEFT\n",
					row_count));	
				break;
			case ANCHOR_ID_LOWER_CENTER:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=LOWER_CENTER\n",
					row_count));															
				break;
			case ANCHOR_ID_LOWER_RIGHT:
				DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow row_count=%d anchor_ID=LOWER_RIGHT\n",
					row_count)); 
				break;
			default:
				break;
		}
	para=data[4];
	column_count=para&0x3f;
	column_count+=1;
	DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow column_count=%d\n",
		window_define[window_id].column_count)); 
	para=data[5];
	window_define[window_id].pen_style=para&0x7;
	window_define[window_id].window_style = (para&0x38)>>3;
//	gx_atsc_cc_creat_window(window_id,row_count,column_count);
	window_define[window_id].row_count=row_count;
	window_define[window_id].column_count=column_count;
	
//	DRIVER_ATSC_CC_DBG(("atsc cc 708 DefineWindow pen_style=%d window_style=%d\n",
//		window_define[window_id].pen_style,window_define[window_id].window_style));
	return 0;
}

int gx_atsc_cc_708_parse(char* data, int cc_count)
{
	int i=0;
	int j=0;
	int k=0;
	int id=0;
	uint8_t cc_type=0x0;
	char* parse_data = NULL;
	char* buffer=NULL;
	uint8_t data1=0,data2=0;
	uint8_t packet_size=0;
	uint8_t packet_data_size=0;
	uint8_t sequence_number=0;
	uint8_t blocksize=0;
	uint8_t service_number=0;
	uint8_t extendedservice_number=0;
	uint8_t null_fill=0;
//	uint8_t char_num=0;
	uint8_t window_id=0;
	uint8_t para=0;
	char show_character[3]={0};
	char special_character[4]={0};
	atsc_cc_window_define_t window_old = {0};
	
	if ((NULL == data)||(0 == cc_count))
		{
//			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_708_parse para error\n"));
			return -1;
		}

	buffer = av_mallocz(cc_count/3*2+1);
	if (NULL == buffer)
		{
			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_708_parse malloc failed\n"));
			return -1;			
		}

	for (i=0; i<cc_count/3;i++)
		{
			cc_type = data[i*3];
			switch(cc_type)
				{
					case 0x00:
					case 0x01:
						break;
					case 0x02:
						break;
					case 0x03:
							{
								data1 = data[i*3+1]; // Pkt Header
								data2 = data[i*3+2]; // Pkt Data service block header
								packet_size = data1&0x3F;
								sequence_number=(data1>>6)&0x3;
//								DRIVER_ATSC_CC_DBG(("packet_size=%d sequence_number=%d\n",
//									packet_size,sequence_number));
								if (0 == packet_size)
									packet_data_size = 127;
								else
									packet_data_size = (packet_size*2)-1;
								memset(buffer,0,cc_count/3*2+1);
								buffer[0]=data2;

								blocksize = data2&0x1F;
								service_number=(data2>>5)&0x7;
								if (0x7 != service_number)
									{
										gx_atsc_cc_add(service_number+GX_ATSC_608_CC4);
									}

								if (service_number+GX_ATSC_608_CC4!=atsc_cc_para.id)
									{
//										DRIVER_ATSC_CC_ERROR(("service_number=%d atsc_cc_para.id=%d\n",
//											service_number,atsc_cc_para.id))
										i = i+(packet_data_size-1)/2;
										break;
									}

								for (k=0;k<(packet_data_size-1)/2;k++)
									{
										if (0x02 == data[(i+1)*3+k*3])
											{
												buffer[2*k+1]=data[(i+1)*3+k*3+1]; // Pkt Data
												buffer[2*k+2]=data[(i+1)*3+k*3+2]; // Pkt Data
											}
										else
											{
												DRIVER_ATSC_CC_ERROR(("data[%d]=0x03\n",i*3+k*3));
												break;
											}
									}
								parse_data = buffer;
								i = i+(packet_data_size-1)/2;
								data1 = parse_data[0];
								blocksize = data1&0x1F;
								service_number=(data1>>5)&0x7;
//								DRIVER_ATSC_CC_DBG(("blocksize=%d service_number=%d\n",
//									blocksize,service_number));

								if (0x7 == service_number)
									{
										data2 = parse_data[2]; 
										extendedservice_number = data2&0x3F;
										null_fill=(data1>>6)&0x3;
										DRIVER_ATSC_CC_DBG(("extendedservice_number=%d null_fill=%d\n",
										extendedservice_number,null_fill));
										parse_data++;
										parse_data++;										
									}
								else
									{
										parse_data++;
									}

#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
								if (NULL == spp_cc_subtitle_surface)
									{
										break;
									}
#endif

								if (0xFF != last_sequence_number[service_number])
									{
										/*
										* check sequence number error or not
										*/
										if (sequence_number == last_sequence_number[service_number])
											{
												DRIVER_ATSC_CC_ERROR(("service_number=%d  sequence_number = %d last_sequence_number[%d]=%d duplicated\n",
													service_number,sequence_number,service_number,last_sequence_number[service_number]));
											}
										else
											if (sequence_number != ((last_sequence_number[service_number]+1)%4))
												{
													gx_atsc_cc_708_clear_window(current_window_id);
													if (0 != sequence_number)
													{
														DRIVER_ATSC_CC_ERROR(("service_number=%d  sequence_number = %d last_sequence_number[%d]=%d error should discard\n",
															service_number,sequence_number,service_number,last_sequence_number[service_number]));
														last_sequence_number[service_number]=0xFF;
														av_free(buffer);
														buffer = NULL;
														return -1;
													}
												}
									}
								else
									{
										if (0 != sequence_number)
											{
												DRIVER_ATSC_CC_ERROR(("service_number=%d  sequence_number = %d  error should discard\n",service_number,sequence_number));
												av_free(buffer);
												buffer = NULL;
												return -1;
											}
									}
								last_sequence_number[service_number] = sequence_number;

								for (j=0;j<blocksize;j++)
									{
										switch(parse_data[j])
											{
												/*
												* CR/C1
												*/
												case C1_SET_CURRENT_WINDOW_0:
												case C1_SET_CURRENT_WINDOW_1:
												case C1_SET_CURRENT_WINDOW_2:
												case C1_SET_CURRENT_WINDOW_3:
												case C1_SET_CURRENT_WINDOW_4:
												case C1_SET_CURRENT_WINDOW_5:
												case C1_SET_CURRENT_WINDOW_6:
												case C1_SET_CURRENT_WINDOW_7:
													window_id = parse_data[j]-C1_SET_CURRENT_WINDOW_0;
													gx_atsc_cc_708_set_current_window(window_id);
													break;
												case C1_CLEAR_WINDOW:
													para=parse_data[j+1];
													for (id=0; id<8; id++)
														{
															if ((para>>id)&0x1)
																{
																	gx_atsc_cc_708_clear_window(id);
																}
														}
													j = j+1;
													break;
												case C1_DISPLAY_WINDOW:
													para=parse_data[j+1];
													for (id=0; id<8; id++)
														{
															if ((para>>id)&0x1)
																{
																	gx_atsc_cc_708_display_window(id);
																}
														}													
													j = j+1;
													break;
												case C1_HIDE_WINDOW:
													para=parse_data[j+1];
													for (id=0; id<8; id++)
														{
															if ((para>>id)&0x1)
																{
																	gx_atsc_cc_708_hide_window(id);
																}
														}	
													j = j+1;													
													break;
												case C1_TOGGLE_WINDOW:
													para=parse_data[j+1];
													for (id=0; id<8; id++)
														{
															if ((para>>id)&0x1)
																{
																	gx_atsc_cc_708_toggle_window(id);
																}
														}
													j = j+1;														
													break;
												case C1_DELETE_WINDOW:
													para=parse_data[j+1];
													for (id=0; id<8; id++)
														{
															if ((para>>id)&0x1)
																{
																	gx_atsc_cc_708_delete_window(id);
																}
														}
													j = j+1;														
													break;
												case C1_DELAY:
													para=parse_data[j+1];
													gx_atsc_cc_708_delay(para);
													j = j+1;														
													break;
												case C1_DELAY_CANCLE:
													gx_atsc_cc_708_delay_cancle();
													break;
												case C1_RESET:
													gx_atsc_cc_708_reset();
													break;												
												case C1_SET_PEN_ATTRIBUTES:
													if (0 == gx_atsc_cc_708_parse_pen_attributes(parse_data+j+1))
														{
															gx_atsc_cc_708_set_pen_attributes(pen_attribute);
														}
													j=j+2;
													break;
												case C1_SET_PEN_COLOR:
													if (0 == gx_atsc_cc_708_parse_pen_color(parse_data+j+1))
														{
															gx_atsc_cc_708_set_pen_color(pen_color);
														}
													j=j+3;
													break;
												case C1_SET_PEN_LOCATION:
													if ( 0 == gx_atsc_cc_708_parse_pen_location(parse_data+j+1))
														{
															gx_atsc_cc_708_set_pen_location(pen_location);															
														}
													j=j+2;
													break;
												case C1_SET_WINDOW_ATTRIBUTES:
													if (0 == gx_atsc_cc_708_parse_window_attributes(parse_data+j+1,current_window_id))
														{
															gx_atsc_cc_708_set_window_attributes(window_attribute[current_window_id],current_window_id);
														}
													j=j+4;
													break;
												case C1_DEFINE_WINDOW_0:
												case C1_DEFINE_WINDOW_1:
												case C1_DEFINE_WINDOW_2:
												case C1_DEFINE_WINDOW_3:
												case C1_DEFINE_WINDOW_4:
												case C1_DEFINE_WINDOW_5:
												case C1_DEFINE_WINDOW_6:
												case C1_DEFINE_WINDOW_7:
													window_id = parse_data[j]-C1_DEFINE_WINDOW_0;
													memcpy(&window_old,&window_define[window_id],sizeof(atsc_cc_window_define_t));
													if (0 == gx_atsc_cc_708_parse_define_window(parse_data+j+1,window_id))
														{
															gx_atsc_cc_708_define_window(window_old,window_define[window_id],window_id);
														}
													
													if (FALSE == pen_attribute.flag)
														{
															gx_atsc_cc_708_parse_pen_predefined_attributes(window_id);
															gx_atsc_cc_708_set_pen_attributes(pen_attribute);
														}
													
													if (FALSE == pen_color.flag)
														{
															gx_atsc_cc_708_parse_pen_predefined_color(window_id);
															gx_atsc_cc_708_set_pen_color(pen_color);
														}

													if (FALSE == window_attribute[window_id].flag)
														{
															gx_atsc_cc_708_parse_window_predefined_attributes(window_id);
															gx_atsc_cc_708_set_window_attributes(window_attribute[window_id],window_id);
														}
													j=j+6;
													break;
												/*
												* CL/C0
												*/
												case C0_NULL:
													/*
													* NULL
													*/
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 NULL\n"));
													gx_atsc_cc_708_c0_code_set(C0_NULL,current_window_id);
													break;
												case 0x01:
												case 0x02:
													break;
												case C0_ETX:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 ETX\n"));
													/*
													*The EndOfText command is a Null Command which can be used to flush any buffered text to the current window. All commands force a flush of any buffered text to the current window, so this command is only needed when no other command is pending.
													*/
													gx_atsc_cc_708_c0_code_set(C0_ETX,current_window_id);
													break;
												case 0x04:
												case 0x05:
												case 0x06:
												case 0x07:
													break;
												case C0_BS:
													gx_atsc_cc_708_c0_code_set(C0_BS,current_window_id);
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 BS\n"));
													break;
												case 0x09:
												case 0x0a:
												case 0x0b:
													break;
												case C0_FF:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 FF\n"));
													gx_atsc_cc_708_c0_code_set(C0_FF,current_window_id);
													/*
													* FF clears the screen and moves the pen location to (0,0)
													*/
													break;
												case C0_CR:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 CR\n"));
													gx_atsc_cc_708_c0_code_set(C0_CR,current_window_id);
													break;
												case C0_HCR:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 HCR\n"));
													/*
													* HCR moves the pen location to the beginning of the current line and deletes its contents.
													*/
													break;
												case 0x0f:
													break;
												case C0_ETX1:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 EXT1\n"));
													/*
													* When EXT1 is not encountered, then reference to the base code sets (C0, C1, G0, and G l ) is assumed. That is, EXT1
is only active for the two-byte extended code sequence in which it exists.

													EXT1 is used to escape to the 'C2', 'C3', 'G2', and 'G3' tables for the following byte

													*/
													switch(parse_data[j+1])
														{
															/*
															* CR/C2
															*/
															case 0x00:
															case 0x01:
															case 0x02:
															case 0x03:
															case 0x04:
															case 0x05:
															case 0x06:
															case 0x07:
																break;
															case 0x08:
															case 0x09:
															case 0x0a:
															case 0x0b:
															case 0x0c:
															case 0x0d:
															case 0x0e:
															case 0x0f:
																j = j+1;
																break;
															case 0x10:
															case 0x11:
															case 0x12:
															case 0x13:
															case 0x14:
															case 0x15:
															case 0x16:
															case 0x17:
																j = j+2;
																break;															
															case 0x18:
															case 0x19:
															case 0x1a:
															case 0x1b:
															case 0x1c:
															case 0x1d:
															case 0x1e:
															case 0x1f:
																j = j+3;
																break;															
															/*
															* GR/G2
															*/
															case G2_TSP:
																{
																	DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 TSP \n"));
																	gx_atsc_cc_708_g2_code_set(G2_TSP);
																}
																break;
															case G2_NBTSP:
																{
																	DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 NBTSP \n"));
																	gx_atsc_cc_708_g2_code_set(G2_NBTSP);
																}
																break;
															case 0x25:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x25 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2026, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x2A:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x2A \n"));
																//memset(special_character,0,sizeof(special_character));
																//gx_atsc_cc_unicode_to_utf8_one(0x2026, special_character);
																//gx_atsc_cc_draw_character("Å ",current_window_id);
																break;
															case 0x2C:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x2C \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x152, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x30:
																{
																	DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x30 \n"));
																	/*
																	* The ¨€ character (30h) is a solid block which fills the entire character position with the text foreground color.
																	*/
																	gx_atsc_cc_708_g2_code_set(G2_30);
																	}
																break;
															case 0x31:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x31 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2018, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("¡®",current_window_id);	
																break;
															case 0x32:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x32 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2019, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("¡¯",current_window_id);	
																break;
															case 0x33:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x33 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x201c, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("¡°",current_window_id);	
																break;
															case 0x34:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x34 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x201d, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("¡±",current_window_id);	
																break;
															case 0x35:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x35 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2022, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x39:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x39 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2122, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x3a:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x3a \n"));
																//memset(special_character,0,sizeof(special_character));
																//gx_atsc_cc_unicode_to_utf8_one(0x152, special_character);
																//gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("Å¡",current_window_id);
																break;
															case 0x3c:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x3c \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x153, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("Å“",current_window_id);
																break;
															case 0x3d:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x3d \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2126, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x3f:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x3f \n"));
																//memset(special_character,0,sizeof(special_character));
																//gx_atsc_cc_unicode_to_utf8_one(0x2126, special_character);
																//gx_atsc_cc_draw_character(special_character,current_window_id);
																//gx_atsc_cc_draw_character("Å¸",current_window_id);
																break;
															case 0x76:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x76 \n"));														
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x215b, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;															
															case 0x77:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x77 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x215c, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;	
															case 0x78:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x78 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x215d, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;	
															case 0x79:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x79 \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x215e, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x7a:																
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7a \n"));		
																break;
															case 0x7b:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7b \n"));	
																break;
															case 0x7c:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7c \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x2524, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															case 0x7d:															
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7d \n"));
																break;
															case 0x7e:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7e \n"));
																break;
															case 0x7f:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G2 0x7f \n"));
																memset(special_character,0,sizeof(special_character));
																gx_atsc_cc_unicode_to_utf8_one(0x251c, special_character);
																gx_atsc_cc_draw_character(special_character,current_window_id);
																break;
															/*
															* CR/C3
															*/
															case 0x80:
															case 0x81:
															case 0x82:
															case 0x83:
															case 0x84:
															case 0x85:
															case 0x86:
															case 0x87:
																j =j+4;
																break;
															case 0x88:
															case 0x89:
															case 0x8a:
															case 0x8b:
															case 0x8c:
															case 0x8d:
															case 0x8e:
															case 0x8f:
																j =j+5;
																break;															
																/*
																* GR/G3
																*/
															case 0xa0:
																DRIVER_ATSC_CC_DBG(("atsc cc 708 G3 0xa0 \n"));
																break;
															default:
																break;
														}
													j = j+1;
													break;
												case 0x11:
												case 0x12:
												case 0x13:
												case 0x14:
												case 0x15:
												case 0x16:
												case 0x17:
													j = j+1;													
													break;
												case C0_P16:
//													DRIVER_ATSC_CC_DBG(("atsc cc 708 C0 P16\n"));
													/*
													* P16 can be used to escape the next two bytes for Chinese and other large character maps
													*/
													memset(show_character,0,sizeof(show_character));
													if (parse_data[j+1]>0)
														show_character[0]=parse_data[j+1];
													if (parse_data[j+2]>0)
														{
															if (strlen(show_character)>0)
																{
																	show_character[1]=parse_data[j+2];
																}
															else
																{
																	show_character[0]=parse_data[j+1];
																}
														}
													if (strlen(show_character)>0)
														gx_atsc_cc_draw_character(show_character,current_window_id);
													j = j+2;
													break;
												case 0x19:
												case 0x1a:
												case 0x1b:
												case 0x1c:
												case 0x1d:
												case 0x1e:
												case 0x1f:
													j = j+2;
													break;
													/*
													* GR/G0
													*/
												case 0x7f:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 G0 MN\n"));
													memset(special_character,0,sizeof(special_character));
													gx_atsc_cc_unicode_to_utf8_one(0x266a, special_character);
													gx_atsc_cc_draw_character(special_character,current_window_id);
													/*
													* MN is a musical note, which replaces the Delete command code in ASCII.
													*/
													break;
												case 0xa0:
													DRIVER_ATSC_CC_DBG(("atsc cc 708 G1 NBS\n"));
													/*
													* GR/G1
													*/
													/*Code NBS (A0h) represents a non-breaking space. This code is to be used (instead of a space character) between
words that should not be split when word-wrapping is in effect.*/
													break;
												default:
													if ((parse_data[j]>=0x20) && (parse_data[j]<=0x7f))
														{
															memset(show_character,0,sizeof(show_character));
															show_character[0]=parse_data[j];
															gx_atsc_cc_draw_character(show_character,current_window_id);
//															DRIVER_ATSC_CC_DBG(("%c\n",parse_data[j]));
														}
													else if((parse_data[j]>=0x80) && (parse_data[j]<=0xff))
													{
															memset(show_character,0,sizeof(show_character));
															gx_atsc_cc_unicode_to_utf8_one(parse_data[j], show_character);
															gx_atsc_cc_draw_character(show_character,current_window_id);
													}
													
													break;												
											}
									}
						}
						break;
					default:
						break;
				}
			
		}

//	DRIVER_ATSC_CC_DBG(("strlen(cc_708_char) = %d ,cc_708_char = %s\n",strlen(cc_708_char),cc_708_char));		
//	DRIVER_ATSC_CC_DBG(("\n"));		

	av_free(buffer);
	buffer = NULL;
	return 0;
}




