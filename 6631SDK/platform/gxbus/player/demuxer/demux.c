#include "gx_system.h"
#include "gx_demux.h"
#include "gx_decoder.h"
#include "stheader.h"
#include "gx_subreader.h"
#include "gx_system.h"
#include "gx_subtitle.h"
#include "gx_device.h"
#include "demux_lavf.h"
#include "gx_media.h"
#include "iframe_probe.h"
#include "demux_dump.h"

#define DEMUX_NEED_SYNC(d) (d && d->info.clock_en &&\
		d->type != GX_DEMUXER_TYPE_MPEG_ES && \
		d->type != GX_DEMUXER_TYPE_ES && \
		HAVE_AUDIO(d) && \
		HAVE_VIDEO(d) && \
		d->audio->dropmode != DROPMODE_UNSUPPORT &&\
		d->stcfreq>=0 && d->stcfreq<2000)


#define DEMUX_GET_STC_RECOVER_MODE(d, stcfreq, en) \
	(HAVE_AUDIO(d) && (stcfreq>=0 && stcfreq<2000) ? (en?PURE_APTS_RECOVER:APTS_RECOVER) : NO_RECOVER )

static struct _swdmx swdmx;
static int _demuxer_control(GxDemuxer *demuxer, int cmd, void* args);

#define GX_DEMUXER_CLASS_MAX (64)
static int demuxer_class_num = 0;
static GxDemuxerClass* gx_demuxer_classes[GX_DEMUXER_CLASS_MAX];
void GxDemuxerRegister(GxDemuxerClass* cls)
{
	int i;
	if (demuxer_class_num == 0)
		memset(&gx_demuxer_classes, 0, sizeof(gx_demuxer_classes));

	if (demuxer_class_num >= GX_DEMUXER_CLASS_MAX)
		return;

	for (i=0; i<demuxer_class_num; i++) {
		if (gx_demuxer_classes[i] == cls)
			return;
	}

	gx_demuxer_classes[demuxer_class_num++] = cls;

	if (cls->_inherit._inherit.init)
		cls->_inherit._inherit.init();
}

static int _demuxer_stc_run(GxDemuxer* demuxer)
{
	return GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Play, NULL, 0);
}

static int _demuxer_stc_stop(GxDemuxer* demuxer)
{
	return GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Stop, NULL, 0);
}

static int _demuxer_stc_pause(GxDemuxer* demuxer)
{
	return GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Pause, NULL, 0);
}

static int _demuxer_stc_resume(GxDemuxer* demuxer)
{
	return GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Resume, NULL, 0);
}

static int _demuxer_stc_config(GxDemuxer* demuxer, int freq)
{
	int pure_apts_recovery = 0;
	char *opt_value = NULL;
	GxSTCProperty_Config Config;
	GxSTCProperty_TimeResolution TimeResolution;

	GxPlayer_SystemGet(PSYS_AUDIO_PURE_RECOVERY, &pure_apts_recovery);
	demuxer->stcfreq = freq;
	demuxer->stcsync = Config.mode;

	if (demuxer->type == GX_DEMUXER_TYPE_MPEG_ES || demuxer->type == GX_DEMUXER_TYPE_ES) {
		return GX_PLAYER_OK;
	} else if (demuxer->type == GX_DEMUXER_TYPE_HW_TS) {
		if ((demuxer->speed != 1000) && (demuxer->speed > 0) && (demuxer->speed < 2000)) {
			Config.mode = PURE_APTS_RECOVER;
			GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Config, &Config, sizeof(GxSTCProperty_Config));
		} else {
			Config.mode = demuxer->hwts_sync_mode;
			GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Config, &Config, sizeof(GxSTCProperty_Config));
		}
		return GX_PLAYER_OK;
	}

	Config.mode = DEMUX_GET_STC_RECOVER_MODE(demuxer, freq, pure_apts_recovery);
	if (demuxer && demuxer->stream && demuxer->stream->options
		&& (opt_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "stc_recover_mode:"))) {
		if (0 == atoi(opt_value))
			Config.mode = AVPTS_RECOVER;
		else if (1 == atoi(opt_value))
			Config.mode = ((HAVE_VIDEO(demuxer) && (freq>=0 && freq<2000))?VPTS_RECOVER:NO_RECOVER);
		else if (2 == atoi(opt_value))
			Config.mode = ((HAVE_AUDIO(demuxer) && (freq>=0 && freq<2000))?APTS_RECOVER:NO_RECOVER);
		else if (3 == atoi(opt_value))
			Config.mode = PCR_RECOVER;
		else if (4 == atoi(opt_value))
			Config.mode = PURE_APTS_RECOVER;
		else if (5 == atoi(opt_value))
			Config.mode = NO_RECOVER;
		else if (6 == atoi(opt_value))
			Config.mode = FIXD_VPTS_RECOVER;
		else if (7 == atoi(opt_value))
			Config.mode = FIXD_APTS_RECOVER;
	}
	GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Config, &Config, sizeof(GxSTCProperty_Config));

	TimeResolution.freq_HZ = freq * demuxer->stc_factor;
	GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_TimeResolution, &TimeResolution, sizeof(GxSTCProperty_TimeResolution));

	gxlogi("[Player]: STC SYNC=%d,FREQ=%d\n", Config.mode, freq);
	return GX_PLAYER_OK;
}

static void _demuxer_stc_set_time(GxDemuxer* demuxer, int64_t time)
{
	GxSTCProperty_Time Time;
	Time.time = time + demuxer->base_time;
	if(demuxer->type == GX_DEMUXER_TYPE_MPEG_TS)
		Time.time += demuxer->start_pts;
	Time.time *= demuxer->stc_factor;
	GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Time, &Time, sizeof(GxSTCProperty_Time));
	demuxer->last_pts = GX_NOPTS_VALUE;
}

static int _demuxer_stc_get_time(GxDemuxer* demuxer)
{
	GxSTCProperty_Time Time;
	GxAVGetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Time, &Time, sizeof(GxSTCProperty_Time));
	return (int)Time.time;
}

static void _demuxer_reset(GxDemuxer* demuxer)
{
	GxDemuxStream_Reset(demuxer->video);
	GxDemuxStream_Reset(demuxer->audio);
	GxDemuxStream_Reset(demuxer->audio1);
	GxDemuxStream_Reset(demuxer->sub);

	demuxer->fbeof = 0;
	demuxer->stream->eof = 0;
}

static void _demuxer_free_packs(GxDemuxer* demuxer)
{
	GxDemuxStream_FreePacks(demuxer->video);
	GxDemuxStream_FreePacks(demuxer->audio);
	GxDemuxStream_FreePacks(demuxer->audio1);
	GxDemuxStream_FreePacks(demuxer->sub);
}

void GxDemuxer_Reset(GxDemuxer* demuxer)
{
	if (demuxer) {
		_demuxer_stc_set_time(demuxer, demuxer->stc_start_timems);
		_demuxer_reset(demuxer);
	}
}

static int _demuxer_wait_fill_data(GxDemuxer *demuxer)
{
	//不需要同步的文件也预填充数据，防止刚播放或者选时之后因为数据不足导致的卡
	if ((GX_SPEED_NORMAL(demuxer->stcfreq)) &&
			((demuxer->stream->file_format != GX_STREAMTYPE_STREAM &&
			  demuxer->stream->file_format != GX_STREAMTYPE_DEMUXER &&
			  demuxer->stream->file_format != GX_STREAMTYPE_FM) ||
			(demuxer->stream->file_format == GX_STREAMTYPE_DEMUXER && demuxer->stream->is_hls_av_separate))) {
		GxTime start, current;
		int timeoutms = (demuxer->stream->file_format == GX_STREAMTYPE_DEMUXER && demuxer->stream->is_hls_av_separate)?8000:2000, costms = 0;
		GxCore_GetTickTime(&start);
		while(demuxer->stream->eof==0 && costms<timeoutms) {
			if (demuxer->audio->fill_full_flags ||
					demuxer->audio1->fill_full_flags ||
					demuxer->video->fill_full_flags)
				break;
			GxCore_ThreadDelay(10);
			GxCore_GetTickTime(&current);
			costms = TIME_MINUS(current, start);
		}
	}

	return 0;
}

static void* demuxerbase_create(GxObject *obj)
{
	GxDemuxer* d = GXDEMUXER(obj);
	swdmx.dev = GxAvdev_CreateDevice(0);
	if (swdmx.dev < 0)
		return NULL;

	GxPlayer_SystemGet(PSYS_BufSizeESV, &swdmx.esvsize);
	GxPlayer_SystemGet(PSYS_BufSizeESA, &swdmx.esasize);
	GxPlayer_SystemGet(PSYS_BufSizeESS, &swdmx.esssize);
	GxPlayer_SystemGet(PSYS_RECORD_CACHE, &swdmx.dumpsize);
	GxPlayer_SystemGet(PSYS_PACKET_CACHE, &swdmx.cachesize);
	GxPlayer_SystemGet(PSYS_FIREWALL_FLAG, &swdmx.security_flag);
	swdmx.security_es = (swdmx.security_flag & GXMEM_FLAG_DEMUX_ES);
	swdmx.security_ts = (swdmx.security_flag & GXMEM_FLAG_DEMUX_TSW);
	swdmx.poolsize = 256*1024;
	swdmx.mod_stc = GxAvdev_OpenModule(swdmx.dev, GXAV_MOD_STC, 0);
	if (swdmx.mod_stc <= 0) {
		GxAvdev_DestroyDevice(swdmx.dev);
		return NULL;
	}

	GxCore_SemCreate(&d->semaphore_exit, 0);

	return obj;
}

static void demuxerbase_release(GxObject* obj)
{
	GxDemuxer* d = GXDEMUXER(obj);

	GxAvdev_CloseModule(swdmx.dev, swdmx.mod_stc);
	GxAvdev_DestroyDevice(swdmx.dev);

	GxCore_SemDelete(d->semaphore_exit);
}

static int demuxerbase_pause(GxMediaFilter* filter)
{
	GxDemuxer* demuxer = GXDEMUXER(filter);

	_demuxer_stc_pause(demuxer);

	return GX_PLAYER_OK;
}

static int demuxerbase_resume(GxMediaFilter* filter)
{
	GxDemuxer* demuxer = GXDEMUXER(filter);

	_demuxer_wait_fill_data(demuxer);

	_demuxer_stc_resume(demuxer);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int demuxerbase_stop(GxMediaFilter* mf)
{
	GxDemuxer *demuxer = GXDEMUXER(mf);
	GxDemuxerClass *cls = GetGxDemuxerClassFromObject(demuxer);
	int is_stop = 1;

	while (GxCore_MutexTrylock(demuxer->mutex)) {
		_demuxer_reset(demuxer);
		GxCore_ThreadDelay(10);
	}
	GxCore_MutexUnlock(demuxer->mutex);

	GxDemuxer_Control(demuxer, GX_DEMUXER_CTRL_FOURC_STOP, &is_stop);

	GxCore_SemPost(demuxer->semaphore_exit);

	GxCore_ThreadJoin(demuxer->pthread_depack);

	GxCore_ThreadJoin(demuxer->pthread_check);

	if (cls && cls->sub_stop)
		cls->sub_stop(demuxer);

	if (demuxer && DEMUX_NEED_SYNC(demuxer))
		_demuxer_stc_stop(demuxer);

	_demuxer_reset(demuxer);

	return GX_PLAYER_OK;
}

static int demuxerbase_config(GxMediaFilter* filter)
{
	GxDemuxer* demuxer = GXDEMUXER(filter);
	GxFifoConfig FifoConfig;
	int fifo_size;

	_demuxer_stc_config(demuxer, 1000);

	if ((!HAVE_AUDIO(demuxer)) && HAVE_VIDEO(demuxer)) {
		GxStreamVideoHeader* sh = demuxer->video->sh;
		sh->priv.pts_sync = 0;
	}

	if (demuxer->audio->pin && demuxer->audio->pin->fifo) {
		fifo_size             = demuxer->audio->pin->fifo->size;
		FifoConfig.empty_gate = 7 * fifo_size / 8;
		FifoConfig.full_gate  = 1024;

		GxFifo_Config(demuxer->audio->pin->fifo, &FifoConfig);
	}

	if (demuxer->video->pin && demuxer->video->pin->fifo) {
		fifo_size             = demuxer->video->pin->fifo->size;
		FifoConfig.empty_gate = 7 * fifo_size / 8;
		FifoConfig.full_gate  = 4096;

		GxFifo_Config(demuxer->video->pin->fifo, &FifoConfig);
	}

	if (demuxer->sub->pin && demuxer->sub->pin->fifo) {
		fifo_size             = demuxer->sub->pin->fifo->size;
		FifoConfig.empty_gate = 7 * fifo_size / 8;
		FifoConfig.full_gate  = 1024;

		GxFifo_Config(demuxer->sub->pin->fifo, &FifoConfig);
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

	return GX_PLAYER_OK;
}

static void sync_add_reserve_data(GxDemuxStream* ds, unsigned char* buf, int size, int64_t pts)
{
	if (size && buf) {
		ds->resvbuffer = av_realloc(ds->resvbuffer, ds->resvsize+size);

		if (ds->resvbuffer)
			memcpy(ds->resvbuffer+ds->resvsize, buf, size);

		ds->resvsize += size;
		ds->resvpts   = (pts < 0 ? GX_NOPTS_VALUE : pts);
	}
}

static void sync_clear_reserve_data(GxDemuxStream* ds)
{
	if (ds->resvbuffer)
		av_free(ds->resvbuffer);

	ds->resvbuffer = NULL;
	ds->resvsize   = 0;
}

static void sync_save_reserve_data(GxDemuxStream* ds, unsigned char* buf, int size, int64_t pts)
{
	if (size && buf) {
		ds->resvbuffer = av_realloc(ds->resvbuffer, size);

		if (ds->resvbuffer)
			memcpy(ds->resvbuffer, buf, size);

		ds->resvsize = size;
		ds->resvpts  = (pts < 0 ? GX_NOPTS_VALUE : pts);
	}
}

static int _demuxer_sync_skip_audio(GxDemuxer* demuxer)
{
	int size=0, skip_frame_count=0;
	int64_t vpts=0, apts=0, first_apts = GX_NOPTS_VALUE, first_vpts = GX_NOPTS_VALUE;
	unsigned char* start = NULL;
#define DEMUXER_SYNC_SKIP_TIME (12000)

	if (!(demuxer && demuxer->stream &&
				(demuxer->stream->file_format == GX_STREAMTYPE_STREAM
				|| demuxer->stream->file_format == GX_STREAMTYPE_DEMUXER
				|| demuxer->type == GX_DEMUXER_TYPE_MPEG_TS)))
		return 0;

	if (HAVE_VIDEO(demuxer) && HAVE_AUDIO(demuxer)) {
		if (demuxer->audio->dropmode || demuxer->video->dropmode) {
			av_free(demuxer->video->resvbuffer);
			av_free(demuxer->audio->resvbuffer);
			demuxer->video->resvbuffer = NULL;
			demuxer->audio->resvbuffer = NULL;
			return 0;
		}
		sync_clear_reserve_data(demuxer->video);
		do {
			int ret = 0;

			size = GxDemuxStream_GetPacketPts(demuxer->video, &start, &vpts, NULL, NULL);
			ret  = avpacket_probe_has_iframe(demuxer, start, size);
			if (ret < 0) {
				skip_frame_count++;
				sync_add_reserve_data(demuxer->video, start, size, vpts);
				if (vpts != GX_NOPTS_VALUE || skip_frame_count > 10)
					break;
			} else if (ret > 0) {
				sync_add_reserve_data(demuxer->video, start, size, vpts);
				break;
			}

			if (first_vpts == GX_NOPTS_VALUE)
				first_vpts = vpts;

			if (vpts == GX_NOPTS_VALUE)
				continue;

			if (vpts >= (first_vpts + DEMUXER_SYNC_SKIP_TIME))
				break;
		} while (size > 0);

		if (vpts != GX_NOPTS_VALUE)
		{
			do {
				size = GxDemuxStream_GetPacketPts(demuxer->audio, &start,&apts, NULL, NULL);
				if(first_apts == GX_NOPTS_VALUE)
					first_apts = apts;

				if (apts == GX_NOPTS_VALUE)
					continue;

				if ((apts + 100) >= vpts)
					break;

				if (apts >= (first_apts + DEMUXER_SYNC_SKIP_TIME))
					break;
			} while (size > 0);

			if(size > 0)
				sync_save_reserve_data(demuxer->audio, start, size, apts);
		}
	}

	return 0;
}

static void _demuxer_data_status(GxDemuxer *demuxer)
{
	int audio0_demux_size = 0;
	int audio1_demux_size = 0;
	int video_demux_size = 0;
	int esa0_size = 0;
	int esa1_size = 0;
	int esv_size = 0;
	int debug_fill_data  = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_FILL_DATA, &debug_fill_data);
	if (!debug_fill_data)
		return;

	if(demuxer->audio->sh) {
		audio0_demux_size = demuxer->audio->bytes;
		if(demuxer->audio->pin && demuxer->audio->pin->fifo)
			esa0_size = GxFifo_GetLength(demuxer->audio->pin->fifo);
	}

	if(demuxer->audio1->sh) {
		audio1_demux_size = demuxer->audio1->bytes;
		if(demuxer->audio1->pin && demuxer->audio1->pin->fifo)
			esa1_size = GxFifo_GetLength(demuxer->audio1->pin->fifo);
	}

	if(demuxer->video->sh) {
		video_demux_size = demuxer->video->bytes;
		if(demuxer->video->pin && demuxer->video->pin->fifo)
			esv_size = GxFifo_GetLength(demuxer->video->pin->fifo);
	}

	gxlogi_raw(">fill - ea0:%.02fK\tea1:%.02fK\tev:%.02fK,\tda0:%.02f(K)\tda1:%.02f(K)\tdv%.02f(K),\tsc:%.02f(K)\n",
			((double)esa0_size)/1024, ((double)esa1_size)/1024, ((double)esv_size)/1024,
			((double)audio0_demux_size)/1024, ((double)audio1_demux_size)/1024, ((double)video_demux_size)/1024,
			((double)(demuxer->stream->buf_len - demuxer->stream->buf_pos)/1024));

	return ;
}

static int demuxerbase_is_check_esavsize(GxDemuxer* demux)
{
	int is_login = 1;
	char *options = NULL;
	if (demux->stream &&
		demux->stream->options &&
		(options = GxOptions_Get_By_Name(demux->stream->options, " -H", "IsOpenNetAudioDelay:"))) {
		is_login = atoi(options)?0:1;
	} else if (demux->stream->nobuffer) {
		is_login = 0;
	}
	return is_login;
}

static void demuxerbase_check_thread(void *data)
{
	GxDemuxer* d = GXDEMUXER(data);
	GxMediaFilter* mf = GXMEDIAFILTER(data);

	while (mf->status != GX_MFT_STATE_STOPPED) {
		if (GxCore_SemTimedWait(d->semaphore_exit, 500) == GXCORE_SUCCESS)
			break;

		_demuxer_data_status(d);

		if (d->stream->file_format == GX_STREAMTYPE_DEMUXER && demuxerbase_is_check_esavsize(d)) {
			unsigned char empty = 0, full = 0;

			GxDemuxStream_CheckFillPacket(d);
			empty = GxDemuxer_AlmostEmpty(d);
			full  = GxDemuxer_AlmostFull(d);
			if (mf->event.func) {
				if (empty && (!d->stream->eof)) {
					GxMediaFilterEventPara EventPara;
					EventPara.type = GX_MFT_EVENT_START_FILL_BUF;
					EventPara.arg  = NULL;
					mf->event.func(mf->event.priv, &EventPara);
				}

				if (full || d->stream->eof) {
					GxMediaFilterEventPara EventPara;
					EventPara.type = GX_MFT_EVENT_END_FILL_BUF;
					EventPara.arg  = NULL;
					mf->event.func(mf->event.priv, &EventPara);
				}
			}
		}
	}
}

static void _demuxer_check_eof(GxDemuxer *d, int *ret1, int *ret2, int *ret3)
{
	if(d->stream->eof &&
			((!HAVE_VIDEO(d)) || d->video->eof || d->video->dropmode) &&
			((!HAVE_AUDIO(d)) || d->audio->eof || d->audio->dropmode)) {
		if (GxDemuxer_Restart(d, NULL)){
			*ret1 = *ret2 = *ret3 = -1;
		} else {
			GxMediaFilter* mf = GXMEDIAFILTER(d);

			if(mf->event.func && d->stcfreq > 0) {
				GxMediaFilterEventPara EventPara;
				EventPara.type = GX_MFT_EVENT_PLAY_END;
				EventPara.arg  = NULL;
				mf->event.func(mf->event.priv, &EventPara);
			}
			*ret1 = *ret2 = *ret3 = -2;
		}
	}

	return;
}

static void demuxerbase_run_thread(void* data)
{
	int ret1 = 0, ret2 = 0, ret3 = 0;
	GxDemuxer* d = GXDEMUXER(data);
	GxMediaFilter* mf = GXMEDIAFILTER(data);

	while (mf->status != GX_MFT_STATE_STOPPED) {
		if (d->pause_fill_fifo) {
			GxCore_ThreadDelay(10);
			continue;
		}
		if (mf->status == GX_MFT_STATE_RUNNING) {
			GxCore_MutexLock(d->mutex);
			if (GX_SPEED_JUMP(d->stcfreq)) {
				ret1 = GxDemuxStream_PushData(d->video);
				ret2 = ret3 = -2;
			} else {
				if ((d->video->packs == 0) &&
						(d->audio->packs == 0) &&
						(d->audio1->packs == 0)) {
					ret1 = GxDemuxStream_PushData(d->video);
					ret2 = GxDemuxStream_PushData(d->audio);
					ret3 = GxDemuxStream_PushData(d->audio1);
				} else {
#define DS_IGNORE(ds) (ds->sh == NULL || ds->fill_error || NEEDDROP(ds->dropmode))
					ret1 = DS_IGNORE(d->video) ? -2 : 0;
					ret2 = DS_IGNORE(d->audio) ? -2 : 0;
					ret3 = DS_IGNORE(d->audio1) ? -2 : 0;
					if ((d->video->packs > 0)
						|| (NEEDDROPSWITCH(d->audio->dropmode))) {
						ret1 = GxDemuxStream_PushData(d->video);
					}
					if (d->audio->packs > 0)
						ret2 = GxDemuxStream_PushData(d->audio);
					if (d->audio1->packs > 0)
						ret3 = GxDemuxStream_PushData(d->audio1);
				}
			}

			if (d->sub->packs > 0)
				GxDemuxStream_PushSub(d->sub);

			_demuxer_check_eof(d, &ret1, &ret2, &ret3);
			GxCore_MutexUnlock(d->mutex);
			if(ret1 == -2 && ret2 == -2 && ret3 == -2) {
				GxCore_ThreadDelay(10);
			} else {
				GxCore_ThreadYield();
			}
		} else if (mf->status == GX_MFT_STATE_PAUSED) {
			if (d->stream->file_format == GX_STREAMTYPE_DEMUXER) {
				if (!GxDemuxer_AlmostFull(d) && !GX_SPEED_JUMP(d->stcfreq)) {
					GxCore_MutexLock(d->mutex);
					int fill_packs = 1;

					fill_packs &= ((d->video->packs  == 0) ? 1 : 0);
					fill_packs &= ((d->audio->packs  == 0) ? 1 : 0);
					fill_packs &= ((d->audio1->packs == 0) ? 1 : 0);
					if (d->video->fill_full_flags)
						ret1 = GxDemuxStream_CachePacket(d->video);
					else {
						ret1 = DS_IGNORE(d->video) ? -2 : 0;
						if (d->video->packs > 0
						|| fill_packs
						|| (NEEDDROPSWITCH(d->audio->dropmode)))
							ret1 = GxDemuxStream_PushData(d->video);
					}
					if (d->audio->fill_full_flags)
						ret2 = GxDemuxStream_CachePacket(d->audio);
					else {
						ret2 = DS_IGNORE(d->audio) ? -2 : 0;
						if (d->audio->packs > 0 || fill_packs)
							ret2 = GxDemuxStream_PushData(d->audio);
					}
					if (d->audio1->fill_full_flags)
						ret3 = GxDemuxStream_CachePacket(d->audio1);
					else {
						ret3 = DS_IGNORE(d->audio1) ? -2 : 0;
						if (d->audio1->packs > 0 || fill_packs)
							ret3 = GxDemuxStream_PushData(d->audio1);
					}

					_demuxer_check_eof(d, &ret1, &ret2, &ret3);
					GxCore_MutexUnlock(d->mutex);

					if (ret1 != 0 && ret2 != 0 && ret3 != 0)
						GxCore_ThreadDelay(100);
				} else
					GxCore_ThreadDelay(100);
			} else
				GxCore_ThreadDelay(100);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
}

static int demuxerbase_run(GxMediaFilter* filter)
{
	GxDemuxer *demuxer = GXDEMUXER(filter);
	GxDemuxerClass *cls = GetGxDemuxerClassFromObject(demuxer);
	int is_stop = 0;

	if (demuxer->debug_dumper)
		GxDemuxDump_Close(demuxer->debug_dumper);
	demuxer->debug_dumper = GxDemuxDump_Open(demuxer);

	demuxer->audio->fill_full_flags  = 0;
	demuxer->audio1->fill_full_flags = 0;
	demuxer->video->fill_full_flags  = 0;

	GxDemuxer_Control(demuxer, GX_DEMUXER_CTRL_FOURC_STOP, &is_stop);
	_demuxer_stc_run(demuxer);
	_demuxer_stc_set_time(demuxer, demuxer->stc_start_timems);

	if(cls && cls->sub_run)
		cls->sub_run(demuxer);

	GxCore_ThreadCreate("demux_depack", &demuxer->pthread_depack,
			demuxerbase_run_thread, filter, 1024*32, GXOS_DEFAULT_PRIORITY);

	_demuxer_wait_fill_data(demuxer);

	GxCore_ThreadCreate("demux_check", &demuxer->pthread_check,
			demuxerbase_check_thread, filter, 8*1024, GXOS_DEFAULT_PRIORITY);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static inline void stream_header_free_priv(GxStreamHeadPriv* priv)
{
	if(priv)
	{
		if (priv->header.data)
			av_free(priv->header.data);
		if (priv->para.data)
			av_free(priv->para.data);
	}
}

GxStreamSubHeader* GxStreamHeader_SubNew(GxDemuxer * demuxer, int id, int sid)
{
	if (id > MAX_S_STREAMS - 1 || id < 0)
	{
		return NULL;
	}

	if (!demuxer->s_streams[id])
	{
		GxStreamSubHeader* sh = av_mallocz(sizeof(GxStreamSubHeader));
		demuxer->s_streams[id] = sh;
		sh->priv.id = sid;
	}

	return demuxer->s_streams[id];
}

void GxStreamHeader_SubFree(GxStreamSubHeader* sh)
{
	if(sh == NULL)
		return ;
	if(sh->extradata)
		av_free(sh->extradata);
#ifdef CONFIG_ASS
	if (sh->ass_track)
		ass_free_track(sh->ass_track);
#endif
	stream_header_free_priv(&sh->priv);
	av_free(sh);
}

GxStreamAudioHeader* GxStreamHeader_AudioNew(GxDemuxer * demuxer, int id, int aid)
{
	if (id > MAX_A_STREAMS - 1 || id < 0)
	{
		gxlogf("Requested audio stream id overflow (%d > %d)\n", id, MAX_A_STREAMS);
		return NULL;
	}
	if (demuxer->a_streams[id])
	{
		GxStreamAudioHeader* sh = demuxer->a_streams[id];
		sh->priv.id = aid;
		sh->stat.decode_frame_cnt = 0;
	}
	else
	{
		GxStreamAudioHeader* sh = av_mallocz(sizeof(GxStreamAudioHeader));
		demuxer->a_streams[id] = sh;
		sh->priv.id = aid;
		sh->ds = demuxer->audio;
		sh->audio_id = 0;
		sh->samplesize = 2;
		sh->pts = 0;
		sh->priv.pts_sync = 0;
		sh->dual_mono     = -1;
		demuxer->num_audio++;
	}

	return demuxer->a_streams[id];
}

GxStreamAudioHeader* GxStreamHeader_Audio1New(GxDemuxer * demuxer, int id, int aid)
{
	if (id > MAX_A_STREAMS - 1 || id < 0)
	{
		gxlogf("Requested audio stream id overflow (%d > %d)\n", id, MAX_A_STREAMS);
		return NULL;
	}
	if (demuxer->a_streams[id])
	{
		GxStreamAudioHeader* sh = demuxer->a_streams[id];
		sh->priv.id = aid;
		sh->stat.decode_frame_cnt = 0;
	}
	else
	{
		GxStreamAudioHeader* sh = av_mallocz(sizeof(GxStreamAudioHeader));
		demuxer->a_streams[id] = sh;
		sh->priv.id = aid;
		sh->ds = demuxer->audio1;
		sh->audio_id = 1;
		sh->samplesize = 2;
		sh->pts = 0;
		sh->priv.pts_sync = 0;
	}

	return demuxer->a_streams[id];
}

void GxStreamHeader_AudioFree(GxStreamAudioHeader* sh)
{
	if(sh == NULL)
		return ;
	if (sh->wf)
		av_free(sh->wf);
	if (sh->codecdata)
		av_free(sh->codecdata);
	stream_header_free_priv(&sh->priv);
	memset(sh, 0, sizeof(GxStreamAudioHeader));
	av_free(sh);
}

GxStreamVideoHeader* GxStreamHeader_VideoNew(GxDemuxer * demuxer, int id, int vid)
{
	if (id > MAX_V_STREAMS - 1 || id < 0)
	{
		gxlogf("Requested video stream id overflow (%d > %d)\n", id, MAX_V_STREAMS);
		return NULL;
	}
	if (demuxer->v_streams[id])
	{
		GxStreamVideoHeader* sh = demuxer->v_streams[id];
		sh->priv.id = vid;
		sh->fps = 0;
		sh->stat.decode_frame_cnt = 0;
	}
	else
	{
		GxStreamVideoHeader* sh = av_mallocz(sizeof(GxStreamVideoHeader));
		demuxer->v_streams[id] = sh;
		sh->priv.id = vid;
		sh->ds = demuxer->video;
		sh->priv.pts_sync = 0;
	}

	return demuxer->v_streams[id];
}

void GxStreamHeader_VideoFree(GxStreamVideoHeader* sh)
{
	if(sh == NULL)
		return ;
	if (sh->bih)
		av_free(sh->bih);
	stream_header_free_priv(&sh->priv);
	memset(sh, 0, sizeof(GxStreamVideoHeader));
	av_free(sh);
}

static int demux_get_type_from_name(char* file_name, int demuxer_type)
{
	int i=0,postfixlen=6;
	char* dot = file_name;
	char* postfixptr = NULL;
	char  postfix[8];

	if (!file_name)
		return GX_DEMUXER_TYPE_UNKNOWN;

	while(dot)
	{
		postfixptr = ++dot;
		dot = strchr(dot,'.');
	}

	if(postfixptr == file_name)
		return GX_DEMUXER_TYPE_UNKNOWN;

	while(*postfixptr != '\0' && i< postfixlen)
	{
		postfix[i++] = tolower(*(postfixptr++));
	}

	postfix[i] = '\0';
	if (!strncmp(postfix, "es",2))
		return GX_DEMUXER_TYPE_ES;
	if (!strncmp(postfix, "ts",2) ||
			(!strncmp(postfix, "ets",3))||
			(!strncmp(postfix, "tp",2))||
			(!strncmp(postfix, "mpg_ts",6))||
			(!strncmp(postfix, "mpg_ps",6))||
			(!strncmp(postfix, "trp",3))) {
		return GX_DEMUXER_TYPE_MPEG_TS;
	}
	if(!strncmp(postfix, "m2ts",4))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "rm",2))||
			(!strncmp(postfix, "ra",2))||
			(!strncmp(postfix, "rv",2))||
			(!strncmp(postfix, "real",4))||
			(!strncmp(postfix, "rmvb",4)))
		return GX_DEMUXER_TYPE_RMVB;
	if ((!strncmp(postfix, "mp4",3))||
			(!strncmp(postfix, "mov",3))||
			(!strncmp(postfix, "3gp",3))||
			(!strncmp(postfix, "ape",3))||
			(!strncmp(postfix, "m4v",3))||
			(!strncmp(postfix, "m4a",3)))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "avi",3))||
			(!strncmp(postfix, "divx",4))||
			(!strncmp(postfix, "xvid",4)))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "flv",3))||
			(!strncmp(postfix, "flx",3))||
			(!strncmp(postfix, "f4v",3))||
			(!strncmp(postfix, "hlv",3)))
		return GX_DEMUXER_TYPE_LAVF;
	if (!strncmp(postfix, "mkv",3))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "mpg",3))||
			(!strncmp(postfix, "mpeg",4))||
			(!strncmp(postfix, "dat",3))||
			(!strncmp(postfix, "vob",3)))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "mp3",3))||
			(!strncmp(postfix, "wav",3))||
			(!strncmp(postfix, "flac",4)))
		return GX_DEMUXER_TYPE_AUDIO;
	if (!strncmp(postfix, "ogg",3))
		return GX_DEMUXER_TYPE_LAVF;
	if (!strncmp(postfix, "aac",3))
		return GX_DEMUXER_TYPE_AAC;
	if (!strncmp(postfix, "bin",3) ||
			(!strncmp(postfix, "m2v",3)))
		return GX_DEMUXER_TYPE_ES;
	if((!strncmp(postfix, "esa",3))||
			(!strncmp(postfix, "esv",3)))
		return GX_DEMUXER_TYPE_ES;
	if (!strncmp(postfix, "dra",3))
		return GX_DEMUXER_TYPE_DRA;
	if (!strncmp(postfix, "ac3",3) ||
			(!strncmp(postfix, "eac3",3)))
		return GX_DEMUXER_TYPE_LAVF;
	if (!strncmp(postfix, "dts",3))
		return GX_DEMUXER_TYPE_LAVF;
	if (!strncmp(postfix, "sbc",3))
		return GX_DEMUXER_TYPE_LAVF;
	if ((!strncmp(postfix, "dvr",3)) ||
			(!strncmp(postfix, "pvr",3)) ||
			(!strncmp(postfix, "evt",3)) ||
			(!strncmp(postfix, "lst",3)) ||
			(!strncmp(postfix, "seg",3))) {
		return demuxer_type;
	}

	return GX_DEMUXER_TYPE_UNKNOWN;
}

static GxDemuxerClass* demux_get_class_from_type(int type)
{
	int i;

	for (i = 0; gx_demuxer_classes[i]; i++)
		if (type == gx_demuxer_classes[i]->type)
			return gx_demuxer_classes[i];

	return NULL;
}

GxDemuxer* new_demuxer(GxStream *stream, GxDemuxerClass *demux_class)
{
	int v_id = -1;
	int a_id = -1;
	int s_id = -1;
	int ext_a_id = -1;
	int pcr_id = -1;

	GxDemuxer* d = GxObjectNew(NULL, demux_class);

	if(d)
	{
		GxCore_MutexCreate(&d->mutex);

		d->fill_pkt_size  = 0;
		d->last_fill_pkt_size = 0;
		d->fill_pkt_time  = 0;
		d->fill_pkt_speed = 0;

		GxUrl_GetItem(stream->url,GX_URL_KEY_VPID, &v_id );
		GxUrl_GetItem(stream->url,GX_URL_KEY_APID, &a_id );
		GxUrl_GetItem(stream->url,GX_URL_KEY_SPID, &s_id );
		GxUrl_GetItem(stream->url,GX_URL_KEY_APID1,  &ext_a_id);
		GxUrl_GetItem(stream->url,GX_URL_KEY_PCRPID, &pcr_id);

		d->audio  = GxDemuxStream_Create(d, a_id);
		d->video  = GxDemuxStream_Create(d, v_id);
		d->audio1 = GxDemuxStream_Create(d, ext_a_id);
		d->sub    = GxDemuxStream_Create(d, s_id);
		d->type   = demux_class->type;
		d->pcr    = pcr_id;
		d->stcfreq         = 1000;
		d->base_time       = 0;
		d->pause_depack    = 0;
		d->muxing          = 0;
		d->playing         = 0;
		d->pause_fill_fifo = 0;

		if (stream != NULL)
		{
			d->stream = stream;
			d->stream_pts = GX_NOPTS_VALUE;
			d->movi_start = stream->start_pos;
			d->movi_end = stream->end_pos;
			switch(stream->file_format)
			{
			case GX_STREAMTYPE_FILE:
				d->seekable = 1;
				break;
			case GX_STREAMTYPE_STREAM:
				d->seekable = ((stream->flags&GX_STREAM_SEEK)==GX_STREAM_SEEK)?1:0;
				break;
			default:
				d->seekable  = 0;
				break;
			}
			d->synced = 0;
			d->filepos = 0;
			d->index_mode = 0;
			d->speedable = 0;
			GxStream_Reset(stream);
			if ((stream->file_format == GX_STREAMTYPE_STREAM) && (stream->demuxer_type != GX_DEMUXER_TYPE_ASSOCIATIONLIBRE))
				GxStream_Seek(stream, stream->start_pos);
		}
	}

	return d;
}

static int _demuxer_get_type(GxStream *stream)
{
	int type = GX_DEMUXER_TYPE_UNKNOWN;

	if(stream == NULL)
		return type;

	switch(stream->file_format)
	{
	case GX_STREAMTYPE_STREAM:
		if(stream->demuxer_type == GX_DEMUXER_TYPE_RTP) {
			gxlogd("Demuxer Type LIVE555 RTP\n");
			type = GX_DEMUXER_TYPE_RTP;
		}
		else if(stream->demuxer_type == GX_DEMUXER_TYPE_REAL) {
			gxlogd("Demuxer Type REAL\n");
			type = GX_DEMUXER_TYPE_REAL;
		}
		else {
			if(stream->demuxer_type == GX_DEMUXER_TYPE_UNKNOWN )
				type = demux_get_type_from_name(stream->url, GX_DEMUXER_TYPE_UNKNOWN);
			else
				type = stream->demuxer_type ;
			if(type == GX_DEMUXER_TYPE_UNKNOWN){
				type = GX_DEMUXER_TYPE_LAVF;
				stream->demuxer_type = type;
			}
			gxlogd("Demuxer Type From HTTP File,(%d)\n", stream->demuxer_type);
		}
		break;
	case GX_STREAMTYPE_MEM:
	case GX_STREAMTYPE_PLAYLIST:
		{
			if (stream->demuxer_type == GX_DEMUXER_TYPE_UNKNOWN )
				type = GX_DEMUXER_TYPE_LAVF;
			else
				type = stream->demuxer_type;
			break;
		}
	case GX_STREAMTYPE_FILE:
		type = demux_get_type_from_name(stream->url, stream->demuxer_type);
		break;
	case GX_STREAMTYPE_DVB:
		type = GX_DEMUXER_TYPE_HW;
		break;
	case GX_STREAMTYPE_FM:
		type = GX_DEMUXER_TYPE_FM;
		break;
	case GX_STREAMTYPE_DEMUXER:
	case GX_STREAMTYPE_VODSYSTEM:
		type = stream->demuxer_type;
		gxlogd("Demuxer Type From Stream,(%d)\n", stream->demuxer_type);
		break;
	default:
		break;
	}

	return type;
}

GxDemuxer* GxDemuxer_Open(GxStream *stream, int flag, GxDemuxerOpenInfo* info)
{
	GxDemuxer* demuxer;
	GxDemuxerClass* demuxer_class;
	int type = GX_DEMUXER_TYPE_UNKNOWN, error_status = PLAYER_ERROR_NO_ERROR;

	if (info && (info->demuxer_type != GX_DEMUXER_TYPE_UNKNOWN))
		type = info->demuxer_type;
	else
		type = _demuxer_get_type(stream);

	if (type == GX_DEMUXER_TYPE_UNKNOWN) {
		error_status = PLAYER_ERROR_NO_DEMUX_TOOLS;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
		return NULL;
	}

recheck:
	if((demuxer_class = demux_get_class_from_type(type)) ==  NULL) {
		error_status = PLAYER_ERROR_NO_DEMUX_TOOLS;
		GxPlayer_SystemSet(PSYS_PLAY_ERROR_STATUS, &error_status);
		return NULL;
	}

	demuxer = new_demuxer(stream, demuxer_class);
	if(demuxer) {
		demuxer->flag  = flag;
		demuxer->type  = demuxer_class->type;
		demuxer->swdmx = &swdmx;
		demuxer->stc_factor = 2;
		demuxer->speed = 1000;
		if (!info) {
			demuxer->info.clock_en  = 1;
			demuxer->info.audio_pid = -1;
			demuxer->info.video_pid = -1;
			demuxer->info.sub_pid   = -1;
			demuxer->info.debug     = NULL;
			demuxer->info.media     = NULL;
		} else
			demuxer->info = *info;

		if (demuxer_class->check_file && (type != demuxer_class->check_file(demuxer))) {
			GxDemuxer_Close(demuxer);
			if (type != GX_DEMUXER_TYPE_HW_TS && type != GX_DEMUXER_TYPE_LAVF) {
				type = GX_DEMUXER_TYPE_LAVF;
				goto recheck;
			}
			return NULL;
		}

		if (demuxer_class->open) {
			if(demuxer_class->open(demuxer) == NULL) {
				if (type != GX_DEMUXER_TYPE_HW_TS && type != GX_DEMUXER_TYPE_LAVF) {
					GxDemuxer_Close(demuxer);
					type = GX_DEMUXER_TYPE_LAVF;
					goto recheck;
				}
				goto error_out;
			}
		}

		if (stream->audio_disable && HAVE_AUDIO(demuxer)) {
			demuxer->audio->dropmode  = DROPMODE_UNSUPPORT;
			demuxer->audio->sh        = NULL;
			demuxer->audio1->dropmode = DROPMODE_UNSUPPORT;
			demuxer->audio1->sh       = NULL;
		}

		if (stream->video_disable && HAVE_VIDEO(demuxer)) {
			demuxer->video->dropmode = DROPMODE_UNSUPPORT;
			demuxer->video->sh       = NULL;
		}

		if (stream->isquiet && HAVE_AUDIO(demuxer))
			demuxer->audio->dropmode = DROPMODE_UNSUPPORT;

		if (HAVE_VIDEO(demuxer)) {
			GxStreamVideoHeader* sh = demuxer->video->sh;
			sh->isquiet = stream->isquiet;
			if(DEMUX_NEED_SYNC(demuxer))
				sh->priv.pts_sync = 1;
			demuxer->speedable = demuxer->seekable ? 1 : 0;
		}

		if (HAVE_AUDIO(demuxer)) {
			GxStreamAudioHeader* sh = demuxer->audio->sh;
			if(DEMUX_NEED_SYNC(demuxer))
				sh->priv.pts_sync = 1;
		}

		gxlogf("[Player]: DMX = %s\n", demuxer_class->name);
		if (flag & GX_DEMUXER_FLAG_PLAYER) {
			if((!HAVE_AUDIO(demuxer)) && (!HAVE_VIDEO(demuxer)) && (!HAVE_AUDIO1(demuxer))) {
				goto error_out;
			}

			if (type != GX_DEMUXER_TYPE_HW && type != GX_DEMUXER_TYPE_HW_TS && type != GX_DEMUXER_TYPE_MPEG_ES && HAVE_VIDEO(demuxer)) {
				demuxer->poolbuf = av_malloc(swdmx.poolsize);
				if (demuxer->poolbuf == NULL)
					goto error_out;
				GxDemuxPool_Init(&demuxer->pool, demuxer->poolbuf, swdmx.poolsize);
			}

			if (HAVE_VIDEO(demuxer)) {
				if (swdmx.security_es && demuxer->type != GX_DEMUXER_TYPE_HW && demuxer->type != GX_DEMUXER_TYPE_HW_TS)
					demuxer->video->pin = GxPin_Create(GX_DEMUX_PIN_NAME_VO, demuxer, GX_PINDIR_OUTPUT, swdmx.esvsize, GX_PINFLAG_SESV);
				else
					demuxer->video->pin = GxPin_Create(GX_DEMUX_PIN_NAME_VO, demuxer, GX_PINDIR_OUTPUT, swdmx.esvsize, GX_PINFLAG_ESV);
				if (demuxer->video->pin == NULL)
					goto error_out;
			}

			if(HAVE_SUB(demuxer)) {
				demuxer->sub->pin = GxPin_Create(GX_DEMUX_PIN_NAME_SO, demuxer, GX_PINDIR_OUTPUT, swdmx.esssize, GX_PINFLAG_SW);
				if (demuxer->sub->pin == NULL)
					goto error_out;
			}

			if (HAVE_AUDIO(demuxer)) {
				if (swdmx.security_es && demuxer->type != GX_DEMUXER_TYPE_HW && demuxer->type != GX_DEMUXER_TYPE_HW_TS)
					demuxer->audio->pin = GxPin_Create(GX_DEMUX_PIN_NAME_AO, demuxer, GX_PINDIR_OUTPUT, swdmx.esasize, GX_PINFLAG_SESA);
				else
					demuxer->audio->pin = GxPin_Create(GX_DEMUX_PIN_NAME_AO, demuxer, GX_PINDIR_OUTPUT, swdmx.esasize, GX_PINFLAG_ESA);
				if (demuxer->audio->pin == NULL)
					goto error_out;
			}

			if(HAVE_AUDIO1(demuxer)) {
				int subflag = GX_PINFLAG_ESA1;
				int esasize = swdmx.esasize;

				if ((demuxer->type == GX_DEMUXER_TYPE_HW) ||
						(demuxer->type == GX_DEMUXER_TYPE_HW_TS)) {
					subflag = GX_PINFLAG_TSA;
					esasize = GX_PINSIZE_TSA;
				}
				if ((demuxer->audio1->pin = GxPin_Create(
								GX_DEMUX_PIN_NAME_ADAO,
								demuxer,
								GX_PINDIR_OUTPUT,
								esasize,
								subflag)) == NULL)
					goto error_out;
			}
			demuxer->playing = 1;
		}

		if (flag & GX_DEMUXER_FLAG_RECORDER) {
			demuxer->tsout = GxPin_Create(GX_DEMUX_PIN_NAME_TSO, demuxer, GX_PINDIR_OUTPUT, swdmx.dumpsize, GX_PINFLAG_MUXTS);
			if(demuxer->tsout == NULL)
				goto error_out;
		}

		if (flag & GX_DEMUXER_FLAG_MUXER) {
			demuxer->muxout = GxPin_Create(GX_DEMUX_PIN_NAME_MUXO, demuxer, GX_PINDIR_OUTPUT, swdmx.dumpsize, GX_PINFLAG_MUXER);
			if(demuxer->muxout == NULL)
				goto error_out;
			demuxer->muxing = 1;
		}

		if (HAVE_AUDIO(demuxer) && (type != GX_DEMUXER_TYPE_HW) && (type != GX_DEMUXER_TYPE_HW_TS)) {
			GxStreamAudioHeader* sh_audio = demuxer->audio->sh;
			if (sh_audio->format == AUDIO_CODEC_AC3 || sh_audio->format == AUDIO_CODEC_EAC3) {
				unsigned char* start = NULL;
				int64_t pts  = -1;
				int32_t size = 0, i = 0, retry = 0;
				AudioCodecType format = sh_audio->format;

				while(retry < 30) {
					size = GxDemuxStream_ProbePacketPts(demuxer->audio, &start, &pts, 1);
					if (size > 0) {
						if (start[0] != 0x0b || start[1] != 0x77) {
							GxDemuxStream_GetPacketPts(demuxer->audio, &start, &pts, NULL, NULL);
						} else
							break;
					}
					GxCore_ThreadDelay(10);
					retry++;
				}

				for (i = 0; i < size - 1; i++) {
					if (start[i] == 0x0b && start[i+1] == 0x77 && i < (size - 6)) {
						unsigned char bitstream_id = (start[i+5]>>3)&0x1F;
						if (bitstream_id <= 10) {
							if (sh_audio->format == AUDIO_CODEC_AC3)
								break;
							format = AUDIO_CODEC_AC3;
						} else if (bitstream_id <= 16) {
							if (sh_audio->format == AUDIO_CODEC_EAC3)
								break;
							format = AUDIO_CODEC_EAC3;
						}
					}
				}

				if (i >= (size -6))
					sh_audio->format = format;
			}
		}

		if(type == GX_DEMUXER_TYPE_MPEG_ES && (HAVE_VIDEO(demuxer))) {
			GxStreamVideoHeader* sh_video = demuxer->video->sh;
			if(stream->file_format != GX_STREAMTYPE_STREAM ||
					(stream->file_format == GX_STREAMTYPE_STREAM && sh_video && sh_video->format==0)) {
				GxDemuxerClass* dc = GetGxDemuxerClassFromObject(demuxer);
				if(video_read_properties(demuxer->video->sh) == GX_PLAYER_ERROR)
					goto error_out;
				if (dc && dc->control)
					dc->control(demuxer, GX_DEMUXER_CTRL_PRIVATE_RESET, NULL);
			}
		}

		demuxer->duration = GxDemuxer_GetTimeLength(demuxer);
		demuxer->stream->eof = 0;
		demuxer->video->eof= 0;
		demuxer->audio->eof = 0;
		demuxer->audio1->eof = 0;
		demuxer->sub->eof   = 0;

		return demuxer;

error_out:
		if(demuxer) {
			GxDemuxer_Close(demuxer);
		}
	}

	return NULL;
}

//abandon code: V2 network don't use it.
GxDemuxer* GxDemuxer_Restart(GxDemuxer* demuxer, void* args)
{
	GxDemuxerClass *cls = GetGxDemuxerClassFromObject(demuxer);
	int ret_args = 1;
	int ret = 0;
	int base_seq_no = -1;
	int cur_seq_no = (args==NULL)?-1:*(int*)args;
	int seekable = demuxer->seekable;

	if(cur_seq_no == -1){
		if(demuxer->stream->demuxer_type != GX_DEMUXER_TYPE_LAVF){
			ret = GxStream_Control(demuxer->stream, GX_STREAM_CTRL_IS_NEW_DATA, &ret_args);
			if(ret == GX_PLAYER_OK && ret_args) {
				GxStreamSeek seq_info = {0,0,-1};

				seq_info.seek_seq = demuxer->stream->cur_seq_no+1;
				GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TIME_BYSEQ, &seq_info);
				demuxer->stream->info.restart_time = seq_info.seek_ms;
				demuxer->stream->err.need_restart = 1;
				return NULL;
			} else if(ret != GX_PLAYER_OK || !ret_args) {
				return NULL;
			}
			GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_CUR_SEQ, &cur_seq_no);
			cur_seq_no++;
		} else {
			cur_seq_no = demuxer->stream->cur_seq_no+1;
		}
		ret = GxStream_Control(demuxer->stream, GX_STREAM_CTRL_IS_FINISHED, &ret_args);
		if(ret != GX_PLAYER_OK || ret_args){
			return NULL;
		}
	}

	if(cls->sub_stop)
		cls->sub_stop(demuxer);

	cls->close(demuxer);
	demuxer->stream->eof = 0;
	demuxer->video->eof= 0;
	demuxer->audio->eof = 0;
	demuxer->sub->eof   = 0;

	demuxer->audio->id = -1;
	demuxer->video->id = -1;
	demuxer->sub->id   = -1;

	GxStream_Control(demuxer->stream, GX_STREAM_CTRL_CACHE_RESET, &cur_seq_no);
	demuxer->stream->cur_seq_no = cur_seq_no;
	base_seq_no = cur_seq_no;

	ret = cls->check_file(demuxer);
	if(demuxer->type != ret){
		gxlogf("error : %s %d...\n", __FUNCTION__, __LINE__);
		goto RESTART_ERROR;
	}

	if(!cls->open(demuxer)){
		gxlogf("error : %s %d...\n", __FUNCTION__, __LINE__);
		goto RESTART_ERROR;
	}

	if(cls->sub_run)
		cls->sub_run(demuxer);

	GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_CUR_SEQ, &cur_seq_no);
	demuxer->stream->cur_seq_no = cur_seq_no;
	demuxer->stream->no_restart_stream = 0;
	GxDemuxer_Control(demuxer, GX_DEMUXER_CTRL_SET_BASE_TIME, &base_seq_no);
	demuxer->seekable = seekable;
	return demuxer;

RESTART_ERROR:
	demuxer->video->fill_error = 1;
	demuxer->audio->fill_error = 1;
	demuxer->sub->fill_error   = 1;
	return NULL;
}

void GxDemuxer_Close(GxDemuxer *demuxer)
{
	int i;
	GxDemuxerClass* dc;

	if (!demuxer)
		return;

	dc = GetGxDemuxerClassFromObject(demuxer);
	if (!dc)
		goto skip_streamfree;
	if (dc->close)
		dc->close(demuxer);

	// free streams:
	for (i = 0; i < MAX_A_STREAMS; i++)
		if (demuxer->a_streams[i])
			GxStreamHeader_AudioFree(demuxer->a_streams[i]);
	for (i = 0; i < MAX_V_STREAMS; i++)
		if (demuxer->v_streams[i])
			GxStreamHeader_VideoFree(demuxer->v_streams[i]);
	for (i = 0; i < MAX_S_STREAMS; i++)
		if (demuxer->s_streams[i])
			GxStreamHeader_SubFree(demuxer->s_streams[i]);

	// free demuxers:
	GxDemuxStream_Destroy(demuxer->audio);
	GxDemuxStream_Destroy(demuxer->video);
	GxDemuxStream_Destroy(demuxer->sub);
	GxDemuxStream_Destroy(demuxer->audio1);

	if (demuxer->poolbuf) {
		GxDemuxPool_Destroy(&demuxer->pool);
		av_free(demuxer->poolbuf);
	}

skip_streamfree:
	if (demuxer->filename)
		av_free(demuxer->filename);
	if (demuxer->chapters)
		GxDemuxer_ChapterRemoveAll(demuxer);

	if (demuxer->debug_dumper) {
		GxDemuxDump_Close(demuxer->debug_dumper);
		demuxer->debug_dumper = NULL;
	}

	GxCore_MutexDelete(demuxer->mutex);
	GxObjectDestroy(demuxer);

	return;
}

int GxDemuxer_M3U8_Seek(GxDemuxer* demuxer, int64_t* seek_time)
{
	int64_t time = *seek_time;
	GxStreamSeek seek_info={0,0,-1};

	seek_info.seek_ms = time;
	if(GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_SEQ_BYTIME, &seek_info) == GX_PLAYER_OK){
		int32_t seek_ms = seek_info.seek_ms;
		seek_info.seek_seq++;
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TIME_BYSEQ, &seek_info);
		if(seek_info.seek_ms>=0 && (seek_info.seek_ms-time)<2000){
			time = 0;
		} else {
			time = (time > seek_ms) ? (time - seek_ms) : 0;
			seek_info.seek_seq--;
		}
		if((seek_info.seek_seq != demuxer->stream->cur_seq_no)||
				(demuxer->stream->no_restart_stream && seek_info.seek_seq == demuxer->stream->cur_seq_no)){
			if(GxDemuxer_Restart(demuxer, &seek_info.seek_seq) == NULL)
				return GX_PLAYER_ERROR;
		}
	}
	*seek_time = time;
	return GX_PLAYER_OK;
}

int _demuxer_seek(GxDemuxer* demuxer, int64_t time, int flag)
{
	int ret = GX_PLAYER_OK;
	uint64_t cur = 0, totle = 0;
	GxDemuxerClass *cls = GetGxDemuxerClassFromObject(demuxer);

	if(!demuxer->seekable)
		return GX_PLAYER_ERROR;

	cur   = GxDemuxer_GetCurrentTime(demuxer);
	totle = GxDemuxer_GetTimeLength(demuxer);

	if (flag & GX_DEMUXER_SEEK_PERCENT){
		if(time >= 100 || time < 0)
			return GX_PLAYER_ERROR;
	}

	if ((flag & GX_DEMUXER_SEEK_RELATIVE) && (cur+time>=totle || cur+time<0))
		return GX_PLAYER_ERROR;
	if ((flag & GX_DEMUXER_SEEK_ABSOLUTE) && (time>=totle || time<0))
		return GX_PLAYER_ERROR;

	if (flag & GX_DEMUXER_SEEK_RELATIVE) {
		time += cur;
		flag &= (~GX_DEMUXER_SEEK_RELATIVE);
		flag |= GX_DEMUXER_SEEK_ABSOLUTE;
	}

	gxlogf("seek: time %lld\n", time);

	_demuxer_reset(demuxer);
	_demuxer_free_packs(demuxer);

	if ((!(flag & GX_DEMUXER_SEEK_PERCENT)) && (GX_STREAMTYPE_DEMUXER != demuxer->stream->file_format)){
		if(GxDemuxer_M3U8_Seek(demuxer, &time) != GX_PLAYER_OK)
			return GX_PLAYER_ERROR;
	}

	if (cls && cls->seek)
		ret = cls->seek(demuxer,time,0,flag);

	if (time > 0)
		_demuxer_sync_skip_audio(demuxer);
	_demuxer_stc_set_time(demuxer, time);
	_demuxer_reset(demuxer);

	if (demuxer->debug_dumper)
		GxDemuxDump_Close(demuxer->debug_dumper);
	demuxer->debug_dumper = GxDemuxDump_Open(demuxer);

	return ret;
}

static int _demuxer_control(GxDemuxer *demuxer, int cmd, void* args)
{
	int ret = GX_PLAYER_ERROR;
	GxDemuxerClass* dc = GetGxDemuxerClassFromObject(demuxer);

	if(cmd == GX_DEMUXER_CTRL_SWITCH_AUDIO) {
		if(demuxer->type != GX_DEMUXER_TYPE_HW && demuxer->type != GX_DEMUXER_TYPE_HW_TS) {
			if(demuxer->audio->sh) {
				GxStreamAudioHeader* sh = demuxer->audio->sh;

				demuxer->audio->dropmode = DROPMODE_AUDIOSWITCH;
				if (demuxer && dc && dc->control) {
					ret = dc->control(demuxer, cmd, args);
				}

				if(DEMUX_NEED_SYNC(demuxer))
					sh->priv.pts_sync = 1;
				GxFifo_Reset(demuxer->audio->pin->fifo);

				if ((demuxer->stream && (!demuxer->stream->is_hls_flg)) && demuxer->video->sh) {
					GxDemuxerClass *cls = GetGxDemuxerClassFromObject(demuxer);
					int a_stream_bitrate = demuxer->audio->compute_stream_bitrate;
					int v_stream_bitrate = demuxer->video->compute_stream_bitrate;
					int stream_bitrate = (a_stream_bitrate > v_stream_bitrate) ? a_stream_bitrate : v_stream_bitrate;

#define SWITCH_STREAM_BITRATE (8*1024*1024)
					if ((stream_bitrate <= SWITCH_STREAM_BITRATE) &&
							(cls && cls->seek)) {
						int64_t cur = GxDemuxer_GetCurrentTime(demuxer);
						int size = 0;

						gxlogd("----->stream bitrate: %d(bps) > %d(bps), seek stream\n",
								stream_bitrate, SWITCH_STREAM_BITRATE);
						if (cur > 0) {
							_demuxer_free_packs(demuxer);
							demuxer->video->fill_pkt_sync = 1;
							demuxer->audio->fill_pkt_sync = 1;
							cls->seek(demuxer, cur, 0, GX_DEMUXER_SEEK_ABSOLUTE|GX_DEMUXER_SEEK_BACKWARD);
							demuxer->pause_depack = 0;
							while (!demuxer->stream->eof) {
								unsigned char *start = NULL;
								int64_t pts = -1;

								size = GxDemuxStream_ProbePacketPts(demuxer->audio, &start, &pts, 1);
								if (size != 0) {
									GxDemuxerStreamSwitch *DemuxSwitchAudio = args;
									DemuxSwitchAudio->runflag = 1;
									break;
								}
							}
						}
					}
				}
			}
			ret = GX_PLAYER_OK;
			goto out;
		}
	}

	if (demuxer && dc && dc->control)
		ret = dc->control(demuxer, cmd, args);

	if (ret == GX_DEMUXER_CTRL_OK) {
		if (cmd == GX_DEMUXER_CTRL_OPEN_AD_AUDIO) {
			if(HAVE_AUDIO1(demuxer)) {
				GxPin* pin = GxMediaFilter_FindPin(demuxer, GX_DEMUX_PIN_NAME_ADAO);
				if (pin)
					demuxer->audio1->pin = pin;
				else {
					int subflag = GX_PINFLAG_ESA1;
					int esasize = swdmx.esasize;

					if ((demuxer->type == GX_DEMUXER_TYPE_HW) ||
							(demuxer->type == GX_DEMUXER_TYPE_HW_TS)) {
						subflag = GX_PINFLAG_TSA;
						esasize = GX_PINSIZE_TSA;
					}
					demuxer->audio1->pin = GxPin_Create(
							GX_DEMUX_PIN_NAME_ADAO,
							demuxer,
							GX_PINDIR_OUTPUT,
							esasize,
							subflag);
				}
			}
			ret = GX_PLAYER_OK;
		} else if (cmd == GX_DEMUXER_CTRL_CLOSE_AD_AUDIO) {
			demuxer->audio1->pin = NULL;
			ret = GX_PLAYER_OK;
		}
	} else {
		switch(cmd) {
		case GX_DEMUXER_CTRL_SYNC_AUDIO:
			if(args)
				demuxer->audio_sync = *(int*)args;
			ret = GX_PLAYER_OK;
			goto out;
		case GX_DEMUXER_CTRL_SYNC_SUB:
			if(args)
				demuxer->sub_sync = *(int*)args;
			ret = GX_PLAYER_OK;
			goto out;
		case GX_DEMUXER_CTRL_GET_VIDEO_LENGTH:
			if(demuxer->video->pin)
				*(unsigned int*)args = GxFifo_GetLength(demuxer->video->pin->fifo);
			else
				*(unsigned int*)args = 0;
			break;
		case GX_DEMUXER_CTRL_GET_AUDIO_LENGTH:
			if(demuxer->audio->pin)
				*(unsigned int*)args = GxFifo_GetLength(demuxer->audio->pin->fifo);
			else
				*(unsigned int*)args = 0;
			break;
		case GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE:
			if (HAVE_AUDIO(demuxer)) {
				demuxer->audio->dropmode = *(int*)args;
				GxDemuxStream_FreePacks(demuxer->audio);
				GxDemuxStream_Reset(demuxer->audio);
			}
			break;
		case GX_DEMUXER_CTRL_SET_AD_DROPMODE:
			if (HAVE_AUDIO1(demuxer)) {
				demuxer->audio1->dropmode = *(int*)args;
				GxDemuxStream_FreePacks(demuxer->audio1);
				GxDemuxStream_Reset(demuxer->audio1);
			}
			break;
		case GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE:
			if (HAVE_VIDEO(demuxer)) {
				demuxer->video->dropmode = *(int*)args;
				GxDemuxStream_FreePacks(demuxer->video);
				GxDemuxStream_Reset(demuxer->video);
			}
			break;
		case GX_DEMUXER_CTRL_RESET_AUDIO:
			if (HAVE_AUDIO(demuxer)) {
				GxDemuxStream_FreePacks(demuxer->audio);
				GxDemuxStream_Reset(demuxer->audio);
			}
			break;
		case GX_DEMUXER_CTRL_RESET_VIDEO:
			if (HAVE_VIDEO(demuxer)) {
				GxDemuxStream_FreePacks(demuxer->video);
				GxDemuxStream_Reset(demuxer->video);
			}
			break;
		case GX_DEMUXER_CTRL_START_MUX:
			if (demuxer->muxout == NULL)
				demuxer->muxout = GxPin_Create(GX_DEMUX_PIN_NAME_MUXO, demuxer, GX_PINDIR_OUTPUT, swdmx.dumpsize, GX_PINFLAG_MUXER);
			demuxer->muxing = 1;
			if (demuxer->muxout == NULL)
				ret = GX_PLAYER_ERROR;
			else
				ret = GX_PLAYER_OK;
			break;
		case GX_DEMUXER_CTRL_STOP_MUX:
			demuxer->muxing = 0;
			break;
		case GX_DEMUXER_CTRL_START_PLAY:
			{
				if((!HAVE_AUDIO(demuxer)) && (!HAVE_VIDEO(demuxer))) {
					ret = GX_PLAYER_ERROR;
					break;
				}

				if (demuxer->type != GX_DEMUXER_TYPE_HW && demuxer->type != GX_DEMUXER_TYPE_HW_TS && demuxer->type != GX_DEMUXER_TYPE_MPEG_ES && HAVE_VIDEO(demuxer)) {
					if (demuxer->poolbuf == NULL) {
						demuxer->poolbuf = av_malloc(swdmx.poolsize);
						if (demuxer->poolbuf == NULL) {
							ret = GX_PLAYER_ERROR;
							break;
						}
						GxDemuxPool_Init(&demuxer->pool, demuxer->poolbuf, swdmx.poolsize);
					}
				}

				if (HAVE_VIDEO(demuxer)) {
					if (demuxer->video->pin == NULL) {
						demuxer->video->pin = GxPin_Create(GX_DEMUX_PIN_NAME_VO,
								demuxer, GX_PINDIR_OUTPUT, swdmx.esvsize, GX_PINFLAG_ESV);
						if (demuxer->video->pin == NULL) {
							ret = GX_PLAYER_ERROR;
							break;
						}
					}
				}

				if(HAVE_SUB(demuxer)) {
					if (demuxer->sub->pin == NULL) {
						demuxer->sub->pin = GxPin_Create(GX_DEMUX_PIN_NAME_SO,
								demuxer, GX_PINDIR_OUTPUT, swdmx.esssize, GX_PINFLAG_SW);
						if (demuxer->sub->pin == NULL) {
							ret = GX_PLAYER_ERROR;
							break;
						}
					}
				}

				if (HAVE_AUDIO(demuxer)) {
					if (demuxer->audio->pin == NULL) {
						demuxer->audio->pin = GxPin_Create(GX_DEMUX_PIN_NAME_AO,
								demuxer, GX_PINDIR_OUTPUT, swdmx.esasize, GX_PINFLAG_ESA);
						if (demuxer->audio->pin == NULL) {
							ret = GX_PLAYER_ERROR;
							break;
						}
					}
				}

				if(HAVE_AUDIO1(demuxer)) {
					if (demuxer->audio1->pin == NULL) {
						int subflag = GX_PINFLAG_ESA1;
						int esasize = swdmx.esasize;

						if ((demuxer->type == GX_DEMUXER_TYPE_HW) ||
								(demuxer->type == GX_DEMUXER_TYPE_HW_TS)) {
							subflag = GX_PINFLAG_TSA;
							esasize = GX_PINSIZE_TSA;
						}
						if ((demuxer->audio1->pin = GxPin_Create(
										GX_DEMUX_PIN_NAME_ADAO,
										demuxer,
										GX_PINDIR_OUTPUT,
										esasize,
										subflag)) == NULL) {
							ret = GX_PLAYER_ERROR;
							break;
						}
					}
				}
				demuxer->playing = 1;
				break;
			}
		case GX_DEMUXER_CTRL_STOP_PLAY:
			demuxer->playing = 0;
			break;
		case GX_DEMUXER_CTRL_PAUSE_FILL_DATA:
			demuxer->pause_fill_fifo = 1;
			break;
		case GX_DEMUXER_CTRL_RESUME_FILL_DATA:
			demuxer->pause_fill_fifo = 0;
			break;
		default:
			ret = GX_PLAYER_ERROR;
			goto out;
		}
	}
out:
	return ret;
}


int GxDemuxer_Seek(GxDemuxer* demuxer, int64_t time, int flag)
{
	int ret;

	demuxer->pause_depack = 1;

	GxCore_MutexLock(demuxer->mutex);

	demuxer->speed = 1000;
	demuxer->video->fill_pkt_sync = 0;
	demuxer->audio->fill_pkt_sync = 0;
	demuxer->audio->fill_full_flags  = 0;
	demuxer->audio1->fill_full_flags = 0;
	demuxer->video->fill_full_flags  = 0;

	ret = _demuxer_seek(demuxer, time, flag|GX_DEMUXER_SEEK_BACKWARD);

	demuxer->pause_depack = 0;
	demuxer->fill_pkt_time = 0;

	_demuxer_stc_config(demuxer, 1000);

	CHANGE_DROPMODE(demuxer->sub->dropmode,    DROPMODE_NONE);
	CHANGE_DROPMODE(demuxer->audio->dropmode,  DROPMODE_NONE);
	CHANGE_DROPMODE(demuxer->audio1->dropmode, DROPMODE_NONE);

	if (HAVE_SUB(demuxer)) {
		_demuxer_control(demuxer, GX_DEMUXER_CTRL_SET_SUB_DROPMODE,   &demuxer->sub->dropmode);
	}

	if (HAVE_AUDIO(demuxer)) {
		GxStreamAudioHeader* sh = demuxer->audio->sh;

		sh->stat.decode_frame_cnt = 0;
		_demuxer_control(demuxer, GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE, &demuxer->audio->dropmode);
	}

	if (HAVE_AUDIO1(demuxer)) {
		GxStreamAudioHeader* sh = demuxer->audio1->sh;

		sh->stat.decode_frame_cnt = 0;
		_demuxer_control(demuxer, GX_DEMUXER_CTRL_SET_AD_DROPMODE,    &demuxer->audio1->dropmode);
	}

	if (HAVE_VIDEO(demuxer)) {
		GxStreamVideoHeader* sh = demuxer->video->sh;
		sh->stat.decode_frame_cnt = 0;
	}

	GxCore_MutexUnlock(demuxer->mutex);

	return ret;
}

status_t GxDemuxer_SeamlessBandwidthSwitch(GxDemuxer* demuxer, unsigned int bandwidth)
{
	status_t ret = GX_PLAYER_ERROR;
	GxDemuxerStreamSwitch StreamSwitch = {0};
	if (bandwidth >= 0) {
		StreamSwitch.pid = bandwidth;
		ret = _demuxer_control(demuxer, GX_DEMUXER_CTRL_SEAMLESS_BANDWIDTH_SWITCH, &StreamSwitch);
	}
	return ret;
}

int GxDemuxer_Speed(GxDemuxer* demuxer, int speed, int flush, int have_audio, int need_audio)
{
	int64_t totle = 0, current = 0;

	if (!HAVE_VIDEO(demuxer) && !GX_SPEED_NORMAL(speed))
		return GX_PLAYER_ERROR;

	demuxer->pause_depack = 1;
	demuxer->video->fill_pkt_sync = 0;
	demuxer->audio->fill_pkt_sync = 0;
	GxCore_MutexLock(demuxer->mutex);

	current = GxDemuxer_GetCurrentTime(demuxer);
	if (flush) {
		totle   = GxDemuxer_GetTimeLength(demuxer);
		if (demuxer->type != GX_DEMUXER_TYPE_HW_TS) {
			current += (speed   > 0 ? 2000 : -2000  );
			current  = (current < 0 ? 0    : current);
		}
	}

	_demuxer_stc_pause(demuxer);

	if (need_audio){
		CHANGE_DROPMODE(demuxer->audio->dropmode,  DROPMODE_NONE);
		CHANGE_DROPMODE(demuxer->audio1->dropmode, DROPMODE_NONE);
	} else {
		CHANGE_DROPMODE(demuxer->audio->dropmode,  DROPMODE_FFFB);
		CHANGE_DROPMODE(demuxer->audio1->dropmode, DROPMODE_FFFB);
	}

	if (have_audio != need_audio) {
		if (HAVE_AUDIO(demuxer))
			_demuxer_control(demuxer, GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE, &demuxer->audio->dropmode);
		if (HAVE_AUDIO1(demuxer))
			_demuxer_control(demuxer, GX_DEMUXER_CTRL_SET_AD_DROPMODE,   &demuxer->audio1->dropmode);
	}

	_demuxer_control(demuxer, GX_DEMUXER_CTRL_UPDATE_TIMEMS, &current);
	demuxer->speed = speed;

	if (flush) {
		if (current >= totle) {
			demuxer->stream->eof = demuxer->audio->eof = demuxer->video->eof = 1;
			goto end;
		}
		_demuxer_seek(demuxer, current, GX_DEMUXER_SEEK_ABSOLUTE | GX_DEMUXER_SEEK_FRAME);
	}

end:
	_demuxer_stc_config(demuxer, speed);
	_demuxer_stc_resume(demuxer);
	demuxer->pause_depack = 0;
	GxCore_MutexUnlock(demuxer->mutex);

	return GX_PLAYER_OK;
}

int GxDemuxer_ReadPesData(GxDemuxer *demuxer, uint8_t* buffer, int size)
{
	int ret = GX_DEMUXER_CTRL_ERROR;
	GxDemuxerPesData pes_data;
	GxDemuxerClass* dc = GetGxDemuxerClassFromObject(demuxer);

	pes_data.buffer = buffer;
	pes_data.size   = size;

	GxCore_MutexLock(demuxer->mutex);
	if (demuxer && dc && dc->control)
		ret = dc->control(demuxer, GX_DEMUXER_CTRL_SUB_READ_DATA, &pes_data);
	GxCore_MutexUnlock(demuxer->mutex);

	if (ret == GX_DEMUXER_CTRL_ERROR)
		return -1;

	return pes_data.size;
}

int GxDemuxer_ReadPsiData(GxDemuxer *demuxer, uint8_t* buffer, int size)
{
	int ret = GX_DEMUXER_CTRL_ERROR;
	GxDemuxerPsiData psi_data;
	GxDemuxerClass* dc = GetGxDemuxerClassFromObject(demuxer);

	psi_data.buffer = buffer;
	psi_data.size   = size;

	GxCore_MutexLock(demuxer->mutex);
	if (demuxer && dc && dc->control)
		ret = dc->control(demuxer, GX_DEMUXER_CTRL_SUB_PSI_READ_DATA, &psi_data);
	GxCore_MutexUnlock(demuxer->mutex);

	if (ret == GX_DEMUXER_CTRL_ERROR)
		return -1;

	return psi_data.size;
}

int GxDemuxer_Control(GxDemuxer *demuxer, int cmd, void* args)
{
	int ret = GX_PLAYER_ERROR;

	if(cmd == GX_DEMUXER_CTRL_SET_BASE_TIME){
		GxStreamSeek seq_info = {0,0,-1};
		if (args == NULL)
			seq_info.seek_seq = 0;
		else
			seq_info.seek_seq = *(int*)args;
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TIME_BYSEQ, &seq_info);
		demuxer->base_time = seq_info.seek_ms;
		return GX_PLAYER_OK;
	}

	demuxer->pause_depack = 1;
	GxCore_MutexLock(demuxer->mutex);
	ret = _demuxer_control(demuxer, cmd, args);
	demuxer->pause_depack = 0;
	GxCore_MutexUnlock(demuxer->mutex);

	return ret;
}

void GxDemuxer_DebugData(GxDemuxer *demuxer)
{
	int audio0_demux_size = 0;
	int audio1_demux_size = 0;
	int video_demux_size = 0;
	int esa0_size = 0;
	int esa1_size = 0;
	int esv_size = 0;

	if(demuxer->audio->sh) {
		audio0_demux_size = demuxer->audio->bytes;
		if(demuxer->audio->pin && demuxer->audio->pin->fifo)
			esa0_size = GxFifo_GetLength(demuxer->audio->pin->fifo);
	}

	if(demuxer->audio1->sh) {
		audio1_demux_size = demuxer->audio1->bytes;
		if(demuxer->audio1->pin && demuxer->audio1->pin->fifo)
			esa1_size = GxFifo_GetLength(demuxer->audio1->pin->fifo);
	}

	if(demuxer->video->sh) {
		video_demux_size = demuxer->video->bytes;
		if(demuxer->video->pin && demuxer->video->pin->fifo)
			esv_size = GxFifo_GetLength(demuxer->video->pin->fifo);
	}

	gxlogi_raw("<fill - ea0:%.02fK\tea1:%.02fK\tev:%.02fK,\tda0:%.02f(K)\tda1:%.02f(K)\tdv%.02f(K),\tsc:%.02f(K)\n",
			((double)esa0_size)/1024, ((double)esa1_size)/1024, ((double)esv_size)/1024,
			((double)audio0_demux_size)/1024, ((double)audio1_demux_size)/1024, ((double)video_demux_size)/1024,
			((double)(demuxer->stream->buf_len - demuxer->stream->buf_pos)/1024));

	return ;
}

uint64_t GxDemuxer_GetTimeLength(GxDemuxer *demuxer)
{
	uint64_t get_time_ans = 0 ;

	if(demuxer) {
		GxStreamVideoHeader* sh_video = demuxer->video->sh;
		GxStreamAudioHeader* sh_audio = demuxer->audio->sh;
		if (_demuxer_control(demuxer, GX_DEMUXER_CTRL_GET_TIME_LENGTH, (void*)&get_time_ans) != GX_PLAYER_OK)
		{
			if (sh_audio && !sh_audio->i_bps) {
				if (demuxer->audio->pin && demuxer->audio->pin->link) {
					GxMediaFilter* next_mf = demuxer->audio->pin->link->filter;
					GxAudioDecProperty_FrameInfo frame;

					frame.bitrate = 0;
					GxAudioDecoder_Control((GxAudioCodec*)next_mf, GX_AD_CTRL_GET_FRAME_INFO, &frame);
					sh_audio->i_bps = frame.bitrate / 8;
				}
			}

			if (sh_video && !sh_video->i_bps) {
				//视频码率获取不到时，考虑从解码器获取
			}

			if (sh_video && sh_video->i_bps && sh_audio && sh_audio->i_bps)
				get_time_ans = (demuxer->movi_end - demuxer->movi_start) / (sh_video->i_bps + sh_audio->i_bps);
			else if (sh_video && sh_video->i_bps)
				get_time_ans = (demuxer->movi_end - demuxer->movi_start) / (sh_video->i_bps);
			else if (sh_audio && sh_audio->i_bps)
				get_time_ans = (demuxer->movi_end - demuxer->movi_start) / (sh_audio->i_bps);
			else
				get_time_ans = 0;

			get_time_ans *= 1000;
		}
	}

	return get_time_ans;
}

uint64_t GxDemuxer_GetSeekMinTime(GxDemuxer *demuxer)
{
	int64_t seekmin_ms = 0;

	if(demuxer->type == GX_DEMUXER_TYPE_MPEG_TS
			|| demuxer->type == GX_DEMUXER_TYPE_HW
			|| demuxer->type == GX_DEMUXER_TYPE_HW_TS) {
		if(_demuxer_control(demuxer, GX_DEMUXER_CTRL_GET_SEEKMIN_TIME, &seekmin_ms) == GX_PLAYER_OK)
		{
			if(seekmin_ms >= 0)
				return seekmin_ms;
		}
	}
	return 0;
}

uint64_t GxDemuxer_GetCurrentTime(GxDemuxer *demuxer)
{
	int64_t current = 0, seek_minims;
	GxMediaFilter* filter = GXMEDIAFILTER(demuxer);

	if (filter->status == GX_MFT_STATE_STOPPED)
		return demuxer->cur_timems;

	seek_minims = (int64_t)GxDemuxer_GetSeekMinTime(demuxer);

	if ((!GX_SPEED_JUMP(demuxer->stcfreq)) || (demuxer->type == GX_DEMUXER_TYPE_HW_TS)) {
		current = -1;
		if (_demuxer_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &current) == GX_PLAYER_OK) {
			if (current >= 0) {
				if (current < seek_minims)
					current = seek_minims;
				demuxer->last_pts = current;
			}
			return (uint64_t)demuxer->last_pts;
		}
	}

	if (demuxer->video->pin && demuxer->video->pin->link &&
			(GX_SPEED_FAST(demuxer->stcfreq) ||
			 GX_SPEED_SLOW(demuxer->stcfreq) ||
			 (GX_SPEED_NORMAL(demuxer->stcfreq) && (!HAVE_AUDIO(demuxer) || demuxer->audio->dropmode)))) {
		int32_t vpts;
		GxMediaFilter* next_mf = demuxer->video->pin->link->filter;
		GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_GET_CUR_PTS, &vpts);
		current = vpts;
		if (current <= 0 && demuxer->last_pts != -1)
			goto end;
	}

	if (current <= 0) {
		current = _demuxer_stc_get_time(demuxer);
	}

	if (demuxer->type == GX_DEMUXER_TYPE_HW)
		current = current / (demuxer->stcfreq / 1000);
	else
		current = current / demuxer->stc_factor;

	if (demuxer->type == GX_DEMUXER_TYPE_MPEG_TS ||
			demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH)
		current -= demuxer->start_pts;

	if (current < seek_minims)
		current = seek_minims;

	if (current > demuxer->duration && demuxer->duration > 0)
		current = demuxer->duration;

	demuxer->last_pts = current;

end:
	if ((demuxer->stream->file_format != GX_STREAMTYPE_STREAM) &&
			(demuxer->type == GX_DEMUXER_TYPE_MPEG_TS)) {
		off_t cur_pos, tell_pos, buf_pos;

		tell_pos  = GxStream_Tell(demuxer->stream);
		buf_pos = GxDemuxer_GetInBufSize(demuxer);
		cur_pos = tell_pos - buf_pos;

		if (demuxer->duration > 0) {
			if (cur_pos > demuxer->tell_pos) {
				if (demuxer->last_pts < demuxer->cur_timems)
					demuxer->last_pts = demuxer->cur_timems;
			} else if (cur_pos == demuxer->tell_pos) {
				demuxer->last_pts = demuxer->cur_timems;
			} else {
				if (demuxer->last_pts > demuxer->cur_timems)
					demuxer->last_pts = demuxer->cur_timems;
			}

			if (demuxer->last_pts == 0)
				demuxer->last_pts = demuxer->cur_timems;

			demuxer->cur_timems = demuxer->last_pts;
			demuxer->tell_pos = cur_pos;
		}
	}

	return (uint64_t)demuxer->last_pts;
}

uint8_t GxDemuxer_GetCurrentPercent(GxDemuxer *demuxer)
{
	int32_t current=0;

	if(_demuxer_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_PERCENT, &current)==GX_PLAYER_OK)
	{
		if(current < 0 || current > 100)
		{
			current = 0;
			gxlogf("GetCurrentPercent inner error \n");
		}
	}

	return current;
}


int32_t GxDemuxer_GetCurrentSTC(GxDemuxer *demuxer)
{
	int32_t current;

	current = _demuxer_stc_get_time(demuxer);
	if(demuxer->type == GX_DEMUXER_TYPE_HW)
		current = current/(demuxer->stcfreq/1000);
	else
		current = current/demuxer->stc_factor;

	return current;
}

int32_t GxDemuxer_GetCurrentVpts(GxDemuxer *demuxer)
{
	int32_t vpts = -1;

	if (demuxer->video->pin && demuxer->video->pin->link) {
		GxMediaFilter* next_mf = demuxer->video->pin->link->filter;
		GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_GET_CUR_PTS, &vpts);
		if (demuxer->type == GX_DEMUXER_TYPE_HW)
			vpts = vpts / (demuxer->stcfreq / 1000);
		else
			vpts = vpts / demuxer->stc_factor;
	}

	return vpts;
}

void GxDemuxer_STCReset(GxDemuxer *demuxer)
{
	GxSTCProperty_Time Time;
	Time.time = 0;
	GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Play, NULL, 0);
	GxAVSetProperty(swdmx.dev, swdmx.mod_stc, GxSTCPropertyID_Time, &Time, sizeof(GxSTCProperty_Time));
}

void GxDemuxer_STCPause(GxDemuxer *demuxer)
{
	GxAVSetProperty(swdmx.dev,swdmx.mod_stc,GxSTCPropertyID_Pause, NULL, 0);
}

void GxDemuxer_STCResume(GxDemuxer *demuxer)
{
	GxAVSetProperty(swdmx.dev,swdmx.mod_stc,GxSTCPropertyID_Resume, NULL, 0);
}

unsigned int GxDemuxer_GetInBufSize(GxDemuxer *demuxer)
{
	int demuxed_size = 0;
	GxPin *inpin = GxMediaFilter_FindPin(demuxer, GX_DEMUX_PIN_NAME_IN);

	if (inpin && inpin->fifo)
		demuxed_size += GxFifo_GetLength(inpin->fifo);

	if(demuxer->audio->sh) {
		demuxed_size += demuxer->audio->bytes;
		if(demuxer->audio->pin && demuxer->audio->pin->fifo)
			demuxed_size += GxFifo_GetLength(demuxer->audio->pin->fifo);
	}
	if(demuxer->video->sh) {
		demuxed_size += demuxer->video->bytes;
		if(demuxer->video->pin && demuxer->video->pin->fifo)
			demuxed_size += GxFifo_GetLength(demuxer->video->pin->fifo);
	}
	if(demuxer->sub->sh) {
		demuxed_size += demuxer->sub->bytes;
		if(demuxer->sub->pin && demuxer->sub->pin->fifo)
			demuxed_size += GxFifo_GetLength(demuxer->sub->pin->fifo);
	}

	return demuxed_size;
}

int GxDemuxer_GetEsBufPercent(GxDemuxer *demuxer, int *esa_percent, int *esv_percent)
{
	if (demuxer) {

		if (esa_percent) {

			*esa_percent = 0;
			if(swdmx.esasize > 0) {
				if(demuxer->audio->pin && demuxer->audio->pin->fifo)
					*esa_percent = 100 * GxFifo_GetLength(demuxer->audio->pin->fifo) / swdmx.esasize;
			}
		}

		if (esv_percent) {

			*esv_percent = 0;
			if(swdmx.esvsize > 0) {
				if(demuxer->video->pin && demuxer->video->pin->fifo)
					*esv_percent = 100 * GxFifo_GetLength(demuxer->video->pin->fifo) / swdmx.esvsize;
			}
		}

		return 0;
	}

	return -1;
}

unsigned int GxDemuxer_GetInBufPercent(GxDemuxer *demuxer)
{
	unsigned int percent = 100;
	unsigned int esasize = 0, esa1size = 0, esvsize = 0;
	unsigned int esv_fifo_cap = 0, esa_fifo_cap = 0, esa1_fifo_cap = 0;
	unsigned int totle_size = 0, data_size = 0;
	AVRational esv_prefill = {30, 100};
	AVRational esa_prefill = {80, 100};

	if (GxDemuxer_AlmostFull(demuxer))
		return percent;

	if(demuxer->audio->sh) {
		if(demuxer->audio->pin && demuxer->audio->pin->fifo) {
			esasize = GxFifo_GetLength(demuxer->audio->pin->fifo);
			esa_fifo_cap = GxFifo_GetCap(demuxer->audio->pin->fifo);
		}
	}

	if(demuxer->audio1->sh) {
		if(demuxer->audio1->pin && demuxer->audio1->pin->fifo) {
			esa1size = GxFifo_GetLength(demuxer->audio1->pin->fifo);
			esa1_fifo_cap = GxFifo_GetCap(demuxer->audio1->pin->fifo);
		}
	}

	if(demuxer->video->sh) {
		if(demuxer->video->pin && demuxer->video->pin->fifo) {
			esvsize = GxFifo_GetLength(demuxer->video->pin->fifo);
			esv_fifo_cap = GxFifo_GetCap(demuxer->video->pin->fifo);
			GxStreamVideoHeader  tmp_sh;
			GxMediaFilter* next_mf = demuxer->video->pin->link->filter;
			GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_GET_FRAME_INFO, &tmp_sh);
			if (tmp_sh.disp_w >= 1280 && tmp_sh.disp_h >= 720) {
				esv_prefill.num= 80;
			}
		}
	}
	totle_size = (esa_fifo_cap + esa1_fifo_cap)*esa_prefill.num/esa_prefill.den+ esv_fifo_cap*esv_prefill.num/esv_prefill.den;
	data_size  = esa1size + esa1size + esvsize;
	percent = (totle_size > 0) ? (100 * data_size / totle_size) : 0;
	percent = AV_MIN(percent, 100);
	return percent;
}

int GxDemuxer_GetEsBufSize(GxDemuxer *demuxer, int *esa_size, int *esv_size)
{
	if (demuxer) {
		if (esa_size) {
			*esa_size = demuxer->audio->bytes;
			if (demuxer->audio->pin && demuxer->audio->pin->fifo)
				*esa_size += GxFifo_GetLength(demuxer->audio->pin->fifo);
		}

		if (esv_size) {
			*esv_size = demuxer->video->bytes;
			if (demuxer->video->pin && demuxer->video->pin->fifo)
				*esv_size += GxFifo_GetLength(demuxer->video->pin->fifo);
		}
	}

	return 0;
}

int GxDemuxer_GetSTCFreq(GxDemuxer *demuxer)
{
	GxSTCProperty_TimeResolution TimeResolution;

	GxAVGetProperty(swdmx.dev, swdmx.mod_stc,
			GxSTCPropertyID_TimeResolution, &TimeResolution, sizeof(GxSTCProperty_TimeResolution));

	return (TimeResolution.freq_HZ == 0) ? 45000 : TimeResolution.freq_HZ;
}

int GxDemuxer_GetSTCSyncMode(GxDemuxer *demuxer)
{
	GxSTCProperty_Config config;

	GxAVGetProperty(swdmx.dev, swdmx.mod_stc,
			GxSTCPropertyID_Config, &config, sizeof(GxSTCProperty_Config));

	return config.mode;
}

unsigned char GxDemuxer_AlmostFull(GxDemuxer *demuxer)
{
	unsigned char full = 0;
	int totle_bytes = 0;
	unsigned int esasize = 0, esa1size = 0, esvsize = 0;
	unsigned int esv_fifo_cap = 0, esa_fifo_cap = 0, esa1_fifo_cap = 0;
	unsigned int pcm_size = 0, audio_demux_size = 0, audio1_demux_size = 0, video_demux_size = 0;
	AVRational esv_prefill = {30, 100};
	AVRational esa_prefill = {80, 100};

	if(demuxer->video->sh && demuxer->video->dropmode != DROPMODE_UNSUPPORT) {
		video_demux_size = demuxer->video->bytes;
		if(demuxer->video->pin && demuxer->video->pin->fifo) {
			esvsize = GxFifo_GetLength(demuxer->video->pin->fifo);
			esv_fifo_cap = GxFifo_GetCap(demuxer->video->pin->fifo);
			GxStreamVideoHeader  tmp_sh;
			GxMediaFilter* next_mf = demuxer->video->pin->link->filter;
			GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_GET_FRAME_INFO, &tmp_sh);
			if (tmp_sh.disp_w >= 1280 && tmp_sh.disp_h >= 720) {
				esv_prefill.num = 80;
			}
			full |= (esvsize >= (esv_fifo_cap*esv_prefill.num/esv_prefill.den))?1:0;
		}
	}

	if(demuxer->audio->sh && demuxer->audio->dropmode != DROPMODE_UNSUPPORT) {
		audio_demux_size = demuxer->audio->bytes;
		if(demuxer->audio->pin && demuxer->audio->pin->fifo) {
			esasize = GxFifo_GetLength(demuxer->audio->pin->fifo);
			esa_fifo_cap = GxFifo_GetCap(demuxer->audio->pin->fifo);
			GxMediaFilter* next_mf = demuxer->audio->pin->link->filter;
			GxAudioDecoder_Control((GxAudioCodec*)next_mf, GX_AD_CTRL_GET_PCM_SIZE, &pcm_size);
			full |= (esasize >= (esa_fifo_cap*esa_prefill.num/esa_prefill.den))?1:0;
		}
	}

	if(demuxer->audio1->sh && demuxer->audio1->dropmode != DROPMODE_UNSUPPORT) {
		audio1_demux_size = demuxer->audio1->bytes;
		if(demuxer->audio1->pin && demuxer->audio1->pin->fifo) {
			esa1size = GxFifo_GetLength(demuxer->audio1->pin->fifo);
			esa1_fifo_cap = GxFifo_GetCap(demuxer->audio1->pin->fifo);
			GxMediaFilter* next_mf = demuxer->audio1->pin->link->filter;
			full |= (esa1size >= (esa1_fifo_cap*esa_prefill.num/esa_prefill.den))?1:0;
		}
	}

	if(demuxer->audio)
		totle_bytes += demuxer->audio->bytes;
	if(demuxer->audio1)
		totle_bytes += demuxer->audio1->bytes;
	if(demuxer->video)
		totle_bytes += demuxer->video->bytes;

	if (totle_bytes >= (demuxer->swdmx->cachesize*4/5)) {
		gxlogf ("[Player] cache full. totle_bytes=%d cachesize=%d.\n",totle_bytes, demuxer->swdmx->cachesize);
		full = 1;
	}

	gxlogf ("pcm:%.02fK\t ea:%.02fK(cap:%.02fK)\t ea1:%.02fK(cap:%.02fK)\t ev:%.02fK(cap:%.02fK)\t da:%.02f(K),\t da1:%.02f(K),\t dv%.02f(K)\t full:%d\n",
			((double)pcm_size)/1024,
			((double)esasize)/1024, ((double)esa_fifo_cap)/1024,
			((double)esa1size)/1024, ((double)esa1_fifo_cap)/1024,
			((double)esvsize)/1024, ((double)esv_fifo_cap)/1024,
			((double)audio_demux_size)/1024, ((double)audio1_demux_size)/1024, ((double)video_demux_size)/1024,
			full);
	return full;
}

unsigned char GxDemuxer_AlmostEmpty(GxDemuxer *demuxer)
{
	int audio_demux_size = 0;
	int video_demux_size = 0;
	int esa_size = 0;
	int pcm_size = 0;
	int esv_size = 0;

	if(demuxer->stream && demuxer->stream->eof)
		return 0;

	if(demuxer->audio->sh) {
		audio_demux_size = demuxer->audio->bytes;
		if (demuxer->audio->pin && demuxer->audio->pin->fifo) {
			GxMediaFilter* next_mf = demuxer->audio->pin->link->filter;
			GxAudioDecoder_Control((GxAudioCodec*)next_mf, GX_AD_CTRL_GET_PCM_SIZE, &pcm_size);
			esa_size = GxFifo_GetLength(demuxer->audio->pin->fifo);
		}
	}

	if(demuxer->video->sh) {
		video_demux_size = demuxer->video->bytes;
		if (demuxer->video->pin && demuxer->video->pin->fifo) {
			esv_size = GxFifo_GetLength(demuxer->video->pin->fifo);
		}
	}

	gxlogf("pcm:%.02fK\t ea:%.02fK\t ev:%.02fK\t da:%.02f(K),\t dv%.02f(K)\t sc:%.02f(K)\n",
			((double)pcm_size)/1024, ((double)esa_size)/1024, ((double)esv_size)/1024,
			((double)audio_demux_size)/1024, ((double)video_demux_size)/1024,
			((double)(demuxer->stream->buf_len - demuxer->stream->buf_pos)/1024));
	if (((demuxer->video->dropmode != DROPMODE_UNSUPPORT) && demuxer->video->sh && (esv_size < 10240)) ||
			((demuxer->audio->dropmode != DROPMODE_UNSUPPORT) && demuxer->audio->sh && (pcm_size < 4072) && (esa_size < 512)))
		return 1;

	return 0;
}

int GxDemuxer_ChapterAdd(GxDemuxer *demuxer, const char *name, uint64_t start, uint64_t end)
{
	GxDemuxChapter* chapter;

	chapter = av_malloc(sizeof(GxDemuxChapter));

	if(chapter != NULL) {
		memset(chapter,0,sizeof(GxDemuxChapter));

		if (demuxer->chapters == NULL) {
			chapter->next = NULL;
			demuxer->chapters = chapter;
		}
		else {
			chapter->next = demuxer->chapters;
			demuxer->chapters = chapter;
		}
		demuxer->chapters->start = start/demuxer->movi_end;
		demuxer->chapters->end   = end/demuxer->movi_end;
		demuxer->chapters->name  = av_strdup(name);

		return demuxer->num_chapters++;
	}

	return GX_PLAYER_ERROR;

}

GxDemuxChapter* GxDemuxer_ChapterFind(GxDemuxer * demuxer, const char *name)
{
	GxDemuxChapter* chapter;

	chapter = demuxer->chapters;

	while (chapter) {
		if (!strcmp(chapter->name, name))
			return chapter;
		chapter = chapter->next;
	}

	return NULL;
}

int GxDemuxer_ChapterRemove(GxDemuxer *demuxer, const char *name)
{
	GxDemuxChapter* chapter;

	chapter = GxDemuxer_ChapterFind(demuxer,name);

	if (chapter) {
		av_free(chapter->name);
		av_free(chapter);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

int GxDemuxer_ChapterRemoveAll(GxDemuxer *demuxer)
{
	GxDemuxChapter* chapter;

	chapter = demuxer->chapters;

	while (chapter) {
		demuxer->chapters = chapter->next;
		av_free(chapter->name);
		av_free(chapter);
		chapter = demuxer->chapters;
	}

	return GX_PLAYER_OK;
}

void GxDemuxerClass_Init(void)
{
	GxClassRegister(&gx_DemuxerBase);
	GxClassInitTable((void* *)gx_demuxer_classes);
}

void GxDemuxerClass_Destroy(void)
{
	int i;
	for (i = 0; gx_demuxer_classes[i]; i++)
		GxClassUnregister(gx_demuxer_classes[i]);
	GxClassUnregister(&gx_DemuxerBase);
}

int GxDemuxer_PrivatePacketSyncExtInfo(GxStream *stream)
{
#define MAX_EXTINFO_CHECK_SIZE (128 * 1024)
	int c=0, synced=0;
	off_t synced_pos;
	off_t npos = GxStream_Tell(stream);

resync:
	if (stream->interrupt_cbk && stream->interrupt_cbk()) {
		gxlogd("###### force exit########\n");
		return synced;
	}

	while(((c = GxStream_ReadChar(stream)) != 0x47) && !stream->eof){
		if(GxStream_Tell(stream) - npos >= MAX_EXTINFO_CHECK_SIZE)
			return 0;
	}

	if(c == 0x47) {
		synced_pos = GxStream_Tell(stream);
		if ((GxStream_ReadChar(stream) == 0xff) &&
				(GxStream_ReadChar(stream) == 0xff) &&
				((GxStream_ReadChar(stream) & 0xF0) == 'P') &&
				((GxStream_ReadChar(stream) & 0xF3) == 'R') &&
				((GxStream_ReadChar(stream) & 0xCF) == 'I') &&
				(GxStream_ReadChar(stream) == 'V') &&
				(GxStream_ReadChar(stream) == 'T')) {
			GxStream_Seek(stream, synced_pos-1);
			synced = 1;
		} else {
			GxStream_Seek(stream, synced_pos+1);
			goto resync;
		}
	}
	else
		synced = 0;

	return synced;
}

static int GxDemuxer_PrivatePacketSync(GxStream *stream, unsigned int packet_size)
{
	int c=0, synced=0;
	off_t synced_pos;

resync:
	while(((c = GxStream_ReadChar(stream)) != 0x47) && !stream->eof);
	if(c == 0x47) {
		synced_pos = GxStream_Tell(stream);
		GxStream_Skip(stream, packet_size-1);
		if(GxStream_ReadChar(stream) != 0x47){
			GxStream_Seek(stream, synced_pos+1);
			goto resync;
		}
		else {
			GxStream_Skip(stream, packet_size-1);
			if(GxStream_ReadChar(stream) != 0x47){
				GxStream_Seek(stream, synced_pos+1);
				goto resync;
			}
			else {
				GxStream_Seek(stream, synced_pos-1);
				synced = 1;
			}
		}
	}
	else
		synced = 0;

	return synced;
}

int GxDemuxer_PrivatePacketCreate(char **ret_buf, unsigned int *ret_buflen, GxDemuxerPrivateTSInfo *priv_info)
{
#define PACKET_MAJOR_VER (0)
#define PACKET_MINOR_VER (1)
#define PACKET_EXTRA_VER (2)
	unsigned char *ts_buf = NULL, *ts_ptr = NULL;
	unsigned int  *ts_int_buf = NULL;
	unsigned int   ts_len = 0;
	unsigned int   ts_packets = 0;
	unsigned int   packet_len = 0;
	GxDemuxerPrivatePacketV01    *packet1 = NULL;
	GxDemuxerPrivatePacketExtV01 *extdat1 = NULL;
	GxDemuxerPrivatePacketV02    *packet2 = NULL;
	unsigned int i = 0;

	packet_len  = (sizeof(GxDemuxerPrivatePacketV01));
	packet_len += (sizeof(GxDemuxerPrivatePacketExtV01) * priv_info->ext_info.ext_num);
	packet_len += (sizeof(GxDemuxerPrivatePacketV02));
	ts_packets  = ((packet_len / 184) + ((packet_len % 184) ? 1 : 0));
	ts_len = ts_packets * 188;
	ts_buf = av_mallocz(ts_len);
	if(!ts_buf)
		return GX_PLAYER_ERROR;

	ts_buf[0] = (('R')|(PACKET_MAJOR_VER<<2));  //[4]
	ts_buf[1] = (('I')|(PACKET_MINOR_VER<<4));  //[5]
	ts_buf[2] = 'V';  //[6]
	ts_buf[3] = 'T';  //[7]
	ts_int_buf = (unsigned int *)(ts_buf + 4);
	ts_int_buf[0] = ts_len + priv_info->user_info.userdata_len;

	ts_ptr  = ts_buf;
	packet1 = (GxDemuxerPrivatePacketV01*)ts_ptr;
	ts_ptr += (sizeof(GxDemuxerPrivatePacketV01));
	extdat1 = (GxDemuxerPrivatePacketExtV01*)ts_ptr;
	ts_ptr += (sizeof(GxDemuxerPrivatePacketExtV01) * priv_info->ext_info.ext_num);
	packet2 = (GxDemuxerPrivatePacketV02*)ts_ptr;

	//desc info
	packet1->ts_packets = ts_packets;
	packet1->ts_length  = ts_len;
	packet1->es_length  = packet_len;

	//prog info
	packet1->a_pid   = priv_info->a_pid;
	packet1->v_pid   = priv_info->v_pid;
	packet1->a_fmt   = priv_info->a_fmt;
	packet1->v_fmt   = priv_info->v_fmt;
	packet1->pcr_pid = priv_info->pcr_pid;
	packet1->hdr_len = priv_info->header_info.len + ts_len;
	packet1->timestamp_pid = priv_info->timestamp_pid;
	packet1->version = PACKET_EXTRA_VER;

	//ext info
	packet1->is_encrypt  = priv_info->ext_info.is_encrypt;
	packet1->service_id  = priv_info->ext_info.service_id;
	packet1->ext_data[0] = priv_info->ext_info.ext_data[0];
	packet1->ext_data[1] = priv_info->ext_info.ext_data[1];
	packet1->ext_data[2] = priv_info->ext_info.ext_data[2];
	packet1->ext_data[3] = priv_info->ext_info.ext_data[3];
	packet1->ext_num     = priv_info->ext_info.ext_num;
	for (i = 0; i < packet1->ext_num; i++) {
		GxDemuxerPrivatePacketExtV01 *tmp_extdat = &extdat1[i];
		tmp_extdat->ext_pids              = priv_info->ext_info.ext_pids[i];
		tmp_extdat->ext_pids_content      = priv_info->ext_info.ext_pids_content[i];
		tmp_extdat->ext_pids_content_type = priv_info->ext_info.ext_pids_content_type[i];
		tmp_extdat->ext_pids_major        = priv_info->ext_info.ext_pids_major[i];
		tmp_extdat->ext_pids_minor        = priv_info->ext_info.ext_pids_minor[i];
		tmp_extdat->ext_pids_codec        = priv_info->ext_info.ext_pids_codec[i];
		tmp_extdat->ext_pids_name         = priv_info->ext_info.ext_pids_name[i];
		tmp_extdat->ext_pids_lang         = priv_info->ext_info.ext_pids_lang[i];
	}

	//prv info
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_SIZEMB,   &packet1->volume_sizemb);
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXNUM,   &packet1->volume_maxnum);
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_FULLSTOP, &packet1->volume_fullstop);
	GxPlayer_SystemGet(PSYS_PVR_RESERVE_SIZEMB,  &packet1->reserve_sizemb);
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXTIMES, &packet1->volume_maxtimes);

	//user info
	packet1->have_pmt = priv_info->user_info.have_pmt;
	packet1->user_len = priv_info->user_info.userdata_len;

	//ctrl info
	packet2->user_encrypt_enable    = priv_info->ctrl_info.encrypt_enable;
	packet2->user_encrypt_blocksize = priv_info->ctrl_info.block_size;
	packet2->tsw_security           = priv_info->ctrl_info.tsw_security;
	packet2->tsw_ptr_en             = priv_info->ctrl_info.tsw_ptr_en;

	for (i = 0; i < ts_packets; i++) {
		unsigned char *tmp_buf = &ts_buf[i * 188];
		unsigned int   tmp_len = ts_len - i * 188 - 4;

		memmove(tmp_buf + 4, tmp_buf, tmp_len);
		tmp_buf[0] = 0x47;
		tmp_buf[1] = 0xFF;
		tmp_buf[2] = 0xFF;
		tmp_buf[3] = 'P';//CC-4bit|有无自适应域-2bit(01:无)|加扰-2bit(01:加扰).
	}

	*ret_buf    = (char*)ts_buf;
	*ret_buflen = ts_len;

	return GX_PLAYER_OK;
}

int GxDemuxer_PrivatePacketDestroy(char *buf)
{
	if(buf)
		av_free(buf);

	return GX_PLAYER_OK;
}

#define V00_EXT_HEAD_SIZE (8*188)
#define V00_EXT_USER_FLAG (('U'<<24)|('S'<<16)|('E'<<8)|('R'))
#define READ_WORD_LE(var) do {                                                  \
	if (mode == READ_STREAM_MODE) {                                             \
		var = GxStream_ReadDword_Le(stream);                                    \
	} else {                                                                    \
		if (size < 4)                                                           \
			return GX_PLAYER_ERROR;                                             \
		var = (buffer[0] | (buffer[1]<<8) | (buffer[2]<<16) | (buffer[3]<<24)); \
		buffer += 4;                                                            \
		size   -= 4;                                                            \
	}                                                                           \
} while(0)
static int _private_packet_analysis_v00(GxStream *stream,
		off_t start_pos,
		unsigned char *buffer,
		unsigned int size,
		GxDemuxerPrivateTSInfo *priv_info,
		PlayerPVRConfig *pvr_config)
{
#define READ_STREAM_MODE (0)
#define READ_BUFFER_MODE (1)
	unsigned int i = 0;
	unsigned int mode = (stream != NULL) ? READ_STREAM_MODE : READ_BUFFER_MODE;
	unsigned int user_flag = 0;
	unsigned char *tmpbuf  = buffer;
	unsigned int   tmpsize = size;

	READ_WORD_LE(priv_info->a_pid);
	READ_WORD_LE(priv_info->v_pid);
	READ_WORD_LE(priv_info->a_fmt);
	READ_WORD_LE(priv_info->v_fmt);
	READ_WORD_LE(priv_info->pcr_pid);
	READ_WORD_LE(priv_info->hdr_len);
	READ_WORD_LE(priv_info->timestamp_pid);
	READ_WORD_LE(priv_info->ext_info.is_encrypt);
	READ_WORD_LE(priv_info->ext_info.service_id);
	for(i = 0; i < 4; i++) {
		READ_WORD_LE(priv_info->ext_info.ext_data[i]);
	}
	READ_WORD_LE(priv_info->ext_info.ext_num);
	if (priv_info->ext_info.ext_num > GX_DEMUXER_MAX_EXTPID_NUM)
		gxlogf("ext_num error, ext_num = %d!\n", priv_info->ext_info.ext_num);
	else {
		for(i = 0; i < priv_info->ext_info.ext_num; i++) {
			READ_WORD_LE(priv_info->ext_info.ext_pids[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_content[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_content_type[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_major[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_minor[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_codec[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_name[i]);
			READ_WORD_LE(priv_info->ext_info.ext_pids_lang[i]);
		}
	}
	READ_WORD_LE(pvr_config->volume_sizemb);
	READ_WORD_LE(pvr_config->volume_maxnum);
	READ_WORD_LE(pvr_config->volume_fullstop);
	READ_WORD_LE(pvr_config->reserve_sizemb);
	READ_WORD_LE(pvr_config->volume_maxtimes);

	READ_WORD_LE(user_flag);
	if (user_flag == V00_EXT_USER_FLAG) {
		unsigned char *userdata_buf = NULL;
		unsigned int   userdata_len = 0;

		READ_WORD_LE(priv_info->user_info.have_pmt);
		READ_WORD_LE(priv_info->user_info.userdata_len);
		userdata_buf = (unsigned char *)priv_info->user_info.userdata;
		userdata_len = priv_info->user_info.userdata_len;
		if (mode == READ_STREAM_MODE) {
			GxStream_Seek(stream, start_pos + V00_EXT_HEAD_SIZE);
			if (userdata_len > 0)
				GxStream_Read(stream, userdata_buf, userdata_len);
		} else {
			tmpbuf  += (V00_EXT_HEAD_SIZE - 8);
			tmpsize -= (V00_EXT_HEAD_SIZE - 8);
			if ((userdata_len > 0) && (userdata_len <= tmpsize)) {
				memcpy(userdata_buf, tmpbuf, userdata_len);
				tmpsize -= userdata_len;
			}
		}
	}

	return GX_PLAYER_OK;
}

static int _private_packet_analysis_v01(unsigned char *buffer,
		unsigned int size,
		GxDemuxerPrivateTSInfo *priv_info,
		PlayerPVRConfig *pvr_config)
{
	unsigned int *pbuf = (unsigned int *)buffer;
	unsigned int  ts_packets = pbuf[3];
	unsigned int  i = 0;
	unsigned char *pptr = NULL;
	GxDemuxerPrivatePacketV01    *packet1 = NULL;
	GxDemuxerPrivatePacketExtV01 *extdat1 = NULL;
	GxDemuxerPrivatePacketV02    *packet2 = NULL;

	for (i = 0; i < ts_packets; i++) {
		unsigned char *tmp_buf = &buffer[(ts_packets - i - 1) * 188];
		unsigned int   tmp_len = (i + 1) * 184;

		memmove(tmp_buf, tmp_buf + 4, tmp_len);
	}

	pptr    = buffer;
	packet1 = (GxDemuxerPrivatePacketV01*)pptr;
	pptr   += (sizeof(GxDemuxerPrivatePacketV01));
	extdat1 = (GxDemuxerPrivatePacketExtV01*)pptr;
	if (packet1->version == 2) {
		pptr += (packet1->ext_num * sizeof(GxDemuxerPrivatePacketExtV01));
		packet2 = (GxDemuxerPrivatePacketV02*)pptr;
		priv_info->ctrl_info.vaild = 1;
	}

	//prog info
	priv_info->a_pid = packet1->a_pid;
	priv_info->v_pid = packet1->v_pid;
	priv_info->a_fmt = packet1->a_fmt;
	priv_info->v_fmt = packet1->v_fmt;
	priv_info->pcr_pid = packet1->pcr_pid;
	priv_info->hdr_len = packet1->hdr_len;
	priv_info->timestamp_pid = packet1->timestamp_pid;

	//ext info
	priv_info->ext_info.is_encrypt  = packet1->is_encrypt;
	priv_info->ext_info.service_id  = packet1->service_id;
	priv_info->ext_info.ext_data[0] = packet1->ext_data[0];
	priv_info->ext_info.ext_data[1] = packet1->ext_data[1];
	priv_info->ext_info.ext_data[2] = packet1->ext_data[2];
	priv_info->ext_info.ext_data[3] = packet1->ext_data[3];
	priv_info->ext_info.ext_num     = packet1->ext_num;
	for (i = 0; i < packet1->ext_num; i++) {
		GxDemuxerPrivatePacketExtV01 *tmp_extdat = &extdat1[i];
		priv_info->ext_info.ext_pids[i]              = tmp_extdat->ext_pids;
		priv_info->ext_info.ext_pids_content[i]      = tmp_extdat->ext_pids_content;
		priv_info->ext_info.ext_pids_content_type[i] = tmp_extdat->ext_pids_content_type;
		priv_info->ext_info.ext_pids_major[i]        = tmp_extdat->ext_pids_major;
		priv_info->ext_info.ext_pids_minor[i]        = tmp_extdat->ext_pids_minor;
		priv_info->ext_info.ext_pids_codec[i]        = tmp_extdat->ext_pids_codec;
		priv_info->ext_info.ext_pids_name[i]         = tmp_extdat->ext_pids_name;
		priv_info->ext_info.ext_pids_lang[i]         = tmp_extdat->ext_pids_lang;
	}

	//prv info
	pvr_config->volume_sizemb   = packet1->volume_sizemb;
	pvr_config->volume_maxnum   = packet1->volume_maxnum;
	pvr_config->volume_fullstop = packet1->volume_fullstop;
	pvr_config->reserve_sizemb  = packet1->reserve_sizemb;
	pvr_config->volume_maxtimes = packet1->volume_maxtimes;

	//user info
	priv_info->user_info.have_pmt     = packet1->have_pmt;
	priv_info->user_info.userdata_len = packet1->user_len;
	if (priv_info->user_info.userdata_len > 0) {
		unsigned int   copy_len = priv_info->user_info.userdata_len;
		unsigned char *copy_buf = (unsigned char *)priv_info->user_info.userdata;
		memcpy(copy_buf, buffer + ts_packets * 188, copy_len);
	}

	if (packet2 != NULL) {
		priv_info->ctrl_info.encrypt_enable = packet2->user_encrypt_enable;
		priv_info->ctrl_info.block_size     = packet2->user_encrypt_blocksize;
		priv_info->ctrl_info.tsw_security   = packet2->tsw_security;
		priv_info->ctrl_info.tsw_ptr_en     = packet2->tsw_ptr_en;
	}

	return GX_PLAYER_OK;
}

int GxDemuxer_PrivatePacketTryAnalysis(GxStream *stream,
		GxDemuxerPrivateTSInfo *priv_info,
		PlayerPVRConfig *pvr_config)
{
	off_t start_pos = 0;
	int ret = GX_PLAYER_ERROR;
	int ver1 = 0, ver2 = 0;

	if (priv_info == NULL)
		return GX_PLAYER_ERROR;

	memset(priv_info, 0, sizeof(GxDemuxerPrivateTSInfo));
	start_pos = GxStream_Tell(stream);
	if ((GxStream_ReadChar(stream) == 0x47) &&
			(GxStream_ReadChar(stream) == 0xff) &&
			(GxStream_ReadChar(stream) == 0xff) &&
			((GxStream_ReadChar(stream) & 0xF0) == 'P') &&
			(((ver1 = GxStream_ReadChar(stream)) & 0xF3) == 'R') &&
			(((ver2 = GxStream_ReadChar(stream)) & 0xCF) == 'I') &&
			(GxStream_ReadChar(stream) == 'V') &&
			(GxStream_ReadChar(stream) == 'T')) {
		ver1 = ((ver1 >> 2) & 0x3);
		ver2 = ((ver2 >> 4) & 0x3);
		if ((ver1 == 0) && (ver2 == 0)) {
			ret = _private_packet_analysis_v00(stream, start_pos, NULL, 0, priv_info, pvr_config);
		} else if ((ver1 == 0) && (ver2 == 1)) {
			unsigned int   tmp_len = GxStream_ReadDword_Le(stream);
			unsigned char *tmp_buf = av_mallocz(tmp_len);

			if (tmp_buf == NULL)
				return GX_PLAYER_ERROR;

			GxStream_Read(stream, tmp_buf + 12, tmp_len - 12);
			ret = _private_packet_analysis_v01(tmp_buf, tmp_len, priv_info, pvr_config);
			av_free(tmp_buf);
		} else {
			gxloge("%s %d: version(%d.%d) error\n", __func__, __LINE__, ver1, ver2);
			GxStream_Seek(stream, start_pos);
			return GX_PLAYER_ERROR;
		}

		/*skip to next packet*/
		GxDemuxer_PrivatePacketSync(stream, 188);
	}
	else {
		gxlogf("parse NC private packet failed!\n");
		GxStream_Seek(stream, start_pos);
		return GX_PLAYER_ERROR;
	}

	return ret;
}

int GxDemuxer_PrivateAnalysis(unsigned char *buffer,
		unsigned int size,
		GxDemuxerPrivateTSInfo *priv_info,
		PlayerPVRConfig *pvr_config)
{
	int ret = GX_PLAYER_OK;

	if((buffer == NULL) || (priv_info == NULL))
		return GX_PLAYER_ERROR;

	memset(priv_info,  0, sizeof(GxDemuxerPrivateTSInfo));
	memset(pvr_config, 0, sizeof(PlayerPVRConfig));
	while (size > 8) {
		if(buffer[0] == 0x47 &&
				(buffer[1] == 0xff) &&
				(buffer[2] == 0xff) &&
				((buffer[3] & 0xF0) == 'P') &&
				((buffer[4] & 0xF3) == 'R') &&
				((buffer[5] & 0xCF) == 'I') &&
				(buffer[6] == 'V') &&
				(buffer[7] == 'T')) {
			unsigned int ver1 = ((buffer[4] >> 2) & 0x3);
			unsigned int ver2 = ((buffer[5] >> 4) & 0x3);

			buffer += 8;
			size   -= 8;
			if ((ver1 == 0) && (ver2 == 0)) {
				ret = _private_packet_analysis_v00(NULL, 0, buffer, size, priv_info, pvr_config);
			} else if ((ver1 == 0) && (ver2 == 1)) {
				unsigned int   tmp_len = (size + 8);
				unsigned char *tmp_buf = av_mallocz(tmp_len);

				if (tmp_buf == NULL)
					return GX_PLAYER_ERROR;

				memcpy(tmp_buf + 12, buffer + 4, tmp_len - 12);
				ret = _private_packet_analysis_v01(tmp_buf, tmp_len, priv_info, pvr_config);
				av_free(tmp_buf);
			} else {
				gxloge("%s %d: version(%d.%d) error\n", __func__, __LINE__, ver1, ver2);
				return GX_PLAYER_ERROR;
			}
			break;
		}
		else {
			buffer += 1;
			size   -= 1;
		}
	}

	return ret;
}

GxDemuxerClass gx_DemuxerBase = {
	._inherit = {
		._inherit = {
			.name    = "Demuxer",
			.parent  = &gx_mediafilter_base,
			.size    = sizeof(GxDemuxer),
			.create  = demuxerbase_create,
			.release = demuxerbase_release,
			.event   = NULL,
		},
		.run    = demuxerbase_run,
		.pause  = demuxerbase_pause,
		.resume = demuxerbase_resume,
		.stop   = demuxerbase_stop,
		.config = demuxerbase_config,
	},

	DEF_AUTHOR("demuxer","base","No description","L.F","No comment"),

	.type        = GX_DEMUXER_TYPE_UNKNOWN,
	.check_file  = NULL,
	.close       = NULL,
	.fill_buffer = NULL,
	.seek        = NULL,
	.control     = NULL,
};


