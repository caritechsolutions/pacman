/**
 * @file gxupdate_ts_dvbc.c
 * @author lixb
 * @brief goxceed升级架构DVBC TS传输层定义
 */
#include <assert.h>
#include "gxavdev.h"

#include "gxcore.h"
#include "update/gxupdate_stream.h"
#include "update/gxupdate_protocol_ts.h"

#include "gxupdate_debug.h"
#define AV_DEVICE_ID                    (0);

struct update_ts {
    char*                           name;
    int32_t                         dev;
    handle_t                        front_end;
    handle_t                        demux;
    int32_t                         slot_id;
    int32_t                         filter_id;
    int32_t                         pid;
    uint8_t                         table_id;
    int32_t                         section_number;
    int32_t                         last_section_number;
    size_t                          section_offset;
};

static struct update_ts  ts_info[GXUPDATE_MAX_NUM_TS_OPEN] = {{0,},};

static void init_demux(struct update_ts* ts, GxUpdate_TSInfo* ts_param)
{
    int32_t                     ret = 0;
    GxDemuxProperty_Slot        slot = {0};
    GxDemuxProperty_Filter      filter = {0};

    ts->dev = GxAvdev_CreateDevice(0);
    ASSERT(ts->dev >= 0);

// TODO:
//ts->front_end = GxAvdev_OpenModule(ts->dev, GXAV_MOD_FRONTEND, 0);
    ASSERT(ts->front_end >= 0);

    ts->demux = GxAvdev_OpenModule(ts->dev, GXAV_MOD_DEMUX, ts_param->demux_id);
    ASSERT(ts->demux >= 0);

    ts_param->front_end.dev = ts->dev;
    ts_param->front_end.demux = ts->demux;
    GxFrontend_SetTp(&ts_param->front_end);

    slot.type = DEMUX_SLOT_PSI;
    ret = GxAVGetProperty(ts->dev, ts->demux,
                          GxDemuxPropertyID_SlotAlloc,
						  (void*)&slot, sizeof(GxDemuxProperty_Slot));
    ASSERT(ret == 0);

    slot.slot_id = ts->slot_id;
    slot.pid = ts->pid;
    slot.type = DEMUX_SLOT_PSI;
    slot.flags = DMX_REPEAT_MODE | DMX_AVOUT_EN;
    ret = GxAVSetProperty(ts->dev, ts->demux,
                          GxDemuxPropertyID_SlotConfig,
						  (void*)&slot, sizeof(GxDemuxProperty_Slot));
    ASSERT(ret == 0);

    ts->slot_id = slot.slot_id;
    filter.slot_id = ts->slot_id;
    ret = GxAVGetProperty(ts->dev, ts->demux,
                          GxDemuxPropertyID_FilterAlloc,
					  (void*)&filter, sizeof(GxDemuxProperty_Filter));
    ASSERT(ret == 0);

    ts->filter_id = filter.filter_id;
}


static void config_demux(struct update_ts* ts, uint8_t* match, uint8_t* mask, uint8_t depth)
{
    int32_t                     i;
    int32_t                     ret;
    GxDemuxProperty_Slot        slot = {0,};
    GxDemuxProperty_Filter      filter = {0, };
    GxDemuxProperty_FilterFifoReset fifo;

    slot.slot_id = ts->slot_id;
	ret = GxAVSetProperty(ts->dev, ts->demux,
	                        GxDemuxPropertyID_SlotDisable,
						    &slot, sizeof(GxDemuxProperty_Slot));
	ASSERT(ret == 0);

	filter.filter_id = ts->filter_id;
	ret = GxAVSetProperty(ts->dev, ts->demux,
	                        GxDemuxPropertyID_FilterDisable,
						    &filter, sizeof(GxDemuxProperty_Filter));

    ASSERT(ret == 0);
    fifo.filter_id = ts->filter_id;
    ret = GxAVSetProperty(ts->dev, ts->demux,
	                        GxDemuxPropertyID_FilterFIFOReset,
						    &fifo, sizeof(GxDemuxPropertyID_FilterFIFOReset));
    ASSERT(ret == 0);

    filter.slot_id = ts->slot_id;
    filter.filter_id = ts->filter_id;
    filter.flags = DMX_EQ;// | DMX_CRC_IRQ;
    for(i = 0; i < depth; i++) {
        filter.key[i].value = match[i];
        filter.key[i].mask = mask[i];
    }
    filter.depth = depth;
	ret = GxAVSetProperty(ts->dev, ts->demux,
	                      GxDemuxPropertyID_FilterConfig,
						(void*)&filter, sizeof(GxDemuxProperty_Filter));


    slot.slot_id = ts->slot_id;
	ret = GxAVSetProperty(ts->dev, ts->demux,
	                        GxDemuxPropertyID_SlotEnable,
						    &slot, sizeof(GxDemuxProperty_Slot));
	ASSERT(ret == 0);

    filter.slot_id      = ts->slot_id;
	filter.filter_id    = ts->filter_id;
	ret = GxAVSetProperty(ts->dev, ts->demux,
	                        GxDemuxPropertyID_FilterEnable,
							&filter, sizeof(GxDemuxProperty_Filter));
	ASSERT(ret == 0);

}

int32_t destroy_demux(struct update_ts* ts)
{
    GxAvdev_CloseModule(ts->dev, ts->front_end);
    GxAvdev_CloseModule(ts->dev, ts->demux);

    GxAvdev_DestroyDevice(0);

    return 0;
}

static handle_t ts_open(const char* name)
{
    int32_t                 i;

    ASSERT(name != NULL);

    if (name == NULL) {
        return (handle_t)E_INVALID_HANDLE;
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_TS_OPEN; i++) {
        if (ts_info[i].name != NULL && strcmp(ts_info[i].name, name) == 0) {
            return (handle_t)&ts_info[i];
        }
    }

    for (i = 0; i < GXUPDATE_MAX_NUM_TS_OPEN; i++) {
        if (ts_info[i].name == NULL) {
            memset(&ts_info[i], 0, sizeof(struct update_ts));
            ts_info[i].name = strdup(name);
            if (ts_info[i].name == NULL) {
                return (handle_t)E_INVALID_HANDLE;
            }


            ts_info[i].dev= GxAvdev_CreateDevice(0);
            if (ts_info[i].dev < 0) {
                GxCore_Free(ts_info[i].name);
                return (handle_t)E_INVALID_HANDLE;
            }
            ts_info[i].section_number = 1;
            return (handle_t)&ts_info[i];
        }
    }

    return (handle_t)E_INVALID_HANDLE;
}


static GxUpdate_Terminal ts_get_type(handle_t handle)
{
    return GXUPDATE_CLIENT;
}

static int32_t ts_set_size(handle_t handle, size_t size)
{
    return E_OK;
}

static int32_t  ts_ioctl(handle_t handle, int32_t key, void* buf, size_t size)
{
    struct update_ts*   ts = (struct update_ts*)handle;

    switch(key) {
    case GXUPDATE_PROTOCOL_TS_CONFIG: {
            GxUpdate_TSInfo*    p = (GxUpdate_TSInfo*)buf;
            ts->table_id = p->table_id;
            ts->pid = p->pid;
            if (size != sizeof(GxUpdate_TSInfo))
                break;
            init_demux(ts, p);
	    }
        return E_OK;
    default:
        break;
    }
    return E_FAILURE;
}

static uint32_t ts_read(handle_t handle, uint8_t* buf, ssize_t* size)
{
#define MAX_UPDATE_TS_FILTER_DEPTH      8
#define MAX_UPDATE_TS_PACKET_LEN        (1024*4)

    int32_t                         ret;
    uint32_t                        event;
    GxDemuxProperty_FilterFifoQuery query;
    GxDemuxProperty_FilterRead      filter_read;
    struct update_ts*               ts = (struct update_ts*)handle;
    uint8_t                         match[MAX_UPDATE_TS_FILTER_DEPTH] = {0,};
    uint8_t                         mask[MAX_UPDATE_TS_FILTER_DEPTH] = {0,};
    uint8_t                         packet[MAX_UPDATE_TS_PACKET_LEN];
    size_t                          packet_length;
    size_t                          data_length;

    size_t                          buffer_length;
    size_t                          read_size = 0;
    size_t                          copy_length;

    ASSERT(buf != NULL);

    match[0] = ts->table_id;
    mask[0] = 0xFF;
    mask[6] = 0xFF;
    mask[7] = 0xFF;

    do {
        match[6] = ts->section_number >> 8;
        match[7] = ts->section_number & 0xFF;

        config_demux(ts, match, mask, MAX_UPDATE_TS_FILTER_DEPTH);

        ret = GxAVWaitEvents(ts->dev, ts->demux,
    					EVENT_DEMUX0_FILTRATE_PSI_END, 2000000, &event);
    	ASSERT(ret == 0);

        ret = GxAVGetProperty(ts->dev, ts->demux,
    			        GxDemuxPropertyID_FilterFIFOQuery,
    			        &query, sizeof(GxDemuxProperty_FilterFifoQuery));

        if ((query.state >> ts->filter_id) == 1) {
            filter_read.filter_id  = ts->filter_id;
            filter_read.max_size   = MAX_UPDATE_TS_PACKET_LEN;
            filter_read.buffer     = packet;

            ret = GxAVGetProperty(ts->dev, ts->demux,
                            GxDemuxPropertyID_FilterRead,
    						&filter_read, sizeof(GxDemuxProperty_FilterRead));
            packet_length = filter_read.read_size;
            ASSERT(packet[0] == ts->table_id);

            data_length = (uint16_t)((packet[1] & 0x0F) << 8) + packet[2] - 11;
            ts->last_section_number = (uint16_t)(packet[8] << 8) + packet[9];

            /*获取有效数据长度*/
            data_length -= ts->section_offset;
            /*获取buf空间实际大小*/
            buffer_length = *size - read_size;

            /*获取实际可复制数据长度*/
            copy_length = data_length > buffer_length ? buffer_length : data_length;


            memcpy(buf + read_size, packet + 10 + ts->section_offset, copy_length);


            if (data_length > buffer_length) {
                /*data_length需要切成两份， 需要过滤两次*/
                ts->section_offset = buffer_length;

            } else {
                /*buffer_length大于或等于data_length,需要接收下一个section*/
                ts->section_number++;
                ts->section_offset = 0;
            }

            read_size += copy_length;
            if (ts->section_number > ts->last_section_number) {
                /*最后一个section，数据已经接收完成*/
                return GXUPDATE_STREAM_FINISH;
            }
        }
    }while(read_size != *size);

    return GXUPDATE_STREAM_CONTINUE;
}

static ssize_t ts_write(handle_t handle, const uint8_t* ptr, size_t size)
{
    return 0;
}


static int32_t ts_close(handle_t handle)
{
    int32_t                 ret;
    GxDemuxProperty_Slot    slot = {0,};
    GxDemuxProperty_Filter  filter = {0, };
    struct update_ts*       ts = (struct update_ts*)handle;

    if (ts->slot_id >= 0) {
        slot.slot_id = ts->slot_id;
        ret = GxAVGetProperty(ts->dev, ts->demux,
                              GxDemuxPropertyID_SlotFree,
                              &slot, sizeof(GxDemuxProperty_Slot));
        ts->slot_id = -1;
    }

    if (ts->filter_id == -1) {
        filter.slot_id = ts->slot_id;
        ret = GxAVGetProperty(ts->dev, ts->demux,
                              GxDemuxPropertyID_FilterFree,
						      (void*)&filter, sizeof(GxDemuxProperty_Filter));
        ASSERT(ret == 0);

        ts->filter_id = -1;
    }

    GxAVCloseModule(ts->dev, ts->demux);

    GxAvdev_DestroyDevice(0);

    GxCore_Free(ts->name);
    ts->name = NULL;

    return E_OK;
}

GxUpdate_ProtocolOps gxupdate_protocol_ts = {
    GXUPDATE_PROTOCOL_TS,
    ts_open,
    ts_get_type,
    ts_set_size,
    ts_ioctl,
    ts_read,
    ts_write,
    ts_close
};

