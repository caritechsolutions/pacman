#ifndef __CASCAM_DVTCAS_DATA_H__
#define __CASCAM_DVTCAS_DATA_H__
#include "gxtype.h"

#define CASCAM_DVTCAS_MAXLEN_MSG				1024// used for cascam event
#define CASCAM_DVTCAS_MAXNUM_OPERATOR		5
#define CASCAM_DVTCAS_MAXNUM_EMAIL			20
#define CASCAM_DVTCAS_MAXNUM_ENTITLE			250
//basicinfo
#define CASCAM_DVTCAS_MAXLEN_CARD_SN			17// ex. 8000302100000333
#define CASCAM_DVTCAS_MAXLEN_CARD_VER		11// ex. 0x12345678. max len is 10 bytes
#define CASCAM_DVTCAS_MAXLEN_RATING			3// ex. from 4 to 18
#define CASCAM_DVTCAS_MAXLEN_CHIP_ID			16// char chip id[13];//ex. 141D567A9127, platform id (4bytes)+ unique id(8bytes),CDCA_MAXLEN_STBSN
#define CASCAM_DVTCAS_MAXLEN_MANU_INFO		30// ex. 1, 2, 3, 4.
#define CASCAM_DVTCAS_MAXLEN_WORKTIME		20// ex. 00:00:00 - 23:59:59, (start[hh:mm:ss] - end[hh:mm:ss] )
#define CASCAM_DVTCAS_MAXLEN_PIN_STATUS	20// "Paired Ok" or "Not paired with any STB" or "Paired to other STB",if paired, need show paired stb id
#define CASCAM_DVTCAS_MAXLEN_AREA_CODE		4// ex. On or Off
#define CASCAM_DVTCAS_MAXLEN_SECURITY_START	4// ex. On or Off
#define CASCAM_DVTCAS_MAXLEN_MARKET_ID		11// ex. 0x83120095
//entitle info
#define CASCAM_DVTCAS_MAXLEN_PRODUCT_ID		11// ex. 1 ~ 4294967295
#define CASCAM_DVTCAS_MAXLEN_PRODUCT_TIME	11// ex. 2018-09-18
#define CASCAM_DVTCAS_MAXLEN_PRODUCT_NAME		21
//feature info
#define CASCAM_DVTCAS_MAXLEN_FEATURE_INFO	18
#define CASCAM_DVTCAS_MAXNUM_FEATURE_ID		7// pACArray[3]~pACArray[9] is feature id
#define CASCAM_DVTCAS_MAXNUM_FEATURE_RES		8// pACArray[10]~pACArray[17] is feature reserved, due to CDCA_MAXNUM_ACLIST
//rating
#define CASCAM_DVTCAS_MAXLEN_PIN_CODE		8
//parent child card
#define CASCAM_DVTCAS_MAXLEN_DATE			20// ex. 2018-09-19 15:40:15 (yyyy-mm-dd hh:mm:ss)
#define CASCAM_DVTCAS_MAXLEN_MAIL_NAME			30
//operator info
#define CASCAM_DVTCAS_MAXLEN_TVSINFO			33
//read status
#define CASCAM_DVTCAS_MAXLEN_READ_STATUS		7// ex. Read or Unread
#define CASCAM_DVTCAS_MAXNUM_DETITLE_CHK		5
//mail
#define CASCAM_DVTCAS_MAXLEN_MAIL_CONTENT	1001
#define CASCAM_DVTCAS_MAXLEN_MAIL_TITLE	21// (mode change)
//osd
#define CASCAM_DVTCAS_MAXLEN_OSD				261*3
#define CASCAM_DVTCAS_MAXLEN_ORIGIN_OSD		210
//finger
#define CASCAM_DVTCAS_MAXNUM_FINGER		20//  finger matrix
//ipp
#define CASCAM_DVTCAS_MAXNUM_IPP_PROG		250
#define CASCAM_DVTCAS_MAXLEN_SERVICE_NAME		21
//action request
#define CASCAM_DVTCAS_MAXLEN_DATA			(256-4)
//child-mother card
#define CASCAM_DVTCAS_MAXLEN_CORRESPOND_DATA			250
//enum
typedef enum{
	//get
	CASCAM_DVTCAS_GET_BASICINFO,//CascamDvtcasBasicInfo
	CASCAM_DVTCAS_GET_ENTITLE_INFO,//CascamDvtcasEntitleInfo
	CASCAM_DVTCAS_GET_FEATURE,//CascamDvtcasFeatureInfo
	CASCAM_DVTCAS_GET_RATING,//CascamDvtcasRating
	CASCAM_DVTCAS_GET_WORKTIME,//CascamDvtcasWorkTime
	CASCAM_DVTCAS_GET_OPERATOR_INFO,//CascamDvtcasOperatorInfo
	CASCAM_DVTCAS_GET_DETITLE,//CascamDvtcasDetitleInfo
	CASCAM_DVTCAS_GET_EMAIL_LIST,//CascamDvtcasMailList
	CASCAM_DVTCAS_GET_EMAIL_CONTENT,//CascamDvtcasMailContent
	CASCAM_DVTCAS_GET_WALLET_INFO,//CascamDvtcasWalletInfo
	CASCAM_DVTCAS_GET_IPP_INFO,//CascamDvtcasIppInfo
	CASCAM_DVTCAS_GET_VIEWED_IPP_INFO,//CascamDvtcasViewedIppInfo
	CASCAM_DVTCAS_GET_CORRESPOND_INFO,//CascamDvtcasCorrespondInfo
	CASCAM_DVTCAS_GET_MOTHER_INFO,//uint32_t
	//set
	CASCAM_DVTCAS_SET_RATING,//CascamDvtcasRating
	CASCAM_DVTCAS_SET_WORKTIME,//CascamDvtcasWorkTime
	CASCAM_DVTCAS_DELETE_DETITLE,//CascamDvtcasDetitleInfo
	CASCAM_DVTCAS_SET_PIN,//CascamDvtcasPin
	CASCAM_DVTCAS_CHECK_PIN,//CascamDvtCheckPin
	CASCAM_DVTCAS_DELETE_EMAIL,//CascamDvtcasMailDel
	CASCAM_DVTCAS_BUY_IPP,//CascamDvtcasIppInfo
	CASCAM_DVTCAS_SET_TIMEZONE,//double
	CASCAM_DVTCAS_START_CA,//NULL
	CASCAM_DVTCAS_STOP_CA,//NULL
	CASCAM_DVTCAS_STOP_DESCRAMBLE,//NULL
	CASCAM_DVTCAS_SWITCH_CHANNEL,//CascamDvtcasSwitchChannel
	CASCAM_DVTCAS_QUICK_SWITCH_CHANNEL,//CascamDvtcasQuickSwitchChannel
	CASCAM_DVTCAS_ADD_SERVICE,//CascamServiceInfo
	CASCAM_DVTCAS_DELETE_SERVICE,//CascamServiceInfo
	CASCAM_DVTCAS_SET_LOCK_SERVICE,//CascamServiceInfo
	CASCAM_DVTCAS_OSD_OVER,//NULL
	CASCAM_DVTCAS_SET_PDSDVALUE,//uint32_t
	CASCAM_DVTCAS_SET_CORRESPOND_INFO,//CascamDvtcasCorrespondInfo
	CASCAM_DVTCAS_INQUIRE_IPP_OVER,//uint16_t
	//queue
	CASCAM_DVTCAS_MSG_PDSDVALUE_CHANGE,//uint32_t pdsdvalue
	CASCAM_DVTCAS_MSG_EMAIL,//CascamDvtcasMailNotify
	CASCAM_DVTCAS_MSG_SHOW_MESSAGE,//CascamDvtcasShowMessage
	CASCAM_DVTCAS_MSG_HIDE_MESSAGE,//CascamDvtcasHideMessage
	CASCAM_DVTCAS_MSG_SHOW_FORCE_MAIL,//show force mail msg
	CASCAM_DVTCAS_MSG_HIDE_FORCE_MAIL,//hide force mail msg
	CASCAM_DVTCAS_MSG_LOCK_SERVICE,//CascamDvtcasLockService
	CASCAM_DVTCAS_MSG_URGENCYBROAD,//CascamDvtcasUrgencyBroadcast
	CASCAM_DVTCAS_MSG_OSD,//CascamDvtcasOsdInfo
	CASCAM_DVTCAS_MSG_FINGER,//CascamDvtcasFingerInfo
	CASCAM_DVTCAS_MSG_ADV_PREVIEW,//CascamDvtcasAdvPreviewNotify
	CASCAM_DVTCAS_MSG_ACTION_REQUEST,//CascamDvtcasActionRequest
	CASCAM_DVTCAS_MSG_DESCRAMBLE_SUCCESS,//NULL
	CASCAM_DVTCAS_MSG_CUR_ECM_PID,//CascamDvtcasCurEcmPid
	CASCAM_DVTCAS_MSG_CARD_IN,//NULL,//NULL
	CASCAM_DVTCAS_MSG_INQUIRE_IPP,//CascamDvtcasIppNotify
}CASCAM_DVTCAS_TYPE;

//struct
typedef struct{
	char card_sn[CASCAM_DVTCAS_MAXLEN_CARD_SN];
	char card_version[CASCAM_DVTCAS_MAXLEN_CARD_VER];
	char cas_manu_info[CASCAM_DVTCAS_MAXLEN_MANU_INFO];

	char area_code[CASCAM_DVTCAS_MAXLEN_AREA_CODE];
	char pin_status[CASCAM_DVTCAS_MAXLEN_PIN_STATUS];
	char rating[CASCAM_DVTCAS_MAXLEN_RATING];
	char work_time[CASCAM_DVTCAS_MAXLEN_WORKTIME];
	char chip_id[CASCAM_DVTCAS_MAXLEN_CHIP_ID];
}CascamDvtcasBasicInfo;

typedef struct{
	uint16_t 	m_wProductID;
	uint32_t	m_tEntitleTime;
	uint32_t	m_tStartTime;
	uint32_t	m_tEndTime;
	char	m_szProductName[CASCAM_DVTCAS_MAXLEN_PRODUCT_NAME];
}CascamDvtcasEntitleDetail;

typedef struct{
	uint16_t operator_id;// [in]
	uint16_t entitle_num;// [inout] 0, default max
	CascamDvtcasEntitleDetail *entitles;// due to CDCA_MAXNUM_ENTITLE
}CascamDvtcasEntitleInfo;

typedef struct{
	uint16_t operator_id;//[in]
	uint16_t feature_num;//[inout] 0, default max
	uint32_t area_code;//ACList[0]
	uint32_t bouquet_id;//ACList[1]
	uint32_t unknown;//ACList[2]
	uint32_t feature_id[CASCAM_DVTCAS_MAXNUM_FEATURE_ID];//ACList[3]~ACList[9]
	uint32_t feature_res[CASCAM_DVTCAS_MAXNUM_FEATURE_RES];//ACList[10]~ACList[17]
}CascamDvtcasFeatureInfo;

typedef struct{
	uint8_t pin[CASCAM_DVTCAS_MAXLEN_PIN_CODE];// [in]  0~9
	uint8_t watch_level;// [3-18]
}CascamDvtcasRating;

typedef struct{
	uint8_t pin[CASCAM_DVTCAS_MAXLEN_PIN_CODE];// [in]  0~9
	uint32_t start_time;// BCD code, ex. 0x235959 23:59:59 (hh:mm:ss)
	uint32_t end_time;// BCD code, ex. 0x235959 23:59:59 (hh:mm:ss)
}CascamDvtcasWorkTime;

typedef struct{
	uint16_t operator_id;// [in]
	uint8_t is_child;// [out] 0, parent or common card; 1, child card
	uint16_t delay_time;// [out] feed period time, only for child card
	char last_feed_time[CASCAM_DVTCAS_MAXLEN_DATE];// [out] only for child card
	char parent_card_sn[CASCAM_DVTCAS_MAXLEN_CARD_SN];// [out] only for child card
	uint8_t is_can_feed;// [out] 0, cannot feed; 1, can feed
}CascamDvtcasParentChildInfo;

typedef enum{
	CASCAM_DVTCAS_FEED_FAILED = 0,
	CASCAM_DVTCAS_FEED_SUCCESS,// feed data success, pls insert child card
}CascamDvtcasFeedNotify;

typedef struct{
	uint16_t operator_id;//[out]
	char tvs_private_info[CASCAM_DVTCAS_MAXLEN_TVSINFO];//[out]
}CascamDvtcasOperatorDetail;

typedef struct{
	uint8_t operator_num;//[inout] 0,default max. due to CDCA_MAXNUM_OPERATOR
	CascamDvtcasOperatorDetail *operator_detail;//
}CascamDvtcasOperatorInfo;

typedef struct{
	uint16_t operator_id;//[in]
	uint8_t detitle_num;//[inout] 0,default max. due to CDCA_MAXNUM_DETITLE
	char read_status[CASCAM_DVTCAS_MAXLEN_READ_STATUS];//[out]
	uint32_t chk_num[CASCAM_DVTCAS_MAXNUM_DETITLE_CHK];//[out]for get [in]for delete
}CascamDvtcasDetitleInfo;

typedef enum{
	CASCAM_DVTCAS_DETITLE_ALL_READ = 0,
	CASCAM_DVTCAS_DETITLE_RECEIVED,
	CASCAM_DVTCAS_DETITLE_SPACE_SMALL,
	CASCAM_DVTCAS_DETITLE_IGNORE,
}CascamDvtcasDetitleNotify;

typedef struct{
	uint8_t old_pin[CASCAM_DVTCAS_MAXLEN_PIN_CODE];// [in]  0~9
	uint8_t new_pin[CASCAM_DVTCAS_MAXLEN_PIN_CODE];// [in]  0~9
}CascamDvtcasPin;

typedef struct{
	uint8_t pin[CASCAM_DVTCAS_MAXLEN_PIN_CODE];
}CascamDvtCheckPin;

typedef struct{
	uint32_t magic;
	uint8_t update_status;
	char update_time[CASCAM_DVTCAS_MAXLEN_DATE];//
}CascamDvtcasCardUpdateInfo;

typedef struct{
	uint16_t progress;
	uint16_t mark;// 1,receive data "数据包接收进度" 2,update card "智能卡升级进度"
}CascamDvtcasCardUpdate;

typedef struct{
	uint32_t		m_dwVersion;								//send time
	char		m_szSenderName[CASCAM_DVTCAS_MAXLEN_MAIL_TITLE];	//send name
	uint16_t		m_wEmailLength;								//len
	char		m_szEmail[CASCAM_DVTCAS_MAXLEN_MAIL_CONTENT];	//content
}CascamDvtcasForceMailMsg;

typedef struct{
	char body[CASCAM_DVTCAS_MAXLEN_MAIL_CONTENT];
	uint32_t pos;
}CascamDvtcasMailContent;

typedef struct{
	uint32_t mail_id;//
	char importance;// 0: normal; 1:important
	char send_time[CASCAM_DVTCAS_MAXLEN_DATE];
	char send_name[CASCAM_DVTCAS_MAXLEN_MAIL_NAME];
	char mail_status[CASCAM_DVTCAS_MAXLEN_READ_STATUS];// Read or Unread
	uint16_t mail_len;
	char title[CASCAM_DVTCAS_MAXLEN_MAIL_TITLE];
}CascamDvtcasMailHead;

typedef struct _CASCAMDVTCASMailInfoClass{
	unsigned short mail_num;//[inout] the max num is CASCAM_DVTCAS_MAXNUM_EMAIL
	unsigned short mail_unread_count;//[out]
	CascamDvtcasMailHead *mail_head;
}CascamDvtcasMailList;

typedef struct{
	uint32_t pos;// 第一个邮件位置
	uint32_t count;// 连续邮件的数量
}CascamDvtcasMailDel;

typedef enum{
	CASCAM_DVTCAS_EMAIL_NEW = 0,//New
	CASCAM_DVTCAS_EMAIL_SPACE_FULL = 1,//Full
	CASCAM_DVTCAS_EMAIL_ICONHIDE = 0xFF,//Hide
}CascamDvtcasMailNotify;

typedef enum {
	CASCAM_DVTCAS_DISPLAY_ALLWAYS,
	CASCAM_DVTCAS_DISPLAY_CLEAN,
}CascamDvtcasMessageDisplayEnum;

typedef struct {
	uint16_t ecm_pid;
	uint16_t error_code;
}CascamDvtcasMessage;

typedef enum {
	CASCAM_DVTCAS_UNFORCE,// can switch channel, and ...
	CASCAM_DVTCAS_FORCE,// can not support remote and panel.
}CascamDvtcasForceType;

typedef enum {
	CASCAM_DVTCAS_HIDE,
	CASCAM_DVTCAS_SHOW,
}CascamDvtcasDisplayType;

typedef struct{
	uint32_t fre;
	uint32_t sym;
	uint32_t polar;
	uint16_t qam;
	uint16_t pcr_pid;
	uint16_t video_pid;
	uint16_t audio_pid;
	uint32_t video_type;
	uint32_t audio_type;
	uint32_t scramble_flag;  //1: scrambled; 0:free
	uint16_t video_ecm_pid;
	uint16_t audio_ecm_pid;
	CascamDvtcasForceType force_flag;// 1: force, can not switch channel
	CascamDvtcasDisplayType display_status;// 1: show; 0; hide
}CascamDvtcasLockService;

typedef struct{
    uint16_t ori_netID;
    uint16_t tSID;
    uint16_t serviceID;
    CascamDvtcasDisplayType display_status;// 1: show; 0; hide
}CascamDvtcasUrgencyBroadcast;

typedef enum {
	CASCAM_DVTCAS_CANCEL,//exit watch limit fucntion
	CASCAM_DVTCAS_ENABLE,//enable watch limit
	CASCAM_DVTCAS_DISABLE,//if stop time status, then change to work time status
}CascamDvtcasWatchLimitType;

typedef struct{
	uint16_t work_time;// 1~256 hours
	uint16_t stop_time;// 30~1440 minutes
	CascamDvtcasWatchLimitType type;
}CascamDvtcasWatchLimit;

typedef enum {
	CASCAM_DVTCAS_OSD_TOP = 1,//normal osd
	CASCAM_DVTCAS_OSD_BOTTOM,
	CASCAM_DVTCAS_OSD_FULLSCREEN,
	CASCAM_DVTCAS_OSD_HALFSCREEN,

	CASCAM_DVTCAS_SUPER_OSD_CENTER = 10,//super osd
	CASCAM_DVTCAS_SUPER_OSD_TOP,
	CASCAM_DVTCAS_SUPER_OSD_BOTTOM,
}CascamDvtcasOsdDisplayMode;

typedef struct{
	CascamDvtcasDisplayType display_type;
	CascamDvtcasOsdDisplayMode display_mode;
	CascamDvtcasForceType force_flag;
	uint8_t byPriority;
	char content[CASCAM_DVTCAS_MAXLEN_OSD];
}CascamDvtcasOsdInfo;

typedef enum {
	CASCAM_DVTCAS_FINGER = 0,//normal finger

	CASCAM_DVTCAS_SUPER_FINGER_STRING = 10,//super finger
	CASCAM_DVTCAS_SUPER_FINGER_DOT,
	CASCAM_DVTCAS_SUPER_FINGER_MATRIX,
}CascamDvtcasFingerDisplayMode;

typedef struct{
	CascamDvtcasDisplayType display_type;
	CascamDvtcasFingerDisplayMode display_mode;

	bool    m_bVisionType;			//visual type(0-Covert,1-Overt).When 0-Covert, duration does not take effect
	char		m_byszContent[CASCAM_DVTCAS_MAXNUM_FINGER];			//Fingerprint content
	int32_t	m_wDuration;			//Fingerprint display time length, - 1 means always display.
	uint8_t	m_byLocationFromTop;	//proportion of height.
	uint8_t	m_byLocationFromLeft;	//proportion of the width.
	uint8_t	m_bySize;				//The proportion of character fingerprint width to screen width; the value range is 0-100.
	uint32_t	m_dwFontARGB;			//Font transparency (1 byte) + font color, three primary colors
	uint32_t	m_dwBackgroundARGB; 	//Background transparency (1 byte) + background color, three primary colors
	uint8_t	m_byFontType;			//Font type
}CascamDvtcasFingerInfo;

typedef struct{
	uint16_t	m_wTVSID;					//运营商编号
	uint16_t	m_wProdID;					//产品ID
	uint8_t		m_bySlotID;					//钱包ID
	char		m_szProdName[CASCAM_DVTCAS_MAXLEN_PRODUCT_NAME];	//产品名称
	uint32_t	m_tStartTime;				//开始时间，time_t格式。
	uint32_t	m_dwDuration;				//持续秒数
	char		m_szServiceName[CASCAM_DVTCAS_MAXLEN_SERVICE_NAME];	//业务名称
	uint16_t	m_wCurTppTapPrice;			//当前的不回传、能录像价格(分)，价格类型值为0
	uint16_t	m_wCurTppNoTapPrice;		//当前的不回传、不可录像价格(分)，价格类型值为1
	uint16_t	m_wCurCppTapPrice;			//当前的要回传、能录像价格(分)，价格类型值为2
	uint16_t	m_wCurCppNoTapPrice;		//当前的要回传、不可录像价格(分)，价格类型值为3
	uint16_t	m_wBookedPrice;			//已经预订的价格(分)
	uint8_t		m_byBookedPriceType;		//已经预订的价格类型，取值范围0~3
	uint8_t		m_byBookedInterval;		//预订收费间隔
	uint8_t		m_byCurInterval;			//当前收费间隔
	uint8_t		m_byIppStatus;				//Ipp产品状态
	uint8_t		m_byUnit;					//收费间隔的单位0 -分钟1-小时2-天3-月4-年
	uint16_t	m_wIpptPeriod; 			//用户订购IPPT的观看周期数,for Philippines LongIPPT。
}CascamDvtcasIppDetail;

typedef struct{
	uint8_t count; // CASCAM_DVTCAS_MAXNUM_IPP_PROG
	CascamDvtcasIppDetail *info;
}CascamDvtcasIppInfo;

typedef struct{
	uint16_t	m_wTVSID;											//运营商编号
	char		m_szProdName[CASCAM_DVTCAS_MAXLEN_PRODUCT_NAME];		//产品名称
	uint32_t	m_tStartTime;						//开始时间，time_t格式。
	uint32_t	m_dwDuration;						//持续秒数
	uint16_t	m_wBookedPrice;						//预订价格(分)
	uint8_t		m_byBookedPriceType;					//预订价格类型：0:TppTap;1:TppNoTap;2:CppTap;3:CppNoTap;
	uint8_t		m_byBookedInterval;					//预订收费间隔
	char		m_szOtherInfo[44];					//ippv时为“此产品为ippv产品”，ippt时为“观看总时间：？分钟，扣钱总数：？分”
	uint8_t		m_byUnit;						//收费单位，0 -分钟1-小时2-天3-月4-年
}CascamDvtcasViewedIppDetail;

typedef struct{
	uint8_t count; // CASCAM_DVTCAS_MAXNUM_IPP_PROG
	CascamDvtcasViewedIppDetail *info;
}CascamDvtcasViewedIppInfo;

typedef struct{
	uint16_t ecmpid;
	CascamDvtcasIppDetail detail;
}CascamDvtcasIppNotify;

typedef struct{
	uint16_t operator_id;// [in]
	uint32_t balance;
	uint32_t remainder;
}CascamDvtcasWalletDetail;

typedef struct{
	uint8_t operator_num;//[in]due to DVTCA_MAXNUMBER_TVSID
	CascamDvtcasWalletDetail* detail;
}CascamDvtcasWalletInfo;

typedef enum {
	CASCAM_DVTCAS_ADV_PREVIEW_CANCEL = 0,//cancel advanced preview func
	CASCAM_DVTCAS_ADV_PREVIEW_OK,// advanced preview prog ready descramble
	CASCAM_DVTCAS_ADV_PREVIEW_TOTTIME_ERR,//stop descramble, reach total watch time
	CASCAM_DVTCAS_ADV_PREVIEW_WATCHTIME_ERR,//stop descramble, reach watchtime limit
	CASCAM_DVTCAS_ADV_PREVIEW_TOTCNT_ERR,//stop descramble, reach total watch count
	CASCAM_DVTCAS_ADV_PREVIEW_ROOM_ERR,//stop descramble, space full
	CASCAM_DVTCAS_ADV_PREVIEW_PARAM_ERR,//stop descramble, parameters of program error
	CASCAM_DVTCAS_ADV_PREVIEW_TIME_ERR,//stop descramble, data error
}CascamDvtcasAdvPreviewNotify;

typedef enum {
	CASCAM_DVTCAS_ACTION_RESTART_STB = 0,//restart stb
	CASCAM_DVTCAS_ACTION_FREEZE_STB,//freeze stb
	CASCAM_DVTCAS_ACTION_SEARCH,//search channel again
	CASCAM_DVTCAS_ACTION_UPGRADE,//software upgrade
	CASCAM_DVTCAS_ACTION_UNFREEZE_STB,//unfreeze stb
	CASCAM_DVTCAS_ACTION_INITIALIZE,//initialize stb
	CASCAM_DVTCAS_ACTION_SHOW_SYSTEM_INFO,//show system info

	CASCAM_DVTCAS_ACTION_CARD_INSERT_FINISH = 255,//dvtcas init finish
}CascamDvtcasActionNotify;

typedef struct{
	uint16_t operator_id;// [in]
	CascamDvtcasActionNotify action_type;
	uint8_t len;
	uint8_t data[CASCAM_DVTCAS_MAXLEN_DATA];//if len != 0, used
}CascamDvtcasActionRequest;

typedef struct{
	uint8_t len;
	uint8_t data[CASCAM_DVTCAS_MAXLEN_CORRESPOND_DATA];//max 250 bytes
}CascamDvtcasCorrespondInfo;

#if 0
//osd string
#define DVTCAS_OSD_OPERATOR_ERR           "Operator ID error"
#define DVTCAS_OSD_OUT_TIME               "Out of Working Hours"
#define DVTCAS_OSD_OUT_RATING             "Out of Teleview Rating"
#define DVTCAS_OSD_NOT_PAIRED             "The card is not paired with this STB"
#define DVTCAS_OSD_NO_ENTITLE             "No Entitlement"
#define DVTCAS_OSD_CHANGE_PIN             "PIN has been changed"
#define DVTCAS_OSD_INVALID_PIN            "Invalid PIN"
#define DVTCAS_OSD_CHANGE_RATING          "Teleview rating changed successfully"
#define DVTCAS_OSD_CHANGE_TIME            "Working Hours changed successfully"
#define DVTCAS_OSD_INVALID_TIME           "Invalid working hours"
#define DVTCAS_OSD_DELETE_MAIL            "Email deleted successfully"
#define DVTCAS_OSD_DELETE_MAIL_ALL        "All of the emails deleted successfully"
#define DVTCAS_OSD_PURCHASED              "IPPV product purchased successfully"
#define DVTCAS_OSD_PURCHASED_IPPT         "IPPT product purchased successfully"
#define DVTCAS_OSD_NO_ROOM                "No room for new IPPV"
#define DVTCAS_OSD_NO_ROOM_IPPT           "No room for new IPPT"
#define DVTCAS_OSD_NO_MONEY               "No enough money"
#define DVTCAS_OSD_REGIONAL_OUT           "Regional Lockout"
#define DVTCAS_OSD_VERIFICATION_FAILED    "Card verification failed. Please contact the operator"
#define DVTCAS_OSD_PATCHING               "Card in patching. Not to remove the card or power off"
#define DVTCAS_OSD_UPGRADE                "Please upgrade the smart card"
#define DVTCAS_OSD_SWITCH_CHANNEL_NOTICE  "Please do not switch channels too frequently"
#define DVTCAS_OSD_ACCOUNT_LOCKED         "Your account is locked, please contact your operator"
#define DVTCAS_OSD_SEND_RECORDS           "Please send the IPPV records back to the operator"
#define DVTCAS_OSD_RESTART                "Please restart the STB"

#define DVTCAS_OSD_BADCARD                "Unidentified Card"/* 无法识别卡 */
#define DVTCAS_OSD_EXPICARD               "Expired Card"/* 智能卡过期，请更换新卡 */
#define DVTCAS_OSD_INSERTCARD             "Please insert the smart card"/* 加扰节目，请插入智能卡 */
#define DVTCAS_OSD_NOOPER                 DVTCAS_OSD_OPERATOR_ERR/* 不支持节目运营商 */
#define DVTCAS_OSD_BLACKOUT               "Conditional Blackout"/* 条件禁播 */
#define DVTCAS_OSD_OUTWORKTIME            DVTCAS_OSD_OUT_TIME/* 当前时段被设定为不能观看 */
#define DVTCAS_OSD_WATCHLEVEL             DVTCAS_OSD_OUT_RATING/* 节目级别高于设定的观看级别 */
#define DVTCAS_OSD_PAIRING                DVTCAS_OSD_NOT_PAIRED/* 智能卡与本机顶盒不对应 */
#define DVTCAS_OSD_NOENTITLE              DVTCAS_OSD_NO_ENTITLE/* 没有授权 */
#define DVTCAS_OSD_DECRYPTFAIL            "Decryption Failure"/* 节目解密失败 */
#define DVTCAS_OSD_NOMONEY                DVTCAS_OSD_NO_MONEY/* 卡内金额不足 */
#define DVTCAS_OSD_ERRREGION              DVTCAS_OSD_REGIONAL_OUT /* 区域不正确 */
#define DVTCAS_OSD_NEEDFEED               "Please insert the parent-card of the current card"/* 子卡需要和母卡对应，请插入母卡 */
#define DVTCAS_OSD_ERRCARD                DVTCAS_OSD_VERIFICATION_FAILED /* 智能卡校验失败，请联系运营商 */
#define DVTCAS_OSD_UPDATE                 DVTCAS_OSD_PATCHING/* 智能卡升级中，请不要拔卡或者关机 */
#define DVTCAS_OSD_LOWCARDVER             DVTCAS_OSD_UPGRADE/* 请升级智能卡 */
#define DVTCAS_OSD_VIEWLOCK               DVTCAS_OSD_SWITCH_CHANNEL_NOTICE/* 请勿频繁切换频道 */
#define DVTCAS_OSD_MAXRESTART             "Card is dormancy. Please insert the card after 5 minutes"/* 智能卡暂时休眠，请5分钟后重新开机 */
#define DVTCAS_OSD_FREEZE                 "Your account is locked, please contact your operator"/* 智能卡已冻结，请联系运营商 */
#define DVTCAS_OSD_CALLBACK               DVTCAS_OSD_SEND_RECORDS/* 智能卡已暂停，请回传收视记录给运营商 */
#define DVTCAS_OSD_CURTAIN                "Advanced preview program, can not be free to watch"/*高级预览节目，该阶段不能免费观看*/
#define DVTCAS_OSD_CARDTESTSTART          "Upgrade test card testing..."/*升级测试卡测试中...*/
#define DVTCAS_OSD_CARDTESTFAILD          "Upgrade test card test failed, please check the machine card communication module"/*升级测试卡测试失败，请检查机卡通讯模块*/
#define DVTCAS_OSD_CARDTESTSUCC           "Upgrade test card test success"/*升级测试卡测试成功*/
#define DVTCAS_OSD_NOCALIBOPER            "There is no custom operator in the card."/*卡中不存在移植库定制运营商*/
#define DVTCAS_OSD_STBLOCKED              DVTCAS_OSD_RESTART/* 请重启机顶盒 */
#define DVTCAS_OSD_STBFREEZE              "STB is freezed. Please contact the operator"/* 机顶盒被冻结 */
#define DVTCAS_OSD_UNSUPPORTDEVICE        "Unsupported terminal type (number: 0xXXXX)"/*不支持的终端类型（编号：0xXXXX）*/

#define DVTCAS_OSD_UPGRADE_STB            "Please upgrade the STB!"
#define DVTCAS_OSD_IPPV_STAGE1            "IPPV free preview stage, buy it?"
#define DVTCAS_OSD_IPPV_STAGE2            "IPPV charging stage,buy it?"
#define DVTCAS_OSD_IPPT_NOTIFY            "IPPT charging stage, buy it?"
#define DVTCAS_OSD_EXPIRED_TIME           "Expired Time"
#define DVTCAS_OSD_INTERVAL_TIME          "Interval Time"
#define DVTCAS_OSD_MINUTE                 "min"
#define DVTCAS_OSD_WALLET_ID              "Wallet ID"
#define DVTCAS_OSD_IPPV_PRICE_TYPE1       "The price without PVR"
#define DVTCAS_OSD_IPPV_PRICE_TYPE2       "The price with PVR"
#define DVTCAS_OSD_IPPV_BUY_TYPE          "Buy Type:"
#define DVTCAS_OSD_IPPV_POINT             "points"

#define DVTCAS_OSD_RECEIVE_DATA           "Receiving data"
#define DVTCAS_OSD_UPDATE_SUCCESS         "Card Update Success"
#define DVTCAS_OSD_UPDATE_FAIL            "Card Update Failed"
//card update
#define DVTCAS_OSD_INSERT_CHILD           "please insert the child-card"
#define DVTCAS_OSD_INSERT_PARENT          "please insert the parent-card"
#define DVTCAS_OSD_FEED_SUCCESS           "Feeding successfully"
#define DVTCAS_OSD_BAD_PARENT             "Invalid parent-card"
#define DVTCAS_OSD_FEED_FAIL              "Feeding failed"
#define DVTCAS_OSD_FEED_NO_TIME           "It is no time to feeding"
#define DVTCAS_OSD_FEED_NO_CARD           "no card find, please insert the card"
#define DVTCAS_OSD_PARENT                 "parent card"
#define DVTCAS_OSD_CHILD                  "child card"
#define DVTCAS_OSD_F1_FEED                "Press F1 will feed card"
#define DVTCAS_OSD_FEED_NO                "Can not feed"
#define DVTCAS_OSD_FEED_YES               "Can feed"
#define DVTCAS_OSD_PARENT_SN              "parent card sn"
#define DVTCAS_OSD_FEED_PEAR              "feed pear"
#define DVTCAS_OSD_LAST_FEED_TIME         "last feed time"
#define DVTCAS_OSD_FEED_UNKNOWN_CARD      "unknown card"
#define DVTCAS_OSD_FEED_HOURS             "hours"
//menu
#define DVTCAS_BASIC_INFO                 "Basic Information"
#define DVTCAS_CARD_SN                    "Card SN:"
#define DVTCAS_STB_ID                     "STB ID:"
#define DVTCAS_STB_MATCH_STATUS           "STB Match Status:"
#define DVTCAS_CAS_VERSION                "CAS Version:"
#define DVTCAS_WORK_TIME                  "Work Time:"
#define DVTCAS_WATCH_RATING               "Watch Level:"
#define DVTCAS_OPERATOR_ID                "Operator ID:"
#define DVTCAS_MARKET_ID                  "Market ID:"
#define DVTCAS_JTAG_STATUS                "JTAG Debug Status:"
#define DVTCAS_SECURITY_START_STATUS      "Security Start Status:"
#define DVTCAS_DETITLE                    "Detitle Information"
#define DVTCAS_READ                       "Read"
#define DVTCAS_UNREAD                     "Unread"
#define DVTCAS_ENTITLEMENT_INFO           "Entitlement Information"
#define DVTCAS_ALLOW                      "Allow"
#define DVTCAS_FORBIDDEN                  "Forbidden"
#define DVTCAS_FEATURES_INFO              "Features Information"
#define DVTCAS_IPPV_INFO                  "IPPV Information"
#define DVTCAS_UNKNOW                     "Unknow"
#define DVTCAS_BOOKING                    "Booking"
#define DVTCAS_VIEWED                     "Viewed"
#define DVTCAS_EXPIRED                    "Expired"
#define DVTCAS_EMAIL_INFO                 "Email List"
#define DVTCAS_NUMBER                     "No."
#define DVTCAS_SEND_TIME                  "Send Time"
#define DVTCAS_SUBJECT                    "Subject"
#define DVTCAS_DELETE_ALL_MAILS           "Delete All Mails"
#define DVTCAS_PRODUCT_ID                 "Product ID"
#define DVTCAS_END_TIME                   "End Time"
#define DVTCAS_TOTAL                      "Total"
#define DVTCAS_IMPORTANT                  "Important"
#define DVTCAS_NORMAL                     "Normal"
#define DVTCAS_IMPORTANCE                 "Importance"
#define DVTCAS_OPERATOR_TITLE             "Operator Information"
#define DVTCAS_ENTITLEMENT                "Entitlement Information"
#define DVTCAS_WALLET                     "Wallet Information"
#define DVTCAS_IPPV                       "IPPV Information"
#define DVTCAS_FEATURES                   "Features Information"
#define DVTCAS_CARD_UPDATE                "Card Update Information"
#define DVTCAS_CARD_PARENT_CHILD          "Parent Child Card"
#define DVTCAS_CARD_PAIR_INFO             "Card Pair Information"
#endif
#endif
