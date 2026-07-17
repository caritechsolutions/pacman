#ifndef _VOD_DECODE_API_H_
#define _VOD_DECODE_API_H_

#include "vod_in_common_def.h"
#include "vod_in_typedef.h"
#include "../../vod_include/vod_decode_def.h"

typedef struct
{
	/* base info */
	uint32_t frequency;
	uint32_t symbolrate;
	uint32_t qam;

	/* sub info type */
	VOD_PTI_ID_TYPE_t id_type;

	/* 3 types*/	
	uint32_t pmtpid;   /* type 1: pmt*/

	uint32_t videopid; /* type 2: pids*/
	uint32_t audiopid;
	uint32_t pcrpid;
	uint32_t EmmPid;
	uint32_t AudioEcmPid;
	uint32_t VideoEcmPid;
	
	uint32_t serviceid; /* type 3: service_id */

} PTI_InjectParam_t;

typedef struct
{
	/* 视频信息 */
	VOD_VideoFormat_t      vidformat;
	VOD_VIDEO_StreamType_t vidstream;
	VOD_VideoDecodeMode_t  viddecmode;

	/* 音频信息 */
	VOD_AudioFormat_t      audformat;
	VOD_AUDIO_StreamType_t audstream;
	uint32_t                    audsamplerate; /*采样率*/

#ifdef PVR_SUPPORT
	uint8_t pvr_mode;
#endif	
	/* 音视频带外数据*/
	uint8_t * aud_media_head_p;
	uint32_t aud_head_len;

	uint8_t * vid_media_head_p;
	uint32_t vid_head_len;
	
} MEM_InjectParam_t;

typedef struct 
{
	VOD_DATA_INPUT_TYPE_t dinputtype; 

	union
	{
		PTI_InjectParam_t PtiParam;
		MEM_InjectParam_t MemParam;		
	} param;
	
}VOD_DECODE_INIT_PARAM_t;


int32_t vod_dec_firmware_init(void);

int32_t vod_dec_firmware_load(void);

int32_t vod_dec_firmware_check_load(void);

int32_t vod_dec_firmware_destory(void);

int32_t vod_dec_draw_osd(uint8_t MenuType,uint32_t MenuParmL,uint32_t MenuParmH);
int32_t vod_dec_clean_osd(uint32_t ulOSDIndex);

int32_t vod_get_media_infomation (uint32_t ulMediaInfoType ,
									uint32_t * pMediaInfo,
									uint32_t ulTimeoutMS  );

int32_t vod_dec_set_media_params(VOD_DECODE_INIT_PARAM_t* initparam );

int32_t vod_dec_start_media ( uint8_t audio_head_required , uint8_t video_head_required );

int32_t vod_dec_stop_media (uint8_t muteaudio ,  uint8_t clearvideo );

int32_t vod_dec_sync_start_media ( uint8_t audio_head_required , uint8_t video_head_required );

int32_t vod_dec_sync_stop_media (uint8_t muteaudio ,  uint8_t clearvideo );

int32_t vod_dec_pause_media (void);

int32_t vod_dec_resume_media (void);

int32_t vod_dec_seek_media (uint32_t ulStartPos , uint32_t ulOffset);

int32_t vod_dec_forward_media (int32_t ulSpeed);

int32_t vod_dec_backward_media (int32_t ulSpeed);

#endif

