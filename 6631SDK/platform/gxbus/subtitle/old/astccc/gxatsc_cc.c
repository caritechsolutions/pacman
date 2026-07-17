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
#include "gui_core.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gxavdev.h"
#include "../scte/gxatsc_cc_inner.h"
#include "../scte/gx_cc_subtitle_common.h"
#include "../scte/gx_cc_subtitle_draw.h"
#include "gx_txt_subtitle.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_config.h"
#include "gx_subtitle_atsccc.h"

extern GuiCore gui;

GxAtsccc_Param atsc_cc_para={0};
GuiSurface *spp_char_window[CC_WINDOW_MAX]={NULL};
GuiSurface *layer_char_window[CC_WINDOW_MAX]={NULL};
char *layer_name[CC_WINDOW_MAX]={NULL};
GXGDI_Rect layer_char_rect[CC_WINDOW_MAX]={{0}};
GXGDI_Rect spp_char_rect[CC_WINDOW_MAX]={{0}};
static CC_STATUS cc_status=CC_OFF;
static char cc_buffer[CC_BUFFER_MAX]={0};
atsc_cc_window_attribute_t window_attribute[CC_WINDOW_MAX];
atsc_cc_window_define_t window_define[CC_WINDOW_MAX];
atsc_cc_pen_attribute_t pen_attribute={0};
atsc_cc_pen_color_t pen_color={0};
atsc_cc_pen_location_t pen_location={0};
char last_unit_data[3]={0};
static GXGDI_Rect last_char_rect[CC_WINDOW_MAX];
char last_char[3]={0}; /*支持双字节字符*/
static char parse_608_unit[CC_608_UNIT_MAX*3]={0};
short valid_cc_708_num = 0;
static char parse_708_unit[CC_708_UNIT_MAX*3]={0};
static char parse_708_unit_bak[CC_708_UNIT_MAX*3]={0};
static handle_t cc_lock=-1;
row_para_t cc_row_para[CC_WINDOW_MAX][ROW_MAX];
static char* player_check=NULL;

GX_SUBTITLE_ATSCCC_INFO cc_info = {0};
static int32_t font_width_small=0;
static int32_t font_width_standard=0;
static int32_t font_width_large=0;

extern uint8_t last_sequence_number[8];
extern int hd_fillrect(GuiSurface *surface, GAL_Rect * dstrect, unsigned int color);
extern int gal_color2index(void *screen, int color);

int gx_atsc_cc_hide(void)
{
	hd_enable_img(FALSE);
	hd_enable_video(TRUE);

	return 0;
}

int gx_atsc_cc_show(void)
{
	hd_enable_img(TRUE);
	hd_enable_video(FALSE);

	return 0;
}

static void gx_atsc_cc_lock_init(void)
{
	if (-1 == cc_lock)
		GxCore_MutexCreate(&cc_lock);
}

static void gx_atsc_cc_lock(void)
{
	if (-1 != cc_lock)
		GxCore_MutexLock(cc_lock);
}

static void gx_atsc_cc_unlock(void)
{
	if (-1 != cc_lock)
		GxCore_MutexUnlock(cc_lock);
}

int gx_atsc_cc_add(GX_SUBTITLE_ATSCCC_SERVICE service)
{
	int i =0;
	int j = 0;
	GX_SUBTITLE_ATSCCC_SERVICE temp = GX_SUBTITLE_ATSC_608_CC1;

	gx_atsc_cc_lock();
	for (i = 0; i< cc_info.num; i++)
		{
			if ( service == cc_info.service[i] )
				break;
		}

	if (i >= cc_info.num)
		{
			if (cc_info.num < ATSCCC_NUM )
				{
					cc_info.service[cc_info.num] = service;
					cc_info.num = cc_info.num +1;
					
					/*
					* sort by service
					*/
					for (i = 0; i< cc_info.num-1;i++)
						for (j=i+1;j<cc_info.num;j++)
							{
								if (cc_info.service[j]<cc_info.service[i])
									{
										temp = cc_info.service[j];
										cc_info.service[j] = cc_info.service[i];
										cc_info.service[i] = temp;							
									}
							}
				}
		}

	gx_atsc_cc_unlock();

	return 0;
}


static inline int is_ksurface(GuiSurface *surface) {
	return (surface->hw & KSURFACE) ? 1 : 0;
}

int gx_atsc_cc_unicode_to_utf8_one(unsigned long unic, char *pOutput)
{    
	if (pOutput == NULL)
	{
		return (0);
	}

	if ( unic <= 0x0000007F )
	{
		// * U-00000000 - U-0000007F:  0xxxxxxx
		*pOutput     = (unic & 0x7F);
		return 1;
	}
	else if ( unic >= 0x00000080 && unic <= 0x000007FF )
	{
		// * U-00000080 - U-000007FF:  110xxxxx 10xxxxxx
		*(pOutput+1) = (unic & 0x3F) | 0x80;
		*pOutput     = ((unic >> 6) & 0x1F) | 0xC0;
		return 2;
	}
	else if ( unic >= 0x00000800 && unic <= 0x0000FFFF )
	{
		// * U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx
		*(pOutput+2) = (unic & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >>  6) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 12) & 0x0F) | 0xE0;
		return 3;
	}
#if 0
	else if ( unic >= 0x00010000 && unic <= 0x001FFFFF )
	{
		// * U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+3) = (unic & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 12) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 18) & 0x07) | 0xF0;
		return 4;
	}
	else if ( unic >= 0x00200000 && unic <= 0x03FFFFFF )
	{
		// * U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+4) = (unic & 0x3F) | 0x80;
		*(pOutput+3) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 18) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 24) & 0x03) | 0xF8;
		return 5;
	}
	else if ( unic >= 0x04000000 && unic <= 0x7FFFFFFF )
	{
		// * U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*(pOutput+5) = (unic & 0x3F) | 0x80;
		*(pOutput+4) = ((unic >>  6) & 0x3F) | 0x80;
		*(pOutput+3) = ((unic >> 12) & 0x3F) | 0x80;
		*(pOutput+2) = ((unic >> 18) & 0x3F) | 0x80;
		*(pOutput+1) = ((unic >> 24) & 0x3F) | 0x80;
		*pOutput     = ((unic >> 30) & 0x01) | 0xFC;
		return 6;
	}
#endif

	return 0;
}

int32_t gx_atsc_cc_init_row_para(uint8_t window_id,uint16_t row_count,int32_t font_height)
{
	int32_t i = 0;
	uint16_t grid_horizontal_max=0;
	uint16_t grid_vertial_max=0;
	uint16_t grid_horizontal_offset=0;
	uint16_t grid_vertial_offset=0;


	grid_horizontal_offset = atsc_cc_para.rect.width*10/100;
	grid_vertial_offset = atsc_cc_para.rect.height*10/100;
	switch(atsc_cc_para.id)
		{
			case GX_ATSC_608_CC1:
			case GX_ATSC_608_CC2:
			case GX_ATSC_608_CC3:
			case GX_ATSC_608_CC4:
				grid_horizontal_max = GRID_608_HORIZONTAL_16_9_MAX;
				grid_vertial_max = GRID_608_VERTIAL_MAX;
				break;
			case GX_ATSC_708_DTV_SERVICE1:
			case GX_ATSC_708_DTV_SERVICE2:
			case GX_ATSC_708_DTV_SERVICE3:
			case GX_ATSC_708_DTV_SERVICE4:
			case GX_ATSC_708_DTV_SERVICE5:
			case GX_ATSC_708_DTV_SERVICE6:
			default:
				grid_horizontal_max = GRID_708_HORIZONTAL_16_9_MAX;
				grid_vertial_max = GRID_708_VERTIAL_MAX;
				break;
		}

	switch(window_define[window_id].anchor_ID)
			{
				case ANCHOR_ID_UPPER_LEFT:
					for (i = 0; i< row_count;i++)
						{
							if (FALSE == window_define[window_id].relative_positioning)
							{
								cc_row_para[window_id][i].top_x= window_define[window_id].anchor_horizontal*(atsc_cc_para.rect.width-grid_horizontal_offset*2)/grid_horizontal_max;
								cc_row_para[window_id][i].top_y= window_define[window_id].anchor_vertical*(atsc_cc_para.rect.height-grid_vertial_offset*2)/grid_vertial_max+i*font_height;
								cc_row_para[window_id][i].top_x+=grid_horizontal_offset;
								cc_row_para[window_id][i].top_y+=grid_vertial_offset;
								cc_row_para[window_id][i].height= font_height;
							}
						else
							{
								cc_row_para[window_id][i].top_x= window_define[window_id].anchor_horizontal*(atsc_cc_para.rect.width-grid_horizontal_offset*2)/100;
								cc_row_para[window_id][i].top_y= window_define[window_id].anchor_vertical*(atsc_cc_para.rect.height-grid_vertial_offset*2)/100+i*font_height;
								cc_row_para[window_id][i].top_x+=grid_horizontal_offset;
								cc_row_para[window_id][i].top_y+=grid_vertial_offset;
								cc_row_para[window_id][i].height= font_height;
							}
						}
					break;
				case ANCHOR_ID_UPPER_CENTER:
					break;
				case ANCHOR_ID_UPPER_RIGHT:
					break;
				case ANCHOR_ID_MIDDLE_LEFT:
					break;
				case ANCHOR_ID_MIDDLE_CENTER:
					break;
				case ANCHOR_ID_MIDDLE_RIGHT:
					break;
				case ANCHOR_ID_LOWER_LEFT:
					for (i = 0; i< row_count;i++)
						{
							if (FALSE == window_define[window_id].relative_positioning)
							{
								cc_row_para[window_id][row_count-i-1].top_x= window_define[window_id].anchor_horizontal*(atsc_cc_para.rect.width-grid_horizontal_offset*2)/grid_horizontal_max;
								cc_row_para[window_id][row_count-i-1].top_y= window_define[window_id].anchor_vertical*(atsc_cc_para.rect.height-grid_vertial_offset*2)/grid_vertial_max-(i+1)*font_height;
								cc_row_para[window_id][row_count-i-1].top_x+=grid_horizontal_offset;
								cc_row_para[window_id][row_count-i-1].top_y+=grid_vertial_offset;
								cc_row_para[window_id][row_count-i-1].height= font_height;
							}
						else
							{
								cc_row_para[window_id][row_count-i-1].top_x= window_define[window_id].anchor_horizontal*(atsc_cc_para.rect.width-grid_horizontal_offset*2)/100;
								cc_row_para[window_id][row_count-i-1].top_y= window_define[window_id].anchor_vertical*(atsc_cc_para.rect.height-grid_vertial_offset*2)/100-(i+1)*font_height;
								cc_row_para[window_id][row_count-i-1].top_x+=grid_horizontal_offset;
								cc_row_para[window_id][row_count-i-1].top_y+=grid_vertial_offset;
								cc_row_para[window_id][row_count-i-1].height= font_height;
							}
						}
					break;
				case ANCHOR_ID_LOWER_CENTER:
					break;
				case ANCHOR_ID_LOWER_RIGHT:
					break;
				default:
					break;
			}
	return 0;
}

int gx_atsc_cc_creat_window(uint8_t window_id,atsc_cc_window_define_t window_old,uint16_t row_count,uint16_t column_count)
{
	int32_t i = 0;
	int32_t ret=-1;
	int32_t font_height=0;
	int32_t font_width=0;
	GuiSurface *create_surface=NULL;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
	GuiSurface *create_layer_surface=NULL;
	char *create_layer_name=NULL;
#endif
	GAL_Rect create_rect = {0};
	GXGDI_Rect font_rect={0};
	uint8_t spp_bpp=2;
	GxColorFormat spp_color_format = GX_COLOR_FMT_YCBCRA6442;
//	DRIVER_ATSC_CC_DBG((" window_id = %d,row_count=%d column_count=%d window_define[%d].row_count=%d  window_define[%d].column_count=%d \n",
//		window_id,row_count,column_count,window_id,window_define[window_id].row_count,window_id,window_define[window_id].column_count));
#if 0
	if (row_count<window_define[window_id].row_count)
		{

		if (NULL != spp_char_window[window_id])
			{
				gx_atsc_cc_clear_window(window_id,spp_char_window[window_id]);
				hd_free_surface(spp_char_window[window_id]);
				spp_char_window[window_id]=NULL;
			}

			if (TRUE == window_define[window_id].visible)
				gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);

			for (i=0;i<CC_WINDOW_MAX;i++)
				{
					if (i == window_id)
						continue;
					if (TRUE == window_define[i].visible)
						gx_atsc_cc_display_window(i);
				}
		}
#endif
	if ((row_count!=window_define[window_id].row_count)
		||(column_count!=window_define[window_id].column_count)
		||(window_old.anchor_ID != window_define[window_id].anchor_ID)
		||(window_old.relative_positioning != window_define[window_id].relative_positioning)
		||(window_old.anchor_horizontal != window_define[window_id].anchor_horizontal)
		||(window_old.anchor_vertical != window_define[window_id].anchor_vertical))
		{
			gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
			if (FALSE == pen_attribute.italic)
				{
					switch(pen_attribute.pen_size)
						{
							case PEN_SMALL:
								if (font_width_small > font_width)
									{
//										DRIVER_ATSC_CC_ERROR((" font_width = %d,font_width_small=%d \n",font_width,font_width_small));
										font_width = font_width_small;
									}
								break;
							case PEN_STANDARD:
								if (font_width_standard > font_width)
									{
//										DRIVER_ATSC_CC_ERROR((" font_width = %d,font_width_standard=%d \n",font_width,font_width_standard));
										font_width = font_width_standard;
									}
								break;
							case PEN_LARGE:
								if (font_width_large > font_width)
									{
//										DRIVER_ATSC_CC_ERROR((" font_width = %d,font_width_large=%d \n",font_width,font_width_large));
										font_width = font_width_large;
									}
								break;
							default:
								if (font_width_standard > font_width)
									{
//										DRIVER_ATSC_CC_ERROR((" font_width = %d,font_width_standard=%d \n",font_width,font_width_standard));
										font_width = font_width_standard;
									}
								break;
						}
				}
			create_rect.x = 0;
			create_rect.y = 0;
			create_rect.w = column_count*font_width;
			create_rect.h = row_count*font_height;
			DRIVER_ATSC_CC_DBG((" create_rect x=%d y=%d w=%d h=%d\n",create_rect.x,create_rect.y,create_rect.w,create_rect.h));
			GXGDI_Lock();
			create_surface = hd_get_surface0(&create_rect, spp_bpp, spp_color_format, NULL, TRUE);
			GXGDI_Unlock();
			if (NULL == create_surface)
				{
					DRIVER_ATSC_CC_ERROR(("hd_get_surface0 failed\n"));
					return -1;
				}
			font_rect.x = 0;
			font_rect.y = 0;
			font_rect.w = create_rect.w;
			font_rect.h = create_rect.h;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			ret = gx_clear_cc_subtitle_spp(font_rect,create_surface);
//			ret = GXGDI_FillRect(create_surface, &font_rect, 0x00);
			if (GXCORE_SUCCESS != ret)
				{
					GXGDI_Lock();
					hd_free_surface(create_surface);
					GXGDI_Unlock();
					create_surface=NULL;
					DRIVER_ATSC_CC_ERROR(("gx_clear_cc_subtitle_spp failed\n"));
					return -1;
				}
#endif
			if (window_old.anchor_vertical!=window_define[window_id].anchor_vertical)
				{
					/*
					* clear before moved
					*/
					if ((TRUE == window_old.flag)&&(TRUE == window_old.visible ))
						{
							if (NULL != spp_cc_subtitle_surface)
								gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);
						}
				}
				gx_atsc_cc_init_row_para(window_id,row_count,font_height);
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
			if ((NULL != layer_char_window[window_id])&&(NULL != layer_name[window_id]))
				{
					ret = GxGDI_LayerUnregister(layer_name[window_id]);
					GXGDI_Lock();
					hd_free_surface(layer_char_window[window_id]);
					GXGDI_Unlock();
					layer_char_window[window_id]=NULL;
					layer_name[window_id]=NULL;
				}
			GXGDI_Lock();
			create_layer_surface = hd_get_surface(&create_rect, 16, NULL, FALSE);
			if (NULL == create_layer_surface)
				{
					hd_free_surface(create_surface);
					GXGDI_Unlock();
					create_surface=NULL;
					DRIVER_ATSC_CC_ERROR(("hd_get_surface failed\n"));
					return -1;
				}
			GXGDI_Begin();
			GXGDI_FillRect(create_layer_surface, &font_rect, gui.config.gui_trans);
			GXGDI_End();
			GXGDI_Blit(create_layer_surface, (struct GXGDI_Rect*)&create_rect, create_surface, (struct GXGDI_Rect*)&create_rect, GDI_BLIT_COPY);
			GXGDI_Unlock();
			create_layer_name = GxGDI_LayerRegister(create_layer_surface, cc_row_para[window_id][0].top_x, cc_row_para[window_id][0].top_y);
			if(NULL == create_layer_name)
				{
					GXGDI_Lock();
					hd_free_surface(create_surface);
					create_surface=NULL;
					hd_free_surface(create_layer_surface);
					create_layer_surface=NULL;
					GXGDI_Unlock();
					DRIVER_ATSC_CC_ERROR(("GxGDI_LayerRegister failed top_x=%d top_y =%d font_rect.w=%d font_rect.h=%d\n",
						cc_row_para[window_id][0].top_x,cc_row_para[window_id][0].top_y,font_rect.w,font_rect.h));
					return -1;
				}
#endif
				if (NULL != spp_char_window[window_id])
					{
						GXGDI_Rect rect={0};
						GXGDI_Rect src_rect={0};
						GXGDI_Rect dst_rect={0};
						uint32_t bottom = 0;
						uint32_t right = 0;
						int32_t i = 0;
						rect.x=0;
						rect.y=0;
						for (i = 0; i< window_define[window_id].row_count; i++)
							{
								if (cc_row_para[window_id][i].width > cc_row_para[window_id][i].max_width)
									cc_row_para[window_id][i].max_width=cc_row_para[window_id][i].width;
								if (cc_row_para[window_id][i].max_width > rect.w)
									rect.w=cc_row_para[window_id][i].max_width;
							}

						for (i = 0; i< window_define[window_id].row_count; i++)
							{
								rect.h+=cc_row_para[window_id][i].height;
							}

						if ((rect.w>0)&&(rect.h>0))
							{
								memcpy(&src_rect,&rect,sizeof(GXGDI_Rect));
								if (row_count<window_define[window_id].row_count)
									{
										rect.h = create_rect.h;
										src_rect.h = create_rect.h;
										for (i = 0; i< window_define[window_id].row_count-row_count; i++)
											{
												/*
												* reduce the low number, copy top from src_retc.x , src_rect.y
												*/
												src_rect.y+=cc_row_para[window_id][i].height;
											}
									}
								else
									if (row_count>window_define[window_id].row_count)
									{
										/*
										* add low number, copy to bottom
										*/
										for (i = 0; i< row_count - window_define[window_id].row_count; i++)
											{
												/*
												* reduce the low number, copy top from src_retc.x , src_rect.y
												*/
												rect.y+=cc_row_para[window_id][i].height;
											}
										memcpy(&src_rect,&spp_char_rect[window_id],sizeof(GXGDI_Rect));
										src_rect.x = 0;
										src_rect.y = 0;
										
									}
								GXGDI_Lock();
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
								GXGDI_Begin();
								ret =  GXGDI_Blit(spp_char_window[window_id], &src_rect, create_surface, &rect, GDI_BLIT_STRETCH);
								GXGDI_End();
#else
								ret =  GXGDI_Blit(spp_char_window[window_id], &src_rect, create_surface, &rect, GDI_BLIT_COPY);
#endif
								GXGDI_Unlock();
								if (GXCORE_SUCCESS != ret)
									{
										DRIVER_ATSC_CC_ERROR(("GXGDI_Blit failed\n"));
										goto error;
									}
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
							if (window_old.anchor_vertical!=window_define[window_id].anchor_vertical)
								{
									if ((TRUE == window_old.flag)&&(TRUE == window_old.visible ))
										{
											/*
											* move to new anchor vertical position
											*/
											dst_rect.x = cc_row_para[window_id][0].top_x;
											dst_rect.y = cc_row_para[window_id][0].top_y;
											dst_rect.w = rect.w;
											dst_rect.h = rect.h;
											GXGDI_Lock();
											ret =  GXGDI_Blit(create_surface , &rect, spp_cc_subtitle_surface, &dst_rect, GDI_BLIT_COPY);
											GXGDI_Unlock();
											if (GXCORE_SUCCESS != ret)
												{
													DRIVER_ATSC_CC_ERROR(("GXGDI_Blit failed\n"));
													goto error;
												}
										}
								}
#else
							if ((TRUE == window_old.flag)&&(TRUE == window_old.visible ))
								{
									GXGDI_Lock();
									ret =  GXGDI_Blit(create_surface , &rect, create_layer_surface, &rect, GDI_BLIT_COPY);
									GxGDI_LayerUpdate(create_layer_name);
									GXGDI_Unlock();
									if (GXCORE_SUCCESS != ret)
										{
											DRIVER_ATSC_CC_ERROR(("GXGDI_Blit failed\n"));
											goto error;
										}
								}
#endif
							}
						GXGDI_Lock();
						hd_free_surface(spp_char_window[window_id]);
						GXGDI_Unlock();
						spp_char_window[window_id]=NULL;
						bottom = spp_char_rect[window_id].y+spp_char_rect[window_id].h;
						right = spp_char_rect[window_id].x+spp_char_rect[window_id].w;
						if (spp_char_rect[window_id].x > cc_row_para[window_id][0].top_x)
							spp_char_rect[window_id].x = cc_row_para[window_id][0].top_x;
						if (spp_char_rect[window_id].y > cc_row_para[window_id][0].top_y)
							spp_char_rect[window_id].y = cc_row_para[window_id][0].top_y;
						if (bottom < cc_row_para[window_id][0].top_y+font_rect.h)
							bottom = cc_row_para[window_id][0].top_y+font_rect.h;
						if (right < cc_row_para[window_id][0].top_x+font_rect.w)
							right = cc_row_para[window_id][0].top_x+font_rect.w;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
						spp_char_rect[window_id].h = bottom - spp_char_rect[window_id].y;
						spp_char_rect[window_id].w = right - spp_char_rect[window_id].x;
#else
						spp_char_rect[window_id].x = cc_row_para[window_id][0].top_x;
						spp_char_rect[window_id].y = cc_row_para[window_id][0].top_y;
						spp_char_rect[window_id].w= font_rect.w;
						spp_char_rect[window_id].h= font_rect.h;
#endif
					}
			else
				{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
					/*
					* clear spp layer
					*/
					GXGDI_Lock();
					ret =  GXGDI_Blit(create_layer_surface, (struct GXGDI_Rect*)&create_rect, create_surface, (struct GXGDI_Rect*)&create_rect, GDI_BLIT_COPY);
					GxGDI_LayerUpdate(create_layer_name);
					GXGDI_Unlock();
#endif
					spp_char_rect[window_id].x = cc_row_para[window_id][0].top_x;
					spp_char_rect[window_id].y = cc_row_para[window_id][0].top_y;
					spp_char_rect[window_id].w= font_rect.w;
					spp_char_rect[window_id].h= font_rect.h;
				}
			spp_char_window[window_id]=create_surface;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
			layer_char_window[window_id]=create_layer_surface;
			layer_name[window_id]=create_layer_name;
			memcpy(&layer_char_rect[window_id],&create_rect,sizeof(struct GXGDI_Rect));
			layer_char_rect[window_id].x = 0;
			layer_char_rect[window_id].y = 0;
#endif
		}

	return 0;

error:
	GXGDI_Lock();
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
	if ((NULL != layer_char_window[window_id])&&(NULL != layer_name[window_id]))
		{
			GXGDI_Unlock();
			ret = GxGDI_LayerUnregister(layer_name[window_id]);
			GXGDI_Lock();
			hd_free_surface(layer_char_window[window_id]);
			layer_char_window[window_id]=NULL;
			layer_name[window_id]=NULL;
		}
#endif
	hd_free_surface(create_surface);
	GXGDI_Unlock();
	create_surface=NULL;
	return -1;
}

int gx_atsc_cc_delete_window(uint8_t window_id,uint8_t clear_flag)
{
	if (FALSE == window_define[window_id].flag)
		return -1;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	gx_atsc_cc_clear_window(window_id,spp_char_window[window_id]);
#endif
	if (TRUE == clear_flag)
		{
			if (TRUE == window_define[window_id].visible)
				gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
			if ((NULL != layer_char_window[window_id])&&(NULL != layer_name[window_id]))
				{
					GxGDI_LayerUnregister(layer_name[window_id]);
					GXGDI_Lock();
					hd_free_surface(layer_char_window[window_id]);
					GXGDI_Unlock();
					layer_char_window[window_id]=NULL;
					layer_name[window_id]=NULL;
				}
#endif
		}

	memset(&window_attribute[window_id],0,sizeof(atsc_cc_window_attribute_t));
	memset(&window_define[window_id],0,sizeof(atsc_cc_window_define_t));
	memset(&cc_row_para[window_id],0,ROW_MAX*sizeof(row_para_t));
	if (NULL != spp_char_window[window_id])
		{
			GXGDI_Lock();
			hd_free_surface(spp_char_window[window_id]);
			GXGDI_Unlock();
			spp_char_window[window_id]=NULL;
			memset(&spp_char_rect[window_id],0,sizeof(GXGDI_Rect));
		}
	memset(&last_char_rect[window_id],0,sizeof(GXGDI_Rect));
	return 0;
}

int gx_atsc_cc_display_window(uint8_t window_id)
{
	GXGDI_Rect rect={0};
	int32_t i = 0;
	int32_t ret = -1;
	GXGDI_Rect font_rect={0};


	if (FALSE == window_define[window_id].flag)
		return -1;

	if (NULL != spp_char_window[window_id])
		{
			rect.x=cc_row_para[window_id][0].top_x;
			rect.y=cc_row_para[window_id][0].top_y;
			for (i = 0; i< window_define[window_id].row_count; i++)
				{
					if (cc_row_para[window_id][i].width > cc_row_para[window_id][i].max_width)
						cc_row_para[window_id][i].max_width=cc_row_para[window_id][i].width;
					if (cc_row_para[window_id][i].max_width > rect.w)
						rect.w=cc_row_para[window_id][i].max_width;
				}

			for (i = 0; i< window_define[window_id].row_count; i++)
				{
					rect.h+=cc_row_para[window_id][i].height;
				}

			if ((rect.w>0)&&(rect.h>0))
				{
					font_rect.x=0;
					font_rect.y=0;
					font_rect.w=rect.w;
					font_rect.h=rect.h;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					GXGDI_Lock();
					GXGDI_Begin();
					ret =  GXGDI_Blit(spp_char_window[window_id], &font_rect, spp_cc_subtitle_surface, &rect, GDI_BLIT_STRETCH);
					GXGDI_End();
					GXGDI_Unlock();
#else
					GXGDI_Lock();
					ret =  GXGDI_Blit(spp_char_window[window_id], &font_rect, layer_char_window[window_id], &font_rect, GDI_BLIT_COPY);
					GxGDI_LayerUpdate(layer_name[window_id]);
					GXGDI_Unlock();
#endif
					if (GXCORE_SUCCESS != ret)
						{
							DRIVER_ATSC_CC_ERROR(("GXGDI_FillRect failed\n"));
							return -1;
						}

				}

		}



	return 0;
}


int gx_atsc_cc_clear_window(uint8_t window_id,GuiSurface *surface)
{
	GXGDI_Rect rect={0};
	int32_t i = 0;

	if (FALSE == window_define[window_id].flag)
		return -1;

	rect.x=cc_row_para[window_id][0].top_x;
	rect.y=cc_row_para[window_id][0].top_y;
	for (i = 0; i< window_define[window_id].row_count; i++)
		{
			if (cc_row_para[window_id][i].width > cc_row_para[window_id][i].max_width)
				cc_row_para[window_id][i].max_width=cc_row_para[window_id][i].width;
			if (cc_row_para[window_id][i].max_width > rect.w)
				rect.w=cc_row_para[window_id][i].max_width;
		}

	for (i = 0; i< window_define[window_id].row_count; i++)
		{
			rect.h+=cc_row_para[window_id][i].height;
		}

	if ((rect.w>0)&&(rect.h>0))
		{
			if (NULL != surface)
				{
					if (surface == spp_char_window[window_id])
						{
							memcpy(&rect,&spp_char_rect[window_id],sizeof(GXGDI_Rect));
							rect.x=0;
							rect.y=0;
						}

					if (surface == spp_cc_subtitle_surface)
						{
							if ((atsc_cc_para.id>=GX_SUBTITLE_ATSC_608_CC1)&&(atsc_cc_para.id<=GX_SUBTITLE_ATSC_608_CC4))
								{
									/*
									* force to erase all when 608 CC
									*/
									rect.x=atsc_cc_para.rect.x;
									rect.y=atsc_cc_para.rect.y;
									rect.w=atsc_cc_para.rect.width;
									rect.h=atsc_cc_para.rect.height;
								}
							else
								memcpy(&rect,&spp_char_rect[window_id],sizeof(GXGDI_Rect));
						}
					DRIVER_ATSC_CC_DBG((" clear window window_id = %d\n",window_id));
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
					if (surface == layer_char_window[window_id])
						{
							//memcpy(&rect,&spp_char_rect[window_id],sizeof(GXGDI_Rect));
							//rect.x=0;
							//rect.y=0;
							GXGDI_Lock();
							GXGDI_Begin();
							GXGDI_LayerClear(layer_name[window_id], &layer_char_rect[window_id]);
							GXGDI_End();
							GxGDI_LayerUpdate(layer_name[window_id]);
							GXGDI_Unlock();
						}
					else
#endif
						{
							gx_clear_cc_subtitle_spp(rect,surface);
						}
				}
		}
	memset(&last_char_rect[window_id],0,sizeof(GXGDI_Rect));

	return 0;
}

int gx_atsc_cc_para_init(void)
{
	memset(&cc_row_para[0],0,CC_WINDOW_MAX*ROW_MAX*sizeof(row_para_t));
	memset(&window_define[0],0,CC_WINDOW_MAX*sizeof(atsc_cc_window_define_t));
	memset(&window_attribute[0],0,CC_WINDOW_MAX*sizeof(atsc_cc_window_attribute_t));
	memset(&pen_attribute,0,sizeof(atsc_cc_pen_attribute_t));
	memset(&pen_color,0,sizeof(atsc_cc_pen_color_t));
	memset(&pen_location,0,sizeof(atsc_cc_pen_location_t));
	memset(&last_char_rect[0],0,CC_WINDOW_MAX*sizeof(GXGDI_Rect));
	proper_order=STYLE_INVALID;
//	memset(&cc_info,0,sizeof(GX_SUBTITLE_ATSCCC_INFO));

	/*
	* 开启CC userdata 使能
	*/
//	GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_USERDATA_ENABLE, 1);
//	GxBus_ConfigSetInt(PLAYER_CONFIG_VIDEO_DEC_USERDATA_DISPLAY,0);
	gx_atsc_cc_lock_init();
	return 0;
}

int gx_atsc_cc_open(GxAtsccc_Param* cc_para)
{
	GAL_Rect create_rect = {0};
	int32_t ret = -1;

	if (NULL == cc_para)
		{
			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_open para NULL\n"));
			return -1;
		}

	gx_atsc_cc_lock();
	if (CC_ON == cc_status)
		{
			gx_atsc_cc_unlock();
			gx_atsc_cc_close();
			gx_atsc_cc_lock();
		}
	gx_atsc_cc_para_init();
	if (cc_info.cur_service != cc_para->id)
		cc_info.cur_service = cc_para->id;

	switch(cc_para->id)
		{
			case GX_ATSC_608_CC1:
			case GX_ATSC_608_CC2:
			case GX_ATSC_608_CC3:
			case GX_ATSC_608_CC4:
				pen_color.background_color = COLOR_BLACK;
				pen_color.foreground_color= COLOR_WHITE;
				pen_color.background_opacity=OPACITY_SOLID;
				pen_attribute.pen_size=PEN_STANDARD;
				pen_attribute.underline=FALSE;
				break;
			case GX_ATSC_708_DTV_SERVICE1:
			case GX_ATSC_708_DTV_SERVICE2:
			case GX_ATSC_708_DTV_SERVICE3:
			case GX_ATSC_708_DTV_SERVICE4:
			case GX_ATSC_708_DTV_SERVICE5:
			case GX_ATSC_708_DTV_SERVICE6:
				pen_attribute.pen_size=PEN_STANDARD;
				break;
			default:
				pen_attribute.pen_size=PEN_STANDARD;
				break;
		}
	cc_para->rect.x = 0;
	cc_para->rect.y = 0;
	cc_para->rect.width = subtitleConfig.canvas.width;
	cc_para->rect.height = subtitleConfig.canvas.height;
	create_rect.x = cc_para->rect.x;
	create_rect.y = cc_para->rect.y;
	create_rect.w = cc_para->rect.width;
	create_rect.h = cc_para->rect.height;

	if (cc_para->id <= GX_SUBTITLE_ATSC_708_DTV_SERVICE6)
		{
			ret = gx_init_cc_subtitle_spp(create_rect);
			if (0 != ret)
				{
					gx_atsc_cc_unlock();
					DRIVER_ATSC_CC_ERROR(("gx_init_cc_subtitle_spp failed\n"));
					return -1;
				}
		}

	memcpy(&atsc_cc_para,cc_para,sizeof(GxAtsccc_Param));
	memset(cc_buffer,0,sizeof(cc_buffer));
	valid_cc_708_num=0;
	memset(parse_708_unit,0,CC_708_UNIT_MAX*3);
	cc_status = CC_ON;
	gx_atsc_cc_unlock();
	return cc_para->id+1;
}

int gx_atsc_cc_close(void)
{
	int32_t ret = -1;
	uint8_t i = 0;

	gx_atsc_cc_lock();
	if (CC_OFF == cc_status)
		{
			gx_atsc_cc_unlock();
			return 0;
		}

	for (i =  0; i< CC_WINDOW_MAX;i++)
		{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_OSD_LAYER)
		if ((NULL != layer_char_window[i])&&(NULL != layer_name[i]))
			{
				ret = GxGDI_LayerUnregister(layer_name[i]);
				GXGDI_Lock();
				hd_free_surface(layer_char_window[i]);
				GXGDI_Unlock();
				layer_char_window[i]=NULL;
				layer_name[i]=NULL;
			}
#endif
			if (FALSE == window_define[i].flag)
				continue;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			gx_atsc_cc_clear_window(i,spp_char_window[i]);
#endif
			if (TRUE == window_define[i].visible)
				{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					gx_atsc_cc_clear_window(i,spp_cc_subtitle_surface);
#endif
				}
			if (NULL != spp_char_window[i])
				{
					GXGDI_Lock();
					hd_free_surface(spp_char_window[i]);
					GXGDI_Unlock();
					spp_char_window[i]=NULL;
				}
		}
	ret = gx_close_cc_subtitle_spp();
	cc_status = CC_OFF;
	memset(last_unit_data,0,3);
	/*
	* 清空buffer
	*/
	memset(cc_buffer,0,sizeof(cc_buffer));
	memset(&atsc_cc_para,0,sizeof(GxAtsccc_Param));
	memset(&window_define[0],0,CC_WINDOW_MAX*sizeof(atsc_cc_window_define_t));
	memset(&window_attribute[0],0,CC_WINDOW_MAX*sizeof(atsc_cc_window_attribute_t));
	memset(&pen_attribute,0,sizeof(atsc_cc_pen_attribute_t));
	memset(&pen_color,0,sizeof(atsc_cc_pen_color_t));
	memset(&pen_location,0,sizeof(atsc_cc_pen_location_t));
	memset(&last_char_rect[0],0,CC_WINDOW_MAX*sizeof(GXGDI_Rect));
	memset(&cc_row_para[0],0,CC_WINDOW_MAX*ROW_MAX*sizeof(row_para_t));
	valid_cc_708_num=0;
	proper_order = STYLE_INVALID;
	memset(parse_708_unit,0,CC_708_UNIT_MAX*3);
	memset(last_sequence_number,0xFF,8);
	gx_atsc_cc_unlock();
	return 0;
}

int32_t gx_atsc_cc_get_font_height(CC_PEN_SIZE pen_size,char* show_char,int32_t *height,int32_t *width)
{
	int32_t ret = -1;
	char* font_name=NULL;
	if ((NULL == height)||(NULL == width)||(NULL == show_char))
		{
			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_get_font_height NULL\n"));
			return -1;
		}

	switch(pen_size)
		{
			case PEN_SMALL:
				if (FALSE == pen_attribute.italic)
					{
						font_name="small";
					}
				else
					{
						font_name="sm_italic";
					}
				break;
			case PEN_STANDARD:
				if (FALSE == pen_attribute.italic)
					{
						font_name="standard";
					}
				else
					{
						font_name="sd_italic";
					}
				break;
			case PEN_LARGE:
				if (FALSE == pen_attribute.italic)
					{
						font_name="large";
					}
				else
					{
						font_name="lg_italic";
					}
				break;
			default:
					font_name="standard";
				break;
		}

	ret =  gx_get_font_width_height((const unsigned char *)font_name,show_char,height,width);
	if (0 != ret)
		{
			DRIVER_ATSC_CC_ERROR(("gx_get_font_width_height failed\n"));
			return -1;
		}
	*height=*height/2*2+2;
//	*width = gdi_text_len((void*)show_char);
	if (0 != (*width%4))
		{
			*width = *width/4*4+4;
		}

	if (FALSE == pen_attribute.italic)
		{
			switch(pen_attribute.pen_size)
				{
					case PEN_SMALL:
						if (font_width_small < *width)
							{
								font_width_small = *width;
							}
						break;
					case PEN_STANDARD:
						if (font_width_standard < *width)
							{
								font_width_standard = *width;
							}
						break;
					case PEN_LARGE:
						if (font_width_large < *width)
							{
								font_width_large = *width;
							}
						break;
					default:
						if (font_width_standard < *width)
							{
								font_width_standard = *width;
							}
						break;
				}
		}

	return 0;
}

int gx_atsc_cc_scroll(uint8_t window_id,CC_DIRECTION scroll_direction)
{
	GuiSurface *create_surface=NULL;
	uint8_t bpp=2;
	uint32_t width=0;
	uint32_t height=0;
	int32_t ret = -1;
	int32_t i = 0;
	GxColorFormat color_format=GX_COLOR_FMT_YCBCRA6442;
	GAL_Rect rect={0};
	GXGDI_Rect src_rect={0};
	GXGDI_Rect dst_rect={0};

	if (CC_OFF == cc_status)
		{
			DRIVER_ATSC_CC_ERROR(("cc_status = %d\n",cc_status));
			return -1;
		}

	if (FALSE == window_define[window_id].flag)
		return -1;

	if (0 == window_define[window_id].row_count)
		{
			DRIVER_ATSC_CC_ERROR(("window_define[%d].row_count = %d\n",window_id,window_define[window_id].row_count));
			return -1;
		}
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	if (NULL == spp_cc_subtitle_surface)
		{
			DRIVER_ATSC_CC_ERROR(("spp_cc_subtitle_surface NULL\n"));
			return -1;
		}
#else
	if (NULL == layer_char_window[window_id])
		{
			DRIVER_ATSC_CC_ERROR(("layer_char_window[%d] NULL\n",window_id));
			return -1;
		}
#endif

	width = 0;
	for (i=1;i<window_define[window_id].row_count;i++)
		{
			if (cc_row_para[window_id][i].width>cc_row_para[window_id][i].max_width)
				cc_row_para[window_id][i].max_width = cc_row_para[window_id][i].width;		
			if (cc_row_para[window_id][i].max_width>width)
				width = cc_row_para[window_id][i].max_width;
		}

	if ( 0 == width)
		{
			return -1;
		}

	switch(scroll_direction)
		{
			case LEFT_TO_RIGHT:
				DRIVER_ATSC_CC_ERROR(("scroll_direction LEFT_TO_RIGHT not support\n"));
				break;
			case RIGHT_TO_LEFT:
				DRIVER_ATSC_CC_ERROR(("scroll_direction RIGHT_TO_LEFT not support\n"));
				break;
			case TOP_TO_BOTTOM:
				DRIVER_ATSC_CC_ERROR(("scroll_direction TOP_TO_BOTTOM not support\n"));
				break;
			case BOTTOM_TO_TOP:
				{
					height = 0;
					for (i=1;i<window_define[window_id].row_count;i++)
						{
							height += cc_row_para[window_id][i].height;
						}

					/*
					* 创建最大行数的surface
					*/
					rect.x = 0;
					rect.y = 0;
					rect.w = width;
					rect.h = height;
					GXGDI_Lock();
					create_surface = hd_get_surface0(&rect, bpp, color_format, NULL, TRUE);
					if (NULL == create_surface)
						{
							GXGDI_Unlock();
							DRIVER_ATSC_CC_ERROR(("hd_get_surface0 para error  NULL\n"));
							return -1;
						}

					/*
					* 第2行开始的数据blit到新建surface(滚动 ，第一行被覆盖)
					*/
					src_rect.x = cc_row_para[window_id][1].top_x;
					src_rect.y = cc_row_para[window_id][1].top_y;
					src_rect.w = width;
					src_rect.h = height;

					dst_rect.x = 0;
					dst_rect.y = 0;
					dst_rect.w = width;
					dst_rect.h = height;

#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					src_rect.x = cc_row_para[window_id][1].top_x;
					src_rect.y = cc_row_para[window_id][1].top_y;
					ret =  GXGDI_Blit(spp_cc_subtitle_surface, &src_rect, create_surface, &dst_rect, GDI_BLIT_COPY);
#else
					src_rect.x = 0;//cc_row_para[window_id][1].top_x-cc_row_para[window_id][0].top_x;
					src_rect.y = cc_row_para[window_id][0].height;//cc_row_para[window_id][1].top_y-cc_row_para[window_id][0].top_y;
					ret =  GXGDI_Blit(layer_char_window[window_id], &src_rect, create_surface, &dst_rect, GDI_BLIT_COPY);
#endif
					if (GXCORE_SUCCESS != ret)
						{
							hd_free_surface(create_surface);
							GXGDI_Unlock();
							create_surface = NULL;
							DRIVER_ATSC_CC_ERROR(("GXGDI_Blit  error \n"));
							return -1;
						}
					GXGDI_Unlock();
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					if (NULL != spp_char_window[window_id])
						gx_atsc_cc_clear_window(window_id,spp_char_window[window_id]);
					if (TRUE == window_define[window_id].visible)
#endif
						{
							dst_rect.w = 0;
							dst_rect.h = 0;
							dst_rect.x = cc_row_para[window_id][0].top_x;
							dst_rect.y = cc_row_para[window_id][0].top_y;
							for (i=0;i<window_define[window_id].row_count;i++)
								{
									if (cc_row_para[window_id][i].width > cc_row_para[window_id][i].max_width)
										cc_row_para[window_id][i].max_width=cc_row_para[window_id][i].width;
									if (cc_row_para[window_id][i].max_width>dst_rect.w)
										dst_rect.w = cc_row_para[window_id][i].max_width;
									dst_rect.h += cc_row_para[window_id][i].height;
								}
							if (TRUE == window_define[window_id].flag)
								{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
									gx_clear_cc_subtitle_spp(dst_rect,spp_cc_subtitle_surface);
#else
									GXGDI_Lock();
									dst_rect.x = 0;
									dst_rect.y = 0;
									GXGDI_Begin();
									GXGDI_LayerClear(layer_name[window_id], &dst_rect);
									GXGDI_End();
									GXGDI_Blit(layer_char_window[window_id], &dst_rect, spp_char_window[window_id], &dst_rect, GDI_BLIT_COPY);
									GxGDI_LayerUpdate(layer_name[window_id]);
									GXGDI_Unlock();
#endif
								}
//							gx_atsc_cc_clear_window(window_id,spp_cc_subtitle_surface);
						}

					src_rect.x = 0;
					src_rect.y = 0;
					src_rect.w = width;
					src_rect.h = height;

					dst_rect.x = cc_row_para[window_id][0].top_x;
					dst_rect.y = cc_row_para[window_id][0].top_y;
					dst_rect.w = width;
					dst_rect.h = height;
					
					GXGDI_Lock();
					if (NULL != spp_char_window[window_id])
						{
							ret =  GXGDI_Blit(create_surface , &src_rect, spp_char_window[window_id], &src_rect, GDI_BLIT_COPY);
							if (GXCORE_SUCCESS != ret)
								{
									hd_free_surface(create_surface);
									GXGDI_Unlock();
									create_surface = NULL;
									DRIVER_ATSC_CC_ERROR(("GXGDI_Blit  error x=%d y=%d w=%d h=%d\n",
										src_rect.x,src_rect.y,src_rect.w,src_rect.h));
									return -1;
								}
						}
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					ret =  GXGDI_Blit(create_surface , &src_rect, spp_cc_subtitle_surface, &dst_rect, GDI_BLIT_COPY);
#else
					ret =  GXGDI_Blit(create_surface , &src_rect, layer_char_window[window_id], &src_rect, GDI_BLIT_COPY);
					GxGDI_LayerUpdate(layer_name[window_id]);
#endif
					hd_free_surface(create_surface);
					create_surface = NULL;
					GXGDI_Unlock();

					if (GXCORE_SUCCESS != ret)
						{
							DRIVER_ATSC_CC_ERROR(("GXGDI_Blit  error \n"));
							return -1;
						}

					for (i=1;i<window_define[window_id].row_count;i++)
					{
						cc_row_para[window_id][i-1].width = cc_row_para[window_id][i].width;					
						cc_row_para[window_id][i-1].max_width = cc_row_para[window_id][i].max_width;
					}

					cc_row_para[window_id][pen_location.row].width=0;
					cc_row_para[window_id][pen_location.row].max_width=0;
					DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
						pen_location.row,cc_row_para[window_id][pen_location.row].width));

				}
				break;
			default:
				break;
		}

	return 0;
}

int32_t gx_atsc_cc_delete_to_end_of_low(uint8_t window_id)
{
	int font_width = 0;
        int font_height = 0;
	GXGDI_Rect rect={0};
	GXGDI_Rect clear_spp_rect={0};
	if (FALSE == window_define[window_id].flag)
		return -1;

	gx_atsc_cc_get_font_height(pen_attribute.pen_size,"W",&font_height,&font_width);
	rect.x=pen_location.column*font_width;
	if (cc_row_para[window_id][pen_location.row].max_width > pen_location.column*font_width)
	{
		rect.w=cc_row_para[window_id][pen_location.row].max_width - pen_location.column*font_width;
		rect.y=cc_row_para[window_id][pen_location.row].top_y-cc_row_para[window_id][0].top_y;
		rect.h=cc_row_para[window_id][pen_location.row].height;

		DRIVER_ATSC_CC_DBG((" pen_location.column*font_width=%d \n",pen_location.column*font_width));
		DRIVER_ATSC_CC_DBG((" rect.x=%d rect.y=%d rect.w=%d rect.h=%d \n",rect.x, rect.y,rect.w,rect.h));
		DRIVER_ATSC_CC_DBG((" cc_row_para[%d][%d].max_width=%d width=%d\n",window_id, pen_location.row,cc_row_para[window_id][pen_location.row].max_width,cc_row_para[window_id][pen_location.row].width));

		if (NULL != spp_char_window[window_id])
			{
				gx_clear_cc_subtitle_spp(rect,spp_char_window[window_id]);
			}

		memcpy(&clear_spp_rect,&rect,sizeof(GXGDI_Rect));
		clear_spp_rect.x = cc_row_para[window_id][pen_location.row].top_x+pen_location.column*font_width;
		clear_spp_rect.y = cc_row_para[window_id][pen_location.row].top_y;
//		clear_spp_rect.w = spp_char_rect[window_id].w - pen_location.column*font_width;
		if (TRUE == window_define[window_id].visible)
			{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
				gx_clear_cc_subtitle_spp(clear_spp_rect,spp_cc_subtitle_surface);
#else
				GXGDI_Lock();
				GXGDI_Begin();
				GXGDI_LayerClear(layer_name[window_id], &rect);
				GXGDI_End();
				GxGDI_LayerUpdate(layer_name[window_id]);
				GXGDI_Unlock();
#endif
			}
	}


	return 1;
}

int32_t gx_atsc_cc_clear_last_character(uint8_t window_id)
{
	GXGDI_Rect rect={0};
	if (FALSE == window_define[window_id].flag)
		return -1;

	if ((last_char_rect[window_id].w>0)&&(pen_location.column >0)&&(cc_row_para[window_id][pen_location.row].width-last_char_rect[window_id].w>=0))
		{
			if (NULL != spp_char_window[window_id])
				{
					rect.w=last_char_rect[window_id].w;
					rect.h=last_char_rect[window_id].h;
					rect.x=last_char_rect[window_id].x-cc_row_para[window_id][pen_location.row].top_x;
					rect.y=cc_row_para[window_id][pen_location.row].top_y-cc_row_para[window_id][0].top_y;
					gx_clear_cc_subtitle_spp(rect,spp_char_window[window_id]);
				}

			if (TRUE == window_define[window_id].visible)
				{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
					gx_clear_cc_subtitle_spp(last_char_rect[window_id],spp_cc_subtitle_surface);
#else
					GXGDI_Lock();
					GXGDI_Begin();
					GXGDI_LayerClear(layer_name[window_id], &rect);
					GXGDI_End();
					GxGDI_LayerUpdate(layer_name[window_id]);
					GXGDI_Unlock();
#endif
				}

			cc_row_para[window_id][pen_location.row].width = cc_row_para[window_id][pen_location.row].width-last_char_rect[window_id].w;
			if (pen_location.column >0)
				pen_location.column = pen_location.column -1;
			DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
				pen_location.row,cc_row_para[window_id][pen_location.row].width));
			memset(&last_char_rect[window_id],0,sizeof(GXGDI_Rect));
			memset(last_char,0,sizeof(last_char));
		}

	return 1;
}

int32_t gx_atsc_cc_draw_character(char* show_char,uint8_t window_id)
{
	int font_width = 0;
	int font_height = 0;
	int ret = -1;
	uint32_t back_color=0;
	uint32_t fore_color=0;
	uint32_t under_color=0;
	GAL_Rect under_rect={0};
	GAL_Rect create_rect = {0};
	GAL_Rect tmp_font_rect={0};
	GXGDI_Rect font_rect={0};
	GXGDI_Rect font_char_rect={0};
	uint32_t y=0,cb=0,cr=0;
	GxVpuProperty_FillRect FillRect = {0};
	STRING_TYPE flag=GDI_STRING_PARAGRAPH; /*段落，自动换行*/
	int alignment=0x0;
	GuiSurface *font_surface=NULL;
	uint8_t font_bpp=32;
	GxColorFormat font_color_format = GX_COLOR_FMT_ARGB8888;

	if(pen_location.column >= window_define[window_id].column_count)
		return -1;
	if (NULL == show_char)
		{
			DRIVER_ATSC_CC_ERROR(("character NULL\n"));
			return -1;
		}

	if (0 == strlen(show_char))
		{
			DRIVER_ATSC_CC_ERROR(("len error strlen(show_char)=%d\n",strlen(show_char)));
			return -1;
		}

	memset(last_char,0,sizeof(last_char));
	if (strlen(show_char)<3)
		memcpy(last_char,show_char,3);
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	if (NULL == spp_cc_subtitle_surface)
		{
			DRIVER_ATSC_CC_ERROR(("spp_cc_subtitle_surface NULL\n"));
			return -1;
		}
#else
		if (NULL == layer_char_window[window_id])
		{
			DRIVER_ATSC_CC_ERROR(("layer_char_window[%d] NULL\n",window_id));
			return -1;
		}
#endif
	if (FALSE == window_define[window_id].flag)
		return -1;

	if (0 == window_define[window_id].row_count)
		{
			return -1;
		}

	pen_location.column++;
	DRIVER_ATSC_CC_DBG(("pen_location.column=%d width=%d\n"
	,pen_location.column,cc_row_para[window_id][pen_location.row].width));
	if (pen_location.column>window_define[window_id].column_count)
		{
			DRIVER_ATSC_CC_ERROR(("pen_location.column = %d window_define[window_id].column_count=%d\n",
				pen_location.column,window_define[window_id].column_count));
			return -1;
			/*
			* 超过限定列
			*/
			;
		}
	gx_atsc_cc_get_font_height(pen_attribute.pen_size,show_char,&font_height,&font_width);
	if ((0 == font_height)||(0 == font_width))
		{
			DRIVER_ATSC_CC_ERROR(("font_height=%d font_width=%d\n",font_height,font_width));
			return -1;
		}
//	font_width = gdi_text_len((void*)show_char);
	create_rect.x = 0;
	create_rect.y = 0;
	create_rect.w = font_width;
	create_rect.h = font_height;
	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=font_width;
	font_rect.h=font_height;

	gx_color_type_ycbcr(pen_color.background_color,&back_color,&y,&cb,&cr);
	font_surface = gx_create_font_surface(&create_rect, font_bpp, font_color_format,y,cb,cr,pen_color.background_opacity);
	if (NULL == font_surface)
		{
			DRIVER_ATSC_CC_ERROR(("hd_get_surface0 failed font_width=%d font_height=%d char_len=%d\n",
				font_width,font_height,strlen(show_char)));
			return -1;
		}

	gx_color_type_ycbcr(pen_color.foreground_color,&fore_color,&y,&cb,&cr);
	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=font_width;
	font_rect.h=font_height;
	switch(pen_color.background_opacity)
		{
			case OPACITY_SOLID:
				/*
				* 不透明
				*/
				break;
			case OPACITY_TRANSLUCENT:
				{
					back_color = GXGDI_GetColorKey();
					/*
					* 半透明
					*/
				}
				break;
			case OPACITY_TRANSPARENT:
					/*
					* 透明
					*/
					back_color = GXGDI_GetColorKey();
				break;
			default:
				break;
		}
	GXGDI_Lock();
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	/*
	* double_buffer mode, draw cc need fill backcolor in advace
	*/
	if ((gui.config.enable_double_buffer) /*&&
			(TTF_FONT_TYPE == gui_core->pCurrentFont->family)*/)
		{
			GXGDI_Begin();
			tmp_font_rect.x = font_rect.x;
			tmp_font_rect.y = font_rect.y;
			tmp_font_rect.w = font_rect.w;
			tmp_font_rect.h = font_rect.h;
			
			hd_fillrect(font_surface, &(tmp_font_rect), back_color);
			GXGDI_End();
		}
#endif
	GXGDI_DrawString_back_color(font_surface,show_char,&font_rect,alignment,fore_color,back_color,flag);
	if (TRUE == pen_attribute.underline)
		{
			/*
			* draw char color underline
			*/
			under_color = gal_color2index(font_surface, fore_color);
			under_rect.x = font_rect.x;
			under_rect.y = font_rect.y+font_rect.h-2;
			under_rect.w = font_rect.w;
			under_rect.h = 2;
			GXGDI_Begin();
			hd_fillrect(font_surface, &under_rect, under_color);
			GXGDI_End();
		}
	GXGDI_Unlock();
//	GXGDI_DrawString(font_surface,show_char,&font_rect,alignment,fore_color,flag);

	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=font_width;
	font_rect.h=font_height;

	last_char_rect[window_id].x=cc_row_para[window_id][pen_location.row].top_x+cc_row_para[window_id][pen_location.row].width;
	last_char_rect[window_id].y=cc_row_para[window_id][pen_location.row].top_y;
	last_char_rect[window_id].w=font_width;
	last_char_rect[window_id].h=font_height;

	if (NULL != spp_char_window[window_id])
		{
			font_char_rect.x=cc_row_para[window_id][pen_location.row].width;
			font_char_rect.y=cc_row_para[window_id][pen_location.row].top_y-cc_row_para[window_id][0].top_y;
			font_char_rect.w=font_width;
			font_char_rect.h=font_height;
			GXGDI_Lock();
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			GXGDI_Begin();
			ret =  GXGDI_Blit(font_surface, &font_rect, spp_char_window[window_id], &font_char_rect, GDI_BLIT_STRETCH);
			GXGDI_End();
#else
			ret =  GXGDI_Blit(font_surface, &font_rect, spp_char_window[window_id], &font_char_rect, GDI_BLIT_COPY);
#endif
			if (GXCORE_SUCCESS != ret)
				{
					hd_free_surface(font_surface);
					font_surface=NULL;
					GXGDI_Unlock();
					DRIVER_ATSC_CC_ERROR(("GXGDI_FillRect failed\n"));
					return -1;
				}
			GXGDI_Unlock();
		}

	GXGDI_Lock();
	if (TRUE == window_define[window_id].visible)
		{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
			GXGDI_Begin();
			ret =  GXGDI_Blit(font_surface, &font_rect, spp_cc_subtitle_surface, &last_char_rect[window_id], GDI_BLIT_STRETCH);
			GXGDI_End();
#else
			GXGDI_Begin();
			if (NULL != layer_char_window[window_id])
				ret =  GXGDI_Blit(font_surface, &font_rect, layer_char_window[window_id], &font_char_rect, /*GDI_BLIT_COPY*/GDI_BLIT_STRETCH);
			GXGDI_End();
			GxGDI_LayerUpdate(layer_name[window_id]);
#endif
			if (GXCORE_SUCCESS != ret)
				{
					hd_free_surface(font_surface);
					font_surface=NULL;
					GXGDI_Unlock();
					DRIVER_ATSC_CC_ERROR(("GXGDI_FillRect failed\n"));
					return -1;
				}
		}
	hd_free_surface(font_surface);
	font_surface=NULL;
	GXGDI_Unlock();

	cc_row_para[window_id][pen_location.row].width = cc_row_para[window_id][pen_location.row].width+font_width;
	if (cc_row_para[window_id][pen_location.row].width > cc_row_para[window_id][pen_location.row].max_width)
		cc_row_para[window_id][pen_location.row].max_width = cc_row_para[window_id][pen_location.row].width;
	DRIVER_ATSC_CC_DBG(("pen_location.row=%d width=%d\n",
		pen_location.row,cc_row_para[window_id][pen_location.row].width));

	return 0;
}

int gx_atsc_cc_parse_userdata(handle_t handle, uint8_t* data, int length)
{
	char *p = NULL;
	int process_cc_data_flag = 0, additional_data_flag = 0;
	short i, cc_count = 0, cc_valid = 0, cc_type = 0;
	short valid_cc_608_num = 0;
	short cc_708_count = 0;
	uint8_t field,data1,data2;
	uint8_t k=0;
	if ((NULL == data)||(0 == length))
		{
			DRIVER_ATSC_CC_ERROR(("user_data NULL\n"));
			return -1;
		}
	memset(parse_608_unit,0,CC_608_UNIT_MAX*3);
//	memset(parse_708_unit,0,CC_708_UNIT_MAX*3);
	p = (char*)data;
	while(p <= (char*)data+length-3) {
		if( * p    == 0x47 && *(p+1) == 0x41 &&
			*(p+2) == 0x39 && *(p+3) == 0x34 ) {
			p += 5;//skip 0x47 0x41 0x39 0x34 0x03 // GA94
			process_cc_data_flag = (*p>>6)&0x1;
			if(process_cc_data_flag) {
				additional_data_flag = (*p>>5)&0x1;
				cc_count = (*p>>0)&0x1f;
				p += 2;//skip 0x41 0x00
//				DRIVER_ATSC_CC_DBG(("cc_count=%d\n",cc_count));
				for(i = 0; i < cc_count; i++, p+=3) {
					cc_valid = (*p>>2)&0x1;
					if(cc_valid) {
						cc_type = (*p>>0)&0x3;//[STD 608]: 00(top), 01(bottom);[STD 708]: 10(top), 11(bottom);						
 						if (CC_ON != cc_status)
							{
								continue;
							}
						switch(cc_type)
							{
								case 0x00:
								case 0x01:
										{
											/*
											* atsc cc 608
											*/
										//	if (((*(p+1)&0x70)>0x0)&&((*(p+2)&0x7f)>0x0))
											if ((*(p+1)&0x70)>0x0)
												{
													data1 = *(p+1);
													data2 = *(p+2);
//													DRIVER_ATSC_CC_DBG(("0x%02x 0x%02x %c 0x%02x %c\n",
//														cc_type,data1,data1&0x7f,data2,data2&0x7f));
													if (valid_cc_608_num<CC_608_UNIT_MAX)
														{
															parse_608_unit[valid_cc_608_num*3]=cc_type;
															parse_608_unit[valid_cc_608_num*3+1]=*(p+1);
															parse_608_unit[valid_cc_608_num*3+2]=*(p+2);
															valid_cc_608_num ++;
														}
													else
														{
															DRIVER_ATSC_CC_ERROR((" valid_cc_608_num = %d  cc data overflow\n",
																valid_cc_608_num));
														}
												}
										}
									break;
								case 0x02:
								case 0x03:
										{
											/*
											* atsc cc 708
											*/
											data1 = *(p+1);
											data2 = *(p+2);
//											DRIVER_ATSC_CC_DBG(("0x%02x 0x%02x %c 0x%02x %c\n",
//												cc_type,data1,data1&0x7f,data2,data2&0x7f));
											if (valid_cc_708_num<CC_708_UNIT_MAX)
											{
												parse_708_unit[valid_cc_708_num*3]=cc_type;
												parse_708_unit[valid_cc_708_num*3+1]=*(p+1);
												parse_708_unit[valid_cc_708_num*3+2]=*(p+2);
												valid_cc_708_num ++;
											}
											else
												{
													DRIVER_ATSC_CC_ERROR((" valid_cc_708_num = %d  cc data overflow\n",
														valid_cc_708_num));
												}
										}
									break;
								default:
									break;
							}
					}
				}
			}
			// other 3 bytes 0xff 0x00 0x00
		}
		else
			p++;
	}

	if (0 != valid_cc_608_num)
		gx_atsc_cc_608_parse(parse_608_unit,valid_cc_608_num*3);

	if (valid_cc_708_num)
		{
//			if (0 != valid_cc_708_num)
//				DRIVER_ATSC_CC_DBG(("valid_cc_708_num = %d parse_708_unit[0]=%d\n",
//				valid_cc_708_num,parse_708_unit[0]));
			cc_708_count = valid_cc_708_num;
			for (k=0; k<cc_708_count;k++)
				{
					if (0x03 == parse_708_unit[k*3])
						{
							/*
							* find DTVCC Ptr Header
							*/
							uint8_t data1=0;
							uint8_t packet_size=0;
							uint8_t packet_data_size=0;
							data1 = parse_708_unit[k*3+1];
							packet_size = data1&0x3F;
							if (0 == packet_size)
								packet_data_size = 127;
							else
								packet_data_size = (packet_size*2)-1;

							if (2*(cc_708_count-k)>=packet_data_size+1)
								{
									/*
									* packet完整
									*/
//									DRIVER_ATSC_CC_DBG(("deal with cc_708_num = %d parse_708_unit[%d]=%d\n",
//										(packet_data_size+1)/2,k*3,parse_708_unit[k*3]));
									gx_atsc_cc_708_parse(parse_708_unit+k*3,(packet_data_size+1)/2*3);
									valid_cc_708_num=cc_708_count-k-(packet_data_size+1)/2;
									if (packet_data_size>1)
										k=k+(packet_data_size+1)/2-1;
//									DRIVER_ATSC_CC_DBG(("packet cc_708_count = %d k=%d \n",cc_708_count,k));
									if (0 == valid_cc_708_num)
										{
//											DRIVER_ATSC_CC_DBG(("packet parse end valid_cc_708_num = %d \n",
//												valid_cc_708_num));
											return 0;
										}
								}
							else
								{
									/*
									* packet不完整，下段数据接收后合并判断
									*/
									valid_cc_708_num=cc_708_count-k;
									memcpy(parse_708_unit_bak,parse_708_unit+k*3,valid_cc_708_num*3);
									memset(parse_708_unit,0,CC_708_UNIT_MAX*3);
									memcpy(parse_708_unit,parse_708_unit_bak,valid_cc_708_num*3);
									DRIVER_ATSC_CC_DBG(("packet parse remain valid_cc_708_num = %d \n",
										valid_cc_708_num));
								}
						}

				}
		}

	return 0;
}

int GxAtsccc_SetCurService(GX_SUBTITLE_ATSCCC_SERVICE service)
{
	return 0;
}

int GxAtsccc_GetServiceInfo(GX_SUBTITLE_ATSCCC_INFO *info)
{
	if (NULL == info){
		DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_get para NULL\n"));
		return 0;
	}

	gx_atsc_cc_lock();
	memcpy(info,&cc_info,sizeof(GX_SUBTITLE_ATSCCC_INFO));
	gx_atsc_cc_unlock();

	return 0;
}

int8_t gx_atsc_cc_check_flag(char* player,uint8_t flag)
{
	if (NULL == player)
		{
			DRIVER_ATSC_CC_ERROR(("gx_atsc_cc_check_flag para NULL\n"));
			return -1;
		}
		
	gx_atsc_cc_lock();
	memset(&cc_info,0,sizeof(GX_SUBTITLE_ATSCCC_INFO));
	gx_atsc_cc_unlock();
	return flag;
}


