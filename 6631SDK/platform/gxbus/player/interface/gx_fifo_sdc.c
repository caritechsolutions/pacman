#include "gx_fifo_sdc.h"
#include "gx_mediafilter.h"

static GxFifo* fifo_create_sdc(size_t size, int subflag)
{
	GxFifo* fifo;
	FifoPrivSDC* priv;

	if ((priv = av_malloc(sizeof(FifoPrivSDC))) == NULL)
		return NULL;

	memset(priv, 0, sizeof(FifoPrivSDC));

	if ((fifo = av_malloc(sizeof(GxFifo))) == NULL) {
		av_free(priv);
		return NULL;
	}

	memset(fifo, 0, sizeof(GxFifo));

	priv->dev    = GxAvdev_CreateDevice(0);
	fifo->buffer = av_memhole_malloc(size, subflag);
	if(fifo->buffer == NULL){
		gxlogd("%s:%d, memhole alloc size == %d failure\n", __func__, __LINE__, size);
		av_free(priv);
		av_free(fifo);
		return NULL;
	}

	priv->handle = GxAVFifoCreate(priv->dev, fifo->buffer, size, subflag & (~GX_PINFLAG_MASK));
	if (priv->handle == 0 || priv->handle == -1 || priv->handle == -2) {
		av_memhole_free(fifo->buffer, subflag);
		av_free(priv);
		av_free(fifo);
		return NULL;
	}

	priv->flag = subflag;
	fifo->priv = (void* )priv;
	return fifo;
}

static int fifo_destroy_sdc(GxFifo *fifo)
{
	size_t size;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	size = GxAVFifoGetCap(priv->dev, priv->handle);
	GxCore_UnMap(priv->dev, fifo->buffer, size);
	GxAVFifoDestroy(priv->dev, priv->handle);
	GxAvdev_DestroyDevice(priv->dev);
	av_memhole_free(fifo->buffer, priv->flag);
	av_free(priv);
	av_free(fifo);
	return 0;
}

static int fifo_config_sdc(GxFifo *fifo, void *configure)
{
	GxAvGateInfo GateInfo;
	GxFifoConfig *FifoConfig = configure;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	GateInfo.almost_empty = FifoConfig->empty_gate;
	GateInfo.almost_full = FifoConfig->full_gate;
	GateInfo.cache_max = FifoConfig->cache_size;

	return GxAVFifoConfig(priv->dev, priv->handle, (GxAvGateInfo *) &GateInfo);
}

static int fifo_unlink_sdc(GxFifo *fifo, void *configure)
{
	GxFifoLink *FifoConfig = configure;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	return GxAVUnLinkFifo(priv->dev, FifoConfig->module, priv->handle);
}

static int fifo_link_sdc(GxFifo *fifo, void *configure)
{
	GxFifoLink *FifoConfig = configure;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	return GxAVLinkFifo(priv->dev, FifoConfig->module, priv->handle, FifoConfig->pin_id, FifoConfig->direction);
}

static int fifo_reset_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoReset(priv->dev, priv->handle);
}

static size_t fifo_write_sdc(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	int ret ;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoWriteData(priv->dev, priv->handle, buffer, len, timeout_us);
	return ret > 0 ? ret : 0;
}

static size_t fifo_write_pts_sdc(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us)
{
	int ret;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoWriteDataPts(priv->dev, priv->handle, buffer, len, timeout_us, pts);
	return ret > 0 ? ret : 0;
}

static size_t fifo_read_sdc(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	int ret ;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoReadData(priv->dev, priv->handle, buffer, len, timeout_us);
	return ret > 0 ? ret : 0;
}

static size_t fifo_read_pts_sdc(GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us)
{
	int ret ;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoReadDataPts(priv->dev, priv->handle, buffer, len, timeout_us, pts);
	return ret > 0 ? ret : 0;
}

static size_t fifo_peek_sdc(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	int ret ;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoPeekData(((FifoPrivSDC*) fifo->priv)->dev, priv->handle, buffer, len, -1);
	return ret > 0 ? ret : 0;
}

static size_t fifo_skip_sdc(GxFifo *fifo, unsigned int len)
{
	int ret ;
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;

	ret = GxAVFifoSkipData(((FifoPrivSDC*) fifo->priv)->dev, priv->handle, len, -1);
	return ret > 0 ? ret : 0;
}

static int fifo_roll_back_sdc(GxFifo *fifo, int len)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoRollback(priv->dev, priv->handle, len);
}

static size_t fifo_getcap_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetCap(priv->dev, priv->handle);
}

static size_t fifo_getlen_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetLength(priv->dev, priv->handle);
}

static size_t fifo_getfree_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetFreeSize(priv->dev, priv->handle);
}

static size_t fifo_getptscap_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetPtsCap(priv->dev, priv->handle);
}

static size_t fifo_getptslen_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetPtsLength(priv->dev, priv->handle);
}

static size_t fifo_getptsfree_sdc(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoGetPtsFreeSize(priv->dev, priv->handle);
}

static int fifo_pts_isfull(GxFifo *fifo)
{
	FifoPrivSDC* priv = (FifoPrivSDC *) fifo->priv;
	return GxAVFifoPtsIsFull(priv->dev, priv->handle);
}

GxFifoClass gx_fifo_sdc = {
	.create   = fifo_create_sdc,
	.destroy  = fifo_destroy_sdc,
	.config   = fifo_config_sdc,
	.link     = fifo_link_sdc,
	.unlink   = fifo_unlink_sdc,
	.reset    = fifo_reset_sdc,
	.read     = fifo_read_sdc,
	.readpts  = fifo_read_pts_sdc,
	.write    = fifo_write_sdc,
	.writepts = fifo_write_pts_sdc,
	.peek     = fifo_peek_sdc,
	.skip     = fifo_skip_sdc,
	.get_cap  = fifo_getcap_sdc,
	.get_len  = fifo_getlen_sdc,
	.get_free = fifo_getfree_sdc,
	.get_pts_len  = fifo_getptslen_sdc,
	.get_pts_free = fifo_getptsfree_sdc,
	.get_pts_cap  = fifo_getptscap_sdc,
	.rollback     = fifo_roll_back_sdc,
	.pts_isfull   = fifo_pts_isfull,
};
