#include "app.h"
#include "app_module.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_cryptoguardcas_data.h"

#define WND_CG_BASEINFO            "wnd_cryptoguard_baseinfo"
#define XML_CG_BASEINFO            "ca/cryptoguardcas/wnd_cryptoguard_baseinfo.xml"

#define TXT_TITLE               "txt_cryptoguard_baseinfo_title"
#define TXT_ITEM1_L             "txt_cryptoguard_baseinfo_item1_l"
#define TXT_ITEM1_R             "txt_cryptoguard_baseinfo_item1_r"
#define TXT_ITEM2_L             "txt_cryptoguard_baseinfo_item2_l"
#define TXT_ITEM2_R             "txt_cryptoguard_baseinfo_item2_r"
#define TXT_ITEM3_L             "txt_cryptoguard_baseinfo_item3_l"
#define TXT_ITEM3_R             "txt_cryptoguard_baseinfo_item3_r"
#define TXT_ITEM4_L             "txt_cryptoguard_baseinfo_item4_l"
#define TXT_ITEM4_R             "txt_cryptoguard_baseinfo_item4_r"
#define TXT_ITEM5_L             "txt_cryptoguard_baseinfo_item5_l"
#define TXT_ITEM5_R             "txt_cryptoguard_baseinfo_item5_r"
#define TXT_ITEM6_L             "txt_cryptoguard_baseinfo_item6_l"
#define TXT_ITEM6_R             "txt_cryptoguard_baseinfo_item6_r"
#define TXT_ITEM7_L             "txt_cryptoguard_baseinfo_item7_l"
#define TXT_ITEM7_R             "txt_cryptoguard_baseinfo_item7_r"
#define TXT_ITEM8_L             "txt_cryptoguard_baseinfo_item8_l"
#define TXT_ITEM8_R             "txt_cryptoguard_baseinfo_item8_r"


static void app_cgcas_display_lib_ver(void);
static void app_cgcas_display_lib_data(void);
static void app_cgcas_display_lib_about_ca(void);

void app_cg_baseinfo_exec(void)
{

    static char init_flag = 0;
    if(0 == init_flag)
    {
        init_flag = 1;
        GUI_LinkDialog(WND_CG_BASEINFO, XML_CG_BASEINFO);
    }
    GUI_CreateDialog(WND_CG_BASEINFO);

    GUI_SetProperty(TXT_TITLE,   "string", "Test Info");
    GUI_SetProperty(TXT_ITEM1_L, "string", "1");
    GUI_SetProperty(TXT_ITEM2_L, "string", "1");
    GUI_SetProperty(TXT_ITEM3_L, "string", "1");
    GUI_SetProperty(TXT_ITEM4_L, "string", "1");
    GUI_SetProperty(TXT_ITEM5_L, "string", "1");
    //GUI_SetProperty(TXT_ITEM6_L, "string", "1");
    //GUI_SetProperty(TXT_ITEM7_L, "string", "1");
    //GUI_SetProperty(TXT_ITEM8_L, "string", "1");
    app_cgcas_display_lib_ver();
    app_cgcas_display_lib_data();
    app_cgcas_display_lib_about_ca();
}

SIGNAL_HANDLER int app_cryptoguard_baseinfo_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

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
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_CG_BASEINFO);
                    break;
                default:
                    break;
            }
        default:
            break;
    }
    return ret;
}

static void app_cgcas_display_lib_ver(void)
{
    uint32_t sub_id = 0;
    uint8_t ver[CASCAM_CG_MAX_LIB_VER_LEN+1] = {0};

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_LIB_VER;
    if(0 != cascam_get(sub_id, (void*)ver)){
        memcpy(ver, "FFFFFFFFFFFF", 12);
    }

    GUI_SetProperty(TXT_ITEM8_L, "string", "Lib Ver:");
    GUI_SetProperty(TXT_ITEM8_R, "string", ver);
}

static void app_cgcas_display_lib_data(void)
{
    uint32_t sub_id = 0;
    uint8_t data[CASCAM_CG_MAX_LIB_DATA_LEN+1] = {0};

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_LIB_DATE;
    if(0 != cascam_get(sub_id, (void*)data)){
        memcpy(data, "FFFFFFFFFFFF", 12);
    }

    GUI_SetProperty(TXT_ITEM7_L, "string", "Lib Data: ");
    GUI_SetProperty(TXT_ITEM7_R, "string", data);
}
static void app_cgcas_display_lib_about_ca(void)
{
    uint32_t sub_id = 0;
    uint8_t data[CASCAM_CG_MAX_ABOUT_CA_LEN+1] = {0};

    sub_id = CASCAM_MSG_CGCAS_BASEID + CASCAM_CGCAS_GET_ABOUT_CA;
    if(0 != cascam_get(sub_id, (void*)data)){
        memcpy(data, "FFFFFFFFFFFF", 12);
    }

    GUI_SetProperty(TXT_ITEM6_L, "string", "about ca: ");
    GUI_SetProperty(TXT_ITEM6_R, "string", data);
}
#else
SIGNAL_HANDLER int app_cryptoguard_baseinfo_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}
#endif
