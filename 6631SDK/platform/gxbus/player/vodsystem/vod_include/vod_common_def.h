#ifndef _VOD_COMMON_DEF_H_
#define _VOD_COMMON_DEF_H_

#if 0
#define GxVod_debug(...)     gxlogf ( __VA_ARGS__ )
#else
#define GxVod_debug(...)     ((void)0)
#endif

#define MOTO_NPT_NOW  			0x80000000
#define MOTO_NPT_END 			0x7FFFFFFF

/*这里的结构定义为vod内部＆外部统一的，如果修改到结构，需要与vod_in_common_def.h 以及 vod_in_typedef.h 或者 vod_manager_main.h 里面同步*/

typedef enum
{
	VOD_APP_UNKNOWN = 0,
	VOD_APP_SIHUA_VOD,
	VOD_APP_SIHUA_SHIFT,
	VOD_APP_SIHUA_SINGLE,
	VOD_APP_SIHUA_MP3,
	VOD_APP_IPTV2_VOD,
	VOD_APP_IPTV2_SINGLE,
	VOD_APP_IPTV2_GROUP,
	VOD_APP_IPTV2_MP3,
	VOD_APP_MOTO_LSCP_VOD,
	VOD_APP_MS_MMS_VOD,
	VOD_APP_COSHIP,
	VOD_APP_COSHIP_LIVING,	
	VOD_APP_PVR,
	VOD_APP_WFD,
	VOD_APP_END
}VOD_APP_TYPE_t;

typedef enum
{
	VOD_RANGE_TYPE_NONE = 0,
	VOD_RANGE_TYPE_NPT,
	VOD_RANGE_TYPE_CLOCK,
	VOD_RANGE_TYPE_PTS,
	VOD_RANGE_TYPE_STAMP,

	VOD_RANGE_TYPE_END
} VOD_RANGE_TYPE_t;


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

typedef  struct
{
	uint8_t url[2048];
	uint8_t url_add[2048];
	int32_t  apptype;
	int32_t  regionid ;
}
Vod_action_param_t;

typedef enum
{
	EVENT_NONE = 0,
	EVENT_PLAY_START,
	EVENT_PROGRAM_END,
	EVENT_SERVER_NORESP,
	EVENT_SERVER_CLOSED,
	EVENT_CONNECTION_CLOSED,
	EVENT_BEGINNING_OF_FILE,
	EVENT_END_OF_FILE,
	EVENT_IP_CABLE_IN,
	EVENT_IP_CABLE_OUT,
	EVENT_TS_CABLE_IN,
	EVENT_TS_CABLE_OUT,
	EVENT_RTP_BUFFER_OVERFLOW,
	EVENT_TS_BUFFER_OVERFLOW,
	EVENT_NET_BUSY,
	EVENT_NET_OK,
	EVENT_RANGE_ERROR,
	EVENT_RETURN_CODE_ERROR,

	EVENT_END	
}VOD_EVENT_t;
typedef enum
{
	VOD_MANAGER_ENGIN_IDEL = 0X00,			/* 空闲 */
	VOD_MANAGER_ENGIN_PREROLL ,		/* 缓存 */
	VOD_MANAGER_ENGIN_RUNNING ,		/* 正常运行*/
	VOD_MANAGER_ENGIN_TRICKING ,		/* 快进快退 */
	VOD_MANAGER_ENGIN_PAUSED ,		/* 暂停 */
	VOD_MANAGER_ENGIN_BUFFERING ,
	VOD_MANAGER_ENGIN_END
}VOD_MANAGER_STATUS_t;


typedef  struct
{
	uint8_t url[2048];                //当前进入点播或者时移地url
	uint8_t url_add[2048];            //进入点播和时移地第二个url，目前用途在上海电信的iptv项目，直播url＆时移url分开的情况
	int32_t  apptype;                //选择vodmachine的业务应用类型
	int32_t  regionid;               //cable线中获取到的regionid
	int32_t  init_scale;             //进入时移或者点播初始状态的scale，允许以快退或者快进方式直接进入时移
	uint8_t init_pause;               //进入时移时，是否通过暂停dvb方式进入
	uint8_t enter_place;              //进入时移或者点播前一种状态，0 为浏览器  1 为 dvb,此标志位跟vod本身没有关系
	uint32_t srmip;			//used for moto dmx srm ip address
	uint16_t srmport;			//srm ip port number
	uint32_t gateway;		//bmi ip address
	uint16_t groupid;			// stb group id
	char assertid[21];		// program assert id
	uint32_t startnpt;
	int io_type;
	int service_type;
	char multicast_ip[16];
	int multicast_port;
	int64_t start_position;    //进入点播或者时移相对于起始时间的位置
	int64_t reference_time;        //进入时移起始UTC时间
	char hostip[64];
	int  hostport;
}vod_start_param_t;


typedef void (*avReadDataCallback_t)(uint8_t** ppbuffer,
				int32_t* plen, uint32_t* ptimestamp, int32_t* pIsIFrame);
#define IO_TCP					1
#define IO_UDP					2
#define IO_CABLE				3
#define SERVICE_VOD				1
#define SERVICE_SHIFTTIME		2
#define SERVICE_NPVR            3
#define SERVICE_LIVING			4


#endif

