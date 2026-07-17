#include "xtr_mjpeg.h"
#include "xtr_mpeg4.h"
#include "bluray_pcm.h"
#include "demux_mpegdash.h"
#include "adpcm.h"

#define CHECK_DP_INVALID(dp)\
	do {\
		if(dp == NULL) {\
			gxlogd("###demux mpegdash out of memory, %d, %s###\n", __LINE__, __FILE__);\
			goto oom;\
		}\
	}while(0)

#define MAX_BASE_URL_LEN 128

static const AVCodecTag mp_wav_tags[] = {
	{CODEC_ID_ADPCM_4XM, MKTAG('4', 'X', 'M', 'A')},
	{CODEC_ID_ADPCM_EA, MKTAG('A', 'D', 'E', 'A')},
	{CODEC_ID_ADPCM_IMA_WS, MKTAG('A', 'I', 'W', 'S')},
	{CODEC_ID_ADPCM_THP, MKTAG('T', 'H', 'P', 'A')},
	{CODEC_ID_AMR_NB, MKTAG('n', 'b', 0, 0)},
	{CODEC_ID_COOK, MKTAG('c', 'o', 'o', 'k')},
	{CODEC_ID_DSICINAUDIO, MKTAG('D', 'C', 'I', 'A')},
	{CODEC_ID_INTERPLAY_DPCM, MKTAG('I', 'N', 'P', 'A')},
	{CODEC_ID_MUSEPACK7, MKTAG('M', 'P', 'C', ' ')},
	{CODEC_ID_PCM_S24BE, MKTAG('i', 'n', '2', '4')},
	{CODEC_ID_PCM_S16BE, MKTAG('t', 'w', 'o', 's')},
	{CODEC_ID_PCM_S8, MKTAG('t', 'w', 'o', 's')},
	{CODEC_ID_ROQ_DPCM, MKTAG('R', 'o', 'Q', 'A')},
	{CODEC_ID_SHORTEN, MKTAG('s', 'h', 'r', 'n')},
	{CODEC_ID_TTA, MKTAG('T', 'T', 'A', '1')},
	{CODEC_ID_WAVPACK, MKTAG('W', 'V', 'P', 'K')},
	{CODEC_ID_WESTWOOD_SND1, MKTAG('S', 'N', 'D', '1')},
	{CODEC_ID_XAN_DPCM, MKTAG('A', 'x', 'a', 'n')},
	{0, 0},
};

static const AVCodecTag mp_bmp_tags[] = {
	{CODEC_ID_AMV, MKTAG('A', 'M', 'V', 'V')},
	{CODEC_ID_BETHSOFTVID, MKTAG('B', 'E', 'T', 'H')},
	{CODEC_ID_C93, MKTAG('C', '9', '3', 'V')},
	{CODEC_ID_DSICINVIDEO, MKTAG('D', 'C', 'I', 'V')},
	{CODEC_ID_DXA, MKTAG('D', 'X', 'A', '1')},
	{CODEC_ID_FLIC, MKTAG('F', 'L', 'I', 'C')},
	{CODEC_ID_IDCIN, MKTAG('I', 'D', 'C', 'I')},
	{CODEC_ID_INTERPLAY_VIDEO, MKTAG('I', 'N', 'P', 'V')},
	{CODEC_ID_ROQ, MKTAG('R', 'o', 'Q', 'V')},
	{CODEC_ID_THP, MKTAG('T', 'H', 'P', 'V')},
	{CODEC_ID_TIERTEXSEQVIDEO, MKTAG('T', 'S', 'E', 'Q')},
	{CODEC_ID_TXD, MKTAG('T', 'X', 'D', 'V')},
	{CODEC_ID_VMDVIDEO, MKTAG('V', 'M', 'D', 'V')},
	{CODEC_ID_WS_VQA, MKTAG('V', 'Q', 'A', 'V')},
	{CODEC_ID_XAN_WC3, MKTAG('W', 'C', '3', 'V')},
	{CODEC_ID_NUV, MKTAG('N', 'U', 'V', '1')},
	{CODEC_ID_HEVC, MKTAG('H', 'E', 'V', 'C')},
	{0, 0},
};

PLAYER_INTERRUPT_CBK mpegdash_interruptcbk = NULL;
static const struct AVCodecTag* mp_wav_taglists[] = { ff_codec_wav_tags, mp_wav_tags, 0 };
static const struct AVCodecTag* mp_bmp_taglists[] = { ff_codec_bmp_tags, mp_bmp_tags, 0 };

static void fourcc_video_format(GxStreamVideoHeader *sh, enum CodecID codec_id)
{
	switch(sh->format) {
	case MKTAG('H','E','V','C'):
	case MKTAG('h','e','v','c'):
	case MKTAG('H','V','C','1'):
	case MKTAG('h','v','c','1'):
	case MKTAG('H','E','V','1'):
	case MKTAG('h','e','v','1'):
		sh->format = VIDEO_CODEC_H265;
		break;
	case MKTAG('H', '2', '6', '4'):
	case MKTAG('h', '2', '6', '4'):
	case MKTAG('a', 'v', 'c', '1'):
		sh->format = VIDEO_CODEC_H264;
		break;
	case MKTAG('H', '2', '6', '3'):
	case MKTAG('h', '2', '6', '3'):
	case MKTAG('s', '2', '6', '3'):
	case MKTAG('S', '2', '6', '3'):
	case MKTAG('F', 'L', 'V', '1'):
	case MKTAG('X', '2', '6', '3'):
	case MKTAG('T', '2', '6', '3'):
	case MKTAG('L', '2', '6', '3'):
	case MKTAG('V', 'X', '1', 'K'):
	case MKTAG('Z', 'y', 'G', 'o'):
	case MKTAG('M', '2', '6', '3'):
	case MKTAG('l', 's', 'v', 'm'):
	case MKTAG('I', '2', '6', '3'):
	case MKTAG('U', '2', '6', '3'):
		sh->format = VIDEO_CODEC_H263;
		break;
	default:
		switch (codec_id)
		{
		case CODEC_ID_MPEG1:
		case CODEC_ID_MPEG2:
		case CODEC_ID_MPEG1VIDEO:
		case CODEC_ID_MPEG2VIDEO:
		case CODEC_ID_MPEG2VIDEO_XVMC:
			sh->format = VIDEO_CODEC_MPEG12;
			break;
		case CODEC_ID_H263:
		case CODEC_ID_H263P:
		case CODEC_ID_H263I:
			sh->format = VIDEO_CODEC_H263;
			break;
		case CODEC_ID_MPEG4:
			sh->format = VIDEO_CODEC_MPEG4;
			break;
		case CODEC_ID_H264:
			sh->format = VIDEO_CODEC_H264;
			break;
		case CODEC_ID_HEVC:
			sh->format = VIDEO_CODEC_H265;
			break;
		case CODEC_ID_CAVS:
			sh->format = VIDEO_CODEC_AVS;
			break;
		default:
			sh->format = vcodec_fourcc2std(sh->format);
			break;
		}
		break;
	}
}

static void fourcc_audio_format(GxStreamAudioHeader *sh)
{
	switch(sh->format) {
	default:
		sh->format = acodec_fourcc2std(sh->format);
		break;
	}
}

#define NAL_UNIT 16
static uint8_t start_code[4] = { 0x00, 0x00, 0x00, 0x01 };

static int Packet_Write(AVPacket*  dp, const uint8_t *data, int len)
{
	int size;

	if(dp && data){
		if (len <= 0)
			return 0;

		if (dp->pos+ len <= dp->size)
			size = len;
		else
			size = dp->size - dp->pos;

		memcpy(dp->data + dp->pos, data, size);

		dp->pos+= size;

		return size;
	}
	return 0;
}

static void write_nal(AVPacket* dp, const uint8_t* data, int *ppos, int data_size, int nal_size_size)
{
	int i;
	int nal_size = 0;

	int pos =* ppos;

	if(nal_size_size)
	{
		for (i = 0; i < nal_size_size; ++i)
			nal_size = (nal_size << 8) | data[pos++];

		if ((pos + nal_size) > data_size){
			gxlogd("Track %d: nal too big\n", 0);
			nal_size = data_size - pos - 4;
			pos += 4;
		}
		Packet_Write(dp, start_code, 4);
	}
	else
		nal_size = data_size-pos;

	Packet_Write(dp, data + pos, nal_size);

	pos += nal_size;

	*ppos = pos;
}

static int nal_read_32bytes(uint8_t* data)
{
	int a = 0;

	a = (data[0]&0xff)<<24;
	a |= (data[1]&0xff)<<16;
	a |= (data[2]&0xff)<<8;
	a |= (data[3]&0xff);

	return a;
}

static int video_probe_nal_size(AVCodecContext* codec, GxStreamVideoHeader* sh_video)
{
	int nal_size = 0;
	GxStreamHeadPriv* hdr = &sh_video->priv;

	if (codec->extradata_size) {
		int priv_size = codec->extradata_size;
		uint8_t* buf  = (uint8_t *)codec->extradata;

		if(sh_video->format == VIDEO_CODEC_H264) {
			if((!codec->nal_handle) && (codec->extradata[0] == 0x01) && priv_size >= 6) {
				AVPacket pkt;
				int i, pos = 6, numsps, numpps;

				av_new_packet(&pkt, codec->extradata_size);
				pkt.pos = 0;
				nal_size = 1 + (buf[4] & 3);
				numsps = buf[5] & 0x1f;

				for (i = 0; (i < numsps) && (priv_size > pos); i++)
					write_nal(&pkt, buf, &pos, priv_size, 2);

				if (priv_size <= pos)
					return -1;

				numpps = buf[pos++];

				for (i = 0; (i < numpps) && (priv_size > pos); i++)
					write_nal(&pkt, buf, &pos, priv_size, 2);

				if(hdr) {
					if(hdr->header.data)
						av_free(hdr->header.data);

					hdr->header.data = av_malloc(pkt.pos);
					hdr->header.len  = pkt.pos;
					hdr->header.flag = 0;
					if(hdr->header.data)
						memcpy(hdr->header.data,pkt.data,pkt.pos);
				}
				av_free_packet(&pkt);
			}
			else if(hdr) {
				if(hdr->header.data)
					av_free(hdr->header.data);

				hdr->header.data = av_malloc(priv_size);
				hdr->header.len = priv_size;
				hdr->header.flag = 0;
				if (hdr->header.data)
					memcpy(hdr->header.data, buf, priv_size);
			}
		}
		else if (sh_video->format == VIDEO_CODEC_H265) {
			if(!codec->nal_handle && (buf[0] || buf[1] || buf[2] > 1) && priv_size >= 23) {
				AVPacket pkt;
				int i, j, pos = 23, numsps;
				unsigned int nal_unit_count;

				av_new_packet(&pkt, codec->extradata_size);
				pkt.pos = 0;

				nal_size = 1 + (buf[21] & 3);
				numsps = buf[22];

				for (i = 0; (i < numsps) && (priv_size > pos); i++) {
					pos++;
					nal_unit_count = (buf[pos] << 8) | buf[pos+1];
					pos += 2;

					for(j = 0; (j < nal_unit_count) && priv_size > pos; j++) {
						write_nal(&pkt, buf, &pos, priv_size, 2);
					}
				}

				if (hdr) {
					if(hdr->header.data)
						av_free(hdr->header.data);

					hdr->header.data = av_malloc(pkt.pos);
					hdr->header.len  = pkt.pos;
					hdr->header.flag = 0;
					if(hdr->header.data)
						memcpy(hdr->header.data,pkt.data,pkt.pos);
				}
				av_free_packet(&pkt);
			} else if(hdr) {
				if(hdr->header.data)
					av_free(hdr->header.data);

				hdr->header.data = av_malloc(priv_size);
				hdr->header.len = priv_size;
				hdr->header.flag = 0;
				if (hdr->header.data)
					memcpy(hdr->header.data, buf, priv_size);
			}
		} else if(hdr) {
			if(hdr->header.data)
				av_free(hdr->header.data);

			hdr->header.data = av_malloc(priv_size);
			hdr->header.len = priv_size;
			hdr->header.flag = 0;
			if (hdr->header.data)
				memcpy(hdr->header.data, buf, priv_size);
		}
	}

	return nal_size;
}

static int _mpegdash_fill(DemuxMpegDashPriv *priv, MpegDashStream *stream)
{
	int len, rlen;
	uint8_t* rbuffer;

	stream->buf_pos %= RECV_BUFFER_SIZE;
	stream->buf_len %= RECV_BUFFER_SIZE;

	rlen    = RECV_BUFFER_SIZE - stream->buf_len;
	rbuffer = stream->recv_buffer + stream->buf_len;
	len = MpegDash_Read(priv->priv, stream->type, rbuffer, rlen);
	gxlogf("%s, pos: %d, \trbuffer %p, \trlen %d, \tlen %d\n", __func__, stream->pos, rbuffer, rlen, len);

	if (len <= 0)
		return -1;

	stream->buf_len += len;
	stream->pos     += len;
	gxlogf("%s, pos: %d, \tbuf_pos %d, buf_len %d\n", __func__, stream->pos, stream->buf_pos, stream->buf_len);

	return len;

}

static int _mpegdash_seek(DemuxMpegDashPriv *priv, MpegDashStream* stream, off_t pos)
{
	if (pos < stream->pos) {
		off_t x = pos - (stream->pos - stream->buf_len);
		if (x >= 0) {
			stream->buf_pos = x;
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int _mpegdash_read(DemuxMpegDashPriv *priv, MpegDashStream *stream, uint8_t *buf, int size)
{
	int totle = size;

	while (totle > 0) {
		if(mpegdash_interruptcbk && mpegdash_interruptcbk())
			return -1;

		int x = stream->buf_len - stream->buf_pos;
		if (x > 0) {
			if (x > totle)
				x = totle;

			memcpy(buf, &stream->recv_buffer[stream->buf_pos], x);
			stream->buf_pos += x;
		} else {
			int len = _mpegdash_fill(priv, stream);
			if (len <= 0) {
				if (MpegDash_GetDownloadState(priv->priv, stream->type) == true) {
					GxCore_ThreadDelay(20);
					continue;
				}
				break;
			}
			continue;
		}

		buf   += x;
		totle -= x;
	}
	gxlogf("%s, pos: %d, \tbuf_pos %d, buf_len %d\n", __func__, (int)stream->pos, stream->buf_pos, stream->buf_len);

	return size - totle;
}

static int _mpegdash_mp_read(void*  opaque, unsigned char *buf, int size)
{
	int ret;
	GxDemuxer* demuxer          = opaque;
	DemuxMpegDashPriv *priv     = demuxer->priv;
	MpegDashStream *stream      = priv->stream[priv->stream_index];

	ret = _mpegdash_read(priv, stream, buf, size);

	return ret;
}

static offset_t _mpegdash_mp_seek(void*  opaque, offset_t pos, int whence)
{
	GxDemuxer* demuxer      = opaque;
	DemuxMpegDashPriv* priv = demuxer->priv;
	MpegDashStream *stream  = priv->stream[priv->stream_index];
	int64_t current_pos;

	if (whence == SEEK_CUR)
		pos += stream->pos + stream->buf_pos - stream->buf_len;
	else if (whence == SEEK_END && stream->end_pos > 0)
		pos += stream->end_pos;
	else if (whence == SEEK_SET)
		pos += stream->start_pos;
	else if (whence == AVSEEK_SIZE && stream->end_pos > 0)
		return stream->end_pos - stream->start_pos;
	else
		return -1;

	if (pos < 0)
		return -1;

	if (pos < stream->end_pos && stream->eof)
		;;//GxStream_Reset(stream);
	current_pos = stream->pos + stream->buf_pos - stream->buf_len;
	if (_mpegdash_seek(priv, stream, pos) == GX_PLAYER_ERROR){
		;;//GxStream_Reset(stream);
		_mpegdash_seek(priv, stream, current_pos);
		return -1;
	}

	return pos - stream->start_pos;
}

static int _mpegdash_set_info(DemuxMpegDashPriv *priv, int perNum)
{
	int i = 0, j = 0;
	int audio_stream_exist    = 0;
	int video_stream_exist    = 0;
	int subtitle_stream_exist = 0;

	if (priv->periodNum <= perNum)
		return -1;

	for (i = 0; i < MAX_MPEGDASH_STREAM_NUM; i++) {
		if (priv->stream[i] == NULL) {
			priv->stream[i] = av_mallocz(sizeof(MpegDashStream));
			if (priv->stream[i] == NULL)
				return -1;
			priv->stream[i]->type = DEMUX_MPEGDASH_UNKNOWN;
		}

		if ((priv->stream[i]->type & DEMUX_MPEGDASH_UNKNOWN) != DEMUX_MPEGDASH_UNKNOWN)
			continue;

		while (j < priv->period[perNum].adpNum) {
			if (!audio_stream_exist &&
					((MpegDash_GetMimeType(priv->priv, perNum, j, 0) & DEMUX_MPEGDASH_AUDIO) == DEMUX_MPEGDASH_AUDIO)) {
				priv->stream[i]->type = DEMUX_MPEGDASH_AUDIO;
				MpegDash_Config(priv->priv, DEMUX_MPEGDASH_AUDIO, perNum, j, 0, mpegdash_interruptcbk);
				audio_stream_exist = 1;
				priv->stream_num++;
				j++;
				break;
			} else if (!video_stream_exist &&
					((MpegDash_GetMimeType(priv->priv, perNum, j, 0) & DEMUX_MPEGDASH_VIDEO) == DEMUX_MPEGDASH_VIDEO)) {
				priv->stream[i]->type = DEMUX_MPEGDASH_VIDEO;
				MpegDash_Config(priv->priv, DEMUX_MPEGDASH_VIDEO, perNum, j, 0, mpegdash_interruptcbk);
				video_stream_exist = 1;
				priv->stream_num++;
				j++;
				break;
			} else if (!subtitle_stream_exist &&
					((MpegDash_GetMimeType(priv->priv, perNum, j, 0) & DEMUX_MPEGDASH_SUBTITLE) == DEMUX_MPEGDASH_SUBTITLE)) {
				priv->stream[i]->type = DEMUX_MPEGDASH_SUBTITLE;
				MpegDash_Config(priv->priv, DEMUX_MPEGDASH_SUBTITLE, perNum, j, 0, mpegdash_interruptcbk);
				subtitle_stream_exist = 1;
				priv->stream_num++;
				j++;
				break;
			} else {
				j++;
			}
		}

		if ((priv->stream[i]->type & DEMUX_MPEGDASH_UNKNOWN) == DEMUX_MPEGDASH_UNKNOWN) {
			av_free(priv->stream[i]);
			priv->stream[i] = NULL;
		}
	}

	return 0;
}

static int _mpegdash_get_duration(GxDemuxer *demuxer, int *duration)
{
	DemuxMpegDashPriv *priv = demuxer->priv;

	MpegDash_GetDuration(priv->priv, duration);

	return GX_PLAYER_OK;
}

static int _mpegdash_stc_run(GxDemuxer* demuxer)
{
	return GxAVSetProperty(demuxer->swdmx->dev, demuxer->swdmx->mod_stc, GxSTCPropertyID_Play, NULL, 0);
}

static void _mpegdash_stc_set_time(GxDemuxer* demuxer, int64_t time)
{
	GxSTCProperty_Time Time;
	Time.time  = time + demuxer->base_time;
	Time.time *= demuxer->stc_factor;
	GxAVSetProperty(demuxer->swdmx->dev, demuxer->swdmx->mod_stc, GxSTCPropertyID_Time, &Time, sizeof(GxSTCProperty_Time));
	demuxer->last_pts = GX_NOPTS_VALUE;

	return;
}

static int _mpegdash_probe_format(GxDemuxer* demuxer, MpegDashStream *stream)
{
	int probe_size;
	AVProbeData avpd;
	uint8_t *buf = NULL;
	DemuxMpegDashPriv *priv = demuxer->priv;

	probe_size = 2048;
	while(probe_size < PROBE_BUF_SIZE) {
		if(mpegdash_interruptcbk && mpegdash_interruptcbk()){
			if(buf)
				av_free(buf);
			return -1;
		}
		if (!buf)
			buf = av_malloc(probe_size);

		if (_mpegdash_read(priv, stream, buf, probe_size) != probe_size) {
			av_free(buf);
			buf = NULL;
			break;
		}

		avpd.filename = demuxer->stream->url;
		avpd.buf = buf;
		avpd.buf_size = probe_size;

		stream->avif = av_probe_input_format(&avpd, 1);
		av_free(buf);
		buf = NULL;
		if (stream->avif)
			break;

		probe_size = probe_size<<1;
	}

	if(!stream->avif)
		return -1;

	return 0;
}

static int _mpegdash_probe_context(GxDemuxer *demuxer, MpegDashStream *stream)
{
	int i, g, n = 0;
	char* mp_filename = NULL;
	DemuxMpegDashPriv *priv = demuxer->priv;

	stream->avfc = avformat_alloc_context();
	stream->pb = avio_alloc_context(stream->buffer, BIO_BUFFER_SIZE, 0, demuxer, _mpegdash_mp_read, NULL, _mpegdash_mp_seek);
	stream->avfc->pb = stream->pb;
	stream->avfc->flags = URL_RDONLY;

	n = strlen("mp:");
	mp_filename = av_mallocz(PLAYER_URL_LONG+n);
	if(mp_filename == NULL)
		return -1;

	if(n > 0)
		av_strlcpy(mp_filename, "mp:", n+1);
	av_strlcat(mp_filename, demuxer->stream->url, PLAYER_URL_LONG);

	stream->avfc->index_limit  = demuxer->index_limit;

	if (avformat_open_input(&(stream->avfc), (const char*)mp_filename, stream->avif, NULL) < 0) {
		av_free(mp_filename);
		return -1;
	}
	av_free(mp_filename);

	stream->avfc->file_format  = demuxer->stream->file_format;
	stream->avfc->seek_by_time = ((demuxer->stream->flags & GX_STREAM_SEEK_TIME) == GX_STREAM_SEEK_TIME)?1:0;

	if(stream->avfc->flags & AVFMT_FLAG_PTS_REORDER)
		demuxer->pts_reorder = 1;

	if (av_find_stream_info(stream->avfc, NULL) < 0) {
		return -1;
	}

	for (i = 0; i < stream->avfc->nb_streams; i++) {
		AVStream* st = stream->avfc->streams[i];
		AVCodecContext* codec = st->codec;

		switch (codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			{
				WAVEFORMATEX* wf = av_calloc(sizeof(WAVEFORMATEX) + codec->extradata_size, 1);
				GxStreamAudioHeader* sh_audio;
				const char *codec_name = NULL;
				if (priv->audio_streams >= MAX_A_STREAMS)
					break;
				sh_audio = GxStreamHeader_AudioNew(demuxer, priv->audio_streams, i);
				if (!sh_audio)
					break;
				GxStreamHeadPriv* hdr = (GxStreamHeadPriv*)sh_audio;
				priv->astreams[priv->audio_streams] = i;
				priv->audio_streams++;
				if (codec->codec_id == CODEC_ID_ADPCM_IMA_AMV)
					codec->codec_tag = MKTAG('A', 'M', 'V', 'A');

				codec_name = avcodec_get_name(codec->codec_id);
				if(codec_name){
					av_strlcpy(hdr->codec, codec_name, sizeof(hdr->codec));
				}
				codec->codec_tag = av_codec_get_tag(mp_wav_taglists, codec->codec_id);
				wf->wFormatTag = codec->codec_tag;
				wf->nChannels = codec->channels;
				wf->nSamplesPerSec = codec->sample_rate;
				wf->nAvgBytesPerSec = codec->bit_rate / 8;
				wf->nBlockAlign = codec->block_align ? codec->block_align : 1;
				wf->wBitsPerSample = codec->bits_per_sample;
				wf->cbSize = codec->extradata_size;
				if (codec->extradata_size) {
					memcpy(wf + 1, codec->extradata, codec->extradata_size);
				}
				if(sh_audio->wf)
					av_free(sh_audio->wf);
				sh_audio->wf = wf;
				sh_audio->audio.dwSampleSize = codec->block_align;
				if (codec->frame_size && codec->sample_rate) {
					sh_audio->audio.dwScale = codec->frame_size;
					sh_audio->audio.dwRate = codec->sample_rate;
				} else {
					sh_audio->audio.dwScale = codec->block_align ? codec->block_align*  8 : 8;
					sh_audio->audio.dwRate = codec->bit_rate;
				}
				g = ff_gcd(sh_audio->audio.dwScale, sh_audio->audio.dwRate);
				sh_audio->audio.dwScale /= g;
				sh_audio->audio.dwRate /= g;
				sh_audio->ds = demuxer->audio;
				sh_audio->format = codec->codec_tag;
				gxlogd("audio: %4s[0x%x]\n", (char*)&(sh_audio->format),sh_audio->format);
				fourcc_audio_format(sh_audio);
				sh_audio->channels = codec->channels;
				sh_audio->samplerate = codec->sample_rate;
				sh_audio->i_bps = codec->bit_rate / 8;
				sh_audio->codecdata_len = aac_get_sample_rate_index(codec->sample_rate);
				strncpy(sh_audio->priv.lang, st->language, 4);
				switch (codec->codec_id) {
				case CODEC_ID_PCM_S8:
				case CODEC_ID_PCM_U8:
					sh_audio->samplesize = 8;
					break;
				case CODEC_ID_PCM_S16LE:
				case CODEC_ID_PCM_U16LE:
					sh_audio->samplesize = 16;
					sh_audio->big_endian = 0;
					sh_audio->format = AUDIO_CODEC_PCM;
					break;
				case CODEC_ID_PCM_S16BE:
				case CODEC_ID_PCM_U16BE:
					sh_audio->samplesize = 16;
					sh_audio->big_endian = 1;
					sh_audio->format = AUDIO_CODEC_PCM;
					break;
				case CODEC_ID_PCM_ALAW:
					sh_audio->format = 0x6;
					break;
				case CODEC_ID_PCM_MULAW:
					sh_audio->format = 0x7;
					break;
				case CODEC_ID_PCM_BLURAY:
					sh_audio->channels   = codec->bluray_pcm.channels;
					sh_audio->big_endian = codec->bluray_pcm.big_endian;
					sh_audio->samplerate = codec->bluray_pcm.sample_rate;
					sh_audio->samplesize = codec->bluray_pcm.sample_size;
					sh_audio->format     = AUDIO_CODEC_PCM;
					break;
				case CODEC_ID_ADPCM_SWF:
					adpcm_decode_init(st->codec);
					sh_audio->channels   = codec->channels;
					sh_audio->big_endian = 0;
					sh_audio->samplerate = codec->sample_rate;
					sh_audio->samplesize = codec->bits_per_sample;
					sh_audio->format     = AUDIO_CODEC_PCM;
				default:
					break;
				}

				/* select a audio track, which is the first one we can support */
				if (demuxer->audio->id == -1 || demuxer->audio->sh == NULL) {
					demuxer->audio->id = priv->audio_streams-1;
					demuxer->audio->sh = sh_audio;
					//		priv->audio_index = i;
				}
				else
					st->discard = AVDISCARD_ALL;
				break;
			}
		case CODEC_TYPE_VIDEO:
			{
				GxStreamVideoHeader* sh_video;
				BITMAPINFOHEADER* bih;
				const char *codec_name = NULL;

				if (priv->video_streams >= MAX_V_STREAMS)
					break;

				//	video_check_format(demuxer, codec);
				sh_video = GxStreamHeader_VideoNew(demuxer, priv->video_streams, i);
				if (!sh_video)
					break;
				priv->vstreams[priv->video_streams] = i;
				priv->video_streams++;
				bih = av_calloc(sizeof(BITMAPINFOHEADER) + codec->extradata_size, 1);

				if (!codec->codec_tag)
					codec->codec_tag = av_codec_get_tag(mp_bmp_taglists, codec->codec_id);

				codec_name = avcodec_get_name(codec->codec_id);
				if(codec_name){
					av_strlcpy(sh_video->priv.codec, codec_name, sizeof(sh_video->priv.codec));
				}
				bih->biSize = sizeof(BITMAPINFOHEADER) + codec->extradata_size;
				bih->biWidth = codec->width;
				bih->biHeight = codec->height;
				bih->biBitCount = codec->bits_per_sample;
				bih->biSizeImage = bih->biWidth*  bih->biHeight * bih->biBitCount / 8;
				bih->biCompression = codec->codec_tag;
				if (codec->extradata_size) {
					memcpy(bih + 1, codec->extradata, codec->extradata_size);
				}

				if(sh_video->bih)
					av_free(sh_video->bih);
				sh_video->bih = bih;
				sh_video->disp_w = codec->width;
				sh_video->disp_h = codec->height;
				if (st->time_base.den) {	/* if container has time_base, use that*/
					sh_video->video.dwRate = st->time_base.den;
					sh_video->video.dwScale = st->time_base.num;
				} else {
					sh_video->video.dwRate = codec->time_base.den;
					sh_video->video.dwScale = codec->time_base.num;
				}
				if(codec->frame_rate == 0){
					if(st->nb_frames && st->codec->time_base.num && st->duration){
						sh_video->fps = st->codec->time_base.den*1.0*st->nb_frames/st->codec->time_base.num/st->duration;
					}
					else if (st->r_frame_rate.den){
						sh_video->fps = av_q2d(st->r_frame_rate);
					}
					else
						sh_video->fps = 0;
				}
				else
					sh_video->fps = codec->frame_rate;

				sh_video->frametime = 1 / av_q2d(st->r_frame_rate);
				sh_video->format = bih->biCompression;
				gxlogd("video: %.4s[0x%x]\n", (char*)&(sh_video->format), sh_video->format);
				fourcc_video_format(sh_video, codec->codec_id);
				sh_video->aspect = (float)codec->width/codec->height;
				sh_video->i_bps = codec->bit_rate / 8;
				sh_video->ds = demuxer->video;

				priv->nal_size_size = video_probe_nal_size(codec, sh_video);
				if (priv->nal_size_size == -1) {
					priv->nal_size_size = 0;
					break;
				}
				priv->extradata_size = codec->extradata_size;

				if (demuxer->video->id == -1 || demuxer->video->sh == NULL) {
					demuxer->video->id = priv->video_streams-1;
					demuxer->video->sh = sh_video;
					//		priv->video_index = i;
				} else
					st->discard = AVDISCARD_ALL;

				break;
			}

		case CODEC_TYPE_SUBTITLE:
			{
				GxStreamSubHeader* sh_sub;
				if (priv->sub_streams >= MAX_S_STREAMS)
					break;

				sh_sub = GxStreamHeader_SubNew(demuxer, priv->sub_streams, i);
				if (!sh_sub)
					break;

				/* only support text subtitles for now*/
				if(codec->codec_id == CODEC_ID_TEXT)
				{
					sh_sub->type = 't';
					strncpy(sh_sub->priv.codec,"srt", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_MOV_TEXT)
				{
					sh_sub->type = 'm';
					strncpy(sh_sub->priv.codec,"smi", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_SSA)
				{
					sh_sub->type = 'a';
					strncpy(sh_sub->priv.codec,"ass", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_DVD_SUBTITLE)
				{
					sh_sub->type = 'v';
					strncpy(sh_sub->priv.codec,"vobsub", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_XSUB)
				{
					sh_sub->type = 'x';
					strncpy(sh_sub->priv.codec,"vobsub", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_DVB_SUBTITLE)
				{
					sh_sub->type = 'b';
					strncpy(sh_sub->priv.codec,"DVB-Subtitle", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_DVB_TELETEXT)
				{
					sh_sub->type = 'd';
					strncpy(sh_sub->priv.codec,"DVB-Teletext", sizeof(sh_sub->priv.codec)-1);
				}
				else if(codec->codec_id == CODEC_ID_HDMV_PGS_SUBTITLE)
				{
					sh_sub->type = 'p';
					strncpy(sh_sub->priv.codec,"PGS-Subtitle", sizeof(sh_sub->priv.codec)-1);
				}
				else
					break;
				if(codec->extradata && codec->extradata_size>0){
					if(sh_sub->extradata)
						av_free(sh_sub->extradata);
					sh_sub->extradata = av_calloc(codec->extradata_size, 1);
					memcpy(sh_sub->extradata, codec->extradata, codec->extradata_size);
					sh_sub->extradata_len = codec->extradata_size;
				}
				priv->sstreams[priv->sub_streams] = i;
				priv->sub_streams++;
				memcpy(sh_sub->priv.lang,st->language,4);
				memcpy(sh_sub->priv.name,st->name,32);
				if (demuxer->sub->id == -1 || demuxer->sub->sh == NULL) {
					demuxer->sub->id = priv->sub_streams-1;
					demuxer->sub->sh = sh_sub;
					//	priv->sub_index = i;
				}
				//	priv->sub_fill = 0;
				break;
			}

		default:
			st->discard = AVDISCARD_ALL;
			break;
		}
	}
	return 0;
}

static int demuxer_debug_filldata_mpegdash = 0;
static void _mpegdash_gxlogd_fillbuffer(GxDemuxer *demuxer)
{
	int audio_demux_size = 0;
	int video_demux_size = 0;
	int esa_size = 0;
	int esv_size = 0;

	if(demuxer->audio->sh) {
		audio_demux_size = demuxer->audio->bytes;
		if(demuxer->audio->pin && demuxer->audio->pin->fifo)
			esa_size = GxFifo_GetLength(demuxer->audio->pin->fifo);
	}

	if(demuxer->video->sh) {
		video_demux_size = demuxer->video->bytes;
		if(demuxer->video->pin && demuxer->video->pin->fifo)
			esv_size = GxFifo_GetLength(demuxer->video->pin->fifo);
	}

	if (demuxer_debug_filldata_mpegdash)
		gxlogd("fill --- ea:%.02fK\tev:%.02fK\tda:%.02f(K),\tdv%.02f(K)\tsc:%.02f(K)\n",
				((double)esa_size)/1024, ((double)esv_size)/1024,
				((double)audio_demux_size)/1024, ((double)video_demux_size)/1024,
				((double)(demuxer->stream->buf_len - demuxer->stream->buf_pos)/1024));
	return ;
}

static void _mpegdash_run_thread(void *data)
{
	int ret1 = 0, ret2 = 0;
	GxDemuxer *demuxer = GXDEMUXER(data);
	GxMediaFilter* mf = GXMEDIAFILTER(data);

	while (mf->status != GX_MFT_STATE_STOPPED) {
		if (mf->status == GX_MFT_STATE_RUNNING) {
			_mpegdash_gxlogd_fillbuffer(demuxer);
			GxCore_MutexLock(demuxer->mutex);
			demuxer->video->type = DEMUX_STREAM_VIDEO;
			ret1 = GxDemuxStream_PushData(demuxer->video);
			demuxer->audio->type = DEMUX_STREAM_AUDIO;
			ret2 = GxDemuxStream_PushData(demuxer->audio);

			if(demuxer->stream->eof &&
					((!HAVE_VIDEO(demuxer)) || demuxer->video->eof || demuxer->video->dropmode) &&
					((!HAVE_AUDIO(demuxer)) || demuxer->audio->eof || demuxer->audio->dropmode)) {
				if (GxDemuxer_Restart(demuxer, NULL)){
					ret1 = ret2 = -1;
				} else {
					if(mf->event.func && demuxer->stcfreq > 0) {
						GxMediaFilterEventPara EventPara;
						EventPara.type = GX_MFT_EVENT_PLAY_END;
						EventPara.arg  = NULL;
						mf->event.func(mf->event.priv, &EventPara);
					}
					ret1  = -2;
					ret2  = -2;
				}
			}
			GxCore_MutexUnlock(demuxer->mutex);
			GxCore_ThreadYield();

			if(ret1 == -2 && ret2 == -2)
				GxCore_ThreadDelay(20);
		} else {
			GxCore_ThreadDelay(100);
		}
	}
	return;
}


static int demux_mpegdash_check_file(GxDemuxer *demuxer)
{
	DemuxMpegDashPriv *priv = NULL;
	if(!(demuxer->stream->file_format == GX_STREAMTYPE_STREAM && demuxer->stream->demuxer_type == GX_DEMUXER_TYPE_MPEG_DASH))
		return GX_DEMUXER_TYPE_UNKNOWN;

	if (!demuxer->priv)
		demuxer->priv = av_mallocz(sizeof(DemuxMpegDashPriv));

	priv = demuxer->priv;

	return GX_DEMUXER_TYPE_MPEG_DASH;
}

static int demux_mpegdash_run(GxMediaFilter* filter)
{
	GxDemuxer *demuxer = GXDEMUXER(filter);
	DemuxMpegDashPriv *priv = demuxer->priv;

	_mpegdash_stc_set_time(demuxer, demuxer->stc_start_timems);
	_mpegdash_stc_run(demuxer);

	GxCore_ThreadCreate("demux_mpegdash_depack", &priv->pthread_depack, _mpegdash_run_thread, filter, 1024*64, GXOS_DEFAULT_PRIORITY);
	return 0;
}

static int demux_mpegdash_control(GxDemuxer *demuxer,int cmd, void* args);
static GxDemuxer* demux_mpegdash_open(GxDemuxer *demuxer)
{
	int i = 0;
	char base_url[MAX_BASE_URL_LEN];
	DemuxMpegDashPriv *priv = demuxer->priv;

	GxPlayer_SystemGet(PSYS_INDEX_CACHE, &demuxer->index_limit);
	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &mpegdash_interruptcbk);

	priv->priv = MpegDash_Open(demuxer->stream->url);
	if (priv->priv == NULL)
		return NULL;

	MpegDash_GetPeriods(priv->priv, &priv->period, &priv->periodNum);

	if ((!priv->periodNum) || (priv->period == NULL))
		return NULL;

	memset(base_url, 0, MAX_BASE_URL_LEN);
	demux_mpegdash_control(demuxer, GX_DEMUXER_CTRL_GET_BASE_URL, base_url);
	MpegDash_SetBaseUrl(priv->priv, (const char*)base_url);

	_mpegdash_set_info(priv, 0);

	MpegDash_StartAll(priv->priv);

	for (i = 0; i < priv->stream_num; i++) {
		priv->stream_index = i;
		if (_mpegdash_probe_format(demuxer, priv->stream[i]) < 0)
			continue;

		_mpegdash_seek(priv, priv->stream[i], 0);

		_mpegdash_probe_context(demuxer, priv->stream[i]);

		priv->stream[i]->last_pts = -1;

	}

	return demuxer;
}

static int demux_mpegdash_fill_buffer(GxDemuxer *demuxer, GxDemuxStream* dsds)
{
	int pid;
	AVPacket pkt;
	GxDemuxPacket* dp       = NULL;
	GxDemuxStream* ds       = NULL;
	MpegDashStream *stream  = NULL;
	AVFormatContext *avfc   = NULL;
	DemuxMpegDashPriv* priv = demuxer->priv;

	int i = 0;
	for (i = 0; i < priv->stream_num; i++) {
		if ((priv->stream[i]->type & dsds->type) == priv->stream[i]->type) {
			stream = priv->stream[i];
			avfc   = stream->avfc;
			priv->stream_index = i;
			break;
		}
	}

	avfc->mov_oom_limit = demuxer->swdmx->cachesize/2 - demuxer->audio->bytes - demuxer->video->bytes - demuxer->sub->bytes;

	demuxer->filepos = stream->pos + stream->buf_pos - stream->buf_len;

	if (!priv || !avfc) {
		gxlogd("%s, %d\n", __func__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	if (av_read_frame(avfc, &pkt) < 0) {
		gxlogd("%s, %d\n", __func__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	if (pkt.data == NULL || pkt.size <= 0) {
		gxlogd("%s, %d\n", __func__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	if (priv->stream[0]->abort) {//FOURC ?
		av_free_packet(&pkt);
		gxlogd("%s, %d\n", __func__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	pid = pkt.stream_index;

	if ((stream->type & DEMUX_MPEGDASH_AUDIO) == DEMUX_MPEGDASH_AUDIO) {
		GxStreamAudioHeader* a_sh = demuxer->audio->sh;

		ds = demuxer->audio;
		if (avfc->streams[pid]->codec->codec_id == CODEC_ID_PCM_BLURAY) {
			BlurayPcmHeader hdr;
			if (pcm_bluray_parse_header(&hdr, pkt.data) == 0) {
				dp = lpcm_swith_to_pcm(demuxer, &hdr, pkt.data+4, pkt.size-4);
				CHECK_DP_INVALID(dp);
			} else
				goto fill_ok;
		} else if (avfc->streams[pid]->codec->codec_id == CODEC_ID_ADPCM_SWF) {
			dp = adpcm_decode_frame(demuxer, avfc->streams[pid]->codec, pkt.data, pkt.size);
			CHECK_DP_INVALID(dp);
		} else if (a_sh->format == AUDIO_CODEC_AAC_ADTS) {
			if (!stream->check_first_apkt) {
				if ((pkt.data[0] == 0xFF) && ((pkt.data[1] & 0xF6) == 0xF0))
					a_sh->raw_adts = 1;
				stream->check_first_apkt = 1;
			}
			if (a_sh->raw_adts) {
				/* raw adts */
				dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size);
				CHECK_DP_INVALID(dp);
				GxDemuxPacket_Write(dp, pkt.data, pkt.size);
			} else {
				dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size + ADTS_HEADER_SIZE);
				CHECK_DP_INVALID(dp);
				GxDemuxPacket_Skip(dp, ADTS_HEADER_SIZE);

				dp->buffer[0] = 0xFF;
				dp->buffer[1] = 0xF1;
				dp->buffer[2] = (0x01<<6)|(a_sh->codecdata_len<<2)|((a_sh->channels>>2)&1);
				dp->buffer[3] = (a_sh->channels << 6) | ((pkt.size+7) >>11);
				dp->buffer[4] = (((pkt.size + 7) & 0x7FF) >> 3) & 0xff;
				dp->buffer[5] = (((pkt.size + 7) & 0x07 ) << 5) | 0x1f;
				dp->buffer[6] = 0xFC;
				GxDemuxPacket_Write(dp, pkt.data, pkt.size);
			}
		} else {
			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size);
			CHECK_DP_INVALID(dp);
			GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		}
	} else if ((stream->type & DEMUX_MPEGDASH_VIDEO) == DEMUX_MPEGDASH_VIDEO) {
		GxStreamVideoHeader* v_sh = demuxer->video->sh;

		ds = demuxer->video;
		if (v_sh->format == VIDEO_CODEC_H264 || v_sh->format == VIDEO_CODEC_H265) {
			char nal_insert = 1;
			int i, numnal=0, nownal=NAL_UNIT;
			unsigned int nal_size = 0, ppos = 0, nal_size_size = 0;

			if (!priv->extradata_size && avfc->streams[pid]->codec->extradata_size) {
				if ((nal_size_size = video_probe_nal_size(avfc->streams[pid]->codec, v_sh)) != -1) {
					priv->nal_size_size  = nal_size_size;
					priv->extradata_size = avfc->streams[pid]->codec->extradata_size;
				}
			}

			if (priv->nal_size_size == 4) {
				if((pkt.size > 8) &&
						(nal_read_32bytes(pkt.data) == 0x01) &&
						(nal_read_32bytes(pkt.data+5) > pkt.size))
					nal_insert = 0;
			}

			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size + NAL_UNIT * (4-priv->nal_size_size));
			CHECK_DP_INVALID(dp);

			if (nal_insert && priv->nal_size_size) {
				while (ppos < pkt.size) {
					if (mpegdash_interruptcbk && mpegdash_interruptcbk()) {
						av_free_packet(&pkt);
						GxDemuxPacket_Destroy(dp);
						return GX_PLAYER_ERROR;
					}

					for (i = 0; i < priv->nal_size_size; ++i)
						nal_size = (nal_size << 8) | ( pkt.data[ppos++]& 0xff);

					if ((numnal++) > nownal) {
						nownal += NAL_UNIT;
						dp = GxDemuxPacket_Resize(demuxer, dp, pkt.size + nownal*(4-priv->nal_size_size));
						CHECK_DP_INVALID(dp);
					}

					GxDemuxPacket_Write(dp, start_code, 4);

					if(nal_size > pkt.size - ppos){
						gxlogd("nal_error !]#####%d: %d : %d :%d\n", nal_size, pkt.size,ppos, priv->nal_size_size);
						nal_size = pkt.size - ppos;
					}
					GxDemuxPacket_Write(dp, pkt.data + ppos, nal_size);
					ppos += nal_size;
					nal_size = 0;
				}
			} else
				GxDemuxPacket_Write(dp, pkt.data, pkt.size);

			dp = GxDemuxPacket_Resize(demuxer, dp, dp->write_size);
			CHECK_DP_INVALID(dp);
		} else {
			int dpsize;

			if (v_sh && v_sh->mpeg4_needmodify && (pkt.flags & AVINDEX_KEYFRAME))
				dpsize = pkt.size + MPEG4_EXTRA_LEN;
			else if(v_sh && v_sh->mjpeg_needmodify)
				dpsize = pkt.size + DTH_JPG_LEN;
			else
				dpsize = pkt.size;
			dp = GxDemuxPacket_Create(demuxer, NULL, dpsize);
			CHECK_DP_INVALID(dp);

			if (v_sh && v_sh->mpeg4_needmodify && (pkt.flags & AVINDEX_KEYFRAME))
				xtr_mpeg4_packet(v_sh, pkt.data, pkt.size, dp);
#ifdef CONFIG_VD_MJPEG
			else if(v_sh && v_sh->mjpeg_needmodify)
				xtr_mjpeg_packet(pkt.data, pkt.size, dp);
#endif
			else
				GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		}
	}
#if 0
	else if(priv->sub_fill) {
		int idx;
		GxStreamSubHeader* c_sh_sub = demuxer->sub->sh;
		GxStreamSubHeader* n_sh_sub = NULL;

		for (idx = 0; idx < priv->sub_streams; idx++) {
			if (priv->sstreams[idx] == pid)
				break;
		}
		if (idx >= priv->sub_streams)
			goto fill_ok;

		n_sh_sub = demuxer->s_streams[idx];
		if ((pid == priv->sub_index) ||
				(((c_sh_sub->type == 't') || (c_sh_sub->type == 'a')) &&
				 ((n_sh_sub->type == 't') || (n_sh_sub->type == 'a')))) {
			ds = demuxer->sub;
			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size+SUBT_HEADER_SIZE);
			CHECK_DP_INVALID(dp);
			GxDemuxPacket_Skip(dp, SUBT_HEADER_SIZE);
			dp->buffer[0] = (pid >> 24)&0xff;
			dp->buffer[1] = (pid >> 16)&0xff;
			dp->buffer[2] = (pid >> 8) &0xff;
			dp->buffer[3] = pid & 0xff;
			dp->buffer[4] = n_sh_sub->type&0xff;
			GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		} else
			goto fill_ok;
	}
#endif
	else
		goto fill_ok;

	if (pkt.pts != AV_NOPTS_VALUE) {
		dp->pts = pkt.pts*  (av_q2d(avfc->streams[pid]->time_base)*1000);
		if (stream->last_pts == -1)
			demuxer->start_pts = dp->pts;
		stream->last_pts = (dp->pts * AV_TIME_BASE)/1000;
		if (pkt.duration)
			dp->endpts = dp->pts + pkt.duration * (av_q2d(avfc->streams[pid]->time_base)*1000);
		else if(pkt.convergence_duration)
			dp->endpts = dp->pts + pkt.convergence_duration;
	}

	dp->pos = pkt.pos;
	dp->flags = !!(pkt.flags & PKT_FLAG_KEY);
	if (dp->pts != -1) {
		if (demuxer->base_time != 0) {
			int64_t start_time = avfc->streams[pid]->start_time;

			if (start_time == AV_NOPTS_VALUE) {
				dp->pts += demuxer->base_time;
			} else {
				start_time = (start_time*av_q2d(avfc->streams[pid]->time_base)*AV_TIME_BASE)/1000;
				dp->pts += start_time;
			}
		}
	}

	GxDemuxStream_AddPacket(ds, dp);

fill_ok:
	av_free_packet(&pkt);
	return GX_PLAYER_OK;

oom:

	av_free_packet(&pkt);
	demuxer->stream->eof = 1;
	demuxer->video->eof = 1;
	demuxer->audio->eof = 1;
	demuxer->sub->eof = 1;

	return GX_PLAYER_ERROR;
}

void demux_mpegdash_close(GxDemuxer *demuxer)
{
	DemuxMpegDashPriv *priv = demuxer->priv;

	GxCore_ThreadJoin(priv->pthread_depack);

	if (priv) {
		if (priv->priv) {
			int i, j;
			MpegDash_StopAll(priv->priv);
			MpegDash_Close(priv->priv);
			for (i = 0; i < priv->stream_num; i++) {
				if (priv->stream[i]) {
					if (priv->stream[i]->avfc) {
						for (j = 0; j < priv->stream[i]->avfc->nb_streams; j++) {
							AVStream* st = priv->stream[i]->avfc->streams[j];
							AVCodecContext* codec = st->codec;
							if (codec->codec_type == CODEC_TYPE_AUDIO && codec->codec_id == CODEC_ID_ADPCM_SWF)
								adpcm_decode_uninit(codec);
						}
						avformat_close_input(&priv->stream[i]->avfc);
					}
					av_freep(&(priv->stream[i]->pb));
					av_free(priv->stream[i]);
				}
			}
		}
		av_free(priv);
	}
	demuxer->priv = NULL;

	return;
}

static int demux_mpegdash_seek(GxDemuxer *demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	return 0;
}

static int demux_mpegdash_control(GxDemuxer *demuxer,int cmd, void* args)
{
	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		{
			int duration = 0;
			if (_mpegdash_get_duration(demuxer, &duration) == GX_PLAYER_OK) {
				demuxer->duration = *((uint64_t*)args) = (uint64_t)duration*1000;
			} else {
				demuxer->duration = *((uint64_t*)args) = 0;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_BASE_URL:
		{
			GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_BASE_URL, args);
			return GX_DEMUXER_CTRL_OK;
		}
	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}

	return 0;
}

GxDemuxerClass gx_demux_mpegdash = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer Mpegdash",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = demux_mpegdash_run,
		.pause = NULL,
		.stop  = NULL,
	},
	DEF_AUTHOR("demuxer","mpegdash","No description","L.F","No comment"),

	.name        = "Demux Mpegdash",
	.type        = GX_DEMUXER_TYPE_MPEG_DASH,
	.check_file  = demux_mpegdash_check_file,
	.open        = demux_mpegdash_open,
	.fill_buffer = demux_mpegdash_fill_buffer,
	.close       = demux_mpegdash_close,
	.seek        = demux_mpegdash_seek,
	.control     = demux_mpegdash_control,
};
