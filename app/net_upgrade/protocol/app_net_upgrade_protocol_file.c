/*************************************************************************
	> File Name: net_upgrade.c
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 15时45分01秒
 ************************************************************************/

#include "app.h"
#include "app_module.h"
#include "app_default_params.h"
#include "../app_net_upgrade.h"
#include "../app_net_upgrade_common.h"
#include "../app_upgrade_xml_parse.h"

#ifdef ECOS_OS
#include "linux/zlib.h"
#include <cyg/fileio/fileio.h>
#else
#include "zlib.h"
#include "app_tools.h"
#endif
#if NET_UPGRADE_SUPPORT

extern UpgradeProtocol file_ops;

static int _get_usb_bin_file(char* path, GxHwMallocObj *buf)
{
    FILE *file = NULL;
    int ret = 0;
    long len = 0;

    gxcurl_callback_data_t callback_data;
    memset(&callback_data, 0, sizeof(gxcurl_callback_data_t));

    if(GXCORE_FILE_EXIST == GxCore_FileExists(path))
    {
        file = fopen(path, "r");
        if (file == NULL)
            goto err;
        // get size
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        if((0 >= len) || (len > (buf->size - 4)))
        {
            fclose(file);
            goto err;
        }
        s_upg_process.total_size = len;
        fseek(file, 0, SEEK_SET);
        ret = fread((unsigned char *)buf->usr_p, 1, len, file);
        if (ret != len)
        {
            fclose(file);
            goto err;
        }

        fclose(file);
        s_upg_process.cur_size = s_upg_process.total_size;
        s_upg_process.finish = 1;
        callback_data.complete_data.retcode = 0;
        netupg_download_callback(GXCURL_STATE_COMPLETE, (int)(&callback_data));
        return 0;
    }
err:
    s_upg_process.finish = 2;
    callback_data.complete_data.retcode = -1;
    netupg_download_callback(GXCURL_STATE_COMPLETE, (int)(&callback_data));
    return -1;
}

static int _get_usb_recv_os_file(char* path, GxHwMallocObj *buf)
{
    FILE *file = NULL;
    long len = 0;
    int ret = 0;

    gxcurl_callback_data_t callback_data;
    memset(&callback_data, 0, sizeof(gxcurl_callback_data_t));
    if(GXCORE_FILE_EXIST == GxCore_FileExists(path))
    {
        file = fopen(path, "rb");
        if (file == NULL)
            goto err;
        fseek(file, 0, SEEK_END);
        len = ftell(file);
        if((0 >= len) || (len > (buf->size - 4)))
        {
            fclose(file);
            goto err;
        }
        s_upg_process.total_size = len;
        fseek(file, 0, SEEK_SET);
        ret = fread((unsigned char *)buf->usr_p, 1, len, file);
        if (ret != len)
        {
            fclose(file);
            goto err;
        }

        fclose(file);
        s_upg_process.cur_size = s_upg_process.total_size;
        callback_data.complete_data.retcode = 0;
        netupg_download_callback(GXCURL_STATE_COMPLETE, (int)(&callback_data));
        return 0;
    }
err:
    callback_data.complete_data.retcode = -1;
    netupg_download_callback(GXCURL_STATE_COMPLETE, (int)(&callback_data));
    return -1;
}

static void _get_usb_file(void* arg)
{
    char *url = (char*)arg;
    char *path = url + strlen(NETUPG_FILE_PREFIX) - 1;

    GxCore_ThreadDetach();

    if(s_upg_process.mode == NET_UPG_DOWNLOAD_BIN)
    {
        _get_usb_bin_file(path, &s_upgrade_bin_data_buf);
    }
    else if(s_upg_process.mode == NET_UPG_DOWNLOAD_RECOVERY_OS)
    {
        _get_usb_recv_os_file(path, &s_upgrade_recv_os_data_buf);
    }

    return;
}

int file_get_upgrade_info(char *url)
{
    if(GXCORE_FILE_EXIST == GxCore_FileExists(url+strlen(NETUPG_FILE_PREFIX)-1))
    {
        FILE *source = NULL, *dest = NULL;
        char *buffer = NULL;
        size_t bytesRead = 0;
        int len = 0;

        source = fopen(url+strlen(NETUPG_FILE_PREFIX)-1, "rb");
        if (source == NULL)
            goto err;
        // get size
        fseek(source, 0, SEEK_END);
        len = ftell(source);
        if(0 > len)
        {
            fclose(source);
            goto err;
        }
        fseek(source, 0, SEEK_SET);
        dest = fopen(UPG_SERVER_CONFIG_FILE, "wb");
        if (dest == NULL)
        {
            fclose(source);
            goto err;
        }
        buffer = (char *)GxCore_Mallocz(len * sizeof(char));
        if(buffer == NULL)
        {
            fclose(source);
            fclose(dest);
            goto err;
        }
        while ((bytesRead = fread(buffer, 1, len, source)) > 0) {
            fwrite(buffer, 1, bytesRead, dest);
        }
        fclose(source);
        fclose(dest);
        APP_FREE(buffer);
        s_upg_process.finish = 1;
        return 0;
    }
err:
    s_upg_process.finish = 2;
    return -1;
}

static int file_upgrade_file_get(char* url)
{
    int thread_id = 0;
    GxCore_ThreadCreate("get_usb_file", &thread_id, _get_usb_file, (void*)url, 10 * 1024, GXOS_DEFAULT_PRIORITY);

    return 0;
}

UpgradeProtocol file_ops =
{
    .protocol = "file",
    .cfg_get = file_get_upgrade_info,
    .cfg_parse = config_file_parse,
    .upg_file_get = file_upgrade_file_get,
    .upg_file_verify = upgrade_file_verify,
    .upg_exec = upgrade_exec,
    .process_get = upgrade_process_get,
};
#endif
