#include <string.h>

#include "gxmsg.h"
#include "gxbus.h"
#include "gxcore.h"
#include "gxservices.h"

int GxCore_Startup(int argc, char **argv)
{
	status_t rel = GXCORE_ERROR;

	GxServiceOps serv_list[1] = {
		book_service
	};
	
	rel = GxBus_Init(serv_list, 1);
	if (rel != GXCORE_SUCCESS) {
		return 0;
	}

    	GxCore_Loop();

	return 0;
}

 
