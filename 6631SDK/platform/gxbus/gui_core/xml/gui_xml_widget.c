#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui.h"
#include "gui_private.h"
#include "gui_xml_parse.h"

status_t create_widget_list(GuiWidget *parent_widget, GXxmlNodePtr GXxml_node);
static status_t widget_parse_GXxmlnode(GuiWidget *widget, GXxmlNodePtr node);

static int atoidef(const char *nptr, int v)
{
	if (nptr)
		return atoi(nptr);
	else
		return v;
}

#if 0
static void _widget_interface_print(GuiCore* gui_core)
{
	if(NULL == gui_core)return;

	GUI_XML_Printf("width:      %d\n", gui_core->config.width);
	GUI_XML_Printf("height:     %d\n", gui_core->config.height);
	GUI_XML_Printf("bpp:        %d\n", gui_core->config.bpp);
	//GUI_XML_Printf("bpp8_multclut:  %d\n", gui_core->config.bpp8_multclut);
	GUI_XML_Printf("osd_trans:  0x%x\n", gui_core->config.osd_trans);
	GUI_XML_Printf("gui_trans:  0x%x\n", gui_core->config.gui_trans);
	GUI_XML_Printf("osd_alpha:  0x%x\n", gui_core->config.osd_alpha);
}
#endif

static status_t widget_parse_GXxmlnode(GuiWidget *widget, GXxmlNodePtr node)
{
	while (node) {
		if ( 0 == GXxmlStrcmp(node->name, PROPERTY_STR) ) {
			GXxmlNodePtr property_node = XML_GET_CHILD(node);
			while (NULL != property_node) {
				xmlChar *content = GXxmlNodeGetContent(property_node);
				//GUI_XML_Printf("set_property: %s, %s\n",(char *)property_node->name, (char *)content);
				widget_set_property(widget, (char *)property_node->name, (char *)content);//"tabstop" ??
				XML_FREE(content);

				property_node = property_node->next;
			}
		}
		else if ( 0 == GXxmlStrcmp(node->name, SIGNAL_STR) ) {
			GXxmlNodePtr signal_node = XML_GET_CHILD(node);
			while (NULL != signal_node) {
				xmlChar *content = GXxmlNodeGetContent(signal_node);
				//GUI_XML_Printf("set_signal: %s, %s\n",(char *)signal_node->name, (char *)content);
				widget_signal_new(widget, (char *)signal_node->name, (char *)content);
				XML_FREE(content);

				signal_node = signal_node->next;
			}
		}
		else if ( 0 == GXxmlStrcmp(node->name, CHILDREN_STR) ) {
			GXxmlNodePtr child_node = XML_GET_CHILD(node);
			while (NULL != child_node) {
				if (0 == GXxmlStrcmp(child_node->name, WIDGET_STR))
					create_widget_list(widget, child_node);

				child_node = child_node->next;
			}
		}
		node = node->next;
	}

	return GXCORE_SUCCESS;
}

status_t widget_init(GuiWidget *widget);
status_t create_widget_list(GuiWidget *parent_widget, GXxmlNodePtr GXxml_node)
{
	GuiWidget *widget = NULL;
	xmlChar *widget_name    = GXxmlGetProp(GXxml_node, NAME_STR );
	xmlChar *widget_classid = GXxmlGetProp(GXxml_node, CLASS_STR);
	xmlChar *widget_style   = GXxmlGetProp(GXxml_node, STYLE_STR);
	xmlChar *widget_GXxml     = GXxmlGetProp(GXxml_node, FILENAME_STR);
	xmlChar *preload_flag    = GXxmlGetProp(GXxml_node, PRELOAD_STR);

	widget = widget_new_child(parent_widget, (char *)widget_name, (char *)widget_classid, (char *)widget_style);

	if (NULL == widget) {
		gxlogd( "[GUI]widget_new_child(""%s"", ""%s"") = NULL\n", (char *)widget_name, (char *)widget_classid);
		return GXCORE_ERROR;
	}

	if(0 == strcasecmp("window", widget->classname)) {
		widget->from_resource = GUI_STRDUP(parent_widget->name);
	} else if(parent_widget->from_resource) {
		widget->from_resource = GUI_STRDUP(parent_widget->from_resource);
	}

	if (widget_GXxml || (widget_GXxml == NULL && XML_GET_CHILD(GXxml_node) == NULL)) {
		if (widget_GXxml)
			widget->xml_filename = GUI_STRDUP((char *)widget_GXxml);
		else {
			widget->xml_filename = (char *)GUI_MALLOCZ(strlen((char *)widget_name) + 5);
			memset(widget->xml_filename, 0, strlen((char *)widget_name) + 4);
			strcpy(widget->xml_filename, (char *)widget_name);
			strcat(widget->xml_filename, ".xml");
		}
		widget->init = GUI_FALSE;

		XML_FREE(widget_name);
		XML_FREE(widget_classid);
		XML_FREE(widget_style);
		XML_FREE(widget_GXxml);

		if((preload_flag) && (0 == strcasecmp("true", (const char *)preload_flag))) {
			widget_set_property(widget, (const char *)PRELOAD_STR, "true");
		}

		return GXCORE_SUCCESS;
	}

	XML_FREE(widget_name);
	XML_FREE(widget_classid);
	XML_FREE(widget_style);
	XML_FREE(widget_GXxml);

	widget->init = GUI_TRUE;
	return widget_parse_GXxmlnode(widget, XML_GET_CHILD(GXxml_node));
}

status_t widget_init(GuiWidget *widget)
{
	GuiCore *gui_core = NULL;
	GuiConfig *config = NULL;
	GXxmlNodePtr node;
	GXxmlDocPtr doc;
	char *path, basename[256];

	if (widget->init != 0)
		return GXCORE_SUCCESS;
	else if(0 == strcasecmp("listitem", widget->classname)) {
		widget->init = 1;
		return GXCORE_SUCCESS;
	}

	if(widget->widget_mutex > 0) {
		GxCore_MutexLock(widget->widget_mutex);
	}

	gui_core = GUI_GetCurrent();
	if(gui_core->resource_config_table) {
		config = hash_get(gui_core->resource_config_table, widget->from_resource);
	}

	if((NULL == config) ||
	   (config && config->invalid)) {
		config = &(gui_core->config);
	}

	GxCore_BasePath(config->files.file_xml_widget, basename);
	path = GxCore_CatPath(basename, widget->xml_filename);
	doc = GXxmlParseFile(path);

	if (doc == NULL) {
		widget->init = 1;
		GUI_FREE(path);
		if(widget->widget_mutex > 0) {
			GxCore_MutexUnlock(widget->widget_mutex);
		}
		return GX_ERROR_XML_DOC_PARSE;
	}
	GUI_FREE(path);

	node = XML_GET_ROOT(doc);
	while (node) {
		xmlChar *widget_name = GXxmlGetProp(node, NAME_STR );
		if (GXxmlStrcmp(widget_name, (xmlChar *)widget->name) == 0)
		{
			XML_FREE(widget_name);
			break;
		}
		XML_FREE(widget_name);
		node = node->next;
	}

	if (node == NULL) {
		GXxmlFreeDoc(doc);
		if(widget->widget_mutex > 0) {
			GxCore_MutexUnlock(widget->widget_mutex);
		}
		return GX_ERROR_XML_DOC_EMPTY;
	}

	widget_parse_GXxmlnode(widget, XML_GET_CHILD(node));
	GXxmlFreeDoc(doc);

	widget->init = 1;

	if(widget->widget_mutex > 0) {
		GxCore_MutexUnlock(widget->widget_mutex);
	}
	return GXCORE_SUCCESS;
}

status_t widget_quit(GuiWidget *widget)
{
    GuiWidget *child = NULL;

    if(NULL == widget)
    {
	return GXCORE_ERROR;
    }

    if(widget->object)
    {
	return GXCORE_ERROR;
    }

    widget->init = GUI_FALSE;
    widget->xml_filename = NULL;

    child = widget_get_firstchild(widget);
    while(child)
    {
	widget_quit(child);
	child = widget_get_nextsibling(child);
    }

    return GXCORE_SUCCESS;
}

static status_t create_freedom_widget_list(GuiWidget *parent_widget, GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr cur = XML_GET_CHILD(GXxml_node);

	while (NULL != cur)
	{
		if (0 == GXxmlStrcmp(cur->name, WIDGET_STR))
		{
			create_widget_list(parent_widget, cur);
		}

		cur = cur->next;
	}

	return GXCORE_SUCCESS;
}

static void _global_signal_free(void *data)
{
	if(data) {
		GUI_FREE(data);
	}
}

int connect_global_signal(GuiConfig *config, const char *name, unsigned char *content)
{
	if((NULL == config) || (NULL == name) || (NULL == content)) {
		return (-1);
	}

	if(NULL == config->signal) {
		config->signal = hash_new(50, _global_signal_free);
	}

	if(config->signal) {
		if(hash_get(config->signal, name)) {
			gxlogd("[GUI] Global signal %s is existed.\n", name);
			return (-1);
		} else {
			hash_add(config->signal, name, content);
		}
	}

	return (0);
}

static void create_global_signal(GuiConfig *config, GXxmlNodePtr node)
{
	GXxmlNodePtr cur;
	xmlChar *content = NULL;

	if(NULL == config->signal) {
		config->signal = hash_new(50, _global_signal_free);
	}

	if(config->signal && node) {
		cur = node;
		while(cur) {
			content = GXxmlNodeGetContent(cur);
			connect_global_signal(config, (const char *)cur->name, content);
			cur = cur->next;
		}
	}
}

status_t create_interface(GuiConfig *config, GXxmlNodePtr GXxml_node)
{
	xmlChar *content = NULL;
	xmlChar *GXxml_color = NULL;
	int osd_alpha_color = 0;
	int osd_alpha_value = 0;

	content = GXxmlNodeGetContent(GXxml_node);

	if((0 == GXxmlStrcmp(WIDTH_STR, GXxml_node->name)) && (config->width == 0))
	{
		config->width  = atoidef((char *)content, 704);
	}
	else if((0 == GXxmlStrcmp(HEIGHT_STR, GXxml_node->name)) && (config->height == 0))
	{
		config->height = atoidef((char *)content, 576);
	}
	else if((0 == GXxmlStrcmp(X_POSITION, GXxml_node->name)) &&
			(config->x == 0) &&
			(config->name))
	{
		config->x = atoidef((char *)content, 0);
	}
	else if((0 == GXxmlStrcmp(Y_POSITION, GXxml_node->name)) &&
			(config->y == 0) &&
			(config->name))
	{
		config->y = atoidef((char *)content, 0);
	}
	else if((0 == GXxmlStrcmp(BPP_STR, GXxml_node->name)) && (config->bpp == 0))
	{
		config->bpp = atoidef((char *)content, 8);
		if(8 == config->bpp)
		{
			if(NULL != config->files.file_pal)
			{
				config->clut = gui_set_color_from_pal((const char *)config->files.file_pal);
				if(NULL == config->clut)
				{
					gxlogd( "[GUI]Illeagal CLUT\n");
				}
			}
		}
	}
	else if(0 == GXxmlStrcmp(BPP8_MULTCLUT_STR, GXxml_node->name))
	{
		GUI_CONFIG_STATE(config->bpp8_multclut, content);
	}
	else if(0 == GXxmlStrcmp(OSD_TRANS_STR, GXxml_node->name))
	{
		if(NULL != content && '#' == *content)
		{
			config->osd_trans = hexToInt((const char *)(content + 1));
			gal_set_osd_transparent(config->osd_trans, 0);
		}
	}
	else if(0 == GXxmlStrcmp(GUI_TRANS_STR, GXxml_node->name))
	{
		if(NULL != content && '#' == *content)
		{
			config->gui_trans = hexToInt((const char *)(content + 1));
			gal_set_gui_transparent(config->gui_trans);
		}
	}
	else if(0 == GXxmlStrcmp(OSD_ALPHA_GLOBAL, GXxml_node->name))
	{
		if(NULL != content && '#' == *content)
		{
			config->osd_alpha = hexToInt((const char *)(content + 1));
		}
	}
	else if(0 == GXxmlStrcmp(OSD_ALPHA_COLOR, GXxml_node->name))
	{
		GXxml_color = GXxmlGetProp(GXxml_node, COLOR );
		osd_alpha_color = hexToInt((const char *)(GXxml_color + 1));
		XML_FREE(GXxml_color);

		if(NULL != content && '#' == *content)
		{
			osd_alpha_value = hexToInt((const char *)(content + 1));
			gui_set_osdalpha_color(osd_alpha_color, osd_alpha_value);
		}
	}
	else if(0 == GXxmlStrcmp(CACHE_WIDGETS_STR, GXxml_node->name)) {
		if((content) && (0 == strcasecmp("disable", (const char *)content))) {
			config->cache_widgets = GUI_FALSE;
		} else {
			// Default cache all widgets
			config->cache_widgets = GUI_TRUE;
		}
	}
	else if(0 == GXxmlStrcmp(MIRROR_STR, GXxml_node->name)) {
		if((content) && (0 == strcasecmp("enable", (const char *)content))) {
			config->mirror = GUI_TRUE;
		}
	} else if(0 == GXxmlStrcmp(KEYPRESS_REVERT_STR, GXxml_node->name)) {
		if((content) && (0 == strcasecmp("enable", (const char *)content))) {
			config->keypress_revert = GUI_TRUE;
		}
	} else if(0 == GXxmlStrcmp(SIGNAL_STR, GXxml_node->name)) {
		create_global_signal(config, XML_GET_CHILD(GXxml_node));
	}

	XML_FREE(content);

	return GXCORE_SUCCESS;
}

static void widget_auto_load_thread(void *data)
{
	GuiWidget *root = NULL, *widget = NULL;
	const char *value = NULL;

	GxCore_ThreadDetach();
	root = (GuiWidget *)data;
	if(NULL == root) {
		return;
	}

	widget = widget_get_firstchild(root);
	if(NULL == widget) {
		return;
	}

	while(widget) {
		value = widget_get_property(widget, (const char *)PRELOAD_STR);
		if((value) &&
		   (0 == strcasecmp("true", value)) &&
		   (GUI_FALSE == widget->init)) {
			if(widget->widget_mutex <= 0) {
				GxCore_MutexCreate(&(widget->widget_mutex));
				if(widget->widget_mutex < 0) {
					return;
				}
			}
			widget_init(widget);
			widget->init = GUI_TRUE;
		}
		widget = widget_get_nextsibling(widget);
	}
}

static void _load_widgets_thread(GuiConfig* config)
{
	GxCore_ThreadCreate("widget_auto_load_thread",
			    &(config->widget_thread_handle),
			    widget_auto_load_thread,
			    (void *)(config->root_widget),
			    1024 * 50,
			    GXOS_DEFAULT_PRIORITY + 1);
}

status_t gui_xml_widget(GuiConfig* config, const char *filename)
{
	GXxmlDocPtr doc;

	if(NULL == config || NULL == filename)
	{
		return GXCORE_ERROR;
	}

	config->osd_trans = 0x000000;
	config->gui_trans = 0xFF00FF;
	config->osd_alpha = 0x0000FF;
	config->cache_widgets = GUI_TRUE;
	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		gxlogd( "[GUI]XML Document widget parse error.\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if (root == NULL)
	{
		gxlogd( "[GUI]XML Document widget empty\n");
		GXxmlFreeDoc(doc);
		return GX_ERROR_XML_DOC_EMPTY;
	}

	GXxmlNodePtr cur = XML_GET_CHILD(root);
	int widget_num = 0;
	while (NULL != cur) {
		if (0 == GXxmlStrcmp(cur->name, WIDGET_STR)) {
			create_widget_list(config->root_widget, cur);
			widget_num++;
		}
		else if (0 == GXxmlStrcmp(cur->name, FREEDOM_WIDGET_STR))
			create_freedom_widget_list(config->freedom_widget, cur);
		//		else if (0 == GXxmlStrcmp(cur->name, COLORS_STR))
		//			create_color_list(config->colors, cur);
		//		else if (0 == GXxmlStrcmp(cur->name, FONTS_STR))
		//			create_font_list(cur);
		else
			create_interface(config, cur);

		cur = cur->next;
	}
	GXxmlFreeDoc(doc);
	_load_widgets_thread(config);

	//	_widget_interface_print(gui_core);
	//	gxlogd("widget num: %d\n", widget_num);

	return GXCORE_SUCCESS;
}

status_t gui_xml_widget_freedata(GuiConfig *cfg)
{
	GuiClut *pclut = NULL, *pnext = NULL;

	if(NULL == cfg)
	{
		return GXCORE_ERROR;
	}

	pclut = cfg->clut;
	while(pclut)
	{
		pnext = pclut->next;
		GUI_FREE(pclut->palette);
		GUI_FREE(pclut);
		pclut = pnext;
	}

	cfg->clut = NULL;

	if(cfg->root_widget)
	{
		widget_del_tree(cfg->root_widget);
		cfg->root_widget = NULL;
	}
	if(cfg->freedom_widget)
	{
		widget_del_tree(cfg->freedom_widget);
		cfg->freedom_widget = NULL;
	}

	if(cfg->signal_table)
	{
		hash_release(cfg->signal_table);  //shencz add
		cfg->signal_table = NULL;
	}
	if(cfg->color_hash)
	{
		hash_release(cfg->color_hash);
		cfg->color_hash = NULL;
	}
	if(cfg->signal) {
		hash_release(cfg->signal);
		cfg->signal= NULL;
	}

	return GXCORE_SUCCESS;
}

