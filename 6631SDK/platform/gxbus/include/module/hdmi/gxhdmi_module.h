#ifndef __GX_HDMI__MODULE_H__
#define __GX_HDMI__MODULE_H__

#include "gxcore.h"
#include "av/avapi.h"

__BEGIN_DECLS

#define HDMI_CONFIG_VOUT_HDCP_ON         "Hdmi>vout_hdcp_on"
#define HDMI_CONFIG_VOUT_HDCP_MODE       "Hdmi>vout_hdcp_mode"

typedef struct {
	unsigned int hdmi_major;
	unsigned int hdmi_minor;
	unsigned int hdmi_revision;
	unsigned int hdcp_major;
	unsigned int hdcp_minor;
	unsigned int hdcp_revision;
}GxHdmi_VersionInfo;

typedef enum
{
	GX_HDMI_RGB_OUT = 1,
	GX_HDMI_YUV_422 = 2,
	GX_HDMI_YUV_444 = 4,
} GxHdmiEncodeOut;

typedef enum
{
	GX_HDMI_CEC_OP_STANDBY       = 1,
	GX_HDMI_CEC_OP_IMAGE_VIEW_ON = 2,
} GxHdmiCecCmd;

__END_DECLS

#endif
