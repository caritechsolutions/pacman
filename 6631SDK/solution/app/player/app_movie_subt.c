#include "app.h"
#if MEDIA_SUBTITLE_SUPPORT
#include "app_wnd_file_list.h"
#include "app_pop.h"
#include "app_utility.h"
#include "pmp_subtitle.h"
#include "pmp_dvb_subt.h"
#include "pmp_psi.h"
#include "app_windows.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#endif

static int32_t s_subt_type = 0;
static char *s_movie_subt_path = NULL;
static char *s_movie_subt_path_bak = NULL;
static int s_subt_select = 0;

typedef enum
{
    MOVIE_SUBT_TYPE = 0,
    MOVIE_SUBT_FILE,
    MOVIE_SUBT_VIS,
    MOVIE_SUBT_SEL,
    MOVIE_SUBT_DELAY,
    MOVIE_SUBT_LANGUAGE,
    MOVIE_SUBT_ENCODING,
    MOVIE_SUBT_FONTSIZE,
}SubtBox;

#define BOX_MOVIE_SUBT                  "movie_subt_box"
#define BUTTON_SUBT_LOAD                "movie_subt_button_load"
#define BOXITEM_SUBT_LOAD               "movie_subt_boxitem1"
#define BOXITEM_SUBT_SELECT             "movie_subt_boxitem3"
#define COMBOBOX_SUBT_VISIBILITY        "movie_subt_combo_visibility"
#define COMBOBOX_SUBT_SELECT            "movie_subt_combo_select"
#define COMBOBOX_SUBT_LANGUAGE          "movie_subt_combo_Language"
#define COMBOBOX_SUBT_ENCODING          "movie_subt_combo_encoding"
#define COMBOBOX_SUBT_FONTSIZE          "movie_subt_combo_FontSize"
#define BOXITEM_SUBT_TYPE               "movie_subt_boxitem0"
#define COMBOBOX_SUBT_TYPE              "movie_subt_combo_type"
#define BOXITEM_SUBT_DELAY              "movie_subt_boxitem4"
#define BUTTON_SUBT_DELAY               "movie_subt_button_delay"
#define MOVIE_SUBT_TITLE                "movie_subt_title"
#define BOXITEM_SUBT_LAN                "movie_subt_boxitem5"
#define BOXITEM_SUBT_ENCODING           "movie_subt_boxitem6"
#define BOXITEM_SUBT_FONT               "movie_subt_boxitem7"

extern int app_movie_dvb_subt_cnt(void);
extern int app_movie_dvb_subt_index(void);
extern status_t app_movie_dvb_subt_switch(int index);
extern PmpPsi *app_movie_psi_info_get(void);
#if OSD_SUBTITLE_SUPPORT
uint32_t app_moive_subt_lang_to_charset(void)
{
    OsdSubtCodePage codepage = {0};
    int lang_value = 0;

    GxBus_ConfigGetInt(MEDIA_SUBTILE_LANGUAGE, &lang_value, OSD_LANG);

    if(lang_value >= LANG_MAX)
        lang_value = 0;

    if ((strcmp(g_LangName[lang_value], STR_ID_ENGLISH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_FRENCH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_PORTUGUESE) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_SPAIN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_ITALIAN) == 0))
    {
        codepage.isocode = ISO885901;
        codepage.wincode = WINDOWS1252;
    }
    else if ((strcmp(g_LangName[lang_value], STR_ID_ARABIC) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_FARSI) == 0))
    {
        codepage.isocode = ISO885906;
        codepage.wincode = WINDOWS1256;
    }
    else if ((strcmp(g_LangName[lang_value], STR_ID_RUSSIAN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_BULGARIAN) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_UKRAINIAN) == 0))
    {
        codepage.isocode = ISO885905;
        codepage.wincode = WINDOWS1251;
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_TURKISH) == 0)
    {
        codepage.isocode = ISO885909;
        codepage.wincode = WINDOWS1254;
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_GERMAN) == 0)
    {
        codepage.isocode = ISO885902;
        codepage.wincode = WINDOWS1252;
    }
    else if(strcmp(g_LangName[lang_value], STR_ID_THAI) == 0)
    {
        codepage.isocode = ISO885911;
        codepage.wincode = WINDOWS1259;
    }
    else if((strcmp(g_LangName[lang_value], STR_ID_POLISH) == 0)
            ||(strcmp(g_LangName[lang_value], STR_ID_CZECH) == 0))
    {
        codepage.isocode = ISO885902;
        codepage.wincode = WINDOWS1250;
    }
    else
    {
        codepage.isocode = ISO885901;
        codepage.wincode = WINDOWS1250;
    }
    app_osd_subt_codepage_set(codepage);
    return 0;
}
#endif

int app_clear_movie_subt_path(void)
{
    s_movie_subt_path = NULL;
    APP_FREE(s_movie_subt_path_bak);

    return 0;
}

int app_subt_visibility_restore(void)
{
    pmpset_tone tone = pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY);
    if(app_movie_dvb_subt_cnt() > 0)
    {
        if(PMPSET_TONE_ON == tone)
        {
            movie_dvb_subt_show();
        }
        else
        {
            movie_dvb_subt_hide();
        }
    }
    else
    {
        pmp_subt_para* subt_para = NULL;
        subt_para = subtitle_get();
        if(subt_para == NULL)
            return 1;

        if(PMPSET_TONE_ON == tone)
        {
            GxPlayer_MediaSubShow(PMP_PLAYER_AV,subt_para->list);
        }
        else
        {
            GxPlayer_MediaSubHide(PMP_PLAYER_AV,subt_para->list);
        }
    }

    return 0;
}

static int combobox_set_player_subt_visibility(PlayerSubtitle* list, pmpset_tone tone)
{
    pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, tone);
    if(NULL == list)
        return 1;

    if(PMPSET_TONE_ON == tone)
    {
        app_log_debug("GXMSG_PLAYER_SUBTITLE_SHOW\n");
        GxPlayer_MediaSubShow(PMP_PLAYER_AV,list);
    }
    else
    {
        app_log_debug("GXMSG_PLAYER_SUBTITLE_HIDE\n");
        GxPlayer_MediaSubHide(PMP_PLAYER_AV,list);
    }
    return 0;
}

static int combobox_set_dvb_subt_visibility(pmpset_tone tone)
{
    if(PMPSET_TONE_ON == tone)
    {
        app_log_debug("MOVIE_SUBTITLE_SHOW\n");
        movie_dvb_subt_show();
        pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_ON);
    }
    else
    {
        app_log_debug("MOVIE_SUBTITLE_HIDE\n");
        movie_dvb_subt_hide();
        pmpset_set_int(PMPSET_MOVIE_SUBT_VISIBILITY, PMPSET_TONE_OFF);
    }
    return 0;
}

static int combobox_init_player_subt_select(pmp_subt_para* para)
{
    char* combobox_buf = NULL;
    char* combobox_node = NULL;
    char default_buf[20] = {0};
    uint32_t i = 0;
    uint32_t j = 0;
    PlayerProgInfo *player_info;

    if(para->para.type != PLAYER_SUB_TYPE_INSIDE)
    {
        GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)"[None]");
        return 1;
    }
    player_info = app_movie_player_info_get();

    if(0 >= player_info->sub_num||PLAYER_MAX_TRACK_SUB <player_info->sub_num)
    {
        GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)"[None]");
        return 1;
    }

    combobox_buf = GxCore_Calloc(1, PLAYER_MAX_TRACK_SUB*PLAYER_TARCK_LANG_LONG + PLAYER_MAX_TRACK_SUB+20);
    if(NULL == combobox_buf) return 1;

        /*combobox context*/
    strcpy(combobox_buf,  "[");
    for(i = 0; i < player_info->sub_num; i++)
    {
        if(0 != j) strcat(combobox_buf, ",");
        if( player_info->sub[i].type != PLAYER_SUB_TYPE_FILE )
        {
            combobox_node = player_info->sub[i].lang;

            /*default str*/
            if((NULL == combobox_node) || (0 == strlen(combobox_node)))
            {
                memset(default_buf, 0, sizeof(default_buf));
                sprintf(default_buf, "subt %d", i);
                combobox_node = default_buf;
            }

            strcat(combobox_buf, combobox_node);
            j++;
        }
    }
    strcat(combobox_buf,  "]");

    app_log_debug("SUBT: combobox_buf %s\n", combobox_buf);
    if(j == 0)
    {
         GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)"[None]");
         APP_FREE(combobox_buf);
         return 0;
    }
    GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)combobox_buf);
    GUI_SetProperty(COMBOBOX_SUBT_SELECT, "select", (void*)&para->cur_subt);
    APP_FREE(combobox_buf);

    return 0;
}

static int combobox_init_dvb_subt_select(void)
{
    char* combobox_buf = NULL;
    char* combobox_node = NULL;
    char default_buf[20] = {0};
    uint32_t i = 0;
    int sel = 0;
    PmpPsi *psi_info = NULL;

    if(app_movie_dvb_subt_cnt() == 0)
    {
        GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)"[None]");
        return 1;
    }

    psi_info = app_movie_psi_info_get();
    combobox_buf = GxCore_Calloc(1, psi_info->subt_info.num * PLAYER_TARCK_LANG_LONG + psi_info->subt_info.num + 20);
    if(NULL == combobox_buf)
        return 1;

    /*combobox context*/
    strcpy(combobox_buf,  "[");
    for(i = 0; i < psi_info->subt_info.num; i++)
    {
        if(0 != i) strcat(combobox_buf, ",");

        memset(default_buf, 0, sizeof(default_buf));
        strncpy(default_buf, (char*)psi_info->subt_info.data[i].lang, sizeof(psi_info->subt_info.data[i].lang));
        /*default str*/
        if(0 == strlen(default_buf))
        {
            memset(default_buf, 0, sizeof(default_buf));
            sprintf(default_buf, "subt %d", i);
        }
        combobox_node = default_buf;
        strcat(combobox_buf, combobox_node);
    }
    strcat(combobox_buf,  "]");

    app_log_debug("SUBT: combobox_buf %s\n", combobox_buf);
    GUI_SetProperty(COMBOBOX_SUBT_SELECT, "content", (void*)combobox_buf);
    sel = app_movie_dvb_subt_index();
    GUI_SetProperty(COMBOBOX_SUBT_SELECT, "select", &sel);
    APP_FREE(combobox_buf);

    return 0;
}

static int combobox_set_player_subt_select(PlayerSubtitle* list, int index)
{
    PlayerSubPID subt_pid;
    PlayerProgInfo *player_info;

    if(NULL == list) return 1;

    player_info = app_movie_player_info_get();
    if(index < player_info->sub_num)
    {
        subt_pid.pid = app_get_subt_real_select(index);
        subt_pid.major = 0;
        subt_pid.minor = 0;

        app_log_debug("GXMSG_PLAYER_SUBTITLE_SWITCH index = %d pid = %d\n", index, subt_pid.pid);
        GxPlayer_MediaSubSwitch(PMP_PLAYER_AV,list,subt_pid);
    }
    return 0;
}

static int combobox_set_dvb_subt_select(int index)
{
    app_movie_dvb_subt_switch(index);
    return 0;
}

static int combobox_set_player_subt_sync(PlayerSubtitle* list, int timems)
{
    if(NULL == list)
        return 1;

    app_log_debug("GXMSG_PLAYER_SUBTITLE_SYNC\n");
    GxPlayer_MediaSubSync(PMP_PLAYER_AV, list, timems);

    return 0;
}

static int combobox_player_subt_update(pmp_subt_para* subt_para)
{
    int32_t value = 0;
    char string[10] = {0};

    combobox_init_player_subt_select(subt_para);

    value = PMPSET_TONE_ON;
    GUI_SetProperty(COMBOBOX_SUBT_VISIBILITY, "select", (void*)&value);
    combobox_set_player_subt_visibility(subt_para->list, value);

    subt_para->delay_ms = 0;
    memset(string,0,sizeof(string));
    sprintf(string, "%dms", subt_para->delay_ms);
    GUI_SetProperty(BUTTON_SUBT_DELAY, "string", (void*)string);
    GxPlayer_MediaSubSync(PMP_PLAYER_AV, subt_para->list,  subt_para->delay_ms);

    return 0;
}

static int _app_movie_subt_filedlg_exit_cb(WndStatus ret)
{
    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    APP_CHECK_P(subt_para, GXCORE_ERROR);

    if(ret == WND_OK)
    {
        app_get_file_real_path(&s_movie_subt_path);

        if(s_movie_subt_path != NULL)
        {
            play_movie_info_and_sub_bak_set(s_movie_subt_path);
            subtitle_start(s_movie_subt_path, PMP_SUBT_LOAD_OUTSIDE);
            GUI_SetProperty(BUTTON_SUBT_LOAD, "string", (void*)s_movie_subt_path);
            combobox_player_subt_update(subt_para);
        }
        else
        {
             GUI_SetProperty(BUTTON_SUBT_LOAD, "string", (void*)(subt_para->file));
        }
        APP_FREE(s_movie_subt_path_bak);
    }
    else
    {
        s_movie_subt_path = s_movie_subt_path_bak;
    }

    return 0;
}

static status_t app_movie_subt_button_load(void)
{
    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    APP_CHECK_P(subt_para, GXCORE_ERROR);

    if(s_subt_type == 0) //outside
    {
        if(check_usb_status() == false)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.str = STR_ID_INSERT_USB;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
        else
        {
            FileListParam file_para;
            memset(&file_para, 0, sizeof(file_para));
            file_para.suffix = SUFFIX_SUBT;

            extern char* play_movie_sub_get(void);
            if(play_movie_sub_get() != NULL)
            {
                file_para.cur_path = play_movie_sub_get();
            }
            else if(explorer_view != NULL)
            {
                file_para.cur_path = explorer_view->path;
            }
            else{;}

            if(s_movie_subt_path)
            {
                if(s_movie_subt_path_bak == NULL)
                {
                    s_movie_subt_path_bak = GxCore_Strdup(s_movie_subt_path);
                }
            }
            file_para.dest_path = &s_movie_subt_path;
            file_para.dest_mode = DEST_MODE_FILE;
            file_para.exit_cb = _app_movie_subt_filedlg_exit_cb;
            app_get_file_path_dlg(&file_para);
        }
    }
    return GXCORE_SUCCESS;
}


static status_t key_ok(void)
{
    uint32_t item_sel = 0;

    if(app_movie_dvb_subt_cnt() > 0)
        return GXCORE_ERROR;

    GUI_GetProperty(BOX_MOVIE_SUBT, "select", (void*)&item_sel);
    switch(item_sel)
    {
        /*load*/
        case 1:{
                app_movie_subt_button_load();
                break;
                }
        default:
            break;
    }
    return GXCORE_SUCCESS;
}

static void app_set_movie_subt_select(int value_sel)
{
    PmpPsi *psi_info = NULL;
    psi_info = app_movie_psi_info_get();
    if(app_movie_dvb_subt_cnt() == 0)
    {
        pmp_subt_para* subt_para = NULL;
        subt_para = subtitle_get();
        if(subt_para->para.type == PLAYER_SUB_TYPE_FILE)
        {
            subt_para->cur_subt = 0;
            combobox_set_player_subt_select(subt_para->list, 0);
        }
        else
        {
            subt_para->cur_subt = value_sel;
            combobox_set_player_subt_select(subt_para->list, value_sel);
        }
    }
    else
    {
        if(psi_info->subt_info.num > 1)
            combobox_set_dvb_subt_select(value_sel);
    }

    return;
}

int app_subt_select_restore(void)
{
    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    if(subt_para->para.type == PLAYER_SUB_TYPE_FILE)
        return 0;

    app_set_movie_subt_select(s_subt_select);

    return 0;
}

int app_subt_select_init(void)
{
    if(app_movie_dvb_subt_cnt() == 0)
        s_subt_select = app_movie_dvb_subt_index();
    else
    {
        pmp_subt_para* subt_para = NULL;
        subt_para = subtitle_get();
        APP_CHECK_P(subt_para, 1);
        s_subt_select = subt_para->cur_subt;
    }

    return 0;
}

static status_t key_leftright(unsigned short value)
{
    uint32_t item_sel = 0;
    int32_t value_sel = 0;
    char string[15];
    pmp_subt_para* subt_para = NULL;
    int subtitle_times = 0;

    if(app_movie_dvb_subt_cnt() == 0)
    {
        subt_para = subtitle_get();
        APP_CHECK_P(subt_para, 1);
    }

    GUI_GetProperty(BOX_MOVIE_SUBT, "select", (void*)&item_sel);
    switch(item_sel)
    {
        //type
        case MOVIE_SUBT_TYPE:
            {
                if(app_movie_dvb_subt_cnt() > 0)
                    break;

                GUI_GetProperty(COMBOBOX_SUBT_TYPE, "select", &s_subt_type);
                if(s_subt_type == 1)//outside->inside
                {
                    app_log_info("###outside->inside\n");
                    subtitle_start(NULL, PMP_SUBT_LOAD_INSIDE);
                    GUI_SetProperty(BOXITEM_SUBT_SELECT, "enable", NULL);
                    app_set_movie_subt_select(s_subt_select);
                    combobox_player_subt_update(subt_para);
                    GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "disable");
                    GUI_SetProperty(BUTTON_SUBT_LOAD, "string", " ");
                    /*encoding select*/
               }
                else//inside->outside
                {
                    app_log_info("###inside->outside\n");
                    if(s_movie_subt_path != NULL)
                        subtitle_start(s_movie_subt_path, PMP_SUBT_LOAD_OUTSIDE);
                    else
                        subtitle_start(NULL, PMP_SUBT_LOAD_OUTSIDE);

                    GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "enable");
                    combobox_player_subt_update(subt_para);
                    GUI_SetProperty(BOXITEM_SUBT_SELECT, "disable", NULL);
                    if(subt_para->file != NULL)
                        GUI_SetProperty(BUTTON_SUBT_LOAD, "string", (void*)(subt_para->file));
                    else
                        GUI_SetProperty(BUTTON_SUBT_LOAD, "string", " ");

                    /*encoding select*/
                }
#if OSD_SUBTITLE_SUPPORT
                pmp_subt_load_type type = PMP_SUBT_LOAD_OUTSIDE;
                if(s_subt_type == 1)
                {
                    type = PMP_SUBT_LOAD_INSIDE;
                }
                value_sel = app_osd_subt_font_get();
                GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &value_sel);

                if(app_get_subt_real_type(type, subt_para->cur_subt) > 0)
                {
                    value_sel = app_osd_subt_encoding_get();
                    GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "enable");
                    GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "enable");
                }
                else
                {
                    value_sel = OSDSUBT_ENCODING_UTF8;
                    GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "disable");
                    GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
                }
                GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &value_sel);
                /*set language*/
                if(value_sel == OSDSUBT_ENCODING_UTF8)
                {
                    value_sel = 0;
                    GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
                }
                else
                {
                    GxBus_ConfigGetInt(MEDIA_SUBTILE_LANGUAGE, &value_sel, OSD_LANG);
                    GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
                }
                GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &value_sel);
#endif
            }
            break;

            /*visibility*/
        case MOVIE_SUBT_VIS:
            {
                GUI_GetProperty(COMBOBOX_SUBT_VISIBILITY, "select", &value_sel);
                if(app_movie_dvb_subt_cnt() > 0)
                    combobox_set_dvb_subt_visibility(value_sel);
                else
                    combobox_set_player_subt_visibility(subt_para->list, value_sel);
                break;
            }

            /*select*/
        case MOVIE_SUBT_SEL:
            {
                GUI_GetProperty(COMBOBOX_SUBT_SELECT, "select", (void*)&value_sel);
                app_set_movie_subt_select(value_sel);
                s_subt_select = value_sel;
                break;
            }

        case MOVIE_SUBT_DELAY:
            {
                if(app_movie_dvb_subt_cnt() > 0)
                    break;
                if(NULL == subt_para)
                    return GXCORE_ERROR;

                subtitle_times = (SUBTITLE_DELAY_TIME > 25) ? 25 : SUBTITLE_DELAY_TIME;
                if(APPK_LEFT==value)
                {
                    if(subt_para->delay_ms >= -((subtitle_times-1)*200) )
                        subt_para->delay_ms -= 200;
                }
                else
                {
                    if(subt_para->delay_ms < (subtitle_times*200) )
                        subt_para->delay_ms += 200;
                }
                memset(string,0,sizeof(string));
                sprintf(string, "%dms", subt_para->delay_ms);
                GUI_SetProperty(BUTTON_SUBT_DELAY, "string", (void*)string);
                combobox_set_player_subt_sync(subt_para->list, subt_para->delay_ms);
                break;
            }
#if OSD_SUBTITLE_SUPPORT
        case MOVIE_SUBT_LANGUAGE:
        {
            GUI_GetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &value_sel);
            GxBus_ConfigSetInt(MEDIA_SUBTILE_LANGUAGE, value_sel);
            app_moive_subt_lang_to_charset();
        }
        break;
        case MOVIE_SUBT_ENCODING:
        {
            GUI_GetProperty(COMBOBOX_SUBT_ENCODING, "select", &value_sel);
            app_osd_subt_encoding_set((OsdSubtEncoding)value_sel);
            if(value_sel == OSDSUBT_ENCODING_UTF8)
            {
                value_sel = 0;
                GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
            }
            else
            {
                GxBus_ConfigGetInt(MEDIA_SUBTILE_LANGUAGE, &value_sel, OSD_LANG);
                GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
                app_moive_subt_lang_to_charset();
            }
            GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &value_sel);
        }
        break;
        case MOVIE_SUBT_FONTSIZE:
        {
            GUI_GetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &value_sel);
            app_osd_subt_font_set((OsdSubtFont)value_sel);
        }
#endif
        break;
        default:
            break;
    }

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER  int movie_subt_service(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;
    GxMessage * msg;
    event = (GUI_Event *)usrdata;
    GxMsgProperty_PlayerStatusReport* player_status = NULL;

    msg = (GxMessage*)event->msg.service_msg;
    switch(msg->msg_id)
    {
        case GXMSG_PLAYER_STATUS_REPORT:
            player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
            if(PLAYER_STATUS_PLAY_END == player_status->status)
            {
                GUI_EndDialog(WIN_MOVIE_SUBT);
                GUI_SetInterface("flush",NULL);
            }
            break;
        default:
            break;
    }
    GUI_SendEvent(WIN_MOVIE_VIEW, event);

    return EVENT_TRANSFER_STOP;
}

static int movie_player_subt_init(void)
{
    int32_t i = 0,sel = 0;
    char combobox_buf[256];
    int32_t value = 0;
    char string[10] = {0};

    pmp_subt_para* subt_para = NULL;
    subt_para = subtitle_get();
    APP_CHECK_P(subt_para, 1);
    if(subt_para->para.type == PLAYER_SUB_TYPE_ARIB_CC)//not support now
    {
        GUI_EndDialog(WIN_MOVIE_SUBT);
        return -1;
    }

    GUI_SetProperty(BOXITEM_SUBT_TYPE, "enable", NULL);
    GUI_SetProperty(BOXITEM_SUBT_DELAY, "state", "enable");
    GUI_SetProperty("movie_subt_box", "select", &value);

    if(subt_para->para.type == PLAYER_SUB_TYPE_INSIDE)
    {

        s_subt_type = 1;
        GUI_SetProperty(BOXITEM_SUBT_SELECT, "enable", NULL);
        GUI_SetProperty(COMBOBOX_SUBT_TYPE, "select", &s_subt_type);
        GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "disable");
        GUI_SetProperty(BUTTON_SUBT_LOAD, "string", " ");
    }
    else
    {
        s_subt_type = 0;
        GUI_SetProperty(BOXITEM_SUBT_SELECT, "disable", NULL);
        GUI_SetProperty(COMBOBOX_SUBT_TYPE, "select", &s_subt_type);
        GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "enable");
        //load
        if(subt_para->file != NULL)
            GUI_SetProperty(BUTTON_SUBT_LOAD, "string", (void*)(subt_para->file));
        else
            GUI_SetProperty(BUTTON_SUBT_LOAD, "string", " ");
    }

    /*visibility*/
    value = pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY);
    GUI_SetProperty(COMBOBOX_SUBT_VISIBILITY, "select", (void*)&value);

    /*select*/
    combobox_init_player_subt_select(subt_para);

    /*subt delay*/
    //TODO: get subt_delay_ms from player
    memset(string,0,sizeof(string));
    sprintf(string, "%dms", subt_para->delay_ms);
    GUI_SetProperty(BUTTON_SUBT_DELAY, "string", (void*)string);
    /*subtile language*/
    memset(combobox_buf,0x0,256);
    strcpy(combobox_buf,  "[");

    for(i = 0;i < LANG_MAX;i++)
    {
        if(0 != i) strcat(combobox_buf, ",");
            strcat(combobox_buf, g_LangName[i]);
    }
    strcat(combobox_buf,  "]");
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", (void*)combobox_buf);

    app_log_debug("SUBT: combobox_buf %s\n", combobox_buf);
#if OSD_SUBTITLE_SUPPORT
    /*font size select */
    sel = app_osd_subt_font_get();
    GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &sel);

    if(app_get_subt_real_type(subt_para->para.type, subt_para->cur_subt) > 0)
    {
        sel = app_osd_subt_encoding_get();
        GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "enable");
        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "enable");
    }
    else
    {
        sel = OSDSUBT_ENCODING_UTF8;
        GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "disable");
        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
    }
    GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &sel);
    /*set language*/
    if(sel == OSDSUBT_ENCODING_UTF8)
    {
        sel = 0;
        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
    }
    else
    {
        GxBus_ConfigGetInt(MEDIA_SUBTILE_LANGUAGE, &sel, OSD_LANG);
        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
    }
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &sel);
#else
    sel= 0;
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &sel);
    GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &sel);
    GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &sel);
    GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
    GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "disable");
    GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
#endif
    return 0;
}

static int movie_dvb_subt_init(void)
{
    int32_t i = 0;
    char combobox_buf[256];
    int32_t value = 0;
    char string[10] = {0};
    int sel = 0;
    value = 1;
    GUI_SetProperty(COMBOBOX_SUBT_TYPE, "select", &value);
    //GUI_SetProperty(BOXITEM_SUBT_TYPE, "state", "disable");
    GUI_SetProperty(BUTTON_SUBT_LOAD, "string", " ");
    //GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "disable");

    /*visibility*/
    value = pmpset_get_int(PMPSET_MOVIE_SUBT_VISIBILITY);
    GUI_SetProperty(COMBOBOX_SUBT_VISIBILITY, "select", (void*)&value);

    /*select*/
    combobox_init_dvb_subt_select();

    /*subt delay*/
    memset(string,0,sizeof(string));
    sprintf(string, "%dms", 0);
    GUI_SetProperty(BUTTON_SUBT_DELAY, "string", (void*)string);
    /*subtile language*/
    memset(combobox_buf,0x0,256);
    strcpy(combobox_buf,  "[");

    for(i = 0;i < LANG_MAX;i++)
    {
        if(0 != i) strcat(combobox_buf, ",");
            strcat(combobox_buf, g_LangName[i]);
    }
    strcat(combobox_buf,  "]");
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "content", (void*)combobox_buf);
    /*encoding select*/
#if OSD_SUBTITLE_SUPPORT
    /*font size select */
    sel = app_osd_subt_font_get();
    GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &sel);

    if(app_get_subt_real_type(PLAYER_SUB_TYPE_INSIDE, app_movie_dvb_subt_index()) > 0)
    {
        sel = app_osd_subt_encoding_get();
        GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "enable");
        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "enable");
    }
    else
    {
        sel = OSDSUBT_ENCODING_UTF8;
        GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "disable");
        GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
    }
    GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &sel);
    /*set language*/
    if(sel == OSDSUBT_ENCODING_UTF8)
    {
        sel = 0;
        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
    }
    else
    {
        GxBus_ConfigGetInt(MEDIA_SUBTILE_LANGUAGE, &sel, OSD_LANG);
        GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "enable");
    }
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &sel);
#else
    sel= 0;
    GUI_SetProperty(COMBOBOX_SUBT_ENCODING, "select", &sel);
    GUI_SetProperty(COMBOBOX_SUBT_LANGUAGE, "select", &sel);
    GUI_SetProperty(COMBOBOX_SUBT_FONTSIZE, "select", &sel);
    GUI_SetProperty(BOXITEM_SUBT_LAN, "state", "disable");
    GUI_SetProperty(BOXITEM_SUBT_ENCODING, "state", "disable");
    GUI_SetProperty(BOXITEM_SUBT_FONT, "state", "disable");
#endif
    GUI_SetProperty(BOXITEM_SUBT_TYPE, "disable", NULL);
    GUI_SetProperty(BOXITEM_SUBT_LOAD, "state", "disable");
    GUI_SetProperty(BOXITEM_SUBT_DELAY, "state", "disable");
    value = 2;
    GUI_SetProperty("movie_subt_box", "select", &value);
    return 0;
}

SIGNAL_HANDLER int movie_subt_init(const char* widgetname, void *usrdata)
{
    if(app_movie_dvb_subt_cnt() == 0)
        movie_player_subt_init();
    else
        movie_dvb_subt_init();
    GUI_SetProperty(MOVIE_SUBT_TITLE, "string", STR_ID_SUBTITLING);
    return 0;
}

SIGNAL_HANDLER int movie_subt_destroy(const char* widgetname, void *usrdata)
{
    return 0;
}

SIGNAL_HANDLER int movie_subt_keypress(const char* widgetname, void *usrdata)
{
    GUI_Event *event = NULL;

    if(NULL == usrdata) return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    switch(event->key.sym)
    {
        case VK_BOOK_TRIGGER:
            GUI_EndDialog(WIN_MOVIE_SUBT);
            GUI_SendEvent(GUI_GetFocusWindow(), event);
            return EVENT_TRANSFER_STOP;

        case APPK_BACK:
        case APPK_MENU:
            GUI_EndDialog(WIN_MOVIE_SUBT);
            return EVENT_TRANSFER_STOP;

        case APPK_LEFT:
            key_leftright(event->key.sym);
            break;

        case APPK_RIGHT:
            key_leftright(event->key.sym);
            break;

        case APPK_OK:
            key_ok();
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_KEEPON;
}

#endif
