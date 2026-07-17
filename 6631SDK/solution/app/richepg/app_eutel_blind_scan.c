#include "gxcore.h"
#include <sys/ioctl.h>
#include "module/app_nim.h"
#include "app_utility.h"
#include "module/pm/gxpm_manage.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/app_log.h"
#if RICHEPG_SUPPORT
#include "app_eutel_database.h"

#ifndef ASSERT
#define ASSERT(a) do{if(!(a)){printf("Assert failure %s in %s:%d\n",#a,__func__,__LINE__);return -1;}}while(0)
#endif

#define EUTEL_BLIND_SCAN_FREE(x)	if(x){GxCore_Free(x);x=NULL;}

#define EUTEL_BLIND_SCAN_SAT_MAX_COUNT (100)
/*max tp count can be scaned per sat*/
#define EUTEL_BLIND_SCAN_TP_MAX_COUNT  (200)

/* 盲扫极化*/
#define BLIND_SCAN_POLAR_H          (0) // 18V
#define BLIND_SCAN_POLAR_V          (1) // 13V
#define BLIND_SCAN_POLAR_ALL        (2)
/* 盲扫本振*/
#define BLIND_SCAN_LNB_OCS          (0)
#define BLIND_SCAN_LNB_SINGLE       (1)
/* 盲扫本振类型*/
#define BLIND_SCAN_LNB_C            (0)
#define BLIND_SCAN_LNB_KU           (1)

#define BLIND_SCAN_STEP_MHZ         (12)
#define BLIND_SCAN_WINDOW_SIZE_K    (45000)
#define BLIND_SCAN_LOW_FREQ_MHZ     (950)
#define BLIND_SCAN_HIGH_FREQ_MHZ    (2150)

typedef enum
{
    EUTEL_BLIND_SCAN_STOP       = 0,
    EUTEL_BLIND_SCAN_START      = 1 << 1,
    EUTEL_BLIND_SCAN_CONTINUE   = 1 << 2,
    EUTEL_BLIND_SCAN_FINISH     = 1 << 3,
}EutelBlindScanStatus;

typedef struct
{
    /*前端设备的句柄*/
    handle_t                module_frontend;

    uint32_t                step_mhz;
    uint32_t                window_size_k;
    uint32_t                low_fre_mhz;
    uint32_t                high_fre_mhz;

    uint32_t                sat_total_num;
    uint32_t                sat_finish_num;
    uint32_t               *sat_id;
    uint32_t                cur_sat_id;

    uint32_t                cur_sat_tuner;
}EutelBlindScanClass;

typedef struct
{
    uint32_t                fre;//khz
    uint32_t                symbol_rate;
    uint32_t                pls_n;
    uint8_t                 stream_size;
    fe_modulation_t         qpsk;
}EutelBlindScanTp;

typedef struct
{
    uint16_t start_fre;
    uint16_t end_fre;

    uint8_t tp_polar;
    uint16_t reserved;

    uint16_t local_fre1;
    uint16_t local_fre2;

    uint8_t switch_22k;
    uint8_t polar;
    uint8_t lnb_ocs;//单本振或双本振
    uint8_t lnb_type;//c波段或者ku波段

    uint32_t tp_count;//盲扫到的tp数量

    EutelBlindScanTp* tp;
}EutelBlindScanParams;

typedef struct
{
    /*盲扫线程句柄*/
    handle_t                thread;

    /*互斥锁句柄*/
    handle_t                lock;

    /*盲扫状态*/
    EutelBlindScanStatus    status;

    RichepgTsTP             tp_array[EUTEL_BLIND_SCAN_TP_MAX_COUNT];
    uint32_t                tp_num;
    int                     scan_percent;
    int                     scan_total;
    int                     scan_count;
}EutelBlindScanCtrl;

static EutelBlindScanClass thiz_EutelBlindScanClass = {
    .module_frontend    = GXCORE_INVALID_POINTER,
    .sat_id             = NULL,
    .cur_sat_id         = 0,
    .sat_total_num      = 0,
    .sat_finish_num     = 0,
    .step_mhz           = 12,
    .window_size_k      = 45000,
    .low_fre_mhz        = 950,
    .high_fre_mhz       = 2150,
};

static EutelBlindScanCtrl thiz_EutelBlindScanCtrl = {
    .thread             = GXCORE_INVALID_POINTER,
    .lock               = GXCORE_INVALID_POINTER,
    .status             = EUTEL_BLIND_SCAN_STOP,
    .tp_num             = 0,
    .scan_percent       = 0,
    .scan_total         = 0,
    .scan_count         = 0,
};

static void _eutel_blind_scan_class_destory(void)
{
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    class->module_frontend  = GXCORE_INVALID_POINTER;
    class->cur_sat_id       = 0;
    class->sat_total_num    = 0;
    class->sat_finish_num   = 0;
    class->cur_sat_tuner    = 0;
    class->step_mhz         = 12;
    class->window_size_k    = 45000;
    class->low_fre_mhz      = 950;
    class->high_fre_mhz     = 2150;
    EUTEL_BLIND_SCAN_FREE(class->sat_id);
    return;
}

static int32_t _eutel_blind_scan_class_init(EutelBlindScan *params)
{
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    class->sat_finish_num   = 0;
    class->cur_sat_id       = 0;
    class->cur_sat_tuner    = 0;
    class->step_mhz         = 12;
    class->window_size_k    = 45000;
    class->low_fre_mhz      = 950;
    class->high_fre_mhz     = 2150;
    class->sat_total_num    = params->sat_num;

    EUTEL_BLIND_SCAN_FREE(class->sat_id);
    class->sat_id = GxCore_Calloc(class->sat_total_num, sizeof(uint32_t));
    if(NULL == class->sat_id)
    {
        app_log_error("\033[31m[%s, %d] malloc failed\033[0m\n", __func__, __LINE__);
        goto err;
    }
    memcpy(class->sat_id, params->sat_id, params->sat_num * sizeof(uint32_t));
    return 0;
err:
    _eutel_blind_scan_class_destory();
    return -1;
}

static int32_t _eutel_blind_scan_params_get(EutelBlindScanParams *params)
{
    uint32_t sat_id = 0;
    GxBusPmDataSat sat = {0};
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    params->tp_count = 0;
    memset(params->tp, 0, EUTEL_BLIND_SCAN_TP_MAX_COUNT * sizeof(EutelBlindScanTp));
    if(class->sat_total_num != 0 && class->sat_finish_num < class->sat_total_num)
    {
        sat_id = class->sat_id[class->sat_finish_num];
        if(GXCORE_ERROR == GxBus_PmSatGetById(sat_id, &sat))
        {
            app_log_error("\033[31m[%s, %d] get sat params failed\033[0m\n", __func__, __LINE__);
            return -1;
        }

        /*判断是双本振还是单本振*/
        params->local_fre1 = sat.sat_s.lnb1;
        params->local_fre2 = sat.sat_s.lnb2;

        if(sat.sat_s.lnb1 != sat.sat_s.lnb2 && sat.sat_s.lnb2 != 0)
            params->lnb_ocs = BLIND_SCAN_LNB_OCS;
        else
            params->lnb_ocs = BLIND_SCAN_LNB_SINGLE;

        params->switch_22k = sat.sat_s.switch_22K;

        /*判断盲扫的极化方向*/
        if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_ON)
            params->polar = BLIND_SCAN_POLAR_ALL;
        else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_18V)
            params->polar = BLIND_SCAN_POLAR_H;
        else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_13V)
            params->polar = BLIND_SCAN_POLAR_V;

        /*设置起始频率和结束频率*/
        params->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
        params->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
        /*判断是ku波段还是c波段*/
        if(sat.sat_s.lnb1<7000 && sat.sat_s.lnb2<7000)
            params->lnb_type = BLIND_SCAN_LNB_C;
        else
            params->lnb_type = BLIND_SCAN_LNB_KU;

        class->cur_sat_id = sat.id;
        class->cur_sat_tuner = sat.tuner;
        class->sat_finish_num++;
        class->module_frontend = GxFrontend_IdToHandle(sat.tuner);
        return 0;
    }
    return -1;
}

static int32_t _eutel_blind_scan_window(EutelBlindScanParams *scan_params)
{
    int32_t tp_num = 0;
    uint32_t fcenter = 0, i = 0;
    int scan_total = 0, scan_count = 0;
    int base_percent = 0, unit_percent = 0;
    struct fe_set_window_parameters params;
    struct fe_get_window_parameters get_params;
    struct dvb_frontend_parameters* tp_params = NULL;
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    if(scan_params->start_fre < BLIND_SCAN_LOW_FREQ_MHZ)
        scan_params->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;

    if(scan_params->end_fre > BLIND_SCAN_HIGH_FREQ_MHZ)
        scan_params->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;

    scan_total = (scan_params->end_fre + BLIND_SCAN_STEP_MHZ - scan_params->start_fre) / BLIND_SCAN_STEP_MHZ;
    GxCore_MutexLock(ctrl->lock);
    base_percent = 100 * ctrl->scan_count / ctrl->scan_total;
    unit_percent = (100 * 4) / (ctrl->scan_total * 10);
    GxCore_MutexUnlock(ctrl->lock);

    fcenter = scan_params->start_fre;
    /*先进行正常符号率的盲扫*/
    do
    {
        i = 0;
        if(EUTEL_BLIND_SCAN_STOP == ctrl->status)
        {
            tp_num = -1;
            goto end;
        }
        params.fcenter = fcenter;
        params.lpf_bw_window = BLIND_SCAN_WINDOW_SIZE_K;
        if(ioctl(class->module_frontend, FE_SET_BLINDSCAN, &params) < 0)
            goto end;

        if(params.tp_num != 0)
        {
            tp_params = GxCore_Calloc(params.tp_num, sizeof(struct dvb_frontend_parameters));
            if(tp_params == NULL)
            {
                tp_num = -1;
                goto end;
            }
            get_params.params = tp_params;
            get_params.tp_num = params.tp_num;
            if(ioctl(class->module_frontend, FE_GET_BLINDSCAN, &get_params) < 0)
            {
                tp_num = -1;
                goto end;
            }
            for(i=0; i<params.tp_num; i++)
            {
                if(tp_num+i >= EUTEL_BLIND_SCAN_TP_MAX_COUNT)
                    goto end;

                scan_params->tp[i+tp_num].fre = tp_params[i].frequency;
                scan_params->tp[i+tp_num].symbol_rate = tp_params[i].u.dvbs_param.symbol_rate;
            }
        }
        tp_num += i;
        fcenter += BLIND_SCAN_STEP_MHZ;

        EUTEL_BLIND_SCAN_FREE(tp_params);

        ++scan_count;
        GxCore_MutexLock(ctrl->lock);
        ctrl->scan_percent = base_percent + unit_percent * scan_count / scan_total;
        GxCore_MutexUnlock(ctrl->lock);
    }while(fcenter <= scan_params->end_fre + BLIND_SCAN_STEP_MHZ);

end:
    EUTEL_BLIND_SCAN_FREE(tp_params);
    return tp_num;
}

static int32_t _eutel_blind_scan_sort_tp_by_fre(const void *p1, const void *p2)
{
    int32_t fre1 = 0;
    int32_t fre2 = 0;
    fre1 = ((EutelBlindScanTp*)p1)->fre;
    fre2 = ((EutelBlindScanTp*)p2)->fre;
    return fre1 - fre2;
}

static void _eutel_blind_scan_downlink_frequency_get(struct dvb_frontend_parameters *params, EutelBlindScanParams *scan_params, int32_t i)
{
    uint32_t if_frequency = 0;
    uint32_t local_frequency = 0;
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    if_frequency = (params->frequency*1000 + 500)/1000;
    if(scan_params->lnb_type == BLIND_SCAN_LNB_C)
    {
        if(scan_params->lnb_ocs == BLIND_SCAN_LNB_OCS)//双本振
        {
            if(scan_params->tp_polar == GXBUS_PM_TP_POLAR_H)
                local_frequency = scan_params->local_fre1;
            else
                local_frequency = scan_params->local_fre2;
        }
        else
        {
            local_frequency = scan_params->local_fre1;
        }

        if(local_frequency > if_frequency)
            scan_params->tp[i].fre = local_frequency - if_frequency;
        else
            scan_params->tp[i].fre = local_frequency + if_frequency;
    }
    else
    {
        if(scan_params->lnb_ocs == BLIND_SCAN_LNB_OCS)
        {
            if(scan_params->switch_22k == GXBUS_PM_SAT_22K_ON)//22k on
            {
                if(scan_params->local_fre1 > scan_params->local_fre2)
                    local_frequency = scan_params->local_fre1;
                else
                    local_frequency = scan_params->local_fre2;
            }
            else
            {
                if(scan_params->local_fre1 > scan_params->local_fre2)
                    local_frequency = scan_params->local_fre2;
                else
                    local_frequency = scan_params->local_fre1;
            }
        }
        else
        {
            local_frequency = scan_params->local_fre1;
        }

        scan_params->tp[i].fre = local_frequency + if_frequency;
    }
    scan_params->tp[i].symbol_rate = params->u.dvbs_param.symbol_rate;
    GxCore_MutexLock(ctrl->lock);
    if(ctrl->tp_num < EUTEL_BLIND_SCAN_TP_MAX_COUNT)
    {
        ctrl->tp_array[ctrl->tp_num].symbol_rate = scan_params->tp[i].symbol_rate;
        ctrl->tp_array[ctrl->tp_num].frequency = scan_params->tp[i].fre;
        ctrl->tp_array[ctrl->tp_num].polar = scan_params->tp_polar;
        ctrl->tp_num++;
    }
    GxCore_MutexUnlock(ctrl->lock);
    return;
}

static void _eutel_blind_scan_tp_info_get(EutelBlindScanParams *scan_params)
{
    int32_t i = 0;
    unsigned long tick = 0;
    int base_percent = 0, unit_percent = 0;
    struct dvb_frontend_parameters params = {0};
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    GxCore_MutexLock(ctrl->lock);
    base_percent = 100 * ctrl->scan_count / ctrl->scan_total + (100 * 4) / (ctrl->scan_total * 10);
    unit_percent = (100 * 6) / (ctrl->scan_total * 10);
    GxCore_MutexUnlock(ctrl->lock);

    ioctl(class->module_frontend, FE_SET_FRONTEND_TUNE_MODE, 1);

    for(i = 0; i < scan_params->tp_count; i++)
    {
        fe_status_t tp_lock_status = 0;

        GxCore_MutexLock(ctrl->lock);
        ctrl->scan_percent = base_percent + unit_percent * i / scan_params->tp_count;
        GxCore_MutexUnlock(ctrl->lock);

        if(EUTEL_BLIND_SCAN_STOP == ctrl->status)
            break;

        params.frequency                = scan_params->tp[i].fre;
        params.u.dvbs_param.symbol_rate = scan_params->tp[i].symbol_rate;
        params.u.dvbs_param.fec_inner   = FEC_AUTO;

        if(ioctl(class->module_frontend, FE_SET_FRONTEND, &params) < 0)
            continue;

        tick = GxCore_TickStart(2000);
        do
        {
            if(ioctl(class->module_frontend, FE_READ_STATUS, &tp_lock_status) >= 0)
            {
                if(FE_HAS_LOCK == tp_lock_status || FE_HAS_CARRIER == tp_lock_status)
                    break;
            }
            else
            {
                break;
            }
            GxCore_ThreadDelay(10);
        }
        while(!GxCore_TickEnd(tick) && ctrl->status != EUTEL_BLIND_SCAN_STOP);

        if(FE_HAS_LOCK == tp_lock_status)
        {
            if(ioctl(class->module_frontend, FE_GET_FRONTEND, &params) >= 0)
                _eutel_blind_scan_downlink_frequency_get(&params, scan_params, i);
        }
        else
        {
            params.frequency = params.frequency/1000;
            _eutel_blind_scan_downlink_frequency_get(&params, scan_params, i);
        }
    }

    ioctl(class->module_frontend, FE_SET_FRONTEND_TUNE_MODE, 0);
    return;
}

static int32_t _eutel_blind_scan_tp(EutelBlindScanParams *scan_params)
{
    int32_t tp_num = 0;
    int32_t fre_temp = 0;
    int32_t symbol_temp = 0;
    uint32_t i = 0, j = 0;
    uint32_t sym_index = 4;
    EutelBlindScanTp *tp_temp = NULL;
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    tp_num = _eutel_blind_scan_window(scan_params);
    if(tp_num <= 0)
        return tp_num;

    tp_temp = GxCore_Calloc(EUTEL_BLIND_SCAN_TP_MAX_COUNT, sizeof(EutelBlindScanTp));
    if(NULL == tp_temp)
    {
        app_log_error("\033[31m[%s, %d] malloc failed\033[0m\n", __func__, __LINE__);
        return 0;
    }

    //sort by fre
    qsort(scan_params->tp, tp_num, sizeof(EutelBlindScanTp), _eutel_blind_scan_sort_tp_by_fre);

    //delete the semblable tp
    for(i=0; i < tp_num; i++)
    {
        if(EUTEL_BLIND_SCAN_STOP == ctrl->status)
        {
            EUTEL_BLIND_SCAN_FREE(tp_temp);
            return 0;
        }
        if(scan_params->tp[i].fre < 945000||
                scan_params->tp[i].fre > 2155000||
                scan_params->tp[i].symbol_rate < 800||
                scan_params->tp[i].symbol_rate > 50000)
        {
            continue;
        }
        if(i != (tp_num-1))
        {
            fre_temp = scan_params->tp[i+1].fre - scan_params->tp[i].fre;
            symbol_temp = scan_params->tp[i+1].symbol_rate - scan_params->tp[i].symbol_rate;

            if(symbol_temp < 0)
                symbol_temp = scan_params->tp[i].symbol_rate - scan_params->tp[i+1].symbol_rate;

            // above 40M symbole, need to record, check it is ok or not after "get_tpinfo"
            if((scan_params->tp[i].symbol_rate <= 40000) && (((fre_temp <= scan_params->tp[i].symbol_rate/sym_index)&&
                            (fre_temp <= scan_params->tp[i+1].symbol_rate/sym_index)&&
                            (symbol_temp < scan_params->tp[i].symbol_rate/32)&&
                            (symbol_temp < scan_params->tp[i+1].symbol_rate/32))||
                        fre_temp <= 500))
            {
                continue;
            }
        }
        memcpy(&tp_temp[j], &(scan_params->tp[i]), sizeof(EutelBlindScanTp));
        j++;
    }

    memset(scan_params->tp, 0, EUTEL_BLIND_SCAN_TP_MAX_COUNT * sizeof(EutelBlindScanTp));
    memcpy(scan_params->tp, tp_temp, sizeof(EutelBlindScanTp)*j);
    scan_params->tp_count = j;
    EUTEL_BLIND_SCAN_FREE(tp_temp);
    return j;
}

static int32_t _eutel_blind_scan_start_step2(uint8_t polar, uint8_t sat22k, EutelBlindScanParams *scan_params)
{
    int32_t ret = 0;
    int32_t sat22k_temp = 0;
    int32_t polar_temp = 0;
    uint32_t sat_id = 0;
    GxBusPmDataSat sat = {0};
    EutelBlindScanClass *class = &thiz_EutelBlindScanClass;

    sat_id = class->cur_sat_id;
    if(GXCORE_ERROR == GxBus_PmSatGetById(sat_id, &sat))
    {
        app_log_error("\033[31m[%s, %d] get sat params failed\033[0m\n", __func__, __LINE__);
        return -1;
    }

    switch(polar)
    {
        case BLIND_SCAN_POLAR_V:
            scan_params->tp_polar = GXBUS_PM_TP_POLAR_V;
            break;

        case BLIND_SCAN_POLAR_H:
            scan_params->tp_polar = GXBUS_PM_TP_POLAR_H;
            break;

        default:
            break;
    }

    if(GXBUS_PM_SAT_LNB_POWER_OFF != sat.sat_s.lnb_power)
    {
        if(sat22k == GXBUS_PM_SAT_22K_ON)
            sat22k_temp = SEC_TONE_ON;
        else
            sat22k_temp = SEC_TONE_OFF;

        if(ioctl(class->module_frontend, FE_SET_TONE, sat22k_temp) < 0)
            return -1;

        if(BLIND_SCAN_POLAR_H == polar)
            polar_temp = SEC_VOLTAGE_18;
        else
            polar_temp = SEC_VOLTAGE_13;

        if(ioctl(class->module_frontend, FE_SET_VOLTAGE, polar_temp) < 0)
            return -1;

        GxFrontendDiseqc para = {0};
        GxFrontendDiseqcParameters diseqc;

        para.tuner = sat.tuner;

        diseqc.type = DiSEQC10;
        diseqc.u.params_10.bPolar = polar_temp;
        diseqc.u.params_10.b22k = sat22k_temp;
        diseqc.u.params_10.chDiseqc = sat.sat_s.diseqc10;
        para.diseqc = diseqc;
        GxFrontend_SetDiseqc(&para);

        diseqc.type = DiSEQC11;
        diseqc.u.params_11.b22k = sat22k_temp;
        diseqc.u.params_11.bPolar = polar_temp;
        diseqc.u.params_11.chCom = sat.sat_s.diseqc10;
        diseqc.u.params_11.chUncom = sat.sat_s.diseqc11;
        para.diseqc = diseqc;
        GxFrontend_SetDiseqc(&para);
        GxCore_ThreadDelay(800);
    }

    if((ret = _eutel_blind_scan_tp(scan_params)) > 0)
        _eutel_blind_scan_tp_info_get(scan_params);

    return ret;
}

static int32_t _eutel_blind_scan_start(EutelBlindScanParams *scan_params)
{
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    if(scan_params->lnb_ocs == BLIND_SCAN_LNB_SINGLE)//单本振
    {
        GxCore_MutexLock(ctrl->lock);
        ctrl->scan_total = 2;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_V)//盲扫h
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_H, scan_params->switch_22k, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_H)//盲扫v
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_V, scan_params->switch_22k, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);
    }
    else if(BLIND_SCAN_LNB_OCS == scan_params->lnb_ocs && BLIND_SCAN_LNB_C == scan_params->lnb_type)//c 波段双本振
    {
        GxCore_MutexLock(ctrl->lock);
        ctrl->scan_total = 2;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_V)//盲扫h
        {
            scan_params->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
            scan_params->end_fre = ((BLIND_SCAN_LOW_FREQ_MHZ + BLIND_SCAN_HIGH_FREQ_MHZ)/2-50);
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_H, scan_params->switch_22k, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_H)//盲扫v
        {
            scan_params->start_fre = (BLIND_SCAN_LOW_FREQ_MHZ + BLIND_SCAN_HIGH_FREQ_MHZ)/2;
            scan_params->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_V, scan_params->switch_22k, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);
    }
    else if(BLIND_SCAN_LNB_OCS == scan_params->lnb_ocs && BLIND_SCAN_LNB_KU == scan_params->lnb_type)//KU 波段双本振
    {
        GxCore_MutexLock(ctrl->lock);
        ctrl->scan_total = 4;
        GxCore_MutexUnlock(ctrl->lock);

        scan_params->start_fre = 1100;
        scan_params->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
        scan_params->switch_22k = GXBUS_PM_SAT_22K_ON;
        if(scan_params->polar != BLIND_SCAN_POLAR_V)//盲扫low h
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_H, GXBUS_PM_SAT_22K_ON, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_H)//盲扫low v
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_V, GXBUS_PM_SAT_22K_ON, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);

        scan_params->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
        scan_params->end_fre = 1950;
        scan_params->switch_22k = GXBUS_PM_SAT_22K_OFF;
        if(scan_params->polar != BLIND_SCAN_POLAR_V)//盲扫high h
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_H, GXBUS_PM_SAT_22K_OFF, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);

        if(scan_params->polar != BLIND_SCAN_POLAR_H)//盲扫high v
        {
            if(_eutel_blind_scan_start_step2(BLIND_SCAN_POLAR_V, GXBUS_PM_SAT_22K_OFF, scan_params) < 0)
                goto err;
        }
        GxCore_MutexLock(ctrl->lock);
        ++ctrl->scan_count;
        GxCore_MutexUnlock(ctrl->lock);
    }
    else
    {
        app_log_error("\033[31merror blind scan type\0330m\n");
        return -1;
    }

    return 0;
err:
    return -1;
}

static void _eutel_blind_scan_loop(void *arg)
{
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;
    EutelBlindScanParams scan_params = {0};

    GxCore_ThreadDetach();
    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, true);

    scan_params.tp = GxCore_Calloc(EUTEL_BLIND_SCAN_TP_MAX_COUNT, sizeof(EutelBlindScanTp));;
    if(NULL == scan_params.tp)
    {
        app_log_error("\033[31m[%s, %d] malloc failed\033[0m\n", __func__, __LINE__);
        goto end;
    }

    while(1)
    {
        if(_eutel_blind_scan_params_get(&scan_params) < 0)
            break;

        if(EUTEL_BLIND_SCAN_STOP == ctrl->status)
            break;

        if(_eutel_blind_scan_start(&scan_params) < 0)
            break;
    }

end:
    EUTEL_BLIND_SCAN_FREE(scan_params.tp);
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);

    GxCore_MutexLock(ctrl->lock);
    ctrl->thread = GXCORE_INVALID_POINTER;
    ctrl->status = EUTEL_BLIND_SCAN_FINISH;
    GxCore_MutexUnlock(ctrl->lock);
    return;
}

int32_t app_eutel_blind_scan_start(EutelBlindScan *params)
{
    int32_t ret = 0;
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    ASSERT(params != NULL);
    ASSERT(params->sat_id != NULL);
    ASSERT(params->sat_num != 0);

    if(GXCORE_INVALID_POINTER == ctrl->lock)
        GxCore_MutexCreate(&ctrl->lock);

    GxCore_MutexLock(ctrl->lock);
    if(GXCORE_INVALID_POINTER == ctrl->thread)
    {
        ctrl->scan_total = 0;
        ctrl->scan_count = 0;
        ctrl->scan_percent = 0;
        ctrl->tp_num = 0;
        ctrl->status = EUTEL_BLIND_SCAN_START;
        memset(ctrl->tp_array, 0, EUTEL_BLIND_SCAN_TP_MAX_COUNT * sizeof(RichepgTsTP));
        _eutel_blind_scan_class_init(params);
        ret = GxCore_ThreadCreate("FTA EPG Blind Scan", &ctrl->thread, _eutel_blind_scan_loop , NULL, 100 * 1024, GXOS_DEFAULT_PRIORITY);
        if(GXCORE_ERROR == ret)
        {
            app_log_error("\033[31mcreate blind scan thread failed\033[0m\n");
            goto err;
        }
    }
    else
    {
        app_log_error("\033[31mblind scan thread doesn't exit\033[0m\n");
    }
    GxCore_MutexUnlock(ctrl->lock);
    return 0;
err:
    ctrl->status = EUTEL_BLIND_SCAN_STOP;
    _eutel_blind_scan_class_destory();
    GxCore_MutexUnlock(ctrl->lock);
    return -1;
}

int32_t app_eutel_blind_scan_stop(void)
{
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    if(GXCORE_INVALID_POINTER == ctrl->lock)
        GxCore_MutexCreate(&ctrl->lock);

    GxCore_MutexLock(ctrl->lock);
    ctrl->status = EUTEL_BLIND_SCAN_STOP;
    GxCore_MutexUnlock(ctrl->lock);
    return 0;
}

int32_t app_eutel_blind_scan_check(RichepgTsTP **tp_array, int *tp_num, int *percent)
{
    EutelBlindScanCtrl *ctrl = &thiz_EutelBlindScanCtrl;

    ASSERT(tp_array != NULL);
    ASSERT(tp_num != NULL);

    if(GXCORE_INVALID_POINTER == ctrl->lock)
        GxCore_MutexCreate(&ctrl->lock);

    GxCore_MutexLock(ctrl->lock);
    if(ctrl->status != EUTEL_BLIND_SCAN_FINISH || 0 == ctrl->tp_num)
        goto err;

    *tp_array = GxCore_Calloc(ctrl->tp_num, sizeof(RichepgTsTP));
    if(NULL == *tp_array)
    {
        app_log_error("\033[31m[%s, %d] malloc failed\033[0m\n", __func__, __LINE__);
        goto err;
    }

    *tp_num = ctrl->tp_num;
    memcpy(*tp_array, ctrl->tp_array, ctrl->tp_num * sizeof(RichepgTsTP));
    if(percent)
    {
        *percent = 100;
    }
    GxCore_MutexUnlock(ctrl->lock);
    return 0;
err:
    if(percent)
    {
        *percent = ctrl->scan_percent;
        if(*percent > 99)
            *percent = 99;
    }
    GxCore_MutexUnlock(ctrl->lock);
    return -1;
}
#endif
