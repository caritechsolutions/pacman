#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_json_parse.h"

static int _json_color_string2value(unsigned char* string)
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

static void color_Free(void *data)
{
	GUI_FREE(data);
}

static void colors_add_color(GuiColors* color, const char* id, const char *value)
{
	if(NULL == color->hash_colors)
	{
		color->hash_colors = hash_new(10, color_Free);
	}

	hash_add(color->hash_colors, id, (void *)value);
}

int gui_json_color(void* ctx, int type, const JSON_value* value)
{
	int hw_color = 0;
	static char *key = NULL, *variable = NULL;

	switch(type)
	{
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			if(NULL != key)
			{
				if(0 == strcasecmp("encoding", key))
				{
					GUI_FREE(key);
					return (1);
				}
				else if(0 == strcasecmp("standalone", key))
				{
					GUI_FREE(key);
					return (1);
				}
				variable = (char *)value->vu.str.value;

				hw_color = _json_color_string2value((unsigned char*)variable);

				GUI_XML_Printf("[GUI]color %s -> '0x%x'\n", key, hw_color);

				colors_add_color(&(gui.config.colors),
						(const char *)key,
						(const char *)value);

				GUI_FREE(key);
			}
			break;
		default:
			break;
	}

	return (1);
}

status_t gui_json_color_free(void)
{
	return GXCORE_SUCCESS;
}


