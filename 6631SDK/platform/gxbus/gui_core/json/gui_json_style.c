#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_private.h"
#include "gui_json_parse.h"

gx_stack_t *style_stack;

static GuiWidget *_json_new_style(GuiWidget *parent, char *key, char *content)
{
	GuiWidget *widget =NULL;
	static char *class_id = NULL, *name = NULL;

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

	if((NULL != class_id) && (NULL != name))
	{
		widget = widget_new_child(parent, name, class_id, NULL);
		GUI_FREE(class_id);
		GUI_FREE(name);
	}

	return (widget);
}

int gui_json_style(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	static char *key = NULL, *variable = NULL, *key_value = NULL;
	GuiWidget *style_widget = NULL, *style_parent = NULL;

	style_parent = (GuiWidget *)ctx;
	if(NULL == style_parent)
	{
		return (0);
	}

	if(NULL == style_stack)
	{
		style_stack = stack_new(style_stack);
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

			stack_push(style_stack, (void *)key_value);
			break;
		case JSON_T_OBJECT_END:
			key_value = stack_pop(style_stack);
			GxCore_Free(key_value);
			break;
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			key_value = stack_peek(style_stack);
			variable = (char *)value->vu.str.value;
			if(key_value)
			{
				if(0 == strcasecmp(JSON_STYLE, key_value))
				{
					style_widget = _json_new_style(style_parent, key, variable);
				}
				else if(0 == strcasecmp(JSON_PROPERTY, key_value))
				{
					if(style_widget)
					{
						widget_set_property(style_widget, key, variable);
					}
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


status_t gui_json_style_freedata(GuiWidget* style_widget)
{
	widget_del_tree(style_widget);

	return GXCORE_SUCCESS;
}

