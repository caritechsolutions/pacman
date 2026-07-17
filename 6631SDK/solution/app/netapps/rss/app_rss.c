#include "app.h"
#ifdef RSS_SUPPORT
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_netapps_service.h"
#include <gx_apps.h>
#include "gx_rss.h"
#include "app_wnd_file_list.h"
#include "app_windows.h"
#include "app_common_view.h"
#include "iptv/app_iptv.h"
#include "iptv/app_iptv_list_service.h"

#define RSS_CHANNEL_LIST                g_CommonView.widget.list_group
#define RSS_NEWS_LIST                   g_CommonView.widget.list_item

#define RSSXML_FILE                     "rss.xml"
#define RSS_DEFAULT_CHANNELLIST_PATH    WORK_PATH"theme/rsslist.xml"
#define RSS_USER_CHANNELLIS_PATH        "/home/gx/rss_userchannel.xml"

#define MAX_RSS_CHANNEL_NUM             (32)
#define RSS_CHANNEL_NAME                (32)
#define RSS_CHANNEL_URL                 (256)
#define RSS_POP_MSG_TIMEOUT             (2000)

#define STR_ID_NET_RSS  "Rss Reader"
#define STR_ID_RSS_NO_FIND_XML         "Not find rss.xml"

typedef struct rss_chanel_list_infor_tag {
    char name[RSS_CHANNEL_NAME];
    char url[RSS_CHANNEL_URL];
} rss_chanel_list_infor_t;

static gx_list_t *rss_channellist = NULL;
gx_rss_newslist_t *gRss_newlist = NULL;
rss_chanel_list_infor_t RssList_Array[MAX_RSS_CHANNEL_NUM];
static event_list *s_rss_news_timer = NULL;

static int is_rss_download = 0;
static int rss_data_valid = 0;
static int news_list_index = 0;

static bool s_rss_channel_add_flag = false;
static const char *s_rss_channel_edit_ret = NULL;
rss_chanel_list_infor_t s_rss_channel_edit_info;

static void rss_channel_list_init(void)
{
    int i = 0;
    int channel_count = 0;
    if (rss_channellist)
    {
        gx_list_free(rss_channellist);
        rss_channellist = NULL;
    }

    if (GXCORE_FILE_EXIST == GxCore_FileExists(RSS_USER_CHANNELLIS_PATH))
        rss_channellist = gx_get_list_from_file(RSS_USER_CHANNELLIS_PATH);
    if (!rss_channellist)
    {
        if (GXCORE_FILE_EXIST == GxCore_FileExists(RSS_DEFAULT_CHANNELLIST_PATH))
            rss_channellist = gx_get_list_from_file(RSS_DEFAULT_CHANNELLIST_PATH);
        if (!rss_channellist)
            rss_channellist = gx_list_new();
        gx_list_save_to_file(rss_channellist, RSS_USER_CHANNELLIS_PATH);
    }
    if ((rss_channellist->item_num <= 0) || (rss_channellist->items == NULL))
        return;

    if (rss_channellist->item_num > RSS_CHANNEL_NAME)
        channel_count = RSS_CHANNEL_NAME;
    else
        channel_count = rss_channellist->item_num;

    for (i = 0; i < channel_count; i++)
    {
        if (strlen(rss_channellist->items[i].key) < RSS_CHANNEL_NAME)
            strcpy((char *)(RssList_Array[i].name), (rss_channellist->items[i].key));
        else
            strncpy((char *)(RssList_Array[i].name), (rss_channellist->items[i].key), RSS_CHANNEL_NAME - 1);

        if (strlen(rss_channellist->items[i].value) < RSS_CHANNEL_URL)
            strcpy((char *)(RssList_Array[i].url), (rss_channellist->items[i].value));
        else
            strncpy((char *)(RssList_Array[i].url), (rss_channellist->items[i].value), RSS_CHANNEL_URL - 1);
    }
}

static void rss_channel_list_destroy(void)
{
    if (rss_channellist)
    {
        //gx_list_save_to_file(rss_channellist, RSS_USER_CHANNELLIS_PATH);
        gx_list_free(rss_channellist);
        rss_channellist = NULL;
    }
}

void app_rss_news_download(void *usrdata)
{
    int sel = *(int *)usrdata;
    gxapps_ret_t ret = GXAPPS_SUCCESS;

    GxCore_ThreadDetach();
    if (gRss_newlist)
    {
        gx_rss_free_newslist(gRss_newlist);
        gRss_newlist = NULL;
    }

    ret  = gx_rss_get_news(RssList_Array[sel].url, &gRss_newlist);
    if (ret == GXAPPS_SUCCESS)
    {
        rss_data_valid = 1;
    }
    is_rss_download = 0;
}

int app_rss_news_update(void *usrdata)
{
    if (!is_rss_download)
    {
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
        {
            APP_TIMER_REMOVE(s_rss_news_timer);
            return 0;
        }
        if((strcmp(GUI_GetFocusWindow(), WND_SYSTEM_SETTING) == 0)
            || GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
        {
            return 0;
        }

        if (rss_data_valid)
        {
            GUI_SetProperty(RSS_NEWS_LIST, "update_all", NULL);
            app_common_view_hide_popup_msg();
            app_update_pop_dlg(WND_POP_TIP);
        }
        else
        {
            app_common_view_show_popup_msg("News Download Error!", RSS_POP_MSG_TIMEOUT);
        }

        APP_TIMER_REMOVE(s_rss_news_timer);
    }
    return 0;
}

int app_rss_news_download_start(int *sel)
{
    static int thread_id = 0;
    static int download_sel = 0;

    if (sel)
        download_sel = *sel;
    else
        GUI_GetProperty(RSS_CHANNEL_LIST, "select", &download_sel);
    GUI_SetProperty(RSS_CHANNEL_LIST, "active", &download_sel);

    app_common_view_show_popup_msg("News Downloading...", 0);
    APP_TIMER_ADD(s_rss_news_timer, app_rss_news_update, 100, TIMER_REPEAT);
    is_rss_download = 1;
    rss_data_valid = 0;
    GxCore_ThreadCreate("rss", &thread_id, app_rss_news_download,
            (void *)&download_sel, 64 * 1024, GXOS_DEFAULT_PRIORITY);

    return 0;
}

int app_rss_news_download_stop(void)
{
    app_common_view_hide_popup_msg();
    APP_TIMER_REMOVE(s_rss_news_timer);
    return 0;
}

static int app_rss_busy_check(void)
{
    return is_rss_download;
}

static bool _rss_channel_full_check(void)
{
    if (rss_channellist != NULL)
    {
        if (rss_channellist->item_num >= MAX_RSS_CHANNEL_NUM)
        {
            app_common_view_show_popup_msg("Channel list full!", RSS_POP_MSG_TIMEOUT);
            return true;
        }
    }
    return false;
}

static bool _rss_channel_empty_check(void)
{
    if(rss_channellist != NULL)
    {
        if(rss_channellist->item_num > 0 && rss_channellist->items != NULL)
            return false;
    }
    return true;
}

static void app_rss_news_list_clear(void)
{
    if (gRss_newlist)
    {
        gx_rss_free_newslist(gRss_newlist);
        gRss_newlist = NULL;
    }

    GUI_SetProperty(RSS_NEWS_LIST, "update_all", NULL);
}

//#define MUTE_EIGHT_TO_CONVER_OUT
#ifdef MUTE_EIGHT_TO_CONVER_OUT
static int rss_copy_file_to_file(const char *src, const char *dst, int unit_size)
{
#define _fclose_fp(fp) do {if (fp) fclose(fp); fp = NULL; } while(0)
#define _free_ptr(ptr) do {if (ptr) GxCore_Free(ptr); ptr = NULL; } while(0)

    int ret = -1;
    FILE *rfp = NULL, *wfp = NULL;
    uint8_t *data = NULL;
    int size = 0;

    if (!src || !dst)
        return -1;
    if ((rfp = fopen(src, "r")) == NULL)
        return -1;
    if ((wfp = fopen(dst, "w+")) == NULL)
        goto end;
    if (unit_size < 512)
        unit_size = 8192;
    if ((data = GxCore_Malloc(unit_size)) == NULL)
        goto end;
    while ((size = fread(data, 1, unit_size, rfp)) > 0) {
        if (size != fwrite(data, 1, size, wfp))
            goto end;
    }

    ret = 0;
end:
    _fclose_fp(rfp);
    _fclose_fp(wfp);
    _free_ptr(data);
    if (ret < 0 && access(dst, F_OK) == 0)
        unlink(dst);
    return ret;
}

static int convert_rss_out_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        HotplugPartitionList *partition_list = NULL;
        char filepath[32] = {0};

        if (check_usb_status() == false)
        {
            app_common_view_show_popup_msg(STR_ID_INSERT_USB, RSS_POP_MSG_TIMEOUT);
            return 0;
        }
        partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
        if (partition_list == NULL || partition_list->partition_num == 0)
        {
            app_common_view_show_popup_msg(STR_ID_INSERT_USB, RSS_POP_MSG_TIMEOUT);
            return 0;
        }
        sprintf(filepath, "%s/%s", partition_list->partition[0].partition_entry, RSSXML_FILE);
        if (rss_copy_file_to_file("/home/gx/rss_userchannel.xml", filepath, 8192) < 0)
        {
            app_common_view_show_popup_msg("Create the USB file fail!", RSS_POP_MSG_TIMEOUT);
            return 0;
        }
        app_common_view_show_popup_msg("Create the USB file success!", RSS_POP_MSG_TIMEOUT);
    }
    return 0;
}

static void app_rss_key_mute_cb(void)
{
    static int mute_counter = 0;
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if ((mute_counter++) >= 8)
        {
            PopDlg  pop;

            mute_counter = 0;
            if (check_usb_status() == false)
            {
                app_common_view_show_popup_msg(STR_ID_INSERT_USB, RSS_POP_MSG_TIMEOUT);
                return;
            }
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.format = POP_FORMAT_DLG;
            pop.str = "are you sure to convert out rss.xml?";
            pop.mode = POP_MODE_UNBLOCK;
            pop.exit_cb = convert_rss_out_cb;
            popdlg_create(&pop);
        }
    }
}
#endif

static void app_rss_key_ok_cb(void)
{
    int sel = 0;
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        GUI_GetProperty(RSS_CHANNEL_LIST, "select", &news_list_index);
        app_rss_news_list_clear();
        app_rss_news_download_start(NULL);
    }
    else if (area == COMMON_VIEW_AREA_LIST_ITEM)
    {
        GUI_GetProperty(RSS_NEWS_LIST, "select", &sel);
        if (gRss_newlist == NULL || gRss_newlist->newses == NULL || sel < 0)
            return;
        GUI_CreateDialog(WND_COMMON_TEXT);
        app_common_text_set_title(gRss_newlist->newses[sel].title);
        app_common_text_set_content(gRss_newlist->newses[sel].description);
    }
}

static int app_rss_delete_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        int sel = 0;

        GUI_GetProperty(RSS_CHANNEL_LIST, "select", &sel);
        gx_list_delete_item(rss_channellist, sel);
        gx_list_save_to_file(rss_channellist, RSS_USER_CHANNELLIS_PATH);
        rss_channel_list_init();
        GUI_SetProperty(RSS_CHANNEL_LIST, "update_all", NULL);
        if (gRss_newlist && sel == news_list_index)
        {
            sel = -1;
            GUI_GetProperty(RSS_CHANNEL_LIST, "active", &sel);
            gx_rss_free_newslist(gRss_newlist);
            gRss_newlist = NULL;
            GUI_SetProperty(RSS_NEWS_LIST, "update_all", NULL);
        }
        else
        {
			if(news_list_index > sel)
				news_list_index = news_list_index - 1;
            GUI_SetProperty(RSS_CHANNEL_LIST, "active", &news_list_index);
        }
        app_common_view_show_popup_msg(STR_ID_DEL_CH_OK, RSS_POP_MSG_TIMEOUT);
    }
    return 0;
}

static void app_rss_key_red_cb(void) //Del
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        PopDlg  pop;
        if (rss_channellist == NULL || rss_channellist->item_num == 0 || true == app_common_view_popup_state_get())
            return;

        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.str = "Sure to delete this Channel?";
        pop.title = "Delete Channel";
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = app_rss_delete_cb;
        popdlg_create(&pop);
    }
}

static void rss_setting_get(char *param1, char *param2, char *param3, char *param4)
{
    strncpy(param1, s_rss_channel_edit_info.name, RSS_CHANNEL_NAME-1);
    param1[RSS_CHANNEL_NAME-1] = 0;
    strncpy(param2, s_rss_channel_edit_info.url, RSS_CHANNEL_URL-1);
    param2[RSS_CHANNEL_URL-1] = 0;
}

static void rss_setting_set(char *param1, char *param2, char *param3, char *param4)
{
    strncpy(s_rss_channel_edit_info.name, param1, RSS_CHANNEL_NAME-1);
    s_rss_channel_edit_info.name[RSS_CHANNEL_NAME-1] = 0;
    strncpy(s_rss_channel_edit_info.url, param2, RSS_CHANNEL_URL-1);
    s_rss_channel_edit_info.url[RSS_CHANNEL_URL-1] = 0;
}
static void rss_setting_save(void)
{
    PopDlg  pop;

    if (rss_channellist != NULL)
    {

        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;

        if (strlen(s_rss_channel_edit_info.name) == 0)
        {
            pop.str = "Invalid Channel!";
            popdlg_create(&pop);
            return;
        }
        if (strlen(s_rss_channel_edit_info.url) == 0)
        {
            pop.str = "Invalid URL!";
            popdlg_create(&pop);
            return;
        }

        if (s_rss_channel_add_flag)
        {
            if (rss_channellist->item_num < MAX_RSS_CHANNEL_NUM)
            {
                gx_list_add_item(rss_channellist, s_rss_channel_edit_info.name, s_rss_channel_edit_info.url, NULL);
                s_rss_channel_edit_ret = STR_ID_AD_CH_OK;
            }
        }
        else
        {
            int sel = 0;
            GUI_GetProperty(RSS_CHANNEL_LIST, "select", &sel);
            gx_list_edit_item(rss_channellist, sel, s_rss_channel_edit_info.name, s_rss_channel_edit_info.url, NULL);
            s_rss_channel_edit_ret = STR_ID_ED_CH_OK;
        }

        gx_list_save_to_file(rss_channellist, RSS_USER_CHANNELLIS_PATH);

        GUI_EndDialog(WND_SYSTEM_SETTING);

        rss_channel_list_init();

    }
}

void rss_create_setting(bool is_add)
{
    int sel = 0;
    IPTVSetCtrl rss_set;

    s_rss_channel_add_flag = is_add;
    memset(&rss_set, 0, sizeof(IPTVSetCtrl));
    rss_set.server_type = IPTV_SERVER_RSS;
    if (is_add)
    {
        rss_set.name = "Add Channel";
        memset(&s_rss_channel_edit_info, 0, sizeof(s_rss_channel_edit_info));
        strncpy(s_rss_channel_edit_info.url, "http://", RSS_CHANNEL_URL-1);
        s_rss_channel_edit_info.url[RSS_CHANNEL_URL-1] = 0;
    }
    else
    {
        rss_set.name = "Edit Channel";
        GUI_GetProperty(RSS_CHANNEL_LIST, "select", &sel);
        memcpy(&s_rss_channel_edit_info, &RssList_Array[sel], sizeof(s_rss_channel_edit_info));
    }
    rss_set.param_num = 2;
    rss_set.param_len[0] = RSS_CHANNEL_NAME;
    rss_set.param_len[1] = RSS_CHANNEL_URL;
    rss_set.param1_name = STR_ID_CHANNEL;
    rss_set.param2_name = "URL";
    rss_set.setting_get = rss_setting_get;
    rss_set.setting_set = rss_setting_set;
    rss_set.save_func = rss_setting_save;
    rss_set.get_status = NULL; //rss_get_status;

    app_iptv_create_setting_menu(&rss_set);
}

static void app_rss_key_green_cb(void) //Add
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if (_rss_channel_full_check())
            return;
        rss_create_setting(true);
    }
}

static void app_rss_key_yellow_cb(void) //Edit
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        if(true == _rss_channel_empty_check() || true == app_common_view_popup_state_get())
            return;

        rss_create_setting(false);
    }
}

static void app_rss_key_blue_cb(void) //Import Channel
{
    CommonViewFocusArea area = app_common_view_focus_area_get();

    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        int i = 0, j = 0;
        char filepath[32] = {0};
        HotplugPartitionList *partition_list = NULL;

        if (_rss_channel_full_check())
            return;

        if (check_usb_status() == false)
        {
            app_common_view_show_popup_msg(STR_ID_INSERT_USB, RSS_POP_MSG_TIMEOUT);
            return;
        }

        partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
        if (partition_list->partition_num > 0)
        {
            for (i = 0; i < partition_list->partition_num; i++)
            {
                sprintf(filepath, "%s/%s", partition_list->partition[i].partition_entry, RSSXML_FILE);
                if (GXCORE_FILE_EXIST == GxCore_FileExists(filepath))
                {
                    gx_list_t *channel_list = gx_get_list_from_file(filepath);
                    if (channel_list)
                    {
                        if (rss_channellist != NULL)
                        {
                            int channel_add_count = 0;
                            if (channel_list->item_num > (MAX_RSS_CHANNEL_NUM - rss_channellist->item_num))
                                channel_add_count = (MAX_RSS_CHANNEL_NUM - rss_channellist->item_num);
                            else
                                channel_add_count  = channel_list->item_num;
                            for (j = 0 ; j < channel_add_count; j++)
                            {
                                gx_list_add_item(rss_channellist, channel_list->items[j].key, channel_list->items[j].value, NULL);
                            }
                            gx_list_save_to_file(rss_channellist, RSS_USER_CHANNELLIS_PATH);

                        }

                        gx_list_free(channel_list);
                        channel_list = NULL;
                        rss_channel_list_init();
                        GUI_SetProperty(RSS_CHANNEL_LIST, "update_all", NULL);
                        GUI_SetProperty(RSS_CHANNEL_LIST, "active", &news_list_index);
                        app_common_view_show_popup_msg("Add Channel Success!", RSS_POP_MSG_TIMEOUT);
                    }
                    else
                    {
                        app_common_view_show_popup_msg("Add Channel failed!", RSS_POP_MSG_TIMEOUT);
                    }
                    return;
                }
            }
        }
        app_common_view_show_popup_msg(STR_ID_RSS_NO_FIND_XML, RSS_POP_MSG_TIMEOUT);
    }
}

static void app_rss_focus_area_change(int area)
{
    if (area == COMMON_VIEW_AREA_LIST_GROUP)
    {
        app_common_view_change_tips_key(STBK_OK, true, "Update", app_rss_key_ok_cb);
        app_common_view_change_tips_key(STBK_RED, true, "Delete", app_rss_key_red_cb);
        app_common_view_change_tips_key(STBK_GREEN, true, "Add", app_rss_key_green_cb);
        app_common_view_change_tips_key(STBK_YELLOW, true, "Edit", app_rss_key_yellow_cb);
        app_common_view_change_tips_key(STBK_BLUE, true, STR_ID_IMPORT, app_rss_key_blue_cb);
#ifdef MUTE_EIGHT_TO_CONVER_OUT
        app_common_view_change_tips_key(STBK_MUTE, true, NULL, app_rss_key_mute_cb);
#endif
    }
    else
    {
        app_common_view_change_tips_key(STBK_OK, true, "Info", app_rss_key_ok_cb);
        app_common_view_change_tips_key(STBK_RED, false, NULL, NULL);
        app_common_view_change_tips_key(STBK_GREEN, false, NULL, NULL);
        app_common_view_change_tips_key(STBK_YELLOW, false, NULL, NULL);
        app_common_view_change_tips_key(STBK_BLUE, false, NULL, NULL);
#ifdef MUTE_EIGHT_TO_CONVER_OUT
        app_common_view_change_tips_key(STBK_MUTE, false, NULL, NULL);
#endif
    }
}

int app_rss_create(GuiWidget *widget, void *usrdata)
{
    int sel = 0;

    if (gRss_newlist)
    {
        gx_rss_free_newslist(gRss_newlist);
        gRss_newlist = NULL;
    }
    rss_channel_list_init();
    s_rss_channel_edit_ret = NULL;
    is_rss_download = 0;
    rss_data_valid = 0;
    news_list_index = 0;
    if (rss_channellist->item_num > 0 && rss_channellist->items != NULL)
    {
        app_rss_news_download_start(&sel);
    }

    return EVENT_TRANSFER_STOP;
}

int app_rss_destroy(GuiWidget *widget, void *usrdata)
{
    app_rss_news_download_stop();
    if (gRss_newlist)
    {
        gx_rss_free_newslist(gRss_newlist);
        gRss_newlist = NULL;
    }
    rss_channel_list_destroy();

    return EVENT_TRANSFER_STOP;
}

int app_rss_got_focus(GuiWidget *widget, void *usrdata)
{
    if (s_rss_channel_edit_ret)
    {
        app_rss_news_update(NULL);
        GUI_SetProperty(RSS_CHANNEL_LIST, "update_all", NULL);
        GUI_SetProperty(RSS_CHANNEL_LIST, "active", &news_list_index);
        app_common_view_show_popup_msg(s_rss_channel_edit_ret, RSS_POP_MSG_TIMEOUT);
        s_rss_channel_edit_ret = NULL;
    }
    return EVENT_TRANSFER_STOP;
}

int app_rss_channel_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (rss_channellist == NULL)
        return 0;
    return rss_channellist->item_num;
}

int app_rss_channel_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = (ListItemPara *)usrdata;

    if (rss_channellist == NULL || rss_channellist->items == NULL || item == NULL || item->sel < 0)
        return EVENT_TRANSFER_STOP;

    //col-0: channel name
    item->image = NULL;
    item->string = rss_channellist->items[item->sel].key;

    return EVENT_TRANSFER_STOP;
}

int app_rss_news_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (gRss_newlist == NULL)
        return 0;
    return gRss_newlist->news_num;
}

int app_rss_news_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = (ListItemPara *)usrdata;
    static char buffer1[8];

    if (gRss_newlist == NULL || gRss_newlist->newses == NULL || item == NULL || item->sel < 0)
        return EVENT_TRANSFER_STOP;

    //col-0: channel No.
    item->image = NULL;
    sprintf(buffer1, "%03d", (item->sel + 1));
    item->string = buffer1;

    //col-1: channel name
    item = item->next;
    item->image = NULL;
        item->string = gRss_newlist->newses[item->sel].title;

    return EVENT_TRANSFER_STOP;
}

void app_rss_create_dialog(void)
{
    CommonViewParas paras = {0};

#ifdef ECOS_OS
    gx_rss_set_buffer_size(512*1024);
#endif
    paras.show_type = COMMON_VIEW_LIST_ITEM_TYPE;
    paras.focus_area = COMMON_VIEW_AREA_LIST_GROUP;
    paras.orig_group_sel = 0;
    paras.net_video_flag = true;
    paras.string.str_title = "RSS";
    paras.string.str_item_title = "RSS News List";

    paras.keyfunc.busy_check = app_rss_busy_check;
    paras.keyfunc.focus_area_change = app_rss_focus_area_change;

    paras.signal.create = app_rss_create;
    paras.signal.destroy = app_rss_destroy;
    paras.signal.got_focus = app_rss_got_focus;
    paras.signal.group_list_get_total = app_rss_channel_list_get_total;
    paras.signal.group_list_get_data = app_rss_channel_list_get_data;
    paras.signal.item_list_get_total = app_rss_news_list_get_total;
    paras.signal.item_list_get_data = app_rss_news_list_get_data;
    app_common_view_create_dialog(&paras);

}
#endif

