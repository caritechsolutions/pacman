#include "subt_system/dmx_subt_system/dmx_subt_system.h"



dmx_handle_t dmx;

int GxCore_Startup(int argc, char **argv)
{
    dmx = GxSubsystm_DmxOpen((uint8_t*)"dmx0",0,0);
    gxlogd("open dmx handle = %x \n",dmx);
    GxCore_Loop();

    return 0;
}
