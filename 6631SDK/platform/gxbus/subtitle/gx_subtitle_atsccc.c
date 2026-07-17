#include "gx_subtitle_atsccc.h"
#include "gx_atsc_cc.h"
#include "gx_avflow.h"

extern int32_t GxAtsccc_GetServiceInfo(GX_SUBTITLE_ATSCCC_INFO *info);
extern int32_t GxAtsccc_SetCurService(GX_SUBTITLE_ATSCCC_SERVICE service);

int GxSubtitle_ATSCGetServiceInfo(GX_SUBTITLE_ATSCCC_INFO *info)
{
	av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
	return GxAtsccc_GetServiceInfo(info);
}

int GxSubtitle_ATSCSwitchService(GX_SUBTITLE_ATSCCC_SERVICE service)
{
	av_flow_log("[Flow] - [%s %d]: %d\n", __func__, __LINE__, service);
	return GxAtsccc_SetCurService(service);
}
