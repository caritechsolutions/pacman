#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <gx_apps.h>
#include <memmgr.h>
#include "app_iptv_list_api.h"
#include "app.h"
#include "app_iptv.h"
//#define IPTV_LIST_DEBUG
#ifdef IPTV_LIST_DEBUG
#define IPTV_PRINTF(...)    printf(__VA_ARGS__)
#else
#define IPTV_PRINTF(...)    do{}while(0)
#endif

#define XML_STR_LIST    "list"
#define XML_STR_ITEM    "item"
#define XML_STR_NAME    "key"
#define XML_STR_URL    "value"
#define XML_STR_TYPE    "type"
#define XML_STR_AD    "ad"
#define XML_STR_AD_NAME    "ad_name"
#define XML_STR_AD_TYPE    "ad_type"
#define XML_STR_AD_VALUE    "ad_value"
#define XML_STR_AV    "av"
#define XML_STR_PIC    "picture"
#define TXT_LINE_BUFFER_LEN 4096

#ifdef LINUX_OS
#define IPTV_LOCAL_DATABASE    "/home/gx/local/iptv/iptv.xml"
#define IPTV_FAV_DATABASE    "/home/gx/local/iptv/iptv_fav.xml"
#define IPTV_REMOTE_DATABASE    "/home/gx/local/iptv/iptv_remote.xml"
#define IPTV_AD_PIC    "/tmp/iptv/ad.jpg"
#endif

#ifdef ECOS_OS
#define IPTV_LOCAL_DATABASE    "/home/gx/iptv.xml"
#define IPTV_FAV_DATABASE    "/home/gx/iptv_fav.xml"
#define IPTV_REMOTE_DATABASE    "/home/gx/iptv_remote.xml"
#define IPTV_AD_PIC    "/mnt/ad.jpg"
#endif

typedef struct _QueryPairClass
{
    char *query_key;
    char *query_value;
}QueryPairClass;

typedef struct _QueryListClass
{
    QueryPairClass *query_list;
    int query_num;
}QueryListClass;

char* GxCore_Strndup(const char* src, int len)
{
	char *p = NULL;

	if(strlen(src) < len)
	{
		printf("[GxCore_Strndup] length err!!!\n");
		return NULL;
	}

	p = (char*)GxCore_Calloc(len+1 , sizeof(char));

	if(p == NULL)
	{
		printf("[GxCore_Strndup] calloc err!!!\n");
		return NULL;
	}

	strncpy(p, src, len);

	return p;
}

IptvListClass *_iptv_list_new(void)
{
    IptvListClass *list = NULL;

    list = (IptvListClass *)GxCore_Malloc(sizeof(IptvListClass));
    memset(list, 0, sizeof(IptvListClass));
    list->iptv_item_list = (IptvItemClass *)GxCore_Malloc(LIST_MAX_ITEM_NUM*sizeof(IptvItemClass));
    memset(list->iptv_item_list, 0, LIST_MAX_ITEM_NUM*sizeof(IptvItemClass));

    return list;
}

static IptvListClass *_get_list_from_txt_file(const char *path)
{
    IptvListClass *list = NULL;
    char *buffer = NULL;
    FILE *fp_handle = NULL;
    char *end=NULL, *temp=NULL;
    char *key=NULL, *value=NULL, *type=NULL;

    IPTV_PRINTF("get_list_from_txt_file\n");
    fp_handle = fopen(path,"rb");
    if(fp_handle)
    {
        list = _iptv_list_new();
        buffer = (char *)GxCore_Malloc(TXT_LINE_BUFFER_LEN);
        while((fgets(buffer,TXT_LINE_BUFFER_LEN,fp_handle))&&(list->iptv_item_num<LIST_MAX_ITEM_NUM))
        {
            if(strlen(buffer)>3)
            {
                if(buffer[strlen(buffer)-1]=='\n')
                {
                    buffer[strlen(buffer)-1] = '\0';
                }

                key = NULL;
                value = NULL;
                type = NULL;

                //get key
                temp=buffer;
                end=strchr(temp, ',');
                if(end&&end>(temp+1))
                {
                    key = GxCore_Strndup(temp, end-temp);
                    IPTV_PRINTF("key=%s\n", key);

                    //get value
                    temp=end+1;
                    end=strchr(temp, ',');
                    if(end&&end>(temp+1))
                    {
                        value = GxCore_Strndup(temp, end-temp);
                        IPTV_PRINTF("value=%s\n", value);

                        //get type
                        temp=end+1;
                        end=strchr(temp, ',');
                        if(end&&end>(temp+1))
                        {
                            type = GxCore_Strndup(temp, end-temp);
                            IPTV_PRINTF("type=%s\n", type);
                        }
                        else
                        {
                            if(strlen(temp)>1)
                            {
                                type = GxCore_Strdup(temp);
                                IPTV_PRINTF("type=%s\n", type);
                            }
                        }

                    }
                    else
                    {
                        if(strlen(temp)>1)
                        {
                            value = GxCore_Strdup(temp);
                            IPTV_PRINTF("value=%s\n", value);
                        }
                    }
                }

                if(key&&value)
                {
                    list->iptv_item_list[list->iptv_item_num].iptv_key = key;
                    list->iptv_item_list[list->iptv_item_num].iptv_value = value;
                    list->iptv_item_list[list->iptv_item_num].iptv_type = type;
                    list->iptv_item_num++;
                }
                else
                {
                    APP_FREE(key);
                    APP_FREE(value);
                    APP_FREE(type);
                }
            }
            memset(buffer, 0, TXT_LINE_BUFFER_LEN);
        }
        APP_FREE(buffer);
        fclose(fp_handle);
    }
    return list;
}

#if 1//def USE_GX_XML
#include <parser.h>
#include <tree.h>
static int _iptv_parse_ad(GXxmlNodePtr ad_node, IptvAdClass *ad_ret)
{
    GXxmlNodePtr item = NULL;
    char *name = NULL;
    char *type = NULL;
    char *value = NULL;

    if((ad_node == NULL) || (ad_ret == NULL))
        return -1;

    item = ad_node->childs;
    while(item)
    {
        if(0 == GXxmlStrcmp(item->name, (const unsigned char *)XML_STR_AD_NAME))
        {
            name = (char *)GXxmlNodeGetContent(item);
        }
        else if(0 == GXxmlStrcmp(item->name, (const unsigned char *)XML_STR_AD_TYPE))
        {
            type = (char *)GXxmlNodeGetContent(item);
        }
        else if(0 == GXxmlStrcmp(item->name, (const unsigned char *)XML_STR_AD_VALUE))
        {
            value = (char *)GXxmlNodeGetContent(item);
        }

        item = item->next;
    }

    if(name && type && value)
    {
        IPTV_PRINTF("ad name = %s\n", name);
        IPTV_PRINTF("ad type = %s\n", type);
        IPTV_PRINTF("ad value = %s\n", value);

        ad_ret->ad_name = GxCore_Strdup(name);
        ad_ret->ad_type = GxCore_Strdup(type);
        ad_ret->ad_value = GxCore_Strdup(value);
    }
    GXxmlFree(name);
    GXxmlFree(type);
    GXxmlFree(value);

    return 0;
}

static IptvListClass *_get_list_from_xml_file(const char *path)
{
    GXxmlDocPtr doc = NULL;
    GXxmlNodePtr root = NULL;
    GXxmlNodePtr item = NULL;
    GXxmlNodePtr item1 = NULL;
    char *key = NULL;
    char *value = NULL;
    char *type = NULL;
    char *ad = NULL;
    IptvAdClass iptv_ad = {0};
    IptvListClass *list = NULL;

    IPTV_PRINTF("get_list_from_xml_file\n");
    if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
        return NULL;

    doc = GXxmlParseFile(path);
    if(doc)
    {
        root = doc->root;
        if((!root )||(0 !=  GXxmlStrcmp(root->name, (const unsigned char *)XML_STR_LIST)))
        {
            GXxmlFreeDoc(doc);
            app_log_error("\033[1;31m[%s] no valid root node\n\033[0m", __func__);
            return NULL;
        }
    }
    else
    {
        app_log_error("\033[1;31m[%s] list parser error\n\033[0m", __func__);
        return NULL;
    }

    list = _iptv_list_new();
    item = root->childs;
    while(item && (list->iptv_item_num < LIST_MAX_ITEM_NUM))
    {
        if(0 ==  GXxmlStrcmp(item->name, (const unsigned char *)XML_STR_ITEM))
        {
            item1 = item->childs;
            key = NULL;
            value = NULL;
            type = NULL;
            memset(&iptv_ad, 0, sizeof(IptvAdClass));
            while(item1)
            {
                if((!key)&&(0 == GXxmlStrcmp(item1->name, (const unsigned char *)XML_STR_NAME)))
                {
                    key = (char *)GXxmlNodeGetContent(item1);
                }
                else if((!value)&&(0 == GXxmlStrcmp(item1->name, (const unsigned char *)XML_STR_URL)))
                {
                    value = (char *)GXxmlNodeGetContent(item1);
                }
                else if((!type)&&(0 == GXxmlStrcmp(item1->name, (const unsigned char *)XML_STR_TYPE)))
                {
                    type = (char *)GXxmlNodeGetContent(item1);
                }
                else if((!ad) && (0 == GXxmlStrcmp(item1->name, (const unsigned char *)XML_STR_AD)))
                {
                    _iptv_parse_ad(item1, &iptv_ad);
                }
                item1 = item1->next;
            }
            if(key && value)
            {
                IPTV_PRINTF("key=%s\n", key);
                IPTV_PRINTF("value=%s\n", value);
                list->iptv_item_list[list->iptv_item_num].iptv_key = GxCore_Strdup(key);
                list->iptv_item_list[list->iptv_item_num].iptv_value = GxCore_Strdup(value);
                if(type)
                {
                    IPTV_PRINTF("type=%s\n", type);
                    list->iptv_item_list[list->iptv_item_num].iptv_type = GxCore_Strdup(type);
                }
                list->iptv_item_list[list->iptv_item_num].iptv_ad = iptv_ad;
                list->iptv_item_num++;
            }
            GXxmlFree(key);
            GXxmlFree(value);
            GXxmlFree(type);
        }
        item = item->next;
    }

    GXxmlFreeDoc(doc);
    return list;
}

static void _iptv_list_save_to_file(IptvListClass *list, const char *path)
{
    GXxmlDocPtr doc = NULL;
    GXxmlNodePtr root = NULL;
    GXxmlNodePtr item = NULL;
    GXxmlNodePtr key = NULL;
    GXxmlNodePtr value = NULL;
    GXxmlNodePtr type = NULL;
    GXxmlNodePtr content = NULL;
    int i = 0;

    IPTV_PRINTF("gx_list_save_to_file\n");
    if((!path)||(strlen(path)==0))
    {
        IPTV_PRINTF("input error\n");
        return;
    }

    doc = GXxmlNewDoc((unsigned char *)"1.0");
    if(!doc)
    {
        return;
    }
    root = GXxmlNewDocNode(doc, NULL, (unsigned char *)XML_STR_LIST, NULL);
    if(!root)
    {
        GXxmlFreeDoc(doc);
        return;
    }

    doc->root = root;
    if(list && list->iptv_item_list && list->iptv_item_num > 0)
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)&&(strlen(list->iptv_item_list[i].iptv_key)>0)
                    &&(list->iptv_item_list[i].iptv_value)&&(strlen(list->iptv_item_list[i].iptv_value)>0))
            {
                item = GXxmlNewNode(NULL, (unsigned  char*)XML_STR_ITEM);
                GXxmlAddChild(root, item);

                //add key
                key = GXxmlNewNode(NULL, (unsigned  char*)XML_STR_NAME);
                GXxmlAddChild(item, key);

                content = GXxmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_key);
                GXxmlAddChild(key,content);

                //add value
                value = GXxmlNewNode(NULL, (unsigned  char*)XML_STR_URL);
                GXxmlAddChild(item, value);

                content = GXxmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_value);
                GXxmlAddChild(value,content);

                //add type if exist
                if((list->iptv_item_list[i].iptv_type)&&(strlen(list->iptv_item_list[i].iptv_type)>0))
                {
                    type = GXxmlNewNode(NULL, (unsigned  char*)XML_STR_TYPE);
                    GXxmlAddChild(item, type);

                    content = GXxmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_type);
                    GXxmlAddChild(type,content);
                }
            }
        }
    }

#ifdef ECOS_OS
    remove(path);
#endif
    GXxmlSaveFile(path, doc);
    GXxmlFreeDoc(doc);
}
#else
#include <libxml/parser.h>
#include <libxml/tree.h>
static int _iptv_parse_ad(xmlNodePtr ad_node, IptvAdClass *ad_ret)
{
    xmlNodePtr item = NULL;
    char *name = NULL;
    char *type = NULL;
    char *value = NULL;

    if((ad_node == NULL) || (ad_ret == NULL))
        return -1;

    item = ad_node->children;
    while(item)
    {
        if(0 == xmlStrcmp(item->name, (unsigned  char*)XML_STR_AD_NAME))
        {
            name = (char *)xmlNodeGetContent(item);
        }
        else if(0 == xmlStrcmp(item->name, (unsigned  char*)XML_STR_AD_TYPE))
        {
            type = (char *)xmlNodeGetContent(item);
        }
        else if(0 == xmlStrcmp(item->name, (unsigned  char*)XML_STR_AD_VALUE))
        {
            value = (char *)xmlNodeGetContent(item);
        }

        item = item->next;
    }

    if(name && type && value)
    {
        IPTV_PRINTF("ad name = %s\n", name);
        IPTV_PRINTF("ad type = %s\n", type);
        IPTV_PRINTF("ad value = %s\n", value);

        ad_ret->ad_name = name;
        ad_ret->ad_type = type;
        ad_ret->ad_value = value;
    }
    else
    {
        if(name)
        {
            free(name);
        }
        if(type)
        {
            free(type);
        }
        if(value)
        {
            free(value);
        }
    }

    return 0;
}

static IptvListClass *_get_list_from_xml_file(char *path)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlNodePtr item = NULL;
    xmlNodePtr item1 = NULL;
    char *key = NULL;
    char *value = NULL;
    char *type = NULL;
    char *ad = NULL;
    IptvAdClass iptv_ad = {0};
    IptvListClass *list = NULL;

    IPTV_PRINTF("get_list_from_xml_file\n");
    doc = xmlReadFile(path, NULL, XML_PARSE_NOBLANKS);
    if(doc)
    {
        root = xmlDocGetRootElement(doc);
        if((!root )||(0 !=  xmlStrcmp(root->name, (const xmlChar *)XML_STR_LIST)))
        {
            xmlFreeDoc(doc);
            IPTV_PRINTF("no valid root node\n");
            return NULL;
        }
    }
    else
    {
        IPTV_PRINTF("list parser error\n");
        return NULL;
    }

    list = _iptv_list_new();
    item = root->children;
    while(item && (list->iptv_item_num < LIST_MAX_ITEM_NUM))
    {
        if(0 ==  xmlStrcmp(item->name, (unsigned  char*)XML_STR_ITEM))
        {
            item1 = item->children;
            key = NULL;
            value = NULL;
            type = NULL;
            memset(&iptv_ad, 0, sizeof(IptvAdClass));
            while(item1)
            {
                if((!key)&&(0 == xmlStrcmp(item1->name, (unsigned  char*)XML_STR_NAME)))
                {
                    key = (char *)xmlNodeGetContent(item1);
                }
                else if((!value)&&(0 == xmlStrcmp(item1->name, (unsigned  char*)XML_STR_URL)))
                {
                    value = (char *)xmlNodeGetContent(item1);
                }
                else if((!type)&&(0 == xmlStrcmp(item1->name, (unsigned  char*)XML_STR_TYPE)))
                {
                    type = (char *)xmlNodeGetContent(item1);
                }
                else if((!ad) && (0 == xmlStrcmp(item1->name, (unsigned  char*)XML_STR_AD)))
                {
                    _iptv_parse_ad(item1, &iptv_ad);
                }
                item1 = item1->next;
            }
            if(key && value)
            {
                IPTV_PRINTF("key=%s\n", key);
                IPTV_PRINTF("value=%s\n", value);
                list->iptv_item_list[list->iptv_item_num].iptv_key = key;
                list->iptv_item_list[list->iptv_item_num].iptv_value = value;
                list->iptv_item_list[list->iptv_item_num].iptv_type = type;
                list->iptv_item_list[list->iptv_item_num].iptv_ad = iptv_ad;
                if(type)
                {
                    IPTV_PRINTF("type=%s\n", type);
                }
                list->iptv_item_num++;
            }
            else
            {
                if(key)
                {
                    free(key);
                }
                if(value)
                {
                    free(value);
                }
                if(type)
                {
                    free(type);
                }
            }
        }
        item = item->next;
    }

    xmlFreeDoc(doc);
    return list;
}

static void _iptv_list_save_to_file(IptvListClass *list, char *path)
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root = NULL;
    xmlNodePtr item = NULL;
    xmlNodePtr key = NULL;
    xmlNodePtr value = NULL;
    xmlNodePtr type = NULL;
    xmlNodePtr content = NULL;
    int i = 0;

    IPTV_PRINTF("gx_list_save_to_file\n");
    if((!path)||(strlen(path)==0))
    {
        IPTV_PRINTF("input error\n");
        return;
    }

    doc = xmlNewDoc(BAD_CAST "1.0");
    if(!doc)
    {
        return;
    }
    root = xmlNewDocNode(doc, NULL, BAD_CAST XML_STR_LIST, NULL);
    if(!root)
    {
        xmlFreeDoc(doc);
        return;
    }

    xmlDocSetRootElement(doc, root);

    if(list && list->iptv_item_list && list->iptv_item_num > 0)
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)&&(strlen(list->iptv_item_list[i].iptv_key)>0)
                    &&(list->iptv_item_list[i].iptv_value)&&(strlen(list->iptv_item_list[i].iptv_value)>0))
            {
                item = xmlNewNode(NULL, (unsigned  char*)XML_STR_ITEM);
                xmlAddChild(root, item);

                //add key
                key = xmlNewNode(NULL, (unsigned  char*)XML_STR_NAME);
                xmlAddChild(item, key);

                content = xmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_key);
                xmlAddChild(key,content);

                //add value
                value = xmlNewNode(NULL, (unsigned  char*)XML_STR_URL);
                xmlAddChild(item, value);

                content = xmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_value);
                xmlAddChild(value,content);

                //add type if exist
                if((list->iptv_item_list[i].iptv_type)&&(strlen(list->iptv_item_list[i].iptv_type)>0))
                {
                    type = xmlNewNode(NULL, (unsigned  char*)XML_STR_TYPE);
                    xmlAddChild(item, type);

                    content = xmlNewText((unsigned  char*)list->iptv_item_list[i].iptv_type);
                    xmlAddChild(type,content);
                }
            }
        }
    }

#ifdef ECOS_OS
    remove(path);
#endif
    xmlSaveFile(path, doc);
    xmlFreeDoc(doc);
}
#endif

static IptvListClass *_iptv_get_list_from_file(const char *path)
{
    IptvListClass *list = NULL;
    char *ext = NULL;

    IPTV_PRINTF("gx_get_list_from_file\n");
    if((!path)||(strlen(path)==0))
    {
        IPTV_PRINTF("input error\n");
        return NULL;
    }

    ext = strrchr(path, '.');
    if(ext)
    {
        if(0 == strcasecmp(ext, ".txt"))
        {
            list = _get_list_from_txt_file(path);
        }
        else if(0 == strcasecmp(ext, ".xml"))
        {
            list = _get_list_from_xml_file(path);
        }
        else
        {
            IPTV_PRINTF("unsupport file format\n");
        }
    }
    else
    {
        IPTV_PRINTF("invalid file path\n");
    }
    return list;
}

IptvListClass *iptv_get_list_by_type(IptvListClass *list, char *type)
{
    IptvListClass *dstlist = NULL;
    char *temp = NULL;
    int i=0;
    if(list && (list->iptv_item_list) && (list->iptv_item_num>0) && (list->iptv_item_num<=LIST_MAX_ITEM_NUM)
            && (type) && (strlen(type) > 0))
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)
                    && (list->iptv_item_list[i].iptv_value)
                    && (list->iptv_item_list[i].iptv_type))
            {
                temp = strstr(list->iptv_item_list[i].iptv_type, type);
                if(temp
                        &&((temp == list->iptv_item_list[i].iptv_type) || (*(temp-1) == '/'))
                        &&((*(temp+strlen(type)) == '\0') || (*(temp+strlen(type)) == '/')))
                {
                    if(!dstlist)
                    {
                        dstlist = _iptv_list_new();
                    }
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_key = GxCore_Strdup(list->iptv_item_list[i].iptv_key);
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_value = GxCore_Strdup(list->iptv_item_list[i].iptv_value);
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_type = GxCore_Strdup(list->iptv_item_list[i].iptv_type);
                    if(list->iptv_item_list[i].iptv_ad.ad_name != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_name = GxCore_Strdup(list->iptv_item_list[i].iptv_ad.ad_name);
                    if(list->iptv_item_list[i].iptv_ad.ad_type != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_type = GxCore_Strdup(list->iptv_item_list[i].iptv_ad.ad_type);
                    if(list->iptv_item_list[i].iptv_ad.ad_value != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_value = GxCore_Strdup(list->iptv_item_list[i].iptv_ad.ad_value);
                    dstlist->iptv_item_num++;
                }
            }
        }
    }

    return dstlist;
}

IptvListClass *iptv_get_group_list(IptvListClass *list)
{
    IptvListClass *dstlist = NULL;
    char *temp = NULL;
    char *separator = NULL;
    char *type_str = NULL;
    char *pos = NULL;;
    int temp_flag = 1;
    int exits_flag = 0;
    int i=0,j=0;
    IPTV_PRINTF("gx_list_get_mode_method\n");
    if(list&&(list->iptv_item_list)&&(list->iptv_item_num>0)&&(list->iptv_item_num<=LIST_MAX_ITEM_NUM))
    {
        for(i=0; i<list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)&&(list->iptv_item_list[i].iptv_value)&&(list->iptv_item_list[i].iptv_type)&&(strlen((list->iptv_item_list[i].iptv_type))>0))
            {
                pos = list->iptv_item_list[i].iptv_type;
                temp_flag = 1;
                while(temp_flag)
                {
                    type_str = NULL;
                    temp = strchr(pos, '/');
                    if(temp)
                    {
                        if(temp>pos)
                        {
                            type_str = GxCore_Strndup(pos, temp-pos);
                        }
                        pos = temp+1;
                    }
                    else
                    {
                        if(strlen(pos)>0)
                        {
                            type_str = GxCore_Strdup(pos);
                        }
                        temp_flag = 0;
                    }
                    if(type_str)
                    {
                        separator = strchr(type_str, ':');
                        if(separator)
                        {
                            separator += 1;
                        }
                        else
                        {
                            separator = type_str;
                        }
                        if(strlen(separator)>0)
                        {
                            if(!dstlist)
                            {
                                dstlist = _iptv_list_new();
                            }
                            exits_flag = 0;
                            if(dstlist->iptv_item_num>0)
                            {
                                for(j=0; j<dstlist->iptv_item_num;j++)
                                {
                                    if(0 == strcmp(dstlist->iptv_item_list[j].iptv_key, separator))
                                    {
                                        exits_flag = 1;
                                        break;
                                    }
                                }
                            }
                            if(exits_flag == 0)
                            {
                                IPTV_PRINTF("method[%d]:%s\n", dstlist->iptv_item_num, separator);
                                dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_key = GxCore_Strdup(separator);
                                dstlist->iptv_item_num++;
                            }
                        }
                        APP_FREE(type_str);
                    }
                }
            }
        }
    }

    return dstlist;
}

int iptv_key_check(char *key_str, char *strFind)
{
	if(key_str&&strFind&&(strcasestr(key_str, strFind)))
	{
		return 1;
	}
	return 0;
}

IptvListClass *iptv_get_list_by_key(IptvListClass *list, char *key)
{
    IptvListClass *dstlist = NULL;
    int  temp = 0;
    int i = 0;
    if (list && (list->iptv_item_list) && (list->iptv_item_num > 0) && (list->iptv_item_num < LIST_MAX_ITEM_NUM)
            && (key) && (strlen(key) > 0))
    {
        for (i = 0; i < list->iptv_item_num; i++)
        {
            if ((list->iptv_item_list[i].iptv_key)
                    && (list->iptv_item_list[i].iptv_value)
                    && (list->iptv_item_list[i].iptv_type))
            {
                temp = iptv_key_check(list->iptv_item_list[i].iptv_key, key);
                if (temp > 0)
                {
                    if (!dstlist)
                    {
                        dstlist = _iptv_list_new();
                    }
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_key = strdup(list->iptv_item_list[i].iptv_key);
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_value = strdup(list->iptv_item_list[i].iptv_value);
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_type = strdup(list->iptv_item_list[i].iptv_type);
                    if (list->iptv_item_list[i].iptv_ad.ad_name != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_name = strdup(list->iptv_item_list[i].iptv_ad.ad_name);
                    if (list->iptv_item_list[i].iptv_ad.ad_type != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_type = strdup(list->iptv_item_list[i].iptv_ad.ad_type);
                    if (list->iptv_item_list[i].iptv_ad.ad_value != NULL)
                        dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_value = strdup(list->iptv_item_list[i].iptv_ad.ad_value);
                    dstlist->iptv_item_num++;
                }
            }
        }
    }

    return dstlist;
}
int iptv_get_list_play_no(IptvListClass *list, IptvItemClass *item)
{

    int i = 0;
    if (NULL == item || NULL == list)
        return -1;
    if (NULL == list->iptv_item_list || list->iptv_item_num <= 0 || list->iptv_item_num >= LIST_MAX_ITEM_NUM || \
            NULL == item->iptv_key || NULL == item->iptv_type || NULL == item->iptv_value)
        return -1;

    for (i = 0; i < list->iptv_item_num; i++)
    {
        if ((list->iptv_item_list[i].iptv_key)
                && (list->iptv_item_list[i].iptv_value)
                && (list->iptv_item_list[i].iptv_type))
        {
            if ((0 == strcmp(list->iptv_item_list[i].iptv_key, item->iptv_key)) && \
                    (0 == strcmp(list->iptv_item_list[i].iptv_type, item->iptv_type)) && \
                    (0 == strcmp(list->iptv_item_list[i].iptv_value, item->iptv_value)))
                return i;
        }
    }

    return -1;
}

void iptv_list_free(IptvListClass *list)
{
    int i=0;
    if(list)
    {
        if((list->iptv_item_list))
        {
            if(list->iptv_item_num>0)
            {
                for(i=0; i<list->iptv_item_num; i++)
                {
                    APP_FREE(list->iptv_item_list[i].iptv_key);
                    APP_FREE(list->iptv_item_list[i].iptv_value);
                    APP_FREE(list->iptv_item_list[i].iptv_type);
                    APP_FREE(list->iptv_item_list[i].iptv_ad.ad_name);
                    APP_FREE(list->iptv_item_list[i].iptv_ad.ad_type);
                    APP_FREE(list->iptv_item_list[i].iptv_ad.ad_value);
                }
            }
            APP_FREE(list->iptv_item_list);
        }
        APP_FREE(list);
    }
}

static IptvListClass* _iptv_api_get_list(const char *src_file)
{
    IptvListClass *ret_list = NULL;
    IPTV_PRINTF("iptv db path: %s\n", src_file);
    ret_list = _iptv_get_list_from_file(src_file);

    return ret_list;
}

IptvListClass* app_iptv_api_get_remote_bak_list(void)
{
    return _iptv_api_get_list(IPTV_REMOTE_DATABASE);
}

IptvListClass* app_iptv_api_get_local_list(void)
{
    return _iptv_api_get_list(IPTV_LOCAL_DATABASE);
}

IptvListClass* app_iptv_api_get_fav_list(void)
{
    return _iptv_api_get_list(IPTV_FAV_DATABASE);
}

IptvListClass* app_iptv_api_get_list_from_file(const char *src_file)
{
    return _iptv_api_get_list(src_file);
}

static int _iptv_service_update_list(char *file_path, const char *dst_path)
{
    int ret = -1;
    IptvListClass *iptv_list = NULL;

    if(file_path == NULL)
        return -1;

    iptv_list = _iptv_get_list_from_file(file_path);
    if(iptv_list != NULL)
    {
        _iptv_list_save_to_file(iptv_list, dst_path);
        iptv_list_free(iptv_list);
        ret = 0;
    }

    return ret;
}

int app_iptv_service_update_remote_bak_list(char *file_path)
{
    return _iptv_service_update_list(file_path, IPTV_REMOTE_DATABASE);
}

int app_iptv_service_update_local_list(char *file_path)
{
    return _iptv_service_update_list(file_path, IPTV_LOCAL_DATABASE);
}

int app_iptv_service_save_remote_bak_list(IptvListClass *iptv_list)
{
    if(iptv_list != NULL)
    {
        _iptv_list_save_to_file(iptv_list, IPTV_REMOTE_DATABASE);
        return 0;
    }
    return -1;
}

int app_iptv_service_save_local_list(IptvListClass *iptv_list)
{
    if(iptv_list != NULL)
    {
        _iptv_list_save_to_file(iptv_list, IPTV_LOCAL_DATABASE);
        return 0;
    }
    return -1;
}

void app_iptv_api_rm_remote_bak_file(void)
{
    if(GXCORE_FILE_EXIST == GxCore_FileExists(IPTV_REMOTE_DATABASE))
    {
        GxCore_FileDelete(IPTV_REMOTE_DATABASE);
    }
}

IptvListClass *iptv_get_list_by_type_simple(IptvListClass *list, char *type)
{
    IptvListClass *dstlist = NULL;
    int i=0;
    if(list && (list->iptv_item_list) && (list->iptv_item_num>0) && (list->iptv_item_num<LIST_MAX_ITEM_NUM)
            && (type) && (strlen(type) > 0))
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)
                    &&(list->iptv_item_list[i].iptv_value)
                    &&(list->iptv_item_list[i].iptv_type)
                    &&(strcmp(list->iptv_item_list[i].iptv_type, type) == 0))
            {
                if(!dstlist)
                {
                    dstlist = _iptv_list_new();
                }
                dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_key = strdup(list->iptv_item_list[i].iptv_key);
                dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_value = strdup(list->iptv_item_list[i].iptv_value);
                dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_type = strdup(list->iptv_item_list[i].iptv_type);
                if(list->iptv_item_list[i].iptv_ad.ad_name != NULL)
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_name = strdup(list->iptv_item_list[i].iptv_ad.ad_name);
                if(list->iptv_item_list[i].iptv_ad.ad_type != NULL)
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_type = strdup(list->iptv_item_list[i].iptv_ad.ad_type);
                if(list->iptv_item_list[i].iptv_ad.ad_value != NULL)
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_ad.ad_value = strdup(list->iptv_item_list[i].iptv_ad.ad_value);
                dstlist->iptv_item_num++;
            }
        }
    }

    return dstlist;
}

IptvListClass *iptv_get_group_list_simple(IptvListClass *list)
{
    IptvListClass *dstlist = NULL;
    int exits_flag = 0;
    int i=0,j=0;

    if(list&&(list->iptv_item_list)&&(list->iptv_item_num>0)&&(list->iptv_item_num<LIST_MAX_ITEM_NUM))
    {
        for(i=0; i<list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)&&(list->iptv_item_list[i].iptv_value)&&(list->iptv_item_list[i].iptv_type)&&(strlen((list->iptv_item_list[i].iptv_type))>0))
            {
                if(!dstlist)
                {
                    dstlist = _iptv_list_new();
                }
                exits_flag = 0;
                if(dstlist->iptv_item_num>0)
                {
                    for(j=0; j<dstlist->iptv_item_num;j++)
                    {
                        if(0 == strcmp(dstlist->iptv_item_list[j].iptv_key, list->iptv_item_list[i].iptv_type))
                        {
                            exits_flag = 1;
                            break;
                        }
                    }
                }
                if(exits_flag == 0)
                {
                    dstlist->iptv_item_list[dstlist->iptv_item_num].iptv_key = strdup(list->iptv_item_list[i].iptv_type);
                    dstlist->iptv_item_num++;
                }
            }
        }
    }

    return dstlist;
}

void iptv_indexlist_free(IptvIndexClass *list)
{
	if(list)
	{
		if((list->iptv_index))
		{
			free(list->iptv_index);
			list->iptv_index = NULL;
		}
		free(list);
	}
}

IptvIndexClass *iptv_indexlist_new(void)
{
    IptvIndexClass *list = NULL;

    list = (IptvIndexClass *)malloc(sizeof(IptvIndexClass));
    memset(list, 0, sizeof(IptvIndexClass));
    list->iptv_index = (int *)malloc(LIST_MAX_ITEM_NUM*sizeof(int *));
    memset(list->iptv_index, 0, LIST_MAX_ITEM_NUM*sizeof(int *));

    return list;
}

void iptv_indexlist_add_item(IptvIndexClass *list, int index)
{
	if(list&&(list->iptv_index_num<LIST_MAX_ITEM_NUM))
	{
		list->iptv_index[list->iptv_index_num] = index;
		list->iptv_index_num++;
	}
}

IptvIndexClass *iptv_get_indexlist_by_type(IptvListClass *list, char *type)
{
    IptvIndexClass *dstlist = NULL;
    char *temp = NULL;
    int i=0;
    if(list && (list->iptv_item_list) && (list->iptv_item_num>0) && (list->iptv_item_num<LIST_MAX_ITEM_NUM)
            && (type) && (strlen(type) > 0))
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)
                    && (list->iptv_item_list[i].iptv_value)
                    && (list->iptv_item_list[i].iptv_type))
            {
                temp = strstr(list->iptv_item_list[i].iptv_type, type);
                if(strncmp(type, "ALL", 3) == 0 ||
                        (temp &&((temp == list->iptv_item_list[i].iptv_type) || (*(temp-1) == '/'))
                              &&((*(temp+strlen(type)) == '\0') || (*(temp+strlen(type)) == '/'))))
                {
                    if(!dstlist)
                    {
                        dstlist = iptv_indexlist_new();
                    }
                    dstlist->iptv_index[dstlist->iptv_index_num] = i;
                    dstlist->iptv_index_num++;
                }
            }
        }
    }

    return dstlist;
}

IptvIndexClass *iptv_get_indexlist_by_type_simple(IptvListClass *list, char *type)
{
    IptvIndexClass *dstlist = NULL;
    int i=0;
    if(list && (list->iptv_item_list) && (list->iptv_item_num>0) && (list->iptv_item_num<LIST_MAX_ITEM_NUM)
            && (type) && (strlen(type) > 0))
    {
        for(i = 0; i < list->iptv_item_num; i++)
        {
            if((list->iptv_item_list[i].iptv_key)
                    &&(list->iptv_item_list[i].iptv_value)
                    &&(list->iptv_item_list[i].iptv_type)
                    &&(strncmp(type, "ALL", 3) == 0 || strcmp(list->iptv_item_list[i].iptv_type, type) == 0))
            {
                if(!dstlist)
                {
                    dstlist = iptv_indexlist_new();
                }
				dstlist->iptv_index[dstlist->iptv_index_num] = i;
				dstlist->iptv_index_num++;
            }
        }
    }

    return dstlist;
}

void iptv_indexlist_del_item(IptvIndexClass *list, int index)
{
	int i = 0;
	if(list&&(list->iptv_index_num>0))
	{
		for(i=0; i<list->iptv_index_num; i++)
		{
			if(list->iptv_index[i] == index)
			{
				if(i<(list->iptv_index_num-1))
				{
					memcpy(&(list->iptv_index[i]), &(list->iptv_index[i+1]), (list->iptv_index_num-1-i)*sizeof(int));
				}
				list->iptv_index_num--;
			}
		}
	}
}

