#include "app.h"
#include "app_module.h"
#include "module/config/gxconfig.h"
#include "app_wnd_system_setting_opt.h"
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"

#define STR_ID_ST_SET "Subtitle Translation"
#define STR_ID_BLUE_ACT "Activate"
#define STR_ID_BLUE_LOGOUT "Disconnect"
#define STR_ITEM_LANG "Translation Language"
#define STR_ITEM_FONTSIZE "Font Size"
#define STR_ITEM_FONTCOL "Font Color"
#define STR_ITEM_BAKGCOL "Backgroud Color"
#define STR_ITEM_CODE "Subscription Code"
#define STR_ITEM_DELAY "Network delay test"
#define STR_CODE_CHANGED_SAVE "Subscription Code has been changed, do you need to save it?"
#define STR_ST_REACTIVE "Whether to reactivate?"
#define STR_ID_SYSTEM_BUSY  "System Busy, Please Wait!"
#define STR_ID_ACTIVING "Activating..."
#define STR_ID_ST_ACTIVATE_FIRST "Please Activate First!"
#define STR_ID_ST_SER_ERR "Service init info error!"
#define STR_ID_ST_TESTING  "Translation time Testing ..."

#define STR_EXPIRE_DATA "Expiration Date"
#define STR_ACT_STATUS  "Subscription Status"
#define STR_ACT_STATUS_IVLD  "Invalid"
#define STR_ACT_STATUS_SUC  "Success"
#define STATUS_INFO_MAX     (64)

#define TEXT_SYS_SET_STATUS_SHOW "text_system_setting_timezone"
#define IMG_SYS_SET_STATUS_BAK "img_system_setting_timezone_back"
#define ST_CODE_STR_MAX (12)

#define ST_SETTING_DELAY_TEST

#ifdef AUTO_ACTIVE_SUBTTRANS
#endif

typedef enum {
    ST_ACT_STATUS_NOLOGIN = 0,
    ST_ACT_STATUS_LOGINING,
    ST_ACT_STATUS_LOGIN_SUC,
    ST_ACT_STATUS_LOGIN_ERR,
} STActiveStatus_e;

typedef enum {
    ITEM_ST_LANG = 0,
    ITEM_ST_FONT_SIZE,
    ITEM_ST_FONT_COL,
    ITEM_ST_BAKG_COL,
    ITEM_ST_CODE,
    ITEM_ST_DELAY,
    ITEM_ST_TOTAL
} ST_ITEM_NAME;

typedef enum {
    ST_FONT_N = 0,
    ST_FONT_S,
    ST_FONT_L,
} STFontSize_e;

typedef enum {
    ST_FONT_COL_Y,
    ST_FONT_COL_B,
    ST_FONT_COL_W
} STFontCol_e;

typedef struct {
    int lang;
    int font_size;
    int font_col;
    int bakg_col;
    int delay;
    int is_active; // 0:not active, 1:active&not enable, 2:actvive&enabletrans
    char code[ST_CODE_STR_MAX+1];
} SysSTPara;

static SystemSettingOpt s_st_setting_opt;
static SystemSettingItem s_st_item[ITEM_ST_TOTAL];
static SysSTPara s_sys_st_para;
static SysSTPara last_st_para;
static STActiveStatus_e act_status = ST_ACT_STATUS_NOLOGIN;
static char *s_expiration_date = NULL;
static int is_st_download = 0;
static int st_active_finish = 0;
static int s_item_sel = 0;
static char **s_st_langlist = NULL;
static char **s_st_fontsize = NULL;
static char **s_st_fontcolor = NULL;
static char **s_st_bgcolor  =NULL;
static int s_st_langlist_num = 0;
static int s_st_fontsize_num = 0;
static int s_st_fontcolor_num = 0;
static int s_st_bgcolor_num  = 0;

static int app_st_setting_bluekey(void);

static gxapps_ret_t app_st_setting_custom_info_init(void)
{
#define ST_LOGIN_STR_MAX (64)
    gxapps_ret_t ret = GXAPPS_SERVER_ERROR;
    char oem[ST_LOGIN_STR_MAX] = {0};
    char brand[ST_LOGIN_STR_MAX] = {0};
    char mac[ST_LOGIN_STR_MAX] = {0};
    char model[ST_LOGIN_STR_MAX] = {0};

    GxBus_ConfigGet(ST_LOGIN_OEM_KEY, oem, ST_LOGIN_STR_MAX-1, ST_LOGIN_DEF_OEM);
    GxBus_ConfigGet(ST_LOGIN_BRAND_KEY, brand, ST_LOGIN_STR_MAX-1, ST_LOGIN_DEF_BRAND);
    GxBus_ConfigGet(ST_LOGIN_MAC_KEY, mac, ST_LOGIN_STR_MAX-1, ST_LOGIN_DEF_MAC);
    GxBus_ConfigGet(ST_LOGIN_MOD_KEY, model, ST_LOGIN_STR_MAX-1, ST_LOGIN_DEF_MODEL);
    ret = Gx_SubtTransSeviceInit(oem, brand, mac, model);
    return ret ;
}

static void app_st_setting_status_set(char *show_str1, char *show_str2)
{
    if(show_str1 || show_str2)
    {
        char *temp = NULL;
        if(show_str2)
        {
            char show_str_len = strlen(show_str2);
            if(show_str1)
            {
                show_str_len += strlen(show_str1);
            }
            temp = GxCore_Calloc(show_str_len + 4, sizeof(char));
            if(temp)
            {
                if(show_str1)
                {
                    sprintf(temp, "%s\n%s", show_str1, show_str2);
                }
                else
                {
                    sprintf(temp, " \n%s", show_str2);
                }
            }
        }
        else
        {
            if(show_str1)
            {
                temp = GxCore_Strdup(show_str1);
            }
        }

        if(temp)
        {
            GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "string", temp);
            GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", "show");
            GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "show");
            GxCore_Free(temp);
            temp = NULL;
        }
    }
    else
    {
        GUI_SetProperty(TEXT_SYS_SET_STATUS_SHOW, "state", "hide");
        GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "hide");
    }
    if(APP_THEME_CLASSIC_PURPLE)
        GUI_SetProperty(IMG_SYS_SET_STATUS_BAK, "state", "hide");
    GUI_SetInterface("flush",NULL);
}

static void app_st_setting_login_status_show(void)
{
    gxapps_ret_t ret;
    char expirate_data[32] = { 0 };
    char expirate_data_info[STATUS_INFO_MAX] = { 0 };
    char act_status_str[STATUS_INFO_MAX] = { 0 };
    ret = Gx_SubtTransActivate_Get_Status(expirate_data, 32);
    if(ret == GXAPPS_SUCCESS)
    {
        snprintf(expirate_data_info, STATUS_INFO_MAX - 1, "%s: %s", STR_EXPIRE_DATA, expirate_data);
        snprintf(act_status_str, STATUS_INFO_MAX - 1, "%s: %s", STR_ACT_STATUS, STR_ACT_STATUS_SUC);
        s_st_setting_opt.menuFuncKey.blueKey.keyName = STR_ID_BLUE_LOGOUT;
        app_st_setting_status_set(expirate_data_info, act_status_str);
    }
    else if(ret == GXAPPS_SERVER_ERROR)
    {
        snprintf(act_status_str, STATUS_INFO_MAX - 1, "%s: %s", STR_ACT_STATUS, STR_ACT_STATUS_IVLD);
        s_st_setting_opt.menuFuncKey.blueKey.keyName = STR_ID_BLUE_ACT;
        app_st_setting_status_set(act_status_str, NULL);
    }
    else
    {
        s_st_setting_opt.menuFuncKey.blueKey.keyName = STR_ID_BLUE_ACT;
        app_st_setting_status_set(STR_ID_ST_ACTIVATE_FIRST, NULL);
    }
    GUI_SetProperty("text_system_setting_blue", "string", s_st_setting_opt.menuFuncKey.blueKey.keyName);
}

typedef void (*st_setting_thread_func)(void);
static void st_setting_thread_body(void *arg)
{
    st_setting_thread_func func = NULL;
    GxCore_ThreadDetach();
    func = (st_setting_thread_func)arg;
    if(func)
    {
        func();
    }
}

static void create_st_stting_thread(st_setting_thread_func func)
{
    static int thread_id = 0;
    GxCore_ThreadCreate("st_setting", &thread_id, st_setting_thread_body, (void *)func, 12*1024, GXOS_DEFAULT_PRIORITY);
}

static void app_st_setting_pop_tip_create(PopDlgType type, char *str, uint32_t timeout_sec, ExitCb cb)
{
    popdlg_destroy();
    PopDlg pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = type;
    pop.str = str;
    pop.mode = POP_MODE_UNBLOCK;
    pop.exit_cb = cb;
    pop.timeout_sec = timeout_sec;
    popdlg_create(&pop);
}

static void app_st_active_start_feed(void)
{
    gxapps_ret_t ret = GXAPPS_SUCCESS;
    char code[ST_CODE_STR_MAX+1] = { 0 };
    int handle = 0;

    is_st_download = 0;
    st_active_finish ++;
    handle = st_active_finish;
    GxBus_ConfigGet(ST_CODE_KEY, code, ST_CODE_STR_MAX, ST_CODE_DEFAULT);
    ret = Gx_SubtTransActivate(code, &s_expiration_date);
    if(handle == st_active_finish)
    {
        if(ret == GXAPPS_SUCCESS)
        {
            static int _st_init = 0;
            if(_st_init == 0)
            {
                Gx_SubtTransInit();
                _st_init = 1;
            }
            act_status = ST_ACT_STATUS_LOGIN_SUC;
        }
        else
        {
            act_status = ST_ACT_STATUS_LOGIN_ERR;
        }
    }
    else
    {
        if(ret == GXAPPS_SUCCESS){
            Gx_SubtTransSeviceDestory();
            act_status = ST_ACT_STATUS_NOLOGIN;
        }
    }
    app_st_setting_login_status_show();
}

static char *app_st_setting_get_arry_data_to_cmb(char **arry, int arry_num)
{
    char *cmb_array = NULL;
    int i = 0, arry_max = 4;

    if((!arry) || (arry_num <= 0))
        return NULL;

    for(i = 0; i < arry_num; i++)
    {
        if(arry[i])
            arry_max += (strlen(arry[i]) + 1);
    }

    if(NULL == (cmb_array = GxCore_Calloc(1, arry_max)))
        return NULL;

    memset(cmb_array, 0, arry_max);
    strcpy(cmb_array, "[");
    for(i = 0; i < arry_num; i++)
    {
        if(arry[i])
        {
            strcat(cmb_array, arry[i]);
        }
        if(i == (arry_num - 1))
        {
            strcat(cmb_array, "]");
        }
        else
        {
            strcat(cmb_array, ",");
        }
    }
    return cmb_array;
}

static void st_setting_all(SysSTPara *ret_para)
{
    Gx_SubtTrans_SubtShowSet(ret_para->font_size, ret_para->font_col, ret_para->bakg_col);
    if(s_sys_st_para.delay != ret_para->delay)
    {

    }
}

static void app_st_system_para_get(void)
{
    memset(&s_sys_st_para, 0, sizeof(s_sys_st_para));
    GxBus_ConfigGetInt(ST_LANG_SEL_KEY, &s_sys_st_para.lang, ST_LANG_SEL);
    GxBus_ConfigGetInt(ST_FONTSIZE_SEL_KEY, &s_sys_st_para.font_size, ST_FONTSIZE_SEL);
    GxBus_ConfigGetInt(ST_FONTCOLOR_SEL_KEY, &s_sys_st_para.font_col, ST_FONTCOLOR_SEL);
    GxBus_ConfigGetInt(ST_BAGKCOLOR_SEL_KEY, &s_sys_st_para.bakg_col, ST_BAGKCOLOR_SEL);
    GxBus_ConfigGetInt(ST_DELAY_KEY, &s_sys_st_para.delay, ST_DELAY_DEFAULT);
    GxBus_ConfigGet(ST_CODE_KEY, s_sys_st_para.code, ST_CODE_STR_MAX, ST_CODE_DEFAULT);
    int ret = Gx_SubtTransGetStatus();
    if(GXAPPS_SUCCESS == Gx_SubtTransActivate_Get_Status(NULL, 0)) {
        if(ret == 0)
            s_sys_st_para.is_active = 1;
        else
            s_sys_st_para.is_active = 2;
    }else {
        s_sys_st_para.is_active = 0;
    }
    s_st_langlist  = Gx_SubtTrans_SubtLangGet(&s_st_langlist_num);
    s_st_fontsize  = Gx_SubtTrans_SubtFontGet(&s_st_fontsize_num);
    s_st_fontcolor = Gx_SubtTrans_SubtFontColorGet(&s_st_fontcolor_num);
    s_st_bgcolor   = Gx_SubtTrans_SubtBackgroudColorGet(&s_st_bgcolor_num);
}

static int app_st_setting_para_save(SysSTPara *ret_para)
{
    GxBus_ConfigSetInt(ST_LANG_SEL_KEY, ret_para->lang);
    GxBus_ConfigSetInt(ST_FONTSIZE_SEL_KEY, ret_para->font_size);
    GxBus_ConfigSetInt(ST_FONTCOLOR_SEL_KEY, ret_para->font_col);
    GxBus_ConfigSetInt(ST_BAGKCOLOR_SEL_KEY, ret_para->bakg_col);
    GxBus_ConfigSet(ST_CODE_KEY, (const char *)ret_para->code);
    GxBus_ConfigSetInt(ST_DELAY_KEY, ret_para->delay);
    st_setting_all(ret_para);
    return 0;
}

static void app_st_result_para_get(SysSTPara *ret_para)
{
    ret_para->lang = s_st_item[ITEM_ST_LANG].itemProperty.itemPropertyCmb.sel;
    ret_para->font_size = s_st_item[ITEM_ST_FONT_SIZE].itemProperty.itemPropertyCmb.sel;
    ret_para->font_col = s_st_item[ITEM_ST_FONT_COL].itemProperty.itemPropertyCmb.sel;
    ret_para->bakg_col = s_st_item[ITEM_ST_BAKG_COL].itemProperty.itemPropertyCmb.sel;
    ret_para->delay = s_st_item[ITEM_ST_DELAY].itemProperty.itemPropertyCmb.sel;
    memset(ret_para->code, 0, ST_CODE_STR_MAX+1);
    strncpy(ret_para->code, (s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.string), ST_CODE_STR_MAX);
    if(GXAPPS_SUCCESS == Gx_SubtTransActivate_Get_Status(NULL, 0)) {
        ret_para->is_active = 1;
    }else {
        ret_para->is_active = 0;
    }
}

static void st_setting_destroy(void)
{
    APP_FREE(s_st_item[ITEM_ST_LANG].itemProperty.itemPropertyCmb.content);
    APP_FREE(s_st_item[ITEM_ST_FONT_SIZE].itemProperty.itemPropertyCmb.content);
    APP_FREE(s_st_item[ITEM_ST_FONT_COL].itemProperty.itemPropertyCmb.content);
    APP_FREE(s_st_item[ITEM_ST_BAKG_COL].itemProperty.itemPropertyCmb.content);
}

static int app_st_setting_para_change(SysSTPara *ret_para)
{
    int ret = 0;
    if(s_sys_st_para.lang != ret_para->lang)
        ret = 1;
    if(s_sys_st_para.font_size != ret_para->font_size)
        ret = 1;
    if(s_sys_st_para.font_col != ret_para->font_col)
        ret = 1;
    if(s_sys_st_para.bakg_col != ret_para->bakg_col)
        ret = 1;
    if(0 != strcmp(s_sys_st_para.code, ret_para->code))
        ret = ret | 1<<1;
    if(s_sys_st_para.delay != ret_para->delay)
        ret = ret | 1<<2;
    return ret;
}

static int _st_setting_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        app_st_setting_para_save(&last_st_para);
    }
    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING, "draw_now", NULL);

    int delay = 0, need_replay = 0, delay_time = 0;
    if(s_st_setting_opt.backGround == BG_PLAY_RPOG)
    {
        GxBus_ConfigGetInt(ST_DELAY_KEY, &delay, ST_DELAY_DEFAULT);
        if(last_st_para.is_active == 0)
        {
            if(s_sys_st_para.is_active == 2)
            {
                if (s_sys_st_para.delay != 0)
                {
                    need_replay = 1; delay_time = 0;
                }
                Gx_SubtTransStop();
            }
        }
        else
        {
            if((s_sys_st_para.is_active == 2) && (delay != s_sys_st_para.delay))
                need_replay = 1; delay_time = delay;
        }
    }
    if(need_replay == 1)
    {
        if(GXCORE_SUCCESS == Gx_SubtTrans_Replay_ByTsbuff())
        {
            PopDlg pop = {0};
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = STR_ID_ST_WAIT_START;
            pop.timeout_sec = delay_time;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            popdlg_create(&pop);
        }
    }
    return 0;
}

static bool _st_setting_check_cb(void)
{
    int ret = 0;
    ret = app_st_setting_para_change(&last_st_para);
    if(ret > 0)
        return 1;
    return 0;
}

static int app_st_setting_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;
    if(EXIT_ABANDON != exit_type)
    {
        app_st_result_para_get(&last_st_para);
        app_system_set_callback_handler(_st_setting_check_cb, _st_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }
    st_active_finish ++;
    st_setting_destroy();
    if((0 == _st_setting_check_cb()) && (s_st_setting_opt.backGround == BG_PLAY_RPOG)
            && (s_sys_st_para.is_active==2) && (last_st_para.is_active == 0)) {
        Gx_SubtTransStop();
        Gx_SubtTrans_Replay_ByTsbuff();
    }
    if(s_st_setting_opt.backGround == BG_PLAY_RPOG)
    {
        extern void app_subt_resume(void);
        app_subt_resume();
    }
    return ret;
}

static char *app_st_get_tran_lang_array_data(void)
{
    return  app_st_setting_get_arry_data_to_cmb(s_st_langlist, s_st_langlist_num);
}

static char *app_st_get_font_size_array_data(void)
{
    return  app_st_setting_get_arry_data_to_cmb(s_st_fontsize, s_st_fontsize_num);
}

static char *app_st_get_font_color_array_data(void)
{
    return  app_st_setting_get_arry_data_to_cmb(s_st_fontcolor, s_st_fontcolor_num);
}

static char *app_st_get_background_color_array_data(void)
{
    return  app_st_setting_get_arry_data_to_cmb(s_st_bgcolor, s_st_bgcolor_num);
}

static int _st_setting_cmb_poplist_cb(int32_t ret_sel, unsigned short key)
{
    GUI_SetProperty(app_system_get_combobox(s_item_sel), "select", &ret_sel);
    s_st_item[s_item_sel].itemProperty.itemPropertyCmb.sel = ret_sel;
    return 0;
}

static void _st_setting_cmb_press_callback(int key, char **item_content, int item_num)
{
    PopList pop_list;
    if((key==STBK_OK) && item_content && (item_num>0))
    {
        memset(&pop_list, 0, sizeof(PopList));
        pop_list.title = s_st_item[s_item_sel].itemTitle;
        pop_list.item_num = item_num;
        pop_list.item_content = item_content;
        pop_list.sel = s_st_item[s_item_sel].itemProperty.itemPropertyCmb.sel;
        pop_list.show_num= false;
        pop_list.mode = POP_MODE_UNBLOCK;
        pop_list.exit_cb = _st_setting_cmb_poplist_cb;
        poplist_create(&pop_list);
    }
}

static int app_st_setting_lang_cmb_press_callback(int key)
{
    s_item_sel = ITEM_ST_LANG;
    _st_setting_cmb_press_callback(key, s_st_langlist, s_st_langlist_num);
    return EVENT_TRANSFER_KEEPON;
}

static int app_st_setting_font_size_cmb_press_callback(int key)
{
    s_item_sel = ITEM_ST_FONT_SIZE;
    _st_setting_cmb_press_callback(key, s_st_fontsize, s_st_fontsize_num);
    return EVENT_TRANSFER_KEEPON;
}

static int app_st_setting_font_color_cmb_press_callback(int key)
{
    s_item_sel = ITEM_ST_FONT_COL;
    _st_setting_cmb_press_callback(key, s_st_fontcolor, s_st_fontcolor_num);
    return EVENT_TRANSFER_KEEPON;
}

static int app_st_setting_bg_color_cmb_press_callback(int key)
{
    s_item_sel = ITEM_ST_BAKG_COL;
    _st_setting_cmb_press_callback(key, s_st_bgcolor, s_st_bgcolor_num);
    return EVENT_TRANSFER_KEEPON;
}

static void _st_setting_full_keyboard_proc(PopKeyboard *data)
{
    if((data->in_ret == POP_VAL_CANCEL)
            || (data->out_name == NULL)
            || (strlen(data->out_name) <= 0))
    {
        return;
    }
    GUI_SetProperty(app_system_get_edit(ITEM_ST_CODE), "string", data->out_name);
    GUI_SetInterface("flush", NULL);
}

static int app_st_setting_code_edit_press_callback(unsigned short key)
{
    int ret = EVENT_TRANSFER_STOP;
    switch (key)
    {
        case STBK_OK:
            {
                PopKeyboard keyboard;
                memset(&keyboard, 0, sizeof(PopKeyboard));
                keyboard.max_num = ST_CODE_STR_MAX;
                keyboard.release_cb = _st_setting_full_keyboard_proc;
                keyboard.pos.x = 500;
                keyboard.pos.y = 60;
                multi_language_keyboard_create(&keyboard);
                return EVENT_TRANSFER_STOP;
            }
        case STBK_BLUE:
            {
                app_st_setting_bluekey();
                return EVENT_TRANSFER_STOP;
            }
        case STBK_LEFT:
        case STBK_RIGHT:
        case STBK_EXIT:
        case STBK_DOWN:
        case STBK_UP:
            {
                ret = EVENT_TRANSFER_KEEPON;
            }
        default:
            break;
    }
    return ret;
}

static void app_st_setting_tran_lang_item_init(void)
{
    s_st_item[ITEM_ST_LANG].itemTitle = STR_ITEM_LANG;
    s_st_item[ITEM_ST_LANG].itemType = ITEM_CHOICE;
    s_st_item[ITEM_ST_LANG].itemProperty.itemPropertyCmb.content = app_st_get_tran_lang_array_data();
    s_st_item[ITEM_ST_LANG].itemProperty.itemPropertyCmb.sel = s_sys_st_para.lang;
    s_st_item[ITEM_ST_LANG].itemCallback.cmbCallback.CmbChange = NULL;
    s_st_item[ITEM_ST_LANG].itemCallback.cmbCallback.CmbPress= app_st_setting_lang_cmb_press_callback;
    s_st_item[ITEM_ST_LANG].itemStatus = ITEM_NORMAL;
}

static void app_st_setting_font_size_item_init(void)
{
    s_st_item[ITEM_ST_FONT_SIZE].itemTitle = STR_ITEM_FONTSIZE;
    s_st_item[ITEM_ST_FONT_SIZE].itemType = ITEM_CHOICE;
    s_st_item[ITEM_ST_FONT_SIZE].itemProperty.itemPropertyCmb.content = app_st_get_font_size_array_data();
    s_st_item[ITEM_ST_FONT_SIZE].itemProperty.itemPropertyCmb.sel = s_sys_st_para.font_size;
    s_st_item[ITEM_ST_FONT_SIZE].itemCallback.cmbCallback.CmbChange = NULL;
    s_st_item[ITEM_ST_FONT_SIZE].itemCallback.cmbCallback.CmbPress= app_st_setting_font_size_cmb_press_callback;
    s_st_item[ITEM_ST_FONT_SIZE].itemStatus = ITEM_NORMAL;
}

static void app_st_setting_font_color_item_init(void)
{
    s_st_item[ITEM_ST_FONT_COL].itemTitle = STR_ITEM_FONTCOL;
    s_st_item[ITEM_ST_FONT_COL].itemType = ITEM_CHOICE;
    s_st_item[ITEM_ST_FONT_COL].itemProperty.itemPropertyCmb.content = app_st_get_font_color_array_data();
    s_st_item[ITEM_ST_FONT_COL].itemProperty.itemPropertyCmb.sel = s_sys_st_para.font_col;
    s_st_item[ITEM_ST_FONT_COL].itemCallback.cmbCallback.CmbChange = NULL;
    s_st_item[ITEM_ST_FONT_COL].itemCallback.cmbCallback.CmbPress= app_st_setting_font_color_cmb_press_callback;
    s_st_item[ITEM_ST_FONT_COL].itemStatus = ITEM_NORMAL;
}

static void app_st_setting_background_color_item_init(void)
{
    s_st_item[ITEM_ST_BAKG_COL].itemTitle = STR_ITEM_BAKGCOL;
    s_st_item[ITEM_ST_BAKG_COL].itemType = ITEM_CHOICE;
    s_st_item[ITEM_ST_BAKG_COL].itemProperty.itemPropertyCmb.content = app_st_get_background_color_array_data();
    s_st_item[ITEM_ST_BAKG_COL].itemProperty.itemPropertyCmb.sel = s_sys_st_para.bakg_col;
    s_st_item[ITEM_ST_BAKG_COL].itemCallback.cmbCallback.CmbChange = NULL;
    s_st_item[ITEM_ST_BAKG_COL].itemCallback.cmbCallback.CmbPress= app_st_setting_bg_color_cmb_press_callback;
    s_st_item[ITEM_ST_BAKG_COL].itemStatus = ITEM_NORMAL;
}

static void app_st_setting_code_item_init(void)
{
    s_st_item[ITEM_ST_CODE].itemTitle = STR_ITEM_CODE;
    s_st_item[ITEM_ST_CODE].itemType = ITEM_EDIT;
    s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.format = "edit_string";
    s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.maxlen = "32";
    s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.string = s_sys_st_para.code;
    s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.default_intaglio = NULL;
    s_st_item[ITEM_ST_CODE].itemCallback.editCallback.EditReachEnd = NULL;
    s_st_item[ITEM_ST_CODE].itemCallback.editCallback.EditPress = app_st_setting_code_edit_press_callback;
    s_st_item[ITEM_ST_CODE].itemStatus = ITEM_NORMAL;
}
static void app_st_setting_delay_init(void)
{
    s_st_item[ITEM_ST_DELAY].itemTitle = "Video Delay";
    s_st_item[ITEM_ST_DELAY].itemType = ITEM_CHOICE;
    s_st_item[ITEM_ST_DELAY].itemProperty.itemPropertyCmb.content = "[0s,1s,2s,3s,4s]";
    s_st_item[ITEM_ST_DELAY].itemProperty.itemPropertyCmb.sel = s_sys_st_para.delay;
    s_st_item[ITEM_ST_DELAY].itemCallback.cmbCallback.CmbChange = NULL;
    s_st_item[ITEM_ST_DELAY].itemCallback.cmbCallback.CmbPress= NULL;
    s_st_item[ITEM_ST_DELAY].itemStatus = ITEM_NORMAL;
}

static int app_st_setting_login(void)
{
    gxapps_ret_t ret = GXAPPS_SERVER_ERROR;

    ret = app_st_setting_custom_info_init();
    if(ret != GXAPPS_SUCCESS)
    {
        app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_ST_SER_ERR, 1, NULL);
        return -1;
    }
    app_st_setting_status_set(STR_ID_ACTIVING, NULL);
    act_status = ST_ACT_STATUS_LOGINING;
    create_st_stting_thread(app_st_active_start_feed);
    return 0;
}

static int _st_setting_code_reactive_pop_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        Gx_SubtTransSeviceDestory();
        app_st_setting_login();
    }
    return 0;
}

static int _st_setting_code_change_pop_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        memset(s_sys_st_para.code, 0, ST_CODE_STR_MAX + 1);
        strncpy(s_sys_st_para.code, s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.string, ST_CODE_STR_MAX);
        GxBus_ConfigSet(ST_CODE_KEY, (const char *)s_sys_st_para.code);
    }
    if(ST_ACT_STATUS_LOGIN_SUC == act_status)
    {
        app_st_setting_pop_tip_create(POP_TYPE_YES_NO, STR_ST_REACTIVE, 0, _st_setting_code_reactive_pop_cb);
    }
    else if(ST_ACT_STATUS_LOGINING == act_status)
    {
        app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_SYSTEM_BUSY, 1, NULL);
    }
    else
    {
        if(ST_ACT_STATUS_LOGIN_ERR == act_status)
        {
            Gx_SubtTransSeviceDestory();
        }
        app_st_setting_login();
    }
    return 0;
}

static int app_st_setting_bluekey(void)
{
    if(ST_ACT_STATUS_LOGINING == act_status)
    {
        app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_SYSTEM_BUSY, 1, NULL);
        return 0;
    }
    app_system_set_item_data_update(ITEM_ST_CODE);
    if(0 != strcmp(s_sys_st_para.code, s_st_item[ITEM_ST_CODE].itemProperty.itemPropertyEdit.string))
    {
        app_st_setting_pop_tip_create(POP_TYPE_YES_NO, STR_CODE_CHANGED_SAVE, 0, _st_setting_code_change_pop_cb);
    }
    else
    {
        if(ST_ACT_STATUS_LOGIN_SUC == act_status)
        {
            Gx_SubtTransSeviceDestory();
            act_status = ST_ACT_STATUS_NOLOGIN;
            app_st_setting_login_status_show();
        }
        else
        {
            if(ST_ACT_STATUS_LOGIN_ERR == act_status)
            {
                Gx_SubtTransSeviceDestory();
            }
            app_st_setting_login();
        }
    }
    return 0;
}
#ifdef ST_SETTING_DELAY_TEST
static int app_st_setting_redkey(void)
{
    gxapps_ret_t ret = GXAPPS_SUCCESS;
    int delay = 0;
    char delay_info[100] = {0};
    if(ST_ACT_STATUS_LOGIN_SUC == act_status)
    {
        GxTime  time_start, time_end;
        GxCore_GetTickTime(&time_start);
        app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_ST_TESTING, 10, NULL);
        GUI_SetInterface("flush",NULL);
        ret = Gx_SubtTransTest();
        if(ret == GXAPPS_SUCCESS)
        {
            GxCore_GetTickTime(&time_end);
            if(time_end.microsecs < time_start.microsecs)
                delay = time_end.seconds-1-time_start.seconds;
            else
                delay = time_end.seconds-time_start.seconds;
            //app_log_debug("Gx_SubtTransTest: %d us\n", delay);
            popdlg_destroy();
            sprintf(delay_info, "The recommended video delay time is %ds", delay);
            app_st_setting_pop_tip_create(POP_TYPE_OK, delay_info, 10, NULL);
        }
        else
        {
            app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_UNKNOWN_ERROR, 1, NULL);
        }
    }
    else
    {
        app_st_setting_pop_tip_create(POP_TYPE_NO_BTN, STR_ID_ST_ACT_ST_STR, 1, NULL);
    }
    return 0;
}
#endif
void app_subttrans_setting_menu_exec(void)
{
    memset(&s_st_setting_opt, 0, sizeof(s_st_setting_opt));
    memset(&s_st_item, 0, sizeof(s_st_item));
    app_st_system_para_get();

    s_st_setting_opt.menuTitle = STR_ID_ST_SET;
    s_st_setting_opt.itemNum = ITEM_ST_TOTAL;
    s_st_setting_opt.timeDisplay = TIP_HIDE;
    s_st_setting_opt.item = s_st_item;
    s_st_setting_opt.exit = app_st_setting_exit_callback;
#ifdef ST_SETTING_DELAY_TEST
    s_st_setting_opt.menuFuncKey.redKey.keyName = STR_ITEM_DELAY;
    s_st_setting_opt.menuFuncKey.redKey.keyPressFun = app_st_setting_redkey;
#endif
    s_st_setting_opt.menuFuncKey.blueKey.keyPressFun = app_st_setting_bluekey;
    if(g_AppPlayOps.running == true)
        s_st_setting_opt.backGround = BG_PLAY_RPOG;

    app_st_setting_tran_lang_item_init();
    app_st_setting_font_size_item_init();
    app_st_setting_font_color_item_init();
    app_st_setting_background_color_item_init();
    app_st_setting_code_item_init();
    app_st_setting_delay_init();

    app_system_set_create(&s_st_setting_opt);
    app_st_setting_login_status_show();
}

void app_subttrans_setting_menu_with_play(void)
{
   if(GXCORE_SUCCESS == app_pop_net_connect_check())
   {
       app_subttrans_setting_menu_exec();
   }
}

#endif
