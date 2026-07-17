#include "all_widget.h"

extern handle_t menu_mutex;

static int _config_menulist_parametre(GuiWidget *self)
{
	GuiMenuList *menulist = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	value = widget_get_property(self, "alignment");
	if((NULL != value) || (0 == menulist->alignment))
	{
		menulist->alignment = widget_get_alignment(self);
	}

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == menulist->font))
	{
		menulist->font = (char *)value;
	}

	value = widget_get_property(self, "unfocus_image");
	if((NULL != value) || (NULL == menulist->image[0]))
	{
		menulist->image[0] = widget_img_load(value);
	}

	value = widget_get_property(self, "focus_image");
	if((NULL != value) || (NULL == menulist->image[1]))
	{
		menulist->image[1] = widget_img_load(value);
	}

	value = widget_get_property(self, "disable_image");
	if((NULL != value) || (NULL == menulist->image[2]))
	{
		menulist->image[2] = widget_img_load(value);
	}

	value = widget_get_property(self, "unfocus_item_image");
	if((NULL != value) || (NULL == menulist->item_image[0]))
	{
		menulist->item_image[0] = widget_img_load(value);
	}

	value = widget_get_property(self, "focus_item_image");
	if((NULL != value) || (NULL == menulist->item_image[1]))
	{
		menulist->item_image[1] = widget_img_load(value);
	}

	value = widget_get_property(self, "disable_item_image");
	if((NULL != value) || (NULL == menulist->item_image[2]))
	{
		menulist->item_image[2] = widget_img_load(value);
	}

	return (0);
}

static void *create_menulist(GuiWidget *self)
{
	GuiWidget *child = NULL, *style = NULL;
	GuiMenuList *menulist = NULL, *menulist_style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	menulist = (GuiMenuList *)GUI_MALLOC(sizeof(GuiMenuList));
	if(NULL == menulist)
	{
		return (NULL);
	}
	memset(menulist, 0, sizeof(GuiMenuList));

	self->object = (void *)(menulist);
	menulist->base = self;

	self->state = WIDGET_STATE_FOCUSSABLE;

	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		menulist_style = (GuiMenuList *)GUI_MALLOC(sizeof(GuiMenuList));
		if(NULL == menulist_style)
		{
			return (NULL);
		}

		memset(menulist_style, 0, sizeof(GuiMenuList));

		style->object = (void *)menulist_style;

		_config_menulist_parametre(style);

		memcpy(menulist, menulist_style, sizeof(GuiMenuList));

		GUI_FREE(style->object);
	}

	_config_menulist_parametre(self);

	menulist->string = (char *)widget_get_property(self, "string");

	child = widget_get_firstchild(self);
	if(child)
	{
		child->state |= WIDGET_STATE_FOCUSED;
		menulist->focus_widget = child;
		menulist->trigger = 1;
	}
	while(child)
	{
		widget_create(child);
		if(menulist->item_image[0])
		{
			GUI_SetProperty(child->name,
					"unfocus_back_image",
					menulist->item_image[0]);
		}

		if(menulist->item_image[1])
		{
			GUI_SetProperty(child->name,
					"focus_back_image",
					menulist->item_image[1]);
		}

		if(menulist->item_image[2])
		{
			GUI_SetProperty(child->name,
					"disable_back_image",
					menulist->item_image[2]);
		}

		GUI_SetProperty(child->name, "alignment", &(menulist->alignment));
		GUI_SetProperty(child->name, "font", menulist->font);

		child->rect.x += self->rect.x;
		child->rect.y += self->rect.y;

		child = widget_get_nextsibling(child);
	}

	return (menulist);
}

static int menulist_release(GuiWidget *self)
{
	GuiMenuList *menulist = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	//gal_free_surface(menulist->surface);
	gal_free_surface(menulist->back_surface);

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	GUI_FREE(self->object);

	return (0);
}

static void dynamic_menulist(void *data)
{
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL, *parent = NULL, *child = NULL;
	GuiMenuList *menulist = NULL;
	void *surface = NULL, *back_surface = NULL;
	unsigned int i = 0, offset = 0, xPos = 0, yPos = 0;
	GUI_Rect rect = {0}, back_rect = {0}, move_rect = {0};
	int format = 0, width = 0;
	int direction = 0;

	GxCore_ThreadDetach();
	self = (GuiWidget*)data;
	if(NULL == self)
	{
		return;
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return;
	}

	gui_core = GUI_GetCurrent();

	surface = menulist->surface;
	if(NULL == surface)
	{
		return;
	}

	GxCore_MutexLock(menu_mutex);

	parent = widget_get_parent(self);
	GUI_GetProperty(parent->name, "format", &format);

	if((MENU_HORIZONTAL_UP == format) ||
			(MENU_HORIZONTAL_DOWN == format))
	{
		offset = menulist->height;
	}
	else if((MENU_VERTICAL_LEFT == format) ||
			(MENU_VERTICAL_RIGHT == format))
	{
		offset = menulist->width;
	}

	xPos = self->rect.x + parent->rect.x;
	yPos = self->rect.y + parent->rect.y;

	if(MENU_HORIZONTAL_UP == format)
	{
		yPos = yPos - menulist->height;
		rect = self->rect;
		rect.y = yPos;
		rect.w = menulist->width;
		rect.h = menulist->height;
	}
	else if(MENU_HORIZONTAL_DOWN == format)
	{
		yPos = yPos + self->rect.h;
		rect = self->rect;
		rect.y = yPos;
		rect.w = menulist->width;
		rect.h = menulist->height;
	}
	else if(MENU_VERTICAL_LEFT == format)
	{
		xPos = xPos - menulist->width;
	}
	else if(MENU_VERTICAL_RIGHT == format)
	{
		xPos = xPos + self->rect.w;
	}

	GUI_GetProperty(parent->name, "direction", &direction);

	if((DIRECTION_LEFT == direction) || (DIRECTION_UP == direction))
	{
		child = widget_get_nextsibling(self);
		if(NULL == child)
		{
			child = widget_get_firstchild(parent);
		}
	}
	else if((DIRECTION_RIGHT == direction) || (DIRECTION_DOWN == direction))
	{
		child = widget_get_priorsibling(self);
		if(NULL == child)
		{
			child = widget_get_lastchild(parent);
		}
	}

	if(child)
	{
		GUI_GetProperty(child->name, "back_surface", &back_surface);

		if(back_surface)
		{
			gal_stretch_surface(back_surface,
					(GAL_Rect*)(&back_rect),
					screen,
					(GAL_Rect*)(&rect));
		}
	}

	/*Capture surface area or paste surface area*/
	if(NULL == menulist->back_surface)
	{
		back_rect = rect;
		back_rect.x = back_rect.y = 0;
		width = back_rect.w;
		back_rect.w = ((((back_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		menulist->back_surface = gal_get_surface((GAL_Rect*)(&rect),
				gui_core->config.bpp);
		back_rect.w = width;

		gdi_commit();
		gal_copy_surface(screen,
				(GAL_Rect*)(&rect),
				menulist->back_surface,
				(GAL_Rect*)(&back_rect));
		gdi_commit();
	}

	i = 0;
	while(i < offset)
	{
		if(MENU_HORIZONTAL_UP == format)
		{
			move_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
			move_rect.y = parent->rect.y - i;
			move_rect.w = menulist->width;
			move_rect.h = i;

			rect.x = 0;
			rect.y = 0;
			rect.w = menulist->width;
			rect.h = i;
		}
		else if(MENU_HORIZONTAL_DOWN == format)
		{
			move_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
			move_rect.y = yPos;
			move_rect.w = menulist->width;
			move_rect.h = i;

			rect.x = 0;
			rect.y = menulist->height - i;
			rect.w = menulist->width;
			rect.h = i;
		}
		else if(MENU_VERTICAL_LEFT == format)
		{
			move_rect.x = xPos;
			move_rect.y = self->rect.y;
			move_rect.w = i;
			move_rect.h = menulist->height;

			rect.x = 0;
			rect.y = 0;
			rect.w = i;
			rect.h = menulist->height;
		}
		else if(MENU_VERTICAL_RIGHT == format)
		{
			move_rect.x = xPos;
			move_rect.y = self->rect.y;
			move_rect.w = i;
			move_rect.h = menulist->height;

			rect.x = menulist->width - i;
			rect.y = 0;
			rect.w = i;
			rect.h = menulist->height;
		}

		gal_stretch_surface(surface,
				(GAL_Rect*)(&rect),
				screen,
				(GAL_Rect*)(&move_rect));

		gui_delay(6);
		i += 4;

		if(i >= offset)
		{
			i = offset;

			if(MENU_HORIZONTAL_UP == format)
			{
				move_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
				move_rect.y = parent->rect.y - i;
				move_rect.w = menulist->width;
				move_rect.h = i;

				rect.x = 0;
				rect.y = 0;
				rect.w = menulist->width;
				rect.h = i;
			}
			else if(MENU_HORIZONTAL_DOWN == format)
			{
				move_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
				move_rect.y = yPos;
				move_rect.w = menulist->width;
				move_rect.h = i;

				rect.x = 0;
				rect.y = menulist->height - i;
				rect.w = menulist->width;
				rect.h = i;
			}
			else if(MENU_VERTICAL_LEFT == format)
			{
				move_rect.x = xPos;
				move_rect.y = self->rect.y;
				move_rect.w = i;
				move_rect.h = menulist->height;

				rect.x = 0;
				rect.y = 0;
				rect.w = i;
				rect.h = menulist->height;
			}
			else if(MENU_VERTICAL_RIGHT == format)
			{
				move_rect.x = xPos;
				move_rect.y = self->rect.y;
				move_rect.w = i;
				move_rect.h = menulist->height;

				rect.x = menulist->width - i;
				rect.y = 0;
				rect.w = i;
				rect.h = menulist->height;
			}

			gal_stretch_surface(surface,
					(GAL_Rect*)(&rect),
					screen,
					(GAL_Rect*)(&move_rect));
		}

		gdi_commit();
	}

	menulist->trigger = GUI_FALSE;

	self->need_update = GUI_TRUE;

	GxCore_MutexUnlock(menu_mutex);
}

static int menulist_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiWidget *child = NULL, *parent = NULL, *last_child = NULL;
	GuiMenuList *menulist = NULL;
	void *surface = NULL;
	int index = 0, thrad_id = 0, color = 0;
	GUI_Rect surface_rect = {0};
	int format, i = 0, height = 0;
	int direction;

	if(NULL == self)
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	parent = widget_get_parent(self);
	GUI_GetProperty(parent->name, "movable_surface", &surface);
	GUI_GetProperty(parent->name, "format", &format);

	surface = surface ? (surface) : (screen);

	index = widget_get_state_index(self);

	if((gui_core->config.mirror) && (self->disable_mirror)) {
		gal_img_desc_mirror(surface,
				    menulist->image[index],
				    self->rect.x,
				    self->rect.y,
				    self->rect.w,
				    self->rect.h,
				    REVERSE_HORIZONTAL);
	} else {
		gal_img_desc(surface,
			     menulist->image[index],
			     self->rect.x,
			     self->rect.y,
			     self->rect.w,
			     self->rect.h);
	}
	if((1 == index) && (menulist->trigger))
	{
		child = widget_get_firstchild(self);

		if(NULL == menulist->surface)
		{
			last_child = widget_get_lastchild(self);
			if(last_child)
			{
				surface_rect = self->rect;

				surface_rect.x = surface_rect.y = 0;

				menulist->width = last_child->rect.w;
				menulist->height = surface_rect.h = last_child->rect.y +
					last_child->rect.h - child->rect.y;
				surface_rect.w = menulist->width;
				surface_rect.h = menulist->height;

				surface_rect.w = ((((surface_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
				menulist->surface = gal_get_surface((GAL_Rect *)(&surface_rect),
						gui_core->config.bpp);
				if(NULL == menulist->surface)
				{
					gxlogd("[GUI]Create menulist surface failed!\n");
				}
				surface_rect.w = menulist->width;
			}
		}

		gdi_commit();
		while(child)
		{
			widget_draw(child);
			child = widget_get_nextsibling(child);
		}
		gdi_commit();

		GxCore_ThreadCreate(self->name,
				&thrad_id,
				dynamic_menulist,
				(void *)self,
				1024*50,
				GXOS_DEFAULT_PRIORITY + 1);

	}
	else if((1 == index) && (!menulist->trigger))
	{
		GUI_GetProperty(parent->name, "direction", &direction);
		GUI_GetProperty(parent->name, "format", &format);

		if((DIRECTION_LEFT == direction) || (DIRECTION_UP == direction))
		{
			child = widget_get_priorsibling(self);
			if(NULL == child)
			{
				child = widget_get_lastchild(parent);
			}
		}
		else if((DIRECTION_RIGHT == direction) || (DIRECTION_DOWN == direction))
		{
			child = widget_get_nextsibling(self);
			if(NULL == child)
			{
				child = widget_get_firstchild(parent);
			}
		}
		else
		{
			child = widget_get_firstchild(self);
			while(child)
			{
				height += child->rect.h;
				child = widget_get_nextsibling(child);
			}

			child = widget_get_firstchild(self);
			i = 0;
			while(child)
			{
				if(child->need_update)
				{
					surface_rect = child->rect;
					child->rect.x = parent->rect.x + (parent->rect.w - child->rect.w) / 2;

					if(MENU_HORIZONTAL_UP == format)
					{
						child->rect.y += (parent->rect.y - height);
					}
					else if(MENU_HORIZONTAL_DOWN == format)
					{
						child->rect.y += (self->rect.h + parent->rect.y);
					}

					widget_draw(child);
					i++;
					child->rect = surface_rect;
				}
				child = widget_get_nextsibling(child);
			}

			if((0 == menulist->width) || (0 == menulist->height))
			{
				child = widget_get_lastchild(self);
				menulist->width = child->rect.w;
				menulist->height = height;//child->rect.y + child->rect.h;
			}
			return (1);
		}

		menulist = (GuiMenuList *)(child->object);
		if(NULL == menulist)
		{
			return (1);
		}

		if(MENU_HORIZONTAL_DOWN == format)
		{
			surface_rect = self->rect;
			surface_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
			surface_rect.y += (self->rect.h + parent->rect.y);
			surface_rect.w = menulist->width;
			surface_rect.h = menulist->height;

			color = gal_color2index(screen, self->back_color[0]);

			gal_fillrect(screen, (GAL_Rect*)(&surface_rect), color);
		}
		else if(MENU_HORIZONTAL_UP == format)
		{
			surface_rect = self->rect;
			surface_rect.x = parent->rect.x + (parent->rect.w - menulist->width) / 2;
			surface_rect.y += (parent->rect.y - menulist->height);
			surface_rect.w = menulist->width;
			surface_rect.h = menulist->height;

			color = gal_color2index(screen, self->back_color[0]);

			gal_fillrect(screen, (GAL_Rect*)(&surface_rect), color);
		}
	}

	if(WIDGET_STATE_FOCUSED == self->state)
	{
		menulist->trigger = 1;
	}

	return (0);
}

static int menulist_event(GuiWidget *self, void *data)
{
	GuiMenuList *menulist = NULL;
	GuiWidget *parent = NULL, *child = NULL;
	GUI_Event *event = NULL;

	if((NULL == self) || (NULL == data))
	{
		return (EVENT_TRANSFER_STOP);
	}

	parent = widget_get_parent(self);
	menulist = (GuiMenuList *)(self->object);
	event = (GUI_Event *)data;

	child = menulist->focus_widget;
	if(NULL == child)
	{
		return (EVENT_TRANSFER_STOP);
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(GUIK_UP == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;\
							widget_set_update(child, GUI_TRUE);
			child = widget_get_priorsibling(child);
			if(NULL == child)
			{
				child = widget_get_lastchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			widget_set_update(child, GUI_TRUE);
			menulist->focus_widget = child;
			self->need_update = GUI_TRUE;
			parent->need_update = GUI_TRUE;
		}
		else if(GUIK_DOWN == event->key.sym)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			widget_set_update(child, GUI_TRUE);
			child = widget_get_nextsibling(child);
			if(NULL == child)
			{
				child = widget_get_firstchild(self);
			}
			child->state |= WIDGET_STATE_FOCUSED;
			widget_set_update(child, GUI_TRUE);
			menulist->focus_widget = child;
			self->need_update = GUI_TRUE;
			parent->need_update = GUI_TRUE;
		}
		else if(GUIK_RETURN == event->key.sym)
		{
			GUI_SendEvent((const char*)menulist->focus_widget->name, event);
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

static int menulist_set_property(GuiWidget *self, char *name, void *data)
{
	GuiMenuList *menulist = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	if(0 == strcasecmp("trigger", name))
	{
		menulist->trigger = *(int *)data;
	}

	return (0);
}

static int menulist_get_property(GuiWidget *self, char *name, void *data)
{
	GuiMenuList *menulist = NULL;
	GuiWidget *parent = NULL;
	int direction;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	if(0 == strcasecmp("surface", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		*(void**)data = menulist->surface;
	}
	else if(0 == strcasecmp("back_surface", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		*(void**)data = menulist->back_surface;
	}
	else if(0 == strcasecmp("direction", name))
	{
		parent = widget_get_parent(self);
		GUI_GetProperty(parent->name, "direction", &direction);
		*(int *)data = direction;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int menulist_prepare_image(GuiWidget *self)
{
	GuiMenuList *menulist = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_image");
	if((NULL != value) || (NULL == menulist->image[0]))
	{
		menulist->image[0] = widget_img_load(value);
	}

	value = widget_get_property(self, "focus_image");
	if((NULL != value) || (NULL == menulist->image[1]))
	{
		menulist->image[1] = widget_img_load(value);
	}

	value = widget_get_property(self, "disable_image");
	if((NULL != value) || (NULL == menulist->image[2]))
	{
		menulist->image[2] = widget_img_load(value);
	}

	value = widget_get_property(self, "unfocus_item_image");
	if((NULL != value) || (NULL == menulist->item_image[0]))
	{
		menulist->item_image[0] = widget_img_load(value);
	}

	value = widget_get_property(self, "focus_item_image");
	if((NULL != value) || (NULL == menulist->item_image[1]))
	{
		menulist->item_image[1] = widget_img_load(value);
	}

	value = widget_get_property(self, "disable_item_image");
	if((NULL != value) || (NULL == menulist->item_image[2]))
	{
		menulist->item_image[2] = widget_img_load(value);
	}

	return (0);
}

static int menulist_clear_image(GuiWidget *self)
{
	GuiMenuList *menulist = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	menulist = (GuiMenuList *)(self->object);
	if(NULL == menulist)
	{
		return (1);
	}

	for(i = 0; i < 3; i++)
	{
		if(menulist->image[i])
		{
			widget_img_clear(&(menulist->image[i]));
		}

		if(menulist->item_image[i])
		{
			widget_img_clear(&(menulist->item_image[i]));
		}
	}

	return (0);
}

GuiWidgetOps menulist_ops =
{
	.owner = "menulist",
	.create = create_menulist,
	.release = menulist_release,
	.prepare_image = menulist_prepare_image,
	.draw = menulist_draw,
	.clear_image   = menulist_clear_image,
	.event = menulist_event,
	.set = menulist_set_property,
	.get = menulist_get_property
};



