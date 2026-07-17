#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_private.h"
#include "gui_json_parse.h"

gx_stack_t *widget_stack = NULL;

static void _json_create_alpha_color(GuiClut *clut, char *key, char *content)
{
	int osd_alpha_color = 0;
	int osd_alpha_value = 0;

	if((NULL == clut) || (NULL == key) || (NULL == content))
	{
		return;
	}

	if(NULL != key && '#' == *content)
	{
		osd_alpha_color = hexToInt((const char *)(key + 1));
	}

	if(NULL != content && '#' == *content)
	{
		osd_alpha_value = hexToInt((const char *)(content + 1));
		gui_set_osdalpha_color(osd_alpha_color, osd_alpha_value);
	}

	return;
}

static void _json_create_interface(GuiCore *gui_core, char *key, void *content)
{
	char *p_str = NULL;
	int value = 0;

	if((NULL == gui_core) || (NULL == key) || (NULL == content))
	{
		return;
	}

	if(0 == strcasecmp(JSON_WIDTH, key))
	{
		value = *((int*)content);
		gui_core->config.width = value;
	}
	else if(0 == strcasecmp(JSON_HEIGHT, key))
	{
		value = *((int*)content);
		gui_core->config.height = value;
	}
	else if(0 == strcasecmp(JSON_BPP, key))
	{
		value = *((int*)content);
		gui_core->config.bpp = value;
	}
	else if(0 == strcasecmp(JSON_MULTCLUT, key))
	{
		p_str = (char *)content;
		GUI_CONFIG_STATE(gui_core->config.bpp8_multclut, p_str);
	}
	else if(0 == strcasecmp(JSON_OSD_TRANS, key))
	{
		p_str = (char *)content;
		if(NULL != p_str && '#' == *p_str)
		{
			gui_core->config.osd_trans = hexToInt((const char *)(p_str + 1));
			gal_set_osd_transparent(gui_core->config.osd_trans, 0);
		}
	}
	else if(0 == strcasecmp(JSON_GUI_TRANS, key))
	{
		p_str = (char *)content;
		if(NULL != p_str && '#' == *p_str)
		{
			gui_core->config.gui_trans = hexToInt((const char *)(p_str + 1));
			gal_set_gui_transparent(gui_core->config.gui_trans);
		}
	}
	else if(0 == strcasecmp(JSON_OSD_ALPHA, key))
	{
		p_str = (char *)content;
		if(NULL != p_str && '#' == *p_str)
		{
			gui_core->config.osd_alpha = hexToInt((const char *)(p_str + 1));
		}
	}

	return;
}

static GuiWidget *_json_new_widget(GuiWidget *parent, char *key, char *content)
{
	GuiWidget *widget =NULL;
	static char *class_id = NULL, *name = NULL, *style_name = NULL;

	if((NULL == parent) || (NULL == key) || (NULL == content))
	{
		return (NULL);
	}

	if(0 == strcasecmp(JSON_CLASS, key))
	{
		class_id = GxCore_Strdup(content);
	}
	else if(0 == strcasecmp(JSON_NAME, key))
	{
		name = GxCore_Strdup(content);
	}
	else if(0 == strcasecmp(JSON_STYLE, key))
	{
		style_name = GxCore_Strdup(content);
	}

	if((NULL != class_id) && (NULL != name) && (NULL != style_name))
	{
		widget = widget_new_child(parent, name, class_id, style_name);
		GUI_FREE(class_id);
		GUI_FREE(name);
		GUI_FREE(style_name);
	}

	return (widget);
}

int gui_json_widget(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	static int freedom_flag = 0;
	static char *key = NULL, *variable = NULL, *key_value = NULL;
	GuiCore *gui_core = NULL;
	static GuiWidget *widget = NULL, *parent = NULL;
	static GuiWidget *free_widget = NULL, *free_parent = NULL;

	gui_core = (GuiCore *)ctx;
	if(NULL == gui_core)
	{
		return (0);
	}

	if(NULL == widget_stack)
	{
		widget_stack = stack_new(widget_stack);
	}

	if(NULL == parent)
	{
		parent = gui.root_widget;
	}
	if(NULL == free_parent)
	{
		free_parent = gui.freedom_widget;
	}

	switch(type) {
		case JSON_T_OBJECT_BEGIN:
			if(key)
			{
				key_value = GxCore_Strdup(key);
			}
			else
			{
				key_value = NULL;
			}
			if((key_value) && (0 == strcasecmp(JSON_CHILDREN, key_value)))
			{
				if(freedom_flag)
				{
					free_parent = free_widget;
				}
				else
				{
					if(widget)
					{
						parent = widget;
					}
				}
			}
			stack_push(widget_stack, (void *)key_value);
			break;
		case JSON_T_OBJECT_END:
			key_value = stack_pop(widget_stack);
			if((key_value) && (0 == strcasecmp(JSON_CHILDREN, key_value)))
			{
				if(freedom_flag)
				{
					free_parent = widget_get_parent(free_parent);
				}
				else
				{
					parent = widget_get_parent(parent);
				}
			}
			GxCore_Free(key_value);
			break;
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			key_value = stack_peek(widget_stack);
			variable = (char *)value->vu.str.value;
			if(key_value)
			{
				if(0 == strcasecmp(JSON_WIDGET, key_value))
				{
					widget = _json_new_widget(parent, key, variable);
					freedom_flag = 0;
				}
				else if(0 == strcasecmp(JSON_FREEDOM, key_value))
				{
					free_widget = _json_new_widget(free_parent, key, variable);
					freedom_flag = 1;
				}
				else if(0 == strcasecmp(JSON_INTERFACE, key_value))
				{
					_json_create_interface(gui_core, key, (void *)variable);
				}
				else if(0 == strcasecmp(JSON_ALPHA_COLOR, key_value))
				{
					_json_create_alpha_color(gui_core->config.clut, key, variable);
				}
				else if(0 == strcasecmp(JSON_PROPERTY, key_value))
				{
					if(widget)
					{
						widget_set_property(widget, key, variable);
					}
				}
				else if(0 == strcasecmp(JSON_SIGNAL, key_value))
				{
					if(widget)
					{
						widget_signal_new(widget, key, variable);
					}
				}
				else
				{
					;
				}
			}
			GUI_FREE(key);
			break;
		case JSON_T_INTEGER:
			if(key_value)
			{
				if(0 == strcasecmp(JSON_INTERFACE, key_value))
				{
					_json_create_interface(gui_core, key, (void *)&value->vu.integer_value);
				}
			}
			GUI_FREE(key);
			break;
		default:
			GUI_FREE(key);
			break;
	}

	return (ret);
}


status_t gui_json_widget_freedata(GuiCore* gui_core)
{
	GuiClut *pclut = NULL, *pnext = NULL;

	if(NULL == gui_core)
	{
		return GXCORE_ERROR;
	}

	pclut = gui_core->config.clut;
	while(pclut)
	{
		pnext = pclut->next;
		GUI_FREE(pclut->palette);
		GUI_FREE(pclut);
		pclut = pnext;
	}

	widget_del_tree(gui_core->root_widget);
	widget_del_tree(gui_core->freedom_widget);
	hash_release(gui.signal_table);  //shencz add

	return GXCORE_SUCCESS;
}


