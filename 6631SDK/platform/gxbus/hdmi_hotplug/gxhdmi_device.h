#ifndef __GXHDMI_DEVICE_H__
#define __GXHDMI_DEVICE_H__

#include "gxavdev.h"
#include "../include/module/hdmi/gxhdmi_module.h"

typedef struct {
	int             dev;
	handle_t        vo;
	handle_t        hdmi;
	int             cec_enable;
	int             hdcp_enable;
	int             data_encrypt;
	int             mute_av_when_failed;
	GxHdmiEncodeOut encode;
}GxHdmi_Device;

extern GxHdmi_Device hdmi_device;

typedef struct get_cec_cmd_param {
	GxHdmiCecCmd *cmd;
	unsigned      timeout_ms;
}GxHdmiGetCecCmdParam;

extern int  GxHdmi_DeviceInit(void);
extern void GxHdmi_DeviceUnInit(void);
extern int  GxHdmi_DeviceSet(int id, void* arg);
extern int  GxHdmi_DeviceGet(int id, void* arg);
#endif
