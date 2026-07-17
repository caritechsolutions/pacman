#include "dumpfilter_fm.h"

#define DUMPER_FM_SAMPLE_MS       (128)

static void _fill_wave_header(unsigned char *buf, GxStreamDataInfo *data_info)
{
	unsigned char *ptr = buf;
	unsigned int  sample_byte = (data_info->bitwidth / 8) * data_info->channels;
	unsigned int  sample_size = data_info->samplerate * sample_byte;

	ptr[0] = 'R'; ptr[1] = 'I'; ptr[2] = 'F'; ptr[3] = 'F';
	ptr += 4;

	ptr[0] = 0xff; ptr[1] = 0xff; ptr[2] = 0xff; ptr[3] = 0xff;
	ptr += 4;

	ptr[0] = 'W'; ptr[1] = 'A'; ptr[2] = 'V'; ptr[3] = 'E';
	ptr += 4;

	ptr[0] = 'f'; ptr[1] = 'm'; ptr[2] = 't'; ptr[3] = ' ';
	ptr += 4;

	ptr[0] = 0x10; ptr[1] = 0x00; ptr[2] = 0x00; ptr[3] = 0x00;
	ptr += 4;

	ptr[0] = 0x01;
	ptr[1] = 0x00;
	ptr[2] = ((data_info->channels >> 0) & 0xff);
	ptr[3] = ((data_info->channels >> 8) & 0xff);
	ptr += 4;

	ptr[0] = ((data_info->samplerate >>  0) & 0xff);
	ptr[1] = ((data_info->samplerate >>  8) & 0xff);
	ptr[2] = ((data_info->samplerate >> 16) & 0xff);
	ptr[3] = ((data_info->samplerate >> 24) & 0xff);
	ptr += 4;

	ptr[0] = ((sample_size >>  0) & 0xff);
	ptr[1] = ((sample_size >>  8) & 0xff);
	ptr[2] = ((sample_size >> 16) & 0xff);
	ptr[3] = ((sample_size >> 24) & 0xff);
	ptr += 4;

	ptr[0] = ((sample_byte >>  0) & 0xff);
	ptr[1] = ((sample_byte >>  8) & 0xff);
	ptr[2] = ((data_info->bitwidth >> 0) & 0xff);
	ptr[3] = ((data_info->bitwidth >> 8) & 0xff);
	ptr += 4;

	ptr[0] = 'd'; ptr[1] = 'a'; ptr[2] = 't'; ptr[3] = 'a';
	ptr += 4;

	ptr[0] = 0xff; ptr[1] = 0xff; ptr[2] = 0xff; ptr[3] = 0xff;
}

static int df_fm_open(GxDumpFilter* df)
{
	GxDumpFilterFMPriv* priv = av_mallocz(sizeof(GxDumpFilterFMPriv));

	if (priv == NULL)
		return -1;

	df->priv = priv;
	return 0;
}

static int df_fm_close(GxDumpFilter* df)
{
	if (df && df->priv) {
		GxDumpFilterFMPriv* priv = df->priv;

		if (priv->page.buffer) {
			av_free(priv->page.buffer);
		}

		av_free(df->priv);
		df->priv = NULL;
	}

	return 0;
}

static int df_fm_read(GxDumpFilter* df)
{
	GxDumpFilterFMPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	page->size = GxStream_Read(priv->source, page->buffer, priv->samples_size);
	return page->size;
}

static int df_fm_write(GxDumpFilter* df)
{
	int size = 0;
	GxDumpFilterFMPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	if (page->size <= 0)
		return 0;

	GxDumpfilter_Heartbeats(df);

	if (priv->wav_header_fill) {
		GxStream_Write(df->s, priv->wav_header, sizeof(priv->wav_header));
		priv->wav_header_fill = 0;
	}

	size = GxStream_Write(df->s, page->buffer, page->size);
	return size;
}

static int df_fm_control(GxDumpFilter* df, int cmd, void* args)
{
	switch (cmd)
	{
	case GX_DUMPER_SET_SOURCE:
		{
			GxDumpFilterFMPriv* priv = df->priv;
			unsigned int samples = 0;

			priv->source = (GxStream *)args;
			GxStream_Control(priv->source, GX_STREAM_CTRL_GET_DATA_INFO, (void *)(&priv->data_info));
			samples = (DUMPER_FM_SAMPLE_MS * (priv->data_info.samplerate / 1000));
			priv->samples_size = samples * (priv->data_info.bitwidth / 8) * priv->data_info.channels;
			priv->page.buffer  = av_malloc(priv->samples_size);
			if (priv->page.buffer == NULL)
				return -1;

			_fill_wave_header(priv->wav_header, &priv->data_info);
			priv->wav_header_fill = 1;
			return 0;
		}
		break;
	default:
		break;
	}

	return -1;
}

GxDumpFilterClass gx_dumpfilter_fm = {
	._inherit = {
		._inherit = {
			.name    = "DumpFilterFM",
			.parent  = &gx_dumpfilter_base,
			.size    = sizeof(GxDumpFilter),
			.create  = NULL,
			.release = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.resume= NULL,
		.stop  = NULL,
	}
	,
		.type    = DUMPFILTER_FM,
		.open    = df_fm_open,
		.close   = df_fm_close,
		.read    = df_fm_read,
		.write   = df_fm_write,
		.control = df_fm_control,
};

