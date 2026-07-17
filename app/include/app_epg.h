#ifndef APP_EPG_H
#define APP_EPG_H
#include "service/gxepg.h"
#include "app_data_type.h"

#define MAX_EPG_DAY 7
#define EVENT_GET_NUM 1
#define EVENT_GET_MAX_SIZE 5000
#define EVENT_NAME_MAX_SIZE 128

typedef struct
{
	int cur_cnt;
	int day_cnt[MAX_EPG_DAY];
}EventCnt;

typedef struct
{
	uint16_t type;
    uint16_t event_id;
	time_t start_time;  // local time
	time_t finish_time;
	char event_name[EVENT_NAME_MAX_SIZE];
}EpgIndex;

typedef struct
{
	uint16_t ts_id;
	uint16_t orig_network_id;
	uint16_t service_id;
	uint32_t prog_id;
	uint32_t tp_id;
	int prog_pos;
}EpgProgInfo;

typedef struct
{
    int32_t want_get_cnt;
    time_t start_time;
    time_t end_time;
    EpgInfoLevel level;
}EpgPeriodPara;

typedef struct epg_ops AppEpgOps;
struct epg_ops
{
	status_t (*prog_info_update)(AppEpgOps*, EpgProgInfo*);
    int32_t (*get_schedule_event_cnt)(AppEpgOps*, time_t, time_t);
	status_t (*get_schedule_event_info)(AppEpgOps*, GxEpgInfo*, EpgPeriodPara);
    int32_t (*get_cur_event_cnt)(AppEpgOps*);
	status_t (*get_cur_event_info)(AppEpgOps*, GxEpgInfo*);
	status_t (*get_next_event_info)(AppEpgOps*, GxEpgInfo*);
    status_t (*get_event_info_by_event_id)(AppEpgOps*, GxEpgInfo*, uint16_t);
	EpgProgInfo prog_info;
};

void epg_ops_init(AppEpgOps * ops);

void app_epg_enable(uint32_t ts_src, uint32_t dmx_id);
void app_epg_disable(void);
void app_epg_info_clean(void);
void app_epg_info_full_speed(void);

extern uint32_t app_epg_get_play_prog_tp_id(void);
extern int app_epg_ts_id_filter_modify(uint32_t ts_id);
extern int app_epg_filter_pause(void);
extern int app_epg_filter_resume(void);
extern void app_epg_filter_reset(void);
extern void app_epg_prog_info_set(EpgProgInfo *epg_prog_info, GxMsgProperty_NodeByPosGet *node_prog);
char *app_get_epg_short_lang_str(uint32_t lang);
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
extern void app_epg_detail_filter_clean(void);
extern void app_epg_detail_filter_set(uint16_t ts_id,uint16_t service_id, uint16_t original_id, uint16_t event_id);
#endif
#endif
