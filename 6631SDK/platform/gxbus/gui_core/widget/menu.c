#include "all_widget.h"

handle_t menu_mutex = -1;

static MENU_FORMAT _get_button_move_format(const char *value)
{
	if(NULL == value)
	{
		return (MENU_HORIZONTAL_DOWN);
	}

	if(0 == strcasecmp("horizontal_up", value))
	{
		return (MENU_HORIZONTAL_UP);
	}
	else if(0 == strcasecmp("horizontal_down", value))
	{
		return (MENU_HORIZONTAL_DOWN);
	}
	else if(0 == strcasecmp("vertical_left", value))
	{
		return (MENU_VERTICAL_LEFT);
	}
	else if(0 == strcasecmp("vertical_right", value))
	{
		return (MENU_VERTICAL_RIGHT);
	}
	else
	{
		return (MENU_HORIZONTAL_DOWN);
	}

}

static int _config_menu_parametre(GuiWidget *self)
{
	GuiMenu *menu = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (1);
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == menu->format))
	{
		menu->format = _get_button_move_format(value);
		if(MENU_HORIZONTAL_DOWN == menu->format)
		{
			menu->direction = DIRECTION_DOWN;
		}
		else if(MENU_HORIZONTAL_UP == menu->format)
		{
			menu->direction = DIRECTION_UP;
		}
		else if(MENU_VERTICAL_LEFT == menu->format)
		{
			menu->direction = DIRECTION_LEFT;
		}
		else if(MENU_VERTICAL_RIGHT == menu->format)
		{
			menu->direction = DIRECTION_RIGHT;
		}
	}

	value = widget_get_property(self, "visible_list");
	if((NULL != value) || (0 == menu->visible_list))
	{
		menu->visible_list = widget_get_int(self, "visible_list", 0);
	}

	value = widget_get_property(self, "fixed_list");
	if((NULL != value) || (0 == menu->fixed_list))
	{
		menu->fixed_list = widget_get_int(self, "fixed_list", 0);
	}

	return (0);
}

static void *create_menu(GuiWidget *self)
{
	GuiMenu *menu = NULL, *menu_style = NULL;
	GuiWidget *parent = NULL, *style = NULL, *child = NULL, *widget = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	if(menu_mutex == -1)
	{
		GxCore_MutexCreate(&menu_mutex);
	}

	menu = (GuiMenu *)GUI_MALLOC(sizeof(GuiMenu));

	if(NULL == menu)
	{
		return (NULL);
	}

	memset(menu, 0, sizeof(GuiMenu));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	menu->base = self;
	self->object = (void *)menu;
	self->state = WIDGET_STATE_FOCUSSABLE;

	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		menu_style = (GuiMenu *)GUI_MALLOC(sizeof(GuiMenu));

		if(NULL == menu_style)
		{
			GUI_FREE(menu);
			return (NULL);
		}

		memset(menu_style, 0, sizeof(GuiMenu));

		style->object = (void *)menu_style;

		_config_menu_parametre(style);
		memcpy(menu, menu_style, sizeof(GuiMenu));

		GUI_FREE(style->object);
	}

	_config_menu_parametre(self);

	if(0 == menu->visible_list)
	{
		gxlogd("[GUI]Invalid visible list count!\n");
		GUI_FREE(menu);
		return (NULL);
	}
	else if((menu->fixed_list) >= (menu->visible_list))
	{
		gxlogd("[GUI]Invalid fixed list number!\n");
		GUI_FREE(menu);
		return (NULL);
	}

	child = widget_get_firstchild(self);
	menu->focused_list = (unsigned char *)(child->name);
	wm_state_set_focus(self);

	while(child)
	{
		widget_create(child);

		if(0 == strcasecmp((const char *)child->name, (const char *)menu->focused_list))
		{
			child->state |= WIDGET_STATE_FOCUSED;
			if(NULL == widget)
			{
				gxlogd("[GUI]There is no widget named as %s, Please check it!\n", menu->focused_list);
			}
		}
		child = widget_get_nextsibling(child);
	}

	menu->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
	menu->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	menu->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
	menu->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
	menu->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");

	if(menu->create_event)
	{
		exec_widget_event_data(self, menu->create_event, NULL);
	}

	return (menu);
}

static void dynamic_menu(void *data)
{
	GuiWidget *self = NULL, *child = NULL;
	GuiMenu *menu = NULL;
	unsigned int position = 0, deadline = 0;
	GUI_Rect src_rect = {0}, dst_rect = {0};
	int value = GUI_FALSE;

	GxCore_ThreadDetach();
	if(NULL == data)
	{
		return;
	}

	self = (GuiWidget *)data;

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return;
	}

	GxCore_MutexLock(menu_mutex);

	child = widget_get_firstchild(self);
	position = 0;

	if((MENU_HORIZONTAL_UP == menu->format) ||
			(MENU_HORIZONTAL_DOWN == menu->format))
	{
		deadline = child->rect.h;
	}
	else if((MENU_VERTICAL_LEFT == menu->format) ||
			(MENU_VERTICAL_RIGHT == menu->format))
	{
		deadline = child->rect.w;
	}

	/*if(menu->movable_surface)
	  {
	  color = gal_color2index(screen, self->back_color[0]);
	  gal_fillrect(menu->movable_surface,
	  (GAL_Rect *)(&self->rect),
	  color);
	  }*/

	position = 0;
	while(position < deadline)
	{
		src_rect = dst_rect = self->rect;
		src_rect.x = src_rect.y = 0;
		gal_stretch_surface(menu->back_surface,
				(GAL_Rect *)(&src_rect),
				screen,
				(GAL_Rect *)(&dst_rect));

		/*if((MENU_HORIZONTAL_UP == menu->format) ||
		  (MENU_HORIZONTAL_DOWN == menu->format))
		  {
		  src_rect.x += deadline;
		  }
		  else if((MENU_VERTICAL_LEFT == menu->format) ||
		  (MENU_VERTICAL_RIGHT == menu->format))
		  {
		  src_rect.y += deadline;
		  }*/

		if(DIRECTION_UP == menu->direction)
		{
			src_rect.y += position;
		}
		else if(DIRECTION_DOWN == menu->direction)
		{
			src_rect.y -= position;
		}
		else if(DIRECTION_LEFT == menu->direction)
		{
			src_rect.x += position;
		}
		else if(DIRECTION_RIGHT == menu->direction)
		{
			src_rect.x += 2 * deadline;
			src_rect.x -= position;
		}

		gal_stretch_surface(menu->movable_surface,
				(GAL_Rect *)(&src_rect),
				screen,
				(GAL_Rect *)(&dst_rect));
		position += 4;

		if(position >= deadline)
		{
			src_rect = dst_rect = self->rect;
			src_rect.x = src_rect.y = 0;
			position = deadline;
			if(DIRECTION_UP == menu->direction)
			{
				src_rect.y += position;
			}
			else if(DIRECTION_DOWN == menu->direction)
			{
				src_rect.y -= position;
			}
			else if(DIRECTION_LEFT == menu->direction)
			{
				src_rect.x += position;
			}
			else if(DIRECTION_RIGHT == menu->direction)
			{
				src_rect.x += 2 * deadline;
				src_rect.x -= position;
			}

			gal_stretch_surface(menu->movable_surface,
					(GAL_Rect *)(&src_rect),
					screen,
					(GAL_Rect *)(&dst_rect));
		}

		gdi_commit();

		gui_delay(5);
	}

	value = GUI_TRUE;
	GUI_SetProperty((const char *)(menu->focused_list), "trigger", (void *)&value);
	widget_draw(gui_get_widget(NULL, (const char *)menu->focused_list));

	menu->direction = DIRECTION_SILL;
	GxCore_MutexUnlock(menu_mutex);
}


static int menu_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiMenu *menu = NULL;
	void *surface = NULL;
	GuiWidget *child = NULL, *first_child = NULL;
	GuiWidget *back_pre_child = NULL, *back_next_child = NULL;
	unsigned int visible_list = 0, count = 0, i = 0;
	unsigned int xPos = 0, yPos = 0, width = 0, height = 0;
	int thrad_id = 0, color = 0;
	GUI_Rect rect = {0}, valid_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (1);
	}
	gui_core = GUI_GetCurrent();

	rect = self->rect;

	visible_list = menu->visible_list;

	count = visible_list / 2;

	if(0 == count)
	{
		count = menu->fixed_list;
		if(0 == count)
		{
			return (1);
		}
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp((const char *)menu->focused_list, (const char *)child->name))
		{
			break;
		}
		child = widget_get_nextsibling(child);
	}

	if(child)
	{
		while(i < count)
		{
			child = widget_get_priorsibling(child);
			if(NULL == child)
			{
				child = widget_get_lastchild(self);
			}
			i++;
		}
	}

	if(menu->back_surface)
	{
		child = widget_get_priorsibling(child);
		if(NULL == child)
		{
			child = widget_get_lastchild(self);
		}
	}

	/*Modify children widget position*/

	xPos = yPos = 0;
	first_child = child;
	while(child)
	{
		child->rect.x = xPos;
		child->rect.y = yPos;

		width = child->rect.w;
		height = child->rect.h;

		child = widget_get_nextsibling(child);
		if((MENU_HORIZONTAL_UP == menu->format) ||
				(MENU_HORIZONTAL_DOWN == menu->format))
		{
			xPos += width;
		}
		else if((MENU_VERTICAL_LEFT == menu->format) ||
				(MENU_VERTICAL_RIGHT == menu->format))
		{
			yPos += height;
		}

		if(NULL == child)
		{
			child = widget_get_firstchild(self);
		}

		if(first_child == child)
		{
			break;
		}
	}

	/*Create Surface and Stretch*/
	if((menu->back_surface) && (DIRECTION_SILL != menu->direction))
	{
		rect = self->rect;
		rect.x = rect.y = 0;
		gal_stretch_surface(menu->back_surface,
				(GAL_Rect*)&rect,
				screen,
				(GAL_Rect*)&(self->rect));
	}
	else
	{
		valid_rect = self->rect;
		valid_rect.x = valid_rect.y = 0;
		valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		surface = gal_get_surface((GAL_Rect*)&(self->rect), gui_core->config.bpp);
		if(NULL == surface)
		{
			return (1);
		}

		rect = self->rect;
		rect.x = 0;
		rect.y = 0;
		gdi_commit();
		gal_copy_surface(screen,
				(GAL_Rect *)(&self->rect),
				surface,
				(GAL_Rect *)(&rect));
		gdi_commit();

		menu->back_surface = surface;
	}
	/*Create movable surface*/
	back_pre_child = first_child;
	if(NULL == back_pre_child)
	{
		back_pre_child = widget_get_lastchild(self);
	}

	i = 0;
	back_next_child = first_child;
	while(i < (menu->visible_list + 1))
	{
		back_next_child = widget_get_nextsibling(back_next_child);

		if(NULL == back_next_child)
		{
			back_next_child = widget_get_firstchild(self);
		}

		i++;
	}

	if(NULL == back_pre_child)
	{
		back_next_child = widget_get_nextsibling(back_next_child);
		if(NULL == back_next_child)
		{
			back_next_child = widget_get_firstchild(self);
		}
	}

	rect = self->rect;
	rect.x = rect.y = 0;

	if((MENU_HORIZONTAL_UP == menu->format) ||
			(MENU_HORIZONTAL_DOWN == menu->format))
	{
		rect.w += (back_pre_child->rect.w + back_next_child->rect.w);
	}
	else if((MENU_VERTICAL_LEFT == menu->format) ||
			(MENU_VERTICAL_RIGHT == menu->format))
	{
		rect.h += (back_pre_child->rect.h + back_next_child->rect.h);
	}

	if(menu->movable_surface)
	{
		color = gal_color2index(screen, self->back_color[0]);
		gdi_commit();
		gal_fillrect(menu->movable_surface,
				(GAL_Rect * )&rect,
				color);
		gdi_commit();

		child = back_pre_child;

		while(child)
		{
			widget_draw(child);

			if(child == back_next_child)
			{
				break;
			}

			child = widget_get_nextsibling(child);
			if(NULL == child)
			{
				child = widget_get_firstchild(self);
			}
		}
		gdi_commit();
	}
	else
	{
		/*Draw all the children regularily*/
		child = first_child;
		i = 0;
		xPos = yPos = 0;
		while(child)
		{
			child->rect.x = (xPos + self->rect.x);
			child->rect.y = (yPos + self->rect.y);

			if(i >= menu->visible_list)
			{
				break;
			}

			widget_draw(child);

			child->rect.x = child->rect.x - self->rect.x;
			child->rect.y = child->rect.y - self->rect.y;

			width = child->rect.w;
			height = child->rect.h;

			if((MENU_HORIZONTAL_UP == menu->format) ||
					(MENU_HORIZONTAL_DOWN == menu->format))
			{
				xPos += width;
			}
			else if((MENU_VERTICAL_LEFT == menu->format) ||
					(MENU_VERTICAL_RIGHT == menu->format))
			{
				yPos += height;
			}

			child = widget_get_nextsibling(child);
			if(NULL == child)
			{
				child = widget_get_firstchild(self);
			}

			if(child == widget_get_priorsibling(back_next_child))
			{
				break;
			}

			i++;
		}

		width = rect.w;
		rect.w = ((((rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		menu->movable_surface = gal_get_surface((GAL_Rect*)(&rect), gui_core->config.bpp);
		rect.w = width;

		if(NULL == menu->movable_surface)
		{
			gxlogd("[GUI]Movable surface cannot be created!\n");
			return (1);
		}

		return (0);
	}

	/*Create Thread*/
	if(DIRECTION_SILL != menu->direction)
	{
		GxCore_ThreadCreate(self->name,
				&thrad_id,
				dynamic_menu,
				(void *)self,
				1024*100,
				GXOS_DEFAULT_PRIORITY + 1);
	}

	return (0);
}

static int menu_release(GuiWidget *self)
{
	GuiWidget *child = NULL;
	GuiMenu *menu = NULL;

	if(NULL == self)
	{
		return (1);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		widget_get_nextsibling(child);
	}

	gal_free_surface(menu->back_surface);
	menu->back_surface = NULL;
	gal_free_surface(menu->movable_surface);
	menu->movable_surface = NULL;

	GxCore_MutexDelete(menu_mutex);

	if(menu->destroy_event)
	{
		exec_widget_event_data(self, menu->destroy_event, NULL);
	}

	GUI_FREE(self->object);

	return (0);
}

static int _horizontal_keypress(GuiWidget *self, GUI_Event *event)
{
	GuiMenu *menu = NULL;
	GuiWidget *child = NULL;
	int value = GUI_FALSE;

	if((NULL == self) || (NULL == event))
	{
		return (EVENT_TRANSFER_STOP);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (EVENT_TRANSFER_STOP);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp((const char *)menu->focused_list, child->name))
		{
			break;
		}
		child = widget_get_nextsibling(child);
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(GUIK_LEFT == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			child = widget_get_priorsibling(child);
			if(NULL == child)
			{
				child = widget_get_lastchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			menu->focused_list = (unsigned char *)child->name;
			menu->direction = DIRECTION_RIGHT;
			widget_set_update(self, GUI_TRUE);
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
		}
		else if(GUIK_RIGHT == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			child = widget_get_nextsibling(child);
			if(NULL == child)
			{
				child = widget_get_firstchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			menu->focused_list = (unsigned char *)child->name;
			menu->direction = DIRECTION_LEFT;
			widget_set_update(self, GUI_TRUE);
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
		}
		else if(GUIK_UP == event->key.sym)
		{
			GUI_SendEvent((const char*)menu->focused_list, event);
			menu->direction = DIRECTION_SILL;
		}
		else if(GUIK_DOWN == event->key.sym)
		{
			GUI_SendEvent((const char*)menu->focused_list, event);
			menu->direction = DIRECTION_SILL;
		}
		else if(GUIK_RETURN == event->key.sym)
		{
			GUI_SendEvent((const char*)menu->focused_list, event);
			menu->direction = DIRECTION_SILL;
		}
		else
		{
			;
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);
}

static int _vertical_keypress(GuiWidget *self, GUI_Event *event)
{
	GuiMenu *menu = NULL;
	GuiWidget *child = NULL;
	int value = GUI_FALSE;

	if((NULL == self) || (NULL == event))
	{
		return (EVENT_TRANSFER_STOP);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (EVENT_TRANSFER_STOP);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp((const char*)menu->focused_list, child->name))
		{
			break;
		}
		child = widget_get_nextsibling(child);
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(GUIK_UP == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			child = widget_get_priorsibling(child);
			if(NULL == child)
			{
				child = widget_get_lastchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			menu->focused_list = (unsigned char *)child->name;
			menu->direction = DIRECTION_DOWN;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			widget_set_update(self, GUI_TRUE);
		}
		else if(GUIK_DOWN == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			child = widget_get_nextsibling(child);
			if(NULL == child)
			{
				child = widget_get_firstchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			menu->focused_list = (unsigned char *)child->name;
			menu->direction = DIRECTION_UP;
			value = GUI_FALSE;
			GUI_SetProperty((const char *)menu->focused_list, "trigger", (void *)&value);
			widget_set_update(self, GUI_TRUE);
		}
		else
		{
			;
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);
}


static int menu_event(GuiWidget *self, void *data)
{
	GuiMenu *menu = NULL;
	GUI_Event *event = NULL;
	int ret = EVENT_TRANSFER_STOP;

	WIDGET_CHECK_OBJECT(self, data, menu, GuiMenu);

	event = (GUI_Event *)data;

	if(menu->keypress_event)
	{
		ret = exec_widget_event_data(self, menu->keypress_event, data);
		if(EVENT_TRANSFER_STOP == ret)
		{
			return ret;
		}
	}

	GxCore_MutexLock(menu_mutex);
	if((MENU_HORIZONTAL_UP == menu->format) ||
			(MENU_HORIZONTAL_DOWN == menu->format))
	{
		ret = _horizontal_keypress(self, event);
		GxCore_MutexUnlock(menu_mutex);
		return (ret);
	}
	else if((MENU_VERTICAL_LEFT == menu->format) ||
			(MENU_VERTICAL_RIGHT == menu->format))
	{
		ret = _vertical_keypress(self, event);
		GxCore_MutexUnlock(menu_mutex);
		return (ret);
	}

	GxCore_MutexUnlock(menu_mutex);

	return (ret);
}

static int menu_set_property(GuiWidget *self, char *name, void *data)
{
	return (0);
}

static int menu_get_property(GuiWidget *self, char *name, void *data)
{
	GuiMenu *menu = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	menu = (GuiMenu *)(self->object);
	if(NULL == menu)
	{
		return (1);
	}

	if(0 == strcasecmp("back_surface", name))
	{
		if(NULL == data)
		{
			return (1);
		}
		*(void**)data = menu->back_surface;
	}
	else if(0 == strcasecmp("movable_surface", name))
	{
		if(NULL == data)
		{
			return (1);
		}
		*(void**)data = menu->movable_surface;
	}
	else if(0 == strcasecmp("format", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		*(int *)data = menu->format;
	}
	else if(0 == strcasecmp("direction", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		*(int *)data = menu->direction;
	}
	else
	{
		return (1);
	}

	return (0);
}

GuiWidgetOps menu_ops =
{
	.owner = "menu",
	.create = create_menu,
	.release = menu_release,
	.draw = menu_draw,
	.event = menu_event,
	.set = menu_set_property,
	.get = menu_get_property
};


