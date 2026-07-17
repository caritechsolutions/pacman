#include "gx_mediafilter.h"
#include "gx_common.h"

typedef struct {
	int dev;
	int module;
	int handle;
	int subflag;
	unsigned int   size;
	unsigned char *buffer;
} FifoPrivHWTS;

static GxFifo* hwts_fifo_create(size_t size,int subflag)
{
	GxFifo       *fifo = NULL;
	FifoPrivHWTS *priv = NULL;

	if (!(fifo = av_mallocz(sizeof(GxFifo)))) {
		gxloge("%s %d: malloc error\n", __func__, __LINE__);
		goto HWTS_ERROR;
	}

	if (!(priv = av_mallocz(sizeof(FifoPrivHWTS)))) {
		gxloge("%s %d: malloc error\n", __func__, __LINE__);
		goto HWTS_ERROR;
	}

	priv->dev    = GxAvdev_CreateDevice(0);
	priv->buffer = av_memhole_malloc(size, subflag);
	if (!priv->buffer) {
		gxloge("%s %d: hole malloc error\n", __func__, __LINE__);
		goto HWTS_ERROR;
	}
	priv->size    = size;
	priv->subflag = subflag;
	priv->handle  = GxAVFifoCreate(priv->dev, priv->buffer, size, GXAV_NO_PTS_FIFO);
	if (!priv->handle) {
		gxloge("%s %d: fifo create error\n", __func__, __LINE__);
		goto HWTS_ERROR;
	}

	fifo->priv = (void* )priv;
	return fifo;

HWTS_ERROR:
	if (priv) {
		if (priv->buffer)
			av_memhole_free(priv->buffer, priv->subflag);
		av_free(priv);
	}
	if (fifo) {
		av_free(fifo);
	}
	return NULL;
}

static int hwts_fifo_destroy(GxFifo *fifo)
{
	if (fifo) {
		FifoPrivHWTS* priv = fifo->priv;

		if (priv) {
			if (priv->module != 0)
				GxAvdev_CloseModule(priv->dev, priv->module);

			if (priv->handle != 0)
				GxAVFifoDestroy(priv->dev, priv->handle);

			if (priv->buffer != NULL)
				av_memhole_free(priv->buffer, priv->subflag);

			av_free(priv);
		}

		fifo->priv = NULL;
		av_free(fifo);
	}

	return 0;
}

static int hwts_fifo_config(GxFifo *fifo, void *configure)
{
	FifoPrivHWTS* priv = fifo->priv;
	GxFifoConfig *FifoConfig = configure;
	GxAvGateInfo GateInfo;
	GxDvrProperty_Config dvrconf;

	if (priv->module == 0)
		priv->module = GxAvdev_OpenModule(priv->dev, GXAV_MOD_DVR, FifoConfig->dvrid);

	GateInfo.almost_empty = FifoConfig->empty_gate;
	GateInfo.almost_full  = FifoConfig->full_gate;
	GateInfo.cache_max    = FifoConfig->cache_size;
	GxAVFifoConfig(priv->dev, priv->handle, &GateInfo);
	FifoConfig->dvrhandle = priv->module;

	memset(&dvrconf, 0, sizeof(dvrconf));
	dvrconf.src      = DVR_INPUT_DMX;
	dvrconf.dst      = DVR_OUTPUT_DSP;
	dvrconf.flags    = 0;
	dvrconf.blocklen = 0;
	dvrconf.dst_buf.sw_buffer_size   = priv->size;
	dvrconf.dst_buf.hw_buffer_size   = priv->size;
	dvrconf.dst_buf.almost_full_gate = FifoConfig->full_gate;

	return GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf));
}

static int hwts_fifo_link(GxFifo *fifo, void *configure)
{
	int ret = 0;
	FifoPrivHWTS* priv = fifo->priv;
	GxFifoLink *FifoConfig = configure;

	if ((ret = GxAVLinkFifo(priv->dev, FifoConfig->module, priv->handle, FifoConfig->pin_id, FifoConfig->direction)) < 0)
		return ret;

	if (FifoConfig->module == priv->module) {
		if ((ret = GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Run, NULL, 0)) < 0)
			return ret;
	}

	return ret;
}

static int hwts_fifo_unlink(GxFifo *fifo, void *configure)
{
	int ret = 0;
	FifoPrivHWTS* priv = fifo->priv;
	GxFifoLink *FifoConfig = configure;

	if (FifoConfig->module == priv->module) {
		if ((ret = GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Stop, NULL, 0)) < 0)
			return ret;
	}

	if ((ret = GxAVUnLinkFifo(priv->dev, FifoConfig->module, priv->handle)) < 0)
		return ret;

	return ret;
}

static int hwts_fifo_reset(GxFifo *fifo)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoReset(priv->dev, priv->handle);
}

static size_t hwts_fifo_read(GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoReadData(priv->dev, priv->handle, buffer, len, timeout_us);
}

static size_t hwts_fifo_read_pts(GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoReadDataPts(priv->dev, priv->handle, buffer, len, timeout_us, pts);
}

static size_t hwts_fifo_write(GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoWriteData(priv->dev, priv->handle, buffer, len, timeout_us);
}

static size_t hwts_fifo_write_pts(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoWriteDataPts(priv->dev, priv->handle, buffer, len, timeout_us, pts);
}

static size_t hwts_fifo_peek(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoPeekData(priv->dev, priv->handle, buffer, len, -1);
}

static size_t hwts_fifo_get_len(GxFifo *fifo)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoGetLength(priv->dev, priv->handle);
}

static size_t hwts_fifo_get_cap(GxFifo *fifo)
{
	FifoPrivHWTS* priv = fifo->priv;

	return priv->size;
}

static int hwts_fifo_roll_back(GxFifo *fifo, int len)
{
	FifoPrivHWTS* priv = fifo->priv;

	return GxAVFifoRollback(priv->dev, priv->handle, len);
}

GxFifoClass gx_fifo_hwts = {
	.create   = hwts_fifo_create,
	.destroy  = hwts_fifo_destroy,
	.config   = hwts_fifo_config,
	.link     = hwts_fifo_link,
	.unlink   = hwts_fifo_unlink,
	.reset    = hwts_fifo_reset,
	.read     = hwts_fifo_read,
	.readpts  = hwts_fifo_read_pts,
	.write    = hwts_fifo_write,
	.writepts = hwts_fifo_write_pts,
	.peek     = hwts_fifo_peek,
	.get_len  = hwts_fifo_get_len,
	.get_cap  = hwts_fifo_get_cap,
	.rollback = hwts_fifo_roll_back,
};
