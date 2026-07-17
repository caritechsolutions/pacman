#include "app.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_module.h"
#include "app_epg.h"

#if NETAPPS_SUPPORT
#include <gx_apps.h>
#endif
#if HDMI_CEC_SUPPORT
#include "hdmi_cec/app_cec_config.h"
#endif

#define LANG_NAME_MAX   (16)
#define ALL_LANG_STR "All"
#if AUDIO_LANG_QAA2OST_SUPPORT
#define OST_LANG_STR "OST"
#endif
#define AUTO_LANG_STR "Auto"

#undef CHANGE_LANG_WITH_MENULANG

typedef enum
{
	DIGITAL_PCM,//pcm
	DIGITAL_BYPASS,
	DIGITAL_AUTO,
	DIGITAL_TOTAL
}Digital_audio;

char g_LangName_osd[LANG_MAX][LANG_NAME_MAX] = {{0}};
char g_LangName_aud[LANG_MAX + 1][LANG_NAME_MAX] = {{0}};
char g_LangName_epg[LANG_MAX + 1][LANG_NAME_MAX] = {{0}};
char g_LangName_ttx[LANG_MAX + 1][LANG_NAME_MAX] = {{0}};

extern char *app_sel_to_priority_lang_str(uint32_t lang_1, uint32_t lang_2);
extern void app_main_menu_listview_update(void);

static int app_t2_set_spdif_output(int audio)
{
    GxMsgProperty_PlayerAudioAC3Mode  mode = {0};//0: decord; 2.bypas 3.auto

    if(audio == DIGITAL_PCM)
        mode.mode = AUDIO_AC3_DECODE_MODE;
    else if(audio == DIGITAL_BYPASS)
        mode.mode = AUDIO_AC3_BYPASS_MODE;
    else if(audio == DIGITAL_AUTO)
        mode.mode = AUDIO_AC3_BYPASS_MODE | AUDIO_AC3_DECODE_MODE;
    else
        mode.mode = AUDIO_AC3_DECODE_MODE;

    GxBus_ConfigSetInt(TV_SPDIF_KEY, mode.mode);
    app_log_debug("app_t2_set_spdif_output  mode.mode= %d \n", mode.mode);

    app_send_msg_exec(GXMSG_PLAYER_AUDIO_AC3_MODE, (void *)&mode);
    return 0;
}

static void _main_menu_option_init(void)
{
    int i = 0;
    static bool option_init = false;

    if(true == option_init)
        return;

    memset(g_LangName_osd, 0x0, LANG_MAX * LANG_NAME_MAX);
    memset(g_LangName_epg, 0x0, (LANG_MAX + 1)*LANG_NAME_MAX);
    memset(g_LangName_aud, 0x0, (LANG_MAX + 1)*LANG_NAME_MAX);
    memset(g_LangName_ttx, 0x0, (LANG_MAX + 1)*LANG_NAME_MAX);

    for(i = 0; i < LANG_MAX; i++)
    {
        memset(g_LangName_osd[i], 0x0, LANG_NAME_MAX);
        sprintf(g_LangName_osd[i], "%s", g_LangName[i]);

        memset(g_LangName_epg[i + 1], 0x0, LANG_NAME_MAX);
        sprintf(g_LangName_epg[i + 1], "%s", g_LangName[i]);

        memset(g_LangName_aud[i], 0x0, LANG_NAME_MAX);
        sprintf(g_LangName_aud[i], "%s", g_LangName[i]);

        memset(g_LangName_ttx[i + 1], 0x0, LANG_NAME_MAX);
        sprintf(g_LangName_ttx[i + 1], "%s", g_LangName[i]);

    }

    memset(g_LangName_epg[0], 0x0, LANG_NAME_MAX);
    sprintf(g_LangName_epg[0], "%s", ALL_LANG_STR);

#if AUDIO_LANG_QAA2OST_SUPPORT
    memset(g_LangName_aud[i], 0x0, LANG_NAME_MAX);
    sprintf(g_LangName_aud[i], "%s", OST_LANG_STR);
#endif

    memset(g_LangName_ttx[0], 0x0, LANG_NAME_MAX);
    sprintf(g_LangName_ttx[0], "%s", AUTO_LANG_STR);

    option_init = true;
    return;
}

static int _setting_sel_move(bool prev, int cur, int end)
{
    if(true == prev)
    {
        if(0 == cur)
            cur = end - 1;
        else
            cur--;
    }
    else
    {
        if(cur == end - 1)
            cur = 0;
        else
            cur++;
    }

    return cur;
}

int app_osd_lang_setting_keypress(int key)
{
    int osd_lang = 0;
    int osd_lang_change = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(OSD_LANG_KEY, &osd_lang, OSD_LANG);
#if (UI_MIRROR_SUPPORT > 0)
    if( app_menu_mirror_get())
    {

        if (key == STBK_RIGHT)
        {
            key = STBK_LEFT;
        }
        else  if (key == STBK_LEFT)
        {
            key = STBK_RIGHT;
        }
    }
#endif

    if(STBK_LEFT == key)
        osd_lang_change = _setting_sel_move(true, osd_lang, LANG_MAX);
    else
        osd_lang_change = _setting_sel_move(false, osd_lang, LANG_MAX);

    GxBus_ConfigSetInt(OSD_LANG_KEY, osd_lang_change);
#ifdef CHANGE_LANG_WITH_MENULANG
    GxBus_ConfigSetInt(SUBTILE_1ST_KEY, osd_lang_change);
    GxBus_ConfigSetInt(TTX_LANG_KEY, osd_lang_change);
    GxBus_ConfigSetInt(AUDIO_1ST_KEY, osd_lang_change);
#endif
    GUI_SetInterface("osd_language", g_LangName_osd[osd_lang_change]);
#if (UI_MIRROR_SUPPORT > 0)
    app_menu_mirror_set(g_LangName[osd_lang_change]);
#endif

    app_main_menu_listview_update();
    GUI_SetProperty(WND_MAIN_MENU, "update", NULL);

    return EVENT_TRANSFER_KEEPON;
}

char *app_osd_lang_setting_content_get(void)
{
    int osd_lang = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(OSD_LANG_KEY, &osd_lang, OSD_LANG);
    return g_LangName_osd[osd_lang];
}

int app_epg_lang_setting_keypress(int key)
{
    int epg_lang = 0;
    int cur_epg_lang = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(EPG_LANG_KEY, &cur_epg_lang, EPG_LANG);

    if(STBK_LEFT == key)
        epg_lang = _setting_sel_move(true, cur_epg_lang, LANG_MAX + 1);
    else
        epg_lang = _setting_sel_move(false, cur_epg_lang, LANG_MAX + 1);

    GxBus_ConfigSetInt(EPG_LANG_KEY, epg_lang);
    app_epg_disable();
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_epg_lang_setting_content_get(void)
{
    int epg_lang = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(EPG_LANG_KEY, &epg_lang, EPG_LANG);
    return g_LangName_epg[epg_lang];
}

int app_subt_lang_setting_keypress(int key)
{
    int subt_lang = 0;
    int cur_subt_lang = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &cur_subt_lang, OSD_LANG);

    if(STBK_LEFT == key)
    {
        if(cur_subt_lang == 1)
        {
            subt_lang = 0;
        }
        else if(cur_subt_lang == 0)
        {
            subt_lang = _setting_sel_move(true, cur_subt_lang, LANG_MAX);
            subt_lang = subt_lang+1;
        }
        else
        {
            cur_subt_lang -= 1;
            subt_lang = _setting_sel_move(true, cur_subt_lang, LANG_MAX);
            subt_lang += 1;
        }
    }
    else
    {
        if(cur_subt_lang == 0)
        {
            subt_lang = 1;
        }
        else
        {
            cur_subt_lang -= 1;
            subt_lang = _setting_sel_move(false, cur_subt_lang, LANG_MAX);
            if(subt_lang != 0)
            {
                subt_lang += 1;
            }
        }
    }

    GxBus_ConfigSetInt(SUBTILE_1ST_KEY, subt_lang);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_subt_lang_setting_content_get(void)
{
    char *string = NULL;
        int subt_lang = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &subt_lang, OSD_LANG);
    if(subt_lang == 0)
    {
        string = STR_ID_OFF;
        return string;
    }
    return g_LangName_osd[subt_lang-1];
}



int app_ttx_lang_setting_keypress(int key)
{
    int ttx_lang = 0;
    int cur_ttx_lang = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(TTX_LANG_KEY, &cur_ttx_lang, TTX_LANG);

    if(STBK_LEFT == key)
        ttx_lang = _setting_sel_move(true, cur_ttx_lang, LANG_MAX + 1);
    else
        ttx_lang = _setting_sel_move(false, cur_ttx_lang, LANG_MAX + 1);

    GxBus_ConfigSetInt(TTX_LANG_KEY, ttx_lang);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_ttx_lang_setting_content_get(void)
{
    int ttx_lang = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(TTX_LANG_KEY, &ttx_lang, TTX_LANG);
    return g_LangName_ttx[ttx_lang];
}

int app_first_audio_lang_setting_keypress(int key)
{
    int audio_1st = 0;
    int audio_2nd = 0;
    int cur_audio_1st = 0;
    char *priority_lang = NULL;
    int end = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &cur_audio_1st, OSD_LANG);
    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &audio_2nd, OSD_LANG);
#if AUDIO_LANG_QAA2OST_SUPPORT
    end = LANG_MAX+1;
#else
    end = LANG_MAX;
#endif
    if(STBK_LEFT == key)
        audio_1st = _setting_sel_move(true, cur_audio_1st, end);
    else
        audio_1st = _setting_sel_move(false, cur_audio_1st, end);

    GxBus_ConfigSetInt(AUDIO_1ST_KEY, audio_1st);

    priority_lang = app_sel_to_priority_lang_str(audio_1st, audio_2nd);
    if(priority_lang != NULL)
    {
        GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
        GxCore_Free(priority_lang);
        priority_lang = NULL;
    }
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_first_audio_lang_setting_content_get(void)
{
    int audio_1st = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &audio_1st, OSD_LANG);
    return g_LangName_aud[audio_1st];
}

int app_second_audio_lang_setting_keypress(int key)
{
    int audio_1st = 0;
    int audio_2nd = 0;
    int cur_audio_2nd = 0;
    char *priority_lang = NULL;
    int end = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &audio_1st, OSD_LANG);
    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &cur_audio_2nd, OSD_LANG);

#if AUDIO_LANG_QAA2OST_SUPPORT
    end = LANG_MAX+1;
#else
    end = LANG_MAX;
#endif
    if(STBK_LEFT == key)
        audio_2nd = _setting_sel_move(true, cur_audio_2nd, end);
    else
        audio_2nd = _setting_sel_move(false, cur_audio_2nd, end);

    GxBus_ConfigSetInt(AUDIO_2ND_KEY, audio_2nd);

    priority_lang = app_sel_to_priority_lang_str(audio_1st, audio_2nd);
    if(priority_lang != NULL)
    {
        GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
        GxCore_Free(priority_lang);
        priority_lang = NULL;
    }
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_second_audio_lang_setting_content_get(void)
{
    int audio_2nd = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &audio_2nd, OSD_LANG);
    return g_LangName_aud[audio_2nd];
}

int app_digital_audio_setting_keypress(int key)
{
    int digital_audio = 0, value = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(TV_SPDIF_KEY, &value, AUDIO_AC3_BYPASS_MODE);

    if(AUDIO_AC3_DECODE_MODE == value)
        digital_audio = DIGITAL_PCM;
    else if(AUDIO_AC3_BYPASS_MODE == value)
        digital_audio = DIGITAL_BYPASS ;
    else if(AUDIO_AC3_FULL_MODE == value)
        digital_audio = DIGITAL_AUTO;

    if(STBK_LEFT == key)
        value = _setting_sel_move(true, digital_audio, DIGITAL_TOTAL);
    else
        value = _setting_sel_move(false, digital_audio, DIGITAL_TOTAL);

    app_t2_set_spdif_output(value);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_digital_audio_setting_content_get(void)
{
    int value = 0;
    char *string = NULL;

    _main_menu_option_init();
    GxBus_ConfigGetInt(TV_SPDIF_KEY, &value, AUDIO_AC3_BYPASS_MODE);

    if(AUDIO_AC3_DECODE_MODE == value)
        string = STR_ID_PCM;
    else if(AUDIO_AC3_BYPASS_MODE == value)
        string = STR_ID_AUDIO_BYPASS;
    else if (AUDIO_AC3_FULL_MODE == value)
        string = STR_ID_AUTO;

    return string;
}

#if NO_SIGNAL_AUTO_STANDBY_SUPPORT
int app_no_signal_auto_standby_setting_keypress(int key)
{
    int mode, value = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(NO_SIGNAL_AUTO_STANDBY_KEY, &value, NO_SIGNAL_AUTO_STANDBY_VALUE);

    mode = (++value )% 2;
    GxBus_ConfigSetInt(NO_SIGNAL_AUTO_STANDBY_KEY, mode);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_no_signal_auto_standby_content_get(void)
{
    int value = 0;
    char *string = NULL;

    _main_menu_option_init();
    GxBus_ConfigGetInt(NO_SIGNAL_AUTO_STANDBY_KEY, &value, NO_SIGNAL_AUTO_STANDBY_VALUE);
    if(0 == value)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif

#if AUDIO_DESCRIPTOR_SUPPORT
int app_audio_desc_setting_keypress(int key)
{
    int audio_desc = 0, value = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);

    value = (++audio_desc )% 2;
    GxBus_ConfigSetInt(AD_CONF_KEY, value);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_audio_desc_setting_content_get(void)
{
    char *string = NULL;
    int audio_desc = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);
    if(0 == audio_desc)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
static char* sg_AD_vol_offset[] = {"-3","-2","-1","0","1","2","3"};
int app_ad_vol_offset_setting_keypress(int key)
{
    int ad_vol_offset = 0;

    _main_menu_option_init();

    if (( STBK_LEFT != key )  && ( STBK_RIGHT != key ))
    {
        return EVENT_TRANSFER_KEEPON;
    }

    GxBus_ConfigGetInt(AD_VOL_OFFSET_KEY, &ad_vol_offset, AD_VOL_OFFSET_VALUE);

    if ( STBK_LEFT == key )
    {
        if(ad_vol_offset <= -3)
        {
            ad_vol_offset = 3;
        }
        else
        {
            ad_vol_offset --;
        }
    }
    else
    {
        if ( ad_vol_offset  >=  3 )
        {
            ad_vol_offset = -3;
        }
        else
        {
            ad_vol_offset ++;
        }
    }
    app_log_debug("app_ad_vol_offset_setting_keypress ad_vol_offset = %d \r\n",ad_vol_offset);
    GxBus_ConfigSetInt(AD_VOL_OFFSET_KEY, ad_vol_offset);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char * app_ad_vol_offset_setting_content_get(void)
{
    char *string = NULL;
    int ad_vol_offset = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(AD_VOL_OFFSET_KEY, &ad_vol_offset, AD_VOL_OFFSET_VALUE);
    if((ad_vol_offset > 3) ||(ad_vol_offset  < -3))
    {
        ad_vol_offset = 0;
    }
    string = sg_AD_vol_offset[ad_vol_offset +3];

    return string;
}
int app_ad_vol_offset_setting_check(void)
{
    int audio_desc = 0;
    GxBus_ConfigGetInt(AD_CONF_KEY, &audio_desc, AD_CONFIG);
    if(1 == audio_desc)
        return 1;

    return 0;
}

#endif
#endif

#if PVR_STANDBY_TIP_SUPPORT
int app_pvr_end_standby_setting_keypress(int key)
{
    int mode, value = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(PVR_END_STANDBY_KEY, &value, PVR_END_STANDBY_VALUE);

    mode = (++value )% 2;
    GxBus_ConfigSetInt(PVR_END_STANDBY_KEY, mode);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_pvr_end_standby_content_get(void)
{
    int value = 0;
    char *string = NULL;

    _main_menu_option_init();
    GxBus_ConfigGetInt(PVR_END_STANDBY_KEY, &value, PVR_END_STANDBY_VALUE);
    if(0 == value)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif

#if NO_SIGNAL_ALERT_TIP_SUPPORT
int app_no_signal_alert_tip_setting_keypress(int key)
{
    int mode = 0, value = 0;

    _main_menu_option_init();
    if(STBK_LEFT != key && STBK_RIGHT != key)
        return EVENT_TRANSFER_KEEPON;

    GxBus_ConfigGetInt(NO_SIGNAL_ALERT_TIP_KEY, &mode, NO_SIGNAL_ALERT_TIP_VALUE);

    value = (++mode )% 2;
    GxBus_ConfigSetInt(NO_SIGNAL_ALERT_TIP_KEY, value);
    app_main_menu_listview_update();

    return EVENT_TRANSFER_KEEPON;
}

char *app_no_signal_alert_tip_content_get(void)
{
    char *string = NULL;
    int mode = 0;

    _main_menu_option_init();
    GxBus_ConfigGetInt(NO_SIGNAL_ALERT_TIP_KEY, &mode, NO_SIGNAL_ALERT_TIP_VALUE);
    if(0 == mode)
        string = STR_ID_OFF;
    else
        string = STR_ID_ON;

    return string;
}
#endif

#if HDMI_CEC_SUPPORT
extern int app_cec_enable(int enable);
int app_cec_setting_keypress(int key)
{
    int value = 0;
	int cec_value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
    {
        return EVENT_TRANSFER_KEEPON;
    }
    GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_value, CEC_CONFIG_VALUE);
    value = (++cec_value) % 2;
    GxBus_ConfigSetInt(CEC_CONF_KEY, value);
    app_cec_enable(cec_value);
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}

char *app_cec_setting_content_get(void)
{
    char *string = NULL;
    int cec_value = 0;

    GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_value, CEC_CONFIG_VALUE);
    if(0 == cec_value)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}

int app_cec_use_tv_remote_control_keypress(int key)
{
    int value = 0;
	int cec_value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
    {
        return EVENT_TRANSFER_KEEPON;
    }
    GxBus_ConfigGetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, &cec_value, CEC_USE_TV_REMOTE_CONTROL_VALUE);
    value = (++cec_value) % 2;
    GxBus_ConfigSetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, value);
    app_cec_set_use_tv_remote_control_enable(cec_value);
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}

char *app_cec_use_tv_remote_control_content_get(void)
{
    char *string = NULL;
    int cec_value = 0;

    GxBus_ConfigGetInt(CEC_USE_TV_REMOTE_CONTROL_KEY, &cec_value, CEC_USE_TV_REMOTE_CONTROL_VALUE);
    if(0 == cec_value)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}

int app_cec_tv_on_stb_activity_keypress(int key)
{
    int value = 0;
	int cec_value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
    {
        return EVENT_TRANSFER_KEEPON;
    }
    GxBus_ConfigGetInt(CEC_TV_ON_STB_ACTIVITY_KEY, &cec_value, CEC_TV_ON_STB_ACTIVITY_VALUE);
    value = (++cec_value) % 2;
    GxBus_ConfigSetInt(CEC_TV_ON_STB_ACTIVITY_KEY, value);
	app_cec_set_tv_on_stb_activity_enable(cec_value);
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}

char *app_cec_tv_on_stb_activity_content_get(void)
{
    char *string = NULL;
    int cec_value = 0;

    GxBus_ConfigGetInt(CEC_TV_ON_STB_ACTIVITY_KEY, &cec_value, CEC_TV_ON_STB_ACTIVITY_VALUE);
    if(0 == cec_value)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}

int app_cec_stb_standby_tv_ativity_keypress(int key)
{
    int value = 0;
	int cec_value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
    {
        return EVENT_TRANSFER_KEEPON;
    }
    GxBus_ConfigGetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, &cec_value, CEC_STB_STANDBY_TV_ACTIVITY_VALUE);
    value = (++cec_value) % 2;
    GxBus_ConfigSetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, value);
	app_cec_set_stb_standby_tv_ativity_enable(cec_value);
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}
char *app_cec_stb_standby_tv_ativity_content_get(void)
{
    char *string = NULL;
    int cec_value = 0;

    GxBus_ConfigGetInt(CEC_STB_STANDBY_TV_ACTIVITY_KEY, &cec_value, CEC_STB_STANDBY_TV_ACTIVITY_VALUE);
    if(0 == cec_value)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}

int app_cec_system_audio_keypress(int key)
{
    int value = 0;
	int cec_value = 0;

    if(STBK_LEFT != key && STBK_RIGHT != key)
    {
        return EVENT_TRANSFER_KEEPON;
    }
    GxBus_ConfigGetInt(CEC_SYSTEM_AUDIO_KEY, &cec_value, CEC_SYSTEM_AUDIO_VALUE);
    value = (++cec_value) % 2;
    GxBus_ConfigSetInt(CEC_SYSTEM_AUDIO_KEY, value);
	app_cec_set_system_audio_mode_status(cec_value);
    app_main_menu_listview_update();
    return EVENT_TRANSFER_KEEPON;
}

char *app_cec_system_audio_content_get(void)
{
    char *string = NULL;
    int cec_value = 0;

    GxBus_ConfigGetInt(CEC_SYSTEM_AUDIO_KEY, &cec_value, CEC_SYSTEM_AUDIO_VALUE);
    if(0 == cec_value)
    {
        string = STR_ID_OFF;
    }
    else
    {
        string = STR_ID_ON;
    }
    return string;
}
#endif
