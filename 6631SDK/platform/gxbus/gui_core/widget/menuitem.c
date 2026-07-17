#include "all_widget.h"

static void *create_menuitem(GuiWidget *self)
{
	GuiMenuItem *menuitem = NULL;
	char *string = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	menuitem = (GuiMenuItem *)GUI_MALLOC(sizeof(GuiMenuItem));
	if(NULL == menuitem)
	{
		return (NULL);
	}

	self->object = (void *)menuitem;
	menuitem->base = self;

	string = (char *)widget_get_property(self, "string");
	if(string)
	{
		menuitem->string = GUI_STRDUP(string);
	}
	menuitem->link = gui_get_widget(NULL, widget_get_property(self, "link"));

	self->state |= WIDGET_STATE_FOCUSSABLE;

	return (menuitem);
}


static int menuitem_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiMenuItem *menuitem = NULL;
	GuiWidget *parent = NULL;
	char *string = NULL;
	void *surface = NULL;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	int index = 0, color = 0, text_len = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	int direction;

	if(NULL == self)
	{
		return (1);
	}

	menuitem = (GuiMenuItem *)(self->object);
	if(NULL == menuitem)
	{
		return (1);
	}

	index = widget_get_state_index(self);

	if(menuitem->font)
	{
		old_font = gxgdi_set_font(menuitem->font);
	}

	string = (char *)i18n_get_string((const char *)menuitem->string);
	valid_rect = self->rect;

	parent = widget_get_parent(self);
	if(NULL == parent)
	{
		return (1);
	}

	GUI_GetProperty(parent->name, "surface", &surface);

	GUI_GetProperty(parent->name, "direction", &direction);

	if(DIRECTION_SILL == direction)
	{
		surface = screen;
		//valid_rect.x += parent->rect.x;
		//valid_rect.y += parent->rect.y;
		valid_rect = self->rect;
	}

	if(NULL == surface)
	{
		return (1);
	}

	if(menuitem->back_image[index])
	{
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			gal_img_desc_mirror(surface,
					    menuitem->back_image[index],
					    valid_rect.x,
					    valid_rect.y,
					    valid_rect.w,
					    valid_rect.h,
					    REVERSE_HORIZONTAL);
		} else {
			gal_img_desc(surface,
					menuitem->back_image[index],
					valid_rect.x,
					valid_rect.y,
					valid_rect.w,
					valid_rect.h);
		}
	}
	else
	{
		color = gal_color2index(screen, self->back_color[index]);
		gal_fillrect(surface, (GAL_Rect*)(&valid_rect), color);
	}

	if(string)
	{
		color = gal_color2index(surface, self->fore_color[index]);
		text_len = gdi_text_len(string);

		new_font = gxgdi_get_font();
		if(NULL == new_font)
		{
			return (1);
		}

		menuitem->alignment = widget_alignment_revert(self, string, menuitem->alignment);
		widget_string_alignment(menuitem->alignment,
				text_len,
				new_font->ysize,
				&(self->rect), &(valid_rect));

		gxgdi_draw_string(surface,
				string,
				(GAL_Rect*)(&valid_rect),
				&interval,
				menuitem->alignment,
				color,
				SINGLELINE_MODE);
	}

	if(old_font)
	{
		gxgdi_set_font((char *)old_font);
	}

	self->need_update = 0;

	return (0);
}

static int menuitem_release(GuiWidget *self)
{
	GUI_FREE(self->object);
	return (0);
}

static int menuitem_event(GuiWidget *self, void *data)
{
	GuiMenuItem *menuitem = NULL;
	GUI_Event *event = NULL;

	if((NULL == self) || (NULL == data))
	{
		return (EVENT_TRANSFER_STOP);
	}

	menuitem = (GuiMenuItem *)(self->object);
	if(NULL == menuitem)
	{
		return (EVENT_TRANSFER_STOP);
	}

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(GUIK_RETURN == event->key.sym)
		{
			GUI_CreateDialog((const char *)menuitem->link);
		}
		return (EVENT_TRANSFER_STOP);
	case GUI_MOUSEBUTTONDOWN:
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);
}

static int menuitem_set_property(GuiWidget *self, char *name, void *data)
{
	GuiMenuItem *menuitem = NULL;
	char *string = NULL;
	int value = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	menuitem = (GuiMenuItem *)(self->object);
	if(NULL == menuitem)
	{
		return (1);
	}

	if(0 == strcasecmp("alignment", name))
	{
		value = *(int *)data;
		menuitem->alignment = value;
	}
	else if(0 == strcasecmp("font", name))
	{
		string = (char *)data;
		menuitem->font = (char *)string;
	}
	else
	{
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
		return (0);
	}

	return (0);
}

static int menuitem_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

GuiWidgetOps menuitem_ops =
{
	.owner = "menuitem",
	.create = create_menuitem,
	.release = menuitem_release,
	.draw = menuitem_draw,
	.event = menuitem_event,
	.set = menuitem_set_property,
	.get = menuitem_get_property
};


