#include "all_widget.h"

static int _table_set_focus(GuiWidget *widget, int flag)
{
	if(NULL == widget)
	{
		return (1);
	}

	if(flag)
	{
		widget->state |= WIDGET_STATE_FOCUSED;
	}
	else
	{
		widget->state &= ~WIDGET_STATE_FOCUSED;
	}

	return (0);
}

static int _config_table_parametre(GuiWidget *self)
{
	GuiTable *table = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (1);
	}

	value = widget_get_property(self, "row_num");
	if((NULL != value) || (0 == table->row_num))
	{
		table->row_num = widget_get_int(self, "row_num", 0);
	}

	value = widget_get_property(self, "column_num");
	if((NULL != value) || (0 == table->column_num))
	{
		table->column_num = widget_get_int(self, "column_num", 0);
	}

	value = widget_get_property(self, "interval");
	if((NULL != value) || (0 == table->interval))
	{
		table->interval = widget_get_int(self, "interval", 0);
	}

	return (0);
}

static int _get_visible_rows(GuiWidget *self)
{
	int count = 0, height = 0, i = 0;
	GUI_Rect rect = {0};
	GuiTable *table = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (0);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (0);
	}

	rect = self->rect;

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 == strcasecmp("button", child->classname))
		{
			if(0 == (i % table->column_num))
			{
				height += (child->rect.h + table->interval);
			}

			if(height > rect.h)
			{
				break;
			}
			i++;
		}

		child = widget_get_nextsibling(child);
	}

	count = i / table->column_num;

	if(i % table->column_num)
	{
		count++;
	}

	return (count);
}

static void* create_table(GuiWidget *self)
{
	GuiTable *table = NULL, *table_style = NULL;
	GuiWidget *child = NULL, *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	table = (GuiTable *)GUI_MALLOC(sizeof(GuiTable));
	if(NULL == table)
	{
		return (NULL);
	}

	memset(table, 0, sizeof(GuiTable));

	self->object = (void *)table;
	table->base = self;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		table_style = (GuiTable *)GUI_MALLOC(sizeof(GuiTable));
		if(NULL == table_style)
		{
			GUI_FREE(table);
			return (NULL);
		}

		memset(table_style, 0, sizeof(GuiTable));

		style->object = (void *)table_style;

		_config_table_parametre(style);

		memcpy(table, table_style, sizeof(GuiTable));

		GUI_FREE(style->object);
	}

	_config_table_parametre(self);

	child = widget_get_firstchild(self);

	while(child)
	{
		if(0 == strcasecmp("button", child->classname))
		{
			if(0 == table->total_num)
			{
				table->first_child = table->focused_child = child;
			}
			table->total_num++;
		}
		else if(0 == strcasecmp("scrollbar", child->classname))
		{
			table->scrollbar = child;
		}

		widget_create(child);
		child = widget_get_nextsibling(child);
	}

	_table_set_focus(table->focused_child, GUI_TRUE);
	self->state = WIDGET_STATE_FOCUSSABLE;
	_table_set_focus(self, GUI_TRUE);

	table->updata_all = 1;

	table->visible_row = _get_visible_rows(self);

	if(self->signal)
	{
		table->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		table->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		table->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		table->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	}

	if(table->create_event)
	{
		exec_widget_event_data(self, table->create_event, NULL);
	}

	return (table);
}

static int table_release(GuiWidget *self)
{
	GuiWidget *child = NULL;
	GuiTable *table = NULL;

	if(NULL == self)
	{
		return (1);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (1);
	}

	if(table->destroy_event)
	{
		exec_widget_event_data(self, table->destroy_event, NULL);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	GUI_FREE(self->object);

	return (0);
}

static int _adjust_coordinate(GuiWidget *self,
		unsigned int row,
		unsigned int col,
		unsigned int interval)
{
	if(NULL == self)
	{
		return (1);
	}

	self->rect.x = self->rect.x + col * (self->rect.w + interval);
	self->rect.y = self->rect.y + row * (self->rect.h + interval);

	return (0);
}

static int table_draw(GuiWidget *self)
{
	GuiTable *table = NULL;
	GuiWidget *child = NULL;
	unsigned int row = 0, col = 0;
	int color = 0;
	GUI_Rect rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (1);
	}

	if(table->updata_all)
	{
		rect = self->rect;
		color = self->back_color[0];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect*)(&rect), color);

		table->updata_all = 0;
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if((0 == strcasecmp("scrollbar", child->classname)) &&
				(child->need_update))
		{
			rect = child->rect;
			child->rect.x += self->rect.x;
			child->rect.y += self->rect.y;
			widget_draw(child);
			child->rect = rect;
			break;
		}
		child = widget_get_nextsibling(child);
	}

	/*Adjust the coordinate*/
	child = table->first_child;
	col = 0;
	row = 0;
	while(child)
	{
		if(0 == strcasecmp("button", child->classname))
		{
			child->rect.x = self->rect.x;
			child->rect.y = self->rect.y;

			_adjust_coordinate(child, row, col, table->interval);

			if(child->need_update)
			{
				widget_draw(child);
			}

			if(col == (table->column_num - 1))
			{
				col = 0;
				row++;
			}
			else
			{
				col++;
			}

			if(row > (table->visible_row - 1))
			{
				break;
			}

			if(row == table->row_num)
			{
				break;
			}
		}
		else
		{
			if(child->need_update)
			{
				widget_draw(child);
			}
		}

		child = widget_get_nextsibling(child);
	}

	return (0);
}

static int _check_top_line(GuiWidget *self, GuiWidget *focus)
{
	GuiTable *table = NULL;
	GuiWidget *child = NULL;

	if((NULL == self) || (NULL == focus))
	{
		return (0);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (0);
	}

	child = table->first_child;
	while(child)
	{
		if(focus == child)
		{
			break;
		}
		child = widget_get_priorsibling(child);
	}

	if(child == table->first_child)
	{
		return (0);
	}

	return (child ? 1 : 0);

}

static int _check_bottom_line(GuiWidget *self, GuiWidget *focus)
{
	GuiTable *table = NULL;
	GuiWidget *child = NULL;
	int height = 0, i = 0;

	if((NULL == self) || (NULL == focus))
	{
		return (0);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (0);
	}

	child = table->first_child;
	while(child)
	{
		if(0 == (i % table->column_num))
		{
			height += (child->rect.h + table->interval);
		}

		if(focus == child)
		{
			break;
		}

		if(child && (0 == strcasecmp("button", child->classname)))
		{
			i++;
		}
		child = widget_get_nextsibling(child);
	}

	return (((height > self->rect.h) && child) ? 1: 0);
}

static GuiWidget *push_up_line(GuiTable *table)
{
	GuiWidget *child = NULL;
	int i = 0;

	if(NULL == table)
	{
		return (NULL);
	}

	child = table->first_child;
	while(child)
	{
		child = widget_get_priorsibling(child);
		if(child && (0 == strcasecmp("button", child->classname)))
		{
			i++;
		}

		if(i == table->column_num)
		{
			break;
		}
	}

	return (child);
}

static GuiWidget *push_down_line(GuiTable *table)
{
	GuiWidget *child = NULL;
	int i = 0;

	if(NULL == table)
	{
		return (NULL);
	}

	child = table->first_child;
	while(child)
	{
		child = widget_get_nextsibling(child);
		if(0 == strcasecmp("button", child->classname))
		{
			i++;
		}

		if(i == table->column_num)
		{
			break;
		}
	}

	return (child);
}

static GuiWidget *push_first_line(GuiTable *table)
{
	GuiWidget *child = NULL;

	child = table->base;
	child = widget_get_firstchild(child);

	while((child) && (0 != strcasecmp("button", child->classname)))
	{
		child = widget_get_nextsibling(child);
	}

	return (child);
}

static GuiWidget *push_last_line(GuiTable *table)
{
	GuiWidget *child = NULL;
	int offset = 0, i = 0;

	offset = table->total_num % table->column_num;
	child = table->base;
	child = widget_get_lastchild(child);

	if(offset)
	{
		i = table->column_num - offset;
	}
	while(child)
	{
		if(i == (table->visible_row * table->column_num - 1))
		{
			break;
		}

		child = widget_get_priorsibling(child);
		if(0 == strcasecmp("button", child->classname))
		{
			i++;
		}
	}


	return (child);
}

static int table_event(GuiWidget *self, void *data)
{
	GuiTable *table = NULL;
	GuiWidget *focus = NULL, *child = NULL, *first = NULL;
	GUI_Event *event = NULL;
	int ret = 0, i = 0, len = 0, offset = 0;

	WIDGET_CHECK_OBJECT(self, data, table, GuiTable);

	event = (GUI_Event *)data;

	offset = table->total_num % table->column_num;
	if(0 == offset)
	{
		offset = table->column_num;
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr callback*/
		if(table->keypress_event)
		{
			ret = exec_widget_event_data(self, table->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}

		}

		/*widget callback*/
		if(GUIK_UP== event->key.sym)
		{
			_table_set_focus(table->focused_child, GUI_FALSE);
			focus = table->focused_child;
			widget_set_update(focus, GUI_TRUE);

			if(table->scrollbar)
			{
				widget_set_update(table->scrollbar, GUI_TRUE);
				widget_set_update(self, GUI_TRUE);
			}

			child = focus;
			while(child)
			{
				child = widget_get_priorsibling(child);
				i++;
				if(i == table->column_num)
				{
					break;
				}
			}

			while((child) && (0 != strcasecmp("button", child->classname)))
			{
				child = widget_get_priorsibling(child);
			}

			if((i != table->column_num) && (NULL == child))
			{
				len = offset - i;
				i = 0;
				child = widget_get_lastchild(self);
				while(i < len)
				{
					child = widget_get_priorsibling(child);
					if(0 == strcasecmp("button", child->classname))
					{
						i++;
					}
				}
			}
			else if(NULL == child)
			{
				child = widget_get_lastchild(self);
			}

			focus = child;
			if(_check_top_line(self, focus))
			{
				table->first_child = push_up_line(table);
				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}
			else if(_check_bottom_line(self, focus))
			{
				table->first_child = push_last_line(table);
				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}
			_table_set_focus(focus, GUI_TRUE);
			table->focused_child = focus;
			widget_set_update(focus, GUI_TRUE);

			self->need_update = 1;

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_DOWN == event->key.sym)
		{
			_table_set_focus(table->focused_child, GUI_FALSE);
			focus = table->focused_child;
			widget_set_update(focus, GUI_TRUE);

			if(table->scrollbar)
			{
				widget_set_update(table->scrollbar, GUI_TRUE);
				widget_set_update(self, GUI_TRUE);
			}

			child = focus;
			i = 0;
			while(child)
			{
				child = widget_get_nextsibling(child);
				if(child && (0 == strcasecmp("button", child->classname)))
				{
					i++;
				}
				if(i == table->column_num)
				{
					break;
				}
			}

			if((i != table->column_num) && (NULL == child))
			{
				len = offset - i;
				i = 0;
				child = widget_get_firstchild(self);//table->first_child;
				while((child) && (0 != strcasecmp("button", child->classname)))
				{
					child = widget_get_nextsibling(child);
				}
				while((i + 1) < len)
				{
					child = widget_get_nextsibling(child);
					if(child && (0 == strcasecmp("button", child->classname)))
					{
						i++;
					}
				}

				if((offset != table->column_num) &&
						(table->focused_child != widget_get_lastchild(self)))
				{
					child = widget_get_firstchild(self);
					while((child) && (0 != strcasecmp("button", child->classname)))
					{
						child = widget_get_nextsibling(child);
					}
				}

				if(len <= 0)
				{
					child = widget_get_lastchild(self);
					while((child) && (0 != strcasecmp("button", child->classname)))
					{
						child = widget_get_priorsibling(child);
					}
				}
			}
			else if((NULL == child) &&
					(offset != table->column_num))
			{
				child = widget_get_lastchild(self);
				while((child) && (0 != strcasecmp("button", child->classname)))
				{
					child = widget_get_priorsibling(child);
				}
			}
			else if(NULL == child)
			{
				child = widget_get_firstchild(self);
				while((child) && (0 != strcasecmp("button", child->classname)))
				{
					child = widget_get_nextsibling(child);
				}
			}

			focus = child;

			if(_check_bottom_line(self, focus))
			{
				table->first_child = push_down_line(table);
				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}
			else if(_check_top_line(self, focus))
			{
				table->first_child = push_first_line(table);
				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}

			_table_set_focus(focus, GUI_TRUE);
			table->focused_child = focus;
			widget_set_update(focus, GUI_TRUE);

			self->need_update = 1;

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == event->key.sym)
		{
			_table_set_focus(table->focused_child, GUI_FALSE);
			focus = table->focused_child;
			widget_set_update(focus, GUI_TRUE);
			focus = widget_get_priorsibling(focus);
			while(focus && (0 != strcasecmp("button", focus->classname)))
			{
				focus = widget_get_priorsibling(focus);
			}

			if(table->scrollbar)
			{
				widget_set_update(table->scrollbar, GUI_TRUE);
				widget_set_update(self, GUI_TRUE);
			}

			if(NULL == focus)
			{
				focus = widget_get_lastchild(self);
				while(0 != strcasecmp("button", focus->classname))
				{
					focus = widget_get_priorsibling(focus);
				}
			}

			first = widget_get_firstchild(self);
			while(0 != strcasecmp("button", first->classname))
			{
				first = widget_get_nextsibling(first);
			}

			if(table->focused_child != first)
			{
				if(_check_bottom_line(self, focus))
				{
					table->first_child = push_down_line(table);

					widget_set_update(self, GUI_TRUE);
					table->updata_all = 1;
				}
				else if(_check_top_line(self, focus))
				{
					table->first_child = push_up_line(table);

					widget_set_update(self, GUI_TRUE);
					table->updata_all = 1;
				}
			}
			else
			{
				table->first_child = push_last_line(table);

				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}

			_table_set_focus(focus, GUI_TRUE);
			table->focused_child = focus;
			widget_set_update(focus, GUI_TRUE);

			self->need_update = 1;

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == event->key.sym)
		{
			_table_set_focus(table->focused_child, GUI_FALSE);
			focus = table->focused_child;
			widget_set_update(focus, GUI_TRUE);
			focus = widget_get_nextsibling(focus);
			if(NULL == focus)
			{
				focus = widget_get_firstchild(self);
				while(0 != strcasecmp("button", focus->classname))
				{
					focus = widget_get_nextsibling(focus);
				}
			}

			if(table->scrollbar)
			{
				widget_set_update(table->scrollbar, GUI_TRUE);
				widget_set_update(self, GUI_TRUE);
			}

			if(table->focused_child != widget_get_lastchild(self))
			{
				if(_check_bottom_line(self, focus))
				{
					table->first_child = push_down_line(table);

					widget_set_update(self, GUI_TRUE);
					table->updata_all = 1;
				}
				else if(_check_top_line(self, focus))
				{
					table->first_child = push_up_line(table);

					widget_set_update(self, GUI_TRUE);
					table->updata_all = 1;
				}
			}
			else
			{
				table->first_child = widget_get_firstchild(self);

				while(0 != strcasecmp("button", table->first_child->classname))
				{
					table->first_child = widget_get_nextsibling(table->first_child);
				}

				widget_set_update(self, GUI_TRUE);
				table->updata_all = 1;
			}

			_table_set_focus(focus, GUI_TRUE);
			table->focused_child = focus;
			widget_set_update(focus, GUI_TRUE);

			self->need_update = 1;

			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			GUI_SendEvent(table->focused_child->name, event);
			return (EVENT_TRANSFER_KEEPON);
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		if(table->clicked_event)
		{
			exec_widget_event_data(self, table->clicked_event, data);
		}
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);
}

static int table_set_property(GuiWidget *self, char *name, void *data)
{
	GuiTable *table = NULL;
	GuiWidget *child = NULL, *first = NULL;
	char *value = NULL;
	int i = 0, column_count = 0, index = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (1);
	}

	if(0 == strcasecmp("focus", name))
	{
		value = (char *)data;
		child = widget_get_firstchild(self);
		while(child)
		{
			if(0 == strcasecmp(value, child->name))
			{
				break;
			}

			if(0 == strcasecmp("button", child->classname))
			{
				i++;
			}

			child = widget_get_nextsibling(child);
		}

		if(child && (child != table->focused_child))
		{
			table->focused_child = child;
			first = widget_get_firstchild(self);

			column_count = i / table->column_num;
			i = 0;

			while(i < column_count)
			{
				first = widget_get_nextsibling(first);
				index++;

				if(0 == index % table->column_num)
				{
					i++;
				}
			}

			if(first)
			{
				table->first_child = first;
			}
		}

		table->updata_all = 1;
		widget_set_update(self, GUI_TRUE);

	}
	else
	{
		return (1);
	}

	return (0);
}

static int table_get_property(GuiWidget *self, char *name, void *data)
{
	GuiTable *table = NULL;
	GuiWidget *child = NULL;
	int count = 0, i = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	table = (GuiTable *)(self->object);
	if(NULL == table)
	{
		return (1);
	}

	if(0 == strcasecmp("total_line", name))
	{
		count = table->total_num / table->column_num;

		if(table->total_num % table->column_num)
		{
			count++;
		}

		*((int *)data) = count;
	}
	else if(0 == strcasecmp("cur_line", name))
	{
		child = widget_get_firstchild(self);
		while(child)
		{
			if(child == table->focused_child)
			{
				break;
			}
			child = widget_get_nextsibling(child);
			i++;
		}

		count = i / table->column_num;
		if(i % table->column_num)
		{
			count++;
		}

		*((int *)data) = count -1;
	}
	else
	{
		return (1);
	}

	return (0);
}

GuiWidgetOps table_ops =
{
	.owner = "table",
	.create = create_table,
	.release = table_release,
	.draw = table_draw,
	.event = table_event,
	.set = table_set_property,
	.get = table_get_property
};

