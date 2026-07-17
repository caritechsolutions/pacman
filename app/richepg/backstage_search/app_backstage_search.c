#include <stdarg.h>
#include "dvbhal/gxdemux_hal.h"
#include "si/app_si_parse.h"
#include "module/si/si_pat.h"
#include "backstage_search_private.h"
#include "module/frontend/gxfrontend_module.h"
#include "app_module.h"
#include "app.h"

#if RICHEPG_SUPPORT
enum LogLevel
{
    LOG_DBG = 0,
    LOG_INFO,
    LOG_ERR,
};

static int32_t thiz_debug_level = LOG_INFO;

static int32_t _log(int32_t debug_level, const char *function, int32_t lineno, char *format, ...);

#define BACKSTAGE_SEARCH_DBG(format, args...)  do {_log(LOG_DBG, __func__, __LINE__, format, ##args);}  while(0)
#define BACKSTAGE_SEARCH_INFO(format, args...) do {_log(LOG_INFO, __func__, __LINE__, format, ##args);} while(0)
#define BACKSTAGE_SEARCH_ERR(format, args...)  do {_log(LOG_ERR, __func__, __LINE__, format, ##args);}  while(0)

static BackStageSearchCtrl thiz_BackStageSearchCtrl = {
    .mutex                       = 0,
    .thread                      = 0,
    .status                      = SEARCH_START,
    .inited                      = false,
    .service_info                = NULL,
    .service_num                 = 0,
    .pat_body                    = NULL,
    .pat_body_count              = 0,
    .pat_timeout                 = 0,
    .pmt_body                    = {0},
    .pmt_video_stream_body       = NULL,
    .pmt_audio_stream_body       = NULL,
    .pmt_audio_stream_body_count = 0,
    .pmt_filter_count            = 0,
    .pmt_finish_count            = 0,
    .pmt_timeout                 = 0,
    .dmx_id                      = 0,
};

static int32_t _log(int32_t debug_level, const char *function, int32_t lineno, char *format, ...)
{
    int i;
    char *sev;
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
            sev = "\033[34m[backstage][debug]";
            break;
        case LOG_INFO:
            sev = "\033[32m[backstage][info]";
            break;
        case LOG_ERR:
            sev = "\033[1;31m[backstage][error]";
            break;
        default:
            sev = "";
            break;
    }
    app_log_info("%s %s\n\033[0m", sev, buffer);

    return 0;
}

static void _filter_stop(TableInfo *table)
{
    if(NULL == table)
        return;

    if(table->handle != -1)
    {
        GxDmxStop(table->handle);
        table->handle = -1;
        table->type = PAT_TABLE;
        table->section_count = 0;
        table->section_deduplication = false;
        memset(table->section_state, 0, sizeof(table->section_state));
        memset(&table->table_cfg, 0, sizeof(private_table_cfg));
    }

    return;
}

static void _variable_exit_init(void)
{
    int32_t i = 0;

    for(i = 0; i < MAX_FILTER; i++)
    {
        _filter_stop(&thiz_BackStageSearchCtrl.table[i]);
    }
    thiz_BackStageSearchCtrl.status = SEARCH_FINISH;
    thiz_BackStageSearchCtrl.service_num = 0;
    thiz_BackStageSearchCtrl.pat_body_count = 0;
    thiz_BackStageSearchCtrl.pat_timeout = 0;
    thiz_BackStageSearchCtrl.pmt_filter_count = 0;
    thiz_BackStageSearchCtrl.pmt_finish_count = 0;
    thiz_BackStageSearchCtrl.pmt_audio_stream_body_count = 0;
    thiz_BackStageSearchCtrl.pmt_timeout = 0;
    GxDmxClearSource(thiz_BackStageSearchCtrl.dmx_id);
    thiz_BackStageSearchCtrl.dmx_id = 0;
    memset(&thiz_BackStageSearchCtrl.pmt_body, 0, sizeof(SearchPmtBody));
    SEARCH_FREE(thiz_BackStageSearchCtrl.service_info);
    SEARCH_FREE(thiz_BackStageSearchCtrl.pat_body);
    SEARCH_FREE(thiz_BackStageSearchCtrl.pat_body);
    SEARCH_FREE(thiz_BackStageSearchCtrl.pmt_audio_stream_body);
    SEARCH_FREE(thiz_BackStageSearchCtrl.pmt_video_stream_body);
    return;
}

static int32_t _table_status_get(TableInfo *table, si_info_t *p_si_info)
{
#define FORCE_SUBTABLE_OK_TIMES (3)
    uint16_t total_section = 0, i = 0;

    total_section = p_si_info->last_sec_number + 1;

    if(((table->section_state[p_si_info->sec_number/8]>>(p_si_info->sec_number%8))&1) != 1)
        table->section_state[p_si_info->sec_number/8] |= (1<<(p_si_info->sec_number%8));
    else
        return SECTION_REPEAT;

    table->section_count++;
    if(table->section_count < p_si_info->last_sec_number*FORCE_SUBTABLE_OK_TIMES)
    {
        for(i = 0; i < total_section; i++)
        {
            if(((table->section_state[i/8]>>(i%8))&1) != 1)
                return SECTION_OK;
        }
    }

    return SUBTABLE_OK;
}

static bool _service_id_check(uint16_t service_id)
{
    int32_t i = 0;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    for(i = 0; i < ctrl->service_num; i++)
    {
        if(service_id == ctrl->service_info[i].servicd_id)
            return true;
    }

    return false;
}

static int32_t _pat_section_ok(TableInfo *table, uint8_t *parsed_data)
{
    uint16_t             i         = 0;
    PatInfo             *pat_info  = NULL;
    prog_info_t         *prog_info = NULL;
    BackStageSearchCtrl *ctrl      = &thiz_BackStageSearchCtrl;

    if(NULL == table || NULL == parsed_data)
        goto err;

    pat_info = (PatInfo *)parsed_data;
    prog_info = (prog_info_t *)(pat_info->prog_info);

    for(i = 0; i < pat_info->prog_count; i++)
    {
        if(false == _service_id_check(prog_info[i].prog_num))
            continue;

        ctrl->pat_body_count++;
        ctrl->pat_body = GxCore_Realloc(ctrl->pat_body, ctrl->pat_body_count * sizeof(SearchPatBody));
        if(NULL == ctrl->pat_body)
        {
            BACKSTAGE_SEARCH_ERR("No enough memory to store");
            goto err;
        }

        ctrl->pat_body[ctrl->pat_body_count - 1].prog_number = prog_info[i].prog_num;
        ctrl->pat_body[ctrl->pat_body_count - 1].pmt_pid = prog_info[i].pmt_pid;
        ctrl->pat_body[ctrl->pat_body_count - 1].ts_id = pat_info->si_info.ts_id;
        ctrl->pat_body[ctrl->pat_body_count - 1].pat_version = pat_info->si_info.ver_number;
        BACKSTAGE_SEARCH_DBG("sid: %d, pmt_pid: %d, ts_id: %d", prog_info[i].prog_num, prog_info[i].pmt_pid, pat_info->si_info.ts_id);
    }
    return 0;
err:
    BACKSTAGE_SEARCH_INFO("Search happen error");
    ctrl->status = SEARCH_FINISH;
    return -1;
}

static int32_t _table_section_ok(TableInfo *table, uint8_t tid, uint8_t *parsed_data)
{
    int32_t ret = -1;

    switch(tid)
    {
        case PAT_TID:
            ret = _pat_section_ok(table, parsed_data);
            break;

        case PMT_TID:
            break;

        case NIT_ACTUAL_NETWORK_TID:
            break;

        case NIT_OTHER_NETWORK_TID:
            break;

        default:
            break;
    }

    return ret;
}

static int32_t _pmt_filter_start(void)
{
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    if(0 == ctrl->pat_body_count)
    {
        BACKSTAGE_SEARCH_ERR("pat body count is zero");
        return -1;
    }

    do
    {
        int32_t i = 0;
        uint16_t prog_number = 0;
        TableInfo *table = NULL;
        GxDmxFilterParams params = {0};

        prog_number     = ctrl->pat_body[ctrl->pmt_filter_count].prog_number;
        params.pid      = ctrl->pat_body[ctrl->pmt_filter_count].pmt_pid;
        params.depth    = 5;
        params.timeout  = PMT_TIMEOUT;
        params.flags    = params.flags | EQ_FLAG;
        params.flags    = params.flags | REPEAT_FLAG;
        params.flags    = params.flags | CRC_FLAG;
        params.match[0] = PMT_TID;
        params.mask[0]  = 0xff;
        params.match[3] = ((prog_number >> 8) & 0xff);
        params.mask[3]  = 0xff;
        params.match[4] = (prog_number & 0xff);
        params.mask[4]  = 0xff;

        for(i = 0; i < MAX_FILTER; i++)
        {
            if(-1 == ctrl->table[i].handle)
            {
                table = &ctrl->table[i];
                break;
            }
        }

        if(NULL == table)
        {
            BACKSTAGE_SEARCH_INFO("No enough filter");
            break;
        }

        table->type = PMT_TABLE;
        table->section_deduplication = true;
        table->table_cfg.mode = PARSE_WITH_STANDARD;
        table->table_cfg.table_parse_fun = NULL;
        table->handle = GxDmxStart(ctrl->dmx_id, params);
        if(-1 == table->handle)
        {
            BACKSTAGE_SEARCH_INFO("call GxDmxStart failed");
            break;
        }

        ctrl->pmt_filter_count++;

    }while(ctrl->pmt_filter_count < ctrl->pat_body_count);

    if(0 == ctrl->pmt_filter_count)
    {
        BACKSTAGE_SEARCH_ERR("no pmt can be created");
        return -1;
    }

    return 0;
}

static int32_t _pat_subtable_ok(TableInfo *table, uint8_t *parsed_data)
{
    uint16_t             i         = 0;
    PatInfo             *pat_info  = NULL;
    prog_info_t         *prog_info = NULL;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    if(NULL == table || NULL == parsed_data)
        goto err;

    pat_info = (PatInfo *)parsed_data;
    prog_info = (prog_info_t *)(pat_info->prog_info);

    for(i = 0; i < pat_info->prog_count; i++)
    {
        if(false == _service_id_check(prog_info[i].prog_num))
            continue;

        ctrl->pat_body_count++;
        ctrl->pat_body = GxCore_Realloc(ctrl->pat_body, ctrl->pat_body_count * sizeof(SearchPatBody));
        if(NULL == ctrl->pat_body)
        {
            BACKSTAGE_SEARCH_ERR("No enough memory to store");
            goto err;
        }

        ctrl->pat_body[ctrl->pat_body_count - 1].prog_number = prog_info[i].prog_num;
        ctrl->pat_body[ctrl->pat_body_count - 1].pmt_pid = prog_info[i].pmt_pid;
        ctrl->pat_body[ctrl->pat_body_count - 1].ts_id = pat_info->si_info.ts_id;
        ctrl->pat_body[ctrl->pat_body_count - 1].pat_version = pat_info->si_info.ver_number;
        BACKSTAGE_SEARCH_DBG("sid: %d, pmt_pid: %d, ts_id: %d", prog_info[i].prog_num, prog_info[i].pmt_pid, pat_info->si_info.ts_id);
    }

    _filter_stop(table);

    if(-1 == _pmt_filter_start())
    {
        BACKSTAGE_SEARCH_ERR("pmt filter start failed");
        goto err;
    }

    return 0;
err:
    ctrl->status = SEARCH_FINISH;
    return -1;
}

static int32_t _pmt_next_filter_start(void)
{
    int32_t i = 0;
    uint16_t prog_number = 0;
    TableInfo *table = NULL;
    GxDmxFilterParams params = {0};
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    prog_number     = ctrl->pat_body[ctrl->pmt_filter_count].prog_number;
    params.pid      = ctrl->pat_body[ctrl->pmt_filter_count].pmt_pid;
    params.depth    = 5;
    params.timeout  = PMT_TIMEOUT;
    params.flags    = params.flags | EQ_FLAG;
    params.flags    = params.flags | REPEAT_FLAG;
    params.flags    = params.flags | CRC_FLAG;
    params.match[0] = PMT_TID;
    params.mask[0]  = 0xff;
    params.match[3] = ((prog_number >> 8) & 0xff);
    params.mask[3]  = 0xff;
    params.match[4] = (prog_number & 0xff);
    params.mask[4]  = 0xff;

    for(i = 0; i < MAX_FILTER; i++)
    {
        if(-1 == ctrl->table[i].handle)
        {
            table = &ctrl->table[i];
            break;
        }
    }

    if(NULL == table)
    {
        BACKSTAGE_SEARCH_INFO("No enough filter");
        return -1;
    }

    table->type = PMT_TABLE;
    table->section_deduplication = true;
    table->table_cfg.mode = PARSE_WITH_STANDARD;
    table->table_cfg.table_parse_fun = NULL;
    table->handle = GxDmxStart(ctrl->dmx_id, params);
    if(-1 == table->handle)
    {
        BACKSTAGE_SEARCH_INFO("call GxDmxStart failed");
        return -1;
    }

    ctrl->pmt_filter_count++;
    return 0;
}

static int32_t _check_ca(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    return 1;
}

static void _prog_check_audio_lang(uint16_t *audio_pid,
                                   GxBusPmDataProgAudioType *type,
                                   uint16_t *ecm_pid,
                                   GxBusPmDataStream *pmt_audio_stream_body,
                                   uint32_t pmt_audio_stream_body_count)
{
    uint32_t i = 0;
    uint32_t j = 0;
    int8_t   lang_buff[128] = {0};
    int8_t   lang[3][4];
    int8_t   flag = -1;//-1代表还没有赋值，0代表没有找到优先语言，1代表找到了最优语言，2代表着找到了第二优语言
    int8_t  *p = 0;
    uint8_t  count = 0;

    memset(lang, 0, 3*4);
    if(NULL == GxBus_ConfigGet(GXBUS_SEARCH_LANG_PRIORITY, (char*)lang_buff, 128, GXBUS_SEARCH_LANG_DEFAULT))
        strcpy((char*)lang_buff,GXBUS_SEARCH_LANG_DEFAULT);

    //lang_buff = "eng_chn"等等表示一个或者师多个国家的iso9660代码组合
    //现在支持两种优先语言
    for(p = lang_buff; ;)
    {
        if(*p != '\0' && count != 2)
        {
            memcpy(lang[count], p, 3);
            count++;
            p += 3;
        }

        if(*p != '\0' && count != 2)
            p++;
        else
            break;
    }

    if(pmt_audio_stream_body_count == 1)
    {
        *audio_pid = pmt_audio_stream_body[0].audio_pid;
        *type = pmt_audio_stream_body[0].audio_type;
        *ecm_pid = pmt_audio_stream_body[0].ecm_pid;
    }
    else
    {
        for(i = 0; i < pmt_audio_stream_body_count; i++)
        {
            if(pmt_audio_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
            {
                if ( *audio_pid == pmt_audio_stream_body[i].audio_pid )
                {
                    *type = pmt_audio_stream_body[i].audio_type;
                    *ecm_pid = pmt_audio_stream_body[i].ecm_pid;
                    return ;
                }
            }
        }
        if ( (i == pmt_audio_stream_body_count) && (pmt_audio_stream_body_count>0) )
        {
            *audio_pid = pmt_audio_stream_body[0].audio_pid;
            *type = pmt_audio_stream_body[0].audio_type;
            *ecm_pid = pmt_audio_stream_body[0].ecm_pid;
        }
    }

    for(i = 0; i < pmt_audio_stream_body_count; i++)
    {
        if(pmt_audio_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
        {
            for(j = 0; j < sizeof(pmt_audio_stream_body[i].name); j++)
            {
                pmt_audio_stream_body[i].name[j] = tolower(pmt_audio_stream_body[i].name[j]);
            }

            if(-1 == flag)
            {
                *audio_pid = pmt_audio_stream_body[0].audio_pid;
                *type = pmt_audio_stream_body[0].audio_type;
                *ecm_pid = pmt_audio_stream_body[0].ecm_pid;
                flag = 0;
            }

            if(0 == strcmp((const char*)(pmt_audio_stream_body[i].name), (const char*)(lang[0])))
            {
                *audio_pid = pmt_audio_stream_body[i].audio_pid;
                *type = pmt_audio_stream_body[i].audio_type;
                *ecm_pid = pmt_audio_stream_body[i].ecm_pid;
                flag = 1;
                break;
            }
            else if(0 == strcmp((const char*)(pmt_audio_stream_body[i].name), (const char*)(lang[1])))
            {
                *audio_pid = pmt_audio_stream_body[i].audio_pid;
                *type = pmt_audio_stream_body[i].audio_type;
                *ecm_pid = pmt_audio_stream_body[i].ecm_pid;
                flag = 2;
            }
        }
    }

    return;
}

static int32_t _pat_find(SearchPmtBody pmt_body, SearchPatBody *pat)
{
    uint32_t             i = 0;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    for(i = 0; i < ctrl->pat_body_count; i++)
    {
        if (pmt_body.service_id == ctrl->pat_body[i].prog_number)
        {
            memcpy(pat, &(ctrl->pat_body[i]), sizeof(SearchPatBody));
            BACKSTAGE_SEARCH_DBG("pat pos = %d", i);
            return i;
        }
    }

    BACKSTAGE_SEARCH_ERR("pat can't find");
    return -1;
}

static int32_t _prog_info_modify(GxBusPmDataStream *pmt_audio_stream_body,
                                 uint32_t pmt_audio_stream_body_count,
                                 SearchPmtBody pmt_body)
{
    int32_t                  i = 0, j = 0, cnt = 0, mod = 0;
    GxBusPmDataProg          prog, prog_bak;
    GxBusPmDataProgAudioType type = 0xff;
    BackStageSearchCtrl     *ctrl = &thiz_BackStageSearchCtrl;
    SearchPatBody            pat_info = {0};

    if(NULL == pmt_audio_stream_body)
        return -1;

    for(i = 0; i < ctrl->service_num; i++)
    {
        if(pmt_body.service_id != ctrl->service_info[i].servicd_id)
            continue;

        memset(&prog, 0, sizeof(GxBusPmDataProg));
        GxBus_PmProgGetById(ctrl->service_info[i].prog_id, &prog);
        if(0 == prog.id)
        {
            BACKSTAGE_SEARCH_ERR("can't find prog data by id: %d", ctrl->service_info[i].prog_id);
            goto end;
        }
        memcpy(&prog_bak, &prog, sizeof(GxBusPmDataProg));

        if(-1 == _pat_find(pmt_body, &pat_info))
            goto end;
        if(pmt_body.video_count > 0)
            prog.service_type = GXBUS_PM_PROG_TV;
        else if(pmt_audio_stream_body_count > 0)
            prog.service_type = GXBUS_PM_PROG_RADIO;
        else
            goto end;

        prog.video_type = pmt_body.service_type;

        if(true == pmt_body.pmt_ca_desc.desc_valid)
        {
            prog.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
            prog.cas_id = pmt_body.pmt_ca_desc.cas_id;
            prog.ecm_pid_v = pmt_body.ecm_pid_video;
        }
        else
        {
            prog.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
            prog.cas_id = CAS_ID_FREE;
        }

        prog.video_pid = pmt_body.video_pid;
        prog.pcr_pid = pmt_body.pcr_pid;

        if(true == pmt_body.pmt_ttx_desc.ttx_desc_valid)
        {
            prog.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
            if(true == pmt_body.pmt_ttx_desc.cc_desc_valid)
                prog.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
        }
        else
        {
            prog.ttx_flag = GXBUS_PM_PROG_BOOL_DISABLE;
            prog.cc_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        if(true == pmt_body.pmt_subt_desc.desc_valid)
            prog.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
        else
            prog.subt_flag = GXBUS_PM_PROG_BOOL_DISABLE;

        prog.pmt_pid = pat_info.pmt_pid;
        prog.pat_version = pat_info.pat_version;
        prog.audio_count = pmt_body.audio_count;    //不包括ac3

        if(true == pmt_body.pmt_ac3_desc.desc_valid)
        {
            prog.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
            for(j = 0; j < pmt_audio_stream_body_count; j++)
            {
                if(pmt_audio_stream_body[j].audio_type == GXBUS_PM_AUDIO_AC3)//ac3
                {
                    prog.ac3_pid = pmt_audio_stream_body[j].audio_pid;
                    prog.cur_audio_type = GXBUS_PM_AUDIO_AC3;
                    break;
                }
                else if(pmt_audio_stream_body[j].audio_type == GXBUS_PM_AUDIO_EAC3)//eac3
                {
                    prog.ac3_pid = pmt_audio_stream_body[j].audio_pid;
                    prog.cur_audio_type = GXBUS_PM_AUDIO_EAC3;
                }
            }
        }
        else
        {
            prog.ac3_flag = GXBUS_PM_PROG_BOOL_DISABLE;
        }

        _prog_check_audio_lang(&(prog.cur_audio_pid),
                &type,
                &(prog.cur_audio_ecm_pid),
                pmt_audio_stream_body,
                pmt_audio_stream_body_count);
        if(type != 0xff)
            prog.cur_audio_type = type;

        if(memcmp(&prog_bak, &prog, sizeof(GxBusPmDataProg)) != 0)
        {
            BACKSTAGE_SEARCH_INFO("pog_id: %d prog name: %s service_id: %d\n", prog.id, prog.prog_name, prog.service_id);
            GxBus_PmProgInfoModify(&prog);
            ++mod;
        }
        ++cnt;
    }

end:
    if(cnt > 0)
    {
        if(mod > 0)
        {
            GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        }
        return 0;
    }
    return -1;
}

static int32_t _pmt_cas_get(uint32_t *cas1,
                            uint32_t  cas1_count,
                            uint32_t *cas2,
                            uint32_t  cas2_count,
                            uint32_t  stream_type)
{
    uint32_t             i = 0;
    CaDescriptor        *cas_decriptor = NULL;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    switch (stream_type)
    {
        /*  case MPEG_1_VIDEO:
            case MPEG_2_VIDEO:
            case AVS:
            case H264:*/
        case 0://视频
            if(cas2 != NULL)   //以分开加密优先
            {
                cas_decriptor = (CaDescriptor *)(cas2[0]);
                ctrl->pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
                ctrl->pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
                for(i = 0; i < cas2_count; i++)
                {
                    cas_decriptor = (CaDescriptor *) (cas2[i]);
                    if(cas_decriptor->elem_pid == ctrl->pmt_body.video_pid)
                    {
                        if(_check_ca(cas_decriptor->ca_system_id, ctrl->pmt_body.video_pid, cas_decriptor->ca_pid))
                        {
                            ctrl->pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
                            ctrl->pmt_body.pmt_ca_desc.cas_id = cas_decriptor->ca_system_id;
                            break;
                        }
                    }
                }
            }
            if(i == cas2_count && cas1 != NULL)    //分开加密没找到,找相同加密
            {
                cas_decriptor = (CaDescriptor *) (cas1[0]);
                ctrl->pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
                ctrl->pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
                for(i = 0; i < cas1_count; i++)
                {
                    cas_decriptor = (CaDescriptor *)(cas1[i]);
                    if(_check_ca(cas_decriptor->ca_system_id, ctrl->pmt_body.video_pid, cas_decriptor->ca_pid))
                    {
                        ctrl->pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
                        ctrl->pmt_body.pmt_ca_desc.cas_id = cas_decriptor->ca_system_id;
                        break;
                    }
                }
            }
            break;

            /*  case MPEG_1_AUDIO:
                case MPEG_2_AUDIO:
                case AAC_ADTS:
                case AAC_LATM:*/
        case 1://音频
            if(cas2 != NULL)   //以分开加密优先
            {
                cas_decriptor = (CaDescriptor *)(cas2[0]);
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].ecm_pid = cas_decriptor->ca_pid;

                for(i = 0; i < cas2_count; i++)
                {
                    cas_decriptor = (CaDescriptor *)(cas2[i]);
                    if(cas_decriptor->elem_pid == ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid)
                    {
                        if(_check_ca(cas_decriptor->ca_system_id,
                                     ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid,
                                     cas_decriptor->ca_pid))
                        {
                            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].ecm_pid = cas_decriptor->ca_pid;
                            break;
                        }
                    }

                }
            }
            if(i == cas2_count && cas1 != NULL)    //分开加密没找到,找相同加密
            {
                cas_decriptor = (CaDescriptor *)(cas1[0]);
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].ecm_pid = cas_decriptor->ca_pid;

                for(i = 0; i < cas1_count; i++)
                {
                    cas_decriptor = (CaDescriptor *)(cas1[i]);
                    if(_check_ca(cas_decriptor->ca_system_id,
                                 ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid,
                                 cas_decriptor->ca_pid))
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].ecm_pid = cas_decriptor->ca_pid;
                        break;
                    }
                }
            }
            break;
    }
    return 0;
}

static int32_t _pmt_info_get(uint8_t *parsed_data)
{
    uint32_t            *cas_info = NULL;
    uint32_t            *cas_info2 = NULL;
    PmtInfo             *pmt_info = NULL;
    stream_info_t       *stream_info = NULL;
    Ac3Descriptor       *ac3_descriptor = NULL;
    Eac3Descriptor      *eac3_descriptor = NULL;
    TeletextDescriptor  *ttx_descriptor = NULL;
    Teletext            *ttx_info = NULL;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;
    uint32_t             stream_info_count = 0;
    uint32_t             i = 0;
    uint32_t             j = 0;
    uint32_t             stream_type = 0;
    status_t             ret = 0;
    uint32_t             video_audio = 0;

    if(NULL == parsed_data)
        return -1;

    pmt_info = (PmtInfo *)parsed_data;
    stream_info_count = (pmt_info->stream_count) + (pmt_info->ac3_count) +(pmt_info->eac3_count);   //加上ac3的空间
    stream_info = (stream_info_t *)(pmt_info->stream_info);

    ctrl->pmt_audio_stream_body = GxCore_Calloc(sizeof(GxBusPmDataStream), stream_info_count);
    ctrl->pmt_audio_stream_body_count = 0;

    ctrl->pmt_video_stream_body = GxCore_Calloc(sizeof(GxBusPmDataVideoStream), stream_info_count);
    ctrl->pmt_body.video_count = 0;
    ctrl->pmt_body.pcr_pid = pmt_info->pcr_pid;
    ctrl->pmt_body.service_id = pmt_info->prog_num;

    if(pmt_info->ca_count != 0)    //先判断音视频是不是同密
    {
        ctrl->pmt_body.pmt_ca_desc.desc_valid = true;
        cas_info = pmt_info->ca_info;
    }
    if(pmt_info->ca_count2 != 0)   //再判断音视频是不是分开加密
    {
        ctrl->pmt_body.pmt_ca_desc.desc_valid = true;
        cas_info2 = pmt_info->ca_info2;
    }
    ctrl->pmt_body.video_pid = 0;//避免上一张pmt的数据残留
    for(i = 0; i < pmt_info->stream_count; i++)    //这里只获得pcm的信息
    {
        video_audio = 2;
        stream_type = stream_info[i].stream_type;
        switch (stream_type)
        {
            case MPEG_1_VIDEO:
            case MPEG_2_VIDEO:
                ctrl->pmt_body.service_type = GXBUS_PM_PROG_MPEG;
                video_audio = 0;
                break;
            case MPEG_4_VIDEO:
                ctrl->pmt_body.service_type = GXBUS_PM_PROG_MPEG4;
                video_audio = 0;
                break;
            case H264:
                ctrl->pmt_body.service_type = GXBUS_PM_PROG_H264;
                video_audio = 0;
                break;
            case H265:
                ctrl->pmt_body.service_type = GXBUS_PM_PROG_H265;
                video_audio = 0;
                break;

            case AVS:
                ctrl->pmt_body.service_type = GXBUS_PM_PROG_AVS;
                video_audio = 0;
                break;
            case MPEG_1_AUDIO:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG1;
                video_audio = 1;
                break;
            case MPEG_2_AUDIO:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG2;
                video_audio = 1;
                break;
            case AAC_ADTS:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
                video_audio = 1;
                break;
            case AAC_LATM:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
                video_audio = 1;
                break;
            case LPCM:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_LPCM;
                video_audio = 1;
                break;
            case AC3:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
                video_audio = 1;
                break;
            case DTS:
                {
                    int32_t dts = 0;
                    GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT, &dts, GXBUS_SEARCH_DTS_YES);
                    if(dts == GXBUS_SEARCH_DTS_YES)
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS;
                        video_audio = 1;
                    }
                }
                break;
            case DOLBY_TRUEHD:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
                video_audio = 1;
                break;
            case AC3_PLUS:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
                video_audio = 1;
                break;
            case DTS_HD:
                {
                    int32_t dts = 0;
                    GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
                    if(dts == GXBUS_SEARCH_DTS_YES)
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD;
                        video_audio = 1;
                    }
                }
                break;
            case DTS_MA:
                {
                    int32_t dts = 0;
                    GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
                    if(dts == GXBUS_SEARCH_DTS_YES)
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_MA;
                        video_audio = 1;
                    }
                }
                break;
            case AC3_PLUS_SEC:
                ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
                video_audio = 1;
                break;
            case DTS_HD_SEC:
                {
                    int32_t dts = 0;
                    GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
                    if(dts == GXBUS_SEARCH_DTS_YES)
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
                        video_audio = 1;
                    }
                }
                break;
            default:
                {
                    uint8_t *d = pmt_info->registration;

                    if(d[0] == 'D' && d[1] == 'R' && d[2] == 'A' && d[3] == '1')
                    {
                        ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_DRA;
                        video_audio = 1;
                    }
                    else
                    {
                        BACKSTAGE_SEARCH_INFO("can't support stream type, type = 0x%x", stream_type);
                        video_audio = 2;
                    }
                }
                break;
        }
        if(video_audio == 0)
        {
            ctrl->pmt_body.video_pid = stream_info[i].elem_pid;
            _pmt_cas_get(cas_info, pmt_info->ca_count, cas_info2, pmt_info->ca_count2, video_audio);
            ctrl->pmt_video_stream_body[ctrl->pmt_body.video_count].video_pid = stream_info[i].elem_pid;
            ctrl->pmt_video_stream_body[ctrl->pmt_body.video_count].video_type = ctrl->pmt_body.service_type;
            if(ctrl->pmt_body.video_count > 0)
            {
                ctrl->pmt_body.video_pid = ctrl->pmt_video_stream_body[0].video_pid;
                ctrl->pmt_body.service_type = ctrl->pmt_video_stream_body[0].video_type;
            }
            ctrl->pmt_body.video_count++;
            j = 1;
        }
        else if(video_audio == 1)
        {
            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid = stream_info[i].elem_pid;
            _pmt_cas_get(cas_info, pmt_info->ca_count, cas_info2, pmt_info->ca_count2, video_audio);
            memset(ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].name, 0, 4);
            if(stream_info[i].iso639_count != 0)
                memcpy(ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].name, stream_info[i].iso639_info[0].iso639, 3);
            ctrl->pmt_body.audio_count++;    //不包括ac3
            ctrl->pmt_audio_stream_body_count++; //包括ac3
            j = 1;
        }
    }
    if(j == 0)
    {
        ret = -1;
        goto end;
    }

    ctrl->pmt_body.pmt_subt_desc.desc_valid = false;
    if(pmt_info->subt_count != 0)  //GxSearchPmtSubtitlingDesc等待si服务
        ctrl->pmt_body.pmt_subt_desc.desc_valid = true;

    ctrl->pmt_body.pmt_ttx_desc.ttx_desc_valid = false;
    ctrl->pmt_body.pmt_ttx_desc.cc_desc_valid = false;
    if(pmt_info->ttx_count != 0)
    {
        ctrl->pmt_body.pmt_ttx_desc.ttx_desc_valid = true;

        for(i = 0; i < pmt_info->ttx_count; i++)
        {
            //ttx_info是一个类似数组的东西，数组的元素是每一个ttx描俗的地址
            ttx_descriptor = (TeletextDescriptor *)(pmt_info->ttx_info[i]);

            for(j = 0; j < ttx_descriptor->ttx_num; j++)
            {
                if(ttx_descriptor->ttx)
                {
                    ttx_info = (Teletext*)((Teletext*)(ttx_descriptor->ttx) + j);
                }
                else
                {
                    BACKSTAGE_SEARCH_ERR("ttx descriptor error");
                    break;
                }

                if(ttx_info->type == 0x02 ||ttx_info->type == 0x05)   //0x02或者0x5是cc类型的ttx
                {
                    ctrl->pmt_body.pmt_ttx_desc.cc_desc_valid = true;
                    break;
                }
            }

            if(ctrl->pmt_body.pmt_ttx_desc.cc_desc_valid == true)
                break;
        }
    }

    ctrl->pmt_body.pmt_ac3_desc.desc_valid = false;
    if(pmt_info->ac3_count != 0)   //GxSearchPmtAc3Desc等待si服务pmt_audio_stream_body_count++
    {
        ctrl->pmt_body.pmt_ac3_desc.desc_valid = true;

        for(i = 0; i < pmt_info->ac3_count; i++)
        {
            ac3_descriptor = (Ac3Descriptor *)(pmt_info->ac3_info[i]);
            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid = ac3_descriptor->elem_pid;
            memset(ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].name, 0, 4);
            ctrl->pmt_audio_stream_body_count++;
        }
    }
    if(pmt_info->eac3_count != 0)  //GxSearchPmtAc3Desc等待si服务pmt_audio_stream_body_count++
    {
        ctrl->pmt_body.pmt_ac3_desc.desc_valid = true;

        for(i = 0; i < pmt_info->eac3_count; i++)
        {
            eac3_descriptor = (Eac3Descriptor *)(pmt_info->eac3_info[i]);
            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
            ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].audio_pid = eac3_descriptor->elem_pid;
            memset(ctrl->pmt_audio_stream_body[ctrl->pmt_audio_stream_body_count].name, 0, 4);
            ctrl->pmt_audio_stream_body_count++;
        }
    }

    ret = _prog_info_modify(ctrl->pmt_audio_stream_body, ctrl->pmt_audio_stream_body_count, ctrl->pmt_body);
    memset(&(ctrl->pmt_body), 0, sizeof(SearchPmtBody));

end:
    ctrl->pmt_audio_stream_body_count = 0;
    SEARCH_FREE(ctrl->pmt_audio_stream_body);
    ctrl->pmt_body.video_count = 0;
    SEARCH_FREE(ctrl->pmt_video_stream_body);
    return ret;
}

static int32_t _pmt_subtable_ok(TableInfo *table, uint8_t *parsed_data)
{
    int32_t ret = 0;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    if(0 == (ret = _pmt_info_get(parsed_data)))
    {
        ctrl->pmt_finish_count++;
        if(ctrl->pmt_filter_count < ctrl->pat_body_count)
        {
            _filter_stop(table);
            ret = _pmt_next_filter_start();
        }
        else
        {
            if(ctrl->pmt_finish_count == ctrl->pat_body_count)
                ctrl->status = SEARCH_FINISH;
        }
    }

    return ret;
}


static int32_t _table_subtable_ok(TableInfo *table, uint8_t tid, uint8_t *parsed_data)
{
    int32_t ret = -1;

    switch(tid)
    {
        case PAT_TID:
            ret = _pat_subtable_ok(table, parsed_data);
            break;

        case PMT_TID:
            ret = _pmt_subtable_ok(table, parsed_data);
            break;

        case NIT_ACTUAL_NETWORK_TID:
            break;

        case NIT_OTHER_NETWORK_TID:
            break;

        default:
            break;
    }

    return ret;
}

static int32_t _filter_timeout(TableInfo *table)
{
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    switch(table->type)
    {
        case PAT_TABLE:
            BACKSTAGE_SEARCH_DBG("PAT timeout\n");
            ctrl->status = SEARCH_FINISH;
            break;

        case PMT_TABLE:
            BACKSTAGE_SEARCH_DBG("PMT timeout\n");
            ctrl->pmt_finish_count++;
            if(ctrl->pmt_filter_count < ctrl->pat_body_count)
            {
                _filter_stop(table);
                _pmt_next_filter_start();
            }
            else
            {
                if(ctrl->pmt_finish_count == ctrl->pat_body_count)
                    ctrl->status = SEARCH_FINISH;
            }
            break;

        default:
            break;
    }

    return 0;
}


static int32_t _section_parse(TableInfo *table, uint8_t *section_data, int32_t parse_size)
{
    if(NULL == table || NULL == section_data || 0 == parse_size)
        return -1;

    int32_t i = 0, ret = 0;
    int32_t sec_len = 0;
    uint8_t *parsed_data = NULL;

    for(i = 0; i < parse_size;)
    {
        sec_len = TODATA12(section_data[i + 1], section_data[i + 2]);

        ret = _table_status_get(table, (si_info_t *)section_data);
        if(SECTION_REPEAT == ret && true == table->section_deduplication)
        {
            i += (sec_len + 3);
            continue;
        }

        if(0 == GxDmxCheckSectionCrc(section_data + i))
            parsed_data = app_si_parse(&table->table_cfg, &section_data[i], sec_len + 3);
        else
            parsed_data = SEC_LEN_ERROR;

        if(NULL == parsed_data)
        {
            BACKSTAGE_SEARCH_ERR("call app_si_parse failed");
            goto err;
        }
        if(SEC_NO_MEMORY == parsed_data)
        {
            BACKSTAGE_SEARCH_ERR("No enough memory to store");
            goto err;
        }
        else if(SEC_SYNTAX_INDICATOR == parsed_data)
        {
            BACKSTAGE_SEARCH_ERR("syntax indicator error");
            goto err;
        }
        else if(SEC_LEN_ERROR == parsed_data)
        {
            BACKSTAGE_SEARCH_ERR("section len error");
            goto err;
        }

        if(SUBTABLE_OK == ret)
        {
            _table_subtable_ok(table, section_data[i], parsed_data);
            break;
        }
        else if(SECTION_OK == ret)
        {
            _table_section_ok(table, section_data[i], parsed_data);
        }

        SEARCH_FREE(parsed_data);
        i += (sec_len + 3);
    }

    SEARCH_FREE(parsed_data);
    return 0;
err:
    return -1;
}

static void backstage_search_console(void *arg)
{
    int32_t  per_read_size  = 28 * 1024;
    uint8_t *section_data = NULL;

    GxCore_ThreadDetach();

    section_data = GxCore_Calloc(per_read_size, sizeof(uint8_t));
    if(NULL == section_data)
    {
        BACKSTAGE_SEARCH_ERR("No enough memory");
        return;
    }

    while(1)
    {
        int32_t i = 0, ret = 0;
        uint32_t read_size = 0;
        TableInfo *table = NULL;
        AppFrontend_LockState lock = FRONTEND_UNLOCK;

        app_ioctl(0, FRONTEND_LOCK_STATE_GET, &lock);
        if(FRONTEND_UNLOCK == lock)
        {
            GxCore_ThreadDelay(100);
            continue;
        }

        GxCore_MutexLock(thiz_BackStageSearchCtrl.mutex);
        table = thiz_BackStageSearchCtrl.table;

        for(i = 0; i < MAX_FILTER; i++)
        {
            if(-1 == table[i].handle)
                continue;

            memset(section_data, 0, per_read_size);

            ret = GxDmxRead(table[i].handle, section_data, per_read_size, &read_size);
            if(-1 == ret)
            {
                _filter_timeout(table);
                continue;
            }
            else if((0 == ret && 0 == read_size) || -2 == ret)
            {
                continue;
            }
            else if(-3 == ret)
            {
                GxDmxReset(table[i].handle);
                continue;
            }

            if(-1 == _section_parse(&table[i], section_data, read_size))
                continue;
        }

        if(SEARCH_FINISH == thiz_BackStageSearchCtrl.status)
            _variable_exit_init();

        GxCore_MutexUnlock(thiz_BackStageSearchCtrl.mutex);
        GxCore_ThreadDelay(10);
    }

    GxCore_Free(section_data);
    return;
}

static int32_t _pat_filter_start(void)
{
    int32_t i = 0;
    TableInfo *table = NULL;
    GxDmxFilterParams params = {0};

    params.pid      = PAT_PID;
    params.depth    = 1;
    params.timeout  = PAT_TIMEOUT;
    params.flags    = params.flags | EQ_FLAG;
    params.flags    = params.flags | REPEAT_FLAG;
    params.flags    = params.flags | CRC_FLAG;
    params.match[0] = PAT_TID;
    params.mask[0]  = 0xff;

    for(i = 0; i < MAX_FILTER; i++)
    {
        if(-1 == thiz_BackStageSearchCtrl.table[i].handle)
        {
            table = &thiz_BackStageSearchCtrl.table[i];
            break;
        }
    }

    if(NULL == table)
    {
        BACKSTAGE_SEARCH_INFO("No enough filter");
        return -1;
    }

    table->type = PAT_TABLE;
    table->section_deduplication = true;
    table->table_cfg.mode = PARSE_WITH_STANDARD;
    table->table_cfg.table_parse_fun = NULL;
    table->handle = GxDmxStart(thiz_BackStageSearchCtrl.dmx_id, params);
    if(-1 == table->handle)
    {
        BACKSTAGE_SEARCH_ERR("call GxDmxStart failed");
        return -1;
    }

    return 0;
}

static int32_t _params_init(void)
{
    int32_t i = 0;

    for(i = 0; i < MAX_FILTER; i++)
    {
        thiz_BackStageSearchCtrl.table[i].handle = -1;
    }

    thiz_BackStageSearchCtrl.status = SEARCH_START;

    return 0;
}

void app_backstage_search_stop(void)
{
    if(false == thiz_BackStageSearchCtrl.inited)
        return;

    GxCore_MutexLock(thiz_BackStageSearchCtrl.mutex);
    _variable_exit_init();
    GxCore_MutexUnlock(thiz_BackStageSearchCtrl.mutex);

    return;
}

static int32_t _valid_service_info_get(uint32_t sat_id, uint32_t tp_id)
{
    int32_t i = 0, prog_num = 0;
    ServiceInfo *service_info = NULL;
    GxBusPmDataProg prog_data = {0};
    GxBusPmViewInfo view_info = {0};
    GxBusPmViewInfo view_info_bak = {0};
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    if(GXCORE_ERROR == GxBus_PmViewInfoGet(&view_info))
        return -1;

    view_info_bak = view_info;

    view_info.stream_type = GXBUS_PM_PROG_ALL;
    view_info.group_mode = GROUP_MODE_SAT;
    view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
    view_info.sat_id = sat_id;

    if(GXCORE_ERROR == GxBus_PmViewInfoModify(&view_info))
        return -1;

    prog_num = GxBus_PmProgNumGet();
    if(prog_num <= 0)
        goto err;

    ctrl->service_info = GxCore_Calloc(prog_num, sizeof(ServiceInfo));
    if(NULL == ctrl->service_info)
    {
        BACKSTAGE_SEARCH_ERR("No enough memory");
        goto err;
    }

    service_info = ctrl->service_info;

    for(i = 0; i < prog_num; i++)
    {
        if(-1 == GxBus_PmProgGetByPos(i, 1, &prog_data))
        {
            BACKSTAGE_SEARCH_INFO("call GxBus_PmProgGetByPos failed");
            continue;
        }

        // if(0 == prog_data.video_pid && 0 == prog_data.cur_audio_pid && tp_id == prog_data.tp_id)
        if(tp_id == prog_data.tp_id)
        {
            service_info[ctrl->service_num].servicd_id = prog_data.service_id;
            service_info[ctrl->service_num].prog_id = prog_data.id;
            ctrl->service_num++;
        }
    }

    if(0 == ctrl->service_num)
        goto err;

    GxBus_PmViewInfoModify(&view_info_bak);
    return 0;
err:
    GxBus_PmViewInfoModify(&view_info_bak);
    return -1;
}

int32_t app_backstage_search_start(uint32_t ts_src, uint32_t dmx_id, uint32_t sat_id, uint32_t tp_id)
{
    if(0 == thiz_BackStageSearchCtrl.mutex)
        GxCore_MutexCreate(&thiz_BackStageSearchCtrl.mutex);

    BACKSTAGE_SEARCH_INFO("\033[32mts_src=%u, dmx_id=%u, sat_id=%u, tp_id=%u\033[0m\n", ts_src, dmx_id, sat_id, tp_id);
    app_backstage_search_stop();

    GxCore_MutexLock(thiz_BackStageSearchCtrl.mutex);

    GxDmxInit();
    GxDmxSetSource(dmx_id, ts_src);

    if(-1 == _valid_service_info_get(sat_id, tp_id))
    {
        BACKSTAGE_SEARCH_ERR("valid_service_info_get failed");
        goto err;
    }

    _params_init();
    thiz_BackStageSearchCtrl.dmx_id = dmx_id;

    if(-1 == _pat_filter_start())
    {
        BACKSTAGE_SEARCH_ERR("start pat filter failed");
        goto err;
    }

    if(0 == thiz_BackStageSearchCtrl.thread)
    {
        if(GXCORE_ERROR == GxCore_ThreadCreate("dmx read", &thiz_BackStageSearchCtrl.thread, backstage_search_console, NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY))
            goto err;
    }

    thiz_BackStageSearchCtrl.inited = true;
    GxCore_MutexUnlock(thiz_BackStageSearchCtrl.mutex);
    return 0;
err:
    GxCore_MutexUnlock(thiz_BackStageSearchCtrl.mutex);
    app_backstage_search_stop();
    return -1;
}

int app_backstage_search_status(void)
{
    int flag = 0;
    BackStageSearchCtrl *ctrl = &thiz_BackStageSearchCtrl;

    flag = (ctrl->status == SEARCH_FINISH) ? 0 : -1;

    return flag;
}
#endif
