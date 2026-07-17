#include "gx_common.h"
#include "gxplayer_module.h"
#include "../vod_include/vod_common_def.h"

static int vod_flags_sync = 0;
static int PlayerIsPlay = 0;
static PlayerVodOption* vod_opt = NULL;

void vod_porting_dvb_register(PlayerVodOption* option)
{
	if(vod_opt == NULL)
		vod_opt = av_mallocz(sizeof(PlayerVodOption));
	memcpy(vod_opt, option, sizeof(PlayerVodOption));
}

void vod_porting_dvb_unregister(void)
{
	if(vod_opt){
		av_free(vod_opt);
		vod_opt = NULL;
	}
}
/*vod_porting_dvb_play_by_pid
说明：异步接口
通过音频是视频的PID参数直接播放一个节目。
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
u32VideoPid:视频PID 
u32AudioPid:音频PID
u32PcrPid:	
u32AudioEcmPid:
u32VideoEcmPid:
返回：
0: ok
Other: fail*/
int vod_porting_dvb_play_by_pid(
			unsigned int frequency,
			unsigned int symborate,
			unsigned int qam_mode,
			unsigned int u32VideoPid,
			unsigned int u32AudioPid,
			unsigned int u32PcrPid,
			unsigned int u32EmmPid,
			unsigned int u32AudioEcmPid,
			unsigned int u32VideoEcmPid
			)
{
	PlayerVodPlayParam param;

	vod_flags_sync = 0;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.PidParam.u32VideoPid = u32VideoPid;
	param.VodParam.PidParam.u32AudioPid = u32AudioPid;
	param.VodParam.PidParam.u32PcrPid = u32PcrPid;
	param.VodParam.PidParam.u32EmmPid = u32EmmPid;
	param.VodParam.PidParam.u32AudioEcmPid = u32AudioEcmPid;
	param.VodParam.PidParam.u32VideoEcmPid = u32VideoEcmPid;
	if(vod_opt && vod_opt->play_by_pid)
		vod_opt->play_by_pid(param , vod_flags_sync);

	PlayerIsPlay = 1;
	return 0;
}

/*vod_porting_dvb_play_by_serviceid
说明：异步接口
通过SERVICE ID 或者 PROGRAM NUMBER 播放一个节目。
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
serviceid :节目的serviceid or program number
返回：
0: ok
Other: fail
*/
int vod_porting_dvb_play_by_serviceid(unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned short serviceid
		)
{
	int ret = 0;
	PlayerVodPlayParam param;
	GxVod_debug("=========================================================================\n");
	GxVod_debug(" serviceid:%d  frequency:%d  symborate:%d  qam_mode:%d\n", serviceid, frequency, symborate, qam_mode);
	GxVod_debug("=========================================================================\n");

	vod_flags_sync = 0;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.serviceid = (unsigned int)serviceid;

	if(vod_opt && vod_opt->play_by_serviceid)
		ret = vod_opt->play_by_serviceid(param , vod_flags_sync);

	PlayerIsPlay = 1;
	return ret;
}

/*
vod_porting_dvb_play_by_pmtpid
说明：异步接口
有些情况下，前端只告诉一个节目PMT表的PID，这样没，我们可以通过先接收PMT，然后解析PMT表，从而得到音频和视频的PID，最后通过解析出来的参数播放一套节目。
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
pmtpid:PMT表的PID
返回：
0: ok
Other: fail*/
int vod_porting_dvb_play_by_pmtpid(
unsigned int frequency,
unsigned int symborate,
unsigned int qam_mode,
unsigned int pmtpid)
{
	PlayerVodPlayParam param;

	vod_flags_sync = 0;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.pmtpid = pmtpid;
	if(vod_opt && vod_opt->play_by_pmtid)
		vod_opt->play_by_pmtid(param , vod_flags_sync);

	PlayerIsPlay = 1;
	return 0;
}

/*vod_porting_dvb_play_by_pid_sync
说明：
通过音频是视频的PID参数直接播放一个节目。同步实现
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
u32VideoPid:视频PID
u32AudioPid:音频PID
u32PcrPid:
u32AudioEcmPid:
u32VideoEcmPid:
返回：
0: ok
Other: fail*/
int vod_porting_dvb_play_by_pid_sync(
			unsigned int frequency,
			unsigned int symborate,
			unsigned int qam_mode,
			unsigned int	u32VideoPid,
			unsigned int	u32AudioPid,
			unsigned int u32PcrPid,
			unsigned int u32EmmPid,
			unsigned int u32AudioEcmPid,
			unsigned int u32VideoEcmPid)
{
	PlayerVodPlayParam param;

	vod_flags_sync = 1;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.PidParam.u32VideoPid = u32VideoPid;
	param.VodParam.PidParam.u32AudioPid = u32AudioPid;
	param.VodParam.PidParam.u32PcrPid = u32PcrPid;
	param.VodParam.PidParam.u32EmmPid = u32EmmPid;
	param.VodParam.PidParam.u32AudioEcmPid = u32AudioEcmPid;
	param.VodParam.PidParam.u32VideoEcmPid = u32VideoEcmPid;
	if(vod_opt && vod_opt->play_by_pid)
		vod_opt->play_by_pid(param, vod_flags_sync);

	PlayerIsPlay = 1;
	return 0;
}


/*vod_porting_dvb_play_by_serviceid_sync
说明：
通过SERVICE ID 或者 PROGRAM NUMBER 播放一个节目。同步实现
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
serviceid :节目的serviceid or program number
返回：
0: ok
Other: fail
*/
int vod_porting_dvb_play_by_serviceid_sync(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned short serviceid
		)
{
	PlayerVodPlayParam param;
	GxVod_debug("=========================================================================\n");
	GxVod_debug(" serviceid:%d  frequency:%d  symborate:%d  qam_mode:%d\n", serviceid, frequency, symborate, qam_mode);
	GxVod_debug("=========================================================================\n");

	vod_flags_sync = 1;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.serviceid = (unsigned int)serviceid;
	if(vod_opt && vod_opt->play_by_serviceid)
		vod_opt->play_by_serviceid(param, vod_flags_sync);

	PlayerIsPlay = 1;
	return 0;
}

/*
vod_porting_dvb_play_by_pmtpid_sync
说明：同步实现
有些情况下，前端只告诉一个节目PMT表的PID，这样没，我们可以通过先接收PMT，然后解析PMT表，从而得到音频和视频的PID，最后通过解析出来的参数播放一套节目。
参数：
frequency:节目所在的频点，单位KHz
symborate:符号率，6875 or 6900
qam_mode:16, 32, 64, 128, 256
pmtpid:PMT表的PID
返回：
0: ok
Other: fail*/
int vod_porting_dvb_play_by_pmtpid_sync(
		unsigned int frequency,
		unsigned int symborate,
		unsigned int qam_mode,
		unsigned int pmtpid)
{
	PlayerVodPlayParam param;

	vod_flags_sync  = 1;
	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.pmtpid = pmtpid;
	if(vod_opt && vod_opt->play_by_pmtid)
		vod_opt->play_by_pmtid(param , vod_flags_sync);

	PlayerIsPlay = 1;
	return 0;
}

/*
说明：
暂停当前正在播放的电视节目
*/
int vod_porting_dvb_pause ( void )
{
	if(vod_opt && vod_opt->pause)
		vod_opt->pause(vod_flags_sync);
	return 0;
}

/*
说明：
恢复播放当前正在暂停的电视节目
*/
int vod_porting_dvb_resume( void )
{
	if(vod_opt && vod_opt->resume)
		vod_opt->resume(vod_flags_sync);
	return 0;
}


/* 
说明：
退出正在播放的电视节目
参数 
needclear: 退出时候是否清楚屏幕
*/
int vod_porting_dvb_stop( int needclear )
{
	if(PlayerIsPlay == 0)
		return 0;

	if(vod_opt && vod_opt->stop)
		vod_opt->stop(1, 1);

	PlayerIsPlay = 0;
	return 0;
}

/*
7.
说明：
对当前正在播放的电视节目做跳转操作
*/
int vod_porting_dvb_seek ( void )
{
	if(vod_opt && vod_opt->seek)
		vod_opt->seek(vod_flags_sync);
	return 0;
}

/*
说明：
对当前解析pmt table，同步实现
*/
int vod_porting_dvb_parse_pmt_sync( unsigned int frequency,
			unsigned int symborate,
			unsigned int qam_mode,
			unsigned int pmtpid,
			unsigned int *u32VideoPid,
			unsigned int *u32AudioPid,
			unsigned int *u32PcrPid,
			VOD_VideoFormat_t * videofmt,
			VOD_AudioFormat_t *audiofmt)
{
	PlayerVodPlayParam param;
	PlayerVodInfo infos = {0};

	param.frequency = frequency;
	param.symborate = symborate;
	param.qam_mode  = qam_mode;
	param.VodParam.pmtpid = pmtpid;
	if(vod_opt && vod_opt->get_info_by_pmtid)
		vod_opt->get_info_by_pmtid(param, &infos);
	*u32VideoPid = infos.u32VideoPid;
	*u32AudioPid = infos.u32AudioPid;
	*u32PcrPid   = infos.u32PcrPid;
	*videofmt    = (VOD_VideoFormat_t)infos.videofmt;
	*audiofmt    = (VOD_VideoFormat_t)infos.audiofmt;
	return 0;
}

/*
说明：
获取dvb当前播放的pid信息
*/
int vod_porting_dvb_get_current_pid(unsigned int *u32VideoPid,
									unsigned int *u32AudioPid,
									unsigned int *u32PcrPid)
{
	PlayerVodInfo infos = {0};
	if(vod_opt && vod_opt->get_info_by_pmtid)
		vod_opt->get_info(&infos);
	*u32VideoPid = infos.u32VideoPid;
	*u32AudioPid = infos.u32AudioPid;
	*u32PcrPid   = infos.u32PcrPid;
	return 0;
}

/*
说明：
获取当前解码的信息，video audio类型
*/
int vod_porting_dvb_get_decode_info(VOD_VideoFormat_t * videofmt,	
									VOD_AudioFormat_t *audiofmt)
{
	PlayerVodInfo infos = {0};
	if(vod_opt && vod_opt->get_info_by_pmtid)
		vod_opt->get_info(&infos);
	*videofmt    = (VOD_VideoFormat_t)infos.videofmt;
	*audiofmt    = (VOD_VideoFormat_t)infos.audiofmt;
	return 0;
}

void vod_porting_get_arecode(char* areaCode)
{
	int ret = -1;
	if(vod_opt && vod_opt->get_play_info)
		vod_opt->get_play_info(PLAYER_VOD_GET_ARECODE, &ret);
	if(ret >= 0)
		snprintf(areaCode, 32, "%d", ret);
	else
		snprintf(areaCode, 32, "11121");
	return;
}
