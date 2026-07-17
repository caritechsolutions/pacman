#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#include "edit_box.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

#define MAX_LINE_STRLEN               (128)
static char string[MAX_LINE_STRLEN] = {0};

static void *edit_box_create(GuiWidget *self)
{
	void *object = NULL;

	WIDGET_ADJUST_ACCORDING_PARENT(self->parent, self);
	return (object);
}

static int edit_box_release(GuiWidget *self)
{
	int ret = 0;

	return (ret);
}


static int edit_box_draw_cursor(GuiWidget *self)
{
	MiniGUI_Rect cursor_rect = {0};
	char *cur_pos = NULL;
	char str_buff[MAX_LINE_STRLEN] = {0};
	char str_buff1[MAX_LINE_STRLEN] = {0};
	char data_buff[MAX_LINE_STRLEN] = {0};
	int string_width = 0, cursor_width = 0;
	int str_buff_width = 0, str_buff_width1 = 0;
	int change_len = 0;
	static int count = 0, len = 0, string_len = 0;
	char *cur_char = NULL;
	int height = 0;

	if(NULL == self)
	{
		return (1);
	}
	cur_pos = (char *)widget_get_property(self, "cursor");
	if(cur_pos)
	{
		height = minigui_get_font_height();
		change_len = atoi(cur_pos);
		string_len = strlen(string);
		string_width = minigui_get_string_pixel(string);

		if(count < MAX_LINE_STRLEN)
		{
			memcpy(str_buff, string, count + 1);
		}

		str_buff_width = minigui_get_string_pixel(str_buff);
		if(string_width < self->rect.w)
		{
			if(0 != change_len)
			{
				memcpy(str_buff1, string, change_len);
				str_buff_width1 = minigui_get_string_pixel(str_buff1);
				cursor_rect.x = self->rect.x + str_buff_width1;
			}
			else
			{
				memcpy(str_buff1, string, 1);
				cursor_rect.x = self->rect.x;
			}
			cur_char = str_buff1 + strlen(str_buff1) - 1;
			cursor_width = minigui_get_string_pixel(cur_char);
			cursor_rect.y = self->rect.y + height;
			count = strlen(string);
		}
		else if(str_buff_width <= self->rect.w && change_len <= count)
		{
			memset(str_buff1, 0, strlen(str_buff1));
			memcpy(str_buff1, string, change_len + 1);
			str_buff_width1 = minigui_get_string_pixel(str_buff1);
			cur_char = str_buff1 + strlen(str_buff1) - 1;
			cursor_width = minigui_get_string_pixel(cur_char);
			cursor_rect.x = self->rect.x + str_buff_width1 - cursor_width;
			cursor_rect.y = self->rect.y + height;
		}
		else
		{
			memcpy(str_buff1, string + count, change_len - count - 1);
			memcpy(data_buff, string + count, string_len - count);
			str_buff_width1 = minigui_get_string_pixel(str_buff1);
			len = strlen(data_buff);
			cur_char = data_buff + len - 1;
			cursor_width = minigui_get_string_pixel(cur_char);
			cursor_rect.x = self->rect.x + str_buff_width1;
			cursor_rect.y = self->rect.y + 2*height;
		}

		cursor_rect.w = cursor_width;
		cursor_rect.h = 2;

		if(((cursor_rect.x + cursor_rect.w) > (self->rect.x + self->rect.w)) && (cur_pos))
		{
			cursor_rect.x = self->rect.x + self->rect.w - cursor_rect.w;
		}

		if(((cursor_rect.x + cursor_rect.w) < self->rect.x) && (cur_pos))
		{
			cursor_rect.x = self->rect.x - cursor_rect.w;
		}

		minigui_fillrect(minigui_config.surface, &cursor_rect, self->back_color[2]);

	}
	return (0);

}


static int edit_box_draw_string(GuiWidget *self)
{
	MiniGUI_Rect rect = {0};
	char *value = NULL;
	int alignment = 0;

	if(NULL == self)
	{
		return (1);
	}

	value = (char *)widget_get_property(self, "string");
	if(value)
	{
		memset(string, 0, strlen(string));
		memcpy(string, value, strlen(value));
		alignment = widget_get_alignment(self);
		rect.x = self->rect.x;
		rect.y = self->rect.y;
		rect.w = self->rect.w;
		rect.h = self->rect.h;
		minigui_draw_string(minigui_config.surface,
							string,
							&rect,
							self->fore_color[0],
							alignment);
	}
	return (0);
}

static int edit_box_draw(GuiWidget *self)
{
	int ret = 0;
	char *value = NULL;
	MiniGUI_Rect rect = {0};
	image_desc *desc = NULL;

	if(NULL == self)
	{
		return (1);
	}

	value = (char *)widget_get_property(self, "img");
	if(value && minigui_config.image)
	{
		desc = hash_get(minigui_config.image, value);
	}
	if(desc)
	{
		minigui_draw_image(minigui_config.surface,
						   desc,
						   self->rect.x,
						   self->rect.y,
						   self->rect.w,
						   self->rect.h);
	}
	else
	{
		rect.x = self->rect.x;
		rect.y = self->rect.y;
		rect.w = self->rect.w;
		rect.h = self->rect.h;

		minigui_fillrect(minigui_config.surface, &rect, self->back_color[0]);
	}

	edit_box_draw_string(self);
	value = (char *)widget_get_property(self, "state");
	if(value && 0 == strcasecmp("focused", value))
	{
		edit_box_draw_cursor(self);
	}

	return (ret);
}

static int edit_box_set_property(GuiWidget *self, char *name, void *data)
{
	GuiEdit_box *edit_box = NULL;
	int ret = 0;

	if(0 == strcasecmp("cursor", name))
	{
		edit_box->cursor_pos = widget_get_int(self, "cursor_pos", 0);
	}
	else
	{
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
	}

    return (ret);
}

static int edit_box_get_property(GuiWidget *self, char *name, void *data)
{
	*((char **)data) = (char *)widget_get_property(self, name);
	return (0);
}

GuiWidgetOps edit_box_ops = {
	.owner      =   "edit_box",
	.create     =   edit_box_create,
	.release    =   edit_box_release,
	.draw       =   edit_box_draw,
	.set        =   edit_box_set_property,
	.get        =   edit_box_get_property
};

