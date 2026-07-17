#ifndef __APP_SAMBA_DB_H__
#define __APP_SAMBA_DB_H__

#include "app_config.h"
#if SAMBA_SUPPORT

#include "app_samba.h"

int app_samba_db_int(AppSambaInfoClass (*samba_list)[MAX_SAMBA_SVR_NUM]);
status_t app_samba_db_add(AppSambaInfoClass *samba_info);
status_t app_samba_db_delete(uint32_t samba_id);
status_t app_samba_db_modify(uint32_t samba_id, AppSambaInfoClass *samba_info);
AppSambaIDListClass *app_samba_db_get_idlist(void);

#endif
#endif
