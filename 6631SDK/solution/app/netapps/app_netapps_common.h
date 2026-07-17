#ifndef __APP_NETAPPS_COMMON_H__
#define __APP_NETAPPS_COMMON_H__

#include "app_windows.h"

#define NETAPP_LOG_MIN 0
#define NETAPP_LOG_ERROR 1
#define NETAPP_LOG_INFO 2
#define NETAPP_LOG_DEBUG 3
#define NETAPP_LOG_FLOW 4
#define NETAPP_LOG_LEVEL NETAPP_LOG_ERROR

#if (NETAPP_LOG_LEVEL <= NETAPP_LOG_MIN)
#define app_netapps_log_min(...)   (void)0
#else
#define app_netapps_log_min(...)   printf(__VA_ARGS__)
#endif

#if (NETAPP_LOG_LEVEL < NETAPP_LOG_ERROR)
#define app_netapps_log_error(...)   (void)0
#else
#define app_netapps_log_error(...)   printf(__VA_ARGS__)
#endif

#if (NETAPP_LOG_LEVEL < NETAPP_LOG_INFO)
#define app_netapps_log_info(...)   (void)0
#else
#define app_netapps_log_info(...)   printf(__VA_ARGS__)
#endif

#if (NETAPP_LOG_LEVEL < NETAPP_LOG_DEBUG)
#define app_netapps_log_debug(...)   (void)0
#else
#define app_netapps_log_debug(...)   printf(__VA_ARGS__)
#endif

#if (NETAPP_LOG_LEVEL < NETAPP_LOG_FLOW)
#define app_netapps_log_flow(...)   (void)0
#else
#define app_netapps_log_flow(...)   printf(__VA_ARGS__)
#endif

#define app_netapps_malloc(...)   malloc(__VA_ARGS__)
#define app_netapps_calloc(...)   calloc(__VA_ARGS__)
#define app_netapps_realloc(...)   realloc(__VA_ARGS__)
#define app_netapps_free(...)   free(__VA_ARGS__)
#define app_netapps_strdup(...)   strdup(__VA_ARGS__)
#define app_netapps_sfree(x)   if(x){free(x);x=NULL;}

typedef enum
{
	NETAPPS_GDI_TEXT = 0,
	NETAPPS_GDI_PIC,
	NETAPPS_GDI_QR,
	NETAPPS_GDI_MAX
}netapps_gdi_type;

typedef struct
{
	unsigned long total;
	unsigned long free;
}netapps_mem_info;

void app_netapps_app_init(void);

void app_netapps_app_destroy(void);

int app_netapps_check_compose_url(void);

char *app_netapps_get_system_language(void);

void app_netapps_aspect_init(void);

char* app_get_netapps_aspect_string(void);

void app_netapps_aspect_hotkey(void);

void app_netapps_aspect_exit(void);

int app_netapps_bandwidth_get_num(void);

int app_netapps_bandwidth_create_dialog(void);

unsigned int app_netapps_get_storage_size(void);

unsigned int app_netapps_get_memory_size(void);

int app_netapps_get_memory_info(netapps_mem_info *info);

void app_netapps_get_ui_size(int *width, int *height);

int app_netapps_get_time_offsets(void);

unsigned int app_netapps_disk_malloc(void);

void app_netapps_disk_free(unsigned int handle);

int app_netapps_disk_get_num(unsigned int handle);

char *app_netapps_disk_get_name(unsigned int handle, int index);

char *app_netapps_disk_get_path(unsigned int handle, int index);

char *app_netapps_config_get_string(const char *key, char *buf, size_t buf_size, const char *dvalue);

int *app_netapps_config_get_int(const char *key, int *buf, int dvalue);

double *app_netapps_config_get_double(const char *key, double *buf, double dvalue);

int app_netapps_config_set_string(const char *key, const char *value);

int app_netapps_config_set_int(const char *key, int value);

int app_netapps_config_set_double(const char *key, double value);

unsigned int app_netapps_hw_malloc(unsigned int size);

void *app_netapps_hw_get(unsigned int handle);

void app_netapps_hw_free(unsigned int handle);

unsigned int app_netapps_gdi_init(netapps_gdi_type type, int pos_x, int pos_y, int width, int height);

void app_netapps_gdi_set_background(unsigned int handle, unsigned int color);

void app_netapps_gdi_set_foreground(unsigned int handle, unsigned int color);

void app_netapps_gdi_set_content(unsigned int handle, char *content);

int app_netapps_gdi_show(unsigned int handle);

void app_netapps_gdi_hide(unsigned int handle);

void app_netapps_gdi_destroy(unsigned int handle);

int app_netapps_set_app_id(const char *app_id);

int app_netapps_get_app_id(char *buf, size_t buf_size);

int app_netapps_chip_otp_write(unsigned int addr, unsigned char *buf, unsigned int size);

int app_netapps_chip_otp_read(unsigned int addr, unsigned char *buf, unsigned int size);

int app_netapps_chip_register_write(unsigned int addr, unsigned int value);

int app_netapps_chip_register_read(unsigned int addr, unsigned int *value);

#endif
