/*
 * =====================================================================================
 *       Filename:  app_time.c
 *
 *    Description:  set utc time to localtime,  provide multi-time mode
 *
 *        Version:  1.0
 *        Created:  2011年09月14日 09时27分24秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_module.h"
#include "module/si/si_pmt.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "gui_core.h"
#include "module/config/gxconfig.h"
#include "common/gxpm.h"
#include "app_utility.h"

#define TIME_PRECISION  (3)
#define UTC_STORE_SEC   600
#define UTC_STORE_PATH  "/home/gx/utc_timestamp"

/**************** time zone api start ****************/
#define HALF_TIME_ZONE_SUPPORT  1   // GEMINI must enable it.

#if HALF_TIME_ZONE_SUPPORT
#define HALF_ZONE_TIMES         24
#define TIME_ZONE_CONTENT       "[GMT -12,GMT -11.5,GMT -11,GMT -10.5,GMT -10,GMT -9.5,GMT -9,GMT -8.5,GMT -8,GMT -7.5,GMT -7,GMT -6.5,GMT -6,GMT -5.5,GMT -5,GMT -4.5,GMT -4,GMT -3.5,GMT -3,GMT -2.5,GMT -2,GMT -1.5,GMT -1,GMT -0.5,GMT +0,GMT +0.5,GMT +1,GMT +1.5,GMT +2,GMT +2.5,GMT +3,GMT +3.5,GMT +4,GMT +4.5,GMT +5,GMT +5.5,GMT +6,GMT +6.5,GMT +7,GMT +7.5,GMT +8,GMT +8.5,GMT +9,GMT +9.5,GMT +10,GMT +10.5,GMT +11,GMT +11.5,GMT +12]"
static char *s_time_zone_array[] = {"GMT -12","GMT -11.5","GMT -11","GMT -10.5","GMT -10","GMT -9.5","GMT -9","GMT -8.5","GMT -8","GMT -7.5","GMT -7","GMT -6.5","GMT -6","GMT -5.5","GMT -5","GMT -4.5","GMT -4","GMT -3.5","GMT -3","GMT -2.5","GMT -2","GMT -1.5","GMT -1","GMT -0.5","GMT +0","GMT +0.5","GMT +1","GMT +1.5","GMT +2","GMT +2.5","GMT +3","GMT +3.5","GMT +4","GMT +4.5","GMT +5","GMT +5.5","GMT +6","GMT +6.5","GMT +7","GMT +7.5","GMT +8","GMT +8.5","GMT +9","GMT +9.5","GMT +10","GMT +10.5","GMT +11","GMT +11.5","GMT +12"};

char *app_time_zone_country_get_by_sel(int zone_sel)
{
    char *country_str = NULL;

    switch (zone_sel)
    {
        case 4:  country_str = STR_ID_UTC_1000_; break;
        case 6:  country_str = STR_ID_UTC_0900_; break;
        case 8:  country_str = STR_ID_UTC_0800_; break;
        case 10: country_str = STR_ID_UTC_0700_; break;
        case 12: country_str = STR_ID_UTC_0600_; break;
        case 14: country_str = STR_ID_UTC_0500_; break;
        case 16: country_str = STR_ID_UTC_0400_; break;
        case 18: country_str = STR_ID_UTC_0300_; break;

        case 24: country_str = STR_ID_UTC_0000;  break;
        case 26: country_str = STR_ID_UTC_0100;  break;
        case 28: country_str = STR_ID_UTC_0200;  break;
        case 30: country_str = STR_ID_UTC_0300;  break;
        case 32: country_str = STR_ID_UTC_0400;  break;
        case 34: country_str = STR_ID_UTC_0500;  break;
        case 36: country_str = STR_ID_UTC_0600;  break;
        case 38: country_str = STR_ID_UTC_0700;  break;
        case 40: country_str = STR_ID_UTC_0800;  break;
        case 42: country_str = STR_ID_UTC_0900;  break;
        case 44: country_str = STR_ID_UTC_1000;  break;
        case 48: country_str = STR_ID_UTC_1200;  break;
        default: break;
    }
    return country_str;
}

int app_time_zone_to_sel(int zone, int half_zone_flag)
{
    return (2*(zone + 12)+half_zone_flag);
}

int app_time_sel_to_zone(int sel)
{
    return ((sel-HALF_ZONE_TIMES)/2);
}

int app_time_sel_to_halfzone(int sel)
{
    int half_zone_flag = 0;
    if((sel-HALF_ZONE_TIMES)%2 == 0)
    {
        half_zone_flag = 0 ;
    }
    else if(sel < HALF_ZONE_TIMES)
    {
        half_zone_flag = -1;
    }
    else if(sel > HALF_ZONE_TIMES)
    {
        half_zone_flag = 1;
    }
    return half_zone_flag;
}

time_t app_time_zone_interval_sec_get(int zone_interval, int summer_interval)
{
    return (zone_interval * HALF_HOUR_TO_SEC + summer_interval * HOUR_TO_SEC);
}

time_t app_time_zone_sec_get(int zone, int half_zone, int summer)
{
    time_t sec = 0;
    sec += zone * HOUR_TO_SEC;
    sec += half_zone * HALF_HOUR_TO_SEC;
    sec += summer * HOUR_TO_SEC;
    return sec;
}

#else
#define HALF_ZONE_TIMES         12
#define TIME_ZONE_CONTENT       "[-12,-11,-10,-9,-8,-7,-6,-5,-4,-3,-2,-1,0,+1,+2,+3,+4,+5,+6,+7,+8,+9,+10,+11,+12]"
static char *s_time_zone_array[] = {"-12","-11","-10","-9","-8","-7","-6","-5","-4","-3","-2","-1","0","+1","+2","+3","+4","+5","+6","+7","+8","+9","+10","+11","+12"};

char *app_time_zone_country_get_by_sel(int zone_sel)
{
    char *country_str = NULL;

    switch (zone_sel)
    {
        case 2:  country_str = STR_ID_UTC_1000_; break;
        case 3:  country_str = STR_ID_UTC_0900_; break;
        case 4:  country_str = STR_ID_UTC_0800_; break;
        case 5:  country_str = STR_ID_UTC_0700_; break;
        case 6:  country_str = STR_ID_UTC_0600_; break;
        case 7:  country_str = STR_ID_UTC_0500_; break;
        case 8:  country_str = STR_ID_UTC_0400_; break;
        case 9:  country_str = STR_ID_UTC_0300_; break;

        case 12: country_str = STR_ID_UTC_0000;  break;
        case 13: country_str = STR_ID_UTC_0100;  break;
        case 14: country_str = STR_ID_UTC_0200;  break;
        case 15: country_str = STR_ID_UTC_0300;  break;
        case 16: country_str = STR_ID_UTC_0400;  break;
        case 17: country_str = STR_ID_UTC_0500;  break;
        case 18: country_str = STR_ID_UTC_0600;  break;
        case 19: country_str = STR_ID_UTC_0700;  break;
        case 20: country_str = STR_ID_UTC_0800;  break;
        case 21: country_str = STR_ID_UTC_0900;  break;
        case 22: country_str = STR_ID_UTC_1000;  break;
        case 24: country_str = STR_ID_UTC_1200;  break;
        default: break;
    }
    return country_str;
}

int app_time_zone_to_sel(int zone, int half_zone_flag)
{
    return (zone + HALF_ZONE_TIMES);
}

int app_time_sel_to_zone(int sel)
{
    return (sel-HALF_ZONE_TIMES);
}

int app_time_sel_to_halfzone(int sel)
{
    return 0;
}

time_t app_time_zone_interval_sec_get(int zone_interval, int summer_interval)
{
    return (zone_interval * HOUR_TO_SEC + summer_interval * HOUR_TO_SEC);
}

time_t app_time_zone_sec_get(int zone, int half_zone, int summer)
{
    time_t sec = 0;
    sec += zone * HOUR_TO_SEC;
    sec += summer * HOUR_TO_SEC;
    return sec;
}
#endif

void app_time_zone_config_get(int *zone, int *half_zone, int *summer)
{
    int init_value = 0;

    GxBus_ConfigGetInt(TIME_ZONE, &init_value, TIME_ZONE_VALUE);
    if (init_value < -12)
        init_value = -12;
    if (init_value > 12)
        init_value = 12;
    if (zone)
        *zone = init_value;

    GxBus_ConfigGetInt(TIME_HALF_ZONE, &init_value, TIME_HALF_ZONE_VALUE);
    if (half_zone)
        *half_zone = init_value;

    GxBus_ConfigGetInt(TIME_SUMMER, &init_value, TIME_SUMMER_VALUE);
    if (summer)
        *summer = init_value;
}

void app_time_zone_config_set(int *zone, int *half_zone, int *summer)
{
    if (zone)
        GxBus_ConfigSetInt(TIME_ZONE, *zone);
    if (half_zone)
        GxBus_ConfigSetInt(TIME_HALF_ZONE, *half_zone);
    if (summer)
        GxBus_ConfigSetInt(TIME_SUMMER, *summer);
}

time_t app_time_to_local_time(time_t sec, int zone, int half_zone, int summer)
{
    return (sec + app_time_zone_sec_get(zone, half_zone, summer));
}

time_t app_time_to_utc_time(time_t sec, int zone, int half_zone, int summer)
{
    return (sec - app_time_zone_sec_get(zone, half_zone, summer));
}

char *app_time_zone_content_get(void)
{
    return TIME_ZONE_CONTENT;
}

char **app_time_zone_name_array_get(void)
{
    return s_time_zone_array;
}

int app_time_zone_name_array_size_get(void)
{
    return (sizeof(s_time_zone_array) / sizeof(s_time_zone_array[0]));
}

/**************** time zone api end ****************/

time_t app_time_store_get(void)
{
    time_t timestamp = 0;
    FILE *fp = NULL;
    const char *path = UTC_STORE_PATH;

    if ((fp = fopen(path, "r")) == NULL)
        return 0;
    if (sizeof(timestamp) != fread(&timestamp, 1, sizeof(timestamp), fp)) {
        timestamp = 0;
    }
    fclose(fp);

    return timestamp;
}

int app_time_store_set(time_t timestamp)
{
    FILE *fp = NULL;
    const char *path = UTC_STORE_PATH;

    if ((fp = fopen(path, "w")) == NULL)
        return -1;
    if (sizeof(timestamp) != fwrite(&timestamp, 1, sizeof(timestamp), fp)) {
        fclose(fp);
        return -1;
    }
    fflush(fp);
    fsync(fileno(fp));
    fclose(fp);

    return 0;
}

void app_time_store_reset(void)
{
    app_time_store_set(SYS_DEFAULT_TIME_UTC);
}

static void app_time_sync(AppTime *tm, time_t utc_time)
{
    GxTime sys_time;

    sys_time.seconds = utc_time;
    sys_time.microsecs = 0;
    GxCore_SetLocalTime(&sys_time);

    tm->seconds = sys_time.seconds;
}

static void app_time_init(AppTime *tm)
{
#if OTT_SUPPORT
    GxBus_ConfigSetInt(TIME_MODE, TIME_NET);
#endif
    unsigned int sWakeFlag = 0;
#if DEMOD_DVB_C && DEMOD_DVB_T && (0 == DEMOD_DVB_S) && NDEMOD_WITH_1TUNER
    extern int app_t2_time_zone_init(void);
    app_t2_time_zone_init();
#endif
    GxCore_GetWakeFlag(&sWakeFlag);
    if(sWakeFlag != WAKE_FROM_POWER_OFF)
    {
        return;
    }
    app_time_sync(tm, SYS_DEFAULT_TIME_UTC);//back to 2001 .1.1 0:0
}

static int time_timeout_cb(void *usrdata)
{
    GxTime sys_time;
    GxCore_GetLocalTime(&sys_time);
    if(sys_time.seconds >= SYS_DEAD_TIME_SEC)
    {
       app_time_sync((AppTime*)usrdata, SYS_DEFAULT_TIME_UTC);
    }
    else
    {
    	((AppTime*)usrdata)->seconds = sys_time.seconds;
    }

    return 0;
}

static void app_time_stop(AppTime *tm)
{
    app_send_msg_exec(GXMSG_EXTRA_RELEASE, NULL);

    remove_timer(tm->timer);
    tm->timer = NULL;
}

static void app_time_start(AppTime *tm)
{
    int32_t value = 0;

    if (tm == NULL) return;


    // time config
    GxBus_ConfigGetInt(TIME_DISP, &value, TIME_DISP_VALUE);
    tm->config.ymd = value;

    GxBus_ConfigGetInt(TIME_MODE, &value, TIME_MODE_VALUE);
    tm->config.gmt_local = value;

    app_time_zone_config_get(&tm->config.zone, &tm->config.half_zone, &value);
    tm->config.summer = value;

    if(tm->config.gmt_local == TIME_GMT)
    {
        app_log_debug("enter %s:%d  tm->config.gmt_local  = %d \n",__func__,__LINE__,tm->config.gmt_local);
        GxMsgProperty_ExtraSyncTime time;
        time.ts_src = g_AppPlayOps.normal_play.ts_src;
        app_send_msg_exec(GXMSG_EXTRA_SYNC_TIME, (void *)(&time));
    }
    else if(tm->config.gmt_local == TIME_NET)
    {
#if GET_NET_TIME_SUPPORT || OTT_SUPPORT
        extern int app_net_time_sync_message(void);
        app_net_time_sync_message();
#endif
    }
    else
    {
        app_time_stop(tm);
    }

    if (reset_timer(tm->timer) != 0)
    {
		time_timeout_cb(tm);
        tm->timer = create_timer(time_timeout_cb,
                TIME_PRECISION*SEC_TO_MILLISEC, tm, TIMER_REPEAT);
    }
}

static void app_time_set(AppTime *tm, time_cfg *cfg)
{
    if (tm == NULL || cfg == NULL)      return;
    int value = 0;
    int *zone = NULL, *half_zone = NULL, *summer = NULL;

    if (cfg->ymd < TIME_YMD_END)
    {
        tm->config.ymd = cfg->ymd;
        GxBus_ConfigSetInt(TIME_DISP, (int32_t)(tm->config.ymd));
    }
    if (cfg->gmt_local < TIME_GL_END)
    {
        tm->config.gmt_local = cfg->gmt_local;
        GxBus_ConfigSetInt(TIME_MODE, (int32_t)(tm->config.gmt_local));
    }
    if (cfg->zone >= -12 && cfg->zone <= 12)
    {
        tm->config.zone = cfg->zone;
        zone = &tm->config.zone;
    }
    if (cfg->half_zone >= -1 && cfg->half_zone <= 1)
    {
        tm->config.half_zone = cfg->half_zone;
        half_zone = &tm->config.half_zone;
    }

    if (cfg->summer< TIME_SUMMER_END)
    {
        tm->config.summer = cfg->summer;
        value = tm->config.summer;
        summer = &value;
    }
    app_time_zone_config_set(zone, half_zone, summer);
}

static void app_time_get(AppTime *tm, char *tm_str, size_t str_len)
{
#define TIME_STR_LEM    (20)
    time_t seconds;

    if (str_len < TIME_STR_LEM)
    {
        app_log_error("[time] buf too small\n");
        return;
    }

    seconds = app_time_to_local_time(tm->seconds, tm->config.zone, tm->config.half_zone, tm->config.summer);
    app_common_data_time_edit_str(seconds, LONG_DATE_EDIT | SHORT_TIME_EDIT, tm_str, str_len);
}

static time_t app_utc_get(AppTime *tm)
{
    return tm->seconds;
}

bool app_time_range_valid(time_t gmt_sec)
{
    if((gmt_sec >= SYS_INIT_TIME_SEC)
        && (gmt_sec < SYS_DEAD_TIME_SEC))
    {
        return true;
    }

    return false;
}

bool app_month_date_valid(uint16_t year, uint8_t mon, uint8_t day)
{
	bool ret = true;

	if(year < 1970 || year > 2038)
		return false;

	switch(mon)
	{
		case 4:
		case 6:
		case 9:
		case 11:
			if(day > 30)
			{
				ret = false;
			}
			break;

		case 2:
			if((year % 400 == 0)
				|| ((year % 4 == 0) && (year % 100 != 0)))
			{
				if(day > 29)
				{
					ret = false;
				}
			}
			else
			{
				if(day > 28)
				{
					ret = false;
				}
			}
			break;

		default:
			if(day > 31)
			{
				ret = false;
			}
			break;
	}

	return ret;
}

time_t app_mktime (time_t year, time_t mon, time_t day, time_t hour, time_t min, time_t sec)
{
    if (0 >= (int) (mon -= 2))
    {    /**//* 1..12 -> 11,12,1..10 */
        mon += 12;      /**//* Puts Feb last since it has leap day */
        year -= 1;
    }

    return (((
                 (time_t) (year/4 - year/100 + year/400 + 367*mon/12 + day) +
                 year*365 - 719499
             )*24 + hour /**//* now have hours */
            )*60 + min /**//* now have minutes */
           )*60 + sec; /**//* finally seconds */
}

AppTime g_AppTime =
{
    .start          = app_time_start,
    .stop           = app_time_stop,
    .sync           = app_time_sync,
    .mode_set       = app_time_set,
    .time_get       = app_time_get,
    .utc_get        = app_utc_get,
    .init           = app_time_init,
    .time_sync_cb   = NULL,
};

#if WEBAPI_SUPPORT
void app_webapi_time_get(int *timezone, int *summer_time, int *year, int *month, int *day, int *hour, int *min, int *sec)
{
    time_t time_sec = 0;
    struct tm ptm = {0};
    int half = 0;
    int zone = 0;
    int summer = 0;
    time_sec = get_display_time_by_timezone(time(NULL));
    localtime_r(&time_sec, &ptm);
    *year = ptm.tm_year+1900;
    *month = ptm.tm_mon+1;
    *day = ptm.tm_mday;
    *hour = ptm.tm_hour;
    *min = ptm.tm_min;
    *sec = ptm.tm_sec;
    app_time_zone_config_get(&zone,&half,&summer);
    *timezone = zone;
    *summer_time = summer;
}

void app_webapi_time_set(int timezone, int summer_time, int year, int month, int day, int hour, int min, int sec)
{
    time_t time = 0;
    int half_zone = 0;

    if(summer_time != 0)
        summer_time = 1;

    app_time_zone_config_set(&timezone, &half_zone, &summer_time);
    time = get_local_time_by_timezone(app_mktime(year, month, day, hour, min, sec));
    g_AppTime.sync(&g_AppTime, time);
}
#endif

