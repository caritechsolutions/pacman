#ifndef __SPEED_DETECT_H__
#define __SPEED_DETECT_H__

#include <stdio.h>
#include "gx_common.h"

typedef enum {
	MODE_FUNC  = (1<<0),
	MODE_TOTAL = (1<<1),
}SpeedDetectMode;

typedef enum {
	FUNC_READ  = 0,
	FUNC_WRITE = 1,
}SpeedDetectFunc;

typedef struct {
	unsigned int    data_size;
	unsigned int    cur_count;
	unsigned int    exec_count;
	unsigned int    costms_func;
	unsigned int    costms_total;

	GxTime          func_stime;
	GxTime          func_etime;
	GxTime          total_stime;
	GxTime          total_etime;

	SpeedDetectFunc func;
	SpeedDetectMode mode;
}SpeedDetect;

SpeedDetect* speed_detect_init();
int speed_detect_produce(SpeedDetect *speed_detect, int size);
int speed_detect_consume(SpeedDetect *speed_detect);
int speed_detect_set(SpeedDetect *speed_detect, int count, SpeedDetectMode mode);
int speed_detect_gxlogd(SpeedDetect *speed_detect);
int speed_detect_uninit(SpeedDetect *speed_detect);
#endif
