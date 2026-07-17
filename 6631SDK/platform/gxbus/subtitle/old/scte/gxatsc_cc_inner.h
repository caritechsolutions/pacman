/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.02.01		  zhouhm 			 creation
*****************************************************************************/

#ifndef __GXATSC_CC_INNER__H__
#define __GXATSC_CC_INNER__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>
#include "gx_cc_subtitle_common.h"
#include "gx_atsc_cc.h"

#define DRIVER_ATSC_CC_ERROR(debug)\
 { \
     gxloge("[ATSC_CC ERROR]%s %d",__FILE__,__LINE__);\
     gxloge debug; \
 } 

 #define DRIVER_ATSC_CC_DBG(debug)\
     do{\
         }while(0)

typedef enum {
     CC_OFF = 0, 
     CC_ON
}CC_STATUS;

typedef enum {
     PEN_SMALL = 0, /*小号字体*/
     PEN_STANDARD ,  /*标准字体*/
     PEN_LARGE,      /*大号字体*/
     PEN_INVALID
}CC_PEN_SIZE;

typedef enum {
     OFFSET_SUBSCRIPT = 0, /*下标，脚注*/
     OFFSET_NORMAL ,      
     OFFSET_SUPERSCRIPT,   /* 上角文字，上标 */ 
     OFFSET_INVALID
}CC_PEN_OFFSET;

typedef enum {
     TEXT_TAG_DIALOG = 0, 
     TEXT_TAG_SOURCE_OR_SPEAKER_ID,
     TEXT_TAG_ELECTRONICALLY_REPRODUCED_VOICE, 
     TEXT_TAG_DIALOG_IN_OTHER_LANGUAGE,
     TEXT_TAG_VOICEOVER,
     TEXT_TAG_AUDIBLE_TRANSLATION,
     TEXT_TAG_SUBTITLE_TRANSLATION,
     TEXT_TAG_VOICE_QUALITY_DESCRIPTION,
     TEXT_TAG_SONG_LYRICS,
     TEXT_TAG_SONG_EFFECT_DESCRIPTION,
     TEXT_TAG_MUSICAL_SCORE_DESCRIPTION,
	 TEXT_TAG_OATH,	 
	 TEXT_TAG_UNDEFINED_0,
	 TEXT_TAG_UNDEFINED_1,
	 TEXT_TAG_UNDEFINED_2,
	 TEXT_TAG_INVISIBLE,
     TEXT_TAG_INVALID
}CC_TEXT_TAG;

typedef enum {
     FONT_TAG_DEFAULT = 0, 
	 FONT_TAG_MONOSPACED_SERIF, /*等宽字体*/
	 FONT_TAG_PROPORTIONAL_SERIF, /*比例字体*/
	 FONT_TAG_MONOSPACED_SANSERIF, /*等宽无衬线字体*/
	 FONT_TAG_PROPORTIONAL_SANSERIF, /*比例无衬线字体*/
	 FONT_TAG_CASUAL, /*临时的*/
	 FONT_TAG_CURSIVE, /*草书*/
	 FONT_TAG_SMALL_CAPS, /*小型大写字母*/
     FONT_TAG_INVALID
}CC_FONT_TAG;

typedef enum {
     EDGE_TYPE_NONE = 0, 
	 EDGE_TYPE_RAISED, /*凸起/浮雕*/
	 EDGE_TYPE_DEPRESSED, /*压下*/ 
	 EDGE_TYPE_UNIFORM,
	 EDGE_TYPE_LEFT_DROP_SHADOW, /*左阴影*/
	 EDGE_TYPE_RIGHT_DROP_SHADOW, /*右阴影*/
	 EDGE_TYPE_ILLEGAL_VAL0, 
	 EDGE_TYPE_ILLEGAL_VAL1, 
     EDGE_TYPE_INVALID
}CC_EDGE_TYPE;

typedef enum {
     ANCHOR_ID_UPPER_LEFT = 0,
	 ANCHOR_ID_UPPER_CENTER,
	 ANCHOR_ID_UPPER_RIGHT,	 
	 ANCHOR_ID_MIDDLE_LEFT,
	 ANCHOR_ID_MIDDLE_CENTER,
	 ANCHOR_ID_MIDDLE_RIGHT,
	 ANCHOR_ID_LOWER_LEFT,
	 ANCHOR_ID_LOWER_CENTER,
	 ANCHOR_ID_LOWER_RIGHT,
     ANCHOR_ID_INVALID
}CC_ANCHOR_ID;

typedef enum {
     BORAD_TYPE_NONE = 0,
	 BORAD_TYPE_RAISED, /*凸起/浮雕*/
	 BORAD_TYPE_DEPRESSED, /*压下*/ 
	 BORAD_TYPE_UNIFORM,
	 BORAD_TYPE_SHADOW_LEFT, /*左阴影*/
	 BORAD_TYPE_SHADOW_RIGHT, /*右阴影*/
     BORAD_TYPE_INVALID
}CC_BORAD_TYPE;

typedef enum {
     LEFT_TO_RIGHT = 0,
	 RIGHT_TO_LEFT, 
	 TOP_TO_BOTTOM, 
	 BOTTOM_TO_TOP,
     DIRECTION_INVALID
}CC_DIRECTION;

typedef enum {
     JUSTIFY_LEFT = 0,
	 JUSTIFY_RIGHT, 
	 JUSTIFY_TOP, 
	 JUSTIFY_BOTTOM,
	 JUSTIFY_CENTER,
	 JUSTIFY_FULL,
     JUSTIFY_INVALID
}CC_JUSTIFY;

typedef enum {
     DISPLAY_EFFECT_SNAP = 0,
	 DISPLAY_EFFECT_FADE, /*淡入淡出*/
	 DISPLAY_EFFECT_WIPE, /*飞入飞出*/
     DISPLAY_EFFECT_INVALID
}CC_DISPLAY_EFFECT;

typedef enum {
     ROLL_UP_STYLE, /*滚动风格*/
     PAINT_ON_STYLE, /*绘画风格*/
 	 POP_ON_STYLE, /*弹出风格*/
     STYLE_INVALID
}PROPER_ORDER_OF_DATA_T;

typedef enum {
	 MISCELLANEOUS_RCL=0, // Resume caption loading
	 MISCELLANEOUS_BS, // Backspace
	 MISCELLANEOUS_AOF, // Reserved (formerly Alarm Off)
	 MISCELLANEOUS_AON, // Reserved (formerly Alarm On)
	 MISCELLANEOUS_DER, // Delete to End of Row
	 MISCELLANEOUS_RU2, // Roll-Up Captions-2 Rows
	 MISCELLANEOUS_RU3, // Roll-Up Captions-3 Rows
	 MISCELLANEOUS_RU4, // Roll-Up Captions-4 Rows
	 MISCELLANEOUS_FON, // Flash On
	 MISCELLANEOUS_RDC, // Resume Direct Captioning
	 MISCELLANEOUS_TR, // Text Restart
	 MISCELLANEOUS_RTD, // Resume Text Display
	 MISCELLANEOUS_EDM, // Erase Displayed Memory
	 MISCELLANEOUS_CR, // Carriage Return
	 MISCELLANEOUS_ENM, // Erase Non-Displayed Memory
	 MISCELLANEOUS_EOC, // End of Caption (Flip Memories
	 MISCELLANEOUS_TO1, // Tab Offset 1 Column
	 MISCELLANEOUS_TO2, // Tab Offset 2 Columns
	 MISCELLANEOUS_TO3, // Tab Offset 3 Columns
     MISCELLANEOUS_INVALID
}MISCELLANEOUS_CONTROL_CODE_T;

typedef enum {
	 PREAMBLE_ROW_1=0, // Row 1
	 PREAMBLE_ROW_2, // Row 2
	 PREAMBLE_ROW_3, // Row 3
	 PREAMBLE_ROW_4, // Row 4
	 PREAMBLE_ROW_5, // Row 5
	 PREAMBLE_ROW_6, // Row 6
	 PREAMBLE_ROW_7, // Row 7
	 PREAMBLE_ROW_8, // Row 8
	 PREAMBLE_ROW_9, // Row 9
	 PREAMBLE_ROW_10, // Row 10
	 PREAMBLE_ROW_11, // Row 11
	 PREAMBLE_ROW_12, // Row 12
	 PREAMBLE_ROW_13, // Row 13
	 PREAMBLE_ROW_14, // Row 14
	 PREAMBLE_ROW_15, // Row 15
     PREAMBLE_FIRST_INVALID
}PREAMBLE_ADDRESS_CODES_FIRST_T;

typedef enum {
	 PREAMBLE_WHITE=0,
	 PREAMBLE_WHITE_UNDERLINE,
	 PREAMBLE_GREEN,
	 PREAMBLE_GREEN_UNDERLINE,
	 PREAMBLE_BLUE,
	 PREAMBLE_BLUE_UNDERLINE,
	 PREAMBLE_CYAN,
	 PREAMBLE_CYAN_UNDERLINE,
	 PREAMBLE_RED,
	 PREAMBLE_RED_UNDERLINE,
	 PREAMBLE_YELLOW,
	 PREAMBLE_YELLOW_UNDERLINE,
	 PREAMBLE_MAGENTA,
	 PREAMBLE_MAGENTA_UNDERLINE,
	 PREAMBLE_WHITE_ITALICS,
	 PREAMBLE_WHITE_ITALICS_UNDERLINE,
	 PREAMBLE_INDENT_0,
	 PREAMBLE_INDENT_0_UNDERLINE,
	 PREAMBLE_INDENT_4,
	 PREAMBLE_INDENT_4_UNDERLINE,
	 PREAMBLE_INDENT_8,
	 PREAMBLE_INDENT_8_UNDERLINE,
	 PREAMBLE_INDENT_12,
	 PREAMBLE_INDENT_12_UNDERLINE,
	 PREAMBLE_INDENT_16,
	 PREAMBLE_INDENT_16_UNDERLINE,
	 PREAMBLE_INDENT_20,
	 PREAMBLE_INDENT_20_UNDERLINE,
	 PREAMBLE_INDENT_24,
	 PREAMBLE_INDENT_24_UNDERLINE,
	 PREAMBLE_INDENT_28,
	 PREAMBLE_INDENT_28_UNDERLINE,
     PREAMBLE_SECOND_INVALID
}PREAMBLE_ADDRESS_CODES_SECOND_T;

typedef enum {
	 BACKGROUND_WHITE_OPAQUE=0,
	 BACKGROUND_WHITE_SEMI_TRANSPARENT,
	 BACKGROUND_GREEN_OPAQUE,
	 BACKGROUND_GREEN_SEMI_TRANSPARENT,
	 BACKGROUND_BLUE_OPAQUE,
	 BACKGROUND_BLUE_SEMI_TRANSPARENT,
	 BACKGROUND_CYAN_OPAQUE,
	 BACKGROUND_CYAN_SEMI_TRANSPARENT,
	 BACKGROUND_RED_OPAQUE,
	 BACKGROUND_RED_SEMI_TRANSPARENT,
	 BACKGROUND_YELLOW_OPAQUE,
	 BACKGROUND_YELLOW_SEMI_TRANSPARENT,
	 BACKGROUND_MAGENTA_OPAQUE,
	 BACKGROUND_MAGENTA_SEMI_TRANSPARENT,
	 BACKGROUND_BLACK_OPAQUE,
	 BACKGROUND_BLACK_SEMI_TRANSPARENT,
	 BACKGROUND_TRANSPARENT,
	 FOREGROUND_BLACK,
	 FOREGROUND_BLACK_UNDERLINE,
     BACK_FORE_GROUND_INVALID
}BACK_FORE_GROUND_CODES_T;


typedef enum {
	 MID_WHITE=0,
	 MID_WHITE_UNDERLINE,
	 MID_GREEN,
	 MID_GREEN_UNDERLINE,
	 MID_BLUE,
	 MID_BLUE_UNDERLINE,
	 MID_CYAN,
	 MID_CYAN_UNDERLINE,
	 MID_RED,
	 MID_RED_UNDERLINE,
	 MID_YELLOW,
	 MID_YELLOW_UNDERLINE,
	 MID_MAGENTA,
	 MID_MAGENTA_UNDERLINE,
	 MID_ITALICS,
	 MID_ITALICS_UNDERLINE,
     MID_INVALID
}MID_ROW_CODES_T;


typedef struct atsc_cc_pen_attribute_s
{
	uint8_t flag;
	CC_PEN_SIZE pen_size;
	CC_PEN_OFFSET pen_offset;
	CC_TEXT_TAG text_tag;
	CC_FONT_TAG font_tag;
	CC_EDGE_TYPE edge_type;
	uint8_t italic;
	uint8_t underline;
} atsc_cc_pen_attribute_t;

typedef struct atsc_cc_pen_color_s
{
	uint8_t flag;
    COLOR_TYPE background_color; 
	COLOR_TYPE foreground_color; 
	COLOR_TYPE edge_color;
	OPACITY_TYPE background_opacity;
	OPACITY_TYPE foreground_opacity;	
} atsc_cc_pen_color_t;

typedef struct atsc_cc_pen_location_s
{
	uint8_t row;
	uint8_t column;
} atsc_cc_pen_location_t;

typedef struct atsc_cc_window_define_s
{
	uint8_t flag;
	uint8_t display_priority;
	uint8_t column_lock;
	uint8_t row_lock;
	uint8_t visible;
	uint8_t anchor_vertical;
	uint8_t relative_positioning; /*相对定位(百分比)*/
	uint8_t anchor_horizontal;
	uint16_t row_count;
	CC_ANCHOR_ID anchor_ID;
	uint16_t column_count;
	uint8_t pen_style;
	uint8_t window_style;
} atsc_cc_window_define_t;

typedef struct atsc_cc_window_attribute_s
{
	uint8_t flag;
	COLOR_TYPE fill_color;
	OPACITY_TYPE fill_opacity;
	COLOR_TYPE border_color;
	CC_BORAD_TYPE border_type;
	CC_DIRECTION scroll_direction;
	CC_DIRECTION print_direction;
	CC_JUSTIFY justify;
	uint8_t word_wrap;
	CC_DISPLAY_EFFECT display_effect;
	CC_DIRECTION effect_direction;
	uint8_t effect_speed; /*单位:0.5秒*/
} atsc_cc_window_attribute_t;

/*
* atsc cc 708
*/

typedef enum {
     C0_NULL = 0X00,
	 C0_ETX = 0X03,
     C0_BS = 0X08, 
     C0_FF = 0X0C,
     C0_CR = 0X0D,
     C0_HCR = 0X0E,
     C0_ETX1 = 0X10,
     C0_P16 = 0X18,
	 C0_INVALID
}C0_CODE_SET_T;

typedef enum {
     G2_TSP = 0x20,
	 G2_NBTSP = 0x21,
	 G2_30 = 0x30,
	 G2_INVALID
}G2_CODE_SET_T;

#define C1_SET_CURRENT_WINDOW_0 (0X80)
#define C1_SET_CURRENT_WINDOW_1 (0X81)
#define C1_SET_CURRENT_WINDOW_2 (0X82)
#define C1_SET_CURRENT_WINDOW_3 (0X83)
#define C1_SET_CURRENT_WINDOW_4 (0X84)
#define C1_SET_CURRENT_WINDOW_5 (0X85)
#define C1_SET_CURRENT_WINDOW_6 (0X86)
#define C1_SET_CURRENT_WINDOW_7 (0X87)
#define C1_CLEAR_WINDOW (0X88)
#define C1_DISPLAY_WINDOW (0X89)
#define C1_HIDE_WINDOW (0X8a)
#define C1_TOGGLE_WINDOW (0x8b)
#define C1_DELETE_WINDOW (0x8c)
#define C1_DELAY (0x8d)
#define C1_DELAY_CANCLE (0x8e)
#define C1_RESET (0x8f)
#define C1_SET_PEN_ATTRIBUTES (0x90)
#define C1_SET_PEN_COLOR (0x91)
#define C1_SET_PEN_LOCATION (0x92)
#define C1_SET_WINDOW_ATTRIBUTES (0x97)
#define C1_DEFINE_WINDOW_0 (0x98)
#define C1_DEFINE_WINDOW_1 (0x99)
#define C1_DEFINE_WINDOW_2 (0x9a)
#define C1_DEFINE_WINDOW_3 (0x9b)
#define C1_DEFINE_WINDOW_4 (0x9c)
#define C1_DEFINE_WINDOW_5 (0x9d)
#define C1_DEFINE_WINDOW_6 (0x9e)
#define C1_DEFINE_WINDOW_7 (0x9f)


#define ROW_MAX (210)
#define CLOUMN_MAX (74)
#define CC_BUFFER_MAX (16*1024)
#define CC_608_UNIT_MAX (100)
#define CC_WINDOW_MAX (8)
#define CC_708_UNIT_MAX (100)

#define GRID_708_VERTIAL_MAX (75)
#define GRID_608_VERTIAL_MAX (15)
#define GRID_608_HORIZONTAL_16_9_MAX (/*32*/210)
#define GRID_708_HORIZONTAL_16_9_MAX (210)
#define GRID_HORIZONTAL_4_3_MAX (160)

extern GxAtsccc_Param atsc_cc_para;
extern atsc_cc_window_define_t window_define[CC_WINDOW_MAX];
extern atsc_cc_window_attribute_t window_attribute[CC_WINDOW_MAX];
extern atsc_cc_pen_attribute_t pen_attribute;
extern atsc_cc_pen_color_t pen_color;
extern atsc_cc_pen_location_t pen_location;
extern char last_unit_data[3];
extern char last_char[3];
extern row_para_t cc_row_para[CC_WINDOW_MAX][ROW_MAX];
extern PROPER_ORDER_OF_DATA_T proper_order;






#ifdef __cplusplus
}
#endif
#endif /*__GXATSC_CC_INNER__H__*/

