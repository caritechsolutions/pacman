/*
 *  FLV demuxer
 *  Copyright (c) 2003 The FFmpeg Project.
 *
 *  This demuxer will generate a 1 byte extradata for VP6F content.
 *  It is composed of:
 *   - upper 4bits: difference between encoded width and visible width
 *   - lower 4bits: difference between encoded height and visible height
 *
 *  This file is part of FFmpeg.
 *
 *  FFmpeg is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  FFmpeg is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with FFmpeg; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include "avformat.h"
#include "flv.h"
#include "../avcodec/mpeg4audio.h"
#include "gx_stream.h"

#define FLV_MAX_PACKET (32*1024)

typedef struct {
	int wrong_dts; ///< wrong dts due to negative cts
	int index_loaded;
	int searched_for_end;
} FLVContext;

union av_intfloat64 {
	uint64_t i;
	double f;
};

static av_always_inline double av_int2double(uint64_t i)
{
	union av_intfloat64 v = { .i = i };
	return v.f;
}

static int flv_probe(AVProbeData*  p)
{
	const uint8_t* d;

	d = p->buf;
	if (d[0] == 'F' && d[1] == 'L' && d[2] == 'V' && d[3] < 5 && d[5] == 0) {
		return AVPROBE_SCORE_MAX;
	}
	return 0;
}

static void flv_set_audio_codec(AVFormatContext*  s, AVStream * astream, int flv_codecid)
{
	AVCodecContext* acodec = astream->codec;
	switch (flv_codecid) {
		//no distinction between S16 and S8 PCM codec flags
		case FLV_CODECID_PCM_BE:
			acodec->codec_id = acodec->bits_per_sample == 8 ? CODEC_ID_PCM_S8 :
#ifdef WORDS_BIGENDIAN
				CODEC_ID_PCM_S16BE;
#else
			CODEC_ID_PCM_S16LE;
#endif
			break;
		case FLV_CODECID_PCM_LE:
			acodec->codec_id = acodec->bits_per_sample == 8 ? CODEC_ID_PCM_S8 : CODEC_ID_PCM_S16LE;
			break;
		case FLV_CODECID_ADPCM:
			acodec->codec_id = CODEC_ID_ADPCM_SWF;
			break;
		case FLV_CODECID_MP4A:
			acodec->codec_tag =  MKTAG('m', 'p', '4', 'a');
			acodec->codec_id = CODEC_ID_AAC;
			break;
		case FLV_CODECID_MP3:
			acodec->codec_id = CODEC_ID_MP3;
			astream->need_parsing = AVSTREAM_PARSE_FULL;
			break;
		case FLV_CODECID_NELLYMOSER_8HZ_MONO:
			acodec->sample_rate = 8000;	//in case metadata does not otherwise declare samplerate
		case FLV_CODECID_NELLYMOSER:
		default:
			gxlogf("Unsupported audio codec (%x)\n", flv_codecid >> FLV_AUDIO_CODECID_OFFSET);
			acodec->codec_tag = flv_codecid >> FLV_AUDIO_CODECID_OFFSET;
	}
}

static int flv_set_video_codec(AVFormatContext*  s, AVStream * vstream, int flv_codecid)
{
	AVCodecContext* vcodec = vstream->codec;
	switch (flv_codecid) {
		case MKBETAG('h', 'v', 'c', '1'):
			vcodec->codec_tag = MKTAG('h', '2', '6', '5');
			return vcodec->codec_id = CODEC_ID_HEVC;
		case MKBETAG('a', 'v', '0', '1'):
			return vcodec->codec_id == CODEC_ID_AV1;
		case MKBETAG('v', 'p', '0', '9'):
			return vcodec->codec_id == CODEC_ID_VP9;
		case FLV_CODECID_H263:
			vcodec->codec_id = CODEC_ID_FLV1;
			break;
		case FLV_CODECID_H264:
			vcodec->codec_tag = MKTAG('h', '2', '6', '4');
			vcodec->codec_id = CODEC_ID_H264;
			return 3;
			break;
		case FLV_CODECID_SCREEN:
			vcodec->codec_id = CODEC_ID_FLASHSV;
			break;
		case FLV_CODECID_VP6:
			vcodec->codec_id = CODEC_ID_VP6F;
		case FLV_CODECID_VP6A:
			if (flv_codecid == FLV_CODECID_VP6A)
				vcodec->codec_id = CODEC_ID_VP6A;
			if (vcodec->extradata_size != 1) {
				vcodec->extradata_size = 1;
				vcodec->extradata = av_malloc(1);
			}
			vcodec->extradata[0] = get_byte(s->pb);
			return 1;	// 1 byte body size adjustment for flv_read_packet()
		case FLV_CODECID_MPEG4:
			//vcodec->codec_tag = MKTAG('F', 'M', 'P', '4');
			vcodec->codec_id = CODEC_ID_MPEG4;
			return 3;
		default:
			gxlogf("Unsupported video codec (%x)\n", flv_codecid);
			vcodec->codec_tag = flv_codecid;
	}

	return 0;
}

//copy some bytes from ioc->buf to buffer
//parameter buffsize is the max bytes of the byte to copy

static int amf_get_string(ByteIOContext*  ioc, char *buffer, int buffsize)
{
	int length = get_be16(ioc);
	if (length >= buffsize) {
		url_fskip(ioc, length);
		return -1;
	}

	get_buffer(ioc, (unsigned char*)buffer, length);

	buffer[length] = '\0';

	return length;
}

static int parse_keyframes_index(AVFormatContext *s, AVIOContext *ioc, AVStream *vstream, int64_t max_pos)
{
//    FLVContext *flv = s->priv_data;
    unsigned int timeslen = 0, fileposlen = 0, i;
    char str_val[256];
    int64_t *times = NULL;
    int64_t *filepositions = NULL;
    int ret = AVERROR(ENOSYS);
    int64_t initial_pos = avio_tell(ioc);

    if(vstream->nb_index_entries>0){
        av_log(s, AV_LOG_WARNING, "Skiping duplicate index\n");
        return 0;
    }

    if (s->flags & AVFMT_FLAG_IGNIDX)
        return 0;

    while (avio_tell(ioc) < max_pos - 2 && amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
        int64_t** current_array;
        unsigned int arraylen;

        // Expect array object in context
        if (avio_r8(ioc) != AMF_DATA_TYPE_ARRAY)
            break;

        arraylen = avio_rb32(ioc);
        if(arraylen>>28)
            break;

        if(!strcmp(KEYFRAMES_TIMESTAMP_TAG , str_val) && !times){
            current_array= &times;
            timeslen= arraylen;
        }else if (!strcmp(KEYFRAMES_BYTEOFFSET_TAG, str_val) && !filepositions){
            current_array= &filepositions;
            fileposlen= arraylen;
        }else // unexpected metatag inside keyframes, will not use such metadata for indexing
            break;

        if (!(*current_array = av_mallocz(sizeof(**current_array) * arraylen))) {
            ret = AVERROR(ENOMEM);
            goto finish;
        }

        for (i = 0; i < arraylen && avio_tell(ioc) < max_pos - 1; i++) {
            if (avio_r8(ioc) != AMF_DATA_TYPE_NUMBER)
                goto invalid;
            current_array[0][i] = av_int2double(avio_rb64(ioc));
        }
        if (times && filepositions) {
            // All done, exiting at a position allowing amf_parse_object
            // to finish parsing the object
            ret = 0;
            break;
        }
    }

    if (timeslen == fileposlen && fileposlen>1 && max_pos <= filepositions[0]) {
        for (i = 0; i < fileposlen; i++) {
            av_add_index_entry(vstream, filepositions[i], times[i]*1000,
                               0, 0, AVINDEX_KEYFRAME);
            //if (i < 2) {
            //    flv->validate_index[i].pos = filepositions[i];
            //    flv->validate_index[i].dts = times[i] * 1000;
            //    flv->validate_count = i + 1;
            //}
        }
    } else {
invalid:
        av_log(s, AV_LOG_WARNING, "Invalid keyframes object, skipping.\n");
    }

finish:
    av_freep(&times);
    av_freep(&filepositions);
    avio_seek(ioc, initial_pos, SEEK_SET);
    return ret;
}

static int amf_parse_object(AVFormatContext*  s, AVStream * astream, AVStream * vstream, const char *key,
		unsigned int max_pos, int depth)
{
	AVCodecContext* acodec, *vcodec;
	ByteIOContext* ioc;
	AMFDataType amf_type;
	char str_val[256];
	double num_val;

	num_val = 0;
	ioc = s->pb;

	amf_type = get_byte(ioc);

	switch (amf_type) {
		case AMF_DATA_TYPE_NUMBER:
			num_val = av_int2double(get_be64(ioc));
			break;
		case AMF_DATA_TYPE_BOOL:
			num_val = get_byte(ioc);
			break;
		case AMF_DATA_TYPE_STRING:
			if (amf_get_string(ioc, str_val, sizeof(str_val)) < 0)
				return -1;
			break;
		case AMF_DATA_TYPE_OBJECT:{
				if ((vstream || astream) && ioc->seekable && key && !strcmp(KEYFRAMES_TAG, key) && depth == 1)
					parse_keyframes_index(s, ioc, vstream ? vstream : astream, max_pos);

				while (avio_tell(ioc) < max_pos - 2 && amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
			       if (amf_parse_object(s, astream, vstream, str_val, max_pos, depth + 1) < 0)
					    return -1; //if we couldn't skip, bomb out.
				}
			    if(avio_r8(ioc) != AMF_END_OF_OBJECT)
				    return -1;
			}
            break;
		case AMF_DATA_TYPE_NULL:
		case AMF_DATA_TYPE_UNDEFINED:
		case AMF_DATA_TYPE_UNSUPPORTED:
					  break;		//these take up no additional space
		case AMF_DATA_TYPE_MIXEDARRAY:
					  url_fskip(ioc, 4);	//skip 32-bit max array index
					  while (url_ftell(ioc) < max_pos - 2 && amf_get_string(ioc, str_val, sizeof(str_val)) > 0) {
						  //this is the only case in which we would want a nested parse to not skip over the object
						  if (amf_parse_object(s, astream, vstream, str_val, max_pos, depth + 1) < 0)
							  return -1;
					  }
					  if (get_byte(ioc) != AMF_END_OF_OBJECT)
						  return -1;
					  break;
		case AMF_DATA_TYPE_ARRAY:{
						 unsigned int arraylen, i;

						 arraylen = get_be32(ioc);
						 for (i = 0; i < arraylen && url_ftell(ioc) < max_pos - 1; i++) {
							 if (amf_parse_object(s, NULL, NULL, NULL, max_pos, depth + 1) < 0)
								 return -1;	//if we couldn't skip, bomb out.
						 }
					 }
					 break;
		case AMF_DATA_TYPE_DATE:
					 url_fskip(ioc, 8 + 2);	//timestamp (double) and UTC offset (int16)
					 break;
		default:		//unsupported type, we couldn't skip
					 return -1;
	}

	if (depth == 1 && key) {	//only look for metadata values when we are not nested and key != NULL
		acodec = astream ? astream->codec : NULL;
		vcodec = vstream ? vstream->codec : NULL;

		gxlogd("key %s, %s, %f\n", key, str_val, num_val);
		if (amf_type == AMF_DATA_TYPE_BOOL) {
			if (!strcmp(key, "stereo") && acodec)
				acodec->channels = num_val > 0 ? 2 : 1;
			else if (!strcmp(key, "framerate")) {
				if (vstream)
					vstream->avg_frame_rate = av_d2q(num_val, 1000);
			}else if (!strcmp(key, "duration"))
				s->duration = num_val * AV_TIME_BASE;
		} else if (amf_type == AMF_DATA_TYPE_STRING) {
			if (!strcmp(key, "audiocodecid") && acodec) {
				if (!strncasecmp(str_val, "mp3", 3)) {
					num_val = 2;
					flv_set_audio_codec(s, astream, (int)num_val << FLV_AUDIO_CODECID_OFFSET);
				}
			}
		} else if (amf_type == AMF_DATA_TYPE_NUMBER) {
			if (!strcmp(key, "duration"))
				s->duration = num_val*  AV_TIME_BASE;
			else if(!strcmp(key, "filesize")){
				s->file_size = num_val;
			}
			else if(!strcmp(key, "framerate") && vcodec){
				vcodec->frame_rate = 0;
				if(num_val < 12)
					vcodec->frame_rate = 12;
				else if(num_val >= 12 && num_val <= 60) {
					vstream->r_frame_rate.den = 1000;
					vstream->r_frame_rate.num = (int)(num_val*1000);
				}
			}else if(!strcmp(key, "width")  && vcodec && num_val > 0) vcodec->width  = num_val;
			else if(!strcmp(key, "height") && vcodec && num_val > 0) vcodec->height = num_val;
			else if (!strcmp(key, "audiocodecid") && acodec)
				flv_set_audio_codec(s, astream, (int)num_val << FLV_AUDIO_CODECID_OFFSET);
			else if (!strcmp(key, "videocodecid") && vcodec)
				flv_set_video_codec(s, vstream, (int)num_val);
			else if (!strcmp(key, "audiosamplesize") && acodec && num_val >= 0) {
				acodec->bits_per_sample = num_val;
				//we may have to rewrite a previously read codecid because FLV only marks PCM endianness.
				if (num_val == 8
						&& (acodec->codec_id == CODEC_ID_PCM_S16BE
							|| acodec->codec_id == CODEC_ID_PCM_S16LE))
					acodec->codec_id = CODEC_ID_PCM_S8;
			} else if (!strcmp(key, "audiosamplerate") && acodec && num_val >= 0) {
				//some tools, like FLVTool2, write consistently approximate metadata sample rates
				switch ((int)num_val) {
					case 44000:
						acodec->sample_rate = 44100;
						break;
					case 22000:
						acodec->sample_rate = 22050;
						break;
					case 11000:
						acodec->sample_rate = 11025;
						break;
					case 5000:
						acodec->sample_rate = 5512;
						break;
					default:
						acodec->sample_rate = num_val;
				}
			}
		}
		if (amf_type == AMF_DATA_TYPE_OBJECT) {
			if(s->nb_streams == 1 &&
					((!acodec && !strcmp(key, "audiocodecid")) ||
					 (!vcodec && !strcmp(key, "videocodecid"))))
                s->ctx_flags &= ~AVFMTCTX_NOHEADER; //If there is either audio/video missing, codecid will be an empty object
		}

		if (!strcmp(key, "duration")        ||
				!strcmp(key, "filesize")        ||
				!strcmp(key, "width")           ||
				!strcmp(key, "height")          ||
				!strcmp(key, "videodatarate")   ||
				!strcmp(key, "framerate")       ||
				!strcmp(key, "videocodecid")    ||
				!strcmp(key, "audiodatarate")   ||
				!strcmp(key, "audiosamplerate") ||
				!strcmp(key, "audiosamplesize") ||
				!strcmp(key, "stereo")          ||
				!strcmp(key, "audiocodecid")    ||
				!strcmp(key, "datastream"))
			return 0;

		if (amf_type == AMF_DATA_TYPE_BOOL) {
			av_strlcpy(str_val, num_val > 0 ? "true" : "false", sizeof(str_val));
		} else if (amf_type == AMF_DATA_TYPE_NUMBER) {
			snprintf(str_val, sizeof(str_val), "%.f", num_val);
		}
	}

	return 0;
}

static int flv_read_metabody(AVFormatContext*  s, unsigned int next_pos)
{
	AMFDataType type;
	AVStream* stream, *astream, *vstream;
	ByteIOContext* ioc;
	int i;
	char buffer[11];	//only needs to hold the string "onMetaData". Anything longer is something we don't want.

	astream = NULL;
	vstream = NULL;
	ioc = s->pb;

	//first object needs to be "onMetaData" string
	type = get_byte(ioc);
	if (type != AMF_DATA_TYPE_STRING || amf_get_string(ioc, buffer, sizeof(buffer)) < 0
			|| strcmp(buffer, "onMetaData"))
		return -1;

	//find the streams now so that amf_parse_object doesn't need to do the lookup every time it is called.
	for (i = 0; i < s->nb_streams; i++) {
		stream = s->streams[i];
		if (stream->codec->codec_type == CODEC_TYPE_AUDIO)
			astream = stream;
		else if (stream->codec->codec_type == CODEC_TYPE_VIDEO)
			vstream = stream;
	}

	//parse the second object (we want a mixed array)
	if (amf_parse_object(s, astream, vstream, buffer, next_pos, 0) < 0)
		return -1;

	return 0;
}

static AVStream* create_stream(AVFormatContext *s, int is_audio){
	AVStream* st = av_new_stream(s, is_audio);
	if (!st)
		return NULL;
	st->codec->codec_type = is_audio ? CODEC_TYPE_AUDIO : CODEC_TYPE_VIDEO;
	av_set_pts_info(st, 32, 1, 1000); /* 32 bit pts in ms*/
	return st;
}

static int flv_read_header(AVFormatContext*  s, AVFormatParameters * ap)
{
	int offset, flags;
	AVStream* st;

	s->ctx_flags |= AVFMTCTX_NOHEADER;
	url_fskip(s->pb, 4);
	flags = get_byte(s->pb);
	/* old flvtool cleared this field*/
	/* FIXME: better fix needed*/
	if (!flags) {
		flags = FLV_HEADER_FLAG_HASVIDEO | FLV_HEADER_FLAG_HASAUDIO;
		gxlogf("Broken FLV file, which says no streams present, this might fail\n");
	}

	if (flags & FLV_HEADER_FLAG_HASVIDEO) {
		st = av_new_stream(s, 0);
		if (!st)
			return AVERROR(ENOMEM);
		st->codec->codec_type = CODEC_TYPE_VIDEO;
		av_set_pts_info(st, 24, 1, 1000);	/* 24 bit pts in ms*/
	}
	if (flags & FLV_HEADER_FLAG_HASAUDIO) {
		st = av_new_stream(s, 1);
		if (!st)
			return AVERROR(ENOMEM);
		st->codec->codec_type = CODEC_TYPE_AUDIO;
		av_set_pts_info(st, 24, 1, 1000);	/* 24 bit pts in ms*/
	}

	offset = get_be32(s->pb);
	url_fseek(s->pb, offset, SEEK_SET);

	s->start_time  = 0;
	s->lastseekpos = 0;
	s->lastseekpts = 0;

	return 0;
}

static int flv_get_extradata(AVFormatContext* s, AVStream *st, int size)
{
	av_free(st->codec->extradata);
	st->codec->extradata = av_mallocz(size + FF_INPUT_BUFFER_PADDING_SIZE);
	if (!st->codec->extradata)
		return AVERROR(ENOMEM);
	st->codec->extradata_size = size;
	get_buffer(s->pb, st->codec->extradata, st->codec->extradata_size);
	st->codec->is_reset_extradata = 1;
	return 0;
}

static int flv_find_rightdata(AVFormatContext* s)
{
	int type = -1;
	int size = 0, i = 0;
	int64_t pos=0, next;

	while(!url_feof(s->pb)){
		while(type != FLV_TAG_TYPE_AUDIO && type != FLV_TAG_TYPE_VIDEO && type != FLV_TAG_TYPE_META){
			if(url_check_interrupt_cb())
				return -1;
			type = get_byte(s->pb);
			pos = url_ftell(s->pb);
			if (url_feof(s->pb))
				return -1;
		}

		size = get_be24(s->pb);
		if(size >= FLV_MAX_PACKET || size <= 0){
			url_fseek(s->pb, pos, SEEK_SET);
			type = -1;
			continue;
		}
		next = pos + 10 + size ;
		url_fseek(s->pb, next, SEEK_SET);

recheck_find:
		url_fskip(s->pb, 4);	/* size of previous packet*/
		type = get_byte(s->pb);
		size = get_be24(s->pb);
		if(type == FLV_TAG_TYPE_AUDIO || type == FLV_TAG_TYPE_VIDEO || type == FLV_TAG_TYPE_META){
			if(i >= 2){
				pos = url_ftell(s->pb);
				url_fseek(s->pb, pos - 8, SEEK_SET);
				return 0;
			}else{
				i++;
				url_fskip(s->pb, 7 + size);
				goto recheck_find;
			}
		}
		i = 0;
		url_fseek(s->pb, pos, SEEK_SET);
	}
	return -1;
}

static int flv_read_packet(AVFormatContext*  s, AVPacket * pkt)
{
	FLVContext* flv = s->priv_data;
	int ret = -1, i = 0, type = 0, size = 0, flags = 0, stream_type = -1, is_audio = 0;
	int64_t next, pos;
	int64_t dts, pts = AV_NOPTS_VALUE;
	AVStream* st = NULL;

	if(flv==NULL) {
		flv = s->priv_data = av_mallocz(sizeof(FLVContext));
		if (!flv)
			return AVERROR_NOMEM;
	}

retry:

	for (;;) {
		if(url_check_interrupt_cb())
			return -AVERROR(EOF);

		pos = url_ftell(s->pb);
		url_fskip(s->pb, 4);	/* size of previous packet*/
		type = (avio_r8(s->pb) & 0x1F);
		size = get_be24(s->pb);
		dts = get_be24(s->pb);
		dts |= (unsigned)get_byte(s->pb) << 24;
		//dts = dts&(~(0xf8<<24));

		if (url_feof(s->pb))
			return AVERROR_EOF;
		url_fskip(s->pb, 3); /* stream id, always 0*/
		flags = 0;

		if (size == 0)
			continue;

		next = size + url_ftell(s->pb);

		if (type == FLV_TAG_TYPE_AUDIO) {
			stream_type = FLV_STREAM_TYPE_AUDIO;
			is_audio = 1;
			flags = get_byte(s->pb);
			size--;
		}
		else if (type == FLV_TAG_TYPE_VIDEO) {
			stream_type = FLV_STREAM_TYPE_VIDEO;
			is_audio = 0;
			flags = get_byte(s->pb);
			size--;
			if ((flags & 0xf0) == 0x50) /* video info / command frame*/
				goto skip;
		} else {
			if (type == FLV_TAG_TYPE_META && size > 13 + 1 + 4) {
				stream_type = FLV_STREAM_TYPE_DATA;
				flv_read_metabody(s, next);
			} else {	/* skip packet*/
				gxlogf("\033[33mskipping flv packet: type %x, size %x, flags %x\033[0m\n", type, size, flags);
				if(flv_find_rightdata(s) < 0)
					return AVERROR(EIO);
				else
					continue;
			}
skip:
			url_fseek(s->pb, next, SEEK_SET);
			continue;
		}

		/* skip empty data packets*/
		if (!size)
			continue;
		/* now find stream*/
		for (i = 0; i < s->nb_streams; i++) {
			st = s->streams[i];
			if (st->id == is_audio)
				break;
		}
		if (i == s->nb_streams) {
			gxlogf("invalid stream\n");
			st= create_stream(s, is_audio);
			s->ctx_flags &= ~AVFMTCTX_NOHEADER;
		}
		//    av_log(NULL, AV_LOG_DEBUG, "%d %X %d \n", is_audio, flags, st->discard);
		if ((st->discard >= AVDISCARD_NONKEY
					&& !((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY || is_audio))
				|| (st->discard >= AVDISCARD_BIDIR
					&& ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_DISP_INTER && !is_audio))
				|| st->discard >= AVDISCARD_ALL) {
			url_fseek(s->pb, next, SEEK_SET);
			continue;
		}
		if ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY)
			av_add_index_entry(st, pos+4, dts, size, 0, AVINDEX_KEYFRAME);
		break;
	}

	// if not streamed and no duration from metadata then seek to end to find the duration from the timestamps
	if (!url_is_streamed(s->pb) &&
			(s->file_format != GX_STREAMTYPE_STREAM) && s->duration == AV_NOPTS_VALUE) {
		int size;
		const int64_t pos= url_ftell(s->pb);
		const int64_t fsize= url_fsize(s->pb);
		url_fseek(s->pb, fsize - 4, SEEK_SET);
		size = get_be32(s->pb);
		url_fseek(s->pb, fsize - 3 - size, SEEK_SET);
		if (size == get_be24(s->pb) + 11) {
			s->duration = get_be24(s->pb)*  (int64_t) AV_TIME_BASE / 1000;
		}else
			s->duration = 0;
		url_fseek(s->pb, pos, SEEK_SET);
	}

	if (stream_type == FLV_STREAM_TYPE_AUDIO) {
		if(!st->codec->channels || !st->codec->sample_rate || !st->codec->bits_per_sample) {
			st->codec->channels = (flags & FLV_AUDIO_CHANNEL_MASK) == FLV_STEREO ? 2 : 1;
			st->codec->sample_rate = (44100 << ((flags & FLV_AUDIO_SAMPLERATE_MASK) >> FLV_AUDIO_SAMPLERATE_OFFSET) >> 3);
			st->codec->bits_per_sample = (flags & FLV_AUDIO_SAMPLESIZE_MASK) ? 16 : 8;
		}
		flv_set_audio_codec(s, st, flags & FLV_AUDIO_CODECID_MASK);
	} else if (stream_type == FLV_STREAM_TYPE_VIDEO) {
		size -= flv_set_video_codec(s, st, flags & FLV_VIDEO_CODECID_MASK);
	}

	if (st->codec->codec_id == CODEC_ID_AAC ||
			st->codec->codec_id == CODEC_ID_H264 ||
			st->codec->codec_id == CODEC_ID_HEVC ||
			st->codec->codec_id == CODEC_ID_AV1 ||
			st->codec->codec_id == CODEC_ID_VP9 ||
			st->codec->codec_id == CODEC_ID_MPEG4) {
		int type = get_byte(s->pb);
		size--;
		if (st->codec->codec_id == CODEC_ID_H264 || st->codec->codec_id == CODEC_ID_HEVC || st->codec->codec_id == CODEC_ID_MPEG4) {
			int32_t cts = (get_be24(s->pb)+0xff800000)^0xff800000; // sign extension
			pts = dts + cts;
			if (cts < 0) { // dts are wrong
				flv->wrong_dts = 1;
				gxlogf("negative cts, previous timestamps might be wrong\n");
			}
			if (flv->wrong_dts)
				dts = AV_NOPTS_VALUE;
		}
		if (type == 0) {
			if ((ret = flv_get_extradata(s, st, size)) < 0)
				return ret;
			if (st->codec->codec_id == CODEC_ID_AAC) {
				MPEG4AudioConfig cfg;
				mpeg4audio_get_config(&cfg, st->codec->extradata,
						st->codec->extradata_size);
				if (cfg.chan_config > 7)
					return -1;
				st->codec->channels = mpeg4audio_channels[cfg.chan_config];
				st->codec->sample_rate = cfg.sample_rate;
				gxlogf("mp4a config channels %d sample rate %d\n", st->codec->channels, st->codec->sample_rate);
			}
			goto retry;
		}
	}

	/* skip empty data packets*/
	if (!size){
		if(s->file_format == GX_STREAMTYPE_STREAM)
			goto retry;
		else
			return AVERROR(EAGAIN);
	}

	ret= av_get_packet(s->pb, pkt, size);
	if (ret < 0) {
		return AVERROR(EIO);
	}
	/* note: we need to modify the packet size here to handle the last
	   packet*/

	pkt->size = ret;
	pkt->dts = dts;
	pkt->pts = (pts == AV_NOPTS_VALUE ? dts : pts);
	pkt->stream_index = st->index;

	if (is_audio || ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY))
		pkt->flags |= PKT_FLAG_KEY;

	return ret;
}

static int flv_read_close(AVFormatContext*  s)
{
	return 0;
}


static int flv_find_targetpos(AVFormatContext* s, int64_t timestamp, int flag)
{
	int i=0, type=0, size, flags=0, is_audio=0,ret;
	int64_t next=0, pos=0,guesspos=0, last_pos=0;
	int64_t dts = 0,targetstamp = timestamp;
	int64_t dis_dts = 0;
	int64_t last_dis_dts = 0;
	int64_t indexstamp = 0;
	int64_t indexpos   = 0;
	int64_t findstamp  = 0;
	int64_t findpos    = 0;
	AVStream* st = NULL;
	offset_t targetpos = 0, filesize = 0;

	if(s->file_size <= 0)
		filesize = url_fsize(s->pb);
	else
		filesize = s->file_size;

	last_pos = url_ftell(s->pb);

	int precision = 4;

	if(timestamp < 0)
		return -1;

	int sample = -1;
	AVStream* vstream = NULL;
	for (i = 0; i < s->nb_streams; i++) {
		st = s->streams[i];
		if (st->codec->codec_type == CODEC_TYPE_VIDEO)
			vstream = st;
	}
	i = 0;
	st = NULL;
	if(vstream){
		sample = av_index_search_timestamp(vstream, timestamp*1000, flag);
		if(sample >= 0){
			if ((flag & AVSEEK_FLAG_BACKWARD)) {
				if(vstream->index_entries[sample].timestamp/1000 > timestamp) sample--;
				dis_dts = timestamp - vstream->index_entries[sample].timestamp/1000;
				if(!(sample >= 0 && dis_dts >= 0 && dis_dts <= 10)) sample = -1;
			}
			else {
				int64_t best_dts = -1;
				int best_sample = -1;
				int min_sample = (sample <= 0) ? sample : (sample - 1);
				int max_sample = (sample >= (vstream->nb_index_entries - 1)) ? (vstream->nb_index_entries - 1) : (sample + 1);
				for (sample = min_sample; sample <= max_sample; sample++) {
					dis_dts = abs(vstream->index_entries[sample].timestamp - timestamp*1000);
					if (best_dts == -1 || best_dts >= dis_dts) {
						best_sample = sample;
						best_dts    = dis_dts;
					}
				}
				if (0 <= best_dts && best_dts <= 10000) {
					sample = best_sample;
				}
				else {
					sample = -1;
				}
			}
		}
	}
	if(sample >= 0){
		//针有关键帧信息的头部数据seek操作
		targetpos = vstream->index_entries[sample].pos;
		if(url_fseek(s->pb, targetpos-4, SEEK_SET) < 0) return -1;
		goto synced2;
	}else{
		//盲找出seek的时间点对应的位置
		int step =  abs(s->lastseekpts/1000-timestamp);
		if(step < precision/4)
		{
			return s->lastseekpos;
		}
		//if video time length is 00:01:25 and audio time length is 00:01:40
		//then, when seek to 00:01:30, video has finished,can not find I frame.
		//so minus targetstamp 10 seconds.
		if(targetstamp > 10)
		{
			if (s->duration > 0)
				targetpos = s->lastseekpos + (timestamp-s->lastseekpts/1000)*(filesize/(s->duration/AV_TIME_BASE));
			if(targetpos > filesize)
				return -1;
		}
		else
			targetpos = 0;
	}

restart:
	if(url_fseek(s->pb, targetpos, SEEK_SET) < 0)
		return -1;

	while(!url_feof(s->pb))
	{
		while(type != FLV_TAG_TYPE_AUDIO && type != FLV_TAG_TYPE_VIDEO && type != FLV_TAG_TYPE_META)
		{
			if(url_check_interrupt_cb())
				return -1;

			type = get_byte(s->pb);
			pos = url_ftell(s->pb);
			if (url_feof(s->pb))
				return AVERROR(EIO);
		}
		size = get_be24(s->pb);
		if(size >= FLV_MAX_PACKET || size <= 0)
		{
			url_fseek(s->pb, pos, SEEK_SET);
			type = -1;
			continue;
		}

		next = pos + 10 + size ;

		if(next >= filesize)
		{
			type = -1;
			url_fseek(s->pb, pos, SEEK_SET);
			continue;
		}

		url_fseek(s->pb, next, SEEK_SET);
recheck:
		url_fskip(s->pb, 4);	/* size of previous packet*/
		type = get_byte(s->pb);
		size = get_be24(s->pb);
		dts  = get_be24(s->pb);
		dts |= get_byte(s->pb) << 24;
		if(type == FLV_TAG_TYPE_AUDIO || type == FLV_TAG_TYPE_VIDEO || type == FLV_TAG_TYPE_META)
		{
			if(i >= 3){
				url_fseek(s->pb, next, SEEK_SET);
				goto synced1;
			}else{
				i++;
				url_fskip(s->pb, 3+size);
				goto recheck;
			}
		}
		i=0;
		url_fseek(s->pb, pos, SEEK_SET);
	}

synced1:
	dis_dts = dts/1000 -(timestamp-2);
	gxlogf("dts %lld, timestamp %lld, dis_dts %lld, last_dis_dts %lld\n",
			dts/1000, timestamp, dis_dts, last_dis_dts);
	if((abs(dis_dts) > precision/4) && (s->duration > 0)){
		if(last_dis_dts!=0 && abs(last_dis_dts)<=abs(dis_dts)){
			targetpos -= (dis_dts)*(filesize/(s->duration/AV_TIME_BASE))/4;
			if(targetpos<0 || targetpos>filesize)
				targetpos = last_pos;
			url_fseek(s->pb, targetpos, SEEK_SET);
			flv_find_rightdata(s);
		}else{
			type=0;flags=0;pos=0;is_audio=0;i=0;st = NULL;
			last_dis_dts = dis_dts;
			if(dis_dts > 0)
				targetpos -= (dis_dts)*(filesize/(s->duration/AV_TIME_BASE));
			else
				targetpos -= (dis_dts)*(filesize/(s->duration/AV_TIME_BASE))/2;
			goto restart;
		}
	}

synced2:
	guesspos = url_ftell(s->pb);
	int64_t min_dts = timestamp;
	while(!((flags&FLV_VIDEO_FRAMETYPE_MASK)==FLV_FRAME_KEY))
	{
		if(url_check_interrupt_cb())
			return -1;

		pos = url_ftell(s->pb);
		url_fskip(s->pb, 4);	/* size of previous packet*/
		type = get_byte(s->pb);
		size = get_be24(s->pb);
		dts  = get_be24(s->pb);
		dts |= get_byte(s->pb) << 24;

		if (url_feof(s->pb))
			return guesspos;//AVERROR(EIO);
		url_fskip(s->pb, 3); /* stream id, always 0*/
		flags = 0;

		if (size == 0)
			continue;

		next = size + url_ftell(s->pb);
		if (type == FLV_TAG_TYPE_AUDIO)
		{
			is_audio = 1;
			flags = get_byte(s->pb);
			size--;
		}
		else if (type == FLV_TAG_TYPE_VIDEO)
		{
			is_audio = 0;
			flags = get_byte(s->pb);
			size--;
			if ((flags & 0xf0) == 0x50) /* video info / command frame*/
			{
				ret = url_fseek(s->pb, next, SEEK_SET);
				if(ret < 0)
					return guesspos;
				continue;
			}
			if ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY)
			{
				int tmp_dts = abs(dts/1000-timestamp);
				if(tmp_dts <= min_dts)
				{
					min_dts = tmp_dts;
					guesspos = pos;
				}
				else
					return guesspos;
			}
		}

		/* skip empty data packets*/
		if (!size)
			continue;

		/* now find stream*/
		for (i = 0; i < s->nb_streams; i++) {
			st = s->streams[i];
			if (st->id == is_audio)
				break;
		}

		if (i == s->nb_streams) {
			gxlogf("invalid stream\n");
			st= create_stream(s, is_audio);
			s->ctx_flags &= ~AVFMTCTX_NOHEADER;
		}

		if ((st->discard >= AVDISCARD_NONKEY
					&& !((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_KEY || is_audio))
				|| (st->discard >= AVDISCARD_BIDIR
					&& ((flags & FLV_VIDEO_FRAMETYPE_MASK) == FLV_FRAME_DISP_INTER && !is_audio))
				|| st->discard >= AVDISCARD_ALL) {
			ret = url_fseek(s->pb, next, SEEK_SET);
			if(ret < 0)
				return guesspos;
			continue;
		}

		ret = url_fseek(s->pb, next, SEEK_SET);
		if(ret < 0)
			return guesspos;
	}

	sample      = (sample < 0) ? (vstream->nb_index_entries - 1) : sample;
	targetstamp = timestamp*1000;
	indexstamp  = vstream->index_entries[sample].timestamp;
	indexpos    = vstream->index_entries[sample].pos;
	findstamp   = dts;
	findpos     = pos;

	if ((targetstamp >= indexstamp) && (targetstamp <= findstamp)) {
		if (flag & AVSEEK_FLAG_BACKWARD) {
			if ((abs(targetstamp - indexstamp) <= 10000)
					|| (abs(targetstamp - findstamp) > 10000))
				findpos = indexpos - 4;
		}
		else {
			if (abs(targetstamp - findstamp) > abs(targetstamp - indexstamp))
				findpos = indexpos - 4;
		}
	}
	else if ((targetstamp <= indexstamp) && (targetstamp >= findstamp)) {
		if (flag & AVSEEK_FLAG_BACKWARD) {
			if ((abs(targetstamp - findstamp) > 10000)
					&& (abs(targetstamp - indexstamp) <= 10000))
				findpos = indexpos - 4;
		}
		else {
			if (abs(targetstamp - findstamp) > abs(targetstamp - indexstamp))
				findpos = indexpos - 4;
		}
	}
	else {
		if (abs(targetstamp - indexstamp) <= abs(targetstamp - findstamp))
			findpos = indexpos - 4;
	}

	return findpos;
}

extern void av_read_frame_flush(AVFormatContext*  s);

static int flv_read_seek(AVFormatContext* s, int stream_index, int64_t timestamp, int flags)
{
	int64_t pos;

	if(s->file_format == GX_STREAMTYPE_STREAM && s->seek_by_time)
		pos = 0;
	else
		pos = flv_find_targetpos(s, s->target_time, flags);

	if(pos < 0)
		return -1;

	/* do the seek*/
	url_fseek(s->pb, pos, SEEK_SET);
	av_read_frame_flush(s);

	return 0;
}


AVInputFormat flv_demuxer = {
	.name           = "flv",
	.priv_data_size = sizeof(FLVContext),
	.read_probe     = flv_probe,
	.read_header    = flv_read_header,
	.read_packet    = flv_read_packet,
	.read_close     = flv_read_close,
	.read_seek      = flv_read_seek,
	.extensions     = "flv",
	.value          = CODEC_ID_FLV1,
};

