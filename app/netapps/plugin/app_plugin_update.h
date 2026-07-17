/*************************************************************************
	> File Name: app_plugin_update.h
	> Author:
	> Mail:
	> Created Time: 2016年10月20日 星期四 13时43分16秒
 ************************************************************************/

#ifndef _APP_PLUGIN_UPDATE_H
#define _APP_PLUGIN_UPDATE_H
#include"plugin_list.h"
#include"gx_pluginstore_api.h"

#define PLUGIN_SETTING_FILE_PATH "/home/gx/plugincfg"

#define MAX_PATH_LEN     (512)
#define MAX_ARRAY_LEN    (512)

#define PLUGIN_ERR(fmt, args...)  do {                              \
    app_log_error(fmt, ##args);                                     \
} while(0)

#define PLUGIN_INFO(fmt, args...)  do {                             \
    app_log_info(fmt, ##args);                                      \
} while(0)

#define PLUGIN_DBG(fmt, args...)  do {                              \
    app_log_debug(fmt, ##args);                                     \
} while(0)

#if PLUGINONLINE_SUPPORT
#define CLASSIFICATION_INSTALL_NAME "Installed"
#define STR_INSTALLED "Installed"

typedef struct _PluginUpdateNode{
    int counter;
    char *id;
    char *name;
    char *ins_ver;
    char *repo_ver;
    char *status;
    char *icon;
    char *shortdesc;
    char *url;
    struct _PluginUpdateNode *link;
    struct dlist_head list;
}PluginUpdateNode, PluginUpdateList;
#endif

typedef struct _PluginClassificationNode{
    int counter;
    char *name;
#if PLUGINONLINE_SUPPORT
    PluginUpdateList *update_list_head;
#endif
    struct dlist_head list;
}PluginClassificationNode, PLuginClassificationList;


void Plugin_classification_list_get(PLuginClassificationList **head);
void Plugin_classification_list_free(PLuginClassificationList **head);
int Plugin_info_list_get(PluginInfoList **head);
#if PLUGINONLINE_SUPPORT
int Plugin_download_install(PluginUpdateNode *node, PluginClassificationNode *classification_node);
int Plugin_download_uninstall(PluginUpdateNode *node, PluginClassificationNode *classification_node);
int Plugin_download_update(PluginUpdateNode *node);
void Plugin_installed_check(PluginClassificationNode *node);
int Plugin_update_check(PluginUpdateNode *node);
void Plugin_download_stop(void);
int Plugin_need_update_counter_get(void);
#endif
#endif
