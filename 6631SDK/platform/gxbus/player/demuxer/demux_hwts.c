#include "demux_hwts.h"
#include <devapi/gxmtc_api.h>
#include "gx_decoder.h"
#include "gx_avtick.h"
#include "gx_avtimer.h"

#define DEBUG_HWTS
#define ALIGN_HWTS (8*188)

static void demux_hwts_close(GxDemuxer* demuxer);
static int  demux_hwts_stop(GxMediaFilter* filter);
static int  demux_hwts_control(GxDemuxer* demuxer, int cmd, void* arg);
static PLAYER_INTERRUPT_CBK hwts_interruptcbk = NULL;

static void hwts_get_duration(GxDemuxer *demuxer)
{
	DemuxHwTSPriv *priv = demuxer->priv;

	if (priv->fd) {
		off_t size = recfile_getsize(priv->fd);

		if ((size - priv->duration_file_size) >= (4 * ENTRY_BLOCK_SIZE)) {
			ssize_t rsize;
			PlayerRecIndexEntry entry;
			off_t pos = (size / ENTRY_BLOCK_SIZE - 1) * ENTRY_BLOCK_SIZE;
#ifdef DEBUG_HWTS
			int debug_stream_speed = 0;
			unsigned int start_time = 0;
			GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
			if (debug_stream_speed)
				start_time = av_get_tick(NULL, __LINE__);
#endif
			rsize = recfile_readAt(priv->fd, &entry, sizeof(entry), pos);
			if (rsize < 0)
				return;

#ifdef DEBUG_HWTS
			if (debug_stream_speed) {
				unsigned int end_time = av_get_tick(NULL, __LINE__);
				unsigned int dis_time = end_time - start_time;
				gxlogi_raw("[hwts - duration]: cost time %d\n", dis_time);
			}
#endif
			if (rsize == sizeof(entry)) {
				demuxer->duration = entry.timestamp;
				demuxer->movi_end = entry.pos;
				priv->duration_file_size = size;
			}
		}
	}

	return;
}

static int hwts_get_time_by_pos(PlayerRecIndexEntry *entries,
		int nb_entries, off_t pos, int64_t *timems)
{
	unsigned int a = 0, b = nb_entries - 1, m = 0;

	if (pos < entries[a].pos)
		return -1;

	if (pos > entries[b].pos)
		return 1;

	while (b - a > 1) {
		m = (a + b) >> 1;
		if (entries[m].pos > pos) {
			b = m;
		} else if (entries[m].pos < pos) {
			a = m;
		} else {
			a = m;
			break;
		}
	}
	m = a;

	//modify curtime skip
#define _CC (10)
	int cc = 0, hc = 0, lc = 0;
	int64_t c_time = 0;

	cc = (nb_entries >= _CC) ? _CC : nb_entries;
	hc = (m < cc) ? cc : m;
	lc = (m < cc) ? 0 : (m - cc);
	if (hc > lc) {
		int i = 0;
		int64_t m_pos = 0, m_time = 0;

		for (i = lc; i < hc; i++) {
			m_time += entries[i].timestamp;
			m_pos  += entries[i].pos;
		}
		m_time = m_time / (hc - lc);
		m_pos  = m_pos  / (hc - lc);
		if (m_pos == 0) {
			gxloge(">>> entries file error,pos range(%d-%d)\n", lc, hc);
			c_time = entries[m].timestamp;
		} else if (pos > m_pos) {
			 c_time = m_time + (pos - m_pos) * m_time / m_pos;
		} else if (pos < m_pos) {
			 c_time = m_time - (m_pos - pos) * m_time / m_pos;
		} else {
			c_time = m_time;
		}
		c_time = (c_time >= 0) ? c_time : 0;
	} else
		c_time = entries[m].timestamp;

	*timems = (int)c_time;
	return 0;
}

static int hwts_get_pos_by_time(PlayerRecIndexEntry *entries,
		int nb_entries, int64_t timems, off_t *pos)
{
	unsigned int a = 0, b = nb_entries - 1, m = 0;

	if (timems < entries[a].timestamp)
		return -1;

	if (timems > entries[b].timestamp)
		return 1;

	while (b - a > 1) {
		m = (a + b) >> 1;
		if (entries[m].timestamp > timems) {
			b = m;
		} else if (entries[m].timestamp < timems) {
			a = m;
		} else {
			a = m;
			break;
		}
	}
	m = a;
	*pos = entries[m].pos;
	return 0;
}

static int hwts_get_cur_time(GxDemuxer* demuxer)
{
	DemuxHwTSPriv *priv = demuxer->priv;
	off_t pos = GxStream_Tell(demuxer->stream);
	off_t filesize = recfile_getsize(priv->fd);
	int ret = 0;
	int64_t timems = 0;
	ssize_t rsize  = 0;

	if (pos > demuxer->movi_end) {
		goto time_eof;
	}

	if (priv->nb_entries == 0) {
		priv->entry_pos  = filesize * pos / demuxer->movi_end / ENTRY_BLOCK_SIZE * ENTRY_BLOCK_SIZE;
		priv->entry_pos -= (ENTRY_BLOCK_SIZE * ENTRY_BLOCK_NUM / 8);
		priv->entry_pos  = (priv->entry_pos < 0) ? 0 : priv->entry_pos;
	}

time_retry:
	if (priv->nb_entries == 0) {
#ifdef DEBUG_HWTS
		int debug_stream_speed = 0;
		unsigned int start_time = 0;
		GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
		if (debug_stream_speed)
			start_time = av_get_tick(NULL, __LINE__);
#endif
		if (priv->entry_pos + (sizeof(priv->entries) + 4 * ENTRY_BLOCK_SIZE) >= filesize)
			recfile_fsync(priv->fd);
		rsize = recfile_readAt(priv->fd, &priv->entries, sizeof(priv->entries), priv->entry_pos);
		if (rsize < 0)
			goto time_eof;

#ifdef DEBUG_HWTS
		if (debug_stream_speed) {
			unsigned int end_time = av_get_tick(NULL, __LINE__);
			unsigned int dis_time = end_time - start_time;
			gxlogi_raw("[hwts - cur_time]: cost time %d\n", dis_time);
		}
#endif

		priv->nb_entries = rsize / ENTRY_BLOCK_SIZE;
		if (priv->nb_entries < 1)
			goto time_eof;

#ifdef DEBUG_HWTS
		{
			gxlogd("--------> [cur time]: (%lld, %d, p:%lld) (%lld, %lld) - (%lld, %lld)\n",
					priv->entry_pos, priv->nb_entries, pos,
					priv->entries[0].timestamp, priv->entries[0].pos,
					priv->entries[priv->nb_entries - 1].timestamp, priv->entries[priv->nb_entries - 1].pos);
		}
#endif
	}

	ret = hwts_get_time_by_pos(priv->entries, priv->nb_entries, pos, &timems);
	if (ret > 0) {
		if (priv->nb_entries <= 1) {
			goto time_eof;
		} else {
			off_t offset = (priv->nb_entries - 1) * ENTRY_BLOCK_SIZE;
			priv->entry_pos += offset;
			priv->entry_pos  = (priv->entry_pos > filesize) ? filesize : priv->entry_pos;
			priv->nb_entries = 0;
			if (priv->entry_pos >= filesize)
				goto time_eof;
			else
				goto time_retry;
		}
	} else if (ret < 0) {
		if ((priv->entry_pos <= 0) || (priv->nb_entries <= 1)) {
			goto time_eof;
		} else {
			off_t offset = (priv->nb_entries - 1) * ENTRY_BLOCK_SIZE;
			priv->entry_pos -= offset;
			priv->entry_pos  = (priv->entry_pos < 0) ? 0 : priv->entry_pos;
			priv->nb_entries = 0;
			goto time_retry;
		}
	}

	priv->fill_cur_timems = timems;
	return 0;

time_eof:
	priv->nb_entries = 0;
	return -1;
}

static off_t hwts_get_time_pos(GxDemuxer* demuxer, int64_t timems, int flags)
{
	ssize_t rsize;
	DemuxHwTSPriv *priv = demuxer->priv;
	off_t filesize = recfile_getsize(priv->fd);
	off_t pos = 0;
	int ret = 0;
#ifdef DEBUG_HWTS
	int debug_stream_speed = 0;
	unsigned int start_time = 0;
#endif

	if ((timems > demuxer->duration) || (demuxer->duration == 0)) {
		goto pos_eof;
	}

	if (priv->nb_entries > 0) {
		ret = hwts_get_pos_by_time(priv->entries, priv->nb_entries, timems, &pos);
		if (ret == 0) {
			if (pos < demuxer->movi_start)
				pos = demuxer->movi_start;
			return pos;
		}
		priv->nb_entries = 0;
	}

	priv->entry_pos  = filesize * timems / demuxer->duration / ENTRY_BLOCK_SIZE * ENTRY_BLOCK_SIZE;
	if (demuxer->speed < 0)
		priv->entry_pos -= (ENTRY_BLOCK_SIZE * ENTRY_BLOCK_NUM * 3 / 4);
	else
		priv->entry_pos -= (ENTRY_BLOCK_SIZE * ENTRY_BLOCK_NUM / 8);
	priv->entry_pos  = (priv->entry_pos < 0) ? 0 : priv->entry_pos;

pos_retry:
#ifdef DEBUG_HWTS
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_SPEED, &debug_stream_speed);
	if (debug_stream_speed)
		start_time = av_get_tick(NULL, __LINE__);
#endif
	if (priv->entry_pos + (sizeof(priv->entries) + 4 * ENTRY_BLOCK_SIZE) >= filesize)
		recfile_fsync(priv->fd);
	rsize = recfile_readAt(priv->fd, &priv->entries, sizeof(priv->entries), priv->entry_pos);
	if (rsize < 0)
		goto pos_eof;

#ifdef DEBUG_HWTS
	if (debug_stream_speed) {
		unsigned int end_time = av_get_tick(NULL, __LINE__);
		unsigned int dis_time = end_time - start_time;
		gxlogi_raw("[hwts - time_pos]: cost time %d\n", dis_time);
	}
#endif
	priv->nb_entries = rsize / ENTRY_BLOCK_SIZE;
	if (priv->nb_entries < 1) {
		goto pos_eof;
	}

#ifdef DEBUG_HWTS
	{
		gxlogd("--------> [seek pos]: (%lld, %d, t:%lld) (%lld, %lld) - (%lld, %lld)\n",
				priv->entry_pos, priv->nb_entries, timems,
				priv->entries[0].timestamp, priv->entries[0].pos,
				priv->entries[priv->nb_entries - 1].timestamp, priv->entries[priv->nb_entries - 1].pos);
	}
#endif
	ret = hwts_get_pos_by_time(priv->entries, priv->nb_entries, timems, &pos);
	if (ret > 0) {
		if (priv->nb_entries <= 1) {
			goto pos_eof;
		} else {
			off_t offset = (priv->nb_entries - 1) * ENTRY_BLOCK_SIZE;
			priv->entry_pos += offset;
			priv->entry_pos  = (priv->entry_pos > filesize) ? filesize : priv->entry_pos;
			if (priv->entry_pos >= filesize)
				goto pos_eof;
			else
				goto pos_retry;
		}
	} else if (ret < 0) {
		if ((priv->entry_pos <= 0) || (priv->nb_entries <= 1)) {
			goto pos_eof;
		} else {
			off_t offset = (priv->nb_entries - 1) * ENTRY_BLOCK_SIZE;
			priv->entry_pos -= offset;
			priv->entry_pos  = (priv->entry_pos < 0) ? 0 : priv->entry_pos;
			goto pos_retry;
		}
	}

	if (pos < demuxer->movi_start)
		pos = demuxer->movi_start;
	return pos;

pos_eof:
	priv->nb_entries = 0;
	return -1;
}

static off_t hwts_get_target_time_pos(GxDemuxer* demuxer, int64_t timems, int flags)
{
	off_t pos;
	DemuxHwTSPriv *priv = demuxer->priv;

	GxCore_MutexLock(priv->mutex);
	if ((demuxer->stream->ispvr) ||
			(demuxer->stream->file_format == GX_STREAMTYPE_MEM)) {
		GxRecordPVRControl ctrl;
		GxRecordPVRTimePos time_pos;

		time_pos.timems = timems;
		time_pos.offset = 0;
		ctrl.opt = GX_RECORD_PVR_GET_POS_BY_TIME;
		ctrl.arg = (void *)(&time_pos);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
		pos = time_pos.offset;
	} else
		pos = hwts_get_time_pos(demuxer, timems, flags);
	GxCore_MutexUnlock(priv->mutex);

	return pos;
}

static void hwts_get_target_duration(GxDemuxer *demuxer)
{
	DemuxHwTSPriv *priv = demuxer->priv;

	GxCore_MutexLock(priv->mutex);
	if ((demuxer->stream->ispvr) ||
			(demuxer->stream->file_format == GX_STREAMTYPE_MEM)) {
		GxRecordPVRControl ctrl;
		int64_t duration = 0, filesize = 0;

		ctrl.opt = GX_RECORD_PVR_GET_DURATION;
		ctrl.arg = (void *)(&duration);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
		demuxer->duration = duration;

		ctrl.opt = GX_RECORD_PVR_GET_FILESIZE;
		ctrl.arg = (void *)(&filesize);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
		demuxer->movi_end = filesize;
	} else
		hwts_get_duration(demuxer);
	GxCore_MutexUnlock(priv->mutex);

	return;
}

static int hwts_get_target_cur_time(GxDemuxer* demuxer)
{
	int ret = 0;
	DemuxHwTSPriv *priv = demuxer->priv;

	GxCore_MutexLock(priv->mutex);
	if ((demuxer->stream->ispvr) ||
			(demuxer->stream->file_format == GX_STREAMTYPE_MEM)) {
		GxRecordPVRControl ctrl;
		int64_t timems = 0;

		ctrl.opt = GX_RECORD_PVR_GET_CURTIME;
		ctrl.arg = (void *)(&timems);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
		priv->fill_cur_timems = timems;
	} else
		ret = hwts_get_cur_time(demuxer);
	GxCore_MutexUnlock(priv->mutex);

	return ret;
}

static int hwts_check_blocksize(GxDemuxer *demuxer)
{
	int blocksize = HW_TS_BLOCK_SIZE;

	if (demuxer->duration >= 2000) {
		DemuxHwTSPriv *priv = demuxer->priv;
		unsigned int nps = 100;
		//每秒不小于nps次填包速度，可以满足流畅播放．
		//该值不能过小，否则在码率分布不均，导致切音轨效果不好．例如: 音视频分布不均.
		//该值不能过大，否则填包次数达不到要求，导致音视频卡顿．例如: 时移．

		priv->kBps = demuxer->movi_end / (demuxer->duration / 1000) / 1024;
		blocksize = priv->kBps * 1024 / nps;
		blocksize = blocksize / ALIGN_HWTS * ALIGN_HWTS;
		blocksize = (blocksize >= HW_TS_BLOCK_SIZE) ? HW_TS_BLOCK_SIZE : blocksize;
		blocksize = (blocksize <= 0) ? ALIGN_HWTS : blocksize;
	}

	return blocksize;
}

static int hwts_get_blocksize(GxDemuxer *demuxer, int *esa_len, int *esv_len)
{
	int acap = 0, vcap = 0, icap = HW_TSR_BUFFER_SIZE;
	int alen = 0, vlen = 0, ilen = 0, align = ALIGN_HWTS, blocksize = 0;
	int afree = 0, vfree = 0, ifree = 0;
	char aflag = 0, vflag = 0;
	DemuxHwTSPriv *priv = demuxer->priv;

	if (priv->vpin && priv->vpin->fifo && (!demuxer->video->dropmode)) {
		vcap  = priv->vpin->fifo->size;
		vlen  = GxFifo_GetLength(priv->vpin->fifo);
		vfree = (vcap * 7 / 8);
		vfree = (vfree > vlen) ? (vfree - vlen) : 0;
		vflag = 1;
	}

	if (priv->apin && priv->apin->fifo && (!demuxer->audio->dropmode)) {
		acap = priv->apin->fifo->size;
		alen = GxFifo_GetLength(priv->apin->fifo);
		afree = (acap * 7 / 8);
		afree = (afree > alen) ? (afree - alen) : 0;
		if (demuxer->audio1->pin && demuxer->audio1->pin->fifo) {
			unsigned int acap1 = demuxer->audio1->pin->fifo->size;
			unsigned int alen1 = GxFifo_GetLength(demuxer->audio1->pin->fifo);
			afree = ((acap1 - alen1) > afree) ? afree : (acap1 - alen1);
		}
		aflag = 1;
	}

	if (esa_len)
		*esa_len = alen;
	if (esv_len)
		*esv_len = vlen;
	if (aflag && vflag)
		blocksize = (afree > vfree) ? vfree : afree;
	else if (vflag)
		blocksize = vfree;
	else if (aflag)
		blocksize = afree;
	else
		goto end;

	ilen = GxFifo_GetLength(priv->infifo);
	if (ilen < blocksize) {
		blocksize -= ilen;
	} else {
		blocksize = 0;
		goto end;
	}

	ifree = GxFifo_GetCap(priv->infifo) - ilen - 1;
	blocksize = (blocksize > ifree) ? ifree : blocksize;

	if (priv->tsr_security || priv->decrypt_enable || priv->tsr_ptr_en) {
		blocksize = (blocksize >= priv->block_size) ? priv->block_size : 0;
	} else {
		int need_blocksize = hwts_check_blocksize(demuxer);

		blocksize = blocksize / align * align;
		blocksize = (blocksize > need_blocksize) ? need_blocksize : blocksize;
	}

end:
	return blocksize;
}

static int hwts_check_ts(unsigned char *buf, unsigned int len)
{
	unsigned int i = 0;

	if ((len % 188) != 0) {
		gxloge(">>> ts len(%d) error\n", len);
		return -1;
	}

	for (i = 0; i < (len / 188); i++) {
		if (buf[i * 188] != 0x47) {
			gxloge(">>> ts buf[%x] error\n", buf[i]);
			return -1;
		}
	}

	return 0;
}

static int hwts_seek(GxDemuxer *demuxer, int64_t pos)
{
	DemuxHwTSPriv* priv = (DemuxHwTSPriv*)demuxer->priv;

	if (priv->tsr_security || priv->decrypt_enable || priv->tsr_ptr_en) {
		if (priv->block_size > 0)
			pos = pos / priv->block_size * priv->block_size;
	} else {
		pos = pos / ALIGN_HWTS * ALIGN_HWTS;
	}

	priv->block_pos = 0;
	return GxStream_Seek(demuxer->stream, pos);
}

static void demux_hwts_check_thread(void *data)
{
	GxMediaFilter* mf = GXMEDIAFILTER(data);
	GxDemuxer* demuxer = GXDEMUXER(data);
	DemuxHwTSPriv* priv = (DemuxHwTSPriv*)demuxer->priv;

	while (mf->status != GX_MFT_STATE_STOPPED && priv->abort == 0) {
		int debug_fill_data  = 0;

		if (GxCore_SemTimedWait(demuxer->semaphore_exit, 500) == GXCORE_SUCCESS)
			break;

		GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_FILL_DATA, &debug_fill_data);
		if (debug_fill_data)
			GxDemuxer_DebugData(demuxer);
	}
}

static void demux_hwts_depack_thread(void* data)
{
	int len;
	GxMediaFilter* mf = GXMEDIAFILTER(data);
	GxDemuxer* demuxer = GXDEMUXER(data);
	DemuxHwTSPriv* priv = (DemuxHwTSPriv*)demuxer->priv;

	while (mf->status != GX_MFT_STATE_STOPPED && priv->abort == 0) {
		if (hwts_interruptcbk && hwts_interruptcbk())
			break;
		if (mf->status == GX_MFT_STATE_RUNNING) {
			int blocksize = 0;
			int esa_len = 0, esv_len = 0;
			int back_costms = 0;
			int hw_reset = 0;

			priv->paused = 0;
			if (priv->pause_fill_data ||
					(GxCore_MutexTrylock(demuxer->mutex) != GXCORE_SUCCESS)) {
				GxCore_ThreadDelay(10);
				continue;
			}
			hwts_get_target_duration(demuxer);

			if (demuxer->stream->eof) {
eof:
				if (demuxer->speed < 0) {
					demuxer->stream->eof = 0;
					GxCore_MutexUnlock(demuxer->mutex);
					GxCore_ThreadDelay(10);
					if (priv->abort) {
						gxlogi("%s %d: exit hwts playback...\n", __func__, __LINE__);
						break;
					}
					GxCore_MutexLock(demuxer->mutex);
					hw_reset = 1;
					goto frameok;
				} else {
					if (mf->event.func && demuxer->stcfreq > 0) {
						GxMediaFilterEventPara EventPara;
						EventPara.type = GX_MFT_EVENT_PLAY_END;
						EventPara.arg  = NULL;
						mf->event.func(mf->event.priv, &EventPara);
					}
					GxCore_MutexUnlock(demuxer->mutex);
					GxCore_ThreadDelay(10);
					continue;
				}
			}
			else if (demuxer->fbeof) {
fbeof:
				GxCore_MutexUnlock(demuxer->mutex);
				GxCore_ThreadDelay(10);
				continue;
			}

			if (GX_SPEED_JUMP(demuxer->speed)) {
				int32_t  frameok = 0;
				AVCodecStatus  state;
				GxMediaFilter* next_mf = demuxer->video->pin->link->filter;

				GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_ONE_FRAME_OVER, &frameok);
				GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_GET_STATUS,     &state);
				hw_reset = 0;
				if ((state.state == AVCODEC_ERROR) &&
						(state.err_code == AVCODEC_ERR_SIZE_UNSUPPORT)) {
					frameok  = 1;
					hw_reset = 1;
					gxlogi("[video]: state = %x, error_code= %x \n", state.state, state.err_code);
				}

				if (frameok) {
					int64_t seek_timems, seek_minims;
					off_t   seek_pos;

frameok:
					demux_hwts_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &seek_timems);
					seek_timems += (demuxer->speed * back_costms);
					back_costms = 0;
					{
						int debug_fill_data  = 0;
						GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_FILL_DATA, &debug_fill_data);
						if (debug_fill_data) {
							gxlogi_raw("[hwts seek] - seek: %lld(ms)\n", seek_timems);
						}
					}
					next_mf = demuxer->video->pin->link->filter;
					GxFifo_Reset(priv->infifo);
					if (priv->vpin && priv->vpin->fifo && (!demuxer->video->dropmode))
						GxFifo_Reset(priv->vpin->fifo);
					GxHwDemux_ResetStream(priv->module, priv->progid, HW_DEMUX_VIDEO);
					if (hw_reset) {
						GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_JUMP_RESET, &hw_reset);
					} else {
						GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_JUMP_RESET, NULL);
						GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_CONTINUE, NULL);
					}

					seek_minims = (int64_t)GxDemuxer_GetSeekMinTime(demuxer);
					if (seek_timems <= seek_minims) {
						if (demuxer->speed < 0) {
							demuxer->fbeof = 1;
							goto fbeof;
						} else
							seek_timems = seek_minims;
					} else if (seek_timems >= demuxer->duration) {
						if (demuxer->speed > 0) {
							demuxer->stream->eof = 1;
							goto eof;
						} else
							seek_timems = demuxer->duration;
					}

					seek_pos = hwts_get_target_time_pos(demuxer, seek_timems, GX_DEMUXER_SEEK_ABSOLUTE);
					if (seek_pos >= 0) {
						GxCore_MutexUnlock(demuxer->mutex);
						hwts_seek(demuxer, seek_pos);
						GxCore_MutexLock(demuxer->mutex);
					}
				}
			} else {
				int64_t curr_timems = 0;

				demux_hwts_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &curr_timems);
				if (curr_timems >= demuxer->duration) {
					demuxer->stream->eof = 1;
					goto eof;
				}

				if (hwts_get_target_cur_time(demuxer) < 0) {
					GxCore_MutexUnlock(demuxer->mutex);
					GxCore_ThreadDelay(10);
					continue;
				}

				if (demuxer->speed == 1000) {
#define MAX_FILL_TIMEMS (1000)
#define LESS_ESA_SIZE   ( 6*1024)
#define LESS_ESV_SIZE   (32*1024)
					int a_fill_flags = 0, v_fill_flags = 0, fill_flags = 0;
					int64_t fill_timems = priv->fill_cur_timems;
					int less_esv = LESS_ESV_SIZE, less_esa = LESS_ESA_SIZE;
					int multi = 1;

					if (priv->kBps >= 1200)
						multi = 4;
					else if (priv->kBps >= 800)
						multi = 3;
					else if (priv->kBps >= 400)
						multi = 2;
					less_esv *= multi;
					less_esa *= multi;
					if (priv->vpin && priv->vpin->fifo && (!demuxer->video->dropmode)) {
						int vlen = GxFifo_GetLength(priv->vpin->fifo);
						v_fill_flags = (vlen < less_esv) ? 1 : 0;
					}

					if (priv->apin && priv->apin->fifo && (!demuxer->audio->dropmode)) {
						int alen = GxFifo_GetLength(priv->apin->fifo);
						a_fill_flags = (alen < less_esa) ? 1 : 0;
					}

					fill_flags = (fill_timems <= (curr_timems + MAX_FILL_TIMEMS)) ? 1 : 0;
					if (!fill_flags && !a_fill_flags && !v_fill_flags) {
						GxCore_MutexUnlock(demuxer->mutex);
						GxCore_ThreadDelay(10);
						continue;
					}
				}
			}

			blocksize = hwts_get_blocksize(demuxer, &esa_len, &esv_len);
			if (blocksize > 0) {
				if (priv->tsr_ptr_en == 0) {
					int ret = 0;

					len = GxStream_Read(demuxer->stream, priv->buffer + priv->block_pos, blocksize - priv->block_pos);
					if (len > 0) {
						ssize_t wsize, align = ALIGN_HWTS;
						len += priv->block_pos;
						priv->block_pos = len % align;
						len = len / align * align;

						ret = hwts_check_ts(priv->buffer, len);
						if (ret < 0) {
							if (demuxer->speed < 0) {
								demuxer->fbeof = 1;
								goto fbeof;
							} else {
								demuxer->stream->eof = 1;
								goto eof;
							}
						}

						wsize = GxFifo_Write(priv->infifo, priv->buffer, len, -1);
						while(wsize < len && priv->abort == 0) {
							wsize += GxFifo_Write(priv->infifo, (unsigned char *)priv->buffer + wsize, len - wsize, -1);
							GxCore_MutexUnlock(demuxer->mutex);
							GxCore_ThreadDelay(10);
							GxCore_MutexLock(demuxer->mutex);
						}
						if (priv->block_pos)
							memcpy(priv->buffer, priv->buffer+len, priv->block_pos);
					} else {
						int errorcode = 0;

						GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_ERROR_CODE, &errorcode);
						if (errorcode == 0) {
							int64_t curr_timems = 0;

							demux_hwts_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &curr_timems);
							gxlogi("########## tsr eof %lld %lld \n", curr_timems, demuxer->duration);
							demuxer->stream->eof = 1;
							goto eof;
						}
					}
				}
				else {
					unsigned char *tsr_paddr = NULL;

					len = GxFifo_ShallowRead(priv->infifo, &tsr_paddr, priv->block_size, -1);
					if (len < priv->block_size) {
						gxloge("########## tsr block size error  %x %x \n", len, priv->block_size);
						GxCore_MutexUnlock(demuxer->mutex);
						GxCore_ThreadDelay(10);
						continue;
					}
					GxCore_MutexUnlock(demuxer->mutex);

shadow_stream_read:
					GxCore_MutexLock(demuxer->mutex);
					len = GxStream_Control(demuxer->stream, GX_STREAM_CTRL_READ_BLOCK, tsr_paddr);
					if (priv->running) {
						unsigned int process_flag = 0;

						GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_PROCESS_FLAG, &process_flag);
						if (process_flag) {
							GxCore_MutexUnlock(demuxer->mutex);
							if (priv->process_flag != process_flag) {
								GxMediaFilterEventPara EventPara;

								priv->paused = 1;
								gxlogi("############# pause\n");
								EventPara.type = GX_MFT_EVENT_MEDIA_PAUSE;
								EventPara.arg  = NULL;
								mf->event.func(mf->event.priv, &EventPara);
								priv->process_flag = process_flag;
							}
							GxCore_ThreadDelay(10);
							goto shadow_stream_read;
						} else {
							if(priv->process_flag != process_flag) {
								GxCore_MutexUnlock(demuxer->mutex);
								GxMediaFilterEventPara EventPara;

								gxlogi("############# resume\n");
								EventPara.type = GX_MFT_EVENT_MEDIA_RESUME;
								EventPara.arg  = NULL;
								mf->event.func(mf->event.priv, &EventPara);
								GxCore_MutexLock(demuxer->mutex);
								priv->process_flag = process_flag;
							}
						}
					}

					if (len == priv->block_size) {
						GxFifo_ShallowWrite(priv->infifo, NULL, len);
					} else if (len == 0) {
						int64_t curr_timems = 0;

						demux_hwts_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &curr_timems);
						hwts_get_target_duration(demuxer);
						if (curr_timems >= demuxer->duration) {
							demuxer->stream->eof = 1;
							goto eof;
						}

						if (mf->status == GX_MFT_STATE_RUNNING) {
							GxCore_MutexUnlock(demuxer->mutex);
							GxCore_ThreadDelay(10);
							goto shadow_stream_read;
						}
					} else {
						if (demuxer->speed < 0)
							back_costms = 200;//speed back and read data eof.
						else {
							int64_t curr_timems = 0;

							demux_hwts_control(demuxer, GX_DEMUXER_CTRL_GET_CURRENT_TIME, &curr_timems);
							gxlogi("########## tsr eof %lld %lld \n", curr_timems, demuxer->duration);
						}
						demuxer->stream->eof = 1;
						goto eof;
					}
				}
				GxCore_MutexUnlock(demuxer->mutex);
			} else {
				GxCore_MutexUnlock(demuxer->mutex);
				GxCore_ThreadDelay(10);
			}
		}
		else {
			priv->paused = 1;
			hwts_get_target_duration(demuxer);
			GxCore_ThreadDelay(10);
		}
	}

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
}

static int demux_hwts_check_file(GxDemuxer* demuxer)
{
	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &hwts_interruptcbk);

	return GX_DEMUXER_TYPE_HW_TS;
}

static GxDemuxer* demux_hwts_open(GxDemuxer* demuxer)
{
	int ret;
	int sync = 0;
	int audio_num = 0, sub_num = 0, video_num = 0;
	int hdr_size = 0;
	off_t start_pos = 0, seek_pos = 0;
	int64_t duration = 0;
	int64_t mini_timems = 0;
	GxStreamVideoHeader *sh_video = NULL;
	GxStreamAudioHeader *sh_audio = NULL;
	DemuxHwTSPriv *priv = NULL;

	priv = av_mallocz(sizeof(DemuxHwTSPriv));
	if (priv == NULL)
		return NULL;

	priv->dev    = -1;
	priv->module = -1;
	priv->progid = -1;
	demuxer->priv = priv;
	demuxer->hwts_sync_mode = PURE_APTS_RECOVER;

	GxStream_Reset(demuxer->stream);
	hwts_seek(demuxer, 0);

	GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PROBE_HDR, &hdr_size);
	if (hdr_size > 0) {
		struct vol_hdr hdr;

		hdr.size   = hdr_size;
		hdr.buffer = av_malloc(hdr_size);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_READ_HDR, &hdr);
		ret = GxDemuxer_PrivateAnalysis(hdr.buffer, hdr.size, &priv->tsinfo, &priv->pvr_config);
		av_free(hdr.buffer);
		if (ret != GX_PLAYER_OK) {
			goto errout;
		}
		start_pos = 0;
	} else {
		sync = GxDemuxer_PrivatePacketSyncExtInfo(demuxer->stream);
		if (!sync)
			hwts_seek(demuxer, 0);
		ret = GxDemuxer_PrivatePacketTryAnalysis(demuxer->stream, &priv->tsinfo, &priv->pvr_config);
		if (ret != GX_PLAYER_OK) {
			goto errout;
		}
		start_pos = priv->tsinfo.hdr_len;
	}

	if (priv->tsinfo.ctrl_info.vaild) {
		priv->tsr_ptr_en     = priv->tsinfo.ctrl_info.tsw_ptr_en;
		priv->tsr_security   = priv->tsinfo.ctrl_info.tsw_security;
		priv->block_size     = priv->tsinfo.ctrl_info.block_size;
		priv->decrypt_enable = priv->tsinfo.ctrl_info.encrypt_enable;
		if ((priv->block_size == 0) && priv->tsr_security)
			priv->block_size = RECODER_BLOCK_SIZE;
	} else {
		int firewall_flag = 0;
		GxRecordPVRControl ctrl;

		GxPlayer_SystemGet(PSYS_FIREWALL_FLAG, &firewall_flag);
		priv->tsr_security   = ((firewall_flag & GXMEM_FLAG_DEMUX_TSW) ? 1: 0);
		priv->block_size     = RECODER_BLOCK_SIZE;
		priv->decrypt_enable = 0;
		priv->tsr_ptr_en     = 0;

		ctrl.opt = GX_RECORD_PVR_GET_HWSAFE;
		ctrl.arg = (void *)(&priv->tsr_security);
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
	}

	if (1) {
		GxStreamSecurity sec;

		sec.security_flag = (priv->tsr_ptr_en) ? priv->tsr_security : 0;
		sec.block_size    = priv->block_size;
		sec.crypt_enable  = priv->decrypt_enable;
		GxStream_Control(demuxer->stream, GX_STREAM_CTRL_SET_SECURITY, &sec);
	}

	if (!VAILD_PID(demuxer->info.video_pid)) {
		demuxer->info.video_pid = priv->tsinfo.v_pid;
	} else {
		if (demuxer->info.video_pid != priv->tsinfo.v_pid) {
			sh_video = GxStreamHeader_VideoNew(demuxer, video_num, priv->tsinfo.v_pid);
			sh_video->format = priv->tsinfo.v_fmt;
			video_num++;
		}
	}

	if (VAILD_PID(demuxer->info.video_pid)) {
		sh_video = GxStreamHeader_VideoNew(demuxer, video_num, demuxer->info.video_pid);
		sh_video->priv.pts_sync = 1;
		sh_video->format   = priv->tsinfo.v_fmt;
		demuxer->video->id = 0;
		demuxer->video->sh = sh_video;
		video_num++;
	}

	if (!VAILD_PID(demuxer->info.audio_pid)) {
		demuxer->info.audio_pid = priv->tsinfo.a_pid;
	} else {
		if (demuxer->info.audio_pid != priv->tsinfo.a_pid) {
			sh_audio = GxStreamHeader_AudioNew(demuxer, audio_num, priv->tsinfo.a_pid);
			sh_audio->format = priv->tsinfo.a_fmt;
			audio_num++;
		}
	}

	if (VAILD_PID(demuxer->info.audio_pid)) {
		sh_audio = GxStreamHeader_AudioNew(demuxer, audio_num, demuxer->info.audio_pid);
		sh_audio->priv.pts_sync = 1;
		sh_audio->format   = priv->tsinfo.a_fmt;
		demuxer->audio->id = 0;
		demuxer->audio->sh = sh_audio;
		audio_num++;
	}

	if (VAILD_PID(demuxer->info.ad_pid) &&
			VAILD_PID(demuxer->info.audio_pid) &&
			(demuxer->info.ad_pid != demuxer->info.audio_pid)) {
		if (GXCHIP_IS_SIRIUS && priv->tsr_ptr_en) {
			gxlogi("Sirus gxloader TSW Config, Unsupport AD: pid %d\n", demuxer->info.ad_pid);
		} else {
			sh_audio = GxStreamHeader_Audio1New(demuxer, 1, demuxer->info.ad_pid);
			sh_audio->priv.pts_sync = 1;
			sh_audio->format = demuxer->info.ad_codec;
			sh_audio->stream_type = AVSTREAM_TS;
			sh_audio->stream_pid  = demuxer->info.ad_pid;
			demuxer->audio1->id = 0;
			demuxer->audio1->sh = sh_audio;
			audio_num++;
		}
	}

	if (!HAVE_AUDIO(demuxer) && !HAVE_VIDEO(demuxer))
		goto errout;

	if (priv->tsinfo.ext_info.ext_num > 0) {
		int i= 0;
		for(i = 0; i < priv->tsinfo.ext_info.ext_num; i++) {
			switch (priv->tsinfo.ext_info.ext_pids_content[i]) {
			case PLAYER_MEDIA_AUDIO:
			case PLAYER_MEDIA_AUDIO_DESC:
				{
					if (demuxer->info.audio_pid == priv->tsinfo.ext_info.ext_pids[i]) {
						if (demuxer->audio->sh != NULL) {
							sh_audio = demuxer->audio->sh;
							sh_audio->format = priv->tsinfo.ext_info.ext_pids_content_type[i];
						}
					} else if (demuxer->info.ad_pid == priv->tsinfo.ext_info.ext_pids[i]) {
						if (demuxer->audio1->sh != NULL) {
							sh_audio = demuxer->audio1->sh;
							sh_audio->format = priv->tsinfo.ext_info.ext_pids_content_type[i];
						}
					} else {
						if (priv->tsinfo.ext_info.ext_pids_content[i] == PLAYER_MEDIA_AUDIO_DESC) {
							if (GXCHIP_IS_SIRIUS && priv->tsr_ptr_en) {
								gxlogi("Sirus gxloader TSW Config, Unsupport AD: pid %d\n",
										priv->tsinfo.ext_info.ext_pids[i]);
							} else {
								sh_audio = GxStreamHeader_Audio1New(demuxer,
										audio_num,
										priv->tsinfo.ext_info.ext_pids[i]);
								sh_audio->format = priv->tsinfo.ext_info.ext_pids_content_type[i];
								audio_num++;
							}
						} else {
							sh_audio = GxStreamHeader_AudioNew(demuxer, audio_num, priv->tsinfo.ext_info.ext_pids[i]);
							sh_audio->format = priv->tsinfo.ext_info.ext_pids_content_type[i];
							audio_num++;
						}
					}
					memcpy(sh_audio->priv.codec, &priv->tsinfo.ext_info.ext_pids_codec[i], sizeof(unsigned int));
					memcpy(sh_audio->priv.name,  &priv->tsinfo.ext_info.ext_pids_name[i],  sizeof(unsigned int));
					memcpy(sh_audio->priv.lang,  &priv->tsinfo.ext_info.ext_pids_lang[i],  sizeof(unsigned int));
				}
				break;
			case PLAYER_MEDIA_SUB:
				{
					GxStreamSubHeader *sh_sub;
					sh_sub = GxStreamHeader_SubNew(demuxer, sub_num, priv->tsinfo.ext_info.ext_pids[i]);
					memset(&sh_sub->priv, 0, sizeof(sh_sub->priv));
					memcpy(sh_sub->priv.codec, &priv->tsinfo.ext_info.ext_pids_codec[i], sizeof(unsigned int));
					memcpy(sh_sub->priv.name,  &priv->tsinfo.ext_info.ext_pids_name[i],  sizeof(unsigned int));
					memcpy(sh_sub->priv.sub_stream[0].lang,
							&priv->tsinfo.ext_info.ext_pids_lang[i], sizeof(unsigned int));
					sh_sub->priv.sub_num = 1;
					sh_sub->priv.id = priv->tsinfo.ext_info.ext_pids[i];
					switch (priv->tsinfo.ext_info.ext_pids_content_type[i]) {
					case PLAYER_SUB_TYPE_DVB:
						sh_sub->type = SUB_CODEC_DVB_DESCRIPTOR;
						break;
					case PLAYER_SUB_TYPE_DVB_TTX:
						sh_sub->type = SUB_CODEC_TXT_DESCRIPTOR;
						sh_sub->priv.sub_stream[0].type = 0x0;
						break;
					case PLAYER_SUB_TYPE_DVB_MAG:
						sh_sub->type = SUB_CODEC_TXT_DESCRIPTOR;
						sh_sub->priv.sub_stream[0].type = 0x1;
						break;
					case PLAYER_SUB_TYPE_SCTE:
						sh_sub->type = SUB_CODEC_SCTE;
						break;
					default:
						sh_sub->type = SUB_CODEC_DVB_DESCRIPTOR;
						break;
					}
					sh_sub->priv.sub_stream[0].major = priv->tsinfo.ext_info.ext_pids_major[i];
					sh_sub->priv.sub_stream[0].minor = priv->tsinfo.ext_info.ext_pids_minor[i];
					sub_num++;
				}
				break;
			default:
				break;
			}
		}
	}

	priv->infifo = GxFifo_Create(HW_TSR_BUFFER_SIZE, GX_PINFLAG_MUXTS);
	if (priv->infifo == NULL) {
		goto errout;
	}

	if ((demuxer->stream->ispvr == 0) &&
			(demuxer->stream->file_format != GX_STREAMTYPE_MEM)) {
		priv->fd = recfile_open(demuxer->stream->url, O_RDONLY);
		if (priv->fd == NULL) {
			goto errout;
		}
	}

	priv->speed = 1000;
	GxPlayer_SystemGet(PSYS_PVR_PLAY_DEMUX_ID, &priv->module);
	if(priv->module < 0) {
		gxloge("[error]: playback dmxid not defined!\n");
		goto errout;
	}

	GxHwDemux_OpenModule(priv->module);
	GxHwDemux_GetSource(priv->module, &priv->default_source);
	GxHwDemux_SetSource(priv->module, DEMUX_SDRAM);
	priv->progid = GxHwDemux_AllocProg(priv->module);
	priv->dev = GxAvdev_CreateDevice(0);
	if (priv->progid < 0)
		goto errout;

	if (HAVE_AUDIO1(demuxer)) {
		sh_audio = demuxer->audio1->sh;
		priv->ad_dmx = GxAdDemux_Open(sh_audio->priv.id, DEMUX_SDRAM);
	}

	GxCore_MutexCreate(&priv->mutex);
	hwts_get_target_duration(demuxer);
	if (demuxer->duration == 0)
		goto errout;

	priv->timer = avtimer_create();
	mini_timems = (int64_t)GxDemuxer_GetSeekMinTime(demuxer);
	seek_pos = hwts_get_target_time_pos(demuxer, mini_timems, 0);

	demuxer->stc_factor = 45;
	demuxer->stcfreq = 45000;
	demuxer->seekable = 1;
	demuxer->movi_start = (start_pos > seek_pos) ? start_pos : seek_pos;
	if (demuxer->stream->ispvr)
		demuxer->movi_start = demuxer->movi_start / ALIGN_HWTS * ALIGN_HWTS;
	priv->fill_cur_timems = 0;
	GxStream_Reset(demuxer->stream);
	hwts_seek(demuxer, demuxer->movi_start);
	GxDemuxer_STCReset(demuxer);
	hwts_get_target_cur_time(demuxer);
	avtimer_update(priv->timer);
	priv->timer_basems = mini_timems;

	return demuxer;

errout:
	demux_hwts_close(demuxer);
	return NULL;
}

static int demux_hwts_config(GxMediaFilter* filter)
{
	int ret, fifo_size, syncmode;
	GxFifoConfig FifoConfig;
	HwDemuxConf   *conf = av_mallocz(sizeof(HwDemuxConf));
	DemuxHwTSPriv *priv;
	GxDemuxer* demuxer = GXDEMUXER(filter);

	if (demuxer) {
		priv = (DemuxHwTSPriv*)demuxer->priv;

		priv->abort = 0;
		priv->apin = demuxer->audio->pin;
		priv->vpin = demuxer->video->pin;

		if (HAVE_AUDIO(demuxer)) {
			GxStreamAudioHeader* sh = demuxer->audio->sh;
			sh->priv.pts_sync = (HAVE_VIDEO(demuxer)) ? 1 : 0;
		}

		if (HAVE_VIDEO(demuxer)) {
			GxStreamVideoHeader* sh = demuxer->video->sh;
			sh->priv.pts_sync = (HAVE_AUDIO(demuxer)) ? 1: 0;
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

		if (priv->infifo) {
			memset(&FifoConfig, 0, sizeof(FifoConfig));
			FifoConfig.dev = priv->dev;
			FifoConfig.dvrid = priv->module;
			FifoConfig.source = DVR_INPUT_MEM;
			FifoConfig.dest = DVR_OUTPUT_DMX;
			FifoConfig.hw_buffer_size = priv->infifo->size;
			FifoConfig.blocklen = priv->block_size;
			FifoConfig.flags |= (priv->tsr_ptr_en) ? DVR_FLAG_PTR_MODE_EN : 0;
			FifoConfig.flags |= (priv->tsr_security) ? 0 : DVR_FLAG_MEM_NOT_PROTECTED;
			if(GxFifo_Config(priv->infifo, &FifoConfig) != 0)
				goto end;
		}

		GxPlayer_SystemGet(PSYS_PVR_PLAYBACK_HWTS_SYNCMODE,(int32_t*)&syncmode);

		conf->a_id = demuxer->audio->dropmode ? 0 : demuxer->info.audio_pid;
		conf->v_id = demuxer->video->dropmode ? 0 : demuxer->info.video_pid;
		conf->pcr_id = priv->tsinfo.pcr_pid;
		if (VAILD_PID(conf->a_id) && VAILD_PID(conf->v_id))
			conf->sync = syncmode;
		else if (VAILD_PID(conf->a_id))
			conf->sync = APTS_RECOVER;
		else
			conf->sync = VPTS_RECOVER;
		conf->stcadjust = 0;
		demuxer->hwts_sync_mode = conf->sync;

		conf->type  = HW_DEMUX_FUNC_DEMUX;
		conf->afifo = (priv->apin && priv->apin->fifo) ? priv->apin->fifo : 0;
		conf->vfifo = (priv->vpin && priv->vpin->fifo) ? priv->vpin->fifo : 0;
		conf->progid = priv->progid;
		conf->bind_descr = priv->tsinfo.ext_info.is_encrypt ? 1 : 0;
		priv->bind_descr = conf->bind_descr;

		ret = GxHwDemux_Config(priv->module, conf);
		if (ret != GX_PLAYER_ERROR) {
			if (ret & AUDIO_CONFIG_FAILED) {
				if (demuxer->video && demuxer->video->sh) {
					GxStreamVideoHeader* sh = demuxer->video->sh;
					sh->priv.pts_sync = 0;
				}
			}
			if (ret & VIDEO_CONFIG_FAILED) {
				if (demuxer->audio && demuxer->audio->sh) {
					GxStreamAudioHeader* sh = demuxer->audio->sh;
					sh->priv.pts_sync = 0;
				}
			}

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			av_free(conf);

			if (!demuxer->audio1->dropmode) {
				if (priv->ad_dmx) {
					GxPin *pin = GxMediaFilter_FindPin(filter, GX_DEMUX_PIN_NAME_ADAO);

					if (pin)
						GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->bind_descr);
				}
			}
			return GX_PLAYER_OK;
		}
	}
end:
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	av_free(conf);
	return GX_PLAYER_ERROR;
}

static int demux_hwts_control(GxDemuxer* demuxer, int cmd, void* arg)
{
	DemuxHwTSPriv* priv = (DemuxHwTSPriv *) demuxer->priv;

	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_SEEKMIN_TIME:
		{
			struct vol_info info;

			if (demuxer->stream->ispvr) {
				GxRecordPVRControl ctrl;
				uint64_t timems = 0;

				ctrl.opt = GX_RECORD_PVR_GET_MINTIME;
				ctrl.arg = (void *)(&timems);
				GxStream_Control(demuxer->stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&ctrl);
				*(uint64_t*)arg = timems;
				return GX_DEMUXER_CTRL_OK;
			}

			if (GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_VOL_INFO, &info) == GX_PLAYER_OK) {
				if (demuxer->duration > 0) {
					int volume_sizemb   = priv->pvr_config.volume_sizemb;
					int volume_maxnum   = priv->pvr_config.volume_maxnum;
					int volume_maxtimes = priv->pvr_config.volume_maxtimes;

					if ((info.truesize > 0) && (volume_maxnum > 0)) {
						uint64_t seek_minims = 0;
						int64_t volume_seekmb = 0;
						uint64_t truesize = 0, tolsize = 0;

						volume_seekmb = (int64_t)(volume_maxnum - 1) * volume_sizemb * (1024 * 1024);
						if ((volume_seekmb > 0) && (info.truesize > volume_seekmb))
							info.truesize = volume_seekmb;

						if (((info.size - info.truesize) >= (uint64_t)0x40000000) && (info.size >= (uint64_t)0x40000000)) {
							truesize = ((info.size - info.truesize) >> 12);
							tolsize  = ((info.size) >> 12);
						} else {
							truesize = (info.size - info.truesize);
							tolsize  = (info.size);
						}
						if (tolsize > 0)
							seek_minims = demuxer->duration * truesize / tolsize;
						else
							seek_minims = 0;
						if (volume_maxtimes > 0 ) {
							if (demuxer->duration > (1000 * volume_maxtimes)) {
								uint64_t seek_ms = demuxer->duration - (1000 * volume_maxtimes);
								seek_minims = (seek_minims > seek_ms) ? seek_minims : seek_ms;
							}
						}
						*(uint64_t*)arg = seek_minims;
					} else {
						*(uint64_t*)arg = 0;
					}
				}
				return GX_DEMUXER_CTRL_OK;
			}

			return GX_DEMUXER_CTRL_NOTIMPL;
		}
	case GX_DEMUXER_CTRL_GET_CURRENT_TIME:
		{
			int64_t timer_timems = (int64_t)avtimer_getms(priv->timer);
			int64_t currs_timems = (priv->timer_basems + (demuxer->speed * timer_timems / 1000));

			if (currs_timems < 0)
				currs_timems = 0;
			else if (currs_timems > demuxer->duration)
				currs_timems = demuxer->duration;

			*(uint64_t*)arg = (uint64_t)(currs_timems);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_CURRENT_PERCENT:
		{
			off_t cur_pos, end_pos;
			cur_pos  = GxStream_Tell(demuxer->stream);
			GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_SIZE, &end_pos);
			if (end_pos > 0)
				*(uint8_t*)arg = (uint8_t)100 * cur_pos / end_pos;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		{
			*(uint64_t*)arg = (uint64_t)(demuxer->duration);
			if (demuxer->duration > 0) {
				if (demuxer->video->sh && priv) {
					GxStreamVideoHeader *hdr = demuxer->video->sh;
					hdr->i_bps = demuxer->movi_end/demuxer->duration;
				}
				else if (demuxer->audio->sh && priv) {
					GxStreamAudioHeader *hdr = demuxer->audio->sh;
					hdr->i_bps = demuxer->movi_end/demuxer->duration;
				}
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_FOURC_STOP:
		{
			if (priv) priv->abort = 1;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_TSRECORDINFO :
		{
			if (priv && arg) {
				PlayerRecordConfig *config = (PlayerRecordConfig *)arg;

				memcpy(&config->ext_info, &priv->tsinfo.ext_info,  sizeof(GxDemuxerRecordExtInfo));
				memcpy(&config->ext_data, &priv->tsinfo.user_info, sizeof(GxDemuxerRecordUserInfo));
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE:
		{
			int dropmode = *(int*)arg;

			if (HAVE_AUDIO(demuxer)) {
				if (NEEDDROP(dropmode)) {
					if (priv && demuxer->info.audio_pid > 0)
						GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_AUDIO);
				}
				demuxer->audio->dropmode = dropmode;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_AD_DROPMODE:
		{
			int dropmode = *(int*)arg;

			if (HAVE_AUDIO1(demuxer)) {
				if (NEEDDROP(dropmode)) {
					if (priv->ad_dmx)
						GxAdDemux_Stop(priv->ad_dmx);
				}
				demuxer->audio1->dropmode = dropmode;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE:
		{
			int dropmode = *(int*)arg;

			if (HAVE_VIDEO(demuxer)) {
				if (NEEDDROP(dropmode)) {
					if (priv && demuxer->info.video_pid > 0)
						GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_VIDEO);
				}
				demuxer->audio->dropmode = dropmode;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SWITCH_AUDIO:
	case GX_DEMUXER_CTRL_SWITCH_VIDEO:
		if (arg)
		{
			GxDemuxerStreamSwitch* StreamSwitch = (GxDemuxerStreamSwitch*)arg;

			if (VAILD_PID(StreamSwitch->pid)) {
				if (cmd == GX_DEMUXER_CTRL_SWITCH_AUDIO)
				{
					unsigned int i = 0;
					GxStreamAudioHeader* sh = NULL;

					for (i = 0; i <  MAX_A_STREAMS; i++) {
						sh = demuxer->a_streams[i];
						if (sh && sh->priv.id == StreamSwitch->pid)
							break;
					}

					if (sh == NULL)
						return GX_PLAYER_ERROR;

					if (i < MAX_A_STREAMS) {
						demuxer->audio->sh = sh;
						if(sh->format == AUDIO_CODEC_AC3 || sh->format == AUDIO_CODEC_EAC3) {
							sh->big_endian = 1;
						}
						GxHwDemux_SwitchStream(priv->module, priv->progid,
								HW_DEMUX_AUDIO, StreamSwitch->pid, priv->bind_descr);
						demuxer->info.audio_pid = StreamSwitch->pid;
						demuxer->audio->id = i;
					}
				}
				else if (cmd == GX_DEMUXER_CTRL_SWITCH_VIDEO)
				{
					unsigned int i = 0;
					GxStreamVideoHeader* sh = demuxer->video->sh;

					for (i = 0; i <  MAX_V_STREAMS; i++) {
						sh = demuxer->v_streams[i];
						if (sh && sh->priv.id == StreamSwitch->pid)
							break;
					}

					if (sh == NULL)
						return GX_PLAYER_ERROR;

					if (i < MAX_V_STREAMS) {
						demuxer->video->sh = sh;
						GxHwDemux_SwitchStream(priv->module, priv->progid,
								HW_DEMUX_VIDEO, StreamSwitch->pid, priv->bind_descr);
						demuxer->info.video_pid = StreamSwitch->pid;
						demuxer->video->id = i;
					}
				}
			}

			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_ON:
		{
			return GxHwDemux_RunStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT, *(int*)arg, priv->bind_descr);
		}
	case GX_DEMUXER_CTRL_SUB_OFF:
		{
			GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SWITCH_SUB:
		{
			GxDemuxerStreamSwitch* StreamSwitch;
			StreamSwitch = (GxDemuxerStreamSwitch*)arg;
			if (StreamSwitch) {
				if (!VAILD_PID(StreamSwitch->pid))
					return GX_DEMUXER_CTRL_ERROR;

				GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT);
				GxHwDemux_RunStream(priv->module, priv->progid, HW_DEMUX_PES_SUBT, StreamSwitch->pid, priv->bind_descr);
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_SUB_READ_DATA:
		{
			GxDemuxerPesData* pes_data = (GxDemuxerPesData*)arg;

			if ((pes_data == NULL) || (pes_data->buffer == NULL) || (pes_data->size <= 0))
				return GX_DEMUXER_CTRL_ERROR;

			pes_data->size = GxHwDemux_ReadFilterData(priv->module,
					priv->progid, HW_DEMUX_PES_SUBT, pes_data->buffer, pes_data->size);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_PSI_ON:
		{
			return GxHwDemux_RunStream(priv->module, priv->progid, HW_DEMUX_PSI_SUBT, *(int*)arg, priv->bind_descr);
		}
	case GX_DEMUXER_CTRL_SUB_PSI_OFF:
		{
			GxHwDemux_StopStream(priv->module, priv->progid, HW_DEMUX_PSI_SUBT);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_PSI_READ_DATA:
		{
			GxDemuxerPsiData* psi_data = (GxDemuxerPsiData*)arg;

			if ((psi_data == NULL) || (psi_data->buffer == NULL) || (psi_data->size <= 0))
				return GX_DEMUXER_CTRL_ERROR;

			psi_data->size = GxHwDemux_ReadFilterData(priv->module,
					priv->progid, HW_DEMUX_PSI_SUBT, psi_data->buffer, psi_data->size);
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_OPEN_AD_AUDIO:
		{
			if (!priv->ad_dmx) {
				GxDemuxerStreamSwitch* ad = (GxDemuxerStreamSwitch*)arg;
				GxStreamAudioHeader* sh_audio = NULL;
				int i = 0;

				if (!ad || !VAILD_PID(ad->pid)) {
					gxloge("[Player]: %s %d (pid %d)\n", __func__, __LINE__, ad->pid);
					return GX_DEMUXER_CTRL_ERROR;
				}

				for (i = 0; i < MAX_A_STREAMS; i++) {
					GxStreamAudioHeader *sh = demuxer->a_streams[i];

					if (sh && (ad->pid == sh->priv.id)) {
						sh_audio = sh;
						break;
					}
				}

				if (!sh_audio) {
					gxloge("[Player]: %s %d (pid %d)\n", __func__, __LINE__, ad->pid);
					return GX_DEMUXER_CTRL_ERROR;
				}

				sh_audio->priv.pts_sync = 1;
				sh_audio->ds = demuxer->audio1;
				sh_audio->format  = ad->type;
				sh_audio->priv.id = ad->pid;
				sh_audio->stream_type = AVSTREAM_TS;
				sh_audio->stream_pid  = ad->pid;
				demuxer->audio1->sh = sh_audio;
				priv->ad_dmx = GxAdDemux_Open(ad->pid, DEMUX_SDRAM);
				return GX_DEMUXER_CTRL_OK;
			} else {
				gxloge("[Player]: %s %d\n", __func__, __LINE__);
			}

			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_CLOSE_AD_AUDIO:
		{
			if (priv->ad_dmx) {
				GxAdDemux_Close(priv->ad_dmx);
				priv->ad_dmx = NULL;
				demuxer->audio1->sh = NULL;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_RUN_AD_AUDIO:
		{
			if (priv->ad_dmx) {
				GxPin *pin = demuxer->audio1->pin;

				if (!pin)
					return GX_DEMUXER_CTRL_ERROR;

				GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->bind_descr);
				GxAdDemux_Run(priv->ad_dmx);

				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_STOP_AD_AUDIO:
		{
			if (priv->ad_dmx) {
				GxAdDemux_Stop(priv->ad_dmx);
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_RESET_AD_AUDIO:
		{
			if (priv->ad_dmx) {
				GxPin *pin = demuxer->audio1->pin;

				if (!pin)
					return GX_DEMUXER_CTRL_ERROR;

				GxAdDemux_Stop(priv->ad_dmx);
				GxAdDemux_Config(priv->ad_dmx, pin->fifo, priv->bind_descr);
				GxAdDemux_Run(priv->ad_dmx);
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_PAUSE_FILL_DATA:
		priv->pause_fill_data = 1;
		return GX_DEMUXER_CTRL_OK;
	case GX_DEMUXER_CTRL_RESUME_FILL_DATA:
		priv->pause_fill_data = 0;
		return GX_DEMUXER_CTRL_OK;
	case GX_DEMUXER_CTRL_UPDATE_TIMEMS:
		avtimer_update(priv->timer);
		priv->timer_basems = *(int64_t *)arg;
		return GX_PLAYER_OK;
	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
}

static int demux_hwts_run(GxMediaFilter* filter)
{
	GxDemuxer* demuxer = GXDEMUXER(filter);
	DemuxHwTSPriv* priv = (DemuxHwTSPriv*)demuxer->priv;

	if (priv->running)
		return GX_PLAYER_OK;

	priv->running = 1;
	GxCore_ThreadCreate("demux_hwts_depack",
			&demuxer->pthread_depack,
			demux_hwts_depack_thread,
			filter, 1024*12, GXOS_DEFAULT_PRIORITY - 1);

	GxCore_ThreadCreate("demux_hwts_check",
			&demuxer->pthread_check,
			demux_hwts_check_thread,
			filter, 1024*8, GXOS_DEFAULT_PRIORITY);

	GxHwDemux_Run(priv->module,priv->progid,HW_DEMUX_FUNC_DEMUX);

	if (!demuxer->audio1->dropmode) {
		if (priv->ad_dmx)
			GxAdDemux_Run(priv->ad_dmx);
	}
	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int hwts_clr_frame_cnt(GxDemuxer* demuxer)
{
	GxStreamVideoHeader *sh_video = demuxer->video->sh;
	GxStreamAudioHeader *sh_audio = demuxer->audio->sh;

	if (sh_video)
		sh_video->stat.play_frame_cnt = 0;
	if (sh_audio)
		sh_audio->stat.play_frame_cnt = 0;

	return 0;
}

static int demux_hwts_seek(GxDemuxer* demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret;
	off_t pos;
	GxMediaFilter* mf   = (GxMediaFilter*)demuxer;
	DemuxHwTSPriv* priv = (DemuxHwTSPriv*)demuxer->priv;

	if (mf->status == GX_MFT_STATE_STOPPED) {
		pos = hwts_get_target_time_pos(demuxer, rel_seek_ms, flags);
		if (pos >= 0)
			hwts_seek(demuxer, pos);

		hwts_clr_frame_cnt(demuxer);
		avtimer_update(priv->timer);
		priv->timer_basems = rel_seek_ms;
		return GX_PLAYER_OK;
	}

	demux_hwts_stop(mf);

	if (GX_SPEED_NORMAL(demuxer->speed)) {
		CHANGE_DROPMODE(demuxer->sub->dropmode,    DROPMODE_NONE);
		CHANGE_DROPMODE(demuxer->audio->dropmode,  DROPMODE_NONE);
		CHANGE_DROPMODE(demuxer->audio1->dropmode, DROPMODE_NONE);
	}
	ret = demux_hwts_config(mf);
	if (ret == GX_PLAYER_OK) {
		pos = hwts_get_target_time_pos(demuxer, rel_seek_ms, flags);
		if (pos >= 0)
			hwts_seek(demuxer, pos);
		hwts_get_target_cur_time(demuxer);
		demux_hwts_run(mf);
	}
	hwts_clr_frame_cnt(demuxer);
	avtimer_update(priv->timer);
	priv->timer_basems = rel_seek_ms;

	return ret;
}

static int demux_hwts_pause(GxMediaFilter* filter)
{
	GxDemuxer  *demuxer = GXDEMUXER(filter);
	DemuxHwTSPriv *priv = (DemuxHwTSPriv*)demuxer->priv;

	while (priv->paused == 0) {
		GxCore_ThreadDelay(10);
	}

	GxDemuxer_STCPause(demuxer);
	avtimer_pause(priv->timer);

	return GX_PLAYER_OK;
}

static int demux_hwts_resume(GxMediaFilter* filter)
{
	GxDemuxer* demuxer = GXDEMUXER(filter);
	DemuxHwTSPriv *priv = (DemuxHwTSPriv*)demuxer->priv;

	GxDemuxer_STCResume(demuxer);
	avtimer_resume(priv->timer);

	return GX_PLAYER_OK;
}

static int demux_hwts_stop(GxMediaFilter* filter)
{
	DemuxHwTSPriv* priv;
	GxDemuxer* demuxer = GXDEMUXER(filter);

	if (demuxer && demuxer->priv) {
		priv = (DemuxHwTSPriv*)demuxer->priv;

		if (priv) {
			if (!priv->running) {
				if (priv->ad_dmx)
					GxAdDemux_Stop(priv->ad_dmx);
				if ((priv->progid >= 0) && (priv->module >= 0))
					GxHwDemux_Stop(priv->module, priv->progid, HW_DEMUX_FUNC_DEMUX);
				return GX_PLAYER_OK;
			}

			priv->running = 0;
			priv->abort = 1;

			GxCore_SemPost(demuxer->semaphore_exit);

			if (demuxer->pthread_depack) {
				GxCore_ThreadJoin(demuxer->pthread_depack);
				demuxer->pthread_depack = 0;
			}

			if (demuxer->pthread_check) {
				GxCore_ThreadJoin(demuxer->pthread_check);
				demuxer->pthread_check = 0;
			}

			if (priv->ad_dmx)
				GxAdDemux_Stop(priv->ad_dmx);

			GxHwDemux_Stop(priv->module, priv->progid, HW_DEMUX_FUNC_DEMUX);

			if (priv->apin && priv->apin->fifo) {
				GxFifo_Reset(priv->apin->fifo);
			}

			if (priv->vpin && priv->vpin->fifo) {
				GxFifo_Reset(priv->vpin->fifo);
			}

			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static void demux_hwts_close(GxDemuxer* demuxer)
{
	demux_hwts_stop((GxMediaFilter*)demuxer);

	if (demuxer->priv) {
		DemuxHwTSPriv *priv = demuxer->priv;

		if (priv->timer) {
			avtimer_destroy(priv->timer);
			priv->timer = NULL;
			priv->timer_basems = 0;
		}

		if (priv->ad_dmx) {
			GxAdDemux_Close(priv->ad_dmx);
			priv->ad_dmx = NULL;
		}

		if (priv->progid >= 0) {
			GxHwDemux_FreeProg(priv->module,priv->progid);
			priv->progid = -1;
		}

		if (priv->module >= 0) {
			GxHwDemux_SetSource(priv->module, priv->default_source);
			GxHwDemux_CloseModule(priv->module);
			priv->module = -1;
		}

		if (priv->fd) {
			recfile_close(priv->fd);
		}

		if (priv->infifo) {
			GxFifo_Destroy(priv->infifo);
		}

		if (priv->mutex) {
			GxCore_MutexDelete(priv->mutex);
		}

		if (priv->dev >= 0) {
			GxAvdev_DestroyDevice(priv->dev);
			priv->dev = -1;
		}


		av_free(demuxer->priv);
		demuxer->priv = NULL;
	}
}

static int demux_hw_init(void)
{
	return GxHwDemux_Init();
}


GxDemuxerClass gx_demux_hwts = {
	._inherit = {
		._inherit = {
			.name    = "Demuxer HW TS",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.init    = demux_hw_init,
			.create  = NULL,
			.release = NULL,
		},
		.run    = demux_hwts_run,
		.pause  = demux_hwts_pause,
		.resume = demux_hwts_resume,
		.config = demux_hwts_config,
		.stop   = demux_hwts_stop,
	},
	.name        = "Demuxer HW TS",
	.type        = GX_DEMUXER_TYPE_HW_TS,
	.check_file  = demux_hwts_check_file,
	.open        = demux_hwts_open,
	.close       = demux_hwts_close,
	.fill_buffer = NULL,
	.seek        = demux_hwts_seek,
	.control     = demux_hwts_control,
};

