#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_json_parse.h"

gx_stack_t *i18n_stack = NULL;

int gui_json_i18n(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	GuiConfig *config = NULL;
	static char *key = NULL, *variable = NULL, *key_value = NULL;
	static GuiI18n *i18n = NULL;
	static int language_flag = 0;

	config = (GuiConfig *)ctx;
	if(NULL == config)
	{
		return (0);
	}

	if(NULL == i18n_stack)
	{
		i18n_stack = stack_new(i18n_stack);
	}

	switch(type)
	{
		case JSON_T_OBJECT_BEGIN:
			if(key)
			{
				key_value = GxCore_Strdup(key);
			}
			else
			{
				key_value = NULL;
			}
			stack_push(i18n_stack, (void *)key_value);
			break;
		case JSON_T_OBJECT_END:
			key_value = stack_pop(i18n_stack);
			GxCore_Free(key_value);
			break;
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			key_value = stack_peek(i18n_stack);
			if(NULL != key_value)
			{
				if(0 == strcasecmp(JSON_LANGUAGE, key_value))
				{
					i18n = i18n_add_language((char *)key);
					language_flag = 1;
				}
			}
			break;
		case JSON_T_STRING:
			variable = (char *)value->vu.str.value;
			if(language_flag)
			{
				i18n_add_tran(i18n, (const char *)key, (const char *)variable);
			}
			break;
		default:
			break;
	}

	GUI_FREE(key);

	return (ret);
}

status_t gui_json_i18n_freedata(GuiConfig* cfg)
{
	GuiI18n *del = NULL;
	GuiI18n* next = NULL;

	if(NULL == cfg)
	{
		return GXCORE_ERROR;
	}

	del = cfg->i18n;
	while (del)
	{
		next = del->next;

		GUI_FREE(del->language);
		hash_release(del->hash_text);
		GUI_FREE(del);

		del = next;
	}

	return GXCORE_SUCCESS;
}

