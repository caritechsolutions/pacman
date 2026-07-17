#include "gx_demux.h"
#include "gx_decoder.h"
#include "stheader.h"
#include "gx_subreader.h"
#include "gx_system.h"
#include "gx_subtitle.h"
#include "gx_device.h"
#include "gx_muxer.h"
#include "demux_dump.h"
#include "gx_media.h"


#define DEBUG_PACKET_SPEED

#define DEMUXSTREAM_SPEED_TIME (2000)

static int _demux_fillbuffer(GxDemuxer* demuxer, GxDemuxStream* ds)
{
	int ret = GX_PLAYER_ERROR;
	GxDemuxerClass* dc = GetGxDemuxerClassFromObject(demuxer);

#ifdef DEBUG_PACKET_SPEED
	unsigned int debug_pkt_speed = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_PACKET_SPEED, &debug_pkt_speed);
	if (debug_pkt_speed)
		demuxer->debug_fill_start_time = av_get_tick_ms(NULL, 0);
#endif

	if (dc && dc->fill_buffer)
		ret = dc->fill_buffer(demuxer, ds);

	return ret;
}

static int _demux_sync_packet(GxDemuxStream *ds, GxDemuxPacket *dp)
{
	int ret = 0;
	int debug_demuxer_apts = 0;
	GxDemuxer *demuxer = ds->demuxer;

#ifdef DEBUG_PACKET_SPEED
	unsigned int debug_pkt_speed = 0;

	GxPlayer_SystemGet(PSYS_DEBUG_PACKET_SPEED, &debug_pkt_speed);
	if (debug_pkt_speed) {
		if (demuxer->debug_fill_start_time != 0) {
			if (ds == demuxer->video) {
				unsigned int cost_time = av_get_tick_ms(NULL, 0) - demuxer->debug_fill_start_time;
				demuxer->debug_video_cost_time += cost_time;
				demuxer->debug_video_data_size += dp->len;
				if (cost_time > DEBUG_MAX_PACKET_TIME_MS) {
					gxlogi_raw("video packet speed %7.2f KB/s, cost time %d",
							((double)dp->len / 1024.0 * 1000) / cost_time, cost_time);
				}
			} else if (ds == demuxer->audio) {
				unsigned int cost_time = av_get_tick_ms(NULL, 0) - demuxer->debug_fill_start_time;
				demuxer->debug_audio_cost_time += cost_time;
				demuxer->debug_audio_data_size += dp->len;
				if (cost_time > DEBUG_MAX_PACKET_TIME_MS) {
					gxlogi_raw("audio packet speed %7.2f KB/s, cost time %d",
							((double)dp->len / 1024.0 * 1000) / cost_time, cost_time);
				}
			}

			if (demuxer->debug_video_cost_time > DEBUG_VIDEO_PACKET_TIME_MS) {
				gxlogi_raw("video packet speed %7.2f KB/s",
						((double)demuxer->debug_video_data_size / 1024.0 * 1000) / demuxer->debug_video_cost_time);
				demuxer->debug_video_cost_time = 0;
				demuxer->debug_video_data_size = 0;
			}

			if (demuxer->debug_audio_cost_time > DEBUG_AUDIO_PACKET_TIME_MS) {
				gxlogi_raw("audio packet speed %7.2f KB/s",
						((double)demuxer->debug_audio_data_size / 1024.0 * 1000) / demuxer->debug_audio_cost_time);
				demuxer->debug_audio_cost_time = 0;
				demuxer->debug_audio_data_size = 0;
			}

			demuxer->debug_fill_start_time = 0;
		}
	}
#endif
	if (demuxer->fill_pkt_time == 0) {
		demuxer->fill_pkt_time = av_get_tick(NULL, 0);
		demuxer->last_fill_pkt_size   = demuxer->fill_pkt_size;
	}

	demuxer->fill_pkt_size += dp->len;

	if (demuxer->fill_pkt_time != 0) {
		unsigned int cur_ms = av_get_tick(NULL, 0);
		/*2s, check packet size.*/
		if ((cur_ms - demuxer->fill_pkt_time) > DEMUXSTREAM_SPEED_TIME) {
			demuxer->fill_pkt_speed = (((demuxer->fill_pkt_size - demuxer->last_fill_pkt_size)/ 1024) * 1000) / (cur_ms - demuxer->fill_pkt_time);
			demuxer->fill_pkt_time  = cur_ms;
			demuxer->last_fill_pkt_size   = demuxer->fill_pkt_size;
		}
	}

	if (ds->fill_pkt_sync) {
		GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_APTS, &debug_demuxer_apts);
		if (demuxer->audio == ds) {
			if ((dp->pts != 0) && (dp->pts != -1)) {
				int64_t current = 0, pts = 0;

				if((demuxer->type == GX_DEMUXER_TYPE_MPEG_TS) ||
						(demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH))
					pts = dp->pts;
				else
					pts = dp->pts - demuxer->start_pts;

				if (HAVE_VIDEO(demuxer))
					current = GxDemuxer_GetCurrentVpts(demuxer);
				else
					current = GxDemuxer_GetCurrentSTC(demuxer);

				if ((pts > current) || ((pts + 10000) < current)) {
					gxlogd("find: pts %lld, current %lld\n", pts, current);
					ds->fill_pkt_sync = 0;
				} else {
					gxlogd("skip: pts %lld, current %lld\n", pts, current);
					ret = 1;
				}
			} else
				ret = 1;
		} else if (demuxer->video == ds) {
			if (dp->pos >= ds->fill_pkt_sync_pos) {
				if (dp->pos != ds->fill_pkt_sync_pos)
					gxlogi("--> sync pos %lld, pkt pos %lld size %d\n",
								ds->fill_pkt_sync_pos, dp->pos, dp->len);
				ds->fill_pkt_sync = 0;
			}
			ret = 1;
		}
	}

	if (dp->len > 0)
		ds->compute_es_size += dp->len;

	if ((dp->pts != 0) && (dp->pts != -1)) {
#define COMPUTE_PACKET_COUNT (50)
#define COMPUTE_JUMP_COUNT   (3)
		if((demuxer->type == GX_DEMUXER_TYPE_MPEG_TS) ||
				(demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH))
			ds->fill_pkt_pts = dp->pts;
		else
			ds->fill_pkt_pts = dp->pts - demuxer->start_pts;

		if (ds->compute_pkt_count == 0) {
			ds->compute_first_pts = dp->pts;
			ds->compute_first_pos = dp->pos;
			ds->compute_es_size   = 0;
		} else if (ds->compute_pkt_count >= COMPUTE_PACKET_COUNT) {
			int64_t dis_pts = abs(dp->pts - ds->compute_first_pts);
			int64_t dis_pos = abs(dp->pos - ds->compute_first_pos);

			if (dis_pts != 0) {
				int stream_bitrate = 1000 * (8 * dis_pos) / dis_pts;
				int es_bitrate     = 1000 * (8 * ds->compute_es_size) / dis_pts;
				if (ds->compute_stream_bitrate == 0) {
					ds->compute_stream_bitrate = stream_bitrate;
					ds->compute_es_bitrate     = es_bitrate;
					ds->compute_jump_cnt = 0;
				} else {
					int dis_bitrate = abs(ds->compute_stream_bitrate - stream_bitrate);
					if (dis_bitrate >= (ds->compute_stream_bitrate / 3)) {
						ds->compute_jump_cnt++;
						if (ds->compute_jump_cnt > COMPUTE_JUMP_COUNT) {
							ds->compute_stream_bitrate = stream_bitrate;
							ds->compute_es_bitrate     = es_bitrate;
							ds->compute_jump_cnt = 0;
						}
					} else {
						ds->compute_stream_bitrate = stream_bitrate;
						ds->compute_es_bitrate     = es_bitrate;
						ds->compute_jump_cnt = 0;
					}
				}
				if (0)
					gxlogi("[%s] bitrate: stream[%d, %d %d] es[%d, %d, size %lld]\n",
							(ds == demuxer->audio) ? "audio" : "video",
							ds->compute_stream_bitrate, ds->compute_jump_cnt, stream_bitrate,
							ds->compute_es_bitrate, es_bitrate, ds->compute_es_size);
				ds->compute_pkt_count = 0;
				return ret;
			}
		}
		ds->compute_pkt_count++;
	}

	return ret;
}

static size_t _demux_stream_read(GxDemuxStream *ds, unsigned char *mem, int len)
{
	int x;
	int bytes = 0;

	while (len > 0) {
		x = ds->buffer_size - ds->buffer_pos;
		if (x == 0) {
			if (GxDemuxStream_FillBuffer(ds), 0!= GX_PLAYER_OK)
				break;
		}
		else {
			if (x > len)
				x = len;
			if (mem)
				memcpy(mem + bytes, &ds->buffer[ds->buffer_pos], x);
			bytes += x;
			len -= x;
			ds->buffer_pos += x;
		}
	}

	return bytes;
}

static size_t _demux_stream_write(GxDemuxStream *ds,
		void  *data,
		size_t size,
		int64_t pts,
		int64_t pos,
		GxDemuxerPktType pkt_type)
{
	if (ds->demuxer->muxing && ds->demuxer->muxout && (pkt_type & GX_DEMUXER_PKT_CONTENT)) {
		GxMuxerPacket pkt;

		pkt.stream_index = ds->mux_index;
		pkt.data = data;
		pkt.size = size;
		pkt.pts  = pts;

		GxFifo_Write(ds->demuxer->muxout->fifo, (uint8_t *)&pkt, sizeof(pkt), -1);
	}

	if (ds->demuxer->playing && ds->pin && ds->pin->fifo) {
		if (pts != GX_NOPTS_VALUE) {
			pts *= ds->demuxer->stc_factor;
			pts |= 0x1;
		}

		if ((ds == ds->demuxer->video) &&
				(pkt_type == GX_DEMUXER_FLAG_PKT_HEADER)) {
			unsigned char pre_code[] = {0, 0, 0, 0};
			ds->wbytes += 4;
			GxFifo_WriteWithPts(ds->pin->fifo, (uint8_t *)pre_code, 4, 0, -1);
		}

		ds->wbytes += size;
		GxFifo_WriteWithPts(ds->pin->fifo, (uint8_t *)data, size, pts, -1);
		if (pos != 0)
			ds->fill_pkt_sync_pos = pos;

		GxDemuxDump_Write(ds->demuxer->debug_dumper, ds, (unsigned char *)data, size);

		if (GX_DEMUXER_FLAG_PKT_CONTENT & pkt_type) {
			if (ds == ds->demuxer->video) {
				av_debug_first_video_es_duty(ds->demuxer->info.debug, size);
			}
			if (ds == ds->demuxer->audio)
				av_debug_first_audio_es_duty(ds->demuxer->info.debug, size);
		}
	}

	return size;
}

int GxDemuxStream_CheckFillPacket(GxDemuxer *demuxer)
{
	if (demuxer->last_fill_pkt_size != demuxer->fill_pkt_size) {
		unsigned int cur_ms = av_get_tick(NULL, 0);

		/*5s, not packet size, is set net_speed = 0;*/
		if ((cur_ms - demuxer->fill_pkt_time) > (5*DEMUXSTREAM_SPEED_TIME/2)) {
			demuxer->fill_pkt_speed = 0;
			demuxer->last_fill_pkt_size = demuxer->fill_pkt_size;
		}
	}
	return 0;
}

GxDemuxStream* GxDemuxStream_Create(GxDemuxer *demuxer, int id)
{
	GxDemuxStream* ds = av_mallocz(sizeof(GxDemuxStream));

	GxCore_MutexCreate(&ds->mutex);
	ds->id = id;
	ds->demuxer = demuxer;

	return ds;
}

void GxDemuxStream_Destroy(GxDemuxStream *ds)
{
	GxDemuxStream_FreePacks(ds);

	if(ds->resvbuffer)
		av_free(ds->resvbuffer);

	GxCore_MutexDelete(ds->mutex);
	av_free(ds);
}

int GxDemuxStream_IsNetworkAudioDelay(GxDemuxer* demux)
{
	GxMedia* media = demux->media_priv;
	char *options = NULL;
	int is_netaudiodelay = 0;

	if (demux->stream &&
		demux->stream->options &&
		(options = GxOptions_Get_By_Name(demux->stream->options, " -H", "IsOpenNetAudioDelay:"))) {
		is_netaudiodelay = atoi(options);
	}

	if (is_netaudiodelay && MEDIA_HAS_AUDIO(media)) {
		extern int GxMedia_GetCurAudioPts(GxMedia* media);
		int a_dec_pts = 0, audio_es_time = 0, set_audio_delayms = 0, time_diff = 0;
		uint32_t time = av_get_tick(NULL, 0);
		if(demux->audio->fill_pkt_pts != GX_NOPTS_VALUE && demux->audio->fill_pkt_pts != 0) {
			a_dec_pts = GxMedia_GetCurAudioPts(media);
			if (a_dec_pts != -1) {
				a_dec_pts = a_dec_pts/demux->stc_factor;
				if (demux->audio->fill_pkt_pts > a_dec_pts)
					audio_es_time = (media->demuxer->audio->fill_pkt_pts - a_dec_pts);
			}
			audio_es_time = (audio_es_time>10000)?0:audio_es_time;
			do {
				GxPlayer_SystemGet(PSYS_NETWORK_AUDIO_DELAY, &set_audio_delayms);
				if (set_audio_delayms) {
					time_diff = (set_audio_delayms - (av_get_tick(NULL, 0) - time - audio_es_time));
					if(time_diff <= 0) {
						set_audio_delayms = 0;
						GxPlayer_SystemSet(PSYS_NETWORK_AUDIO_DELAY, &set_audio_delayms);
						return GX_PLAYER_OK;
					}
					GxCore_ThreadDelay(100);
				} else {
					break;
				}
			} while(1);
			if (audio_es_time >= 300) {
				return GX_PLAYER_ERROR;
			}
		}
	}
	return GX_PLAYER_OK;
}

int GxDemuxStream_CachePacket(GxDemuxStream *ds)
{
	GxDemuxer* demux = ds->demuxer;

	if (ds->eof)
		return -1;

	if (demux->video && NEEDDROP(demux->video->dropmode))
		GxDemuxStream_FreePacks(demux->video);
	if (demux->audio && NEEDDROP(demux->audio->dropmode))
		GxDemuxStream_FreePacks(demux->audio);
	if (demux->audio1 && NEEDDROP(demux->audio1->dropmode))
		GxDemuxStream_FreePacks(demux->audio1);

	if (_demux_fillbuffer(demux, ds) != GX_PLAYER_OK) {
		return -1;
	}

	return 0;
}

void GxDemuxStream_Reset(GxDemuxStream *ds)
{
	if (ds) {
		if (ds->sh) {
			GxStreamHeadPriv *hdr = ds->sh;
			hdr->header.flag = 0;
		}

		if (ds->pin && ds->pin->fifo) {
			GxFifo_Reset(ds->pin->fifo);
		}

		ds->eof = 0;
		ds->wbytes = 0;
	}
}

void GxDemuxStream_AddPacket(GxDemuxStream *ds, GxDemuxPacket *dp)
{
	if (ds && dp) {
		if (_demux_sync_packet(ds, dp)) {
			GxDemuxPacket_Destroy(dp);
			return;
		}

		if (ds->sh) {
			if (dp->buffer == NULL || dp->len == 0) {
				gxlogd("ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd\n");
				while(1) GxCore_ThreadDelay(10);
			}

			if (dp->buffer[dp->pool_size] != 0xa0) {
				gxlogd("eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n");
				while(1) GxCore_ThreadDelay(10);
			}

			ds->bytes += dp->len;
			if (ds->last) {
				ds->last->next = dp;
				ds->last = dp;
			}
			else
				ds->first = ds->last = dp;
			++ds->packs;
		}
		else
			GxDemuxPacket_Destroy(dp);
	}
}

void GxDemuxStream_ReadPacket(GxDemuxStream *ds, GxStream *stream, int len, uint32_t pts, off_t pos, int flags)
{
	GxDemuxPacket *dp = GxDemuxPacket_Create(ds->demuxer, NULL, len);

	if (dp) {
		len = GxStream_Read(stream, dp->buffer, len);
		if (len > 0) {
			dp->len = len;
			dp->pts = pts;
			dp->pos = pos;
			dp->flags = flags;

			GxDemuxStream_AddPacket(ds, dp);
		}
		else
			GxDemuxPacket_Destroy(dp);
	}
}

int GxDemuxStream_FillBuffer(GxDemuxStream *ds)
{
	int is_full_detection = 0;
	GxDemuxer* demux = ds->demuxer;

	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
	if (ds->current) {
		GxDemuxPacket_Destroy(ds->current);
		ds->current = NULL;
	}

	while (ds->eof == 0) {
		if (ds->packs > 0 && ds->first) {
			GxDemuxPacket *p = ds->first;

			ds->buffer       = p->buffer;
			ds->buffer_pos   = 0;
			ds->buffer_size  = p->len;
			ds->pos          = p->pos;
			ds->dpos        += p->len;

			if (p->pts != GX_NOPTS_VALUE) {
				ds->pts       = p->pts;
				ds->pts_bytes = 0;
			}
			if(is_full_detection && p->is_start_switch_dp)
			{
				ds->write_first_switch_pack_flag = 1;
				p->is_start_switch_dp = 0;
			}
			ds->pts_bytes  += p->len;
			ds->flags       = p->flags;
			ds->bytes      -= p->len;
			ds->current     = p;
			ds->first       = p->next;
			if (!ds->first)
				ds->last = NULL;
			--ds->packs;

			return GX_PLAYER_OK;
		}

#ifndef I386_LINUX
		if (demux->video && NEEDDROP(demux->video->dropmode))
			GxDemuxStream_FreePacks(demux->video);
		if (demux->audio && NEEDDROP(demux->audio->dropmode))
			GxDemuxStream_FreePacks(demux->audio);
		if (demux->audio1 && NEEDDROP(demux->audio1->dropmode))
			GxDemuxStream_FreePacks(demux->audio1);

		if (demux->sub->bytes >= demux->swdmx->cachesize/8) {
			gxlogf("[Player] To Many Sub Data Left!...Free Packs ...packs=%d,bytes=%d\n",demux->sub->packs,demux->sub->bytes);
			GxDemuxStream_FreePacks(demux->sub);
		}

		int totle_bytes = 0;
		if(demux->audio)
			totle_bytes += demux->audio->bytes;
		if(demux->audio1)
			totle_bytes += demux->audio1->bytes;
		if(demux->video)
			totle_bytes += demux->video->bytes;

		if (totle_bytes >= demux->swdmx->cachesize/2) {
			int esa_size = 0, esv_size = 0;
			if(demux->audio->pin && demux->audio->pin->fifo)
				esa_size = GxFifo_GetLength(demux->audio->pin->fifo);
			if(demux->video->pin && demux->video->pin->fifo)
				esv_size = GxFifo_GetLength(demux->video->pin->fifo);
			gxlogf("[Player][fill] a=%d v=%d esa=%d esv=%d\n",demux->audio->bytes, demux->video->bytes, esa_size, esv_size);
			ds->buffer_pos = ds->buffer_size = 0;
			ds->buffer  = NULL;

			return GX_PLAYER_ERROR;
		}
#endif

		if (_demux_fillbuffer(demux, ds) != GX_PLAYER_OK) {
			GxDemuxStream_FreePacks(ds);
			gxlogf("[Player] Demux Stream [%d] Finish !...\n",ds->id);
			break;
		}
	}

	ds->buffer_pos     = 0;
	ds->buffer_size    = 0;
	ds->buffer         = NULL;
	ds->eof            = (ds->packs > 0) ? 0 : 1;

	demux->stream->eof = 1;//if a or v eof; then s eof;

	return GX_PLAYER_ERROR;
}

/**
 *  \brief read data until the given 3-byte pattern is encountered, up to maxlen
 *  \param mem memory to read data into, may be NULL to discard data
 *  \param maxlen maximum number of bytes to read
 *  \param read number of bytes actually read
 *  \param pattern pattern to search for (lowest 8 bits are ignored)
 *  \return whether pattern was found
 */
int GxDemuxStream_Pattern3(GxDemuxStream *ds, unsigned char *mem, int maxlen, int *read, uint32_t pattern)
{
	register uint32_t head = 0xffffff00;
	register uint32_t pat = pattern & 0xffffff00;
	int total_len = 0, ret;

	do {
		register unsigned char* ds_buf = &ds->buffer[ds->buffer_size];
		int len = ds->buffer_size - ds->buffer_pos;
		register long pos = -len;

		if (pos >= 0) {
			if (GxDemuxStream_FillBuffer(ds)!= GX_PLAYER_OK)
				return 0;
			else
				continue;
		}
		do {
			head |= ds_buf[pos];
			head <<= 8;
		} while (++pos && head != pat);

		len += pos;
		if (total_len + len > maxlen)
			len = maxlen - total_len;
		len = _demux_stream_read(ds, mem ? &mem[total_len] : NULL, len);
		total_len += len;
	} while ((head != pat || total_len < 3) && total_len < maxlen && !ds->eof);

	if (read)
		*read = total_len;
	ret = total_len >= 3 && head == pat;

	return ret;
}

void GxDemuxStream_FreePacks(GxDemuxStream *ds)
{
	GxDemuxPacket *dp;

	dp = ds->first;
	while (dp) {
		GxDemuxPacket* dn = dp->next;
		GxDemuxPacket_Destroy(dp);
		dp = dn;
	}

	ds->first = ds->last = NULL;
	ds->packs = 0;
	ds->bytes = 0;
	if (ds->current) {
		GxDemuxPacket_Destroy(ds->current);
		ds->current = NULL;
	}
	ds->buffer = NULL;
	ds->buffer_pos = ds->buffer_size;
	ds->pts = 0;
	ds->pts_bytes = 0;

}

void GxGxDemuxStream_FreePacks(GxDemuxStream *ds)
{
	GxDemuxStream_FreePacks(ds);
}

size_t GxDemuxStream_GetPacket(GxDemuxStream *ds, unsigned char **start)
{
	int len = 0;

	if (ds->buffer_pos >= ds->buffer_size) {
		if (GxDemuxStream_FillBuffer(ds)!= GX_PLAYER_OK) {
			*start = NULL;
			return -1;
		}
	}
	len = ds->buffer_size - ds->buffer_pos;
	*start = &ds->buffer[ds->buffer_pos];
	ds->buffer_pos += len;

	return len;
}

size_t GxDemuxStream_GetPacketPts(GxDemuxStream *ds,
		unsigned char **start,
		int64_t *pts,
		int64_t *pos,
		struct gx_ctrl_info* ctrlinfo)
{
	int len;
	*pts = GX_NOPTS_VALUE;

	if (ds->buffer_pos >= ds->buffer_size) {
		if (GxDemuxStream_FillBuffer(ds)!= GX_PLAYER_OK ) {
			*start = NULL;
			return -1;
		}
	}

	if((ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_TS) ||
		(ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH))
		*pts = ds->current->pts;
	else
		*pts = ds->current->pts - ds->demuxer->start_pts;

	if (pos)
		*pos = ds->current->pos;

	if(ctrlinfo){
		ctrlinfo->ad_tags      = ds->current->ctrlinfo.ad_tags;
		ctrlinfo->ad_fade_byte = ds->current->ctrlinfo.ad_fade_byte;
		ctrlinfo->ad_pan_byte  = ds->current->ctrlinfo.ad_pan_byte;
	}
	len = ds->buffer_size - ds->buffer_pos;
	*start = &ds->buffer[ds->buffer_pos];
	ds->buffer_pos += len;

	return len;
}

size_t GxDemuxStream_GetPacketSub(GxDemuxStream *ds, unsigned char **start, int64_t *pts, int64_t *endpts)
{
	int len = -1;

	*pts = GX_NOPTS_VALUE;
	*endpts = 0;

	if (ds->current) {
		GxDemuxPacket_Destroy(ds->current);
		ds->current = NULL;
	}

	if (ds->buffer_pos >= ds->buffer_size) {
		if (ds->packs) {
			GxDemuxPacket* p = ds->first;

			ds->buffer      = p->buffer;
			ds->buffer_pos  = 0;
			ds->buffer_size = p->len;
			ds->pos         = p->pos;
			ds->dpos       += p->len;

			if (p->pts != GX_NOPTS_VALUE) {
				ds->pts       = p->pts;
				ds->pts_bytes = 0;
				*endpts       = p->endpts;
			}

			ds->pts_bytes += p->len;
			//if (p->stream_pts != GX_NOPTS_VALUE)
			//	ds->demuxer->stream_pts = p->stream_pts;
			ds->flags   = p->flags;
			ds->bytes  -= p->len;
			ds->current = p;
			ds->first   = p->next;
			if (!ds->first)
				ds->last = NULL;
			--ds->packs;
		}
		else {
			*start = NULL;
			goto out;
		}
	}

	if (!ds->buffer_pos)
		*pts = ds->pts;

	if((ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_TS) ||
		(ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH))
		*pts = ds->pts;
	else
		*pts = ds->pts - ds->demuxer->start_pts;

	len             = ds->buffer_size - ds->buffer_pos;
	*start          = &ds->buffer[ds->buffer_pos];
	ds->buffer_pos += len;

out:
	return len;
}

double GxDemuxStream_GetNextPts(GxDemuxStream *ds)
{
	double pts = GX_NOPTS_VALUE;
	GxDemuxer* demux = ds->demuxer;

	while (!ds->first) {
		if (demux->audio->packs >= MAX_PACKS || demux->audio->bytes >= MAX_PACK_BYTES)
			goto out;
		if (demux->video->packs >= MAX_PACKS || demux->video->bytes >= MAX_PACK_BYTES)
			goto out;
		if (_demux_fillbuffer(demux, ds))
			goto out;
	}
	pts = ds->first->pts;
out:
	return pts;
}


static GxDemuxPacket* GxDemuxStream_FillPacket(GxDemuxStream *ds, int depack)
{
	int retry = 0;
	GxDemuxer* demux = ds->demuxer;

	while (ds->eof == 0) {
		if (ds->packs > 0)
			return ds->first;
		else if (demux->stream->file_format != GX_STREAMTYPE_DEMUXER) {
			if (retry > 50)
				return NULL;
		}

		if ((depack == 0) || (demux->pause_depack == 1))
			return NULL;

#ifndef I386_LINUX
		if (demux->video && NEEDDROP(demux->video->dropmode))
			GxDemuxStream_FreePacks(demux->video);
		if (demux->audio && NEEDDROP(demux->audio->dropmode))
			GxDemuxStream_FreePacks(demux->audio);
		if (demux->audio1 && NEEDDROP(demux->audio1->dropmode))
			GxDemuxStream_FreePacks(demux->audio1);

		if (demux->sub->bytes >= demux->swdmx->cachesize/8) {
			gxlogf("[Player] To Many Sub Data Left!...Free Packs ...packs=%d,bytes=%d\n",demux->sub->packs,demux->sub->bytes);
			GxDemuxStream_FreePacks(demux->sub);
		}

		int totle_bytes = 0;
		if(demux->audio)
			totle_bytes += demux->audio->bytes;
		if(demux->audio1)
			totle_bytes += demux->audio1->bytes;
		if(demux->video)
			totle_bytes += demux->video->bytes;

		int esa_size = 0, esv_size = 0;
		if (totle_bytes >= demux->swdmx->cachesize/2) {
			if(demux->audio->pin && demux->audio->pin->fifo)
				esa_size = GxFifo_GetLength(demux->audio->pin->fifo);
			if(demux->video->pin && demux->video->pin->fifo)
				esv_size = GxFifo_GetLength(demux->video->pin->fifo);
			if((demux->audio->bytes==0 && esa_size==0) || (demux->video->bytes==0 && esv_size==0))
				gxlogf("[Player][probe] a=%d v=%d esa=%d esv=%d\n",demux->audio->bytes, demux->video->bytes, esa_size, esv_size);
			return NULL;
		}
#endif

		if (_demux_fillbuffer(demux, ds) != GX_PLAYER_OK) {
			GxDemuxStream_FreePacks(ds);
			gxlogf("[Player] Demux Stream [%d] Finish !...\n",ds->id);
			break;
		}

		retry++;
	}

	ds->buffer_pos   = 0;
	ds->buffer_size  = 0;
	ds->buffer       = NULL;
	ds->eof          = (ds->packs > 0) ? 0 : 1;

	demux->stream->eof = 1;//if a or v eof; then s eof;

	return NULL;
}

size_t GxDemuxStream_ProbePacketPts(GxDemuxStream *ds, unsigned char **start, int64_t *pts, int depack)
{
	int len;
	*pts = GX_NOPTS_VALUE;
	GxDemuxPacket* dp = ds->current;

	if ((dp = GxDemuxStream_FillPacket(ds, depack)) == NULL) {
		*start = NULL;
		return -1;
	}

	if((ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_TS) ||
		(ds->demuxer->type == GX_DEMUXER_TYPE_MPEG_DASH))
		*pts = dp->pts;
	else
		*pts = dp->pts - ds->demuxer->start_pts;

	len = dp->len;
	*start = dp->buffer;

	return len;
}

int GxDemuxStream_PushData(GxDemuxStream *ds)
{
	int size = 0, freesize, is_full_detection = 0;
	int debug_demuxer_apts = 0, debug_demuxer_vpts = 0;
	int64_t pts = 0, pos = 0;;
#if PRINT_DEMUX
	static int64_t a_pts = 0, v_pts = 0;
#endif
	unsigned char* start = NULL;
	struct gx_ctrl_info ctrlinfo;
	GxStreamHeadPriv* hdr = ds->sh;
	GxDemuxer* demuxer = ds->demuxer;
	GxMediaFilter* mf = GXMEDIAFILTER(ds->demuxer);
	GxDemuxerClass* cls = GetGxDemuxerClassFromObject(demuxer);

	if (ds->sh == NULL || ds->fill_error || NEEDDROP(ds->dropmode) ||
			(demuxer->playing && (ds->pin == NULL || ds->pin->fifo == NULL)) ||
			(demuxer->muxing  && (demuxer->muxout == NULL))) {
		return -2;
	}

	GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_APTS, &debug_demuxer_apts);
	GxPlayer_SystemGet(PSYS_DEBUG_DEMUX_VPTS, &debug_demuxer_vpts);
	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
	do {
		if (GX_SPEED_JUMP(demuxer->stcfreq)) {
			int32_t  frameok = 0;
			GxMediaFilter* next_mf = demuxer->video->pin->link->filter;

#ifndef SPEED_JUMP_RESET_VDEC
			GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_ONE_FRAME_OVER, &frameok);
#else
			GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_CHECK_FRAME_OK, &frameok);
#endif
			if(frameok || demuxer->video->eof) {
				int64_t seek_timems, seek_minims, current;

				if(frameok) {
#ifndef SPEED_JUMP_RESET_VDEC
					hdr->header.flag = 0;
					GxFifo_Rollback(demuxer->video->pin->fifo, -1);
					GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_CONTINUE, NULL);
#else
					GxDemuxStream_Reset(ds);
					GxVideoDecoder_Control((GxVideoCodec*)next_mf, GX_VD_CTRL_JUMP_RESET, NULL);
#endif
				}
				current = (int64_t)GxDemuxer_GetCurrentSTC(demuxer);
				if(demuxer->type == GX_DEMUXER_TYPE_MPEG_TS)
					current -= demuxer->start_pts;
				seek_minims = (int64_t)GxDemuxer_GetSeekMinTime(demuxer);
				if (current <= seek_minims || current >= (int64_t)demuxer->duration || demuxer->video->eof) {
					if (demuxer->stcfreq < 0) {
						if (current >= (int64_t)demuxer->duration || demuxer->video->eof) {
							demuxer->video->eof = 0;
							demuxer->stream->eof = 0;
							gxlogf("[Player] speed backward retry:%lld, eof = %d\n",
									current, demuxer->video->eof);
							goto retry;
						}
						else {
							demuxer->fbeof = 1;
						}
					}
					else {
						demuxer->stream->eof = 1;
						demuxer->video->eof  = 1;
						demuxer->audio->eof  = 1;
						demuxer->audio1->eof = 1;
						demuxer->sub->eof    = 1;
					}
					return -2;
				}
				else {
retry:
					seek_timems = current + demuxer->stcfreq / 2;
					seek_timems = (seek_timems > seek_minims) ? seek_timems : seek_minims;
					seek_timems = (seek_timems < (int64_t)demuxer->duration) ? seek_timems : demuxer->duration;
					GxDemuxer_M3U8_Seek(demuxer, &seek_timems);
					cls->seek(demuxer, seek_timems, 0, GX_DEMUXER_SEEK_ABSOLUTE);
					if (demuxer->debug_dumper)
						GxDemuxDump_Close(demuxer->debug_dumper);
					demuxer->debug_dumper = GxDemuxDump_Open(demuxer);
				}
			} else {
				if (GxFifo_GetLength(demuxer->video->pin->fifo) >= 100*1024)
					return -2;
			}
		}

		if ((ds == demuxer->audio) && (GxDemuxStream_IsNetworkAudioDelay(demuxer) != GX_PLAYER_OK))
			return -2;

		if (ds->pin) {
			size = GxDemuxStream_ProbePacketPts(ds, &start, &pts, 1);
			if (size <= 0) {
				if (demuxer->stream->eof) {
					ds->eof = (ds->packs > 0) ? 0 : 1;
					return -2;
				}
				return -1;
			}

			freesize = ds->pin->fifo->size - GxFifo_GetLength(ds->pin->fifo);
			if (size >= freesize) {
				if (size < ds->pin->fifo->size) {
					ds->fill_full_flags = 1;
					return -2;
				} else {
					gxlogi_raw("[%s] - packet size %d, fifo size %d\n",
							(ds == demuxer->video) ? "video" : "audio",
							size, ds->pin->fifo->size);
				}
			} else
				ds->fill_full_flags = 0;

			if (GxFifo_PtsIsFull(ds->pin->fifo)) {
				return -2;
			}
		}

		if (demuxer->muxout) {
			freesize = GxFifo_GetCap(demuxer->muxout->fifo) - GxFifo_GetLength(demuxer->muxout->fifo);
			if (freesize <= 4)
				return -2;
		}

		if(ds->dropmode == DROPMODE_AUDIOSWITCH) {
			do {
				size = GxDemuxStream_ProbePacketPts(ds, &start, &pts, 1);
				if (size <= 0) {
					if (demuxer->stream->eof) {
						ds->eof = (ds->packs > 0) ? 0 : 1;
						return -2;
					}
					return -1;
				}

				if(pts != GX_NOPTS_VALUE && pts != 0) {
					int64_t current = 0, sync_cur = 0;
#define DECODE_COST_TIME (500)

					if (HAVE_VIDEO(demuxer))
						current = GxDemuxer_GetCurrentVpts(demuxer);
					else
						current = GxDemuxer_GetCurrentSTC(demuxer);

					sync_cur = (current + DECODE_COST_TIME);
					if ((abs(sync_cur - pts) <= 200) || (abs(sync_cur - pts) >= 100000) || (!HAVE_VIDEO(demuxer))) {
						ds->fill_flag |= (FILL_PACKET_LESSER|FILL_PACKET_GREATER);
					} else if (sync_cur - pts > 200) {
						ds->fill_flag |= FILL_PACKET_GREATER;
					} else if (sync_cur - pts < -200) {
						ds->fill_flag |= FILL_PACKET_LESSER;
					}

					if (ds->fill_flag == (FILL_PACKET_LESSER|FILL_PACKET_GREATER)) {
						if (debug_demuxer_apts)
							gxlogi_raw("find - pts:%lld, cur:%lld, start:%lld\n", pts, current, demuxer->start_pts);
						ds->fill_flag = 0;
						ds->dropmode = DROPMODE_NONE;
						break;
					} else if (ds->fill_flag == FILL_PACKET_GREATER) {
						if (debug_demuxer_apts)
							gxlogi_raw("skip - pts:%lld, cur:%lld, start:%lld\n", pts, current, demuxer->start_pts);
						GxDemuxStream_GetPacketPts(ds, &start, &pts, NULL, NULL);
						continue;
					} else {
						if (debug_demuxer_apts)
							gxlogi_raw("rept - pts:%lld, cur:%lld, start:%lld\n", pts, current, demuxer->start_pts);
						return -2;
					}
				}
				else {
					GxDemuxStream_GetPacketPts(ds, &start, &pts, NULL, NULL);
					continue;
				}
			} while (1);
		}

		size = GxDemuxStream_GetPacketPts(ds, &start, &pts, &pos, &ctrlinfo);
		if (size <= 0)
			return -1;
		if (pts < 0)
			pts = GX_NOPTS_VALUE;

		if((ds == demuxer->audio1) && ctrlinfo.ad_tags) {
			GxStreamAudioHeader* header = demuxer->audio1->sh;
			if(header && header->dec_dev && header->dec_mod){
				GxAudioDecProperty_ContrlInfo ctrlinfo_buffer;
				ctrlinfo_buffer.esa_size     = ds->wbytes;
				ctrlinfo_buffer.ad_fade_byte = ctrlinfo.ad_fade_byte;
				ctrlinfo_buffer.ad_pan_byte  = ctrlinfo.ad_pan_byte;
				GxAVWriteContrlInfo(header->dec_dev, header->dec_mod,
						(void*)&ctrlinfo_buffer, sizeof(GxAudioDecProperty_ContrlInfo), -1);
			}
		}
		if (is_full_detection && ds->is_fdr_switch_flag) {
			if (ds == demuxer->video) {
				void **pheaders;
				pheaders = demuxer->v_streams;
				GxStreamHeadPriv *text;
				hdr = pheaders[ds->id];
			}
			if (hdr->header.flag == 0 && hdr->header.data && hdr->header.len > 0 && ds->write_first_switch_pack_flag) {
				_demux_stream_write(ds, hdr->header.data, hdr->header.len, GX_NOPTS_VALUE, 0, GX_DEMUXER_FLAG_PKT_HEADER);
				hdr->header.flag = 1;
				ds->write_first_switch_pack_flag = 0;
				ds->is_fdr_switch_flag = 0;
			}
		} else {
			if (hdr->header.flag == 0 && hdr->header.data && hdr->header.len > 0) {
				_demux_stream_write(ds, hdr->header.data, hdr->header.len, GX_NOPTS_VALUE, 0, GX_DEMUXER_FLAG_PKT_HEADER);
				hdr->header.flag = 1;
			}
		}

		if (ds->resvbuffer) {
			if (GX_SPEED_JUMP(demuxer->stcfreq))
				ds->resvpts = GX_NOPTS_VALUE;

			_demux_stream_write(ds, ds->resvbuffer, ds->resvsize, GX_NOPTS_VALUE, 0, GX_DEMUXER_FLAG_PKT_CONTENT);

			if(debug_demuxer_vpts && (ds == demuxer->video)){
				gxlogi_raw("video: pts %lld, size %d\n", ds->resvpts*demuxer->stc_factor, size);
			}else if(debug_demuxer_apts && (ds == demuxer->audio)){
				gxlogi_raw("audio0: pts %lld, size %d\n", ds->resvpts*demuxer->stc_factor, size);
			}else if(debug_demuxer_apts && (ds == demuxer->audio1)){
				gxlogi_raw("audio1: pts %lld, size %d\n", ds->resvpts*demuxer->stc_factor, size);
			}
			av_free(ds->resvbuffer);
			ds->resvbuffer = NULL;
		}

		if (ds == demuxer->audio || ds == demuxer->audio1) {
			if(pts == 0)
				pts = -1;
#if PRINT_DEMUX
			if (pts / 1000 != a_pts / 1000) {
				gxlogf("[Player] %02lld:%02lld:%02lld:%lld------a\n",pts/3600000,pts%3600000/60000,pts%60000/1000,pts);
				a_pts = pts;
			}
#endif
		}

		if (ds == demuxer->video) {
			pts += ds->demuxer->audio_sync;

			if (pts == ds->demuxer->audio_sync)
				pts = -1;
#if PRINT_DEMUX
			if (pts / 1000 != v_pts / 1000 && pts != -1) {
				gxlogf("[Player] %02lld:%02lld:%02lld:%lld------v\n",pts/3600000,pts%3600000/60000,pts%60000/1000,pts);
				v_pts = pts;
			}
#endif
		}

		if(debug_demuxer_vpts && (ds == demuxer->video)){
			gxlogi_raw("video: pts %lld, size %d\n", pts*demuxer->stc_factor, size);
		}else if(debug_demuxer_apts && (ds == demuxer->audio)){
			gxlogi_raw("audio0: pts %lld, size %d\n", pts*demuxer->stc_factor, size);
		}else if(debug_demuxer_apts && (ds == demuxer->audio1)){
			gxlogi_raw("audio1: pts %lld, size %d\n", pts*demuxer->stc_factor, size);
		}

		_demux_stream_write(ds, start, size, pts, pos, GX_DEMUXER_FLAG_PKT_CONTENT);
	} while (ds->packs && size > 0 && mf->status == GX_MFT_STATE_RUNNING);

	return 0;
}

int GxDemuxStream_PushSub(GxDemuxStream *ds)
{
	unsigned char* buf;
	unsigned char* pdata;
	size_t ac_size;
	size_t write_size;
	int size, freesize;
	static GxSub sf;
	GxDemuxer* d = ds->demuxer;
	GxMediaFilter* mf = GXMEDIAFILTER(d);

	sf.magic = GX_SUB_MAGIC;

	if (ds->sh == NULL || ds->pin == NULL || ds->pin->fifo == NULL || NEEDDROP(ds->dropmode))
		return -2;

	freesize = ds->pin->fifo->size - GxFifo_GetLength(ds->pin->fifo);
	size = GxDemuxStream_ProbePacketPts(ds, &buf, &sf.startpts, 0);
	if((size == -1)||((size + sizeof(GxSub) - 5) >= freesize)){
		//gxlogd("ds fifo '%c' full, pktsize:%d, freesize:%d\n", 's', size, freesize);
		return -2;
	}

	sf.size = GxDemuxStream_GetPacketSub(ds, &buf, &(sf.startpts), &(sf.endpts));
	if(sf.size > 5) {
		sf.pid   = (buf[0]<<24)|(buf[1]<<16)|(buf[2]<<8)|buf[3];
		sf.type  = buf[4];
		sf.size -= 5;
		gxlogf("pid:%d size:%d pts:%d\n",sf.pid,sf.size,(uint32_t)sf.startpts);
		ac_size = sizeof(GxSub) + sf.size;
		pdata = av_malloc(ac_size);
		if(pdata == NULL){
			gxlogd("%s malloc failed\n",__FUNCTION__);
			return -2;
		}
		memcpy(pdata, &sf, sizeof(GxSub));
		memcpy(pdata+sizeof(GxSub), buf+5, sf.size);

		write_size = GxFifo_Write(ds->pin->fifo, pdata, ac_size, 1000);
		if (write_size < ac_size)
			gxloge("%s %d: write_size %d, ac_size %d\n", write_size, ac_size);

		if (pdata)
			av_free(pdata);
	}

	return 0;
}

int GxDemuxStream_GetEsBufSize(GxDemuxStream *ds)
{
	int es_size = 0;
	GxDemuxer *demuxer = ds->demuxer;

	if (demuxer) {
		if ((ds == demuxer->audio) && demuxer->audio->sh) {
			if(demuxer->audio->pin && demuxer->audio->pin->fifo)
				es_size = GxFifo_GetLength(demuxer->audio->pin->fifo);
		} else if ((ds == demuxer->video) && demuxer->video->sh) {
			if(demuxer->video->pin && demuxer->video->pin->fifo)
				es_size = GxFifo_GetLength(demuxer->video->pin->fifo);
		}
	}

	return es_size;
}

