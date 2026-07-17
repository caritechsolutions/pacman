#ifndef __APP_NETAPPS_SERVICE_H__
#define __APP_NETAPPS_SERVICE_H__

typedef void(* app_netapps_service_func)(void);

void app_netapps_service_init(void);

void app_netapps_service_destroy(void);

int app_netapps_service_push(app_netapps_service_func func);

#endif
