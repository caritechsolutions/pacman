/*
 * gui_xml_image.c
 *
 * Copyright (C) 2010-doomsday, shencz.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_xml_parse.h"

static void color_Free(void *data)
{
	XML_FREE(data);
}

static void colors_add_color(GuiColors* color, const char* id, const char *value)
{
	if(NULL == color->hash_colors)
	{
		color->hash_colors = hash_new(10, color_Free);
	}

	hash_add(color->hash_colors, id, (void *)value);
}

status_t create_color_list(GuiColors* color,  GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr node = NULL;
	xmlChar *key = NULL;
	xmlChar *value = NULL;

	node = GXxml_node;
	while (node != NULL)
	{
		if (GXxmlStrcmp(node->name, COLOR_STR) == 0)
		{
			key = GXxmlGetProp(node, NAME_STR);
			value = GXxmlNodeGetContent(node);

			//			GUI_XML_Printf("color %s -> '%s'\n", value, key);
			colors_add_color(color, (const char *)key, (const char *)value);

			XML_FREE(key);
			//GUI_FREE(value);
		}
		node = node->next;
	}

	return GXCORE_SUCCESS;
}

status_t gui_xml_color(GuiColors* color, const char *filename)
{
	GXxmlDocPtr doc;

	if((NULL == color) || (NULL == filename))
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document color parse error.\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document color empty\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_EMPTY;
	}

	if (0 !=  GXxmlStrcmp(root->name, COLORS_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}


	GXxmlNodePtr cur = XML_GET_CHILD(root);

	create_color_list(color, cur);

	GXxmlFreeDoc(doc);

	return GXCORE_SUCCESS;

}

status_t gui_xml_color_freedata(GuiColors* color)
{
	if(NULL == color)
	{
		return GXCORE_ERROR;
	}

	if(color->hash_colors) {
		hash_release(color->hash_colors);
		color->hash_colors = NULL;
	}

	return GXCORE_SUCCESS;
}

