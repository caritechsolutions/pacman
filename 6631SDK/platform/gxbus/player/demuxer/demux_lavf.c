#include "demux_lavf.h"
#include "xtr_mpeg4.h"
#include "xtr_mjpeg.h"
#include "bluray_pcm.h"
#include "adpcm.h"
#include "xiph.h"

static GX_LAVF_SUPPORT_PROTOCOL support_protocol[] =  {
	{GX_PROTOLCOL_UNKNOWN, "unknown"},
	{GX_PROTOLCOL_FILE, "file"},
	{GX_PROTOLCOL_HTTP, "http"},
	{GX_PROTOLCOL_HTTPS, "https"},
	{GX_PROTOLCOL_HLS, "hls"},
	{GX_PROTOLCOL_UDP, "udp"},
	{GX_PROTOLCOL_RTP, "rtp"},
	{GX_PROTOLCOL_RTMP, "rtmp"},
	{GX_PROTOLCOL_RTMP, "rtmpt"},
	{GX_PROTOLCOL_RTMP, "rtmps"},
	{GX_PROTOLCOL_RTSP, "rtsp"},
	{GX_PROTOLCOL_RTSP, "sdp"},
	{GX_PROTOLCOL_RTSP, "rtspt"},
	{GX_PROTOLCOL_RTSP, "rtsps"},
	{GX_PROTOLCOL_MMS, "mms"},
	{GX_PROTOLCOL_MMS, "mmst"},
	{GX_PROTOLCOL_MMS, "mmsh"},
	{GX_PROTOLCOL_RTP, "rtpsdp"},
	{GX_PROTOLCOL_CONCAT, "concat"},
};

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
	{CODEC_ID_APE, MKTAG('a', 'p', 'e', ' ')},
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

static const AVSubtitleTag mp_subtitle_tags[] = {
	{CODEC_ID_TEXT, SUB_CODEC_SRT, "srt"},
	{CODEC_ID_MOV_TEXT, SUB_CODEC_MOV_TEXT, "tx3g"},
	{CODEC_ID_SSA, SUB_CODEC_SSA, "ass"},
	{CODEC_ID_DVD_SUBTITLE, SUB_CODEC_DVD, "vobsub"},
	{CODEC_ID_XSUB, SUB_CODEC_VOB, "vobsub"},
	{CODEC_ID_DVB_SUBTITLE, SUB_CODEC_DVB_DESCRIPTOR, "DVB-Subtitle"},
	{CODEC_ID_DVB_TELETEXT, SUB_CODEC_TXT_DESCRIPTOR, "DVB-Teletext"},
	{CODEC_ID_HDMV_PGS_SUBTITLE, SUB_CODEC_PGS, "PGS-Subtitle"},
	{CODEC_ID_TTML, SUB_CODEC_TTML, "ttml"},
};

static handle_t lavf_mutex = -1;
PLAYER_SEEK_CBK lavf_seekcbk = NULL;
PLAYER_INTERRUPT_CBK lavf_interruptcbk = NULL;

#define SUB_FILTER_SIZE (64*1024 + 6)
#define VAILD_PID(pid)          ((pid>0)&&(pid<0x1fff))
#define DISABLE_DEMUX_AND_CODEC 0x1

static int demux_lavf_control(GxDemuxer*  demuxer, int cmd, void* arg);

const struct AVCodecTag* mp_wav_taglists[] = { ff_codec_wav_tags, mp_wav_tags, 0 };
const struct AVCodecTag* mp_bmp_taglists[] = { ff_codec_bmp_tags, mp_bmp_tags, 0 };

int _lavf_av_send_msg(GxDemuxer* s, int cmd, void*arg)
{
	GxMediaFilter* mf = GXMEDIAFILTER(s);
	GxMediaFilterEventPara EventPara = {0};

	if (mf == NULL)
		return 0;

	if (mf->event.func) {
		EventPara.type = cmd;
		EventPara.arg  = arg;
		mf->event.func(mf->event.priv, &EventPara);
		return 1;
	}
	return 0;
}

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
	case MKTAG('a', 'v', 'c', '2'):
	case MKTAG('a', 'v', 'c', '3'):
	case MKTAG('a', 'v', 'c', '4'):
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

static unsigned int nal_read_32bytes(uint8_t* data)
{
	int a = 0;

	a = (data[0]&0xff)<<24;
	a |= (data[1]&0xff)<<16;
	a |= (data[2]&0xff)<<8;
	a |= (data[3]&0xff);

	return a;
}

static int mp_read(void*  opaque, unsigned char *buf, int size)
{
	GxDemuxer* demuxer = opaque;
	GxStream*  stream  = demuxer->stream;
	int ret;

	ret = GxStream_Read(stream, buf, size);

	if ((ret == 0) && (GxStream_Eof(stream)))
		return -1;

	return ret;
}

static offset_t mp_seek(void*  opaque, offset_t pos, int whence)
{
	GxDemuxer* demuxer = opaque;
	GxStream* stream   = demuxer->stream;
	int64_t current_pos;

	if (whence == SEEK_CUR)
		pos += GxStream_Tell(stream);
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
		GxStream_Reset(stream);
	current_pos = GxStream_Tell(stream);
	if (GxStream_Seek(stream, pos) == GX_PLAYER_ERROR){
		GxStream_Reset(stream);
		GxStream_Seek(stream, current_pos);
		return -1;
	}

	return pos - stream->start_pos;
}

static int ogg_check_vorbis_header(GxDemuxer *demuxer, GxStreamAudioHeader* sh_audio, AVCodecContext* codec)
{
	DemuxLavfPriv* priv = demuxer->priv;
	int ogg_header_len[3];
	const uint8_t *ogg_header_start[3];
	GxStreamHeadPriv *hdr = (GxStreamHeadPriv*)sh_audio;
	OGGStreamHeader  *osh = NULL;

	priv->page_seguence = 0;
	if (hdr) {
		int i, offset;

		if (avpriv_split_xiph_headers(codec->extradata,
					codec->extradata_size, 30, ogg_header_start, ogg_header_len) < 0)
			return -1;

		osh = (OGGStreamHeader*)av_mallocz(sizeof(OGGStreamHeader) * OGGS_HEADER_NUM);

		osh[0].headerType          = 0x02;
		osh[1].headerType          = 0x00;
		osh[2].headerType          = 0x00;

		for (i = 0; i < OGGS_HEADER_NUM; i++) {
			osh[i].pageLogo[0]     = 0x4F;
			osh[i].pageLogo[1]     = 0x67;
			osh[i].pageLogo[2]     = 0x67;
			osh[i].pageLogo[3]     = 0x53;
			osh[i].version         = 0x00;
			osh[i].pageSequence[0] = priv->page_seguence++;
			osh[i].serialNumber[0] = 0x47;
			osh[i].serialNumber[1] = 0x58;
			osh[i].serialNumber[2] = 0x00;
			osh[i].serialNumber[3] = 0x00;
			osh[i].numSegments     = ogg_header_len[i]/0xFF+((ogg_header_len[i]%0xFF == 0)?0:1);

			int j = 0;
			while (j < osh[i].numSegments-1)
				osh[i].segmentTable[j++] = 0xFF;
			osh[i].segmentTable[j] = ogg_header_len[i]%0xFF;

			memset(osh[i].crcCheckSum,    0x00, 4);
			memset(osh[i].granulePostion, 0x00, 8);
			osh[i].hdrLen = OGGS_HEADER_SIZE+osh[i].numSegments;
		}

		if (hdr->header.data)
			av_free(hdr->header.data);

		hdr->header.len  = codec->extradata_size+osh[0].hdrLen+osh[1].hdrLen+osh[2].hdrLen-3;
		hdr->header.data = av_malloc(hdr->header.len);
		hdr->header.flag = 0;

		if (!hdr->header.data)
			return 0;

		for (i = 0, offset = 0; i < OGGS_HEADER_NUM; i++) {
			memcpy(hdr->header.data+offset, (void*)&osh[i], osh[i].hdrLen);
			offset += osh[i].hdrLen;
			memcpy(hdr->header.data+offset, (void*)ogg_header_start[i], ogg_header_len[i]);
			offset += ogg_header_len[i];
		}

		if (osh) {
			av_free(osh);
			osh = NULL;
		}
	}

	return 0;
}

static int ogg_check_opus_header(GxDemuxer *demuxer, GxStreamAudioHeader* sh_audio, AVCodecContext* codec)
{
	DemuxLavfPriv* priv = demuxer->priv;
	GxStreamHeadPriv *hdr = (GxStreamHeadPriv*)sh_audio;
	OGGStreamHeader  osh;

	if (hdr) {
		int i = 0;

		osh.pageLogo[0]     = 0x4F;
		osh.pageLogo[1]     = 0x67;
		osh.pageLogo[2]     = 0x67;
		osh.pageLogo[3]     = 0x53;
		osh.version         = 0x00;
		osh.headerType      = 0x02;
		memset(osh.granulePostion, 0x00, 8);
		osh.serialNumber[0] = 0x47;
		osh.serialNumber[1] = 0x58;
		osh.serialNumber[2] = 0x00;
		osh.serialNumber[3] = 0x00;
		osh.pageSequence[0] = priv->page_seguence++;
		memset(osh.crcCheckSum, 0x00, 4);
		osh.numSegments = codec->extradata_size / 0xFF + ((codec->extradata_size % 0xFF == 0)?0:1);

		while (i < osh.numSegments - 1)
			osh.segmentTable[i++] = 0xFF;
		osh.segmentTable[i] = codec->extradata_size % 0xFF;
		osh.hdrLen = OGGS_HEADER_SIZE + osh.numSegments;

		if (hdr->header.data)
			av_free(hdr->header.data);

		hdr->header.len  = codec->extradata_size + osh.hdrLen;
		hdr->header.data = av_malloc(hdr->header.len);
		hdr->header.flag = 0;

		if (!hdr->header.data)
			return 0;

		memcpy(hdr->header.data, (void*)&osh, osh.hdrLen);
		memcpy(hdr->header.data + osh.hdrLen, codec->extradata, codec->extradata_size);
	}

	return 0;
}

static int ape_check_ape_header(GxDemuxer *demuxer, GxStreamAudioHeader* sh_audio, AVCodecContext* codec)
{
	GxStreamHeadPriv *hdr = (GxStreamHeadPriv*)sh_audio;
	APEStreamHeader ash;

	if (hdr) {
		ash.tags[0] = 'M';
		ash.tags[1] = 'A';
		ash.tags[2] = 'C';
		ash.tags[3] = ' ';
		ash.header_len[0]   = ((sizeof(APEStreamHeader) - 8) >> 24) & 0xff;
		ash.header_len[1]   = ((sizeof(APEStreamHeader) - 8) >> 16) & 0xff;
		ash.header_len[2]   = ((sizeof(APEStreamHeader) - 8) >>  8) & 0xff;
		ash.header_len[3]   = ((sizeof(APEStreamHeader) - 8)      ) & 0xff;
		ash.file_version    = ((codec->extradata[1] << 8) | codec->extradata[0]);
		ash.compress_level  = ((codec->extradata[3] << 8) | codec->extradata[2]);
		ash.format_flags    = ((codec->extradata[5] << 8) | codec->extradata[4]);
		ash.bits_per_sample = codec->bits_per_sample;
		ash.channels_num    = codec->channels;
		ash.sample_rate     = codec->sample_rate;
		if (hdr->header.data)
			av_free(hdr->header.data);

		hdr->header.len  = sizeof(APEStreamHeader);
		hdr->header.data = av_malloc(hdr->header.len);
		if (!hdr->header.data)
			return 0;

		memcpy(hdr->header.data, &ash, hdr->header.len);
	}

	return 0;
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

static unsigned int video_probe_pkt_size(GxDemuxer * demuxer, AVPacket *pkt)
{
	DemuxLavfPriv* priv = demuxer->priv;
	unsigned int pkt_size = 0, nal_size = 0, ppos = 0, i = 0;

	while (ppos + priv->nal_size_size < pkt->size) {
		if (lavf_interruptcbk && lavf_interruptcbk())
			return 0;

		for (i = 0; i < priv->nal_size_size; ++i)
			nal_size = (nal_size << 8) | ( pkt->data[ppos++]& 0xff);

		pkt_size += 4;
		if (nal_size > (pkt->size - ppos))
			nal_size = pkt->size - ppos;

		pkt_size += nal_size;
		ppos += nal_size;
		nal_size = 0;
	}

	return pkt_size;
}

static int demux_lavf_check_file(GxDemuxer*  demuxer)
{
	int probe_size = 2048;
	AVProbeData avpd;
	uint8_t *buf = NULL;
	DemuxLavfPriv* priv;

	GxPlayer_SystemGet(PSYS_CBK_SEEK, &lavf_seekcbk);
	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &lavf_interruptcbk);

	if (!demuxer->priv) {
		demuxer->priv = av_mallocz(sizeof(DemuxLavfPriv));
		if (!demuxer->priv)
			return GX_DEMUXER_TYPE_UNKNOWN;
	}
	priv = demuxer->priv;
	priv->audio_index = priv->video_index = priv->sub_index = -1;

	if(demuxer->stream->file_format == GX_STREAMTYPE_DEMUXER)
		return GX_DEMUXER_TYPE_LAVF;

	while(probe_size < PROBE_BUF_SIZE) {
		if(lavf_interruptcbk && lavf_interruptcbk()){
			if(buf)
				av_free(buf);
			return GX_DEMUXER_TYPE_UNKNOWN;
		}
		buf = av_malloc(probe_size);
		if (!buf)
			return GX_DEMUXER_TYPE_UNKNOWN;
		if (GxStream_Read(demuxer->stream, buf, probe_size) != probe_size){
			av_free(buf);
			return 0;
		}
		avpd.filename = demuxer->stream->url;
		avpd.buf = buf;
		avpd.buf_size = probe_size;

		priv->iformat = av_probe_input_format(&avpd, 1);
		av_freep(&buf);
		if (priv->iformat)
			break;

		probe_size = probe_size<<1;
	}
	if(!priv->iformat)
		return GX_DEMUXER_TYPE_UNKNOWN;

	return GX_DEMUXER_TYPE_LAVF;
}

static int _lavf_add_stream(AVStream* st, GxDemuxer * demuxer, int set_index_flg, AVDictionary *opt1, AVDictionaryEntry **e, int is_iptv_play)
{
	int iRetFlg = GX_DEMUXER_CTRL_OK, is_full_detection = 0;
	DemuxLavfPriv* priv = demuxer->priv;
	AVCodecContext* codec = st->codec;
	codec->is_reset_extradata = 0;

	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
	is_full_detection = is_iptv_play?is_full_detection:0;

	switch (codec->codec_type) {
	case CODEC_TYPE_AUDIO:
		{
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->audio_disable)
				break;
			WAVEFORMATEX* wf = av_calloc(sizeof(WAVEFORMATEX) + codec->extradata_size, 1);
			GxStreamAudioHeader* sh_audio;
			const char *codec_name = NULL;
			AVDictionaryEntry *opts= NULL;
			if (priv->audio_streams >= MAX_A_STREAMS) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}
			sh_audio = GxStreamHeader_AudioNew(demuxer, priv->audio_streams, priv->astreams[priv->audio_streams]);
			if (!sh_audio) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}
			GxStreamHeadPriv* hdr = &sh_audio->priv;
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
			int g = ff_gcd(sh_audio->audio.dwScale, sh_audio->audio.dwRate);
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
			opts = av_dict_get(st->metadata, "language", NULL, 0);
			if (opts) {
				av_strlcpy(hdr->lang, opts->value, sizeof(hdr->lang)-1);
			} else {
				av_strlcpy(sh_audio->priv.lang, st->language, 4);
			}
			switch (codec->codec_id) {
			case CODEC_ID_PCM_S8:
			case CODEC_ID_PCM_U8:
				sh_audio->samplesize = 8;
				sh_audio->big_endian = 0;
				sh_audio->format = AUDIO_CODEC_PCM;
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
			case CODEC_ID_PCM_F32LE:
				sh_audio->samplesize = 32;
				sh_audio->big_endian = 0;
				sh_audio->format = AUDIO_CODEC_PCM;
				break;
			case CODEC_ID_PCM_S24LE:
			case CODEC_ID_PCM_U24LE:
				sh_audio->samplesize = 24;
				sh_audio->big_endian = 0;
				sh_audio->format = AUDIO_CODEC_PCM;
				break;
			case CODEC_ID_PCM_S24BE:
			case CODEC_ID_PCM_U24BE:
				sh_audio->samplesize = 24;
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
			case CODEC_ID_VORBIS:
				ogg_check_vorbis_header(demuxer, sh_audio, codec);
				sh_audio->format = AUDIO_CODEC_VORBIS;
				break;
			case CODEC_ID_OPUS:
				ogg_check_opus_header(demuxer, sh_audio, codec);
				sh_audio->format = AUDIO_CODEC_OPUS;
				break;
			case CODEC_ID_APE:
				ape_check_ape_header(demuxer, sh_audio, codec);
				sh_audio->format = AUDIO_CODEC_APE;
				break;
			case CODEC_ID_SBC:
				sh_audio->format = AUDIO_CODEC_SBC;
				break;
			default:
				break;
			}

			/* select a audio track, which is the first one we can support */
			if (set_index_flg || demuxer->audio->id == -1 || demuxer->audio->sh == NULL) {
				demuxer->audio->id = priv->audio_streams-1;
				demuxer->audio->sh = sh_audio;
				priv->audio_index = priv->astreams[priv->audio_streams-1];
			} else {
				st->discard = AVDISCARD_ALL;
			}
			break;
		}
	case CODEC_TYPE_VIDEO:
		{
			GxStreamVideoHeader* sh_video, *tmp_sh_video;
			BITMAPINFOHEADER* bih;
			const char *codec_name = NULL;

			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->video_disable)
				break;
			if (priv->video_streams >= MAX_V_STREAMS) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}

			sh_video = GxStreamHeader_VideoNew(demuxer, priv->video_streams, priv->vstreams[priv->video_streams]);
			if (!sh_video) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}
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
				else {
					sh_video->fps = (st->r_frame_rate.den)?av_q2d(st->r_frame_rate):0;
				}
			}
			else
				sh_video->fps = codec->frame_rate;

			sh_video->frametime = 1 / av_q2d(st->r_frame_rate);
			sh_video->format = bih->biCompression;
			gxlogd("video: %.4s[0x%x]\n", (char*)&(sh_video->format), sh_video->format);
			fourcc_video_format(sh_video, codec->codec_id);
			sh_video->aspect = (float)codec->width/codec->height;
			sh_video->i_bps = codec->bit_rate / 8;
			if (is_full_detection) {
				while ((*e = av_dict_get(opt1, "", *e, AV_DICT_IGNORE_SUFFIX))) {
					if (!av_strcasecmp("vid_variant_bitrate", (*e)->key)) { /*current video index.*/
						sh_video->i_bps = (int)(strtoll((*e)->value, NULL, 10) / 8);
						break;
					}
				}
			}
			sh_video->ds = demuxer->video;

			int i = 0;
			for (i = 0; i < st->nb_side_data; i++) {
				if (st->side_data && (st->side_data[i].type == AV_PKT_DATA_DOVI_CONF)) {
					sh_video->dolby_flag = 1;
				}
			}

			priv->nal_size_size = video_probe_nal_size(codec, sh_video);
			if (priv->nal_size_size == -1) {
				priv->nal_size_size = 0;
				break;
			}
			priv->extradata_size = codec->extradata_size;

			tmp_sh_video = (GxStreamVideoHeader*)demuxer->video->sh;
			if (demuxer->video->id == -1
				|| demuxer->video->sh == NULL
				||(is_full_detection && (sh_video->i_bps > tmp_sh_video->i_bps))) {
				AVStream* tmp_st = (AVStream* )(demuxer->video->st);
				if (tmp_st) {
					tmp_st->discard = AVDISCARD_ALL;
				}
				demuxer->video->id = priv->video_streams-1;
				demuxer->video->sh = sh_video;
				demuxer->video->st = st;
				priv->video_index = priv->vstreams[priv->video_streams-1];
			} else {
				st->discard = AVDISCARD_ALL;
			}
			break;
		}
	case CODEC_TYPE_SUBTITLE:
		{
			GxStreamSubHeader* sh_sub;
			AVDictionaryEntry *opts = NULL;
			int index = 0;
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->subtitle_disable)
				break;
			if (priv->sub_streams >= MAX_S_STREAMS) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}

			sh_sub = GxStreamHeader_SubNew(demuxer, priv->sub_streams, priv->sstreams[priv->sub_streams]);
			if (!sh_sub) {
				iRetFlg = GX_DEMUXER_CTRL_ERROR;
				break;
			}
			/* only support text subtitles for now*/
			for (index = 0; index < sizeof(mp_subtitle_tags)/sizeof(mp_subtitle_tags[0]); index++) {
				if (codec->codec_id == mp_subtitle_tags[index].id) {
					sh_sub->type = mp_subtitle_tags[index].type;
					snprintf (sh_sub->priv.codec, sizeof(sh_sub->priv.codec)-1, "%s", mp_subtitle_tags[index].subtitle_codec);
					break;
				}
			}

			if (index >= sizeof(mp_subtitle_tags)/sizeof(mp_subtitle_tags[0])) {
				break;
			}

			if(codec->extradata && codec->extradata_size>0){
				if(sh_sub->extradata)
					av_free(sh_sub->extradata);
				sh_sub->extradata = av_calloc(codec->extradata_size, 1);
				memcpy(sh_sub->extradata, codec->extradata, codec->extradata_size);
				sh_sub->extradata_len = codec->extradata_size;
			}
			priv->sub_streams++;
			if (!strcmp(sh_sub->priv.codec, "DVB-Subtitle")) {
				sh_sub->priv.sub_stream[sh_sub->priv.sub_num].major = st->codec->codec_tag >>16;
				sh_sub->priv.sub_stream[sh_sub->priv.sub_num].minor = st->codec->codec_tag & 0xffff;
				sh_sub->priv.sub_num += 1;
			} else if (!strcmp(sh_sub->priv.codec, "DVB-Teletext")) {
				sh_sub->priv.sub_stream[sh_sub->priv.sub_num].minor = st->codec->codec_tag >>8;
				sh_sub->priv.sub_stream[sh_sub->priv.sub_num].major = st->codec->codec_tag & 0xff;
				sh_sub->priv.sub_num += 1;
			}
			opts = av_dict_get(st->metadata, "language", NULL, 0);
			if (opts) {
				av_strlcpy(sh_sub->priv.lang, opts->value, sizeof(sh_sub->priv.lang)-1);
			} else {
				av_strlcpy(sh_sub->priv.lang, st->language, sizeof(sh_sub->priv.lang)-1);
			}
			av_strlcpy(sh_sub->priv.name, st->name, sizeof(sh_sub->priv.lang)-1);
			if (demuxer->sub->id == -1 || demuxer->sub->sh == NULL) {
				demuxer->sub->id = priv->sub_streams-1;
				demuxer->sub->sh = sh_sub;
				priv->sub_index = priv->sstreams[priv->sub_streams-1];
			} else if (is_full_detection) {
				st->discard = AVDISCARD_ALL;
			}
			priv->sub_fill = 0;
			break;
		}
	default:
		iRetFlg = GX_DEMUXER_CTRL_ERROR;
		break;
	}
	return iRetFlg;
}

static GX_SUPPORT_PROTOCOL _lavf_get_input_uri_protocol_type(char *uri)
{
	char acProHead[PLAYER_URL_LONG] = "";
	int iIndex = 0;
	memset (acProHead, 0, sizeof(acProHead)/sizeof(char));
	GX_SUPPORT_PROTOCOL emProtocol = GX_PROTOLCOL_UNKNOWN;

	do {
		if (NULL == uri) {
			gxlogd ("url NULL, fail\n");
			break;
		}
		char *pcTemp = strchr(uri, ':');
		snprintf (acProHead, ((pcTemp- uri+1)>(sizeof(acProHead)/sizeof(char)-1))?(sizeof(acProHead)/sizeof(char)-1):(pcTemp- uri+1), "%s", uri);
		for (iIndex = 0; iIndex < sizeof(support_protocol)/sizeof(support_protocol[0]); iIndex++) {
			if (0 == strncmp(support_protocol[iIndex].acProtocol, acProHead, strlen(support_protocol[iIndex].acProtocol))) {
				if (strstr(uri, ".m3u8"))
					emProtocol = GX_PROTOLCOL_HLS;
				else
					emProtocol = support_protocol[iIndex].iProtocol;
			}
		}
	}while(0);

	if (GX_PROTOLCOL_UNKNOWN == emProtocol)
		gxlogd ("demux input uri is not support protocol.\n");
	return emProtocol;

}

static GxDemuxer* _lavf_open_demux(GxDemuxer * demuxer, AVDictionary **opts)
{
	int i = 0, ret = 0;
	char *mp_filename= NULL, *mp="mp:";
	DemuxLavfPriv*   priv = demuxer->priv;
	AVFormatContext* avfc = priv->avfc;
	AVDictionaryEntry *e = NULL;

	GxStream_Seek(demuxer->stream, 0);
	avfc->pb = avio_alloc_context(priv->buffer, BIO_BUFFER_SIZE, 0, demuxer, mp_read, NULL, mp_seek);
	avfc->flags = URL_RDONLY;
	avfc->file_format  = demuxer->stream->file_format;
	avfc->nobuffer = 0;
	if(!(mp_filename = av_mallocz(PLAYER_URL_LONG+strlen(mp)+1)))
		return NULL;
	av_strlcatf(mp_filename, PLAYER_URL_LONG+strlen(mp), "%s%s", mp, demuxer->stream->url);

	if ((ret = avformat_open_input(&avfc, (const char*)mp_filename, priv->iformat, opts)) < 0) {
		av_free(mp_filename);
		priv->avfc = avfc;
		gxloge_raw_l ("demux_lavf.c open input fail.ret=%d.err:[%s].\n", ret, av_err2str(ret));
		return NULL;
	}
	av_free(mp_filename);

	avfc->seek_by_time = ((demuxer->stream->flags & GX_STREAM_SEEK_TIME) == GX_STREAM_SEEK_TIME) ? 1 : 0;
	demuxer->seekable  = (avfc->seek_by_time || avfc->iformat->read_seek) ? GX_STREAM_SEEK : 0;
	if (avfc->flags & AVFMT_FLAG_PTS_REORDER)
		demuxer->pts_reorder = 1;

	if ((ret = avformat_find_stream_info(avfc, opts)) < 0) {
		gxloge_raw_l ("demux_lavf.c find stream info fail.ret=%d(%s)\n", ret, av_err2str(ret));
		return NULL;
	}

	//_lavf_avfc_printf(avfc);
	if (avfc->iformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->iformat->name);
	if (avfc->oformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->oformat->name);

	for (i = 0; i < avfc->nb_streams; i++) {
		AVStream* st = avfc->streams[i];
		st->discard = AVDISCARD_DEFAULT;
		switch (st->codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->audio_disable) {
				st->discard = AVDISCARD_ALL;
				break;
			}
			priv->astreams[priv->audio_streams] = i;
			break;
		case CODEC_TYPE_VIDEO:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->video_disable) {
				st->discard = AVDISCARD_ALL;
				break;
			}
			priv->vstreams[priv->video_streams] = i;
			break;
		case CODEC_TYPE_SUBTITLE:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->subtitle_disable) {
				st->discard = AVDISCARD_ALL;
				break;
			}
			priv->sstreams[priv->sub_streams] = i;
			break;
		default:
			st->discard = AVDISCARD_ALL;
			continue;
		}
		_lavf_add_stream(st, demuxer, 0, *opts, &e, 0);
	}

	if (!priv->audio_streams)
		demuxer->audio->id = -2;	// nosound
	if (!priv->video_streams) {
		if (!priv->audio_streams) {
			return NULL;
		}
		demuxer->video->id = -2;	// audio-only
	}

	demuxer->start_pts = avfc->start_time/1000;
	//check mpeg4 file
	xtr_mpeg4_check(demuxer);
	if (demuxer->info.audio_pid > 0) {
		GxDemuxerStreamSwitch StreamSwitch;
		StreamSwitch.pid = demuxer->info.audio_pid;
		demux_lavf_control(demuxer, GX_DEMUXER_CTRL_SWITCH_AUDIO, &StreamSwitch);
	}

	if (demuxer->info.sub_pid > 0) {
		GxDemuxerStreamSwitch StreamSwitch;
		StreamSwitch.pid = demuxer->info.sub_pid;
		demux_lavf_control(demuxer, GX_DEMUXER_CTRL_SWITCH_SUB, &StreamSwitch);
	}

	return demuxer;
}

static int lavf_str2vcodec(char *vcodec_type)
{
	int vcodec = -1;
	if (vcodec_type) {
		if (av_stristr(vcodec_type, "avc1.")) {
			vcodec = PLAYER_VCODEC_H264;
		} else if (av_stristr(vcodec_type, "hvc1.") || av_stristr(vcodec_type, "hev1.")) {
			vcodec = PLAYER_VCODEC_HEVC;
		}
	}
	return vcodec;
}

#define AV_MALLOCZ_FUNC(a, size)\
{\
    if (!a) {\
        a = av_mallocz(size);\
        if (!a) {\
            ret = AVERROR_NOMEM;\
            goto fail;\
        }\
    }\
};

static int lavf_info_to_gxplayer_media(GxDemuxer *demuxer, AVDictionary **opts)
{
	AVDictionaryEntry *e = NULL;
	int vid_idx = -1, aud_idx = -1, sub_idx = -1, i = 0, ret = GX_DEMUXER_CTRL_OK, cur_periods_idx = 0, idx = 0;
	lavf_cont_stream_clips *cont_stream_clips = NULL;
	LavfMulitStreamInformation *periods_stream_info = NULL;

	if (!(cont_stream_clips = av_mallocz(sizeof(lavf_cont_stream_clips)))) {
		ret = AVERROR_NOMEM;
		goto fail;
	}
	cont_stream_clips->n_periods = 1;
	cont_stream_clips->cur_periods = -1;
	/*TODO: get ffmpeg infomation*/
	while ((e = av_dict_get(*opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
		if (!av_strcasecmp("dash_n_periods", e->key)) { /*current video index.*/
			cont_stream_clips->n_periods = atoi(e->value);
			AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods);
		} else if (!av_strcasecmp("dash_cur_periods", e->key)) { /*current video index.*/
			cont_stream_clips->cur_periods = atoi(e->value);
		} else if (!av_strcasecmp("cur_periods_idx", e->key)) { /*current video index.*/
			cur_periods_idx = atoi(e->value);
			if (cur_periods_idx >= 0) {
				if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
				} else {
					AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
					AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
				}
				vid_idx = aud_idx = sub_idx = -1;
			}
		} else if (!av_strcasecmp("vid_variant_bitrate_max", e->key)) { /*current video index.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->vid_max = atoi(e->value);
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->vid_max > 0) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track = av_mallocz(sizeof(LavfVideoTrackInfo)*cont_stream_clips->periods_stream_info[cur_periods_idx]->vid_max);
				if (!cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
					gxlogi ("malloc fail.\n");
					ret= AVERROR_NOMEM;
					goto fail;
				}
			}
		} else if (!av_strcasecmp("vid_variant_bitrate", e->key)) { /*video bitrate.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_variant_bitrate = atoi(e->value);
			}
		} else if (!av_strcasecmp("vid_id", e->key)) { /*video id.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				vid_idx += 1;
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_id = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("vid_codecs", e->key)) { /*video codecs.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_codecs = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("vid_width", e->key)) { /*video width.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_width = atoi(e->value);
			}
		} else if (!av_strcasecmp("vid_height", e->key)) { /*video height.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_height = atoi(e->value);
			}
		} else if (!av_strcasecmp("vid_frameRate", e->key)) { /*video sar.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->video_track[vid_idx].vid_frameRate = atoi(e->value);
			}
		} else if (!av_strcasecmp("vid_stream_index", e->key)) { /*current video index.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->cur_vid_idx = atoi(e->value);
		} else if (!av_strcasecmp("audio_prog_max", e->key)) { /*audio stream number.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_max = atoi(e->value);
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_max > 0) {
				if (NULL == (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track = av_mallocz(sizeof(LavfAudioTrackInfo)*cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_max))) {
					gxlogi ("malloc fail.\n");
					ret= AVERROR_NOMEM;
					goto fail;
				}
			}
		} else if (!av_strcasecmp("a_variant_bitrate", e->key)) { /*audio variant bitrate.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track[aud_idx].aud_variant_bitrate = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("aud_id", e->key)) { /*audio id*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track) {
				aud_idx += 1;
				cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track[aud_idx].aud_id = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("aud_language", e->key)) { /*audio language*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track[aud_idx].aud_language = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("aud_codecs", e->key)) { /*audio codecs*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track[aud_idx].aud_codecs = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("aud_samplerate", e->key)) { /*audio codecs*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->audio_track[aud_idx].aud_samplerate = atoi(e->value);
			}
		} else if (!av_strcasecmp("aud_stream_index", e->key)) { /*current audio stream index*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->cur_aud_idx = atoi(e->value);
		} else if (!av_strcasecmp("sub_prog_max", e->key)) { /*subtiles stream number.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->subtiles_max = atoi(e->value);
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->subtiles_max > 0) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track = av_mallocz(sizeof(LavfSubtitlesTrackInfo)*cont_stream_clips->periods_stream_info[cur_periods_idx]->subtiles_max);
				if (!cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track) {
					gxlogi ("malloc fail.\n");
					ret = AVERROR_NOMEM;
					goto fail;
				}
			}
		} else if (!av_strcasecmp("sub_id", e->key)) { /*subtiles id*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track) {
				sub_idx += 1;
				cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track[sub_idx].sub_id = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("sub_language", e->key)) { /*subtiles language*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track[sub_idx].sub_language = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("sub_codecs", e->key)) { /*subtiles codecs.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			if (cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track) {
				cont_stream_clips->periods_stream_info[cur_periods_idx]->subtitles_track[sub_idx].sub_codecs = av_strdup(e->value);
			}
		} else if (!av_strcasecmp("sub_stream_index", e->key)) { /*current subtiles stream index.*/
			if (cont_stream_clips->periods_stream_info) {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			} else {
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info, (sizeof(LavfMulitStreamInformation)*cont_stream_clips->n_periods));
				AV_MALLOCZ_FUNC(cont_stream_clips->periods_stream_info[cur_periods_idx], sizeof(LavfMulitStreamInformation));
			}
			cont_stream_clips->periods_stream_info[cur_periods_idx]->cur_sub_idx = atoi(e->value);
		}
	}
	cont_stream_clips->n_periods = FFMAX(cont_stream_clips->n_periods, 1);
	//TODO: lavf information to gxplayer_get_proginfo function.
	/*video infor to gxplayer_get_proginfo*/
	for (idx = 0; idx < cont_stream_clips->n_periods; idx++) {
		if (idx != cont_stream_clips->cur_periods)
			continue;
		if (!cont_stream_clips->periods_stream_info)
			continue;
		periods_stream_info  = cont_stream_clips->periods_stream_info[idx];
		if (!periods_stream_info)
			continue;
		if (vid_idx > 0) {
			demuxer->stream->prog_now = periods_stream_info->cur_vid_idx;
			vid_idx = AV_MIN(periods_stream_info->vid_max, GXPLAYER_MAX_BAND_WIDTH);
			demuxer->stream->prog_max = vid_idx;
			for (i = 0; i < vid_idx; i++) {
				demuxer->stream->prog[i].bandwidth = periods_stream_info->video_track[i].vid_variant_bitrate;
				demuxer->stream->prog[i].vcodec	  = lavf_str2vcodec(periods_stream_info->video_track[i].vid_codecs);
				demuxer->stream->prog[i].width = periods_stream_info->video_track[i].vid_width;
				demuxer->stream->prog[i].height = periods_stream_info->video_track[i].vid_height;
				demuxer->stream->prog[i].frameRate = periods_stream_info->video_track[i].vid_frameRate;
			}
		}
		/*audio infor to gxplayer_get_proginfo*/
		if (aud_idx > 0) {
			GxStreamAudioHeader* sh_audio;
			if (periods_stream_info->audio_max >= MAX_A_STREAMS) {
				gxlogi_raw_l ("Exceeding the limit.cur_num:%d.support_max:%d.\n", periods_stream_info->audio_max, MAX_A_STREAMS);
			}
			aud_idx = AV_MIN(periods_stream_info->audio_max, MAX_A_STREAMS);
			for (i = 0; i < aud_idx; i++) {
				sh_audio = GxStreamHeader_AudioNew(demuxer, i, i);
				if (!sh_audio) {
					ret = GX_DEMUXER_CTRL_ERROR;
					break;
				}
				GxStreamHeadPriv* hdr = &sh_audio->priv;
				if (periods_stream_info->audio_track[i].aud_language)
					av_strlcpy(hdr->lang, periods_stream_info->audio_track[i].aud_language, sizeof(hdr->lang)-1);
				if (periods_stream_info->audio_track[i].aud_codecs)
					av_strlcpy(hdr->codec, periods_stream_info->audio_track[i].aud_codecs, sizeof(hdr->codec)-1);
				if (periods_stream_info->audio_track[i].aud_language)
					av_strlcpy(hdr->name, periods_stream_info->audio_track[i].aud_language, sizeof(hdr->name)-1);
				if (periods_stream_info->audio_track[aud_idx].aud_samplerate > 0)
					sh_audio->samplerate = periods_stream_info->audio_track[aud_idx].aud_samplerate;
				hdr->id = i;
			}
		}
		/*subtiles infor to gxplayer_get_proginfo*/
		if (sub_idx > 0) {
			GxStreamSubHeader* sh_sub;
			if (periods_stream_info->subtiles_max >= MAX_S_STREAMS) {
				gxlogi_raw_l ("Exceeding the limit.cur_num:%d.support_max:%d.\n", periods_stream_info->subtiles_max, MAX_S_STREAMS);
			}
			sub_idx = AV_MIN(periods_stream_info->subtiles_max, MAX_S_STREAMS);
			for (i = 0; i < sub_idx; i++) {
				sh_sub = GxStreamHeader_SubNew(demuxer, i, i);
				if (!sh_sub) {
					ret = GX_DEMUXER_CTRL_ERROR;
					break;
				}
				GxStreamHeadPriv* hdr = &sh_sub->priv;
				if (periods_stream_info->subtitles_track[i].sub_id)
					av_strlcpy(hdr->lang, periods_stream_info->subtitles_track[i].sub_id, sizeof(hdr->lang)-1);
				if (periods_stream_info->subtitles_track[i].sub_codecs)
					av_strlcpy(hdr->codec, periods_stream_info->subtitles_track[i].sub_codecs, sizeof(hdr->codec)-1);
				if (periods_stream_info->subtitles_track[i].sub_language)
					av_strlcpy(hdr->name, periods_stream_info->subtitles_track[i].sub_language, sizeof(hdr->name)-1);
			}
		}
	}
fail:
	/*free memory.*/
	for (idx = 0; idx < cont_stream_clips->n_periods; idx++) {
		if (!cont_stream_clips->periods_stream_info)
			continue;
		periods_stream_info  = cont_stream_clips->periods_stream_info[idx];
		if (!periods_stream_info)
			continue;
		if (periods_stream_info->video_track) {
			for (i = 0; i < periods_stream_info->vid_max; i++) {
				if (periods_stream_info->video_track[i].vid_id) {
					av_free(periods_stream_info->video_track[i].vid_id);
					periods_stream_info->video_track[i].vid_id = NULL;
				}
				if (periods_stream_info->video_track[i].vid_codecs) {
					av_free(periods_stream_info->video_track[i].vid_codecs);
					periods_stream_info->video_track[i].vid_codecs = NULL;
				}
				if (periods_stream_info->video_track[i].vid_sar) {
					av_free(periods_stream_info->video_track[i].vid_sar);
					periods_stream_info->video_track[i].vid_sar = NULL;
				}
			}
			av_free(periods_stream_info->video_track);
			periods_stream_info->video_track = NULL;
		}
		if (periods_stream_info->audio_track) {
			for (i = 0; i < periods_stream_info->audio_max; i++) {
				if (periods_stream_info->audio_track[i].aud_variant_bitrate) {
					av_free(periods_stream_info->audio_track[i].aud_variant_bitrate);
					periods_stream_info->audio_track[i].aud_variant_bitrate = NULL;
				}
				if (periods_stream_info->audio_track[i].aud_id) {
					av_free(periods_stream_info->audio_track[i].aud_id);
					periods_stream_info->audio_track[i].aud_id = NULL;
				}
				if (periods_stream_info->audio_track[i].aud_language) {
					av_free(periods_stream_info->audio_track[i].aud_language);
					periods_stream_info->audio_track[i].aud_language = NULL;
				}
				if (periods_stream_info->audio_track[i].aud_codecs) {
					av_free(periods_stream_info->audio_track[i].aud_codecs);
					periods_stream_info->audio_track[i].aud_codecs = NULL;
				}
			}
			av_free(periods_stream_info->audio_track);
			periods_stream_info->audio_track = NULL;
		}
		if (periods_stream_info->subtitles_track) {
			for (i = 0; i < periods_stream_info->subtiles_max; i++) {
				if (periods_stream_info->subtitles_track[i].sub_id) {
					av_free(periods_stream_info->subtitles_track[i].sub_id);
					periods_stream_info->subtitles_track[i].sub_id = NULL;
				}
				if (periods_stream_info->subtitles_track[i].sub_language) {
					av_free(periods_stream_info->subtitles_track[i].sub_language);
					periods_stream_info->subtitles_track[i].sub_language = NULL;
				}
				if (periods_stream_info->subtitles_track[i].sub_codecs) {
					av_free(periods_stream_info->subtitles_track[i].sub_codecs);
					periods_stream_info->subtitles_track[i].sub_codecs = NULL;
				}
			}
			av_free(periods_stream_info->subtitles_track);
			periods_stream_info->subtitles_track= NULL;
		}
		if (periods_stream_info) {
			av_free(periods_stream_info);
			periods_stream_info = NULL;
		}
	}
	if (cont_stream_clips->periods_stream_info) {
		av_free(cont_stream_clips->periods_stream_info);
		cont_stream_clips->periods_stream_info = NULL;
	}
	if (cont_stream_clips) {
		av_free(cont_stream_clips);
		cont_stream_clips = NULL;
	}
	return ret;
}

extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);
static GxDemuxer* _lavf_open_stream_and_demux(GxDemuxer * demuxer, AVDictionary **opts)
{
	int i = 0, ret = 0, set_index_flg = 0, idef_audio_index = -1, findAudioFlg = 0, findVideoFlg = 0, is_full_detection = 0;
	DemuxLavfPriv*   priv = demuxer->priv;
	AVFormatContext* avfc = priv->avfc;
	AVDictionaryEntry *e = NULL;
	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

	demuxer->audio_total = -1;
	avfc->flags = AVIO_FLAG_READ;
	avfc->nobuffer = 0;
	if (1 == demuxer->stream->nobuffer) {
		avfc->nobuffer = AVFMT_FLAG_NOBUFFER;
		av_dict_set_int(opts, "nobuffer", avfc->nobuffer, 0);
	}
	if (*opts && (e = av_dict_get(*opts, "net_stream_live_mode", NULL, 0))) {
		avfc->nobuffer = (atoi(e->value)&0x1)?AVFMT_FLAG_QUICK_START:AVFMT_FLAG_NOBUFFER;
		demuxer->stream->nobuffer = 1;
	}
	if (demuxer->stream->prog_now >= 0) {
		av_dict_set_int(opts, "mulit_bandwidth_prog_now", demuxer->stream->prog_now, 0);/*set hls.c hls mulit bandwidth program index.*/
	}
	priv->protocol_type  = _lavf_get_input_uri_protocol_type(demuxer->stream->url);
	avfc->file_format  = GX_STREAMTYPE_STREAM;
	if ((ret = avformat_open_input(&avfc, demuxer->stream->url, priv->iformat, opts)) < 0) {
		priv->avfc = avfc;
		gxloge_raw_l ("demux_lavf.c open input fail.ret=%d.err:[%s].\n", ret, av_err2str(ret));
		if (AVERROR_INVALIDDATA == ret) //Invalid data.(not found demux).
			demuxer->stream->err.open_err = PLAYER_ERROR_DEMUX_ERROR;
		else if (AVERROR_PROTOCOL_NOT_FOUND == ret) //protocol not found.
			demuxer->stream->err.open_err = PLAYER_ERROR_URL_UNSUPPORT;
		else //other.is return no data source.
			demuxer->stream->err.open_err = PLAYER_ERROR_NO_DATA_SOURCE;
		return NULL;
	}

	if ((e = av_dict_get(*opts, "ffprobesize", NULL, 0))) {
		/*default value:32KB;for out of memory,need to set up probesize size.*/
		avfc->probesize = (atoi(e->value)*1024 > 32*1024)?(atoi(e->value)*1024):avfc->probesize;
	}

	if ((e = av_dict_get(*opts, "analyzeduration", NULL, 0))) {
		/* max_analyze_duration default value:5*AV_TIME_BASE */
		avfc->max_analyze_duration = ((atoi(e->value)*AV_TIME_BASE)> 5*AV_TIME_BASE)?(atoi(e->value)*AV_TIME_BASE):(5*AV_TIME_BASE);
	}

	avfc->seek_by_time = ((demuxer->stream->flags & GX_STREAM_SEEK_TIME) == GX_STREAM_SEEK_TIME)?1:0;

	if(avfc->flags & AVFMT_FLAG_PTS_REORDER)
		demuxer->pts_reorder = 1;

	if ((ret = avformat_find_stream_info(avfc, opts)) < 0) {
		gxloge_raw_l ("demux_lavf.c find stream info fail.ret=%d(%s)\n", ret, av_err2str(ret));
		if (AVERROR_INVALIDDATA == ret)
			demuxer->stream->err.open_err = PLAYER_ERROR_NO_DEMUX_TOOLS;
		else
			demuxer->stream->err.open_err = PLAYER_ERROR_NO_DATA_SOURCE;
		return NULL;
	}

	if (avfc->iformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->iformat->name);
	if (avfc->oformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->oformat->name);

	demuxer->seekable = (avfc->duration > 0) ? GX_STREAM_SEEK : 0;
	av_dump_format(avfc, 0, demuxer->stream->url, 0);
	lavf_info_to_gxplayer_media(demuxer, opts);

	for (i = 0; i < avfc->nb_streams; i++) {
		AVStream* st = avfc->streams[i];
		if (!is_full_detection)
			st->discard = AVDISCARD_ALL;
		switch (st->codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			set_index_flg = 0;
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->audio_disable) {
				st->discard = AVDISCARD_ALL;
			} else if (!is_full_detection && (!findAudioFlg) &&
					((idef_audio_index == i) || (-1 == idef_audio_index))) {
				set_index_flg = 1;
				st->discard = AVDISCARD_DEFAULT;
				findAudioFlg = 1;
			}
			priv->astreams[priv->audio_streams] = i;
			break;
		case CODEC_TYPE_VIDEO:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->video_disable) {
				st->discard = AVDISCARD_ALL;
			} else if (!is_full_detection && !findVideoFlg) {
				st->discard = AVDISCARD_DEFAULT;
				findVideoFlg = 1;
			}
			priv->vstreams[priv->video_streams] = i;
			break;
		case CODEC_TYPE_SUBTITLE:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->subtitle_disable) {
				st->discard = AVDISCARD_ALL;
			} else if (!is_full_detection) {
				st->discard = AVDISCARD_DEFAULT;
			}
			priv->sstreams[priv->sub_streams] = i;
			break;
		default:
			break;
		}
		_lavf_add_stream(st, demuxer, set_index_flg, *opts, &e, 1);
	}

	if (!priv->audio_streams)
		demuxer->audio->id = -2;	// nosound
	if (!priv->video_streams) {
		if (!priv->audio_streams) {
			return NULL;
		}
		demuxer->video->id = -2;	// audio-only
	}
	/*check hls is av separate stream.*/
	demuxer->stream->is_hls_av_separate = (avfc->is_hls_av_separate)?1:0;

	demuxer->start_pts = avfc->start_time/1000;
	//check mpeg4 file
	xtr_mpeg4_check(demuxer);
	if (demuxer->info.audio_pid > 0) {
		GxDemuxerStreamSwitch StreamSwitch;
		StreamSwitch.pid = demuxer->info.audio_pid;
		demux_lavf_control(demuxer, GX_DEMUXER_CTRL_SWITCH_AUDIO, &StreamSwitch);
	}

	if (demuxer->info.sub_pid > 0) {
		GxDemuxerStreamSwitch StreamSwitch;
		StreamSwitch.pid = demuxer->info.sub_pid;
		demux_lavf_control(demuxer, GX_DEMUXER_CTRL_SWITCH_SUB, &StreamSwitch);
	}
	/*delay 1S.free 1S packet data.*/
	if (priv->avfc && priv->avfc->iformat && !strcasecmp(priv->avfc->iformat->name, "rtp") && demuxer->stream->nobuffer) {
		AVPacket pkt;
		unsigned int cur_time = av_get_tick(NULL, 0);
		do {
			if ((ret = av_read_frame(priv->avfc, &pkt)) < 0) {
				return NULL;
			}
			av_free_packet(&pkt);
		} while ((av_get_tick(NULL, 0) - cur_time) <= 1*1000);
	}
	return demuxer;
}

static void demux_lavf_get_url_options_value(GxDemuxer * demuxer, AVFormatContext* avfc, AVDictionary **opts)
{
	char* options_value = NULL;
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "decryption_key:"))) {
		av_dict_set(opts, "decryption_key", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "transport_type:"))) {
		av_dict_set(opts, "rtsp_transport", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "fifo_size:"))) {
		av_dict_set(opts, "fifo_size", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "User-Agent:"))) {
		av_dict_set(opts, "user_agent", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "disable_accept:"))) {
		av_dict_set(opts, "disable_accept", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Connection:"))) {
		av_dict_set(opts, "Connection", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Referer:"))) {
		av_dict_set(opts, "referer", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "timeout:"))) {
		av_dict_set(opts, "rw_timeout", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "no_cache_flag:"))) {
		demuxer->stream->nobuffer = atoi(options_value);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "screen_to_dmx_pts_lms:"))) {
		demuxer->stream->screen_to_dmx_pts_lms = atoi(options_value);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "screen_to_dmx_pts_hms:"))) {
		demuxer->stream->screen_to_dmx_pts_hms = atoi(options_value);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "ffprobesize:"))) {
		av_dict_set(opts, "ffprobesize", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "xClientGUID:"))) {
		av_dict_set(opts, "xClientGUID", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "sdp_media_info:"))) {
		av_dict_set(opts, "sdp_media_info", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Set-Cookie:"))) {
		av_dict_set(opts, "ffcookies", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "ffnonblock_flag:"))) {
		if (atoi(options_value)) {
			char nonblock[8] = {0};
			av_strlcatf(nonblock, sizeof(nonblock), "%d", AVIO_FLAG_NONBLOCK);
			av_dict_set(opts, "avioflags", nonblock, 0);
		}
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "net_stream_live_mode:"))) {
		av_dict_set(opts, "net_stream_live_mode", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "ffanalyzeduration:"))) {
		av_dict_set(opts, "analyzeduration", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "live_start_index:"))) {
		av_dict_set(opts, "live_start_index", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "http_seekable:"))) {
		av_dict_set(opts, "http_seekable", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "av_freerun:"))) {
		if (1 == atoi(options_value)) {
			extern int av_dict_set_int(AVDictionary **pm, const char *key, int64_t value, int flags);
			int vdec_sync = 0, aout_sync = 0;
			GxPlayer_SystemGet(PSYS_VDEC_SYNC_FLAG, &vdec_sync);/*get current video PSYS_VDEC_SYNC_FLAG flag*/
			GxPlayer_SystemGet(PSYS_AOUT_SYNC_FLAG, &aout_sync);/*get current video PSYS_AOUT_SYNC_FLAG flag*/
			av_dict_set_int(opts, "vdec_sync", (int64_t)vdec_sync, 0);/*record vdec_sync flag */
			av_dict_set_int(opts, "aout_sync", (int64_t)aout_sync, 0);/*record aout_sync flag */
			vdec_sync = aout_sync = 0;
			GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &vdec_sync); /*set free run*/
			GxPlayer_SystemSet(PSYS_AOUT_SYNC_FLAG, &aout_sync); /*set free run*/
		}
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "vn_streams:"))) {
		av_dict_set(opts, "ff_vn_streams", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "an_streams:"))) {
		av_dict_set(opts, "ff_an_streams", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "rtmp_enhanced_codecs:"))) {
		av_dict_set(opts, "rtmp_enhanced_codecs", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "ssl_cipher_list:"))) {
		av_dict_set(opts, "ssl_cipher_list", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Authorization:"))) {
		av_dict_set(opts, "Authorization", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "CustomHeaders:"))) {
		av_dict_set(opts, "CustomHeaders", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "socks5_proxy:"))) {
		av_dict_set(opts, "socks5_proxy", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "http_proxy:"))) {
		av_dict_set(opts, "http_proxy", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "rtp_queue_size:"))) {
		av_dict_set(opts, "rtp_queue_size", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "rist_secret:"))) {
		av_dict_set(opts, "rist_secret", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "rist_encryption_type:"))) {
		av_dict_set(opts, "rist_encryption_type", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "rist_profile:"))) {
		av_dict_set(opts, "rist_profile", options_value, 0);
	}
}

static GxDemuxer* demux_lavf_open(GxDemuxer* demuxer)
{
	DemuxLavfPriv*   priv = demuxer->priv;
	AVFormatContext* avfc = avformat_alloc_context();

	if (!avfc) {
		return NULL;
	}
	if (!demuxer->stream->url) {
		avformat_close_input(&avfc);
		return NULL;
	}
	demux_lavf_get_url_options_value(demuxer, avfc, &priv->opts);

	GxPlayer_SystemGet(PSYS_INDEX_CACHE, &demuxer->index_limit);
	avfc->index_limit  = demuxer->index_limit;
	priv->avfc = avfc;
	demuxer->stream->err.open_err = 0;
	demuxer->stream->err.recv_err = 0;
	priv->avfc->concat_eof = 0;

	if (GX_STREAMTYPE_DEMUXER != demuxer->stream->file_format) {
		gxlogi ("IPTV Version:V1(demux).\n");
		return _lavf_open_demux(demuxer, &priv->opts);//only demux section.
	} else {
		gxlogi ("IPTV Version:V2(protocol+dmx).\n");
		return _lavf_open_stream_and_demux(demuxer, &priv->opts);//login ff stream and demux.
	}

	return NULL;
}

static void write_pts(uint8_t *q, int fourbits, int64_t pts)
{
	int val;

	val = fourbits << 4 | (((pts >> 30) & 0x07) << 1) | 1;
	*q++ = val;
	val = (((pts >> 15) & 0x7fff) << 1) | 1;
	*q++ = val >> 8;
	*q++ = val;
	val = (((pts) & 0x7fff) << 1) | 1;
	*q++ = val >> 8;
	*q++ = val;
}

static int lavf_dvb_subtitle_es_to_pes(DemuxLavfPriv* priv, AVPacket *pkt, AVStream *st)
{
	int header_len = 0, flags = 0, len = 0, val = 0, payload_size = pkt->size;
	uint8_t *buffer = NULL, *q = NULL;
	if (!(buffer = av_mallocz(payload_size+32)))
		return AVERROR(ENOMEM);
	q = buffer;
	*q++ = 0x00;
	*q++ = 0x00;
	*q++ = 0x01;
	*q++ = 0xbd;
	if (pkt->pts != AV_NOPTS_VALUE) {
		header_len += 5;
		flags	   |= 0x80;
	}

	len = payload_size + header_len + 3;
	if (len > 0xffff)
		len = 0;
	*q++ = len >> 8;
	*q++ = len;
	val  = 0x80;
	/* data alignment indicator is required for subtitle and data streams */
	if (st->codec->codec_type == AVMEDIA_TYPE_SUBTITLE || st->codec->codec_type == AVMEDIA_TYPE_DATA)
		val |= 0x04;
	*q++ = val;
	*q++ = flags;
	*q++ = header_len;
	if (pkt->pts != AV_NOPTS_VALUE) {
		write_pts(q, flags >> 6, pkt->pts);
		q += 5;
	}

	memcpy(q, pkt->data, pkt->size);
	q += pkt->size;

	GxFifo_Write(priv->sub_filter.fifo, buffer, q - buffer, -1);
	if (buffer)
		av_free(buffer);
	return GX_PLAYER_OK;
}

static int server_proble_is_restart(int err)
{
	int ret = 0;
	switch (err) {
	case AVERROR_HTTP_BAD_REQUEST:
	case AVERROR_HTTP_UNAUTHORIZED:
	case AVERROR_HTTP_FORBIDDEN:
	case AVERROR_HTTP_NOT_FOUND:
	case AVERROR_HTTP_OTHER_4XX:
	case AVERROR_HTTP_SERVER_ERROR:
	case AVERROR(ETIMEDOUT):
	case AVERROR_RSTRT:
		ret = 1;
		break;
	default:
		break;
	}
	return ret;
}

int demux_lavf_fill_buffer(GxDemuxer* demuxer, GxDemuxStream* dsds)
{
#define CHECK_DP_INVALID(dp)\
	do {\
		if(dp == NULL) {\
			gxlogd("###demux lavf out of memory, %d, %s###\n", __LINE__, __FILE__);\
			goto oom;\
		}\
	}while(0)

	DemuxLavfPriv* priv = demuxer->priv;
	AVPacket pkt;
	GxDemuxPacket* dp = NULL;
	GxDemuxStream* ds = NULL;
	int pid, ret = 0, is_start_switch_tarck = 0, is_full_detection = 0;
	AVFormatContext *avfc = priv->avfc;

	demuxer->filepos = GxStream_Tell(demuxer->stream);

	{
		int esacap  = 0, esvcap  = 0;
		int esasize = 0, esvsize = 0;

		GxPlayer_SystemGet(PSYS_BufSizeESA, &esacap);
		GxPlayer_SystemGet(PSYS_BufSizeESV, &esvcap);
		GxDemuxer_GetEsBufSize(demuxer, &esasize, &esvsize);
		if (((esasize >= esacap) && (esvsize <= esvcap / 8)) ||
				((esasize <= esacap / 2) && (esvsize >= esvcap))) {
			avfc->get_packet_bypts = 1;
		} else if ((esvsize <= esvcap / 16) || (esasize <= esacap / 8)) {
			avfc->get_packet_bypts = 0;
		}
	}

	if (!priv || !priv->avfc)
		return GX_PLAYER_ERROR;
	{
		int pkt_cachesize = 0, is_use_flase_live = 0;
		GxPlayer_SystemGet(PSYS_NETWORK_IS_FALSE_LIVE, &is_use_flase_live);
		GxPlayer_SystemGet(PSYS_PACKET_CACHE, &pkt_cachesize);
		/*When reading a dash package, if it is da >=3MB other dv >= cachesize/2,AV PTS can no longer be corrected in dashdec.c,  it needs to be replayed.*/
		if(is_use_flase_live
			&& (avfc && avfc->iformat && 0 == strcasecmp(avfc->iformat->name, "dash"))
			&& ((demuxer->audio && demuxer->audio->bytes >= pkt_cachesize*3*1024*1024) || (demuxer->video && demuxer->video->bytes >= pkt_cachesize/2))){
			demuxer->stream->err.recv_err = PLAYER_ERROR_SOURCE_RESTART;
			return GX_PLAYER_ERROR;
		}
	}

	/*concatdec.c send information to lavf. lavf send restart codec info to media.c, monitor to restart*/
	if (avfc->concat_eof == 1) {
		_lavf_av_send_msg(demuxer, GX_MFT_EVENT_PLAY_END, NULL);
		demuxer->info.concat_eof = 1;
		return GX_PLAYER_OK;
	}

	GxCore_MutexLock(lavf_mutex);

	if ((ret = av_read_frame(priv->avfc, &pkt)) < 0) {
		if ((avfc->pb && (AVERROR(ETIMEDOUT) == avfc->pb->error))
			|| server_proble_is_restart(ret)
			|| ((ret == AVERROR_EOF) && (avfc->duration < 0))) {
			/*AVERROR(EIO):read fail,not data */
			demuxer->stream->err.recv_err = PLAYER_ERROR_SOURCE_RESTART;
		} else if ((ret == AVERROR_EOF) || (priv->avfc && priv->avfc->pb && avio_feof(priv->avfc->pb))) {
			demuxer->stream->eof = 1;
			demuxer->stream->err.recv_err = PLAYER_ERROR_NO_ERROR;
		} else {
			gxlogi_raw_l("lavf read frame fail.ret:%d(%s)(%s).set PLAYER_ERROR_NO_DATA_SOURCE.\n", ret, av_fourcc2str(abs(ret)), av_err2str(ret));
			demuxer->stream->err.recv_err = PLAYER_ERROR_NO_DATA_SOURCE;
		}
		GxCore_MutexUnlock(lavf_mutex);
		return GX_PLAYER_ERROR;
	}
	GxCore_MutexUnlock(lavf_mutex);

	/*concatdec.c send information to lavf. lavf send restart codec info to media.c, monitor to restart*/

	if (priv->abort) {
		av_free_packet(&pkt);
		return GX_PLAYER_ERROR;
	}

	if (pkt.data == NULL || pkt.size <= 0) {
		av_free_packet(&pkt);
		return GX_PLAYER_OK;
	}

	pid = pkt.stream_index;
	if (avfc && avfc->streams[pid] && avfc->streams[pid]->codec && avfc->streams[pid]->codec->is_reset_extradata) {
		priv->extradata_size = 0;
		avfc->streams[pid]->codec->is_reset_extradata = 0;
	}

	GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);
	if(is_full_detection && (pid == priv->new_video_index || pid == priv->new_audio_index)
		&&(priv->avfc && priv->avfc->iformat && !strcasecmp(priv->avfc->iformat->name, "dash")) 
		&& avfc->is_switch_av_track && priv->switch_state) {
		GxDemuxStream* ds_new = {0};
		if(pid == priv->new_video_index){
			ds_new = demuxer->video;
			ds_new->id   = priv->new_id;
			ds_new->sh   = priv->new_sh;
			priv->video_index = priv->new_video_index;
			priv->extradata_size = 0;
			is_start_switch_tarck = 1;
			priv->new_video_index = -1;
		}else if(pid == priv->new_audio_index){
			ds_new = demuxer->audio;
			ds_new->id   = priv->new_id;
			ds_new->sh   = priv->new_sh;
			priv->audio_index = priv->new_audio_index;
			priv->new_audio_index = -1;
		}
		priv->switch_state = 0;
		avfc->is_switch_av_track = 0;
	}

	if (pid == priv->audio_index) {
		GxStreamAudioHeader* a_sh = demuxer->audio->sh;

		if (!a_sh)
			goto fill_ok;

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
			if (!priv->check_first_apkt) {
				if ((pkt.data[0] == 0xFF) && ((pkt.data[1] & 0xF6) == 0xF0))
					a_sh->raw_adts = 1;
				priv->check_first_apkt = 1;
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
		} else if (avfc->streams[pid]->codec->codec_id == CODEC_ID_VORBIS ||
				avfc->streams[pid]->codec->codec_id == CODEC_ID_OPUS) {
			int i;
			OGGStreamHeader osh;
			memset(&osh, 0, sizeof(OGGStreamHeader));

			osh.pageLogo[0]     = 0x4F;
			osh.pageLogo[1]     = 0x67;
			osh.pageLogo[2]     = 0x67;
			osh.pageLogo[3]     = 0x53;
			osh.version         = 0x00;
			osh.headerType      = 0x00;

			for (i = 0; i < 4; i++)
				osh.pageSequence[i] = priv->page_seguence>>(i*8);

			priv->page_seguence = (++priv->page_seguence == 0xffffffff) ? 0 : priv->page_seguence;

			osh.serialNumber[0] = 0x47;
			osh.serialNumber[1] = 0x58;
			osh.serialNumber[2] = 0x00;
			osh.serialNumber[3] = 0x00;
			osh.numSegments     = pkt.size/0xFF+((pkt.size%0xFF == 0)?0:1);

			i = 0;
			while (i < osh.numSegments-1)
				osh.segmentTable[i++] = 0xFF;
			osh.segmentTable[i] = pkt.size%0xFF;

			memset(osh.crcCheckSum,    0x00, 4);
			memset(osh.granulePostion, 0xff, 8);
			osh.hdrLen = OGGS_HEADER_SIZE+osh.numSegments;

			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size+osh.hdrLen);
			CHECK_DP_INVALID(dp);

			GxDemuxPacket_Write(dp, (const uint8_t*)&osh, osh.hdrLen);
			GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		} else if (avfc->streams[pid]->codec->codec_id == CODEC_ID_APE) {
			unsigned char header[8];
			header[0] = 'A';
			header[1] = 'P';
			header[2] = 'E';
			header[3] = ' ';
			header[4] = (pkt.size >> 24) & 0xff;
			header[5] = (pkt.size >> 16) & 0xff;
			header[6] = (pkt.size >>  8) & 0xff;
			header[7] = (pkt.size      ) & 0xff;

			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size+sizeof(header));
			CHECK_DP_INVALID(dp);
			GxDemuxPacket_Write(dp, (const uint8_t*)header, sizeof(header));
			GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		} else {
			dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size);
			CHECK_DP_INVALID(dp);
			GxDemuxPacket_Write(dp, pkt.data, pkt.size);
		}
	} else if (pid == priv->video_index) {
		GxStreamVideoHeader* v_sh = demuxer->video->sh;

		if (!v_sh)
			goto fill_ok;

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

			if (nal_insert && priv->nal_size_size) {
				unsigned int pkt_size = video_probe_pkt_size(demuxer, &pkt);

				if (pkt_size == 0) {
					av_free_packet(&pkt);
					return GX_PLAYER_ERROR;
				}

				dp = GxDemuxPacket_Create(demuxer, NULL, pkt_size);
				if (dp == NULL) {
					gxlogi_raw("malloc failed: loss pkt size (%d)\n", pkt_size);
					av_free_packet(&pkt);
					return GX_PLAYER_OK;
				}

				while (ppos + priv->nal_size_size < pkt.size) {
					if (lavf_interruptcbk && lavf_interruptcbk()) {
						av_free_packet(&pkt);
						GxDemuxPacket_Destroy(dp);
						return GX_PLAYER_ERROR;
					}

					GxDemuxPacket_Write(dp, start_code, 4);
					for (i = 0; i < priv->nal_size_size; ++i)
						nal_size = (nal_size << 8) | ( pkt.data[ppos++]& 0xff);
					if (nal_size > (pkt.size - ppos)) {
						gxlogi_raw("nal_error !]#####%d: %d : %d :%d\n", nal_size, pkt.size,ppos, priv->nal_size_size);
						nal_size = pkt.size - ppos;
					}
					GxDemuxPacket_Write(dp, pkt.data + ppos, nal_size);
					ppos += nal_size;
					nal_size = 0;
				}
			} else {
				dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size);
				CHECK_DP_INVALID(dp);
				GxDemuxPacket_Write(dp, pkt.data, pkt.size);
			}
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
	} else if(priv->sub_fill) {
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
		if ((pid == priv->sub_index) && ((avfc->streams[pid]->codec->codec_id == CODEC_ID_DVB_SUBTITLE && n_sh_sub->type == SUB_CODEC_DVB_DESCRIPTOR)
			|| (avfc->streams[pid]->codec->codec_id == CODEC_ID_DVB_TELETEXT && n_sh_sub->type == SUB_CODEC_TXT_DESCRIPTOR))) {
			if (pkt.pts != AV_NOPTS_VALUE) {
				int64_t pts = pkt.pts * (av_q2d(priv->avfc->streams[pid]->time_base)*1000);
				if (pts != AV_NOPTS_VALUE) {
					if (demuxer->base_time != 0) {
						int64_t start_time = avfc->streams[pid]->start_time;
						if (start_time == AV_NOPTS_VALUE) {
							pts += demuxer->base_time;
						} else {
							start_time = (start_time * av_q2d(priv->avfc->streams[pid]->time_base)*AV_TIME_BASE)/1000;
							pts += start_time;
						}
					}
				}
				gxlogd ("[CYQ] subtitle pid : %d pts : %lld  pkt.pts : %lld pkt.dts : %lld\n",pid,pts,pkt.pts,pkt.pts);
				pkt.pts = pts;
			}
			lavf_dvb_subtitle_es_to_pes(priv, &pkt, avfc->streams[pid]);
			goto fill_ok;
		} else if ((pid == priv->sub_index) ||
				(((c_sh_sub->type == SUB_CODEC_SRT) || (c_sh_sub->type == SUB_CODEC_SSA) || (c_sh_sub->type == SUB_CODEC_TTML)) &&
				 ((n_sh_sub->type == SUB_CODEC_SRT) || (n_sh_sub->type == SUB_CODEC_SSA) || (n_sh_sub->type == SUB_CODEC_TTML)))) {
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
		} else {
			goto fill_ok;
		}
	} else {
		goto fill_ok;
	}

	if (pkt.pts != AV_NOPTS_VALUE) {
		dp->pts = pkt.pts*  (av_q2d(priv->avfc->streams[pid]->time_base)*1000);
		priv->last_pts = (dp->pts * AV_TIME_BASE)/1000;
		if (pkt.duration)
			dp->endpts = dp->pts + pkt.duration * (av_q2d(priv->avfc->streams[pid]->time_base)*1000);
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
				start_time = (start_time*av_q2d(priv->avfc->streams[pid]->time_base)*AV_TIME_BASE)/1000;
				dp->pts += start_time;
			}
		}
	}
	if (is_full_detection && is_start_switch_tarck) {
		dp->is_start_switch_dp = 1;
	}
	GxDemuxStream_AddPacket(ds, dp);

fill_ok:
	av_free_packet(&pkt);
	return GX_PLAYER_OK;

oom:
	av_free_packet(&pkt);
	demuxer->stream->eof = 1;
	demuxer->video->eof = (dsds == demuxer->video) ? 1 : demuxer->video->eof;
	demuxer->audio->eof = (dsds == demuxer->audio) ? 1 : demuxer->audio->eof;
	demuxer->sub->eof   = (dsds == demuxer->sub) ? 1 : demuxer->sub->eof;

	return GX_PLAYER_ERROR;
}

static int demux_lavf_seek(GxDemuxer* demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	DemuxLavfPriv* priv = demuxer->priv;
	int avsflags = 0;
	int64_t last_pts = priv->last_pts;
	AVFormatContext* avfc = priv->avfc;

	if (flags & GX_DEMUXER_SEEK_ABSOLUTE)
		priv->last_pts = avfc->start_time;
	if (flags & GX_DEMUXER_SEEK_PERCENT) {
		if (avfc->duration == 0 || avfc->duration == AV_NOPTS_VALUE)
			return GX_PLAYER_ERROR;
		priv->last_pts += rel_seek_ms * avfc->duration;
	}
	else {
		priv->last_pts += ((rel_seek_ms * AV_TIME_BASE)/1000);
	}

	if (flags & GX_DEMUXER_SEEK_ANY)
		avsflags |= AVSEEK_FLAG_ANY;
	if (flags & GX_DEMUXER_SEEK_FRAME)
		avsflags |= AVSEEK_FLAG_FRAME;
	if (flags & GX_DEMUXER_SEEK_BACKWARD)
		avsflags |= AVSEEK_FLAG_BACKWARD;

	avsflags |= AVSEEK_FLAG_TOLERANCE;
	avfc->seek_tolerance_ms = 10000;

	if (avfc->seek_by_time)
		GxStream_SeekTime(demuxer->stream, rel_seek_ms);

	if(av_seek_frame(avfc, -1, priv->last_pts, avsflags) < 0){
		av_seek_frame(avfc, -1, last_pts, avsflags);
		ret = GX_PLAYER_ERROR;
	}
	priv->page_seguence  = 3;
	demuxer->stream->eof = 0;
	demuxer->video->eof = 0;
	demuxer->audio->eof = 0;

	return ret;
}

static int demux_lavf_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxLavfPriv* priv = demuxer->priv;

	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		{
			int duration = 0;
			if ((GX_STREAMTYPE_DEMUXER != demuxer->stream->file_format) && (GxStream_Control(demuxer->stream, GX_STREAM_CTRL_GET_TOTAL_TIME, &duration)==GX_PLAYER_OK)) {
				demuxer->duration = *((uint64_t* )arg) = (uint64_t)duration*1000;
				return GX_DEMUXER_CTRL_OK;
			}
			if (!priv || priv->avfc->duration == 0 || priv->avfc->duration == AV_NOPTS_VALUE || priv->avfc->duration < 0) {
				return GX_DEMUXER_CTRL_ERROR;
			}
			demuxer->duration = *((uint64_t* )arg) = (uint64_t)(priv->avfc->duration/ AV_TIME_BASE)*1000;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_PERCENT_POS:
		if (priv->avfc->duration == 0 || priv->avfc->duration == AV_NOPTS_VALUE)
			return GX_DEMUXER_CTRL_ERROR;

		*((int* )arg) = (int)((priv->last_pts - priv->avfc->start_time) * 100 / priv->avfc->duration);
		return GX_DEMUXER_CTRL_OK;
	case GX_DEMUXER_CTRL_SUB_ON:
		{
			int idx, newpid = *(int*)arg;
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->subtitle_disable)
				return GX_DEMUXER_CTRL_OK;

			for (idx = 0; idx < priv->sub_streams; idx++) {
				if (priv->sstreams[idx] == newpid)
					break;
			}

			if (idx >= priv->sub_streams)
				return GX_DEMUXER_CTRL_ERROR;
			GxStreamSubHeader* n_sh_sub = NULL;
			n_sh_sub = demuxer->s_streams[idx];
			if (SUB_CODEC_DVB_DESCRIPTOR == n_sh_sub->type ||
				SUB_CODEC_TXT_DESCRIPTOR == n_sh_sub->type) {
				if (!priv->sub_filter.fifo)
					priv->sub_filter.fifo = GxFifo_Create(128*1024, GX_PINFLAG_SW);
				else
					GxFifo_Reset(priv->sub_filter.fifo);

				if (priv->sub_filter.fifo) {
					if (!priv->sub_filter.data) {
						priv->sub_filter.data = av_malloc(SUB_FILTER_SIZE);
						if (priv->sub_filter.data == NULL) {
							GxFifo_Destroy(priv->sub_filter.fifo);
							priv->sub_filter.fifo = NULL;
							return GX_DEMUXER_CTRL_ERROR;
						}
					}
					priv->sub_filter.len = 0;
					priv->sub_filter.pid = newpid;
					if (newpid != priv->sub_index) {
						priv->avfc->streams[priv->sub_index]->discard = AVDISCARD_ALL;
						priv->avfc->streams[newpid]->discard  = AVDISCARD_NONE;
					}
				}
			}else {
				GxStreamSubHeader* n_sh_sub = NULL;
				n_sh_sub = demuxer->s_streams[idx];
				demuxer->sub->id = idx;
				demuxer->sub->sh = demuxer->s_streams[idx];
			}
			priv->sub_index  = newpid;
			priv->sub_fill   = 1;
			return GX_DEMUXER_CTRL_OK;
		}
		return GX_DEMUXER_CTRL_ERROR;
	case GX_DEMUXER_CTRL_SUB_OFF:
		{
			GxStreamSubHeader* c_sh_sub = demuxer->sub->sh;
			if (SUB_CODEC_DVB_DESCRIPTOR == c_sh_sub->type ||
				SUB_CODEC_TXT_DESCRIPTOR == c_sh_sub->type) {
				priv->sub_filter.pid = 0;
				if (priv->sub_filter.fifo) {
					GxFifo_Destroy(priv->sub_filter.fifo);
					priv->sub_filter.fifo = NULL;
				}
				if (priv->sub_filter.data) {
					av_free(priv->sub_filter.data);
					priv->sub_filter.data = NULL;
				}
				priv->sub_filter.len = 0;
			}
			priv->sub_fill   = 0;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SUB_READ_DATA:
		{
			GxDemuxerPesData* data = (GxDemuxerPesData*)arg;
			GxStreamSubHeader* c_sh_sub = demuxer->sub->sh;
			int max_data_size,pes_len = 0;

			if (data == NULL || data->buffer == NULL) {
				return GX_DEMUXER_CTRL_ERROR;
			}

			if (!VAILD_PID(priv->sub_filter.pid) ||
					(priv->sub_filter.data == NULL) ||
					(priv->sub_filter.fifo == NULL)) {
				data->size = 0;
				return GX_DEMUXER_CTRL_ERROR;
			}

			max_data_size = data->size;
			data->size = 0;

			while(data->size < max_data_size) {
				uint8_t header[9];
				if(GxFifo_GetLength(priv->sub_filter.fifo) < 6)
					break;
				GxFifo_Peek(priv->sub_filter.fifo,header, 6);
				pes_len = AV_RB16(header + 4) + 6;
				if(pes_len == 0)
					break;
				data->size += GxFifo_Read(priv->sub_filter.fifo, data->buffer + data->size, pes_len, -1);
			}

			if(data->size == 0)
				return GX_DEMUXER_CTRL_ERROR;

			return GX_DEMUXER_CTRL_OK;
		}

	case GX_DEMUXER_CTRL_SWITCH_AUDIO:
	case GX_DEMUXER_CTRL_SWITCH_VIDEO:
	case GX_DEMUXER_CTRL_SWITCH_SUB:
		{
			int  newpid , idx, *curpid, is_full_detection = 0;
			void **pheaders;
			int nstreams, *pstreams;
			GxDemuxStream* ds;
			GxDemuxerStreamSwitch* StreamSwitch = arg;
			GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

			if (StreamSwitch) {
				newpid = StreamSwitch->pid;

				if (cmd == GX_DEMUXER_CTRL_SWITCH_VIDEO) {
					ds       = demuxer->video;
					pheaders = demuxer->v_streams;
					curpid   = &priv->video_index;
					nstreams = priv->video_streams;
					pstreams = priv->vstreams;
				} else if (cmd == GX_DEMUXER_CTRL_SWITCH_AUDIO) {
					ds       = demuxer->audio;
					pheaders = demuxer->a_streams;
					curpid   = &priv->audio_index;
					nstreams = priv->audio_streams;
					pstreams = priv->astreams;
				} else {
					ds       = demuxer->sub;
					pheaders = demuxer->s_streams;
					curpid   = &priv->sub_index;
					nstreams = priv->sub_streams;
					pstreams = priv->sstreams;
				}

				for (idx =0; idx < nstreams; idx++) {
					if (pstreams[idx] == newpid)
						break;
				}

				if (idx >= nstreams)
					return GX_DEMUXER_CTRL_ERROR;
				if (priv->avfc && priv->avfc->iformat && !strcasecmp(priv->avfc->iformat->name, "dash")) {
					AvControl av_switch_ctr;
					av_switch_ctr.discard_idx = 0;
					av_switch_ctr.retain_idx = newpid;
					if (cmd == GX_DEMUXER_CTRL_SWITCH_AUDIO) {
						if (is_full_detection) {
							if (priv->switch_state == 0) {
								av_switch_ctr.discard_idx = *curpid;
								av_switch_ctr.retain_idx = newpid;
								avformat_control(&priv->avfc, DASH_SWITCH_AUDIO_MULITPROGRAM_TRACK, &av_switch_ctr);
 								priv->switch_state = 1;
								priv->new_id = newpid;
								priv->new_sh = pheaders[newpid];
								priv->new_audio_index = newpid;
							}
							break;
						} else {
							avformat_control(&priv->avfc, DASH_SWITCH_AUDIO_MULITPROGRAM_TRACK, &av_switch_ctr);
							demuxer->audio->id = newpid;
						}
					} else if (cmd == GX_DEMUXER_CTRL_SWITCH_SUB) {
						avformat_control(&priv->avfc, DASH_SWITCH_SUBTILE_MULITPROGRAM_TRACK, &av_switch_ctr);
						demuxer->sub->id = newpid;
					}
					return GX_DEMUXER_CTRL_OK;
				} else {
					if (ds != demuxer->sub) {
						GxDemuxStream_FreePacks(ds);
						priv->avfc->streams[*curpid]->discard = AVDISCARD_ALL;
						priv->avfc->streams[newpid]->discard  = AVDISCARD_NONE;
					}
				}

				ds->id   = idx;
				ds->sh   = pheaders[idx];
				*curpid  = newpid;
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_ERROR;
		}
	case GX_DEMUXER_CTRL_FOURC_STOP:
		{
			if(priv) priv->abort = *(int *)arg;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_SUB_DROPMODE:
		{
			int dropmode = *(int*)arg;
			demuxer->sub->dropmode = dropmode;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE:
	case GX_DEMUXER_CTRL_SET_VIDEO_DROPMODE:
		{
			int curridx;
			int dropmode = *(int*)arg;
			GxDemuxStream* ds;

			if(cmd == GX_DEMUXER_CTRL_SET_AUDIO_DROPMODE) {
				ds = demuxer->audio;
				curridx = priv->audio_index;
			}
			else {
				ds = demuxer->video;
				curridx = priv->video_index;
			}

			switch(dropmode)
			{
			case DROPMODE_NONE:
				priv->avfc->streams[curridx]->discard = AVDISCARD_NONE;
				break;
			case DROPMODE_FFFB:
			case DROPMODE_UNSUPPORT:
				priv->avfc->streams[curridx]->discard = AVDISCARD_ALL;
				break;
			default:
				return GX_DEMUXER_CTRL_OK;
			}
			GxDemuxStream_FreePacks(ds);
			ds->dropmode = dropmode;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SEAMLESS_BANDWIDTH_SWITCH:
		{
			int cur_idx = 0, is_full_detection = 0, alt_pid = -2, switch_idx = -2;
			GxDemuxStream* ds = {0};
			AvControl av_switch_ctr = {0};
			GxDemuxerStreamSwitch* StreamSwitch = arg;

			GxPlayer_SystemGet(PSYS_NETWORK_SEGMENT_FUNC_IS_FULL_DETECTION, &is_full_detection);

			if (!StreamSwitch) {
				return GX_DEMUXER_CTRL_ERROR;
			}
			alt_pid = StreamSwitch->pid;
			av_switch_ctr.discard_idx = 0;
			av_switch_ctr.retain_idx = alt_pid;
			if (priv->avfc && priv->avfc->iformat && !strcasecmp(priv->avfc->iformat->name, "dash")) {
				void **pheaders = {0};
				if (is_full_detection) {
					pheaders = demuxer->v_streams;
					if(priv->switch_state == 1)
						return GX_DEMUXER_CTRL_ERROR;
					ds = demuxer->video;
					cur_idx = priv->video_index;
					switch_idx = priv->vstreams[alt_pid];
					if ((ds->id == -2) || (switch_idx == cur_idx)) {
						return GX_DEMUXER_CTRL_ERROR;
					}
					av_switch_ctr.discard_idx = cur_idx;
					av_switch_ctr.retain_idx = priv->vstreams[alt_pid];
				}
				avformat_control(&priv->avfc, DASH_SWITCH_VIDEO_MULITPROGRAM_TRACK, &av_switch_ctr);
				if (is_full_detection) {
					GxStreamVideoHeader *v_video;
					GxStreamHeadPriv* hdr;
					ds->is_fdr_switch_flag = 1;
					v_video = pheaders[alt_pid];
					hdr = &v_video->priv;
					hdr->header.flag = 1;
					hdr->pts_sync = 1;
					priv->switch_state = 1;
					priv->new_id = alt_pid;
					priv->new_sh = pheaders[alt_pid];
					priv->new_video_index = switch_idx;
				} else {
					demuxer->stream->prog_now = alt_pid;
				}
				return GX_DEMUXER_CTRL_OK;
			} else if (priv->avfc && priv->avfc->iformat && !strcasecmp(priv->avfc->iformat->name, "hls")) {
				avformat_control(&priv->avfc, HLS_SEAMLESS_BANDWIDTH_SWITCH, &av_switch_ctr);
				demuxer->stream->prog_now = switch_idx;
				return GX_DEMUXER_CTRL_OK;
			}
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_LAVF_CONTINUE:
		{
			priv->avfc->concat_eof = 0;
			demuxer->info.concat_eof = 0;
			GxDemuxer_Reset(demuxer);
			return GX_DEMUXER_CTRL_OK;
		}
	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
	return GX_DEMUXER_CTRL_ERROR;
}

static void demux_lavf_close(GxDemuxer*  demuxer)
{
	DemuxLavfPriv* priv = demuxer->priv;
	AVDictionaryEntry *e = NULL;
	int sync_flag = 0;
	if (priv) {
		if (priv->avfc) {
			int i;
			for (i = 0; i < priv->avfc->nb_streams; i++) {
				AVStream* st = priv->avfc->streams[i];
				AVCodecContext* codec = st->codec;
				if (codec->codec_type == CODEC_TYPE_AUDIO && codec->codec_id == CODEC_ID_ADPCM_SWF)
					adpcm_decode_uninit(codec);
			}
			if (demuxer && demuxer->stream && (GX_STREAMTYPE_DEMUXER != demuxer->stream->file_format)) {
				avio_context_free(&priv->avfc->pb);
			}
			avformat_close_input(&priv->avfc);
		}

		if ((e = av_dict_get(priv->opts, "vdec_sync", NULL, 0))) {
			sync_flag = atoi(e->value);
			GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &sync_flag);
		}
		if ((e = av_dict_get(priv->opts, "aout_sync", NULL, 0))) {
			sync_flag = atoi(e->value);
			GxPlayer_SystemSet(PSYS_AOUT_SYNC_FLAG, &sync_flag);
		}

		if (priv->opts)
			av_dict_free(&priv->opts);
		av_free(priv);
		demuxer->priv = NULL;
	}
}

static int demux_lavf_init(void)
{
	if(lavf_mutex == -1)
		GxCore_MutexCreate(&lavf_mutex);

	return 0;
}

GxDemuxerClass gx_demux_lavf = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer LAVF",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.init    = demux_lavf_init,
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	DEF_AUTHOR("demuxer","lavf","No description","L.F","No comment"),

	.name        = "Demux LAVF",
	.type        = GX_DEMUXER_TYPE_LAVF,
	.check_file  = demux_lavf_check_file,
	.open        = demux_lavf_open,
	.fill_buffer = demux_lavf_fill_buffer,
	.close       = demux_lavf_close,
	.seek        = demux_lavf_seek,
	.control     = demux_lavf_control,
};
