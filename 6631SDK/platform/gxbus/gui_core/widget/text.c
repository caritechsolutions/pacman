#include "all_widget.h"
#include "hd_system.h"

static int _get_text_style(GuiWidget *self)
{
	int text_style = 0;
	const char *value = NULL;
	char style_name[30] = {0};

	if(NULL == self)
	{
		return (-1);
	}

	if(self->property)
	{
		value = (const char *)hash_get(self->property, "format");
	}

	if(NULL == value)
	{
		return (STATIC);
	}

	strcpy(style_name, value);

	if(0 == strcmp("static", style_name))
	{
		text_style = STATIC;
	}

	if(0 == strcmp("roll_r2l", style_name))
	{
		text_style = ROLL_R2L;
	}

	if(0 == strcmp("roll_l2r", style_name))
	{
		text_style = ROLL_L2R;
	}

	if(0 == strcmp("roll_b2u", style_name))
	{
		text_style = ROLL_B2U;
	}

	if(0 == strcmp("roll_u2b", style_name))
	{
		text_style = ROLL_U2B;
	}

	if(0 == strcmp("automatic", style_name))
	{
		text_style = AUTOMATIC;
	}

	if(0 == strcasecmp("fix_r2l", style_name))
	{
		text_style = FIX_ROLL_R2L;
	}

	if(0 == strcasecmp("step_r2l", style_name))
	{
		text_style = STEP_ROLL_R2L;
	}

	if(0 == strcasecmp("longer_r2l", style_name))
	{
		text_style = LONGER_ROLL_R2L;
	}

	return (text_style);
}

int _text_timer_event(void *userdata)
{
	GuiWidget *self = NULL;

	self = (GuiWidget *)userdata;

	widget_set_update(self, GUI_TRUE);

	return (0);
}

static int _config_text_parametre(GuiWidget *self)
{
	const char *value = NULL;
	GuiText *text = NULL;

	if(NULL == self)
	{
		return (1);
	}

	text = (GuiText *)(self->object);
	if(NULL == text)
	{
		return (1);
	}

	text->alignment = widget_get_alignment(self);

	value = (const char *)widget_get_property(self, "font");
	if((NULL != value) || (NULL == text->font))
	{
		text->font = (char *)value;
	}

	text->step = widget_get_int(self, "step", ROLLING_STEP);

	value = (const char *)widget_get_property(self, "roll_mode");
	if(NULL != value) {
		if(0 == strcasecmp("smooth", value)) {
			text->roll_mode = TEXT_ROLL_SMOOTH;
		} else {
			text->roll_mode = TEXT_ROLL_ANTIFLICKER;
		}
	} else {
		text->roll_mode = TEXT_ROLL_ANTIFLICKER;
	}

	return (0);
}

static void* create_text(GuiWidget *self)
{
	GuiText* text = NULL, *text_style = NULL;
	char *string = NULL;
	GuiWidget *parent = NULL;
	GuiWidget *style = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	if(self->object)
	{
		return (self->object);
	}

	text = (GuiText *)GUI_MALLOC(sizeof(GuiText));

	if(NULL == text)
	{
		return (NULL);
	}

	memset(text, 0, sizeof(GuiText));

	text->base = self;

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	style = (GuiWidget *)widget_get_style(self->stylename);

	if(style)
	{
		text_style = (GuiText *)GUI_MALLOC(sizeof(GuiText));

		if(NULL == text_style)
		{
			GUI_FREE(text);
			return (NULL);
		}

		memset(text_style, 0, sizeof(GuiText));
		style->object = (void *)text_style;

		_config_text_parametre(style);

		memcpy(text, text_style, sizeof(GuiText));
		text->base = self;

		GUI_FREE(style->object);
	}

	self->object = (void *)text;
	_config_text_parametre(self);

	text->text_mutex = -1;

	/*Unfocussable*/
	self->state = 0;

	string = (char *)widget_get_property(self, "string");
	if(string)
	{
		text->string = GUI_STRDUP(string);
	}
	text->format = _get_text_style(self);
	text->rolling_time = widget_get_int(self, "time", 0);
	if(text->rolling_time <= 0)
	{
		text->rolling_time = 10;
	}
	text->text_roll_exit = GUI_TRUE;

	if(text->text_mutex == -1)
	{
		GxCore_MutexCreate(&text->text_mutex);
	}

	if(self->signal)
	{
		text->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		text->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
	}

	if(text->create_event)
	{
		exec_widget_event_data(self, text->create_event, NULL);
	}

	return (text);
}

static int text_release(GuiWidget *self)
{
	GuiText *text = NULL;

	if(NULL == self)
	{
		return (1);
	}

	text = (GuiText*)self->object;
	if(NULL == text)
	{
		return (1);
	}

	if(TEXT_MOVE == text->roll_state)
	{
		text->roll_state = TEXT_STILL;
		while(GUI_TRUE != text->text_roll_exit)
		{
			gui_delay(10);
		}
	}

	GxCore_MutexLock(text->text_mutex);

	if(text->destroy_event)
	{
		exec_widget_event_data(self, text->destroy_event, NULL);
	}

	if(ROLL_R2L == text->format ||
			ROLL_L2R == text->format ||
			ROLL_B2U == text->format ||
			ROLL_U2B == text->format ||
			FIX_ROLL_R2L == text->format ||
			STEP_ROLL_R2L == text->format ||
			LONGER_ROLL_R2L == text->format)
	{
		if(text->timer)
		{
			remove_timer(text->timer);
		}
		text->roll_state = TEXT_STILL;
	}

	if(text->surface)
	{
		gal_free_surface(text->surface);
	}

	if(text->surface_roll_text)
	{
		gal_free_surface(text->surface_roll_text);
	}

	if(text->rolling_stop_timer) {
		remove_timer(text->rolling_stop_timer);
		text->rolling_stop_timer = NULL;
	}

	GxCore_MutexUnlock(text->text_mutex);
	GxCore_MutexDelete(text->text_mutex);
	text->text_mutex = 0;

	GUI_FREE(text->string);
	GUI_FREE((self->object));

	return (0);
}

static void _relay_roll(GuiText *text)
{
	GuiWidget *self = NULL;
	GAL_Rect surface_rect = {0}, src_rect = {0}, dst_rect = {0};
	GAL_Interval interval = {0};
	int color = 0, index = 0, ret = 0, valid_len = 0;
	char *string = NULL, *tmp_str = NULL;

	self = text->base;
	surface_rect.w = text->surface_roll_text->sf.width;
	surface_rect.h = text->surface_roll_text->sf.height;

	if(text->roll_offset)
	{
		src_rect.x = surface_rect.w - self->rect.w;
		src_rect.y = 0;
		src_rect.w = self->rect.w;
		src_rect.h = surface_rect.h;

		dst_rect = src_rect;
		dst_rect.x = 0;
		hd_copy_surface(text->surface_roll_text, &src_rect, text->surface_roll_text, &dst_rect);

		surface_rect.x = src_rect.w;
		surface_rect.w -= src_rect.w;
	}

	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(screen, color);
	gdi_begin();
	hd_fillrect(text->surface_roll_text, (GAL_Rect *)&surface_rect, color);
	gdi_end();

	string = text->string + text->roll_offset;

	if(text->roll_offset)
	{
		tmp_str = GUI_MALLOCZ(text->roll_offset + 10);
		memcpy(tmp_str, text->string + text->prev_offset, text->roll_offset - text->prev_offset);
		tmp_str[text->roll_offset] = '\0';
		valid_len = gdi_text_surfacewidth(tmp_str) + text->prev_pos;
		valid_len = text->surface_roll_text->sf.width - valid_len;
		if(valid_len > 0)
		{
			surface_rect.x -= valid_len;
			surface_rect.w = ROLL_MAX_LEN - surface_rect.x;
			text->prev_pos = surface_rect.x;
		}
		GUI_FREE(tmp_str);
	}
	else
	{
		valid_len = gdi_text_surfacewidth(text->string);
	}

	text->alignment = widget_alignment_revert(self, text->string, text->alignment);
	color = self->fore_color[index];
	color = gal_color2index(screen, color);
	if(GUI_TA_VCENTRE & text->alignment)
	{
		surface_rect.y += (surface_rect.h - gdi_get_charheight()) / 2;
		surface_rect.h -= (surface_rect.h - gdi_get_charheight()) / 2;
	}
	else if(GUI_TA_BOTTOM & text->alignment)
	{
		surface_rect.y += (surface_rect.h - gdi_get_charheight());
		surface_rect.h -= (surface_rect.h - gdi_get_charheight());
	}

	if((*string == '\n') || (*string == '\0')) {
		text->prev_offset = 0;
		text->roll_offset = 0;
		text->prev_pos = 0;
		text->pos = 0;
		return;
	}

	gdi_begin();
	ret = gxgdi_draw_string(text->surface_roll_text,
			string,
			(GAL_Rect*)(&surface_rect),
			&interval,
			text->alignment,
			color,
			SINGLELINE_MODE);
	if(ret != strlen(string))
	{
		text->prev_offset = text->roll_offset;
		text->roll_offset += (ret - 1);
		text->pos = self->rect.w;//surface_rect.x;
	}
	else
	{
		text->format = FIX_ROLL_R2L;
		text->prev_offset = 0;
		text->roll_offset = 0;
		text->prev_pos = 0;
		text->pos = 0;
	}

	gdi_end();
}

static void _check_force_stop_roll(GuiWidget *self)
{
	char *value = NULL, *focus_window = NULL;
	GuiWidget *window = NULL;
	GuiText *text = NULL;

	value = (char *)widget_get_property(self, "rolling_run");
	if(NULL == value)
	{
		return;
	}

	if(0 == strcasecmp("false", value))
	{
		text = (GuiText *)(self->object);
		if(NULL == text)
		{
			return;
		}

		focus_window = (char *)GUI_GetFocusWindow();
		if(focus_window)
		{
			window = widget_get_window(self);
			if(window)
			{
				if(0 != strcasecmp(window->name, focus_window))
				{
					text->roll_state = TEXT_STILL;
				}
			}
		}
	}
}

static void dynamic_text_roll(void *data)
{
	char *string = NULL;
	int text_len = 0,index =0,color = 0, valid_len = 0;
	int cost_ms_time = 0, wait_time = 0, inited = 0;
	uint32_t event = 0;
	int fix_offset = 0;
	GuiCore *gui_core = NULL;
	GuiWidget *self = NULL;
	GuiText* text = NULL;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	GUI_Rect valid_rect = {0}, surface_rect = {0}, fill_rect = {0};
	GxTime start = {0}, stop = {0};


	self = (GuiWidget*)data;
	if(NULL == self)
	{
		return ;
	}

	text = (GuiText*)(self->object);
	if(NULL == text)
	{
		return ;
	}

	gui_core = &gui;

	GxCore_MutexLock(text->text_mutex);
	text->text_roll_exit = GUI_FALSE;

	GxCore_ThreadDetach();

	gdi_lock();
	old_font = gxgdi_set_font((char *)text->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		gxgdi_set_font((char *)old_font);
		gdi_unlock();
		text->text_roll_exit = GUI_TRUE;
		GxCore_MutexUnlock(text->text_mutex);
		return;
	}

	string = (char *)i18n_get_string((const char *)(text->string));
	text_len = gdi_text_surfacewidth(string) + new_font->xsize;

	if(text_len >= ROLL_MAX_LEN)//ROLLING_RECTANGLE)
	{
		valid_len = ROLL_MAX_LEN;//ROLLING_RECTANGLE;
	}
	else if(text->surface_roll_text)
	{
		valid_len = text->surface_roll_text->sf.width;
	}

	text_len = gdi_text_len(string);
	gxgdi_set_font((char *)old_font);
	gdi_unlock();

	if((0 == text->text_offset) && (text->text_tail))
	{
		gdi_text_surfacewidth(string + text->text_tail);
	}

	valid_rect = self->rect;
	text->pos = text->pos_temp;
	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(screen, color);


	if((((ROLL_R2L == text->format) ||
					(FIX_ROLL_R2L == text->format) ||
					(STEP_ROLL_R2L == text->format) ||
					(LONGER_ROLL_R2L == text->format))
				&& (text_len > valid_rect.w)) ||
			(ROLL_L2R == text->format))
	{
		while(1)
		{
			if(self->state & WIDGET_STATE_HIDE)
			{
				text->roll_state = TEXT_STILL;
			}
			if((TEXT_STILL == text->roll_state) ||
					(NULL == text->surface_roll_text))
			{
				goto exit;
			}

			if(FIX_ROLL_R2L == text->format)
			{
				fix_offset = self->rect.w;
			}
			else
			{
				fix_offset = 0;
			}

			color = self->back_color[index];
			color = gal_color2index(screen, color);
			fill_rect = self->rect;
			if(inited)
				hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
			inited = GUI_TRUE;
			while(text->pos <= (valid_len + self->rect.w))
			{
				_check_force_stop_roll(self);
				if((FIX_ROLL_R2L == text->format) &&
						(text->relay) &&
						((text->pos + fix_offset) >= (valid_len)))
				{
					break;
				}
				if(self->state & WIDGET_STATE_HIDE)
				{
					text->roll_state = TEXT_STILL;
				}
				if((TEXT_STILL == text->roll_state) ||
						(NULL == text->surface_roll_text))
				{
					goto exit;
				}

				if(text->pos < self->rect.w)
				{
					surface_rect.y = 0;
					if((FIX_ROLL_R2L == text->format) || (STEP_ROLL_R2L == text->format))
					{
						surface_rect.x = text->pos;
						surface_rect.w = self->rect.w;
					}
					else if(ROLL_L2R == text->format)
					{
						surface_rect.x = text->surface_roll_text->sf.width - text->pos;
						surface_rect.w = text->pos;
						if(0 > surface_rect.x)
						{
							surface_rect.w += surface_rect.x;
							surface_rect.x = 0;
						}
					}
					else
					{
						surface_rect.x = 0;
						surface_rect.w = text->pos;
					}
					surface_rect.h = self->rect.h;


					if((FIX_ROLL_R2L == text->format) || (STEP_ROLL_R2L == text->format))
					{
						surface_rect.w = self->rect.w;
#if 0
						if(FIX_ROLL_R2L == text->format)
						{
							if((surface_rect.x + surface_rect.w) > text_len)
							{
								surface_rect.w = text_len - surface_rect.x;
								surface_rect.x = text->pos = 0;
								valid_rect.x = self->rect.x;
								valid_rect.w = self->rect.w;
							}
						}
						else
						{
#endif
							if((surface_rect.x) > text_len)
							{
								surface_rect.w = text_len - surface_rect.x;
								text->pos = 0;
							}
							else if((surface_rect.x + surface_rect.w) > text_len)
							{
								surface_rect.w = text->surface_roll_text->sf.width - surface_rect.x;
								valid_rect.x = self->rect.x;
								valid_rect.w = self->rect.w;
								//text->pos = 0;
							}
							//}
					}
					else if(ROLL_L2R == text->format)
					{
						if(surface_rect.x == 0)
						{
							valid_rect = self->rect;
							valid_rect.x -= (text->surface_roll_text->sf.width - text->pos);
							valid_rect.w += (text->surface_roll_text->sf.width - text->pos);
							color = self->back_color[index];
							color = gal_color2index(screen, color);
							fill_rect = self->rect;
							fill_rect.w = valid_rect.x - fill_rect.x;
							hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
						}
						else
						{
							valid_rect = self->rect;
						}
					}
					else
					{
						valid_rect.x = valid_rect.x + valid_rect.w - text->pos;
						valid_rect.w = text->pos;
					}
				}
				else
				{
					if((FIX_ROLL_R2L == text->format) || (STEP_ROLL_R2L == text->format))
					{
						surface_rect.x = text->pos;
					}
					else if(ROLL_L2R == text->format)
					{
						surface_rect.x = text->surface_roll_text->sf.width - text->pos;
						surface_rect.w = self->rect.w;
						if(surface_rect.x < 0)
						{
							surface_rect.x = 0;
							surface_rect.w += text->surface_roll_text->sf.width - text->pos;
						}
					}
					else
					{
						surface_rect.x = text->pos - self->rect.w;
					}

					surface_rect.y = 0;
					if((FIX_ROLL_R2L == text->format) || (STEP_ROLL_R2L == text->format))
					{
						if((valid_len - text->pos) < 0)
						{
							text->pos = 0;
							break;
						}
						else if((valid_len - text->pos) < self->rect.w)
						{
							surface_rect.w = valid_len - text->pos;
						}
						else
						{
							surface_rect.w = self->rect.w;
						}
					}
					else
					{
						if((valid_len - text->pos) < 0)
						{
							if(text->relay)
								surface_rect.w = self->rect.w;
							else
								surface_rect.w = valid_len - (text->pos - self->rect.w);
						}
						else
						{
							surface_rect.w = self->rect.w;
						}
					}
					surface_rect.h = self->rect.h;

					if((ROLL_L2R == text->format) && (surface_rect.x == 0))
					{
						valid_rect = self->rect;
						valid_rect.x -= (text->surface_roll_text->sf.width - text->pos);
						valid_rect.w += (text->surface_roll_text->sf.width - text->pos);
						color = self->back_color[index];
						color = gal_color2index(screen, color);
						fill_rect = self->rect;
						fill_rect.w = valid_rect.x - fill_rect.x;
						hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
					}
					else
					{
						valid_rect.x = self->rect.x;
						valid_rect.w = self->rect.w;
					}
				}

				color = self->back_color[index];
				color = gal_color2index(screen, color);
				fill_rect = self->rect;

				wait_time = text->rolling_time;
				cost_ms_time = ((stop.seconds * 1000000 + stop.microsecs) -
						(start.seconds * 1000000 + start.microsecs)) / 1000;
				wait_time -= cost_ms_time;
				int start_time = 0, end_time = 0;
				if(TEXT_ROLL_ANTIFLICKER == text->roll_mode) {
					while(wait_time > 0)
					{
						start_time = hd_get_tick();
						GxAVWaitEvents(g_device_handle, vdec_handle, EVENT_VPU_INTERRUPT, 200 * 1000, &event);
						end_time = hd_get_tick();
						wait_time -= (end_time - start_time);
						cost_ms_time = 0;
					}
				} else {
					if(wait_time > 0) {
						gui_delay(wait_time);
					} else {
						gui_delay(10);
					}
				}

				gdi_lock();
				if(self->state & WIDGET_STATE_HIDE)
				{
					gdi_update();
					if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
						gui.layer_need_update = GUI_TRUE;
					} else {
						wm_copy_main_surface();
					}
					text->pos_temp = text->pos;
					text->roll_state = TEXT_STILL;
					text->text_roll_exit = GUI_TRUE;
					gdi_unlock();
					GxCore_MutexUnlock(text->text_mutex);

					return;
				}

				GxCore_GetLocalTime(&start);
				if((self->back_color[index] == gui_core->config.gui_trans) &&
						(text->surface))
				{
					GAL_Rect trans_rect = {0};
					trans_rect.w = self->rect.w;
					trans_rect.h = self->rect.h;
					trans_rect.x = trans_rect.y = 0;

					gal_copy_surface(text->surface,
							(GAL_Rect*)&trans_rect,
							screen,
							(GAL_Rect*)(&(self->rect)));
				}
				else if((widget_get_property(self, "fill")) &&
						(0 == strcasecmp("enable", widget_get_property(self, "fill"))))
				{
					hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
				}

				if(surface_rect.w > valid_rect.w) {
					surface_rect.w = valid_rect.w;
				}
				hd_stretch_surface(text->surface_roll_text,
						(GAL_Rect*)&(surface_rect),
						screen,
						(GAL_Rect*)(&valid_rect));

				if(TEXT_ROLL_ANTIFLICKER == text->roll_mode) {
					gdi_update();
					if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
						gui.layer_need_update = GUI_TRUE;
					} else {
						wm_copy_main_surface();
					}
				} else {
					hd_stretch_surface(text->surface_roll_text,
							   (GAL_Rect*)&(surface_rect),
							   back_screen,
							   (GAL_Rect*)(&valid_rect));
					gdi_commit();
				}
				GxCore_GetLocalTime(&stop);
				gdi_unlock();
				if((FIX_ROLL_R2L == text->format) && (text->pos == 0))
				{
					int count = 0;
					while(count < 7)
					{
						if((TEXT_STILL == text->roll_state) ||
								(NULL == text->surface_roll_text))
						{
							text->pos_temp = text->pos;
							text->roll_state = TEXT_STILL;
							text->text_roll_exit = GUI_TRUE;
							GxCore_MutexUnlock(text->text_mutex);
							return;
						}
						gui_delay(200);
						count++;
					}
				}

				text->pos += text->step;
				if((text->roll_offset) && (text->pos >= ROLL_MAX_LEN))
				{
					break;
				}

				if((0 == (text->pos % ROLLING_RECTANGLE)) &&
						(text_len >= ROLLING_RECTANGLE))
				{
					gdi_lock();
					gdi_update();
					if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
						gui.layer_need_update = GUI_TRUE;
					} else {
						wm_copy_main_surface();
					}
					text->pos_temp = text->pos = self->rect.x + self->rect.w;
					hd_free_surface(text->surface_roll_text);
					text->surface_roll_text = NULL;
					text->text_length += ROLLING_RECTANGLE;
					text->roll_state = TEXT_STILL;
					text->text_roll_exit = GUI_TRUE;
					gdi_unlock();
					widget_set_update(self, GUI_TRUE);
					GxCore_MutexUnlock(text->text_mutex);

					return;
				}
				else if((text->text_tail) && (text_len >= ROLLING_RECTANGLE))
				{
					if(surface_rect.x >=
							(gdi_text_surfacewidth(string + text->text_tail) +
							 self->rect.x +
							 self->rect.w))
					{
						gdi_lock();
						gdi_update();
						if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
							gui.layer_need_update = GUI_TRUE;
						} else {
							wm_copy_main_surface();
						}
						hd_free_surface(text->surface_roll_text);
						text->surface_roll_text = NULL;
						text->roll_state = TEXT_STILL;
						gdi_unlock();
						text->times++;

						text->text_roll_exit= GUI_TRUE;
						widget_set_update(self, GUI_TRUE);
						GxCore_MutexUnlock(text->text_mutex);

						return;
					}
				}

#if 0
				cost_ms_time = ((stop.seconds * 1000000 + stop.microsecs) -
						(start.seconds * 1000000 + start.microsecs)) / 1000;
				if((text->rolling_time - cost_ms_time)<10)
				{
					gui_delay(10);
				}
				else
				{
					gui_delay(text->rolling_time - cost_ms_time);
				}
#endif

			}

			if(text->roll_offset)
			{
				valid_len = gdi_text_surfacewidth(text->string + text->roll_offset) + self->rect.w;
				_relay_roll(text);
				if(valid_len > ROLL_MAX_LEN)
				{
					valid_len = ROLL_MAX_LEN;
				}
			}
			else
			{
				if(text->relay)
				{
					_relay_roll(text);
					text->format = _get_text_style(self);
					valid_len = gdi_text_surfacewidth(text->string);
					if(valid_len > ROLL_MAX_LEN)
					{
						valid_len = ROLL_MAX_LEN;
					}
				}
				text->pos = 0;
				text->times++;
			}
		}
	}
	else if((ROLL_R2L == text->format) && (text_len <= valid_rect.w))
	{
		while(1)
		{
			if(self->state & WIDGET_STATE_HIDE)
			{
				text->roll_state = TEXT_STILL;
			}
			if((TEXT_STILL == text->roll_state) ||
					(NULL == text->surface_roll_text))
			{
				goto exit;
			}
			if(0 == text->pos_temp)
			{
				text->pos = 0;
			}
			while(text->pos <= (text_len + self->rect.w))
			{
				if(self->state & WIDGET_STATE_HIDE)
				{
					text->roll_state = TEXT_STILL;
				}
				if((TEXT_STILL == text->roll_state) ||
						(NULL == text->surface_roll_text))
				{
					goto exit;
				}

				if(text->pos < self->rect.w)
				{
					surface_rect.x = 0;
					surface_rect.y = 0;
					surface_rect.h = self->rect.h;
					if(text->pos < text_len)
					{
						surface_rect.w = text->pos;
					}
					else
					{
						surface_rect.w = text_len;
					}

					valid_rect.x = self->rect.x + self->rect.w - text->pos;
					if(text->pos < text_len)
					{
						valid_rect.w = text->pos;
					}
					else
					{
						valid_rect.w = text_len;
					}

				}
				else
				{
					surface_rect.x = text->pos - self->rect.w;
					surface_rect.y = 0;
					surface_rect.w = text_len - (text->pos - self->rect.w);
					surface_rect.h = self->rect.h;

					valid_rect.x = self->rect.x;
					valid_rect.w = self->rect.w;
				}

				color = self->back_color[index];
				color = gal_color2index(screen, color);
				fill_rect = self->rect;
				wait_time = text->rolling_time;
				cost_ms_time = ((stop.seconds * 1000000 + stop.microsecs) -
						(start.seconds * 1000000 + start.microsecs)) / 1000;
				wait_time -= cost_ms_time;
				int start_time = 0, end_time = 0;
				if(TEXT_ROLL_ANTIFLICKER == text->roll_mode) {
					while(wait_time > 0)
					{
						start_time = hd_get_tick();
						GxAVWaitEvents(g_device_handle, vdec_handle, EVENT_VPU_INTERRUPT, 200 * 1000, &event);
						end_time = hd_get_tick();
						wait_time -= (end_time - start_time);
						cost_ms_time = 0;
					}
				} else {
					if(wait_time > 0) {
						gui_delay(wait_time);
					} else {
						gui_delay(10);
					}
				}

				gdi_lock();
				GxCore_GetLocalTime(&start);
				if((self->back_color[index] == gui_core->config.gui_trans) &&
						(text->surface))
				{
					GAL_Rect trans_rect = {0};
					trans_rect.w = self->rect.w;
					trans_rect.h = self->rect.h;
					trans_rect.x = trans_rect.y = 0;

					gal_copy_surface(text->surface,
							(GAL_Rect*)&trans_rect,
							screen,
							(GAL_Rect*)(&(self->rect)));
				}
				else if((widget_get_property(self, "fill")) &&
						(0 == strcasecmp("enable", widget_get_property(self, "fill"))))
				{
					hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
				} else {
					if(((surface_rect.x + surface_rect.w + text->step) <= text->surface_roll_text->sf.width) &&
					   ((valid_rect.x + valid_rect.w + text->step) <= screen->sf.width)) {
						surface_rect.w += text->step;
						valid_rect.w += text->step;
					}
				}

				hd_stretch_surface(text->surface_roll_text,
						(GAL_Rect*)&(surface_rect),
						screen,
						(GAL_Rect*)(&valid_rect));
				if(TEXT_ROLL_ANTIFLICKER == text->roll_mode) {
					gdi_update();
					if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
						gui.layer_need_update = GUI_TRUE;
					} else {
						wm_copy_main_surface();
					}
				} else {
					hd_stretch_surface(text->surface_roll_text,
							   (GAL_Rect*)&(surface_rect),
							   back_screen,
							   (GAL_Rect*)(&valid_rect));
					gdi_commit();
				}

				GxCore_GetLocalTime(&stop);
				gdi_unlock();

#if 0
				cost_ms_time = ((stop.seconds * 1000000 + stop.microsecs) -
						(start.seconds * 1000000 + start.microsecs)) / 1000;
				if((text->rolling_time - cost_ms_time)<10)
				{
					gui_delay(10);
				}
				else
				{
					gui_delay(text->rolling_time - cost_ms_time);
				}
#endif
				text->pos += text->step;
			}
			text->pos = 0;
			text->times++;
			if((widget_get_property(self, "fill")) &&
					(0 == strcasecmp("enable", widget_get_property(self, "fill"))))
			{
				hd_fillrect(screen, (GAL_Rect *)&fill_rect, color);
			}
		}
	}

	text->text_roll_exit = GUI_TRUE;

	text->roll_state = TEXT_STILL;
	GxCore_MutexUnlock(text->text_mutex);

	return;

exit:
	gdi_lock();
	gdi_update();
	if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode) {
		gui.layer_need_update = GUI_TRUE;
	} else {
		wm_copy_main_surface();
	}
	text->pos_temp = text->pos;
	text->roll_state = TEXT_STILL;
	text->text_roll_exit = GUI_TRUE;
	gdi_unlock();
	GxCore_MutexUnlock(text->text_mutex);

	return;
}

static int _text_timer_start_roll(void *userdata)
{
	GuiWidget *self = NULL;
	GuiText *text = NULL;
	int thread_id = 0;

	if(NULL == userdata)
		return (-1);

	self = (GuiWidget *)userdata;
	text = (GuiText *)(self->object);
	if(NULL == text)
		return (-1);

	text->roll_state = TEXT_MOVE;
	text->text_roll_exit = GUI_FALSE;
	text->roll_timer = NULL;
	wm_copy_main_surface();
	GxCore_ThreadCreate(self->name,
		            &thread_id,
			    dynamic_text_roll,
			    (void *)self,
			    16 * 1024,
			    GXOS_DEFAULT_PRIORITY - 2);

	return (0);
}

static int text_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiText* text = NULL;
	GuiWidget *parent = NULL;
	char *string = NULL, *arabic_string = NULL;
	const char* value = NULL;
	GuiSurface *dst_surface = NULL;
	GUI_Rect valid_rect = {0}, surface_rect = {0}, fill_rect = {0}, string_rect = {0};
	GAL_Interval interval = {0};
	struct gdi_font* old_font = NULL, *new_font = NULL;
	int text_len = 0, line_num = 0, ret = 0, alignment = 0;
	int color = 0, index = 0, flag = 0, auto_width = 0;

	if(NULL == self)
	{
		return (1);
	}

	text = (GuiText*)(self->object);
	if(NULL == text)
	{
		return (1);
	}

	if((TEXT_MOVE == text->roll_state) &&
	   (widget_get_update(self) == 0) &&
	   (0 == strcasecmp("prompt", self->parent->classname))) {
		return (0);
	}

	gui_core = GUI_GetCurrent();
	parent = widget_get_parent(self);

	dst_surface = widget_get_current_surface(screen);

	if((FIX_ROLL_R2L == text->format) ||
			(STEP_ROLL_R2L == text->format) ||
			(ROLL_R2L == text->format) ||
			(ROLL_L2R == text->format) ||
			(LONGER_ROLL_R2L == text->format))
	{
		if(TEXT_MOVE == text->roll_state)
		{
			index = widget_get_state_index(self);
			color = self->back_color[index];
			color = gal_color2index(dst_surface, color);
			if(0 != strcasecmp("prompt", self->parent->classname))
				gal_fillrect(dst_surface, (GAL_Rect *)&(self->rect), color);
			return (0);
		}
	}

	if(check_rect_not_zero(&(self->string_rect)))
	{
		valid_rect = self->string_rect;
	}
	else
	{
		valid_rect = self->rect;
	}
	text->font = (char*)widget_get_property(self, "font");
	text->alignment = widget_get_alignment(self);

	old_font = gxgdi_set_font((char *)text->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	value = widget_get_property(self, "auto_width");
	if((value) && (0 == strcasecmp("true", value)))
	{
		auto_width = GUI_TRUE;
	}

	value = widget_get_property(self, "auto_height");
	if((value) && (0 == strcasecmp("true", value)))
	{
		//		auto_height = GUI_TRUE;
	}

	fill_rect = self->rect;

	interval.horizontal = widget_get_int(self, "inter_char", 0);
	interval.vertical = widget_get_int(self, "inter_line", 0);

	string = (char *)i18n_get_string((const char *)(text->string));
	text->alignment = widget_alignment_revert(self, string, text->alignment);

	text_len = gdi_text_len(string);

	if(AUTOMATIC == text->format)
	{
		line_num = gdi_text_get_lines((void *)string,
				(GAL_Rect *)(&valid_rect),
				&interval);
	} else {
		if(check_rect_not_zero(&(self->string_rect)))
		{
			string_rect = self->string_rect;
		}
		else
		{
			string_rect = self->rect;
		}
		widget_string_alignment(text->alignment,
				text_len,
				gdi_get_charheight(),
				&(string_rect),
				&(valid_rect));
	}

	if(auto_width)
	{
		fill_rect = valid_rect;
	}

	index = widget_get_state_index(self);
	color = self->back_color[index];
	color = gal_color2index(dst_surface, color);
	if(color && (8 < gui_core->config.bpp))
	{
		if(gal_color2index(dst_surface, gui_core->config.gui_trans) == color)
		{
			surface_rect = self->rect;
			surface_rect.x = surface_rect.y = 0;

			//gal_free_surface(text->surface);
			//text->surface = NULL;

			if(NULL == text->surface)
			{
				gdi_commit();

				text->surface = gal_get_surface((GAL_Rect*)&surface_rect,
						gui_core->config.bpp);

				gal_copy_surface(dst_surface,
						(GAL_Rect*)&(self->rect),
						text->surface,
						(GAL_Rect*)(&surface_rect));
			}
		}
		else
		{
			if((TEXT_MOVE == text->roll_state) &&
			   (0 == strcasecmp("prompt", self->parent->classname))) {
				;
			} else {
				gal_fillrect(dst_surface, (GAL_Rect *)&fill_rect, color);
			}
		}
	}
	else
	{
			if((TEXT_MOVE == text->roll_state) &&
			   (0 == strcasecmp("prompt", self->parent->classname))) {
				;
			} else {
				gal_fillrect(dst_surface, (GAL_Rect *)&fill_rect, color);
			}
	}

	index = widget_get_state_index(self);
	color = self->fore_color[index];
	color = gal_color2index(dst_surface, color);

	if(ROLL_B2U == text->format)
	{
		if(line_num)
		{
			flag = PARAGRAPH_MODE;
		}
		line_num = line_num * gdi_get_charheight();
	}

	flag = (AUTOMATIC & text->format) ? (PARAGRAPH_MODE) : (SINGLELINE_MODE);

	if(NULL == text->surface_roll_text)
	{
		if((ROLL_R2L == text->format) ||
				(ROLL_B2U == text->format) ||
				(ROLL_B2U == text->format) ||
				(ROLL_U2B == text->format) ||
				(STEP_ROLL_R2L == text->format) ||
				(FIX_ROLL_R2L == text->format) ||
				((LONGER_ROLL_R2L == text->format) && (text_len > valid_rect.w)) ||
				(ROLL_L2R == text->format))
		{
			surface_rect.x = surface_rect.y = 0;
			surface_rect.h = self->rect.h;
			surface_rect.w = (((((text_len + new_font->xsize) * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);

			if(surface_rect.w > ROLL_MAX_LEN)
			{
				surface_rect.w = ROLL_MAX_LEN;
				text->relay = GUI_TRUE;
			}
			text->surface_roll_text = gal_get_surface((GAL_Rect*)&surface_rect,gui_core->config.bpp);

			if(surface_rect.w == ROLL_MAX_LEN)
			{
				;
			}
			else
			{
				surface_rect.w = (((((text_len + new_font->xsize) * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
			}

			color = self->back_color[index];
			color = gal_color2index(dst_surface, color);
			gdi_begin();
			hd_fillrect(text->surface_roll_text, (GAL_Rect *)&surface_rect, color);
			gdi_end();

			text->alignment = widget_alignment_revert(self, string, text->alignment);

			color = self->fore_color[index];
			color = gal_color2index(dst_surface, color);

			if(GUI_TA_VCENTRE == (GUI_TA_VERTICAL & text->alignment))
			{
				surface_rect.y += (surface_rect.h - gdi_get_charheight()) / 2;
				surface_rect.h -= (surface_rect.h - gdi_get_charheight()) / 2;
			}
			else if(GUI_TA_BOTTOM == (GUI_TA_VERTICAL & text->alignment))
			{
				surface_rect.y += (surface_rect.h - gdi_get_charheight());
				surface_rect.h -= (surface_rect.h - gdi_get_charheight());
			}

			gdi_begin();
			ret = gxgdi_draw_string(text->surface_roll_text,
					string,
					(GAL_Rect*)(&surface_rect),
					&interval,
					text->alignment,
					color,
					flag);
			gdi_end();
			if((string) && (ret < strlen(string)) && (ret > 1))
				text->roll_offset = ret - 1;
			else
				text->roll_offset = 0;
			widget_set_update(self, GUI_FALSE);
		}
	}

#if 0
	if(((ROLL_R2L == text->format) ||
				(ROLL_L2R == text->format) ||
				(((FIX_ROLL_R2L == text->format) ||
				  (STEP_ROLL_R2L == text->format) ||
				  (LONGER_ROLL_R2L == text->format)) && (text_len > self->rect.w)))&&
			(text->roll_state == TEXT_STILL))
	{
		gxgdi_set_font((char *)old_font);
		text->roll_state = TEXT_MOVE;
		text->text_roll_exit = GUI_FALSE;
		wm_copy_main_surface();
		GxCore_ThreadCreate(self->name,
				&thrad_id,
				dynamic_text_roll,
				(void *)self,
				16 * 1024,
				GXOS_DEFAULT_PRIORITY - 2);

	}
#endif
	if((ROLL_B2U == text->format) && (line_num > valid_rect.h))
	{
		if(NULL == text->timer)
		{
			text->timer = create_timer(_text_timer_event,
					text->rolling_time,
					self,
					TIMER_REPEAT);
		}
		gdi_roll_string(dst_surface,
				string,
				(GAL_Rect *)(&valid_rect),
				text->pos,
				color,
				PARAGRAPH_MODE);

		text->pos += text->step;
		if(text->pos >= line_num)
		{
			text->pos = 0;
		}
		gxgdi_set_font((char *)old_font);
	}
	else
	{
		if((gal_color2index(dst_surface, gui_core->config.gui_trans) ==
					gal_color2index(dst_surface, self->back_color[index])) &&
				(text->update))
		{
			wm_redraw_prior_widget(self);
		}
		if(PARAGRAPH_MODE == flag)
		{
			line_num = ((line_num == 0) ? 1 : line_num);

			line_num = gdi_get_charheight() * line_num;
			if(line_num < valid_rect.h)
			{
				alignment = (text->alignment) & GUI_TA_VERTICAL;
				if(GUI_TA_VCENTRE == alignment)
				{
					valid_rect.y = valid_rect.y + (valid_rect.h - line_num) / 2;
					valid_rect.h = valid_rect.h - (valid_rect.h - line_num) / 2;
				}
				else if(GUI_TA_BOTTOM == alignment)
				{
					valid_rect.y = valid_rect.y + (valid_rect.h - line_num);
					valid_rect.h = valid_rect.h - (valid_rect.h - line_num);
				}
			}
		}

		if((text->surface) &&
				(0 != strcasecmp("boxitem", parent->classname))  &&
				(0 != strcasecmp("header", parent->classname)) &&
				(0 != strcasecmp("prompt", parent->classname)) &&
				(!(self->state & WIDGET_STATE_HIDE)))
		{
			surface_rect = self->rect;
			surface_rect.x = surface_rect.y = 0;

			gal_copy_surface(text->surface,
					(GAL_Rect*)&surface_rect,
					dst_surface,
					(GAL_Rect*)(&(self->rect)));

		}

		if(gxgdi_is_arabic((const unsigned char *)string) &&
		  (GB_FAMILY != new_font->family) &&
		  (AUTOMATIC == text->format))
		{
			line_num = gdi_text_get_lines((void *)string,
					(GAL_Rect *)(&valid_rect),
					&interval);
			arabic_string = GUI_MALLOCZ(strlen(string) + line_num + 100);
			gdi_arabic_multilines(string, arabic_string, valid_rect.w);
			string = arabic_string;
		}

		if((STATIC == text->format) ||
		   (AUTOMATIC == text->format) ||
		   (FIX_ROLL_R2L == text->format) ||
		   (STEP_ROLL_R2L == text->format) ||
		   (LONGER_ROLL_R2L == text->format)) {
			gxgdi_draw_string(dst_surface,
					string,
					(GAL_Rect*)(&valid_rect),
					&interval,
					text->alignment,
					color,
					flag);
		}
		if(arabic_string)
		{
			GUI_FREE(arabic_string);
		}
		if(text->timer)
		{
			remove_timer(text->timer);
			text->timer = NULL;
		}
		if(((ROLL_R2L == text->format) ||
		   (ROLL_L2R == text->format) ||
		   (((FIX_ROLL_R2L == text->format) ||
		   (STEP_ROLL_R2L == text->format) ||
		   (LONGER_ROLL_R2L == text->format)) && (text_len > self->rect.w)))&&
		   (text->roll_state == TEXT_STILL)) {
			if(NULL == text->roll_timer)
			{
				text->roll_timer = create_timer(_text_timer_start_roll,
								80,
								self,
								TIMER_ONCE);
			}
		}
		gxgdi_set_font((char *)old_font);
	}
	return (0);
}

static int text_event(GuiWidget *self, void *data)
{
	return (0);
}


static int _text_get_first_byte(unsigned char* pstr)
{
	unsigned char *str = NULL;

	if(NULL == pstr)
	{
		return (0);
	}

	str = pstr;

	if((*str > 0) && (*str <= 0x5))
	{
		return (1);
	}
	else if(0x10 == *str)
	{
		return (3);
	}
	else if(0x11 == *str)
	{
		return (1);
	}
	else
	{
		return (0);
	}
}

static int _copy_iso10646_string(GuiText *text, char *pstr)
{
	int len = 0;
	unsigned char *wstr = NULL;
	unsigned short unicode = 0;

	wstr = (unsigned char *)(pstr + 1);

	while(1)
	{
		unicode = (wstr[0] << 8) | wstr[1];
		if(0 == unicode)
		{
			break;
		}
		len++;
		wstr += 2;
	}

	len = len * 2 + 1;

	text->string = (char *)GUI_MALLOC(len + 2);
	if(NULL == text->string)
	{
		return (1);
	}
	memset(text->string, 0, len + 2);

	memcpy(text->string, pstr, len);

	return (0);
}

static int text_set_property(GuiWidget *self, char *name, void *data);
static int _stop_rolling_timer_event(void *userdata)
{
	GuiWidget *self = (GuiWidget *)userdata;
	if(NULL == self) {
		return (1);
	}

	GuiText *text = (GuiText *)(self->object);
	if(NULL == text) {
		return (1);
	}

	text_set_property(self, "rolling_stop_soon", userdata);

	if(text->rolling_stop_timer) {
		remove_timer(text->rolling_stop_timer);
		text->rolling_stop_timer = NULL;
	}
	return (0);
}

static int text_set_property(GuiWidget *self, char *name, void *data)
{
	GuiText* text = NULL;
	GuiWidget *parent = NULL;
	char *pstr = NULL;
	const char *value = NULL;
	int index = 0, color = 0, code_offset = 0;
	char *pcolor = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	text = (GuiText *)(self->object);
	if(NULL == text)
	{
		return (1);
	}

	if(0 == strcasecmp("string", name))
	{
		if(TEXT_MOVE == text->roll_state)
		{
			text->roll_state = TEXT_STILL;
			while(GUI_TRUE != text->text_roll_exit)
			{
				gui_delay(10);
			}
			text->pos = 0;
			text->pos_temp = 0;
			text->times = 0;
		}
		GxCore_MutexLock(self->widget_mutex);
		pstr = (char *)data;
		GUI_FREE(text->string);
		if(NULL == pstr)
		{
			widget_set_update(self, GUI_TRUE);
			GxCore_MutexUnlock(self->widget_mutex);
			return (1);
		}

		if((NULL == pstr) || (0 == strcasecmp("", pstr)))
		{
			GxCore_MutexUnlock(self->widget_mutex);
			return (0);
		}

		code_offset = _text_get_first_byte((unsigned char *)pstr);
		if(code_offset)
		{
			if(0x11 == *pstr)
			{
				_copy_iso10646_string(text, pstr);
			}
			else
			{
				text->string = (char *)GUI_MALLOC(strlen(pstr + code_offset) + code_offset + 1);
				if(NULL == text->string)
				{
					GxCore_MutexUnlock(self->widget_mutex);
					return (1);
				}

				pstr = (char *)data;
				memset(text->string, 0, strlen(pstr + code_offset) + code_offset + 1);
				memcpy(text->string, pstr, code_offset);
				if(strlen(pstr + code_offset))
				{
					strcpy(text->string + code_offset, pstr + code_offset);
				}
				else
				{
					text->string[code_offset] = '\0';
				}
			}
		}
		else
		{
			text->string = GUI_STRDUP(pstr);
		}

		text->pos = text->pos_temp = 0;

		parent = widget_get_parent(self);

		if(NULL != text->surface_roll_text)
		{
			text->roll_state = TEXT_STILL;
			gui_delay(5 + text->rolling_time);
			GxCore_MutexLock(text->text_mutex);
			gal_free_surface(text->surface_roll_text);
			text->surface_roll_text = NULL;
			text->prev_offset = 0;
			text->roll_offset = 0;
			text->prev_pos = 0;
			text->pos_temp = 0;
			GxCore_MutexUnlock(text->text_mutex);
		}

		if(self->state & WIDGET_STATE_HIDE)
		{
			GxCore_MutexUnlock(self->widget_mutex);
			return (0);
		}

		//widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		if(0 == color)
		{
			text->update = GUI_TRUE;
		}

		if(parent)
		{
			if(0 == strcasecmp("boxitem", parent->classname))
			{
				GUI_SetProperty(parent->name, "update", NULL);
			}
		}
		GxCore_MutexUnlock(self->widget_mutex);
	}
	else if(0 == strcasecmp("backcolor", name))
	{
		pcolor = (char *)data;

		GxCore_MutexLock(text->text_mutex);
		if(NULL != text->surface_roll_text)
		{
			text->roll_state = TEXT_STILL;
		}

		widget_set_property(self, "backcolor", data);

		if(pcolor)
		{
			getColorFromString(pcolor, self->back_color);
		}
		else
		{
			GxCore_MutexUnlock(text->text_mutex);
			return (1);
		}

		if(NULL != text->surface_roll_text)
		{
			hd_free_surface(text->surface_roll_text);
			text->surface_roll_text = NULL;
			text->pos_temp = text->pos;
		}

		GxCore_MutexUnlock(text->text_mutex);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("forecolor", name))
	{
		pcolor = (char *)data;

		GxCore_MutexLock(text->text_mutex);
		if(NULL != text->surface_roll_text)
		{
			text->roll_state = TEXT_STILL;
		}

		widget_set_property(self, "forecolor", data);

		if(pcolor)
		{
			getColorFromString(pcolor, self->fore_color);
		}
		else
		{
			GxCore_MutexUnlock(text->text_mutex);
			return (1);
		}

		if(NULL != text->surface_roll_text)
		{
			hd_free_surface(text->surface_roll_text);
			text->surface_roll_text = NULL;
			text->pos_temp = text->pos;
		}

		GxCore_MutexUnlock(text->text_mutex);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("reset_rolling", name))
	{
		text->pos = 0;
		text->pos_temp = 0;
		text->times = 0;
		text->prev_offset = 0;
		text->roll_offset = 0;
		text->prev_pos = 0;
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("continue_rolling", name))
	{
		if(TEXT_MOVE == text->roll_state)
		{
			text->roll_state = TEXT_STILL;
			while(GUI_TRUE != text->text_roll_exit)
			{
				gui_delay(10);
			}
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("rolling_position", name))
	{
		GxCore_MutexLock(text->text_mutex);
		text->roll_state = TEXT_STILL;
		text->pos_temp = text->pos = *((int *)(data));
		widget_set_update(self, GUI_TRUE);
		GxCore_MutexUnlock(text->text_mutex);
	}
	else if(0 == strcasecmp("rolling_stop_soon", name))
	{
		if(TEXT_MOVE == text->roll_state)
		{
			text->roll_state = TEXT_STILL;
			while(GUI_TRUE != text->text_roll_exit)
			{
				text->roll_state = TEXT_STILL;
				gui_delay(10);
			}
		}
	}
	else if(0 == strcasecmp("rolling_stop", name))
	{
		if(gui.console_id == GxCore_ThreadGetId()) {
			if(TEXT_MOVE == text->roll_state) {
				text->roll_state = TEXT_STILL;
				while(GUI_TRUE != text->text_roll_exit) {
					text->roll_state = TEXT_STILL;
					gui_delay(10);
				}
			}
		} else {
			text->rolling_stop_timer = create_timer(_stop_rolling_timer_event,
								100,
								self,
								TIMER_REPEAT_NOW);
		}
	}
	else if(0 == strcasecmp("clear_surface", name))
	{
		if(NULL != text->surface)
		{
			gal_free_surface(text->surface);
			text->surface = NULL;
		}

		value = widget_get_property(self, "keep_rolling");
		if((NULL != text->surface_roll_text) &&
				((NULL == value) || ((value) && (0 == strcasecmp("false", value)))))
		{
			if(TEXT_MOVE == text->roll_state)
			{
				text->roll_state = TEXT_STILL;
				while(GUI_TRUE != text->text_roll_exit)
				{
					gui_delay(10);
				}
				text->pos = 0;
				text->prev_offset = 0;
				text->roll_offset = 0;
				text->prev_pos = 0;
				text->pos_temp = 0;
				text->times = 0;
			}
			gal_free_surface(text->surface_roll_text);
			text->surface_roll_text = NULL;
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("step", name))
	{
		if(TEXT_MOVE == text->roll_state)
		{
			text->roll_state = TEXT_STILL;
			while(GUI_TRUE != text->text_roll_exit)
			{
				gui_delay(10);
			}
		}
		widget_set_property(self, name, data);
		text->step = widget_get_int(self, "step", ROLLING_STEP);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("format", name))
	{
		text->roll_state = TEXT_STILL;
		GxCore_MutexLock(text->text_mutex);
		widget_set_property(self, name, data);
		text->format = _get_text_style(self);
		widget_set_update(self, GUI_TRUE);
		GxCore_MutexUnlock(text->text_mutex);
	} else if(0 == strcasecmp("roll_mode", name)) {
		widget_set_property(self, name, data);
		value = (const char *)widget_get_property(self, "roll_mode");
		if(NULL != value) {
			if(0 == strcasecmp("smooth", value)) {
				text->roll_mode = TEXT_ROLL_SMOOTH;
			} else {
				text->roll_mode = TEXT_ROLL_ANTIFLICKER;
			}
		} else {
			text->roll_mode = TEXT_ROLL_ANTIFLICKER;
		}
	} else
	{
		text->roll_state = TEXT_STILL;
		GxCore_MutexLock(text->text_mutex);
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
		GxCore_MutexUnlock(text->text_mutex);
	}

	return (0);
}

static int text_get_property(GuiWidget *self, char *name, void *data)
{
	GuiText* text = NULL;
	char *string = NULL;
	struct gdi_font* old_font = NULL;

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	text = (GuiText *)(self->object);
	if(NULL == text)
	{
		return (1);
	}

	if(0 == strcasecmp("times", name))
	{
		*((int *)data) = text->times;
	}
	else if(0 == strcasecmp("rolling_position", name))
	{
		*((int *)data) = text->pos;
	}
	else if(0 == strcasecmp("rolling_state", name))
	{
		GxCore_MutexLock(self->widget_mutex);
		old_font = gxgdi_set_font((char *)text->font);
		string = (char *)i18n_get_string((const char *)(text->string));
		if(text->text_roll_exit == GUI_TRUE)
			*((char **)data) = ROLL_STATE_STOP;
		else
		{
			if(text->pos > gdi_text_surfacewidth(string))
				*((char **)data) = ROLL_STATE_END;
			else
				*((char **)data) = ROLL_STATE_ROLLING;
		}
		gxgdi_set_font((char *)old_font);
		GxCore_MutexUnlock(self->widget_mutex);
	}
	else if(0 == strcasecmp("string", name))
	{
		if(NULL != data)
		{
			*((char **)data) = text->string;
		}
	}
	else
	{
		return (1);
	}

	return (0);

}

static int text_unactive(GuiWidget *self)
{
	GuiText *text = NULL;

	if(NULL == self) {
		return (1);
	}

	text = (GuiText *)(self->object);
	if(NULL == text)
	{
		return (1);
	}

	if(text->surface) {
		gal_free_surface(text->surface);
		text->surface = NULL;
	}

	if(text->surface_roll_text) {
		gal_free_surface(text->surface_roll_text);
		text->surface_roll_text = NULL;
	}

	if(text->roll_dst_surface) {
		gal_free_surface(text->roll_dst_surface);
		text->roll_dst_surface = NULL;
	}

	return (0);
}


GuiWidgetOps text_ops =
{
	.owner = "text",
	.create = create_text,
	.release = text_release,
	.draw = text_draw,
	.event = text_event,
	.unactive = text_unactive,
	.set = text_set_property,
	.get = text_get_property
};


