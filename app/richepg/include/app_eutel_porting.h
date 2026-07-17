
/*
==============================================================================
 *
 *       Filename:  app_eutel_porting.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2024年7月22日 15时31分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * ===========================================================================

可能随着版本变化的一些接口或者调用外部接口可以移植到这里。

*/

#ifndef APP_EUTEL_PORTING_H
#define APP_EUTEL_PORTING_H

#include <time.h>
#include "module/app_time.h"

#define APP_SAT_TYPE_IS_FREE   (0)
#define APP_SAT_TYPE_IS_FUSE   (1)
#define APP_SAT_TYPE_IS_EUTEL  (2)

int  app_eutel_porting_get_sat_type(unsigned int sat_id);

int  app_eutel_porting_set_sat_search_mode(int mode);
int  app_eutel_porting_get_sat_search_mode(void);

int  app_eutel_porting_sat_opt_init(void);
int  app_eutel_antenan_setting_ops_init(void);
int  app_eutel_number_ops_init(void);
int  app_eutel_channel_ops_init(void);

int  app_eutel_channel_easy_data_is_ready(int sat_id);
int  app_eutel_porting_channel_get_lcn(int sel);
int  app_eutel_channel_easy_data_uninit(void);

char *app_eutel_porting_get_iso6392_code(uint32_t lang);

void app_number_response_cb_set(int (*cb)(int num_value));

int app_time_check_mode_sync(AppTime *tm, time_t utc_time);

void app_eutel_parental_rating_set(int flag);

#endif

