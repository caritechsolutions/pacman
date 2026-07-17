#ifndef __APP_WEBLET_H__
#define __APP_WEBLET_H__
#include "module/app_log.h"
//#ifndef DEBUG
#define DEBUG(fmt, x...) app_log_debug("%s(line %d): "fmt, __FILE__, __LINE__, ##x)
#define WARNING(fmt, x...) app_log_info("\033[1;33m%s(line %d): "fmt"\033[m", __FILE__, __LINE__, ##x)
#define ERROR(fmt, x...) app_log_error("\033[1;31m%s(line %d): "fmt"\033[m", __FILE__, __LINE__, ##x)
//#endif

typedef enum{
    PREV_GROUP=0,
    NEXT_GROUP
}GroupDir;

int app_weblet_init(char *host_ip);
bool app_check_weblet_server_state(void);
char *app_get_weblet_server_default_port(void);
void app_weblet_uninit(void);
int app_qr_ip_addr_get(char *ip_get);
int app_weblet_pop_rc_qr(void);
int app_weblet_switch_cmb_group(GroupDir dir);
status_t app_kb_get_weblet_content_to_edit(char *edit);
status_t app_kl_get_edit_content(char *input);
status_t app_kb_get_weblet_content_to_edit_save(char *edit);
void app_kb_exec_edit_key(void);
#endif

