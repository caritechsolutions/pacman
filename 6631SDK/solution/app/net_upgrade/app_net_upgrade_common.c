/*************************************************************************
	> File Name: net_upgrade.c
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 15时45分01秒
 ************************************************************************/

#include "app.h"
#include "app_config.h"
#include "app_net_upgrade.h"
#include "app_net_upgrade_common.h"
#if NET_UPGRADE_SUPPORT
typedef enum
{
    NET_UPG_PROTOCOL_HTTP = 0,
    NET_UPG_PROTOCOL_HTTPS,
    NET_UPG_PROTOCOL_FTP,
    NET_UPG_PROTOCOL_FILE,
    NET_UPG_PROTOCOL_TOTAL,
}Protocol;

static UpgradeProtocol *thiz_protocol[NET_UPG_PROTOCOL_TOTAL];
static UpgradeProtocol *thiz_style_ops = NULL;

static void _net_upg_ops_auto_adapt(char* url)
{
    if(0 == strncmp(url, NETUPG_HTTP_PREFIX, strlen(NETUPG_HTTP_PREFIX)))
    {
        thiz_style_ops = thiz_protocol[NET_UPG_PROTOCOL_HTTP];
    }
    else if(0 == strncmp(url, NETUPG_HTTPS_PREFIX, strlen(NETUPG_HTTPS_PREFIX)))
    {
        thiz_style_ops = thiz_protocol[NET_UPG_PROTOCOL_HTTPS];
    }
    else if(0 == strncmp(url, NETUPG_FTP_PREFIX, strlen(NETUPG_FTP_PREFIX)))
    {
        thiz_style_ops = thiz_protocol[NET_UPG_PROTOCOL_FTP];
    }
    else if(0 == strncmp(url, NETUPG_FILE_PREFIX, strlen(NETUPG_FILE_PREFIX)))
    {
        thiz_style_ops = thiz_protocol[NET_UPG_PROTOCOL_FILE];
    }
}

inline static void _add_protocol(Protocol index, UpgradeProtocol *protocol)
{
    if(NULL == thiz_protocol[index])
    {
        thiz_protocol[index] = protocol;
    }
}

#define NET_UPG_PROTOCOL_REGISTER(name,index, p) \
  void NET_UPG_ProtocolRegister##name(void) \
{\
    extern Protocol p;\
    _add_protocol(index, (UpgradeProtocol*)&p);\
}
NET_UPG_PROTOCOL_REGISTER(HTTP,NET_UPG_PROTOCOL_HTTP ,http_ops);
NET_UPG_PROTOCOL_REGISTER(HTTPS,NET_UPG_PROTOCOL_HTTPS ,https_ops);
NET_UPG_PROTOCOL_REGISTER(FTP,NET_UPG_PROTOCOL_FTP ,ftp_ops);
NET_UPG_PROTOCOL_REGISTER(FILE,NET_UPG_PROTOCOL_FILE ,file_ops);

void app_upgrade_protocol_register(void)
{
    NET_UPG_ProtocolRegisterHTTP();
    NET_UPG_ProtocolRegisterHTTPS();
    NET_UPG_ProtocolRegisterFTP();
    NET_UPG_ProtocolRegisterFILE();
}


int app_cfg_file_get(char* url)
{
    int ret = -1;
    _net_upg_ops_auto_adapt(url);
    if(thiz_style_ops && thiz_style_ops->cfg_get)
        ret = thiz_style_ops->cfg_get(url);
    return ret;
}

int app_cfg_file_parse(char **info)
{
    char *info1 = NULL;
    int ret = -1;

    if(thiz_style_ops && thiz_style_ops->cfg_parse)
    {
        ret = thiz_style_ops->cfg_parse(&info1);
        *info = info1;
    }
    return ret;
}

int app_upgrade_file_get(char* url)
{
    int ret = -1;

    _net_upg_ops_auto_adapt(url);
    if(thiz_style_ops && thiz_style_ops->upg_file_get)
        ret= thiz_style_ops->upg_file_get(url);

    return ret;
}

int app_upgrade_file_verify(char **tip)
{
    char *tip1 = NULL;
    int ret = -1;

    if(thiz_style_ops && thiz_style_ops->upg_file_verify)
    {
        ret = thiz_style_ops->upg_file_verify(&tip1);
        *tip = tip1;
    }
    return ret;
}

int app_upgrade_exec(void)
{
    int ret = -1;
    if(thiz_style_ops && thiz_style_ops->upg_exec)
        ret = thiz_style_ops->upg_exec();

    return ret;
}


NetUpgProcess app_upgrade_process_get(void)
{
    NetUpgProcess ret ={0};
    if(thiz_style_ops && thiz_style_ops->process_get)
        ret = thiz_style_ops->process_get();

    return ret;
}
#endif
