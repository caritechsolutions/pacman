#include "app_default_params.h"
#include "module/app_emm.h"
#if EMM_SUPPORT

typedef struct
{
    uint8_t section_data[EMM_DATA_LENGTH];
    uint32_t sub_idx;
} EmmInfo;

static handle_t s_emm_mutex = -1;
static handle_t s_emm_fifo = -1;
static handle_t s_emm_thread_handle = -1;
static struct app_emm s_AppEmm = {0};

static void app_emm_thread(void *arg)
{
    EmmInfo *emm_info = GxCore_Malloc(sizeof(EmmInfo));
    if (!emm_info)
    {
        return;
    }

    while (1)
    {
        memset(emm_info, 0, sizeof(EmmInfo));
        if (app_fifo_read(s_emm_fifo, (char *)emm_info, sizeof(EmmInfo)) > 0)
        {
            app_emm_analyse(emm_info->sub_idx, emm_info->section_data);
        }
        GxCore_ThreadDelay(10);
    }
}

status_t app_emm_analyse(uint32_t sub_idx, uint8_t *section_data)
{
    struct app_emm *emm = &s_AppEmm;

    //app_log_debug("\033[0mGet Current EMM Data, CASID : 0x%x, tabid = 0x%x!!\n\033[0m", caid, section_data[0]);
    {
        DataInfoClass data_info = {0};
        ControlClass control_info = {0};
        data_info.psi_type = DATA_EMM;
        data_info.data = section_data;
        data_info.data_len = ((((section_data[1] & 0xf) << 8) | section_data[2]) + 3);
        control_info.demux_id = emm->demuxid;
        control_info.pid = emm->emm_sub[sub_idx].pid;
        control_info.control_type = CONTROL_NORMAL;
        data_info.cas_id  = emm->emm_sub[sub_idx].ca_id;
        app_send_psi_data(&data_info, &control_info);

    }
    return GXCORE_SUCCESS;
}

status_t app_emm_get(GxMsgProperty_SiSubtableOk *data)
{
    int32_t i = 0;
    uint32_t length = 0;
    status_t ret = GXCORE_ERROR;
    struct app_emm *emm = &s_AppEmm;
    EmmInfo *emm_info = GxCore_Malloc(sizeof(EmmInfo));
    if (!emm_info)
    {
        return ret;
    }

    GxCore_MutexLock(s_emm_mutex);

    for (i = 0; i < emm->emm_count; i++)
    {
        if (data->si_subtable_id == emm->emm_sub[i].subt_id
                && data->request_id == emm->emm_sub[i].req_id)
        {
        if ((data->parsed_data[0] & 0xF0) == 0x80)
            {
            memset(emm_info, 0, sizeof(EmmInfo));
            length = ((data->parsed_data[1] & 0x0f) << 8) | data->parsed_data[2];
            length += 3;
            if (length <= EMM_DATA_LENGTH)
            {
                //APP_PRINT("---[app_emm_get] data len : %d---\n", length);
                memcpy(emm_info->section_data, data->parsed_data, length);
                emm_info->sub_idx = i;
                app_fifo_write(s_emm_fifo, (char *)emm_info, sizeof(EmmInfo));
                ret = GXCORE_SUCCESS;
            }
            }
            GxBus_SiParserBufFree(data->parsed_data);
            break;
        }
    }

    GxCore_Free(emm_info);
    emm_info = NULL;
    GxCore_MutexUnlock(s_emm_mutex);

    return ret;
}

void app_emm_init(void)
{
    int32_t i = 0;
    struct app_emm *emm = &s_AppEmm;

    if (s_emm_mutex == -1)
    {
        GxCore_MutexCreate(&s_emm_mutex);
    }

    GxBus_ConfigSetInt(SI_CONFIG_PER_READ_SIZE, 128 * 1024);
    GxCore_MutexLock(s_emm_mutex);

    if (s_emm_fifo == -1)
    {
        app_fifo_create(&s_emm_fifo, sizeof(EmmInfo), EMM_DATA_FIFO_NUM);
        if (s_emm_thread_handle == -1)
        {
            GxCore_ThreadCreate("app_emm_proc", &s_emm_thread_handle, app_emm_thread, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);
        }
    }

    memset(emm, 0, sizeof(struct app_emm));
    for (i = 0; i < EMM_MAX_COUNT; i++)
    {
        emm->emm_sub[i].subt_id = -1;
    }
    GxCore_MutexUnlock(s_emm_mutex);
}

void app_emm_start(uint16_t ts_src, uint32_t demuxid, uint32_t pid, uint32_t caid, uint32_t timeout)
{
    int32_t i = 0;
    struct app_emm *emm = &s_AppEmm;
    int32_t ca_count = -1;
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;

    if (emm->emm_count >= EMM_MAX_COUNT)
    {
        return;
    }

    GxCore_MutexLock(s_emm_mutex);
    //emm->stop(emm);

    for (i = 0; i < emm->emm_count; i++)
    {
        if (emm->emm_sub[i].pid == pid && emm->emm_sub[i].ca_id == caid)
        {
            GxCore_MutexUnlock(s_emm_mutex);
            return;
        }
    }

    for (i = 0; i < EMM_MAX_COUNT; i++)
    {
        if (emm->emm_sub[i].subt_id == -1 && emm->emm_sub[i].pid == 0)
        {
            ca_count = i;
            break;
        }
    }

    if (ca_count >= EMM_MAX_COUNT)
    {
        GxCore_MutexUnlock(s_emm_mutex);
        return;
    }

    emm->ts_src = ts_src;
    emm->demuxid = demuxid;
    emm->timeout = timeout;
    emm->emm_sub[ca_count].pid = pid;
    emm->emm_sub[ca_count].ca_id = caid;
    app_demux_param_adjust(&(emm->ts_src), &(emm->demuxid), 0);

    if(app_tsd_check())
    {
        app_exshift_pid_add(1,(int *)&(emm->emm_sub[i].pid));
    }

    memset(&subt_detail, 0, sizeof(GxSubTableDetail));
    subt_detail.ts_src = (uint16_t)emm->ts_src;
    subt_detail.demux_id = (uint16_t)emm->demuxid;
    subt_detail.si_filter.pid = (uint16_t)emm->emm_sub[ca_count].pid;
    subt_detail.si_filter.match_depth = 1;
    subt_detail.si_filter.eq_or_neq = EQ_MATCH;
    subt_detail.si_filter.match[0] = 0x82;
    subt_detail.si_filter.mask[0] = 0xF0;
    subt_detail.si_filter.crc = CRC_OFF;
    subt_detail.si_filter.sw_buffer_size = 128 * 1024;
    subt_detail.time_out = 0x7fffffff;//emm->timeout * 1000000;

    subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;
    subt_detail.table_parse_cfg.table_parse_fun = NULL;

    params_create = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void *)&params_create);

    emm->emm_sub[ca_count].subt_id = subt_detail.si_subtable_id;
    emm->emm_sub[ca_count].req_id = subt_detail.request_id;

    params_start = emm->emm_sub[ca_count].subt_id;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void *)&params_start);

    //for test
    APP_PRINT("---[app_emm_start][start]---\n");
    APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n", subt_detail.ts_src, subt_detail.si_filter.pid, emm->emm_sub[i].subt_id, emm->emm_sub[i].req_id);
    APP_PRINT("---[app_emm_start][end]---\n");

    emm->emm_sub[ca_count].ver = 0xff;
    emm->emm_count++;

    GxCore_MutexUnlock(s_emm_mutex);
}

uint32_t app_emm_release_one(uint32_t pid)
{
    int32_t i = 0;
    struct app_emm *emm = &s_AppEmm;
    GxCore_MutexLock(s_emm_mutex);

    for (i = 0; i < emm->emm_count; i++)
    {
        if (emm->emm_sub[i].pid == pid)
        {
            GxSubTableDetail subt_detail = {0};
            GxMsgProperty_SiGet si_get;
            GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)emm->emm_sub[i].subt_id;

            subt_detail.si_subtable_id = emm->emm_sub[i].subt_id;
            si_get = &subt_detail;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void *)&si_get);

            app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&params_release);
            emm->emm_sub[i].subt_id = -1;
            APP_PRINT("---[app_emm_release_one] pid : 0x%x---\n", pid);
			emm->emm_count--;
            break;
        }
    }

    GxCore_MutexUnlock(s_emm_mutex);
    return 0;
}

uint32_t app_emm_release_all(void)
{
    int32_t i = 0;
    struct app_emm *emm = &s_AppEmm;
    GxCore_MutexLock(s_emm_mutex);

    for (i = 0; i < emm->emm_count; i++)
    {
        if (emm->emm_sub[i].subt_id != -1)
        {
            GxSubTableDetail subt_detail = {0};
            GxMsgProperty_SiGet si_get;
            GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)emm->emm_sub[i].subt_id;

            APP_PRINT("---[app_emm_release_all][start] id : 0x%x---\n", emm->emm_sub[i].subt_id);
            subt_detail.si_subtable_id = emm->emm_sub[i].subt_id;
            si_get = &subt_detail;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void *)&si_get);

            app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&params_release);
            emm->emm_sub[i].subt_id = -1;
        }
    }

    emm->emm_count = 0;
    memset(emm->emm_sub, 0, sizeof(AppEmmSub_t)*EMM_MAX_COUNT);
    for (i = 0; i < EMM_MAX_COUNT; i++)
    {
        emm->emm_sub[i].subt_id = -1;
    }

    GxCore_MutexUnlock(s_emm_mutex);
    return 0;
}

status_t app_emm_timeout(GxParseResult *parse_result)
{
    int32_t i = 0;
    GxSubTableDetail subt_detail = {0};
    struct app_emm *emm = &s_AppEmm;
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;
    GxMsgProperty_SiRelease params_release;

    GxCore_MutexLock(s_emm_mutex);

    for (i = 0; i < emm->emm_count; i++)
    {
        if (parse_result->si_subtable_id == emm->emm_sub[i].subt_id
                && parse_result->request_id == emm->emm_sub[i].req_id)
        {
            APP_PRINT("---[app_emm_timeout][table_id = 0x%x][data->si_subtable_id = %d]\n",
                      parse_result->table_id, parse_result->si_subtable_id);
            params_release = (GxMsgProperty_SiRelease)emm->emm_sub[i].subt_id;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void *)&params_release);
            emm->emm_sub[i].subt_id = -1;

            memset(&subt_detail, 0, sizeof(GxSubTableDetail));
            subt_detail.ts_src = (uint16_t)emm->ts_src;
            subt_detail.demux_id = (uint16_t)emm->demuxid;
            subt_detail.si_filter.pid = (uint16_t)emm->emm_sub[i].pid;
            subt_detail.si_filter.match_depth = 1;
            subt_detail.si_filter.eq_or_neq = EQ_MATCH;
            subt_detail.si_filter.match[0] = 0x80;
            subt_detail.si_filter.mask[0] = 0xF0;
            subt_detail.si_filter.crc = CRC_OFF;
            subt_detail.si_filter.sw_buffer_size = 128 * 1024;
            subt_detail.time_out = emm->timeout * 1000000;
            subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;//PARSE_WITH_STANDARD;
            subt_detail.table_parse_cfg.table_parse_fun = NULL;//_emm_private;

            params_create = &subt_detail;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void *)&params_create);

            emm->emm_sub[i].subt_id = subt_detail.si_subtable_id;
            emm->emm_sub[i].req_id = subt_detail.request_id;

            params_start = emm->emm_sub[i].subt_id;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void *)&params_start);

            //for test
            APP_PRINT("---[app_emm_timeout][start]---\n");
            APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n", subt_detail.ts_src, subt_detail.si_filter.pid, emm->emm_sub[i].subt_id, emm->emm_sub[i].req_id);
            APP_PRINT("---[app_emm_timeout][end]---\n");

            emm->emm_sub[i].ver = 0xff;
            break;
        }
    }

    GxCore_MutexUnlock(s_emm_mutex);
    return GXCORE_SUCCESS;
}

#endif
