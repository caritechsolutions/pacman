#ifndef _GX_UPGRADE_H
#define _GX_UPGRADE_H
#include <gx_apps.h>

#define UPGRADE_DEBUG //allen
#ifdef UPGRADE_DEBUG
#define UPGRADE_PRINTF  printf
#else
#define UPGRADE_PRINTF(...)	do{}while(0)
#endif

typedef struct
{
    char *sw_ver;
    char *sw_ver_match;
    char *hw_ver;
    char *manufacture_id;
    char *model_sn;
    char *url;
    char *file_size;
    char *log;
    char *tip_timeout;
    char *rec_url;
    char *rec_par;
    char *sw_ver_par;
}NetUpgSettingInfo;
gxapps_ret_t upgrade_parse_data_xml(char *buffer, int size, NetUpgSettingInfo **upgrade_info);
void gx_free_upgrade_info(NetUpgSettingInfo *upgrade_info);

typedef enum
{
    UPGRADE_SW_VERSION=0,
    UPGRADE_SW_VERSION_MATCH,
    UPGRADE_HW_VERSION,
    UPGRADE_MANUFACTURE_ID,
    UPGRADE_MODEL_SN,
}app_upgrade_info_type;
#endif
