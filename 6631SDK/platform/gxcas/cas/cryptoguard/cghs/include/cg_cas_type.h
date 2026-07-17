#ifndef _CG_CAS_TYPE_H
#define _CG_CAS_TYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
//#include "sys_types.h"
#include "time.h"
#include "string.h"

typedef char int8_cg;
typedef unsigned char uint8_cg;
typedef short int16_cg;
typedef unsigned short uint16_cg;
typedef int int32_cg;
typedef unsigned int uint32_cg;
typedef unsigned long long uint64_cg;

#define	PSI_LONG_SECTION_LENGTH	4096
#define CAS_MAX_SESSIONS 2
#define CAS_MAX_CW_LENGTH 32
#define NR_OF_CASID 3
#define ECM_FP_DATA_SIZE 8
#define ECM_STB_CTRL_SIZE 3

typedef enum caid_e{
	CA_4B24 = 0,
	CA_1EC0,
	CA_CARD,
	CA_MAX_IDNR
}caid_t;

typedef enum {
	error_address = 0,
	cb_unique_address,
	cb_group_address,
	cl_unique_address,
	cl_group_address,
	global_address
} cg_address_t;

/// Structure of global variable holding status of CAS library
typedef struct {
	/// Holds a list of all CAID currently active
	uint16_cg	Caid[NR_OF_CASID];
	/// Library version information
	uint8_cg	Version[3];
	/// STB Cardless identity in : separated format XX:XX:XX......
	int8_cg		IrdNrStr[24];
	/// Library date string
	int8_cg		CaDateStr[11];
	/// Flag for CAS Initialized ok.
	bool		CasInited;
	/// Flag if card is inserted and valid.
	bool		card_ready;
} cg_status_t;

typedef enum cg_event_cb_e
{
	CG_MSG_CAID_CHANGE = 1,
	CG_MSG_EMMPID_CHANGE,
	CG_MSG_PIN_REQUEST,
	CG_MSG_OSD_MESSAGE,
	CG_MSG_LONG_MESSAGE,
	CG_MSG_SCROLL_MESSAGE,
	CG_MSG_FINGERPRINT,
	CG_MSG_OTA_TRIGGER,
	CG_MSG_STB_RULES
} cg_event_cb_t;

/// Possible errorcodes when calling a function that returns using cg_error_t
typedef enum cg_error_e
{
    CG_SUCCESS = 0,
    CG_ERROR_NOT_INITIALIZED,       // 1
    CG_ERROR_DATA_MISSING,          // 2
    CG_ERROR_NOT_ADDRESS,           // 3
    CG_ERROR_HASH,                  // 4
    CG_ERROR_TABLE,                 // 5
    CG_ERROR_NOACCESS,              // 6
    CG_ERROR_FLASH,                 // 7
    CG_ERROR_EMM_ALREADY_PROCESSED, // 8
    CG_ERROR_ECM_ALREADY_PROCESSED, // 9
    CG_ERROR_SECTION_STRUCTURE,     // 10
    CG_ERROR_CARD,                  // 11
    CG_ERROR_OTA,                   // 12
    CG_ERROR_GET_CHIP_ID,           // 13
    CG_ERROR_UNKNOWN,               // 14
    CG_ERROR_CAID_NOTFOUND,         // 15
    CG_ERROR_KEY_UNKNOWN,           // 16
    CG_ERROR_ALGO_UNKNOWN,          // 17
    CG_ERROR_OTP_FUSE,              // 18
    CG_ERROR_OTP_SNR,               // 19
    CG_ERROR_INVALID_PIN,           // 20
    CG_ERROR_CRC,                   // 21
    CG_ERROR_VERSION_UNKNOWN,       // 22
    CG_ERROR_INVALID_SESSION,       // 23
    CG_ERROR_CRYPTO                 // 24
} cg_error_t;


/// Structure of data received in a OTA trigger callback
typedef struct {
	uint8_cg     ForcedUpgrade; ///< Forced upgrade
	uint32_cg    Manufacturer;  ///< Manufacturer ID used for OTA trigger, contact Cryptoguard to receive ID if needed
	uint32_cg    Model;         ///< Manufacturer model identifier
	uint32_cg    Version;       ///< Sw version in OTA stream
	uint32_cg    PID;           ///< Data PID
	uint32_cg    Delivery;      ///< 1 = Cable delivery, 2 = Terrestial delivery, 3 = Satellite
	uint32_cg    Frequency;     ///< frequency
	uint32_cg    Symbolrate;    ///< Symbolrate 0x1ADB => 6875
	uint16_cg    Fec;           ///< fec (satellite)
	uint16_cg    Polarity;      ///< polarity (satellite)
	uint16_cg    Modulation;    ///< modulation 2=qam64 3=qam128 4=qam256
	uint16_cg    Bandwith;      ///< bandwith (terrestial), 8 = 8mhz and 7 = 7mhz
	uint8_cg     StructVersion; ///< version of this structure
} ota_trigger_t;

/// Structure of data received in a OSD message callback
typedef struct {
	uint16_cg	ms_duration;		///< Duration (in milli seconds) for the message to be displayed
	uint8_cg	textstring[128];	///< Texstring to be displayed inlcuding if any character encoding
	uint8_cg	length;				///< Lenght of message to be displayed
} text_msg_t;

/// Structure of data received in a callback if video rules has changed.
/// CGMSA,HDCP,Macrovision,AnaHD,DigHD
typedef struct
{
	///@brief Copy Generation Management System - Analog workmodes.
    uint8_cg CGMSA;
    ///< 0: Neutral operation. (do nothing to the signal)
    ///< 1: Off, forced deactivation.
    ///< 2: On, supported units.
    ///< 3: On, mandatory on all units.

	///@brief High-bandwidth Digital Content Protection workmodes.
    uint8_cg HDCP;
    ///< 0: Neutral operation. (do nothing to the signal)
    ///< 1: Off, forced deactivation.
    ///< 2: On, supported units.
    ///< 3: On, mandatory on all units.

	///@brief Analog Copy Protection workmodes.
    uint8_cg Macrovision;
    ///< 0: Neutral operation. (do nothing to the signal)
    ///< 1: Off, forced deactivation.
    ///< 2: On, supported units.
    ///< 3: On, mandatory on all units.

	///@brief HD Video resolution restrictions on the analog output.
    uint8_cg AnaHD;
    ///< 0: Neutral operation. (do nothing to the signal)
    ///< 1: Max analogue resultion is 576i.

	///@brief HD Video resolution restrictions on the digital output.
    uint8_cg DigHD;
    ///< 0: Neutral operation. (do nothing to the signal)
    ///< 1: Max digital resultion is 1080p.
    ///< 2: Max digital resultion is 1080i.
    ///< 3: Max digital resultion is 720p.
    ///< 4: Max digital resultion is 720i.
    ///< 5: Max digital resultion is 576i.

	///@brief CH session number (0 for watched 1 for DVR).
	uint8_cg CH;

} ecm_stb_videorules_t;

/// Structure of data received in a EMM long message callback
typedef struct
{
    uint8_cg	Forced;				///< Flag for forced message, if set message should show full duration
    uint16_cg	Duration;			///< Duration (in seconds) to be displayed on screen.
    uint8_cg	Text[2048];			///< Texstring to be displayed inlcuding if any character encoding
} message_descriptor_t;

/// Structure of data received in a URI callback
typedef struct
{
	uint8_cg	Enabled;				///< URI enabled flag.
	uint8_cg	CH;						///< CH session number (0 for watched 1 for DVR).
	uint8_cg	Version;				///< Protocol version 0x02
	uint8_cg	aps_copy_control_info;	///< Analogue Protection System (APS)
	uint8_cg	emi_copy_control_info;	///< Encryption Mode Indicator (EMI)
	uint8_cg	ict_copy_control_info;	///< Image Constrained Trigger (ICT)
	uint8_cg	rct_copy_control_info;	///< Encryption Plus Non-assert (RCT)
	uint8_cg	dot_copy_control_info;	///< Digital Only Token (DOT)
	uint8_cg	rl_copy_control_info;	///< Retention limit of the recording and/or time-shift
	uint8_cg	reserved_bits;			///<
	uint8_cg	raw_data[8];			///<
} uri_descriptor_t;

/// Structure of data received in a RSD callback
typedef struct
{
	uint8_cg	Enabled;			///< RSD enabled flag.
	uint16_cg	Version;			///< RSD file version number.
} rsd_descriptor_t;

/// Structure of data received in a EMM force tune callback
typedef struct
{
	uint16_cg	DestinationSID;
	uint8_cg	tagvalue;			///< 0x5A = DVB-T,  0x44 = DVB-C,  0x43 = DVB-S
	uint8_cg	len;
	uint8_cg	descriptordata[11];	///< specified in Digital Video Broadcasting (DVB) Specification for Service Information (SI) in DVB systems (en_300468v011101p.pdf)
									///< depending on type "Satellite delivery system descriptor", "Cable delivery system descriptor" or "Terrestrial delivery system descriptor"
} delivery_descriptor_t;

/// Structure of data received in a EMM Scrolling message callback
typedef struct {
	uint16_cg 	Duration;   		///< 0-65535: Duration in seconds for display of scrolling message.
	uint8_cg  	Y;					///< 0-15: Placement Y cordinate
	uint32_cg	FontColor;			///< Font color
	uint32_cg	BackgroundColor;	///< Background color
	uint16_cg	ScrollSpeed;        ///< scrollspeed in ms
	uint8_cg	Forced;				///< Flag if message is forced type or normal
	uint8_cg	FontSize;			///< Font size used for text
	char		Text[256];			///< Texstring to be displayed including (if any) character encoding
} scroll_descriptor_t;

/// Structure of data received in a display fingerprint callback
typedef struct {
	char  		DisplayText[32];	///< This is a null terminated string to be displayed.
	uint16_cg 	DurationMs;			///< 0-65535: Duration in ms for display of fingerprint.
	uint8_cg	X;					///< 0-15: Placement X coordinate
	uint8_cg  	Y;					///< 0-15: Placement Y coordinate
	uint32_cg	FontColor;			///< Font color
	uint32_cg 	BackgroundColor;	///< Background color
	uint8_cg	CH;					///< Session number
} fingerprint_descriptor_t;

/// Structure of data received in a request pin unlock callback
typedef struct {
	char		text[32];
	uint8_cg	rating;
} pin_descriptor_t;

#ifdef __cplusplus
}
#endif
#endif	//_CG_CAS_TYPE_H
