#include "gx_stream.h"
#include "gx_demux.h"
#include "gx_avtick.h"
#include "stheader.h"
#include "demux_dump.h"

#define DEBUG_DUMP_DEMUX_ESV_DATA  0
#define DEBUG_DUMP_DEMUX_ESA_DATA  0
#define DEBUG_ROOT "/mnt/usb01/debuges"

#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
typedef struct {
#define DUMP_DEMUXER_ES_LEN (64*1024)
	GxStream      *stream;
	unsigned char *wbuf;
	int            wptr;
} DemuxDumpStream;

typedef struct {
	DemuxDumpStream a0;
	DemuxDumpStream a1;
	DemuxDumpStream v0;
	char            urlbuf[1024];
} DemuxDumpConetxt;
#endif

void* GxDemuxDump_Open(GxDemuxer *demuxer)
{
#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
	DemuxDumpConetxt *ddc = NULL;
	unsigned int time_ms = av_get_tick_ms(NULL, 0);

	ddc = av_mallocz(sizeof(DemuxDumpConetxt));
#if (DEBUG_DUMP_DEMUX_ESA_DATA)
	if (HAVE_AUDIO(demuxer)) {
		GxStreamAudioHeader *sh = demuxer->audio->sh;

		snprintf(ddc->urlbuf, sizeof(ddc->urlbuf),
				"%s/audio_0-%02d-%02d-%02d.%s.esa",
				DEBUG_ROOT,
				time_ms/1000/60/60,
				time_ms/1000/60,
				time_ms/1000%60,
				acodec_std2str(sh->format));
		ddc->a0.stream = GxStream_Open(ddc->urlbuf, GX_STREAM_TRUNC, 0);
	}
	if (HAVE_AUDIO1(demuxer)) {
		GxStreamAudioHeader *sh = demuxer->audio1->sh;

		snprintf(ddc->urlbuf, sizeof(ddc->urlbuf),
				"%s/audio_1-%02d-%02d-%02d.%s.esa",
				DEBUG_ROOT,
				time_ms/1000/60/60,
				time_ms/1000/60,
				time_ms/1000%60,
				acodec_std2str(sh->format));
		ddc->a1.stream = GxStream_Open(ddc->urlbuf, GX_STREAM_TRUNC, 0);
	}
#endif
#if (DEBUG_DUMP_DEMUX_ESV_DATA)
	if (HAVE_VIDEO(demuxer)) {
		GxStreamVideoHeader *sh = demuxer->video->sh;

		snprintf(ddc->urlbuf, sizeof(ddc->urlbuf),
				"%s/video_0-%02d-%02d-%02d.%s.esv",
				DEBUG_ROOT,
				time_ms/1000/60/60,
				time_ms/1000/60,
				time_ms/1000%60,
				vcodec_std2str(sh->format));
		ddc->v0.stream = GxStream_Open(ddc->urlbuf, GX_STREAM_TRUNC, 0);
	}
#endif
	return ddc;
#endif

	return NULL;
}

int GxDemuxDump_Read(void *priv, GxDemuxStream *ds, unsigned char *buf, unsigned int size)
{
#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
	int ret = 0;
	DemuxDumpConetxt *ddc = (DemuxDumpConetxt *)priv;
	GxDemuxer *d = ds->demuxer;

	if (!ddc) return -1;
	if (ds == d->video) {
		if (ddc->v0.stream)
			ret = GxStream_Read(ddc->v0.stream, buf, size);
	} else if (ds == d->audio) {
		if (ddc->a0.stream)
			ret = GxStream_Read(ddc->a0.stream, buf, size);
	} else if (ds == d->audio1) {
		if (ddc->a1.stream)
			ret = GxStream_Read(ddc->a1.stream, buf, size);
	}
	return ret;
#endif
	return -1;
}

#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
static int __demux_dump_write(DemuxDumpStream *dds, unsigned char *buf, unsigned int size, int over_flag)
{
	int ret = 0;
	unsigned int wsize = 0;

	if (!dds->stream)
		return -1;

	if (dds->wbuf == NULL) {
		dds->wbuf = av_mallocz(DUMP_DEMUXER_ES_LEN);
		dds->wptr = 0;
	}

	if ((size > 0) && (buf != NULL)) {
		while (wsize < size) {
			unsigned int csize = (DUMP_DEMUXER_ES_LEN - dds->wptr);

			csize = ((size - wsize) > csize) ? csize : (size - wsize);
			memcpy(dds->wbuf + dds->wptr, buf + wsize, csize);
			wsize     += csize;
			dds->wptr += csize;
			if (dds->wptr >= DUMP_DEMUXER_ES_LEN) {
				ret = GxStream_Write(dds->stream, dds->wbuf, dds->wptr);
				dds->wptr = 0;
			}
		}
	}

	if (over_flag) {
		ret = GxStream_Write(dds->stream, dds->wbuf, dds->wptr);
		av_free(dds->wbuf);
		dds->wbuf = NULL;
		dds->wptr = 0;
	}

	return ret;
}
#endif

int GxDemuxDump_Write(void *priv, GxDemuxStream *ds, unsigned char *buf, unsigned int size)
{
#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
	int ret = 0;
	DemuxDumpConetxt *ddc = (DemuxDumpConetxt *)priv;
	GxDemuxer *d = ds->demuxer;

	if (!ddc) return -1;
	if (ds == d->video) {
		__demux_dump_write(&ddc->v0, buf, size, 0);
	} else if (ds == d->audio) {
		__demux_dump_write(&ddc->a0, buf, size, 0);
	} else if (ds == d->audio1) {
		__demux_dump_write(&ddc->a1, buf, size, 0);
	}
	return ret;
#endif
	return -1;
}

void GxDemuxDump_Close(void *priv)
{
#if (DEBUG_DUMP_DEMUX_ESA_DATA || DEBUG_DUMP_DEMUX_ESV_DATA)
	DemuxDumpConetxt *ddc = (DemuxDumpConetxt *)priv;

	if (!ddc) return;

	__demux_dump_write(&ddc->v0, NULL, 0, 1);
	__demux_dump_write(&ddc->a0, NULL, 0, 1);
	__demux_dump_write(&ddc->a1, NULL, 0, 1);

	if (ddc->v0.stream)
		GxStream_Close(ddc->v0.stream);
	if (ddc->a0.stream)
		GxStream_Close(ddc->a0.stream);
	if (ddc->a1.stream)
		GxStream_Close(ddc->a1.stream);

	av_free(ddc);
#endif
	return;
}

