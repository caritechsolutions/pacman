#ifndef __APP_NETAPPS_INIT_H__
#define __APP_NETAPPS_INIT_H__

void app_netapps_player_init(void);

void app_netapps_gui_init(void);

void app_netapps_event_init(void);

void app_netapp_event_register(app_netapp_event_class* cls);

int app_netapps_top_msg_proc(GxMessage *msg);

void app_netapps_top_msg_dev_out_proc(void);

#endif
