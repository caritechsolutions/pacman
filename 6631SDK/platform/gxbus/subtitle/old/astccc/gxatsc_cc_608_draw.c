/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2020.02.28		  zhouhm 			 creation
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
#define ATSC_CC_608_PEN_STANDARD_MAX_COLUMN (32)
#define ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN (32)
PROPER_ORDER_OF_DATA_T proper_order=STYLE_INVALID;
static uint8_t erase_memory_flag = FALSE;
extern GuiSurface *spp_char_window[CC_WINDOW_MAX];
extern GuiSurface *layer_char_window[CC_WINDOW_MAX];
extern char *layer_name[CC_WINDOW_MAX];
extern GXGDI_Rect spp_char_rect[CC_WINDOW_MAX];

int gx_atsc_cc_608_miscellaneous_control_codes_action(MISCELLANEOUS_CONTROL_CODE_T control_code)
{
	uint32_t i = 0;
	int font_height = 0;
	int font_width = 0;	
	uint8_t row_count=0;
	GXGDI_Rect rect={0};
	switch(control_code)
		{
			case MISCELLANEOUS_RCL:
					DRIVER_ATSC_CC_DBG(("Resume caption loading,Pop-On Style\n"));
					if (POP_ON_STYLE != proper_order)
						{
							gx_atsc_cc_delete_window(0,0);
							pen_color.flag = false;
						}
					proper_order=POP_ON_STYLE;
					window_define[0].visible=FALSE;
	//						window_define[0].flag=FALSE;
					window_define[0].anchor_ID=/*ANCHOR_ID_LOWER_LEFT*/ANCHOR_ID_UPPER_LEFT;
					window_attribute[0].scroll_direction=DIRECTION_INVALID;
	//						window_define[0].anchor_vertical = 0;
					break;
			case MISCELLANEOUS_BS:
					DRIVER_ATSC_CC_DBG(("Backspace.\n"));
					gx_atsc_cc_clear_last_character(0);
					break;
			case MISCELLANEOUS_AOF:		
					DRIVER_ATSC_CC_DBG(("Reserved (formerly Alarm Off).\n"));
					break;
			case MISCELLANEOUS_AON:
					DRIVER_ATSC_CC_DBG(("Reserved (formerly Alarm On).\n"));
					break;
			case MISCELLANEOUS_DER:
					DRIVER_ATSC_CC_DBG(("Delete to End of Row.\n"));
					gx_atsc_cc_delete_to_end_of_low(0);
					break;
			case MISCELLANEOUS_RU2:
			case MISCELLANEOUS_RU3:
			case MISCELLANEOUS_RU4:
					if (ROLL_UP_STYLE != proper_order)
						{
							gx_atsc_cc_delete_window(0,0);
							pen_color.flag = false;
						}
					proper_order=ROLL_UP_STYLE;
					gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
					if (0 == font_height)
						{
							DRIVER_ATSC_CC_ERROR(("font_height = 0	error \n"));	
							break;
						}
//					window_define[0].visible=TRUE;
//					window_define[0].flag=TRUE;
//					window_define[0].anchor_ID=ANCHOR_ID_LOWER_LEFT;
//					window_attribute[0].scroll_direction=BOTTOM_TO_TOP;
					if (MISCELLANEOUS_RU2 == control_code)
						{
							DRIVER_ATSC_CC_DBG(("Roll-Up Captions-2 Rows,Roll-Up Style\n"));
//							if (2 == window_define[0].row_count)
//								break;
							row_count=2;
							pen_location.row = row_count-1;
						}
					else
						if (MISCELLANEOUS_RU3 == control_code)
							{
								DRIVER_ATSC_CC_DBG(("Roll-Up Captions-3 Rows,Roll-Up Style\n"));								
//								if (3 == window_define[0].row_count)
//									break;
								row_count=3;
								pen_location.row = row_count-1;
							}
						else
							if (MISCELLANEOUS_RU4 == control_code)
								{
									DRIVER_ATSC_CC_DBG(("Roll-Up Captions-4 Rows,Roll-Up Style\n")); 																
//									if (4 == window_define[0].row_count)
//										break;
									row_count=4;
									pen_location.row = row_count-1;
								}
					DRIVER_ATSC_CC_DBG(("pen_location.row=%d row_count=%d\n",
						pen_location.row,row_count));
					if (FALSE == pen_color.flag)
						{
							window_define[0].row_count=row_count;
							break;
//							window_define[0].anchor_vertical=15-1;
//							DRIVER_ATSC_CC_DBG(("window_define[0].anchor_vertical=%d\n",window_define[0].anchor_vertical));								
						}
//					if ((TRUE == window_define[0].flag)&&(window_define[0].row_count==row_count))
//						break;
					if (PEN_LARGE == pen_attribute.pen_size)
						{
							gx_atsc_cc_creat_window(0,window_define[0],row_count,ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN);
							window_define[0].column_count=ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN;								
						}
					else
						{
							gx_atsc_cc_creat_window(0,window_define[0],row_count,ATSC_CC_608_PEN_STANDARD_MAX_COLUMN);
							window_define[0].column_count=ATSC_CC_608_PEN_STANDARD_MAX_COLUMN;			
						}
					window_define[0].row_count=row_count;
					break;
			case MISCELLANEOUS_FON:	
					DRIVER_ATSC_CC_DBG(("Flash On.\n"));
					break;
			case MISCELLANEOUS_RDC:					
					DRIVER_ATSC_CC_DBG(("Resume Direct Captioning,Paint-On Style\n"));
					if (PAINT_ON_STYLE != proper_order)
						{
							gx_atsc_cc_delete_window(0,0);
							pen_color.flag = false;
						}
					proper_order=PAINT_ON_STYLE;
					window_define[0].visible=FALSE;
//						window_define[0].flag=FALSE;
					window_define[0].anchor_ID=/*ANCHOR_ID_LOWER_LEFT*/ANCHOR_ID_UPPER_LEFT;
					window_attribute[0].scroll_direction=DIRECTION_INVALID;
//						window_define[0].anchor_vertical = 0;
					break;
			case MISCELLANEOUS_TR:
					DRIVER_ATSC_CC_DBG(("Text Restart.\n"));
					break;
			case MISCELLANEOUS_RTD:
					DRIVER_ATSC_CC_DBG(("Resume Text Display.\n"));
					break;
			case MISCELLANEOUS_EDM:
					DRIVER_ATSC_CC_DBG(("Erase Displayed Memory.\n"));
					{
						if (POP_ON_STYLE == proper_order)
							{
								rect.x=atsc_cc_para.rect.x;
								rect.y=atsc_cc_para.rect.y;
								rect.w=atsc_cc_para.rect.width;
								rect.h=atsc_cc_para.rect.height;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
								gx_clear_cc_subtitle_spp(rect,spp_cc_subtitle_surface);
#else
								gx_atsc_cc_clear_window(0,layer_char_window[0]);
#endif
								erase_memory_flag = FALSE;
								break;
							}
						if (ROLL_UP_STYLE == proper_order)
							{
								gx_atsc_cc_delete_window(0,1);
								pen_color.flag = false;
								break;
							}
						if (FALSE == window_define[0].flag)
							break;
						if (TRUE == window_define[0].visible)
							{
								gx_atsc_cc_clear_window(0,spp_char_window[0]);
								if (TRUE == window_define[0].visible)
									{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
										gx_atsc_cc_clear_window(0,spp_cc_subtitle_surface);
										rect.x=atsc_cc_para.rect.x;
										rect.y=atsc_cc_para.rect.y;
										rect.w=atsc_cc_para.rect.width;
										rect.h=atsc_cc_para.rect.height;
										gx_clear_cc_subtitle_spp(rect,spp_cc_subtitle_surface);
#else
										gx_atsc_cc_clear_window(0,layer_char_window[0]);
#endif
									}
								for (i=0; i< window_define[0].row_count;i++)
									{
										cc_row_para[0][i].width = 0;
										cc_row_para[0][i].max_width = 0;
									}							
							}
						else
							{
								rect.x=atsc_cc_para.rect.x;
								rect.y=atsc_cc_para.rect.y;
								rect.w=atsc_cc_para.rect.width;
								rect.h=atsc_cc_para.rect.height;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
								gx_clear_cc_subtitle_spp(rect,spp_cc_subtitle_surface);
#else
								gx_atsc_cc_clear_window(0,layer_char_window[0]);
#endif
							}
					}
					break;
			case MISCELLANEOUS_CR:
					DRIVER_ATSC_CC_DBG(("Carriage Return.\n"));
					gx_atsc_cc_scroll(0,BOTTOM_TO_TOP);
					break;
			case MISCELLANEOUS_ENM:
					DRIVER_ATSC_CC_DBG(("Erase Non-Displayed Memory.\n"));
					gx_atsc_cc_clear_window(0,spp_char_window[0]);
					for (i=0; i< window_define[0].row_count;i++)
						{
							cc_row_para[0][i].width = 0;
							cc_row_para[0][i].max_width = 0;
						}							
//						memset(&cc_row_para[0],0,ROW_MAX*sizeof(row_para_t));
					if (POP_ON_STYLE == proper_order)
						{
							erase_memory_flag = TRUE;
						}
//					break;
			case MISCELLANEOUS_EOC:
					DRIVER_ATSC_CC_DBG(("End of Caption (Flip Memories).\n"));
					if (TRUE == window_define[0].flag)
						{
							if (TRUE == erase_memory_flag)
								{
									rect.x=atsc_cc_para.rect.x;
									rect.y=atsc_cc_para.rect.y;
									rect.w=atsc_cc_para.rect.width;
									rect.h=atsc_cc_para.rect.height;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
									gx_clear_cc_subtitle_spp(rect,spp_cc_subtitle_surface);
#else
									gx_atsc_cc_clear_window(0,layer_char_window[0]);
#endif
									erase_memory_flag = FALSE;
								}
							gx_atsc_cc_display_window(0);
							if (POP_ON_STYLE != proper_order)
								{
									window_define[0].visible=TRUE;
								}
							gx_atsc_cc_delete_window(0,0);
							pen_color.flag = false;
						}
					break;
			case MISCELLANEOUS_TO1:			
					DRIVER_ATSC_CC_DBG(("Tab Offset 1 Column.\n"));
					gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
					if (0 == font_width)
						{
							DRIVER_ATSC_CC_ERROR(("font_width = 0  error \n")); 
							break;
						}
					pen_location.column=4;
					DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
					cc_row_para[0][pen_location.row].width = 4*font_width;
					if ( cc_row_para[0][pen_location.row].width > cc_row_para[0][pen_location.row].max_width )
						cc_row_para[0][pen_location.row].max_width = cc_row_para[0][pen_location.row].width;
					DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
						pen_location.row,cc_row_para[0][pen_location.row].width));
					break;
			case MISCELLANEOUS_TO2:
					DRIVER_ATSC_CC_DBG(("Tab Offset 2 Columns.\n"));
					gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
					if (0 == font_width)
						{
							DRIVER_ATSC_CC_ERROR(("font_width = 0  error \n")); 
							break;
						}
					pen_location.column=8;
					DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
					cc_row_para[0][pen_location.row].width = 8*font_width;
					if ( cc_row_para[0][pen_location.row].width > cc_row_para[0][pen_location.row].max_width )
						cc_row_para[0][pen_location.row].max_width = cc_row_para[0][pen_location.row].width;
					DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
						pen_location.row,cc_row_para[0][pen_location.row].width));
					break;
			case MISCELLANEOUS_TO3:
					DRIVER_ATSC_CC_DBG(("Tab Offset 3 Columns.\n"));
					gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
					if (0 == font_width)
						{
							DRIVER_ATSC_CC_ERROR(("font_width = 0  error \n")); 
							break;
						}
					pen_location.column=12;
					DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
					cc_row_para[0][pen_location.row].width = 12*font_width;
					if ( cc_row_para[0][pen_location.row].width > cc_row_para[0][pen_location.row].max_width )
						cc_row_para[0][pen_location.row].max_width = cc_row_para[0][pen_location.row].width;
					DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
						pen_location.row,cc_row_para[0][pen_location.row].width));
					break;
			default:
				break;
		}
	
	return 0;
	
}

int gx_atsc_cc_608_preamble_address_codes_action(uint8_t first_code_pair,uint8_t second_code_pair)
{
	int32_t font_height=0;
	int32_t font_width=0;
	uint8_t anchor_vertical=0;
	uint8_t old_anchor_vertical=0;
	uint8_t row_count=0;
	GXGDI_Rect src_rect={0};
	GXGDI_Rect dst_rect={0};
	int32_t ret = -1;
	int32_t i = 0;
	uint32_t width=0;
	uint32_t height=0;

	switch(first_code_pair)
		{
			case PREAMBLE_ROW_1:
			case PREAMBLE_ROW_2:
			case PREAMBLE_ROW_3:
			case PREAMBLE_ROW_4:
			case PREAMBLE_ROW_5:
			case PREAMBLE_ROW_6:
			case PREAMBLE_ROW_7:
			case PREAMBLE_ROW_8:
			case PREAMBLE_ROW_9:
			case PREAMBLE_ROW_10:
			case PREAMBLE_ROW_11:
			case PREAMBLE_ROW_12:
			case PREAMBLE_ROW_13:
			case PREAMBLE_ROW_14:
			case PREAMBLE_ROW_15:
				if (STYLE_INVALID == proper_order)
					break;
				anchor_vertical = first_code_pair;
				window_define[0].anchor_horizontal=0;

				if ((POP_ON_STYLE == proper_order)||(PAINT_ON_STYLE == proper_order))
					{
						if ((FALSE == window_define[0].flag)
							||(anchor_vertical < window_define[0].anchor_vertical))
							{
								row_count = 15-anchor_vertical;
								window_define[0].anchor_vertical=anchor_vertical;
								DRIVER_ATSC_CC_DBG(("window_define[0].anchor_vertical=%d\n",window_define[0].anchor_vertical));
								window_define[0].flag = TRUE;
								if (PEN_LARGE == pen_attribute.pen_size)
									{
										gx_atsc_cc_creat_window(0,window_define[0],row_count,ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN);
										window_define[0].column_count=ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN;												
									}
								else
									{
										gx_atsc_cc_creat_window(0,window_define[0],row_count,ATSC_CC_608_PEN_STANDARD_MAX_COLUMN);
										window_define[0].column_count=ATSC_CC_608_PEN_STANDARD_MAX_COLUMN;
									}
								window_define[0].row_count=row_count;
								pen_location.row = 0;
								DRIVER_ATSC_CC_DBG(("pen_location.row=%d row_count=%d\n",
								pen_location.row,row_count));
							}
						else
							{
								pen_location.row = anchor_vertical - window_define[0].anchor_vertical;
								DRIVER_ATSC_CC_DBG(("pen_location.row=%d\n",pen_location.row));
							}
						if (PAINT_ON_STYLE == proper_order)
							{
								window_define[0].visible= TRUE;
							}
						
					}
				else
					{
//						window_define[0].anchor_vertical=anchor_vertical;
						DRIVER_ATSC_CC_DBG(("window_define[0].anchor_vertical=%d\n",window_define[0].anchor_vertical));
					}
				pen_color.flag=TRUE;
				pen_color.foreground_color=COLOR_WHITE;
				pen_attribute.italic=FALSE;
				pen_attribute.underline=FALSE;
				pen_location.column=0;
				DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
				switch(second_code_pair)
					{
						case PREAMBLE_WHITE:
							pen_color.foreground_color=COLOR_WHITE;
							break;
						case PREAMBLE_WHITE_UNDERLINE:
							pen_color.foreground_color=COLOR_WHITE;
							pen_attribute.underline=TRUE;				
							break;
						case PREAMBLE_GREEN:
							pen_color.foreground_color=COLOR_GREEN;
							break;
						case PREAMBLE_GREEN_UNDERLINE:
							pen_color.foreground_color=COLOR_GREEN;
							pen_attribute.underline=TRUE;
							break;
						case PREAMBLE_BLUE:
							pen_color.foreground_color=COLOR_BLUE;	
							break;
						case PREAMBLE_BLUE_UNDERLINE:
							pen_color.foreground_color=COLOR_BLUE;
							pen_attribute.underline=TRUE;
							break;
						case PREAMBLE_CYAN:
							pen_color.foreground_color=COLOR_CYAN;
							break;
						case PREAMBLE_CYAN_UNDERLINE:
							pen_color.foreground_color=COLOR_CYAN;
							pen_attribute.underline=TRUE;
							break;								
						case PREAMBLE_RED:
							pen_color.foreground_color=COLOR_RED;
							break;
						case PREAMBLE_RED_UNDERLINE:
							pen_color.foreground_color=COLOR_RED;
							pen_attribute.underline=TRUE;
							break;
						case PREAMBLE_YELLOW:
							pen_color.foreground_color=COLOR_YELLOW;
							break;
						case PREAMBLE_YELLOW_UNDERLINE:
							pen_color.foreground_color=COLOR_YELLOW;
							pen_attribute.underline=TRUE;
							break;								
						case PREAMBLE_MAGENTA:
							pen_color.foreground_color=COLOR_MAGENTA;
							break;
						case PREAMBLE_MAGENTA_UNDERLINE:
							pen_color.foreground_color=COLOR_MAGENTA;
							pen_attribute.underline=TRUE;
							break;
						case PREAMBLE_WHITE_ITALICS:
							pen_color.foreground_color=COLOR_WHITE;
							pen_attribute.italic=TRUE;
							break;							
						case PREAMBLE_WHITE_ITALICS_UNDERLINE:
							pen_color.foreground_color=COLOR_WHITE;
							pen_attribute.italic=TRUE;
							pen_attribute.underline=TRUE;
							break;
						case PREAMBLE_INDENT_0:
							pen_location.column=0;
							break;
						case PREAMBLE_INDENT_0_UNDERLINE:
							pen_location.column=0;
							pen_attribute.underline=TRUE;									
							break;
						case PREAMBLE_INDENT_4:
							pen_location.column=4;
							break;
						case PREAMBLE_INDENT_4_UNDERLINE:
							pen_location.column=4;
							pen_attribute.underline=TRUE;										
							break;
						case PREAMBLE_INDENT_8:
							pen_location.column=8;
							break;
						case PREAMBLE_INDENT_8_UNDERLINE:
							pen_location.column=8;
							pen_attribute.underline=TRUE;	
							break;
						case PREAMBLE_INDENT_12:
							pen_location.column=12;
							break;
						case PREAMBLE_INDENT_12_UNDERLINE:
							pen_location.column=12;
							pen_attribute.underline=TRUE;	
							break;
						case PREAMBLE_INDENT_16:
							pen_location.column=16;
							break;
						case PREAMBLE_INDENT_16_UNDERLINE:
							pen_location.column=16;
							pen_attribute.underline=TRUE;	
							break;
						case PREAMBLE_INDENT_20:
							pen_location.column=20;
							break;
						case PREAMBLE_INDENT_20_UNDERLINE:
							pen_location.column=20;
							pen_attribute.underline=TRUE;	
							break;
						case PREAMBLE_INDENT_24:
							pen_location.column=24;
							break;
						case PREAMBLE_INDENT_24_UNDERLINE:
							pen_location.column=24;
							pen_attribute.underline=TRUE;	
							break;
						case PREAMBLE_INDENT_28:
							pen_location.column=28;
							break;
						case PREAMBLE_INDENT_28_UNDERLINE:
							pen_location.column=28;
							pen_attribute.underline=TRUE;
							break;
						default:
							break;

						
					}
				DRIVER_ATSC_CC_DBG(("pen_location.column=%d\n",pen_location.column));
				gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
				cc_row_para[0][pen_location.row].width = pen_location.column*font_width;
				if ( cc_row_para[0][pen_location.row].width >  cc_row_para[0][pen_location.row].max_width )
					cc_row_para[0][pen_location.row].max_width = cc_row_para[0][pen_location.row].width;
				DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",pen_location.row,cc_row_para[0][pen_location.row].width));
				if (0 == window_define[0].row_count)
					window_define[0].row_count=1;
				old_anchor_vertical = window_define[0].anchor_vertical;
				if (ROLL_UP_STYLE == proper_order)
					{
						if (FALSE == window_define[0].flag)
							{
								window_define[0].visible=TRUE;
								window_define[0].flag=TRUE;
								window_define[0].anchor_ID=ANCHOR_ID_LOWER_LEFT;
								window_attribute[0].scroll_direction=BOTTOM_TO_TOP;
								window_define[0].anchor_vertical=anchor_vertical;
								if (PEN_LARGE == pen_attribute.pen_size)
									{
										gx_atsc_cc_creat_window(0,window_define[0],window_define[0].row_count,ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN);
										window_define[0].column_count=ATSC_CC_608_PEN_DOUBLE_MAX_COLUMN;
									}
								else
									{
										gx_atsc_cc_creat_window(0,window_define[0],window_define[0].row_count,ATSC_CC_608_PEN_STANDARD_MAX_COLUMN);
										window_define[0].column_count=ATSC_CC_608_PEN_STANDARD_MAX_COLUMN;
									}
								break;
							}

						if (old_anchor_vertical!=anchor_vertical)
							{
								/*
								* Roll-up styple need support moved without being erased command
								*/
								if (TRUE == window_define[0].flag)
									{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
										gx_atsc_cc_clear_window(0,spp_cc_subtitle_surface);
#endif
									}
								DRIVER_ATSC_CC_DBG(("window_define[0].anchor_vertical old=%d new=%d\n",old_anchor_vertical,anchor_vertical));
							}
						window_define[0].anchor_vertical=anchor_vertical;
					}
				gx_atsc_cc_init_row_para(0,window_define[0].row_count,font_height);
				if (ROLL_UP_STYLE == proper_order)
					{
						if (old_anchor_vertical!=anchor_vertical)
							{
								/*
								* Roll-up styple need support moved without being erased
								*/
								if (TRUE == window_define[0].flag)
									{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
										/*
										* move to new anchor vertical position
										*/
										width = 0;
										height = 0;
										for (i=0;i<window_define[0].row_count;i++)
											{
												if (cc_row_para[0][i].width>cc_row_para[0][i].max_width)
													cc_row_para[0][i].max_width = cc_row_para[0][i].width;		
												if (cc_row_para[0][i].max_width>width)
													width = cc_row_para[0][i].max_width;
												height += cc_row_para[0][i].height;
											}
										src_rect.x = 0;
										src_rect.y = 0;
										src_rect.w = width;
										src_rect.h = height;
										dst_rect.x = cc_row_para[0][0].top_x;
										dst_rect.y = cc_row_para[0][0].top_y;
										dst_rect.w = width;
										dst_rect.h = height;
										if (NULL != spp_char_window[0])
											{
												spp_char_rect[0].x = cc_row_para[0][0].top_x;
												spp_char_rect[0].y = cc_row_para[0][0].top_y;
												if ((width >0)&&(height>0))
													{
														GXGDI_Lock();
														ret =  GXGDI_Blit(spp_char_window[0] , &src_rect, spp_cc_subtitle_surface, &dst_rect, GDI_BLIT_COPY);
														GXGDI_Unlock();
														if (GXCORE_SUCCESS != ret)
															{
																DRIVER_ATSC_CC_ERROR(("GXGDI_FillRect failed dst_rect.x =%d dst_rect.y=%d dst_rect.w=%d dst_rect.h=%d\n"
																	,dst_rect.x,dst_rect.y,dst_rect.w,dst_rect.h));
															}
													}
											}
#else
										if (NULL != layer_name[0])
											{
												GXGDI_Lock();
												GxGDI_LayerSetPosition((const char *)layer_name[0], cc_row_para[0][0].top_x, cc_row_para[0][0].top_y);
												GXGDI_Unlock();
											}
#endif
									}
							}
					}	
				break;
			default:
				break;
		}
	
	return 0;	
}

int gx_atsc_cc_608_background_foreground_attributes_action(BACK_FORE_GROUND_CODES_T back_fore_ground_code)
{
	switch(back_fore_ground_code)
		{
			/*
			* Background and Foreground Attribute Codes
			*/
			case BACKGROUND_WHITE_OPAQUE:
				pen_color.background_color = COLOR_WHITE;
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case BACKGROUND_WHITE_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_WHITE;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;							
				break;
			case BACKGROUND_GREEN_OPAQUE:
				pen_color.background_color = COLOR_GREEN;
				pen_color.background_opacity = OPACITY_SOLID;	
				break;
			case BACKGROUND_GREEN_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_GREEN;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_BLUE_OPAQUE:
				pen_color.background_color = COLOR_BLUE;
				pen_color.background_opacity = OPACITY_SOLID;							
				break;
			case BACKGROUND_BLUE_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_BLUE;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_CYAN_OPAQUE:
				pen_color.background_color = COLOR_CYAN;
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case BACKGROUND_CYAN_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_CYAN;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_RED_OPAQUE:
				pen_color.background_color = COLOR_RED;
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case BACKGROUND_RED_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_RED;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_YELLOW_OPAQUE:
				pen_color.background_color = COLOR_YELLOW;
				pen_color.background_opacity = OPACITY_SOLID;
				break;
			case BACKGROUND_YELLOW_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_YELLOW;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_MAGENTA_OPAQUE:
				pen_color.background_color = COLOR_MAGENTA;
				pen_color.background_opacity = OPACITY_SOLID;
				break;	
			case BACKGROUND_MAGENTA_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_MAGENTA;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;	
			case BACKGROUND_BLACK_OPAQUE:
				pen_color.background_color = COLOR_BLACK;
				pen_color.background_opacity = OPACITY_SOLID;
				break;	
			case BACKGROUND_BLACK_SEMI_TRANSPARENT:
				pen_color.background_color = COLOR_BLACK;
				pen_color.background_opacity = OPACITY_TRANSLUCENT;
				break;
			case BACKGROUND_TRANSPARENT:
				pen_color.background_opacity=OPACITY_TRANSPARENT;
				break;
			case FOREGROUND_BLACK:
				pen_color.foreground_color = COLOR_BLACK;
				break;
			case FOREGROUND_BLACK_UNDERLINE:
				pen_color.foreground_color = COLOR_BLACK;
				pen_attribute.underline=TRUE;
				break;
			default:
				break;
	}

	return 0;
}

int gx_atsc_cc_608_mid_row_codes_action(MID_ROW_CODES_T mid_row_codes)
{
	if ((mid_row_codes>=MID_WHITE)&&(mid_row_codes<=MID_ITALICS_UNDERLINE))
		{
			pen_attribute.italic=FALSE;
			pen_attribute.underline=FALSE;
		}
	switch(mid_row_codes)
		{
			case MID_WHITE:
				pen_color.foreground_color = COLOR_WHITE;
				break;
			case MID_WHITE_UNDERLINE:
				pen_color.foreground_color = COLOR_WHITE;
				pen_attribute.underline=TRUE;						
				break;
			case MID_GREEN:
				pen_color.foreground_color = COLOR_GREEN;
				break;
			case MID_GREEN_UNDERLINE:
				pen_color.foreground_color = COLOR_GREEN;
				pen_attribute.underline=TRUE;
				break;
			case MID_BLUE:
				pen_color.foreground_color = COLOR_BLUE;
				break;
			case MID_BLUE_UNDERLINE:
				pen_color.foreground_color = COLOR_BLUE;
				pen_attribute.underline=TRUE;
				break;
			case MID_CYAN:
				pen_color.foreground_color = COLOR_CYAN;
				break;
			case MID_CYAN_UNDERLINE:
				pen_color.foreground_color = COLOR_CYAN;
				pen_attribute.underline=TRUE;
				break;
			case MID_RED:
				pen_color.foreground_color = COLOR_RED;
				break;
			case MID_RED_UNDERLINE:
				pen_color.foreground_color = COLOR_RED;
				pen_attribute.underline=TRUE;
				break;
			case MID_YELLOW:
				pen_color.foreground_color = COLOR_YELLOW;
				break;
			case MID_YELLOW_UNDERLINE:
				pen_color.foreground_color = COLOR_YELLOW;
				pen_attribute.underline=TRUE;
				break;
			case MID_MAGENTA:
				pen_color.foreground_color = COLOR_MAGENTA;
				break;	
			case MID_MAGENTA_UNDERLINE:
				pen_color.foreground_color = COLOR_MAGENTA;
				pen_attribute.underline=TRUE;
				break;	
			case MID_ITALICS:
				pen_attribute.italic=TRUE;
				break;	
			case MID_ITALICS_UNDERLINE:
				pen_attribute.italic=TRUE;
				pen_attribute.underline=TRUE;
				break;
			default:
				break;
		}

	return 0;

}

int gx_atsc_cc_608_pen_attribute_action(CC_PEN_SIZE pen_size)
{
	switch(pen_size)
		{
			case PEN_STANDARD:
				pen_attribute.pen_size=PEN_STANDARD;
				break;
			case PEN_LARGE:
				pen_attribute.pen_size=PEN_LARGE;			
				break;
			default:
				pen_attribute.pen_size=PEN_STANDARD;
				break;
		}

	return 0;

}

int gx_atsc_cc_608_set_pen_opacity_type(OPACITY_TYPE opacity_type)
{
	pen_color.background_opacity=opacity_type;
	return 0;
}

OPACITY_TYPE gx_atsc_cc_608_get_pen_opacity_type(void)
{
	return pen_color.background_opacity;
}










