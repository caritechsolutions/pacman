/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.03.05		  zhouhm 			 creation
*****************************************************************************/

#ifndef __GXSCTE_SUBTITLE_INNER__H__
#define __GXSCTE_SUBTITLE_INNER__H__
#ifdef __cplusplus
extern "C" {
#endif
#include <gxtype.h>
#include "gx_cc_subtitle_common.h"
#include "gx_scte_subtitle.h"


#define TABLE_EXTENSION_MAX (20)

typedef enum {
     SUB_OFF = 0, 
     SUB_ON
}SUB_STATUS;

typedef enum {
     DISPLAY_STANDARD_720_480_30 = 0,       
	 DISPLAY_STANDARD_720_576_25,			
	 DISPLAY_STANDARD_1280_720_60, 
	 DISPLAY_STANDARD_1920_1080_60,      
     DISPLAY_STANDARD_RESERVED
}SCTE_SUB_DISPLAY_STANDARD;

typedef enum {
     BACKGROUND_STYLE_TRANSPARENT = 0,       
	 BACKGROUND_STYLE_FRAMED			
}SCTE_SUB_BACKGROUND_STYLE;


typedef enum {
     OUTLINE_STYLE_NONE = 0,       
	 OUTLINE_STYLE_OUTLINE,
	 OUTLINE_STYLE_DROP_SHADOW,
	 OUTLINE_STYLE_RESERVED
}SCTE_SUB_OUTLINE_STYLE;


typedef enum {
     PRE_CLEAR_DISPLAY = 0,       
	 IMMEDIATE
}SCTE_SUB_DISPLAY_MODES;

typedef enum {       
     SUB_TYPE_SIMPLE_BITMAP=0,
     SUB_TYPE_REVERSE,
}SCTE_SUB_TYPE;

typedef enum {       
     SUB_OPERATOR_NOOP=0,
     SUB_OPERATOR_EOL,
     SUB_OPERATOR_RESERVED
}SCTE_SUB_OPERATOR;

typedef struct scte_sub_coordinate_s
{
	uint16_t top_H_coordinate;
	uint16_t top_V_coordinate;
	uint16_t bottom_H_coordinate;
	uint16_t bottom_V_coordinate;
} scte_sub_coordinate_t;

typedef struct scte_sub_color_s
{
	uint8_t Y_component;
	uint8_t opaque_enable;
	uint8_t Cr_component;
	uint8_t Cb_component;
} scte_sub_color_t;

typedef struct scte_sub_frame_s
{
	scte_sub_coordinate_t frame_coordinate;
	scte_sub_color_t frame_color;
} scte_sub_frame_t;

typedef struct scte_sub_outline_s
{
	uint8_t outline_thickness;
	scte_sub_color_t outline_color; 
} scte_sub_outline_t;

typedef struct scte_sub_shadow_s
{
	uint8_t shadow_right;
	uint8_t shadow_bottom;
	scte_sub_color_t shadow_color;
} scte_sub_shadow_t;

typedef struct scte_sub_segment_s
{
	uint8_t recieved;
	uint16_t compressed_bitmap_len;
} scte_sub_segment_t;


typedef struct scte_sub_simple_bitmap_s
{
	uint8_t status;
	uint8_t segmentation_overlay_included;
	uint8_t protocol_version;
	uint16_t table_extension;
	uint8_t last_segment_number;
	uint8_t segment_recieve_finish;
	char ISO_639_language_code[4];
	SCTE_SUB_DISPLAY_MODES display_modes;
	SCTE_SUB_DISPLAY_STANDARD display_standard;
	int64_t display_in_PTS;
	SCTE_SUB_TYPE sub_type;
	uint32_t display_duration;
	int32_t display_start_ms;
	uint8_t block_length;
	uint8_t display_status;
	/*
	* header
	*/
	SCTE_SUB_BACKGROUND_STYLE background_style;
	SCTE_SUB_OUTLINE_STYLE outline_style;
	scte_sub_color_t character_color;
	scte_sub_coordinate_t bitmap_coordinate;
	scte_sub_frame_t frame;
	scte_sub_outline_t outline;
	scte_sub_shadow_t shadow;
	uint16_t bitmap_length;
	char* data;
	scte_sub_segment_t *segment;
} scte_sub_simple_bitmap_t;


typedef struct scte_sub_compressed_bitmap_s
{
	uint8_t steering1;
	uint8_t steering2;
	uint8_t steering3;	
	SCTE_SUB_OPERATOR operator;
	uint8_t run_length_16_on;
	uint8_t run_length_64_off;
	uint8_t run_length_8_on;
	uint8_t run_length_32_off;
} scte_sub_compressed_bitmap_t;


#ifdef __cplusplus
}
#endif
#endif /*__GXSCTE_SUBTITLE_INNER__H__*/

