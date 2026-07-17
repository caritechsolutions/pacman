#include "gx_media.h"
#include "gx_device.h"
#include "gxplayer_module.h"
#include "gx_common.h"
#include "../avout/ao.h"
#include "../avout/vo.h"
#include "../avout/so.h"

enum
{
	MEDIA_STREAM_VIDEO,
	MEDIA_STREAM_AUDIO
};

#define MEDIA_EXEC_EVENT(pMedia,eventid,errorid) do{\
	if (1)\
	{\
		int i;\
		for(i=0; i<MEDIA_MAX_CLONE+1;i++)\
		{\
			if (pMedia->event[i].func)\
			{\
				pMedia->event[i].func(pMedia->event[i].priv, eventid, errorid);\
			}\
		}\
	}\
}while(0);

#define MEDIA_CHECK_RET(media,ret,error) do{\
	if (ret < 0)\
	{\
		MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, error);\
		return ret;\
	}\
}while(0)

static void _media_fill_buf(GxMedia* pMedia, int fill_flag)
{
	if (!pMedia->buffering_pause) {
		if (fill_flag)
			MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_START_FILL_BUF, PLAYER_ERROR_NO_ERROR);
		pMedia->buffering  = 1;
		pMedia->buffering_pause  = 1;
		GxMedia_Pause(pMedia);
	}
	return;
}

static void _media_end_buf(GxMedia* pMedia, int fill_flag)
{
	if (pMedia->buffering_pause) {
		pMedia->buffering  = 0;
		GxMedia_Resume(pMedia);
		pMedia->buffering_pause  = 0;
		if (fill_flag)
			MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_END_FILL_BUF, PLAYER_ERROR_NO_ERROR);
	}
	return;
}

uint64_t media_get_current_cb(void* args)
{
	GxMedia* media = args;
	uint64_t cur = 0;

	if (GxMedia_GetCurrentTime(media, &cur) != GX_PLAYER_OK)
		return 0;

	return cur;
}

static void media_filter_event(void* priv, void* arg)
{
	if (priv && arg)
	{
		GxMedia* pMedia = priv;
		GxMediaFilterEventPara* pPara = arg;
		AVCodecStatus CodecStatus;

		switch(pPara->type)
		{
		case GX_MFT_EVENT_PLAY_END:
			{
				if (pMedia->state == GX_MEDIA_STATE_RUNNING) {
					if (GxCore_MutexTrylock(pMedia->mutex) != GXCORE_SUCCESS) {
						return;
					}
					if (MEDIA_HAS_VIDEO(pMedia) && !pMedia->demuxer->video->dropmode) {
						GxVideoDecoder_Control(pMedia->vcodec,GX_VD_CTRL_UPDATE,NULL);
						if (pMedia->demuxer->type == GX_DEMUXER_TYPE_MPEG_ES || pMedia->demuxer->type == GX_DEMUXER_TYPE_ES)
							GxVideoDecoder_Control(pMedia->vcodec,GX_VD_CTRL_REFRESH,NULL);
					}

					if (MEDIA_HAS_AUDIO(pMedia)) {
						GxAudioDecoder_Control(pMedia->acodec,GX_AD_CTRL_UPDATE,NULL);
					}

					if (MEDIA_HAS_AUDIO1(pMedia)) {
						GxAudioDecoder_Control(pMedia->acodec1,GX_AD_CTRL_UPDATE,NULL);
					}
					pMedia->state = GX_MEDIA_STATE_SOON_OVER;
					GxCore_GetTickTime(&pMedia->eoftime);
					GxCore_MutexUnlock(pMedia->mutex);
				} else if (pMedia->state == GX_MEDIA_STATE_SOON_OVER) {
					if (GxCore_MutexTrylock(pMedia->mutex) != GXCORE_SUCCESS) {
						return;
					}

					if (MEDIA_HAS_AUDIO(pMedia) && !pMedia->demuxer->audio->dropmode) {
						GxAudioDecoder_Control(pMedia->acodec, GX_AD_CTRL_GET_STATUS, &CodecStatus);
						if (CodecStatus.state != AVCODEC_FINISHED &&
								CodecStatus.state != AVCODEC_ERROR &&
								CodecStatus.state != AVCODEC_STOPPED) {
							GxTime curtime;
							GxCore_MutexUnlock(pMedia->mutex);
							GxCore_GetTickTime(&curtime);
							if(TIME_MINUS(curtime, pMedia->eoftime) >= 30000)
								gxloge("%s(%d), acodec error, status = 0x%x\n", __func__, __LINE__, CodecStatus.state);
							return;
						} else if (MEDIA_HAS_VIDEO(pMedia) && !pMedia->demuxer->video->dropmode) {
							GxStreamVideoHeader* sh = pMedia->demuxer->video->sh;
							sh->priv.pts_sync = 0;
							GxVideoDecoder_Control(pMedia->vcodec, GX_VD_CTRL_SET_SYNC, &sh->priv.pts_sync);
						}
						GxAudioOut_Control(pMedia->aout, GX_AO_CTRL_DRAIN_WAIT, NULL);
					}

					if (MEDIA_HAS_VIDEO(pMedia) && !pMedia->demuxer->video->dropmode) {
						GxVideoDecoder_Control(pMedia->vcodec, GX_VD_CTRL_GET_STATUS, &CodecStatus);
						if (CodecStatus.state != AVCODEC_FINISHED &&
								CodecStatus.state != AVCODEC_ERROR &&
								CodecStatus.state != AVCODEC_STOPPED) {
							GxTime curtime;
							GxCore_MutexUnlock(pMedia->mutex);
							GxCore_GetTickTime(&curtime);
							if(TIME_MINUS(curtime, pMedia->eoftime) >= 10000)
								gxloge("%s(%d), vcodec error, status = 0x%x\n", __func__, __LINE__, CodecStatus.state);
							return;
						} else if (CodecStatus.state == AVCODEC_READY) {
							GxTime curtime;
							GxCore_GetTickTime(&curtime);
							int costms = TIME_MINUS(curtime, pMedia->eoftime);
							if (abs(costms) <= 2000) {
								GxCore_MutexUnlock(pMedia->mutex);
								return;
							}
						}
					}

					if (pMedia->demuxer->info.concat_eof == 1) {
						/*only concat login. other login else..*/
						GxMedia_VideoReset(pMedia, 1);
						GxMedia_AudioReset(pMedia);
						GxDemuxer_Control(pMedia->demuxer, GX_DEMUXER_CTRL_LAVF_CONTINUE, NULL);

						pMedia->state = GX_MEDIA_STATE_RUNNING;
					} else {
						if (MEDIA_IS_NETSTREAM(pMedia) &&
								(pMedia->stream->err.need_restart || !pMedia->demuxer->seekable)) {
							if (pMedia->demuxer && (pMedia->stream->info.restart_time == 0))
								pMedia->stream->info.restart_time = GxDemuxer_GetCurrentTime(pMedia->demuxer);
							MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_ERROR, PLAYER_ERROR_SOURCE_RESTART);
						} else {
							int error_status = PLAYER_ERROR_NO_ERROR;
							GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
							if (MEDIA_IS_NETDEMUXER(pMedia) && pMedia->stream && (pMedia->stream->err.recv_err > 0 || error_status > 0)) {/*lavf read data fail.*/
								MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_ERROR, ((error_status > 0)?error_status:pMedia->stream->err.recv_err));
							} else {
								MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_PLAY_END, pMedia->errInfo);
							}
							if (error_status > 0)  {
								error_status = -1;
								GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
							}
						}

						pMedia->state = GX_MEDIA_STATE_OVER;
					}
					GxCore_MutexUnlock(pMedia->mutex);
				}
				break;
			}

		case GX_MFT_EVENT_RECORD_END:
			{
				MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_RECORD_END, PLAYER_ERROR_NO_ERROR);
				break;
			}
		case GX_MFT_EVENT_RECORD_FULL:
			{
				MEDIA_EXEC_EVENT(pMedia, PLAYER_STATUS_RECORD_FULL, PLAYER_ERROR_NO_ERROR);
				break;
			}
		case GX_MFT_EVENT_START_FILL_BUF:
			{
				unsigned char empty;
				AVCodecStatus acodec;
				AVCodecStatus vcodec;

				acodec.state = AVCODEC_STOPPED;
				vcodec.state = AVCODEC_STOPPED;
				if (pMedia->state == GX_MEDIA_STATE_RUNNING) {
					GxMedia_GetCodecStatus(pMedia, &acodec, &vcodec, NULL, NULL);
					empty = GxDemuxer_AlmostEmpty(pMedia->demuxer);
					if (empty &&
							(!MEDIA_HAS_AUDIO(pMedia) || acodec.state == AVCODEC_RUNNING || AUDIO_IS_UNSUPPORT(pMedia)) &&
							(!MEDIA_HAS_VIDEO(pMedia) || vcodec.state == AVCODEC_RUNNING || VIDEO_IS_UNSUPPORT(pMedia))) {
						int buffering = 1;
						GxStream_Control(pMedia->stream, GX_STREAM_CTRL_STREAM_BUFFERING, &buffering);
						_media_fill_buf(pMedia, 1);
					}
				}
				break;
			}
		case GX_MFT_EVENT_END_FILL_BUF:
			{
				int buffering = 0;
				GxStream_Control(pMedia->stream, GX_STREAM_CTRL_STREAM_BUFFERING, &buffering);
				_media_end_buf(pMedia, 1);
				break;
			}
		case GX_MFT_EVENT_MEDIA_PAUSE:
			{
				_media_fill_buf(pMedia, 0);
				break;
			}
		case GX_MFT_EVENT_MEDIA_RESUME:
			{
				_media_end_buf(pMedia, 0);
				break;
			}
		default:
			break;
		}
	}
}

int media_switch_audio(GxMedia* media)
{
	if (media->sh_audio) {
		GxAudioDecoder_Control(media->acodec, GX_AD_CTRL_SWITCH_HEADER, media->sh_audio);
		GxAudioOut_Control    (media->aout,   GX_AO_CTRL_SWITCH_HEADER, media->sh_audio);

		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_AO,    media->acodec, GX_AD_PIN_NAME_ESA_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_PCM_OUT,  media->aout,   GX_AO_PIN_NAME_PCM_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AC3_OUT,  media->aout,   GX_AO_PIN_NAME_AC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_EAC3_OUT, media->aout,   GX_AO_PIN_NAME_EAC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_DTS_OUT,  media->aout,   GX_AO_PIN_NAME_DTS_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AAC_OUT,  media->aout,   GX_AO_PIN_NAME_AAC_IN);
	}

	return GX_PLAYER_OK;
}

void GxMedia_Abort(GxMedia* media)
{
	if (media)
	{
		if (MEDIA_HAS_AUDIO(media))
		{
			GxMediaFilter_Abort(media->acodec);
		}
		if (MEDIA_HAS_VIDEO(media))
		{
			GxMediaFilter_Abort(media->vcodec);
		}
		if (MEDIA_HAS_SUB(media))
		{
			GxMediaFilter_Abort(media->scodec);
		}
	}
}

int GxMedia_Config(GxMedia* media)
{
	int ret = GX_PLAYER_ERROR;

	if (media)
	{
		if (media->state == GX_MEDIA_STATE_IDLE || media->state == GX_MEDIA_STATE_FREEZED_IDLE)
		{
			GxPlayer_SystemGet(PSYS_ERROR_STOP, &media->errorstop);

			media->state   = GX_MEDIA_STATE_INITING;
			media->errInfo = PLAYER_ERROR_NO_ERROR;

			if (media->alive == 0) {
				av_debug_stream_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Config(media->stream);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_ACCESS_ERROR);
				av_debug_stream_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);

				av_debug_demuxer_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Config(media->demuxer);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_DEMUX_ERROR);
				av_debug_demuxer_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			media_switch_audio(media);

			if (MEDIA_HAS_AUDIO(media) && media->alive == 0) {
				av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Config(media->acodec);
				if (ret < 0) {
					if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
						return GX_PLAYER_ERROR;
				}
				else {
					if (MEDIA_HAS_AUDIO1(media)) {
						ret = GxMediaFilter_Config(media->acodec1);
						if (ret < 0) {
							if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
								return GX_PLAYER_ERROR;
						}
					}
					ret = GxMediaFilter_Config(media->aout);
					if (ret < 0) {
						if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
							return GX_PLAYER_ERROR;
					}
				}
				av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_VIDEO(media))
			{
				av_debug_video_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Config(media->vcodec);
				if (ret < 0) {
					if (GxMedia_VideoUnsupport(media) != GX_PLAYER_OK)
						return GX_PLAYER_ERROR;
				}
				else {
					ret = GxMediaFilter_Config(media->vout);
					if (ret < 0) {
						if (GxMedia_VideoUnsupport(media) != GX_PLAYER_OK)
							return GX_PLAYER_ERROR;
					}
				}
				av_debug_video_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_SUB(media))
			{
				av_debug_subt_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Config(media->scodec);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_SUB_DECODER_ERROR);

				ret = GxMediaFilter_Config(media->sout);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_SUB_RENDER_ERROR);
				av_debug_subt_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			if (media->muxer) {
				ret = GxMediaFilter_Config(media->muxer);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_MUXER_ERROR);
			}

			if (media->dumper) {
				if (media->demuxer) {
					GxDumpFilterExtra* extra = av_malloc(sizeof(GxDumpFilterExtra));

					if(extra == NULL)
						return GX_PLAYER_ERROR;
					GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_RECORD_HEADER_INFO, &extra->header_info);
					GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_RECORD_USER_INFO,   &extra->user_info);
					GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_RECORD_CTRL_INFO,   &extra->ctrl_info);
					GxDumpFilter_Control(media->dumper, GX_DUMPER_SET_EXTRA, extra);
					av_free(extra);
				}
				ret = GxMediaFilter_Config(media->dumper);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_DUMPER_ERROR);
			}

			media->state = GX_MEDIA_STATE_READY;
			return GX_PLAYER_OK;
		}
	}

	return ret;
}

static int GxMedia_RunAudioLater(GxMedia* media)
{
	int ret;

	if (MEDIA_HAS_AUDIO(media) && media->audio_showing == 0)
	{
		av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
		GxMediaFilter_Stop(media->aout);
		GxMediaFilter_Stop(media->acodec1);
		GxMediaFilter_Stop(media->acodec);
		av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

		media_switch_audio(media);

		av_debug_first_avframe_duty(media->debug);
		av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
		ret = GxMediaFilter_Config(media->acodec);
		if (ret < 0) {
			if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
				return GX_PLAYER_ERROR;
		}
		else {
			if (MEDIA_HAS_AUDIO1(media)) {
				ret = GxMediaFilter_Config(media->acodec1);
				if (ret < 0) {
					if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
						return GX_PLAYER_ERROR;
				}
			}
			ret = GxMediaFilter_Config(media->aout);
			if (ret < 0) {
				if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
					return GX_PLAYER_ERROR;
			}
		}
		av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);

		av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
		ret = GxMediaFilter_Run(media->aout);
		MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_RENDER_ERROR);
		ret = GxMediaFilter_Run(media->acodec);
		MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_DECODER_ERROR);
		if (MEDIA_HAS_AUDIO1(media)) {
			ret = GxMediaFilter_Run(media->acodec1);
			MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_DECODER_ERROR);
		}
		av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
		GxCore_GetTickTime(&media->aframe.last_frame_time);
		media->audio_showing = 1;
	}

	return 0;
}

int GxMedia_Run(GxMedia* media)
{
	int ret = GX_PLAYER_OK;

	if (media)
	{
		if (media->state == GX_MEDIA_STATE_READY)
		{
			av_debug_first_avframe_duty(media->debug);
			GxMediaFilter_Run(media->dumper);

			if (media->muxer) {
				ret = GxMediaFilter_Run(media->muxer);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_MUXER_ERROR);
			}

			if (media->alive == 0)
			{
				av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Run(media->stream);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_ACCESS_ERROR);
				av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);

				av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Run(media->demuxer);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_DEMUX_ERROR);

				av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_VIDEO(media))
			{
				av_debug_video_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Run(media->vcodec);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_VIDEO_DECODER_ERROR);
				//ret = GxMediaFilter_Run(media->vout);
				//MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_VIDEO_RENDER_ERROR);
				av_debug_video_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_AUDIO(media)) {
				if (media->alive == 0) {
					av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
					ret = GxMediaFilter_Run(media->aout);
					MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_RENDER_ERROR);
					ret = GxMediaFilter_Run(media->acodec);
					MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_DECODER_ERROR);
					if (MEDIA_HAS_AUDIO1(media)) {
						ret = GxMediaFilter_Run(media->acodec1);
						MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_AUDIO_DECODER_ERROR);
					}
					av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
					GxCore_GetTickTime(&media->aframe.last_frame_time);
					media->audio_showing = 1;
				}
				else
					GxMedia_RunAudioLater(media);
			}
			else if (media->alive == 1) {
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->aout);
				GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->acodec);
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_SUB(media))
			{
				GxStreamSubHeader* sh_sub =  media->demuxer->s_streams[media->demuxer->sub->id];
				av_debug_subt_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				ret = GxMediaFilter_Run(media->scodec);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_SUB_DECODER_ERROR);
				GxSubOut_Control(media->sout,GX_SO_CTRL_SET_PID,&(sh_sub->priv.id));
				ret = GxMediaFilter_Run(media->sout);
				MEDIA_CHECK_RET(media, ret, PLAYER_ERROR_SUB_RENDER_ERROR);
				av_debug_subt_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			media->state = GX_MEDIA_STATE_RUNNING;
			GxCore_GetTickTime(&media->aframe.last_frame_time);
			GxCore_GetTickTime(&media->vframe.last_frame_time);
		}
	}

	return ret;
}

void GxMedia_Stop(GxMedia *media, int freeze, int alive)
{
	int ret = 0;
	int has_video, has_audio, has_sub, has_audio1;

	if (media)
	{
		media->alive = 0;
		media->audio_saved = 0;
		media->video_saved = 0;
		media->audio_showing = 0;
		media->video_showing = 0;
		if (media->state >= GX_MEDIA_STATE_INITING)
		{
			if (freeze)
				media->state = GX_MEDIA_STATE_FREEZED_IDLE;
			else
				media->state = GX_MEDIA_STATE_IDLE;

			has_video = MEDIA_HAS_VIDEO(media);
			has_audio = MEDIA_HAS_AUDIO(media);
			has_sub   = MEDIA_HAS_SUB(media);
			has_audio1 = MEDIA_HAS_AUDIO1(media);

			if(MEDIA_IS_DVB(media)) {
				av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->stream);
				av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->demuxer);
				av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				if (alive)
				{
					av_debug_alive_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
					GxMediaFilter_Pause(media->acodec);
					GxMediaFilter_Pause(media->vcodec);
					//GxMediaFilter_Pause(media->aout);
					av_debug_alive_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
					av_debug_stream_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
					ret |= GxMediaFilter_Config(media->stream);
					av_debug_stream_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
					ret |= GxMediaFilter_Config(media->demuxer);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
					ret |= GxMediaFilter_Run(media->demuxer);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
					av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
					ret |= GxMediaFilter_Run(media->stream);
					av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
					media->alive = 1;
				}
			}
			else if (MEDIA_IS_HWTS(media)) {
				av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->demuxer);
				av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->stream);
				av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
			}

			if (has_sub) {
				av_debug_subt_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->sout);
				GxMediaFilter_Stop(media->scodec);
				av_debug_subt_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
			}

			if(has_video) {
				av_debug_video_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
				av_debug_video_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
			}

			if (has_audio && alive == 0) {
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->aout);
				GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->acodec);
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
			}

			if ((!MEDIA_IS_HWTS(media)) && (!(MEDIA_IS_DVB(media)))) {
				if (MEDIA_IS_NETSTREAM(media)) {
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
					GxMediaFilter_Stop(media->demuxer);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

					av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
					GxStream_Control(media->stream, GX_STREAM_CTRL_FORCE_STOP_STREAM, NULL);
					GxMediaFilter_Stop(media->stream);
					av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
				}else{
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
					GxMediaFilter_Stop(media->demuxer);
					av_debug_demuxer_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

					av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
					GxMediaFilter_Stop(media->stream);
					av_debug_stream_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);
				}
			}

			GxMediaFilter_Stop(media->muxer);
			GxMediaFilter_Stop(media->dumper);
			media->runflag &= ~GX_MEDIA_RUN_PLAY;
			media->runflag &= ~GX_MEDIA_RUN_RECORD;
		} else {
			if (media->state == GX_MEDIA_STATE_FREEZED_IDLE)
				media->state = GX_MEDIA_STATE_IDLE;
			if (MEDIA_HAS_VIDEO(media))
			{
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
			}
		}
	}
}

void GxMedia_Pause(GxMedia* media)
{
	if (media)
	{
		if (!media->holdPause && !media->buffering) return;

		GxCore_MutexLock(media->mutex);
		if (media->state == GX_MEDIA_STATE_RUNNING || media->state == GX_MEDIA_STATE_SOON_OVER)
		{
			if (MEDIA_HAS_AUDIO(media))
			{
				GxMediaFilter_Pause(media->aout);
				if (MEDIA_HAS_AUDIO1(media))
					GxMediaFilter_Pause(media->acodec1);
				GxMediaFilter_Pause(media->acodec);
			}
			if (MEDIA_HAS_VIDEO(media))
			{
				GxMediaFilter_Pause(media->vout);
				GxMediaFilter_Pause(media->vcodec);
			}
			if (MEDIA_HAS_SUB(media))
			{
				GxMediaFilter_Pause(media->sout);
				GxMediaFilter_Pause(media->scodec);
			}

			GxMediaFilter_Pause(media->demuxer);
			if (!media->buffering_pause && !media->seeking)
				GxMediaFilter_Pause(media->stream);

			GxMediaFilter_Pause(media->muxer);
			GxMediaFilter_Pause(media->dumper);
		}
		GxCore_MutexUnlock(media->mutex);
	}
}

void GxMedia_Resume(GxMedia* media)
{
	if (media)
	{
		if (media->holdPause || media->buffering) return;

		GxCore_MutexLock(media->mutex);
		if (media->state == GX_MEDIA_STATE_RUNNING || media->state == GX_MEDIA_STATE_SOON_OVER)
		{
			GxMediaFilter_Resume(media->demuxer);
			if (!media->buffering_pause && !media->seeking)
				GxMediaFilter_Resume(media->stream);

			if (MEDIA_HAS_AUDIO(media))
			{
				GxMediaFilter_Resume(media->acodec);
				if (MEDIA_HAS_AUDIO1(media))
					GxMediaFilter_Resume(media->acodec1);
				GxMediaFilter_Resume(media->aout);
			}

			if (MEDIA_HAS_VIDEO(media))
			{
				if (MEDIA_HAS_AUDIO(media) && (!MEDIA_IS_DVB(media)) &&
						media->demuxer->audio->dropmode == DROPMODE_NONE)
				{
					GxTime start, current;
					int timeoutms = 1000, costms = 0;
					AVCodecStatus acodec;

					acodec.state = AVCODEC_STOPPED;
					GxMedia_GetCodecStatus(media, &acodec, NULL, NULL, NULL);
					GxCore_GetTickTime(&start);
					while((acodec.state!=AVCODEC_RUNNING) && (costms<timeoutms))
					{
						GxMedia_GetCodecStatus(media, &acodec, NULL, NULL, NULL);
						GxCore_ThreadDelay(10);
						GxCore_GetTickTime(&current);
						costms = TIME_MINUS(current, start);
					}
				}
				GxMediaFilter_Resume(media->vcodec);
				GxMediaFilter_Resume(media->vout);
			}

			if (MEDIA_HAS_SUB(media))
			{
				GxMediaFilter_Resume(media->scodec);
				GxMediaFilter_Resume(media->sout);
			}

			GxMediaFilter_Resume(media->dumper);
			GxMediaFilter_Resume(media->muxer);
		}
		GxCore_MutexUnlock(media->mutex);
	}
}

void GxMedia_Destroy(GxMedia* media, int freeze)
{
	if (media)
	{
		GxMedia_Stop(media, freeze, 0);

		GxStream_Close(media->stream);

		GxDemuxer_Close(media->demuxer);

		GxAudioDecoder_Close(media->acodec1);
		GxAudioDecoder_Close(media->acodec);
		GxAudioOut_Close(media->aout, 1);

		GxVideoDecoder_Close(media->vcodec);
		GxVideoOut_Close(media->vout);

		GxSubDecoder_Close(media->scodec);
		GxSubOut_Close(media->sout);

		GxMuxer_Close(media->muxer);
		GxDumpFilter_Close(media->dumper);

		GxCore_MutexDelete(media->mutex);

		av_debug_media_mod_end(media->debug, media);

		av_free(media);
	}
}

static GxMedia* new_media_player(GxMediaPara* para)
{
	GxMedia* media ;

	if (para == NULL || para->player.url == NULL)
		return NULL;

	media = av_mallocz(sizeof(GxMedia));
	if (media == NULL)
		return NULL;

	media->debug  = para->debug;
	media->speed = 1000;
	media->sub   = para->player.sub;
	media->type  = GX_MEDIA_PLAYER;
	media->event[0] = para->player.event;
	media->runflag |= GX_MEDIA_RUN_PLAY;

	av_debug_stream_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
	if (para->player.stream == NULL)
	{
		media->stream = GxStream_Open(para->player.url,GX_STREAM_READ, para->player.bandwidth);
		if (media->stream == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DATA_SOURCE);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:Stream Open Fail !\n");
			return NULL;
		}
	}
	else{
		media->stream = para->player.stream;
	}

	GxStream_Control(media->stream, GX_STREAM_CTRL_GET_CUR_SEQ, &media->stream->cur_seq_no);
#ifdef CONFIG_STREAM_CACHE
	if (media->stream->file_format == GX_STREAMTYPE_STREAM) {
		if (media->stream->demuxer_type != GX_DEMUXER_TYPE_ASSOCIATIONLIBRE) {
			int cache_size, seek_limit;
			GxPlayer_SystemGet(PSYS_NETWORK_CACHE,      &cache_size);
			GxPlayer_SystemGet(PSYS_NETWORK_SEEK_CACHE, &seek_limit);
			GxMediaFilter_SetEvent(media->stream, media_filter_event, media);
			GxStream_CacheEnable(media->stream, cache_size, seek_limit);
		}
	}
#endif
	av_debug_stream_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);

	av_debug_demuxer_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
	para->player.demuxInfo.demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
	para->player.demuxInfo.media        = media;
	para->player.demuxInfo.debug        = media->debug;
	media->demuxer = GxDemuxer_Open(media->stream,GX_DEMUXER_FLAG_PLAYER, &para->player.demuxInfo);
	if (media->demuxer == NULL)
	{
		int error_status = PLAYER_ERROR_NO_ERROR;
		GxPlayer_SystemGet(PSYS_PLAY_ERROR_STATUS, &error_status);
		if (GX_STREAMTYPE_DEMUXER == media->stream->file_format && GX_DEMUXER_TYPE_LAVF == media->stream->demuxer_type) {
			error_status = (error_status>0)?error_status:media->stream->err.open_err;
			error_status = error_status?error_status:PLAYER_ERROR_NO_DATA_SOURCE;
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, error_status);
		} else {
			PLAYER_INTERRUPT_CBK _interruptcbk = NULL;
			GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &_interruptcbk);
			if (!(_interruptcbk && _interruptcbk())) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, (error_status > 0)?error_status:PLAYER_ERROR_NO_DEMUX_TOOLS);
			}
		}
		error_status = PLAYER_ERROR_NO_ERROR;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
		GxMedia_Destroy(media, 0);
		gxlogf("[Media]:Demuxer Open Fail !\n");
		return NULL;
	}
	media->demuxer->media_priv = media;
	av_debug_demuxer_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);

	GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_BASE_TIME, NULL);

	if (para->player.start>0 && media->demuxer->seekable && media->demuxer->duration>0)
	{
		int ret = GX_PLAYER_ERROR;
		uint64_t seek_minms = GxDemuxer_GetSeekMinTime(media->demuxer);

		if (para->player.start <= seek_minms)
			para->player.start = seek_minms;

#define SEEK_STEP (1000)
		if ((para->player.start + SEEK_STEP) >= media->demuxer->duration)
			para->player.start = (para->player.start >= SEEK_STEP) ? (media->demuxer->duration - SEEK_STEP) : 0;

		ret = GxDemuxer_Seek(media->demuxer, para->player.start, GX_DEMUXER_SEEK_ABSOLUTE);
		if (ret == GX_PLAYER_OK) {
			media->demuxer->stc_start_timems = para->player.start;
		}else{
			if (media->stream->file_format == GX_STREAMTYPE_STREAM) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_DEMUX_ERROR);
				GxMedia_Destroy(media, 0);
				return NULL;
			} else {
				if (GX_DEMUXER_TYPE_LAVF == media->demuxer->type) {
					para->player.start = 0;
					media->demuxer->stc_start_timems = para->player.start;
				}
			}
		}
	}

	media->rtimeout = media->stream->read_timeoutms;
	media->sh_audio  = media->demuxer->audio->sh;
	media->sh_audio1 = media->demuxer->audio1->sh;
	media->sh_video = media->demuxer->video->sh;
	media->sh_sub   = media->demuxer->sub->sh;

	if (media->sh_audio)
	{
		av_debug_audio_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
		media->acodec = GxAudioDecoder_Open(media->sh_audio);
		if (media->acodec == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_DECODER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:AudioDec Open Fail !\n");
			return NULL;
		}

		if (media->sh_audio1)
		{
			media->acodec1 = GxAudioDecoder_Open(media->sh_audio1);
			if (media->acodec1 == NULL)
			{
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_DECODER);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:AudioDec Open Fail !\n");
				return NULL;
			}
		}

		media->aout = GxAudioOut_Open(media->sh_audio, media->sh_audio1);
		if (media->aout == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_RENDER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:AudioPlay Open Fail !\n");
			return NULL;
		}
		av_debug_audio_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);
	}

	if (media->sh_video)
	{
		av_debug_video_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
		media->vcodec = GxVideoDecoder_Open(media->sh_video);
		if (media->vcodec == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_VIDEO_DECODER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:VideoDec Open Fail !\n");
			return NULL;
		}

		media->vout = GxVideoOut_Open(media->sh_video, para->player.sub);
		if (media->vout== NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_VIDEO_RENDER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:VideoOut Open Fail !\n");
			return NULL;
		}
		av_debug_video_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);
	}

	if (media->sh_sub)
	{
		av_debug_subt_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_BEGIN);
		media->scodec = GxSubDecoder_Open(media->sh_sub);
		if (media->scodec == NULL) {
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_DECODER);
		}
		media->sout = GxSubOut_Open(media->sh_sub,media_get_current_cb,media);
		if (media->sout == NULL) {
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_RENDER);
		}
		av_debug_subt_duty(media->debug, AV_TICK_OP_OPEN, AV_TICK_ST_END);
	}

	if (media->sh_audio)
	{
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_AO,    media->acodec, GX_AD_PIN_NAME_ESA_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_PCM_OUT,  media->aout,   GX_AO_PIN_NAME_PCM_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AC3_OUT,  media->aout,   GX_AO_PIN_NAME_AC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_EAC3_OUT, media->aout,   GX_AO_PIN_NAME_EAC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_DTS_OUT,  media->aout,   GX_AO_PIN_NAME_DTS_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AAC_OUT,  media->aout,   GX_AO_PIN_NAME_AAC_IN);
		if(media->sh_audio1){
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_ADAO, media->acodec1, GX_AD_PIN_NAME_ADESA_IN);
			GxMediaFilter_Connect(media->acodec1, GX_AD_PIN_NAME_ADPCM_OUT,media->aout,    GX_AO_PIN_NAME_ADPCM_IN);
		}
	}

	if (media->sh_sub)
	{
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_SO, media->scodec, GX_SD_PIN_NAME_IN);
		GxMediaFilter_Connect(media->scodec, GX_SD_PIN_NAME_OUT, media->sout, GX_SO_PIN_NAME_IN);
	}

	GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_VO, media->vcodec, GX_VD_PIN_NAME_IN);
	GxMediaFilter_Connect(media->vcodec, GX_VD_PIN_NAME_OUT, media->vout, GX_VO_PIN_NAME_IN);

	GxCore_MutexCreate(&media->mutex);
	return media;
}

static GxMedia* restart_media_player(GxMedia*media, GxMediaPara* para)
{
	uint64_t totle = 0;

	if (para == NULL || media == NULL)
		return NULL;

	media->sub   = para->player.sub;
	media->type  = GX_MEDIA_PLAYER;
	media->event[0] = para->player.event;
	media->buffering = 0;

	if (media->stream) {
		media->stream->err.recv_err = 0;
		media->stream->err.open_err = 0;
		media->stream->err.need_restart = 0;
	}

	//restart frist seq
	int seek_seq = 0;
	GxStream_Control(media->stream, GX_STREAM_CTRL_CACHE_RESET, &seek_seq);

	para->player.demuxInfo.demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;
	media->demuxer = GxDemuxer_Open(media->stream,GX_DEMUXER_FLAG_PLAYER, &para->player.demuxInfo);
	if (media->demuxer == NULL)
	{
		MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DEMUX_TOOLS);
		GxMedia_Destroy(media, 0);
		gxlogf("[Media]:Demuxer Reset Fail !\n");
		return NULL;
	}

	GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_BASE_TIME, NULL);

	if (para->player.start>0 && media->demuxer->seekable)
	{
		totle = GxDemuxer_GetTimeLength(media->demuxer);
		if (para->player.start < totle)
		{
			uint64_t seek_minms = GxDemuxer_GetSeekMinTime(media->demuxer);
			if (para->player.start <= seek_minms)
				para->player.start = seek_minms;

			GxDemuxer_Seek(media->demuxer, (int64_t)para->player.start, GX_DEMUXER_SEEK_ABSOLUTE);
			media->demuxer->stc_start_timems = para->player.start;
		}
	}

	media->rtimeout  = media->stream->read_timeoutms;
	media->sh_audio  = media->demuxer->audio->sh;
	media->sh_audio1 = media->demuxer->audio1->sh;
	media->sh_video  = media->demuxer->video->sh;
	media->sh_sub    = media->demuxer->sub->sh;

	if (media->sh_audio)
	{
		media->acodec = GxAudioDecoder_Open(media->sh_audio);
		if (media->acodec == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_DECODER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:AudioDec Open Fail !\n");
			return NULL;
		}

		if (media->sh_audio1)
		{
			media->acodec1 = GxAudioDecoder_Open(media->sh_audio1);
			if (media->acodec1 == NULL)
			{
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_DECODER);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:AudioDec Open Fail !\n");
				return NULL;
			}
		}

		media->aout = GxAudioOut_Open(media->sh_audio, media->sh_audio1);
		if (media->aout == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_AUDIO_RENDER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:AudioPlay Open Fail !\n");
			return NULL;
		}
	}

	if (media->sh_video)
	{
		media->vcodec = GxVideoDecoder_Open(media->sh_video);
		if (media->vcodec == NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_VIDEO_DECODER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:VideoDec Open Fail !\n");
			return NULL;
		}

		media->vout = GxVideoOut_Open(media->sh_video, para->player.sub);
		if (media->vout== NULL)
		{
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_VIDEO_RENDER);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:VideoOut Open Fail !\n");
			return NULL;
		}
	}

	if (media->sh_sub)
	{
		media->scodec = GxSubDecoder_Open(media->sh_sub);
		if (media->scodec == NULL) {
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_DECODER);
		}
		media->sout = GxSubOut_Open(media->sh_sub,media_get_current_cb,media);
		if (media->sout == NULL) {
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_RENDER);
		}
	}

	if (media->sh_audio)
	{
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_AO,    media->acodec, GX_AD_PIN_NAME_ESA_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_PCM_OUT,  media->aout,   GX_AO_PIN_NAME_PCM_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AC3_OUT,  media->aout,   GX_AO_PIN_NAME_AC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_EAC3_OUT, media->aout,   GX_AO_PIN_NAME_EAC3_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_DTS_OUT,  media->aout,   GX_AO_PIN_NAME_DTS_IN);
		GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AAC_OUT,  media->aout,   GX_AO_PIN_NAME_AAC_IN);
		if(media->sh_audio1)
		{
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_ADAO,  media->acodec1, GX_AD_PIN_NAME_ADESA_IN);
			GxMediaFilter_Connect(media->acodec1, GX_AD_PIN_NAME_ADPCM_OUT, media->aout, GX_AO_PIN_NAME_ADPCM_IN);
		}
	}


	if (media->sh_sub)
	{
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_SO, media->scodec, GX_SD_PIN_NAME_IN);
		GxMediaFilter_Connect(media->scodec, GX_SD_PIN_NAME_OUT, media->sout, GX_SO_PIN_NAME_IN);
	}

	GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_VO, media->vcodec, GX_VD_PIN_NAME_IN);
	GxMediaFilter_Connect(media->vcodec, GX_VD_PIN_NAME_OUT, media->vout, GX_VO_PIN_NAME_IN);

	return media;
}

static GxMedia* new_media_recorder(GxMediaPara* para)
{
	GxMedia* media ;

	if (para == NULL || para->recorder.srcurl == NULL || para->recorder.dsturl == NULL)
		return NULL;

	media = av_mallocz(sizeof(GxMedia));
	if (media == NULL)
		return NULL;

	media->speed = 1000;
	media->sub   = para->recorder.sub;
	media->type  = GX_MEDIA_RECORDER;
	media->event[0] = para->recorder.event;
	media->runflag |= GX_MEDIA_RUN_RECORD;

	media->stream = GxStream_Open(para->recorder.srcurl, GX_STREAM_READ, 0);
	if (media->stream == NULL) {
		MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_DATA_SOURCE);
		GxMedia_Destroy(media, 0);
		gxlogf("[Media]:stream open fail !\n");
		return NULL;
	}
	if (URL_IS_DVB(para->recorder.srcurl)) {
		media->demuxer = GxDemuxer_Open(media->stream, GX_DEMUXER_FLAG_RECORDER, 0);
		if (media->demuxer == NULL) {
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_DEMUX_TOOLS);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:Demuxer Open Fail !\n");
			return NULL;
		}
		media->dumper = GxDumpFilter_Open(para->recorder.dsturl, DUMPFILTER_DVB);
		if (media->dumper == NULL) {
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DUMP_TOOLS);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:DumpFilter Open Fail !\n");
			return NULL;
		}
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_TSO, media->dumper, GX_DUMPFILTER_PIN_NAME_IN);
	} else if (URL_IS_FM(para->recorder.srcurl)) {
		media->dumper = GxDumpFilter_Open(para->recorder.dsturl, DUMPFILTER_FM);
		if (media->dumper == NULL) {
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DUMP_TOOLS);
			GxMedia_Destroy(media, 0);
			gxlogf("[Media]:DumpFilter Open Fail !\n");
			return NULL;
		}
		GxDumpFilter_Control(media->dumper, GX_DUMPER_SET_SOURCE, (void *)(media->stream));
	} else {
		if (1) {
			GxMuxerOpenInfo info = {0};
			GxMuxerHeaderInfo aheader_info;
			GxMuxerHeaderInfo vheader_info;
			GxDemuxer* demuxer = media->demuxer = GxDemuxer_Open(media->stream, GX_DEMUXER_FLAG_MUXER, 0);
			if (media->demuxer == NULL) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DEMUX_TOOLS);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:Demuxer Open Fail !\n");
				return NULL;
			}

			info.format = "mpegts";
			info.nb_streams = 0;
			if (HAVE_VIDEO(demuxer)) {
				GxStreamVideoHeader *vheader = media->demuxer->video->sh;

				vheader_info.stream_index                = info.nb_streams;
				media->demuxer->video->mux_index         = info.nb_streams;
				vheader_info.extradata                   = vheader->priv.header.data;
				vheader_info.extradata_size              = vheader->priv.header.len;
				info.streams[info.nb_streams].codec_id   = vcodec_std2codecid(vheader->format);
				info.streams[info.nb_streams].codec_type = CODEC_TYPE_VIDEO;
				info.nb_streams++;
			}

			if (HAVE_AUDIO(demuxer)){
				GxStreamAudioHeader *aheader = media->demuxer->audio->sh;

				aheader_info.stream_index                = info.nb_streams;
				media->demuxer->audio->mux_index         = info.nb_streams;
				aheader_info.extradata                   = aheader->priv.header.data;
				aheader_info.extradata_size              = aheader->priv.header.len;
				info.streams[info.nb_streams].codec_id   = acodec_std2codecid(aheader->format);
				info.streams[info.nb_streams].codec_type = CODEC_TYPE_AUDIO;
				info.nb_streams++;
			}

			media->muxer = GxMuxer_Open(&info);
			if (media->muxer == NULL) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_MUXER_TOOLS);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:MuxerFilter Open Fail !\n");
				return NULL;
			}
			media->dumper = GxDumpFilter_Open(para->recorder.dsturl, DUMPFILTER_RAW);
			if (media->dumper == NULL) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DUMP_TOOLS);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:DumpFilter Open Fail !\n");
				return NULL;
			}
			GxMuxer_Control(media->muxer, GX_MUXER_CTRL_SET_VIDEO_HEADER, &vheader_info);
			GxMuxer_Control(media->muxer, GX_MUXER_CTRL_SET_AUDIO_HEADER, &aheader_info);
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_MUXO, media->muxer, GX_MUXER_PIN_NAME_INPUT);
			GxMediaFilter_Connect(media->muxer, GX_MUXER_PIN_NAME_OUTPUT, media->dumper, GX_DUMPFILTER_PIN_NAME_IN);
		}
		else {
			GxStream_RecordEnable(media->stream, 0x400000);
			media->dumper = GxDumpFilter_Open(para->recorder.dsturl, DUMPFILTER_RAW);
			if (media->dumper == NULL) {
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_NO_DUMP_TOOLS);
				GxMedia_Destroy(media, 0);
				gxlogf("[Media]:DumpFilter Open Fail !\n");
				return NULL;
			}
			GxMediaFilter_Connect(media->stream, GX_STREAM_PIN_NAME_OUTPUT, media->dumper, GX_DUMPFILTER_PIN_NAME_IN);
		}
	}

	media->rtimeout  = media->stream->read_timeoutms;
	if (media->demuxer) {
		media->sh_audio  = media->demuxer->audio->sh;
		media->sh_audio1 = media->demuxer->audio1->sh;
		media->sh_video  = media->demuxer->video->sh;
		media->sh_sub    = media->demuxer->sub->sh;
	}

	return media;
}

static GxMedia* new_media_prober(GxMediaPara* para)
{
	GxMedia* media ;

	if (para == NULL || para->prober.url == NULL)
		return NULL;

	media = av_mallocz(sizeof(GxMedia));
	if (media == NULL)
		return NULL;

	media->speed = 1000;
	media->stream = GxStream_Open(para->prober.url,GX_STREAM_READ, 0);
	if (media->stream == NULL)
	{
		GxMedia_Destroy(media, 0);
		gxlogf("[Media]:Stream Open Fail !\n");
		return NULL;
	}

	media->demuxer = GxDemuxer_Open(media->stream,GX_DEMUXER_FLAG_PROBE,0);
	if (media->demuxer == NULL)
	{
		GxMedia_Destroy(media, 0);
		gxlogf("[Media]:Demuxer Open Fail !\n");
		return NULL;
	}

	media->sh_audio = media->demuxer->audio->sh;
	media->sh_video = media->demuxer->video->sh;
	media->sh_sub   = media->demuxer->sub->sh;

	return media;
}

GxMedia* GxMedia_New(GxMediaType type, GxMediaPara* para)
{
	GxMedia* media;

	if (para == NULL)
		return NULL;

	if (type < GX_MEDIA_PLAYER || type > GX_MEDIA_PCMDUMPER)
		return NULL;

	switch(type)
	{
	case GX_MEDIA_PLAYER:
		media = new_media_player(para);
		break;
	case GX_MEDIA_RECORDER:
		media = new_media_recorder(para);
		break;
	case GX_MEDIA_PROBER:
		media = new_media_prober(para);
		break;
	default:
		return NULL;
	}

	if (media)
	{
		GxMediaFilter_SetEvent(media->stream, media_filter_event, media);
		GxMediaFilter_SetEvent(media->demuxer, media_filter_event, media);
		GxMediaFilter_SetEvent(media->vcodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->vout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->acodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->aout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->scodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->sout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->dumper, media_filter_event, media);
		GxMediaFilter_SetEvent(media->muxer, media_filter_event, media);

#ifdef AV_GXPLAYER_DEBUG
		{
			av_debug_media_mod_begin(media->debug, media, type);

			if (media->stream) {
				GxStreamClass* st = GetGxStreamClassFromObject(media->stream);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_STREAM, st->_inherit._inherit.name);
			}

			if (media->demuxer) {
				GxDemuxerClass* dc = GetGxDemuxerClassFromObject(media->demuxer);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_DEMUXER, dc->name);
			}

			if (media->acodec) {
				GxAVCodecClass* ac = GxGetDecoderClassFromObject(media->acodec);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_AUDIO_DECODER_0, ac->name);
				if (MEDIA_HAS_AUDIO(media)) {
					GxStreamAudioHeader *sh = media->sh_audio;
					av_debug_media_mod_duty(media->debug, media, AV_MEDIA_AUDIO_CODEC_0, acodec_std2str(sh->format));
				}
			}

			if (media->acodec1) {
				GxAVCodecClass* ac = GxGetDecoderClassFromObject(media->acodec1);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_AUDIO_DECODER_1, ac->name);
				if (MEDIA_HAS_AUDIO1(media)) {
					GxStreamAudioHeader *sh = media->sh_audio1;
					av_debug_media_mod_duty(media->debug, media, AV_MEDIA_AUDIO_CODEC_1, acodec_std2str(sh->format));
				}
			}

			if (media->vcodec) {
				GxAVCodecClass* vc = GxGetDecoderClassFromObject(media->vcodec);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_VIDEO_DECODER, vc->name);
				if (MEDIA_HAS_VIDEO(media)) {
					GxStreamVideoHeader *sh = media->sh_video;
					av_debug_media_mod_duty(media->debug, media, AV_MEDIA_VIDEO_CODEC, vcodec_std2str(sh->format));
				}
			}

			if (media->scodec) {
				GxAVCodecClass* sc = GxGetDecoderClassFromObject(media->scodec);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_SUBTITLE_DECODER, sc->name);
			}

			if (media->aout) {
				GxAudioOutClass* ac = GetGxAudioOutClassFromObject(media->aout);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_AUDIO_PLAY, ac->name);
			}

			if (media->vout) {
				GxVideoOutClass* vc = GetGxVideoOutClassFromObject(media->vout);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_VIDEO_PLAY, vc->name);
			}

			if (media->sout) {
				GxSubOutClass* sc = GetGxSubOutClassFromObject(media->sout);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_SUBTITLE_PLAY, sc->name);
			}

			if (media->dumper) {
				GxDumpFilterClass* dc = GetDumpFilterClassFromObject(media->dumper);
				av_debug_media_mod_duty(media->debug, media, AV_MEDIA_DUMPER, dc->_inherit._inherit.name);
			}
		}
#endif
	}

	return media;
}

GxMedia* GxMedia_Restart(GxMedia* media, GxMediaPara* para)
{
	int has_video, has_audio, has_sub, has_audio1, freeze=1;
	if (para == NULL || media == NULL)
		return NULL;

	if (media) {
		if (media->state >= GX_MEDIA_STATE_INITING)
		{
			media->state = GX_MEDIA_STATE_FREEZED_IDLE;
			has_video = MEDIA_HAS_VIDEO(media);
			has_audio = MEDIA_HAS_AUDIO(media);
			has_sub   = MEDIA_HAS_SUB(media);
			has_audio1= MEDIA_HAS_AUDIO1(media);
			GxMediaFilter_Stop(media->dumper);
			if (has_video) {
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
			}
			if (has_audio)
				GxMediaFilter_Stop(media->aout);
			if (has_sub)
				GxMediaFilter_Stop(media->sout);
			if (has_video) {
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
			}
			if (has_audio1)
				GxMediaFilter_Stop(media->acodec1);
			if (has_audio)
				GxMediaFilter_Stop(media->acodec);
			if (has_sub)
				GxMediaFilter_Stop(media->scodec);
			//no restart stream
			GxMediaFilter_Stop(media->demuxer);
		}
		else if (media->state == GX_MEDIA_STATE_FREEZED_IDLE)
		{
			media->state = GX_MEDIA_STATE_IDLE;
			if (MEDIA_HAS_VIDEO(media))
			{
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
			}
		}

		GxDemuxer_Close(media->demuxer);
		GxAudioDecoder_Close(media->acodec1);
		GxAudioDecoder_Close(media->acodec);
		GxAudioOut_Close(media->aout, 1);
		GxVideoDecoder_Close(media->vcodec);
		GxVideoOut_Close(media->vout);
		GxSubDecoder_Close(media->scodec);
		GxSubOut_Close(media->sout);
		GxDumpFilter_Close(media->dumper);
		GxMuxer_Close(media->muxer);
		media->demuxer = NULL;
		media->acodec1 = NULL;
		media->acodec = NULL;
		media->aout = NULL;
		media->vcodec = NULL;
		media->vout = NULL;
		media->scodec = NULL;
		media->dumper = NULL;
		media->muxer = NULL;
	}

	media = restart_media_player(media, para);
	if (media)
	{
		GxMediaFilter_SetEvent(media->stream, media_filter_event, media);
		GxMediaFilter_SetEvent(media->demuxer, media_filter_event, media);
		GxMediaFilter_SetEvent(media->vcodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->vout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->acodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->aout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->scodec, media_filter_event, media);
		GxMediaFilter_SetEvent(media->sout, media_filter_event, media);
		GxMediaFilter_SetEvent(media->dumper, media_filter_event, media);
	}

	return media;
}

int _meida_audio_need_play(GxMedia* media, int speed)
{
	int need_play = 0;
	int support_speed = 0;

	GxAudioDecoder_Control(media->acodec, GX_AD_CTRL_SUPPORT_SPEED, &support_speed);
	if (!MEDIA_HAS_VIDEO(media)) {
		if (support_speed)
			need_play = (GX_SPEED_SLOW(speed) || GX_SPEED_NORMAL(speed) || GX_SPEED_FAST(speed)) ? 1 : 0;
		else
			need_play = -1;
	} else {
		if (support_speed)
			need_play = (GX_SPEED_SLOW(speed) || GX_SPEED_NORMAL(speed)) ? 1 : 0;
		else
			need_play = (speed == 1000) ? 1 : 0;
	}

	return need_play;
}

int _media_video_need_clear(GxMedia* media, int old_speed, int new_speed)
{
	int need_clear = 0;

	if (MEDIA_HAS_VIDEO(media)) {
		if ((GX_SPEED_SLOW(old_speed) || GX_SPEED_NORMAL(old_speed) || GX_SPEED_FAST(old_speed)) &&
				GX_SPEED_JUMP(new_speed)) {
			need_clear = 1;
		} else if (GX_SPEED_JUMP(old_speed) &&
				(GX_SPEED_SLOW(new_speed) || GX_SPEED_NORMAL(new_speed) || GX_SPEED_FAST(new_speed))) {
			need_clear = 1;
		}
	}
	return need_clear;
}

int GxMedia_SetSpeed(GxMedia* media, int speed)
{
	int ret = GX_PLAYER_ERROR;

	if (media)
	{
		GxCore_MutexLock(media->mutex);
		if (media->errInfo != PLAYER_ERROR_VIDEO_DECODER_ERROR)
		{
			int freeze = 1;
			int rtimeout = (speed > 1000) ? 0 : media->rtimeout;
			int have_audio_play  = _meida_audio_need_play(media, media->speed);
			int need_audio_play  = _meida_audio_need_play(media, speed);
			int need_clear_video = _media_video_need_clear(media, media->speed, speed);

			if (media->state == GX_MEDIA_STATE_RUNNING)
			{
				if ((have_audio_play == -1) && (need_audio_play == -1)) {
					GxCore_MutexUnlock(media->mutex);
					return ret;
				} else if (have_audio_play == need_audio_play) {
					if (need_clear_video) {
						GxDemuxer_Speed(media->demuxer, speed, 1, have_audio_play, need_audio_play);
						GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
						GxMediaFilter_Stop(media->vcodec);
						av_memhole_flush();
						GxMediaFilter_Config(media->vcodec);
						GxAudioOut_Control(media->aout,        GX_AO_CTRL_SET_SPEED, &speed);
						GxAudioDecoder_Control(media->acodec,  GX_AD_CTRL_SET_SPEED, &speed);
						GxAudioDecoder_Control(media->acodec1, GX_AD_CTRL_SET_SPEED, &speed);
						GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_SET_SPEED, &speed);
						GxMediaFilter_Run(media->vcodec);
					} else {
						GxDemuxer_Speed(media->demuxer, speed, 0, have_audio_play, need_audio_play);
						GxAudioOut_Control(media->aout,        GX_AO_CTRL_SET_SPEED, &speed);
						GxAudioDecoder_Control(media->acodec,  GX_AD_CTRL_SET_SPEED, &speed);
						GxAudioDecoder_Control(media->acodec1, GX_AD_CTRL_SET_SPEED, &speed);
						GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_SET_SPEED, &speed);
					}
				} else if ((have_audio_play == 1) && (need_audio_play == 0)) {
					if (need_clear_video) {
						GxDemuxer_Speed(media->demuxer, speed, 1, have_audio_play, need_audio_play);
						GxMediaFilter_Stop(media->aout);
						GxMediaFilter_Stop(media->acodec1);
						GxMediaFilter_Stop(media->acodec);
						GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
						GxMediaFilter_Stop(media->vcodec);
						av_memhole_flush();
						GxMediaFilter_Config(media->vcodec);
						GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_SPEED, &speed);
						GxMediaFilter_Run(media->vcodec);
					} else {
						GxDemuxer_Speed(media->demuxer, speed, 0, have_audio_play, need_audio_play);
						GxMediaFilter_Stop(media->aout);
						GxMediaFilter_Stop(media->acodec1);
						GxMediaFilter_Stop(media->acodec);
						av_memhole_flush();
						GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_SPEED, &speed);
					}
				} else if ((have_audio_play == 0) && (need_audio_play == 1)) {
					GxDemuxer_Speed(media->demuxer, speed, 1, have_audio_play, need_audio_play);
					GxMediaFilter_Stop(media->aout);
					GxMediaFilter_Stop(media->acodec1);
					GxMediaFilter_Stop(media->acodec);
					GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
					GxMediaFilter_Stop(media->vcodec);
					av_memhole_flush();
					GxMediaFilter_Config(media->aout);
					GxMediaFilter_Config(media->acodec);
					GxMediaFilter_Config(media->acodec1);
					GxMediaFilter_Config(media->vcodec);
					GxAudioOut_Control(media->aout,        GX_AO_CTRL_SET_SPEED, &speed);
					GxAudioDecoder_Control(media->acodec,  GX_AD_CTRL_SET_SPEED, &speed);
					GxAudioDecoder_Control(media->acodec1, GX_AD_CTRL_SET_SPEED, &speed);
					GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_SET_SPEED, &speed);
					GxMediaFilter_Resume(media->demuxer);
					GxMediaFilter_Run(media->aout);
					GxMediaFilter_Run(media->acodec);
					GxMediaFilter_Run(media->acodec1);
					GxMediaFilter_Run(media->vcodec);
				}
			}
			else if (media->state == GX_MEDIA_STATE_SOON_OVER) {
				int need_audio_play = _meida_audio_need_play(media, speed);

				GxDemuxer_Speed(media->demuxer, speed, 1, have_audio_play, need_audio_play);
				GxMediaFilter_Stop(media->aout);
				GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->acodec);
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
				if (need_audio_play) {
					GxMediaFilter_Config(media->aout);
					GxMediaFilter_Config(media->acodec);
					GxMediaFilter_Config(media->acodec1);
				}
				GxMediaFilter_Config(media->vcodec);
				GxAudioOut_Control(media->aout,       GX_AO_CTRL_SET_SPEED, &speed);
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_SPEED, &speed);
				if (need_audio_play) {
					GxMediaFilter_Run(media->aout);
					GxMediaFilter_Run(media->acodec);
					GxMediaFilter_Run(media->acodec1);
				}
				GxMediaFilter_Run(media->vcodec);
				media->state = GX_MEDIA_STATE_RUNNING;
			}
			else
			{
				GxCore_MutexUnlock(media->mutex);
				return ret;
			}
			GxStream_Control(media->stream, GX_STREAM_CTRL_SET_READ_TIMEOUT, &rtimeout);
			media->speed = speed;
			ret = GX_PLAYER_OK;
		}
		GxCore_MutexUnlock(media->mutex);
	}

	return ret;
}

int GxMedia_TrackAdd(GxMedia* media, GxStreamTrackAdd* ptrack)
{
	if (media)
	{
		GxCore_MutexLock(media->mutex);
		if (media->state == GX_MEDIA_STATE_RUNNING || media->state == GX_MEDIA_STATE_SOON_OVER)
		{
			GxStream_Control(media->stream, GX_STREAM_CTRL_ADD_TRACK, ptrack);
		}
		GxCore_MutexUnlock(media->mutex);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

int GxMedia_TrackDel(GxMedia* media, GxStreamTrackDel* ptrack)
{
	if (media)
	{
		GxCore_MutexLock(media->mutex);
		if (media->state == GX_MEDIA_STATE_RUNNING || media->state == GX_MEDIA_STATE_SOON_OVER)
		{
			GxStream_Control(media->stream, GX_STREAM_CTRL_DEL_TRACK, ptrack);
		}
		GxCore_MutexUnlock(media->mutex);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

int  GxMedia_SeekPause(GxMedia* media, int64_t time, int flag)
{
	int ret = GX_PLAYER_OK;

	if (media)
	{
		GxCore_MutexLock(media->mutex);
		if ((media->state == GX_MEDIA_STATE_RUNNING) ||
				(media->state == GX_MEDIA_STATE_SOON_OVER) ||
				(media->state == GX_MEDIA_STATE_OVER))
		{
			media->video_showing = 0;
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_PAUSE_FILL_DATA, NULL);
			av_debug_demuxer_duty(media->debug, AV_TICK_OP_SEEK, AV_TICK_ST_BEGIN);
			ret = GxDemuxer_Seek(media->demuxer, time, flag);
			av_debug_demuxer_duty(media->debug, AV_TICK_OP_SEEK, AV_TICK_ST_END);

			if (MEDIA_HAS_AUDIO(media))
			{
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->aout);
				GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->acodec);
				av_debug_audio_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				if (GxMediaFilter_Config(media->acodec) == 0) {
					GxMediaFilter_Config(media->acodec1);
					GxMediaFilter_Config(media->aout);
				}
				av_debug_audio_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_VIDEO(media))
			{
				int freeze = 1;
				av_debug_video_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
				av_debug_video_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				av_debug_video_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				GxMediaFilter_Config(media->vcodec);
				GxMediaFilter_Config(media->vout);
				av_debug_video_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_SUB(media))
			{
				av_debug_subt_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_BEGIN);
				GxMediaFilter_Stop(media->sout);
				GxMediaFilter_Stop(media->scodec);
				av_debug_subt_duty(media->debug, AV_TICK_OP_STOP, AV_TICK_ST_END);

				av_debug_subt_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_BEGIN);
				GxMediaFilter_Config(media->scodec);
				GxMediaFilter_Config(media->sout);
				av_debug_subt_duty(media->debug, AV_TICK_OP_CONFIG, AV_TICK_ST_END);
			}
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESUME_FILL_DATA, NULL);

			media->speed = 1000;
			media->buffering  = 0;
			if (!MEDIA_IS_NETDEMUXER(media)) {
				media->buffering_pause = 0;
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_END_FILL_BUF, PLAYER_ERROR_NO_ERROR);
			}
		}
		GxCore_MutexUnlock(media->mutex);
	}

	return ret;
}

int  GxMedia_SeekResume(GxMedia* media)
{
	if (media)
	{
		GxCore_MutexLock(media->mutex);
		if ((media->state == GX_MEDIA_STATE_RUNNING) ||
				(media->state == GX_MEDIA_STATE_SOON_OVER) ||
				(media->state == GX_MEDIA_STATE_OVER))
		{
			if (media->state == GX_MEDIA_STATE_SOON_OVER ||
					(media->state == GX_MEDIA_STATE_OVER))
				media->state = GX_MEDIA_STATE_RUNNING;

			av_debug_first_avframe_duty(media->debug);
			av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
			GxMediaFilter_Resume(media->demuxer);
			av_debug_demuxer_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);

			av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
			if (!media->seeking)
				GxMediaFilter_Resume(media->stream);
			av_debug_stream_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);

			if (MEDIA_HAS_AUDIO(media))
			{
				av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				GxMediaFilter_Run(media->aout);
				GxMediaFilter_Run(media->acodec);
				GxMediaFilter_Run(media->acodec1);
				av_debug_audio_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_VIDEO(media))
			{
				av_debug_video_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				GxMediaFilter_Run(media->vcodec);
				GxMediaFilter_Run(media->vout);
				av_debug_video_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			if (MEDIA_HAS_SUB(media))
			{
				av_debug_subt_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_BEGIN);
				GxMediaFilter_Run(media->scodec);
				GxMediaFilter_Run(media->sout);
				av_debug_subt_duty(media->debug, AV_TICK_OP_RUN, AV_TICK_ST_END);
			}

			GxMediaFilter_Resume(media->dumper);

			if (MEDIA_IS_NETDEMUXER(media)) {
				media->buffering_pause = 0;
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_END_FILL_BUF, PLAYER_ERROR_NO_ERROR);
			}
		}
		GxCore_MutexUnlock(media->mutex);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

int GxMedia_AudioSync(GxMedia* media, int timems)
{
	if (media)
	{
		return GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SYNC_AUDIO, (void*)&timems);
	}

	return GX_PLAYER_ERROR;
}

int GxMedia_AudioDelay(GxMedia* media, int delayms)
{
	if (media)
		return GxAudioOut_Control(media->aout, GX_AO_CTRL_SET_DELAY, (void*)&delayms);

	return GX_PLAYER_ERROR;
}

int GxMedia_VideoDelay(GxMedia* media, int delayms)
{
	if (media)
		return GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_DELAY, (void*)&delayms);

	return GX_PLAYER_ERROR;
}

int GxMedia_AudioSwitch(GxMedia* media, int pid, AudioCodecType codectype)
{
	int ret;
	GxStreamStreamSwitch  StreamSwitchAudio;
	GxDemuxerStreamSwitch DemuxSwitchAudio;

	if (media) {
		AudioCodecType raw_codectype = media->sh_audio ? media->sh_audio->format : AUDIO_CODEC_UNKNOWN;

		DemuxSwitchAudio.pid = pid;
		StreamSwitchAudio.pid = pid;
		StreamSwitchAudio.type = codectype;
		DemuxSwitchAudio.type = codectype;
		DemuxSwitchAudio.runflag = 0;

		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_PAUSE_FILL_DATA, NULL);
		GxMediaFilter_Stop(media->aout);
		if (MEDIA_HAS_AUDIO1(media)) {
			int mix = (0x1 << 0);

			GxAudioOut_Control(media->aout, GX_AO_CTRL_SET_PCM_MIX, &mix);
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_STOP_AD_AUDIO, NULL);
			GxMediaFilter_Stop(media->acodec1);
			GxMediaFilter_Stop(media->acodec);
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_CLOSE_AD_AUDIO, NULL);
			GxAudioDecoder_Close(media->acodec1);
			media->aout->header1 = NULL;
			media->sh_audio1 = NULL;
			media->acodec1   = NULL;
		} else
			GxMediaFilter_Stop(media->acodec);

		av_memhole_flush();
		GxStream_Control(media->stream, GX_STREAM_CTRL_SWITCH_AUDIO, &StreamSwitchAudio);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SWITCH_AUDIO, &DemuxSwitchAudio);
		if (GxAudioDecoder_CheckReOpen(raw_codectype, codectype)) {
			GxAudioDecoder_Close(media->acodec);
			media->sh_audio = media->demuxer->audio->sh;
			media->acodec   = GxAudioDecoder_Open(media->sh_audio);
		} else
			media->sh_audio = media->demuxer->audio->sh;
		media_switch_audio(media);

		if (MEDIA_HAS_AUDIO(media)) {
			GxCore_MutexLock(media->mutex);
			if (DemuxSwitchAudio.runflag ||
					media->state == GX_MEDIA_STATE_RUNNING) {
				ret = GxMediaFilter_Config(media->acodec);
				if (ret < 0) {
					GxMedia_AudioUnsupport(media);
					GxCore_MutexUnlock(media->mutex);
					GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESUME_FILL_DATA, NULL);
					return ret;
				}
				else {
					if (MEDIA_HAS_VIDEO(media)) {
						GxStreamAudioHeader* sh = media->demuxer->audio->sh;
						sh->priv.pts_sync = 1;
						GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_SYNC, &sh->priv.pts_sync);
					}
					if (media->errInfo == PLAYER_ERROR_AUDIO_DECODER_ERROR)
						media->errInfo = PLAYER_ERROR_NO_ERROR;
				}

				GxMediaFilter_Config(media->aout);
				GxMediaFilter_Abort(media->acodec);
				GxMediaFilter_Run(media->acodec);
				GxMediaFilter_Run(media->aout);

			}
			GxCore_MutexUnlock(media->mutex);
		}
		media->sh_audio = media->demuxer->audio->sh;
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESUME_FILL_DATA, NULL);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

int GxMedia_SubSync(GxMedia* media, int timems)
{
	if (media)
	{
		return GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SYNC_SUB, (void*)&timems);
	}

	return GX_PLAYER_ERROR;
}

int GxMedia_SubSwitch(GxMedia* media, int pid)
{
	if (media)
	{
		GxStreamSubHeader* sh_sub = NULL;
		GxStreamSubCodecType old_type = 0;
		GxDemuxerStreamSwitch StreamSwitch = {pid, 0};

		sh_sub = media->demuxer->sub->sh;
		if (sh_sub == NULL)
			return GX_PLAYER_ERROR;

		old_type = sh_sub->type;
		if (GX_PLAYER_OK == GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SWITCH_SUB, &StreamSwitch)) {
			sh_sub =  media->demuxer->sub->sh;
			if (sh_sub) {
				if (sh_sub->type != old_type) {
					GxMediaFilter_Stop(media->sout);
					GxSubOut_Close(media->sout);
					GxMediaFilter_Stop(media->scodec);
					GxSubDecoder_Close(media->scodec);

					media->scodec = GxSubDecoder_Open(sh_sub);
					if (media->scodec == NULL) {
						GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
						MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_DECODER);
					}

					GxSubDecoder_Control(media->scodec, GX_SD_CTRL_SWITCH_HEADER, media->demuxer->sub->sh);
					media->sout = GxSubOut_Open(sh_sub,media_get_current_cb,media);
					if (media->sout == NULL) {
						GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
						MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_RENDER);
					}

					GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_SO, media->scodec, GX_SD_PIN_NAME_IN);
					GxMediaFilter_Connect(media->scodec, GX_SD_PIN_NAME_OUT, media->sout, GX_SO_PIN_NAME_IN);

					GxMediaFilter_Run(media->scodec);
					GxSubOut_Control(media->sout, GX_SO_CTRL_SET_PID, &pid);
					GxMediaFilter_Run(media->sout);
				} else {
					GxSubOut_Control(media->sout, GX_SO_CTRL_SET_PID, &pid);
				}

			}
			media->sh_sub = sh_sub;
			return GX_PLAYER_OK;
		}
	}
	return GX_PLAYER_ERROR;
}

void GxMedia_FlushFrameInfo(GxMedia* media)
{
	if (media->acodec)
	{
		GxAudioDecoder_Control(media->acodec, GX_AD_CTRL_GET_FRAME_INFO, NULL);
	}

	if (media->vcodec)
	{
		GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_FRAME_INFO, NULL);
	}

	if (media->aout)
	{
		GxAudioOut_Control(media->aout, GX_AO_CTRL_GET_FRAME_INFO, NULL);
	}
}

static AVCodecSyncState _switch_sync_state(GxAvSyncFrameState synced_state)
{
	AVCodecSyncState state = AVCODEC_SYNC_FREERUN;

	switch (synced_state) {
	case GXAV_FRAME_PLAY_NORMAL:
		state = AVCODEC_SYNC_NORMAL;
		break;
	case GXAV_FRAME_PLAY_REPEAT:
		state = AVCODEC_SYNC_REPEAT;
		break;
	case GXAV_FRAME_PLAY_SKIP:
		state = AVCODEC_SYNC_SKIP;
		break;
	case GXAV_FRAME_PLAY_FREERUN:
		state = AVCODEC_SYNC_FREERUN;
		break;
	default:
		break;
	}

	return state;
}

static int GxMedia_GetNetworkUrlOptionsTimeoutMs(GxStream *stream, int def_timeoutms)
{
	int timeoutms = def_timeoutms;
	char *options_value = NULL;
	if (stream && stream->options && (options_value = GxOptions_Get_By_Name(stream->options, " -H", "timeout:"))) {
		timeoutms = strtol(options_value, NULL, 10)*1000;
	}
	return timeoutms;
}

void GxMedia_GetCodecStatus(GxMedia* media,
		AVCodecStatus* acodec, AVCodecStatus* vcodec,
		AVCodecSyncStatus *async, AVCodecSyncStatus *vsync)
{
	if (media) {
		AVCodecStatus astatus;
		AVCodecStatus vstatus;
		int timeoutms = 10000;

		if (MEDIA_IS_NETDEMUXER(media) || MEDIA_IS_NETSTREAM(media)) {
			GxPlayer_SystemGet(PSYS_NETWORK_TIMEOUT, &timeoutms);
			if (MEDIA_IS_NETDEMUXER(media)) {
				timeoutms = GxMedia_GetNetworkUrlOptionsTimeoutMs(media->stream, timeoutms);
			}
		}

		astatus.state = AVCODEC_STOPPED;
		astatus.err_code = AVCODEC_ERR_NONE;
		vstatus.state = AVCODEC_STOPPED;
		vstatus.err_code = AVCODEC_ERR_NONE;

		GxMedia_FlushFrameInfo(media);

		if (MEDIA_HAS_AUDIO(media)) {
			GxStreamAudioHeader *sh = media->sh_audio;

			GxAudioDecoder_Control(media->acodec, GX_AD_CTRL_GET_STATUS, &astatus);
			if (sh->stat.decode_frame_cnt > 0)
				av_debug_frist_audio_frame_duty(media->debug, sh->stat.decode_frame_cnt);

			if (sh->stat.decode_frame_cnt != media->aframe.last_frame_cnt) {
				media->aframe.last_frame_cnt = sh->stat.decode_frame_cnt;
				GxCore_GetTickTime(&media->aframe.last_frame_time);
			}
			else {
				GxTime cur_time;
				GxCore_GetTickTime(&cur_time);
				if (GX_SPEED_NORMAL(media->speed) &&
						(astatus.state == AVCODEC_RUNNING ||
						 astatus.state == AVCODEC_READY)) {
					if (TIME_MINUS(cur_time, media->aframe.last_frame_time) >= timeoutms) {
						astatus.state = AVCODEC_TIMEOUT;
					}
				} else
					media->aframe.last_frame_time = cur_time;
			}
			if (async) {
				async->sync_flag  = sh->stat.synced_flag;
				async->sync_state = _switch_sync_state(sh->stat.synced_state);
			}
		}

		if (MEDIA_HAS_VIDEO(media)) {
			GxStreamVideoHeader *sh = media->sh_video;

			GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_STATUS, &vstatus);
			if (sh->stat.decode_frame_cnt > 0)
				av_debug_frist_video_frame_duty(media->debug, sh->stat.decode_frame_cnt);

			if (sh->stat.decode_frame_cnt != media->vframe.last_frame_cnt) {
				media->vframe.last_frame_cnt = sh->stat.decode_frame_cnt;
				GxCore_GetTickTime(&media->vframe.last_frame_time);
			}
			else {
				GxTime cur_time;
				GxCore_GetTickTime(&cur_time);
				if (GX_SPEED_NORMAL(media->speed) &&
						(vstatus.state == AVCODEC_RUNNING ||
						 vstatus.state == AVCODEC_READY ||
						 vstatus.state == AVCODEC_STARTED)) {
					if (TIME_MINUS(cur_time, media->vframe.last_frame_time) >= timeoutms) {
						vstatus.state = AVCODEC_TIMEOUT;
					}
				} else
					media->vframe.last_frame_time = cur_time;
			}
			if (vsync) {
				vsync->sync_flag  = sh->stat.synced_flag;
				vsync->sync_state = vsync->sync_flag ? AVCODEC_SYNC_NORMAL: AVCODEC_SYNC_FREERUN;
			}
		}

		if (MEDIA_IS_NETDEMUXER(media)) {
			GxTime cur_time;
			GxCore_GetTickTime(&cur_time);
			if (media->buffering && MEDIA_IS_NOT_FILLPACKET(media)) {
				if (TIME_MINUS(cur_time, media->packet.last_frame_time) >= timeoutms) {
					if (MEDIA_HAS_AUDIO(media))
						astatus.state = AVCODEC_TIMEOUT;
					if (MEDIA_HAS_VIDEO(media))
						vstatus.state = AVCODEC_TIMEOUT;
				}
			} else
				media->packet.last_frame_time = cur_time;
		}

		if (acodec)
			*acodec = astatus;

		if (vcodec)
			*vcodec = vstatus;

		if (!MEDIA_IS_DVB(media) && GX_SPEED_NORMAL(media->speed)) {
			if (((astatus.state == AVCODEC_TIMEOUT) && (vstatus.state == AVCODEC_TIMEOUT)) ||
					((astatus.state == AVCODEC_STOPPED) && (vstatus.state == AVCODEC_TIMEOUT)) ||
					((astatus.state == AVCODEC_TIMEOUT) && (vstatus.state == AVCODEC_STOPPED))) {
				GxCore_MutexLock(media->mutex);
				if (media->state == GX_MEDIA_STATE_RUNNING ||
						media->state == GX_MEDIA_STATE_SOON_OVER) {
					gxlogi("[Media]: AVCodec Timeout ...\n");
					GxDemuxer_DebugData(media->demuxer);
					if (MEDIA_IS_NETDEMUXER(media)) {
						if (media->demuxer->seekable) {
							uint64_t cur_timems = 0;
							uint64_t tol_timems = 0;

							GxMedia_GetCurrentTime(media, &cur_timems);
							tol_timems = GxMedia_GetDuration(media);
							if ((tol_timems > 0) && (cur_timems + 3000 >= tol_timems)) {
								MEDIA_EXEC_EVENT(media, PLAYER_STATUS_PLAY_END, media->errInfo);
							} else {
								MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_SOURCE_RESTART);
							}
						} else {
							MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_SOURCE_RESTART);
						}
					} else {
						MEDIA_EXEC_EVENT(media, PLAYER_STATUS_PLAY_END, media->errInfo);
					}
					media->state = GX_MEDIA_STATE_OVER;
				}
				GxCore_MutexUnlock(media->mutex);
			}
		}
	}
}

int GxMedia_GetCurAudioPts(GxMedia* media)
{
	int apts = 0;
	GxAudioOut_Control(media->aout, GX_AO_CTRL_GET_CUR_PTS, &apts);
	return apts;
}

void GxMedia_AVPrintf(GxMedia* media)
{
	if (!media) return;
	static char *_stc_recovery_string[] = {"avpts", "vpts", "apts", "pcr", "pure.apts", "free", "fix.apts", "fix.vpts"};

	if (MEDIA_HAS_AUDIO(media) && MEDIA_HAS_VIDEO(media)) {
		int apts = 0, asize = 0;
		int vpts = 0, vsize = 0, vdpts = 0;
		int stc_freq  = GxDemuxer_GetSTCFreq(media->demuxer);
		int stc_mode = GxDemuxer_GetSTCSyncMode(media->demuxer);

		GxAudioOut_Control    (media->aout,    GX_AO_CTRL_GET_CUR_PTS,     &apts);
		GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_CUR_PTS,     &vdpts);
		GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_DISPLAY_PTS, &vpts);
		GxDemuxer_GetEsBufSize(media->demuxer, &asize, &vsize);
		gxlogi_raw("[av]: stc - %s, pts - %u %u, disp offset %-4d sync offset: %-4d, size - %-6d %-7d\n",
				(stc_mode == 0xff) ? "dly" : _stc_recovery_string[stc_mode],
				apts, vpts, (apts - vpts) * 1000 / stc_freq, (apts - vdpts) * 1000 / stc_freq, asize, vsize);
	} else if (MEDIA_HAS_AUDIO(media)) {
		int apts = 0, asize = 0;
		int stc_mode = GxDemuxer_GetSTCSyncMode(media->demuxer);

		GxAudioOut_Control    (media->aout,    GX_AO_CTRL_GET_CUR_PTS, &apts);
		GxDemuxer_GetEsBufSize(media->demuxer, &asize, NULL);
		gxlogi_raw("[ a]: stc - %s, pts - %u, size - %-6d\n",
				(stc_mode == 0xff) ? "dly" : _stc_recovery_string[stc_mode], apts, asize);
	} else if (MEDIA_HAS_VIDEO(media)) {
		int vpts = 0, vsize = 0;
		int stc_mode = GxDemuxer_GetSTCSyncMode(media->demuxer);

		GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_DISPLAY_PTS, &vpts);
		GxDemuxer_GetEsBufSize(media->demuxer, NULL, &vsize);
		gxlogi_raw("[ v]: stc - %s, pts - %u size - %-7d\n",
				(stc_mode == 0xff) ? "dly" : _stc_recovery_string[stc_mode], vpts, vsize);
	}

	return;
}

void GxMedia_DetectVideoPtsDiff(GxMedia* media)
{
	int stc_mode = 0, video_delay_lms = 0, video_delay_hms = 0,  video_delay_fms = 10000;

	if (!(MEDIA_IS_NETSTREAM(media) || MEDIA_IS_NETDEMUXER(media)))
		return;
	if (!(media->demuxer && media->demuxer->stream && media->demuxer->stream->nobuffer)) /*only support nocache.*/
		return ;

	GxPlayer_SystemGet(PSYS_NETWORK_SCREEN_TO_DEMUX_PTS_DIFFERENCE, &video_delay_lms);
	video_delay_hms = video_delay_lms + 250;
	if (media->demuxer->stream->screen_to_dmx_pts_lms > 0)
		video_delay_lms = media->demuxer->stream->screen_to_dmx_pts_lms;
	if ((media->demuxer->stream->screen_to_dmx_pts_hms > 0) && (media->demuxer->stream->screen_to_dmx_pts_hms > video_delay_lms))
		video_delay_hms = media->demuxer->stream->screen_to_dmx_pts_hms;

	stc_mode = GxDemuxer_GetSTCSyncMode(media->demuxer);
	if (VPTS_RECOVER != stc_mode) /*only support video master.*/
		return;

	if (MEDIA_HAS_AUDIO(media) && MEDIA_HAS_VIDEO(media)) {
		int v_disp_pts = 0, esv_time = 0, cur_codec_speed_factor = 0, set_codec_speed_factor = 1, is_set_speed_factor = 0;

		cur_codec_speed_factor = media->cur_codec_speed_factor;
		GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_DISPLAY_PTS, &v_disp_pts);

		if (v_disp_pts != -1) {
			v_disp_pts = v_disp_pts/media->demuxer->stc_factor;
			if (media->demuxer->video->fill_pkt_pts > v_disp_pts)
				esv_time = (media->demuxer->video->fill_pkt_pts - v_disp_pts);
		}
		esv_time = (esv_time>10000)?0:esv_time;

		if (esv_time > 0 && esv_time < video_delay_lms) {
			if (cur_codec_speed_factor != 1) {
				is_set_speed_factor = 1;
				set_codec_speed_factor = 1;
			}
		} else if ((esv_time >= video_delay_hms) && (esv_time < video_delay_fms)) {
			if (cur_codec_speed_factor != 2) {
				is_set_speed_factor = 1;
				set_codec_speed_factor = 2;
			}
		} else if ((esv_time >= video_delay_lms) && (esv_time < video_delay_hms)) {

		} else {
			if (cur_codec_speed_factor != 1) {
				is_set_speed_factor = 1;
				set_codec_speed_factor = 1;
			}
		}
		if (is_set_speed_factor) {
			media->cur_codec_speed_factor = set_codec_speed_factor;
			set_codec_speed_factor = set_codec_speed_factor * 1000;
			GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_SET_XSPEED, &set_codec_speed_factor);
		}
	}
	return;
}

void GxMedia_GetDataLength(GxMedia* media, unsigned int * adata, unsigned int * vdata)
{
	if (media)
	{
		if (adata)
		{
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_AUDIO_LENGTH, adata);
		}

		if (vdata)
		{
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_VIDEO_LENGTH, vdata);
		}
	}
}

void GxMedia_SwitchUrl(GxMedia* media, const char* url)
{
	if (media && url)
	{
		GxStream_Control(media->stream, GX_STREAM_CTRL_CHANGE_URL, (void *)url);
	}
}

status_t GxMedia_GetCurrentTime(GxMedia* media, uint64_t *cur)
{
	if (media && media->demuxer) {
		int aframe = 10, vframe = 10;
		int availd = 0, vvaild = 0;

		GxMedia_FlushFrameInfo(media);

		if (MEDIA_HAS_AUDIO(media) && !NEEDDROP(media->demuxer->audio->dropmode)) {
			GxStreamAudioHeader *sh = media->sh_audio;
			aframe = sh->stat.play_frame_cnt;
			availd = 1;
		}

		if (MEDIA_HAS_VIDEO(media) && !NEEDDROP(media->demuxer->video->dropmode)) {
			GxStreamVideoHeader *sh = media->sh_video;
			vframe = sh->stat.play_frame_cnt;
			vvaild = 1;
		}

		if (availd == 0 && vvaild == 0)
			return GX_PLAYER_ERROR;

		if (media->speed == 1000) {
			if ((aframe + vframe >= 100) || (aframe >= 10 && vframe >= 10) || MEDIA_IS_HWTS(media)) {
				*cur = GxDemuxer_GetCurrentTime(media->demuxer);
				return GX_PLAYER_OK;
			}
		}
		else {
			*cur = GxDemuxer_GetCurrentTime(media->demuxer);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

uint64_t GxMedia_GetSeekMinTime(GxMedia* media)
{
	if (media && media->demuxer)
	{
		return GxDemuxer_GetSeekMinTime(media->demuxer);
	}

	return 0;
}

int32_t GxMedia_GetCacheInfo(GxMedia *media, PlayerNetInfo* info)
{
	if (media && media->stream) {
		if (GX_STREAMTYPE_DEMUXER != media->stream->file_format) {
			info->net_speed          = media->stream->info.net_speed;
			info->buf_percent        = media->stream->info.buf_percent;
			info->cache_buf_size     = media->stream->info.cache_buf_size;
			info->cache_buf_percent  = media->stream->info.cache_buf_percent;
			info->cache_back_size    = media->stream->info.cache_back_size;
			info->cache_back_percent = media->stream->info.cache_back_percent;
		} else {
			if (media->demuxer) {
				unsigned int percent     = GxDemuxer_GetInBufPercent(media->demuxer);

				info->net_speed          = media->demuxer->fill_pkt_speed;
				info->buf_percent        = percent;
				info->cache_buf_size     = GxDemuxer_GetInBufSize(media->demuxer);
				info->cache_buf_percent  = percent;
				info->cache_back_size    = 0;
				info->cache_back_percent = 0;
			}
		}
	}

	return 0;
}

int32_t GxMedia_GetCacheTime(GxMedia *media)
{
	int32_t cache_time = 0;
	int8_t  aflag = 0, vflag = 0;

	if (media)
	{
		int32_t audio_es_time = 0, video_es_time = 0;
		int32_t audio_es_bitrate = 0, video_es_bitrate = 0;
		int32_t audio_stream_bitrate = 0, video_stream_bitrate = 0;

		GxMedia_FlushFrameInfo(media);

		if (MEDIA_HAS_AUDIO(media) && media->demuxer
				&& (!NEEDDROP(media->demuxer->audio->dropmode)
					&& !NEEDDROPSWITCH(media->demuxer->audio->dropmode))) {
			GxStreamAudioHeader *sh = media->sh_audio;

			if (sh->stat.play_frame_cnt > 0) {
				int32_t apts = -1;

				GxAudioOut_Control(media->aout, GX_AO_CTRL_GET_CUR_PTS, &apts);
				if (apts != -1) {
					if(media->demuxer->type == GX_DEMUXER_TYPE_HW)
						apts = apts/(media->demuxer->stcfreq/1000);
					else
						apts = apts/media->demuxer->stc_factor;

					if (media->demuxer->audio->fill_pkt_pts > apts)
						audio_es_time = (media->demuxer->audio->fill_pkt_pts - apts);
				}
			} else
				audio_es_time = media->demuxer->audio->fill_pkt_pts;

			audio_es_bitrate     = media->demuxer->audio->compute_es_bitrate;
			audio_stream_bitrate = media->demuxer->audio->compute_stream_bitrate;
			aflag = 1;
		}

		if (MEDIA_HAS_VIDEO(media) && media->demuxer
				&& !NEEDDROP(media->demuxer->video->dropmode)) {
			GxStreamVideoHeader *sh = media->sh_video;

			if (sh->stat.play_frame_cnt > 0) {
				int32_t vpts = -1;

				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_CUR_PTS, &vpts);
				if (vpts != -1) {
					if(media->demuxer->type == GX_DEMUXER_TYPE_HW)
						vpts = vpts/(media->demuxer->stcfreq/1000);
					else
						vpts = vpts/media->demuxer->stc_factor;

					if (media->demuxer->video->fill_pkt_pts > vpts)
						video_es_time = (media->demuxer->video->fill_pkt_pts - vpts);
				}
			} else
				video_es_time = media->demuxer->video->fill_pkt_pts;

			video_es_bitrate     = media->demuxer->video->compute_es_bitrate;
			video_stream_bitrate = media->demuxer->video->compute_stream_bitrate;
			vflag = 1;
		}

		if (aflag && vflag)
			cache_time = (audio_es_time > video_es_time) ? video_es_time : audio_es_time;
		else if (aflag)
			cache_time = audio_es_time;
		else if (vflag)
			cache_time = video_es_time;

		if (media->demuxer) {
			int es_size    = GxDemuxer_GetInBufSize(media->demuxer);
			int es_bitrate = (audio_es_bitrate + video_es_bitrate);

			if (es_bitrate > 0) {
				int es_time = 1000 * ((int64_t)es_size * 8) / es_bitrate;
				if (cache_time >= (2 * es_time))
					cache_time = es_time;
			} else
				cache_time = 0;
		}

		if (media->stream && media->demuxer) {
			if (media->stream->info.cache_data_len > 0) {
				int stream_bitrate = 0;

				if (video_stream_bitrate > 0)
					stream_bitrate = video_stream_bitrate;
				else if (audio_stream_bitrate > 0)
					stream_bitrate = audio_stream_bitrate;
				else if (media->demuxer->duration > 0)
					stream_bitrate = 1000 * (8 * (int64_t)media->stream->end_pos) / (int64_t)media->demuxer->duration;

				if (stream_bitrate > 0) {
					cache_time += 1000 * ((int64_t)media->stream->info.cache_data_len * 8) / (int64_t)stream_bitrate;
				}
			}
		}
	}

	return cache_time;
}

void GxMedia_GetSyncInfo(GxMedia* media, PlayerSyncInfo* syncinfo)
{
	if(media)
	{
		int stc_freq  = GxDemuxer_GetSTCFreq(media->demuxer);

		if (MEDIA_HAS_AUDIO(media) && MEDIA_HAS_VIDEO(media)) {
			GxAudioOut_Control    (media->aout,    GX_AO_CTRL_GET_CUR_PTS,     &syncinfo->apts);
			GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_CUR_PTS,     &syncinfo->vpts);
			syncinfo->apts        = syncinfo->apts / (stc_freq / 1000);
			syncinfo->vpts        = syncinfo->vpts / (stc_freq / 1000);
		} else if (MEDIA_HAS_AUDIO(media)) {
			GxAudioOut_Control    (media->aout,    GX_AO_CTRL_GET_CUR_PTS,     &syncinfo->apts);
			syncinfo->apts        = syncinfo->apts / (stc_freq / 1000);
			syncinfo->vpts        = -1;
		} else if (MEDIA_HAS_VIDEO(media)) {
			GxVideoDecoder_Control(media->vcodec,  GX_VD_CTRL_GET_CUR_PTS,     &syncinfo->vpts);
			syncinfo->apts        = -1;
			syncinfo->vpts        = syncinfo->vpts / (stc_freq / 1000);
		} else {
			syncinfo->apts        = -1;
			syncinfo->vpts        = -1;
		}
	}
}

int32_t GxMedia_GetCurrentPts(GxMedia* media)
{
	if (media)
	{
		int32_t apts = -1,  vpts = -1, stc = -1;
		int32_t availd = 0, vvaild = 0;

		GxMedia_FlushFrameInfo(media);

		if (MEDIA_HAS_AUDIO(media) && media->demuxer
				&& (!NEEDDROP(media->demuxer->audio->dropmode)
					&& !NEEDDROPSWITCH(media->demuxer->audio->dropmode))) {
			GxStreamAudioHeader *sh = media->sh_audio;

			if (sh->stat.play_frame_cnt > 0) {
				GxAudioOut_Control(media->aout, GX_AO_CTRL_GET_CUR_PTS, &apts);
				if (apts != -1) {
					if(media->demuxer->type == GX_DEMUXER_TYPE_HW)
						apts = apts/(media->demuxer->stcfreq/1000);
					else
						apts = apts/media->demuxer->stc_factor;
				} else
					apts = -1;

				availd = 1;
			}
		}

		if (MEDIA_HAS_VIDEO(media) && media->demuxer && !NEEDDROP(media->demuxer->video->dropmode)) {
			GxStreamVideoHeader *sh = media->sh_video;

			if (sh->stat.play_frame_cnt > 0) {
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_CUR_PTS, &vpts);
				if (vpts != -1) {
					if(media->demuxer->type == GX_DEMUXER_TYPE_HW)
						vpts = vpts/(media->demuxer->stcfreq/1000);
					else
						vpts = vpts/media->demuxer->stc_factor;
				} else
					vpts = -1;
				vvaild = 1;
			}
		}

		if (!availd && !vvaild && media->demuxer)
			stc = GxDemuxer_GetCurrentSTC(media->demuxer);

		if (availd)
			return (apts!=-1)?apts:0;
		else if (vvaild)
			return vpts;
		else
			return stc;
	}
	return 0;
}

uint8_t GxMedia_GetCurrentPercent(GxMedia* media)
{
	if (media && media->demuxer)
	{
		return GxDemuxer_GetCurrentPercent(media->demuxer);
	}

	return 0;
}
uint64_t GxMedia_GetDuration(GxMedia* media)
{
	if (media && media->demuxer)
	{
		return GxDemuxer_GetTimeLength(media->demuxer);
	}

	return 0;
}

void GxMedia_GetTSRecordInfo(GxMedia *media, PlayerRecordConfig* config)
{
	if (media && media->demuxer)
	{
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_TSRECORDINFO, config);
	}
}

void GxMedia_SetPlayTsFilterConfig(GxMedia* media, GxDemuxerPlayTsExtInfo* config)
{
	if (media && media->demuxer)
	{
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_PLAY_TS_CONFIG, config);
	}
}

void GxMedia_SetPlayTsCacheConfig(GxMedia* media, GxStreamTrackCache* config)
{
	if (media && media->stream)
	{
		GxStream_Control(media->stream, GX_STREAM_CTRL_TS_CACHE_CONFIG, config);
	}
}

void GxMedia_SetRecordConfig(GxMedia* media, PlayerRecordConfig* config, PlayerRecordEncrypt *encrypt)
{
	memcpy(&media->recconfig, config, sizeof(GxDemuxerRecordExtInfo));

	if (media && media->demuxer)
	{
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_RECORD_CONFIG,    &config->ext_info);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_RECORD_USER_INFO, &config->ext_data);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_RECORD_ENCRYPT,   encrypt);
	}
}

unsigned int GxMedia_RecordCheckOverflow(GxMedia* media)
{
	int ret = 0;
	if (media && media->demuxer)
	{
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_CHECK_RECORD_OVERFLOW, &ret);
		return ret;
	}

	return 0;
}

unsigned int GxMedia_GetTSProgStreamInfo(GxMedia* media, PlayerProgStreamInfo *info)
{
	int ret = GX_PLAYER_ERROR;

	if (media && media->demuxer)
		ret = GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_PROG_STREAM_INFO, info);

	return ret;
}

unsigned int GxMedia_VideoStartDecord(GxMedia* media)
{
	if (media && media->vcodec && media->video_saved == 0)
	{
		AVCodecStatus CodecStatus = {0};
		GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_STATUS, &CodecStatus);

		if (CodecStatus.state == AVCODEC_READY) {
			int ok = 0;
			int ret = GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_START_DECODE, &ok);
			if (ret != 0 && media->errInfo != PLAYER_ERROR_VIDEO_DECODER_ERROR) {
				return GxMedia_VideoUnsupport(media);
			}
			if (ok) {
				av_debug_start_video_decode_duty(media->debug);
			}
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_VideoCheckDynamicRangeChange(GxMedia* media)
{
	int ret = GX_PLAYER_ERROR;

	if (media && media->vcodec)
		ret = GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_CHECK_DYNAMIC_RANGE_CHANGE, NULL);

	return ret;
}

unsigned int GxMedia_VideoStartDisplay(GxMedia* media)
{
	if (media && media->vcodec && media->video_saved == 0 && media->video_showing == 0)
	{
		int displayed_first_frame = 0;
		GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_CHECK_FRAME_OK, &displayed_first_frame);
		if (displayed_first_frame == 1) {
			av_debug_frist_video_show_duty(media->debug);
			GxMediaFilter_Run(media->vout);
			GxStream_Control(media->stream, GX_STREAM_CTRL_VIDEO_READY, NULL);
			media->video_showing = 1;
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_VideoReset(GxMedia* media, int freeze)
{
	if (media && media->video_saved == 0) {
		GxMediaFilter_Pause(media->vcodec);
		GxMediaFilter_Pause(media->vout);
		GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESET_VIDEO, NULL);
		GxMediaFilter_Reset(media->vcodec);
		GxMediaFilter_Reset(media->vout);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_AudioReset(GxMedia* media)
{
	if (media && media->audio_saved == 0) {
		GxMediaFilter_Pause(media->acodec);
		if (MEDIA_HAS_AUDIO1(media))
			GxMediaFilter_Pause(media->acodec1);
		GxMediaFilter_Pause(media->aout);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESET_AUDIO, NULL);
		if (MEDIA_HAS_AUDIO1(media))
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESET_AD_AUDIO, NULL);
		GxMediaFilter_Reset(media->acodec);
		if (MEDIA_HAS_AUDIO1(media))
			GxMediaFilter_Reset(media->acodec1);
		GxMediaFilter_Reset(media->aout);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_AudioReset1(GxMedia* media)
{
	if (media) {
		if (MEDIA_HAS_AUDIO(media))
			GxAudioDecoder_Control(media->acodec, GX_AD_CTRL_RESET, NULL);
		if (MEDIA_HAS_AUDIO1(media))
			GxAudioDecoder_Control(media->acodec1, GX_AD_CTRL_RESET, NULL);
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_Reset(GxMedia* media, int freeze)
{
	if(media)
	{
		//stop
		{
			if (MEDIA_HAS_AUDIO(media)) {
				GxMediaFilter_Stop(media->acodec);
				if (MEDIA_HAS_AUDIO1(media))
					GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->aout);
			}
			if (MEDIA_HAS_VIDEO(media)) {
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
			}
			if (MEDIA_HAS_AUDIO1(media))
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_STOP_AD_AUDIO, NULL);
			GxMediaFilter_Stop(media->demuxer);
		}

		//config
		{
			GxMediaFilter_Config(media->demuxer);
			if (MEDIA_HAS_AUDIO(media)) {
				GxMediaFilter_Config(media->acodec);
				if (MEDIA_HAS_AUDIO1(media))
					GxMediaFilter_Config(media->acodec1);
				GxMediaFilter_Config(media->aout);
			}
			if (MEDIA_HAS_VIDEO(media)) {
				GxMediaFilter_Config(media->vcodec);
				GxMediaFilter_Config(media->vout);
			}
		}

		//run
		{
			GxMediaFilter_Run(media->demuxer);
			if (MEDIA_HAS_AUDIO1(media)) {
				GxStreamAudioHeader* sh1 = media->sh_audio1;
				GxDemuxerStreamSwitch DemuxSwitchAudio;

				DemuxSwitchAudio.pid  = sh1->priv.id;
				DemuxSwitchAudio.type = sh1->format;
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RUN_AD_AUDIO, &DemuxSwitchAudio);
			}
			if (MEDIA_HAS_AUDIO(media)) {
				GxMediaFilter_Run(media->aout);
				GxMediaFilter_Run(media->acodec);
				if (MEDIA_HAS_AUDIO1(media))
					GxMediaFilter_Run(media->acodec1);
			}
			if (MEDIA_HAS_VIDEO(media)) {
				GxMediaFilter_Run(media->vcodec);
				GxMediaFilter_Run(media->vout);
			}
		}

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_VideoUnsupport_Really(GxMedia* media)
{
	int check_gate = 3;

	if(media && MEDIA_HAS_VIDEO(media)) {
		GxStreamVideoHeader* sh = media->demuxer->video->sh;
#if 0
		GxStreamVideoHeader  tmp_sh = {0};

		if (media->vcodec) {
			GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_GET_FRAME_INFO, &tmp_sh);
		}

		if(sh->wh_check.inited == 0) {
			sh->wh_check.w = tmp_sh.disp_w;
			sh->wh_check.h = tmp_sh.disp_h;
			sh->wh_check.cnt    = 0;
			sh->wh_check.inited = 1;
		} else if(tmp_sh.disp_w != sh->wh_check.w || tmp_sh.disp_w != sh->wh_check.w) {
			sh->wh_check.w = tmp_sh.disp_w;
			sh->wh_check.h = tmp_sh.disp_h;
			sh->wh_check.cnt = 0;
		} else {
			sh->wh_check.cnt++;
		}
#endif

		return ++sh->wh_check.cnt >= check_gate;
	}

	return 0;
}

unsigned int GxMedia_VideoUnsupport(GxMedia* media)
{
	if (media) {
		if (!media->errorstop && MEDIA_HAS_AUDIO(media) && media->errInfo != PLAYER_ERROR_AUDIO_DECODER_ERROR) {
			int freeze = 0, dropmode = DROPMODE_UNSUPPORT;
			GxStreamAudioHeader* sh = media->demuxer->audio->sh;
			sh->priv.pts_sync = 0;
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE, &dropmode);
			GxAudioOut_Control(media->aout, GX_AO_CTRL_SET_SYNC, &sh->priv.pts_sync);
			GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
			GxMediaFilter_Stop(media->vcodec);
			GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
			GxMediaFilter_Stop(media->vout);
			media->errInfo = PLAYER_ERROR_VIDEO_DECODER_ERROR;
			return GX_PLAYER_OK;
		} else {
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_VIDEO_DECODER_ERROR);
			return GX_PLAYER_ERROR;
		}
	}

	return GX_PLAYER_ERROR;
}

unsigned int GxMedia_AudioUnsupport(GxMedia* media)
{
	if (media) {
		if (!media->errorstop && MEDIA_HAS_VIDEO(media) && media->errInfo != PLAYER_ERROR_VIDEO_DECODER_ERROR) {
			int dropmode = DROPMODE_UNSUPPORT;
			GxStreamVideoHeader* sh = media->demuxer->video->sh;
			sh->priv.pts_sync = 0;
			GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE, &dropmode);
			GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_SET_SYNC, &sh->priv.pts_sync);
			media->errInfo  = PLAYER_ERROR_AUDIO_DECODER_ERROR;
			return GX_PLAYER_OK;
		}
		else {
			MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR, PLAYER_ERROR_AUDIO_DECODER_ERROR);
			return GX_PLAYER_ERROR;
		}
	}

	return GX_PLAYER_ERROR;
}

status_t GxMedia_AdAudioEnable(GxMedia* media, int pid, AudioCodecType codectype)
{
	if (media) {
		if (MEDIA_HAS_AUDIO(media) && !NEEDDROP(media->demuxer->audio->dropmode)) {
			if (!MEDIA_HAS_AUDIO1(media)) {
				GxDemuxerStreamSwitch DemuxSwitchAudio0;
				GxDemuxerStreamSwitch DemuxSwitchAudio1;

				DemuxSwitchAudio1.pid  = pid;
				DemuxSwitchAudio1.type = codectype;

				if (!acodec_check_ad_enable(codectype, media->sh_audio->format))
					return GX_PLAYER_ERROR;

				GxMediaFilter_Stop(media->acodec);
				GxMediaFilter_Stop(media->aout);
				DemuxSwitchAudio0.pid  = media->sh_audio->priv.id;
				DemuxSwitchAudio0.type = media->sh_audio->format;
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SWITCH_AUDIO, &DemuxSwitchAudio0);

				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_OPEN_AD_AUDIO, &DemuxSwitchAudio1);
				media->sh_audio1 = media->demuxer->audio1->sh;
				if (media->sh_audio1) {
					GxPin* pin = NULL;

					media->acodec1 = GxAudioDecoder_Open(media->sh_audio1);
					media->aout->header1 = media->demuxer->audio1->sh;
					pin = GxMediaFilter_FindPin(media->aout, GX_AO_PIN_NAME_ADPCM_IN);
					if (!pin)
						GxPin_Create(GX_AO_PIN_NAME_ADPCM_IN, media->aout, GX_PINDIR_INPUT, 0, GX_PINFLAG_NO_PTS_FIFO);
					GxMediaFilter_Connect(media->demuxer,GX_DEMUX_PIN_NAME_ADAO,media->acodec1,GX_AD_PIN_NAME_ADESA_IN);
					GxMediaFilter_Connect(media->acodec1,GX_AD_PIN_NAME_ADPCM_OUT,media->aout,GX_AO_PIN_NAME_ADPCM_IN);
				}
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RUN_AD_AUDIO, &DemuxSwitchAudio1);
				GxMediaFilter_Config(media->acodec);
				GxMediaFilter_Config(media->acodec1);
				GxMediaFilter_Config(media->aout);
				GxMediaFilter_Run(media->aout);
				GxMediaFilter_Run(media->acodec);
				GxMediaFilter_Run(media->acodec1);
			}

			if (MEDIA_HAS_AUDIO1(media)) {
				int mix = ((0x1 << 0) | (0x1 << 1));

				GxAudioOut_Control(media->aout, GX_AO_CTRL_SET_PCM_MIX, &mix);
				return GX_PLAYER_OK;
			}
		}
	}
	return GX_PLAYER_ERROR;
}

status_t GxMedia_AdAudioMix(GxMedia* media, int mix)
{
	if (media) {
		if (MEDIA_HAS_AUDIO(media) && !NEEDDROP(media->demuxer->audio->dropmode)) {
			if (MEDIA_HAS_AUDIO1(media)) {
				GxAudioOut_Control(media->aout, GX_AO_CTRL_SET_PCM_MIX, &mix);
				return GX_PLAYER_OK;
			}
		}
	}
	return GX_PLAYER_ERROR;
}

status_t GxMedia_SeamlessBandwidthSwitch(GxMedia* media, unsigned int bandwidth)
{
	status_t ret = GX_PLAYER_ERROR;
	if(GX_STREAMTYPE_DEMUXER != media->demuxer->stream->file_format) {
		if (bandwidth < media->stream->prog_max) {
			ret = GxStream_Control(media->stream, GX_STREAM_CTRL_SEAMLESS_BANDWIDTH_SWITCH, &bandwidth);
		}
	} else {
		ret = GxDemuxer_SeamlessBandwidthSwitch(media->demuxer, bandwidth);
	}
	return ret;
}

status_t GxMedia_Save(GxMedia* media, PlayerMediaContent content)
{
	int ret = GX_PLAYER_ERROR;

	if (media) {
		if (content & PLAYER_MEDIA_AUDIO) {
			if (MEDIA_HAS_AUDIO(media) && !media->demuxer->audio->dropmode) {
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SAVE_AUDIO, NULL);
				GxMediaFilter_Stop(media->acodec1);
				GxMediaFilter_Stop(media->acodec);
				GxMediaFilter_Stop(media->aout);
				media->audio_saved = 1;
				ret = GX_PLAYER_OK;
			}
		}

		if (content & PLAYER_MEDIA_VIDEO) {
			if (MEDIA_HAS_VIDEO(media) && !media->demuxer->video->dropmode) {
				int freeze = 1;
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SAVE_VIDEO, NULL);
				GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vcodec);
				GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
				GxMediaFilter_Stop(media->vout);
				av_memhole_flush();
				media->video_saved = 1;
				ret = GX_PLAYER_OK;
			}
		}
	}

	return ret;
}

status_t GxMedia_Restore(GxMedia* media, PlayerMediaContent content)
{
	int ret = GX_PLAYER_ERROR;

	if (media) {
		if (content & PLAYER_MEDIA_AUDIO) {
			if (MEDIA_HAS_AUDIO(media) && !media->demuxer->audio->dropmode) {
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESTORE_AUDIO, NULL);
				GxMediaFilter_Config(media->acodec);
				GxMediaFilter_Config(media->aout);
				GxMediaFilter_Run(media->acodec);
				GxMediaFilter_Run(media->aout);
				media->audio_saved = 0;
				ret = GX_PLAYER_OK;
			}
		}

		if (content & PLAYER_MEDIA_VIDEO) {
			if (MEDIA_HAS_VIDEO(media) && !media->demuxer->video->dropmode) {
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_RESTORE_VIDEO, NULL);
				GxMediaFilter_Config(media->vcodec);
				GxMediaFilter_Config(media->vout);
				GxMediaFilter_Run(media->vcodec);
				GxMediaFilter_Run(media->vout);
				media->video_saved = 0;
				ret = GX_PLAYER_OK;
			}
		}
	}

	return ret;
}

status_t GxMedia_StartRecord(GxMedia* media, const char* dtsurl)
{
	int ret = GX_PLAYER_ERROR;

	if (media) {
		GxMuxerOpenInfo info = {0};
		GxMuxerHeaderInfo aheader_info;
		GxMuxerHeaderInfo vheader_info;
		GxDumpFilterExtra* extra = av_malloc(sizeof(GxDumpFilterExtra));

		if (extra == NULL)
			goto RECORD_ERR;

		info.format = "mpegts";
		info.nb_streams = 0;
		if (HAVE_VIDEO(media->demuxer)) {
			GxStreamVideoHeader *vheader = media->sh_video;

			vheader_info.stream_index                = info.nb_streams;
			media->demuxer->video->mux_index         = info.nb_streams;
			vheader_info.extradata                   = vheader->priv.header.data;
			vheader_info.extradata_size              = vheader->priv.header.len;
			info.streams[info.nb_streams].codec_id   = vcodec_std2codecid(vheader->format);
			info.streams[info.nb_streams].codec_type = CODEC_TYPE_VIDEO;
			info.nb_streams++;
		}

		if (HAVE_AUDIO(media->demuxer)){
			GxStreamAudioHeader *aheader = media->sh_audio;

			aheader_info.stream_index                = info.nb_streams;
			media->demuxer->audio->mux_index         = info.nb_streams;
			aheader_info.extradata                   = aheader->priv.header.data;
			aheader_info.extradata_size              = aheader->priv.header.len;
			info.streams[info.nb_streams].codec_id   = acodec_std2codecid(aheader->format);
			info.streams[info.nb_streams].codec_type = CODEC_TYPE_AUDIO;
			info.nb_streams++;
		}

		media->muxer = GxMuxer_Open(&info);
		if (media->muxer == NULL)
			goto RECORD_ERR;

		media->dumper = GxDumpFilter_Open(dtsurl, DUMPFILTER_RAW);
		if (media->dumper == NULL)
			goto RECORD_ERR;

		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_START_MUX, NULL);
		GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_MUXO, media->muxer, GX_MUXER_PIN_NAME_INPUT);
		GxMediaFilter_Connect(media->muxer, GX_MUXER_PIN_NAME_OUTPUT, media->dumper, GX_DUMPFILTER_PIN_NAME_IN);

		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_RECORD_HEADER_INFO, &extra->header_info);
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_GET_RECORD_USER_INFO, &extra->user_info);
		GxMuxer_Control(media->muxer, GX_MUXER_CTRL_SET_VIDEO_HEADER, &vheader_info);
		GxMuxer_Control(media->muxer, GX_MUXER_CTRL_SET_AUDIO_HEADER, &aheader_info);
		GxDumpFilter_Control(media->dumper, GX_DUMPER_SET_EXTRA, extra);
		ret = GxMediaFilter_Config(media->dumper);
		if (ret != GX_PLAYER_OK)
			goto RECORD_ERR;

		ret = GxMediaFilter_Config(media->muxer);
		if (ret != GX_PLAYER_OK)
			goto RECORD_ERR;

		ret = GxMediaFilter_Run(media->dumper);
		if (ret != GX_PLAYER_OK)
			goto RECORD_ERR;

		ret = GxMediaFilter_Run(media->muxer);
		if (ret != GX_PLAYER_OK)
			goto RECORD_ERR;

		media->runflag |= GX_MEDIA_RUN_RECORD;
		av_free(extra);
		return GX_PLAYER_OK;

RECORD_ERR:
		if (extra)
			av_free(extra);
		if (media->muxer) {
			GxMuxer_Close(media->muxer);
			media->muxer = NULL;
		}
		if (media->dumper) {
			GxDumpFilter_Close(media->dumper);
			media->dumper = NULL;
		}

		gxlogd("%s %d record error\n", __FUNCTION__, __LINE__);
	}

	return GX_PLAYER_ERROR;
}

status_t GxMedia_StopRecord(GxMedia* media)
{
	if (media) {
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_STOP_MUX, NULL);
		if (media->muxer)
			GxMediaFilter_Stop(media->muxer);
		if (media->dumper)
			GxMediaFilter_Stop(media->dumper);
		if (media->muxer)
			GxMuxer_Close(media->muxer);
		if (media->dumper)
			GxDumpFilter_Close(media->dumper);
		media->muxer = NULL;
		media->dumper = NULL;
		media->runflag &= ~GX_MEDIA_RUN_RECORD;
	}

	return GX_PLAYER_ERROR;
}

status_t GxMedia_StartPlay(GxMedia* media)
{
	int ret = GX_PLAYER_ERROR;

	if (media) {
		if (HAVE_AUDIO(media->demuxer)) {
			media->acodec = GxAudioDecoder_Open(media->sh_audio);
			if (media->acodec == NULL)
				goto error_play;

			if (HAVE_AUDIO1(media->demuxer))
				media->acodec1 = GxAudioDecoder_Open(media->sh_audio1);

			media->aout = GxAudioOut_Open(media->sh_audio, media->sh_audio1);
			if (media->aout == NULL)
				goto error_play;
		}

		if (HAVE_VIDEO(media->demuxer)) {
			media->vcodec = GxVideoDecoder_Open(media->sh_video);
			if (media->vcodec == NULL)
				goto error_play;

			media->vout = GxVideoOut_Open(media->sh_video, 0);
			if (media->vout== NULL)
				goto error_play;
		}

		if (HAVE_SUB(media->demuxer)) {
			media->scodec = GxSubDecoder_Open(media->sh_sub);
			if (media->scodec == NULL) {
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_DECODER);
			}

			media->sout = GxSubOut_Open(media->sh_sub,media_get_current_cb,media);
			if (media->sout == NULL) {
				GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_SUB_OFF, NULL);
				MEDIA_EXEC_EVENT(media, PLAYER_STATUS_ERROR,PLAYER_ERROR_NO_SUB_RENDER);
			}
		}

		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_START_PLAY, NULL);
		if (HAVE_AUDIO(media->demuxer)) {
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_AO,    media->acodec, GX_AD_PIN_NAME_ESA_IN);
			GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_PCM_OUT,  media->aout,   GX_AO_PIN_NAME_PCM_IN);
			GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AC3_OUT,  media->aout,   GX_AO_PIN_NAME_AC3_IN);
			GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_EAC3_OUT, media->aout,   GX_AO_PIN_NAME_EAC3_IN);
			GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_DTS_OUT,  media->aout,   GX_AO_PIN_NAME_DTS_IN);
			GxMediaFilter_Connect(media->acodec,  GX_AD_PIN_NAME_AAC_OUT,  media->aout,   GX_AO_PIN_NAME_AAC_IN);
			if (HAVE_AUDIO1(media->demuxer)) {
				GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_ADAO, media->acodec1, GX_AD_PIN_NAME_ADESA_IN);
				GxMediaFilter_Connect(media->acodec1, GX_AD_PIN_NAME_ADPCM_OUT,media->aout,    GX_AO_PIN_NAME_ADPCM_IN);
			}
		}

		if (HAVE_VIDEO(media->demuxer)) {
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_VO, media->vcodec, GX_VD_PIN_NAME_IN);
			GxMediaFilter_Connect(media->vcodec, GX_VD_PIN_NAME_OUT, media->vout, GX_VO_PIN_NAME_IN);
		}

		if (HAVE_SUB(media->demuxer)) {
			GxMediaFilter_Connect(media->demuxer, GX_DEMUX_PIN_NAME_SO, media->scodec, GX_SD_PIN_NAME_IN);
			GxMediaFilter_Connect(media->scodec, GX_SD_PIN_NAME_OUT, media->sout, GX_SO_PIN_NAME_IN);
		}

		if (HAVE_AUDIO(media->demuxer)) {
			ret = GxMediaFilter_Config(media->acodec);
			if (ret < 0) {
				if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
					goto error_play;
			} else {
				if (MEDIA_HAS_AUDIO1(media)) {
					ret = GxMediaFilter_Config(media->acodec1);
					if (ret < 0) {
						if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
							goto error_play;
					}
				}
				ret = GxMediaFilter_Config(media->aout);
				if (ret < 0) {
					if (GxMedia_AudioUnsupport(media) != GX_PLAYER_OK)
						goto error_play;
				}
			}
		}

		if (HAVE_VIDEO(media->demuxer))
		{
			ret = GxMediaFilter_Config(media->vcodec);
			if (ret < 0) {
				if (GxMedia_VideoUnsupport(media) != GX_PLAYER_OK)
					goto error_play;
			}
			else {
				ret = GxMediaFilter_Config(media->vout);
				if (ret < 0) {
					if (GxMedia_VideoUnsupport(media) != GX_PLAYER_OK)
						goto error_play;
				}
			}
		}

		if (HAVE_SUB(media->demuxer)) {
			ret = GxMediaFilter_Config(media->scodec);
			if (ret < 0)
				goto error_play;

			ret = GxMediaFilter_Config(media->sout);
			if (ret < 0)
				goto error_play;
		}

		if (HAVE_AUDIO(media->demuxer)) {
			GxMediaFilter_Run(media->aout);
			GxMediaFilter_Run(media->acodec);
			if (HAVE_AUDIO1(media->demuxer))
				GxMediaFilter_Run(media->acodec1);
		}

		if (HAVE_VIDEO(media->demuxer)) {
			GxMediaFilter_Run(media->vcodec);
			GxMediaFilter_Run(media->vout);
		}

		if (HAVE_SUB(media->demuxer)) {
			GxStreamSubHeader* sh_sub =  media->demuxer->s_streams[media->demuxer->sub->id];
			GxMediaFilter_Run(media->scodec);
			GxSubOut_Control(media->sout, GX_SO_CTRL_SET_PID,&(sh_sub->priv.id));
			GxMediaFilter_Run(media->sout);
		}

		GxCore_MutexCreate(&media->mutex);
		media->runflag |= GX_MEDIA_RUN_PLAY;
		return GX_PLAYER_OK;
	}

error_play:
	if (HAVE_AUDIO(media->demuxer)) {
		GxMediaFilter_Stop(media->aout);
		if (HAVE_AUDIO1(media->demuxer))
			GxMediaFilter_Stop(media->acodec1);
		GxMediaFilter_Stop(media->acodec);
	}

	if (HAVE_VIDEO(media->demuxer)) {
		int freeze = 0;
		GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
		GxMediaFilter_Stop(media->vcodec);
		GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
		GxMediaFilter_Stop(media->vout);
	}

	if (HAVE_SUB(media->demuxer)) {
		GxMediaFilter_Stop(media->sout);
		GxMediaFilter_Stop(media->scodec);
	}

	GxAudioDecoder_Close(media->acodec1);
	GxAudioDecoder_Close(media->acodec);
	GxAudioOut_Close(media->aout, 1);
	GxVideoDecoder_Close(media->vcodec);
	GxVideoOut_Close(media->vout);
	GxSubDecoder_Close(media->scodec);
	GxSubOut_Close(media->sout);

	media->acodec1 = NULL;
	media->acodec  = NULL;
	media->aout    = NULL;
	media->vcodec  = NULL;
	media->vout    = NULL;
	media->scodec  = NULL;
	media->sout    = NULL;

	return GX_PLAYER_ERROR;
}

status_t GxMedia_StopPlay(GxMedia* media, int freeze)
{
	if (media) {
		GxDemuxer_Control(media->demuxer, GX_DEMUXER_CTRL_STOP_PLAY, NULL);
		if (HAVE_AUDIO(media->demuxer)) {
			GxMediaFilter_Stop(media->aout);
			if (HAVE_AUDIO1(media->demuxer))
				GxMediaFilter_Stop(media->acodec1);
			GxMediaFilter_Stop(media->acodec);
		}

		if (HAVE_VIDEO(media->demuxer)) {
			int freeze = 0;
			GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_FREEZE_VIDEO, &freeze);
			GxMediaFilter_Stop(media->vcodec);
			GxVideoOut_Control(media->vout, GX_VO_CTRL_FREEZE_VIDEO, &freeze);
			GxMediaFilter_Stop(media->vout);
		}

		if (HAVE_SUB(media->demuxer)) {
			GxMediaFilter_Stop(media->sout);
			GxMediaFilter_Stop(media->scodec);
		}

		GxAudioDecoder_Close(media->acodec1);
		GxAudioDecoder_Close(media->acodec);
		GxAudioOut_Close(media->aout, 1);
		GxVideoDecoder_Close(media->vcodec);
		GxVideoOut_Close(media->vout);
		GxSubDecoder_Close(media->scodec);
		GxSubOut_Close(media->sout);

		media->acodec1 = NULL;
		media->acodec  = NULL;
		media->aout    = NULL;
		media->vcodec  = NULL;
		media->vout    = NULL;
		media->scodec  = NULL;
		media->sout    = NULL;

		GxCore_MutexDelete(media->mutex);
		media->runflag &= ~GX_MEDIA_RUN_PLAY;
		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

int GxMedia_Read(GxMedia *media, PlayerReadType type, uint8_t* buffer, int len)
{
	int ret = -1;

	if (media) {
		if (type == PLAYER_READ_DUMPER)
			ret = GxDumpFilter_Read(media->dumper, buffer, len);
		else if (type == PLAYER_READ_VDECODER_SUBCC) {
			AVCodecSubCC subcc;

			subcc.data    = buffer;
			subcc.size    = len;
			subcc.timeout = 0;

			ret = GxVideoDecoder_Control(media->vcodec, GX_VD_CTRL_READ_SUBCC, &subcc);
			if (ret != GX_PLAYER_OK)
				return -1;

			return subcc.size;
		}
	}

	return ret;
}

int GxMedia_SetPVRControl(GxMedia *media, GxRecordPVRControl *ctrl)
{
	int ret = -1;

	if (media) {
		GxCore_MutexLock(media->mutex);
		if (media->dumper)
			ret = GxDumpFilter_Control(media->dumper, GX_DUMPER_SET_PVR_CONTROL, ctrl);
		GxCore_MutexUnlock(media->mutex);
	}

	return ret;
}

int GxMedia_GetPVRControl(GxMedia *media, GxRecordPVRControl *ctrl)
{
	int ret = -1;

	if (media) {
		GxCore_MutexLock(media->mutex);
		if (media->dumper)
			ret = GxDumpFilter_Control(media->dumper, GX_DUMPER_GET_PVR_CONTROL, ctrl);
		GxCore_MutexUnlock(media->mutex);
	}

	return ret;
}
