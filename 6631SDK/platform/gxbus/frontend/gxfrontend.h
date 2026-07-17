#ifndef __GXFRONTEND_H__
#define __GXFRONTEND_H__

#include "module/frontend/gxfrontend_module.h"

typedef struct
{
	int32_t sat22k;
	int32_t flag;
}GxFrontend_22k;

typedef struct
{
	fe_sec_voltage_t polar;
	int32_t  flag;
}GxFrontend_Polar;

typedef struct
{
	GxFrontendDiseqc para;
	int32_t flag;
}GxFrontend_Dis;

typedef struct
{
	struct dvb_diseqc_master_cmd cmd;
	int32_t flag;
}GxFrontend_Unicable;

typedef struct {
	int32_t           dev;
	handle_t          demux;
	handle_t          frontend;
	handle_t          hSocket;

	int32_t           tuner;
	int32_t           fre;
	int32_t           symb;
	int32_t           sat22k;
	uint32_t          ip;
	uint32_t          port;
	fe_modulation_t   qam;
	fe_sec_voltage_t  polar;
	fe_bandwidth_t    bandwidth;
	fe_set_tp_mode_t  set_tp_mode;
	fe_dvbc_tune_mode tune_mode;
	GxFrontendType    type;
	int32_t           type_1501;//1501有两种工作模式，dtmb或者
	uint8_t           data_plp_id;
	uint8_t           common_plp_exist;
	uint8_t           common_plp_id;
	uint8_t           if_channel_index;
	uint16_t          lnb_index;
	uint16_t          centre_fre;
	int32_t           if_fre;
	uint32_t          pls_n;
	uint8_t           pls_ts_id;
	uint8_t           pls_code;
	uint8_t           pls_chanage;
	uint8_t           pls_ts_status;
	char             *url;
}GxFrontend_Params;

typedef struct {
	GxFrontend_Polar   polar[FRONTEND_MAX];
	GxFrontend_22k     tone[FRONTEND_MAX];
	GxFrontend_Dis     diseqc10[FRONTEND_MAX];
	GxFrontend_Dis     diseqc11[FRONTEND_MAX];
	GxFrontend_Dis     diseqc12[FRONTEND_MAX];
	GxFrontend_Unicable unicable[FRONTEND_MAX];
	fronted_set_status set_status[FRONTEND_MAX];
	GxFrontend_Params  params[FRONTEND_MAX];
}GxFrontend_MonitorParams;






#endif

