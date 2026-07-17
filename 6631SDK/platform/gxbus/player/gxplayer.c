#include "gxplayer_internal.h"

static const char*  status_print[] = {
	"PLAYER_STATUS_STOPPED"             ,
	"PLAYER_STATUS_PLAY_START"          ,
	"PLAYER_STATUS_PLAY_PAUSE"          ,
	"PLAYER_STATUS_PLAY_RUNNING"        ,
	"PLAYER_STATUS_PLAY_END"            ,
	"PLAYER_STATUS_RECORD_PAUSE"        ,
	"PLAYER_STATUS_RECORD_RUNNING"      ,
	"PLAYER_STATUS_RECORD_FULL"         ,
	"PLAYER_STATUS_RECORD_END"          ,
	"PLAYER_STATUS_SHIFT_START"         ,
	"PLAYER_STATUS_SHIFT_PAUSE"         ,
	"PLAYER_STATUS_SHIFT_RUNNING"       ,
	"PLAYER_STATUS_SHIFT_SWITCH"        ,
	"PLAYER_STATUS_SHIFT_HOLD_PAUSE"    ,
	"PLAYER_STATUS_SHIFT_HOLD_RUNNING"  ,
	"PLAYER_STATUS_SHIFT_END"           ,
	"PLAYER_STATUS_ERROR"               ,
	"PLAYER_STATUS_START_FILL_BUF"      ,
	"PLAYER_STATUS_END_FILL_BUF"        ,
	"PLAYER_STATUS_SERVER_ERROR"        ,
	"PLAYER_STATUS_PLAY_SOF"            ,
};

static const char*  error_print[] = {
	"PLAYER_ERROR_NO_ERROR"             ,
	"PLAYER_ERROR_NO_DATA_SOURCE"       ,
	"PLAYER_ERROR_NO_DEMUX_TOOLS"       ,
	"PLAYER_ERROR_NO_MUXER_TOOLS"       ,
	"PLAYER_ERROR_NO_DUMP_TOOLS"        ,
	"PLAYER_ERROR_NO_VIDEO_DECODER"     ,
	"PLAYER_ERROR_NO_AUDIO_DECODER"     ,
	"PLAYER_ERROR_NO_SUB_DECODER"       ,
	"PLAYER_ERROR_NO_VIDEO_RENDER"      ,
	"PLAYER_ERROR_NO_AUDIO_RENDER"      ,
	"PLAYER_ERROR_NO_SUB_RENDER"        ,
	"PLAYER_ERROR_DEMUX_ERROR"          ,
	"PLAYER_ERROR_VIDEO_DECODER_ERROR"  ,
	"PLAYER_ERROR_AUDIO_DECODER_ERROR"  ,
	"PLAYER_ERROR_SUB_DECODER_ERROR"    ,
	"PLAYER_ERROR_VIDEO_RENDER_ERROR"   ,
	"PLAYER_ERROR_AUDIO_RENDER_ERROR"   ,
	"PLAYER_ERROR_SUB_RENDER_ERROR"     ,
	"PLAYER_ERROR_ACCESS_ERROR"         ,
	"PLAYER_ERROR_MUXER_ERROR"          ,
	"PLAYER_ERROR_DUMPER_ERROR"         ,
	"PLAYER_ERROR_SOURCE_RESTART"       ,
};

static const char*  codec_print[] = {
	"AVCODEC_STOPPED"  ,
	"AVCODEC_RUNNING"  ,
	"AVCODEC_PAUSED"   ,
	"AVCODEC_FINISHED" ,
	"AVCODEC_UNSUPPORT",
	"AVCODEC_BUSY"     ,
	"AVCODEC_ERROR"    ,
	"AVCODEC_READY"    ,
	"AVCODEC_STARTED"  ,
	"AVCODEC_TIMEOUT"  ,
};

static int player_inited_count = 0;

static struct gxlist_head player_event_list;
static gx_mempool* player_event_mempool;

static struct gxlist_head player_node_list;
static int player_monitor_thread_id = 0;

handle_t player_manager_sem = -1;

struct player_node {
	struct gxlist_head head;
	GxPlayer           player;
	AVCodecStatus      acodec;
	AVCodecStatus      vcodec;
	float              speed;
	int                resolution;
	int                debug_avcount;
};

struct player_event_packet {
	struct gxlist_head      msg_head;
	PlayerEventID           msg_id;
	union{
		PlayerEventAVCodec  avcodec;
		PlayerEventStatus   status;
		PlayerEventSpeed    speed;
		PlayerEventResolution resolution;
	}msg_args;
};

static int player_freeze_begin_cbk(void* arg);
static int player_freeze_abort_cbk(void* arg);
static int player_freeze_end_cbk(void* arg, int videodisplying);
static void gxplayer_thread_init(GxPlayer* p);
static void gxplayer_thread_uninit(GxPlayer* p);

static int player_interrupt = 0;
static int player_interrupt_callback(void)
{
	return player_interrupt;
}

void gxplayer_interrupt_set(GxPlayer *p)
{
	if (p && (!URL_IS_DVB(p->srcurl)) && (!URL_IS_LOGO(p->srcurl))) {
		player_interrupt = 1;
	}
}

void gxplayer_interrupt_clr(GxPlayer *p)
{
	if (p && (!URL_IS_DVB(p->srcurl)) && (!URL_IS_LOGO(p->srcurl))) {
		player_interrupt = 0;
	}
}

static void player_event_get(struct player_event_packet** packet)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (packet) {
		*packet = NULL;
		gxlist_for_each_safe(pos, n, &player_event_list) {
			*packet = gxlist_entry(pos, struct player_event_packet, msg_head);
			gxlist_del(pos);
			break;
		}
	}
}

static void player_event_put(struct player_event_packet* packet)
{
	if (packet) {
		gxlist_add_tail((struct gxlist_head *)packet, &player_event_list);
	}
}

static struct player_event_packet* player_event_new(void)
{
	return (struct player_event_packet*)gx_mempool_alloc(player_event_mempool, sizeof(struct player_event_packet));
}

static void player_event_free(struct player_event_packet* packet)
{
	if (packet) {
		gx_mempool_free(player_event_mempool, packet, sizeof(struct player_event_packet));
	}
}

static void player_event_list_create(void)
{
	player_event_mempool = gx_mempool_create(sizeof(struct player_event_packet), 16, NULL);
	GX_INIT_LIST_HEAD(&player_event_list);
}

static void player_event_list_destroy(void)
{
	struct player_event_packet* packet;

	player_event_get(&packet);

	while (packet) {
		player_event_free(packet);
		player_event_get(&packet);
	}

	gx_mempool_destory(player_event_mempool);
}

static struct player_node* player_node_new(const char* name)
{
	int i = 0;
	struct player_node* node = (struct player_node*)av_mallocz(sizeof(struct player_node));

	if (node == NULL)
		return NULL;

	node->speed = 1000;
	node->acodec.state = AVCODEC_STOPPED;
	node->vcodec.state = AVCODEC_STOPPED;
	node->acodec.err_code = AVCODEC_ERR_NONE;
	node->vcodec.err_code = AVCODEC_ERR_NONE;
	node->player.speed  = 1000;
	node->player.bandwidth  = -1;
	node->player.status_masked  = 1;
	node->player.sub_file_count = 0;
	node->player.cur_sub_idx    = -1;
	node->player.status = PLAYER_STATUS_STOPPED;
	node->player.error  = PLAYER_ERROR_NO_ERROR;
	node->player.acodec.state = AVCODEC_STOPPED;
	node->player.vcodec.state = AVCODEC_STOPPED;
	node->player.freeze_begin_cbk = player_freeze_begin_cbk;
	node->player.freeze_abort_cbk = player_freeze_abort_cbk;
	node->player.freeze_end_cbk   = player_freeze_end_cbk;
	node->player.playconfig.tsfilter.apid  = -1;
	node->player.playconfig.tsfilter.apid1 = -1;
	node->player.playconfig.tsfilter.vpid  = -1;
	node->player.cur_timems = 0;
	node->player.video_delayms   = 0;
	node->player.audio_delayms   = 0;
	for (i=0; i<PLAYER_FILTER_MAX; i++){
		node->player.playconfig.tsfilter.callback[i].pid = -1;
		node->player.playconfig.tsfilter.callback[i].type = 0;
		node->player.playconfig.tsfilter.callback[i].writeback = NULL;
	}
	GX_INIT_LIST_HEAD(&node->player.playlist);
	strncpy(node->player.name, name, PLAYER_NAME_LONG);
	GxCore_MutexCreate(&(node->player.mutex));
	node->player.debug = av_debug_create(name);
	return node;
}

static void player_node_destroy(struct player_node* node)
{
	GxPlayer* p = &(node->player);

	gxplayer_thread_uninit(p);
	gxplayer_eventlist_clr(p);
	gxplayer_playlist_clr(p);
	av_debug_destory(p->debug);
	GxCore_MutexDelete(p->mutex);
	av_free(node);
}

static void player_node_add(struct player_node* node)
{
	if (node) {
		gxlist_add_tail((struct gxlist_head *)node, &player_node_list);
	}
}

static void player_node_remove(struct player_node* node)
{
	if (node) {
		gxlist_del(&(node->head));
	}
}

static void player_node_list_destroy(void)
{
	struct player_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;

	gxlist_for_each_safe(pos, n, &player_node_list) {
		node = gxlist_entry(pos, struct player_node, head);
		player_node_remove(node);
		player_node_destroy(node);
	}
}

static int player_freeze_begin_cbk(void* arg)
{
	int mute_state = 0;
	GxPlayer* player = arg;

	if (MEDIA_HAS_AUDIO(player->media_play)) {
		if (player->freeze_audio_flag == 0 && (MEDIA_HAS_VIDEO(player->media_play)) && VIDEO_NO_ERROR(player->media_play)) {
			player->freeze_audio_flag = 1;
			GxPlayer_SystemGet(PSYS_AOUT_MUTE, &mute_state);
			if (!mute_state) {
				player->freeze_audio_flag = 1;
				GxPlayer_SystemSet(PSYS_AOUT_MUTE, &player->freeze_audio_flag);
			}
		}
	}

	GxPlayer_SystemGet(PSYS_VDEC_TIMEOUT, &player->freeze_timeout_ms);
	player->freeze_timeout_ms /= 1000;
	player->freeze_begin_tick = av_get_tick(NULL, __LINE__);

	return 0;
}

static int player_freeze_abort_cbk(void* arg)
{
	GxPlayer* player = arg;
	player->freeze_begin_tick = 0;

	return 0;
}

static int player_freeze_end_cbk(void* arg, int videodisplying)
{
	int mute_state = 0;
	GxPlayer* player = arg;

	if (MEDIA_HAS_AUDIO(player->media_play)) {
		if (player->freeze_audio_flag == 1) {
			player->freeze_audio_flag = 0;
			GxPlayer_SystemGet(PSYS_AOUT_MUTE, &mute_state);
			if (mute_state != -1) {
				GxPlayer_SystemSet(PSYS_AOUT_MUTE, &player->freeze_audio_flag);
			}
		}
	}

	if (MEDIA_HAS_VIDEO(player->media_play) && videodisplying == 0) {
		if (player->status != PLAYER_STATUS_SHIFT_PAUSE && player->status != PLAYER_STATUS_PLAY_PAUSE) {
				gxplayer_play_background(player);
		}
	}

	player->freeze_begin_tick = 0;

	return 0;
}

static void _player_play_thread(void* arg)
{
	GxPlayer* p = arg;
	struct stream_node* stream = NULL;
	while (p->running) {
		GxCore_SemWait(p->streamsem);
		stream = gxplayer_stream_get(p);
		if (stream) {
			struct stream_node* node;
			struct gxlist_head* pos;
			struct gxlist_head* n;

			gxplayer_media_play_step2(p, stream);
			av_debug_end_duty(p->debug);

			GxCore_MutexLock(p->streammutex);
			gxlist_for_each_safe(pos, n, &p->streamlist) {
				node = gxlist_entry(pos, struct stream_node, head);
				if (node == stream){
					gxlist_del(pos);
					break;
				}
			}
			av_free(stream);
			GxCore_MutexUnlock(p->streammutex);
		}
	}
}

static void gxplayer_thread_init(GxPlayer* p)
{
	if (p && p->thread_inited == 0) {
		int i;
		p->running = 1;
		GxCore_SemCreate(&(p->streamsem), 0);
		GxCore_MutexCreate(&(p->streammutex));
		GxCore_MutexCreate(&(p->windowmutex));
		GX_INIT_LIST_HEAD(&(p->streamlist));
		for (i=0; i<MAX_STREAM_THREAD; i++)
			GxCore_ThreadCreate("play step2", &p->streamthreads[i], _player_play_thread, p, 1024*32, GXOS_DEFAULT_PRIORITY-1);
		p->thread_inited = 1;
	}
}

static void gxplayer_thread_uninit(GxPlayer* p)
{
	if (p && p->thread_inited == 1) {
		int i;
		p->running = 0;
		p->thread_inited = 0;
		for (i=0; i<MAX_STREAM_THREAD; i++)
			GxCore_SemPost(p->streamsem);
		for (i=0; i<MAX_STREAM_THREAD; i++)
			GxCore_ThreadJoin(p->streamthreads[i]);
		GxCore_MutexDelete(p->streammutex);
		GxCore_MutexDelete(p->windowmutex);
		GxCore_SemDelete(p->streamsem);
		if (p->stream) {
			av_free(p->stream);
			p->stream = NULL;
		}
	}
}

status_t gxplayer_playlist_add(GxPlayer* p, const char* srcurl)
{
	struct url_node* node = av_mallocz(sizeof(struct url_node));
	if (node) {
		strncpy(node->url, srcurl, PLAYER_URL_LONG);
		gxlist_add_tail((struct gxlist_head *)node, &p->playlist);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

status_t gxplayer_playlist_del(GxPlayer* p, const char* srcurl)
{
	struct url_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (p && srcurl){
		gxlist_for_each_safe(pos, n, &p->playlist) {
			node = gxlist_entry(pos, struct url_node, head);
			if (strcmp(node->url, srcurl) == 0) {
				gxlist_del(pos);
				return GX_PLAYER_OK;
			}
		}
	}

	return GX_PLAYER_ERROR;
}

status_t gxplayer_playlist_clr(GxPlayer* p)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (p) {
		gxlist_for_each_safe(pos, n, &p->playlist) {
			gxlist_del(pos);
		}
	}

	return GX_PLAYER_OK;
}

GxPlayer* GxPlayer_Find(const char* name)
{
	struct player_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (name == NULL || strlen(name) >= PLAYER_NAME_LONG)
		return NULL;

	gxlist_for_each_safe(pos, n, &player_node_list) {
		node = gxlist_entry(pos, struct player_node, head);
		if (strcmp(node->player.name,name) == 0) {
			return (&(node->player));
		}
	}

	return NULL;
}

void* GxPlayer_Create(const char* name)
{
	struct player_node* node = NULL;
	struct gxlist_head* pos;

	if (name == NULL || strlen(name) >= PLAYER_NAME_LONG)
		return NULL;

	gxlist_for_each(pos, &player_node_list) {
		node = gxlist_entry(pos, struct player_node, head);
		if (strcmp(node->player.name,name) == 0) {
			node->player.bandwidth  = -1;
			node->player.cur_timems = 0;
			return (&(node->player));
		}
	}

	node = player_node_new(name);
	if (node) {
		player_node_add(node);
		return (&(node->player));
	}

	gxlogd("[Player]: Create player Failed!...\n");
	return NULL;
}

void gxplayer_eventlist_clr(GxPlayer* p)
{
	struct gxlist_head* pos;
	struct gxlist_head* n;
	struct player_event_packet* packet = NULL;
	PlayerEventStatus EventStatus;

	gxlist_for_each_safe(pos, n, &player_event_list) {
		packet = gxlist_entry(pos, struct player_event_packet, msg_head);
		if (packet) {
			EventStatus = packet->msg_args.status;
			if (strcmp(p->name, EventStatus.player) == 0) {
				gxlist_del(pos);
				player_event_free(packet);
			}
		}
	}
}

void GxPlayer_Destroy(void* player)
{
	GxPlayer* p = player;
	struct player_node* node = NULL;
	struct gxlist_head* pos;
	struct gxlist_head* n;

	if (p) {
		gxlist_for_each_safe(pos, n, &player_node_list) {
			node = gxlist_entry(pos, struct player_node, head);
			if (&(node->player) == p) {
				player_node_remove(node);
				player_node_destroy(node);
				return;
			}
		}
	}
}

static void player_status_report(GxPlayer* player, PlayerEventStatus* EventStatus)
{
	PlayerEventReport EventReport;

	if (EventStatus) {
		EventReport.event = PLAYER_EVENT_STATUS_REPORT;
		EventReport.args = EventStatus;

		if (EventStatus) {
			strncpy(EventStatus->player, player->name,   PLAYER_NAME_LONG);
			strncpy(EventStatus->url,    player->srcurl, PLAYER_URL_LONG);
		}
		av_flow_log("[Player]:[%s]: Status ==> %s ,error ==> %s\n",
				player->name,
				status_print[EventStatus->status - PLAYER_STATUS_STOPPED],
				error_print[EventStatus->error - PLAYER_ERROR_NO_ERROR]);

		GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
	}
}

static void player_avcodec_report(GxPlayer* player,
		AVCodecStatus acodec, AVCodecStatus vcodec, unsigned int changed)
{
	PlayerEventReport EventReport;
	PlayerEventAVCodec EventAVCodec;

	if (player) {
		av_flow_log("[Player]:[%s]: ACodec ==> %s [%d], VCodec ==> %s [%d]\n",
				player->name,  codec_print[acodec.state - AVCODEC_STOPPED], ((changed & ACODEC) != 0),
				codec_print[vcodec.state - AVCODEC_STOPPED], ((changed & VCODEC) != 0));

		EventAVCodec.changed = changed;
		EventAVCodec.acodec = acodec;
		EventAVCodec.vcodec = vcodec;
		strncpy(EventAVCodec.player, player->name, PLAYER_NAME_LONG);
		strncpy(EventAVCodec.url, player->srcurl, PLAYER_URL_LONG);

		EventReport.event = PLAYER_EVENT_AVCODEC_REPORT;
		EventReport.args = &EventAVCodec;
		GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
	}
}

static void player_record_overflow_report(GxPlayer* player)
{
	PlayerEventReport EventReport;
	PlayerEventRecordOverflow EventRecordOverflow;

	if (player) {
		gxlogf("[Player]: Record Monitor ==> overflow \n");

		strncpy(EventRecordOverflow.player, player->name, PLAYER_NAME_LONG);

		EventReport.args = &EventRecordOverflow;
		EventReport.event = PLAYER_EVENT_RECORD_OVERFLOW;
		GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
	}
}

#if 0
static void player_resolution_report(GxPlayer* player, int resolution)
{
	PlayerEventReport EventReport;
	PlayerEventResolution EventResolution;

	if (player) {
		gxlogf("[Player]: Resolution Monitor ==> %d \n", resolution);

		EventResolution.resolution = resolution;

		EventReport.args = &EventResolution;
		EventReport.event = PLAYER_EVENT_RESOLUTION_REPORT;
		GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
	}
}
#endif

static void player_speed_report(GxPlayer* player, float speed)
{
	PlayerEventReport EventReport;
	PlayerEventSpeed EventSpeed;

	if (player) {
		gxlogf("[Player]: Speed Monitor ==> %f \n", speed);

		EventSpeed.speed = speed/1000;
		strncpy(EventSpeed.player, player->name, PLAYER_NAME_LONG);

		EventReport.args = &EventSpeed;
		EventReport.event = PLAYER_EVENT_SPEED_REPORT;
		GxPlayer_SystemSet(PSYS_EVENT_REPORT, &EventReport);
	}
}

extern status_t player_shift2dvb(GxPlayer* p);
extern status_t player_shift2file(GxPlayer* p);
extern status_t player_reset(GxPlayer* p);
extern status_t player_pause(GxPlayer* p);
extern status_t player_reset_audio(GxPlayer* p);
extern status_t player_stop_timeshift(GxPlayer* p);
extern status_t player_stop_record(GxPlayer* p);

status_t gxplayer_stream_put(GxPlayer* p, const char* srcurl, PlayerPlayInfo *info)
{
	gxplayer_thread_init(p);

	GxCore_MutexLock(p->streammutex);
	if (p->stream == NULL) {
		if ((p->stream = av_mallocz(sizeof(struct stream_node))) == NULL){
			GxCore_MutexUnlock(p->streammutex);
			return GX_PLAYER_ERROR;
		}
		GxCore_SemPost(p->streamsem);
	}

	if (info)
		p->stream->info = *info;
	strncpy(p->stream->url, srcurl, PLAYER_URL_LONG);

	if (1) {
		struct stream_node* node;
		struct gxlist_head* pos;
		struct gxlist_head* n;

		gxlist_for_each_safe(pos, n, &p->streamlist) {
			node = gxlist_entry(pos, struct stream_node, head);
			node->abort = 1;
		}
	}

	GxCore_MutexUnlock(p->streammutex);
	return GX_PLAYER_OK;
}

status_t gxplayer_stream_clr(GxPlayer* p)
{
	if (p && p->thread_inited == 1) {

		GxCore_MutexLock(p->streammutex);

		if (1) {
			struct stream_node* node;
			struct gxlist_head* pos;
			struct gxlist_head* n;

			gxlist_for_each_safe(pos, n, &p->streamlist) {
				node = gxlist_entry(pos, struct stream_node, head);
				node->abort = 1;
			}
			if (p->stream)
				p->stream->abort = 1;
		}

		GxCore_MutexUnlock(p->streammutex);
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

struct stream_node* gxplayer_stream_get(GxPlayer* p)
{
	struct stream_node* stream;
	GxCore_MutexLock(p->streammutex);
	if (p->stream == NULL) {
		GxCore_MutexUnlock(p->streammutex);
		if (p->running)
			gxlogd("######%s fail ...\n", __func__);
		return NULL;
	}
	stream = p->stream;
	p->stream = NULL;
	gxlist_add_tail((struct gxlist_head *)stream, &p->streamlist);
	GxCore_MutexUnlock(p->streammutex);

	return stream;
}

void gxplayer_reset(GxPlayer* p)
{
	if (player_inited_count > 0) {
		struct player_node* node = NULL;
		struct gxlist_head* pos;
		struct gxlist_head* n;

		gxlist_for_each_safe(pos, n, &player_node_list) {
			node = gxlist_entry(pos, struct player_node, head);
			if (&node->player == p) {
				node->speed = 1000;
				node->acodec.state = AVCODEC_STOPPED;
				node->vcodec.state = AVCODEC_STOPPED;
				node->player.speed = 1000;
				node->player.acodec.state = AVCODEC_STOPPED;
				node->player.vcodec.state = AVCODEC_STOPPED;
				node->player.status = PLAYER_STATUS_STOPPED;
				node->player.error  = PLAYER_ERROR_NO_ERROR;
				break;
			}
		}
	}
}

static void _player_video_unsupport(GxPlayer* p)
{
	if(p->error != PLAYER_ERROR_VIDEO_DECODER_ERROR) {
		if (GxMedia_VideoUnsupport(p->media_play) == 0) {
			GxPlayer_StatusReport(p, p->status, p->media_play->errInfo, p->srcurl);
			if (p->media_play->errInfo == PLAYER_ERROR_VIDEO_DECODER_ERROR) {
				p->freeze_end_cbk(p, 0);
			}
		}
	}
}

void GxPlayer_Monitor(void)
{
	if (player_inited_count > 0) {
		struct player_node* node = NULL;
		struct gxlist_head* pos;
		struct gxlist_head* n;

		gxlist_for_each_safe(pos, n, &player_node_list) {
			node = gxlist_entry(pos, struct player_node, head);
			gxplayer_detect_video_pts_diff(&(node->player));
			if (node->player.speed == 1000) {
				PlayerStatusInfo status;
				int ret = 0;
				int debug_avinfo = 0;
				memset(&status,0,sizeof(PlayerStatusInfo));
				GxPlayer_SystemGet(PSYS_DEBUG_AV_INFO, &debug_avinfo);
				if (debug_avinfo) {
					if (node->debug_avcount > 5) {
						gxplayer_avprintf(&(node->player));
						node->debug_avcount = 0;
					} else
						node->debug_avcount++;
				}

				ret = gxplayer_get_status(&(node->player), &status);
				if (ret == GXCORE_SUCCESS && status.status != PLAYER_STATUS_ERROR) {
					//avcodec report
					if (IS_RECOVERABLE_ERROR(status.acodec.state, status.acodec.err_code)) {
						if (status.acodec.err_code == AVCODEC_ERR_CODEC_ERROR) {
							gxlogi("[audio_reset]: [AudioErr] state = 0x%x, errno = 0x%x\n", status.acodec.state, status.acodec.err_code);
							player_reset_audio(&(node->player));
						} else {
							gxlogi("[player_reset]: [AudioErr] state = 0x%x, errno = 0x%x\n", status.acodec.state, status.acodec.err_code);
							player_reset(&(node->player));
						}
					}
					else if (IS_RECOVERABLE_ERROR(status.vcodec.state, status.vcodec.err_code)) {
						node->player.video_unrecoverable_cnt = 0;
						if (MEDIA_IS_DVB(node->player.media_play) || MEDIA_IS_TS(node->player.media_play)) {
							gxlogi("[player_reset]: [VideoErr] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
							player_reset(&(node->player));
						}
						else {
							switch (status.vcodec.err_code) {
							case AVCODEC_ERR_SIZE_CHANGED:
							case AVCODEC_ERR_FB_OVERFLOW:
								{
									gxlogi("[player_reset]: [VideoErr] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
									player_reset(&(node->player));
								}
								break;
							case AVCODEC_ERR_SIZE_UNSUPPORT:
								{
									gxlogi("[player_error]: [VideoUnsupport] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
									_player_video_unsupport(&node->player);
								}
								break;
							default:
								break;
							}
						}
					}
					else if(IS_UNRECOVERABLE_ERROR(status.vcodec.state, status.vcodec.err_code)) {
						if (MEDIA_IS_DVB(node->player.media_play) || MEDIA_IS_TS(node->player.media_play)) {
							if (node->player.video_unrecoverable_cnt ++ < 3) {
								gxlogi("[player_reset]: [VideoErr] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
								player_reset(&(node->player));
							}
							else {
								node->player.video_unrecoverable_cnt = 0;
								gxlogi("[player_error]: [VideoUnsupport] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
								_player_video_unsupport(&node->player);
							}
						}
						else {
							gxlogi("[player_error]: [VideoUnsupport] state = 0x%x, errno = 0x%x\n", status.vcodec.state, status.vcodec.err_code);
							_player_video_unsupport(&node->player);
						}
					}
					else if (status.vcodec.state != node->vcodec.state || status.acodec.state != node->acodec.state) {
						unsigned int changed = 0;
						if (status.vcodec.state != node->vcodec.state) changed |= VCODEC;
						if (status.acodec.state != node->acodec.state) changed |= ACODEC;
						node->acodec = status.acodec;
						node->vcodec = status.vcodec;
						node->player.video_unrecoverable_cnt = 0;
						player_avcodec_report(&node->player, status.acodec, status.vcodec, changed);
					}
				}
			}
			else if (node->player.speed < 0 && MEDIA_FB_EOF(node->player.media_play)) {
				int replay;
				PlayerEventStatus _status;

				GxPlayer_SystemGet(PSYS_PLAY_SOF_REPLAY, &replay);
				if (replay) {
					_status.status = PLAYER_STATUS_PLAY_SOF;
					_status.error = PLAYER_ERROR_NO_ERROR;
					player_status_report(&node->player, &_status);
					player_reset(&(node->player));
				}
				else {
					if (node->player.status != PLAYER_STATUS_PLAY_PAUSE) {
						player_pause(&(node->player));
						_status.status = PLAYER_STATUS_PLAY_SOF;
						_status.error = PLAYER_ERROR_NO_ERROR;
						node->player.status = PLAYER_STATUS_PLAY_PAUSE;
						player_status_report(&node->player, &_status);
					}
				}
			}

			//add speed change report
			if (node->player.speed != node->speed) {
				node->speed = node->player.speed;
				player_speed_report(&(node->player), node->speed);
			}

			//add record overflow report
			if (node->player.media_record != NULL) {
				if (PLAYER_MEDIA_TRYLOCK((&node->player)) == GXCORE_SUCCESS) {
					unsigned int overflow = gxplayer_check_record_overflow(&(node->player));
					if (overflow) {
						player_record_overflow_report(&(node->player));
					}
					PLAYER_MEDIA_UNLOCK((&node->player));
				}
			}

			//add resolution auto monitor
			if (MEDIA_HAS_VIDEO(node->player.media_play)) {
				if (PLAYER_MEDIA_TRYLOCK((&node->player)) == GXCORE_SUCCESS) {
					gxplayer_resolution_reconfig(&(node->player));
					PLAYER_MEDIA_UNLOCK((&node->player));
				}
			}

			//add dynamic-range change monitor
			if (MEDIA_HAS_VIDEO(node->player.media_play)) {
				if (PLAYER_MEDIA_TRYLOCK((&node->player)) == GXCORE_SUCCESS) {
					gxplayer_video_dynamic_range_monitor(&(node->player));
					PLAYER_MEDIA_UNLOCK((&node->player));
				}
			}

			if (PLAYER_MEDIA_TRYLOCK((&node->player)) == GXCORE_SUCCESS) {
				// freeze frame check
				if (node->player.freeze_begin_tick != 0) {
					unsigned int tick = av_get_tick(NULL, __LINE__);
					unsigned int novideo = !MEDIA_HAS_VIDEO(node->player.media_play);
					unsigned int videodisplaying = gxplayer_get_video_display_frame_cnt(&(node->player));
					unsigned int freeze_cost_timems = tick - node->player.freeze_begin_tick;
					PlayerStatusInfo status;
					int ret = gxplayer_get_status(&(node->player), &status);

					if (novideo ||
							videodisplaying ||
							(status.vcodec.state == AVCODEC_ERROR) ||
							(freeze_cost_timems >= node->player.freeze_timeout_ms)) {
						node->player.freeze_end_cbk(&node->player, videodisplaying);
						if (node->player.switch_cost_timems == 0)
							node->player.switch_cost_timems = tick - node->player.switch_begin_tick;
						gxlogi("[Player]: switch cost %d ms, freeze cost %d ms, timeout = %d ms, videodisplaying = %d. \n",
							node->player.switch_cost_timems,
							freeze_cost_timems,
							node->player.freeze_timeout_ms,
							videodisplaying);
					}
				}
				// start video decord & display
				gxplayer_start(&(node->player));
				PLAYER_MEDIA_UNLOCK((&node->player));
			}
		}

		//play statsu report
		{
			struct player_event_packet* event_packet;
			player_event_get(&event_packet);

			if (event_packet != NULL) {
				GxPlayer* p;
				PlayerEventStatus EventStatus = event_packet->msg_args.status;
				player_event_free(event_packet);
				if ((p = GxPlayer_Find(EventStatus.player)) != NULL) {
					if (EventStatus.status == PLAYER_STATUS_SHIFT_SWITCH) {
						if (p->speed > 0) {
							player_shift2dvb(p);
						}
						else {
							p->start_timems = 0;
							player_shift2file(p);
						}
						return;
					}

					if (EventStatus.status == PLAYER_STATUS_PLAY_END) {
						struct gxlist_head* pos;
						struct gxlist_head* n;
						struct url_node* node;

						gxlist_for_each_safe(pos, n, &p->playlist) {
							node = gxlist_entry(pos, struct url_node, head);
							p->speed = 1000;
							p->start_timems = 0;
							gxplayer_media_play_step1(p, node->url, NULL);
							gxlist_del(pos);
							return;
						}
					}

					if (EventStatus.status == PLAYER_STATUS_RECORD_END) {
						gxlist_for_each_safe(pos, n, &player_node_list) {
							node = gxlist_entry(pos, struct player_node, head);
							if (strcmp(node->player.name, p->name) != 0 &&
									node->player.media_record != NULL &&
									node->player.media_record == p->media_record) {
								PlayerEventStatus _status;
								player_stop_timeshift(&node->player);
								_status.status = PLAYER_STATUS_SHIFT_END;
								_status.error = PLAYER_ERROR_NO_ERROR;
								player_status_report(&node->player, &_status);
								node->player.status = PLAYER_STATUS_PLAY_RUNNING;
								player_status_report(&node->player, &_status);
							}
						}
						gxplayer_media_stop_record(p);
						player_status_report(p, &EventStatus);
						return;
					}
					else if (EventStatus.status == PLAYER_STATUS_SHIFT_END) {
						gxlist_for_each_safe(pos, n, &player_node_list) {
							node = gxlist_entry(pos, struct player_node, head);
							if (strcmp(node->player.name, p->name) != 0 &&
									node->player.media_record != NULL &&
									node->player.media_record == p->media_record) {
								PlayerEventStatus _status;
								player_stop_record(&node->player);
								_status.status = PLAYER_STATUS_RECORD_END;
								_status.error = PLAYER_ERROR_NO_ERROR;
								player_status_report(&node->player, &_status);
							}
						}
						player_stop_timeshift(p);
						player_status_report(p, &EventStatus);
						EventStatus.status = PLAYER_STATUS_PLAY_RUNNING;
						player_status_report(p, &EventStatus);
						return;
					}

					player_status_report(p, &EventStatus);
				}
			}
		}
	}
}

static void _player_monitor_thread(void* arg)
{
	while (player_inited_count > 0) {
		PLAYER_MANAGE_LOCK();
		GxPlayer_Monitor();
		PLAYER_MANAGE_UNLOCK();

		GxCore_ThreadDelay(10);
	}
}

void GxPlayer_ModuleInit(void)
{
	if (player_inited_count++ == 0) {
		srand((unsigned int)(time(NULL)));
		if (player_manager_sem == -1)
			GxCore_SemCreate(&player_manager_sem, 1);
		player_event_list_create();
		GX_INIT_LIST_HEAD(&player_node_list);
		GxPlayer_SystemInit();
		GxPlayer_HeapInit();
		GxPlayer_SystemSet(PSYS_CBK_INTERRUPT, (void*)player_interrupt_callback);

		GxCore_ThreadCreate("pMonitor", &player_monitor_thread_id, _player_monitor_thread, NULL, 1024*32, GXOS_DEFAULT_PRIORITY-1);
	}
}

void GxPlayer_ModuleDestroy(void)
{
	if (player_inited_count-- == 1) {
		GxCore_ThreadJoin(player_monitor_thread_id);
		player_monitor_thread_id = 0;

		PLAYER_MANAGE_LOCK();
		GxPlayer_SystemUninit();
		player_node_list_destroy();
		player_event_list_destroy();
		PLAYER_MANAGE_UNLOCK();

		// player_manager_sem 不销毁，防止GxPlayer_ModuleDestroy时，还有其他Player模块的接口被调用.
		//GxCore_SemDelete(player_manager_sem);
	}
}

void GxPlayer_StatusReport(void* p,int status,int error,char*srcurl)
{
	GxPlayer* player = p;
	struct player_event_packet* event_packet;

	if (player && (!PLAYER_STATUS_MASKED(player))) {
		//time shift mask play_end??record_end ??error msg
		if (player->status == PLAYER_STATUS_SHIFT_RUNNING ||
				player->status == PLAYER_STATUS_SHIFT_START   ||
				player->status == PLAYER_STATUS_SHIFT_HOLD_PAUSE||
				player->status == PLAYER_STATUS_SHIFT_HOLD_RUNNING||
				player->status == PLAYER_STATUS_SHIFT_PAUSE ) {
			switch(status)
			{
			case PLAYER_STATUS_PLAY_END:
				status = PLAYER_STATUS_SHIFT_SWITCH;
				break;
			case PLAYER_STATUS_RECORD_END:
				status = PLAYER_STATUS_SHIFT_END;
				break;
			case PLAYER_STATUS_RECORD_FULL:
				goto keep_status;
			case PLAYER_STATUS_SHIFT_RUNNING:
			case PLAYER_STATUS_SHIFT_START:
			case PLAYER_STATUS_SHIFT_PAUSE:
			case PLAYER_STATUS_SHIFT_HOLD_PAUSE:
			case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
			case PLAYER_STATUS_STOPPED:
			case PLAYER_STATUS_SHIFT_END:
				break;
			default:
				goto mask_status;
			}
		}

		if (player->status == PLAYER_STATUS_STOPPED) {
			switch(status)
			{
			case PLAYER_STATUS_PLAY_START:
			case PLAYER_STATUS_RECORD_RUNNING:
			case PLAYER_STATUS_SHIFT_START:
			case PLAYER_STATUS_ERROR:
				break;
			default:
				goto mask_status;
			}
		}

		if (player->status == PLAYER_STATUS_PLAY_START) {
			switch(status)
			{
			case PLAYER_STATUS_PLAY_END:
				goto mask_status;
			default:
				break;
			}
		}

		if (status != PLAYER_STATUS_START_FILL_BUF && status != PLAYER_STATUS_END_FILL_BUF) {
			player->status = status;
			player->error = error;
		}

		if (player&&player->media_play&&player->media_play->stream)
			GxPlayer_EventStatus(status, player->media_play->stream->file_format, srcurl);
		else
			GxPlayer_EventStatus(status, -1, srcurl);

keep_status:
		event_packet = player_event_new();
		if (event_packet) {
			event_packet->msg_id = PLAYER_EVENT_STATUS_REPORT;
			event_packet->msg_args.status.status = status;
			event_packet->msg_args.status.error  = error;
			strncpy(event_packet->msg_args.status.player,player->name,PLAYER_NAME_LONG);
			if (srcurl)
				strncpy(event_packet->msg_args.status.url,srcurl,PLAYER_URL_LONG);
			else
				strncpy(event_packet->msg_args.status.url,player->srcurl,PLAYER_URL_LONG);
			player_event_put(event_packet);
		}
	}
	else {
		gxlogf("[Player] All Status Masked [%s]!...\n",status_print[status]);
	}

	return;

mask_status:
	gxlogf("[Player]:Mask Status [%s]\n",status_print[status]);
	return;
}

