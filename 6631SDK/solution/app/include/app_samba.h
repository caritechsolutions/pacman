#ifndef __APP_SAMBA_H__
#define __APP_SAMBA_H__

#include "gxtype.h"

#define MAX_SAMBA_SVR_NUM    (8)
#define MAX_SAMBA_SVR_PATH_LEN    (128)
#define MAX_SAMBA_USERNAME_LEN    (64)
#define MAX_SAMBA_PASSWD_LEN    (64)
#define MAX_SAMBA_LOCAL_PATH_LEN    (32)

typedef struct _AppSambaInfoClass
{
    uint32_t samba_id;
    char samba_svr_path[MAX_SAMBA_SVR_PATH_LEN + 1];
    char samba_username[MAX_SAMBA_USERNAME_LEN + 1];
    char samba_passwd[MAX_SAMBA_PASSWD_LEN + 1];
    char samba_local_path[MAX_SAMBA_LOCAL_PATH_LEN + 1];
    char samba_enable;
}AppSambaInfoClass;

typedef struct _AppSambaSrvListClass
{
    int samba_num;
    AppSambaInfoClass samba_list[MAX_SAMBA_SVR_NUM];
}AppSambaListClass;

typedef struct _AppSambaIDListClass
{
    int id_num;
    int id_list[MAX_SAMBA_SVR_NUM];
}AppSambaIDListClass;

typedef enum _AppSambaConnectStatus
{
    SAMBA_STATUS_DISCONNECTED = 0,
    SAMBA_STATUS_CONNECTED,
    SAMBA_STATUS_CONNECTING,
}AppSambaConnectStatus;

status_t app_samba_init(void);
AppSambaListClass *app_get_samba_List(void);
int32_t app_get_samba_num(void);
AppSambaInfoClass *app_get_samba_by_id(uint32_t samba_id);
uint32_t app_samba_connect(uint32_t samba_id);
uint32_t app_samba_disconnect(uint32_t samba_id);
AppSambaConnectStatus app_samba_check_connect(uint32_t samba_id);
uint32_t app_samba_new(AppSambaInfoClass *samba_info);
uint32_t app_samba_delete(uint32_t samba_id);
uint32_t app_samba_modify(uint32_t samba_id, AppSambaInfoClass *new_info);
uint32_t app_samba_svr_exist(const char *samba_svr_path);

#endif
