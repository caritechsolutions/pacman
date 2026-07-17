#include "all_widget.h"
#include "hd_gal.h"
#include "../../include/gxavdev.h"


void _widget_set_x(GuiWidget *widget, int16_t offset)
{
	GuiCore *gui_core = NULL;
	GuiWidget *child = NULL;

	gui_core = GUI_GetCurrent();
	widget->rect.x += offset;

	child = widget_get_firstchild(widget);
	while(child)
	{
		widget_set_update(child, GUI_TRUE);
		if(0 == strcasecmp("header", widget->classname))
		{
			if(0 != strcasecmp("text", child->classname))
			{
				_widget_set_x(child, offset);
			}
		}
		else
		{
			_widget_set_x(child, offset);
		}

		child = widget_get_nextsibling(child);
	}
	widget->mirrored = GUI_FALSE;
	widget_set_update(widget, GUI_TRUE);
}

void _widget_set_y(GuiWidget *widget, int16_t offset)
{
	GuiWidget *child = NULL;

	widget->rect.y += offset;

	child = widget_get_firstchild(widget);
	while(child)
	{
		widget_set_update(child, GUI_TRUE);
		_widget_set_y(child, offset);

		child = widget_get_nextsibling(child);
	}

	widget_set_update(widget, GUI_TRUE);
}


static int _find_focus_by_mouse( GUI_MouseButton* button)
{
	GuiCore *gui_core = NULL;
	GuiWidget* self = NULL;
	GuiWidget* child = NULL;
	int i = 0;

	gui_core = GUI_GetCurrent();
	i = stack_count(gui_core->win_stack);
	for( ; i > 0; i--)
	{
		self = (GuiWidget*)stack_get(gui_core->win_stack, i-1);
		if(NULL == self)
		{
			return 1;
		}

		//GUI_Printf("=MOUSE= button down in window '%s' ?\n", self->name);
		if( (button->x >= self->rect.x)
				&&(button->y >= self->rect.y)
				&&(button->x < self->rect.x + self->rect.w)
				&&(button->y < self->rect.y + self->rect.h))
		{
			//GUI_Printf("YES\n");
			/*mose button down in child, set focus on it*/
			child = widget_get_lastchild(self);
			while(child)
			{
				if( (button->x >= child->rect.x)
						&&(button->y >= child->rect.y)
						&&(button->x < child->rect.x + child->rect.w)
						&&(button->y < child->rect.y + child->rect.h))
				{
					//GUI_Printf("And on widget '%s' !\n", child->name);
					wm_state_set_focus(child);
					return GUI_TRUE;
				}
				child = widget_get_priorsibling(child);
			}

			/*set focus on window*/
			wm_state_set_focus(self);
			return GUI_TRUE;
		}

	}

	return GUI_FALSE;
}


static int _set_prior_widget(GuiWindow *window)
{
	GuiWidget *focus = NULL;

	if(NULL == window)
	{
		return (1);
	}

	focus = window->active_widget;
	if(NULL == focus)
	{
		return 0;
	}

	if(WIDGET_IS_WINDOW(focus))
	{
		return 0;
	}

	//make sure next loop not dead
	if(!widget_get_focussable(focus))
	{
		return 1;
	}

	focus = widget_get_priorsibling(focus);
	if(NULL == focus)
	{
		focus = widget_get_lastchild(window->base);
	}

	while(focus)
	{
		if(widget_get_focussable(focus))
		{
			if(focus == window->active_widget)
			{
				return 0;
			}

			wm_state_set_focus(focus);
			return 0;
		}

		focus = widget_get_priorsibling(focus);
		if(NULL == focus)
		{
			focus = widget_get_lastchild(window->base);
		}
	}

	return (0);
}

static int _set_next_widget(GuiWindow *window)
{
	GuiWidget *focus = NULL;

	if(NULL == window)
	{
		return (1);
	}

	focus = window->active_widget;
	if(NULL == focus)
	{
		return 0;
	}

	if(WIDGET_IS_WINDOW(focus))
	{
		return 0;
	}

	//make sure next loop not dead
	if(!widget_get_focussable(focus))
	{
		return 1;
	}

	focus = widget_get_nextsibling(focus);
	if(NULL == focus)
	{
		focus = widget_get_firstchild(window->base);
	}

	while(focus)
	{

		if(widget_get_focussable(focus))
		{
			if(focus == window->active_widget)
			{
				return 0;
			}

			wm_state_set_focus(focus);
			return 0;
		}

		focus = widget_get_nextsibling(focus);
		if(NULL == focus)
		{
			focus = widget_get_firstchild(window->base);
		}
	}

	return (0);
}

static GuiWidget* _tab_windows(GuiWidget* self)
{
	GuiCore *gui_core = NULL;
	GuiWidget* next_window = NULL;
	int id = 0;
	int count = 0;

	gui_core = GUI_GetCurrent();
	count = stack_count(gui_core->win_stack);
	if(count <= 1)
	{
		return self;
	}

	id = stack_get_id(gui_core->win_stack, (void*)self);
	if(-1 != id)
	{
		if(id < count -1)
		{
			next_window = (GuiWidget*)stack_get(gui_core->win_stack, id+1);
		}
		else
		{
			next_window = (GuiWidget*)stack_get(gui_core->win_stack, 0);
		}
		wm_state_set_focus(next_window);
	}

	return next_window;
}

static int _add_key(GuiWidget* self,  GUI_Event* event)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWindow *window = NULL;

	if(self)
	{
		window = (GuiWindow*)self->object;
		if(NULL == window)
		{
			return EVENT_TRANSFER_STOP;
		}
	}

	if(NULL == event)
	{
		return (EVENT_TRANSFER_STOP);
	}

	switch(event->key.sym)
	{
	case GUIK_UP:
	case GUIK_LEFT:
		_set_prior_widget(window);
		exec_widget_signal_data(self, "change", (void*)event);
		break;
	case GUIK_DOWN:
	case GUIK_RIGHT:
		_set_next_widget(window);
		exec_widget_signal_data(self, "change", (void*)event);
		break;
	case GUIK_TAB:
		_tab_windows(self);
		break;
	case GUIK_ESCAPE:
		GUI_EndDialog(widget_get_name(self));
		break;
	default:
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		return EVENT_TRANSFER_KEEPON;

	}

	return EVENT_TRANSFER_STOP;
}

static int _add_mouse(GuiWidget* self, GUI_Event* event)
{
	GUI_MouseButton* button = NULL;

	if(NULL == self || NULL == event)
	{
		return EVENT_TRANSFER_STOP;
	}

	button = &(event->button);
	//GUI_Printf("=MOUSE= type:%d, button:%d, x:%d, y:%d\n", button->type, button->button, button->x, button->y);

	// TODO:
	/*button index*/
	if(1 != button->button)
	{
		return EVENT_TRANSFER_STOP;
	}

	switch(button->type)
	{
	case GUI_MOUSEBUTTONDOWN:
		_find_focus_by_mouse(button);
		break;
	}

	return EVENT_TRANSFER_KEEPON;
}

static int _set_window_format(GuiWidget *self)
{
	GuiWindow *window = NULL;
	char *pName = NULL;

	if(NULL == self)
	{
		return (FORMAT_DIALOG);
	}

	window = (GuiWindow *)(self->object);
	if(NULL == window)
	{
		return (FORMAT_DIALOG);
	}

	pName = (char *)widget_get_property(self, "format");
	if(NULL == pName)
	{
		return (FORMAT_DIALOG);
	}

	if(0 == strcasecmp("dialog", pName))
	{
		return (FORMAT_DIALOG);
	}
	else if(0 == strcasecmp("popup", pName))
	{
		window->popup_recorded = GUI_FALSE;
		return (FORMAT_POPUP);
	}
	else if(0 == strcasecmp("movable", pName))
	{
		window->move_origin_x = self->rect.x;
		window->move_origin_y = self->rect.y;
		return (FORMAT_MOVABLE);
	}

	return (FORMAT_DIALOG);
}


static int _config_window_parametre(GuiWidget *self)
{
	GuiWindow *window = NULL;
	const char *value = NULL;

	if (self == NULL || self->object == NULL)
		return 1;

	window = (GuiWindow *)(self->object);

	if (window->format == 0) {
		value = widget_get_property(self, "format");
		if (value)
			window->format = _set_window_format(self);
	}

	value = widget_get_property(self, "back_sync");
	if((value) && (0 == strcasecmp("true", value))) {
		window->back_sync = GUI_TRUE;
	} else {
		window->back_sync = GUI_FALSE;
	}

	return (0);
}

static int _set_window_need_fade(GuiWidget *self, int flag)
{
	GuiWindow *window = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	const char *value = NULL;

	if (self == NULL || self->object == NULL)
		return 1;

	if((0 == self->rect.x) &&
	   (0 == self->rect.y) &&
	   (gui_core->config.width == self->rect.w) &&
	   (gui_core->config.height == self->rect.h))
	{
		window = (GuiWindow *)(self->object);
		value = widget_get_property(self, "fade_style");
		if((NULL == value) ||
		   ((value) && (0 == strcasecmp("no_effect", value)))) {
			window->fade_mode = IMAGE_EFFECT_NONE;
			return 0;
		} else if(value) {
			if(0 == strcasecmp("random", value))
				window->fade_mode = (rand() % IMAGE_CHECKER_BOARD) + 1;
			else if(0 == strcasecmp("slide_left", value))
				window->fade_mode = IMAGE_SLIDE_LEFT;
			else if(0 == strcasecmp("slide_right", value))
				window->fade_mode = IMAGE_SLIDE_RIGHT;
			else if(0 == strcasecmp("slide_top", value))
				window->fade_mode = IMAGE_SLIDE_TOP;
			else if(0 == strcasecmp("slide_bottom", value))
				window->fade_mode = IMAGE_SLIDE_BOTTOM;
			else if(0 == strcasecmp("diagonal_top_left", value))
				window->fade_mode = IMAGE_DIAGONAL_TOP_LEFT;
			else if(0 == strcasecmp("diagonal_top_right", value))
				window->fade_mode = IMAGE_DIAGONAL_TOP_RIGHT;
			else if(0 == strcasecmp("diagonal_bottom_left", value))
				window->fade_mode = IMAGE_DIAGONAL_BOTTOM_LEFT;
			else if(0 == strcasecmp("diagonal_bottom_right", value))
				window->fade_mode = IMAGE_DIAGONAL_BOTTOM_RIGHT;
			else if(0 == strcasecmp("hstep_top_left", value))
				window->fade_mode = IMAGE_HSTEP_TOP_LEFT;
			else if(0 == strcasecmp("hstep_top_right", value))
				window->fade_mode = IMAGE_HSTEP_TOP_RIGHT;
			else if(0 == strcasecmp("hstep_bottom_left", value))
				window->fade_mode = IMAGE_HSTEP_BOTTOM_LEFT;
			else if(0 == strcasecmp("hstep_bottom_right", value))
				window->fade_mode = IMAGE_HSTEP_BOTTOM_RIGHT;
			else if(0 == strcasecmp("checker_board", value))
				window->fade_mode = IMAGE_CHECKER_BOARD;
			window->need_fade = flag;
			window->fade_steps = widget_get_int(self, "fade_steps", 64);
		}
	}

	return 0;
}

static void *window_create(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiWindow *window = NULL, *window_style = NULL;
	GuiWidget *child = NULL, *style = NULL, *stack_win = NULL;
	int id = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	gui_core = GUI_GetCurrent();

	if(((self->rect.x + self->rect.w) > gui_core->config.width) ||
			((self->rect.y + self->rect.h) > gui_core->config.height))
	{
		return (NULL);
	}

	if((self->rect.x == 0) && (self->rect.y == 0) &&
			(self->rect.w == gui_core->config.width) && (self->rect.h == gui_core->config.height))
	{
		id = stack_count(gui_core->win_stack);
		while(id >= 0)
		{
			stack_win = stack_get(gui_core->win_stack, id);
			if(stack_win && stack_win != self)
			{
				GUI_SetProperty(stack_win->name, "free_record", NULL);
			}
			id--;
		}
	}

	window = (GuiWindow *)GUI_MALLOC(sizeof(GuiWindow));
	if(NULL == window)
	{
		return (NULL);
	}
	memset(window, 0, sizeof(GuiWindow));

	window->base = self;
	self->object = (void *)(window);
	self->state = (WIDGET_STATE_FOCUSSABLE | WIDGET_STATE_FOCUSED);//special widget

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		window_style = (GuiWindow *)GUI_MALLOC(sizeof(GuiWindow));
		if(NULL == window_style)
		{
			GUI_FREE(window);
			return (NULL);
		}

		memset(window_style, 0, sizeof(GuiWindow));

		style->object = (void *)window_style;

		_config_window_parametre(style);

		memcpy(window, window_style, sizeof(GuiWindow));
		window->base = self;

		GUI_FREE(window_style);
	}

	_config_window_parametre(self);

	if(self->signal)
	{
		window->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		window->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		window->close_event = (GuiWidgetSignal *)hash_get(self->signal, "close");
		window->show_event = (GuiWidgetSignal *)hash_get(self->signal, "show");
		window->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
		window->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		window->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
		window->service_event = (GuiWidgetSignal *)hash_get(self->signal, "service");
		window->got_focus = (GuiWidgetSignal *)hash_get(self->signal, "got_focus");
		window->lost_focus = (GuiWidgetSignal *)hash_get(self->signal, "lost_focus");
	}
	child = widget_get_firstchild(self);

	while(child)
	{
		widget_create(child);
		child = widget_get_nextsibling(child);
	}

	//set focus on window, return first focussable child and focus on it
	window->active_widget = wm_state_set_focus(self);
	_set_window_need_fade(self, true);

	return (window);
}

static int _window_set_surface(GuiWidget *self)
{
	int ret = 0, color = 0;
	GuiWindow *window = NULL;
	GuiWidget *pre_win = NULL;
	void *surface = NULL, *back_surface = NULL;
	GuiCore *gui_core = NULL;
	GUI_Rect rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	window = (GuiWindow *)(self->object);
	if(NULL == window)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	rect = self->rect;
	rect.x = rect.y = 0;
	surface = gal_get_surface((GAL_Rect*)&rect, gui_core->config.bpp);
	if(NULL == surface)
	{
		return (1);
	}

	gdi_commit();
	color = gal_color2index(screen, gui_core->config.osd_trans);
	gal_fillrect(surface, (GAL_Rect*)&rect, color);
	gdi_commit();

	gal_copy_surface(screen,
			(GAL_Rect*)(&(self->rect)),
			surface,
			(GAL_Rect*)&rect);
	gdi_commit();

	ret = stack_count(gui_core->win_stack) - 2;
	pre_win = stack_get(gui_core->win_stack, ret);

	if(NULL == pre_win)
	{
		pre_win = stack_get(gui_core->win_stack, 0);
	}

	if(NULL == pre_win)
	{
		gal_free_surface(surface);
		return (1);
	}

	GUI_GetProperty(pre_win->name, "surface", &back_surface);

	if(NULL == back_surface)
	{
		GUI_SetProperty(pre_win->name, "surface", &surface);
		GUI_SetProperty(pre_win->name, "surface_rect", &self->rect);
	}
	else if(surface)
	{
		hd_free_surface(surface);
	}

	return ret;
}

static int window_release(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiWindow *window = NULL;
	GuiWidget *child = NULL;
	int ret = 0;
	GuiWidget *pre_win = NULL;
	GuiSurface *tmp_surface = NULL;

	if(NULL == self)
		return (1);

	window = (GuiWindow *)(self->object);
	if(NULL == window)
		return (1);

	gui_core = GUI_GetCurrent();

	if(FORMAT_POPUP == window->format)
	{
		ret = stack_count(gui_core->win_stack) - 2;
		pre_win = stack_get(gui_core->win_stack, ret);

		if(NULL == pre_win)
		{
			pre_win = stack_get(gui_core->win_stack, 0);
		}
		else
		{
			GUI_SetInterface("flush", NULL);
			GUI_SetProperty(pre_win->name, "surface", &tmp_surface);
		}
	}

	if (window->back_ground)
		GUI_FREE(window->back_ground);

	if(window->surface)
	{
		gal_free_surface(window->surface);
		window->surface = NULL;
	}

	if(window->surface_rect)
	{
		GUI_FREE(window->surface_rect);
	}

	if(NULL != window->movable_surface)
	{
		gal_free_surface(window->movable_surface);
		window->movable_surface = NULL;
	}

	if(NULL != window->back_up_surface)
	{
		gal_free_surface(window->back_up_surface);
		window->back_up_surface = NULL;
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	exec_widget_signal_data(self, "destroy", NULL);

	GUI_FREE(self->object);

	return (0);
}

static int _draw_popup_back(void)
{
	int ret = 0;
	GuiCore *gui_core = NULL;
	GuiWidget  *pre_win = NULL;
	void *surface = NULL;
	GUI_Rect rect = {0}, valid_rect = {0};

	gui_core = GUI_GetCurrent();

	ret = stack_count(gui_core->win_stack) - 2;
	pre_win = stack_get(gui_core->win_stack, ret);

	if(pre_win)
	{
		GUI_GetProperty(pre_win->name, "surface", &surface);
		GUI_GetProperty(pre_win->name, "surface_rect", (void *)(&rect));
	}
	if(surface)
	{
		valid_rect = rect;
		valid_rect.x = valid_rect.y = 0;

		gal_stretch_surface(surface,
				(GAL_Rect*)&valid_rect,
				screen,
				(GAL_Rect*)&rect);
	}

	return (0);
}

static int _draw_movable_window_surface(GuiWidget *self, GuiWindow *window)
{
	int ret = 0;
	unsigned int offset_x = 0, offset_y = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};
	GAL_Rect move_s_rect = {0}, move_d_rect = {0};
	void *surface = NULL;
	GuiCore *gui_core = NULL;

	if((NULL == self) || (NULL == window))
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	src_rect.x = self->rect.x;
	src_rect.y = self->rect.y;
	src_rect.w = self->rect.w;
	src_rect.h = self->rect.h;
	dst_rect = src_rect;
	src_rect.x = src_rect.y = 0;
	if(NULL == window->movable_surface)
	{
		window->movable_surface = gal_get_surface(&src_rect, gui_core->config.bpp);

		if(NULL == window->movable_surface)
		{
			return (1);
		}
		ret = gdi_commit();
		ret |= gal_copy_surface(screen, &dst_rect, window->movable_surface, &src_rect);
		window->move_origin_x = dst_rect.x;
		window->move_origin_y = dst_rect.y;
	}

	dst_rect.x = window->move_origin_x;
	dst_rect.y = window->move_origin_y;
	if(((window->move_origin_x > self->rect.x) && ((window->move_origin_x - self->rect.x) > self->rect.w)) ||
			((window->move_origin_y > self->rect.y) && ((window->move_origin_y - self->rect.y) > self->rect.h)) ||
			((window->move_origin_x < self->rect.x) && ((self->rect.x - window->move_origin_x) > self->rect.w)) ||
			((window->move_origin_y < self->rect.y) && ((self->rect.y - window->move_origin_y) > self->rect.h)))
	{
		ret |= gdi_commit();
		ret |= gal_copy_surface(window->movable_surface, &src_rect, screen, &dst_rect);
		ret |= gdi_commit();

		dst_rect.x = self->rect.x;
		dst_rect.y = self->rect.y;
		ret |= gal_copy_surface(screen, &dst_rect, window->movable_surface, &src_rect);
		window->move_origin_x = dst_rect.x;
		window->move_origin_y = dst_rect.y;
		return (ret);
	}

	surface = gal_get_surface(&src_rect, gui_core->config.bpp);
	if(NULL == surface)
	{
		return (1);
	}

	/*Decrease*/
	if(window->move_origin_x > self->rect.x)
	{
		offset_x = window->move_origin_x - self->rect.x;
		if(offset_x > self->rect.w)
		{
			offset_x = self->rect.w;
		}
		move_s_rect.x = self->rect.x;
		move_s_rect.y = self->rect.y;
		move_s_rect.w = offset_x;
		move_s_rect.h = self->rect.h;
		move_d_rect = move_s_rect;
		move_d_rect.x = move_d_rect.y = 0;
		ret |= gal_copy_surface(screen, &move_s_rect, surface, &move_d_rect);

		dst_rect.x = self->rect.x + self->rect.w;
		dst_rect.w = offset_x;
		src_rect.w = dst_rect.w;
		src_rect.x = self->rect.w - offset_x;
		gdi_begin();
		ret |= gal_stretch_surface(window->movable_surface, &src_rect, screen, &dst_rect);
		gdi_end();

		move_s_rect.x = 0;
		move_s_rect.y = 0;
		move_s_rect.w = self->rect.w - offset_x;
		move_s_rect.h = self->rect.h;
		move_d_rect = move_s_rect;
		move_d_rect.x = offset_x;
		ret |= gal_copy_surface(window->movable_surface, &move_s_rect, surface, &move_d_rect);
		move_d_rect.x = move_d_rect.y = 0;
		move_d_rect.w = self->rect.w;
		move_d_rect.h = self->rect.h;
		ret |= gal_copy_surface(surface, &move_d_rect, window->movable_surface, &move_d_rect);
	}
	/*Increase*/
	else if(window->move_origin_x < self->rect.x)
	{
		offset_x = self->rect.x - window->move_origin_x;
		if(offset_x > self->rect.w)
		{
			offset_x = self->rect.w;
		}
		move_s_rect.x = offset_x;
		move_s_rect.y = 0;
		move_s_rect.w = self->rect.w - offset_x;
		move_s_rect.h = self->rect.h;
		move_d_rect = move_s_rect;
		move_d_rect.x = 0;
		ret |= gal_copy_surface(window->movable_surface, &move_s_rect, surface, &move_d_rect);

		dst_rect.w = offset_x;
		src_rect.w = dst_rect.w;
		gdi_begin();
		ret |= gal_stretch_surface(window->movable_surface, &src_rect, screen, &dst_rect);
		gdi_end();

		move_s_rect.x = window->move_origin_x + self->rect.w;
		move_s_rect.y = self->rect.y;
		move_s_rect.w = offset_x;
		move_s_rect.h = self->rect.h;
		move_d_rect = move_s_rect;
		move_d_rect.x = self->rect.w - offset_x;
		move_d_rect.y = 0;
		ret |= gal_copy_surface(screen, &move_s_rect, surface, &move_d_rect);

		move_d_rect.x = move_d_rect.y = 0;
		move_d_rect.w = self->rect.w;
		move_d_rect.h = self->rect.h;
		ret |= gal_copy_surface(surface, &move_d_rect, window->movable_surface, &move_d_rect);
	}

	/*Decrease*/
	if(window->move_origin_y > self->rect.y)
	{
		offset_y = window->move_origin_y - self->rect.y;
		if(offset_y > self->rect.h)
		{
			offset_y = self->rect.h;
		}
		move_s_rect.x = self->rect.x;
		move_s_rect.y = self->rect.y;
		move_s_rect.w = self->rect.w;
		move_s_rect.h = offset_y;
		move_d_rect = move_s_rect;
		move_d_rect.x = move_d_rect.y = 0;
		ret |= gal_copy_surface(screen, &move_s_rect, surface, &move_d_rect);

		dst_rect.y = self->rect.y + self->rect.h;
		dst_rect.h = offset_y;
		src_rect.h = dst_rect.h;
		src_rect.y = self->rect.h - offset_y;

		gdi_begin();
		ret |= gal_stretch_surface(window->movable_surface, &src_rect, screen, &dst_rect);
		gdi_end();

		move_s_rect.x = 0;
		move_s_rect.y = 0;
		move_s_rect.w = self->rect.w;
		move_s_rect.h = self->rect.h - offset_y;
		move_d_rect = move_s_rect;
		move_d_rect.y = offset_y;
		ret |= gal_copy_surface(window->movable_surface, &move_s_rect, surface, &move_d_rect);

		move_d_rect.x = move_d_rect.y = 0;
		move_d_rect.w = self->rect.w;
		move_d_rect.h = self->rect.h;
		ret |= gal_copy_surface(surface, &move_d_rect, window->movable_surface, &move_d_rect);
	}
	/*Increase*/
	else if(window->move_origin_y < self->rect.y)
	{
		offset_y = self->rect.y - window->move_origin_y;
		if(offset_y > self->rect.h)
		{
			offset_y = self->rect.h;
		}

		move_s_rect.x = 0;
		move_s_rect.y = offset_y;
		move_s_rect.w = self->rect.w;
		move_s_rect.h = self->rect.h - offset_y;
		move_d_rect = move_s_rect;
		move_d_rect.y= 0;

		ret |= gal_copy_surface(window->movable_surface, &move_s_rect, surface, &move_d_rect);
        dst_rect.h = offset_y;
        src_rect.h = dst_rect.h;

		gdi_begin();
		ret |= gal_stretch_surface(window->movable_surface, &src_rect, screen, &dst_rect);
		gdi_end();

		move_s_rect.x = self->rect.x;
		move_s_rect.y = window->move_origin_y + self->rect.h;
		move_s_rect.w =  self->rect.w;
		move_s_rect.h = offset_y;
		move_d_rect = move_s_rect;
		move_d_rect.x = 0;
		move_d_rect.y = self->rect.h - offset_y;
		ret |= gal_copy_surface(screen, &move_s_rect, surface, &move_d_rect);

		move_d_rect.x = move_d_rect.y = 0;
		move_d_rect.w = self->rect.w;
		move_d_rect.h = self->rect.h;
		ret |= gal_copy_surface(surface, &move_d_rect, window->movable_surface, &move_d_rect);
	}
	//ret |= gdi_commit();

	dst_rect.x = self->rect.x;
	dst_rect.y = self->rect.y;
	dst_rect.w = self->rect.w;
	dst_rect.h = self->rect.h;
	//ret |= gal_copy_surface(screen, &dst_rect, window->movable_surface, &src_rect);
	window->move_origin_x = dst_rect.x;
	window->move_origin_y = dst_rect.y;
	gal_free_surface(surface);

	return (ret);
}

static int _draw_file_background(GuiWidget *self)
{
	GuiWindow *window = NULL;
	image_desc *desc = NULL;
	const char *value = NULL;

	value = widget_get_property(self, "back_ground_file");
	if(NULL == value)
	{
		return (1);
	}

	if(0 == strcasecmp("null", value))
	{
		return (1);
	}

	window = (GuiWindow *)(self->object);
	if((NULL == window->back_ground) ||
			((window->back_ground) &&
			 (0 != strcasecmp(window->back_ground, value))))
	{
		desc = gal_img_load(desc, value);
		if(NULL == desc)
		{
			return (1);
		}
		GxAvdev_SppLock();
		hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
		gal_background_desc(spp_screen,desc);
		gdi_commit();
		gal_enable_img(GUI_TRUE);
		hd_enable_video(GUI_FALSE);
		GUI_FREE(window->back_ground);
		window->back_ground = GUI_STRDUP(value);
		gal_img_cleanup(desc);
		GxAvdev_SppUnlock();
	}

	return (0);
}

static int window_draw_background(GuiWidget *self)
{
	GuiWindow *window = NULL;
	const char *value = NULL;
	int ret = 0;

	window = (GuiWindow *)(self->object);
	value = widget_get_property(self, "back_ground");

	if (value == NULL)
		return 1;

	if(0 == strcasecmp("x", value)) {
		return (0);
	}

	ret = _draw_file_background(self);
	if(0 == ret)
	{
		return (1);
	}

	GUI_FREE(window->back_ground);

	window->back_ground = GUI_STRDUP(value);

	window->back_desc = widget_img_load(value);
	if(window->back_desc != gal_get_background())
	{
		GxAvdev_SppLock();

		if (strcasecmp(window->back_ground, "null") == 0) {
			hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
			gal_background_desc(spp_screen, NULL);
			gal_enable_img(GUI_FALSE);
			hd_enable_video(GUI_TRUE);
		}
		else {
			hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
			gal_background_desc(spp_screen, window->back_desc);
			gdi_commit();
			gal_enable_img(GUI_TRUE);
			hd_enable_video(GUI_FALSE);
		}
		GxAvdev_SppUnlock();
	}
	if(window->back_desc)
	{
		if((window->back_desc->vq) &&
		   (window->back_desc->data != window->back_desc->vq->usr_p))
			gal_img_cleanup(window->back_desc);
		widget_img_clear(&(window->back_desc));
	}
	else
	{
		hd_img_clear();
	}

	return 0;
}

static int window_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiWindow *window = NULL;
	GuiWidget *child = NULL, *focus = NULL;
	GuiClut *pclut = NULL;
	int index = 0, color = 0, ret = 0, update_count = 0, draw_count = 0;
	GUI_Rect rect = {0};
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	window = (GuiWindow *)(self->object);
	if(NULL == window)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	if(GUI_FALSE == window->back_sync) {
		window_draw_background(self);
	}
	if(window->update_back) {
		window->update_back = GUI_FALSE;
		return (0);
	}

	if((FORMAT_POPUP == window->format) && (GUI_FALSE == window->popup_recorded))
	{
		_window_set_surface(self);
		window->popup_recorded = GUI_TRUE;
	}
	else if(FORMAT_MOVABLE == window->format)
	{
		if((window->move_origin_x != self->rect.x) ||
				(window->move_origin_y != self->rect.y))
		{
			//initial = GUI_TRUE;
			src_rect.x = window->move_origin_x;
			src_rect.y = window->move_origin_y;
			src_rect.w = self->rect.w;
			src_rect.h = self->rect.h;
			dst_rect = src_rect;
			src_rect.x = src_rect.y = 0;

			if(window->surface_move)
			{
				gal_copy_surface(screen, &dst_rect, window->back_up_surface, &src_rect);
				window->surface_move = GUI_TRUE;
			}
		}
		else
		{
			window->surface_move = GUI_FALSE;
			goto NORMAL;
		}
		ret = _draw_movable_window_surface(self, window);
		if(ret)
		{
			gxlogd("[GUI]Move error!\n");
		}

		if(NULL == window->back_up_surface)
		{
			src_rect.x = self->rect.x;
			src_rect.y = self->rect.y;
			src_rect.w = self->rect.w;
			src_rect.h = self->rect.h;
			dst_rect = src_rect;
			src_rect.x = src_rect.y = 0;
			window->back_up_surface = gal_get_surface(&src_rect, gui_core->config.bpp);
			if(NULL == window->back_up_surface)
			{
				window->update_all = GUI_FALSE;
				return (1);
			}
			//initial = GUI_TRUE;
		}
		else
		{
			if((window->surface) && (!window->update_all))
			{
				if(window->surface_rect)
				{
					rect = *(window->surface_rect);
					rect.x = rect.y = 0;
					gal_stretch_surface(window->surface,
							(GAL_Rect*)&rect,
							screen,
							(GAL_Rect*)window->surface_rect);
					//GUI_FREE(window->surface_rect);
				}
				gdi_commit();
				gal_free_surface(window->surface);
				window->surface = NULL;

				child = widget_get_firstchild(self);
				while(child)
				{
					//Patch by shencz
					if((0 == strcasecmp("progbar", child->classname)) ||
							(0 == strcasecmp("sliderbar", child->classname)))
					{
						GUI_SetProperty(child->name, "update", NULL);
					}
					if((WIDGET_STATE_FOCUSED & child->state) ||
							(child->need_update))
					{
						if(0 == strcasecmp("listview", child->classname))
						{
							GUI_GetProperty(child->name, "focus_widget", &focus);
							child->need_update = GUI_TRUE;
							widget_set_update(focus, GUI_TRUE);
							widget_draw(child);
						}
						else
						{
							widget_draw(child);
						}
					}

					child = widget_get_nextsibling(child);
				}

				draw_count = gui_core->widget_need_update_count;
				exec_widget_signal_data(self, "show", NULL);
				if(gui_core->widget_need_update_count != draw_count)
				{
					gdi_unlock();
					GUI_SetInterface("flush", NULL);
					gdi_lock();
				}
			}
			else
			{
				src_rect.x = self->rect.x;
				src_rect.y = self->rect.y;
				src_rect.w = self->rect.w;
				src_rect.h = self->rect.h;

				dst_rect = src_rect;
				src_rect.x = src_rect.y = 0;
				gal_copy_surface(window->back_up_surface, &src_rect, screen, &dst_rect);
			}

			window->update_all = GUI_FALSE;
			if(GUI_TRUE == window->back_sync) {
				gdi_commit();
				window_draw_background(self);
			}
			return (0);
		}
	}

NORMAL:
	if((window->surface) && (!window->update_all))
	{
		if(window->surface_rect)
		{
			rect = *(window->surface_rect);
			rect.x = rect.y = 0;
			gal_stretch_surface(window->surface,
					(GAL_Rect*)&rect,
					screen,
					(GAL_Rect*)window->surface_rect);
			//GUI_FREE(window->surface_rect);
		}
		gdi_commit();
		gal_free_surface(window->surface);
		window->surface = NULL;

		child = widget_get_firstchild(self);
		while(child)
		{
			//Patch by shencz
			if((0 == strcasecmp("progbar", child->classname)) ||
					(0 == strcasecmp("sliderbar", child->classname)))
			{
				GUI_SetProperty(child->name, "update", NULL);
			}
			if((WIDGET_STATE_FOCUSED & child->state) ||
					(child->need_update))
			{
				if(0 == strcasecmp("listview", child->classname))
				{
					GUI_GetProperty(child->name, "focus_widget", &focus);
					child->need_update = GUI_TRUE;
					widget_set_update(focus, GUI_TRUE);
					widget_draw(child);
					focus = NULL;
				}
				else
				{
					if(gui_core->config.focus_widget != child)
					{
						widget_draw(child);
					}
					else
					{
						focus = child;
					}
				}
			}

			child = widget_get_nextsibling(child);
		}
		if(focus)
		{
			widget_draw(focus);
		}

		draw_count = gui_core->widget_need_update_count;
		exec_widget_signal_data(self, "show", NULL);
		if(gui_core->widget_need_update_count != draw_count)
		{
			gdi_unlock();
			GUI_SetInterface("flush", NULL);
			gdi_lock();
		}
		window->update_all = GUI_FALSE;
		if(GUI_TRUE == window->back_sync) {
			gdi_commit();
			window_draw_background(self);
		}
		return (0);
	}


	if(gal_get_clut_index() != gui_core->config.logical_clut)
	{
		pclut = gui_core->config.firstclut;
		index = 0;
		while(index < gal_get_clut_index())
		{
			if(NULL == pclut)
			{
				break;
			}
			pclut = pclut->next;
			index++;
		}

		if(pclut)
		{
			gui_core->config.clut = pclut;
			if(gui_core->config.enable_double_buffer)
			{
				gdi_begin();
				ret = gui_set_clut(back_screen,
						pclut->palette,
						0,
						pclut->num);
				gdi_end();
				hd_enable_osd(GUI_TRUE);
			}

			gdi_begin();
			ret = gui_set_clut(screen,
					pclut->palette,
					0,
					pclut->num);
			gdi_end();
			hd_enable_osd(GUI_TRUE);
		}

		gal_set_clut_index(gui_core->config.logical_clut);
	}

	if(window->image)
	{
		gal_img_desc(screen,
				window->image,
				self->rect.x,
				self->rect.y,
				self->rect.w,
				self->rect.h);
	}
	else
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		if(color == gui_core->config.gui_trans)
		{
			_draw_popup_back();
		}
		else
		{
			color = gal_color2index(screen, color);
			gal_fillrect(screen, (GAL_Rect*)&self->rect, color);
		}
	}

	update_count = gui_core->widget_need_update_count;
	widget_children_set_update(self, GUI_TRUE);//listview box update all
	gui_core->widget_need_update_count = update_count;
	child = widget_get_firstchild(self);

	while(child)
	{
		//Patch by shencz
		if((0 == strcasecmp("progbar", child->classname)) ||
				(0 == strcasecmp("sliderbar", child->classname)))
		{
			GUI_SetProperty(child->name, "update", NULL);
		}
		if(gui_core->config.focus_widget != child)
		{
			widget_draw(child);
		}
		else
		{
			focus = child;
		}
		child = widget_get_nextsibling(child);
	}
	if(focus)
	{
		widget_draw(focus);
	}

	if(FORMAT_MOVABLE == window->format)
	{

		src_rect.x = self->rect.x;
		src_rect.y = self->rect.y;
		src_rect.w = self->rect.w;
		src_rect.h = self->rect.h;
		dst_rect = src_rect;
		src_rect.x = src_rect.y = 0;
		gdi_commit();
		gal_copy_surface(screen, &dst_rect, window->back_up_surface, &src_rect);
		//initial = GUI_FALSE;
	}

	draw_count = gui_core->widget_need_update_count;
	exec_widget_signal_data(self, "show", NULL);
	if(gui_core->widget_need_update_count != draw_count)
	{
		gdi_unlock();
		GUI_SetInterface("flush", NULL);
		gdi_lock();
	}

	if(GUI_TRUE == window->back_sync) {
		gdi_commit();
		window_draw_background(self);
	}
	window->update_all = GUI_FALSE;
	if(window->need_fade) {
		gui.prev_layer_mode = gui.layer_mode;
		gui.layer_mode = GUI_GRADIANT_LAYER;
		gui.config.window_fade_mode = window->fade_mode;
		if(window->fade_steps)
			gui.config.window_fade_steps = 64;
		window->need_fade = false;
		window->fade_mode = IMAGE_EFFECT_NONE;
	}
	return (0);
}



static int window_event(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWindow *window = NULL;
	GUI_Event *event = NULL;
	int ret = 0;

	WIDGET_CHECK_OBJECT(self, data, window, GuiWindow);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr callback*/
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		if(window->keypress_event)
		{
			ret = exec_widget_signal_data(self, "keypress", data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}

		}

		/*widget callback*/
		return (_add_key(self, event));

	case GUI_MOUSEBUTTONDOWN:
		if(window->clicked_event)
		{
			ret = exec_widget_signal_data(self, "clicked", data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}

		_add_mouse(self, event);
		break;

	case GUI_SERVICE_MSG:
		if(window->service_event)
		{
			return exec_widget_signal_data(self, "service", data);
		}
		else
		{
			return EVENT_TRANSFER_KEEPON;
		}
		break;

	default:
		break;
	}

	return (EVENT_TRANSFER_STOP);
}

static void _widget_revert_x(GuiWidget *widget)
{
	    GuiCore *gui_core = NULL;

        gui_core = GUI_GetCurrent();
        GuiWidget *child = NULL;
        if(widget->mirrored) {
            widget->rect.x = gui_core->config.width - widget->rect.x - widget->rect.w;
        }

        child = widget_get_firstchild(widget);
        while(child) {
            _widget_revert_x(child);
            child = widget_get_nextsibling(child);
        }
}

static int window_set_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = NULL;
	GuiWidget *child = NULL;
	GuiWindow *window = NULL;
	char *pIdName = NULL;
	GUI_Rect rect = {0};
	int16_t x = 0, y = 0;
	int16_t offset = 0;
	int ret = 0;
	char *string = NULL;


	if(self == NULL || self->object == NULL || name == NULL)
		return (1);

	if(WIDGET_ISNOT_WINDOW(self) ) {
		GUI_Printf("[GUI][window] set property, self is not a window\n");
		return 1;
	}

	gui_core = GUI_GetCurrent();
	window = (GuiWindow *)(self->object);
	window->update_back = GUI_FALSE;

	if(0 == strcasecmp("save_active_widget", name)) {
		if(NULL == data) return 1;
		window->active_widget = (GuiWidget*)data;
	}
	else if(0 == strcasecmp("clear_window", name)) {
		if(!gui_core->stop_update) {
			gal_fillrect(screen, (GAL_Rect *)&self->rect, gal_color2index(screen, gui_core->config.osd_trans));
		}
	}
	else if(0 == strcasecmp("move_window_x", name)) {
		if(NULL == data) return 1;
		x = *(int16_t*)data;
		_widget_revert_x(self);
		offset = x - self->rect.x;
		_widget_set_x(self, offset);
		if(FORMAT_MOVABLE == window->format)
		{
			window->surface_move = GUI_TRUE;
		}
	}
	else if(0 == strcasecmp("move_window_y", name)) {
		if(NULL == data) return 1;
		y = *(int16_t*)data;
		offset = y - self->rect.y;
		_widget_set_y(self, offset);
		if(FORMAT_MOVABLE == window->format)
		{
			window->surface_move = GUI_TRUE;
		}
	}
	else if(0 == strcasecmp("update", name)) {
		widget_set_update(self, GUI_TRUE);
		window->update_all = GUI_TRUE;
	}
	else if(0 == strcasecmp("update_background", name)) {
		if(!gui_core->stop_update) {
			hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
			gal_background_desc(spp_screen, NULL);
			widget_set_update(self, GUI_TRUE);
		}
	}
	else if(0 == strcasecmp("back_ground", name)) {
		if(NULL == data) {
			window->back_ground = NULL;
			widget_set_property(self, name, "x");
			widget_set_update(self, GUI_TRUE);
			return (0);
		}
		ret = widget_set_property(self, name, (const char*)data);
		pIdName = (char *)data;
		GUI_FREE(window->back_ground);
		if(0 == strcasecmp("null", pIdName)) {
			window->back_ground = GUI_STRDUP(pIdName);
			gal_background_desc(spp_screen, NULL);
			return (0);
		}
		window->back_ground = GUI_STRDUP((char *)pIdName);
		if(!self->need_update) {
			window->update_back = GUI_TRUE;
		}

		self->need_update = GUI_TRUE;
#ifndef NO_OS
		gui_core->widget_need_update_count++;
#else
		widget_need_update_count++;
#endif /*NO_OS*/
	}
	else if(0 == strcasecmp("disable_background", name)) {
		hd_enable_video(GUI_TRUE);
		gal_enable_img(GUI_FALSE);
	}
	else if(0 == strcasecmp("got_focus", name)) {
		child = widget_get_firstchild(self);
		while(child) {
			widget_got_focus(child);
			child = widget_get_nextsibling(child);
		}
		exec_widget_signal_data(self, "got_focus", NULL);
	}
	else if(0 == strcasecmp("lost_focus", name)) {
		exec_widget_signal_data(self, "lost_focus", NULL);
	}
	else if(0 == strcasecmp("surface", name)) {
		if(window->surface) {
			gal_free_surface(window->surface);
			window->surface = NULL;
		}
		window->surface = *(void **)data;
		window->popup_recorded = GUI_FALSE;
	}
	else if(0 == strcasecmp("surface_rect", name)) {
		if(NULL == data)
			return (1);

		rect = *(GUI_Rect *)(data);
		GUI_FREE(window->surface_rect);
		window->surface_rect = (GUI_Rect *)GUI_MALLOC(sizeof(GUI_Rect));
		if(NULL == window->surface_rect)
			return (1);

		*(window->surface_rect) = rect;
		window->popup_recorded = GUI_FALSE;
	}
	else if(0 == strcasecmp("format", name)) {
		if(NULL == data)
			return (1);

		string = (char *)data;
		if(0 == strcasecmp("dialog", string))
			window->format = FORMAT_DIALOG;
		else if(0 == strcasecmp("popup", string))
		{
			window->format = FORMAT_POPUP;
			window->popup_recorded = GUI_FALSE;
		}
		widget_set_property(self, "format", string);
	}
	else if(0 == strcasecmp("free_record", name)) {
		if(window->surface)
		{
			gal_free_surface(window->surface);
			window->surface = NULL;
		}
		if(window->movable_surface)
		{
			gal_free_surface(window->movable_surface);
			window->movable_surface = NULL;
		}
		if(window->back_up_surface)
		{
			gal_free_surface(window->back_up_surface);
			window->back_up_surface = NULL;
		}
		if(FORMAT_MOVABLE == window->format)
		{
			if(self->rect.x)
				window->move_origin_x = self->rect.x - 1;
			else
				window->move_origin_x = self->rect.x + 1;

			if(self->rect.y)
				window->move_origin_y = self->rect.y - 1;
			else
				window->move_origin_y = self->rect.y + 1;
		}
	}
	else if(0 == strcasecmp("back_sync", name)) {
		if(0 == strcasecmp("true", data)) {
			window->back_sync = GUI_TRUE;
		} else {
			window->back_sync = GUI_FALSE;
		}
	} else if(0 == strcasecmp("clear_surface", name)) {
		child = widget_get_firstchild(self);
		while(child) {
			if(child) {
				GUI_SetProperty(child->name, "clear_surface", NULL);
			}
			child = widget_get_nextsibling(child);
		}
	} else if(0 == strcasecmp("need_fade", name)) {
		if((data) && (0 == strcasecmp("true", data)))
			_set_window_need_fade(self, true);
		else
			_set_window_need_fade(self, false);
	}
	else {
		ret = widget_set_property(self, name, (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (ret);
	}

	return (0);
}

static int window_get_property(GuiWidget *self, char *name, void *data)
{
	GuiWidget * widget = NULL;
	GuiWindow *window = NULL;


	if(self == NULL || self->object == NULL || name == NULL || data == NULL)
		return 1;

	if(WIDGET_ISNOT_WINDOW(self) ) {
		GUI_Printf("[GUI][window] get property, self is not a window\n");
		return 1;
	}

	window = (GuiWindow *)(self->object);

	if(0 == strcasecmp(name, "get_active_widget"))
		*(GuiWidget**)data = window->active_widget;
	else if( 0 == strcasecmp(name, "get_first_focussable_widget")) {
		widget = widget_get_firstchild(self);
		while(widget) {
			if(widget_get_focussable(widget)) {
				*(GuiWidget**)data = widget;
				return 0;
			}
			widget = widget_get_nextsibling(widget);
		}
		*(GuiWidget**)data = NULL;

	}
	else if(0 == strcasecmp("surface", name))
		*(void**)data = window->surface;
	else if(0 == strcasecmp("surface_rect", name))
		*(void**)data = window->surface_rect;
	else {
		GUI_Error("[window] get property, %s not support\n", name);
		return (1);
	}

	return 0;
}

static int window_prepare_image(GuiWidget *self)
{
	GuiWindow *window = NULL;
	const char *value = NULL;

	if (self == NULL || self->object == NULL)
		return 1;

	window = (GuiWindow *)(self->object);

	if (window->image == NULL) {
		value = widget_get_property(self, "image");
		if (value)
			window->image = widget_img_load(value);
	}

	return (0);
}

static int window_update(GuiWidget *self, GUI_Rect *rect)
{
	GuiWindow *window = NULL;
	GUI_Rect src_rect = {0}, dst_rect = {0};
	int index = 0, color = 0;

	if (self == NULL || self->object == NULL)
	{
		return 1;
	}

	window = (GuiWindow *)(self->object);

	dst_rect = *rect;
	if((dst_rect.x + dst_rect.w) > (self->rect.x + self->rect.w))
	{
		dst_rect.w = (self->rect.x + self->rect.w) - dst_rect.x;
	}

	if((dst_rect.y + dst_rect.h) > (self->rect.y + self->rect.h))
	{
		dst_rect.h = (self->rect.y + self->rect.h) - dst_rect.y;
	}

	src_rect = dst_rect;
	if(window->image)
	{
		gal_img_stretch(screen, window->image, (GAL_Rect *)&src_rect, (GAL_Rect *)&dst_rect);
	}
	else
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)(&dst_rect), color);
	}
	return (0);
}

static int window_clear_image(GuiWidget *self)
{
	GuiWindow *window = NULL;

	if (self == NULL || self->object == NULL)
		return 1;

	window = (GuiWindow *)(self->object);

	if(window->image)
	{
		widget_img_clear(&(window->image));
	}

	if(window->back_desc)
	{
		widget_img_clear(&(window->back_desc));
	}
	return (0);
}

static int window_unactive(GuiWidget *self)
{
	GuiWindow *window = NULL;
	GuiWidget *child = NULL;

	if(NULL == self) {
		return (1);
	}
	window = (GuiWindow *)(self->object);
	if(NULL == window) {
		return (1);
	}

	if(window->surface) {
		gal_free_surface(window->surface);
		window->surface = NULL;;
	}

	if(window->movable_surface) {
		gal_free_surface(window->movable_surface);
		window->movable_surface = NULL;
	}

	if(window->back_up_surface) {
		gal_free_surface(window->back_up_surface);
		window->back_up_surface = NULL;
	}

	child = widget_get_firstchild(self);
	while(child) {
		wm_unactive_widget(child);
		child = widget_get_nextsibling(child);
	}

	return (0);
}

GuiWidgetOps window_ops = {
	.owner   = "window",
	.create  = window_create,
	.release = window_release,
	.prepare_image = window_prepare_image,
	.draw    = window_draw,
	.update	 = window_update,
	.clear_image   = window_clear_image,
	.unactive = window_unactive,
	.event   = window_event,
	.set     = window_set_property,
	.get     = window_get_property
};

