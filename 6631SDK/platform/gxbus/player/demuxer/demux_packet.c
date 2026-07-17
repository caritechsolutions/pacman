#include "gx_demux.h"
#include "gx_decoder.h"
#include "stheader.h"
#include "gx_subreader.h"
#include "gx_system.h"
#include "gx_subtitle.h"
#include "gx_device.h"

static void fifo_to_packet(GxFifo *fifo, int offset, GxDemuxPacket *dp, int len)
{
	dp->buffer = fifo->buffer + offset;
	if (offset + len > fifo->size) {
		dp->buffer2     = fifo->buffer;
		dp->buffer_len  = fifo->size - offset;
		dp->buffer2_len = len - dp->buffer_len;
	}
	else {
		dp->buffer_len  = len;
		dp->buffer2_len = 0;
	}

	dp->fifo = fifo;
	fifo->alloc_out = offset + len;
}

GxDemuxPacket *GxDemuxPacket_Create(GxDemuxer  * demuxer, GxFifo *fifo, int len)
{
	GxDemuxPacket *dp;
	size_t free_size;

	if (len <= 0)
		return NULL;

	if (fifo != NULL) {
		free_size = fifo->size - GxFifo_GetLength(fifo);

		if (len < free_size)
			return NULL;
	}

	dp = GxDemuxPool_PacketNew(&demuxer->pool);

	if(dp) {
		dp->len        = len;
		dp->pool_size  = len;
		dp->next       = NULL;
		dp->pts        = 0;
		dp->endpts     = GX_NOPTS_VALUE;
		dp->pos        = 0;
		dp->is_start_switch_dp = 0;
		dp->flags      = 0;
		dp->buffer     = NULL;
		dp->buffer2    = NULL;
		dp->write_size  = 0;
		dp->demuxer    = demuxer;
		memset(&dp->ctrlinfo, 0, sizeof(dp->ctrlinfo));

		if (fifo)
			fifo_to_packet(fifo, fifo->alloc_out, dp, len);
		else
			GxDemuxPool_PacketAllocBuffer(&demuxer->pool, dp);

		if(dp->buffer == NULL){
			GxDemuxPacket_Destroy(dp);
			dp = NULL;
		}
	}

	return dp;
}

GxDemuxPacket *GxDemuxPacket_Resize(GxDemuxer  * demuxer, GxDemuxPacket* dp, int len)
{
	GxDemuxPacket *p = NULL;
	GxFifo *fifo;

	if (dp == NULL)
		return NULL;

	if(len == dp->len)
		return dp;

	fifo = dp->fifo;
	if (fifo == NULL) {
		if (len > 0) {
			p = GxDemuxPacket_Create(demuxer, NULL, len);
			if (p == NULL) {
				gxlogf("Packet error len = %d, dp->len = %d\n", len, dp->len);
				GxDemuxPacket_Destroy(dp);
				return NULL;
			}
			else {
				memcpy(p->buffer, dp->buffer, p->pool_size);
				p->write_size = dp->write_size;
				p->pts        = dp->pts;
				p->endpts     = dp->endpts;
				p->pos        = dp->pos;
				p->flags      = dp->flags;
				p->next       = dp->next;
				GxDemuxPacket_Destroy(dp);
				return p;
			}
		}
		else {
			dp->len = 0;
			return dp;
		}
	}
	else {
		size_t free_size = fifo->size - GxFifo_GetLength(fifo);

		if (len > free_size)
			fifo_to_packet(fifo, dp->buffer - fifo->buffer, dp, len);

		return dp;
	}
}

int GxDemuxPacket_Write(GxDemuxPacket *dp, const uint8_t *data, int len)
{
	int size = 0;

	if(dp && data){
		if (len <= 0)
			return 0;

		if (dp->buffer[dp->pool_size] != 0xa0) {
			gxlogd("ddddddddddddddddddddddddddadf asdf asdfasdf asd 1, pool_size = %d, dp->write_size = %d, len=%d, size = %d\n",
					dp->pool_size, dp->write_size, len, size);
			while (1) GxCore_ThreadDelay(10);
		}
		size = GXMIN(dp->len - dp->write_size, len);
		memcpy(dp->buffer + dp->write_size, data, size);
		if (dp->buffer[dp->pool_size] != 0xa0) {
			gxlogd("ddddddddddddddddddddddddddadf asdf asdfasdf asd 2, pool_size = %d, dp->write_size = %d, len=%d, size = %d\n",
					dp->pool_size, dp->write_size, len, size);
			while (1) GxCore_ThreadDelay(10);
		}
		dp->write_size += size;

		return size;
	}

	return 0;
}

int GxDemuxPacket_Skip(GxDemuxPacket *dp, int len)
{
	int size;

	if(dp && len > 0) {
		if (dp->write_size + len <= dp->len)
			size = len;
		else
			size = dp->len - (dp->write_size + len);

		dp->write_size += size;

		return size;
	}

	return 0;
}

void GxDemuxPacket_Destroy(GxDemuxPacket *dp)
{
	if(dp && dp->demuxer)
		GxDemuxPool_PacketFree(&dp->demuxer->pool, dp);
}

