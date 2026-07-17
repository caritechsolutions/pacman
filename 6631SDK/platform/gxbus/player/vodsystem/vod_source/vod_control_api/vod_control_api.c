#include "../include/vod_in_common_def.h"
#include "../include/vod_common_func.h"
#include "../include/vod_manager_main.h"
#include "../include/vod_decode_api.h"
#include "../include/vod_porting_all.h"
#include "../../vod_include/vod_control_api.h"
#include "gxplayer_module.h"
#include "gx_common.h"

#define VOD_API_MAX_WAIT_TIME 20000
#define VOD_API_MAX_SEND_TIME 100

extern int32_t vod_manager_init (void);
extern void vod_manager_exit(void);
extern void vod_manager_deselect_machine(void);
extern int32_t vod_manager_select_machine(VOD_APP_TYPE_t apptype);
extern int32_t  vod_api_msg_reset(void);
extern int32_t  vod_api_msg_send(uint32_t timeout_ms ,int32_t event ,int32_t lParams ,int32_t wParams);
extern int32_t  vod_api_msg_recv(uint32_t timeout_ms ,int32_t* event ,int32_t* lParams ,int32_t* wParams);
extern unsigned int coship_get_start_time(void);
extern int coship_get_duration(void);
extern int coship_get_current_pos(void);
extern int coship_get_really_current_pos(void);
extern int coship_get_trick_mode(void);
extern int32_t vod_dec_get_media_infomation (uint32_t ulMediaInfoType, uint32_t * pMediaInfo);
extern uint32_t vod_trans_get_v_packnum(void);
extern uint32_t vod_trans_get_a_packnum(void);

VOD_VideoFormat_t ulMediaVideoType_TS = VOD_VIDEO_TYPE_END;
VOD_AudioFormat_t ulMediaAudioType_TS = VOD_AUDIO_NULL;

VOD_APP_TYPE_t machine_type = VOD_APP_UNKNOWN;
/******************************************************************
  说明:
  初始化VOD模块内容
  名称:
  STB_VOD_Init
  参数:
protocol:使用的控制协议
play_type:业务类型
model_priority:模块任务级别
callback:事件回掉函数
 ********************************************************************/
int32_t STB_VOD_Init(void)
{
	return vod_manager_init();
}

int32_t STB_VOD_Exit(void)
{

	vod_manager_exit();

	vod_manager_deselect_machine();
	return ERRNO_VOD_NO_ERROR;
}
/**********************************************************
  说明：
  开始点播一部影片，通过调用本接口，VOD将会和SERVER进行指令交互，如果指令交互成功并且开始播放节目，函数成功返回，否则返回错误代码。
  定义：
  int32_t STB_VOD_Play( vod_start_param_t* param, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
  参数：

  返回：
  VOD错误代码
 *********************************************************/
int32_t STB_VOD_Play( vod_start_param_t* param, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
{
	int32_t err = ERRNO_VOD_NO_ERROR;
	static vod_start_param_t my_param;
	int32_t event = 0, param1 = 0, param2 = 0;
	uint32_t wait_time;
	uint32_t begin_time, lastcbtime;

	if(param == NULL)
	{
		GxVod_debug("STB_VOD_Play param == NULL\n");
		return ERRNO_VOD_PARAM;
	}

	if(cbfunc && cbtime)
	{
		wait_time = 100;
	}
	else
	{
		wait_time = time_out;
	}

	memcpy(&my_param, param, sizeof(vod_start_param_t));

	vod_manager_select_machine(my_param.apptype);
	machine_type = my_param.apptype;
	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_OPEN, (int32_t)&my_param);
	if(err)
	{
		GxVod_debug("STB_VOD_Play vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}

	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}
	begin_time = vod_porting_get_ms();
	lastcbtime = begin_time;

	while(1)
	{
		err = vod_api_msg_recv(wait_time, &event, &param1, &param2);
		if(err == 0)
		{
			GxVod_debug("STB_VOD_Play ret[%d][%d][%d]\n", event, param1, param2);
			if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_OPEN)
			{
				return ERRNO_VOD_NO_ERROR;
			}
			else if(event == VOD_MANAGER_MID_MSGCODE_ERROR && param1 == VOD_MANAGER_MID_MSG_OPEN)
			{
				return ERRNO_VOD_ERROR_RESULT;
			}
			else
			{
				GxVod_debug("STB_VOD_Play recv errorcode [%d][%d][%d]\n", event, param1, param2);
			}
		}

		if(cbfunc && ((vod_porting_get_ms() - lastcbtime) >= cbtime))
		{
			cbfunc();
			lastcbtime = vod_porting_get_ms();
		}

		if((vod_porting_get_ms() - begin_time) >= time_out)
		{
			break;
		}

	}
	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  暂停当前播放的节目到某一点或者当前正在播放的点。
  定义：
  int32_t STB_VOD_Pause(uint32_t position, uint32_t time_out);
  参数：
Position:如果是0，表示立即暂停到当前播放的点，如果>0，表示当节目播放到position点的时候暂停。
Time_out:超时时间，如果为0，则使用使用VOD默认超时时间。
返回：
VOD错误代码

 *********************************************************/
int32_t STB_VOD_Pause(uint32_t position, uint32_t time_out)
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;

	vod_api_msg_reset();
	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_PAUSE, 0);
	if(err)
	{
		GxVod_debug("STB_VOD_Pause vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}

	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}
	err = vod_api_msg_recv(time_out, &event, &param1, &param2);
	if(err == 0)
	{
		GxVod_debug("STB_VOD_Pause ret[%d][%d][%d]\n", event, param1, param2);
		if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_PAUSE)
		{
			return ERRNO_VOD_NO_ERROR;
		}
	}
	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  调用本接口时候，节目应该是处在暂停状态，否则可能出错。
  定义：
  int32_t STB_VOD_Resume ( uint32_t position, uint32_t time_out );
  参数:
Position:如果为0，表示节目从暂停点开始播放，如果不为0，表示节目从position点开始播放，而不是从暂停的点开始播放。此时操作相当于resume(0) + seek(position),有些SERVER支持这种暂停和恢复的操作模式。
Time_out:超时时间，单位豪秒。
返回：
VOD错误代码

 *********************************************************/
int32_t STB_VOD_Resume ( uint32_t position, uint32_t time_out )
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;

	vod_api_msg_reset();
	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_RESTART, 0);
	if(err)
	{
		GxVod_debug("STB_VOD_Resume vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}
	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}

	err = vod_api_msg_recv(time_out, &event, &param1, &param2);
	if(err == 0)
	{
		GxVod_debug("STB_VOD_Resume ret[%d][%d][%d]\n", event, param1, param2);
		if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_RESTART)
		{
			return ERRNO_VOD_NO_ERROR;
		}
	}
	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  节目跳转操作。
  定义：
  int32_t STB_VOD_Seek(uint8_t* dstposition, VOD_RANGE_TYPE_t rtype, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
  参数：
Dstposition:节目要跳转的目的时间点。
rtype:range 字段的类型，目前支持两种 npt类型＆clock类型
Time_out：同上
返回：
VOD错误代码

 *********************************************************/
int32_t STB_VOD_Seek(char* dstposition, VOD_RANGE_TYPE_t rtype, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;
	static char range[64] = {0};/*这里必须是static,因为需要传送地址到vodmanager*/
	char* p_tmp;
	uint32_t wait_time;
	uint32_t begin_time, lastcbtime;

	memset(range, 0, sizeof(range));
	vod_api_msg_reset();


	if(dstposition == NULL || strlen(dstposition) <= 0)
	{
		GxVod_debug("STB_VOD_Seek param err dstposition [%s]\n", dstposition);
		return ERRNO_VOD_PARAM;
	}

	if(cbfunc && cbtime)
	{
		wait_time = 100;
	}
	else
	{
		wait_time = time_out;
	}

	switch(rtype)
	{
		case VOD_RANGE_TYPE_NPT:
			strncpy(range, "npt=", sizeof(range)-1);
			strncat(range, dstposition, 50);
			break;
		case VOD_RANGE_TYPE_CLOCK:
			strncpy(range, "Clock=", sizeof(range)-1);
			strncat(range, (char*)time_to_rtspstr(atoi(dstposition)), 50);
			break;
		case VOD_RANGE_TYPE_STAMP:
			strncat(range, dstposition, 50);
			break;

		default:
			GxVod_debug("STB_VOD_Seek param err rtype [%d]\n", rtype);
			return ERRNO_VOD_PARAM;
			break;
	}

	/*range格式为 npt=xx-xx或者 npt=xx-,所以如果没有-,则必须在后面加上一个*/
	if(rtype != VOD_RANGE_TYPE_STAMP)
	{
		p_tmp = strchr(range, '-');
		if(p_tmp == NULL)
		{
			strcat(range, "-");
		}
	}

	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_SEEK, (int32_t)range);
	if(err)
	{
		GxVod_debug("STB_VOD_Seek vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}
	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}

	begin_time = vod_porting_get_ms();
	lastcbtime = begin_time;

	while(1)
	{
		err = vod_api_msg_recv(wait_time, &event, &param1, &param2);
		if(err == 0)
		{
			if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_SEEK)
			{
				return ERRNO_VOD_NO_ERROR;
			}
			else if(event == VOD_MANAGER_MID_MSGCODE_ERROR && param1 == VOD_MANAGER_MID_MSG_SEEK)
			{
				return ERRNO_VOD_ERROR_RESULT;
			}
			else
			{
				GxVod_debug("STB_VOD_Seek recv errorcode [%d][%d][%d]\n", event, param1, param2);
			}
		}

		if(cbfunc && ((vod_porting_get_ms() - lastcbtime) >= cbtime))
		{
			cbfunc();
			lastcbtime = vod_porting_get_ms();
		}

		if((vod_porting_get_ms() - begin_time) >= time_out)
		{
			break;
		}

	}

	return ERRNO_VOD_ERROR_RESULT;
}


/**********************************************************
  说明：
  节目跳转操作。
  定义：
  int32_t STB_VOD_Seek_Ex(uint32_t srcposition, uint32_t dstposition, uint32_t time_out);
  参数：
Srcposition: 如果=0 表示节目从当前正在播放的点跳转到dstposition点开始播放。
如果>0表示当节目播放到srcposition点的时候，直接跳转到
dstposition继续进行播放。
Dstposition:节目要跳转的目的时间点。
Time_out：同上
返回：
VOD错误代码

 *********************************************************/
int32_t STB_VOD_Seek_Ex(uint32_t srcposition, uint32_t dstposition, uint32_t time_out)
{

	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  本函数用于调整正在播放的节目的播放速度。正常速度为0，大于0的速度表示快速向前进行播放，小于0的速度表示快速向后进行播放。
  定义：
  int32_t STB_VOD_Scale( int speed, uint32_t time_out );//speed
  参数：
Speed:目标播放速度，服务器可能只支持几种播放速度，当设置一个speed后，服务器会选择最接近speed的速度进行播放。
Time_out:同上
返回：
VOD错误代码

 *********************************************************/
int32_t STB_VOD_Scale( int speed, uint32_t time_out, vodcontrolcallback cbfunc, uint32_t cbtime )
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;
	uint32_t wait_time;
	uint32_t begin_time, lastcbtime;

	vod_api_msg_reset();


	if(speed != -32 && speed != -16 && speed != -8 &&speed != -4 && speed != -2 && 
			speed != 2 && speed != 4 && speed != 8 &&speed != 16 && speed != 32)
	{
		GxVod_debug("STB_VOD_Scale err speed %d\n", speed);
	}

	if(cbfunc && cbtime)
	{
		wait_time = 100;
	}
	else
	{
		wait_time = time_out;
	}

	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_TRICK, speed);
	if(err)
	{
		GxVod_debug("STB_VOD_Scale vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}
	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}

	begin_time = vod_porting_get_ms();
	lastcbtime = begin_time;

	while(1)
	{
		err = vod_api_msg_recv(wait_time, &event, &param1, &param2);
		if(err == 0)
		{
			if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_TRICK)
			{
				return ERRNO_VOD_NO_ERROR;
			}
			else if(event == VOD_MANAGER_MID_MSGCODE_ERROR && param1 == VOD_MANAGER_MID_MSG_TRICK)
			{
				return ERRNO_VOD_ERROR_RESULT;
			}
			else
			{
				GxVod_debug("STB_VOD_Scale recv errorcode [%d][%d][%d]\n", event, param1, param2);
			}
		}

		if(cbfunc && ((vod_porting_get_ms() - lastcbtime) >= cbtime))
		{
			cbfunc();
			lastcbtime = vod_porting_get_ms();
		}

		if((vod_porting_get_ms() - begin_time) >= time_out)
		{
			break;
		}

	}

	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  退出正在播放的节目。
  定义：
  int32_t STB_VOD_Stop( uint32_t time_out );
  参数：
  无
  返回：
  VOD错误代码

 *********************************************************/
int32_t STB_VOD_Stop( uint32_t time_out )
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;

	vod_api_msg_reset();

	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_STOP, 0);
	if(err)
	{
		GxVod_debug("STB_VOD_Stop vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}
	if(time_out == 0)
	{
		return ERRNO_VOD_NO_ERROR;
	}

	err = vod_api_msg_recv(time_out, &event, &param1, &param2);
	if(err == 0)
	{
		GxVod_debug("STB_VOD_Stop ret[%d][%d][%d]\n", event, param1, param2);
		if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_STOP)
		{
			return ERRNO_VOD_NO_ERROR;
		}
	}
	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  进行getparameter操作
  定义：
  int32_t STB_VOD_Getparam( uint32_t time_out );
  参数：
  无
  返回：
  VOD错误代码

 *********************************************************/
int32_t STB_VOD_Getparam( uint32_t time_out )
{
	int32_t err;
	int32_t event = 0, param1 = 0, param2 = 0;

	vod_api_msg_reset();


	err = vod_api_msg_send(VOD_API_MAX_SEND_TIME, VOD_MANAGER_MID_CLASS_CTRL_MSG, VOD_MANAGER_MID_MSG_GET_PARAMETER, 0);
	if(err)
	{
		GxVod_debug("STB_VOD_Getparam vod_api_msg_send ret %d\n", err);
		return ERRNO_VOD_ERROR_RESULT;
	}

	err = vod_api_msg_recv(time_out, &event, &param1, &param2);
	if(err == 0)
	{
		GxVod_debug("STB_VOD_Getparam ret[%d][%d][%d]\n", event, param1, param2);
		if(event == VOD_MANAGER_MID_MSGCODE_SUCCEED && param1 == VOD_MANAGER_MID_MSG_GET_PARAMETER)
		{
			return ERRNO_VOD_NO_ERROR;
		}
	}
	return ERRNO_VOD_ERROR_RESULT;
}

/**********************************************************
  说明：
  此接口用于获得当前播放的节目的时间信息，主要包括开始时间，结束时间，当前音频解码器时间，当前视频解码器时间，在使用本函数的时候，如果只想获得其中的某个参数而不是全部，可以把不想取的参数用NULL。
  定义：

  参数：
Start:返回当前节目的开始时间，在点播中应该是相对时间，通常为0，在时移中应该是绝对时间。如果不想获取本参数，在调用的时候可以使用NULL.
End:返回当前节目的结束时间，在点播中应该是相对时间，通常为0，在时移中应该是绝对时间。如果不想获取本参数，在调用的时候可以使用NULL.
current:当前播放时间
Aud_dec_time:返回音频解码器当前时间，如果不想获得本参数，可是使用NULL。
Vid_dec_time: 返回视频解码器当前时间，如果不想获得本参数，可以使用NULL。

 *********************************************************/
int32_t STB_VOD_Info_Position(int32_t* start, int32_t* end, int32_t * current, int32_t * aud_dec_time, int32_t* vid_dec_time)
{
	if(start)
	{
		if (machine_type == VOD_APP_COSHIP)
		{
			*start = coship_get_start_time();
		}
	}
	if(end)
	{
		if (machine_type == VOD_APP_COSHIP)
		{
			*end = coship_get_start_time() + coship_get_duration();
		}
	}
	if(current)
	{
		if (machine_type == VOD_APP_COSHIP)
		{
			*current = coship_get_start_time() +coship_get_really_current_pos();
		}
	}
	if(vid_dec_time)
	{
		uint32_t vdt;

		vod_dec_get_media_infomation (VOD_GET_VIDEO_DEC_TIME, &vdt);

		*vid_dec_time = vdt;
	}
	if(aud_dec_time)
	{
		uint32_t adt;

		vod_dec_get_media_infomation (VOD_GET_AUDIO_DEC_TIME, &adt);

		*aud_dec_time = adt;
	}

	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  本函数用于获取VOD核心模块当前的状态信息。
  定义：
  int32_t STB_VOD_Info_Buffersize(&iShowBufferSizeTmp, NULL);
  参数：
Status:返回VOD核心模块的状态

 *********************************************************/

int32_t STB_VOD_Info_Buffersize(uint32_t * vsize, uint32_t * asize)
{
	if(vsize)
	{
		vod_dec_get_media_infomation (VOD_GET_VIDEO_BUF_SIZE, vsize);
	}
	if(asize)
	{
		vod_dec_get_media_infomation (VOD_GET_AUDIO_BUF_SIZE, asize);
	}

	return ERRNO_VOD_NO_ERROR;
}
/**********************************************************
  说明：
  本函数用于获取VOD核心模块当前的状态信息。
  定义：
  int32_t STB_VOD_Info_Buffersize(&iShowBufferSizeTmp, NULL);
  参数：
Status:返回VOD核心模块的状态

 *********************************************************/

int32_t STB_VOD_Info_BufferFreesize(uint32_t * vsize, uint32_t * asize)
{
	if(vsize)
	{
		vod_dec_get_media_infomation (VOD_GET_VIDEO_BUF_FREESIZE, vsize);
	}
	if(asize)
	{
		vod_dec_get_media_infomation (VOD_GET_AUDIO_BUF_FREESIZE, asize);
	}

	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  本函数用于获取VOD核心模块当前的状态信息。
  定义：
  int32_t  STB_VOD_Info_Status ( VOD_MANAGER_STATUS_t * status );
  参数：
Status:返回VOD核心模块的状态

 *********************************************************/
VOD_MANAGER_STATUS_t STB_VOD_Info_Status ( void )
{
	return vod_manager_get_state();
}


/**********************************************************
  说明：
  获取当前节目音频编码类型。
  定义：
  VOD_AudioFormat_t STB_VOD_Info_Aud_Type(void)

 *********************************************************/
VOD_AudioFormat_t STB_VOD_Info_Aud_Type(void)
{
	return 0;
}


/**********************************************************
  说明：
  获取当前节目的视频编码格式。
  定义：
  int32_t STB_VOD_Info_Vid_Type(DRV_VideoFormat_t * type);

 *********************************************************/
VOD_VideoFormat_t STB_VOD_Info_Vid_Type(void)
{
	return 0;
}
/**********************************************************
  说明：
  获取当前节目的视频编码格式。
  定义：
  int32_t STB_VOD_Info_Vid_Size( uint32_t * width uint32_t * hight);

 *********************************************************/
uint32_t STB_VOD_Info_Vid_Size( uint32_t * width, uint32_t * hight)
{
	return ERRNO_VOD_NO_ERROR;
}


/**********************************************************
  说明：
  获取音频的声道信息。
  定义：
  int32_t STB_VOD_Info_Aud_Channel(uint32_t * left, uint32_t* right);
  参数：
  Left：如果左声道返回1，否则返回0
Right:如果右声道返回1，否则返回0
如果是立体声，二者都返回1
返回：
错误代码

 *********************************************************/
int32_t STB_VOD_Info_Aud_Channel(uint32_t * left, uint32_t* right)
{
	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  获取音频频率。
  定义：
  int32_t STB_VOD_Info_Aud_Freq( uint32_t * freq);

 *********************************************************/
int32_t STB_VOD_Info_Aud_Freq( uint32_t * freq)
{
	int32_t ifreq = 0;
	if ( freq == NULL )
	{
		return ERRNO_VOD_PARAM;
	}

	if ( machine_type == VOD_APP_MOTO_LSCP_VOD )
	{
		return 48000;
	}
	else
	{
	}
	return ifreq;
}

/**********************************************************
  说明：
  获得视频解码器的配置参数。
  定义：
  int32_t STB_VOD_Info_Vid_Config(
  uint8_t * configstr, 
  uint32_t* configlen, 
  uint32_t configmaxlen );
  参数：
Configstr:返回字符串的保存地址
Configlen:返回的音频配置字符串长度
Configmaxlen:能够接收的字符串最大长度
返回：
错误代码

 *********************************************************/
int32_t STB_VOD_Info_Vid_Config(
		uint8_t * configstr, 
		uint32_t* configlen, 
		uint32_t configmaxlen )
{
	if (configstr == NULL || configlen == NULL || configmaxlen <= 0 )
	{
		return ERRNO_VOD_PARAM;
	}

	return ERRNO_VOD_NO_ERROR;
}


/**********************************************************
  说明：
  获取音频侦的侦率
  定义：
  int32_t STB_VOD_Info_Aud_FrameRate( uint32_t* rate );
  参数：
Rate:返回音频侦率

 *********************************************************/
int32_t STB_VOD_Info_Aud_FrameRate( uint32_t* rate )
{
	if ( rate == NULL )
	{
		return ERRNO_VOD_PARAM;
	}

	return ERRNO_VOD_NO_ERROR;
}


/**********************************************************
  说明：
  获取视频侦的侦率
  定义：
  int32_t STB_VOD_Info_Vid_FrameRate ( uint32_t* rate );
  参数：
Rate:返回视频侦率

 *********************************************************/
int32_t STB_VOD_Info_Vid_FrameRate ( uint32_t* rate )
{
	if ( rate == NULL )
	{
		return ERRNO_VOD_PARAM;
	}
	*rate = 25;

	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  获取在VOD的数据队列中保存的视频RTP的数量
  定义：
  int32_t STB_VOD_Info_Vid_PacketNum( uint32_t* num);
  参数：
Num:返回视频RTP的数量
返回：
错误代码

 *********************************************************/
int32_t STB_VOD_Info_Vid_PacketNum( uint32_t* num)
{
	if(num)
	{
		*num = vod_trans_get_v_packnum();
	}
	return ERRNO_VOD_NO_ERROR;
}


/**********************************************************
  说明：
  获取在VOD的数据队列中保存的视频RTP的数量
  定义：
  int32_t STB_VOD_Info_Aud_PacketNum ( uint32_t* num);
  参数：
Num:返回音频RTP的数量
返回：
错误代码

 *********************************************************/
int32_t STB_VOD_Info_Aud_PacketNum ( uint32_t* num)
{
	if(num)
	{
		*num = vod_trans_get_a_packnum();
	}
	return ERRNO_VOD_NO_ERROR;
}



/**********************************************************
  说明：
  返回当前影片使用的range类型是npt、clock、pts
  注：npt为相对时间    clock为绝对时间  pts为pts形式的时间
  定义：
  ErrorCode_t STB_VOD_Info_RangeType ( VOD_RANGETYPE_t* rangetype);
  参数：
Timemode:返回影片使用的是相对时间还是绝对时间
返回：
错误代码
 *********************************************************/
int32_t STB_VOD_Info_RangeType ( VOD_RANGE_TYPE_t * rtype )
{

	return ERRNO_VOD_NO_ERROR;
}


/**********************************************************
  说明：
  返回当前影片使用的range类型是npt、clock、pts
  定义：
  int32_t STB_VOD_Set_Debug_Level ( int32_t debuglevel )
  参数：
debuglevel: 调试级别
返回：
错误代码
 *********************************************************/
int32_t STB_VOD_Set_Debug_Level ( int32_t debuglevel )
{

	return ERRNO_VOD_NO_ERROR;
}

/**********************************************************
  说明：
  返回当前影片的进度播放情况，类型seekable, speed_scale
  定义：
  int32_t STB_VOD_Info_Seek ( int32_t* seekable, int32_t* speed_scale )
  参数：
  debuglevel: 调试级别
  返回：
  错误代码
 ********************************************************/
int32_t STB_VOD_Info_Seek ( int32_t* seekable, int32_t* speed_scale )
{
	 int trickmode = coship_get_trick_mode();
	 if(trickmode!=0){
		 *seekable = 1;
		 *speed_scale = abs(trickmode);
	 }else{
		 *seekable = 0;
		 *speed_scale = 0;
	 }
	return ERRNO_VOD_NO_ERROR;
}

#define VOD_TIMEOUT 3000

static int32_t parse_parameter_by_url(const char* srcurl, int64_t start, vod_start_param_t* param)
{
	char* p = NULL;
	char* p1 = NULL;

	if(srcurl == NULL)
		return -1;

	memset(param, 0 , sizeof(vod_start_param_t));
	if(strncmp(srcurl, "vod://", strlen("vod://")) == 0){
		int len = strlen("vod://");
		param->apptype = VOD_APP_COSHIP;
		param->io_type = IO_CABLE;
		//	param->io_type = IO_TCP;
		param->start_position = start/1000;
		if((p=strstr(srcurl, "--parameter")) != NULL)
		{
			char word[64];
			for(p--; ((*p==' ')&&(p>srcurl));) p--;
			p++;
			memcpy((char*)param->url, srcurl+len, p-srcurl-len);
			p1 = str_get_field(p, "position", word);
			if(p1)
				param->start_position = atoi(word)/1000;
			p1 = str_get_field(p, "hostip", word);
			if(p1)
				memcpy((char*)param->hostip, word, 64);
			p1 = str_get_field(p, "hostport", word);
			if(p1)
				param->hostport = atoi(word);
			p1 = str_get_field(p, "starttime", word);
			if(p1)
				param->reference_time = urlstr_to_time(word);
		}else{
			strncpy((char*)param->url, srcurl+len, sizeof(param->url)-1);
		}
		GxVod_debug("url      : %s\n", param->url);
		GxVod_debug("hostip   : %s\n", param->hostip);
		GxVod_debug("hostport : %d\n", param->hostport);
		GxVod_debug("position : %lld\n", param->start_position);
		GxVod_debug("ref time : %lld\n", param->reference_time);
		return 0;
	}else if(strncmp(srcurl, "wfd://", strlen("wfd://")) == 0){
		param->apptype = VOD_APP_WFD;
		param->io_type = IO_TCP;
		param->start_position = start/1000;
		if((p=strstr(srcurl, "--parameter")) != NULL)
		{
			gxlogd("not support parameter\n");
		}else{
			strncpy((char*)param->url, srcurl, sizeof(param->url)-1);
		}
		return 0;
	}

	return -1;
}

int32_t gxmedia_vod_play(const char* srcurl, int64_t start)
{
	vod_start_param_t param;

	if(parse_parameter_by_url(srcurl, start, &param)!=0)
		return GX_PLAYER_ERROR;

	STB_VOD_Init();
	if(STB_VOD_Play(&param, VOD_TIMEOUT*10, NULL, 0)){
		STB_VOD_Exit();
		return GX_PLAYER_ERROR;
	}
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_stop(void)
{
	STB_VOD_Stop(VOD_TIMEOUT);
	STB_VOD_Exit();
	vod_porting_event_seturl(NULL, NULL, NULL);
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_seek(int64_t seek_time, int flag)
{
	char range[32] = {0};
	int32_t start = 0, end = 0;

	seek_time = seek_time/1000;
	STB_VOD_Info_Position(&start, &end, NULL, NULL, NULL);
	GxVod_debug("%s %d start %d, end %d, seek_time %lld...\n", __FUNCTION__, __LINE__, start, end, seek_time);
	if((end-start) > 0 && (seek_time > (end-start)))
		return GX_PLAYER_ERROR;

	if(start > 360000){
		snprintf(range, 32, "%lld", seek_time+start);
		STB_VOD_Seek(range, VOD_RANGE_TYPE_CLOCK, VOD_TIMEOUT, NULL, 0);
	}else{
		snprintf(range, 32, "%lld", seek_time+start);
		STB_VOD_Seek(range, VOD_RANGE_TYPE_NPT, VOD_TIMEOUT, NULL, 0);
	}
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_pause(void)
{
	STB_VOD_Pause(0, VOD_TIMEOUT);
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_resume(void)
{
	STB_VOD_Resume(0, VOD_TIMEOUT);
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_get_time(int64_t* start, int64_t* position, int64_t* duration)
{
	int32_t t1 = 0, t2 = 0, t3 = 0;
	STB_VOD_Info_Position(&t1, &t2, &t3, NULL, NULL);
	if(start)
		*start = (int64_t)t1;
	if(position)
		*position = ((int64_t)(t3-t1))*1000;
	if(duration)
		*duration = ((int64_t)(t2-t1))*1000;
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_speed(int scale)
{
	STB_VOD_Scale(scale, VOD_TIMEOUT, NULL, 0);
	return GX_PLAYER_OK;
}

void gxmedia_vod_seturl(const char* player, const char* src_url, void* cb)
{
	vod_porting_event_seturl(player, src_url, cb);
	return;
}

extern void vod_porting_dvb_register(PlayerVodOption* option);
extern void vod_porting_dvb_unregister(void);
void STB_VOD_Register_Options(PlayerVodOption* opt)
{
	vod_porting_dvb_register(opt);
}


void STB_VOD_UnRegister_Options(void)
{
	vod_porting_dvb_unregister();
}
