#ifndef __RICHEPG_EPG_H__
#define __RICHEPG_EPG_H__

#include "gxtype.h"
#include "gxcore.h"
#include "common/list.h"
#include "common/gxmalloc.h"
#include <time.h>
#include "richepg_json.h"
#include "richepg_store.h"

typedef struct {
    int event_id;
    time_t start_time;
    time_t finish_time;
    int parent_rate;
    int genres_num;
    int *genres_ids;
    char *event_logo;
    char *event_title;
    char *event_desc;
} RichepgEpgEvent;
typedef int (*richepg_epg_id_cb)(int i);
int richepg_epg_id_array_update(int num, richepg_epg_id_cb id_cb);
int richepg_epg_ctrl_release(void);
int richepg_epg_ctrl_init(size_t limit_mem);
int richepg_epg_add_start(int *total_channel); // total(null is DEFAULT_RICHEPG_EPG_PATH)
int richepg_epg_set_channel(int channel_id);
int richepg_epg_add_epg_event(RichepgEpgEvent *event);
int richepg_epg_add_finish(bool do_sort);
int richepg_epg_data_print(void);
int richepg_epg_event_info_get_by_time(time_t sec, int channel_id, RichepgEpgEvent *event, int size, bool desc_flag);
int richepg_epg_save(struct eutelsat_epg *epg, int *cnt); // return event num(>=0)
void richepg_epg_build(bool force_flag);
void richepg_epg_build_enable_set(bool enable_build, bool clear_rq);
bool richepg_epg_build_working_check(void);
int richepg_epg_store_save(const char *json_path, size_t limit_size); // return total event num(>=0)
int richepg_epg_store_parse(const char *json_path); // return total event num(>=0)
int richepg_epg_jsons_parse(const char *usb_entry); // return total event num(>=0)
int richepg_epg_event_total_get(int channel_id, time_t start_time, time_t finish_time);
int richepg_epg_event_info_get_by_pos(int pos, RichepgEpgEvent *event, int size, bool desc_flag);
int richepg_epg_event_info_get_by_id(int event_id, RichepgEpgEvent *event, int size, bool desc_flag);
int richepg_epg_event_info_get_cur(int channel_id, RichepgEpgEvent *event, int size, bool desc_flag);
int richepg_epg_event_info_get_next(int channel_id, RichepgEpgEvent *event, int size, bool desc_flag);
int32_t richepg_epg_channel_total_get(void);
time_t richepg_epg_last_finish_time_get(void);

// for "new epg window" similar to "PVR Recording window"
typedef struct {
    int event_id;
    time_t start_time;
    time_t finish_time;
    int genres_num;
    int *genres_ids;
    int event_flag;
} RichepgEpgBriefEvent;

typedef struct {
    int channel_id;
    int ch_channel_pos;
    int epg_channel_pos;
    int start_event_pos;
    int event_total;
    RichepgEpgBriefEvent *event_array;
} RichepgEpgBriefChannel;

typedef struct {
    int channel_total;
    RichepgEpgBriefChannel *channel_array;
} RichepgEpgBriefManage;

int richepg_epg_event_info_get(int channel_id, int channel_pos, int event_id, int event_pos, RichepgEpgEvent *event, int size, bool desc_flag);
int richepg_epg_brief_manage_update(RichepgEpgBriefManage *mgr, time_t start_time, time_t finish_time, void* (*alloc)(size_t size));

int richepg_epg_event_info_get_by_time_from_usb(time_t sec, int channel_id, int event_id, RichepgEpgEvent *event, int size);
#endif
