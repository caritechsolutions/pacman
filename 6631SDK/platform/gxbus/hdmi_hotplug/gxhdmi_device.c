#include "gxhdmi_device.h"
#include "../include/module/config/gxconfig.h"

#define HDMI_DEVICE_SET(type,func,arg) {\
	if(arg == NULL) \
		return -1; \
	type newarg = *(type*)arg;\
	ret = func(newarg);\
	break;\
};

#define HDMI_DEVICE_GET(type,func,arg) {\
	if(arg == NULL) \
		return -1; \
	type* newarg = arg;\
	ret = func(newarg);\
	break;\
}

GxHdmi_Device hdmi_device;

int GxHdmi_DeviceInit(void)
{
	memset(&hdmi_device, 0, sizeof(GxHdmi_Device));
	hdmi_device.dev  = GxAvdev_CreateDevice(0);
	hdmi_device.vo   = GxAvdev_OpenModule(hdmi_device.dev, GXAV_MOD_VIDEO_OUT, 0);
	hdmi_device.hdmi = GxAvdev_OpenModule(hdmi_device.dev, GXAV_MOD_HDMI, 0);

	GxBus_ConfigGetInt(HDMI_CONFIG_VOUT_HDCP_ON, &hdmi_device.hdcp_enable, 0);
	return 0;
}

void GxHdmi_DeviceUnInit(void)
{
	GxAvdev_CloseModule(hdmi_device.dev, hdmi_device.vo);
	GxAvdev_CloseModule(hdmi_device.dev, hdmi_device.hdmi);
	GxAvdev_DestroyDevice(hdmi_device.dev);
}
