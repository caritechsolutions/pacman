#include "app.h"
#include "app_module.h"
#if ((NETAPPS_SUPPORT>0)&&(MEDIA_SUBTITLE_SUPPORT>0))
#include "app_wnd_file_list.h"
#include "app_pop.h"
#include "app_utility.h"
#include "pmp_subtitle.h"
#include "pmp_dvb_subt.h"
#include "pmp_psi.h"
#include "app_windows.h"
#include "module/app_osd_subtitle.h"
#include "app_netvideo_play.h"

#define MAX_SUB_NAME_LEN 16
#define BOX_NETVIDEO_SUBT           "wnd_netvideo_subt_box"
#define COMBOBOX_SUBT_TYPE          "wnd_netvideo_subt_setting_combobox01"
#define COMBOBOX_SUBT_LANGUAGE      "wnd_netvideo_subt_setting_combobox02"
#define COMBOBOX_SUBT_ENCODING      "wnd_netvideo_subt_setting_combobox03"
#define COMBOBOX_SUBT_FONTSIZE      "wnd_netvideo_subt_setting_combobox04"


#define BOXITEM_SUBT_TYPE          "wnd_netvideo_subt_boxitem0"
#define BOXITEM_SUBT_LAN           "wnd_netvideo_subt_boxitem1"
#define BOXITEM_SUBT_CP            "wnd_netvideo_subt_boxitem2"
#define BOXITEM_SUBT_FONT          "wnd_netvideo_subt_boxitem3"

extern uint32_t app_moive_subt_lang_to_charset(void);
extern NetVideoSubtitleList *app_net_video_get_subtitlelist(void);
extern char *app_get_short_lang_str(uint32_t lang);

typedef enum
{
    NETVIDEO_SUBT_TYPE,
    NETVIDEO_SUBT_LANGUAGE,
    NETVIDEO_SUBT_ENCODING,
    NETVIDEO_SUBT_FONTSIZE,
}SubtBox;

typedef enum
{
    NET_SUBTITLE_OFF = 0,
    NET_SUBTITLE_INSIDE,
    NET_SUBTITLE_OUTSIDE,
}NetSubtitleMode;

static void _netvideo_sub_change_cb(void)
{
    uint32_t type = 0;
    uint32_t value_sel = 0;
    GUI_GetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &value_sel);
    GUI_GetProperty(COMBOBOX_SUBT_TYPE,"select", &type);
    if(type == NET_SUBTITLE_INSIDE)
    {
        PlayerProgInfo *info = (PlayerProgInfo *)app_get_netvideo_playinfo();
        if(info != NULL)
        {
            PlayerSubPID subt_pid;
            subt_pid.pid = value_sel;
            subt_pid.major = info->sub[value_sel].pid.major;
            subt_pid.minor = info->sub[value_sel].pid.minor;
            pmp_subt_para* subt_para = NULL;
            subt_para = subtitle_get();
            subt_para->cur_subt = value_sel;
            GxPlayer_MediaSubSwitch(PLAYER_FOR_IPTV,subt_para->list,subt_pid);
        }
    }
    else if(type == NET_SUBTITLE_OUTSIDE)
    {
        app_net_video_subtitle_status_set(SUBT_STATE_NEED_DOWNLOAD,value_sel);
    }
    value_sel = (type<<8)+value_sel;
    GxBus_ConfigSetInt(NETAPPS_SUBTILE_LANGUAGE, value_sel);
    return;
}

static status_t netvideo_key_leftright(unsigned short value)
{
    uint32_t item_sel = 0;
    uint32_t value_sel = 0;
    uint32_t type = 0;
    uint32_t lang_num = 0;
    int i = 0;

    GUI_GetProperty(BOX_NETVIDEO_SUBT, "select", (void*)&item_sel);
    switch(item_sel)
    {
        case NETVIDEO_SUBT_TYPE:
            {
                app_net_video_subtitle_status_set(SUBT_STATE_GET_URL,0);
                GUI_GetProperty(COMBOBOX_SUBT_TYPE,"select", &type);
                if(type == NET_SUBTITLE_OFF)
                {
                    subtitle_stop();
                    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", "[None]");
                    GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
                    GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
                    GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
                    GxBus_ConfigSetInt(NETAPPS_SUBTILE_LANGUAGE, value_sel);
                }
                else
                {
                    char subname[MAX_SUB_NAME_LEN];
                    PlayerProgInfo* info = NULL;
                    NetVideoSubtitleList* sublist = NULL;
                    if(type == NET_SUBTITLE_INSIDE)
                    {
                        info = (PlayerProgInfo *)app_get_netvideo_playinfo();
                        if(info != NULL)
                            lang_num = info->sub_num;
                    }
                    else
                    {
                       sublist = app_net_video_get_subtitlelist();
                       if(sublist != NULL)
                            lang_num = sublist->subtitle_num;
                    }
                    if(lang_num == 0)
                    {
                        subtitle_stop();
                        GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", "[None]");
                        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
                        GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
                        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
                        value_sel = (type<<8);
                        GxBus_ConfigSetInt(NETAPPS_SUBTILE_LANGUAGE, value_sel);
                    }
                    else
                    {
                        char* combobox_buf = NULL;
                        combobox_buf = GxCore_Malloc(MAX_SUB_NAME_LEN*lang_num);
                        memset(combobox_buf,0x0,sizeof(MAX_SUB_NAME_LEN*lang_num));
                        strcpy(combobox_buf,  "[");
                        for(i = 0;i < lang_num;i++)
                        {
                            if(i != 0)
                            {
                                strcat(combobox_buf, ",");
                            }
                            if(type == 1)
                            {
                                if(0 == strcmp(info->sub[i].lang,""))
                                {
                                    sprintf(info->sub[i].lang,"sub%d",i);
                                }
                                strcat(combobox_buf, info->sub[i].lang);
                            }
                            else
                            {
                                if(sublist->subtitle_item[i].title == NULL)
                                {
                                    memset(subname,0x0,sizeof(subname));
                                    sprintf(subname,"sub%d",i);
                                    sublist->subtitle_item[i].title = GxCore_Strdup(subname);
                                }
                                else if(0 == strcmp(sublist->subtitle_item[i].title,""))
                                {
                                    sprintf(sublist->subtitle_item[i].title,"sub%d",i);
                                }
                                strcat(combobox_buf, sublist->subtitle_item[i].title);
                            }
                        }
                        if(type == 1)
                            subtitle_start(NULL, PMP_SUBT_LOAD_INSIDE);
                        strcat(combobox_buf,  "]");
                        GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", (void*)combobox_buf);
                        GxCore_Free(combobox_buf);
                        GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &value_sel);
                        _netvideo_sub_change_cb();
                        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
#if OSD_SUBTITLE_SUPPORT
                        if(TEXT_SUBT == app_osd_subt_type_get())
                        {
                            GUI_SetProperty(BOXITEM_SUBT_CP, "state", "enable");
                            GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "enable");
                        }
                        else
                        {
                            GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
                            GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
                        }
#else
                        GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
                        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
#endif
                    }
                }
                break;
            }
        case NETVIDEO_SUBT_LANGUAGE:
            {
                _netvideo_sub_change_cb();
                break;
            }
#if OSD_SUBTITLE_SUPPORT
        case NETVIDEO_SUBT_ENCODING:
            {
                GUI_GetProperty(COMBOBOX_SUBT_ENCODING, "select", &value_sel);
                app_osd_subt_encoding_set((OsdSubtEncoding)value_sel);
                break;
            }
        case NETVIDEO_SUBT_FONTSIZE:
            {
                GUI_GetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &value_sel);
                app_osd_subt_font_set((OsdSubtFont)value_sel);
                break;
            }
#endif

        default:
            break;
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_netvideo_subt_setting_create(const char* widgetname, void *usrdata)
{
    int32_t i = 0, type = 0;
    int32_t value = 0, sel = 0;
    int lang_num = 0;
    GxBus_ConfigGetInt(NETAPPS_SUBTILE_LANGUAGE, &value, SUB_LANG);
    type = value>>8;
    sel = value&(0xff);
    if(type == NET_SUBTITLE_OFF)
    {
        GUI_SetProperty(COMBOBOX_SUBT_TYPE, "select", &sel);
        GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", "[None]");
        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
        GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
    }
    else
    {
        /*subtile language*/
        char subname[MAX_SUB_NAME_LEN];
        char* combobox_buf = NULL;
        PlayerProgInfo* info = NULL;
        NetVideoSubtitleList* sublist = NULL;
        if(type == NET_SUBTITLE_INSIDE)
        {
            info = (PlayerProgInfo *)app_get_netvideo_playinfo();
            if(info != NULL)
                lang_num = info->sub_num;
        }
        else
        {
            sublist = app_net_video_get_subtitlelist();
            if(sublist != NULL)
                lang_num = sublist->subtitle_num;
        }
        if(lang_num == 0)
        {
            GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", "[None]");
            GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
            GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
            GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
        }
        else
        {
            combobox_buf = GxCore_Malloc(MAX_SUB_NAME_LEN*lang_num);
            memset(combobox_buf,0x0,sizeof(MAX_SUB_NAME_LEN*lang_num));
            strcpy(combobox_buf,  "[");
            for(i = 0;i < lang_num;i++)
            {
                if(i != 0)
                {
                    strcat(combobox_buf, ",");
                }
                if(type == 1)
                {
                    if(0 == strcmp(info->sub[i].lang,""))
                    {
                        sprintf(info->sub[i].lang,"sub%d",i);
                    }
                    strcat(combobox_buf, info->sub[i].lang);
                }
                else
                {
                    if(sublist->subtitle_item[i].title == NULL)
                    {
                        memset(subname,0x0,sizeof(subname));
                        sprintf(subname,"sub%d",i);
                        sublist->subtitle_item[i].title = GxCore_Strdup(subname);
                    }
                    else if(0 == strcmp(sublist->subtitle_item[i].title,""))
                    {
                        sprintf(sublist->subtitle_item[i].title,"sub%d",i);
                    }
                    strcat(combobox_buf, sublist->subtitle_item[i].title);
                }
            }
            strcat(combobox_buf,  "]");
            GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", (void*)combobox_buf);
            GxCore_Free(combobox_buf);
            GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &sel);
            GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
            GUI_SetProperty(COMBOBOX_SUBT_TYPE, "select", &type);
#if OSD_SUBTITLE_SUPPORT
            if(TEXT_SUBT == app_osd_subt_type_get())
            {
                GUI_SetProperty(BOXITEM_SUBT_CP, "state", "enable");
                GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "enable");
            }
            else
            {
                GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
                GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
            }
            /*codepage select*/
            sel = app_osd_subt_encoding_get();
            GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &sel);

            /*font size select */
            sel = app_osd_subt_font_get();
            GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &sel);
#else
            GUI_SetProperty(BOXITEM_SUBT_CP, "state", "disable");
            GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
#endif
        }
   }
    return 0;
}

SIGNAL_HANDLER int app_netvideo_subt_setting_destroy(const char* widgetname, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int app_netvideo_subt_setting_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->key.sym)
    {
        case VK_BOOK_TRIGGER:
            GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
            GUI_SendEvent(GUI_GetFocusWindow(), event);
            return EVENT_TRANSFER_STOP;

        case STBK_EXIT:
        case STBK_MENU:
        case STBK_OK:
            GUI_EndDialog(WND_NETVIDEO_SUBT_SETTING);
            return EVENT_TRANSFER_STOP;

        case STBK_LEFT:
            netvideo_key_leftright(event->key.sym);
            break;

        case STBK_RIGHT:
            netvideo_key_leftright(event->key.sym);
            break;


        default:
            break;
    }

    return EVENT_TRANSFER_KEEPON;
}
#endif

