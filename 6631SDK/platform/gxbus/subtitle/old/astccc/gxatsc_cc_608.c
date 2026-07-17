/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.01.03		  zhouhm 			 creation
*****************************************************************************/
#include "gxtype.h"
#include "gxos/gxcore_os.h"
#include "gxcore.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gxavdev.h"
#include "../scte/gxatsc_cc_inner.h"
#include "../scte/gx_cc_subtitle_common.h"
#include "../scte/gx_cc_subtitle_draw.h"
#include "gx_subtitle_atsccc.h"

extern int gx_atsc_cc_608_miscellaneous_control_codes_action(MISCELLANEOUS_CONTROL_CODE_T control_code);
extern int gx_atsc_cc_608_preamble_address_codes_action(uint8_t first_code_pair,uint8_t second_code_pair);
extern int gx_atsc_cc_608_background_foreground_attributes_action(BACK_FORE_GROUND_CODES_T back_fore_ground_code);
extern int gx_atsc_cc_608_pen_attribute_action(CC_PEN_SIZE pen_size);
extern int32_t gx_atsc_cc_clear_last_character(uint8_t window_id);
extern int32_t gx_atsc_cc_draw_character(char* show_char,uint8_t window_id);
extern int gx_atsc_cc_608_set_pen_opacity_type(OPACITY_TYPE opacity_type);
extern OPACITY_TYPE gx_atsc_cc_608_get_pen_opacity_type(void);
extern int gx_atsc_cc_608_mid_row_codes_action(MID_ROW_CODES_T mid_row_codes);
extern int gx_atsc_cc_add(GX_SUBTITLE_ATSCCC_SERVICE service);
extern int gx_atsc_cc_unicode_to_utf8_one(unsigned long unic, char *pOutput);

static GX_SUBTITLE_ATSCCC_SERVICE last_check_cc_service = GX_SUBTITLE_ATSCCC_CC_INVAILD;
static int gx_atsc_cc_608_field_channel_check_cc_service(uint8_t field,uint8_t data_channel)
{
	GX_SUBTITLE_ATSCCC_SERVICE cc_service = GX_SUBTITLE_ATSC_608_CC1;

	if ((1 == field)&&(1 == data_channel))
		{
			cc_service = GX_SUBTITLE_ATSC_608_CC1;
		}

	if ((1 == field)&&(2 == data_channel))
		{
			cc_service = GX_SUBTITLE_ATSC_608_CC2;
		}

	if ((2 == field)&&(1 == data_channel))
		{
			cc_service = GX_SUBTITLE_ATSC_608_CC3;
		}

	if ((2 == field)&&(2 == data_channel))
		{
			cc_service = GX_SUBTITLE_ATSC_608_CC4;
		}
	last_check_cc_service = cc_service;
	gx_atsc_cc_add(cc_service);
	DRIVER_ATSC_CC_DBG((" field=%d data_channel=%d cc_service=%d \n",field,data_channel,cc_service));
	if (cc_service != atsc_cc_para.id)
		{
			return FALSE;
		}

	return TRUE;
}

static MISCELLANEOUS_CONTROL_CODE_T gx_atsc_cc_608_miscellaneous_control_codes(uint8_t data1,uint8_t data2)
{
	MISCELLANEOUS_CONTROL_CODE_T control_code = MISCELLANEOUS_INVALID;
	switch(data1)
		{
			case 0x14:
			case 0x1c:
				switch(data2)
				{
					case 0x20:
						DRIVER_ATSC_CC_DBG(("Resume caption loading,Pop-On Style\n"));
						control_code = MISCELLANEOUS_RCL;
						break;
					case 0x21:
						DRIVER_ATSC_CC_DBG(("Backspace.\n"));
						control_code = MISCELLANEOUS_BS;
						break;
					case 0x22:
						DRIVER_ATSC_CC_DBG(("Reserved (formerly Alarm Off).\n"));
						control_code = MISCELLANEOUS_AOF;
						break;
					case 0x23:
						DRIVER_ATSC_CC_DBG(("Reserved (formerly Alarm On).\n"));
						control_code = MISCELLANEOUS_AON;
						break;
					case 0x24:
						DRIVER_ATSC_CC_DBG(("Delete to End of Row.\n"));
						control_code = MISCELLANEOUS_DER;
						break;
					case 0x25:					
						DRIVER_ATSC_CC_DBG(("Roll-Up Captions-2 Rows,Roll-Up Style\n"));
						control_code = MISCELLANEOUS_RU2;
						break;
					case 0x26:
						DRIVER_ATSC_CC_DBG(("Roll-Up Captions-3 Rows,Roll-Up Style\n"));								
						control_code = MISCELLANEOUS_RU3;
						break;
					case 0x27:
						DRIVER_ATSC_CC_DBG(("Roll-Up Captions-4 Rows,Roll-Up Style\n")); 
						control_code = MISCELLANEOUS_RU4;
						break;
					case 0x28:
						DRIVER_ATSC_CC_DBG(("Flash On.\n"));
						control_code = MISCELLANEOUS_FON;
						break;
					case 0x29:						
						DRIVER_ATSC_CC_DBG(("Resume Direct Captioning,Paint-On Style\n"));
						control_code = MISCELLANEOUS_RDC;
						break;
					case 0x2A:
						DRIVER_ATSC_CC_DBG(("Text Restart.\n"));
						control_code = MISCELLANEOUS_TR;
						break;
					case 0x2B:
						DRIVER_ATSC_CC_DBG(("Resume Text Display.\n"));
						control_code = MISCELLANEOUS_RTD;
						break;
					case 0x2c:
						DRIVER_ATSC_CC_DBG(("Erase Displayed Memory.\n"));
						control_code = MISCELLANEOUS_EDM;
						break;
					case 0x2d:
						DRIVER_ATSC_CC_DBG(("Carriage Return.\n"));
						control_code = MISCELLANEOUS_CR;
						break;
					case 0x2E:
						DRIVER_ATSC_CC_DBG(("Erase Non-Displayed Memory.\n"));
						control_code = MISCELLANEOUS_ENM;
						break;
					case 0x2F:
						DRIVER_ATSC_CC_DBG(("End of Caption (Flip Memories).\n"));
						control_code = MISCELLANEOUS_EOC;
						break;
					default:
						break;
				}
				break;
			case 0x17:	/* CC1/CC3*/
			case 0x1f: /* CC2/CC4*/
				switch(data2)
				{
					case 0x21:					
						DRIVER_ATSC_CC_DBG(("Tab Offset 1 Column.\n"));
						control_code = MISCELLANEOUS_TO1;
						break;
					case 0x22:
						DRIVER_ATSC_CC_DBG(("Tab Offset 2 Columns.\n"));
						control_code = MISCELLANEOUS_TO2;
						break;
					case 0x23:
						control_code = MISCELLANEOUS_TO3;
						DRIVER_ATSC_CC_DBG(("Tab Offset 3 Columns.\n"));
						break;
					default:
						break;
				}

				break;
			default:
				break;
		}
	
	return control_code;
	
}


static int gx_atsc_cc_608_preamble_address_codes(uint8_t field,uint8_t data1,uint8_t data2,uint8_t* first_code_pair,uint8_t* second_code_pair)
{
	uint8_t data_channel=0;

	if ((NULL == first_code_pair)||(NULL == second_code_pair))
		{
			DRIVER_ATSC_CC_ERROR((" param NULL\n"));
			return -1;
		}

	switch(data1)
		{
			case 0x11: /*Row 1*/
			case 0x12: /*Row 3*/
			case 0x15: /*Row 5*/
			case 0x16: /*Row 7*/
			case 0x17: /*Row 9*/
			case 0x10: /*Row 11*/
			case 0x13: /*Row 12*/
			case 0x14: /*Row 14*/
			case 0x19: /*Row 1*/
			case 0x1a: /*Row 3*/
			case 0x1d: /*Row 5*/
			case 0x1e: /*Row 7*/
			case 0x1f: /*Row 9*/
			case 0x18: /*Row 11*/
			case 0x1b: /*Row 12*/
			case 0x1c: /*Row 14*/
				if ((data2>=0x40)&&(data2<=0x5f))
					{
						if ((0x11 == data1)||(0x12 == data1)
							||(0x15 == data1)||(0x16 == data1)
							||(0x17 == data1)||(0x10 == data1)
							||(0x13 == data1)||(0x14 == data1))
							{
								/*
								* CC1/CC3
								*/
								data_channel = 1;													
							}
						else 
							if ((0x19 == data1)||(0x1a == data1)
							||(0x1d == data1)||(0x1e == data1)
							||(0x1f == data1)||(0x18 == data1)
							||(0x1b == data1)||(0x1c == data1))
							{
								/*
								* CC2/CC4
								*/
								data_channel = 2;						
							}
						
						if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
							{
								break;
							}

						if ((0x11 == data1)||(0x19 == data1))
							{
								*first_code_pair = PREAMBLE_ROW_1;							
							}

						if ((0x12 == data1)||(0x1a == data1))
							{
								*first_code_pair = PREAMBLE_ROW_3;
							}

						if ((0x15 == data1)||(0x1d == data1))
							{
								*first_code_pair = PREAMBLE_ROW_5;
							}

						if ((0x16 == data1)||(0x1e == data1))
							{
								*first_code_pair = PREAMBLE_ROW_7;
							}

						if ((0x17 == data1)||(0x1f == data1))
							{
								*first_code_pair = PREAMBLE_ROW_9;
							}

						if ((0x10 == data1)||(0x18 == data1))
							{
								*first_code_pair = PREAMBLE_ROW_11;
							}

						if ((0x13 == data1)||(0x1b == data1))
							{
								*first_code_pair = PREAMBLE_ROW_12;
							}

						if ((0x14 == data1)||(0x1c == data1))
							{
								*first_code_pair = PREAMBLE_ROW_14;
							}
						switch(data2)
							{
								case 0x40:
									*second_code_pair = PREAMBLE_WHITE;
									break;
								case 0x41:
									*second_code_pair = PREAMBLE_WHITE_UNDERLINE;
									break;
								case 0x42:
									*second_code_pair = PREAMBLE_GREEN; 						
									break;
								case 0x43:
									*second_code_pair = PREAMBLE_GREEN_UNDERLINE;
									break;
								case 0x44:
									*second_code_pair = PREAMBLE_BLUE;
									break;
								case 0x45:
									*second_code_pair = PREAMBLE_BLUE_UNDERLINE;
									break;
								case 0x46:
									*second_code_pair = PREAMBLE_CYAN;
									break;
								case 0x47:
									*second_code_pair = PREAMBLE_CYAN_UNDERLINE;
									break;								
								case 0x48:
									*second_code_pair = PREAMBLE_RED;
									break;
								case 0x49:
									*second_code_pair = PREAMBLE_RED_UNDERLINE;
									break;
								case 0x4a:
									*second_code_pair = PREAMBLE_YELLOW;
									break;
								case 0x4b:
									*second_code_pair = PREAMBLE_YELLOW_UNDERLINE;
									break;								
								case 0x4c:
									*second_code_pair = PREAMBLE_MAGENTA;
									break;
								case 0x4d:
									*second_code_pair = PREAMBLE_MAGENTA_UNDERLINE;
									break;
								case 0x4e:
									*second_code_pair = PREAMBLE_WHITE_ITALICS;
									break;							
								case 0x4f:
									*second_code_pair = PREAMBLE_WHITE_ITALICS_UNDERLINE;
									break;
								case 0x50:
									*second_code_pair = PREAMBLE_INDENT_0;
									break;
								case 0x51:
									*second_code_pair = PREAMBLE_INDENT_0_UNDERLINE;									
									break;
								case 0x52:
									*second_code_pair = PREAMBLE_INDENT_4;
									break;
								case 0x53:
									*second_code_pair = PREAMBLE_INDENT_4_UNDERLINE;										
									break;
								case 0x54:
									*second_code_pair = PREAMBLE_INDENT_8;
									break;
								case 0x55:
									*second_code_pair = PREAMBLE_INDENT_8_UNDERLINE;	
									break;
								case 0x56:
									*second_code_pair = PREAMBLE_INDENT_12;
									break;
								case 0x57:
									*second_code_pair = PREAMBLE_INDENT_12_UNDERLINE;	
									break;
								case 0x58:
									*second_code_pair = PREAMBLE_INDENT_16;
									break;
								case 0x59:
									*second_code_pair = PREAMBLE_INDENT_16_UNDERLINE;	
									break;
								case 0x5a:
									*second_code_pair = PREAMBLE_INDENT_20;
									break;
								case 0x5b:
									*second_code_pair = PREAMBLE_INDENT_20_UNDERLINE;
									break;
								case 0x5c:
									*second_code_pair = PREAMBLE_INDENT_24;
									break;
								case 0x5d:
									*second_code_pair = PREAMBLE_INDENT_24_UNDERLINE;	
									break;
								case 0x5e:
									*second_code_pair = PREAMBLE_INDENT_28;
									break;
								case 0x5f:
									*second_code_pair = PREAMBLE_INDENT_28_UNDERLINE;	
									break;
								default:
									break;	
							}
					}
				break;
			default:
				break;
		}

	switch(data1)
		{
			case 0x11: /*Row 2*/
			case 0x12: /*Row 4*/
			case 0x15: /*Row 6*/
			case 0x16: /*Row 8*/
			case 0x17: /*Row 10*/
			case 0x13: /*Row 13*/
			case 0x14: /*Row 15*/
			case 0x19: /*Row 2*/
			case 0x1a: /*Row 4*/
			case 0x1d: /*Row 6*/
			case 0x1e: /*Row 8*/
			case 0x1f: /*Row 10*/
			case 0x1b: /*Row 13*/
			case 0x1c: /*Row 15*/
				if ((data2>=0x60)&&(data2<=0x7f))
					{
						if ((0x11 == data1)||(0x12 == data1)
							||(0x15 == data1)||(0x16 == data1)
							||(0x17 == data1)||(0x13 == data1)
							||(0x14 == data1))
							{
								/*
								* CC1/CC3
								*/
								data_channel = 1;						
							}
						else 
							if ((0x19 == data1)||(0x1a == data1)
							||(0x1d == data1)||(0x1e == data1)
							||(0x1f == data1)||(0x18 == data1)
							||(0x1b == data1)||(0x1c == data1))
							{
								/*
								* CC2/CC4
								*/
								data_channel = 2;														
							}
						
						if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
							{
								break;
							}

						window_define[0].anchor_horizontal=0;
						if ((0x11 == data1)||(0x19 == data1))
							{
								*first_code_pair = PREAMBLE_ROW_2;
							}

						if ((0x12 == data1)||(0x1a == data1))
							{
								*first_code_pair = PREAMBLE_ROW_4;
							}

						if ((0x15 == data1)||(0x1d == data1))
							{
								*first_code_pair = PREAMBLE_ROW_6;
							}

						if ((0x16 == data1)||(0x1e == data1))
							{
								*first_code_pair = PREAMBLE_ROW_8;
							}

						if ((0x17 == data1)||(0x1f == data1))
							{
								*first_code_pair = PREAMBLE_ROW_10;
							}

						if ((0x10 == data1)||(0x18 == data1))
							{
								*first_code_pair = PREAMBLE_ROW_11;
							}

						if ((0x13 == data1)||(0x1b == data1))
							{
								*first_code_pair = PREAMBLE_ROW_13;
							}

						if ((0x14 == data1)||(0x1c == data1))
							{
								*first_code_pair = PREAMBLE_ROW_15;
							}

						switch(data2)
							{
								case 0x60:
									*second_code_pair = PREAMBLE_WHITE;
									break;
								case 0x61:
									*second_code_pair = PREAMBLE_WHITE_UNDERLINE;
									break;
								case 0x62:
									*second_code_pair = PREAMBLE_GREEN; 					
									break;
								case 0x63:
									*second_code_pair = PREAMBLE_GREEN_UNDERLINE;
									break;
								case 0x64:
									*second_code_pair = PREAMBLE_BLUE;	
									break;
								case 0x65:
									*second_code_pair = PREAMBLE_BLUE_UNDERLINE;
									break;
								case 0x66:
									*second_code_pair = PREAMBLE_CYAN;
									break;
								case 0x67:
									*second_code_pair = PREAMBLE_CYAN_UNDERLINE;
									break;								
								case 0x68:
									*second_code_pair = PREAMBLE_RED;
									break;
								case 0x69:
									*second_code_pair = PREAMBLE_RED_UNDERLINE;
									break;
								case 0x6a:
									*second_code_pair = PREAMBLE_YELLOW;
									break;
								case 0x6b:
									*second_code_pair = PREAMBLE_YELLOW_UNDERLINE;
									break;								
								case 0x6c:
									*second_code_pair = PREAMBLE_MAGENTA;
									break;
								case 0x6d:
									*second_code_pair = PREAMBLE_MAGENTA_UNDERLINE;
									break;
								case 0x6e:
									*second_code_pair = PREAMBLE_WHITE_ITALICS;
									break;							
								case 0x6f:
									*second_code_pair = PREAMBLE_WHITE_ITALICS_UNDERLINE;
									break;
								case 0x70:
									*second_code_pair = PREAMBLE_INDENT_0;
									break;
								case 0x71:
									*second_code_pair = PREAMBLE_INDENT_0_UNDERLINE;									
									break;
								case 0x72:
									*second_code_pair = PREAMBLE_INDENT_4;
									break;
								case 0x73:
									*second_code_pair = PREAMBLE_INDENT_4_UNDERLINE;										
									break;
								case 0x74:
									*second_code_pair = PREAMBLE_INDENT_8;
									break;
								case 0x75:
									*second_code_pair = PREAMBLE_INDENT_8_UNDERLINE;
									break;
								case 0x76:
									*second_code_pair = PREAMBLE_INDENT_12;
									break;
								case 0x77:
									*second_code_pair = PREAMBLE_INDENT_12_UNDERLINE;	
									break;
								case 0x78:
									*second_code_pair = PREAMBLE_INDENT_16;
									break;
								case 0x79:
									*second_code_pair = PREAMBLE_INDENT_16_UNDERLINE;		
									break;
								case 0x7a:
									*second_code_pair = PREAMBLE_INDENT_20;
									break;
								case 0x7b:
									*second_code_pair = PREAMBLE_INDENT_20_UNDERLINE;	
									break;
								case 0x7c:
									*second_code_pair = PREAMBLE_INDENT_24;
									break;
								case 0x7d:
									*second_code_pair = PREAMBLE_INDENT_24_UNDERLINE;	
									break;
								case 0x7e:
									*second_code_pair = PREAMBLE_INDENT_28;
									break;
								case 0x7f:
									*second_code_pair = PREAMBLE_INDENT_28_UNDERLINE;	
									break;
								default:
									break;	
							}
					}
				break;
			default:
				break;
		}

	return 0;	
}


static BACK_FORE_GROUND_CODES_T gx_atsc_cc_608_background_foreground_attributes(uint8_t data1,uint8_t data2)
{
	BACK_FORE_GROUND_CODES_T back_fore_ground_code = BACK_FORE_GROUND_INVALID;
	switch(data1)
		{
			case 0x10:
			case 0x18:
				switch(data2)
					{
						/*
						* Background and Foreground Attribute Codes
						*/
						case 0x20:
							back_fore_ground_code = BACKGROUND_WHITE_OPAQUE;
							break;
						case 0x21:
							back_fore_ground_code = BACKGROUND_WHITE_SEMI_TRANSPARENT;						
							break;
						case 0x22:
							back_fore_ground_code = BACKGROUND_GREEN_OPAQUE;	
							break;
						case 0x23:
							back_fore_ground_code = BACKGROUND_GREEN_SEMI_TRANSPARENT;
							break;
						case 0x24:
							back_fore_ground_code = BACKGROUND_BLUE_OPAQUE; 						
							break;
						case 0x25:
							back_fore_ground_code = BACKGROUND_BLUE_SEMI_TRANSPARENT;
							break;
						case 0x26:
							back_fore_ground_code = BACKGROUND_CYAN_OPAQUE;
							break;
						case 0x27:
							back_fore_ground_code = BACKGROUND_CYAN_SEMI_TRANSPARENT;
							break;
						case 0x28:
							back_fore_ground_code = BACKGROUND_RED_OPAQUE;
							break;
						case 0x29:
							back_fore_ground_code = BACKGROUND_RED_SEMI_TRANSPARENT;
							break;
						case 0x2a:
							back_fore_ground_code = BACKGROUND_YELLOW_OPAQUE;
							break;
						case 0x2b:
							back_fore_ground_code = BACKGROUND_YELLOW_SEMI_TRANSPARENT;
							break;
						case 0x2c:
							back_fore_ground_code = BACKGROUND_MAGENTA_OPAQUE;
							break;	
						case 0x2d:
							back_fore_ground_code = BACKGROUND_MAGENTA_SEMI_TRANSPARENT;
							break;	
						case 0x2e:
							back_fore_ground_code = BACKGROUND_BLACK_OPAQUE;
							break;	
						case 0x2f:
							back_fore_ground_code = BACKGROUND_BLACK_SEMI_TRANSPARENT;
							break;
						default:
							break;
					}

				break;
			case 0x17:
			case 0x1f:
				switch(data2)
				{
					case 0x2d:
						back_fore_ground_code = BACKGROUND_TRANSPARENT;
						break;
					case 0x2e:
						back_fore_ground_code = FOREGROUND_BLACK;
						break;
					case 0x2f:
						back_fore_ground_code = FOREGROUND_BLACK_UNDERLINE;
						break;
					default:
						break;
				}

				break;
			default:
				break;
		}
	return back_fore_ground_code;
}


static MID_ROW_CODES_T gx_atsc_cc_608_mid_row_codes(uint8_t data1,uint8_t data2)
{
	MID_ROW_CODES_T mid_row_codes = MID_INVALID;
	switch(data1)
		{
			case 0x11:
			case 0x19:
				switch(data2)
					{
						case 0x20:
							mid_row_codes = MID_WHITE;
							break;
						case 0x21:
							mid_row_codes = MID_WHITE_UNDERLINE;						
							break;
						case 0x22:
							mid_row_codes = MID_GREEN;	
							break;
						case 0x23:
							mid_row_codes = MID_GREEN_UNDERLINE;	
							break;
						case 0x24:
							mid_row_codes = MID_BLUE;	
							break;
						case 0x25:
							mid_row_codes = MID_BLUE_UNDERLINE;
							break;
						case 0x26:
							mid_row_codes = MID_CYAN;
							break;
						case 0x27:
							mid_row_codes = MID_CYAN_UNDERLINE;
							break;
						case 0x28:
							mid_row_codes = MID_RED;
							break;
						case 0x29:
							mid_row_codes = MID_RED_UNDERLINE;
							break;
						case 0x2a:
							mid_row_codes = MID_YELLOW;
							break;
						case 0x2b:
							mid_row_codes = MID_YELLOW_UNDERLINE;
							break;
						case 0x2c:
							mid_row_codes = MID_MAGENTA;
							break;	
						case 0x2d:
							mid_row_codes = MID_MAGENTA_UNDERLINE;
							break;	
						case 0x2e:
							mid_row_codes = MID_ITALICS;
							break;	
						case 0x2f:
							mid_row_codes = MID_ITALICS_UNDERLINE;
							break;
						default:
							break;
					}
				break;
			default:
				break;
		}
	return mid_row_codes;

}


int gx_atsc_cc_608_unit_parse(char* data)
{
	char character[3]={0};
	int32_t i =0;
	int32_t clear_flag =0;
	int32_t ret = -1;
	char show_character[4]={0};
	uint8_t data1=0,data2=0;
	uint8_t odd_parity=0;
	uint8_t field=0;
	uint8_t data_channel=0;
	uint8_t bytes_flag = 0;// 2bytes confirm one character
	uint8_t first_code_pair = PREAMBLE_FIRST_INVALID;
	uint8_t second_code_pair = PREAMBLE_SECOND_INVALID;
	MISCELLANEOUS_CONTROL_CODE_T miscellaneous_control_code = MISCELLANEOUS_INVALID;
	MID_ROW_CODES_T mid_row_codes = MID_INVALID;
	BACK_FORE_GROUND_CODES_T back_fore_ground_code = BACK_FORE_GROUND_INVALID;

	
	if (NULL == data)
		{
			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_608_unit_parse para error\n"));
			return -1;			
		}

	odd_parity = (data[1]&0x80)>>7;
	data1 = data[1]&0x7f;
	data2 = data[2]&0x7f;
	if (0 == data[0])
		{
			field=1;
		}
	else
		{
			field=2;
		}

	 if ((data1>=0x10)&&(data1<=0x17))
		 {
			 /*
			 * CC1/CC3
			 */
			 data_channel = 1;
			 gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel);
		 }
	 
	 if ((data1>=0x18)&&(data1<=0x1f))
		  {
			  /*
			 *	CC2/CC4
			 */
			 data_channel = 2;
			 gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel);
		 }

     if (atsc_cc_para.id > GX_SUBTITLE_ATSC_608_CC4)
         {
             return 0;
         }

	if (1 == field)
	   {
		   if ((GX_SUBTITLE_ATSC_608_CC1 != atsc_cc_para.id)&&(GX_SUBTITLE_ATSC_608_CC2 != atsc_cc_para.id))
			   {
				   return 0;
			   }
	   }
	
	if (2 == field)
	   {
			if ((GX_SUBTITLE_ATSC_608_CC3 != atsc_cc_para.id)&&(GX_SUBTITLE_ATSC_608_CC4 != atsc_cc_para.id))
				{
					return 0;
				}
	   }
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	if (NULL == spp_cc_subtitle_surface)
		{
			DRIVER_ATSC_CC_ERROR(("spp_cc_subtitle_surface = %p\n",spp_cc_subtitle_surface));
			return 0;
		}
#endif
	if ((data1>=0x10)&&(data1<=0x1f))
	{
		switch(data1)
		{
			case 0x10:
			case 0x18:
				if (0x10 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x18 == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);
				back_fore_ground_code = gx_atsc_cc_608_background_foreground_attributes(data1,data2);
				gx_atsc_cc_608_background_foreground_attributes_action(back_fore_ground_code);
				break;
			case 0x11:
			case 0x19:
				if (0x11 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x19 == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);
				if ((data2>=0x30)&&(data2<=0x3f))
					{
						DRIVER_ATSC_CC_DBG(("Special North American character data1=0x%x data2=0x%x\n",
							data1,data2));
					}
				switch(data2)
					{
						/*
						* Special North American character set
						*/
						case 0x30:
							character[0]=0xae;
							break;
						case 0x31:
							character[0]=0xb0;
							break;
						case 0x32:
							character[0]=0xbd;
							break;
						case 0x33:
							character[0]=0xbf;
							break;
						case 0x34:
							character[0]=0x21;
							character[1]=0x22;
							bytes_flag = 1;
							break;
							//DRIVER_ATSC_CC_DBG(("Special North American character set TM \n"));
							/*
							* TM is short for unregistered trademark and should be represented in superscript (TM).
							*/
							break;
						case 0x35:
							character[0]=0xa2;//'¢'
							break;
						case 0x36:
							character[0]=0xa3;//'£'
							break;
						case 0x37:
							character[0]=0x26;
							character[1]=0x6a;
							bytes_flag = 1;
							//DRIVER_ATSC_CC_DBG(("Special North American character set the Eighth note \n"));
							/*
							* the Eighth note is used to denote singing or background music in captions.
							*/
							break;
						case 0x38:
							character[0]=0xe0;//'à'
							break;
						case 0x39:
							{
								DRIVER_ATSC_CC_DBG(("Special North American character set TS  \n"));
								OPACITY_TYPE opacity=gx_atsc_cc_608_get_pen_opacity_type();					
								gx_atsc_cc_608_set_pen_opacity_type(OPACITY_TRANSPARENT);
								memset(show_character,0,sizeof(show_character));
								show_character[0]=' ';
								gx_atsc_cc_draw_character(show_character,0);
								gx_atsc_cc_608_set_pen_opacity_type(opacity);						
								/*
								* TS in the table above represents a "transparent space" or non-breaking space. 
								*/
							}
							break;
						case 0x3a:
							character[0]=0xe8;//'è'	
							break;
						case 0x3b:
							character[0]=0xe2;//'â'
							break;
						case 0x3c:
							character[0]=0xea;//'ê'
							break;
						case 0x3d:
							character[0]=0xee;//'î'
							break;
						case 0x3e:
							character[0]=0xf4;//'ô'
							break;
						case 0x3f:
							character[0]=0xfb;//'û'
							break;
						default:
							break;
					}
				mid_row_codes = gx_atsc_cc_608_mid_row_codes(data1,data2);
				gx_atsc_cc_608_mid_row_codes_action(mid_row_codes);
				break;
			case 0x12:
			case 0x1A:
				if (0x12 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x1a == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);

				if ((data2>=0x20)&&(data2<=0x2f))
					{
						DRIVER_ATSC_CC_DBG(("Extended Spanish data1=0x%x data2=0x%x\n",
							data1,data2));
					}
				if ((data2>=0x30)&&(data2<=0x3f))
					{
						DRIVER_ATSC_CC_DBG(("Extended French data1=0x%x data2=0x%x\n",
							data1,data2));
					}
				switch(data2)
					{
						/*
						* Extended Spanish 0x20~0x2f
						*/
						case 0x20:
							character[0]=0xc1;//'Á'
							break;
						case 0x21:
							character[0]=0xc9;//'É'
							break;
						case 0x22:
							character[0]=0xd3;//'Ó'
							break;
						case 0x23:
							character[0]=0xda;//'Ú'
							break;
						case 0x24:
							character[0]=0xdc;//'Ü'
							break;
						case 0x25:
							character[0]=0xfc;//'ü'
							break;
						case 0x26:
							character[0]=0xb4;
							break;
						case 0x27:
							character[0]=0xa1;//'¡'							
							break;
						case 0x28:
							character[0]=0x2a;//'*'
							break;
						case 0x29:
							character[0]=0x27;
							break;
						case 0x2a:
							character[0]=0xad;
							break;
						case 0x2b:
							character[0]=0xa9;
							break;
						case 0x2c:
							character[0]=0x21;
							character[1]=0x26;
							bytes_flag = 1;
							//DRIVER_ATSC_CC_DBG(("Extended Spanish SM \n"));							
							/*
							* SM is short for service mark and should be represented in superscript. 
							* The single quote mark is a curly left and double quote marks are curly left and right. 
							* The plus signs refer to top left, top right, lower left and lower right corners for box drawing.
							*/
							break;
						case 0x2d:
							character[0]=0xb7;//'?'
							break;
						case 0x2e:
							character[0]=0x22;
							break;
						case 0x2f:
							character[0]=0x22;
							break;
						/*
						* Extended French 0x30~0x3f
						*/
						case 0x30:
							character[0]=0xc0;//'À'
							break;
						case 0x31:
							character[0]=0xc2;//'Â'
							break;
						case 0x32:
							character[0]=0xc7;//'Ç'
							break;
						case 0x33:
							character[0]=0xc8;//'È'
							break;
						case 0x34:
							character[0]=0xca;//'Ê'
							break;
						case 0x35:
							character[0]=0xcb;//'Ë'
							break;
						case 0x36:
							character[0]=0xeb;//'ë'
							break;
						case 0x37:
							character[0]=0xce;//'Î'
							break;
						case 0x38:
							character[0]=0xcf;//'Ï'
							break;
						case 0x39:
							character[0]=0xef;//'ï'
							break;
						case 0x3a:
							character[0]=0xd4;//'Ô'
							break;
						case 0x3b:
							character[0]=0xd9;//'Ù'
							break;
						case 0x3c:
							character[0]=0xf9;//'ù'
							break;
						case 0x3d:
							character[0]=0xdb;//'Û'
							break;
						case 0x3e:
							character[0]=0xab;//'«'
							break;
						case 0x3f:
							character[0]=0xbb;//'»'
							break;
						default:
							break;
					}
				break;
			case 0x13:
			case 0x1b:
				if (0x13 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x1b == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);


				if ((data2>=0x20)&&(data2<=0x2f))
					{
						DRIVER_ATSC_CC_DBG(("Portuguese data1=0x%x data2=0x%x\n",
							data1,data2));
					}

				if ((data2>=0x30)&&(data2<=0x37))
					{
						DRIVER_ATSC_CC_DBG(("German data1=0x%x data2=0x%x\n",
							data1,data2));
					}

				if ((data2>=0x38)&&(data2<=0x3f))
					{
						DRIVER_ATSC_CC_DBG(("Danish data1=0x%x data2=0x%x\n",
							data1,data2));
					}

				switch(data2)
					{
						/*
						* Portuguese 0x20~0x2f
						*/
						case 0x20:
							character[0]=0xc3;//'Ã'
							break;
						case 0x21:
							character[0]=0xe3;//'ã'
							break;
						case 0x22:
							character[0]=0xcd;//'Í'
							break;
						case 0x23:
							character[0]=0xcc;//'Ì'
							break;
						case 0x24:
							character[0]=0xec;//'ì'
							break;
						case 0x25:
							character[0]=0xd2;//'Ò'
							break;
						case 0x26:
							character[0]=0xf2;//'ò'
							break;
						case 0x27:
							character[0]=0xd5;//'Õ'
							break;
						case 0x28:
							character[0]=0xf5;//'õ'
							break;
						case 0x29:
							character[0]=0x7b;//'{'
							break;
						case 0x2a:
							character[0]=0x7d;//'}'
							break;
						case 0x2b:
							character[0]=0x5c;
							break;
						case 0x2c:
							character[0]=0x5e;//'^'
							break;
						case 0x2d:
							character[0]=0x5f;//'_'
							break;
						case 0x2e:
							character[0]=0x7c;//'|'
							break;
						case 0x2f:
							character[0]=0x7e;//'~'
							break;
						/*
						* German
						*/
						case 0x30:
							character[0]=0xc4;//'Ä'							
							break;
						case 0x31:
							character[0]=0xe4;//'ä'				
							break;
						case 0x32:
							character[0]=0xd6;//'Ö'							
							break;
						case 0x33:
							character[0]=0xf6;//'ö'							
							break;
						case 0x34:
							character[0]=0xdf;//'ß'
							break;
						case 0x35:
							character[0]=0xa5;//'¥'
							break;
						case 0x36:
							character[0]=0xa4;//'¤'
							break;
						case 0x37:
							character[0]=0xa6;//'¦'
							break;
						/*
						* Danish
						*/
						case 0x38:
							character[0]=0xc5;//'Å'
							break;
						case 0x39:
							character[0]=0xe5;//'å'
							break;
						case 0x3a:
							character[0]=0xd8;//'Ø'
							break;
						case 0x3b:
							character[0]=0xf8;//'ø'
							break;
						case 0x3c:
							character[0]=0x2b;//'+'
							break;
						case 0x3d:
							character[0]=0x2b;//'+'
							break;
						case 0x3e:
							character[0]=0x2b;//'+'
							break;
						case 0x3f:
							character[0]=0x2b;//'+'
							break;
						default:
							break;
					}
				break;
			case 0x14:
			case 0x15:
			case 0x1c:
			case 0x1d:
				if ((0x14 == data1)||(0x15 == data1))
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if ((0x1c == data1)||(0x1d == data1))
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);
				miscellaneous_control_code = gx_atsc_cc_608_miscellaneous_control_codes(data1,data2);
				gx_atsc_cc_608_miscellaneous_control_codes_action(miscellaneous_control_code);
				break;
			case 0x16: /* CC1/CC3*/
			case 0x1e: /* CC2/CC4*/
				if (0x16 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x1e == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);
				break;
			case 0x17:	/* CC1/CC3*/
			case 0x1f: /* CC2/CC4*/
				if (0x17 == data1)
					{
						/*
						* CC1/CC3
						*/
						data_channel = 1;
					}
				else
					if (0x1f == data1)
					{
						/*
						* CC2/CC4
						*/
						data_channel = 2;
					}

				if (FALSE == gx_atsc_cc_608_field_channel_check_cc_service(field,data_channel))
					{
						return 0;
					}
				first_code_pair = PREAMBLE_FIRST_INVALID;
				second_code_pair = PREAMBLE_SECOND_INVALID;
				gx_atsc_cc_608_preamble_address_codes(field,data1,data2,&first_code_pair,&second_code_pair);
				gx_atsc_cc_608_preamble_address_codes_action(first_code_pair,second_code_pair);

				switch(data2)
				{
					case 0x24:
						/*
						* Standard
						*/
						gx_atsc_cc_608_pen_attribute_action(PEN_STANDARD);
						DRIVER_ATSC_CC_DBG(("Select the standard line 21 character set in normal size.\n"));
						break;
					case 0x25:
						/*
						* Standard Double size
						*/
						gx_atsc_cc_608_pen_attribute_action(PEN_LARGE);
						DRIVER_ATSC_CC_DBG(("Select the standard line 21 character set in double size.\n"));
						break;
					case 0x26:
						/*
						* Decoder Specific 1
						*/
						DRIVER_ATSC_CC_DBG(("Select the first private character set.\n"));
						break;
					case 0x27:
						/*
						* Decoder Specific 2
						*/
						DRIVER_ATSC_CC_DBG(("Select the second private character set.\n"));
						break;
					case 0x28:
						/*
						* China's GB 2312 (1980)
						*/
						DRIVER_ATSC_CC_DBG(("Select the People's Republic of China character set: GB 2312-80.\n"));
						break;
					case 0x29:
						/*
						* Korea's KS C 5601 (1987)
						*/
						DRIVER_ATSC_CC_DBG(("Select the Korean Standard character set: KSC 5601-1987.\n"));
						break;
					case 0x2a:
						/*
						* Loadable
						*/
						DRIVER_ATSC_CC_DBG(("Select the first registered character set.\n"));
						break;
					default:
						break;
				}
				back_fore_ground_code = gx_atsc_cc_608_background_foreground_attributes(data1,data2);
				gx_atsc_cc_608_background_foreground_attributes_action(back_fore_ground_code);
				miscellaneous_control_code = gx_atsc_cc_608_miscellaneous_control_codes(data1,data2);
				gx_atsc_cc_608_miscellaneous_control_codes_action(miscellaneous_control_code);
				break;
			default:
				break;
		}
	}
	else if ((data1>=0x20)&&(data1<=0x7f))
	{
		if (last_check_cc_service != atsc_cc_para.id)
		{
			/*
			 * check last cc service is not same atsc_cc_para.id, should not draw character
			 */
			return 0;
		}
		if ((data1>=0x20)&&(data1<=0x7f))
		{
			character[0]=data1;
		}

		if ((data2>=0x20)&&(data2<=0x7f))
		{
			character[1]=data2;
		}

		for (i=0;i<2;i++)
			{
				switch(character[i])
					{
						/*
						* exceptions
						*/
						case 0x2a:
							character[i]=0xe1;//'á'
							break;
						case 0x5c:
							character[i]=0xe9;//'é'
							break;
						case 0x5e:
							character[i]=0xed;//'í'
							break;
						case 0x5f:
							character[i]=0xf3;//'Ó'
							break;
						case 0x60:
							character[i]=0xfa;//'ú'
							break;
						case 0x7b:
							character[i]=0xe7;//'ç'
							break;
						case 0x7c:
							character[i]=0xf7;//'÷'
							break;
						case 0x7d:
							character[i]=0xd1;//'Ñ'
							break;
						case 0x7e:
							character[i]=0xf1;//'ñ'
							break;
						case 0x7f:
							/*
							* SB represents a solid block
							*/
							break;
						default:
							break;
					}
			}

	}

	if (strlen(character)>0)
		{
			if(bytes_flag == 0)
			{
				for (i=0; i<strlen(character);i++)
				{
					switch(character[i])
						{
							/*
							* replace A,a or c,C or e, E or  i, I or n, N or o, O  or u, U
							*/
								/*a*/
							case 0xe0:
							case 0xe1:
							case 0xe2:
							case 0xe3:
							case 0xe4:
							case 0xe5:
								/*A*/
							case 0xc0:
							case 0xc1:
							case 0xc2:
							case 0xc3:
							case 0xc4:
							case 0xc5:
								if (1 == strlen(last_char))
									{
										if (('a' == last_char[0])||('A' == last_char[0])
											||(0xe0 == last_char[0])||(0xe1 == last_char[0])
											||(0xe2 == last_char[0])||(0xe3 == last_char[0])
											||(0xe4 == last_char[0])||(0xe5 == last_char[0])
											||(0xc0 == last_char[0])||(0xc1 == last_char[0])
											||(0xc2 == last_char[0])||(0xc3 == last_char[0])
											||(0xc4 == last_char[0])||(0xc5 == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break;
								/*c*/
							case 0xe7:
								/*C*/
							case 0xc7:
								if (1 == strlen(last_char))
									{
										if (('c' == last_char[0])||('C' == last_char[0])
											||(0xe7 == last_char[0])||(0xc7 == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break;
								/*e*/
							case 0xeb:
							case 0xe8:
							case 0xe9:
							case 0xea:
								/*E*/
							case 0xc8:
							case 0xc9:
							case 0xca:
							case 0xcb:
								if (1 == strlen(last_char))
									{
										if (('e' == last_char[0])||('E' == last_char[0])
											||(0xeb == last_char[0])||(0xe8 == last_char[0])
											||(0xe9 == last_char[0])||(0xea == last_char[0])
											||(0xc8 == last_char[0])||(0xc9 == last_char[0])
											||(0xca == last_char[0])||(0xcb == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break;
								/*i*/
							case 0xec:
							case 0xed:
							case 0xee:
							case 0xef:
								/*I*/
							case 0xcc:
							case 0xcd:
							case 0xce:
							case 0xcf:
								if (1 == strlen(last_char))
									{
										if (('i' == last_char[0])||('I' == last_char[0])
											||(0xec == last_char[0])||(0xed == last_char[0])
											||(0xee == last_char[0])||(0xef == last_char[0])
											||(0xcc == last_char[0])||(0xcd == last_char[0])
											||(0xce == last_char[0])||(0xcf == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break; 
								/*n*/
							case 0xf1:
								/*N*/
							case 0xd1:
								if (1 == strlen(last_char))
									{
										if (('n' == last_char[0])||('N' == last_char[0])
											||(0xf1 == last_char[0])||(0xd1 == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break;
								 /*o*/
							case 0xf2:
							case 0xf3:
							case 0xf4:
							case 0xf5:
							case 0xf6:
								/*O*/
							case 0xd2:
							case 0xd3:
							case 0xd4:
							case 0xd5:
							case 0xd6:	
								if (1 == strlen(last_char))
									{
										if (('o' == last_char[0])||('O' == last_char[0])
											||(0xf2 == last_char[0])||(0xf3 == last_char[0])
											||(0xf4 == last_char[0])||(0xf5 == last_char[0])
											||(0xf6 == last_char[0])||(0xd2 == last_char[0])
											||(0xd3 == last_char[0])||(0xd4 == last_char[0])
											||(0xd5 == last_char[0])||(0xd6 == last_char[0]))
											{
												clear_flag = gx_atsc_cc_clear_last_character(0);
											}
									}
								break;
								/*u*/
							case 0xf9: 
							case 0xfa:
							case 0xfb:
							case 0xfc:
								/*U*/
							case 0xd9:
							case 0xda:
							case 0xdb:
							case 0xdc:
								if (1 == strlen(last_char))
									{
									if (('u' == last_char[0])||('U' == last_char[0])
										||(0xf9 == last_char[0])||(0xfa == last_char[0])
										||(0xfb == last_char[0])||(0xfc == last_char[0])
										||(0xd9 == last_char[0])||(0xda == last_char[0])
										||(0xdb == last_char[0])||(0xdc == last_char[0]))
										{
											clear_flag = gx_atsc_cc_clear_last_character(0);
										}
									}
								break;
							default:
								break;
						}

					if (1 == clear_flag)
						{
							if ((character[i]>=0x20) && (character[i]<=0x7f))
							{
								show_character[0]=character[i];
								gx_atsc_cc_draw_character(show_character,0);
							}
							else 
							{
								uint32_t uni_code = 0;
								uni_code = character[i];
								if(uni_code != 0){
									//gxlogd("%s, %d, ---c1 = 0x%0x, c2 = 0x%0x---,\n", __func__, __LINE__, character[0], character[1]);
									gx_atsc_cc_unicode_to_utf8_one(uni_code,show_character);
									ret = gx_atsc_cc_draw_character(show_character,0);
									if (0 == ret)
										{
											memset(last_char,0,sizeof(last_char));
											last_char[0] = character[i];
										}
									memset(show_character, 0, 3);
								}
							}

							clear_flag = 0;
							continue;
						}
					if (1 == strlen(last_char))
						{
							switch(last_char[0])
								{
									/*
									* ??һ?ַ?Ϊ?????ַ?
									*/
										/*a*/
									case 0xe0:
									case 0xe1:
									case 0xe2:
									case 0xe3:
									case 0xe4:
									case 0xe5:
										/*A*/
									case 0xc0:
									case 0xc1:
									case 0xc2:
									case 0xc3:
									case 0xc4:
									case 0xc5:
										if (('a' == character[i])||('A' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										/*c*/
									case 0xe7:
										/*C*/
									case 0xc7:
										if (('c' == character[i])||('C' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										/*e*/
									case 0xea:
									case 0xeb:
									case 0xe8:
									case 0xe9:
										/*E*/
									case 0xc8:
									case 0xc9:
									case 0xca:
									case 0xcb:
										if (('e' == character[i])||('E' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										/*i*/
									case 0xec:
									case 0xed:
									case 0xee:
									case 0xef:
										/*I*/
									case 0xcc:
									case 0xcd:
									case 0xce:
									case 0xcf:
										if (('i' == character[i])||('I' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										/*n*/
									case 0xf1:
										/*N*/
									case 0xd1:
										if (('n' == character[i])||('N' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										 /*o*/
									case 0xf2:
									case 0xf3:
									case 0xf4:
									case 0xf5:
									case 0xf6:
										/*O*/
									case 0xd2:
									case 0xd3:
									case 0xd4:
									case 0xd5:
									case 0xd6:
										if (('o' == character[i])||('O' == character[i]))
											{
												character[i]=0x0;
											}
										break;
										/*u*/
									case 0xf9:
									case 0xfa:
									case 0xfb:
									case 0xfc:
										/*U*/
									case 0xd9:
									case 0xda:
									case 0xdb:
									case 0xdc:
										if (('u' == character[i])||('U' == character[i]))
											{
												character[i]=0x0;
											}
										break;
									default:
										break;
								}
						}

					
					if ((character[i]>=0x20) && (character[i]<=0x7f))
						{
							show_character[0]=character[i];
							gx_atsc_cc_draw_character(show_character,0);
						}
					else 
					{
						uint32_t uni_code = 0;
						uni_code = character[i];
						if(uni_code != 0){
							//gxlogd("%s, %d, ---c1 = 0x%0x, c2 = 0x%0x---,\n", __func__, __LINE__, character[0], character[1]);
							gx_atsc_cc_unicode_to_utf8_one(uni_code,show_character);
							ret = gx_atsc_cc_draw_character(show_character,0);
							if (0 == ret)
								{
									memset(last_char,0,sizeof(last_char));
									last_char[0] = character[i];
								}
							memset(show_character, 0, 3);
						}
					}
				}
			}
			else
			{
				uint32_t uni_code = 0;
				for (i=0; i<strlen(character);i++){
					uni_code = uni_code << 8;
					uni_code |= character[i];
				}
				if(uni_code != 0){
					//gxlogd("%s, %d, ---c1 = 0x%0x, c2 = 0x%0x---,\n", __func__, __LINE__, character[0], character[1]);
					gx_atsc_cc_unicode_to_utf8_one(uni_code,show_character); 
					gx_atsc_cc_draw_character(show_character,0);
					memset(last_char,0,sizeof(last_char));
					memset(character, 0, 3);
				}
			}

		}
	else
		{
			memset(last_char,0,sizeof(last_char));
		}
	
	return 0;
}

int gx_atsc_cc_608_parse(char* data, int cc_count)
{
	int ret  = -1;
	int i=0;
	uint8_t cc_type=0x0;
	char* parse_data = NULL;

	if ((NULL == data)||(0 == cc_count))
		{
//			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_608_parse para error\n"));
			return -1;
		}

	for (i=0; i<cc_count/3;i++)
		{
			cc_type = data[i*3];
			switch(cc_type)
				{
					case 0x00:
					case 0x01:
						parse_data = data+i*3;
						if ( 0  == memcmp(last_unit_data,parse_data,3))
							{
								if (((parse_data[1]&0x70)>0x0)&&((parse_data[1]&0x7f)<=0x1f))
									{
										/*
										*
										*/
										break;
									}
							}
						ret = gx_atsc_cc_608_unit_parse(parse_data);
						memcpy(last_unit_data,parse_data,3);
						break;
					case 0x02:
					case 0x03:
						break;
					default:
						break;
				}
			
		}
	return 0;
}






