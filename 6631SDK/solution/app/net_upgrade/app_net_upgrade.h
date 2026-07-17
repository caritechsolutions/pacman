/*************************************************************************
	> File Name: net_upgrade.h
	> Author:
	> Mail:
	> Created Time: 2024年07月02日 星期二 16时48分31秒
 ************************************************************************/

#ifndef _NET_UPGRADE_H
#define _NET_UPGRADE_H

#define NETUPG_MAX_URL_LEN (512)
#define NETUPG_MAX_USER_LEN (128)
#define NETUPG_MAX_PWD_LEN (128)
#define NETUPG_HTTP_PREFIX "http://"
#define NETUPG_HTTPS_PREFIX "https://"
#define NETUPG_FTP_PREFIX "ftp://"
#define NETUPG_FILE_PREFIX "file://"
#define NETUPG_CONFIG_INFO_DELEMITER "%"
#define NETUPG_CONFIG_INFO_INVALID_FILL "NONE"

#ifdef ECOS_OS
#define UPG_SERVER_CONFIG_FILE "/mnt/upgrade_setting.xml"
#else
#define UPG_SERVER_CONFIG_FILE "/tmp/upgrade_setting.xml"
#endif

typedef enum
{
    NET_UPG_NONE = 0,
    NET_UPG_DOWNLOAD_CONFIG,
    NET_UPG_DOWNLOAD_BIN,
    NET_UPG_DOWNLOAD_RECOVERY_OS,
    NET_UPG_UPDATE_SKIP_RECOVERY_PART,
    NET_UPG_UPDATE_RECOVERY_PART,
    NET_UPG_UPDATE_NO_SAFE,
}NetUpgProcessMode;

typedef struct
{
	NetUpgProcessMode mode;
	unsigned int handle;
	unsigned int total_size;
	unsigned int cur_size;
	unsigned int finish;
}NetUpgProcess;
NetUpgProcess s_upg_process;


extern int app_upgrade_file_get_init(char* url);
extern void app_upg_main_upgrade_finish_process(void);
extern int app_cfg_file_get(char* url);
extern int app_cfg_file_parse(char **info);
extern int app_upgrade_file_get(char *url);
extern int app_upgrade_file_verify(char **tip);
extern int app_upgrade_exec(void);
extern NetUpgProcess app_upgrade_process_get(void);
extern void app_upgrade_protocol_register(void);
extern int app_upgrade(void);
extern void _upg_update_write_sw_version(void);

#endif
