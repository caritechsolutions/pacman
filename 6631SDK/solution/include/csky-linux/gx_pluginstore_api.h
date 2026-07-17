/*************************************************************************
	> File Name: gx_pluginstore_api.h
	> Author:
	> Mail:
	> Created Time: 2016年09月02日 星期五 10时28分48秒
 ************************************************************************/

#ifndef _GX_PLUGINSTORE_API_H
#define _GX_PLUGINSTORE_API_H

#include "plugin_list.h"

#define STR_PLUGIN_MANAGE_ID "plugin"

#define TMP_DOWNLOAD_PATH "/tmp/plugin"

typedef struct _SourceInfoNode{
    int counter;
    char *type;
    char *title;
    char *extra_data;
    char *icon_url;
    char *shortdesc;
    struct _SourceInfoNode *child_list;
    struct dlist_head list;
}SourceInfoNode, SourceInfoList;

typedef struct _ClassificationNode{
    int counter;
    char *title;
    char *type;
    char *extra_data;
    SourceInfoList *source_list_head;
    struct dlist_head list;
}ClassificationNode, ClassificationList;

typedef struct _PluginInfoNode{
    int counter;
    char *id;
    char *version;
    char *title;
    char *folder;
    char *ui_type;
    char *synopsis;
    char *icon_url;
    char *setting;
    ClassificationList *classification_list_head;
    struct dlist_head list;
}PluginInfoNode, PluginInfoList;

void Plugin_store_init(void);
void Plugin_store_exit(void);
void Plugin_source_want_more_data(void);
int Classification_list_get(PluginInfoNode *node);
int Source_list_get(SourceInfoList **head, int index, int use_thread);
int Source_list_search_get(SourceInfoList **head, int index, int use_thread, char *search_str);
int Classification_list_refresh(ClassificationList *head);
int Source_list_refresh(SourceInfoList *head);
void Classification_list_free(ClassificationList **head);
void Source_list_free(SourceInfoList **head);
int Source_url_get(int index);
int Source_print_tree(void);
int Source_url_refresh(char **url);
void Source_url_refresh_close(void);
void Plugin_install(const char *name);
void Plugin_uninstall(const char *id);
int Plugin_zip_decompression(const char *src, char *des);
void Plugin_exit_abort(int abort);
extern const char *app_plugin_installed_path(void);
#endif

