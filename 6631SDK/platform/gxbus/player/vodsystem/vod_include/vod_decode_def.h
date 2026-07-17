/*****************************************************************************

File Name  : vod_decode_def.h

Description: decoder module definitions

Copyright (C) 2008 Hangzhou Motorola Technology Co.,Ltd

Author: Wang Xinhong

Create Date:2008.05.08

* Date		   	 Name			Modification
* ----			 --------    	------------
* 2008.5.08 	Wang.Xinhong  	  Created
*  
*****************************************************************************/

#ifndef _VOD_DECODE_DEF_H_
#define _VOD_DECODE_DEF_H_

typedef enum
{
	DATA_INPUT_NONE = 0,
	DATA_INPUT_PTI,
	DATA_INPUT_MEM,

	DATA_INPUT_END
}VOD_DATA_INPUT_TYPE_t;

typedef enum
{
    VOD_AUDIO_AAC = 1,
    VOD_AUDIO_AC3,
    VOD_AUDIO_MPEG1,
    VOD_AUDIO_MPEG2,
    VOD_AUDIO_MP3,
    VOD_AUDIO_PCM,

    VOD_AUDIO_NULL
} VOD_AudioFormat_t;

typedef enum
{
    VOD_AUDIO_STREAM_TYPE_ES = 1,
    VOD_AUDIO_STREAM_TYPE_PES,
    VOD_AUDIO_STREAM_TYPE_ES_BLK
} VOD_AUDIO_StreamType_t;

typedef enum
{
    VOD_VIDEO_MPEG1= 1,
    VOD_VIDEO_MPEG2,
    VOD_VIDEO_MPEG4,
	VOD_VIDEO_H264,
    VOD_VIDEO_VC1,
	VOD_VIDEO_AVS,
	VOD_VIDEO_TYPE_END
}VOD_VideoFormat_t;

typedef enum
{
    VOD_DECODE_SD = 1,	/*标清*/
    VOD_DECODE_HD		/*高清显示*/
} VOD_VideoDecodeMode_t;

typedef enum
{
    VOD_VIDEO_STREAM_TYPE_ES = 1,
    VOD_VIDEO_STREAM_TYPE_PES
}VOD_VIDEO_StreamType_t;

typedef enum
{
    VOD_PTI_ID_TYPE_PID = 0,
	VOD_PTI_ID_TYPE_PMT,
	VOD_PTI_ID_TYPE_SERVICEID
}VOD_PTI_ID_TYPE_t;

typedef enum
{
	VOD_GET_CURRENT_TIME,
	VOD_GET_AUDIO_DEC_TIME,
	VOD_GET_VIDEO_DEC_TIME,
	VOD_GET_AUDIO_BUF_SIZE,
	VOD_GET_AUDIO_BUF_FREESIZE,
	VOD_GET_VIDEO_BUF_SIZE,
	VOD_GET_VIDEO_BUF_FREESIZE,

	VOD_GET_INFO_END
}VOD_GETINFO_TYPE_t;


typedef void (*avReadDataCallback_t)(uint8_t** ppbuffer,
				int32_t* plen, uint32_t* ptimestamp, int32_t* pIsIFrame);



#endif

