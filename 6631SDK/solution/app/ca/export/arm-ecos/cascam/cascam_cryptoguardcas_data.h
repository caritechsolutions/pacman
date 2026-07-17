#ifndef __CASCAM_CRYPTOGUARDCAS_DATA_H__
#define __CASCAM_CRYPTOGUARDCAS_DATA_H__
#include "gxtype.h"

#define CASCAM_CG_MAX_PIN_LEN               (4)
#define CASCAM_CG_MAX_PARENTAL_CONTROL_LEN  (1000)
#define CASCAM_CG_MAX_ABOUT_CA_LEN          (1000)
#define CASCAM_CG_MAX_PAY_INFO_LEN          (1000)
#define CASCAM_CG_MAX_SUBS_INFO_LEN         (1000)
#define CASCAM_CG_MAX_MAIN_MENU_LEN         (1000)
#define CASCAM_CG_MAX_LIB_VER_LEN           (12)
#define CASCAM_CG_MAX_LIB_LVER_LEN          (64)
#define CASCAM_CG_MAX_LIB_DATA_LEN          (12)

#define CASCAM_CG_MSG_MAX_LEN			    (1024*3)
//osd
#define CASCAM_CG_OSD_MSG_LEN				(128)
#define CASCAM_CG_LONG_MSG_LEN				(2048)

typedef enum {
	CASCAM_CGCAS_HIDE,
	CASCAM_CGCAS_SHOW,
} CASCAM_CGCAS_DISPLAY_TYPE;

typedef enum{
    //event
    CASCAM_CGCAS_OSD_MESSAGE,           //Cascam_CGShowOsdMessage
    CASCAM_CGCAS_LONG_MESSAGE,          //Cascam_CGShowLongMessage
    CASCAM_CGCAS_SCROLL_MESSAGE,        //Cascam_CGScrollMessage)
    CASCAM_CGCAS_VIDEO_RULES,           //Cascam_CGVideoRules)
    CASCAM_CGCAS_OTA_TRIGGER,           //Cascam_CGOtaTrigger)
    CASCAM_CGCAS_PIN_REQUIRED,          //Cascam_CGPinRequired)
    CASCAM_CGCAS_FINGER_INFO,           //Cascam_CGFingerInfo)
    CASCAM_CGCAS_CARD_IN,
    CASCAM_CGCAS_CARD_OUT,
	CASCAM_CGCAS_FORCETUNE,		
	
    //set
    CASCAM_CGCAS_SWITCH_CHANNEL,        //Cascam_SwitchChannel)
    CASCAM_CGCAS_SET_STB_INFO,          //Cascam_CGSetStbInfo)
    CASCAM_CGCAS_CHECK_PIN,             //Cascam_CGCheckPin)
    CASCAM_CGCAS_SET_PIN,               //Cascam_CGChangePin)
    CASCAM_CGCAS_SET_MATURITY_RATING,   //Cascam_CGChangeRating)
    CASCAM_CGCAS_UNLOCK_PIN,            //Cascam_CGUnlockPin)
    CASCAM_CGCAS_SET_STRICT_RATING,     //Cascam_CGSetStrictRating)

    //get
    CASCAM_CGCAS_GET_LIB_VER_LONG,      //char[64])
    CASCAM_CGCAS_GET_LIB_VER,           //char[12])
    CASCAM_CGCAS_GET_LIB_DATE,          //char[12])
    CASCAM_CGCAS_GET_MATURITY_RATING,   //unsigned char)
    CASCAM_CGCAS_GET_STRICT_RATING,     //unsigned char)
    CASCAM_CGCAS_GET_MAIN_MENU,         //char[25000])
    CASCAM_CGCAS_GET_SUBSCRIPTION_INFO, //char[25000])
    CASCAM_CGCAS_GET_PAY_INFO,          //char[25000])
    CASCAM_CGCAS_GET_ABOUT_CA,          //char[25000])
    CASCAM_CGCAS_GET_PARENTAL_CONTROL,  //char[25000])

    CASCAM_CGCAS_MSG_MAX,
}CASCAM_CGCAS_TYPE;


typedef struct{
    uint32_t IrdNumber;
    uint8_t STB_SystemGlobalKey[16];
    uint8_t STB_ManufacturerGlobalKey[16];
    uint8_t STB_GroupKey[16];
    uint8_t STB_UniqueKey[16];
    uint8_t STB_CardlessGroupKey[16];
    uint8_t STB_CardlessUniqueKey[16];
}Cascam_CGSetStbInfo;

typedef struct{
    char pin[CASCAM_CG_MAX_PIN_LEN];
}Cascam_CGCheckPin;

typedef struct{
    char old_pin[CASCAM_CG_MAX_PIN_LEN];
    char new_pin[CASCAM_CG_MAX_PIN_LEN];
}Cascam_CGChangePin;

typedef struct{
    char pin[CASCAM_CG_MAX_PIN_LEN];
    unsigned char rating;
}Cascam_CGChangeRating;

typedef struct{
    char pin[CASCAM_CG_MAX_PIN_LEN];
    unsigned char CH;
}Cascam_CGUnlockPin;

typedef struct{
    char pin[CASCAM_CG_MAX_PIN_LEN];
    unsigned char enable;
}Cascam_CGSetStrictRating;

typedef struct{
    unsigned char encoding;
    uint16_t  msg_ms_duration;
    uint8_t   msg_textstring[128];
    uint8_t   msg_length;
}Cascam_CGShowOsdMessage;

typedef struct{
    unsigned char encoding;
    uint8_t   lmsgForced;
    uint16_t  lmsgDuration;
    uint8_t   lmsgText[2048];
}Cascam_CGShowLongMessage;

typedef struct{
    unsigned char encoding;
    uint16_t  msgDuration;
    uint8_t   msgY;
    uint32_t  msgFontColor;
    uint32_t  msgBackgroundColor;
    uint16_t  msgScrollSpeed;
    uint8_t   msgForced;
    uint8_t   msgFontSize;
    char      msgText[256];
}Cascam_CGScrollMessage;

typedef struct{
    uint8_t CGMSA;
    uint8_t HDCP;
    uint8_t Macrovision;
    uint8_t AnaHD;
    uint8_t DigHD;
    uint8_t CH;
}Cascam_CGVideoRules;

typedef struct{
    uint8_t     ForcedUpgrade; ///< Forced upgrade
    uint32_t    Manufacturer;  ///< Manufacturer ID used for OTA trigger, contact Cryptoguard to receive ID if needed
    uint32_t    Model;         ///< Manufacturer model identifier
    uint32_t    Version;       ///< Sw version in OTA stream
    uint32_t    PID;           ///< Data PID
    uint32_t    Delivery;      ///< 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
    uint32_t    Frequency;     ///< frequency
    uint32_t    Symbolrate;    ///< Symbolrate 0x1ADB => 6875
    uint16_t    Fec;           ///< fec (satellite)
    uint16_t    Polarity;      ///< polarity (satellite)
    uint16_t    Modulation;    ///< modulation 2=qam64 3=qam128 4=qam256
    uint16_t    Bandwith;      ///< bandwith (terrestial), 8 = 8mhz and 7 = 7mhz
    uint8_t     StructVersion;
}Cascam_CGOtaTrigger;

typedef struct{
    char  text[32];
    uint8_t   rating;
}Cascam_CGPinRequired;

typedef struct{
    char     DisplayText[32];
    uint16_t  DurationMs;
    uint8_t   X;
    uint8_t   Y;
    uint32_t  FontColor;
    uint32_t  BackgroundColor;
    uint8_t   CH;
}Cascam_CGFingerInfo;

typedef struct {
	unsigned short DestinationSID;
	unsigned char  tagvalue;			// 0x5A = DVB-T,  0x44 = DVB-C,  0x43 = DVB-S
	unsigned char  len;
	unsigned char  descriptordata[11];	// specified in Digital Video Broadcasting (DVB) Specification for Service Information (SI) in DVB systems (en_300468v011101p.pdf)
										// depending on type "Satellite delivery system descriptor", "Cable delivery system descriptor" or "Terrestrial delivery system descriptor"
}Cascam_CGDelivery_descriptor;
#endif
