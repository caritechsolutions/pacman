#include "../../include/vod_manager_main.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_decode_api.h"
#include "../../include/vod_trans_api.h"
#include "../../include/vod_common_func.h"
#include "../../vod_protocol/coship/rtsp.h"
#include "../../vod_recv/coship/receiver.h"

#define TIMEOUT_VALUE 3000

extern int32_t vod_dec_start_media(uint8_t audio_head_required, uint8_t video_head_required);
extern int32_t vod_dec_scale_media(uint8_t flag);
extern void coship_set_shift_vod(void);

static coship_rtsp_info_t* rtsp_info;

static void make_current_pos_range(char* range, int get_pos)
{
	if(get_pos){
		int pos = 0;
		coship_rtsp_get_position(&pos);
	}

	if (rtsp_info->start_time > 360000)
		sprintf(range, "Clock=%s-", time_to_rtspstr(rtsp_info->position));
	else
		sprintf(range, "npt=%d-", rtsp_info->position);
}

static void run_timer()
{
	switch(vod_manager_timer_tigger())
	{
		case VOD_MANAGER_TIMER_GET_PARAMETER:
			{
				int pos;
				if(coship_rtsp_get_position(&pos)<0)
					rtsp_info->error_count++;
				else
					rtsp_info->error_count = 0;
				break;
			}
		default:
			break;
	}
}

static void coship_player_callback(int event, int errorcode)
{
	if (event == RTSP_EVENT_CLOSE)
	{
		vod_porting_event_notify(EVENT_PROGRAM_END, errorcode);
	}
	else if (event == RTSP_EVENT_BOS)
	{
		vod_porting_event_notify(EVENT_END_OF_FILE, errorcode);
	}
	else if (event == RTSP_EVENT_EOS)
	{
		vod_porting_event_notify(EVENT_END_OF_FILE, errorcode);
	}
	else if(event == RTSP_EVENT_SERVER_CLOSE ||
			event == RTSP_EVENT_SOCK_ERR)
	{
		vod_porting_event_notify(EVENT_SERVER_CLOSED, errorcode);
	}
	else if(event == RTSP_EVENT_ERROR_CODE)
	{
		vod_porting_event_notify(EVENT_RETURN_CODE_ERROR, errorcode);
	}
}

static void add_areaid_to_url(char* url, char* buf, int write_flags)
{
	if(write_flags){
		char* p;
		int len;
		p = strstr(url, "areaCode=");
		if(!p){
			memcpy(buf, url ,strlen(url));
		}else{
			char areaCode[32];
			memset(areaCode, 0, 32);
			vod_porting_get_arecode(areaCode);
			len = p - url + 9;
			memcpy(buf, url, len);
			strcpy(buf + len, areaCode);
			strcat(buf, p + 9);
		}
	}else{
		memcpy(buf, url ,strlen(url));
	}
	return;
}

static int32_t coship_machine_idle_open(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char range[64];
	char url[1024];
	vod_start_param_t* open_param ;
	coship_receiver_param_t receiver;
	VOD_DECODE_INIT_PARAM_t decoder_param;

	open_param = (vod_start_param_t *)ulActionParam ;

	memset(url, 0, 1024);
	add_areaid_to_url((char*)open_param->url, url, 1);

	if (coship_rtsp_open(url, open_param->hostip, open_param->hostport, open_param->io_type, TIMEOUT_VALUE, coship_player_callback) < 0)
		return -1;

	rtsp_info = coship_rtsp_get_info();

	vod_trans_open(VOD_TRANS_TYPE_TS);

	receiver.io_type = open_param->io_type;
	receiver.ip = (char*)rtsp_info->media_ip;
	receiver.key = rtsp_info->keystring;
	receiver.port = rtsp_info->media_port;
	receiver.session = rtsp_info->session;
	receiver.url = url;

	if (coship_receiver_open(&receiver) < 0)
	{
		coship_rtsp_close();
		vod_trans_close();
		return -1;
	}

	int start_pos = open_param->start_position;
	if (open_param->reference_time > 360000)
		rtsp_info->start_time = open_param->reference_time;

	if (rtsp_info->start_time > 360000)
	{
		if (rtsp_info->duration == 0){
			sprintf(range, "Clock=%s-", time_to_rtspstr(rtsp_info->start_time+start_pos));
			rtsp_info->start_time += start_pos;
		}
		else{
			char s1[17];
			char s2[17];
			strcpy(s1, time_to_rtspstr(rtsp_info->start_time+start_pos));
			strcpy(s2, time_to_rtspstr(rtsp_info->start_time+rtsp_info->duration));
			sprintf(range, "Clock=%s-%s", s1, s2);
		}
		coship_set_shift_vod();
	}
	else
	{
		if (rtsp_info->duration == 0)
			sprintf(range, "npt=%d-", rtsp_info->start_time+start_pos);
		else
			sprintf(range, "npt=%d-%d", rtsp_info->start_time+start_pos, rtsp_info->start_time + rtsp_info->duration);
	}

	coship_rtsp_play((char*)range, 0, 0);

	if(open_param->io_type == IO_CABLE){
		PTI_InjectParam_t *pp;
		decoder_param.dinputtype = DATA_INPUT_PTI;
		pp = &(decoder_param.param.PtiParam);
		pp->id_type = VOD_PTI_ID_TYPE_SERVICEID;
		pp->frequency = rtsp_info->freq;
		pp->symbolrate = rtsp_info->symbrate;
		pp->qam= rtsp_info->qamsize;
		pp->serviceid = rtsp_info->program_no;
	}else{
		MEM_InjectParam_t * mp;
		decoder_param.dinputtype = DATA_INPUT_MEM;
		mp = &(decoder_param.param.MemParam);
		mp->vidformat     = VOD_VIDEO_H264;
		mp->vidstream     = VOD_VIDEO_STREAM_TYPE_PES;
		mp->viddecmode    = VOD_DECODE_SD;
		mp->audformat     = VOD_AUDIO_MPEG1;
		mp->audstream     = VOD_AUDIO_STREAM_TYPE_PES;
		mp->audsamplerate = 48000;
	}
	vod_dec_set_media_params(&decoder_param);

//	vod_porting_task_delay(1000);
	if(vod_dec_start_media(0, 1) != 0){
		coship_rtsp_close();
		coship_receiver_close();
		vod_trans_close();
		return -1;
	}
	vod_dec_scale_media(0);
	vod_manager_timer_set(90000, VOD_MANAGER_TIMER_GET_PARAMETER, VOD_MANAGER_TIMER_USED, VOD_MANAGER_TIMER_MOD_LOOP) ;
	vod_manager_set_state(VOD_MANAGER_ENGIN_RUNNING);
	return 0;
}

static int32_t coship_machine_idle_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_dec_stop_media(0, 1);
	vod_dec_scale_media(0);
	coship_rtsp_close();
	coship_receiver_close();
	vod_trans_close();
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulIdelMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = coship_machine_idle_open ,
	/*pActionStart:   */		NULL , 
	/*pActionStop:    */		coship_machine_idle_stop , 
	/*pActionPause:   */		NULL , 
	/*pActionSeek:    */		NULL , 
	/*pActionResume: */			NULL ,
	/*pActionTrick:   */		NULL ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL

};

static int32_t coship_machine_running_running(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	run_timer();
	vod_porting_task_delay(20);
	return 0;
}

static int32_t coship_machine_running_pause(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	coship_rtsp_pause();
	vod_manager_set_state(VOD_MANAGER_ENGIN_PAUSED);
	vod_dec_pause_media();
	return 0;
}

static int32_t coship_machine_running_seek(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
//	vod_dec_stop_media(0, 1);
	vod_trans_clean();
	coship_receiver_reset();
	coship_rtsp_play((char*)ulActionParam, 0, 0);
//	vod_porting_task_delay(1000);
	vod_dec_start_media(0, 0);
	vod_dec_scale_media(0);
	return 0;
}

static int32_t coship_machine_running_trick(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char range[64];

	coship_rtsp_pause();
	make_current_pos_range(range, 1);
	vod_trans_clean();
	coship_receiver_reset();
//	vod_dec_stop_media(0, 1);
	coship_rtsp_play(range, 0, ulActionParam);
	vod_dec_start_media(0, 0);
	vod_dec_scale_media(1);
	vod_manager_set_state(VOD_MANAGER_ENGIN_TRICKING); 
	return 0;
}
static int32_t coship_machine_running_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_dec_stop_media(0, 1);
	vod_dec_scale_media(0);
	coship_rtsp_close();
	coship_receiver_close();
	vod_trans_close();
	vod_manager_set_state(VOD_MANAGER_ENGIN_IDEL);
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulRunningMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = NULL ,
	/*pActionStart:   */		coship_machine_running_running ,
	/*pActionStop:    */		coship_machine_running_stop ,
	/*pActionPause:   */		coship_machine_running_pause ,
	/*pActionSeek:    */		coship_machine_running_seek ,
	/*pActionResume: */			NULL ,
	/*pActionTrick:   */		coship_machine_running_trick ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL

};

static int32_t coship_machine_pause_running(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	run_timer();
	vod_porting_task_delay(20);
	return 0;
}

static int32_t coship_machine_pause_resume(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char range[64];

	make_current_pos_range(range, 0);
	vod_dec_resume_media();
	coship_rtsp_play(range, 0, 0);
	vod_manager_set_state(VOD_MANAGER_ENGIN_RUNNING);
	return 0;
}
static int32_t coship_machine_pause_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_dec_stop_media(0, 1);
	vod_dec_scale_media(0);
	coship_rtsp_close();
	coship_receiver_close();
	vod_trans_close();
	vod_manager_set_state(VOD_MANAGER_ENGIN_IDEL);
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulPauseMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = NULL ,
	/*pActionStart:   */		coship_machine_pause_running ,
	/*pActionStop:    */		coship_machine_pause_stop ,
	/*pActionPause:   */		NULL ,
	/*pActionSeek:    */		NULL ,
	/*pActionResume: */			coship_machine_pause_resume ,
	/*pActionTrick:   */		NULL ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL

};

static int32_t coship_machine_tricking_running(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	run_timer();
	vod_porting_task_delay(20);
	return 0;
}

static int32_t coship_machine_tricking_resume(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char range[64];

	coship_rtsp_pause();
	make_current_pos_range(range, 0);
	vod_trans_clean();
	coship_receiver_reset();
//	vod_dec_stop_media(0, 1);
	coship_rtsp_play(range, 0, 0);
	vod_dec_start_media(0, 0);
	vod_dec_scale_media(0);
	vod_manager_set_state(VOD_MANAGER_ENGIN_RUNNING);
	return 0;
}

static int32_t coship_machine_tricking_trick(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char range[64];

	coship_rtsp_pause();
	make_current_pos_range(range, 0);
	vod_trans_clean();
	coship_receiver_reset();
//	vod_dec_stop_media(0, 1);
	coship_rtsp_play(range, 0, ulActionParam);
	vod_dec_start_media(0, 0);
	vod_dec_scale_media(1);
	return 0;
}

static int32_t coship_machine_tricking_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_dec_stop_media(0, 1);
	vod_dec_scale_media(0);
	coship_rtsp_close();
	coship_receiver_close();
	vod_trans_close();
	vod_manager_set_state(VOD_MANAGER_ENGIN_IDEL);
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulTrickMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = NULL ,
	/*pActionStart:   */		coship_machine_tricking_running ,
	/*pActionStop:    */		coship_machine_tricking_stop ,
	/*pActionPause:   */		NULL ,
	/*pActionSeek:    */		NULL ,
	/*pActionResume: */			coship_machine_tricking_resume ,
	/*pActionTrick:   */		coship_machine_tricking_trick ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL

};

VOD_MANAGER_MIDDLE_ACTION_MACHINE * gPStatMachine_coship[VOD_MANAGER_ENGIN_END]=
{
	&ulIdelMachine ,
	0 ,
	&ulRunningMachine ,
	&ulTrickMachine ,
	&ulPauseMachine,
	0
};

