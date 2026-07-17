#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_wnd_system_setting_opt.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_pop.h"
#include "app_config.h"
#include "app_epg.h"

#define LANG_NAME_MAX   (16)

enum TIME_AV_NAME
{
	ITEM_MENU_LANG = 0,
#if !OTT_SUPPORT
	ITEM_AUDIO_1ST,
	ITEM_AUDIO_2ND,
	ITEM_SUB_LANG,
	ITEM_TTX_LANG,
	ITEM_EPG_LANG,
#endif
	ITEM_LANG_TOTAL
};

typedef enum
{
	LANG_PRIORITY_EN,
	LANG_PRIORITY_CHS
}LangPriority;

typedef enum
{
	LANG_EPG_EN,
	LANG_EPG_CHS
}LangEpg;

typedef struct
{
	uint32_t     menu_lang;
#if !OTT_SUPPORT
	uint32_t audio_1;
	uint32_t audio_2;
	uint32_t sub_1;
	uint32_t sub_2;
	uint32_t ttx_lang;
	uint32_t epg_lang;
#endif
}SysLangPara;

static SystemSettingOpt s_lang_setting_opt;
static SystemSettingItem s_lang_item[ITEM_LANG_TOTAL];
static SysLangPara s_sys_lang_para;
static int s_lang_menu_flag = 0;
static char s_lang_array[LANG_MAX*LANG_NAME_MAX] = {0};
#if !OTT_SUPPORT
static char s_sub_lang_array[(LANG_MAX+1)*LANG_NAME_MAX] = {0};
static char s_epg_lang_array[(LANG_MAX+1)*LANG_NAME_MAX] = {0};
static char s_ttx_lang_array[(LANG_MAX+1)*LANG_NAME_MAX] = {0};
#if PARENTAL_LOCK_SUPPORT
static bool s_epg_lang_locked = false;
#endif
#endif

char *app_get_common_short_lang_str(uint32_t lang)
{
#define LANG_SHORT_ENG    "eng"
#define LANG_SHORT_CHI    "chi"
#define LANG_SHORT_VIE    "vie"
#define LANG_SHORT_ARA    "ara"
#define LANG_SHORT_RUS    "rus"
#define LANG_SHORT_FRA    "fra"
#define LANG_SHORT_POR    "por"
#define LANG_SHORT_TUR    "tur"
#define LANG_SHORT_SPA    "spa"
#define LANG_SHORT_ITA    "ita"
#define LANG_SHORT_POL    "pol"
#define LANG_SHORT_GER    "ger"
#define LANG_SHORT_FAS    "fas"
#define LANG_SHORT_CES    "ces"

    char *str = NULL;
    if(strcmp(g_LangName[lang], "English") == 0)
    {
        str = LANG_SHORT_ENG;
    }
    else if(strcmp(g_LangName[lang], "Chinese") == 0)
    {
        str = LANG_SHORT_CHI;
    }
    else if(strcmp(g_LangName[lang], "Vietnamese") == 0)
    {
        str = LANG_SHORT_VIE;
    }
    else if(strcmp(g_LangName[lang], "Arabic") == 0)
    {
        str = LANG_SHORT_ARA;
    }
    else if(strcmp(g_LangName[lang], "Russian") == 0)
    {
        str = LANG_SHORT_RUS;
    }
    else if(strcmp(g_LangName[lang], "French") == 0)
    {
        str = LANG_SHORT_FRA;
    }
    else if(strcmp(g_LangName[lang], "Portuguese") == 0)
    {
        str = LANG_SHORT_POR;
    }
    else if(strcmp(g_LangName[lang], "Turkish") == 0)
    {
        str = LANG_SHORT_TUR;
    }
    else if(strcmp(g_LangName[lang], "Spain") == 0)
    {
        str = LANG_SHORT_SPA;
    }
    else if(strcmp(g_LangName[lang], "Italian") == 0)
    {
        str = LANG_SHORT_ITA;
    }
    else if(strcmp(g_LangName[lang], "Polski") == 0)
    {
        str = LANG_SHORT_POL;
    }
    else if(strcmp(g_LangName[lang], "German") == 0)
    {
        str = LANG_SHORT_GER;
    }
    else if(strcmp(g_LangName[lang], "Farsi") == 0)
    {
        str = LANG_SHORT_FAS;
    }
    else if(strcmp(g_LangName[lang], "Czech") == 0)
    {
        str = LANG_SHORT_CES;
    }
    else
    {
        str = "lang error";
    }

    return str;

}

char *app_get_epg_short_lang_str(uint32_t lang)
{
#define LANG_SHORT_ALL    "all"
    char *str=NULL;

    if(lang==0)
    {
       str=LANG_SHORT_ALL;
    }
    else
    {
        str=app_get_common_short_lang_str(lang-1);
    }
    return str;
}

static char *app_get_audio_lang_str(uint32_t lang)
{
    char *str=NULL;
#if AUDIO_LANG_QAA2OST_SUPPORT
#define AUDIO_LANG_OST    "qaa"

    if(lang == LANG_MAX)
    {
       str = AUDIO_LANG_OST;
    }
    else
    {
        str=app_get_common_short_lang_str(lang);
    }
#else
    str=app_get_common_short_lang_str(lang);
#endif

    return str;
}

#if 0
char *app_get_ttx_short_lang_str(uint32_t lang)
{
#define LANG_SHORT_AUTO    "auto"
    char *str=NULL;

    if(lang==0)
    {
       str=LANG_SHORT_AUTO;
    }
    else
    {
        str=app_get_common_short_lang_str(lang-1);
    }

    return str;
}
#endif
char *app_get_short_lang_str(uint32_t lang)
{
    char *str=NULL;
    str=app_get_common_short_lang_str(lang);

    return str;
}
///should free return value outside
char *app_sel_to_priority_lang_str(uint32_t lang_1, uint32_t lang_2)
{
	char *str1 = NULL;
	char *str2 = NULL;
	int len1 = 0;
	int len2 = 0;
	char *result = NULL;

	str1 = app_get_audio_lang_str(lang_1);
	len1= strlen(str1);

	str2 = app_get_audio_lang_str(lang_2);
	len2= strlen(str2);

	result = (char *)GxCore_Malloc(len1 + len2 + 2);
	if(result != NULL)
	{
		memset(result, 0, len1 + len2 + 2);
		memcpy(result, str1, len1);
		result[len1] = '_';
		memcpy(result + len1 + 1 , str2, len2);
	}
	return result;
}

static void app_lang_system_para_get(void)
{
    int init_value = 0;

    GxBus_ConfigGetInt(OSD_LANG_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.menu_lang= init_value;
#if !OTT_SUPPORT
    GxBus_ConfigGetInt(AUDIO_1ST_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.audio_1= init_value;

    GxBus_ConfigGetInt(AUDIO_2ND_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.audio_2= init_value;
    GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &init_value, OSD_LANG);
    s_sys_lang_para.sub_1 = init_value;

    GxBus_ConfigGetInt(TTX_LANG_KEY, &init_value, TTX_LANG);
    s_sys_lang_para.ttx_lang= init_value;

    GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
    s_sys_lang_para.epg_lang= init_value;
#endif
}

static void app_lang_result_para_get(SysLangPara *ret_para)
{
    ret_para->menu_lang = s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.sel;
#if !OTT_SUPPORT
    ret_para->audio_1 = s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel;
    ret_para->audio_2 = s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel;
    ret_para->sub_1 = s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel;
    ret_para->ttx_lang = s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel;
    ret_para->epg_lang = s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel;
#endif
}

static bool app_lang_setting_para_change(SysLangPara *ret_para)
{
	bool ret = FALSE;

	if(s_sys_lang_para.menu_lang != ret_para->menu_lang)
	{
		ret = TRUE;
	}
#if !OTT_SUPPORT
	if(s_sys_lang_para.audio_1 != ret_para->audio_1)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.audio_2 != ret_para->audio_2)
	{
		ret = TRUE;
	}
	if(s_sys_lang_para.sub_1 != ret_para->sub_1)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.ttx_lang != ret_para->ttx_lang)
	{
		ret = TRUE;
	}

	if(s_sys_lang_para.epg_lang!= ret_para->epg_lang)
	{
		ret = TRUE;
	}
#endif
	return ret;
}

static int app_lang_setting_para_save(SysLangPara *ret_para)
{
    int init_value = 0;

	if(s_sys_lang_para.menu_lang != ret_para->menu_lang)
    {
        init_value = ret_para->menu_lang;
        GxBus_ConfigSetInt(OSD_LANG_KEY,init_value);

        app_fuse_spec_build_by_language_change(init_value);

    }
#if !OTT_SUPPORT
    char * priority_lang = NULL;
	if(s_sys_lang_para.audio_1 != ret_para->audio_1)
    {
        init_value = ret_para->audio_1;
        GxBus_ConfigSetInt(AUDIO_1ST_KEY,init_value);
    }

	if(s_sys_lang_para.audio_2 != ret_para->audio_2)
    {
        init_value = ret_para->audio_2;
        GxBus_ConfigSetInt(AUDIO_2ND_KEY,init_value);
    }

    if((s_sys_lang_para.audio_1 != ret_para->audio_1) || (s_sys_lang_para.audio_2 != ret_para->audio_2))
    {
        priority_lang = app_sel_to_priority_lang_str(ret_para->audio_1, ret_para->audio_2);
        if(priority_lang != NULL)
        {
            GxBus_ConfigSet(PRIORITY_LANG_KEY, priority_lang);
            GxCore_Free(priority_lang);
            priority_lang = NULL;
        }
    }
	if(s_sys_lang_para.sub_1 != ret_para->sub_1)
    {
        init_value = ret_para->sub_1;
        GxBus_ConfigSetInt(SUBTILE_1ST_KEY,init_value);
    }

	if(s_sys_lang_para.ttx_lang != ret_para->ttx_lang)
    {
        init_value = ret_para->ttx_lang;
        GxBus_ConfigSetInt(TTX_LANG_KEY,init_value);
    }

	if(s_sys_lang_para.epg_lang!= ret_para->epg_lang)
    {
        app_epg_disable();
        init_value = ret_para->epg_lang;
        GxBus_ConfigSetInt(EPG_LANG_KEY,init_value);
    }
#endif
    return 0;
}

static int _lang_setting_save_pop_cb(PopDlgRet ret)
{
    SysLangPara para_ret;

    if(POP_VAL_OK == ret)
    {
        app_lang_result_para_get(&para_ret);
        app_lang_setting_para_save(&para_ret);
    }
    else
    {
        //app_log_debug("language : %s\n",g_LangName[s_sys_lang_para.menu_lang]);
        GUI_SetInterface("osd_language",g_LangName[s_sys_lang_para.menu_lang]);
#if (UI_MIRROR_SUPPORT > 0)
        app_menu_mirror_set(g_LangName[s_sys_lang_para.menu_lang]);
#endif
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
    s_lang_menu_flag = 0;

    return 0;
}

static bool _lang_setting_check_cb(void)
{
    bool ret = false;
    SysLangPara para_ret;

    app_lang_result_para_get(&para_ret);
    ret = app_lang_setting_para_change(&para_ret);

    return ret;
}

static int app_lang_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(EXIT_ABANDON == exit_type)
    {
        GUI_SetInterface("osd_language",g_LangName[s_sys_lang_para.menu_lang]);
#if (UI_MIRROR_SUPPORT > 0)
        app_menu_mirror_set(g_LangName[s_sys_lang_para.menu_lang]);
#endif
        s_lang_menu_flag = 0;
    }
    else
    {
        if(false == app_system_set_callback_handler(_lang_setting_check_cb, _lang_setting_save_pop_cb))
        {
            s_lang_menu_flag = 0;
        }
        ret = EXIT_RET_POP;
    }

    return ret;
}

static int app_menu_lang_change_callback(int sel)
{
	//app_log_debug("language : %s\n",g_LangName[sel]);
	GUI_SetInterface("osd_language",g_LangName[sel]);
    #if (UI_MIRROR_SUPPORT > 0)
        app_menu_mirror_set(g_LangName[sel]);
#endif
	app_system_set_wnd_update();

	return EVENT_TRANSFER_KEEPON;
}

static int32_t _app_lang_menu_lang_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_MENU_LANG), "select", &ret_sel);
    return 0;
}

static int app_lang_menu_lang_pop(int sel)
{
	PopList pop_list;
	int ret =-1;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = STR_ID_MENU_LANG;
	pop_list.item_num = LANG_MAX;
	pop_list.item_content = g_LangName;
	pop_list.sel = sel;
	pop_list.show_num= false;
	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _app_lang_menu_lang_poplist_cb;

	ret = poplist_create(&pop_list);

	return ret;
}

static int app_menu_lang_cmb_press_callback(int key)
{
	int cmb_sel =-1;

	if(key == STBK_OK)
	{
		GUI_GetProperty(app_system_get_combobox(ITEM_MENU_LANG), "select", &cmb_sel);
		cmb_sel = app_lang_menu_lang_pop(cmb_sel);
	}
	return EVENT_TRANSFER_KEEPON;
}

#if !OTT_SUPPORT
static int32_t _app_lang_audio_1_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_AUDIO_1ST), "select", &ret_sel);
    return 0;
}

static int app_lang_setting_audio_1_cmb_press_callback(int key)
{
	PopList pop_list;

	app_system_set_item_data_update(ITEM_AUDIO_1ST);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_AUDIO_LANG_1;
		pop_list.item_num = LANG_MAX;
		pop_list.item_content = g_LangName;
		pop_list.sel = s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.audio_1;
		pop_list.show_num= false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_lang_audio_1_poplist_cb;

		poplist_create(&pop_list);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int32_t _app_lang_audio_2_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_AUDIO_2ND), "select", &ret_sel);
    return 0;
}

static int app_lang_setting_audio_2_cmb_press_callback(int key)
{
	PopList pop_list;

	app_system_set_item_data_update(ITEM_AUDIO_2ND);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_AUDIO_LANG_2;
		pop_list.item_num = LANG_MAX;
		pop_list.item_content = g_LangName;
		pop_list.sel = s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.audio_2;
		pop_list.show_num= false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_lang_audio_2_poplist_cb;

		poplist_create(&pop_list);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int32_t _app_lang_sub_lang_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_SUB_LANG), "select", &ret_sel);
    return 0;
}

static int app_lang_setting_sub_lang_cmb_press_callback(int key)
{
	PopList pop_list;
	static char * s_sub_lang[LANG_MAX+1]={};
	int i = 0;

	app_system_set_item_data_update(ITEM_SUB_LANG);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_SUB_LANG;
		pop_list.item_num = LANG_MAX + 1;
        for(i = 0; i < LANG_MAX; i++)
        {
            s_sub_lang[i+1] = g_LangName[i];
        }
        s_sub_lang[0] = STR_ID_OFF;
		pop_list.item_content = s_sub_lang;
		pop_list.sel = s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.sub_1;
		pop_list.show_num= false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_lang_sub_lang_poplist_cb;
		poplist_create(&pop_list);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int32_t _app_lang_ttx_lang_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_TTX_LANG), "select", &ret_sel);
    return 0;
}

static int app_lang_setting_ttx_cmb_press_callback(int key)
{
	PopList pop_list;
	static char * s_ttx_lang[LANG_MAX+1]={};
	int i = 0;

	app_system_set_item_data_update(ITEM_TTX_LANG);
	if(key == STBK_OK)
	{
		memset(&pop_list, 0, sizeof(PopList));
		pop_list.title = STR_ID_TTX_LANG;
		pop_list.item_num = LANG_MAX+1;
        for(i = 0; i < LANG_MAX; i++)
		{
			s_ttx_lang[i+1] = g_LangName[i];
		}
		s_ttx_lang[0] = STR_ID_AUTO;
		pop_list.item_content = s_ttx_lang;
		pop_list.sel = s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.ttx_lang;
		pop_list.show_num= false;
		pop_list.mode = POP_MODE_UNBLOCK;
		pop_list.exit_cb = _app_lang_ttx_lang_poplist_cb;
		poplist_create(&pop_list);
	}

	return EVENT_TRANSFER_KEEPON;
}

static int32_t _app_lang_epg_lang_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(ITEM_EPG_LANG), "select", &ret_sel);
    return 0;
}

static int app_lang_setting_epg_cmb_press_callback(int key)
{
    PopList pop_list;
    static char * s_epg_lang[LANG_MAX+1]={};
    int i = 0;
    app_system_set_item_data_update(ITEM_EPG_LANG);
    if(key == STBK_OK)
    {
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = STR_ID_EPG_LANG;
        pop_list.item_num = LANG_MAX +1;
        for(i = 0; i < LANG_MAX; i++)
        {
            s_epg_lang[i+1] = g_LangName[i];
        }
        s_epg_lang[0] = STR_ID_ALL;
        pop_list.item_content = s_epg_lang;
        pop_list.sel = s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel;//s_sys_lang_para.epg_lang;
        pop_list.show_num= false;
        pop_list.mode = POP_MODE_UNBLOCK;
        pop_list.exit_cb = _app_lang_epg_lang_poplist_cb;
        poplist_create(&pop_list);
    }
    return EVENT_TRANSFER_KEEPON;
}

#if PARENTAL_LOCK_SUPPORT
static int _epg_lang_lock_change_passwd_dlg_cb(PopPwdRet *ret, void *usr_data)
{
    if(ret->result ==  PASSWD_ERROR)
	{
        PasswdDlg passwd_dlg;
		memset(&passwd_dlg, 0, sizeof(PasswdDlg));
		passwd_dlg.str = STR_ID_ERR_PASSWD;
		passwd_dlg.usr_data = usr_data;
		passwd_dlg.passwd_exit_cb = _epg_lang_lock_change_passwd_dlg_cb;
		passwd_dlg_create(&passwd_dlg);
	}
	else if(ret->result ==  PASSWD_CANCEL)
	{
	    *(int*)usr_data = -1;
    }
	else
	{
		s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel = *(int*)usr_data;
		app_system_set_item_property(ITEM_EPG_LANG, &s_lang_item[ITEM_EPG_LANG].itemProperty);
        s_epg_lang_locked = false;
        *(int*)usr_data = -1;
	}
	return 0;
}

static int app_lang_setting_epg_cmb_change_callback(int sel)
{
    static int sel_bak = -1;
    if((s_epg_lang_locked) && (sel != s_sys_lang_para.epg_lang) && (sel != sel_bak))
    {
        sel_bak = sel;
        s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.epg_lang;
        app_system_set_item_property(ITEM_EPG_LANG, &s_lang_item[ITEM_EPG_LANG].itemProperty);

        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = NULL;
        passwd_dlg.usr_data = &sel_bak;
        passwd_dlg.passwd_exit_cb = _epg_lang_lock_change_passwd_dlg_cb;
        passwd_dlg_create(&passwd_dlg);
    }
    return EVENT_TRANSFER_STOP;
}
#endif

static void app_lang_setting_audio_1_item_init(void)
{
	s_lang_item[ITEM_AUDIO_1ST].itemTitle = STR_ID_AUDIO_LANG_1;
	s_lang_item[ITEM_AUDIO_1ST].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.content= s_lang_array;
	s_lang_item[ITEM_AUDIO_1ST].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.audio_1;
	s_lang_item[ITEM_AUDIO_1ST].itemCallback.cmbCallback.CmbPress= app_lang_setting_audio_1_cmb_press_callback;
	s_lang_item[ITEM_AUDIO_1ST].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_AUDIO_1ST].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_audio_2_item_init(void)
{
	s_lang_item[ITEM_AUDIO_2ND].itemTitle = STR_ID_AUDIO_LANG_2;
	s_lang_item[ITEM_AUDIO_2ND].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.content= s_lang_array;
	s_lang_item[ITEM_AUDIO_2ND].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.audio_2;
	s_lang_item[ITEM_AUDIO_2ND].itemCallback.cmbCallback.CmbPress= app_lang_setting_audio_2_cmb_press_callback;
	s_lang_item[ITEM_AUDIO_2ND].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_AUDIO_2ND].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_sub_lang_item_init(void)
{
	s_lang_item[ITEM_SUB_LANG].itemTitle = STR_ID_SUB_LANG;
	s_lang_item[ITEM_SUB_LANG].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.content= s_sub_lang_array;
	s_lang_item[ITEM_SUB_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.sub_1;
	s_lang_item[ITEM_SUB_LANG].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_SUB_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_sub_lang_cmb_press_callback;
	s_lang_item[ITEM_SUB_LANG].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_ttx_lang_item_init(void)
{
	s_lang_item[ITEM_TTX_LANG].itemTitle = STR_ID_TTX_LANG;
	s_lang_item[ITEM_TTX_LANG].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.content= s_ttx_lang_array;
	s_lang_item[ITEM_TTX_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.ttx_lang;
	s_lang_item[ITEM_TTX_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_ttx_cmb_press_callback;
	s_lang_item[ITEM_TTX_LANG].itemCallback.cmbCallback.CmbChange= NULL;
	s_lang_item[ITEM_TTX_LANG].itemStatus = ITEM_NORMAL;
}

static void app_lang_setting_epg_lang_item_init(void)
{
	s_lang_item[ITEM_EPG_LANG].itemTitle = STR_ID_EPG_LANG;
	s_lang_item[ITEM_EPG_LANG].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.content= s_epg_lang_array;
	s_lang_item[ITEM_EPG_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.epg_lang;
	s_lang_item[ITEM_EPG_LANG].itemCallback.cmbCallback.CmbPress= app_lang_setting_epg_cmb_press_callback;
	s_lang_item[ITEM_EPG_LANG].itemStatus = ITEM_NORMAL;
#if PARENTAL_LOCK_SUPPORT
	s_lang_item[ITEM_EPG_LANG].itemCallback.cmbCallback.CmbChange=app_lang_setting_epg_cmb_change_callback;
    int32_t parental_guidace = PARENTAL_GUIDACE_OFF;
    GxBus_ConfigGetInt(PARENTAL_GUIDANCE_KEY, &parental_guidace, PARENTAL_GUIDANCE_VALUE);
    s_epg_lang_locked = (parental_guidace == PARENTAL_GUIDACE_OFF ? false: true);
#endif
}
#endif

static void app_lang_setting_menu_lang_item_init(void)
{
	s_lang_item[ITEM_MENU_LANG].itemTitle = STR_ID_MENU_LANG;
	s_lang_item[ITEM_MENU_LANG].itemType = ITEM_POP_CHOICE;
	s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.content= s_lang_array;//"[English,Chinese]";
	s_lang_item[ITEM_MENU_LANG].itemProperty.itemPropertyCmb.sel = s_sys_lang_para.menu_lang;
	s_lang_item[ITEM_MENU_LANG].itemCallback.cmbCallback.CmbChange= app_menu_lang_change_callback;
	s_lang_item[ITEM_MENU_LANG].itemCallback.cmbCallback.CmbPress= app_menu_lang_cmb_press_callback;
	s_lang_item[ITEM_MENU_LANG].itemStatus = ITEM_NORMAL;
}

void app_lang_setting_menu_exec(void)
{
	uint32_t i;

	s_lang_menu_flag = 1;
	memset(&s_lang_setting_opt, 0 ,sizeof(s_lang_setting_opt));

	app_lang_system_para_get();

	s_lang_setting_opt.menuTitle = STR_ID_LANG;
	s_lang_setting_opt.itemNum = ITEM_LANG_TOTAL;
	s_lang_setting_opt.timeDisplay = TIP_HIDE;
	s_lang_setting_opt.item = s_lang_item;

	memset(s_lang_array, 0, LANG_MAX * LANG_NAME_MAX);
#if OTT_SUPPORT
	strcpy(s_lang_array, "[");
	for (i=0; i<LANG_MAX; i++)
	{
		strcat(s_lang_array, g_LangName[i]);
		strcat(s_lang_array, ",");
	}
	strcat(s_lang_array, "]");
	app_lang_setting_menu_lang_item_init();
#else
    memset(s_sub_lang_array,0,(LANG_MAX + 1) * LANG_NAME_MAX);
	memset(s_epg_lang_array, 0, (LANG_MAX + 1) * LANG_NAME_MAX);
	memset(s_ttx_lang_array, 0, (LANG_MAX + 1) * LANG_NAME_MAX);
	strcpy(s_lang_array, "[");
    strcpy(s_sub_lang_array,"[");
    strcat(s_sub_lang_array, STR_ID_OFF);
	strcpy(s_epg_lang_array, "[");
	strcat(s_epg_lang_array, STR_ID_ALL);
	strcpy(s_ttx_lang_array, "[");
    strcat(s_ttx_lang_array, STR_ID_AUTO);
	for (i=0; i<LANG_MAX; i++)
	{
		strcat(s_lang_array, g_LangName[i]);
		strcat(s_lang_array, ",");

        strcat(s_sub_lang_array, ",");
        strcat(s_sub_lang_array, g_LangName[i]);

		strcat(s_epg_lang_array, ",");
		strcat(s_epg_lang_array, g_LangName[i]);

		strcat(s_ttx_lang_array, ",");
        strcat(s_ttx_lang_array, g_LangName[i]);
	}
	strcat(s_lang_array, "]");
    strcat(s_sub_lang_array, "]");
	strcat(s_epg_lang_array, "]");
	strcat(s_ttx_lang_array, "]");

	app_lang_setting_menu_lang_item_init();
	app_lang_setting_audio_1_item_init();
	app_lang_setting_audio_2_item_init();
	app_lang_setting_sub_lang_item_init();
	app_lang_setting_ttx_lang_item_init();
	app_lang_setting_epg_lang_item_init();
#endif
	s_lang_setting_opt.exit = app_lang_setting_exit_callback;

	app_system_set_create(&s_lang_setting_opt);
}

extern int app_get_muli_audio_pid(int index,int *count);
extern char *app_pmt_get_audio_lang_str(uint32_t index);
extern int app_pmt_get_muli_audio_type(int index);
extern int app_pmt_check_service_id(unsigned short service_id);
extern int app_pmt_check_apid(unsigned short apid);
extern int app_pmt_check_vpid(unsigned short vpid);

static int _lang_check_current_pmtinfo_valid(GxMsgProperty_NodeByPosGet* node)
{
    unsigned short current_vpid = node->prog_data.video_pid;
    unsigned short current_apid = node->prog_data.cur_audio_pid;

    if (0 != app_pmt_check_service_id(node->prog_data.service_id)) {
        return -1;
    }

    if(GXBUS_PM_PROG_TV == node->prog_data.service_type) {
        if(0 == app_pmt_check_apid(current_apid) && 0 == app_pmt_check_vpid(current_vpid))
            return 0;
    }
    else if(GXBUS_PM_PROG_RADIO == node->prog_data.service_type) {
        if(0 == app_pmt_check_apid(current_apid))
            return 0;
    }

    return -1;
}

int app_lang_check_audio_lang(uint16_t* audio_pid, GxBusPmDataProgAudioType* type, GxMsgProperty_NodeByPosGet* node)
{
    uint32_t i = 0;
    uint16_t select_auio_pid = 0;
    char lang_buff[60];
    char lang[3][4];
    int8_t flag = -1;//-1代表还没有赋值，0代表没有找到优先语言，1代表找到了最优语言，2代表着找到了第二优语言
    char* p = NULL;
    char* ptr = NULL;
    uint8_t count = 0;
    int audio_count = 0;
    char audiolang[4] = {0};
    int second_lang_total = 0;
    int second_lang_select = 0;

    if(0 != _lang_check_current_pmtinfo_valid(node)) {
        return -1;
    }

    if(NULL ==  GxBus_ConfigGet(PRIORITY_LANG_KEY,lang_buff,sizeof(lang_buff),PRIORITY_LANG_VALUE))
    {
       strcpy(lang_buff,PRIORITY_LANG_VALUE);
    }

    memset(lang,0,3*4);
    p = strtok_r(lang_buff, "_",&ptr);
    while(p)
    {
        if(count == 3)
            break;
        memcpy(lang[count],p,3);
        count++;
        p = strtok_r(NULL, "_",&ptr);
    }
	app_get_muli_audio_pid(0,&audio_count);
	select_auio_pid = node->prog_data.cur_audio_pid;
	*audio_pid = node->prog_data.cur_audio_pid;
	*type = node->prog_data.cur_audio_type;

	for(i=0; i<audio_count; i++)
	{
		if(app_pmt_get_muli_audio_type(i)<= GXBUS_PM_AUDIO_DTS_HD_SEC)
		{
            if(-1 == flag)
            {
                *audio_pid = app_get_muli_audio_pid(0,&audio_count);
                *type = app_pmt_get_muli_audio_type(0);
                flag = 0;
            }
			memset(audiolang,0,4);
			memcpy(audiolang,app_pmt_get_audio_lang_str(i),3);
            if(0 == app_language_code_cmp(audiolang, (char *)lang[0]))
            {
                *audio_pid = app_get_muli_audio_pid(i,&audio_count);
                *type = app_pmt_get_muli_audio_type(i);
                flag = 1;
                break;
            }
            else if(0 == app_language_code_cmp(audiolang, (char *)lang[1]))
            {
                *audio_pid = app_get_muli_audio_pid(i,&audio_count);
                *type = app_pmt_get_muli_audio_type(i);
                flag = 2;
                second_lang_total++;
                if(second_lang_total == 1)
                {
                    second_lang_select = i;
                }
            }
        }
    }

    if(flag == 2 && second_lang_total !=1)//节目音轨中有多个第二个语言
    {
        *audio_pid = app_get_muli_audio_pid(second_lang_select,&audio_count);
        *type = app_pmt_get_muli_audio_type(second_lang_select);
    }

	if((select_auio_pid != *audio_pid)&&(*audio_pid!=0))
	{
		GxMsgProperty_NodeModify node_modify = {0};
		node_modify.node_type = NODE_PROG;
		memcpy(&node_modify.prog_data, &node->prog_data, sizeof(GxBusPmDataProg));
		node_modify.prog_data.cur_audio_pid= *audio_pid;
		node_modify.prog_data.cur_audio_type= *type;


		if(memcmp(&node->prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
		{
			if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				app_log_error("\n-------modify program error\n");
			}
		}
	}

	return 0;
}

int app_set_first_audio_lang(void)
{
	GxBusPmDataProgAudioType type = 0xff;
    AudioCodecType audiotype = AUDIO_CODEC_UNKNOWN;
	uint16_t cur_audio_pid = 0;


	GxMsgProperty_NodeByPosGet prog_node = {0};
	prog_node.node_type = NODE_PROG;
	prog_node.pos = g_AppPlayOps.normal_play.play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
	{
		app_lang_check_audio_lang(&cur_audio_pid,&type,&prog_node);
		if(type == GXBUS_PM_AUDIO_AC3)
		{
			audiotype = AUDIO_CODEC_AC3;
		}
		else if(type == GXBUS_PM_AUDIO_EAC3)
		{
			audiotype = AUDIO_CODEC_EAC3;
		}
		else if(type == GXBUS_PM_AUDIO_AAC_LATM)
		{
			audiotype = AUDIO_CODEC_AAC_LATM;
		}
		else if(type == GXBUS_PM_AUDIO_MPEG1)
		{
			audiotype = AUDIO_CODEC_MPEG1;
		}
		else if(type == GXBUS_PM_AUDIO_MPEG2)
		{
			audiotype = AUDIO_CODEC_MPEG2;
		}
		else if(type == GXBUS_PM_AUDIO_DRA)
        {
			audiotype = AUDIO_CODEC_DRA1;
        }
		else if(GXBUS_PM_AUDIO_AAC_ADTS == type)
        {
            audiotype = AUDIO_CODEC_AAC_ADTS;
        }
        else if(GXBUS_PM_AUDIO_DRA == type)
        {
            audiotype = AUDIO_CODEC_DRA1;
        }
        else
        {;}

        GxPlayer_MediaAudioSwitch(PLAYER_FOR_NORMAL,cur_audio_pid,audiotype,NULL);
	}

	return 0;
}

#if WEBAPI_SUPPORT
int app_webapi_language_setting_busy(void)
{
	return s_lang_menu_flag;
}
#endif

