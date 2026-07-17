/**
 *
 * @file        gxsubtitle_demux.c
 * @brief
 * @version     1.1.0
 * @date        04/08/2010 04:28:11 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gx_mem.h"
#include "gxcore.h"
#include "assert.h"
#include "gxavdev.h"
#include "sub_demux.h"
#include "sub_debug.h"
#include "subtitle/gxsubtitle.h"

#if ((SIMULATOR & DEMUX_SIMULATOR) != DEMUX_SIMULATOR)

#define GXSUBTITLE_PES_PACKET_LEN   (64*1024)

struct gxsub_dmx {
    int32_t     dev;
    handle_t    demux;
    handle_t    front_end;
    int32_t     slot_id;
    int32_t     filter_id;
    uint32_t    read_pos;
    uint32_t    write_pos;
    uint8_t     bit_index;
    uint8_t     pes[GXSUBTITLE_PES_PACKET_LEN];
};

static handle_t dmx_open(uint32_t demux_id)
{
    int32_t                     ret;
    struct gxsub_dmx*           dmx;
    //GxDemuxProperty_ConfigDemux param;
    GxDemuxProperty_Slot        slot = {0,};
    GxDemuxProperty_Filter      filter = {0, };

    dmx = (struct gxsub_dmx*)CALLOC(1, sizeof(struct gxsub_dmx));
    if (dmx == NULL) {
        return E_INVALID_HANDLE;
    }

    dmx->dev    = GxAvdev_CreateDevice(0);
    dmx->demux  = GxAvdev_OpenModule(dmx->dev, GXAV_MOD_DEMUX, demux_id);

/*
    param.source            = DEMUX_TS1;
    param.ts_select         = FRONTEND;
    param.stream_mode       = DEMUX_PARALLEL;
    param.time_gate         = 0xf;
    param.byt_cnt_err_gate  = 0x03;
    param.sync_loss_gate    = 0x03;
    param.sync_lock_gate    = 0x03;

    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                          GxDemuxPropertyID_Config, &param,
                          sizeof(GxDemuxProperty_ConfigDemux));
    ASSERT(ret == 0);
*/
    slot.type = DEMUX_SLOT_PES;

    ret = GxAVGetProperty(dmx->dev, dmx->demux,
                          GxDemuxPropertyID_SlotAlloc,
                          &slot, sizeof(GxDemuxProperty_Slot));
    ASSERT(ret == 0);

    dmx->slot_id    = slot.slot_id;
    filter.slot_id  = dmx->slot_id;

    ret = GxAVGetProperty(dmx->dev, dmx->demux,
                          GxDemuxPropertyID_FilterAlloc,
                          &filter, sizeof(GxDemuxProperty_Filter));
    ASSERT(ret == 0);

    dmx->filter_id = filter.filter_id;

    return (handle_t)dmx;
}

static int32_t dmx_close(handle_t handle)
{
    int32_t                 ret;
    GxDemuxProperty_Slot    slot = {0,};
    GxDemuxProperty_Filter  filter = {0, };
    struct gxsub_dmx*       dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return E_FAILURE;

	if (dmx->filter_id >= 0) {
        filter.filter_id = dmx->filter_id;
        ret = GxAVSetProperty(dmx->dev, dmx->demux,
                              GxDemuxPropertyID_FilterFree,
                              (void*)&filter, sizeof(GxDemuxProperty_Filter));
        ASSERT(ret == 0);

        dmx->filter_id = -1;
    }

    if (dmx->slot_id >= 0) {
        slot.slot_id = dmx->slot_id;
        ret = GxAVSetProperty(dmx->dev, dmx->demux,
                              GxDemuxPropertyID_SlotFree,
                              &slot, sizeof(GxDemuxProperty_Slot));
        ASSERT(ret == 0);
        dmx->slot_id = -1;
    }

    

    GxAvdev_CloseModule(dmx->dev, dmx->demux);

    FREE(dmx);

    GxAvdev_DestroyDevice(0);

    return E_OK;
}

static int64_t  dmx_get_stc(handle_t handle)
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
    ASSERT(ret == 0);
	ret = GxAVGetProperty(dev, stc_mod, GxSTCPropertyID_TimeResolution, &resolution, sizeof(GxSTCProperty_TimeResolution));
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

static int32_t dmx_set_pid(handle_t handle, uint16_t pid)
{
    int32_t                 ret;
    uint32_t i = 0;
    struct gxsub_dmx*       dmx = (struct gxsub_dmx*)handle;
    GxDemuxProperty_Slot    slot = {0,};
    GxDemuxProperty_Filter  filter = {0, };
	
    if(dmx == NULL)
        return E_FAILURE;

    slot.slot_id    = dmx->slot_id;
    slot.pid        = pid;
    slot.type       = DEMUX_SLOT_PES;
    slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                          GxDemuxPropertyID_SlotConfig,
                          &slot, sizeof(GxDemuxProperty_Slot));
    if (ret != 0) {
        return E_FAILURE;
    }

    for (i = 0; i < 18; i++) {
        filter.key[i].value = 0;
        filter.key[i].mask  = 0;
    }

    filter.key[0].value = 0xbd;
    filter.key[0].mask  = 0xff;
    filter.depth        = 1;
    filter.slot_id      = dmx->slot_id;
    filter.filter_id    = dmx->filter_id;
    filter.flags        = DMX_EQ;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                          GxDemuxPropertyID_FilterConfig,
                          &filter, sizeof(GxDemuxProperty_Filter));
    if (ret != 0) {
        return E_FAILURE;
    }

    return E_OK;
}
static int32_t dmx_start(handle_t handle)
{
    int32_t                         ret;
    struct gxsub_dmx*               dmx = (struct gxsub_dmx*)handle;
    GxDemuxProperty_Slot            slot = {0,};
    GxDemuxProperty_Filter          filter = {0,};
    GxDemuxProperty_FilterFifoReset fifo;

    if(dmx == NULL)
        return E_FAILURE;

    slot.slot_id = dmx->slot_id;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                            GxDemuxPropertyID_SlotEnable,
                            &slot, sizeof(GxDemuxProperty_Slot));
    ASSERT(ret == 0);


#if 1
    fifo.filter_id = dmx->filter_id;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                            GxDemuxPropertyID_FilterFIFOReset,
                            &fifo, sizeof(GxDemuxProperty_FilterFifoReset));
    ASSERT(ret == 0);
#endif
    filter.slot_id      = dmx->slot_id;
    filter.filter_id    = dmx->filter_id;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                            GxDemuxPropertyID_FilterEnable,
                            &filter, sizeof(GxDemuxProperty_Filter));
    ASSERT(ret == 0);

    return E_OK;
}

static int32_t dmx_stop(handle_t handle)
{
    int32_t                 ret;
    GxDemuxProperty_Slot    slot = {0,};
    GxDemuxProperty_Filter  filter = {0,};
    struct gxsub_dmx*       dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return E_FAILURE;

    slot.slot_id = dmx->slot_id;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                            GxDemuxPropertyID_SlotDisable,
                            &slot, sizeof(GxDemuxProperty_Slot));
    ASSERT(ret == 0);

    filter.filter_id = dmx->filter_id;
    ret = GxAVSetProperty(dmx->dev, dmx->demux,
                            GxDemuxPropertyID_FilterDisable,
                            &filter, sizeof(GxDemuxProperty_Filter));
    ASSERT(ret == 0);

    return E_OK;
}

static size_t  dmx_get_pes(handle_t handle, uint8_t** ptr, size_t size)
{
    int32_t                         ret;
    uint32_t                        event;
    GxDemuxProperty_FilterFifoQuery query;
    GxDemuxProperty_FilterRead      filter_read;
    size_t                          read = 0;
    struct gxsub_dmx*               dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return E_FAILURE;

    ret = GxAVWaitEvents(dmx->dev, dmx->demux,
                    EVENT_DEMUX0_FILTRATE_PES_END, 500000, &event);
    if (ret != 0) {
        return 0;
    }

    ret = GxAVGetProperty(dmx->dev, dmx->demux,
                    GxDemuxPropertyID_FilterFIFOQuery,
                    &query, sizeof(GxDemuxProperty_FilterFifoQuery));
    ASSERT(ret == 0);
    if (((query.state >> dmx->filter_id) & 1) == 1) {
        filter_read.filter_id  = dmx->filter_id;
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
    struct gxsub_dmx* dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return E_FAILURE;

    if (dmx->read_pos >= dmx->write_pos) {
        dmx->read_pos  = 0;
        dmx->write_pos = dmx_get_pes(handle, NULL,  GXSUBTITLE_PES_PACKET_LEN);
    }
    return dmx->pes[(dmx->read_pos)++];
}


static void  dmx_skipbytes(handle_t handle, uint16_t size)
{
    int32_t             len;
    struct gxsub_dmx*   dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return;

    len = size - dmx->write_pos + dmx->read_pos;

    if (len <= 0) {
        dmx->read_pos += size;
    } else {
        while(len > 0) {
            dmx->write_pos = dmx_get_pes(handle, NULL, GXSUBTITLE_PES_PACKET_LEN);
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
    struct gxsub_dmx*   dmx = (struct gxsub_dmx*)handle;

    if((dmx == NULL) || (ptr == NULL))
        return;

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
            dmx->write_pos = dmx_get_pes(handle, NULL, GXSUBTITLE_PES_PACKET_LEN);
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
    struct gxsub_dmx*   dmx = (struct gxsub_dmx*)handle;

    if((dmx == NULL) || (ptr == NULL))
        return E_FAILURE;

    data = ptr;

    switch(dmx->bit_index) {
    case 0:
        value = (data[*index] & 0xC0) >> 6;
        dmx->bit_index = 2;
        break;
    case 2:
        value = (data[*index] & 0x30) >> 4;
        dmx->bit_index = 4;;
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
    struct gxsub_dmx* dmx = (struct gxsub_dmx*)handle;

    if(dmx == NULL)
        return E_FAILURE;

    dmx->bit_index = 0;

    return index + 1;
}


GxSubtitle_DemuxOps  gxsub_demux = {
                                        dmx_open,
                                        dmx_close,
                                        dmx_start,
                                        dmx_stop,
                                        dmx_get_stc,
                                        dmx_set_pid,
                                        dmx_get_pes,
                                        dmx_nextbyte,
                                        dmx_skipbytes,
                                        dmx_copybytes,
                                        dmx_get2bits,
                                        dmx_alignbyte
                                    };
#endif

