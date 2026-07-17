#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_private.h"
#include "gui_json_parse.h"

static char* _json_path_strdup(const char* path_file)
{
	char* path = NULL;
	int dir_file_len = 0;
	int dir_len = 0;
	int i = 0;

	path = (char*)GxCore_Strdup(path_file);
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


status_t gui_json_main(const char *config_file, JSON_parser_callback cb, void *ptr)
{
	JSON_config config = {0};
	struct JSON_parser_struct* jc = NULL;
	status_t ret = GXCORE_SUCCESS;
	handle_t hfile = 0;
	struct stat buf = {0};
	char *buffer = NULL;
	unsigned int next_char = 0;
	int i = 0;

	init_JSON_config(&config);
	config.depth                  = 20;
	config.callback               = cb;
	config.callback_ctx	      = ptr;
	config.allow_comments         = 1;
	config.handle_floats_manually = 0;

	jc = new_JSON_parser(&config);
	if(NULL == jc)
	{
		return (GXCORE_ERROR);
	}

	hfile = GxCore_Open((char *)config_file, "r");
	if(hfile <=0)
	{
		gxlogd("[GUI][JSON]There is no such a file:%s\n", config_file);
		return (GXCORE_ERROR);
	}

	ret = stat(config_file, &buf);
	if (ret < 0)
	{
		gxlogd("[GUI][JSON]There is no such a file:%s\n", config_file);
		return (GXCORE_ERROR);
	}

	buffer = (char *)GxCore_Malloc(buf.st_size + 100);
	if(NULL == buffer)
	{
		return (GXCORE_ERROR);
	}
	memset(buffer, 0, buf.st_size + 100);

	GxCore_Read(hfile, buffer, buf.st_size, 1);

	for (i = 0; i < buf.st_size; i++)
	{
		next_char = buffer[i];

		if (!JSON_parser_char(jc, (unsigned int)next_char)) {
			gxloge ( "[GUI]JSON_parser_char: syntax error, byte %d\n", i);
			ret = GXCORE_ERROR;
			goto done;
		}
	}

	if (!JSON_parser_done(jc)) {
		gxloge ( "[GUI]JSON_parser_end: syntax error\n");
		ret = GXCORE_ERROR;
		goto done;
	}

done:
	delete_JSON_parser(jc);

	GxCore_Close(hfile);

	GxCore_Free(buffer);

	return (ret);
}

status_t gui_json_parse(GuiCore* gui_core, const char *config_file)
{
	status_t ret = GXCORE_SUCCESS;

	if(NULL == gui_core || NULL == config_file)
	{
		return GXCORE_ERROR;
	}

	GUI_Printf_Blue("\n|----------------------------------------------|");
	GUI_Printf_Blue("\n|----------------------------------------------|");
	GUI_Printf_Blue("\n|==            JSON script                   ==|");
	GUI_Printf_Blue("\n|==            Auhtor : Shencz               ==|");
	GUI_Printf_Blue("\n|==            Arpil 15th, 2010              ==|");
	GUI_Printf_Blue("\n|----------------------------------------------|");
	GUI_Printf_Blue("\n|----------------------------------------------|\n");

	if(NULL == gui_core->config.files.file_xml_config)
	{
		gui_core->config.files.file_xml_config = GxCore_Strdup(config_file);
		gui_core->config.theme_path = _json_path_strdup(config_file);
	}

	ret = gui_json_main(config_file, gui_json_config, (void *)&gui_core->config);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_keymap,
			gui_json_keymap,
			NULL);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_i18n,
			gui_json_i18n,
			(void *)&gui_core->config);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_image,
			gui_json_images,
			(void *)&gui_core->config.imgs);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_font,
			gui_json_font,
			NULL);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_color,
			gui_json_color,
			NULL);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	//default value
	gui_core->config.width = 704;
	gui_core->config.height = 576;
	gui_core->config.bpp = 8;
	gui_core->config.osd_trans = 0x000000;
	gui_core->config.gui_trans = 0xFF00FF;
	gui_core->config.osd_alpha = 0x0000FF;

	ret = gui_json_main(gui_core->config.files.file_xml_widget,
			gui_json_widget,
			(void *)gui_core);

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	ret = gui_json_main(gui_core->config.files.file_xml_style,
			gui_json_style,
			(void *)(gui_core->style_widget));

	if(GXCORE_SUCCESS != ret)
	{
		return ret;
	}

	return (ret);
}

char* gui_mallocpath_swapfile(char* dir_file, char* file_name)
{
	char* absolute_path = NULL;
	int dir_file_len = 0;
	int dir_len = 0;
	int file_len = 0;
	int i = 0;

	//#NOTICE#  return GxCore_Malloc data, GxCore_Free it please
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


status_t gui_script_parse(GuiCore* gui_core, const char *config_file)
{
	status_t ret = GXCORE_SUCCESS;

	ret = gui_json_parse(gui_core, config_file);

	return (ret);
}


status_t gui_script_parse_freedata(GuiCore* gui_core)
{
	status_t ret = GXCORE_SUCCESS;

	if(NULL == gui_core)
	{
		return (GXCORE_ERROR);
	}

	ret = gui_json_main(gui_core->config.files.file_xml_font,
			gui_json_font_freedata,
			NULL);
	ret |= gui_json_keymap_free();
	ret |= gui_json_color_free();
	ret |= gui_json_widget_freedata(gui_core);
	ret |= gui_json_style_freedata(gui_core->style_widget);
	ret |= gui_json_i18n_freedata(&(gui_core->config));
	ret |= gui_json_image_freedata(&(gui_core->config.imgs));
	ret |= gui_json_config_freedata(&(gui_core->config));

	return (ret);
}


