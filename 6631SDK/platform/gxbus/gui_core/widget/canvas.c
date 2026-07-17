#include "all_widget.h"

static void* create_canvas(GuiWidget *self)
{
	GuiCanvas *canvas = NULL;
	const char *value = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	int color = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	canvas = (GuiCanvas *)GUI_MALLOCZ(sizeof(GuiCanvas));
	if(NULL == canvas)
	{
		return (NULL);
	}

	self->object = (void *)canvas;
	canvas->base = self;

	canvas->font = (char *)widget_get_property(self, "font");
	value = (const char *)widget_get_property(self, "record");
	if(value && (0 == strcasecmp("enable", value))) {
		GAL_Rect rect = {0};

		if(NULL == canvas->surface) {
			rect.x = rect.y = 0;
			rect.w = self->rect.w;
			rect.h = self->rect.h;
			canvas->surface = hd_get_surface(&rect, gui_core->config.bpp, NULL, GUI_FALSE);
			if(canvas->surface) {
				color = gal_color2index(canvas->surface, self->back_color[0]);
				gal_fillrect(canvas->surface, &rect, color);
			}
		} else {
			return (0);
		}
	}

	//canvas->paint_list = NULL;
	if(self->signal)
	{
		canvas->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		canvas->show_event = (GuiWidgetSignal *)hash_get(self->signal, "show");
		canvas->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
	}

	if(canvas->create_event)
	{
		exec_widget_event_data(self, canvas->create_event, NULL);
	}

	return (canvas);
}

static int canvas_release(GuiWidget *self)
{
	GuiCanvas *canvas = NULL;
	//GuiPaintList *cur_node = NULL;

	if(NULL == self)
	{
		return (1);
	}

	canvas = (GuiCanvas *)(self->object);
	if(NULL == canvas)
	{
		return (1);
	}

	if(canvas->surface) {
		hd_free_surface(canvas->surface);
		canvas->surface = NULL;
	}

	/*cur_node = canvas->paint_list;
	  while(cur_node)
	  {
	  canvas->paint_list = cur_node->next;
	  GUI_FREE(cur_node);
	  gal_img_release(cur_node->image);
	  cur_node = cur_node->next;
	  }*/

	if(canvas->destroy_event)
	{
		exec_widget_event_data(self, canvas->destroy_event, NULL);
	}

	GUI_FREE(self->object);

	return (0);
}

static int canvas_draw(GuiWidget *self)
{
	GuiCanvas *canvas = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	canvas = (GuiCanvas *)(self->object);
	if(NULL == canvas)
	{
		return (1);
	}

	if(canvas->surface) {
		dst_rect.x = self->rect.x;
		dst_rect.y = self->rect.y;
		dst_rect.w = self->rect.w;
		dst_rect.h = self->rect.h;
		src_rect = dst_rect;
		src_rect.x = src_rect.y = 0;
		gal_copy_surface(canvas->surface, &src_rect, screen, &dst_rect);
	}

	if(canvas->show_event)
	{
		exec_widget_event_data(self, canvas->show_event, NULL);
	}

	return (0);
}

static int canvas_event(GuiWidget *self, void *data)
{
	return (0);
}

#if 0
static int _add_paint_element(GuiCanvas *canvas, void *data)
{
	GuiPaintList *new_node = NULL;
	GuiPaintList *paint_node = NULL;
	GuiUpdatePixel *pixel = NULL;
	GuiUpdateRect *rect = NULL;
	GuiUpdateImage *image = NULL;

	if((NULL == canvas) || (NULL == data))
	{
		return (1);
	}

	paint_node = canvas->paint_list;
	new_node = (GuiPaintList *)GUI_MALLOC(sizeof(GuiPaintList));
	memset(new_node, 0, sizeof(GuiPaintList));
	if(NULL == paint_node)
	{
		canvas->paint_list = new_node;
	}
	else
	{
		while(NULL != paint_node->next)
		{
			paint_node = paint_node->next;
		}

		paint_node->next = new_node;
	}

	new_node->type = canvas->type;

	switch(canvas->type)
	{
	case CANVAS_PIXEL:
		pixel = (GuiUpdatePixel *)data;

		new_node->color = pixel->color;
		new_node->update_rect.x = pixel->x;
		new_node->update_rect.y = pixel->y;
		break;
	case CANVAS_RECTANGLE:
		rect = (GuiUpdateRect *)data;

		new_node->color = rect->color;
		new_node->update_rect.x = rect->x;
		new_node->update_rect.y = rect->y;
		new_node->update_rect.w = rect->w;
		new_node->update_rect.h = rect->h;
		break;
	case CANVAS_IMAGE:
		image = (GuiUpdateImage *)data;

		new_node->update_rect.x = image->x;
		new_node->update_rect.y = image->y;
		new_node->image = (image_desc *)widget_img_load(image->name);
		break;
	default:
		new_node->type = CANVAS_NONE;
		break;
	}

	new_node->next = NULL;

	return (0);
}
#endif

static int canvas_set_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiCanvas *canvas = NULL;
	GuiUpdatePixel *pixel = NULL;
	GuiUpdateRect *rect = NULL;
	GuiUpdateImage *image = NULL;
	GuiUpdateImageFile *imagefile = NULL;
	GuiUpdateString *string = NULL;
	image_desc *img_desc = NULL;
	struct gdi_font* old_font = NULL;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	int color = 0, alignment = 0;
	int width  = 0, height = 0, x_offset = 0, y_offset = 0;
	GuiSurface *content_surface = NULL;
	const char *value = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	if(gui_core->stop_update) {
		return (0);
	}

	canvas = (GuiCanvas *)(self->object);
	if(NULL == canvas)
	{
		return (1);
	}

	if(canvas->surface) {
		content_surface = canvas->surface;
		x_offset = self->rect.x;
		y_offset = self->rect.y;
	} else {
		content_surface = screen;
	}

	canvas->font = (char *)widget_get_property(self, "font");

	if(0 == strcasecmp("pixel", name))
	{
		pixel = (GuiUpdatePixel *)data;
		if(NULL == pixel)
		{
			return (1);
		}

		canvas->type = CANVAS_PIXEL;

		if((self->rect.x + pixel->x) > (self->rect.x + self->rect.w))
		{
			return (1);
		}

		if((self->rect.y + pixel->y) > (self->rect.y + self->rect.h))
		{
			return (1);
		}

		if(pixel->color)
		{
			color = gui_get_color((const char *)pixel->color, 0);
		}
		else
		{
			color = pixel->real_color;
		}
		color = gal_color2index(content_surface, color);

		gdi_lock();
		gal_putpixel(content_surface,
				self->rect.x + pixel->x - x_offset,
				self->rect.y + pixel->y - x_offset,
				color);

		gui_core->widget_need_update_count++;
		if(canvas->surface) {
			widget_set_update(self, GUI_TRUE);
		}
		gdi_unlock();

		//_add_paint_element(canvas, (void *)pixel);
	}
	else if(0 == strcasecmp("rectangle", name))
	{
		rect = (GuiUpdateRect *)data;
		if(NULL == rect)
		{
			return (1);
		}

		canvas->type = CANVAS_RECTANGLE;

		if((self->rect.x + rect->x + rect->w) > (self->rect.x + self->rect.w))
		{
			return (1);
		}

		if((self->rect.y + rect->y + rect->h) > (self->rect.y + self->rect.h))
		{
			return (1);
		}

		valid_rect.x = self->rect.x + rect->x;
		valid_rect.y = self->rect.y + rect->y;
		valid_rect.w = rect->w;
		valid_rect.h = rect->h;

		if(rect->color)
		{
			color = gui_get_color((const char *)rect->color, 0);
		}
		else
		{
			color = rect->real_color;
		}
		color = gal_color2index(content_surface, color);

		gdi_lock();
		valid_rect.x -= x_offset;
		valid_rect.y -= y_offset;
		if(canvas->surface) {
			gdi_begin();
		}
		gal_fillrect(content_surface, (GAL_Rect *)(&valid_rect), color);
		if(canvas->surface) {
			gdi_end();
		}

		if(canvas->surface) {
			widget_set_update(self, GUI_TRUE);
		}
		gui_core->widget_need_update_count++;
		gdi_unlock();

		//_add_paint_element(canvas, (void *)rect);
	}
	else if(0 == strcasecmp("image", name))
	{
		image = (GuiUpdateImage *)data;
		if(NULL == image)
		{
			return (1);
		}

		canvas->type = CANVAS_IMAGE;

		if(NULL == image->name)
		{
			return (1);
		}

		img_desc = (image_desc *)widget_img_load(image->name);
		if(NULL == img_desc)
		{
			return (1);
		}

		if((self->rect.x + image->x + img_desc->width) > (self->rect.x + self->rect.w))
		{
			return (1);
		}

		if((self->rect.y + image->y + img_desc->height) > (self->rect.y + self->rect.h))
		{
			return (1);
		}

		gdi_lock();
		if(image->width)
		{
			width = image->width;
		}
		else
		{
			width = img_desc->width;
		}

		if(image->height)
		{
			height = image->height;
		}
		else
		{
			height = img_desc->height;
		}

		if(canvas->surface) {
			gdi_begin();
		}
		gal_img_desc(content_surface,
				img_desc,
				self->rect.x + image->x - x_offset,
				self->rect.y + image->y - y_offset,
				width,
				height);
		if(canvas->surface) {
			gdi_end();
		}

		if(canvas->surface) {
			widget_set_update(self, GUI_TRUE);
		}
		gui_core->widget_need_update_count++;

		widget_img_clear(&img_desc);
		gdi_unlock();
	}
	else if(0 == strcasecmp("image_file", name))
	{
		imagefile = (GuiUpdateImageFile *)data;
		if(NULL == imagefile)
		{
			return (1);
		}

		img_desc = gal_img_load(NULL, imagefile->path);
		if(NULL == img_desc)
		{
			return (1);
		}

		//image_convert2current(img_desc);

		if((self->rect.x + imagefile->x + img_desc->width) > (self->rect.x + self->rect.w))
		{
			gal_img_release(img_desc);
			return (1);
		}

		if((self->rect.y + imagefile->y + img_desc->height) > (self->rect.y + self->rect.h))
		{
			gal_img_release(img_desc);
			return (1);
		}

		gdi_lock();
		if(imagefile->width)
		{
			width = imagefile->width;
		}
		else
		{
			width = img_desc->width;
		}

		if(imagefile->height)
		{
			height = imagefile->height;
		}
		else
		{
			height = img_desc->height;
		}

		if(canvas->surface) {
			gdi_begin();
		}
		gal_img_desc(content_surface,
				img_desc,
				self->rect.x + imagefile->x - x_offset,
				self->rect.y + imagefile->y - y_offset,
				width,
				height);
		if(canvas->surface) {
			gdi_end();
		}

		if(canvas->surface) {
			widget_set_update(self, GUI_TRUE);
		}
		gui_core->widget_need_update_count++;

		gal_img_release(img_desc);
		gdi_unlock();
	}
	else if(0 == strcasecmp("string", name))
	{
		string = (GuiUpdateString *)data;
		if(NULL == string)
		{
			return (1);
		}

		if(string->alignment)
		{
			widget_set_property(self, "alignment", string->alignment);
			alignment = widget_get_alignment(self);
			alignment = widget_alignment_revert(self, string->string, alignment);
		}
		else
		{
			alignment = GUI_TA_TOP | GUI_TA_LEFT;
		}

		canvas->type = CANVAS_STRING;

		valid_rect.x = self->rect.x + string->x - x_offset;
		valid_rect.y = self->rect.y + string->y - y_offset;
		valid_rect.w = string->xsize;
		valid_rect.h = string->ysize;

		string->length = gdi_text_surfacewidth(string->string);
		interval.horizontal = string->inter_char;
		interval.vertical = string->inter_line;

		gdi_lock();
		color = gal_color2index(content_surface, string->color);
		old_font = gxgdi_set_font((char *)canvas->font);
		if(canvas->surface) {
			gdi_begin();
		}
		gxgdi_draw_string(content_surface,
				string->string,
				(GAL_Rect *)&valid_rect,
				&interval,
				alignment,
				color,
				PARAGRAPH_MODE);
		if(canvas->surface) {
			gdi_end();
		}
		gxgdi_set_font((char *)old_font);
		if(canvas->surface) {
			widget_set_update(self, GUI_TRUE);
		}
		gui_core->widget_need_update_count++;
		gdi_unlock();
	}
	else if(0 == strcasecmp("record", name)) {
		GAL_Rect rect = {0};

		value = (const char *)(data);
		if(value && (0 == strcasecmp("enable", value))) {
			if(NULL == canvas->surface) {
				rect.x = rect.y = 0;
				rect.w = self->rect.w;
				rect.h = self->rect.h;
				canvas->surface = hd_get_surface(&rect, gui_core->config.bpp, NULL, GUI_FALSE);
				if(canvas->surface) {
					color = gal_color2index(canvas->surface, self->back_color[0]);
					gal_fillrect(canvas->surface, &rect, color);
				}
			} else {
				return (0);
			}
		}
	}
	else
	{
		canvas->type = CANVAS_NONE;
		widget_set_property(self, name, data);
		return (1);
	}


	return (0);
}

static int canvas_get_property(GuiWidget *self, char *name, void *data)
{
	GuiCanvas *canvas = NULL;
	GuiGetString *string_info = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	canvas = (GuiCanvas *)(self->object);
	if(NULL == canvas)
	{
		return (1);
	}

	if(0 == strcasecmp("string", name))
	{
		string_info = (GuiGetString *)data;
		string_info->height = gdi_get_charheight();
		string_info->width = gdi_text_surfacewidth(string_info->string);
	} else {
		return (1);
	}

	return (0);
}

GuiWidgetOps canvas_ops =
{
	.owner = "canvas",
	.create = create_canvas,
	.release = canvas_release,
	.draw = canvas_draw,
	.event = canvas_event,
	.set = canvas_set_property,
	.get = canvas_get_property
};


