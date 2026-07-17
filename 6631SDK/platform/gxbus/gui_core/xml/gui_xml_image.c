#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gui_private.h"
#include "gui_xml_parse.h"
#include "image_cache.h"
#include "IMG.h"
#include "gal.h"

extern void images_release_data(void *data);
extern void logo_Free(void *data);

status_t gui_xml_image(GuiConfig* config, const char *filename)
{
	GXxmlDocPtr doc;
	GXxmlNodePtr root;
	GXxmlNodePtr cur;
	xmlChar *key = NULL;
	xmlChar *flag = NULL;
	xmlChar *value = NULL;
	int preload = 0;

	if(filename == NULL)
		return GXCORE_ERROR;

	doc = GXxmlParseFile(filename);
	if (NULL == doc) {
		gxlogd( "[GUI]XML Document image parse error.\n");
		return GX_ERROR_XML_DOC_PARSE;
	}

	root = XML_GET_ROOT(doc);
	if (root == NULL) {
		gxlogd( "[GUI]XML Document image empty\n");
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	if (0 !=  GXxmlStrcmp(root->name, IMAGES_STR)) {
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	cur = XML_GET_CHILD(root);
	while (cur != NULL) {
		if (GXxmlStrcmp(cur->name, IMAGE_STR) == 0) {
			preload = 0;
			key = GXxmlGetProp(cur, ID_STR);
			value = GXxmlNodeGetContent(cur);
			flag = GXxmlGetProp(cur, FLAG_STR);
			if((flag) && (0 == strcasecmp((const char *)PRELOAD_STR, (const char*)flag)))
				preload = 1;

			image_cache_put(config, (char*)key, (char*)value, preload);

			XML_FREE(key);
			XML_FREE(value);
			XML_FREE(flag);
		}
		else if(0 == GXxmlStrcmp(cur->name, DEFAULT_PAL_STR))
		{
		    value = GXxmlNodeGetContent(cur);
		    gal_add_palette(config, (const char *)value);
		    XML_FREE(value);
		}
		cur = cur->next;
	}
	GXxmlFreeDoc(doc);

	return GXCORE_SUCCESS;
}

status_t gui_xml_image_freedata(void *data)
{
	return image_cache_free( (ImageCache*)data);
}


