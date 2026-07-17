/*
    20240722
*/

#include "app_config.h"

#if RICHEPG_SUPPORT

#include "app.h"
#include "app_number.h"
#include "full_screen.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_eutel_common.h"

#define RICHEPG_DUMP_KEY            (9898)

static int thiz_eutel_tag_num = 0;

static int (*s_number_response_cb)(int num_value)=NULL;

void app_number_response_cb_set(int (*cb)(int num_value))
{
	s_number_response_cb = cb;
}

static int _app_eutel_number_exit_cb(int num_value)
{
	if(s_number_response_cb)
	{
		s_number_response_cb(num_value);
		return 1 ;
	}
	return 0 ;
}

static int _eutel_special_number_key_check(int num_value)
{
	if ( num_value == RICHEPG_DUMP_KEY )
		return 1 ;
	return 0 ;
}

static int _eutel_special_number_key_proc(int num_value)
{
	int ret = 0 ;
	switch(num_value)
	{
		case RICHEPG_DUMP_KEY:
		{
			extern int richepg_flash_file_dupall(void);
			PopDlg pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.mode = POP_MODE_UNBLOCK;
			pop.str = "Flash datas copying...";
			pop.create_cb = NULL;
			pop.exit_cb = NULL;
			pop.timeout_sec= 300;
			popdlg_create(&pop);
			GUI_SetInterface("flush", NULL);

			if (richepg_flash_file_dupall() == 0)
				pop.str = STR_ID_SUCCESS;
			else
				pop.str = "Fail!";
			pop.timeout_sec= 3;
			popdlg_create(&pop);
			GUI_SetInterface("flush", NULL);
			ret = 1 ;
		}
		break;

		default:
			break;
	}
	return ret ;
}

static int _eutel_number_key_exec(int32_t num_value)
{
	GxMsgProperty_NodeByPosGet tmp_node ;
	GxMsgProperty_NodeByPosGet node_prog_tmp ;
	int i = 0 ;
	int pos = -1;
	int prog_pos = -1;
	pos = eutel_channel_play_check_num(num_value);
	if (pos < 0)
		return -1 ;

	tmp_node.node_type = NODE_PROG;
	tmp_node.id = s_eutel_ch_ctrl.arg_array[pos].prog_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tmp_node);
	
	for(i = 0; i < g_AppPlayOps.normal_play.play_total; i++)
	{
		node_prog_tmp.node_type = NODE_PROG;
		node_prog_tmp.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog_tmp);
		if(node_prog_tmp.prog_data.id == tmp_node.prog_data.id)
		{
			prog_pos = i;
			break;
		}
	}

	if ( prog_pos < 0 )
		return -1 ;

#if SAT2IP_SERVER_SUPPORT
	extern int app_sat2ip_switch_tip_get_by_pos(int prog_pos);
	if (app_sat2ip_switch_tip_get_by_pos(prog_pos) < 0)
	{
		return GXCORE_SUCCESS;
	}
#endif
#if DVB2IP_SERVER_SUPPORT
	if (app_dvb2ip_switch_tip_get_by_pos(prog_pos) < 0)
	{
		return GXCORE_SUCCESS;
	}
#endif
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD))
	{
		return GXCORE_SUCCESS;
	}

	eutel_channel_play_by_num(PLAY_TYPE_NORMAL|PLAY_MODE_POINT,num_value);
	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

	return 0 ;
}

static int _eutel_number_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr ;
    cur_pvr = app_pvr_get_state();
    if(POP_VAL_OK == ret)
    {
        GUI_SetInterface("flush",NULL);
        if(cur_pvr == PVR_TIMESHIFT)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }
        _eutel_number_key_exec(thiz_eutel_tag_num);
    }
    return 0;
}

static int _eutel_number_play(int32_t num_value)
{
    int pos = 0 ;
    PopDlg pop = {0};

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_NO_PROGRAM;
        pop.timeout_sec = 1;
        popdlg_create(&pop);

        return -1;
    }

    pos = eutel_channel_play_check_num(num_value);
    if (pos < 0)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_NO_PROGRAM;
        pop.timeout_sec = 1;
        popdlg_create(&pop);

        return -1;
    }

    if ( s_eutel_ch_ctrl.arg_total <= pos )
        return -1;

    thiz_eutel_tag_num = num_value ;

    if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_eutel_number_stop_pvr_cb))
    {
        _eutel_number_key_exec(num_value);
    }

    return 1 ;
}

static int _app_eutel_number_response(unsigned int number_value)
{
	int ret = 0 ;

	if ( _eutel_special_number_key_check(number_value) )
	{
		_eutel_special_number_key_proc(number_value);
		return 2 ;
	}
	if ( _app_eutel_number_exit_cb(number_value) )
	{
		return 2 ;
	}

	if (app_richepg_mode_get())
	{
		_eutel_number_play(number_value);

		return 2 ;
	}
	else
	{
		ret = 1 ;
	}
	return ret ;
}

int  app_eutel_number_ops_init(void)
{
	app_number_respones_ex_func_register(_app_eutel_number_response);

	return 0 ;
}

#endif

