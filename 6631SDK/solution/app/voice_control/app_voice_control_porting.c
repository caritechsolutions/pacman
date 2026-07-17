#include "app_utility.h"
#include "module/app_time.h"
#include "module/app_network_service.h"

#if VOICE_CONTROL_SUPPORT
extern int app_voice_control_msg_send(unsigned int cmd, char* des, unsigned int des_len);

time_t vc_porting_localtime_get(void)
{
    time_t time = 0;
    time = g_AppTime.utc_get(&g_AppTime);
    time = get_display_time_by_timezone(time);
    return time;
}

int vc_porting_second_2_time_str(time_t seconds, char *outstr, unsigned int str_buf)
{
    struct tm ptm = {0};
    struct tm *ret = NULL;

    if((str_buf < 20) || (NULL == outstr))
        return -1;

    ret = localtime_r(&seconds, &ptm);

    if(NULL != ret)
    {
        sprintf(outstr,"%d-%02d-%02d, %02d:%02d:%02d", ptm.tm_year+1900,ptm.tm_mon+1,ptm.tm_mday,ptm.tm_hour,ptm.tm_min,ptm.tm_sec);
        return 0;
    }
    return -1;
}


void vc_porting_clean_tips(void)
{
    app_voice_control_msg_send(3/*VOICE_CMD_TO_IDLE*/, NULL, 0);
}

int vc_cmd_sent_to_stb(unsigned int cmd, char* des, unsigned int des_len)
{
    return app_voice_control_msg_send(cmd, des, des_len);
}

// from dongle, opus data
int vc_upload_voice_data(char* data, unsigned int des_len)
{
extern int app_vc_cloud_voice_data_store(char* data, unsigned int len);
    return app_vc_cloud_voice_data_store(data, des_len);
}

// new voice data comeing
int vc_upload_voice_data_begin(void)
{
extern int app_vc_cloud_data_begin(void);
    return app_vc_cloud_data_begin();
}

// current voice data finish
int vc_upload_voice_data_end(void)
{
extern int app_vc_cloud_data_term(void);
    return app_vc_cloud_data_term();
}

int vc_check_network_is_connect(void)
{
#if NETWORK_SUPPORT
    if(IF_STATE_CONNECTED == app_network_get_cur_dev_and_state(NULL))
        return 1;
    else
        return 0;
#endif
    return 0;
}

int vc_check_cloud_used(void)
{
    int is_used = 0;
#if defined(VC_DEVICE_CLOUD_SUPPORT)
    is_used = 1;
#endif
    return is_used;
}
#endif
