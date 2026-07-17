#include "gxcore.h"
#include "porting.h"
#include "gxcas/gxcas.h"
#include "gxcas/rs_api.h"
#if 1
#include "frontend.h"
#include "frontend/demod.h"
#include "frontend/tuner.h"
#include "fcntl.h"
#endif

#define PLAY_PAR1	  "dvbs://apid:1120&vpid:1160&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:257&tsid:0&dmxid:0&service_id:401"
#define PLAY_PAR2	  "dvbs://apid:1220&vpid:1260&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:258&tsid:0&dmxid:0&service_id:402"
#define PLAY_PAR3	  "dvbs://apid:1320&vpid:1360&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:259&tsid:0&dmxid:0&service_id:403"
#define PLAY_PAR4	  "dvbs://apid:1420&vpid:1460&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:260&tsid:0&dmxid:0&service_id:404"
#define PLAY_PAR5	  "dvbs://apid:1520&vpid:1560&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:261&tsid:0&dmxid:0&service_id:405"
void GxCas_CaTest_FrontendInit()
{
#if 0
	//frontend_mod_init_gx1801(1,"|0:0:0xe4:62:0:0xc0:&0:0");
	frontend_mod_init_gx3211(1, "|8:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//strong
#else
	struct dev_attach dev;

	dev.tuner_attach = RDA5815M_ATTACH;
	dev.demod_attach = GX3211_ATTACH;

	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xe4;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0x18;
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
	//strcpy(param, "dvbc://fre:306000&qam:3&symbol:6875000&tuner:0&tsid:0");
	strcpy(param, "dvbs://fre:1370&polar:0&symbol:30000&22k:1&tuner:0&tsid:0");
}
extern int32_t GxCas_Desc_SetCW(int32_t descid,uint16_t pid,uint8_t* OddKey,uint8_t* EvenKey,uint32_t KeyLen );
extern int32_t GxCas_Desc_Open(void);
extern int32_t GxCas_Demux_Init(uint32_t tsid, uint32_t dmx_id);
extern int32_t GxCas_Desc_Init(uint32_t dmxid);
static void rs_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		enum RSCA_CODE_TYPE* msg = NULL;
		GxCas_WaitEvent(&type, &p);
		msg = (enum RSCA_CODE_TYPE *)p;
		printf("!!!Get Event:%s,%d,%d\n", __func__, __LINE__, *msg);
		switch(*msg){
		case RSCA_C01://email
			{
				printf("%s,%d,Email Get\n", __func__, __LINE__);
				GxCas_Rs_Email email = {0};
				email.pos = 0;
				GxCas_Get(GXCAS_RS_GET_EMAILNUM, &email.num);
				email.email = GxCore_Mallocz(sizeof(RSCA_MAIL_t));
				GxCas_Get(GXCAS_RS_GET_EMAIL, &email);
				GxCore_Free(email.email);
				break;
			}
		case RSCA_C02://osd
			{
				printf("%s,%d,Osd Get\n", __func__, __LINE__);
				RSCA_SUBTITLE_t osd = {0};
				GxCas_Get(GXCAS_RS_GET_OSD, &osd);
				break;
			}
		case RSCA_C03://finger
			{
				printf("%s,%d,Finger Get\n", __func__, __LINE__);
				GxCas_FpInfo finger = {0};
				GxCas_Get(GXCAS_RS_GET_FP, &finger);
				break;
			}
		case RSCA_C16://hasten
			{
				printf("%s,%d, Hasten Get\n", __func__, __LINE__);
				GxCas_HastenInfor hasten = {0};
				GxCas_Get(GXCAS_RS_GET_HASTEN, &hasten);
				break;
			}
		case RSCA_C30://watermark
			{
				printf("%s,%d, Watermark Get\n", __func__, __LINE__);
				RSCA_WaterMarkApp_t watermark = {0};
				GxCas_Get(GXCAS_RS_GET_WATERMASK, &watermark);
				break;
			}
		default:
			break;
		}
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
	init.flash.size = 320*1024;
	init.flash.offset = 0;
	init.sci.sci_switch = 0;
	GxCas_RsHsInit(init);
	GxCore_ThreadCreate("rs_wait_event", &handle, rs_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

static void rs_get_wt()
{
	GxCas_RS_WorkTime wt = {0};
	strcpy(wt.oldpin,"1234");
	GxCas_Get(GXCAS_RS_GET_WORK_TIME, &wt);
}
static void rs_set_wt()
{
	GxCas_RS_WorkTime wt = {0};
	wt.worktime.starHour = 15;
	wt.worktime.endHour = 23;
	wt.worktime.endMin = 59;
	strcpy(wt.oldpin,"1234");
	GxCas_Set(GXCAS_RS_SET_WORK_TIME, &wt);
}
static void rs_get_wl()
{
	GxCas_RS_WatchLevel wl = {0};
	strcpy(wl.oldpin,"1234");
	GxCas_Get(GXCAS_RS_GET_WATCH_LEVEL, &wl);
}
static void rs_set_wl()
{
	GxCas_RS_WatchLevel wl = {0};
	wl.level = 12;
	strcpy(wl.oldpin,"1234");
	GxCas_Set(GXCAS_RS_SET_WATCH_LEVEL, &wl);
}
static void rs_set_pin(void)
{
	GxCas_RS_Pin pin = {0};
	strcpy(pin.newpin, "0000");
	strcpy(pin.oldpin, "1234");
	GxCas_Set(GXCAS_RS_SET_PIN, &pin);
}

static void get_usrinfo(void)
{
	RSCA_USERINFOR_t info = {0};
	usrio_getuserinfor(&info);
	GxCas_Get(GXCAS_RS_GET_USRINFO, &info);
}

static void get_entitle(void)
{
	RS_U16 num = 0;
	GxCas_Rs_EntitleInfo info = {0};

	GxCas_Get(GXCAS_RS_GET_ENTITLE, &info);
	GxCas_Get(GXCAS_RS_GET_CURENTITLEINDEX, &num);
}

static void get_email()
{
	printf("%s,%d,Email Get\n", __func__, __LINE__);
	GxCas_Rs_Email email = {0};

	email.pos = 0;
	GxCas_Get(GXCAS_RS_GET_EMAILNUM, &email.num);//num = 1
	email.email = GxCore_Mallocz(sizeof(RSCA_MAIL_t));
	GxCas_Get(GXCAS_RS_GET_EMAIL, &email);
	printf("email head:%s\n", (char *)email.email->mailHData);
	printf("email body:%s\n", (char *)email.email->mailBData);
	printf("email tail:%s\n", (char *)email.email->mailTData);
	GxCore_Free(email.email);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	int32_t ret = 0;
	int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[5] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3, PLAY_PAR4, PLAY_PAR5};
	char data[3] = {200,2,3};
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
		GxCas_Set(GXCAS_RS_SWITCH_CHANNEL, (void *)&gxcas_switch);
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			rs_get_wt();
			break;
		case GXCAS_TEST_KEY1:
			rs_get_wl();
			break;
		case GXCAS_TEST_KEY2:
			rs_set_wt();
			break;
		case GXCAS_TEST_KEY3:
			rs_set_wl();
			break;
		case GXCAS_TEST_KEY4:
			get_usrinfo();
			break;
		case GXCAS_TEST_KEY5:
			get_entitle();
			break;
		case GXCAS_TEST_KEY6:
			get_email();
			break;
		case GXCAS_TEST_KEY7:
			rs_set_pin();
			break;
		case GXCAS_TEST_KEY8:
			break;
		case GXCAS_TEST_KEY9:
			break;
		default:
			break;
	}
}
