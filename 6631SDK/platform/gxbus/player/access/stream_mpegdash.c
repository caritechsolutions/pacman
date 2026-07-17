#include "gx_stream.h"
#include "gx_media.h"
#include "gx_options.h"

#define DEFAULE_DASH_FILE_PATH "/tmp/mpegdash.xml"

typedef struct {
	GxStream parent;
} GxStreamMpegDash;

static int mpegdash_open(GxStream *s, int mode)
{
	GxStreamMpegDash *stream = (GxStreamMpegDash*)s;

	stream->parent.file_format = GX_STREAMTYPE_PLAYLIST;
	stream->parent.flags   = 0;
	stream->parent.end_pos = 0;
	stream->parent.demuxer_type = GX_DEMUXER_TYPE_MPEG_DASH;

	return GX_PLAYER_OK;
}

static void mpegdash_close(GxStream *s)
{
	remove(DEFAULE_DASH_FILE_PATH);
	return;
}

static int mpegdash_control(GxStream *s, int cmd, void *arg)
{
	switch(cmd) {
	case GX_STREAM_CTRL_GET_BASE_URL:
		{
			char *option = GxOptions_Get_By_Name(s->options, " -H", "BaseUrl:");
			if (arg && option)
				strncpy(arg, option, strlen(option));
			break;
		}
	default:
		break;
	}

	return GX_PLAYER_OK;
}

static int mpegdash_probe(GxStream *s, uint8_t *buffer, size_t size, void **priv)
{
	int iStatuFlg = GX_PLAYER_ERROR;
	if (!strncasecmp((const char *)buffer, "<?xml", 5)) {
		if (strstr((const char *)buffer, "<MPD") || strstr((const char *)buffer, "<mpd"))
			iStatuFlg = GX_PLAYER_OK;
	}

	if ((GX_PLAYER_OK == iStatuFlg) && (strlen(((const char *)buffer)) > 0))
	{
		FILE *fpFile = fopen(DEFAULE_DASH_FILE_PATH, "w+");
		if (NULL == fpFile)
		{
			gxlogd ("fopen fail, [%s, %d]\n", __func__, __LINE__);
			return 0;
		}
		fprintf(fpFile, "%s", buffer);
		fclose(fpFile);
		sprintf (s->url, "%s", DEFAULE_DASH_FILE_PATH);
		iStatuFlg = GX_PLAYER_OK;
	}
	return iStatuFlg;
}

GxStreamClass gx_stream_mpegdash = {
	._inherit = {
		._inherit     = {
			.name    = "StreamMpegDash",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStreamMpegDash),
		},
	},
	.protocols = {"mpegdash", NULL},
	DEF_AUTHOR("Stream","mpegdash","No description","L.F","No comment"),

	.flags   = 0,
	.probe   = mpegdash_probe,
	.open    = mpegdash_open,
	.read    = NULL,
	.write   = NULL,
	.seek    = NULL,
	.control = mpegdash_control,
	.close   = mpegdash_close,
};
