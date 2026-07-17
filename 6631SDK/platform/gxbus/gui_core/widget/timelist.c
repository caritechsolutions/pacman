#include "timelist.h"
#include "gui.h"

static int _get_timelist_format(GuiWidget *self)
	{
		GuiTimelist *timelist = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (FLOP_MOVE);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (FLOP_MOVE);
	}

	value = (const char *)widget_get_property(self, "format");
	if(NULL == value)
	{
		return (FLOP_MOVE);
	}

	if(0 == strcasecmp("flop_move", value))
	{
		return (FLOP_MOVE);
	}
	else if(0 == strcasecmp("glid_move", value))
	{
		return (GLID_MOVE | GLID_UPDATE);
//		return (GLID_MOVE);
	}
	else if(0 == strcasecmp("glid_update", value)) {
		return (GLID_MOVE | GLID_UPDATE);
	}

	return (FLOP_MOVE);
}

static int _config_timelist_parametre(GuiWidget *self)
{
	const char *value = NULL;
	GuiTimelist *timelist = NULL;

	if(NULL == self)
	{
		return (1);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (1);
	}

	timelist->alignment = widget_get_alignment(self);

	value = (const char *)widget_get_property(self, "font");
	if((NULL != value) || (NULL == timelist->font))
	{
		timelist->font = (char *)value;
	}

	value = (const char *)widget_get_property(self, "grid_width");
	if((NULL != value) || (0 == timelist->grid_width))
	{
		timelist->grid_width = widget_get_int(self, "grid_width", 0);
	}

	value = (const char *)widget_get_property(self, "format");
	if((NULL != value) || (0 == timelist->format))
	{
		timelist->format = _get_timelist_format(self);
	}

	value = (const char *)widget_get_property(self, "time_interval");
	if((NULL != value) || (0 == timelist->time_interval))
	{
		timelist->time_interval = widget_get_int(self, "time_interval", 0);

	}

	value = (const char *)widget_get_property(self, "arrange_format");
	if((NULL != value) && (0 == strcasecmp("right_to_left", value))) {
		timelist->arrange_format = RIGHT_TO_LEFT;
	} else {
		timelist->arrange_format = LEFT_TO_RIGHT;
	}

	value = (const char *)widget_get_property(self, "time_offset");
	if((NULL != value) || (0 == timelist->time_offset))
	{
		timelist->time_offset = widget_get_int(self, "time_offset", 60);
	}
	if(timelist->time_offset > 60) {
		timelist->time_offset = 60;
	}

	return (0);
}

static void *create_timelist(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiTimelist *timelist = NULL, *timelist_style = NULL;
	GuiWidget *style = NULL, *child = NULL, *grandchild = NULL;
	GUI_Rect valid_rect = {0};
	int i = 0, total = 0, head_height = 0, real_width = 0, item_height = 0;

	gui_core = GUI_GetCurrent();
	timelist = (GuiTimelist *)GUI_MALLOC(sizeof(GuiTimelist));
	if(NULL == timelist)
	{
		return (NULL);
	}
	memset(timelist, 0, sizeof(GuiTimelist));

	self->object = (void *)timelist;
	timelist->base = self;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		timelist_style = (GuiTimelist *)GUI_MALLOC(sizeof(GuiTimelist));
		if(timelist_style)
		{
			GUI_FREE(timelist);
			return (NULL);
		}

		memset(timelist_style, 0, sizeof(GuiTimelist));
		style->object = (void *)timelist_style;

		_config_timelist_parametre(style);

		timelist->base = self;
		memcpy(timelist, timelist_style, sizeof(GuiTimelist));
		GUI_FREE(style->object);
	}

	_config_timelist_parametre(self);

	if(self->signal)
	{
		timelist->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		timelist->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		timelist->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
		timelist->getdata_event = (GuiWidgetSignal *)hash_get(self->signal, "get_data");
		timelist->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
		timelist->show_event = (GuiWidgetSignal *)hash_get(self->signal, "show");
	}
	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		widget_set_property(self, "arrange_format", "right_to_left");
	} else {
		widget_set_property(self, "arrange_format", "left_to_right");
	}
	child = widget_get_firstchild(self);
	while(child)
	{
		widget_create(child);
		if(0 == strcasecmp("timeitem", child->classname))
		{
			if(0 == i)
			{
				timelist->focus = child;
			}
			i++;
			GUI_SetProperty((const char *)child->name,
					"get_data",
					timelist->getdata_event);
			GUI_SetProperty((const char *)child->name,
					"grid_width",
					&(timelist->grid_width));
			GUI_SetProperty((const char *)child->name,
					"arrange_format",
					(void *)widget_get_property(self, "arrange_format"));
			if(0 == item_height) {
				item_height = child->rect.h + timelist->grid_width;
			}
		}
		else if(0 == strcasecmp("header", child->classname))
		{
			head_height = child->rect.h;
			grandchild = widget_get_firstchild(child);
			while(grandchild)
			{
				if(0 == strcasecmp("text", grandchild->classname))
				{
					timelist->total_col++;
				}
				grandchild = widget_get_nextsibling(grandchild);
			}
		}
		timelist->visible_row++;
		child = widget_get_nextsibling(child);
	}

	timelist->focus->state |= WIDGET_STATE_FOCUSED;
	self->state |= (WIDGET_STATE_FOCUSED | WIDGET_STATE_FOCUSSABLE);
	timelist->direction = TIMELIST_UPDATE;

	if(GLID_MOVE & timelist->format)
	{
		valid_rect = self->rect;
		valid_rect.x = valid_rect.y = 0;
		total = timelist->total_col;
		if(total) {
			real_width = valid_rect.w += (valid_rect.w / total);
		}
		valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		if(GLID_UPDATE & timelist->format) {
			valid_rect.h += item_height * 5;
		}
		timelist->surface = gal_get_surface((GAL_Rect *)(&valid_rect), gui_core->config.bpp);
		valid_rect.w = real_width;
		timelist->timelist_mutex = -1;

		if(timelist->timelist_mutex == -1)
		{
			GxCore_MutexCreate(&timelist->timelist_mutex);
		}
	}

	timelist->head_height = head_height;

	if(timelist->create_event)
	{
		exec_widget_event_data(self, timelist->create_event, NULL);
	}

	timelist->direction = TIMELIST_CREATE;

	return (timelist);
}

static int timelist_release(GuiWidget *self)
{
	GuiTimelist *timelist = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (1);
	}

	if(timelist->surface)
	{
		gal_free_surface(timelist->surface);
		timelist->surface = NULL;
	}

	if(timelist->back_surface)
	{
		gal_free_surface(timelist->back_surface);
		timelist->back_surface = NULL;
	}

	if(timelist->destroy_event)
	{
		exec_widget_event_data(self, timelist->destroy_event, NULL);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	GxCore_MutexDelete(timelist->timelist_mutex);
	timelist->timelist_mutex = -1;

	GUI_FREE(self->object);

	return (0);
}

static int _time2string(TimeData *time, char* string)
{
	char *pstr = NULL;

	if((NULL == time) || (NULL == string))
	{
		return (1);
	}

	pstr = string;
	sprintf(pstr, "%02d:%02d", time->hour, time->minute);

	return (0);
}

static GuiWidget *_get_fix_child(GuiWidget *parent, const char *classname)
{
	GuiWidget *child = widget_get_firstchild(parent);
	while(child) {
		if(0 == strcasecmp(child->classname, classname)) {
			return (child);
		}
		child = widget_get_nextsibling(child);
	}

	return (NULL);
}

static void dynamic_timelist(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiWidget *child = NULL;
	GuiTimelist *timelist = NULL;
	void *surface = NULL;
	int offset = 0, i = 0, xpos = 0, step = 0;
	unsigned int index = 0, color = 0;
	GUI_Rect src_rect = {0}, dst_rect = {0}, valid_rect = {0};

	if(NULL == self)
	{
		return;
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return;
	}

	gui_core = GUI_GetCurrent();

	surface = timelist->surface;
	if(NULL == surface)
	{
		GxCore_MutexUnlock(timelist->timelist_mutex);
		return;
	}

	if(timelist->total_col) {
		offset = self->rect.w / timelist->total_col;
	}

	if(MOVE_LEFT == timelist->direction)
	{
		xpos = offset;
	}
	else if(MOVE_RIGHT == timelist->direction)
	{
		xpos = 0;
	}

	/*if(GLID_MOVE & timelist->format)
	  {
	  GxCore_MutexLock(timelist->timelist_mutex);
	  }*/

	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(screen, color);

	src_rect = dst_rect = self->rect;
	src_rect.x = src_rect.y = 0;
	src_rect.x = xpos;
	dst_rect.h = src_rect.h -= timelist->head_height;
	src_rect.y += timelist->head_height;
	dst_rect.y += timelist->head_height;

	valid_rect = self->rect;
	valid_rect.y += timelist->head_height;
	valid_rect.h -= timelist->head_height;

	if(8 < gui_core->config.bpp)
	{
		step = 12;
	}
	else
	{
		step = 4;
	}

	child = _get_fix_child(self, "header");
	while(i < offset)
	{
		//gal_fillrect(screen, (GAL_Rect *)(&valid_rect), color);
		index = widget_get_state_index(self);
		color = self->back_color[index];
		if((color == gui.config.gui_trans) && (timelist->back_surface))
		{
			GAL_Rect src_rect = {0}, dst_rect = {0};

			src_rect.x = 0;
			src_rect.y = 0;
			src_rect.w = self->rect.w;
			src_rect.h = self->rect.h;

			dst_rect = src_rect;
			dst_rect.x = self->rect.x;
			dst_rect.y = self->rect.y;

			if(child) {
				src_rect.y += (child->rect.h + child->rect.y);
				src_rect.h -= (child->rect.h + child->rect.y);
				dst_rect.y += (child->rect.h + child->rect.y);
				dst_rect.h -= (child->rect.h + child->rect.y);
			}
			hd_copy_surface1(timelist->back_surface, &src_rect, screen, &dst_rect);
			hd_add_blit_element(NULL);
			if(gui_core->config.enable_double_buffer) {
				hd_copy_surface1(timelist->back_surface, &src_rect, back_screen, &dst_rect);
				hd_add_blit_element(NULL);
			}
		}

		gal_stretch_surface(timelist->surface,
				(GAL_Rect *)(&src_rect),
				screen,
				(GAL_Rect *)(&dst_rect));

		if(gui_core->config.enable_double_buffer)
		{
			gal_stretch_surface(timelist->surface,
					(GAL_Rect *)(&src_rect),
					back_screen,
					(GAL_Rect *)(&dst_rect));
		}

		gdi_commit();

		if(MOVE_LEFT == timelist->direction)
		{
			xpos = i;
			src_rect.x -= step;
		}
		else if(MOVE_RIGHT == timelist->direction)
		{
			xpos = i;
			src_rect.x += step;
		}

		i += step;

		if(16 == gui_core->config.bpp)
		{
			gui_delay(1 + timelist->time_interval);
		}
		else
		{
			gui_delay(5 + timelist->time_interval);
		}
	}

	if(MOVE_LEFT == timelist->direction)
	{
		xpos = i;
		src_rect.x = 0;
	}
	else if(MOVE_RIGHT == timelist->direction)
	{
		xpos = i;
		src_rect.x = offset;
	}

	if((color == gui.config.gui_trans) && (timelist->back_surface))
	{
		GAL_Rect src_rect = {0}, dst_rect = {0};

		src_rect.x = 0;
		src_rect.y = 0;
		src_rect.w = self->rect.w;
		src_rect.h = self->rect.h;

		dst_rect = src_rect;
		dst_rect.x = self->rect.x;
		dst_rect.y = self->rect.y;

		if(child) {
			src_rect.y += (child->rect.h + child->rect.y);
			src_rect.h -= (child->rect.h + child->rect.y);
			dst_rect.y += (child->rect.h + child->rect.y);
			dst_rect.h -= (child->rect.h + child->rect.y);
		}
		hd_copy_surface1(timelist->back_surface, &src_rect, screen, &dst_rect);
		hd_add_blit_element(NULL);
		if(gui_core->config.enable_double_buffer) {
			hd_copy_surface1(timelist->back_surface, &src_rect, back_screen, &dst_rect);
			hd_add_blit_element(NULL);
		}
	}

	gal_stretch_surface(timelist->surface,
			(GAL_Rect *)(&src_rect),
			screen,
			(GAL_Rect *)(&dst_rect));

	if(gui_core->config.enable_double_buffer)
	{
		gal_stretch_surface(timelist->surface,
				(GAL_Rect *)(&src_rect),
				back_screen,
				(GAL_Rect *)(&dst_rect));
	}

	gdi_commit();

	if(GLID_MOVE & timelist->format)
	{
		GxCore_MutexUnlock(timelist->timelist_mutex);
	}

	timelist->direction = TIMELIST_UPDATE;
}

static void copy_timelist(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiSurface *surface = NULL;
	GuiTimelist *timelist = NULL;
	GuiWidget *child = NULL;
	int header_height = 0, y0 = 0, y1 = 0, color = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == self) {
		return;
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist) {
		return;
	}

	child = widget_get_firstchild(self);
	while(child) {
		if(0 == strcasecmp("timeitem", child->classname)) {
			y0 = child->rect.y;
			child = widget_get_nextsibling(child);
			if(child) {
				y1 = child->rect.y;
			}
			break;
		} else if(0 == strcasecmp("header", child->classname)){
			header_height = child->rect.h;
		}
		child = widget_get_nextsibling(child);
	}

	if((child) && (0 == timelist->item_height)) {
		timelist->item_pos = y0;
		timelist->item_height = y1 - y0;
	}

	gui_core = GUI_GetCurrent();

	surface = timelist->surface;
	if(NULL == surface) {
		return;
	}

	src_rect.x = src_rect.y = 0;
	src_rect.w = ((GuiSurface *)timelist->surface)->sf.width;
	src_rect.h = ((GuiSurface *)timelist->surface)->sf.height;
	color = self->back_color[0];
	color = hd_color2index(timelist->surface, color);
	gdi_begin();
	hd_fillrect(timelist->surface, &src_rect, color);
	gdi_end();

	src_rect.x = self->rect.x;
	src_rect.y = self->rect.y + timelist->item_pos;
	src_rect.w = self->rect.w;
	src_rect.h = self->rect.h - timelist->item_pos;

	dst_rect = src_rect;
	dst_rect.x = dst_rect.y = 0;
	dst_rect.y += timelist->item_pos;
	if(MOVE_UP == timelist->direction) {
		dst_rect.y += timelist->item_height;
	}

	gdi_commit();
	gal_copy_surface(screen, &src_rect, timelist->surface, &dst_rect);
}

static void dynamic_update(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiTimelist *timelist = NULL;
	GuiSurface *surface = NULL;
	GuiWidget *child = NULL;
	int i = 0, header_height = 0, pos = 0;
	GAL_Rect src_rect = {0}, dst_rect = {0};

	if(NULL == self) {
		return;
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist) {
		return;
	}

	child = widget_get_firstchild(self);
	while(child) {
		if(0 == strcasecmp("header", child->classname)){
			header_height = child->rect.h;
			break;
		}
		child = widget_get_nextsibling(child);
	}

	gui_core = GUI_GetCurrent();

	surface = timelist->surface;
	if(NULL == surface) {
		return;
	}

	dst_rect.x = self->rect.x;
	if(MOVE_DOWN == timelist->direction) {
		dst_rect.y = self->rect.y + timelist->item_pos - 1;
	} else {
		dst_rect.y = self->rect.y + timelist->item_pos + 1;
	}
	dst_rect.w = self->rect.w;
	dst_rect.h = self->rect.h - timelist->item_pos;

	src_rect = dst_rect;
	src_rect.x = src_rect.y = 0;
	src_rect.y += timelist->item_pos;
	if(MOVE_UP == timelist->direction) {
		pos = timelist->item_height;
		src_rect.y += timelist->item_height;
	} else {
		pos = 0;
	}
	for(i = 0; i < timelist->item_height; i++) {
		hd_copy_surface(surface, &src_rect, screen, &dst_rect);
		if(gui_core->config.enable_double_buffer) {
			hd_copy_surface(surface, &src_rect, back_screen, &dst_rect);
		}

		if(MOVE_UP == timelist->direction) {
			pos--;
			src_rect.y--;
		} else {
			pos++;
			src_rect.y++;
		}
		gui_delay(1 + timelist->time_interval);
	}
}

static int timelist_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiTimelist *timelist = NULL;
	GuiWidget *child = NULL, *grandchild = NULL;
	int i = 0, number = 0, header_index = 0, index = 0, color = 0, wrap = 0;
	char time_string[8] = {0};
	const char *value = NULL;
	TimeData time_format = {0};
	GUI_Rect rect = {0}, back_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	value = widget_get_property(self, "wrap");
	if((value) && (0 == strcasecmp("enable", value))) {
		wrap = GUI_TRUE;
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (1);
	}
	timelist->alignment = widget_get_alignment(self);

	if((gui_core->config.mirror) && (!self->disable_mirror)) {
		widget_set_property(self, "arrange_format", "right_to_left");
	} else {
		widget_set_property(self, "arrange_format", "left_to_right");
	}
	value = (const char *)widget_get_property(self, "arrange_format");
	if((NULL != value) && (0 == strcasecmp("right_to_left", value))) {
		timelist->arrange_format = RIGHT_TO_LEFT;
	} else {
		timelist->arrange_format = LEFT_TO_RIGHT;
	}

	if((GLID_MOVE & timelist->format) &&
			((MOVE_LEFT == timelist->direction) ||
			 (MOVE_RIGHT == timelist->direction)))
	{
		GxCore_MutexLock(timelist->timelist_mutex);
	}

	if(((MOVE_UP == timelist->direction) ||
	   (MOVE_DOWN == timelist->direction)) &&
	   (GLID_UPDATE & timelist->format)) {
		copy_timelist(self);
	}

	if((self->back_color[0] == gui.config.gui_trans) ||
	   (self->back_color[1] == gui.config.gui_trans))
	{
		if(NULL == timelist->back_surface)
		{
			gdi_commit();
			timelist->back_surface = widget_get_surface(&(self->rect));
			if(NULL == timelist->back_surface)
			{
				return (1);
			}
		}
		else if((TIMELIST_CREATE == timelist->direction) ||
			(MOVE_LEFT == timelist->direction) ||
			(MOVE_RIGHT == timelist->direction))
		{
			GAL_Rect src_rect = {0}, dst_rect = {0};

			src_rect.x = 0;
			src_rect.y = 0;
			src_rect.w = self->rect.w;
			src_rect.h = self->rect.h;

			dst_rect = src_rect;
			dst_rect.x = self->rect.x;
			dst_rect.y = self->rect.y;
			hd_copy_surface(timelist->back_surface, &src_rect, screen, &dst_rect);
		}
	}

	if(NULL != timelist->surface)
	{
		rect = self->rect;
		rect.x = rect.y = 0;
		if(timelist->total_col) {
			rect.w = rect.w + rect.w / timelist->total_col;
		}
		rect.y += timelist->head_height;
		rect.h -= timelist->head_height;

		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		if(TIMELIST_UPDATE == timelist->direction)
		{
			gdi_commit();
		}
		else if(TIMELIST_CREATE == timelist->direction)
		{
			gal_fillrect(screen,
					(GAL_Rect *)&(self->rect),
					color);
		}
		if((MOVE_LEFT == timelist->direction) ||
		   (MOVE_RIGHT == timelist->direction)) {
			gdi_begin();
			hd_fillrect(timelist->surface,
					(GAL_Rect *)(&rect),
					color);
			hd_add_blit_element(NULL);
			gdi_end();
		}

		rect = self->rect;
		if(MOVE_LEFT == timelist->direction)
		{
			if(timelist->total_col) {
				rect.x = rect.x + rect.w - (rect.w / timelist->total_col);
				rect.w = rect.w / timelist->total_col;
			}
			back_rect = rect;
			rect.y += timelist->head_height;
			rect.h -= timelist->head_height;
			back_rect.x = self->rect.w;
			back_rect.y = timelist->head_height;
			back_rect.h -= timelist->head_height;
			if((self->back_color[0] != gui.config.gui_trans) &&
			   (self->back_color[1] != gui.config.gui_trans)) {
				gdi_commit();
				gal_copy_surface(screen,
						(GAL_Rect*)(&rect),
						timelist->surface,
						(GAL_Rect*)(&back_rect));
				gdi_commit();
			}
		}
		else if(MOVE_RIGHT == timelist->direction)
		{
			if(timelist->total_col) {
				rect.w = rect.w / timelist->total_col;
			}
			back_rect = rect;
			back_rect.x = back_rect.y = 0;
			rect.y += timelist->head_height;
			rect.h -= timelist->head_height;
			back_rect.y += timelist->head_height;
			back_rect.h -= timelist->head_height;
			if((self->back_color[0] != gui.config.gui_trans) &&
			   (self->back_color[1] != gui.config.gui_trans)) {
				gdi_commit();
				gal_copy_surface(screen,
						(GAL_Rect*)(&rect),
						timelist->surface,
						(GAL_Rect*)(&back_rect));
				gdi_commit();
			}
		}
	}

	if((TIMELIST_UPDATE != timelist->direction) &&
			(FLOP_MOVE == timelist->format))
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)(&(self->rect)), color);
	}

	i = 0;
	child = widget_get_firstchild(self);

	if(((MOVE_LEFT == timelist->direction) ||
				(MOVE_RIGHT == timelist->direction)) &&
			(GLID_MOVE & timelist->format))
	{
		gdi_begin();
	}

	while(child)
	{
		if(0 == strcasecmp("timeitem", child->classname))
		{
			number = timelist->current_row + i;
			GUI_SetProperty(child->name, "current_select", (void *)(&number));
			GUI_SetProperty(child->name, "font", (void *)(timelist->font));
			GUI_SetProperty(child->name, "alignment", (void *)(&(timelist->alignment)));
			if(wrap) {
				GUI_SetProperty(child->name, "wrap", "enable");
			}
			i++;
		}
		else if(0 == strcasecmp("header", child->classname))
		{
			if(LEFT_TO_RIGHT == timelist->arrange_format) {
				grandchild = widget_get_firstchild(child);
			} else {
				grandchild = widget_get_lastchild(child);
			}
			header_index = 0;
			while(grandchild)
			{
				if(0 == strcasecmp("text", grandchild->classname))
				{
					time_format = timelist->start_time;
					if(timelist->time_offset == 60) {
						time_format.hour += header_index;
					} else {
						time_format.minute += header_index * timelist->time_offset;
						time_format.hour += (time_format.minute / 60);
						time_format.minute = time_format.minute % 60;
					}
					if(time_format.hour > 23)
					{
						time_format.hour -= 24;
					}
					_time2string(&time_format, time_string);
					GUI_SetProperty(grandchild->name,
							"string",
							time_string);
					header_index++;
				}
				if(LEFT_TO_RIGHT == timelist->arrange_format) {
					grandchild = widget_get_nextsibling(grandchild);
				} else {
					grandchild = widget_get_priorsibling(grandchild);
				}
			}
		}

		if(child->need_update)
		{
			child->rect.x += self->rect.x;
			child->rect.y += self->rect.y;

			widget_draw(child);

			child->rect.x -= self->rect.x;
			child->rect.y -= self->rect.y;
		}
		child = widget_get_nextsibling(child);
	}

	if(((MOVE_LEFT == timelist->direction) ||
				(MOVE_RIGHT == timelist->direction)) &&
			(GLID_MOVE & timelist->format))
	{
		gdi_end();
	}

	if(((MOVE_LEFT == timelist->direction) ||
				(MOVE_RIGHT == timelist->direction)) &&
			(GLID_MOVE & timelist->format))
	{
		dynamic_timelist(self);
	}

	if(((MOVE_UP == timelist->direction) ||
	   (MOVE_DOWN == timelist->direction)) &&
	   (GLID_UPDATE & timelist->format)) {
		dynamic_update(self);
	}

	if(!(GLID_MOVE & timelist->format))
	{
		timelist->direction = TIMELIST_STILL;
	}
	timelist->direction = TIMELIST_STILL;
	if(timelist->show_event) {
		exec_widget_event_data(self, timelist->show_event, NULL);
	}

	return (0);
}

static int _timelist_check_content(GuiWidget *self, int key)
{
	GuiTimelist *timelist = NULL;
	GuiWidget *child = NULL;
	int current_select = 0, total_items = 0, total = 0;

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (-1);
	}

	child = timelist->focus;
	GUI_GetProperty(child->name, "current_item", &current_select);
	GUI_GetProperty(child->name, "total_items", &total_items);

	if(GUIK_UP == key)
	{
		child = widget_get_priorsibling(child);
	}
	else if(GUIK_DOWN == key)
	{
		child = widget_get_nextsibling(child);
	}

	if(child)
	{
		GUI_GetProperty(child->name, "total_items", &total);

		if(total_items) {
			current_select = current_select * total / total_items;
		}

		return (current_select);
	}

	return (0);
}

static int timelist_event(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiTimelist *timelist = NULL;
	GUI_Event *event = NULL;
	GuiWidget *child = NULL;
	int key = 0;
	int ret = 0, value = 0;

	WIDGET_CHECK_OBJECT(self, data, timelist, GuiTimelist);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		key = event->key.sym;
		if(timelist->keypress_event)
		{
			ret = exec_widget_event_data(self, timelist->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}

		child = timelist->focus;

		if(GLID_MOVE & timelist->format)
		{
			GxCore_MutexLock(timelist->timelist_mutex);
		}

		switch(key)
		{
		case GUIK_UP:
			if((timelist->direction != TIMELIST_STILL) &&
			   (!(GLID_UPDATE & timelist->format)))
			{
				break;
			}
			value = _timelist_check_content(self, key);
			timelist->direction = TIMELIST_UPDATE;

			widget_set_update(child, GUI_TRUE);
			child->state &= ~WIDGET_STATE_FOCUSED;
			ret = 0;
			GUI_SetProperty(child->name, "current_item", &ret);
			child = widget_get_priorsibling(child);
			if((NULL == child) ||
					(0 != strcasecmp("timeitem", child->classname)))
			{
				child = timelist->focus;
				if(GLID_UPDATE & timelist->format) {
					timelist->direction = MOVE_UP;
				}
			}

			GUI_SetProperty(child->name, "current_item", &value);
			child->state |= WIDGET_STATE_FOCUSED;
			timelist->focus = child;
			widget_set_update(child, GUI_TRUE);
			if(MOVE_UP == timelist->direction) {
				child = widget_get_nextsibling(child);
				widget_set_update(child, GUI_TRUE);
			}
			self->need_update = GUI_TRUE;
			break;
		case GUIK_DOWN:
			if((timelist->direction != TIMELIST_STILL) &&
			   (!(GLID_UPDATE & timelist->format)))
			{
				break;
			}
			value = _timelist_check_content(self, key);
			timelist->direction = TIMELIST_UPDATE;

			widget_set_update(child, GUI_TRUE);
			child->state &= ~WIDGET_STATE_FOCUSED;
			ret = 0;
			GUI_SetProperty(child->name, "current_item", &ret);
			child = widget_get_nextsibling(child);
			if((NULL == child) ||
					(0 != strcasecmp("timeitem", child->classname)))
			{
				child = timelist->focus;
				if(GLID_UPDATE & timelist->format) {
					timelist->direction = MOVE_DOWN;
				}
			}

			GUI_SetProperty(child->name, "current_item", &value);
			child->state |= WIDGET_STATE_FOCUSED;
			timelist->focus = child;
			widget_set_update(child, GUI_TRUE);
			if(MOVE_DOWN == timelist->direction) {
				child = widget_get_priorsibling(child);
				widget_set_update(child, GUI_TRUE);
			}
			self->need_update = GUI_TRUE;
			break;
		case GUIK_LEFT:
		case GUIK_RIGHT:
			widget_exec_event(timelist->focus, data);
			if((TIMELIST_CREATE == timelist->direction) ||
					(TIMELIST_STILL == timelist->direction))
			{
				timelist->direction = TIMELIST_UPDATE;
			}
			break;
		default:
			break;
		}

		if(GLID_MOVE & timelist->format)
		{
			GxCore_MutexUnlock(timelist->timelist_mutex);
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

static int _time_format_parse(char **string, TimeData *time)
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

		time->day_offset = 0;
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

static char *_convert_direction_name(GuiTimelist *timelist, char *name)
{
	if(LEFT_TO_RIGHT == timelist->arrange_format) {
		return (name);
	}

	if(0 == strcasecmp("move_left", name)) {
		return ("move_right");
	} else if(0 == strcasecmp("move_right", name)) {
		return ("move_left");
	} else {
		return (name);
	}
}

static int timelist_set_property(GuiWidget *self, char *name, void *data)
{
	GuiWidget *widget = NULL;
	GuiTimelist *timelist = NULL;
	char *value = NULL, message_info[10] = {0};
	int item_select = 0, total_item = 0, new_item = 0, new_total = 0;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (1);
	}

	name = _convert_direction_name(timelist, name);

	if(0 == strcasecmp("start_time", name))
	{
		value = (char *)data;
		_time_format_parse(&value, &(timelist->start_time));
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("move_left", name))
	{
		if(LEFT_TO_RIGHT == timelist->arrange_format) {
			timelist->direction = MOVE_LEFT;
		} else {
			timelist->direction = MOVE_RIGHT;
		}
		timelist->start_time.minute = timelist->start_time.hour * 60 + timelist->start_time.minute;
		timelist->start_time.minute -= timelist->time_offset;
		timelist->start_time.hour = timelist->start_time.minute / 60;
		timelist->start_time.minute = timelist->start_time.minute % 60;
		if((0 >= timelist->start_time.hour) || (0 > timelist->start_time.minute))
		{
			timelist->start_time.hour = 23;
			if(timelist->start_time.minute < 0) {
				timelist->start_time.minute = 0 - timelist->start_time.minute;
			}
			timelist->start_time.day_offset -= 1;
		}
		widget_set_update(self, GUI_TRUE);
		if(timelist->change_event)
		{
			strcpy(message_info, "left");
			exec_widget_event_data(self, timelist->change_event, message_info);
		}
	}
	else if(0 == strcasecmp("move_right", name))
	{
		if(LEFT_TO_RIGHT == timelist->arrange_format) {
			timelist->direction = MOVE_RIGHT;
		} else {
			timelist->direction = MOVE_LEFT;
		}
		timelist->start_time.minute = timelist->start_time.hour * 60 + timelist->start_time.minute;
		timelist->start_time.minute += timelist->time_offset;
		timelist->start_time.hour = timelist->start_time.minute / 60;
		timelist->start_time.minute = timelist->start_time.minute % 60;
		if(timelist->start_time.hour >= 24)
		{
			timelist->start_time.hour = 0;
			timelist->start_time.day_offset += 1;
		}
		widget_set_update(self, GUI_TRUE);
		if(timelist->change_event)
		{
			strcpy(message_info, "right");
			exec_widget_event_data(self, timelist->change_event, message_info);
		}
	}
	else if(0 == strcasecmp("update_all", name))
	{
		timelist->direction = TIMELIST_CREATE;
		widget_set_update(self, GUI_TRUE);
		widget_children_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("select_row", name))
	{
		if(NULL == data)
		{
			return (1);
		}

		if(timelist->timelist_mutex)
		{
			GxCore_MutexLock(timelist->timelist_mutex);
		}

		value = (char *)data;
		widget = gui_get_widget(NULL, value);
		GUI_GetProperty(timelist->focus->name, "current_item", &item_select);
		item_select++;
		GUI_GetProperty(timelist->focus->name, "total_items", &total_item);

		GUI_GetProperty(widget->name, "total_items", &new_total);
		if(total_item)
		{
			new_item = (item_select * new_total) / total_item;
		}
		else
		{
			new_item = 0;
		}

		if(new_item)
		{
			new_item--;
		}

		if((new_item) && (new_item >= new_total))
		{
			new_item = new_total - 1;
		}

		if(new_item < 0)
		{
			new_item = 0;
		}
		GUI_SetProperty(widget->name, "current_item", &new_item);

		timelist->direction = TIMELIST_UPDATE;
		timelist->focus->state &= ~WIDGET_STATE_FOCUSED;
		widget_set_update(timelist->focus, GUI_TRUE);
		timelist->focus = widget;
		widget->state |= WIDGET_STATE_FOCUSED;
		widget_set_update(widget, GUI_TRUE);

		self->need_update = GUI_TRUE;

		if(timelist->timelist_mutex)
		{
			GxCore_MutexUnlock(timelist->timelist_mutex);
		}
	}
	else if(0 == strcasecmp("last_x", name)) {
		timelist->last_x = *(int *)data;
	}
	else
	{
		widget_set_property(self, name, data);
		self->need_update = GUI_TRUE;
	}

	return (0);
}

static int timelist_get_property(GuiWidget *self, char *name, void *data)
{
	GuiTimelist *timelist = NULL;
	TimeData *time_data = NULL;
	char time_str[8] = {0}, *pstr = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist)
	{
		return (1);
	}

	if(0 == strcasecmp("start_time", name))
	{
		if(data)
		{
			time_data = &(timelist->start_time);
			*(TimeData **)data = time_data;
		}
	}
	else if(0 == strcasecmp("start", name))
	{
		if(data)
		{
			time_data = &(timelist->start_time);
			sprintf(time_str, "<%02d:%02d>", time_data->hour, time_data->minute);
			pstr = (char *)data;
			strcpy(pstr, time_str);
		}
	}
	else if(0 == strcasecmp("total_col", name))
	{
		if(data)
		{
			*((int *)data) = timelist->total_col;
		}
	}
	else if(0 == strcasecmp("surface", name))
	{
		if(data)
		{
			*(void**)data = timelist->surface;
		}
	}
	else if(0 == strcasecmp("direction", name))
	{
		if(data)
		{
			*((int *)data) = timelist->direction;
		}
	}
	else if(0 == strcasecmp("select_row", name))
	{
		GUI_GetProperty(timelist->focus->name, "select_row", data);
	}
	else if(0 == strcasecmp("select_column", name))
	{
		GUI_GetProperty(timelist->focus->name, "select_column", data);
	}
	else if(0 == strcasecmp("current_item", data))
	{
		GUI_GetProperty(timelist->focus->name, "current_item", data);
	}
	else if(0 == strcasecmp("last_x", name)) {
		if(data)
		{
			if((MOVE_UP == timelist->direction) ||
			   (MOVE_DOWN == timelist->direction)) {
				*((int *)data) = timelist->last_x - self->rect.x;
			} else {
				*((int *)data) = timelist->last_x;
			}
		}
	}
	else if(0 == strcasecmp("focus_rect", name)) {
		if(data) {
			GUI_GetProperty(timelist->focus->name, "focus_rect", data);
		}
	}
	else if(0 == strcasecmp("time_offset", name)) {
		if(data) {
			*((int *)data) = timelist->time_offset;
		}
	}
	else if(0 == strcasecmp("item_height", name)) {
		if(data) {
			*((int *)data) = timelist->item_height;
		}
	}
	else if(0 == strcasecmp("state", name)) {
		return (1);
	}
	else
	{
		*(char **)data = (char *)widget_get_property(self, (const char *)name);
	}

	return (0);
}

static int timelist_unactive(GuiWidget *self)
{
	GuiTimelist *timelist = NULL;

	if(NULL == self) {
		return (1);
	}

	timelist = (GuiTimelist *)(self->object);
	if(NULL == timelist) {
		return (1);
	}

	if(timelist->back_surface) {
		gal_free_surface(timelist->back_surface);
		timelist->back_surface = NULL;
	}

	if(timelist->surface) {
		gal_free_surface(timelist->surface);
		timelist->surface = NULL;
	}

	return (0);
}

GuiWidgetOps timelist_ops =
{
	.owner = "timelist",
	.create = create_timelist,
	.release = timelist_release,
	.draw = timelist_draw,
	.unactive = timelist_unactive,
	.event = timelist_event,
	.set = timelist_set_property,
	.get = timelist_get_property
};

