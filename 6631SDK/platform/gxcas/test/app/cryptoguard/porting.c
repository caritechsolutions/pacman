#include "time.h"
#include "gxcore.h"
#include "porting.h"
#include "gxcas/cryptoguard_api.h"
#include "gxcas/gxcas.h"
#include "frontend.h"
#include "fcntl.h"
#include "tuner.h"
#include "demod.h"

#if 1
#define PROGRAM_NUM (5)
#define PLAY_PAR1	  "dvbc://vpid:880&apid:881&pcrpid:880&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:870&tsid:0&dmxid:0&service_id:870"
#define PLAY_PAR2	  "dvbc://vpid:1249&apid:1248&pcrpid:1249&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:1240&tsid:0&dmxid:0&service_id:1240"
#define PLAY_PAR3	  "dvbc://vpid:3441&apid:3440&pcrpid:3440&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:3430&tsid:0&dmxid:0&service_id:3430"
#define PLAY_PAR4	  "dvbc://vpid:1030&apid:1031&pcrpid:1030&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:5010&tsid:0&dmxid:0&service_id:5010"
#define PLAY_PAR5	  "dvbc://vpid:1019&apid:1018&pcrpid:1019&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:5710&tsid:0&dmxid:0&service_id:5710"
#endif
#if 0
#define PROGRAM_NUM (6)
#define PLAY_PAR1	  "dvbs://vpid:1010&apid:1011&pcrpid:1010&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:1015&tsid:0&dmxid:0&service_id:101"
#define PLAY_PAR2	  "dvbs://vpid:1020&apid:1021&pcrpid:1020&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:1025&tsid:0&dmxid:0&service_id:102"
#define PLAY_PAR3	  "dvbs://vpid:1030&apid:1031&pcrpid:1030&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:1035&tsid:0&dmxid:0&service_id:103"
#define PLAY_PAR4	  "dvbs://vpid:1210&apid:1211&pcrpid:1214&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:1215&tsid:0&dmxid:0&service_id:121"
#define PLAY_PAR5	  "dvbs://vpid:1220&apid:1221&pcrpid:1214&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:1225&tsid:0&dmxid:0&service_id:122"
#define PLAY_PAR6	  "dvbs://vpid:1350&apid:1351&pcrpid:1214&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:1355&tsid:0&dmxid:0&service_id:135" //cctv_music
#endif



void GxCas_CaTest_FrontendInit()
{
	//frontend_mod_init_gx1801(1, "|0:0:0xe4:62:0:0xc0:&0:0");//3201h
	//frontend_mod_init_gx3211(1, "|1:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//6605s rda5815M
	//frontend_mod_init_gx3211(1, "|1:0:0xe4:41:0:0xc0:&0:1:2:0:0:1");//6605s av2011
#if 1
	//frontend_mod_init_atbm888x(1, "|7:0:0x80:62:0:0xc0:&0:0");//3212
	struct dev_attach dev;


	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xe4;
	dev.demod_attach = GX1801_ATTACH;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0xc0;
	dev.tuner_attach = ATBM2040_C_ATTACH;
	register_frontend();

	handle_t fe_handle = open("/dev/gxfe0", O_RDWR);
	if (fe_handle < 0) {
		printf("register frontend failed\n");
		return;
	}
	ioctl(fe_handle, FE_SET_DEVICE, &dev);
	close(fe_handle);

#endif
}

void GxCas_CaTest_GetTpParam(char* param)
{
	strcpy(param, "dvbc://fre:850000&symbol:6875000&bandwidth:0&qam:3&tuner:0&tsid:0");//3201h
	//strcpy(param, "dtmb://fre:714000&bandwidth:0&qam:3&tuner:0&tsid:1"); //3212
	//strcpy(param, "dvbs://fre:1150&polar:1&symbol:27500&22k:1&tuner:0&tsid:0");
}



static void cpg_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		GxCas_WaitEvent(&type, &p);
		//printf("%s,%d,service_id :%d, error type:%d\n", __func__, __LINE__, err_info->ServiceID, err_info->ca_error_type);
		switch(type)
		{
			default:
				break;
		}
		GxCore_ThreadDelay(100);
		GxCore_Free(p);
	}
}

void GxCas_CaTest_CaInit()
{
	handle_t handle = 0;
	GxCasInitParam init = {0};

	init.dmx_id = 0;
	init.ts_id = 0;
	//strcpy(init.flash.name, "/home/gx/gxca_nvram.dat");
#if 1
	strcpy(init.flash.name, "CA");
	init.flash.size = 64*1024;
	init.flash.offset = 0;
	init.sci.sci_switch = 1;
	init.sci.detect_pole = GXCAS_SMC_HIGH_LEVEL;
	init.sci.vcc_pole = GXCAS_SMC_HIGH_LEVEL;
	GxCas_CryptoGuardInit(init);
#else
	init.sci.sci_switch = 1;
	init.flash.offset = 0;
	init.flash.size = 64*1024;
	strcpy(init.flash.name, "PRIVATE");
	init.backup.offset = 0;
	init.backup.size = 64*1024;
	strcpy(init.backup.name, "BACKUP");
	GxCas_CryptoGuardInit(init);
#endif
	{
	GxCas_CGSetStbInfo CPGStbInfo;
	memset(&CGStbInfo, 0, sizeof(GxCas_CGSetStbInfo));

	unsigned char SystemGlobalKey[]         = { 0xDE,0x52,0x38,0xF7,0xCD,0x98,0x20,0x35,0x70,0x12,0x36,0xC1,0x59,0xF3,0x67,0x12 };
	unsigned char ManufacturerGlobalKey[]   = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
	unsigned char UniqueKey[]               = { 0xA1,0x26,0x5C,0x1D,0xB4,0x6F,0xC7,0xBC,0xC1,0xB6,0x3B,0xD3,0x47,0x5F,0xDA,0xA2 };
	unsigned char GroupKey[]                = { 0xD6,0x99,0xE2,0xAB,0x4C,0xCF,0x41,0xC8,0xF3,0x4A,0x13,0xFD,0xD3,0xB6,0xEF,0x7F };
	unsigned char CardlessGroupKey[]        = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
	unsigned char CardlessUniqueKey[]       = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
	CPGStbInfo.IrdNumber = 990000001;

	memcpy(CPGStbInfo.STB_SystemGlobalKey, SystemGlobalKey, 16);
	memcpy(CPGStbInfo.STB_ManufacturerGlobalKey, ManufacturerGlobalKey, 16);
	memcpy(CPGStbInfo.STB_GroupKey, GroupKey, 16);
	memcpy(CPGStbInfo.STB_UniqueKey, UniqueKey, 16);
	memcpy(CPGStbInfo.STB_CardlessGroupKey, CardlessGroupKey, 16);
	memcpy(CPGStbInfo.STB_CardlessUniqueKey, CardlessUniqueKey, 16);
	GxCas_Set(GXCAS_CRYPTOGUARD_SET_STB_INFO, (void *)&CPGStbInfo);
	}

    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
    //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
	GxCore_ThreadCreate("cpg_wait_event", &handle, cpg_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	static int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[PROGRAM_NUM] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3, PLAY_PAR4, PLAY_PAR5};

	if (type == GXCAS_TEST_KEYUP) {
		pos++;
		if (pos == sizeof(playmap)/sizeof(char *))
			pos = 0;
	} else if (type == GXCAS_TEST_KEYDOWN) {
		pos--;
		if (pos < 0)
			pos = (sizeof(playmap)/sizeof(char *)) - 1;
	}

	if ((type == GXCAS_TEST_KEYUP) || (type == GXCAS_TEST_KEYDOWN) || (type == GXCAS_TEST_PLAY)) {
		GxCasSet_SwitchChannel gxcas_switch = {0};
		GxUrl_GetItem(playmap[pos],"pmt", &pmt_pid);
		GxUrl_GetItem(playmap[pos],"service_id", &service_id);
		GxUrl_GetItem(playmap[pos],"apid", &a_pid);
		GxUrl_GetItem(playmap[pos],"vpid", &v_pid);
		gxcas_switch.pmt_pid = pmt_pid;
		gxcas_switch.service_id = service_id;
		gxcas_switch.audio_pid = a_pid;
		gxcas_switch.video_pid = v_pid;
		GxCas_Set(GXCAS_CRYPTOGUARD_SWITCH_CHANNEL, (void *)&gxcas_switch);
		GxCas_TestCa_Play(playmap[pos]);
	if(0)
	{
		static handle_t ades = 0;
		static handle_t vdes = 0;
		unsigned char buf[8] = {0x01,0x02,0x03,0x06,0x04,0x05,0x06,0x0f};
		if(vdes == 0)
		    vdes = GxCas_Desc_Open();
		if(ades == 0)
		    ades = GxCas_Desc_Open();
		GxCas_Desc_SetCW(vdes, v_pid, buf, buf, 8);
		GxCas_Desc_SetCW(ades, a_pid, buf, buf, 8);
	}
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			{
				break;
			}
		case GXCAS_TEST_KEY1:
			{
				break;
			}
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
			{
				break;
			}
		default:
			break;
	}
}
