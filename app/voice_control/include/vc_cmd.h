#ifndef __VC_CMD_H__
#define __VC_CMD_H__

typedef enum _VoiceCMDIDEnum{
    // Basic commands
    VOICE_CMD_ACTIVISION       = 1,
    VOICE_CMD_NOT_DETECTED     = 2,
    VOICE_CMD_TO_IDLE          = 3,
    VOICE_CMD_DATA_BEGIN       = 4,
    VOICE_CMD_DATA_END         = 5,
    // Quick action commands
    VOICE_CMD_VOLUME_UP        = 100,
    VOICE_CMD_VOLUME_DOWN,
    VOICE_CMD_STANDBY,
    VOICE_CMD_WAKEUP,
    VOICE_CMD_MUTE,
    VOICE_CMD_UNMUTE,
    VOICE_CMD_END_RECORD,
    VOICE_CMD_STOP_PLAY,
    VOICE_CMD_PAUSE,
    VOICE_CMD_PLAY,
    VOICE_CMD_PRE,//Switch to the previous channel
    VOICE_CMD_NEXT,//Switch to the next channel
    VOICE_CMD_OPEN_EPG,
    VOICE_CMD_OPEN_MAINMENU,
    VOICE_CMD_GOTO_FULLSCREEN,
    VOICE_CMD_SWITCH_FORMAT,
    VOICE_CMD_REC,
    VOICE_CMD_PRE_FILE,
    VOICE_CMD_NEXT_FILE,
    VOICE_CMD_FAST_BACKWARD,
    VOICE_CMD_FAST_FORWARD,
    VOICE_CMD_OPEN_TELETEXT,
    VOICE_CMD_RECALL_PROG,
    VOICE_CMD_MARK_PROG,
    VOICE_CMD_UNMARK_PROG,
    VOICE_CMD_OPEN_MARKLIST,
    VOICE_CMD_GOTO_MARKCH_1,
    VOICE_CMD_GOTO_MARKCH_2,
    VOICE_CMD_GOTO_MARKCH_3,
    VOICE_CMD_GOTO_MARKCH_4,
    VOICE_CMD_GOTO_MARKCH_5,
    VOICE_CMD_GOTO_MARKCH_6,
    VOICE_CMD_GOTO_MARKCH_7,
    VOICE_CMD_GOTO_MARKCH_8,
    VOICE_CMD_GOTO_MARKCH_9,
    VOICE_CMD_GOTO_MARKCH_10,
    // Cloud cmd
    VOICE_CMD_OPEN,
    VOICE_CMD_VOLUME,
    VOICE_CMD_PLAY_BY_NMAE,
    VOICE_CMD_PLAY_BY_NUM,
    VOICE_CMD_SEEK_FORWARD,
    VOICE_CMD_SEEK_BACKWARD,
    VOICE_CMD_SEEK_TO,
    VOICE_CMD_STANDBY_TV,
    VOICE_CMD_WAKEUP_TV,
    VOICE_CMD_OPEN_APPLETS,
    VOICE_CMD_NETAPPS_EXIT,
    VOICE_CMD_NETAPPS_SEARCH,
    VOICE_CMD_NETAPPS_CATEGORY,
    VOICE_CMD_NETAPPS_GROUP,
    VOICE_CMD_NETAPPS_PAGE,
    VOICE_CMD_NETAPPS_PLAY,
    VOICE_CMD_NETAPPS_SETTING,
    VOICE_CMD_NETAPPS_USER,
    VOICE_CMD_NETAPPS_PRIVATE,

    // Remote key command
    VOICE_CMD_KEY_POWER         = 201,
    VOICE_CMD_KEY_MUTE,
    VOICE_CMD_KEY_1,
    VOICE_CMD_KEY_2,
    VOICE_CMD_KEY_3,
    VOICE_CMD_KEY_4,
    VOICE_CMD_KEY_5,
    VOICE_CMD_KEY_6,
    VOICE_CMD_KEY_7,
    VOICE_CMD_KEY_8,
    VOICE_CMD_KEY_9,
    VOICE_CMD_KEY_0,
    VOICE_CMD_KEY_MENU,
    VOICE_CMD_KEY_EPG,
    VOICE_CMD_KEY_INFO,
    VOICE_CMD_KEY_EXIT,
    VOICE_CMD_KEY_UP,
    VOICE_CMD_KEY_DOWN,
    VOICE_CMD_KEY_LEFT,
    VOICE_CMD_KEY_RIGHT,
    VOICE_CMD_KEY_OK,
    VOICE_CMD_KEY_PN,//output standard switch
    VOICE_CMD_KEY_PAGEUP,
    VOICE_CMD_KEY_PAGEDOWN,
    VOICE_CMD_KEY_RED,
    VOICE_CMD_KEY_GREEN,
    VOICE_CMD_KEY_YELLOW,
    VOICE_CMD_KEY_BLUE,
    VOICE_CMD_KEY_F1,
    VOICE_CMD_KEY_F2,
    VOICE_CMD_KEY_F3,
    VOICE_CMD_KEY_F4,
    VOICE_CMD_KEY_REC,
    VOICE_CMD_KEY_STOP,
    VOICE_CMD_KEY_PLAY,
    VOICE_CMD_KEY_PAUSE,
    VOICE_CMD_KEY_PRE,
    VOICE_CMD_KEY_NEXT,
    VOICE_CMD_KEY_FB,// fast backward
    VOICE_CMD_KEY_FF,// fast forward
    VOICE_CMD_KEY_AUDIO,
    VOICE_CMD_KEY_FIND,
    VOICE_CMD_KEY_SUBT,
    VOICE_CMD_KEY_TTXT,
    VOICE_CMD_KEY_RECALL,
    VOICE_CMD_KEY_FAV,
    VOICE_CMD_KEY_SAT,
    VOICE_CMD_KEY_TV_RADIO,
    // study remote
    VOICE_CMD_KEY_TV_POWER,
    //cancel
    VOICE_CMD_KEY_CANCEL,

    VOICE_CMD_ERR,// err
    VOICE_CMD_NONE,// not support
}VoiceCMDIDEnum;

typedef struct _VoiceCMDMapClass{
    VoiceCMDIDEnum  cmd_id;
    struct {
        unsigned int id;
        char* desc;
    }c_info;
}VoiceCMDMapClass;

typedef struct _VoiceCMDSeekClass{
#define BEGIN_DIRECT  -1
#define END_DIRECT     1
#define NONE_DIRECT    0
#define BEGIN_POS     -1
#define END_POS        1
#define HALF_POS       2
#define NONE_POS       0
    int direction;//
    int pos;//
    int seconds;// seconds
}VoiceCMDSeekClass;

typedef struct _VoiceCMDParamsClass{
#define STR_BYTES      1000
    VoiceCMDIDEnum cmd;
    union {
        unsigned char str[STR_BYTES+1];//channel name, menu or APPs name,
        int value;// channel num (from 1), ff play or fb play, 0(self increase), 1, 2, 4...,
        VoiceCMDSeekClass seek_params;
    };
}VoiceCMDParamsClass;


#endif //__APP_VOICE_CMD_H__
