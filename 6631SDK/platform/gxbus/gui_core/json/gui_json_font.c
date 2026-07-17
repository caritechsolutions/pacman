#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_private.h"
#include "gui_json_parse.h"

int gui_json_font(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	static char *key = NULL, *variable = NULL;
	static unsigned char *name= NULL;
	unsigned char *file = NULL;
	unsigned char* absolute_path = NULL;
	GuiCore *gui_core = GUI_GetCurrent();

	switch(type) {
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			if(NULL != key)
			{
				variable = (char *)value->vu.str.value;
				if(0 == strcasecmp(JSON_NAME, key))
				{
					name = (unsigned char *)GxCore_Strdup(variable);
				}
				else if(0 == strcasecmp(JSON_FILE, key))
				{
					file = (unsigned char *)GxCore_Strdup(variable);
					if((NULL != name) && (NULL != file))
					{
						absolute_path = (unsigned char *)gui_mallocpath_swapfile(
								gui.config.files.file_xml_font,
								(char*)file
								);
						if(NULL != absolute_path)
						{
							ret = gdi_font_load(&(gui_core->config), (char *)name, (char *)absolute_path, 0, TTF_STYLE_NORMAL);
							ret ^= 1;
							gxgdi_set_font((char *)name);
							GUI_FREE(absolute_path);
							GUI_FREE(name);
							GUI_FREE(file);
						}
					}
				}
				else if(0 == strcasecmp(JSON_SIZE, key))
				{

				}
			}
			GUI_FREE(key);
			break;
		case JSON_T_INTEGER:
			GUI_FREE(key);
			break;
		default:
			break;
	}

	return (1);
}

int gui_json_font_freedata(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	static char *key = NULL, *variable = NULL;
	GuiCore *gui_core = GUI_GetCurrent();

	switch(type) {
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			variable = (char *)value->vu.str.value;
			if(0 == strcasecmp(JSON_NAME, key))
			{
				if(variable)
				{
					ret = gdi_font_destroy(&(gui_core->config), (char *)variable);
				}

				if(0 == ret)
				{
					return (1);
				}
				else
				{
					return (0);
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


