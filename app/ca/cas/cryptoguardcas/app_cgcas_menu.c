#include "app.h"
#include "app_module.h"
#include "app_config.h"
#include "app_wnd_system_setting_opt.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_cgcas_config.h"
#include "cascam_ui_porting.h"

#define WND_CG_MAINMENU					"wnd_cryptoguard_mainmenu"
#define XML_CG_MAINMENU					"ca/cryptoguardcas/wnd_cryptoguard_mainmenu.xml"
#define TITLE_CG_MAINMENU               "text_cryptoguard_mainmenu_title"
#define LTV_CG_MAINMENU                 "ltv_cryptoguard_mainmenu"

extern void app_cgcas_event_task(void);

#define MENU_LEN_MAX  (50)
typedef struct _Cg_Menu_List
{
    char *title;
    char **submenu;
    uint32_t subnum;
}CgMenuList;
static CgMenuList s_list;

extern void app_cgcas_submenu_exec(char *info);

status_t app_cgcas_menu_list_data_free(CgMenuList *data)
{
    int i = 0;
    if(data == NULL)
        return GXCORE_ERROR;

    if(data->title != NULL)
    {
        GxCore_Free(data->title);
        data->title = NULL;
    }

    if(data->submenu != NULL)
    {
        for(i = 0; i < data->subnum; i++)
        {
            if(data->submenu[i] != NULL)
            {
                GxCore_Free(data->submenu[i]);
                data->submenu[i] = NULL;
            }
        }
        GxCore_Free(data->submenu);
        data->submenu= NULL;
    }
    data->subnum = 0;

    return GXCORE_SUCCESS;
}

static int app_cgcas_menu_list_get(char *menu)
{
    char *p = NULL, *q = NULL;
    uint32_t list_num = 0;
    int i = 0;
    if(menu == NULL)
        return -1;

    s_list.subnum = 0;
    q = menu;
    while((q-menu) < strlen(menu))
    {
        p = strchr(q, '\n');
        list_num ++;
        q = p+1;
    }

    q = menu;
    p = strchr(q, '\n');
    //title
    s_list.title = (char*)GxCore_Calloc((p-menu+1), sizeof(char));
    if(s_list.title == NULL) goto err_ret;
    memcpy(s_list.title, menu, (p-menu));
    q = p+1;

    //sub menu
    s_list.subnum = list_num - 1;
    s_list.submenu = (char**)GxCore_Calloc(s_list.subnum, sizeof(char*));
    if(s_list.submenu == NULL)
        goto err_ret;

    for(i = 0; i < s_list.subnum; i++)
    {
        p = strchr(q, '\n');
        s_list.submenu[i] = (char*)GxCore_Calloc((p-q+1), sizeof(char));
         if(s_list.submenu[i] == NULL)
             goto err_ret;

        memcpy(s_list.submenu[i], q, (p-q));
        q = p+1;
    }

    return 0;
err_ret:
    app_cgcas_menu_list_data_free(&s_list);
    return -1;
}
/*
static void test_lib_info(void)
{
    char info[CASCAM_CG_MAX_LIB_LVER_LEN] = {0};
    uint32_t sub_id = 0;

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_LIB_VER_LONG;
    if(0 != cascam_get(sub_id, (void*)&info))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);

    memset(info, 0, CASCAM_CG_MAX_LIB_LVER_LEN);
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_LIB_VER;
    if(0 != cascam_get(sub_id, (void*)&info))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);

    memset(info, 0, CASCAM_CG_MAX_LIB_LVER_LEN);
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_LIB_DATE;
    if(0 != cascam_get(sub_id, (void*)&info))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
}

//Just for test pin
static int test_parental_control_get(void)
{
    uint32_t sub_id = 0;
    unsigned char rate = 0;

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_STRICT_RATING;
    if(0 != cascam_get(sub_id, (void*)&rate))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    Cascam_CGSetStrictRating strictrating = {"0000", !rate};
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_STRICT_RATING;
    if(0 != cascam_set(sub_id, (void*)&strictrating))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    rate = 0;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_STRICT_RATING;
    if(0 != cascam_get(sub_id, (void*)&rate))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);

    rate = 0;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_MATURITY_RATING;
    if(0 != cascam_get(sub_id, (void*)&rate))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    Cascam_CGChangeRating rating = {"0000", rate+3};
    rate = rate+2;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_MATURITY_RATING;
    if(0 != cascam_set(sub_id, (void*)&rating))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    rate = 0;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_MATURITY_RATING;
    if(0 != cascam_get(sub_id, (void*)&rate))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);

    return 0;
}

static void test_cryptoguard_other_set_func(void)
{
    uint32_t sub_id = 0;

    Cascam_CGSetStbInfo stbinfo = {0};
    stbinfo.IrdNumber = 990000001;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_STB_INFO;          //Cascam_CGSetStbInfo)
    if(0 != cascam_set(sub_id, (void*)&stbinfo))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    //test return Error Param

    Cascam_CGCheckPin checkpin;
    strncpy(checkpin.pin, "1234", 4);
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_CHECK_PIN;             //Cascam_CGCheckPin)
    if(0 != cascam_set(sub_id, (void*)&checkpin))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    //test return CAS RETURN ERR
    strncpy(checkpin.pin, "0000", 4);
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_CHECK_PIN;             //Cascam_CGCheckPin)
    if(0 != cascam_set(sub_id, (void*)&checkpin))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    //test return 0; sucess;

    Cascam_CGChangePin changepin;
    strncpy(changepin.old_pin, "0000", 4);
    strncpy(changepin.new_pin, "1234", 4);
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_PIN;               //Cascam_CGChangePin)
    if(0 != cascam_set(sub_id, (void*)&changepin))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    //test return pin success;

    Cascam_CGUnlockPin unlockpin;
    strncpy(unlockpin.pin, "1234", 4);
    unlockpin.CH = 0;;
    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_UNLOCK_PIN;
    if(0 != cascam_set(sub_id, (void*)&unlockpin))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
    //test return pin success;
}
*/

int app_app_cryptoguard_get_sub_menu(uint32_t sel)
{
    static char info[4 * CASCAM_CG_MAX_MAIN_MENU_LEN];
    uint32_t sub_id = 0;

    if(sel == 0) {    // Subscription information
        sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_SUBSCRIPTION_INFO;
    } else if(sel == 1) {// Pay Per View information
        sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_PAY_INFO;
    } else if(sel == 2) {// About CA
        sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_ABOUT_CA;
    } else if(sel == 3) {// Parental Control
        sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_PARENTAL_CONTROL;
    }

    if(0 != cascam_get(sub_id, (void*)&info)) {
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);
        return -1;
    }

    app_cgcas_submenu_exec(info);

    return 0;

}

void app_cgcas_menu_exec(void)
{
    static char init_flag = 0;
    if(0 == init_flag) {
        init_flag = 1;
        GUI_LinkDialog(WND_CG_MAINMENU, XML_CG_MAINMENU);
    }

    int ret = 0;
    char menu_info[CASCAM_CG_MAX_MAIN_MENU_LEN] = {0};
    uint32_t sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_MAIN_MENU;

    if(0 != cascam_get(sub_id, (void*)&menu_info)) {
        printf("%s[%d] get menu info err!\n", __func__, __LINE__);
        return;
    }
    if((ret = app_cgcas_menu_list_get(menu_info)) == 0) {
        GUI_SetProperty(TITLE_CG_MAINMENU, "string", s_list.title);
        GUI_CreateDialog(WND_CG_MAINMENU);
    }
}

SIGNAL_HANDLER int app_cryptogurad_mainmenu_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cryptoguard_mainmenu_destroy(GuiWidget *widget, void *usrdata)
{
    app_cgcas_menu_list_data_free(&s_list);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cryptoguard_mainmenu_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type) {
        int key_val = find_virtualkey_ex(event->key.scancode,event->key.sym);
        switch (key_val) {
            case STBK_OK:
            case STBK_0:
            case STBK_1:
            case STBK_2:
            case STBK_3:
            case STBK_4:
            case STBK_5:
            case STBK_6:
            case STBK_7:
                break;
            case STBK_8:
                //test_cryptoguard_other_set_func();
                break;
            case STBK_9:
                //test_lib_info();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_CG_MAINMENU);
                break;

            default:
                ret = EVENT_TRANSFER_KEEPON;
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int  app_cryptoguard_mainmenu_ltv_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    uint32_t sel=0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_DOWN:
                case STBK_UP:
                case STBK_PAGE_DOWN:
                case STBK_PAGE_UP:
                    {
                        ret = EVENT_TRANSFER_KEEPON;
                    }
                    break;

                case STBK_OK:
                    {
                        GUI_GetProperty(LTV_CG_MAINMENU, "select", &sel);
                        app_app_cryptoguard_get_sub_menu(sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

SIGNAL_HANDLER int app_cryptoguard_mainmenu_ltv_get_total(GuiWidget *widget, void *usrdata)
{
    return s_list.subnum;
}

SIGNAL_HANDLER int app_cryptoguard_mainmenu_ltv_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    item->x_offset = 0;
    item->image = NULL;
    item->string = s_list.submenu[item->sel];

    return GXCORE_SUCCESS;
}


void app_cgcas_switch_channel(CascamSwitchChannel switch_channel)
{
    //TODO:
    cascam_set(CASCAM_MSG_CGCAS_BASEID+CASCAM_CGCAS_SWITCH_CHANNEL, (void*)&switch_channel);
}

void app_cgcas_switch_freq(void)
{
    //TODO:
}

void app_cgcas_stop_ca(void)
{
    //TODO:
}

void app_cgcas_init(void)
{
    CascamInitParam init_param;
    memset(&init_param, 0, sizeof(CascamInitParam));

    init_param.dmx_id = 0;
    init_param.ts_id = 0;
    //flash
    strcpy(init_param.flash.name, "PRIVATE");
    init_param.flash.offset = 0;
    init_param.flash.size = 64 * 1024;
    strcpy(init_param.backup.name, "CABUP");
    init_param.backup.offset = 64 * 1024;
    init_param.backup.size = 64 * 1024;
    //card
    init_param.sci.sci_switch = 1;
    init_param.sci.detect_pole = CASCAM_SMC_HIGH_LEVEL;
    init_param.sci.vcc_pole = CASCAM_SMC_HIGH_LEVEL;

	cascam_enable_debug();
	//cascam_disable_debug();
	cascam_init(&init_param);

	cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_CAS);
	cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_CASLIB);
	//cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_DEMUX);
	//cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_DESC);
	cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_SMC);
	cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_FLASH);
	//cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_SI);
	//cascam_control_gxcas_debug(1, CASCAM_DBG_GXCAS_MODULE_EADLIB);

    uint32_t sub_id = 0;

    Cascam_CGSetStbInfo stbinfo = {0};
    uint32_t IrdNumber = 990000001;
    uint8_t STB_SystemGlobalKey[16] = {0xDE, 0x52, 0x38, 0xF7, 0xCD, 0x98, 0x20, 0x35, 0x70, 0x12, 0x36, 0xC1, 0x59, 0xF3, 0x67, 0x12};
    uint8_t STB_ManufacturerGlobalKey[16] = {0};
    uint8_t STB_GroupKey[16] = {0xD6, 0x99, 0xE2, 0xAB, 0x4C, 0xCF, 0x41, 0xC8, 0xF3, 0x4A, 0x13, 0xFD, 0xD3, 0xB6, 0xEF, 0x7F};
    uint8_t STB_UniqueKey[16] = {0xA1, 0x26, 0x5C, 0x1D, 0xB4, 0x6F, 0xC7, 0xBC, 0xC1, 0xB6, 0x3B, 0xD3, 0x47, 0x5F, 0xDA, 0xA2};
    uint8_t STB_CardlessGroupKey[16] = {0};
    uint8_t STB_CardlessUniqueKey[16] = {0};

    stbinfo.IrdNumber = IrdNumber;
    memcpy(stbinfo.STB_SystemGlobalKey, STB_SystemGlobalKey, 16);
    memcpy(stbinfo.STB_ManufacturerGlobalKey, STB_ManufacturerGlobalKey, 16);
    memcpy(stbinfo.STB_GroupKey, STB_GroupKey, 16);
    memcpy(stbinfo.STB_UniqueKey, STB_UniqueKey, 16);
    memcpy(stbinfo.STB_CardlessGroupKey, STB_CardlessGroupKey, 16);
    memcpy(stbinfo.STB_CardlessUniqueKey, STB_CardlessUniqueKey, 16);

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_SET_STB_INFO;          //Cascam_CGSetStbInfo)
    if(0 != cascam_set(sub_id, (void*)&stbinfo))
        printf("%s[%d] get menu info err! id = %d\n", __func__, __LINE__, sub_id-CASCAM_MSG_CGCAS_BASEID);

    app_cgcas_event_task();

    return;
}

cascam_ui_control cascam_ui_cgcas_ops = {
    .ca_init = app_cgcas_init,
    .ca_menu_exec = app_cgcas_menu_exec,
    .ca_start_ca = NULL,
    .ca_stop_ca = app_cgcas_stop_ca,
    .ca_stop_descramble = NULL,
    .ca_switch_freq = app_cgcas_switch_freq,
    .ca_switch_channel = app_cgcas_switch_channel,
    .ca_quick_switch_channel = NULL,
    .ca_add_service = NULL,
    .ca_delete_service = NULL,
    .ca_destroy_finger = NULL,
    .ca_destroy_rolling_osd = NULL,
    .ca_do_event = app_cgcas_do_event,
#if CASCAM_UI_MULTI_LAYER
    .ca_do_event_layer = app_cgcas_do_event_layer,
#endif
    .ca_check_pvr_forbid = NULL, //app_abvcas_get_pvr_forbid,
};

#endif

