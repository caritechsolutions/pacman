#include "app.h"
#if RICHEPG_SUPPORT
#include "app_windows.h"
#include "richepg_json.h"
#include "richepg_store.h"
#include "richepg.h"
#include "app_eutel_database.h"
#include "app_eutel_common.h"
#include "richepg_file_splice.h"

#define LIST_EUTEL_LINEUP   "listview_lineup_list"
#define TEXT_EUTEL_LINEUP   "text_lineup_title"
#define GROUP_EUTEL_LINEUP  "cmb_lineup_group"

static struct eutelsat_lineup_list *s_elli = NULL;
static bool s_install_flag = false;
static char *s_content = NULL;

static int _update_lineup_logo(void)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    int logo_index = 0, i = 0;
    int active_sel = -1;
    int sel = 0;

    if (s_elli) {
        for (i = 0; i < s_elli->lineup_num; i++) {
            logo_index = richepg_get_logo_index(s_elli->lineups[i].logos_num, s_elli->lineups[i].logos);
            if(-1 == logo_index)
                continue;
            char *filepath = s_elli->lineups[i].logos[logo_index].path;
            char *logo_path = richepg_get_logo_path(filepath, RICHEPG_LOGO_LINEUP);
            if (logo_path)
                gal_add_key_path(filepath, logo_path);
            if(s_elli->lineups[i].id == mgr->sat_array[mgr->sat_sel].lineup_id)
                active_sel = i;
        }
    }

    GUI_SetProperty(LIST_EUTEL_LINEUP, "i18n", (s_elli && s_elli->lineup_num) ? "false" : "true");
    GUI_SetProperty(LIST_EUTEL_LINEUP, "update_all", NULL);
    GUI_SetProperty(LIST_EUTEL_LINEUP, "active", &active_sel);
    GUI_SetProperty(LIST_EUTEL_LINEUP, "select", &sel);

    return 0;
}

static int _change_lineup_sat(int sel)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    int ret = -1;
    int cur_sat_sel = -1 ;

    if (mgr)
       cur_sat_sel = mgr->sat_sel ;

    s_elli = NULL;
    richepg_parse_release();
    app_eutel_sat_sel_set(sel, true);

    if (mgr->sat_array[mgr->sat_sel].lineup_id == 0) {
        goto end;
    }
    if (richepg_switch_lineup_step1() < 0) {
        goto end;
    }
    if(richepg_get_elli(&s_elli) < 0) {
        goto end;
    }

    if (cur_sat_sel != mgr->sat_sel){
        richepg_files_splice_ctrl_release();
        richepg_files_splice_index_json_parse_all();
    }

    ret = 0;
end:
    _update_lineup_logo();
    return ret;
}

SIGNAL_HANDLER int app_eutel_lineup_create(GuiWidget *widget, void *usrdata)
{
    RichepgTsMgr *mgr = richepg_ts_mgr_get();
    GxBusPmDataSat *sat = NULL;
    char buffer[32] = {0};
    int i = 0;
    int len = 0;

    if (s_install_flag) {
        GUI_SetProperty(GROUP_EUTEL_LINEUP, "state", "hide");
        GUI_SetProperty(TEXT_EUTEL_LINEUP, "state", "show");
        _update_lineup_logo();
    } else {
        for (i = 0; i < mgr->sat_num; i++) {
            sat = app_eutel_sat_data_get(i);
            sprintf(buffer, "(%d/%d) ", i+1, mgr->sat_num);
            len += strlen((char*)sat->sat_s.sat_name) + strlen(buffer) + 1;
        }
        len += 2;
        s_content = GxCore_Calloc(1, len);
        s_content[0] = '[';
        for (i = 0; i < mgr->sat_num; i++) {
            sat = app_eutel_sat_data_get(i);
            sprintf(buffer, "(%d/%d) ", i+1, mgr->sat_num);
            strcat(s_content, buffer);
            strcat(s_content, (char*)sat->sat_s.sat_name);
            strcat(s_content, ",");
        }
        s_content[len-2] = ']';

        GUI_SetProperty(TEXT_EUTEL_LINEUP, "state", "hide");
        GUI_SetProperty(GROUP_EUTEL_LINEUP, "state", "show");
        GUI_SetProperty(GROUP_EUTEL_LINEUP, "content", s_content);
        if (mgr->sat_sel == 0) {
            _change_lineup_sat(0);
        } else {
            GUI_SetProperty(GROUP_EUTEL_LINEUP, "select", &mgr->sat_sel);
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_lineup_destroy(GuiWidget *widget, void *usrdata)
{
    APP_FREE(s_content);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_lineup_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (s_elli) {
        return s_elli->lineup_num;
    } else {
        return 1;
    }
}

SIGNAL_HANDLER int app_eutel_lineup_group_change(GuiWidget *widget, void *usrdata)
{
    if (!s_install_flag) {
        int sel = 0;
        GUI_GetProperty(GROUP_EUTEL_LINEUP, "select", &sel);
        _change_lineup_sat(sel);
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_lineup_list_get_data(GuiWidget *widget, void *usrdata)
{
    int index = 0;
    ListItemPara *item = NULL;
    char *logo = STR_ID_BLANK;
    char *name = STR_ID_BLANK;
    char *url = STR_ID_BLANK;
    RichepgTsMgr *mgr = richepg_ts_mgr_get();

    item = (ListItemPara *)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if(0 > item->sel)
        return GXCORE_ERROR;

    if (s_elli) {
        index = richepg_get_logo_index(s_elli->lineups[item->sel].logos_num, s_elli->lineups[item->sel].logos);
        if(index >= 0) logo = s_elli->lineups[item->sel].logos[index].path;
        index = richepg_get_locale_index(s_elli->lineups[item->sel].locales_num, s_elli->lineups[item->sel].locales);
        if(index >= 0) name = s_elli->lineups[item->sel].locales[index].name;
        url = s_elli->lineups[item->sel].customer.url;
    } else {
        if (mgr->sat_array[mgr->sat_sel].lineup_id == 0) {
            name = STR_ID_INSTALL_FIRST;
        } else {
            name = STR_ID_NOT_ENHANCED_MODE;
        }
    }

    //col-0: logo
    item->x_offset = 0;
    item->string = NULL;
    item->zoom = 1;
    item->image = logo;

    //col-1: name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    item->string = name;

    //col-2: url
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;
    item->string = url;

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_eutel_lineup_list_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int32_t sel = 0;
    int32_t ret = EVENT_TRANSFER_KEEPON;

    event = (GUI_Event *)usrdata;
    if (event->type != GUI_KEYDOWN)
        return ret;

    switch (find_virtualkey_ex(event->key.scancode,event->key.sym))
    {
        case STBK_LEFT:
        case STBK_RIGHT:
            if (!s_install_flag) {
                GUI_SendEvent(GROUP_EUTEL_LINEUP, event);
            }
            ret = EVENT_TRANSFER_STOP;
            break;
        case STBK_PAGE_UP:
            GUI_GetProperty(LIST_EUTEL_LINEUP, "select", &sel);
            if (sel == 0) {
                sel = app_eutel_lineup_list_get_total(NULL, NULL) - 1;
                GUI_SetProperty(LIST_EUTEL_LINEUP, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        case STBK_PAGE_DOWN:
            GUI_GetProperty(LIST_EUTEL_LINEUP, "select", &sel);
            if (app_eutel_lineup_list_get_total(NULL, NULL) == sel + 1) {
                sel = 0;
                GUI_SetProperty(LIST_EUTEL_LINEUP, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        case STBK_OK:
            if (s_elli) {
                GUI_GetProperty(LIST_EUTEL_LINEUP, "select", &sel);
                app_eutel_change_lineup(s_elli->lineups[sel].id);
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_EXIT:
        case STBK_MENU:
            if (  app_eutel_maintenance_check_end_dialog() ){
                richepg_parse_release();
                s_elli = NULL;
                richepg_files_splice_ctrl_release();
                richepg_files_splice_index_json_parse_all();
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        default:
            break;
    }

    return ret;
}

int app_eutel_lineup_create_dialog(bool install_flag)
{
    s_install_flag = install_flag;
    if (s_install_flag) {
        s_elli = NULL;
        if (richepg_get_elli(&s_elli) < 0)
            return -1;
    }

    GUI_CreateDialog(WND_EUTEL_LINEUP);
    GUI_SetFocusWidget(LIST_EUTEL_LINEUP);
    return 0;
}
#endif
