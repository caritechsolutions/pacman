#include "gx_common.h"
#include "../vod_include/vod_decode_def.h"
#include "../vod_source/include/vod_in_typedef.h"
#include "../vod_source/include/vod_porting_all.h"
#include "gx_avutil.h"
#include "gxmedia_module.h"

static GxMediaAudio* audio_handle = NULL;
static GxMediaVideo* video_handle = NULL;

avReadDataCallback_t   AudReadData = NULL;
avReadDataCallback_t   VidReadData = NULL;

static char RunTransferTaskFlag = 0;
static unsigned int inject_thread_id = 0;

void Read_AV_Data_task(void *p)
{
	uint8_t* p_data_v = NULL;
	uint8_t* p_data_a = NULL;
	int remainlen_v = 0;
	int remainlen_a = 0;
	uint32_t timestamp_v = 0;
	uint32_t timestamp_a = 0;
	int I_frame_flag_v = 0;
	int I_frame_flag_a = 0;
	int a_ts = 0;
	int v_ts = 0;

	while(RunTransferTaskFlag){
		if (v_ts > a_ts){
			if(AudReadData != NULL){
				if(remainlen_a == 0){
					p_data_a = NULL;
					AudReadData(&p_data_a, &remainlen_a, &timestamp_a, &I_frame_flag_a);
				}
				if(remainlen_a == 0){
					if(VidReadData != NULL){
						if(remainlen_v == 0){
							p_data_v = NULL;
							VidReadData(&p_data_v,&remainlen_v,&timestamp_v,&I_frame_flag_v);
						}
						if(remainlen_v){
							v_ts = timestamp_v;
							GxMedia_VideoInjectData(video_handle,
									GXMEDIA_SOURCE_ES, p_data_v, remainlen_v, timestamp_v);
							remainlen_v = 0;
						}
					}
				}else{
					if(remainlen_a>(14+7)){
						a_ts = timestamp_a;
						GxMedia_AudioInjectData(audio_handle,
								GXMEDIA_SOURCE_ES, p_data_a, remainlen_a, timestamp_a);
						remainlen_a = 0;
					}else{
						remainlen_a = 0;
					}
				}
			}
		}else{
			if(VidReadData != NULL)
			{
				if(remainlen_v == 0){
					p_data_v = NULL;
					VidReadData(&p_data_v,&remainlen_v,&timestamp_v,&I_frame_flag_v);
				}
				if(remainlen_v == 0){
					if(AudReadData != NULL){
						if(remainlen_a == 0){
							p_data_a = NULL;
							AudReadData(&p_data_a,&remainlen_a,&timestamp_a,&I_frame_flag_a);
						}
						if(remainlen_a){
							if(remainlen_a>(14+7)){
								a_ts = timestamp_a;
								GxMedia_AudioInjectData(audio_handle,
										GXMEDIA_SOURCE_ES, p_data_a, remainlen_a, timestamp_a);
								remainlen_a = 0;
							}else{
								remainlen_a = 0;
							}
						}
					}
				}else{
					v_ts = timestamp_v;
					GxMedia_VideoInjectData(video_handle,
							GXMEDIA_SOURCE_ES, p_data_v, remainlen_v, timestamp_v);
					remainlen_v = 0;
				}
			}
		}
		vod_porting_task_delay(100);
	}
}

static int InstallTransferTask(void)
{
	RunTransferTaskFlag = 1;
	if (vod_porting_task_create(Read_AV_Data_task, 0, 2*1024, 9,
				&inject_thread_id, (const unsigned char*)"Read_AV_Data_task") < 0)
	{
		gxlogd("[%s-%d]: inject data create task err\n", __FUNCTION__, __LINE__);
		return -1;
	}
	return 0;
}

static int UnInstallTransferTask(void)
{
	RunTransferTaskFlag = 0;
	if(inject_thread_id)
	{
		vod_porting_task_waitdel(inject_thread_id, 1500);
		inject_thread_id = 0;
	}
	return 0;
}

int32_t vod_porting_drv_videostart(VOD_VideoFormat_t vidformat,
		VOD_VIDEO_StreamType_t vidstream,
		VOD_VideoDecodeMode_t  viddecmode,
		avReadDataCallback_t vcb)
{
	GxMediaVideoPara vparam;

	video_handle = GxMedia_VideoOpen(NULL);
	if(video_handle == NULL)
		return -1;

	VidReadData = vcb;
	switch(vidformat)
	{
		case VOD_VIDEO_MPEG1:
		case VOD_VIDEO_MPEG2:
			vparam.codectype = VIDEO_CODEC_MPEG12;
			break;
		case VOD_VIDEO_MPEG4:
			vparam.codectype = VIDEO_CODEC_MPEG4;
			break;
		case VOD_VIDEO_H264:
			vparam.codectype = VIDEO_CODEC_H264;
			break;
		case VOD_VIDEO_VC1:
			vparam.codectype = VIDEO_CODEC_VC1;
			break;
		case VOD_VIDEO_AVS:
			vparam.codectype = VIDEO_CODEC_AVS;
			break;
		default:
			vparam.codectype = VIDEO_CODEC_MPEG12;
			break;
	}

	switch(viddecmode)
	{
		case VOD_DECODE_SD:
			vparam.width = 720;
			vparam.height = 576;
			break;
		case VOD_DECODE_HD:
			vparam.width = 1280;
			vparam.height = 720;
			break;
		default:
			vparam.width = -1;
			vparam.height = -1;
			break;
	}
	GxMedia_VideoConfig(video_handle, vparam);
	GxMedia_VideoStart(video_handle);
	return 0;
}

int32_t vod_porting_drv_videostop(uint8_t clearvideo)
{
	VidReadData = NULL;
	GxMedia_VideoStop(video_handle, clearvideo);
	GxMedia_VideoClose(video_handle);
	video_handle = NULL;
	return 0;
}

int32_t vod_porting_drv_videopause(void)
{
	GxMedia_VideoPause(video_handle);
	return 0;
}

int32_t vod_porting_drv_videoresume(void)
{
	GxMedia_VideoResume(video_handle);
	return 0;
}

int32_t vod_porting_drv_audiostart(uint32_t audsamplerate,
		VOD_AudioFormat_t audformat,
		VOD_AUDIO_StreamType_t audstream,
		avReadDataCallback_t acb)
{
	GxMediaAudioPara aparam;

	audio_handle = GxMedia_AudioOpen(NULL);
	if(audio_handle == NULL)
		return -1;

	AudReadData = acb;
	switch(audformat)
	{
		case VOD_AUDIO_AAC:
			aparam.codectype= AUDIO_CODEC_AAC_LATM;
			break;
		case VOD_AUDIO_AC3:
			aparam.codectype= AUDIO_CODEC_AC3;
			break;
		case VOD_AUDIO_MPEG1:
			aparam.codectype= AUDIO_CODEC_MPEG1;
			break;
		case VOD_AUDIO_MPEG2:
		case VOD_AUDIO_MP3:
			aparam.codectype= AUDIO_CODEC_MPEG2;
			break;
		case VOD_AUDIO_PCM:
			aparam.codectype= AUDIO_CODEC_PCM;
			break;
		default:
			aparam.codectype= AUDIO_CODEC_MPEG1;
			break;
	}
	InstallTransferTask();
	GxMedia_AudioConfig(audio_handle, aparam);
	GxMedia_AudioStart(audio_handle);
	return 0;
}

int32_t vod_porting_drv_audiostop(uint8_t muteaudio)
{
	AudReadData = NULL;
	GxMedia_AudioStop(audio_handle);
	UnInstallTransferTask();
	GxMedia_AudioClose(audio_handle);
	audio_handle = NULL;
	return 0;
}

int32_t vod_porting_drv_audioresume(void)
{
	GxMedia_AudioPause(audio_handle);
	return 0;
}

int32_t vod_porting_drv_audiopause(void)
{
	GxMedia_AudioResume(audio_handle);
	return 0;
}

int32_t vod_porting_firmware_init(VOD_VideoFormat_t vidfmt)
{
	int32_t err = 0;
	return  err;
}

int32_t vod_porting_firmware_load(void)
{
	int32_t err = 0;
	return  err;
}
int32_t vod_porting_firmware_check_load(void)
{
	int32_t err = 0;
	return  err;
}
int32_t vod_porting_firmware_destory(void)
{
	int32_t err = 0;

	return  err;
}

int32_t vod_porting_get_decoder_bufsize(uint32_t aud_vid, uint32_t * free, uint32_t * used)
{
	return 0;
}

int32_t vod_porting_get_decoder_time(uint32_t aud_vid, uint32_t * time)
{
	return 0;
}

int32_t vod_porting_draw_osd(uint8_t MenuType,uint32_t MenuParmL,uint32_t MenuParmH)
{
	return 0;
}

int32_t vod_porting_drv_Iframe_mode(uint8_t flag)
{
	return 0;
}



