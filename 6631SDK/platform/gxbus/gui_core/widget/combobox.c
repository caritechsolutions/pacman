#include "all_widget.h"
#include <assert.h>

static int _check_string(char *str)
{
	char *p = NULL;

	if(NULL == str)
	{
		return (1);
	}

	p = str;

	while('[' != *p)
	{
		p++;
	}

	while(*p)
	{
		if((']' == *p) && ('\0' == *(p + 1)))
		{
			return (0);
		}
		p++;
	}

	return (1);
}

static int _get_item_content(GuiWidget *self, char ***content)
{
	char *value = NULL, *pstr = NULL;
	char **result_content;
	int i = 0, index = 0, count = 0;

	if((NULL == self) || (NULL == content))
	{
		return (0);
	}

	value = (char *)widget_get_property(self, "content");
	if(NULL == value)
	{
		return (0);
	}

	if(_check_string(value))
	{
		assert("The value which pass to combobox is incorrect\n");
		return (0);
	}

	pstr = value;
	while(*pstr)
	{
		if(',' == *pstr)
		{
			count++;
		}
		pstr++;
	}

	result_content = (char**)GUI_MALLOC((count + 1) * sizeof(void*));
	if(result_content)
	{
		memset(result_content, 0, (count + 1) * sizeof(void*));
	}

	while('[' != *value)
	{
		value++;
	}

	value++;

	while((']'  != *value) && ('\0' != *(value + 1)))
	{
		pstr = value;
		i = 0;
		while(',' != *pstr)
		{
			if((']' == *pstr) && ('\0' == *(pstr + 1)))
			{
				break;
			}
			i++;
			pstr++;
		}

		result_content[index] = (char *)GUI_MALLOC(i + 1);
		strncpy(result_content[index], value, i);
		result_content[index][i] = '\0';
		index++;
		if(index > count) {
			break;
		}

		if((']' == *pstr) && ('\0' == *(pstr + 1)))
		{
			value = pstr;
		}
		else
		{
			value = pstr + 1;
		}
	}

	*content = result_content;

	return (index);
}

static int _get_item_from_string(char *value, char ***content)
{
	char * pstr = NULL;
	char **result_content;
	int i = 0, index = 0, count = 0;

	if((NULL == value) || (NULL == content))
	{
		return (1);
	}

	if(_check_string(value))
	{
		assert("The value which pass to combobox is incorrect\n");
		return (1);
	}

	pstr = value;
	while(*pstr)
	{
		if(',' == *pstr) {
			count++;
		} else if('\\' == *pstr) {
			pstr++;
		}
		pstr++;
	}

	result_content = (char**)GUI_MALLOC((count + 1) * sizeof(void*));
	memset(result_content, 0, (count + 1) * sizeof(void*));

	while('[' != *value)
	{
		value++;
	}

	value++;

	while((']' != *value) && ('\0' != *(value + 1)))
	{
		pstr = value;
		i = 0;
		while(',' != *pstr)
		{
			if(']' == *pstr) {
				if('\0' != *(pstr + 1))
				{
					i++;
					pstr++;
				}
				break;
			} else if('\\' == *pstr) {
				i++;
				pstr++;
			}
			i++;
			pstr++;
		}

		result_content[index] = (char *)GUI_MALLOC(i + 1);
		memset(result_content[index], 0, i + 1);
		strncpy(result_content[index], value, i);
		result_content[index][i] = '\0';
		gxgdi_str_del_char(result_content[index], '\\');
		index++;
		if(index > count) {
			break;
		}

		if((']' == *pstr) && ('\0' == *(pstr + 1)))
		{
			value = pstr;
		}
		else
		{
			value = pstr + 1;
		}
	}

	*content = result_content;

	return (index);
}

static int _free_content(GuiWidget *self)
{
	char **result_content = NULL;
	GuiComboBox *combobox = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	result_content = combobox->content;
	if(NULL == result_content)
	{
		return (1);
	}

	while(combobox->content[i] != NULL && i < combobox->total)
	{
		GUI_FREE(combobox->content[i]);
		i++;
	}

	GUI_FREE(combobox->content);

	return (0);
}

static int _config_combobox_parametre(GuiWidget *self)
{
	GuiComboBox *combobox = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == combobox->font))
	{
		combobox->font = (char *)value;
	}

	return (0);
}

static void *create_combobox(GuiWidget *self)
{
	GuiComboBox *combobox = NULL, *combobox_style = NULL;
	GuiWidget *parent = NULL, *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	combobox = (GuiComboBox *)GUI_MALLOC(sizeof(GuiComboBox));
	if(NULL == combobox)
	{
		return (NULL);
	}

	memset(combobox, 0, sizeof(GuiComboBox));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	combobox->base = self;
	self->object = (void *)combobox;
	self->state = WIDGET_STATE_FOCUSSABLE;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		combobox_style = (GuiComboBox *)GUI_MALLOC(sizeof(GuiComboBox));
		if(NULL == combobox_style)
		{
			GUI_FREE(combobox);
			return (NULL);
		}

		memset(combobox_style, 0, sizeof(GuiComboBox));
		style->object = (void *)combobox_style;

		_config_combobox_parametre(style);

		memcpy(combobox, combobox_style, sizeof(GuiComboBox));
		combobox->base = self;

		GUI_FREE(style->object);
	}

	_config_combobox_parametre(self);

	combobox->cur_sel = widget_get_int(self, "select", 0);

	combobox->total = _get_item_content(self, &(combobox->content));

	if(combobox->cur_sel > combobox->total - 1)
	{
		combobox->cur_sel = combobox->total - 1;
	}

	if(combobox->cur_sel < 0)
	{
		combobox->cur_sel = 0;
	}

	if(self->signal)
	{
		combobox->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		combobox->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		combobox->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
		combobox->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		combobox->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
	}

	if(combobox->create_event)
	{
		exec_widget_event_data(self, combobox->create_event, NULL);
	}


	return (combobox);
}

static int combobox_release(GuiWidget *self)
{
	GuiComboBox *combobox = NULL;
	//int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	_free_content(self);

	if(combobox->destroy_event)
	{
		exec_widget_event_data(self, combobox->destroy_event, NULL);
	}

	if(combobox->surface)
	{
		hd_free_surface(combobox->surface);
		combobox->surface = NULL;
	}

	GUI_FREE((self->object));

	return (0);
}

static int combobox_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiComboBox *combobox = NULL;
	GuiSurface *dst_surface = NULL;
	int index = 0, color = 0, str_len = 0, height = 0, flag = 0;
	char *string = NULL;
	GUI_Rect valid_rect = {0}, img_rect = {0};
	GAL_Interval interval = {0};
	struct gdi_font* old_font = NULL, *new_font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	index = widget_get_state_index(self);
	if(NULL != combobox->image_g[index][1])
	{
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			widget_img_group_draw_mirror(dst_surface,
						     &(self->rect),
						     combobox->image_g[index][0],
						     combobox->image_g[index][1],
						     combobox->image_g[index][2],
						     self->back_color[index],
						     REVERSE_HORIZONTAL);
		} else {
			widget_img_group_draw(dst_surface,
					      &(self->rect),
					      combobox->image_g[index][0],
					      combobox->image_g[index][1],
					      combobox->image_g[index][2],
					      self->back_color[index]);
		}
	}
	else if(combobox->image[index])
	{
		img_rect = self->rect;
		img_rect.w = combobox->image[index]->width;
		img_rect.h = combobox->image[index]->height;
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			gal_img_desc_mirror(dst_surface,
					    combobox->image[index],
					    self->rect.x,
					    self->rect.y,
					    self->rect.w,
					    self->rect.h,
					    REVERSE_HORIZONTAL);
		} else {
			gal_img_desc(dst_surface,
					combobox->image[index],
					self->rect.x,
					self->rect.y,
					self->rect.w,
					self->rect.h);
		}
	}
	else
	{
		color = self->back_color[index];
		if(color == gui.config.gui_trans &&
		   (0 != strcasecmp("boxitem", self->parent->classname)))
		{
			if((NULL == combobox->surface) || (combobox->update_surface))
			{
				if(combobox->surface)
				{
					hd_free_surface(combobox->surface);
					combobox->surface = NULL;
					combobox->update_surface = GUI_FALSE;
				}
				combobox->surface = widget_get_surface(&(self->rect));
			}
			else
			{
				GAL_Rect src_rect = {0}, dst_rect = {0};

				dst_rect.x = self->rect.x;
				dst_rect.y = self->rect.y;
				dst_rect.w = self->rect.w;
				dst_rect.h = self->rect.h;
				src_rect = dst_rect;
				src_rect.x = src_rect.y = 0;
				hd_copy_surface(combobox->surface, &src_rect, dst_surface, &dst_rect);
			}
		}
		else
		{
			color = gal_color2index(dst_surface, color);
			gal_fillrect(dst_surface, (GAL_Rect *)(&(self->rect)), color);
		}
	}

	if(combobox->left_arrow[index])
	{
		if(combobox->image[index])
		{
			height = combobox->image[index]->height;
		}
		else
		{
			height = self->rect.h;
		}

		gal_img_desc(dst_surface,
				combobox->left_arrow[index],
				self->rect.x + 5,
				self->rect.y +
				(height -
				 combobox->left_arrow[index]->height) / 2,
				combobox->left_arrow[index]->width,
				combobox->left_arrow[index]->height);
	}

	if(combobox->right_arrow[index])
	{
		if(combobox->image[index])
		{
			height = combobox->image[index]->height;
		}
		else
		{
			height = self->rect.h;
		}

		gal_img_desc(dst_surface,
				combobox->right_arrow[index],
				self->rect.x + self->rect.w - combobox->right_arrow[index]->width - 5,
				self->rect.y +
				(height -
				 combobox->right_arrow[index]->height) / 2,
				combobox->right_arrow[index]->width,
				combobox->right_arrow[index]->height);
	}

	combobox->font = (char*)widget_get_property(self, "font");

	old_font = gxgdi_set_font((char *)combobox->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	color = self->fore_color[index];
	color = gal_color2index(dst_surface, color);

	if(combobox->cur_sel < 0)
	{
		combobox->cur_sel = 0;
	}
	index = combobox->cur_sel;

	if(NULL == combobox->content)
	{
		gxgdi_set_font((char *)old_font);
		return (1);
	}

	string = (char *)i18n_get_string((const char *)combobox->content[index]);

	valid_rect = self->rect;
	str_len = (combobox->total) ? (gdi_text_surfacewidth(string)) : (0);

	index = widget_get_state_index(self);

	if(combobox->left_arrow[0])
	{
		valid_rect.x += (combobox->left_arrow[0]->width + 5);
		valid_rect.w -= (combobox->right_arrow[0]->width + 5);
	}
	else if(combobox->left_arrow[1])
	{
		valid_rect.x += (combobox->left_arrow[1]->width + 5);
		valid_rect.w -= (combobox->right_arrow[1]->width + 5);
	}
	else if(combobox->left_arrow[2])
	{
		valid_rect.x += (combobox->left_arrow[2]->width + 5);
		valid_rect.w -= (combobox->right_arrow[2]->width + 5);
	}


	if(combobox->right_arrow[0])
	{
		valid_rect.w -= (combobox->right_arrow[0]->width + 5);
	}
	else if(combobox->right_arrow[1])
	{
		valid_rect.w -= (combobox->right_arrow[1]->width + 5);
	}
	else if(combobox->right_arrow[2])
	{
		valid_rect.w -= (combobox->right_arrow[2]->width + 5);
	}

	if(str_len < valid_rect.w)
	{
		valid_rect.x += (valid_rect.w - str_len) / 2;
		valid_rect.w = (valid_rect.w - (valid_rect.w - str_len) / 2);
		flag = GUI_TA_LEFT | GUI_TA_VCENTRE;
	} else {
		flag = GUI_TA_HCENTRE | GUI_TA_VCENTRE;
	}

	if(new_font)
	{
		valid_rect.y += (self->rect.h - new_font->ysize) / 2;
	}

	if(combobox->total)
	{
		gxgdi_draw_string(dst_surface,
				string,
				(GAL_Rect*)(&valid_rect),
				&interval,
				flag,
				color,
				SINGLELINE_MODE);
	}

	gxgdi_set_font((char *)old_font);

	return (0);
}




static int combobox_event(GuiWidget *self, void *data)
{
	GuiCore *gui_core = NULL;
	GuiComboBox *combobox = NULL;
	GUI_Event *event = NULL;
	int key = 0, index = 0, color = 0;
	int ret = 0;

	WIDGET_CHECK_OBJECT(self, data, combobox, GuiComboBox);

	gui_core = GUI_GetCurrent();
	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		/*usr signal callback*/
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		if(combobox->keypress_event)
		{
			ret = exec_widget_event_data(self, combobox->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}

		/*object private callback*/
		if(GUIK_LEFT == key)
		{
			if(combobox->cur_sel)
			{
				combobox->cur_sel--;
			}
			else
			{
				combobox->cur_sel = combobox->total - 1;
			}

			if(combobox->change_event)
			{
				exec_widget_event_data(self, combobox->change_event, NULL);
			}
			widget_set_update(self, GUI_TRUE);
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if(gal_color2index(screen, gui_core->config.gui_trans) == color)
			{
				wm_update_contain_widget(self);
			}
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			if(combobox->cur_sel < (combobox->total - 1))
			{
				combobox->cur_sel++;
			}
			else
			{
				combobox->cur_sel = 0;
			}

			if(combobox->change_event)
			{
				exec_widget_event_data(self, combobox->change_event, NULL);
			}
			widget_set_update(self, GUI_TRUE);
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if(gal_color2index(screen, gui_core->config.gui_trans) == color)
			{
				wm_update_contain_widget(self);
			}
			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			if(widget_is_keypress_revert(self)) {
				revert_gui_key(event);
			}
			return (EVENT_TRANSFER_KEEPON);
		}
		break;
	default:
		break;
	}

	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}
	return (EVENT_TRANSFER_KEEPON);
}

static int combobox_set_property(GuiWidget *self, char *name, void *data)
{
	GuiComboBox *combobox = NULL;
	GuiWidget *parent = NULL;
	int sel = 0, index = 0, color = 0;
	char *string = NULL;
	char value[100] = {0};

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	parent = widget_get_parent(self);

	if(0 == strcasecmp("select", name))
	{
		sel = *((int *)data);
		if(sel < 0)
			return (0);

		if((sel < combobox->total) && (sel != combobox->cur_sel))
		{
			if(combobox->cur_sel < 0)
			{
				combobox->cur_sel = 0;
			}
			else if(combobox->cur_sel >= combobox->total)
			{
				combobox->cur_sel = combobox->total - 1;
			}
			else
			{
				combobox->cur_sel = sel;
			}

			if(combobox->change_event)
			{
				exec_widget_event_data(self, combobox->change_event, NULL);
			}
			sprintf(value, "%d", sel);
			widget_set_property(self, name, value);
			widget_set_update(self, GUI_TRUE);
			if((parent) && (0 == strcasecmp("boxitem", parent->classname)))
			{
				widget_set_update(parent, GUI_TRUE);
				if(widget_get_parent(parent))
				{
					parent = widget_get_parent(parent);
					parent->need_update = GUI_TRUE;
				}
			}
		}
	}
	else if(0 == strcasecmp("content", name))
	{
		string = (char *)data;
		if(NULL != string)
		{
			_free_content(self);
			widget_set_property(self, name, data);
			combobox->total = _get_item_from_string(string, &(combobox->content));
			combobox->cur_sel = 0;
			widget_set_update(self, GUI_TRUE);
			if((parent) && (0 == strcasecmp("boxitem", parent->classname)))
			{
				widget_set_update(parent, GUI_TRUE);
			}
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if(0 == color)
			{
				wm_update_contain_widget(self);
			}

			if((parent) && (0 == strcasecmp("boxitem", parent->classname)))
			{
				widget_set_update(parent, GUI_TRUE);
				if(widget_get_parent(parent))
				{
					parent = widget_get_parent(parent);
					parent->need_update = GUI_TRUE;
				}
			}
			return (0);
		}
		else
		{
			return (1);
		}
	}
	else if((0 == strcasecmp("unfocus_img", name)) ||
			(0 == strcasecmp("focus_img", name)) ||
			(0 == strcasecmp("disable_img", name)) ||
			(0 == strcasecmp("unfocus_left_img", name)) ||
			(0 == strcasecmp("focus_left_img", name) ||
			 (0 == strcasecmp("disable_left_img", name)) ||
			 (0 == strcasecmp("unfocus_right_img", name))) ||
			(0 == strcasecmp("focus_right_img", name)) ||
			(0 == strcasecmp("disable_right_img", name)))
	{
		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
		if((parent) && (0 == strcasecmp("boxitem", parent->classname)))
		{
			widget_set_update(parent, GUI_TRUE);
			if(widget_get_parent(parent))
			{
				parent = widget_get_parent(parent);
				parent->need_update = GUI_TRUE;
			}
		}
	}
	else if(0 == strcasecmp("update_surface", name))
	{
		combobox->update_surface = GUI_TRUE;
	}
	else
	{
		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (0);
	}

	return (0);
}

static int combobox_get_property(GuiWidget *self, char *name, void *data)
{
	GuiComboBox *combobox = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	if(0 == strcasecmp("select", name))
	{
		if(combobox->cur_sel < combobox->total)
		{
			*(int*)(data) = combobox->cur_sel;
		}
	}
	else if(0 == strcasecmp("select_content", name))
	{
		if(combobox->content)
		{
			*((char **)data) = combobox->content[combobox->cur_sel];
		}
	}
	else
	{
		return (1);
	}

	return (0);
}

static int combobox_prepare_image(GuiWidget *self)
{
	GuiComboBox *combobox = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if((NULL != value) || (NULL == combobox->image[0]) || (NULL == combobox->image_g[0][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(combobox->image_g[0][0]),
								  &(combobox->image_g[0][1]),
								  &(combobox->image_g[0][2]));
		}
		else
		{
			combobox->image[0] = widget_img_load(value);
			combobox->image_g[0][0] = combobox->image_g[0][1] = combobox->image_g[0][2] = NULL;
		}
	}
	value = widget_get_property(self, "focus_img");
	if((NULL != value) || (NULL == combobox->image[1]) || (NULL == combobox->image_g[1][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(combobox->image_g[1][0]),
								  &(combobox->image_g[1][1]),
								  &(combobox->image_g[1][2]));
		}
		else
		{
			combobox->image[1] = widget_img_load(value);
			combobox->image_g[1][0] = combobox->image_g[1][1] = combobox->image_g[1][2] = NULL;
		}
	}
	value = widget_get_property(self, "disable_img");
	if((NULL != value) || (NULL == combobox->image[2]) || (NULL == combobox->image_g[2][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(combobox->image_g[2][0]),
								  &(combobox->image_g[2][1]),
								  &(combobox->image_g[2][2]));
		}
		else
		{
			combobox->image[2] = widget_img_load(value);
			combobox->image_g[2][0] = combobox->image_g[2][1] = combobox->image_g[2][2] = NULL;
		}
	}

	value = widget_get_property(self, "unfocus_left_img");
	if((NULL != value) || (NULL == combobox->left_arrow[0]))
	{
		combobox->left_arrow[0] = widget_img_load(value);
	}
	value = widget_get_property(self, "focus_left_img");
	if((NULL != value) || (NULL == combobox->left_arrow[1]))
	{
		combobox->left_arrow[1] = widget_img_load(value);
	}
	value = widget_get_property(self, "disable_left_img");
	if((NULL != value) || (NULL == combobox->left_arrow[2]))
	{
		combobox->left_arrow[2] = widget_img_load(value);
	}

	value = widget_get_property(self, "unfocus_right_img");
	if((NULL != value) || (NULL == combobox->right_arrow[0]))
	{
		combobox->right_arrow[0] = widget_img_load(value);
	}
	value = widget_get_property(self, "focus_right_img");
	if((NULL != value) || (NULL == combobox->right_arrow[1]))
	{
		combobox->right_arrow[1] = widget_img_load(value);
	}
	value = widget_get_property(self, "disable_right_img");
	if((NULL != value) || (NULL == combobox->right_arrow[2]))
	{
		combobox->right_arrow[2] = widget_img_load(value);
	}

	return (0);
}

static int combobox_clear_image(GuiWidget *self)
{
	GuiComboBox *combobox = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	for(i = 0; i < 3; i++)
	{
		widget_img_clear(&(combobox->image[i]));
		widget_img_group_clear(&(combobox->image_g[i][0]),
							   &(combobox->image_g[i][1]),
							   &(combobox->image_g[i][2]));
		widget_img_clear(&(combobox->left_arrow[i]));
		widget_img_clear(&(combobox->right_arrow[i]));
	}

	return (0);
}

static int combobox_unactive(GuiWidget *self)
{
	GuiComboBox *combobox = NULL;

	if(NULL == self)
	{
		return (1);
	}

	combobox = (GuiComboBox *)(self->object);
	if(NULL == combobox)
	{
		return (1);
	}

	if(combobox->surface) {
		gal_free_surface(combobox->surface);
		combobox->surface = NULL;
	}

	return (0);
}

GuiWidgetOps combobox_ops =
{
	.owner = "combobox",
	.create = create_combobox,
	.release = combobox_release,
	.prepare_image = combobox_prepare_image,
	.draw = combobox_draw,
	.clear_image   = combobox_clear_image,
	.unactive   = combobox_unactive,
	.event = combobox_event,
	.set = combobox_set_property,
	.get = combobox_get_property
};

