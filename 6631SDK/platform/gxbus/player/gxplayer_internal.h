#ifndef __GX_PLAYER_INTERNAL__
#define __GX_PLAYER_INTERNAL__

#include "gx_media.h"
#include "gx_system.h"
#include "gx_mem.h"
#include "gxsecure/gxtfm_api.h"
#include "module/player/gxplayer_module.h"
#include "module/subtitle/gxsubtitle.h"

typedef struct {
	const char*           url;
	PlayerSubPID          pid;
	PlayerATSCServiceID   service_id;
	int                   use_ticktime;
}PISubParam;

typedef struct {
	uint64_t start_timems;
	uint64_t goto_timems;
}PISubTimer;

typedef struct {
#define GXPLAYER_SUB_RUNNING 1
#define GXPLAYER_SUB_PAUSED  2
#define GXPLAYER_SUB_STOPPED 3
	void*                 player;
	int                   delay;
	int                   subload;
	int                   subtype;
	int                   substatus;
	int                   pause_ack;
	PlayerSubtitle        *subout;
	GxSubData             *subint;
	char*                 lang;
	int                   sub_thread;
	int                   sub_handle;
	PISubParam            subpara;
	PISubTimer            sub_timer;
}PISubBlock;

typedef struct {
	int                   type;
	char                  *name;

	PISubBlock* (*open)   (PISubBlock* sb,PISubParam* subpara);
	void (*close)         (PISubBlock* sb);
	void (*hide)          (PISubBlock* sb);
	void (*show)          (PISubBlock* sb);
	void (*pause)         (PISubBlock* sb);
	void (*stop)          (PISubBlock* sb);
	void (*resume)        (PISubBlock* sb);
	void (*sync)          (PISubBlock* sb,int32_t timems);
	void (*switch_stream) (PISubBlock* sb,PlayerSubPID pid);
	void (*switch_render) (PISubBlock* sb,PlayerSubRender render);
	void (*switch_resolution) (PISubBlock* sb,VideoOutputMode mode);
	void (*config)        (PISubBlock* sb,void* config);
	void (*get_info)      (PISubBlock* sb,PlayerProgInfo* sub_info);
	void (*get_timeinfo)  (PISubBlock* sb, uint64_t *curtime, uint64_t *duration);
	void (*goto_localtime)(PISubBlock* sb, uint64_t  timems);
	void (*reset_source)  (PISubBlock* sb);
}PISubCtrl;

struct url_node{
	struct gxlist_head head;
	char url[PLAYER_URL_LONG+1];
};

struct stream_node{
	struct gxlist_head head;
	char url[PLAYER_URL_LONG+1];
	PlayerPlayInfo info;
	int abort;
};

#ifdef ECOS_OS
#define MAX_STREAM_THREAD (1)
#else
#define MAX_STREAM_THREAD (4)
#endif

typedef struct {
	PlayerRecordConfig  recconfig;  //record  config
	GxRecordPVRSegment  segconfig;  //segment config
	PlayerRecordEncrypt recencrypt; //record  encrpyt
} PlayerRecorder;

typedef struct {
	int                    type;
	char                   name[PLAYER_NAME_LONG];

	GxMedia                *media_play;
	GxMedia                *media_record;

	char                   srcurl[PLAYER_URL_LONG+1];
	char                   dsturl[PLAYER_URL_LONG+1];

	PlayerStatus           status;
	PlayerError            error;
	AVCodecStatus          acodec;
	AVCodecStatus          vcodec;

	PlayerWindow           rect;
	handle_t               windowmutex;
	int                    volume;
	int                    audio_pid;
	int                    audio_mask;
	int                    video_pid;
	int                    audio1_pid;
	int                    audio1_mask;
	int                    audio1_type;
	int                    audio1_dmxid;
	int                    sub_pid;
	int                    major;
	int                    minor;
	uint64_t               start_timems;

	int                    pre_opened;
	int                    speed;
	int                    seekable;
	int                    speedable;
	int                    status_masked;
	int                    bandwidth;
	PlayerRecorder        *recorder;
	PlayerPlayConfig       playconfig;

	int                    freezen;
	int                    freeze_timeout_ms;
	int                    freeze_audio_flag;
	int                    (*freeze_begin_cbk)(void*);
	int                    (*freeze_abort_cbk)(void*);
	int                    (*freeze_end_cbk)(void*, int videodisplaying);
	unsigned int           freeze_begin_tick;
	unsigned int           switch_begin_tick;
	unsigned int           switch_cost_timems;

	struct gxlist_head     playlist;

	int                    running;
	int                    thread_inited;
	handle_t               streamsem;
	handle_t               streammutex;
	handle_t               streamthreads[MAX_STREAM_THREAD];
	struct stream_node*    stream;
	struct gxlist_head     streamlist;

	PISubBlock             subblock[PLAYER_MAX_SUB_LOAD];
	handle_t               mutex;
	int                    has_video;
	int                    video_unrecoverable_cnt;
#define PLAY_STAGE_GOT_URL 1
#define PLAY_STAGE_PLAY_START 2
#define PLAY_STAGE_PLAY_FINISH 3
	int                    play_stage;
	int64_t                play_end_time;
	int                    isquiet;
	uint64_t               cur_timems;
	char                   sub_file_url[PLAYER_URL_LONG+1];
	int                    sub_file_count;
	PlayerSubTrack         sub_file_track[PLAYER_MAX_TRACK_SUB];
	int                    cur_sub_idx;
	int                    audio_delayms;
	int                    video_delayms;
	void*                  debug;
}GxPlayer;

extern handle_t player_manager_sem;
#define PLAYER_MANAGE_LOCK() GxCore_SemWait(player_manager_sem)
#define PLAYER_MANAGE_TRYLOCK(timems) GxCore_SemTimedWait(player_manager_sem, timems)
#define PLAYER_MANAGE_UNLOCK() GxCore_SemPost(player_manager_sem);

#define PLAYER_RUN_FUNC(code)\
	do \
{\
	GxPlayer* p;\
	status_t ret;\
	PLAYER_MANAGE_LOCK();\
	p = GxPlayer_Create(name);\
	ret = code;\
	PLAYER_MANAGE_UNLOCK();\
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name); \
	return ret;\
}while (0);

#define PLAYER_TRY_FUNC(code)\
	do \
{\
	GxPlayer* p;\
	status_t ret;\
	PLAYER_MANAGE_LOCK();\
	p = GxPlayer_Find(name);\
	ret = code;\
	PLAYER_MANAGE_UNLOCK();\
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name); \
	return ret;\
}while (0);

//#define PLAYER_STATUS_LOCK(p) GxCore_MutexLock(((GxPlayer*)p)->mutex);
//#define PLAYER_STATUS_UNLOCK(p) GxCore_MutexUnlock(((GxPlayer*)p)->mutex);

//#define PLAYER_MEDIA_LOCK(p) ((void)0)
//#define PLAYER_MEDIA_UNLOCK(p) ((void)0)
#define PLAYER_MEDIA_LOCK(p)    GxCore_MutexLock(((GxPlayer*)p)->mutex)
#define PLAYER_MEDIA_TRYLOCK(p) GxCore_MutexTrylock(((GxPlayer*)p)->mutex)
#define PLAYER_MEDIA_UNLOCK(p)  GxCore_MutexUnlock(((GxPlayer*)p)->mutex)

#define PLAYER_STATUS_MASK(p)   p->status_masked = 1
#define PLAYER_STATUS_UNMASK(p) p->status_masked = 0
#define PLAYER_STATUS_MASKED(p) (p->status_masked == 1)

extern GxPlayer* GxPlayer_Find(const char* name);
extern void* GxPlayer_Create(const char* name);
extern void  GxPlayer_Destroy(void* player);


extern void GxPlayer_StatusReport(void* p,int status,int error,char*srcurl);

extern PISubBlock* GxPlayer_SubOpen(PISubBlock* subblock, PISubParam* subpara);
extern PISubBlock* GxPlayer_SubSwitch(PISubBlock* subblock, int type, PISubParam* subpara);
extern void GxPlayer_SubResetSource(PISubBlock* subblock);
extern void GxPlayer_SubClose(PISubBlock* subblock);
extern void GxPlayer_SubSwitchStream(PISubBlock* subblock,PlayerSubPID pid);
extern void GxPlayer_SubSwitchRender(PISubBlock* subblock,PlayerSubRender render);
extern void GxPlayer_SubHide(PISubBlock* subblock);
extern void GxPlayer_SubShow(PISubBlock* subblock);
extern void GxPlayer_SubPause(PISubBlock* subblock);
extern void GxPlayer_SubStop(PISubBlock* subblock);
extern void GxPlayer_SubResume(PISubBlock* subblock);
extern void GxPlayer_SubSync(PISubBlock* subblock,int32_t timems);
extern void GxPlayer_SubSwitchResolution(PISubBlock* subblock,VideoOutputMode mode);
extern void GxPlayer_SubGetInfo(PISubBlock* subblock, PlayerProgInfo* info);
extern void GxPlayer_SubGetTime(PISubBlock* subblock, uint64_t *curtime, uint64_t *duration);
extern void GxPlayer_SubGotoLocalTime(PISubBlock* subblock, uint64_t timems);

extern status_t gxplayer_open(GxPlayer* p, PlayerWindow* window);
extern status_t gxplayer_close(GxPlayer* p);
extern status_t gxplayer_media_play_step1(GxPlayer* p, const char* srcurl, PlayerPlayInfo *info);
extern status_t gxplayer_media_play_step2(GxPlayer* p, struct stream_node* stream);
extern status_t gxplayer_media_play_exit(GxPlayer* p);
extern status_t gxplayer_media_play_wait(GxPlayer* p);
extern status_t gxplayer_media_record(GxPlayer* p, const char* srcurl, const char* file);
extern status_t gxplayer_media_pause(GxPlayer* p);
extern status_t gxplayer_media_resume(GxPlayer* p);
extern status_t gxplayer_media_stop(GxPlayer* p);
extern status_t gxplayer_media_flush(GxPlayer* p);
extern status_t gxplayer_media_seek(GxPlayer* p, int64_t time, SeekFlag seek_flag);
extern status_t gxplayer_media_speed(GxPlayer* p, float speed);
extern status_t gxplayer_track_add(GxPlayer* p, GxStreamTrackAdd* track);
extern status_t gxplayer_track_delete(GxPlayer* p, GxStreamTrackDel* track);
extern status_t gxplayer_video_view(GxPlayer* p, PlayerWindow* window);
extern status_t gxplayer_video_clip(GxPlayer* p, PlayerWindow* window);
extern status_t gxplayer_video_hide(GxPlayer* p);
extern status_t gxplayer_video_show(GxPlayer* p);
extern status_t gxplayer_play_background(GxPlayer* p);
extern status_t gxplayer_video_userhide(GxPlayer* p);
extern status_t gxplayer_video_usershow(GxPlayer* p);
extern status_t gxplayer_audio_sync(GxPlayer* p, int timems);
extern status_t gxplayer_audio_delay(GxPlayer* p, int delayms);
extern status_t gxplayer_video_delay(GxPlayer* p, int delayms);
extern status_t gxplayer_audio_switch(GxPlayer* p, int pid, AudioCodecType type, char* url);
extern status_t gxplayer_ad_audio_enable (GxPlayer* p, int pid, AudioCodecType type, int dmxid);
extern status_t gxplayer_ad_audio_disable(GxPlayer* p, int pid);
extern status_t gxplayer_sub_switch(GxPlayer* p, PlayerSubtitle* subdata, PlayerSubPID pid);
extern status_t gxplayer_sub_sync(GxPlayer* p, PlayerSubtitle* subdata, int timems);
extern PlayerSubtitle* gxplayer_sub_load(GxPlayer* p, PlayerSubPara* subpara);
extern status_t gxplayer_sub_unload(GxPlayer* p, PlayerSubtitle* subdata);
extern status_t gxplayer_sub_hide(GxPlayer* p, PlayerSubtitle* subdata);
extern status_t gxplayer_sub_show(GxPlayer* p, PlayerSubtitle* subdata);
extern status_t gxplayer_sub_resolution(GxPlayer* p, PlayerSubtitle* subdata, VideoOutputMode res);
extern status_t gxplayer_sub_get_time(GxPlayer* p, PlayerSubtitle* subdata, uint64_t *curtime, uint64_t *duration);
extern status_t gxplayer_sub_goto_localtime(GxPlayer* p, PlayerSubtitle* subdata, uint64_t timems);
extern status_t gxplayer_get_status(GxPlayer* p, PlayerStatusInfo* info);
extern status_t gxplayer_get_time(GxPlayer* p, uint64_t* current, uint64_t* totle, uint64_t* seek_min);
extern status_t gxplayer_get_duration(GxPlayer* p, uint64_t* duration);
extern status_t gxplayer_get_percent(GxPlayer* p, uint8_t* current);
extern status_t gxplayer_get_info(GxPlayer* p, PlayerProgInfo* info);
extern status_t gxplayer_get_syncinfo(GxPlayer* p, PlayerSyncInfo* syncinfo);
extern status_t gxplayer_get_streaminfo(GxPlayer* p, PlayerProgStreamInfo* info);
extern status_t gxplayer_get_netinfo(GxPlayer* p, PlayerNetInfo* info);
extern status_t gxplayer_get_datalen(GxPlayer* p, uint32_t* adataLen, uint32_t* vdataLen);
extern status_t gxplayer_get_proginfo(GxPlayer* player, GxMedia *media, PlayerProgInfo* info);
extern status_t gxplayer_get_prog_streaminfo(GxPlayer* player, GxMedia *media, PlayerProgStreamInfo* info);
extern status_t gxplayer_get_dual_mono (GxPlayer* player, int *dual_mono);
extern status_t gxplayer_get_recconfig (GxPlayer* player, GxMedia* media, PlayerRecordConfig* config);
extern status_t gxplayer_record_config (GxPlayer* player, PlayerRecordConfig* config);
extern status_t gxplayer_record_encrypt(GxPlayer* player, PlayerRecordEncrypt* encrpyt);
extern status_t gxplayer_play_config(GxPlayer* p, PlayerPlayConfig* config);
extern status_t gxplayer_delay_config(GxPlayer* p, PlayerDelayConfig* config);
extern status_t gxplayer_resolution_reconfig(GxPlayer* p);
extern status_t gxplayer_video_dynamic_range_monitor(GxPlayer* p);
extern status_t gxplayer_bandwidth_switch(GxPlayer* p, unsigned int index);
extern void gxplayer_interrupt_set(GxPlayer *p);
extern void gxplayer_interrupt_clr(GxPlayer *p);
extern void gxplayer_avprintf(GxPlayer* p);
extern void gxplayer_detect_video_pts_diff(GxPlayer* p);
extern status_t gxplayer_playlist_add(GxPlayer* p, const char* srcurl);
extern status_t gxplayer_playlist_del(GxPlayer* p, const char* srcurl);
extern status_t gxplayer_playlist_clr(GxPlayer* p);
extern status_t gxplayer_stream_put(GxPlayer* p, const char* srcurl, PlayerPlayInfo *info);
extern struct stream_node* gxplayer_stream_get(GxPlayer* p);
extern unsigned int gxplayer_get_video_display_frame_cnt(GxPlayer* p);
extern unsigned int gxplayer_start(GxPlayer* p);
extern unsigned int gxplayer_check_record_overflow(GxPlayer* p);
extern status_t gxplayer_stream_clr(GxPlayer* p);
extern void gxplayer_eventlist_clr(GxPlayer* p);
extern status_t gxplayer_media_save(GxPlayer* p, PlayerMediaContent content);
extern status_t gxplayer_media_restore(GxPlayer* p, PlayerMediaContent content);
extern status_t gxplayer_media_stop_play(GxPlayer* p);
extern status_t gxplayer_media_destroy_play(GxPlayer* p);
extern status_t gxplayer_media_stop_record(GxPlayer* p);
extern int gxplayer_media_read(GxPlayer* p, PlayerReadType type, uint8_t* buffer, int len);
extern int gxplayer_media_read_subcc(GxPlayer* p, uint8_t* buffer, int len);
extern void gxplayer_reset(GxPlayer* p);

extern status_t gxpvrplayer_new_segment(GxPlayer *p, char *prefix, PlayerPvrSegmentInfo *info);
extern status_t gxpvrplayer_delete_segment(GxPlayer *p, char *prefix);
extern status_t gxpvrplayer_open_event(GxPlayer *p, char *prefix, PlayerPvrEventInfo *info);
extern status_t gxpvrplayer_close_event(GxPlayer *p, char *prefix);
extern status_t gxpvrplayer_delete_event(GxPlayer *p, char *prefix);
extern status_t gxpvrplayer_open_list(GxPlayer *p, char *prefix, PlayerPvrListInfo *info);
extern status_t gxpvrplayer_close_list(GxPlayer *p, char *prefix);
extern status_t gxpvrplayer_delete_list(GxPlayer *p, char *prefix);
extern status_t gxpvrplayer_add_segment_in_event(GxPlayer *p, char *prefix, PlayerPvrSegmentList *list);
extern status_t gxpvrplayer_remove_segment_in_event(GxPlayer *p, char *prefix, PlayerPvrSegmentList *list);
extern status_t gxpvrplayer_add_event_in_list(GxPlayer *p, char *prefix, PlayerPvrEventList *list);
extern status_t gxpvrplayer_remove_event_in_list(GxPlayer *p, char *prefix, PlayerPvrEventList *list);

extern int32_t gxmedia_vod_play(const char* srcurl, int64_t start);
extern int32_t gxmedia_vod_stop(void);
extern int32_t gxmedia_vod_seek(int64_t seek_time, int flag);
extern int32_t gxmedia_vod_pause(void);
extern int32_t gxmedia_vod_resume(void);
extern int32_t gxmedia_vod_get_time(int64_t* start, int64_t* position, int64_t* duration);
extern int32_t gxmedia_vod_speed(int scale);
extern int32_t gxmedia_vod_dvb_pause(GxPlayer* p);
extern int32_t gxmedia_vod_dvb_resume(GxPlayer* p, int clear_data);
extern int32_t gxmedia_vod_dvb_seek(GxPlayer* p, int64_t seek_time, SeekFlag flag);
extern void gxmedia_vod_seturl(const char* player, const char* src_url, void* cb);
extern status_t gxplayer_seamless_bandwidth_switch(GxPlayer* p, unsigned int index);
extern status_t gxplayer_seamless_mpegdash_switch(GxPlayer* p, unsigned int index);
#endif

