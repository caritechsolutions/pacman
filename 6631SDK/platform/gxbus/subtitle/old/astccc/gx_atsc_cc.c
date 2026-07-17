#include "gx_atsc_cc.h"

extern int gx_atsc_cc_open(GxAtsccc_Param* cc_para);
extern int gx_atsc_cc_close(void);
extern int gx_atsc_cc_hide(void);
extern int gx_atsc_cc_show(void);
extern int gx_atsc_cc_parse_userdata(handle_t handle, uint8_t* data, int length);


handle_t GxAtsccc_Open(GxAtsccc_Param* param)
{
	return gx_atsc_cc_open(param);
}

int32_t GxAtsccc_StreamSet(handle_t handle, GX_SUBTITLE_ATSCCC_SERVICE id)
{
	return 0;
}

int32_t GxAtsccc_Close(handle_t handle)
{
	return gx_atsc_cc_close();
}

int32_t GxAtsccc_Start(handle_t handle)
{
	return 0;
}

int32_t GxAtsccc_Stop(handle_t handle)
{
	return 0;
}

int32_t GxAtsccc_Show(handle_t handle)
{
	return gx_atsc_cc_show();
}

int32_t GxAtsccc_Clear(handle_t handle)
{
	return 0;
}

int32_t GxAtsccc_Hide(handle_t handle)
{
	return gx_atsc_cc_hide();
}

int32_t GxAtsccc_SendData(handle_t handle, uint8_t* data, int length)
{
	return gx_atsc_cc_parse_userdata(handle,data,length);
}
