#include "time.h"
#include "gxcore.h"
#include "porting.h"
#include "gxcas/uti_api.h"
#include "gxcas/gxcas.h"
//#include "frontend.h"
#include "fcntl.h"
//#include "tuner.h"
//#include "demod.h"

#if 0
#define PROGRAM_NUM (7)
#define PLAY_PAR1	  "dvbc://vpid:512&apid:650&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:257&tsid:0&dmxid:0&service_id:301"
#define PLAY_PAR2	  "dvbc://vpid:513&apid:660&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:258&tsid:0&dmxid:0&service_id:302"
#define PLAY_PAR3	  "dvbc://vpid:514&apid:670&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:259&tsid:0&dmxid:0&service_id:303"
#define PLAY_PAR4	  "dvbc://vpid:515&apid:680&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:260&tsid:0&dmxid:0&service_id:304"
#define PLAY_PAR5	  "dvbc://vpid:516&apid:690&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:261&tsid:0&dmxid:0&service_id:305"
#define PLAY_PAR6	  "dvbc://vpid:517&apid:700&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:262&tsid:0&dmxid:0&service_id:306"
#define PLAY_PAR7	  "dvbc://vpid:518&apid:710&pcrpid:8190&vcodec:0&acodec:1&tuner:0&scramble:0&pmt:263&tsid:0&dmxid:0&service_id:307"
#endif
#if 1
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
#ifdef ECOS_OS	
	//frontend_mod_init_gx1801(1, "|0:0:0xe4:62:0:0xc0:&0:0");//3201h
	frontend_mod_init_gx3211(1, "|1:0:0xe4:55:0:0x18:&1:1:2:0:0:1");//6605s rda5815M
	//frontend_mod_init_gx3211(1, "|1:0:0xe4:41:0:0xc0:&0:1:2:0:0:1");//6605s av2011
#endif
#if 0
	//frontend_mod_init_atbm888x(1, "|7:0:0x80:62:0:0xc0:&0:0");//3212
	struct dev_attach dev;


	dev.demod_dev.id = 0;
	dev.demod_dev.addr = 0xe4;
	dev.demod_attach = GX3211_ATTACH;
	dev.tuner_dev.id = 0;
	dev.tuner_dev.addr = 0x18;
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
	//strcpy(param, "dvbc://fre:714000&symbol:6875000&bandwidth:0&qam:3&tuner:0&tsid:0");//3201h
	//strcpy(param, "dtmb://fre:714000&bandwidth:0&qam:3&tuner:0&tsid:1"); //3212
	strcpy(param, "dvbs://fre:1150&polar:1&symbol:27500&22k:1&tuner:0&tsid:0");
}

void _menu_get_baseinfo(void)
{
	int32_t ret = 0;
	GxCas_UtiGetWatchLevel watch_level;
	GxCas_UtiGetSerialNumber serial_num;
	GxCas_UtiGetAreaId area_id;
	GxCas_UtiGetBusinessId business_id;
	uint8_t build_info[129];
	//watch level
	watch_level.index_of_sector = 0;
	ret = GxCas_Get(GXCAS_UTI_GET_WATCH_LEVEL, &watch_level);

	//serial number
	ret = GxCas_Get(GXCAS_UTI_GET_USER_SERIAL_NUMBER, &serial_num);

	//area_id
	area_id.index_of_sector = 0;
	ret = GxCas_Get(GXCAS_UTI_GET_AREA_ID, &area_id);
	
	//business_id
	business_id.index_of_sector = 0;
	ret = GxCas_Get(GXCAS_UTI_GET_BUSINESS_ID, &business_id);
	
	//business_id
	memset(build_info, 0, 129);
	ret = GxCas_Get(GXCAS_UTI_GET_BUILD_INFO, build_info);
}

void _menu_set_baseinfo(uint16_t service_id)
{
	int32_t ret = 0;	
	GxCas_UtiSetWatchLevel watch_level;
	GxCas_UtiWatchLevelDisTmp view_level_temp;
	//watch level
	watch_level.index_of_sector = 0;
	watch_level.level = 12;
	memcpy(watch_level.pin, "123456",6);
	watch_level.result = 0;
	ret = GxCas_Set(GXCAS_UTI_SET_WATCH_LEVEL, (void *)&watch_level);
	
	//watch level
	view_level_temp.tuner_id = 0;
	view_level_temp.service_id = service_id;
	memcpy(view_level_temp.pin, "123456",6);
	view_level_temp.result = 0;
	ret = GxCas_Set(GXCAS_UTI_WATCH_LEVEL_DISABLE_TEMP, (void *)&view_level_temp);
}

void _menu_get_entitle(void)
{
	int32_t ret = 0;
	uint32_t sector_num = 0;
	GxCas_UtiProductNum product_num;
	GxCas_UtiProductInfo product_info;
	GxCas_UtiGetBusinessId business_id;
	//sector number
	ret = GxCas_Get(GXCAS_UTI_GET_SECTOR_NUM, &sector_num);

	//product number
	product_num.index_of_sector = 0;
	ret = GxCas_Get(GXCAS_UTI_GET_PRODUCT_NUM, &product_num);

	//product info
	product_info.index_of_sector = 0;
	product_info.index_of_product = 0;
	ret = GxCas_Get(GXCAS_UTI_GET_PRODUCT_INFO, &product_info);	
}

void _menu_set_entitle(void)
{
	int32_t ret = 0;
	uint32_t pre_auth_days = 7;
	//pre auth date
	ret = GxCas_Set(GXCAS_UTI_SET_PRE_AUTH_DATE, (void *)&pre_auth_days);
}

void _menu_get_misc(void)
{
	int32_t ret = 0;
	GxCas_UtiCheckPin check_pin;
	//check pin
	check_pin.index_of_sector = 0;
	memcpy(check_pin.pin, "123456",6);
	check_pin.result = 0;
	ret = GxCas_Get(GXCAS_UTI_CHECK_PIN, &check_pin);
}

void _menu_set_misc(void)
{
	int32_t ret = 0;
	GxCas_UtiSetPin set_pin;
	//check pin
	set_pin.index_of_sector = 0;
	memcpy(set_pin.old_pin, "123456",6);
	memcpy(set_pin.new_pin, "111111",6);
	set_pin.result = 0;
	ret = GxCas_Set(GXCAS_UTI_SET_PIN, &set_pin);
}

void _menu_get_email(void)
{
	int32_t ret = 0;
	GxCas_UtiEmailCount email_count;
	GxCas_UtiEmailHead email_head;
	UtiEmailHeadInfo_t* head_info = NULL;
	GxCas_UtiEmailInfo email_info;
	//email number
	ret = GxCas_Get(GXCAS_UTI_GET_EMAIL_COUNT, &email_count);

	//email head
	if( email_count.total_num )
	{
		email_head.total_count = email_count.total_num;	
		head_info = GxCore_Malloc(sizeof(UtiEmailHeadInfo_t)*email_head.total_count);
		email_head.head_info = head_info;
		ret = GxCas_Get(GXCAS_UTI_GET_EMAIL_HEAD, &email_head);
		GxCore_Free(head_info);
	}

	//email info
	ret = GxCas_Get(GXCAS_UTI_GET_EMAIL_INFO, &email_info);	
}

void _menu_set_email(void)
{
	int32_t ret = 0;
	uint8_t mail_pos = 0;
	//del email
	ret = GxCas_Set(GXCAS_UTI_DELETE_EMAIL, (void *)&mail_pos);
}

static void uti_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		GxCas_WaitEvent(&type, &p);
		//printf("%s,%d,service_id :%d, error type:%d\n", __func__, __LINE__, err_info->ServiceID, err_info->ca_error_type);
		switch(type)
		{
			case GXCAS_UTI_EMAIL_NOTIFY:
				{
					GxCas_UtiEmailNotify *info = (GxCas_UtiEmailNotify*)p;
					printf("[uti email notify] status = %d\n",info->status);
					break;
				}
			case GXCAS_UTI_SHOW_FOCUS_EMAIL:
				{
					struct tm *local;
					GxCas_UtiEmailInfo *info = (GxCas_UtiEmailInfo*)p;
					local=localtime((time_t *)&(info->email.email.utc));
					printf("[uti show focus email] display_type = %d\n",info->email.email.displayType);
					printf("[utc time] utc data = %d-%d-%d %d:%d:%d", 
							1900 + local->tm_year, 1+ local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
					printf("[title]: %s\n",info->email.email.title);
					printf("[content]: %s\n",info->email.email.content);
					break;
				}
			case GXCAS_UTI_OSD_NOTIFY:
				{
					struct tm *local;
					GxCas_UtiOSDInfo *info = (GxCas_UtiOSDInfo*)p;
					local=localtime((time_t *)&(info->osd_info.utc));
					printf("[uti osd notify] display_type = %d\n",info->osd_info.displayType);
					printf("[duration] duration = %d\n",info->osd_info.during);
					printf("[utc time] utc data = %d-%d-%d %d:%d:%d", 
							1900 + local->tm_year, 1+ local->tm_mon, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec);
					printf("[content]: %s\n",info->osd_info.content);

					break;
				}
			case GXCAS_UTI_SERVICE_NOTIFY:
				{
					GxCas_UtiServiceInfo *info = (GxCas_UtiServiceInfo*)p;
					printf("[uti service notify]\n tuner id = %d\n sevice id = %d\n sector index = %d\n error index = %d\n",
							info->service_info.tunerID,info->service_info.serviceId,info->service_info.sectorIndex,info->service_info.stringErrorIndex);
					break;
				}
			case GXCAS_UTI_FINGER_NOTIFY:
				{
					GxCas_UtiFingerInfo *info = (GxCas_UtiFingerInfo*)p;
					printf("[uti finger notify]\n status = %d\n serial number = %s",info->tuner_id,info->string);
					break;
				}
			case GXCAS_UTI_HIDE_FINGER:
				{
					printf("[uti hide finger]\n");
					break;
				}
			case GXCAS_UTI_PPC_NOTIFY:
				{
					GxCas_UtiPPCInfo *info = (GxCas_UtiPPCInfo*)p;
					printf("[uti ppc notify]\n product number = %d\n",info->ppc_info.u8NumberOfProduct);
					break;
				}
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
	GxCas_UtiSetStbInfo stb_info = {0};
	uint8_t key_data[16] = {0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};

	init.dmx_id = 0;
	init.ts_id = 0;
	//strcpy(init.flash.name, "/home/gx/gxca_nvram.dat");
#if 1	
	strcpy(init.flash.name, "CA");
	init.flash.size = 64*1024;
	init.flash.offset = 0;
	init.sci.sci_switch = 0;
	GxCas_UtiHsInit(init);
#else
	init.sci.sci_switch = 0;
	init.flash.offset = 0;
	init.flash.size = 64*1024;
	strcpy(init.flash.name, "PRIVATE");
	init.backup.offset = 0;
	init.backup.size = 64*1024;
	strcpy(init.backup.name, "BACKUP");
	GxCas_UtiHsInit(init);
#endif
	stb_info.soc_id = 0x6616;
	stb_info.manufacture_id = 0x1;
	stb_info.area = 0x856;
	stb_info.key = key_data;
	stb_info.key_len = 16;
	GxCas_Set(GXCAS_UTI_SET_STB_INFO, (void *)&stb_info);
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
    //GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
	GxCore_ThreadCreate("uti_wait_event", &handle, uti_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	static int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
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
		GxCas_Set(GXCAS_UTI_SWITCH_CHANNEL, (void *)&gxcas_switch);
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
				_menu_get_baseinfo();
				break;
			}
		case GXCAS_TEST_KEY1:
			{
				_menu_set_baseinfo(service_id);
				break;
			}
		case GXCAS_TEST_KEY2:
			_menu_get_entitle();
			break;
		case GXCAS_TEST_KEY3:
			_menu_set_entitle();
			break;
		case GXCAS_TEST_KEY4:
			_menu_get_misc();
			break;
		case GXCAS_TEST_KEY5:
			_menu_set_misc();
			break;
		case GXCAS_TEST_KEY6:
			_menu_get_email();
			break;
		case GXCAS_TEST_KEY7:
			_menu_set_email();
			break;
		case GXCAS_TEST_KEY8:
			//GxCas_Set(GXCAS_UTI_STOP_CA,NULL);
			break;
		case GXCAS_TEST_KEY9:
			{
			GxCas_Set(GXCAS_UTI_STOP_DESCRAMBLE,NULL);
				break;
			}
		default:
			break;
	}
}
