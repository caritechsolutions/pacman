#include "gx_avtick.h"
#include "gx_mem.h"
#include "../gxplayer_internal.h"

#ifdef AV_GXPLAYER_DEBUG
typedef struct {
#define AV_DEBUG_MEDIA_MAX (4)
	int          type;
	void        *priv;
	const char  *str[AV_MEDIA_MAX_MOD];
} _av_debug_media;

typedef struct {
	unsigned int begin;
	unsigned int player_prep[AV_TICK_ST_MAX];
	unsigned int player_zoom[AV_TICK_ST_MAX];
	unsigned int player_freeze[AV_TICK_ST_MAX];
	unsigned int media  [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int stream [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int demuxer[AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int audio  [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int video  [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int subt   [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int alive  [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int seek   [AV_TICK_OP_MAX][AV_TICK_ST_MAX];
	unsigned int end;
	unsigned int first_audio_frame;
	unsigned int first_audio_frame_cnt;
	unsigned int first_video_frame;
	unsigned int first_video_frame_cnt;
	unsigned int first_video_show;
	unsigned int start_video_decode;
	unsigned int first_audio_es;
	unsigned int first_audio_es_size;
	unsigned int first_video_es;
	unsigned int first_video_es_size;
} _av_debug_timer;
#endif

typedef struct {
	unsigned int begin;
	unsigned int end;
	unsigned int audio_frame;
	unsigned int video_frame;
	unsigned int video_show;
	unsigned int video_decode;
} _av_debug_tick;

typedef struct {
	AV_TICK_FUNC    func;
	unsigned int    avframe;
	const char     *player_name;
	const char     *player_srcurl;
	const char     *player_dsturl;
#ifdef AV_GXPLAYER_DEBUG
	_av_debug_timer timer;
	_av_debug_media media[AV_DEBUG_MEDIA_MAX];
#endif
	_av_debug_tick  tick[AV_TICK_FUNC_MAX];
} _av_debug;

#define CHECK_AV_TIME_NULL(p) do { \
	if (!p)                        \
		return;                    \
} while(0);

#ifdef AV_GXPLAYER_DEBUG
static _av_debug *_duty[] = {NULL, NULL, NULL, NULL};
static char *_media_str[] = {"player", "recoder", "prober", "dumper"};
static char *_media_type  = "       Meida Type";
static char *_media[] = {
	"             Stream          ",
	"             Demuxer         ",
	"                      IFormat",
	"                      OFormat",
	"             Audio0   Decoder",
	"                      Codec  ",
	"             Audio1   Decoder",
	"                      Codec  ",
	"             Video    Decoder",
	"                      Codec  ",
	"             Subtitle Decoder",
	"             Audio    Play   ",
	"             Video    Play   ",
	"             Subtitle Play   ",
	"             Dumper          ",
};

static void __av_debug_add_duty(void *duty)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(_duty) /sizeof(_duty[0]); i++) {
		if (_duty[i] == NULL) {
			_duty[i] = duty;
			return;
		}
	}

	return;
}

static void __av_debug_del_duty(void *duty)
{
	unsigned int i = 0;

	for (i = 0; i < sizeof(_duty) /sizeof(_duty[0]); i++) {
		if (_duty[i] == duty) {
			_duty[i] = NULL;
			return;
		}
	}

	return;
}

static _av_debug_media *__av_debug_media(_av_debug *duty, void *priv)
{
	unsigned int i = 0;
	int idx = -1;

	for (i = 0; i < AV_DEBUG_MEDIA_MAX; i++) {
		if (duty->media[i].priv == NULL) {
			if (idx == -1)
				idx = i;
		}

		if (duty->media[i].priv == priv)
			return &duty->media[i];
	}

	if (idx != -1) {
		duty->media[idx].priv = priv;
		return &duty->media[idx];
	}

	return NULL;
}

static __attribute__((unused)) int __cost_time(unsigned int *tick)
{
	return (int)(tick[AV_TICK_ST_END] - tick[AV_TICK_ST_BEGIN]);
}
#endif

void *av_debug_create(const char *player_name)
{
	_av_debug *duty = av_mallocz(sizeof(_av_debug));

	if (!duty)
		return NULL;

#ifdef AV_GXPLAYER_DEBUG
	__av_debug_add_duty(duty);
#endif
	duty->player_name = player_name;

	return duty;
}

void av_debug_destory(void *handle)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	av_free((void *)duty);
#ifdef AV_GXPLAYER_DEBUG
	__av_debug_del_duty(duty);
#endif
}

void av_debug_begin_duty(void *handle, AV_TICK_FUNC func)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);

	duty->func    = func;
	duty->avframe = 0;
	tick = av_get_tick(NULL, 0);
	memset(&duty->tick[func], 0, sizeof(_av_debug_tick));
	duty->tick[duty->func].begin = tick;
#ifdef AV_GXPLAYER_DEBUG
	memset(&duty->timer, 0, sizeof(_av_debug_timer));
	duty->timer.begin = tick;
#endif

	return;
}

void av_debug_end_duty(void *handle)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);

	tick = av_get_tick(NULL, 0);
	duty->tick[duty->func].end = tick;
#ifdef AV_GXPLAYER_DEBUG
	duty->timer.end = tick;
#endif

	return;
}

void av_debug_first_avframe_duty(void *handle)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->avframe = 1;

	return;
}

void av_debug_frist_audio_frame_duty(void *handle, unsigned int frame_cnt)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	tick = av_get_tick(NULL, 0);
	if (duty->tick[duty->func].audio_frame == 0) {
		duty->tick[duty->func].audio_frame = tick;
	}
#ifdef AV_GXPLAYER_DEBUG
	if (duty->timer.first_audio_frame == 0) {
		duty->timer.first_audio_frame     = tick;
		duty->timer.first_audio_frame_cnt = frame_cnt;
	}
#endif

	return;
}

void av_debug_frist_video_frame_duty(void *handle, unsigned int frame_cnt)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	tick = av_get_tick(NULL, 0);
	if (duty->tick[duty->func].video_frame == 0) {
		duty->tick[duty->func].video_frame = tick;
	}
#ifdef AV_GXPLAYER_DEBUG
	if (duty->timer.first_video_frame == 0) {
		duty->timer.first_video_frame     = tick;
		duty->timer.first_video_frame_cnt = frame_cnt;
	}
#endif
	return;
}

void av_debug_frist_video_show_duty(void *handle)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	tick = av_get_tick(NULL, 0);
	if (duty->tick[duty->func].video_show == 0) {
		duty->tick[duty->func].video_show = tick;
	}
#ifdef AV_GXPLAYER_DEBUG
	if (duty->timer.first_video_show == 0) {
		duty->timer.first_video_show = tick;
	}
#endif
	return;
}

void av_debug_start_video_decode_duty(void *handle)
{
	_av_debug *duty = (_av_debug *)handle;
	unsigned int tick = 0;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	tick = av_get_tick(NULL, 0);
	if (duty->tick[duty->func].video_decode == 0) {
		duty->tick[duty->func].video_decode = tick;
	}
#ifdef AV_GXPLAYER_DEBUG
	if (duty->timer.start_video_decode == 0) {
		duty->timer.start_video_decode = tick;
	}
#endif
	return;
}

void av_debug_get_costtime(void *handle, void *costtime)
{
	_av_debug *duty = (_av_debug *)handle;
	PlayerProgCostTime *costms = (PlayerProgCostTime *)costtime;
	unsigned int play_audio_ms = 0, play_video_ms = 0;
	unsigned int seek_audio_ms = 0, seek_video_ms = 0;

	CHECK_AV_TIME_NULL(duty);
	CHECK_AV_TIME_NULL(costtime);

	play_audio_ms = duty->tick[AV_TICK_FUNC_PLAY].audio_frame - duty->tick[AV_TICK_FUNC_PLAY].begin;
	play_video_ms = duty->tick[AV_TICK_FUNC_PLAY].video_show  - duty->tick[AV_TICK_FUNC_PLAY].begin;
	seek_audio_ms = duty->tick[AV_TICK_FUNC_SEEK].audio_frame - duty->tick[AV_TICK_FUNC_SEEK].begin;
	seek_video_ms = duty->tick[AV_TICK_FUNC_SEEK].video_show  - duty->tick[AV_TICK_FUNC_SEEK].begin;
	costms->play_cost_ms = (play_video_ms == 0) ? play_audio_ms : play_video_ms;
	costms->seek_cost_ms = (seek_video_ms == 0) ? seek_audio_ms : seek_video_ms;
}

#ifdef AV_GXPLAYER_DEBUG

void av_debug_srcurl_duty(void *handle, const char *player_srcurl)
{
	_av_debug *duty = (_av_debug *)handle;

	duty->player_srcurl = player_srcurl;
}

void av_debug_dsturl_duty(void *handle, const char *player_dsturl)
{
	_av_debug *duty = (_av_debug *)handle;

	duty->player_dsturl = player_dsturl;
}

void av_debug_first_audio_es_duty(void *handle, unsigned int size)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	if (duty->timer.first_audio_es == 0) {
		duty->timer.first_audio_es      = av_get_tick(NULL, 0);
		duty->timer.first_audio_es_size = size;
	}
	return;
}

void av_debug_first_video_es_duty(void *handle, unsigned int size)
{
	unsigned int cnt = 0;
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);
	if (!duty->avframe)
		return;

	if (duty->timer.first_video_es == 0) {
		duty->timer.first_video_es      = av_get_tick(NULL, 0);
		duty->timer.first_video_es_size = size;
	}

	return;
}

void av_debug_player_prep_duty(void *handle, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.player_prep[st] = av_get_tick(NULL, 0);
	return;

}

void av_debug_player_zoom_duty(void *handle, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.player_zoom[st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_player_freeze_duty(void *handle, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.player_freeze[st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_media_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.media[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_stream_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.stream[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_demuxer_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.demuxer[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_audio_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.audio[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_subt_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.subt[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_alive_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.alive[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_video_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st)
{
	_av_debug *duty = (_av_debug *)handle;

	CHECK_AV_TIME_NULL(duty);

	duty->timer.video[op][st] = av_get_tick(NULL, 0);
	return;
}

void av_debug_media_mod_begin(void *handle, void *priv, int type)
{
	_av_debug *duty = (_av_debug *)handle;
	_av_debug_media *media = NULL;

	CHECK_AV_TIME_NULL(duty);

	media = __av_debug_media(duty, priv);
	if (media)
		media->type = type;
}

void av_debug_media_mod_end(void *handle, void *priv)
{
	_av_debug *duty = (_av_debug *)handle;
	_av_debug_media *media = NULL;

	CHECK_AV_TIME_NULL(duty);

	media = __av_debug_media(duty, priv);
	if (media) {
		memset(media, 0, sizeof(__av_debug_media));
	}
}

void av_debug_media_mod_duty(void *handle, void *priv, AV_MEDIA_MOD mod, const char *name_str)
{
	_av_debug *duty = (_av_debug *)handle;
	_av_debug_media *media = NULL;

	CHECK_AV_TIME_NULL(duty);

	media = __av_debug_media(duty, priv);
	if (media)
		media->str[mod] = name_str;
}

void av_debug_media_printf(void)
{
	unsigned int i = 0, j = 0, k = 0;

	gxlogi_raw("==================== Player Media Module  ====================\n");
	for (i = 0; i < sizeof(_duty) /sizeof(_duty[0]); i++) {
		_av_debug *duty = _duty[i];
		if (duty != NULL) {
			gxlogi_raw("===========================================================\n");
			gxlogi_raw("Player   Name: %s\n", duty->player_name);
			gxlogi_raw("Player Srcurl: %s\n", duty->player_srcurl);
			gxlogi_raw("Player Dsturl: %s\n", duty->player_dsturl);
			for (j = 0; j < AV_DEBUG_MEDIA_MAX; j++) {
				_av_debug_media *media = &duty->media[j];

				if (media->priv) {
					gxlogi_raw("%s: %s\n", _media_type,  _media_str[media->type]);
					for (k = 0; k < AV_MEDIA_MAX_MOD; k++) {
						if (media->str[k]) {
							gxlogi_raw("%s: %s\n", _media[k], media->str[k]);
						}
					}
				}
			}
		}
	}
	gxlogi_raw("==============================================================\n");
}

static void __av_debug_play_printf(_av_debug *duty)
{
	gxlogi_raw("       total [play] func:         %-4d ms\n", duty->timer.end - duty->timer.begin);
	gxlogi_raw("             player prep:         %-4d ms\n",   __cost_time(duty->timer.player_prep));
	gxlogi_raw("             player zoom:         %-4d ms\n",   __cost_time(duty->timer.player_zoom));
	gxlogi_raw("             media  stop:         %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_STOP]));
	gxlogi_raw("                    stream:       %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_STOP]));
	gxlogi_raw("                    demuxer:      %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_STOP]));
	gxlogi_raw("                    video:        %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_STOP]));
	gxlogi_raw("                    audio:        %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_STOP]));
	gxlogi_raw("                    subtitle:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_STOP]));
	gxlogi_raw("                    alive:        %-4d ms\n",   __cost_time(duty->timer.alive[AV_TICK_OP_STOP]));
	gxlogi_raw("             media  open:         %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_OPEN]));
	gxlogi_raw("                    stream:       %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_OPEN]));
	gxlogi_raw("                    demuxer:      %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_OPEN]));
	gxlogi_raw("                    video:        %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_OPEN]));
	gxlogi_raw("                    audio:        %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_OPEN]));
	gxlogi_raw("                    subtitle:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_OPEN]));
	gxlogi_raw("             media  config:       %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    stream:       %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    demuxer:      %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    video:        %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    audio:        %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    subtitle:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_CONFIG]));
	gxlogi_raw("             media  run:          %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_RUN]));
	gxlogi_raw("                    stream:       %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_RUN]));
	gxlogi_raw("                    demuxer:      %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_RUN]));
	gxlogi_raw("                    video:        %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_RUN]));
	gxlogi_raw("                    audio:        %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_RUN]));
	gxlogi_raw("                    subtitle:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_RUN]));
	if (duty->timer.first_audio_es != 0) {
		gxlogi_raw("       audio first  data:(%6d) %-4d ms\n",
				duty->timer.first_audio_es_size, duty->timer.first_audio_es - duty->timer.begin);
	}
	if (duty->timer.first_audio_frame != 0) {
		gxlogi_raw("       audio frame  decode:(%4d) %-4d ms\n",
				duty->timer.first_audio_frame_cnt, (duty->timer.first_audio_frame - duty->timer.begin));
	}
	if (duty->timer.first_video_es != 0) {
		gxlogi_raw("       video first  data:(%6d) %-4d ms\n",
				duty->timer.first_video_es_size, duty->timer.first_video_es - duty->timer.begin);
	}
	if (duty->timer.first_video_frame != 0) {
		gxlogi_raw("       video frame  decode:(%4d) %-4d ms\n",
				duty->timer.first_video_frame_cnt, (duty->timer.first_video_frame - duty->timer.begin));
	}
	if (duty->timer.start_video_decode != 0) {
		gxlogi_raw("       video start  decode:       %-4d ms\n",
				duty->timer.start_video_decode - duty->timer.begin);
	}
	if (duty->timer.first_video_show != 0) {
		gxlogi_raw("       video first  show:         %-4d ms\n",
				duty->timer.first_video_show - duty->timer.begin);
	}
}

static void __av_debug_seek_printf(_av_debug *duty)
{
	gxlogi_raw("       total [seek] func:             %-4d ms\n", duty->timer.end - duty->timer.begin);
	gxlogi_raw("             media  seek pause:       %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_SEEK_PAUSE]));
	gxlogi_raw("                    demuxer  seek:    %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_SEEK]));
	gxlogi_raw("                    video    stop:    %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_STOP]));
	gxlogi_raw("                    audio    stop:    %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_STOP]));
	gxlogi_raw("                    subtitle stop:    %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_STOP]));
	gxlogi_raw("                    video    config:  %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    audio    config:  %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_CONFIG]));
	gxlogi_raw("                    subtitle config:  %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_CONFIG]));
	gxlogi_raw("             media  seek resume:      %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_SEEK_RESUME]));
	gxlogi_raw("                    stream   resume:  %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_RUN]));
	gxlogi_raw("                    demuxer  resume:  %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_RUN]));
	gxlogi_raw("                    video    run:     %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_RUN]));
	gxlogi_raw("                    audio    run:     %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_RUN]));
	gxlogi_raw("                    subtitle run:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_RUN]));
	if (duty->timer.first_audio_es != 0) {
		gxlogi_raw("       audio first  data:(%10d) %-4d ms\n",
				duty->timer.first_audio_es_size, duty->timer.first_audio_es - duty->timer.begin);
	}
	if (duty->timer.first_audio_frame != 0) {
		gxlogi_raw("       audio frame  decode:(%8d) %-4d ms\n",
				duty->timer.first_audio_frame_cnt, (duty->timer.first_audio_frame - duty->timer.begin));
	}
	if (duty->timer.first_video_es != 0) {
		gxlogi_raw("       video first  data:(%10d) %-4d ms\n",
				duty->timer.first_video_es_size, duty->timer.first_video_es - duty->timer.begin);
	}
	if (duty->timer.first_video_frame != 0) {
		gxlogi_raw("       video frame  decode:(%8d) %-4d ms\n",
				duty->timer.first_video_frame_cnt, (duty->timer.first_video_frame - duty->timer.begin));
	}
	if (duty->timer.start_video_decode != 0) {
		gxlogi_raw("       video start  decode:           %-4d ms\n",
				duty->timer.start_video_decode - duty->timer.begin);
	}
	if (duty->timer.first_video_show != 0) {
		gxlogi_raw("       video first  show:             %-4d ms\n",
				duty->timer.first_video_show - duty->timer.begin);
	}

}

static void __av_debug_stop_printf(_av_debug *duty)
{
	gxlogi_raw("       total [stop] func:         %-4d ms\n", duty->timer.end - duty->timer.begin);
	gxlogi_raw("             media  stop:         %-4d ms\n",   __cost_time(duty->timer.media[AV_TICK_OP_STOP]));
	gxlogi_raw("                    stream:       %-4d ms\n",  __cost_time(duty->timer.stream[AV_TICK_OP_STOP]));
	gxlogi_raw("                    demuxer:      %-4d ms\n", __cost_time(duty->timer.demuxer[AV_TICK_OP_STOP]));
	gxlogi_raw("                    video:        %-4d ms\n",   __cost_time(duty->timer.video[AV_TICK_OP_STOP]));
	gxlogi_raw("                    audio:        %-4d ms\n",   __cost_time(duty->timer.audio[AV_TICK_OP_STOP]));
	gxlogi_raw("                    subtitle:     %-4d ms\n",    __cost_time(duty->timer.subt[AV_TICK_OP_STOP]));
	gxlogi_raw("                    alive:        %-4d ms\n",   __cost_time(duty->timer.alive[AV_TICK_OP_STOP]));
	gxlogi_raw("             player freeze:       %-4d ms\n",   __cost_time(duty->timer.player_freeze));
}

void av_debug_costtime_printf(void)
{
	unsigned int i = 0;

	gxlogi_raw("==================== Player Cost Time  ====================\n");
	for (i = 0; i < sizeof(_duty) /sizeof(_duty[0]); i++) {
		_av_debug *duty = _duty[i];
		if (duty != NULL) {
			gxlogi_raw("===========================================================\n");
			gxlogi_raw("Player   Name: %s\n", duty->player_name);
			gxlogi_raw("Player Srcurl: %s\n", duty->player_srcurl);
			gxlogi_raw("Player Dsturl: %s\n", duty->player_dsturl);
			if (duty->func == AV_TICK_FUNC_PLAY)
				__av_debug_play_printf(duty);
			else if (duty->func == AV_TICK_FUNC_SEEK)
				__av_debug_seek_printf(duty);
			else if (duty->func == AV_TICK_FUNC_STOP)
				__av_debug_stop_printf(duty);
		}
	}
	gxlogi_raw("===========================================================\n");

	return;
}

#endif

