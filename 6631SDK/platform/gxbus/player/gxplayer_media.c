#include "gx_media.h"
#include "gx_mediamanager.h"
#include "gxplayer_internal.h"
#include "avutil/avstring.h"
#include "gx_avout.h"
#include "avout/ao.h"
#include "avout/vo.h"
#include "avout/so.h"

#define CHECK_PLAYER(p) if (p == NULL) return GX_PLAYER_ERROR;
#define CHECK_PLAYER_INFO_READY(p) if (p->status == PLAYER_STATUS_STOPPED || p->status == PLAYER_STATUS_PLAY_START) return GX_PLAYER_ERROR;

static int _player_check_url_type(const char* srcurl)
{
#define LOGO_TYPE   (1<<0)
#define VIDEO_TYPE  (1<<1)
#define AUDIO_TYPE  (1<<2)
#define AUDIO1_TYPE (1<<3)
	int type = 0;

	if (URL_IS_LOGO(srcurl)){
		type |= (LOGO_TYPE | VIDEO_TYPE);
	}else if (URL_IS_DVB(srcurl)){
		int apid = 0, apid1 = 0, vpid = 0;

		GxUrl_GetItem(srcurl, GX_URL_KEY_VPID, &vpid);
		GxUrl_GetItem(srcurl, GX_URL_KEY_APID, &apid);
		GxUrl_GetItem(srcurl, GX_URL_KEY_APID1, &apid1);

		if ((apid > 0) && (apid < 0x1fff))
			type |= AUDIO_TYPE;

		if ((vpid > 0) && (vpid < 0x1fff))
			type |= VIDEO_TYPE;

		if ((apid1 > 0) && (apid1 < 0x1fff))
			type |= AUDIO1_TYPE;
	}

	return type;
}

static int _player_check_source(const char *oldurl, const char *newurl)
{
	int source_chanage = 0;
	char *ptr1 = NULL, *ptr2 = NULL;
	GxOptions* op1 = GxOptions_Create();
	GxOptions* op2 = GxOptions_Create();

	GxOptions_Set_By_Url(op1, oldurl);
	GxOptions_Set_By_Url(op2, newurl);
	ptr1 = GxOptions_Get_By_Id(op1, " -H", OPTIONS_TS_SOURCE);
	ptr2 = GxOptions_Get_By_Id(op2, " -H", OPTIONS_TS_SOURCE);

	if (ptr1 && ptr2) {
		if (0 != strcmp(ptr1, ptr2)) {
			source_chanage = 1;
		}
	} else if ((!ptr1 && ptr2) || (ptr1 && !ptr2)) {
		source_chanage = 1;
	}

	GxOptions_Destory(op1);
	GxOptions_Destory(op2);
	return source_chanage;
}

static int _player_need_close_audio(const char *srcurl, const char *newurl)
{
	int acodec1 = -1, acodec2 = -1;
	AudioCodecType format1 = 0, format2 = 0;

	GxUrl_GetItem(srcurl, GX_URL_KEY_ACODEC, &acodec1);
	GxUrl_GetItem(newurl, GX_URL_KEY_ACODEC, &acodec2);

	if ((acodec1 == -1) || (acodec2 == -1))
		return 0;

	format1 = acodec_url2std(acodec1);
	format2 = acodec_url2std(acodec2);

	return GxAudioDecoder_CheckReOpen(format1, format2);
}

static int _player_need_reopen(GxPlayer *p, const char* newurl)
{
	int type1 = 0, type2 = 0;

	switch (p->status) {
	case PLAYER_STATUS_SHIFT_START:
	case PLAYER_STATUS_SHIFT_END:
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		return 1;
	default:
		if (!MEDIA_IS_DVB(p->media_play) || !URL_IS_DVB(newurl))
			return 1;
	}

	if (_player_need_close_audio(p->srcurl, newurl))
		return 1;

	if (_player_check_source(p->srcurl, newurl) != 0)
		return 1;

	type1 = _player_check_url_type(p->srcurl);
	type2 = _player_check_url_type(newurl);

	if (type1 && (type1 == type2)) {
		if (p->media_play)
			return 0;
	}

	return 1;
}

static int _player_ad_playing(GxPlayer *p)
{
	if ((p->audio1_pid > 0) && (p->audio1_pid < 0x1fff)) {
		return 1;
	}

	return 0;
}

static int _player_need_close_video(const char* url1, const char* url2)
{
	int type1 = 0, type2 = 0;

	type1 = _player_check_url_type(url1);
	type2 = _player_check_url_type(url2);

	if ((type1 & VIDEO_TYPE) && (!(type2 & VIDEO_TYPE))) {
		//prev program has video, new program no video
		return 1;
	}

	return 0;
}

static void _player_event(void* priv, PlayerStatus status, PlayerError error)
{
	if ( priv ) {
		GxPlayer* p = priv;
		if (status == PLAYER_STATUS_PLAY_END)
			p->play_end_time = GxMedia_GetDuration(p->media_play);
		GxPlayer_StatusReport(priv, status, error, NULL);
	}
}

static status_t _player_sub_unload(GxPlayer* p)
{
	int i;

	CHECK_PLAYER(p);

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++) {
		if (p->subblock[i].subload) {
			GxPlayer_SubClose(&p->subblock[i]);
			p->subblock[i].subload = 0;
		}
	}

	p->sub_file_count = 0;

	return 0;
}

static status_t _player_sub_pause(GxPlayer* p, int stop)
{
	int i;

	CHECK_PLAYER(p);

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++) {
		if (p->subblock[i].subload) {
			if (stop)
				GxPlayer_SubStop(&p->subblock[i]);
			else
				GxPlayer_SubPause(&p->subblock[i]);
		}
	}

	return 0;
}

static status_t _player_sub_resume(GxPlayer* p)
{
	int i;

	CHECK_PLAYER(p);

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++) {
		if (p->subblock[i].subload)
			GxPlayer_SubResume(&p->subblock[i]);
	}

	return 0;
}

static uint64_t _player_get_record_filesize(GxPlayer* p)
{
	uint64_t filesize = 0;

	if (p->media_record) {
		GxDumpFilter_Control(p->media_record->dumper, GX_DUMPER_GET_FILESIZE, &filesize);
		if (p->media_play && p->media_play->stream)
			p->media_play->stream->end_pos = filesize;
	}

	return filesize;
}

static uint64_t _player_get_record_druation(GxPlayer* p)
{
	uint64_t duration = 0;

	if (p->media_record) {
		GxDumpFilter_Control(p->media_record->dumper, GX_DUMPER_GET_DURATION, &duration);
		if (p->media_play && p->media_play->demuxer)
			p->media_play->demuxer->duration = duration;
	}

	return duration;
}

static uint64_t _player_get_record_seekmin(GxPlayer* p)
{
	uint64_t duration, seekmin = 0;
	struct vol_info info;

	if (p->media_record) {
		int volume_sizemb = 0;
		int volume_maxnum = 0, volume_maxtimes = 0;
		uint64_t volume_seekmb = 0;

		GxPlayer_SystemGet(PSYS_PVR_VOLUME_SIZEMB,   &volume_sizemb);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXNUM,   &volume_maxnum);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXTIMES, &volume_maxtimes);
		GxDumpFilter_Control(p->media_record->dumper, GX_DUMPER_GET_VOLINFO, &info);
		if (info.size/1024 <= 0)
			return 0;
		volume_seekmb = (uint64_t)(volume_maxnum-1)*volume_sizemb*(1024*1024);
		info.truesize = ((volume_seekmb>0)&&(info.truesize>volume_seekmb))?volume_seekmb:info.truesize;
		duration = _player_get_record_druation(p);
		seekmin  = (duration/1000) * ((info.size-info.truesize)/1024) / (info.size/1024) * 1000;
		if (volume_maxtimes > 0) {
			if (duration > (1000 * volume_maxtimes)) {
				uint64_t seek_ms = duration - (1000 * volume_maxtimes);
				seekmin = (seekmin > seek_ms) ? seekmin : seek_ms;
			}
		}
	}
	return seekmin;
}

static GxMedia* _player_media_new_player(GxPlayer* p, const char* srcurl, GxStream* stream)
{
	GxMediaPara MediaPara;
	GxMedia* media;

	if (p == NULL || srcurl == NULL)
		return NULL;

	av_debug_media_duty(p->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
	if (p->status != PLAYER_STATUS_SHIFT_START &&
			p->status != PLAYER_STATUS_SHIFT_PAUSE &&
			p->status != PLAYER_STATUS_SHIFT_RUNNING &&
			srcurl != p->srcurl)
	{
		strncpy(p->srcurl, srcurl, PLAYER_URL_LONG);
	}

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.player.url = srcurl;
	MediaPara.player.sub = 0;
	MediaPara.player.stream = stream;
	MediaPara.player.bandwidth = p->bandwidth;
	MediaPara.player.start = p->start_timems;
	MediaPara.player.event.func = _player_event;
	MediaPara.player.event.priv = p;
	MediaPara.debug = p->debug;

	MediaPara.player.demuxInfo.clock_en = 1;
	MediaPara.player.demuxInfo.video_pid = (p->video_pid  > 0) ? p->video_pid : p->playconfig.tsfilter.vpid;
	MediaPara.player.demuxInfo.audio_pid = (p->audio_pid  > 0) ? p->audio_pid : p->playconfig.tsfilter.apid;
	MediaPara.player.demuxInfo.ad_pid    = (p->audio1_pid > 0) ? p->audio1_pid: p->playconfig.tsfilter.apid1;
	MediaPara.player.demuxInfo.ad_codec  = (p->audio1_type);
	MediaPara.player.demuxInfo.sub_pid   = (p->sub_pid) > 0 ? p->sub_pid : -1;

	media = GxMediaManager_MediaCreate(GX_MEDIA_PLAYER, &MediaPara);
	if (media) {
		p->seekable = media->demuxer->seekable;
		p->speedable = media->demuxer->speedable;
	}
	av_debug_media_duty(p->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);

	return media;
}

static GxMedia* _player_media_restart_player(GxPlayer* p, const char* srcurl)
{
	GxMedia* media = NULL;
	GxMediaPara MediaPara;

	if (p == NULL || p->media_play == NULL)
		return NULL;

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.player.url = srcurl;
	MediaPara.player.sub = 0;
	MediaPara.player.bandwidth = p->bandwidth;
	MediaPara.player.start = p->start_timems;
	MediaPara.player.event.func = _player_event;
	MediaPara.player.event.priv = p;

	MediaPara.player.demuxInfo.clock_en = 1;
	MediaPara.player.demuxInfo.video_pid = (p->video_pid  > 0) ? p->video_pid : p->playconfig.tsfilter.vpid;
	MediaPara.player.demuxInfo.audio_pid = (p->audio_pid  > 0) ? p->audio_pid : p->playconfig.tsfilter.apid;
	MediaPara.player.demuxInfo.ad_pid    = (p->audio1_pid > 0) ? p->audio1_pid: p->playconfig.tsfilter.apid1;
	MediaPara.player.demuxInfo.ad_codec  = (p->audio1_type);
	MediaPara.player.demuxInfo.sub_pid   = (p->sub_pid   > 0) ? p->sub_pid : -1;

	media = GxMediaManager_MediaRestart(p->media_play, &MediaPara);

	if (media){
		p->seekable = media->demuxer->seekable;
		p->speedable = media->demuxer->speedable;
	}

	return media;
}

static GxMedia* _player_media_new_recorder(GxPlayer* p,const char* srcurl,const char* dsturl)
{
	GxMediaPara MediaPara;
	GxMedia* media;

	if (p == NULL || srcurl == NULL || dsturl == NULL)
		return NULL;

	if (p->media_play == NULL)
		strncpy(p->srcurl, srcurl, PLAYER_URL_LONG);
	strncpy(p->dsturl, dsturl, PLAYER_URL_LONG);

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.recorder.srcurl = srcurl;
	MediaPara.recorder.dsturl = dsturl;
	MediaPara.recorder.sub = 0;
	MediaPara.recorder.event.func = _player_event;
	MediaPara.recorder.event.priv = p;
	MediaPara.debug = p->debug;

	av_debug_media_duty(p->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
	media = GxMediaManager_MediaCreate(GX_MEDIA_RECORDER, &MediaPara);
	if (media && media->cloned)
	{
		p->media_record = media;
		strncpy(p->dsturl, media->dumper->s->url, PLAYER_URL_LONG);
		p->start_timems = _player_get_record_druation(p);
	}
	av_debug_media_duty(p->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);

	return media;
}

static status_t _player_media_extconfig(GxPlayer* p, GxMedia* media)
{
	GxRecordPVRControl pvr_ctrl;

	if (p->recorder) {
		GxMedia_SetRecordConfig(media, &p->recorder->recconfig, &p->recorder->recencrypt);
		pvr_ctrl.arg = (void *)&p->recorder->segconfig;
	} else
		pvr_ctrl.arg = NULL;
	pvr_ctrl.opt = GX_RECORD_PVR_NEW_SEGMENT;
	GxMedia_SetPVRControl(media, &pvr_ctrl);
	GxMedia_SetPlayTsFilterConfig(media, &p->playconfig.tsfilter);
	GxMedia_SetPlayTsCacheConfig(media, &p->playconfig.tscache);

	return GX_PLAYER_OK;
}

static status_t _player_media_config(GxPlayer* p, GxMedia* media)
{
	int ret = 0;

	av_debug_media_duty(p->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
	_player_media_extconfig(p, media);

	GxMedia_AudioDelay(media, p->audio_delayms);
	GxMedia_VideoDelay(media, p->video_delayms);

	ret = GxMedia_Config(media);
	av_debug_media_duty(p->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);

	return ret;
}

static status_t _player_media_run(GxPlayer* p, GxMedia* media)
{
	int ret = GX_PLAYER_ERROR;

	if (media)
		media->holdPause = 0;

	av_debug_media_duty(p->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
	ret = GxMedia_Run(media);
	av_debug_media_duty(p->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
	if (ret == GX_PLAYER_OK) {
		if ((p->media_play == media) && MEDIA_IS_DVB(media)) {
			GxUrl_GetItem(p->srcurl, GX_URL_KEY_APID,  &p->audio_pid);
			GxUrl_GetItem(p->srcurl, GX_URL_KEY_VPID,  &p->video_pid);
			GxUrl_GetItem(p->srcurl, GX_URL_KEY_SPID,  &p->sub_pid);
		}
	}

	return ret;
}

static void _player_media_stop(GxPlayer* p, GxMedia* media, int freeze, int alive)
{
	av_debug_media_duty(p->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
	GxMedia_Stop(media, freeze, alive);
	av_debug_media_duty(p->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
}

static status_t _player_media_pause_nolock(GxPlayer* p, GxMedia* media)
{
	if (media)
		media->holdPause = 1;
	GxMedia_Pause(media);

	return GX_PLAYER_OK;
}

static status_t _player_media_resume_nolock(GxPlayer* p, GxMedia* media)
{
	if (media)
		media->holdPause = 0;
	GxMedia_Resume(media);

	return GX_PLAYER_OK;
}

static status_t _player_media_pause(GxPlayer* p, GxMedia* media)
{
	PLAYER_MEDIA_LOCK(p);

	_player_media_pause_nolock(p, media);

	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

static status_t _player_media_resume(GxPlayer* p, GxMedia* media)
{
	PLAYER_MEDIA_LOCK(p);

	_player_media_resume_nolock(p, media);

	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

static status_t _player_media_destroy(GxPlayer* p, GxMedia** media, int freeze)
{
	if (media && *media) {
		GxMediaManager_MediaDestroy(*media, freeze);
		*media = NULL;
	}

	return GX_PLAYER_OK;
}

static status_t _player_set_window(GxPlayer* p, PlayerWindow* window)
{
	int ret = 0;
	PlayerWindow old_window = {0};

	ret = GxPlayer_SystemGet(PSYS_VOUT_VIEW, &old_window);
	if ((ret == GX_PLAYER_OK) &&
			(old_window.x == window->x) &&
			(old_window.y == window->y) &&
			(old_window.width == window->width) &&
			(old_window.height == window->height)) {
		p->rect = *window;
		return GX_PLAYER_OK;
	}

	ret = GxPlayer_SystemSet(PSYS_VOUT_VIEW, window);
	if (ret == GX_PLAYER_OK) {
		p->rect = *window;
		gxlogf("[Player]: [View]:x=%d,y=%d,w=%d,h=%d]\n",
				window->x,window->y,window->width,window->height);
	}

	return ret;
}

static status_t _player_media_zoom(GxPlayer* p, GxMedia* media)
{
	GxCore_MutexLock(p->windowmutex);

	if (MEDIA_HAS_VIDEO(media)) {
		if (p->rect.width == 0 || p->rect.height == 0) {
			DisplayScreen Screen;
			GxPlayer_SystemGet(PSYS_VOUT_SCREEN, &Screen);
			p->rect.x = p->rect.y = 0;
			p->rect.width = Screen.xres;
			p->rect.height = Screen.yres;
		}
		_player_set_window(p, &p->rect);
	}

	GxCore_MutexUnlock(p->windowmutex);

	return GX_PLAYER_OK;
}

static status_t _player_seek_restart(GxPlayer* p)
{
	char* url;
	int ret = GX_PLAYER_ERROR;

	if (p == NULL)
		return GX_PLAYER_ERROR;

	_player_media_destroy(p, &p->media_play, 1);

	if (p->status == PLAYER_STATUS_SHIFT_RUNNING ||
			p->status == PLAYER_STATUS_SHIFT_PAUSE ){
		url = p->dsturl;
	}
	else{
		url = p->srcurl;
	}

	p->media_play = _player_media_new_player(p, url, NULL);
	if (p->media_play == NULL)
		return ret;

	ret = _player_media_config(p, p->media_play);
	if (ret != GX_PLAYER_OK)
	{
		_player_media_destroy(p, &p->media_play, 0);
		gxloge("[Player]: Config Error!...\n");
		return ret;
	}

	switch(p->status)
	{
	case PLAYER_STATUS_PLAY_PAUSE:
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);
		break;
	case PLAYER_STATUS_SHIFT_PAUSE:
		GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);
		break;
	default:
		GxPlayer_StatusReport(p, p->status, p->error, p->srcurl);
		break;
	}

	_player_media_zoom(p, p->media_play);

	_player_media_run(p, p->media_play);
	p->freeze_begin_cbk(p);

	gxlogf("Seek Restart !!!\n");

	return GX_PLAYER_OK;
}

static status_t _player_media_restart(GxPlayer* p, uint64_t start_timems)
{
	if (MEDIA_NO_RESTART_STREAM(p->media_play)){
		PLAYER_MEDIA_LOCK(p);

		gxlogd("restart time %lld\n", start_timems);

		p->start_timems = start_timems;
		p->media_play = _player_media_restart_player(p, p->srcurl);
		if (p->media_play == NULL){
			PLAYER_MEDIA_UNLOCK(p);
			return GX_PLAYER_ERROR;
		}

		if (_player_media_config(p, p->media_play) != GX_PLAYER_OK)
		{
			_player_media_destroy(p, &p->media_play, 0);
			gxloge("[Player]: Restart Config Error!...\n");
			PLAYER_MEDIA_UNLOCK(p);
			return GX_PLAYER_ERROR;
		}
		GxPlayer_StatusReport(p, p->status, p->error, p->srcurl);

		_player_media_zoom(p, p->media_play);
		_player_media_run (p, p->media_play);
		p->freeze_begin_cbk(p);

		gxlogf("Seek Restart Restart!!!\n");
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_OK;

	}else{
		PLAYER_STATUS_MASK(p);
		_player_media_destroy(p, &p->media_play, 1);
		PLAYER_STATUS_UNMASK(p);
		return _player_seek_restart(p);
	}
	return GX_PLAYER_OK;
}

static status_t _player_media_volume(GxPlayer* p, GxMedia* media)
{
	if (MEDIA_HAS_AUDIO(media))
	{
		int private;

		GxPlayer_SystemGet(PSYS_AOUT_PRIVATE, &private);

		if (private)
		{
			GxPlayer_SystemSet(PSYS_AOUT_VOLUME, &p->volume);
		}
	}

	return GX_PLAYER_OK;
}

static status_t _player_media_play(GxPlayer* p, const char* srcurl, GxStream* stream)
{
	int ret = GX_PLAYER_ERROR;

	GxPlayer_SystemGet(PSYS_FREEZE_ENABLE, &p->freezen);

	_player_media_destroy(p, &p->media_play, p->freezen);

	GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_START, p->error, (char*)srcurl);
	p->media_play = _player_media_new_player(p, srcurl, stream);
	if (p->media_play == NULL)
		goto error_out;

	PLAYER_RUNTIME("config", ret = _player_media_config(p, p->media_play));
	if (ret != GX_PLAYER_OK)
	{
		_player_media_destroy(p, &p->media_play, 0);
		gxloge("[Player]: Config Error!...\n");
		goto error_out;
	}

	av_debug_player_zoom_duty(p->debug, AV_TICK_ST_BEGIN);
	_player_media_volume(p, p->media_play);
	_player_media_zoom(p, p->media_play);
	av_debug_player_zoom_duty(p->debug, AV_TICK_ST_END);
	GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->media_play->errInfo, p->srcurl);

	PLAYER_RUNTIME("run", (ret = _player_media_run(p, p->media_play)));

	if (MEDIA_HAS_VIDEO(p->media_play) && p->media_play->errInfo != PLAYER_ERROR_VIDEO_DECODER_ERROR) {
		p->freeze_begin_cbk(p);
	}
	else if (p->has_video) {
		gxplayer_video_hide(p);
		p->has_video = 0;
	}

	return ret;

error_out:
	if (p->has_video){
		gxplayer_video_hide(p);
		p->has_video = 0;
	}
	return ret;
}

static status_t _player_prepare(GxPlayer* p)
{
	status_t ret = GX_PLAYER_OK;

	switch(p->status)
	{
	case PLAYER_STATUS_STOPPED:
	case PLAYER_STATUS_ERROR:
	case PLAYER_STATUS_RECORD_END:
	case PLAYER_STATUS_RECORD_FULL:
	case PLAYER_STATUS_PLAY_PAUSE:
		{
			_player_media_destroy(p, &p->media_play, p->freezen);
			_player_media_destroy(p, &p->media_record, p->freezen);
			break;
		}
	case PLAYER_STATUS_PLAY_START:
	case PLAYER_STATUS_PLAY_RUNNING:
	case PLAYER_STATUS_PLAY_END:
		{
			_player_media_destroy(p, &p->media_record, p->freezen);
			break;
		}
	case PLAYER_STATUS_SHIFT_START:
	case PLAYER_STATUS_SHIFT_END:
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		{
			PLAYER_STATUS_MASK(p);
			_player_media_destroy(p, &p->media_play, p->freezen);
			_player_media_destroy(p, &p->media_record, p->freezen);
			PLAYER_STATUS_UNMASK(p);
			p->status = PLAYER_STATUS_STOPPED;
			break;
		}
	default:
		ret = GX_PLAYER_ERROR;
		gxlogf("[Player]: Busy !...");
		break;
	}

	gxplayer_reset(p);
	return ret;
}

static status_t _player_media_record(GxPlayer* p,const char* srcurl,const char* file)
{
	int ret;

	if (p==NULL || srcurl==NULL)
		return GX_PLAYER_ERROR;

	gxlogf("[Recoder]: URL  = \"%s\"\n",srcurl);
	gxlogf("[Recoder]: File = \"%s\"\n",file);

	PLAYER_MEDIA_LOCK(p);
	p->media_record = _player_media_new_recorder(p,srcurl,file);
	if (p->media_record == NULL){
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_ERROR;
	}

	if (p->media_record->cloned == 0)
	{
		ret = _player_media_config(p, p->media_record);
		if (ret != GX_PLAYER_OK)
		{
			_player_media_destroy(p, &p->media_record, 0);
			gxloge("[Player]: New Recorder Config Error!\n");
			PLAYER_MEDIA_UNLOCK(p);
			return GX_PLAYER_ERROR;
		}

		_player_media_run(p, p->media_record);
	}

	PLAYER_MEDIA_UNLOCK(p);
	return GX_PLAYER_OK;
}

static status_t _player_media_record2(GxPlayer* p,const char* srcurl,const char* file)
{
	int ret;

	if (p==NULL || srcurl==NULL)
		return GX_PLAYER_ERROR;

	gxlogf("[Recoder]: URL  = \"%s\"\n",srcurl);
	gxlogf("[Recoder]: File = \"%s\"\n",file);

	PLAYER_MEDIA_LOCK(p);
	p->media_play = _player_media_new_recorder(p, srcurl, file);
	if (p->media_play == NULL) {
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_ERROR;
	}

	ret = _player_media_config(p, p->media_play);
	if (ret != GX_PLAYER_OK) {
		_player_media_destroy(p, &p->media_play, 0);
		gxloge("[Player]: New Recorder Config Error!\n");
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_ERROR;
	}
	_player_media_run(p, p->media_play);
	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

static PISubBlock* _player_find_subblock(GxPlayer* p, PlayerSubtitle* subdata)
{
	int i;
	PISubBlock* subblock=NULL;

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++)
	{
		if (p->subblock[i].subload==1 && p->subblock[i].subout==subdata)
			subblock = &p->subblock[i];
	}

	return subblock;
}

static status_t _player_shift_pause(GxPlayer* p)
{
	if (p == NULL)
		return GX_PLAYER_ERROR;

	_player_sub_pause(p, 1);

	_player_media_stop(p, p->media_play, 1, 0);

	p->freeze_abort_cbk(p);

	p->start_timems = _player_get_record_druation(p);

	gxlogf("[Player]: TimeShift Pause !...\n");

	GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_START, p->error, p->srcurl);

	return GX_PLAYER_OK;
}

static status_t _player_shift_resume(GxPlayer* p)
{
	int ret = GX_PLAYER_ERROR;

	if (p == NULL || p->media_record == NULL)
		return GX_PLAYER_ERROR;

	p->media_play = _player_media_new_player(p, p->dsturl, NULL);
	if (p->media_play == NULL)
		return ret;

	if (_player_media_config(p, p->media_play) != GX_PLAYER_OK)
	{
		_player_media_destroy(p, &p->media_play, 0);
		gxloge("[Player]: Config Error!...\n");
		return ret;
	}

	gxlogf("[Player]: TimeShift Resume !...\n");

	GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);

	_player_media_zoom(p, p->media_play);

	_player_media_run(p, p->media_play);

	_player_sub_resume(p);

	p->freeze_begin_cbk(p);

	return GX_PLAYER_OK;
}

static void _player_clr_recorder(PlayerRecorder *recorder, int freed)
{
	if (recorder == NULL)
		return;

	if (recorder->segconfig.prefix) {
		av_free(recorder->segconfig.prefix);
		recorder->segconfig.prefix = NULL;
	}
	if (recorder->segconfig.desc) {
		av_free(recorder->segconfig.desc);
		recorder->segconfig.desc = NULL;
	}

	if (freed)
		av_free(recorder);

	return;
}

status_t player_shift2dvb(GxPlayer* p)
{
	GxMediaPara MediaPara;

	PLAYER_MEDIA_LOCK(p);

	_player_sub_pause(p, 1);
	p->start_timems = 0;
	GxMedia_GetCurrentTime(p->media_play, &p->start_timems);
	p->seekable = 1;
	p->speedable = 1;
	p->speed = 1000;
	p->cur_timems = 0;

	_player_media_destroy(p, &p->media_play, 1);

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.player.url = p->srcurl;
	MediaPara.player.sub = 0;
	MediaPara.player.start = p->start_timems;
	MediaPara.player.event.func = _player_event;
	MediaPara.player.event.priv = p;
	MediaPara.player.demuxInfo.clock_en = 1;
	MediaPara.player.demuxInfo.video_pid = (p->video_pid > 0) ? p->video_pid : p->playconfig.tsfilter.vpid;
	MediaPara.player.demuxInfo.audio_pid = (p->audio_pid > 0) ? p->audio_pid : p->playconfig.tsfilter.apid;
	MediaPara.player.demuxInfo.ad_pid    = (p->audio1_pid > 0) ? p->audio1_pid: p->playconfig.tsfilter.apid1;
	MediaPara.player.demuxInfo.ad_codec  = (p->audio1_type);
	MediaPara.player.demuxInfo.sub_pid   = (p->sub_pid   > 0) ? p->sub_pid : -1;

	p->media_play = GxMedia_New(GX_MEDIA_PLAYER, &MediaPara);

	GxMedia_AudioDelay(p->media_play, p->audio_delayms);
	GxMedia_VideoDelay(p->media_play, p->video_delayms);
	GxMedia_Config(p->media_play);
	GxMedia_Run(p->media_play);

	if (p->audio1_pid > 0) {
		unsigned int mix = 0x0;

		mix |= ((p->audio_mask  == 1) ? 0 : (0x1 << 0));
		mix |= ((p->audio1_mask == 1) ? 0 : (0x1 << 1));
		GxMedia_AdAudioMix(p->media_play, mix);
	}

	p->freeze_begin_cbk(p);

	_player_sub_resume(p);
	gxlogf("[Shift]  2 DVB !...\n");
	GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_HOLD_RUNNING, p->error, p->srcurl);

	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

status_t player_shift2file(GxPlayer* p)
{
	GxMediaPara MediaPara;
	PLAYER_MEDIA_LOCK(p);

	_player_sub_pause(p, 1);
	_player_media_destroy(p, &p->media_play, 1);

	memset(&MediaPara, 0, sizeof(MediaPara));
	MediaPara.player.url = p->dsturl;
	MediaPara.player.sub = 0;
	MediaPara.player.start = p->start_timems;
	MediaPara.player.event.func = _player_event;
	MediaPara.player.event.priv = p;
	MediaPara.player.demuxInfo.clock_en = 1;
	MediaPara.player.demuxInfo.video_pid = (p->video_pid  > 0) ? p->video_pid : p->playconfig.tsfilter.vpid;
	MediaPara.player.demuxInfo.audio_pid = (p->audio_pid  > 0) ? p->audio_pid : p->playconfig.tsfilter.apid;
	MediaPara.player.demuxInfo.ad_pid    = (p->audio1_pid > 0) ? p->audio1_pid: p->playconfig.tsfilter.apid1;
	MediaPara.player.demuxInfo.ad_codec  = (p->audio1_type);
	MediaPara.player.demuxInfo.sub_pid   = (p->sub_pid > 0) ? p->sub_pid : -1;

	p->speed = 1000;
	p->cur_timems = 0;
	p->media_play = GxMedia_New(GX_MEDIA_PLAYER, &MediaPara);
	if (p->media_play == NULL)
	{
		gxlogf("[Shift] File Not Exist!...\n");
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_END, p->error, p->srcurl);
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_ERROR;
	}

	GxMedia_AudioDelay(p->media_play, p->audio_delayms);
	GxMedia_VideoDelay(p->media_play, p->video_delayms);
	GxMedia_Config(p->media_play);
	GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);

	GxMedia_Run(p->media_play);

	p->freeze_begin_cbk(p);

	gxlogf("[Shift]  2 FILE !...\n");

	_player_sub_resume(p);
	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

status_t player_reset(GxPlayer* p)
{
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	p->cur_timems = 0;
	_player_sub_pause(p, 1);
	if (p->speed < 0) {
		uint64_t seek_minms = 0;

		PLAYER_STATUS_MASK(p);
		p->speed = 1000;
		gxplayer_get_time(p, &p->start_timems, NULL, &seek_minms);
		if (MEDIA_FB_EOF(p->media_play))
			p->start_timems = seek_minms;
		if (p->start_timems < seek_minms)
			p->start_timems = seek_minms;
		PLAYER_STATUS_UNMASK(p);

		if (_player_media_restart(p, p->start_timems) == GX_PLAYER_ERROR)
			gxplayer_video_hide(p);
	} else {
		unsigned int i = 0;

		GxMedia_Reset(p->media_play, 1);
		for (i = 0; i < PLAYER_MAX_SUB_LOAD; i++) {
			if (p->subblock[i].subload) {
				GxPlayer_SubResetSource(&p->subblock[i]);
			}
		}
	}
	_player_sub_resume(p);
	p->freeze_begin_cbk(p);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	return GX_PLAYER_OK;
}

status_t player_reset_audio(GxPlayer *p)
{
	GxMedia_AudioReset1(p->media_play);
	return GX_PLAYER_OK;
}

status_t player_stop_timeshift(GxPlayer* p)
{
	CHECK_PLAYER(p);

	PLAYER_STATUS_MASK(p);
	if (!MEDIA_IS_DVB(p->media_play)) {
		_player_media_play(p, p->srcurl, NULL);
	}
	_player_media_destroy(p, &p->media_record, 0);
	gxplayer_eventlist_clr(p);
	PLAYER_STATUS_UNMASK(p);
	return GX_PLAYER_OK;
}

status_t player_stop_record(GxPlayer* p)
{
	CHECK_PLAYER(p);

	PLAYER_STATUS_MASK(p);
	_player_media_destroy(p, &p->media_record, 0);
	PLAYER_STATUS_UNMASK(p);
	return GX_PLAYER_OK;
}

status_t player_pause(GxPlayer* p)
{
	PLAYER_MEDIA_LOCK(p);

	_player_media_pause_nolock(p, p->media_play);

	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

status_t gxplayer_open(GxPlayer* p, PlayerWindow* window)
{
	status_t ret = GX_PLAYER_ERROR;

	CHECK_PLAYER(p);
	if (window && window->width && window->height) {
		p->rect = *window;
		gxplayer_video_view(p, window);
	}

	ret = gxplayer_play_background(p);
	if (ret == GXCORE_SUCCESS)
		p->pre_opened = 1;

	return ret;
}

status_t gxplayer_close(GxPlayer* p)
{
	CHECK_PLAYER(p);
	if (p->pre_opened) {
		gxplayer_media_stop(p);
		_player_media_destroy(p, &p->media_record, 0);
		_player_media_destroy(p, &p->media_play, 0);
		gxplayer_interrupt_clr(p);
		GxPlayer_Destroy(p);
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_media_play_step1(GxPlayer* p, const char* srcurl, PlayerPlayInfo *info)
{
	CHECK_PLAYER(p);
	CHECK_NULL(srcurl);

	av_debug_begin_duty (p->debug, AV_TICK_FUNC_PLAY);
	av_debug_srcurl_duty(p->debug, p->srcurl);
	av_debug_dsturl_duty(p->debug, p->dsturl);

	GxPlayer_SystemGet(PSYS_FREEZE_ENABLE, &p->freezen);

	p->play_stage = PLAY_STAGE_GOT_URL;
	_player_sub_unload(p);

	if (info) {
		p->start_timems = info->start;
		p->volume = info->volume;
		if (info->rect.width && info->rect.height)
			p->rect = info->rect;
	}

	if (URL_IS_DVB(srcurl) || URL_IS_LOGO(srcurl)) {
		status_t ret;

		PLAYER_MEDIA_LOCK(p);

		p->speed = 1000;
		p->audio_mask = 0;
		p->has_video |= MEDIA_HAS_VIDEO(p->media_play);
		if (p->media_play == NULL ||
				_player_need_reopen(p, srcurl) ||
				_player_ad_playing(p)) {

			PLAYER_STATUS_MASK(p);
			if (_player_ad_playing(p) ||
					!_player_need_close_video(p->srcurl, srcurl)) {
				if (_player_need_close_video(p->srcurl, srcurl))
					p->freezen = 0;
				else
					p->freeze_end_cbk(p, 1);
				_player_media_destroy(p, &p->media_play, p->freezen);
				p->audio1_mask = 0;
				p->audio1_pid  = 0;
			} else {
				if (p->status != PLAYER_STATUS_STOPPED)
					_player_media_stop(p, p->media_play, 0, 0);
			}
			PLAYER_STATUS_UNMASK(p);

			av_memhole_flush();
			ret = _player_prepare(p);
			if (ret != GX_PLAYER_OK)
				goto errout;
			ret = _player_media_play(p, srcurl, NULL);
			goto errout;
		}

		PLAYER_STATUS_MASK(p);
		GxMedia_SwitchUrl(p->media_play, srcurl);
		PLAYER_RUNTIME("stop", _player_media_stop(p, p->media_play, p->freezen, 1));
		gxplayer_reset(p);
		PLAYER_STATUS_UNMASK(p);

		strncpy(p->srcurl, srcurl, PLAYER_URL_LONG);
		av_memhole_flush();

		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_START, p->error, (char*)srcurl);
		PLAYER_RUNTIME("config", ret = _player_media_config(p, p->media_play));
		if (ret != GX_PLAYER_OK)
		{
			_player_media_destroy(p, &p->media_play, 0);
			gxloge("[Player]: Config Error!...\n");
			goto errout;
		}

		_player_media_volume(p, p->media_play);
		av_debug_player_zoom_duty(p->debug, AV_TICK_ST_BEGIN);
		_player_media_zoom(p, p->media_play);
		av_debug_player_zoom_duty(p->debug, AV_TICK_ST_END);
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->media_play->errInfo, p->srcurl);

		PLAYER_RUNTIME("run", (ret = _player_media_run(p, p->media_play)));
		p->freeze_begin_cbk(p);
errout:
		p->play_stage = PLAY_STAGE_PLAY_FINISH;
		{
			GxUrl_GetItem(p->srcurl, GX_URL_KEY_APID1,   &p->audio1_pid);
			GxUrl_GetItem(p->srcurl, GX_URL_KEY_ACODEC1, &p->audio1_type);
			p->audio1_type = acodec_url2std(p->audio1_type);
			p->audio1_mask = 0;
		}

		PLAYER_MEDIA_UNLOCK(p);
		av_debug_end_duty(p->debug);
		return ret;

	}
	else {
		if (p->media_play && (p->media_play->runflag&GX_MEDIA_RUN_RECORD)) {
			//录制一个节目,播放另外一个节目,需要不同的player
			if (!(p->media_play->runflag&GX_MEDIA_RUN_PLAY)) {
				if (strcmp(p->srcurl, srcurl) != 0)
					return GX_PLAYER_ERROR;

				GxMedia_StartPlay(p->media_play);
				av_debug_end_duty(p->debug);
			}
		} else
			gxplayer_stream_put(p, srcurl, info);
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_media_play_step2(GxPlayer* p, struct stream_node* stream)
{
	status_t ret;
	GxStream* pstream;

	p->play_stage = PLAY_STAGE_PLAY_START;

	gxplayer_interrupt_set(p); //exit v1 network play step2 stream open
	PLAYER_MEDIA_LOCK(p);
	gxplayer_interrupt_set(p); //v2 network stop clear flags, play will set flags
	PLAYER_STATUS_MASK(p);
	if (stream->abort == 0) {
		p->has_video |= MEDIA_HAS_VIDEO(p->media_play);
		_player_media_destroy(p, &p->media_play, p->freezen);
		av_memhole_flush();
	}
	PLAYER_STATUS_UNMASK(p);
	gxplayer_interrupt_clr(p);
	av_debug_player_prep_duty(p->debug, AV_TICK_ST_BEGIN);

	if (stream->abort) {
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_OK;
	}

	pstream = GxStream_Open(stream->url, GX_STREAM_READ, p->bandwidth);
	if (stream->abort || pstream == NULL){
		if (!stream->abort){
			if (p->has_video){
				gxplayer_video_hide(p);
				p->has_video = 0;
			}
			strncpy(p->srcurl, stream->url, PLAYER_URL_LONG);
			int error_status = PLAYER_ERROR_NO_ERROR;
			GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
			GxPlayer_StatusReport(p, PLAYER_STATUS_ERROR, ((error_status > 0)?error_status:PLAYER_ERROR_NO_DATA_SOURCE), stream->url);
			if (error_status > 0) {
				error_status = -1;
				GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
			}
		}
		if (p->play_stage == PLAY_STAGE_PLAY_START)
			p->play_stage = PLAY_STAGE_PLAY_FINISH;
		GxStream_Close(pstream);
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_OK;
	}
	av_debug_player_prep_duty(p->debug, AV_TICK_ST_END);

	p->speed = 1000;
	p->audio_mask = 0;
	p->audio_pid  = stream->info.audio_pid;
	p->audio1_pid = stream->info.audio1_pid;
	p->video_pid  = stream->info.video_pid;
	p->sub_pid    = stream->info.sub_pid;
	p->isquiet = pstream->isquiet;

	ret = _player_prepare(p);
	if (ret != GX_PLAYER_OK)
		goto errout;

	ret = _player_media_play(p, stream->url, pstream);
	if (ret != GX_PLAYER_OK)
		goto errout;

	if (MEDIA_HAS_AUDIO(p->media_play))
		p->audio_pid = p->media_play->sh_audio->priv.id;

	if (MEDIA_HAS_AUDIO1(p->media_play))
		p->audio1_pid = p->media_play->sh_audio1->priv.id;

	if (MEDIA_HAS_VIDEO(p->media_play))
		p->video_pid = p->media_play->sh_video->priv.id;

errout:
	if (p->play_stage == PLAY_STAGE_PLAY_START)
		p->play_stage = PLAY_STAGE_PLAY_FINISH;
	PLAYER_MEDIA_UNLOCK(p);
	return ret;
}

status_t gxplayer_media_play_wait(GxPlayer* p)
{
	CHECK_PLAYER(p);

	while (p->play_stage != PLAY_STAGE_PLAY_FINISH) {
		GxCore_ThreadDelay(100);
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_media_play_exit(GxPlayer* p)
{
	CHECK_PLAYER(p);

	av_debug_begin_duty (p->debug, AV_TICK_FUNC_STOP);
	av_debug_srcurl_duty(p->debug, p->srcurl);
	av_debug_dsturl_duty(p->debug, p->dsturl);

	GxPlayer_SystemGet(PSYS_FREEZE_ENABLE, &p->freezen);

	PLAYER_STATUS_MASK(p);
	gxplayer_interrupt_set(p); //exit v1 network play step2 stream open
	PLAYER_MEDIA_LOCK(p);
	gxplayer_interrupt_set(p); //v2 network play step2 clear flags, stop will set flags
	gxplayer_stream_clr(p);
	gxplayer_playlist_clr(p);
	p->has_video |= MEDIA_HAS_VIDEO(p->media_play);
	_player_media_stop(p, p->media_play, p->freezen, 0);
	p->freeze_end_cbk(p, p->freezen);
	_player_media_destroy(p, &p->media_play, p->freezen);
	PLAYER_MEDIA_UNLOCK(p);
	gxplayer_interrupt_clr(p);
	PLAYER_STATUS_UNMASK(p);
	av_debug_end_duty(p->debug);

	return GX_PLAYER_OK;
}

status_t gxplayer_media_record(GxPlayer* p, const char* srcurl, const char* file)
{
	status_t ret = GX_PLAYER_ERROR;

	CHECK_PLAYER(p);

	if (URL_IS_DVB(srcurl) || URL_IS_LOGO(srcurl) || URL_IS_FM(srcurl)) {
		switch(p->status)
		{
		case PLAYER_STATUS_STOPPED:
		case PLAYER_STATUS_ERROR:
		case PLAYER_STATUS_PLAY_END:
		case PLAYER_STATUS_RECORD_END:
		case PLAYER_STATUS_RECORD_FULL:
			{
				PLAYER_STATUS_MASK(p);
				_player_media_destroy(p, &p->media_play, 0);
				_player_media_destroy(p, &p->media_record, 0);
				PLAYER_STATUS_UNMASK(p);
				ret = _player_media_record(p,srcurl,file);
				break;
			}
		default:
			gxlogf("[Player]: Busy !...");
			break;
		}

		if (ret == GX_PLAYER_OK)
			GxPlayer_StatusReport(p, PLAYER_STATUS_RECORD_RUNNING, p->error, p->srcurl);
	} else {
		if (p->media_play && (p->media_play->runflag&GX_MEDIA_RUN_PLAY)) {
			//播放一个节目,录制另外一个节目,需要不同的player
			if (!(p->media_play->runflag&GX_MEDIA_RUN_RECORD)) {
				if (strcmp(p->srcurl, srcurl) != 0)
					return GX_PLAYER_ERROR;

				ret = GxMedia_StartRecord(p->media_play, file);
			}
		} else {
			_player_media_destroy(p, &p->media_play, 0);
			ret = _player_media_record2(p, srcurl, file);
		}
	}

	return ret;
}

status_t gxplayer_media_pause(GxPlayer* p)
{
	int ret, shift_enable;

	CHECK_PLAYER(p);

	switch (p->status) {
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		{
			gxplayer_get_time(p, &(p->start_timems), NULL, NULL);
			_player_sub_pause(p, (MEDIA_IS_DVB(p->media_play) ? 1 : 0));

			if (MEDIA_DUMP_EOF(p->media_record))
			{
				_player_media_stop(p, p->media_play, 1, 0);
				GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_HOLD_PAUSE, p->error, p->srcurl);
			}
			else
			{
				_player_media_stop(p, p->media_play, 1, 0);
				p->freeze_abort_cbk(p);
				GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_START, p->error, p->srcurl);
			}

			return GX_PLAYER_OK;
		}
	case PLAYER_STATUS_PLAY_RUNNING:
	case PLAYER_STATUS_SHIFT_RUNNING:
		if (MEDIA_IS_DVB(p->media_play)) {
			GxPlayer_SystemGet(PSYS_PVR_SHIFT_ENABLE, &shift_enable);
			_player_sub_pause(p, 1);

			if (shift_enable)
			{
				int shift_dmxid;
				char *dsturl = NULL;
				char *srcurl = av_mallocz(PLAYER_URL_LONG+1);
				if (srcurl == NULL)
					return GX_PLAYER_ERROR;
				strncpy(srcurl, p->srcurl, PLAYER_URL_LONG);
				GxPlayer_SystemGet(PSYS_PVR_SHIFT_FILE, &dsturl);
				GxPlayer_SystemGet(PSYS_PVR_SHIFT_DMXID, &shift_dmxid);
				GxUrl_SetItem(srcurl, GX_URL_KEY_DMXID, shift_dmxid, PLAYER_URL_LONG);

				PLAYER_STATUS_MASK(p);
				ret = _player_media_record(p, srcurl, dsturl);
				av_free(srcurl);
				PLAYER_STATUS_UNMASK(p);

				if (ret == GX_PLAYER_OK) {
					_player_shift_pause(p);
					return GX_PLAYER_OK;
				}
				else {
					_player_media_destroy(p, &p->media_record, 1);
					_player_media_stop(p, p->media_play, 1, 0);
					GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_PAUSE, PLAYER_ERROR_DUMPER_ERROR, p->srcurl);
					return GX_PLAYER_OK;
				}
			}

			_player_media_pause(p, p->media_play);
			_player_media_stop(p, p->media_play, 1, 0);
		}
		else {
			gxplayer_get_time(p, &(p->start_timems), NULL, NULL);
			_player_media_pause(p, p->media_play);
		}

		if (p->status == PLAYER_STATUS_PLAY_RUNNING || p->status == PLAYER_STATUS_ERROR)
			GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_PAUSE, p->error, p->srcurl);
		else if (p->status == PLAYER_STATUS_SHIFT_RUNNING)
			GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_PAUSE, p->error, p->srcurl);
		break;
	case PLAYER_STATUS_RECORD_RUNNING:
		_player_media_pause(p, p->media_record);
		GxPlayer_StatusReport(p, PLAYER_STATUS_RECORD_PAUSE, p->error, p->srcurl);
		break;
	case PLAYER_STATUS_PLAY_END:
	case PLAYER_STATUS_PLAY_PAUSE:
		return GX_PLAYER_OK;
	default:
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_media_resume(GxPlayer* p)
{
	int ret;

	CHECK_PLAYER(p);

	switch(p->status)
	{
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_PLAY_PAUSE:
	case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
		{
			if (MEDIA_IS_DVB(p->media_play))
			{
				PLAYER_MEDIA_LOCK(p);
				_player_media_play(p, p->srcurl, NULL);
				_player_sub_resume(p);
				PLAYER_MEDIA_UNLOCK(p);
			}
			else {
				uint64_t seek_min = _player_get_record_seekmin(p);
				if (seek_min > p->start_timems) {
					GxMedia_SeekPause(p->media_play, seek_min, GX_DEMUXER_SEEK_ABSOLUTE);
					p->speed = 1000;
					p->start_timems = seek_min;
					if (p->media_play)
						p->media_play->holdPause = 0;
					GxMedia_SeekResume(p->media_play);
				} else {
					_player_media_resume(p, p->media_play);
				}
			}

			if (p->status == PLAYER_STATUS_PLAY_PAUSE)
				GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);
			if (p->status == PLAYER_STATUS_SHIFT_HOLD_PAUSE)
				GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_HOLD_RUNNING, p->error, p->srcurl);
			else if (p->status == PLAYER_STATUS_SHIFT_PAUSE)
				GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);
			return GX_PLAYER_OK;
		}
	case PLAYER_STATUS_RECORD_PAUSE:
		{
			_player_media_resume(p, p->media_record);
			GxPlayer_StatusReport(p, PLAYER_STATUS_RECORD_RUNNING, p->error, p->srcurl);
			return GX_PLAYER_OK;
		}
	case PLAYER_STATUS_SHIFT_START:
		{
			uint64_t dur = _player_get_record_druation(p);
			uint64_t seekmin = _player_get_record_seekmin(p);

			_player_media_destroy(p, &p->media_play, 1);
			p->start_timems = seekmin >= p->start_timems ? seekmin : p->start_timems;
#define TIME_STEP (2000)
			if ((dur <= TIME_STEP) || ((dur - p->start_timems) <= TIME_STEP)) {
				ret = player_shift2dvb(p);
				return ret;
			}

			ret = _player_shift_resume(p);
			_player_sub_resume(p);
			if (ret == GX_PLAYER_ERROR)
			{
				gxlogf("[Player]: TimeShift End !...\n");
				GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_END, p->error, p->srcurl);
				return GX_PLAYER_ERROR;
			}
			else
				return GX_PLAYER_OK;
		}
	case PLAYER_STATUS_PLAY_END:
		return GX_PLAYER_OK;
	default:
		break;
	}

	return GX_PLAYER_ERROR;
}

status_t gxplayer_media_stop(GxPlayer* p)
{
	CHECK_PLAYER(p);

	av_debug_begin_duty (p->debug, AV_TICK_FUNC_STOP);
	av_debug_srcurl_duty(p->debug, p->srcurl);
	av_debug_dsturl_duty(p->debug, p->dsturl);

	GxPlayer_SystemGet(PSYS_FREEZE_ENABLE, &p->freezen);

	p->play_stage = PLAY_STAGE_PLAY_FINISH;
	gxplayer_interrupt_set(p); //exit v1 network play step2 stream open
	_player_sub_unload(p);
	PLAYER_MEDIA_LOCK(p);
	gxplayer_interrupt_set(p); //v2 network play step2 clear flags, stop will set flags
	gxplayer_stream_clr(p);
	gxplayer_playlist_clr(p);
	PLAYER_STATUS_MASK(p);
	if (MEDIA_HAS_VIDEO(p->media_play)){
		int flag = 0;
		GxPlayer_SystemSet(PSYS_VOUT_CLOSED, &flag);
	}
	{
		int error_status = 0;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
	}
	_player_clr_recorder(p->recorder, 1);
	if (p->pre_opened == 0) {
		_player_media_stop(p, p->media_play, 0, 0);
		av_debug_player_freeze_duty(p->debug, AV_TICK_ST_BEGIN);
		p->freeze_end_cbk(p, 1);
		av_debug_player_freeze_duty(p->debug, AV_TICK_ST_END);
		_player_media_destroy(p, &p->media_record, 0);
		_player_media_destroy(p, &p->media_play, 0);
		gxplayer_interrupt_clr(p);
		if (p->has_video)
			gxplayer_video_hide(p);
		PLAYER_MEDIA_UNLOCK(p);
		av_debug_end_duty(p->debug);
		GxPlayer_Destroy(p);
	} else {
		_player_media_stop(p, p->media_play, p->freezen, 0);
		p->freeze_end_cbk(p, 1);
		gxplayer_interrupt_clr(p);
		PLAYER_STATUS_UNMASK(p);
		GxPlayer_StatusReport(p, PLAYER_STATUS_STOPPED, p->error, p->srcurl);
		PLAYER_MEDIA_UNLOCK(p);
		av_debug_end_duty(p->debug);
	}
	av_memhole_flush();

	return GX_PLAYER_OK;
}

status_t gxplayer_media_flush(GxPlayer* p)
{
	CHECK_PLAYER(p);

	GxMedia_Reset(p->media_play, 0);
	gxplayer_play_background(p);

	return GX_PLAYER_OK;
}

static void _player_media_set_seeking(GxPlayer* p, int flag)
{
	if (p && p->media_play){
		p->media_play->seeking = flag;
	}
}

status_t gxplayer_media_seek(GxPlayer* p, int64_t time, SeekFlag seek_flag)
{
	int seekflag;
	uint64_t cur = 0, duration = 0, seek_min = 0;
	int ret = GX_PLAYER_ERROR;

#define SEEK_STEP (3000)

	CHECK_PLAYER(p);

	av_debug_begin_duty (p->debug, AV_TICK_FUNC_SEEK);
	av_debug_srcurl_duty(p->debug, p->srcurl);
	av_debug_dsturl_duty(p->debug, p->dsturl);

	if (p->status == PLAYER_STATUS_SHIFT_START) {
		gxplayer_media_resume(p);
	}

	if (p->media_play == NULL ||
			p->seekable == 0 ||
			p->isquiet ||
			p->media_play->demuxer == NULL) {
		ret = GX_PLAYER_ERROR;
		goto seek_exit;
	}

	//p->freeze_abort_cbk(p); /*freeze_begin_cbk以后，未能freeze_end_cbk，在resume以后，导致音频被静音
	gxplayer_get_time(p, &cur, &duration, &seek_min);

	PLAYER_MEDIA_LOCK(p);
	PLAYER_STATUS_MASK(p);
	_player_media_set_seeking(p, 1);

	switch (p->status) {
	case PLAYER_STATUS_PLAY_RUNNING:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		_player_media_pause_nolock(p, p->media_play);
		break;
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_PLAY_PAUSE:
	case PLAYER_STATUS_PLAY_END:
		break;
	default:
		PLAYER_STATUS_UNMASK(p);
		_player_media_set_seeking(p, 0);
		PLAYER_MEDIA_UNLOCK(p);
		ret = GX_PLAYER_ERROR;
		goto seek_exit;
	}

	switch (seek_flag) {
	case SEEK_ORIGIN_SET:
		seekflag = GX_DEMUXER_SEEK_ABSOLUTE;
		break;
	case SEEK_ORIGIN_CUR:
		seekflag = GX_DEMUXER_SEEK_RELATIVE;
		break;
	case SEEK_ORIGIN_END:
		time = duration - time;
		seekflag = GX_DEMUXER_SEEK_ABSOLUTE;
		break;
	case SEEK_ORIGIN_PERCENT:
		seekflag = GX_DEMUXER_SEEK_PERCENT;
		break;
	default:
		seekflag = GX_DEMUXER_SEEK_ABSOLUTE;
		break;
	}

	if (((duration>0) && (time + SEEK_STEP >= duration)) ||
			((seekflag & GX_DEMUXER_SEEK_PERCENT) && time>=100)) {
		unsigned int margin = 1000;

		switch (p->status) {
		case PLAYER_STATUS_SHIFT_PAUSE:
		case PLAYER_STATUS_SHIFT_RUNNING:
			margin = 2000;
		case PLAYER_STATUS_PLAY_PAUSE:
		case PLAYER_STATUS_PLAY_RUNNING:
			if (time + margin > duration) {
				PLAYER_STATUS_UNMASK(p);
				p->speed = 1000;
				GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_END, p->error, p->srcurl);
				_player_media_set_seeking(p, 0);
				PLAYER_MEDIA_UNLOCK(p);
				ret = GX_PLAYER_OK;
				goto seek_exit;
			}
			time = duration - SEEK_STEP;
			break;
		case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
			if (time + 1000 > duration) {
				PLAYER_STATUS_UNMASK(p);
				_player_media_resume_nolock(p, p->media_play);
				_player_media_set_seeking(p, 0);
				PLAYER_MEDIA_UNLOCK(p);
				ret = GX_PLAYER_OK;
				goto seek_exit;
			}
			time = duration - SEEK_STEP;
			break;
		case PLAYER_STATUS_PLAY_END:
			time = duration - SEEK_STEP;
			break;
		default:
			PLAYER_STATUS_UNMASK(p);
			_player_media_resume_nolock(p, p->media_play);
			_player_media_set_seeking(p, 0);
			PLAYER_MEDIA_UNLOCK(p);
			ret = GX_PLAYER_ERROR;
			goto seek_exit;
		}
	} else if (time < 0) {
		PLAYER_STATUS_UNMASK(p);
		_player_media_resume_nolock(p, p->media_play);
		_player_media_set_seeking(p, 0);
		PLAYER_MEDIA_UNLOCK(p);
		ret = GX_PLAYER_ERROR;
		goto seek_exit;
	} else if (seek_min > 0) {
		if (seek_min > time) {
			gxlogi("%s %d: seek_min %lld, seek_time %d\n", __func__, __LINE__, seek_min, time);
			time = seek_min;
		}
	}

	if (p->status == PLAYER_STATUS_SHIFT_HOLD_RUNNING) {
		p->start_timems = time;
		PLAYER_STATUS_UNMASK(p);
		_player_media_set_seeking(p, 0);
		PLAYER_MEDIA_UNLOCK(p);
		ret = player_shift2file(p);
		goto seek_exit;
	} else {
		if (p->media_play && p->media_play->demuxer && p->media_play->demuxer->seekable == 0) {
			PLAYER_STATUS_UNMASK(p);
			_player_media_resume_nolock(p, p->media_play);
			_player_media_set_seeking(p, 0);
			PLAYER_MEDIA_UNLOCK(p);
			ret = GX_PLAYER_ERROR;
			goto seek_exit;
		}

		if (p->media_play)
			p->media_play->holdPause = 1;

		if ( ((seekflag & GX_DEMUXER_SEEK_PERCENT)  && (time>=100 || time<0)) ||
				((seekflag & GX_DEMUXER_SEEK_RELATIVE) && (cur+time>=duration || cur+time<0)) ||
				((seekflag & GX_DEMUXER_SEEK_ABSOLUTE) && (time>=duration || time<0))) {
			PLAYER_STATUS_UNMASK(p);
			_player_media_resume_nolock(p, p->media_play);
			_player_media_set_seeking(p, 0);
			PLAYER_MEDIA_UNLOCK(p);
			ret = GX_PLAYER_ERROR;
			goto seek_exit;
		}

		av_debug_media_duty(p->debug, AV_TICK_OP_SEEK_PAUSE, AV_TICK_ST_BEGIN);
		_player_sub_pause(p, 1);
		ret = GxMedia_SeekPause(p->media_play, time, seekflag);
		av_debug_media_duty(p->debug, AV_TICK_OP_SEEK_PAUSE, AV_TICK_ST_END);

		p->speed = 1000;
		PLAYER_STATUS_UNMASK(p);
		if (p->media_play)
			p->media_play->holdPause = 0;
		p->freeze_begin_cbk(p);
		p->start_timems = time;

		av_debug_media_duty(p->debug, AV_TICK_OP_SEEK_RESUME, AV_TICK_ST_BEGIN);
		_player_sub_resume(p);
		GxMedia_SeekResume(p->media_play);
		av_debug_media_duty(p->debug, AV_TICK_OP_SEEK_RESUME, AV_TICK_ST_END);

		if (MEDIA_IS_NETSTREAM(p->media_play) || MEDIA_IS_NETDEMUXER(p->media_play)) {
			if (ret == GX_PLAYER_ERROR) {
				GxPlayer_StatusReport(p, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DATA_SOURCE, p->srcurl);
			} else {
				if (p->status == PLAYER_STATUS_PLAY_PAUSE)
					GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);
			}
		} else {
			switch (p->status) {
			case PLAYER_STATUS_PLAY_END:
			case PLAYER_STATUS_PLAY_PAUSE:
				GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);
				break;
			case PLAYER_STATUS_SHIFT_PAUSE:
				GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);
				break;
			default:
				break;
			}
		}
		_player_media_set_seeking(p, 0);
		PLAYER_MEDIA_UNLOCK(p);
		goto seek_exit;
	}

seek_exit:
	av_debug_end_duty(p->debug);
	return ret;
}

status_t gxplayer_media_speed(GxPlayer* p, float speed)
{
	uint64_t cur, duration;
	PlayerStatusInfo info;
	int gx_speed = (int)(speed*1000);

	CHECK_PLAYER(p);
	gxplayer_media_play_wait(p);

	if (p->status == PLAYER_STATUS_SHIFT_START) {
		gxplayer_media_resume(p);
	}

	if (!p->media_play && p->media_record)
		gxplayer_media_resume(p);

	while (gxplayer_get_status(p, &info) != GXCORE_SUCCESS)
		GxCore_ThreadDelay(10);
	if ((p->vcodec.state == AVCODEC_ERROR) &&
			(p->vcodec.err_code != AVCODEC_ERR_NONE)) {
		gxloge("%s %d: video state 0x%x err_code 0x%x\n",
				__func__, __LINE__, p->vcodec.state, p->vcodec.err_code);
		return GX_PLAYER_ERROR;
	}

	gxplayer_get_time(p, &cur, &duration, NULL);

	if (p->media_play == NULL ||
			gx_speed == 0 ||
			p->isquiet ||
			p->media_play->demuxer == NULL ||
			((MEDIA_HAS_VIDEO(p->media_play)) && (p->speedable == 0)) ||
			((MEDIA_HAS_VIDEO(p->media_play)) && (duration <= 0)) ||
			((!MEDIA_HAS_VIDEO(p->media_play)) && (speed < 0.5 || speed > 2.0)) ||
			(( MEDIA_HAS_VIDEO(p->media_play)) &&
			 ((speed < 0.125 && speed > -0.125) || speed > 128.9 || speed < -128.9)))
		return GX_PLAYER_ERROR;

	switch(p->status)
	{
	case PLAYER_STATUS_PLAY_RUNNING:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_SHIFT_START:
		if (p->speed == gx_speed) {
			return GX_PLAYER_OK;
		}
		_player_media_pause(p, p->media_play);
		break;
	case PLAYER_STATUS_PLAY_PAUSE:
	case PLAYER_STATUS_SHIFT_PAUSE:
		if (p->speed == gx_speed) {
			gxplayer_media_resume(p);
			return GX_PLAYER_OK;
		}
		break;
	case PLAYER_STATUS_PLAY_END:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		if (gx_speed < 1000)
		{
			if (gxplayer_media_seek(p, 1500, SEEK_ORIGIN_END) != GX_PLAYER_OK) {
				return GX_PLAYER_ERROR;
			}
			_player_media_pause(p, p->media_play);
			break;
		}
		return GX_PLAYER_ERROR;
	default:
		return GX_PLAYER_ERROR;
	}

	p->speed = gx_speed;

	GxMedia_SetSpeed(p->media_play, gx_speed);
	if (gx_speed == 1000)
		p->freeze_begin_cbk(p);

	_player_media_resume(p, p->media_play);
	switch (p->status) {
	case PLAYER_STATUS_PLAY_PAUSE:
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);
		break;
	case PLAYER_STATUS_SHIFT_START:
	case PLAYER_STATUS_SHIFT_PAUSE:
		GxPlayer_StatusReport(p, PLAYER_STATUS_SHIFT_RUNNING, p->error, p->srcurl);
		break;
	default:
		break;
	}

	gxlogf("[Player]: [SPEED]---->>>> %.1fX\n",speed);
	return GX_PLAYER_OK;
}

status_t gxplayer_track_add(GxPlayer* p, GxStreamTrackAdd* track)
{
	int ret = GX_PLAYER_ERROR;

	CHECK_PLAYER(p);
	CHECK_PLAYER(track);

	if (MEDIA_IS_DVB(p->media_play))
	{
		ret = GxMedia_TrackAdd(p->media_play, track);
	}

	return ret;
}

status_t gxplayer_track_delete(GxPlayer* p, GxStreamTrackDel* track)
{
	int ret = GX_PLAYER_ERROR;

	CHECK_PLAYER(p);
	CHECK_PLAYER(track);

	if (MEDIA_IS_DVB(p->media_play))
	{
		ret = GxMedia_TrackDel(p->media_play, track);
	}

	return ret;
}

status_t gxplayer_video_view(GxPlayer* p, PlayerWindow* window)
{
	int ret;
	CHECK_PLAYER(p);
	CHECK_PLAYER(window);

	GxCore_MutexLock(p->windowmutex);

	ret = _player_set_window(p, window);

	GxCore_MutexUnlock(p->windowmutex);

	return ret;
}

status_t gxplayer_video_clip(GxPlayer* p, PlayerWindow* window)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(window);

	gxlogf("[Player]: [Clip]:x=%d,y=%d,w=%d,h=%d]\n",
			window->x,window->y,window->width,window->height);

	return GxPlayer_SystemSet(PSYS_VOUT_CLIP, window);
}

status_t gxplayer_video_hide(GxPlayer* p)
{
	PlayerLayer layer = SNAP_LAYER_VPP;

	return GxPlayer_SystemSet(PSYS_VOUT_HIDE, &layer);
}

status_t gxplayer_video_show(GxPlayer* p)
{
	int flag;
	PlayerLayer layer = SNAP_LAYER_VPP;

	GxPlayer_SystemGet(PSYS_VOUT_CLOSED, &flag);
	if (flag == 1)
		return 0;
	return GxPlayer_SystemSet(PSYS_VOUT_SHOW, &layer);
}

status_t gxplayer_play_background(GxPlayer* p)
{
	PlayerLayer layer = SNAP_LAYER_VPP;

	return GxPlayer_SystemSet(PSYS_BACKGROUND, &layer);
}

status_t gxplayer_video_userhide(GxPlayer* p)
{
	if (p) {
		int flag;
		GxPlayer_SystemGet(PSYS_VOUT_CLOSED, &flag);
		if (flag == 0) {
			flag = 1;
			PlayerLayer layer = SNAP_LAYER_VPP;
			GxPlayer_SystemSet(PSYS_VOUT_HIDE, &layer);
			GxPlayer_SystemSet(PSYS_VOUT_CLOSED, &flag);
		}
		return 0;
	}

	return -1;
}

status_t gxplayer_video_usershow(GxPlayer* p)
{
	if (p) {
		int flag;
		GxPlayer_SystemGet(PSYS_VOUT_CLOSED, &flag);
		if (flag == 1) {
			PlayerVideoShow show;

			flag = 0;
			show.force = p->pre_opened ? 1: 0;
			show.layer = SNAP_LAYER_VPP;

			GxPlayer_SystemSet(PSYS_VOUT_CLOSED, &flag);
			GxPlayer_SystemSet(PSYS_VOUT_SHOW, &show);
		}
		return 0;
	}

	return -1;
}

status_t gxplayer_media_save(GxPlayer* p, PlayerMediaContent content)
{
	CHECK_PLAYER(p);

	if (content && PLAYER_MEDIA_VIDEO) {
		p->freeze_abort_cbk(p);
	}

	return GxMedia_Save(p->media_play, content);
}

status_t gxplayer_media_restore(GxPlayer* p, PlayerMediaContent content)
{
	CHECK_PLAYER(p);

	return GxMedia_Restore(p->media_play, content);
}

status_t gxplayer_media_stop_play(GxPlayer* p)
{
	CHECK_PLAYER(p);

	if (p->media_play && (p->media_play->runflag&GX_MEDIA_RUN_PLAY))
		GxMedia_StopPlay(p->media_play, p->freezen);

	if (p->media_play && !(p->media_play->runflag&(GX_MEDIA_RUN_PLAY|GX_MEDIA_RUN_RECORD)))
		gxplayer_media_stop(p);

	return GX_PLAYER_OK;
}

status_t gxplayer_media_destroy_play(GxPlayer* p)
{
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	_player_media_destroy(p, &p->media_play, p->freezen);
	p->has_video = 0;
	PLAYER_MEDIA_UNLOCK(p);

	return GX_PLAYER_OK;
}

status_t gxplayer_media_stop_record(GxPlayer* p)
{
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);

	if (p->media_record) {
		_player_clr_recorder(p->recorder, 0);
		_player_media_stop(p, p->media_record, 1, 0);
		_player_media_destroy(p, &p->media_record, p->freezen);
	}

	if (p->media_play && (p->media_play->runflag&GX_MEDIA_RUN_RECORD))
		GxMedia_StopRecord(p->media_play);

	GxPlayer_StatusReport(p, PLAYER_STATUS_STOPPED, p->error, p->srcurl);

	PLAYER_MEDIA_UNLOCK(p);
	return GX_PLAYER_OK;
}

int gxplayer_media_read(GxPlayer* p, PlayerReadType type, uint8_t* buffer, int len)
{
	int rlen = -1;
	CHECK_PLAYER(p);

	if (URL_IS_DVB(p->srcurl) || URL_IS_LOGO(p->srcurl)) {
		if (p->media_record && p->media_record->stream)
			rlen = GxMedia_Read(p->media_record, type, buffer, len);
	} else {
		if (p->media_play && p->media_play->stream)
			rlen = GxMedia_Read(p->media_play, type, buffer, len);
	}
	return rlen;
}

int gxplayer_media_read_subcc(GxPlayer* p, uint8_t* buffer, int len)
{
	unsigned int rlen = 0;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	rlen = GxMedia_Read(p->media_play, PLAYER_READ_VDECODER_SUBCC, buffer, len);
	PLAYER_MEDIA_UNLOCK(p);

	return rlen;
}

status_t gxplayer_audio_sync(GxPlayer* p, int timems)
{
	CHECK_PLAYER(p);

	if (p->media_play && p->media_play->demuxer)
	{
		if (GxMedia_AudioSync(p->media_play, timems) == GX_PLAYER_OK)
			return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

status_t gxplayer_audio_switch(GxPlayer* p, int pid, AudioCodecType type, char* url)
{
	int ret = GX_PLAYER_OK, codec;

	CHECK_PLAYER(p);
	CHECK_NULL(p->media_play);

	if (!GX_SPEED_NORMAL(p->speed) || !MEDIA_HAS_AUDIO(p->media_play))
		return GX_PLAYER_ERROR;

	if (URL_IS_DVB(p->srcurl)) {
		if (pid <= 0 || pid >= 0x1fff)
			return GX_PLAYER_ERROR;

		GxUrl_GetItem(p->srcurl, GX_URL_KEY_APID, &p->audio_pid);
		codec = acodec_std2url(type);
		if ((pid != p->audio_pid) && (pid != p->audio1_pid)) {
			GxUrl_SetItem(p->srcurl, GX_URL_KEY_APID,   pid,   PLAYER_URL_LONG);
			GxUrl_SetItem(p->srcurl, GX_URL_KEY_ACODEC, codec, PLAYER_URL_LONG);
			GxMedia_SwitchUrl(p->media_play, p->srcurl);
		}
	}
	else {
		int index = pid;
		PlayerProgInfo *pinfo = av_mallocz(sizeof(PlayerProgInfo));
		if (pinfo == NULL)
			return GX_PLAYER_ERROR;
		gxplayer_get_proginfo(p, p->media_play, pinfo);
		if (index < 0 || index >= pinfo->audio_num || pinfo->audio_num <= 1) {
			av_free(pinfo);
			return GX_PLAYER_ERROR;
		}
		if (p->audio_pid == -1)
			p->audio_pid = pinfo->audio[pinfo->cur_audio_id].id;

		pid = pinfo->audio[index].id;
		av_free(pinfo);
	}

	if (p->audio1_pid == pid)
		return GX_PLAYER_ERROR;

	if (p->audio_pid == pid)
		return GX_PLAYER_OK;

	p->audio_pid   = pid;
	p->audio_mask  = 0;
	p->audio1_pid  = -1;
	p->audio1_mask = 0;
	if (p->media_play) {
		ret = GxMedia_AudioSwitch(p->media_play, pid, type);
		if (ret == GX_PLAYER_ERROR)
			GxPlayer_StatusReport(p, p->status, p->media_play->errInfo, p->srcurl);
	}

	return ret;
}

status_t gxplayer_ad_audio_enable(GxPlayer* p, int pid, AudioCodecType type, int dmxid)
{
	int codec;
	int ret = GX_PLAYER_OK;

	CHECK_PLAYER(p);

	if (p->status == PLAYER_STATUS_SHIFT_START)
		return GX_PLAYER_ERROR;

	if (URL_IS_DVB(p->srcurl)) {
		if (pid <= 0 || pid >= 0x1fff)
			return GX_PLAYER_ERROR;
	} else {
		int index = pid;
		PlayerProgInfo *pinfo = av_mallocz(sizeof(PlayerProgInfo));
		if (pinfo == NULL)
			return GX_PLAYER_ERROR;
		gxplayer_get_proginfo(p, p->media_play, pinfo);
		if (index < 0 || index >= pinfo->audio_num || pinfo->audio[index].track_type != AUDIO_DESCRIPTOR_TRACK) {
			av_free(pinfo);
			return GX_PLAYER_ERROR;
		}

		pid = pinfo->audio[index].id;
		av_free(pinfo);
	}

	if (p->audio1_pid == pid) {
		if (p->audio1_mask) {
			unsigned int mix = (0x1 << 1);

			mix |= ((p->audio_mask  == 1) ? 0x0 : (0x1 << 0));
			GxMedia_AdAudioMix(p->media_play, mix);
			p->audio1_mask = 0;
		}
		return GX_PLAYER_OK;
	} else if (p->audio_pid == pid) {
		if (p->audio_mask) {
			unsigned int mix = (0x1 << 0);

			mix |= ((p->audio1_mask == 1) ? 0x0 : (0x1 << 1));
			GxMedia_AdAudioMix(p->media_play, mix);
			p->audio_mask = 0;
		}
		return GX_PLAYER_OK;
	}

	p->audio1_pid   = pid;
	p->audio1_mask  = 0;
	p->audio1_type  = type;
	p->audio1_dmxid = (URL_IS_DVB(p->srcurl)) ? dmxid : -1;

	ret = GxMedia_AdAudioEnable(p->media_play, pid, type);
	if (ret != GX_PLAYER_OK) {
		p->audio1_pid = -1;
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_ad_audio_disable(GxPlayer* p, int pid)
{
	CHECK_PLAYER(p);

	if (p->status == PLAYER_STATUS_SHIFT_START)
		return GX_PLAYER_ERROR;

	if ((pid != -1) &&
			(p->audio_pid != pid) &&
			(p->audio1_pid != pid)) {
		gxloge("%s %d: pid not match(%x %x - %x)\n", __func__, __LINE__, pid);
		return GX_PLAYER_ERROR;
	}

	if (URL_IS_DVB(p->srcurl)) {
		if (p->audio1_pid <= 0)
			return GX_PLAYER_OK;
	}

	if ((pid == -1) ||
			(p->audio1_pid == pid)) {
		unsigned int mix = 0;

		mix |= ((p->audio_mask  == 1) ? 0x0 : (0x1 << 0));
		GxMedia_AdAudioMix(p->media_play, mix);
		p->audio1_mask = 1;
	} else if (p->audio_pid == pid) {
		unsigned int mix = 0;

		mix |= ((p->audio1_mask == 1) ? 0x0 : (0x1 << 1));
		GxMedia_AdAudioMix(p->media_play, mix);
		p->audio_mask = 1;
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_switch(GxPlayer* p, PlayerSubtitle* subdata, PlayerSubPID pid)
{
	int i;
	PISubBlock* subblock = NULL;
	PlayerSubPID subPID;

	CHECK_PLAYER(p);

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++) {
		if (p->subblock[i].subload==1 && p->subblock[i].subout==subdata) {
			subblock = &p->subblock[i];
			break;
		}
	}

	if (subblock == NULL)
		return GX_PLAYER_ERROR;

	if (URL_IS_DVB(p->srcurl)) {
		if (pid.pid <= 0 || pid.pid >= 0x1fff)
			return GX_PLAYER_ERROR;
		subPID = pid;
	}
	else {
		int index = pid.pid;
		PlayerProgInfo *pinfo = av_mallocz(sizeof(PlayerProgInfo));
		if (pinfo == NULL)
			return GX_PLAYER_ERROR;
		gxplayer_get_proginfo(p, p->media_play, pinfo);
		if (index < 0 || index >= pinfo->sub_num) {
			av_free(pinfo);
			return GX_PLAYER_ERROR;
		}

		if (pinfo->sub[index].type != pinfo->sub[pinfo->cur_sub_id].type) {
			PISubParam params;
			params.pid = pinfo->sub[index].pid;
			params.url = (pinfo->sub[index].type==PLAYER_SUB_TYPE_FILE)?p->sub_file_url:NULL;
			GxPlayer_SubSwitch(subblock, pinfo->sub[index].type, &params);

			p->cur_sub_idx = index;
			av_free(pinfo);
			return GX_PLAYER_OK;
		}

		subPID = pinfo->sub[index].pid;
		p->cur_sub_idx = index;
		av_free(pinfo);
	}

	if ((p->sub_pid == subPID.pid) && (p->major == subPID.major) && (p->minor == subPID.minor))
		return GX_PLAYER_OK;

	GxPlayer_SubSwitchStream(subblock, subPID);

	p->sub_pid = subPID.pid;
	p->major   = subPID.major;
	p->minor   = subPID.minor;

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_sync(GxPlayer* p, PlayerSubtitle* subdata, int timems)
{
	int i;
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock = NULL;

	CHECK_PLAYER(p);

	for (i=0;i<PLAYER_MAX_SUB_LOAD;i++)
	{
		if (p->subblock[i].subload==1 && p->subblock[i].subout==subdata)
			subblock = &p->subblock[i];
	}

	if (subblock == NULL)
		return ret;

	subblock->delay = timems;

	GxPlayer_SubSync(subblock,timems);

	return GX_PLAYER_OK;

}

PlayerSubtitle* gxplayer_sub_load(GxPlayer* p, PlayerSubPara* subpara)
{
	int i;
	PISubBlock* subblock = NULL;
	PISubBlock* sb = NULL;
	PISubParam params;
	PlayerProgInfo *pinfo = av_mallocz(sizeof(PlayerProgInfo));

	if (p == NULL || pinfo == NULL|| p->isquiet || subpara == NULL)
		goto errout;

	for (i = 0; i < PLAYER_MAX_SUB_LOAD; i++) {
		if (p->subblock[i].subload == 0) {
			subblock = &p->subblock[i];
			break;
		}
	}

	if (subblock == NULL)
		goto errout;

	subblock->player    = (void*)p;
	params.use_ticktime = 0;

	if (URL_IS_DVB(p->srcurl)) {
		if (subpara->type == PLAYER_SUB_TYPE_ATSC_CC) {
			params.service_id = subpara->service_id;
			subblock->subtype = PLAYER_SUB_TYPE_ATSC_CC;
		} else if (subpara->type == PLAYER_SUB_TYPE_SCTE) {
			params.pid        = subpara->pid;
			subblock->subtype = PLAYER_SUB_TYPE_SCTE;
		} else if (subpara->type == PLAYER_SUB_TYPE_ARIB_CC) {
			params.pid        = subpara->pid;
			subblock->subtype = PLAYER_SUB_TYPE_ARIB_CC;
		} else if (subpara->type == PLAYER_SUB_TYPE_DVB_TTX) {
			params.pid        = subpara->pid;
			subblock->subtype = PLAYER_SUB_TYPE_DVB_TTX;
		} else if (subpara->type == PLAYER_SUB_TYPE_DVB_MAG) {
			params.pid        = subpara->pid;
			subblock->subtype = PLAYER_SUB_TYPE_DVB_MAG;
		} else if (subpara->type == PLAYER_SUB_TYPE_FILE) {
			if (subpara->file_name == NULL)
				goto errout;
			params.url          = subpara->file_name;
			subblock->subtype   = PLAYER_SUB_TYPE_FILE;
			params.use_ticktime = 1;
		} else {
			params.pid        = subpara->pid;
			subblock->subtype = PLAYER_SUB_TYPE_DVB;
		}
	} else {
		gxplayer_get_proginfo(p, p->media_play, pinfo);
		if (subpara->type == PLAYER_SUB_TYPE_FILE) {
			if (subpara->file_name == NULL)
				goto errout;
			params.url     = subpara->file_name;
			subblock->subtype = PLAYER_SUB_TYPE_FILE;
		} else if (subpara->type == PLAYER_SUB_TYPE_ATSC_CC) {
			params.service_id = subpara->service_id;
			subblock->subtype = PLAYER_SUB_TYPE_ATSC_CC;
		} else {
			int index = subpara->pid.pid;
			if (index < 0 || index >= pinfo->sub_num)
				goto errout;
			params.pid        = pinfo->sub[index].pid;
			subblock->subtype = pinfo->sub[index].type;
		}
	}

	if (subblock->subtype != PLAYER_SUB_TYPE_INSIDE) {//解决死机问题,切换到非内置字幕,需要停止内置字幕.
		if (MEDIA_HAS_SUB(p->media_play)) {
			GxSubOut_Control(p->media_play->sout, GX_SO_CTRL_EXE_DESTORY_FUNC, NULL);
		}
	}

	sb = GxPlayer_SubOpen(subblock, &params);
	if (sb) {
		sb->subload = 1;
		if (p->media_play)
			p->media_play->priv = (void*)sb;

		if (subblock->subtype == PLAYER_SUB_TYPE_FILE) {
			p->cur_sub_idx    = pinfo->sub_num;
			GxPlayer_SubGetInfo(subblock, pinfo);
			p->sub_file_count = pinfo->sub_num;
			for (i = 0; i < pinfo->sub_num; i++)
				p->sub_file_track[i] = pinfo->sub[i];
			p->sub_file_url[0] = '\0';
			strncpy(p->sub_file_url, params.url, sizeof(p->sub_file_url));
			p->cur_sub_idx += pinfo->cur_sub_id;
		} else
			p->cur_sub_idx = subpara->pid.pid;

		p->sub_pid = params.pid.pid;
		p->major   = params.pid.major;
		p->minor   = params.pid.minor;
		av_free(pinfo);
		return sb->subout;
	}

errout:
	if (pinfo)
		av_free(pinfo);
	return NULL;
}

status_t gxplayer_sub_unload(GxPlayer* p, PlayerSubtitle* subdata)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL)
		return ret;

	GxPlayer_SubClose(subblock);
	if (p->media_play)
		p->media_play->priv = NULL;

	subblock->subload = 0;
	p->sub_file_count = 0;
	p->sub_pid = -1;
	p->major   = 0;
	p->minor   = 0;
	return GX_PLAYER_OK;
}

status_t gxplayer_sub_hide(GxPlayer* p, PlayerSubtitle* subdata)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL)
		return ret;

	GxPlayer_SubHide(subblock);

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_show(GxPlayer* p, PlayerSubtitle* subdata)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL)
		return ret;

	GxPlayer_SubShow(subblock);

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_get_time(GxPlayer* p, PlayerSubtitle* subdata, uint64_t *curtime, uint64_t *duration)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL)
		return ret;

	GxPlayer_SubGetTime(subblock, curtime, duration);

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_goto_localtime(GxPlayer* p, PlayerSubtitle* subdata, uint64_t timems)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL)
		return ret;

	GxPlayer_SubGotoLocalTime(subblock, timems);

	return GX_PLAYER_OK;
}

status_t gxplayer_sub_resolution(GxPlayer* p, PlayerSubtitle* subdata, VideoOutputMode res)
{
	int ret = GX_PLAYER_ERROR;
	PISubBlock* subblock;

	CHECK_PLAYER(p);

	subblock = _player_find_subblock(p, subdata);

	if (subblock == NULL ||
			res <  GX_VIDEO_OUTPUT_PAL ||
			res >= GX_VIDEO_OUTPUT_MODE_MAX)
		return ret;

	GxPlayer_SubSwitchResolution(subblock,res);

	return GX_PLAYER_OK;
}

status_t gxplayer_get_status(GxPlayer* p, PlayerStatusInfo* info)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(info);

	info->status = p->status;
	info->error  = p->error;
	info->vcodec.state = AVCODEC_STOPPED;
	info->vcodec.err_code = AVCODEC_ERR_NONE;
	info->acodec.state = AVCODEC_STOPPED;
	info->acodec.err_code = AVCODEC_ERR_NONE;
	info->async.sync_flag  = 0;
	info->async.sync_state = AVCODEC_SYNC_FREERUN;
	info->vsync.sync_flag  = 0;
	info->vsync.sync_state = AVCODEC_SYNC_FREERUN;

	switch(p->status)
	{
	case PLAYER_STATUS_PLAY_PAUSE:
	case PLAYER_STATUS_PLAY_RUNNING:
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_SHIFT_SWITCH:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
	case PLAYER_STATUS_PLAY_END:
		{
			int ret = PLAYER_MEDIA_TRYLOCK(p);
			if (ret != GXCORE_SUCCESS){
				return GX_PLAYER_ERROR;
			}
			GxMedia_GetCodecStatus(p->media_play, &info->acodec, &info->vcodec, &info->async, &info->vsync);
			PLAYER_MEDIA_UNLOCK(p);
		}
		break;
	default:
		break;
	}

	p->acodec = info->acodec;
	p->vcodec = info->vcodec;

	return GX_PLAYER_OK;
}

void gxplayer_avprintf(GxPlayer* p)
{
	if (p)
		GxMedia_AVPrintf(p->media_play);
	return;
}

void gxplayer_detect_video_pts_diff(GxPlayer* p)
{
	if (p)
		GxMedia_DetectVideoPtsDiff(p->media_play);
	return;
}

status_t gxplayer_get_time(GxPlayer* p, uint64_t* current, uint64_t* totle, uint64_t* seek_min)
{
	CHECK_PLAYER(p);

	if (MEDIA_IS_DVB(p->media_play) && !p->media_record)
		return GX_PLAYER_ERROR;

	if (totle) {
		switch(p->status) {
		case PLAYER_STATUS_SHIFT_START:
		case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
		case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		case PLAYER_STATUS_SHIFT_SWITCH:
		case PLAYER_STATUS_RECORD_PAUSE:
		case PLAYER_STATUS_RECORD_RUNNING:
		case PLAYER_STATUS_RECORD_FULL:
		case PLAYER_STATUS_RECORD_END:
		case PLAYER_STATUS_SHIFT_PAUSE:
		case PLAYER_STATUS_SHIFT_RUNNING:
			PLAYER_MEDIA_LOCK(p);
			*totle = _player_get_record_druation(p);
			PLAYER_MEDIA_UNLOCK(p);
			break;
		case PLAYER_STATUS_PLAY_START:
			*totle = 0 ;
			break;
		case PLAYER_STATUS_PLAY_PAUSE:
		case PLAYER_STATUS_PLAY_RUNNING:
			PLAYER_MEDIA_LOCK(p);
			*totle = GxMedia_GetDuration(p->media_play);
			PLAYER_MEDIA_UNLOCK(p);
			break;
		case PLAYER_STATUS_PLAY_END:
			*totle = p->play_end_time;
			break;
		default:
			*totle = 0;
			return GX_PLAYER_ERROR;
		}
		//gxlogf(": [Totle]:=%lld\n", *totle);
	}

	if (seek_min) {
		switch(p->status) {
		case PLAYER_STATUS_SHIFT_START:
		case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
		case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
		case PLAYER_STATUS_SHIFT_SWITCH:
		case PLAYER_STATUS_RECORD_PAUSE:
		case PLAYER_STATUS_RECORD_RUNNING:
		case PLAYER_STATUS_RECORD_FULL:
		case PLAYER_STATUS_RECORD_END:
		case PLAYER_STATUS_SHIFT_PAUSE:
		case PLAYER_STATUS_SHIFT_RUNNING:
			PLAYER_MEDIA_LOCK(p);
			*seek_min = _player_get_record_seekmin(p);
			PLAYER_MEDIA_UNLOCK(p);
			break;
		case PLAYER_STATUS_PLAY_START:
		case PLAYER_STATUS_PLAY_PAUSE:
		case PLAYER_STATUS_PLAY_RUNNING:
		case PLAYER_STATUS_PLAY_END:
			PLAYER_MEDIA_LOCK(p);
			*seek_min = GxMedia_GetSeekMinTime(p->media_play);
			PLAYER_MEDIA_UNLOCK(p);
			break;
		default:
			*seek_min = 0;
			break;
		}
	}

	if (current) {
		PlayerStatusInfo info;
		if (gxplayer_get_status(p, &info) != GXCORE_SUCCESS) {
			return GX_PLAYER_ERROR;
		}

		//if get time error in speed, consider it
		if ((info.acodec.state == AVCODEC_ERROR ||
					(p->speed == 1000 && info.acodec.state == AVCODEC_STOPPED)) &&
				(info.vcodec.state == AVCODEC_ERROR ||
					(p->speed == 1000 && info.vcodec.state == AVCODEC_STOPPED)) &&
				p->status != PLAYER_STATUS_SHIFT_START &&
				p->status != PLAYER_STATUS_RECORD_RUNNING &&
				p->status != PLAYER_STATUS_SHIFT_HOLD_RUNNING) {
			*current = 0;
			return GX_PLAYER_ERROR;
		}
		else {
			switch(p->status)
			{
			case PLAYER_STATUS_SHIFT_START:
				*current = p->start_timems;
				break;
			case PLAYER_STATUS_RECORD_RUNNING:
				*current = 0;
				break;
			case PLAYER_STATUS_PLAY_END:
				*current = p->play_end_time;
				break;
			case PLAYER_STATUS_STOPPED:
			case PLAYER_STATUS_PLAY_START:
				*current = 0;
				break;
			case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
			case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
			case PLAYER_STATUS_SHIFT_SWITCH:
				PLAYER_MEDIA_LOCK(p);
				*current = _player_get_record_druation(p);
				PLAYER_MEDIA_UNLOCK(p);
				break;
			default:
				PLAYER_MEDIA_LOCK(p);
				if (GX_PLAYER_OK != GxMedia_GetCurrentTime(p->media_play, current)) {
					/*
					 * when speed != 1000 and player status is pause
					 * and the video module does't play any frame yet
					 * we use the before-time in this condition
					 */
					if ((p->speed != 1000 && p->status == PLAYER_STATUS_PLAY_PAUSE) ||
							p->status == PLAYER_STATUS_SHIFT_PAUSE) {
						*current = p->cur_timems;
					} else {
						PLAYER_MEDIA_UNLOCK(p);
						return GX_PLAYER_ERROR;
					}
				}
				PLAYER_MEDIA_UNLOCK(p);
				break;
			}

			if (*current == (uint64_t)-1){
				return GX_PLAYER_ERROR;
			}

			if (current && totle && (*totle > 0) && (*current > *totle))
				*current = *totle;

			if (current && seek_min && (*seek_min >= 0) && (*current < *seek_min))
				*current = *seek_min;

			p->cur_timems = *current;
		}
		//gxlogf(": [c]:=%lld \n", *current);
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_get_duration(GxPlayer* p, uint64_t* duration)
{
	CHECK_PLAYER(p);

	if (duration == NULL)
		return GX_PLAYER_ERROR;
	if (MEDIA_IS_DVB(p->media_play) && !p->media_record)
		return GX_PLAYER_ERROR;

	switch(p->status) {
	case PLAYER_STATUS_SHIFT_START:
	case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
	case PLAYER_STATUS_SHIFT_SWITCH:
	case PLAYER_STATUS_RECORD_PAUSE:
	case PLAYER_STATUS_RECORD_RUNNING:
	case PLAYER_STATUS_RECORD_FULL:
	case PLAYER_STATUS_RECORD_END:
	case PLAYER_STATUS_SHIFT_PAUSE:
	case PLAYER_STATUS_SHIFT_RUNNING:
		PLAYER_MEDIA_LOCK(p);
		*duration = _player_get_record_druation(p);
		PLAYER_MEDIA_UNLOCK(p);
		break;
	case PLAYER_STATUS_PLAY_START:
		*duration = 0 ;
		break;
	case PLAYER_STATUS_PLAY_PAUSE:
	case PLAYER_STATUS_PLAY_RUNNING:
		PLAYER_MEDIA_LOCK(p);
		*duration = GxMedia_GetDuration(p->media_play);
		PLAYER_MEDIA_UNLOCK(p);
		break;
	case PLAYER_STATUS_PLAY_END:
		*duration = p->play_end_time;
		break;
	default:
		*duration = 0;
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_get_percent(GxPlayer* p, uint8_t* current)
{
	uint64_t totle=0;
	CHECK_PLAYER(p);
	if (current)
	{
		PlayerStatusInfo info;
		if (gxplayer_get_status(p, &info) != GXCORE_SUCCESS)
			return GX_PLAYER_ERROR;

		if (info.acodec.state == AVCODEC_STOPPED &&
				info.vcodec.state == AVCODEC_STOPPED &&
				p->status != PLAYER_STATUS_SHIFT_START &&
				p->status != PLAYER_STATUS_SHIFT_HOLD_RUNNING)
			*current = 0;
		else
		{
			PLAYER_MEDIA_LOCK(p);
			switch(p->status)
			{
			case PLAYER_STATUS_SHIFT_START:
				totle = GxMedia_GetDuration(p->media_play);
				if (totle>0){
					*current = p->start_timems/totle *100;
				}
				break;
			case PLAYER_STATUS_PLAY_END:
				*current = 100;
				break;
			case PLAYER_STATUS_STOPPED:
			case PLAYER_STATUS_PLAY_START:
				*current = 0;
				break;
			case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
			case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
			case PLAYER_STATUS_SHIFT_SWITCH:
				totle = GxMedia_GetDuration(p->media_play);
				if (totle>0) {
					uint64_t current_time;
					if (GX_PLAYER_OK == GxMedia_GetCurrentTime(p->media_play, &current_time)) {
						*current = current_time / totle * 100;
					} else {
						PLAYER_MEDIA_UNLOCK(p);
						return GX_PLAYER_ERROR;
					}
				}
				break;
			default:
				*current = GxMedia_GetCurrentPercent(p->media_play);
				break;
			}
			PLAYER_MEDIA_UNLOCK(p);
		}
		//gxlogf(": [c]:=%lld \n", *current);
	}

	return GX_PLAYER_OK;
}
status_t gxplayer_get_datalen(GxPlayer* p, uint32_t* adataLen, uint32_t* vdataLen)
{
	uint32_t aLen = 0, vLen = 0;

	CHECK_PLAYER(p);

	switch(p->status)
	{
	case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
	case PLAYER_STATUS_SHIFT_RUNNING:
	case PLAYER_STATUS_PLAY_RUNNING:
		PLAYER_MEDIA_LOCK(p);
		GxMedia_GetDataLength(p->media_play, &aLen, &vLen);
		PLAYER_MEDIA_UNLOCK(p);
		break;
	default:
		break;
	}

	if (adataLen) *adataLen = aLen;
	if (vdataLen) *vdataLen = vLen;

	return GX_PLAYER_OK;
}

status_t gxplayer_get_info(GxPlayer* p, PlayerProgInfo* info)
{
	int ret = GX_PLAYER_ERROR;;
	CHECK_PLAYER(p);
	CHECK_PLAYER(info);
	CHECK_PLAYER_INFO_READY(p);

	PLAYER_MEDIA_LOCK(p);

	if (p->media_record) {
		info->rec_filesize = _player_get_record_filesize(p);
		info->rec_duration = _player_get_record_druation(p);
		ret = GX_PLAYER_OK;
	}
	if (p->media_play) {
		ret = gxplayer_get_proginfo(p, p->media_play, info);
		info->switch_cost_timems = p->switch_cost_timems;
	}

	PLAYER_MEDIA_UNLOCK(p);
	return ret;
}

status_t gxplayer_get_syncinfo(GxPlayer* p, PlayerSyncInfo* syncinfo)
{
	int ret = GX_PLAYER_ERROR;
	CHECK_PLAYER(p);
	CHECK_PLAYER(syncinfo);
	PLAYER_MEDIA_LOCK(p);

	if (p->media_play) {
		GxMedia_GetSyncInfo(p->media_play, syncinfo);
		ret = GX_PLAYER_OK;
	}

	PLAYER_MEDIA_UNLOCK(p);
	return ret;
}

status_t gxplayer_get_streaminfo(GxPlayer* p, PlayerProgStreamInfo* info)
{
	int ret = GX_PLAYER_ERROR;;
	CHECK_PLAYER(p);
	CHECK_PLAYER(info);
	CHECK_PLAYER_INFO_READY(p);
	PLAYER_MEDIA_LOCK(p);

	if (p->media_play)
		ret = gxplayer_get_prog_streaminfo(p, p->media_play, info);

	PLAYER_MEDIA_UNLOCK(p);
	return ret;
}

status_t gxplayer_get_netinfo(GxPlayer* p, PlayerNetInfo* info)
{
	GxMedia* media = NULL;

	CHECK_PLAYER(p);
	CHECK_PLAYER(info);

	if(PLAYER_MEDIA_TRYLOCK(p)!=GXCORE_SUCCESS) {
		return GX_PLAYER_ERROR;
	}

	media = p->media_play;
	if (media && media->stream) {
		int i;

		GxMedia_GetCacheInfo(media, info);
		info->cache_buf_time   = GxMedia_GetCacheTime(media);
		info->seek_flag        = p->seekable;
		info->buffering        = media->buffering;
		info->eof              = media->stream->eof;
		info->restart_time     = media->stream->info.restart_time;
		info->bandwidth.cur_id = media->stream->prog_now;
		info->bandwidth.num    = media->stream->prog_max;
		for (i = 0; i < GXPLAYER_MAX_BAND_WIDTH; i++) {
			if (i >= info->bandwidth.num)
				break;
			info->bandwidth.value[i] = media->stream->prog[i].bandwidth;
		}
		PLAYER_MEDIA_UNLOCK(p);
		return  GX_PLAYER_OK;
	}

	PLAYER_MEDIA_UNLOCK(p);
	return GX_PLAYER_ERROR;
}

unsigned int gxplayer_check_record_overflow(GxPlayer* p)
{
	return GxMedia_RecordCheckOverflow(p->media_record);
}

unsigned int gxplayer_get_video_display_frame_cnt(GxPlayer* p)
{
	if (MEDIA_HAS_VIDEO(p->media_play)) {
		GxStreamVideoHeader* sh = p->media_play->sh_video;
		GxMedia_FlushFrameInfo(p->media_play);
		return sh->stat.play_frame_cnt;
	}
	return 0;
}

unsigned int gxplayer_start(GxPlayer* p)
{
	int ret = 0;

	if (p->media_play && (
			p->status == PLAYER_STATUS_PLAY_RUNNING       ||
			p->status == PLAYER_STATUS_SHIFT_RUNNING      ||
			p->status == PLAYER_STATUS_SHIFT_HOLD_RUNNING ||
			p->status == PLAYER_STATUS_START_FILL_BUF     ||
			p->status == PLAYER_STATUS_PLAY_END           ||
			p->status == PLAYER_STATUS_END_FILL_BUF )) {
		ret = GxMedia_VideoStartDecord(p->media_play);
		GxMedia_VideoStartDisplay(p->media_play);
		if (p->error != p->media_play->errInfo) {
			GxPlayer_StatusReport(p, p->status, p->media_play->errInfo, p->srcurl);
			if (p->media_play->errInfo == PLAYER_ERROR_VIDEO_DECODER_ERROR) {
				p->freeze_end_cbk(p, 0);
			}
		}
	}

	return ret;
}

status_t gxplayer_resolution_reconfig(GxPlayer* p)
{
	int ret = 0;

	if (MEDIA_HAS_VIDEO(p->media_play)) {
		GxStreamVideoHeader* sh = p->media_play->sh_video;
		GxMedia_FlushFrameInfo(p->media_play);
		if (sh->stat.play_frame_cnt > 1) {
			ret = GxPlayer_SystemSet(PSYS_VOUT_AUTOCONFIG, &sh->fps);
		}
		return ret;
	}
	return GX_PLAYER_ERROR;
}

status_t gxplayer_video_dynamic_range_monitor(GxPlayer* p)
{
	int ret = GX_PLAYER_ERROR;

	if (MEDIA_HAS_VIDEO(p->media_play)) {
		ret = GxMedia_VideoCheckDynamicRangeChange(p->media_play);
		if (ret == 0) {
			PlayerEventReport EventReport;
			PlayerEventVideoDynamicRangeChanged EventDynamicRangeChanged;

			GxMedia_FlushFrameInfo(p->media_play);
			GxStreamVideoHeader* sh = p->media_play->sh_video;
			EventDynamicRangeChanged.dr.type       = sh->drt;
			EventDynamicRangeChanged.dr.dolby_flag = sh->dolby_flag;

			EventReport.args = &EventDynamicRangeChanged;
			EventReport.event = PLAYER_EVENT_DYNAMIC_RANGE_CHANGED;
			GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
		}
	}

	return ret;
}

status_t gxplayer_get_recconfig(GxPlayer* player, GxMedia* media, PlayerRecordConfig* config)
{
	if (player && player->media_play)
		GxMedia_GetTSRecordInfo(player->media_play, config);
	else if (player && player->media_record)
		GxMedia_GetTSRecordInfo(player->media_record, config);
	else if (media)
		GxMedia_GetTSRecordInfo(media, config);
	else
		return GX_PLAYER_ERROR;

	return GX_PLAYER_OK;
}

status_t gxplayer_get_dual_mono(GxPlayer* p, int *dual_mono)
{
	CHECK_PLAYER(p);
	CHECK_NULL(p->media_play);

	if (p->media_play->demuxer) {
		GxMedia *media = p->media_play;
		int i = media->demuxer->audio->id;
		GxStreamAudioHeader* sh = media->demuxer->a_streams[i];
		if (sh && (sh->dual_mono != -1)) {
			*dual_mono = sh->dual_mono;
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

status_t gxplayer_get_proginfo(GxPlayer* player, GxMedia* media, PlayerProgInfo* info)
{
	int i;
	PlayerNetInfo net_info;

	if (!info || !media || !media->stream || !media->demuxer)
		return GX_PLAYER_ERROR;

	GxMedia_FlushFrameInfo(media);

	memset(info, 0, sizeof(PlayerProgInfo));
	GxMedia_GetCacheInfo(media, &net_info);
	info->file_size = media->stream->end_pos;
	info->duration  = media->demuxer->duration;
	snprintf (info->url, sizeof(info->url) - 1, "%s", media->stream->original_url);
	snprintf (info->mime_type, sizeof(info->mime_type) - 1, "%s", media->stream->mime_type);
	info->eof                = media->stream->eof;
	info->net_speed          = net_info.net_speed;
	info->buf_percent        = net_info.buf_percent;
	info->cache_buf_size     = net_info.cache_buf_size;
	info->cache_buf_percent  = net_info.cache_buf_percent;
	info->cache_back_size    = net_info.cache_back_size;
	info->cache_back_percent = net_info.cache_back_percent;
	info->cache_buf_time     = GxMedia_GetCacheTime(media);
	GxDemuxer_GetEsBufPercent(media->demuxer, &(info->esa_percent), &(info->esv_percent));
	info->buffering = media->buffering;
	info->seekable  = media->demuxer->seekable;
	if (info->duration >= 1000)
		info->bitrate = info->file_size*8/(info->duration/1000);

	if (MEDIA_HAS_AUDIO(media) && (!MEDIA_HAS_VIDEO(media)))
		info->is_radio = 1;
	else
		info->is_radio = 0;

	info->cur_bandwidth_id = media->stream->prog_now;
	info->bandwidth_num    = FFMIN(GXPLAYER_MAX_BAND_WIDTH, media->stream->prog_max);
	if (info->bandwidth_num > 0) {
		for (i = 0; i < info->bandwidth_num; i++) {
			info->bandwidth[i].bandwidth = media->stream->prog[i].bandwidth;
			info->bandwidth[i].acodec = media->stream->prog[i].acodec;
			info->bandwidth[i].vcodec = media->stream->prog[i].vcodec;
			info->bandwidth[i].width = media->stream->prog[i].width;
			info->bandwidth[i].height = media->stream->prog[i].height;
			info->bandwidth[i].frameRate = media->stream->prog[i].frameRate;
		}
	}

	if (media->sh_video) {
		GxStreamVideoHeader* sh = media->sh_video;

		info->video.width    = sh->disp_w;
		info->video.height   = sh->disp_h;
		info->video.bitrate  = sh->i_bps*8;
		info->video.fps      = sh->fps;
		info->video.dar.num  = sh->dar.num;
		info->video.dar.den  = sh->dar.den;
		info->video.sar.num  = sh->sar.num;
		info->video.sar.den  = sh->sar.den;
		info->video.bpp      = sh->bpp;
		info->video.interlace= sh->interlace;
		info->video_stat     = sh->stat;
		info->video.ratio    = (PlayerVideoAspectRatio )sh->ratio;
		info->video.bitrate  = sh->bitrate;

		snprintf (info->video.codec_id, sizeof(info->video.codec_id) - 1, "%s", sh->priv.codec);
		snprintf (info->video.track_name, sizeof(info->video.track_name) - 1, "%s", sh->priv.name);

		info->video.codec_type = sh->format;

		info->video.dr.type       = sh->drt;
		info->video.dr.dolby_flag = sh->dolby_flag;
	}

	if (1) {
		for (i=0;i<MAX_A_STREAMS;i++) {
			GxStreamAudioHeader* sh = media->demuxer->a_streams[i];
			if (sh == NULL)
				break;
			info->audio[info->audio_num].id         = sh->priv.id;
			info->audio[info->audio_num].bitrate    = (sh->wf!=NULL)?sh->wf->nAvgBytesPerSec:sh->i_bps;
			info->audio[info->audio_num].samplerate = (sh->wf!=NULL)?sh->wf->nSamplesPerSec:sh->samplerate;
			info->audio[info->audio_num].channels   = (sh->wf!=NULL)?sh->wf->nChannels:sh->channels;
			info->audio[info->audio_num].track_type = (sh->audio_id==1)?AUDIO_DESCRIPTOR_TRACK:AUDIO_PRIMARY_TRACK;
			info->audio_stat                        = sh->stat;

			snprintf (info->audio[info->audio_num].codec_id, sizeof(info->audio[info->audio_num].codec_id) - 1, "%s", sh->priv.codec);
			snprintf (info->audio[info->audio_num].track_name, sizeof(info->audio[info->audio_num].track_name) - 1, "%s", sh->priv.name);
			snprintf (info->audio[info->audio_num].lang, sizeof(info->audio[info->audio_num].lang) - 1, "%s", sh->priv.lang);

			info->audio[info->audio_num].codec_type = sh->format;
			info->audio_num++;
			if (info->audio_num >= PLAYER_MAX_TRACK_AUDIO)
				break;
		}
		info->cur_audio_id = media->demuxer->audio->id;
	}

	if (1) {
		for (i = 0; i < MAX_S_STREAMS; i++) {
			GxStreamSubHeader *sh = media->demuxer->s_streams[i];
			PlayerSubTrack    *sub = NULL;
			if (sh == NULL)
				break;
			if (sh->priv.sub_num > 0 && sh->priv.sub_num < MAX_SUB_STREAM_NUM) {
				int j = 0;
				for (j = 0; j < sh->priv.sub_num; j++) {
					sub = &info->sub[info->sub_num];
					sub->id        = sh->priv.id;
					sub->pid.pid   = sh->priv.id;
					sub->pid.major = sh->priv.sub_stream[j].major;
					sub->pid.minor = sh->priv.sub_stream[j].minor;
					if (sh->type == SUB_CODEC_DVB_DESCRIPTOR) {
						sub->type   = PLAYER_SUB_TYPE_DVB;
						sub->encode = PLAYER_SUB_ENC_SPU;
					} else if (sh->type == SUB_CODEC_TXT_DESCRIPTOR) {
						if (sh->priv.sub_stream[j].type == 0x01)
							sub->type = PLAYER_SUB_TYPE_DVB_MAG;
						else
							sub->type = PLAYER_SUB_TYPE_DVB_TTX;
						sub->encode = PLAYER_SUB_ENC_SPU;
					} else if (sh->type == SUB_CODEC_SCTE) {
						sub->type   = PLAYER_SUB_TYPE_SCTE;
						sub->encode = PLAYER_SUB_ENC_SPU;
					} else if (sh->type == SUB_CODEC_ARIB) {
						sub->type   = PLAYER_SUB_TYPE_ARIB_CC;
						sub->encode = PLAYER_SUB_ENC_SPU;
					} else {
						sub->type = PLAYER_SUB_TYPE_INSIDE;
						switch (sh->type) {
						case SUB_CODEC_SRT:
						case SUB_CODEC_MOV_TEXT:
						case SUB_CODEC_SSA:
							sub->encode = PLAYER_SUB_ENC_UTF8;
							break;
						case SUB_CODEC_DVD:
						case SUB_CODEC_VOB:
						case SUB_CODEC_PGS:
							sub->encode = PLAYER_SUB_ENC_SPU;
							break;
						default:
							sub->encode = PLAYER_SUB_ENC_UTF8;
							break;
						}
					}
					snprintf (sub->codec_id, sizeof(sub->codec_id) - 1, "%s", sh->priv.codec);
					snprintf (sub->track_name, sizeof(sub->track_name) - 1, "%s", sh->priv.name);
					if ((sub->type == PLAYER_SUB_TYPE_DVB || sub->type == PLAYER_SUB_TYPE_DVB_TTX || sub->type == PLAYER_SUB_TYPE_DVB_MAG)
						&& (GX_DEMUXER_TYPE_LAVF == media->demuxer->type)) {
						snprintf (sub->lang, sizeof(sub->lang) - 1, "%s", sh->priv.lang);
					} else {
						snprintf (sub->lang, sizeof(sub->lang) - 1, "%s", sh->priv.sub_stream[j].lang);
					}
					info->sub_num++;
					if (info->sub_num >= PLAYER_MAX_TRACK_SUB)
						break;
				}
			} else {
				sub = &info->sub[info->sub_num];
				sub->id        = sh->priv.id;
				sub->pid.pid   = sh->priv.id;
				sub->type   = PLAYER_SUB_TYPE_INSIDE;
				switch (sh->type) {
				case SUB_CODEC_SRT:
				case SUB_CODEC_MOV_TEXT:
				case SUB_CODEC_SSA:
					sub->encode = PLAYER_SUB_ENC_UTF8;
					break;
				case SUB_CODEC_DVD:
				case SUB_CODEC_VOB:
				case SUB_CODEC_PGS:
					sub->encode = PLAYER_SUB_ENC_SPU;
					break;
				default:
					sub->encode = PLAYER_SUB_ENC_UTF8;
					break;
				}
				snprintf (sub->codec_id, sizeof(sub->codec_id) - 1, "%s", sh->priv.codec);
				snprintf (sub->track_name, sizeof(sub->track_name) - 1, "%s", sh->priv.name);
				snprintf (sub->lang, sizeof(sub->lang) - 1, "%s", sh->priv.lang);
				info->sub_num++;
				if (info->sub_num >= PLAYER_MAX_TRACK_SUB)
					break;
			}
		}

		if (player) {
			for (i = 0; i < player->sub_file_count; i++) {
				if (info->sub_num < PLAYER_MAX_TRACK_SUB) {
					info->sub[info->sub_num] = player->sub_file_track[i];
					info->sub_num++;
				}
			}

			info->cur_sub_id = player->cur_sub_idx;
		} else
			info->cur_sub_id = -1;
	}
	gxlogf("\n[Player]  ------ [URL]:%s ---------\n",info->url);
	gxlogf("[Player]  ------ [DUR]:%lld Ms---------\n",info->duration);
	gxlogf("[Player]  ------ [SIZ]:%lld Byte ---------\n",info->file_size);
	gxlogf("[Player]  ------ [IS_RADIO]:%d ---------\n",info->is_radio);
	if (info->duration >= 1000)
		gxlogf("[Player]  ------ [BPS]:%lld bps ---------\n",info->file_size*8/(info->duration/1000));
	if (media->sh_video){
		gxlogf("[Player] \t| + [VIDEO...]\n");
		gxlogf("[Player] \t| \t+ codec   =  %s\n",info->video.codec_id);
		gxlogf("[Player] \t| \t+ width   =  %d\n",info->video.width);
		gxlogf("[Player] \t| \t+ height  =  %d\n",info->video.height);
		gxlogf("[Player] \t| \t+ bitrate =  %d\n",info->video.bitrate);
		gxlogf("[Player] \t| \t+ fps     =  %.6f\n",info->video.fps);
		gxlogf("[Player] \t| \t+ sar     =  [%d:%d]\n",info->video.sar.num, info->video.sar.den);
		gxlogf("[Player] \t| \t+ dar     =  [%d:%d]\n",info->video.dar.num, info->video.dar.den);
	}

	if (info->audio_num){
		for (i=0;i<info->audio_num;i++){
			gxlogf("[Player] \t| + [AUDIO...%d]\n",i);
			gxlogf("[Player] \t| \t+ id         =  %d\n",info->audio[i].id);
			gxlogf("[Player] \t| \t+ lang       =  %s\n",info->audio[i].lang);
			gxlogf("[Player] \t| \t+ codec      =  %s\n",info->audio[i].codec_id);
			gxlogf("[Player] \t| \t+ name       =  %s\n",info->audio[i].track_name);
			gxlogf("[Player] \t| \t+ bitreate   =  %d\n",info->audio[i].bitrate);
			gxlogf("[Player] \t| \t+ samplerate =  %d HZ\n",info->audio[i].samplerate);
			gxlogf("[Player] \t| \t+ channels   =  %d\n",info->audio[i].channels);
		}
		gxlogf("[Player] \t| + [Playing Track ID...%d]\n", info->cur_audio_id);
	}

	if (media->sh_sub && info->sub_num){
		for (i=0;i<info->sub_num;i++){
			gxlogf("[Player] \t| + [SUB...%d]\n",i);
			gxlogf("[Player] \t| \t+ id         =  %d\n",info->sub[i].id);
			gxlogf("[Player] \t| \t+ lang       =  %s\n",info->sub[i].lang);
			gxlogf("[Player] \t| \t+ codec      =  %s\n",info->sub[i].codec_id);
			gxlogf("[Player] \t| \t+ name       =  %s\n",info->sub[i].track_name);
			gxlogf("[Player] \t| \t+ encoder    =  %d\n",info->sub[i].encode);
		}
		gxlogf("[Player] \t| + [Playing Track ID...%d]\n", info->cur_sub_id);
	}

	if (info->bandwidth_num > 0)
	{
		for (i=0; i<info->bandwidth_num; i++){
			gxlogf("[Player] \t| + [band width...]\n");
			gxlogf("[Player] \t| \t+ [%-2d]     =  %-8d\n", i, info->bandwidth[i]);
		}
		gxlogf("[Player] \t| + [Playing Track ID...%d]\n", info->cur_bandwidth_id);
	}

	gxlogf("[Player]  ----------------------------------------------\n\n");
	gxlogf("[Player]  Video: play_frame     : %lld\n", info->video_stat.play_frame_cnt);
	gxlogf("[Player]  Video: error_frame    : %lld\n", info->video_stat.error_frame_cnt);
	gxlogf("[Player]  Video: filter_frame   : %lld\n", info->video_stat.filter_frame_cnt);
	gxlogf("[Player]  Video: lose_sync_cnt  : %lld\n", info->video_stat.lose_sync_cnt);
	gxlogf("[Player]  ----------------------------------------------\n\n");
	gxlogf("[Player]  Audio: play_frame     : %lld\n", info->audio_stat.play_frame_cnt);
	gxlogf("[Player]  Audio: error_frame    : %lld\n", info->audio_stat.error_frame_cnt);
	gxlogf("[Player]  Audio: filter_frame   : %lld\n", info->audio_stat.filter_frame_cnt);
	gxlogf("[Player]  Audio: lose_sync_cnt  : %lld\n", info->audio_stat.lose_sync_cnt);
	gxlogf("[Player]  ----------------------------------------------\n\n");
	return GX_PLAYER_OK;
}

status_t gxplayer_get_prog_streaminfo(GxPlayer* player, GxMedia* media, PlayerProgStreamInfo* info)
{
	if (!media || !info)
		return GX_PLAYER_ERROR;

	memset(info, 0, sizeof(PlayerProgStreamInfo));
	if (media->demuxer->type == GX_DEMUXER_TYPE_MPEG_TS)
		GxMedia_GetTSProgStreamInfo(media, info);

	if (info->prog_count == 0) {
		unsigned int has_prog = 0, i = 0, j = 0, sc = 0;

		if (media->sh_video) {
			GxStreamVideoHeader* sh = media->sh_video;

			info->prog[0].video_stream_count      = 1;
			info->prog[0].video_stream.pid        = sh->priv.id;
			info->prog[0].video_stream.codec_type = sh->format;
			snprintf (info->prog[0].video_stream.lang, PLAYER_TARCK_LANG_LONG - 1, "%s", sh->priv.lang);
			has_prog = 1;
		}

		for (i = 0; i < MAX_A_STREAMS; i++) {
			GxStreamAudioHeader* sh = media->demuxer->a_streams[i];

			if (sh == NULL)
				break;

			if (i >= PLAYER_MAX_PROG_STREAM_COUNT)
				break;

			info->prog[0].audio_stream[i].pid        = sh->priv.id;
			info->prog[0].audio_stream[i].codec_type = sh->format;
			snprintf (info->prog[0].audio_stream[i].lang, PLAYER_TARCK_LANG_LONG - 1, "%s", sh->priv.lang);
			has_prog = 1;
		}
		info->prog[0].audio_stream_count = i;

		for (i = 0; i < MAX_S_STREAMS; i++) {
			GxStreamSubHeader *sh = media->demuxer->s_streams[i];

			if (sh == NULL)
				break;

			if ((sh->priv.sub_num > 0) && (sh->priv.sub_num < MAX_SUB_STREAM_NUM)) {
				int j = 0;

				for (j = 0; j < sh->priv.sub_num; j++) {
					info->prog[0].subtitle_stream[sc].pid   = sh->priv.id;
					info->prog[0].subtitle_stream[sc].major = sh->priv.sub_stream[j].major;
					info->prog[0].subtitle_stream[sc].minor = sh->priv.sub_stream[j].minor;
					switch (sh->type) {
					case SUB_CODEC_DVB_DESCRIPTOR:
						info->prog[0].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_DVB;
						info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_DVB;
						break;
					case SUB_CODEC_TXT_DESCRIPTOR:
						if (sh->priv.sub_stream[j].type == 0x01)
							info->prog[0].subtitle_stream[sc].type   = PLAYER_SUB_TYPE_DVB_MAG;
						else
							info->prog[0].subtitle_stream[sc].type   = PLAYER_SUB_TYPE_DVB_TTX;
						info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_TELETEXT;
						break;
					case SUB_CODEC_SCTE:
						info->prog[0].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_SCTE;
						info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SCTE_CC;
						break;
					case SUB_CODEC_ARIB:
						info->prog[0].subtitle_stream[sc].type       = PLAYER_SUB_TYPE_ARIB_CC;
						info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_ARIB_CC;
						break;
					default:
						info->prog[0].subtitle_stream[sc].type = PLAYER_SUB_TYPE_INSIDE;
						if (sh->type == SUB_CODEC_SRT)
							info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SRT;
						else if (sh->type == SUB_CODEC_MOV_TEXT)
							info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_TEXT;
						else if (sh->type == SUB_CODEC_SSA)
							info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SSA;
						else if (sh->type == SUB_CODEC_DVD)
							info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_VOB;
						else if (sh->type == SUB_CODEC_VOB) //内部类型定义错误，因此以下赋值纠正.
							info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_XSUB;
						break;
					}
					snprintf (info->prog[0].subtitle_stream[sc].lang, PLAYER_TARCK_LANG_LONG - 1, "%s", sh->priv.sub_stream[j].lang);
					sc++;
					if (sc >= PLAYER_MAX_PROG_STREAM_COUNT)
						break;
				}
			} else {
				info->prog[0].subtitle_stream[sc].pid    = sh->priv.id;
				info->prog[0].subtitle_stream[sc].type   = PLAYER_SUB_TYPE_INSIDE;
				if (sh->type == SUB_CODEC_SRT)
					info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SRT;
				else if (sh->type == SUB_CODEC_MOV_TEXT)
					info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_TEXT;
				else if (sh->type == SUB_CODEC_SSA)
					info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_SSA;
				else if (sh->type == SUB_CODEC_DVD)
					info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_VOB;
				else if (sh->type == SUB_CODEC_VOB) //内部类型定义错误，因此以下赋值纠正.
					info->prog[0].subtitle_stream[sc].codec_type = SUBTITLE_CODEC_XSUB;
				snprintf (info->prog[0].subtitle_stream[sc].lang, PLAYER_TARCK_LANG_LONG - 1, "%s", sh->priv.lang);
				sc++;
			}
			if (sc >= PLAYER_MAX_PROG_STREAM_COUNT)
				break;
		}
		info->prog[0].subtitle_stream_count = sc;
		info->prog_count = ((has_prog > 0) ? 1 : 0);
	}

	return GX_PLAYER_OK;
}

status_t gxplayer_audio_delay(GxPlayer* p, int delayms)
{
	CHECK_PLAYER(p);

	p->audio_delayms = delayms;
	if (p->media_play)
		GxMedia_AudioDelay(p->media_play, delayms);

	return GX_PLAYER_OK;
}

status_t gxplayer_video_delay(GxPlayer* p, int delayms)
{
	CHECK_PLAYER(p);

	p->video_delayms = delayms;
	if (p->media_play)
		GxMedia_VideoDelay(p->media_play, delayms);

	return GX_PLAYER_OK;
}

status_t gxplayer_play_config(GxPlayer* p, PlayerPlayConfig* config)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(config);

	memcpy(&p->playconfig, config, sizeof(PlayerPlayConfig));
	if (p->playconfig.tsfilter.vpid <= 0)
		p->playconfig.tsfilter.vpid = -1;
	if (p->playconfig.tsfilter.apid <= 0)
		p->playconfig.tsfilter.apid = -1;
	if (p->playconfig.tsfilter.apid1 <= 0)
		p->playconfig.tsfilter.apid1 = -1;
	return GX_PLAYER_OK;
}

status_t gxplayer_delay_config(GxPlayer* p, PlayerDelayConfig* config)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(config);

	GxPlayer_SystemSet(PSYS_DELAY_CACHE_DIR, &config->cachedir);
	GxPlayer_SystemSet(PSYS_DELAY_CACHE_TO_MEM, &config->cache2mem);
	GxPlayer_SystemSet(PSYS_DELAY_CACHE_SIZE, &config->cachesize);

	return GX_PLAYER_OK;
}

status_t gxplayer_record_config(GxPlayer* p, PlayerRecordConfig* config)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(config);

	if (p->recorder == NULL) {
		p->recorder = av_mallocz(sizeof(PlayerRecorder));
		if (p->recorder == NULL) {
			gxloge("%s %d: malloc failed\n", __func__, __LINE__);
			return GX_PLAYER_ERROR;
		}
	}
	memcpy(&p->recorder->recconfig, config, sizeof(PlayerRecordConfig));

	return GX_PLAYER_OK;
}

status_t gxplayer_record_encrypt(GxPlayer* p, PlayerRecordEncrypt* encrpyt)
{
	CHECK_PLAYER(p);
	CHECK_PLAYER(encrpyt);

	if (p->recorder == NULL) {
		p->recorder = av_mallocz(sizeof(PlayerRecorder));
		if (p->recorder == NULL) {
			gxloge("%s %d: malloc failed\n", __func__, __LINE__);
			return GX_PLAYER_ERROR;
		}
	}
	if (encrpyt->user_encrypt_enable == 0)
		encrpyt->user_encrypt_blocksize = 0;
	memcpy(&p->recorder->recencrypt, encrpyt, sizeof(PlayerRecordEncrypt));

	return GX_PLAYER_OK;
}

status_t gxplayer_bandwidth_switch(GxPlayer* p, unsigned int index)
{
	PlayerPlayInfo info;

	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if(p->media_play == NULL ||
			p->media_play->stream == NULL ||
			index >= p->media_play->stream->prog_max) {
		PLAYER_MEDIA_UNLOCK(p);
		return GX_PLAYER_ERROR;
	}
	PLAYER_MEDIA_UNLOCK(p);

	p->bandwidth = index;

	gxplayer_get_time(p, &p->start_timems, NULL, NULL);

	info.start = p->start_timems;
	info.volume = p->volume;
	info.rect = p->rect;
	info.audio_pid  = p->audio_pid;
	info.audio1_pid = p->audio1_pid;
	info.video_pid  = p->video_pid;
	info.sub_pid    = p->sub_pid;

	return gxplayer_media_play_step1(p, p->srcurl, &info);
}

status_t gxpvrplayer_new_segment(GxPlayer *p, char *prefix, PlayerPvrSegmentInfo *info)
{
	int ret = 0;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->recorder == NULL) {
		p->recorder = av_mallocz(sizeof(PlayerRecorder));
		if (p->recorder == NULL) {
			gxloge("%s %d: malloc failed\n", __func__, __LINE__);
			PLAYER_MEDIA_UNLOCK(p);
			return -1;
		}
	}
	_player_clr_recorder(p->recorder, 0);
	p->recorder->segconfig.prefix = av_strdup(prefix);
	p->recorder->segconfig.desc   = av_strdup(info->desc);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_NEW_SEGMENT;
		ctrl.arg = (void *)&p->recorder->segconfig;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_delete_segment(GxPlayer *p, char *prefix)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_DELETE_SEGMENT;
		ctrl.arg = (void *)prefix;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_open_event(GxPlayer *p, char *prefix, PlayerPvrEventInfo *info)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;
		GxRecordPVREventSet evtconfig;

		evtconfig.prefix       = prefix;
		evtconfig.desc         = (char *)info->desc;
		evtconfig.chapter      = (char *)info->chapter;
		evtconfig.chapter_desc = (char *)info->chapter_desc;
		evtconfig.start_time   = (char *)info->start_time;
		evtconfig.end_time     = (char *)info->end_time;
		ctrl.opt = GX_RECORD_PVR_OPEN_EVENT;
		ctrl.arg = (void *)&evtconfig;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_close_event(GxPlayer *p, char *prefix)
{
	int ret = -1;

	if (p == NULL) return 0;

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_CLOSE_EVENT;
		ctrl.arg = (void *)prefix;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_delete_event(GxPlayer *p, char *prefix)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_DELETE_EVENT;
		ctrl.arg = (void *)prefix;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_open_list(GxPlayer *p, char *prefix, PlayerPvrListInfo *info)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;
		GxRecordPVRListSet lstconfig;

		lstconfig.prefix = prefix;
		lstconfig.desc   = (char *)info->desc;
		ctrl.opt = GX_RECORD_PVR_OPEN_LIST;
		ctrl.arg = (void *)&lstconfig;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_close_list(GxPlayer *p, char *prefix)
{
	int ret = -1;

	if (p == NULL) return 0;

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_CLOSE_LIST;
		ctrl.arg = (void *)prefix;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_delete_list(GxPlayer *p, char *prefix)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;

		ctrl.opt = GX_RECORD_PVR_DELETE_LIST;
		ctrl.arg = (void *)prefix;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_add_segment_in_event(GxPlayer *p, char *prefix, PlayerPvrSegmentList *list)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRElem elem;
		GxRecordPVRControl ctrl;

		elem.prefix = prefix;
		elem.elem_count  = list->count;
		elem.elem_prefix = (char **)list->prefix;
		ctrl.opt = GX_RECORD_PVR_ADD_SEGMENT_IN_EVENT;
		ctrl.arg = (void *)&elem;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_remove_segment_in_event(GxPlayer *p, char *prefix, PlayerPvrSegmentList *list)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRElem elem;
		GxRecordPVRControl ctrl;

		elem.prefix = prefix;
		elem.elem_count  = list->count;
		elem.elem_prefix = (char **)list->prefix;
		ctrl.opt = GX_RECORD_PVR_REMOVE_SEGMENT_IN_EVENT;
		ctrl.arg = (void *)&elem;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_add_event_in_list(GxPlayer *p, char *prefix, PlayerPvrEventList *list)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRElem elem;
		GxRecordPVRControl ctrl;

		elem.prefix = prefix;
		elem.elem_count  = list->count;
		elem.elem_prefix = (char **)list->prefix;
		ctrl.opt = GX_RECORD_PVR_ADD_EVENT_IN_LIST;
		ctrl.arg = (void *)&elem;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

status_t gxpvrplayer_remove_event_in_list(GxPlayer *p, char *prefix, PlayerPvrEventList *list)
{
	int ret = -1;
	CHECK_PLAYER(p);

	PLAYER_MEDIA_LOCK(p);
	if (p->media_record) {
		GxRecordPVRControl ctrl;
		GxRecordPVRElem elem;

		elem.prefix = prefix;
		elem.elem_count  = list->count;
		elem.elem_prefix = (char **)list->prefix;
		ctrl.opt = GX_RECORD_PVR_REMOVE_EVENT_IN_LIST;
		ctrl.arg = (void *)&elem;
		ret = GxMedia_SetPVRControl(p->media_record, &ctrl);
	}
	PLAYER_MEDIA_UNLOCK(p);

	return ret;
}

int32_t gxmedia_vod_dvb_pause(GxPlayer* p)
{
	CHECK_PLAYER(p);

	if (p->status != PLAYER_STATUS_PLAY_RUNNING)
		return GX_PLAYER_ERROR;

	PLAYER_STATUS_MASK(p);
	_player_media_pause(p, p->media_play);
	PLAYER_STATUS_UNMASK(p);

	if (p->status == PLAYER_STATUS_PLAY_RUNNING ||
			p->status == PLAYER_STATUS_ERROR)
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_PAUSE, p->error, p->srcurl);

	return GX_PLAYER_OK;
}

status_t gxplayer_seamless_bandwidth_switch(GxPlayer* p, unsigned int index)
{
	status_t ret = GX_PLAYER_ERROR;
	ret = GxMedia_SeamlessBandwidthSwitch(p->media_play, index);
	return ret;
}

status_t gxplayer_seamless_mpegdash_switch(GxPlayer* p, unsigned int index)
{
	GxDemuxerStreamSwitch DemuxSwitch;
	int i,bandindex = index;
#if 0 /*Customer Private Customization*/
	PlayerDashShieldMaxSize dash_shield_max_size_config = {0};
	GxPlayer_SystemGet(PSYS_DASH_SHIELD_MAX_SIZE_CONFIG,&dash_shield_max_size_config);
#endif
	if(p->media_play) {
#if 0 /*Customer Private Customization*/
		if(dash_shield_max_size_config.width != 0 && dash_shield_max_size_config.height != 0) {
			PlayerMpegDashProgInfo info;
			memset(&info,0,sizeof(info));
			gxplayer_mpeg_dash_get_info(p,&info);
			for (i=0;i<MAX_V_STREAMS;i++) {
				GxStreamVideoHeader* sh = p->media_play->demuxer->v_streams[i];
				if (sh) {
					if(info.video[index].width == sh->disp_w && info.video[index].height== sh->disp_h)  {
						bandindex = i;
						break;
					}
				}
			}
		}
#endif
		DemuxSwitch.pid = bandindex;
		GxDemuxer_Control(p->media_play->demuxer, GX_DEMUXER_CTRL_SEAMLESS_BANDWIDTH_SWITCH, &DemuxSwitch);
	}
	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_dvb_resume(GxPlayer* p, int clear_data)
{
	int vdec_sync1 = 0, vdec_sync2 = 1;//pause之后，出来音视频同步
	int mosaic_drop1 = 0, mosaic_drop2 = 1;//pause之后，出来的视频不出现mosaic

	CHECK_PLAYER(p);
	if (p->status != PLAYER_STATUS_PLAY_PAUSE)
		return GX_PLAYER_ERROR;

	PLAYER_STATUS_MASK(p);
	if (clear_data){
		GxPlayer_SystemGet(PSYS_VDEC_SYNC_FLAG, &vdec_sync1);
		GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &vdec_sync2);
		GxPlayer_SystemGet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop1);
		GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop2);
		PLAYER_MEDIA_LOCK(p);
		_player_media_play(p, p->srcurl, NULL);
		PLAYER_MEDIA_UNLOCK(p);
		GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &vdec_sync1);
		GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop1);
	}else
		_player_media_resume(p, p->media_play);
	PLAYER_STATUS_UNMASK(p);

	if (p->status == PLAYER_STATUS_PLAY_PAUSE)
		GxPlayer_StatusReport(p, PLAYER_STATUS_PLAY_RUNNING, p->error, p->srcurl);

	return GX_PLAYER_OK;
}

int32_t gxmedia_vod_dvb_seek(GxPlayer* p, int64_t seek_time, SeekFlag flag)
{
	int vdec_sync1 = 0, vdec_sync2 = 2;		//seek之后先出视频，再同步:1不看到同步过程 2看到同步过程
	int mosaic_drop1 = 0, mosaic_drop2 = 1;		//pause之后，出来的视频不出现mosaic
	CHECK_PLAYER(p);

	PLAYER_STATUS_MASK(p);
	GxPlayer_SystemGet(PSYS_VDEC_SYNC_FLAG, &vdec_sync1);
	GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &vdec_sync2);
	GxPlayer_SystemGet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop1);
	GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop2);
	PLAYER_MEDIA_LOCK(p);
	_player_media_play(p, p->srcurl, NULL);
	PLAYER_MEDIA_UNLOCK(p);
	GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &vdec_sync1);
	GxPlayer_SystemSet(PSYS_VDEC_MOSAIC_DROP, &mosaic_drop1);
	PLAYER_STATUS_UNMASK(p);

	return GX_PLAYER_OK;
}

