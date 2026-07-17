#include "listview.h"
#include "header.h"
#include "listitem.h"
#include "color.h"
#include "gui.h"

static GuiWidget *_get_header_widget(GuiWidget *widget)
{
	GuiWidget *child = NULL;

	child = widget_get_firstchild(widget);
	while(child) {
		if((child) &&
		   (child->name) &&
		   (0 == strcasecmp("header", child->classname))) {
			return (child);
		}
		child = widget_get_firstchild(widget);
	}

	return (NULL);
}

static int _listview_timer(void *userdata)
{
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL, *child = NULL;
	GuiListView *listview = NULL;
	GuiWidget *self_win = NULL, *focus_widget = NULL;

	self = (GuiWidget *)userdata;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	reset_timer(listview->start_timer);

	if(-1 == listview->cur_sel)
	{
		return (1);
	}

	child = listview->focus_item;

	GUI_SetProperty(child->name, "enable_roll", &(listview->enable_roll));
	GUI_SetProperty(child->name, "roll_time", &(listview->time));

	self_win = widget_get_window(self);
	focus_widget = gui_get_widget(gui_core->config.root_widget, GUI_GetFocusWidget());
	focus_widget = widget_get_window(focus_widget);
	if((focus_widget) &&
	   (0 == strcasecmp(self_win->name, focus_widget->name)) &&
	   (0 == strcasecmp(self->name, GUI_GetFocusWidget())))
	{
		widget_set_update(child, GUI_TRUE);
		if((listview->update_state != ROLL_UP_UPDATE) &&
		   (listview->update_state != ROLL_DOWN_UPDATE) &&
		   (listview->update_state != ALL_UPDATE))
			listview->update_state = ITEM_UPDATE;
		self->need_update = GUI_TRUE;
	} else {
		listview->update_state = ITEM_UPDATE;
		widget_set_update(self, GUI_FALSE);
	}

	return (0);
}

static int _listview_get_format(GuiWidget *self)
{
	const char *value = NULL;
	char*p = NULL;
	char format_str[30] = {0};
	GuiListView *listview = NULL;
	int ret = 0, i = 0;

	if(NULL == self)
	{
		return (-1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (-1);
	}

	if(self->property)
	{
		value = (const char *)hash_get(self->property, "format");
	}

	if(NULL == value)
	{
		return (0);
	}

	listview->fix_select = -1;
	p = (char *)value;
	while(*p != '\0')
	{
		i = 0;
		while(*p != '|' && *p != '\0')
		{
			format_str[i] = *p;
			i++;
			p++;
		}

		format_str[i] = '\0';

		if(0 == strcmp("headershow", format_str))
		{
			ret |= LISTVIEW_SHOW_HEADER;
		}
		else if(0 == strcmp("headerhide", format_str))
		{
			ret |= LISTVIEW_HIDE_HEADER;
		}
		else if(0 == strcmp("scrollshow", format_str))
		{
			ret |= LISTVIEW_SHOW_SCROLL;
		}
		else if(0 == strcmp("scrollhide", format_str))
		{
			ret |= LISTVIEW_HIDE_SCROLL;
		}
		else if(0 == strcasecmp("enable_roll", format_str))
		{
			listview->enable_roll = GUI_TRUE;
		}
		else if(0 == strcasecmp("page", format_str))
		{
			ret |= LISTVIEW_PAGE;
		}
		else if(0 == strcasecmp("fix_round", format_str))
		{
			ret |= LISTVIEW_FIX_ROUND;
		}
		else
		{
			;
		}

		if('|' == *p)
			p++;
	}

	return (ret);
}


static void *_listview_get_item(GuiWidget *self, int index)
{
	GuiWidget *child = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp("listitem", child->classname))
		{
			if(i == index)
			{
				break;
			}
			else
			{
				i++;
			}
		}

		child = widget_get_nextsibling(child);
	}

	if(child)
	{
		return (child->object);
	}
	else
	{
		return (NULL);
	}
}

GuiWidget * _set_list_item(GuiWidget *listitem)
{
	GuiWidget *listview = NULL;
	GuiWidget *cur_widget = NULL;
	GuiListView *listview_obj = NULL;

	if(NULL == listitem)
	{
		return (NULL);
	}

	if(strcasecmp("listitem", listitem->classname))
	{
		gxlogd("Your listitem class is incorrect!\n");
		return (NULL);
	}

	listview = widget_get_parent(listitem);
	if(NULL == listview)
	{
		return (NULL);
	}

	listview_obj = (GuiListView *)(listview->object);

	if(NULL == listview_obj)
	{
		return (NULL);
	}

	cur_widget= listview_obj->focus_item;

	cur_widget->state &= ~WIDGET_STATE_FOCUSED;
	widget_set_update(cur_widget, GUI_TRUE);

	cur_widget = listitem;
	listview_obj->focus_item = listitem;
	cur_widget->state |= WIDGET_STATE_FOCUSED;
	widget_set_update(cur_widget, GUI_TRUE);

	listview->need_update = GUI_TRUE;

	return (cur_widget);
}

static GuiWidget * _get_scrollbar(GuiWidget *self)
{
	GuiWidget *sibling = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	if(1 == strcasecmp("listview", self->classname))
	{
		return (NULL);
	}

	sibling  = widget_get_firstchild(self);

	while(sibling)
	{
		if(0 == strcasecmp("scrollbar", sibling->classname))
		{
			break;
		}
		sibling = widget_get_nextsibling(sibling);
	}

	return (sibling);
}

int _listview_time_out(void *userdata)
{
	GuiWidget *self = NULL;
	GuiListView *listview = NULL;

	self = (GuiWidget *)userdata;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	listview->timer = NULL;

	if(listview->timeout_event)
	{
		exec_widget_event_data(self, listview->timeout_event, userdata);
	}

	return (0);
}

static int _reset_rolling(const char *widget_name)
{
	int value = 0;

	GUI_SetProperty(widget_name,
			"stop_rolling",
			NULL);
	value = GUI_TRUE;
	GUI_SetProperty(widget_name,
			"enable_roll",
			&value);
	value = 0;
	GUI_SetProperty(widget_name,
			"roll_time",
			&value);
	return (0);
}

static int _add_key(GuiWidget* self, GuiListView* listview, int key)
{
	GuiListItem* listitem = NULL;
	ListViewItem *list_item = NULL;
	GuiWidget *listscroll = NULL, *header = NULL;
	int ret = 0, value = 0;

	listscroll = _get_scrollbar(self);
	header = _get_header_widget(self);

	switch(key)
	{
	case GUIK_UP:
		if(0 == listview->total_num)
		{
			return (0);
		}

		if(listview->cur_sel)
		{
			if(listview->timer)
			{
				reset_timer(listview->timer);
			}
			else
			{
				listview->timer = create_timer(_listview_time_out,
						listview->time_out,
						self,
						TIMER_ONCE);
			}

			if(LISTVIEW_PAGE & listview->format)
			{
				value = listview->cur_sel-1;
				GUI_SetProperty(self->name, "select", &value);
				break;
			}

			listview->cur_sel--;
			if(listview->focus_sel)
			{
				if(LISTVIEW_FIX_ROUND & listview->format) {
					widget_set_update(self, GUI_TRUE);
					if((header) && (ALL_UPDATE != listview->update_state)) {
						widget_set_update(header, GUI_FALSE);
					}
				} else {
					listview->focus_sel--;
				}
				listitem = (GuiListItem*)_listview_get_item(listview->base, listview->focus_sel);
				if(listitem)
				{
					_set_list_item(listitem->base);
				}
				if(listscroll)
				{
					widget_set_update(listscroll, GUI_TRUE);
				}
				listview->update_state = ITEM_UPDATE;
			}
			else
			{
				// TODO:
				list_item = &(listview->item);
				list_item->min = listview->cur_sel - 1;
				//widget_set_update(self, GUI_TRUE);
				listitem = (GuiListItem*)_listview_get_item(listview->base, listview->focus_sel);
				if(listitem) {
					widget_set_update(listitem->base, GUI_TRUE);
					widget_set_update(widget_get_nextsibling(listitem->base), GUI_TRUE);
				}
				if(listscroll)
				{
					widget_set_update(listscroll, GUI_TRUE);
				}
				self->need_update = GUI_TRUE;
				if((ARRANGE_RIGHT_TO_LEFT == listview->arrange) ||
				   ((listview->disable_accelerate))) {
					listview->update_state = ALL_UPDATE;
					widget_set_update(self, GUI_TRUE);
					if((header) && (ALL_UPDATE != listview->update_state)) {
						widget_set_update(header, GUI_FALSE);
					}
				} else {
					listview->update_state = ROLL_DOWN_UPDATE;
				}
			}
		}
		else
		{
			// TODO:
			value = listview->total_num - 1;
			GUI_SetProperty(self->name, "select", &value);
			widget_set_update(self, GUI_TRUE);
			if((listview->enable_roll) && (listview->total_num))
			{
				_reset_rolling(listview->focus_item->name);
			}
			widget_set_update(self, GUI_TRUE);
			if((header) && (ALL_UPDATE != listview->update_state)) {
				widget_set_update(header, GUI_FALSE);
			}
			listview->update_state = ALL_UPDATE;
			break;
		}

		if(listview->start_timer)
		{
			remove_timer(listview->start_timer);
			listview->start_timer = NULL;
			value = 0;
			_reset_rolling(listview->focus_item->name);
		}

		if(NULL == listview->start_timer && listview->enable_roll)
		{
			listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
		}

		if(listview->change_event && listview->total_num)
		{
			exec_widget_event_data(self, listview->change_event, NULL);
		}

		break;

	case GUIK_DOWN:
		if(0 == listview->total_num)
		{
			return (0);
		}

		if(listview->cur_sel + 1 < listview->total_num)
		{
			if(listview->timer)
			{
				reset_timer(listview->timer);
			}
			else
			{
				listview->timer = create_timer(_listview_time_out,
						listview->time_out,
						self,
						TIMER_ONCE);
			}

			if(LISTVIEW_PAGE & listview->format)
			{
				value = listview->cur_sel+1;
				GUI_SetProperty(self->name, "select", &value);
				break;
			}
			listview->cur_sel++;
			if(listview->focus_sel < (listview->visible_row - 1))
			{
				if(LISTVIEW_FIX_ROUND & listview->format) {
					widget_set_update(self, GUI_TRUE);
					if((header) && (ALL_UPDATE != listview->update_state)) {
						widget_set_update(header, GUI_FALSE);
					}
				} else {
					listview->focus_sel++;
				}
				listitem = (GuiListItem*)_listview_get_item(listview->base, listview->focus_sel);
				if(listitem)
				{
					_set_list_item(listitem->base);
				}
				if(listscroll)
				{
					widget_set_update(listscroll, GUI_TRUE);
				}
				listview->update_state = ITEM_UPDATE;
			}
			else
			{
				// TODO:
				list_item = &(listview->item);
				list_item->min = listview->cur_sel + 1;
				//widget_set_update(self, GUI_TRUE);
				listitem = (GuiListItem*)_listview_get_item(listview->base, listview->focus_sel);
				if(listitem) {
					widget_set_update(listitem->base, GUI_TRUE);
					widget_set_update(widget_get_priorsibling(listitem->base), GUI_TRUE);
				}
				if(listscroll)
				{
					widget_set_update(listscroll, GUI_TRUE);
				}
				self->need_update = GUI_TRUE;
				if((ARRANGE_RIGHT_TO_LEFT == listview->arrange) ||
				   ((listview->disable_accelerate))) {
					listview->update_state = ALL_UPDATE;
					widget_set_update(self, GUI_TRUE);
					if((header) && (ALL_UPDATE != listview->update_state)) {
						widget_set_update(header, GUI_FALSE);
					}
				} else {
					listview->update_state = ROLL_UP_UPDATE;
				}
			}
		}
		else
		{
			// TODO:
			value = 0;
			GUI_SetProperty(self->name, "select", &value);
			if((listview->enable_roll) && (listview->total_num))
			{
				_reset_rolling(listview->focus_item->name);
			}
			widget_set_update(self, GUI_TRUE);
			if((header) && (ALL_UPDATE != listview->update_state)) {
				widget_set_update(header, GUI_FALSE);
			}
			listview->update_state = ALL_UPDATE;
			break;
		}

		if(listview->start_timer)
		{
			remove_timer(listview->start_timer);
			listview->start_timer = NULL;
			value = 0;
			_reset_rolling(listview->focus_item->name);
		}

		if(NULL == listview->start_timer && listview->enable_roll)
		{
			listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
		}

		if(listview->change_event && listview->total_num)
		{
			exec_widget_event_data(self, listview->change_event, NULL);
		}
		break;
	case GUIK_PAGE_UP:
		if(0 == listview->total_num)
		{
			return (0);
		}
		if(LISTVIEW_FIX_ROUND & listview->format) {
			return (0);
		}

		if((listview->cur_sel - listview->visible_row) < 0)
		{
			value = 0;
		}
		else
		{
			value = listview->cur_sel - listview->visible_row;
		}

		if(listview->start_timer)
		{
			remove_timer(listview->start_timer);
			listview->start_timer = NULL;
			value = 0;
			_reset_rolling(listview->focus_item->name);
		}

		if(NULL == listview->start_timer && listview->enable_roll)
		{
			listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
		}
		GUI_SetProperty(self->name, "select", &value);
		listview->update_state = ALL_UPDATE;
		widget_set_update(self, GUI_TRUE);
		if((header) && (ALL_UPDATE != listview->update_state)) {
			widget_set_update(header, GUI_FALSE);
		}
		break;
	case GUIK_PAGE_DOWN:
		if(0 == listview->total_num)
		{
			return (0);
		}

		if(LISTVIEW_FIX_ROUND & listview->format) {
			return (0);
		}

		if((listview->cur_sel + listview->visible_row) >=  listview->total_num)
		{
			value = listview->total_num - 1;
		}
		else
		{
			value = listview->cur_sel + listview->visible_row;
		}

		if(listview->start_timer)
		{
			remove_timer(listview->start_timer);
			listview->start_timer = NULL;
			value = 0;
			_reset_rolling(listview->focus_item->name);
		}

		if(NULL == listview->start_timer && listview->enable_roll)
		{
			listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
		}
		GUI_SetProperty(self->name, "select", &value);
		listview->update_state = ALL_UPDATE;
		widget_set_update(self, GUI_TRUE);
		if((header) && (ALL_UPDATE != listview->update_state)) {
			widget_set_update(header, GUI_FALSE);
		}
		break;
	case GUIK_RETURN:
		if(listview->focus_item)
		{
			ret = listview->focus_item->ops->event(listview->focus_item, NULL);
			if(listview->clicked_event)
			{
				exec_widget_event_data(self, listview->clicked_event, NULL);
			}
		}
		return (ret);
	default:
		return EVENT_TRANSFER_KEEPON;
	}

	return EVENT_TRANSFER_STOP;
}

static void _set_right_to_left(GuiWidget *self)
{
	GuiWidget *child = NULL;

	child = widget_get_firstchild(self);
	while(child) {
		widget_set_property(child, "arrange", "right_to_left");
		child = widget_get_nextsibling(child);
	}
}

static int _config_listview_parametre(GuiWidget *self)
{
	GuiListView *listview = NULL;
	const char *value = NULL;
	char *p_str = NULL;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == listview->format))
	{
		listview->format = _listview_get_format(self);
	}

	value = widget_get_property(self, "interval");
	if((NULL != value) || (0 == listview->interval))
	{
		listview->interval = widget_get_int(self, "interval", 0);
	}

	value = widget_get_property(self, "roll_time");
	if((NULL != value) || (0 == listview->time))
	{
		listview->time = widget_get_int(self, "roll_time", 0);
	}

	value = widget_get_property(self, "fix_select");
	if((LISTVIEW_FIX_ROUND & listview->format) && ((NULL != value) || (0 == listview->fix_select)))
	{
		listview->fix_select = widget_get_int(self, "fix_select", 0);
	}

	value = widget_get_property(self, "item_active_color");
	if((NULL != value) || (0 == listview->active_color))
	{
		p_str = (char *)value;
		if(p_str)
		{
			listview->active_color = gui_get_color(p_str, gal_get_gui_transparent());
		}
	}

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == listview->font))
	{
		p_str = (char *)value;

		listview->font = (char *)widget_get_property(self, "font");
	}

	value = widget_get_property(self, "i18n");
	if((NULL != value) || (0 == listview->i18n))
	{
		p_str = (char *)value;
		if(p_str)
		{
			if(0 == strcasecmp("true", p_str))
			{
				listview->i18n = GUI_TRUE;
			}
			else
			{
				listview->i18n = GUI_FALSE;
			}
		}
	}

	value = widget_get_property(self, "arrange");
	if((NULL != value) && (0 == strcasecmp(value, "right_to_left"))) {
		listview->arrange = ARRANGE_RIGHT_TO_LEFT;
		_set_right_to_left(self);
	}

	listview->disable_accelerate = GUI_TRUE;
	value = widget_get_property(self, "accelerate");
	if((NULL != value) && (0 == strcasecmp(value, "enable"))) {
		listview->disable_accelerate = GUI_FALSE;
	}

	return (0);
}

static int _set_roll_step(GuiWidget *child, const char *value)
{
	if(NULL == value)
	{
		return (0);
	}

	widget_set_property(child, "step", value);
	return (0);
}

static void _set_right_to_left_arrange(GuiWidget *self)
{
	GuiWidget *child = NULL;
	const char *value = NULL;
	int scrollbar_width = 0;

	value = widget_get_property(self, "arrange");
	if(NULL == value) {
		return;
	}

	if(0 == strcasecmp("left_to_right", value)) {
		return;
	}

	child = widget_get_firstchild(self);
	while(child) {
		if(0 == strcasecmp("scrollbar", child->classname)) {
			scrollbar_width = child->rect.w;
			child->rect.x = self->rect.x;
			break;
		}
		child = widget_get_nextsibling(child);
	}

	child = widget_get_firstchild(self);
	while(child) {
		if((0 == strcasecmp("header", child->classname)) ||
		   (0 == strcasecmp("listitem", child->classname))) {
			child->rect.x += scrollbar_width;
		}
		child = widget_get_nextsibling(child);
	}
}

static void *create_listview(GuiWidget *self)
{
	GuiListView *listview = NULL, *listview_style = NULL;
	GuiListItem *listitem = NULL;
	GUI_Rect item_rect = {0};
	const char *value = NULL;
	GuiWidget *child = NULL, *style = NULL;
	GuiWidget *parent = NULL;
	int offset = 0, postpone_time = 0;
	GuiCore *gui_core = GUI_GetCurrent();

	if(NULL == self)
	{
		return (NULL);
	}

	listview = (GuiListView *)GUI_MALLOC(sizeof(GuiListView));
	if(NULL == listview)
	{
		return (NULL);
	}
	memset(listview, 0, sizeof(GuiListView));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	self->disable_mirror = parent->disable_mirror;

	listview->base = self;
	self->object = (void *)(listview);

	if(self->property)
	{
		value = (const char *)hash_get(self->property, "item_rect");
	}

	if(value)
	{
		sscanf(value, "[%d,%d,%d,%d]",
				&item_rect.x,
				&item_rect.y,
				&item_rect.w,
				&item_rect.h);
	}

	if((item_rect.x < 0) ||
	   (item_rect.y < 0) ||
	   (item_rect.w < 0) ||
	   (item_rect.h < 0)){
		GUI_FREE(self->object);
		return (NULL);
	}

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		listview_style = (GuiListView *)GUI_MALLOC(sizeof(GuiListView));
		if(NULL == listview_style)
		{
			GUI_FREE(listview);
			return (NULL);
		}

		memset(listview_style, 0, sizeof(GuiListView));
		style->object = (void *)listview_style;

		_config_listview_parametre(style);

		memcpy(listview, listview_style, sizeof(GuiListView));
		listview->base = self;

		GUI_FREE(style->object);
	}

	_config_listview_parametre(self);

	listview->time_out = widget_get_int(self, "time_out", 0);

	self->state = WIDGET_STATE_FOCUSSABLE;

	if(self->signal)
	{
		listview->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		listview->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		listview->draw_event = (GuiWidgetSignal *)hash_get(self->signal, "draw");
		listview->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
		listview->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		listview->gettotal_event = (GuiWidgetSignal *)hash_get(self->signal, "get_total");
		listview->getdata_event = (GuiWidgetSignal *)hash_get(self->signal, "get_data");
		listview->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
		listview->timeout_event = (GuiWidgetSignal *)hash_get(self->signal, "timeout");
	}
	child = widget_get_firstchild(self);
	while(child)
	{
		if((0 != strcasecmp("header", child->classname)) &&
				(0 != strcasecmp("listitem", child->classname)) &&
				(0 != strcasecmp("scrollbar", child->classname)))
		{
			GUI_FREE(self->object);
			return (NULL);
		}
		widget_create(child);
		value = widget_get_property(self, "step");
		_set_roll_step(child, value);

		child->rect.x += self->rect.x;
		child->rect.y += self->rect.y;

		if(0 == strcasecmp("header", child->classname))
		{
			offset += child->rect.h;
		}
		else if(0 == strcasecmp("listitem", child->classname))
		{
			if(0 == listview->visible_row)
			{
				child->state |= WIDGET_STATE_FOCUSED;
				listview->focus_item = child;
			}

			if(listview->font)
			{
				widget_set_property(child, "font", listview->font);
			}

			child->rect = item_rect;
			child->rect.x += self->rect.x;
			child->rect.y += (self->rect.y + offset);

			if(listview->i18n)
			{
				GUI_SetProperty(child->name, "i18n", "true");
			}

			postpone_time = widget_get_int(self, "postpone_time", 0);
			GUI_SetProperty(child->name, "postpone_time", &postpone_time);
			value = (const char *)widget_get_property(self, "item_back_color");
			if(value)
			{
				getColorFromString(value, child->back_color);
			}

			if((child->back_color[0] == gal_get_gui_transparent()) ||
			   (child->back_color[1] == gal_get_gui_transparent()) ||
			   (child->back_color[2] == gal_get_gui_transparent())) {
				listview->disable_accelerate = GUI_TRUE;
			}

			value = (const char *)widget_get_property(self, "item_fore_color");
			if(value)
			{
				getColorFromString(value, child->fore_color);
			}

			value = (const char *)widget_get_property(self, "item_mirror");
			if(value && (0 == strcasecmp("disable", value))) {
				GUI_SetProperty(child->name, "mirror", "disable");
			}

			listitem = (GuiListItem *)(child->object);
			if(listitem)
			{
				if(-1 == listitem->alignment)
				{
					listitem->alignment = widget_get_alignment(self);
				}

				if(NULL == listitem->font)
				{
					listitem->font = (char *)widget_get_property(self, "font");
				}
			}

			offset += (child->rect.h + listview->interval);

			listview->visible_row++;
		}

		child = child->next_sibling;
	}

	_set_right_to_left_arrange(self);

	listview->item.min = 0;
	listview->item.num = listview->visible_row;
	if((LISTVIEW_FIX_ROUND & listview->format) && (-1 == listview->fix_select)) {
		listview->fix_select = listview->visible_row - 1;
	}

	listview->active_index = -1;

	listview->cur_sel = -1;
	listview->focus_sel = -1;

	listview->update_state = ALL_UPDATE;

	if(listview->real_total) {
		listview->total_num = listview->real_total;
	} else {
		listview->total_num = -1;
	}

	if(listview->create_event)
	{
		exec_widget_event_data(self, listview->create_event, NULL);
	}

	if((NULL == listview->start_timer) && (listview->enable_roll))
	{
		listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
	}

	if(gui_core->config.mirror) {
		listview->disable_accelerate = GUI_TRUE;
	}

	return (listview);
}

static int listview_release(GuiWidget *self)
{
	GuiListView *listview = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	if(listview->start_timer)
	{
		remove_timer(listview->start_timer);
	}

	if(listview->surface) {
		hd_free_surface(listview->surface);
		listview->surface = NULL;
	}

	child = self->first_child;
	while(child)
	{
		widget_release(child);

		child = child->next_sibling;
	}

	if(listview->destroy_event)
	{
		exec_widget_event_data(self, listview->destroy_event, NULL);
	}

	GUI_FREE(self->object);


	return (0);
}

static int _listview_surface(GuiWidget *self, GuiListView *listview)
{
	int ret = 0, header_height = 0, item_width = 0, item_height = 0, item_y = 0, index = 0, color = 0;
	GuiWidget *child = NULL;
	GuiHeader *header = NULL;
	GAL_Rect surface_rect = {0}, content_rect = {0}, src_rect = {0};
	GuiSurface *surface = NULL;
	GuiCore *gui_core = NULL;

	if((LISTVIEW_PAGE & listview->format) ||
	   (listview->disable_accelerate) ||
	   (ARRANGE_RIGHT_TO_LEFT == listview->arrange)) {
		if(listview->surface) {
			hd_free_surface(listview->surface);
			listview->surface = NULL;
		}
		return (0);
	}

	if((ITEM_UPDATE == listview->update_state) && (NULL != listview->surface)) {
		return (0);
	}

	gui_core = GUI_GetCurrent();

	child = widget_get_firstchild(self);
	while(child) {
		if(0 == strcasecmp("listitem", child->classname)) {
			item_y = child->rect.y;
			item_width = child->rect.w;
			break;
		} else if(0 == strcasecmp("header", child->classname)) {
			header = (GuiHeader *)child->object;
			header_height = child->rect.h;
		}
		child = widget_get_nextsibling(child);
	}

	item_height = child->rect.h + listview->interval;

	surface_rect.x = surface_rect.y = 0;
	surface_rect.w = item_width;
	surface_rect.h = (listview->visible_row + 2) * item_height;
	if(NULL == listview->surface) {
		listview->surface = hd_get_surface(&surface_rect, 32, NULL, GUI_FALSE);
		listview->update_state = ALL_UPDATE;
	}
	if(NULL == listview->surface) {
		return (-1);
	} else {
		if((ROLL_UP_UPDATE == listview->update_state) ||
		   (ROLL_DOWN_UPDATE == listview->update_state)) {
			content_rect.x = content_rect.y = 0;
			content_rect.w = item_width;
			content_rect.h = self->rect.h;
			content_rect.h = item_height * (listview->visible_row - 1);
			surface = hd_get_surface(&content_rect, 32, NULL, GUI_FALSE);
			if(NULL == surface) {
				return (1);
			}
			src_rect.x = self->rect.x;
			src_rect.y = item_y;
			src_rect.w = item_width;
			src_rect.h = item_height * (listview->visible_row - 1);
			if(ROLL_DOWN_UPDATE == listview->update_state) {
				src_rect.y += item_height;
				src_rect.h -= item_height;
				content_rect.y += item_height;
				content_rect.h -= item_height;
			}
			hd_copy_surface(screen, &src_rect, surface, &content_rect);
		}
		index = widget_get_state_index(self);
		color = hd_color2index(listview->surface, self->back_color[index]);
		gdi_begin();
		if(ALL_UPDATE == listview->update_state) {
			if((32 != gui_core->config.bpp) && (0xFF000000 & color) == 0) {
				color |= 0xFF000000;
			}
			hd_fillrect(listview->surface, (GAL_Rect *)&surface_rect, color);
		} else if(ROLL_UP_UPDATE == listview->update_state) {
			src_rect.x = src_rect.y = 0;
			src_rect.w = item_width;
			src_rect.h = content_rect.h;
		} else {
			src_rect.x = 0;
			src_rect.y = 3 * item_height;
			src_rect.w = item_width;
			src_rect.h = content_rect.h;
		}
		hd_copy_surface(surface, &content_rect, listview->surface, &src_rect);
		gdi_end();

		if(surface) {
			hd_free_surface(surface);
		}
	}

	return (ret);
}

static void _copy_list_surface(GuiWidget *self, GuiListView *listview)
{
	int ret = 0, header_height = 0, item_width = 0, item_height = 0;
	GuiWidget *child = NULL;
	GuiHeader *header = NULL;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == listview->surface) {
		return;
	}

	child = widget_get_firstchild(self);
	while(child) {
		if(0 == strcasecmp("listitem", child->classname)) {
			break;
		} else if(0 == strcasecmp("header", child->classname)) {
			header = (GuiHeader *)child->object;
			header_height = child->rect.h;
		}
		child = widget_get_nextsibling(child);
	}
	item_height = child->rect.h + listview->interval;
	item_width = child->rect.w;

	src_rect.y = src_rect.y + item_height;
	src_rect.x = 0;
	src_rect.w = item_width;
	src_rect.h = listview->visible_row * item_height;

	dst_rect.x = self->rect.x;
	dst_rect.y = self->rect.y + header_height;
	dst_rect.w = item_width;
	dst_rect.h = src_rect.h;
	gdi_commit();
	hd_stretch_surface(listview->surface, &src_rect, screen, &dst_rect);
	listview->update_state = ALL_UPDATE;
}

void _relative_coordinate(GuiWidget *parent, GUI_Rect *rect)
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

static int listview_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiListView *listview = NULL;
	GuiHeader *header = NULL;
	GuiListItem *listitem = NULL;
	GuiWidget *child = NULL;
	const char *value = NULL;
	ListItemPara *item_para = NULL, *next_item = NULL, *pItem = NULL;
	GUI_Rect grid_rect = {0};
	int Index = 0, color = 0;
	int iSel = 0, i = 0;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	value = widget_get_property(self, "accelerate");
	if((NULL != value) && (0 == strcasecmp(value, "enable")) && (!gui_core->config.mirror)) {
		listview->disable_accelerate = GUI_FALSE;
	}

	_listview_surface(self, listview);

	value = widget_get_property(self, "arrange");
	if((NULL != value) && (0 == strcasecmp(value, "right_to_left"))) {
		listview->arrange = ARRANGE_RIGHT_TO_LEFT;
		_set_right_to_left(self);
	}

	if((listview->gettotal_event) && (-1 == listview->total_num))
	{
		listview->total_num = exec_widget_event_data(self, listview->gettotal_event, NULL);
	}

	if(listview->real_total)
	{
		listview->total_num = listview->real_total;
	}

	if(listview->draw_event)
	{
		exec_widget_event_data(self, listview->draw_event, NULL);
	}

	child = widget_get_firstchild(self);
	if(NULL == child)
	{
		return (1);
	}

	/*Add for rolling exit from other window*/
	if(NULL == listview->start_timer && listview->enable_roll)
	{
		listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
	}

	if(0 == strcasecmp("header", child->classname))
	{
		header = (GuiHeader *)(child->object);
		if(NULL != header)
		{
			ListItemPara *old_item;

			if(header->num_col == 0)
				return (1);

			old_item = NULL;
			if(listview->getdata_event)
			{
				for(i = (header->num_col - 1); i >= 0 ; i--)
				{
					item_para = (ListItemPara *)GUI_MALLOC(sizeof(ListItemPara));
					if(NULL == item_para)
					{
						return (1);
					}
					memset(item_para, 0, sizeof(ListItemPara));
					item_para->next = old_item;
					old_item = item_para;
				}
			}
			if(HEADER_SHOW == header->visible)
			{
				child->ops->draw(child);
			}
		}
	}

	if(0 == listview->total_num)
	{
		if(listview->gettotal_event)
		{
			listview->total_num = exec_widget_event_data(self, listview->gettotal_event, NULL);
		}
		else
		{
			listview->total_num = listview->visible_row;
		}

		if(listview->real_total)
		{
			listview->total_num = listview->real_total;
		}
	}
	if((listview->total_num) && (listview->cur_sel == -1)) {
		if(LISTVIEW_FIX_ROUND & listview->format) {
			GuiListItem *listitem = NULL;

			listview->focus_sel = listview->fix_select;
			if(0 <= listview->focus_sel) {
				listitem = (GuiListItem*)_listview_get_item(self, listview->focus_sel);
				if(listitem) {
					_set_list_item(listitem->base);
				}
			}
		} else {
			listview->focus_sel = 0;
		}
		listview->cur_sel = 0;
	}

	child = widget_get_nextsibling(child);
	iSel = listview->cur_sel - listview->focus_sel;
	if((LISTVIEW_FIX_ROUND & listview->format) &&
	   (0 > iSel)) {
		iSel = listview->total_num + iSel;
	}

	while(child)
	{
		if(0 == strcasecmp("listitem", child->classname) )
		{
			int state_index = 0;
			if(child->need_update)
			{
				listitem = (GuiListItem *)(child->object);
				if(NULL == listitem)
				{
					_copy_list_surface(self, listview);
					widget_set_update(self, GUI_FALSE);
					return (1);
				}

				for(state_index = 0; state_index < 3; state_index++)
				{
					listitem->image[state_index] = listview->item_img[state_index];
					listitem->image_g[state_index][0] = listview->item_img_g[state_index][0];
					listitem->image_g[state_index][1] = listview->item_img_g[state_index][1];
					listitem->image_g[state_index][2] = listview->item_img_g[state_index][2];
				}

				if(0 == listview->total_num)
				{
					child->state &= ~WIDGET_STATE_FOCUSED;
				}

				if((listview->total_num) &&
				   (iSel < listview->total_num) &&
				   (listview->getdata_event))
				{
					pItem = item_para;
					item_para->sel = iSel;
					while(pItem)
					{
						pItem->sel = iSel;
						pItem->font = pItem->back_color = pItem->fore_color = NULL;
						pItem = pItem->next;
					}
					exec_widget_event_data(self, listview->getdata_event, item_para);
				}
				else
				{
					pItem = item_para;
					while(pItem)
					{
						pItem->string = NULL;
						pItem->image = NULL;
						pItem = pItem->next;
					}
				}

				if(item_para)
				{
					child->ops->set(child, "content", item_para);
				}

				if((listview->active_index < listview->total_num) &&
				   (listview->active_index == iSel))
				{
					listitem->active_color = listview->active_color;
					listitem->active_img = listview->active_img;
					listitem->active_img_g[0] = listview->active_img_g[0];
					listitem->active_img_g[1] = listview->active_img_g[1];
					listitem->active_img_g[2] = listview->active_img_g[2];
				}
				else
				{
					listitem->active_color = 0;
					listitem->active_img = NULL;
					listitem->active_img_g[0] = listitem->active_img_g[1] = listitem->active_img_g[2] = NULL;
				}

				widget_draw(child);

				if(listview->interval)
				{
					grid_rect.x = child->rect.x;
					grid_rect.y = child->rect.y + child->rect.h;
					grid_rect.w = child->rect.w;
					grid_rect.h = listview->interval;

					Index = widget_get_state_index(self);
					color = self->back_color[Index];
					color = gal_color2index(screen, color);
					gal_fillrect(screen, (GAL_Rect *)&grid_rect, color);
					if(NULL != listview->surface) {
						_relative_coordinate(self, &grid_rect);
						gdi_begin();
						color = self->back_color[Index];
						color = gal_color2index(listview->surface, color);
						gal_fillrect(listview->surface, (GAL_Rect *)&grid_rect, color);
						gdi_end();
					}
				}

				widget_set_update(child, GUI_FALSE);
			}

			iSel++;
			if((LISTVIEW_FIX_ROUND & listview->format) && (iSel >= listview->total_num)) {
				iSel = 0;
			}
		}
		else if(0 == strcasecmp("scrollbar", child->classname))
		{
			if(child->need_update)
			{
				widget_draw(child);
				widget_set_update(child, GUI_FALSE);
			}
		}
		child = widget_get_nextsibling(child);
	}

	for(i = (header->num_col - 1); i >= 0 ; i--)
	{
		if(item_para)
		{
			next_item = item_para->next;
			GUI_FREE(item_para);
			item_para = next_item;
		}
	}
	_copy_list_surface(self, listview);
	widget_set_update(self, GUI_FALSE);

	listview->update_state = NO_UPDATE;

	return (0);
}

static int listview_event(GuiWidget *self, void *data)
{
	GuiListView *listview = NULL;
	GUI_Event *event = NULL;
	int key = 0;
	int ret = 0, value = 0;

	WIDGET_CHECK_OBJECT(self, data, listview, GuiListView);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
        if(widget_is_keypress_revert(self)) {
            revert_gui_key(event);
        }
		if(listview->keypress_event)
		{
			ret = exec_widget_event_data(self, listview->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
                if(widget_is_keypress_revert(self)) {
                    revert_gui_key(event);
                }
                return ret;
			}
		}

		/*usr signal callback*/
		/*enable roll OK key -> stop rolling
		 * other keys && total number > 1 -> stop rolling*/
		if(((listview->start_timer) &&
					(GUIK_RETURN == event->key.sym)) ||
				(((listview->total_num > 1) &&
				  (listview->start_timer)) &&
				 ((GUIK_UP == event->key.sym) ||
				  (GUIK_DOWN == event->key.sym) ||
				  (GUIK_PAGE_UP == event->key.sym) ||
				  (GUIK_PAGE_DOWN == event->key.sym))))
		{
			remove_timer(listview->start_timer);
			listview->start_timer = NULL;
			value = 0;
			GUI_SetProperty(listview->focus_item->name,
					"stop_rolling",
					NULL);
			GUI_SetProperty(listview->focus_item->name,
					"enable_roll",
					&value);
			GUI_SetProperty(listview->focus_item->name,
					"roll_time",
					&value);
		}

		/*object private callback*/
		key = event->key.sym;
		ret = _add_key(self, listview, key);
		if((listview->start_timer) &&
				((GUIK_UP == event->key.sym) ||
				 (GUIK_DOWN == event->key.sym) ||
				 (GUIK_RETURN == event->key.sym) ||
				 (GUIK_PAGE_UP == event->key.sym) ||
				 (GUIK_PAGE_DOWN == event->key.sym)))
		{
			value = listview->time;
			GUI_SetProperty(listview->focus_item->name,
					"roll_time",
					&value);
		}
        if(widget_is_keypress_revert(self)) {
            revert_gui_key(event);
        }
		return (ret);

	case GUI_MOUSEBUTTONDOWN:
		break;

	default:
		return EVENT_TRANSFER_KEEPON;
	}


	return (EVENT_TRANSFER_STOP);
}

static int listview_set_property(GuiWidget *self, char *name, void *data)
{
	GuiListView *listview = NULL;
	GuiListItem* listitem = NULL;
	GuiWidget *child = NULL, *header = NULL;
	int sel = 0, i = 0, sel_in = 0, old_active = 0, old_head = 0;
	char value[100] = {0};


	if(NULL == self || NULL == name)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	header = _get_header_widget(self);

	if(0 == strcasecmp("update_all", name))
	{
		listview->total_num = -1;
		listview->total_num = exec_widget_event_data(self, listview->gettotal_event, NULL);

		if(listview->real_total)
		{
			listview->total_num = listview->real_total;
		}
		listview->active_index = -1;
		if(listview->total_num)
		{
			if(listview->cur_sel == -1) {
				listview->cur_sel = 0;
				listview->focus_sel = 0;
			}
			if((listview->cur_sel >= 0) &&
					(listview->cur_sel < listview->total_num))
			{
				i = listview->cur_sel;
			}
			else
			{
				i = 0;
			}
			GUI_SetProperty(self->name, "select", &i);
		}
		else
		{
			listview->cur_sel = -1;
			listview->focus_sel = -1;
		}
		if(0 <= listview->focus_sel)
		{
			listitem = (GuiListItem*)_listview_get_item(listview->base, listview->focus_sel);
			if(listitem)
			{
				_set_list_item(listitem->base);
			}
		}

		widget_set_update(self, GUI_TRUE);

		if(NULL == listview->start_timer && listview->enable_roll)
		{
			listview->start_timer = create_timer(_listview_timer, 1000, self, TIMER_REPEAT);
		}

		listview->update_state = ALL_UPDATE;
	}
	else if(0 == strcasecmp("update_row", name))
	{
		int total = 0;

		sel = *((int *)(data));
		i = listview->cur_sel - listview->focus_sel;
		total = i + listview->visible_row;

		if((sel < i) || (sel > total))
		{
			return (0);
		}

		child = widget_get_firstchild(self);
		while(child)
		{
			if(0 == strcasecmp("listitem", child->classname))
			{
				if(i == sel)
				{
					widget_set_update(child, GUI_TRUE);
				}
				i++;
			}
			child = widget_get_nextsibling(child);
		}

		if((listview->update_state != ROLL_UP_UPDATE) && (listview->update_state != ROLL_DOWN_UPDATE))
			listview->update_state = ITEM_UPDATE;
		self->need_update = GUI_TRUE;

	}
	else if(0 == strcasecmp("select", name))
	{
		if(listview->timer)
		{
			reset_timer(listview->timer);
		}
		else
		{
			listview->timer = create_timer(_listview_time_out,
					listview->time_out,
					self,
					TIMER_ONCE);
		}
		sel = *((int *)(data));

		if(0 == listview->total_num)
		{
			listview->cur_sel = -1;
			return (0);
		} else if((sel >= listview->total_num) && (listview->total_num > 0)) {
			sel = listview->total_num - 1;
		}

		old_head = listview->cur_sel - listview->focus_sel;
		if((listview->cur_sel >= 0) && (listview->cur_sel == sel))
		{
			if(LISTVIEW_FIX_ROUND & listview->format) {
				listview->focus_sel = listview->fix_select;
			}
			return (0);
		}

		if(sel > (listview->cur_sel + (listview->visible_row - listview->focus_sel) - 1) ||
				sel < (listview->cur_sel - listview->focus_sel + 1)) {
			widget_set_update(self, GUI_TRUE);
			if((header) && (ALL_UPDATE != listview->update_state)) {
				widget_set_update(header, GUI_FALSE);
			}
			listview->update_state = ALL_UPDATE;
		} else {
			if((listview->update_state != ROLL_UP_UPDATE) &&
			   (listview->update_state != ROLL_DOWN_UPDATE) &&
			   (listview->update_state != ALL_UPDATE))
				listview->update_state = ITEM_UPDATE;
		}
		listview->cur_sel = sel;
		sprintf(value, "%d", sel);
		widget_set_property(self, name, value);
		if(listview->cur_sel < listview->visible_row)
		{
			if(LISTVIEW_FIX_ROUND & listview->format) {
				sel_in = listview->focus_sel = listview->fix_select;
			} else {
				listview->focus_sel = listview->cur_sel;
				sel_in = listview->cur_sel;
			}
		}
		else
		{
			if(LISTVIEW_FIX_ROUND & listview->format) {
				listview->focus_sel = listview->fix_select;
			} else {
				if(listview->visible_row > 1)
				{
					listview->focus_sel = listview->cur_sel % listview->visible_row;
				}
				else
				{
					listview->focus_sel = 0;
				}

				if(((listview->total_num - (listview->cur_sel - listview->focus_sel)) < listview->visible_row) &&
						(LISTVIEW_PAGE & listview->format) &&
						((listview->total_num - listview->cur_sel) >= listview->visible_row))
					listview->focus_sel = listview->visible_row - 1;
			}

			sel_in = listview->focus_sel;
		}

		if((listview->cur_sel - listview->focus_sel) != old_head) {
			widget_set_update(self, GUI_TRUE);
			if((header) && (ALL_UPDATE != listview->update_state)) {
				widget_set_update(header, GUI_FALSE);
			}
			listview->update_state = ALL_UPDATE;
		}

		i = 0;
		child = widget_get_firstchild(self);
		while(child)
		{
			if(i == sel_in &&
					(0 == strcasecmp("listitem", child->classname)))
			{
				break;
			}
			if(0 == strcasecmp("listitem", child->classname))
			{
				i++;
			}
			else if(0 == strcasecmp("scrollbar", child->classname))
			{
				widget_set_update(child, GUI_TRUE);
			}
			child = widget_get_nextsibling(child);
		}

		if(child)
		{
			_set_list_item(child);
		}
		else
		{
			_set_list_item(listview->focus_item);
		}

		self->need_update = GUI_TRUE;

		if(listview->change_event && listview->total_num)
		{
			exec_widget_event_data(self, listview->change_event, NULL);
		}
	}
	else if(0 == strcasecmp("active", name))
	{
		if((listview->gettotal_event) && (-1 == listview->total_num))
		{
			listview->total_num = exec_widget_event_data(self, listview->gettotal_event, NULL);
		}

		if(NULL == data)
		{
			listview->active_index = listview->focus_sel;
		}
		else
		{
			old_active = listview->active_index;
			sel = *((int *)data);
			if(sel != listview->active_index)
			{
				widget_set_update(self, GUI_TRUE);
				if((header) && (ALL_UPDATE != listview->update_state)) {
					widget_set_update(header, GUI_FALSE);
				}
			}

			if(sel < listview->total_num) {
				listview->active_index = sel;
			}
		}
		if((listview->active_index <= (listview->visible_row -
						listview->focus_sel +
						listview->cur_sel)) &&
				(listview->active_index >= (listview->cur_sel - listview->focus_sel)))
		{
			self->need_update = GUI_TRUE;
			child = widget_get_firstchild(self);
			i = 0;
			old_active = old_active - (listview->cur_sel - listview->focus_sel);
			sel = listview->active_index - (listview->cur_sel - listview->focus_sel);

			while(child)
			{
				if(0 == strcasecmp("listitem", child->classname))
				{
					if(i == old_active)
					{
						widget_set_update(child, GUI_TRUE);
					}
					else if(i == sel)
					{
						widget_set_update(child, GUI_TRUE);
					}
					i++;
				}
				child = widget_get_nextsibling(child);
			}
		}
		if((listview->update_state != ROLL_UP_UPDATE) &&
		   (listview->update_state != ROLL_DOWN_UPDATE) &&
		   (listview->update_state != ALL_UPDATE))
			listview->update_state = ITEM_UPDATE;
	}
	else if(0 == strcasecmp("total_number", name))
	{
		listview->real_total = *((int *)data);
		widget_set_update(self, GUI_TRUE);
		if((header) && (ALL_UPDATE != listview->update_state)) {
			widget_set_update(header, GUI_FALSE);
		}
		listview->update_state = ALL_UPDATE;
	}
	else if(0 == strcasecmp("step", name))
	{
		widget_set_property(self, name, (const char *)data);
		_set_roll_step(child, (char *)data);
	}
	else if(0 == strcasecmp("stop_rolling", name))
	{
		GUI_SetProperty(listview->focus_item->name,
						"stop_rolling",
						NULL);
		if(listview->start_timer)
		{
			timer_stop(listview->start_timer);
		}
	}
	else if(0 == strcasecmp("enable_roll", name))
	{
		if(!listview->enable_roll) {
			return (0);
		}
		i = GUI_TRUE;
		GUI_SetProperty(listview->focus_item->name,
					    "enable_roll",
						&i);
		if(listview->start_timer)
		{
			reset_timer(listview->start_timer);
		}
	}
	else if(0 == strcasecmp("i18n", name))
	{
		char *str = (char *)data;
		if(0 == strcasecmp("true", str))
		{
			listview->i18n = GUI_TRUE;
		}
		else
		{
			listview->i18n = GUI_FALSE;
		}

		child = widget_get_firstchild(self);
		while(child)
		{
			if(0 == strcasecmp("listitem", child->classname))
			{
				if(listview->i18n)
				{
					GUI_SetProperty(child->name, "i18n", "true");
				}
				else
				{
					GUI_SetProperty(child->name, "i18n", "false");
				}
			}
			child = widget_get_nextsibling(child);
		}
	}
	else
	{
		widget_set_property(self, name, (const char *)data);
		widget_set_update(self, GUI_TRUE);
		if((header) && (ALL_UPDATE != listview->update_state)) {
			widget_set_update(header, GUI_FALSE);
		}
		return (0);
	}

	return (0);
}

static int listview_get_property(GuiWidget *self, char *name, void *data)
{
	GuiListView *listview = NULL;
	GuiWidget *child = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	if(0 == strcasecmp("select", name))
	{
		if(data)
		{
			*((int *)data) = listview->cur_sel;
		}
	}
	else if(0 == strcasecmp("get_head", name))
	{
		child = widget_get_firstchild(self);
		while(child)
		{
			if(0 == strcasecmp("header", child->classname))
			{
				break;
			}
			child = widget_get_nextsibling(self);
		}

		if(child)
		{
			*(GuiWidget**)data = child;
		}
	}
	else if(0 == strcasecmp("active", name))
	{
		*((int *)data) = listview->active_index;
	}
	else if(0 == strcasecmp("cur_sel", name))
	{
		*((int *)data) = listview->focus_sel;
	}
	else if(0 == strcasecmp("total_num", name))
	{
		*((int *)data) = listview->total_num;
	}
	else if(0 == strcasecmp("visible_row", name))
	{
		*((int *)data) = listview->visible_row;
	}
	else if(0 == strcasecmp("focus_widget", name))
	{
		*((GuiWidget **)data) = listview->focus_item;
	}
	else if(0 == strcasecmp("surface", name)) {
		*((GuiSurface **)data) = listview->surface;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int listview_prepare_image(GuiWidget *self)
{
	GuiListView *listview = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if((NULL != value) || (NULL == listview->item_img[0]) || (NULL == listview->item_img_g[0][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(listview->item_img_g[0][0]),
								  &(listview->item_img_g[0][1]),
								  &(listview->item_img_g[0][2]));
		}
		else
		{
			listview->item_img[0] = widget_img_load( value);
			listview->item_img_g[0][0] = listview->item_img_g[0][1] = listview->item_img_g[0][2] = NULL;
		}
	}
	value = widget_get_property(self, "focus_img");
	if((NULL != value) || (NULL == listview->item_img[1]) || (NULL == listview->item_img_g[1][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(listview->item_img_g[1][0]),
								  &(listview->item_img_g[1][1]),
								  &(listview->item_img_g[1][2]));
		}
		else
		{
			listview->item_img[1] = widget_img_load( value);
			listview->item_img_g[1][0] = listview->item_img_g[1][1] = listview->item_img_g[1][2] = NULL;
		}
	}
	value = widget_get_property(self, "disable_img");
	if((NULL != value) || (NULL == listview->item_img[2]) || (NULL == listview->item_img_g[2][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(listview->item_img_g[2][0]),
								  &(listview->item_img_g[2][1]),
								  &(listview->item_img_g[2][2]));
		}
		else
		{
			listview->item_img[2] = widget_img_load( value);
			listview->item_img_g[2][0] = listview->item_img_g[2][1] = listview->item_img_g[2][2] = NULL;
		}
	}

	value = widget_get_property(self, "item_active_image");
	if((NULL != value) || (NULL == listview->active_img) || (NULL == listview->active_img_g[1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(listview->active_img_g[0]),
								  &(listview->active_img_g[1]),
								  &(listview->active_img_g[2]));
		}
		else
		{
			listview->active_img = widget_img_load(value);
			listview->active_img_g[0] = listview->active_img_g[1] = listview->active_img_g[2] = NULL;
		}
	}

	return (0);
}

static int listview_clear_image(GuiWidget *self)
{
	GuiListView *listview = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	for(i = 0; i < 3; i++) {
		if(listview->item_img[i]) {
			widget_img_clear(&(listview->item_img[i]));
		}

		if((listview->item_img_g[i][0]) ||
		   (listview->item_img_g[i][1]) ||
		   (listview->item_img_g[i][2])) {
			widget_img_group_clear(&(listview->item_img_g[i][0]),
					       &(listview->item_img_g[i][1]),
					       &(listview->item_img_g[i][2]));
		}
	}

	if(listview->active_img)
	{
		widget_img_clear(&(listview->active_img));
	}

	if(listview->active_img_g[1])
	{
		widget_img_group_clear(&(listview->active_img_g[0]),
				       &(listview->active_img_g[1]),
				       &(listview->active_img_g[2]));
	}

	return (0);
}

static int listview_unactive(GuiWidget *self)
{
	GuiWidget *child = NULL;
	GuiListView *listview = NULL;

	if(NULL == self) {
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	if(listview->surface) {
		gal_free_surface(listview->surface);
		listview->surface = NULL;
	}

	child = widget_get_firstchild(self);
	while(child) {
		wm_unactive_widget(child);
		child = widget_get_nextsibling(child);
	}

	return (0);
}

static int listview_gotfocus(GuiWidget *self)
{

	GuiListView *listview = NULL;
	if(NULL == self) {
		return (1);
	}

	listview = (GuiListView *)(self->object);
	if(NULL == listview)
	{
		return (1);
	}

	if(listview->start_timer && listview->enable_roll) {
		reset_timer(listview->start_timer);
	}

	return (0);
}

GuiWidgetOps listview_ops =
{
	.owner = "listview",
	.create = create_listview,
	.release = listview_release,
	.prepare_image = listview_prepare_image,
	.draw = listview_draw,
	.clear_image   = listview_clear_image,
	.unactive = listview_unactive,
	.gotfocus = listview_gotfocus,
	.event = listview_event,
	.set = listview_set_property,
	.get = listview_get_property
};



