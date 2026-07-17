#include "all_widget.h"
#include "color.h"

static int _box_get_scrollbar_mode(GuiWidget *self)
{
	char *value = NULL;
	int ret = 0;

	if(NULL == self)
	{
		return (BOX_HIDE_SCROLLBAR);
	}

	value = (char *)widget_get_property(self, "scroll_enable");

	if(value)
	{
		if(0 == strcasecmp("hide", value))
		{
			ret = BOX_HIDE_SCROLLBAR;
		}
		else if(0 == strcasecmp("show", value))
		{
			ret = BOX_SHOW_SCROLLBAR;
		}
	}

	return (ret);
}

static int _box_get_format(GuiWidget *self)
{
	char *value = NULL;
	int ret = 0;

	if(NULL == self)
	{
		return (BOX_VERTICAL);
	}

	value = (char *)widget_get_property(self, "format");
	if(value)
	{
		if(0 == strcasecmp("vertical", value))
		{
			ret = BOX_VERTICAL;
		}
		else if(0 == strcasecmp("horizontal", value))
		{
			ret = BOX_HORIZONTAL;
		}
	}

	return (ret);
}

static int _check_box(GUI_Rect *child, GUI_Rect *self)
{
	if(((child->x + child->w) <= (self->x + self->w)) &&
			((child->y + child->h) <= (self->y + self->h)) &&
			(child->x >= self->x) &&
			(child->y >= self->y))
	{
		return (GUI_TRUE);
	}
	else
	{
		return (GUI_FALSE);
	}

}

static int _config_box_parametre(GuiWidget *self)
{
	GuiBox *box = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	value = widget_get_property(self, "interval");
	if((NULL != value) || (0 == box->item_interval))
	{
		box->item_interval = widget_get_int(self, "interval", 0);
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == box->format))
	{
		box->format = _box_get_format(self);
	}

	value = widget_get_property(self, "scroll_enable");
	if((NULL != value) || (0 == box->show_scrollbar))
	{
		box->show_scrollbar = _box_get_scrollbar_mode(self);
	}

	return (0);
}


static void *create_box(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiBox *box = NULL, *box_style = NULL;
	GuiWidget *child = NULL, *parent = NULL;
	GuiWidget *first_child = NULL, *style = NULL;
	const char *value = NULL, *string = NULL;
	int offset = 0;
	void *ret = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	box = (GuiBox *)GUI_MALLOC(sizeof(GuiBox));
	if(NULL == box)
	{
		return (NULL);
	}

	memset(box, 0, sizeof(GuiBox));

	self->object = (void *)box;

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	self->disable_mirror = parent->disable_mirror;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		box_style = (GuiBox *)GUI_MALLOC(sizeof(GuiBox));
		if(NULL == box_style)
		{
			GUI_FREE(box);
			return (NULL);
		}

		memset(box_style, 0, sizeof(GuiBox));

		style->object = (void *)box_style;
		_config_box_parametre(style);

		memcpy(box, box_style, sizeof(GuiBox));

		GUI_FREE(style->object);
	}

	_config_box_parametre(self);

	first_child = child = widget_get_firstchild(self);

	offset = box->item_interval;
	while(child)
	{
		//widget_create(child);
		string = (char *)widget_get_property(child, "state");
		if((WIDGET_STATE_HIDE & (child->state)) ||
				((NULL != string) && (strstr(string, "hide"))))
		{
			widget_create(child);
			child->state |= WIDGET_STATE_HIDE;
			widget_set_update(child, GUI_FALSE);
			child = widget_get_nextsibling(child);
			continue;
		}
		widget_get_rect(child, &(child->rect));
		if(BOX_VERTICAL == box->format)
		{
			child->rect.y = offset;
			if(box->left_arrow)
			{
				child->rect.y += box->left_arrow->height;
			}
			offset = child->rect.y + child->rect.h + box->item_interval;
		}
		else if(BOX_HORIZONTAL == box->format)
		{
			child->rect.x = offset;
			if(box->left_arrow)
			{
				child->rect.x += box->left_arrow->width;
			}
			offset = child->rect.x + child->rect.w + box->item_interval;
		}
		value = (const char *)widget_get_property(child, "backcolor");
		if(value)
		{
			getColorFromString(value, child->back_color);
		}
		value = (const char *)widget_get_property(child, "forecolor");
		if(value)
		{
			getColorFromString(value, child->fore_color);
		}

		if((0 != strcasecmp("boxitem", child->classname)) &&
				(0 != strcasecmp("scrollbar", child->classname)))
		{
			child = widget_get_nextsibling(child);
			continue;
		}


		if(child->ops && child->ops->create)
		{
			value = widget_get_property(child, "mirror");
			if((value) && (0 == strcasecmp("disable", value))) {
				child->disable_mirror = GUI_TRUE;
			} else {
				child->disable_mirror = GUI_FALSE;
			}

			value = widget_get_property(child, "keypress_revert");
			if((value) && (0 == strcasecmp("enable", value))) {
				child->keypress_revert = GUI_TRUE;
			} else {
				child->keypress_revert = gui_core->config.keypress_revert;
			}

			ret = child->ops->create(child);

			if(NULL != ret)
			{
				widget_set_update(child, GUI_TRUE);
			}
		}
		widget_set_update(child, GUI_TRUE);

		child = widget_get_nextsibling(child);

		if(first_child->state & WIDGET_STATE_DISABLE)
		{
			first_child = child;
		}
	}

	if((WIDGET_STATE_FOCUSED & self->state) &&
	   (first_child))
	{
		GUI_SetProperty(first_child->name, "active", NULL);
	}
	/*child = widget_get_firstchild(first_child);
	  while(child)
	  {
	  widget_set_update(child, GUI_TRUE);
	  child = widget_get_nextsibling(child);
	  }*/

			self->state = WIDGET_STATE_FOCUSSABLE;

			while(first_child &&
					((first_child->state & WIDGET_STATE_HIDE) ||
					 (first_child->state & WIDGET_STATE_DISABLE)))
			{
				first_child = widget_get_nextsibling(first_child);
			}

			box->focus = first_child;

			if(self->signal)
			{
				box->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
				box->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
				box->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
				box->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
				box->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
			}

			box->base = self;

			if(box->create_event)
			{
				exec_widget_event_data(self, box->create_event, NULL);
			}

			if(box->change_event)
			{
				exec_widget_event_data(self, box->change_event, NULL);
			}

			return (box);
}

static int box_release(GuiWidget *self)
{
	GuiWidget *child = NULL;
	GuiBox *box = NULL;

	if(NULL == self)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	if(box->destroy_event)
	{
		exec_widget_event_data(self, box->destroy_event, NULL);
	}

	GUI_FREE(self->object);

	return (0);
}

static int box_draw(GuiWidget *self)
{
	GuiWidget *child = NULL;
	GuiWidget *focus = NULL;
	GuiBox *box = NULL;
	GUI_Rect valid_rect = {0};
	int index = 0, color = 0;

	if(NULL == self)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	focus = box->focus;

	if(focus != widget_get_firstchild(self))
	{
		if(box->left_arrow)
		{
			valid_rect = self->rect;
			if(BOX_HORIZONTAL== box->format)
			{
				valid_rect.y = valid_rect.y +
					((valid_rect.h - box->left_arrow->height) / 2);
			}
			else
			{
				valid_rect.x = valid_rect.x +
					((valid_rect.w - box->left_arrow->width) / 2);
			}
			gal_img_desc(screen,
					box->left_arrow,
					valid_rect.x,
					valid_rect.y,
					box->left_arrow->width,
					box->left_arrow->height);
		}
	}
	else
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		if(box->left_arrow)
		{
			valid_rect = self->rect;
			if(BOX_HORIZONTAL== box->format)
			{
				valid_rect.y = valid_rect.y +
					((valid_rect.h - box->left_arrow->height) / 2);
			}
			else
			{
				valid_rect.x = valid_rect.x +
					((valid_rect.w - box->left_arrow->width) / 2);
			}
			valid_rect.w = box->left_arrow->width;
			valid_rect.h = box->left_arrow->height;
		}
		else if(box->update_all)
		{
			valid_rect = self->rect;
		}
		gal_fillrect(screen, (GAL_Rect *)(&valid_rect), color);
	}

	if(focus != widget_get_lastchild(self))
	{
		if(box->right_arrow)
		{
			valid_rect = self->rect;
			if(BOX_HORIZONTAL== box->format)
			{
				valid_rect.y = valid_rect.y +
					((valid_rect.h - box->right_arrow->height) / 2);
				valid_rect.x = valid_rect.x + valid_rect.w - box->right_arrow->width;
			}
			else
			{
				valid_rect.x = valid_rect.x +
					((valid_rect.w - box->right_arrow->width) / 2);
				valid_rect.y = valid_rect.y + valid_rect.h - box->right_arrow->height;
			}
			valid_rect.w = box->right_arrow->width;
			valid_rect.h = box->right_arrow->height;
			gal_img_desc(screen,
					box->right_arrow,
					valid_rect.x,
					valid_rect.y,
					box->right_arrow->width,
					box->right_arrow->height);
		}
	}
	else
	{
		if(box->right_arrow)
		{
			valid_rect = self->rect;
			if(BOX_HORIZONTAL== box->format)
			{
				valid_rect.y = valid_rect.y +
					((valid_rect.h - box->right_arrow->height) / 2);
				valid_rect.x = valid_rect.x + valid_rect.w - box->right_arrow->width;
			}
			else
			{
				valid_rect.x = valid_rect.x +
					((valid_rect.w - box->right_arrow->width) / 2);
				valid_rect.y = valid_rect.y + valid_rect.h - box->right_arrow->height;
			}
			valid_rect.w = box->right_arrow->width;
			valid_rect.h = box->right_arrow->height;
		}
		else if(box->update_all)
		{
			valid_rect = self->rect;
		}
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)(&valid_rect), color);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(WIDGET_STATE_HIDE & (child->state))
		{
			widget_set_update(child, GUI_FALSE);
			child = widget_get_nextsibling(child);
			continue;
		}

		widget_mirror_rect(child);
		if(_check_box(&child->rect, &self->rect))
		{
			if((WIDGET_STATE_FOCUSED & self->state) && (focus))
			{
				//focus->ops->set(focus, "active", NULL);
				GUI_SetProperty(focus->name, "active", NULL);
			}
			else if(focus)
			{
				GUI_SetProperty(focus->name, "unactive", NULL);
				//focus->ops->set(focus, "unactive", NULL);
			}

			if(!(WIDGET_STATE_HIDE & child->state))
			{
				widget_draw(child);
			}
			else
			{
				valid_rect = child->rect;
				index = widget_get_state_index(self);
				color = self->back_color[index];
				color = gal_color2index(screen, color);
				gal_fillrect(screen, (GAL_Rect *)(&valid_rect), color);
			}
			widget_set_update(child, GUI_FALSE);
		}
		child = widget_get_nextsibling(child);
	}

	box->update_all = GUI_FALSE;

	return (0);
}

static int _box_update_all(GuiWidget *self, int offset, int flag)
{
	GuiBox *box = NULL;
	GuiWidget *child = NULL, *parent = NULL;

	if(NULL == self)
	{
		return (1);
	}

	parent = widget_get_parent(self);
	box = (GuiBox *)(parent->object);
	if(NULL == box)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(BOX_VERTICAL == flag)
		{
			child->rect.y += offset;
		}
		else if(BOX_HORIZONTAL == flag)
		{
			child->rect.x += offset;
		}
		widget_set_update(child, GUI_TRUE);
		child = widget_get_nextsibling(child);
	}

	widget_set_update(self, GUI_TRUE);

	return (0);
}

static int _box_update_child(GuiBox *box, GuiWidget *child)
{
	GuiWidget *self = NULL;

	if(NULL == box || NULL == child)
	{
		return (1);
	}

	self = box->base;
	if(NULL == self)
	{
		return (1);
	}


	if(box->focus) {
		box->focus->ops->set(box->focus, "unactive", NULL);
	}
	child->ops->set(child, "active", NULL);
	if(box->focus) {
		widget_set_update(box->focus, GUI_TRUE);
		box->focus = child;
	}
	widget_set_update(child, GUI_TRUE);
	//widget_set_update(self, GUI_TRUE);
	self->need_update = GUI_TRUE;

	return (0);
}

static int box_event(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiBox *box = NULL;
	GuiWidget *focus = NULL;
	GUI_Event *event = NULL;
	int key = 0, ret = 0, offset = 0;
	GuiWidget *child = NULL;

	WIDGET_CHECK_OBJECT(self, data, box, GuiBox);

	event = (GUI_Event *)data;

	if(0 == event->type)
	{
		return (1);
	}

	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}

	/*******box->boxitem********/
	focus = box->focus;
	if(focus && focus->ops && focus->ops->event)
	{
		ret = focus->ops->event(box->focus, data);
		//widget_set_update(self, GUI_TRUE);
		if(GUI_KEYDOWN == event->type)
		{
			self->need_update = GUI_TRUE;
		}
		/*box不阻止消息继续传递，因为box组件存在横向的，不能将按键阻止，故相当于23631修改作废*/
		/*if(EVENT_TRANSFER_STOP == ret)
		  {
		  widget_set_update(self, GUI_TRUE);
		  return ret;
		  }*/
	}


	/*******box_usr********/
	if(box->keypress_event)
	{
		ret = exec_widget_event_data(self, box->keypress_event, data);
		if(EVENT_TRANSFER_STOP == ret)
		{
			return ret;
		}
	}

	/*******box_pub********/
	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;

		if(BOX_VERTICAL == box->format)
		{
			if(GUIK_DOWN == key)
			{
				child = widget_get_nextsibling(box->focus);
				while(child &&
						((child->state & WIDGET_STATE_DISABLE) ||
						 (child->state & WIDGET_STATE_HIDE)))
				{
					child = widget_get_nextsibling(child);
				}
				focus = child;

				if(child)
				{
					if(_check_box(&child->rect, &self->rect))
					{
						_box_update_child(box, child);
					}
					else
					{
						_box_update_child(box, child);
						if((focus->rect.x < self->rect.x) || (focus->rect.y < self->rect.y))
						{
							gxlogd("[GUI] Focus on out bound widget in box %s\n", self->name);
							return (EVENT_TRANSFER_STOP);
						}

						while((focus->rect.y + focus->rect.h) > (self->rect.y + self->rect.h))
						{
							child = widget_get_firstchild(self);
							while(child)
							{
								offset = box->item_interval + child->rect.h;
								if(box->right_arrow)
								{
									offset += box->right_arrow->height;
								}
								child->rect.y -= offset;
								_box_update_all(child,
										-offset,
										BOX_VERTICAL);
								child = widget_get_nextsibling(child);
							}
						}
					}
				}
				if(box->change_event)
				{
					exec_widget_event_data(self, box->change_event, NULL);
				}

				return (EVENT_TRANSFER_STOP);
			}
			else if(GUIK_UP == key)
			{
				child = widget_get_priorsibling(box->focus);
				while(child &&
						((child->state & WIDGET_STATE_DISABLE) ||
						 (child->state & WIDGET_STATE_HIDE)))
				{
					child = widget_get_priorsibling(child);
				}

				focus = child;
				if(child)
				{
					if(_check_box(&child->rect, &self->rect))
					{
						_box_update_child(box, child);
					}
					else
					{
						_box_update_child(box, child);

						while(focus->rect.y < self->rect.y)
						{
							child = widget_get_firstchild(self);
							while(child)
							{
								offset = box->item_interval + child->rect.h;
								if(box->right_arrow)
								{
									offset += box->left_arrow->height;
								}
								child->rect.y += offset;
								_box_update_all(child,
										offset,
										BOX_VERTICAL);
								child = widget_get_nextsibling(child);
							}
						}
					}
				}
				if(box->change_event)
				{
					exec_widget_event_data(self, box->change_event, NULL);
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
		}
		else if(BOX_HORIZONTAL== box->format)
		{
			if(GUIK_LEFT == key)
			{
				child = widget_get_priorsibling(box->focus);
				while(child &&
						((child->state & WIDGET_STATE_DISABLE) ||
						 (child->state & WIDGET_STATE_HIDE)))
				{
					child = widget_get_priorsibling(child);
				}
				focus = child;

				if(child)
				{
					if(_check_box(&child->rect, &self->rect))
					{
						_box_update_child(box, child);
					}
					else
					{
						_box_update_child(box, child);

						while(focus->rect.x <= self->rect.x)
						{
							child = widget_get_firstchild(self);
							while(child)
							{
								offset = box->item_interval + child->rect.w;
								if(box->right_arrow)
								{
									offset += box->right_arrow->width;
								}
								child->rect.x += offset;
								_box_update_all(child,
										offset,
										BOX_HORIZONTAL);
								child = widget_get_nextsibling(child);
							}
						}
					}
				}
				if(box->change_event)
				{
					exec_widget_event_data(self, box->change_event, NULL);
				}

				return (EVENT_TRANSFER_STOP);
			}
			else if(GUIK_RIGHT == key)
			{
				child = widget_get_nextsibling(box->focus);
				while(child &&
						((child->state & WIDGET_STATE_DISABLE) ||
						 (child->state & WIDGET_STATE_HIDE)))
				{
					child = widget_get_nextsibling(child);
				}
				focus = child;

				if(child)
				{
					if(_check_box(&child->rect, &self->rect))
					{
						_box_update_child(box, child);
					}
					else
					{
						_box_update_child(box, child);

						while((focus->rect.x + focus->rect.w) >= (self->rect.x + self->rect.w))
						{
							child = widget_get_firstchild(self);
							while(child)
							{
								offset = box->item_interval + child->rect.w;
								if(box->left_arrow)
								{
									offset += box->left_arrow->width;
								}
								child->rect.x -= offset;
								_box_update_all(child,
										-offset,
										BOX_HORIZONTAL);
								child = widget_get_nextsibling(child);
							}
						}
					}
				}
				if(box->change_event)
				{
					exec_widget_event_data(self, box->change_event, NULL);
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
		}
		break;
	default:
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		return (EVENT_TRANSFER_KEEPON);
	}
	return (EVENT_TRANSFER_STOP);
}

static int box_set_property(GuiWidget *self, char *name, void *data)
{
	GuiWidget *child = NULL, *next_child = NULL;
	GuiWidget *free_child = NULL;
	GuiWidget *new_child = NULL;
	GuiBox *box = NULL;
	char *string = NULL;
	int value = 0, i = 0, offset = 0;
	void *ret = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	if(0 == strcasecmp("select", name))
	{
		value = *(int *)(data);
		child = widget_get_firstchild(self);
		while(child)
		{
			if(i == value)
			{
				while(child &&
						((child->state & WIDGET_STATE_DISABLE) ||
						 (child->state & WIDGET_STATE_HIDE)))
				{
					child = widget_get_priorsibling(child);
				}
				if(NULL == child)
				{
					return (1);
				}

				GUI_SetProperty(child->name, "active", NULL);

				if(box->focus) {
					GUI_SetProperty(box->focus->name, "unactive", NULL);
					widget_set_update(box->focus, GUI_TRUE);
				}
				box->focus = child;
				widget_set_update(child, GUI_TRUE);
				widget_set_update(self, GUI_TRUE);
				break;
			}
			i++;
			child = widget_get_nextsibling(child);
		}
		if(box->change_event)
		{
			exec_widget_event_data(self, box->change_event, NULL);
		}
	}
	else if(0 == strcasecmp("update", name))
	{
		widget_set_update(self, GUI_TRUE);
		child = widget_get_firstchild(self);
		while(child)
		{
			widget_set_update(child, GUI_TRUE);
			next_child = widget_get_firstchild(child);
			while(next_child)
			{
				widget_set_update(next_child, GUI_TRUE);
				next_child = widget_get_nextsibling(next_child);
			}
			child = widget_get_nextsibling(child);
		}
		box->update_all = GUI_TRUE;
	}
	else if(0 == strcasecmp("add_item", name))
	{
		string = (char *)data;
		free_child = gui_get_widget(gui.config.freedom_widget, string);
		if(NULL == free_child)
		{
			return (1);
		}

		new_child = (GuiWidget *)GUI_MALLOC(sizeof(GuiWidget));
		if(NULL == new_child)
		{
			return (1);
		}

		memset(new_child, 0, sizeof(GuiWidget));

		memcpy(new_child, free_child, sizeof(GuiWidget));

		child = widget_get_firstchild(new_child);
		while(child)
		{
			child->parent = new_child;
			child = widget_get_nextsibling(child);
		}

		child = widget_get_lastchild(self);
		if(NULL == child)
		{
			return (1);
		}

		offset = box->item_interval;

		widget_get_rect(new_child, &(new_child->rect));
		if(BOX_VERTICAL == box->format)
		{
			new_child->rect.y = offset + child->rect.y + child->rect.h;
			new_child->rect.x = child->rect.x;
		}
		else if(BOX_HORIZONTAL == box->format)
		{
			new_child->rect.x = offset + child->rect.x + child->rect.w;
			new_child->rect.y = child->rect.y;
		}
		string = (char *)widget_get_property(new_child, "backcolor");
		if(string)
		{
			getColorFromString(string, new_child->back_color);
		}
		string = (char *)widget_get_property(new_child, "forecolor");
		if(string)
		{
			getColorFromString(string, new_child->fore_color);
		}


		if(new_child->ops && new_child->ops->create)
		{

			ret = new_child->ops->create(new_child);

			if(NULL != ret)
			{
				widget_set_update(new_child, GUI_TRUE);
			}
		}

		widget_add_child(self, new_child);
		widget_set_update(new_child, GUI_TRUE);
		widget_set_update(self, GUI_TRUE);

		gxlogd("[GUI]**********FREE Child = %s**********\n", new_child->name);
	}
	else if(0 == strcasecmp("focus", name))
	{
		widget_set_update(box->focus, GUI_TRUE);
		box->focus = (GuiWidget *)data;
		if(box->focus) {
			widget_set_update(box->focus, GUI_TRUE);
		}
		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		return (1);
	}

	return (0);
}

static int box_get_property(GuiWidget *self, char *name, void *data)
{
	GuiWidget *child = NULL;
	GuiBox *box = NULL;
	int value = 0;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	if(0 == strcasecmp("get_first_focussable_widget", name))
	{
		child = widget_get_firstchild(self);
		child = widget_get_firstchild(child);
		while(child)
		{
			if(widget_get_focussable(child))
			{
				*(GuiWidget**)data = child;
				break;
			}
			child = widget_get_nextsibling(child);
		}
	}
	else if(0 == strcasecmp("select", name))
	{
		child = widget_get_firstchild(self);
		while(child)
		{
			if(child == box->focus)
			{
				*((int *)data) = value;
				break;
			}
			value++;
			child = widget_get_nextsibling(child);
		}
	}
	else if(0 == strcasecmp("focus", name))
	{
		*(GuiWidget **)data = box->focus;
	}
	else
	{
		return (1);
	}
	return (0);
}

static int box_prepare_image(GuiWidget *self)
{
	GuiBox *box = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	value = widget_get_property(self, "left_arrow");
	if((NULL != value) || (NULL == box->left_arrow))
	{
		box->left_arrow = widget_img_load(value);
	}

	value = widget_get_property(self, "right_arrow");
	if((NULL != value) || (NULL == box->right_arrow))
	{
		box->right_arrow = widget_img_load(value);
	}

	return (0);
}

static int box_clear_image(GuiWidget *self)
{
	GuiBox *box = NULL;

	if(NULL == self)
	{
		return (1);
	}

	box = (GuiBox *)(self->object);
	if(NULL == box)
	{
		return (1);
	}

	if(box->left_arrow)
	{
		widget_img_clear(&(box->left_arrow));
	}

	if(box->right_arrow)
	{
		widget_img_clear(&(box->right_arrow));
	}

	return (0);
}

GuiWidgetOps box_ops =
{
	.owner = "box",
	.create = create_box,
	.release = box_release,
	.prepare_image = box_prepare_image,
	.draw = box_draw,
	.clear_image   = box_clear_image,
	.event = box_event,
	.set = box_set_property,
	.get = box_get_property
};


