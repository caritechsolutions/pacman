#include <gxmsg.h>
#include <gxbus.h>
#include <gxcore.h>
#include <gxservices.h>


status_t APP_Init(handle_t self)
{
	return GXCORE_SUCCESS;
}
status_t APP_Destroy(handle_t self)
{
	return GXCORE_SUCCESS;
}


int GxCore_Startup(int argc, char **argv)
{
	status_t rel = GXCORE_ERROR;

	GxServiceClass serv_list[1] = 
	{
		gui_view_service
	};
	
	GxCore_Init();
	
	rel = GxBus_Init(serv_list, 1);
	if (rel != GXCORE_SUCCESS) 
	{
		return 0;
	}

    	GxCore_Loop();

	return 0;
}
