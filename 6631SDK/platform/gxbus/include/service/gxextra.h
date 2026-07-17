#ifndef __GX_EXTRA_H__
#define __GX_EXTRA_H__

#include "gxbus.h"
#include <time.h>
#include "module/si/si_tot.h"

///< 是否持续过滤tdt tot 默认为no
// 开启方式： GxBus_ConfigSetInt(GXBUS_EXTRA_FILTER_CONTINULY,GXBUS_EXTRA_YES);
#define GXBUS_EXTRA_FILTER_CONTINULY "gxextra_filter_continuly"
#define GXBUS_EXTRA_YES 1
#define GXBUS_EXTRA_NO  0

///< 和系统时间的偏差价值 ，当tdt tot时间和系统时间的差值>=偏差之 发送SYNC_OK 默认为不考虑偏差值
//单位为秒
//配置方式 GxBus_ConfigSetInt(GXBUS_EXTRA_COMPARE_TIME,10);
#define GXBUS_EXTRA_COMPARE_TIME "gxextra_compare_time"

typedef enum {
	TDT_TIME,
	TOT_TIME
}FOUNTAIN;

typedef struct
{
	uint8_t     offset_time_count;
	time_offset offset_time[MAX_OFFSET_TIME_COUNT];
	time_t      utc_second;
	FOUNTAIN    time_fountain;
}GxMsgProperty_ExtraTimeOk;

typedef struct
{
	uint32_t ts_src;    // 通过哪一路TS去过滤tdt或者tot，TS1-0  TS2-1  TS3-2 ...
}GxMsgProperty_ExtraSyncTime;

#endif /* __GX_EXTRA_H__ */


