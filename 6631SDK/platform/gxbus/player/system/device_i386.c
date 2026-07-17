
#include "gx_system.h"
#include "gx_device.h"

#ifdef I386_LINUX

GxDevice gx_device_i386 = {
	.priv = &gx_device_i386 ,
	.chip = 0,
	
	.init = NULL ,
	.uninit = NULL ,
	.set = NULL ,
	.get = NULL ,
};

#endif


