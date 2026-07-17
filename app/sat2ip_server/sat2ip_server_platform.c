#include "sat2ip_server_platform.h"
#if SAT2IP_SERVER_SUPPORT
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "module/channel_edit.h"
#include "app_windows.h"
#include "module/app_ioctl.h"

#ifdef LINUX_OS
#define SATIP_HTTP_SERVER_MAX_CLIENTS   16      // 文件服务器最大监听socket, 不要设为0
#define SATIP_RTSP_STREAM_MAX_CLIENTS   8       // rtsp流服务器的最大客户端数目，不要设为0
#define SATIP_HTTP_STREAM_MAX_CLIENTS   8       // http流服务器的最大客户端数目, 不要设为0
#else
#define SATIP_HTTP_SERVER_MAX_CLIENTS   8
#define SATIP_RTSP_STREAM_MAX_CLIENTS   4
#define SATIP_HTTP_STREAM_MAX_CLIENTS   4
#endif
#define SATIP_MAX_DATA_CNT              1000    // sat2ip库数据分发缓存数目(每个数据大小1316)，不要小于100

#define SATIP_MAX_REC_SIZE_LOW          564000  // 低优先级下player录制缓存大小，188的倍数 (188*3*1000)
#define SATIP_MAX_REC_SIZE_HIGH         1128000 // 高优先级下player录制缓存大小，188的倍数 (188*6*1000)
#define PLAYER_SAT2IP                   "sat2ip"

typedef struct {
	uint8_t use_flag;
	uint8_t prio_flag;
	uint8_t switch_flag;
	uint8_t rtsp_flag;
	uint8_t client_flag;
	uint8_t remove_flag;
	uint8_t build_flag;

	uint8_t net_ok;
	uint8_t gui_ok;
	uint8_t dlg_ok;
	uint8_t start_ok; // service start 返回状态，很特殊
	uint8_t play_flag;
	int total_prog;
	int ign_sec;
	char ip_addr[SATIP_IPADDR_LEN];
	GxBusPmDataProg prog;
	event_list* start_timer;
} SatipArg;

SatipArg s_satip_arg = {
	.use_flag = SATIP_USE_FLAG_DEFAULT,
	.prio_flag = SATIP_PRIO_FLAG_DEFAULT,
	.switch_flag = SATIP_SWITCH_FLAG_DEFAULT,
	.rtsp_flag = SATIP_RTSP_FLAG_DEFAULT,
	.client_flag = SATIP_CLIENT_FLAG_DEFAULT,
	.remove_flag = SATIP_EXIT_REMOVE_FILES_DEFAULT,
	.build_flag = 1,
	.net_ok = 0,
	.gui_ok = 0,
	.dlg_ok = 1,
	.start_ok = 0,
	.play_flag = 0,
	.total_prog = 0,
	.ign_sec = 30,
	.ip_addr = {0},
	.prog = {0},
	.start_timer = NULL
};

static int app_sat2ip_record_start(GxBusPmDataProg *prog)
{
	SatipArg *arg = &s_satip_arg;
	char play_url[PLAYER_URL_LONG] = {0};
	char dest_url[100] = {0};
	PlayerRecordConfig *config = NULL;
	int rec_size = 0;

	//uint16_t ts_src = g_AppPlayOps.normal_play.ts_src;
	//uint16_t dmx_id = (ts_src == 3) ? 0 : 1;

	if ((config = (PlayerRecordConfig*)GxCore_Calloc(1, sizeof(PlayerRecordConfig))) == NULL)
	{
		SATIP_ERR("malloc failed!");
		return -1;
	}
	extern void app_player_record_config_get(int prog_id, PlayerRecordConfig *config);
	app_player_record_config_get(prog->id, config);

	AppFrontend_Config frontend_cfg = {0};
	app_ioctl(prog->tuner, FRONTEND_CONFIG_GET, &frontend_cfg);
	extern void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id);
	app_player_url_get(play_url, prog, frontend_cfg.ts_src, frontend_cfg.dmx_id);

	rec_size = (arg->prio_flag) ? SATIP_MAX_REC_SIZE_HIGH : SATIP_MAX_REC_SIZE_LOW;
	snprintf(dest_url, sizeof(dest_url), "ringmem://id:%d&size:%d&", prog->id, rec_size);
#if 1
	extern status_t GxPlayer_MediaRecord2(const char* name, const char* srcurl, const char* file, PlayerRecordConfig* config);
	if (GxPlayer_MediaRecord2(PLAYER_SAT2IP, play_url, dest_url, config) != GXCORE_SUCCESS)
	{
		SATIP_ERR("record failed!\n");
		GxCore_Free(config);
		return -1;
	}
#else
	extern status_t GxPlayer_MediaRecord(const char* name, const char* srcurl, const char* file);
	if (GxPlayer_MediaRecord(PLAYER_SAT2IP, play_url, dest_url) != GXCORE_SUCCESS)
	{
		SATIP_ERR("record failed!\n");
		GxCore_Free(config);
		return -1;
	}
#endif

	SATIP_INFO("sat2ip record start success!\n");

	arg->play_flag = 1;
	GxCore_Free(config);
	return 0;
}

static int app_sat2ip_record_stop(void)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->play_flag)
	{
		//GxPlayer_MediaStopRecord(PLAYER_SAT2IP);
		GxPlayer_MediaStop(PLAYER_SAT2IP);
		arg->play_flag = 0;
	}

	SATIP_INFO("sat2ip record stop success!\n");
	return 0;
}

static int app_sat2ip_record_read(uint8_t *buf, int size)
{
	int len = 0;
	SatipArg *arg = &s_satip_arg;
	if (arg->play_flag)
	{
		len = GxPlayer_MediaRead(PLAYER_SAT2IP, PLAYER_READ_DUMPER, buf, size);
		//SATIP_DBG("sat2ip record read size = %d!\n", len);
		return len;
	}

	return 0;
}

static void _service_start(void)
{
	SatipArg *arg = &s_satip_arg;
	SatipStartMsg msg;

	memcpy(msg.ip_addr, arg->ip_addr, sizeof(msg.ip_addr));
	msg.build_flag = arg->build_flag;
	app_send_msg_exec(GXMSG_SATIP_SERVICE_START, &msg);
	arg->start_ok = msg.start_ok;
	arg->build_flag = 0;
}

static void _service_stop(void)
{
	SatipArg *arg = &s_satip_arg;
	int msg;

	msg = arg->remove_flag;
	app_send_msg_exec(GXMSG_SATIP_SERVICE_STOP, &msg);
	arg->start_ok = 0;
}

void app_sat2ip_arg_init(void)
{
	SatipArg *arg = &s_satip_arg;
	int init_value = 0;
	int use_flag = 0;

	GxBus_ConfigGetInt(SATIP_USE_FLAG_KEY, &init_value, SATIP_USE_FLAG_DEFAULT);
	use_flag = init_value;
	GxBus_ConfigGetInt(SATIP_PRIO_FLAG_KEY, &init_value, SATIP_PRIO_FLAG_DEFAULT);
	arg->prio_flag = (uint8_t)init_value;
	app_satip_data_prio_change(init_value);
	GxBus_ConfigGetInt(SATIP_SWITCH_FLAG_KEY, &init_value, SATIP_SWITCH_FLAG_DEFAULT);
	arg->switch_flag = (uint8_t)init_value;
	GxBus_ConfigGetInt(SATIP_RTSP_FLAG_KEY, &init_value, SATIP_RTSP_FLAG_DEFAULT);
	arg->rtsp_flag = (uint8_t)init_value;
	app_satip_dms_rtsp_use_change(init_value);
	GxBus_ConfigGetInt(SATIP_CLIENT_FLAG_KEY, &init_value, SATIP_CLIENT_FLAG_DEFAULT);
	arg->client_flag = (uint8_t)init_value;
	app_satip_multi_client_flag_change(init_value);
	GxBus_ConfigGetInt(SATIP_EXIT_REMOVE_FILES_KEY, &init_value, SATIP_EXIT_REMOVE_FILES_DEFAULT);
	arg->remove_flag = (uint8_t)init_value;
	app_sat2ip_use_flag_set(use_flag);
}

int app_sat2ip_use_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->use_flag;
}

void app_sat2ip_use_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;

#if DVB2IP_SERVER_SUPPORT
	if (flag)
	{
		extern void app_dvb2ip_use_flag_set(int flag);
		app_dvb2ip_use_flag_set(0);
	}
#endif
	if (arg->use_flag == flag)
		return;
	arg->use_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_USE_FLAG_KEY, flag);
	if (flag)
	{
		arg->build_flag = 1;
		if (arg->net_ok && arg->gui_ok && arg->total_prog > 0)
		{
			_service_start();
		}
	}
	else
	{
		if (arg->net_ok && arg->gui_ok && arg->total_prog > 0)
		{
			_service_stop();
		}
	}
}

int app_sat2ip_prio_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->prio_flag;
}

void app_sat2ip_prio_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->prio_flag == flag)
		return;

	arg->prio_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_PRIO_FLAG_KEY, flag);
	app_satip_data_prio_change(flag);
	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag)
	{
		_service_stop();
		_service_start();
	}
}

int app_sat2ip_switch_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->switch_flag;
}

void app_sat2ip_switch_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->switch_flag == flag)
		return;

	arg->switch_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_SWITCH_FLAG_KEY, flag);
}

int app_sat2ip_rtsp_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->rtsp_flag;
}

void app_sat2ip_rtsp_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->rtsp_flag == flag)
		return;

	arg->rtsp_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_RTSP_FLAG_KEY, flag);
	app_satip_dms_rtsp_use_change(flag);
	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag)
	{
		_service_stop();
		_service_start();
	}
}

int app_sat2ip_client_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->client_flag;
}

void app_sat2ip_client_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->client_flag == flag)
		return;

	arg->client_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_CLIENT_FLAG_KEY, flag);
	app_satip_multi_client_flag_change(flag);
}

int app_sat2ip_remove_flag_get(void)
{
	SatipArg *arg = &s_satip_arg;
	return arg->remove_flag;
}

void app_sat2ip_remove_flag_set(int flag)
{
	SatipArg *arg = &s_satip_arg;
	if (arg->remove_flag == flag)
		return;

	arg->remove_flag = (uint8_t)flag;
	GxBus_ConfigSetInt(SATIP_EXIT_REMOVE_FILES_KEY, flag);

	if (flag)
	{
		if (!(arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag))
		{
			arg->build_flag = 1;
			satip_clear_cache_files();
		}
	}
}

void app_sat2ip_update_prog_total(void)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;
	int total = satip_proglist_total_num_get();

	if (total < 0)
		total = 0;

	if (total)
	{
		if (arg->net_ok && arg->gui_ok && arg->use_flag)
			flag = true;

		if (arg->total_prog == 0)
			g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

		arg->total_prog = total;
		arg->build_flag = 1;
		if (flag)
		{
			_service_stop();
			_service_start();
		}
	}
	else
	{
		if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag)
			flag = true;

		arg->total_prog = total;
		if (flag)
		{
			_service_stop();
		}
	}
}

void app_sat2ip_set_net_ok(const char* ip_addr)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;

	if ((strcmp(arg->ip_addr, ip_addr) || !arg->net_ok || !arg->start_ok)
			&& arg->gui_ok && arg->total_prog > 0 && arg->use_flag)
		flag = true;

	memset(arg->ip_addr, 0, sizeof(ip_addr));
	strncpy(arg->ip_addr, ip_addr, sizeof(arg->ip_addr)-1);
	arg->net_ok = 1;

	if (flag)
	{
		_service_stop();
		_service_start();
	}
}

void app_sat2ip_unset_net_ok(void)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;

	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag && arg->start_ok)
		flag = true;

	arg->net_ok = 0;

	if (flag)
		_service_stop();
}

void app_sat2ip_set_gui_ok(void)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;

	if (arg->net_ok && (!arg->gui_ok || !arg->start_ok) && arg->total_prog > 0 && arg->use_flag)
		flag = true;

	arg->gui_ok = 1;

	if (flag)
		_service_start();
}

void app_sat2ip_unset_gui_ok(void)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;

	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag && arg->start_ok)
		flag = true;

	arg->gui_ok = 0;

	if (flag)
	{
		_service_stop();
	}
}

void app_sat2ip_unset_gui_ok_fast(void)
{
	SatipArg *arg = &s_satip_arg;
	bool flag = false;

	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag && arg->start_ok)
		flag = true;

	arg->gui_ok = 0;

	if (flag)
	{
		app_satip_fast_close_set(1);
		_service_stop();
		while (app_satip_task_state_get())
			GxCore_ThreadDelay(10);
	}
}

void app_sat2ip_set_dlg_ok(void)
{
	SatipArg *arg = &s_satip_arg;

	if (!arg->dlg_ok)
	{
		arg->dlg_ok = 1;
		app_send_msg_exec(GXMSG_SATIP_DIALOG_START, NULL);
	}
}

void app_sat2ip_unset_dlg_ok(void)
{
	SatipArg *arg = &s_satip_arg;

	if (arg->dlg_ok)
	{
		arg->gui_ok = 0;
		app_send_msg_exec(GXMSG_SATIP_DIALOG_STOP, NULL);
	}
}

void app_sat2ip_set_ignore_sec(int sec)
{
	SatipArg *arg = &s_satip_arg;

	arg->ign_sec = sec;
}

void app_sat2ip_set_dialog_pause(void)
{
	SatipArg *arg = &s_satip_arg;

	app_send_msg_exec(GXMSG_SATIP_DIALOG_PAUSE, &arg->ign_sec);
	APP_TIMER_REMOVE(arg->start_timer);
}

void app_sat2ip_set_dialog_resume(void)
{
	app_send_msg_exec(GXMSG_SATIP_DIALOG_RESUME, NULL);
}

static int app_sat2ip_play_start_exec(void *userdata)
{
	SatipArg *arg = &s_satip_arg;
	int prog_pos = 0;

	// 需要等待阻塞框退出
	extern void book_popdlg_exec(PopDlgRet ret);
	extern void app_trigger_end_popdlg(void);
	extern bool app_trigger_check_popdlg(void);

	book_popdlg_exec(POP_VAL_CANCEL);
	if (app_trigger_check_popdlg())
	{
		app_trigger_end_popdlg();
		return 0;
	}
	APP_TIMER_REMOVE(arg->start_timer);

	extern int app_trigger_end_window(void);
	if (app_trigger_end_window() == GXCORE_ERROR)
	{
		SATIP_ERR("can not play in the win!\n");
		return 0;
	}
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

	if(g_AppPvrOps.state != PVR_DUMMY)
	{
		app_pvr_stop();
		extern void app_set_avcodec_flag(bool state);
		app_set_avcodec_flag(false);
	}
	if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_PASSWD);
	}

	if (app_get_prog_pos_by_id_all(arg->prog.id, &prog_pos) < 0)
	{
		SATIP_ERR("can not find prog!\n");
		return 0;
	}

	g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, prog_pos);

	g_AppFullArb.state.tv = (arg->prog.service_type == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
	g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
	g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

	if (app_sat2ip_record_start(&arg->prog) == 0)
	{
		if (GUI_CheckDialog(WND_PASSWD) != GXCORE_SUCCESS)
		{
			PopDlg pop = {0};

			popdlg_destroy();
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.str = STR_ID_SAT2IP_PLAY;
			pop.mode = POP_MODE_UNBLOCK;
			pop.timeout_sec = 3;
			pop.exit_cb = NULL;
			popdlg_create(&pop);
		}
	}

	return 0;
}

void app_sat2ip_play_start(int prog_id)
{
	SatipArg *arg = &s_satip_arg;
	if (prog_id > 0)
	{
		GxMsgProperty_NodeByIdGet prog_node = {0};

		prog_node.node_type = NODE_PROG;
		prog_node.id = prog_id;
		if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
		{
			SATIP_ERR("can not find prog!\n");
			return;
		}
		memcpy(&arg->prog, &prog_node.prog_data, sizeof(GxBusPmDataProg));
		app_sat2ip_record_stop();
		APP_TIMER_ADD(arg->start_timer, app_sat2ip_play_start_exec, 10, TIMER_REPEAT);
	}
}

void app_sat2ip_play_stop(int prog_id)
{
	SatipArg *arg = &s_satip_arg;

	APP_TIMER_REMOVE(arg->start_timer);
	app_sat2ip_record_stop();
	memset(&arg->prog, 0, sizeof(GxBusPmDataProg));
}

bool app_sat2ip_check_playing(void)
{
	SatipArg *arg = &s_satip_arg;

	return (bool)arg->play_flag;
}

int app_sat2ip_play_change(GxBusPmDataProg *prog)
{
	// 切台后直接推新节目的流，vlc会无法播放, 报pid错误, kodi有时可以
#if 0
	SatipArg *arg = &s_satip_arg;
	if (!prog)
		return -1;
	if (arg->play_flag && arg->switch_flag)
	{
		if (arg->prog.tp_id != prog->tp_id)
		{
			memcpy(&arg->prog, prog, sizeof(GxBusPmDataProg));
			app_sat2ip_record_stop();
			app_sat2ip_record_start(prog);
		}
	}
#endif
	return 0;
}

static void _sat2ip_switch_tip(void)
{
	PopDlg pop = {0};

	popdlg_destroy();
	pop.type = POP_TYPE_NO_BTN;
	pop.format = POP_FORMAT_DLG;
#if SAT2IP_CONFIGURE_SUPPORT
	pop.str = STR_ID_SAT2IP_STOP_TIP1;
#else
	pop.str = STR_ID_SAT2IP_STOP_TIP2;
#endif
	pop.mode = POP_MODE_UNBLOCK;
	pop.timeout_sec = 3;
	pop.exit_cb = NULL;
	popdlg_create(&pop);
}

int app_sat2ip_switch_tip_get_by_prog(GxBusPmDataProg *prog)
{
	SatipArg *arg = &s_satip_arg;

	if (arg->play_flag && !arg->switch_flag)
	{
		if (arg->prog.tp_id != prog->tp_id && arg->prog.tuner == prog->tuner)
		{
			_sat2ip_switch_tip();
			return -1;
		}
	}
	return 0;
}

int app_sat2ip_switch_tip_get_by_pos(int prog_pos)
{
	GxMsgProperty_NodeByPosGet prog_node = {0};

	prog_node.node_type = NODE_PROG;
	prog_node.pos = prog_pos;
	if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
	{
		SATIP_ERR("can not find prog!\n");
		return 0;
	}

	return app_sat2ip_switch_tip_get_by_prog(&prog_node.prog_data);
}

int app_sat2ip_switch_tip_get_by_id(int prog_id)
{
	GxMsgProperty_NodeByIdGet prog_node = {0};

	prog_node.node_type = NODE_PROG;
	prog_node.id = prog_id;
	if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
	{
		SATIP_ERR("can not find prog!\n");
		return 0;
	}

	return app_sat2ip_switch_tip_get_by_prog(&prog_node.prog_data);
}

int app_sat2ip_switch_tip_get_force(void)
{
	SatipArg *arg = &s_satip_arg;

	if (arg->play_flag && !arg->switch_flag)
	{
		_sat2ip_switch_tip();
		return -1;
	}

	return 0;
}

int app_sat2ip_operate_tip_get_force(void)
{
	SatipArg *arg = &s_satip_arg;

	if (arg->net_ok && arg->gui_ok && arg->total_prog > 0 && arg->use_flag && !arg->switch_flag)
	{
		_sat2ip_switch_tip();
		return -1;
	}

	return 0;
}

// service
static int app_satip_lock_state_get(int tuner, int *lock, int *quality)
{
	AppFrontend_LockState lock_tmp;
	int quality_tmp;

	app_ioctl(tuner, FRONTEND_LOCK_STATE_GET, &lock_tmp);
	*lock = (lock_tmp == FRONTEND_LOCKED) ? 1 : 0;

	app_ioctl(tuner, FRONTEND_QUALITY_GET, &quality_tmp);
	if (quality_tmp > 99)
		quality_tmp = 100;
	*quality = quality_tmp;

	return 0;
}

static int app_satip_play_start_msg_send(int prog_id)
{
	app_send_msg_exec(GXMSG_SATIP_PLAY_START, &prog_id);
	return 0;
}

static int app_satip_play_stop_msg_send(int prog_id)
{
	app_send_msg_exec(GXMSG_SATIP_PLAY_STOP, &prog_id);
	return 0;
}

status_t AppSatipServiceInit(handle_t self, int priority_offset)
{
	SatipCallback callback;

	callback.max_http_server = SATIP_HTTP_SERVER_MAX_CLIENTS;
	callback.max_rtsp_stream = SATIP_RTSP_STREAM_MAX_CLIENTS;
	callback.max_http_stream = SATIP_HTTP_STREAM_MAX_CLIENTS;
	callback.max_data_cnt = SATIP_MAX_DATA_CNT;

	callback.lock_state_get = app_satip_lock_state_get;
	callback.start_play_send = app_satip_play_start_msg_send;
	callback.stop_play_send = app_satip_play_stop_msg_send;
	callback.record_read = app_sat2ip_record_read;

	app_satip_ctrl_init(&callback);

	GxBus_MessageRegister(GXMSG_SATIP_SERVICE_START, sizeof(SatipStartMsg));
	GxBus_MessageRegister(GXMSG_SATIP_SERVICE_STOP, sizeof(int));
	GxBus_MessageRegister(GXMSG_SATIP_DIALOG_START, 0);
	GxBus_MessageRegister(GXMSG_SATIP_DIALOG_STOP, 0);
	GxBus_MessageRegister(GXMSG_SATIP_DIALOG_PAUSE, sizeof(int));
	GxBus_MessageRegister(GXMSG_SATIP_DIALOG_RESUME, 0);

	GxBus_MessageRegister(GXMSG_SATIP_PLAY_START, sizeof(int));     // gui接收
	GxBus_MessageRegister(GXMSG_SATIP_PLAY_STOP, sizeof(int));      // gui接收

	GxBus_MessageListen(self, GXMSG_SATIP_SERVICE_START);
	GxBus_MessageListen(self, GXMSG_SATIP_SERVICE_STOP);
	GxBus_MessageListen(self, GXMSG_SATIP_DIALOG_START);
	GxBus_MessageListen(self, GXMSG_SATIP_DIALOG_STOP);
	GxBus_MessageListen(self, GXMSG_SATIP_DIALOG_PAUSE);
	GxBus_MessageListen(self, GXMSG_SATIP_DIALOG_RESUME);

	handle_t sch;
	sch = GxBus_SchedulerCreate("SatipMsgScheduler", GXBUS_SCHED_MSG, 1024 * 10, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("SatipConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 20, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void AppSatipServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_SATIP_SERVICE_START);
	GxBus_MessageUnListen(self, GXMSG_SATIP_SERVICE_STOP);
	GxBus_MessageUnListen(self, GXMSG_SATIP_DIALOG_START);
	GxBus_MessageUnListen(self, GXMSG_SATIP_DIALOG_STOP);
	GxBus_MessageUnListen(self, GXMSG_SATIP_DIALOG_PAUSE);
	GxBus_MessageUnListen(self, GXMSG_SATIP_DIALOG_RESUME);

	GxBus_MessageUnregister(GXMSG_SATIP_SERVICE_START);
	GxBus_MessageUnregister(GXMSG_SATIP_SERVICE_STOP);
	GxBus_MessageUnregister(GXMSG_SATIP_DIALOG_START);
	GxBus_MessageUnregister(GXMSG_SATIP_DIALOG_STOP);
	GxBus_MessageUnregister(GXMSG_SATIP_DIALOG_PAUSE);
	GxBus_MessageUnregister(GXMSG_SATIP_DIALOG_RESUME);

	GxBus_MessageUnregister(GXMSG_SATIP_PLAY_START);
	GxBus_MessageUnregister(GXMSG_SATIP_PLAY_STOP);

	GxBus_ServiceUnlink(self);
}

GxMsgStatus AppSatipServiceRecvMsg(handle_t self, GxMessage *msg)
{
	GxMsgStatus ret = GXCORE_SUCCESS;

	if(msg == NULL)
		return GXCORE_ERROR;

	switch(msg->msg_id)
	{
		case GXMSG_SATIP_SERVICE_START:
			{
				SatipStartMsg *param = NULL;
				param = GxBus_GetMsgPropertyPtr(msg, SatipStartMsg);
				if (app_satip_service_start(param) < 0)
					param->start_ok = 0;
				else
					param->start_ok = 1;
			}
			break;
		case GXMSG_SATIP_SERVICE_STOP:
			{
				int *param = NULL;
				param = GxBus_GetMsgPropertyPtr(msg, int);
				app_satip_service_stop(*param);
			}
			break;
		case GXMSG_SATIP_DIALOG_START:
			app_satip_dialog_start();
			break;
		case GXMSG_SATIP_DIALOG_STOP:
			app_satip_dialog_stop();
			break;
		case GXMSG_SATIP_DIALOG_PAUSE:
			{
				int *sec = NULL;
				sec = GxBus_GetMsgPropertyPtr(msg, int);
				app_satip_dialog_pause(*sec);
			}
			break;
		case GXMSG_SATIP_DIALOG_RESUME:
			app_satip_dialog_resume();
			break;
		default:
			break;
	}
	return ret;
}

void AppSatipServiceConsole(handle_t self)
{
	app_satip_task();
}

GxServiceClass app_satip_service = {
	"app satip service",
	AppSatipServiceInit,
	AppSatipServiceDestroy,
	AppSatipServiceRecvMsg,
	AppSatipServiceConsole,
};

#endif
