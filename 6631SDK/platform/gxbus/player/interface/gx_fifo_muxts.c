#include "gx_mediafilter.h"
#include "gx_common.h"

typedef struct {
	int dev;
	int module;
	int hwbuf_security;
	int hwbuf_ptr_en;
	int block_size;
}FifoPrivMuxts;

static GxFifo* fifo_create_muxts(size_t size,int subflag)
{
	GxFifo* fifo;
	FifoPrivMuxts* priv;

	if ((priv = av_mallocz(sizeof(FifoPrivMuxts))) == NULL)
		return NULL;

	if ((fifo = av_mallocz(sizeof(GxFifo))) == NULL) {
		av_free(priv);
		return NULL;
	}

	fifo->priv = (void* )priv;

	return fifo;
}

static int fifo_destroy_muxts(GxFifo *fifo)
{
	if(fifo)
	{
		FifoPrivMuxts* priv = fifo->priv;
		if (priv->module != 0) {
			GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Stop, NULL, 0);
			GxAvdev_CloseModule(priv->dev, priv->module);
		}
		av_free(fifo->priv);
		av_free(fifo);
	}
	return 0;
}

static int fifo_config_muxts(GxFifo *fifo, void *configure)
{
	int ret = 0;
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;
	GxFifoConfig *fifoconf = configure;
	GxDvrProperty_Config dvrconf;
	int tsr_flag = 0;

	if (fifoconf->source == DVR_INPUT_MEM ||
			(fifoconf->source >= DVR_INPUT_DVR0 && fifoconf->source <= DVR_INPUT_DVR3)) {
		tsr_flag = 1;
	}
	memset(&dvrconf, 0, sizeof(dvrconf));
	priv->dev = fifoconf->dev;
	if (priv->module == 0) {
		priv->module = fifoconf->dvrhandle = GxAvdev_OpenModule(priv->dev, GXAV_MOD_DVR, fifoconf->dvrid);

		if ((ret = GxAVGetProperty(priv->dev, priv->module, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf))) < 0)
			return ret;

		if (!(dvrconf.flags & DVR_FLAG_MEM_NOT_PROTECTED)) {
			if (tsr_flag == 0) {
				if (av_memhole_tsw_getsize() == 0)
					fifoconf->flags &= ~DVR_FLAG_PTR_MODE_EN;
				else
					fifoconf->flags |= DVR_FLAG_PTR_MODE_EN;
			}
			if (fifoconf->flags & DVR_FLAG_PTR_MODE_EN) {
				fifoconf->sw_buffer_size   = 0;
				fifoconf->hw_buffer_size   = fifo->size;
				fifoconf->almost_full_gate = fifo->size/20;
			}
			priv->hwbuf_security = 1;
		}
	}

	if (tsr_flag == 0)
		fifoconf->blocklen = (fifoconf->flags & DVR_FLAG_PTR_MODE_EN) ? fifoconf->blocklen : 0 ;
	priv->hwbuf_ptr_en = (fifoconf->flags & DVR_FLAG_PTR_MODE_EN) ? 1 : 0;
	priv->block_size   = fifoconf->blocklen;
	gxlogi(">>>> source: %s - hwsec %d, ptr_en %d, swbuf 0x%x, hwbuf 0x%x, blocksize 0x%x\n",
			tsr_flag ? "tsr" : "tsw",
			priv->hwbuf_security,
			priv->hwbuf_ptr_en,
			fifoconf->sw_buffer_size,
			fifoconf->hw_buffer_size,
			priv->block_size);

	dvrconf.src      = fifoconf->source;
	dvrconf.dst      = fifoconf->dest;
	dvrconf.flags    = fifoconf->flags;
	dvrconf.blocklen = fifoconf->blocklen;
	if (tsr_flag == 1) { // TSR
		dvrconf.src_buf.sw_buffer_size = fifoconf->sw_buffer_size;
		dvrconf.src_buf.hw_buffer_size = fifoconf->hw_buffer_size;
		dvrconf.src_buf.almost_full_gate = fifoconf->almost_full_gate;
	}
	else {
		dvrconf.dst_buf.sw_buffer_size = fifoconf->sw_buffer_size;
		dvrconf.dst_buf.hw_buffer_size = fifoconf->hw_buffer_size;
		dvrconf.dst_buf.almost_full_gate = fifoconf->almost_full_gate;
	}

	if ((ret = GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf))) < 0)
		return ret;
	if ((ret = GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Run, NULL, 0)) < 0)
		return ret;
	return ret;
}

static int fifo_unlink_muxts(GxFifo *fifo, void *configure)
{
	return 0;
}

static int fifo_link_muxts(GxFifo *fifo, void *configure)
{
	return 0;
}

static int fifo_reset_muxts(GxFifo *fifo)
{
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;
	GxDvrProperty_Reset dvrreset = { DVR_MOD_TSR };

	return GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_Reset, &dvrreset, sizeof(dvrreset));
}

static size_t fifo_write_muxts(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	int ret = 0;
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0) {
		return 0;
	}

	ret = GxAVModuleWrite(priv->dev, priv->module, buffer, len, timeout_us);
	if(ret < 0 )
		return 0;

	return ret;
}

static size_t fifo_write_pts_muxts(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us)
{
	return 0;
}

static size_t fifo_read_muxts(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	int ret = 0;
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0) {
		return 0;
	}

	ret = GxAVModuleRead(priv->dev, priv->module, buffer, len, timeout_us);
	if(ret < 0 )
		return 0;

	return ret;
}

static size_t fifo_read_pts_muxts(GxFifo *fifo,
		unsigned char *buffer, unsigned int len, int *pts, int timeout_us)
{
	return 0;
}

static size_t fifo_peek_muxts(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	return 0;
}

static int fifo_roll_back_muxts(GxFifo *fifo, int len)
{
	return 0;
}

static size_t fifo_getlen_muxts(GxFifo *fifo)
{
	int ret = 0;
	GxDvrProperty_Config dvrconf = {0};
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0) {
		return 0;
	}

	ret = GxAVGetProperty(priv->dev, priv->module, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf));
	if (ret < 0)
		return 0;

	return dvrconf.src_buf.buffer_len;
}

static size_t fifo_shallow_read(GxFifo *fifo, unsigned char **start, unsigned int len, int timeout_us)
{
	int ret = 0;
	GxDvrProperty_PtrInfo info = {0};
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0)
		return 0;

	info.blocklen = len;
	ret = GxAVGetProperty(priv->dev, priv->module, GxDvrPropertyID_PtrInfo, &info, sizeof(info));
	if (ret < 0)
		return 0;

	*start = (void *)info.paddr;
	return info.size;
}

static int fifo_shallow_write(GxFifo *fifo, unsigned char *start,  unsigned int len)
{
	GxDvrProperty_PtrInfo info = {0};
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0)
		return 0;

	info.paddr = (unsigned int)start;
	info.size  = len;
	info.blocklen = len;
	return GxAVSetProperty(priv->dev, priv->module, GxDvrPropertyID_PtrInfo, &info, sizeof(info));
}

static int fifo_shallow_info(GxFifo *fifo, GxFifoShallowInfo *info)
{
	FifoPrivMuxts* priv = (FifoPrivMuxts *) fifo->priv;

	if (priv->module == 0)
		return -1;

	info->security_flag = priv->hwbuf_security;
	info->block_size    = priv->block_size;
	info->ptr_enable    = priv->hwbuf_ptr_en;
	return 0;
}

GxFifoClass gx_fifo_muxts = {
	.create   = fifo_create_muxts,
	.destroy  = fifo_destroy_muxts,
	.config   = fifo_config_muxts,
	.link     = fifo_link_muxts,
	.unlink   = fifo_unlink_muxts,
	.reset    = fifo_reset_muxts,
	.read     = fifo_read_muxts,
	.readpts  = fifo_read_pts_muxts,
	.write    = fifo_write_muxts,
	.writepts = fifo_write_pts_muxts,
	.peek     = fifo_peek_muxts,
	.get_len  = fifo_getlen_muxts,
	.rollback = fifo_roll_back_muxts,

	.shallow_read  = fifo_shallow_read,
	.shallow_write = fifo_shallow_write,
	.shallow_info  = fifo_shallow_info,
};
