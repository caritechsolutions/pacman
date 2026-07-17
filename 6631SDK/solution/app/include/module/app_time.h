/*
 * =====================================================================================
 *
 *       Filename:  app_time.h
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年09月14日 15时08分37秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#ifndef __APP_TIME_H__
#define __APP_TIME_H__

#include "gxcore.h"
#include "gui_core.h"
#include <time.h>

//Those time define under GMT:+0   UTC Time
#define SEC_TO_MILLISEC     1000
#define HOUR_TO_SEC         3600
#define HALF_HOUR_TO_SEC    1800

//#define SYS_DEFAULT_TIME_UTC (978307200)//Time:2001-01-01 00:00
#define SYS_DEFAULT_TIME_UTC (1420070400UL)//Time:2015-01-01 00:00
#define SYS_INIT_TIME_SEC (946684800) //sec of 2000.1.1 0:0
#define SYS_DEAD_TIME_SEC (2145916800) //sec of 2038.1.1 0:0

enum _ymd
{
    TIME_YMD,
    TIME_DMY,
    TIME_YMD_END
};

enum _gmt_local
{
    TIME_GMT,
    TIME_LOCAL,
    TIME_NET,
    TIME_GL_END
};

enum _summer
{
    TIME_SUMMER_OFF,
    TIME_SUMMER_ON,
    TIME_SUMMER_END
};

typedef struct
{
    enum _ymd           ymd;
    enum _gmt_local     gmt_local;
    enum _summer        summer;
    int32_t             zone;
    int32_t             half_zone;
#define TIME_MODE_IGNORE    (0xffff)
}time_cfg;

typedef struct app_time AppTime;
struct app_time
{
    time_cfg    config;
    time_t      seconds;
    event_list* timer;
    void        (*start)(AppTime*);
    void        (*stop)(AppTime*);
    void        (*sync)(AppTime*,time_t);
    void        (*mode_set)(AppTime*,time_cfg*);
    void        (*time_get)(AppTime*,char*,size_t); // minsize = 20
    time_t      (*utc_get)(AppTime*);       // need add 1900(Year)
    void        (*time_sync_cb)(AppTime*);
    void        (*init)(AppTime*);
#define MIN_SEC_OFFSET  (11)
};
extern AppTime g_AppTime;

bool app_time_range_valid(time_t gmt_sec);
bool app_month_date_valid(uint16_t year, uint8_t mon, uint8_t day);
time_t app_mktime (time_t year, time_t mon, time_t day, time_t hour, time_t min, time_t sec);

char *app_time_zone_country_get_by_sel(int zone_sel);
int app_time_zone_to_sel(int zone, int half_zone_flag);
int app_time_sel_to_zone(int sel);
int app_time_sel_to_halfzone(int sel);
time_t app_time_zone_interval_sec_get(int zone_interval, int summer_interval);
time_t app_time_zone_sec_get(int zone, int half_zone, int summer);
void app_time_zone_config_get(int *zone, int *half_zone, int *summer);
void app_time_zone_config_set(int *zone, int *half_zone, int *summer);
time_t app_time_to_local_time(time_t sec, int zone, int half_zone, int summer);
time_t app_time_to_utc_time(time_t sec, int zone, int half_zone, int summer);
char *app_time_zone_content_get(void);
char **app_time_zone_name_array_get(void);
int app_time_zone_name_array_size_get(void);

#endif

