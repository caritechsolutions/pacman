#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gui_private.h"
#include "gui_xml_parse.h"
#include "gui.h"

static int key_string2value(xmlChar* string)
{
	int ret = 0;
	char *pstr = NULL;

	if(NULL == string)
	{
		return 0;
	}

	//hex
	if(strlen((char*)string) > 2)
	{
		if('0' == string[0] && 'x' == string[1])
		{
			pstr = (char*)&string[2];
			while(*pstr)
			{
				if (*pstr>='0' && *pstr<='9')
				{
					ret=ret * 16 + ((int)*pstr-'0');
				}
				else
				{
					if (*pstr>='A' && *pstr<='F')
					{
						ret=ret * 16 +((int)*pstr-'A'+10);
					}
					else if (*pstr>='a' && *pstr<='f')
					{
						ret=ret *16 + ((int)*pstr-'a'+10);
					}
				}
				pstr++;
			}
			return ret;
		}
	}

	//dec
	pstr = (char*)string;
	while(*pstr)
	{
		if (*pstr>='0' && *pstr<='9')
		{
			ret=ret * 10 + ((int)*pstr-'0');
		}
		else
		{
			gxlogd("[GUI]=key_string2value=error value '%s'\n", string);
			return 0;
		}
		pstr++;
	}

	return (ret);
}

static void _connect_key_process(xmlChar *value)
{
	GuiCore *gui_core = GUI_GetCurrent();
    GuiSignal *key_signal = NULL;

    if(NULL == gui_core->config.signal_table)
    {
	gxlogd("[GUI] No signal table!\n");
	return;
    }

    key_signal = (GuiSignal*)hash_get(gui_core->config.signal_table, (const char *)value);
    if(NULL == key_signal)
    {
	gxlogd("[GUI] No signal %s!\n", value);
	return;
    }

    gui.key_process = key_signal;
}

static void _config_keymap_mask(xmlChar* str_key)
{
    if(10 == strlen((const char *)str_key))
    {
	gui.config.key_mask = 0xFFFFFFFF;
    }
    else if(8 == strlen((const char *)str_key))
    {
	gui.config.key_mask = 0x00FFFFFF;
    }
    else
    {
	gui.config.key_mask = 0x0000FFFF;
    }
}

static status_t create_key_node(GuiConfig *config, GXxmlNodePtr GXxml_node)
{
	GXxmlNodePtr node = NULL;
	xmlChar *name= NULL;
	xmlChar *str_keycode = NULL;
	int hw_keycode = 0;
	int ret = 1;

	node =  GXxml_node;
	name = GXxmlGetProp(node, NAME_STR );
	str_keycode = GXxmlNodeGetContent(node);

	if(name && str_keycode)
	{
		hw_keycode = key_string2value(str_keycode);
		_config_keymap_mask(str_keycode);

		if (GXxmlStrcmp(GXxml_node->name, KEY_STR) == 0)
		    ret = key_map((void *)config, (char*)name, hw_keycode);
		else if (GXxmlStrcmp(GXxml_node->name, VIRTUALKEY_STR) == 0)
		    ret = virtualkey_map((void *)config, (char*)name, hw_keycode);

		if(-1 == ret)
			gxlogd( "[GUI]=ERROR=cant map key '%s'-0x%x\n", name, hw_keycode);
		else
			GUI_Printf("[GUI]key 0x%x -> '%s'\n", hw_keycode, name);
	}
	XML_FREE(name);
	XML_FREE(str_keycode);

	return GXCORE_SUCCESS;
}


status_t gui_xml_keymap(GuiConfig *config, const char *filename)
{
	GXxmlDocPtr doc;
	GXxmlNodePtr cur_child;
	xmlChar *str = NULL;
	//xmlChar *str_keycode = NULL;

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


	if (0 !=  GXxmlStrcmp(root->name, KEYMAP_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	GXxmlNodePtr cur = XML_GET_CHILD(root);

	if((0 == GXxmlStrcmp(GROUP_STR, cur->name)) ||
	   (0 == GXxmlStrcmp(TREE_STR, cur->name))) {
		while(NULL != cur) {
			if(0 == GXxmlStrcmp(GROUP_STR, cur->name)) {
				cur_child = XML_GET_CHILD(cur);
				while(cur_child) {
					create_key_node(config, cur_child);
					cur_child = cur_child->next;
				}
			} else if(0 == GXxmlStrcmp(KEY_PROCESS, cur->name)) {
				cur_child = XML_GET_CHILD(cur);
				str = GXxmlNodeGetContent(cur_child);
				if(str)
					_connect_key_process(str);
				XML_FREE(str);
			}
			cur = cur->next;
		}

		GXxmlFreeDoc(doc);
		return GXCORE_SUCCESS;
	}
	else
	{
		while (NULL != cur)
		{
			create_key_node(config, cur);
			cur = cur->next;
		}

		GXxmlFreeDoc(doc);
		return GXCORE_SUCCESS;
	}
}

status_t gui_xml_keymap_freedata(GuiConfig* config)
{
	return (key_data_free((void *)config));
}


