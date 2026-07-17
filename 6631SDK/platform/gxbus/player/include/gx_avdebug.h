#ifndef __AV_DEBUG_H__
#define __AV_DEBUG_H__

typedef enum {
	AV_TICK_FUNC_PLAY,
	AV_TICK_FUNC_SEEK,
	AV_TICK_FUNC_STOP,
	AV_TICK_FUNC_MAX
} AV_TICK_FUNC;

typedef enum {
	AV_TICK_OP_OPEN = 0,
	AV_TICK_OP_CONFIG,
	AV_TICK_OP_RUN,
	AV_TICK_OP_STOP,
	AV_TICK_OP_SEEK,
	AV_TICK_OP_SEEK_PAUSE,
	AV_TICK_OP_SEEK_RESUME,
	AV_TICK_OP_MAX
} AV_TICK_OP;

typedef enum {
	AV_TICK_ST_BEGIN = 0,
	AV_TICK_ST_END,
	AV_TICK_ST_MAX
} AV_TICK_ST;

typedef enum {
	AV_MEDIA_STREAM  = 0,
	AV_MEDIA_DEMUXER,
	AV_MEDIA_IFROMAT,
	AV_MEDIA_OFROMAT,
	AV_MEDIA_AUDIO_DECODER_0,
	AV_MEDIA_AUDIO_CODEC_0,
	AV_MEDIA_AUDIO_DECODER_1,
	AV_MEDIA_AUDIO_CODEC_1,
	AV_MEDIA_VIDEO_DECODER,
	AV_MEDIA_VIDEO_CODEC,
	AV_MEDIA_SUBTITLE_DECODER,
	AV_MEDIA_AUDIO_PLAY,
	AV_MEDIA_VIDEO_PLAY,
	AV_MEDIA_SUBTITLE_PLAY,
	AV_MEDIA_DUMPER,
	AV_MEDIA_MAX_MOD
} AV_MEDIA_MOD;

void *av_debug_create                 (const char *player_name);
void  av_debug_destory                (void *handle);
void  av_debug_begin_duty             (void *handle, AV_TICK_FUNC func);
void  av_debug_end_duty               (void *handle);
void  av_debug_frist_audio_frame_duty (void *handle, unsigned int frame_cnt);
void  av_debug_frist_video_frame_duty (void *handle, unsigned int frame_cnt);
void  av_debug_frist_video_show_duty  (void *handle);
void  av_debug_start_video_decode_duty(void *handle);
void  av_debug_first_avframe_duty     (void *handle);
void  av_debug_get_costtime           (void *handle, void *costtime);

#ifdef AV_GXPLAYER_DEBUG

void  av_debug_first_audio_es_duty   (void *handle, unsigned int size);
void  av_debug_first_video_es_duty   (void *handle, unsigned int size);
void  av_debug_player_prep_duty(void *handle, AV_TICK_ST st);
void  av_debug_player_zoom_duty(void *handle, AV_TICK_ST st);
void  av_debug_player_freeze_duty(void *handle, AV_TICK_ST st);
void  av_debug_media_duty  (void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_stream_duty (void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_demuxer_duty(void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_audio_duty  (void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_video_duty  (void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_subt_duty   (void *handle, AV_TICK_OP op, AV_TICK_ST st);
void  av_debug_alive_duty  (void *handle, AV_TICK_OP op, AV_TICK_ST st);

void  av_debug_srcurl_duty(void *handle, const char *player_srcurl);
void  av_debug_dsturl_duty(void *handle, const char *player_dsturl);

void  av_debug_media_mod_begin(void *handle, void *priv, int type);
void  av_debug_media_mod_end  (void *handle, void *priv);
void  av_debug_media_mod_duty (void *handle, void *priv, AV_MEDIA_MOD mod, const char *name_str);

void  av_debug_costtime_printf(void);
void  av_debug_media_printf   (void);
#else

#define av_debug_first_audio_es_duty(handle, size)   ((void)0)
#define av_debug_first_video_es_duty(handle, size)   ((void)0)
#define av_debug_player_prep_duty(handle, st)        ((void)0)
#define av_debug_player_zoom_duty(handle, st)        ((void)0)
#define av_debug_player_freeze_duty(handle, st)      ((void)0)
#define av_debug_media_duty(  handle, op, st)        ((void)0)
#define av_debug_stream_duty( handle, op, st)        ((void)0)
#define av_debug_demuxer_duty(handle, op, st)        ((void)0)
#define av_debug_audio_duty(  handle, op, st)        ((void)0)
#define av_debug_video_duty(  handle, op, st)        ((void)0)
#define av_debug_subt_duty(   handle, op, st)        ((void)0)
#define av_debug_alive_duty(  handle, op, st)        ((void)0)

#define av_debug_srcurl_duty(handle, srcurl)         ((void)0)
#define av_debug_dsturl_duty(handle, dsturl)         ((void)0)

#define av_debug_media_mod_begin(handle, priv, type) ((void)0)
#define av_debug_media_mod_end(  handle, priv)       ((void)0)
#define av_debug_media_mod_duty( handle, priv, m, s) ((void)0)

#define av_debug_costtime_printf()                   ((void)0)
#define av_debug_media_printf()                      ((void)0)
#endif

#endif
