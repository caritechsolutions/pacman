#include "all_widget.h"

int _get_scrollbar_format(GuiWidget *widget)
{
	char *format = NULL;

	if(NULL == widget)
	{
		return (SCROLL_SHOW);
	}

	format = (char *)widget_get_property(widget, "format");
	if(NULL == format)
	{
		return (SCROLL_SHOW);
	}

	if(0 == strcasecmp("scroll_show", format))
	{
		return (SCROLL_SHOW);
	}
	else if(0 == strcasecmp("scroll_hide", format))
	{
		return (SCROLL_HIDE);
	}
	else
	{
		return (SCROLL_HIDE);
	}
}

static int _config_scrollbar_parametre(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == scrollbar->format))
	{
		scrollbar->format = _get_scrollbar_format(self);
	}

	return (0);
}

static void *create_scrollbar(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL, *scrollbar_style = NULL;
	GuiWidget *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	scrollbar = (GuiScrollbar *)GUI_MALLOC(sizeof(GuiScrollbar));
	if(NULL == scrollbar)
	{
		return (NULL);
	}

	memset(scrollbar, 0, sizeof(GuiScrollbar));

	self->object = (void *)(scrollbar);
	scrollbar->base = self;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		scrollbar_style = (GuiScrollbar *)GUI_MALLOC(sizeof(GuiScrollbar));
		if(NULL == scrollbar_style)
		{
			GUI_FREE(scrollbar);
			return (NULL);
		}

		memset(scrollbar_style, 0, sizeof(GuiScrollbar));

		style->object = (void *)scrollbar_style;

		_config_scrollbar_parametre(style);

		memcpy(scrollbar, scrollbar_style, sizeof(GuiScrollbar));

		GUI_FREE(scrollbar_style);
	}

	_config_scrollbar_parametre(self);

	return (scrollbar);
}

static int scrollbar_release(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL;

	if(NULL == self)
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	GUI_FREE(self->object);
	return (0);
}

static int scrollbar_draw(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL;
	GuiWidget *parent = NULL;
	int cur_line = 0, total_num = 0, visible_line = 0;
	image_desc *back_img = NULL;
	GuiSurface *tile_surface = NULL;
	int index = 0, color = 0, bar_height = 0, y_pos = 0, scale_bar = 0;
	int img_height = 0, arrow_height = 0;
	GUI_Rect valid_rect = {0};
	GuiSurface *surface = NULL;

	if(NULL == self)
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	parent = widget_get_parent(self);
	if(NULL == parent)
	{
		return (1);
	}

	if(0 == strcasecmp("listview", parent->classname))
	{
		GUI_GetProperty(parent->name, "select", &cur_line);
		if(cur_line < 0)
		{
			cur_line = 0;
		}
		GUI_GetProperty(parent->name, "total_num", &total_num);
		GUI_GetProperty(parent->name, "visible_row", &visible_line);
	}
	else if(0 == strcasecmp("notepad", parent->classname))
	{
		GUI_GetProperty(parent->name, "cur_pos", &cur_line);
		GUI_GetProperty(parent->name, "total_byte", &total_num);
		visible_line = cur_line;
	}
	else if(0 == strcasecmp("table", parent->classname))
	{
		GUI_GetProperty(parent->name, "cur_line", &cur_line);
		GUI_GetProperty(parent->name, "total_lines", &total_num);
		GUI_GetProperty(parent->name, "visible_lines", &visible_line);
	}


	valid_rect = self->rect;

	if(SCROLL_SHOW == scrollbar->format)
	{

		/*Draw Scrollbar according to listview or notepad*/
		if(scrollbar->back_img)
		{
			GAL_Rect src_rect = {0}, dst_rect = {0};

			back_img = scrollbar->back_img;
			tile_surface = image_desc_surface(back_img);
			src_rect.x = src_rect.y = 0;
			src_rect.w = tile_surface->sf.width;
			src_rect.h = tile_surface->sf.height;
			dst_rect.x = valid_rect.x;
			dst_rect.y = valid_rect.y;
			dst_rect.w = tile_surface->sf.width;
			dst_rect.h = valid_rect.h;
			hd_tile_surface(tile_surface, &src_rect, screen, &dst_rect);
			hd_add_blit_element(tile_surface);
		}
		else
		{
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);

			gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);
		}

		if(scrollbar->arrow_img[0] && scrollbar->arrow_img[1])
		{
			gal_img_desc(screen,
					scrollbar->arrow_img[0],
					valid_rect.x,
					valid_rect.y,
					scrollbar->arrow_img[0]->width,
					scrollbar->arrow_img[0]->height);

			img_height = scrollbar->arrow_img[1]->height;
			valid_rect.y = valid_rect.y + valid_rect.h - img_height;
			gal_img_desc(screen,
					scrollbar->arrow_img[1],
					valid_rect.x,
					valid_rect.y,
					scrollbar->arrow_img[1]->width,
					scrollbar->arrow_img[1]->height);
		}
		else
		{
			index = widget_get_state_index(self);
			color = self->fore_color[index];
			color = gal_color2index(screen, color);

			valid_rect = self->rect;
			valid_rect.h = valid_rect.w;

			gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);

			valid_rect = self->rect;
			valid_rect.y = valid_rect.y + valid_rect.h - valid_rect.w;
			valid_rect.h = valid_rect.w;

			arrow_height = valid_rect.h;

			gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);
		}

		/*Draw Roll bar*/
		if((scrollbar->fore_img[0]) &&
		   (scrollbar->fore_img[1]) &&
		   (scrollbar->fore_img[2])) {
			GAL_Rect src_rect = {0}, dst_rect = {0};

			valid_rect = self->rect;
			if(scrollbar->arrow_img[0]) {
				img_height = scrollbar->arrow_img[0]->height;
			} else {
				img_height = valid_rect.w;
			}
			valid_rect.y += img_height;
			valid_rect.h -= img_height;

			if(visible_line < total_num) {
				bar_height = (visible_line * valid_rect.h) / total_num;
			} else {
				bar_height = (total_num * valid_rect.h) / visible_line;
			}

			if((scrollbar->fore_img[0]->height + scrollbar->fore_img[1]->height) > bar_height) {
				bar_height = scrollbar->fore_img[0]->height + scrollbar->fore_img[1]->height;
			} else {
				;//bar_height -= (scrollbar->fore_img[0]->height + scrollbar->fore_img[1]->height);
			}
			if(bar_height > valid_rect.h) {
				bar_height = valid_rect.h;
			}
			valid_rect.h = bar_height;
			if(total_num > 1)
			{
				if(cur_line <= total_num -1)
					valid_rect.y += (self->rect.h - bar_height - 2 * img_height) *
						cur_line / (total_num - 1);
				else
					valid_rect.y += self->rect.h - bar_height - 2 * img_height;
			}

			if((valid_rect.y + bar_height) > (self->rect.y + self->rect.h)) {
				valid_rect.y = self->rect.y;
				bar_height = self->rect.h;
			}

			gal_img_desc(screen,
				     scrollbar->fore_img[0],
				     valid_rect.x,
				     valid_rect.y,
				     scrollbar->fore_img[0]->width,
				     scrollbar->fore_img[0]->height);

			gal_img_desc(screen,
				     scrollbar->fore_img[2],
				     valid_rect.x,
				     valid_rect.y + bar_height - scrollbar->fore_img[2]->height,
				     scrollbar->fore_img[2]->width,
				     scrollbar->fore_img[2]->height);

			surface = image_desc_surface(scrollbar->fore_img[1]);
			if(surface) {
				src_rect.x = src_rect.y = 0;
				src_rect.w = scrollbar->fore_img[1]->width;
				src_rect.h = bar_height - scrollbar->fore_img[0]->height - scrollbar->fore_img[2]->height;
				dst_rect = src_rect;
				dst_rect.x = valid_rect.x;
				dst_rect.y = valid_rect.y + scrollbar->fore_img[0]->height;
				src_rect.h = surface->sf.height;
				hd_tile_surface(surface, &src_rect, screen, &dst_rect);
				hd_free_surface(surface);
			}

		} else if(scrollbar->fore_img[1]) {
			valid_rect = self->rect;
			if(scrollbar->arrow_img[0])
			{
				img_height = scrollbar->arrow_img[0]->height;
			}
			else
			{
				img_height = valid_rect.w;
			}
			valid_rect.y += img_height;
			valid_rect.h -= img_height;

			if(scale_bar) {
				if(visible_line < total_num) {
					bar_height = (visible_line * valid_rect.h) / total_num;
				} else {
					bar_height = valid_rect.h;//(total_num * valid_rect.h) / visible_line;
				}
			} else {
				bar_height = scrollbar->fore_img[1]->height;
			}

			valid_rect.h = bar_height;

			if(total_num > 1)
			{
				if(cur_line <= total_num -1)
					valid_rect.y += (self->rect.h - bar_height - 2 * img_height) *
						cur_line / (total_num - 1);
				else
					valid_rect.y += self->rect.h - bar_height - 2 * img_height;
			}

			gal_img_desc(screen,
					scrollbar->fore_img[1],
					valid_rect.x,
					valid_rect.y,
					scrollbar->fore_img[1]->width,
					bar_height);
		} else {
			index = widget_get_state_index(self);
			color = self->fore_color[index];
			color = gal_color2index(screen, color);

			valid_rect = self->rect;
			valid_rect.y += valid_rect.w;
			valid_rect.h -= valid_rect.w;
			if(total_num && (total_num > visible_line) && (total_num > 1))
			{
				bar_height = valid_rect.h * visible_line / (total_num - 1);
			}

			valid_rect.h = bar_height;

			if(total_num > 1)
			{
				valid_rect.y += (self->rect.h -
						bar_height -
						2 * self->rect.w) *
					cur_line /
					(total_num - 1);
				/*change total num*/
				if((valid_rect.y + valid_rect.h) > (self->rect.y + self->rect.h))
				{
					valid_rect.y = self->rect.y + self->rect.h - arrow_height - bar_height;
				}
			}

			gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);
		}
	}
	return (0);
}

static int scrollbar_event(GuiWidget *self, void *data)
{
	return (0);
}

static int scrollbar_set_property(GuiWidget *self, char *name, void *data)
{
	GuiScrollbar *scrollbar = NULL;
	char *string = NULL;
	int ret = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	if(0 == strcasecmp("format", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		string = (char *)data;
		widget_set_property(self, (const char*)name, (const char*)string);
		scrollbar->format = _get_scrollbar_format(self);

		if(0 == strcasecmp("scroll_hide", (const char*)string))
		{
			widget_set_update(self->parent, GUI_TRUE);
		}
		else
		{
			widget_set_update(self, GUI_TRUE);
		}
	}
	else
	{
		ret = widget_set_property(self, name, (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (ret);
	}

	return (0);
}

static int scrollbar_get_property(GuiWidget *self, char *name, void *data)
{
	return (0);
}

static int scrollbar_prepare_image(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	value = widget_get_property(self, "arrow_up");
	if((NULL != value) || (NULL == scrollbar->arrow_img[0]))
	{
		scrollbar->arrow_img[0] = widget_img_load(value);
	}
	value = widget_get_property(self, "arrow_down");
	if((NULL != value) || (NULL == scrollbar->arrow_img[1]))
	{
		scrollbar->arrow_img[1] = widget_img_load(value);
	}

	value = widget_get_property(self, "back_img");
	if((NULL != value) || (NULL == scrollbar->back_img))
	{
		scrollbar->back_img = widget_img_load(value);
	}

	value = widget_get_property(self, "fore_img");
	if((NULL != value) && widget_check_img_mode(value)) {
		widget_img_group_load(value,
				      &(scrollbar->fore_img[0]),
				      &(scrollbar->fore_img[1]),
				      &(scrollbar->fore_img[2]));
	} else {
		if((NULL != value) || (NULL == scrollbar->fore_img[1]))
		{
			scrollbar->fore_img[1] = widget_img_load(value);
		}
	}
	return (0);
}

static int scrollbar_clear_image(GuiWidget *self)
{
	GuiScrollbar *scrollbar = NULL;

	if(NULL == self)
	{
		return (1);
	}

	scrollbar = (GuiScrollbar *)(self->object);
	if(NULL == scrollbar)
	{
		return (1);
	}

	if(scrollbar->arrow_img[0])
	{
		widget_img_clear(&(scrollbar->arrow_img[0]));
	}

	if(scrollbar->arrow_img[1])
	{
		widget_img_clear(&(scrollbar->arrow_img[1]));
	}

	if(scrollbar->back_img)
	{
		widget_img_clear(&(scrollbar->back_img));
	}

	if(scrollbar->fore_img[0] &&
	   scrollbar->fore_img[1] &&
	   scrollbar->fore_img[2]) {
		widget_img_group_clear(&(scrollbar->fore_img[0]),
				       &(scrollbar->fore_img[1]),
				       &(scrollbar->fore_img[1]));
	} else if(scrollbar->fore_img[1]){
		widget_img_clear(&(scrollbar->fore_img[1]));
	}

	return (0);
}

GuiWidgetOps scrollbar_ops =
{
	.owner = "scrollbar",
	.create = create_scrollbar,
	.release = scrollbar_release,
	.prepare_image = scrollbar_prepare_image,
	.draw = scrollbar_draw,
	.clear_image   = scrollbar_clear_image,
	.event = scrollbar_event,
	.set = scrollbar_set_property,
	.get = scrollbar_get_property
};




