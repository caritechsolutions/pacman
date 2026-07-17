#include "app.h"
#include "app_module.h"
#include "app_config.h"
#ifdef CASCAM_CGCAS_SUPPORT
#include "cascam.h"
#include "cascam_ui_common.h"
#include "cascam_cryptoguardcas_data.h"
#include "app_cgcas_config.h"

#define WND_CG_SUBMENU                 "wnd_cryptoguard_submenu"
#define XML_CG_SUBMENU                 "ca/cryptoguardcas/wnd_cryptoguard_submenu.xml"
#define TITLE_CG_SUBMENU               "text_cryptoguard_submenu_title"
#define LTV_CG_SUBMENU                 "ltv_cryptoguard_submenu"

typedef struct _Cg_Submenu_List
{
    char *title;
    char **submenu;
    uint32_t subnum;
}CgSubMenuList;
static CgSubMenuList s_subList;

status_t app_cgcas_submenu_list_data_free(CgSubMenuList *data)
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

static int app_cgcas_submenu_list_get(char *menu)
{
    char *p = NULL, *q = NULL;
    uint32_t list_num = 0;
    int i = 0;
    if(menu == NULL)
        return -1;

    s_subList.subnum = 0;
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
    s_subList.title = (char*)GxCore_Calloc((p-menu+1), sizeof(char));
    if(s_subList.title == NULL) goto err_ret;
    memcpy(s_subList.title, menu, (p-menu));
    q = p+1;

    //sub menu
    s_subList.subnum = list_num - 1;
    s_subList.submenu = (char**)GxCore_Calloc(s_subList.subnum, sizeof(char*));
    if(s_subList.submenu == NULL)
        goto err_ret;

    for(i = 0; i < s_subList.subnum; i++)
    {
        p = strchr(q, '\n');
        s_subList.submenu[i] = (char*)GxCore_Calloc((p-q+1), sizeof(char));
         if(s_subList.submenu[i] == NULL)
             goto err_ret;

        memcpy(s_subList.submenu[i], q, (p-q));
        q = p+1;
    }

    return 0;
err_ret:
    app_cgcas_submenu_list_data_free(&s_subList);
    return -1;
}


void app_cgcas_submenu_exec(char *info)
{
    static char init_flag = 0;
    if(0 == init_flag) {
        init_flag = 1;
        GUI_LinkDialog(WND_CG_SUBMENU, XML_CG_SUBMENU);
    }

    int ret = 0;
    if((ret = app_cgcas_submenu_list_get(info)) == 0) {
        GUI_CreateDialog(WND_CG_SUBMENU);
        GUI_SetProperty(TITLE_CG_SUBMENU, "string", s_subList.title);
    }
}

SIGNAL_HANDLER int app_cryptogurad_submenu_create(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cryptoguard_submenu_destroy(GuiWidget *widget, void *usrdata)
{
    app_cgcas_submenu_list_data_free(&s_subList);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_cryptoguard_submenu_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int ret = EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type) {
        int key_val = find_virtualkey_ex(event->key.scancode,event->key.sym);
        switch (key_val) {
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_CG_SUBMENU);
                break;

            default:
                ret = EVENT_TRANSFER_KEEPON;
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int  app_cryptoguard_submenu_ltv_keypress(GuiWidget *widget, void *usrdata)
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
                        if (strcmp("Parental Control", s_subList.title) == 0)
                        {
                            GUI_GetProperty(LTV_CG_SUBMENU, "select", &sel);
                            switch (sel)
                            {
                                case 0:
                                    break;

                                case 1: //S05-Change Maturity level
                                    app_cg_maturity_exec();
                                    break;

                                case 2: //S06-Change card PIN code
                                    app_cg_pin_exec();
                                    break;

                                case 3: //S07-Strict parental control
                                    app_cg_strict_rating_exec();
                                    break;

                                default:
                                    break;
                            }
                        }

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

SIGNAL_HANDLER int app_cryptoguard_submenu_ltv_get_total(GuiWidget *widget, void *usrdata)
{
    return s_subList.subnum;
}

SIGNAL_HANDLER int app_cryptoguard_submenu_ltv_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    item->x_offset = 0;
    item->image = NULL;
    item->string = s_subList.submenu[item->sel];

    return GXCORE_SUCCESS;
}
#endif

