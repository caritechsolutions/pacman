#include "gxcore.h"
#include "porting.h"
#include "gxcas/dvt_api.h"
#include "gxcas/gxcas.h"
#include "frontend.h"
#include "fcntl.h"

#define DVT_HS10_NOSCI
#ifdef DVT_HS10_NOSCI
#define PROGRAM_NUM (7)
#define PLAY_PAR1	  "dvbc://vpid:512&apid:650&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:257&tsid:0&dmxid:0&service_id:301"
#define PLAY_PAR2	  "dvbc://vpid:513&apid:660&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:258&tsid:0&dmxid:0&service_id:302"
#define PLAY_PAR3	  "dvbc://vpid:514&apid:670&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:259&tsid:0&dmxid:0&service_id:303"
#define PLAY_PAR4	  "dvbc://vpid:515&apid:680&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:260&tsid:0&dmxid:0&service_id:304"
#define PLAY_PAR5	  "dvbc://vpid:516&apid:690&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:261&tsid:0&dmxid:0&service_id:305"
#define PLAY_PAR6	  "dvbc://vpid:517&apid:700&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:262&tsid:0&dmxid:0&service_id:306"
#define PLAY_PAR7	  "dvbc://vpid:518&apid:710&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:307"
#endif

#if 0
#define PROGRAM_NUM (6)
//#define PLAY_PAR1	  "dvbs://vpid:512&apid:650&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:257&tsid:0&dmxid:0&service_id:301"
#define PLAY_PAR1	  "dvbs://vpid:513&apid:660&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:258&tsid:0&dmxid:0&service_id:302"
#define PLAY_PAR2	  "dvbs://vpid:514&apid:670&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:259&tsid:0&dmxid:0&service_id:303"
#define PLAY_PAR3	  "dvbs://vpid:515&apid:680&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:260&tsid:0&dmxid:0&service_id:304"
#define PLAY_PAR4	  "dvbs://vpid:516&apid:690&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:261&tsid:0&dmxid:0&service_id:305"
#define PLAY_PAR5	  "dvbs://vpid:517&apid:700&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:262&tsid:0&dmxid:0&service_id:306"
#define PLAY_PAR6	  "dvbs://vpid:518&apid:710&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:307"
#endif



void GxCas_CaTest_FrontendInit()
{
	frontend_mod_init_gx1801(1, "|0:0:0xe4:62:0:0xc0:&0:0");//3201h
	//frontend_mod_init_gx3211(1, "|1:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//6605s
#if 0
	//frontend_mod_init_atbm888x(1, "|7:0:0x80:62:0:0xc0:&0:0");//3212
	struct dev_attach dev;


	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xe4;
	dev.demod_attach = GX3211_ATTACH;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0xc0;
	dev.tuner_attach = RDA5815M_ATTACH;
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
	strcpy(param, "dvbc://fre:714000&symbol:6875000&bandwidth:0&qam:3&tuner:0&tsid:0");//3201h
	//strcpy(param, "dtmb://fre:714000&bandwidth:0&qam:3&tuner:0&tsid:1"); //3212
	//strcpy(param, "dvbs://fre:1150&polar:1&symbol:27500&22k:1&tuner:0&tsid:0");
}

static void dvt_event(void *arg)
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
	strcpy(init.flash.name, "DATA");
	init.flash.size = 128*1024;
	init.flash.offset = 0;
	init.sci.sci_switch = 0;
	GxCas_DvtHsInit(init);
        //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SMC);
        //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
	GxCore_ThreadCreate("dvt_wait_event", &handle, dvt_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[PROGRAM_NUM] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3, PLAY_PAR4, PLAY_PAR5, PLAY_PAR6};

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
		GxCas_Set(GXCAS_DVT_SWITCH_CHANNEL, (void *)&gxcas_switch);
		GxCas_TestCa_Play(playmap[pos]);
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			{
				GxCas_DvtEntileInfo entitle = {0};
				entitle.wTvsId = 12;
				GxCas_Get(GXCAS_DVT_GET_ENTITLE_INFO, &entitle);
				break;
			}
		case GXCAS_TEST_KEY1:
			{
				GxCas_DvtManuIno info = {0};
				GxCas_Get(GXCAS_DVT_GET_MANUINFO, &info);
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
