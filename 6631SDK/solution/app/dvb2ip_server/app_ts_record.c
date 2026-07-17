#include "gxcore.h"
#include "common/gx_hw_malloc.h"
#include "common/memhole.h"
#include "app_config.h"
#if DVB2IP_SERVER_SUPPORT
#include "app_ts_record.h"
#include "app_module.h"
#include "module/app_frontend_board_config.h"

#define TS_REC_USER_MAX_COUNT 32
#define TS_REC_USER_MAX_LEN   (TS_REC_USER_MAX_COUNT*188)

#define TS_REC_DUMP_DVB_BLOCK_SIZE  (48128)    //(256 * 188)
#define TS_REC_DEMUX_MOD_MAX        (4)
#define TS_REC_DEMUX_SLOT_MAX       (64)
#define TS_REC_MAX_EXTPID_NUM       (32)
#define SW_BUFFER_SIZE              (3*188*1024)
#define HW_BUFFER_SIZE              (1*188*1024)
#define AV_MODULE_ID                (1)

#define PMT_FIXED_LENGTH            (16)
#define PMT_LOOP_FIXED_LENGTH       (5)

#define ISO639_DESCIPTOR_LENGTH     (6)
#define AC3_DESCIPTOR_LENGTH        (3)
#define EAC3_DESCIPTOR_LENGTH       (3)
#define DRA1_DESCIPTOR_LENGTH       (6)
#define SUBT_DESCIPTOR_LENGTH       (10)
#define TTX_DESCIPTOR_LENGTH        (7)

#define VALID_PID(pid)          ((pid>0)&&(pid<0x1fff))
#define CHECK_MODID(index)      if(index <0 || index>=TS_REC_DEMUX_MOD_MAX) return -1;

#define TS_REC_MEM_FREE(ptr) do { if (ptr) GxCore_Free(ptr); ptr = NULL; } while(0)

#define TS_REC_TICKTIME_MINUS_MS(end, start)\
    (((end.seconds*1000 + end.microsecs/1000)-(start.seconds*1000 + start.microsecs/1000)))

typedef enum
{
    FIFO_STATUS_STOPPED = 0,
    FIFO_STATUS_PAUSED,
    FIFO_STATUS_RUNNING,
    FIFO_STATUS_UNKNOWN,
}FifoStatus;

typedef struct
{
    int32_t size;
    int32_t total;
    int32_t use_cnt;
    int32_t *len;
    uint8_t *ptr;
    uint8_t *head;
    uint8_t **cur;
    uint8_t *full;
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    GxHwMallocObj hwobj;
#endif
    FifoStatus status;
}MultiFifo;

typedef enum
{
    TS_REC_STATUS_STOPPED = 0,
    TS_REC_STATUS_PAUSED,
    TS_REC_STATUS_RUNNING,
    TS_REC_STATUS_UNKNOWN,
}TsRecStatus;

typedef struct _TsRecUserInfo
{
    int32_t userdata_len;
    uint8_t *userdata;
}TsRecUserInfo;

typedef struct _ProgDmxInfo
{
    int32_t dvr_handle;
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    unsigned char *sw_buffer_ptr;
    unsigned char *hw_buffer_ptr;
#endif
    uint16_t prog_id;
    int32_t vslot_index;
    int32_t aslot_index;
    int32_t ext_slot_num;
    int32_t ext_slot_idx[TS_REC_MAX_EXTPID_NUM];
    MultiFifo *fifo;
    int32_t fifo_sel;
    TsRecUserInfo user_info;
    GxTime start_ticktime;
    uint32_t timems[2];
    uint32_t fill_packet_timems;
}ProgDmxInfo;

typedef struct _TsRecCtrl
{
    int32_t dev;
    int32_t dmx_handle;
    int32_t thread_id;
    int32_t fifo_size;
    int32_t max_prog;
    ProgDmxInfo *prog;
    handle_t mutex;
    bool inited;
    TsRecStatus status;
    GxDemuxProperty_Slot slot[TS_REC_DEMUX_SLOT_MAX];
}TsRecCtrl;

enum LogLevel
{
    LOG_DBG = 0,
    LOG_INFO,
    LOG_ERR,
};

static TsRecCtrl thiz_TsRecCtrl = {
    .dev = -1,
    .dmx_handle = -1,
    .thread_id = 0,
    .fifo_size = 0,
    .max_prog = 0,
    .mutex = 0,
    .inited = false,
    .prog = NULL,
    .status = TS_REC_STATUS_STOPPED,
};

static void _ts_rec_prog_release(ProgDmxInfo *prog);

static void _ts_rec_prog_unset(ProgDmxInfo *prog);

static int32_t thiz_debug_level = LOG_INFO;

static int32_t _ts_rec_log(int32_t debug_level, const char *function, int32_t lineno, char *format, ...);

#define TS_REC_DBG(format, args...)  do {_ts_rec_log(LOG_DBG, __func__, __LINE__, format, ##args);}  while(0)
#define TS_REC_INFO(format, args...) do {_ts_rec_log(LOG_INFO, __func__, __LINE__, format, ##args);} while(0)
#define TS_REC_ERR(format, args...)  do {_ts_rec_log(LOG_ERR, __func__, __LINE__, format, ##args);}  while(0)

static int32_t _ts_rec_log(int32_t debug_level, const char *function, int32_t lineno, char *format, ...)
{
    int i;
    char *sev = NULL;
    char buffer[1024] = {0};

    if(debug_level < thiz_debug_level)
        return 0;

    i = snprintf(buffer, 1024, "%d %s ", lineno, function);

    va_list va;
    va_start(va, format);
    vsnprintf(buffer + i, 1024 - i, format, va);
    va_end(va);

    switch (debug_level) {
        case LOG_DBG:
            sev = "\033[34m[tsrec][debug]";
            app_log_debug("%s %s\n\033[0m", sev, buffer);
            break;
        case LOG_INFO:
            sev = "\033[32m[tsrec][info]";
            app_log_info("%s %s\n\033[0m", sev, buffer);
            break;
        case LOG_ERR:
            sev = "\033[1;31m[tsrec][error]";
            app_log_error("%s %s\n\033[0m", sev, buffer);
            break;
        default:
            sev = sev? sev:"";
            app_log_debug("%s %s\n\033[0m", sev, buffer);
            break;
    }

    return 0;
}

#define TS_SPECIAL_FLAG 0
#define TS_REC_MAX_DATA_SIZE          (188 * 10)

#define MULTI_FIFO_MEM_FREE(ptr) do { if (ptr) free(ptr); ptr = NULL; } while(0)

static int ts_multi_fifo_destroy(MultiFifo *fifo)
{
    if(NULL == fifo)
    {
        TS_REC_ERR("params err");
        return -1;
    }
    MULTI_FIFO_MEM_FREE(fifo->full);
    MULTI_FIFO_MEM_FREE(fifo->cur);
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    if(fifo->ptr)
    {
        GxCore_HwFree(&fifo->hwobj);
        memset(&fifo->hwobj, 0, sizeof(GxHwMallocObj));
        fifo->ptr = NULL;
    }
#else
    MULTI_FIFO_MEM_FREE(fifo->ptr);
#endif
    MULTI_FIFO_MEM_FREE(fifo->len);
    MULTI_FIFO_MEM_FREE(fifo);
    return 0;
}

static int32_t ts_multi_fifo_free_sel_get(MultiFifo *fifo)
{
    int32_t i = 0, ret = -1;

    if(NULL == fifo)
    {
        TS_REC_ERR("params err");
        return -1;
    }

    for(i = 0; i < fifo->total; i++)
    {
        if(NULL == fifo->cur[i])
        {
            ret = i;
            break;
        }
    }
    return ret;
}

#if 0
static bool ts_multi_fifo_empty_check(MultiFifo *fifo, int32_t sel)
{
    bool ret = false;

    if(NULL == fifo)
    {
        TS_REC_ERR("params err");
        return true;
    }

    if(fifo->head == fifo->cur[sel] && 0 == fifo->full[sel])
        ret = true;
    return ret;
}

static FifoStatus ts_multi_fifo_status_get(MultiFifo *fifo)
{
    if(NULL == fifo)
    {
        TS_REC_ERR("params err");
        return FIFO_STATUS_STOPPED;
    }
    return fifo->status;
}
#endif

static int32_t ts_multi_fifo_use_cnt_get(MultiFifo *fifo)
{
    if(NULL == fifo)
    {
        TS_REC_ERR("params err");
        return -1;
    }

    return fifo->use_cnt;
}

static MultiFifo* ts_multi_fifo_init(int size, int total)
{
    MultiFifo *fifo = NULL;

    if ((fifo = calloc(1, sizeof(MultiFifo))) == NULL)
    {
        TS_REC_ERR("malloc(%d) failed!", sizeof(MultiFifo));
        return NULL;
    }

#if DVB2IP_SERVER_USE_STATIC_SCREEN
    fifo->hwobj.size = size;
    GxCore_HwMalloc(&fifo->hwobj, MALLOC_WITH_CACHE);
    if(fifo->hwobj.usr_p == NULL)
    {
        TS_REC_ERR("hw malloc(%d) failed!", size);
        goto err;
    }
    fifo->ptr = fifo->hwobj.usr_p;
#else
    if ((fifo->ptr = calloc(1, size)) == NULL)
    {
        TS_REC_ERR("malloc(%d) failed!", size);
        goto err;
    }
#endif

    if ((fifo->len = calloc(total, sizeof(int32_t*))) == NULL)
    {
        TS_REC_ERR("malloc(%d) failed!", total*sizeof(int32_t*));
        goto err;
    }
    if ((fifo->cur = calloc(total, sizeof(uint8_t*))) == NULL)
    {
        TS_REC_ERR("malloc(%d) failed!", total*sizeof(uint8_t*));
        goto err;
    }
    if ((fifo->full = calloc(total, sizeof(uint8_t))) == NULL)
    {
        TS_REC_ERR("malloc(%d) failed!", total*sizeof(uint8_t));
        goto err;
    }

    fifo->size = size;
    fifo->total = total;
    fifo->head = fifo->ptr;
    fifo->use_cnt = 0;
    fifo->status = FIFO_STATUS_RUNNING;
    return fifo;

err:
    MULTI_FIFO_MEM_FREE(fifo->full);
    MULTI_FIFO_MEM_FREE(fifo->cur);
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    if(fifo->ptr)
    {
        GxCore_HwFree(&fifo->hwobj);
        memset(&fifo->hwobj, 0, sizeof(GxHwMallocObj));
        fifo->ptr = NULL;
    }
#else
    MULTI_FIFO_MEM_FREE(fifo->ptr);
#endif
    MULTI_FIFO_MEM_FREE(fifo->len);
    MULTI_FIFO_MEM_FREE(fifo);
    return NULL;
}

#if 0
static int32_t ts_multi_fifo_status_set(MultiFifo *fifo, FifoStatus status)
{
    if(NULL == fifo)
    {
        TS_REC_ERR("params error!");
        return -1;
    }

    fifo->status = status;
    return 0;
}

static int ts_multi_fifo_reset(MultiFifo *fifo)
{
    int i = 0;

    if (NULL == fifo || NULL == fifo->ptr)
    {
        return -1;
    }

    fifo->head = fifo->ptr;
    for (i = 0; i < fifo->total; i++)
    {
        if (fifo->cur[i] != NULL)
        {
            fifo->cur[i] = fifo->head;
            fifo->full[i] = 0;
        }
    }

    return 0;
}
#endif

static int ts_multi_fifo_write(MultiFifo *fifo, uint8_t *buf, int size)
{
    int i = 0;
    int ret = 0;
    int tmp = 0;

    if (!fifo || !buf || size <= 0)
        return 0;

    if (!fifo->ptr)
    {
        goto end;
    }
    if (fifo->size < size)
    {
        TS_REC_ERR("Fifo size is small than size to write, %d < %d!", fifo->size, size);
        goto end;
    }

    for (i = 0; i < fifo->total; i++)
    {
        if (fifo->cur[i] != NULL)
        {
            if (fifo->head > fifo->cur[i])
                fifo->len[i] = fifo->size - (fifo->head - fifo->cur[i]);
            else if (fifo->head < fifo->cur[i])
                fifo->len[i] = fifo->cur[i] - fifo->head;
            else if (fifo->full[i] == 0)
                fifo->len[i] = fifo->size;
            else
                fifo->len[i] = 0;

            if (fifo->len[i] < size)
            {
                fifo->cur[i] = fifo->head;
                fifo->full[i] = 0;
                TS_REC_DBG("Client %d read too slow! %d < %d", i, fifo->len[i], size);
            }
        }
    }

    if (fifo->head + size <= fifo->ptr + fifo->size)
    {
        memcpy(fifo->head, buf, size);
        fifo->head += size;
    }
    else
    {
        tmp = fifo->size - (fifo->head - fifo->ptr);
        memcpy(fifo->head, buf, tmp);
        memcpy(fifo->ptr, buf + tmp, size - tmp);
        fifo->head = fifo->ptr + size - tmp;
    }

    for (i = 0; i < fifo->total; i++)
    {
        if (fifo->cur[i] != NULL)
        {
            if (fifo->cur[i] == fifo->head)
                fifo->full[i] = 1;
        }
    }

    ret = size;
end:
    return ret;
}

static int ts_multi_fifo_read(MultiFifo *fifo, uint8_t *buf, int size, int sel)
{
#define INTERVAL_CNT_NO_DATA (200)
    int ret = 0;
    int max = 0;
    int tmp = 0;
    static int32_t cnt = 0;

    if (!fifo || !buf || size <= 0 || sel < 0)
        return 0;

    if (!fifo->ptr)
    {
        goto end;
    }
    if (fifo->total <= sel)
    {
        TS_REC_ERR("sel is too large, %d <= %d!", fifo->total, sel);
        goto end;
    }

    if (fifo->head > fifo->cur[sel])
        max = fifo->head - fifo->cur[sel];
    else if (fifo->head < fifo->cur[sel])
        max = fifo->size - (fifo->cur[sel] - fifo->head);
    else if (fifo->full[sel] == 0)
        max = 0;
    else
        max = fifo->size;

    size = (max <= size) ? max : size;
    if (size == 0)
    {
        if(INTERVAL_CNT_NO_DATA == ++cnt)
        {
            cnt = 0;
            TS_REC_DBG("fifo is no data!\n");
        }
        goto end;
    }

    if (fifo->cur[sel] + size <= fifo->ptr + fifo->size)
    {
        memcpy(buf, fifo->cur[sel], size);
        fifo->cur[sel] += size;
    }
    else
    {
        tmp = fifo->size - (fifo->cur[sel] - fifo->ptr);
        memcpy(buf, fifo->cur[sel], tmp);
        memcpy(buf + tmp, fifo->ptr, size - tmp);
        fifo->cur[sel] = fifo->ptr + size - tmp;
    }
    fifo->full[sel] = 0;

    ret = size;
end:
    return ret;
}


static int ts_multi_fifo_sel_set(MultiFifo *fifo, int sel)
{
    if (!fifo || sel < 0)
        return -1;

    if (fifo->total <= sel && fifo->use_cnt < fifo->total)
    {
        TS_REC_ERR("sel is too large, %d <= %d!", fifo->total, sel);
        return -1;
    }
    fifo->cur[sel] = fifo->head;
    fifo->full[sel] = 0;
    fifo->len[sel] = 0;
    fifo->use_cnt++;

    return 0;
}

static int ts_multi_fifo_sel_unset(MultiFifo *fifo, int sel)
{
    if (!fifo || sel < 0)
        return -1;

    if (sel >= fifo->total && fifo->use_cnt > 0)
    {
        TS_REC_ERR("sel is too large, %d <= %d!", fifo->total, sel);
        return -1;
    }
    fifo->cur[sel] = NULL;
    fifo->full[sel] = 0;
    fifo->len[sel] = 0;
    fifo->use_cnt--;

    return 0;
}

static int32_t _ts_rec_demux_device_open(void)
{
    if(-1 == thiz_TsRecCtrl.dev)
        thiz_TsRecCtrl.dev = GxAVCreateDevice(0);

    if(thiz_TsRecCtrl.dev < 0)
        return -1;

    return 0;
}

static int32_t _ts_rec_demux_module_config(uint32_t tuner)
{
    GxDemuxProperty_ConfigDemux demux;
    int32_t ts_src;

#if DEMOD_AUTO_ADAPT_SUPPORT
    demod_adapt_info_ts_t info_ts_list[] = APP_FRONTEND_TS_DAPT_PARAMETERS;
    int32_t demod_index = FE_DEMOD_ADAPT_INVALID_INDEX;
    int32_t demod_addr_index = FE_DEMOD_ADAPT_INVALID_INDEX;
    extern int app_get_demod_config(uint8_t fe_id, int32_t *demod_index, int32_t *demod_addr_index);
    app_get_demod_config(tuner, &demod_index, &demod_addr_index);
    ts_src = info_ts_list[demod_index].ts_src;
#else
    enum dmx_input ts_src_list[APP_MULTI_DEMOD_NUM] = APP_FRONTEND_DEMUX_TS_SRC;
    ts_src = ts_src_list[tuner];
#endif

    if(thiz_TsRecCtrl.dmx_handle < 0)
        return -1;

    // demux config
    demux.source = ts_src;
    demux.ts_select = FRONTEND;
    demux.stream_mode = DEMUX_PARALLEL;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;

    return GxAVSetProperty(thiz_TsRecCtrl.dev, thiz_TsRecCtrl.dmx_handle, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));
}

static int32_t _ts_rec_demux_module_open(void)
{
    if(-1 == thiz_TsRecCtrl.dmx_handle)
        thiz_TsRecCtrl.dmx_handle = GxAVOpenModule(thiz_TsRecCtrl.dev, GXAV_MOD_DEMUX, AV_MODULE_ID);

    if(thiz_TsRecCtrl.dmx_handle < 0)
        return -1;

    return 0;
}

static void _ts_rec_demux_device_close(void)
{
    if(thiz_TsRecCtrl.dev != -1)
        GxAVDestroyDevice(thiz_TsRecCtrl.dev);

    thiz_TsRecCtrl.dev = -1;
    return;
}

static void _ts_rec_demux_module_close(void)
{
    if(thiz_TsRecCtrl.dmx_handle != -1)
        GxAVCloseModule(thiz_TsRecCtrl.dev, thiz_TsRecCtrl.dmx_handle);

    thiz_TsRecCtrl.dmx_handle = -1;
    return;
}

static int32_t _ts_rec_slot_find(uint16_t pid)
{
    int32_t i = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    for(i = 0; i < TS_REC_DEMUX_SLOT_MAX; i++)
    {
        if(ctrl->slot[i].pid == pid)
            return i;
    }

    return -1;
}

static int32_t _ts_rec_free_slot_get(void)
{
    int32_t i = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    for(i = 0; i < TS_REC_DEMUX_SLOT_MAX; i++)
    {
        if(0 == ctrl->slot[i].pid)
            return i;
    }

    return -1;
}

static int32_t _ts_rec_slot_add(GxDemuxProperty_Slot slot)
{
    int32_t slot_index = -1;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    if(-1 == (slot_index = _ts_rec_free_slot_get()))
        return -1;

    ctrl->slot[slot_index] = slot;

    return slot_index;
}

static int32_t _ts_rec_slot_update(int32_t slot_index, GxDemuxProperty_Slot slot)
{
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    if(slot_index < 0 || slot_index >= TS_REC_DEMUX_SLOT_MAX)
        return -1;

    ctrl->slot[slot_index] = slot;
    return 0;
}

static int32_t _ts_rec_heart_beats(ProgDmxInfo *prog)
{
    GxTime cur_time = {0};

    if(0 == prog->start_ticktime.seconds)
        GxCore_GetTickTime(&prog->start_ticktime);

    GxCore_GetTickTime(&cur_time);
    prog->timems[0] = prog->timems[1];
    prog->timems[1] = TS_REC_TICKTIME_MINUS_MS(cur_time, prog->start_ticktime);
    return 0;
}

static bool _ts_rec_private_packet_fill(ProgDmxInfo *prog)
{
    if(0 == prog->fill_packet_timems || (prog->timems[1] - prog->fill_packet_timems) >= 500)
    {
        prog->fill_packet_timems = prog->timems[1];
        return true;
    }
    return false;
}

static void ts_rec_dumpfilter_thread(void *usrdata)
{
    uint8_t *buffer = NULL;
    int32_t i = 0;
    int32_t read_len = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = NULL;
    bool need_delay = false;

    GxCore_ThreadDetach();

    buffer = GxCore_Calloc(TS_REC_DUMP_DVB_BLOCK_SIZE, sizeof(uint8_t));
    if(NULL == buffer)
        return;

    while(TS_REC_STATUS_RUNNING == ctrl->status)
    {
        need_delay = true;

        for(i = 0; i < ctrl->max_prog; i++)
        {
            GxCore_MutexLock(ctrl->mutex);
            prog = &ctrl->prog[i];
            if(prog->dvr_handle > 0)
            {
                _ts_rec_heart_beats(prog);

                if(true == _ts_rec_private_packet_fill(prog) && prog->user_info.userdata_len)
                {
                    ts_multi_fifo_write(prog->fifo, prog->user_info.userdata, prog->user_info.userdata_len);
                }
                else
                {
                    if((read_len = GxAVModuleRead(ctrl->dev, prog->dvr_handle, buffer, TS_REC_DUMP_DVB_BLOCK_SIZE, 20000000)) > 0)
                    {
                        ts_multi_fifo_write(prog->fifo, buffer, read_len);
                        if(read_len > 188)
                            need_delay = false;
                    }
                }
            }
            GxCore_MutexUnlock(ctrl->mutex);
        }

        if(true == need_delay)
            GxCore_ThreadDelay(10);
    }

    GxCore_Free(buffer);
    return;
}

int32_t app_ts_record_init(int32_t fifo_size, int32_t max_prog)
{
    int32_t i = 0, j = 0, ret = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    if(0 == fifo_size || 0 == max_prog)
    {
        TS_REC_ERR("params error");
        return -1;
    }

    if(0 == ctrl->mutex)
    {
        if(GXCORE_SUCCESS != GxCore_MutexCreate(&ctrl->mutex))
        {
            TS_REC_ERR("mutex create failed");
            return -1;
        }
    }

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->inited)
        goto end;

    if(-1 == _ts_rec_demux_device_open())
    {
        TS_REC_ERR("demux device open failed");
        goto end;
    }

    if(-1 == _ts_rec_demux_module_open())
    {
        TS_REC_ERR("open demux module failed");
        _ts_rec_demux_device_close();
        goto end;
    }

    ctrl->fifo_size = fifo_size;
    ctrl->max_prog = max_prog;
    TS_REC_MEM_FREE(ctrl->prog);

    ctrl->prog = GxCore_Calloc(ctrl->max_prog, sizeof(ProgDmxInfo));
    if(NULL == ctrl->prog)
    {
        TS_REC_ERR("No enough memory");
        return -1;
    }

    for(i = 0; i < ctrl->max_prog; i++)
    {
        ctrl->prog[i].vslot_index = -1;
        ctrl->prog[i].aslot_index = -1;
        ctrl->prog[i].fifo_sel = -1;
        ctrl->prog[i].dvr_handle = -1;
        for(j = 0; j < TS_REC_MAX_EXTPID_NUM; j++)
        {
            ctrl->prog[i].ext_slot_idx[j] = -1;
        }
    }

    if(0 == ctrl->thread_id)
    {
        ctrl->status = TS_REC_STATUS_RUNNING;
        GxCore_ThreadCreate("ts_rec_dumpfilter_thread", &ctrl->thread_id, ts_rec_dumpfilter_thread, NULL, 16*1024, GXOS_DEFAULT_PRIORITY - 1);
    }

    ctrl->inited = true;
end:
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

static int32_t _ts_rec_dvr_run(int32_t dev, int32_t dvr_handle)
{
    return GxAVSetProperty(dev, dvr_handle, GxDvrPropertyID_Run, NULL, 0);
}

static int32_t _ts_rec_dvr_stop(int32_t dev, int32_t dvr_handle)
{
    return GxAVSetProperty(dev, dvr_handle, GxDvrPropertyID_Stop, NULL, 0);
}

static int32_t _ts_rec_dvr_config(int32_t index)
{
    GxDvrProperty_Config dvrconf;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = &ctrl->prog[index];

    if(-1 == prog->dvr_handle)
        prog->dvr_handle = GxAVOpenModule(ctrl->dev, GXAV_MOD_DVR, AV_MODULE_ID);
    if(prog->dvr_handle < 0)
    {
        TS_REC_ERR("open dvr failed!\n");
        return -1;
    }

    memset(&dvrconf, 0, sizeof(GxDvrProperty_Config));
    dvrconf.src = DVR_INPUT_DMX;
    dvrconf.dst = DVR_OUTPUT_MEM;
    dvrconf.dst_buf.sw_buffer_size = SW_BUFFER_SIZE;
    dvrconf.dst_buf.hw_buffer_size = HW_BUFFER_SIZE;
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    prog->sw_buffer_ptr = GxCore_MemholeMalloc(SW_BUFFER_SIZE, NULL);
    prog->hw_buffer_ptr = GxCore_MemholeMalloc(HW_BUFFER_SIZE, NULL);
    if(!prog->sw_buffer_ptr || !prog->hw_buffer_ptr)
    {
        TS_REC_ERR("HW malloc failed!\n");
        goto err;
    }
    dvrconf.dst_buf.sw_buffer_ptr = (uint32_t)prog->sw_buffer_ptr;
    dvrconf.dst_buf.hw_buffer_ptr = (uint32_t)prog->hw_buffer_ptr;
#endif

    if(GxAVSetProperty(ctrl->dev, prog->dvr_handle, GxDvrPropertyID_Config, &dvrconf, sizeof(dvrconf)) < 0)
    {
        TS_REC_ERR("dvr config failed!\n");
        goto err;
    }

    return 0;
err:

#if DVB2IP_SERVER_USE_STATIC_SCREEN
    if(prog->sw_buffer_ptr)
    {
        GxCore_MemholeFree(prog->sw_buffer_ptr);
        prog->sw_buffer_ptr = NULL;
    }
    if(prog->hw_buffer_ptr)
    {
        GxCore_MemholeFree(prog->hw_buffer_ptr);
        prog->hw_buffer_ptr = NULL;
    }
#endif
    if(prog->dvr_handle > 0)
    {
        GxAVCloseModule(ctrl->dev, prog->dvr_handle);
        prog->dvr_handle = -1;
    }
    return -1;
}

static int32_t _ts_rec_demux_slot_alloc(int32_t index, uint16_t pid, int32_t slot_flags, int32_t slot_type)
{
    int32_t slot_index = -1;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = &ctrl->prog[index];
    GxDemuxProperty_Slot slot;

    if(!VALID_PID(pid))
        return -1;

    memset(&slot, 0, sizeof(GxDemuxProperty_Slot));

    slot_index = _ts_rec_slot_find(pid);
    if(-1 == slot_index)
    {
        slot.pid = pid;
        slot.flags = slot_flags;
        slot.type = slot_type;
        if(GxAVGetProperty(ctrl->dev, ctrl->dmx_handle, GxDemuxPropertyID_SlotAlloc, &slot, sizeof(GxDemuxProperty_Slot)) < 0)
            return -1;
        slot_index = _ts_rec_slot_add(slot);
    }
    else
    {
        slot = ctrl->slot[slot_index];
        slot.flags |= slot_flags;
        _ts_rec_slot_update(slot_index, slot);
    }

    switch(slot_type)
    {
        case DEMUX_SLOT_VIDEO:
            ctrl->prog[index].vslot_index = slot_index;
            break;
        case DEMUX_SLOT_AUDIO:
            ctrl->prog[index].aslot_index = slot_index;
            break;
        case DEMUX_SLOT_PES_AUDIO:
        case DEMUX_SLOT_PES:
            break;
        case DEMUX_SLOT_PSI:
            prog->ext_slot_idx[prog->ext_slot_num] = slot_index;
            prog->ext_slot_num++;
            break;
        default:
            break;
    }

    if(slot_flags & DMX_TSOUT_EN)
    {
        slot.ts_out_pin = ctrl->prog[index].dvr_handle;
        _ts_rec_slot_update(slot_index, slot);
    }

    if(GxAVSetProperty(ctrl->dev, ctrl->dmx_handle, GxDemuxPropertyID_SlotConfig, &slot, sizeof(GxDemuxProperty_Slot)) < 0)
        return -1;

    return GxAVSetProperty(ctrl->dev, ctrl->dmx_handle, GxDemuxPropertyID_SlotEnable, &slot, sizeof(GxDemuxProperty_Slot));
}

static int32_t _ts_rec_demux_slot_free(GxDemuxProperty_Slot *slot)
{
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    GxAVSetProperty(ctrl->dev, ctrl->dmx_handle, GxDemuxPropertyID_SlotDisable, slot, sizeof(GxDemuxProperty_Slot));

    return GxAVSetProperty(ctrl->dev, ctrl->dmx_handle, GxDemuxPropertyID_SlotFree, slot, sizeof(GxDemuxProperty_Slot));
}

static int32_t _ts_rec_demux_ext_slot_alloc(int32_t index, TsRecConfig *config)
{
    int32_t slot_flags = 0;
    GxBusPmDataProg prog = {0};

    if(GXCORE_ERROR == GxBus_PmProgGetById(config->prog_id, &prog))
        return -1;

    if(VALID_PID(prog.pcr_pid) && prog.pcr_pid != prog.cur_audio_pid && prog.pcr_pid != prog.video_pid)
    {
        slot_flags = DMX_REPEAT_MODE|DMX_TSOUT_EN;
        if(_ts_rec_demux_slot_alloc(index, prog.pcr_pid, slot_flags, DEMUX_SLOT_PSI) < 0)
        {
            TS_REC_ERR("demux slot alloc pid: %d failed", prog.pcr_pid);
            return -1;
        }
    }

    if(false == config->user_pmt && VALID_PID(prog.pmt_pid))
    {
        slot_flags = DMX_REPEAT_MODE|DMX_TSOUT_EN;
        if(_ts_rec_demux_slot_alloc(index, prog.pmt_pid, slot_flags, DEMUX_SLOT_PSI) < 0)
        {
            TS_REC_ERR("demux slot alloc pid: %d failed", prog.pmt_pid);
            return -1;
        }
    }

    return 0;
}

static int32_t _ts_rec_prog_find(uint16_t prog_id)
{
    int32_t i = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    if(0 == prog_id)
        return -1;

    for(i = 0; i < ctrl->max_prog; i++)
    {
        if(prog_id == ctrl->prog[i].prog_id)
            return i;
    }

    return -1;
}

static int32_t _ts_rec_free_prog_get(void)
{
    int32_t i = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    for(i = 0; i < ctrl->max_prog; i++)
    {
        if(0 == ctrl->prog[i].prog_id)
            return i;
    }

    return -1;
}

static int32_t _ts_rec_multi_fifo_config(int32_t index)
{
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = &ctrl->prog[index];

    prog->fifo = ts_multi_fifo_init(ctrl->fifo_size, ctrl->max_prog);
    if(NULL == prog->fifo)
    {
        TS_REC_ERR("multi fifo init failed");
        return -1;
    }

    prog->fifo_sel = 0;
    return ts_multi_fifo_sel_set(prog->fifo, 0);
}

static int32_t _ts_rec_multi_fifo_config_mux(int32_t prog_index, int32_t new_prog_index)
{
    ProgDmxInfo *prog = &thiz_TsRecCtrl.prog[prog_index];
    ProgDmxInfo *new_prog = &thiz_TsRecCtrl.prog[new_prog_index];

    if(NULL == prog->fifo)
        return -1;

    new_prog->fifo = prog->fifo;

    if(-1 == (new_prog->fifo_sel = ts_multi_fifo_free_sel_get(new_prog->fifo)))
    {
        TS_REC_ERR("the usage of fifo is full");
        return -1;
    }

    return ts_multi_fifo_sel_set(new_prog->fifo, new_prog->fifo_sel);
}

static char *_ts_rec_pat_packet_generate(uint16_t prog_id)
{
#define PAT_FIXED_LENGTH    (12)
#define PAT_LOOP_LENGTH    (4)
    int32_t i = 0;
    char *pat_data = NULL;
    unsigned short sec_length = 0;
    unsigned int crc_value = 0;
    GxBusPmDataProg prog = {0};

    if(0 == prog_id || GXCORE_ERROR == GxBus_PmProgGetById(prog_id, &prog))
        return NULL;

    pat_data = (char *)GxCore_Calloc(PAT_FIXED_LENGTH + PAT_LOOP_LENGTH, 1);
    if(NULL == pat_data)
    {
        TS_REC_ERR("NO enough memory");
        return NULL;
    }

    //table_id
    pat_data[i++] = 0x00;

    //synctax
    pat_data[i] = 0x80;
    //section length
    sec_length = PAT_FIXED_LENGTH + PAT_LOOP_LENGTH - 3;
    pat_data[i++] |= ((sec_length >> 8) & 0x0f);
    pat_data[i++] = sec_length & 0xff;

    //ts_id
    pat_data[i++] = 0;
    pat_data[i++] = 1;

    //version num / current_next_indicator
    pat_data[i++] = 1;

    //sec_num
    pat_data[i++] = 0;

    //last_sec_num
    pat_data[i++] = 0;

    //prog_num
    pat_data[i++] = (prog.service_id >> 8) & 0xff;
    pat_data[i++] = prog.service_id & 0xff;

    //pmt_pid
    pat_data[i++] = (prog.pmt_pid >> 8) & 0x1f;
    pat_data[i++] = prog.pmt_pid & 0xff;

    crc_value = dvb_crc32((unsigned char*)pat_data, i);

    //crc
    pat_data[i++] = (crc_value >> 24 & 0xff);
    pat_data[i++] = (crc_value >> 16 & 0xff);
    pat_data[i++] = (crc_value >> 8 & 0xff);
    pat_data[i++] = (crc_value & 0xff);

#if 0
    sec_length = i;
    printf("\n######### PAT Length = %d #########\n", sec_length);
    for(i = 0; i < sec_length; i++)
    {
        printf("%02x ", pat_data[i]);
        if(i % 16 == 15)
            printf("\n");
    }
    printf("\n###################################\n");
#endif
    return pat_data;

}

static int32_t _ts_rec_pat_ts_packet(uint16_t prog_id, AppTsData *ts_data)
{
    char *pat_data = NULL;
    int32_t data_len = 0;
    int32_t ret = 0;

    if(NULL == ts_data)
        return -1;

    if((pat_data = _ts_rec_pat_packet_generate(prog_id)) != NULL)
    {
        data_len = ((pat_data[1] & 0x0f) << 8) | pat_data[2];
        data_len += 3;
        ret = app_ts_pack(0, pat_data, data_len, ts_data);
        TS_REC_MEM_FREE(pat_data);
    }
    return ret;
}

static unsigned short _ts_rec_pmt_stream_video_generate(char *buff, unsigned short video_pid, GxBusPmDataProgVideoType video_type)
{
    unsigned short i = 0;
    unsigned char stream_type = 0;

    if(buff == NULL)
        return 0;

    if(video_type == GXBUS_PM_PROG_H264)
        stream_type = 0x1b;
    else if(video_type == GXBUS_PM_PROG_MPEG)
        stream_type = 0x01;
    else if(video_type == GXBUS_PM_PROG_H265)
        stream_type = 0x24;
    else if(video_type == GXBUS_PM_PROG_AVS)
        stream_type = 0x42;
    else
        stream_type = 0x02;

    buff[i++] = stream_type;
    buff[i++] = ((video_pid >> 8) & 0x1f);
    buff[i++] = video_pid & 0xff;
    buff[i++] = 0;
    buff[i++] = 0;

    return i;
}

static bool _ts_rec_check_audio_iso639(char *lang)
{
    if((lang == NULL) || (strlen(lang) == 0) || (strlen(lang) > 3))
        return false;

    return true;
}

static unsigned short _ts_rec_pmt_stream_audio_generate(char *buff, unsigned short audio_pid, GxBusPmDataProgAudioType audio_type, char *iso_str)
{
    unsigned short i = 0;
    unsigned char stream_type = 0;
    unsigned short es_info_length = 0;
    bool iso_flag = false;
    bool ac3_flag = false;
    bool eac3_flag = false;

    if(buff == NULL)
        return 0;

    if(audio_type == GXBUS_PM_AUDIO_MPEG1)
    {
        stream_type = 0x03;
    }
    else if(audio_type == GXBUS_PM_AUDIO_MPEG2)
    {
        stream_type = 0x04;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AAC_ADTS)
    {
        stream_type = 0x0f;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AAC_LATM)
    {
        stream_type = 0x11;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AC3)
    {
        stream_type = 0x06;
        es_info_length += AC3_DESCIPTOR_LENGTH;
        ac3_flag = true;
    }
    else if(audio_type == GXBUS_PM_AUDIO_EAC3)
    {
        stream_type = 0x06;
        es_info_length += EAC3_DESCIPTOR_LENGTH;
        eac3_flag = true;
    }
    else if(audio_type == GXBUS_PM_AUDIO_DRA)
    {
        stream_type = 0x06;
        es_info_length += DRA1_DESCIPTOR_LENGTH; //DRA DESCIPTOR LENGTH
    }
    else
    {
        stream_type = 0x04;
    }

    if(_ts_rec_check_audio_iso639(iso_str) == true)
    {
        es_info_length += ISO639_DESCIPTOR_LENGTH;
        iso_flag = true;
    }

    buff[i++] = stream_type;
    buff[i++] = ((audio_pid >> 8) & 0x1f);
    buff[i++] = audio_pid & 0xff;

    //es info lenth
    buff[i++] = ((es_info_length >> 8) & 0x0f);
    buff[i++] = (es_info_length & 0xff);

    if(iso_flag == true)
    {
        buff[i++] = 0x0a;
        buff[i++] = 4;
        buff[i++] = iso_str[0];
        buff[i++] = iso_str[1];
        buff[i++] = iso_str[2];
        buff[i++] = 1;
    }

    if(ac3_flag == true)
    {
        buff[i++] = 0x6a;
        buff[i++] = 1;
        buff[i++] = 0;
    }

    if(eac3_flag == true)
    {
        buff[i++] = 0x7a;
        buff[i++] = 1;
        buff[i++] = 0;
    }

    if(audio_type == GXBUS_PM_AUDIO_DRA)
    {
        buff[i++] = 0x05;
        buff[i++] = 4;
        buff[i++] = 'D';
        buff[i++] = 'R';
        buff[i++] = 'A';
        buff[i++] = '1';
    }

    return i;
}

static char *_ts_rec_pmt_packet_generate(uint16_t prog_id)
{
    char *pmt_data = NULL;
    char *temp_data = NULL;
    unsigned short i = 0;
    unsigned short length = 0;
    unsigned short temp_len = 0;
    unsigned int crc_value = 0;
    GxBusPmDataProg node = {0};

    if(GXCORE_ERROR == GxBus_PmProgGetById(prog_id, &node))
        return NULL;

    pmt_data = (char*)GxCore_Calloc(PMT_FIXED_LENGTH, 1);
    if(NULL == pmt_data)
    {
        TS_REC_ERR("No enough memory");
        return NULL;
    }

    length = PMT_FIXED_LENGTH;
    //table_id
    pmt_data[i++] = 0x02;
    //synctax / length
    pmt_data[i++] = 0x80;
    pmt_data[i++] = 0x00;
    //prog_num
    pmt_data[i++] = ((node.service_id >> 8) & 0xff);
    pmt_data[i++] = (node.service_id & 0xff);
    //version num / current_next_indicator
    pmt_data[i++] = 1;
    //sec_num
    pmt_data[i++] = 0x00;
    //last_sec_num
    pmt_data[i++] = 0x00;
    //pcr
    pmt_data[i++] = (node.pcr_pid >> 8) & 0x1f;//0x1f;
    pmt_data[i++] = node.pcr_pid & 0xff;//0xff;
    //prog_info_length
    pmt_data[i++] = 0;
    pmt_data[i++] = 0;

    if(node.service_type == GXBUS_PM_PROG_TV)
    {
        // video
        temp_data = (char*)GxCore_Realloc(pmt_data, length + PMT_LOOP_FIXED_LENGTH);
        if(temp_data == NULL)
            goto error_ret;

        pmt_data = temp_data;
        temp_data = NULL;
        temp_len = _ts_rec_pmt_stream_video_generate(pmt_data + i, node.video_pid, node.video_type);
        i += temp_len;
        length += temp_len;
    }

    ///current audio
    temp_len = PMT_LOOP_FIXED_LENGTH;

    if(node.cur_audio_type == GXBUS_PM_AUDIO_AC3) //ac3 descriptor
        temp_len += AC3_DESCIPTOR_LENGTH;
    if(node.cur_audio_type == GXBUS_PM_AUDIO_EAC3) //eac3 descriptor
        temp_len += EAC3_DESCIPTOR_LENGTH;
    if(node.cur_audio_type == GXBUS_PM_AUDIO_DRA) //dra descriptor
        temp_len += DRA1_DESCIPTOR_LENGTH;

    temp_data = (char*)GxCore_Realloc(pmt_data, length + temp_len);
    if(temp_data == NULL)
        goto error_ret;

    pmt_data = temp_data;
    temp_data = NULL;
    temp_len = _ts_rec_pmt_stream_audio_generate(pmt_data + i, node.cur_audio_pid, node.cur_audio_type, NULL);
    i += temp_len;
    length += temp_len;

    length -= 3;
    pmt_data[1] |= ((length >> 8) & 0x0f);
    pmt_data[2] = (length & 0xff);

    crc_value = dvb_crc32((unsigned char*)pmt_data, i);
    //crc
    pmt_data[i++] = (crc_value >> 24 & 0xff);
    pmt_data[i++] = (crc_value >> 16 & 0xff);
    pmt_data[i++] = (crc_value >> 8 & 0xff);
    pmt_data[i++] = (crc_value & 0xff);

#if 0
    length = i;
    printf("\n######### PMT Length = %d #########\n", length);
    for(i = 0; i < length; i++)
    {
        printf("%02x ", pmt_data[i]);
        if(i % 16 == 15)
            printf("\n");
    }
    printf("\n###################################\n");
#endif
    return pmt_data;

error_ret:
    TS_REC_MEM_FREE(pmt_data);
    return NULL;
}

static int32_t _ts_rec_pmt_ts_packet(uint16_t prog_id, AppTsData *ts_data)
{
    char *pmt_data = NULL;
    int32_t ret = 0, data_len = 0;
    GxBusPmDataProg prog = {0};

    if(NULL == ts_data)
        return -1;

    if((pmt_data = _ts_rec_pmt_packet_generate(prog_id)) != NULL)
    {
        if(GXCORE_SUCCESS == GxBus_PmProgGetById(prog_id, &prog))
        {
            data_len = ((pmt_data[1] & 0x0f) << 8) | pmt_data[2];
            data_len += 3;
            ret = app_ts_pack(prog.pmt_pid, pmt_data, data_len, ts_data);
        }

        TS_REC_MEM_FREE(pmt_data);
    }
    return ret;
}

static int32_t _ts_rec_user_info_generate(int32_t index, TsRecConfig *config)
{
    int32_t i = 0;
    AppTsData ts_data = {0};
    ProgDmxInfo *prog = &thiz_TsRecCtrl.prog[index];

    if(NULL == prog->user_info.userdata)
    {
        prog->user_info.userdata = GxCore_Calloc(TS_REC_USER_MAX_LEN, sizeof(uint8_t));
        if(NULL == prog->user_info.userdata)
        {
            TS_REC_ERR("No enough memory");
            return -1;
        }
    }

    _ts_rec_pat_ts_packet(config->prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(prog->user_info.userdata + prog->user_info.userdata_len, ts_data.pack_data[i], 188);
        prog->user_info.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);

    _ts_rec_pmt_ts_packet(config->prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(prog->user_info.userdata + prog->user_info.userdata_len, ts_data.pack_data[i], 188);
        prog->user_info.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);
    return 0;
}

static int32_t _ts_rec_prog_config(int32_t index, TsRecConfig *config)
{
    int32_t vslot_flags = 0, aslot_flags = 0;
    GxBusPmDataProg node_prog = {0};
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = &ctrl->prog[index];

    if(GXCORE_ERROR == GxBus_PmProgGetById(config->prog_id, &node_prog))
        return -1;

    if(_ts_rec_demux_module_config(node_prog.tuner) < 0)
        return -1;

    if(_ts_rec_dvr_config(index) < 0)
        return -1;

    if(_ts_rec_dvr_run(ctrl->dev, prog->dvr_handle) < 0)
        return -1;

    aslot_flags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_TSOUT_EN|DMX_DES_EN|DMX_ERR_DISCARD_EN);
    vslot_flags = (DMX_REPEAT_MODE|DMX_PTS_TO_SDRAM|DMX_TSOUT_EN|DMX_DES_EN);

    if(VALID_PID(node_prog.video_pid) && _ts_rec_demux_slot_alloc(index, node_prog.video_pid, vslot_flags, DEMUX_SLOT_VIDEO) < 0)
    {
        TS_REC_ERR("demux slot pid = %d alloc failed", node_prog.video_pid);
        return -1;
    }

    if(VALID_PID(node_prog.cur_audio_pid) && _ts_rec_demux_slot_alloc(index, node_prog.cur_audio_pid, aslot_flags, DEMUX_SLOT_AUDIO) < 0)
    {
        TS_REC_ERR("demux slot pid = %d alloc failed", node_prog.cur_audio_pid);
        return -1;
    }

    if(true == config->user_pmt && _ts_rec_user_info_generate(index, config) < 0)
    {
        TS_REC_ERR("user info generate failed");
        return -1;
    }

    if(_ts_rec_demux_ext_slot_alloc(index, config) < 0)
    {
        TS_REC_ERR("demux ext alloc failed");
        return -1;
    }

#if CA_SUPPORT
    TS_REC_INFO("%s[%d]ProgID: %d, SatID: %d, TPID: %d\n", __func__, __LINE__, config->prog_id, node_prog.sat_id, node_prog.tp_id);
    app_extend_send_service(config->prog_id, node_prog.sat_id, node_prog.tp_id, AV_MODULE_ID, CONTROL_PVR);
#endif
    return _ts_rec_multi_fifo_config(index);
}

handle_t app_ts_record_start(TsRecConfig *config)
{
    handle_t handle = 0;
    int32_t prog_index = -1;
    int32_t new_prog_index = -1;
    ProgDmxInfo *prog = NULL, *new_prog = NULL;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    if(NULL == config || 0 == config->prog_id)
        return -1;

    GxCore_MutexLock(ctrl->mutex);

    if(-1 == (prog_index = _ts_rec_prog_find(config->prog_id)))
    {
        if(-1 == (new_prog_index = _ts_rec_free_prog_get()))
        {
            TS_REC_INFO("the num of prog record exceed the maxinum: %d", ctrl->max_prog);
            goto end;
        }

        new_prog = &ctrl->prog[new_prog_index];

        if(_ts_rec_prog_config(new_prog_index, config) < 0)
        {
            TS_REC_ERR("prog record config failed");
            _ts_rec_prog_release(new_prog);
            goto end;
        }
        new_prog->prog_id = config->prog_id;
        handle = (handle_t)new_prog;
        TS_REC_INFO("not find, the new num of prog is %d", new_prog_index);
    }
    else
    {
        TS_REC_INFO("The original program index of the same prog id is %d", prog_index);
        prog = &ctrl->prog[prog_index];
        if(-1 == (new_prog_index = _ts_rec_free_prog_get()))
        {
            TS_REC_INFO("the num of prog record exceed the maxinum: %d", ctrl->max_prog);
            goto end;
        }

        new_prog = &ctrl->prog[new_prog_index];
        *new_prog = *prog;
        new_prog->start_ticktime.seconds = 0;
        new_prog->start_ticktime.microsecs = 0;
        if(_ts_rec_multi_fifo_config_mux(prog_index, new_prog_index) < 0)
        {
            TS_REC_ERR("mulit fifo config mux failed");
            _ts_rec_prog_unset(new_prog);
            goto end;
        }
        TS_REC_INFO("the new num of prog is %d", new_prog_index);
        handle = (handle_t)new_prog;
    }

end:
    GxCore_MutexUnlock(ctrl->mutex);
    return handle;
}

int32_t app_ts_record_read(handle_t handle, uint8_t *buffer, int32_t size)
{
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    ProgDmxInfo *prog = (ProgDmxInfo *)handle;
    int32_t ret = 0;

    if(NULL == buffer || 0 == size)
        return ret;

    GxCore_MutexLock(ctrl->mutex);
    ret = ts_multi_fifo_read(prog->fifo, buffer, size, prog->fifo_sel);
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

static void _ts_rec_prog_release(ProgDmxInfo *prog)
{
#define AV_SLOT_NUM 2
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;
    int32_t i = 0, ext_slot_num = 0;
    int32_t *ext_slot_idx = NULL;
    int32_t slot_index[AV_SLOT_NUM] = {-1, -1};

    if(NULL == prog)
        return;

    if(prog->fifo)
        ts_multi_fifo_destroy(prog->fifo);

    slot_index[0] = prog->vslot_index;
    slot_index[1] = prog->aslot_index;

    for(i = 0; i < AV_SLOT_NUM; i++)
    {
        if(slot_index[i] >= 0 && slot_index[i] < TS_REC_DEMUX_SLOT_MAX)
        {
            _ts_rec_demux_slot_free(&ctrl->slot[slot_index[i]]);
            memset(&ctrl->slot[slot_index[i]], 0, sizeof(GxDemuxProperty_Slot));
        }
    }
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    if(prog->sw_buffer_ptr)
    {
        GxCore_MemholeFree(prog->sw_buffer_ptr);
        prog->sw_buffer_ptr = NULL;
    }
    if(prog->hw_buffer_ptr)
    {
        GxCore_MemholeFree(prog->hw_buffer_ptr);
        prog->hw_buffer_ptr = NULL;
    }
#endif
    if(prog->dvr_handle > 0)
    {
        _ts_rec_dvr_stop(ctrl->dev, prog->dvr_handle);
        GxAVCloseModule(ctrl->dev, prog->dvr_handle);
#if CA_SUPPORT
        app_descrambler_close_by_control_type(CONTROL_PVR);
#endif
    }

    ext_slot_num = prog->ext_slot_num;
    ext_slot_idx = prog->ext_slot_idx;

    int j = 0 ;
    int k = 0 ;
    int used = 0 ;

    for(i = 0; i < ext_slot_num; i++)
    {
        for(j = 0; j < ctrl->max_prog; j++)
        {
            if ( (&ctrl->prog[j]) == prog )
            {
                continue ;
            }
            for(k = 0; k < ctrl->prog[j].ext_slot_num; k++)
            {
                if ( *(ext_slot_idx + i) == ctrl->prog[j].ext_slot_idx[k] )
                {
                    used = 1 ;
                    break;
                }
            }
            if (used)
                break;
        }

        if ( 0 == used )
        {
            _ts_rec_demux_slot_free(&ctrl->slot[*(ext_slot_idx + i)]);
            memset(&ctrl->slot[*(ext_slot_idx + i)], 0, sizeof(GxDemuxProperty_Slot));
        }

        ext_slot_idx[i] = -1;
    }

    TS_REC_MEM_FREE(prog->user_info.userdata);
    prog->user_info.userdata_len = 0;
    prog->fifo = NULL;
    prog->fifo_sel = -1;
    prog->user_info.userdata = NULL;
    prog->user_info.userdata_len = 0;
    prog->dvr_handle = -1;
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    prog->sw_buffer_ptr = NULL;
    prog->hw_buffer_ptr = NULL;
#endif
    prog->vslot_index = -1;
    prog->aslot_index = -1;
    prog->prog_id = 0;
    prog->ext_slot_num = 0;
    prog->start_ticktime.seconds = 0;
    prog->start_ticktime.microsecs = 0;
    prog->fill_packet_timems = 0;
    prog->timems[0] = prog->timems[1] = 0;
    return;
}

static void _ts_rec_prog_unset(ProgDmxInfo *prog)
{
    int32_t i = 0, ext_slot_num = 0;
    int32_t *ext_slot_idx = NULL;

    if(NULL == prog)
        return;

    if(prog->fifo)
        ts_multi_fifo_sel_unset(prog->fifo, prog->fifo_sel);

    ext_slot_num = prog->ext_slot_num;
    ext_slot_idx = prog->ext_slot_idx;
    for(i = 0; i < ext_slot_num; i++)
    {
        ext_slot_idx[i] = -1;
    }
    prog->user_info.userdata_len = 0;
    prog->fifo = NULL;
    prog->fifo_sel = -1;
    prog->user_info.userdata = NULL;
    prog->user_info.userdata_len = 0;
    prog->dvr_handle = -1;
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    prog->sw_buffer_ptr = NULL;
    prog->hw_buffer_ptr = NULL;
#endif
    prog->vslot_index = -1;
    prog->aslot_index = -1;
    prog->prog_id = 0;
    prog->ext_slot_num = 0;
    prog->start_ticktime.seconds = 0;
    prog->start_ticktime.microsecs = 0;
    prog->fill_packet_timems = 0;
    prog->timems[0] = prog->timems[1] = 0;
    return;
}

static void _ts_rec_prog_stop(ProgDmxInfo *prog)
{
    if(NULL == prog)
        return;

    if(prog->fifo)
    {
        if(1 == ts_multi_fifo_use_cnt_get(prog->fifo))
            _ts_rec_prog_release(prog);
        else
            _ts_rec_prog_unset(prog);
    }
    return;
}

void app_ts_record_stop(handle_t handle)
{
    ProgDmxInfo *prog = (ProgDmxInfo *)handle;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    GxCore_MutexLock(ctrl->mutex);
    _ts_rec_prog_stop(prog);
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

void app_ts_record_destroy(void)
{
    int i = 0;
    TsRecCtrl *ctrl = &thiz_TsRecCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(false == ctrl->inited)
        goto end;

    for(i = 0; i < ctrl->max_prog; i++)
    {
        _ts_rec_prog_stop(ctrl->prog + i);
    }

    TS_REC_MEM_FREE(ctrl->prog);
    ctrl->status = TS_REC_STATUS_STOPPED;
    ctrl->thread_id = 0;
    ctrl->fifo_size = 0;
    ctrl->max_prog = 0;

    _ts_rec_demux_module_close();
    _ts_rec_demux_device_close();

    memset(ctrl->slot, 0, sizeof(GxDemuxProperty_Slot) * TS_REC_DEMUX_SLOT_MAX);
    ctrl->inited = false;
end:
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

#if 0
#define TS_REC_PROG_MAX (4)

static handle_t test_handle[TS_REC_PROG_MAX] = {0};
static const char *file_name[TS_REC_PROG_MAX] = {"/test.ts", "/test2.ts", "/test3.ts", "/test4.ts"};
static bool test_flag = true;
static int32_t test_thread_id = 0;

static void test_thread(void *usrdata)
{
    uint8_t *buffer = NULL;
    int32_t fd[TS_REC_PROG_MAX] = {0};
    int32_t ret_len = 0, i = 0;

    for(i = 0; i < TS_REC_PROG_MAX; i++)
    {
        if(0 == test_handle[i])
            continue;

        fd[i] = GxCore_Open(file_name[i], "w");
        if(fd[i] < 0)
            goto end;
    }

    buffer = GxCore_Calloc(TS_REC_DUMP_DVB_BLOCK_SIZE, sizeof(uint8_t));
    if(NULL == buffer)
    {
        TS_REC_INFO("no memory");
        goto end;
    }

    GxCore_ThreadDetach();
    i = 0;

    while(test_flag)
    {
        if(test_handle[i] != 0)
        {
            ret_len = app_ts_record_read(test_handle[i], buffer, TS_REC_DUMP_DVB_BLOCK_SIZE);
            if(ret_len > 0)
                GxCore_Write(fd[i], buffer, 1, ret_len);
            else
                GxCore_ThreadDelay(10);
        }

        i = (i + 1) % TS_REC_PROG_MAX;
    }

end:
    if(buffer)
        GxCore_Free(buffer);

    for(i = 0; i < TS_REC_PROG_MAX; i++)
    {
        if(fd[i] > 0)
            GxCore_Close(fd[i]);
    }
    return;
}

void ts_rec_test_func(void)
{
    int32_t i = 0;
#if 0
    uint16_t prog_id[TS_REC_PROG_MAX] = {1, 3, 6, 7};
#else
    uint16_t prog_id[TS_REC_PROG_MAX] = {1, 1, 1, 0};
#endif
    TsRecConfig config[TS_REC_PROG_MAX];
    GxBusPmDataProg prog = {0};

    TS_REC_INFO("start test");
    app_ts_record_init(80 * TS_REC_DUMP_DVB_BLOCK_SIZE, TS_REC_PROG_MAX);

    memset(config, 0, TS_REC_PROG_MAX * sizeof(TsRecConfig));
    for(i = 0; i < TS_REC_PROG_MAX; i++)
    {
        memset(&prog, 0, sizeof(GxBusPmDataProg));
        if(prog_id[i] > 0)
        {
            GxBus_PmProgGetById(prog_id[i], &prog);
            config[i].prog_id = prog.id;
            config[i].user_pmt = true;
            test_handle[i] = app_ts_record_start(&config[i]);
        }
    }
    test_flag = true;
    if(0 == test_thread_id)
        GxCore_ThreadCreate("test_thread", &test_thread_id, test_thread, NULL, 16*1024, GXOS_DEFAULT_PRIORITY - 1);
}
void stop_test_ts_rec(void)
{
    TS_REC_INFO("stop test");
    test_flag = false;
    test_thread_id = 0;
    int i = 0;
    for(i = 0; i < TS_REC_PROG_MAX; i++)
    {
        if(test_handle[i] != 0)
            app_ts_record_stop(test_handle[i]);
    }
    app_ts_record_destroy();
    memset(test_handle, 0, sizeof(handle_t));
}
#endif

#endif
