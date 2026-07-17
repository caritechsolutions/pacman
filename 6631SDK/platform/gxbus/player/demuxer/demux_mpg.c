
#include "demux_mpg.h"
#include "gx_common.h"

static off_t ps_probe = 64000;
static int mpeg_pts_error = 0;
static int num_elementary_packets100 = 0;
static int num_elementary_packets101 = 0;
static int num_elementary_packets12x = 0;
static int num_elementary_packets1B6 = 0;
static int num_elementary_packetsPES = 0;
static int num_h264_slice = 0;	//combined slice
static int num_h264_dpa = 0;	//DPA Slice
static int num_h264_dpb = 0;	//DPB Slice
static int num_h264_dpc = 0;	//DPC Slice
static int num_h264_idr = 0;	//IDR Slice
static int num_h264_sps = 0;
static int num_h264_pps = 0;

static int num_mp3audio_packets = 0;
static int num_pcm_header_parsed = 0;

static int parse_psm(GxDemuxer*  demux, int len)
{
	unsigned char c, id, type;
	unsigned int plen, prog_len, es_map_len;
	DemuxMpgPriv* priv = (DemuxMpgPriv *) demux->priv;

	if (!len || len > 1018)
		return 0;

	c = GxStream_ReadChar(demux->stream);
	if (!(c & 0x80)) {
		GxStream_Skip(demux->stream, len - 1);	//not yet valid, discard
		return 0;
	}
	GxStream_Skip(demux->stream, 1);
	prog_len = GxStream_ReadWord(demux->stream);	//length of program descriptors
	GxStream_Skip(demux->stream, prog_len);	//.. that we ignore
	es_map_len = GxStream_ReadWord(demux->stream);	//length of elementary streams map
	es_map_len = GXMIN(es_map_len, len - prog_len - 8);	//sanity check
	while (es_map_len > 0) {
		type = GxStream_ReadChar(demux->stream);
		id = GxStream_ReadChar(demux->stream);
		if (id >= 0xB0 && id <= 0xEF && priv) {
			int idoffset = id - 0xB0;
			switch (type) {
			case 0x1:
				priv->es_map[idoffset] = VIDEO_MPEG1;
				break;
			case 0x2:
				priv->es_map[idoffset] = VIDEO_MPEG2;
				break;
			case 0x3:
			case 0x4:
				priv->es_map[idoffset] = AUDIO_MP2;
				break;
			case 0x0f:
			case 0x11:
				priv->es_map[idoffset] = AUDIO_AAC;
				break;
			case 0x10:
				priv->es_map[idoffset] = VIDEO_MPEG4;
				break;
			case 0x1b:
				priv->es_map[idoffset] = VIDEO_H264;
				break;
			case 0x81:
				priv->es_map[idoffset] = AUDIO_A52;
				break;
			}

		}
		plen = GxStream_ReadWord(demux->stream);	//length of elementary stream descriptors
		plen = GXMIN(plen, es_map_len);	//sanity check
		GxStream_Skip(demux->stream, plen);	//skip descriptors for now
		es_map_len -= 4 + plen;
	}
	GxStream_Skip(demux->stream, 4);	//skip crc32
	return 1;
}

//returns the first pts found within TIME_STAMP_PROBE_LEN bytes after stream_pos in demuxer's stream.
//if no pts is found or an error occurs, -1.0 is returned.
//Packs are freed.
static float read_first_mpeg_pts_at_position(GxDemuxer*  demuxer, off_t stream_pos, off_t offset)
{
	GxStream* s = demuxer->stream;
	DemuxMpgPriv* mpg_d = demuxer->priv;
	float pts = -1.0;	//the pts to return;
	float found_pts1;	//the most recently found pts
	float found_pts2;	//the pts found before found_pts1
	float found_pts3;	//the pts found before found_pts2
	int found = 0;

	if (!mpg_d || stream_pos < 0)
		return pts;

	found_pts3 = found_pts2 = found_pts1 = mpg_d->last_pts;
	GxStream_Seek(s, stream_pos);

	//We look for pts.
	//However, we do not stop at the first found one, as timestamps may reset
	//Therefore, we seek until we found three consecutive
	//pts within MAX_PTS_DIFF_FOR_CONSECUTIVE.

	while (found < 3 && !s->eof && (fabs(found_pts2 - found_pts1) < MAX_PTS_DIFF_FOR_CONSECUTIVE)
	       && (fabs(found_pts3 - found_pts2) < MAX_PTS_DIFF_FOR_CONSECUTIVE)
	       && (GxStream_Tell(s) < stream_pos + offset)
	       && GxDemuxStream_FillBuffer(demuxer->video)== GX_PLAYER_OK)
    {
		if (mpg_d->last_pts != found_pts1) {
			if (!found)
				found_pts3 = found_pts2 = found_pts1 = mpg_d->last_pts;	//the most recently found pts
			else {
				found_pts3 = found_pts2;
				found_pts2 = found_pts1;
				found_pts1 = mpg_d->last_pts;
			}
			found++;
		}
	}

	//if (found == 3)
		pts = found_pts3;

	//clean up from searching of first pts;
	GxDemuxStream_FreePacks(demuxer->audio);
	GxDemuxStream_FreePacks(demuxer->video);
	GxDemuxStream_FreePacks(demuxer->sub);

	return pts;
}

/// Open an mpg physical stream
static GxDemuxer* demux_mpg_open(GxDemuxer * demuxer)
{
	GxStream* s = demuxer->stream;
	DemuxMpgPriv* mpg_d;

	if (GxDemuxStream_FillBuffer(demuxer->video) != GX_PLAYER_OK)
		return NULL;
	mpg_d = av_calloc(1, sizeof(DemuxMpgPriv));
	if (mpg_d) {
		demuxer->priv = mpg_d;
		mpg_d->last_pts = -1.0;
		mpg_d->first_pts = -1.0;
		GxParseES_Init(&mpg_d->es_parser, demuxer->video);

		//if seeking is allowed set has_valid_timestamps if appropriate
		if (demuxer->seekable
		    && (demuxer->stream->file_format == GX_STREAMTYPE_FILE)
		    && demuxer->movi_start != demuxer->movi_end) {
			//We seek to the beginning of the stream, to somewhere in the
			//middle, and to the end of the stream, while remembering the pts
			//at each of the three positions. With these pts, we check whether
			//or not the pts are "linear enough" to justify seeking by the pts
			//of the stream

			//The position where the stream is now
			off_t pos = GxStream_Tell(s);
			float first_pts = read_first_mpeg_pts_at_position(demuxer, demuxer->movi_start, TIMESTAMP_PROBE_LEN*5);
			demuxer->start_pts = (int32_t)first_pts;
			if (first_pts != -1.0) {
				float middle_pts = read_first_mpeg_pts_at_position(demuxer,(demuxer->movi_end + demuxer->movi_start)/2, TIMESTAMP_PROBE_LEN*5);
				if (middle_pts != -1.0) {
					float final_pts = read_first_mpeg_pts_at_position(demuxer,demuxer->movi_end-TIMESTAMP_PROBE_LEN, TIMESTAMP_PROBE_LEN);
					if (final_pts != -1.0) {

                        //  find PTS from movi_end, mimus 100K every time
                        int counter = 2;
						// found proper first, middle, and final pts.
						float proportion = (middle_pts - first_pts ==0) ? -1 : (final_pts - middle_pts) / (middle_pts - first_pts);
						// if they are linear enough set has_valid_timestamps
                        while(counter < 10 && ((proportion < 0.5) || (proportion > 2)))
                        {
                            final_pts  =
                                read_first_mpeg_pts_at_position(demuxer, demuxer->movi_end-TIMESTAMP_PROBE_LEN*counter, TIMESTAMP_PROBE_LEN*counter);
                            proportion =
                                (middle_pts - first_pts ==0) ? -1 : (final_pts - middle_pts) / (middle_pts - first_pts);

                            counter ++;
                            demuxer->stream->eof = 0;
                            demuxer->video->eof = 0;
                        }
						if ((0.5 <= proportion) && (proportion <= 2)) {
							mpg_d->first_pts = first_pts;
							mpg_d->first_to_final_pts_len = final_pts - first_pts;
							mpg_d->has_valid_timestamps = 1;
						}
					}
				}
			}
			//Cleaning up from seeking in stream
			demuxer->stream->eof = 0;
			demuxer->video->eof = 0;
			demuxer->audio->eof = 0;

			GxStream_Seek(s, pos);
			GxDemuxStream_FillBuffer(demuxer->video);
		}		// if ( demuxer->seekable )
	}			// if ( mpg_d )

    mpg_d->lastseekpts = demuxer->start_pts;
    mpg_d->lastseekpos = demuxer->movi_start;
    mpg_d->bps = (demuxer->movi_end - demuxer->movi_start) / mpg_d->first_to_final_pts_len;
	return demuxer;
}

static void demux_mpg_close(GxDemuxer*  demuxer)
{
	DemuxMpgPriv* mpg_d = demuxer->priv;
	if (mpg_d)
		av_free(mpg_d);
}

static unsigned long long read_mpeg_timestamp(GxStream*  s, int c)
{
	unsigned int d, e;
	unsigned long long pts;
	d = GxStream_ReadWord(s);
	e = GxStream_ReadWord(s);
	if (((c & 1) != 1) || ((d & 1) != 1) || ((e & 1) != 1)) {
		++mpeg_pts_error;
		return 0;	// invalid pts
	}
	pts = (((uint64_t) ((c >> 1) & 7)) << 30) | ((d >> 1) << 15) | (e >> 1);

	return pts;
}

static void new_audio_stream(GxDemuxer*  demux, int aid)
{
	if (!demux->a_streams[aid]) {
		DemuxMpgPriv* mpg_d = (DemuxMpgPriv *) demux->priv;
		GxStreamAudioHeader* sh_a;
		GxStreamHeader_AudioNew(demux, aid, aid);
		sh_a = (GxStreamAudioHeader* ) demux->a_streams[aid];
		switch (aid & 0xE0) {	// 1110 0000 b  (high 3 bit: type  low 5: id)
		case 0x00:
			sh_a->format = 0x50;
			break;	// mpeg
		case 0xA0:
			sh_a->format = 0x10001;
			break;	// dvd pcm
		case 0x80:
			if ((aid & 0xF8) == 0x88)
				sh_a->format = 0x2001;	//dts
			else
				sh_a->format = 0x2000;
			break;	// ac3
		}
		//evo files
		if ((aid & 0xC0) == 0xC0)
			sh_a->format = 0x2000;
		else if (aid >= 0x98 && aid <= 0x9f)
			sh_a->format = 0x2001;
		if (mpg_d)
			mpg_d->a_stream_ids[mpg_d->num_a_streams++] = aid;
	}
	if (demux->audio->id == -1)
		demux->audio->id = aid;
}

static int parse_pcm_header(GxStreamAudioHeader *sh)
{
/* DVD PCM Audio:*/
    sh->i_bps = 0;
    if(sh->codecdata_len==3){
	// we have LPCM header:
	unsigned char h=sh->codecdata[1];
	sh->channels=1+(h&7);
	switch((h>>4)&3){
	case 0: sh->samplerate=48000;break;
	case 1: sh->samplerate=96000;break;
	case 2: sh->samplerate=44100;break;
	case 3: sh->samplerate=32000;break;
	}
	switch ((h >> 6) & 3) {
	  case 0:
	    sh->sample_format = 1;
	    sh->samplesize = 2;
	    break;
	  case 1:
	    sh->i_bps = sh->channels * sh->samplerate * 5 / 2;
	  case 2:
	    sh->sample_format = 1;
	    sh->samplesize = 3;
	    break;
	  default:
	    sh->sample_format = 1;
	    sh->samplesize = 2;
		break;
	}
    } else {
	// use defaults:
	sh->channels=2;
	sh->samplerate=48000;
	sh->sample_format = 1;
	sh->samplesize = 16;
    }
    if (!sh->i_bps)
    sh->i_bps = sh->samplesize * sh->channels * sh->samplerate;
    return 1;
}

static int demux_mpg_read_packet(GxDemuxer*  demux, int id)
{
	int d;
	int len;
	int set_pts = 0;	// !=0 iff pts has been set to a proper value
	unsigned char c = 0;
	unsigned long long pts = 0;
	unsigned long long dts = 0;
	int l;
	int pes_ext2_subid = -1;
	//int32_t stream_pts = GX_NOPTS_VALUE;
	GxDemuxStream* ds = NULL;
	GxDemuxPacket* dp;
	DemuxMpgPriv* priv = (DemuxMpgPriv *) demux->priv;

	if ((id < 0x1BC || id >= 0x1F0) && id != 0x1FD)
		return -1;
	if (id == 0x1BE)
		return -1;	// padding stream
	if (id == 0x1BF)
		return -1;	// private2

	len = GxStream_ReadWord(demux->stream);
	if (len == 0 || len > MAX_PS_PACKETSIZE) {
		return -2;	// invalid packet !!!!!!
	}

	mpeg_pts_error = 0;

	if (id == 0x1BC) {
		parse_psm(demux, len);
		return 0;
	}

	while (len > 0) {	// Skip stuFFing bytes
		c = GxStream_ReadChar(demux->stream);
		--len;
		if (c != 0xFF)
			break;
	}
	if ((c >> 6) == 1) {	// Read (skip) STD scale & size value
		d = ((c & 0x1F) << 8) | GxStream_ReadChar(demux->stream);
		len -= 2;
		c = GxStream_ReadChar(demux->stream);
	}
	// Read System-1 stream timestamps:
	if ((c >> 4) == 2) {
		pts = read_mpeg_timestamp(demux->stream, c);
		set_pts = 1;
		len -= 4;
	} else if ((c >> 4) == 3) {
		pts = read_mpeg_timestamp(demux->stream, c);
		c = GxStream_ReadChar(demux->stream);
		if ((c >> 4) != 1)
			pts = 0;	//gxlogf("{ERROR4}");
		else
			set_pts = 1;
		dts = read_mpeg_timestamp(demux->stream, c);
		len -= 4 + 1 + 4;
	} else if ((c >> 6) == 2) {
		int pts_flags;
		int hdrlen;
		int parse_ext2;
		// System-2 (.VOB) stream:
		c = GxStream_ReadChar(demux->stream);
		pts_flags = c >> 6;
		parse_ext2 = (id == 0x1FD) && ((c & 0x3F) == 1);
		c = GxStream_ReadChar(demux->stream);
		hdrlen = c;
		len -= 2;
		if (hdrlen > len) {
			return -1;
		}
		if (pts_flags == 2 && hdrlen >= 5) {
			c = GxStream_ReadChar(demux->stream);
			pts = read_mpeg_timestamp(demux->stream, c);
			set_pts = 1;
			len -= 5;
			hdrlen -= 5;
		} else if (pts_flags == 3 && hdrlen >= 10) {
			c = GxStream_ReadChar(demux->stream);
			pts = read_mpeg_timestamp(demux->stream, c);
			set_pts = 1;
			c = GxStream_ReadChar(demux->stream);
			dts = read_mpeg_timestamp(demux->stream, c);
			len -= 10;
			hdrlen -= 10;
		}
		len -= hdrlen;
		if (parse_ext2 && hdrlen >= 3) {
			c = GxStream_ReadChar(demux->stream);
			hdrlen--;

			if ((c & 0x0F) != 0x0F) {
				return -1;
			}
			if (c & 0x80) {	//pes_private_data_flag
				if (hdrlen < 16) {
					return -1;
				}
				GxStream_Skip(demux->stream, 16);
				hdrlen -= 16;
			}
			if (c & 0x40) {	//pack_header_field_flag
				int l = GxStream_ReadChar(demux->stream);
				if (l < 0)	//couldn't read from the stream?
					return -1;
				hdrlen--;
				if (l < 0 || hdrlen < l) {
					return -1;
				}
				GxStream_Skip(demux->stream, l);
				hdrlen -= l;
			}
			if (c & 0x20) {	//program_packet_sequence_counter_flag
				if (hdrlen < 2) {
					return -1;
				}
				GxStream_Skip(demux->stream, 2);
				hdrlen -= 2;
			}
			if (c & 0x10) {
				//STD
				GxStream_Skip(demux->stream, 2);
				hdrlen -= 2;
			}
			c = GxStream_ReadChar(demux->stream);	//pes_extension2 flag
			hdrlen--;
			if (c != 0x81) {
				return -1;
			}
			c = GxStream_ReadChar(demux->stream);	//pes_extension2 payload === substream id
			hdrlen--;
			if (c < 0x55 || c > 0x5F) {
				return -1;
			}
			pes_ext2_subid = c;
		}
		if (hdrlen > 0)
			GxStream_Skip(demux->stream, hdrlen);	// skip header and stuffing bytes

		if (id == 0x1FD && pes_ext2_subid != -1) {
			//==== EVO VC1 STREAMS ===//
			if (!demux->v_streams[pes_ext2_subid])
				GxStreamHeader_VideoNew(demux, pes_ext2_subid, pes_ext2_subid);
			if (demux->video->id == -1)
				demux->video->id = pes_ext2_subid;
			if (demux->video->id == pes_ext2_subid) {
				ds = demux->video;
				if (!ds->sh)
					ds->sh = demux->v_streams[pes_ext2_subid];
				if (priv && ds->sh) {
					GxStreamVideoHeader* sh = (GxStreamVideoHeader *) ds->sh;
					sh->format = GX_FOURCC('W', 'V', 'C', '1');
				}
			}
		}
		//============== DVD Audio sub-stream ======================
		if (id == 0x1BD) {
			int aid, rawa52 = 0;
			off_t tmppos;
			unsigned int tmp;

			tmppos = GxStream_Tell(demux->stream);
			tmp = GxStream_ReadWord(demux->stream);
			GxStream_Seek(demux->stream, tmppos);
			/// vdr stores A52 without the 4 header bytes, so we have to check this condition first
			if (tmp == 0x0B77) {
				aid = 128;
				rawa52 = 1;
			} else {
				aid = GxStream_ReadChar(demux->stream);
				--len;
				if (len < 3)
					return -1;	// invalid audio packet
			}

			// AID:
			// 0x20..0x3F  subtitle
			// 0x80..0x87 and 0xC0..0xCF  AC3 audio
			// 0x88..0x8F and 0x98..0x9F  DTS audio
			// 0xA0..0xBF  PCM audio

			if ((aid & 0xE0) == 0x20) {
				// subtitle:
				aid &= 0x1F;

				if (!demux->s_streams[aid]) {
					GxStreamSubHeader* sh = GxStreamHeader_SubNew(demux, aid, aid);
					if (sh)
						sh->type = 'v';
				}

				if (demux->sub->id > -1)
					demux->sub->id &= 0x1F;
				if (!demux->sub_lang && demux->sub->id == -1)
					demux->sub->id = aid;
				if (demux->sub->id == aid) {
					ds = demux->sub;
				}
			} else if ((aid >= 0x80 && aid <= 0x8F) || (aid >= 0x98 && aid <= 0xAF)
				   || (aid >= 0xC0 && aid <= 0xCF)) {

//        aid=128+(aid&0x7F);
				// aid=0x80..0xBF
				new_audio_stream(demux, aid);
				if (demux->audio->id == aid) {
					int type;
					ds = demux->audio;
					if (!ds->sh)
						ds->sh = demux->a_streams[aid];
					// READ Packet: Skip additional audio header data:
					if (!rawa52) {
						c = GxStream_ReadChar(demux->stream);	//num of frames
						type = GxStream_ReadChar(demux->stream);	//startpos hi
						type = (type << 8) | GxStream_ReadChar(demux->stream);	//startpos lo
						len -= 3;
					}
					if ((aid & 0xE0) == 0xA0 && len >= 3) {
						unsigned char* hdr;
						// save audio header as codecdata!
						if (!((GxStreamAudioHeader* ) (ds->sh))->codecdata_len) {
							((GxStreamAudioHeader* ) (ds->sh))->codecdata = av_malloc(3);
							((GxStreamAudioHeader* ) (ds->sh))->codecdata_len = 3;
						}
						hdr = ((GxStreamAudioHeader* ) (ds->sh))->codecdata;
						// read LPCM header:
						// emphasis[1], mute[1], rvd[1], frame number[5]:
						hdr[0] = GxStream_ReadChar(demux->stream);
						// quantization[2],freq[2],rvd[1],channels[3]
						hdr[1] = GxStream_ReadChar(demux->stream);
						((GxStreamAudioHeader*)(ds->sh))->channels = hdr[1]&0x7;
						// dynamic range control (0x80=off):
						hdr[2] = GxStream_ReadChar(demux->stream);
						len -= 3;

						if(!num_pcm_header_parsed)
						{
							parse_pcm_header(ds->sh);
							num_pcm_header_parsed = 1;
						}
					}
				}	//  if(demux->audio->id==aid)

			}
		}		//if(id==0x1BD)
	} else {
		if (c != 0x0f) {
			return -1;	// invalid packet !!!!!!
		}
	}

	if (len <= 0 || len > MAX_PS_PACKETSIZE) {
		return -1;	// invalid packet !!!!!!
	}

	if (id >= 0x1C0 && id <= 0x1DF) {
		// mpeg audio
		int aid = id - 0x1C0;
		new_audio_stream(demux, aid);
		if (demux->audio->id == aid) {
			ds = demux->audio;
			if (!ds->sh)
				ds->sh = demux->a_streams[aid];
			if (priv && ds->sh) {
				GxStreamAudioHeader* sh = (GxStreamAudioHeader *) ds->sh;
				if (priv->es_map[id - 0x1B0])
					sh->format = priv->es_map[id - 0x1B0];
			}
		}
	} else if (id >= 0x1E0 && id <= 0x1EF) {
		// mpeg video
		int aid = id - 0x1E0;
		if (!demux->v_streams[aid])
			GxStreamHeader_VideoNew(demux, aid, aid);
		if (demux->video->id == -1)
			demux->video->id = aid;
		if (demux->video->id == aid) {
			ds = demux->video;
			if (!ds->sh)
				ds->sh = demux->v_streams[aid];
			if (priv && ds->sh) {
				GxStreamVideoHeader* sh = (GxStreamVideoHeader *) ds->sh;
				if (priv->es_map[id - 0x1B0]) {
					sh->format = priv->es_map[id - 0x1B0];
				}
			}
		}
	}

	if (ds)
	{
		GxStreamVideoHeader* sh = (GxStreamVideoHeader *) ds->sh;
		//TODO liufei
		dp = GxDemuxPacket_Create(demux, NULL, len);
		if (!dp) {
			GxStream_Skip(demux->stream, len);
			return 0;
		}

		l = GxStream_Read(demux->stream, dp->buffer, len);
		if (l < len)
			dp = GxDemuxPacket_Resize(demux, dp, l);

		// mpeg1 only 1 sqh,seek bug
		if(!sh->priv.header.data && dp->buffer[0] == 0 && dp->buffer[1] == 0 && dp->buffer[2] == 1 && dp->buffer[3] == 0xb3)
		{
			int i;
			char* hdr;
			sh->priv.header.data = av_malloc(l);
			sh->priv.header.len = l;
			memcpy(sh->priv.header.data,dp->buffer,l);
			hdr = (char*)sh->priv.header.data+4;
			for(i=0;i<l;i++)
			{
				if(hdr[i] == 0 && hdr[i+1] == 0 && hdr[i+2] == 1 && hdr[i+3] != 0xb5)
					break;
			}
			sh->priv.header.len = i+4;
            memcpy(dp->buffer,dp->buffer+sh->priv.header.len,l-sh->priv.header.len);
            dp->len = dp->write_size = (l-sh->priv.header.len);
		}
		len = l;
		dp->pts = pts /90.0f;
		dp->pos = demux->filepos;
		/*
		   workaround:
		   set dp->stream_pts only when feeding the video stream, or strangely interleaved files
		   (such as SWIII) will show strange alternations in the stream time, wildly going
		   back and forth

		if (ds == demux->video
		    && GxStream_Control(demux->stream, GX_STREAM_CTRL_GET_CURRENT_TIME,
					(void* )&stream_pts) != GX_PLAYER_ERROR)
			dp->stream_pts = stream_pts*/;
		GxDemuxStream_AddPacket(ds, dp);
		if (demux->priv && set_pts)
			((DemuxMpgPriv* ) demux->priv)->last_pts = pts /90.0f ;
//    if(ds==demux->sub) parse_dvdsub(ds->last->buffer,ds->last->len);
          return 1;
	}
	if (len <= 2356)
		GxStream_Skip(demux->stream, len);
	return 0;
}

static void clear_stats(void)
{
	num_elementary_packets100 = 0;
	num_elementary_packets101 = 0;
	num_elementary_packets1B6 = 0;
	num_elementary_packets12x = 0;
	num_elementary_packetsPES = 0;
	num_h264_slice = 0;	//combined slice
	num_h264_dpa = 0;	//DPA Slice
	num_h264_dpb = 0;	//DPB Slice
	num_h264_dpc = 0;	//DPC Slice
	num_h264_idr = 0;	//IDR Slice
	num_h264_sps = 0;
	num_h264_pps = 0;
	num_mp3audio_packets = 0;
	num_pcm_header_parsed = 0;
}

//assumes demuxer->synced < 2
static inline void update_stats(int head)
{
	if (head == 0x1B6)
		++num_elementary_packets1B6;
	else if (head == 0x100)
		++num_elementary_packets100;
	else if (head == 0x101)
		++num_elementary_packets101;
	else if (head == 0x1BD || (0x1C0 <= head && head <= 0x1EF))
		num_elementary_packetsPES++;
	else if (head >= 0x120 && head <= 0x12F)
		++num_elementary_packets12x;
	if (head >= 0x100 && head < 0x1B0) {
		if ((head & ~0x60) == 0x101)
			++num_h264_slice;
		else if ((head & ~0x60) == 0x102)
			++num_h264_dpa;
		else if ((head & ~0x60) == 0x103)
			++num_h264_dpb;
		else if ((head & ~0x60) == 0x104)
			++num_h264_dpc;
		else if ((head & ~0x60) == 0x105 && head != 0x105)
			++num_h264_idr;
		else if ((head & ~0x60) == 0x107 && head != 0x107)
			++num_h264_sps;
		else if ((head & ~0x60) == 0x108 && head != 0x108)
			++num_h264_pps;
	}
}

static int demux_mpg_check_file(GxDemuxer*  demuxer)
{
	int pes = 1;
	int tmp;
	off_t tmppos;
	int type = GX_DEMUXER_TYPE_UNKNOWN;

	tmppos = GxStream_Tell(demuxer->stream);
	tmp = GxStream_ReadDword(demuxer->stream);
	if (tmp == 0x1E0 || tmp == 0x1C0) {
		tmp = GxStream_ReadWord(demuxer->stream);
		if (tmp > 1 && tmp <= 2048)
			pes = 0;	// demuxer->synced=3; // PES...
	}
	GxStream_Seek(demuxer->stream, tmppos);

	clear_stats();

	if (demux_mpg_open(demuxer))
		type = GX_DEMUXER_TYPE_MPEG_PS;
	else {
		//MPEG packet stats: p100: 458  p101: 458  PES: 0  MP3: 1103  (.m2v)
		type = GX_DEMUXER_TYPE_MPEG_ES;
		#if 0
		if (num_mp3audio_packets > 50 && num_mp3audio_packets > 2*  num_elementary_packets100
		    && abs(num_elementary_packets100 - num_elementary_packets101) > 2)
			return type;

		// some hack to get meaningfull error messages to our unhappy users:
		if (num_elementary_packets100 >= 2 && num_elementary_packets101 >= 2 &&
		    abs(num_elementary_packets101 + 8 - num_elementary_packets100) < 16) {
			if (num_elementary_packetsPES >= 4
			    && num_elementary_packetsPES >= num_elementary_packets100 - 4) {
				return type;
			}
			type = GX_DEMUXER_TYPE_MPEG_ES;	//  <-- hack is here :)
		} else
			// fuzzy mpeg4-es detection. do NOT enable without heavy testing of mpeg formats detection!
		if (num_elementary_packets1B6 > 3 && num_elementary_packets12x >= 1 &&
			    num_elementary_packetsPES == 0
			    && num_elementary_packets100 <= num_elementary_packets12x && demuxer->synced < 2) {
			type = GX_DEMUXER_TYPE_MPEG4_ES;
		} else
			// fuzzy h264-es detection. do NOT enable without heavy testing of mpeg formats detection!
			if ((num_h264_slice > 3 || (num_h264_dpa > 3 && num_h264_dpb > 3 && num_h264_dpc > 3)) &&
			    /* FIXME num_h264_sps>=1 &&*/ num_h264_pps >= 1 && num_h264_idr >= 1 &&
			    num_elementary_packets1B6 == 0 && num_elementary_packetsPES == 0 && demuxer->synced < 2) {
			type = GX_DEMUXER_TYPE_H264_ES;
		} else {
		}
		#endif
	}
	//FIXME this shouldn't be necessary
	GxStream_Seek(demuxer->stream, tmppos);
	return type;
}

static int demux_mpg_es_fill_buffer(GxDemuxer*  demux,GxDemuxStream* ds)
{
	// Elementary video stream
	int stream_buffer_size = 0;

	if (demux->stream->eof)
		return GX_PLAYER_ERROR;
	demux->filepos = GxStream_Tell(demux->stream);
	GxPlayer_SystemGet(PSYS_STREAM_BUFFER_SIZE, &stream_buffer_size);
	GxDemuxStream_ReadPacket(demux->video, demux->stream, stream_buffer_size, 0, demux->filepos, 0);
	return GX_PLAYER_OK;
}

static int demux_mpg_fill_buffer(GxDemuxer*  demux,GxDemuxStream* ds)
{
	unsigned int head = 0;
	int skipped = 0;
	int max_packs = 256;	// 512kbyte
	int ret = 0;

// System stream
	do {
		demux->filepos = GxStream_Tell(demux->stream);
#if 1
		//lame workaround: this is needed to show the progress bar when playing dvdnav://
		//(ths poor guy doesn't know teh length of the stream at startup)
		demux->movi_end = demux->stream->end_pos;
#endif
		head = GxStream_ReadDword(demux->stream);
		if ((head & 0xFFFFFF00) != 0x100)
        {
			// sync...
			demux->filepos -= skipped;
			while (1)
            {
				int c = GxStream_ReadChar(demux->stream);
				if (c < 0)
					break;	//EOF
				head <<= 8;
				if (head != 0x100)
                {
					head |= c;
					if (audio_check_mp3_header(head))
						++num_mp3audio_packets;
					++skipped;	//++demux->filepos;
					continue;
				}
				head |= c;
				break;
			}
			demux->filepos += skipped;
		}
		if (GxStream_Eof(demux->stream))
			break;
		// sure: head=0x000001XX
		if (demux->synced == 0)
        {
			if (head == 0x1BA)
				demux->synced = 1;	//else
           //if(head==0x1BD || (head>=0x1C0 && head<=0x1EF)) demux->synced=3; // PES?
		}
        else
            if (demux->synced == 1)
            {
			    if (head == 0x1BB || head == 0x1BD || (head >= 0x1C0 && head <= 0x1EF))
                {
				    demux->synced = 2;
				    num_elementary_packets100 = 0;	// requires for re-sync!
				    num_elementary_packets101 = 0;	// requires for re-sync!
			    }
                else
				    demux->synced = 0;
		    }		// else

		if (demux->synced >= 2)
        {
			ret = demux_mpg_read_packet(demux, head);
			if (!ret)
				if (--max_packs == 0)
                {
					demux->stream->eof = 1;
					return GX_PLAYER_ERROR;
				}
			if (demux->synced == 3)
				demux->synced = (ret == 1) ? 2 : 0;	// PES detect
		}
        else
        {
			update_stats(head);
			if (((num_elementary_packets100 > 50 && num_elementary_packets101 > 50) ||
			     (num_elementary_packetsPES > 50)) && skipped > 4000000)
            {
				demux->stream->eof = 1;
				break;
			}
			if (num_mp3audio_packets > 100 && num_elementary_packets100 < 10)
            {
				demux->stream->eof = 1;
				break;
			}
		}
	} while (ret != 1);

	if (demux->stream->eof)
    {
		return GX_PLAYER_ERROR;
	}

	return GX_PLAYER_OK;
}

static int64_t get_newpos(GxDemuxer *demuxer, int64_t newpts)
{
    int64_t flag = 0, newpos = 0;
    int64_t to_start, to_last, to_end;
	DemuxMpgPriv* priv = (DemuxMpgPriv *) demuxer->priv;

    to_start = newpts - demuxer->start_pts;
    to_last  = labs(newpts - priv->lastseekpts);
    to_end   = demuxer->start_pts + (uint64_t)demuxer->duration - newpts;

    flag = (to_start <= to_end) ? (to_start <= to_last ? 1 : 2) : (to_end <= to_last ? 3 : 2);

    switch(flag)
    {
    case 1: // near start
        newpos = demuxer->movi_start + to_start*priv->bps;
        break;
    case 2: // near lastseek
        newpos = priv->lastseekpos + (newpts - priv->lastseekpts)*priv->bps;
        break;
    case 3: // near end
        newpos = demuxer->movi_end - to_end*priv->bps;
        break;
    default:
        break;
    }

    return newpos;
}

static int demux_mpg_seek(GxDemuxer*  demuxer, int64_t rel_seek_ms, int32_t audio_delay, int flags)
{
	int ret = GX_PLAYER_OK;
	GxDemuxStream* d_audio = demuxer->audio;
	GxDemuxStream* d_video = demuxer->video;
	GxStreamAudioHeader* sh_audio = d_audio->sh;
	GxStreamVideoHeader* sh_video = d_video->sh;
	DemuxMpgPriv* mpg_d = (DemuxMpgPriv *) demuxer->priv;
	float oldpts = 0;
	off_t oldpos = demuxer->filepos;
	float newpts = 0;
	off_t newpos = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? demuxer->movi_start : oldpos;
    int precision = 12;

    if(mpg_d->first_to_final_pts_len <= 0.0)
        mpg_d->first_to_final_pts_len = demuxer->duration;

	if (mpg_d)
		oldpts = mpg_d->last_pts;
	newpts = (flags & GX_DEMUXER_SEEK_ABSOLUTE) ? 0.0 : oldpts;

	if (flags & GX_DEMUXER_SEEK_PERCENT)
	{
		if (mpg_d && mpg_d->first_to_final_pts_len > 0.0)
			newpts += mpg_d->first_to_final_pts_len*  rel_seek_ms;
		else
			newpts += rel_seek_ms*  (demuxer->movi_end - demuxer->movi_start) * oldpts / oldpos;
	}
	else
	{
		newpts += demuxer->start_pts;
		newpts += rel_seek_ms;
	}

	if (newpts < 0)
		newpts = 0;

	if (flags & GX_DEMUXER_SEEK_PERCENT)
	{
		// float seek 0..1
		newpos += (demuxer->movi_end - demuxer->movi_start)*  rel_seek_ms;
	}
	else
	{
		// time seek (secs)
		if (mpg_d && mpg_d->has_valid_timestamps)
		{
			if (mpg_d->first_to_final_pts_len > 0.0)
                newpos = get_newpos(demuxer, newpts);
			else if (oldpts > 0.0)
				newpos += rel_seek_ms*  (oldpos - demuxer->movi_start) / oldpts;
		}
		else if (!sh_video || !sh_video->i_bps)	// unspecified or VBR
			newpos += 2324*  75 * rel_seek_ms;	// 174.3 kbyte/sec
		else
		{
			//newpts -= 5000;
			newpos += sh_video->i_bps*(rel_seek_ms/1000);
		}
	}

	int i;
	while (!GxStream_Eof(demuxer->stream))
	{
        if(GxStream_Seek(demuxer->stream, newpos) == GX_PLAYER_ERROR)
        {
            gxlogd("\n----------------------------seek error!!!\n");
            break;
        }

		// re-sync video:
		mpg_d->es_parser.videobuf_code_len = 0;	// reset ES stream buffer

		GxDemuxStream_FillBuffer(d_video);
		if (sh_audio)
        {
			GxDemuxStream_FillBuffer(d_audio);
		}

		while (!GxStream_Eof(demuxer->stream))
        {
			if (!sh_video)
				break;

			mpg_d->es_parser.ds = d_video;
			i = GxParseES_SyncVideoPacket(&mpg_d->es_parser);
			if (sh_video->format == GX_FOURCC('W', 'V', 'C', '1'))
            {
				if (i == 0x10E || i == 0x10F)	//entry point or sequence header
					break;
			}
            else
                if (sh_video->format == 0x10000004)
                {	//mpeg4
				    if (i == 0x1B6)
                    {	//vop (frame) startcode
					    int pos = mpg_d->es_parser.videobuf_len;
					    if (!GxParseES_ReadVideoPacket(&mpg_d->es_parser))
						    break;	// EOF
					    if ((mpg_d->es_parser.videobuffer[pos + 4] & 0x3F) == 0)
						    break;	//I-frame
				    }
			    }
                else
                    if (sh_video->format == 0x10000005)
                    {	//h264
				        if ((i & ~0x60) == 0x105)
					        break;
			        }
                    else
                    {	//default mpeg1/2
				        if (i == 0x1B3 || i == 0x1B8)
					        break;	// found it!
			        }

			if (!i || !GxParseES_SkipVideoPacket(&mpg_d->es_parser))
				break;	// EOF?
		}

		if (!mpg_d)
			break;
		if (!precision || (abs(newpts - mpg_d->last_pts) < 3000) || (mpg_d->last_pts == oldpts))
			break;

		if ((newpos - oldpos)*(mpg_d->last_pts - oldpts) < 0)
        {	// invalid timestamps
			//mpg_d->has_valid_timestamps = 0;
			break;
		}

        precision--;

		//prepare another seek because we are off by more than 0.5s
		if (mpg_d)
        {
			newpos += ((newpts - mpg_d->last_pts)/1000)*(newpos - oldpos)/((mpg_d->last_pts - oldpts)/1000);
			GxDemuxStream_FreePacks(d_audio);
			GxDemuxStream_FreePacks(d_video);
			GxDemuxStream_FreePacks(demuxer->sub);
			demuxer->stream->eof = 0;	// clear eof flag
			d_video->eof = 0;
			d_audio->eof = 0;
		}
	}

    mpg_d->lastseekpts = mpg_d->last_pts;
    mpg_d->lastseekpos = GxStream_Tell(demuxer->stream);
	
	return ret;
}

static int demux_mpg_control(GxDemuxer*  demuxer, int cmd, void* arg)
{
	DemuxMpgPriv* mpg_d = (DemuxMpgPriv *) demuxer->priv;

	switch (cmd) {
	case GX_DEMUXER_CTRL_GET_TIME_LENGTH:
		if (mpg_d && mpg_d->has_valid_timestamps) {
			*((int64_t* )arg) = (int64_t)mpg_d->first_to_final_pts_len;
			return GX_DEMUXER_CTRL_OK;
		}
		else
		{
			int ibps=0;
			GxStreamVideoHeader* sh_v = NULL;
			GxStreamAudioHeader* sh_a = NULL;
			if(demuxer->video && demuxer->video->sh)
			{
				sh_v = demuxer->video->sh;
				if(sh_v->i_bps>0)
				{
					ibps = sh_v->i_bps;
				}
			}
			if(demuxer->audio && demuxer->audio->sh)
			{
				sh_a = demuxer->audio->sh;
				if(sh_a->i_bps)
				{
					ibps += sh_a->i_bps;
				}
			}
			if(ibps > 0)
			{
				*((int64_t* )arg) = (int64_t)demuxer->movi_end*1000/ibps;
				return GX_DEMUXER_CTRL_OK;
			}
		}
		return GX_DEMUXER_CTRL_ERROR;

	case GX_DEMUXER_CTRL_GET_PERCENT_POS:
		if (mpg_d && mpg_d->has_valid_timestamps && mpg_d->first_to_final_pts_len > 0.0) {
			*((int* )arg) =
			    (int)(100*  (mpg_d->last_pts - mpg_d->first_pts) / mpg_d->first_to_final_pts_len);
			return GX_DEMUXER_CTRL_OK;
		}
		return GX_DEMUXER_CTRL_ERROR;

	case GX_DEMUXER_CTRL_SWITCH_AUDIO:
		if (!(mpg_d && mpg_d->num_a_streams > 1 && demuxer->audio && demuxer->audio->sh))
			return GX_DEMUXER_CTRL_ERROR;
		else {
			GxDemuxStream* d_audio = demuxer->audio;
			GxStreamAudioHeader* sh_audio = d_audio->sh;
			GxStreamAudioHeader* sh_a = sh_audio;
			GxDemuxerStreamSwitch* StreamSwitch = arg;

			int i,aid;
			if (!sh_audio || !arg)
				return GX_DEMUXER_CTRL_ERROR;

			aid = StreamSwitch->pid;

			if (aid < 0) {
				for (i = 0; i < mpg_d->num_a_streams; i++) {
					if (d_audio->id == mpg_d->a_stream_ids[i])
						break;
				}
				i = (i + 1) % mpg_d->num_a_streams;
				sh_a = (GxStreamAudioHeader* ) demuxer->a_streams[mpg_d->a_stream_ids[i]];
			} else {
				for (i = 0; i < mpg_d->num_a_streams; i++)
					if (aid == mpg_d->a_stream_ids[i])
						break;
				if (i < mpg_d->num_a_streams)
					sh_a = (GxStreamAudioHeader* ) demuxer->a_streams[*((int *)arg)];
			}
			if (i < mpg_d->num_a_streams && d_audio->id != mpg_d->a_stream_ids[i]) {
				d_audio->id = mpg_d->a_stream_ids[i];
				d_audio->sh = sh_a;
				GxDemuxStream_FreePacks(d_audio);
			}
		}
		return GX_DEMUXER_CTRL_OK;

	default:
		return GX_DEMUXER_CTRL_ERROR;
	}
}

static int demux_mpg_pes_check_file(GxDemuxer*  demuxer)
{
	demuxer->synced = 3;
	return (demux_mpg_check_file(demuxer) == GX_DEMUXER_TYPE_MPEG_PS) ? GX_DEMUXER_TYPE_MPEG_PES : 0;
}

static int demux_mpg_es_check_file(GxDemuxer*  demuxer)
{
	return GX_DEMUXER_TYPE_MPEG_ES;
}

static GxDemuxer* demux_mpg_es_open(GxDemuxer * demuxer)
{
	GxStreamVideoHeader* sh_video = NULL;

	demuxer->audio->sh = NULL;	// ES streams has no audio channel
	demuxer->video->sh = GxStreamHeader_VideoNew(demuxer, 0, 0);	// create dummy video stream header, id=0
	sh_video = demuxer->video->sh;
	sh_video->ds = demuxer->video;

	return demuxer;
}

static GxDemuxer* demux_mpg_ps_open(GxDemuxer * demuxer)
{
	GxStreamAudioHeader* sh_audio;
	GxStreamVideoHeader* sh_video;
	DemuxMpgPriv* mpg_d;

	sh_video = demuxer->video->sh;
	sh_video->ds = demuxer->video;
	mpg_d = (DemuxMpgPriv* ) demuxer->priv;

	if (demuxer->audio->id != -2) {
		if (GxDemuxStream_FillBuffer(demuxer->audio) != GX_PLAYER_OK) {
			demuxer->audio->sh = NULL;
		} else {
			sh_audio = demuxer->audio->sh;
			sh_audio->ds = demuxer->audio;
		}
	}

	if (!sh_video->format && ps_probe > 0) {
		int head;
		off_t pos = GxStream_Tell(demuxer->stream);

		clear_stats();
		do {
			mpg_d->es_parser.ds = demuxer->video;
			head = GxParseES_SyncVideoPacket(&mpg_d->es_parser);
			if (!head)
				break;
			update_stats(head);
			GxParseES_SkipVideoPacket(&mpg_d->es_parser);
		} while (GxStream_Tell(demuxer->stream) < pos + ps_probe && !demuxer->stream->eof);

		GxDemuxStream_FreePacks(demuxer->video);
		demuxer->stream->eof = 0;
		GxStream_Seek(demuxer->stream, pos);

		if (num_elementary_packets1B6 > 3 && num_elementary_packets12x >= 1 &&
		    num_elementary_packets100 <= num_elementary_packets12x)
			sh_video->format = 0x10000004;
		else if ((num_h264_slice > 3 || (num_h264_dpa > 3 && num_h264_dpb > 3 && num_h264_dpc > 3)) &&
			 num_h264_sps >= 1 && num_h264_pps >= 1 && num_h264_idr >= 1 && num_elementary_packets1B6 == 0)
			sh_video->format = 0x10000005;
		else
			sh_video->format = 0x10000002;
	}

	GxDemuxStream_FreePacks(demuxer->video);
	GxDemuxStream_FreePacks(demuxer->audio);
	GxDemuxStream_FreePacks(demuxer->sub);
	GxStream_Seek(demuxer->stream, 0);

	return demuxer;
}

GxDemuxerClass gx_demux_mpeg_ps = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer MPEG-PS",
			.parent  = &gx_DemuxerBase,
			.size    = sizeof(GxDemuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run    = NULL,
		.pause  = NULL,
		.resume = NULL,
		.stop   = NULL,
	},
	DEF_AUTHOR("demuxer","mpeg ps","No description","L.F","No comment"),

	.name        = "Demuxer MPEG-PS",
	.type        = GX_DEMUXER_TYPE_MPEG_PS,
	.check_file  = demux_mpg_check_file,
	.open        = demux_mpg_ps_open,
	.close       = demux_mpg_close,
	.fill_buffer = demux_mpg_fill_buffer,
	.seek        = demux_mpg_seek,
	.control     = demux_mpg_control,
};

GxDemuxerClass gx_demux_mpeg_pes = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer MPEG-PES",
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
	DEF_AUTHOR("demuxer","mpeg pes","No description","L.F","No comment"),

	.name        = "Demuxer MPEG-PES",
	.type        = GX_DEMUXER_TYPE_MPEG_PES,
	.check_file  = demux_mpg_pes_check_file,
	.open        = demux_mpg_ps_open,
	.close       = demux_mpg_close,
	.fill_buffer = demux_mpg_fill_buffer,
	.seek        = demux_mpg_seek,
	.control     = demux_mpg_control,
};

GxDemuxerClass gx_demux_mpeg_es = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer MPEG-ES",
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
	DEF_AUTHOR("demuxer","mpeg es","No description","L.F","No comment"),

	.name        = "Demuxer MPEG-ES",
	.type        = GX_DEMUXER_TYPE_MPEG_ES,
	.check_file  = demux_mpg_es_check_file,
	.open        = demux_mpg_es_open,
	.close       = demux_mpg_close,
	.fill_buffer = demux_mpg_es_fill_buffer,
	.seek        = demux_mpg_seek,
	.control     = demux_mpg_control,
};

GxDemuxerClass gx_demux_mpeg4_es = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "Demuxer MPEG4-ES",
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
	DEF_AUTHOR("demuxer","mpeg4 es","No description","L.F","No comment"),

	.name        = "Demuxer MPEG4-ES",
	.type        = GX_DEMUXER_TYPE_MPEG4_ES,
	.check_file  = NULL,
	.open        = demux_mpg_es_open,
	.close       = demux_mpg_close,
	.fill_buffer = demux_mpg_es_fill_buffer,
	.seek        = demux_mpg_seek,
	.control     = demux_mpg_control,
};

GxDemuxerClass gx_demux_h264_es = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "DemuxerH264ES",
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
	DEF_AUTHOR("demuxer","h264 es","No description","L.F","No comment"),

	.name        = "Demuxer h.264-ES",
	.type        = GX_DEMUXER_TYPE_H264_ES,
	.check_file  = NULL,
	.open        = demux_mpg_es_open,
	.close       = demux_mpg_close,
	.fill_buffer = demux_mpg_es_fill_buffer,
	.seek        = demux_mpg_seek,
	.control     = demux_mpg_control,
};
