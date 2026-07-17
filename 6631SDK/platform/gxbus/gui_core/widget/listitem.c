#include "all_widget.h"

static void *create_listitem(GuiWidget *self)
{
	GuiListItem *listitem = NULL;
	ListItemPara *item_content = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	listitem = (GuiListItem *)GUI_MALLOC(sizeof(GuiListItem));
	if(NULL == listitem)
	{
		return (NULL);
	}

	memset(listitem, 0, sizeof(GuiListItem));

	listitem->base = self;
	self->object = (void *)(listitem);
	self->state = WIDGET_STATE_FOCUSSABLE;

	item_content = (ListItemPara *)GUI_MALLOC(sizeof(ListItemPara));
	if(NULL == item_content)
	{
		return (NULL);
	}
	memset(item_content, 0, sizeof(ListItemPara));
	item_content->string = (char *)widget_get_property(self, "string");
	item_content->x_offset = 0;
	item_content->next = NULL;
	listitem->content = (void *)(item_content);
	listitem->font = (char *)widget_get_property(self, "font");
	listitem->alignment = widget_get_alignment(self);

	listitem->link = gui_get_widget(NULL, widget_get_property(self, "link"));

	return (listitem);
}

static int listitem_release(GuiWidget *self)
{
	GuiListItem *listitem = NULL;

	if(NULL == self)
	{
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem)
	{
		return (1);
	}

	if(listitem->timer)
	{
		remove_timer(listitem->timer);
		listitem->timer = NULL;
		listitem->pos = 0;
	}

	if(listitem->surface)
	{
		gal_free_surface(listitem->surface);
		listitem->surface = NULL;
	}

	GUI_FREE(listitem->content);
	GUI_FREE(self->object);


	return (0);
}

int _listitem_timer_event(void *userdata)
{
	GuiWidget *self = NULL;
	GuiWidget *parent = NULL;
	GuiListItem *listitem = NULL;
	const char *focus_widget = NULL;
	int select = 0;

	self = (GuiWidget *)userdata;
	if(NULL == self)
	{
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem) {
		return (1);
	}

	if(listitem->wait_time < listitem->postpone_time) {
		listitem->wait_time += listitem->time;
		return (0);
	}

	parent = widget_get_parent(self);
	if(NULL == parent)
	{
		return (1);
	}

	focus_widget = GUI_GetFocusWidget();
	if((focus_widget) && (0 != strcasecmp(parent->name, focus_widget))) {
		return (0);
	}

	if(!(self->state & WIDGET_STATE_FOCUSED) || !(parent->state & WIDGET_STATE_FOCUSED))
	{
		GuiWidget *window = NULL;

		if(listitem->postpone_time) {
			window = parent;
			while(WIDGET_ISNOT_WINDOW(window)) {
				window = widget_get_parent(window);
			}
			if(0 != strcasecmp(GUI_GetFocusWindow(), window->name)) {
				return (0);
			}
		}
	}

	if(0 != strcasecmp("listview", parent->classname))
	{
		return (1);
	}

	widget_set_update(self, GUI_TRUE);
	GUI_GetProperty(parent->name, "select", &select);
	GUI_SetProperty(parent->name, "update_row", &select);
	parent->need_update = 1;

	return (0);
}

static void _longstring_convert(char *src, char *dst, int length)
{
	char *pstr = src, *prev = NULL;
	unsigned char is_accent = 0;
	unsigned int unicode = 0, accent = 0;
	int width = 0, suffix_len = 0;

	suffix_len = gdi_text_len((void *)"...");

	while(*pstr) {
		prev = pstr;
		gdi_font_utf82unicode((unsigned char **)&pstr, &unicode, &accent, &is_accent);
		if((width + suffix_len) >= length) {
			pstr = prev;
			break;
		}
		width += gdi_get_charwidth(unicode);
	}

	strncpy(dst, src, pstr - src - 1);
	strcat(dst, "...");
}

static int _process_ext_surface(GuiWidget *self, ExternSurface *extsurface, GAL_Rect *rect)
{
	int ret = 0;
	GuiSurface *surface;
	GAL_Rect inner_rect = {0}, src_rect = {0}, dst_rect = {0};

	if(NULL == extsurface->surface) {
		return (1);
	}

	surface = extsurface->surface;
	inner_rect = extsurface->rect;

	src_rect.x = src_rect.y = 0;
	src_rect.w = surface->sf.width;
	src_rect.h = surface->sf.height;

	dst_rect.x = rect->x + inner_rect.x;
	dst_rect.y = rect->y + inner_rect.y;
	dst_rect.w = inner_rect.w;
	dst_rect.h = inner_rect.h;

	gdi_begin();
	ret = hd_stretch_surface(surface, &src_rect, screen, &dst_rect);
	hd_add_blit_element(NULL);
	gdi_end();

	return (ret);
}

void _convert_relative_coordinate(GuiWidget *parent, GUI_Rect *rect)
{
	GuiWidget *widget = NULL;
	int item_height = 0, header_height = 0, interval_height = 0;

	interval_height = widget_get_int(parent, "interval", 0);
	widget = widget_get_firstchild(parent);
	while(widget) {
		if(0 == strcasecmp("listitem", widget->classname)) {
			item_height = widget->rect.h + interval_height;
			break;
		} else if(0 == strcasecmp("header", widget->classname)) {
			header_height = widget->rect.h;
		}
		widget = widget_get_nextsibling(widget);
	}
	rect->x = rect->x - parent->rect.x;
	rect->y = rect->y - parent->rect.y + item_height - header_height;
}

static int listitem_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiListItem *listitem = NULL;
	GuiWidget *parent = NULL, *header = NULL;
	int Index = 0, color = 0, list_color = 0, i = 0, flag = 0;
	ListItemPara *item_array = NULL;
	GuiSurface *list_surface = NULL;
	Header_Para head_para = {0};
	GUI_Rect valid_rect = {0}, relative_rect = {0};
	GAL_Interval interval = {0};
	int text_len = 0, img_xsize = 0, img_ysize = 0, alignment = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	image_desc *pimg_desc = NULL;
	char *string = NULL, *value = NULL;
	char tmp_str[200] = {0};

	if(NULL == self)
	{
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	value = (char *)widget_get_property(self, "arrange");
	if((NULL != value) && (0 == strcasecmp("right_to_left", value))) {
		listitem->arrange_right = GUI_TRUE;
	}

	parent = widget_get_parent(self);
	if(parent)
	{
		parent->ops->get(parent, "get_head", &header);
		GUI_GetProperty(parent->name, "surface", &list_surface);
	}

	Index = widget_get_state_index(self);
	if(!(parent->state & WIDGET_STATE_FOCUSED))
	{
		Index = 0;
	}

	list_color = color = self->back_color[Index];
	if(0 == (0xFF000000 & list_color)) {
		list_color |= 0xFF000000;
	}
	if(color == gui_core->config.gui_trans)
	{
		if(NULL == listitem->surface)
		{
			gdi_commit();
			valid_rect = self->rect;
			valid_rect.x = valid_rect.y = 0;
			listitem->surface = gal_get_surface((GAL_Rect * )&valid_rect,
					gui_core->config.bpp);

			if(list_surface) {
				relative_rect = self->rect;
				_convert_relative_coordinate(parent, &relative_rect);
				gal_copy_surface(list_surface,
						 (GAL_Rect *)&(relative_rect),
						 listitem->surface,
						 (GAL_Rect *)&valid_rect);
			} else {
				gal_copy_surface(screen,
						(GAL_Rect *)&(self->rect),
						listitem->surface,
						(GAL_Rect *)&valid_rect);
			}
		}
		else
		{
			valid_rect = self->rect;
			valid_rect.x = valid_rect.y = 0;

			if(NULL == listitem->timer)
			{
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					gal_copy_surface(listitem->surface,
							(GAL_Rect *)&valid_rect,
							list_surface,
							(GAL_Rect *)&(relative_rect));
				} else {
					gal_copy_surface(listitem->surface,
							(GAL_Rect *)&valid_rect,
							screen,
							(GAL_Rect *)&(self->rect));
				}
			}
		}
	}
	if(!(self->state & WIDGET_STATE_FOCUSED) || !(parent->state & WIDGET_STATE_FOCUSED))
	{
		GuiWidget *window = NULL;
		if(listitem->timer)
		{
			remove_timer(listitem->timer);
			listitem->timer = NULL;
			listitem->pos = 0;
			listitem->wait_time = 0;
		}

		if(listitem->postpone_time) {
			window = parent;
			while(WIDGET_ISNOT_WINDOW(window)) {
				window = widget_get_parent(window);
			}
			if(0 != strcasecmp(GUI_GetFocusWindow(), window->name)) {
				return (0);
			}
		}
	}

	if((listitem->active_img) ||
	   (listitem->active_img_g[0]) ||
	   (listitem->active_img_g[1]) ||
	   (listitem->active_img_g[2]))
	{
		if((listitem->active_img_g[0]) ||
		   (listitem->active_img_g[1]) ||
		   (listitem->active_img_g[2]))
		{
			if(!listitem->arrange_right) {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						widget_img_group_draw_mirror(list_surface,
									     &(relative_rect),
									     listitem->active_img_g[0],
									     listitem->active_img_g[1],
									     listitem->active_img_g[2],
									     listitem->active_color,
									     REVERSE_HORIZONTAL);
					} else {
						widget_img_group_draw(list_surface,
								      &(relative_rect),
								      listitem->active_img_g[0],
								      listitem->active_img_g[1],
								      listitem->active_img_g[2],
								      listitem->active_color);
					}
				} else {
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						widget_img_group_draw_mirror(screen,
									     &(self->rect),
									     listitem->active_img_g[0],
									     listitem->active_img_g[1],
									     listitem->active_img_g[2],
									     listitem->active_color,
									     REVERSE_HORIZONTAL);
					} else {
						widget_img_group_draw(screen,
								      &(self->rect),
								      listitem->active_img_g[0],
								      listitem->active_img_g[1],
								      listitem->active_img_g[2],
								      listitem->active_color);
					}
				}
			} else {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					widget_img_group_draw_mirror(list_surface,
								     &(relative_rect),
								     listitem->active_img_g[0],
								     listitem->active_img_g[1],
								     listitem->active_img_g[2],
								     listitem->active_color,
								     REVERSE_HORIZONTAL);
				} else {
					widget_img_group_draw_mirror(screen,
								&(self->rect),
								listitem->active_img_g[0],
								listitem->active_img_g[1],
								listitem->active_img_g[2],
								listitem->active_color,
								REVERSE_HORIZONTAL);
				}
			}
		}
		else
		{
			if(!listitem->arrange_right) {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						gal_img_desc_mirror(list_surface,
								    listitem->active_img,
								    relative_rect.x,
								    relative_rect.y,
								    relative_rect.w,
								    relative_rect.h,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(list_surface,
							listitem->active_img,
							relative_rect.x,
							relative_rect.y,
							relative_rect.w,
							relative_rect.h);
					}
				} else {
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						gal_img_desc_mirror(screen,
								    listitem->active_img,
								    self->rect.x,
								    self->rect.y,
								    self->rect.w,
								    self->rect.h,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(screen,
							listitem->active_img,
							self->rect.x,
							self->rect.y,
							self->rect.w,
							self->rect.h);
					}
				}
			} else {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					gal_img_desc_mirror(list_surface,
							    listitem->active_img,
							    relative_rect.x,
							    relative_rect.y,
							    relative_rect.w,
							    relative_rect.h,
							    REVERSE_HORIZONTAL);
				} else {
					gal_img_desc_mirror(screen,
							listitem->active_img,
							self->rect.x,
							self->rect.y,
							self->rect.w,
							self->rect.h,
							REVERSE_HORIZONTAL);
				}
			}
		}
	}
	else if((listitem->image[Index]) ||
		(listitem->image_g[Index][0]) ||
		(listitem->image_g[Index][1]) ||
		(listitem->image_g[Index][2]))
	{
		if((listitem->image_g[Index][0]) ||
		   (listitem->image_g[Index][1]) ||
		   (listitem->image_g[Index][2]))
		{
			if(!listitem->arrange_right) {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						widget_img_group_draw_mirror(list_surface,
									     &(relative_rect),
									     listitem->image_g[Index][0],
									     listitem->image_g[Index][1],
									     listitem->image_g[Index][2],
									     self->back_color[Index],
									     REVERSE_HORIZONTAL);
					} else {
						widget_img_group_draw(list_surface,
								      &(relative_rect),
								      listitem->image_g[Index][0],
								      listitem->image_g[Index][1],
								      listitem->image_g[Index][2],
								      self->back_color[Index]);
					}
				} else {
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						widget_img_group_draw_mirror(screen,
									     &(self->rect),
									     listitem->image_g[Index][0],
									     listitem->image_g[Index][1],
									     listitem->image_g[Index][2],
									     self->back_color[Index],
									     REVERSE_HORIZONTAL);
					} else {
						widget_img_group_draw(screen,
								      &(self->rect),
								      listitem->image_g[Index][0],
								      listitem->image_g[Index][1],
								      listitem->image_g[Index][2],
								      self->back_color[Index]);
					}
				}
			} else {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					widget_img_group_draw_mirror(list_surface,
								     &(relative_rect),
								     listitem->image_g[Index][0],
								     listitem->image_g[Index][1],
								     listitem->image_g[Index][2],
								     self->back_color[Index],
								     REVERSE_HORIZONTAL);
				} else {
					widget_img_group_draw_mirror(screen,
								&(self->rect),
								listitem->image_g[Index][0],
								listitem->image_g[Index][1],
								listitem->image_g[Index][2],
								self->back_color[Index],
								REVERSE_HORIZONTAL);
				}
			}
		}
		else
		{
			if(!listitem->arrange_right) {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						gal_img_desc_mirror(list_surface,
								    listitem->image[Index],
								    relative_rect.x,
								    relative_rect.y,
								    relative_rect.w,
								    relative_rect.h,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(list_surface,
							listitem->image[Index],
							relative_rect.x,
							relative_rect.y,
							relative_rect.w,
							relative_rect.h);
					}
				} else {
					if((gui_core->config.mirror) && (self->disable_mirror)) {
						gal_img_desc_mirror(screen,
								    listitem->image[Index],
								    self->rect.x,
								    self->rect.y,
								    self->rect.w,
								    self->rect.h,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(screen,
							listitem->image[Index],
							self->rect.x,
							self->rect.y,
							self->rect.w,
							self->rect.h);
					}
				}
			} else {
				if(list_surface) {
					relative_rect = self->rect;
					_convert_relative_coordinate(parent, &relative_rect);
					gal_img_desc_mirror(list_surface,
							    listitem->image[Index],
							    relative_rect.x,
							    relative_rect.y,
							    relative_rect.w,
							    relative_rect.h,
							    REVERSE_HORIZONTAL);
				} else {
					gal_img_desc_mirror(screen,
							listitem->image[Index],
							self->rect.x,
							self->rect.y,
							self->rect.w,
							self->rect.h,
							REVERSE_HORIZONTAL);
				}
			}
		}
	}
	else
	{
		item_array = (ListItemPara *)(listitem->content);
		if(item_array->back_color)
		{
			if('#' == item_array->back_color[0])
			{
				color = hexToInt(item_array->back_color + 1);
			}
			else
			{
				color = gui_get_color(item_array->back_color, 0);
			}
		}
		else
		{
			color = self->back_color[Index];
		}
		list_color = color;
		if(gui_core->config.bpp != 32) {
			list_color |= 0xFF000000;
		}
		color = gal_color2index(screen, color);
		if(list_surface) {
			relative_rect = self->rect;
			_convert_relative_coordinate(parent, &relative_rect);
			list_color = gal_color2index(list_surface, list_color);
			hd_fillrect(list_surface, (GAL_Rect *)&(relative_rect), list_color);
		} else {
			gal_fillrect(screen, (GAL_Rect *)&(self->rect), color);
		}
	}

	item_array = (ListItemPara *)(listitem->content);
	/*Draw Text*/
	if(item_array->fore_color)
	{
		if('#' == item_array->fore_color[0])
		{
			color = hexToInt(item_array->fore_color + 1);
		}
		else
		{
			color = gui_get_color(item_array->fore_color, 0);
		}
	}
	else
	{
		color = self->fore_color[Index];
	}
	list_color = color;
	if(gui_core->config.bpp != 32) {
		list_color |= 0xFF000000;
	}
	color = gal_color2index(screen, color);

	while(item_array)
	{
		if(header)
		{
			head_para.index = i;
			header->ops->get(header, "get_xoff", &head_para);
		}
		if(item_array->image)
		{
			valid_rect = self->rect;
			valid_rect.x  += head_para.xPos;
			valid_rect.w = head_para.xSize;

			pimg_desc = widget_img_load(item_array->image);
			if(pimg_desc)
			{
				img_xsize = pimg_desc->width;
				img_ysize = pimg_desc->height;

				head_para.alignment = 0xFFFFFFFF;

				if(header)
				{
					header->ops->get(header, "alignment", &head_para);
					header->ops->get(header, "format", &head_para);
				}

				if(0xFFFFFFFF != head_para.alignment)
				{
					alignment = head_para.alignment;
				}
				else
				{
					alignment = listitem->alignment;
				}

				widget_string_alignment(alignment,
						img_xsize, img_ysize,
						&(valid_rect),
						&(valid_rect));

				if(item_array->zoom) {
					img_xsize = valid_rect.w - item_array->x_offset;
					img_ysize = valid_rect.h - item_array->y_offset;
				} else {
					img_xsize = pimg_desc->width;
					img_ysize = pimg_desc->height;
				}

				if((32 == pimg_desc->bpp) && (item_array->zoom)) {
					gdi_commit();
					gdi_begin();
				}
				if(list_surface) {
					relative_rect = valid_rect;
					_convert_relative_coordinate(parent, &relative_rect);
					if((head_para.disable_mirror) && (gui_core->config.mirror)) {
						gal_img_desc_mirror(list_surface,
								    pimg_desc,
								    relative_rect.x + item_array->x_offset,
								    relative_rect.y,
								    img_xsize,
								    img_ysize,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(list_surface,
							     pimg_desc,
							     relative_rect.x + item_array->x_offset,
							     relative_rect.y,
							     img_xsize,
							     img_ysize);
					}
				} else {
					if((head_para.disable_mirror) && (gui_core->config.mirror)) {
						gal_img_desc_mirror(screen,
								    pimg_desc,
								    valid_rect.x + item_array->x_offset,
								    valid_rect.y,
								    img_xsize,
								    img_ysize,
								    REVERSE_HORIZONTAL);
					} else {
						gal_img_desc(screen,
							     pimg_desc,
							     valid_rect.x + item_array->x_offset,
							     valid_rect.y,
							     img_xsize,
							     img_ysize);
					}
				}
				if((32 == pimg_desc->bpp) && (item_array->zoom)) {
					gdi_end();
				}

				widget_img_clear(&pimg_desc);
			}
		}
		else
		{
			valid_rect = self->rect;
			valid_rect.x  += head_para.xPos;
			valid_rect.w = head_para.xSize;

			listitem->font = (char *)widget_get_property(self, "font");
			if(item_array->font)
			{
				old_font = gxgdi_set_font((char *)item_array->font);
			}
			else
			{
				old_font = gxgdi_set_font((char *)listitem->font);
			}

			new_font = gxgdi_get_font();
			if(NULL == new_font)
			{
				return (1);
			}

			if(header)
			{
				head_para.index = i;
				header->ops->get(header, "get_xoff", &head_para);
			}

			if(listitem->i18n)
			{
				string = (char *)i18n_get_string(item_array->string);
			}
			else
			{
				string = item_array->string;
			}

			text_len = gdi_text_len(string);

			head_para.alignment = 0xFFFFFFFF;
			if(header)
			{
				header->ops->get(header, "alignment", &head_para);
				header->ops->get(header, "format", &head_para);
			}
			if(0xFFFFFFFF != head_para.alignment)
			{
				alignment = head_para.alignment;
			}
			else
			{
				alignment = listitem->alignment;
			}
			alignment = widget_alignment_revert(self, string, alignment);

			widget_string_alignment(alignment,
					text_len,
					gdi_get_charheight(),
					&valid_rect,
					&(valid_rect));

			if(listitem->active_color)
			{
				list_color = listitem->active_color;
				if(gui_core->config.bpp != 32) {
					list_color |= 0xFF000000;
				}
				color = gal_color2index(screen, listitem->active_color);
			}

			// TODO: Text rolling
			if((text_len > valid_rect.w) && (Index) && (listitem->enable_roll))
			{
				if(NULL == listitem->timer)
				{
					if(listitem->time == 0)
					{
						/*The lowest litmit timer's time out value*/
						value = (char *)widget_get_property(parent, "roll_time");
						if(value)
						{
							listitem->time = atoi(value);
						}

						if(0 == listitem->time)
						{
							listitem->time = 400;
						}
					}
					listitem->wait_time = 0;
					listitem->timer = create_timer(_listitem_timer_event,
							listitem->time,
							self,
							TIMER_REPEAT);
				}
				if(gui_core->config.enable_double_buffer)
				{
					gdi_commit();
				}

				if((listitem->postpone_time) &&
				   (listitem->wait_time < listitem->postpone_time)) {
					_longstring_convert(string, tmp_str, valid_rect.w);
					string = tmp_str;
				}

				if(list_surface) {
					relative_rect = valid_rect;
					_convert_relative_coordinate(parent, &relative_rect);
					gdi_begin();
					gdi_roll_string(list_surface,
							string,
							(GAL_Rect *)(&relative_rect),
							listitem->pos,
							list_color,
							SINGLELINE_MODE);
					gdi_end();
				} else {
					gdi_roll_string(screen,
							string,
							(GAL_Rect *)(&valid_rect),
							listitem->pos,
							color,
							SINGLELINE_MODE);
				}

				if(listitem->wait_time >= listitem->postpone_time) {
					value = (char *)widget_get_property(self, "step");
					if(NULL == value)
					{
						listitem->pos += 2;
					}
					else
					{
						listitem->pos += widget_get_int(self, "step", 2);
					}
					if((text_len - listitem->pos + (valid_rect.w / 3)) <= valid_rect.w)
					{
						listitem->pos = 0;
						gdi_commit();
					}
				}
			}
			else
			{
				if((listitem->timer) && (string) && (!listitem->postpone_time))
				{
					remove_timer(listitem->timer);
					listitem->timer = NULL;
					//listitem->pos = 0;
				}

				if((listitem->postpone_time) &&
				   (listitem->wait_time < listitem->postpone_time) &&
				   (gdi_text_len(string) > valid_rect.w)) {
					_longstring_convert(string, tmp_str, valid_rect.w);
					string = tmp_str;
					valid_rect = self->rect;
					valid_rect.x  += head_para.xPos;
					valid_rect.w = head_para.xSize;
					text_len = gdi_text_len(string);
					alignment = widget_alignment_revert(self, string, alignment);
					widget_string_alignment(alignment,
								text_len,
								gdi_get_charheight(),
								&valid_rect,
								&(valid_rect));
				}
				if((alignment & 0x3) == GUI_TA_RIGHT) {
					valid_rect.x -= item_array->x_offset;
				} else {
					valid_rect.x += item_array->x_offset;
					valid_rect.w -= item_array->x_offset;
				}

				if(item_array->y_offset) {
					valid_rect.y += item_array->y_offset;
					valid_rect.h -= item_array->y_offset;
				}
				
				if(head_para.string_wrap) {
					flag = PARAGRAPH_MODE;
				} else {
					flag = SINGLELINE_MODE;
				}
				if(list_surface) {
					relative_rect = valid_rect;
					_convert_relative_coordinate(parent, &relative_rect);
					gxgdi_draw_string(list_surface,
							string,
							(GAL_Rect*)(&relative_rect),
							&interval,
							listitem->alignment,
							list_color,
							SINGLELINE_MODE);
				} else {
					gxgdi_draw_string(screen,
							string,
							(GAL_Rect*)(&valid_rect),
							&interval,
							listitem->alignment,
							color,
							SINGLELINE_MODE);
				}
				if((text_len > valid_rect.w) &&
						(NULL != widget_get_property(parent, "enable_roll")) &&
						(0 == strcasecmp("true", widget_get_property(parent, "enable_roll"))))
				{
					listitem->enable_roll = GUI_TRUE;
					value = (char *)widget_get_property(parent, "roll_time");
					if(value)
					{
						listitem->time = atoi(value);
					}
				}
			}
			gxgdi_set_font((char *)old_font);
		}

		if(item_array->ext_surface) {
			GAL_Rect tmp_rect = {0};

			head_para.index = i;
			header->ops->get(header, "get_xoff", &head_para);

			tmp_rect.x = self->rect.x;
			tmp_rect.y = self->rect.y;
			tmp_rect.w = self->rect.w;
			tmp_rect.h = self->rect.h;
			tmp_rect.x  += head_para.xPos;
			tmp_rect.w = head_para.xSize;

			_process_ext_surface(self, item_array->ext_surface, &tmp_rect);
		}

		i++;
		item_array = item_array->next;
	}


	return (0);
}

static int listitem_event(GuiWidget *self, void *data)
{
	GuiListItem *item = NULL;

	if(NULL == self)
	{
		return (EVENT_TRANSFER_KEEPON);
	}


	item = (GuiListItem *)(self->object);
	if(NULL == item)
	{
		return (EVENT_TRANSFER_KEEPON);
	}

	/*To Item in listview there is only on event which is SDLK_RETURN*/
	if(item->link)
	{
		gxlogd("**********Name = %s**********\n", widget_get_name(item->link));
		GUI_CreateDialog(widget_get_name(item->link));
		return (EVENT_TRANSFER_STOP);
	}
	else
	{
		return (EVENT_TRANSFER_KEEPON);
	}

	return (0);
}

static int listitem_set_property(GuiWidget *self, char *name, void *data)
{
	GuiListItem *listitem = NULL;
	char *str = NULL;

	if(NULL == self)
	{
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem)
	{
		return (1);
	}

	if(0 == strcasecmp("content", name) && (listitem->content))
	{
		memcpy(listitem->content ,data, sizeof(ListItemPara));
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("enable_roll", name))
	{
		listitem->enable_roll = *(int *)data;
	}
	else if(0 == strcasecmp("roll_time", name))
	{
		listitem->time = *(int *)data;
	}
	else if(0 == strcasecmp("postpone_time", name)) {
		listitem->postpone_time = *(int *)data;
	}
	else if(0 == strcasecmp("stop_rolling", name))
	{
		if(listitem->timer)
		{
			remove_timer(listitem->timer);
			listitem->timer = NULL;
		}
		listitem->pos = 0;
		listitem->wait_time = 0;
	}
	else if(0 == strcasecmp("i18n", name))
	{
		str = (char *)data;
		if(0 == strcasecmp("true", str))
		{
			listitem->i18n = GUI_TRUE;
		}
		else
		{
			listitem->i18n = GUI_FALSE;
		}
	}
	else if(0 == strcasecmp("mirror", name))
	{
		str = (char *)data;
		if(str && 0 == strcasecmp("disable", str)) {
			self->disable_mirror = GUI_TRUE;
		} else {
			self->disable_mirror = GUI_FALSE;
		}
	}
	else
	{
		widget_set_property(self, name, data);
		return (0);
	}


	return (0);
}

static int listitem_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

static int listitem_prepare_image(GuiWidget *self)
{
	GuiListItem *listitem = NULL;
	const char *value = NULL;

	if(NULL == self) {
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem) {
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_load(value,
					      &(listitem->image_g[0][0]),
					      &(listitem->image_g[0][1]),
					      &(listitem->image_g[0][2]));
			listitem->image[0] = NULL;
		} else {
			listitem->image[0] = widget_img_load(value);
			listitem->image_g[0][0] = listitem->image_g[0][1] = listitem->image_g[0][2] = NULL;
		}
	}

	value = widget_get_property(self, "focus_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_load(value,
					      &(listitem->image_g[1][0]),
					      &(listitem->image_g[1][1]),
					      &(listitem->image_g[1][2]));
			listitem->image[1] = NULL;
		} else {
			listitem->image[1] = widget_img_load(value);
			listitem->image_g[1][0] = listitem->image_g[1][1] = listitem->image_g[1][2] = NULL;
		}
	}

	value = widget_get_property(self, "disable_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_load(value,
					      &(listitem->image_g[2][0]),
					      &(listitem->image_g[2][1]),
					      &(listitem->image_g[2][2]));
			listitem->image[2] = NULL;
		} else {
			listitem->image[2] = widget_img_load(value);
			listitem->image_g[2][0] = listitem->image_g[2][1] = listitem->image_g[2][2] = NULL;
		}
	}

	return (0);
}

static int listitem_clear_image(GuiWidget *self)
{
	GuiListItem *listitem = NULL;
	const char *value = NULL;

	if(NULL == self) {
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem) {
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_clear(&(listitem->image_g[0][0]),
					       &(listitem->image_g[0][1]),
					       &(listitem->image_g[0][2]));
			listitem->image_g[0][0] = listitem->image_g[0][1] = listitem->image_g[0][2] = NULL;
		} else {
			widget_img_clear(&(listitem->image[0]));
			listitem->image[0] = NULL;
		}
	}

	value = widget_get_property(self, "focus_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_clear(&(listitem->image_g[1][0]),
					       &(listitem->image_g[1][1]),
					       &(listitem->image_g[1][2]));
			listitem->image_g[1][0] = listitem->image_g[1][1] = listitem->image_g[1][2] = NULL;
		} else {
			widget_img_clear(&(listitem->image[1]));
			listitem->image[1] = NULL;
		}
	}

	value = widget_get_property(self, "disable_img");
	if(NULL != value) {
		if(widget_check_img_mode(value)) {
			widget_img_group_clear(&(listitem->image_g[2][0]),
					       &(listitem->image_g[2][1]),
					       &(listitem->image_g[2][2]));
			listitem->image_g[2][0] = listitem->image_g[2][1] = listitem->image_g[2][2] = NULL;
		} else {
			widget_img_clear(&(listitem->image[2]));
			listitem->image[2] = NULL;
		}
	}
	return (0);
}

static int listiem_unactive(GuiWidget *self)
{
	GuiListItem *listitem = NULL;

	if(NULL == self) {
		return (1);
	}

	listitem = (GuiListItem *)(self->object);
	if(NULL == listitem) {
		return (1);
	}

	if(listitem->surface) {
		gal_free_surface(listitem->surface);
		listitem->surface = NULL;
	}

	return (0);
}

GuiWidgetOps listitem_ops =
{
	.owner = "listitem",
	.create = create_listitem,
	.release = listitem_release,
	.prepare_image = listitem_prepare_image,
	.draw = listitem_draw,
	.clear_image = listitem_clear_image,
	.unactive = listiem_unactive,
	.event = listitem_event,
	.set = listitem_set_property,
	.get = listitem_get_property
};


