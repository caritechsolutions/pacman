#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "gui_private.h"
#include "gui_xml_parse.h"

static void _config_print(GuiConfig* cfg)
{
	if(NULL == cfg)return;

	GUI_Printf("theme_path:         %s\n", cfg->theme_path);
	GUI_Printf("file_xml_config:    %s\n", cfg->files.file_xml_config);
	GUI_Printf("file_xml_keymap:    %s\n", cfg->files.file_xml_keymap);
	GUI_Printf("file_xml_widget:    %s\n", cfg->files.file_xml_widget);
	GUI_Printf("file_xml_style:     %s\n", cfg->files.file_xml_style);
	GUI_Printf("file_xml_color:     %s\n", cfg->files.file_xml_color);
	GUI_Printf("file_xml_i18n:      %s\n", cfg->files.file_xml_i18n);
	GUI_Printf("file_xml_image:     %s\n", cfg->files.file_xml_image);
	GUI_Printf("file_xml_font:      %s\n", cfg->files.file_xml_font);
	GUI_Printf("file_pal:           %s\n", cfg->files.file_pal);
	GUI_Printf("enable_ga:          %d\n", cfg->enable_ga);
	GUI_Printf("enable_double_buffer%d\n", cfg->enable_double_buffer);
	GUI_Printf("enable_antiflicker: %d\n", cfg->enable_antiflicker);
	GUI_Printf("image buffer size:  %d\n", cfg->image_buffer_size);
}

char* _path_strdup(const char* path_file)
{
	char* path = NULL;
	int dir_file_len = 0;
	int dir_len = 0;
	int i = 0;

	path = (char*)GXxmlStrdup((const CHAR*)path_file);
	if(NULL == path) return NULL;

	dir_file_len = strlen(path);
	dir_len = 0;

	for(i = dir_file_len - 1; i >=0; i--)
	{
		if( ('/' == path[i]) && (i != dir_file_len - 1) )
		{
			dir_len = i + 1;
			break;
		}
	}
	if(0 == dir_len) return NULL;

	path[dir_len] = '\0';

	return path;
}

static int _string2number(xmlChar *value)
{
	xmlChar t_char_value = 0;
	xmlChar *temp_str = NULL;
	unsigned int length = 0, ret = 0;

	if(NULL == value)
	{
		return (0);
	}

	length = strlen((char *)value);
	t_char_value = value[length - 1];

	temp_str = (xmlChar *)GxCore_Malloc(length + 1);
	if(NULL == temp_str)
	{
		return (0);
	}

	memset(temp_str, 0, length + 1);

	if(('k' == t_char_value) || ('K' == t_char_value))
	{
		memcpy(temp_str, value, length - 1);
		temp_str[length - 1] = '\0';
		ret = atoi((char *)temp_str);
		ret = ret * 1024;
	}
	else if(('m' == t_char_value) || ('M' == t_char_value))
	{
		memcpy(temp_str, value, length - 1);
		temp_str[length - 1] = '\0';
		ret = atoi((char *)temp_str);
		ret = ret * 1024 * 1024;
	}
	else
	{
		ret = atoi((char *)value);
	}

	GUI_FREE(temp_str);

	return (ret);
}

status_t gui_xml_config(GuiConfig* cfg, const char *filename)
{
	GXxmlDocPtr doc;
	xmlChar *value;
	handle_t handle = GXCORE_INVALID_POINTER;

	if(NULL == cfg || NULL == filename)
	{
		return GXCORE_ERROR;
	}

	handle = GxCore_FileExists(filename);
	if(GXCORE_FILE_UNEXIST == handle)
	{
		gxlogd( "[GUI]XML Document config '%s' unexist.\n", filename);
		return GX_ERROR_XML_DOC_PARSE;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document config '%s' parse error.\n", filename);
		return GX_ERROR_XML_DOC_PARSE;
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document config empty\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_EMPTY;
	}

	if(doc->encoding)
	{
	    value = (xmlChar *)doc->encoding;
	    if(0 == GXxmlStrcmp((const CHAR *)"utf8", (const CHAR *)value))
	    {
		cfg->enable_i18n = GUI_TRUE;
	    }
	    else
	    {
		cfg->enable_i18n = GUI_FALSE;
		gdi_set_default_local((char *)value);
	    }
	}

	cfg->files.file_xml_config = (char*)GXxmlStrdup((const CHAR*)filename);
	cfg->theme_path = _path_strdup(filename);

	GXxmlNodePtr cur = XML_GET_CHILD(root);
	while (NULL != cur) {
		//file path
		char *filename = (char*)GXxmlNodeGetContent(cur);

		if (filename == NULL) {
			gxlogd( "[GUI]XML Document config file: %s\n", cur->name);
			break;
		}
		if (0 == GXxmlStrcmp(cur->name, FILE_KEYMAP_STR))
			cfg->files.file_xml_keymap = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_WIDGET_STR))
			cfg->files.file_xml_widget = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_STYLE_STR))
			cfg->files.file_xml_style = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_COLOR_STR))
			cfg->files.file_xml_color = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_I18N_STR))
			cfg->files.file_xml_i18n = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_IMAGE_STR))
			cfg->files.file_xml_image = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_FONT_STR))
			cfg->files.file_xml_font = GxCore_CatPath(cfg->theme_path, filename);
		else if (0 == GXxmlStrcmp(cur->name, FILE_PAL_STR))
			cfg->files.file_pal = GxCore_CatPath(cfg->theme_path, filename);
		//config
		else if (0 == GXxmlStrcmp(cur->name, GA_STR)) {
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->enable_ga, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, ANTIFLICKER_STR)) {
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->enable_antiflicker, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, DOUBLE_BUFFER_STR)){
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->enable_double_buffer, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, HARDWARE_LAYER_STR)) {
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->hardware_layer, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, FONT_OVERLAY_STR)) {
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->font_overlay, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, FONT_INPOOL_STR)) {
			value = GXxmlNodeGetContent(cur);
			GUI_CONFIG_STATE(cfg->font_in_pool, value);
			XML_FREE(value);
		}
		else if (0 == GXxmlStrcmp(cur->name, BUFFER_SIZE_STR))
		{
			value = GXxmlNodeGetContent(cur);
			cfg->image_buffer_size = _string2number(value);
			XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, BLOCK_JOIN))
		{
		    value = GXxmlNodeGetContent(cur);
		    GUI_CONFIG_STATE(cfg->block_join, value);
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, GA_COUNT))
		{
		    value = GXxmlNodeGetContent(cur);
		    cfg->ga_count = _string2number(value);
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, FOR_GDI))
		{
		    value = GXxmlNodeGetContent(cur);
		    if(0 == GXxmlStrcmp((const CHAR *)"enable", (const CHAR *)value)) {
			cfg->only_gdi = GUI_TRUE;
		    } else {
			cfg->only_gdi = GUI_FALSE;
		    }
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, OSD_SPP_SHARE))
		{
		    value = GXxmlNodeGetContent(cur);
		    if(0 == GXxmlStrcmp((const CHAR *)"enable", (const CHAR *)value)) {
			cfg->buffer_share = GUI_TRUE;
		    } else {
			cfg->buffer_share = GUI_FALSE;
		    }
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, IMAGE_LIMIT))
		{
		    value = GXxmlNodeGetContent(cur);
		    cfg->image_limit = _string2number(value);
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, KEY_SHIELD_STR))
		{
		    value = GXxmlNodeGetContent(cur);
		    if(0 == GXxmlStrcmp((const CHAR *)"enable", (const CHAR *)value))
			cfg->enable_key_shield = GUI_TRUE;
		    else
			cfg->enable_key_shield = GUI_FALSE;
			XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, SCHEDULE_TIME_STR))
		{
		    value = GXxmlNodeGetContent(cur);
			cfg->schedule_time = _string2number(value);
		    XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, ESOTERIC_LANGUAGE))
		{
		    value = GXxmlNodeGetContent(cur);
		    if(0 == GXxmlStrcmp((const CHAR *)"enable", (const CHAR *)value))
				cfg->esoteric_language = GUI_TRUE;
			else
				cfg->esoteric_language = GUI_FALSE;
			XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, KEY_MODE))
		{
		    value = GXxmlNodeGetContent(cur);
			if(cfg != &(gui.config))
			{
				cfg->key_mode = get_virtual_gui_mode((char *)value);
			}
			XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, MESSAGE_MODE))
		{
		    value = GXxmlNodeGetContent(cur);
			if(cfg != &(gui.config))
			{
				cfg->msg_mode = get_virtual_gui_mode((char *)value);
			}
			XML_FREE(value);
		}
		else if(0 == GXxmlStrcmp(cur->name, CACHE_CONTEXT_STR)) {
			value = GXxmlNodeGetContent(cur);
			if(0 == GXxmlStrcmp((const CHAR *)"disable", (const CHAR *)value)) {
				cfg->uncache_context = GUI_TRUE;
			}
			XML_FREE(value);
		}
		else
		{
			create_interface(&(gui.config), cur);
		}

		XML_FREE(filename);

		cur = cur->next;
	}

	GXxmlFreeDoc(doc);

	_config_print(cfg);

	return GXCORE_SUCCESS;
}


status_t gui_xml_config_freedata(GuiConfig* cfg)
{
	if(NULL == cfg)
	{
		return GXCORE_ERROR;
	}

	XML_FREE(cfg->theme_path);

	XML_FREE(cfg->files.file_xml_config);
	GUI_FREE(cfg->files.file_xml_keymap);
	GUI_FREE(cfg->files.file_xml_widget);
	GUI_FREE(cfg->files.file_xml_style);
	GUI_FREE(cfg->files.file_xml_color);
	GUI_FREE(cfg->files.file_xml_i18n);
	GUI_FREE(cfg->files.file_xml_image);
	GUI_FREE(cfg->files.file_xml_font);
	GUI_FREE(cfg->files.file_pal);

	return GXCORE_SUCCESS;
}

