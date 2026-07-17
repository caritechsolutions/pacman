#include "app_epg.h"
#include "module/app_log.h"
#include "module/app_psi_module.h"
#include "module/si/si_public_parser.h"
#include "app_preset_prog_info_correct.h"
#include "app_default_params.h"

#if PRESET_PROG_INFO_CORRECT_SUPPORT
#define ERROR_TS_ID 0x1fff

typedef struct _PmtFilterInfo
{
    uint16_t prog_id;
    uint16_t service_id;
    uint16_t pmt_pid;
    uint8_t filter_id;
}PmtFilterInfo;

typedef struct _PresetProgInfoCorrectCtrl
{
    PmtFilterInfo *pmt_filter_info;
    uint32_t prog_num;
    uint32_t pmt_filter_num;
    uint32_t pmt_finish_num;
    uint8_t  pat_filter_id;
    uint16_t ts_id;
    uint32_t dmx_id;
    handle_t mutex;
}PresetProgInfoCorrectCtrl;

static PresetProgInfoCorrectCtrl s_PresetProgInfoCorrectCtrl = {
    .pmt_filter_info = NULL,
    .prog_num = 0,
    .pmt_filter_num = 0,
    .pmt_finish_num = 0,
    .pat_filter_id = 0,
    .ts_id = ERROR_TS_ID,
    .dmx_id = 0,
    .mutex = 0,
};

static status_t _pmt_subtable_ok(void *data, psi_module_status state);
static void _pmt_next_filter_create(void);
static void _preset_prog_info_correct_stop(void);

static bool _preset_prog_info_invalid_check(GxBusPmDataProg *prog)
{
    if(NULL == prog)
        return false;

    if(ERROR_TS_ID == prog->ts_id)
        return true;

    return false;
}

static int32_t _invalid_service_info_get(uint32_t tp_id)
{
#define MAX_PROG_NUM (200)
    int32_t i = 0, prog_num = 0;
    GxBusPmDataProg *prog_array = NULL;
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    prog_num = GxBus_PmProgNumGetByTpId(tp_id);
    if(prog_num <= 0)
        goto err;

    ctrl->pmt_filter_info = GxCore_Calloc(MAX_PROG_NUM, sizeof(PmtFilterInfo));
    if(NULL == ctrl->pmt_filter_info)
    {
        app_log_error("[%s, %d] no enough memory\n", __func__, __LINE__);
        goto err;
    }

    prog_array = GxCore_Calloc(prog_num, sizeof(GxBusPmDataProg));
    if(NULL == prog_array)
    {
        app_log_error("[%s, %d] no enough memory\n", __func__, __LINE__);
        goto err;
    }

    prog_num = GxBus_PmProgGetByTpId(tp_id, prog_num, prog_array);
    if(prog_num <= 0)
        goto err;

    for(i = 0; i < prog_num; i++)
    {
        if(true == _preset_prog_info_invalid_check(prog_array + i))
        {
            ctrl->pmt_filter_info[ctrl->prog_num].prog_id = prog_array[i].id;
            ctrl->pmt_filter_info[ctrl->prog_num].service_id = prog_array[i].service_id;
            ctrl->pmt_filter_info[ctrl->prog_num].pmt_pid = prog_array[i].pmt_pid;
            ctrl->prog_num++;
        }

        if(MAX_PROG_NUM - 1 == ctrl->prog_num)
            break;
    }

    if(0 == ctrl->prog_num)
        goto err;

    APP_FREE(prog_array);
    return 0;
err:
    APP_FREE(ctrl->pmt_filter_info);
    APP_FREE(prog_array);
    ctrl->prog_num = 0;
    return -1;
}

static status_t _pmt_timeout(uint8_t filter_id)
{
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    GxCore_MutexLock(ctrl->mutex);
    ctrl->pmt_finish_num++;
    if(ctrl->pmt_filter_num < ctrl->prog_num)
        _pmt_next_filter_create();

    if(ctrl->pmt_finish_num == ctrl->prog_num)
        _preset_prog_info_correct_stop();
    GxCore_MutexUnlock(ctrl->mutex);

    return GXCORE_SUCCESS;
}

static status_t _pmt_subtable_ok(void *data, psi_module_status state)
{
    int32_t i = 0;
    PmtInfo *pmt_info = (PmtInfo *)data;
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;
    GxBusPmDataProg prog = {0};

    GxCore_MutexLock(ctrl->mutex);
    ctrl->pmt_finish_num++;

    for(i = 0; i < ctrl->prog_num; i++)
    {
        if(ctrl->pmt_filter_info[i].service_id != pmt_info->prog_num)
            continue;

        if(GXCORE_ERROR == GxBus_PmProgGetById(ctrl->pmt_filter_info[i].prog_id, &prog))
        {
            app_log_error("get prog info failed\n");
            break;
        }

        if(ctrl->ts_id != ERROR_TS_ID)
            prog.ts_id = ctrl->ts_id;

        if(GXCORE_SUCCESS == GxBus_PmProgInfoModify(&prog))
            GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        break;
    }

    if(ctrl->pmt_filter_num < ctrl->prog_num)
        _pmt_next_filter_create();

    if(ctrl->pmt_finish_num == ctrl->prog_num)
        _preset_prog_info_correct_stop();

    GxCore_MutexUnlock(ctrl->mutex);
    return GXCORE_SUCCESS;
}

static void _pmt_next_filter_create(void)
{
    psi_module_config config = {0};
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    uint16_t pmt_pid = ctrl->pmt_filter_info[ctrl->pmt_filter_num].pmt_pid;
    uint16_t service_id = ctrl->pmt_filter_info[ctrl->pmt_filter_num].service_id;

    config.demux_id = ctrl->dmx_id;
    config.params.pid = pmt_pid;
    config.params.flags |= CRC_FLAG;
    config.params.flags |= REPEAT_FLAG;
    config.params.flags |= EQ_FLAG;
    config.params.match[0] = PMT_TID;
    config.params.mask[0] = 0xff;
    config.params.match[3] = service_id >> 8;
    config.params.mask[3] = 0xff;
    config.params.match[4] = service_id & 0xff;
    config.params.mask[4] = 0xff;
    config.params.depth = 5;
    config.node.timeout = 7000;
    config.node.independent_thread = true;
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_solv_cb = _pmt_subtable_ok;
    config.node.parse_timeout_cb = _pmt_timeout;
    ctrl->pmt_filter_info[ctrl->pmt_filter_num].filter_id = app_psi_module_start(config);
    if(ctrl->pmt_filter_info[ctrl->pmt_filter_num].filter_id != 0)
        ctrl->pmt_filter_num++;
    return;
}

static void _pmt_filter_create(void)
{
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    do
    {
        psi_module_config config = {0};
        uint16_t pmt_pid = ctrl->pmt_filter_info[ctrl->pmt_filter_num].pmt_pid;
        uint16_t service_id = ctrl->pmt_filter_info[ctrl->pmt_filter_num].service_id;

        config.demux_id = ctrl->dmx_id;
        config.params.pid = pmt_pid;
        config.params.flags |= CRC_FLAG;
        config.params.flags |= REPEAT_FLAG;
        config.params.flags |= EQ_FLAG;
        config.params.match[0] = PMT_TID;
        config.params.mask[0] = 0xff;
        config.params.match[3] = service_id >> 8;
        config.params.mask[3] = 0xff;
        config.params.match[4] = service_id & 0xff;
        config.params.mask[4] = 0xff;
        config.params.depth = 5;
        config.node.timeout = 7000;
        config.node.independent_thread = true;
        config.node.mode = PSI_MODULE_STANDARD_ONLY;
        config.node.parse_solv_cb = _pmt_subtable_ok;
        config.node.parse_timeout_cb = _pmt_timeout;
        ctrl->pmt_filter_info[ctrl->pmt_filter_num].filter_id = app_psi_module_start(config);
        if(ctrl->pmt_filter_info[ctrl->pmt_filter_num].filter_id != 0)
            ctrl->pmt_filter_num++;
    }while(ctrl->pmt_filter_info[ctrl->pmt_filter_num].filter_id != 0 && ctrl->pmt_filter_num < ctrl->prog_num);
    return;
}

static status_t _pat_subtable_ok(void *data, psi_module_status state)
{
    int32_t epg_ctr = 0;
    PatInfo *pat_info = (PatInfo *)data;
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    if(NULL == pat_info)
        return GXCORE_ERROR;

    GxBus_ConfigGetInt(EPG_INFO_CLEAN_KEY, &epg_ctr, EPG_INFO_CLEAN_TP_CHANGE);
    if(EPG_INFO_CLEAN_TP_CHANGE == epg_ctr)
        app_epg_ts_id_filter_modify(pat_info->si_info.ts_id);

    GxCore_MutexLock(ctrl->mutex);
    ctrl->ts_id = pat_info->si_info.ts_id;
    _pmt_filter_create();
    GxCore_MutexUnlock(ctrl->mutex);

    return GXCORE_SUCCESS;
}

static status_t _pat_timeout(uint8_t filter_id)
{
    app_preset_prog_info_correct_stop();
    return GXCORE_SUCCESS;
}

int32_t app_preset_prog_info_correct_start(uint32_t dmx_id, uint32_t tp_id)
{
    psi_module_config config = {0};
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    if(0 == ctrl->mutex && GxCore_MutexCreate(&ctrl->mutex) != GXCORE_SUCCESS)
        return -1;

    GxCore_MutexLock(ctrl->mutex);
    if(_invalid_service_info_get(tp_id) < 0)
        goto err;

    config.demux_id = dmx_id;
    config.params.pid = PAT_PID;
    config.params.match[0] = PAT_TID;
    config.params.mask[0] = 0xff;
    config.params.depth = 1;
    config.params.flags |= CRC_FLAG;
    config.params.flags |= EQ_FLAG;
    config.params.flags |= REPEAT_FLAG;
    config.node.timeout = 5000;
    config.node.independent_thread = true;
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_solv_cb = _pat_subtable_ok;
    config.node.parse_timeout_cb = _pat_timeout;

    ctrl->pat_filter_id = app_psi_module_start(config);
    if(0 == ctrl->pat_filter_id)
        goto err;

    ctrl->dmx_id = dmx_id;
    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
err:
    _preset_prog_info_correct_stop();
    GxCore_MutexUnlock(ctrl->mutex);
    return -1;
}

static void _preset_prog_info_correct_stop(void)
{
    int32_t i = 0;
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    for(i = 0; i < ctrl->prog_num; i++)
    {
        if(ctrl->pmt_filter_info[i].filter_id != 0)
            app_psi_module_stop(ctrl->pmt_filter_info[i].filter_id);
    }

    APP_FREE(ctrl->pmt_filter_info);
    app_psi_module_stop(ctrl->pat_filter_id);
    ctrl->pat_filter_id = 0;
    ctrl->prog_num = 0;
    ctrl->pmt_filter_num = 0;
    ctrl->pmt_finish_num = 0;
    ctrl->ts_id = ERROR_TS_ID;
    ctrl->dmx_id = 0;
}

bool app_preset_prog_info_correct_running_check(void)
{
    bool ret = false;
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(0 == ctrl->prog_num)
        ret = false;
    else
        ret = true;
    GxCore_MutexUnlock(ctrl->mutex);

    return ret;
}

void app_preset_prog_info_correct_stop(void)
{
    PresetProgInfoCorrectCtrl *ctrl = &s_PresetProgInfoCorrectCtrl;

    if(0 == ctrl->mutex && GxCore_MutexCreate(&ctrl->mutex) != GXCORE_SUCCESS)
        return;

    GxCore_MutexLock(ctrl->mutex);
    _preset_prog_info_correct_stop();
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}
#endif
