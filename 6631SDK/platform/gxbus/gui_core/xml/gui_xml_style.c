/*
 * gui_xml_style.c
 *
 * Copyright (C) 2009-doomsday, zfz.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_xml_parse.h"


extern status_t create_widget_list(GuiWidget *parent_widget, GXxmlNodePtr GXxml_node);

status_t gui_xml_style(GuiWidget* style_widget, const char *filename)
{
	GXxmlDocPtr doc;

	if(NULL == style_widget || NULL == filename)
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document style parse error.\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document style empty\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_EMPTY;
	}


	GXxmlNodePtr cur = XML_GET_CHILD(root);
	int style_num = 0;
	while (NULL != cur)
	{
		if (0 ==  GXxmlStrcmp(cur->name, STYLE_STR))
		{
			create_widget_list(style_widget, cur);
			style_num++;
		}
		cur = cur->next;
	}


	GXxmlFreeDoc(doc);

	gxlogd("[GUI]style num: %d\n", style_num);

	return GXCORE_SUCCESS;
}

status_t gui_xml_style_freedata(GuiWidget* style_widget)
{
	widget_del_tree(style_widget);

	return GXCORE_SUCCESS;
}

