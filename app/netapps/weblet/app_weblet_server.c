#define _GNU_SOURCE
#include "app_module.h"
#include "app_send_msg.h"
#include "app_msg.h"
#include "module/app_play_control.h"
#include "app_config.h"
#include "full_screen.h"
#include "app_pop.h"
#include "app_windows.h"
#include "channel_common.h"
#include "app_channel_info.h"

#if WEBLET_SUPPORT
#include "app_key.h"
#include "app_weblet.h"
#include "mongoose.h"
#include "app_weblet_service.h"

#define MAX_FORBID_IP_LEN 30

static int server_run = 0;
static handle_t server_threadid = 0;
static pvr_state web_finder_pvr_status = PVR_DUMMY;
static int thiz_tag_pos = 0;
static GxMsgProperty_NodeByPosGet thiz_node_prog_tag = {0};

static char* s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;
extern int Gui_WriteKeyBuf(unsigned int key);
extern int utf8_check_string(const char *string, int length);
static event_list *sp_EndVolume=NULL;
static int _end_volume_cb(void *data)
{
    sp_EndVolume=NULL;
    GUI_EndDialog(WND_VOLUME);
    return (0);
}

typedef struct{
    GxBusPmViewInfo weblet_view_info;
    uint32_t prog_num;
    uint32_t *prog_id_list;
}WebletProgListInfo;
static WebletProgListInfo s_weblet_prog_list_info;
static bool s_weblet_prog_list_need_update_flag = false;
static uint32_t s_kb_need_sync_num = 0;

void app_kb_exec_edit_key(void)
{
    if(server_run && s_kb_need_sync_num >0)
       s_kb_need_sync_num--;
}

static int _web_finder_play_exec(void)
{
    GxMsgProperty_NodeByPosGet node_prog_tmp = {0};
	int i = 0;

	if(web_finder_pvr_status == PVR_RECORD)
	{
		for(i = 0; i < g_AppPlayOps.normal_play.play_total; i++)
		{
			node_prog_tmp.node_type = NODE_PROG;
			node_prog_tmp.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
			if(node_prog_tmp.prog_data.id == thiz_node_prog_tag.prog_data.id)
			{
				thiz_tag_pos = i;
				break;
			}
		}
	}

	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD) ||GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
	{
		return GXCORE_SUCCESS;
	}

	g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, thiz_tag_pos);
	g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

	return GXCORE_SUCCESS;
}

static int _web_finder_stop_pvr_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        if(web_finder_pvr_status == PVR_TIMESHIFT)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }

        _web_finder_play_exec();
    }
    return 0;
}

static void app_weblet_server_string_format(char *orig_char, char *dst_char)
{
    char *p1=orig_char;
    char *p2=dst_char;
    for(;*p1!='\0';p1++)
    {
        if(*p1 == '\"' || *p1 == '\\')
        {
            *p2='\\';
            p2++;
        }
        *p2=*p1;
        p2++;
    }
}

static int app_mg_send_head(struct mg_connection* nc, int ret)
{
    if(ret==GXCORE_SUCCESS)
    {
        mg_send_head(nc, 200, 0, 0);
    }
    else
    {
        mg_send_head(nc, 500, 0, 0);
    }

    return 0;
}

static int app_web_finder_play_count(int count)
{
    PopDlg pop = {0};

    web_finder_pvr_status = PVR_DUMMY;
    thiz_tag_pos = 0;
    memset(&thiz_node_prog_tag, 0, sizeof(GxMsgProperty_NodeByPosGet));

	if(g_AppPvrOps.state == PVR_TIMESHIFT || g_AppPvrOps.state == PVR_TMS_AND_REC)
		return 0;

    if (count >= g_AppPlayOps.normal_play.play_total)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_NO_PROGRAM;
		pop.timeout_sec = 3;

		popdlg_create(&pop);

		return GXCORE_ERROR;
	}
	else
	{
        thiz_tag_pos = count;
    }

	thiz_node_prog_tag.node_type = NODE_PROG;
	thiz_node_prog_tag.pos = thiz_tag_pos;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &thiz_node_prog_tag);

    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;

        if(g_AppPvrOps.state == PVR_TP_RECORD)
        {
            pop.str = STR_ID_STOP_REC_INFO;
            web_finder_pvr_status = PVR_TP_RECORD;
        }
        else if(g_AppPvrOps.state == PVR_RECORD)
        {
            if(app_pvr_check_change_state())
            {
                pop.str = STR_ID_STOP_REC_INFO;
                web_finder_pvr_status = PVR_RECORD;
            }
        }
        else
        {
			;
		}
    }

    if(web_finder_pvr_status != PVR_DUMMY)
    {
        pop.exit_cb = _web_finder_stop_pvr_cb;
        popdlg_create(&pop);
    }
    else
	{
		_web_finder_play_exec();
	}

    return GXCORE_SUCCESS;
}


static int app_web_finder_change_group(GroupDir dir)
{
    GxBusPmViewInfo view_info_temp = {0};
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
    if(s_weblet_prog_list_info.weblet_view_info.group_mode == g_AppPlayOps.normal_play.view_info.group_mode)
    {
        if((g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_ALL)||
            (g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_HD)||
           ((g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_SAT)&&(s_weblet_prog_list_info.weblet_view_info.sat_id ==g_AppPlayOps.normal_play.view_info.sat_id))||
           ((g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_FAV)&&(s_weblet_prog_list_info.weblet_view_info.fav_id ==g_AppPlayOps.normal_play.view_info.fav_id)))
        {
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_LIST))
            {
                GUI_Event event = {0};
                event.type = GUI_KEYDOWN;
                event.key.sym = (dir==PREV_GROUP)?STBK_LEFT:STBK_RIGHT;
                GUI_SendEvent("listview_channel_list", &event);
            }
            else
            {
                app_weblet_switch_cmb_group(dir);
            }
        }
    }
    //else just update weblet group
    return GXCORE_SUCCESS;
}

static void app_weblet_server_push_web_input_content(struct mg_connection* nc, struct http_message* hm)
{
	char edit[128] = {0};
    AppMsg_WebletUIControl param = {0};

	mg_get_http_var(&hm->body, "edit", edit, sizeof(edit));

	app_log_debug(" -- [%s][%d] get weblet edit data: %s\n", __func__, __LINE__, edit);
    param.type = MSG_WEBLET_KEYBOARD_INPUT;
	param.input = edit;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);
    app_mg_send_head(nc, param.result);
}

static void app_weblet_server_confirm_web_edit_content(struct mg_connection* nc, struct http_message* hm)
{
	char edit[128] = {0};
    AppMsg_WebletUIControl param = {0};

	mg_get_http_var(&hm->body, "edit", edit, sizeof(edit));

	app_log_debug(" -- [%s][%d] confirm weblet edit data: %s\n", __func__, __LINE__, edit);
    param.type = MSG_WEBLET_KEYBOARD_INPUT_CONFIRM;
	param.input = edit;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);
    app_mg_send_head(nc, param.result);
}


static void app_weblet_server_kb_update(struct mg_connection* nc, struct http_message* hm)
{
	char edit[128] = {0};
    char edit1[256]={0};
    AppMsg_WebletUIControl param = {0};
    s_kb_need_sync_num = 0;


    param.type = MSG_WEBLET_GET_KEYBOARD_INPUT;
    param.input = edit;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

    if(param.result==GXCORE_SUCCESS)
    {
        app_weblet_server_string_format(edit,edit1);
        app_log_debug(" -- [%s][%d] get stb keyboard data: \n%s\n", __func__, __LINE__, param.input);
        mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc, "{ \"keyedit\": [ ");
        mg_printf_http_chunk(nc, "{ \"content\": \"%s\" }", edit1);
        mg_printf_http_chunk(nc, " ] }");
        mg_send_http_chunk(nc, "", 0);
    }
    else
    {
        mg_send_head(nc, 500, 0, 0);
    }
}

static void app_weblet_server_kb_edit_sync(struct mg_connection* nc, struct http_message* hm)
{
    char edit[128] = {0};
    char edit1[256] = {0};
    AppMsg_WebletUIControl param = {0};

    param.type = MSG_WEBLET_GET_KEYBOARD_INPUT;
    param.input = edit;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);
    if((s_kb_need_sync_num == 0)||(strlen(param.input) == 0))
    {
        s_kb_need_sync_num = 0;
        if(param.result==GXCORE_SUCCESS)
        {
            app_log_debug(" -- [%s][%d] get stb keyboard data: \n%s\n", __func__, __LINE__, param.input);
            app_weblet_server_string_format(edit,edit1);
            mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
            mg_printf_http_chunk(nc, "{ \"keyedit\": [ ");

            mg_printf_http_chunk(nc, "{ \"content\": \"%s\" }", edit1);

            mg_printf_http_chunk(nc, " ] }");
            mg_send_http_chunk(nc, "", 0);
        }
        else
        {
            mg_send_head(nc, 500, 0, 0);
        }
    }
    else
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc, "{ \"keyedit\": [ ");
        mg_printf_http_chunk(nc, "{ \"content\": \"%s\" }", "invalid");
        mg_printf_http_chunk(nc, " ] }");
        mg_send_http_chunk(nc, "", 0);
    }
}

static void app_weblet_server_get_cur_play_program(struct mg_connection* nc, struct http_message* hm)
{
    int pos;
    GxBusPmViewInfo  view_info_temp = {0};

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
    if(view_info_temp.skip_view_switch != VIEW_INFO_SKIP_VANISH)
    {
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        g_AppPlayOps.play_list_create(&view_info_temp);
        pos = g_AppPlayOps.normal_play.play_count;
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        g_AppPlayOps.play_list_create(&view_info_temp);
    }
    else
    {
        pos = g_AppPlayOps.normal_play.play_count;
    }

	mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(nc, "{ \"curprog\": [ ");
	mg_printf_http_chunk(nc, "{ \"id\": %d }",  pos);
	mg_printf_http_chunk(nc, " ] }");
	mg_send_http_chunk(nc, "", 0);
}

static int app_save_weblet_prog_list_info(GxBusPmViewInfo view_info,uint32_t prog_num)
{
    GxCore_Free(s_weblet_prog_list_info.prog_id_list);
    memset(&s_weblet_prog_list_info, 0, sizeof(WebletProgListInfo));
    s_weblet_prog_list_need_update_flag = false;
    memcpy(&s_weblet_prog_list_info.weblet_view_info, &view_info,sizeof(view_info));
    s_weblet_prog_list_info.prog_num = prog_num;
    s_weblet_prog_list_info.prog_id_list = (uint32_t *)GxCore_Malloc(prog_num*sizeof(uint32_t));
    uint32_t i;
    GxMsgProperty_NodeByPosGet ProgNode;
    for(i=0; i< prog_num; i++)
    {
        ProgNode.node_type = NODE_PROG;
        ProgNode.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
        s_weblet_prog_list_info.prog_id_list[i] = ProgNode.prog_data.id;
    }
   return 0;
}

static bool app_check_weblet_prog_list_interal_change(GxBusPmViewInfo view_info,uint32_t prog_num)
{
    if(s_weblet_prog_list_info.prog_num != prog_num)
    {
        return false;
    }
    else
    {
        if(s_weblet_prog_list_info.weblet_view_info.order != view_info.order)
        {
            return false;
        }
        uint32_t i;
        GxMsgProperty_NodeByPosGet ProgNode;
        for(i=0; i<prog_num; i++)
        {
            ProgNode.node_type = NODE_PROG;
            ProgNode.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
            if(s_weblet_prog_list_info.prog_id_list[i] != ProgNode.prog_data.id)
            {
                return false;
            }
        }
    }
    return true; //no change but not include prog name change
}

int app_compare_weblet_prog_list_with_box_list(GxBusPmViewInfo view_info, uint32_t prog_num)
{
    if(s_weblet_prog_list_info.weblet_view_info.group_mode != view_info.group_mode)
    {
        s_weblet_prog_list_need_update_flag = true;
    }
    else
    {
        if(view_info.group_mode == GROUP_MODE_ALL)
        {
            if(!app_check_weblet_prog_list_interal_change(view_info,prog_num))
            {
                s_weblet_prog_list_need_update_flag = true;
            }
        }
        else if(view_info.group_mode == GROUP_MODE_SAT)
        {
            if(s_weblet_prog_list_info.weblet_view_info.sat_id != view_info.sat_id)
            {
                s_weblet_prog_list_need_update_flag = true;
            }
            else
            {
                if(!app_check_weblet_prog_list_interal_change(view_info,prog_num))
                {
                    s_weblet_prog_list_need_update_flag = true;
                }
            }
        }
        else if(view_info.group_mode == GROUP_MODE_FAV)
        {
            if(s_weblet_prog_list_info.weblet_view_info.fav_id != view_info.fav_id)
            {
                s_weblet_prog_list_need_update_flag = true;
            }
            else
            {
                if(!app_check_weblet_prog_list_interal_change(view_info,prog_num))
                {
                    s_weblet_prog_list_need_update_flag = true;
                }
            }
        }
        else
          app_log_error("prog list compare error\n");
    }
    return 0;
}

static void app_set_update_flag_by_pvr_state(void)
{
    if(g_AppPvrOps.state == PVR_TIMESHIFT)
    {
        s_weblet_prog_list_need_update_flag = true;
    }
}

static int _weblet_switch_prog_or_group_uicheck(WebletUICheck mode)
{
    const char *focus = NULL;

    focus = GUI_GetFocusWindow();

    if(!focus)
    {
    }
    else if(0 == strcmp(WND_WEBLET, focus))
    {
        GUI_EndDialog("after wnd_full_screen");
    }
    else if ((0 == strcmp(WND_FULL_SCREEN, focus))
            || (0 == strcmp(WND_POP_QR, focus))
            || (0 == app_channel_list_compare_dialog(focus)) )
    {
        if(mode == SWITCH_PROG)
            GUI_EndDialog("after wnd_full_screen");
    }
    else if (0 == app_channel_info_compare_dialog(focus))
    {
        if(g_AppFullArb.state.tv == STATE_ON)
            app_channel_info_end_dialog();
    }
    else if (0 == app_channel_info_signal_compare_dialog(focus))
    {
        app_channel_info_signal_end_dialog();
        if(g_AppFullArb.state.tv == STATE_ON)
            app_channel_info_end_dialog();
    }
    else if( (0 == strcmp(WND_PASSWD, focus)) &&
            ((GXCORE_SUCCESS == app_channel_list_check_dialog())
            || (GXCORE_SUCCESS == app_channel_info_check_dialog()))
           )
    {
        GUI_EndDialog("after wnd_full_screen");
    }
    else if (0 == strcmp(WND_VOLUME, focus))
    {
        if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
        {
            if (reset_timer(sp_EndVolume) != 0)
            {
                sp_EndVolume = create_timer(_end_volume_cb, 0, NULL, TIMER_ONCE);
            }
        }
        else
        {
            app_log_error("the window is media!!!\n");
            return -1;
        }
    }
    else
    {
        app_log_error("%s is not support wepfind!!!\n",focus);
        return -1;
    }
    return 0;
}

static void app_weblet_server_get_weblet_prog_list_status(struct mg_connection* nc, struct http_message* hm)
{
    GxMsgProperty_NodeNumGet prog_num;

    prog_num.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));

    app_compare_weblet_prog_list_with_box_list(g_AppPlayOps.normal_play.view_info,prog_num.node_num);
    app_set_update_flag_by_pvr_state();
    if(s_weblet_prog_list_need_update_flag == true)
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc, "{ \"curstatus\": [ ");
        mg_printf_http_chunk(nc, "{ \"sta\": \"invalid\" }");
        mg_printf_http_chunk(nc, " ] }");
		mg_send_http_chunk(nc, "", 0);
    }
    else
    {
        mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc, "{ \"curstatus\": [ ");
        mg_printf_http_chunk(nc, "{ \"sta\": \"valid\" }");
        mg_printf_http_chunk(nc, " ] }");
		mg_send_http_chunk(nc, "", 0);
    }
    return;
}
static void app_weblet_server_get_programmes(struct mg_connection* nc, char* keyword)
{
	GxMsgProperty_NodeNumGet nodeNum = {0};
	GxMsgProperty_NodeByPosGet node = {0};
    GxBusPmViewInfo view_info = {0};
    int ret=GXCORE_SUCCESS;
	int pos;
	int first = 1;
    bool view_info_change_flag = false;
    char progname[MAX_PROG_NAME*2]={0};

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
	if(view_info.skip_view_switch == VIEW_INFO_SKIP_CONTAIN)
	{
		view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        g_AppPlayOps.play_list_create(&view_info);
        view_info_change_flag = true;
    }

	nodeNum.node_type = NODE_PROG;
	ret = app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void*)(&nodeNum));
	if (ret != GXCORE_SUCCESS)
	{
		mg_send_head(nc, 500, 0, 0);
		return;
	}
	mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
	mg_printf_http_chunk(nc, "{ \"programmes\": [ ");
    for (pos=0; pos<nodeNum.node_num; pos++)
    {
        node.node_type = NODE_PROG;
        node.pos = pos;

        ret = app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)(&node));
        if (ret==GXCORE_SUCCESS && (!keyword || strcasestr((char*)node.prog_data.prog_name, keyword)))
        {
            memset(progname,0,sizeof(progname));
            if(utf8_check_string((char*)node.prog_data.prog_name,-1))
                app_weblet_server_string_format((char*)node.prog_data.prog_name,progname);
            else
                strcpy(progname,"unkown");
            mg_printf_http_chunk(nc, "%s{ \"id\": %d, \"name\": \"%s\",\"scramble\": \"%s\",\"lock\": \"%s\"}",
                first?"":", ", pos,
                progname,
                node.prog_data.scramble_flag?"Y":"N",
                node.prog_data.lock_flag?"Y":"N");
            if (first) first = 0;
        }
    }
    mg_printf_http_chunk(nc, " ] }");
    mg_send_http_chunk(nc, "", 0);
    app_save_weblet_prog_list_info(view_info,nodeNum.node_num);
    if(view_info_change_flag)
    {
        view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        g_AppPlayOps.play_list_create(&view_info);

    }
    return;
}

static int app_weblet_server_get_cur_group(struct mg_connection* nc, struct http_message* hm)
{
    char buffer[100];
    bool view_info_change_flag = false;
    GxBusPmViewInfo view_info_temp = {0};
    GxMsgProperty_NodeByIdGet Node;
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void*)(&view_info_temp));
    if(view_info_temp.skip_view_switch != VIEW_INFO_SKIP_VANISH)
    {
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        g_AppPlayOps.play_list_create(&view_info_temp);
        view_info_change_flag = true;
    }

    mg_printf(nc, "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");

    if(s_weblet_prog_list_info.weblet_view_info.group_mode == GROUP_MODE_ALL)
    {
        mg_printf_http_chunk(nc, "{\"name\": \"%s\"}","ALL");
    }
    else if(s_weblet_prog_list_info.weblet_view_info.group_mode == GROUP_MODE_SAT)
    {
        Node.node_type = NODE_SAT;
        Node.id = s_weblet_prog_list_info.weblet_view_info.sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &Node);

        memset(buffer, 0, 100);
        app_weblet_server_string_format((char*)app_get_sat_name(&Node.sat_data),buffer);
        mg_printf_http_chunk(nc, "{\"name\": \"%s\"}",buffer);
    }
    else if(s_weblet_prog_list_info.weblet_view_info.group_mode == GROUP_MODE_HD)
    {
        mg_printf_http_chunk(nc, "{\"name\": \"%s\"}","HD");
    }
    else
    {
        Node.node_type = NODE_FAV_GROUP;
        Node.id = s_weblet_prog_list_info.weblet_view_info.fav_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &Node);

        memset(buffer, 0, 100);
        sprintf(buffer,"%s",Node.fav_item.fav_name);
        app_weblet_server_string_format((char*)Node.fav_item.fav_name,buffer);
        mg_printf_http_chunk(nc, "{\"name\": \"%s\"}",buffer);
    }
    mg_send_http_chunk(nc, "", 0);

    if(view_info_change_flag)
    {
        view_info_temp.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
        g_AppPlayOps.play_list_create(&view_info_temp);
    }
    return 0;


}

static void app_weblet_server_switch_programme(struct mg_connection* nc, struct http_message* hm)
{
    char id[32];
    AppMsg_WebletUIControl param = {0};
    unsigned int prog_id;

    param.type = MSG_WEBLET_UI_CHECK;
    param.mode = SWITCH_PROG;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

    if(param.result==GXCORE_SUCCESS)
    {
        memset(&param,0,sizeof(param));
        mg_get_http_var(&hm->body, "id", id, sizeof(id));
        prog_id = atoi(id);
        param.type = MSG_WEBLET_PLAY_PROGRAM;
        param.prog_id = prog_id;
        app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

        app_mg_send_head(nc,param.result);
    }
    else
    {
        mg_send_head(nc, 500, 0, 0);
    }
}

static void app_weblet_server_change_group(struct mg_connection* nc, struct http_message* hm)
{
    AppMsg_WebletUIControl param = {0};
    char dir[32];

    param.type = MSG_WEBLET_UI_CHECK;
    param.mode = SWITCH_GROUP;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

    if(param.result==GXCORE_SUCCESS)
    {
        memset(&param,0,sizeof(param));
        mg_get_http_var(&hm->body, "dir", dir, sizeof(dir));
        param.type = MSG_WEBLET_CHANGE_GROUP;
        if(!strcmp(dir,"prev"))
        {
            param.group_dir=PREV_GROUP;
        }
        else
        {
            param.group_dir=NEXT_GROUP;
        }
        app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

        app_mg_send_head(nc,param.result);
    }
    else
    {
        mg_send_head(nc, 501, 0, 0);
    }
}

static void app_weblet_server_find_programmes(struct mg_connection* nc, struct http_message* hm)
{
	char keyword[512];

	mg_get_http_var(&hm->body, "keyword", keyword, sizeof(keyword));
	app_weblet_server_get_programmes(nc, keyword);
}

static void app_weblet_server_remote(struct mg_connection* nc, struct http_message* hm)
{
	char key[32];
    unsigned int code;
    AppMsg_WebletUIControl param = {0};

	mg_get_http_var(&hm->body, "key", key, sizeof(key));
	DEBUG("key: %s\n", key);

	if (!strcmp(key, "power")) 			code = STBK_HALT;
	else if (!strcmp(key, "mute")) 		code = STBK_MUTE;
	else if (!strcmp(key, "menu")) 		code = STBK_MENU;
	else if (!strcmp(key, "ok")) 		code = STBK_OK;
	else if (!strcmp(key, "record")) 	code = STBK_REC_START;
	else if (!strcmp(key, "play")) 		code = STBK_PLAY;
	else if (!strcmp(key, "pause")) 	code = STBK_PAUSE;
	else if (!strcmp(key, "stop")) 		code = APPK_STOP;
	else if (!strcmp(key, "up")) 		code = STBK_UP;
	else if (!strcmp(key, "down")) 		code = STBK_DOWN;
	else if (!strcmp(key, "left")) 		code = STBK_LEFT;
	else if (!strcmp(key, "right")) 	code = STBK_RIGHT;
	else if (!strcmp(key, "fav")) 		code = STBK_FAV;
	else if (!strcmp(key, "volup")) 	code = STBK_VOLUP;
	else if (!strcmp(key, "voldown")) 	code = STBK_VOLDOWN;
	else if (!strcmp(key, "red")) 		code = STBK_RED;
	else if (!strcmp(key, "green")) 	code = STBK_GREEN;
	else if (!strcmp(key, "yellow")) 	code = STBK_YELLOW;
	else if (!strcmp(key, "blue")) 		code = STBK_BLUE;
	else if (!strcmp(key, "epg")) 		code = STBK_EPG;
	else if (!strcmp(key, "audio")) 	code = STBK_AUDIO;
	else if (!strcmp(key, "sub")) 		code = STBK_SUBT;
	else if (!strcmp(key, "ttx")) 		code = STBK_TTX;
	else if (!strcmp(key, "exit")) 		code = STBK_EXIT;
	else if (!strcmp(key, "info")) 		code = STBK_INFO;
	else if (!strcmp(key, "previous")) 	code = APPK_PREVIOUS;
	else if (!strcmp(key, "next")) 		code = STBK_NEXT;
	else if (!strcmp(key, "reward")) 	code = APPK_REW;
	else if (!strcmp(key, "forward")) 	code = APPK_FF;
	else if (!strcmp(key, "tvr")) 	    code = STBK_TV_RADIO;
    else if (!strcmp(key, "sat"))       code = STBK_SAT;
    else if (!strcmp(key, "fav"))       code = STBK_FAV;
	else if (!strcmp(key, "zoom"))	    code = STBK_QR;
    else if (!strcmp(key, "recall"))    code = STBK_RECALL;
    else if (!strcmp(key, "find"))      code = STBK_FIND;
    else if (!strcmp(key, "media"))     code = STBK_MEDIA;
	else if (!strcmp(key, "0")) 		code = STBK_0;
	else if (!strcmp(key, "1")) 		code = STBK_1;
	else if (!strcmp(key, "2")) 		code = STBK_2;
	else if (!strcmp(key, "3")) 		code = STBK_3;
	else if (!strcmp(key, "4")) 		code = STBK_4;
	else if (!strcmp(key, "5")) 		code = STBK_5;
	else if (!strcmp(key, "6")) 		code = STBK_6;
	else if (!strcmp(key, "7")) 		code = STBK_7;
	else if (!strcmp(key, "8")) 		code = STBK_8;
	else if (!strcmp(key, "9")) 		code = STBK_9;
	else
	{
		mg_http_send_error(nc, 403, 0);
		return;
	}

    param.type = MSG_WEBLET_REMOTE;
    param.keycode = code;
    app_send_msg_exec(APPMSG_WEBLET_UI_CONTROL, &param);

    app_mg_send_head(nc,param.result);
}

static void _weblet_ev_handler(struct mg_connection* nc, int ev, void* ev_data)
{
	struct http_message* hm = (struct http_message*)ev_data;

	if (ev != MG_EV_HTTP_REQUEST) return;

	if (!mg_vcmp(&hm->uri, "/remote"))
		app_weblet_server_remote(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/programmes"))
		app_weblet_server_get_programmes(nc, 0);
	else if (!mg_vcmp(&hm->uri, "/programme"))
		app_weblet_server_switch_programme(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/finder"))
		app_weblet_server_find_programmes(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/curprog"))
		app_weblet_server_get_cur_play_program(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/kb_update"))
		app_weblet_server_kb_update(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/kb_edit_sync"))
		app_weblet_server_kb_edit_sync(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/pushedit"))
		app_weblet_server_push_web_input_content(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/confirm"))
		app_weblet_server_confirm_web_edit_content(nc, hm);
	else if (!mg_vcmp(&hm->uri, "/getliststatus"))
        app_weblet_server_get_weblet_prog_list_status(nc, hm);
    else if (!mg_vcmp(&hm->uri, "/changegroup"))
        app_weblet_server_change_group(nc, hm);
    else if (!mg_vcmp(&hm->uri, "/getcurgroup"))
        app_weblet_server_get_cur_group(nc, hm);
	else /* static content */
		mg_serve_http(nc, hm, s_http_server_opts);
}

static void weblet_serverThread(void* arg)
{
	struct mg_mgr mgr;
	struct mg_connection* nc;
	struct mg_bind_opts bind_opts;
	const char* err_str;

	mg_mgr_init(&mgr, 0);

	/* Set HTTP server options */
	memset(&bind_opts, 0, sizeof(bind_opts));
	bind_opts.error_string = &err_str;

	nc = mg_bind_opt(&mgr, s_http_port, _weblet_ev_handler, bind_opts);
	if (!nc)
	{
		ERROR("Error starting server on port %s: %s\n", s_http_port, *bind_opts.error_string);
		return;
	}

	mg_set_protocol_http_websocket(nc);
	s_http_server_opts.enable_directory_listing = "no";
	s_http_server_opts.document_root = "/dvb/theme/www";
    //s_http_server_opts.ip_acl = "-127.0.0.0/8,-192.168.199.123";

	server_run = 1;

	DEBUG("Starting server on  %s ...\n", s_http_port);
	while (server_run) mg_mgr_poll(&mgr, 1000);
	mg_mgr_free(&mgr);

	return;
}

bool app_check_weblet_server_state(void)
{
    if(server_run)
        return true;
    else
        return false;
}

char *app_get_weblet_server_default_port(void)
{
    return s_http_port;
}

int app_weblet_init(char *host_ip)
{
	//server_run = 1;
    GxCore_ThreadCreate("weblet_server",&server_threadid,weblet_serverThread,host_ip,64*1024,GXOS_DEFAULT_PRIORITY);
	return 0;
}

void app_weblet_uninit(void)
{
	server_run = 0;
	if (server_threadid && GxCore_ThreadJoin(server_threadid)) ERROR("pthread_join failed\n");
	server_threadid = 0;
}

static int _weblet_ui_control(AppMsg_WebletUIControl *msg)
{
	int ret = 0;
    if(msg == NULL)
        return -1;

    switch(msg->type)
    {
        case MSG_WEBLET_KEYBOARD_INPUT:
        {
			ret=app_kb_get_weblet_content_to_edit(msg->input);
            break;
        }
        case MSG_WEBLET_KEYBOARD_INPUT_CONFIRM:
        {
			ret=app_kb_get_weblet_content_to_edit_save(msg->input);
            break;
        }
		case MSG_WEBLET_GET_KEYBOARD_INPUT:
		{
			ret = app_kl_get_edit_content(msg->input);
			break;
		}
        case MSG_WEBLET_PLAY_PROGRAM:
        {
            ret = app_web_finder_play_count(msg->prog_id);
            break;
        }
        case MSG_WEBLET_CHANGE_GROUP:
        {
            ret = app_web_finder_change_group(msg->group_dir);
            break;
        }
        case MSG_WEBLET_REMOTE:
        {
            if(msg->keycode== STBK_BLUE || msg->keycode==STBK_OK )
            {
                s_kb_need_sync_num++;

            }
            Gui_WriteKeyBuf(msg->keycode);
            break;
        }
        case MSG_WEBLET_UI_CHECK:
        {
            ret=_weblet_switch_prog_or_group_uicheck(msg->mode);
            break;
        }
        default:
            break;
    }
	return ret;
}

int app_weblet_msg_proc(GxMessage *msg)
{
    AppMsg_WebletUIControl *ui_msg;
	int ret = -1;

    if(msg == NULL)
        return EVENT_TRANSFER_STOP;

    switch(msg->msg_id)
    {
        case APPMSG_WEBLET_UI_CONTROL:
        {
           ui_msg = GxBus_GetMsgPropertyPtr(msg, AppMsg_WebletUIControl);
           ret = _weblet_ui_control(ui_msg);
           ui_msg->result=ret;
           break;
        }
        default:
            break;
    }

    return ret;
}
#endif
