#include <sys/stat.h>

#include "all_widget.h"

static int _config_notepad_parametre(GuiWidget *self)
{
	GuiNotepad *notepad = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == notepad->font))
	{
		notepad->font = (char *)(value);
	}

	value = widget_get_property(self, "alignment");
	if((NULL != value) || (0 == notepad->alignment))
	{
		notepad->alignment = widget_get_alignment(self);
	}

	value = widget_get_property(self, "vertical_margin");
	if((NULL != value) || (0 == notepad->vertical_margin))
	{
		notepad->vertical_margin = widget_get_int(self, "vertical_margin", 0);
	}

	value = widget_get_property(self, "horizontal_margin");
	if((NULL != value) || (0 == notepad->horizontal_margin))
	{
		notepad->horizontal_margin = widget_get_int(self, "horizontal_margin", 0);
	}

	value = widget_get_property(self, "vinterval");
	if((NULL != value) || (0 == notepad->vinterval))
	{
		notepad->vinterval = widget_get_int(self, "vinterval", 0);
	}

	value = widget_get_property(self, "hinterval");
	if((NULL != value) || (0 == notepad->hinterval))
	{
		notepad->hinterval = widget_get_int(self, "hinterval", 0);
	}

	value = widget_get_property(self, "scrollbar_auto_hide");
	if((NULL != value) && (0 == strcasecmp("true", value)))
	{
		notepad->autohide_scroll = GUI_TRUE;
	}

	value = widget_get_property(self, "epg");
	if((NULL != value) && (0 == strcasecmp("true", value)))
	{
		notepad->epg_flag = GUI_TRUE;
	}

	value = widget_get_property(self, "buffer_size");
	if((NULL != value) && (0 == notepad->buffer_size))
	{
		notepad->buffer_size = widget_get_int(self, "buffer_size", FILE_BUFFER_LIMIT);
	}
	else
		notepad->buffer_size = FILE_BUFFER_LIMIT;

	return (0);
}

static int _notepad_alignment(int alignment, int v_margin, int h_margin, GUI_Rect *rect)
{
	if(NULL == rect)
	{
		return (1);
	}

	if(v_margin >= rect->h)
	{
		return (1);
	}

	if(h_margin >= rect->w)
	{
		return (1);
	}

	switch (alignment & 0x3)
	{
	case GUI_TA_HCENTRE:
	case GUI_TA_RIGHT:
		rect->x += (h_margin / 2);
		rect->w -= h_margin;
		break;
	case GUI_TA_LEFT:
		rect->w -= h_margin;
		break;
	default:
		break;
	}

	switch (alignment & 0xc)
	{
	case GUI_TA_VCENTRE:
		rect->y += (v_margin / 2);
		rect->h -= v_margin;
		break;
	case GUI_TA_BOTTOM:
		rect->y += v_margin;
		rect->h -= v_margin;
		break;
	case GUI_TA_TOP:
		rect->h -= v_margin;
		break;
	default:
		break;
	}

	return (0);
}


static void* create_notepad(GuiWidget *self)
{
	GuiNotepad *notepad = NULL, *notepad_style = NULL;
	GuiWidget *child = NULL, *style = NULL, *parent = NULL;
	struct stat buf;
	int ret = 0;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};

	if(NULL == self)
	{
		return (NULL);
	}

	if(self->object)
	{
		return (self->object);
	}

	notepad = (GuiNotepad *)GUI_MALLOC(sizeof(GuiNotepad));
	if(NULL == notepad)
	{
		return (NULL);
	}

	memset(notepad, 0, sizeof(GuiNotepad));

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	self->object = (void *)notepad;
	notepad->base = self;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style)
	{
		notepad_style = (GuiNotepad *)GUI_MALLOC(sizeof(GuiNotepad));
		if(NULL == notepad_style)
		{
			GUI_FREE(notepad);
			return (NULL);
		}

		memset(notepad_style, 0, sizeof(GuiNotepad));

		style->object = notepad_style;

		_config_notepad_parametre(style);

		memcpy(notepad, notepad_style, sizeof(GuiNotepad));
		notepad->base = self;
		GUI_FREE(style->object);
	}

	_config_notepad_parametre(self);

	self->state = 0;

	notepad->file_path = (char *)widget_get_property(self, "file");
	if(notepad->file_path)
	{
		ret = stat(notepad->file_path, &buf);
		if (ret < 0)
		{
			GUI_FREE(self->object);
			return (NULL);
		}

		notepad->file = GxCore_Open((char *)notepad->file_path, "r");
	}

	if (notepad->file)
	{
		notepad->string = (char *)GUI_MALLOCZ(notepad->buffer_size * 2 + 2);
		if(NULL == notepad->string)
		{
			GUI_FREE(notepad);
			GxCore_Close(notepad->file);
			return (NULL);
		}

		memset(notepad->string, 0, notepad->buffer_size * 2);
		notepad->file_size = buf.st_size;
		GxCore_Read(notepad->file, notepad->string, notepad->buffer_size, 1);

		valid_rect = self->rect;

		_notepad_alignment(notepad->alignment,
				notepad->vertical_margin,
				notepad->horizontal_margin,
				&valid_rect);

		interval.horizontal = notepad->hinterval;
		interval.vertical = notepad->vinterval;
		gdi_lock();
		notepad->total_num = gdi_text_get_lines((void *)notepad->string,
				(GAL_Rect *)&(self->rect),
				&interval);
		gdi_unlock();
		notepad->visible_lines = self->rect.h / gdi_get_charheight();
	}

	notepad->vertical_margin = widget_get_int(self, "vertical_margin", 0);
	notepad->horizontal_margin = widget_get_int(self, "horizontal_margin", 0);
	notepad->delay = widget_get_int(self, "delay", 0);
	notepad->active_line = -1;
	notepad->line_num = 0;
	notepad->line_index = 0;
	notepad->temp_line = 0;
	notepad->line_pos = NULL;
	memset(notepad->active_array, 0, sizeof(int) * MAX_ACTIVE_LINE);
	// TODO:Add delay timer

	child = widget_get_firstchild(self);
	while(child)
	{
		if(0 != strcasecmp("scrollbar", child->classname))
		{
			child = widget_get_nextsibling(child);
			continue;
		}
		widget_create(child);

		child->rect.x += self->rect.x;
		child->rect.y += self->rect.y;

		child = widget_get_nextsibling(child);
	}
	notepad->adjust_string = GUI_FALSE;

	if(self->signal)
	{
		notepad->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		notepad->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
	}

	if(notepad->create_event)
	{
		exec_widget_event_data(self, notepad->create_event, NULL);
	}

	return (0);
}

static int notepad_release(GuiWidget *self)
{
	GuiNotepad *notepad = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	if(notepad->save_context)
	{
		return (0);
	}

	GUI_FREE(notepad->line_pos);

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);

		child = widget_get_nextsibling(child);
	}

	if(notepad->destroy_event)
	{
		exec_widget_event_data(self, notepad->destroy_event, NULL);
	}

	if(notepad->file)
	{
		GxCore_Close(notepad->file);
		GUI_FREE((notepad->string));
	}

	GUI_FREE((notepad->arabic_string));

	GUI_FREE((self->object));

	return (0);
}

static int _notepad_calc_offset(const char *p_start, const char *string, GAL_Interval *interval, int width, int alignment, int line_count)
{
	char *p_str = NULL;
	int offset = 0, i = 0, count = 0, delta = 0;

	p_str = (char *)string;
	if(NULL == p_str || NULL == p_start || NULL == interval)
	{
		return (0);
	}

	if(line_count < 0)
	{
		count = -line_count;
	}
	else
	{
		count = line_count;
	}

	if((string < p_start) || (string > (p_start + strlen(p_start))))
	{
		return (0);
	}

	while(i < count)
	{
		if(line_count > 0)
		{
			delta = gdi_text_skip_line((void *)p_start, (void *)p_str, interval, width, alignment);
			if(*(p_str + delta) != '\0')
			{
				p_str += delta;
			}
			else
			{
				delta = 0;
				break;
			}
		}
		else
		{
			delta = gdi_text_skip_line((void *)p_start, (void *)p_str, interval, -width, alignment);
			if(*(p_str - delta) != '\0')
				p_str -= delta;
		}
		offset += delta;
		i++;
	}

	if(i < count)
		offset = -1;

	return (offset);
}

static int _get_first_code(char *pstr)
{
	char *str = NULL;
	int first_code = 0;

	if(NULL == pstr)
	{
		return (0);
	}

	str = pstr;
	if(NULL == str)
	{
		return (0);
	}

	if((*str > 0) && (*str <= 0x5))
	{
		first_code = *str;
	}
	else if(0x10 == *str)
	{
		first_code = (*(str + 1) << 8) | (*(str + 2));
	}
	else if(0x11 == *str)
	{
		first_code = *str;
		return (111);
	}
	else
	{
		first_code = 0;
	}

	return (first_code);
}

static int _vertical_alignment(int alignment,
		char *ptr,
		GUI_Rect *valid_rect,
		GAL_Interval *interval)
{
	int line_num = 0;

	line_num = gdi_text_get_lines(ptr, (GAL_Rect*)(valid_rect), interval);
	line_num = line_num * (gdi_get_charheight() + interval->vertical);
	if(line_num < valid_rect->h)
	{
		switch (alignment & 0xc)
		{
		case GUI_TA_VCENTRE:
			valid_rect->y += (valid_rect->h - line_num) / 2;
			valid_rect->h = line_num;
			break;
		case GUI_TA_BOTTOM:
			valid_rect->y += valid_rect->h - line_num;
			valid_rect->h = line_num;
			break;
		default:
			break;
		}
	}

	return (0);
}

static void _draw_notepad_line(GuiNotepad *notepad, int active_line)
{
	GuiWidget *self = NULL;
	int active_off = 0, active_color = 0, offset = 0, index = 0, color = 0;
	GAL_Interval interval = {0};
	GUI_Rect valid_rect = {0};
	struct gdi_font* new_font = NULL;

	self = (void *)(notepad->base);

	valid_rect = (GUI_Rect)(self->rect);

	interval.horizontal = notepad->hinterval;
	interval.vertical = notepad->vinterval;
	_notepad_alignment(notepad->alignment,
			   notepad->vertical_margin,
			   notepad->horizontal_margin,
			   &valid_rect);
	_vertical_alignment(notepad->alignment,
			    notepad->string + notepad->text_offset,
			    &valid_rect,
			    &interval);

	active_off = _notepad_calc_offset(notepad->string,
					  notepad->string,
					  &interval,
					  valid_rect.w,
					  notepad->alignment,
					  active_line);
	if(active_off < 0)
		return;

	active_color = self->fore_color[1];
	active_color = gal_color2index(screen, active_color);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return;
	}

	offset = (new_font->ysize + interval.vertical) * (active_line - notepad->line_num);

	if(((valid_rect.y + offset) >= valid_rect.y) && ((valid_rect.y + offset) <= (valid_rect.y + valid_rect.h)))
	{
		GUI_Rect note_rect = {0};
		int text_len = {0};

		note_rect = valid_rect;
		note_rect.y += offset;
		note_rect.h = new_font->ysize;

		text_len = gdi_text_len(notepad->string + active_off);

		if((notepad->string + active_off) >= (notepad->string + notepad->text_offset)) {
			if(NULL == notepad->img_desc)
			{
				GAL_Rect clear_rect = {0};

				clear_rect.x = self->rect.x;
				clear_rect.w = self->rect.w;
				clear_rect.y = note_rect.y;
				clear_rect.h = note_rect.h;
				index = widget_get_state_index(self);
				color = self->back_color[index];
				color = gal_color2index(screen, color);
				gal_fillrect(screen, (GAL_Rect *)(&clear_rect), color);
			}
		}
		gxgdi_draw_string(screen,
				notepad->string + active_off,
				(GAL_Rect*)(&note_rect),
				&interval,
				notepad->alignment,
				active_color,
				PARAGRAPH_MODE);
	}
}

static int notepad_draw(GuiWidget *self)
{
	GuiNotepad *notepad = NULL;
	GuiWidget *child = NULL;
	int index = 0, color = 0, offset = 0;
	int first_code = 0, len = 0;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	char *string = NULL, *ptr = NULL;
	struct gdi_font* old_font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	valid_rect = (GUI_Rect)(self->rect);

	notepad->font = (char*)widget_get_property(self, "font");
	old_font = gxgdi_set_font((char *)(notepad->font));

	if(NULL == notepad->img_desc)
	{
		index = widget_get_state_index(self);
		color = self->back_color[index];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)(&valid_rect), color);
	}
	else
	{
		gal_img_desc(screen,
				notepad->img_desc,
				valid_rect.x,
				valid_rect.y,
				valid_rect.w,
				valid_rect.h);
	}

	index = widget_get_state_index(self);
	color = self->fore_color[index];
	color = gal_color2index(screen, color);

	interval.horizontal = notepad->hinterval;
	interval.vertical = notepad->vinterval;

	_notepad_alignment(notepad->alignment,
			notepad->vertical_margin,
			notepad->horizontal_margin,
			&valid_rect);
	if(notepad->string)
	{
		first_code = _get_first_code(notepad->string);

		if(first_code || notepad->arabic_string)
		{
			if(0x10 == *notepad->string)
			{
				offset = 3;
			}
			else if(0x20 > *notepad->string)
			{
				offset = 1;
			}

			if(NULL == notepad->arabic_string)
			{
				len = strlen(notepad->string + offset);
				string = (char *)GUI_MALLOC(len + 5);
				if(NULL == string)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}

				memset(string ,0, len + 5);
				strcpy(string, notepad->string);
			}
			else
			{
				len = strlen(notepad->arabic_string + offset);
				string = (char *)GUI_MALLOC(len + 5);
				if(NULL == string)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}
				memset(string, 0, len + 5);
				string[0] = notepad->string[0];
				strcpy(string + offset, notepad->arabic_string + offset);
			}

			if(notepad->text_offset)
			{
				if(3 == offset)
				{
					string[notepad->text_offset - offset] = 0x10;
					string[notepad->text_offset - offset + 1] = 0x0;
					string[notepad->text_offset - offset + 2] = notepad->string[2];
				}
				else if(1 == offset)
				{
					string[notepad->text_offset - offset] = notepad->string[0];
				}
			}

			if(notepad->text_offset)
			{
				ptr = string + notepad->text_offset - offset;
			}
			else
			{
				ptr = string + notepad->text_offset;
			}

			_vertical_alignment(notepad->alignment,
					ptr,
					&valid_rect,
					&interval);

			gxgdi_draw_string(screen,
					ptr,
					(GAL_Rect*)(&valid_rect),
					&interval,
					notepad->alignment,
					color,
					PARAGRAPH_MODE);

			GUI_FREE(string);
		}
		else
		{
			if(notepad->epg_flag)
			{
				string = (char *)GUI_MALLOC(strlen(notepad->string) + 5);
				if(NULL == string)
				{
					gxgdi_set_font((char *)old_font);
					return (0);
				}

				memset(string, 0, strlen(notepad->string) + 5);
				strcpy(string + 1, notepad->string + notepad->text_offset);
				string[0] = 0x1F;

				_vertical_alignment(notepad->alignment,
						string,
						&valid_rect,
						&interval);

				gxgdi_draw_string(screen,
						string,
						(GAL_Rect*)(&valid_rect),
						&interval,
						notepad->alignment,
						color,
						PARAGRAPH_MODE);
				GUI_FREE(string);

			}
			else
			{
				_vertical_alignment(notepad->alignment,
						notepad->string + notepad->text_offset,
						&valid_rect,
						&interval);
				gxgdi_draw_string(screen,
						notepad->string + notepad->text_offset,
						(GAL_Rect*)(&valid_rect),
						&interval,
						notepad->alignment,
						color,
						PARAGRAPH_MODE);

				if((FILE_DIRECTION_DOWN == notepad->direction) &&
						(notepad->text_offset > FILE_BUFFER_LIMIT))
				{
					memset(notepad->string, 0, notepad->buffer_size * 2);
					GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
					GxCore_Read(notepad->file, notepad->string, notepad->buffer_size, 1);
					notepad->file_position += notepad->buffer_size;
					notepad->text_offset -= notepad->buffer_size;
					notepad->total_num += gdi_text_get_lines((void *)notepad->string,
							(GAL_Rect *)&valid_rect,
							&interval);
					notepad->direction = FILE_DIRECTION_NO;
					if((gdi_text_get_lines(notepad->string + notepad->text_offset, (GAL_Rect *)&valid_rect, &interval) <= notepad->visible_lines) &&
							(notepad->file))
					{
						GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
						memset(notepad->string + notepad->buffer_size, 0, notepad->buffer_size);
						GxCore_Read(notepad->file,
								notepad->string + notepad->buffer_size,
								notepad->buffer_size,
								1);
						notepad->string[notepad->buffer_size * 2] = '\0';
					}
					gxgdi_draw_string(screen,
							notepad->string + notepad->text_offset,
							(GAL_Rect*)(&valid_rect),
							&interval,
							notepad->alignment,
							color,
							PARAGRAPH_MODE);
				}
				else if(FILE_DIRECTION_UP == notepad->direction)
				{
					notepad->direction = FILE_DIRECTION_NO;
				}

			}
		}
	}

	if(notepad->active_line >= 0) {
		_draw_notepad_line(notepad, notepad->active_line);
	} else if(notepad->active_array) {
		for(index = 0; index < MAX_ACTIVE_LINE; index++) {
			if(0 == notepad->active_array[index]) {
				break;
			}
			_draw_notepad_line(notepad, notepad->active_array[index]);
		}
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		if((GUI_TRUE == notepad->autohide_scroll) &&
				(0 == strcasecmp("scrollbar", child->classname)))
		{
			if((notepad->line_num * gdi_get_charheight()) <= (self->rect.h))
			{
				;
			}
			else
			{
				widget_draw(child);
			}
		}
		else
		{
			widget_draw(child);
		}

		child = widget_get_nextsibling(child);
	}

	gxgdi_set_font((char *)old_font);

	return (0);
}

static int notepad_event(GuiWidget *self, void *data)
{
	// TODO:
	return (0);
}

static int _convert_utf8_to_unicode(const unsigned char* pInput, unsigned int *Unic, int utfbytes)
{
	char b1, b2, b3, b4, b5, b6;
	unsigned char *pOutput = (unsigned char *) Unic;

	*Unic = 0x0;

	switch (utfbytes)
	{
	case 1:
		*pOutput = *pInput;
		return 1;
	case 2:
		b1 = *pInput;
		b2 = *(pInput + 1);
		if ((b2 & 0xE0) != 0x80)
			return 0;
		*pOutput     = (b1 << 6) + (b2 & 0x3F);
		*(pOutput+1) = (b1 >> 2) & 0x07;
		return 2;
	case 3:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80))
			return 0;
		*pOutput     = (b2 << 6) + (b3 & 0x3F);
		*(pOutput+1) = (b1 << 4) + ((b2 >> 2) & 0x0F);
		return 2;
	case 4:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80))
			return 0;
		*pOutput     = (b3 << 6) + (b4 & 0x3F);
		*(pOutput+1) = (b2 << 4) + ((b3 >> 2) & 0x0F);
		*(pOutput+2) = ((b1 << 2) & 0x1C)  + ((b2 >> 4) & 0x03);
		return 3;
	case 5:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80))
			return 0;
		*pOutput     = (b4 << 6) + (b5 & 0x3F);
		*(pOutput+1) = (b3 << 4) + ((b4 >> 2) & 0x0F);
		*(pOutput+2) = (b2 << 2) + ((b3 >> 4) & 0x03);
		*(pOutput+3) = (b1 << 6);
		return 4;
	case 6:
		b1 = *pInput;
		b2 = *(pInput + 1);
		b3 = *(pInput + 2);
		b4 = *(pInput + 3);
		b5 = *(pInput + 4);
		b6 = *(pInput + 5);
		if (((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)
				|| ((b4 & 0xC0) != 0x80) || ((b5 & 0xC0) != 0x80)
				|| ((b6 & 0xC0) != 0x80))
			return 0;
		*pOutput     = (b5 << 6) + (b6 & 0x3F);
		*(pOutput+1) = (b5 << 4) + ((b6 >> 2) & 0x0F);
		*(pOutput+2) = (b3 << 2) + ((b4 >> 4) & 0x03);
		*(pOutput+3) = ((b1 << 6) & 0x40) + (b2 & 0x3F);
		return 4;
	default:
		break;
	}

	return 0;
}

static unsigned short _convert_unicode_to_gb(unsigned short wc)
{
	int i = 0;

	encoding_node *encoding = gdi_get_local();

	unsigned int (*table)[2] = encoding->table;

	for(i = 0; i < encoding->end; i++)
	{
		if(wc == table[i][1])
		{
			return table[i][0];
		}
	}

	return 0;
}

static int _get_utf8_bytes(const char ch)
{
	if(((ch) & 0x80) == UTF8_0XXXXXXX)
		return 1;
	else if(((ch) & 0xE0) == UTF8_110XXXXX)
		return 2;
	else if(((ch) & 0xF0) == UTF8_1110XXXX)
		return 3;
	else if(((ch) & 0xF8) == UTF8_11110XXX)
		return 4;
	else if(((ch) & 0xFC) == UTF8_111110XX)
		return 5;
	else
		return 6;
}

static char *_convert_utf8str_to_gb(const char *str)
{
	int length = strlen(str);
	int pos = 0, off = 0, i = 0, outbytes = 0;
	char *str_pos = NULL;
	char *pstr = NULL;
	unsigned char tmp_str[10] = {0};
	unsigned short gbcode = 0;
	unsigned int unicode = 0;

	pstr = (char *)GUI_MALLOC(length * 2 + 2);
	memset(pstr, 0, length * 2);

	if(NULL == gdi_get_local())
	{
		strcpy(pstr, str);
		return (pstr);
	}

	str_pos = pstr;

	while(*str != '\0')
	{
		off = _get_utf8_bytes(*str);
		pos += off;

		if(pos > length)
			break;

		for(i = 0; i < off; i++)
		{
			tmp_str[i] = *str++;
		}
		tmp_str[off] = '\0';

		outbytes = _convert_utf8_to_unicode(tmp_str, &unicode, off);
		if(outbytes == 1)
		{
			*str_pos++ = tmp_str[0];
		}
		else if(outbytes == 2)
		{
			gbcode = _convert_unicode_to_gb(unicode);
			if(gbcode != 0)
			{
				*str_pos++ = (char)((gbcode >> 8) & 0xff);
				*str_pos++ = (char)(gbcode & 0xff);
			}
		}
	}
	*str_pos = '\0';

	return pstr;
}

static int _check_str_utf8(const char* str,long length)
{
	int i;
	int nBytes = 0;
	unsigned char chr;
	bool bAllAscii = 1;
	for(i = 0; i < length; i++)
	{
		chr= *(str+i);
		if ((chr & 0x80) != 0)
			bAllAscii = 0;
		if (nBytes == 0)
		{
			if (chr >= 0x80)
			{
				if (chr >= 0xFC && chr <= 0xFD)
					nBytes = 6;
				else if (chr >= 0xF8)
					nBytes = 5;
				else if (chr >= 0xF0)
					nBytes = 4;
				else if (chr >= 0xE0)
					nBytes = 3;
				else if (chr >= 0xC0)
					nBytes = 2;
				else
				{
					return 0;
				}
				nBytes--;
			}
		}
		else
		{
			if ((chr&0xC0) != 0x80)
			{
				return 0;
			}
			nBytes--;
		}
	}

	if (bAllAscii)
	{
		return 0;
	}
	return 1;
}

static void _parse_active_array(char *str, int *active_array)
{
	char *pstr = str, *tmp_str = NULL, *ptr = NULL;
	int i = 0;

	memset(active_array, 0, sizeof(int) * MAX_ACTIVE_LINE);
	ptr = tmp_str = GUI_MALLOCZ(strlen(pstr) + 10);
	if(NULL == tmp_str) {
		return;
	}

	while(*pstr) {
		if(*pstr == '[') {
			pstr++;
			break;
		}
		pstr++;
	}

	while(*pstr) {
		*ptr = *pstr;
		ptr++;
		pstr++;

		if(*pstr == ']') {
			break;
		}
	}

	*ptr = '\0';
	ptr = strtok(tmp_str, ",");
	active_array[i] = atoi(ptr);
	i++;

	while(ptr) {
		ptr = strtok(NULL, ",");
		if(NULL == ptr) {
			break;
		}
		active_array[i] = atoi(ptr);
		i++;
		if(i == MAX_ACTIVE_LINE) {
			break;
		}
	}

	GUI_FREE(tmp_str);
}

static int notepad_set_property(GuiWidget *self, char *name, void *data)
{
	GuiNotepad *notepad = NULL;
	char *p_str = NULL;
	struct stat buf;
	GUI_Rect valid_rect = {0};
	int ret = 0, line_count = 0, offset = 0, i = 0, text_position = 0, prev_position = 0;
	int text_string_off = 0, size = 0, read_offset = 0, last_position = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL, *current_font = NULL;
	GAL_Interval interval = {0};

	if((NULL == self) || (NULL == name))
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	old_font = gxgdi_set_font((char *)(notepad->font));

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	valid_rect = self->rect;
	_notepad_alignment(notepad->alignment,
			notepad->vertical_margin,
			notepad->horizontal_margin,
			&valid_rect);

	if(0 == strcasecmp("line_up", name))
	{
		if(((0 == notepad->file) && (NULL == notepad->string)) ||
		   (NULL == notepad->line_pos))
		{
			return (0);
		}

		if(data)
		{
			line_count = (int)(*(int *)data);
			if((line_count < 0) || (line_count > notepad->visible_lines))
			{
				gxgdi_set_font((char *)old_font);
				return (0);
			}
		}

		if((notepad->line_num <= 0) && ((notepad->line_num - line_count)  < 0))
		{
			line_count = notepad->line_num;
			notepad->line_num = 0;
			gxgdi_set_font((char *)old_font);
			return (0);
		}

		if((notepad->line_num >= line_count) && notepad->line_num)
		{
			notepad->line_num -= line_count;
			notepad->line_index -= line_count;
		}
		else
		{
			notepad->line_num = 0;
			notepad->text_offset = 0;
			widget_set_update(self, GUI_TRUE);
			gxgdi_set_font((char *)old_font);
			return (0);
		}

		notepad->text_offset = notepad->line_pos[notepad->line_num];
		if(notepad->file)
		{
			if(notepad->text_offset < notepad->file_position)
			{
				notepad->file_position -= notepad->buffer_size;
				if(notepad->file_position < 0)
					notepad->file_position = 0;
				memset(notepad->string, 0, notepad->buffer_size * 2);
				GxCore_Seek(notepad->file, notepad->file_position, SEEK_SET);
				GxCore_Read(notepad->file, notepad->string, notepad->buffer_size * 2, 1);
			}
			notepad->text_offset -= notepad->file_position;
		}

		if((notepad->text_offset < 0) || (notepad->text_offset > strlen(notepad->string)))
		{
			notepad->text_offset = 0;
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("line_down", name))
	{
		if((0 == notepad->file) && (NULL == notepad->string))
		{
			return (0);
		}

		if((notepad->file) && ((notepad->file_position + notepad->text_offset) >= notepad->file_size - 1))
			return (0);

		if(data)
		{
			line_count = (int)(*(int *)data);
			if((line_count < 0) ||
			   (line_count > notepad->visible_lines) ||
			   (line_count >= notepad->total_num))
			{
				gxgdi_set_font((char *)old_font);
				return (0);
			}
		}
		if(line_count >= FILE_MAX_BUFFER)
		{
			gxgdi_set_font((char *)old_font);
			return (0);
		}

		i = 0;
		notepad->line_index = notepad->line_num;
		prev_position = notepad->text_offset;
		while(i < line_count)
		{
			if((notepad->file) &&
					(notepad->line_index > notepad->total_num))
			{
				notepad->direction = FILE_DIRECTION_DOWN;
			}
			else if(notepad->file &&
					((((notepad->line_index + notepad->visible_lines) > notepad->total_num)) ||
					 ('\0' == notepad->string[0]) ||
					 ('\0' == notepad->string[notepad->text_offset + text_string_off])))
			{
				GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
				memset(notepad->string + notepad->buffer_size, 0, notepad->buffer_size);
				if(notepad->text_offset > FILE_BUFFER_LIMIT)
				{
					read_offset = 0;
					notepad->text_offset -= FILE_BUFFER_LIMIT;
					notepad->file_position += notepad->buffer_size;
					memset(notepad->string, 0, 2 * notepad->buffer_size);
				}
				else
				{
					read_offset = notepad->buffer_size;
				}

				GxCore_Read(notepad->file,
						notepad->string + read_offset,
						notepad->buffer_size,
						1);
			}
			text_string_off = _notepad_calc_offset( notepad->string,
					&(notepad->string[notepad->text_offset]),
					&interval,
					valid_rect.w,
					notepad->alignment,
					1);
			if(0 > text_string_off) {
				text_string_off = 0;
			}
			if(('\0' == notepad->string[notepad->text_offset + text_string_off]) &&
					((notepad->file) && (notepad->line_index + notepad->visible_lines) > notepad->total_num))
			{
				widget_set_update(self, GUI_TRUE);
				gxgdi_set_font((char *)old_font);
				return (0);
			}
			notepad->text_offset += text_string_off;
			if((0 == text_string_off) &&
					((notepad->file) && (notepad->line_index + notepad->visible_lines) > notepad->total_num))
			{
				if(0 == ((notepad->line_index + i) % FILE_MAX_BUFFER))
				{
					notepad->line_pos = (int *)GUI_REALLOC(notepad->line_pos,
							FILE_MAX_BUFFER * sizeof(int) *
							((notepad->line_index +i) / FILE_MAX_BUFFER + 1));
					if(NULL == notepad->line_pos)
					{
						gxgdi_set_font((char *)old_font);
						return (1);
					}

					memset(notepad->line_pos +
							FILE_MAX_BUFFER *
							((notepad->line_index + i) / FILE_MAX_BUFFER),
							0,
							FILE_MAX_BUFFER * sizeof(int));
					notepad->temp_line = ((notepad->line_index + i) / FILE_MAX_BUFFER + 1) * FILE_MAX_BUFFER;
				}
				notepad->line_pos[notepad->line_index + i] = notepad->text_offset;
				gxgdi_set_font((char *)old_font);
				notepad->text_offset = prev_position;
				notepad->line_num -= i;
				notepad->line_index -= i;
				return (0);
			}

			if(NULL == notepad->line_pos)
			{
				notepad->line_pos = (int *)GUI_MALLOC(FILE_MAX_BUFFER * sizeof(int));
				if(NULL == notepad->line_pos)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}
				memset(notepad->line_pos, 0, FILE_MAX_BUFFER * sizeof(int));
				notepad->temp_line = FILE_MAX_BUFFER;
			}
			else
			{
				if(0 == ((notepad->line_index + 1) % FILE_MAX_BUFFER))
				{
					notepad->line_pos = (int *)GUI_REALLOC(notepad->line_pos,
							FILE_MAX_BUFFER * sizeof(int) *
							((notepad->line_index + 1) / FILE_MAX_BUFFER + 1));
					if(NULL == notepad->line_pos)
					{
						gxgdi_set_font((char *)old_font);
						return (1);
					}

					memset(notepad->line_pos +
							FILE_MAX_BUFFER *
							((notepad->line_index + 1) / FILE_MAX_BUFFER),
							0,
							FILE_MAX_BUFFER * sizeof(int));
					notepad->temp_line = ((notepad->line_index + 1) / FILE_MAX_BUFFER + 1) * FILE_MAX_BUFFER;
				}
			}

			if(notepad->line_pos[notepad->line_index] == (notepad->file_position + notepad->text_offset))
			{
				break;
			}
			else
			{
				notepad->line_index ++;
				notepad->line_num ++;
				i++;
				notepad->line_pos[notepad->line_index] = notepad->file_position + notepad->text_offset;
			}

			if('\0' == notepad->string[offset])
			{
				break;
			}
		}

		gdi_lock();
		if((gdi_text_get_lines(notepad->string + notepad->text_offset, (GAL_Rect *)&valid_rect, &interval) <= notepad->visible_lines) &&
				(notepad->file))
		{
			GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
			memset(notepad->string + notepad->buffer_size, 0, notepad->buffer_size);
			if(notepad->text_offset > FILE_BUFFER_LIMIT)
			{
				read_offset = 0;
				notepad->text_offset -= FILE_BUFFER_LIMIT;
				memset(notepad->string, 0, 2 * notepad->buffer_size);
				notepad->file_position += FILE_BUFFER_LIMIT;
			}
			else
			{
				read_offset = notepad->buffer_size;
			}

			GxCore_Read(notepad->file,
					notepad->string + read_offset,
					notepad->buffer_size,
					1);
			notepad->string[notepad->buffer_size * 2] = '\0';
		}
		gdi_unlock();

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("page_up", name))
	{
		if(((0 == notepad->file) && (NULL == notepad->string)) ||
		   (NULL == notepad->line_pos))
		{
			return (0);
		}

		line_count = valid_rect.h / (gdi_get_charheight() + notepad->vinterval);

		if(notepad->line_num  <= (line_count + 1))
		{
			notepad->line_num = 0;
			notepad->text_offset = 0;
			widget_set_update(self, GUI_TRUE);
			gxgdi_set_font((char *)old_font);
			return (0);
		}
		else
		{
			notepad->line_num -= line_count;
		}

		if(NULL == notepad->line_pos)
		{
			notepad->line_pos = (int *)GUI_MALLOC(FILE_MAX_BUFFER * sizeof(int));
			if(NULL == notepad->line_pos)
			{
				gxgdi_set_font((char *)old_font);
				return (1);
			}
			memset(notepad->line_pos, 0, FILE_MAX_BUFFER * sizeof(int));
		}
		else
		{
			if(0 == ((notepad->line_index + 1) % FILE_MAX_BUFFER))
			{
				notepad->line_pos = (int *)GUI_REALLOC(notepad->line_pos,
						FILE_MAX_BUFFER * sizeof(int) *
						((notepad->line_index + 1) / FILE_MAX_BUFFER + 1));
				if(NULL == notepad->line_pos)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}

				memset(notepad->line_pos +
						FILE_MAX_BUFFER *
						((notepad->line_index + 1) / FILE_MAX_BUFFER),
						0,
						FILE_MAX_BUFFER * sizeof(int));
			}
		}
		notepad->text_offset = notepad->line_pos[notepad->line_num];
		if(notepad->file)
		{
			if(notepad->text_offset < notepad->file_position)
			{
				notepad->file_position -= notepad->buffer_size;
				if(notepad->file_position < 0)
					notepad->file_position = 0;
				memset(notepad->string, 0, notepad->buffer_size * 2);
				GxCore_Seek(notepad->file, notepad->file_position, SEEK_SET);
				GxCore_Read(notepad->file, notepad->string, notepad->buffer_size * 2, 1);
			}
			notepad->text_offset -= notepad->file_position;
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("page_down", name))
	{
		if((0 == notepad->file) && (NULL == notepad->string))
		{
			return (0);
		}

		if((notepad->file) && ((notepad->file_position + notepad->text_offset) >= notepad->file_size))
			return (0);

		line_count = valid_rect.h / (gdi_get_charheight() + notepad->vinterval);

		notepad->line_index = notepad->line_num;


		text_position = offset = notepad->text_offset;

		i = 0;
		last_position = notepad->text_offset;
		while(i < line_count)
		{
			if((notepad->file) &&
					(notepad->line_index > notepad->total_num))
			{
				notepad->direction = FILE_DIRECTION_DOWN;
			}

			if((notepad->file) &&
					(((notepad->line_index + notepad->visible_lines + 1) >= notepad->total_num) ||
					 (('\0' == notepad->string[0])) ||
					 ('\0' == notepad->string[notepad->text_offset + text_string_off])))
			{
				GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
				memset(notepad->string + notepad->buffer_size, 0, notepad->buffer_size);
				GxCore_Read(notepad->file,
						notepad->string + notepad->buffer_size,
						notepad->buffer_size,
						1);
			}
			text_string_off= _notepad_calc_offset( notepad->string,
					&(notepad->string[notepad->text_offset]),
					&interval,
					valid_rect.w,
					notepad->alignment,
					1);
			if(text_string_off > 0) {
				offset += text_string_off;
			}
			notepad->text_offset = offset;
			if(NULL == notepad->line_pos)
			{
				notepad->line_pos = (int *)GUI_MALLOC(FILE_MAX_BUFFER * sizeof(int));
				if(NULL == notepad->line_pos)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}
				memset(notepad->line_pos, 0, FILE_MAX_BUFFER * sizeof(int));
			}
			else
			{
				if(0 == ((notepad->line_index + 1) % FILE_MAX_BUFFER))
				{
					notepad->line_pos = (int *)GUI_REALLOC(notepad->line_pos,
							FILE_MAX_BUFFER * sizeof(int) *
							((notepad->line_index + 1) / FILE_MAX_BUFFER + 1));
					if(NULL == notepad->line_pos)
					{
						gxgdi_set_font((char *)old_font);
						return (1);
					}

					memset(notepad->line_pos +
							FILE_MAX_BUFFER *
							((notepad->line_index + 1) / FILE_MAX_BUFFER),
							0,
							FILE_MAX_BUFFER * sizeof(int));
				}
			}

			if((notepad->line_pos[notepad->line_index] == notepad->file_position + offset) ||
			   ('\0' == notepad->string[offset]))
			{
				notepad->line_index -= i;
				notepad->line_num -= i;
				break;
			}
			else
			{
				notepad->line_index++;
				notepad->line_num++;
				notepad->line_pos[notepad->line_index] = notepad->file_position + offset;
				i++;
			}
		}
		if(i < line_count) {
			notepad->text_offset = last_position;
			return (0);
		}

		if('\0' != notepad->string[offset])
		{
			notepad->text_offset = offset;
		}
		else
		{
			notepad->text_offset = text_position;
		}

		gdi_lock();
		if((gdi_text_get_lines(notepad->string + notepad->text_offset, (GAL_Rect *)&valid_rect, &interval) <= notepad->visible_lines) &&
				(notepad->file))
		{
			GxCore_Seek(notepad->file, notepad->file_position + notepad->buffer_size, SEEK_SET);
			memset(notepad->string + notepad->buffer_size, 0, notepad->buffer_size);
			GxCore_Read(notepad->file,
					notepad->string + notepad->buffer_size,
					notepad->buffer_size,
					1);
			notepad->string[notepad->buffer_size * 2] = '\0';
			if(notepad->text_offset > notepad->buffer_size)
				notepad->direction = FILE_DIRECTION_DOWN;
		}
		gdi_unlock();
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("file", name))
	{
		p_str = (char *)data;

		if((notepad->save_context) && (0 == strcasecmp(p_str, notepad->file_path)))
		{
			return (0);
		}
		else if((notepad->file_path) && (0 != strcasecmp(p_str, notepad->file_path)))
		{
			notepad->save_context = GUI_FALSE;
			GUI_FREE(self->object);
			notepad = (GuiNotepad *)GUI_MALLOCZ(sizeof(GuiNotepad));
			if(NULL == notepad)
				return 1;
			self->object = (void *)notepad;
		}

		if(notepad->file)
		{
			GxCore_Close(notepad->file);
			notepad->file = 0;
		}
		notepad->file_path = p_str;
		if(notepad->file_path)
		{
			ret = stat(notepad->file_path, &buf);
			if (ret < 0)
			{
				gxgdi_set_font((char *)old_font);
				return (1);
			}
		}

		if(notepad->adjust_string) {
			GUI_FREE(notepad->string);
			notepad->adjust_string = GUI_FALSE;
		}
		notepad->file = GxCore_Open((char *)notepad->file_path, "r");
		if(notepad->file)
		{
			GUI_FREE(notepad->string);
			notepad->string = (char *)GUI_MALLOC(notepad->buffer_size * 2 + 2);
			if(NULL == notepad->string)
			{
				GxCore_Close(notepad->file);
				notepad->file = 0;
				gxgdi_set_font((char *)old_font);
				return (1);
			}

			memset(notepad->string, 0, notepad->buffer_size * 2);
			notepad->file_size = buf.st_size;
			GxCore_Read(notepad->file, notepad->string, notepad->buffer_size, 1);
			notepad->string[notepad->buffer_size] = '\0';

			current_font = gxgdi_get_font();
			if(gxgdi_is_arabic((const unsigned char *)notepad->string) &&
					(GB_FAMILY != current_font->family))
			{
				/*Deal with EPG Arabic string*/
				valid_rect = self->rect;

				_notepad_alignment(notepad->alignment,
						notepad->vertical_margin,
						notepad->horizontal_margin,
						&valid_rect);

				interval.horizontal = notepad->hinterval;
				interval.vertical = notepad->vinterval;
				gdi_lock();
				notepad->total_num = gdi_text_get_lines((void *)notepad->string,
						(GAL_Rect *)&valid_rect,
						&interval);
				gdi_unlock();

				size = strlen(notepad->string) + (notepad->total_num + 100);
				GUI_FREE(notepad->arabic_string);
				if(size < (notepad->buffer_size * 2 + 2))
					size = notepad->buffer_size * 2 + 2;
				notepad->arabic_string = (char *)GUI_MALLOCZ(size);
				if(NULL == notepad->arabic_string)
				{
					gxgdi_set_font((char *)old_font);
					return (1);
				}

				gdi_arabic_multilines(notepad->string,
						notepad->arabic_string,
						self->rect.w - notepad->horizontal_margin);
				GUI_FREE(notepad->string);
				notepad->string = notepad->arabic_string;
				if(strlen(notepad->string) > notepad->file_size)
				{
					notepad->file_size = strlen(notepad->string);
				}
				notepad->arabic_string = NULL;
			}
			else
			{
				GUI_FREE(notepad->arabic_string);
			}

			if ((current_font->family == GB_FAMILY) && _check_str_utf8(notepad->string, 100))
			{
				gdi_set_local("gb2312");

				p_str = _convert_utf8str_to_gb((char *)notepad->string);
				memset(notepad->string, 0, notepad->buffer_size * 2);
				memcpy(notepad->string, p_str, strlen(p_str));
				GUI_FREE(p_str);
				p_str = NULL;

				gdi_set_local(NULL);
			}

			if(notepad->file)
			{
				//GxCore_Close(notepad->file);
				//notepad->file = 0;
			}

			valid_rect = self->rect;

			_notepad_alignment(notepad->alignment,
					notepad->vertical_margin,
					notepad->horizontal_margin,
					&valid_rect);

			interval.horizontal = notepad->hinterval;
			interval.vertical = notepad->vinterval;
			gdi_lock();
			notepad->total_num = gdi_text_get_lines((void *)notepad->string,
					(GAL_Rect *)&valid_rect,
					&interval);
			gdi_unlock();
			notepad->visible_lines = self->rect.h / gdi_get_charheight();
			notepad->file_position = 0;
			notepad->text_offset = 0;
			notepad->line_index = 0;
			GUI_FREE(notepad->line_pos);
		}
		else
		{
			widget_set_update(self, GUI_TRUE);
			return (1);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("string", name))
	{
		p_str = (char *)data;
		notepad->save_context = GUI_FALSE;
		if(notepad->adjust_string) {
			GUI_FREE(notepad->string);
			notepad->adjust_string = GUI_FALSE;
		}

		if(p_str)
		{
			notepad->string  = p_str;   //Modify for EPG information
		}
		else
		{
			notepad->string = NULL;
		}

		notepad->text_offset = 0;
		notepad->line_num = 0;
		notepad->line_index = 0;
		GUI_FREE(notepad->line_pos);

		if(notepad->file)
		{
			GxCore_Close(notepad->file);
			notepad->file = 0;
		}

		valid_rect = self->rect;

		if(NULL == notepad->string)
		{
			gxgdi_set_font((char *)old_font);
			return (0);
		}
		_notepad_alignment(notepad->alignment,
				notepad->vertical_margin,
				notepad->horizontal_margin,
				&valid_rect);
		interval.horizontal = notepad->hinterval;
		interval.vertical = notepad->vinterval;
		gdi_lock();
		notepad->total_num = gdi_text_get_lines((void *)notepad->string,
				(GAL_Rect *)&(valid_rect),
				&interval);
		gdi_unlock();

		notepad->visible_lines = self->rect.h / gdi_get_charheight();
		current_font = gxgdi_get_font();
		if((gxgdi_is_arabic((const unsigned char *)notepad->string) ||
		    ((notepad->alignment & 0x3) == GUI_TA_RIGHT)) &&
		    (GB_FAMILY != current_font->family))
		{
			/*Deal with EPG Arabic string*/
			size = strlen(notepad->string) + (notepad->total_num + 100);
			GUI_FREE(notepad->arabic_string);
			notepad->arabic_string = (char *)GUI_MALLOCZ(size);
			if(NULL == notepad->arabic_string)
			{
				gxgdi_set_font((char *)old_font);
				return (1);
			}

			gdi_arabic_multilines(notepad->string,
					notepad->arabic_string,
					self->rect.w - notepad->horizontal_margin);
			notepad->string = notepad->arabic_string;
			if(strlen(notepad->string) > notepad->file_size)
			{
				notepad->file_size = strlen(notepad->string);
			}
			notepad->arabic_string = NULL;
			notepad->adjust_string = GUI_TRUE;
		}
		else
		{
			GUI_FREE(notepad->arabic_string);
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("active", name))
	{
		if(data)
		{
			line_count = (int)(*(int *)data);

			notepad->active_line = line_count;

			widget_set_update(self, GUI_TRUE);
		}
	}
	else if((0 == strcasecmp("active_array", name)) && (data)) {
		_parse_active_array(data, notepad->active_array);
		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("cur_pos", name))
	{
		if(NULL == notepad->string)
		{
			gxgdi_set_font((char *)old_font);
			return (-1);
		}
		text_position = *((int *)(data));

		if(notepad->file)
		{
			notepad->file_position = (text_position / notepad->buffer_size) * notepad->buffer_size;
			text_position = text_position % notepad->buffer_size;
		}

		notepad->text_offset = text_position;
		if((NULL != notepad->string) && (notepad->file))
		{
			memset(notepad->string, 0, notepad->buffer_size * 2);
			GxCore_Seek(notepad->file, notepad->file_position, SEEK_SET);
			GxCore_Read(notepad->file, notepad->string, notepad->buffer_size * 2, 1);
			widget_set_update(self, GUI_TRUE);
		}
	}
	else if(0 == strcasecmp("current_line", name))
	{
		if(data)
		{
			line_count = (int)(*(int *)data);
		}

		if(0 == line_count)
		{
			notepad->text_offset = 0;
			notepad->line_num = 0;
			widget_set_update(self, GUI_TRUE);
			gxgdi_set_font((char *)old_font);
			return (0);
		}

		if((line_count < 0) || (line_count >= notepad->total_num))
		{
			gxgdi_set_font((char *)old_font);
			return (1);
		}

		line_count = line_count - notepad->line_num;

		notepad->line_num += line_count;

		offset = _notepad_calc_offset( notepad->string,
				&(notepad->string[notepad->text_offset]),
				&interval,
				valid_rect.w,
				notepad->alignment,
				line_count);
		if(line_count >= 0)
		{
			notepad->text_offset += offset;
		}
		else
		{
			notepad->text_offset -= offset;
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("epg", name))
	{
		p_str = (char *)data;
		if(0 == strcasecmp("true", p_str))
		{
			notepad->epg_flag = GUI_TRUE;
		}
		else
		{
			notepad->epg_flag = GUI_FALSE;
		}
	}
	else if(0 == strcasecmp("backcolor", name))
	{
		p_str = (char *)data;
		widget_set_property(self, "backcolor", data);
		if(p_str)
		{
			getColorFromString((const char *)p_str, self->back_color);
		}
		else
		{
			gxgdi_set_font((char *)old_font);
			return (1);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("forecolor", name))
	{
		p_str = (char *)data;
		widget_set_property(self, "forecolor", data);
		if(p_str)
		{
			getColorFromString((const char *)p_str, self->fore_color);
		}
		else
		{
			gxgdi_set_font((char *)old_font);
			return (1);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("save_context", name))
	{
		p_str = (char *)data;

		if((p_str) && (0 == strcasecmp("null", p_str)))
		{
			notepad->save_context = GUI_FALSE;
			GUI_FREE(notepad->line_pos);
			GUI_FREE(notepad->arabic_string);
		}
		else
		{
			notepad->save_context = GUI_TRUE;
		}
	}
	else
	{
		widget_set_property(self, name, (const char *)data);
		widget_set_update(self, GUI_TRUE);
		gxgdi_set_font((char *)old_font);
		return (0);
	}

	gxgdi_set_font((char *)old_font);

	return (0);
}

static int notepad_get_property(GuiWidget *self, char *name, void *data)
{
	GuiNotepad *notepad = NULL;
	GAL_Interval interval = {0};

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	if(0 == strcasecmp("cur_pos", name))
	{
		if(notepad->text_offset + notepad->file_position)
			*((int *)data) = notepad->text_offset + notepad->file_position - 1;
		else
			*((int *)data) = 0;
		if(((notepad->text_offset + notepad->file_position) > (notepad->file_size - 100)) &&
				(gdi_text_get_lines(notepad->string + notepad->text_offset, (GAL_Rect *)&(self->rect), &interval) <= 1))
		{
			if(notepad->file_size)
			{
				*((int *)data) = notepad->file_size;
			}
			else if(notepad->string)
			{
				*((int *)data) = (strlen(notepad->string));
			}
		}
	}
	else if(0 == strcasecmp("total_byte", name))
	{
		if(notepad->file_size)
		{
			*((int *)data) = notepad->file_size;
		}
		else if(notepad->string)
		{
			*((int *)data) = (strlen(notepad->string));
		}
	}
	else if(0 == strcasecmp("total_lines", name))
	{
		if(notepad->string)
		{
			*((int *)data) = notepad->total_num;
		}
	}
	else if(0 == strcasecmp("visible_lines", name))
	{
		if(notepad->string)
		{
			*((int *)data) = notepad->visible_lines;
		}
	}
	else if(0 == strcasecmp("file", name))
	{
		*((char **)data) = notepad->file_path;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int notepad_prepare_image(GuiWidget *self)
{
	GuiNotepad *notepad = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	value = widget_get_property(self, "image");
	if((NULL != value) || (NULL == notepad->img_desc))
	{
		notepad->img_desc = widget_img_load(value);
	}

	return (0);
}

static int notepad_clear_image(GuiWidget *self)
{
	GuiNotepad *notepad = NULL;

	if(NULL == self)
	{
		return (1);
	}

	notepad = (GuiNotepad *)(self->object);
	if(NULL == notepad)
	{
		return (1);
	}

	if(notepad->img_desc)
	{
		widget_img_clear(&(notepad->img_desc));
	}

	return (0);
}

GuiWidgetOps notepad_ops =
{
	.owner = "notepad",
	.create = create_notepad,
	.release = notepad_release,
	.prepare_image = notepad_prepare_image,
	.draw = notepad_draw,
	.clear_image   = notepad_clear_image,
	.event = notepad_event,
	.set = notepad_set_property,
	.get = notepad_get_property
};

