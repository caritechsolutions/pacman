#ifndef __APP_NETAUDIO_H__
#define __APP_NETAUDIO_H__

extern int app_netaudio_netaudio_play_stop(void);
extern int app_netaudio_check_flag(void);
extern int app_netaudio_auto_msg_proc(GxMessage *msg);
#if NET_AUDIO_SUPPORT
extern int app_netaudio_tsbuff_time_init(void);
extern int  app_netaudio_nomal_replay(void);
extern void app_netaudio_set_flag(int flag);
extern char * app_netaudio_get_name(unsigned int sel);
extern void app_netaudio_auto_get_data_start(const char* url);
extern int  app_netaudio_online_data_play(int sel);
extern int app_get_netaudio_tatal_num(void);
extern int app_netaudio_get_download_data_state(void);
extern int app_get_netaudio_current_play_index(void);
extern int app_get_netaudio_tsbuff_index(void);
extern void app_set_netaudio_tsbuff_time(int tsbuffTime);
extern int app_get_netaudio_max_tsbuff_time(void);
extern int app_get_netaudio_tsbuff_time(int tsbuffTime);
extern int app_netaudio_tsbuff_replay(void);
extern void app_netaudio_list_update(char *path);
#endif
#endif
