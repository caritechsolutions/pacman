/*
 * =====================================================================================
 *
 *       Filename:  autotest_frontend.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  03/08/2023 05:04:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  B.Z.
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */
#include "../include/common_autotest.h"
#include "../include/module_command.h"
#include "module/app_log.h"
#include "av/hal/gxav_hal_vout_vpf.h"
#include "av/hal/gxav_hal_vout.h"
#include "av/hal/gxav_hal_vout_sec.h"
#include "app_volume_value.h"
#include "app_default_params.h"

typedef struct _CVBSIndicatorClass{
    char *indicator_name;
    GxVideoPerformanceType type;
}CVBSIndicatorClass;

static CVBSIndicatorClass thiz_indicator[] = {
    {"sync_level", VPF_SyncLevel},
    {"bar_level", VPF_BarLevel},
    {"chroma_gain", VPF_ChromaGain},
    {"vectoru_gain", VPF_ChromaGainM1Scale1},//cb
    {"vectorv_gain", VPF_ChromaGainM2Scale2},//cr
    {"chroma_sync", VPF_ChromaSyncLevel},
    {"fre_response", VPF_FrequencyResponse},
};

#define INDICATOR_COUNT sizeof(thiz_indicator)/sizeof(CVBSIndicatorClass)

#define MAX_EDGE  35
static int thiz_increase_count[INDICATOR_COUNT] = {0};
static int thiz_decrease_count[INDICATOR_COUNT] = {0};

static char thiz_debug_flag = 0;
static char thiz_cvbs_test_init_flag = 0;
static GxVideoPerformanceType* _get_indicator_type(char *str, unsigned int *index)
{
    int i = 0;
    int count = INDICATOR_COUNT;
    for(i = 0; i < count; i++)
    {
        if(0 == strcmp(thiz_indicator[i].indicator_name, str))
        {
            break;
        }
    }
    if(i != count)
    {// find
        *index = i;
        return &(thiz_indicator[i].type);
    }
    return NULL;
}

// set_indicator sync_level 1 crc32=xxxxxxx\r\n
// set_indicator bar_level -1 crc32=xxxxxxx\r\n
// set_indicator chroma_gain 2 crc32=xxxxxxx\r\n
// set_indicator chroma_sync -2 crc32=xxxxxxx\r\n
// set_indicator fre_response -3 crc32=xxxxxxx\r\n
// ret: -1(parameter error); -2(overstep); -3(no init); 0 (success)
int cvbs_indicator_set(char* str, int steps, int *error_step)
{
    GxVideoPerformanceType *type = NULL;
    int ret = 0;
    int i = 0;
    unsigned int index = 0;

    if(0 == thiz_cvbs_test_init_flag)
        return -3;

    if(NULL == str)
        return -1;
    // get type
    if(NULL == (type = _get_indicator_type(str, &index)))
    {
        return -1;
    }
    //
    if(steps > 0)
    {// increase
        for(i = 0; i < steps; i++)
        {
            if((thiz_increase_count[index] < MAX_EDGE)
                    || (1 == thiz_debug_flag))
            {
                ret = GxVoutHAL_VideoPerformanceFixerIncrease(*type);
                if(ret < 0)
                {// overstep
                    *error_step = (i + 1);
                    return -2;
                }
                thiz_increase_count[index]++;
                thiz_decrease_count[index]--;
            }
            else
            {
                *error_step = (i + 1);
                return -2;
            }

        }
    }
    else
    {//reduce
        steps = steps*(-1);
        for(i = 0; i < steps; i++)
        {
            if((thiz_decrease_count[index] < MAX_EDGE)
                    || (1 == thiz_debug_flag))
            {
                ret = GxVoutHAL_VideoPerformanceFixerDecrease(*type);
                if(ret < 0)
                {// overstep
                    *error_step = (i + 1);
                    return -2;
                }
                thiz_decrease_count[index]++;
                thiz_increase_count[index]--;
            }
            else
            {
                *error_step = (i + 1);
                return -2;
            }
        }
    }
    *error_step = 0;
    return 0;
}

// set_cvbs on crc32=xxxxxxx\r\n
// set_cvbs off crc32=xxxxxxx\r\n
// cvbs work mode control
// mode: "on"-start cvbs test; "off"-exit cvbs test; "debug"-start cvbs test with debug mode
// ret: -1(parameter error); -2(repeat start);-3(no start); 0 (success)
extern void app_cvbs_autotest_init(int is_576);
int cvbs_work_mode_switch(char *mode)
{
    int trans = 0;
    if(NULL == mode)
    {
        return -1;
    }
    if(0 == strncmp("on", mode, 2))
    {// start cvbs test
        if(1 == thiz_cvbs_test_init_flag)
            return -2;
        trans = 0;
        GUI_SetInterface("osd_alpha_global", &trans);
        GUI_StopUpdate(true);
        GUI_SetInterface("osd_disable", NULL);
        // data init
        memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
        memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));
        // init
#if CVBS_TEST_SUPPORT
        app_cvbs_autotest_init(1);
#endif
        thiz_cvbs_test_init_flag = 1;
    }
    else if(0 == strncmp("off", mode, 3))
    {// exit cvbs test
        if(0 == thiz_cvbs_test_init_flag)
            return -3;
        stop_all();
        GxBus_ConfigGetInt(TRANS_LEVEL_KEY, &trans, TRANS_LEVEL);
        GUI_SetInterface("osd_alpha_global", &trans);
        GUI_SetInterface("osd_enable", NULL);
        GUI_StopUpdate(false);

        GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &trans, AUDIO_VOLUME);
        app_system_vol_set(trans);

        // data init
        memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
        memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));
        thiz_debug_flag = 0;
        thiz_cvbs_test_init_flag = 0;
    }
    else if(0 == strncmp("debug", mode, 5))
    {
        if(1 == thiz_cvbs_test_init_flag)
            return -2;
        trans = 0;
        GUI_SetInterface("osd_alpha_global", &trans);
        GUI_StopUpdate(true);
        GUI_SetInterface("osd_disable", NULL);
        // data init
        memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
        memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));
        // init
#if CVBS_TEST_SUPPORT
        app_cvbs_autotest_init(1);
#endif
        thiz_debug_flag = 1;
        thiz_cvbs_test_init_flag = 1;
    }
    else if(0 == strncmp("480ION", mode, 6))
    {// start cvbs test
        if(1 == thiz_cvbs_test_init_flag)
            return -2;
        trans = 0;
        GUI_SetInterface("osd_alpha_global", &trans);
        GUI_StopUpdate(true);
        GUI_SetInterface("osd_disable", NULL);
        // data init
        memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
        memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));
        // init
#if CVBS_TEST_SUPPORT
        app_cvbs_autotest_init(0);
#endif
        thiz_cvbs_test_init_flag = 1;
    }
    else if(0 == strncmp("480Idebug", mode, 9))
    {
        if(1 == thiz_cvbs_test_init_flag)
            return -2;
        trans = 0;
        GUI_SetInterface("osd_alpha_global", &trans);
        GUI_StopUpdate(true);
        GUI_SetInterface("osd_disable", NULL);
        // data init
        memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
        memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));
        // init
#if CVBS_TEST_SUPPORT
        app_cvbs_autotest_init(0);
#endif
        thiz_debug_flag = 1;
        thiz_cvbs_test_init_flag = 1;
    }
    else
    {// parameter error
        return -1;
    }

    return 0;
}

// set_volume maximum crc32=xxxxxxx\r\n
// set_volume minimum crc32=xxxxxxx\r\n
// set_volume mute crc32=xxxxxxx\r\n
// set_volume unmute crc32=xxxxxxx\r\n
// ret: -1(parameter error); -3(no start); 0 (success)
int cvbs_set_volume(char *cmd)
{
    if(0 == thiz_cvbs_test_init_flag)
        return -3;

    if(NULL == cmd)
    {
        return -1;
    }
    if(0 == strcmp(cmd, "maximum"))
    {
        int volume = AUDIO_MAP_25(MAX_DISPLAY_VOL);
        app_system_vol_set(volume);
    }
    else if(0 == strcmp(cmd, "minimum"))
    {
        app_system_vol_set(0);
    }
    else if(0 == strcmp(cmd, "mute"))
    {
         GxPlayer_SetAudioMute(1);
    }
    else if(0 == strcmp(cmd, "unmute"))
    {
         GxPlayer_SetAudioMute(0);
    }
    else
    {
        return -1;
    }
    return 0;
}

//
//Bar Level
//Sync Level Y1
//Sync Level Y2
//Sync Level Y3
//Sync Level Y4
//Chroma Gain M1
//Chroma Gain M2
//Chroma Sync A
//Chroma sync B
//Fre Response V0
//Fre Response V1
//Fre Response V2
//Fre Response V3
//Fre Response V4
// get_all_data crc32=xxxxxxx\r\n
// ret: -3 (no start); 0 (success)
int cvbs_get_all_data(char *data, unsigned int size)
{
    int ret = 0;
    if(0 == thiz_cvbs_test_init_flag)
        return -3;

    GxVideoPerformanceFixer fixer_data;
    memset(&fixer_data, 0, sizeof(GxVideoPerformanceFixer));

    ret = GxVoutHAL_VideoPerformanceFixerGet(&fixer_data);
    if(ret)
        return -1;

    memset(data, 0, size);
    // gdac0_ips~gdac3_ips，有可能是负值
    sprintf(data, "(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)",
                   fixer_data.m0_scale0,fixer_data.gdac0_ips, fixer_data.gdac1_ips, fixer_data.gdac2_ips, fixer_data.gdac3_ips,
                   fixer_data.m1_scale1, fixer_data.m2_scale2, fixer_data.burst_amp_a, fixer_data.burst_amp_b, fixer_data.tv_fir_val0,
                   fixer_data.tv_fir_val1, fixer_data.tv_fir_val2, fixer_data.tv_fir_val3, fixer_data.tv_fir_val4);
    return 0;
}

// set_all_data data crc32=xxxxxxx\r\n
// data: 160,29,29,29,28,150,212,106,150,7692,549491732,638093639,577014138,590901355
// ret: -1 (parameter error); -3 (no start); 0 (success)
int cvbs_set_all_data(char *data)
{
    GxVideoPerformanceFixer fixer_data;
    int ret = 0;
    unsigned int buf[20] = {0};

    if(0 == thiz_cvbs_test_init_flag)
        return -3;

    if(NULL == data)
        return -1;
    memset(&fixer_data, 0, sizeof(GxVideoPerformanceFixer));

    sscanf(data, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
             &buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5], &buf[6], &buf[7], &buf[8], &buf[9], &buf[10], &buf[11], &buf[12], &buf[13]);

    if((buf[0] > 0x3FF)// bar level
            || (buf[5] > 0x3FF) || (buf[6] > 0x3FF)// chroma gain
            || (buf[7] > 0xFF) || (buf[8] > 0xFF)// chroma sync
      )
        return -1;
    GxVoutHAL_VideoPerformanceFixerGet(&fixer_data);
    fixer_data.m0_scale0 = buf[0];
    fixer_data.gdac0_ips = buf[1];
    fixer_data.gdac1_ips = buf[2];
    fixer_data.gdac2_ips = buf[3];
    fixer_data.gdac3_ips = buf[4];
    fixer_data.m1_scale1 = buf[5];
    fixer_data.m2_scale2 = buf[6];
    fixer_data.burst_amp_a = buf[7];
    fixer_data.burst_amp_b = buf[8];
    fixer_data.tv_fir_val0 = buf[9];
    fixer_data.tv_fir_val1 = buf[10];
    fixer_data.tv_fir_val2 = buf[11];
    fixer_data.tv_fir_val3 = buf[12];
    fixer_data.tv_fir_val4 = buf[13];
    ret = GxVoutHAL_VideoPerformanceFixerSet(&fixer_data);
    if(ret)
    {
        return -2;
    }

    // init data
    memset(thiz_increase_count, 0, sizeof(thiz_increase_count));
    memset(thiz_decrease_count, 0, sizeof(thiz_decrease_count));

    return 0;
}
