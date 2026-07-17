#include "avfifo.h"
#include "gx_mediafilter.h"
#include "gx_muxer.h"

#define MUXER_PKT_MAX (4096)

GxFifo *fifo_create_muxer(size_t size,int subflag)
{
	GxFifo *gxfifo;
	AvFIFO *avfifo;

	if ((avfifo = AvFIFO_New(MUXER_PKT_MAX)) == NULL)
		return NULL;
	if ((gxfifo = (GxFifo *) av_mallocz(sizeof(GxFifo))) == NULL) {
		AvFIFO_Free(avfifo);
		return NULL;
	}

	gxfifo->priv = avfifo;

	return gxfifo;
}

int fifo_destroy_muxer(GxFifo *fifo)
{
	if (fifo) {
		GxMuxerPacket *pkt = NULL;
		while(AvFIFO_Get((AvFIFO *) fifo->priv, (uint8_t *)pkt, sizeof(pkt))) {
			GxMuxerPacket_Destroy(pkt);	
		}

		AvFIFO_Free((AvFIFO *) fifo->priv);
		av_free(fifo);
	}

	return 0;
}

int fifo_config_muxer(GxFifo *fifo, void *configure)
{
	return 0;
}

size_t fifo_write_muxer(GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us)
{
	if (fifo && buffer){
		if ((AvFIFO_FreeLen((AvFIFO *) fifo->priv)) <= sizeof(void*)) {
			gxlogd("%s, oom (1) !!!\n", __func__);
			return 0;
		}

		if (len == sizeof(GxMuxerPacket)) {
			GxMuxerPacket *ipkt = (GxMuxerPacket *)buffer;
			GxMuxerPacket *pkt  = GxMuxerPacket_New(ipkt->size);
			if (pkt == NULL) {
				gxlogd("%s, oom (2) !!!\n", __func__);
				return 0;
			}
			pkt->stream_index = ipkt->stream_index;
			pkt->pts = ipkt->pts;
			memcpy(pkt->data, ipkt->data, ipkt->size);
			AvFIFO_Put((AvFIFO *) fifo->priv, (uint8_t *)(&pkt), sizeof(pkt));
			return len;
		}
	}
		
	return 0;
}

size_t fifo_write_pts_muxer(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts,int timeout_us)
{
	return 0;
}

size_t fifo_read_muxer(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	if (fifo)
		return AvFIFO_Get((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_peek_muxer(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	if (fifo)
		return AvFIFO_Peek((AvFIFO *) fifo->priv, buffer, len);
	else
		return 0;
}

size_t fifo_getlen_muxer(GxFifo *fifo)
{
	if (fifo)
		return AvFIFO_Len((AvFIFO *) fifo->priv);
	else
		return 0;
}

size_t fifo_getcap_muxer(GxFifo *fifo)
{
	if (fifo)
		return MUXER_PKT_MAX;
	else
		return 0;
}

int fifo_reset_muxer(GxFifo *fifo)
{
	if (fifo) {
		GxMuxerPacket *pkt = NULL;
		while(AvFIFO_Get((AvFIFO *) fifo->priv, (uint8_t *)pkt, sizeof(pkt))) {
			GxMuxerPacket_Destroy(pkt);	
		}
		AvFIFO_Reset((AvFIFO *) (fifo->priv));
	}

	return 0;
}

GxFifoClass gx_fifo_muxer = {
	.create   = fifo_create_muxer,
	.destroy  = fifo_destroy_muxer,
	.config   = fifo_config_muxer,
	.reset    = fifo_reset_muxer,
	.read     = fifo_read_muxer,
	.write    = fifo_write_muxer,
	.writepts = fifo_write_pts_muxer,
	.peek     = fifo_peek_muxer,
	.get_len  = fifo_getlen_muxer,
	.get_cap  = fifo_getcap_muxer,
	.link     = NULL,
	.unlink   = NULL,
};


