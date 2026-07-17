/*************************************************************************
	> File Name: app_plugin_server.h
	> Author:
	> Mail:
	> Created Time: 2016年09月22日 星期四 15时06分23秒
 ************************************************************************/

#ifndef _APP_PLUGIN_SERVER_H
#define _APP_PLUGIN_SERVER_H
#include "gx_pluginstore_api.h"

struct PlugiaSourceUIMsg{
    int type;
    int draw_mode;
    int item;
    int active;
    int total;
    char *image_path;
    char *status;
    SourceInfoNode node;
};
void app_plugin_source_list_deal_err_from_ecmascript(void);
void app_plugin_listview_play_deal_err_from_ecmascript(void);
char *source_url_json_parse(char *string);
#endif
