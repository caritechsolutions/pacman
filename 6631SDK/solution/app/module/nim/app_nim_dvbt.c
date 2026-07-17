#include "app.h"
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
#include "app_module.h"
#include "app_rf_channels.h"

#define SIGNAL_GRADIENT_COUNT 1

typedef struct
{
	uint8_t signal_value[SIGNAL_GRADIENT_COUNT];
	uint8_t signal_num;
}GradientSignal;

static uint32_t dvbt_gradient_quality(unsigned int value)
{
	static GradientSignal gradient_quality = {{0},0};
	unsigned int ret = 0;
	int i;

	//app_log_flow("[APP Ioctl] get quality before gradient = %d\n", value);
	gradient_quality.signal_value[gradient_quality.signal_num] = value;
	gradient_quality.signal_num++;
	gradient_quality.signal_num %= SIGNAL_GRADIENT_COUNT;

	for(i = 0; i < SIGNAL_GRADIENT_COUNT; i++)
	{
		ret += gradient_quality.signal_value[i];
	}
	ret /= SIGNAL_GRADIENT_COUNT;
    if(ret > 100)
        ret = 100;

	//app_log_flow("[APP Ioctl] get quality after gradient = %d\n", ret);
	return ret;
}

static uint32_t dvbt_gradient_strength(unsigned int value)
{
	static GradientSignal gradient_strength = {{0},0};
	unsigned int ret = 0;
	int i;

	//app_log_flow("[APP Ioctl] get signal strength = %d\n", value);
	gradient_strength.signal_value[gradient_strength.signal_num] = value;
	gradient_strength.signal_num++;
	gradient_strength.signal_num %= SIGNAL_GRADIENT_COUNT;

	for(i = 0; i < SIGNAL_GRADIENT_COUNT; i++)
	{
		ret += gradient_strength.signal_value[i];
	}
	ret /= SIGNAL_GRADIENT_COUNT;
    if(ret > 100)
        ret = 100;

	//app_log_flow("[APP Ioctl] signal strength gradient = %d\n", ret);
	return ret;
}

static void dvbt_tp_set(AppNim *nim, AppFrontend_SetTp *params)
{
    // the follow struct is the msg for bus(frontend)
    AppFrontend_SetTp frontend;
    uint32_t mode = 0;

    // copy fre, symb, polar & 22k
    memcpy(&frontend, params, sizeof(AppFrontend_SetTp));

    frontend.dev = nim->fro_dev_hdl;
    frontend.demux = nim->dmx_mod_hdl;
    frontend.tuner = nim->tuner;

    if(app_get_country_type() == COUNTRY_TYPE_INA){//for 322712
        GxFrontend_GetFrontendTuneMode(nim->tuner, &mode);
        GxFrontend_SetFrontendTuneMode(nim->tuner, mode|FE_INA_SEARCH);
    }else{
        GxFrontend_GetFrontendTuneMode(nim->tuner, &mode);
        GxFrontend_SetFrontendTuneMode(nim->tuner, mode&(~FE_INA_SEARCH));
    }
    // set tp
    app_send_msg_exec(GXMSG_FRONTEND_SET_TP, (void*)(&frontend));
}

static void dvbt_lock_state_set(AppNim* nim, AppFrontend_LockState params)
{
    nim->lock_state = params;
}

static void dvbt_lock_state_get(AppNim* nim, AppFrontend_LockState *params)
{// modify in 20120826
    if((GxFrontend_QueryFrontendStatus(nim->tuner)!=1)
            || (GxFrontend_QueryStatus(nim->tuner)!=1))
    {
        *params = FRONTEND_UNLOCK;
        //app_log_debug("\n^^^^^^^^^^^^^^^^^^^^[UNLOCK]---\n");
    }
    else
    {
        *params = FRONTEND_LOCKED;
        //app_log_debug("\n^^^^^^^^^^^^^^^^^^^^[LOCKED]---\n");
    }
}

static void dvbt_monitor_set(AppNim *nim, AppFrontend_Monitor params)
{
    GxMsgProperty_FrontendMonitor tuner = nim->tuner;

    if (params == FRONTEND_MONITOR_ON)
    {
        app_send_msg_exec(GXMSG_FRONTEND_START_MONITOR, &tuner);
    }
    else
    {
        app_send_msg_exec(GXMSG_FRONTEND_STOP_MONITOR, &tuner);
    }
}

static void dvbt_strength_get(AppNim *nim, AppFrontend_Strengh *str)
{
    *str = GxFrontend_GetStrength(nim->tuner);
    if(*str > 0xFF)
    {
        *str = CALC_SIGNAL_STRENGTH(*str);
    }
    *str = dvbt_gradient_strength(*str);
}

static void dvbt_quality_get(AppNim *nim, AppFrontend_Quality *qua)
{
    GxFrontendSignalQuality sq = {0};

    if (0 == GxFrontend_GetQuality(nim->tuner, &sq))
    {
        // the following code so ugly!!!!!!! by Blacker Liu
        // used for DVB-C
        *qua = sq.snr;

        // used for DVB-S/S2
        if(*qua > 0xFF)
        {
            *qua = sq.snr*100/0xFFFF; // since beta4
        }

        *qua= dvbt_gradient_quality(*qua);
    }
    else
    {
        *qua= 0;
    }
    //app_log_flow("[APP Ioctl] get quality = %d\n", *qua );
}

static void dvbt_errorrate_get(AppNim *nim, AppFrontend_ErrorRate *err)
{
    GxFrontendSignalQuality sq = {0};

    if (0 == GxFrontend_GetQuality(nim->tuner, &sq))
    {
        *err = sq.error_rate;
    }
    else
    {
        *err = 0;
    }
}

static void dvbt_info_get(AppNim *nim, AppFrontend_Info *info)
{
    int32_t Handle = 0;
    if((nim == NULL) || (info == NULL))
        return;
    if(-1 != (Handle = GxFrontend_IdToHandle(nim->tuner)))
        GxFrontend_GetInfo(Handle, info);
}

static void dvbt_config_get(AppNim *nim, AppFrontend_Config *cfg)
{
    cfg->tuner  = nim->tuner;
    cfg->ts_src = nim->ts_src;
    cfg->dmx_id = nim->dmx_id;
}

static status_t dvbt_frontend_open(AppNim* nim, AppFrontend_Open *fopen)
{

    GxDemuxProperty_ConfigDemux demux;

    nim->tuner      = fopen->tuner;
    nim->ts_src     = fopen->ts_src;
    nim->dmx_id     = fopen->dmx_id;

    nim->dmx_mod_hdl =
        GxAvdev_OpenModule(nim->fro_dev_hdl, GXAV_MOD_DEMUX, nim->dmx_id);
    if (nim->dmx_mod_hdl <= 0) {
        return GXCORE_ERROR;
    }

    // demux config
    demux.source = nim->ts_src;
    demux.ts_select = FRONTEND;
    demux.stream_mode = DEMUX_PARALLEL;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;
    GxAVSetProperty(nim->fro_dev_hdl, nim->dmx_mod_hdl, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));

    return GXCORE_SUCCESS;
}

static status_t dvbt_demux_config(AppNim *nim)
{
    GxDemuxProperty_ConfigDemux demux;

    // demux config
    demux.source = nim->ts_src;
    demux.ts_select = FRONTEND;
    demux.stream_mode = DEMUX_PARALLEL;
    demux.time_gate = 0xf;
    demux.byt_cnt_err_gate = 0x03;
    demux.sync_loss_gate = 0x03;
    demux.sync_lock_gate = 0x03;
    GxAVSetProperty(nim->fro_dev_hdl, nim->dmx_mod_hdl, GxDemuxPropertyID_Config, &demux, \
            sizeof(GxDemuxProperty_ConfigDemux));

    return GXCORE_SUCCESS;
}

static status_t dvbt_frontend_close(AppNim *nim)
{
    status_t  ret = GXCORE_SUCCESS;

    //frontend
    if(nim->fro_dev_hdl <= 0
            || nim->dmx_mod_hdl <= 0)
    {
        return GXCORE_ERROR;
    }

    ret = GxAVCloseModule(nim->fro_dev_hdl, nim->dmx_mod_hdl);
    nim->dmx_mod_hdl = -1;

    return ret;
}

AppNim nim_dvbt =
{
    .lock_state     = false,
    .open           = dvbt_frontend_open,
    .close          = dvbt_frontend_close,
    .tp_set         = dvbt_tp_set,
    .monitor_set    = dvbt_monitor_set,
    .lock_state_set = dvbt_lock_state_set,
    .lock_state_get = dvbt_lock_state_get,
    .strength_get   = dvbt_strength_get,
    .quality_get    = dvbt_quality_get,
    .errorrate_get  = dvbt_errorrate_get,
    .info_get       = dvbt_info_get,
    .config_get     = dvbt_config_get,
    .demux_cfg      = dvbt_demux_config,
};
#endif
