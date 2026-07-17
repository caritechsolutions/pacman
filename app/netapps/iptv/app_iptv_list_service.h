#ifndef __APP_IPTV_SERVICE_H__
#define __APP_IPTV_SERVICE_H__

#include "app_config.h"
#if NETAPPS_SUPPORT || defined(RSS_SUPPORT)
#include <gx_apps.h>
#include <app_iptv_list_api.h>
#include "gxtype.h"

typedef enum
{
    IPTV_LIST_LOCAL = 0,
    IPTV_LIST_REMOTE,
    IPTV_LIST_FAV,
    IPTV_LIST_UNKOWN,
}IptvListSource;

typedef enum
{
    IPTV_SERVER_GX = 0,
#ifdef STALKER_SUPPORT
    IPTV_SERVER_STALKER,
#endif
#ifdef XTREAM_SUPPORT
    IPTV_SERVER_XTREAM,
#endif
#ifdef FREEIPTV_SUPPORT
    IPTV_SERVER_FREEIPTV,
#endif
#ifdef RSS_SUPPORT
    IPTV_SERVER_RSS,
#endif
    IPTV_SERVER_UNKOWN,
}IptvServerType;

typedef struct
{
    IptvListSource iptv_source;
    IptvServerType iptv_server;
    int remote_fastget;
    IptvListClass *iptv_list;
    gxapps_ret_t retcode;
}AppMsg_IptvList;


status_t app_iptv_service_start(void);
#ifdef IPTV_CLOUD_SUPPORT
extern int  _get_remote_channels_total(void);
#endif
#endif
#endif
