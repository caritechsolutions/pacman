#include <string.h>

#include <gxmsg.h>
#include <gxbus.h>
#include <gxcore.h>

#include <module/si/si_parser.h>



extern uint32_t CheckTSLock(void);




// msg_id    1   GXMSG_EPG_CREATE    
//                     2     GXMSG_EPG_GET
void epg_test_msg(uint32_t msg_id)
{
	GxMessage *msg;
//	GxMsgProperty_EpgCreate epg_cfg;
	GxMsgProperty_EpgCreate* cfg_params = NULL;
//	GxMsgProperty_EpgGet epg;
	GxMsgProperty_EpgGet* epg_params = NULL;
	static uint32_t i;
	
//	uint32_t prog_num = 0;
//	GxBusPmDataProg * prog_arry = NULL;

	switch (msg_id)
	{
		case 1:
			msg = GxBus_MessageNew(GXMSG_EPG_CREATE);
			cfg_params = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_EpgCreate);
			cfg_params->service_count = 10;
			cfg_params->event_count = 100;
			cfg_params->event_size = 100;
			cfg_params->epg_day = 3;
			GxBus_MessageSendWait(msg);
			GxBus_MessageFree(msg);
			break;

		case 2:
			msg = GxBus_MessageNew(GXMSG_EPG_GET);
			//msg = GxBusMsgNew(GXMSG_TYPE_NORMAL,GXMSG_PRIOR_NORMAL);
	
			epg_params = GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_EpgGet);

			//prog_num = GxBus_PmProgNumGet();
			//prog_arry  = (GxBusPmDataProg *)GxCore_Malloc(sizeof(GxBusPmDataProg)*prog_num);
			//memset(prog_arry,0,sizeof(GxBusPmDataProg)*prog_num);
			//GxBus_PmProgGetByPos(0,prog_num,prog_arry);

			//for (i=0; i<prog_num; i++)
			
			epg_params->service_id = 3106;
			epg_params->ts_id = 1502;
			epg_params->epg_info = (GxEpgInfo *)GxCore_Malloc(15*1024);
			epg_params->epg_info_size = 15*1024;
			
			if (epg_params->epg_info == NULL)
			{
				gxlogd("alloc epg space failed!\n");
			}

			GxBus_MessageSendWait(msg);
			for(i=0; i<epg_params->real_event_count; i++)
			{
				gxlogd("demo : %s \n", epg_params->epg_info[i].event_name);
			}
			
		
			gxlogd("free!\n");
			GxCore_Free(epg_params->epg_info);
			epg_params->epg_info = NULL;
			GxBus_MessageFree(msg);
			break;

	}


}
#if 0
int main(void)
{
	//status_t rel = GXCORE_ERROR;
	CheckTSLock();

	GxCore_Init();
	
	

	if (GxBus_SiParserBufCreate() == GXCORE_ERROR)
	{
		gxlogd("init mempool failed!\n");
	}

	
	GxBus_Init();
	
	while (1) {
			static uint32_t once = 0;

				if (once == 0)
				{
					epg_test_msg(1);
					once = 1;
				}

			//GxCore_ThreadDelay(2000);
			epg_test_msg(2);
				
		GxCore_ThreadDelay(100);
	}
	return 0;
}
#endif

int GxCore_Startup(int argc, char **argv)
{
	//status_t rel = GXCORE_ERROR;
	//CheckTSLock();
	GxServiceClass serv_list[2] = 
	{{	"epg_service",	GxEpgServiceInit,	GxEpgServiceDestroy,	GxEpgServiceRecvMsg,	GxEpgServiceConsole},
	{	"si_service",	GxSiServiceInit,	GxSiServiceDestroy,	        GxSiServiceRecvMsg,	GxSiServiceConsole	}};
//GxServiceClass ServSi = 
	GxCore_Init();
	
	if (GxBus_SiParserBufCreate() == GXCORE_ERROR)
	{
		gxlogd("init mempool failed!\n");
	}


	GxBus_Init(serv_list, 2);

    	GxCore_Loop();

	return 0;
}



