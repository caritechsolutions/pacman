#include "app.h"
#include "app_windows.h"
#include "app_str_id.h"
#include "app_main_menu_tip.h"
#include "app_book.h"
#include "app_module.h"
#include "channel_common.h"
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if PARENTAL_LOCK_SUPPORT
#include "app_parent_lock.h"
#endif

#if TKGS_SUPPORT
GxTkgsOperatingMode app_tkgs_get_operatemode(void);
extern void app_tkgs_delete_all_channels(void);
extern int app_check_tkgs_control_db(void);
extern void app_tkgs_check_channel_num(unsigned int *tv_num, unsigned int *radio_num);
extern void app_tkgs_cantwork_popup(void);
#endif

#define BUFFER_LENGTH		36

extern void app_reboot(void);
extern void app_system_reset(void);

static MainMenuPopupType s_PopupType;

unsigned int app_check_channel_num(GxBusPmDataProgType type)
{
    GxBusPmViewInfo view_info;
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info.group_mode = GROUP_MODE_ALL;
    view_info.taxis_mode = TAXIS_MODE_NON;
    view_info.stream_type = type;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;

    return GxBus_PmProgNumGetByViewInfo(&view_info);
}

static void app_play_id_init(void)
{
    int i;
    GxMsgProperty_NodeNumGet   node_num;
    GxMsgProperty_NodeByPosGet node_data;
    GxMsgProperty_NodeModify   node_modify;

    memset(&node_num, 0, sizeof(GxMsgProperty_NodeNumGet));
    node_num.node_type = NODE_SAT;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    for(i = 0; i < node_num.node_num; i++)
    {
        memset(&node_data, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_data.node_type = NODE_SAT;
        node_data.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_data);
        //app_log_debug("----%s %d:%d----\n", node_data.sat_data.sat_s.sat_name, node_data.sat_data.cur_tv, node_data.sat_data.cur_radio);
        if ( app_fuse_spec_sat_type_check(node_data.sat_data.id) )
            continue ;

        if(node_data.sat_data.cur_tv != 0 || node_data.sat_data.cur_radio != 0)
        {
            memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
            node_modify.node_type = NODE_SAT;
            memcpy(&node_modify.sat_data, &node_data.sat_data, sizeof(GxBusPmDataSat));
            node_modify.sat_data.cur_tv = 0;
            node_modify.sat_data.cur_radio = 0;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
        }
    }

    memset(&node_num, 0, sizeof(GxMsgProperty_NodeNumGet));
    node_num.node_type = NODE_FAV_GROUP;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
    for(i = 0; i < node_num.node_num; i++)
    {
        memset(&node_data, 0, sizeof(GxMsgProperty_NodeByPosGet));
        node_data.node_type = NODE_FAV_GROUP;
        node_data.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_data);
        //app_log_debug("----%s %d:%d----\n", node_data.fav_item.fav_name, node_data.fav_item.cur_tv, node_data.fav_item.cur_radio);
        if(node_data.fav_item.cur_tv != 0 || node_data.fav_item.cur_radio != 0)
        {
            memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
            node_modify.node_type = NODE_FAV_GROUP;
            memcpy(&node_modify.fav_item, &node_data.fav_item, sizeof(GxBusPmDataFavItem));
            node_modify.fav_item.cur_tv = 0;
            node_modify.fav_item.cur_radio = 0;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
        }
    }

    GxBus_ConfigSetInt(ALL_TV_KEY, ALL_TV_ID);
    GxBus_ConfigSetInt(ALL_RADIO_KEY, ALL_RADIO_ID);
}

void _delete_all_channels(void)
{
    GxBusPmViewInfo view_info;
    GxMsgProperty_NodeDelete node_del = {0};
    uint32_t *p_id_array = NULL;
    uint32_t num;

    //delete all tv
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    view_info.stream_type = GXBUS_PM_PROG_ALL;
    view_info.group_mode = GROUP_MODE_ALL;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    num = GxBus_PmProgNumGetByViewInfo(&view_info);
    if (num == 0)
    {
        app_log_info("____delete_all_channels not TV program\n");
    }
    else
    {
        p_id_array = GxCore_Malloc(num * sizeof(uint32_t));
        if (p_id_array == NULL)
        {
            app_log_info("____malloc p_id_array failed\n");
        }
        else
        {
            GxBus_PmProgIdArryGetByViewInfo(&view_info, num, p_id_array);
            node_del.node_type = NODE_PROG;
            node_del.id_array = p_id_array;
            node_del.num = num;
            app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node_del);
            GxCore_Free(p_id_array);
            app_play_id_init();
            g_AppBook.invalid_remove();
#if BOUQUET_SUPPORT
            app_bouquet_clear_all();
#endif
#if SAT2IP_SERVER_SUPPORT
            app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
            app_dvb2ip_prog_num_update();
#endif
            g_AppChOps.create(0);

            app_fuse_spec_switch_work_mode(APP_FUSE_SATTV_WORK_MODE);
        }
    }
    return;
}

SIGNAL_HANDLER int app_mainmenu_tip_create(GuiWidget *widget, void *usrdata)
{
    char buffer[BUFFER_LENGTH] = {0};
    unsigned int tv_num = 0, radio_num = 0;

    if (MM_DEL_ALL == s_PopupType)
    {
#if TKGS_SUPPORT
        app_tkgs_check_channel_num(&tv_num,&radio_num);
#else
        tv_num = app_check_channel_num(GXBUS_PM_PROG_TV);
        radio_num = app_check_channel_num(GXBUS_PM_PROG_RADIO);
#endif

        if (tv_num == 0 && radio_num == 0)
        {
            s_PopupType = MM_DEL_ALL_NO_CH;
            GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_CH);
            GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
            GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
            GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
        }
        else
        {
            s_PopupType = MM_DEL_ALL_HAVE_CH;
            GUI_SetProperty("text_mainmenu_tip_tv", "string", STR_ID_TV);
            GUI_SetProperty("text_mainmenu_tip_radio", "string", STR_ID_RADIO);
            GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_DEL_ALL_CH);
            memset(buffer, 0, BUFFER_LENGTH);
            sprintf(buffer, "%d", tv_num);
            GUI_SetProperty("text_mainmenu_tip_tv_num", "string", buffer);
            memset(buffer, 0, BUFFER_LENGTH);
            sprintf(buffer, "%d", radio_num);
            GUI_SetProperty("text_mainmenu_tip_radio_num", "string", buffer);
            GUI_SetProperty("text_mainmenu_tip_tv", "state", "show");
            GUI_SetProperty("text_mainmenu_tip_radio", "state", "show");
            GUI_SetProperty("text_mainmenu_tip_tv_num", "state", "show");
            GUI_SetProperty("text_mainmenu_tip_radio_num", "state", "show");
            GUI_SetFocusWidget("btn_mainmenu_tip_ok");
        }
    }
    else if (MM_NO_CHANNEL == s_PopupType)
    {
        GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_CH);
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
        GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
        GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
    }
    else if (MM_NO_SAT == s_PopupType)
    {
        GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_NO_SAT);
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
        GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_INSTALL_FIRST);
        GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
    }
    else if (MM_MAX_SAT == s_PopupType)
    {
        GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_MAX_SAT);
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
        GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_DLE_ONE_SAT_FIRST);
        GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
    }
    else if (MM_MAX_TP == s_PopupType)
    {
        GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_MAX_TP);
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
        GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_DLE_ONE_TP_FIRST);
        GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
    }
#if APPLETS_SUPPORT
    else if (MM_APPLETS_OPEN == s_PopupType)
    {
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "hide");
        GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
        GUI_SetProperty("text_mainmenu_tip_info2", "string", STR_ID_WAITING);
        GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
        GUI_SetProperty("btn_mainmenu_tip_ok", "state", "hide");
    }
#endif
    GUI_SetProperty("img_mainmenu_tip_opt", "img", "s_bar_choice_blue4");
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mainmenu_tip_destroy(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_mainmenu_tip_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch (event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_MAINMENU_TIP);
                    GUI_SendEvent(WND_MAIN_MENU_ITEM, event);
                    break;
                case STBK_MENU:
                case STBK_EXIT:
                {
#if APPLETS_SUPPORT
                    if(MM_APPLETS_OPEN != s_PopupType)
#endif
                        GUI_EndDialog(WND_MAINMENU_TIP);
                }
                break;

                case STBK_LEFT:
                case STBK_RIGHT:
                    if (MM_DEL_ALL_HAVE_CH == s_PopupType)
                    {
                        if (0 == strcasecmp("btn_mainmenu_tip_ok", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("img_mainmenu_tip_opt", "img", "s_bar_choice_blue4_l");
                            GUI_SetFocusWidget("btn_mainmenu_tip_cancel");
                        }
                        else
                        {
                            GUI_SetProperty("img_mainmenu_tip_opt", "img", "s_bar_choice_blue4");
                            GUI_SetFocusWidget("btn_mainmenu_tip_ok");
                        }
                    }
                    break;

                case STBK_OK:
                    if (MM_DEL_ALL_HAVE_CH == s_PopupType)
                    {
                        if (0 == strcasecmp("btn_mainmenu_tip_ok", GUI_GetFocusWidget()))
                        {
                            GUI_SetProperty("text_mainmenu_tip_info1", "string", STR_ID_DELETING);
                            GUI_SetProperty("text_mainmenu_tip_info1", "state", "show");
                            GUI_SetProperty("text_mainmenu_tip_tv", "state", "hide");
                            GUI_SetProperty("text_mainmenu_tip_radio", "state", "hide");
                            GUI_SetProperty("text_mainmenu_tip_tv_num", "state", "hide");
                            GUI_SetProperty("text_mainmenu_tip_radio_num", "state", "hide");
                            GUI_SetProperty("text_mainmenu_tip_info2", "string", " ");
                            GUI_SetProperty("btn_mainmenu_tip_cancel", "state", "hide");
                            GUI_SetProperty("btn_mainmenu_tip_ok", "state", "hide");
                            GUI_SetProperty("img_mainmenu_tip_opt", "state", "hide");
                            GUI_SetProperty("img_mainmenu_tip_opt", "draw_now", NULL);

#if TKGS_SUPPORT
                            app_tkgs_delete_all_channels();
#else
                            _delete_all_channels();
#endif
                            GxCore_ThreadDelay(600);
                        }
                        GUI_EndDialog(WND_MAINMENU_TIP);
                    }
                    else if (MM_DEL_ALL_NO_CH == s_PopupType
                             || MM_NO_CHANNEL == s_PopupType
                             || MM_NO_SAT == s_PopupType
                             || MM_MAX_SAT == s_PopupType
                             || MM_MAX_TP == s_PopupType)
                    {
                        GUI_EndDialog(WND_MAINMENU_TIP);
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

void app_mainmenu_tip_exec(MainMenuPopupType type)
{
    s_PopupType = type;
    GUI_CreateDialog(WND_MAINMENU_TIP);
}

void app_deleteall_menu_exec(void)
{
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_operate_tip_get_force() < 0)
        return;
#endif
#if DVB2IP_SERVER_SUPPORT
    if (app_dvb2ip_operate_tip_get_force() < 0)
        return;
#endif
#if TKGS_SUPPORT
	if(GX_TKGS_OFF != app_tkgs_get_operatemode())
    {
    	app_tkgs_cantwork_popup();
	    return;
    }
#endif

    app_mainmenu_tip_exec(MM_DEL_ALL);
}

#if !TKGS_SUPPORT
static int _reset_reboot_cb(PopDlgRet ret)
{
    app_reboot();
    return 0;
}
#endif

static int _reset_passwd_poptip_creat_cb(void)
{
    GUI_SetInterface("flush",NULL);
	app_system_reset();
#if (TKGS_SUPPORT == 0)
    app_pop_reset_timeout(1);
    app_pop_set_string(STR_ID_REBOOT_NOW);
    app_pop_set_exitcb(_reset_reboot_cb);
#endif
    return 0;
}

static int _reset_sure_yes_no(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.timeout_sec = 2;
        pop.str  = STR_ID_PLZ_WAIT;
        pop.create_cb = _reset_passwd_poptip_creat_cb;
        popdlg_create(&pop);
    }
    return 0;
}

void app_reset_menu_exec(void)
{
    PopDlg  pop;
    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_SURE_RESET;
    pop.title = STR_ID_FACTORY_RESET;
    pop.exit_cb = _reset_sure_yes_no;
    popdlg_create(&pop);
}

