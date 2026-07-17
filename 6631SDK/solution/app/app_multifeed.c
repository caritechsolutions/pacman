/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2014, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_mulifeed.c
* Author    : 	gechq
* Project   :	mulifeed Foundation
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   1.0  	2014.06	          glen           Creation
*****************************************************************************/

//public header files
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_common_select.h"
#include "app_channel_info.h"

#if MULTIFEED_LIST_SUPPORT
#define MAX_VIDEO_SOURCE_NAME	6
#define MULTIFEED_BUFFER_LENGTH	(MAX_VIDEO_SOURCE_NAME+20)

enum UNICABLE_SET_POPUP_BOX
{
	MULTIFEED_BOX_ITEM_VIDEO_PID = 0,
	MULTIFEED_BOX_ITEM_LANGUAGE,
	MULTIFEED_BOX_ITEM_AUDIO_TRACK
};

static int thiz_video_total = 0;
int app_multifeed_create_dialog(void);
extern uint16_t app_get_video_pid_status(int number,int *count);
extern int app_play_program_user_info(GxMsgProperty_NodeModify node_modify);
extern int app_pmt_check_vpid(unsigned short vpid);
extern int app_get_video_type_from_pmt(int sel);

static void app_multifeed_init_video_sourece(void)
{
	int i = 0;
	int video_total = 0;
	uint16_t video_pid = 0;
	GxMsgProperty_NodeByPosGet node = {0};

    video_total = thiz_video_total;

    node.node_type = NODE_PROG;
	node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

	if(node.prog_data.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE
        || ((g_AppPmt.sync_flag) && (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)))
	{
	    for(i = 0; i<video_total; i++)
	    {
		    video_pid = app_get_video_pid_status(i,&video_total);
		    if(node.prog_data.video_pid == video_pid)
		    {
			   GUI_SetProperty(g_CommonSelect.widget.list,"select",&i);
			   GUI_SetProperty(g_CommonSelect.widget.list,"active",&i);
			   return;
		    }
	   }
	   GUI_SetFocusWidget(g_CommonSelect.widget.list);
	}
	return;
}

int app_multifeed_clear(void)
{
    thiz_video_total = 0;

    return 0;
}

static void _multifeed_no_data_tip(void)
{
    PopDlg pop = {0};
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_NO_DATA;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;
    popdlg_create(&pop);
}

int app_video_multifeed_fullscreen(void)
{
    if(g_AppPlayOps.normal_play.play_total != 0)
    {
        GxMsgProperty_NodeByPosGet node = {0};
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        {
            if ((g_AppPvrOps.state != PVR_DUMMY)
                    &&(node.prog_data.id == g_AppPvrOps.env.prog_id))
            {
                app_pvr_create_no_btn_popdlg(NULL);
                return 1;
            }
            app_get_video_pid_status(0, &thiz_video_total);
            if(thiz_video_total < 2)
            {
                _multifeed_no_data_tip();
                return 0;
            }
            app_clean_fullscreen_tip(false, false, true);
            app_multifeed_create_dialog();
            app_multifeed_init_video_sourece();
        }
    }
    return 0;
}

static int app_multifeed_list_get_total(GuiWidget *widget, void *usrdata)
{
    return thiz_video_total;
}

static int app_multifeed_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;
	static char buffer[MULTIFEED_BUFFER_LENGTH] = {0};

	item = (ListItemPara*)usrdata;
	if(NULL == item)
		return GXCORE_ERROR;
	if(0 > item->sel)
		return GXCORE_ERROR;

	// col-0: num
	item->x_offset = 0;
	item->image = NULL;
	item->string = NULL;

	//col-1: channel name
	item = item->next;
	item->image = NULL;


	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);

	memset(buffer, 0, MULTIFEED_BUFFER_LENGTH);
	sprintf(buffer,"(%d/%d) %s%d", (item->sel+1), thiz_video_total, "Video",(item->sel+1));


	item->string = buffer;//app_multifeed_get_video_str(item->sel);

	return EVENT_TRANSFER_STOP;
}

static int app_multifeed_list_keypress(GuiWidget *widget, void *usrdata)
{
   GUI_Event *event = NULL;
   uint32_t box_sel = 0;
   int ret = EVENT_TRANSFER_KEEPON;
   uint16_t video_pid = 0;
   int video_total = 0;
   GxMsgProperty_NodeByPosGet node = {0};
   GxMsgProperty_NodeModify node_modify = {0};

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_COMMON_SELECT);
					app_channel_info_create_dialog();
					break;

				case STBK_EXIT:
				case STBK_MENU:
					GUI_EndDialog(WND_COMMON_SELECT);
					app_channel_info_create_dialog();
					break;

				case STBK_OK:
                    if(!thiz_video_total)
                        break;
					node.node_type = NODE_PROG;
					node.pos = g_AppPlayOps.normal_play.play_count;
					if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
				    {
                        if(!app_pmt_check_vpid(node.prog_data.video_pid))
                        {
                            GUI_GetProperty(g_CommonSelect.widget.list,"select",&box_sel);
                            GUI_SetProperty(g_CommonSelect.widget.list,"active",&box_sel);
                            video_pid = app_get_video_pid_status(box_sel,&video_total);
                            if(node.prog_data.video_pid!=video_pid)
                            {
                                node_modify.node_type = NODE_PROG;
					            memcpy(&node_modify.prog_data, &node.prog_data, sizeof(GxBusPmDataProg));
					            node_modify.prog_data.video_pid= video_pid;
					            node_modify.prog_data.video_type = app_get_video_type_from_pmt(box_sel);
						        if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
						        {
							        app_play_program_user_info(node_modify);
                                    if(g_AppFullArb.state.pause)
                                    {
                                        event->key.sym = STBK_PAUSE;
                                        GUI_SendEvent(WND_FULL_SCREEN, event);
                                    }
                                }
					        }
					    }
                    }
					GUI_EndDialog(WND_COMMON_SELECT);
					app_channel_info_create_dialog();
					return EVENT_TRANSFER_STOP;
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					return EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
	}

	return ret;
}

static int app_video_multifeed_create(GuiWidget *widget, void *usrdata)
{
	GUI_SetProperty(g_CommonSelect.widget.list,"update_all",NULL);
    GUI_SetProperty(g_CommonSelect.widget.title, "string", STR_ID_MULTIFEED);
	return EVENT_TRANSFER_STOP;
}

static int app_video_multifeed_destroy(GuiWidget *widget, void *usrdata)
{
    app_multifeed_clear();
	return EVENT_TRANSFER_STOP;
}

static int app_video_multifeed_keypress(GuiWidget *widget, void *usrdata)
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
				case VK_BOOK_TRIGGER:
					GUI_EndDialog(WND_COMMON_SELECT);
					if(g_AppFullArb.state.tv == STATE_OFF)
					{
						app_channel_info_create_dialog();
					}
					break;
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog(WND_COMMON_SELECT);
					}
					break;

				case STBK_OK:

					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

int app_multifeed_create_dialog(void)
{
    CommonSelectParas paras = {0};

    paras.choice = COMMON_SELECT_TITLE_ONLY;
    paras.title = STR_ID_MULTIFEED;

    paras.signal.create = app_video_multifeed_create;
    paras.signal.destroy = app_video_multifeed_destroy;
    paras.signal.keypress = app_video_multifeed_keypress;
    paras.signal.list_get_total = app_multifeed_list_get_total;
    paras.signal.list_get_data = app_multifeed_list_get_data;
    paras.signal.list_keypress = app_multifeed_list_keypress;
    app_common_select_create_dialog(&paras);

    return 0;
}
#endif
