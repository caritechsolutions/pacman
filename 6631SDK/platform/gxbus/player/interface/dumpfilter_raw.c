#include "page_cache.h"
#include "gx_dumpfilter.h"

#define DUMP_RAW_PAGE_SIZE (512 * 188)
#define DUMP_RAW_TIMEOUTUS (20000)

typedef struct {
	struct cache_page page;
}GxDumpFilterRAWPriv;

static int df_raw_open(GxDumpFilter* df)
{
	GxDumpFilterRAWPriv* priv = av_mallocz(sizeof(GxDumpFilterRAWPriv));
	if (priv == NULL)
		goto errout;

	priv->page.buffer  = av_malloc(DUMP_RAW_PAGE_SIZE);
	if (priv->page.buffer == NULL)
		goto errout;

	df->priv = priv;
	return 0;

errout:
	if (priv)
		av_free(priv);
	return -1;
}

static int df_raw_close(GxDumpFilter* df)
{
	if (df && df->priv) {
		GxDumpFilterRAWPriv* priv = df->priv;
		if (priv->page.buffer) {
			av_free(priv->page.buffer);
		}
	}
	av_free(df->priv);
	df->priv = NULL;

	return 0;
}

static int df_raw_read(GxDumpFilter* df)
{
	GxDumpFilterRAWPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	page->size = GxFifo_Read(df->inpin->source->fifo, page->buffer, DUMP_RAW_PAGE_SIZE, DUMP_RAW_TIMEOUTUS);
	return page->size;
}

static int df_raw_write(GxDumpFilter* df)
{
	size_t write_size, size = 0;
	GxDumpFilterRAWPriv* priv = df->priv;
	struct cache_page* page = &priv->page;

	if (page->size > 0) {
		GxDumpfilter_Heartbeats(df);
		write_size = GxStream_Write(df->s, page->buffer, page->size);
		if ((int)write_size != page->size)
			return -1;
		size += write_size;
	}

	return (int)size;
}

static int df_raw_control(GxDumpFilter* df, int cmd, void* args)
{
	switch (cmd)
	{
	case GX_DUMPER_SET_EXTRA:
		break;
	default:
		break;
	}

	return -1;
}

GxDumpFilterClass gx_dumpfilter_raw = {
	._inherit = {
		._inherit = {
			.name    = "DumpFilterraw",
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
		.type = DUMPFILTER_RAW,
		.open = df_raw_open,
		.close = df_raw_close,
		.read = df_raw_read,
		.write = df_raw_write,
		.control = df_raw_control,
};

