
#ifndef __APP_TKGS_OTA_PROTOCOL_H__
#define __APP_TKGS_OTA_PROTOCOL_H__
#if TKGS_SUPPORT

#include "gxcore.h"
#include "gxsi.h"
#include "app_config.h"
#include "gxtkgs.h"

#define TKGS_MULTI_LANG_USER_MSG_DES_TAG	0x90
#define TKGS_STANDARD_DES_TAG				0x91
#define TKGS_MULTI_CATEGORY_LABEL_DES_TAG	0X92
#define TKGS_MULTI_SERV_LIST_NAME_DES_TAG	0X93
#define TKGS_SERVICE_LIST_LOCATION_DES_TAG	0x94
#define	TKGS_EXTN_SERVICE_DES_TAG			0x95
#define	TKGS_CONSTANT_CONTROL_WORD_DES_TAG	0x96
#define	TKGS_COMPONENT_DES_TAG				0x98
#define	TKGS_SAT_BROADCAST_SOURCE_DES_TAG	0xA0
#define	TKGS_CABLE_BROADCAST_SOURCE_DES_TAG	0xA1
#define	TKGS_TERRESTRIAL_BROADCAST_SOURCE_DES_TAG	0xA2
#define TKGS_COMPONENT_TAG	0X98

//blocking filter flag
#define TKGS_DELIVERY_FILTER	0X8000
#define TKGS_SERVICE_ID_FILTER	0X4000
#define TKGS_NETWORK_ID_FILTER	0X2000
#define TKGS_TRANSPORT_STREAM_ID_FILTER	0X1000
#define TKGS_VIDEO_PID_FILTER	0X0800
#define TKGS_AUDIO_PID_FILTER	0X0400
#define TKGS_NAME_FILTER		0X0200

//delivery filter flag
#define TKGS_DELIVERY_FREQ_FILTER 	0X8000
#define TKGS_DELIVERY_SYMB_FILTER	0X4000
#define TKGS_DELIVERY_POL_FILTER	0X2000
#define TKGS_DELIVERY_ORBITAL_FILTER	0X1000
#define TKGS_DELIVERY_MOD_TYPE_FILTER	0X0800
#define TKGS_DELIVERY_MOD_SYS_FILTER	0X0400
#define TKGS_DELIVERY_MOD_FILTER		0X0200
#define TKGS_DELIVERY_FEC_FILTER		0X0100

//stream type
#define  MPEG_1_VIDEO		0x01
#define  MPEG_2_VIDEO		0x02
#define  MPEG_1_AUDIO		0x03
#define  MPEG_2_AUDIO		0x04
#define	 TKGS_DATA			0X05
#define  H264				0x1b
#define  AAC_ADTS			0xf
#define  AAC_LATM			0x11
#define  AVS				0x42
#define	 PRIVATE_PES_STREAM  0x06//出现在pmt中一般是EAC3
#define	 LPCM  0x80
#define	 AC3  0x81
#define	 DTS  0x82
#define	 DOLBY_TRUEHD  0x83
#define	 AC3_PLUS  0x84
#define	 DTS_HD  0x85
#define	 DTS_MA  0x86
#define	 AC3_PLUS_SEC  0xa1
#define	 DTS_HD_SEC  0xa2

/*sdt指明的业务类型*/
typedef enum {
	SI_RESERVED_SERVICE = 0x00,
	SI_VIDEO_SERVICE = 0x01,
	SI_AUDIO_SERVICE = 0x02,
	SI_VBI_SERVICE = 0x03,
	SI_NVOD_REFERENCE_SERVICE = 0x04,
	SI_NVOD_TIMESHIFT_SERVICE = 0x05,
	SI_MOSAIC_SERVICE = 0x06,
	SI_PAL_SERVICE = 0x07,
	SI_SECAM_SERVICE = 0x08,
	SI_DD2_MAC_SERVICE = 0x09,
	SI_FRE_BROADCAST_SERVICE = 0x0a,
	SI_NTSC_SERVICE = 0x0b,
	SI_DATA_BROADCAST_SERVICE = 0x0c,
	SI_COMMON_RESERVED_SERVICE = 0x0d,
	SI_RCS_MAP_SERVICE = 0x0e,
	SI_RCS_FLS_SERVICE = 0x0f,
	SI_DVB_MHP_SERVICE = 0x10
} GxSearchSiServiceType;

typedef enum {			/*NIT, BAT, SDT, EIT, TOT, PMT, SIT     */
	CA_DESC = 0x09,
	ISO_639_LANGUAGE_DESC = 0x0a,
	NETWORK_NAME_DESC = 0x40,
	SERVICE_LIST_DESC = 0x41,
	STUFFING_DESC = 0x42,
	SATELLITE_DELIVERY_SYSTEM_DESC = 0x43,
	CABLE_DELIVERY_SYSTEM_DESC = 0x44,
	VBI_DATA_DESC = 0x45,
	VBI_TELETEXT_DESC = 0x46,
	BOUQUET_NAME_DESC = 0x47,
	SERVICE_DESC = 0x48,
	COUNTRY_AVAILABILITY_DESC = 0x49,
	LINKAGE_DESC = 0x4A,
	NVOD_REFERENCE_DESC = 0x4B,
	TIME_SHIFTED_SERVICE_DESC = 0x4C,
	SHORT_EVENT_DESC = 0x4D,
	EXTENDED_EVENT_DESC = 0x4E,
	TIME_SHIFTED_EVENT_DESC = 0x4F,
	COMPONENT_DESC = 0x50,
	MOSAIC_DESC = 0x51,
	STREAM_IDENTIFIER_DESC = 0x52,
	CA_IDENTIFIER_DESC = 0x53,
	CONTENT_DESC = 0x54,
	PARENTAL_RATING_DESC = 0x55,
	TELTEXT_DESC = 0x56,
	TELEPHONE_DESC = 0x57,
	LOCAL_TIME_OFFSET_DESC = 0x58,
	SUBTITLE_DESC = 0x59,
	TERRESTRIAL_DELIEVERY_SYSTEM_DESC = 0x5A,
	MULTILINGUAL_NETWORK_NAME_DESC = 0x5B,
	MULTILINGUAL_BOUQUET_NAME_DESC = 0x5C,
	MULTILINGUAL_SERVICE_NAME_DESC = 0x5D,
	MULTILINGUAL_COMPONENT_DESC = 0x5E,
	PRIVATE_DATA_SPECIFIER_DESC = 0x5F,
	SERVICE_MOVE_DESC = 0x60,
	SHORT_SMOOTHING_BUFFER_DESC = 0x61,
	FREQUENCY_LIST_DESC = 0x62,
	PARTIAL_TRANSPORT_STREAM_DESC = 0x63,
	DATA_BROADCAST_DESC = 0x64,
	CA_SYSTEM_DESC = 0x65,
	DATA_BROADCAST_ID_DESC = 0x66,
	TRANSPORT_STREAM_DESC = 0x67,
	DSNG_DESC = 0x68,
	PDC_DESC = 0x69,
	AC3_DESC = 0x6A,
	ANCILLARY_DATA_DESC = 0x6B,
	CELL_LIST_DESC = 0x6C,
	CELL_FREQUENCY_LINK_DESC = 0x6D,
	ANNOUNCEMENT_SUPPORT_DESC = 0x6E,
	APPLICATION_SIGNALLING_DESC = 0x6F,
	ADAPTATION_FIELD_DATA_DESC = 0x70,
	SERVICE_IDENTIFIER_DESC = 0x71,
	SERVICE_AVAILABILITY_DESC = 0x72,
	DEFAULT_AUTHORITY_DESC = 0x73,
	RELATED_CONTENT_DESC = 0x74,
	TVA_IDE_DESC = 0x75,
	CONTENT_IDENTIFIER_DESC = 0x76,
	TIME_SLICE_FEC_IDENTIFIER_DESC = 0x77,
	ECM_REPETITION_RATE_DESC = 0x78
} GxSearchSiDescriptor;


typedef struct
{
	uint32_t infoLen;
	uint8_t *pTkgsInfo;

	uint32_t broadcastSourceLen;
	uint8_t *pBroadcastSource;

	uint32_t serviceListLen;
	uint8_t *pserviceList;

	uint32_t categoryLen;
	uint8_t *pcategory;

	uint32_t channelBlockingLen;
	uint8_t *pchannelBlocking;
}TkgsData_s;


//tkgs descriptor define

typedef struct{
	uint8_t		language_code[3];
	uint8_t		*msg;
}TkgsMsg;
//Multilingual user message descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x90, TKGS_MULTI_LANG_USER_MSG_DES_TAG
    uint8_t descriptor_length;
    uint16_t multi_lang_msg_count;
	TkgsMsg* tkgs_msg;
}TkgsMultiLangUserMsgDes;	//tkgs_info_descriptor_loop

//TKGS standard descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x91, TKGS_STANDARD_DES_TAG
    uint8_t descriptor_length;
    uint8_t interface_version;
}TkgsStandardDes;

//Multilingual category label descriptor
typedef struct
{
    uint8_t		language_code[3];
    uint8_t 	name_length;
    uint8_t 	*name;
}TkgsCategoryName, TkgsServiceListName;
typedef struct
{
    uint8_t descriptor_tag;	//0x92, TKGS_MULTI_CATEGORY_LABEL_DES_TAG
    uint8_t descriptor_length;
    uint16_t category_id;
	uint16_t multi_lang_name_count;
	TkgsCategoryName *category_name;
}TkgsMultiCategoryLabDes;

//multilingual_service_list_name_descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x93, TKGS_MULTI_SERV_LIST_NAME_DES_TAG
    uint8_t descriptor_length;
	uint16_t multi_lang_name_count;
	TkgsServiceListName *servie_list_name;
}TkgsServiceListNameDes;

//service_list_location_descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x94, TKGS_SERVICE_LIST_LOCATION_DES_TAG
    uint8_t descriptor_length;
	GxTkgsService	location;
}TkgsServiceListLocationDes;


//TKGS extended service descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x95, TKGS_EXTN_SERVICE_DES_TAG
    uint8_t descriptor_length;
	uint8_t	age_limit;
	uint16_t category_count;
	uint16_t *category;
}TkgsExtenServiceDes;

//constant_control_word_descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x96 TKGS_CONSTANT_CONTROL_WORD_DES_TAG
    uint8_t descriptor_length;
	uint64_t control_world;
}TkgsControlWordDes;

//TKGS component descriptor
typedef struct
{
    uint8_t descriptor_tag;	//0x98 TKGS_COMPONENT_DES_TAG
    uint8_t descriptor_length;
	uint32_t tkgs_identifier;
	uint8_t		current_next_indicator;
}TkgsComponentDes;

//Satellite broadcast source descriptor
typedef struct
{
    uint8_t 	descriptor_tag;	//0xA0 TKGS_SAT_BROADCAST_SOURCE_DES_TAG
    uint8_t 	descriptor_length;
	uint8_t		name_length;
	uint8_t		*name;
	uint16_t	orbital_position;
	uint8_t		west_east_flag;
}TkgsSatSourceDes;

//Cable broadcast source descriptor, Terrestrial broadcast source descriptor
typedef struct
{
    uint8_t 	descriptor_tag;	//0xA1 TKGS_CABLE_BROADCAST_SOURCE_DES_TAG, //0xA2 TKGS_TERRESTRIAL_BROADCAST_SOURCE_DES_TAG
    uint8_t 	descriptor_length;
	uint8_t		name_length;
	uint8_t		*name;
}TkgsCableSourceDes, TkgsTerrestrialSourceDes;



#endif
#endif

