#include "app.h"
#ifdef IPTV_SUPPORT
#include "app_module.h"
#include "iptv/app_iptv.h"
#include "app_wnd_file_list.h"
#include "iptv/app_iptv_list_service.h"
#include "app_windows.h"
#include "full_screen.h"
#include <gx_apps.h>
#include "app_common_view.h"
#include "netapps/app_netvideo_play.h"
#include "app_iptv_list_api.h"

extern void set_iptv_save_channel(IPTVChannel *channel);
extern IPTVChannel* get_iptv_save_channel(void);
extern uint64_t app_get_iptvtime(void);
extern int app_net_mktime_sync_message(void);
static int player_time_regist_start = 0;
#ifdef IPTV_FAV_SUPPORT
extern gx_list_t* get_local_iptv_fav_list(void);
extern void save_local_iptv_fav_list(gx_list_t* list);
#endif

void app_create_gxiptv_menu(void)
{
    IPTVOps ops;

    memset(&ops, 0, sizeof(IPTVOps));
#ifdef IPTV_CLOUD_SUPPORT
    ops.server_type = IPTV_SERVER_GX;
    ops.server_title = STR_ID_CLOUD;
    ops.source_num = 2;
#else
    ops.server_type = IPTV_SERVER_UNKOWN;
    ops.source_num = 1;
#endif
    ops.auto_play = 0;
    ops.set_save_channel = set_iptv_save_channel;
    ops.get_save_channel = get_iptv_save_channel;
#ifdef IPTV_FAV_SUPPORT
    ops.get_fav_list = get_local_iptv_fav_list;
    ops.save_fav_list = save_local_iptv_fav_list;
#endif
    app_net_mktime_sync_message();
    if(player_time_regist_start == 0)
    {
        GxExternalNetTime_RegisterCallback(app_get_iptvtime);
        player_time_regist_start = 1;
    }
    app_create_iptv_menu(&ops);
}

#endif
