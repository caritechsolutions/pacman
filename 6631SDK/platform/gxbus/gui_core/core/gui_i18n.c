#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_config.h"
#include "gui.h"

static void char_Free(void *data)
{
	if (data)
	{
	    GUI_FREE(data);
	}
}

GuiI18n *i18n_add_language(const char *language)
{
	GuiI18n *i18n = NULL;
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();

	if (language == NULL) return NULL;
	i18n = (GuiI18n *) GxCore_Malloc(sizeof(GuiI18n));

	if (i18n == NULL) return NULL;

	i18n->language = GxCore_Strdup(language);
	i18n->hash_text = hash_new(40, char_Free);
	i18n->next = NULL;
	if (gui_core->config.i18n != NULL)
	{
		i18n->next = gui_core->config.i18n;
	}

	gui_core->config.i18n = i18n;

	GUI_FREE(gui_core->config.cur_language);
	gui_core->config.cur_language = GxCore_Strdup(language);

	return i18n;
}

void i18n_add_tran(GuiI18n *i18n, const char *english, const char *text)
{
	if (english && text)
	{
		hash_add(i18n->hash_text, english, GxCore_Strdup(text));
	}
}

static const char *_i18n_config_get_string(GuiConfig *config, const char *text)
{
	if(NULL == config->cur_i18n) {
		return (text);
	}

	if(config->cur_i18n->hash_text && text) {
		const char *i18n_text = (char *)hash_get(config->cur_i18n->hash_text, text);
		return i18n_text  ? i18n_text : text;
	}

	return (text);
}

const char *i18n_get_string(const char *text)
{
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	const char *i18n_text = NULL;
	int i = 0, count = 0;

	gui_core = GUI_GetCurrent();

	if(gui_core->resource_config_table && gui_core->resource_config_stack) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = stack_get(gui_core->resource_config_stack, i);
			if((NULL == config) ||
			   ((config && config->invalid))) {
				continue;
			}
			i18n_text = _i18n_config_get_string(config, text);
			if((i18n_text) && (i18n_text != text)) {
				return (i18n_text);
			}
		}
	}

	i18n_text = _i18n_config_get_string(&(gui_core->config), text);
	if((i18n_text == text) && (gui_core != &gui)) {
		i18n_text = _i18n_config_get_string(&(gui.config), text);
	}

	return i18n_text ? i18n_text : text;
}

void Language_Free(void *data)
{
	GUI_FREE(data);
}

int _i18n_set_parse(GuiConfig *config, const char *language)
{
	int ret = 0;
	GXxmlDocPtr doc;
	GuiI18n *i18n = NULL;
	GXxmlNodePtr text_node = NULL;
	//xmlChar *file_language = NULL;
	xmlChar *key = NULL;
	xmlChar *value = NULL;
	char *file = NULL;

	if(config->cur_i18n)
	{
		GUI_FREE(config->cur_i18n->language);
		hash_release(config->cur_i18n->hash_text);
		GUI_FREE(config->cur_i18n);
	}

	file = hash_get(config->i18n->hash_text, language);
	if(NULL == file)
	{
		return (1);
	}

	doc = GXxmlParseFile(file);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document i18n parse error.\n");
		return (1);
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);

	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document i18n empty\n");
		GXxmlFreeDoc(doc);
		return (1);
	}

	GXxmlNodePtr cur = XML_GET_CHILD(root);

	if(cur)
	{
		i18n = (GuiI18n *)GxCore_Malloc(sizeof(GuiI18n));//i18n_add_language((char *)language);
		memset(i18n, 0, sizeof(GuiI18n));
		i18n->hash_text = hash_new(100, Language_Free);
		i18n->language = GUI_STRDUP(language);
		if(NULL == i18n->hash_text)
		{
			GUI_FREE(i18n->language);
			GUI_FREE(i18n);
			return (1);
		}

		if(i18n)
		{
			text_node = cur;
			while (text_node != NULL)
			{
				if (GXxmlStrcmp(text_node->name, TRAN_STR) == 0)
				{
					key   = GXxmlGetProp(text_node, ID_STR);
					value = GXxmlNodeGetContent(text_node);
					i18n_add_tran(i18n, (const char *)key, (const char *)value);
					XML_FREE(key);
					XML_FREE(value);
				}
				else if(GXxmlStrcmp(FONT_STR, text_node->name) == 0)
				{
					value = GXxmlNodeGetContent(text_node);
					if(value)
					{
						gxgdi_set_font((char *)value);
						gxgdi_set_default_font(config, gxgdi_get_font());
					}
					XML_FREE(value);
				}
				text_node = text_node->next;
			}
		}

		config->cur_i18n = i18n;
	}

	GXxmlFreeDoc(doc);

	return (ret);
}

int i18n_set_language(const char *language)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;
	int i = 0, count = 0;

	if(gui_core->resource_config_table && gui_core->resource_config_stack) {
		count = stack_count(gui_core->resource_config_stack);
		for(i = 0; i < count; i++) {
			config = stack_get(gui_core->resource_config_stack, i);
			if((NULL == config) ||
			   (NULL == config->i18n) ||
			   ((config && config->invalid))) {
				continue;
			}
			_i18n_set_parse(config, language);
		}
	}

	config = &(gui_core->config);
	return (_i18n_set_parse(config, language));

}

