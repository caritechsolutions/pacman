#include "widget.h"
#include "all_widget.h"

//#include <wchar.h>

static int _get_image_mode(GuiWidget *self)
{
	const char *value = NULL;


	if(NULL == self)
	{
		return (IMG_SINGLE);
	}

	if(self->property)
	{
		value = (const char *)hash_get(self->property, "mode");
	}

	if(NULL == value)
	{
		return (IMG_MULTIPLE);
	}

	if(0 == strcmp("single", value))
	{
		return (IMG_SINGLE);
	}
	else if(0 == strcmp("multiple", value))
	{
		return (IMG_MULTIPLE);
	}
	else
	{
		return (IMG_SINGLE);
	}
}

static int gif_timer_func(void *userdata)
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

static int _config_image_parametre(GuiWidget *self)
{
	const char *value = NULL;
	GuiImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
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

	image->mode = _get_image_mode(self);
	return (0);
}


static void* create_image(GuiWidget *self)
{
	GuiImage *image = NULL, *image_style = NULL;
	GuiWidget *parent = NULL;
	GuiWidget *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	image = (GuiImage *)GUI_MALLOC(sizeof(GuiImage));
	if(NULL == image)
	{
		return (NULL);
	}

	memset(image, 0, sizeof(GuiImage));

	image->base = self;
	self->object = (void *)image;
	self->state = 0;

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		image_style = (GuiImage *)GUI_MALLOC(sizeof(GuiImage));
		if(NULL == image_style)
		{
			GUI_FREE(image);
			return (NULL);
		}

		memset(image_style, 0, sizeof(GuiImage));
		style->object = (void *)image_style;

		_config_image_parametre(style);

		memcpy(image, image_style, sizeof(GuiImage));

		image->base = self;
		GUI_FREE(style->object);
	}

	_config_image_parametre(self);

	if(image->p.single_img)
	{
		/*Turn on a timer*/
		image->timer = create_timer(gif_timer_func, 50, self, TIMER_REPEAT);
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

static int image_release(GuiWidget *self)
{
	GuiImage *image = NULL;
	image_desc *img_desc = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);

	if(NULL == image)
	{
		return (1);
	}

	if(IMG_SINGLE == image->mode)
	{
		img_desc = image->p.single_img;

		if(img_desc && (FILE_TYPE == img_desc->type))
		{
			gal_img_release(img_desc);
		}
	}
	else
	{
		if(image->map_surface)
		{
			gal_free_surface(image->map_surface);
			image->map_surface = NULL;
		}
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

static int _draw_gif(GuiImage *image)
{
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL;
	image_desc *gif_desc = NULL;
	GIF_Para *gif_header = NULL;
	GIF_Image *gif_node = NULL;
	int ret = 0;
	Color color = 0, *pclut = NULL;
	image_desc img_des = {0};

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

	gif_header = (GIF_Para *)(gif_desc->data);
	if(NULL == gif_header)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	/*Switch the palette in case palette in GIF is different from main palette*/
	if((gif_header->pal) && (gui_core->config.clut))
	{
		if(0 != memcmp(gui_core->config.clut->palette, gif_header->pal, gif_header->num_color))
		{
			ret = gui_set_clut(screen, gif_header->pal, 0, gif_header->num_color);
			if(!ret)
			{
				return (1);
			}
		}
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

	img_des.bpp = gif_header->bpp;
	img_des.width = gif_node->img_width;
	img_des.height = gif_node->img_height;
	if(gif_node->img_pal)
	{
		img_des.pal = gif_node->img_pal;
	}
	else
	{
		img_des.pal = gif_header->pal;
	}
	img_des.data = gif_node->img_data;

	color = gal_get_gui_transparent();
	if(gif_node->img_pal)
	{
		pclut = gif_node->img_pal;
	}
	else
	{
		pclut = gif_header->pal;
	}
	if((pclut[gif_node->img_trans] != color) && gif_node->img_trans)
	{
		gal_set_gui_transparent(pclut[gif_node->img_trans]);
	}
	img_des.size = img_des.width * img_des.height * img_des.bpp / 8;
	gal_img_desc(screen,
			&img_des,
			self->rect.x + gif_node->img_left,
			self->rect.y + gif_node->img_top,
			self->rect.w,
			self->rect.h);

	if((pclut[gif_node->img_trans] != color) && gif_node->img_trans)
	{
		gal_set_gui_transparent(color);
	}

	return (0);
}

static int _add_alpha(int color, int alpha)
{
	GuiCore *gui_core = NULL;
	int real_color = 0;

	gui_core = GUI_GetCurrent();
	real_color = color;

	if(16 == gui_core->config.bpp) {
		real_color = ((alpha << 16) & 0xFF0000) | real_color;
	} else if(32 == gui_core->config.bpp) {
		if((real_color != gui_core->config.gui_trans) &&
		   (real_color != gui_core->config.osd_trans))
			real_color = (alpha << 24) | real_color;
	}

	return (real_color);
}

static int _draw_img(GuiSurface *surface, GuiImage *image, GUI_Rect *rect)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GUI_Rect valid_rect = {0}, img_rect = {0};
	GuiWidget *self = NULL;
	int color = 0, index = 0;

	if((NULL == surface) || (NULL == image) || (NULL == rect))
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	valid_rect = *rect;

	if(NULL == image->p.single_img)
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(surface, color);
		if(image->opacity && 32 == surface->bpp)
		{
			color = _add_alpha(color, image->opacity);
		}
		gal_fillrect(surface, (GAL_Rect *)&(valid_rect), color);
	}
	else
	{
		if(GIF_TYPE == image->p.single_img->type)
		{
			_draw_gif(image);
		}
		else
		{
			img_rect.w = image->p.single_img->width;
			img_rect.h = image->p.single_img->height;

			if((valid_rect.w > img_rect.w) ||
					(valid_rect.h > img_rect.h))
			{
				index = widget_get_state_index(self);
				color = self->back_color[index];
				color = gal_color2index(surface, color);
				if(image->opacity && 32 == surface->bpp)
				{
					color = _add_alpha(color, image->opacity);
				}
				gal_fillrect(surface, (GAL_Rect *)&(valid_rect), color);
			}
			if(0xFF != image->opacity)
			{
				image->p.single_img->alpha = image->opacity;
			}
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				gal_img_desc_mirror(surface,
						    image->p.single_img,
						    valid_rect.x,
						    valid_rect.y,
						    valid_rect.w,
						    valid_rect.h,
						    REVERSE_HORIZONTAL);
			} else {
				gal_img_desc(surface,
						image->p.single_img,
						valid_rect.x,
						valid_rect.y,
						valid_rect.w,
						valid_rect.h);
			}
			image->p.single_img->alpha = 0;
		}
	}
	return (0);

}

static int _draw_rect(GuiSurface *surface, GUI_Rect *rect, GuiImage *image, int color)
{
	int h0 = 0, h1 = 0, h2 = 0, h3 = 0;
	int w0 = 0, w1 = 0, w2 = 0, w3 = 0;
	int offset0 = 0, offset1 = 0, offset2 = 0, offset3 = 0;
	GAL_Rect valid_rect = {0};

	if(NULL == rect || NULL == image)
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
	gal_fillrect(surface, (GAL_Rect *)&valid_rect, color);

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
	valid_rect.w = rect->w - offset2 - offset3;

	valid_rect.h = rect->h - h0 - h2;
	gal_fillrect(surface, (GAL_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + w2;
	valid_rect.y = rect->y + rect->h - h3;
	valid_rect.h = h3;
	valid_rect.w = rect->w - w2 - w3;
	if(h3 > offset1)
	{
		valid_rect.h = h3 - offset1;
	}
	else
	{
		valid_rect.h = 0;
	}
	gal_fillrect(surface, (GAL_Rect *)&valid_rect, color);

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
	gal_fillrect(surface, (GAL_Rect *)&valid_rect, color);

	valid_rect.x = rect->x + w0;
	valid_rect.y = rect->y + h0;
	valid_rect.w = rect->w - w0 -w1;
	valid_rect.h = rect->h - h0 - h2;
	gal_fillrect(surface, (GAL_Rect *)&valid_rect, color);


	return (0);
}


static int _buffer_rect(char *buffer, GUI_Rect *full_rect, GUI_Rect *rect, int color)
{
	GuiCore *gui_core = NULL;
	int width = 0, i = 0;
	char *temp = NULL;

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

static int _draw_buffer_rect(char *buffer, GUI_Rect *rect, GuiImage *image, int color)
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

static int _draw_multi_img(GuiSurface *surface, GuiImage *image, GUI_Rect *rect)
{
	GUI_Rect valid_rect = {0};
	GAL_Rect src_rect = {0}, dst_rect = {0};
	GuiSurface *src_surface = NULL;
	GuiWidget *self = NULL;
	int color = 0, index = 0;
	int x0 = 0, y0 = 0, x1 = 0, y1 = 0;//, offset = 0;

	if((NULL == surface) || (NULL == image) || (NULL == rect))
	{
		return (1);
	}

	self = image->base;

	if(NULL == self)
	{
		return (1);
	}

	valid_rect = *rect;

	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(screen, color);
	if(image->opacity)
	{
		color = _add_alpha(color, image->opacity);
	}

	_draw_rect(surface, &valid_rect, image, color);

	x0 = valid_rect.x;
	x1 = valid_rect.x + valid_rect.w;

	if(image->p.com_img.image_lt)
	{
		if(0xFF != image->opacity)
		{
			image->p.com_img.image_lt->alpha = image->opacity;
		}

		gal_img_desc(surface,
				image->p.com_img.image_lt,
				valid_rect.x,
				valid_rect.y,
				image->p.com_img.image_lt->width,
				image->p.com_img.image_lt->height);
		image->p.com_img.image_lt->alpha = 0;
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
		if(0xFF != image->opacity)
		{
			image->p.com_img.image_rt->alpha = image->opacity;
		}

		gal_img_desc(surface,
				image->p.com_img.image_rt,
				x1,
				valid_rect.y,
				image->p.com_img.image_rt->width,
				image->p.com_img.image_rt->height);
		image->p.com_img.image_rt->alpha = 0;
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
	}

	if(image->p.com_img.image_t) {
		dst_rect.x = rect->x;
		dst_rect.y = rect->y;
		dst_rect.w = rect->w;
		dst_rect.h = image->p.com_img.image_t->height;
		if(image->p.com_img.image_lt) {
			dst_rect.x += image->p.com_img.image_lt->width;
			dst_rect.w -= image->p.com_img.image_lt->width;
		}

		if(image->p.com_img.image_rt) {
			dst_rect.w -= image->p.com_img.image_rt->width;
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = image->p.com_img.image_t->width;
		src_rect.h = image->p.com_img.image_t->height;
		src_surface = image_desc_surface(image->p.com_img.image_t);
		hd_tile_surface(src_surface, &src_rect, surface, &dst_rect);
		hd_add_blit_element(src_surface);
	}

	if(image->p.com_img.image_lb)
	{
		x0 = valid_rect.x + image->p.com_img.image_lb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_lb->height;
		if(0xFF != image->opacity)
		{
			image->p.com_img.image_lb->alpha = image->opacity;
		}

		gal_img_desc(surface,
				image->p.com_img.image_lb,
				valid_rect.x,
				y1,
				image->p.com_img.image_lb->width,
				image->p.com_img.image_lb->height);
		image->p.com_img.image_lb->alpha = 0;
	}
	else
	{
		x0 = valid_rect.x;
		y1 = valid_rect.y + valid_rect.h;
	}

	if(image->p.com_img.image_l) {
		dst_rect.x = rect->x;
		dst_rect.y = rect->y;
		dst_rect.w = image->p.com_img.image_l->width;
		dst_rect.h = rect->h;

		if(image->p.com_img.image_lt) {
			dst_rect.y += image->p.com_img.image_lt->height;
			dst_rect.h -= image->p.com_img.image_lt->height;
		}

		if(image->p.com_img.image_lb) {
			dst_rect.h -= image->p.com_img.image_lb->height;
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = image->p.com_img.image_l->width;
		src_rect.h = image->p.com_img.image_l->height;
		src_surface = image_desc_surface(image->p.com_img.image_l);
		hd_tile_surface(src_surface, &src_rect, surface, &dst_rect);
		hd_add_blit_element(src_surface);
	}

	if(image->p.com_img.image_rb)
	{
		x1 = valid_rect.x + valid_rect.w - image->p.com_img.image_rb->width;
		y1 = valid_rect.y + valid_rect.h - image->p.com_img.image_rb->height;
		if(0xFF != image->opacity)
		{
			image->p.com_img.image_rb->alpha = image->opacity;
		}

		gal_img_desc(surface,
				image->p.com_img.image_rb,
				x1,
				y1,
				image->p.com_img.image_rb->width,
				image->p.com_img.image_rb->height);
		image->p.com_img.image_rb->alpha = 0;
	}
	else
	{
		x1 = valid_rect.x + valid_rect.w;
		y1 = valid_rect.y + valid_rect.h;
	}
	if( image->p.com_img.image_b) {
		dst_rect.x = rect->x;
		dst_rect.y = rect->y + rect->h - image->p.com_img.image_b->height;
		dst_rect.w = rect->w;
		dst_rect.h = image->p.com_img.image_b->height;

		if(image->p.com_img.image_lb) {
			dst_rect.x += image->p.com_img.image_lb->width;
			dst_rect.w -= image->p.com_img.image_lb->width;
		}

		if(image->p.com_img.image_rb) {
			dst_rect.w -= image->p.com_img.image_rb->width;
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = image->p.com_img.image_b->width;
		src_rect.h = image->p.com_img.image_b->height;
		src_surface = image_desc_surface(image->p.com_img.image_b);
		hd_tile_surface(src_surface, &src_rect, surface, &dst_rect);
		hd_add_blit_element(src_surface);
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

	if( image->p.com_img.image_r) {
		dst_rect.x = rect->x + rect->w - image->p.com_img.image_r->width;
		dst_rect.y = rect->y;
		dst_rect.w = image->p.com_img.image_r->width;
		dst_rect.h = rect->h;

		if(image->p.com_img.image_rt) {
			dst_rect.y += image->p.com_img.image_rt->height;
			dst_rect.h -= image->p.com_img.image_rt->height;
		}

		if(image->p.com_img.image_rb) {
			dst_rect.h -= image->p.com_img.image_rb->height;
		}

		src_rect.x = src_rect.y = 0;
		src_rect.w = image->p.com_img.image_r->width;
		src_rect.h = image->p.com_img.image_r->height;
		src_surface = image_desc_surface(image->p.com_img.image_r);
		hd_tile_surface(src_surface, &src_rect, surface, &dst_rect);
		hd_add_blit_element(src_surface);
	}

	return (0);
}

static int _draw_surface(GuiWidget *self)
{
	GuiImage *image = NULL;
	GuiSurface *src = NULL, *dst = NULL, *temp = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};
	GUI_Rect rect = {0};
	int bpp = 0, mirrored = 0;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);

	if(NULL == image)
	{
		return (1);
	}

	src = image->surface;
	src_rect.x = src_rect.y = 0;
	src_rect.w = src->sf.width;
	src_rect.h = src->sf.height;

	mirrored = self->mirrored;
	widget_get_rect(self, &rect);
	self->mirrored = mirrored;
	dst_rect.x = dst_rect.y = 0;
	dst_rect.w = rect.w;
	dst_rect.h = rect.h;

	if((src_rect.w != self->rect.w) ||
			(src_rect.h != self->rect.h))
	{
		bpp = src->bpp;
		if(GX_COLOR_FMT_YCBCR422 == src->sf.color_format)
			bpp = 0;

		temp = hd_get_surface(&dst_rect, bpp, NULL, GUI_TRUE);
		if(NULL == temp)
			return 1;

		gdi_begin();
		hd_blit_surface(src, &src_rect, temp, &dst_rect);
		gdi_end();
	}
	else
	{
		temp = src;
	}

	dst = screen;
	src_rect.w = temp->sf.width;
	src_rect.h = temp->sf.height;
	dst_rect.x = self->rect.x;
	dst_rect.y = self->rect.y;
	if(temp != src) {
		gdi_commit();
		hd_copy_surface(temp, &src_rect, dst, &dst_rect);
	} else {
		gdi_begin();
		hd_blit_surface(temp, &src_rect, dst, &dst_rect);
		gdi_end();
	}

	if(temp != src)
		hd_free_surface(temp);

	return 0;
}

#define COPY_SURFACE_OPERATION					surface = hd_get_surface(&rect, 32, NULL, GUI_TRUE);\
												if(NULL == surface)\
													return (1);\
												\
												img_rect = self->rect;\
												img_rect.x = img_rect.y = 0;\
												gdi_begin();\
												color = gal_color2index(dst_surface, gui.config.gui_trans);\
												hd_fillrect(surface, &rect, color);\
												gdi_end();\
												\
												gdi_begin();\
												_draw_multi_img(surface, image, &(img_rect));\
												gdi_end();\
												surface->alpha = (self->back_color[0] >> 24) & 0x000000FF;

static int image_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiImage *image = NULL;
	image_desc *imgdesc = NULL, *oldimage = NULL;
	GuiSurface *dst_surface = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);

	if(NULL == image)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	// TODO:
	/*Adjust the opacity of the OSD*/
	if(image->surface)
	{
		_draw_surface(self);
		return 0;
	}

	dst_surface = widget_get_current_surface(screen);
	switch(image->mode)
	{
	case IMG_SINGLE:
		if(image->filepath)
		{
			imgdesc = gal_img_load(NULL, image->filepath);
			if(NULL == imgdesc)
				goto NORMAL;

			imgdesc->type = FILE_TYPE;

			if(IMG_SINGLE == image->mode)
			{
				oldimage = image->p.single_img;
				if(oldimage && (FILE_TYPE == oldimage->type))
				{
					gal_img_release(oldimage);
				}

				image->p.single_img = imgdesc;
			}
		}
NORMAL:
		_draw_img(dst_surface, image, &(self->rect));
		break;
	case IMG_MULTIPLE:
		if((32 == gui_core->config.bpp) &&
		   ((self->back_color[0] >> 24) & 0x000000FF))
		{
			GuiSurface *surface = NULL;
			GAL_Rect rect = {0}, dst_rect = {0};
			GUI_Rect img_rect = {0};
			int color = 0;

			rect.x = rect.y = 0;
			rect.w = self->rect.w;
			rect.h = self->rect.h;

			if(NULL == image->map_surface)
			{
				COPY_SURFACE_OPERATION;
				image->map_surface = surface;
			}
			else
			{
				surface = image->map_surface;
			}

			dst_rect = rect;
			dst_rect.x = self->rect.x;
			dst_rect.y = self->rect.y;
			hd_stretch_surface(surface, &rect, dst_surface, &dst_rect);
			hd_add_blit_element(NULL);
		}
		else
		{
			GUI_Rect valid_rect = {0}, src_rect = {0}, dst_rect = {0};
			GuiSurface *surface = NULL;

			if(image->map_surface)
			{
				gal_free_surface(image->map_surface);
				image->map_surface = NULL;
			}
			valid_rect = self->rect;
			valid_rect.x = valid_rect.y = 0;
			surface = gal_get_surface((GAL_Rect *)(&valid_rect), gui_core->config.bpp);
			if(NULL == surface)
			{
				return (1);
			}
			gdi_commit();
			gdi_begin();
			gal_copy_surface(dst_surface, (GAL_Rect *)&(self->rect), surface, (GAL_Rect *)&(valid_rect));
			gdi_end();
			gdi_commit();
			gdi_begin();
			_draw_multi_img(surface, image, &(valid_rect));
			gdi_end();
			src_rect = valid_rect;
			dst_rect = self->rect;
			gdi_begin();
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				hd_mirror_image_surface(surface, (GAL_Rect *)&src_rect, dst_surface, (GAL_Rect *)&dst_rect, REVERSE_HORIZONTAL);
			} else {
				hd_stretch_surface(surface, (GAL_Rect *)&src_rect, dst_surface, (GAL_Rect *)&dst_rect);
			}
			gdi_end();
			hd_free_surface(surface);
		}
		break;
	default:
		break;
	}

	return (0);
}

static int image_update(GuiWidget *self, GUI_Rect *rect)
{
	GuiCore *gui_core = NULL;
	GuiImage *image = NULL;
	GuiSurface *surface = NULL;
	GUI_Rect src_rect = {0}, dst_rect = {0}, valid_rect = {0};
	int index = 0, color = 0;

	if((NULL == self) || (NULL == rect))
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	image = (GuiImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	dst_rect = *rect;
	if(dst_rect.x < self->rect.x)
	{
		dst_rect.x = self->rect.x;
	}
	if(dst_rect.y < self->rect.y)
	{
		dst_rect.y = self->rect.y;
	}
	if((dst_rect.x + dst_rect.w) > (self->rect.x + self->rect.w))
	{
		dst_rect.w = (self->rect.x + self->rect.w) - dst_rect.x;
	}

	if((dst_rect.y + dst_rect.h) > (self->rect.y + self->rect.h))
	{
		dst_rect.h = (self->rect.y + self->rect.h) - dst_rect.y;
	}

	src_rect = dst_rect;
	src_rect.x = dst_rect.x - self->rect.x;
	src_rect.y = dst_rect.y - self->rect.y;
	switch(image->mode)
	{
	case IMG_SINGLE:
		if(NULL == image->p.single_img)
		{
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if(image->opacity)
			{
				color = _add_alpha(color, image->opacity);
			}
			gal_fillrect(screen, (GAL_Rect *)&(dst_rect), color);
		}
		else
		{
			GAL_Rect surface_rect = {0};

			surface_rect.x = surface_rect.y = 0;
			surface_rect.w = image->p.single_img->width;
			surface_rect.h = image->p.single_img->height;
			surface = hd_get_surface(&surface_rect, screen->bpp, NULL, GUI_FALSE);
			if(NULL == surface)
			{
				return (1);
			}
			gdi_begin();
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				hd_img_desc_mirror(surface,
						   image->p.single_img,
						   0,
						   0,
						   image->p.single_img->width,
						   image->p.single_img->height,
						   REVERSE_HORIZONTAL);
			} else {
				hd_img_desc(surface,
					    image->p.single_img,
					    0,
					    0,
					    image->p.single_img->width,
					    image->p.single_img->height);
			}
			gdi_end();
			gdi_commit();
			hd_stretch_surface(surface, (GAL_Rect *)&src_rect, screen, (GAL_Rect *)&dst_rect);
			hd_free_surface(surface);
		}
		break;
	case IMG_MULTIPLE:
		valid_rect = self->rect;
		valid_rect.x = valid_rect.y = 0;
		surface = gal_get_surface((GAL_Rect *)(&valid_rect), gui_core->config.bpp);
		if(NULL == surface)
		{
			return (1);
		}
		gdi_begin();
		_draw_multi_img(surface, image, &(valid_rect));
		gdi_end();
		gdi_begin();
		hd_stretch_surface(surface, (GAL_Rect *)&src_rect, screen, (GAL_Rect *)&dst_rect);
		gdi_end();
		hd_free_surface(surface);
		break;
	default:
		break;
	}

	return (0);
}

static int image_event(GuiWidget *self, void *data)
{
	return (0);
}

static int _image_update(GuiImage *image, GUI_Rect *rect)
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

	if(NULL == image->p.single_img)
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
	img_desc.size = img_desc.width * img_desc.height * img_desc.bpp / 8;
	img_desc.pal_index = pal_index;

	gal_img_desc(screen, &img_desc, rect->x, rect->y, rect->w, rect->h);

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
	GuiImage *image = NULL;
	image_desc img_desc = {0};

	if((NULL == self) || (NULL == self))
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
	if(NULL == image)
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
			if ( image->p.com_img.image_t->width == 0)
				break;
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
			if ( image->p.com_img.image_l->height == 0)
				break;
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
			if ( image->p.com_img.image_b->width == 0)
				break;

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
			if ( image->p.com_img.image_r->height == 0)
				break;
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
	img_desc.size = img_desc.width * img_desc.height * img_desc.bpp / 8;
	img_desc.pal_index = gal_get_clut_index();

	gal_img_desc(screen, &img_desc, rect->x, rect->y, rect->w, rect->h);

	GUI_FREE(buffer);
	GUI_FREE(image_data);

	return (0);
}

static int _image_update_rect(GuiWidget *self, GUI_Rect *rect)
{
	GUI_Rect valid_rect = {0};
	GuiImage *image = NULL;

	if((NULL == self) || (NULL == rect))
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
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
	case IMG_SINGLE:
		_image_update(image, &valid_rect);
		break;
	case IMG_MULTIPLE:
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

static bool _check_image_rect(GUI_Rect *rect, image_desc *desc)
{
	if((NULL == rect) || (NULL == desc)) {
		return (GUI_TRUE);
	}

	if((desc->width > rect->w) || (desc->height > rect->h)) {
		return (GUI_FALSE);
	}

	return (GUI_TRUE);
}

static int image_prepare_image(GuiWidget *self)
{
	const char *value = NULL;
	GuiImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
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
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_lt)) {
				gxloge( "[GUI] widget %s -> lt_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_lt));
			}
		}
		value = widget_get_property(self, "rt_img");
		if((NULL != value) || (NULL == image->p.com_img.image_rt))
		{
			image->p.com_img.image_rt = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_rt)) {
				gxloge( "[GUI] widget %s -> rt_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_rt));
			}
		}
		value = widget_get_property(self, "lb_img");
		if((NULL != value) || (NULL == image->p.com_img.image_lb))
		{
			image->p.com_img.image_lb = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_lb)) {
				gxloge( "[GUI] widget %s -> lb_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_lb));
			}
		}
		value = widget_get_property(self, "rb_img");
		if((NULL != value) || (NULL == image->p.com_img.image_rb))
		{
			image->p.com_img.image_rb = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_rb)) {
				gxloge( "[GUI] widget %s -> rb_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_rb));
			}
		}
		value = widget_get_property(self, "l_img");
		if((NULL != value) || (NULL == image->p.com_img.image_l))
		{
			image->p.com_img.image_l = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_l)) {
				gxloge( "[GUI] widget %s -> l_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_l));
			}
		}
		value = widget_get_property(self, "r_img");
		if((NULL != value) || (NULL == image->p.com_img.image_r))
		{
			image->p.com_img.image_r = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_r)) {
				gxloge( "[GUI] widget %s -> r_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_r));
			}
		}
		value = widget_get_property(self, "t_img");
		if((NULL != value) || (NULL == image->p.com_img.image_t))
		{
			image->p.com_img.image_t = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_t)) {
				gxloge( "[GUI] widget %s -> t_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_t));
			}
		}
		value = widget_get_property(self, "b_img");
		if((NULL != value) || (NULL == image->p.com_img.image_b))
		{
			image->p.com_img.image_b = widget_img_load(value);
			if(GUI_FALSE == _check_image_rect(&(self->rect), image->p.com_img.image_b)) {
				gxloge( "[GUI] widget %s -> b_img %s is out of bounds!\n", self->name, value);
				widget_img_clear(&(image->p.com_img.image_b));
			}
		}
		break;
	default:
		break;
	}

	return (0);
}

static int image_clear_image(GuiWidget *self)
{
	GuiImage *image = NULL;

	if(NULL == self)
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
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

static int _covert2rgb(image_desc *img)
{
	int ret = 0;

	ret = GDI_ColorSpaceConvert(img);

	return (ret);
}

static void _save_bmp(FILE* pFile, GuiSurface *surface)
{
	int file_size = 0, offset = 0, dword = 0;
	int i = 0;
	short word = 0;
	char image_tag[2] = {0x42, 0x4D};
	char bit_field[16] = {0x00, 0xF8, 0x00, 0x00, 0xE0, 0x07, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00};
	char *buffer = NULL;

	offset = (((surface->sf.width * 16) + 31) >> 5) << 2;
	file_size = 0x46 + offset * surface->sf.height;

	buffer = GUI_MALLOCZ(offset * surface->sf.height * surface->bpp / 8);
	if(NULL == buffer)
		return;

	/*Write header*/
	fwrite(image_tag, 2, 1, pFile);
	fwrite(&file_size, 4, 1, pFile);
	fwrite(&dword, 4, 1, pFile);
	dword = 0x46;
	fwrite(&dword, 4, 1, pFile);
	dword = 0x38;
	fwrite(&dword, 4, 1, pFile);
	dword = surface->sf.width;
	fwrite(&dword, 4, 1, pFile);
	dword = surface->sf.height;
	fwrite(&dword, 4, 1, pFile);
	word = 1;
	fwrite(&word, 2, 1, pFile);
	word = surface->bpp;
	fwrite(&word, 2, 1, pFile);
	dword = 3;
	fwrite(&dword, 4, 1, pFile);
	dword = offset * surface->sf.height;
	fwrite(&dword, 4, 1, pFile);
	dword = 0;
	fwrite(&dword, 4, 1, pFile);
	fwrite(&dword, 4, 1, pFile);
	fwrite(&dword, 4, 1, pFile);
	fwrite(&dword, 4, 1, pFile);

	fwrite(bit_field, 16, 1, pFile);

	/*Write data*/
	for(i = 0; i < surface->sf.height; i++)
	{
		memcpy((buffer + (surface->sf.height - i - 1) * offset),
				surface->data + i * surface->sf.width * surface->bpp / 8,
				surface->sf.width * surface->bpp / 8);
	}

	fwrite(buffer, offset * surface->sf.height, 1, pFile);

	GUI_FREE(buffer);
}

static int image_set_property(GuiWidget *self, char *name, void *data)
{
	GuiImage *image = NULL;
	GuiWidget *parent = NULL, *sibling = NULL;
	char *pData = NULL, *path = NULL, *id = NULL;
	image_desc *imgdesc = NULL, *oldimage = NULL;
	int value = 0, ret = 0;
	GAL_Rect rect = {0};

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	if(0 == strcasecmp("opacity", name))
	{
		pData = (char*)data;
		value = atoi(pData);
		image->opacity = value;
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("img", name) &&
			(IMG_SINGLE == image->mode))
	{
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
				}
			}
			sibling = widget_get_nextsibling(sibling);
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("update_rect", name))
	{
		image_prepare_image(self);
		_image_update_rect(self, (GUI_Rect *)data);
		image_clear_image(self);
		return (0);
	}
	else if(0 == strcasecmp("file", name))
	{
		path = (char *)data;
		if(NULL == path)
		{
			GUI_FREE(image->filepath);
			return (0);
		}

		id = _get_path_id(path);
		imgdesc = gal_img_load(NULL, path);
		if(JPEG_TYPE == imgdesc->type)
		{
			_covert2rgb(imgdesc);
		}

		imgdesc->type = FILE_TYPE;

		if(IMG_SINGLE == image->mode)
		{
			oldimage = image->p.single_img;
			if(oldimage && (FILE_TYPE == oldimage->type))
			{
				gal_img_release(oldimage);
			}

			image->p.single_img = imgdesc;
		}
		GUI_FREE(image->filepath);
		image->filepath = GUI_STRDUP(path);
	}
	else if(0 == strcasecmp("surface", name))
	{
		image->surface = (GuiSurface *)data;
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("capture", name))
	{
		int v_width = 0, v_height = 0;
		GAL_Rect capture_rect = {0};

		if(NULL == data)
			return (1);

		sscanf((char*)data, "[%d,%d,%d,%d]",
				&capture_rect.x,
				&capture_rect.y,
				&capture_rect.w,
				&capture_rect.h);

		rect.x = rect.y = 0;

		rect.w = (self->rect.w >> 3) << 3;
		rect.h = (self->rect.h >> 3) << 3;
		if(rect.w > 720)
			rect.w = 720;
		if(rect.h > 576)
			rect.h = 576;

		v_width = capture_rect.w;
		v_height = capture_rect.h;

		if(rect.w < (v_width / 4))
		{
			rect.w = v_width / 4;
			//rect.w = (rect.w >> 3) << 3;
		}

		if(rect.h < (v_height / 4))
		{
			rect.h = v_height / 4;
			//rect.h = (rect.h >> 3) << 3;
		}

		if(image->surface)
		{
			hd_free_surface(image->surface);
			image->surface = NULL;
		}
		image->surface = hd_get_surface(&rect, 16, NULL, GUI_FALSE);

		ret = hd_capture(GX_LAYER_VPP, image->surface, &capture_rect);
		if(ret)
		{
			hd_free_surface(image->surface);
			image->surface = NULL;
		}
	}
	else if(0 == strcasecmp("clear", name))
	{
		hd_free_surface(image->surface);
		image->surface = NULL;
	}
	else if(0 == strcasecmp("save_image", name))
	{
		char *filename = (char *)data;
		FILE *pFile = NULL;

		if((NULL == filename) || (NULL == image->surface))
			return (1);

		pFile = fopen(filename, "wb");;
		if(0 == pFile)
			return (1);

		_save_bmp(pFile, image->surface);

		fclose(pFile);
	}
	else
	{
		ret = widget_set_property(self, name, (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (ret);
	}

	widget_set_update(self, GUI_TRUE);

	return (0);
}

static int image_get_property(GuiWidget *self, char *name, void *data)
{
	GuiImage *image = NULL;
	char *string  = NULL;

	if((NULL == self) || (NULL == name) || (NULL == data))
	{
		return (1);
	}

	image = (GuiImage *)(self->object);
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
	else
	{
		return (1);
	}

	return (0);
}

static int image_unactive(GuiWidget *self)
{
	GuiImage *image = NULL;

	if(NULL == self) {
		return (1);
	}

	image = (GuiImage *)(self->object);
	if(NULL == image)
	{
		return (1);
	}

	if(image->surface) {
		gal_free_surface(image->surface);
		image->surface = NULL;
	}

	if(image->map_surface) {
		gal_free_surface(image->map_surface);
		image->map_surface = NULL;
	}

	return (0);
}

GuiWidgetOps image_ops =
{
	.owner	    =	    "image",
	.create	    =	    create_image,
	.release    =	    image_release,
	.prepare_image =    image_prepare_image,
	.draw	    =	    image_draw,
	.update	    =	    image_update,
	.clear_image =	    image_clear_image,
	.unactive    =      image_unactive,
	.event	    =	    image_event,
	.set	    =	    image_set_property,
	.get	    =	    image_get_property
};

