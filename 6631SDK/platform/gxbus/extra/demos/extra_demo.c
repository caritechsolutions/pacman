#include <string.h>

#include "gxmsg.h"
#include "gxbus.h"
#include "gxcore.h"
#include "gxservices.h"
#include "gxavdev.h"
#include "av/gxav_frontend_propertytypes.h"

static handle_t device_handle;
static handle_t module_frontend;

static status_t init_frontend(void)
{
	GxFrontendProperty_Info info;	
	status_t ret;	

	device_handle = GxAvdev_CreateDevice(0);
	if (device_handle < 0)
	{
		return GXCORE_ERROR;
	}

	//frontend	
	module_frontend = GxAVOpenModule(device_handle, GXAV_MOD_FRONTEND, 0);	
	if(module_frontend < 0)	{
		return GXCORE_ERROR;
	}
	
	ret = GxAVGetProperty(device_handle, module_frontend, GxFrontendPropertyID_Info, &info,sizeof(GxFrontendProperty_Info));	
	if(ret < 0)	 {	
		return GXCORE_ERROR;
	}

	if (info.fe == FE_QAM)
	{
		gxlogd("frontend is cable\n");
	}

	return GXCORE_SUCCESS;
}

void cable_set_tp(uint32_t fre, uint32_t symb)
{
	GxFrontendProperty_Parameters params;	
	status_t ret;	
	
	//DVB-C    
	params.frequency = fre*1000*1000;        
	params.inversion = INVERSION_OFF;        
	params.u.dvbc_param.modulation = QAM_64;        
	params.u.dvbc_param.symbol_rate = symb;		

	ret = GxAVSetProperty(device_handle, module_frontend, GxFrontendPropertyID_FrontendSet, &params, sizeof(GxFrontendProperty_Parameters));	
	if(ret < 0)
	{
		return;
	}
}

static void lock_ts(void)
{
	int32_t i;

	for(i=0; i<20; i++)
		{
	if (GxBus_SiTsLockQuery(0) == SI_TS_LOCK)
	{
		gxlogd("ts lock!\n");
	}
	else
	{
		gxlogd("ts unlock!\n");
	}
		}
}

int GxCore_Startup(int argc, char **argv)
{
	status_t rel = GXCORE_ERROR;

	GxServiceClass serv_list[2] = {
		si_service,
		extra_service
	};
	
	rel = GxBus_Init(serv_list, 2);
	if (rel != GXCORE_SUCCESS) {
		return 0;
	}

	init_frontend();
	cable_set_tp(355,6900);
	lock_ts();
    	GxCore_Loop();

	return 0;
}

 
