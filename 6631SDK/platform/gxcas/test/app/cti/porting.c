#include "gxcore.h"
#include "porting.h"
#include "gxcas/ctiapi.h"
#include "gxcas/ctidef.h"
#include "gxcas/gxcas.h"
#include "fcntl.h"
#include "frontend/demod.h"
#include "frontend/tuner.h"
#include "dvbhal/gxfe_hal.h"


#define PLAY_PAR1	  "dvbc://vpid:85&apid:652&pcrpid:83&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:269&tsid:0&dmxid:0&service_id:0001" //TS_06_add_1st&2nd_entitlments.ts
//#define PLAY_PAR1	  "dvbc://vpid:4507&apid:4508&pcrpid:4507&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:415&tsid:0&dmxid:0&service_id:111" //
#define PLAY_PAR2	  "dvbc://vpid:514&apid:648&pcrpid:83&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:268&tsid:0&dmxid:0&service_id:0002" //TS_06_add_1st&2nd_entitlments.ts
#define PLAY_PAR3	  "dvbc://vpid:517&apid:84&pcrpid:83&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:267&tsid:0&dmxid:0&service_id:0003" //TS_06_add_1st&2nd_entitlments.ts
#define PLAY_PAR4	  "dvbc://apid:521&apid:676&pcrpid:83&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:265&tsid:0&dmxid:0&service_id:0004" //TS_06_add_1st&2nd_entitlments.ts
//#define PLAY_PAR5	  "dvbc://apid:280&pcrpid:280&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:3065" //RFI Francais 3065

void GxCas_CaTest_FrontendInit()
{
#ifdef ECOS_OS
	//frontend_mod_init_gx3211(1, "|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//strong
	struct dev_attach dev;
	int demod_ts_mode = SERIAL;
	int iq_swap = 0;
	int demod_is_oscillator = 0;
	handle_t tuner_handle ;

	dev.tuner_attach = MXL608_C_ATTACH;
	dev.demod_attach = Taurus_DVBC_ATTACH;

	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xA4;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0xC0;
	
	register_frontend();

	if(GxFrontend_Init(0)<0)
	{
		return;
	}

	handle_t fe_handle = GxFrontend_IdToHandle(0);
	if (fe_handle < 0) {
		printf("register frontend failed\n");
		return;
	}
	ioctl(fe_handle, FE_SET_DEVICE, &dev);

	ioctl(fe_handle, FE_SET_TS_MODE, (void*)demod_ts_mode);	
	ioctl(fe_handle, FE_SET_IQ_SWAP, (void*)iq_swap);	
	ioctl(fe_handle, FE_SET_IS_OSCILLATOR, (void*)demod_is_oscillator);	
	ioctl(fe_handle, FE_SET_REINIT, NULL);
	//close(fe_handle);

	tuner_handle = GxFeOpen(0);
#endif
}

int GxCas_Demux()
{
	GxDmxInit();
	GxDmxSetSource(0, DEMUX_TS1);	
	return 0;
}

bool GxCas_GxFeLockTp(void )
{
	GxFeTpInfo tpInfo={0};
	fe_status_t status=0;
	int demux_lock = 0;

	uint32_t fre = 0;
	uint32_t symb = 0;
	uint32_t ret = 0;
	GxTime old = {0}, new = {0};

	memset(&tpInfo,0,sizeof(tpInfo));
	tpInfo.type = GXFE_DVB_C;
	tpInfo.fre = CTI_FREQ; // khz 
	tpInfo.symb = CTI_SYMB;
	tpInfo.qam = DVBC_64QAM;
	tpInfo.force_lock=1;
	
	ret = GxFeLockTp(0,tpInfo);
	GxCore_GetTickTime(&old);
	while(1)
	{
		ret = GxFeGetStatus(0,&status);
		if(status == FE_HAS_LOCK)
			printf("----TUNER LOCK\n");
		else
			printf("----TUNER UNLOCK, ret = [%d]\n", status);

		demux_lock = GxDmxCheckTsLock(0);
		if(demux_lock < 0)
			printf("----DEMUX UNLOCK\n");
		else
			printf("----DEMUX LOCK\n");

		GxCore_GetTickTime(&new);
		if ((status == FE_HAS_LOCK) && (demux_lock > 0)) 
		{
			printf("----TUNER AND DEMUX IS LOCKED\n");	
			return TRUE;
		}
		if ((new.microsecs - old.microsecs) >= 5000) //5 S
		{
			printf("error: %s(%d) timeout\n", __func__, __LINE__);
			return FALSE;
		}
		
		GxCore_ThreadDelay(1000);
	}
}

void GxCas_CaTest_GetTpParam(char* param)
{
	GxCas_Demux();
	GxCas_GxFeLockTp();
	//strcpy(param, "dvbc://fre:299000&symbol:6875&bandwidth:0&qam:3&tuner:0&tsid:0");//3201h
}

static void cti_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		GxCas_WaitEvent(&type, &p);
		GxCore_Free(p);
	}
}

void GxCas_CaTest_CaInit()
{
	handle_t handle = 0;
	GxCasInitParam init = {0};

	init.dmx_id = 0;
	init.ts_id = 0;
	strcpy(init.flash.name, "CA");
	init.flash.size = 64*1024;
	init.flash.offset = 0;
	strcpy(init.backup.name, "CA");
	init.backup.size = 64*1024;
	init.backup.offset = 64*1024;
	
	init.sci.sci_switch = 1;
	init.sci.detect_pole = GXCAS_SMC_HIGH_LEVEL;
	init.sci.vcc_pole = GXCAS_SMC_HIGH_LEVEL;
	GxCas_CtiHsInit(init);
	GxCore_ThreadCreate("cd_wait_event", &handle, cti_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[4] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3, PLAY_PAR4};
	fe_status_t status=0;
	int demux_lock = 0;

	if (type == GXCAS_TEST_KEYUP) {
		pos++;
		if (pos == sizeof(playmap)/sizeof(char *))
			pos = 0;
	} else if (type == GXCAS_TEST_KEYDOWN) {
		pos--;
		if (pos < 0)
			pos = (sizeof(playmap)/sizeof(char *)) - 1;
	}

	if ((type == GXCAS_TEST_KEYUP) || (type == GXCAS_TEST_KEYDOWN) || (type == GXCAS_TEST_PLAY)) 
	{
		GxCas_TestCa_Play(playmap[pos]);
		GxCasSet_SwitchChannel gxcas_switch = {0};
		GxUrl_GetItem(playmap[pos],"pmt", &pmt_pid);
		GxUrl_GetItem(playmap[pos],"service_id", &service_id);
		GxUrl_GetItem(playmap[pos],"apid", &a_pid);
		GxUrl_GetItem(playmap[pos],"vpid", &v_pid);
		gxcas_switch.pmt_pid = pmt_pid;
		gxcas_switch.service_id = service_id;
		gxcas_switch.audio_pid = a_pid;
		gxcas_switch.video_pid = v_pid;
		GxCas_Set(GXCAS_CTI_SWITCH_CHANNEL, (void *)&gxcas_switch);
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			CTICAS_SendMsgToCAcore(SC_INFO_QUERY, NULL, 0);
			break;
		case GXCAS_TEST_KEY1:
			 GxFeGetStatus(0,&status);
			if(status == FE_HAS_LOCK)
				printf("----TUNER LOCK\n");
			else
				printf("----TUNER UNLOCK, ret = [%d]\n", status);

			demux_lock = GxDmxCheckTsLock(0);
			if(demux_lock < 0)
				printf("----DEMUX UNLOCK\n");
			else
				printf("----DEMUX LOCK\n");

			if ((status == FE_HAS_LOCK) && (demux_lock > 0)) 
				printf("----TUNER AND DEMUX IS LOCKED\n");
			else
				printf("----TUNER AND DEMUX IS UNLOCKED\n");
			break;
		case GXCAS_TEST_KEY2:
			break;
		case GXCAS_TEST_KEY3:
			break;
		case GXCAS_TEST_KEY4:
			break;
		case GXCAS_TEST_KEY5:
			break;
		case GXCAS_TEST_KEY6:
			break;
		case GXCAS_TEST_KEY7:
			break;
		case GXCAS_TEST_KEY8:
			break;
		case GXCAS_TEST_KEY9:
			break;
		default:
			break;
	}
}
