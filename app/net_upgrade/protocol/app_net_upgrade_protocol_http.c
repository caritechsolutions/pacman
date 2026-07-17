/*************************************************************************
	> File Name: net_upgrade.c
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 15时45分01秒
 ************************************************************************/

#include "app.h"
#include "app_module.h"
#include <gx_apps.h>
#include "../app_net_upgrade.h"
#include "../app_net_upgrade_common.h"
#if NET_UPGRADE_SUPPORT
extern UpgradeProtocol http_ops;

int http_get_upgrade_info(char *url)
{
    s_upg_process.handle = gxapps_curl_async_download(url,UPG_SERVER_CONFIG_FILE,netupg_download_callback);
    if(!s_upg_process.handle)
        s_upg_process.finish = 2;
    return 0;
}


static int http_upgrade_file_get(char *url)
{
    if(s_upg_process.mode == NET_UPG_DOWNLOAD_BIN)
    {
        s_upg_process.handle = gxapps_curl_async_download_ext(url, NULL, NULL, s_upgrade_bin_data_buf.usr_p, s_upgrade_bin_data_buf.size, netupg_download_callback);
    }
    else if(s_upg_process.mode == NET_UPG_DOWNLOAD_RECOVERY_OS)
    {
        s_upg_process.handle = gxapps_curl_async_download_ext(url, NULL, NULL, s_upgrade_recv_os_data_buf.usr_p, s_upgrade_recv_os_data_buf.size, netupg_download_callback);
    }
    if(!s_upg_process.handle)
        s_upg_process.finish = 2;
    return 0;
}

UpgradeProtocol http_ops =
{
    .protocol = "http",
    .cfg_get = http_get_upgrade_info,
    .cfg_parse = config_file_parse,
    .upg_file_get = http_upgrade_file_get,
    .upg_file_verify= upgrade_file_verify,
    .upg_exec = upgrade_exec,
    .process_get = upgrade_process_get,
};

#endif
