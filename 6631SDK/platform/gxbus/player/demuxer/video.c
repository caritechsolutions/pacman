#include "gx_stream.h"
#include "gx_demux.h"
#include "stheader.h"
#include "parse_es.h"
#include "mpeg_header.h"

#define VIDEO_MAX_PROBE_SIZE   0x400000

static MpegHeader picture;

static int telecine = 0;
static float telecine_cnt = -2.5;

int video_read_properties(GxStreamVideoHeader*  sh_video)
{
	int ret = GX_PLAYER_OK;
	GxDemuxStream* d_video;
	GxParseES es_parser;

	if (sh_video == NULL || sh_video->ds == NULL)
		return GX_PLAYER_ERROR;

	d_video = sh_video->ds;
	GxParseES_Init(&es_parser, d_video);
	memset(&picture,0,sizeof(MpegHeader));

	switch (sh_video->format) {
	case VIDEO_CODEC_MPEG4:
		{
			int pos = 0, vop_cnt = 0, units[3];
			es_parser.videobuf_len = 0;
			es_parser.videobuf_code_len = 0;
			es_parser.ds = d_video;
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i <= 0x11F)
					break;	// found it!
				if (!i || !GxParseES_SkipVideoPacket(&es_parser))
					goto cleanup;;
			}

			if (!es_parser.videobuffer) {
				es_parser.videobuffer =
					(uint8_t* ) av_malloc( VIDEOBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
				if (es_parser.videobuffer)
					memset(es_parser.videobuffer + VIDEOBUFFER_SIZE, 0,
							FF_INPUT_BUFFER_PADDING_SIZE);
				else
					goto cleanup;;
			}

			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i >= 0x120 && i <= 0x12F)
					break;	// found it!
				if (!i || !GxParseES_ReadVideoPacket(&es_parser))
					goto cleanup;;
			}
			pos = es_parser.videobuf_len + 4;
			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;
			mp4_header_process_vol(&picture, &(es_parser.videobuffer[pos]));
mp4_init:
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i == 0x1B6)
					break;	// found it!
				if (!i || !GxParseES_ReadVideoPacket(&es_parser))
					goto cleanup;;
			}
			pos = es_parser.videobuf_len + 4;
			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;
			mp4_header_process_vop(&picture, &(es_parser.videobuffer[pos]));
			units[vop_cnt] = picture.timeinc_unit;
			vop_cnt++;

			if (!picture.fps) {
				int i, mn, md, mx, diff;
				if (vop_cnt < 3)
					goto mp4_init;

				i = 0;
				mn = mx = units[0];
				for (i = 0; i < 3; i++) {
					if (units[i] < mn)
						mn = units[i];
					if (units[i] > mx)
						mx = units[i];
				}
				md = mn;
				for (i = 0; i < 3; i++) {
					if ((units[i] > mn) && (units[i] < mx))
						md = units[i];
				}

				if (mx - md > md - mn)
					diff = md - mn;
				else
					diff = mx - md;
				if (diff > 0)
					picture.fps = ((float)picture.timeinc_resolution) / diff;
			}
			if (picture.fps) {
				sh_video->fps = picture.fps;
				sh_video->frametime = 1.0 / picture.fps;
			}
			break;
		}
	case VIDEO_CODEC_H264:
		{
			int pos = 0;
			es_parser.videobuf_len = 0;
			es_parser.videobuf_code_len = 0;
			es_parser.ds = d_video;
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if ((i & ~0x60) == 0x107 && i != 0x107)
					break;	// found it!
				if (!i || !GxParseES_SkipVideoPacket(&es_parser))
					goto cleanup;
			}
			if (!es_parser.videobuffer) {
				es_parser.videobuffer =
					(uint8_t* ) av_malloc( VIDEOBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
				if (es_parser.videobuffer)
					memset(es_parser.videobuffer + VIDEOBUFFER_SIZE, 0,
							FF_INPUT_BUFFER_PADDING_SIZE);
				else
					goto cleanup;
			}
			pos = es_parser.videobuf_len + 4;
			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;
			h264_parse_sps(&picture, &(es_parser.videobuffer[pos]), es_parser.videobuf_len - pos);
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if ((i & ~0x60) == 0x108 && i != 0x108)
					break;	// found it!
				if (!i || !GxParseES_ReadVideoPacket(&es_parser))
					goto cleanup;;
			}
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if ((i & ~0x60) == 0x101 || (i & ~0x60) == 0x102 || (i & ~0x60) == 0x105)
					break;	// found it!
				if (!i || !GxParseES_ReadVideoPacket(&es_parser))
					goto cleanup;;
			}
			if (picture.fps) {
				sh_video->fps = picture.fps;
				sh_video->frametime = 1.0 / picture.fps;
			}

			break;
		}
	case VIDEO_CODEC_MPEG12:
		{
mpeg_header_parser:
			// Find sequence_header first:
			es_parser.videobuf_len = 0;
			es_parser.videobuf_code_len = 0;
			es_parser.ds = d_video;
			telecine = 0;
			telecine_cnt = -2.5;

			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i == 0x1B3)
					break;	// found it!
				if (!i || !GxParseES_SkipVideoPacket(&es_parser))
					goto cleanup;;
				if(GxStream_Tell(sh_video->ds->demuxer->stream) > VIDEO_MAX_PROBE_SIZE)
				{
					ret = GX_PLAYER_ERROR;
					goto cleanup;
				}
			}

			// ========= Read & process sequence header & extension ============
			if (!es_parser.videobuffer) {
				es_parser.videobuffer =
					(uint8_t* ) av_malloc( VIDEOBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
				if (es_parser.videobuffer)
					memset(es_parser.videobuffer + VIDEOBUFFER_SIZE, 0,
							FF_INPUT_BUFFER_PADDING_SIZE);
				else
					goto cleanup;
			}

			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;
			if (mp_header_process_sequence_header(&picture, &es_parser.videobuffer[4]))
				goto mpeg_header_parser;
			if (GxParseES_SyncVideoPacket(&es_parser) == 0x1B5) {	// next packet is seq. ext.
				int pos = es_parser.videobuf_len;
				if (!GxParseES_SyncVideoPacket(&es_parser))
					goto cleanup;;
				if (mp_header_process_extension(&picture, &es_parser.videobuffer[pos + 4]))
					goto cleanup;;
			}
			// display info:
			sh_video->fps = picture.fps;
			if (!sh_video->fps) {
				sh_video->frametime = 0;
			} else {
				sh_video->frametime = 1.0 / picture.fps;
			}
			sh_video->disp_w = picture.display_picture_width;
			sh_video->disp_h = picture.display_picture_height;
			sh_video->aspect = (float)sh_video->disp_w/sh_video->disp_h;
			// bitrate:
			if (picture.bitrate != 0x3FFFF && sh_video->i_bps==0)
				sh_video->i_bps = picture.bitrate*400/8;

			GxDemuxStream_FreePacks(d_video->demuxer->video);
			GxDemuxStream_FreePacks(d_video->demuxer->audio);
			GxDemuxStream_FreePacks(d_video->demuxer->sub);
			GxStream_Seek(d_video->demuxer->stream, 0);

			break;
		}
	case VIDEO_CODEC_VC1:
		{
			// Find sequence_header:
			es_parser.videobuf_len = 0;
			es_parser.videobuf_code_len = 0;
			es_parser.ds = d_video;
			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i == 0x10F)
					break;	// found it!
				if (!i || !GxParseES_SkipVideoPacket(&es_parser))
					goto cleanup;
			}
			if (!es_parser.videobuffer) {
				es_parser.videobuffer =
					(uint8_t* ) av_malloc( VIDEOBUFFER_SIZE + FF_INPUT_BUFFER_PADDING_SIZE);
				if (es_parser.videobuffer)
					memset(es_parser.videobuffer + VIDEOBUFFER_SIZE, 0,
							FF_INPUT_BUFFER_PADDING_SIZE);
				else
					goto cleanup;
			}
			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;

			while (1) {
				int i = GxParseES_SyncVideoPacket(&es_parser);
				if (i == 0x10E)
					break;	// found it!
				if (!i || !GxParseES_SkipVideoPacket(&es_parser))
					goto cleanup;;
			}
			if (!GxParseES_ReadVideoPacket(&es_parser))
				goto cleanup;;

			if (mp_vc1_decode_sequence_header
					(&picture, &es_parser.videobuffer[4], es_parser.videobuf_len - 4)) {
				sh_video->bih =
					(BITMAPINFOHEADER* ) av_calloc(1, sizeof(BITMAPINFOHEADER) + es_parser.videobuf_len);
				if (sh_video->bih == NULL)
					goto cleanup;;
				sh_video->bih->biSize = sizeof(BITMAPINFOHEADER) + es_parser.videobuf_len;
				memcpy(sh_video->bih + 1, es_parser.videobuffer, es_parser.videobuf_len);
				sh_video->bih->biCompression = sh_video->format;
				sh_video->bih->biWidth = sh_video->disp_w = picture.display_picture_width;
				sh_video->bih->biHeight = sh_video->disp_h = picture.display_picture_height;
				if (picture.fps > 0) {
					sh_video->frametime = 1.0 / picture.fps;
					sh_video->fps = picture.fps;
				}
			}
			break;
		}
	default:
		sh_video->format = VIDEO_CODEC_MPEG12;
		break;
	}
cleanup:
	if(es_parser.videobuffer)
		av_free(es_parser.videobuffer);

	return ret;
}


