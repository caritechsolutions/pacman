#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_json_parse.h"

static int _json_key_string2value(unsigned char* string)
{
	int ret = 0;
	char *pstr = NULL;

	if(NULL == string)
	{
		return 0;
	}

	//hex
	if(strlen((char*)string) > 2)
	{
		if('0' == string[0] && 'x' == string[1])
		{
			pstr = (char*)&string[2];
			while(*pstr)
			{
				if (*pstr>='0' && *pstr<='9')
				{
					ret=ret * 16 + ((int)*pstr-'0');
				}
				else
				{
					if (*pstr>='A' && *pstr<='F')
					{
						ret=ret * 16 +((int)*pstr-'A'+10);
					}
					else if (*pstr>='a' && *pstr<='f')
					{
						ret=ret *16 + ((int)*pstr-'a'+10);
					}
				}
				pstr++;
			}
			return ret;
		}
	}

	//dec
	pstr = (char*)string;
	while(*pstr)
	{
		if (*pstr>='0' && *pstr<='9')
		{
			ret=ret * 10 + ((int)*pstr-'0');
		}
		else
		{
			gxlogd("[GUI]=key_string2value=error value '%s'\n", string);
			return 0;
		}
		pstr++;
	}

	return (ret);
}

gx_stack_t *key_stack;

int gui_json_keymap(void* ctx, int type, const JSON_value* value)
{
	int ret = 1, hw_keycode = 0;
	static char *key = NULL, *variable = NULL, *key_value = NULL;

	if(NULL == key_stack)
	{
		key_stack = stack_new(key_stack);
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

			stack_push(key_stack, (void *)key_value);
			break;
		case JSON_T_OBJECT_END:
			key_value = stack_pop(key_stack);
			GxCore_Free(key_value);
			break;
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			if(NULL != key)
			{
				key_value = stack_peek(key_stack);
				variable = (char *)value->vu.str.value;

				if(key && variable)
				{
					if((key_value) &&(0 == strcasecmp(JSON_KEY, key_value)))
					{
						hw_keycode = _json_key_string2value((unsigned char *)variable);
						if(-1 == key_map((char*)key, hw_keycode))
						{
							ret = 0;
						}
					}
				}

				GUI_FREE(key);
			}
			break;
		default:
			break;
	}

	return (ret);
}

status_t gui_json_keymap_free(void)
{
	return GXCORE_SUCCESS;
}


