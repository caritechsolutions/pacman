#include "gx_media.h"
#include "gx_mediamanager.h"
#include "gxplayer_internal.h"
#include "gx_avtick.h"

GxPlayerMediaEventNode *gxplayer_ehe_head = NULL;
void* GxPlayer_RegisterEventHook(EventCallback callback, void *usr_data)
{
	GxPlayerMediaEventNode* new_node = NULL;
	if (!( new_node = (GxPlayerMediaEventNode*)av_mallocz(sizeof(GxPlayerMediaEventNode)))) {
		gxloge("memory allocation failed.\n");
		return NULL;
	}
	new_node->callback = callback;
	new_node->usr_data = usr_data;
	new_node->next = NULL;
	if (!gxplayer_ehe_head) {
		gxplayer_ehe_head = new_node;
	} else {
		GxPlayerMediaEventNode *cur_node = gxplayer_ehe_head;
		while (cur_node->next) {
		    cur_node = cur_node->next;
		}
		cur_node->next = new_node;
	}
	return (void*)new_node;
}

void GxPlayer_UnregisterEventHook(void* node)
{
	GxPlayerMediaEventNode* cur_node = gxplayer_ehe_head;
	GxPlayerMediaEventNode* prev_node = NULL;
	GxPlayerMediaEventNode *free_node = (GxPlayerMediaEventNode *)node;

	while (cur_node && free_node) {
		if (cur_node == free_node) {
			if (!prev_node) {
				gxplayer_ehe_head = cur_node->next;
			} else {
				prev_node->next = cur_node->next;
			}
			if (cur_node) {
				av_freep(&cur_node);
			}
			return;
		}
		prev_node = cur_node;
		cur_node = cur_node->next;
	}
}

void GxPlayer_TriggerEvent(PlayerHEHStatus status, int file_format, const char *url)
{
	GxPlayerInfo* player_info = NULL;
	GxPlayerMediaEventNode* current = gxplayer_ehe_head;

	if (!(player_info = (GxPlayerInfo*)av_mallocz(sizeof(GxPlayerInfo)))) {
		gxloge("malloc fail.\n");
		return;
	}
	player_info->status = status;
	player_info->file_format = file_format;
	if (url) {
		if (!(player_info->url = av_strdup(url))) {
			av_freep(&player_info);
			gxloge("strdup fail.\n");
			return;
		}
	}

	while (current) {
		if (current->callback) {
			current->callback(player_info, current->usr_data);
		}
		current = current->next;
	}

	if (player_info->url) {
		av_freep(&(player_info->url));
	}
	av_freep(&player_info);
}

static void GxplayerEventHookStatusReport(GxPlayer* p, PlayerHEHStatus status, int file_format, const char*srcurl)
{
	if (p&&p->media_play&&p->media_play->stream)
		GxPlayer_TriggerEvent(status, p->media_play->stream->file_format, p->media_play->stream->original_url);
	else {
		GxPlayer_TriggerEvent(status, file_format, srcurl);
	}
}

void GxPlayer_EventStatus(PlayerStatus status, int file_format, char*srcurl)
{
	switch(status){
		case PLAYER_STATUS_PLAY_RUNNING:
			GxplayerEventHookStatusReport(NULL, PLAYER_EHE_STATUS_PLAY_RUNNING, file_format, srcurl);
			break;
		case PLAYER_STATUS_START_FILL_BUF:
			GxplayerEventHookStatusReport(NULL, PLAYER_EHE_STATUS_START_FILL_BUF, file_format, srcurl);
			break;
		case PLAYER_STATUS_END_FILL_BUF:
			GxplayerEventHookStatusReport(NULL, PLAYER_EHE_STATUS_END_FILL_BUF, file_format, srcurl);
			break;
		case PLAYER_STATUS_PLAY_END:
			GxplayerEventHookStatusReport(NULL, PLAYER_EHE_STATUS_PLAY_END, file_format, srcurl);
			break;
		case PLAYER_STATUS_ERROR:
			GxplayerEventHookStatusReport(NULL, PLAYER_EHE_STATUS_ERROR, file_format, srcurl);
			break;
		default:
			break;
	}
}

status_t GxPlayer_Open(const char* name, PlayerWindow* window)
{
	GxPlayer* p = NULL;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Create(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_START, -1, NULL);
	if (p != NULL)
		ret = gxplayer_open(p, window);
	if (GX_PLAYER_OK != ret)
		GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_ERROR, -1, NULL);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_Close(const char* name)
{
	GxPlayer* p = NULL;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_STOPPED, -1, NULL);
	ret = gxplayer_close(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaPlay(const char* name, const char* srcurl, int64_t start, int volume, PlayerWindow* window)
{
	GxPlayer* p = NULL;
	status_t ret = GX_PLAYER_ERROR;
	PlayerPlayInfo info;
	unsigned int tick = av_get_tick(NULL, __LINE__);

	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	memset(&info, 0, sizeof(info));
	PLAYER_MANAGE_LOCK();
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_START, -1, srcurl);
	p = GxPlayer_Create(name);
	if (p != NULL) {
		p->switch_begin_tick = tick;
		p->switch_cost_timems = 0;
		info.start = start;
		info.volume = volume;
		info.audio_pid = -1;
		info.video_pid = -1;
		info.sub_pid = -1;
		if (window)
			info.rect = *window;
		ret = gxplayer_media_play_step1(p, srcurl, &info);
	}
	if (GX_PLAYER_OK != ret)
		GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_ERROR, -1, srcurl);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaWaitPlay(const char* name)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_START, -1, NULL);
	ret = gxplayer_media_play_wait(p);
	if (GX_PLAYER_OK != ret)
		GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_ERROR, -1, NULL);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaPlaylistAdd(const char* name, const char* srcurl)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_playlist_add(p, srcurl);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaPlaylistDel(const char* name, const char* srcurl)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_playlist_del(p, srcurl);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaPlaylistClr(const char* name)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_playlist_clr(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaPlay2(const char* name, const char* srcurl, PlayerPlayInfo *info)
{
	GxPlayer* p = NULL;
	status_t ret = GX_PLAYER_ERROR;
	unsigned int tick = av_get_tick(NULL, __LINE__);

	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	PLAYER_MANAGE_LOCK();
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_START, -1, srcurl);
	p = GxPlayer_Create(name);
	if (p != NULL) {
		p->switch_begin_tick = tick;
		ret = gxplayer_media_play_step1(p, srcurl, info);
	}
	if (GX_PLAYER_OK != ret)
		GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_ERROR, -1, srcurl);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaAudioDelayMs(const char *name, int delay)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_audio_delay(p, delay));
}

status_t GxPlayer_MediaVideoDelayMs(const char *name, int delay)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_video_delay(p, delay));
}

status_t GxPlayer_MediaPlayConfig(const char* name, PlayerPlayConfig* config)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_play_config(p, config));
}

status_t GxPlayer_MediaDelayConfig(const char* name, PlayerDelayConfig* config)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_delay_config(p, config));
}

status_t GxPlayer_MediaPause(const char* name)
{
	status_t ret;
	GxPlayer* p;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_PAUSE, -1, NULL);
	ret = gxplayer_media_pause(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaResume(const char* name)
{
	status_t ret;
	GxPlayer* p;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_RESUME, -1, NULL);
	ret = gxplayer_media_resume(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaExitPlay(const char* name)
{
	status_t ret;
	GxPlayer* p = GxPlayer_Find(name);

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	gxplayer_interrupt_set(p);
	PLAYER_MANAGE_LOCK();
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_STOPPED, -1, NULL);
	ret = gxplayer_media_play_exit(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaStop(const char* name)
{
	status_t ret;
	GxPlayer* p = GxPlayer_Find(name);

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	gxplayer_interrupt_set(p);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_STOPPED, -1, NULL);
	ret = gxplayer_media_stop(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSeek(const char* name,int64_t time,SeekFlag flag)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s, %lld\n", __func__, __LINE__, name, time);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_SEEK, -1, NULL);
	ret = gxplayer_media_seek(p, time, flag);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_PLAY_RUNNING, -1, NULL);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSpeed(const char* name, float speed)
{
	av_flow_log("[FLOW] - [%s %d]: %s, %f\n", __func__, __LINE__, name, speed);
	PLAYER_TRY_FUNC(gxplayer_media_speed(p, speed));
}

status_t GxPlayer_MediaTrackAdd(const char* name, GxStreamTrackAdd* track)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_track_add(p, track));
}

status_t GxPlayer_MediaTrackDel(const char* name, GxStreamTrackDel* track)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_track_delete(p, track));
}

status_t GxPlayer_MediaAdAudioEnable(const char* name, int pid, AudioCodecType type, int dmxid)
{
	av_flow_log("[FLOW] - [%s %d]: %s, %d\n", __func__, __LINE__, name, pid);
	PLAYER_TRY_FUNC(gxplayer_ad_audio_enable(p, pid, type, dmxid));
}

status_t GxPlayer_MediaAdAudioDisable(const char* name)
{
	av_flow_log("[FLOW] - [%s %d]: %s, %d\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_ad_audio_disable(p, -1));
}

status_t GxPlayer_MediaBandwidthSwitch(const char* name, unsigned int index)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_BANDWIDTH_SWITCH, -1, NULL);
	ret = gxplayer_bandwidth_switch(p, index);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSeamlessBandwidthSwitch(const char* name, unsigned int index)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_BANDWIDTH_SWITCH, -1, NULL);
	ret = gxplayer_seamless_bandwidth_switch(p, index);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSeamlessMpegDashSwitch(const char* name, PlayerMpegDashSwitch indata)
{
	int is_full_detection = 0;
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
	if (!is_full_detection)
		return ret;
//	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	if(p) {
		GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_BANDWIDTH_SWITCH, -1, NULL);
		if(indata.video_id >= 0) {
			ret = gxplayer_seamless_mpegdash_switch(p, indata.video_id);
		}
		if(indata.audio_id >= 0) {
			ret |= gxplayer_audio_switch(p, indata.audio_id, AUDIO_CODEC_UNKNOWN, NULL);
		}
		if(indata.subdata && indata.sub_id.pid >= 0) {
			ret |= gxplayer_sub_switch(p,indata.subdata,indata.sub_id);
		}
	}
//	PLAYER_MANAGE_UNLOCK();
	return ret;
}

status_t GxPlayer_MediaWindow(const char* name, PlayerWindow* window)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_video_view(p, window));
}

status_t GxPlayer_MediaClip(const char* name, PlayerWindow* window)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_video_clip(p, window));
}

status_t GxPlayer_MediaVideoHide(const char* name)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_video_userhide(p));
}

status_t GxPlayer_MediaVideoShow(const char* name)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_video_usershow(p));
}

status_t GxPlayer_MediaFlush(const char* name)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_media_flush(p));
}

status_t GxPlayer_MediaAudioSync(const char* name, int timems)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_audio_sync(p, timems));
}

PlayerSubtitle* GxPlayer_MediaSubLoad(const char* name, PlayerSubPara* subpara)
{
	GxPlayer* p;
	PlayerSubtitle* ret = NULL;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();

	p = GxPlayer_Find(name);

	ret = gxplayer_sub_load(p, subpara);

	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);

	return ret;
}

status_t GxPlayer_MediaAudioSwitch(const char* name, int pid, AudioCodecType type, char* url)
{
	status_t ret;
	GxPlayer* p;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_AUDIO_SWITCH, -1, NULL);
	ret = gxplayer_audio_switch(p, pid, type, url);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSubSwitch(const char* name, PlayerSubtitle* subdata,PlayerSubPID pid)
{
	status_t ret;
	GxPlayer* p;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_SUB_SWITCH, -1, NULL);
	ret = gxplayer_sub_switch(p, subdata, pid);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaSubSync(const char* name, PlayerSubtitle* subdata,int timems)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_sync(p, subdata, timems));
}

status_t GxPlayer_MediaSubUnLoad(const char* name, PlayerSubtitle* subdata)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_unload(p, subdata));
}

status_t GxPlayer_MediaSubHide(const char* name, PlayerSubtitle* subdata)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_hide(p, subdata));
}

status_t GxPlayer_MediaSubShow(const char* name, PlayerSubtitle* subdata)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_show(p, subdata));
}

status_t GxPlayer_MediaSubResolution(const char* name, PlayerSubtitle* subdata, int resolution)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_resolution(p, subdata, (VideoOutputMode)resolution));
}

status_t GxPlayer_MediaSubGetTime(const char* name, PlayerSubtitle* subdata, uint64_t *curtime, uint64_t *duration)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_get_time(p, subdata, curtime, duration));
}

status_t GxPlayer_MediaSubGotoLocalTime(const char* name, PlayerSubtitle* subdata, uint64_t localtime)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_sub_goto_localtime(p, subdata, localtime));
}

status_t GxPlayer_MediaGetStatus(const char* name, PlayerStatusInfo* info)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS){
		return GX_PLAYER_ERROR;
	}
	p = GxPlayer_Find(name);
	ret = gxplayer_get_status(p, info);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetTime(const char* name, PlayTimeInfo *info)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS) {
		return GX_PLAYER_ERROR;
	}

	p = GxPlayer_Find(name);
	ret = gxplayer_get_time(p, &info->current, &info->totle, &info->seek_min);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s, %lld, %lld, %lld (ret=%d)\n",
			__func__, __LINE__, name, info->current, info->totle, info->seek_min, ret);
	return ret;
}

status_t GxPlayer_MediaGetDuration(const char* name, uint64_t *duration)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS) {
		return GX_PLAYER_ERROR;
	}

	p = GxPlayer_Find(name);
	ret = gxplayer_get_duration(p, duration);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}


status_t GxPlayer_MediaGetPercent(const char* name, uint8_t* current)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS){
		return GX_PLAYER_ERROR;
	}
	p = GxPlayer_Find(name);
	ret = gxplayer_get_percent(p, current);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}
status_t GxPlayer_MediaGetDataLength(const char* name, uint32_t* aDataLen, uint32_t* vDataLen)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS){
		return GX_PLAYER_ERROR;
	}
	p = GxPlayer_Find(name);
	ret = gxplayer_get_datalen(p, aDataLen, vDataLen);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetProgInfoByName(const char* name, PlayerProgInfo* info)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_get_info(p, info);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetSyncInfo(const char* name, PlayerSyncInfo* info)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_get_syncinfo(p, info);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetProgStreamInfoByName(const char* name, PlayerProgStreamInfo* info)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_get_streaminfo(p, info);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetDualMonoByName(const char* name, int *dual_mono)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_get_dual_mono(p, dual_mono);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetCostTimeByName(const char *name, PlayerProgCostTime *costtime)
{
	GxPlayer* p;
	int ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	if (p) {
		av_debug_get_costtime(p->debug, costtime);
		ret = GX_PLAYER_OK;
	}
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaGetNetInfo(const char* name, PlayerNetInfo* info)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(1000);
	if (ret != GXCORE_SUCCESS) {
		return GX_PLAYER_ERROR;
	}

	p = GxPlayer_Find(name);
	ret = gxplayer_get_netinfo(p, info);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////

PlayerID3Info* GxPlayer_MediaGetID3Info(const char* file)
{
	ID3V1* pID3V1 = NULL;
	ID3V2FL* pID3V2FL = NULL;
	PlayerID3Info* pID3=NULL;
	int tagflag = 0;
	int ID3v2Size = 0;
	GxStream* s = NULL;


	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, file);
	if (!(s = GxStream_Open(file, GX_STREAM_READ, 0))){
		gxlogd("Open file '%s' failure!\n",file);
		return NULL;
	}

	if (!(pID3 = (PlayerID3Info* )av_malloc(sizeof(PlayerID3Info))) ){
		gxlogd("Memory allocate for ID3V error");
		return NULL;
	}

	if (!(pID3V1 = (ID3V1* )av_malloc(TAGSIZE))){
		gxlogd("Memory allocate for ID3V1 error");
		av_free(pID3);
		return NULL;
	}

	if (!(pID3V2FL = (ID3V2FL*)av_malloc(TAGFLSIZE))){
		gxlogd("Memory allocate for ID3V2 error");
		av_free(pID3V1);
		av_free(pID3);
		return NULL;
	}

	memset(pID3,0,sizeof(PlayerID3Info));
	memset(pID3V1,0,TAGSIZE);
	memset(pID3V2FL,0,TAGFLSIZE);

	if (!get_ID3V2Tag_info(s, pID3V2FL, &ID3v2Size)){
		tagflag += HAS_ID3v2;
		pID3->v2 = (PlayerID3V2*)pID3V2FL;
	}else{
		av_free(pID3V2FL);
		pID3V2FL = NULL;
	}

	if (!get_ID3V1Tag_info(s, pID3V1)){
		tagflag += HAS_ID3v1;
		pID3->v1 = (PlayerID3V1*)pID3V1;
	}else{
		av_free(pID3V1);
		pID3V1 = NULL;
	}

	GxStream_Close(s);

#if 1
	print_ID3v1Tag(pID3V1);
	print_ID3v2Tag(pID3V2FL);
#endif

	if (tagflag==0)
	{
		av_free(pID3);
		return NULL;
	}

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	return pID3;
}

status_t GxPlayer_MediaFreeID3Info(PlayerID3Info* pID3)
{
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	if (pID3!=NULL)
	{
		av_free(pID3->v1);
		free_frame_linkedtable((ID3V2FL*)(pID3->v2));
		av_free(pID3);
	}
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	return GX_PLAYER_OK;
}

status_t GxPlayer_MediaGetProgInfoByUrl(const char* srcurl, PlayerProgInfo* info)
{
	int ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, srcurl);
	if (info) {
		GxMedia* media;
		GxMediaPara MediaPara;

		memset(&MediaPara, 0, sizeof(MediaPara));
		MediaPara.prober.url = srcurl;
		media = GxMedia_New(GX_MEDIA_PROBER, &MediaPara);
		ret = gxplayer_get_proginfo(NULL, media, info);
		GxMedia_Destroy(media, 0);
	}
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);

	return ret;
}

status_t GxPlayer_MediaGetProgStreamInfoByUrl(const char *srcurl, PlayerProgStreamInfo *info)
{
	int ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, srcurl);
	if (info) {
		GxMedia* media;
		GxMediaPara MediaPara;

		memset(&MediaPara, 0, sizeof(MediaPara));
		MediaPara.prober.url = srcurl;
		media = GxMedia_New(GX_MEDIA_PROBER, &MediaPara);
		ret = gxplayer_get_prog_streaminfo(NULL, media, info);
		GxMedia_Destroy(media, 0);
	}
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);

	return ret;
}

status_t GxPlayer_MediaSave(const char* name, PlayerMediaContent content)
{
	GxPlayer* p;
	status_t ret;
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_media_save(p, content);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaRestore(const char* name, PlayerMediaContent content)
{
	GxPlayer* p;
	status_t ret;
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_media_restore(p, content);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaStopPlay(const char* name)
{
	GxPlayer* p;
	status_t ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_STOPPED, -1, NULL);
	ret = gxplayer_media_stop_play(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaDestroyPlay(const char* name)
{
	GxPlayer* p;
	status_t ret;
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	GxplayerEventHookStatusReport(p, PLAYER_EHE_STATUS_STOPPED, -1, NULL);
	ret = gxplayer_media_destroy_play(p);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

int GxPlayer_MediaRead(const char *name, PlayerReadType type, uint8_t *buffer, int len)
{
	GxPlayer* p;
	int ret;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_MANAGE_LOCK();
	p = GxPlayer_Find(name);
	ret = gxplayer_media_read(p, type, buffer, len);
	PLAYER_MANAGE_UNLOCK();
	av_flow_log("[FLOW] - [%s %d]: %s, %d\n", __func__, __LINE__, name, len);
	return ret;
}

int GxPlayer_MediaReadSubCC(const char *name, uint8_t *subcc, int len)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_media_read_subcc(p, subcc, len));
}

int GxPlayer_MediaCurrentPts(const char *name, int32_t* pts)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	ret = PLAYER_MANAGE_TRYLOCK(200);
	if (ret != GXCORE_SUCCESS) {
		return GX_PLAYER_ERROR;
	}

	p = GxPlayer_Find(name);
	*pts = GxMedia_GetCurrentPts(p->media_play);
	PLAYER_MANAGE_UNLOCK();
	return 0;
}

