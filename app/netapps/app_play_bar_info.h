/*************************************************************************
	> File Name: app_play_bar_info.h
	> Author:
	> Mail:
	> Created Time: 2017年12月10日 星期日 21时11分32秒
 ************************************************************************/

#ifndef _APP_PLUGIN_PLAY_INFO_H
#define _APP_PLUGIN_PLAY_INFO_H

#include"iptv/app_iptv.h"

typedef struct _PlayBarInfoOps{
    int channel_total;
    char *bar_title;
    int channel_num;
    char *name;
    char *url;
#ifdef IPTV_EPG_SUPPORT
    NetappsEpgCtrl epg_ctrl;
#endif
}PlayBarInfoOps;

int app_play_bar_info_show(PlayBarInfoOps *info_ops);
int app_play_bar_info_hide(void);

#endif
