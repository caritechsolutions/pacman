#ifndef __PMP_PSI_H__
#define __PMP_PSI_H__

#include "module/app_ttx_subt.h"
#include "app_audio.h"

typedef struct
{
    unsigned short video_pid;
    VideoCodecType video_type;
}VideoInfo;

typedef struct
{
    ttx_subt *data;
    int num;
}SubtTTxInfo;

typedef struct
{
    unsigned short ts_id;
    unsigned short service_id;
    PrivateProgInfo private_info;
    VideoInfo video_info;
    AudioLang_t audio_info;
    SubtTTxInfo subt_info;
    SubtTTxInfo ttx_info;
    char valid_flag;
}PmpPsi;

#endif
