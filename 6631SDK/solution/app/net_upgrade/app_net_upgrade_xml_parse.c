#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gx_apps.h>
#include "app_upgrade_xml_parse.h"
#include "gxcore.h"
#include "tree.h"
#include "parser.h"
#include "app_module.h"
#include "app.h"
#include "app_bsp.h"
#include "app_config.h"
#if NET_UPGRADE_SUPPORT
#define XML_STR_SW_VER    "sw_version"
#define XML_STR_SW_VER_MATCH    "sw_version_match"
#define XML_STR_HW_VER          "hw_version"
#define XML_STR_MANU_ID         "manufacture_id"
#define XML_STR_MODEL_SN        "model_sn"
#define XML_STR_URL             "url"
#define XML_STR_FILESIZE        "file_size"
#define XML_STR_LOG             "log"
#define XML_STR_TIP_TIMEOUT     "tip_timeout"
#define XML_STR_RECOVER_URL     "recovery_url"
#define XML_STR_RECOVER_PAR     "recovery_partition"
#define XML_STR_SW_VER_PAR      "sw_version_partition"

#include <parser.h>
#include <tree.h>
static gxapps_ret_t upgrade_parse_cfg_node(GXxmlNodePtr node, NetUpgSettingInfo **upgrade_info)
{
    gxapps_ret_t ret = GXAPPS_SERVER_ERROR;
    GXxmlNodePtr node1 = NULL;
    NetUpgSettingInfo *upgrade_data = NULL;
    char *file_size = NULL;
    char *sw_ver = NULL, *sw_ver_match = NULL, *hw_ver = NULL, *manufacture_id = NULL, *model_sn = NULL, *tip_timeout = NULL, *url = NULL, *log = NULL, *rec_url = NULL, *rec_par = NULL, *sw_ver_par = NULL;

    node1 = node->childs;
    while(node1)
    {
        if((!sw_ver) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_SW_VER)))
        {
            sw_ver = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("sw_ver=%s\n", sw_ver);
        }
        else if((!sw_ver_match) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_SW_VER_MATCH)))
        {
            sw_ver_match = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("sw_ver_match=%s\n", sw_ver_match);
        }
        else if((!hw_ver) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_HW_VER)))
        {
            hw_ver = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("hw_ver=%s\n", hw_ver);
        }
        else if((!manufacture_id) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_MANU_ID)))
        {
            manufacture_id = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("manufacture_id=%s\n", manufacture_id);
        }
        else if((!model_sn) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_MODEL_SN)))
        {
            model_sn = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("model_sn=%s\n", model_sn);
        }
        else if((!url) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_URL)))
        {
            url = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("url=%s\n", url);
        }
        else if((!file_size) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_FILESIZE)))
        {
            file_size = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("file_size=%s\n", file_size);
        }
        else if((!log) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_LOG)))
        {
            log = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("log=%s\n", log);
        }
        else if((!tip_timeout) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_TIP_TIMEOUT)))
        {
            tip_timeout = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("tip_timeout=%s\n", tip_timeout);
        }
        else if((!rec_url) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_RECOVER_URL)))
        {
            rec_url = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("rec_url=%s\n", rec_url);
        }
        else if((!rec_par) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_RECOVER_PAR)))
        {
            rec_par = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("rec_par=%s\n", rec_par);
        }
        else if((!sw_ver_par) && (0 == GXxmlStrcmp(node1->name, (const unsigned char *)XML_STR_SW_VER_PAR)))
        {
            sw_ver_par = (char *)GXxmlNodeGetContent(node1);
            UPGRADE_PRINTF("sw_ver_par=%s\n", sw_ver_par);
        }
        node1 = node1->next;
    }
    upgrade_data = (NetUpgSettingInfo *)malloc(sizeof(NetUpgSettingInfo));
    if(upgrade_data != NULL)
    {
        memset(upgrade_data, 0, sizeof(NetUpgSettingInfo));
        upgrade_data->sw_ver = sw_ver;
        upgrade_data->sw_ver_match = sw_ver_match;
        upgrade_data->hw_ver = hw_ver;
        upgrade_data->manufacture_id = manufacture_id;
        upgrade_data->model_sn = model_sn;
        upgrade_data->url = url;
        upgrade_data->file_size = file_size;
        upgrade_data->log = log;
        upgrade_data->tip_timeout = tip_timeout;
        upgrade_data->rec_url = rec_url;
        upgrade_data->rec_par = rec_par;
        upgrade_data->sw_ver_par = sw_ver_par;
        *upgrade_info = upgrade_data;
        ret = GXAPPS_SUCCESS;
    }
    return ret;
}

gxapps_ret_t upgrade_parse_data_xml(char *buffer, int size, NetUpgSettingInfo **upgrade_info)
{
    gxapps_ret_t ret = GXAPPS_SERVER_ERROR;
    GXxmlDocPtr doc = NULL;
    GXxmlNodePtr root = NULL;

    UPGRADE_PRINTF("upgrade_parse_data_xml\n");
    doc = GXxmlParseMemory(buffer, size);
    if(!doc)
    {
        UPGRADE_PRINTF("doc is null\n");
        return GXAPPS_SERVER_ERROR;
    }
    root = doc->root;
    if(root)
    {
        ret = upgrade_parse_cfg_node(root, upgrade_info);
    }

    GXxmlFreeDoc(doc);
    return ret;
}

void gx_free_upgrade_info(NetUpgSettingInfo *upgrade_info)
{
    if(upgrade_info)
    {
        if(upgrade_info->sw_ver)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->sw_ver);
            upgrade_info->sw_ver = NULL;
        }
        if(upgrade_info->sw_ver_match)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->sw_ver_match);
            upgrade_info->sw_ver_match = NULL;
        }
        if(upgrade_info->hw_ver)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->hw_ver);
            upgrade_info->hw_ver = NULL;
        }
        if(upgrade_info->manufacture_id)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->manufacture_id);
            upgrade_info->manufacture_id = NULL;
        }
        if(upgrade_info->model_sn)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->model_sn);
            upgrade_info->model_sn = NULL;
        }
        if(upgrade_info->url)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->url);
            upgrade_info->url = NULL;
        }
        if(upgrade_info->file_size)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->file_size);
            upgrade_info->file_size = NULL;
        }
        if(upgrade_info->log)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->log);
            upgrade_info->log = NULL;
        }
        if(upgrade_info->tip_timeout)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->tip_timeout);
            upgrade_info->tip_timeout = NULL;
        }
        if(upgrade_info->rec_url)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->rec_url);
            upgrade_info->rec_url = NULL;
        }
        if(upgrade_info->rec_par)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->rec_par);
            upgrade_info->rec_par = NULL;
        }
        if(upgrade_info->sw_ver_par)
        {
            GXxmlFreeChar((CHAR *)upgrade_info->sw_ver_par);
            upgrade_info->sw_ver_par = NULL;
        }
        free(upgrade_info);
        upgrade_info = NULL;
    }
}
#endif
