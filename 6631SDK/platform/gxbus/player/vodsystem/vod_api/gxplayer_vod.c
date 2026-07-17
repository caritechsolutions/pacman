#include "gx_media.h"
#include "gx_mediamanager.h"
#include "../../gxplayer_internal.h"
#include "gxplayer_vod.h"

void GxPlayer_Register_VOD_Options(PlayerVodOption* opt)
{
	extern void STB_VOD_Register_Options(PlayerVodOption* opt);
	STB_VOD_Register_Options(opt);
}

void GxPlayer_unRegister_Vod_Options(void)
{
	extern void STB_VOD_UnRegister_Options(void);
	STB_VOD_UnRegister_Options();
}

status_t GxPlayer_MediaVodPlay(const char* name, const char* srcurl, int64_t start, void* cb)
{
	int ret;
	gxmedia_vod_seturl(name, srcurl, cb);
	ret = gxmedia_vod_play(srcurl, start);
	return ret;
}

status_t GxPlayer_MediaVodStop(const char* name)
{
	int ret;
	ret = gxmedia_vod_stop();
	return ret;
}

status_t GxPlayer_MediaVodSeek(const char* name, int64_t time, SeekFlag flag)
{
	int ret;
	ret = gxmedia_vod_seek(time, flag);
	return ret;
}

status_t GxPlayer_MediaVodPause(const char* name)
{
	int ret;
	ret = gxmedia_vod_pause();
	return ret;
}

status_t GxPlayer_MediaVodResume(const char* name)
{
	int ret;
	ret = gxmedia_vod_resume();
	return ret;
}

status_t GxPlayer_MediaVodSpeed(const char* name, float speed)
{
	int ret;
	ret = gxmedia_vod_speed(speed);
	return ret;
}

status_t GxPlayer_MediaVodGetTime(const char* name, int64_t* start, int64_t* position, int64_t* duration)
{
	int ret;
	ret = gxmedia_vod_get_time(start, position, duration);
	return ret;
}

status_t GxPlayer_MediaVodDvbPause(const char* name)
{
	GxPlayer* p;
	status_t ret;
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxmedia_vod_dvb_pause(p);
	PLAYER_MANAGE_UNLOCK();
	return ret;
}

status_t GxPlayer_MediaVodDvbResume(const char* name, int clear_data)
{
	GxPlayer* p;
	status_t ret;
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxmedia_vod_dvb_resume(p, clear_data);
	PLAYER_MANAGE_UNLOCK();
	return ret;
}

status_t GxPlayer_MediaVodDvbSeek(const char* name, int64_t seek_time, SeekFlag flag)
{
	GxPlayer* p;
	status_t ret;
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxmedia_vod_dvb_seek(p, seek_time, flag);
	PLAYER_MANAGE_UNLOCK();
	return ret;
}
