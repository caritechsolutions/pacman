#include "app.h"
#include "dvbhal/gxdemux_hal.h"
#ifdef SUBTTRANS_SUPPORT
#include "gx_subttrans.h"

#define PSI_INVALID_PID (0x1FFF)
#define SECTION_SIZE (64*1024+6)
#define SUBTITILE_TABLEID  (0xbd)
#define ST_DEMUX_INIT_FLAG (0x12345678)

#define AV_RB16(x)  ((((uint8_t*)(x))[0] << 8) | ((uint8_t*)(x))[1])

typedef struct _st_demux {
    int32_t    dmx_id;
    uint32_t   tsid;
    handle_t   lock;
    int32_t    _init;
}st_demux_t;

typedef struct _st_dmx_info {
    handle_t dmx_handle;
    uint32_t pid;
    uint16_t service_id;
    uint8_t  subt_type;
    char lang[SUBT_LANG_MAX];
} st_dmx_info_t;

static st_demux_t st_dmx = {0};
static st_dmx_info_t s_dmx_info = {0};

static void st_demux_send_subtdata_msg(uint8_t *data, uint32_t length)
{
    uint8_t *pes_data = NULL;
    AppMsg_SubtTransControl st_msg = { 0 };

    if(data && (length > 0) && (NULL != (pes_data = GxCore_Malloc(length))))
    {
        memcpy(pes_data, data, length);
        st_msg.type = STMSG_DVB_SUBT_GET;
        st_msg.data = pes_data;
        st_msg.data_len = length;
        st_msg.service_id = s_dmx_info.service_id;
        st_msg.subt_type = s_dmx_info.subt_type;
        strncpy(st_msg.lang, s_dmx_info.lang, SUBT_LANG_MAX);
        //GxTime  time_start;
        //GxCore_GetTickTime(&time_start);
        //app_log_debug("[ST Demux][%ds,%dus] SendMSG->STMSG_DVB_SUBT_GET: service_id=0x%x, subt_type=%d, lang:%s\n",
        //               time_start.seconds, time_start.microsecs, st_msg.service_id,s_dmx_info.subt_type,st_msg.lang);
        app_send_msg_exec(APPMSG_SUBT_TRANS_CONTROL, &st_msg);
    }
}

static int gx_subttrans_dvbsub_decode(const uint8_t *buf, int buf_size)
{
    const uint8_t *p, *p_end;
    int segment_type;
    int got_segment = 0;
    int segment_length;

    if (buf_size <= 6 || *buf != 0x0f) {
        app_log_error("%s %d: incomplete or broken packet\n", __FUNCTION__, __LINE__);
        return -1;
    }
    p = buf;
    p_end = buf + buf_size;

    while (p_end - p >= 6 && *p == 0x0f) {
        p += 1;
        segment_type = *p++;
        p += 2;
        segment_length = AV_RB16(p);
        p += 2;

        if (p_end - p < segment_length) {
            return -1;
        }
        got_segment |=  segment_type;
        p += segment_length;
    }
    if((got_segment & 0x0f) != 0)
        return 1;
    return 0;
}

static int gx_subttrans_pes_parse(uint8_t *packet, uint32_t length)
{
    int ret = 0;
    uint8_t *buf = packet, *out_buf = NULL, *p, *p_end;
    unsigned int   buf_size = 0, out_size = 0, buf_pos = 0;
    int payload_size = 0, header_len = 0;

    if(buf[0] != 0x00 || buf[1] != 0x00 || buf[2] != 0x01)
        return 0;

    buf += 3;
    payload_size = (buf[1] << 8 | buf[2]);
    if(payload_size > (length - 6))
        return 0;
    buf += 3;

    header_len = buf[2];
    buf += 3;
    buf += header_len;
    buf_size = payload_size - (header_len - 3);

    if(buf_size < 2 || buf[0] != 0x20 || buf[1] != 0x00)
        return 0;

    buf_pos = 2;
    out_buf = p = buf + buf_pos;
    p_end = buf + (buf_size - buf_pos);
    while(p < p_end)
    {
        if(*p == 0x0f)
        {
            if(6 <= p_end - p)
            {
                unsigned int len = (p[4] << 8 | p[5]);
                if(len + 6 <= p_end - p)
                {
                    out_size += len + 6;
                    p += len + 6;
                }
                else
                    break;
            }
            else
                break;
        }
        else
            break;
    }

    if(out_buf != NULL && out_size > 0)
    {
        ret = gx_subttrans_dvbsub_decode(out_buf, out_size);
    }

    if(ret == 1) //IS SUBT Display info PES
    {
        st_demux_send_subtdata_msg(packet, length);
    }
    return 0;
}

static void st_demux_task(void *args)
{
    uint8_t *packet = NULL;
    uint32_t length = 0;
    int ret = 0;
    GxCore_ThreadDetach();
    packet = GxCore_Malloc(SECTION_SIZE);
    if(packet == NULL) {
        app_log_error("[ST]: demux malloc failed\n");
        return;
    }

    while(st_dmx._init == ST_DEMUX_INIT_FLAG) {
        if(s_dmx_info.dmx_handle <= 0){
            GxCore_ThreadDelay(10);
            continue;
        }
        GxCore_MutexLock(st_dmx.lock);
        ret = GxDmxRead(s_dmx_info.dmx_handle, packet, SECTION_SIZE, (uint32_t *)&length);
        if((ret == 0) && (length > 0) && packet)
        {
            gx_subttrans_pes_parse(packet, length);
        }

        GxCore_MutexUnlock(st_dmx.lock);
        GxCore_ThreadDelay(10);
    }
    GxCore_Free(packet);
}

int Gx_SubtTrans_SiInit(uint32_t dmx_id, uint32_t tsid)
{
    int32_t ret = 0;
    handle_t st_demux_thread;

    ret = GxDmxInit();
    if(ret < 0) {
        app_log_error("[ST]: GxDmxInit failed\n");
        return -1;
    }
    ret = GxDmxSetSource(dmx_id, tsid);
    if(ret < 0) {
        app_log_error("[ST]: GxDmxSetSource failed\n");
        return -1;
    }
    memset(&st_dmx, 0, sizeof(st_dmx));
    memset(&s_dmx_info, 0, sizeof(s_dmx_info));
    st_dmx.dmx_id = dmx_id;
    st_dmx.tsid   = tsid;
    st_dmx._init = ST_DEMUX_INIT_FLAG;
    GxCore_MutexCreate(&st_dmx.lock);
    GxCore_ThreadCreate("st_demux", &st_demux_thread, st_demux_task, NULL, 128 * 1024, GXOS_DEFAULT_PRIORITY);

    return ret;
}

int Gx_SubtTrans_SiDestory(void)
{
    int32_t ret = 0;
    if(s_dmx_info.dmx_handle > 0)
        ret = GxDmxStop(s_dmx_info.dmx_handle);
    ret = GxDmxDeinit();
    GxCore_MutexDelete(st_dmx.lock);
    memset(&st_dmx, 0, sizeof(st_dmx));
    memset(&s_dmx_info, 0, sizeof(s_dmx_info));
    return ret;
}

int Gx_SubtTrans_SiPesStop(void)
{
    int ret = -1;
    if(s_dmx_info.dmx_handle > 0)
    {
        ret = GxDmxStop(s_dmx_info.dmx_handle);
        memset(&s_dmx_info, 0, sizeof(s_dmx_info));
    }
    return ret;
}

int Gx_SubtTrans_SiPesStart(uint32_t pid, uint16_t service_id, uint8_t subt_type, char *lang)
{
    GxDmxFilterParams params = { 0 };
    if((pid >= PSI_INVALID_PID)|| (pid<=0))
        return -1;
    ST_API_PRINTF("[ST Demux] GxDmxPesStart, PID=0x%x\n", pid);
    Gx_SubtTrans_SiPesStop();
    memset(&params, 0, sizeof(GxDmxFilterParams));
    params.pid =  pid;
    params.depth = 1;
    params.match[0] = SUBTITILE_TABLEID;
    params.mask[0] = 0xff;
    params.depth = 1;
    params.flags = EQ_FLAG | REPEAT_FLAG;
    params.size = 128 * 1024;  // PES 类型默认128KB
    params.timeout = 0;
    s_dmx_info.dmx_handle = GxDmxPesStart(st_dmx.dmx_id, params);
    if(s_dmx_info.dmx_handle <= 0)
        return -1;

    s_dmx_info.service_id = service_id;
    s_dmx_info.pid = pid;
    s_dmx_info.subt_type = subt_type;
    if(lang)
        strncpy(s_dmx_info.lang, lang, SUBT_LANG_MAX);
    return 0;
}
#endif



