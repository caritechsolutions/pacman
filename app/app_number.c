/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_full_screen.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.3.8	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app.h"
#include "app_eutel_porting.h"
#include "app_module.h"
#include "app_msg.h"
#include "gui_core.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_fuse_ops.h"
#include "app_number.h"
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif
#if LCN_SUPPORT
#include "app_lcn.h"
#endif

#define TEXT_NUMBER	"txt_number"
#define PROG_NUM_MAX  (4)

#if CMM_SUPPORT
#define CMB_MENU_KEY         (7788)
#define CMP_MENU_KEY         (7799)
#endif

#define KEY_SCANCODE_KEY       (7789)
#define MERGE_DB_KEY           (7790)

#define TP_RECORD_KEY           (7791)

#define FACTORY_TEST_MENU_KEY  (9999)
#define CVBS_TEST_MENU_KEY     (6088)
#define LOG_SET_MENU_KEY       (8765)
#if USB_MICROPHONE_SUPPORT
#define EN_USB_MIKE_MENU_KEY   (8808)
#define DIS_USB_MIKE_MENU_KEY  (8809)
#define VOICE_USB_MIKE_MENU_KEY  (8810)
#define VOICE_USB_MIKE_MUTE_KEY  (8811)
#endif
/* static variable ------------------------------------------------------ */
static event_list * sp_TimerNumber = NULL;
static char key_buf[PROG_NUM_MAX + 1];
static uint32_t key_num = 0;
static pvr_state thiz_number_cur_pvr = PVR_DUMMY;
static GxMsgProperty_NodeByPosGet thiz_node_prog_tag = {0};
static int thiz_tag_pos = 0;

/* Private functions ------------------------------------------------------ */

static _ex_number_response_func  s_func_ex_number_response = NULL;
// 新增外部函数
void app_number_respones_ex_func_register(int (*response_func)(unsigned int number_value))
{
    if(response_func)
        s_func_ex_number_response = response_func;
}

_ex_number_response_func app_number_respones_ex_func_get(void)
{
    return s_func_ex_number_response;
}

void number_set(unsigned short key_value)
{
	switch (key_value)
	{
		case STBK_1:
			strcat(key_buf, "1");
			break;
		case STBK_2:
			strcat(key_buf, "2");
			break;
		case STBK_3:
			strcat(key_buf, "3");
			break;
		case STBK_4:
			strcat(key_buf, "4");
			break;
		case STBK_5:
			strcat(key_buf, "5");
			break;
		case STBK_6:
			strcat(key_buf, "6");
			break;
		case STBK_7:
			strcat(key_buf, "7");
			break;
		case STBK_8:
			strcat(key_buf, "8");
			break;
		case STBK_9:
			strcat(key_buf, "9");
			break;
		case STBK_0:
			strcat(key_buf, "0");
			break;
		default:
			break;
	}
}

#if TKGS_SUPPORT
int app_t2_lcn_num_switch(int32_t num)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};
	int i = 0;
	PopDlg pop = {0};
	bool find_logicnum = false;
	#if TKGS_SUPPORT
	AppProgExtInfo prog_ext_info = {{0}};
	#endif

	for(i=0;i<g_AppPlayOps.normal_play.play_total;i++)
	{
		node_prog.node_type = NODE_PROG;
		node_prog.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
		#if TKGS_SUPPORT
		if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
		{
			if(num == prog_ext_info.tkgs.logicnum)
			{
				find_logicnum = true;
				return i;
			}
		}
		#else
		if(num == node_prog.prog_data.logicnum)
		{
			find_logicnum = true;
			return i;
		}
		#endif
	}

	if(!find_logicnum)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_NO_PROGRAM;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
		return GXCORE_ERROR;
	}

    return 0;
}
#endif

static bool number_special_key_check(int num_value)
{
	if(0
#if CMM_SUPPORT
            ||(num_value == CMB_MENU_KEY)
            ||(num_value == CMP_MENU_KEY)
#endif
#if FACTORY_TEST_SUPPORT
                ||(num_value == FACTORY_TEST_MENU_KEY)
#endif
#if (CVBS_TEST_SUPPORT || SMART_VOLUME_TEST_SUPPORT)
                ||(num_value == CVBS_TEST_MENU_KEY)
#endif
#if USB_MICROPHONE_SUPPORT
                ||(num_value == EN_USB_MIKE_MENU_KEY)
                ||(num_value == DIS_USB_MIKE_MENU_KEY)
                ||(num_value == VOICE_USB_MIKE_MENU_KEY)
                ||(num_value == VOICE_USB_MIKE_MUTE_KEY)
#endif
#if KEY_SCANCODE_SUPPORT
                || (num_value == KEY_SCANCODE_KEY)
#endif
#if MERGE_DB_SUPPORT
                || (num_value == MERGE_DB_KEY)
#endif
                || (num_value == TP_RECORD_KEY)
			)
		return true;
	else
		return false;
}

static int _number_timeout_key_exec(void)
{
    GxMsgProperty_NodeByPosGet node_prog_tmp = {0};
    int32_t num_value;
    int i;

    num_value = atoi(key_buf);
    switch(num_value)
    {
#if CMM_SUPPORT
        case CMB_MENU_KEY:
            {
                extern void app_cmb_create_exec(void);
                g_AppFullArb.timer_stop();
                app_cmb_create_exec();
                break;
            }

        case CMP_MENU_KEY:
            {
                break;
            }
#endif
#if FACTORY_TEST_SUPPORT
        case FACTORY_TEST_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                g_AppPlayOps.program_stop();
                extern void app_factory_test_menu_exec(void);
                app_factory_test_menu_exec();
                break;
            }
#endif
#if CVBS_TEST_SUPPORT
        case CVBS_TEST_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                extern void app_cvbs_test_menu_exec(void);
                app_cvbs_test_menu_exec();
                break;
            }
#else
#if SMART_VOLUME_TEST_SUPPORT
        case CVBS_TEST_MENU_KEY:
        {
            extern void app_smart_volume_test_menu_exec(void);
            app_smart_volume_test_menu_exec();
            break;
        }
#endif
#endif
#if USB_MICROPHONE_SUPPORT
        case EN_USB_MIKE_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                extern void app_mircophone_voice_start(void);
                app_mircophone_voice_start();
                break;
            }
        case DIS_USB_MIKE_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                extern void app_mircophone_voice_stop(void);
                app_mircophone_voice_stop();
                break;
            }
        case VOICE_USB_MIKE_MENU_KEY:
            {
                g_AppFullArb.timer_stop();
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                extern void app_create_mike_menu(void);
                app_create_mike_menu();
                break;
            }
        case VOICE_USB_MIKE_MUTE_KEY:
            {
                extern int app_microphone_set_mute(int mute);
                extern int app_microphone_get_mute_flag(void);
                int value = app_microphone_get_mute_flag();
                app_microphone_set_mute(value);
                break;
            }
#endif
#if KEY_SCANCODE_SUPPORT
        case KEY_SCANCODE_KEY:
            {
                g_AppFullArb.timer_stop();
                GUI_CreateDialog(WND_KEY_SCANCODE);
                break;
            }
#endif

#if MERGE_DB_SUPPORT
        case MERGE_DB_KEY:
            {
                g_AppFullArb.timer_stop();
                GUI_CreateDialog(WND_MERGE_DB);
                break;
            }
#endif

#if PVR_RECORD_TP_SUPPORT
        case TP_RECORD_KEY:
            {
                usb_check_state usb_state = USB_OK;

                if (g_AppPlayOps.normal_play.play_total == 0)
                    break;

                if(g_AppPvrOps.state == PVR_DUMMY)
                {
                    PopDlg pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.create_cb = NULL;
                    pop.exit_cb = NULL;
                    pop.timeout_sec= 3;

                    usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
                    if(usb_state == USB_ERROR)
                    {
                        pop.str = STR_ID_INSERT_USB;
                        popdlg_create(&pop);
                        break;
                    }
                    else if(usb_state == USB_NO_SPACE)
                    {
                        pop.str = STR_ID_NO_SPACE;
                        popdlg_create(&pop);
                        break;
                    }
                    else if(usb_state == USB_READ_ONLY)
                    {
                        pop.str = STR_ID_NOWRITE_PERMISSIONS;
                        popdlg_create(&pop);
                        break;
                    }

                    app_tp_rec_start(0);
                    GUI_CreateDialog(WND_PVR_BAR);
                    GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                }
                else
                {
                    app_pvr_create_no_btn_popdlg(NULL);
                    break;
                }
            }
            break;
#endif
        default:
            {
                if(thiz_number_cur_pvr == PVR_RECORD)
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

#if SAT2IP_SERVER_SUPPORT
                extern int app_sat2ip_switch_tip_get_by_pos(int prog_pos);
                if (app_sat2ip_switch_tip_get_by_pos(thiz_tag_pos) < 0)
                {
                    return GXCORE_SUCCESS;
                }
#endif
#if DVB2IP_SERVER_SUPPORT
                if (app_dvb2ip_switch_tip_get_by_pos(thiz_tag_pos) < 0)
                {
                    return GXCORE_SUCCESS;
                }
#endif
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD))
                {
                    return GXCORE_SUCCESS;
                }

                // player prog by pos, pos from 0 and num from 1, so need del 1
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, thiz_tag_pos);
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
#if HDMI_CEC_SUPPORT
				extern int app_cec_change_service(void);
	            app_cec_change_service();
#endif
                break;
            }
    }
    return 0;
}

static int _number_stop_pvr_cb(PopDlgRet ret)
{
    thiz_number_cur_pvr = app_pvr_get_state();
    if(POP_VAL_OK == ret)
    {
        GUI_SetInterface("flush",NULL);
        if(thiz_number_cur_pvr == PVR_TIMESHIFT)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }
        _number_timeout_key_exec();
    }
    return 0;
}

#if LCN_SUPPORT
int app_lcn_num_switch(int32_t num)
{
	GxMsgProperty_NodeByPosGet node_prog = {0};
    AppProgExtInfo prog_ext_info;

	int i = 0;
	PopDlg pop = {0};
	bool find_logicnum = false;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));

	for(i=0;i<g_AppPlayOps.normal_play.play_total;i++)
	{
		node_prog.node_type = NODE_PROG;
		node_prog.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
        if (GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(node_prog.prog_data.id,sizeof(AppProgExtInfo),&prog_ext_info))
        {
            if(num ==prog_ext_info.logicnum)
    		{
    			find_logicnum = true;
    			return i;
    		}
	    }
	}

	if(!find_logicnum)
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_NO_BTN;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_NO_PROGRAM;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
		return GXCORE_ERROR;
	}

    return 0;
}
#endif

static int app_number_response(void)
{
    int32_t num_value;
    PopDlg pop = {0};

    // get number
    num_value = atoi(key_buf);
    thiz_number_cur_pvr = PVR_DUMMY;
    thiz_tag_pos = 0;
    memset(&thiz_node_prog_tag, 0, sizeof(GxMsgProperty_NodeByPosGet));

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NUMBER))
    {
        GUI_EndDialog(WND_NUMBER);
        GUI_SetProperty(WND_NUMBER, "draw_now", NULL);
    }

    if (!number_special_key_check(num_value))
    {
        // 扩展功能，自定义数字按键响应
        if(s_func_ex_number_response)
        {
            if(1 != s_func_ex_number_response(num_value))
            {
                return 0;
            }
        }
    }
#if FM_SUPPORT
    extern void app_fm_number_response(int num);
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_FM_PLAY_INFO))
    {
        app_fm_number_response(num_value);

        return 0;
    }
#endif
#if TKGS_SUPPORT
    if(num_value == 0)
        return GXCORE_ERROR;

    if(!number_special_key_check(num_value))
    {
        if(GX_TKGS_OFF != app_tkgs_get_operatemode())
        {
            thiz_tag_pos = app_t2_lcn_num_switch(num_value);
            if(thiz_tag_pos == GXCORE_ERROR)
            {
                return GXCORE_ERROR;
            }
        }
        else
        {
            if (num_value > g_AppPlayOps.normal_play.play_total)
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_NO_PROGRAM;
                pop.timeout_sec = 3;

                popdlg_create(&pop);

                return GXCORE_ERROR;
            }
            thiz_tag_pos = num_value - 1;
        }
    }
#elif LCN_SUPPORT
    if(1 == app_lcn_config_get())
     {
          thiz_tag_pos = app_lcn_num_switch(num_value);
          if (thiz_tag_pos == GXCORE_ERROR)
          {
              return GXCORE_ERROR;
          }
    }
    else
    {
        if (num_value > g_AppPlayOps.normal_play.play_total || num_value == 0)
        {
            if(number_special_key_check(num_value))
            {
                ;
            }
            else
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_NO_PROGRAM;
                pop.timeout_sec = 3;

                popdlg_create(&pop);

                return GXCORE_ERROR;
            }
        }
        else
        {
            thiz_tag_pos = num_value - 1;
        }
    }
#else
    if (num_value > g_AppPlayOps.normal_play.play_total || num_value == 0)
    {
        if(number_special_key_check(num_value))
        {
            ;
        }
        else
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_NO_PROGRAM;
            pop.timeout_sec = 3;

            popdlg_create(&pop);

            return GXCORE_ERROR;
        }
    }
	else
	{
        thiz_tag_pos = num_value - 1;
    }
#endif

    if(number_special_key_check(num_value))
    {
        ;
    }
    else
    {
        thiz_node_prog_tag.node_type = NODE_PROG;
        thiz_node_prog_tag.pos = thiz_tag_pos;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &thiz_node_prog_tag);
    }

    if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_number_stop_pvr_cb))
    {
        _number_timeout_key_exec();
    }
    return GXCORE_SUCCESS;

}

static int number_timeout(void* data)
{
    sp_TimerNumber = NULL;

    app_number_response();
    return GXCORE_SUCCESS;
}

/* Exported functions ----------------------------------------------------- */
SIGNAL_HANDLER int app_number_create(const char* widgetname, void *usrdata)
{
	if (reset_timer(sp_TimerNumber) != 0)
	{
		sp_TimerNumber = create_timer(number_timeout, 1500, NULL, TIMER_ONCE);
	}

	key_num = 0;
	memset(key_buf, 0, PROG_NUM_MAX + 1);
    memset(&thiz_node_prog_tag, 0, sizeof(GxMsgProperty_NodeByPosGet));

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_number_destroy(const char* widgetname, void *usrdata)
{
	remove_timer(sp_TimerNumber);
	sp_TimerNumber = NULL;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_number_keypress(const char* widgetname, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	if(GUI_KEYDOWN ==  event->type)
	{
		switch(event->key.sym)
		{
			case STBK_1:
			case STBK_2:
			case STBK_3:
			case STBK_4:
			case STBK_5:
			case STBK_6:
			case STBK_7:
			case STBK_8:
			case STBK_9:
			case STBK_0:
				reset_timer(sp_TimerNumber);
				if (key_num >= PROG_NUM_MAX)
				{
					key_num = 0;
					memset(key_buf, 0, PROG_NUM_MAX + 1);
				}

				key_num++;
				number_set(event->key.sym);
				GUI_SetProperty(TEXT_NUMBER, "string", key_buf);
				break;

			case STBK_OK:
                app_number_response();
                break;
			case STBK_EXIT:
			case VK_BOOK_TRIGGER:
				GUI_EndDialog(WND_NUMBER);
				break;
			default:
				break;
		}
	}
	return EVENT_TRANSFER_STOP;
}

#if VOICE_CONTROL_SUPPORT
int app_vc_number_switch(unsigned int num, char* name, unsigned int name_len)
{
    int ret = 0;
    unsigned int len = 0;
    memset(key_buf, 0, sizeof(key_buf));
    sprintf(key_buf,"%d", num);
    ret = app_number_response();
    if((0 == ret) && (0 < name_len) && (NULL != name))
    {// get channel name
        if(g_AppPlayOps.normal_play.play_total > 0)
        {
            GxMsgProperty_NodeByPosGet node = {0};
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
            {
                len = strlen((const char*)(node.prog_data.prog_name));
                len = name_len > len?len:name_len;
                memcpy(name, node.prog_data.prog_name, len);
                return 0;
            }
        }
        ret = -1;
    }
    return ret;
}
#endif

/* End of file -------------------------------------------------------------*/


