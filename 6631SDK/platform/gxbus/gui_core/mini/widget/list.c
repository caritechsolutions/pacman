#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#include "list.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif
#include "gdi_font.h"
#include "color.h"

static void _free_list_item(void *p)
{
	if(p)
	{
		GxCore_Free(p);
	}
}

static void *list_create(GuiWidget *self)
{
	GuiList *list = NULL;
	int row = 0;

	WIDGET_ADJUST_ACCORDING_PARENT(self->parent, self);

	list = (GuiList *)GxCore_Mallocz(sizeof(GuiList));
	if(NULL == list)
	{
		return (NULL);
	}

	if(NULL == list->list_data)
	{
		list->list_data = hash_new(10, _free_list_item);
		list->count = 0;
	}

	row = widget_get_int(self, "rows", 0);
	if(row)
	{
		int space = 0, height = 0, i = 0;

		list->rect_array = (GAL_Rect *)GxCore_Mallocz(row * sizeof(GAL_Rect));
		if(NULL == list->rect_array)
		{
			GxCore_Free(list);
			return (NULL);
		}

		space = widget_get_int(self, "space", 0);
		height = self->rect.h / row - space;

		while(i < row)
		{
			list->rect_array[i].x = self->rect.x;
			list->rect_array[i].y = i * (self->rect.h / row);
			list->rect_array[i].w = self->rect.w;
			list->rect_array[i].h = height;
			i++;
		}
	}

	list->base = self;
	self->object = (void *)list;
	list->select = -1;

	return ((void *)list);
}

static int list_release(GuiWidget *self)
{
	GuiList *list = NULL;
	int ret = 0;

	list = (GuiList *)(self->object);
	if(NULL == list)
	{
		return (1);
	}

	hash_release(list->list_data);
	if(list->rect_array)
	{
		GxCore_Free(list->rect_array);
	}

	GxCore_Free(list);
	self->object = NULL;

	return (ret);
}

static int *_get_column_content(GuiWidget *self, int *size)
{
	int *array = NULL, array_size = 0, i = 0, index = 0;
	char *value = NULL, *ptr = NULL;
	char str[20] = {0};

	value = (char *)widget_get_property(self, "column");
	if(NULL == value)
	{
		array = GxCore_Mallocz(sizeof(int));
		if(NULL == array)
			return (NULL);

		array[0] = 0;
		*size = 1;
		return (array);
	}
	else
	{
		ptr = value;
		array_size = 1;
		array = GxCore_Mallocz(sizeof(int));
		if(NULL == array)
			return (NULL);

		while(*ptr)
		{
			if('[' == *ptr)
			{
				ptr++;
				break;
			}
			ptr++;
		}

		while(*ptr)
		{
			if(']' == *ptr)
			{
				if(index >= array_size)
				{
					array = GxCore_Realloc(array, sizeof(int) * (index + 1));
					if(NULL == array)
						return (NULL);

					array[index] = atoi(str);
					array_size++;
				}
				break;
			}

			if(',' == *ptr)
			{
				if(index >= array_size)
				{
					array = GxCore_Realloc(array, sizeof(int) * (index + 1));
					if(NULL == array)
						return (NULL);

					array[index] = atoi(str);
					array_size++;
				}
				i = 0;
				index++;
			}
			else if((*ptr >= '0') && ('9' >= *ptr))
			{
				str[i] = *ptr;
				i++;
			}

			ptr++;
		}
	}

	*size = array_size;
	return (array);
}

#define ITEM_STRING						(0)
#define ITEM_IMAGE						(1)

static int _get_column_item(char *content, int index, char **value)
{
	char *str = GxCore_Strdup(content);
	char *item_str = NULL, *ptr = NULL;
	int i = 0, ret = 0;

	item_str = strtok(str, "|");
	if(i == index)
		goto EXIT;
	i++;
	while(item_str)
	{
		item_str = strtok(NULL, "|");
		if(i == index)
			goto EXIT;
		i++;
	}

EXIT:
	if(*item_str == '<')
	{
		item_str++;
		ptr = item_str;
		while(*ptr)
		{
			if('>' == *ptr)
				*ptr = '\0';
			ptr++;
		}
		ret = ITEM_IMAGE;
	}
	else
		ret = ITEM_STRING;

	*value = GxCore_Strdup(item_str);
	GxCore_Free(str);

	return (ret);
}

static int _color_len(const char *str)
{
    const char *pstr = NULL;

    if(NULL == str)
    {
	return (0);
    }

    pstr = str;
    while(*pstr)
    {
	if((',' == *pstr) || (']' == *pstr))
	{
	    break;
	}

	pstr++;
    }

    return (pstr - str);
}

int hex2decimal(const char *string)
{
	int ret = 0;
	char *pstr = NULL;

	if (NULL == string) {
		return (0);
	}

	pstr = (char *)string;

	while (*pstr) {
		if (*pstr >= '0' && *pstr <= '9') {
			ret = ret * 16 + ((int)*pstr - '0');
		} else {
			if (*pstr >= 'A' && *pstr <= 'F') {
				ret = ret * 16 + ((int)*pstr - 'A' + 10);
			} else if (*pstr >= 'a' && *pstr <= 'f') {
				ret = ret * 16 + ((int)*pstr - 'a' + 10);
			}
		}

		pstr++;
	}

	return (ret);
}

static int string2color(const char *input, int *color)
{
	char *pstr = NULL;
	char value[30] = { 0 };
	int i = 0;
	int ret_len = 0;

	pstr = (char *)input;

	while(*pstr)
	{
		if((',' == *pstr) || ('[' == *pstr))
		{
			pstr++;
		}
		else if(']' == *pstr)
		{
			break;
		}
		else if('#' == *pstr)
		{
			pstr++;
			ret_len = _color_len(pstr);
			strncpy(value, pstr, ret_len);
			pstr += ret_len;
			color[i] = hex2decimal(value);
			i++;
		}

		memset(value, 0, 30);
	}

	return (0);
}

static void _draw_item(GuiWidget *self, GuiList *list, int index)
{
	int grid = 0, select = 0, scroll= 0, i = 0;
	int fore_color = 0, item_color[3] = {0}, font_color = 0, key = 0;
	int column_num = 0, *column_array = NULL, select_index = 0, state_index = 0;
	char select_str[10] = {0}, *content = NULL, *item_str = NULL, *value = NULL;
	image_desc *desc = NULL, *item_desc = NULL;
	MiniGUI_Rect *rect_array = NULL;

	grid = widget_get_int(self, "grid", 0);
	select = widget_get_int(self, "select", 0);
	scroll = widget_get_int(self, "scroll", 0);

	if(index <0)
	{
		return;
	}

	value = (char *)widget_get_property(self, "itemcolor");
	if(value)
	{
		string2color((const char *)value, item_color);
	}

	state_index = widget_get_state_index(self);
	if((select == index) && (state_index))
	{
		fore_color = self->fore_color[1];
		font_color = item_color[1];
		value = (char *)widget_get_property(self, (const char *)"itemfocus");
		if(value)
		{
			item_desc = hash_get(minigui_config.image, value);
		}
	}
	else
	{
		fore_color = self->fore_color[0];
		font_color = item_color[0];
		value = (char *)widget_get_property(self, (const char *)"itemunfocus");
		if(value)
		{
			item_desc = hash_get(minigui_config.image, value);
		}
	}

	column_array = _get_column_content(self, &column_num);
	if(NULL == column_array)
		return;

	rect_array = GxCore_Mallocz(column_num * sizeof(MiniGUI_Rect));
	if(NULL == rect_array)
		return;

	select_index = index - list->start_index;
	if(item_desc)
	{
		minigui_draw_image(minigui_config.surface,
						   item_desc,
						   list->rect_array[select_index].x,
						   list->rect_array[select_index].y,
						   list->rect_array[select_index].w,
						   list->rect_array[select_index].h);
	}

	sprintf(select_str, "%d", index);
	content = hash_get(list->list_data, select_str);
	i = 0;
	while(i < column_num)
	{
		rect_array[i].x = list->rect_array[select_index].x;
		rect_array[i].y = self->rect.y + list->rect_array[select_index].y;
		rect_array[i].w = list->rect_array[select_index].w;
		rect_array[i].h = list->rect_array[select_index].h;

		rect_array[i].x = self->rect.x + column_array[i];
		if((i + 1) < column_num)
			rect_array[i].w = column_array[i + 1] - column_array[i] - grid;
		else
			rect_array[i].w =  list->rect_array[select_index].w - column_array[i] - scroll;

		if(NULL == item_desc)
		{
			minigui_fillrect(minigui_config.surface, &rect_array[i], fore_color);
		}

		if(content)
		{
			key = _get_column_item(content, i, &item_str);
			if(ITEM_STRING == key)
			{
				minigui_draw_string(minigui_config.surface,
									item_str,
									&rect_array[i],
									font_color,
									GUI_TA_VCENTRE | GUI_TA_HCENTRE);
			}
			else
			{
				if(item_str)
				{
					desc = hash_get(minigui_config.image, item_str);
					if(desc)
					{
						minigui_draw_image(minigui_config.surface,
										   desc,
										   rect_array[i].x,
										   rect_array[i].y,
										   rect_array[i].w,
										   rect_array[i].h);

					}
				}
			}
		}
		i++;
	}

	GxCore_Free(column_array);
	GxCore_Free(rect_array);
}

static void _draw_scroll(GuiWidget *self)
{
	MiniGUI_Rect rect = {0}, fore_rect = {0};
	int width = 0, color = 0, rows = 0;
	GuiList *list = NULL;

	if(NULL == self)
	{
		return;
	}

	list = (GuiList *)(self->object);
	if(NULL == list)
	{
		return;
	}

	width = widget_get_int(self, "scroll", 0);
	rect.x = self->rect.x + self->rect.w - width;
	rect.w = width;
	rect.y = self->rect.y;
	rect.h = self->rect.h;
	color = self->back_color[2];
	minigui_fillrect(minigui_config.surface, &rect, color);

	rows = widget_get_int(self, "rows", 0);
	fore_rect = rect;
	if(rows < list->count)
	{
		fore_rect.h = rows * fore_rect.h / list->count;
		fore_rect.y = fore_rect.y + (rect.h - fore_rect.h) * list->select / list->count;
	}
	color = self->fore_color[2];
	minigui_fillrect(minigui_config.surface, &fore_rect, color);
}

static int list_draw(GuiWidget *self)
{
	GuiList *list = NULL;
	int ret = 0, i = 0, row = 0, index = 0;
	MiniGUI_Rect rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	list = (GuiList *)(self->object);
	if(NULL == list)
	{
		return (1);
	}

	if(list->update)
	{
		_draw_item(self, list, list->update_index[0]);
		_draw_item(self, list, list->update_index[1]);
	}
	else
	{
		rect.x = self->rect.x;
		rect.y = self->rect.y;
		rect.w = self->rect.w;
		rect.h = self->rect.h;

		index = widget_get_state_index(self);
		minigui_fillrect(minigui_config.surface, &rect, self->back_color[index]);
		row = widget_get_int(self, "rows", 0);
		for(i = list->start_index; i < (list->start_index) + row; i++)
		{
			_draw_item(self, list, i);
		}
	}
	_draw_scroll(self);
	list->update = GUI_FALSE;
	memset(list->update_index, 0, 10);

	return (ret);
}

static int list_set_property(GuiWidget *self, char *name, void *data)
{
	GuiList *list = NULL;
	int ret = 0, new_select = 0, rows = 0;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	list = (GuiList *)(self->object);
	if(NULL == list)
	{
		return (1);
	}

	if(0 == strcasecmp("clear", name))
	{
		hash_release(list->list_data);
		list->list_data = NULL;
		list->count = 0;
	}
	else if(0 == strcasecmp("add", name))
	{
		char index_key[10] = {0};

		if(NULL == list->list_data)
		{
			list->list_data = hash_new(10, _free_list_item);
		}

		sprintf(index_key, "%d", list->count);
		hash_add(list->list_data, index_key, GxCore_Strdup((char *)data));
		list->count++;
	}
	else if(0 == strcasecmp("select", name))
	{
		rows = widget_get_int(self, "rows", 0);
		widget_set_property(self, "select", data);
		new_select = widget_get_int(self, "select", 0);

		if((0 > new_select) || (list->count < new_select))
		{
			return (1);
		}

		if((new_select < list->start_index) ||
		   (new_select >= (list->start_index + rows)))
		{
			list->start_index = new_select - rows + 1;
			if(0 > list->start_index)
				list->start_index = 0;
			list->update = GUI_FALSE;
		}
		else
		{
			list->update = GUI_TRUE;
		}

		if((list->select >= list->start_index) &&
		   (list->select <= (list->start_index + rows)))
		{
			list->update_index[0] = list->select;
		}
		else
		{
			list->update_index[0] = list->select;
		}

		list->update_index[1] = new_select;
		list->select = new_select;
	}
	else if(0 == strcasecmp("update", name))
	{
		list->update = GUI_FALSE;
		widget_set_update(self, GUI_TRUE);
	}

	return (ret);
}

static int list_get_property(GuiWidget *self, char *name, void *data)
{
	int ret = 0;
	GuiList *list = NULL;

	list = (GuiList *)(self->object);
	if(NULL == list)
	{
		return (1);
	}

	if(0 == strcasecmp("select_content", name))
	{
		char *content = NULL;

		if(NULL != list->list_data) {
			content = hash_get(list->list_data, (const char *)widget_get_property(self, "select"));
			if(NULL != content) {
				*(char **)data = content;
			}
		}
	}

	return (ret);
}

GuiWidgetOps list_ops = {
	.owner		=	"list",
	.create		=	list_create,
	.release	=	list_release,
	.draw		=	list_draw,
	.set		=	list_set_property,
	.get		=	list_get_property
};
