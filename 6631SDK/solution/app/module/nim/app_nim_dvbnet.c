#include "app.h"
#if (NET_SEARCH_SUPPORT > 0)
#include "app_module.h"

static void dvbnet_lock_state_set(AppNim* nim, AppFrontend_LockState params)
{
    nim->lock_state = params;
}

static void dvbnet_lock_state_get(AppNim* nim, AppFrontend_LockState *params)
{
    if(GxFrontend_QueryFrontendStatus(nim->tuner) != 1)
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

static void dvbnet_monitor_set(AppNim *nim, AppFrontend_Monitor params)
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


static void dvbnet_config_get(AppNim *nim, AppFrontend_Config *cfg)
{
    cfg->tuner  = nim->tuner;
    cfg->ts_src = nim->ts_src;
    cfg->dmx_id = nim->dmx_id;
}

static status_t dvbnet_frontend_open(AppNim* nim, AppFrontend_Open *fopen)
{
    nim->tuner      = fopen->tuner;
    nim->ts_src     = fopen->ts_src;
    nim->dmx_id     = fopen->dmx_id;
    return GXCORE_SUCCESS;
}

static status_t dvbnet_frontend_close(AppNim *nim)
{
    return GXCORE_SUCCESS;
}

AppNim nim_dvbnet =
{
    .lock_state     = false,
    .open           = dvbnet_frontend_open,
    .close          = dvbnet_frontend_close,
    .monitor_set    = dvbnet_monitor_set,
    .lock_state_set = dvbnet_lock_state_set,
    .lock_state_get = dvbnet_lock_state_get,
    .config_get     = dvbnet_config_get,
};

#endif

