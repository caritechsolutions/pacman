#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "gui_private.h"
#include "gui_xml_parse.h"

static status_t free_font_list(GuiConfig *config)
{
	struct gdi_font *pFont = NULL, *pNext = NULL;

	if(NULL == config) {
		return GXCORE_ERROR;
	}
	pFont = config->pFirstFont;
	while(pFont) {
		pNext = pFont->next;
		gdi_font_destroy(config, pFont);
		pFont = pNext;
	}
	config->pFirstFont = config->pLastFont = config->pCurrentFont = config->pDefaultFont = NULL;

	return GXCORE_SUCCESS;
}

static status_t create_font(GuiConfig *config, GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr node = NULL;
	xmlChar *name= NULL;
	xmlChar *file = NULL;
	xmlChar *size = NULL;
#ifdef GUI_TTF
	xmlChar *style = NULL;
#endif
	GuiCore *gui_core = GUI_GetCurrent();
	int ret = 1, value = 0, font_style = 0;

	if(NULL == config) {
		return GXCORE_ERROR;
	}

#ifdef GUI_TTF
	config->font_threshold_height = GUI_FONT_THRESHOLD_SIZE;
#endif
	node =  XML_GET_CHILD(GXxml_node);
	while (node != NULL)
	{
		if (GXxmlStrcmp(node->name, NAME_STR) == 0)
			name = GXxmlNodeGetContent(node);
		else if (GXxmlStrcmp(node->name, FILE_STR) == 0)
			file = GXxmlNodeGetContent(node);
		else if (GXxmlStrcmp(node->name, SIZE_STR) == 0) {
			size = GXxmlNodeGetContent(node);
			value = atoi((const char *)size);
		}
		else if(GXxmlStrcmp(node->name, STYLE_STR) == 0) {
#ifdef GUI_TTF
			style = GXxmlNodeGetContent(node);
			if(0 == GXxmlStrcmp((xmlChar *)"normal", style))
				font_style |= TTF_STYLE_NORMAL;
			else if(0 == GXxmlStrcmp((xmlChar *)"bold", style))
				font_style |= TTF_STYLE_BOLD;
			else if(0 == GXxmlStrcmp((xmlChar *)"italic", style))
				font_style |= TTF_STYLE_ITALIC;
			else if(0 == GXxmlStrcmp((xmlChar *)"underline", style))
				font_style |= TTF_STYLE_UNDERLINE;
			XML_FREE(style);
#endif
		}
		node = node->next;
	}


	if(name && file && size) {
		char* absolute_path = NULL;

		absolute_path = gui_mallocpath_swapfile(config->files.file_xml_font, (char*)file);
		if(NULL != absolute_path) {
			ret = gdi_font_load(config, (char *)name, absolute_path, value, font_style);
			if(&(gui_core->config) == config)
				gxgdi_set_font((char *)name);
			GUI_FREE(absolute_path);
		}
	}
	XML_FREE(name);
	XML_FREE(file);
	XML_FREE(size);

	return (ret = 0 ?  GXCORE_SUCCESS : GXCORE_ERROR);
}

static status_t _set_font_threshold_size(GuiConfig* config, GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr node = NULL;
	xmlChar *file = NULL, *path = NULL;
	xmlChar *size = NULL;
	int font_size = 0;

	node =  XML_GET_CHILD(GXxml_node);
	while (node != NULL) {
		if (GXxmlStrcmp(node->name, FILE_STR) == 0) {
			file = GXxmlNodeGetContent(node);
		} else if (GXxmlStrcmp(node->name, SIZE_STR) == 0) {
			size = GXxmlNodeGetContent(node);
			font_size = atoi((const char *)size);
			XML_FREE(size);
		}
		node = node->next;
	}
#ifdef GUI_TTF
	if(file) {
		path = (xmlChar *)gui_mallocpath_swapfile(config->files.file_xml_font, (char *)file);
		gxgdi_font_thrshold_size(config, (char *)path, font_size);
		GUI_FREE(path);
	} else {
		config->font_threshold_height = GUI_FONT_THRESHOLD_SIZE;
		config->font_threshold_size   = -1;
	}
#else
		config->font_threshold_height = GUI_FONT_THRESHOLD_SIZE;
		config->font_threshold_size   = 0;
#endif /*GUI_TTF*/
	XML_FREE(file);
	return (0);
}

status_t create_font_list(GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr cur = XML_GET_CHILD(GXxml_node);
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	if(NULL == gui_core) {
		return GXCORE_ERROR;
	}

	while (NULL != cur)
	{
		if (0 ==  GXxmlStrcmp(cur->name, FONT_STR))
			create_font(&(gui_core->config), cur);
		cur = cur->next;
	}

	return GXCORE_SUCCESS;
}

status_t gui_xml_font(GuiConfig* config, const char *filename)
{
	GXxmlDocPtr doc;

	if((NULL == config) || (NULL == filename))
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document font parse error.\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document font empty\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_EMPTY;
	}


	if (0 !=  GXxmlStrcmp(root->name, FONTS_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}


	GXxmlNodePtr cur = XML_GET_CHILD(root);
	while (NULL != cur)
	{
		if (0 ==  GXxmlStrcmp(cur->name, FONT_STR))
		{
			create_font(config, cur);
		}
		else if(0 == GXxmlStrcmp(cur->name, FONT_THRESHOLD_STR))
		{
			_set_font_threshold_size(config, cur);
		}
		cur = cur->next;
	}
	gxgdi_font_infomation_print();

	GXxmlFreeDoc(doc);
	return GXCORE_SUCCESS;
}

status_t gui_xml_font_freedata(GuiConfig *config)
{
	GXxmlDocPtr doc;
	const char *filename = NULL;

	if(NULL == config) {
		return GXCORE_ERROR;
	}

	free_font_list(config);

	return GXCORE_SUCCESS;
}


