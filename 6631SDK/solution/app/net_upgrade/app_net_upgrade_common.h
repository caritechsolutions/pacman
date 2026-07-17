#include "app.h"
#include "app_config.h"

typedef int (*CfgGetCb)(char *url);
typedef int (*CfgVerifyCb)(char** tip);
typedef int (*FileGetCb)(char *url);
typedef int (*FileVerifyCb)(char** tip);
typedef int (*UpgradeCb)(void);
typedef NetUpgProcess (*ProcessCb)(void);

typedef struct {
    char *protocol;
    CfgGetCb cfg_get;
    CfgVerifyCb cfg_parse;
    FileGetCb upg_file_get;
    FileVerifyCb upg_file_verify;
    UpgradeCb upg_exec;
    ProcessCb process_get;
}UpgradeProtocol;

extern GxHwMallocObj s_upgrade_bin_data_buf;
extern GxHwMallocObj s_upgrade_recv_os_data_buf;
extern void netupg_download_callback(int message, int body);

extern int config_file_parse(char **info);
extern int upgrade_file_verify(char** tip);
extern int upgrade_exec(void);
extern NetUpgProcess upgrade_process_get(void);
