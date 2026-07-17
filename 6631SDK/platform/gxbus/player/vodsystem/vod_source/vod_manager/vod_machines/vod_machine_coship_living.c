#include "../../include/vod_in_common_def.h"
#include "../../include/vod_manager_main.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_decode_api.h"
#include "../../include/vod_trans_api.h"
#include "../../vod_recv/coship/receiver.h"

static int32_t coship_machine_idle_open(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_start_param_t* open_param ;
	coship_receiver_param_t receiver;
	VOD_DECODE_INIT_PARAM_t decoder_param;
	MEM_InjectParam_t * mp;

	open_param = (vod_start_param_t *)ulActionParam ;

	vod_trans_open(VOD_TRANS_TYPE_TS);

	receiver.io_type = IO_UDP;
	receiver.ip = open_param->multicast_ip;
	receiver.key = 0;
	receiver.port = open_param->multicast_port;
	receiver.session = 0;
	receiver.url = 0;

	if (coship_receiver_open(&receiver) < 0)
	{
		vod_trans_close();
		return -1;
	}

	decoder_param.dinputtype = DATA_INPUT_MEM;
	mp = &(decoder_param.param.MemParam);
	mp->vidformat     = VOD_VIDEO_MPEG2;
	mp->vidstream     = VOD_VIDEO_STREAM_TYPE_PES;
	mp->viddecmode    = VOD_DECODE_SD;
	mp->audformat     = VOD_AUDIO_MPEG1;
	mp->audstream     = VOD_AUDIO_STREAM_TYPE_PES;
	mp->audsamplerate = 48000;
	vod_dec_set_media_params(&decoder_param);


	vod_porting_task_delay(1000);

	vod_dec_start_media(0, 1);

	return 0;
}

static int32_t coship_machine_idle_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_dec_stop_media(0, 1);
	coship_receiver_close();
	vod_trans_close();
	return 0;
}

static int32_t coship_machine_idle_running(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_porting_task_delay(100);
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulIdelMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = coship_machine_idle_open ,
	/*pActionStart:   */		coship_machine_idle_running ,
	/*pActionStop:    */		coship_machine_idle_stop ,
	/*pActionPause:   */		NULL ,
	/*pActionSeek:    */		NULL ,
	/*pActionResume: */			NULL ,
	/*pActionTrick:   */		NULL ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL

};

VOD_MANAGER_MIDDLE_ACTION_MACHINE* gPStatMachine_coship_living[VOD_MANAGER_ENGIN_END]=
{
	&ulIdelMachine ,
	0 ,
	0 ,
	0 ,
	0,
	0
};


