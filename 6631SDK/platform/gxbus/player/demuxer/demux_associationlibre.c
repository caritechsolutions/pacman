#include "demux_associationlibre.h"
#include "xtr_mpeg4.h"
#include "xtr_mjpeg.h"
#include "bluray_pcm.h"
#include "adpcm.h"
#include "xiph.h"

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


static handle_t associationlibre_mutex = -1;
PLAYER_INTERRUPT_CBK ass_interruptcbk = NULL;

#define SUB_FILTER_SIZE (64*1024 + 6)
#define VAILD_PID(pid)          ((pid>0)&&(pid<0x1fff))
#define DISABLE_DEMUX_AND_CODEC 0x1

const struct AVCodecTag* mp_wav_taglists1[] = { ff_codec_wav_tags, mp_wav_tags, 0 };
const struct AVCodecTag* mp_bmp_taglists1[] = { ff_codec_bmp_tags, mp_bmp_tags, 0 };

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
	struct representation *rep = opaque;
	GxStream*  stream  = rep->stream;
	int ret;

	ret = GxStream_Read(stream, buf, size);
	if ((ret == 0) && (GxStream_Eof(stream))) {
		return -1;
	}

	return ret;
}

static offset_t mp_seek(void*  opaque, offset_t pos, int whence)
{
	struct representation *rep = opaque;
	GxStream* stream   = rep->stream;
	int64_t current_pos;

	if (whence == SEEK_CUR)
		pos += GxStream_Tell(stream);
	else if (whence == SEEK_END && stream->end_pos > 0)
		pos += stream->end_pos;
	else if (whence == SEEK_SET)
		pos += stream->start_pos;
	else if (whence == AVSEEK_SIZE && stream->end_pos > 0)
		return stream->end_pos - stream->start_pos;
	else {
		return -1;
	}

	if (pos < 0) {
		return -1;
	}

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
	DemuxAssociationLibrePriv* priv = demuxer->priv;
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
	DemuxAssociationLibrePriv* priv = demuxer->priv;
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
	DemuxAssociationLibrePriv* priv = demuxer->priv;
	unsigned int pkt_size = 0, nal_size = 0, ppos = 0, i = 0;

	while (ppos + priv->nal_size_size < pkt->size) {
		if (ass_interruptcbk && ass_interruptcbk())
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

static int parse_url_playlist(DemuxAssociationLibrePriv* priv, char *url)
{
    int ret = 0, index = 0;
    char *next_url = NULL, *cur_url = NULL;

    if (!url) {
        gxloge ("invalid url!\n");
        return AVERROR(EINVAL);
    }
    next_url = url;
    /*delete avassociationlibre: header.*/
    if (!av_strstart(next_url, "associationlibre:", (const char **)&next_url)) {
        gxloge ("invalid url!\n");
        ret = AVERROR(EINVAL);
        goto fail;
    }
	next_url = url;
	while ((cur_url = av_strtok(next_url, "|", &next_url)))  {
		struct representation *rep = NULL;
		cur_url += strspn(cur_url, "associationlibre:");
		if (index == 0) {
			if (!(rep = av_mallocz(sizeof(struct representation)))) {
				ret = AVERROR(ENOMEM);
				goto fail;
			}
			rep->url = av_strdup(cur_url);
			dynarray_add(&priv->videos, &priv->n_videos, rep);
		} else if (index == 1) {
			if (!(rep = av_mallocz(sizeof(struct representation)))) {
				ret = AVERROR(ENOMEM);
				goto fail;
			}
			rep->url = av_strdup(cur_url);
			dynarray_add(&priv->audios, &priv->n_audios, rep);
		}
		index += 1;
	}
fail:
    return ret;
}

static int demux_one_url_probe_format(struct representation *rep)
{
	int probe_size = 2048;
	AVProbeData avpd;
	uint8_t *buf = NULL;

	while(probe_size < PROBE_BUF_SIZE) {
		if(ass_interruptcbk && ass_interruptcbk()){
			if(buf)
				av_free(buf);
			return GX_DEMUXER_TYPE_UNKNOWN;
		}
		if (!(buf = av_mallocz(probe_size)))
			return GX_DEMUXER_TYPE_UNKNOWN;
		if (GxStream_Read(rep->stream, buf, probe_size) != probe_size){
			av_free(buf);
			return GX_DEMUXER_TYPE_ASSOCIATIONLIBRE;
		}
		avpd.filename = rep->stream->url;
		avpd.buf = buf;
		avpd.buf_size = probe_size;
		rep->avfc = av_probe_input_format(&avpd, 1);
		av_freep(&buf);
		if (rep->avfc)
			break;
		probe_size = probe_size<<1;
	}
	if(!rep->avfc)
		return GX_DEMUXER_TYPE_UNKNOWN;
	return GX_DEMUXER_TYPE_ASSOCIATIONLIBRE;

}

static void free_representation(struct representation *pls)
{	
	if (pls->ctx)
		avformat_close_input(&pls->ctx);
	if(pls->pb)
		av_freep(&pls->pb);
	GxStream_Close(pls->stream);
	if (pls->url) {
		av_free(pls->url);
		pls->url = NULL;
	}
	if (pls)
		av_freep(&pls);
}

static void free_video_list(DemuxAssociationLibrePriv *c)
{
	int i;
	for (i = 0; i < c->n_videos && c->videos; i++) {
		struct representation *pls = c->videos[i];
		free_representation(pls);
	}
	av_freep(&c->videos);
	c->n_videos = 0;
}

static void free_audio_list(DemuxAssociationLibrePriv *c)
{
	int i;
	for (i = 0; i < c->n_audios && c->audios; i++) {
		struct representation *pls = c->audios[i];
		free_representation(pls);
	}
	av_freep(&c->audios);
	c->n_audios = 0;
}

static void demux_associationlibre_close(GxDemuxer*  demuxer)
{
	DemuxAssociationLibrePriv* priv = demuxer->priv;
	AVDictionaryEntry *e = NULL;
	int sync_flag = 0;
	if (priv) {
		free_video_list(priv);
		free_audio_list(priv);
		if ((e = av_dict_get(priv->opts, "vdec_sync", NULL, 0))) {
			sync_flag = atoi(e->value);
			GxPlayer_SystemSet(PSYS_VDEC_SYNC_FLAG, &sync_flag);
		}
		if ((e = av_dict_get(priv->opts, "aout_sync", NULL, 0))) {
			sync_flag = atoi(e->value);
			GxPlayer_SystemSet(PSYS_AOUT_SYNC_FLAG, &sync_flag);
		}
		av_free(priv);
		demuxer->priv = NULL;
	}
}

static int demux_associationlibre_check_file(GxDemuxer*  demuxer)
{
	int i = 0, probe_size = 2048, stream_index = 0, cache_size = 0, ret = 0, seek_limit = 0;
	AVProbeData avpd;
	uint8_t *buf = NULL;
	DemuxAssociationLibrePriv* priv;
	struct representation *rep = NULL;

	if(!(demuxer->stream->file_format == GX_STREAMTYPE_STREAM && demuxer->stream->demuxer_type == GX_DEMUXER_TYPE_ASSOCIATIONLIBRE))
		return GX_DEMUXER_TYPE_UNKNOWN;
	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &ass_interruptcbk);
	if (!demuxer->priv) {
		demuxer->priv = av_mallocz(sizeof(DemuxAssociationLibrePriv));
		if (!demuxer->priv)
			return GX_DEMUXER_TYPE_UNKNOWN;
	}
	priv = demuxer->priv;
	priv->audio_index = priv->video_index = -1;
	if ((parse_url_playlist(priv, demuxer->stream->url)) < 0)
		return GX_DEMUXER_TYPE_UNKNOWN;
	for (i = 0; i < priv->n_videos; i++) {
		rep = priv->videos[i];
		if(ass_interruptcbk && ass_interruptcbk())
			goto end;
		rep->stream = GxStream_Open(rep->url,GX_STREAM_READ, 0);
		if (rep->stream == NULL) {
			goto end;
		}
		rep->priv = priv;
		rep->stream_index = stream_index;
		++stream_index;
		rep->type = CODEC_TYPE_VIDEO;
		GxPlayer_SystemGet(PSYS_NETWORK_CACHE, &cache_size);
		cache_size = cache_size >= 0x180000 ? 0x180000: cache_size;
		GxPlayer_SystemGet(PSYS_NETWORK_SEEK_CACHE, &seek_limit);
		ret = GxStream_CacheEnable(rep->stream, cache_size, seek_limit);
		if(ret == GX_PLAYER_ERROR) {
			goto end;
		}
		GxStream_Reset(rep->stream);
		GxStream_Seek(rep->stream, 0);
		if (GX_DEMUXER_TYPE_UNKNOWN == demux_one_url_probe_format(rep)) {
			goto end;
		}
	}

	for (i = 0; i < priv->n_audios; i++) {
		rep = priv->audios[i];
		if(ass_interruptcbk && ass_interruptcbk())
			goto end;
		rep->stream = GxStream_Open(rep->url,GX_STREAM_READ, 0);
		if (rep->stream == NULL) {
			gxloge ("GxStream_Open rep audio failed\n");
			goto end;
		}
		rep->priv = priv;
		rep->stream_index = stream_index;
		++stream_index;
		rep->type = CODEC_TYPE_AUDIO;
		cache_size = 0x80000;
		GxPlayer_SystemGet(PSYS_NETWORK_SEEK_CACHE, &seek_limit);
		ret = GxStream_CacheEnable(rep->stream, cache_size, seek_limit);
		if(ret == GX_PLAYER_ERROR) {
			goto end;
		}
		GxStream_Reset(rep->stream);
		GxStream_Seek(rep->stream, 0);
		if (GX_DEMUXER_TYPE_UNKNOWN == demux_one_url_probe_format(rep)) {
			goto end;
		}
	}
	if (!stream_index)
	   goto end;
	priv->priv = demuxer;
	return GX_DEMUXER_TYPE_ASSOCIATIONLIBRE;		
end:
	demux_associationlibre_close(demuxer);
	return GX_DEMUXER_TYPE_UNKNOWN;
}

static int _lavf_add_stream(AVStream* st, GxDemuxer * demuxer, int set_index_flg)
{
	int iRetFlg = GX_DEMUXER_CTRL_OK;
	DemuxAssociationLibrePriv* priv = demuxer->priv;
	AVCodecContext* codec = st->codec;
	codec->is_reset_extradata = 0;

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
			codec->codec_tag = av_codec_get_tag(mp_wav_taglists1, codec->codec_id);
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
			}
			break;
		}
	case CODEC_TYPE_VIDEO:
		{
			GxStreamVideoHeader* sh_video;
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
				codec->codec_tag = av_codec_get_tag(mp_bmp_taglists1, codec->codec_id);

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

			if (demuxer->video->id == -1 || demuxer->video->sh == NULL) {
				demuxer->video->id = priv->video_streams-1;
				demuxer->video->sh = sh_video;
				priv->video_index = priv->vstreams[priv->video_streams-1];
			}
			break;
		}		
	default:
		iRetFlg = GX_DEMUXER_CTRL_ERROR;
		break;
	}
	return iRetFlg;
}

static int associationlibre_open_demux(GxDemuxer * demuxer, struct representation *cur, AVDictionary **opts)
{
	int i = 0, ret = 0;
	int findAudioFlg = 0, findVideoFlg = 0;
	char* mp_filename    = NULL;
	DemuxAssociationLibrePriv *priv = demuxer->priv;
	AVFormatContext* avfc = NULL;

	if (!(avfc = avformat_alloc_context())) {
		return GX_PLAYER_ERROR;
	}
	GxPlayer_SystemGet(PSYS_INDEX_CACHE, &demuxer->index_limit);
	avfc->index_limit  = demuxer->index_limit;
	cur->ctx = avfc;
	GxStream_Seek(cur->stream, 0);
	cur->pb = avio_alloc_context(cur->buffer, BIO_BUFFER_SIZE, 0, cur, mp_read, NULL, mp_seek);
	avfc->pb = cur->pb;
	avfc->flags = URL_RDONLY;
	avfc->file_format  = cur->stream->file_format = GX_STREAMTYPE_STREAM;
	if(!(mp_filename = av_mallocz(PLAYER_URL_LONG + strlen("mp:")+1)))
		return GX_PLAYER_ERROR;
	av_strlcatf(mp_filename, PLAYER_URL_LONG + strlen("mp:"), "%s%s", "mp:", cur->url);
	if ((ret = avformat_open_input(&avfc, (const char*)mp_filename, cur->avfc, opts)) < 0) {
		av_free(mp_filename);
		gxloge ("demux_associationlibre.c open input fail.ret=%d.err:[%s].\n", ret, av_err2str(ret));
		return GX_PLAYER_ERROR;
	}
	av_free(mp_filename);

	avfc->seek_by_time = ((cur->stream->flags & GX_STREAM_SEEK_TIME) == GX_STREAM_SEEK_TIME) ? 1 : 0;
	demuxer->seekable  = (avfc->seek_by_time || avfc->iformat->read_seek) ? GX_STREAM_SEEK : 0;

	if (avfc->flags & AVFMT_FLAG_PTS_REORDER)
		demuxer->pts_reorder = 1;
	if ((ret = avformat_find_stream_info(avfc, opts)) < 0) {
		gxloge ("demux_associationlibre.c find stream info fail.ret=%d.err:[%s].\n", ret, av_err2str(ret));
		return GX_PLAYER_ERROR;
	}

	if (avfc->iformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->iformat->name);
	if (avfc->oformat)
		av_debug_media_mod_duty(demuxer->info.debug, demuxer->info.media, AV_MEDIA_IFROMAT, avfc->oformat->name);

	for (i = 0; i < avfc->nb_streams; i++) {
		AVStream* st = avfc->streams[i];
		st->discard = AVDISCARD_ALL;
		switch (st->codec->codec_type) {
		case CODEC_TYPE_AUDIO:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->audio_disable)
				break;
			if (!findAudioFlg) {
				st->discard = AVDISCARD_DEFAULT;
				findAudioFlg = 1;
			}
			priv->astreams[priv->audio_streams] = cur->stream_index + i;
			break;
		case CODEC_TYPE_VIDEO:
			if (DISABLE_DEMUX_AND_CODEC == demuxer->stream->video_disable)
				break;
			if (!findVideoFlg) {
				st->discard = AVDISCARD_DEFAULT;
				findVideoFlg = 1;
			}
			priv->vstreams[priv->video_streams] = cur->stream_index + i;
			break;
		default:
			continue;
		}
		_lavf_add_stream(st, demuxer, 0);
	}

	if (!priv->audio_streams)
		demuxer->audio->id = -2;	// nosound
	if (!priv->video_streams) {
		if (!priv->audio_streams) {
			return GX_PLAYER_ERROR;
		}
		demuxer->video->id = -2;	// audio-only
	}
	demuxer->start_pts = avfc->start_time/1000;
	return GX_PLAYER_OK;
}

static void demux_lavf_get_url_options_value(GxDemuxer * demuxer, AVDictionary **opts)
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
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Connection:"))) {
		av_dict_set(opts, "Connection", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "Referer:"))) {
		av_dict_set(opts, "referer", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "timeout:"))) {
		av_dict_set(opts, "rw_timeout", options_value, 0);
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
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "vn_streams:"))) {
		av_dict_set(opts, "ff_vn_streams", options_value, 0);
	}
	if ((options_value = GxOptions_Get_By_Name(demuxer->stream->options, " -H", "an_streams:"))) {
		av_dict_set(opts, "ff_an_streams", options_value, 0);
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
}

static GxDemuxer* demux_associationlibre_open(GxDemuxer* demuxer)
{
	DemuxAssociationLibrePriv*   priv = demuxer->priv;
	AVFormatContext* avfc = NULL;
	int i  = 0;

	if (!(demuxer->stream->url) || (GX_DEMUXER_TYPE_ASSOCIATIONLIBRE != demuxer->stream->demuxer_type)) {
		return NULL;
	}
	demux_lavf_get_url_options_value(demuxer, &priv->opts);
	demuxer->stream->err.open_err = 0;
	demuxer->stream->err.recv_err = 0;

	for (i = 0; i < priv->n_videos; i++) {
		struct representation *cur_video = priv->videos[i];
		if (cur_video && (GX_PLAYER_ERROR == associationlibre_open_demux(demuxer, cur_video, &priv->opts))){
			return NULL;
		}
	}

	for (i = 0; i < priv->n_audios; i++) {
		struct representation *cur_audio = priv->audios[i];
		if (cur_audio && (GX_PLAYER_ERROR == associationlibre_open_demux(demuxer, cur_audio, &priv->opts))){
			return NULL;
		}
	}
	return demuxer;
}

static int demux_associationlibre_fill_buffer(GxDemuxer* demuxer, GxDemuxStream* dsds)
{
#define CHECK_DP_INVALID(dp)\
	do {\
		if(dp == NULL) {\
			gxlogd("###demux lavf out of memory, %d, %s###\n", __LINE__, __FILE__);\
			goto oom;\
		}\
	}while(0)

	DemuxAssociationLibrePriv* asspriv = demuxer->priv;
	AVPacket pkt;
	GxDemuxPacket* dp = NULL;
	GxDemuxStream* ds = NULL;
	int pid, ret = 0, i = 0, cur_type = -1;
	int64_t mints = 0;
	AVFormatContext *avfc = NULL;
	struct representation *cur = NULL;
	struct representation *rep = NULL;

	if (!asspriv || !asspriv->n_videos || !asspriv->n_audios) {
		return GX_PLAYER_ERROR;
	}

	GxCore_MutexLock(associationlibre_mutex);
	for (i = 0; i < asspriv->n_videos; i++) {
		rep = asspriv->videos[i];
		if (!rep->ctx)
			continue;
		if (!cur || rep->cur_timestamp < mints) {
			cur = rep;
			mints = rep->cur_timestamp;
			demuxer->filepos = GxStream_Tell(cur->stream);
			cur_type = 0;
		}
	}

	for (i = 0; i < asspriv->n_audios; i++) {
		rep = asspriv->audios[i];
		if (!rep->ctx)
			continue;
		if (!cur || rep->cur_timestamp < mints) {
			cur = rep;
			mints = rep->cur_timestamp;
			cur_type = 1;
		}
	}

	if (!cur) {
		GxCore_MutexUnlock(associationlibre_mutex);
		return GX_PLAYER_ERROR;
	}
	avfc = cur->ctx;
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

	while (!ret) {
		ret = av_read_frame(avfc, &pkt);
		if (ret >= 0) {
			/* If we got a packet, return it */
			cur->cur_timestamp = av_rescale(pkt.pts, (int64_t)avfc->streams[0]->time_base.num * 90000, avfc->streams[0]->time_base.den);
			cur->avstream_index = pkt.stream_index;
			pkt.stream_index = cur->stream_index;
			break;
		} else {
			GxCore_MutexUnlock(associationlibre_mutex);
			return GX_PLAYER_ERROR;	
		}
	}
	GxCore_MutexUnlock(associationlibre_mutex);
	/*concatdec.c send information to lavf. lavf send restart codec info to media.c, monitor to restart*/
	if (asspriv->abort) {
		av_free_packet(&pkt);
		return GX_PLAYER_ERROR;
	}

	if (pkt.data == NULL || pkt.size <= 0) {
		av_free_packet(&pkt);
		return GX_PLAYER_OK;
	}

	pid = pkt.stream_index;
	if (pid == asspriv->audio_index) {
		GxStreamAudioHeader* a_sh = demuxer->audio->sh;

		if (!a_sh)
			goto fill_ok;

		ds = demuxer->audio;
		pid = cur->avstream_index;
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
			if (!asspriv->check_first_apkt) {
				if ((pkt.data[0] == 0xFF) && ((pkt.data[1] & 0xF6) == 0xF0))
					a_sh->raw_adts = 1;
				asspriv->check_first_apkt = 1;
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
				osh.pageSequence[i] = asspriv->page_seguence>>(i*8);

			asspriv->page_seguence = (++asspriv->page_seguence == 0xffffffff) ? 0 : asspriv->page_seguence;

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
	} else if (pid == asspriv->video_index) {
		GxStreamVideoHeader* v_sh = demuxer->video->sh;

		if (!v_sh)
			goto fill_ok;

		ds = demuxer->video;
		pid = cur->avstream_index;
		if (v_sh->format == VIDEO_CODEC_H264 || v_sh->format == VIDEO_CODEC_H265) {
			char nal_insert = 1;
			int i, numnal=0, nownal=NAL_UNIT;
			unsigned int nal_size = 0, ppos = 0, nal_size_size = 0;

			if (!asspriv->extradata_size && avfc->streams[pid]->codec->extradata_size) {
				if ((nal_size_size = video_probe_nal_size(avfc->streams[pid]->codec, v_sh)) != -1) {
					asspriv->nal_size_size  = nal_size_size;
					asspriv->extradata_size = avfc->streams[pid]->codec->extradata_size;
				}
			}

			if (asspriv->nal_size_size == 4) {
				if((pkt.size > 8) &&
						(nal_read_32bytes(pkt.data) == 0x01) &&
						(nal_read_32bytes(pkt.data+5) > pkt.size))
					nal_insert = 0;
			}

			if (nal_insert && asspriv->nal_size_size) {
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

				while (ppos + asspriv->nal_size_size < pkt.size) {
					if (ass_interruptcbk && ass_interruptcbk()) {
						av_free_packet(&pkt);
						GxDemuxPacket_Destroy(dp);
						return GX_PLAYER_ERROR;
					}

					GxDemuxPacket_Write(dp, start_code, 4);
					for (i = 0; i < asspriv->nal_size_size; ++i)
						nal_size = (nal_size << 8) | ( pkt.data[ppos++]& 0xff);
					if (nal_size > (pkt.size - ppos)) {
						gxloge_raw("nal_error !]#####%d: %d : %d :%d\n", nal_size, pkt.size,ppos, asspriv->nal_size_size);
						nal_size = pkt.size - ppos;
					}
					GxDemuxPacket_Write(dp, pkt.data + ppos, nal_size);
					ppos += nal_size;
					nal_size = 0;
				}
			} else {
				dp = GxDemuxPacket_Create(demuxer, NULL, pkt.size);
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
	} else {
		goto fill_ok;
	}

	pid = cur->avstream_index;
	if (pkt.pts != AV_NOPTS_VALUE) {
		dp->pts = pkt.pts*  (av_q2d(avfc->streams[pid]->time_base)*1000);
		asspriv->last_pts = (dp->pts * AV_TIME_BASE)/1000;
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
	demuxer->video->eof = (dsds == demuxer->video) ? 1 : demuxer->video->eof;
	demuxer->audio->eof = (dsds == demuxer->audio) ? 1 : demuxer->audio->eof;
	demuxer->sub->eof   = (dsds == demuxer->sub) ? 1 : demuxer->sub->eof;

	return GX_PLAYER_ERROR;
}

static int demux_one_url_seek(struct representation *rep, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK, ret1 = 0;
	int avsflags = 0;
	AVFormatContext* avfc = rep->ctx;
	int64_t last_pts = rep->last_pts;

	if (flags & GX_DEMUXER_SEEK_ABSOLUTE)
		rep->last_pts = avfc->start_time;
	if (flags & GX_DEMUXER_SEEK_PERCENT) {
		if (avfc->duration == 0 || avfc->duration == AV_NOPTS_VALUE)
			return GX_PLAYER_ERROR;
		rep->last_pts += rel_seek_ms * avfc->duration;
	}
	else {
		rep->last_pts += ((rel_seek_ms * AV_TIME_BASE)/1000);
	}

	if (flags & GX_DEMUXER_SEEK_ANY)
		avsflags |= AVSEEK_FLAG_ANY;
	if (flags & GX_DEMUXER_SEEK_FRAME)
		avsflags |= AVSEEK_FLAG_FRAME;
	if (flags & GX_DEMUXER_SEEK_BACKWARD)
		avsflags |= AVSEEK_FLAG_BACKWARD;

	avsflags |= AVSEEK_FLAG_TOLERANCE;
	avfc->seek_tolerance_ms = 10000;
	avfc->ass_is_forward = 1;

	if (avfc->seek_by_time)
		GxStream_SeekTime(rep->stream, rel_seek_ms);

	if((ret1 = av_seek_frame(avfc, -1, rep->last_pts, avsflags)) < 0){
		ret1 = av_seek_frame(avfc, -1, last_pts, avsflags);
		ret = GX_PLAYER_ERROR;
	}
	avfc->ass_is_forward = 0;
	rep->cur_timestamp = 0;
	//rep->stream->eof = 0;

	return ret;
}

static int demux_associationlibre_seek(GxDemuxer* demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	DemuxAssociationLibrePriv *priv = demuxer->priv;
	int ret = GX_PLAYER_OK, i = 0;

	for (i = 0; i < priv->n_videos; i++) {
		ret = demux_one_url_seek(priv->videos[i], rel_seek_ms, audio_delay, flags);
		if (ret == GX_PLAYER_ERROR)
			return ret;
	}
	for (i = 0; i < priv->n_audios; i++) {
		ret = demux_one_url_seek(priv->audios[i],  rel_seek_ms, audio_delay, flags);
		if (ret == GX_PLAYER_ERROR)
			return ret;
	}
	demuxer->stream->eof = 0;
	demuxer->video->eof = 0;
	demuxer->audio->eof = 0;
	return ret;

}


static int demux_associationlibre_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxAssociationLibrePriv* priv = demuxer->priv;
	struct representation *rep = NULL;

	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		{
			int duration = 0;
			rep = priv->videos[0];
			if ((GX_STREAMTYPE_DEMUXER != rep->stream->file_format) && (GxStream_Control(rep->stream, GX_STREAM_CTRL_GET_TOTAL_TIME, &duration)==GX_PLAYER_OK)) {
				demuxer->duration = *((uint64_t* )arg) = (uint64_t)duration*1000;
				return GX_DEMUXER_CTRL_OK;
			}
			if (!rep || rep->ctx->duration == 0 || rep->ctx->duration == AV_NOPTS_VALUE || rep->ctx->duration < 0) {
				return GX_DEMUXER_CTRL_ERROR;
			}
			demuxer->duration = *((uint64_t* )arg) = (uint64_t)(rep->ctx->duration/ AV_TIME_BASE)*1000;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_GET_PERCENT_POS:
		rep = priv->videos[0];
		if (rep->ctx->duration == 0 || rep->ctx->duration == AV_NOPTS_VALUE)
			return GX_DEMUXER_CTRL_ERROR;

		*((int* )arg) = (int)((rep->last_pts - rep->ctx->start_time) * 100 / rep->ctx->duration);
		return GX_DEMUXER_CTRL_OK;			
	case GX_DEMUXER_CTRL_FOURC_STOP:
		{
			if (priv)
				priv->abort = *(int *)arg;
			return GX_DEMUXER_CTRL_OK;
		}
	case GX_DEMUXER_CTRL_SET_SUB_DROPMODE:
		{
			int dropmode = *(int*)arg;
			demuxer->sub->dropmode = dropmode;
			return GX_DEMUXER_CTRL_OK;
		}		
	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
}

static int demux_associationlibre_init(void)
{
	if(associationlibre_mutex == -1)
		GxCore_MutexCreate(&associationlibre_mutex);

	return 0;
}

GxDemuxerClass gx_demux_associationlibre = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer associationlibre",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.init    = demux_associationlibre_init,
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	DEF_AUTHOR("demuxer","associationlibre","No description","L.F","No comment"),

	.name        = "Demux associationlibre",
	.type        = GX_DEMUXER_TYPE_ASSOCIATIONLIBRE,
	.check_file  = demux_associationlibre_check_file,
	.open        = demux_associationlibre_open,
	.fill_buffer = demux_associationlibre_fill_buffer,
	.close       = demux_associationlibre_close,
	.seek        = demux_associationlibre_seek,
	.control     = demux_associationlibre_control,
};
