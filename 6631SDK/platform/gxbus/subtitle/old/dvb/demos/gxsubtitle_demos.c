/**
 *
 * @file        subtitle_demos.c
 * @brief
 * @version     1.1.0
 * @date        04/06/2010 03:14:08 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gxcore.h"
#include "gxavdev.h"
#include "gxsubtitle.h"




void set_tuner(void)
{
    int32_t                         ret;
    int32_t                         dev;
    handle_t                        front_end;
    GxFrontendProperty_Parameters   param;
    GxFrontendProperty_Info         info;

    dev = GxAvdev_CreateDevice(0);
    if (dev < 0) {
        gxlogd("[subtitle] GxAvdev_CreateDevice Failure\n");
    }

    front_end = GxAVOpenModule(dev, GXAV_MOD_FRONTEND, 0);
    if (front_end < 0) {
        gxlogd("[subtitle] GxAVOpenModule Failure, GXAV_MOD_FRONTEND\n");
    }

    ret = GxAVGetProperty(dev, front_end,
    	    GxFrontendPropertyID_Info, &info,
    	    sizeof(GxFrontendProperty_Info));
    if (ret < 0) {
        gxlogd("[subtitle] GxAVGetProperty Failure, GxFrontendPropertyID_Info\n");
    }
    gxlogd("[subtitle] Tuner type=%d\n", info.tuner);
    param.frequency = 802000000;
    //param.frequency = 299000000;
    param.inversion = INVERSION_OFF;
    param.u.qam.symbol_rate = 6900;
    param.u.qam.modulation = QAM_64;

    ret = GxAVSetProperty(dev, front_end,
    	    GxFrontendPropertyID_FrontendSet, &param,
    	    sizeof(GxFrontendProperty_Parameters));
    if (ret < 0) {
        gxlogd("[subtitle] GxAVGetProperty Failure, GxFrontendPropertyID_FrontendSet\n");
    }
}


static void gxsubitile_demo_thread(void* arg)
{
    handle_t          handle;
    GxSubtitle_Param  param;
    GxSubtitle_Stream stream = {0x38a, 0x2, 0x2};//{0x390,2, 3};//TS3
    #if 1
    set_tuner();

    param.demux_id = 0;
    param.render_type = 0;

    gxlogd("[subtitle] start\n");
    GxCore_ThreadDelay(10000);
    gxlogd("[subtitle] keep going\n");

    #else
    rect.x = 0;
    rect.y = 0;
    rect.width = 720;
    rect.height = 576;
    GxSubtitle_DisplayInit();
    f = GxSubtitle_BitmapCreate(&rect);
    GxSubtitle_BitmapDisplay(f, TRUE);
    #endif

    while(1) {
        #if 0
        if (dmx_lock_query(0) == 1) {
            gxlogd("[subtitle] TS Lock\n");
        } else {
            gxlogd("[subtitle] TS Unlock\n");
        }
        #endif

        handle = GxSubtitle_Open(&param);
        GxSubtitle_StreamSet(handle, &stream);
        GxCore_ThreadDelay(30000);

        GxSubtitle_Hide(handle);
        GxCore_ThreadDelay(600);
        GxSubtitle_Show(handle);
        GxCore_ThreadDelay(600);
        GxSubtitle_Stop(handle);
        GxCore_ThreadDelay(600);
        GxSubtitle_Start(handle);
        GxCore_ThreadDelay(600);
        GxSubtitle_Close(handle);
    }
}

int GxCore_Startup(int argc, char **argv)
{
    handle_t thread;

    GxCore_ThreadCreate("test", &thread, gxsubitile_demo_thread, NULL, 100*1024, GXOS_DEFAULT_PRIORITY);

    GxCore_Loop();

    return 0;
}
