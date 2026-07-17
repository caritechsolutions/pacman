#include "gxcore.h"
#include "porting.h"
#include "gxcas/gxcas.h"
#include "gxcas/abv_api.h"
#include "gxcas/osd_api.h"
#include "frontend.h"
#include "frontend/demod.h"
#include "frontend/tuner.h"
#include "fcntl.h"

#define lts
#ifdef lts
#define PROGRAM_NUM (6)
#define PLAY_PAR1	  "dvbc://vpid:33&apid:34&pcrpid:33&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:32&tsid:0&dmxid:0&service_id:3301"
#define PLAY_PAR2	  "dvbc://vpid:36&apid:37&pcrpid:36&vcodec:0&acodec:0&tuner:0&scramble:1&pmt:35&tsid:0&dmxid:0&service_id:3302"
#define PLAY_PAR3	  "dvbc://vpid:40&apid:41&pcrpid:40&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:39&tsid:0&dmxid:0&service_id:3303"
#define PLAY_PAR4	  "dvbc://vpid:43&apid:44&pcrpid:43&vcodec:2&acodec:4&tuner:0&scramble:1&pmt:42&tsid:0&dmxid:0&service_id:3304"
#define PLAY_PAR5	  "dvbc://vpid:46&apid:47&pcrpid:46&vcodec:2&acodec:4&tuner:0&scramble:0&pmt:45&tsid:0&dmxid:0&service_id:3305"
#define PLAY_PAR6	  "dvbc://vpid:49&apid:50&pcrpid:49&vcodec:2&acodec:4&tuner:0&scramble:0&pmt:48&tsid:0&dmxid:0&service_id:3306"
//#define PLAY_PAR6	  "dvbc://vpid:0&apid:53&pcrpid:52&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:51&tsid:0&dmxid:0&service_id:3307"
//#define PLAY_PAR7	  "dvbc://vpid:0&apid:55&pcrpid:55&vcodec:0&acodec:0&tuner:0&scramble:0&pmt:54&tsid:0&dmxid:0&service_id:3308"
#endif

#ifdef ABV_HS
#define PROGRAM_NUM (3)
#define PLAY_PAR1	  "dvbc://vpid:264&apid:265&pcrpid:264&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:258&tsid:0&dmxid:0&service_id:257"
#define PLAY_PAR2	  "dvbc://vpid:273&apid:274&pcrpid:273&vcodec:0&acodec:1&tuner:0&scramble:1&pmt:260&tsid:0&dmxid:0&service_id:261"
#define PLAY_PAR3	  "dvbc://vpid:275&apid:276&pcrpid:275&vcodec:0&acodec:0&tuner:0&scramble:1&pmt:261&tsid:0&dmxid:0&service_id:262"
#endif

#ifdef ABV_HS_NOSCI
#define PROGRAM_NUM (6)
#define PLAY_PAR1	  "dvbc://vpid:273&apid:274&pcrpid:273&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:259&tsid:0&dmxid:0&service_id:3221"
#define PLAY_PAR2	  "dvbc://vpid:282&apid:283&pcrpid:282&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:3222"
#define PLAY_PAR3	  "dvbc://vpid:284&apid:285&pcrpid:284&vcodec:0&acodec:0&tuner:0&scramble:0&pmt:264&tsid:0&dmxid:0&service_id:3223"
#define PLAY_PAR4	  "dvbc://vpid:269&apid:270&pcrpid:269&vcodec:2&acodec:4&tuner:0&scramble:0&pmt:257&tsid:0&dmxid:0&service_id:3224"
#define PLAY_PAR5	  "dvbc://vpid:271&apid:272&pcrpid:271&vcodec:2&acodec:4&tuner:0&scramble:0&pmt:258&tsid:0&dmxid:0&service_id:3225"
#define PLAY_PAR6	  "dvbc://vpid:267&apid:268&pcrpid:267&vcodec:2&acodec:4&tuner:0&scramble:0&pmt:256&tsid:0&dmxid:0&service_id:3226"


#endif

void GxCas_CaTest_FrontendInit()
{
#ifdef ECOS_OS
	//frontend_mod_init_atbm888x(1, "|7:0:0x80:62:0:0xc0:&0:0");//3212
	//frontend_mod_init_gx1801(1,"|0:0:0xe4:62:0:0xc0:&0:0");//3201h
	struct dev_attach dev;

	dev.tuner_attach = MXL608_C_ATTACH;
	dev.demod_attach = GX1801_ATTACH;

	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xe4;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0xc0;
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
}

GxTime abv_time = {0};
static void abv_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		CAS_ERROR_Type_t *err_info = NULL;
		GxCas_WaitEvent(&type, &p);
		err_info = (CAS_ERROR_Type_t *)p;
		//printf("%s,%d,service_id :%d, error type:%d\n", __func__, __LINE__, err_info->ServiceID, err_info->ca_error_type);
		switch(err_info->ca_error_type)
		{
			case MSG_STB_FINGERPRINT:
				{
					GxCas_AbvStbFingerPrint ecm_print = {0};

					if (GxCas_Get(GXCAS_ABV_GET_STB_FINGER_PRINT, &ecm_print) != ABV_FALSE)
						printf("%s,%d,service_id:%d, Get Ecm_finger Failed\n", __func__, __LINE__, err_info->ServiceID);
					
					GxCas_AbvEmmFingerPrint emm_print = {0};
					if (GxCas_Get(GXCAS_ABV_GET_EMM_FINGER_PRINT, &emm_print) != ABV_FALSE)
						printf("%s,%d,service_id:%d, Get Emm_finger Failed\n", __func__, __LINE__, err_info->ServiceID);
					else
						printf("%s,%d,%s\n", __func__, __LINE__, emm_print.content);

					break;
				}
			case MSG_CHANNEL_EMM_FP:
			case MSG_EMM_FINGERPRINT:
				{
					GxCas_AbvEmmFingerPrint emm_print = {0};

					if (GxCas_Get(GXCAS_ABV_GET_EMM_FINGER_PRINT, &emm_print) != ABV_FALSE)
						printf("%s,%d,service_id:%d, Get Emm_finger Failed\n", __func__, __LINE__, err_info->ServiceID);
					else
						printf("%s,%d,%s\n", __func__, __LINE__, emm_print.content);

					break;
				}
			case MSG_GET_SUB:
				{
					GxCas_AbvSubtitleInfo sub_info = {0};
					
					if (GxCas_Get(GXCAS_ABV_GET_SUBTITLE_INFO, &sub_info) != ABV_FALSE)
						printf("%s,%d,service_id:%d, Get Sub_info Failed\n", __func__, __LINE__, err_info->ServiceID);
					else
						printf("%s,%d,%s\n", __func__, __LINE__, sub_info.content);
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
	strcpy(init.flash.name, "DATA");
	init.flash.size = 128*1024;
	init.flash.offset = 0;
#if 0 //ABV
	init.sci.detect_pole = GXCAS_SMC_LOW_LEVEL;
	init.sci.vcc_pole = GXCAS_SMC_LOW_LEVEL;
#endif
#if 1 //STRONG
	init.sci.sci_switch = 1;
	init.sci.detect_pole = GXCAS_SMC_LOW_LEVEL;
	init.sci.vcc_pole = GXCAS_SMC_HIGH_LEVEL;
#endif
        //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SMC);
        //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
	GxCas_AbvHsInit(init);
	GxCore_ThreadCreate("abv_wait_event", &handle, abv_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

static void _get_ca_menu(void)
{
	//SmartCard_Info
	ABV_SC_Info_t sc_info = {0};
	if (GxCas_Get(GXCAS_ABV_GET_SMARTCARD_INFO, &sc_info) == ABV_TRUE)
		printf("ABV TEST Get SmartCard_Info Failed\n");
		

	//Provider_Info
	uint32_t i = 0;
	ABV_UInt8 provider_num = 0;

	if (GxCas_Get(GXCAS_ABV_GET_PPID_NUM, &provider_num) == -1)
		printf("ABV TEST Get Provider_Total_Num Failed\n");

	for (i = 0; i < provider_num; i++) {
		GxCas_AbvHistRecordNum record_num = {0};
		GxCas_AbvProviderinfo provider_info = {0};
		
		provider_info.ppid_no = i;
		if (GxCas_Get(GXCAS_ABV_GET_PROVIDER_INFO, &provider_info) == ABV_TRUE)
			printf("ABV TEST Get Provider_Info Failed\n");

		record_num.ppid_no = provider_info.ppid_no;
		if (GxCas_Get(GXCAS_ABV_GET_HIST_RECORD_NUM, &record_num) > 0) {
			uint32_t j = 0;
			for (j = 0; j < record_num.num; j++) {
				GxCas_AbvHistRecordInfo record_info = {0};
				
				record_info.ppid_no = provider_info.ppid_no;
				record_info.record_no = j;
				GxCas_Get(GXCAS_ABV_GET_HIST_RECORD_INFO, &record_info);
			}
		}
	}

#if 1
	//Change Pin
	GxCas_AbvChangePin pin = {0};
	strcpy(pin.old_pin, "1234");
	strcpy(pin.new_pin, "0000");
	strcpy(pin.match_pin, "0000");
	GxCas_Set(GXCAS_ABV_SET_PIN, &pin);

	//Modify Watch Time
	GxCas_AbvWorkTime work_time = {0};
	strcpy(work_time.pin, "0000");
	work_time.start_hour = 0;
	work_time.start_min = 0;
	work_time.end_hour = 32;
	work_time.end_min = 0;
	GxCas_Set(GXCAS_ABV_SET_WATCH_TIME, &work_time);
#endif

#if 0
	//Modify MR
	GxCas_AbvModifyMR modify_mr = {0};
	modify_mr.pin[0] = 0;
	modify_mr.pin[1] = 0;
	modify_mr.pin[2] = 0;
	modify_mr.pin[3] = 0;
	modify_mr.new_level = 22;
	GxCas_Set(GXCAS_ABV_MODIFY_MR, &modify_mr);
#endif
}

static void _get_development_menu(void)
{
	uint32_t i = 0;
	ABV_UInt8 provider_num = 0;

	if (GxCas_Get(GXCAS_ABV_GET_PPID_NUM, &provider_num) == -1)
		printf("ABV TEST Get Provider_Total_Num Failed\n");

	for (i = 0; i < provider_num; i++) {
		GxCas_AbvEntitleNum entitle_num = {0};
		GxCas_AbvProviderinfo provider_info = {0};
		
		provider_info.ppid_no = i;
		if (GxCas_Get(GXCAS_ABV_GET_PROVIDER_INFO, &provider_info) == ABV_TRUE)
			printf("ABV TEST Get Provider_Info Failed\n");

		//Floating entitle
		memset(&entitle_num, 0, sizeof(GxCas_AbvEntitleNum));
		entitle_num.ppid_no = provider_info.ppid_no;
		entitle_num.type = PPID_FREE_ENTITLE;
		if (GxCas_Get(GXCAS_ABV_GET_ENTITLE_NUM, &entitle_num) > 0) {
			uint32_t j = 0;
			printf("%s,%d,%d\n", __func__, __LINE__, entitle_num);
			for ( j = 0; j < entitle_num.num; j++) {
				GxCas_AbvEntitleInfo entitle_info = {0};

				entitle_info.ppid_no = provider_info.ppid_no;
				entitle_info.entitle_no = j;
				entitle_info.type = entitle_num.type;
				GxCas_Get(GXCAS_ABV_GET_ENTITLE_INFO, &entitle_info);
			}
		}


		//PPC entitle
		memset(&entitle_num, 0, sizeof(GxCas_AbvEntitleNum));
		entitle_num.ppid_no = provider_info.ppid_no;
		entitle_num.type = PPID_PPC_ENTITLE;
		if (GxCas_Get(GXCAS_ABV_GET_ENTITLE_NUM, &entitle_num) > 0) {
			uint32_t j = 0;
			printf("%s,%d,%d\n", __func__, __LINE__, entitle_num.num);
			for ( j = 0; j < entitle_num.num; j++) {
				GxCas_AbvEntitleInfo entitle_info = {0};

				entitle_info.ppid_no = provider_info.ppid_no;
				entitle_info.entitle_no = j;
				entitle_info.type = entitle_num.type;
				GxCas_Get(GXCAS_ABV_GET_ENTITLE_INFO, &entitle_info);
			}
		}

		//PPV entitle
		memset(&entitle_num, 0, sizeof(GxCas_AbvEntitleNum));
		entitle_num.ppid_no = provider_info.ppid_no;
		entitle_num.type = PPID_PPV_ENTITLE;
		if (GxCas_Get(GXCAS_ABV_GET_ENTITLE_NUM, &entitle_num) > 0) {
			printf("%s,%d,%d\n", __func__, __LINE__, entitle_num);
			uint32_t j = 0;
			for ( j = 0; j < entitle_num.num; j++) {
				GxCas_AbvEntitleInfo entitle_info = {0};

				entitle_info.ppid_no = provider_info.ppid_no;
				entitle_info.entitle_no = j;
				entitle_info.type = entitle_num.type;
				GxCas_Get(GXCAS_ABV_GET_ENTITLE_INFO, &entitle_info);
			}
		}
	}
}

static void _get_stbinfo_menu(void)
{
	//OTA Info
	ABV_Ota_info_t ota_info = {0};
	GxCas_Get(GXCAS_ABV_GET_OTA_INFO, &ota_info);


	//Serie Number
	GxCas_AbvSerieNumber serie_number = {0};
	GxCas_Get(GXCAS_ABV_GET_SERIE_NUMBER, &serie_number);
	
	//STBID
	int32_t i = 0;
	ABV_UInt8 stbid[20];
	memset(stbid, 0, sizeof(stbid));
	GxCas_Get(GXCAS_ABV_GET_STBID, stbid);
	printf("stbid:");
	for (i = 0; i < 20; i++)
		printf("%d,", stbid[i]);
	printf("\n");

	//CALIB version
	GxCas_AbvLibInfo lib_info = {0};
	GxCas_Get(GXCAS_ABV_GET_LIB_INFO, &lib_info);
}

static void _get_email_menu()
{
	ABV_UInt8 mail_count = 0, max_support_count = 0, new_mail_flag = 0;
	uint32_t i = 0;
	
	GxCas_Get(GXCAS_ABV_GET_EMAIL_COUNT, &mail_count);
	GxCas_Get(GXCAS_ABV_GET_EMAIL_MAXNUM, &max_support_count);
	GxCas_Get(GXCAS_ABV_GET_NEW_EMAIL, &new_mail_flag);
	for (i = 0; i < mail_count; i++) {
		GxCas_AbvMailHeadInfo head = {0};
		GxCas_AbvMailContent content = {0};

		head.num = i;
		if (GxCas_Get(GXCAS_ABV_GET_EMAIL_HEAD, &head) != ABV_FALSE)
			printf("ABV TEST Get Email Head Failed\n");

		content.msgid = head.head.mail_Mesg_id;
		if (GxCas_Get(GXCAS_ABV_GET_EMAIL_CONTENT, &content) != ABV_FALSE)
			printf("ABV TEST Get Email Content Failed\n");
		else
			printf("%d,%s\n", content.msgid, content.content);

		if (GxCas_Set(GXCAS_ABV_DELETE_EMAIL, &head.head.mail_Mesg_id) != ABV_FALSE)
			printf("ABV TEST Delete One Email Failed\n");
	}
	GxCas_Set(GXCAS_ABV_DELETE_ALL_EMAIL, NULL);
}

static void _get_subtitle()
{
	GxCas_AbvSubtitleInfo sub_info;

	if (GxCas_Get(GXCAS_ABV_GET_SUBTITLE_INFO, &sub_info) != ABV_FALSE)
		printf("%s,%d, Get Sub_info Failed\n", __func__, __LINE__);
	else
		printf("%s\n", sub_info.content);
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
		GxCore_GetTickTime(&abv_time);
		printf("%s,%d,%d,%d", __func__, __LINE__, abv_time.seconds, abv_time.microsecs);
		GxCas_Set(GXCAS_ABV_SWITCH_CHANNEL, (void *)&gxcas_switch);
		GxCas_TestCa_Play(playmap[pos]);
	}

	switch(type) {
		case GXCAS_TEST_KEY0:
			_get_ca_menu();
			break;
		case GXCAS_TEST_KEY1:
			_get_development_menu();
			break;
		case GXCAS_TEST_KEY2:
			_get_stbinfo_menu();
			break;
		case GXCAS_TEST_KEY3:
			_get_email_menu();
			break;
		case GXCAS_TEST_KEY4:
			_get_subtitle();
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
				Entitle_time_t time = {0};
				GxCas_Get(GXCAS_ABV_GET_CA_TIME, &time);
				break;
			}
		default:
			break;
	}
}
