/*************************************************************************
	> File Name: app_bsp_parse.c
	> Author:
	> Mail:
	> Created Time: 2019年11月06日 星期三 18时29分31秒
 ************************************************************************/
#include"app_bsp.h"
#include <memmgr.h>

#define BSP_STR      (xmlChar*)"config"
#define ITEM_STR       (xmlChar*)"item"
#define GROUP_NAME     (xmlChar*)"name"
#define ITEM_DISP      (xmlChar*)"display"
#define ITEM_VAL       (xmlChar*)"value"
#define LIST_VAL       (xmlChar*)"value"
#define KEY_NAME       (xmlChar*)"name"


/*************** xml parser start ****************/
static status_t info_get_list_item(GXxmlNodePtr xml_item, xmlChar *name, char *buf, size_t size)
{
    GXxmlNodePtr item = NULL;
    xmlChar *value = NULL;
    xmlChar *iname = NULL;

    item =  xml_item->childs;
    while (item != NULL) {
        iname = GXxmlGetProp(item, LIST_VAL);

        if (GXxmlStrcmp(iname, name) == 0) {
            value = GXxmlGetProp(item, ITEM_DISP);
            strncpy(buf, (char *)value, size);

            GXxmlFree(iname);
            GXxmlFree(value);

            return GXCORE_SUCCESS;
        }
        GXxmlFree(iname);
        item = item->next;
    }

    GXxmlFree(value);

    return GXCORE_ERROR;
}

static status_t info_get(GXxmlNodePtr GXxml_item, xmlChar *name, char *buf, size_t size, int mode)
{
    GXxmlNodePtr item = NULL;
    xmlChar *value = NULL;
    xmlChar *iname = NULL;

    item =  GXxml_item->childs;
    while (item != NULL) {
        iname = GXxmlGetProp(item, ITEM_DISP);

        if (GXxmlStrcmp(iname, name) == 0) {
            value = GXxmlGetProp(item, ITEM_VAL);
            strncpy(buf, (char *)value, size);
            if (mode) {
                info_get_list_item(item, value, buf, size);
            }
            GXxmlFree(iname);
            GXxmlFree(value);

            return GXCORE_SUCCESS;
        }
        GXxmlFree(iname);
        item = item->next;
    }

    GXxmlFree(value);

    return GXCORE_ERROR;
}

static status_t key_get(GXxmlNodePtr xml_item, xmlChar *name, uint32_t *keyvalue)
{
    GXxmlNodePtr item = NULL;
    xmlChar *value = NULL;
    xmlChar *iname = NULL;
    uint32_t panel_keymap = 0;

    item =  xml_item->childs;
    while (item != NULL) {
        iname = GXxmlGetProp(item, KEY_NAME);

        if (GXxmlStrcmp(iname, name) == 0) {

            value = GXxmlNodeGetContent(item);
            if (value != NULL) {
                panel_keymap = app_hexstr2value(value);
                //	app_log_debug("key=%d\n",panel_keymap);
            }

            //strncpy(buf, (char*)value, size);
            *keyvalue = panel_keymap;

            GXxmlFree(iname);
            GXxmlFree(value);

            return GXCORE_SUCCESS;
        }
        GXxmlFree(iname);
        item = item->next;
    }

    GXxmlFree(value);

    return GXCORE_ERROR;

}

/*************** GXxml parser end ****************/


status_t bsp_parser(const char *filename, BspItem *item, char *buf, size_t size, int mode)
{
    GXxmlDocPtr doc;
    xmlChar *gname = NULL;
    status_t ret = GXCORE_ERROR;

    if (NULL == filename) {
        return ret;
    }

    doc = GXxmlParseFile(filename);
    if (NULL == doc) {
        app_log_error("[app panel]GXxml parse error.\n");
        return ret;
    }

    // root
    GXxmlNodePtr root = doc->root;
    if (root == NULL) {
        app_log_error("[app panel]GXxml empty\n");
        GXxmlFreeDoc(doc);
        return ret;
    }

    if (0 !=  GXxmlStrcmp(root->name, BSP_STR)) {
        GXxmlFreeDoc(doc);
        return ret;
    }

    // group
    GXxmlNodePtr group = root->childs;
    while (NULL != group) {
        gname = GXxmlGetProp(group, GROUP_NAME);

        if (gname != NULL &&
                0 ==  GXxmlStrcmp(gname, (xmlChar *)(item->gname))) {
            ret = info_get(group, (xmlChar *)(item->iname), buf, size, mode);
            GXxmlFree(gname);
            GXxmlFreeDoc(doc);
            return ret;
        }

        GXxmlFree(gname);
        gname = NULL;
        group = group->next;
    }

    GXxmlFreeDoc(doc);
    return ret;
}


status_t app_get_bsp_key_list(const char *filename, char **key_name, BspItem *item, uint32_t *keyvalue, int size)
{

    xmlChar *gname = NULL;
    GXxmlDocPtr doc;
    uint8_t i = 0;

    doc = GXxmlParseFile(filename);
    if (NULL == doc) {
        app_log_error("[panel keymap]keyGXxml parse error.\n");
        return GXCORE_ERROR;
    }

    // root
    GXxmlNodePtr root = doc->root;
    if (root == NULL) {
        app_log_error("[panel keymap]keyGXxml empty\n");
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    if (0 !=  GXxmlStrcmp(root->name, BSP_STR)) {
        GXxmlFreeDoc(doc);
        return GXCORE_ERROR;
    }

    // group
    GXxmlNodePtr group = root->childs;
    while (NULL != group) {
        gname = GXxmlGetProp(group, GROUP_NAME);

        if (gname != NULL &&
                0 ==  GXxmlStrcmp(gname, (xmlChar *)(item->gname))) {
            for (i = 0; i < size; i++) {
                key_get(group, (xmlChar *)(key_name[i]), &keyvalue[i]);
            }
            GXxmlFree(gname);
            GXxmlFreeDoc(doc);
            return GXCORE_SUCCESS;
        }

        GXxmlFree(gname);
        gname = NULL;
        group = group->next;
    }

    GXxmlFreeDoc(doc);
    return GXCORE_ERROR;
}
