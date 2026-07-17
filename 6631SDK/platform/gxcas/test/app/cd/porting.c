#include "gxcore.h"
#include "porting.h"
#include "gxcas/cd_api.h"
#include "gxcas/CDCASS.h"

#define PLAY_PAR1	  "dvbs://vpid:264&apid:265&pcrpid:264&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:256&tsid:0&dmxid:0&service_id:3601" //now 3061
#define PLAY_PAR2	  "dvbs://vpid:273&apid:274&pcrpid:273&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:260&tsid:0&dmxid:0&service_id:3063" //TV5 3063
#define PLAY_PAR3	  "dvbs://vpid:275&apid:276&pcrpid:275&vcodec:0&acodec:0&tuner:0&scramble:0&pmt:261&tsid:0&dmxid:0&service_id:3062" //DW-TV ASIEN 3062
#define PLAY_PAR4	  "dvbs://apid:279&pcrpid:278&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:262&tsid:0&dmxid:0&service_id:3064" //DW-M 3064
#define PLAY_PAR5	  "dvbs://apid:280&pcrpid:280&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:3065" //RFI Francais 3065

void GxCas_CaTest_FrontendInit()
{
#ifdef ECOS_OS
	frontend_mod_init_gx3211(1, "|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//strong
#endif
}

void GxCas_CaTest_GetTpParam(char* param)
{
	strcpy(param, "dvbs://fre:1150&polar:0&symbol:27500&22k:1&tuner:0&tsid:0");
}

static void cd_event(void *arg)
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
	strcpy(init.flash.name, "PRIVATE");
	init.flash.size = 128*1024;
	init.flash.offset = 0;
	init.sci.sci_switch = 1;
	init.sci.detect_pole = GXCAS_SMC_HIGH_LEVEL;
	init.sci.vcc_pole = GXCAS_SMC_LOW_LEVEL;
	GxCas_CdHsInit(init);
	GxCore_ThreadCreate("cd_wait_event", &handle, cd_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[5] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3, PLAY_PAR4, PLAY_PAR5};

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
		GxCas_Set(GXCAS_CD_SWITCH_CHANNEL, (void *)&gxcas_switch);
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			break;
		case GXCAS_TEST_KEY1:
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
