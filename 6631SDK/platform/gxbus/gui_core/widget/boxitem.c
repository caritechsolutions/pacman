#include "all_widget.h"

static void *create_boxitem(GuiWidget *self)
{
	GuiBoxItem *boxitem = NULL, *boxitem_style = NULL;
	GuiWidget *child = NULL, *parent = NULL, *style = NULL;
	char *string = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	boxitem = (GuiBoxItem *)GUI_MALLOC(sizeof(GuiBoxItem));
	if(NULL == boxitem)
	{
		return (NULL);
	}

	memset(boxitem, 0, sizeof(GuiBoxItem));

	boxitem->active = 0;

	self->object = (void *)boxitem;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		boxitem_style = (GuiBoxItem *)GUI_MALLOC(sizeof(GuiBoxItem));
		if(NULL == boxitem_style)
		{
			GUI_FREE(boxitem);
			return (NULL);
		}

		memset(boxitem_style, 0, sizeof(GuiBoxItem));

		style->object = (void *)(boxitem_style);

		memcpy(boxitem, boxitem_style, sizeof(GuiBoxItem));
		GUI_FREE(style->object);
	}

	string = (char *)widget_get_property(self, "state");
	if(string)
	{
		if(0 == strcasecmp("disable", string))
		{
			self->state |= WIDGET_STATE_DISABLE;
			boxitem->active = 2;
		}
		else if(0 == strcasecmp("hide", string))
		{
			self->state |= WIDGET_STATE_HIDE;
		}
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_create(child);

		if(self->state & WIDGET_STATE_DISABLE)
		{
			child->state |= WIDGET_STATE_DISABLE;
		}

		if(widget_get_focussable(child))
		{
			boxitem->focused = child;
		}

		child = widget_get_nextsibling(child);
	}

	return (boxitem);
}

static int boxitem_release(GuiWidget *self)
{
	GuiBoxItem *boxitem = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	boxitem = (GuiBoxItem *)(self->object);

	if(NULL == boxitem)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	if(boxitem->surface)
	{
		hd_free_surface(boxitem->surface);
		boxitem->surface = NULL;
	}
	//GUI_FREE(boxitem->image[0]);
	//GUI_FREE(boxitem->image[1]);
	//GUI_FREE(boxitem->image[2]);

	GUI_FREE(self->object);

	return (0);
}

static int _check_rect(GUI_Rect *item_rect, GUI_Rect *widget_rect)
{
	if(NULL == item_rect || NULL == widget_rect)
	{
		return (GUI_FALSE);
	}

	if(widget_rect->x < item_rect->x)
	{
		return (GUI_FALSE);
	}

	if(widget_rect->y < item_rect->y)
	{
		return (GUI_FALSE);
	}

	if((widget_rect->x + widget_rect->w) > (item_rect->x + item_rect->w))
	{
		return (GUI_FALSE);
	}

	if((widget_rect->y + widget_rect->h) > (item_rect->y + item_rect->h))
	{
		return (GUI_FALSE);
	}

	return (GUI_TRUE);
}

static int boxitem_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiBoxItem *boxitem = NULL;
	GuiBox *box = NULL;
	GuiWidget *child = NULL, *parent = NULL;
	int index = 0, color = 0, old_state = 0;

	if(NULL == self)
	{
		return (1);
	}

	boxitem = (GuiBoxItem *)(self->object);
	if(NULL == boxitem)
	{
		return (1);
	}

	if(widget_get_update(self))
	{
		if((self->back_color[0] == gui.config.gui_trans) ||
		   (self->back_color[1] == gui.config.gui_trans))
		{
			if(boxitem->surface == NULL)
			{
				boxitem->surface = widget_get_surface(&(self->rect));
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
				hd_copy_surface(boxitem->surface, &src_rect, screen, &dst_rect);
			}

			child = widget_get_firstchild(self);
			while(child)
			{
				GUI_SetProperty(child->name, "update_surface", NULL);
				child = widget_get_nextsibling(child);
			}
		}
		index = boxitem->active;
		if(2 == widget_get_state_index(self))
		{
			index = 2;
		}
		if(boxitem->image_g[index][1])
		{
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				widget_img_group_draw_mirror(screen,
							     &(self->rect),
							     boxitem->image_g[index][0],
							     boxitem->image_g[index][1],
							     boxitem->image_g[index][2],
							     self->back_color[index],
							     REVERSE_HORIZONTAL);
			} else {
				widget_img_group_draw(screen,
						      &(self->rect),
						      boxitem->image_g[index][0],
						      boxitem->image_g[index][1],
						      boxitem->image_g[index][2],
						      self->back_color[index]);
			}
		}
		else if(boxitem->image[index])
		{
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				gal_img_desc_mirror(screen,
						    (const image_desc * )(boxitem->image[index]),
						    self->rect.x,
						    self->rect.y,
						    self->rect.w,
						    self->rect.h,
						    REVERSE_HORIZONTAL);
			} else {
				gal_img_desc(screen,
						(const image_desc * )(boxitem->image[index]),
						self->rect.x,
						self->rect.y,
						self->rect.w,
						self->rect.h);
			}
		}
		else
		{
			index = boxitem->active;
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			gal_fillrect(screen, (GAL_Rect *)(&(self->rect)), color);
		}
	}

	parent = widget_get_parent(self);
	if(0 == strcasecmp("box", parent->classname))
	{
		box = (GuiBox *)(parent->object);
	}

	if(NULL == box)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_mirror_rect(child);
		if(widget_get_update(self) &&
				(GUI_TRUE == _check_rect(&self->rect, &child->rect)) &&
				!(child->state & WIDGET_STATE_HIDE))
		{
			if((0 == strcasecmp("sliderbar", child->classname)) ||
					(0 == strcasecmp("progbar", child->classname)))
			{
				GUI_SetProperty(child->name, "update", NULL);
			}
			old_state = child->state;

			widget_draw(child);
			widget_set_update(child, GUI_FALSE);
			child->state = old_state;
		}
		child = widget_get_nextsibling(child);
	}

	return (0);
}

static int boxitem_event(GuiWidget *self, void *data)
{
	GuiBoxItem *boxitem = NULL;
	GuiWidget *child = NULL;
	GuiCore *gui_core = NULL;
	GUI_Event* event = NULL;
	int ret = 0;

	if(NULL == data)
	{
		return (EVENT_TRANSFER_KEEPON);
	}

	boxitem = (GuiBoxItem*)(self->object);
	if(NULL == boxitem)
	{
		return (EVENT_TRANSFER_KEEPON);
	}

	gui_core = GUI_GetCurrent();
	event = (GUI_Event *)data;

	if(event->type == GUI_KEYDOWN) {
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
	}

	if(boxitem->focused && boxitem->focused->ops && boxitem->focused->ops->event)
	{
		if(!(WIDGET_STATE_HIDE & boxitem->focused->state))
		{
			ret = boxitem->focused->ops->event(boxitem->focused, data);
		}
		else
		{
			child = widget_get_nextsibling(boxitem->focused);
			while(child)
			{
				if((WIDGET_STATE_FOCUSSABLE & child->state) &&
						!(WIDGET_STATE_HIDE & child->state))
				{
					break;
				}
				child = widget_get_nextsibling(child);
			}

			if(child)
			{
				boxitem->focused = child;
			}

			ret = boxitem->focused->ops->event(boxitem->focused, data);
		}
		//widget_set_update(boxitem->focused, GUI_TRUE);

		return (ret);
	}
	else
	{
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		return (EVENT_TRANSFER_KEEPON);
	}

	return (EVENT_TRANSFER_KEEPON);
}

static int boxitem_set_property(GuiWidget *self, char *name, void *data)
{
	GuiBoxItem *boxitem = NULL;
	GuiWidget *child = NULL, *parent = NULL, *focus = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	boxitem = (GuiBoxItem *)(self->object);
	if(NULL == boxitem)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(child->state & WIDGET_STATE_FOCUSSABLE)
		{
			break;
		}
		child = widget_get_nextsibling(child);
	}

	if(0 == strcasecmp("active", name))
	{
		if(((self->state & WIDGET_STATE_DISABLE) ||
					(self->state & WIDGET_STATE_HIDE)))
		{
			return (0);
		}

		boxitem->active = 1;

		self->state &= WIDGET_STATE_FOCUSED;

		child = widget_get_firstchild(self);
		while(child)
		{
			child->state |= WIDGET_STATE_FOCUSED;
			//widget_set_update(child, GUI_TRUE);
			child->need_update = GUI_TRUE;
			child = widget_get_nextsibling(child);
		}

		widget_set_update(self, GUI_TRUE);

	}
	else if(0 == strcasecmp("unactive", name))
	{
		if(((self->state & WIDGET_STATE_DISABLE) ||
					(self->state & WIDGET_STATE_HIDE)))
		{
			return (0);
		}

		boxitem->active = 0;

		self->state &= WIDGET_STATE_FOCUSSABLE;

		child = widget_get_firstchild(self);
		while(child)
		{
			child->state &= ~WIDGET_STATE_FOCUSED;
			if(!(child->state & WIDGET_STATE_HIDE))
			{
				//widget_set_update(child, GUI_TRUE);
				child->need_update = GUI_TRUE;
			}
			child = widget_get_nextsibling(child);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("disable", name))
	{
		parent = widget_get_parent(self);

		if(!(WIDGET_STATE_DISABLE & self->state))
		{
			child = widget_get_firstchild(self);
			while(child)
			{
				child->state &= ~WIDGET_STATE_FOCUSED;
				child->state |= WIDGET_STATE_DISABLE;
				//widget_set_update(child, GUI_TRUE);
				child->need_update = GUI_TRUE;
				child = widget_get_nextsibling(child);
			}

			self->state |= WIDGET_STATE_DISABLE;
			boxitem->active = 2;
		}
		else
		{
			boxitem->active = 2;
		}

		GUI_GetProperty(parent->name, "focus", &focus);
		if(focus == self)
		{
			while((focus->state & WIDGET_STATE_DISABLE) ||
					(focus->state & WIDGET_STATE_HIDE))
			{
				focus = widget_get_nextsibling(focus);
				if(NULL == focus)
				{
					focus = widget_get_firstchild(parent);
				}
				if(focus == widget_get_lastchild(parent) &&
				   ((focus->state & WIDGET_STATE_DISABLE) ||
				    (focus->state & WIDGET_STATE_HIDE))) {
					focus = NULL;
					break;
				}
			}
			GUI_SetProperty(parent->name, "focus", focus);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("enable", name))
	{
		if(WIDGET_STATE_DISABLE & self->state)
		{
			child = widget_get_firstchild(self);
			while(child)
			{
				child->state &= ~WIDGET_STATE_DISABLE;
				if(widget_get_focussable(child))
				{
					boxitem->focused = child;
				}

				widget_set_update(child, GUI_TRUE);
				child = widget_get_nextsibling(child);
			}

			self->state &= ~WIDGET_STATE_DISABLE;
			boxitem->active = 0;
		}
		else
		{
			boxitem->active = 0;
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("update", name))
	{
		widget_set_update(self, GUI_TRUE);

		parent = widget_get_parent(self);
		if(parent)
		{
			if(0 == strcasecmp("box", parent->classname))
			{
				parent->need_update = GUI_TRUE;
			}
		}
	}
	else if((0 == strcasecmp("unfocus_image", name))||
			(0 == strcasecmp("focus_image", name)) ||
			(0 == strcasecmp("disable_image", name)))
	{
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);

		parent = widget_get_parent(self);
		if(parent)
		{
			if(0 == strcasecmp("box", parent->classname))
			{
				parent->need_update = GUI_TRUE;
			}
		}
	}
	else
	{
		return (1);
	}

	return (0);
}

static int boxitem_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

static int boxitem_prepare_image(GuiWidget *self)
{
	GuiBoxItem *boxitem = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	boxitem = (GuiBoxItem *)(self->object);
	if(NULL == boxitem)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_image");
	if((NULL != value) || (NULL == boxitem->image[0]) || (NULL == boxitem->image_g[0][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(boxitem->image_g[0][0]),
								  &(boxitem->image_g[0][1]),
								  &(boxitem->image_g[0][2]));
		}
		else
		{
			boxitem->image[0] = widget_img_load(value);
			boxitem->image_g[0][0] = boxitem->image_g[0][1] = boxitem->image_g[0][2] = NULL;
		}
	}
	value = widget_get_property(self, "focus_image");
	if((NULL != value) || (NULL == boxitem->image[1]) || (NULL == boxitem->image_g[1][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(boxitem->image_g[1][0]),
								  &(boxitem->image_g[1][1]),
								  &(boxitem->image_g[1][2]));
		}
		else
		{
			boxitem->image[1] = widget_img_load(value);
			boxitem->image_g[1][0] = boxitem->image_g[1][1] = boxitem->image_g[1][2] = NULL;
		}
	}
	value = widget_get_property(self, "disable_image");
	if((NULL != value) || (NULL == boxitem->image[2]) || (NULL == boxitem->image_g[2][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(boxitem->image_g[2][0]),
								  &(boxitem->image_g[2][1]),
								  &(boxitem->image_g[2][2]));
		}
		else
		{
			boxitem->image[2] = widget_img_load(value);
			boxitem->image_g[2][0] = boxitem->image_g[2][1] = boxitem->image_g[2][2] = NULL;
		}
	}

	return (0);
}

static int boxitem_clear_image(GuiWidget *self)
{
	GuiBoxItem *boxitem = NULL;

	if(NULL == self)
	{
		return (1);
	}

	boxitem = (GuiBoxItem *)(self->object);
	if(NULL == boxitem)
	{
		return (1);
	}

	if(boxitem->image[0])
	{
		widget_img_clear(&(boxitem->image[0]));
	}

	if(boxitem->image[1])
	{
		widget_img_clear(&(boxitem->image[1]));
	}

	if(boxitem->image[2])
	{
		widget_img_clear(&(boxitem->image[2]));
	}

	widget_img_group_clear(&(boxitem->image_g[0][0]),
						   &(boxitem->image_g[0][1]),
						   &(boxitem->image_g[0][2]));
	widget_img_group_clear(&(boxitem->image_g[1][0]),
						   &(boxitem->image_g[1][1]),
						   &(boxitem->image_g[1][2]));
	widget_img_group_clear(&(boxitem->image_g[2][0]),
						   &(boxitem->image_g[2][1]),
						   &(boxitem->image_g[2][2]));
	return (0);
}

GuiWidgetOps boxitem_ops =
{
	.owner = "boxitem",
	.create = create_boxitem,
	.release = boxitem_release,
	.prepare_image = boxitem_prepare_image,
	.draw = boxitem_draw,
	.clear_image   = boxitem_clear_image,
	.event = boxitem_event,
	.set = boxitem_set_property,
	.get = boxitem_get_property
};




