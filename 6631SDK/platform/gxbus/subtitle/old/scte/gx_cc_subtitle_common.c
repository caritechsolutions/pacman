/*****************************************************************************
* 						 CONFIDENTIAL
* 	   Hangzhou Nationalchip Science and Technology Co., Ltd.
* 					 (C) All right reserved
******************************************************************************

******************************************************************************
* Release History:
VERSION	      Date			        AUTHOR		       Description
1.0	                   2018.03.19	  zhouhm 			 creation
*****************************************************************************/
#include "gxtype.h"
#include "gxos/gxcore_os.h"
#include "gxcore.h"
#include "gui_core.h"
#include "module/player/gxplayer_module.h"
#include "module/config/gxconfig.h"
#include "gxavdev.h"
#include "dvbhal/gxdemux_hal.h"
#include "gx_cc_subtitle_common.h"
#include "gx_cc_subtitle_draw.h"

static void* spp_main_surface=NULL;
GuiSurface *spp_cc_subtitle_surface=NULL;
extern int hd_add_blit_element(GuiSurface *surface);

int32_t gx_color_type_ycbcr(COLOR_TYPE color_type,uint32_t* color,uint32_t* y,uint32_t* cb,uint32_t* cr)
{
	uint8_t r=0,g=0,b=0;
	if ((NULL == color)||(NULL == y)||(NULL == cb)||(NULL == cr))
		{
			CC_SUBTITLE_COMMON_ERROR((" para NULL error \n"));	
			return -1;
		}
	
	switch(color_type)
		{
			case COLOR_BLACK:
				if ((0x000000 == (gui.config.gui_trans&0xffffff))
					||(0x000000 == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xff000001;
						r=0x00;
						g=0x00;
						b=0x01;
					}
				else
					{
						*color = 0xff000000;
						r=0x00;
						g=0x00;
						b=0x00;						
					}					
				break;
			case COLOR_WHITE:
				if ((0xffffff == (gui.config.gui_trans&0xffffff))
					||(0xffffff == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xfffffffe;
						r=0xff;
						g=0xff;
						b=0xfe;
					}
				else
					{
						*color = 0xffffffff;
						r=0xff;
						g=0xff;
						b=0xff;						
					}
				break;
			case COLOR_GREEN:
				if ((0x00ff00 == (gui.config.gui_trans&0xffffff))
					||(0x00ff00 == (gui.config.osd_trans&0xffffff)))
					{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
						*color = 0xff00ff01;
						r=0x00;
						g=0xff;
						b=0x01;
#else
						/*
						* osd mode
						*/
						*color = 0xff00cc00;
						r=0x00;
						g=0xcc;
						b=0x00;
#endif
					}
				else
					{
						*color = 0xff00ff00;
						r=0x00;
						g=0xff;
						b=0x00;
					}
				break;
			case COLOR_BLUE:
				if ((0x0000ff == (gui.config.gui_trans&0xffffff))
					||(0x0000ff == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xff0000fe;
						r=0x00;
						g=0x00;
						b=0xfe;
					}
				else
					{
						*color = 0xff0000ff;
						r=0x00;
						g=0x00;
						b=0xff;						
					}					
				break;
			case COLOR_CYAN:
				if ((0x00ffff == (gui.config.gui_trans&0xffffff))
					||(0x00ffff == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xff00fffe;
						r=0x00;
						g=0xff;
						b=0xfe; 
					}
				else
					{
						*color = 0xff00ffff;
						r=0x00;
						g=0xff;
						b=0xff;						
					}
				break;				
			case COLOR_RED:
				if ((0xff0000 == (gui.config.gui_trans&0xffffff))
					||(0xff0000 == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xffff0001;
						r=0xff;
						g=0x00;
						b=0x01; 
					}
				else
					{
						*color = 0xffff0000;
						r=0xff;
						g=0x00;
						b=0x00;						
					}
				break;
			case COLOR_YELLOW:
				if ((0xffff00 == (gui.config.gui_trans&0xffffff))
					||(0xffff00 == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xffffff01;
						r=0xff;
						g=0xff;
						b=0x01;
					}
				else
					{
						*color = 0xffffff00;
						r=0xff;
						g=0xff;
						b=0x00;
					}
				break;
			case COLOR_MAGENTA:
				if ((0xff00ff == (gui.config.gui_trans&0xffffff))
					||(0xff00ff == (gui.config.osd_trans&0xffffff)))
					{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
						*color = 0xffff00fe;
						r=0xff;
						g=0x00;
						b=0xfe;
#else
						/*
						* osd mode
						*/
						*color = 0xffff00cc;
						r=0xff;
						g=0x00;
						b=0xcc;
#endif
					}
				else
					{
						*color = 0xffff00ff;
						r=0xff;
						g=0x00;
						b=0xff;
					}
				break;
			default:
				if ((0x000000 == (gui.config.gui_trans&0xffffff))
					||(0x000000 == (gui.config.osd_trans&0xffffff)))
					{
						*color = 0xff000001;
						r=0x00;
						g=0x00;
						b=0x01;
					}
				else
					{
						*color = 0xff000000;
						r=0x00;
						g=0x00;
						b=0x00;						
					}
				break;
		}

	*y = (257*r+564*g+98*b)/1000+16;
	*cb = ((-148)*r-291*g+439*b)/1000+128;
	*cr  = (439*r-368*g-71*b)/1000+128;
	return 0;
}


int32_t gx_ycbcr_rgb(uint8_t y,uint8_t cb,uint8_t cr,uint8_t* r,uint8_t* g,uint8_t* b)
{
	unsigned int y1 = y, u1 = cb, v1 = cr;
	unsigned int r1 = 0, g1 = 0,  b1 = 0;

	if ((NULL == r)||(NULL == g)||(NULL == b)) {
			CC_SUBTITLE_COMMON_ERROR(("para NULL error\n"));
			return -1;
	}
	
	r1 = y1 + 1.402   * (v1 - 128);
	g1 = y1 - 0.34414 * (u1 - 128) - 0.71414 * (v1 - 128);
	b1 = y1 + 1.772   * (u1 - 128);

	*r = r1;
	*g = g1;
	*b = b1;
	
	return 0;
}

static inline int is_ksurface(GuiSurface *surface) {
	return (surface->hw & KSURFACE) ? 1 : 0;
}

int gx_clear_cc_subtitle_spp(GXGDI_Rect rect,GuiSurface *surface)
{
	GxVpuProperty_FillRect FillRect = {0};
	int ret = 0;

	if(NULL == surface)
	{	
		CC_SUBTITLE_COMMON_ERROR(("para NULL error\n"));
		return -1;
	}

	if ((0 == rect.w)||(0 == rect.h))
		{
			CC_SUBTITLE_COMMON_ERROR(("rect para rect.w=%d rect.h=%d error\n",
				rect.w,rect.h));
			return -1;			
		}
	GXGDI_Lock();
	GXGDI_Begin();
#if 1//(CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	FillRect.color.y = 0x10;
	FillRect.color.cb = 0x80;
	FillRect.color.cr = 0x80;
	FillRect.color.alpha = 0x00;
	FillRect.rect.x = rect.x;
	FillRect.rect.y = rect.y;
	FillRect.is_ksurface = is_ksurface(surface);
	FillRect.rect.width = rect.w;
	FillRect.rect.height = rect.h;
	FillRect.surface = surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));
	hd_add_blit_element(NULL);
#else
#endif
	GXGDI_End();
	GXGDI_Unlock();

	if(0 != ret)
	{
		CC_SUBTITLE_COMMON_ERROR(("GxVpuPropertyID_FillRect failed rect.w=%d rect.h=%d\n",
			rect.w,rect.h));
		return -1;
	}

	return 0;
}

int32_t gx_get_font_width_height(const unsigned char *name,char* show_char,int32_t *height,int32_t *width)
{
	void* font=NULL;
	if ((NULL == name)||(NULL == height)||(NULL == width)||(NULL == show_char))
		{
			CC_SUBTITLE_COMMON_ERROR(("gx_get_font_width_height para NULL error\n"));
			return -1;
		}
	
	GXGDI_Lock();
	GXGDI_SetFont(name);
	font = GXGDI_GetFont();
	if (NULL == font)
		{
			GXGDI_Unlock();
			CC_SUBTITLE_COMMON_ERROR(("GXGDI_GetFont NULL\n"));					
			return -1; 					
		}
	
	*height = GXGDI_GetFontHeight(font);
	*height=*height/2*2+2;
	*width = gdi_text_len((void*)show_char);
	GXGDI_Unlock();
	
	return 0;
}

GuiSurface *gx_create_font_surface(GAL_Rect* create_rect,int bpp, GxColorFormat color_format,uint32_t y,uint32_t cb,uint32_t cr,OPACITY_TYPE opacity)
{
	GuiSurface *spp_surface=NULL;
	GuiSurface *create_surface=NULL;	
	GXGDI_Rect font_rect={0};
	uint8_t spp_bpp=2;
	int ret = -1;
	GxVpuProperty_FillRect FillRect = {0};
	GxColorFormat spp_color_format = GX_COLOR_FMT_YCBCRA6442;

	if (NULL == create_rect)
		{
			CC_SUBTITLE_COMMON_ERROR(("gx_create_font_surface para NULL error\n"));
			return NULL;			
		}

	GXGDI_Lock();
	create_surface = hd_get_surface0(create_rect, bpp, color_format, NULL, TRUE);
	if (NULL == create_surface)
		{
			GXGDI_Unlock();
			CC_SUBTITLE_COMMON_ERROR(("hd_get_surface0 failed font_width=%d font_height=%d\n",
				create_rect->w,create_rect->h)); 		
			return NULL;
		}

	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=create_rect->w;
	font_rect.h=create_rect->h;

	spp_surface = hd_get_surface0(create_rect, spp_bpp, spp_color_format, NULL, TRUE);
	if (NULL == spp_surface)
		{
			hd_free_surface(create_surface);
			GXGDI_Unlock();
			create_surface=NULL;
			CC_SUBTITLE_COMMON_ERROR(("hd_get_surface0 failed\n")); 		
			return NULL;
		}
	memset(&FillRect,0,sizeof(GxVpuProperty_FillRect));

	switch(opacity)
		{
			case OPACITY_SOLID:
				/*
				* 不透明
				*/
				FillRect.color.y = y;
				FillRect.color.cb = cb;
				FillRect.color.cr = cr;
				FillRect.color.alpha = 0x3;
				break;
			case OPACITY_TRANSLUCENT:
				/*
				* 半透明
				*/
				FillRect.color.y = y;
				FillRect.color.cb = cb;
				FillRect.color.cr = cr;
				FillRect.color.alpha = 0x1;		
				break;
			case OPACITY_TRANSPARENT:
				/*
				* 透明
				*/
				FillRect.color.y = 0x10;
				FillRect.color.cb = 0x80;
				FillRect.color.cr = 0x80;
				FillRect.color.alpha = 0x0; 			
				break;
			default:
				FillRect.color.y = y;
				FillRect.color.cb = cb;
				FillRect.color.cr = cr;
				FillRect.color.alpha = 0x3;
				break;
		}
	
	GXGDI_Begin();
	FillRect.rect.x = 0x0;
	FillRect.rect.y = 0x0;
	FillRect.is_ksurface = is_ksurface(spp_surface);
	FillRect.rect.width = create_rect->w;
	FillRect.rect.height = create_rect->h;
	FillRect.surface = spp_surface->hw_surface;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_FillRect,
			&FillRect,
			sizeof(GxVpuProperty_FillRect));
	hd_add_blit_element(NULL);
	GXGDI_End();

	if(0 != ret)
	{
		hd_free_surface(create_surface);
		create_surface=NULL;
		hd_free_surface(spp_surface);
		GXGDI_Unlock();
		spp_surface=NULL;
		CC_SUBTITLE_COMMON_ERROR(("GxVpuPropertyID_FillRect failed rect.w=%d rect.h=%d\n",
			FillRect.rect.width,FillRect.rect.height)); 
		return NULL;
	}
	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=create_rect->w;
	font_rect.h=create_rect->h;
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	GXGDI_Begin();
	ret =  GXGDI_Blit(spp_surface, &font_rect, create_surface, &font_rect, GDI_BLIT_STRETCH);
	GXGDI_End();
#else
	ret =  GXGDI_Blit(spp_surface, &font_rect, create_surface, &font_rect, GDI_BLIT_COPY);
#endif
	hd_free_surface(spp_surface);
	GXGDI_Unlock();
	spp_surface=NULL;	
	if (GXCORE_SUCCESS != ret)
		{
			GXGDI_Lock();
			hd_free_surface(create_surface);
			GXGDI_Unlock();
			create_surface=NULL;
			CC_SUBTITLE_COMMON_ERROR(("GXGDI_Blit failed\n")); 
			return NULL;
		}
	
	return create_surface;
}

int32_t gx_init_cc_subtitle_spp(GAL_Rect create_rect)
{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	GxAvRect viewport_rect={0};
	GXGDI_Rect rect={0};
	int32_t ret = -1;
	uint8_t spp_bpp=2;
	GxColorFormat spp_color_format = GX_COLOR_FMT_YCBCRA6442;

	spp_main_surface = GxAvdev_GetLayerMainSurface(g_device_handle,vpu_handle,GX_LAYER_SPP);
	hd_enable_img(FALSE);
	if (NULL == spp_cc_subtitle_surface)
		{
			GXGDI_Lock();
			spp_cc_subtitle_surface = hd_get_surface0(&create_rect, spp_bpp, spp_color_format, NULL, TRUE);
			GXGDI_Unlock();
			if (NULL == spp_cc_subtitle_surface)
				{
					if (NULL != spp_main_surface)
						{
							GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
							spp_main_surface = NULL;
						}
					CC_SUBTITLE_COMMON_ERROR(("hd_get_surface0 failed x=%d y=%d w=%d h=%d\n",
						create_rect.x,create_rect.y,create_rect.w,create_rect.h));
					return -1;
				}				
		}

	ret = GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_cc_subtitle_surface->hw_surface);
	if(0 != ret) 
	{
		if (NULL != spp_main_surface)
			{
				GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
				spp_main_surface = NULL;
			}
		GXGDI_Lock();
		hd_free_surface(spp_cc_subtitle_surface);
		GXGDI_Unlock();
		spp_cc_subtitle_surface=NULL;
		CC_SUBTITLE_COMMON_ERROR(("GxAvdev_SetLayerMainSurface failed\n"));	
		return -1;
	}

	viewport_rect.x = viewport_rect.y = 0;
	viewport_rect.width = create_rect.w;
	viewport_rect.height = create_rect.h;
	ret = GxAvdev_SetLayerViewport(g_device_handle, vpu_handle, GX_LAYER_SPP, &viewport_rect);
	if(0 != ret) 
	{
		if (NULL != spp_main_surface)
			{
				GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
				spp_main_surface = NULL;
			}
		GXGDI_Lock();
		hd_free_surface(spp_cc_subtitle_surface);
		GXGDI_Unlock();
		spp_cc_subtitle_surface=NULL;
		CC_SUBTITLE_COMMON_ERROR(("GxAvdev_SetLayerViewport failed\n"));
		return -1;
	}
	rect.x = 0;
	rect.y = 0;
	rect.w = create_rect.w;
	rect.h = create_rect.h;
	ret = gx_clear_cc_subtitle_spp(rect,spp_cc_subtitle_surface);
	if(0 != ret) 
	{
		if (NULL != spp_main_surface)
			{
				GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
				spp_main_surface = NULL;
			}
		GXGDI_Lock();
		hd_free_surface(spp_cc_subtitle_surface);
		GXGDI_Unlock();
		spp_cc_subtitle_surface=NULL;
		CC_SUBTITLE_COMMON_ERROR(("gx_clear_cc_subtitle_spp failed ret=%d\n",ret));	
		return -1;
	}	

	ret = hd_enable_img(TRUE);
	if (0 != ret)
		{
			hd_enable_img(FALSE);
		if (NULL != spp_main_surface)
			{
				GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
				spp_main_surface = NULL;
			}
			GXGDI_Lock();
			hd_free_surface(spp_cc_subtitle_surface);
			spp_cc_subtitle_surface=NULL;
			GXGDI_Unlock();
			CC_SUBTITLE_COMMON_ERROR(("hd_enable_img failed ret=%d\n",ret));
			return -1;			
		}
	ret = hd_enable_video(FALSE);
	if (GXCORE_SUCCESS != ret)
		{
			hd_enable_img(FALSE);
			if (NULL != spp_main_surface)
				{
					GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
					spp_main_surface = NULL;
				}
			GXGDI_Lock();
			hd_free_surface(spp_cc_subtitle_surface);
			spp_cc_subtitle_surface=NULL;
			GXGDI_Unlock();
			CC_SUBTITLE_COMMON_ERROR(("hd_enable_video  failed ret=%d\n",
				ret));
			return -1;
		}
#endif

	return 0;
	
}

int32_t gx_close_cc_subtitle_spp(void)
{
#if (CC_SUPPORT_LAYER == CC_SUPPORT_SPP_LAYER)
	int32_t ret = -1;

	if (NULL != spp_main_surface)
		{
			GxAvdev_SetLayerMainSurface(g_device_handle, vpu_handle, GX_LAYER_SPP, spp_main_surface);
			spp_main_surface = NULL;
		}
	
	if (NULL != spp_cc_subtitle_surface)
		{
			GXGDI_Lock();
			hd_free_surface(spp_cc_subtitle_surface);
			spp_cc_subtitle_surface=NULL;
			GXGDI_Unlock();
		}
	ret = hd_enable_img(FALSE);
	ret = hd_enable_video(TRUE);
#endif
	return 0;
	
}




