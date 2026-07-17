#ifndef __MEDIA_H__
#define __MEDIA_H__

#include "stheader.h"
#include "gx_decoder.h"
#include "gx_avout.h"
#include "gx_msg.h"
#include "gx_dumpfilter.h"
#include "gx_cachefilter.h"
#include "gx_muxer.h"
#include "gx_id3.h"
#include "gx_system.h"

#define MEDIA_HAS_VIDEO(media)  (media && media->sh_video  && media->sh_video->format!=PLAYER_VCODEC_DUMMY)
#define MEDIA_HAS_AUDIO(media)  (media && media->sh_audio  && media->sh_audio->format!=PLAYER_ACODEC_DUMMY)
#define MEDIA_HAS_AUDIO1(media) (media && media->sh_audio1 && media->sh_audio1->format!=PLAYER_ACODEC_DUMMY)
#define MEDIA_HAS_SUB(media)    (media && media->sh_sub)

#define MEDIA_IS_DVB(media)          (media && media->stream && \
		((media->stream->file_format == GX_STREAMTYPE_DVB) || (media->stream->file_format == GX_STREAMTYPE_FM)))

#define MEDIA_IS_HWTS(media)         (media && media->demuxer && \
		(media->demuxer->type == GX_DEMUXER_TYPE_HW_TS))

#define MEDIA_IS_TS(media)         (media && media->demuxer && \
		(media->demuxer->type == GX_DEMUXER_TYPE_HW_TS || media->demuxer->type == GX_DEMUXER_TYPE_MPEG_TS))

#define MEDIA_IS_NETSTREAM(media)    (media && media->stream && \
		(media->stream->file_format == GX_STREAMTYPE_STREAM))

#define MEDIA_IS_NETDEMUXER(media)   (media && media->stream && \
		(media->stream->file_format == GX_STREAMTYPE_DEMUXER))

#define MEDIA_IS_NOT_FILLPACKET(media) (media && media->demuxer && \
		(media->demuxer->fill_pkt_speed == 0))

#define MEDIA_IS_LOGO(media)   (media && media->stream && media->demuxer && \
		(media->demuxer->type == GX_DEMUXER_TYPE_MPEG_ES))

#define URL_IS_LOGO(url)        ((url) && (strlen(url) >= 4) && \
		((strncasecmp((char*)((url)+strlen(url)-4), ".bin", 4) == 0) ||\
		 (strncasecmp((char*)((url)+strlen(url)-4), ".esv", 4) == 0) ||\
		 (strncasecmp((char*)((url)+strlen(url)-4), ".m2v", 4) == 0)))
#define URL_IS_DVB(url)         ((url) && ((strncasecmp("dvb", (url), 3) == 0) ||(strncasecmp("dtmb", (url), 4) == 0)))

#define URL_IS_FM(url)          ((url) && (strncasecmp("fm", (url), 2) == 0))

#define MEDIA_NO_RESTART_STREAM(media) (media && media->stream && media->stream->no_restart_stream)

#define MEDIA_DUMP_EOF(media)  (media && media->dumper && media->dumper->eof)

#define MEDIA_FB_EOF(media)  (media && media->demuxer && media->demuxer->fbeof == 1)

#define AUDIO_NO_ERROR(media) (media && media->demuxer && media->demuxer->audio && media->demuxer->audio->dropmode == DROPMODE_NONE)
#define VIDEO_NO_ERROR(media) (media && media->demuxer && media->demuxer->video && media->demuxer->video->dropmode == DROPMODE_NONE)
#define AUDIO_IS_UNSUPPORT(media) (media && media->demuxer && media->demuxer->audio && media->demuxer->audio->dropmode == DROPMODE_UNSUPPORT)
#define VIDEO_IS_UNSUPPORT(media) (media && media->demuxer && media->demuxer->video && media->demuxer->video->dropmode == DROPMODE_UNSUPPORT)

#define IS_UNRECOVERABLE_ERROR(state, err_code) \
	((state==AVCODEC_ERROR) && (err_code==AVCODEC_ERR_CODECTYPE_UNSUPPORT))
#define IS_RECOVERABLE_ERROR(state, err_code) \
	((state==AVCODEC_ERROR) && (err_code!=AVCODEC_ERR_CODECTYPE_UNSUPPORT))

typedef enum {
	GX_MEDIA_PLAYER,
	GX_MEDIA_RECORDER,
	GX_MEDIA_PROBER,
	GX_MEDIA_PCMDUMPER,
}GxMediaType;

typedef enum {
	GX_MEDIA_RUN_PLAY   = (1<<0),
	GX_MEDIA_RUN_RECORD = (1<<1)
}GxMediaRunFlag;

typedef enum {
	GX_MEDIA_STATE_IDLE,
	GX_MEDIA_STATE_FREEZED_IDLE,
	GX_MEDIA_STATE_INITING,
	GX_MEDIA_STATE_READY,
	GX_MEDIA_STATE_RUNNING,
	GX_MEDIA_STATE_SOON_OVER,
	GX_MEDIA_STATE_OVER,
}GxMediaState;

typedef struct {
	void*       priv;
	void        (*func)(void* priv, PlayerStatus status, PlayerError error);
} GxMediaEvent;

#define MEDIA_MAX_CLONE (1)

typedef struct  {
	int                    sub;
	GxMediaType            type;

	GxStream*              stream;
	GxDemuxer*             demuxer;
	GxMuxer*               muxer;
	GxAudioCodec*          acodec;
	GxAudioCodec*          acodec1;
	GxVideoCodec*          vcodec;
	GxSubCodec*            scodec;
	GxAudioOut*            aout;
	GxVideoOut*            vout;
	GxSubOut*              sout;
	GxDumpFilter*          dumper;

	GxStreamAudioHeader*   sh_audio;
	GxStreamAudioHeader*   sh_audio1;
	GxStreamVideoHeader*   sh_video;
	GxStreamSubHeader*     sh_sub;

	int                    rtimeout;
	int                    buffering;
	int                    buffering_pause;

	int                    errInfo;
	int                    holdPause;

	int32_t                mutex;
	int                    state;
	int                    cloned;
	GxMediaEvent           event[MEDIA_MAX_CLONE+1];

	int                    alive;
	void*                  priv;
	int                    speed;
	int                    seeking;
	int                    errorstop;

	int                    audio_saved;
	int                    video_saved;
	int                    audio_showing;
	int                    video_showing;
	int                    cur_codec_speed_factor;
	GxMediaRunFlag         runflag;
	struct {
		unsigned long long last_frame_cnt;
		GxTime             last_frame_time;
	}aframe, vframe, packet;
	GxDemuxerRecordExtInfo recconfig;
	void*                  debug;
	GxTime                 eoftime;
} GxMedia;

typedef struct {
	union {
		struct {
			int            sub;
			const char*    url;
			uint64_t       start;
			int            bandwidth;
			GxDemuxerOpenInfo demuxInfo;
			GxStream*      stream;

			GxMediaEvent   event;
		}player;

		struct {
			int            sub;
			const char*    srcurl;
			const char*    dsturl;

			GxMediaEvent   event;
		}recorder;

		struct {
			const char*    url;
		}prober;

		struct {
			const char*    url;
		}pcmdumper;
	};
	void*          debug;
}GxMediaPara;

GxMedia* GxMedia_New(GxMediaType type, GxMediaPara* para);

GxMedia* GxMedia_Restart(GxMedia* media, GxMediaPara* para);

void GxMedia_Destroy(GxMedia *media, int freeze);

void GxMedia_Stop(GxMedia *media, int freeze, int alive);

int  GxMedia_Config(GxMedia *media);

int  GxMedia_Run(GxMedia *media);

void GxMedia_Pause(GxMedia *media);

void GxMedia_Resume(GxMedia *media);

void GxMedia_Abort(GxMedia* media);

int GxMedia_AudioSync(GxMedia* media, int timems);

int GxMedia_AudioDelay(GxMedia* media, int delayms);

int GxMedia_VideoDelay(GxMedia* media, int delayms);

int  GxMedia_AudioSwitch(GxMedia* media,int pid, AudioCodecType codectype);

int  GxMedia_SubSync(GxMedia* media, int timems);

int  GxMedia_SubSwitch(GxMedia* media,int pid);

int  GxMedia_SetSpeed(GxMedia* media, int speed);

int  GxMedia_SeekPause(GxMedia* media, int64_t time, int flag);
int  GxMedia_SeekResume(GxMedia* media);

void GxMedia_FlushFrameInfo(GxMedia* media);

void GxMedia_GetCodecStatus(GxMedia* media,
		AVCodecStatus *acodec, AVCodecStatus *vcodec,
		AVCodecSyncStatus *async, AVCodecSyncStatus *vsync);

void GxMedia_GetDataLength(GxMedia* media, unsigned int * adata, unsigned int * vdata);

void GxMedia_SwitchUrl(GxMedia* media, const char* url);

status_t GxMedia_GetCurrentTime(GxMedia* media, uint64_t *cur);

uint64_t GxMedia_GetSeekMinTime(GxMedia* media);

int32_t GxMedia_GetCacheTime(GxMedia* media);

int32_t GxMedia_GetCacheInfo(GxMedia *media, PlayerNetInfo* info);

uint8_t GxMedia_GetCurrentPercent(GxMedia* media);

uint64_t GxMedia_GetDuration(GxMedia* media);

void GxMedia_AVPrintf(GxMedia* media);
void GxMedia_DetectVideoPtsDiff(GxMedia* media);

void GxMedia_GetTSRecordInfo(GxMedia *media, PlayerRecordConfig* config);

int GxMedia_TrackAdd(GxMedia* media, GxStreamTrackAdd* ptrack);

int GxMedia_TrackDel(GxMedia* media, GxStreamTrackDel* ptrack);

void GxMedia_SetPlayTsFilterConfig(GxMedia* media, GxDemuxerPlayTsExtInfo* config);

void GxMedia_SetPlayTsCacheConfig(GxMedia* media, GxStreamTrackCache* config);

void GxMedia_SetRecordConfig(GxMedia* media, PlayerRecordConfig* config, PlayerRecordEncrypt *encrypt);

void GxMedia_SetRecordUserInfo(GxMedia* media, GxDemuxerRecordUserInfo* user_info);

void GxMedia_GetSyncInfo(GxMedia* media, PlayerSyncInfo* syncinfo);

unsigned int GxMedia_RecordCheckOverflow(GxMedia* media);

unsigned int GxMedia_GetTSProgStreamInfo(GxMedia* media, PlayerProgStreamInfo *info);

unsigned int GxMedia_VideoStartDecord(GxMedia* media);

unsigned int GxMedia_VideoCheckDynamicRangeChange(GxMedia* media);

unsigned int GxMedia_VideoStartDisplay(GxMedia* media);

unsigned int GxMedia_VideoReset(GxMedia* media, int freeze);

unsigned int GxMedia_AudioReset(GxMedia* media);

unsigned int GxMedia_AudioReset1(GxMedia* media);

unsigned int GxMedia_Reset(GxMedia* media, int freeze);

unsigned int GxMedia_VideoUnsupport(GxMedia* media);

unsigned int GxMedia_VideoUnsupport_Really(GxMedia* media);

unsigned int GxMedia_AudioUnsupport(GxMedia* media);

int32_t GxMedia_GetCurrentPts(GxMedia* media);

status_t GxMedia_AdAudioEnable(GxMedia* media, int pid, AudioCodecType codectype);

status_t GxMedia_AdAudioMix(GxMedia* media, int mix);

status_t GxMedia_Save(GxMedia* media, PlayerMediaContent content);

status_t GxMedia_Restore(GxMedia* media, PlayerMediaContent content);

status_t GxMedia_StartRecord(GxMedia* media, const char* dtsurl);

status_t GxMedia_StopRecord(GxMedia* media);

status_t GxMedia_StartPlay(GxMedia* media);

status_t GxMedia_StopPlay(GxMedia* media, int freeze);

int GxMedia_Read(GxMedia *media, PlayerReadType type, uint8_t* buffer, int len);

status_t GxMedia_SeamlessBandwidthSwitch(GxMedia* media, unsigned int bandwidth);

int GxMedia_SetPVRControl(GxMedia *media, GxRecordPVRControl *ctrl);

int GxMedia_GetPVRControl(GxMedia *media, GxRecordPVRControl *ctrl);

#endif

