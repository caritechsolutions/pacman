#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "gui_private.h"
#include "gui_xml_parse.h"
#include "image_cache.h"

status_t gui_xml_parse(GuiCore *gui_core, const char *config_file)
{
	handle_t handle = GXCORE_INVALID_POINTER;
	status_t ret = GXCORE_SUCCESS;

	if(NULL == gui_core || NULL == config_file)
		return GXCORE_ERROR;

	handle = GxCore_FileExists("/");
	if(GXCORE_FILE_UNEXIST == handle) {
		GUI_Printf_Yellow("[GUI] rootfs unexist!\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	ret = gui_xml_config(&gui_core->config, config_file);
	if( ret != GXCORE_SUCCESS)
		return ret;

	if((gui_core->config.files.file_xml_keymap) && (NULL == gui_core->config.name))
	{
		ret = gui_xml_keymap(&gui_core->config, gui_core->config.files.file_xml_keymap);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	if(gui_core->config.files.file_xml_i18n)
	{
		ret = gui_xml_i18n(&gui_core->config, gui_core->config.files.file_xml_i18n);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	if(gui_core->config.files.file_xml_image)
	{
		ret = gui_xml_image(&(gui_core->config), gui_core->config.files.file_xml_image);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	if(gui_core->config.files.file_xml_widget)
	{
		ret = gui_xml_widget(&(gui_core->config), gui_core->config.files.file_xml_widget);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	if(gui_core->config.files.file_xml_color)
	{
		ret = gui_xml_color(&(gui_core->config.colors), gui_core->config.files.file_xml_color);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	if(gui_core->config.files.file_xml_font)
	{
	    ret = gui_xml_font(&(gui_core->config), (const char*)gui_core->config.files.file_xml_font);
	    if( ret != GXCORE_SUCCESS)
		return ret;
	}

	if((gui_core->config.files.file_xml_style) && (gui_core == &gui))
	{
		ret = gui_xml_style(gui_core->config.style_widget, gui_core->config.files.file_xml_style);
		if( ret != GXCORE_SUCCESS)
			return ret;
	}

	return GXCORE_SUCCESS;
}

status_t gui_script_parse(GuiCore* gui_core, const char *config_file)
{
	status_t ret = GXCORE_SUCCESS;

	RUNTIME("[XML] Parse", ret = gui_xml_parse(gui_core, config_file));

	return (ret);
}

status_t gui_xml_parse_freedata(GuiCore *gui_core)
{
	status_t ret = GXCORE_ERROR;

	if(NULL == gui_core)
		return GXCORE_ERROR;

	ret |= gui_xml_font_freedata(&(gui_core->config));
	ret |= gui_xml_config_freedata(&gui_core->config);
	//ret |= gui_xml_keymap_freedata();
	ret |= gui_xml_widget_freedata(&(gui_core->config));
	if(gui_core->config.style_widget)
	{
		ret |= gui_xml_style_freedata(gui_core->config.style_widget);
		gui_core->config.style_widget = NULL;
	}
	ret |= gui_xml_i18n_freedata(&gui_core->config);
	ret |= gui_xml_color_freedata(&gui_core->config.colors);
	ret |= gui_xml_image_freedata(gui_core->config.image_cache);


	return ret;
}

status_t gui_xml_reload_widget(GuiCore* gui_core, const char *file_path)
{
	status_t ret = GXCORE_ERROR;

	wm_end_all_window();

	ret = widget_del_tree(gui_core->config.root_widget);
	ret |= widget_del_tree(gui_core->config.freedom_widget);

	gui_core->config.root_widget = widget_new("root", "window", NULL);
	gui_core->config.focus_widget = gui_core->config.root_widget;
	GUI_ASSERT(NULL != gui_core->root_widget);

	gui_core->config.freedom_widget = widget_new("freedom_widget", "boxitem", NULL);
	GUI_ASSERT(NULL != gui_core->freedom_widget);
	//ret |= hash_release(gui_core->signal_table);
	ret |= gui_xml_widget(&(gui_core->config), file_path);

	GUI_FREE(gui_core->config.files.file_xml_widget);
	gui_core->config.files.file_xml_widget = GxCore_Strdup(file_path);

	return (ret);
}

status_t gui_script_parse_freedata(GuiCore *gui_core)
{
	status_t ret = GXCORE_ERROR;

	ret = gui_xml_parse_freedata(gui_core);

	return (ret);
}

char* gui_mallocpath_swapfile(const char* dir_file, const char* file_name)
{
	char* absolute_path = NULL;
	int dir_file_len = 0;
	int dir_len = 0;
	int file_len = 0;
	int i = 0;

	//#NOTICE#  return GxCore_Malloc data, free it please
	if(NULL == dir_file || NULL == file_name)  return NULL;

	dir_file_len = strlen(dir_file);
	file_len = strlen(file_name);
	dir_len = 0;

	for(i = dir_file_len - 1; i >=0; i--)
	{
		if( ('/' == dir_file[i]) && (i != dir_file_len - 1) )
		{
			dir_len = i + 1;
			break;
		}
	}
	if(0 == dir_len) return NULL;

	absolute_path = (char*)GxCore_Malloc(dir_len + file_len + 3);
	if(NULL == absolute_path) return NULL;

	memset(absolute_path, 0, dir_len + file_len + 3);
	memcpy(absolute_path, dir_file, dir_len);
	memcpy(absolute_path + dir_len, file_name, file_len);
	absolute_path[dir_len + file_len] = '\0';

	return absolute_path;
}

static void _resource_config_free(void *data)
{
	GuiConfig *config = NULL;

	if(data) {
		config = (GuiConfig *)data;
		// TODO:
		gui_xml_i18n_freedata(config);
		gui_xml_image_freedata(config->image_cache);
		gui_xml_widget_freedata(config);
		gui_xml_color_freedata(&(config->colors));
		gui_xml_font_freedata(config);
		gui_xml_config_freedata(config);
		if(config->style_widget) {
			gui_xml_style_freedata(config->style_widget);
			config->style_widget = NULL;
		}
		gui_xml_keymap_freedata(config);
		GUI_FREE(config->name);
		GUI_FREE(config);
	} else {
		return;
	}
}

static void gui_rsc_signal_release(void *data)
{
	if (data)
	{
		GUI_FREE(data);
	}
}

status_t gui_rsc_xml_parse (const char *name, GuiCore *gui_core, const char *config_file)
{
	status_t ret = GXCORE_SUCCESS;
	char *style_name = NULL;
	GuiConfig *config = NULL;

	if((gui_core->resource_config_table) && hash_get(gui_core->resource_config_table, name)) {
		config = hash_get(gui_core->resource_config_table, name);
		if(config) {
			if(config->invalid) {
				config->invalid = GUI_FALSE;
				gxlogd(" %s Resource is unload.\n", name);
				return (GXCORE_ERROR);
			} else {
				gxlogd(" %s Resource is existed.\n", name);
				return (GXCORE_SUCCESS);
			}
		}
	}

	config = (GuiConfig *)GUI_MALLOCZ(sizeof(GuiConfig));
	config->name = GUI_STRDUP(name);
	if(config) {
		ret = gui_xml_config(config, config_file);
		if(GXCORE_SUCCESS == ret) {
			// Signal
			if(NULL == config->signal_table) {
				config->signal_table = hash_new(20, gui_rsc_signal_release);
				if(NULL == config->signal_table)
					return (GXCORE_ERROR);
			}
			// Basic hash table
			if(NULL == gui_core->resource_config_table) {
				gui_core->resource_config_table = hash_new(10, _resource_config_free);
			}
			// Basic stack
			if(NULL == gui_core->resource_config_stack) {
				gui_core->resource_config_stack = stack_new(NULL);
			}
			// Resource config
			if(gui_core->resource_config_table) {
				hash_add(gui_core->resource_config_table, name, (void *)config);
			}
			if(gui_core->resource_config_stack) {
				stack_push(gui_core->resource_config_stack, (void *)config);
			}

			// i18n
			if(config->files.file_xml_i18n) {
				gui_xml_i18n(config, config->files.file_xml_i18n);
				if(gui_core->config.cur_i18n)
					_i18n_set_parse(config, gui_core->config.cur_i18n->language);
			}
			// image
			if(config->files.file_xml_image) {
				config->image_cache = GUI_MALLOCZ(sizeof(ImageCache));
				image_cache_init(config->image_cache, gui.fb.size);
				if(config->image_cache) {
					gui_xml_image(config, config->files.file_xml_image);
				}
			}
			// widget
			if(config->files.file_xml_widget) {
				config->root_widget = widget_new(name, "window", NULL);
				if(config->root_widget) {
					config->root_widget->rect.x = config->root_widget->rect.y = 0;
					config->root_widget->rect.w = gui_core->config.width;
					config->root_widget->rect.h = gui_core->config.height;
					gui_xml_widget(config, config->files.file_xml_widget);
				}
			}
			// style
			if(config->files.file_xml_style) {
				style_name = (char *)GUI_MALLOCZ(strlen(name) + 10);
				strncpy(style_name, name, strlen(name));
				strcat(style_name, "_style");
				config->style_widget = widget_new(style_name, "window", NULL);
				if(config->style_widget) {
					ret = gui_xml_style(config->style_widget, config->files.file_xml_style);
				}
				GUI_FREE(style_name);
			}
			// color
			if(config->files.file_xml_color) {
				gui_xml_color(&(config->colors), config->files.file_xml_color);
			}
			// font
			if(config->files.file_xml_font) {
				ret = gui_xml_font(config, (const char*)config->files.file_xml_font);
				if( ret != GXCORE_SUCCESS)
					return ret;
			}

			// TODO:
			// key
			if(config->files.file_xml_keymap) {
				ret = gui_xml_keymap(config, (const char*)config->files.file_xml_keymap);
				if( ret != GXCORE_SUCCESS)
					return ret;
			}
		}
	} else {
		ret = GXCORE_ERROR;
	}

	return (ret);
}

status_t gui_resource_parse(const char *name, GuiCore *gui_core, const char *config_file)
{
	status_t ret = GXCORE_SUCCESS;

	if((NULL == name) || (NULL == gui_core) || (NULL == config_file)) {
		return (GXCORE_ERROR);
	}

	RUNTIME("[Resource XML] Parse", ret = gui_rsc_xml_parse(name, gui_core, config_file));

	return (ret);
}

status_t gui_resource_parse_freedata(const char *name, GuiCore *gui_core)
{
	status_t ret = GXCORE_SUCCESS;
	GuiConfig *config = NULL;
	int count = 0, i = 0;

	if((NULL == name) || (NULL == gui_core)) {
		return (GXCORE_ERROR);
	}

	if(gui_core->resource_config_table) {
		config = (GuiConfig *)hash_get(gui_core->resource_config_table, name);
		if(config && config->invalid) {
			if(config->signal_table) {
				hash_release(config->signal_table);
				config->signal_table = NULL;
			}
			hash_drop(gui_core->resource_config_table, name);
			if(gui_core->resource_config_stack) {
				count = stack_count(gui_core->resource_config_stack);
				for(i = 0; i < count; i++) {
					if(config == stack_get(gui_core->resource_config_stack, i)) {
						stack_del(gui_core->resource_config_stack, i);
						break;
					}
				}

				// release all
				if(0 == stack_count(gui_core->resource_config_stack)) {
					stack_release(gui_core->resource_config_stack);
					hash_release(gui_core->resource_config_table);
					gui_core->resource_config_table = NULL;
					gui_core->resource_config_stack = NULL;
				}
			}
		}
	} else {
		ret = GXCORE_ERROR;
	}

	return (ret);
}


