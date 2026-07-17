#include "gx_fileops.h"
#include "libpvr/libpvr.h"

#define CTX_CHECK_OPS_NULL(ctx) do {                \
	if (!ctx || !ctx->ops) {                        \
		gxloge("%s %d\n", __func__, __LINE__);      \
		return -1;                                  \
	}                                               \
} while(0)

typedef struct {
	handle_t     mutex;
	PVROps      *ops;
	void        *priv;
} PVRContext;

static int pvrops_open(char*filename, mode_t mode)
{
	int ret = -1;
	PVRContext *ctx = NULL;
	PVROps *ops = NULL;

	if (!(ctx = av_mallocz(sizeof(PVRContext)))) {
		gxloge("%s %d\n", __func__, __LINE__);
		return -1;
	}

	ops = pvr_get_ops(filename);
	if (!ops) {
		gxloge("%s %d\n", __func__, __LINE__);
		av_free(ctx);
		return -1;
	}

	if (ops->open)
		ctx->priv = ops->open(filename, mode);

	if (!ctx->priv) {
		av_free(ctx);
		return -1;
	}

	ctx->ops = ops;
	GxCore_MutexCreate(&ctx->mutex);

	return ((int)ctx);
}

static int pvrops_close(int fildes)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->close)
		ctx->ops->close(ctx->priv);

	ctx->ops  = NULL;
	ctx->priv = NULL;
	GxCore_MutexDelete(ctx->mutex);
	av_free(ctx);

	return 0;
}

static int pvrops_read(int fildes, uint8_t * buffer, size_t max_len, int timeoutms)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->read)
		return ctx->ops->read(ctx->priv, buffer, max_len);

	return -1;
}

static int pvrops_read_ext(int fildes, uint8_t *buffer, size_t max_len, int timeoutms, off_t *outpos)
{
	int ret = -1;
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->read)
		ret = ctx->ops->read(ctx->priv, buffer, max_len);

	if (outpos) {
		if (ctx->ops->getpos) {
			*outpos = ctx->ops->getpos(ctx->priv);
		}
	}

	return ret;
}

static int pvrops_write(int fildes, uint8_t * buffer, size_t len)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->write)
		return ctx->ops->write(ctx->priv, buffer, len);

	return -1;
}

static off_t pvrops_seek(int fildes, off_t offset, int whence)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->seek)
		return ctx->ops->seek(ctx->priv, offset);

	return -1;
}

static off_t pvrops_seek_ext(int fildes, off_t offset, int whence, off_t *outpos)
{
	int ret = -1;
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->seek)
		ret = ctx->ops->seek(ctx->priv, offset);

	if (outpos) {
		if (ctx->ops->getpos) {
			*outpos = ctx->ops->getpos(ctx->priv);
		}
	}

	return ret;
}

static off_t pvrops_getsize(int fildes)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->getinfo) {
		PVRInfo info;

		ctx->ops->getinfo(ctx->priv, &info);
		return info.tl_filesize;
	}

	return 0;
}

static struct vol_info pvrops_getvolinfo(int fildes)
{
	struct vol_info volinfo = {0, 0};
	PVRContext *ctx = (PVRContext *)fildes;

	if (!ctx || !ctx->ops) {
		gxloge("%s %d\n", __func__, __LINE__);
		return volinfo;
	}

	if (ctx->ops->getinfo) {
		PVRInfo info;

		ctx->ops->getinfo(ctx->priv, &info);
		volinfo.size = info.tl_filesize;
		volinfo.truesize = info.tr_filesize;
	}

	return volinfo;
}

static void pvrops_settimestamp(int fildes, unsigned int timems)
{
	PVRContext *ctx = (PVRContext *)fildes;

	if (!ctx || !ctx->ops) {
		gxloge("%s %d\n", __func__, __LINE__);
		return;
	}

	if (ctx->ops->settime) {
		ctx->ops->settime(ctx->priv, timems);
	}

	return;
}

static int pvrops_read_hdr(int fildes, uint8_t *buffer, size_t max_len, int timeoutms)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->readhdr) {
		return ctx->ops->readhdr(ctx->priv, buffer, max_len);
	}

	return -1;
}

static int pvrops_write_hdr(int fildes, uint8_t *buffer, size_t max_len)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->writehdr) {
		return ctx->ops->writehdr(ctx->priv, buffer, max_len);
	}

	return -1;
}

static int pvrops_probe_hdr(int fildes)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->probehdr) {
		return ctx->ops->probehdr(ctx->priv);
	}

	return -1;
}

static int pvrops_set_control(int fildes, GxRecordPVRControl *ctrl)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->setctrl) {
		return ctx->ops->setctrl(ctx->priv, ctrl);
	}

	return -1;
}

static int pvrops_get_contorl(int fildes, GxRecordPVRControl *ctrl)
{
	PVRContext *ctx = (PVRContext *)fildes;

	CTX_CHECK_OPS_NULL(ctx);

	if (ctx->ops->getctrl) {
		return ctx->ops->getctrl(ctx->priv, ctrl);
	}

	return -1;
}

GxFileOps gx_fileops_pvr = {
	.open          = pvrops_open,
	.read          = pvrops_read,
	.read_ext      = pvrops_read_ext,
	.write         = pvrops_write,
	.seek          = pvrops_seek,
	.seek_ext      = pvrops_seek_ext,
	.close         = pvrops_close,
	.getsize       = pvrops_getsize,
	.getvolinfo    = pvrops_getvolinfo,
	.settimestamp  = pvrops_settimestamp,
	.read_hdr      = pvrops_read_hdr,
	.write_hdr     = pvrops_write_hdr,
	.probe_hdr     = pvrops_probe_hdr,
	.set_pvrctrl   = pvrops_set_control,
	.get_pvrctrl   = pvrops_get_contorl,
};

