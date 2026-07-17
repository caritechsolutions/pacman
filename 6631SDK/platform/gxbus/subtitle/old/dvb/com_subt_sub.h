/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2008, All right reserved
******************************************************************************

******************************************************************************
* File Name :	com_subt_sub.h
* Author    : 	brucechow
* Project   :	GX6102 
* Type      :	
******************************************************************************
* Purpose   :	Flash node management interface error enum define
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2008.06.30	     brucechow	         creation
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __COM_SUBT_SUB_H__
#define __COM_SUBT_SUB_H__

/* Includes --------------------------------------------------------------- */
#include "com_sub_def.h"

/* Cooperation with C and C++ */
#ifdef __cplusplus
extern "C" {
#endif

extern u8 g_chSubtEnable;

extern u16 g_wSubtitleCompositionPageId;
extern u16 g_wSubtitleAncillaryPageId;
extern u16 g_wClutTable[256];
/* Exported Constants ----------------------------------------------------- */
#define PES_PACK_BUF_MAX_LEN        20000
#define COMPOSITION_BUF_MAX_LEN     10140
#define OBJECT_BUF_MAX_LEN          100000
#define PIXEL_BUF_MAX_LEN           200000
#define PAGE_BUF_MAX_LEN            100

//+++++++++数据类型+++++++++++segment_type: 表明字段的属性
#define	DT_PAGE_COMPOSITION_SEGMENT                      0x10
#define	DT_REGION_COMPOSITION_SEGMENT                  0x11
#define	DT_CLUT_DEFINITION_SEGMENT                         0x12
#define	DT_OBJECT_DATA_SEGMENT                                0x13
#define	DT_DISPLAY_DEFINITION_SEGMENT                  0x14
#define	DT_END_OF_THE_DISPLAY_SEGMENT                  0x80
#define	DT_STUFFING_SEGMENT                                    0xFF
//*********************************

//+++++++++解码状态+++++++++
#define	ST_WAIT_MODE_CHANGE							0
#define	ST_SUBTITLE_MODE_CHANGE_PROCESS			1
#define	ST_WAIT_PTS								       2//播出时间标记	
#define	ST_RESERVED_PROCESS						3
#define	ST_MAIN_PROCESS								4
#define	ST_SUBTITLE_ACQUISITION_POINT_PROCESS		5
#define	ST_SUBTITLE_NORML_CASE_PROCESS					       6
//+++++++++页类型+++++++++++
#define	PG_NORMAL_CASE_PAGE_UPDATE 0
#define     PG_ACQUISITION_POINT 1
#define     PG_MODE_CHANGE 2
#define     PG_RESERVED 3

#define pes_packet_head__stream_id(ts_data_ptr) (*((u8*)ts_data_ptr + 3))
#define pes_packet_head__PES_packet_length_h(ts_data_ptr) (*((u8*)ts_data_ptr + 4))
#define pes_packet_head__PES_packet_length_l(ts_data_ptr) (*((u8*)ts_data_ptr + 5))
#define pes_packet_head__PES_header_data_length(pes_buffer) (*((u8*)((u8*)pes_buffer + 8)))

//pts
#define pes_packet_head__PTS_H(ts_data_ptr)  ((*((u8*)ts_data_ptr +  9))&0x0E)
#define pes_packet_head__PTS_MH(ts_data_ptr) (*((u8*)ts_data_ptr +  10))
#define pes_packet_head__PTS_ML(ts_data_ptr) ((*((u8*)ts_data_ptr + 11))&0xFE)
#define pes_packet_head__PTS_LH(ts_data_ptr) (*((u8*)ts_data_ptr +  12))
#define pes_packet_head__PTS_LL(ts_data_ptr) ((*((u8*)ts_data_ptr + 13))&0xFE)

//subtitle
#define PES_data_field__data_identifier(pes_data_ptr)	(*((u8*)pes_data_ptr))
#define PES_data_field__subtitle_stream_id(pes_data_ptr)	(*((u8*)pes_data_ptr+1))
#define PES_data_field__subtitling_segment_ptr(pes_data_ptr)	((u8*)pes_data_ptr+2)

//subtitle segment
#define subtital_segment__sync_byte(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr))
#define subtital_segment__segment_type(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr+1))
#define subtital_segment__page_id_h(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr+2))
#define subtital_segment__page_id_l(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr+3))
#define subtital_segment__segment_length_h(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr+4))
#define subtital_segment__segment_length_l(subtitling_segment_ptr)	(*((u8*)subtitling_segment_ptr+5))
#define subtital_segment__segment_data_field_ptr(subtitling_segment_ptr)	((u8*)subtitling_segment_ptr+6)

//Display definition page
#define display_definition_segment__dds_version_number(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr))>>4)
#define display_definition_segment__display_window_flag(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr))>>3)&0x1)

#define display_definition_segment__display_width_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +1))
#define display_definition_segment__display_width_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +2))
#define display_definition_segment__display_height_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +3))
#define display_definition_segment__display_height_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +4))

#define display_definition_segment__display_window_horizontal_min_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +5))
#define display_definition_segment__display_window_horizontal_min_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +6))
#define display_definition_segment__display_window_horizontal_max_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +7))
#define display_definition_segment__display_window_horizontal_max_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +8))
#define display_definition_segment__display_window_vertical_min_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +9))
#define display_definition_segment__display_window_vertical_min_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +10))
#define display_definition_segment__display_window_vertical_max_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +11))
#define display_definition_segment__display_window_vertical_max_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +12))

//subtitle page
#define page_composition_segment__page_time_out(segment_data_field_ptr) (*((u8*)segment_data_field_ptr))
#define page_composition_segment__page_version_number(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr +1))>>4)
#define page_composition_segment__page_state(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr+1))>>2)&0x3)
#define page_composition_segment__region_data_field_start_ptr(segment_data_field_ptr) ((u8*)segment_data_field_ptr + 2)

#define page_composition_segment__region_id(region_data_field_ptr) (*((u8*)region_data_field_ptr))
#define page_composition_segment__region_horizontal_address_h(region_data_field_ptr) (*((u8*)region_data_field_ptr + 2))
#define page_composition_segment__region_horizontal_address_l(region_data_field_ptr) (*((u8*)region_data_field_ptr + 3))
#define page_composition_segment__region_vertical_address_h(region_data_field_ptr) (*((u8*)region_data_field_ptr +4))
#define page_composition_segment__region_vertical_address_l(region_data_field_ptr) (*((u8*)region_data_field_ptr +5))

//subtitle region
#define region_compostion_segment__region_id(segment_data_field_ptr) (*((u8*)segment_data_field_ptr))
#define region_compostion_segment__region_version_number(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr + 1))>>4)//移位后高位补零
#define region_compostion_segment__region_fill_flag(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr + 1))>>3)&0x1)
#define region_compostion_segment__region_width_h(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 2))
#define region_compostion_segment__region_width_l(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 3))
#define region_compostion_segment__region_height_h(segment_data_field_ptr) (*((u8*)segment_data_field_ptr +4))
#define region_compostion_segment__region_height_l(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 5))
#define region_compostion_segment__region_level_of_compatibility(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr + 6))>>5)&0x7)
#define region_compostion_segment__region_depth(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr + 6))>>2)&0x7)
#define region_compostion_segment__CLUT_id(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 7))
#define region_compostion_segment__region_8bit_pixel_code(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 8))
#define region_compostion_segment__region_4bit_pixel_code(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr + 9))>>4)
#define region_compostion_segment__region_2bit_pixel_code(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr + 9))>>2)
#define region_compostion_segment__object_data_field_start_ptr(segment_data_field_ptr) ((u8*)segment_data_field_ptr + 10)

#define region_compostion_segment__object_id_h(object_data_field_ptr) (*((u8*)object_data_field_ptr ))
#define region_compostion_segment__object_id_l(object_data_field_ptr) (*((u8*)object_data_field_ptr +1))
#define region_compostion_segment__object_type(object_data_field_ptr) ((*((u8*)object_data_field_ptr +2))>>6)
#define region_compostion_segment__object_provider_flag(object_data_field_ptr) (((*((u8*)object_data_field_ptr +2))>>4)&0x3)
#define region_compostion_segment__object_horizontal_position_h(object_data_field_ptr) ((*((u8*)object_data_field_ptr +2))&0xf)
#define region_compostion_segment__object_horizontal_position_l(object_data_field_ptr) (*((u8*)object_data_field_ptr +3))
#define region_compostion_segment__object_vertical_postion_h(object_data_field_ptr) ((*((u8*)object_data_field_ptr +4))&0xf)
#define region_compostion_segment__object_vertical_postion_l(object_data_field_ptr) (*((u8*)object_data_field_ptr +5))
#define region_compostion_segment__foreground_pixel_code(object_data_field_ptr) (*((u8*)object_data_field_ptr +6))
#define region_compostion_segment__background_pixel_code(object_data_field_ptr) (*((u8*)object_data_field_ptr +7))

//subtitle clut
#define CLUT_definition_segment__CLUT_id(segment_data_field_ptr) (*((u8*)segment_data_field_ptr))
#define CLUT_definition_segment__CLUT_version_number(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 1))
#define CLUT_definition_segment__CLUT_data_field_start_ptr(segment_data_field_ptr) ((u8*)segment_data_field_ptr + 2)

#define CLUT_definition_segment__CLUT_entry_id(CLUT_data_field_ptr) (*((u8*)CLUT_data_field_ptr))
#define CLUT_definition_segment__2bit_entry_CLUT_flag(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 1))>>7)
#define CLUT_definition_segment__4bit_entry_CLUT_flag(CLUT_data_field_ptr) (((*((u8*)CLUT_data_field_ptr + 1))>>6) & 0x1)
#define CLUT_definition_segment__8bit_entry_CLUT_flag(CLUT_data_field_ptr) (((*((u8*)CLUT_data_field_ptr + 1))>>5) & 0x1)
#define CLUT_definition_segment__full_range_flag(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 1))&0x1)
#define CLUT_definition_segment__Y_value_flag1(CLUT_data_field_ptr) (*((u8*)CLUT_data_field_ptr + 2))
#define CLUT_definition_segment__Cr_value_flag1(CLUT_data_field_ptr) (*((u8*)CLUT_data_field_ptr + 3))
#define CLUT_definition_segment__Cb_value_flag1(CLUT_data_field_ptr) (*((u8*)CLUT_data_field_ptr + 4))
#define CLUT_definition_segment__T_value_flag1(CLUT_data_field_ptr) (*((u8*)CLUT_data_field_ptr + 5))


//change by hulj 080924------------------------------------------
//#define CLUT_definition_segment__Y_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 2))&0xfc)
#define CLUT_definition_segment__Y_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 2))&0xfc)>>2
//#define CLUT_definition_segment__Cr_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 3))<<6) + (((*((u8*)CLUT_data_field_ptr + 3))&0xc)>>2)
#define CLUT_definition_segment__Cr_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 2))<<6) + (((*((u8*)CLUT_data_field_ptr + 3))&0xc0)>>2)

//#define CLUT_definition_segment__Cb_value_flag0(CLUT_data_field_ptr) (((*((u8*)CLUT_data_field_ptr + 3))<<2)&0xf0)
#define CLUT_definition_segment__Cb_value_flag0(CLUT_data_field_ptr) (((*((u8*)CLUT_data_field_ptr + 3))<<2)&0xf0)>>4
//#define CLUT_definition_segment__T_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 3))<<6)
#define CLUT_definition_segment__T_value_flag0(CLUT_data_field_ptr) ((*((u8*)CLUT_data_field_ptr + 3))&3)

//subtitle object
#define object_data_segment__object_id_h(segment_data_field_ptr) (*((u8*)segment_data_field_ptr))
#define object_data_segment__object_id_l(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 1))
#define object_data_segment__object_version_number(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr + 2))>>4)
#define object_data_segment__object_coding_method(segment_data_field_ptr) (((*((u8*)segment_data_field_ptr + 2)) >> 2)&0x3)
#define object_data_segment__non_modifying_colour_flag(segment_data_field_ptr) ((*((u8*)segment_data_field_ptr + 2))&0x1)
#define object_data_segment__top_field_data_block_length_h(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 3))
#define object_data_segment__top_field_data_block_length_l(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 4))
#define object_data_segment__bottom_field_data_block_length_h(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 5))
#define object_data_segment__bottom_field_data_block_length_l(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 6))
#define object_data_segment__field_pixel_data_sub_block_ptr(segment_data_field_ptr) (((u8*)segment_data_field_ptr + 7))
#define object_data_segment__number_of_codes(segment_data_field_ptr) (*((u8*)segment_data_field_ptr + 3))
#define object_data_segment__character_code_start_ptr(segment_data_field_ptr) (((u8*)segment_data_field_ptr + 4))

//subtitle subblock
#define pixel_data_sub_block__data_type(field_pixel_data_sub_block_ptr) (*((u8*)field_pixel_data_sub_block_ptr))
#define pixel_data_sub_block__pixel_code_string_ptr(field_pixel_data_sub_block_ptr) (((u8*)field_pixel_data_sub_block_ptr + 1))

/* Error Constants */

/* Exported Types --------------------------------------------------------- */

/**
* DVBSubtitleWindow
* @version: version 
* @display_window_flag: window_* are valid
* @display_width: assumed width of display
* @display_height: assumed height of display
* @window_x: x coordinate of top left corner of the subtitle window
* @window_y: y coordinate of top left corner of the subtitle window
* @window_width: width of the subtitle window
* @window_height: height of the subtitle window
* 
* A structure presenting display and window information
* display definition segment from ETSI EN 300 743 V1.3.1 (Not V1.2.1)
*/
typedef struct DVBSubtitleWindow {
    u32 version;
    u32 window_flag;

    u16 display_width;
    u16 display_height;

    u16 window_x;
    u16 window_y;
    u16 window_width;
    u16 window_height;
} DVBSubtitleWindow;
 
typedef struct run_length
{
    //实际解码数据的长度
	u16 dec_length;
	//读取数据的长度
	u16 read_length;
	u32 pixel_length;
}run_length_t;

typedef struct subtitle_region
{
	u8 region_id;
	u8 region_visible;
	u16 region_right;
	u16 region_left;
	u16 region_top;
	u16 region_bottom;
	u8 CLUT_id;
	u8 clut_ptr;
	u8 bit_per_pix;//最低深度
	u8 region_depth;//最低深度
	u8* top_start_ptr;
	u8* bottom_start_ptr;
	u8* top_end;
	u8* bottom_end;
	u16 bit_16_per_line;//每行占的16BIT的个数
}subtitle_region_t;

typedef struct subtitle_clut
{
	u8 clut_id;
	u8 *clut_ptr;
}subtitle_clut_t;

typedef struct draw_pen
{
	u8* draw_pen_head;
	u8 pixel_switch;
	
}draw_pen_t;

/* Exported Variables ----------------------------------------------------- */
extern u8* g_SppBuffer;


/* Exported Macros -------------------------------------------------------- */

/* Exported Messages ------------------------------------------------------ */

/* Exported Functions ----------------------------------------------------- */
AppErr_t subt_mem_copy(u8 *pSourcePtr, u8 *pTargetPtr, u16 wLength);
AppErr_t subt_mem_move(u8 *pStratPtr, u8 *pEndPtr,u16 wLength);
AppErr_t subt_pic_layer_enable(void);
AppErr_t subt_pic_layer_disable(void);
void subt_clean_pic_buf(void);
AppErr_t subt_filter_setup(u16 subt_pid);
AppErr_t subt_initial(void);
void subt_buf_reset(void);
AppErr_t subt_user_interface(void);
AppErr_t subt_send_to_buffer(u16 wPackLen );
u8* subt_get_region_ptr(u8 chRegionId);
u8* subt_get_CLUT_ptr(u8 chClutId);
u8* subt_get_object_ptr(u16 wObjectId);
u8 subt_wait_PTS(void);
void subt_dec(void);
AppErr_t subt_creat_region(u8 chRegionId,struct subtitle_region *pSubtitleRegions);
AppErr_t subt_creat_object_data (u8 *pSegmentDataFieldPtr,u16 wSegmentLength,u16 wObjectPositionH,u16 wObjectPositionV,struct subtitle_region *pSubtitleRegions );
u8 subt_creat_CLUT(u8 chClutId);
AppErr_t subt_creat_block_data(u8*pFieldPixelDataSubBlockPtr,u8 *pBufrerWritePtr,u16 wObjectPositionV,u16 wObjectPositionH,u16 wFieldDataBlockLength,struct subtitle_region* pSubtitleRegions);
void subt_draw_region(struct subtitle_region* SubtitleRegions);
struct run_length subt_run_length_dec(u8* data_source_ptr,u8* data_target_ptr,u8 write_buffer_switch,u8 bpp,u8 depth,u16 wLeftBlockLength, u8* map_table);
void subt_set_pixel(u16 wX,u16 wY,u16 wYcbcr);
void subt_subtable_close(void);


#ifdef __cplusplus
}
#endif

#endif /* __COM_SUBT_SUB_H__ */

/* End of file -------------------------------------------------------------*/





































