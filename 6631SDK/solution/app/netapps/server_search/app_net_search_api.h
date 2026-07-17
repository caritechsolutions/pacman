#ifndef __APP_NET_SEARCH_API_H__
#define __APP_NET_SEARCH_API_H__

#if NET_SEARCH_SUPPORT

#if DVB_SERVER_SEARCH_SUPPORT
extern status_t app_add_net_server_from_json(char *json_path);
#endif
#if DVB_FILE_SEARCH_SUPPORT
extern status_t app_add_ts_path_from_usb(char *ts_path);
#endif

#endif

#endif
