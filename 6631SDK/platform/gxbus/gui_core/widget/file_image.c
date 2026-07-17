#include "widget.h"
#include "all_widget.h"
#include "file_image.h"
#include "gdi_core.h"
#include "gui_private.h"
#include "image_cache.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS==64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64

#else
#include <sys/mman.h> //for mmap
#endif

#endif

typedef enum _img_load_type
{
	IMG_LOAD_TYPE_RESIZE,
	IMG_LOAD_TYPE_ZOOM,
	IMG_LOAD_TYPE_CUT
}IMG_LOAD_TYPE;


static int _get_file_image_mode(GuiWidget *self)
{
	const char *value = NULL;

	if(NULL == self)
	{
		return (FILE_IMG_SINGLE);
	}

	if(self->property)
	{
		value = (const char *)hash_get(self->property, "mode");
	}

	if(NULL == value)
	{
		return (FILE_IMG_MULTIPLE);
	}

	if(0 == strcmp("single", value))
	{
		return (FILE_IMG_SINGLE);
	}
	else if(0 == strcmp("multiple", value))
	{
		return (FILE_IMG_MULTIPLE);
	}
	else
	{
		return (FILE_IMG_SINGLE);
	}
}

static int _file_image_gif_timer_func(void *userdata)
{
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL;
	GuiWidget *stack_top = NULL;

	if(NULL == userdata)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	stack_top = stack_peek(gui_core->win_stack);

	self = (GuiWidget *)userdata;

	if(stack_top != self->parent)
	{
		return (0);
	}

	widget_set_update(self, GUI_TRUE);

	return (0);
}

static int _config_file_image_parametre(GuiWidget *self)
{
	const char *value = NULL;
	GuiFileImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	/*It is ineffective in case of 8-bpp bit map*/
	image->opacity = 0xFF;
	if(self->property)
	{
		value = (const char *)hash_get(self->property, "opacity");
	}
	if((NULL != value) || (0 == image->opacity))
	{
		image->opacity = widget_get_int(self, "opacity", 0xFF);
	}

	image->mode = _get_file_image_mode(self);

	return (0);
}

static void* create_file_image(GuiWidget *self)
{
	GuiFileImage *image = NULL, *image_style = NULL;
	GuiWidget *parent = NULL;
	GuiWidget *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	image = (GuiFileImage *)GUI_MALLOC(sizeof(GuiFileImage));
	if(NULL == image)
	{
		return (NULL);
	}

	memset(image, 0, sizeof(GuiFileImage));

	image->base = self;
	self->object = (void *)image;
	self->state = 0;

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		image_style = (GuiFileImage *)GUI_MALLOC(sizeof(GuiFileImage));
		if(NULL == image_style)
		{
			GUI_FREE(image);
			return (NULL);
		}

		memset(image_style, 0, sizeof(GuiFileImage));
		style->object = (void *)image_style;

		_config_file_image_parametre(style);

		memcpy(image, image_style, sizeof(GuiFileImage));
		image->base = self;

		GUI_FREE(style->object);
	}

	_config_file_image_parametre(self);

	if((image->p.single_img) && (GIF_TYPE == image->p.single_img->type))
	{
		/*Turn on a timer*/
		image->timer = create_timer(_file_image_gif_timer_func, 50, self, TIMER_REPEAT);
	}

	if(self->signal)
	{
		image->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		image->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
	}

	if(image->create_event)
	{
		exec_widget_event_data(self, image->create_event, NULL);
	}

	return (image);
}

static int file_image_release(GuiWidget *self)
{
	GuiFileImage *image = NULL;
	image_desc *img_desc = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);

	if(NULL == image)
	{
		return (1);
	}

	if(FILE_IMG_SINGLE == image->mode)
	{
		if(image->p.single_img != NULL)
		{
			img_desc = image->p.single_img;

			if(img_desc && (FILE_TYPE == img_desc->type))
			{
				if(image->surface)
				{
					hd_free_surface(image->surface);
					image->surface = NULL;
				}
				gal_img_release(image->p.single_img);
				image->p.single_img = NULL;
			}
		}
	}
	else if(FILE_IMG_DYNAMIC == image->mode)
	{
		if(image->p.single_img)
		{
			if(NULL != image->p.single_img->data)
			{
				if(GIF_TYPE == image->p.single_img->type)
				{
					IMG_GIF_Free((void*)image->p.single_img->data);
				}
				else
				{
					if(image->surface)
					{
						hd_free_surface(image->surface);
						image->surface = NULL;
					}
					//destroy the new surface
				}
			}
			gal_img_release(image->p.single_img);
			image->p.single_img = NULL;
		}
	}

	if(image->snap_surface)
	{
		hd_free_surface(image->snap_surface);
		image->snap_surface = NULL;
	}

	if(image->timer)
	{
		remove_timer(image->timer);
		image->timer = NULL;
	}

	if(image->destroy_event)
	{
		exec_widget_event_data(self, image->destroy_event, NULL);
	}

	GUI_FREE(self->object);

	return (0);
}

static unsigned int _get_bpprgb(int rgb_value, int alpha)
{
	GuiCore *gui_core = NULL;
	char r = 0, g = 0, b = 0;
	unsigned int bpp_rgb = 0;

	gui_core = GUI_GetCurrent();
	if(gui_core->config.bpp == 32)
	{
		bpp_rgb = (unsigned int)((alpha << 24) | rgb_value);
	}
	else if(gui_core->config.bpp == 16)
	{
		r = (rgb_value & 0x00FF0000) >> 16;
		g = (rgb_value & 0x0000FF00) >> 8;
		b = (rgb_value & 0x000000FF);
		bpp_rgb = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
	}


	return (bpp_rgb);
}
static void _force_commit(void)
{
	GAL_Rect rect = {0};
	int ret = 0;

	/*调此接口往往是需要即刻见到，故在flip后加此操作*/
	if((GUI_NO_VIRTUAL_LAYER == gui.layer_mode) && (gui.config.enable_double_buffer))
	{
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		ret = gal_copy_surface(screen, &rect, back_screen, &rect);
		if(ret)
		{
			gxlogd("[GUI] Flush failed!\n");
		}
	}
}

static int _draw_file_gif(GuiFileImage *image, GxAluMode mode)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *self = NULL;
	image_desc *gif_desc = NULL;
	GIF_Para *gif_header = NULL;
	GIF_Image *gif_node = NULL;
	int ret = 0;
	unsigned char *gif_MemBuf = NULL;

	int width = 0, height = 0;
	int i = 0, j = 0, rgb_value = 0;
	unsigned int index = 0;
	unsigned int bpp_rgb = 0;
	unsigned int *gif_color = NULL;
	char *data = NULL;
	Color *clut_ptr = NULL, *global_clut = NULL;

	//GxVpuProperty_CreateSurface Gif_CreateSurface = {0};
	GAL_Rect gif_rect = {0};
	GuiSurface *gif_surface = NULL;
	GxVpuProperty_Blit Blit;
	GxVpuProperty_EndUpdate EndUpdate = {0};
	GxVpuProperty_BeginUpdate BeginUpdate = {0};

	unsigned int src_top = 0, src_left = 0, src_w = 0, src_h = 0;
	unsigned int dst_top = 0, dst_left = 0, dst_w = 0, dst_h = 0;
	bool b_need_fill_bg = false;
	int color = 0, alpha = 0;

	if(NULL ==image)
	{
		return (1);
	}

	self = image->base;
	if(NULL == self)
	{
		return (1);
	}

	gif_desc = image->p.single_img;
	if(NULL == gif_desc)
	{
		return (1);
	}

	if(GIF_TYPE != image->p.single_img->type)
	{
		return (1);
	}

	gif_header = (GIF_Para *)(gif_desc->data);
	if(NULL == gif_header)
	{
		return (1);
	}

	/*Draw current image*/
	if(NULL == gif_header->cur_image)
	{
		gif_node = (GIF_Image *)(gif_header->each_image);
		if(NULL == gif_node)
		{
			return (1);
		}

		gif_header->cur_image = gif_node;
	}
	else
	{
		gif_node = gif_header->cur_image->next;
		if(NULL == gif_node)
		{
			gif_node = (GIF_Image *)(gif_header->each_image);
			gif_header->cur_image = gif_node;
		}

		gif_header->cur_image = gif_node;
	}

	//begin update
	BeginUpdate.max_job_num = 10;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_BeginUpdate, (void*)&BeginUpdate,
			sizeof(GxVpuProperty_BeginUpdate));
	if (GXCORE_SUCCESS != ret)
	{
		gxlogd("[GUI file image] show GxVpuPropertyID_BeginUpdate error\n");
		return 0;
	}


	//---------------------------------------------
	//Create New surface
	gif_rect.x = gif_rect.y = 0;
	gif_rect.w = gif_node->img_width;
	gif_rect.h = gif_node->img_height;

	gif_surface = hd_get_surface(&gif_rect, gui_core->config.bpp, NULL, GUI_TRUE);
	if(NULL == gif_surface)
	{
		return (0);
	}

	gif_MemBuf = gif_surface->data;//fb_getmmap(&gui.fb, (uint32_t)gif_surface->data);

	//draw the gif
	height = gif_node->img_height;
	width = gif_node->img_width;

	clut_ptr = gif_node->img_pal;
	if(NULL == clut_ptr)
	{
		global_clut = gif_header->pal;
		clut_ptr = global_clut;
	}

	alpha = (mode == GX_ALU_ROP_COPY_INVERT ? 0 : 0xFF);
	data = (char *)(gif_node->img_data);
	if(data)
	{
		for(i = 0; i < height; i++)
		{
			for(j = 0; j < width; j++)
			{
				index = data[i * width + j];
				rgb_value = clut_ptr[index];

				if(gif_node->enable_trans)
				{
					if(gif_node->color_key_index != index)
					{
						bpp_rgb = _get_bpprgb(rgb_value, alpha);
					}
					else
					{
						rgb_value = gal_get_gui_transparent();
						bpp_rgb = _get_bpprgb(rgb_value, 0);
					}
					if(gui_core->config.bpp == 32)
					{
						gif_color = (unsigned int *)(&gif_MemBuf[(i * width + j) * 4]);
						*gif_color = bpp_rgb;
					}
					else
					{
						gif_MemBuf[(i * width + j) * 2] = bpp_rgb & 0x00FF;
						gif_MemBuf[(i * width + j) * 2 + 1] = (bpp_rgb >> 8) & 0x00FF;
					}
				}
				else
				{
					bpp_rgb = _get_bpprgb(rgb_value, alpha);
					if(gui_core->config.bpp == 32)
					{
						gif_color = (unsigned int *)(&gif_MemBuf[(i * width + j) * 4]);
						*gif_color = bpp_rgb;
					}
					else
					{
						gif_MemBuf[(i * width + j) * 2] = bpp_rgb & 0x00FF;
						gif_MemBuf[(i * width + j) * 2 + 1] = (bpp_rgb >> 8) & 0x00FF;
					}
				}
			}
		}
	}

	//---------------------------------------------
	int primitive_width = 0, primitive_height = 0;

	primitive_width = gif_desc->width;
	primitive_height = gif_desc->height;

	gif_desc->width = gif_surface->sf.width;
	gif_desc->height = gif_surface->sf.height;

	if(self->rect.w != gif_desc->width ||
			self->rect.h != gif_desc->height)
	{
		if(gif_desc->width <= self->rect.w)
		{
			if(!gif_node->enable_trans)
			{
				b_need_fill_bg = true;
			}

			src_left = 0;
			src_w = gif_desc->width;
			if(gif_node->img_left)
			{
				dst_left = self->rect.x + gif_node->img_left;
			}
			else
			{
				dst_left = self->rect.x;// + (self->rect.w - image->p.single_img->width) / 2;
			}
			dst_w = src_w;
		}
		else
		{
			src_left = (gif_desc->width - self->rect.w) / 2;
			src_w = self->rect.w;
			dst_left = self->rect.x;
			dst_w = src_w;
		}

		if(gif_desc->height <= self->rect.h)
		{
			if(!gif_node->enable_trans)
			{
				b_need_fill_bg = true;
			}

			src_top = 0;
			src_h = gif_desc->height;
			if(gif_node->img_top)
			{
				dst_top = self->rect.y + gif_node->img_top;
			}
			else
			{
				dst_top = self->rect.y;// + (self->rect.h - image->p.single_img->height) / 2;
			}
			dst_h = src_h;
		}
		else
		{
			src_top = (gif_desc->height - self->rect.h) / 2;
			src_h = self->rect.h;
			dst_top = self->rect.y;
			dst_h = src_h;
		}
	}
	else
	{
		src_left = 0;
		src_top = 0;
		src_w = gif_desc->width;
		src_h = gif_desc->height;

		dst_left = self->rect.x;
		dst_top = self->rect.y;
		dst_w = src_w;
		dst_h = src_h;
	}
	gif_desc->width = primitive_width;
	gif_desc->height = primitive_height;

	if(b_need_fill_bg)
	{
		color = self->back_color[0];
		if(color != gui_core->config.gui_trans)
		{
			color = gal_color2index(screen, color);
			hd_fillrect(screen, (GAL_Rect *)&(self->rect), color);
		}
	}

	hd_check_surface(gif_surface);
	//Blit to main surface
	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));
	Blit.srca.cct = NULL;
	Blit.mode = mode;//GX_ALU_ROP_COPY_INVERT;//GX_ALU_ROP_COPY_INVERT;//GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.dst.alpha = gui_core->config.osd_alpha;
	Blit.srca.surface = gif_surface->hw_surface;
	Blit.srca.dst_format = get_color_format(0, gui.config.bpp);;
	Blit.srca.is_ksurface = GUI_TRUE;
	Blit.srca.rect.x = src_left;
	Blit.srca.rect.y = src_top;
	Blit.srca.rect.width = src_w;
	Blit.srca.rect.height = src_h;
	Blit.srcb.surface = screen->hw_surface;
	Blit.srcb.dst_format = get_color_format(0, gui_core->config.bpp);//GX_COLOR_FMT_RGB565;
	Blit.srcb.is_ksurface = GUI_TRUE;
	Blit.srcb.rect.x = dst_left;
	Blit.srcb.rect.y = dst_top;
	Blit.srcb.rect.width = dst_w;
	Blit.srcb.rect.height = dst_h;
	Blit.dst = Blit.srcb;
	if(gif_node->enable_trans)
	{
		rgb_value = gal_get_gui_transparent();
		bpp_rgb = _get_bpprgb(rgb_value, 0);
		Blit.colorkey_info.src_colorkey_en = GUI_TRUE;
		Blit.colorkey_info.src_colorkey = bpp_rgb;
	}
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));

	if (0 != ret)
	{
		gxlogd("[GUI_IMG] Blit gif surface error!!!\n");
	}

	//flush
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_EndUpdate, (void*)&EndUpdate,
			sizeof(GxVpuProperty_EndUpdate));
	if (GXCORE_SUCCESS != ret)
	{
		gxlogd("[GUI_IMG] EndUpdate gif surface error!!!\n");
	}

	//wm_copy_main_surface();
	_force_commit();

	//destroy the new surface
	hd_free_surface(gif_surface);

	return (0);
}

static int _draw_img(GuiFileImage *image)
{
	GuiCore *gui_core = NULL;
	GUI_Rect valid_rect = {0};
	GuiWidget *self = NULL;
	int color = 0, index = 0;

	if(NULL == image)
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	valid_rect = self->rect;

	if(NULL == image->p.single_img)
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		if(gui_core->config.gui_trans != color)
			GXGDI_FillRect(screen, (GXGDI_Rect *)&(self->rect), color);
	}
	else
	{
		if(GIF_TYPE == image->p.single_img->type)
		{
			//_draw_gif(image);
			index = widget_get_state_index(self);
			color = self->back_color[index];
			if(gui_core->config.gui_trans != color)
				GXGDI_FillRect(screen, (GXGDI_Rect *)&(self->rect), color);
		}
		else
		{
			valid_rect.w = image->p.single_img->width;
			valid_rect.h = image->p.single_img->height;
			//WIDGET_CHECK_RECT(valid_rect, self->rect);
			if(self->rect.w > valid_rect.w || self->rect.h > valid_rect.h)
			{
				index = widget_get_state_index(self);
				color = self->back_color[index];
				if(gui_core->config.gui_trans != color)
					GXGDI_FillRect(screen, (GXGDI_Rect *)&(self->rect), color);
			}
			GXGDI_DrawImage(screen, (void *)(image->p.single_img), self->rect.x, self->rect.y);
		}
	}
	return (0);

}

static int _draw_dynamic(GuiFileImage *image)
{
	//	GUI_Rect valid_rect = {0};
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL;
	int color = 0, index = 0;

#if 0
	unsigned int top = 0, left = 0;
#else
	unsigned int src_top = 0, src_left = 0, src_w = 0, src_h = 0;
	unsigned int dst_top = 0, dst_left = 0, dst_w = 0, dst_h = 0;
	bool b_need_fill_bg = false;
#endif

	GxStatus ret;
	GxVpuProperty_Blit Blit;

	if(NULL == image)
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	//	valid_rect = self->rect;

	if(NULL == image->p.single_img && NULL == image->surface)
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		if(image->opacity != 0xFF)
		{
			color |= (((image->opacity) << 16) & 0xFF0000);
		}
		gal_fillrect(screen, (GAL_Rect *)&(self->rect), color);
	}
	else
	{
		if((image->p.single_img) && (GIF_TYPE == image->p.single_img->type))
		{
			_draw_file_gif(image, (GxAluMode)(int)image->alu_mode);
		}
		else
		{
			if((NULL == image->surface) && (image->p.single_img))
			{
				image->surface = image_desc_surface(image->p.single_img);
			}

			if(self->rect.w != image->surface->sf.width ||
					self->rect.h != image->surface->sf.height)
			{
				if(image->surface->sf.width <= self->rect.w)
				{
					b_need_fill_bg = true;

					src_left = 0;
					src_w = image->surface->sf.width;
					dst_left = self->rect.x + (self->rect.w - image->surface->sf.width) / 2;
					dst_w = src_w;
				}
				else
				{
					src_left = (image->surface->sf.width - self->rect.w) / 2;
					src_w = self->rect.w;
					dst_left = self->rect.x;
					dst_w = src_w;
				}

				if(image->surface->sf.height <= self->rect.h)
				{
					b_need_fill_bg = true;

					src_top = 0;
					src_h = image->surface->sf.height;
					dst_top = self->rect.y + (self->rect.h - image->p.single_img->height) / 2;
					dst_h = src_h;
				}
				else
				{
					src_top = (image->surface->sf.height - self->rect.h) / 2;
					src_h = self->rect.h;
					dst_top = self->rect.y;
					dst_h = src_h;
				}
			}
			else
			{
				src_left = 0;
				src_top = 0;
				src_w = image->surface->sf.width;
				src_h = image->surface->sf.height;

				dst_left = self->rect.x;
				dst_top = self->rect.y;
				dst_w = src_w;
				dst_h = src_h;
			}

			if(b_need_fill_bg)
			{
				color = self->back_color[0];
				if(color != gui_core->config.gui_trans)
				{
					color = gal_color2index(screen, color);
					hd_fillrect(screen, (GAL_Rect *)&(self->rect), color);
				}
			}

			if((image->surface) &&
			   (image->p.single_img) &&
			   (24 == image->p.single_img->bpp)) {
				gal_free_surface(image->surface);
				image->surface = NULL;
				gal_img_desc(screen, image->p.single_img, dst_left, dst_top, dst_w, dst_h);
			} else {
				memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

				Blit.srca.cct = 0;
				Blit.mode = GX_ALU_ROP_COPY;
				Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
				Blit.dst.alpha = gui_core->config.osd_alpha;
				color = gui_core->config.gui_trans;
				color = ((((color >> 16) & 0x0000FF) >> 3)<<11) |
					((((color >> 8) & 0x0000FF) >>2)<<5) |
					((color & 0x0000FF)>>3);
				Blit.colorkey_info.src_colorkey_en = GUI_TRUE;
				Blit.colorkey_info.src_colorkey = color;

				Blit.srca.surface = image->surface->hw_surface;
				Blit.srca.dst_format = image->surface->sf.color_format;
				Blit.srca.is_ksurface = is_ksurface(image->surface);
				Blit.srca.rect.x = src_left;
				Blit.srca.rect.y = src_top;
				Blit.srca.rect.width = src_w;//image->p.single_img->width;
				Blit.srca.rect.height = src_h;//image->p.single_img->height;
				Blit.srcb.surface = screen->hw_surface;//screen;//GDI_GetMainOSDSurface();
				Blit.srcb.dst_format = screen->sf.color_format;
				Blit.srcb.is_ksurface = is_ksurface(screen);
				Blit.srcb.rect.x = dst_left;
				Blit.srcb.rect.y = dst_top;
				Blit.srcb.rect.width = dst_w;//image->p.single_img->width;
				Blit.srcb.rect.height = dst_h;//image->p.single_img->height;
				Blit.dst = Blit.srcb;
				if(0xFF != image->opacity)
				{
					Blit.mode = GX_ALU_MIX_NONE;
					Blit.srca.alpha_en = GUI_TRUE;
					Blit.srca.alpha = image->opacity;
				}

				if(gui_core->config.bpp == 32)
				{
					int trans_color = 0;

					trans_color = gal_get_gui_transparent();
					trans_color = hd_color2index(screen, trans_color);
					Blit.colorkey_info.src_colorkey_en = GUI_TRUE;
					Blit.colorkey_info.src_colorkey = trans_color;
				}

				if(image->surface->bpp == 32)
				{
					Blit.colorkey_info.src_colorkey_en = GUI_FALSE;
					Blit.colorkey_info.dst_colorkey_en = GUI_TRUE;
					Blit.colorkey_info.dst_colorkey = gui_core->config.osd_trans;
				}

				ret = GxAVSetProperty(g_device_handle,
						vpu_handle,
						GxVpuPropertyID_Blit,
						&Blit,
						sizeof(GxVpuProperty_Blit));

				if (0 != ret)
				{
					return 0;
				}

				if(image->surface->bpp == 32)
				{
					hd_add_blit_element(NULL);
					hd_commit_blit();
					memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

					Blit.srca.surface     = image->surface->hw_surface;
					Blit.srca.is_ksurface = is_ksurface(image->surface);
					Blit.srca.rect.x      = src_left;
					Blit.srca.rect.y      = src_top;
					Blit.srca.rect.width  = src_w;       //((int)(r_width / 4)) * 4;
					Blit.srca.rect.height = src_h;      //((int)(r_height / 4)) * 4;

					Blit.srcb.surface     = screen->hw_surface;
					Blit.srcb.is_ksurface = is_ksurface(screen);
					Blit.srcb.rect        = Blit.srca.rect;
					Blit.srcb.rect.x      = dst_left;
					Blit.srcb.rect.y      = dst_top;
					Blit.srca.rect.width  = dst_w;       //((int)(r_width / 4)) * 4;
					Blit.srca.rect.height = dst_h;      //((int)(r_height / 4)) * 4;

					Blit.dst       = Blit.srcb;
					Blit.dst.alpha = gui_core->config.osd_alpha;
					Blit.mode = GX_ALU_MIX_NONE;
					ret = GxAVSetProperty(g_device_handle, vpu_handle, GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));
					if (0 != ret)
					{
						return 0;
					}
				}
				else
				{
					hd_add_blit_element(NULL);
				}
				gdi_commit();
				if(image->surface)
				{
					hd_free_surface(image->surface);
					image->surface = NULL;
				}
			}
		}
	}
	return (0);

}

static int _draw_rect(GUI_Rect *rect, GuiFileImage *image, int color)
{
	GuiCore *gui_core = NULL;
	int h0 = 0, h1 = 0, h2 = 0, h3 = 0;
	int w0 = 0, w1 = 0, w2 = 0, w3 = 0;
	int offset0 = 0, offset1 = 0, offset2 = 0, offset3 = 0;
	GAL_Rect valid_rect = {0};

	if(NULL == rect || NULL == image)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	if(image->p.com_img.image_lt)
	{
		w0 = image->p.com_img.image_lt->width;
		h0 = image->p.com_img.image_lt->height;
	}

	if(image->p.com_img.image_rt)
	{
		w1 = image->p.com_img.image_rt->width;
		h1 = image->p.com_img.image_rt->height;
	}

	if(image->p.com_img.image_lb)
	{
		w2 = image->p.com_img.image_lb->width;
		h2 = image->p.com_img.image_lb->height;
	}

	if(image->p.com_img.image_rb)
	{
		w3 = image->p.com_img.image_rb->width;
		h3 = image->p.com_img.image_rb->height;
	}

	if(image->p.com_img.image_t)
	{
		offset0 = image->p.com_img.image_t->height;
	}

	if(image->p.com_img.image_b)
	{
		offset1 = image->p.com_img.image_b->height;
	}

	if(image->p.com_img.image_l)
	{
		offset2 = image->p.com_img.image_l->width;
	}

	if(image->p.com_img.image_r)
	{
		offset3 = image->p.com_img.image_r->width;
	}

	valid_rect.x = rect->x + w0;
	valid_rect.y = rect->y + offset0;
	valid_rect.w = rect->w - w0 -w1;
	if(h0 > offset0)
	{
		valid_rect.h = h0 - offset0;
	}
	else
	{
		valid_rect.h = 0;
	}
	if(gui_core->config.gui_trans != color)
		GXGDI_FillRect(screen, (GXGDI_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + offset2;
	valid_rect.y = rect->y + h0;
	if(w0 > offset2)
	{
		valid_rect.h = w0 - offset2;
	}
	else
	{
		valid_rect.h = 0;
	}
	valid_rect.w = rect->w - offset2;

	valid_rect.h = rect->h - h0 - h2;
	if(gui_core->config.gui_trans != color)
		GXGDI_FillRect(screen, (GXGDI_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + w2;
	valid_rect.y = rect->y + rect->h - h2;
	valid_rect.w = rect->w - w2 - w3;
	if(h3 > offset1)
	{
		valid_rect.h = h3 - offset1;
	}
	else
	{
		valid_rect.h = 0;
	}
	if(gui_core->config.gui_trans != color)
		GXGDI_FillRect(screen, (GXGDI_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + rect->w - w1;
	valid_rect.y = rect->y + h2;
	if(w1 > offset3)
	{
		valid_rect.w = w1 - offset3;
	}
	else
	{
		valid_rect.w = 0;
	}
	valid_rect.h = rect->h - h1 - h3;
	if(gui_core->config.gui_trans != color)
		GXGDI_FillRect(screen, (GXGDI_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + w0;
	valid_rect.y = rect->y + h0;
	valid_rect.w = rect->w - w0 -w1;
	valid_rect.h = rect->h - h0 - h2;
	if(gui_core->config.gui_trans != color)
		GXGDI_FillRect(screen, (GXGDI_Rect *)&valid_rect, color);


	return (0);
}

static int _buffer_rect(char *buffer, GUI_Rect *full_rect, GUI_Rect *rect, int color)
{
	GuiCore *gui_core = NULL;
	int width = 0, i = 0/*, j = 0*/;
	char *temp = NULL;
	//unsigned short *us_temp = NULL;

	if(NULL == buffer || NULL == rect || NULL == full_rect)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	width = full_rect->w * gui_core->config.bpp / 8;

	temp = buffer + rect->y * width + rect->x;

	if(8 == gui_core->config.bpp)
	{
		for(i = 0; i < rect->h; i++)
		{
			memset(temp + i * width, color,
					rect->w * gui_core->config.bpp / 8);
		}
	}
	else if(16 == gui_core->config.bpp)
	{
		/*us_temp = (unsigned short *)(temp);
		  for(i = 0; i < rect->h; i++)
		  {
		  for(j = 0; j < rect->w; j++)
		  {
		  us_temp[i * rect->w + j] = (unsigned short)color;
		  }
		  }
		  wmemset((wchar_t *)temp + i * width,
		  (unsigned short)color,
		  rect->w)*/;
	}

	return (0);
}

static int _buffer_image(char *buffer, const image_desc * pDesc, int x, int y, int width, int height)
{
	char *temp = NULL, *data = NULL;
	int image_width = 0, image_height = 0, i = 0;

	if(NULL == buffer || NULL == pDesc)
	{
		return (1);
	}

	temp = buffer + width * y +x;
	image_width = pDesc->width;
	image_height = pDesc->height;
	data = pDesc->data;

	for(i = 0; i < image_height; i++)
	{
		memcpy(temp + i * width, data, image_width);
		data += image_width;
	}

	return (0);
}

static int _draw_buffer_rect(char *buffer, GUI_Rect *rect, GuiFileImage *image, int color)
{
	int h0 = 0, h1 = 0, h2 = 0, h3 = 0;
	int w0 = 0, w1 = 0, w2 = 0, w3 = 0;
	int offset0 = 0, offset1 = 0, offset2 = 0, offset3 = 0;
	GUI_Rect valid_rect = {0};

	if(NULL == rect || NULL == image || NULL == buffer)
	{
		return (1);
	}

	if(image->p.com_img.image_lt)
	{
		w0 = image->p.com_img.image_lt->width;
		h0 = image->p.com_img.image_lt->height;
	}

	if(image->p.com_img.image_rt)
	{
		w1 = image->p.com_img.image_rt->width;
		h1 = image->p.com_img.image_rt->height;
	}

	if(image->p.com_img.image_lb)
	{
		w2 = image->p.com_img.image_lb->width;
		h2 = image->p.com_img.image_lb->height;
	}

	if(image->p.com_img.image_rb)
	{
		w3 = image->p.com_img.image_rb->width;
		h3 = image->p.com_img.image_rb->height;
	}

	if(image->p.com_img.image_t)
	{
		offset0 = image->p.com_img.image_t->height;
	}

	if(image->p.com_img.image_b)
	{
		offset1 = image->p.com_img.image_b->height;
	}

	if(image->p.com_img.image_l)
	{
		offset2 = image->p.com_img.image_l->width;
	}

	if(image->p.com_img.image_r)
	{
		offset3 = image->p.com_img.image_r->width;
	}

	valid_rect.x = rect->x + w0;
	valid_rect.y = rect->y + offset0;
	valid_rect.w = rect->w - w0 -w1;
	if(h0 > offset0)
	{
		valid_rect.h = h0 - offset0;
	}
	else
	{
		valid_rect.h = 0;
	}
	_buffer_rect(buffer, rect, &valid_rect, color);

	valid_rect.x = rect->x + offset2;
	valid_rect.y = rect->y + h0;
	if(w0 > offset2)
	{
		valid_rect.h = w0 - offset2;
	}
	else
	{
		valid_rect.h = 0;
	}
	valid_rect.w = rect->w - offset2;

	valid_rect.h = rect->h - h0 - h2;
	_buffer_rect(buffer, rect, &valid_rect, color);

	valid_rect.x = rect->x + w2;
	valid_rect.y = rect->y + rect->h - h2;
	valid_rect.w = rect->w - w2 - w3;
	if(h3 > offset1)
	{
		valid_rect.h = h3 - offset1;
	}
	else
	{
		valid_rect.h = 0;
	}
	_buffer_rect(buffer, rect, &valid_rect, color);

	valid_rect.x = rect->x + rect->w - w1;
	valid_rect.y = rect->y + h2;
	if(w1 > offset3)
	{
		valid_rect.w = w1 - offset3;
	}
	else
	{
		valid_rect.w = 0;
	}
	valid_rect.h = rect->h - h1 - h3;
	_buffer_rect(buffer, rect, &valid_rect, color);

	valid_rect.x = rect->x + w0;
	valid_rect.y = rect->y + h0;
	valid_rect.w = rect->w - w0 -w1;
	valid_rect.h = rect->h - h0 - h2;
	_buffer_rect(buffer, rect, &valid_rect, color);


	return (0);
}

static int _draw_multi_img(GuiFileImage *image)
{
	GUI_Rect valid_rect = {0};
	GuiWidget *self = NULL;
	int color = 0, index = 0;
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0, offset = 0;

	if(NULL == image)
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	valid_rect = self->rect;

	index = widget_get_state_index(self);
	color = self->back_color[index];

	_draw_rect(&valid_rect, image, color);

	x0 = valid_rect.x;
	x1 = valid_rect.x + valid_rect.w;

	if(image->p.com_img.image_lt)
	{
		GXGDI_DrawImage(screen, (void *)image->p.com_img.image_lt, valid_rect.x, valid_rect.y);
		x0 = valid_rect.x +image->p.com_img.image_lt->width;
		y0 = valid_rect.y + image->p.com_img.image_lt->height;
	}
	else
	{
		x0 = valid_rect.x;
		y0 = valid_rect.y;
	}

	if(image->p.com_img.image_rt)
	{
		x1 = valid_rect.x + valid_rect.w - image->p.com_img.image_rt->width;
		GXGDI_DrawImage(screen, (void *)image->p.com_img.image_rt, x1, valid_rect.y);
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
	}

	if( image->p.com_img.image_t)
	{
		while(x0 < x1)
		{
			if((x1 - x0) < image->p.com_img.image_t->width)
			{
				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_t,
						x1 - image->p.com_img.image_t->width,
						valid_rect.y);
			}
			else
			{
				GXGDI_DrawImage(screen, (void *)image->p.com_img.image_t, x0, valid_rect.y);
			}
			x0 += image->p.com_img.image_t->width;
		}
	}

	if(image->p.com_img.image_lb)
	{
		x0 = valid_rect.x + image->p.com_img.image_lb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_lb->height;
		GXGDI_DrawImage(screen, (void *)image->p.com_img.image_lb, valid_rect.x, y1);
	}
	else
	{
		x0 = valid_rect.x;
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_l)
	{
		while(y0 < y1)
		{
			if((y1 - y0) < image->p.com_img.image_l->height)
			{
				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_l,
						valid_rect.x,
						y0 - (image->p.com_img.image_l->height - (y1 - y0)));
			}
			else
			{
				GXGDI_DrawImage(screen, (void *)image->p.com_img.image_l, valid_rect.x, y0);
			}
			y0 += image->p.com_img.image_l->height;
		}
	}

	if(image->p.com_img.image_rb)
	{
		x1 = valid_rect.x + valid_rect.w - image->p.com_img.image_rb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_rb->height;
		GXGDI_DrawImage(screen, (void *)image->p.com_img.image_rb, x1, y1);
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_b)
	{
		while(x0 < x1)
		{
			if((x1 - x0) < image->p.com_img.image_b->width)
			{
				offset = x1;
				if(image->p.com_img.image_r)
				{
					offset -= (image->p.com_img.image_r->width >
							image->p.com_img.image_b->width) ?
						(image->p.com_img.image_r->width) :
						(image->p.com_img.image_b->width);
				}
				else
				{
					offset -= image->p.com_img.image_b->width;
				}

				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_b,
						offset,
						valid_rect.h + valid_rect.y - image->p.com_img.image_b->height);
			}
			else
			{
				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_b,
						x0,
						valid_rect.h + valid_rect.y - image->p.com_img.image_b->height);
			}
			x0 += image->p.com_img.image_b->width;
		}

		GXGDI_DrawImage(screen,
				(void *)image->p.com_img.image_b,
				x1 - image->p.com_img.image_b->width,
				valid_rect.h + valid_rect.y - image->p.com_img.image_b->height);
	}

	if(image->p.com_img.image_rt)
	{
		y0 = valid_rect.y + image->p.com_img.image_rt->height;
	}
	else
	{
		y0 = valid_rect.y;
	}

	if(image->p.com_img.image_rb)
	{
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_rb->height;
	}
	else
	{
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_r)
	{
		while(y0 < y1)
		{
			x1 = valid_rect.x + valid_rect.w;
			if((y1 - y0) < image->p.com_img.image_r->height)
			{
				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_r,
						x1 - image->p.com_img.image_r->width,
						y0 - (image->p.com_img.image_r->height - (y1 - y0)));
			}
			else
			{
				GXGDI_DrawImage(screen,
						(void *)image->p.com_img.image_r,
						x1 - image->p.com_img.image_r->width,
						y0);
			}
			y0 += image->p.com_img.image_r->height;
		}
	}
	return (0);
}

static int file_image_draw(GuiWidget *self)
{
	GuiFileImage *image = NULL;
	const char *value = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);

	if(NULL == image)
	{
		return (1);
	}

	// TODO:
	/*Adjust the opacity of the OSD*/

	if(image->snap_surface)
	{
		value = widget_get_property(self, "snap");
		if(value)
		{
			sscanf((const char *)value, "[%d,%d,%d,%d]",
					&dst_rect.x, &dst_rect.y, &dst_rect.w, &dst_rect.h);
			src_rect = dst_rect;
			src_rect.x = src_rect.y = 0;
			gal_copy_surface(image->snap_surface, &src_rect, screen, &dst_rect);
		}
	}

	switch(image->mode)
	{
	case FILE_IMG_SINGLE:
		_draw_img(image);
		break;
	case FILE_IMG_MULTIPLE:
		_draw_multi_img(image);
		break;
	case FILE_IMG_DYNAMIC:
		_draw_dynamic(image);
	default:
		break;
	}

	return (0);
}

static int file_image_event(GuiWidget *self, void *data)
{
	return (0);
}

static int _file_image_update(GuiFileImage *image, GUI_Rect *rect)
{
	GuiCore *gui_core = NULL;
	GUI_Rect valid_rect = {0};
	GuiWidget *self = NULL;
	int color = 0, index = 0;
	int width = 0, xpos = 0, ypos = 0, i = 0, pal_index = 0;
	char *buffer = NULL, *image_data = NULL, *temp = NULL;
	image_desc img_desc = {0};

	if(NULL == image || NULL == rect)
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	valid_rect = self->rect;
	valid_rect.x = 0;
	valid_rect.y = 0;

	buffer = (char *)GUI_MALLOC(valid_rect.w * valid_rect.h * gui_core->config.bpp / 8);
	if(NULL == buffer)
	{
		return (1);
	}

	memset(buffer, 0, valid_rect.w * valid_rect.h * gui_core->config.bpp / 8);

	if((NULL == image->p.single_img) && (NULL == image->surface))
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		_buffer_rect(buffer, &valid_rect, &valid_rect, color);
	}
	else
	{
		valid_rect.w = image->p.single_img->width;
		valid_rect.h = image->p.single_img->height;

		if(self->rect.w > valid_rect.w || self->rect.h > valid_rect.h)
		{
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			_buffer_rect(buffer, &valid_rect, &valid_rect, color);
		}
		_buffer_image(buffer,
				image->p.single_img,
				valid_rect.x,
				valid_rect.y,
				valid_rect.w,
				valid_rect.h);

		pal_index = image->p.single_img->pal_index;
	}

	xpos = rect->x - self->rect.x;
	ypos = rect->y - self->rect.y;
	width = (((rect->w * gui_core->config.bpp) + 31) >> 5) << 2;

	image_data = (char *)GUI_MALLOC(width * rect->h);
	if(NULL == image_data)
	{
		return (1);
	}

	memset(image_data, 0, width * rect->h);

	temp = buffer + ypos * valid_rect.w + xpos;

	for(i = 0; i < rect->h; i++)
	{
		memcpy(image_data + i * rect->w * gui_core->config.bpp / 8,
				temp + i * valid_rect.w * gui_core->config.bpp / 8,
				rect->w * gui_core->config.bpp / 8);
	}

	img_desc.bpp = gui_core->config.bpp;
	img_desc.type = BMP_TYPE;
	img_desc.data = image_data;
	img_desc.width = rect->w;
	img_desc.height = rect->h;
	img_desc.pal_index = pal_index;

	GXGDI_DrawImage(screen, (void *)&img_desc, rect->x, rect->y);

	GUI_FREE(buffer);
	GUI_FREE(image_data);

	return (0);

}

static int _multiimage_updata_rect(GuiWidget *self, GUI_Rect *rect)
{
	GuiCore *gui_core = NULL;
	GUI_Rect valid_rect = {0};
	int color = 0, index = 0;
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0, offset = 0;
	int width = 0, xpos = 0, ypos = 0, i = 0;
	char *buffer = NULL, *image_data = NULL, *temp = NULL;
	GuiFileImage *image = NULL;
	image_desc img_desc = {0};

	if((NULL == self) || (NULL == self))
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	valid_rect = self->rect;
	valid_rect.x = 0;
	valid_rect.y = 0;
	//width = (((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2;

	buffer = (char *)GUI_MALLOC(valid_rect.w * valid_rect.h * gui_core->config.bpp / 8);
	if(NULL == buffer)
	{
		return (1);
	}

	memset(buffer, 0, valid_rect.w * valid_rect.h * gui_core->config.bpp / 8);

	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(screen, color);

	_draw_buffer_rect(buffer, &valid_rect, image, color);

	x0 = valid_rect.x;
	x1 = valid_rect.x + valid_rect.w;

	if(image->p.com_img.image_lt)
	{
		_buffer_image(buffer,
				image->p.com_img.image_lt,
				valid_rect.x,
				valid_rect.y,
				valid_rect.w,
				valid_rect.h);
		x0 = valid_rect.x +image->p.com_img.image_lt->width;
		y0 = valid_rect.y + image->p.com_img.image_lt->height;
	}
	else
	{
		x0 = valid_rect.x;
		y0 = valid_rect.y;
	}

	if(image->p.com_img.image_rt)
	{
		x1 = valid_rect.x + valid_rect.w - image->p.com_img.image_rt->width;
		_buffer_image(buffer,
				image->p.com_img.image_rt,
				x1,
				valid_rect.y,
				valid_rect.w,
				valid_rect.h);
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
	}

	if( image->p.com_img.image_t)
	{
		while(x0 < x1)
		{
			if((x1 - x0) < image->p.com_img.image_t->width)
			{
				_buffer_image(buffer,
						image->p.com_img.image_t,
						x1 - image->p.com_img.image_t->width,
						valid_rect.y,
						valid_rect.w,
						valid_rect.h);
			}
			else
			{
				_buffer_image(buffer,
						image->p.com_img.image_t,
						x0,
						valid_rect.y,
						valid_rect.w,
						valid_rect.h);
			}
			x0 += image->p.com_img.image_t->width;
		}
	}

	if(image->p.com_img.image_lb)
	{
		x0 = valid_rect.x + image->p.com_img.image_lb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_lb->height;
		_buffer_image(buffer,
				image->p.com_img.image_lb,
				valid_rect.x,
				y1,
				valid_rect.w,
				valid_rect.h);
	}
	else
	{
		x0 = valid_rect.x;
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_l)
	{
		while(y0 < y1)
		{
			if((y1 - y0) < image->p.com_img.image_l->height)
			{
				_buffer_image(buffer,
						image->p.com_img.image_l,
						valid_rect.x,
						y0 - (image->p.com_img.image_l->height - (y1 - y0)),
						valid_rect.w,
						valid_rect.h);
			}
			else
			{
				_buffer_image(buffer,
						image->p.com_img.image_l,
						valid_rect.x,
						y0,
						valid_rect.w,
						valid_rect.h);
			}
			y0 += image->p.com_img.image_l->height;
		}
	}

	if(image->p.com_img.image_rb)
	{
		x1 = valid_rect.x + valid_rect.w - image->p.com_img.image_rb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_rb->height;
		_buffer_image(buffer,
				image->p.com_img.image_rb,
				x1,
				y1,
				valid_rect.w,
				valid_rect.h);
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_b)
	{
		while(x0 < x1)
		{
			if((x1 - x0) < image->p.com_img.image_b->width)
			{
				offset = x1;
				if(image->p.com_img.image_r)
				{
					offset -= image->p.com_img.image_r->width;
				}
				else
				{
					offset -= image->p.com_img.image_b->width;
				}

				_buffer_image(buffer,
						image->p.com_img.image_b,
						offset,
						valid_rect.h + valid_rect.y - image->p.com_img.image_b->height,
						valid_rect.w,
						valid_rect.h);
			}
			else
			{
				_buffer_image(buffer,
						image->p.com_img.image_b,
						x0,
						valid_rect.h + valid_rect.y - image->p.com_img.image_b->height,
						valid_rect.w,
						valid_rect.h);
			}
			x0 += image->p.com_img.image_b->width;
		}

		_buffer_image(buffer,
				image->p.com_img.image_b,
				x1 - image->p.com_img.image_b->width,
				valid_rect.h + valid_rect.y - image->p.com_img.image_b->height,
				valid_rect.w,
				valid_rect.h);
	}

	if(image->p.com_img.image_rt)
	{
		y0 = valid_rect.y + image->p.com_img.image_rt->height;
	}
	else
	{
		y0 = valid_rect.y;
	}

	if(image->p.com_img.image_rb)
	{
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_rb->height;
	}
	else
	{
		y1 = valid_rect.y + valid_rect.h;
	}

	if( image->p.com_img.image_r)
	{
		while(y0 < y1)
		{
			x1 = valid_rect.x + valid_rect.w;
			if((y1 - y0) < image->p.com_img.image_r->height)
			{
				_buffer_image(buffer,
						image->p.com_img.image_r,
						x1 - image->p.com_img.image_r->width,
						y0 - (image->p.com_img.image_r->height - (y1 - y0)),
						valid_rect.w,
						valid_rect.h);
			}
			else
			{
				_buffer_image(buffer,
						image->p.com_img.image_r,
						x1 - image->p.com_img.image_r->width,
						y0,
						valid_rect.w,
						valid_rect.h);
			}
			y0 += image->p.com_img.image_r->height;
		}
	}

	xpos = rect->x - self->rect.x;
	ypos = rect->y - self->rect.y;
	width = (((rect->w * gui_core->config.bpp) + 31) >> 5) << 2;

	image_data = (char *)GUI_MALLOC(width * rect->h);
	if(NULL == image_data)
	{
		return (1);
	}

	memset(image_data, 0, width * rect->h);

	temp = buffer + ypos * valid_rect.w + xpos;

	for(i = 0; i < rect->h; i++)
	{
		memcpy(image_data + i * rect->w * gui_core->config.bpp / 8,
				temp + i * valid_rect.w * gui_core->config.bpp / 8,
				rect->w * gui_core->config.bpp / 8);
	}

	img_desc.bpp = gui_core->config.bpp;
	img_desc.type = BMP_TYPE;
	img_desc.data = image_data;
	img_desc.width = rect->w;
	img_desc.height = rect->h;
	img_desc.pal_index = gal_get_clut_index();

	GXGDI_DrawImage(screen, (void *)&img_desc, rect->x, rect->y);

	GUI_FREE(buffer);
	GUI_FREE(image_data);

	return (0);
}

static int _image_update_rect(GuiWidget *self, GUI_Rect *rect)
{
	GUI_Rect valid_rect = {0};
	GuiFileImage *image = NULL;

	if((NULL == self) || (NULL == rect))
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	valid_rect = self->rect;

	valid_rect.x = (valid_rect.x > rect->x) ? valid_rect.x : rect->x;
	valid_rect.y = (valid_rect.y > rect->y) ? valid_rect.y : rect->y;
	valid_rect.w = (valid_rect.w < rect->w) ? valid_rect.w : rect->w;
	valid_rect.h = (valid_rect.h < rect->h) ? valid_rect.h : rect->h;

	switch(image->mode)
	{
	case FILE_IMG_SINGLE:
		_file_image_update(image, &valid_rect);
		break;
	case FILE_IMG_MULTIPLE:
		_multiimage_updata_rect(self, &valid_rect);
		break;
	default:
		break;
	}

	return (0);
}

static char *_get_path_id(char *path)
{
	char *pstr = NULL;

	if(NULL == path)
	{
		return (NULL);
	}

	pstr = path + strlen(path) - 1;

	while(pstr > path)
	{
		pstr--;
		if(*pstr == '/')
		{
			pstr++;
			break;
		}
	}

	return (pstr);
}

static int _covert2rgb(image_desc *img)
{
	int ret = 0;

	ret = GDI_ColorSpaceConvert(img);

	return (ret);
}

extern struct image_special_node special_jpeg_zoom;
extern struct image_special_node special_jpeg_cut;
extern struct image_special_node special_png;

static int load_img_to_surface(char *path, GuiFileImage *image, GuiWidget *self, IMG_LOAD_TYPE load_type)
{
	image_desc *imgdesc = NULL;
	GIF_Para * gif_src = NULL;
	GAL_Rect src_rect = {0}, zoom_rect = {0};
	GuiSurface *zoom_surface = NULL;
	Image_Exif image_information = {0};

	unsigned char *data = NULL,*pSrc = NULL, *value = NULL;
	unsigned int i,j, img_width, img_height, per_line_width, per_pixel;
	unsigned char *dst_data;
	int bpp, type;
	int r, g, b, ret = 0;

	FILE *fp = NULL;
	char pic_buf[4];

	if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
		return 2;

	fp = fopen(path, "r");
	if(NULL == fp)
		return -1;

	memset(pic_buf, 0, 4);
	fread(pic_buf, 1, 4, fp);
	fclose(fp);

	if(pic_buf[0] == 0xFF && pic_buf[1] == 0xD8)
	{
		//JPG
		value = (unsigned char *)widget_get_property(self, (const char *)"decode");
		if(IMG_LOAD_TYPE_RESIZE == load_type)
		{
			if((value) && (0 == strcasecmp("hardware", (char *)value)))
			{
				IMG_JPEG_GetInfo(path, &image_information);
				if(image_information.width && image_information.height)
				{
					imgdesc = jpeg_image_descriptor(path,
							imgdesc,
							image_information.width,
							image_information.height,
							self->rect.w,
							self->rect.h);
					if((NULL == imgdesc) && (special_jpeg_zoom.img_zoom_cut))
					{
						imgdesc = special_jpeg_zoom.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
					}
				}
			}
			else
			{
				if(special_jpeg_zoom.img_zoom_cut)
				{
					imgdesc = special_jpeg_zoom.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
				}
			}
		}
		else if(IMG_LOAD_TYPE_ZOOM == load_type)
		{
			if((value) && (0 == strcasecmp("hardware", (char *)value)))
			{
				IMG_JPEG_GetInfo(path, &image_information);
				if(image_information.width && image_information.height)
				{
					imgdesc = jpeg_image_descriptor(path,
							imgdesc,
							image_information.width,
							image_information.height,
							self->rect.w,
							self->rect.h);
					if((NULL == imgdesc) && (special_jpeg_zoom.img_zoom_cut))
					{
						imgdesc = special_jpeg_zoom.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
					}
				}
			}
			else
			{
				if(special_jpeg_zoom.img_zoom_cut)
				{
					imgdesc = special_jpeg_zoom.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
				}
			}
		}
		else if(IMG_LOAD_TYPE_CUT)
		{
			if((value) && (0 == strcasecmp("hardware", (char *)value)))
			{
				IMG_JPEG_GetInfo(path, &image_information);
				if(image_information.width && image_information.height)
				{
					imgdesc = jpeg_image_descriptor(path,
							imgdesc,
							image_information.width,
							image_information.height,
							self->rect.w,
							self->rect.h);
					if((NULL == imgdesc) && (special_jpeg_cut.img_zoom_cut))
					{
						imgdesc = special_jpeg_cut.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
					}
				}
			}
			else
			{
				if(special_jpeg_cut.img_zoom_cut)
				{
					imgdesc = special_jpeg_cut.img_zoom_cut(path, (int)self->rect.w, (int)self->rect.h);
				}
			}
		}

		if(imgdesc)
			imgdesc->type = JPEG_TYPE;
	}
	else if(pic_buf[0] == 0x89 && pic_buf[1] == 'P' && pic_buf[2] == 'N' && pic_buf[3] == 'G')
	{
		//PNG
		imgdesc = IMG_PNG_Load(NULL, path);
		if(imgdesc)
			imgdesc->type = PNG_TYPE;
	}
	else if( (pic_buf[0] == 'B' && pic_buf[1] == 'M') ||
			(pic_buf[0] == 'B' && pic_buf[1] == 'A') ||
			(pic_buf[0] == 'C' && pic_buf[1] == 'I') ||
			(pic_buf[0] == 'C' && pic_buf[1] == 'P') ||
			(pic_buf[0] == 'I' && pic_buf[1] == 'C') ||
			(pic_buf[0] == 'P' && pic_buf[1] == 'T') )
	{
		//BMP
		imgdesc = IMG_BMP_Load(NULL, path);
		if(imgdesc)
		{
			imgdesc->type = BMP_TYPE;
			if(8 == imgdesc->bpp)
			{
				gal_img_release(imgdesc);
				return (1);
			}

			GUI_Printf("####bmp-> bpp=%d, width=%d, height=%d, data=%p\n",
					imgdesc->bpp, imgdesc->width, imgdesc->height, imgdesc->data);
		}
	}
	else if(pic_buf[0] == 'G' && pic_buf[1] == 'I' && pic_buf[2] == 'F')
	{
		//GIF
		imgdesc = IMG_GIF_Load(NULL, path);
		if(imgdesc)
		{
			imgdesc->type = GIF_TYPE;

			GUI_Printf("####gif-> bpp=%d, width=%d, height=%d, data=%p\n",
					imgdesc->bpp, imgdesc->width, imgdesc->height, imgdesc->data);
		}
	}
	else if(pic_buf[0] == 'R' && pic_buf[1] == 'I' && pic_buf[2] == 'F' && pic_buf[3] == 'F') {
		imgdesc = gal_img_load(NULL, path);
		if(imgdesc) {
			imgdesc->type = WEBP_TYPE;
			GUI_Printf("####gif-> bpp=%d, width=%d, height=%d, data=%p\n",
					imgdesc->bpp, imgdesc->width, imgdesc->height, imgdesc->data);
		}
	}
	else
	{
		gxlogd("[GUI_IMG] Unknow image type!!!\n");
		return 3;
	}

	if(NULL == imgdesc)
	{
		gxlogd("####load img fail! imgdesc=NULL\n");
		return 1;
	}

	if(NULL == imgdesc->data)
		return 2;

	switch(imgdesc->type)
	{
	case GIF_TYPE:
		{
			gif_src = (GIF_Para *)(imgdesc->data);
			gif_src->cur_image = NULL;

			image->mode = FILE_IMG_DYNAMIC;
			image->p.single_img = imgdesc;

			int alumode = GX_ALU_ROP_COPY;
			image->alu_mode = alumode;

			return 0;
		}
		break;
	case JPEG_TYPE:
	case BMP_TYPE:
	case PNG_TYPE:
	case WEBP_TYPE:
		break;
	default:
		gxlogd("[GUI_IMG] Error, not support image type.\n");
		return 3;
		break;
	}

	data = imgdesc->data;
	img_width = imgdesc->width;
	img_height = imgdesc->height;

	//---------------------------------------------
	//Create New surface
	if(img_width % 2 != 0)
	{
		img_width = img_width - 1;
	}
	else
	{
		img_width = img_width;
	}

	if(JPEG_TYPE == imgdesc->type)
	{
		if((NULL == value) ||
				((value) && (0 == strcasecmp("software", (char *)value))))
		{
			imgdesc->type = BMP_TYPE;
		}
		image->surface = image_desc_surface(imgdesc);
		imgdesc->type = JPEG_TYPE;
		if((value) &&
				(0 == strcasecmp("hardware", (char *)value)))
		{
			image->mode = FILE_IMG_DYNAMIC;
			image->p.single_img = imgdesc;
			return (0);
		}
	}
	else
		image->surface = image_desc_surface(imgdesc);

	switch(imgdesc->type)
	{
	case JPEG_TYPE:
		{
			pSrc = data;
			dst_data = image->surface->data;

			for(i = 0; i < img_height; i++)
			{
				for(j = 0; j < imgdesc->width; j++)
				{
					r = pSrc[0];
					g = pSrc[1];
					b = pSrc[2];

#ifdef SWITCH_ELEB
					dst_data[0] = (r & 0xF8) | ( (g&0xE0) >> 5 );
					dst_data[1] = ( (g&0x1C) << 3 ) | ( (b & 0xF8) >> 3);
#else
					dst_data[1] = (r & 0xF8) | ( (g&0xE0) >> 5 );
					dst_data[0] = ( (g&0x1C) << 3 ) | ( (b & 0xF8) >> 3);
#endif
					dst_data += 2;
					pSrc += 3;
				}

				if(img_width%2 != 0)
				{
					dst_data +=2;
				}
			}

			if(img_width%2 != 0)
			{
				img_width -= 1;
			}
			imgdesc->type = BMP_TYPE;
			imgdesc->width = img_width;
			memcpy(imgdesc->data, image->surface->data, image->surface->buffer_size);
			imgdesc->size = image->surface->buffer_size;
			imgdesc->width = image->surface->sf.width;
			imgdesc->height = image->surface->sf.height;
			if((image->surface) &&
					((image->surface->sf.width == self->rect.w) &&
					 (image->surface->sf.height == self->rect.h)))
			{
				hd_free_surface(image->surface);
				image->surface = NULL;
			}
		}
		break;
	case PNG_TYPE:
		imgdesc->size = image->surface->buffer_size;
		imgdesc->width = image->surface->sf.width;
		imgdesc->height = image->surface->sf.height;
		if((image->surface) &&
				((image->surface->sf.width == self->rect.w) &&
				 (image->surface->sf.height == self->rect.h)))
		{
			hd_free_surface(image->surface);
			image->surface = NULL;
		}
		/*memcpy(image->surface->data,
		  data,
		  image->surface->buffer_size);*/
		break;
	case BMP_TYPE:
		{
			pSrc = data;
			dst_data = image->surface->data;

			if(img_width%2 != 0)
			{
				per_pixel = 2;
				per_line_width = img_width * per_pixel;
				for(i=0; i<img_height; i++)
				{
					memcpy(dst_data, pSrc, per_line_width);
					dst_data += per_line_width;
					pSrc += per_line_width;

					memset(dst_data, 0x0, per_pixel);
					dst_data += per_pixel;
				}

				img_width -=1;
			}
			else
			{
				memcpy(dst_data,
						pSrc,
						image->surface->buffer_size);
			}
			memcpy(imgdesc->data, image->surface->data, image->surface->buffer_size);
			imgdesc->size = image->surface->buffer_size;
			imgdesc->width = image->surface->sf.width;
			imgdesc->height = image->surface->sf.height;
			if((image->surface) &&
					((image->surface->sf.width == self->rect.w) &&
					 (image->surface->sf.height == self->rect.h)))
			{
				hd_free_surface(image->surface);
				image->surface = NULL;
			}
		}
		break;
	case GIF_TYPE:
		//can't run to here.
		break;
	default:
		break;
	}

	if((image->surface) &&
			((image->surface->sf.width != self->rect.w) ||
			 (image->surface->sf.height != self->rect.h)))
	{
		zoom_rect.x = zoom_rect.y = 0;
		zoom_rect.w = self->rect.w;
		zoom_rect.h = self->rect.h;
		if(image->surface->bpp == 24) {
			zoom_surface = hd_get_surface(&zoom_rect, 32, NULL, GUI_FALSE);
		} else {
			zoom_surface = hd_get_surface(&zoom_rect, image->surface->bpp, NULL, GUI_FALSE);
		}
		if(NULL == zoom_surface)
		{
			return (1);
		}
		src_rect.x = src_rect.y = 0;
		src_rect.w = image->surface->sf.width;
		src_rect.h = image->surface->sf.height;
		gdi_begin();
		hd_img_desc(zoom_surface, imgdesc, 0, 0, zoom_surface->sf.width, zoom_surface->sf.height);
		//ret = hd_zoom_surface(image->surface, &src_rect, zoom_surface, &zoom_rect);
		gdi_end();
		hd_free_surface(image->surface);
		image->surface = zoom_surface;

		if(24 == imgdesc->bpp) {
			bpp = 32;
		} else {
			bpp = imgdesc->bpp;
		}
		type = imgdesc->type;
		gal_img_release(imgdesc);

		imgdesc = gal_img_new();
		imgdesc->data = GUI_MALLOCZ(image->surface->buffer_size);
		memcpy(imgdesc->data, image->surface->data, image->surface->buffer_size);
		imgdesc->size = image->surface->buffer_size;
		imgdesc->width = image->surface->sf.width;
		imgdesc->height = image->surface->sf.height;
		imgdesc->bpp = bpp;
		imgdesc->type = type;
	}

	image->mode = FILE_IMG_DYNAMIC;
	image->p.single_img = imgdesc;

	switch(load_type)
	{
	case IMG_LOAD_TYPE_RESIZE:
		{
			self->rect.w = imgdesc->width;
			self->rect.h = imgdesc->height;
		}
		break;
	case IMG_LOAD_TYPE_ZOOM:
	case IMG_LOAD_TYPE_CUT:
		break;
	}

	if(imgdesc->type == JPEG_TYPE)
	{
		if((0 == strstr(path, "jpg")) && (0 != strstr(path, "jpeg")))
			gxlogd("[GUI_IMG] Warning: Maybe it is not JPEG file.\n");
	}
	else if(imgdesc->type == BMP_TYPE)
	{
		if((strstr(path, "jpg")) || (strstr(path, "jpeg")) || (strstr(path, "bmp")))
			gxlogd("[GUI_IMG] Warning: Maybe it is not BMP file.\n");
	}
	else if(imgdesc->type == GIF_TYPE)
	{
		if(0 == strstr(path, "gif"))
			gxlogd("[GUI_IMG] Warning: Maybe it is not GIF file.\n");
	}
	else if(imgdesc->type == PNG_TYPE)
	{
		if(0 == strstr(path, "png"))
			gxlogd("[GUI_IMG] Warning: Maybe it is not PNG file.\n");
	}
	else if(imgdesc->type == WEBP_TYPE) {
		if(0 == strstr(path, "webp"))
			gxlogd("[GUI_IMG] Warning: Maybe it is not WEBP file.\n");
	}

	return ret;

}

static int file_image_set_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = NULL;
	GuiFileImage *image = NULL;
	GuiWidget *parent = NULL, *sibling = NULL;
	char *pData = NULL, *path = NULL, *id = NULL;
	image_desc *imgdesc = NULL, *oldimage = NULL;
	GAL_Rect snap_rect = {0}, src_rect = {0};
	int value = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	if(0 == strcasecmp("opacity", name))
	{
		if(NULL == data)
			return (1);
		pData = (char*)data;
		value = atoi(pData);
		image->opacity = value;
	}
	else if(0 == strcasecmp("img", name) &&
			(FILE_IMG_SINGLE == image->mode))
	{
		if(NULL == data)
			return (1);
		pData = (char*)data;
		widget_set_property(self, name, pData);
		parent = widget_get_parent(self);
		sibling = widget_get_firstchild(parent);
		while(sibling)
		{
			if((self->rect.x <= sibling->rect.x) &&
					(self->rect.y <= sibling->rect.y) &&
					((self->rect.x + self->rect.w) >= (sibling->rect.x + sibling->rect.w)) &&
					((self->rect.y + self->rect.h) >= (sibling->rect.y + sibling->rect.h)))
			{
				if(0 == strcasecmp("text", sibling->classname))
				{
					GUI_SetProperty(sibling->name, "clear_surface", NULL);
					break;
				}
			}
			sibling = widget_get_nextsibling(sibling);
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("update_rect", name))
	{
		if(NULL == data)
			return (1);
		_image_update_rect(self, (GUI_Rect *)data);
		return (0);
	}
	else if(0 == strcasecmp("file", name))
	{
		path = (char *)data;
		if(NULL == path)
		{
			return (1);
		}

		id = _get_path_id(path);
		imgdesc = gal_img_load(NULL, path);
		if(JPEG_TYPE == imgdesc->type)
		{
			_covert2rgb(imgdesc);
		}

		imgdesc->type = FILE_TYPE;

		if(FILE_IMG_SINGLE == image->mode)
		{
			oldimage = image->p.single_img;
			if(oldimage && (FILE_TYPE == oldimage->type))
			{
				gal_img_release(oldimage);
			}

			image->p.single_img = imgdesc;
		}
	}
	else if(0 == strcasecmp("load_img", name) ||
			0 == strcasecmp("load_zoom_img", name) ||
			0 == strcasecmp("load_scal_img", name))
	{
		int ret = 0;
		/*if(gui_core->config.bpp != 16)
		  {
		  gxlogd("[GUI_IMG] now the load_img don't suport osd which on bpp 16!!!\n");
		  return (1);
		  }*/

		if(FILE_IMG_SINGLE == image->mode)
		{
#if 0
			oldimage = image->p.single_img;
			if(oldimage)
			{
				GUI_FREE(oldimage->pal);
				GUI_FREE(oldimage->data);
				GUI_FREE(oldimage->filename);
				GUI_FREE(image->p.single_img);
			}
#endif
		}
		else if(FILE_IMG_DYNAMIC == image->mode)
		{
			if(image->p.single_img)
			{
				if(NULL != image->p.single_img->data)
				{
					if(GIF_TYPE == image->p.single_img->type)
					{
						IMG_GIF_Free((void *)image->p.single_img->data);
					}
					else
					{
						//destroy the new surface
						if(image->surface)
						{
							hd_free_surface(image->surface);
							image->surface = NULL;
						}
					}

					gal_img_release(image->p.single_img);
					image->p.single_img = NULL;
				}

			}
		}
		else
		{
			gxlogd("[GUI_IMG] property 'load_img' need IMG_SINGLE type!!!\n");
			return (1);
		}

		path = (char *)data;
		if(NULL != path)
		{
			int cache = 0;
			cache = gal_img_uncache(GUI_TRUE);
			if(0 == strcasecmp("load_zoom_img", name))
			{
				ret = load_img_to_surface(path, image, self, IMG_LOAD_TYPE_ZOOM);
			}
			else if(0 == strcasecmp("load_scal_img", name))
			{
				ret = load_img_to_surface(path, image, self, IMG_LOAD_TYPE_CUT);
			}
			else
			{
				ret = load_img_to_surface(path, image, self, IMG_LOAD_TYPE_RESIZE);
			}
			gal_img_uncache(cache);
		}
		else
		{
			image->mode = FILE_IMG_DYNAMIC;
			image->p.single_img = imgdesc;
		}

		widget_set_update(self, GUI_TRUE);
		return ret;
	}
	else if(0 == strcasecmp("init_gif_alu_mode", name))
	{
		if(NULL == data)
		{
			int alumode = GX_ALU_ROP_COPY;
			image->alu_mode = alumode;
		}
		else
		{
			int alumode = *((int*)data);
			if(alumode < 0)
				alumode = GX_ALU_ROP_COPY;
			image->alu_mode = alumode;
		}
	}
	else if(0 == strcasecmp("draw_gif", name))
	{
		if(NULL == data)
		{
			_draw_file_gif(image, GX_ALU_ROP_COPY);
		}
		else
		{
			int alumode = *((int*)data);
			if(alumode < 0)
				alumode = GX_ALU_ROP_COPY;
			image->alu_mode = alumode;
			_draw_file_gif(image, (GxAluMode)alumode);
		}
		gui_core->widget_need_update_count++;
		return (0);
	}
	else if(0 == strcasecmp("width", name))
	{
		if(NULL == data)
			self->rect.w = 0;
		else
			self->rect.w = *((int*)data);
		return (0);
	}
	else if(0 == strcasecmp("height", name))
	{
		if(NULL == data)
			self->rect.h = 0;
		else
			self->rect.h = *((int*)data);
		return (0);
	}
	else if(0 == strcasecmp("x", name))
	{
		if(NULL == data)
			self->rect.x = 0;
		else
			self->rect.x = *((int*)data);
		return (0);
	}
	else if(0 == strcasecmp("y", name))
	{
		if(NULL == data)
			self->rect.y = 0;
		else
			self->rect.y = *((int*)data);
		return (0);
	}
	else if(0 == strcasecmp("snap", name))
	{
		pData = (char*)data;
		if((strcasecmp("null", pData)) && (image->snap_surface))
		{
			hd_free_surface(image->snap_surface);
			image->snap_surface = NULL;
		}
		else
		{
			sscanf((const char *)pData, "[%d,%d,%d,%d]",
					&snap_rect.x, &snap_rect.y, &snap_rect.w, &snap_rect.h);
			src_rect = snap_rect;
			src_rect.x = src_rect.y = 0;

			if(image->snap_surface)
			{
				if((image->snap_surface->sf.width == src_rect.w) &&
						(image->snap_surface->sf.height == src_rect.h))
				{
					;
				}
				else
				{
					hd_free_surface(image->snap_surface);
					image->snap_surface = hd_get_surface(&src_rect, gui_core->config.bpp, NULL, GUI_TRUE);
				}
			}
			else
			{
				image->snap_surface = hd_get_surface(&src_rect, gui_core->config.bpp, NULL, GUI_TRUE);
			}
			gdi_commit();
			gal_copy_surface(screen, &snap_rect, image->snap_surface, &src_rect);
			widget_set_property(self, name, data);
		}
	}
	else if(0 == strcasecmp("update", name)) {
		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		return (1);
	}

	widget_set_update(self, GUI_TRUE);

	return (0);
}

static int file_image_get_property(GuiWidget *self, char *name, void *data)
{
	GuiFileImage *image = NULL;
	char *string  = NULL;

	if((NULL == self) || (NULL == name) || (NULL == data))
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	if(0 == strcmp("opacity", name))
	{
		string = (char *)data;
		image->opacity = widget_get_int(self, string, 100);
		if(image->opacity < 0 || image->opacity > 100)
		{
			image->opacity = 100;
		}
	}
	else if(0 == strcmp("img_type", name))
	{
		if(image && image->p.single_img)
			*(int*)data = (int)image->p.single_img->type;
		else
			*(int*)data = -1;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int file_image_prepare_image(GuiWidget *self)
{
	const char *value = NULL;
	GuiFileImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	switch(image->mode)
	{
	case IMG_SINGLE:
		value = widget_get_property(self, "img");
		if((NULL != value) || (NULL == image->p.single_img))
		{
			image->p.single_img = widget_img_load(value);
		}
		break;
	case IMG_MULTIPLE:
		value = widget_get_property(self, "lt_img");
		if((NULL != value) || (NULL == image->p.com_img.image_lt))
		{
			image->p.com_img.image_lt = widget_img_load(value);
		}
		value = widget_get_property(self, "rt_img");
		if((NULL != value) || (NULL == image->p.com_img.image_rt))
		{
			image->p.com_img.image_rt = widget_img_load(value);
		}
		value = widget_get_property(self, "lb_img");
		if((NULL != value) || (NULL == image->p.com_img.image_lb))
		{
			image->p.com_img.image_lb = widget_img_load(value);
		}
		value = widget_get_property(self, "rb_img");
		if((NULL != value) || (NULL == image->p.com_img.image_rb))
		{
			image->p.com_img.image_rb = widget_img_load(value);
		}
		value = widget_get_property(self, "l_img");
		if((NULL != value) || (NULL == image->p.com_img.image_l))
		{
			image->p.com_img.image_l = widget_img_load(value);
		}
		value = widget_get_property(self, "r_img");
		if((NULL != value) || (NULL == image->p.com_img.image_r))
		{
			image->p.com_img.image_r = widget_img_load(value);
		}
		value = widget_get_property(self, "t_img");
		if((NULL != value) || (NULL == image->p.com_img.image_t))
		{
			image->p.com_img.image_t = widget_img_load(value);
		}
		value = widget_get_property(self, "b_img");
		if((NULL != value) || (NULL == image->p.com_img.image_b))
		{
			image->p.com_img.image_b = widget_img_load(value);
		}
		break;
	default:
		break;
	}

	return (0);
}

static int file_image_clear_image(GuiWidget *self)
{
	GuiFileImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	switch(image->mode)
	{
	case IMG_SINGLE:
		if(image->p.single_img)
		{
			widget_img_clear(&(image->p.single_img));
		}
		break;
	case IMG_MULTIPLE:
		if(image->p.com_img.image_lt)
		{
			widget_img_clear(&(image->p.com_img.image_lt));
		}

		if(image->p.com_img.image_rt)
		{
			widget_img_clear(&(image->p.com_img.image_rt));
		}

		if(image->p.com_img.image_lb)
		{
			widget_img_clear(&(image->p.com_img.image_lb));
		}

		if(image->p.com_img.image_rb)
		{
			widget_img_clear(&(image->p.com_img.image_rb));
		}

		if(image->p.com_img.image_l)
		{
			widget_img_clear(&(image->p.com_img.image_l));
		}

		if(image->p.com_img.image_r)
		{
			widget_img_clear(&(image->p.com_img.image_r));
		}

		if(image->p.com_img.image_t)
		{
			widget_img_clear(&(image->p.com_img.image_t));
		}

		if(image->p.com_img.image_b)
		{
			widget_img_clear(&(image->p.com_img.image_b));
		}
		break;
	default:
		break;
	}

	return (0);
}

static int file_image_unactive(GuiWidget *self)
{
	GuiFileImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiFileImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	if(image->surface) {
		gal_free_surface(image->surface);
		image->surface = NULL;
	}

	if(image->snap_surface) {
		gal_free_surface(image->snap_surface);
		image->snap_surface = NULL;
	}

	return (0);
}

GuiWidgetOps file_image_ops =
{
	.owner = "file_image",
	.create = create_file_image,
	.release = file_image_release,
	.prepare_image = file_image_prepare_image,
	.draw = file_image_draw,
	.clear_image   = file_image_clear_image,
	.unactive = file_image_unactive,
	.event = file_image_event,
	.set = file_image_set_property,
	.get = file_image_get_property
};


