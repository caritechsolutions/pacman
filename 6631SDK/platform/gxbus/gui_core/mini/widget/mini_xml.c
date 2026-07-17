#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

extern MiniGUIConfig minigui_config;

#define XML_FREE(x) if(x){GxCore_Free(x);x=NULL;}
#define XML_GET_CHILD(node) node->childs
#define XML_GET_ROOT(doc)   doc->root

static int atoidef(const char *nptr, int v)
{
	if (nptr)
		return atoi(nptr);
	else
		return v;
}

static int _minigui_font_style(char *style)
{
#ifdef GUI_TTF
	if(0 == strcasecmp("normal", style))
	{
		return (TTF_STYLE_NORMAL);
	}
	else if(0 == strcasecmp("bold", style))
	{
		return (TTF_STYLE_BOLD);
	}
	else if(0 == strcasecmp("italic", style))
	{
		return (TTF_STYLE_ITALIC);
	}
	else if(0 == strcasecmp("underline", style))
	{
		return (TTF_STYLE_UNDERLINE);
	}
#endif
	return (0);
}

static void _minigui_xml_fonts(GXxmlNodePtr cur)
{
	char *name = NULL, *filename = NULL, *value = NULL, *ptr = NULL;
	int size = 0, style = 0;
	GXxmlNodePtr child_cur;

	if(0 != strcmp("fonts", (char *)cur->name))
	{
		gxlogd("[MINIGUI] There is no fonts XML.\n");
		return;
	}

	cur = XML_GET_CHILD(cur);
	while(NULL != cur)
	{
		if(0 == strcmp("font", (char *)cur->name))
		{
			child_cur = XML_GET_CHILD(cur);

			while(NULL != child_cur)
			{
				if(0 == strcmp("name", (char *)child_cur->name))
				{
					XML_FREE(name);
					name = (char *)GXxmlNodeGetContent(child_cur);
				}
				else if(0 == strcmp("file", (char *)child_cur->name))
				{
					XML_FREE(filename);
					filename = (char *)GXxmlNodeGetContent(child_cur);
				}
				if(0 == strcmp("size", (char *)child_cur->name))
				{
					value = (char *)GXxmlNodeGetContent(child_cur);
					size = atoidef(value, 0);
					XML_FREE(value);
				}
				else if(0 == strcmp("style", (char *)child_cur->name))
				{
					value = (char *)GXxmlNodeGetContent(child_cur);
					style = _minigui_font_style(value);
					XML_FREE(value);
				}

				if(name && filename)
				{
					ptr = filename + strlen(filename) - 3;
					if((0 == strcasecmp("ttf", ptr)) && (0 == size))
					{
						child_cur = child_cur->next;
						continue;
					}
					minigui_load_font(name, filename, size, style);
				}
				child_cur = child_cur->next;
			}
		}
		cur = cur->next;
	}
	XML_FREE(name);
	XML_FREE(filename);
}

static void _minigui_xml_images(GXxmlNodePtr cur)
{
	char *key = NULL, *filename = NULL;
	image_desc *desc = NULL;

	if(0 != strcmp("images", (char *)cur->name))
	{
		gxlogd("[MINIGUI] There is no images XML.\n");
		return;
	}

	cur = XML_GET_CHILD(cur);
	while(NULL != cur)
	{
		if(0 == strcmp("image", (char *)cur->name))
		{
			key = (char *)GXxmlGetProp(cur, (unsigned char *)"id");
			filename = (char *)GXxmlNodeGetContent(cur);

			desc = minigui_load_image(filename);

			if(minigui_config.image == NULL)
			{
				minigui_config.image = hash_new(10, NULL);
			}

			if(NULL == minigui_config.image)
			{
				XML_FREE(key);
				XML_FREE(filename);
				return;
			}

			hash_add(minigui_config.image, key, (void *)desc);

			XML_FREE(key);
			XML_FREE(filename);
		}
		cur = cur->next;
	}
}

static void _add_widget_list(GuiWidget *parent, GXxmlNodePtr cur);
static void _widget_parse(GuiWidget *widget, GXxmlNodePtr cur)
{
	while(cur)
	{
		if(0 == strcmp("property", (char *)cur->name))
		{
			GXxmlNodePtr property_node = XML_GET_CHILD(cur);
			while(property_node)
			{
				char *value = (char *)GXxmlNodeGetContent(property_node);
				widget_set_property(widget, (const char *)property_node->name, (const char *)value);
				XML_FREE(value);

				property_node = property_node->next;
			}
		}
		else if(0 == strcmp("signal", (char *)cur->name))
		{
			GXxmlNodePtr signal_node = XML_GET_CHILD(cur);
			while(signal_node)
			{
				char *value = (char *)GXxmlNodeGetContent(signal_node);

				widget_signal_new(widget, (char *)signal_node->name, value);
				XML_FREE(value);
				signal_node = signal_node->next;
			}
		}
		else if(0 == strcmp("children", (char *)cur->name))
		{
			GXxmlNodePtr child_node = XML_GET_CHILD(cur);

			while (NULL != child_node)
			{
				if(0 == strcmp("widget", (char *)child_node->name))
				{
					_add_widget_list(widget, child_node);
				}

				child_node = child_node->next;
			}
		}
		cur = cur->next;
	}
}

static void _add_widget_list(GuiWidget *parent, GXxmlNodePtr cur)
{
	GuiWidget *widget = NULL;

	char *widget_name = (char *)GXxmlGetProp(cur, (unsigned char *)"name");
	char *class_name = (char *)GXxmlGetProp(cur, (unsigned char *)"class");

	widget = widget_new_child(parent, widget_name, class_name, NULL);
	if(NULL == widget)
	{
		gxlogd( "[MINIGUI]widget_new_child(""%s"", ""%s"") = NULL\n", (char *)widget_name, (char *)class_name);
		return;
	}

	XML_FREE(widget_name);
	XML_FREE(class_name);

	_widget_parse(widget, XML_GET_CHILD(cur));
}

static void _minigui_xml_widgets(GXxmlNodePtr cur)
{
	if(0 != strcmp("widget", (char *)cur->name))
	{
		gxlogd("[MINIGUI] There is no widget XML.\n");
		return;
	}

	if(NULL == minigui_config.root_widget)
	{
		minigui_config.root_widget = widget_new("root", "frame", NULL);
	}

	if(NULL == minigui_config.root_widget)
	{
		gxlogd("[MINIGUI] Cannot create root widget.\n");
		return;
	}

	_add_widget_list(minigui_config.root_widget, cur);
}

status_t minigui_xml_parse(char *path)
{
	status_t ret = GXCORE_ERROR;
	GXxmlDocPtr doc;
	char *value = NULL;

	minigui_config.theme_path = GUI_STRDUP(path);

	doc = GXxmlParseFile(path);
	if(NULL == doc)
	{
		gxlogd("[MINIGUI] XML Document config '%s' parse error.\n", path);
		return (ret);
	}

	GXxmlNodePtr root = XML_GET_ROOT(doc);
	if(NULL == root)
	{
		gxlogd( "[GUI]XML Document config empty\n");
		GXxmlFreeDoc(doc);
		return (ret);
	}

	GXxmlNodePtr cur = XML_GET_CHILD(root);
	while (NULL != cur) {
		value = (char *)GXxmlNodeGetContent(cur);
		if(0 == strcmp((char *)cur->name, "width"))
		{
			minigui_config.width = atoidef(value, 1280);
		}
		else if(0 == strcmp((char *)cur->name, "height"))
		{
			minigui_config.height = atoidef(value, 720);
		}
		else if(0 == strcmp((char *)cur->name, "bpp"))
		{
			minigui_config.bpp = atoidef(value, 16);
		}
		else if(0 == strcmp((char *)cur->name, "fonts"))
		{
			_minigui_xml_fonts(cur);
		}
		else if(0 == strcmp((char *)cur->name, "images"))
		{
			_minigui_xml_images(cur);
		}
		else if(0 == strcmp((char *)cur->name, "widget"))
		{
			_minigui_xml_widgets(cur);
		}
		XML_FREE(value);
		cur = cur->next;
	}

	GXxmlFreeDoc(doc);
	ret = GXCORE_SUCCESS;
	return (ret);
}

