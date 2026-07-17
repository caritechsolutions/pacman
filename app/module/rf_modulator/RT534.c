#include "app.h"
#include "app_module.h"
#include "module/app_gpio.h"
#include "RT534.h"

#if ((RF_MODULATOR_SUPPORT > 0) && (RF_RT534 > 0))


void RT534_RF_ON_OFF(int  tmp)
{
    AppFrontend_SetTp params = {0};
    GxFrontend sProg_Tp = {0};
    GpioData _gpio;
    handle_t gpio_handle = app_get_gpio_handle();

    _gpio.port = V_GPIO_RF_ON_OFF;
    ioctl(gpio_handle, GPIO_OUTPUT, &_gpio);
    if (tmp == 0) //RF_ON
        ioctl(gpio_handle, GPIO_HIGH, &_gpio);
    else
        ioctl(gpio_handle, GPIO_LOW, &_gpio);

    GxFrontend_GetCurFre(0, &sProg_Tp);
    if (sProg_Tp.fre != 0)
    {
        params.fre = sProg_Tp.fre;
        params.bandwidth = sProg_Tp.bandwidth;
        app_log_debug("===1===fre:%d======bandwidth:%d========\n", sProg_Tp.fre, sProg_Tp.bandwidth);
    }
    else
    {
        params.fre = 474000000;
        params.bandwidth = 0;//8M
        app_log_debug("===2===fre:%d======bandwidth:%d========\n", sProg_Tp.fre, sProg_Tp.bandwidth);
    }

    params.type = FRONTEND_DVB_T2;
    params.data_plp_id = 0xff;
    params.common_plp_exist = 0xff;
    params.common_plp_id = 0xff;
    params.type_1501 = 0;

    GxFrontend_CleanRecordPara(0);
    app_set_tp(0, &params, 1);//SET_TP_FORCE
    app_log_debug("=========T2_RF_ON_OFF_KEY:%d==========\n", tmp);
}

void RT534_RF_CH3_CH4(int tmp)
{
    GpioData _gpio;
    handle_t gpio_handle = app_get_gpio_handle();

    _gpio.port = V_GPIO_RF_CH3_CH4;
    ioctl(gpio_handle, GPIO_OUTPUT, &_gpio);
    if (tmp == 0) //RF_CH3
        ioctl(gpio_handle, GPIO_HIGH, &_gpio);
    else
        ioctl(gpio_handle, GPIO_LOW, &_gpio);
    app_log_debug("=========T2_RF_MODE_KEY:%d==========\n", tmp);
}
#endif
