#include "gxtype.h"
#include "gx_mem.h"
#include "gxos/gxcore_os.h"
#include "gxcore.h"
#include "gui_core.h"
#include "subtitle_config.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gx_scte_subtitle.h"
#ifdef CONFIG_SUBTITLE_GDI_DRAW
#include "gxavdev.h"
#include "gxscte_subtitle_inner.h"
#include "gx_cc_subtitle_common.h"
#include "gx_cc_subtitle_draw.h"
#include "gx_txt_subtitle.h"
#include "gx_subtitle_show_text.h"
#include "gx_subtitle_config.h"
extern GuiCore gui;
GxScteSubtitleWindow scte_sub_para={0};
static SUB_STATUS sub_status=SUB_OFF;
static handle_t sub_lock=-1;
scte_sub_simple_bitmap_t simple_bitmap[TABLE_EXTENSION_MAX];
static GxScteSubtitleParam gx_scte_subtitle_param;
int gx_scte_sub_close(void);

static void gx_scte_sub_lock_init(void)
{
	if (-1 == sub_lock)
		GxCore_MutexCreate(&sub_lock);
}

static void gx_scte_sub_lock(void)
{
	if (-1 != sub_lock)
		GxCore_MutexLock(sub_lock);
}

static void gx_scte_sub_unlock(void)
{
	if (-1 != sub_lock)
		GxCore_MutexUnlock(sub_lock);
}

static int gx_scte_sub_spp_clear_pre_display(void)
{
	uint8_t i = 0;
	GXGDI_Rect spp_rect={0};
	spp_rect.x=0;
	spp_rect.y=0;
	spp_rect.w=scte_sub_para.width;
	spp_rect.h=scte_sub_para.height;

	for (i=0; i< TABLE_EXTENSION_MAX;i++)
	{
		if ((TRUE == simple_bitmap[i].status)
				&&(TRUE == simple_bitmap[i].display_status))
		{
			gx_clear_cc_subtitle_spp(spp_rect,spp_cc_subtitle_surface);
			if (NULL != simple_bitmap[i].data)
			{
				av_free(simple_bitmap[i].data);
				simple_bitmap[i].data=NULL;
			}

			if (NULL != simple_bitmap[i].segment)
			{
				av_free(simple_bitmap[i].segment);
				simple_bitmap[i].segment=NULL;
			}
			memset(&simple_bitmap[i],0,sizeof(scte_sub_simple_bitmap_t));
			break;
		}
	}

	return 0;
}

scte_sub_simple_bitmap_t* gx_scte_sub_get_simple_bitmap(uint8_t segmentation_overlay_included,uint16_t table_extension,uint16_t segment_number)
{
	uint32_t i =0;
	for (i = 0; i< TABLE_EXTENSION_MAX;i++)
	{
		if (TRUE == segmentation_overlay_included)
		{
			if ((TRUE == simple_bitmap[i].status)
					&&(segmentation_overlay_included == simple_bitmap[i].segmentation_overlay_included)
					&&(table_extension == simple_bitmap[i].table_extension))
			{
				return &simple_bitmap[i];
			}
		}
	}

	for (i = 0; i< TABLE_EXTENSION_MAX;i++)
	{
		if (TRUE == segmentation_overlay_included)
		{
			if (FALSE == simple_bitmap[i].status)
			{
				if (0 == segment_number)
				{
					simple_bitmap[i].status = TRUE;
					return &simple_bitmap[i];
				}
			}
		}
		else
		{
			if (FALSE == simple_bitmap[i].status)
			{
				simple_bitmap[i].status = TRUE;
				return &simple_bitmap[i];
			}
		}
	}
	CC_SUBTITLE_COMMON_ERROR(("gx_scte_sub_get_simple_bitmap failed\n"));
	return NULL;

}

int32_t gx_scte_sub_simple_bitmap_parse( char* data,uint16_t len ,scte_sub_simple_bitmap_t* simple_bitmap_p)
{
	char*	sectionData=data;
	uint8_t background_style=0;
	uint8_t outline_style=0;
	uint8_t Y_component=0;
	uint8_t opaque_enable=0;
	uint8_t Cr_component=0;
	uint8_t Cb_component=0;

	uint16_t bitmap_top_H_coordinate=0;
	uint16_t bitmap_top_V_Coordinate=0;
	uint16_t bitmap_bottom_H_coordinate=0;
	uint16_t bitmap_bottom_V_coordinate=0;

	uint16_t frame_top_H_coordinate=0;
	uint16_t frame_top_V_coordinate=0;
	uint16_t frame_bottom_H_coordinate=0;
	uint16_t frame_bottom_V_coordinate=0;

	uint8_t outline_thickness=0;
	uint8_t shadow_right=0;
	uint8_t shadow_bottom=0;
	uint16_t bitmap_length=0;
	uint16_t remain_len=len;
	uint16_t compressed_bitmap_len=0;

	if ((NULL == data)||(NULL == simple_bitmap_p))
	{
		CC_SUBTITLE_COMMON_ERROR(("gx_scte_sub_simple_bitmap_parse para NULL\n"));
		return -1;
	}

	outline_style = sectionData[0]&0x3;
	background_style = (sectionData[0]>>2)&0x1;
	switch(background_style)
	{
	case 0:
		simple_bitmap_p->background_style = BACKGROUND_STYLE_TRANSPARENT;
		//CC_SUBTITLE_COMMON_ERROR(("background_style transparent\n"));
		break;
	case 1:
		simple_bitmap_p->background_style = BACKGROUND_STYLE_FRAMED;
		//CC_SUBTITLE_COMMON_ERROR(("background_style framed\n"));
		break;
	default:
		break;
	}

	switch(outline_style)
	{
	case 0:
		simple_bitmap_p->outline_style = OUTLINE_STYLE_NONE;
		//CC_SUBTITLE_COMMON_ERROR(("outline_style none\n"));
		break;
	case 1:
		simple_bitmap_p->outline_style = OUTLINE_STYLE_OUTLINE;
		//CC_SUBTITLE_COMMON_ERROR(("outline_style outline\n"));
		break;
	case 2:
		simple_bitmap_p->outline_style = OUTLINE_STYLE_DROP_SHADOW;
		//CC_SUBTITLE_COMMON_ERROR(("outline_style drop_shadow\n"));
		break;
	case 3:
		simple_bitmap_p->outline_style = OUTLINE_STYLE_RESERVED;
		//CC_SUBTITLE_COMMON_ERROR(("outline_style reserved\n"));
		break;
	default:
		break;
	}
	sectionData+=1;
	remain_len-=1;
	Y_component = (sectionData[0]>>3)&0x1f;
	opaque_enable = (sectionData[0]>>2)&0x1;
	Cr_component = ((sectionData[0]&0x3)<<3)+((sectionData[1]>>5)&0x7);
	Cb_component = sectionData[1]&0x1f;
	Y_component=Y_component*8;
	Cr_component=Cr_component*8;
	Cb_component=Cb_component*8;
	simple_bitmap_p->character_color.Y_component = Y_component;
	simple_bitmap_p->character_color.opaque_enable = opaque_enable;
	simple_bitmap_p->character_color.Cr_component = Cr_component;
	simple_bitmap_p->character_color.Cb_component = Cb_component;
	sectionData+=2;
	remain_len-=2;
	bitmap_top_H_coordinate=(sectionData[0]<<4)+((sectionData[1]&0xf0)>>4);
	bitmap_top_V_Coordinate=((sectionData[1]&0x0f)<<8)+sectionData[2];
	bitmap_bottom_H_coordinate=(sectionData[3]<<4)+((sectionData[4]&0xf0)>>4);;
	bitmap_bottom_V_coordinate=((sectionData[4]&0x0f)<<8)+sectionData[5];
	simple_bitmap_p->bitmap_coordinate.top_H_coordinate = bitmap_top_H_coordinate;
	simple_bitmap_p->bitmap_coordinate.top_V_coordinate = bitmap_top_V_Coordinate;
	simple_bitmap_p->bitmap_coordinate.bottom_H_coordinate = bitmap_bottom_H_coordinate;
	simple_bitmap_p->bitmap_coordinate.bottom_V_coordinate = bitmap_bottom_V_coordinate;
	sectionData+=6;
	remain_len-=6;
	if (BACKGROUND_STYLE_FRAMED == simple_bitmap_p->background_style)
	{
		frame_top_H_coordinate=(sectionData[0]<<4)+((sectionData[1]&0xf0)>>4);
		frame_top_V_coordinate=((sectionData[1]&0x0f)<<8)+sectionData[2];
		frame_bottom_H_coordinate=(sectionData[3]<<4)+((sectionData[4]&0xf0)>>4);;
		frame_bottom_V_coordinate=((sectionData[4]&0x0f)<<8)+sectionData[5];
		sectionData+=6;
		remain_len-=6;
		simple_bitmap_p->frame.frame_coordinate.top_H_coordinate = frame_top_H_coordinate;
		simple_bitmap_p->frame.frame_coordinate.top_V_coordinate = frame_top_V_coordinate;
		simple_bitmap_p->frame.frame_coordinate.bottom_H_coordinate = frame_bottom_H_coordinate;
		simple_bitmap_p->frame.frame_coordinate.bottom_V_coordinate = frame_bottom_V_coordinate;
		Y_component = (sectionData[0]>>3)&0x1f;
		opaque_enable = (sectionData[0]>>2)&0x1;
		Cr_component = ((sectionData[0]&0x3)<<3)+((sectionData[1]>>5)&0x7);
		Cb_component = sectionData[1]&0x1f;
		Y_component=Y_component*8;
		Cr_component=Cr_component*8;
		Cb_component=Cb_component*8;
		simple_bitmap_p->frame.frame_color.Y_component = Y_component;
		simple_bitmap_p->frame.frame_color.opaque_enable = opaque_enable;
		simple_bitmap_p->frame.frame_color.Cr_component = Cr_component;
		simple_bitmap_p->frame.frame_color.Cb_component = Cb_component;
		sectionData+=2;
		remain_len-=2;
	}

	if (OUTLINE_STYLE_OUTLINE == simple_bitmap_p->outline_style)
	{
		outline_thickness = sectionData[0]&0xf;
		sectionData+=1;
		remain_len-=1;
		Y_component = (sectionData[0]>>3)&0x1f;
		opaque_enable = (sectionData[0]>>2)&0x1;
		Cr_component = ((sectionData[0]&0x3)<<3)+((sectionData[1]>>5)&0x7);
		Cb_component = sectionData[1]&0x1f;
		Y_component=Y_component*8;
		Cr_component=Cr_component*8;
		Cb_component=Cb_component*8;
		simple_bitmap_p->outline.outline_thickness = outline_thickness;
		simple_bitmap_p->outline.outline_color.Y_component = Y_component;
		simple_bitmap_p->outline.outline_color.opaque_enable = opaque_enable;
		simple_bitmap_p->outline.outline_color.Cr_component = Cr_component;
		simple_bitmap_p->outline.outline_color.Cb_component = Cb_component;
		sectionData+=2;
		remain_len-=2;
	}
	else
		if (OUTLINE_STYLE_DROP_SHADOW == simple_bitmap_p->outline_style)
		{
			shadow_right=(sectionData[0]>>4)&0xf;
			shadow_bottom=sectionData[0]&0xf;
			sectionData+=1;
			remain_len-=1;
			Y_component = (sectionData[0]>>3)&0x1f;
			opaque_enable = (sectionData[0]>>2)&0x1;
			Cr_component = ((sectionData[0]&0x3)<<3)+((sectionData[1]>>5)&0x7);
			Cb_component = sectionData[1]&0x1f;
			Y_component=Y_component*8;
			Cr_component=Cr_component*8;
			Cb_component=Cb_component*8;
			simple_bitmap_p->shadow.shadow_right = shadow_right;
			simple_bitmap_p->shadow.shadow_bottom = shadow_bottom;
			simple_bitmap_p->shadow.shadow_color.Y_component = Y_component;
			simple_bitmap_p->shadow.shadow_color.opaque_enable = opaque_enable;
			simple_bitmap_p->shadow.shadow_color.Cr_component = Cr_component;
			simple_bitmap_p->shadow.shadow_color.Cb_component = Cb_component;
			sectionData+=2;
			remain_len-=2;
		}
		else
			if (OUTLINE_STYLE_RESERVED == outline_style)
			{
				sectionData+=3;
				remain_len-=3;
			}
	bitmap_length = (sectionData[0]<<8)+sectionData[1];
	sectionData+=2;
	remain_len-=2;
	compressed_bitmap_len=remain_len-4; /*crc*/
	//CC_SUBTITLE_COMMON_DBG(("bitmap_length = 0x%x\n",bitmap_length));
	//CC_SUBTITLE_COMMON_DBG(("compressed_bitmap_len = 0x%x\n",compressed_bitmap_len));
	simple_bitmap_p->bitmap_length = bitmap_length;
	if (compressed_bitmap_len>bitmap_length)
	{
		CC_SUBTITLE_COMMON_ERROR(("compressed_bitmap_len = %d bitmap_length=%d error\n",
					compressed_bitmap_len,bitmap_length));
		return -1;
	}
	simple_bitmap_p->data = (char*)av_mallocz(bitmap_length+1);
	if (NULL == simple_bitmap_p->data)
	{
		CC_SUBTITLE_COMMON_ERROR(("simple_bitmap_p->data malloc failed\n"));
		return -1;
	}
	simple_bitmap_p->segment[0].recieved = TRUE;
	simple_bitmap_p->segment[0].compressed_bitmap_len= compressed_bitmap_len;
	memcpy(simple_bitmap_p->data,sectionData,compressed_bitmap_len);
	return 0;
}


int32_t gx_scte_sub_compressed_bitmap(scte_sub_simple_bitmap_t* simple_bitmap_p)
{
	char*	sectionData=NULL;
	uint8_t steering1=0;
	uint8_t steering2=0;
	uint8_t steering3=0;
	uint8_t operator_eol=0;
	uint8_t run_length_16_on=0;
	uint8_t run_length_64_off=0;
	uint8_t run_length_8_on=0;
	uint8_t run_length_32_off=0;
	uint8_t bit_remain=0;
	int16_t parse_len = 0;
	uint16_t x=0;
	uint16_t y=0;
	GuiSurface *font_surface=NULL;
	uint8_t font_bpp=32;
	GxColorFormat font_color_format = GX_COLOR_FMT_ARGB8888;
	GAL_Rect create_rect = {0};
	GXGDI_Rect font_rect={0};
	GXGDI_Rect spp_rect={0};
	char*	data=NULL;
	uint16_t len=0;
	uint8_t Y_component=0;
	uint8_t Cb_component=0;
	uint8_t Cr_component=0;
	uint8_t a=0xff;
	uint8_t r=0;
	uint8_t g=0;
	uint8_t b=0;
	uint32_t font_color=0;
	uint16_t i=0;
	int32_t ret=-1;
	uint16_t standard_width=0;
	uint16_t standard_height=0;
	uint16_t x_offset=0;
	uint16_t y_offset=0;
	GxVpuProperty_FillRect FillRect = {0};
	GxTime starttime={0};
	OPACITY_TYPE opacity=OPACITY_SOLID;

	if ((NULL == simple_bitmap_p)||(NULL == simple_bitmap_p->data))
	{
		CC_SUBTITLE_COMMON_ERROR(("gx_scte_sub_compressed_bitmap para NULL error\n"));
		return -1;
	}

	gx_scte_sub_spp_clear_pre_display();
	data = simple_bitmap_p->data;
	len = simple_bitmap_p->bitmap_length;

	Y_component = simple_bitmap_p->character_color.Y_component;
	Cb_component = simple_bitmap_p->character_color.Cb_component;
	Cr_component = simple_bitmap_p->character_color.Cr_component;
	gx_ycbcr_rgb(Y_component,Cb_component,Cr_component,&r,&g,&b);
	font_color = (a<<24)|(r<<16)|(g<<8)|b;


	switch(simple_bitmap_p->background_style)
	{
	case BACKGROUND_STYLE_TRANSPARENT:
		create_rect.x = 0;
		create_rect.y = 0;
		create_rect.w = simple_bitmap_p->bitmap_coordinate.bottom_H_coordinate-simple_bitmap_p->bitmap_coordinate.top_H_coordinate;
		create_rect.h = simple_bitmap_p->bitmap_coordinate.bottom_V_coordinate-simple_bitmap_p->bitmap_coordinate.top_V_coordinate;
		break;
	case BACKGROUND_STYLE_FRAMED:
		create_rect.x = 0;
		create_rect.y = 0;
		create_rect.w = simple_bitmap_p->frame.frame_coordinate.bottom_H_coordinate-simple_bitmap_p->frame.frame_coordinate.top_H_coordinate;
		create_rect.h = simple_bitmap_p->frame.frame_coordinate.bottom_V_coordinate-simple_bitmap_p->frame.frame_coordinate.top_V_coordinate;
		x_offset=simple_bitmap_p->bitmap_coordinate.top_H_coordinate-simple_bitmap_p->frame.frame_coordinate.top_H_coordinate;
		y_offset=simple_bitmap_p->bitmap_coordinate.top_V_coordinate-simple_bitmap_p->frame.frame_coordinate.top_V_coordinate;
		break;
	default:
		break;
	}

	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=create_rect.w;
	font_rect.h=create_rect.h;

	Y_component = simple_bitmap_p->frame.frame_color.Y_component;
	Cb_component = simple_bitmap_p->frame.frame_color.Cb_component;
	Cr_component = simple_bitmap_p->frame.frame_color.Cr_component;

	switch(simple_bitmap_p->background_style)
	{
	case BACKGROUND_STYLE_TRANSPARENT:
		opacity=OPACITY_TRANSPARENT;
		break;
	case BACKGROUND_STYLE_FRAMED:
		switch(simple_bitmap_p->frame.frame_color.opaque_enable)
		{
		case 0:
			if ((0 == Y_component)
					&&(0 == Cb_component)
					&&(0 == Cr_component))
			{
				opacity=OPACITY_TRANSPARENT;
			}
			else
			{
				opacity=OPACITY_SOLID;
			}
			break;
		case 1:
			FillRect.color.alpha = OPACITY_TRANSLUCENT;
			break;
		default:
			opacity=OPACITY_SOLID;
			break;
		}
		break;
	default:
		break;
	}

	font_surface = gx_create_font_surface(&create_rect, font_bpp, font_color_format,Y_component,Cb_component,Cr_component,opacity);
	if (NULL == font_surface)
	{
		CC_SUBTITLE_COMMON_ERROR(("hd_get_surface0 failed create_rect.w=%d create_rect.h=%d \n",
					create_rect.w,create_rect.h));
		return -1;
	}

	sectionData = data;

	while(parse_len <len)
	{
		if ( 0 == bit_remain)
		{
			bit_remain = 8;
			steering1=(sectionData[parse_len]>>(bit_remain-1))&0x1;
			bit_remain = bit_remain-1;

		}
		else
		{
			steering1=(sectionData[parse_len]>>(bit_remain-1))&0x1;
			bit_remain = bit_remain-1;
		}

		if (0 == steering1)
		{
			if ( 0 == bit_remain)
			{
				if (parse_len+1 >=len)
				{
					break;
				}
				parse_len++;
				bit_remain = 8;
				steering2=(sectionData[parse_len]>>(bit_remain-1))&0x1;
				bit_remain = bit_remain-1;

			}
			else
			{
				steering2=(sectionData[parse_len]>>(bit_remain-1))&0x1;
				bit_remain = bit_remain-1;
			}
			if (0 == steering2)
			{
				if ( 0 == bit_remain)
				{
					if (parse_len+1 >=len)
					{
						break;
					}
					parse_len++;
					bit_remain = 8;
					steering3=(sectionData[parse_len]>>(bit_remain-1))&0x1;
					bit_remain = bit_remain-1;

				}
				else
				{
					steering3=(sectionData[parse_len]>>(bit_remain-1))&0x1;
					bit_remain = bit_remain-1;
				}
				if (0 == steering3)
				{
					if (bit_remain >=2)
					{
						operator_eol = (sectionData[parse_len]>>(bit_remain-2))&0x3;
						bit_remain = bit_remain-2;
					}
					else
						if (0 == bit_remain)
						{
							if (parse_len+1 >=len)
							{
								break;
							}
							parse_len++;
							bit_remain = 8;
							operator_eol = (sectionData[parse_len]>>(bit_remain-2))&0x3;
							bit_remain = bit_remain-2;
						}
						else
							if (bit_remain <2)
							{
								if (parse_len+1 >=len)
								{
									break;
								}
								operator_eol = sectionData[parse_len]<<(8-bit_remain);
								operator_eol = operator_eol>>(8-bit_remain);
								operator_eol = (operator_eol<<(2-bit_remain))
									+(sectionData[parse_len+1]>>(8-(2-bit_remain)));
								parse_len++;
								bit_remain = 8-(2-bit_remain);
							}

					switch(operator_eol)
					{
					case 0x00:
						//CC_SUBTITLE_COMMON_DBG(("No operation. Used when stuffing the final byte of a compressed bitmap.\n"));
						break;
					case 0x01:
						y++;
						x=0;
						//CC_SUBTITLE_COMMON_DBG(("End of line; move pixel cursor back to the initial horizontal coordinate and increment the vertical coordinate.\n"));
						break;
					case 0x02:
						//CC_SUBTITLE_COMMON_DBG(("reserved\n"));
						break;
					case 0x03:
						//CC_SUBTITLE_COMMON_DBG(("reserved\n"));
						break;
					default:
						break;
					}
				}
				else
				{
					if (bit_remain >=4)
					{
						run_length_16_on = (sectionData[parse_len]>>(bit_remain-4))&0xf;
						bit_remain = bit_remain-4;
					}
					else
						if (0 == bit_remain)
						{
							if (parse_len+1 >=len)
							{
								break;
							}
							parse_len++;
							bit_remain = 8;
							run_length_16_on = (sectionData[parse_len]>>(bit_remain-4))&0xf;
							bit_remain = bit_remain-4;
						}
						else
							if (bit_remain <4)
							{
								if (parse_len+1 >=len)
								{
									break;
								}
								run_length_16_on = sectionData[parse_len]<<(8-bit_remain);
								run_length_16_on = run_length_16_on>>(8-bit_remain);
								run_length_16_on = (run_length_16_on<<(4-bit_remain))
									+(sectionData[parse_len+1]>>(8-(4-bit_remain)));
								parse_len++;
								bit_remain = 8-(4-bit_remain);
							}
					if (0 == run_length_16_on)
						run_length_16_on = 16;
					for (i=0; i<run_length_16_on;i++)
					{
						*((uint32_t*)font_surface->data + x+i+x_offset+ (y+y_offset)*create_rect.w)=(uint32_t)font_color;
					}
					x += run_length_16_on;
				}
			}
			else
			{
				if (bit_remain >=6)
				{
					run_length_64_off = (sectionData[parse_len]>>(bit_remain-6))&0x3f;
					bit_remain = bit_remain-6;
				}
				else
					if (0 == bit_remain)
					{
						if (parse_len+1 >=len)
						{
							break;
						}
						parse_len++;
						bit_remain = 8;
						run_length_64_off = (sectionData[parse_len]>>(bit_remain-6))&0x3f;
						bit_remain = bit_remain-6;
					}
					else
						if (bit_remain <6)
						{
							if (parse_len+1 >=len)
							{
								break;
							}
							run_length_64_off = sectionData[parse_len]<<(8-bit_remain);
							run_length_64_off = run_length_64_off>>(8-bit_remain);
							run_length_64_off = (run_length_64_off<<(6-bit_remain))
								+(sectionData[parse_len+1]>>(8-(6-bit_remain)));
							parse_len++;
							bit_remain = 8-(6-bit_remain);
						}
				if (0 == run_length_64_off)
					run_length_64_off = 64;
				x+=run_length_64_off;

			}
		}
		else
		{
			if (bit_remain >=3)
			{						
				run_length_8_on = (sectionData[parse_len]>>(bit_remain-3))&0x7;
				bit_remain = bit_remain-3;
			}
			else
				if (0 == bit_remain)
				{
					if (parse_len+1 >=len)
					{
						break;
					}
					parse_len++;
					bit_remain = 8;
					run_length_8_on = (sectionData[parse_len]>>(bit_remain-3))&0x7;
					bit_remain = bit_remain-3;
				}
				else
					if (bit_remain <3)
					{
						if (parse_len+1 >=len)
						{
							break;
						}
						run_length_8_on = sectionData[parse_len]<<(8-bit_remain);
						run_length_8_on = run_length_8_on>>(8-bit_remain);
						run_length_8_on = (run_length_8_on<<(3-bit_remain))
							+(sectionData[parse_len+1]>>(8-(3-bit_remain)));
						parse_len++;
						bit_remain = 8-(3-bit_remain);
					}
			if (0 == run_length_8_on)
				run_length_8_on = 8;
			for (i=0; i<run_length_8_on;i++)
			{
				*((uint32_t*)font_surface->data + x+i+x_offset+ (y+y_offset)*create_rect.w)=(uint32_t)font_color;
			}
			x += run_length_8_on;

			if (bit_remain >=5)
			{
				run_length_32_off = (sectionData[parse_len]>>(bit_remain-5))&0x1f;
				bit_remain = bit_remain-5;
			}
			else
				if (0 == bit_remain)
				{
					if (parse_len+1 >=len)
					{
						break;
					}
					parse_len++;
					bit_remain = 8;
					run_length_32_off = (sectionData[parse_len]>>(bit_remain-5))&0x1f;
					bit_remain = bit_remain-5;
				}
				else
					if (bit_remain <5)
					{
						if (parse_len+1 >=len)
						{
							break;
						}
						run_length_32_off = sectionData[parse_len]<<(8-bit_remain);
						run_length_32_off = run_length_32_off>>(8-bit_remain);
						run_length_32_off = (run_length_32_off<<(5-bit_remain))
							+(sectionData[parse_len+1]>>(8-(5-bit_remain)));
						parse_len++;
						bit_remain = 8-(5-bit_remain);
					}
			if (0 == run_length_32_off)
				run_length_32_off = 32;
			x += run_length_32_off;
		}

		if ( 0 == bit_remain )
		{
			parse_len++;
		}
	}

	switch(simple_bitmap_p->display_standard)
	{
	case DISPLAY_STANDARD_720_480_30:
		standard_width=720;
		standard_height=480;
		break;
	case DISPLAY_STANDARD_720_576_25:
		standard_width=720;
		standard_height=576;
		break;
	case DISPLAY_STANDARD_1280_720_60:
		standard_width=1280;
		standard_height=720;
		break;
	case DISPLAY_STANDARD_1920_1080_60:
		standard_width=1920;
		standard_height=1080;
		break;
	default:
		standard_width=1280;
		standard_height=720;
		break;
	}

	GXGDI_Lock();
	if ((scte_sub_para.width== standard_width)
			&&(scte_sub_para.height== standard_height))
	{
		spp_rect.x = simple_bitmap_p->bitmap_coordinate.top_H_coordinate;
		spp_rect.y = simple_bitmap_p->bitmap_coordinate.top_V_coordinate;
		spp_rect.w=create_rect.w;
		spp_rect.h=create_rect.h;
		GXGDI_Begin();
		ret =  GXGDI_Blit(font_surface, &font_rect, spp_cc_subtitle_surface, &spp_rect, GDI_BLIT_STRETCH);
		GXGDI_End();
	}
	else
	{
		GuiSurface *font_zoom_surface=NULL;
		create_rect.x = 0;
		create_rect.y = 0;
		create_rect.w=font_rect.w*scte_sub_para.width/standard_width;;
		create_rect.h=font_rect.h*scte_sub_para.height/standard_height;
		font_zoom_surface = hd_get_surface0(&create_rect, font_bpp, font_color_format, NULL, TRUE);
		if (NULL == font_zoom_surface)
		{
			hd_free_surface(font_surface);
			GXGDI_Unlock();
			font_surface=NULL;
			CC_SUBTITLE_COMMON_ERROR(("hd_get_surface0 failed\n"));
			return -1;
		}
		ret =  hd_zoom_surface(font_surface, (GAL_Rect*)&font_rect, font_zoom_surface, (GAL_Rect*)&create_rect);
		spp_rect.x = simple_bitmap_p->bitmap_coordinate.top_H_coordinate*scte_sub_para.width/standard_width;
		spp_rect.y = simple_bitmap_p->bitmap_coordinate.top_V_coordinate*scte_sub_para.height/standard_height;
		spp_rect.w=font_rect.w*scte_sub_para.width/standard_width;;
		spp_rect.h=font_rect.h*scte_sub_para.height/standard_height;
		font_rect.x = 0;
		font_rect.y = 0;
		font_rect.w=font_rect.w*scte_sub_para.width/standard_width;;
		font_rect.h=font_rect.h*scte_sub_para.height/standard_height;
		GXGDI_Begin();
		ret =  GXGDI_Blit(font_zoom_surface, &font_rect, spp_cc_subtitle_surface, &spp_rect, GDI_BLIT_STRETCH);
		GXGDI_End();
		hd_free_surface(font_zoom_surface);
		font_zoom_surface=NULL;
	}
	hd_free_surface(font_surface);
	GXGDI_Unlock();
	font_surface=NULL;
	simple_bitmap_p->display_status=TRUE;
	GxCore_GetTickTime(&starttime);
	simple_bitmap_p->display_start_ms=starttime.seconds*1000 + starttime.microsecs/1000;
	if (GXCORE_SUCCESS != ret)
	{
		CC_SUBTITLE_COMMON_ERROR(("GXGDI_FillRect failed\n"));
		return -1;
	}

	return 0;
}



int32_t gx_scte_sub_message_parse( char *data,size_t Size)
{
	uint8_t* sectionData;
	int32_t  section_length = 0;
	char*   tmp_data=data;
	uint8_t segmentation_overlay_included=0;
	uint8_t protocol_version=0;
	uint16_t table_extension=0;
	uint16_t segment_number=0;
	uint16_t last_segment_number=0;
	uint8_t pre_clear_display=0;
	uint8_t immediate=0;
	uint8_t display_standard=0;
	uint8_t subtitle_type=0;
	uint8_t frame_per_sec=0;
	uint16_t block_length=0;
	uint16_t i = 0;
	uint16_t offset_length=0;
	uint16_t remain_length=0;
	uint16_t compressed_bitmap_len=0;
	scte_sub_simple_bitmap_t* simple_bitmap_p=NULL;

	if (NULL == data)
		return -1;

	if (0xc6 != tmp_data[0])
	{
		CC_SUBTITLE_COMMON_ERROR(("tableid=%d\n", tmp_data[0]));
		return -1;
	}
	section_length = ((tmp_data[1]&0x0f)<<8) + tmp_data[2] + 3;
	if (Size != section_length)
	{
		CC_SUBTITLE_COMMON_ERROR(("Size=%d secTotalLen=%d\n", Size, section_length));
		return -1;
	}

	if (section_length>1024)
	{
		CC_SUBTITLE_COMMON_ERROR(("secTotalLen=%d\n",section_length));
		return -1;
	}

	segmentation_overlay_included=(tmp_data[3]>>6)&0x01;
	CC_SUBTITLE_COMMON_DBG(("segmentation_overlay_included=0x%x\n",segmentation_overlay_included));
	protocol_version=tmp_data[3]&0x3f;
	sectionData = (uint8_t*)tmp_data+4;
	remain_length = section_length-4;
	if (TRUE == segmentation_overlay_included)
	{
		table_extension=((sectionData[0]&0x0f)<<8) + sectionData[1];
		CC_SUBTITLE_COMMON_DBG(("table_extension=0x%x\n",table_extension));
		last_segment_number=(sectionData[2]<<4)+((sectionData[3]&0xf0)>>4);
		CC_SUBTITLE_COMMON_DBG(("last_segment_number=0x%x\n",last_segment_number));
		segment_number=((sectionData[3]&0x0f)<<4)+sectionData[4];
		CC_SUBTITLE_COMMON_DBG(("segment_number=0x%x\n",segment_number));
		sectionData+=5;
		remain_length-=5;
	}

	simple_bitmap_p = gx_scte_sub_get_simple_bitmap(segmentation_overlay_included,table_extension,segment_number);
	if (NULL == simple_bitmap_p)
	{
		CC_SUBTITLE_COMMON_ERROR(("segmentation_overlay_included =%d table_extension =%d segment_number =%d\n",segmentation_overlay_included,table_extension,segment_number));
		return -1;
	}
	if (segmentation_overlay_included != simple_bitmap_p->segmentation_overlay_included)
		simple_bitmap_p->segmentation_overlay_included = segmentation_overlay_included;
	if (table_extension != simple_bitmap_p->table_extension)
		simple_bitmap_p->table_extension = table_extension;
	if (last_segment_number != simple_bitmap_p->last_segment_number)
		simple_bitmap_p->last_segment_number = last_segment_number;

	if ((FALSE == segmentation_overlay_included)
			||((TRUE == segmentation_overlay_included)&&(0 == segment_number)))
	{
		if((NULL != simple_bitmap_p->segment)&&(TRUE == simple_bitmap_p->segment[0].recieved))
		{
			CC_SUBTITLE_COMMON_ERROR(("segment_number parse already\n"));
			return 0;
		}

		if (NULL == simple_bitmap_p->segment)
		{
			simple_bitmap_p->segment = (scte_sub_segment_t *)av_mallocz(
					(last_segment_number+1)*sizeof(scte_sub_segment_t));
			if (NULL == simple_bitmap_p->segment)
			{
				CC_SUBTITLE_COMMON_ERROR(("simple_bitmap_p->segment malloc failed\n"));
				return -1;
			}
		}

		memcpy(simple_bitmap_p->ISO_639_language_code,sectionData,3);
		if ((0xFF == simple_bitmap_p->ISO_639_language_code[0])
				&&(0xFF == simple_bitmap_p->ISO_639_language_code[1])
				&&(0xFF == simple_bitmap_p->ISO_639_language_code[2]))
		{
			;
			//CC_SUBTITLE_COMMON_DBG(("ISO_639_language_code universal\n"));
		}
		else
		{
			;
			//CC_SUBTITLE_COMMON_DBG(("ISO_639_language_code=%s\n",simple_bitmap_p->ISO_639_language_code));
		}
		sectionData+=3;
		remain_length-=3;
		pre_clear_display = (sectionData[0]>>7)&0x1;
		CC_SUBTITLE_COMMON_DBG(("pre_clear_display=0x%x\n",pre_clear_display));
		immediate = (sectionData[0]>>6)&0x1;
		CC_SUBTITLE_COMMON_DBG(("immediate=0x%x\n",immediate));
		if (TRUE == immediate)
		{
			simple_bitmap_p->display_modes = IMMEDIATE;
		}
		if (TRUE == pre_clear_display)
		{
			simple_bitmap_p->display_modes = PRE_CLEAR_DISPLAY;
		}
		display_standard = sectionData[0]&0x1f;
		switch(display_standard)
		{
		case 0:
			simple_bitmap_p->display_standard = DISPLAY_STANDARD_720_480_30;
			frame_per_sec=30;
			CC_SUBTITLE_COMMON_DBG(("display_standard _720_480_30\n"));
			break;
		case 1:
			simple_bitmap_p->display_standard = DISPLAY_STANDARD_720_576_25;
			frame_per_sec=25;
			CC_SUBTITLE_COMMON_DBG(("display_standard _720_576_25\n"));
			break;
		case 2:
			simple_bitmap_p->display_standard = DISPLAY_STANDARD_1280_720_60;
			frame_per_sec=60;
			CC_SUBTITLE_COMMON_DBG(("display_standard _1280_720_60\n"));
			break;
		case 3:
			simple_bitmap_p->display_standard = DISPLAY_STANDARD_1920_1080_60;
			frame_per_sec=60;
			CC_SUBTITLE_COMMON_DBG(("display_standard _1920_1080_60\n"));
			break;
		default:
			frame_per_sec=30;
			break;
		}
		sectionData+=1;
		remain_length-=1;
		simple_bitmap_p->display_in_PTS = (int64_t)((unsigned int)(sectionData[0]<<24)|(sectionData[1]<<16)|(sectionData[2]<<8)|sectionData[3]);
		//CC_SUBTITLE_COMMON_DBG(("display_in_PTS = sectionData[%d]=0x%x sectionData[%d]=0x%x sectionData[%d]=0x%x sectionData[%d]=0x%x \n",0,sectionData[0],1,sectionData[1],2,sectionData[2],3,sectionData[3]));
		//CC_SUBTITLE_COMMON_DBG(("display_in_PTS = 0x%llx\n",simple_bitmap_p->display_in_PTS));
		sectionData+=4;
		remain_length-=4;
		subtitle_type=(sectionData[0]>>4)&0xf;
		if (1 == subtitle_type)
		{
			simple_bitmap_p->sub_type = SUB_TYPE_SIMPLE_BITMAP;
			//CC_SUBTITLE_COMMON_DBG(("subtitle_type simple_bitmap\n"));
		}
		else
		{
			simple_bitmap_p->sub_type = SUB_TYPE_REVERSE;
		}
		simple_bitmap_p->display_duration = ((sectionData[0]&0x7)<<3)+sectionData[1];
		//CC_SUBTITLE_COMMON_DBG(("display_duration =0x%04x frame\n",simple_bitmap_p->display_duration));
		simple_bitmap_p->display_duration=simple_bitmap_p->display_duration*1000/frame_per_sec;
		sectionData+=2;
		remain_length-=2;
		block_length=(sectionData[0]<<8)+sectionData[1];
		sectionData+=2;
		remain_length-=2;
		simple_bitmap_p->block_length = block_length;
		if (SUB_TYPE_SIMPLE_BITMAP == simple_bitmap_p->sub_type)
		{
			gx_scte_sub_simple_bitmap_parse((char*)sectionData,remain_length,simple_bitmap_p);
			if (FALSE ==segmentation_overlay_included)
			{
				//CC_SUBTITLE_COMMON_DBG(("simple_bitmap_p->data[%d] = 0x%x\n",
				//simple_bitmap_p->bitmap_length-1,simple_bitmap_p->data[simple_bitmap_p->bitmap_length-1]));
				simple_bitmap_p->segment_recieve_finish=TRUE;
				if (IMMEDIATE == simple_bitmap_p->display_modes)
				{
					gx_scte_sub_compressed_bitmap(simple_bitmap_p);
				}
			}
		}
	}
	else
	{
		compressed_bitmap_len = remain_length-4; /*crc*/
		if (TRUE == simple_bitmap_p->segment[segment_number].recieved)
		{
			CC_SUBTITLE_COMMON_DBG(("segment_number = %d recieved already\n",segment_number));
			return 0;
		}
		offset_length = 0;
		for (i = 0;i< segment_number;i++)
		{
			if (FALSE == simple_bitmap_p->segment[i].recieved)
			{
				CC_SUBTITLE_COMMON_ERROR(("segment_number = %d not recieved error\n",i));
				return -1;
			}
			else
			{
				offset_length += simple_bitmap_p->segment[i].compressed_bitmap_len;
			}
		}

		if (segment_number != last_segment_number)
		{
			if (offset_length+compressed_bitmap_len>simple_bitmap_p->bitmap_length)
			{
				CC_SUBTITLE_COMMON_ERROR(("offset_length=%d compressed_bitmap_len = %d bitmap_length=%d error\n",
							offset_length,compressed_bitmap_len,simple_bitmap_p->bitmap_length));
				return -1;
			}
		}
		else
		{
			if (offset_length>=simple_bitmap_p->bitmap_length)
			{
				CC_SUBTITLE_COMMON_ERROR(("offset_length=%d compressed_bitmap_len = %d bitmap_length=%d error\n",
							offset_length,compressed_bitmap_len,simple_bitmap_p->bitmap_length));
				return -1;
			}
			compressed_bitmap_len = simple_bitmap_p->bitmap_length-offset_length;

		}


		simple_bitmap_p->segment[segment_number].recieved = TRUE;
		simple_bitmap_p->segment[segment_number].compressed_bitmap_len= compressed_bitmap_len;
		memcpy(simple_bitmap_p->data+offset_length,sectionData,compressed_bitmap_len);
		if (segment_number == simple_bitmap_p->last_segment_number)
		{
			//CC_SUBTITLE_COMMON_DBG(("simple_bitmap_p->data[%d] = 0x%x\n",
			//simple_bitmap_p->bitmap_length-1,simple_bitmap_p->data[simple_bitmap_p->bitmap_length-1]));
			simple_bitmap_p->segment_recieve_finish=TRUE;
			if (IMMEDIATE == simple_bitmap_p->display_modes)
			{
				gx_scte_sub_compressed_bitmap(simple_bitmap_p);
			}


		}
	}

	return 0;
}


static void gx_scte_sub_draw_thread(void* args)
{
	uint32_t i = 0;
	GxTime nowtime={0};
	uint32_t nowMs;
	GXGDI_Rect spp_rect={0};
    GxScteSubtitleSyncState sync_state = GX_SCTE_SYNC_ERROR;
	
    while(1)
    {
		gx_scte_sub_lock();
		for (i=0; i< TABLE_EXTENSION_MAX; i++)
		{
			if ((TRUE == simple_bitmap[i].status)
					&&(TRUE == simple_bitmap[i].display_status))
			{
				GxCore_GetTickTime(&nowtime);
				nowMs = nowtime.seconds*1000 + nowtime.microsecs/1000;
				if (nowMs - simple_bitmap[i].display_start_ms >= simple_bitmap[i].display_duration)
				{
					spp_rect.x=0;
					spp_rect.y=0;
					spp_rect.w=scte_sub_para.width;
					spp_rect.h=scte_sub_para.height;
					CC_SUBTITLE_COMMON_DBG(("display consume display_duration i=%d , clear\n",i));
					gx_clear_cc_subtitle_spp(spp_rect,spp_cc_subtitle_surface);
					if (TRUE == simple_bitmap[i].status)
					{
						if (NULL != simple_bitmap[i].data)
						{
							av_free(simple_bitmap[i].data);
							simple_bitmap[i].data=NULL;
						}

						if (NULL != simple_bitmap[i].segment)
						{
							av_free(simple_bitmap[i].segment);
							simple_bitmap[i].segment=NULL;
						}
						memset(&simple_bitmap[i],0,sizeof(scte_sub_simple_bitmap_t));
					}
				}
			}
			else if ((TRUE == simple_bitmap[i].status)
					&&(TRUE == simple_bitmap[i].segment_recieve_finish)
					&&(FALSE == simple_bitmap[i].display_status))
			{
				if ((NULL != gx_scte_subtitle_param.priv)&&(NULL != gx_scte_subtitle_param.sync_pts))
				{
					sync_state = gx_scte_subtitle_param.sync_pts(gx_scte_subtitle_param.priv,simple_bitmap[i].display_in_PTS);
				}

				if ((GX_SCTE_SYNC_FREERUN == sync_state)||(GX_SCTE_SYNC_NORMAL == sync_state)
						||(GX_SCTE_SYNC_SKIP == sync_state))
				{
//					CC_SUBTITLE_COMMON_DBG(("simple_bitmap->display_in_PTS= 0x%llx i=%d display start\n",
//								simple_bitmap[i].display_in_PTS,i));
					gx_scte_sub_compressed_bitmap(&simple_bitmap[i]);
				}
			}
		}
		gx_scte_sub_unlock();
		GxCore_ThreadDelay(50);
	}
}

static void gx_scte_sub_filter_thread(void* args)
{
	uint8_t *section;
	int32_t ret=-1;
	int32_t length=0;
	int32_t len=0;
	uint32_t section_len=0;
	char *data = NULL;

	section = (uint8_t *)av_malloc(64*1024);

	if( section == NULL )
	{
		CC_SUBTITLE_COMMON_ERROR(("av_malloc failed\n"));
		return;
	}

	while(1)
	{
		gx_scte_sub_lock();
		if (SUB_ON != sub_status)
		{
			gx_scte_sub_unlock();
			GxCore_ThreadDelay(50);
			continue;
		}
#if 0
		if ((-1 == sub_filter)||(-1 == sub_channel))
		{
			GxCore_ThreadDelay(100);
			gx_scte_sub_unlock();
			continue;
		}
#endif
		memset(section,0,64*1024);
		ret =0;
		length = 0;
#if 0
		ret = GxFilterRead(sub_filter, section, 64*1024, &length);
#else
		if ((NULL != gx_scte_subtitle_param.priv)&&(NULL != gx_scte_subtitle_param.read_packet))
		{
			length = gx_scte_subtitle_param.read_packet(gx_scte_subtitle_param.priv,section,64*1024);
		}
#endif
		if ((0 == ret)&&(length > 0))
		{
			len = length;
			data = (char *)section;
			while(len>0)
			{
				section_len = (unsigned int)(((data[1]&0xf) << 8) | data[2]) + 3;
				gx_scte_sub_message_parse(data,section_len);
				data += section_len;
				len -= section_len;
			}
		}
		gx_scte_sub_unlock();
		GxCore_ThreadDelay(50);
	}
}

int gx_scte_sub_open(GxScteSubtitleWindow sub_para)
{
	GAL_Rect create_rect = {0};
	int32_t ret = -1;

	gx_scte_sub_lock();
	if (SUB_ON == sub_status)
	{
		gx_scte_sub_unlock();
		gx_scte_sub_close();
		gx_scte_sub_lock();
	}

	sub_para.x = 0;
	sub_para.y = 0;
	sub_para.width  = subtitleConfig.canvas.width;
	sub_para.height = subtitleConfig.canvas.height;

	create_rect.x = sub_para.x;
	create_rect.y = sub_para.y;
	create_rect.w = sub_para.width;
	create_rect.h = sub_para.height;
	ret = gx_init_cc_subtitle_spp(create_rect);
	if (0 != ret)
	{
		gx_scte_sub_unlock();
		CC_SUBTITLE_COMMON_ERROR(("gx_init_cc_subtitle_spp failed\n"));
		return -1;
	}

	memcpy(&scte_sub_para,&sub_para,sizeof(GxScteSubtitleWindow));
	sub_status = SUB_ON;
	//gx_scte_sub_filter_start(scte_sub_para.demux_id,scte_sub_para.pid);
	gx_scte_sub_unlock();
	return 0;
}

int gx_scte_sub_close(void)
{
	uint32_t i = 0;

	gx_scte_sub_lock();
	//gx_scte_sub_filter_stop();
	if (SUB_OFF == sub_status)
	{
		gx_scte_sub_unlock();
		return 0;
	}

	gx_close_cc_subtitle_spp();
	sub_status = SUB_OFF;
	memset(&gx_scte_subtitle_param,0,sizeof(GxScteSubtitleParam));
	memset(&scte_sub_para,0,sizeof(GxScteSubtitleWindow));
	for (i=0; i< TABLE_EXTENSION_MAX; i++)
	{
		if (TRUE == simple_bitmap[i].status)
		{
			if (NULL != simple_bitmap[i].data)
			{
				av_free(simple_bitmap[i].data);
				simple_bitmap[i].data=NULL;
			}

			if (NULL != simple_bitmap[i].segment)
			{
				av_free(simple_bitmap[i].segment);
				simple_bitmap[i].segment=NULL;
			}
			memset(&simple_bitmap[i],0,sizeof(scte_sub_simple_bitmap_t));
		}
	}
	gx_scte_sub_unlock();
	return 0;
}
#endif

handle_t GxScteSubtitle_Open(GxScteSubtitleParam *param)
{
#ifdef CONFIG_SUBTITLE_GDI_DRAW
	static handle_t    sub_filter_thread=-1;
	static handle_t    sub_draw_thread=-1;
	if ((NULL == param)||(NULL == param->priv)||(NULL == param->read_packet))
	{
		CC_SUBTITLE_COMMON_ERROR(("GxScteSubtitle_Open para NULL failed\n"));
		return -1;
	}
	gx_scte_sub_lock_init();
	gx_scte_sub_lock();

	memset(&simple_bitmap[0],0,TABLE_EXTENSION_MAX*sizeof(scte_sub_simple_bitmap_t));
	memset(&gx_scte_subtitle_param,0,sizeof(GxScteSubtitleParam));
	memcpy(&gx_scte_subtitle_param,param,sizeof(GxScteSubtitleParam));
	gx_scte_sub_unlock();
	if (-1 == sub_filter_thread)
	{
		GxCore_ThreadCreate("sub_filter_thread",&sub_filter_thread, gx_scte_sub_filter_thread, NULL, 1024 * 64, GXOS_DEFAULT_PRIORITY);
	}

	if (-1 == sub_draw_thread)
	{
		GxCore_ThreadCreate("sub_draw",&sub_draw_thread, gx_scte_sub_draw_thread, NULL,1024 * 64, GXOS_DEFAULT_PRIORITY);
	}


	return gx_scte_sub_open(param->win);
#else
	return 0;
#endif
}

int32_t GxScteSubtitle_Clean(handle_t handle)
{
#ifdef CONFIG_SUBTITLE_GDI_DRAW
	return gx_scte_sub_spp_clear_pre_display();
#else
	return 0;
#endif
}

int32_t GxScteSubtitle_Stop(handle_t handle)
{
	return 0;
}

int32_t GxScteSubtitle_Show(handle_t handle)
{
	return 0;
}

int32_t GxScteSubtitle_Hide(handle_t handle)
{
	return 0;
}

int32_t GxScteSubtitle_Close(handle_t handle)
{
#ifdef CONFIG_SUBTITLE_GDI_DRAW
	return gx_scte_sub_close();
#else
	return 0;
#endif
}

int32_t GxScteSubtitle_Start(handle_t handle)
{
	return 0;
}
