#ifndef __APP_EUTREL_DATABASE_H__
#define __APP_EUTREL_DATABASE_H__
#include "richepg_database.h"
#include "bus/module/pm/gxpm_manage.h"

#define FAST_SWITCH_PM_SUPPORT  1

/******** preset sat and ts manage ********/
int app_eutel_import_ts(const char *src);
int app_eutel_ts_mgr_init(void);
int app_eutel_ts_mgr_parse(void);
bool app_eutel_same_ts_check(const RichepgTsTP *tp1, const RichepgTsTP *tp2);
bool app_eutel_trusted_tp_check(GxBusPmDataTP *tp);
bool app_eutel_sat_type_check(int sat_id);
int app_eutel_ts_set_lineup(int id);
int app_eutel_sat_sel_set(int sel, bool change_store);
int app_eutel_sat_id_get(int sel);
int app_eutel_cur_sat_id_get(void);
GxBusPmDataSat *app_eutel_sat_data_get(int sel);
GxBusPmDataSat *app_eutel_cur_sat_data_get(void);
void app_eutel_sat_save_params(int sel,GxBusPmDataSat *p_sat);
int app_eutel_get_tp_by_sel(int sel,unsigned int *fre,unsigned int *symbol,unsigned int *polar);

/******** prog list manage ********/
char *app_eutel_proglist_prog_name_get(int num, RichepgProgNameItem *names, const char *lang);
int app_eutel_proglist_skip_set(bool skip_set);
int app_eutel_proglist_update(bool store_ch, bool del_all);
int app_eutel_proglist_update2(void);
int app_eutel_proglist_ear_update(void);
int app_eutel_database_reset(void);

/******** blind scan ********/
typedef struct {
    uint32_t               *sat_id;
    uint32_t                sat_num;
} EutelBlindScan;

int32_t app_eutel_blind_scan_start(EutelBlindScan *params);
int32_t app_eutel_blind_scan_stop(void);
int32_t app_eutel_blind_scan_check(RichepgTsTP **tp_array, int *tp_num, int *percent);

#endif
