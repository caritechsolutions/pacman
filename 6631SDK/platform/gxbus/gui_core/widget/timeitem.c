#include "all_widget.h"

static void *create_timeitem(GuiWidget *self)
{
	GuiTimeitem *timeitem = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	timeitem = (GuiTimeitem *)GUI_MALLOC(sizeof(GuiTimeitem));
	if(NULL == timeitem)
	{
		return (NULL);
	}
	memset(timeitem, 0, sizeof(GuiTimeitem));

	self->object = (void *)timeitem;
	timeitem->base = self;

	if(self->signal)
	{
		timeitem->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		timeitem->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		timeitem->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
	}
	if(timeitem->create_event)
	{
		exec_widget_event_data(self, timeitem->create_event, NULL);
	}

	return (NULL);
}

static int timeitem_release(GuiWidget *self)
{
	GuiTimeitem *timeitem = NULL;

	if(NULL == self)
	{
		return (1);
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem)
	{
		return (1);
	}

	if(timeitem->destroy_event)
	{
		exec_widget_event_data(self, timeitem->destroy_event, NULL);
	}

	self->state &= (~WIDGET_STATE_FOCUSED);

	GUI_FREE(self->object);

	return (0);
}

static int _digit_parse(char **string, int *number)
{
	char *pstr = NULL, *pend = NULL, *pnew = NULL;
	int length = 0;

	if(NULL == string)
	{
		return (1);
	}

	pstr = (char *)(*string);
	if(NULL == pstr)
	{
		return (1);
	}

	while(*pstr)
	{
		if('<' == *pstr)
		{
			break;
		}
		pstr++;
	}

	if('<' != *pstr)
	{
		return (1);
	}
	else
	{
		pstr++;
		pend = pstr;

		while(*pend)
		{
			if('>' == *pend)
			{
				break;
			}
			pend++;
		}

		if('>' == *pend)
		{
			length = pend - pstr;
		}

		pnew = (char *)GUI_MALLOC(length + 1);
		if(NULL == pnew)
		{
			return (1);
		}

		memset(pnew, 0, length + 1);
		strncpy(pnew, pstr, length);
		pnew[length] = '\0';

		*number = atoi(pnew);

		GUI_FREE(pnew);

		pstr = pend + 1;
	}

	*string = pstr;

	return (0);

}

static int _time_parse(char **string, TimeData *time)
{
	char *pstr = NULL, *pend = NULL, *pnew = NULL, *pcur = NULL;
	char strnum[5] = {0};
	int length = 0, i = 0;
	char symbol = 0;

	if((NULL == string) || (NULL == time))
	{
		return (1);
	}

	pstr = (char *)(*string);
	if(NULL == pstr)
	{
		return (1);
	}

	while(*pstr)
	{
		if('<' == *pstr)
		{
			break;
		}
		pstr++;
	}

	if('<' != *pstr)
	{
		return (1);
	}
	else
	{
		pstr++;
		pend = pstr;

		while(*pend)
		{
			if('>' == *pend)
			{
				break;
			}
			pend++;
		}
		if('>' == *pend)
		{
			length = pend - pstr;
		}

		pnew = (char *)GUI_MALLOC(length + 1);
		if(NULL == pnew)
		{
			return (1);
		}

		memset(pnew, 0, length + 1);
		strncpy(pnew, pstr, length);
		pnew[length] = '\0';

		pcur = pnew;
		i = 0;
		while(':' != *pcur)
		{
			strnum[i] = *pcur;
			i++;
			pcur++;
		}

		strnum[i] = '\0';
		time->hour = atoi(strnum);

		pcur++;
		i = 0;
		while(*pcur)
		{
			strnum[i] = *pcur;
			i++;
			pcur++;

			if(('+' == *pcur) || ('-' == *pcur))
			{
				symbol = *pcur;
				pcur++;
				break;
			}
		}
		strnum[i] = '\0';
		time->minute = atoi(strnum);

		i = 0;
		while(*pcur)
		{
			strnum[i] = *pcur;
			pcur++;
			i++;
		}

		strnum[i] = '\0';
		time->day_offset = atoi(strnum);
		if('-' == symbol)
		{
			time->day_offset = 0 - time->day_offset;
		}

		GUI_FREE(pnew);

		pstr = pend + 1;
	}

	*string = pstr;

	return (0);
}

static char *_name_parse(char **string)
{
	char *pstr = NULL, *pend = NULL, *pnew = NULL;
	int length = 0;

	if(NULL == string)
	{
		return (NULL);
	}

	pstr = (char *)(*string);
	if(NULL == pstr)
	{
		return (NULL);
	}

	while(*pstr)
	{
		if('<' == *pstr)
		{
			break;
		}
		pstr++;
	}

	if(('<' != *pstr) || ('[' == *(pstr + 1)))
	{
		return (NULL);
	}
	else
	{
		pstr++;
		pend = pstr;

		if(0x10 == *pstr)
		{
			pend += 2;
		}

		while(*pend)
		{
			if('>' == *pend)
			{
				break;
			}
			pend++;
		}
		if('>' == *pend)
		{
			length = pend - pstr;
		}

		pnew = (char *)GUI_MALLOC(length + 1);
		if(NULL == pnew)
		{
			return (NULL);
		}
		memset(pnew, 0, length + 1);

		strncpy(pnew, pstr, length);
		pnew[length] = '\0';

		pstr = pend + 1;
	}

	*string = pstr;

	return (pnew);
}

static int _color_parse(char **string)
{
	char *pstr = NULL, *pend = NULL, *pnew = NULL;
	int length = 0, color = 0;

	if(NULL == string)
		return 0;

	pstr = (char *)(*string);
	if(NULL == pstr)
		return 0;

	if('\0' == *pstr)
		return 0;

	while(*pstr)
	{
		if('<' == *pstr)
			break;
		pstr++;
	}

	if('<' != *pstr)
		return 0;
	else
	{
		pstr++;
		pend = pstr;

		while(*pend)
		{
			if('>' == *pend)
				break;
			pend++;
		}

		if('>' == *pend)
			length = pend - pstr;

		pnew = (char *)GUI_MALLOC(length + 1);
		if(NULL == pnew)
			return 0;

		memset(pnew, 0, length + 1);
		strncpy(pnew, pstr, length);
		pnew[length] = '\0';

		if('#' == *pnew)
		{
			color = hexToInt(pnew + 1);
		} else {
			pstr--;
			*string = pstr;
			return (0);
		}

		GUI_FREE(pnew);
		pstr = pend + 1;
		*string = pstr;

		return color;
	}
}

#ifndef NO_OS
static void _get_rect(const char *value, int *arg0, int *arg1, int *arg2, int *arg3)
{
	char *pstr = NULL, *num_str = NULL;

	pstr = (char *)value;

	num_str = strtok(pstr, ",");
	*arg0 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg1 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg2 = atoi(num_str);
	num_str = strtok(NULL, ",");
	*arg3 = atoi(num_str);
}
#endif /*NO_OS*/

static char *_get_value(char **string)
{
	char *pstr = (char *)*string;
	char *pend = NULL, *pnew = NULL;
	int length = 0;

	while(*pstr) {
		if('[' == *pstr)
			break;
		pstr++;
	}

	if('[' == *pstr) {
		pend = pstr;
		while(*pend) {
			if(']' == *pend)
				break;
            else if('\0' == *pend) {
                *string = pend;
                return NULL;
            }
			pend++;
		}

		if(']' == *pend) {
			length = pend - pstr;
		}

		if(length) {
			pnew = GUI_MALLOCZ((length + 2) * sizeof(char));
			strncpy(pnew, pstr + 1, length - 1);
			*string = *string + length + 1;
		} else {
			*string = pend;
		}
	}
	return (pnew);
}

static int _timeitem_parse_image(char **string, Timeitem_node *node)
{
	char *value = NULL, *image_id = NULL, *rect_value = NULL, *laymode = NULL;
	char *pstr = *string;

	value = _get_value(&pstr);
	if((value) && (0 == strcasecmp("image", value))) {
		image_id = _get_value(&pstr);
		rect_value = _get_value(&pstr);
		laymode = _get_value(&pstr);

		node->image = widget_img_load(image_id);
		_get_rect(rect_value,
			  &(node->rect.x),
			  &(node->rect.y),
			  &(node->rect.w),
			  &(node->rect.h));
		if((laymode) && (0 == strcasecmp("tile", laymode))) {
			node->laymode = LAY_TILE;
		} else {
			node->laymode = LAY_OVERLAP;
		}
		GUI_FREE(value);
		GUI_FREE(image_id);
		GUI_FREE(rect_value);
		GUI_FREE(laymode);
	} else {
		*string = pstr;
		return (0);
	}

	*string = pstr;
	return (0);
}

static int _timeitem_parse_string(const char *string, Timeitem_node *node)
{
	char *pstr = NULL, *name = NULL, *font = NULL;
	int ret = 0, fore_color = 0, back_color = 0;
	int start_minutes = 0, end_minutes = 0;
	TimeData *time_data = NULL;

#define DAY_MINUTES              (24 * 60)

	if((NULL == string) || (NULL == node))
	{
		return (1);
	}

	pstr = (char *)string;

	while(*pstr)
	{
		ret = _digit_parse(&pstr, &(node->series_number));

		ret |= _time_parse(&pstr, &(node->start));
		ret |= _time_parse(&pstr, &(node->end));

		if(ret)
		{
			return (1);
		}

		name = _name_parse(&pstr);
		if(pstr) {
			fore_color = _color_parse(&pstr);
		}

		if(pstr) {
			back_color = _color_parse(&pstr);
		}
		if(pstr) {
			if('\0' != *pstr)
				font = _name_parse(&pstr);
			if((NULL == font) && ('\0' == *pstr)) {
				goto OVER;
			}
			if((*pstr) && ('<' == *pstr)) {
				pstr++;
			}
			if(*pstr == '[') {
				_timeitem_parse_image(&pstr, node);
			} else {
				;
			}
			if(*pstr)
			{
				goto OVER;
			}
		}
	}

OVER:
	node->string = name;
	node->fore_color = fore_color;
	node->back_color = back_color;
	node->font = font;

	return (0);
}

static int _time_compare(TimeData *timea, TimeData *timeb, TimeData *delta)
{
	TimeData time_srca = {0}, time_srcb = {0};

	if((NULL == timea) || (NULL == timeb) || (NULL == delta))
	{
		return (0);
	}

	time_srca = *timea;
	time_srcb = *timeb;

	time_srca.hour = time_srca.hour + 24 * time_srca.day_offset;
	time_srcb.hour = time_srcb.hour + 24 * time_srcb.day_offset;

	if(time_srca.hour > time_srcb.hour)
	{
		delta->hour = time_srca.hour - time_srcb.hour;
		delta->minute = time_srca.minute - time_srcb.minute;
		if(delta->minute < 0)
		{
			delta->hour--;
			delta->minute = (time_srca.minute + 60) - time_srcb.minute;
		}
		return (1);
	}
	else if(time_srca.hour < time_srcb.hour)
	{
		delta->hour = time_srcb.hour - time_srca.hour;
		delta->minute = time_srcb.minute - time_srca.minute;
		if(delta->minute < 0)
		{
			delta->hour--;
			delta->minute = (time_srcb.minute + 60) - time_srca.minute;
		}
		return (-1);
	}
	else
	{
		delta->hour = 0;
		if(time_srca.minute > time_srcb.minute)
		{
			delta->minute = time_srca.minute - time_srcb.minute;
			return (1);
		}
		else if(time_srca.minute < time_srcb.minute)
		{
			delta->minute = time_srcb.minute - time_srca.minute;
			return (-1);
		}
		else
		{
			delta->minute = 0;
			return (0);
		}
	}

}

static int _calc_rect(TimeData* time,
		TimeData* start_time,
		GUI_Rect *rect,
		int time_offset,
		int total_col)
{
	GUI_Rect valid_rect = {0};

	if((NULL == time) || (NULL == start_time) || (NULL == rect))
	{
		return (1);
	}

	valid_rect = *rect;

	/*if(time->hour >= total_col)
	  {
	  return (0);
	  }*/

	rect->w = valid_rect.w / total_col *
		((time->hour + time->day_offset * 24) -
		 (start_time->hour + start_time->day_offset * 24));
	rect->w = rect->w + valid_rect.w / total_col * (time->minute - start_time->minute) / 60;
	rect->w = rect->w * 60 / time_offset;

	return (0);
}

static void _revert_rect(GuiWidget *self, int x0, int x1, GUI_Rect *rect, GUI_Rect *adjust_rect)
{
	const char *value = NULL;

	*adjust_rect = *rect;
	value = widget_get_property(self, "arrange_format");
	if((NULL != value) && (0 == strcasecmp("right_to_left", value))) {
		adjust_rect->x = x1 - (rect->x - x0) - rect->w;
	} else {
		return;
	}
}

static bool _is_in_range(GuiTimeitem *timeitem, int x_offset, int x_start, int x_end)
{
	if((x_offset >= x_start - timeitem->grid_width) && (x_offset <= x_end)) {
		return (true);
	} else {
		return (false);
	}
}

static image_desc ** _get_image_group(GuiTimeitem *timeitem, int index)
{
	if(0 == index) {
		return (timeitem->unfocus_img);
	} else {
		return (timeitem->focus_img);
	}
}

static int timeitem_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiTimeitem *timeitem = NULL;
	GuiWidget *parent = NULL;
	TimeItemPara item_value = {0};
	TimeData *ptime_value = NULL;
	TimeData time_delta = {0}, time_temp = {0};
	Timeitem_node time_node = {0, {0}, {0}, 0};
	char *string = NULL;
	const char *value = NULL;
	int ret = 0, total_col = 0, index = 0, color = 0, xpos = 0, i = 0, wrap = 0, char_height = 0, is_focused_back = GUI_FALSE, is_focused_fore = GUI_FALSE;
	int direction = 0, bound = 0, focus_index = 0, time_flag = 0, text_len = 0;
	int left_bound = 0, right_bound = 0, x_offset = 0, time_offset = 0, item_height = 0;
	bool is_full;
	GUI_Rect valid_rect = {0}, fillrect = {0}, adjust_rect = {0}, focus_rect = {9};
	GAL_Interval interval = {0};
	GuiSurface *surface = NULL;
	struct gdi_font* old_font = NULL, *temp_font = NULL;
	image_desc **p_image_group = NULL;
	GUI_Rect group_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	value = widget_get_property(self, "wrap");
	if((NULL != value) && (0 == strcasecmp("enable", value))) {
		wrap = PARAGRAPH_MODE;
	} else {
		wrap = SINGLELINE_MODE;
	}

	parent = widget_get_parent(self);

	GUI_GetProperty((const char *)parent->name, "item_height", &item_height);
	GUI_GetProperty((const char *)parent->name, "direction", &direction);
	if((MOVE_LEFT == direction) ||
	   (MOVE_RIGHT == direction) ||
	   (MOVE_UP == direction) ||
	   (MOVE_DOWN == direction))
	{
		GUI_GetProperty((const char *)parent->name, "surface", &surface);
	}

	GUI_GetProperty((const char *)parent->name, "total_col", &total_col);
	GUI_GetProperty((const char *)parent->name, "time_offset", &time_offset);

	if(NULL == surface)
	{
		surface = screen;
		bound = self->rect.x + self->rect.w;
	}
	else
	{
		if(MOVE_RIGHT == direction)
		{
			bound = self->rect.x + self->rect.w + self->rect.w / total_col;
		}
		else
		{
			bound = self->rect.x + self->rect.w;
		}
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem)
	{
		return (1);
	}

	old_font = gxgdi_set_font(timeitem->font);

	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		widget_set_property(self, "arrange_format", "right_to_left");
	} else {
		widget_set_property(self, "arrange_format", "left_to_right");
	}
	value = widget_get_property(self, "arrange_format");
	item_value.sel = timeitem->current_select;

	xpos = self->rect.x;
	if(surface != screen)
	{
		xpos -= parent->rect.x;
	}
	item_value.from_start = 1;
	focus_index = 0;

	GUI_GetProperty(self->parent->name, "last_x", &x_offset);
	while(1)
	{
		if(timeitem->getdata_event)
		{
			exec_widget_event_data(self,
					timeitem->getdata_event,
					&item_value);
			item_value.from_start = 0;
		}
		else
		{
			break;
		}

		string = item_value.string;
		if(NULL == string)
		{
			;
		}
		else if(0 == strcasecmp("<end>", string))
		{
			valid_rect = self->rect;
			if(surface != screen)
			{
				if(xpos >= (bound - parent->rect.x))
				{
					break;
				}
				else
				{
					if(xpos)
					{
						valid_rect.x = xpos + timeitem->grid_width;
						valid_rect.w = bound - parent->rect.x - xpos - timeitem->grid_width;
					}
					else
					{
						valid_rect.x = xpos;
						valid_rect.w = bound - parent->rect.x - xpos;
					}

					valid_rect.y -= parent->rect.y;
				}
			}
			else
			{
				valid_rect.x = xpos + timeitem->grid_width;
				valid_rect.w = self->rect.x + self->rect.w - xpos - timeitem->grid_width;
			}

			if(i < timeitem->current_item)
			{
				i = timeitem->current_item;
			}

			index = widget_get_state_index(self);
			if((1 == index) && (i == timeitem->current_item))
			{
				index = 1;
			}
			else
			{
				index = 0;
				if((surface == screen) &&
						(valid_rect.x == (self->rect.x + timeitem->grid_width)))
				{
					valid_rect.x -= timeitem->grid_width;
					valid_rect.w += timeitem->grid_width;
				}
			}

			if(0 >= valid_rect.w)
			{
				timeitem->total_items = i;
				gxgdi_set_font((char *)old_font);
				return (1);
			}

			color = self->back_color[index];
			color = gal_color2index(screen, color);

			is_full = surface == screen ? GUI_TRUE : GUI_FALSE;
			if(is_full) {
				left_bound = self->rect.x;
				right_bound = self->rect.x + self->rect.w;
			} else {
				left_bound = 0;
				right_bound = left_bound + bound - parent->rect.x;
			}
			_revert_rect(self, left_bound, right_bound, &valid_rect, &adjust_rect);
			if((x_offset) &&
			   (widget_get_state_index(self)) &&
			   (!is_focused_back) &&
			   (_is_in_range(timeitem, x_offset, adjust_rect.x, adjust_rect.x + adjust_rect.w))) {
				index = 1;
				timeitem->focus_item = timeitem->current_item = i;
				color = self->back_color[index];
				color = gal_color2index(screen, color);
				is_focused_back = GUI_TRUE;
			}
			p_image_group = _get_image_group(timeitem, index);
			if((p_image_group[0]) && (p_image_group[2])) {
				group_rect.x = adjust_rect.x;
				group_rect.y = adjust_rect.y;
				group_rect.w = adjust_rect.w;
				group_rect.h = adjust_rect.h;
				widget_img_tile_draw(surface,
						     &group_rect,
						     p_image_group[0],
						     p_image_group[1],
						     p_image_group[2],
						     self->back_color[index]);
			} else {
				gal_fillrect(surface,
						(GAL_Rect*)(&adjust_rect),
						color);
			}
			if(index) {
				int x_value = adjust_rect.x;
				const char *key_value = NULL;

				key_value = widget_get_property(self, "arrange_format");
				if((NULL != key_value) && (0 == strcasecmp("right_to_left", key_value))) {
					x_value = x_value + adjust_rect.w;
				}

				if(surface != screen) {
					if(MOVE_RIGHT == direction) {
						x_value = x_value - (self->rect.w / total_col) + self->rect.x;
					} else {
						x_value = x_value + self->rect.x;
					}
				}
				GUI_SetProperty(self->parent->name, "last_x", &x_value);
			}
			break;
		}
		else
		{
			ret = _timeitem_parse_string(string, &time_node);
			if(ret)
			{
				gxlogd("[GUI] Timeitem widget get data error! %s\n", string);
				gxgdi_set_font((char *)old_font);
				timeitem->total_items = i;
				GUI_FREE(time_node.string);
				GUI_FREE(time_node.font);
				return (1);
			}
			timeitem->max_index = time_node.series_number;

			focus_index++;
			GUI_GetProperty((const char *)parent->name,
					"start_time",
					&ptime_value);

			if((time_node.start.hour) &&
			   (time_node.start.hour == ((ptime_value->hour + total_col) % 24)))
				break;

			ret = _time_compare(ptime_value, &time_node.start, &time_delta);
			if(ret > 0)
			{
				ret = _time_compare(ptime_value, &time_node.start, &time_delta);
			}
			if(ret > 0)
			{
				if((ptime_value->hour > 23) &&
						(time_node.end.hour < 24) &&
						(time_node.end.hour < time_node.start.hour))
				{
					ptime_value->hour -= (24 * ptime_value->day_offset);
					time_flag = 1;
				}

				ret = _time_compare(ptime_value,
						&time_node.end,
						&time_delta);
				if(ret >= 0)
				{
					GUI_FREE(time_node.string);
					GUI_FREE(time_node.font);
					continue;
				}
				else
				{
					valid_rect = self->rect;
					_calc_rect(&time_delta,
							&time_temp,
							&valid_rect,
							time_offset,
							total_col);
				}

				if(time_flag)
				{
					ptime_value->hour += (24 * ptime_value->day_offset);
				}
			}
			else if(ret < 0)
			{
				valid_rect = self->rect;
				_calc_rect(&(time_node.start),
						ptime_value,
						&valid_rect,
						time_offset,
						total_col);
				ret = _time_compare(&time_node.start,
						&time_node.end,
						&time_delta);
				valid_rect.x = valid_rect.x + valid_rect.w;
				valid_rect.w = self->rect.w;
				_calc_rect(&time_delta,
						&time_temp,
						&valid_rect,
						time_offset,
						total_col);
			}
			else
			{
				ret = _time_compare(&time_node.start,
						&time_node.end,
						&time_delta);

				if(ret > 0)
				{
					time_node.end.hour += (24 * time_node.end.day_offset);
					ret = _time_compare(&time_node.start,
							&time_node.end,
							&time_delta);
					time_node.end.hour -= (24 * time_node.end.day_offset);
				}

				valid_rect = self->rect;
				_calc_rect(&time_delta,
						&time_temp,
						&valid_rect,
						time_offset,
						total_col);
			}

			if(valid_rect.w > self->rect.w)
			{
				valid_rect.w = bound - valid_rect.x;
			}

			if((valid_rect.x < self->rect.x) ||
					(valid_rect.y < self->rect.y) ||
					(valid_rect.w > (bound - parent->rect.x)) ||
					(valid_rect.h > self->rect.h))
			{
				GUI_FREE(time_node.string);
				GUI_FREE(time_node.font);
				gxgdi_set_font((char *)old_font);
				timeitem->total_items = i;
				return (0);
			}

			//WIDGET_CHECK_RECT(valid_rect, self->rect);

			index = widget_get_state_index(self);
			if((1 == index) && (i == timeitem->current_item) && (0 == x_offset))
			{
				index = 1;
			}
			else
			{
				index = 0;
			}
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if((0 == index) && (time_node.back_color))
			{
				color = gal_color2index(screen, time_node.back_color);
			}

			valid_rect.x += timeitem->grid_width;
			if(valid_rect.w > timeitem->grid_width)
			{
				valid_rect.w -= timeitem->grid_width;
			}

			if((valid_rect.x + valid_rect.w) > (self->rect.x + self->rect.w))
			{
				valid_rect.w = self->rect.x + self->rect.w - valid_rect.x;
			}

			if(0 >= valid_rect.w)
			{
				timeitem->total_items = i;
				gxgdi_set_font((char *)old_font);

				GUI_FREE(time_node.string);
				GUI_FREE(time_node.font);
				return (1);
			}

			if((0 == i) &&
					((valid_rect.x - timeitem->grid_width) > self->rect.x))
			{
				fillrect = self->rect;
				fillrect.w = valid_rect.x - self->rect.x - timeitem->grid_width;
				if((fillrect.x + fillrect.w) > (self->rect.x + self->rect.w))
				{
					fillrect.w = self->rect.x + self->rect.w - fillrect.x;
				}

				if(screen != surface)
				{
					fillrect.x -= parent->rect.x;
					fillrect.y -= parent->rect.y;
					if(MOVE_RIGHT == direction)
					{
						fillrect.x += self->rect.w / total_col;
					}
				}

				is_full = surface == screen ? GUI_TRUE : GUI_FALSE;
				if(is_full) {
					left_bound = self->rect.x;
					right_bound = self->rect.x + self->rect.w;
				} else {
					left_bound = 0;
					right_bound = left_bound + bound - parent->rect.x;
				}
				_revert_rect(self, left_bound, right_bound, &fillrect, &adjust_rect);

				if((x_offset) &&
				   (widget_get_state_index(self)) &&
				   (!is_focused_back) &&
				   (_is_in_range(timeitem, x_offset, adjust_rect.x, adjust_rect.x + adjust_rect.w))) {
					index = 1;
					color = self->back_color[index];
					color = gal_color2index(screen, color);
					timeitem->focus_item = timeitem->current_item = i;
					is_focused_back = GUI_TRUE;
				}
				p_image_group = _get_image_group(timeitem, index);
				if((p_image_group[0]) && (p_image_group[2])) {
					group_rect.x = adjust_rect.x;
					group_rect.y = adjust_rect.y;
					group_rect.w = adjust_rect.w;
					group_rect.h = adjust_rect.h;
					widget_img_tile_draw(surface,
							&group_rect,
							p_image_group[0],
							p_image_group[1],
							p_image_group[2],
							self->back_color[index]);
				} else {
					gal_fillrect(surface,
							(GAL_Rect*)(&adjust_rect),
							color);
				}
				if(index) {
					int x_value = adjust_rect.x;
					const char *key_value = NULL;

					key_value = widget_get_property(self, "arrange_format");
					if((NULL != key_value) && (0 == strcasecmp("right_to_left", key_value))) {
						x_value = x_value + adjust_rect.w;
					}

					if(surface != screen) {
						if(MOVE_RIGHT == direction) {
							x_value = x_value - (self->rect.w / total_col) + self->rect.x;
						} else {
							x_value = x_value + self->rect.x;
						}
					}
					GUI_SetProperty(self->parent->name, "last_x", &x_value);
				}
				i++;

			}

			if(valid_rect.x > bound)
			{
				GUI_FREE(time_node.string);
				GUI_FREE(time_node.font);
				break;
			}
			else if((valid_rect.x + valid_rect.w) > bound)
			{
				GUI_FREE(time_node.string);
				GUI_FREE(time_node.font);
				break;
			}

			index = widget_get_state_index(self);
			if((1 == index) && (i == timeitem->current_item) && (x_offset == 0))
			{
				timeitem->focus_item = time_node.series_number;
				index = 1;
			}
			else
			{
				index = 0;
			}
			color = self->back_color[index];
			color = gal_color2index(screen, color);
			if((0 == index) && (time_node.back_color))
			{
				color = gal_color2index(screen, time_node.back_color);
			}

			_revert_rect(self, left_bound, right_bound, &valid_rect, &adjust_rect);
			focus_rect = adjust_rect;

			if(screen != surface)
			{
				valid_rect.x -= parent->rect.x;
				valid_rect.y -= parent->rect.y;
				if(MOVE_RIGHT == direction)
				{
					if((NULL != value) && (0 == strcasecmp("right_to_left", value))) {
						;
					} else {
						valid_rect.x += self->rect.w / total_col;
					}
				} else if(MOVE_DOWN == direction) {
					valid_rect.y += item_height;
				}
			}
			is_full = surface == screen ? GUI_TRUE : GUI_FALSE;
			if(is_full) {
				left_bound = self->rect.x;
				right_bound = self->rect.x + self->rect.w;
			} else {
				left_bound = 0;
				right_bound = left_bound + bound - parent->rect.x;
			}
			_revert_rect(self, left_bound, right_bound, &valid_rect, &adjust_rect);
			if((x_offset) &&
			   (widget_get_state_index(self)) &&
			   (!is_focused_back) &&
			   (_is_in_range(timeitem, x_offset, adjust_rect.x, adjust_rect.x + adjust_rect.w))) {
				index = 1;
				color = self->back_color[index];
				color = gal_color2index(screen, color);
				timeitem->focus_item = timeitem->current_item = i;
				is_focused_back = GUI_TRUE;
			}

			if(index) {
				timeitem->focus_rect = focus_rect;
			}
			p_image_group = _get_image_group(timeitem, index);
			if((p_image_group[0]) && (p_image_group[2])) {
				group_rect.x = adjust_rect.x;
				group_rect.y = adjust_rect.y;
				group_rect.w = adjust_rect.w;
				group_rect.h = adjust_rect.h;
				widget_img_tile_draw(surface,
						&group_rect,
						p_image_group[0],
						p_image_group[1],
						p_image_group[2],
						self->back_color[index]);
			} else {
				gal_fillrect(surface,
						(GAL_Rect*)(&adjust_rect),
						color);
			}
			if(index) {
				int x_value = adjust_rect.x;
				const char *key_value = NULL;

				key_value = widget_get_property(self, "arrange_format");
				if((NULL != key_value) && (0 == strcasecmp("right_to_left", key_value))) {
					x_value = x_value + adjust_rect.w;
				}

				if(surface != screen) {
					if(MOVE_RIGHT == direction) {
						x_value = x_value - (self->rect.w / total_col) + self->rect.x;
					} else {
						x_value = x_value + self->rect.x;
					}
				}
				if((MOVE_UP != direction) && (MOVE_DOWN != direction)) {
					GUI_SetProperty(self->parent->name, "last_x", &x_value);
				}
			}

			index = widget_get_state_index(self);
			if((1 == index) && (i == timeitem->current_item)  && (0 == x_offset))
			{
				index = 1;
			}
			else
			{
				index = 0;
			}

			if((x_offset) &&
			   (widget_get_state_index(self)) &&
			   (!is_focused_fore) &&
			   (_is_in_range(timeitem, x_offset, adjust_rect.x, adjust_rect.x + adjust_rect.w))) {
				index = 1;
				timeitem->focus_item = timeitem->current_item = i;
				is_focused_fore = GUI_TRUE;
			}
			color = self->fore_color[index];
			color = gal_color2index(screen, color);

			if((0 == index) && (time_node.fore_color))
			{
				color = gal_color2index(screen, time_node.fore_color);
			}

			if(time_node.font) {
			    temp_font = gxgdi_set_font(time_node.font);
			} else {
				temp_font = (struct gdi_font *)timeitem->font;
			}

			text_len = gdi_text_len(time_node.string);
			if(wrap) {
				char_height = gdi_get_charheight();
			} else {
				char_height = gdi_text_get_lines(time_node.string,
								 (GAL_Rect *)&adjust_rect,
								 &interval) * gdi_get_charheight();
			}
			widget_string_alignment(timeitem->alignment,
						text_len,
						char_height,
						&(adjust_rect),
						&(adjust_rect));

			if(LAY_TILE == time_node.laymode) {
				if((adjust_rect.x + adjust_rect.w) > (adjust_rect.x + time_node.rect.x)) {
					adjust_rect.w = time_node.rect.x;
				}
			}

			gxgdi_draw_string(surface,
					time_node.string,
					(GAL_Rect*)(&adjust_rect),
					(GAL_Interval*)(&interval),
					timeitem->alignment,
					color,
					wrap);
			if(time_node.font) {
				old_font = gxgdi_set_font((char *)temp_font);
				temp_font = NULL;
			}

			if(time_node.image) {
				int img_x = 0, img_y = 0;

				_revert_rect(self, left_bound, right_bound, &valid_rect, &adjust_rect);
				img_x = adjust_rect.x + time_node.rect.x;
				img_y = adjust_rect.y + time_node.rect.y;
				if((img_x >= adjust_rect.x) &&
				   (img_y >= adjust_rect.y) &&
				   ((img_x + time_node.rect.w) <= (adjust_rect.x + adjust_rect.w)) &&
				   ((img_y + time_node.rect.h) <= (adjust_rect.y + adjust_rect.h))) {
					gdi_begin();
					gal_img_desc(surface,
						time_node.image,
						img_x,
						img_y,
						time_node.rect.w,
						time_node.rect.h);
					gdi_end();
				}
				widget_img_clear(&(time_node.image));
				time_node.image = NULL;
				memset(&(time_node.rect), 0, sizeof(GAL_Rect));
			}

			if((1 == index) && (i == timeitem->current_item))
			{
				timeitem->focus_item = time_node.series_number;
			}

			index = 0;
			i++;

			GUI_FREE(time_node.string);
			GUI_FREE(time_node.font);

			xpos = valid_rect.x + valid_rect.w;
			if(xpos >= bound)
			{
				if((xpos - valid_rect.w) <= bound)
				{
					valid_rect.w = self->rect.x + self->rect.w - xpos;
				}
				break;
			}
		}
	}

	timeitem->total_items = i;
	gxgdi_set_font((char *)old_font);

	GUI_FREE(time_node.string);
	GUI_FREE(time_node.font);
	return (0);
}

static int _calc_item_total(GuiTimeitem *timeitem)
{
	GuiWidget *self = NULL, *parent = NULL;
	int total_col = 0, time_offset = 0,  total_num = 0, ret = 0;
	TimeData *pstart = NULL, end = {0}, time_delta = {0};
	TimeItemPara item_value = {0};
	Timeitem_node time_node = {0, {0}, {0}, 0};

	if(NULL == timeitem)
	{
		return (0);
	}

	self = timeitem->base;
	if(NULL == self)
	{
		return (0);
	}

	parent = widget_get_parent(self);
	GUI_GetProperty(parent->name, "total_col", &total_col);

	GUI_GetProperty((const char *)parent->name,
			"start_time",
			&pstart);
	GUI_GetProperty((const char *)parent->name,
			"time_offset",
			&time_offset);
	end = *pstart;
	end.minute += (total_col * time_offset);
	end.hour += (end.minute / 60);
	end.minute = end.minute % 60;
//	end.hour += total_col;

	if(end.hour > 23)
	{
		end.hour -= 24;
		end.day_offset++;
	}

	item_value.from_start = 1;
	item_value.sel = timeitem->current_select;
	while(1)
	{
		if(timeitem->getdata_event)
		{
			exec_widget_event_data(self,
					timeitem->getdata_event,
					&item_value);
			item_value.from_start = 0;
		}

		if(0 == strcasecmp("<end>", item_value.string))
		{
			break;
		}
		else
		{
			ret = _timeitem_parse_string(item_value.string, &time_node);
			if(ret)
			{
				gxlogd("[GUI] Timeitem widget get data error! %s\n", item_value.string);
				return (1);
			}

			GUI_FREE(time_node.string);
			GUI_FREE(time_node.font);

			ret = _time_compare(&end, &time_node.end, &time_delta);
			if(ret >= 0)
			{
				ret = _time_compare(pstart, &time_node.start, &time_delta);

				if(ret > 0)
				{
					ret = _time_compare(pstart, &time_node.end, &time_delta);
					if(ret < 0)
					{
						total_num++;
						continue;
					}
				}
				else if(ret <= 0)
				{
					total_num++;
				}
			}
			else
			{
				return (total_num);
			}
		}
	}

	return (total_num);

}

static int _convert_key(GuiWidget *self, int original_key)
{
	GuiCore *gui_core = GUI_GetCurrent();
	const char * value = widget_get_property(self, "arrange_format");

	if((NULL != value) &&
	   (0 == strcasecmp("right_to_left", value)) &&
	   (!gui_core->config.mirror)) {
		if(GUIK_LEFT == original_key) {
			return (GUIK_RIGHT);
		} else if(GUIK_RIGHT == original_key) {
			return (GUIK_LEFT);
		}
	} else {
		return (original_key);
	}

	return (original_key);
}

static int timeitem_event(GuiWidget *self, void *data)
{
	GuiTimeitem *timeitem = NULL;
	GUI_Event *event = NULL;
	int key = 0, total = 0, value = 0;
	const char *property_value = NULL;
	GuiWidget *parent = NULL;

	WIDGET_CHECK_OBJECT(self, data, timeitem, GuiTimeitem);

	event = (GUI_Event *)data;

	parent = widget_get_parent(self);

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		property_value = widget_get_property(self, "arrange_format");
		key = _convert_key(self, key);
		switch(key)
		{
		case GUIK_LEFT:
			GUI_GetProperty(parent->name, "direction", &value);
			if((MOVE_LEFT == value) ||
					(MOVE_RIGHT == value))
			{
				return (0);
			}
			// TODO:
			timeitem->current_item--;
			if(-1 == timeitem->current_item)
			{
				if((NULL != property_value) && (0 == strcasecmp("right_to_left", property_value))) {
					GUI_SetProperty(parent->name, "move_right", NULL);
				} else {
					GUI_SetProperty(parent->name, "move_left", NULL);
				}
				timeitem->current_item = 0;
			}
			value = 0;
			GUI_SetProperty(parent->name, "last_x", &value);
			widget_set_update(self, GUI_TRUE);
			parent->need_update = GUI_TRUE;
			break;
		case GUIK_RIGHT:
			GUI_GetProperty(parent->name, "direction", &value);
			if((MOVE_LEFT == value) ||
					(MOVE_RIGHT == value))
			{
				return (0);
			}
			// TODO:
			timeitem->focus_item++;
			timeitem->current_item++;
			if(timeitem->total_items <= timeitem->current_item)
			{
				if((NULL != property_value) && (0 == strcasecmp("right_to_left", property_value))) {
					GUI_SetProperty(parent->name, "move_left", NULL);
				} else {
					GUI_SetProperty(parent->name, "move_right", NULL);
				}
				total = _calc_item_total(timeitem);
				if(total)
				{
					timeitem->current_item = total - 1;
				}
				else
				{
					timeitem->current_item = total;
				}
			}
			value = 0;
			GUI_SetProperty(parent->name, "last_x", &value);
			widget_set_update(self, GUI_TRUE);
			parent->need_update = GUI_TRUE;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_KEEPON);
}

static int timeitem_set_property(GuiWidget *self, char *name, void *data)
{
	GuiTimeitem *timeitem = NULL;

	if(NULL == self)
	{
		return (1);
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem)
	{
		return (1);
	}

	if(0 == strcasecmp("current_select", name))
	{
		timeitem->current_select = *((int *)(data));
	}
	else if(0 == strcasecmp("font", name))
	{
		timeitem->font = (char *)data;
	}
	else if(0 == strcasecmp("alignment", name))
	{
		timeitem->alignment = *((int *)(data));
	}
	else if(0 == strcasecmp("get_data", name))
	{
		timeitem->getdata_event = (GuiWidgetSignal *)data;
	}
	else if(0 == strcasecmp("grid_width", name))
	{
		timeitem->grid_width = *((int *)(data));
	}
	else if(0 == strcasecmp("current_item", name))
	{
		timeitem->current_item = *((int *)(data));
	}
	else if(0 == strcasecmp("update", name))
	{
		self->parent->need_update = GUI_TRUE;
		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		widget_set_property(self, (const char *)name, (const char *)data);
	}

	return (0);
}

static int timeitem_get_property(GuiWidget *self, char *name, void *data)
{
	GuiTimeitem *timeitem = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem)
	{
		return (1);
	}

	if(0 == strcasecmp("select_row", name))
	{
		if(data)
		{
			*((int *)data) = timeitem->current_select;
		}
	}
	else if(0 == strcasecmp("select_column", name))
	{
		if(data)
		{
			*((int *)data) = timeitem->focus_item;
			if(timeitem->focus_item > timeitem->max_index)
			{
				*((int *)data) = -1;
			}
		}
	}
	else if(0 == strcasecmp("current_select", name))
	{
		if(data)
		{
			*((int *)data) = timeitem->current_select;
		}
	}
	else if(0 == strcasecmp("current_item", name))
	{
		if(data)
		{
			*((int *)data) = timeitem->current_item;
		}
	}
	else if(0 == strcasecmp("total_items", name))
	{
		if(data)
		{
			*((int *)data) = timeitem->total_items;
		}
	}
	else if(0 == strcasecmp("focus_rect", name)) {
		if(data) {
			*((GUI_Rect *)data) = timeitem->focus_rect;
		}
	} else if(0 == strcasecmp("state", name)) {
		return (1);
	}
	else
	{
		*(char **)data = (char *)widget_get_property(self, (const char *)name);
	}

	return (0);
}

static int timeitem_prepare_image(GuiWidget *self)
{
	GuiTimeitem *timeitem = NULL;
	const char *value = NULL;

	if(NULL == self) {
		return (1);
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem) {
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if(NULL != value) {
		widget_img_group_load(value,
				      &(timeitem->unfocus_img[0]),
				      &(timeitem->unfocus_img[1]),
				      &(timeitem->unfocus_img[2]));
	}

	value = widget_get_property(self, "focus_img");
	if(NULL != value) {
		widget_img_group_load(value,
				      &(timeitem->focus_img[0]),
				      &(timeitem->focus_img[1]),
				      &(timeitem->focus_img[2]));
	}

	return (0);
}

static int timeitem_clear_image(GuiWidget *self)
{
	GuiTimeitem *timeitem = NULL;

	if(NULL == self) {
		return (1);
	}

	timeitem = (GuiTimeitem *)(self->object);
	if(NULL == timeitem) {
		return (1);
	}

	widget_img_group_clear(&(timeitem->unfocus_img[0]),
			       &(timeitem->unfocus_img[1]),
			       &(timeitem->unfocus_img[2]));

	widget_img_group_clear(&(timeitem->focus_img[0]),
			       &(timeitem->focus_img[1]),
			       &(timeitem->focus_img[2]));

	return (0);
}

GuiWidgetOps timeitem_ops =
{
	.owner = "timeitem",
	.create = create_timeitem,
	.release = timeitem_release,
	.prepare_image = timeitem_prepare_image,
	.clear_image = timeitem_clear_image,
	.draw = timeitem_draw,
	.event = timeitem_event,
	.set = timeitem_set_property,
	.get = timeitem_get_property
};

