/*************************************************************************
	> File Name: app/include/module/app_parent_lock.h
	> Author:
	> Mail:
	> Created Time: 2020年07月24日 星期五 14时30分50秒
 ************************************************************************/

#ifndef _APP_PARENT_LOCK_H
#define _APP_PARENT_LOCK_H
#include"app_config.h"
#if PARENTAL_LOCK_SUPPORT

typedef enum
{
	PARENTAL_GUIDACE_OFF = 0,
	PARENTAL_GUIDACE_4,
	PARENTAL_GUIDACE_5,
	PARENTAL_GUIDACE_6,
	PARENTAL_GUIDACE_7,
	PARENTAL_GUIDACE_8,
	PARENTAL_GUIDACE_9,
	PARENTAL_GUIDACE_10,
	PARENTAL_GUIDACE_11,
	PARENTAL_GUIDACE_12,
	PARENTAL_GUIDACE_13,
	PARENTAL_GUIDACE_14,
	PARENTAL_GUIDACE_15,
	PARENTAL_GUIDACE_16,
	PARENTAL_GUIDACE_17,
	PARENTAL_GUIDACE_18,
	PARENTAL_GUIDACE_TOTAL
}ParentalGuidace;

void app_parent_lock_setting_menu_exec(void);
extern void app_dvbt_parent_lock_setting_menu_exec(void);
extern int app_parental_rating_msg_proc(GxMessage * msg);
extern void app_pvr_file_parental_rating_stop(void);
extern void app_pvr_file_parental_rating_start(uint16_t dmx_id,uint16_t ts_id, uint16_t service_id);
extern void app_parental_rating_set(int flag);
#endif
#endif
