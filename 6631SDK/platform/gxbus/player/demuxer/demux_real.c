
#include "demux_real.h"
#include "ra_depack.h"


#define REAL_SYNC_HEADER 0x00000155
//aac head//
#define AAC_NUM     3
#define AAC_MP      0
#define AAC_LC      1
#define AAC_SSR     2

#if 0
static unsigned char sipr_swaps[38][2] = {
	{0, 63}, {1, 22}, {2, 44}, {3, 90}, {5, 81}, {7, 31}, {8, 86}, {9, 58}, {10, 36}, {12, 68},
	{13, 39}, {14, 73}, {15, 53}, {16, 69}, {17, 57}, {19, 88}, {20, 34}, {21, 71}, {24, 46},
	{25, 94}, {26, 54}, {28, 75}, {29, 50}, {32, 70}, {33, 92}, {35, 74}, {38, 85}, {40, 56},
	{42, 87}, {43, 65}, {45, 59}, {48, 79}, {49, 93}, {51, 89}, {55, 95}, {61, 76}, {67, 83},
	{77, 80}
};
#endif

static GxDemuxPacket* real_asf_packet = NULL;
static uint32_t s_uchunks = 0;

static inline uint32_t bswap32(uint32_t x)
{
	x = ((x>>24)&0xff)|(((x>>16)&0xff)<<8)|(((x>>8)&0xff)<<16)|((x&0xff)<<24);
	return x;
}

static inline uint16_t bswap16(uint16_t x)
{
	x = ((x>>8)&0xFF)|((x&0xFF)<<8);
	return x;
}

static void get_str(int isbyte, GxDemuxer*  demuxer, char *buf, int buf_size)
{
	int len;

	if (isbyte)
		len = GxStream_ReadChar(demuxer->stream);
	else
		len = GxStream_ReadWord(demuxer->stream);

	GxStream_Read(demuxer->stream, (uint8_t* )buf, (len > buf_size) ? buf_size : len);
	if (len > buf_size)
		GxStream_Skip(demuxer->stream, len - buf_size);

}

static int parse_index_chunk(GxDemuxer*  demuxer)
{
	DemuxRealPriv* priv = demuxer->priv;
	int origpos = GxStream_Tell(demuxer->stream);
	int next_header_pos = priv->index_chunk_offset;
	int i, entries, stream_id;

      read_index:
	GxStream_Seek(demuxer->stream, next_header_pos);

	i = GxStream_ReadDword_Le(demuxer->stream);
	if ((i == -256) || (i != MKTAG('I', 'N', 'D', 'X'))) {
		demuxer->index_mode = -1;
		if (i == -256)
			GxStream_Reset(demuxer->stream);
		GxStream_Seek(demuxer->stream, origpos);
		return 0;
		//goto end;
	}

	i = GxStream_ReadDword(demuxer->stream);

	i = GxStream_ReadWord(demuxer->stream);

	entries = GxStream_ReadDword(demuxer->stream);

	stream_id = GxStream_ReadWord(demuxer->stream);

	next_header_pos = GxStream_ReadDword(demuxer->stream);

	if (entries <= 0 || entries > MAX_INDEX_ENTRIES) {
		if (next_header_pos)
			goto read_index;
		i = entries;
		goto end;
	}

	priv->index_table_size[stream_id] = entries;
	priv->index_table[stream_id] = av_calloc(priv->index_table_size[stream_id], sizeof(RealIndexTable));

	for (i = 0; i < entries; i++) {
		GxStream_Skip(demuxer->stream, 2);	/* version*/
		priv->index_table[stream_id][i].timestamp = GxStream_ReadDword(demuxer->stream);
		priv->index_table[stream_id][i].offset = GxStream_ReadDword(demuxer->stream);
		GxStream_Skip(demuxer->stream, 4);	/* packetno*/

	}

	if (next_header_pos > 0)
		goto read_index;

      end:
	if (i == -256)
		GxStream_Reset(demuxer->stream);
	GxStream_Seek(demuxer->stream, origpos);
	if (i == -256)
		return 0;
	else
		return 1;
}

static void add_index_item(GxDemuxer*  demuxer, int stream_id, unsigned int timestamp, int offset)
{
	int media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	if ((unsigned)stream_id < media_max_streams) {
		DemuxRealPriv* priv = demuxer->priv;
		RealIndexTable* index;
		if (priv->index_table_size[stream_id] >= MAX_INDEX_ENTRIES) {
			return;
		}
		if (priv->index_table_size[stream_id] >= priv->index_av_malloc_size[stream_id]) {
			if (priv->index_av_malloc_size[stream_id] == 0)
				priv->index_av_malloc_size[stream_id] = 2048;
			else
				priv->index_av_malloc_size[stream_id] += priv->index_av_malloc_size[stream_id] / 2;
			// in case we have a really large chunk...
			if (priv->index_table_size[stream_id] >= priv->index_av_malloc_size[stream_id])
				priv->index_av_malloc_size[stream_id] = priv->index_table_size[stream_id] + 1;
			priv->index_table[stream_id] =
			    av_realloc(priv->index_table[stream_id],
				    priv->index_av_malloc_size[stream_id]*  sizeof(priv->index_table[0][0]));
		}
		if (priv->index_table_size[stream_id] > 0) {
			index = &priv->index_table[stream_id][priv->index_table_size[stream_id] - 1];
			if (index->timestamp >= timestamp || index->offset >= offset)
				return;
		}
		index = &priv->index_table[stream_id][priv->index_table_size[stream_id]++];
		index->timestamp = timestamp;
		index->offset = offset;
	}
}

static void add_index_segment(GxDemuxer*  demuxer, int seek_stream_id, int64_t seek_timestamp)
{
	int tag, len, stream_id, flags;
	unsigned int timestamp;
	int media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	if (seek_timestamp != -1 && (unsigned)seek_stream_id >= media_max_streams)
		return;
	while (1) {
		demuxer->filepos = GxStream_Tell(demuxer->stream);

		tag = GxStream_ReadDword(demuxer->stream);
		if (tag == MKTAG('A', 'T', 'A', 'D')) {
			GxStream_Skip(demuxer->stream, 14);
			continue;	/* skip to next loop*/
		}
		len = tag & 0xffff;
		if (tag == -256 || len < 12)
			break;

		stream_id = GxStream_ReadWord(demuxer->stream);
		timestamp = GxStream_ReadDword(demuxer->stream);

		GxStream_Skip(demuxer->stream, 1);	/* reserved*/
		flags = GxStream_ReadChar(demuxer->stream);

		if (flags == -256)
			break;

		if (flags & 2) {
			add_index_item(demuxer, stream_id, timestamp, demuxer->filepos);
			if (stream_id == seek_stream_id && timestamp >= seek_timestamp) {
				GxStream_Seek(demuxer->stream, demuxer->filepos);
				return;
			}
		}
		// gxlogf("Index: stream=%d packet=%d timestamp=%u len=%d flags=0x%x datapos=0x%x\n", stream_id, entries, timestamp, len, flags, index->offset);
		/* skip data*/
		GxStream_Skip(demuxer->stream, len - 12);
	}
}

static int generate_index(GxDemuxer*  demuxer)
{
	DemuxRealPriv* priv = demuxer->priv;
	int origpos = GxStream_Tell(demuxer->stream);
	int data_pos = priv->data_chunk_offset - 10;
	int tag;

	GxStream_Seek(demuxer->stream, data_pos);
	tag = GxStream_ReadDword(demuxer->stream);
	if (tag == MKTAG('A', 'T', 'A', 'D')) {
		GxStream_Skip(demuxer->stream, 14);
		add_index_segment(demuxer, -1, -1);
	}

	GxStream_Reset(demuxer->stream);
	GxStream_Seek(demuxer->stream, origpos);
	return 0;
}

static int real_check_file(GxDemuxer*  demuxer)
{
	DemuxRealPriv* priv;
	int c, media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	c = GxStream_ReadDword_Le(demuxer->stream);
	if (c == -256)
		return 0;	/* EOF*/
	if (c != MKTAG('.', 'R', 'M', 'F'))
		return 0;	/* bad magic*/

	if (!(priv = av_mallocz(sizeof(DemuxRealPriv))))
		goto exit;
	if (!(priv->index_table = av_mallocz(sizeof(RealIndexTable)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->index_table_size = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->index_av_malloc_size = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->str_data_offset = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->intl_id = av_mallocz(sizeof(unsigned int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->sub_packet_size = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->sub_packet_h = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->coded_framesize = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	if (!(priv->audiopk_size = av_mallocz(sizeof(int)*media_max_streams))) {
		goto exit;
	}
	demuxer->priv = priv;

	return GX_DEMUXER_TYPE_REAL;
exit:
	if (priv->audiopk_size)
		av_free(priv->audiopk_size);
	if (priv->coded_framesize)
		av_free(priv->coded_framesize);
	if (priv->sub_packet_h)
		av_free(priv->sub_packet_h);
	if (priv->sub_packet_size)
		av_free(priv->sub_packet_size);
	if (priv->intl_id)
		av_free(priv->intl_id);
	if (priv->str_data_offset)
		av_free(priv->str_data_offset);
	if (priv->index_av_malloc_size)
		av_free(priv->index_av_malloc_size);
	if (priv->index_av_malloc_size)
		av_free(priv->index_av_malloc_size);
	if (priv->index_table_size)
		av_free(priv->index_table_size);
	if (priv->index_table)
		av_free(priv->index_table);
	if (priv)
		av_free(priv);
	return 0;

}

static double real_fix_timestamp(DemuxRealPriv*  priv, unsigned char *s, unsigned int timestamp, double frametime,
				 unsigned int format)
{
	double v_pts;
	uint32_t buffer = (s[0] << 24) + (s[1] << 16) + (s[2] << 8) + s[3];
	unsigned int kf = timestamp;
	int pict_type;
	unsigned int orig_kf;

	if (format == GX_FOURCC('R', 'V', '3', '0') || format == GX_FOURCC('R', 'V', '4', '0')) {
		if (format == GX_FOURCC('R', 'V', '3', '0')) {
			SKIP_BITS(3);
			pict_type = SHOW_BITS(2);
			SKIP_BITS(2 + 7);
		} else {
			SKIP_BITS(1);
			pict_type = SHOW_BITS(2);
			SKIP_BITS(2 + 7 + 3);
		}
		orig_kf = kf = SHOW_BITS(13);	//    kf= 2*SHOW_BITS(12);
//    if(pict_type==0)
		if (pict_type <= 1) {
			// I frame, sync timestamps:
			priv->kf_base = (int64_t) timestamp - kf;
			kf = timestamp;
		} else {
			// P/B frame, merge timestamps:
			int64_t tmp = (int64_t) timestamp - priv->kf_base;
			kf |= tmp & (~0x1fff);	// combine with packet timestamp
			if (kf < tmp - 4096)
				kf += 8192;
			else	// workaround wrap-around problems
			if (kf > tmp + 4096)
				kf -= 8192;
			kf += priv->kf_base;
		}
		if (pict_type != 3) {	// P || I  frame -> swap timestamps
			unsigned int tmp = kf;
			kf = priv->kf_pts;
			priv->kf_pts = tmp;
//      if(kf<=tmp) kf=0;
		}
	}
	v_pts = kf;
//    if(v_pts<priv->v_pts || !kf) v_pts=priv->v_pts+frametime;
	priv->v_pts = v_pts;
	return v_pts;
}

// return value:
//     0 = EOF or no stream found
//     1 = successfully read a packet
static int demux_real_fill_buffer(GxDemuxer*  demuxer,GxDemuxStream* dsds)
{
	DemuxRealPriv* priv = demuxer->priv;
	GxDemuxStream* ds = NULL;
	int len;
	unsigned int timestamp;
	int stream_id;
	int flags;
	int version;
	int reserved;
	GxDemuxPacket* dp;
	int audioreorder_getnextpk = 0;
	int media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);


	// Don't demux video if video codec init failed
	if (demuxer->video->id >= 0 && !demuxer->video->sh)
		demuxer->video->id = -2;

	while (!GxStream_Eof(demuxer->stream)) {
		/* Handle audio/video demxing switch for multirate files (non-interleaved)*/
		if (priv->is_multirate && priv->stream_switch) {
			if (priv->a_idx_ptr >= priv->index_table_size[demuxer->audio->id])
				demuxer->audio->eof = 1;
			if (priv->v_idx_ptr >= priv->index_table_size[demuxer->video->id])
				demuxer->video->eof = 1;
			if (demuxer->audio->eof && demuxer->video->eof)
				return GX_PLAYER_ERROR;
			else if (!demuxer->audio->eof && demuxer->video->eof)
				GxStream_Seek(demuxer->stream, priv->audio_curpos);	// Get audio
			else if (demuxer->audio->eof && !demuxer->video->eof)
				GxStream_Seek(demuxer->stream, priv->video_curpos);	// Get video
			else if (priv->index_table[demuxer->audio->id][priv->a_idx_ptr].timestamp <
				 priv->index_table[demuxer->video->id][priv->v_idx_ptr].timestamp)
				GxStream_Seek(demuxer->stream, priv->audio_curpos);	// Get audio
			else
				GxStream_Seek(demuxer->stream, priv->video_curpos);	// Get video
			priv->stream_switch = 0;
		}

		demuxer->filepos = GxStream_Tell(demuxer->stream);
		version = GxStream_ReadWord(demuxer->stream);	/* version*/
		len = GxStream_ReadWord(demuxer->stream);
		if ((version == 0x4441) && (len == 0x5441)) {	// new data chunk

			if (priv->is_multirate)
				return GX_PLAYER_ERROR;	// EOF
			GxStream_Skip(demuxer->stream, 14);
			demuxer->filepos = GxStream_Tell(demuxer->stream);
			version = GxStream_ReadWord(demuxer->stream);	/* version*/
			len = GxStream_ReadWord(demuxer->stream);
		} else if ((version == 0x494e) && (len == 0x4458)) {

			demuxer->stream->eof = 1;
			return GX_PLAYER_ERROR;
		}

		if (len == -256) {	/* EOF*/
			return GX_PLAYER_ERROR;
		}
		if (len < 12) {
			GxStream_Skip(demuxer->stream, len);
			continue;	//goto loop;
		}

		stream_id = GxStream_ReadWord(demuxer->stream);
		timestamp = GxStream_ReadDword(demuxer->stream);
		reserved = GxStream_ReadChar(demuxer->stream);
		flags = GxStream_ReadChar(demuxer->stream);
//gxlogd("%s %d sid:%x, aid:%x, vid:%x\n",__FUNCTION__,__LINE__,stream_id,demuxer->audio->id,demuxer->video->id);
		/* flags:          */
		/*  0x1 - reliable */
		/*  0x2 - keyframe */

		if (version == 1) {
			int tmp;
			tmp = GxStream_ReadChar(demuxer->stream);
			len--;
		}

		//if (flags & 2)
		//	add_index_item(demuxer, stream_id, timestamp, demuxer->filepos);

		priv->current_packet++;
		len -= 12;

		/* check stream_id:*/

		if (demuxer->audio->id == stream_id) {
			if (priv->audio_need_keyframe == 1 && flags != 0x2)
				goto discard;
			got_audio:
			ds = demuxer->audio;
			if (flags & 2) {
				priv->sub_packet_cnt = 0;
				audioreorder_getnextpk = 0;
			}
			// parse audio chunk:
			{
				if (((GxStreamAudioHeader* ) ds->sh)->format == GX_FOURCC('R', 'A', 'A', 'C')) {
					uint16_t* sub_packet_lengths, sub_packets, i;
					uint32_t  packet_len = 0;
					uint16_t* dp_audio_extra;
					RealDemuxAudioHeader* dp_audio = NULL;
					GxDemuxPacket* dp = NULL;
					/* AAC in Real: several AAC frames in one Real packet.*/
					/* Second byte, upper four bits: number of AAC frames*/
					/* next n*  2 bytes: length of the AAC frames in bytes, BE */
					sub_packets = (GxStream_ReadWord(demuxer->stream) & 0xf0) >> 4;
					sub_packet_lengths = av_calloc(sub_packets, sizeof(uint16_t));
					for (i = 0; i < sub_packets; i++)
					{
						sub_packet_lengths[i] = GxStream_ReadWord(demuxer->stream);
						packet_len+=(uint32_t)sub_packet_lengths[i];
					}
					dp = GxDemuxPacket_Create(demuxer, NULL,sizeof(RealDemuxAudioHeader)+2*(sub_packets+1)+packet_len);
					dp->pos = demuxer->filepos;
					dp->flags = (flags&0x2)?0:0x10;
					dp_audio = (RealDemuxAudioHeader*)dp->buffer;
					dp_audio->u32DataLen = bswap32(sizeof(RealDemuxAudioHeader)+2*(sub_packets+1)+packet_len);
					dp_audio->u16Reserved = bswap16(reserved);
					dp_audio->u32Timestamp = bswap32(timestamp);
					dp_audio->u16DataFlags = bswap16((flags&0x2?0:0x10));
					dp_audio_extra = (uint16_t*)(dp->buffer+sizeof(RealDemuxAudioHeader));
					dp_audio_extra[0] = bswap16(sub_packets<<4);
					dp_audio_extra++;
					for(i = 0; i < sub_packets; i++)
						dp_audio_extra[i]=bswap16(sub_packet_lengths[i]);
					GxStream_Read(demuxer->stream,dp->buffer+sizeof(RealDemuxAudioHeader)+2*(sub_packets+1),packet_len);
					dp->pts = timestamp;
//gxlogd("audio: pts = %x, packs=%x\n",(uint32_t)dp->pts,ds->packs);
					GxDemuxStream_AddPacket(ds, dp);
					av_free(sub_packet_lengths);
					return GX_PLAYER_OK;
				}

				if (((GxStreamAudioHeader* ) ds->sh)->format == GX_FOURCC('c', 'o', 'o', 'k'))
				{
					RealDemuxAudioHeader* dp_audio = NULL;

					dp = GxDemuxPacket_Create(demuxer, NULL,len+sizeof(RealDemuxAudioHeader));
					dp_audio = (RealDemuxAudioHeader*)dp->buffer;
					dp_audio->u32DataLen = bswap32(len + sizeof(RealDemuxAudioHeader));
					dp_audio->u16Reserved = bswap16(reserved);
					dp_audio->u32Timestamp = bswap32(timestamp);
					dp_audio->u16DataFlags = bswap16(flags&0x2?0:0x01);
					GxStream_Read(demuxer->stream, dp->buffer + sizeof(RealDemuxAudioHeader), len);
					if (priv->audio_need_keyframe == 1) {
						priv->audio_need_keyframe = 0;
					}
					dp->pts = timestamp;
					priv->a_pts = timestamp;
					dp->pos = demuxer->filepos;
					dp->flags = (flags&0x2)?0:0x01;
//gxlogd("audio: pts = %x, packs=%x\n",(uint32_t)dp->pts,ds->packs);
					GxDemuxStream_AddPacket(ds, dp);
					return GX_PLAYER_OK;
				}
			}
// we will not use audio index if we use -idx and have a video
			if (!demuxer->video->sh && demuxer->index_mode == 2
			    && (unsigned)demuxer->audio->id < media_max_streams)
				while (priv->current_apacket + 1 < priv->index_table_size[demuxer->audio->id]
				       && timestamp >
				       priv->index_table[demuxer->audio->id][priv->current_apacket].timestamp)
					priv->current_apacket += 1;

			if (priv->is_multirate)
				while (priv->a_idx_ptr + 1 < priv->index_table_size[demuxer->audio->id] &&
				       timestamp >
				       priv->index_table[demuxer->audio->id][priv->a_idx_ptr + 1].timestamp) {
					priv->a_idx_ptr++;
					priv->audio_curpos = GxStream_Tell(demuxer->stream);
					priv->stream_switch = 1;
				}
			// If we're reordering audio packets and we need more data get it
			if (audioreorder_getnextpk)
				continue;

			return GX_PLAYER_OK;
		}

		if (demuxer->video->id == stream_id) {
		      got_video:
			ds = demuxer->video;

			// parse video chunk:
			{
				// we need a more complicated, 2nd level demuxing, as the video
				// frames are stored fragmented in the video chunks :(
				GxStreamVideoHeader* sh_video = ds->sh;
				GxDemuxPacket* dp;
				unsigned vpkg_header, vpkg_length, vpkg_offset;
				int vpkg_seqnum = -1;
				int vpkg_subseq = 0;

				while (len > 2) {
					RealDemuxVideoHeader* dp_hdr;
					unsigned char* dp_data;
					uint32_t* extra;

					// read packet header
					// bit 7: 1=last block in block chain
					// bit 6: 1=short header (only one block?)
					vpkg_header = GxStream_ReadChar(demuxer->stream);
					--len;

					if (0x40 == (vpkg_header & 0xc0)) {
						// seems to be a very short header
						// 2 bytes, purpose of the second byte yet unknown
						int bummer;
						bummer = GxStream_ReadChar(demuxer->stream);
						--len;
						vpkg_offset = 0;
						vpkg_length = len;
					} else {

						if (0 == (vpkg_header & 0x40)) {
							// sub-seqnum (bits 0-6: number of fragment. bit 7: ???)
							vpkg_subseq = GxStream_ReadChar(demuxer->stream);
							--len;
							vpkg_subseq &= 0x7f;
						}
						// size of the complete packet
						// bit 14 is always one (same applies to the offset)
						vpkg_length = GxStream_ReadWord(demuxer->stream);
						len -= 2;
						if (!(vpkg_length & 0xC000)) {
							vpkg_length <<= 16;
							vpkg_length |= (uint16_t) GxStream_ReadWord(demuxer->stream);
							len -= 2;
						} else
							vpkg_length &= 0x3fff;
						// offset of the following data inside the complete packet
						// Note: if (hdr&0xC0)==0x80 then offset is relative to the
						// _end_ of the packet, so it's equal to fragment size!!!
						vpkg_offset = GxStream_ReadWord(demuxer->stream);
						len -= 2;
						if (!(vpkg_offset & 0xC000)) {
							vpkg_offset <<= 16;
							vpkg_offset |= (uint16_t) GxStream_ReadWord(demuxer->stream);
							len -= 2;
						} else
							vpkg_offset &= 0x3fff;
						vpkg_seqnum = GxStream_ReadChar(demuxer->stream);
						--len;
					}

					if (real_asf_packet) {
						dp = real_asf_packet;
						dp_hdr = (RealDemuxVideoHeader* ) dp->buffer;
						dp_data = dp->buffer + sizeof(RealDemuxVideoHeader) + 8*dp_hdr->segnum;
						extra = (uint32_t* ) (dp->buffer + sizeof(RealDemuxVideoHeader));
						// we have an incomplete packet:
						if (dp_hdr->seqnum != vpkg_seqnum) {
							// this fragment is for new packet, close the old one
							if (priv->video_after_seek) {
								priv->kf_base = 0;
								priv->kf_pts = dp_hdr->timestamp;
								dp->pts =
								    real_fix_timestamp(priv, dp_data, dp_hdr->timestamp,
										       sh_video->frametime,
										       sh_video->format);
								priv->video_after_seek = 0;
							} else if (dp_hdr->len >= 3)
								dp->pts =
								    real_fix_timestamp(priv, dp_data, dp_hdr->timestamp,
										       sh_video->frametime,
										       sh_video->format);
							dp->pts           = dp_hdr->timestamp;
							dp_hdr->len       = bswap32(dp_hdr->len);
							dp_hdr->timestamp = bswap32(dp_hdr->timestamp);
							dp_hdr->seqnum    = bswap16(dp_hdr->seqnum);
							dp_hdr->flags     = bswap16(dp_hdr->flags);
							dp_hdr->lastpacket= bswap32(0x1);
							dp_hdr->segnum    = bswap32(dp_hdr->segnum);
							GxDemuxStream_AddPacket(ds, dp);
							real_asf_packet = NULL;
							s_uchunks = 0;
						} else {
							// append data to it!
							++s_uchunks;
							if (s_uchunks >= dp_hdr->segnum) {
								// increase buffer size, this should not happen!
								uint32_t tmp_len = dp_hdr->len;
								uint8_t* tmp_buf = av_malloc(tmp_len);
								memcpy(tmp_buf,dp_data,tmp_len);
								// re-calc pointers:
								dp = GxDemuxPacket_Resize(demuxer, dp,dp->len+8);
								real_asf_packet = dp;
								dp_hdr = (RealDemuxVideoHeader* ) dp->buffer;
								dp_hdr->segnum++;
								dp_data = dp->buffer + sizeof(RealDemuxVideoHeader) + 8*dp_hdr->segnum;
								extra = (uint32_t* ) (dp->buffer + sizeof(RealDemuxVideoHeader));
								memcpy(dp_data,tmp_buf,tmp_len);
								av_free(tmp_buf);
							}
							extra[2*  s_uchunks + 0] = bswap32(0x1);
							extra[2*  s_uchunks + 1] = bswap32(dp_hdr->len);
							if (0x80 == (vpkg_header & 0xc0)) {
								// last fragment!
								GxStream_Read(demuxer->stream, dp_data + dp_hdr->len,
									      vpkg_offset);
								if ((dp_data[dp_hdr->len] & 0x20) && (sh_video->format == 0x30335652))
									--s_uchunks;
								else
									dp_hdr->len += vpkg_offset;
								len -= vpkg_offset;
								// we know that this is the last fragment -> we can close the packet!
								if (priv->video_after_seek) {
									priv->kf_base = 0;
									priv->kf_pts = dp_hdr->timestamp;
									dp->pts =
									    real_fix_timestamp(priv, dp_data,
											       dp_hdr->timestamp,
											       sh_video->frametime,
											       sh_video->format);
									priv->video_after_seek = 0;
								} else if (dp_hdr->len >= 3)
									dp->pts =
									    real_fix_timestamp(priv, dp_data,
											       dp_hdr->timestamp,
											       sh_video->frametime,
											       sh_video->format);
								dp->pts           = dp_hdr->timestamp;
								dp_hdr->len       = bswap32(dp_hdr->len);
								dp_hdr->timestamp = bswap32(dp_hdr->timestamp);
								dp_hdr->seqnum    = bswap16(dp_hdr->seqnum);
								dp_hdr->flags     = bswap16(dp_hdr->flags);
								dp_hdr->lastpacket= bswap32(dp_hdr->lastpacket);
								dp_hdr->segnum    = bswap32(dp_hdr->segnum);
//gxlogd("\t\t\t\t\t\tvideo: pts = %x, packs=%x\n",(uint32_t)dp->pts,ds->packs);
								GxDemuxStream_AddPacket(ds, dp);
								real_asf_packet = NULL;
								s_uchunks = 0;
								continue;
							}
							// non-last fragment:
							GxStream_Read(demuxer->stream, dp_data + dp_hdr->len, len);
							if ((dp_data[dp_hdr->len] & 0x20) && (sh_video->format == 0x30335652))
								--s_uchunks;
							else
								dp_hdr->len += len;
							len = 0;
							break;	// no more fragments in this chunk!
						}
					}
					// create new packet!
					dp = GxDemuxPacket_Create(demuxer, NULL,sizeof(RealDemuxVideoHeader) + vpkg_length +
								  8* ( 2*(vpkg_header&0x3F) - 1));
					// the timestamp seems to be in milliseconds
					dp->pos = demuxer->filepos;
					dp->flags = (flags & 0x2) ? 0x10 : 0;
					dp_hdr = (RealDemuxVideoHeader* ) dp->buffer;
					dp_hdr->sync1 = bswap32(REAL_SYNC_HEADER);
					dp_hdr->sync2 = bswap32(REAL_SYNC_HEADER);
					dp_hdr->sync3 = bswap32(REAL_SYNC_HEADER);
					dp_hdr->sync4 = bswap32(REAL_SYNC_HEADER);
					dp_hdr->len = vpkg_length;
					dp_hdr->timestamp = timestamp;
					dp_hdr->seqnum = vpkg_seqnum;
					dp_hdr->lastpacket = 0;
					dp_hdr->flags = (flags & 0x2) ? 0x10 : 0;
					dp_hdr->segnum = 2*(vpkg_header&0x3F) - 1;
					extra = (uint32_t* ) (dp->buffer + sizeof(RealDemuxVideoHeader));
					s_uchunks = 0;
					extra[2*s_uchunks + 0] = bswap32(0x1);
					extra[2*s_uchunks + 1] = bswap32(0x0);	// offset of the first chunk
					dp_data = dp->buffer + sizeof(RealDemuxVideoHeader)+8*dp_hdr->segnum;
					if (0x00 == (vpkg_header & 0xc0)) {
						// first fragment:
						dp_hdr->len = len;
						GxStream_Read(demuxer->stream, dp_data, len);
						real_asf_packet = dp;
						len = 0;
						if (priv->video_after_seek) {
							priv->kf_base = 0;
							priv->kf_pts = dp_hdr->timestamp;
							dp->pts =
							    real_fix_timestamp(priv, dp_data, dp_hdr->timestamp,
									       sh_video->frametime, sh_video->format);
							priv->video_after_seek = 0;
						}
						break;
					}
					// whole packet (not fragmented):
					if (vpkg_length > len) {
						/*
						*  To keep the video stream rolling, we need to break
						*  here. We shouldn't touch len to make sure rest of the
						*  broken packet is skipped.
						*/
						break;
					}
					len -= vpkg_length;
					GxStream_Read(demuxer->stream, dp_data, vpkg_length);
					if (priv->video_after_seek) {
						priv->kf_base = 0;
						priv->kf_pts = dp_hdr->timestamp;
						dp->pts =
						    real_fix_timestamp(priv, dp_data, dp_hdr->timestamp,
								       sh_video->frametime, sh_video->format);
						priv->video_after_seek = 0;
					} else if (dp_hdr->len >= 3)
						dp->pts =
						    real_fix_timestamp(priv, dp_data, dp_hdr->timestamp,
								       sh_video->frametime, sh_video->format);
						dp->pts           = dp_hdr->timestamp;
						dp_hdr->len       = bswap32(dp_hdr->len);
						dp_hdr->timestamp = bswap32(dp_hdr->timestamp);
						dp_hdr->seqnum    = bswap16(dp_hdr->seqnum);
						dp_hdr->flags     = bswap16(dp_hdr->flags);
						dp_hdr->lastpacket= bswap32(dp_hdr->lastpacket);
						dp_hdr->segnum    = bswap32(dp_hdr->segnum);
					GxDemuxStream_AddPacket(ds, dp);
				}	// while(len>0)
				if (len > 0)
					GxStream_Skip(demuxer->stream, len);
			}
			if ((unsigned)demuxer->video->id < media_max_streams)
				while (priv->current_vpacket + 1 < priv->index_table_size[demuxer->video->id] &&
				       timestamp >
				       priv->index_table[demuxer->video->id][priv->current_vpacket + 1].timestamp)
					priv->current_vpacket += 1;

			if (priv->is_multirate)
				while (priv->v_idx_ptr + 1 < priv->index_table_size[demuxer->video->id] &&
				       timestamp >
				       priv->index_table[demuxer->video->id][priv->v_idx_ptr + 1].timestamp) {
					priv->v_idx_ptr++;
					priv->video_curpos = GxStream_Tell(demuxer->stream);
					priv->stream_switch = 1;
				}

			return GX_PLAYER_OK;
		}

		if ((unsigned)stream_id < media_max_streams) {
			if (demuxer->audio->id == -1 && demuxer->a_streams[stream_id]) {
				GxStreamAudioHeader* sh = demuxer->a_streams[stream_id];
				demuxer->audio->id = stream_id;
				sh->ds = demuxer->audio;
				demuxer->audio->sh = sh;
				priv->audio_buf =
				    av_calloc(priv->sub_packet_h[demuxer->audio->id],
					   priv->audiopk_size[demuxer->audio->id]);
				priv->audio_timestamp = av_calloc(priv->sub_packet_h[demuxer->audio->id], sizeof(double));
				goto got_audio;
			}
			if (demuxer->video->id == -1 && demuxer->v_streams[stream_id]) {
				GxStreamVideoHeader* sh = demuxer->v_streams[stream_id];
				demuxer->video->id = stream_id;
				sh->ds = demuxer->video;
				demuxer->video->sh = sh;
				goto got_video;
			}
		}
	discard:
		GxStream_Skip(demuxer->stream, len);
	}			//    goto loop;
	return GX_PLAYER_ERROR;
}

static GxDemuxer* demux_open_real(GxDemuxer * demuxer)
{
	DemuxRealPriv* priv = demuxer->priv;
	int num_of_headers;
	int a_streams = 0;
	int v_streams = 0;
	int i;
	int header_size;
	int media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	header_size = GxStream_ReadDword(demuxer->stream);	/* header size*/
	i = GxStream_ReadWord(demuxer->stream);	/* version*/
	if (header_size == 0x10)
		i = GxStream_ReadWord(demuxer->stream);
	else			/* we should test header_size here too.*/
		i = GxStream_ReadDword(demuxer->stream);
	num_of_headers = GxStream_ReadDword(demuxer->stream);

gxlogd("%s %d...\n",__FUNCTION__,__LINE__);
	/* parse chunks*/
	for (i = 1; i <= num_of_headers; i++)
//    for (i = 1; ; i++)
	{
		int chunk_id, chunk_pos, chunk_size;

		chunk_pos = GxStream_Tell(demuxer->stream);
		chunk_id = GxStream_ReadDword_Le(demuxer->stream);
		chunk_size = GxStream_ReadDword(demuxer->stream);

//gxlogd ("num_of_headers=%d, cur_num=%d, chunk_id='%C%C%C%C', chunk_size=%d\n",num_of_headers,i,chunk_id&0xff,(chunk_id>>8)&0xff,(chunk_id>>16)&0xff,chunk_id>>24,chunk_size);
		GxStream_Skip(demuxer->stream, 2);	/* version*/

		if (chunk_size < 10) {
			break;	//return;
		}
gxlogd("chunk_id: 0x%08x --> %C%C%C%C\n",chunk_id,chunk_id&0xff,(chunk_id>>8)&0xff,(chunk_id>>16)&0xff,chunk_id>>24);
		switch (chunk_id) {
		case MKTAG('P', 'R', 'O', 'P'):
			/* Properties header*/

			GxStream_Skip(demuxer->stream, 4);	/* max bitrate*/
			GxStream_Skip(demuxer->stream, 4);	/* avg bitrate*/
			GxStream_Skip(demuxer->stream, 4);	/* max packet size*/
			GxStream_Skip(demuxer->stream, 4);	/* avg packet size*/
			GxStream_Skip(demuxer->stream, 4);	/* nb packets*/
			priv->duration = GxStream_ReadDword(demuxer->stream) / 1000;	/* duration*/
			GxStream_Skip(demuxer->stream, 4);	/* preroll*/
			priv->index_chunk_offset = GxStream_ReadDword(demuxer->stream);
			priv->data_chunk_offset = GxStream_ReadDword(demuxer->stream) + 10;
gxlogd("duration=%x index=%x data=%x\n",(uint32_t)priv->duration,priv->index_chunk_offset,priv->data_chunk_offset);
			GxStream_Skip(demuxer->stream, 2);	/* nb streams*/
			{
				int flags = GxStream_ReadWord(demuxer->stream);

				if (flags) {
					gxlogf("Flags (%x): ", flags);
					if (flags & 0x1)
						gxlogf("[save allowed] ");
					if (flags & 0x2)
						gxlogf("[perfect play (more buffers)] ");
					if (flags & 0x4)
						gxlogf("[live broadcast] ");
					gxlogf("\n");
				}
			}
			break;
		case MKTAG('C', 'O', 'N', 'T'):
			{
				/* Content description header*/
				uint8_t * buf;
				int len;

				len = GxStream_ReadWord(demuxer->stream);
				if (len > 0) {
					buf = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, buf, len);
					buf[len] = 0;
					gxlogf("name :%s\n", (const char*)buf);
					av_free(buf);
				}

				len = GxStream_ReadWord(demuxer->stream);
				if (len > 0) {
					buf = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, buf, len);
					buf[len] = 0;
					gxlogf("author :%s\n", (const char*)buf);
					av_free(buf);
				}

				len = GxStream_ReadWord(demuxer->stream);
				if (len > 0) {
					buf = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, buf, len);
					buf[len] = 0;
					gxlogf("copyright :%s\n", (const char*)buf);
					av_free(buf);
				}

				len = GxStream_ReadWord(demuxer->stream);
				if (len > 0) {
					buf = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, buf, len);
					buf[len] = 0;
					gxlogf("comment :%s\n", (const char*)buf);
					av_free(buf);
				}
				break;
			}
		case MKTAG('M', 'D', 'P', 'R'):
			{
				/* Media properties header*/
				int stream_id;
				int bitrate;
				int codec_data_size;
				int codec_pos;
				int tmp;
				int len;
				uint8_t* descr;
				char* mimet = NULL;

				stream_id = GxStream_ReadWord(demuxer->stream);

				GxStream_Skip(demuxer->stream, 4);	/* max bitrate*/
				bitrate = GxStream_ReadDword(demuxer->stream);	/* avg bitrate*/
				GxStream_Skip(demuxer->stream, 4);	/* max packet size*/
				GxStream_Skip(demuxer->stream, 4);	/* avg packet size*/
				GxStream_Skip(demuxer->stream, 4);	/* start time*/
				GxStream_Skip(demuxer->stream, 4);	/* preroll*/
				GxStream_Skip(demuxer->stream, 4);	/* duration*/


//gxlogd ("stream_id=%d, bitrate=%d\n",stream_id,bitrate);
				if ((len = GxStream_ReadChar(demuxer->stream)) > 0) {
					descr = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, descr, len);
					descr[len] = 0;
					av_free(descr);
				}
				if ((len = GxStream_ReadChar(demuxer->stream)) > 0) {
					mimet = av_malloc(len + 1);
					GxStream_Read(demuxer->stream, (uint8_t*)mimet, len);
					mimet[len] = 0;
				}

				/* Type specific header*/
				codec_data_size = GxStream_ReadDword(demuxer->stream);
				codec_pos = GxStream_Tell(demuxer->stream);
//gxlogd ("mimet=%s\n",mimet);
				if (!strncmp(mimet, "audio/", 6)) {
					if (strstr(mimet, "x-pn-realaudio")
					    || strstr(mimet, "x-pn-multirate-realaudio")) {
						tmp = GxStream_ReadDword(demuxer->stream);
						if (tmp == MKTAG(0xfd, 'a', 'r', '.')) {
							/* audio header*/
							GxStreamAudioHeader* sh =
							    GxStreamHeader_AudioNew(demuxer, stream_id, stream_id);
							char buf[128];	/* for codec name*/
							int frame_size = 0;
							int sub_packet_size = 0;
							int sub_packet_h = 0;
							int version;
							int flavor;
							int coded_frame_size = 0;
							int codecdata_length;
							int i;
							char* buft;
							int hdr_size;
							int bytes_sec=0;

							version = GxStream_ReadWord(demuxer->stream);
							if (version == 3) {
								GxStream_Skip(demuxer->stream, 2);
								GxStream_Skip(demuxer->stream, 10);
								GxStream_Skip(demuxer->stream, 4);
								// Name, author, (c) are also in CONT tag
								if ((i = GxStream_ReadChar(demuxer->stream)) != 0) {
									buft = av_malloc(i + 1);
									GxStream_Read(demuxer->stream, (uint8_t*)buft, i);
									buft[i] = 0;
									gxlogf("Name :%s\n", buft);
									av_free(buft);
								}
								if ((i = GxStream_ReadChar(demuxer->stream)) != 0) {
									buft = av_malloc(i + 1);
									GxStream_Read(demuxer->stream, (uint8_t*)buft, i);
									buft[i] = 0;
									gxlogf("Author :%s\n", buft);
									av_free(buft);
								}
								if ((i = GxStream_ReadChar(demuxer->stream)) != 0) {
									buft = av_malloc(i + 1);
									GxStream_Read(demuxer->stream, (uint8_t*)buft, i);
									buft[i] = 0;
									gxlogf("Copyright :%s\n", buft);
									av_free(buft);
								}
								GxStream_Skip(demuxer->stream, 1);
								i = GxStream_ReadChar(demuxer->stream);
								sh->format = GxStream_ReadDword_Le(demuxer->stream);
								if (i != 4) {
									GxStream_Skip(demuxer->stream, i - 4);
								}
								sh->channels = 1;
								sh->samplesize = 16;
								sh->samplerate = 8000;
								frame_size = 240;
								strncpy(buf, "14_4", sizeof(buf)-1);
							} else {
								GxStream_Skip(demuxer->stream, 2);	// 00 00
								GxStream_Skip(demuxer->stream, 4);	/* .ra4 or .ra5*/
								GxStream_Skip(demuxer->stream, 4);	// ???
								GxStream_Skip(demuxer->stream, 2);	/* version (4 or 5)*/
								hdr_size = GxStream_ReadDword(demuxer->stream);	// header size
								flavor = GxStream_ReadWord(demuxer->stream);	/* codec flavor id*/
								coded_frame_size = GxStream_ReadDword(demuxer->stream);	/* needed by codec*/
								GxStream_Skip(demuxer->stream, 4);
								bytes_sec = GxStream_ReadDword(demuxer->stream);
								GxStream_Skip(demuxer->stream, 4);
								sub_packet_h = GxStream_ReadWord(demuxer->stream);
								frame_size = GxStream_ReadWord(demuxer->stream);
								sub_packet_size = GxStream_ReadWord(demuxer->stream);
								GxStream_Skip(demuxer->stream, 2);	// 0
								if (version == 5)
									GxStream_Skip(demuxer->stream, 6);	//0,srate,0

								sh->samplerate = GxStream_ReadWord(demuxer->stream);
								GxStream_Skip(demuxer->stream, 2);	// 0
								sh->samplesize = GxStream_ReadWord(demuxer->stream) / 8;
								sh->channels = GxStream_ReadWord(demuxer->stream);

								if (version == 5) {
									GxStream_Read(demuxer->stream, (uint8_t*)buf, 4);	// interleaver id
									priv->intl_id[stream_id] =
									    MKTAG(buf[0], buf[1], buf[2], buf[3]);
									GxStream_Read(demuxer->stream, (uint8_t*)buf, 4);	// fourcc
									buf[4] = 0;
								} else {
									/* Interleaver id*/
									get_str(1, demuxer, buf, sizeof(buf));
									priv->intl_id[stream_id] =
									    MKTAG(buf[0], buf[1], buf[2], buf[3]);
									/* Codec FourCC*/
									get_str(1, demuxer, buf, sizeof(buf));
								}
							}

							/* Emulate WAVEFORMATEX struct:*/
							sh->wf = av_malloc(sizeof(WAVEFORMATEX));
							memset(sh->wf, 0, sizeof(WAVEFORMATEX));
							sh->wf->nChannels = sh->channels;
							sh->wf->wBitsPerSample = sh->samplesize*  8;
							sh->wf->nSamplesPerSec = sh->samplerate;
							sh->wf->nAvgBytesPerSec = bitrate / 8;
							sh->wf->nBlockAlign = frame_size;
							sh->wf->cbSize = 0;
							sh->format = MKTAG(buf[0], buf[1], buf[2], buf[3]);
							switch (sh->format) {
							case MKTAG('d', 'n', 'e', 't'):
//                          sh->format = 0x2000;
								break;
							case MKTAG('1', '4', '_', '4'):
								sh->wf->nBlockAlign = 0x14;
								break;

							case MKTAG('2', '8', '_', '8'):
								sh->wf->nBlockAlign = coded_frame_size;
								break;

							case MKTAG('s', 'i', 'p', 'r'):
							case MKTAG('a', 't', 'r', 'c'):
							case MKTAG('c', 'o', 'o', 'k'):
								{
									uint32_t block_duration = 0;
									uint32_t coded_frame_num = 0;
									uint32_t frame_per_block = 0;
									uint32_t num_samples = 0;
									uint32_t num_regions = 0;
									uint32_t cplStart = 0;
									uint32_t cplQBits = 0;
									uint8_t* pOpaqueData;
									input_info_header* para_cook;
								// realaudio codec plugins - common:
									GxStream_Skip(demuxer->stream, 3);	// Skip 3 unknown bytes
									if (version == 5)
										GxStream_Skip(demuxer->stream, 1);	// Skip 1 additional unknown byte
									codecdata_length = GxStream_ReadDword(demuxer->stream);
								// Check extradata len, we can't store bigger values in cbSize anyway
									if ((unsigned)codecdata_length > 0xffff) {
										goto skip_this_chunk;
									}
									sh->wf->cbSize = codecdata_length;
									sh->wf = av_realloc(sh->wf,sizeof(WAVEFORMATEX) + sh->wf->cbSize);
									GxStream_Read(demuxer->stream, ((uint8_t*)(sh->wf + 1)), codecdata_length);	// extras
									int i;
									for(i=0;i<sh->wf->cbSize;i++)
										gxlogd("%02x ", ((uint8_t*)(sh->wf + 1))[i]);
									gxlogd("\n");
									if(priv->intl_id[stream_id] == MKTAG('g', 'e', 'n', 'r'))
										sh->wf->nBlockAlign = sub_packet_size;
									else
										sh->wf->nBlockAlign = coded_frame_size;
									sh->format = GX_FOURCC('c', 'o', 'o', 'k');
									strncpy(sh->priv.codec ,"cook", sizeof(sh->priv.codec)-1);
									sh->priv.para.data = av_malloc(sizeof(input_info_header));
									sh->priv.para.len  = sizeof(input_info_header);
									memset(sh->priv.para.data,0,sizeof(input_info_header));
									para_cook = (input_info_header*)sh->priv.para.data;
									if(bytes_sec)
										block_duration = ((double) coded_frame_size)*60000.0/((double)bytes_sec);
									if(sub_packet_size)
									{
										coded_frame_num = sub_packet_h*coded_frame_size/sub_packet_size;
										frame_per_block = coded_frame_size/sub_packet_size;
									}
									para_cook->ulSuperBlockSize = bswap32(sub_packet_h*frame_size);
									para_cook->dBlockDuration_inte = bswap32(block_duration);
									para_cook->dBlockDuration_frac = 0x0;
									para_cook->ulInterleaveFactor = bswap32(sub_packet_h);
									para_cook->ulInterleaveBlockSize = bswap32(coded_frame_size);
									para_cook->ulInterleaverID = priv->intl_id[stream_id];
									para_cook->ulCodecFrameSize = bswap32(sub_packet_size);
									para_cook->ulNumCodecFrames = bswap32(coded_frame_num);
									para_cook->rule2Flag[0] = bswap32(0x2);
									para_cook->rule2Flag[1] = 0x0;
									para_cook->sampRate = bswap32(sh->samplerate);
									para_cook->nChannels = bswap32(sh->channels);
									pOpaqueData = (uint8_t*)(sh->wf + 1);
									num_samples = ((pOpaqueData[4]<<8)&0xff00)|pOpaqueData[5];
									num_regions = ((pOpaqueData[6]<<8)&0xff00)|pOpaqueData[7];
									if((pOpaqueData[0]>=1) && (pOpaqueData[3]>=3)){
										cplStart = ((pOpaqueData[12]<<8)&0xff00)|pOpaqueData[13];
										cplQBits = ((pOpaqueData[14]<<8)&0xff00)|pOpaqueData[15];
									}else{
										cplStart = 0;
										cplQBits = 0;
									}
									para_cook->nSamples = bswap32(num_samples);
									para_cook->nRegions = bswap32(num_regions);
									para_cook->cplStart = bswap32(cplStart);
									para_cook->cplQbits = bswap32(cplQBits);
									/*
									*  We don't have a stream header pattern, so
									*  we generate the standard interleave pattern.
									*/
									int ulBlockIndx = 0;
									int ulFrameIndx = 0;
									if (sub_packet_h == 1) {
										for (i = 0; i < coded_frame_num; i++) {
											para_cook->pulGENRPattern[i] = bswap32(i);
										}
									} else {
										int bEven = TRUE;
										int ulCount = 0;
										while (ulCount < coded_frame_num) {
											para_cook->pulGENRPattern[ulCount] = bswap32(ulBlockIndx*  frame_per_block + ulFrameIndx);
											ulCount++;
											ulBlockIndx += 2;
											if (ulBlockIndx >= sub_packet_h) {
												if (bEven) {
													bEven = FALSE;
													ulBlockIndx = 1;
												} else {
													bEven = TRUE;
													ulBlockIndx = 0;
													ulFrameIndx++;
												}
											}
										}
									}
									for (ulBlockIndx = 0; ulBlockIndx < sub_packet_h; ulBlockIndx++){
										for (ulFrameIndx = 0; ulFrameIndx < frame_per_block;ulFrameIndx++){
											int ulIndx = ulBlockIndx*  frame_per_block + ulFrameIndx;
										//	gxlogd("ulIndx = %d\n",ulIndx);
											para_cook->pulGENRBlockNum[ulIndx] = bswap32(ulBlockIndx);
											para_cook->pulGENRBlockOffset[ulIndx] = bswap32(ulFrameIndx);
										}
									}
								}
								break;
							case MKTAG('r', 'a', 'a', 'c'):
							case MKTAG('r', 'a', 'c', 'p'):
								{
									/* This is just AAC. The two or five bytes of*/
									/* config data needed for libfaad are stored*/
									/* after the audio headers.*/
									uint32_t block_duration = 0;
									uint32_t block_type = 0;
									input_info_header_aac* para_aac = NULL;

									GxStream_Skip(demuxer->stream, 3);	// Skip 3 unknown bytes
									if (version == 5)
										GxStream_Skip(demuxer->stream, 1);	// Skip 1 additional unknown byte
									codecdata_length = GxStream_ReadDword(demuxer->stream);
									if (codecdata_length >= 1) {
										sh->codecdata_len = codecdata_length - 1;
										sh->codecdata = av_calloc(sh->codecdata_len, 1);
										block_type = GxStream_ReadChar(demuxer->stream);
										GxStream_Read(demuxer->stream, sh->codecdata,
											      sh->codecdata_len);
									}
									sh->format = GX_FOURCC('R', 'A', 'A', 'C');
									strncpy(sh->priv.codec ,"AAC", sizeof(sh->priv.codec)-1);
									sh->priv.para.data = av_malloc(sizeof(input_info_header));
									sh->priv.para.len  = sizeof(input_info_header);
									memset(sh->priv.para.data,0,sizeof(input_info_header));
									para_aac = (input_info_header_aac*)sh->priv.para.data;
									if(bytes_sec)
										block_duration = ((double) coded_frame_size)*60000.0/((double)bytes_sec);
									para_aac->dBlockDuration_inte = bswap32(block_duration);
									para_aac->dBlockDuration_frac = 0x0;
									para_aac->body_type = bswap32(block_type);
									para_aac->nChans    = bswap32(sh->channels) ;
									para_aac->sampRateCore = bswap32(sh->samplerate);
									para_aac->profile = bswap32(AAC_LC);//default ACC_LC or know info
								}
								break;
							default:
								break;
							}

							// Interleaver setup
							priv->sub_packet_size[stream_id] = sub_packet_size;
							priv->sub_packet_h[stream_id] = sub_packet_h;
							priv->coded_framesize[stream_id] = coded_frame_size;
							priv->audiopk_size[stream_id] = frame_size;
							sh->wf->wFormatTag = sh->format;
gxlogd("audio format: 0x%08x --> %C%C%C%C\n",sh->format,sh->format&0xff,(sh->format>>8)&0xff,(sh->format>>16)&0xff,sh->format>>24);
							/* Select audio stream with highest bitrate if multirate file*/
							if ((demuxer->audio->id == -1) ||
										   ((demuxer->audio->id >= 0)
										    && priv->a_bitrate
										    && (bitrate > priv->a_bitrate))) {
								//stream_id = 5;
								demuxer->audio->id = stream_id;
								priv->a_bitrate = bitrate;
							}

							if (demuxer->audio->id == stream_id) {
								sh->ds = demuxer->audio;
								demuxer->audio->sh = sh;
								priv->audio_buf =
								    av_calloc(priv->sub_packet_h[demuxer->audio->id],
									   priv->audiopk_size[demuxer->audio->id]);
								priv->audio_timestamp =
								    av_calloc(priv->sub_packet_h[demuxer->audio->id],
									   sizeof(double));
							}

							++a_streams;

						}
					} else if (strstr(mimet, "X-MP3-draft-00")) {
						GxStreamAudioHeader* sh =
						    GxStreamHeader_AudioNew(demuxer, stream_id, stream_id);

						/* Emulate WAVEFORMATEX struct:*/
						sh->wf = av_malloc(sizeof(WAVEFORMATEX));
						memset(sh->wf, 0, sizeof(WAVEFORMATEX));
						sh->wf->nChannels = 0;	//sh->channels;
						sh->wf->wBitsPerSample = 16;
						sh->wf->nSamplesPerSec = 0;	//sh->samplerate;
						sh->wf->nAvgBytesPerSec = 0;	//bitrate;
						sh->wf->nBlockAlign = 0;	//frame_size;
						sh->wf->cbSize = 0;
						sh->wf->wFormatTag = sh->format = GX_FOURCC('a', 'd', 'u', 0x55);

						if (demuxer->audio->id == stream_id) {
							sh->ds = demuxer->audio;
							demuxer->audio->sh = sh;
						}

						++a_streams;
					} else if (strstr(mimet, "x-ralf-mpeg4")) {
					} else {
						;
					}
				} else if (!strncmp(mimet, "video/", 6)) {
					if (strstr(mimet, "x-pn-realvideo")
					    || strstr(mimet, "x-pn-multirate-realvideo")) {
						GxStream_Skip(demuxer->stream, 4);	// VIDO length, same as codec_data_size
						tmp = GxStream_ReadDword(demuxer->stream);
						if (tmp == MKTAG('O', 'D', 'I', 'V')) {
							/* video header*/
							GxStreamVideoHeader* sh =
							    GxStreamHeader_VideoNew(demuxer, stream_id, stream_id);

							sh->format = GxStream_ReadDword_Le(demuxer->stream);	/* fourcc*/
							/* emulate BITMAPINFOHEADER*/
							sh->bih = av_malloc(sizeof(BITMAPINFOHEADER));
							memset(sh->bih, 0, sizeof(BITMAPINFOHEADER));
							sh->bih->biSize = sizeof(BITMAPINFOHEADER);
							sh->disp_w = sh->bih->biWidth =
							    GxStream_ReadWord(demuxer->stream);
							sh->disp_h = sh->bih->biHeight =
							    GxStream_ReadWord(demuxer->stream);
							sh->bih->biPlanes = 1;
							sh->bih->biBitCount = 12;
							sh->bih->biCompression = sh->format;
							sh->bih->biSizeImage = sh->bih->biWidth*  sh->bih->biHeight * 3;

							sh->fps = (float)GxStream_ReadWord(demuxer->stream);
							if (sh->fps <= 0)
								sh->fps = 24;	// we probably won't even care about fps
							sh->frametime = 1.0f / sh->fps;
							GxStream_Skip(demuxer->stream, 4);
							if (1) {
								int tmp = GxStream_ReadWord(demuxer->stream);
								if (tmp > 0) {
									sh->fps = tmp;
									sh->frametime = 1.0f / sh->fps;
								}
							} else {
								GxStream_ReadWord(demuxer->stream);
							}
							GxStream_Skip(demuxer->stream, 2);
								// read and store codec extradata
							unsigned int cnt =codec_data_size-(GxStream_Tell(demuxer->stream)-codec_pos);
							if (cnt > 0x7fffffff - sizeof(BITMAPINFOHEADER)) {
								cnt = 0;
							} else {
								sh->bih = av_realloc(sh->bih, sizeof(BITMAPINFOHEADER) + cnt);
								sh->bih->biSize += cnt;
								GxStream_Read(demuxer->stream,((unsigned char* )(sh->bih + 1)),cnt);
							}

							if (sh->format == 0x30315652
							    && ((unsigned char* )(sh->bih + 1))[6] == 0x30)
								sh->bih->biCompression = sh->format =
								    GX_FOURCC('R', 'V', '1', '3');
gxlogd("vedio format: 0x%08x --> %C%C%C%C\n",sh->format,sh->format&0xff,(sh->format>>8)&0xff,(sh->format>>16)&0xff,sh->format>>24);
							/* Select video stream with highest bitrate if multirate file*/
							if ((demuxer->video->id == -1) ||
										   ((demuxer->video->id >= 0)
										    && priv->v_bitrate
										    && (bitrate > priv->v_bitrate))) {
								//stream_id = 5;
								demuxer->video->id = stream_id;
								priv->v_bitrate = bitrate;
							}

							if (demuxer->video->id == stream_id) {
								sh->ds = demuxer->video;
								demuxer->video->sh = sh;
								sh->priv.header.data = av_malloc(sizeof(RealPacketVideoHeader)+cnt-2);
								sh->priv.header.len  = sizeof(RealPacketVideoHeader)+cnt-2;
								RealPacketVideoHeader* video_header =sh->priv.header.data;
								video_header->u32Length = bswap32(sizeof(RealPacketVideoHeader)+cnt-2);
								video_header->u32MOFTag = bswap32(0x5649444f);
								video_header->u32SubMOFTag  = sh->format;
								video_header->u16Width  = bswap16(sh->disp_w);
								video_header->u16Height = bswap16(sh->disp_h);
								video_header->u16BitCount = bswap16(sh->bih->biBitCount);
								video_header->u16PadWidth = bswap16((uint16_t)sh->bih->biXPelsPerMeter);
								video_header->u16PadHeight= bswap16((uint16_t)sh->bih->biYPelsPerMeter);
								video_header->u32FramesPerSecond = bswap32((((uint16_t)sh->fps)<<16)|0xffff);
								if(cnt>2)
									memcpy(sh->priv.header.data+sizeof(RealPacketVideoHeader),(uint8_t*)(sh->bih+1),cnt-2);
							}
							++v_streams;
						}
					} else {
					}
				} else if (strstr(mimet, "logical-")) {
					if (strstr(mimet, "fileinfo")) {

						;
					} else if (strstr(mimet, "-audio") || strstr(mimet, "-video")) {
						int i, stream_cnt;
						int *stream_list;
						if (!(stream_list = av_mallocz(sizeof(int)*media_max_streams)))
							goto skip_this_chunk;
						priv->is_multirate = 1;
						GxStream_Skip(demuxer->stream, 4);	// Length of codec data (repeated)
						stream_cnt = GxStream_ReadDword(demuxer->stream);	// Get number of audio or video streams
						if ((unsigned)stream_cnt >= media_max_streams) {
							if (stream_list)
								av_free(stream_list);
							goto skip_this_chunk;
						}
						for (i = 0; i < stream_cnt; i++)
							stream_list[i] = GxStream_ReadWord(demuxer->stream);
						for (i = 0; i < stream_cnt; i++)
							if ((unsigned)stream_list[i] >= media_max_streams) {
								GxStream_Skip(demuxer->stream, 4);	// Skip DATA offset for broken stream
							} else {
								priv->str_data_offset[stream_list[i]] =
								    GxStream_ReadDword(demuxer->stream);
							}
						// Skip the rest of this chunk
						if (stream_list)
							av_freep(&stream_list);
					}

				}
			      skip_this_chunk:
				/* skip codec info*/
				tmp = GxStream_Tell(demuxer->stream) - codec_pos;
				GxStream_Skip(demuxer->stream, codec_data_size - tmp);

				if (mimet)
					av_free(mimet);
				break;
			}
		case MKTAG('D', 'A', 'T', 'A'):
			goto header_end;
		case MKTAG('I', 'N', 'D', 'X'):
		default:
			GxStream_Skip(demuxer->stream, chunk_size - 10);
			break;
		}
	}
gxlogd("%s %d...\n",__FUNCTION__,__LINE__);
      header_end:
	if (priv->is_multirate) {
		/* Perform some sanity checks to avoid checking streams id all over the code*/
		if (demuxer->audio->id >= media_max_streams) {
			demuxer->audio->id = -2;
		} else if ((demuxer->audio->id >= 0) && (priv->str_data_offset[demuxer->audio->id] == 0)) {
			demuxer->audio->id = -2;
		}
		if (demuxer->video->id >= media_max_streams) {
			demuxer->video->id = -2;
		} else if ((demuxer->video->id >= 0) && (priv->str_data_offset[demuxer->video->id] == 0)) {
			demuxer->video->id = -2;
		}
	}

	if (priv->is_multirate && ((demuxer->video->id >= 0) || (demuxer->audio->id >= 0))) {
		/* If audio or video only, seek to right place and behave like standard file*/
		if (demuxer->video->id < 0) {
			// Stream is audio only, or -novideo
			GxStream_Seek(demuxer->stream, priv->data_chunk_offset =
				      priv->str_data_offset[demuxer->audio->id] + 10);
			priv->is_multirate = 0;
		}
		if (demuxer->audio->id < 0) {
			// Stream is video only, or -nosound
			GxStream_Seek(demuxer->stream, priv->data_chunk_offset =
				      priv->str_data_offset[demuxer->video->id] + 10);
			priv->is_multirate = 0;
		}
	}

	if (!priv->is_multirate) {
//    gxlogf("i=%d num_of_headers=%d   \n",i,num_of_headers);
		priv->num_of_packets = GxStream_ReadDword(demuxer->stream);
		GxStream_Skip(demuxer->stream, 4);	/* next data header*/

		if (priv->num_of_packets == 0)
			priv->num_of_packets = -10;
	} else {
		priv->audio_curpos = priv->str_data_offset[demuxer->audio->id] + 18;
		GxStream_Seek(demuxer->stream, priv->str_data_offset[demuxer->audio->id] + 10);
		priv->a_num_of_packets = GxStream_ReadDword(demuxer->stream);
		priv->video_curpos = priv->str_data_offset[demuxer->video->id] + 18;
		GxStream_Seek(demuxer->stream, priv->str_data_offset[demuxer->video->id] + 10);
		priv->v_num_of_packets = GxStream_ReadDword(demuxer->stream);
		priv->stream_switch = 1;
		/* Index required for multirate playback, force building if it's not there*/
		/* but respect user request to force index regeneration*/
		if (demuxer->index_mode == -1)
			demuxer->index_mode = 1;
	}

	priv->audio_need_keyframe = 0;
	priv->video_after_seek = 0;

	switch (demuxer->index_mode) {
	case -1:		// untouched
		if (priv->index_chunk_offset && parse_index_chunk(demuxer)) {
			demuxer->seekable = 1;
		}
		break;
	case 1:		// use (generate index)
		if (priv->index_chunk_offset && parse_index_chunk(demuxer)) {
			demuxer->seekable = 1;
		} else {
			generate_index(demuxer);
			demuxer->seekable = 1;
		}
		break;
	case 2:		// force generating index
		generate_index(demuxer);
		demuxer->seekable = 1;
		break;
	default:		// do nothing
		break;
	}

	if (priv->is_multirate)
		demuxer->seekable = 0;	// No seeking yet for multirate streams
	else
		demuxer->seekable = 1;

	// detect streams:
	if (demuxer->video->id == -1 && v_streams > 0) {
		// find the valid video stream:
		if (GxDemuxStream_FillBuffer(demuxer->video)!= GX_PLAYER_OK) {
			;
		}
	}
	if (demuxer->audio->id == -1 && a_streams > 0) {
		// find the valid audio stream:
		if (GxDemuxStream_FillBuffer(demuxer->audio)!= GX_PLAYER_OK) {
			;
		}
	}

	real_asf_packet = NULL;
	return demuxer;
}

static void demux_close_real(GxDemuxer*  demuxer)
{
	int i;
	DemuxRealPriv* priv = demuxer->priv;
	int media_max_streams = 0;
	GxPlayer_SystemGet(PSYS_MEDIA_MAX_STREAMS, &media_max_streams);

	if (priv) {
		for (i = 0; i < media_max_streams; i++)
			if (priv->index_table[i])
				av_free(priv->index_table[i]);
		if (priv->audio_buf)
			av_free(priv->audio_buf);
		if (priv->audio_timestamp)
			av_free(priv->audio_timestamp);
		if (priv->audiopk_size)
			av_free(priv->audiopk_size);
		if (priv->coded_framesize)
			av_free(priv->coded_framesize);
		if (priv->sub_packet_h)
			av_free(priv->sub_packet_h);
		if (priv->sub_packet_size)
			av_free(priv->sub_packet_size);
		if (priv->intl_id)
			av_free(priv->intl_id);
		if (priv->str_data_offset)
			av_free(priv->str_data_offset);
		if (priv->index_av_malloc_size)
			av_free(priv->index_av_malloc_size);
		if (priv->index_av_malloc_size)
			av_free(priv->index_av_malloc_size);
		if (priv->index_table_size)
			av_free(priv->index_table_size);
		if (priv->index_table)
			av_free(priv->index_table);
		av_free(priv);
		demuxer->priv = NULL;
	}

	return;
}

/* please upload RV10 samples WITH INDEX CHUNK*/
static int demux_seek_real(GxDemuxer *demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
#if 0
	DemuxRealPriv* priv = demuxer->priv;
	GxDemuxStream* d_audio = demuxer->audio;
	GxDemuxStream* d_video = demuxer->video;
	GxStreamAudioHeader* sh_audio = d_audio->sh;
	GxStreamVideoHeader* sh_video = d_video->sh;
	int vid = d_video->id, aid = d_audio->id;
	int next_offset = 0;
	int64_t cur_timestamp = 0;
	int streams = 0;
	int retried = 0;
	int rel_seek_secs = rel_seek_ms/1000;

	gxlogd("%s %d, rel_seek_secs=%d\n",__FUNCTION__,__LINE__,rel_seek_secs);
	if (sh_video && (unsigned)vid < MAX_STREAMS && priv->index_table_size[vid])
		streams |= 1;
	if (sh_audio && (unsigned)aid < MAX_STREAMS && priv->index_table_size[aid])
		streams |= 2;

	if (!streams)
		return;

	if (flags & 1)
		/* seek absolute*/
		priv->current_apacket = priv->current_vpacket = 0;
	if (flags & 2)		// percent seek
		rel_seek_secs*= priv->duration;

	if ((streams & 1) && priv->current_vpacket >= priv->index_table_size[vid])
		priv->current_vpacket = priv->index_table_size[vid] - 1;
	if ((streams & 2) && priv->current_apacket >= priv->index_table_size[aid])
		priv->current_apacket = priv->index_table_size[aid] - 1;

	if (streams & 1) {	// use the video index if we have one
		cur_timestamp = priv->index_table[vid][priv->current_vpacket].timestamp;
		if (rel_seek_secs > 0)
			while ((priv->index_table[vid][priv->current_vpacket].timestamp - cur_timestamp) <
			       rel_seek_secs*  1000) {
				priv->current_vpacket += 1;
				if (priv->current_vpacket >= priv->index_table_size[vid]) {
					priv->current_vpacket = priv->index_table_size[vid] - 1;
					if (!retried) {
						GxStream_Seek(demuxer->stream,
							      priv->index_table[vid][priv->current_vpacket].offset);
						add_index_segment(demuxer, vid, cur_timestamp + rel_seek_secs*  1000);
						retried = 1;
					} else
						break;
				}
		} else if (rel_seek_secs < 0)
			while ((cur_timestamp - priv->index_table[vid][priv->current_vpacket].timestamp) <
			       -rel_seek_secs*  1000) {
				priv->current_vpacket -= 1;
				if (priv->current_vpacket < 0) {
					priv->current_vpacket = 0;
					break;
				}
			}
		next_offset = priv->index_table[vid][priv->current_vpacket].offset;
		priv->audio_need_keyframe = 1;
		priv->video_after_seek = 1;
	} else if (streams & 2) {
		cur_timestamp = priv->index_table[aid][priv->current_apacket].timestamp;
		if (rel_seek_secs > 0)
			while ((priv->index_table[aid][priv->current_apacket].timestamp - cur_timestamp) <
			       rel_seek_secs*  1000) {
				priv->current_apacket += 1;
				if (priv->current_apacket >= priv->index_table_size[aid]) {
					priv->current_apacket = priv->index_table_size[aid] - 1;
					break;
				}
		} else if (rel_seek_secs < 0)
			while ((cur_timestamp - priv->index_table[aid][priv->current_apacket].timestamp) <
			       -rel_seek_secs*  1000) {
				priv->current_apacket -= 1;
				if (priv->current_apacket < 0) {
					priv->current_apacket = 0;
					break;
				}
			}
		next_offset = priv->index_table[aid][priv->current_apacket].offset;
	}

	if (next_offset){
		GxStream_Seek(demuxer->stream, next_offset);
	}
#endif
	if(demuxer->stream)
		GxStream_Reset(demuxer->stream);

	GxStream_SeekTime(demuxer->stream, rel_seek_ms);
	return ret;
}

static int demux_real_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxRealPriv* priv = demuxer->priv;
	unsigned int lastpts = priv->v_pts ? priv->v_pts : priv->a_pts;

	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		if (priv->duration == 0)
		{
			return GX_DEMUXER_CTRL_ERROR;
		}

		*((double* )arg) = (double)priv->duration;
		return GX_DEMUXER_CTRL_OK;

	case GX_DEMUXER_CTRL_GET_PERCENT_POS:
		if (priv->duration == 0)
		{
			return GX_DEMUXER_CTRL_ERROR;
		}

		*((int* )arg) = (int)(100 * lastpts / priv->duration);
		return GX_DEMUXER_CTRL_OK;

	default:
		return GX_DEMUXER_CTRL_NOTIMPL;
	}
}

GxDemuxerClass gx_demux_real = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer REAL",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	DEF_AUTHOR("demuxer","real","No description","L.F","No comment"),

	.name        = "Demux REAL",
	.type        = GX_DEMUXER_TYPE_REAL,
	.check_file  = real_check_file,
	.open        = demux_open_real,
	.fill_buffer = demux_real_fill_buffer,
	.close       = demux_close_real,
	.seek        = demux_seek_real,
	.control     = demux_real_control,
};
