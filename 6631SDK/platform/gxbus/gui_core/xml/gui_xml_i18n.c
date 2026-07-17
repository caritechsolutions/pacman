/*
 * gui_xml_i18n.c
 *
 * Copyright (C) 2009-doomsday, zfz.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_config.h"
#include "gui_private.h"
#include "gui_xml_parse.h"

void I18N_Free(void *data)
{
	GUI_FREE(data);
}

status_t gui_xml_i18n(GuiConfig* cfg, const char *filename)
{
	GXxmlDocPtr doc;
	GXxmlNodePtr root;
	GXxmlNodePtr cur;

	if(NULL == cfg || NULL == filename)
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document i18n parse error.\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_PARSE;
	}

	root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document i18n empty\n");
		GXxmlFreeDoc(doc);
		return (GXCORE_ERROR);
	}

	if (0 !=  GXxmlStrcmp(root->name, I18N_STR))
	{
		GXxmlFreeDoc(doc);
		return (GXCORE_ERROR);
	}

	cur = XML_GET_CHILD(root);

	if(NULL == cfg->i18n)
	{
		cfg->i18n = (GuiI18n *)GxCore_Malloc(sizeof(GuiI18n));
		if(NULL == cfg->i18n)
		{
			GXxmlFreeDoc(doc);
			return (GXCORE_ERROR);
		}
		memset(cfg->i18n, 0, sizeof(GuiI18n));

		cfg->i18n->hash_text = hash_new(80, I18N_Free);
		if(NULL == cfg->i18n->hash_text)
		{
			GXxmlFreeDoc(doc);
			return (GXCORE_ERROR);
		}
	}

	while(NULL != cur)
	{
		if (GXxmlStrcmp(cur->name, LANGUAGE_STR) == 0)
		{
			char* file = NULL;
			xmlChar *key = NULL;
			xmlChar *value = NULL;

			key = GXxmlGetProp(cur, ID_STR);
			value = GXxmlNodeGetContent(cur);

			file = gui_mallocpath_swapfile(cfg->files.file_xml_i18n, (char*)value);
			hash_add(cfg->i18n->hash_text, (const char *)key, file);
			XML_FREE(key);
			XML_FREE(value);
		}
		cur = cur->next;
	}

	GXxmlFreeDoc(doc);
	return GXCORE_SUCCESS;
}


status_t gui_xml_i18n_freedata(GuiConfig* cfg)
{
	GuiI18n *del = NULL;
	GuiI18n* next = NULL;

	if((NULL == cfg) || (NULL == cfg->i18n))
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
	cfg->i18n = NULL;

	del = cfg->cur_i18n;
	while (del)
	{
		next = del->next;

		GUI_FREE(del->language);
		hash_release(del->hash_text);
		GUI_FREE(del);

		del = next;
	}
	cfg->cur_i18n = NULL;

	return GXCORE_SUCCESS;
}


