
#include "gxcore.h"
#include <gxtype.h>
#include "module/ttx/gxttx.h"
#include "gxavdev.h"
#include "av/gxav_demux_propertytypes.h"
#include "av/gxav_module_property.h"
#include "av/gxav_event_type.h"
#include "module/pm/gxpm_manage.h"

static handle_t         frontend_device_handle;
static handle_t         frontend_module_handle;

int32_t init_frontend(void)
{

	frontend_device_handle = GxAvdev_CreateDevice(0);
	if (frontend_device_handle < 0) {
		return 1;
	}
	//frontend      
	frontend_module_handle =
	    GxAvdev_OpenModule(frontend_device_handle, GXAV_MOD_FRONTEND, 0);
	if (frontend_module_handle < 0) {
		return 2;
	}


	{
	GxFrontendProperty_Tuner tuner;
	tuner.type = TUNER_TDAD3_C02A;
	GxAVSetProperty(frontend_device_handle, frontend_module_handle,GxFrontendPropertyID_SetTuner, &tuner,sizeof(GxFrontendProperty_Tuner));
	}


	return 0;
}


int32_t set_tp(GxFrontendProperty_Parameters* params)
{

	if(frontend_device_handle<0 || frontend_module_handle<0)
	{
		return 1;
	}
	return   GxAVSetProperty(frontend_device_handle, frontend_module_handle,
			    GxFrontendPropertyID_FrontendSet, params,
			    sizeof(GxFrontendProperty_Parameters));
}


int GxCore_Startup(int argc, char **argv)
{
	GxFrontendProperty_Parameters params;
	int32_t ret=0;

	init_frontend();
	GxBus_PmDbaseInit();
	
	params.frequency = 826 *1000000;
	params.u.ofdm.bandwidth = 0;
	ret = set_tp(&params);
	gxlogd("\n@@@@@set_tp ret:%d @@@@@@@@\n",ret);

	GxTtx_Init();


	

	
	GxCore_Loop();
 


	
	


	return 0;
}

