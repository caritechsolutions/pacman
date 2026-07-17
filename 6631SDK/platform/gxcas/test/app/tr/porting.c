#include "time.h"
#include "gxcore.h"
#include "porting.h"
#include "gxcas/tr_api.h"
#include "gxcas/gxcas.h"
//#include "frontend.h"
#include "fcntl.h"
//#include "tuner.h"
//#include "demod.h"


#if 1
#define PROGRAM_NUM (3)
#define PLAY_PAR1	  "dvbs://vpid:572&apid:573&pcrpid:514&vcodec:0&acodec:1&tuner:0&scrable:1&pmt:34&tsid:0&dmxid:0&progid:3&pls_num:0"
#define PLAY_PAR2	  "dvbs://vpid:542&apid:543&pcrpid:514&vcodec:0&acodec:1&tuner:0&scrable:1&pmt:33&tsid:0&dmxid:0&progid:2&pls_num:0"
#define PLAY_PAR3	  "dvbs://vpid:512&apid:513&pcrpid:514&vcodec:0&acodec:1&tuner:0&scrable:1&pmt:32&tsid:0&dmxid:0&progid:1&pls_num:0"

//#define PLAY_PAR4	  "dvbs://vpid:125&apid:225&pcrpid:125&vcodec:0&acodec:1&tuner:0&scrable:0&pmt:1125&tsid:0&dmxid:0&progid:1&pls_num:0"
//#define PLAY_PAR5	  "dvbs://vpid:127&apid:227&pcrpid:127&vcodec:0&acodec:1&tuner:0&scrable:0&pmt:1127&tsid:0&dmxid:0&progid:2&pls_num:0"


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
	//strcpy(param, "dvbs://fre:1935&polar:0&symbol:44999&22k:1&tuner:0&tsid:0");
	strcpy(param, "dvbs://fre:1350&polar0&symbol:27500&22k:1&tuner:0&tsid:0");

}

void _menu_get_baseinfo(void)
{
	int32_t ret = 0;
	uint8_t verStr[26] = {0};
	GxCas_TrGetWatchLevel	watchLevel;

	//version string
	ret = GxCas_Get(GXCAS_TR_GET_VER_STR, verStr);

	ret = GxCas_Get(GXCAS_TR_GET_WATCH_LEVEL, &watchLevel);
}

void _menu_set_baseinfo(uint16_t service_id)
{
	int32_t ret = 0;	
	GxCas_TrSetWatchLevel	watchLevel;
	GxCas_TrParentalCtrlUnlock	parentCtrlUnlock;
	int16_t		regionCode;

	watchLevel.level = 0;
	memset(watchLevel.pin, 0, 6);
	ret = GxCas_Set(GXCAS_TR_SET_WATCH_LEVEL, (void *)&watchLevel);
	
	memset(parentCtrlUnlock.pin, 0, 6);
	ret = GxCas_Set(GXCAS_TR_PARENTAL_CTRL_UNLOCK, (void *)&parentCtrlUnlock);

	regionCode = 0;
	ret = GxCas_Set(GXCAS_TR_SET_REGION_CODE, (void *)&regionCode);
	printf("====@====%s,%d,\n", __FUNCTION__, __LINE__);
}

void _menu_get_entitle(void)
{
	int32_t ret = 0;
	GxCas_TrProductType_e productType;
	GxCas_TrEPurseInfo	purseInfo;
	GxCas_TrIppProduct	ippProduct;
	GxCas_TrIppRecord	ippRecord;

	//product type
	ret = GxCas_Get(GXCAS_TR_GET_PRODUCT_TYPE, &productType);

	printf("-------GXCAS_TR_GET_PRODUCT_TYPE, productType:[%d], ret:%d\n", productType, ret);
	purseInfo.bIndex = 0;
	ret = GxCas_Get(GXCAS_TR_GET_EPURSE_INFO, &purseInfo);
	printf("-------GXCAS_TR_GET_PRODUCT_TYPE, purseInfo:[%d, %d, %d], ret:%d\n", purseInfo.epurse_info.ulCashValue, purseInfo.epurse_info.ulCreditValue, purseInfo.epurse_info.wRecordIndex, ret);

	ippProduct.wIndex = 0;
	ret = GxCas_Get(GXCAS_TR_GET_IPP_PRODUCT, &ippProduct);
	printf("-------GXCAS_TR_GET_IPP_PRODUCT, ippProduct:[%d,%d, %02d%02d.%d.%d %d:%d:%d~~~~%02d%02d.%d.%d %d:%d:%d], ret:%d\n", ippProduct.Product.bProductType, ippProduct.Product.ulRunningNum,
		ippProduct.Product.ulStartTime.bYear[0], ippProduct.Product.ulStartTime.bYear[1], ippProduct.Product.ulStartTime.bMonth, ippProduct.Product.ulStartTime.bDay, ippProduct.Product.ulStartTime.bHour, ippProduct.Product.ulStartTime.bMinute, ippProduct.Product.ulStartTime.bSecond,
		ippProduct.Product.ulEndTime.bYear[0], ippProduct.Product.ulEndTime.bYear[1], ippProduct.Product.ulEndTime.bMonth, ippProduct.Product.ulEndTime.bDay, ippProduct.Product.ulEndTime.bHour, ippProduct.Product.ulEndTime.bMinute, ippProduct.Product.ulEndTime.bSecond, ret);
	
	ippRecord.wIndex = 0;
	ret = GxCas_Get(GXCAS_TR_GET_IPP_RECORD, &ippRecord);
	printf("-------GXCAS_TR_GET_IPP_RECORD, ippRecord:%d,(%02d%02d.%d.%d %d:%d:%d), %d, %d,%d,%s, ret:%d\n", ippRecord.recordData.bStateFlag, 
		ippRecord.recordData.ulExchTime.bYear[0], ippRecord.recordData.ulExchTime.bYear[1], ippRecord.recordData.ulExchTime.bMonth, ippRecord.recordData.ulExchTime.bDay, ippRecord.recordData.ulExchTime.bHour, ippRecord.recordData.ulExchTime.bMinute, ippRecord.recordData.ulExchTime.bSecond,
		ippRecord.recordData.ulExchRunningNum, ippRecord.recordData.ulExchValue,ippRecord.recordData.bContentLen,ippRecord.recordData.pbContent, ret);
	printf("========%s,%d,\n", __FUNCTION__, __LINE__);
	
}

void _menu_set_entitle(void)
{
	int32_t ret = 0;
	GxCas_TrIPPPurchase	ippPurchase;
	//GxCas_TrScWorkTable	scWorkTable;
	//GxCas_TrFeedData	feedData;

	memset(ippPurchase.pin, 0, 6);
	ret = GxCas_Set(GXCAS_TR_IPP_PURCHASE, (void *)&ippPurchase);

	//ret = GxCas_Set(GXCAS_TR_SET_SC_WORKTABLE, (void *)&scWorkTable);


	//ret = GxCas_Set(GXCAS_TR_SET_FEED_DATA, (void *)&feedData);
	printf("========%s,%d,ret:%d\n", __FUNCTION__, __LINE__, ret);
}

void _menu_get_misc(void)
{
	int32_t ret = 0;
	GxCas_TrCheckPin	checkPin;
	
	memset(checkPin.pin, 0, 6);
	ret = GxCas_Get(GXCAS_TR_CHECK_PIN, &checkPin);
	printf("========%s,%d, result:%d ret:%d\n", __FUNCTION__, __LINE__,checkPin.result ,ret);
}

void _menu_set_misc(void)
{
	int32_t ret = 0;
	GxCas_TrSetPin	setPin;
	
	memset(setPin.old_pin, 0, 6);
	memcpy(setPin.new_pin, "123456", 6);
	ret = GxCas_Set(GXCAS_TR_SET_PIN, (void *)&setPin);
	printf("========%s,%d,result:%d\n", __FUNCTION__, __LINE__, setPin.result);
}


void _menu_reset_pin(void)
{
	int32_t ret = 0;
	GxCas_TrSetPin	setPin;
	
	memcpy(setPin.old_pin, "123456", 6);
	memset(setPin.new_pin, 0, 6);
	ret = GxCas_Set(GXCAS_TR_SET_PIN, (void *)&setPin);
	printf("========%s,%d,result:%d\n", __FUNCTION__, __LINE__, setPin.result);
}


static void tr_event(void *arg)
{
	while(1) {
		unsigned long type = 0;
		void *p = NULL;
		GxCas_WaitEvent(&type, &p);
		printf("%s,%d,event type :%d\n", __func__, __LINE__, type);
		switch(type)
		{

			case GXCAS_TR_SERVICE_RATING:
				{
					printf("[tr service rating notify] rating = %d\n",*(uint16_t*)p);
					break;
				}
			
			case GXCAS_TR_SC_ACCESS:
				{
					printf("[tr sc access notify], CAS_STATE_INFO status:%d\n", *(uint16_t*)p);
					break;
				}
			
			case GXCAS_TR_IPP_NOTIFY:
				{
					printf("[IPP_NOTIFY notify], wState:%d\n", *(uint16_t*)p);
					break;
				}
			
			case GXCAS_TR_IPP_INFO_UPDATE:
				{
					GxCas_TrIppNotify *info = (GxCas_TrIppNotify *)p;
					printf("[IPP_INFO_UPDATE notify]\n");
					printf("info:\n service id:%d,\n bIppType:%d\n ulIppCharge:%d\n ulIppUnitTime:%d\n ulIppRunningNum:%d\n ulIppStart[%d%d.%d.%d %d:%d:%d]\n ulIppEnd[%d%d.%d.%d %d:%d:%d]\n bContentLen:%d\n, pbContent:%s\n", info->ippInfo.wChannelId,
						info->ippInfo.bIppType,
						info->ippInfo.ulIppCharge,
						info->ippInfo.ulIppUnitTime,
						info->ippInfo.ulIppRunningNum,
						info->ippInfo.ulIppStart.bYear[0], info->ippInfo.ulIppStart.bYear[1], info->ippInfo.ulIppStart.bMonth, info->ippInfo.ulIppStart.bDay, info->ippInfo.ulIppStart.bHour, info->ippInfo.ulIppStart.bMinute, info->ippInfo.ulIppStart.bSecond,
						info->ippInfo.ulIppEnd.bYear[0], info->ippInfo.ulIppEnd.bYear[1], info->ippInfo.ulIppEnd.bMonth, info->ippInfo.ulIppEnd.bDay, info->ippInfo.ulIppEnd.bHour, info->ippInfo.ulIppEnd.bMinute, info->ippInfo.ulIppEnd.bSecond,
						info->ippInfo.bContentLen,
						info->ippInfo.pbContent);
					break;
				}
			
			case GXCAS_TR_EMAIL_NOTIFY:
				{
					//GxCas_TrShortMsg *msg = (GxCas_TrShortMsg*)p;
					printf("[GXCAS_TR_EMAIL_NOTIFY]\n");
					/*printf("short msg info:\n wTitleLen:%d\n bMsgTitle:%s\n wDataLen:%d\n bMsgData:%s\n wIndex:%d\n bType:%d\n bClass:%d\n bPriority:%d\n wPeriod:%d\n sMsgTime:[%d%d.%d.%d %d:%d:%d]\n sCreateTime:[%d%d.%d.%d %d:%d:%d]\n ",
					msg->msg.wTitleLen,
					msg->msg.bMsgTitle,
					msg->msg.wDataLen,
					msg->msg.bMsgData,
					msg->msg.wIndex,
					msg->msg.bType,
					msg->msg.bClass,
					msg->msg.bPriority,
					msg->msg.wPeriod,
					msg->msg.sMsgTime.bYear[0], msg->msg.sMsgTime.bYear[1], msg->msg.sMsgTime.bMonth, msg->msg.sMsgTime.bDay, msg->msg.sMsgTime.bHour, msg->msg.sMsgTime.bMinute, msg->msg.sMsgTime.bSecond,
					msg->msg.sCreateTime.bYear[0], msg->msg.sCreateTime.bYear[1], msg->msg.sCreateTime.bMonth, msg->msg.sCreateTime.bDay,msg->msg.sCreateTime.bHour, msg->msg.sCreateTime.bMinute, msg->msg.sCreateTime.bSecond);*/
					break;
				}
							
			case GXCAS_TR_FORCE_EMAIL:
				{
					//GxCas_TrEnShortMsg *enMsg = (GxCas_TrEnShortMsg *)p;
					printf("[EN_SHORT_MSG notify]\n");
					/*printf("en short msg info:\n sCreateTime:[%d%d.%d.%d %d:%d:%d]\n wIndex:%d\n bType:%d\n bClass:%d\n bPriority:%d\n bBlockFlag:%d\n dwRemainTime:%d\n dwPeriod:%d\n bXPos:%d\n bYPos:%d\n bWidth:%d\n bHeight:%d\n bTitleLen:%d\n MsgTitle:%s\n wDataLen:%d\n MsgData:%s\n ",
					enMsg->msg.sCreateTime.bYear[0], enMsg->msg.sCreateTime.bYear[1], enMsg->msg.sCreateTime.bMonth, enMsg->msg.sCreateTime.bDay, enMsg->msg.sCreateTime.bHour, enMsg->msg.sCreateTime.bMinute, enMsg->msg.sCreateTime.bSecond,
					enMsg->msg.wIndex,
					enMsg->msg.bType,
					enMsg->msg.bClass,
					enMsg->msg.bPriority,
					enMsg->msg.bBlockFlag,
					enMsg->msg.dwRemainTime,
					enMsg->msg.dwPeriod,
					enMsg->msg.bXPos,
					enMsg->msg.bYPos,
					enMsg->msg.bWidth,
					enMsg->msg.bHeight,
					enMsg->msg.bTitleLen,
					enMsg->msg.MsgTitle,
					enMsg->msg.wDataLen,
					enMsg->msg.MsgData);*/
					break;
				}
							
			case GXCAS_TR_ENHANCED_FP:
				{
					GxCas_TrEnFP * fp = (GxCas_TrEnFP *)p;
					
					printf("[ENHANCED_FP notify]\n");
					printf("enhanced fp info\n sActivateTime:[%d%d.%d.%d %d:%d:%d]\n dwOnTime:%d\n dwOffTime:%d\n bRepeatNum:%d\n bFontSize:%d\n bXPos:%d\n bYPos:%d\n bWidth:%d\n bHeight:%d\n bTextColorRed:%d\n bTextColorGreen:%d\n bTextColorBlue:%d\n bTextOpacity:%d\n bBackgColorRed:%d\n bBackgColorGreen:%d\n bBackgColorBlue:%d\n bBackgOpacity:%d\n bBlockFlag:%d\n bDispType:%d\n bDispScNum:%d\n bDispText:%d\n bScNumLen:%d\n ScNumString:%s\n bTextLen:%d\n TextString:%s\n",
					fp->enFp.sActivateTime.bYear[0], fp->enFp.sActivateTime.bYear[1], fp->enFp.sActivateTime.bMonth, fp->enFp.sActivateTime.bDay, fp->enFp.sActivateTime.bHour, fp->enFp.sActivateTime.bMinute, fp->enFp.sActivateTime.bSecond,
					fp->enFp.dwOnTime,
					fp->enFp.dwOffTime,
					fp->enFp.bRepeatNum,
					fp->enFp.bFontSize,
					fp->enFp.bXPos,
					fp->enFp.bYPos,
					fp->enFp.bWidth,
					fp->enFp.bHeight,
					fp->enFp.bTextColorRed,
					fp->enFp.bTextColorGreen,
					fp->enFp.bTextColorBlue,
					fp->enFp.bTextOpacity,
					fp->enFp.bBackgColorRed,
					fp->enFp.bBackgColorGreen,
					fp->enFp.bBackgColorBlue,
					fp->enFp.bBackgOpacity,
					fp->enFp.bBlockFlag,
					fp->enFp.bDispType,
					fp->enFp.bDispScNum,
					fp->enFp.bDispText,
					fp->enFp.bScNumLen,
					fp->enFp.ScNumString,
					fp->enFp.bTextLen,
					fp->enFp.TextString);
					break;
				}
							
			case GXCAS_TR_FORCE_CHANNEL:
				{
					GxCas_TrForceCh * fChannel = (GxCas_TrForceCh *)p;
					printf("[FORCE_CHANNEL notify]\n");
					printf("force channel info\n bLockFlag:%d\n wNetwrokId:%d\n wTsId:%d\n wServId:%d\n wContentLen:%d\n pbContent:%s\n",
						fChannel->forceCh.bLockFlag,
						fChannel->forceCh.wNetwrokId,
						fChannel->forceCh.wTsId,
						fChannel->forceCh.wServId,
						fChannel->forceCh.wContentLen,
						fChannel->forceCh.pbContent);
					break;
				}
							
			case GXCAS_TR_FINGER_PRINT:
				{
					GxCas_TrFP * fingerp = (GxCas_TrFP *)p;
					printf("[FINGER_PRINT notify]\n");
					printf("finger pint info:\n bHashedNumLen:%d\n pbHashedNum:%s\n bContentLen:%d\n pbContentData:%s\n",
					fingerp->fp.bHashedNumLen,
					fingerp->fp.pbHashedNum,
					fingerp->fp.bContentLen,
					fingerp->fp.pbContentData);
					break;
				}
							
			case GXCAS_TR_CURRENT_STATE:
				{
					printf("[CURRENT_STATE notify], status:%d\n", *(uint16_t*)p);
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
//	uint8_t key_data[16] = {0xFE,0xDC,0xBA,0x98,0x76,0x54,0x32,0x10,0x01,0x23,0x45,0x67,0x89,0xAB,0xCD,0xEF};
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);

	init.dmx_id = 0;
	init.ts_id = 0;
	//strcpy(init.flash.name, "/home/gx/gxca_nvram.dat");
#if 1	
	init.flash.offset = 0;
	init.flash.size = 64*1024;
	strcpy(init.flash.name, "PRIVATE");
//	init.backup.offset = 0;
//	init.backup.size = 64*1024;
//	strcpy(init.backup.name, "BACKUP");
	init.sci.sci_switch = 0;
	GxCas_TrHsInit(init);
#else
	init.sci.sci_switch = 0;
	init.flash.offset = 0;
	init.flash.size = 64*1024;
	strcpy(init.flash.name, "PRIVATE");
	init.backup.offset = 0;
	init.backup.size = 64*1024;
	strcpy(init.backup.name, "BACKUP");
	GxCas_TrHsInit(init);
#endif
    GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
	GxCore_ThreadCreate("tr_wait_event", &handle, tr_event, NULL,30*1024, GXOS_DEFAULT_PRIORITY);
}

void gxCas_CaTest_switchChannel(uint16_t pmt_pid,uint16_t service_id,
	uint16_t audio_pid,	uint16_t video_pid)
{
	GxCasSet_SwitchChannel gxcas_switch = {0};
	
	gxcas_switch.pmt_pid = pmt_pid;
	gxcas_switch.service_id = service_id;
	gxcas_switch.audio_pid = audio_pid;
	gxcas_switch.video_pid = video_pid;
	printf("====%s,%d, [pmtPid:%d,serviceId:%d,videoPid:%d,audioPid:%d]\n", __FUNCTION__, __LINE__, pmt_pid, service_id, video_pid, audio_pid);
	GxCas_Set(GXCAS_TR_SWITCH_CHANNEL, (void *)&gxcas_switch);
}

void gxCas_CaTest_switchFreq(void)
{	
	printf("====%s,%d\n", __FUNCTION__, __LINE__);
	GxCas_Set(GXCAS_TR_SWITCH_FREQ, NULL);
}

void gxCas_CaTest_stop(void)
{	
	printf("====%s,%d\n", __FUNCTION__, __LINE__);
	GxCas_Set(GXCAS_TR_STOP_CA, NULL);
}


void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap type)
{
	static int32_t pos = 0;
	static int32_t service_id = 0, pmt_pid = 0, a_pid = 0, v_pid = 0;
	char* playmap[PROGRAM_NUM] = {PLAY_PAR1, PLAY_PAR2, PLAY_PAR3/*, PLAY_PAR4, PLAY_PAR5*/};

	if (type == GXCAS_TEST_KEYUP) {
		pos++;
		if (pos == sizeof(playmap)/sizeof(char *))
			pos = 0;
	} else if (type == GXCAS_TEST_KEYDOWN) {
		pos--;
		if (pos < 0)
			pos = (sizeof(playmap)/sizeof(char *)) - 1;
	}

	if (type == GXCAS_TEST_PLAY)
	{		
		GxCas_Set(GXCAS_TR_SWITCH_FREQ, NULL);
	}

	if ((type == GXCAS_TEST_KEYUP) || (type == GXCAS_TEST_KEYDOWN) || (type == GXCAS_TEST_PLAY)) {
		GxCasSet_SwitchChannel gxcas_switch = {0};
		GxUrl_GetItem(playmap[pos],"pmt", &pmt_pid);
		GxUrl_GetItem(playmap[pos],"progid", &service_id);
		GxUrl_GetItem(playmap[pos],"apid", &a_pid);
		GxUrl_GetItem(playmap[pos],"vpid", &v_pid);
		gxcas_switch.pmt_pid = pmt_pid;
		gxcas_switch.service_id = service_id;
		gxcas_switch.audio_pid = a_pid;
		gxcas_switch.video_pid = v_pid;
		printf("====SWITCH_CHANNEL, pmtPid:0x%x, serviceId:%d, v_pid:0x%x, a_pid:0x%x\n", pmt_pid, service_id, v_pid, a_pid);
		GxCas_Set(GXCAS_TR_SWITCH_FREQ, NULL);
		GxCas_Set(GXCAS_TR_SWITCH_CHANNEL, (void *)&gxcas_switch);
		GxCas_TestCa_Play(playmap[pos]);
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
			_menu_reset_pin();
			break;

		default:
			break;
	}
}
