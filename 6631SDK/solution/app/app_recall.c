#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_common_select.h"
#include "app_channel_info.h"

#define MAX_RECALL_CPUNT	16
/* Private variable------------------------------------------------------- */
/* widget macro -------------------------------------------------------- */

int g_recall_count = 0;


#if RECALL_LIST_SUPPORT
int app_recall_create_dialog(void);

int _recall_normal(void)
{
    g_AppFullArb.timer_stop();
    app_clean_fullscreen_tip(true, false, true);
    app_recall_create_dialog();

    return 0;
}

int _key_recall_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if (POP_VAL_OK == ret)
    {
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _recall_normal();
    }
    else
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            app_channel_info_create_dialog();
    }
    return 0;
}

static GxBusPmViewInfo s_old_info = {0};
extern int app_play_get_recall_total_count(void);

void app_recall_list_set_play_num(int recall_num)
{
	int total_recall_num = 0;
	total_recall_num = app_play_get_recall_total_count();
	if(total_recall_num <= 1)
	{
		g_recall_count = 0;
	}
	else
	{
		g_recall_count = recall_num;
	}
	return;
}

static int _recall_key_ok_keypress(void)
{
    GxBusPmViewInfo view_info = {0};
    int32_t sel = 0;

    GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    if(view_info.stream_type != g_AppPlayOps.recall_play[sel].view_info.stream_type)
    {
        if(view_info.stream_type != GXBUS_PM_PROG_TV)
        {
            g_AppFullArb.state.tv = STATE_ON;
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        }
        else
        {
            g_AppFullArb.state.tv = STATE_OFF;
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        }
    }
    g_AppPlayOps.play_list_create(&(g_AppPlayOps.recall_play[sel].view_info));
    g_AppPlayOps.program_play(PLAY_TYPE_FORCE, g_AppPlayOps.recall_play[sel].play_count);

    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    GUI_EndDialog(WND_COMMON_SELECT);

    return 0;
}

static int _recall_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if(POP_VAL_OK == ret)
    {
        if(PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _recall_key_ok_keypress();
    }
    return 0;
}

int app_recall_create(GuiWidget *widget, void *usrdata)
{
	GxMsgProperty_NodeNumGet node = {0};
	int cur_sel = 0;

	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&s_old_info));
	node.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node);

	if(0 < node.node_num)
	{
		GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
		cur_sel = 0;
		GUI_SetProperty(g_CommonSelect.widget.list, "select", (void*)(&cur_sel));
	}
	else
	{
		GUI_SetProperty(g_CommonSelect.widget.list, "update_all", NULL);
		GUI_SetProperty(g_CommonSelect.widget.list, "select", (void*)(&cur_sel));
	}


    	return EVENT_TRANSFER_STOP;
}

int app_recall_destroy(GuiWidget *widget, void *usrdata)
{
    app_channel_info_create_dialog();
	return EVENT_TRANSFER_STOP;
}

int app_recall_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
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
			switch(event->key.sym)
			{
				case STBK_DOWN:
				case STBK_UP:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					{
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_OK:
					GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
					g_recall_count = sel;
					GUI_EndDialog(WND_COMMON_SELECT);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_GetProperty(g_CommonSelect.widget.list, "select", &sel);
						g_recall_count = sel;
						g_AppPlayOps.play_list_create(&s_old_info);
                        GUI_EndDialog(WND_COMMON_SELECT);
                        if(g_AppFullArb.state.tv == STATE_ON
                                || (event->key.sym == VK_BOOK_TRIGGER))
                            app_channel_info_end_dialog();
                        break;
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


#define RECALL_LISTVIEW_
int app_recall_list_get_total(GuiWidget *widget, void *usrdata)
{
	uint32_t total = 0;
	total = app_play_get_recall_total_count();

	if(total > MAX_RECALL_CPUNT)
	{
		total = MAX_RECALL_CPUNT;
	}
	return total;
}

int app_recall_list_get_data(GuiWidget *widget, void *usrdata)
{
    #define CHANNEL_NUM_LEN	    (10)
	ListItemPara* item = NULL;
	static GxMsgProperty_NodeByPosGet node;
	static char buffer[CHANNEL_NUM_LEN];
	int cur_sel = 0;

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	cur_sel = item->sel;
	if(cur_sel <=0 || cur_sel > MAX_RECALL_CPUNT)
	{
		cur_sel = 0;
	}
	//col-0: num
	item->x_offset = 2;
	item->image = NULL;
	memset(buffer, 0, CHANNEL_NUM_LEN);
	sprintf(buffer, " %d", (item->sel+1));
	item->string = buffer;

	//col-1: channel name
	item = item->next;
	item->x_offset = 0;
	item->image = NULL;
	node.node_type = NODE_PROG;
	if(g_AppPlayOps.recall_play[cur_sel].view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		node.id = g_AppPlayOps.recall_play[cur_sel].view_info.radio_prog_cur;
	}
	else
	{
		node.id = g_AppPlayOps.recall_play[cur_sel].view_info.tv_prog_cur;
	}
	//get node
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
	item->string = (char*)node.prog_data.prog_name;

	  //col-2: $
	item = item->next;
	item->x_offset = 0;
	item->string = NULL;
	if (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
	    item->image = "s_icon_money";
	}
	else
	{
	    item->image = NULL;
	}

	return EVENT_TRANSFER_STOP;
}

int app_recall_list_keypress(GuiWidget *widget, void *usrdata)
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
			switch(event->key.sym)
			{
				case STBK_DOWN:
				case STBK_UP:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					{
						ret = EVENT_TRANSFER_KEEPON;
					}
					break;

				case STBK_LEFT:
				case STBK_RIGHT:

					break;

				case STBK_EXIT:
				case STBK_MENU:
					ret = EVENT_TRANSFER_KEEPON;
					break;

				case STBK_OK:
                    {
                        if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_recall_stop_pvr_cb))
                        {
                            _recall_key_ok_keypress();
                        }
                    }
                    break;

                case STBK_SAT:
                case STBK_FAV:
                    if(g_AppPvrOps.state != PVR_DUMMY && app_pvr_check_change_state())
                    {
                        app_pvr_create_no_btn_popdlg(NULL);
                        break;
                    }
                    ret = EVENT_TRANSFER_KEEPON;
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

int app_recall_create_dialog(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = "Recall";

    paras.signal.create = app_recall_create;
    paras.signal.destroy = app_recall_destroy;
    paras.signal.keypress = app_recall_keypress;
    paras.signal.list_get_total = app_recall_list_get_total;
    paras.signal.list_get_data = app_recall_list_get_data;
    paras.signal.list_keypress = app_recall_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
#endif

