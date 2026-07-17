/*
 * =====================================================================================
 *
 *       Filename:  muxer_lavf.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2015年12月08日 15时18分27秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  yingc (), yingc@nationalchip.com
 *   Organization:
 *
 * =====================================================================================
 */
#include "../avformat/avformat.h"
#include "../include/gx_muxer.h"

static AVPacket vheader_pkt;

typedef struct  {
	AVFormatContext *avfc;
	ByteIOContext* pb;
	uint8_t buffer[MUX_IO_BUFFER_SIZE];
} MuxLavfPriv;

static int muxer_ffmpeg_write(void* opaque, unsigned char *buf, int size)
{
	int ret = 0;
	GxMuxer* muxer = opaque;

	while (ret < size) {
		if (ret != 0)
			GxCore_ThreadDelay(50);
		ret += GxFifo_Write(muxer->outpin->fifo, buf, size, -1);
	}

	return ret;
}

static int muxer_ffmpeg_control(GxMuxer *muxer, int cmd, void *args)
{

	MuxLavfPriv *priv = muxer->priv;

	switch(cmd)
	{
		case GX_MUXER_CTRL_SET_VIDEO_HEADER:
		{
			AVPacket pkt;
			GxMuxerHeaderInfo *info = (GxMuxerHeaderInfo*)args;
			if(info->extradata == NULL || info->extradata_size == 0)
				return -1;

			av_init_packet(&pkt);
			pkt.pts          = 0;
			pkt.dts          = pkt.pts;
			pkt.data         = info->extradata;
			pkt.size         = info->extradata_size;
			pkt.flags        = AV_PKT_FLAG_VIDEO_HEADER;
			pkt.stream_index = info->stream_index;
			memcpy(&vheader_pkt, &pkt, sizeof(AVPacket));
			av_write_frame(priv->avfc, &pkt);
			break;
		}

		case GX_MUXER_CTRL_SET_AUDIO_HEADER:
		{
			AVPacket pkt;
			GxMuxerHeaderInfo *info = (GxMuxerHeaderInfo*)args;
			if(info->extradata == NULL || info->extradata_size == 0)
				return -1;

			av_init_packet(&pkt);
			pkt.pts          = 0;
			pkt.dts          = pkt.pts;
			pkt.data         = info->extradata;
			pkt.size         = info->extradata_size;
			pkt.flags        = AV_PKT_FLAG_AUDIO_HEADER;
			pkt.stream_index = info->stream_index;
			av_write_frame(priv->avfc, &pkt);
			break;
		}

		default:
			break;
	}
	return 0;
}

static void muxer_ffmpeg_close(GxMuxer *muxer)
{
	MuxLavfPriv *priv = muxer->priv;

	if (priv) {
		if (priv->avfc)
			avformat_free_context(priv->avfc);
		av_free(priv->pb);
		av_free(priv);
	}
}

static GxMuxer *muxer_ffmpeg_open(GxMuxer *muxer)
{
	int i;
	MuxLavfPriv *priv;
	AVFormatContext* avfc = NULL;

	if((priv = av_mallocz(sizeof(MuxLavfPriv))) == NULL) {
		return NULL;
	}
	muxer->priv = priv;

	if (avformat_alloc_output_context2(&avfc, NULL, muxer->info.format, NULL) < 0) {
		gxlogd("@@@@@open out error@@@@\n");
		goto errout;
	}
	priv->avfc = avfc;

	for (i = 0; i < muxer->info.nb_streams; i++) {
		AVStream *st = NULL;
		st = avformat_new_stream(avfc, NULL);
		st->codec->codec_type = muxer->info.streams[i].codec_type;
		st->codec->codec_id = muxer->info.streams[i].codec_id;
	}

	priv->pb = avio_alloc_context(priv->buffer, MUX_IO_BUFFER_SIZE, 0,
			muxer, NULL, muxer_ffmpeg_write, NULL);
	avfc->pb = priv->pb;
	avfc->flags = URL_RDONLY;

	avformat_write_header(avfc, NULL);
	return muxer;

errout:
	muxer_ffmpeg_close(muxer);
	return NULL;
}

static int muxer_ffmpeg_write_trailer(GxMuxer *muxer)
{
	MuxLavfPriv *priv = muxer->priv;
	av_write_trailer(priv->avfc);
	return 0;
}

static int muxer_ffmpeg_write_header(GxMuxer *muxer, int stream_index, uint8_t *data, size_t size)
{
	MuxLavfPriv *priv = muxer->priv;
	avformat_write_header(priv->avfc, NULL);
	return 0;
}

static int muxer_ffmpeg_write_frame(GxMuxer *muxer, int stream_index, uint8_t *data, size_t size, int pts)
{
	AVPacket pkt;
	MuxLavfPriv *priv = muxer->priv;

	if (stream_index >= muxer->info.nb_streams || stream_index < 0) {
		gxlogd("bad mux data\n");
		return -1;
	}
	av_init_packet(&pkt);
	pkt.data = data;
	pkt.size = size;
	pkt.stream_index = stream_index;
	pkt.pts = pts < 0 ? 0 : pts*90;
	pkt.dts = pkt.pts;
	pkt.flags = AV_PKT_FLAG_CONTENT;

	av_write_frame(priv->avfc, &vheader_pkt);
	av_write_frame(priv->avfc, &pkt);
	return 0;
}

GxMuxerClass gx_muxer_ffmpeg = {
	._inherit = {
		._inherit = {
			.name    = "MuxerFFmpeg",
			.parent  = &gx_MuxerBase,
			.size    = sizeof(GxMuxer),
			.create  = NULL,
			.release = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.resume= NULL,
		.stop  = NULL,
	}
	,
		.name = "MuxerFFMpeg",
		.type = GX_MUXER_TYPE_FFMPEG,
		.open = muxer_ffmpeg_open,
		.close = muxer_ffmpeg_close,
		.control = muxer_ffmpeg_control,
		.write_header = muxer_ffmpeg_write_header,
		.write_frame = muxer_ffmpeg_write_frame,
		.write_trailer = muxer_ffmpeg_write_trailer,
};


