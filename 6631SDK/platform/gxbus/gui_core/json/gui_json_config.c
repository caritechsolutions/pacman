#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_json_parse.h"

char *_json_get_absolute_path(const char *path, const char *filename)
{
	char *file_path = NULL;
	int len = 0;

	if(NULL != path)
	{
		len = strlen(path);
	}

	if(NULL != filename)
	{
		len += strlen(filename);
	}
	len += 1;

	file_path = (char *)GxCore_Malloc(len);
	if(NULL == file_path)
	{
		return (NULL);
	}

	strcpy(file_path, path);
	strcat(file_path, filename);

	return (file_path);
}

int gui_json_config(void* ctx, int type, const JSON_value* value)
{
	int ret = 1;
	GuiConfig *config = NULL;
	static char *key = NULL, *variable = NULL;

	config = (GuiConfig *)ctx;
	if(NULL == config)
	{
		return (0);
	}

	switch(type)
	{
		case JSON_T_KEY:
			GUI_FREE(key);
			key = GxCore_Strdup(value->vu.str.value);
			break;
		case JSON_T_STRING:
			if(NULL != key)
			{
				variable = (char *)value->vu.str.value;
				if(0 == strcasecmp(JSON_FILE_KEYMAP, key))
				{
					config->files.file_xml_keymap =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_WIDGET, key))
				{
					config->files.file_xml_widget =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_STYLE, key))
				{
					config->files.file_xml_style =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_COLOR, key))
				{
					config->files.file_xml_color =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_I18N, key))
				{
					config->files.file_xml_i18n =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_IMAGE, key))
				{
					config->files.file_xml_image =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_FONT, key))
				{
					config->files.file_xml_font =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_FILE_PAL, key))
				{
					config->files.file_pal =
						_json_get_absolute_path(config->theme_path,variable);
				}
				else if(0 == strcasecmp(JSON_GA, key))
				{
					if(0 == strcasecmp("enable", variable))
					{
						config->enable_ga = 1;
					}
				}
				else if(0 == strcasecmp(JSON_ANTIFLICKER, key))
				{
					if(0 == strcasecmp("enable", variable))
					{
						config->enable_antiflicker = 1;
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

status_t gui_json_config_freedata(GuiConfig* cfg)
{
	if(NULL == cfg)
	{
		return GXCORE_ERROR;
	}

	GUI_FREE(cfg->theme_path);

	GUI_FREE(cfg->files.file_xml_config);
	GUI_FREE(cfg->files.file_xml_keymap);
	GUI_FREE(cfg->files.file_xml_widget);
	GUI_FREE(cfg->files.file_xml_style);
	GUI_FREE(cfg->files.file_xml_i18n);
	GUI_FREE(cfg->files.file_xml_image);
	GUI_FREE(cfg->files.file_xml_font);
	GUI_FREE(cfg->files.file_pal);

	return GXCORE_SUCCESS;
}


