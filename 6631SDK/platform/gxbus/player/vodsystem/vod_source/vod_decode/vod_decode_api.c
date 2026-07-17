#include "../include/vod_trans_api.h"
#include "../include/vod_decode_api.h"
#include "../include/vod_porting_all.h"
#include "../../vod_include/vod_decode_def.h"

extern int32_t vod_porting_drv_videostart(VOD_VideoFormat_t vidformat,
		VOD_VIDEO_StreamType_t vidstream,
		VOD_VideoDecodeMode_t  viddecmode,
		avReadDataCallback_t vcb);
extern int32_t vod_porting_drv_audiostart(uint32_t audsamplerate,
		VOD_AudioFormat_t audformat,
		VOD_AUDIO_StreamType_t audstream,
		avReadDataCallback_t acb);
extern int32_t vod_porting_get_decoder_time(uint32_t aud_vid, uint32_t * time);
extern int32_t vod_porting_get_decoder_bufsize(uint32_t aud_vid, uint32_t * free, uint32_t * used);
extern int32_t vod_porting_drv_audiostop(uint8_t muteaudio);
extern int32_t vod_porting_drv_videostop(uint8_t clearvideo);
extern int32_t vod_porting_drv_videopause(void);
extern int32_t vod_porting_drv_audiopause(void);
extern int32_t vod_porting_drv_videoresume(void);
extern int32_t vod_porting_drv_audioresume(void);
extern int32_t vod_porting_drv_Iframe_mode(uint8_t flag);

static VOD_DECODE_INIT_PARAM_t g_decodeparam = {0};

uint8_t s_audio_head_required = 0; /*是否需要注入音频带外数据*/
uint8_t s_video_head_required = 0; /*是否需要注入视频带外数据*/

static uint8_t first_start_decode_flag = 0;
static uint8_t udec_scale_flag = 0;

void vod_dec_reset_decode_flag(void)
{
	first_start_decode_flag = 0;
}
/*-------------------------------------Internal-------------------------------*/
static void vod_dec_audio_callback_wrapper(uint8_t** ppbuffer,
		int32_t* plen,
		uint32_t* ptimestamp,
		int32_t* pIsIFrame)
{
	if(g_decodeparam.dinputtype == DATA_INPUT_MEM)
	{
		vod_trans_get_frame(VOD_TRANS_FRAME_TYPE_AUDIO,
				ppbuffer, plen, ptimestamp, pIsIFrame);
		if(udec_scale_flag)
		{
			*plen = 0;
			*ppbuffer = NULL;
		}
	}
}

static void vod_dec_video_callback_wrapper(uint8_t** ppbuffer,
		int32_t* plen,
		uint32_t* ptimestamp,
		int32_t* pIsIFrame)
{
	if(g_decodeparam.dinputtype == DATA_INPUT_MEM)
	{
		vod_trans_get_frame(VOD_TRANS_FRAME_TYPE_VIDEO,
				ppbuffer, plen, ptimestamp, pIsIFrame);
	}
}

/*--------------------------------Module API----------------------------------*/

/*
   vod_dec_get_media_infomation
   说明：
   查询媒体状态
   定义：
   int32_t vod_dec_get_media_infomation (uint32_t ulMediaInfoType, uint32_t * pMediaInfo)
   参数：
   ulMediaInfoType：查询动作的类型
   pMediaInfo：承载查询返回的信息指针
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_get_media_infomation (uint32_t ulMediaInfoType, uint32_t * pMediaInfo)
{
	uint32_t temp = 0;

	switch (ulMediaInfoType)
	{
		case VOD_GET_CURRENT_TIME:
			vod_porting_get_decoder_time(1, &temp);
			temp = temp / 1000;
			*pMediaInfo = temp;/*unit:s*/
			break;
		case VOD_GET_AUDIO_DEC_TIME:
			vod_porting_get_decoder_time(0, pMediaInfo); /*unit:ms*/
			break;
		case VOD_GET_VIDEO_DEC_TIME:
			vod_porting_get_decoder_time(1, pMediaInfo); /*unit:ms*/
			break;
		case VOD_GET_AUDIO_BUF_SIZE:
			vod_porting_get_decoder_bufsize(0, NULL, pMediaInfo);
			break;
		case VOD_GET_AUDIO_BUF_FREESIZE:
			vod_porting_get_decoder_bufsize(0, pMediaInfo, NULL);
			break;
		case VOD_GET_VIDEO_BUF_SIZE:
			vod_porting_get_decoder_bufsize(1, NULL, pMediaInfo);
			break;
		case VOD_GET_VIDEO_BUF_FREESIZE:
			vod_porting_get_decoder_bufsize(1, pMediaInfo, NULL);
			break;
		default:
			break;
	}

	return 0;
}


/*
   vod_dec_set_media_params 
   说明：
   将待解媒体的参数设置到解码模块中。注意，这个接口仅仅提供媒体参数的设置，
   而不提供解码器的起停等动作。底层驱动需要通过满足驱动porting接口的需求
   来满足某些媒体播放时的设置<->播放顺序。
   定义：
   int32_t vod_dec_set_media_params(VOD_DECODE_INIT_PARAM_t* initparam )
   参数：
   ulMediaType：媒体类型
   ulLParam、ulLParam：供扩展使用的参数
   pReturnParam：供扩展使用的返回参数指针
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_set_media_params(VOD_DECODE_INIT_PARAM_t* initparam )
{
	if (initparam == NULL)
	{
		return 1;
	}

	memcpy(&g_decodeparam, initparam, sizeof(VOD_DECODE_INIT_PARAM_t));

	return 0;
}


/*
   vod_dec_start_media
   说明：
   启动解码器进行解码。
   定义：
   int32_t vod_dec_start_media ( uint8_t audio_head_required , uint8_t video_head_required )
   参数：
   need_trans_head：是否需要注入带外数据payload head
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_start_media ( uint8_t audio_head_required , uint8_t video_head_required )
{
	int32_t ret = -1;
	PTI_InjectParam_t * pp = &(g_decodeparam.param.PtiParam);
	MEM_InjectParam_t * mp = &(g_decodeparam.param.MemParam);

	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI: /* Inject from PTI(Realtime) */
			if (first_start_decode_flag == 0)
			{
				if (pp->id_type == VOD_PTI_ID_TYPE_PID)
				{
					ret = vod_porting_dvb_play_by_pid(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->videopid,
							pp->audiopid,
							pp->pcrpid,
							pp->EmmPid,
							pp->AudioEcmPid,
							pp->VideoEcmPid);
				}
				else if (pp->id_type == VOD_PTI_ID_TYPE_PMT)
				{
					ret = vod_porting_dvb_play_by_pmtpid(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->pmtpid);
				}
				else if (pp->id_type == VOD_PTI_ID_TYPE_SERVICEID)
				{
					ret = vod_porting_dvb_play_by_serviceid(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->serviceid);

				}
				first_start_decode_flag = 1;
			}
			break;
		case DATA_INPUT_MEM: /* Inject from Memory(Non-Realtime) */

			/*是否需要先注入带外数据*/
			s_audio_head_required = audio_head_required;
			s_video_head_required = video_head_required;

			/* 启动视频解码, 若格式不同，需要重新初始化 */
			ret = vod_porting_drv_videostart(mp->vidformat,
					mp->vidstream,
					mp->viddecmode,
					vod_dec_video_callback_wrapper);

			/* 启动音频解码 */
			ret |= vod_porting_drv_audiostart(mp->audsamplerate,
					mp->audformat,
					mp->audstream,
					vod_dec_audio_callback_wrapper);

			break;

		default: /*Inject from HDD driver ect.*/
			break;

	}

	return ret;
}

/*
   vod_dec_sync_start_media
   说明：
   启动解码器进行解码,PMT方式修改为同步启动解码器，不再去搜pmt表。
   定义：
   int32_t vod_dec_sync_start_media ( uint8_t audio_head_required , uint8_t video_head_required )
   参数：
   need_trans_head：是否需要注入带外数据payload head
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_sync_start_media ( uint8_t audio_head_required , uint8_t video_head_required )
{
	PTI_InjectParam_t * pp = &(g_decodeparam.param.PtiParam);
	MEM_InjectParam_t * mp = &(g_decodeparam.param.MemParam);

	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI: /* Inject from PTI(Realtime) */
			{
				if (pp->id_type == VOD_PTI_ID_TYPE_PID)
				{
					vod_porting_dvb_play_by_pid_sync(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->videopid,
							pp->audiopid,
							pp->pcrpid,
							pp->EmmPid,
							pp->AudioEcmPid,
							pp->VideoEcmPid);
				}
				else if (pp->id_type == VOD_PTI_ID_TYPE_PMT)
				{
					vod_porting_dvb_play_by_pmtpid_sync(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->pmtpid);
				}
				else if (pp->id_type == VOD_PTI_ID_TYPE_SERVICEID)
				{
					vod_porting_dvb_play_by_serviceid_sync(pp->frequency,
							pp->symbolrate,
							pp->qam,
							pp->serviceid);

				}

			}
			break;
		case DATA_INPUT_MEM: /* Inject from Memory(Non-Realtime) */

			/*是否需要先注入带外数据*/
			s_audio_head_required = audio_head_required;
			s_video_head_required = video_head_required;

			/* 启动视频解码, 若格式不同，需要重新初始化 */
			vod_porting_drv_videostart(mp->vidformat,
					mp->vidstream,
					mp->viddecmode,
					vod_dec_video_callback_wrapper);

			/* 启动音频解码 */
			vod_porting_drv_audiostart(mp->audsamplerate,
					mp->audformat,
					mp->audstream,
					vod_dec_audio_callback_wrapper);

			break;

		default: /*Inject from HDD driver ect.*/
			break;

	}

	return 0;
}


/*
   vod_dec_stop_media
   说明：
   停止解码器解码。
   定义：
   int32_t vod_dec_stop_media ( uint8_t clearvideo, uint8_t muteaudio )
   参数：
clearvideo :是否清除video
muteaudio: 解码器停止时是否需要静音	
返回：
0：成功
其他：错误
*/
int32_t vod_dec_stop_media ( uint8_t muteaudio, uint8_t clearvideo)
{
	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI:
			vod_porting_dvb_stop(clearvideo);
			break;
		case DATA_INPUT_MEM:
			vod_porting_drv_audiostop(muteaudio);
			vod_porting_drv_videostop(clearvideo);
			break;
		default:
			break;
	}
	vod_dec_reset_decode_flag();
	return 0;
}

/*
   vod_dec_sync_stop_media
   说明：
   停止解码器解码。
   定义：
   int32_t vod_dec_sync_stop_media ( uint8_t clearvideo, uint8_t muteaudio )
   参数：
clearvideo :是否清除video
muteaudio: 解码器停止时是否需要静音	
返回：
0：成功
其他：错误
*/
int32_t vod_dec_sync_stop_media ( uint8_t muteaudio, uint8_t clearvideo)
{
	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI:
			vod_porting_dvb_stop(clearvideo);
			break;

		case DATA_INPUT_MEM:
			vod_porting_drv_audiostop(muteaudio);
			vod_porting_drv_videostop(clearvideo);
			break;

		default:
			break;

	}
	return 0;
}

/*
   vod_dec_pause_media
   说明：
   暂停解码器解码。要求可以通过resume接口进行
   定义：
   int32_t vod_dec_pause_media (void)
   参数：
   无
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_pause_media (void)
{
	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI:
			vod_porting_dvb_pause();
			break;

		case DATA_INPUT_MEM:
			vod_porting_drv_videopause();
			vod_porting_drv_audiopause();
			break;

		default:
			break;
	}

	return 0;
}


/*
   vod_dec_resume_media
   说明：
   恢复被暂停的解码器继续解码。要求可以在pause一次后调用该接口进行恢复解码。
   定义：
   int32_t vod_dec_resume_media (void)
   参数：
   无
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_resume_media (void)
{
	switch (g_decodeparam.dinputtype)
	{
		case DATA_INPUT_PTI:
			vod_porting_dvb_resume();
			break;

		case DATA_INPUT_MEM:
			/*先启动视频解码(启动视频解码后需要等待时间和音频同步出来)*/
			vod_porting_drv_videoresume();

			/*经验值:音频视频同时恢复后，音频会快225ms,所以主动delay*/
			vod_porting_task_delay(350);

			/*再开启音频*/
			vod_porting_drv_audioresume();
			break;

		default:
			break;
	}

	return 0;
}

int32_t vod_dec_seek_media (uint32_t ulStartPos , uint32_t ulOffset)
{
	return 0;
}

/*------ Module APIs for independ DSP architecture only ----*/

/*
   vod_dec_firmware_init
   说明：
   初始化解码设备模块，以及解码模块本身的参变量初始化。
   该接口针对不同的解码设备如不需要做初始化工作可以为空实现，但必须有实现体。
   该接口存在的原因是在某些采用解码器在工作前需要装载firmware，
   比如5516平台用的PNX1500在工作前需要装载firmware。
   定义：
   int32_t vod_dec_firmware_init(void)
   参数：
   无
   返回：
   0：成功
   其他：失败
   */
int32_t vod_dec_firmware_init(void)
{
	return 0;
}

/*
   vod_dec_firmware_load
   说明：
   装载解码器的固件。该接口存在的原因是在某些采用解码器在工作前需要装载firmware，比如5516平台用的PNX1500在工作前需要装载firmware。
   定义：
   int32_t vod_dec_firmware_load(void)
   参数：
   无
   返回：
   0：成功
   其他：失败
   */
int32_t vod_dec_firmware_load(void)
{
	return 0;
}


/*
   vod_dec_firmware_check_load
   说明：
   检查解码器固件的装载是否成功。该接口存在的原因是在某些采用解码器在工作前
   需要装载firmware，比如5516平台用的PNX1500在工作前需要装载firmware。
   定义：
   int32_t vod_dec_firmware_check_load(void);
   参数：
   无
   返回：
   0：成功
   1：正在装载
   2：装载中
   其他：未知错误
   */
int32_t vod_dec_firmware_check_load(void)
{
	return 0;
}


/*
   vod_dec_firmware_destory 
   说明：
   该功能提供了解码器销毁过程，销毁过程必须注意的是在正常播放解码过程中，
   不能被销毁，必须等待解码器停止或者IDLE状态才可以被销毁。
   定义：
   int32_t vod_dec_firmware_destory(void);
   参数：
   无
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_firmware_destory(void)
{
	return 0;
}


/*
   vod_dec_draw_osd
   说明：
   该接口存在的原因是在某些采用解码器在解码时接管了VOUT，所以菜单的显示需要
   通过CPU往解码器送命令而非传统的调用OSD接口显示菜单。
   如5516平台用的PNX1500在解码时画菜单需要调用该接口。
   定义：
   int32_t vod_dec_draw_osd(uint32_t ulOSDIndex, uint32_t ulOSDParam);
   参数：
   ulOSDIndex：OSD的索引
   ulOSDParam：画OSD 的命令
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_draw_osd(uint8_t MenuType,uint32_t MenuParmL,uint32_t MenuParmH)
{
	return 0;
}


/*
   vod_dec_clean_osd
   说明：
   该接口存在的原因是在某些采用解码器在解码时接管了VOUT，所以菜单的显示需要
   通过CPU往解码器送而非传统的调用OSD接口显示菜单。
   如5516平台用的PNX1500在解码时画菜单需要调用该接口。
   定义：
   int32_t vod_dec_clean_osd(uint32_t ulOSDIndex);
   参数：
   ulOSDIndex：OSD的索引
   返回：
   0：成功
   其他：错误
   */
int32_t vod_dec_clean_osd(uint32_t ulOSDIndex)
{
	return 0;
}

/***********************************************************************************
  vod_dec_forward_media
  说明：
  指定媒体解码引擎向前的TRICKMODE操作。工作状态中的解码引擎，包括几类工作模式：
  正常工作、拖动工作、Forward模式、Backward模式。
  工作模式的转换操作包括：Seek、Forward、Backward、Normal。
  定义：
  int32_t vod_dec_scale_media( int32_t speed );
  参数：
flag:表示是否进入scalemode
返回：
0：成功
其他：错误
 ***********************************************************************************/
int32_t vod_dec_scale_media( uint8_t flag )
{
	if(flag == 0 && udec_scale_flag != 0)
	{
		vod_porting_drv_Iframe_mode(0);
	}
	else if(flag != 0 && udec_scale_flag == 0)
	{
		vod_porting_drv_Iframe_mode(1);
	}

	udec_scale_flag = flag;

	return 0 ;
}

