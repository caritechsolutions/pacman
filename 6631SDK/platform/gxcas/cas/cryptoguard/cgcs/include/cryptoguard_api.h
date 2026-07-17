#ifndef __CRYPTOGUARD_API_H__
#define __CRYPTOGUARD_API_H__
#include "gxcas.h"
#include "cg_cas_type.h"

#define GXCAS_CG_MAX_PIN_LEN (4)

typedef struct{
	uint32_t IrdNumber;
	uint8_t STB_SystemGlobalKey[16];
	uint8_t STB_ManufacturerGlobalKey[16];
	uint8_t STB_GroupKey[16];
	uint8_t STB_UniqueKey[16];
	uint8_t STB_CardlessGroupKey[16];
	uint8_t STB_CardlessUniqueKey[16];
}GxCas_CGSetStbInfo;

typedef struct{
	char pin[GXCAS_CG_MAX_PIN_LEN];
}GxCas_CGCheckPin;

typedef struct{
	char old_pin[GXCAS_CG_MAX_PIN_LEN];
	char new_pin[GXCAS_CG_MAX_PIN_LEN];
}GxCas_CGChangePin;

typedef struct{
	char pin[GXCAS_CG_MAX_PIN_LEN];
	unsigned char rating;
}GxCas_CGChangeRating;

typedef struct{
	char pin[GXCAS_CG_MAX_PIN_LEN];
	unsigned char CH;
}GxCas_CGUnlockPin;

typedef struct{
	char pin[GXCAS_CG_MAX_PIN_LEN];
	unsigned char enable;
}GxCas_CGSetStrictRating;

typedef struct{
	unsigned char encoding;
	text_msg_t osd_msg;
}GxCas_CGShowOsdMessage;

typedef struct{
	unsigned char encoding;
	message_descriptor_t long_msg;
}GxCas_CGShowLongMessage;

typedef struct{
	unsigned char encoding;
	scroll_descriptor_t scroll_msg;
}GxCas_CGScrollMessage;

typedef struct{
	ecm_stb_videorules_t video_rules;
}GxCas_CGVideoRules;

typedef struct{
	ota_trigger_t ota_trigger;
}GxCas_CGOtaTrigger;

typedef struct{
	pin_descriptor_t pin_required;
}GxCas_CGPinRequired;

typedef struct{
	fingerprint_descriptor_t finger_info;
}GxCas_CGFingerInfo;

typedef struct {
	unsigned short DestinationSID;
	unsigned char  tagvalue;			// 0x5A = DVB-T,  0x44 = DVB-C,  0x43 = DVB-S
	unsigned char  len;
	unsigned char  descriptordata[11];	// specified in Digital Video Broadcasting (DVB) Specification for Service Information (SI) in DVB systems (en_300468v011101p.pdf)
										// depending on type "Satellite delivery system descriptor", "Cable delivery system descriptor" or "Terrestrial delivery system descriptor"
}GxCas_CGDelivery_descriptor;


#define GXCAS_CRYPTOGUARD_OSD_MESSAGE                       GXCAS_IOR(GXCAS_EVENT, 0, GxCas_CGShowOsdMessage)
#define GXCAS_CRYPTOGUARD_LONG_MESSAGE                      GXCAS_IOR(GXCAS_EVENT, 1, GxCas_CGShowLongMessage)
#define GXCAS_CRYPTOGUARD_SCROLL_MESSAGE                    GXCAS_IOR(GXCAS_EVENT, 2, GxCas_CGScrollMessage)
#define GXCAS_CRYPTOGUARD_VIDEO_RULES                    	GXCAS_IOR(GXCAS_EVENT, 3, GxCas_CGVideoRules)
#define GXCAS_CRYPTOGUARD_OTA_TRIGGER                    	GXCAS_IOR(GXCAS_EVENT, 4, GxCas_CGOtaTrigger)
#define GXCAS_CRYPTOGUARD_PIN_REQUIRED                    	GXCAS_IOR(GXCAS_EVENT, 5, GxCas_CGPinRequired)
#define GXCAS_CRYPTOGUARD_FINGER_INFO                    	GXCAS_IOR(GXCAS_EVENT, 6, GxCas_CGFingerInfo)
#define GXCAS_CRYPTOGUARD_CARD_IN                    	    GXCAS_IO(GXCAS_EVENT, 7)
#define GXCAS_CRYPTOGUARD_CARD_OUT                    	    GXCAS_IO(GXCAS_EVENT, 8)
#define GXCAS_CRYPTOGUARD_FORCETUNE                         GXCAS_IO(GXCAS_EVENT, 9)


#define GXCAS_CRYPTOGUARD_SWITCH_CHANNEL					GXCAS_IOW(GXCAS_SET, 20, GxCasSet_SwitchChannel)
#define GXCAS_CRYPTOGUARD_SET_STB_INFO						GXCAS_IOW(GXCAS_SET, 21, GxCas_CGSetStbInfo)
#define GXCAS_CRYPTOGUARD_CHECK_PIN							GXCAS_IOW(GXCAS_SET, 22, GxCas_CGCheckPin)
#define GXCAS_CRYPTOGUARD_SET_PIN							GXCAS_IOW(GXCAS_SET, 23, GxCas_CGChangePin)
#define GXCAS_CRYPTOGUARD_SET_MATURITY_RATING				GXCAS_IOW(GXCAS_SET, 24, GxCas_CGChangeRating)
#define GXCAS_CRYPTOGUARD_UNLOCK_PIN						GXCAS_IOW(GXCAS_SET, 25, GxCas_CGUnlockPin)
#define GXCAS_CRYPTOGUARD_SET_STRICT_RATING					GXCAS_IOW(GXCAS_SET, 26, GxCas_CGSetStrictRating)

#define GXCAS_CRYPTOGUARD_GET_LIB_VER_LONG                  GXCAS_IOR(GXCAS_GET, 40, char[64])
#define GXCAS_CRYPTOGUARD_GET_LIB_VER                       GXCAS_IOR(GXCAS_GET, 41, char[12])
#define GXCAS_CRYPTOGUARD_GET_LIB_DATE						GXCAS_IOR(GXCAS_GET, 42, char[12])
#define GXCAS_CRYPTOGUARD_GET_MATURITY_RATING				GXCAS_IOR(GXCAS_GET, 43, unsigned char)
#define GXCAS_CRYPTOGUARD_GET_STRICT_RATING					GXCAS_IOR(GXCAS_GET, 44, unsigned char)
#define GXCAS_CRYPTOGUARD_GET_MAIN_MENU                     GXCAS_IOR(GXCAS_GET, 45, char[25000])
#define GXCAS_CRYPTOGUARD_GET_SUBSCRIPTION_INFO				GXCAS_IOR(GXCAS_GET, 46, char[25000])
#define GXCAS_CRYPTOGUARD_GET_PAY_INFO						GXCAS_IOR(GXCAS_GET, 47, char[25000])
#define GXCAS_CRYPTOGUARD_GET_ABOUT_CA						GXCAS_IOR(GXCAS_GET, 48, char[25000])
#define GXCAS_CRYPTOGUARD_GET_PARENTAL_CONTROL				GXCAS_IOR(GXCAS_GET, 49, char[25000])

//1 blocks
int32_t GxCas_CryptoGuardInit(const GxCasInitParam);

#endif

