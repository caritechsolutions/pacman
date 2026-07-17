#include "hd_gal.h"
#include "gui_private.h"
#include "gui_core.h"
#include "all_widget.h"

extern int GxGuiViewServiceRecvMsgStop(void);

static void _locale_surface(GuiWidget *self, void *data)
{
	GuiPrompt *prompt = NULL;
	GuiCore *gui_core = NULL;
	GAL_Rect rect = {0};
	int color = 0;
	const char *value = NULL;

	if(NULL == self)
	{
		return;
	}

	prompt = (GuiPrompt *)(self->object);
	if(NULL == prompt)
	{
		return;
	}

	gui_core = GUI_GetCurrent();

	if((prompt->locale_surface) &&
		((prompt->locale_surface->sf.width != self->rect.w) ||
		(prompt->locale_surface->sf.height != self->rect.h)))
	{
		if(prompt->layer_name)
		{
			GxGDI_LayerUnregisterNolock(prompt->layer_name);
			gal_free_surface(prompt->locale_surface);
			GUI_FREE(prompt->layer_name);
			prompt->layer_name = NULL;
		}
		prompt->locale_surface = NULL;
	}
	value = (const char *)data;
	if(NULL == value)
	{
		return;
	}

	rect.x = self->rect.x;
	rect.y = self->rect.y;
	rect.w = self->rect.w;
	rect.h = self->rect.h;
	if((0 == strcasecmp("true", value)) && (NULL == prompt->locale_surface))
	{
		prompt->locale_surface = gal_get_surface(&rect, gui_core->config.bpp);
		if(NULL == prompt->locale_surface)
		{
			return;
		}

		if(NULL == prompt->layer_name)
		{
			prompt->layer_name = GxGDI_LayerRegisterNolock(prompt->locale_surface, self->rect.x, self->rect.y);
			if(prompt->layer_name) {
				prompt->layer_name = GUI_STRDUP(prompt->layer_name);
			}
			if(NULL == prompt->layer_name)
			{
				gxlogd("[GUI] %s prompt Register surface failed.\n", self->name);
			}
			color = hd_color2index(prompt->locale_surface, gui_core->config.gui_trans);
			gdi_begin();
			hd_fillrect(prompt->locale_surface, &rect, color);
			gdi_end();
		}
	}
}

static void *create_prompt(GuiWidget *self)
{
	GuiWidget *style = NULL, *child = NULL;
	GuiPrompt *prompt = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	prompt = (GuiPrompt *)GUI_MALLOC(sizeof(GuiPrompt));
	if(NULL == prompt)
	{
		return (NULL);
	}
	memset(prompt, 0, sizeof(GuiPrompt));

	prompt->base = self;
	self->object = (void *)prompt;

	style = (GuiWidget *)widget_get_style(self->stylename);
	if(NULL == style)
	{
		gxlogd("[GUI] Prompt is not valid.\n");
		return (NULL);
	}
	else
	{

	}

	prompt->surface = NULL;
	prompt->end_draw = GUI_FALSE;
	self->rect.x = self->rect.y = 0;

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_create(child);
		//child->rect.x += self->rect.x;
		//child->rect.y += self->rect.y;
		child = widget_get_nextsibling(child);
	}

	if(self->signal)
	{
		prompt->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		prompt->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
	}
	widget_set_update(self, GUI_TRUE);

	return (prompt);
}

static int prompt_release(GuiWidget *self)
{
	GuiPrompt *prompt = NULL;

	prompt = (GuiPrompt *)(self->object);
	if(NULL == prompt)
	{
		return (1);
	}

	if(NULL == prompt->locale_surface)
	{
		prompt->end_draw = GUI_TRUE;
		GxCore_MutexUnlock(self->widget_mutex);
		gdi_lock();
		widget_draw(self);
		gdi_unlock();
		GxCore_MutexLock(self->widget_mutex);
		prompt->end_draw = GUI_FALSE;
	}

	if(prompt->surface)
	{
		gal_free_surface(prompt->surface);
		prompt->surface = NULL;
	}

	if(prompt->locale_surface)
	{
		if(prompt->layer_name)
		{
			GxGDI_LayerUnregisterNolock(prompt->layer_name);
			gal_free_surface(prompt->locale_surface);
			GUI_FREE(prompt->layer_name);
			prompt->layer_name = NULL;
		}
		prompt->locale_surface = NULL;
	}

	if(prompt->destroy_event)
	{
		exec_widget_event_data(self, prompt->destroy_event, NULL);
	}

	GUI_FREE(self->object);

	return (0);
}

static int _is_rolling(GuiWidget *self)
{
	GuiWidget *child = NULL;

	child = widget_get_firstchild(self);
	while(child) {
		char* value = NULL;
		GUI_GetProperty(child->name, "rolling_state", &value);
		if(value && (0 != strcasecmp(ROLL_STATE_STOP, value))) {
			return (GUI_TRUE);
		}
		child = widget_get_nextsibling(child);
	}

	return (GUI_FALSE);
}

static int prompt_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiPrompt *prompt = NULL;
	GuiWidget *child = NULL;
	GuiSurface *old_surface = NULL;
	GUI_Rect rect = {0};
	int color = 0, index = 0, is_roll = 0;

	if(NULL == self)
	{
		return (1);
	}

	is_roll = _is_rolling(self);
	if(is_roll) {
		return (0);
	}

	prompt = (GuiPrompt *)(self->object);
	if(NULL == prompt)
	{
		return (1);
	}

	if((self->rect.x < 0) ||
	   (self->rect.y < 0) ||
	   ((self->rect.x + self->rect.w) > gui.config.width) ||
	   ((self->rect.y + self->rect.h) > gui.config.height) ||
	   (self->rect.w <= 0) ||
	   (self->rect.h <= 0))
	{
		gxlogd("[GUI] Error %s prompt rect is [%d, %d, %d, %d]\n", self->name, self->rect.x, self->rect.y, self->rect.w,
				self->rect.h);
		return (1);
	}

	if(prompt->layer_name) {
		char *value = NULL;

		child = widget_get_firstchild(self);
		if((child) &&
		   (0 == strcasecmp("text", child->classname))) {
			GUI_GetProperty(child->name, "rolling_state", &value);
			if(value && (0 != strcasecmp(ROLL_STATE_STOP, value))) {
				GxGDI_LayerEnable(prompt->layer_name, GUI_FALSE);
			}
		}
	}

	gui_core = GUI_GetCurrent();
	if(NULL == prompt->locale_surface)
	{
		if(GUI_TRUE == prompt->end_draw)
		{
			rect = self->rect;
			rect.x = rect.y = 0;

			gal_copy_surface(prompt->surface,
					(GAL_Rect*)&rect,
					screen,
					(GAL_Rect*)(&(self->rect)));

			if(gui_core->config.enable_double_buffer)
			{
				gal_copy_surface(prompt->surface,
						(GAL_Rect*)&rect,
						back_screen,
						(GAL_Rect*)(&(self->rect)));
			}

			gdi_commit();
			return (0);
		}
		else if(NULL == prompt->surface)
		{
			rect = self->rect;
			rect.x = rect.y = 0;
			prompt->surface = gal_get_surface((GAL_Rect*)&rect,
					gui_core->config.bpp);
			if(NULL == prompt->surface)
			{
				return (1);
			}

			gdi_begin();
			color = gal_color2index(screen, gui_core->config.osd_trans);
			gal_fillrect(prompt->surface, (GAL_Rect*)&rect, color);
			gdi_end();

			gdi_commit();

			gal_copy_surface(screen,
					(GAL_Rect*)(&(self->rect)),
					prompt->surface,
					(GAL_Rect*)&rect);

			gdi_commit();
		}
	}
	else if((32 == gui.config.bpp) &&
	        (prompt->surface))
	{
		rect = self->rect;
		rect.x = rect.y = 0;

		gal_copy_surface(prompt->surface,
				(GAL_Rect*)&rect,
				screen,
				(GAL_Rect*)(&(self->rect)));
	}

	if(prompt->locale_surface) {
		gui_core->dst_surface = prompt->locale_surface;
	} else {
		const char *value = NULL;

		value = widget_get_property(self, "locale_surface");
		if(((NULL == value) ||
		   ((NULL != value) && (0 == strcasecmp("true", value))))) {
			return (0);
		}
	}
	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(prompt->locale_surface, color);
	gal_fillrect(prompt->locale_surface, (GAL_Rect *)(&(self->rect)), color);

	child = widget_get_firstchild(self);
	while(child)
	{
		//child->rect.x += self->rect.x;
		//child->rect.y += self->rect.y;
		widget_draw(child);
		//child->rect.x -= self->rect.x;
		//child->rect.y -= self->rect.y;
		child = widget_get_nextsibling(child);
	}
	gui_core->dst_surface = NULL;

	return (0);
}

static int _prompt_set_prior_widget(GuiPrompt *prompt)
{
	GuiCore *gui_core = NULL;
	GuiWidget *focus = NULL, *old_widget = NULL;

	if(NULL == prompt)
	{
		return (1);
	}

	focus = prompt->active_widget;
	if(NULL == focus)
	{
		return (0);
	}

	gui_core = GUI_GetCurrent();
	//make sure next loop not dead
	if(!widget_get_focussable(focus))
	{
		return (1);
	}

	focus = widget_get_priorsibling(focus);
	if(NULL == focus)
	{
		focus = widget_get_lastchild(prompt->base);
	}

	while(focus)
	{
		if(widget_get_focussable(focus))
		{
			if(focus == prompt->active_widget)
			{
				return 0;
			}

			if(focus)
			{
				old_widget = gui_core->config.focus_widget;
				old_widget->state &= ~WIDGET_STATE_FOCUSED;
				widget_set_update(old_widget, GUI_TRUE);

				gui_core->config.focus_widget = focus;
				focus->state |= WIDGET_STATE_FOCUSED;

				prompt->active_widget = focus;

				widget_set_update(focus, GUI_TRUE);
			}

			return 0;
		}

		focus = widget_get_priorsibling(focus);
		if(NULL == focus)
		{
			focus = widget_get_lastchild(prompt->base);
		}
	}

	return (0);
}

static int _prompt_set_next_widget(GuiPrompt *prompt)
{
	GuiCore *gui_core = NULL;
	GuiWidget *focus = NULL, *old_widget = NULL;;

	if(NULL == prompt)
	{
		return (1);
	}

	focus = prompt->active_widget;
	if(NULL == focus)
	{
		return 0;
	}

	gui_core = GUI_GetCurrent();
	//make sure next loop not dead
	if(!widget_get_focussable(focus))
	{
		return 1;
	}

	focus = widget_get_nextsibling(focus);
	if(NULL == focus)
	{
		focus = widget_get_firstchild(prompt->base);
	}

	while(focus)
	{

		if(widget_get_focussable(focus))
		{
			if(focus == prompt->active_widget)
			{
				return 0;
			}

			if(focus)
			{
				old_widget = gui_core->config.focus_widget;
				old_widget->state &= ~WIDGET_STATE_FOCUSED;
				widget_set_update(old_widget, GUI_TRUE);

				gui_core->config.focus_widget = focus;
				focus->state |= WIDGET_STATE_FOCUSED;

				prompt->active_widget = focus;

				widget_set_update(focus, GUI_TRUE);
			}
			return 0;
		}

		focus = widget_get_nextsibling(focus);
		if(NULL == focus)
		{
			focus = widget_get_firstchild(prompt->base);
		}
	}

	return (0);
}

static int _add_key(GuiWidget* self,  GUI_Event* event)
{
	GuiCore *gui_core = NULL;
	GuiPrompt *prompt = NULL;
	GuiWidget *focus = NULL;
	char *return_string = NULL;

	gui_core = GUI_GetCurrent();
	prompt = (GuiPrompt *)(self->object);

	switch(event->key.sym)
	{
	case GUIK_RETURN:
		focus = gui_core->config.focus_widget;
		return_string = (char *)widget_get_property(focus, "string");
		widget_set_property(self, "return", return_string);
		widget_release(self);
		break;
	case GUIK_LEFT:
	case GUIK_UP:
		_prompt_set_prior_widget(prompt);
		break;
	case GUIK_RIGHT:
	case GUIK_DOWN:
		_prompt_set_next_widget(prompt);
		break;
	default:
		return (EVENT_TRANSFER_KEEPON);
	}

	return (EVENT_TRANSFER_STOP);
}

static int prompt_event(GuiWidget *self, void *data)
{
	GuiPrompt *prompt = NULL;
	GUI_Event *event = NULL;
	//int ret = 0;

	WIDGET_CHECK_OBJECT(self, data, prompt, GuiPrompt);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		return (_add_key(self, event));
		break;
	default:
		break;
	}

	return (EVENT_TRANSFER_STOP);
}

static int prompt_set_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = NULL;
	GuiPrompt *prompt = NULL;
	GuiWidget *child = NULL;
	GUI_Rect rect = {0}, new_rect = {0};
	const char *value = NULL;
	int count_ontop = 0, i = 0, is_roll = 0;
	GuiWidget *ontop_window = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	prompt = (GuiPrompt *)(self->object);
	if(NULL == prompt)
	{
		return (1);
	}

	if(0 == strcasecmp("update_surface", name))
	{
		if(prompt->surface)
		{
			is_roll = _is_rolling(self);
			if(is_roll) {
				return (0);
			}

			rect = self->rect;
			rect.x = rect.y = 0;

			gal_copy_surface(prompt->surface,
					(GAL_Rect*)&rect,
					screen,
					(GAL_Rect*)(&(self->rect)));

			prompt_draw(self);
			gdi_commit();
		}
	}
	else if(0 == strcasecmp("change_surface", name))
	{
		if(prompt->surface)
		{
			is_roll = _is_rolling(self);
			if(is_roll) {
				return (0);
			}

			rect = self->rect;
			rect.x = rect.y = 0;

			hd_free_surface(prompt->surface);
			prompt->surface = NULL;

			prompt->surface = gal_get_surface((GAL_Rect *)&(self->rect), gui_core->config.bpp);
			gdi_commit();
			gal_copy_surface(screen,
					(GAL_Rect*)(&(self->rect)),
					prompt->surface,
					(GAL_Rect*)&rect);
			widget_set_update(self, GUI_TRUE);
		}
	}
	else if(0 == strcasecmp("rect", name))
	{
		char rect_value[100] = {0};

		if((self->rect.x < 0) ||
		(self->rect.y < 0) ||
		((self->rect.x + self->rect.w) > gui.config.width) ||
		((self->rect.y + self->rect.h) > gui.config.height) ||
		(self->rect.w <= 0) ||
		(self->rect.h <= 0))
		{
			gxlogd("[GUI] Error %s prompt rect is [%d, %d, %d, %d]\n", self->name, self->rect.x, self->rect.y, self->rect.w,
					self->rect.h);
			return (1);
		}
		rect = self->rect;
		if(prompt->surface)
		{
			rect = self->rect;
			rect.x = rect.y = 0;

			count_ontop = stack_count(gui_core->ontop_stack);
			for(i = 0; i < count_ontop; i++)
			{
				ontop_window = (GuiWidget *) stack_get(gui_core->ontop_stack, i);
				GUI_SetProperty(ontop_window->name, "update_surface", NULL);
			}

			gal_copy_surface(prompt->surface,
					(GAL_Rect*)&rect,
					screen,
					(GAL_Rect*)(&(self->rect)));

			gdi_commit();

			for(i = 0; i < count_ontop; i++)
			{
				ontop_window = (GuiWidget *) stack_get(gui_core->ontop_stack, i);
				GUI_SetProperty(ontop_window->name, "change_surface", NULL);
			}

			for(i = 0; i < count_ontop; i++)
			{
				ontop_window = (GuiWidget *) stack_get(gui_core->ontop_stack, i);
				if(ontop_window != self)
				{
					gdi_lock();
					widget_draw(ontop_window);
					gdi_commit();
					gdi_unlock();
				}
			}
			//wm_copy_main_surface();
			hd_free_surface(prompt->surface);
			prompt->surface = NULL;
		}
		widget_set_property(self, name, data);
		value = widget_get_property(self, name);
		if(value)
		{
			sscanf(value, "[%d,%d,%d,%d]", &new_rect.x, &new_rect.y, &new_rect.w, &new_rect.h);

			if((0 == new_rect.w) || (0 == new_rect.h))
				return (1);

			if(((new_rect.x + new_rect.w) > gui_core->config.width) ||
					(new_rect.x < 0) ||
					((new_rect.y + new_rect.h) > gui_core->config.height) ||
					(new_rect.y < 0))
			{
				return (1);
			}

			child = widget_get_firstchild(self);
			while(child)
			{
				child->rect.x += (new_rect.x - self->rect.x);
				child->rect.y += (new_rect.y - self->rect.y);
				child->rect.w = child->rect.w * new_rect.w / rect.w;
				child->rect.h = child->rect.h * new_rect.h / rect.h;

				sprintf(rect_value, "[0,0,%d,%d]", child->rect.w, child->rect.h);
				widget_set_property(child, name, rect_value);
				child = widget_get_nextsibling(child);
			}
			self->rect = new_rect;
			widget_set_update(self, GUI_TRUE);
		}
		if((prompt->locale_surface) && (prompt->layer_name)) {
			int x_offset = 0, y_offset = 0;
			self->rect.x = self->rect.y = 0;
			child = widget_get_firstchild(self);
			while(child) {
				x_offset = child->rect.x - self->rect.x;
				y_offset = child->rect.y - self->rect.y;
				child->rect.x -= x_offset;
				child->rect.y -= y_offset;

				sprintf(rect_value, "[0,0,%d,%d]", child->rect.w, child->rect.h);
				widget_set_property(child, name, rect_value);
				child = widget_get_nextsibling(child);
			}
			_locale_surface(self, (void *)"true");
			sscanf(value, "[%d,%d,%d,%d]", &new_rect.x, &new_rect.y, &new_rect.w, &new_rect.h);
			GxGDI_LayerSetPosition(prompt->layer_name, new_rect.x, new_rect.y);
			return (0);
		}
	}
	else if(0 == strcasecmp("update", name))

	{
		child = widget_get_firstchild(self);
		while(child)
		{
			widget_set_update(child, GUI_TRUE);
			child->rect.x += self->rect.x;
			child->rect.y += self->rect.y;
			gdi_lock();
			widget_draw(child);
			gdi_unlock();
			child->rect.x -= self->rect.x;
			child->rect.y -= self->rect.y;
		}
	}
	else if(0 == strcasecmp("locale_surface", name))
	{
		widget_set_property(self, name, data);
	}

	return (0);
}

static int prompt_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

GuiWidgetOps prompt_ops =
{
	.owner = "prompt",
	.create = create_prompt,
	.release = prompt_release,
	.draw = prompt_draw,
	.event = prompt_event,
	.set = prompt_set_property,
	.get = prompt_get_property
};

const char *GUI_CreatePrompt(int x,
		int y,
		const char *name,
		const char *style,
		const char *string,
		const char *mode)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL, *widget_style = NULL, *child = NULL;
	GuiWidget *focus = NULL, *old_widget = NULL, *next = NULL;
	GuiWidgetOps *widget_ops = NULL;
	GuiPrompt *prompt = NULL;
	char *mode_string = NULL, *token_string = NULL;
	char *button_string[20];
	char *ret_string = NULL;
	const char *value = NULL;
	int i = 0, count = 0, ret = 0, ontop = 0;
	GUI_Event event = { 0 };
	char *save_p = NULL;
	widget_ops = widget_get_ops("prompt");
	if(NULL == widget_ops)
	{
		return (NULL);
	}

	gui_core = GUI_GetCurrent();
	/*Parse mode*/
	if(mode)
	{
		if(0 == strcasecmp("ontop", mode))
		{
			ontop = 1;
		}
		else
		{
			i = 0;
			if(NULL != mode)
			{
				mode_string = GUI_STRDUP(mode);
			}
			token_string = strtok_r(mode_string, "|", &save_p);
			if(token_string)
			{
				button_string[i] = GUI_STRDUP(token_string);
				i++;
				for(;;)
				{
					if(NULL == token_string)
					{
						break;
					}
					token_string = strtok_r(NULL, "|", &save_p);
					if(token_string)
					{
						button_string[i] = GUI_STRDUP(token_string);
					}
					i++;
				}
			}
			GUI_FREE(mode_string);
			count = i - 1;
		}
	}
	else
	{
		count = 0;
	}

	i = 0;

	widget = (GuiWidget *)GUI_MALLOC(sizeof(GuiWidget));
	if (NULL == widget)
	{
		return (NULL);
	}
	memset(widget, 0, sizeof(GuiWidget));

	widget_style = (GuiWidget *)widget_get_style(style);
	if(NULL == widget_style)
	{
		GUI_Debug_Print("[GUI] Prompt is not valid.\n");
		return (NULL);
	}
	else
	{
		memcpy(widget, widget_style, sizeof(GuiWidget));
	}

	if(widget){
		widget->classname = GUI_STRDUP("prompt");
		widget->ops = widget_ops;
		widget->name = GUI_STRDUP(name);
		widget->parent = NULL;
		if(style)
		{
			widget->stylename = GUI_STRDUP(style);
		}

		widget->first_child =
			widget->last_child =
			widget->prior_sibling =
			widget->next_sibling = NULL;
		if(ontop)
		{
			if(gui_core->ontop_stack)
			{
				stack_push(gui_core->ontop_stack, (void *)widget);
			}
		}
	}

	/*Copy each of the children widgets deeply.*/
	widget_style = widget_get_firstchild(widget_style);
	while(widget_style)
	{
		child = widget_new(widget_style->name,
				widget_style->classname,
				widget_style->stylename);
		GUI_FREE(child->name);
		GUI_FREE(child->classname);
		GUI_FREE(child->stylename);
		GUI_FREE(child->xml_filename)
			memcpy(child, widget_style, sizeof(GuiWidget));

		if((0 == strcasecmp("text", child->classname)) && (string))
		{
			widget_set_property(child, "string", string);
			string = NULL;
		}
		if(count)
		{
			if(0 == strcasecmp("button", child->classname))
			{
				if(NULL == focus)
				{
					focus = child;
				}
				if(i < count)
				{
					widget_set_property(child,
							"string",
							(const char *)button_string[i]);
				}
				i++;
			}
		}

		if(count)
		{
			if(i <= count)
				widget_add_child(widget, child);
		}
		else
		{
			if((0 == strcasecmp("text", child->classname)) ||
					(0 == strcasecmp("image", child->classname)) ||
					(0 == strcasecmp("progbar", child->classname)))
				widget_add_child(widget, child);
		}

		widget_style = widget_get_nextsibling(widget_style);
	}

	if(((x + widget->rect.w) > gui_core->config.width) ||
			(x < 0) ||
			((y + widget->rect.h) > gui_core->config.height) ||
			(y < 0))
	{
		return (NULL);
	}

	widget_create(widget);
	prompt = (GuiPrompt *)(widget->object);
	if(NULL == prompt) {
		return (NULL);
	}
	value = widget_get_property(widget, "locale_surface");
	if((prompt->locale_surface) ||
	   ((NULL == value) ||
	   ((NULL != value) && (0 == strcasecmp("true", value))))) {
		gdi_lock();
		_locale_surface(widget, (void *)"true");
		gdi_unlock();
	}
	if(focus)
	{
		old_widget = gui_core->config.focus_widget;
		old_widget->state &= ~WIDGET_STATE_FOCUSED;
		widget_set_update(old_widget, GUI_TRUE);

		gui_core->config.focus_widget = focus;
		focus->state |= WIDGET_STATE_FOCUSED;

		widget_set_update(focus, GUI_TRUE);
	}

	if((prompt->locale_surface) && (prompt->layer_name))
	{
		char *value = NULL;
		GxGDI_LayerSetPosition(prompt->layer_name, x, y);

		child = widget_get_firstchild(widget);
		if((child) &&
		   (0 == strcasecmp("text", child->classname))) {
			value = (char *)widget_get_property(child, "format");
			if((value) &&
			   (0 != strcasecmp("static", value)) &&
			   (0 != strcasecmp("automatic", value))) {
				widget->rect.x = x;
				widget->rect.y = y;
			}
	}
	}
	else
	{
		widget->rect.x = x;
		widget->rect.y = y;
	}

	if(prompt->create_event)
	{
		exec_widget_event_data(widget, prompt->create_event, NULL);
	}
	prompt->active_widget = focus;

	child = widget_get_firstchild(widget);
	while(child)
	{
		child->rect.x += widget->rect.x;
		child->rect.y += widget->rect.y;
		child = widget_get_nextsibling(child);
	}
	if(!ontop)
	{
		gdi_lock();
		widget_draw(widget);
		gdi_commit();
		wm_copy_main_surface();
		gdi_unlock();
	}
	if(count)
	{
		GxGuiViewServiceRecvMsgStop();
		while(1)
		{
			if(NULL == widget->object)
			{
				break;
			}

			/*child = widget_get_firstchild(widget);
			  while(child)
			  {
			  if(child->need_update)
			  {
			  child->rect.x += widget->rect.x;
			  child->rect.y += widget->rect.y;
			  widget_draw(child);
			  child->rect.x -= widget->rect.x;
			  child->rect.y -= widget->rect.y;
			  }
			  child = widget_get_nextsibling(child);
			  }
			  gdi_commit();*/

			ret = gui_poll_event(&event);
			if (GUI_NOEVENT != ret)
			{
				if(event.type == GUI_KEYDOWN)
					GUI_Printf("[KEY]scancode:0x%x\n", event.key.scancode);

				prompt_event(widget, (void *)(&event));//GUI_ExecEvent(&event);
				if(!ontop)
				{
					gdi_lock();
					widget_draw(widget);
					gdi_commit();
					wm_copy_main_surface();
					gdi_unlock();
				}
			}
			timer_exec();

			gui_delay(10);
		}
		GUI_StartSchedule();
	}
	else
	{
		widget_add_child(gui_core->config.root_widget, widget);
	}

	if(old_widget)
	{
		wm_state_set_focus(old_widget);
	}

	if(!ontop)
	{
		ret_string = (char *)widget_get_property(widget, "return");
	}

	if(count)
	{
		child = widget_get_firstchild(widget);
		while(child)
		{
			next = widget_get_nextsibling(child);
			widget_release(child);
			GUI_FREE(child);
			child = next;
		}

		i = 0;
		while(i < count)
		{
			GUI_FREE(button_string[i]);
			i++;
		}

		GUI_FREE(widget->classname);
		GUI_FREE(widget->name);
		GUI_FREE(widget->stylename);
		GUI_FREE(widget->xml_filename);
		GUI_FREE(widget);
	}

	return (ret_string);

}

status_t GUI_CheckPrompt(const char *name)
{
	GuiWidget *widget = NULL;

	widget = gui_get_window(name);
	if(NULL == widget)
	{
		return (GXCORE_ERROR);
	}

	if(0 != strcasecmp("prompt", widget->classname))
	{
		return (GXCORE_ERROR);
	}

	if(NULL != widget->object)
	{
		return (GXCORE_SUCCESS);
	}

	return (GXCORE_ERROR);
}

status_t GUI_PromptString(const char *name, const char *string)
{
	GuiWidget *widget = NULL, *child = NULL;
	GuiPrompt *prompt = NULL;

	if(NULL == name)
	{
		return (GXCORE_ERROR);
	}

	widget = gui_get_window(name);
	if(NULL == widget)
	{
		return (GXCORE_ERROR);
	}

	if(0 != strcasecmp("prompt", widget->classname))
	{
		return (GXCORE_ERROR);
	}

	if(NULL != widget->object)
	{
		child = widget_get_firstchild(widget);
		while(child)
		{
			if(0 == strcasecmp("text", child->classname))
			{
				GUI_SetProperty(child->name, "string", (char *)string);
				widget_set_update(child, GUI_TRUE);
				//child->rect.x += widget->rect.x;
				//child->rect.y += widget->rect.y;
				prompt = (GuiPrompt *)(widget->object);
				gdi_lock();
				if((prompt) && (prompt->locale_surface) && (prompt->layer_name)) {
					widget_draw(widget);
				} else {
					widget_draw(child);
				}
				gdi_unlock();
				//child->rect.x -= widget->rect.x;
				//child->rect.y -= widget->rect.y;
				break;
			}
			child = widget_get_nextsibling(child);
		}
	}

	return (GXCORE_SUCCESS);
}

status_t GUI_EndPrompt(const char *name)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL, *child = NULL, *next = NULL;
	int id = -1, count_win = 0;
	int count_ontop = 0, i = 0;
	GuiWidget *ontop_window = NULL, *win_unit = NULL;
	GuiPrompt *prompt = NULL;

	gui_core = GUI_GetCurrent();
	widget = gui_get_window(name);
	if(NULL == widget)
	{
		return (GXCORE_ERROR);
	}

	count_ontop = stack_count(gui.ontop_stack);
	for(i = 0; i < count_ontop; i++)
	{
		ontop_window = (GuiWidget *) stack_get(gui.ontop_stack, i);
		if(ontop_window != widget)
		{
			prompt = (GuiPrompt *)(ontop_window->object);
			if(NULL == prompt)
			{
				continue;
			}

			prompt->end_draw = GUI_TRUE;
			gdi_lock();
			widget_draw(ontop_window);
			if(prompt->surface &&
			   ((widget_have_intersect(widget, ontop_window))||
			    (widget_have_contain(widget, ontop_window))))
			{
				gal_free_surface(prompt->surface);
				prompt->surface = NULL;
			}
			prompt->end_draw = GUI_FALSE;
			gdi_commit();
			gdi_unlock();
		}
		else
			break;
	}
	id = stack_get_id(gui.ontop_stack, (void*)widget);
	if (-1 != id)
	{
		stack_del(gui_core->ontop_stack, id);
	}

	widget_del_child(gui_core->config.root_widget, name);
	widget_release(widget);

	child = widget_get_firstchild(widget);
	while(child)
	{
		next = widget_get_nextsibling(child);
		widget_release(child);
		GUI_FREE(child);
		child = next;
	}
	GUI_FREE(widget->classname);
	GUI_FREE(widget->name);
	GUI_FREE(widget->stylename);
	GUI_FREE(widget->xml_filename);
	GUI_FREE(widget);

	for(i = 0; i < count_ontop; i++)
	{
		ontop_window = (GuiWidget *) stack_get(gui.ontop_stack, i);
		if(ontop_window != widget)
		{
			gdi_lock();
			widget_draw(ontop_window);
			gdi_commit();
			gdi_unlock();
		}
	}

	gdi_lock();
	gdi_commit();
	gdi_unlock();

	if(gui.win_stack) {
		count_win = stack_count(gui.win_stack);
		for(i = count_win - 1; i >= 0; i--) {
			win_unit = (GuiWidget *)stack_get(gui.win_stack, i);
			if(win_unit) {
				if((win_unit->rect.w == gui_core->config.width) &&
				(win_unit->rect.h == gui_core->config.height)) {
					break;
				}
			}
		}
	}

	return (GXCORE_SUCCESS);
}


