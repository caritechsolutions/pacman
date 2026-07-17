#include "all_widget.h"

int _sliderbar_get_format(GuiWidget *self)
{
	char *pName = NULL;
	int ret = 0;

	if(NULL == self)
	{
		return (HORIZONTAL_NO_SCALE);
	}

	pName = (char *)widget_get_property(self, "format");
	if(NULL == pName)
	{
		return (HORIZONTAL_NO_SCALE);
	}

	if(0 == strcasecmp("horizontal_scale", pName))
	{
		ret = HORIZONTAL_SCALE;
	}
	else if(0 == strcasecmp("vertical_scale", pName))
	{
		ret = VERTICAL_SCALE;
	}
	else if(0 == strcasecmp("horizontal_no_scale", pName))
	{
		ret = HORIZONTAL_NO_SCALE;
	}
	else if(0 == strcasecmp("vertical_no_scale", pName))
	{
		ret = VERTICAL_NO_SCALE;
	}

	return (ret);
}

static int _config_sliderbar_parametre(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	value = widget_get_property(self, "min");
	if((NULL != value) || (0 == sliderbar->min))
	{
		sliderbar->min = widget_get_int(self, "min", 0);
	}

	value = widget_get_property(self, "max");
	if((NULL != value) || (0 == sliderbar->max))
	{
		sliderbar->max = widget_get_int(self, "max", 0);
	}

	value = widget_get_property(self, "value");
	if((NULL != value) || (0 == sliderbar->value))
	{
		sliderbar->value = widget_get_int(self, "value", 0);
	}

	value = widget_get_property(self, "step");
	if((NULL != value) || (0 == sliderbar->step))
	{
		sliderbar->step = widget_get_int(self, "step", 1);
	}
	else
	{
		sliderbar->step = 1;
	}

	if((sliderbar->step > sliderbar->max) || (sliderbar->step < sliderbar->min))
	{
		sliderbar->step = 1;
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == sliderbar->format))
	{
		sliderbar->format = _sliderbar_get_format(self);
	}

	return (0);
}

static void* create_sliderbar(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL, *sliderbar_style = NULL;
	GuiWidget *parent = NULL, *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	sliderbar = (GuiSliderbar *)GUI_MALLOC(sizeof(GuiSliderbar));
	if(NULL == sliderbar)
	{
		return (NULL);
	}

	memset(sliderbar, 0, sizeof(GuiSliderbar));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	sliderbar->base = self;
	self->object = (void *)sliderbar;
	self->state = WIDGET_STATE_FOCUSSABLE;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		sliderbar_style = (GuiSliderbar *)GUI_MALLOC(sizeof(GuiSliderbar));
		if(NULL == sliderbar_style)
		{
			GUI_FREE(sliderbar);
			return (NULL);
		}

		memset(sliderbar_style, 0, sizeof(GuiSliderbar));
		style->object = (void *)sliderbar_style;

		_config_sliderbar_parametre(style);

		sliderbar->base = self;
		memcpy(sliderbar, sliderbar_style, sizeof(GuiSliderbar));

		GUI_FREE(style->object);
	}

	_config_sliderbar_parametre(self);

	if(self->signal)
	{
		sliderbar->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		sliderbar->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		sliderbar->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		sliderbar->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	}

	if(sliderbar->create_event)
	{
		exec_widget_event_data(self, sliderbar->create_event, NULL);
	}

	return (sliderbar);
}

static int sliderbar_release(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(sliderbar->back_surface)
	{
		gal_free_surface(sliderbar->back_surface);
		sliderbar->back_surface = NULL;
	}

	if(sliderbar->fore_surface)
	{
		gal_free_surface(sliderbar->fore_surface);
		sliderbar->fore_surface = NULL;
	}
	
	if(sliderbar->surface) {
		gal_free_surface(sliderbar->surface);
		sliderbar->surface = NULL;
	}

	if(sliderbar->destroy_event)
	{
		exec_widget_event_data(self, sliderbar->destroy_event, NULL);
	}

	GUI_FREE((self->object));

	return (0);
}

static int _draw_multiple_image(void *screen, image_desc * image, GUI_Rect *rect, int flag)
{
	//GUI_Rect valid_rect = {0};
	//int x_pos = 0, y_pos = 0;
	GuiSurface *src_surface = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if((NULL == image) || (NULL == rect))
	{
		return (1);
	}

	dst_rect.x = rect->x;
	dst_rect.y = rect->y;
	dst_rect.w = rect->w;
	dst_rect.h = rect->h;

	src_rect.x = src_rect.y = 0;
	src_rect.w = image->width;
	src_rect.h = image->height;

	src_surface = image_desc_surface(image);
	if(NULL == src_surface) {
		return (1);
	}

	hd_tile_surface(src_surface, &src_rect, screen, &dst_rect);
	hd_add_blit_element(src_surface);

	//valid_rect = *rect;

	/*
	if(flag)
	{
		y_pos = valid_rect.y;
		while((y_pos + image->height) < (valid_rect.y + valid_rect.h))
		{
			gal_img_desc(screen, image, valid_rect.x, y_pos, image->width, image->height);
			y_pos += image->height;
		}

		y_pos = valid_rect.y + valid_rect.h - image->height;
		gal_img_desc(screen, image, valid_rect.x, y_pos, image->width, image->height);
	}
	else
	{
		x_pos = valid_rect.x;
		while((x_pos + image->width) < (valid_rect.x + valid_rect.w))
		{
			gal_img_desc(screen, image, x_pos, valid_rect.y, image->width, image->height);
			x_pos += image->width;
		}

		x_pos = valid_rect.x + valid_rect.w - image->width;
		gal_img_desc(screen, image, x_pos, valid_rect.y, image->width, image->height);
	}
	*/

	return (0);
}

static int _draw_null(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	int color = 0, slot_length = 0, slot_width = 0;
	int delta = 0, cell = 0, x_pos = 0, y_pos = 0;
	GUI_Rect rect = {0}, slot_rect = {0}, scale_rect = {0}, cursor_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	rect = self->rect;
	color = self->back_color[0];
	color = gal_color2index(screen, color);
	gal_fillrect(screen, (GAL_Rect *)(&rect), color);

	if((HORIZONTAL_NO_SCALE == sliderbar->format) ||
			(HORIZONTAL_SCALE == sliderbar->format))
	{
		slot_length = rect.w - (rect.w / 6);
		slot_width = rect.h / 10;
		slot_width = slot_width ? slot_width : 2;

		slot_rect.x = rect.x + ( rect.w - slot_length ) / 2;
		slot_rect.y = rect.y + ( rect.h - slot_width) / 2;
		slot_rect.w = slot_length;
		slot_rect.h = slot_width;
	}
	else if((VERTICAL_NO_SCALE == sliderbar->format) ||
			(VERTICAL_SCALE == sliderbar->format))
	{
		slot_length = rect.h - (rect.h / 6);
		slot_width = rect.w / 10;
		slot_width = slot_width ? slot_width : 2;

		slot_rect.x = rect.x + (rect.w - slot_width) / 2;
		slot_rect.y = rect.y + (rect.h - slot_length) / 2;
		slot_rect.w = slot_width;
		slot_rect.h = slot_length;
	}

	if(sliderbar->back_img[1])
	{
		if((HORIZONTAL_NO_SCALE == sliderbar->format) ||
				(HORIZONTAL_SCALE == sliderbar->format))
		{
			slot_rect.y -= (sliderbar->back_img[1]->height / 2);
			_draw_multiple_image(screen, sliderbar->back_img[1], &slot_rect, 0);
			slot_rect.y += (sliderbar->back_img[1]->height / 2);
		}
		else if((VERTICAL_NO_SCALE == sliderbar->format) ||
				(VERTICAL_SCALE == sliderbar->format))
		{
			slot_rect.x -= (sliderbar->back_img[1]->width / 2);
			_draw_multiple_image(screen, sliderbar->back_img[1], &slot_rect, 1);
			slot_rect.x += (sliderbar->back_img[1]->width / 2);
		}
	}
	else
	{
		color = self->back_color[1];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)(&slot_rect), color);
	}

	/*Scale*/
	delta = sliderbar->max - sliderbar->min;

	if(HORIZONTAL_SCALE == sliderbar->format)
	{
		cell = slot_rect.w / delta;
		cell = cell ? cell : slot_rect.w / 10;

		if(cell)
		{
			x_pos = slot_rect.x;
			while(x_pos < (slot_rect.x + slot_rect.w))
			{
				scale_rect.x = x_pos;
				scale_rect.y = slot_rect.y - 8;
				scale_rect.w = 1;
				scale_rect.h = 8;

				color = self->fore_color[0];
				color = gal_color2index(screen, color);
				gal_fillrect(screen, (GAL_Rect *)(&scale_rect), color);

				x_pos += cell;
			}
		}
	}
	else if(VERTICAL_SCALE == sliderbar->format)
	{
		cell = slot_rect.h / delta;
		cell = cell ? cell : slot_rect.h / 10;

		if(cell)
		{
			y_pos = slot_rect.y;
			while(y_pos < (slot_rect.y + slot_rect.h))
			{
				scale_rect.x = slot_rect.x - 8;
				scale_rect.y = y_pos;
				scale_rect.w = 8;
				scale_rect.h = 1;

				color = self->fore_color[0];
				color = gal_color2index(screen, color);
				gal_fillrect(screen, (GAL_Rect *)(&scale_rect), color);

				y_pos += cell;
			}
		}
	}

	/*Cursor*/
	if((HORIZONTAL_NO_SCALE == sliderbar->format) ||
			(HORIZONTAL_SCALE == sliderbar->format))
	{
		x_pos = (sliderbar->value - sliderbar->min) * slot_rect.w / delta;
		x_pos += slot_rect.x;
		if(sliderbar->cursor_img)
		{
			x_pos -= sliderbar->cursor_img->width / 2;
			y_pos = slot_rect.y - sliderbar->cursor_img->height / 2;

			gal_img_desc(screen,
					sliderbar->cursor_img,
					x_pos,
					y_pos,
					sliderbar->cursor_img->width,
					sliderbar->cursor_img->height);
		}
		else
		{
			x_pos -= 5;
			cursor_rect.h = rect.h - (rect.h / 10);
			cursor_rect.y = rect.y + rect.h / 20;
			cursor_rect.w = 10;
			cursor_rect.x = x_pos;

			color = self->fore_color[1];
			color = gal_color2index(screen, color);
			gal_fillrect(screen, (GAL_Rect *)(&cursor_rect), color);
		}
	}
	else if((VERTICAL_NO_SCALE == sliderbar->format) ||
			(VERTICAL_SCALE == sliderbar->format))
	{
		y_pos = (sliderbar->value - sliderbar->min) * slot_rect.h / delta;
		y_pos = slot_rect.y + slot_rect.h - y_pos;
		if(sliderbar->cursor_img)
		{
			y_pos += sliderbar->cursor_img->height / 2;
			x_pos = slot_rect.x - sliderbar->cursor_img->width / 2;

			gal_img_desc(screen,
					sliderbar->cursor_img,
					x_pos,
					y_pos,
					sliderbar->cursor_img->width,
					sliderbar->cursor_img->height);
		}
		else
		{
			y_pos -= 5;
			cursor_rect.w = rect.w - (rect.w / 10);
			cursor_rect.x = rect.x + (rect.w / 20);
			cursor_rect.h = 10;
			cursor_rect.y = y_pos;

			color = self->fore_color[1];
			color = gal_color2index(screen, color);
			gal_fillrect(screen, (GAL_Rect *)(&cursor_rect), color);
		}
	}

	return (0);
}

static int _check_draw_widget(GuiWidget *self)
{
	GuiWidget *parent = NULL, *sibling = NULL;

	if(NULL == self)
	{
		return (1);
	}

	parent = widget_get_parent(self);
	if(NULL == parent)
	{
		return (1);
	}

	sibling = widget_get_firstchild(parent);
	while(sibling)
	{
		if(0 != strcasecmp(self->name, sibling->name))
		{
			if((self->rect.x == sibling->rect.x) &&
					(self->rect.y == sibling->rect.y) &&
					(self->rect.w == sibling->rect.w) &&
					(self->rect.h == sibling->rect.h))
			{
				if(GUI_FALSE == sibling->need_update)
				{
					widget_set_update(sibling, GUI_TRUE);
				}
				else
				{
					sibling = widget_get_nextsibling(sibling);
					continue;
				}
				widget_draw(sibling);
				widget_set_update(sibling, GUI_FALSE);
			}
		}
		sibling = widget_get_nextsibling(sibling);
	}

	return (0);
}

static int _draw_normal(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiSliderbar *sliderbar = NULL;
	image_desc *back_img = NULL, *fore_img = NULL, *cursor_img = NULL;
	GUI_Rect valid_rect = {0}, src_rect = {0}, dst_rect = {0};
	int value = 0, min = 0, max = 0, cur_pos = 0;
	Color color = 0;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	min = sliderbar->min;
	max = sliderbar->max;
	value = sliderbar->value;

	back_img = sliderbar->back_img[1];
	fore_img = sliderbar->fore_img[1];
	cursor_img = sliderbar->cursor_img;
	if(NULL == cursor_img)
	{
		return (0);
	}

	if((gal_get_gui_transparent() == self->back_color[0]) &&
			(NULL != sliderbar->cursor_img) &&
			(WIDGET_STATE_FOCUSED & self->state))
	{
		_check_draw_widget(self);
		cursor_img = sliderbar->cursor_img;
		value = (sliderbar->value - min) * self->rect.w / (max - min);
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			cur_pos = self->rect.x + self->rect.w - value;
		} else {
			cur_pos = self->rect.x + value;
		}
		if(cur_pos < (self->rect.x + self->rect.w - cursor_img->width))
		{
			gal_img_desc(screen,
					cursor_img,
					cur_pos,
					self->rect.y,
					cursor_img->width,
					cursor_img->height);
		}
		else
		{
			gal_img_desc(screen,
					cursor_img,
					self->rect.x + self->rect.w - cursor_img->width,
					self->rect.y,
					cursor_img->width,
					cursor_img->height);
		}
		return (0);
	}

	valid_rect = self->rect;

	dst_rect = src_rect = self->rect;
	if(NULL == sliderbar->back_surface)
	{
		if(NULL != back_img)
		{
			valid_rect.x = valid_rect.y = 0;
			valid_rect.h = (valid_rect.h < back_img->height) ? (valid_rect.h) : (back_img->height);

			gdi_begin();
			sliderbar->back_surface = gal_get_surface((GAL_Rect *)&valid_rect, gui_core->config.bpp);
			_draw_multiple_image(sliderbar->back_surface,
					back_img,
					(GUI_Rect *)&valid_rect,
					0);
			gdi_end();
		}

		src_rect = dst_rect = valid_rect;
	}

	if(NULL == sliderbar->fore_surface)
	{
		if(NULL != fore_img)
		{
			valid_rect.x = valid_rect.y = 0;
			valid_rect.h = (valid_rect.h < fore_img->height) ? (valid_rect.h) : (fore_img->height);

			gdi_begin();
			sliderbar->fore_surface = gal_get_surface((GAL_Rect *)&valid_rect, gui_core->config.bpp);
			_draw_multiple_image(sliderbar->fore_surface,
					fore_img,
					(GUI_Rect *)&valid_rect,
					0);
			gdi_end();
		}

		src_rect = dst_rect = valid_rect;
	}

	dst_rect = src_rect = self->rect;
	src_rect.x = dst_rect.y = 0;

	dst_rect.x = self->rect.x;
	dst_rect.y = self->rect.y;
	value = (value - min) * valid_rect.w / (max - min);

	if(NULL != back_img)
	{
		src_rect.y = 0;
		gal_stretch_surface(sliderbar->back_surface,
				(GAL_Rect *)&src_rect,
				screen,
				(GAL_Rect *)&dst_rect);
	}
	else
	{
		color = self->back_color[0];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)&dst_rect, color);
	}

	dst_rect.w = value;
	src_rect.w = value;
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		dst_rect.x = dst_rect.x + self->rect.w - dst_rect.w;
	}
	if(NULL != fore_img)
	{
		src_rect.y = 0;
		gal_stretch_surface(sliderbar->fore_surface,
				(GAL_Rect *)&src_rect,
				screen,
				(GAL_Rect *)&dst_rect);
	}
	else
	{
		color = self->back_color[1];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)&dst_rect, color);
	}

	if(WIDGET_STATE_FOCUSED & self->state)
	{
		if((value - cursor_img->width) > 0)
		{
			if((gui_core->config.mirror) && (!self->disable_mirror)) {
				cur_pos = self->rect.x + self->rect.w - src_rect.w;
			} else {
				cur_pos = self->rect.x + src_rect.w - cursor_img->width;
			}
			gal_img_desc(screen,
					cursor_img,
					cur_pos,
					self->rect.y,
					cursor_img->width,
					cursor_img->height);

		}
		else
		{
			if((self->rect.x + src_rect.w - cursor_img->width) > self->rect.x)
			{
				if((gui_core->config.mirror) && (!self->disable_mirror)) {
					cur_pos = self->rect.x + self->rect.w - src_rect.w;
				} else {
					cur_pos = self->rect.x + src_rect.w - cursor_img->width;
				}
				gal_img_desc(screen,
						cursor_img,
						cur_pos,
						self->rect.y,
						cursor_img->width,
						cursor_img->height);
			}
		}
	}

	return (0);
}

static int _draw_water(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiSliderbar *sliderbar = NULL;
	image_desc *back_img[3] = {NULL, NULL, NULL};
	image_desc *fore_img[3] = {NULL, NULL, NULL};
	image_desc *cursor_img = NULL, *tmp_img = NULL;
	GUI_Rect valid_rect = {0}, src_rect = {0}, dst_rect = {0};
	int value = 0;
	int max = 0, min = 0, cur_pos = 0, color = 0, index = 0;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	if(NULL == sliderbar->surface) {
		valid_rect.x = 0;
		valid_rect.y = 0;
		valid_rect.w = self->rect.w;
		valid_rect.h = self->rect.h;

		sliderbar->surface = gal_get_surface((GAL_Rect *)&valid_rect, gui_core->config.bpp);
		if(sliderbar->surface) {
			src_rect = dst_rect = valid_rect;
			src_rect.x = self->rect.x;
			src_rect.y = self->rect.y;
			gdi_commit();
			hd_copy_surface(screen, (GAL_Rect *)&src_rect, sliderbar->surface, (GAL_Rect *)&dst_rect);
		}
	} else {
		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.w = self->rect.w;
		src_rect.h = self->rect.h;
		dst_rect = src_rect;
		dst_rect.x = self->rect.x;
		dst_rect.y = self->rect.y;
		hd_copy_surface(sliderbar->surface, (GAL_Rect *)&src_rect, screen, (GAL_Rect *)&dst_rect);
	}

	back_img[0] = sliderbar->back_img[0];
	back_img[1] = sliderbar->back_img[1];
	back_img[2] = sliderbar->back_img[2];

	if((NULL == back_img[0]) && (NULL == back_img[1]) && (NULL == back_img[2]))
	{
		if((gal_get_gui_transparent() == self->back_color[0]) &&
				(NULL != sliderbar->cursor_img))
		{
			cursor_img = sliderbar->cursor_img;
			value = (sliderbar->value - min) * self->rect.w / (max - min);
			cur_pos = dst_rect.x + value;
			if(cursor_img)
			{
				gal_img_desc(screen,
						cursor_img,
						cur_pos,
						self->rect.y,
						cursor_img->width,
						cursor_img->height);
			}
			return (0);
		}
		else
		{
			return (1);
		}
	}

	fore_img[0] = sliderbar->fore_img[0];
	fore_img[1] = sliderbar->fore_img[1];
	fore_img[2] = sliderbar->fore_img[2];

	if((NULL == fore_img[0]) && (NULL == fore_img[1]) && (NULL == fore_img[2]))
	{
		return (1);
	}

	cursor_img = sliderbar->cursor_img;

	valid_rect = self->rect;
	valid_rect.h = (self->rect.h < back_img[1]->height) ? (self->rect.h) : (back_img[1]->height);

	if(NULL == sliderbar->back_surface)
	{
		valid_rect.x = 0;
		valid_rect.y = 0;
		valid_rect.w -= (back_img[0]->width + back_img[2]->width);

		sliderbar->back_surface = gal_get_surface((GAL_Rect *)&valid_rect, gui_core->config.bpp);

		gdi_begin();
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(sliderbar->back_surface,
				(GAL_Rect*)&valid_rect,
				(unsigned int)color);
		_draw_multiple_image(sliderbar->back_surface,
				back_img[1],
				(GUI_Rect *)&valid_rect,
				0);
		gdi_end();
	}

	valid_rect = self->rect;
	valid_rect.h = (self->rect.h < back_img[1]->height) ? (self->rect.h) : (back_img[1]->height);

	if(NULL == sliderbar->fore_surface)
	{
		valid_rect.x = 0;
		valid_rect.y = 0;
		valid_rect.w -= (fore_img[0]->width + fore_img[2]->width);

		sliderbar->fore_surface = gal_get_surface((GAL_Rect *)&valid_rect, gui_core->config.bpp);

		gdi_begin();
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(sliderbar->fore_surface,
				(GAL_Rect*)&valid_rect,
				(unsigned int)color);
		_draw_multiple_image(sliderbar->fore_surface,
				fore_img[1],
				(GUI_Rect *)&valid_rect,
				0);
		gdi_end();
	}

	valid_rect.w = self->rect.w;

	value = sliderbar->value;
	min = sliderbar->min;
	max = sliderbar->max;

	value = (sliderbar->value - min) * self->rect.w / (max - min);

	if(value < back_img[0]->width)
	{
		gal_img_desc(screen,
				back_img[0],
				self->rect.x,
				self->rect.y,
				back_img[0]->width,
				back_img[0]->height);
	}
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		gal_img_desc_mirror(screen,
				    back_img[2],
				    self->rect.x,
				    self->rect.y,
				    back_img[2]->width,
				    back_img[2]->height,
				    REVERSE_HORIZONTAL);
	} else {
		gal_img_desc(screen,
			     back_img[2],
			     self->rect.x + self->rect.w - back_img[2]->width,
			     self->rect.y,
			     back_img[2]->width,
			     back_img[2]->height);
	}
	src_rect = dst_rect = valid_rect;
	src_rect.x = src_rect.y = 0;
	dst_rect.x = self->rect.x + src_rect.x + back_img[0]->width;
	dst_rect.y = self->rect.y + src_rect.y;
	valid_rect.w = valid_rect.w - (back_img[0]->width + back_img[2]->width);
	src_rect.w = dst_rect.w = valid_rect.w;
	gal_stretch_surface(sliderbar->back_surface,
			(GAL_Rect *)&src_rect,
			screen,
			(GAL_Rect *)&dst_rect);

	src_rect = dst_rect = valid_rect;
	src_rect.x = src_rect.y = 0;
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		dst_rect.x = self->rect.x + self->rect.w - src_rect.x - fore_img[0]->width;
		tmp_img = fore_img[0];
	} else {
		dst_rect.x = self->rect.x + src_rect.x;
		tmp_img = fore_img[0];
	}
	dst_rect.y = self->rect.y + src_rect.y;
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		gal_img_desc_mirror(screen,
				    tmp_img,
				    dst_rect.x,
				    dst_rect.y,
				    tmp_img->width,
				    tmp_img->height,
				    REVERSE_HORIZONTAL);
	} else {
		gal_img_desc(screen,
				tmp_img,
				dst_rect.x,
				dst_rect.y,
				tmp_img->width,
				tmp_img->height);
	}
	src_rect = dst_rect = valid_rect;
	src_rect.x = src_rect.y = 0;

	dst_rect.y = self->rect.y + src_rect.y;
	dst_rect.x = self->rect.x + src_rect.x;
	if((value - fore_img[2]->width) > fore_img[0]->width)
	{
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			cur_pos = dst_rect.x + self->rect.w - value + fore_img[2]->width;
		} else {
			cur_pos = dst_rect.x + value - fore_img[2]->width;
		}
	}
	else
	{
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			cur_pos = dst_rect.x + self->rect.w - fore_img[0]->width - fore_img[2]->width;
		} else {
			cur_pos = dst_rect.x + fore_img[0]->width;
		}
	}

	src_rect.w = dst_rect.w = value - (fore_img[0]->width + fore_img[2]->width);
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		dst_rect.x = self->rect.x + self->rect.w - value + fore_img[2]->width;
	} else {
		dst_rect.x = self->rect.x + fore_img[0]->width;
	}
	if((value - (fore_img[0]->width + fore_img[2]->width)) > 0)
	{
		gal_stretch_surface(sliderbar->fore_surface,
				(GAL_Rect *)&src_rect,
				screen,
				(GAL_Rect *)&dst_rect);
	}

	if(!(WIDGET_STATE_FOCUSED & self->state))
	{
		cursor_img = fore_img[2];
	}

	if(cursor_img)
	{
		gal_img_desc(screen,
				cursor_img,
				cur_pos,
				self->rect.y,
				cursor_img->width,
				cursor_img->height);
	}
	_check_draw_widget(self);

	return (0);
}

static int sliderbar_draw(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	value = widget_get_property(self, "max");
	if((NULL != value) || (0 == sliderbar->max))
	{
		sliderbar->max = widget_get_int(self, "max", 0);
	}

	value = widget_get_property(self, "step");
	if((NULL != value) || (0 == sliderbar->step))
	{
		sliderbar->step = widget_get_int(self, "step", 1);
	}
	else
	{
		sliderbar->step = 1;
	}

	if(SLIDERBAR_NORMAL == sliderbar->mode)
	{
		_draw_normal(self);
	}
	else if(SLIDERBAR_NULL == sliderbar->mode)
	{
		_draw_null(self);
	}
	else if(SLIDERBAR_WATER == sliderbar->mode)
	{
		_draw_water(self);
	}

	return (0);
}

static int _slider_horizontal_key(GuiWidget *self, GUI_Event *event)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiSliderbar *sliderbar = NULL;
	int ret = 0;

	if((NULL == self) || (NULL == event))
	{
		return (EVENT_TRANSFER_STOP);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr callback*/
		if(sliderbar->keypress_event)
		{
			ret = exec_widget_event_data(self,
					sliderbar->keypress_event,
					event);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}

		switch(event->key.sym)
		{
		case GUIK_LEFT:
			sliderbar->value -= sliderbar->step;
			if(sliderbar->value < sliderbar->min)
			{
				sliderbar->value = sliderbar->min;
			}
			else if(sliderbar->value > sliderbar->max)
			{
				sliderbar->value = sliderbar->max;
			}
			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		case GUIK_RIGHT:
			sliderbar->value += sliderbar->step;
			if(sliderbar->value < sliderbar->min)
			{
				sliderbar->value = sliderbar->min;
			}
			else if(sliderbar->value > sliderbar->max)
			{
				sliderbar->value = sliderbar->max;
			}
			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		default:
			if(widget_is_keypress_revert(self)) {
				revert_gui_key(event);
			}
			return (EVENT_TRANSFER_KEEPON);
		}

		break;
	case GUI_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}

	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}
	return (EVENT_TRANSFER_KEEPON);

}

static int _slider_vertical_key(GuiWidget *self, GUI_Event *event)
{
	GuiSliderbar *sliderbar = NULL;
	int ret = 0;

	if((NULL == self) || (NULL == event))
	{
		return (EVENT_TRANSFER_STOP);
	}

	sliderbar = (GuiSliderbar *)(self->object);

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr callback*/
		if(sliderbar->keypress_event)
		{
			ret = exec_widget_event_data(self,
					sliderbar->keypress_event,
					event);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}

		switch(event->key.sym)
		{
		case GUIK_UP:
			sliderbar->value -= sliderbar->step;
			if(sliderbar->value < sliderbar->min)
			{
				sliderbar->value = sliderbar->min;
			}
			else if(sliderbar->value > sliderbar->max)
			{
				sliderbar->value = sliderbar->max;
			}
			widget_set_update(self, GUI_TRUE);
			break;
		case GUIK_DOWN:
			sliderbar->value += sliderbar->step;
			if(sliderbar->value < sliderbar->min)
			{
				sliderbar->value = sliderbar->min;
			}
			else if(sliderbar->value > sliderbar->max)
			{
				sliderbar->value = sliderbar->max;
			}
			widget_set_update(self, GUI_TRUE);
			break;
		default:
			return (EVENT_TRANSFER_KEEPON);
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);

}

static int sliderbar_event(GuiWidget *self, void *data)
{
	GuiSliderbar *sliderbar = NULL;
	GUI_Event *event = NULL;

	WIDGET_CHECK_OBJECT(self, data, sliderbar, GuiSliderbar);

	event = (GUI_Event *)data;

	if((HORIZONTAL_NO_SCALE == sliderbar->format) ||
			(HORIZONTAL_SCALE == sliderbar->format))
	{
		return (_slider_horizontal_key(self, event));
	}
	else if((VERTICAL_NO_SCALE == sliderbar->format) ||
			(VERTICAL_SCALE == sliderbar->format))
	{
		return (_slider_vertical_key(self, event));
	}

	return (0);
}

static int sliderbar_set_property(GuiWidget *self, char *name, void *data)
{
	GuiSliderbar *sliderbar = NULL;
	int value = 0;
	char string[100] = {0};

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(0 == strcasecmp("value", name))
	{
		if(NULL == data)
		{
			return (1);
		}
		value = *(int*)data;

		if((value <= sliderbar->max) &&
				(value >= sliderbar->min))
		{
			sliderbar->value = value;
		}
		else
		{
			return (0);
		}

		sprintf(string, "%d", value);
		widget_set_property(self, name, string);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("update", name))
	{
		sliderbar->old_value = 0;
	}
	else if(0 == strcasecmp("back_image_l", name))
	{
		if(NULL != sliderbar->back_surface)
		{
			gal_free_surface(sliderbar->back_surface);
			sliderbar->back_surface = NULL;
		}

		widget_set_property(self, "back_image_l", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("back_image_m", name))
	{
		if(NULL != sliderbar->back_surface)
		{
			gal_free_surface(sliderbar->back_surface);
			sliderbar->back_surface = NULL;
		}

		widget_set_property(self, "back_image_m", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("back_image_r", name))
	{
		if(NULL != sliderbar->back_surface)
		{
			gal_free_surface(sliderbar->back_surface);
			sliderbar->back_surface = NULL;
		}

		widget_set_property(self, "back_image_r", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("fore_image_l", name))
	{
		if(NULL != sliderbar->fore_surface)
		{
			gal_free_surface(sliderbar->fore_surface);
			sliderbar->fore_surface = NULL;
		}

		widget_set_property(self, "fore_image_l", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("fore_image_m", name))
	{
		if(NULL != sliderbar->fore_surface)
		{
			gal_free_surface(sliderbar->fore_surface);
			sliderbar->fore_surface = NULL;
		}

		widget_set_property(self, "fore_image_m", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("fore_image_r", name))
	{
		if(NULL != sliderbar->fore_surface)
		{
			gal_free_surface(sliderbar->fore_surface);
			sliderbar->fore_surface = NULL;
		}

		widget_set_property(self, "fore_image_r", data);

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("cursor_image", name))
	{
		widget_set_property(self, "cursor_image", data);

		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
		return (0);
	}

	return (0);
}

static int sliderbar_get_property(GuiWidget *self, char *name, void *data)
{
	GuiSliderbar *sliderbar = NULL;

	if((NULL == self) || (NULL == name) || (NULL == data))
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(0 == strcasecmp("min", name))
	{
		*(int*)data = sliderbar->min;
	}
	else if(0 == strcasecmp("max", name))
	{
		*(int*)data = sliderbar->max;
	}
	else if(0 == strcasecmp("step", name))
	{
		*(int*)data = sliderbar->step;
	}
	else if(0 == strcasecmp("value", name))
	{
		*(int*)data = sliderbar->value;
	}
	else if(0 == strcasecmp("update", name))
	{
		sliderbar->old_value = 0;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int sliderbar_prepare_image(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(0 == strcasecmp("water", widget_get_property(self, "format")))
	{
		value = widget_get_property(self, "back_image_l");
		if((NULL != value) || (NULL == sliderbar->back_img[0]))
		{
			sliderbar->back_img[0] = widget_img_load(value);
		}

		value = widget_get_property(self, "back_image_m");
		if((NULL != value) || (NULL == sliderbar->back_img[1]))
		{
			sliderbar->back_img[1] = widget_img_load(value);
		}

		value = widget_get_property(self, "back_image_r");
		if((NULL != value) || (NULL == sliderbar->back_img[2]))
		{
			sliderbar->back_img[2] = widget_img_load(value);
		}

		value = widget_get_property(self, "fore_image_l");
		if((NULL != value) || (NULL == sliderbar->fore_img[0]))
		{
			sliderbar->fore_img[0] = widget_img_load(value);
		}

		value = widget_get_property(self, "fore_image_m");
		if((NULL != value) || (NULL == sliderbar->fore_img[1]))
		{
			sliderbar->fore_img[1] = widget_img_load(value);
		}

		value = widget_get_property(self, "fore_image_r");
		if((NULL != value) || (NULL == sliderbar->fore_img[2]))
		{
			sliderbar->fore_img[2] = widget_img_load(value);
		}
	}
	else if(0 == strcasecmp("normal", widget_get_property(self, "format")))
	{
		value = widget_get_property(self, "back_image");
		if((NULL != value) || (NULL == sliderbar->back_img[1]))
		{
			sliderbar->back_img[1] = widget_img_load(value);
		}

		value = widget_get_property(self, "fore_image");
		if((NULL != value) || (NULL == sliderbar->fore_img[1]))
		{
			sliderbar->fore_img[1] = widget_img_load(value);
		}
	}

	value = widget_get_property(self, "cursor_image");
	if((NULL != value) || (NULL == sliderbar->cursor_img))
	{
		sliderbar->cursor_img = widget_img_load(value);
	}

	if((NULL == sliderbar->back_img[0]) && (NULL == sliderbar->back_img[2]) &&
			(NULL == sliderbar->fore_img[0]) && (NULL == sliderbar->fore_img[2]) &&
			(NULL != sliderbar->cursor_img))
	{
		sliderbar->mode = SLIDERBAR_NORMAL;
	}
	else if((NULL == sliderbar->back_img[0]) && (NULL == sliderbar->back_img[2]) &&
			(NULL == sliderbar->fore_img[0]) && (NULL == sliderbar->fore_img[2]) &&
			(NULL == sliderbar->fore_img[1]))
	{
		sliderbar->mode = SLIDERBAR_NULL;
	}
	else
	{
		sliderbar->mode = SLIDERBAR_WATER;
	}

	return (0);
}

static int sliderbar_clear_image(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(0 == strcasecmp("water", widget_get_property(self, "format")))
	{
		for(i = 0; i < 3; i++)
		{
			if(sliderbar->back_img[i])
			{
				widget_img_clear(&(sliderbar->back_img[i]));
			}

			if(sliderbar->fore_img[i])
			{
				widget_img_clear(&(sliderbar->fore_img[i]));
			}
		}
	}
	else if(0 == strcasecmp("normal", widget_get_property(self, "format")))
	{
		if(sliderbar->back_img[1])
		{
			widget_img_clear(&(sliderbar->back_img[1]));
		}

		if(sliderbar->fore_img[1])
		{
			widget_img_clear(&(sliderbar->fore_img[1]));
		}
	}

	if(sliderbar->cursor_img)
	{
		widget_img_clear(&(sliderbar->cursor_img));
	}

	return (0);
}

static int sliderbar_unactive(GuiWidget *self)
{
	GuiSliderbar *sliderbar = NULL;
	
	if(NULL == self) {
		return (1);
	}

	sliderbar = (GuiSliderbar *)(self->object);
	if(NULL == sliderbar)
	{
		return (1);
	}

	if(sliderbar->back_surface) {
		gal_free_surface(sliderbar->back_surface);
		sliderbar->back_surface = NULL;
	}

	if(sliderbar->fore_surface) {
		gal_free_surface(sliderbar->fore_surface);
		sliderbar->fore_surface = NULL;
	}

	if(sliderbar->surface) {
		gal_free_surface(sliderbar->surface);
		sliderbar->surface = NULL;
	}

	return (0);
}

GuiWidgetOps sliderbar_ops =
{
	.owner = "sliderbar",
	.create = create_sliderbar,
	.release = sliderbar_release,
	.prepare_image = sliderbar_prepare_image,
	.draw = sliderbar_draw,
	.clear_image   = sliderbar_clear_image,
	.unactive = sliderbar_unactive,
	.event = sliderbar_event,
	.set = sliderbar_set_property,
	.get = sliderbar_get_property
};

