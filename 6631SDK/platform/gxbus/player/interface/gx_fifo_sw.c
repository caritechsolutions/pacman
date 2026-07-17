#include "avfifo.h"
#include "gx_mediafilter.h"

GxFifo *fifo_create_sw(size_t size,int subflag)
{
	GxFifo *gxfifo;
	AvFIFO *avfifo;

	if ((avfifo = AvFIFO_New(size)) == NULL)
		return NULL;
	if ((gxfifo = (GxFifo *) av_mallocz(sizeof(GxFifo))) == NULL) {
		AvFIFO_Free(avfifo);
		return NULL;
	}	
	
	gxfifo->priv   = avfifo;

	return gxfifo;
}

int fifo_destroy_sw(GxFifo *fifo)
{
	if (fifo) {
		AvFIFO_Free((AvFIFO *) fifo->priv);
		av_free(fifo);
		fifo = NULL;
	}
	return 0;
}

int fifo_config_sw(GxFifo *fifo, void *configure)
{
	return 0;
}

size_t fifo_write_sw(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	if (fifo && buffer){
		return AvFIFO_Put((AvFIFO *) fifo->priv, buffer, len);
	}
	else
		return 0;
}

size_t fifo_write_pts_sw(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us)
{
	if (fifo && buffer)
		return AvFIFO_Put((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_read_sw(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	if (fifo)
		return AvFIFO_Get((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_read_pts_sw(GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us)
{
	if (fifo)
		return AvFIFO_Get((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_peek_sw(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	if (fifo)
		return AvFIFO_Peek((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_getlen_sw(GxFifo *fifo)
{
	if (fifo)
		return AvFIFO_Len((AvFIFO *) fifo->priv);
	else
		return 0;
}

int fifo_reset_sw(GxFifo *fifo)
{
	if (fifo)
		AvFIFO_Reset((AvFIFO *) (fifo->priv));
	return 0;
}

GxFifoClass gx_fifo_sw = {
	.create   = fifo_create_sw,
	.destroy  = fifo_destroy_sw,
	.config   = fifo_config_sw,
	.reset    = fifo_reset_sw,
	.read     = fifo_read_sw,
	.readpts  = fifo_read_pts_sw,
	.write    = fifo_write_sw,
	.writepts = fifo_write_pts_sw,
	.peek     = fifo_peek_sw,
	.get_len  = fifo_getlen_sw,
	.get_cap  = NULL,
	.link     = NULL,
	.unlink   = NULL,
};


