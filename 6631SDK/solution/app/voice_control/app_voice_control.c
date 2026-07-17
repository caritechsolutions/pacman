#include <ctype.h>
#include "gxcore.h"
#include "app_utility.h"
#include "module/app_log.h"
#if VOICE_CONTROL_SUPPORT
#include "include/app_voice_control_api.h"
#include "include/vc_cmd.h"

//#define VC_CLOUD_DEBUG
#ifdef VC_CLOUD_DEBUG
// debug
#include "module/app_time.h"
static void _time_format(char * file_name)
{
    time_t time;
    struct tm ptm;
    char buf[16];

    time = g_AppTime.utc_get(&g_AppTime);
    time = get_display_time_by_timezone(time);
    localtime_r(&time, &ptm);

    memset(buf, 0, 16);
    sprintf(buf, "_%d%02d%02d%02d%02d%02d", ptm.tm_year+1900, ptm.tm_mon+1,
            ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);

    strcat(file_name, buf);
}

static void _write_voice_to_usb(unsigned char *wav_data, unsigned int data_size)
{
    char *file_path = NULL;
    int fd = 0;
    int ret = 0;

    file_path = GxCore_Mallocz(2048);
    if(NULL != file_path)
    {
        strcat(file_path, "/mnt/usb01/GX8301A_Voice");
        _time_format(file_path);
        strcat(file_path, ".wav");
        fd = GxCore_Open(file_path, "wb");
        if(fd > 0)
        {
            ret = GxCore_Write(fd, wav_data, data_size, 1);
            if(ret != 1)
                app_log_error("\nWrite error...\n");
            else
                app_log_info("\nWrite success...\n");
        }
    }
    if(fd > 0)
        GxCore_Close(fd);
    if(file_path)
        GxCore_Free(file_path);
}
// debug code end
#endif

typedef struct _CMDInClass{
	char *intension;
	char *mode;
	char *para;
}CMDInClass;

typedef struct _Mode2CMDClass{
    char* mode;
    char* params;
    VoiceCMDIDEnum cmd;
}Mode2CMDClass;

typedef struct _NLPProcessClass{
    char* NLPText;
    Mode2CMDClass* mode2cmd;
    unsigned int   mode2cmd_count;
    int (*process)(int index, CMDInClass params, VoiceCMDParamsClass *cmd_params);
}NLPProcessClass;



static int _play_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _control_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _audio_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _open_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _pvr_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _fav_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _basic_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _power_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);
static int _applets_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params);

// 字符串全小写
static NLPProcessClass thiz_NLP_process[] = {
    // play process
    {
        .NLPText = "play_process",
        .mode2cmd_count = 5,
        .mode2cmd = (Mode2CMDClass[]){
            {"name",            "none",         VOICE_CMD_PLAY_BY_NMAE},
            {"number",          "none",         VOICE_CMD_PLAY_BY_NUM},
            {"next",            "none",         VOICE_CMD_NEXT},
            {"previous",        "none",         VOICE_CMD_PRE},
            {"history",         "none",         VOICE_CMD_RECALL_PROG},
        },
        .process = _play_process,
    },
    // audio output process
    {
        .NLPText = "audio_output_process",
        .mode2cmd_count = 5,
        .mode2cmd = (Mode2CMDClass[]){
            {"increase",        "none",         VOICE_CMD_VOLUME_UP},
            {"decrease",        "none",         VOICE_CMD_VOLUME_DOWN},
            {"mute",            "none",         VOICE_CMD_MUTE},
            {"unmute",          "none",         VOICE_CMD_UNMUTE},
            {"value",           "none",         VOICE_CMD_VOLUME},
        },
        .process = _audio_process,
    },
    // play control process
    {
        .NLPText = "play_control_process",
        .mode2cmd_count = 8,
        .mode2cmd = (Mode2CMDClass[]){
            {"pause",           "none",         VOICE_CMD_PAUSE},
            {"play",            "none",         VOICE_CMD_PLAY},
            {"stop",            "none",         VOICE_CMD_STOP_PLAY},
            {"fast_forward",    "none",         VOICE_CMD_FAST_FORWARD},
            {"fast_backward",   "none",         VOICE_CMD_FAST_BACKWARD},
            {"forward",         "none",         VOICE_CMD_SEEK_FORWARD},
            {"backward",        "none",         VOICE_CMD_SEEK_BACKWARD},
            {"seek_to",         "none",         VOICE_CMD_SEEK_TO},
        },
        .process = _control_process,
    },
    // open process
    {
        .NLPText = "open_process",
        .mode2cmd_count = 1,
        .mode2cmd = (Mode2CMDClass[]){
            {"open",            "none",         VOICE_CMD_OPEN},
        },
        .process = _open_process,
    },
    // PVR Recording
    {
        .NLPText = "pvr_recording_process",
        .mode2cmd_count = 2,
        .mode2cmd = (Mode2CMDClass[]){
            {"start",           "none",         VOICE_CMD_REC},
            {"stop",            "none",         VOICE_CMD_END_RECORD},
        },
        .process = _pvr_process,
    },
    // favorite control
    {
        .NLPText = "favorite_edit",
        .mode2cmd_count = 2,
        .mode2cmd = (Mode2CMDClass[]){
            {"add",             "none",         VOICE_CMD_MARK_PROG},
            {"delete",          "none",         VOICE_CMD_UNMARK_PROG},
        },
        .process = _fav_process,
    },
    // basic control
    {
        .NLPText = "basic_control_process",
        .mode2cmd_count = 7,
        .mode2cmd = (Mode2CMDClass[]){
            {"confirm",         "none",         VOICE_CMD_KEY_OK},
            {"cancel",          "none",         VOICE_CMD_KEY_CANCEL},
            {"return",          "none",         VOICE_CMD_KEY_EXIT},
            {"left",            "none",         VOICE_CMD_KEY_LEFT},
            {"right",           "none",         VOICE_CMD_KEY_RIGHT},
            {"up",              "none",         VOICE_CMD_KEY_UP},
            {"down",            "none",         VOICE_CMD_KEY_DOWN},
        },
        .process = _basic_process,
    },
    // power control
    {
        .NLPText = "power_control_process",
        .mode2cmd_count = 3,
        .mode2cmd = (Mode2CMDClass[]){
            {"standby",         "stb",          VOICE_CMD_STANDBY},
            {"standby",         "tv",           VOICE_CMD_STANDBY_TV},
            {"wakeup",          "tv",           VOICE_CMD_WAKEUP_TV},
        },
        .process = _power_process,
    },
    // applets process
    {
        .NLPText = "applets_process",
        .mode2cmd_count = 9,
        .mode2cmd = (Mode2CMDClass[]){
            {"open",            "none",         VOICE_CMD_OPEN_APPLETS},
            {"exit",            "none",         VOICE_CMD_NETAPPS_EXIT},
            {"search",            "none",         VOICE_CMD_NETAPPS_SEARCH},
            {"category",            "none",         VOICE_CMD_NETAPPS_CATEGORY},
            {"group",            "none",         VOICE_CMD_NETAPPS_GROUP},
            {"page",            "none",         VOICE_CMD_NETAPPS_PAGE},
            {"play",            "none",         VOICE_CMD_NETAPPS_PLAY},
            {"setting",            "none",         VOICE_CMD_NETAPPS_SETTING},
            {"user",            "none",         VOICE_CMD_NETAPPS_USER},
            {"private",            "none",         VOICE_CMD_NETAPPS_PRIVATE},
        },
        .process = _applets_process,
    },
};


// 将字符串转换为小写
static void ___to_lowercase(char *str, unsigned int len)
{
    int i ;
    for (i = 0; i < len; i++)
    {
        str[i] = tolower((unsigned char)str[i]);
    }
}

static int ___insensitive_strstr(const char *str1, const char *str2)
{// if str1 include str2, return 0
    char *result = NULL;
    result = strstr(str1, str2);
    return (result ? 0 : -1);
}

static int _play_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int ret = 0;
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    unsigned int len = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
/*
            if((0 != memcmp(p[i].para, "none", 4))
                    && (0 == ___insensitive_strstr(p[i].para, params.para)))
            {// 带固定参数的指令
                cmd_params->cmd = p[i].cmd;
                return 0;
            }
*/
            cmd_params->cmd = p[i].cmd;
            if(0 == memcmp("name", params.mode, 4))
            {
                len = strlen(params.para) > STR_BYTES?STR_BYTES:strlen(params.para);
                memset(cmd_params->str, 0, sizeof(cmd_params->str));
                memcpy(cmd_params->str,params.para,len);
            }
            else if(0 == memcmp("number", params.mode, 6))
            {
                cmd_params->value = atoi(params.para);
            }
            break;
        }
    }
    return ret;
}

static int _control_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int ret = 0;
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            if((0 == memcmp("fast_forward", params.mode, strlen(params.mode)))
                || (0 == memcmp("fast_backward", params.mode, strlen(params.mode))))
            {
                cmd_params->value = atoi(params.para);
            }
            else if(0 == memcmp("seek_to", params.mode, strlen(params.mode)))
            {
                // TODO:...
                cmd_params->seek_params.direction = END_DIRECT;
                cmd_params->seek_params.pos = NONE_POS;
                cmd_params->seek_params.direction = 10;
            }
            break;
        }
    }
    return ret;

}

static int _audio_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int ret = 0;
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    unsigned int len = 0;
    //char buf[30] = {0};
    char *result = NULL;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            len = strlen(params.mode);
            if((0 == memcmp("increase", params.mode, len))
                    || (0 == memcmp("decrease", params.mode, len))
                    )
            {
                result = strchr(params.para, '%');
                if(result)
                {// pecent
                    sscanf(params.para, "%d%%", &cmd_params->value);
                    cmd_params->value += 0x10000000;
                }
                else
                {
                    sscanf(params.para, "%d", &cmd_params->value);
                }
            }
            else if(0 == memcmp("value", params.mode, len))
            {
                sscanf(params.para, "%d", &cmd_params->value);
            }
            break;
        }
    }
    return ret;
}

static int _open_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    unsigned int len = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            len = strlen(params.para) > STR_BYTES?STR_BYTES:strlen(params.para);
            memset(cmd_params->str, 0, sizeof(cmd_params->str));
            memcpy(cmd_params->str,params.para,len);
            return 0;
        }
    }
    return -1;
}

static int _pvr_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            return 0;
        }
    }
    return -1;

}

static int _fav_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    app_log_error("\n%s, %d\n", __func__, __LINE__);
    return -1;
}

static int _basic_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            return 0;
        }
    }
    return -1;
}

static int _power_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    app_log_error("\n%s, %d\n", __func__, __LINE__);
    return -1;
}

static int _applets_process(int index, CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int count = thiz_NLP_process[index].mode2cmd_count;
    int i = 0;
    unsigned int len = 0;
    Mode2CMDClass *p = thiz_NLP_process[index].mode2cmd;
    app_log_error("\n%s, %d\n", __func__, __LINE__);
    for(i = 0; i < count; i++)
    {
        if(0 == ___insensitive_strstr(p[i].mode, params.mode))
        {
            cmd_params->cmd = p[i].cmd;
            len = strlen(params.para) > STR_BYTES?STR_BYTES:strlen(params.para);
            memset(cmd_params->str, 0, sizeof(cmd_params->str));
            memcpy(cmd_params->str,params.para,len);
            return 0;
        }
    }
    return -1;
}

static int __data_convert(CMDInClass params, VoiceCMDParamsClass* cmd_params)
{
    int ret = 0;
    unsigned int cmd_count = (sizeof(thiz_NLP_process)/sizeof(NLPProcessClass));
    int i = 0;

    ___to_lowercase(params.intension, strlen(params.intension));
    ___to_lowercase(params.mode, strlen(params.mode));
    ___to_lowercase(params.para, strlen(params.para));
    for(i = 0; i < cmd_count; i++)
    {
        if(0 == ___insensitive_strstr(thiz_NLP_process[i].NLPText, params.intension))
        {
            if(thiz_NLP_process[i].process)
            {
                ret = thiz_NLP_process[i].process(i, params, cmd_params);
                return ret;
            }
        }
    }
    return -1;
}

int app_vc_voicecmd_process(CMDInClass cmdin);
extern int vc_cmd_sent_to_stb(unsigned int cmd, char* des, unsigned int des_len);
extern int vc_get_data_from_cloud(const char *data_upload, int data_len, char* intension, char *mode, char *para);
extern int vc_check_network_is_connect(void);
int app_vc_voicecmd_process(CMDInClass cmdin)
{
    VoiceCMDParamsClass cmd_params = {0};
    int ret = 0;

    memset(&cmd_params, 0, sizeof(cmd_params));
    ret = __data_convert(cmdin, &cmd_params);
    if(0 == ret)
    {
        vc_cmd_sent_to_stb(cmd_params.cmd, (char*)&cmd_params, sizeof(cmd_params));
    }
    else
    {// not support
        vc_cmd_sent_to_stb(VOICE_CMD_ERR, "Sorry, I don't understand!", 27);
    }
    return ret;
}


//#define VC_CMD_TEST
#ifdef VC_CMD_TEST
// for test
static int thiz_cmd_test[] = {
    VOICE_CMD_PLAY_BY_NMAE, // 0
    VOICE_CMD_PLAY_BY_NUM,  // 1
    VOICE_CMD_NEXT,         // 3
    VOICE_CMD_PRE,          // 4
    VOICE_CMD_RECALL_PROG,  // 5
    VOICE_CMD_VOLUME_UP,    // 6
    VOICE_CMD_VOLUME_DOWN,  // 7
    VOICE_CMD_MUTE,         // 8
    VOICE_CMD_UNMUTE,       // 9
    VOICE_CMD_VOLUME,       // 10
    VOICE_CMD_PAUSE,        // 11
    VOICE_CMD_PLAY,         // 12
    VOICE_CMD_STOP_PLAY,    // 13
    VOICE_CMD_FAST_FORWARD, // 14
    VOICE_CMD_FAST_BACKWARD,// 15
    VOICE_CMD_SEEK_FORWARD, // 16
    VOICE_CMD_SEEK_BACKWARD,// 17
    VOICE_CMD_SEEK_TO,      // 18
    VOICE_CMD_OPEN,         // 19
    VOICE_CMD_REC,          // 20
    VOICE_CMD_END_RECORD,   // 21
    VOICE_CMD_MARK_PROG,    // 22
    VOICE_CMD_UNMARK_PROG,  // 23
    VOICE_CMD_KEY_OK,       // 24
    VOICE_CMD_KEY_EXIT,     // 25
    VOICE_CMD_KEY_LEFT,     // 26
    VOICE_CMD_KEY_RIGHT,    // 27
    VOICE_CMD_KEY_UP,       // 28
    VOICE_CMD_KEY_DOWN,     // 29
    VOICE_CMD_STANDBY,      // 30
    VOICE_CMD_STANDBY_TV,   // 31
    VOICE_CMD_WAKEUP_TV,    // 32
};

int app_vc_cloud_cmd_test(int index)
{
    VoiceCMDParamsClass cmd_params = {0};
    unsigned int count = sizeof(thiz_cmd_test)/4;
    if((index < 0) || (index >= count))
    {
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }
    cmd_params.cmd = VOICE_CMD_NONE;

    switch(thiz_cmd_test[index])
    {
        case VOICE_CMD_PLAY_BY_NMAE:
            cmd_params.cmd = VOICE_CMD_PLAY_BY_NMAE;
            memcpy(cmd_params.str,"HD Phx Chinese Channel",22);
            app_log_info("\nPLAY_BY_NMAE, name = %s\n", cmd_params.str);
            break;
        case VOICE_CMD_PLAY_BY_NUM:
            cmd_params.cmd = VOICE_CMD_PLAY_BY_NUM;
            cmd_params.value = 2;
            app_log_info("\nPLAY_BY_NUM, number = %d\n", cmd_params.value);
            break;
        case VOICE_CMD_NEXT:
            cmd_params.cmd = VOICE_CMD_NEXT;
            app_log_info("\nPLAY_BY_NEXT\n");
            break;
        case VOICE_CMD_PRE:
            cmd_params.cmd = VOICE_CMD_PRE;
            app_log_info("\nPLAY_BY_PREVIOUS\n");
            break;
        case VOICE_CMD_RECALL_PROG:
            cmd_params.cmd = VOICE_CMD_RECALL_PROG;
            app_log_info("\nPLAY_BY_RECALL\n");
            break;
        case VOICE_CMD_VOLUME_UP:
            cmd_params.cmd = VOICE_CMD_VOLUME_UP;
            cmd_params.value = 0x10000008;// increase by 8%
            app_log_info("\nIncrease volume: %d%%\n", (cmd_params.value - 0x10000000));
            break;
        case VOICE_CMD_VOLUME_DOWN:
            cmd_params.cmd = VOICE_CMD_VOLUME_DOWN;
            cmd_params.value = 8;// decrease by 8
            app_log_info("\nDecrease volume: %d\n", cmd_params.value);
            break;
        case VOICE_CMD_MUTE:
            cmd_params.cmd = VOICE_CMD_MUTE;
            app_log_info("\nMute\n");
            break;
        case VOICE_CMD_UNMUTE:
            cmd_params.cmd = VOICE_CMD_UNMUTE;
            app_log_info("\nUnmute\n");
            break;
        case VOICE_CMD_VOLUME:
            cmd_params.cmd = VOICE_CMD_VOLUME;
            cmd_params.value = 12;// decrease by 8
            app_log_info("\nChange volume to %d\n", cmd_params.value);
            break;
        case VOICE_CMD_PAUSE:
            cmd_params.cmd = VOICE_CMD_PAUSE;
            app_log_info("\nPause\n");
            break;
        case VOICE_CMD_PLAY:
            cmd_params.cmd = VOICE_CMD_PLAY;
            app_log_info("\nPlay\n");
            break;
        case VOICE_CMD_STOP_PLAY:
            cmd_params.cmd = VOICE_CMD_STOP_PLAY;
            app_log_info("\nStop\n");
            break;
        case VOICE_CMD_FAST_FORWARD:
            cmd_params.cmd = VOICE_CMD_FAST_FORWARD;
            cmd_params.value = 4;// decrease by 8
            app_log_info("\nFF Play: %d\n", cmd_params.value);
            break;
        case VOICE_CMD_FAST_BACKWARD:
            cmd_params.cmd = VOICE_CMD_FAST_BACKWARD;
            cmd_params.value = 2;// decrease by 8
            app_log_info("\nFB Play: %d\n", cmd_params.value);
            break;
        case VOICE_CMD_SEEK_FORWARD:
            cmd_params.cmd = VOICE_CMD_SEEK_FORWARD;
            cmd_params.seek_params.direction = END_DIRECT;
            cmd_params.seek_params.pos = NONE_POS;
            cmd_params.seek_params.seconds = 10;
            app_log_info("\nSeek forward: %d\n", cmd_params.seek_params.seconds);
            break;
        case VOICE_CMD_SEEK_BACKWARD:
            cmd_params.cmd = VOICE_CMD_SEEK_BACKWARD;
            cmd_params.seek_params.direction = BEGIN_DIRECT;
            cmd_params.seek_params.pos = NONE_POS;
            cmd_params.seek_params.seconds = 20;
            app_log_info("\nSeek backward: %d\n", cmd_params.seek_params.seconds);
            break;
        case VOICE_CMD_SEEK_TO:
            cmd_params.cmd = VOICE_CMD_SEEK_BACKWARD;
            cmd_params.seek_params.direction = NONE_DIRECT;
            cmd_params.seek_params.pos = NONE_POS;
            cmd_params.seek_params.seconds = 90;
            app_log_info("\nSeek to: %d\n", cmd_params.seek_params.seconds);
            break;
        case VOICE_CMD_OPEN:
            cmd_params.cmd = VOICE_CMD_OPEN;
            memcpy(cmd_params.str,"epg",3);
            app_log_info("\nopen: %s\n", cmd_params.str);
            break;
        case VOICE_CMD_REC:
            cmd_params.cmd = VOICE_CMD_REC;
            app_log_info("\nPVR Record\n");
            break;
        case VOICE_CMD_END_RECORD:
            cmd_params.cmd = VOICE_CMD_END_RECORD;
            app_log_info("\nPVR End Record\n");
            break;
        case VOICE_CMD_MARK_PROG:
            cmd_params.cmd = VOICE_CMD_MARK_PROG;
            app_log_info("\nAdd current channel to favorite!\n");
            break;
        case VOICE_CMD_UNMARK_PROG:
            cmd_params.cmd = VOICE_CMD_MARK_PROG;
            app_log_info("\nRemove current channel from favorite list!\n");
            break;
        case VOICE_CMD_KEY_OK:
            cmd_params.cmd = VOICE_CMD_KEY_OK;
            app_log_info("\nVOICE_CMD_KEY_OK\n");
            break;
        case VOICE_CMD_KEY_EXIT:
            cmd_params.cmd = VOICE_CMD_KEY_EXIT;
            app_log_info("\nVOICE_CMD_KEY_EXIT\n");
            break;
        case VOICE_CMD_KEY_LEFT:
            cmd_params.cmd = VOICE_CMD_KEY_LEFT;
            app_log_info("\nVOICE_CMD_KEY_LEFT\n");
            break;
        case VOICE_CMD_KEY_RIGHT:
            cmd_params.cmd = VOICE_CMD_KEY_RIGHT;
            app_log_info("\nVOICE_CMD_KEY_RIGHT\n");
            break;
        case VOICE_CMD_KEY_UP:
            cmd_params.cmd = VOICE_CMD_KEY_UP;
            app_log_info("\nVOICE_CMD_KEY_UP\n");
            break;
        case VOICE_CMD_KEY_DOWN:
            cmd_params.cmd = VOICE_CMD_KEY_DOWN;
            app_log_info("\nVOICE_CMD_KEY_DOWN\n");
            break;
        case VOICE_CMD_STANDBY:
            //cmd_params.cmd = VOICE_CMD_STANDBY;
            //printf("\nVOICE_CMD_STANDBY\n");
            break;
        case VOICE_CMD_WAKEUP:
            //cmd_params.cmd = VOICE_CMD_WAKEUP;
            //printf("\nVOICE_CMD_WAKEUP\n");
            break;
        case VOICE_CMD_STANDBY_TV:
            break;
        case VOICE_CMD_WAKEUP_TV:
            break;
        default:
            app_log_error("\nERROR, %s, %d, index = %d\n", __func__, __LINE__, index);
            break;
    }
    if(cmd_params.cmd != VOICE_CMD_NONE)
        vc_cmd_sent_to_stb(cmd_params.cmd, (char*)&cmd_params, sizeof(cmd_params));
    return 0;
}

void app_vc_cloud_play_by_name_test(char* name)
{
    VoiceCMDParamsClass cmd_params = {0};
    cmd_params.cmd = VOICE_CMD_PLAY_BY_NMAE;
    memcpy(cmd_params.str,name,strlen(name));
    vc_cmd_sent_to_stb(cmd_params.cmd, (char*)&cmd_params, sizeof(cmd_params));
}
#endif
#endif
