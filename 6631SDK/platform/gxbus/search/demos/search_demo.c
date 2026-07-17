#include <string.h>
#include <gxcore.h>
#include "gxmsg.h"
#include "gxbus.h"
#include "service/gxsi.h"



extern status_t GxBus_SiParserBufCreate(void);

extern uint32_t CheckTSLock(void);

int GxCore_Startup(int argc, char **argv)
{

	gxlogd("search start!\n");


	//GxBusPmDataSat sat;
	//GxBusPmDataTP tp;
	status_t rel = GXCORE_ERROR;
	//handle_t self;
	GxMessage *msg;
//	GxMsgProperty_EpgCreate epg_cfg;
	GxMsgProperty_EpgCreate* cfg_params = NULL;
//	GxMsgProperty_EpgGet epg;
	GxMsgProperty_EpgGet* epg_params = NULL;
	static uint32_t i;
	GxServiceClass service_list[2] = {{"search_servie",GxSearchInit,GxSearchDestroy,GxSearchServiceRecvMsg,NULL},
									  {"si_service",GxSiServiceInit,GxSiServiceDestroy,GxSiServiceRecvMsg,GxSiServiceConsole}}; 
	uint32_t service_num = 2;

	//sqlite search
	//GxBus_PmDbaseInit();
	
	//CheckTSLock();
	
	/*sat.type = GXBUS_PM_SAT_S;
	sat.tuner = 1;
	sat.sat_s.diseqc10 = 1;
	sat.sat_s.diseqc11 = 2;
	sat.sat_s.diseqc12_pos = 3;
	sat.sat_s.diseqc_version = 4;
	sat.sat_s.lnb1 =5150;
	sat.sat_s.lnb2 = 5750;
	sat.sat_s.lnb_power = 0;
	sat.sat_s.lnb_type = GXBUS_PM_SAT_LNB_UNIVERSAL_1;
	sat.sat_s.longitude = 180;
	sat.sat_s.longitude_direct = 0;
	memcpy(sat.sat_s.sat_name,"sat111",MAX_SAT_NAME);
	sat.sat_s.switch_12V = 1;
	sat.sat_s.switch_22K =0;
	GxBus_PmSatAdd(&sat);

	tp.sat_id = 1;
	tp.frequency = 3000;
	tp.tp_s.symbol_rate = 27500;
	tp.tp_s.polar = 0;
	GxBus_PmTpAdd(&tp);*/
	
	
	//GxCore_Init();
	
//	if (GxBus_SiParserBufCreate() == GXCORE_ERROR)
//	{
//		gxlogd("init mempool failed!\n");
//	}
//	i2c_init(108000000,(100 * (1000)),0x20070000,0);

	//si_filter_demux_open(0);
	rel = GxBus_Init(service_list,2);
	
#if 1

	//self = GxBus_ServiceFindByName("search_service");

	//si_service = GxBus_ServiceCreate(service_list[SERVICE_ID_SI]);
	//search_service = GxBus_ServiceCreate(service_list[SERVICE_ID_SEARCH]);

	//#ifdef GX_BUS_SEARCH_DBUG
	GxMessage *new_msg = NULL;
	GxMsgProperty_ScanSatStart sat_search={0};
	GxMsgProperty_ScanSatStart* params = NULL;
	uint32_t arry[1];
	arry[0] = 1;
	sat_search.scan_type = GX_SEARCH_MANUAL;
	sat_search.tv_radio = GX_SEARCH_ALL;
	sat_search.fta_cas = GX_SEARCH_ALL;
	sat_search.nit_switch = GX_SEARCH_NIT_DISABLE;
	sat_search.params_s.max_num = 1;
	sat_search.params_s.array = arry;

	new_msg = GxBus_MessageNew(GXMSG_SEARCH_SCAN_SAT_START);
	
	params = (GxMsgProperty_ScanSatStart*)GxBus_GetMsgPropertyPtr(new_msg,GxMsgProperty_ScanSatStart);
	memcpy(params,&sat_search,sizeof(GxMsgProperty_ScanSatStart));

	GxBus_MessageSend(new_msg);
	//#endif
	//while(1);
	//gxlogd("send GXMSG_SEARCH_SCAN_SAT_START!\n");

	//return;


#endif

	GxCore_Loop();

	return 0;
}

