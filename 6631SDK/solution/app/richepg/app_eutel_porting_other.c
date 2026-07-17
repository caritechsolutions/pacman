/*
    20240722
*/

#include "app_config.h"

#if RICHEPG_SUPPORT
#include "app.h"
#include "app_eutel_porting.h"
#include "app_eutel_database.h"
#include "app_eutel_sat_fuse.h"
#include "app_eutel_common.h"
#include "app_windows.h"

extern char *app_get_common_short_lang_str(uint32_t lang);
extern void app_system_reset(void);
extern void app_parental_rating_set(int flag);

char *app_eutel_porting_get_iso6392_code(uint32_t lang)
{
    return app_get_common_short_lang_str(lang);
}

static int _eutel_reboot_cb(PopDlgRet ret)
{
    app_reboot();
    return 0;
}

static int _eutel_reset_poptip_creat_cb(void)
{
    bool flag = !app_richepg_certification_access_get();
    GUI_SetInterface("flush",NULL);
    app_system_reset();

    app_richepg_certification_access_set(flag);

    app_pop_reset_timeout(1);
    app_pop_set_string(STR_ID_REBOOT_NOW);
    app_pop_set_exitcb(_eutel_reboot_cb);

    return 0;
}

int app_factory_reset(void)
{
   PopDlg  pop;
   memset(&pop, 0, sizeof(PopDlg));
   pop.type = POP_TYPE_NO_BTN;
   pop.mode = POP_MODE_UNBLOCK;
   pop.format = POP_FORMAT_DLG;
   pop.timeout_sec = 2;
   pop.str  = STR_ID_PLZ_WAIT;
   pop.create_cb = _eutel_reset_poptip_creat_cb;
   popdlg_create(&pop);

   return 0 ;
}

int app_time_check_mode_sync(AppTime *tm, time_t utc_time)
{
    int32_t value = 0;

    if (tm == NULL) return 0;

    GxBus_ConfigGetInt(TIME_MODE, &value, TIME_MODE_VALUE);
    if (value == TIME_LOCAL )
    {
        app_log_debug("current is local time mode\n");
        return 0 ;
    }
    if ( tm->sync )
        tm->sync(tm,utc_time);

    return 1 ;
}

void app_eutel_parental_rating_set(int flag)
{
#if PARENTAL_LOCK_SUPPORT
	app_parental_rating_set(flag);
#endif
}

#endif

