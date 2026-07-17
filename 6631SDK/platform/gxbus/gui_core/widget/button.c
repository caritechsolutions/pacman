#include "all_widget.h"

static int _config_button_parametre(GuiWidget *self)
{
	GuiButton *button = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	button->alignment = widget_get_alignment(self);

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == button->font))
	{
		button->font = (char *)widget_get_property(self, "font");
	}

	return (0);
}

static void* create_button(GuiWidget *self)
{
	GuiButton *button = NULL, *button_style = NULL;
	GuiWidget *parent = NULL, *style = NULL, *launch_widget = NULL;
	const char *launch = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	button = (GuiButton *)GUI_MALLOC(sizeof(GuiButton));
	if(NULL == button)
	{
		return (NULL);
	}

	memset(button, 0, sizeof(GuiButton));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	button->base = self;
	self->object = (void *)button;
	self->state = WIDGET_STATE_FOCUSSABLE;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		button_style = (GuiButton *)GUI_MALLOC(sizeof(GuiButton));
		if(NULL == button_style)
		{
			GUI_FREE(button);
			return (NULL);
		}

		memset(button_style, 0, sizeof(GuiButton));
		style->object = (void *)button_style;

		_config_button_parametre(style);

		memcpy(button, button_style, sizeof(GuiButton));
		button->base = self;

		GUI_FREE(style->object);
	}

	_config_button_parametre(self);

	button->string = (char *)widget_get_property(self, "string");
	button->link = gui_get_widget(NULL, widget_get_property(self, "link"));

	launch = widget_get_property(self, "launch");
	if(launch)
	{
		launch_widget = gui_get_widget(NULL, launch);
		if(launch_widget)
		{
			widget_set_virtual_focus(launch_widget);
		}
	}
	else
	{
		launch_widget = gui_get_widget(NULL, launch);
		if(widget_get_property(launch_widget, "virtual_focus")){
			widget_set_property(launch_widget, "virtual_focus", "disable");
		}
	}

	if(self->signal)
	{
		button->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		button->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		button->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		button->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	}

	if(button->create_event)
	{
		exec_widget_event_data(self, button->create_event, NULL);
	}

	return (button);

}

static int button_release(GuiWidget *self)
{
	GuiButton *button = NULL;

	if(NULL == self)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	if(button->destroy_event)
	{
		exec_widget_event_data(self, button->destroy_event, NULL);
	}

	if(button->string != widget_get_property(self, "string"))
	{
		GUI_FREE(button->string);
	}
	GUI_FREE((self->object));

	return (0);
}

static int button_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiButton *button = NULL;
	GuiSurface *dst_surface = NULL;
	char *string = NULL, *value = NULL;
	GUI_Rect valid_rect = {0}, string_rect = {0};
	GUI_Rect img_rect;
	GAL_Interval interval = {0};
	int Index = 0, color = 0, text_len = 0, lines = 0, flag = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	button->alignment = widget_get_alignment(self);
	Index = widget_get_state_index(self);

	if(NULL != button->image_g[Index][1])
	{
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			widget_img_group_draw_mirror(dst_surface,
						     &(self->rect),
						     button->image_g[Index][0],
						     button->image_g[Index][1],
						     button->image_g[Index][2],
						     self->back_color[Index],
						     REVERSE_HORIZONTAL);
		} else {
			widget_img_group_draw(dst_surface,
					      &(self->rect),
					      button->image_g[Index][0],
					      button->image_g[Index][1],
					      button->image_g[Index][2],
					      self->back_color[Index]);
		}
	}
	else if(NULL != button->image[Index])
	{
		img_rect = self->rect;
		img_rect.w = button->image[Index]->width;
		img_rect.h = button->image[Index]->height;
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			gal_img_desc_mirror(dst_surface,
					    button->image[Index],
					    self->rect.x,
					    self->rect.y,
					    self->rect.w,
					    self->rect.h,
					    REVERSE_HORIZONTAL);
		} else {
			gal_img_desc(dst_surface,
					button->image[Index],
					self->rect.x,
					self->rect.y,
					self->rect.w,
					self->rect.h);
		}
	}
	else
	{
		color = self->back_color[Index];
		color = gal_color2index(dst_surface, color);
		gal_fillrect(dst_surface, (GAL_Rect *)(&(self->rect)), color);
	}

	string = (char *)i18n_get_string((const char *)button->string);

	if(string)
	{
		if(1 == Index)
		{
			value = (char*)widget_get_property(self, "focus_font");
			if(value)
			{
				old_font = gxgdi_set_font((char *)value);
			}
			else
			{
				button->font = (char*)widget_get_property(self, "font");
				old_font = gxgdi_set_font((char *)button->font);
			}
		}
		else
		{
			button->font = (char*)widget_get_property(self, "font");
			old_font = gxgdi_set_font((char *)button->font);
		}

		new_font = gxgdi_get_font();
		if(NULL == new_font)
		{
			return (1);
		}

		text_len = gdi_text_len(string);
		if(check_rect_not_zero(&(self->string_rect)))
		{
			valid_rect = self->string_rect;
			string_rect = self->string_rect;
		}
		else
		{
			valid_rect = self->rect;
			string_rect = self->rect;
		}

		value = (char *)widget_get_property(self, "format");
		if((value) && (0 == strcasecmp("automatic", value))) {
			lines = gdi_text_get_lines((void *)string, (GAL_Rect *)(&valid_rect), &interval);
			flag = PARAGRAPH_MODE;
		} else {
			lines = 1;
			flag = SINGLELINE_MODE;
		}

		button->alignment = widget_alignment_revert(self, string, button->alignment);
		widget_string_alignment(button->alignment, text_len, new_font->ysize * lines, &(string_rect), &(valid_rect));

		color = self->fore_color[Index];
		color = gal_color2index(dst_surface, color);

		gxgdi_draw_string(dst_surface,
				string,
				(GAL_Rect*)(&valid_rect),
				&interval,
				button->alignment,
				color,
				flag);

		gxgdi_set_font((char *)old_font);
	}


	return (0);
}

static int button_update(GuiWidget *self, GUI_Rect *rect)
{
	GuiButton *button = NULL;
	GuiSurface *surface = NULL, *dst_surface = NULL;
	GUI_Rect src_rect = {0}, dst_rect = {0};
	int index = 0, color = 0;

	if((NULL == self) || (NULL == rect))
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

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
	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(dst_surface, color);
	if(button->image[index])
	{
		surface = image_desc_surface(button->image[index]);
		if(NULL == surface)
		{
			return (1);
		}
		gdi_commit();
		hd_copy_surface(surface, (GAL_Rect *)&src_rect, dst_surface, (GAL_Rect *)&dst_rect);
		gdi_commit();
		hd_free_surface(surface);
	}
	else
	{
		gal_fillrect(dst_surface, (GAL_Rect *)&(dst_rect), color);
	}

	return (0);
}

static int button_event(GuiWidget *self, void *data)
{
	// TODO:
	GuiCore *gui_core = GUI_GetCurrent();
	GuiButton *button = NULL;
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;
	const char *key_value = NULL, *launch = NULL;
	unsigned int unicode = 0, accent = 0;
	unsigned char is_iso6937 = 0;

	WIDGET_CHECK_OBJECT(self, data, button, GuiButton);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr callback*/
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		if(button->keypress_event)
		{
			ret = exec_widget_event_data(self, button->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return (ret);
			}

		}

		WIDGET_CHECK_OBJECT(self, data, button, GuiButton);
		/*widget callback*/
		if(GUIK_RETURN == event->key.sym)
		{
			if(button->clicked_event)
			{
				exec_widget_event_data(self, button->clicked_event, NULL);
				WIDGET_CHECK_OBJECT(self, data, button, GuiButton);
			}

			if(button->link)
			{
				GUI_CreateDialog(widget_get_name(button->link));
			}

			launch = widget_get_property(self, "launch");
			if(launch)
			{
				key_value = widget_get_property(self, "virtual_key");
				if(key_value)
				{
					event->type = GUI_KEYDOWN;
					event->key.sym = find_sys_code(key_value);
					if(0xFFFF == event->key.sym)
					{
						gdi_font_utf82unicode((unsigned char **)&key_value, &unicode, &accent, &is_iso6937);
						event->key.sym = unicode;
					}
					event->key.scancode = 0;
					GUI_SendEvent(launch, event);
				}
				else
				{
					GUI_SendEvent(launch, event);
				}
			}
		}
		else
		{
			if(widget_is_keypress_revert(self)) {
				revert_gui_key(event);
			}
			return EVENT_TRANSFER_KEEPON;
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		if(button->clicked_event)
		{
			exec_widget_event_data(self, button->clicked_event, data);
		}
		break;
	default:
		break;
	}

	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}
	return EVENT_TRANSFER_KEEPON;
}

static int button_set_property(GuiWidget *self, char *name, void *data)
{
	GuiButton *button = NULL;
	GuiWidget *parent = NULL;
	char *string = NULL;
	int ret = 0;

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	if((0 == strcasecmp("unfocus_img", name)) ||
			(0 == strcasecmp("focus_img", name)) ||
			(0 == strcasecmp("disable_img", name)))

	{
		ret = widget_set_property(self, name, (const char*)data);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("string", name))
	{
		GxCore_MutexLock(self->widget_mutex);
		string = (char *)data;
		if(button->string != widget_get_property(self, "string"))
		{
			GUI_FREE(button->string);
		}

		if(NULL == string)
		{
			button->string = NULL;
			widget_set_update(self, GUI_TRUE);
			GxCore_MutexUnlock(self->widget_mutex);
			return (1);
		}

		button->string = (char *)GUI_MALLOC(gdi_strlen(string) + 1);
		if(NULL == button->string)
		{
			GxCore_MutexUnlock(self->widget_mutex);
			return (1);
		}
		memset(button->string, 0, gdi_strlen(string) + 1);

		gdi_strcpy(button->string, string);

		//widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);

		parent = widget_get_parent(self);
		if(parent)
		{
			if(0 == strcasecmp("boxitem", parent->classname))
			{
				GUI_SetProperty(parent->name, "update", NULL);
			}
		}

		GxCore_MutexUnlock(self->widget_mutex);
	}
	else
	{
		ret = widget_set_property(self, name, (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (ret);
	}

	return (0);
}

static int button_get_property(GuiWidget *self, char *name, void *data)
{
	GuiButton *button = NULL;
	char *string = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (0);
	}

	if(0 == strcasecmp("string", name))
	{
		string = button->string;
		if(NULL == string)
		{
			return (1);
		}

		*(char**)data = string;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int button_prepare_image(GuiWidget *self)
{
	GuiButton *button = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if((NULL != value) || (NULL == button->image[0]) || (NULL == button->image_g[0][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(button->image_g[0][0]),
								  &(button->image_g[0][1]),
								  &(button->image_g[0][2]));
		}
		else
		{
			button->image[0] = widget_img_load(value);
			button->image_g[0][0] = button->image_g[0][1] = button->image_g[0][2] = NULL;
		}
	}
	value = widget_get_property(self, "focus_img");
	if((NULL != value) || (NULL == button->image[1]) || (NULL == button->image_g[1][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(button->image_g[1][0]),
								  &(button->image_g[1][1]),
								  &(button->image_g[1][2]));
		}
		else
		{
			button->image[1] = widget_img_load(value);
			button->image_g[1][0] = button->image_g[1][1] = button->image_g[1][2] = NULL;
		}
	}
	value = widget_get_property(self, "disable_img");
	if((NULL != value) || (NULL == button->image[2]) || (NULL == button->image_g[2][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(button->image_g[2][0]),
								  &(button->image_g[2][1]),
								  &(button->image_g[2][2]));
		}
		else
		{
			button->image[2] = widget_img_load(value);
			button->image_g[2][0] = button->image_g[2][1] = button->image_g[2][2] = NULL;
		}
	}

	return (0);
}

static int button_clear_image(GuiWidget *self)
{
	GuiButton *button = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	button = (GuiButton *)(self->object);
	if(NULL == button)
	{
		return (1);
	}

	for(i = 0; i < 3; i++) {
		if(button->image[i]) {
			widget_img_clear(&(button->image[i]));
		}

		if((button->image_g[i][0]) ||
		   (button->image_g[i][1]) ||
		   (button->image_g[i][2])) {
			widget_img_group_clear(&(button->image_g[i][0]),
					       &(button->image_g[i][1]),
					       &(button->image_g[i][2]));
		}
	}

	return (0);
}

GuiWidgetOps button_ops =
{
	.owner = "button",
	.create = create_button,
	.release = button_release,
	.prepare_image = button_prepare_image,
	.draw = button_draw,
	.update = button_update,
	.clear_image   = button_clear_image,
	.event = button_event,
	.set = button_set_property,
	.get = button_get_property
};


