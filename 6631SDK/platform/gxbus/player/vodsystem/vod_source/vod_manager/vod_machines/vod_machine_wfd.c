#include "../../include/vod_in_common_def.h"
#include "../../include/vod_manager_main.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_decode_api.h"
#include "../../include/vod_trans_api.h"
#include "../../vod_trans/vod_trans.h"
#include "../../vod_recv/wfd/wfd_receiver.h"
#include "../../vod_protocol/wfd/wfd_util.h"
#include "gxmedia_module.h"
#include "stheader.h"

static GxMediaAudio* audio_handle = NULL;
static GxMediaVideo* video_handle = NULL;

static void wfd_player_callback(int event, int errorcode)
{
}

static int32_t wfd_machine_idle_open(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	char host[64] = "127.0.0.1";
	int client_port = 0;
	vod_start_param_t* open_param ;
	wfd_receiver_param_t receiver;

	open_param = (vod_start_param_t *)ulActionParam ;

	if (wfd_rtsp_open((char*)open_param->url, host, &client_port, 4000, wfd_player_callback) < 0)
		return -1;

	vod_trans_open(VOD_TRANS_TYPE_HWTS);

	receiver.io_type = IO_UDP;
	receiver.ip = host;
	receiver.port = client_port;
	receiver.key = NULL;
	receiver.session = NULL;
	receiver.url = NULL;

	if (wfd_receiver_open(&receiver) < 0)
	{
		vod_trans_close();
		wfd_rtsp_close();
		return -1;
	}

	GxMediaVideoPara vparam;
	GxMediaAudioPara aparam;
	uint32_t demux_handle = 0;

	vparam.codectype = VIDEO_CODEC_H264;
	vparam.width  = 1280;
	vparam.height = 720;
	aparam.codectype = AUDIO_CODEC_AAC_LATM;
	aparam.sample_rate = -1;
	aparam.sample_bits = -1;
	vod_trans_demux(&demux_handle);

	video_handle = GxMedia_VideoOpen((GxMediaDemux*)demux_handle);
	audio_handle = GxMedia_AudioOpen((GxMediaDemux*)demux_handle);
	GxMedia_VideoConfig(video_handle, vparam);
	GxMedia_AudioConfig(audio_handle, aparam);
	GxMedia_VideoStart(video_handle);
	GxMedia_AudioStart(audio_handle);

	return 0;
}

static int32_t wfd_machine_idle_stop(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	wfd_receiver_close();
	GxMedia_AudioStop(audio_handle);
	GxMedia_VideoStop(video_handle, 0);
	GxMedia_AudioClose(audio_handle);
	GxMedia_VideoClose(video_handle);
	audio_handle = NULL;
	video_handle = NULL;
	vod_trans_close();
	wfd_rtsp_close();
	return 0;
}

static int32_t wfd_machine_idle_running(vod_manager_info_t * vinfo , int32_t ulActionParam)
{
	vod_porting_task_delay(100);
	return 0;
}

static VOD_MANAGER_MIDDLE_ACTION_MACHINE ulIdelMachine =
{
	/*pActionOpen:    */.pActionMachine.pActionFun.pActionOpen = wfd_machine_idle_open ,
	/*pActionStart:   */		wfd_machine_idle_running ,
	/*pActionStop:    */		wfd_machine_idle_stop ,
	/*pActionPause:   */		NULL ,
	/*pActionSeek:    */		NULL ,
	/*pActionResume: */			NULL ,
	/*pActionTrick:   */		NULL ,
	/*pActionbuffing*/			NULL ,
	/*pActionGetparameter: */	NULL ,
	/*pActionDestory: */		NULL
};

VOD_MANAGER_MIDDLE_ACTION_MACHINE* gPStatMachine_wfd[VOD_MANAGER_ENGIN_END]=
{
	&ulIdelMachine ,
	0 ,
	0 ,
	0 ,
	0,
	0
};


