#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "app_config.h"

int g_avapi_test_handle = 0;
int g_VpuModHandle = 0;
void app_osd_init_1(void)
{
    g_avapi_test_handle = GxAVCreateDevice(0);
    g_VpuModHandle = GxAVOpenModule(g_avapi_test_handle, GXAV_MOD_VPU, 0);
    return;
}

void app_osd_init_test(void)
{
    //g_avapi_test_handle = GxAVCreateDevice(0);
    //g_VpuModHandle = GxAVOpenModule(g_avapi_test_handle, GXAV_MOD_VPU,0);

    GxVpuProperty_LayerViewport Viewport;
    /*Viewport.layer = GX_LAYER_OSD;
    Viewport.clip_rect.x = 0;
    Viewport.clip_rect.y = 0;
    Viewport.clip_rect.width = 720;
    Viewport.clip_rect.height = 576;
    Viewport.view_rect.x = 100;
    Viewport.view_rect.y = 100;
    Viewport.view_rect.width = 720;
    Viewport.view_rect.height = 576;
    GxAVSetProperty(g_avapi_test_handle,VpuModHandle,GxVpuPropertyID_LayerViewport,&Viewport,sizeof(GxVpuProperty_LayerViewport));
    */

    Viewport.layer = GX_LAYER_OSD;
    Viewport.rect.x = 200;
    Viewport.rect.y = 100;
    Viewport.rect.width  = 624;
    Viewport.rect.height = 376;

    GxAVSetProperty(g_avapi_test_handle, g_VpuModHandle, \
                    GxVpuPropertyID_LayerViewport, (void *)&Viewport, sizeof(GxVpuProperty_LayerViewport));

    return;
}
