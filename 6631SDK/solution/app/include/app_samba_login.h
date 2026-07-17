#ifndef __APP_SAMBA_LOGIN_H__
#define __APP_SAMBA_LOGIN_H__

#include "app_config.h"

#if SAMBA_SUPPORT

#include "app_samba.h"

typedef void (*SambaLoginExitCb)(bool save_ret, AppSambaInfoClass *samba_out);
status_t app_create_samba_login(AppSambaInfoClass *samba_in, AppSambaInfoClass *samba_ret, SambaLoginExitCb exit_cb);

#endif
#endif
