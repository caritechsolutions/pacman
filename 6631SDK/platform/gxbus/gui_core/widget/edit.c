#include <stdlib.h>

#include "all_widget.h"

char *gxstrrev(char *s)
{
	char *h = s;
	char *t = s;
	char ch;

	t = t + strlen(s) - 1;
	while(h < t) {
		ch = *h;
		*h++ = *t;
		*t-- = ch;
	}

	return (s);
}

static int _draw_edit_cursor(GuiWidget *self,
		char intaglio,
		int cur_pos,
		int color,
		int ysize,
		int alignment,
		char *string,
		GUI_Rect *rect)
{
	GuiEdit *edit = NULL;
	GUI_Rect cursor_rect = {0};
	GuiSurface *dst_surface = NULL;
	int i = 0, ret = 0;
	unsigned int cur_char = 0, accent = 0;
	const char *value = NULL;
	unsigned char *pstr = NULL, is_flag = 0;

	if((NULL == self) ||
			(NULL == string) ||
			(NULL == rect))
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	value = widget_get_property(self, "i18n");
	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
		return (1);

	cursor_rect.x = rect->x;
	if((GUI_TA_HCENTRE & alignment) &&
			((0 == cur_pos) && (0 == strlen(string))))
	{
		cursor_rect.x = cursor_rect.x + (rect->w) / 2;
	}
	cursor_rect.y = rect->y + ysize;
	if(intaglio)
	{
		if((string[cur_pos] == edit->default_intaglio) || (string[cur_pos] == 0))
			cur_char = string[cur_pos];
		else
			cur_char = intaglio;
	}
	else
	{
		if((value) && (0 == strcasecmp("true", value)))
		{
			i = 0;

			pstr = (unsigned char *)string;
			for(i = 0; i <= cur_pos; i++)
			{
				ret = gdi_font_utf82unicode(&pstr, &cur_char, &accent, &is_flag);
				if(0 != ret)
				{
					cur_char = 0;
				}
			}
		}
		else
		{
			cur_char = string[cur_pos];
		}
	}
	cursor_rect.w = gdi_get_charwidth(cur_char);// 1;
	cursor_rect.h = 2;
	pstr = (unsigned char *)string;
	for(i = 0; i < cur_pos; i++)
	{
		if(intaglio)
		{
			cur_char = intaglio;
		}
		else
		{
			if((value) && (0 == strcasecmp("true", value)))
			{
				ret = gdi_font_utf82unicode(&pstr, &cur_char, &accent, &is_flag);
				if(0 != ret)
				{
					cur_char = 0;
				}
			}
			else
			{
				cur_char = string[i];
			}
		}
		cursor_rect.x += gdi_get_charwidth(cur_char);
	}

	if(((cursor_rect.x + cursor_rect.w) > (rect->x + rect->w)) && (cur_pos))
	{
		cursor_rect.x = rect->x + rect->w - cursor_rect.w;
	}

	if((value) &&
			(0 == strcasecmp("true", value)) &&
			(gxgdi_is_arabic((const unsigned char *)string)))
	{
		cursor_rect.x = rect->x;
	}

	if(cursor_rect.w)
	{
		gal_fillrect(dst_surface, (GAL_Rect *)&cursor_rect, color);
	}
	else
	{
		cursor_rect.w = gdi_get_charwidth('*');
		gal_fillrect(dst_surface, (GAL_Rect *)&cursor_rect, color);
	}

	return (0);
}

static int _edit_position(char *string, int cursor, int maxlen)
{
	int text_len = 0, str_len = 0, pos = 0;
	char *pstr = NULL, *pcur = NULL;
	unsigned char is_iso = 0;
	unsigned int unicode = 0, accent = 0;

	str_len = strlen(string);
	text_len = gdi_text_len(string);
	if(text_len <= maxlen)
		return 0;

	pcur = pstr = GUI_MALLOCZ(cursor + 10);
	if(NULL == pstr)
	{
		return 0;
	}

	memcpy(pstr, string, cursor);
	pstr[cursor] = '\0';

	text_len = gdi_text_len(pstr);
	if(text_len > maxlen)
	{
		while(*pcur)
		{
			if(gdi_text_len(pcur) <= maxlen)
			{
				break;
			}
			gdi_font_utf82unicode((unsigned char **)&pcur, &unicode, &accent, &is_iso);
		}
	}

	pos = pcur - pstr;
	GUI_FREE(pstr);

	return (pos);
}

static int _edit_draw_slider_string(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiEdit *edit = NULL;
	GuiSurface *surface = NULL, *dst_surface = NULL;
	struct gdi_font *font = NULL;
	GAL_Rect surface_rect = {0};
	GUI_Rect valid_rect = {0}, draw_rect = {0};
	GAL_Interval interval = {0};
	unsigned int total_len = 0, head_len = 0, middle_len = 0, tail_len = 0, source_width = 0;
	int color = 0, Index = 0;

	gui_core = GUI_GetCurrent();
	font = gxgdi_get_font();
	edit = (GuiEdit *)(self->object);
	if(edit->str_head)
	{
		head_len = gdi_text_surfacewidth(edit->str_head);
	}

	if(edit->str_middle)
	{
		middle_len = gdi_text_surfacewidth(edit->str_middle);
	}
	else
	{
		middle_len = gdi_get_charwidth('*');
	}

	if(edit->str_tail)
	{
		tail_len = gdi_text_surfacewidth(edit->str_tail);
	}

	dst_surface = widget_get_current_surface(screen);

	total_len = head_len + middle_len + tail_len;

	surface_rect.x = surface_rect.y = 0;
	if(gxgdi_is_arabic((const unsigned char *)edit->string)) {
		// Fix Arabic string enough length to convert character to other status.
		surface_rect.w = total_len + middle_len;
		if(head_len) {
			head_len += middle_len;
		} else {
			tail_len += middle_len * 2;
		}
	} else {
		surface_rect.w = total_len;
	}
	surface_rect.h = self->rect.h;
	surface = hd_get_surface(&surface_rect, gui_core->config.bpp, NULL, GUI_TRUE);
	if(NULL == surface)
	{
		return (GUI_FALSE);
	}

	valid_rect = self->rect;
	valid_rect.w = total_len;
	if(valid_rect.w < self->rect.w)
	{
		if(GUI_TA_HCENTRE & edit->alignment)
		{
			valid_rect.x += ((self->rect.w - valid_rect.w) / 2);
		}
		else if(GUI_TA_RIGHT & edit->alignment)
		{
			valid_rect.x += (self->rect.w - valid_rect.w);
		}
	}

	if(surface->sf.width > self->rect.w)
	{
		draw_rect = valid_rect;
		draw_rect.x = edit->slider_pos;
		draw_rect.w -= edit->slider_pos;
		valid_rect.w -= edit->slider_pos;
		if((valid_rect.x + valid_rect.w) > dst_surface->sf.width) {
			valid_rect.w = dst_surface->sf.width - valid_rect.x;
			draw_rect.w = valid_rect.w;
		}
		draw_rect.y = 0;
	}
	else
	{
		draw_rect = valid_rect;
		draw_rect.x = draw_rect.y = 0;
		draw_rect.w = surface->sf.width;
		valid_rect.w = draw_rect.w;
	}
	gdi_commit();
	gdi_begin();
	hd_copy_surface(dst_surface, (GAL_Rect *)(&valid_rect), surface, (GAL_Rect *)(&draw_rect));
	gdi_end();
	source_width = draw_rect.w;

	valid_rect.x = surface_rect.x;
	valid_rect.y = surface_rect.y;
	valid_rect.w = surface_rect.w;
	valid_rect.h = surface_rect.h;
	widget_string_alignment(edit->alignment, total_len, font->ysize, &(valid_rect), &(valid_rect));

	Index = widget_get_state_index(self);
	color = self->fore_color[Index];
	color = gal_color2index(dst_surface, color);
	if(edit->str_head)
	{
		draw_rect = valid_rect;
		draw_rect.w = head_len;
		gdi_begin();
		gxgdi_draw_string(surface,
				edit->str_head,
				(GAL_Rect*)(&draw_rect),
				&interval,
				GUI_TA_LEFT | GUI_TA_TOP,
				color,
				SINGLELINE_MODE);
		hd_add_blit_element(NULL);
		gdi_end();
	}

	draw_rect = valid_rect;
	draw_rect.x = head_len;
	draw_rect.w = middle_len;
	if(edit->str_middle)
	{
		gdi_begin();
		gxgdi_draw_string(surface,
				edit->str_middle,
				(GAL_Rect*)(&draw_rect),
				&interval,
				GUI_TA_LEFT | GUI_TA_TOP,
				color,
				SINGLELINE_MODE);
		hd_add_blit_element(NULL);
		gdi_end();
	}

	/*draw_cursor*/
	if(self->state & WIDGET_STATE_FOCUSED)
	{
		draw_rect.y = (draw_rect.y + draw_rect.h - 2);
		draw_rect.h = 2;
		gdi_begin();
		gal_fillrect(surface, (GAL_Rect *)&draw_rect, color);
		gdi_end();
	}

	if(edit->str_tail)
	{
		draw_rect = valid_rect;
		draw_rect.x = head_len + middle_len;
		draw_rect.w = surface->sf.width - draw_rect.x;
		gdi_begin();
		gxgdi_draw_string(surface,
				edit->str_tail,
				(GAL_Rect*)(&draw_rect),
				&interval,
				GUI_TA_LEFT | GUI_TA_TOP,
				color,
				SINGLELINE_MODE);
		hd_add_blit_element(NULL);
		gdi_end();
	}

	draw_rect = valid_rect;
	draw_rect.x = edit->slider_pos;
	draw_rect.w -= edit->slider_pos;
	draw_rect.y = 0;
	draw_rect.h = surface->sf.height;
	if(total_len > self->rect.w)
		total_len = self->rect.w;

	valid_rect.x = self->rect.x;
	valid_rect.y = self->rect.y;
	valid_rect.w = draw_rect.w;
	valid_rect.h = surface->sf.height;
	if(valid_rect.w > self->rect.w)
		valid_rect.w = self->rect.w;

	if(valid_rect.w < self->rect.w)
	{
		if(GUI_TA_HCENTRE & edit->alignment)
		{
			valid_rect.x += ((self->rect.w - valid_rect.w) / 2);
		}
		else if(GUI_TA_RIGHT & edit->alignment)
		{
			valid_rect.x += (self->rect.w - valid_rect.w);
		}
	}

	draw_rect.w = surface->sf.width - draw_rect.x;
	/*
	if(draw_rect.w > valid_rect.w)
		draw_rect.w = valid_rect.w;
	if((draw_rect.x + draw_rect.w) > source_width)
		valid_rect.w = draw_rect.w = source_width;
	*/
	if((valid_rect.x + surface->sf.width) > (self->rect.x + self->rect.w))
		draw_rect.w = valid_rect.w = self->rect.x + self->rect.w - valid_rect.x;
	else
		draw_rect.w = valid_rect.w = surface->sf.width;
	hd_stretch_surface(surface, (GAL_Rect *)&draw_rect, dst_surface, (GAL_Rect *)&valid_rect);

	hd_add_blit_element(surface);

	return (0);
}

static int _edit_draw_string(GuiWidget *self)
{
	GuiEdit *edit;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}
	_edit_draw_slider_string(self);
	return (0);
}

static int _edit_draw_digit(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiSurface *dst_surface = NULL;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	GuiEdit *edit;
	char *new_string = NULL, *pstr = NULL, *pStart = NULL;
	char *str_value = NULL, *ptr = NULL;
	int color = 0, Index = 0, text_len = 0, maxlen = 0, str_max_len = 0, position = 0;
	struct gdi_font *font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);

	if(NULL == edit)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();
	dst_surface = widget_get_current_surface(screen);

	font = gxgdi_get_font();

	valid_rect = self->rect;
	text_len = gdi_text_len(edit->string);

	widget_string_alignment(edit->alignment, text_len, font->ysize, &(self->rect), &(valid_rect));

	Index = widget_get_state_index(self);;

	color = self->fore_color[Index];
	color = gal_color2index(dst_surface, color);
	str_max_len = strlen(edit->string);
	if(str_max_len < edit->maxlen)
		str_max_len = edit->maxlen;
	str_value = (char *)GUI_MALLOC(str_max_len + 1);
	memset(str_value, 0, str_max_len + 1);

	pStart = new_string = (char *)GUI_MALLOC(strlen(edit->string) + 1);
	if(NULL == new_string)
	{
		return (1);
	}

	memset(new_string, 0, strlen(edit->string) + 1);

	pstr = edit->string;
	while(*pstr)
	{
		if((*pstr >= '0') && (*pstr <= '9'))
		{
			*new_string++ = *pstr++;
		}
		else
		{
			pstr++;
		}
	}

	*new_string = '\0';

	if(edit->intaglio)
	{
		if(edit->default_intaglio)
		{
			memset(str_value, edit->intaglio, strlen(edit->string));
			memset(str_value + strlen(edit->string), edit->default_intaglio, edit->maxlen - strlen(edit->string));
			str_value[edit->maxlen] = '\0';
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				str_value = gxstrrev(str_value);
			}
			text_len = gdi_text_len(str_value);
			widget_string_alignment(edit->alignment, text_len, font->ysize, &(self->rect), &(valid_rect));
		}
		else
		{
			maxlen = strlen(edit->string);
			if(maxlen > edit->maxlen)
			{
				maxlen = edit->maxlen;
			}
			memset(str_value, edit->intaglio, maxlen);
			str_value[edit->maxlen] = '\0';

			text_len = gdi_text_len(str_value);
			widget_string_alignment(edit->alignment, text_len, font->ysize, &(self->rect), &(valid_rect));

			memset(str_value, edit->intaglio, strlen(edit->string));
			str_value[strlen(edit->string) ] = '\0';
			if((gui_core->config.mirror) && (self->disable_mirror)) {
				str_value = gxstrrev(str_value);
			}
		}
	}
	else
	{
		strcpy(str_value, edit->string);
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			str_value = gxstrrev(str_value);
		}
	}

	position = _edit_position(edit->string, edit->cur_pos, self->rect.w);
	gxgdi_draw_string(dst_surface,
			str_value + position,
			(GAL_Rect*)(&valid_rect),
			&interval,
			edit->alignment,
			color,
			SINGLELINE_MODE);

	if(self->state & WIDGET_STATE_FOCUSED)
	{
		if(0 == strlen(edit->string))
		{
			ptr = str_value;
		}
		else
		{
			ptr = edit->string;
		}
		_draw_edit_cursor(self,
				edit->intaglio,
				edit->cur_pos,
				color,
				font->ysize,
				edit->alignment,
				ptr + position,
				&valid_rect);
	}


	GUI_FREE(str_value);

	GUI_FREE(pStart);

	return (0);
}

static int _edit_draw_ip(GuiWidget *self)
{
	GuiEdit *edit = NULL;
	GuiSurface *dst_surface = NULL;
	char *ip_string = NULL, *pvalue = NULL, *pStart = NULL;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	int i = 0, color = 0, Index = 0, text_len = 0;
	struct gdi_font *font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	valid_rect = self->rect;

	if(15 != strlen(edit->string))
	{
		return (1);
	}

	pvalue = edit->string;
	pStart = ip_string = (char *)GUI_MALLOC(16);

	if(NULL == ip_string)
	{
		return (1);
	}

	memset(ip_string, 0, 16);

	Index = widget_get_state_index(self);

	color = self->fore_color[Index];
	color = gal_color2index(dst_surface, color);

	while(*pvalue)
	{
		if((*pvalue >= '0') && (*pvalue <= '9'))
		{
			if(edit->intaglio)
			{
				*ip_string = edit->intaglio;
				ip_string++;
				pvalue++;
			}
			else
			{
				*ip_string++ = *pvalue++;
			}
			i++;
		}
		else if(*pvalue == '.' && (0 == (i % 3)))
		{
			*ip_string++ = *pvalue++;
			i = 0;
		}
		else
		{
			GUI_FREE(pStart);
			return (1);
		}

	}
	*ip_string = '\0';

	font = gxgdi_get_font();

	text_len = gdi_text_len(pStart);

	widget_string_alignment(edit->alignment,
			text_len,
			font->ysize,
			&(self->rect),
			&(valid_rect));

	gxgdi_draw_string(dst_surface,
			pStart,
			(GAL_Rect*)(&valid_rect),
			&interval,
			edit->alignment,
			color,
			SINGLELINE_MODE);

	if(self->state & WIDGET_STATE_FOCUSED)
	{
		_draw_edit_cursor(self,
				0,
				edit->cur_pos,
				color,
				font->ysize,
				edit->alignment,
				pStart,
				&valid_rect);
	}

	GUI_FREE(pStart);

	return (0);
}

static int _edit_draw_time(GuiWidget *self)
{
	GuiEdit *edit = NULL;
	GuiSurface *dst_surface = NULL;
	char *pvalue = NULL, *ptime = NULL, *pStart = NULL;
	int i = 0, color = 0, Index = 0, text_len = 0;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	struct gdi_font *font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);
	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	pvalue = edit->string;

	if(5 == strlen(pvalue) || 8 == strlen(pvalue))
	{

		pStart = ptime = (char *)GUI_MALLOC(strlen(pvalue) + 1);
		if(NULL == ptime)
		{
			return (1);
		}

		memset(ptime, 0, strlen(pvalue) + 1);

		while(*pvalue)
		{
			if((*pvalue >= '0') && (*pvalue <= '9'))
			{
				*ptime++ = *pvalue++;
				i++;
			}
			else if((*pvalue == ':') && ( 2 == i))
			{
				*ptime++ = *pvalue++;
			}
			else if((*pvalue == ':') && (4 == i))
			{
				*ptime++ = *pvalue++;
			}
			else
			{
				GUI_FREE(pStart);
			}
		}

		*ptime = '\0';

		font = gxgdi_get_font();

		text_len = gdi_text_len(edit->string);
		valid_rect = self->rect;

		widget_string_alignment(edit->alignment,
				text_len,
				font->ysize,
				&(self->rect),
				&(valid_rect));

		Index = widget_get_state_index(self);;

		color = self->fore_color[Index];
		color = gal_color2index(dst_surface, color);

		gxgdi_draw_string(dst_surface,
				pStart,
				(GAL_Rect*)(&valid_rect),
				&interval,
				edit->alignment,
				color,
				SINGLELINE_MODE);

		if(self->state & WIDGET_STATE_FOCUSED)
		{
			_draw_edit_cursor(self,
					edit->intaglio,
					edit->cur_pos, color,
					font->ysize,
					edit->alignment,
					edit->string,
					&valid_rect);
		}

		GUI_FREE(pStart);

	}
	else
	{
		return (1);
	}

	return (0);
}

static bool _is_digit(char c)
{
	if((c >= '0') && (c <= '9')) {
		return (GUI_TRUE);
	} else {
		return (GUI_FALSE);
	}
}

static int _check_date_format(int type, const char *str)
{
	char *p = NULL;
	int i = 0;

	if(NULL == str)
	{
		return (1);
	}

	p = (char *)str;
	if(10 != strlen(p))
	{
		return (1);
	}

	if((EDIT_DATE_BOX == type) ||
	   (EDIT_DATE_YMD == type) ||
	   (EDIT_DATE_YDM == type)) {
		while(*p) {
			if(4 == i || 7 == i)
			{
				if(_is_digit(*p))
				{
					return (1);
				}
			}
			i++;
			p++;
		}
	} else {
		while(*p) {
			if(2 == i || 5 == i)
			{
				if(_is_digit(*p))
				{
					return (1);
				}
			}
			i++;
			p++;
		}
	}

	return (0);
}

static int _get_date_index_from_format(int format)
{
	int ret = 0;

	switch(format) {
		case EDIT_STRING:
			ret = 0;
			break;
		case EDIT_DIGIT:
			ret = 1;
			break;
		case EDIT_IP_ADDRESS:
			ret = 2;
			break;
		case EDIT_TIME_BOX:
			ret = 3;
			break;
		case EDIT_DATE_BOX:
		case EDIT_DATE_YMD:
		case EDIT_DATE_YDM:
		case EDIT_DATE_MDY:
		case EDIT_DATE_DMY:
			ret = 4;
			break;
		case EDIT_FLOAT:
			ret = 5;
			break;
		default:
			break;
	}

	return(ret);
}

static int _edit_draw_date(GuiWidget *self)
{
	GuiEdit *edit = NULL;
	GuiSurface *dst_surface = NULL;
	struct gdi_font *new_font = NULL, *old_font = NULL;
	int text_len = 0, color = 0, index = 0;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	char *string = NULL;

	if(NULL == self)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(0 == edit->maxlen || NULL == edit->string)
	{
		return (1);
	}

	if(_check_date_format(edit->format, edit->string))
	{
		return (1);
	}

	valid_rect = self->rect;
	/*Draw text content*/
	old_font = gxgdi_set_font((char *)edit->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	string = (char *)i18n_get_string((const char *)(edit->string));

	text_len = gdi_text_len(string);

	widget_string_alignment(edit->alignment,
			text_len,
			new_font->ysize,
			&(self->rect),
			&(valid_rect));

	index = widget_get_state_index(self);
	color = self->fore_color[index];
	color = gal_color2index(dst_surface, color);

	gxgdi_draw_string(dst_surface,
			string,
			(GAL_Rect*)(&valid_rect),
			&interval,
			edit->alignment,
			color,
			SINGLELINE_MODE);

	if(self->state & WIDGET_STATE_FOCUSED)
	{
		_draw_edit_cursor(self,
				edit->intaglio,
				edit->cur_pos,
				color,
				new_font->ysize,
				edit->alignment,
				edit->string,
				&valid_rect);
	}

	gxgdi_set_font((char *)old_font->font_name);

	return (0);
}

static int _edit_draw_float(GuiWidget *self)
{
	GuiEdit *edit = NULL;
	GuiSurface *dst_surface = NULL;
	struct gdi_font *new_font = NULL, *old_font = NULL;
	int text_len = 0, color = 0, index = 0;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	char *string = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(NULL == edit->string)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	valid_rect = self->rect;
	/*Draw text content*/
	old_font = gxgdi_set_font((char *)edit->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	string = (char *)i18n_get_string((const char *)(edit->string));

	if(string[edit->cur_pos] == '.') {
		if(edit->cur_pos) {
			edit->cur_pos--;
		} else {
			edit->cur_pos++;
		}
	}

	text_len = gdi_text_len(string);

	widget_string_alignment(edit->alignment,
			text_len,
			new_font->ysize,
			&(self->rect),
			&(valid_rect));

	index = widget_get_state_index(self);
	color = self->fore_color[index];
	color = gal_color2index(dst_surface, color);

	gxgdi_draw_string(dst_surface,
			string,
			(GAL_Rect*)(&valid_rect),
			&interval,
			edit->alignment,
			color,
			SINGLELINE_MODE);

	if(self->state & WIDGET_STATE_FOCUSED)
	{
		_draw_edit_cursor(self,
				0, //edit->intaglio,
				edit->cur_pos,
				color,
				new_font->ysize,
				edit->alignment,
				edit->string,
				&valid_rect);
	}


	gxgdi_set_font((char *)old_font->font_name);

	return (0);
}

static int _edit_remove_char(GuiEdit *edit)
{
	int index = 0;
	char *pStr = NULL;

	if(NULL == edit)
	{
		return (1);
	}

	index = edit->cur_pos;
	pStr = &(edit->string[index - 1]);
	if(edit->cur_pos == 0)
	{
		memset(edit->string, 0, edit->maxlen + 1);
		return (0);
	}
	edit->cur_pos--;

	if('\0' == *pStr)
	{
		return (0);
	}

	while(*pStr)
	{
		*pStr = *(pStr + 1);
		pStr++;
	}

	return (0);
}

static int _edit_move_cursor(GuiEdit *edit, int offset)
{
	GuiWidget *self = NULL;

	if(NULL == edit)
	{
		return (1);
	}

	self = edit->base;

	if(offset > 0)
	{
		if(edit->cur_pos < (edit->maxlen - 1))
		{
			edit->cur_pos++;
		}
		else
		{
			if(edit->reachend_event)
			{
				widget_set_update(self, GUI_TRUE);
				exec_widget_event_data(edit->base, edit->reachend_event, NULL);
				return (1);
			}
			/*Emit signal to parent*/
		}
	}
	else if(offset < 0)
	{
		if(edit->cur_pos > 0)
		{
			edit->cur_pos--;
		}
		else
		{
			if(edit->reachend_event) {
				widget_set_update(self, GUI_TRUE);
				exec_widget_event_data(edit->base, edit->reachend_event, NULL);
			}
			return (1);
		}
	}
	return (0);
}

static int _edit_string_partition(GuiWidget *self)
{
	GuiEdit *edit = NULL;
	unsigned int offset = 0;
	char *pstr = NULL, *ptail = NULL;
	char *primative_str = NULL;
	int is_arabic = GUI_FALSE, str_num = 0;

	edit = (GuiEdit *)(self->object);
	GUI_FREE(edit->str_head);
	GUI_FREE(edit->str_middle);
	GUI_FREE(edit->str_tail);
	edit->slider_pos = edit->focus_pos = 0;

	primative_str = GUI_STRDUP(edit->string);
	if(NULL == primative_str)
		return (0);

	is_arabic = gxgdi_is_arabic((const unsigned char *)primative_str);
	if(is_arabic)
	{
		GUI_FREE(primative_str);
		primative_str = GUI_MALLOCZ(strlen(edit->string) + 2);
		strcpy(primative_str, edit->string);
		strcat(primative_str, " ");
	}
	str_num = gxgdi_string_num(primative_str);
	if((edit->cur_pos >= str_num) && (is_arabic))
		edit->cur_pos = 0;

	pstr = gxgdi_string_move(primative_str, edit->cur_pos, &offset);

	if(edit->cur_pos)
	{
		edit->str_head = GUI_MALLOCZ((pstr - primative_str) + 10);
		if(NULL == edit->str_head)
		{
			GUI_FREE(primative_str);
			return (1);
		}

		if(edit->intaglio)
		{
			memset(edit->str_head, edit->intaglio, (pstr - primative_str));
		}
		else
		{
			memcpy(edit->str_head, primative_str, (pstr - primative_str));
		}
		edit->str_head[pstr - primative_str] = '\0';
	}

	if((0 == edit->cur_pos) &&
	   (is_arabic) &&
	   (str_num > 1))
	{
		ptail = gxgdi_string_move(pstr, str_num - 1, &offset);
	}
	else
	{
		ptail = gxgdi_string_move(pstr, 1, &offset);
	}
	if(ptail - pstr)
	{
		edit->str_middle = GUI_MALLOCZ((ptail - pstr) + 10);
		if(NULL == edit->str_middle)
		{
			GUI_FREE(primative_str);
			return (1);
		}
		memcpy(edit->str_middle, pstr, (ptail - pstr));
		edit->str_middle[ptail - pstr] = '\0';
	}

	if(*ptail)
	{
		edit->str_tail = GUI_STRDUP(ptail);
	}

	if((0 == edit->cur_pos) &&
	   (is_arabic) &&
	   (str_num > 1))
	{
		ptail = edit->str_tail;
		edit->str_tail = edit->str_middle;
		edit->str_middle = ptail;
	}

	GUI_FREE(primative_str);
	return (0);
}

#define EDIT_CURSOR_LEFT    (0)
#define EDIT_CURSOR_RIGHT   (1)

static int _edit_fix_position(GuiWidget *self, int direction)
{
	GuiEdit *edit = NULL;
	int head_len = 0, middle_len = 0, tail_len = 0, total_len = 0;
	unsigned int slider_pos = 0, focus_pos = 0;

	edit = (GuiEdit *)(self->object);
	if(edit->str_head)
		head_len = gdi_text_len(edit->str_head);

	if(edit->str_middle)
		middle_len = gdi_text_len(edit->str_middle);
	else
		middle_len = gdi_get_charwidth('*');

	if(edit->str_tail)
		tail_len = gdi_text_len(edit->str_tail);

	total_len = head_len + middle_len + tail_len;

	focus_pos = head_len;
	slider_pos = edit->slider_pos;

	if(direction == EDIT_CURSOR_LEFT)
	{
		if(focus_pos < slider_pos)
		{
			slider_pos = focus_pos;
		}
		else if((focus_pos + middle_len - slider_pos) > self->rect.w)
		{
			slider_pos = (focus_pos + middle_len) - self->rect.w;
		}
	}
	else
	{
		if((focus_pos + middle_len - slider_pos) > self->rect.w)
		{
			slider_pos = (focus_pos + middle_len) - self->rect.w;
		}
	}

	edit->slider_pos = slider_pos;
	edit->focus_pos = focus_pos;

	return (0);
}

typedef int (*class_draw)(GuiWidget *self);
class_draw edit_draw_ops[6] =
{
	_edit_draw_string,
	_edit_draw_digit,
	_edit_draw_ip,
	_edit_draw_time,
	_edit_draw_date,
	_edit_draw_float
};

static int _edit_event_string(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int key = 0, value = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		key = event->key.sym;
		if((key >= GUIK_0) && (key <= GUIK_9))
		{
			value = key - GUIK_0 + '0';
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos <= edit->maxlen)
			{
				gdi_lock();
				edit->string[edit->cur_pos] = (char)(value);
				_edit_move_cursor(edit, 1);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_RIGHT);
				gdi_unlock();
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if((key >= GUIK_A) && (key <= GUIK_Z))
		{
			value = key - GUIK_A + 'a';
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos <= edit->maxlen)
			{
				gdi_lock();
				edit->string[edit->cur_pos] = (char)(value);
				_edit_move_cursor(edit, 1);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_RIGHT);
				gdi_unlock();
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos)
			{
				gdi_lock();
				_edit_move_cursor(edit, -1);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_LEFT);
				gdi_unlock();
				widget_set_update(self, GUI_TRUE);
				return (EVENT_TRANSFER_STOP);
			}
		}
		else if(GUIK_RIGHT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos < strlen(edit->string))
			{
				gdi_lock();
				_edit_move_cursor(edit, 1);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_RIGHT);
				gdi_unlock();
			}
			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_DELETE == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos)
			{
				gdi_lock();
				_edit_remove_char(edit);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_LEFT);
				gdi_unlock();
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if((GUIK_UP == key) ||
				(GUIK_DOWN == key) ||
				(GUIK_ESCAPE == key))
		{
			widget_set_update(self, GUI_TRUE);
			if(widget_is_keypress_revert(self)) {
				revert_gui_key(event);
			}
			return (EVENT_TRANSFER_KEEPON);
		}
		else
		{
			value = key;
			if(value < 0x20) {
				return (EVENT_TRANSFER_STOP);
			}
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if(edit->cur_pos <= edit->maxlen)
			{
				gdi_lock();
				edit->string[edit->cur_pos] = (char)(value);
				_edit_move_cursor(edit, 1);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_RIGHT);
				gdi_unlock();
			}
			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}
	return (EVENT_TRANSFER_KEEPON);
}

static int _edit_event_digit(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int key = 0, value = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		if(widget_is_keypress_revert(self)) {
			revert_gui_key(event);
		}
		key = event->key.sym;
		if((key >= GUIK_0) && (key <= GUIK_9))
		{
			value = key - GUIK_0 + '0';
			/*if(strlen(edit->string) == edit->maxlen)
			  {
			  edit->string[edit->cur_pos - 1] = (char)(value);
			  }
			  else
			  {
			  i = strlen(edit->string);
			  edit->string[i + 1] = '\0';
			  while(i > edit->cur_pos)
			  {
			  edit->string[i] = edit->string[i - 1];
			  i--;
			  }
			  edit->string[edit->cur_pos] = (char)(value);
			  }*/
			if(edit->cur_pos <= edit->maxlen)
			{
				edit->string[edit->cur_pos] = (char)(value);
				if(_edit_move_cursor(edit, 1) == 0)
				{
					widget_set_update(self, GUI_TRUE);
				}
			}

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == key)
		{
			_edit_move_cursor(edit, -1);
			widget_set_update(self, GUI_TRUE);
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			if(edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
				widget_set_update(self, GUI_TRUE);
			}

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_DELETE == key)
		{
			if(edit->cur_pos >= 0)
			{
				_edit_remove_char(edit);
				widget_set_update(self, GUI_TRUE);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			;
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	if(widget_is_keypress_revert(self)) {
		revert_gui_key(event);
	}
	return (EVENT_TRANSFER_KEEPON);
}

static int _edit_event_ip(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	char *ip_value[4] = {0};
	int key = 0, value = 0, i = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	event = (GUI_Event *)data;

	if(widget_is_keypress_revert(self)) {
		if(GUIK_LEFT == event->key.sym) {
			event->key.sym = GUIK_RIGHT;
		} else if(GUIK_RIGHT == event->key.sym) {
			event->key.sym = GUIK_LEFT;
		}
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		if((key >= GUIK_0) && (key <= GUIK_9) && (edit->cur_pos < edit->maxlen))
		{
			value = key - GUIK_0 + '0';
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if((0 == edit->cur_pos) ||
					(4 == edit->cur_pos) ||
					(8 == edit->cur_pos) ||
					(12 == edit->cur_pos))
			{
				if(value > '2')
				{
					break;
				}
			}
			else if((1 == edit->cur_pos) ||
					(5 == edit->cur_pos) ||
					(9 == edit->cur_pos) ||
					(13 == edit->cur_pos))
			{
				if(edit->string[edit->cur_pos - 1] == '2' && value > '5')
				{
					break;
				}
			}
			else if((2 == edit->cur_pos) ||
					(6 == edit->cur_pos) ||
					(10 == edit->cur_pos) ||
					(14 == edit->cur_pos))
			{
				if(edit->string[edit->cur_pos - 2] == '2' &&
						edit->string[edit->cur_pos - 1] == '5' &&
						value > '5')
				{
					break;
				}
			}
			else
			{
				;
			}
			edit->string[edit->cur_pos] = (char)(value);

			for(i = 0; i < 4; i++)
			{
				memcpy(ip_value, edit->string + (i * 4), 3);
				ip_value[3] = '\0';
				value = atoi((const char *)ip_value);
				if(255 < value)
				{
					edit->string[i * 4] = '2';
					edit->string[i * 4 + 1] = '5';
					edit->string[i * 4 + 2] = '5';
				}
			}

			_edit_move_cursor(edit, 1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, -1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, -1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, 1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			;
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	return (EVENT_TRANSFER_KEEPON);
}

static int _edit_event_time(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int key = 0, value = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	event = (GUI_Event *)data;

	if(widget_is_keypress_revert(self)) {
		if(GUIK_LEFT == event->key.sym) {
			event->key.sym = GUIK_RIGHT;
		} else if(GUIK_RIGHT == event->key.sym) {
			event->key.sym = GUIK_LEFT;
		}
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		if((key >= GUIK_0) &&
				(key <= GUIK_9) &&
				(edit->cur_pos < edit->maxlen))
		{
			value = key - GUIK_0 + '0';
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			if((0 == edit->cur_pos) && (value > '2'))
			{
				break;
			}
			else if((1 == edit->cur_pos) &&
					(edit->string[edit->cur_pos - 1] == '2') &&
					(value > '3'))
			{
				break;
			}
			else if((3 == edit->cur_pos) &&
					(value > '5'))
			{
				break;
			}
			else if((6 == edit->cur_pos) &&
					(value > '5'))
			{
				break;
			}
			else
			{
				;
			}
			edit->string[edit->cur_pos] = (char)(value);

			if('2' <= edit->string[0])
			{
				if('3' < edit->string[1])
				{
					edit->string[1] = '3';
				}
			}

			_edit_move_cursor(edit, 1);
			if(':' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}
			widget_set_update(self, GUI_TRUE);

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, -1);
			if(':' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, -1);
			}
			widget_set_update(self, GUI_TRUE);

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, 1);
			if(':' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}
			widget_set_update(self, GUI_TRUE);

			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			;
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	return (EVENT_TRANSFER_KEEPON);
}

static int date_type_index[4][6] = {
	{0, 3, 5, 6, 8, 9}, // YYYYMMDD
	{0, 3, 8, 9, 5, 6}, // YYYYDDMM
	{6, 9, 0, 1, 3, 4}, // MMDDYYYY
	{6, 9, 3, 4, 0, 1}, // DDMMYYYY
};

static void _get_date_ymd(int type, const char *string, int *year, int *month, int *day)
{
	int index = 0;
	char year_str[5] = {0}, month_str[3] = {0}, day_str[3] = {0};
	char *pstr = NULL;

	if(EDIT_DATE_BOX == type) {
		type = EDIT_DATE_YMD;
	}

	index = type - EDIT_DATE_YMD;

	pstr = (char *)string + date_type_index[index][0];
	memcpy(year_str, pstr, 4);

	pstr = (char *)string + date_type_index[index][2];
	memcpy(month_str, pstr, 2);

	pstr = (char *)string + date_type_index[index][4];
	memcpy(day_str, pstr, 2);

	*year = atoi(year_str);
	*month = atoi(month_str);
	*day = atoi(day_str);
}

static int _edit_event_date(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int key = 0, value = 0, index = 0, year = 0, month = 0, day = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(EDIT_DATE_BOX == edit->format) {
		index = 0;
	} else {
		index = edit->format - EDIT_DATE_YMD;
	}

	event = (GUI_Event *)data;

	if(widget_is_keypress_revert(self)) {
		if(GUIK_LEFT == event->key.sym) {
			event->key.sym = GUIK_RIGHT;
		} else if(GUIK_RIGHT == event->key.sym) {
			event->key.sym = GUIK_LEFT;
		}
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;

		_get_date_ymd(edit->format, edit->string, &year, &month, &day);

		if((key >= GUIK_0) &&
				(key <= GUIK_9) &&
				(edit->cur_pos < edit->maxlen))
		{
			value = key - GUIK_0 + '0';

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			if((date_type_index[index][3] == edit->cur_pos) &&
					('1' == edit->string[edit->cur_pos - 1]))
			{
				if(key > GUIK_2)
				{
					widget_set_update(self, GUI_TRUE);
					return (EVENT_TRANSFER_STOP);
				}
			}
			if(((date_type_index[index][3] == edit->cur_pos) ||
			   (date_type_index[index][5] == edit->cur_pos)) &&
			   ('0' == edit->string[edit->cur_pos - 1]))
			{
				if(key == GUIK_0)
				{
					edit->string[edit->cur_pos] = '1';
					widget_set_update(self, GUI_TRUE);
					return (EVENT_TRANSFER_STOP);
				}
			}
			else if(date_type_index[index][2] == edit->cur_pos)
			{
				if(key > GUIK_1)
				{
					widget_set_update(self, GUI_TRUE);
					return (EVENT_TRANSFER_STOP);
				}
				else if((key == GUIK_1) &&
						('2' < edit->string[edit->cur_pos + 1]))
				{
					edit->string[edit->cur_pos] = '1';
					edit->string[edit->cur_pos + 1] = '2';
					edit->cur_pos++;
					widget_set_update(self, GUI_TRUE);
					return (EVENT_TRANSFER_STOP);
				}
			}
			else if(date_type_index[index][4] == edit->cur_pos)
			{
				if(key > GUIK_3)
				{
					widget_set_update(self, GUI_TRUE);
					return (EVENT_TRANSFER_STOP);
				}
				else if(2 == month)
				{
					if(GUIK_2 < key)
					{
						value = '2';
					}
					if('9' == edit->string[edit->cur_pos + 1])
					{
						if(((0 == (year % 4)) &&
									(0 != (year % 100))) ||
								(0 == (year % 400)))
						{
							;
						}
						else
						{
							edit->string[edit->cur_pos + 1] = '8';
						}
					}
				}

				if('3' == value)
				{
					if((4 == month) ||
							(6 == month) ||
							(9 == month) ||
							(11 == month))
					{
						edit->string[edit->cur_pos + 1] = '0';
					}
				}
			}
			else if((date_type_index[index][5] == edit->cur_pos) &&
					(('3' == edit->string[edit->cur_pos - 1]) ||
					 ('2' == edit->string[edit->cur_pos - 1])))
			{
				if(2 == month)
				{
					edit->string[edit->cur_pos - 1] = '2';
					if(((0 == (year % 4)) &&
								(0 != (year % 100))) ||
							(0 == (year % 400)))
					{
						;
					}
					else if('9' == value)
					{
						value = '8';
					}
				}
				else if((key >= GUIK_1) &&
						((4 == month) ||
						 (6 == month) ||
						 (9 == month) ||
						 (11 == month)))
				{
					if('3' == edit->string[edit->cur_pos - 1])
					{
						value = '0';
					}
				}
			}
			edit->string[edit->cur_pos] = (char)(value);

			_get_date_ymd(edit->format, edit->string, &year, &month, &day);

			if(0 == year)
			{
				edit->string[date_type_index[index][1]] = '1';
			}

			if(0 == month)
			{
				edit->string[date_type_index[index][3]] = '1';
			}

			if(((4 == month) ||
						(6 == month) ||
						(9 == month) ||
						(11 == month)) &&
					(31 == day))
			{
				if(day > 30)
				{
					edit->string[date_type_index[index][4]] = '3';
				}
				edit->string[date_type_index[index][5]] = '0';
			}
			else if(2 == month)
			{
				if(((0 == (year % 4)) &&
							(0 != (year % 100))) ||
						(0 == (year % 400)))
				{
					if(29 < day)
					{
						edit->string[date_type_index[index][4]] = '2';
						edit->string[date_type_index[index][5]] = '9';
					}
				}
				else
				{
					if(28 < day)
					{
						edit->string[date_type_index[index][4]] = '2';
						edit->string[date_type_index[index][5]] = '8';
					}
				}
			}
			else
			{
				if(day > 31)
				{
					edit->string[date_type_index[index][4]] = '3';
					edit->string[date_type_index[index][5]] = '1';
				}
			}

			if(day == 0)
			{
				edit->string[date_type_index[index][4]] = '0';
				edit->string[date_type_index[index][5]] = '1';
			}

			_edit_move_cursor(edit, 1);
			if(!_is_digit(edit->string[edit->cur_pos]))
			{
				_edit_move_cursor(edit, 1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_LEFT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, -1);
			if(!_is_digit(edit->string[edit->cur_pos]))
			{
				_edit_move_cursor(edit, -1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
			_edit_move_cursor(edit, 1);
			if(!_is_digit(edit->string[edit->cur_pos]))
			{
				_edit_move_cursor(edit, 1);
			}

			widget_set_update(self, GUI_TRUE);
			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			;
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	return (EVENT_TRANSFER_KEEPON);
}

static int _edit_event_float(GuiWidget *self, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int key = 0, value = 0, cur_pos = 0;

	if(NULL == self || NULL == data)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	event = (GUI_Event *)data;

	if(widget_is_keypress_revert(self)) {
		if(GUIK_LEFT == event->key.sym) {
			event->key.sym = GUIK_RIGHT;
		} else if(GUIK_RIGHT == event->key.sym) {
			event->key.sym = GUIK_LEFT;
		}
	}

	switch(event->type)
	{
	case GUI_KEYDOWN:
		key = event->key.sym;
		if((key >= GUIK_0) &&
				(key <= GUIK_9) &&
				(edit->cur_pos < edit->maxlen))
		{
			value = key - GUIK_0 + '0';

			edit->string[edit->cur_pos] = (char)(value);
			_edit_move_cursor(edit, 1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}
			widget_set_update(self, GUI_TRUE);

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}
		}
		else if(GUIK_LEFT == key)
		{
			cur_pos = edit->cur_pos;
			_edit_move_cursor(edit, -1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, -1);
			}
			if(cur_pos)
				widget_set_update(self, GUI_TRUE);

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else if(GUIK_RIGHT == key)
		{
			cur_pos = edit->cur_pos;
			_edit_move_cursor(edit, 1);
			if('.' == edit->string[edit->cur_pos])
			{
				_edit_move_cursor(edit, 1);
			}
			if(cur_pos < (edit->maxlen - 1))
				widget_set_update(self, GUI_TRUE);

			if(edit->change_event)
			{
				exec_widget_event_data(self, edit->change_event, NULL);
			}

			return (EVENT_TRANSFER_STOP);
		}
		else
		{
			;
		}
		break;
	default:
		break;
	}

	widget_set_update(self, GUI_TRUE);
	return (EVENT_TRANSFER_KEEPON);
}

typedef int (*process_event)(GuiWidget *self, void *data);
process_event edit_key_ops[6] =
{
	_edit_event_string,
	_edit_event_digit,
	_edit_event_ip,
	_edit_event_time,
	_edit_event_date,
	_edit_event_float
};

int _edit_get_format(GuiWidget *self)
{
	char *pName = NULL;
	int ret = 0;

	if(NULL == self)
	{
		return (EDIT_DIGIT);
	}

	pName = (char *)widget_get_property(self, "format");
	if(NULL == pName)
	{
		return (EDIT_STRING);
	}
	if(0 == strcmp("edit_string", pName))
	{
		ret = EDIT_STRING;
	}
	else if(0 == strcmp("edit_digit", pName))
	{
		ret = EDIT_DIGIT;
	}
	else if(0 == strcmp("edit_ip", pName))
	{
		ret = EDIT_IP_ADDRESS;
	}
	else if(0 == strcmp("edit_time", pName))
	{
		ret = EDIT_TIME_BOX;
	}
	else if(0 == strcmp("edit_date", pName))
	{
		ret = EDIT_DATE_BOX;
	}
	else if(0 == strcmp("edit_date_ymd", pName))
	{
		ret = EDIT_DATE_YMD;
	}
	else if(0 == strcmp("edit_date_ydm", pName))
	{
		ret = EDIT_DATE_YDM;
	}
	else if(0 == strcmp("edit_date_mdy", pName))
	{
		ret = EDIT_DATE_MDY;
	}
	else if(0 == strcmp("edit_date_dmy", pName))
	{
		ret = EDIT_DATE_DMY;
	}
	else if(0 == strcmp("edit_float", pName))
	{
		ret = EDIT_FLOAT;
	}

	return (ret);
}

static int _check_digit(char *string)
{
	char *pstr = string;

	while(*pstr)
	{
		if(!_is_digit(*pstr))
			return GUI_FALSE;
		pstr++;
	}

	return GUI_TRUE;
}

static int _check_ip(char *string)
{
	char *pstr = string;
	int i = 0, len = 0;

	len = strlen(pstr);
	if(15 != len)
		return GUI_FALSE;

	while(*pstr)
	{
		if((3 == i) || (7 == i) || (11 == i))
		{
			if('.' != *pstr)
				return GUI_FALSE;
		}
		else
		{

			if(((*pstr) < '0') ||
					((*pstr) > '9'))
				return GUI_FALSE;
		}
		i++;
		pstr++;
	}

	return GUI_TRUE;
}

static int _check_time(char *string)
{
	char *pstr = string;
	int i = 0, len = 0;

	len = strlen(pstr);
	if((5 != len) && (8 != len))
		return GUI_FALSE;

	while(*pstr)
	{
		if((5 == len) && (2 == i))
		{
			if(':' != *pstr)
				return GUI_FALSE;
		}
		else if((8 == len) && ((2 == i) || (5 == i)))
		{
			if(':' != *pstr)
				return GUI_FALSE;
		}
		else
		{
			if(((*pstr) < '0') ||
					((*pstr) > '9'))
				return GUI_FALSE;
		}
		i++;
		pstr++;
	}

	return GUI_TRUE;
}

static int _check_date(GuiWidget *self, char *string)
{
	char *pstr = string;
	const char *format = NULL;
	int i = 0, len = 0;

	format = widget_get_property(self, "format");
	if(0 != strcasecmp("edit_date", format)) {
		return(GUI_TRUE);
	}

	len = strlen(string);
	if(10 != len)
		return GUI_FALSE;

	while(*pstr)
	{
		if((4 == i) || (7 == i))
		{
			if(_is_digit(*pstr))
				return GUI_FALSE;
		}
		else
		{

			if(((*pstr) < '0') ||
					((*pstr) > '9'))
				return GUI_FALSE;
		}
		i++;
		pstr++;
	}

	return GUI_TRUE;
}

static int _check_float_point(char *string)
{
	char *pstr = string;
	int flag = 0;

	while(*pstr)
	{
		if(((*pstr) < '0') ||
				((*pstr) > '9'))
		{
			if('.' != *pstr)
				return GUI_FALSE;
			else if('.' == *pstr)
			{
				if(flag == GUI_TRUE)
					return GUI_FALSE;
				flag = GUI_TRUE;
			}
		}
		pstr++;
	}

	return GUI_TRUE;
}

static int _check_string(GuiWidget *self, char *string, EDIT_TYPE type, int maxlen)
{
	if(NULL == string)
		return GUI_TRUE;

	if(((EDIT_IP_ADDRESS == type) ||
				(EDIT_TIME_BOX == type) ||
				(EDIT_DATE_BOX == type)) &&
			(maxlen != strlen(string)))
		return GUI_FALSE;

	switch(type)
	{
	case EDIT_DIGIT:
		return (_check_digit(string));
		break;
	case EDIT_IP_ADDRESS:
		return (_check_ip(string));
		break;
	case EDIT_TIME_BOX:
		return (_check_time(string));
		break;
	case EDIT_DATE_BOX:
		return (_check_date(self, string));
		break;
	case EDIT_FLOAT:
		return (_check_float_point(string));
		break;
	default:
		return GUI_TRUE;
	}
}

void _check_maxlen(GuiEdit *edit)
{
	if(0 > edit->maxlen)
		edit->maxlen = 0;

	if(EDIT_IP_ADDRESS == edit->format)
	{
		if(edit->maxlen != 15)
		{
			edit->maxlen = 15;
			GUI_FREE(edit->string);
			edit->string = GUI_MALLOCZ(edit->maxlen + 1);
		}
	}
	else if(EDIT_DATE_BOX == edit->format)
	{
		if(edit->maxlen != 10)
		{
			edit->maxlen = 10;
			GUI_FREE(edit->string);
			edit->string = GUI_MALLOCZ(edit->maxlen + 1);
		}
	}
	else if(EDIT_TIME_BOX == edit->format)
	{
		if((edit->maxlen != 5) && (edit->maxlen != 8))
		{
			edit->maxlen = 5;
			GUI_FREE(edit->string);
			edit->string = GUI_MALLOCZ(edit->maxlen + 1);
		}
	}

	if(NULL == edit->string)
	{
		edit->string = GUI_MALLOCZ(edit->maxlen + 1);
	}

}

static int _config_edit_parametre(GuiWidget *self)
{
	GuiEdit *edit;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	edit->alignment = widget_get_alignment(self);

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == edit->font))
	{
		edit->font = (char *)widget_get_property(self, "font");
	}

	value = widget_get_property(self, "maxlen");
	if((NULL != value) || (0 == edit->maxlen))
	{
		edit->maxlen = widget_get_int(self, "maxlen", 0);
		edit->cur_pos = 0;
	}

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == edit->format))
	{
		edit->format = _edit_get_format(self);
	}

	return (0);
}

int _check_float(const char *string)
{
	char *pstr = NULL;
	int has_point = 0;

	pstr = (char *)string;

	if(NULL == pstr)
	{
		return (1);
	}

	while(*pstr)
	{
		if(('0' > *pstr) && ('9' < *pstr))
		{
			return (1);
		}
		else if('.' == *pstr)
		{
			has_point = 1;
		}
		pstr++;
	}

	if(0 == has_point)
	{
		gxlogd("Suggesting using the decimal mode or others.\n");
	}

	return (0);
}

static void _fix_ip_address(GuiWidget *self)
{
	char ip_value[30] = {0}, address_str[10];
	char *pstr = NULL;
	int address = 0;
	char *string = (char *)widget_get_property(self, "string");
	if(NULL == string)
	{
		strcpy(ip_value, "000.000.000.000");
		widget_set_property(self, "string", ip_value);
		return;
	}

	pstr = string;
	if(strlen(pstr) > 15)
	{
		strcpy(ip_value, "000.000.000.000");
		widget_set_property(self, "string", ip_value);
		return;
	}
	while(*pstr)
	{
		if(((*pstr < '0') || (*pstr > '9')) &&
				('.' != *pstr))
			break;
		else if('.' == *pstr)
		{
			memset(address_str, 0, 10);
			if(address > 255)
				address = 255;
			sprintf(address_str, "%03d", address);
			address = 0;
			strcat(ip_value, address_str);
			strcat(ip_value, ".");
		}
		else
		{
			address = address * 10 + ((*pstr) - '0');
		}
		pstr++;
	}
	if((*pstr) &&
			((*pstr < '0') || (*pstr > '9')))
		strcpy(ip_value, "000.000.000.000");
	else
	{
		memset(address_str, 0, 10);
		sprintf(address_str, "%03d", address);
		strcat(ip_value, address_str);
	}

	widget_set_property(self, "string", ip_value);
}

static void *create_edit(GuiWidget *self)
{
	GuiEdit* edit = NULL, *edit_style = NULL;
	GuiWidget *parent = NULL, *style = NULL;
	struct gdi_font *old_font = NULL;
	char *pStr = NULL;
	int ret = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	edit = (GuiEdit *)GUI_MALLOC(sizeof(GuiEdit));
	if(NULL == edit)
	{
		return (NULL);
	}
	memset(edit, 0, sizeof(GuiEdit));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	edit->base = self;
	self->object = (void *)(edit);
	self->state = WIDGET_STATE_FOCUSSABLE;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		edit_style = (GuiEdit *)GUI_MALLOC(sizeof(GuiEdit));
		if(NULL == edit_style)
		{
			GUI_FREE(edit);
			return (NULL);
		}

		memset(edit_style, 0, sizeof(GuiEdit));
		style->object = (void *)edit_style;

		_config_edit_parametre(style);

		memcpy(edit, edit_style, sizeof(GuiEdit));
		edit->base = self;

		GUI_FREE(style->object);
	}

	_config_edit_parametre(self);
	_check_maxlen(edit);

	if(NULL == edit->string)
	{
		edit->string = (char *)GUI_MALLOC(edit->maxlen + 1);
	}
	edit->string[edit->maxlen] = '\0';
	if(GUI_FALSE == _check_string(self, (char *)widget_get_property(self, "string"), edit->format, edit->maxlen))
	{
		if(EDIT_IP_ADDRESS == edit->format)
		{
			_fix_ip_address(self);
		}
		else
		{
			GUI_Debug_Print("The string %s is ivalide, Please check!\n", (char *)widget_get_property(self, "string"));
			GUI_FREE(edit->string);
			return (edit);
		}
	}
	if(widget_get_property(self, "string") &&
			(edit->maxlen >= strlen(widget_get_property(self, "string"))))
	{
		edit->font = (char*)widget_get_property(self, "font");
		gdi_lock();
		old_font = gxgdi_set_font((char *)edit->font);
		strcpy(edit->string, widget_get_property(self, "string"));
		_edit_string_partition(self);
		_edit_fix_position(self, EDIT_CURSOR_RIGHT);
		gxgdi_set_font(old_font->font_name);
		gdi_unlock();
	}

	edit->cur_pos = 0;

	pStr = (char *)widget_get_property(self, "intaglio");
	if(NULL != pStr)
	{
		edit->intaglio = *pStr;
	}

	pStr = (char *)widget_get_property(self, "default_intaglio");
	if(NULL != pStr)
	{
		edit->default_intaglio = *pStr;
	}

	if(EDIT_DATE_BOX == edit->format)
	{
		edit->maxlen = 10;
	}
	else if(EDIT_IP_ADDRESS == edit->format)
	{
		edit->maxlen = 15;
	}
	else if(EDIT_FLOAT == edit->format)
	{
		ret = _check_float(edit->string);

		if(0 != ret)
		{
			GUI_FREE(edit->string);
			GUI_FREE(self->object);
			return (NULL);
		}
	}

	if(self->signal)
	{
		edit->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		edit->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		edit->keypress_event = (GuiWidgetSignal *)hash_get(self->signal, "keypress");
		edit->clicked_event = (GuiWidgetSignal *)hash_get(self->signal, "clicked");
		edit->change_event = (GuiWidgetSignal *)hash_get(self->signal, "change");
		edit->reachend_event = (GuiWidgetSignal *)hash_get(self->signal, "reach_end");
	}

	if(edit->create_event)
	{
		exec_widget_event_data(self, edit->create_event, NULL);
	}
	return (edit);
}



static int edit_draw(GuiWidget *self)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiEdit *edit = NULL;
	GuiSurface *dst_surface = NULL;
	int Index = 0, color = 0, ops_index = 0;
	GUI_Rect img_rect = {0};
	char *pstr = NULL;
	struct gdi_font* old_font = NULL, *new_font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	dst_surface = widget_get_current_surface(screen);

	if(edit->maxlen != widget_get_int(self, "maxlen", 0))
	{
		GUI_FREE(edit->string);
		edit->maxlen = widget_get_int(self, "maxlen", 0);
		_check_maxlen(edit);
	}

	pstr = (char *)widget_get_property(self, "intaglio");
	if(pstr)
	{
		edit->intaglio = *pstr;
	}
	else
	{
		edit->intaglio = 0;
	}

	pstr = (char *)widget_get_property(self, "default_intaglio");
	if(pstr)
	{
		edit->default_intaglio = *pstr;
	}
	else
	{
		edit->default_intaglio = 0;
	}

	if((EDIT_DIGIT == edit->format) && (NULL == edit->string))
	{
		edit->string = GUI_MALLOCZ(edit->maxlen);
		if(NULL == edit->string)
			return (1);
	}

	if((edit->intaglio) || (edit->default_intaglio))
	{
		char intaglio_str[2] = {0};
		unsigned int intaglio_len = 0;

		if(edit->intaglio)
		{
			intaglio_str[0] = edit->intaglio;
			intaglio_str[1] = '\0';
			intaglio_len = gdi_text_len(intaglio_str) * strlen(edit->string);
		}
		else if(edit->default_intaglio)
		{
			intaglio_str[0] = edit->default_intaglio;
			intaglio_str[1] = '\0';
			intaglio_len = (intaglio_len > gdi_text_len(intaglio_str) * strlen(edit->string)) ?
				(intaglio_len) :
				(gdi_text_len(intaglio_str) * strlen(edit->string));
		}

		if(intaglio_len > self->rect.w)
		{
			return (0);
		}
	}
	else if((gdi_text_len(edit->string) > self->rect.w) &&
			(EDIT_STRING != edit->format))
	{
		return (0);
	}

	Index = widget_get_state_index(self);
	if(NULL != edit->image_g[Index][1])
	{
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			widget_img_group_draw_mirror(dst_surface,
						     &(self->rect),
						     edit->image_g[Index][0],
						     edit->image_g[Index][1],
						     edit->image_g[Index][2],
						     self->back_color[Index],
						     REVERSE_HORIZONTAL);
		} else {
			widget_img_group_draw(dst_surface,
					      &(self->rect),
					      edit->image_g[Index][0],
					      edit->image_g[Index][1],
					      edit->image_g[Index][2],
					      self->back_color[Index]);
		}
	}
	else if(NULL != edit->image[Index])
	{
		img_rect = self->rect;
		img_rect.w = edit->image[Index]->width;
		img_rect.h = edit->image[Index]->height;
		if((gui_core->config.mirror) && (self->disable_mirror)) {
			gal_img_desc_mirror(dst_surface,
					    edit->image[Index],
					    self->rect.x,
					    self->rect.y,
					    self->rect.w,
					    self->rect.h,
					    REVERSE_HORIZONTAL);
		} else {
			gal_img_desc(dst_surface,
					edit->image[Index],
					self->rect.x,
					self->rect.y,
					self->rect.w,
					self->rect.h);
		}
	}
	else
	{
		color = self->back_color[Index];
		color = gal_color2index(dst_surface, color);
		gal_fillrect(dst_surface, (GAL_Rect *)&(self->rect), color);
	}

	edit->format = _edit_get_format(self);

	if(edit->string)
	{
		edit->font = (char*)widget_get_property(self, "font");
		old_font = gxgdi_set_font((char *)edit->font);

		new_font = gxgdi_get_font();
		if(NULL == new_font)
		{
			return (1);
		}

		if((gui_core->config.mirror) &&
		   (self->disable_mirror) &&
		   ((edit->format == EDIT_DIGIT) ||
		    (edit->format == EDIT_STRING))) {
			edit->cur_pos = edit->maxlen - edit->cur_pos - 1;
		}

		edit->alignment = widget_alignment_revert(self, edit->string, edit->alignment);

		ops_index = _get_date_index_from_format(edit->format);
		if(edit_draw_ops[ops_index])
		{
			edit_draw_ops[ops_index](self);
		}
		if(old_font)
		{
			gxgdi_set_font((char *)old_font);
		}

		if((gui_core->config.mirror) &&
		   (self->disable_mirror) &&
		   ((edit->format == EDIT_DIGIT) ||
		    (edit->format == EDIT_STRING))) {
			edit->cur_pos = edit->maxlen - edit->cur_pos - 1;
		}
	}


	return (0);
}

static int edit_release(GuiWidget *self)
{
	GuiEdit *edit = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(edit->destroy_event)
	{
		exec_widget_event_data(self, edit->destroy_event, NULL);
	}

	GUI_FREE(edit->string);
	GUI_FREE(edit->str_head);
	GUI_FREE(edit->str_middle);
	GUI_FREE(edit->str_tail);

	GUI_FREE(self->object);


	return (0);
}

static int edit_event(GuiWidget *self, void *data)
{
	GuiEdit *edit = NULL;
	GUI_Event *event = NULL;
	int ret = 0, index = 0;
	struct gdi_font *old_font = NULL;

	WIDGET_CHECK_OBJECT(self, data, edit, GuiEdit);

	event = (GUI_Event *)data;

	switch(event->type)
	{
	case GUI_KEYDOWN:
		/*usr signal callback*/
		if(edit->keypress_event)
		{
			ret = exec_widget_event_data(self, edit->keypress_event, data);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
			widget_set_update(self, GUI_TRUE);
		}

		/*object private callback*/
		index = _get_date_index_from_format(edit->format);
		if(edit_key_ops[index])
		{
			if(NULL == edit->string)
				return (EVENT_TRANSFER_STOP);

			edit->font = (char*)widget_get_property(self, "font");
			old_font = gxgdi_set_font((char *)edit->font);
			ret = edit_key_ops[index](self, data);
			gxgdi_set_font(old_font->font_name);
			if(EVENT_TRANSFER_STOP == ret)
			{
				return ret;
			}
		}
		break;
	case GUI_MOUSEBUTTONDOWN:
		if(edit->clicked_event)
		{
			exec_widget_event_data(self, edit->clicked_event, data);
			widget_set_update(self, GUI_TRUE);//temp
		}
		break;
	default:
		break;
	}

	return EVENT_TRANSFER_KEEPON;
}

static int edit_set_property(GuiWidget *self, char *name, void *data)
{
	GuiEdit *edit = NULL;
	GuiWidget *parent = NULL;
	char *string = NULL;
	struct gdi_font *old_font = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(0 == strcasecmp("string", name))
	{
		gdi_lock();
		GxCore_MutexLock(self->widget_mutex);
		string = (char *)data;
		if(GUI_FALSE == _check_string(self, string, edit->format, edit->maxlen))
		{
			if(EDIT_IP_ADDRESS == edit->format)
			{
				widget_set_property(self, "string", string);
				_fix_ip_address(self);
				string = (char *)widget_get_property(self, "string");
			}
			else
			{
				GUI_Debug_Print("The string %s is ivalide, Please check!\n", (char *)string);
				GUI_FREE(edit->string);
				string = NULL;
				widget_set_update(self, GUI_TRUE);
				GxCore_MutexUnlock(self->widget_mutex);
				gdi_unlock();
				return (1);
			}
		}
		if(string)
		{
			edit->font = (char*)widget_get_property(self, "font");
			old_font = gxgdi_set_font((char *)edit->font);
			if(strlen(string) <= edit->maxlen)
			{
				if(NULL == edit->string)
				{
					edit->string = GUI_MALLOCZ(edit->maxlen + 1);
				}
				strcpy(edit->string, string);
				_edit_string_partition(self);
				_edit_fix_position(self, EDIT_CURSOR_RIGHT);
			}
			gxgdi_set_font(old_font->font_name);
			widget_set_update(self, GUI_TRUE);
		}

		parent = widget_get_parent(self);
		if(parent)
		{
			if(0 == strcasecmp("boxitem", parent->classname))
			{
				GUI_SetProperty(parent->name, "update", NULL);
			}
		}
		//widget_set_property(self, name, data);
		GxCore_MutexUnlock(self->widget_mutex);
		gdi_unlock();
	}
	else if(0 == strcasecmp("clear", name))
	{
		if(NULL == edit->string)
			return (1);
		edit->font = (char*)widget_get_property(self, "font");
		gdi_lock();
		old_font = gxgdi_set_font((char *)edit->font);
		memset(edit->string, 0, edit->maxlen + 1);
		edit->cur_pos = 0;
		_edit_string_partition(self);
		_edit_fix_position(self, EDIT_CURSOR_RIGHT);
		gxgdi_set_font(old_font->font_name);
		gdi_unlock();

		widget_set_update(self, GUI_TRUE);

		parent = widget_get_parent(self);
		if(parent)
		{
			if(0 == strcasecmp("boxitem", parent->classname))
			{
				GUI_SetProperty(parent->name, "update", NULL);
			}
		}
	}
	else if(0 == strcasecmp("format", name))
	{
		widget_set_property(self, (const char*)(name), (const char*)data);
		edit->format = _edit_get_format(self);
		GUI_FREE(edit->string);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("maxlen", name))
	{
		widget_set_property(self, (const char*)(name), (const char*)data);
		edit->maxlen = widget_get_int(self, "maxlen", 0);
		_check_maxlen(edit);
		edit->cur_pos = 0;
		GUI_FREE(edit->string);
		edit->string = (char *)GUI_MALLOC(edit->maxlen + 1);
		memset(edit->string, 0, edit->maxlen + 1);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("select", name))
	{
		int pos = *((int *)(data));
		if(pos >= edit->maxlen)
		{
			pos = edit->maxlen - 1;
		}
		else if (pos < 0)
		{
			pos = 0;
		}

		if(pos > edit->maxlen)
		{
			return (0);
		}

		edit->cur_pos = pos;

		edit->font = (char*)widget_get_property(self, "font");
		gdi_lock();
		old_font = gxgdi_set_font((char *)edit->font);
		_edit_string_partition(self);
		_edit_fix_position(self, EDIT_CURSOR_RIGHT);
		gxgdi_set_font(old_font->font_name);
		gdi_unlock();
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("unfocus_img", name))
	{
		string  = (char *)data;

		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("focus_img", name))
	{
		string  = (char *)data;

		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("disable_img", name))
	{
		string  = (char *)data;

		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		widget_set_property(self, (const char*)(name), (const char*)data);
		widget_set_update(self, GUI_TRUE);
		return (0);
	}

	return (0);
}


static int edit_get_property(GuiWidget *self, char *name, void *data)
{
	GuiEdit *edit = NULL;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	if(0 == strcasecmp("string", name))
	{
		if(NULL == edit->string)
		{
			return (1);
		}

		*(char**)data = edit->string;
	}
	else if(0 == strcasecmp("select", name))
	{
		*((int *)(data)) = edit->cur_pos;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int edit_prepare_image(GuiWidget *self)
{
	GuiEdit *edit;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	value = widget_get_property(self, "unfocus_img");
	if((NULL != value) || (NULL == edit->image[0]) || (NULL == edit->image_g[0][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(edit->image_g[0][0]),
								  &(edit->image_g[0][1]),
								  &(edit->image_g[0][2]));
		}
		else
		{
			edit->image[0] = widget_img_load(value);
			edit->image_g[0][0] = edit->image_g[0][1] = edit->image_g[0][2] = NULL;
		}
	}
	value = widget_get_property(self, "focus_img");
	if((NULL != value) || (NULL == edit->image[1]) || (NULL == edit->image_g[1][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(edit->image_g[1][0]),
								  &(edit->image_g[1][1]),
								  &(edit->image_g[1][2]));
		}
		else
		{
			edit->image[1] = widget_img_load(value);
			edit->image_g[1][0] = edit->image_g[1][1] = edit->image_g[1][2] = NULL;
		}
	}
	value = widget_get_property(self, "disable_img");
	if((NULL != value) || (NULL == edit->image[2]) || (NULL == edit->image_g[2][1]))
	{
		if(widget_check_img_mode(value))
		{
			widget_img_group_load(value,
								  &(edit->image_g[2][0]),
								  &(edit->image_g[2][1]),
								  &(edit->image_g[2][2]));
		}
		else
		{
			edit->image[2] = widget_img_load(value);
			edit->image_g[2][0] = edit->image_g[2][1] = edit->image_g[2][2] = NULL;
		}
	}

	return (0);
}

static int edit_clear_image(GuiWidget *self)
{
	GuiEdit *edit;
	int i = 0;

	if(NULL == self)
	{
		return (1);
	}

	edit = (GuiEdit *)(self->object);
	if(NULL == edit)
	{
		return (1);
	}

	for(i = 0; i < 3; i++) {
		if(edit->image[i]) {
			widget_img_clear(&(edit->image[i]));
		}

		if((edit->image_g[i][0]) ||
		   (edit->image_g[i][1]) ||
		   (edit->image_g[i][2])) {
			widget_img_group_clear(&(edit->image_g[i][0]),
					       &(edit->image_g[i][1]),
					       &(edit->image_g[i][2]));
		}
	}

	return (0);
}

GuiWidgetOps edit_ops =
{
	.owner = "edit",
	.create = create_edit,
	.release = edit_release,
	.prepare_image = edit_prepare_image,
	.draw = edit_draw,
	.clear_image   = edit_clear_image,
	.event = edit_event,
	.set = edit_set_property,
	.get = edit_get_property
};




