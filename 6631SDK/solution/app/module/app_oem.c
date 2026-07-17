/*
 * =====================================================================================
 *
 *       Filename:  app_oem.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2012年05月10日 14时20分15秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include <memmgr.h>
#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_module.h"


static const struct
{
    app_oem_type type;
    char * gname;
    char * iname;
}oem_item[] = {
    { OEM_LANG_OSD,         "language",         "osd language"      },
    { OEM_LANG_AUD1,         "language",         "audio1 language"      },
    { OEM_LANG_AUD2,         "language",         "audio2 language"      },
    { OEM_LANG_SUBT,        "language",         "sub language" },
    { OEM_LANG_TTX,         "language",         "ttx language" },
    { OEM_LANG_EPG,         "language",         "epg language" },
    { OEM_HW_VERSION,          "common",           "hardware version"},
    { OEM_SW_VERSION,          "common",           "software version"},
    { OEM_TRANS,            "common",           "transparence"      },
    { OEM_VOLUME,           "common",           "volume"            },
    { OEM_TV_STANDARD,      "common",           "tv standard"       },
    { OEM_TIME_ZONE,        "common",           "time zone"         },
    { OEM_SUMMER_TIME,      "common",           "summer time"       },
    { OEM_SEARCH_MODE,      "common",           "search mode"       },
    { OEM_SCART_CONF,      "scart config",           "SCART"       },
    { OEM_SPDIF_CONF,      "spdif config",           "SPDIF"       },
    { OEM_LCN_CONFIG,        "LCN config",         "LCN"                },
    { OEM_ANTENNA_P0WER,     "Antenna Power",       "Antenna_Power"     },
    { OEM_TVSTANARD_CONF,    "TV Standard",         "Standard"          },
    { OEM_COUNTRY_CONFIG,      "Country config",       "country select"   },
    { OEM_STANDBY_SHOWTIME,  "Standby showtime",       "showtime"   },
    { OEM_PASSWORD,            "PassWord config",       "PassWord"   },
    { OEM_DLNA_DMP,            "netapp",                "DMP"   },
    { OEM_DLNA_DMR,            "netapp",                "DMR"   },
    { OEM_DLNA_SAT2IP,         "netapp",                "Sat2IP"   },
    { OEM_TYPE_MAX,         NULL,               NULL                }
};

#define APP_OEM_PATH    WORK_PATH"theme/default.xml"
#define OEM_STR         (xmlChar*)"default"
#define ITEM_STR        (xmlChar*)"item"
#define GROUP_NAME      (xmlChar*)"name"
#define ITEM_DISP       (xmlChar*)"display"
#define ITEM_VAL        (xmlChar*)"value"
#define LIST_VAL        (xmlChar*)"value"


/*************** xml parser start ****************/
static status_t info_get_list_item(GXxmlNodePtr xml_item, xmlChar *name, char * buf, size_t size)
{
	GXxmlNodePtr item = NULL;
	xmlChar *value = NULL;
	xmlChar *iname = NULL;

	item =  xml_item->childs;
	while (item != NULL)
	{
        	iname = GXxmlGetProp(item, LIST_VAL);

		if (GXxmlStrcmp(iname, name) == 0)
		{
		    value = GXxmlGetProp(item, ITEM_DISP);
		    strncpy(buf, (char*)value, size);

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
static status_t info_get(GXxmlNodePtr GXxml_item, xmlChar *name, char * buf, size_t size,int mode)
{
	GXxmlNodePtr item = NULL;
	xmlChar *value = NULL;
	xmlChar *iname = NULL;

	item =  GXxml_item->childs;
	while (item != NULL)
	{
        iname = GXxmlGetProp(item, ITEM_DISP);

		if (GXxmlStrcmp(iname, name) == 0)
        {
            value = GXxmlGetProp(item, ITEM_VAL);
            strncpy(buf, (char*)value, size);
            if(mode)
            {
                info_get_list_item(item,value,buf, size);
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

status_t oem_parser(const char *filename, app_oem_type type, char * buf, size_t size,int mode)
{
	GXxmlDocPtr doc;
	xmlChar *gname = NULL;
	status_t ret = GXCORE_SUCCESS;

	if(NULL == filename)
	{
		return GXCORE_ERROR;
	}

	doc = GXxmlParseFile(filename);
	if (NULL == doc)
	{
		app_log_error( "[app oem]GXxml parse error.\n");
		return GXCORE_ERROR;
	}

    // root
	GXxmlNodePtr root = doc->root;
	if (root == NULL)
	{
		app_log_error( "[app oem]GXxml empty\n");
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

	if (0 !=  GXxmlStrcmp(root->name, OEM_STR))
	{
		GXxmlFreeDoc(doc);
		return GXCORE_ERROR;
	}

    // group
	GXxmlNodePtr group = root->childs;
	while (NULL != group)
	{
		gname = GXxmlGetProp(group, GROUP_NAME);

		if (gname != NULL &&
			0 ==  GXxmlStrcmp(gname, (xmlChar*)(oem_item[type].gname)))
		{
			ret = info_get(group, (xmlChar*)(oem_item[type].iname), buf, size,mode);
			GXxmlFree(gname);
			GXxmlFreeDoc(doc);
			return ret;
 		}

 		GXxmlFree(gname);
 		gname = NULL;
		group = group->next;
	}

	GXxmlFreeDoc(doc);
	return GXCORE_ERROR;
}

/*************** GXxml parser end ****************/

status_t app_oem_get(app_oem_type type, char * buf, size_t size)
{
    return oem_parser(APP_OEM_PATH, type, buf, size,0);
}

status_t app_oem_get_int(app_oem_type type, int32_t *result)
{
#define BUF_LEN 256
    char buf[BUF_LEN] = {0};

    if (app_oem_get(type, buf, BUF_LEN) != GXCORE_SUCCESS)
        return GXCORE_ERROR;

    *result = atoi(buf);
    return GXCORE_SUCCESS;
}

status_t app_oem_get_list_value(app_oem_type type, char * buf, size_t size)
{
	#ifdef LINUX_OS
	return oem_parser(WORK_PATH"theme/default.xml", type, buf, size,0);
	#else
    	return oem_parser(APP_OEM_PATH, type, buf, size,1);
	#endif
}

status_t app_oem_get_theme_ver(const char *path,app_oem_type type, char * buf, size_t size)
{
    return oem_parser(path, type, buf, size,0);
}

status_t app_oem_info_get(GXxmlNodePtr GXxml_item, xmlChar *name, char * buf, size_t size,int mode)
{
	return info_get(GXxml_item, name, buf, size,mode);
}
