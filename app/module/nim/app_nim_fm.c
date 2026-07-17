#include "app.h"
#if ((DEMOD_DVB_T > 0)||( FM_SUPPORT> 0))
#include "app_module.h"
#include "app_rf_channels.h"
#include "app_fm.h"

extern void app_fm_config_init(uint32_t index);


static void dvbt_fm_config_get(AppNim *nim, AppFrontend_Config *cfg)
{
    cfg->tuner  = nim->tuner;
    cfg->ts_src = nim->ts_src;
    cfg->dmx_id = nim->dmx_id;
}

static status_t dvbt_fm_frontend_open(AppNim* nim, AppFrontend_Open *fopen)
{

    nim->tuner      = fopen->tuner;
    nim->ts_src     = fopen->ts_src;
    nim->dmx_id     = fopen->dmx_id;
    app_fm_config_init(fopen->tuner);

    return GXCORE_SUCCESS;
}

static status_t dvbt_fm_frontend_close(AppNim *nim)
{
    status_t  ret = GXCORE_SUCCESS;

    if(nim->fro_dev_hdl <= 0
            || nim->dmx_mod_hdl <= 0)
    {
        return GXCORE_ERROR;
    }

    nim->dmx_mod_hdl = -1;

    return ret;
}

AppNim nim_fm =
{
    .lock_state     = false,
    .open           = dvbt_fm_frontend_open,
    .close          = dvbt_fm_frontend_close,
    .config_get     = dvbt_fm_config_get,
};
#endif
