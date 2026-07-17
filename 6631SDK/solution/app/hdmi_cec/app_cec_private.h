#ifndef __APP_CEC_PRIVATE_H__
#define __APP_CEC_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "app_config.h"
#if HDMI_CEC_SUPPORT
#include <gxtype.h>
#include "app.h"

#define CEC_OSD_STRING_LEN (13)
#define CEC_OSD_NAME_LEN (14)
#define CEC_MAX_AFC_NUM (4)
#define CEC_MAX_SHORT_AD_NUM (4)
#define CEC_MSG_PARAM_LEN_ZERO (0)
#define CEC_MSG_PARAM_LEN_ONE (1)
#define CEC_MSG_PARAM_LEN_TWO (2)
#define CEC_MSG_PARAM_LEN_THREE (3)
#define CEC_MSG_PARAM_LEN_FOUR (4)
#define CEC_MSG_PARAM_LEN_FIVE (5)
#define CEC_MSG_PARAM_LEN_SIX (6)
#define CEC_MSG_PARAM_LEN_SEVEN (7)
#define CEC_MSG_PARAM_LEN_EIGHT (8)
#define CEC_MSG_PARAM_LEN_NINE (9)
#define CEC_MSG_PARAM_LEN_TEN (10)
#define CEC_MSG_PARAM_LEN_ELEVEN (11)
#define CEC_MSG_PARAM_LEN_TWELVE (12)
#define CEC_MSG_PARAM_LEN_FOURTEEN (14)
#define IEEE_OUI_SONY (0x0013A9)
#define IEEE_OUI_SCE (0x0015C1) // Sony Computer Entertainment
#define IEEE_OUI_MEI (0x000B97) // Matsushita Electric Industrial Co.,Ltd. (Panasonic)
#define IEEE_OUI_LG (0x00E091) // LG ELECTRONICS, INC.
#define IEEE_OUI_SAMSUNG (0x0000F0) // SAMSUNG ELECTRONICS CO., LTD.
#define IEEE_OUI_PHILIPS (0x00903E) // PHILIPS
#define IEEE_OUI_MSTAR (0x4D5354) // MSTAR (KTC TV)
#define IEEE_OUI_NEXT (0x4E5854) // NEXT&NEXTSTAR
#define STB_VENDOR_ID IEEE_OUI_PHILIPS
#define INVALID_CEC_PHYSICAL_ADDRESS (0xFFFF)
#define FAKE_CEC_PHYSICAL_ADDRESS (0x1000)

typedef enum
{
    CEC_MSG_OK = 0,
    CEC_MSG_ERR_PARAM_VALUE,
    CEC_MSG_ERR_PAYLOAD_LENGTH,
    CEC_MSG_ERR_SRC_ADDR,
    CEC_MSG_ERR_DST_ADDR,
    CEC_MSG_UNKNOWN,
} E_CEC_MSG_ERRNO;

typedef enum
{
    // CDC_HEC
    CDC_HEC_INQUIRE_STATE = 0x00,
    CDC_HEC_REPORT_STATE = 0x01,
    CDC_HEC_SET_STATE_ADJACENT = 0x02,
    CDC_HEC_SET_STATE = 0x03,
    CDC_HEC_REQUEST_DEACTIVATION = 0x04,
    CDC_HEC_NOTIFY_ALIVE = 0x05,
    CDC_HEC_DISCOVER = 0x06,
    // CDC_HPD
    CDC_HPD_SET_STATE = 0x10,
    CDC_HPD_REPORT_STATE = 0x11,
} E_CDC_OPCODE;

//typedef and enum
typedef enum
{
    CEC_VER_1_1 = 0,
    CEC_VER_1_2 = 1,
    CEC_VER_1_2A = 2,
    CEC_VER_1_3 = 3,
    CEC_VER_1_3A = 4,
    CEC_VER_1_4A = 5, // added in CEC 1.4a
} E_CEC_MSG_CEC_VERSION;

typedef enum
{
    CEC_DEV_TV = 0,
    CEC_DEV_RECORD_DEV = 1,
    CEC_DEV_RESERVED = 2,
    CEC_DEV_TUNER = 3,
    CEC_DEV_PLAYBACK_DEV = 4,
    CEC_DEV_ADUIO_SYSTEM = 5,
    CEC_DEV_CEC_SWITCH = 6, // added in CEC 1.4a
    CEC_DEV_VIDEO_PROCESSOR = 7, // added in CEC 1.4a
} E_CEC_MSG_DEV_TYPE;

typedef enum
{
    MENU_REQ_ACTIVATE = 0,
    MENU_REQ_DEACTIVATE = 1,
    MENU_REQ_QUERY = 2,
}    E_CEC_MSG_MENU_REQ_TYPE;

typedef enum
{
    MENU_STATE_ACTIVATED = 0,
    MENU_STATE_DEACTIVATED = 1,
} E_CEC_MSG_MENU_STATE;

typedef enum
{
    CEC_AFC_RESERVED_0 = 0,
    CEC_AFC_LPCM,
    CEC_AFC_AC3 ,
    CEC_AFC_MPEG1,
    CEC_AFC_MP3,
    CEC_AFC_MPEG2,
    CEC_AFC_AAC,
    CEC_AFC_DTS,
    CEC_AFC_ATRAC,
    CEC_AFC_ONE_BIT_AUDIO,
    CEC_AFC_DD_PLUS,
    CEC_AFC_DTS_HD,
    CEC_AFC_MAT,
    CEC_AFC_DST,
    CEC_AFC_BYE1_PRO,
    CEC_AFC_RESERVED_15,
} E_CEC_AUDIO_FORMAT_CODE;

typedef enum
{
    // One Touch Play
    OPCODE_TEXT_VIEW_ON = 0x0D,
    OPCODE_ACTIVE_SOURCE = 0x82,
    OPCODE_IMAGE_VIEW_ON = 0x04,
    // Routing Control Feature
    OPCODE_ROUTING_CHANGE = 0x80,
    OPCODE_INACTIVE_SOURCE = 0x9D,
    OPCODE_SET_STREAM_PATH = 0x86,
    OPCODE_ROUTING_INFORMATION = 0x81,
    OPCODE_REQUEST_ACTIVE_SOURCE = 0x85,
    // Standby Feature
    OPCODE_SYSTEM_STANDBY = 0x36,
    // One Touch Record
    OPCODE_RECORD_ON = 0x09,
    OPCODE_RECORD_OFF = 0x0B,
    OPCODE_RECORD_STATUS = 0x0A,
    OPCODE_RECORD_TV_SCREEN = 0x0F,
    // Timer Programming
    OPCODE_TIMER_STATUS = 0x35,
    OPCODE_SET_ANALOG_TIMER = 0x34,
    OPCODE_SET_DIGITAL_TIMER = 0x97,
    OPCODE_CLEAR_ANALOG_TIMER = 0x33,
    OPCODE_SET_EXTERNAL_TIMER = 0xA2,
    OPCODE_CLEAR_DIGITAL_TIMER = 0x99,
    OPCODE_TIMER_CLEARED_STATUS = 0x43,
    OPCODE_CLEAR_EXTERNAL_TIMER = 0xA1,
    OPCODE_SET_TIMER_PROGRAM_TITLE = 0x67,
    // System Information
    OPCODE_CEC_VERSION = 0x9E,
    OPCODE_GET_CEC_VERSION = 0x9F,
    OPCODE_GET_MENU_LANGUAGE = 0x91,
    OPCODE_SET_MENU_LANGUAGE = 0x32,
    OPCODE_GIVE_PHYSICAL_ADDR = 0x83,
    OPCODE_REPORT_PHYSICAL_ADDR = 0x84,
    // Deck Control
    OPCODE_PLAY = 0x41,
    OPCODE_DECK_STATUS = 0x1B,
    OPCODE_DECK_CONTROL = 0x42,
    OPCODE_GIVE_DECK_STATUS = 0x1A,
    //Tuner control
    OPCODE_TUNER_DEVICE_STATUS = 0x07,
    OPCODE_TUNER_STEP_DECREMENT = 0x06,
    OPCODE_TUNER_STEP_INCREMENT = 0x05,
    OPCODE_SELECT_ANALOG_SERVICE = 0x92,
    OPCODE_SELECT_DIGITAL_SERVICE = 0x93,
    OPCODE_GIVE_TUNER_DEVICE_STATUS = 0x08,
    // Vendor Specific Commands
    OPCODE_VENDOR_COMMAND = 0x89,
    OPCODE_DEVICE_VENDOR_ID = 0x87,
    OPCODE_GIVE_DEVICE_VENDOR_ID = 0x8C,
    OPCODE_VENDOR_COMMAND_WITH_ID = 0xA0,
    OPCODE_VENDOR_REOMTE_BUTTON_UP = 0x8B,
    OPCODE_VENDOR_REOMTE_BUTTON_DOWN = 0x8A,
    // OSD Status Display
    OPCODE_SET_OSD_STRING = 0x64,
    // Device OSD Transfer
    OPCODE_SET_OSD_NAME = 0x47,
    OPCODE_GIVE_OSD_NAME = 0x46,
    // Device Menu Control
    OPCODE_MENU_STATUS = 0x8E,
    OPCODE_MENU_REQUEST = 0x8D,
    // Remote Control Passthrough
    OPCODE_USER_CTRL_PRESSED = 0x44,
    OPCODE_USER_CTRL_RELEASED = 0x45,
    // Power STATUS
    OPCODE_GIVE_POWER_STATUS = 0x8F,
    OPCODE_REPORT_POWER_STATUS = 0x90,
    // General Protocol Message
    OPCODE_ABORT = 0xFF,
    OPCODE_FEATURE_ABORT = 0x00,
    // System Audio Control
    OPCODE_GIVE_AUDIO_STATUS = 0x71,
    OPCODE_REPORT_AUDIO_STATUS = 0x7A,
    OPCODE_SET_SYSTEM_AUDIO_MODE = 0x72,
    OPCODE_SYSTEM_AUDIO_MODE_STATUS = 0x7E,
    OPCODE_SYSTEM_AUDIO_MODE_REQUEST = 0x70,
    OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D,
    OPCODE_REPORT_SHORT_AUDIO_DESCRIPTOR = 0xA3, // CEC V1.4 New MSG
    OPCODE_REQUEST_SHORT_AUDIO_DESCRIPTOR = 0xA4, // CEC V1.4 New MSG
    // Audio Rate Control
    OPCODE_SET_AUDIO_RATE = 0x9A,
    // Audio Return Channel Control (CEC V1.4 New MSG)
    OPCODE_INITIATE_ARC = 0xC0,
    OPCODE_TERMINAE_ARC = 0xC5,
    OPCODE_REPORT_ARC_INITIATED = 0xC1,
    OPCODE_REPORT_ARC_TERMINATED = 0xC2,
    OPCODE_REQUEST_ARC_INITIATION = 0xC3,
    OPCODE_REQUEST_ARC_TERMINATION = 0xC4,
    // Capability Discovery and Control Feature (CEC V1.4 New MSG)
    OPCODE_CDC_MESSAGE = 0xF8,
}    E_CEC_OPCODE;

typedef enum
{
    CEC_LA_TV = 0x0,
    CEC_LA_RECORD_1 = 0x1,
    CEC_LA_RECORD_2 = 0x2,
    CEC_LA_TUNER_1 = 0x3,
    CEC_LA_PLAYBACK_1 = 0x4,
    CEC_LA_AUDIO_SYSTEM = 0x5,
    CEC_LA_TUNER_2 = 0x6,
    CEC_LA_TUNER_3 = 0x7,
    CEC_LA_PLAYBACK_2 = 0x8,
    CEC_LA_RECORD_3 = 0x9,
    CEC_LA_TUNER_4 = 0xA,
    CEC_LA_PLAYBACK_3 = 0xB,
    CEC_LA_RESERVED_1 = 0xC,
    CEC_LA_RESERVED_2 = 0xD,
    CEC_LA_FREE_USE = 0xE,
    CEC_LA_BROADCAST = 0xF,
} E_CEC_LOGIC_ADDR;

typedef enum
{
    POW_ON = 0,
    POW_STANDBY = 1,
    POW_STANDBY_2_ON = 2,
    POW_ON_2_STANDBY = 3,
} E_CEC_MSG_POW_STAUS;

typedef enum
{
    CEC_MENU_STATE_ACTIVATE = 0,
    CEC_MENU_STATE_DEACTIVATE,
    CEC_MENU_STATE_UNINITED,
} E_CEC_MENU_STATE;

typedef enum
{
    CEC_SOURCE_STATE_IDLE = 0,
    CEC_SOURCE_STATE_ACTIVE,
} E_CEC_SOURCE_STATE;

typedef enum
{
    CEC_LOCAL_STANDBY_MODE = 0,
    CEC_SYSTEM_STANDBY_FEATURE_MODE,
} E_CEC_STANDBY_MODE;

typedef enum
{
    CEC_KEY_SELECT = 0x0,
    CEC_KEY_UP,
    CEC_KEY_DOWN,
    CEC_KEY_LEFT,
    CEC_KEY_RIGHT,
    CEC_KEY_RIGHT_UP,
    CEC_KEY_RIGHT_DOWN,
    CEC_KEY_LEFT_UP,
    CEC_KEY_LEFT_DOWN,
    CEC_KEY_ROOT_MENU,
    CEC_KEY_SETUP_MENU,
    CEC_KEY_CONTENT_MENU,
    CEC_KEY_FAVORITE_MENU,
    CEC_KEY_EXIT,
    CEC_KEY_MEDIA_TOP_MENU = 0x10,
    CEC_KEY_MEDIA_CONTEXT,
    CEC_KEY_NUMBER_ENTRY_MODE = 0x1D,
    CEC_KEY_NUMBER_11,
    CEC_KEY_NUMBER_12,
    CEC_KEY_NUMBER_0 = 0x20,
    CEC_KEY_NUMBER_1,
    CEC_KEY_NUMBER_2,
    CEC_KEY_NUMBER_3,
    CEC_KEY_NUMBER_4,
    CEC_KEY_NUMBER_5,
    CEC_KEY_NUMBER_6,
    CEC_KEY_NUMBER_7,
    CEC_KEY_NUMBER_8,
    CEC_KEY_NUMBER_9,
    CEC_KEY_DOT,
    CEC_KEY_ENTER,
    CEC_KEY_CLEAR,
    CEC_KEY_NEXT_FAVORITE = 0x2F,
    CEC_KEY_CHANNEL_UP,
    CEC_KEY_CHANNEL_DOWN,
    CEC_KEY_PREVIOUS_CHANNEL,
    CEC_KEY_SOUND_SELECT,
    CEC_KEY_INPUT_SELECT,
    CEC_KEY_DISPLAY_INFORMATION,
    CEC_KEY_HELP,
    CEC_KEY_PAGE_UP,
    CEC_KEY_PAGE_DOWN,
    CEC_KEY_POWER = 0x40,
    CEC_KEY_VOLUME_UP,
    CEC_KEY_VOLUME_DOWN,
    CEC_KEY_MUTE,
    CEC_KEY_PLAY,
    CEC_KEY_STOP,
    CEC_KEY_PAUSE,
    CEC_KEY_RECORD,
    CEC_KEY_REWIND,
    CEC_KEY_FAST_FORWARD,
    CEC_KEY_EJECT,
    CEC_KEY_FORWARD,
    CEC_KEY_BACKWARD,
    CEC_KEY_STOP_RECORD,
    CEC_KEY_PAUSE_RECORD,
    CEC_KEY_ANGLE = 0x50,
    CEC_KEY_SUB_PICTURE,
    CEC_KEY_VIDEO_ON_DEMAND,
    CEC_KEY_EPG,
    CEC_KEY_TIMER_PROGRAMMING,
    CEC_KEY_INITIAL_CONFIGURATION,
    CEC_KEY_SELECT_BROADCAST_TYPE,
    CEC_KEY_SELECT_SOUND_PRESENTATION,
    CEC_KEY_PLAY_FUNCTION = 0x60,
    CEC_KEY_PAUSE_PLAY_FUNCTION,
    CEC_KEY_RECORD_FUNCTION,
    CEC_KEY_PAUSE_RECORD_FUNCTION,
    CEC_KEY_STOP_FUNCTION,
    CEC_KEY_MUTE_FUNCTION,
    CEC_KEY_RESTORE_VOLUME_FUNCTION,
    CEC_KEY_TUNE_FUNCTION,
    CEC_KEY_SELECT_MEDIA_FUNCTION,
    CEC_KEY_SELECT_AV_INPUT_FUNCTION,
    CEC_KEY_SELECT_AUDIO_INPUT_FUNCTION,
    CEC_KEY_POWER_TOGGLE_FUNCTION,
    CEC_KEY_POWER_OFF_FUNCTION,
    CEC_KEY_POWER_ON_FUNCTION,
    CEC_KEY_BLUE = 0x71,
    CEC_KEY_RED,
    CEC_KEY_GREEN,
    CEC_KEY_YELLOW,
    CEC_KEY_F5,
    CEC_KEY_DATA,
    CEC_KEY_IPTV = 0xa0,
    CEC_KEY_SAT,
    CEC_KEY_TV_RADIO,
    CEC_KEY_RESOLUTION,
    CEC_KEY_M_PLUS,
    CEC_KEY_INF_PLUS,
    CEC_KEY_COLLECT,
} E_CEC_KEY_CODE;

typedef enum
{
    CHANNEL_NUM_1_PART = 0x01,
    CHANNEL_NUM_2_PART = 0x02,
} E_CHANNEL_NUM_FORAMT;

typedef enum
{
    SERVICE_BY_DIGITAL_ID = 0x00,
    SERVICE_BY_CAHNNEL = 0x01,
} E_SERVICE_ID_METHOD;

typedef enum
{
    BCAST_ARIB_GENERIC = 0x00,
    BCAST_ATSC_GENERIC = 0x01,
    BCAST_DVB_GENERIC = 0x02,
    BCAST_ARIB_BS = 0x08,
    BCAST_ARIB_CS = 0x09,
    BCAST_ARIB_T = 0x0A,
    BCAST_ATSC_C = 0x10,
    BCAST_ATSC_S = 0x11,
    BCAST_ATSC_T = 0x12,
    BCAST_DVB_C = 0x18,
    BCAST_DVB_S = 0x19,
    BCAST_DVB_S2 = 0x1A,
    BCAST_DVB_T = 0x1B,
} E_DIGITAL_BROADCAST_SYSTEM;

typedef enum
{
    CEC_SYS_CALL_SHOW_OSD_MSG = 0,
    CEC_SYS_CALL_BY_CEC_OPCODE,
} E_CEC_SYS_CALL_TYPE;

typedef enum
{
    CEC_STATUS_REQUEST_ON = 1,
    CEC_STATUS_REQUEST_OFF = 2,
    CEC_STATUS_REQUEST_ONCE = 3,
} E_CEC_TUNER_STATUS_REQUEST;

typedef enum
{
    CEC_MENU_REQUEST_ACTIVATE = 0,
    CEC_MENU_REQUEST_DEACTIVATE,
    CEC_MENU_REQUEST_QUERY,

} E_CEC_MENU_REQUEST;

typedef enum
{
    REASON_UNRECOGNIZE_OPCODE = 0,
    REASON_NOT_IN_CORRECT_MODE = 1,
    REASON_CANNOT_PROVIDE_SOURCE = 2,
    REASON_INVALID_OPRAND = 3,
    REASON_REFUSED = 4,
    REASON_UNABLE_DETERMINE = 5, // added in CEC 1.4a
} E_CEC_FEATURE_ABORT_REASON;

typedef enum
{
    //One Touch Play
    CEC_CMD_IMAGE_VIEW_ON,
    CEC_CMD_TEXT_VIEW_ON,
    //Routing Control
    CEC_CMD_ACTIVE_SOURCE,
    CEC_CMD_INACTIVE_SOURCE, // System will go to standby, need to send message to inform the TV.
    CEC_CMD_REQUEST_ACTIVE_SOURCE,
    CEC_CMD_ROUTING_CHANGE,
    CEC_CMD_ROUTING_INFOMATION,
    CEC_CMD_SET_STREAM_PATH,
    //Sytem Standby
    CEC_CMD_SYSTEM_STANDBY,
    //One Touch Record
    CEC_CMD_RECORD_OFF,
    CEC_CMD_RECORD_ON,
    CEC_CMD_RECORD_STATUS,
    CEC_CMD_RECORD_TV_SCREEN,
    //Timer Programming
    CEC_CMD_CLEAR_ANALOGUE_TIMER,
    CEC_CMD_CLEAR_DIGITAL_TIMER,
    CEC_CMD_CLEAR_EXTERNAL_TIMER,
    CEC_CMD_SET_ANALOGUE_TIMER,
    CEC_CMD_SET_DIGITAL_TIMER,
    CEC_CMD_SET_EXTERNAL_TIMER,
    CEC_CMD_SET_TIMER_PROGRAM_TITLE,
    CEC_CMD_TIMER_CLEARED_STATUS,
    CEC_CMD_TIMER_STATUS,
    // System Information
    CEC_CMD_CEC_VERSION,
    CEC_CMD_GET_CEC_VERSION,
    CEC_CMD_GIVE_PHYSICAL_ADDRESS,
    CEC_CMD_GET_MENU_LANGUAGE,
    CEC_CMD_POLLING_MESSAGE,
    CEC_CMD_REPORT_PHYSICAL_ADDRESS,
    CEC_CMD_SET_MENU_LANGUAGE,
    //Deck Control
    CEC_CMD_DECK_CONTROL,
    CEC_CMD_DECK_STATUS,
    CEC_CMD_GIVE_DECK_STATUS,
    CEC_CMD_PLAY,
    //Tuner Control
    CEC_CMD_GIVE_TUNER_DEVICE_STATUS,
    CEC_CMD_SELECT_ANALOGUE_SERVICE,
    CEC_CMD_SELECT_DIGITAL_SERVICE,
    CEC_CMD_TUNER_DEVICE_STATUS,
    CEC_CMD_TUNER_STEP_DECREMENT,
    CEC_CMD_TUNER_STEP_INCREMENT,
    CEC_CMD_DEVICE_VENDOR_ID,
    CEC_CMD_GIVE_DEVICE_VENDOR_ID,
    CEC_CMD_VENDOR_COMMAND,
    CEC_CMD_VENDOR_COMMAND_WITH_ID,
    CEC_CMD_VENDOR_REMOTE_BUTTON_DOWN,
    CEC_CMD_VENDOR_REMOTE_BUTTON_UP,
    //OSD Display Feature
    CEC_CMD_SET_OSD_STRING,
    //Device OSD Transfer
    CEC_CMD_GIVE_OSD_NAME,
    CEC_CMD_SET_OSD_NAME,
    //Device Menu Control
    CEC_CMD_MENU_REQUEST,
    CEC_CMD_MENU_STATUS,
    //Remote Control Passthrough
    CEC_CMD_USER_CONTROL_PRESSED,
    CEC_CMD_USER_CONTROL_RELEASED,
    //Power Status
    CEC_CMD_GIVE_DEVICE_POWER_STATUS,
    CEC_CMD_REPORT_POWER_STATUS,
    //General Protocol
    CEC_CMD_FEATURE_ABORT,
    CEC_CMD_ABORT,
    //System Audio Control
    CEC_CMD_GIVE_AUDIO_STATUS,
    CEC_CMD_GIVE_SYSTEM_AUDIO_MODE_STATUS,
    CEC_CMD_REPORT_AUDIO_STATUS,
    CEC_CMD_REPORT_SHORT_AUDIO_DESCRIPTOR,
    CEC_CMD_REQUEST_SHORT_AUDIO_DESCRIPTOR,
    CEC_CMD_SET_SYSTEM_AUDIO_MODE,
    CEC_CMD_SYSTEM_AUDIO_MODE_REQUEST,
    CEC_CMD_SYSTEM_AUDIO_MODE_STATUS,
    //Audio Rate Control
    CEC_CMD_SET_AUDIO_RATE,
    //Audio Return Channel
    CEC_CMD_INITIATE_ARC,
    CEC_CMD_REPORT_ARC_INITIATED,
    CEC_CMD_REPORT_ARC_TERMINATED,
    CEC_CMD_REQUEST_ARC_INITIATION,
    CEC_CMD_REQUEST_ARC_TERMINATION,
    CEC_CMD_TERMINATE_ARC,
} E_CEC_AP_CMD_TYPE;

typedef enum
{
	OWN_SOURCE = 1,
	DIGITAL_SERVICE = 2,
	ANALOGUE_SERVICE = 3,
	EXTERNAL_PLUG = 4,
	EXTERNAL_PHYSICAL_ADDRESS = 5,
}E_CEC_RECORD_SOURCE_TYPE;

typedef enum
{
	RECORD_CURRENT_SELECT_SOURCE = 0x01,
	RECORD_DIGITAL_SERVICE = 0x02,
	RECORD_ANALOGUE_SERVICE = 0x03,
	RECORD_EXTERNAL_INPUT = 0x04,
	NO_RECORD_UNABLE_TO_RECORD_DIGITAL_SERVICE = 0x05,
	NO_RECORD_UNABLE_TO_RECORD_ANALOGUE_SERVICE = 0x06,
	NO_RECORD_UNABLE_TO_SELECT_REQUIRED_SERVICE = 0x07,
	NO_RECORD_INVALID_EXTERNAL_PLUG_NUMBER = 0x09,
	NO_RECORD_INVALID_EXTERNAL_PHYSICAL_ADDRESS = 0x0A,
	NO_RECORD_CA_SYSTEM_NOT_SUPPORTED = 0x0B,
	NO_RECORD_NO_OR_INSUFFICIENT_CA_ENTITLEMENTS = 0x0C,
	NO_RECORD_NOT_ALLOWED_TO_COPY_SOURCE = 0x0D,
	NO_RECORD_NO_FURTHER_COPIES_ALLOWED = 0x0E,
	NO_RECORD_NO_MEDIA = 0x10,
	NO_RECORD_PLAYING = 0x11,
	NO_RECORD_ALREADY_RECORDING = 0x12,
	NO_RECORD_MEDIA_PROTECTED = 0x13,
	NO_RECORD_NO_SOURCE_SIGNAL = 0x14,
	NO_RECORD_MEDIA_PROBLEM = 0x15,
	NO_RECORD_NOT_ENOUGH_SPACE_AVAILABLE = 0x16,
	NO_RECORD_PARENTAL_LOCK_ON = 0x17,
	RECORD_TERMINATED_NORMALLY = 0x1A,
	RECORD_HAS_ALREADY_TERMINATED = 0x1B,
	NO_RECORD_OTHER_REASON = 0x1F,
}E_CEC_RECORD_STATUS_INFO;

typedef enum
{
	DECK_CONTROL_MODE_SKIP_FORWARD_WIND = 1,
	DECK_CONTROL_MODE_SKIP_REVERSE_REWIND = 2,
	DECK_CONTROL_MODE_STOP = 3,
	DECK_CONTROL_MODE_EJECT = 4,
}E_CEC_DECK_CONTROL_MODE;

typedef enum
{
	DECK_INFO_PLAY = 0x11,
	DECK_INFO_RECORD = 0x12,
	DECK_INFO_PLAY_REVERSE = 0x13,
	DECK_INFO_STILL = 0x14,
	DECK_INFO_SLOW = 0x15,
	DECK_INFO_SLOW_REVERSE = 0x16,
	DECK_INFO_FAST_FORWARD = 0x17,
	DECK_INFO_FAST_REVERSE = 0x18,
	DECK_INFO_NO_MEDIA = 0x19,
	DECK_INFO_STOP = 0x1A,
	DECK_INFO_SKIP_FORWARD_WIND = 0x1B,
	DECK_INFO_SKIP_REVERSE_REWIND = 0x1C,
	DECK_INFO_INDEX_SEARCH_FORWARD = 0x1D,
	DECK_INFO_INDEX_SEARCH_REVERSE = 0x1E,
	DECK_INFO_OTHER_STATUS = 0x1F,
}E_CEC_DECK_STATUS_INFO;

typedef enum
{
	PLAY_MODE_PLAY_FORWARD = 0x24,
	PLAY_MODE_PLAY_REVERSE = 0x20,
	PLAY_MODE_PLAY_STILL = 0x25,
	PLAY_MODE_FAST_FORWARD_MIN_SPEED = 0x05,
	PLAY_MODE_FAST_FORWARD_MEDIUM_SPEED = 0x06,
	PLAY_MODE_FAST_FORWARD_MAX_SPEED = 0x07,
	PLAY_MODE_FAST_REVERSE_MIN_SPEED = 0x09,
	PLAY_MODE_FAST_REVERSE_MEDIUM_SPEED = 0x0A,
	PLAY_MODE_FAST_REVERSE_MAX_SPEED = 0x0B,
	PLAY_MODE_SLOW_FORWARD_MIN_SPEED = 0x15,
	PLAY_MODE_SLOW_FORWARD_MEDIUM_SPEED = 0x16,
	PLAY_MODE_SLOW_FORWARD_MAX_SPEED = 0x17,
	PLAY_MODE_SLOW_REVERSE_MIN_SPEED = 0x19,
	PLAY_MODE_SLOW_REVERSE_MEDIUM_SPEED = 0x1A,
	PLAY_MODE_SLOW_REVERSE_MAX_SPEED = 0x1B,
}E_CEC_DECK_PLAY_MODE;

typedef enum{
	CEC_TIMER_TYPE_SUNDAY = (1<<0),
	CEC_TIMER_MONDAY = (1<<1),
	CEC_TIMER_TUESDAY = (1<<2),
	CEC_TIMER_WEDNESDAY = (1<<3),
	CEC_TIMER_THURSDAY = (1<<4),
	CEC_TIMER_FRIDAY = (1<<5),
	CEC_TIMER_SATURDAY = (1<<6),
	CEC_TIMER_ONCE_ONLY = (0<<0),
} E_CEC_TIMER_RECORDING_SEQUENCE;


typedef enum{
	TIMER_NOT_CLEARED_RECORDING = 0x00,
	TIMER_NOT_CLEARED_NO_MATCHING = 0x01,
	TIMER_NOT_CLEARED_NO_INFO_AVAILABLE = 0x02,
	TIMER_CLEARED = 0x80,
} E_CEC_TIMER_CLEAR_STATUS;

typedef struct
{
    unsigned char opcode;
	unsigned char dest_address;
	unsigned char source_address;
    E_CEC_FEATURE_ABORT_REASON reason;
} CEC_FEATURE_ABORT_INFO, *P_CEC_FEATURE_ABORT_INFO;

typedef struct
{
    unsigned char opcode;
    unsigned char param[14];
    unsigned char param_length;
	unsigned char request_type;
	unsigned char dest_address;
	unsigned char source_address;
} CEC_SYS_CALL_MSG_T, *P_CEC_SYS_CALL_MSG_T;

/* Protocol Related Structure are Big-Endian, there special handle is required !*/
typedef struct
{
    unsigned short service_id;
	unsigned short transport_stream_id;
	unsigned short original_network_id;
} ARIB_SERVICE_ID;

typedef struct
{
     unsigned short reserved;
	 unsigned short program_number;
	 unsigned short transport_stream_id;
} ATSC_SERVICE_ID;

typedef struct
{
     unsigned short service_id;
     unsigned short transport_stream_id;
     unsigned short original_network_id;
} DVB_SERVICE_ID;

typedef struct  __attribute__((packed)) _CHANNEL_DATA
{
     unsigned short major_ch_num :10; // for 1-part channel number, this operand should be ignored.
                                        // for 2-part channel number, present a 3-digital major channel number in hex format
     unsigned short ch_numr_format :6; // 0x01: 1-part channel number
                                        // 0x02: 2-part channel number
     unsigned short min_ch_num; // for 1-part channel number, present a channel number in hex format
                                        // for 2-part channel number, present a minor channel number in hex format
     unsigned short reserved;
} CHANNEL_DATA;

typedef union
{
    DVB_SERVICE_ID dvb;
	ATSC_SERVICE_ID atsc;
	ARIB_SERVICE_ID arib;
    CHANNEL_DATA channel;
} SERVICE_ID;

typedef struct  __attribute__((__packed__)) _DIGITAL_SERVICE_ID
{
    /* Big-Endian Structure Layout */
     unsigned char broadcast_system :7; // 0x00: ARIB generic
                                                    // 0x01: ATSC generic
                                                    // 0x02: DVB generic
                                                    // 0x08: ARIB-BS
                                                    // 0x09: ARIB-CS
                                                    // 0x0A: ARIB-T
                                                    // 0x10: ATSC Cable
                                                    // 0x11: ATSC Satellite
                                                    // 0x12: ATSC Terrestrial
                                                    // 0x18: DVB-C
                                                    // 0x19: DVB-S
                                                    // 0x1A: DVB-S2
                                                    // 0x1B: DVB-T
     unsigned char service_id_method :1; // 0: Service id by Digital IDs
                                                    // 1: Service id by Channel

    SERVICE_ID  service_id; // 6 Bytes

} DIGITAL_SERVICE_ID, *P_DIGITAL_SERVICE_ID;        // 7

typedef struct __attribute__((__packed__)) _CEC_TUNER_DEVICE_INFO
{
    /* Big-Endian Structure Layout */
    unsigned char tuner_display_info :7; // 0 Displaying Digital Tuner
                                         // 1 Not Displaying Tuner
                                         // 2 Displaying Analog Tuner

    unsigned char recording_flag :1; // 0 Not used for Recording
                                    // 1 Being Used for Recording
    DIGITAL_SERVICE_ID digital_service_id;
} CEC_TUNER_DEVICE_INFO, *P_CEC_TUNER_DEVICE_INFO; // 8 Bytes

typedef struct __attribute__((__packed__)) _CEC_RECORD_SOURCE_INFO
{
    E_CEC_RECORD_SOURCE_TYPE record_source_type;
    DIGITAL_SERVICE_ID digital_service_id;
} CEC_RECORD_SOURCE_INFO, *P_CEC_RECORD_SOURCE_INFO;

typedef struct __attribute__((__packed__)) _CEC_DIGITAL_TIMER_INFO
{
    unsigned char day_of_month;
	unsigned char month_of_year;
	unsigned short start_time;
	unsigned short duration;
    unsigned char recording_sequence :7;
	unsigned char reserved_bit :1;
    DIGITAL_SERVICE_ID digital_service_id;
} CEC_DIGITAL_TIMER_INFO, *P_CEC_DIGITAL_TIMER_INFO;

typedef struct __attribute__((__packed__)) _CEC_TIMER_STATUS_DATA
{
    unsigned char timer_overlap_warning :1;
	unsigned char media_info :2;
	unsigned char programmed_indicator :1;
	unsigned char programmed_info :4;
	unsigned char not_programmed_error_info :4;
	unsigned short duration_available;
} CEC_TIMER_STATUS_DATA, *P_CEC_TIMER_STATUS_DATA;

typedef struct
{
    E_CEC_TUNER_STATUS_REQUEST request_type;
} CEC_STATUS_REQUEST, *P_CEC_STATUS_REQUEST;

typedef struct
{
    unsigned char lang[3];
} CEC_MENU_LANGUAGE, *P_CEC_MENU_LANGUAGE;

typedef struct
{
    E_CEC_OPCODE opcode;
    E_CEC_FEATURE_ABORT_REASON reason;
} CEC_FEATURE_ABORT, *P_CEC_FEATURE_ABORT;

typedef struct
{
    E_CEC_AP_CMD_TYPE type;
    E_CEC_LOGIC_ADDR dest_addr;
    unsigned char params[CEC_OSD_NAME_LEN+1];  // MAX Param Size, add one for NULL chracter
} CEC_CMD, *P_CEC_CMD;

typedef struct    _CEC_OSD_STRING
{
    /* Big-Endian Structure Layout */
    unsigned char bit5_bit0 :6; //should be zero
    unsigned char display_control :2; //00 Display for default time
                                        //01 Display unitl cleared
                                        //10 Clear Previous message
                                        //11 reserved for future use
    unsigned char string[CEC_OSD_STRING_LEN+1]; //last one for NULL chracter
} CEC_OSD_STRING, *P_CEC_OSD_STRING;

typedef struct _CEC_OSD_NAME
{
    unsigned char name[14+1]; // last one for NULL chracter
} CEC_OSD_NAME, *P_CEC_OSD_NAME;

typedef struct
{
    E_CEC_LOGIC_ADDR device_logical_address;
    unsigned short device_physical_address;
    unsigned int device_vendor_id;
    unsigned char device_cec_version;
    unsigned short remote_passthrough_key;
    E_CHANNEL_NUM_FORAMT channel_num_format; // Tuner Report Mode
    E_SERVICE_ID_METHOD service_id_method; // Tuner Report Mode
    unsigned short tuner_status_subscribers;
    E_CEC_SOURCE_STATE source_active_state;
    E_CEC_MENU_STATE menu_active_state;
    E_CEC_STANDBY_MODE cec_device_standby_mode; //0: local standby, 1: system standby
    bool cec_func_enable;
    bool cec_feature_rcp_enable; //remote control passthrough
    bool cec_feature_sac_enable; //system audio control
    bool cec_osd_popup_enable;	
	bool cec_use_tv_remote_control_enable;
	bool cec_tv_on_stb_activity_enable;
	bool cec_stb_standby_tv_ativity_enable;
    E_CEC_LOGIC_ADDR bus_active_source_logical_address;
    unsigned short bus_physical_address[16];
    bool system_audio_mode_status; //system audio mode
    unsigned char system_audio_status; //AMP's volum + mute info
    CEC_FEATURE_ABORT_INFO cec_feature_abort_info; //last received cec feature abort info
} CEC_CONFIG;

typedef struct
{
    E_CEC_OPCODE opcode;
	E_CEC_LOGIC_ADDR dest_address;
	E_CEC_LOGIC_ADDR source_address;
	unsigned char param[16];
	unsigned short param_length;
} CEC_OPCODE_HANDLER, *P_CEC_OPCODE_HANDLER;

#ifdef __cplusplus
 }
#endif
#endif
#endif // _CEC_PRIVATE_H_
