#ifndef __APP_CHANNEL_INFO_H__
#define __APP_CHANNEL_INFO_H__

typedef enum
{
    LIST_MODE_PROG,
    LIST_MODE_FIND,
#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
    LIST_MODE_SAT,
#endif
    LIST_MODE_FAV,
    LIST_MODE_HD,
    LIST_MODE_TOTAL
}ListMode;

status_t app_channel_info_check_dialog(void);
int app_channel_info_compare_dialog(const char *wnd);
void app_channel_info_create_dialog(void);
void app_channel_info_end_dialog(void);

status_t app_channel_info_signal_check_dialog(void);
int app_channel_info_signal_compare_dialog(const char *wnd);
void app_channel_info_signal_end_dialog(void);

void app_channel_info_update_dialog(void);
void app_channel_info_update_under_pop_dlg(void);
void app_channel_info_clear_epg_str(void);

status_t app_channel_list_check_dialog(void);
int app_channel_list_compare_dialog(const char *wnd);
status_t app_channel_epg_check_dialog(void);
int app_channel_epg_compare_dialog(const char *wnd);
extern int app_get_channel_list_mode(void);

#endif
