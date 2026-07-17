#include "gxcore.h"
#include "assert.h"
#include "gxavdev.h"
#include "gxaribcc.h"
#include "aribcc.h"
#include "module/app_log.h"

#if ARIBCC_SUPPORT

#define GXARIBCC_PES_PACKET_LEN   (64*1024)

struct gxaribcc_dmx {
	int32_t     dev;
	handle_t    demux;
	int32_t     demux_id;
	GxDemuxProperty_Slot        slot ;
	GxDemuxProperty_Filter      filter;
	uint32_t    read_pos;
	uint32_t    write_pos;
	uint8_t     bit_index;
	uint8_t     pes[GXARIBCC_PES_PACKET_LEN];
};

static handle_t dmx_open(uint32_t demux_id,uint16_t pid)
{
	int32_t                     ret;
	struct gxaribcc_dmx*           dmx;
	GxDemuxProperty_Slot        slot = {0,};
	GxDemuxProperty_Filter      filter = {0, };

	dmx = CALLOC(1, sizeof(struct gxaribcc_dmx));
	if (dmx == NULL) {
		return E_INVALID_HANDLE;
	}

	memset(dmx,0,sizeof(struct gxaribcc_dmx));

	dmx->dev    = GxAvdev_CreateDevice(0);
	dmx->demux  = GxAvdev_OpenModule(dmx->dev, GXAV_MOD_DEMUX, demux_id);

	slot.pid        = pid;
	slot.type = DEMUX_SLOT_PES;
	slot.flags = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	ret = GxAVGetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_SlotAlloc,
			&slot, sizeof(GxDemuxProperty_Slot));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_SlotAlloc failed!!!!!!!\n");
    ASSERT(ret == 0);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_SlotConfig,
			&slot, sizeof(GxDemuxProperty_Slot));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_SlotConfig failed!!!!!!!\n");
	ASSERT(ret == 0);

	dmx->slot = slot;

	filter.slot_id  = slot.slot_id;
	ret = GxAVGetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterAlloc,
			&filter, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterAlloc failed!!!!!!!\n");
	ASSERT(ret == 0);

	filter.depth        = 0;
	filter.flags        = DMX_EQ;
	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterConfig,
			&filter, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterConfig failed!!!!!!!\n");
	ASSERT(ret == 0);

	dmx->filter = filter;
	dmx->demux_id = demux_id;

	return (handle_t)dmx;
}

static int32_t dmx_close(handle_t handle)
{
	int32_t                 ret;
	struct gxaribcc_dmx*       dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterFree,
			&dmx->filter, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterFree failed!!!!!!!\n");
	ASSERT(ret == 0);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_SlotFree,
			&dmx->slot, sizeof(GxDemuxProperty_Slot));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_SlotFree failed!!!!!!!\n");
	ASSERT(ret == 0);

	GxAvdev_CloseModule(dmx->dev, dmx->demux);

	FREE(dmx);

	GxAvdev_DestroyDevice(0);

	return E_OK;
}

static uint32_t  dmx_get_stc(handle_t handle)
{
	int32_t                      ret;
	GxDemuxProperty_ReadStc      readstc;
	GxSTCProperty_TimeResolution resolution;
	int64_t                      stc_value;

	int dev        = GxAvdev_CreateDevice(0);
	int demux_mod  = GxAvdev_OpenModule(dev, GXAV_MOD_DEMUX, 0);
	int stc_mod    = GxAvdev_OpenModule(dev, GXAV_MOD_STC, 0);
	int stc_freq   = 0;

	ret = GxAVGetProperty(dev, demux_mod, GxDemuxPropertyID_ReadStc, &readstc, sizeof(GxDemuxProperty_ReadStc));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_ReadStc failed!!!!!!!\n");
	ASSERT(ret == 0);
	ret = GxAVGetProperty(dev, stc_mod, GxSTCPropertyID_TimeResolution, &resolution, sizeof(GxSTCProperty_TimeResolution));
    if(ret != 0)
        app_log_error("aribcc GxSTCPropertyID_TimeResolution failed!!!!!!!\n");
	ASSERT(ret == 0);

	GxAvdev_CloseModule(dev, demux_mod);
	GxAvdev_CloseModule(dev, stc_mod);
	GxAvdev_DestroyDevice(0);

	stc_freq  = resolution.freq_HZ/1000;
	stc_value = readstc.stc_value;
	if(stc_freq == 45)
		return stc_value*2;
	else
		return (stc_value*90/stc_freq);
}


static int32_t dmx_start(handle_t handle)
{
	int32_t                         ret;
	struct gxaribcc_dmx*               dmx = (struct gxaribcc_dmx*)handle;
	GxDemuxProperty_FilterFifoReset fifo;

	ASSERT(dmx != NULL);

	fifo.filter_id = dmx->filter.filter_id;
	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterFIFOReset,
			&fifo, sizeof(GxDemuxProperty_FilterFifoReset));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterFIFOReset failed!!!!!!!\n");
	ASSERT(ret == 0);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterEnable,
			&dmx->filter, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterEnable failed!!!!!!!\n");
	ASSERT(ret == 0);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_SlotEnable,
			&dmx->slot, sizeof(GxDemuxProperty_Slot));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_SlotEnable failed!!!!!!!\n");
	ASSERT(ret == 0);

	return E_OK;
}

static int32_t dmx_stop(handle_t handle)
{
	int32_t                 ret;
	struct gxaribcc_dmx*       dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterDisable,
			&dmx->filter, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_FilterDisable failed!!!!!!!\n");
	ASSERT(ret == 0);

	ret = GxAVSetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_SlotDisable,
			&dmx->slot, sizeof(GxDemuxProperty_Slot));
    if(ret != 0)
        app_log_error("aribcc GxDemuxPropertyID_SlotDisable failed!!!!!!!\n");
	ASSERT(ret == 0);

	return E_OK;
}

static size_t  dmx_get_pes(handle_t handle, uint8_t** ptr, size_t size)
{
	int32_t                         ret;
	uint32_t                        event,eventval;
	GxDemuxProperty_FilterFifoFreeSize filter_avail = {0};
	GxDemuxProperty_FilterFifoQuery query = {0};
	GxDemuxProperty_FilterRead      filter_read = {0};
	size_t                          read = 0;
	struct gxaribcc_dmx*               dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	filter_avail.filter_id = dmx->filter.filter_id;/* get pes filter avail data len */
	ret = GxAVGetProperty(dmx->dev, dmx->demux,
		GxDemuxPropertyID_FilterFifoFreeSize,
		&filter_avail, sizeof(GxDemuxProperty_FilterFifoFreeSize));
	if(ret < 0)
		return 0;

	if(filter_avail.free_size != 0) {
		filter_read.filter_id  = dmx->filter.filter_id;
		filter_read.max_size   = size;
		filter_read.buffer     = dmx->pes;

		ret = GxAVGetProperty(dmx->dev, dmx->demux,
				GxDemuxPropertyID_FilterRead,
				&filter_read, sizeof(GxDemuxProperty_FilterRead));
		ASSERT(ret == 0);

		read = filter_read.read_size;
		if (ptr != NULL)
			*ptr = dmx->pes;

		return read;
	}

	switch(dmx->demux_id) {
		case 0:
			event = EVENT_DEMUX0_FILTRATE_PES_END;
			break;
		case 1:
			event = EVENT_DEMUX1_FILTRATE_PES_END;
			break;
		default:
			event = EVENT_DEMUX0_FILTRATE_PES_END;
			break;
	}

	ret = GxAVWaitEvents(dmx->dev, dmx->demux,
			event, 500000, &eventval);
	if (ret != 0) {
		return 0;
	}

	ret = GxAVGetProperty(dmx->dev, dmx->demux,
			GxDemuxPropertyID_FilterFIFOQuery,
			&query, sizeof(GxDemuxProperty_FilterFifoQuery));
	ASSERT(ret == 0);

	if (((query.state >> dmx->filter.filter_id) & 1) == 1) {
		filter_read.filter_id  = dmx->filter.filter_id;
		filter_read.max_size   = size;
		filter_read.buffer     = dmx->pes;

		ret = GxAVGetProperty(dmx->dev, dmx->demux,
				GxDemuxPropertyID_FilterRead,
				&filter_read, sizeof(GxDemuxProperty_FilterRead));
		ASSERT(ret == 0);

		read = filter_read.read_size;
		if (ptr != NULL) {
			*ptr = dmx->pes;
		}
	}
	return read;
}

static uint8_t dmx_nextbyte(handle_t handle)
{
	struct gxaribcc_dmx* dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	if (dmx->read_pos >= dmx->write_pos) {
		dmx->read_pos  = 0;
		dmx->write_pos = dmx_get_pes(handle, NULL,  GXARIBCC_PES_PACKET_LEN);
	}
	return dmx->pes[(dmx->read_pos)++];
}


static void  dmx_skipbytes(handle_t handle, uint16_t size)
{
	int32_t             len;
	struct gxaribcc_dmx*   dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	len = size - dmx->write_pos + dmx->read_pos;

	if (len <= 0) {
		dmx->read_pos += size;
	} else {
		while(len > 0) {
			dmx->write_pos = dmx_get_pes(handle, NULL, GXARIBCC_PES_PACKET_LEN);
			dmx->read_pos = 0;

			if (len > dmx->write_pos) {
				len -= dmx->write_pos;
			} else {
				dmx->read_pos = len;
				return;
			}
		}
	}
	return;
}

static void dmx_copybytes(handle_t handle, uint8_t* ptr, uint16_t size)
{
	int32_t             x;
	uint8_t*            buf;
	int32_t             len;
	struct gxaribcc_dmx*   dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);
	ASSERT(ptr != NULL);

	buf = ptr;
	x = dmx->write_pos - dmx->read_pos;
	len = size - x;

	if (len <= 0) {
		memcpy(buf, dmx->pes + dmx->read_pos, size);
		dmx->read_pos += size;
		buf += x;
	} else {
		memcpy(buf, dmx->pes  + dmx->read_pos, x);
		buf += x;
		while(len > 0) {
			dmx->write_pos = dmx_get_pes(handle, NULL, GXARIBCC_PES_PACKET_LEN);
			dmx->read_pos = 0;

			if (len > dmx->write_pos) {
				memcpy(buf, dmx->pes, dmx->write_pos);
				len -= dmx->write_pos;
				buf += dmx->write_pos;
			} else {
				memcpy(buf, dmx->pes, len);
				dmx->read_pos = len;
				return;
			}
		}
	}
	return;
}

static uint8_t dmx_get2bits(handle_t handle, uint8_t* ptr, uint32_t* index)
{
	uint8_t*            data;
	uint8_t             value = 0;
	struct gxaribcc_dmx*   dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	data = ptr;

	switch(dmx->bit_index) {
		case 0:
			value = (data[*index] & 0xC0) >> 6;
			dmx->bit_index = 2;
			break;
		case 2:
			value = (data[*index] & 0x30) >> 4;
			dmx->bit_index = 4;
			break;
		case 4:
			value = (data[*index] & 0xC) >> 2;
			dmx->bit_index = 6;
			break;
		case 6:
			value = data[*index] & 0x3;
			dmx->bit_index = 0;
			(*index)++;
			break;
		default:
			break;
	}

	return value;
}

static uint32_t dmx_alignbyte(handle_t handle, uint32_t index)
{
	struct gxaribcc_dmx* dmx = (struct gxaribcc_dmx*)handle;

	ASSERT(dmx != NULL);

	dmx->bit_index = 0;

	return index + 1;
}

GxAribCC_DemuxOps  aribcc_demux = {
	.open		= dmx_open,
	.close		= dmx_close,
	.start		= dmx_start,
	.stop		= dmx_stop,
	.get_stc		= dmx_get_stc,
	.get_pes		= dmx_get_pes,
	.nextbyte		= dmx_nextbyte,
	.skipbytes	= dmx_skipbytes,
	.copybytes	= dmx_copybytes,
	.get2bits		= dmx_get2bits,
	.alignbyte	= dmx_alignbyte,
};
#endif

