#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_epg.h"
#include "app_utility.h"
#include "app_book.h"
#include "app_windows.h"
#include "app_pop.h"
#include "full_screen.h"
#include "channel_common.h"
#include "app_channel_info.h"

extern bool app_get_same_start_play_or_pvr_book(uint16_t prog_id, time_t start_time, GxBook *book_ret);
extern void app_timer_edit_ex_func_register(void (*exit_func)(void));
void app_epg_timer_edit_exit_cb(void);
extern bool app_epg_timer_menu_exec(time_t start_time, time_t duration, int prog_id, const char *prog_name);
extern bool app_check_play_exec_flag(void);

#define LISTVIEW_EPG_PROG               "listview_epg_prog"
#define TIMELIST_EPG_EVENT              "timelist_epg_event"
#define LISTVIEW_EPG_EVENT              "listview_epg_event"
#define TEXT_EGP_DATE                   "text_epg_date"
#define TEXT_EPG_SIGNAL                 "txt_epg_signal"

// brief event
#define TEXT_EPG_PROG_NAME              "text_epg_event_prog_name"
#define TEXT_EPG_EVENT_NAME             "text_epg_event_event_name"
#define TEXT_EPG_START_TIME             "text_epg_event_details_startT1"
#define TEXT_EPG_TIME_INTERVAL          "text_epg_event_details_interval1"
#define TEXT_EPG_END_TIME               "text_epg_event_details_endT1"

// event details
#define IMG_EPG_EVENT_DETAILS_BACK      "img_epg_event_details_back"
#define IMG_EPG_EVENT_DETAILS_LINE5     "img_epg_event_details_line5"
#define IMG_EPG_EVENT_DETAILS_LINE6     "img_epg_event_details_line6"
#define IMG_EPG_DETAILS_BACK            "img_epg_details_back"
#define TEXT_EVENT_TITLE                "text_epg_event_details_title"
#define TEXT_EVENT_START_TIME           "text_epg_event_details_startT"
#define TEXT_EVENT_TIME_INTERVAL        "text_epg_event_details_interval"
#define TEXT_EVENT_END_TIME             "text_epg_event_details_endT"
#define NOTEPAD_EPG_EVENT               "notepad_epg_event_details"
#define BTN_EVNET_DETAILS_EXIT          "btn_epg_event_details_exit"

// tip
#define IMG_EPG_BOOK                    "img_epg_red_tip"
#define IMG_EPG_PREV_DAY                "img_epg_green_tip"
#define IMG_EPG_NEXT_DAY                "img_epg_yellow_tip"
#define IMG_EPG_MODE                    "img_epg_blue_tip"
#define IMG_EPG_SHOW_DETAIL             "img_epg_show_detail_tip"
#define TEXT_EPG_BOOK                   "text_epg_book_tip"
#define TEXT_EPG_PREV_DAY               "text_epg_prev_day_tip"
#define TEXT_EPG_NEXT_DAY               "text_epg_next_day_tip"
#define TEXT_EPG_MODE                   "text_epg_mode_tip"
#define TEXT_EPG_SHOW_DETAIL            "text_epg_show_detail_tip"

#define SEC_PER_MIN                     60
#define SEC_PER_HOUR                    3600  // 60*60
#define SEC_PER_DAY                     86400 // 60*60*12

#define DISPLAY_ZONE_HOUR               4
#if LVLIST_EPG_SUPPORT
#define MAX_DISPLAY_EVENT               120
#else
#define MAX_DISPLAY_EVENT               80
#endif

#if APP_MAIN_MENU_OBJ_SUPPORT
#define MAX_EPG_PROG                    6
#else
#if(THEME_CLASSIC_DARK == 1)
#define MAX_EPG_PROG                    5
#else
#define MAX_EPG_PROG                    6
#endif
#endif

enum {
	TMLIST_EPG_STYLE,
	LVLIST_EPG_STYLE
};
enum {
	EPG_CUR_EVENT = 0xfffc,
	EPG_NEXT_EVENT = 0xfffd,
    EPG_DETAIL_EVENT = 0xfffe,
	EPG_INVALID_EVENT = 0xffff
};
enum {
	CUR_EPG_FLAG = 1 << 0,
	PLAY_EPG_FLAG = 1 << 1,
	PVR_EPG_FLAG = 1 << 2
};

typedef struct {
	uint16_t vsel;
	uint16_t flag;
	uint16_t type;
    uint16_t event_id;
	time_t start_time;
    time_t end_time;
} EpgListIndex;

typedef struct {
	int total;
	EpgListIndex *index;
	char **detail;
} EpgListData;

typedef struct {
	int         epg_style;
	bool        detail_show;
	bool        update_flag;
	int         day_sel;
	int         now_sel;

	time_t      zone_sec;
	time_t      local_sec;      // = utc_time + zone_sec
	time_t      local_hour;     // = local_sec - local_sec % SEC_PER_HOUR
	time_t      local_day;      // = local_hour - local_hour % SEC_PER_DAY
	time_t      next_event_sec;
	time_t      tmlist_start_sec;
	time_t      focus_time_bak;

	GxMsgProperty_NodeByPosGet cur_prog;

	EpgIndex*   tmp_index;
	EpgIndex*   use_index;
	GxEpgInfo*  tmp_info;      //epg基本信息,开辟的空间不包含detail
	GxEpgInfo*  cur_info;
	GxEpgInfo*  next_info;
	GxEpgInfo*  focus_info;

	event_list* show_update;
	event_list* time_update;
	event_list* count_update;
	event_list* focus_update;
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
    event_list* detail_showtimer;
    int         show_timeout;
#endif
} EpgCtl;

static int s_epg_max_prog_num = 0 ;

#if TMLIST_EPG_SUPPORT
static AppEpgOps   s_tm_epg_ops[MAX_EPG_PROG];
static EpgListData s_tm_epg_data[MAX_EPG_PROG];
static int         s_tm_cnt_bak[MAX_EPG_PROG];
static int         s_epg_prog_map[MAX_EPG_PROG];
#endif

#if LVLIST_EPG_SUPPORT
static AppEpgOps   s_lv_epg_ops;
static EpgListData s_lv_epg_data;
static int         s_lv_cnt_bak;
#endif

static EpgCtl s_epg_ctl = {
	.epg_style = TMLIST_EPG_STYLE,
	.update_flag = false,
	.detail_show = false,
	.day_sel = 0,
	.now_sel = -1,

	.zone_sec = 0,
	.local_sec = 0,
	.local_hour = 0,
	.local_day = 0,
	.next_event_sec = 0,
	.tmlist_start_sec = 0,
	.focus_time_bak = -1,

	.cur_prog = {0},

	.tmp_index = NULL,
	.tmp_info = NULL,
	.cur_info = NULL,
	.next_info = NULL,
	.focus_info = NULL,

	.show_update = NULL,
	.time_update = NULL,
	.count_update = NULL,
	.focus_update = NULL,
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
    .detail_showtimer = NULL,
    .show_timeout = 60
#endif
};

static char *s_epg_event_row[] =
{
	"timeitem_epg_evnt_list1",
	"timeitem_epg_evnt_list2",
	"timeitem_epg_evnt_list3",
	"timeitem_epg_evnt_list4",
	"timeitem_epg_evnt_list5",
	"timeitem_epg_evnt_list6",
	"timeitem_epg_evnt_list7",
	"timeitem_epg_evnt_list8",
	"timeitem_epg_evnt_list9",
	"timeitem_epg_evnt_list10",
};

/* -------- init and release   --------*/

int app_epg_timer_add(void);
int app_epg_timer_rm(void);

static int app_epg_ctl_release(void)
{
	EpgCtl *ctl = &s_epg_ctl;

	app_epg_timer_rm();
	memset(&ctl->cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));


    APP_FREE(ctl->use_index);
	APP_FREE(ctl->tmp_index);
	APP_FREE(ctl->tmp_info);
	APP_FREE(ctl->cur_info);
	APP_FREE(ctl->next_info);
	APP_FREE(ctl->focus_info);

	return 0;
}

static time_t _utc_time_get(void)
{
    GxTime time = {0};

    GxCore_GetLocalTime(&time);
    return time.seconds;
}

static int app_epg_ctl_init(void)
{
	EpgCtl *ctl = &s_epg_ctl;

#if (TMLIST_EPG_SUPPORT && LVLIST_EPG_SUPPORT)
	// default or last style
#elif TMLIST_EPG_SUPPORT
	ctl->epg_style = TMLIST_EPG_STYLE;
#else
	ctl->epg_style = LVLIST_EPG_STYLE;
#endif
	ctl->update_flag = false;
	ctl->detail_show = false;
	ctl->day_sel = 0;

	ctl->zone_sec = get_display_time_by_timezone(0);
	ctl->local_sec = _utc_time_get() + ctl->zone_sec;
	ctl->local_hour = ctl->local_sec - ctl->local_sec % SEC_PER_HOUR;
	ctl->local_day = ctl->local_hour - ctl->local_hour % SEC_PER_DAY;
	ctl->next_event_sec = 0;
	ctl->tmlist_start_sec = ctl->local_hour % SEC_PER_DAY;
	ctl->focus_time_bak = -1;

	app_epg_ctl_release();

	if(!(ctl->tmp_index = GxCore_Calloc(MAX_DISPLAY_EVENT, sizeof(EpgIndex))))
		goto err;
    if(!(ctl->use_index = GxCore_Calloc(MAX_DISPLAY_EVENT, sizeof(EpgIndex))))
        goto err;
	if(!(ctl->tmp_info = GxCore_Calloc(MAX_DISPLAY_EVENT, sizeof(GxEpgInfo) + EVENT_NAME_MAX_SIZE)))
		goto err;
	if(!(ctl->cur_info = GxCore_Calloc(1, EVENT_GET_NUM*EVENT_GET_MAX_SIZE)))
		goto err;
	if(!(ctl->next_info = GxCore_Calloc(1, EVENT_GET_NUM*EVENT_GET_MAX_SIZE)))
		goto err;
	if(!(ctl->focus_info = GxCore_Calloc(1, EVENT_GET_NUM*EVENT_GET_MAX_SIZE)))
		goto err;

	app_epg_timer_add();

	return 0;
err:
	app_epg_ctl_release();
	return -1;
}

static void app_epg_max_prog_init(void)
{
#if APP_MAIN_MENU_OBJ_SUPPORT
	if (APP_THEME_CLASSIC_DARK == 1)
		s_epg_max_prog_num = 5 ;
	else
		s_epg_max_prog_num = 6 ;
#else
#if(THEME_CLASSIC_DARK == 1)
	s_epg_max_prog_num = 5 ;
#else
	s_epg_max_prog_num = 6 ;
#endif
#endif
}

static int app_epg_ops_init(void)
{
#if TMLIST_EPG_SUPPORT
	int i;
	for(i = 0; i < s_epg_max_prog_num; i++)
	{
		epg_ops_init(&s_tm_epg_ops[i]);
	}
#endif
#if LVLIST_EPG_SUPPORT
	epg_ops_init(&s_lv_epg_ops);
#endif

	return 0;
}

static inline int app_epg_time_cmp(time_t a, time_t b)
{
	return (a - a % SEC_PER_MIN) - (b - b % SEC_PER_MIN);
}

static inline void app_epg_insert_event(int sel, int type, time_t start_time, time_t finish_time, const char* event_name, uint16_t event_id)
{
	EpgCtl *ctl = &s_epg_ctl;
	ctl->use_index[sel].type = type;
    ctl->use_index[sel].event_id = event_id;
	ctl->use_index[sel].start_time = start_time;
	ctl->use_index[sel].finish_time = finish_time;
	strncpy(ctl->use_index[sel].event_name, event_name, EVENT_NAME_MAX_SIZE);
}



#if TMLIST_EPG_SUPPORT || LVLIST_EPG_SUPPORT
static int app_epg_show_time_range_get(time_t *day_start, time_t *show_start, time_t *show_finish)
{
	EpgCtl *ctl = &s_epg_ctl;

	*day_start = ctl->local_day + ctl->day_sel * SEC_PER_DAY;
	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
		// 前面和后面多获取一个小时，timelist边缘左右移动更流畅
		*show_start = *day_start + ctl->tmlist_start_sec - SEC_PER_HOUR;
		*show_finish = *day_start + ctl->tmlist_start_sec + (DISPLAY_ZONE_HOUR) * SEC_PER_HOUR;
	}
	else
	{
		if(ctl->day_sel == 0)
			*show_start = ctl->local_sec;
		else
			*show_start = *day_start;
		*show_finish = *day_start + SEC_PER_DAY;
	}
	return 0;
}

static void app_epg_list_data_detail_release(EpgListData *data)
{
    int i = 0;

    if(NULL == data)
        return;

    if(data->detail)
    {
        for(i = 0; i < data->total; i++)
        {
            APP_FREE(data->detail[i]);
        }
    }
    return;
}

static void app_epg_list_data_release(EpgListData *data)
{
	if(data == NULL)
		return;

	if(data->detail != NULL)
	{
		int i = 0;
		for(i = 0; i < data->total; i++)
		{
			APP_FREE(data->detail[i]);
		}
		APP_FREE(data->detail);
	}

	APP_FREE(data->index);
	data->total = 0;
}

static int32_t app_epg_schedule_cnt_get(AppEpgOps *ops, time_t show_start, time_t show_finish, EpgInfoLevel level)
{
    int32_t schedule_cnt = 0, i = 0;
    time_t utc_start_time = 0;
    time_t utc_end_time = 0;
    EpgPeriodPara para = {0};
    EpgCtl *ctl = &s_epg_ctl;

    if(EPG_COMPLETE_INFO == level)
        return 0;

    utc_start_time = show_start - ctl->zone_sec;
    utc_end_time = show_finish - ctl->zone_sec;

    schedule_cnt = ops->get_schedule_event_cnt(ops, utc_start_time, utc_end_time);
    if(0 == schedule_cnt)
        return 0;

    if(schedule_cnt > MAX_DISPLAY_EVENT)
        schedule_cnt = MAX_DISPLAY_EVENT;

    para.want_get_cnt = schedule_cnt;
    para.start_time = utc_start_time;
    para.end_time = utc_end_time;
    para.level = level;

    if(GXCORE_ERROR == ops->get_schedule_event_info(ops, ctl->tmp_info, para))
        return 0;

    for(i = 0; i < schedule_cnt; i++)
    {
        ctl->tmp_index[i].type = EPG_DETAIL_EVENT;
        ctl->tmp_index[i].event_id = ctl->tmp_info[i].event_id;
        ctl->tmp_index[i].start_time = ctl->tmp_info[i].start_time + ctl->zone_sec;
        ctl->tmp_index[i].finish_time = ctl->tmp_index[i].start_time + ctl->tmp_info[i].duration;
        if(EPG_WITH_NAME_INFO == level)
            strncpy(ctl->tmp_index[i].event_name, (char*)ctl->tmp_info[i].event_name, EVENT_NAME_MAX_SIZE);
    }

    return schedule_cnt;
}

static int app_epg_cnt_get(AppEpgOps* ops, EpgInfoLevel level) //不可重入
{
	int i = 0, cnt = 0, tmp_cnt = 0;
	time_t day_start = 0, show_start = 0, show_finish = 0;
	EpgCtl *ctl = &s_epg_ctl;

	if(ctl->day_sel < 0 || ctl->day_sel >= MAX_EPG_DAY || ops == NULL)
		return 0;

	// 当前后续event
	ops->get_cur_event_info(ops, ctl->cur_info);
	ops->get_next_event_info(ops, ctl->next_info);

	// 显示时间范围
	if(app_epg_show_time_range_get(&day_start, &show_start, &show_finish) < 0)
		return 0;

	// 给定天的event
	memset(ctl->tmp_index, 0, MAX_DISPLAY_EVENT*sizeof(EpgIndex));
    cnt = app_epg_schedule_cnt_get(ops, show_start, show_finish, level);

	// 决定是否插入cur_event 和 next event
	int cur_flag = 0, next_flag = 0;  // 0 未处理，-1 无需处理，1 已经处理
	time_t cur_start, cur_finish, next_start, next_finish;

	cur_start = ctl->cur_info->start_time + ctl->zone_sec;
	cur_finish = cur_start + ctl->cur_info->duration;
	next_start = ctl->next_info->start_time + ctl->zone_sec;
	next_finish = next_start + ctl->next_info->duration;
	if(0 == ctl->cur_info->duration || app_epg_time_cmp(cur_start, show_finish) >= 0 || app_epg_time_cmp(cur_finish, show_start) < 0)
		cur_flag = -1;
	if(0 == ctl->next_info->duration || app_epg_time_cmp(next_start, show_finish) >= 0 || app_epg_time_cmp(next_finish, show_start) < 0)
		next_flag = -1;

#define INSERT_CUR_EVENT(sel) app_epg_insert_event(sel, EPG_CUR_EVENT, cur_start, cur_finish, (char*)ctl->cur_info->event_name, ctl->cur_info->event_id)
#define INSERT_NEXT_EVENT(sel) app_epg_insert_event(sel, EPG_NEXT_EVENT, next_start, next_finish, (char*)ctl->next_info->event_name, ctl->next_info->event_id)

	memset(ctl->use_index, 0, MAX_DISPLAY_EVENT*sizeof(EpgIndex));
	if(cnt == 0)
	{
		if(cur_flag == -1 && next_flag == -1)
		{
			return 0;
		}
		else if(next_flag == -1)
		{
			INSERT_CUR_EVENT(0);
			return 1;
		}
		else if(cur_flag == -1)
		{
			INSERT_NEXT_EVENT(0);
			return 1;
		}
		else
		{
			INSERT_CUR_EVENT(0);
			INSERT_NEXT_EVENT(1);
			return 2;
		}
	}
	else
	{
		if(cur_flag == -1 && next_flag == -1)
		{
			memcpy(ctl->use_index, ctl->tmp_index, cnt*sizeof(EpgIndex));
			return cnt;
		}
		else
		{
			tmp_cnt = cnt;
			cnt = 0;
			for(i = 0; i < tmp_cnt; i++)
			{
				if(cnt >= MAX_DISPLAY_EVENT)
					break;
				if(cur_flag != 0 && next_flag != 0)
				{
					int num = tmp_cnt - i;
					if(num + cnt >= MAX_DISPLAY_EVENT)
						num = MAX_DISPLAY_EVENT - cnt;
					memcpy(ctl->use_index + cnt, ctl->tmp_index + i, num*sizeof(EpgIndex));
					cnt += num;
					break;
				}
				if(cur_flag == 0)
				{
					int flag1 = 0, flag2 = 0;
					if(i > 0)
						flag1 = app_epg_time_cmp(cur_start, ctl->tmp_index[i-1].start_time);
					else
						flag1 = app_epg_time_cmp(cur_finish, show_start);
					flag2 = app_epg_time_cmp(cur_start, ctl->tmp_index[i].start_time);
					if(flag2 == 0)
						cur_flag = 1;
					else if(flag1 > 0 && flag2 < 0)
					{
						if(cnt >= MAX_DISPLAY_EVENT)
							break;
						cur_flag = 1;
						INSERT_CUR_EVENT(cnt);
						++cnt;
					}
				}
				if(next_flag == 0)
				{
					int flag1 = 0, flag2 = 0;
					if(i > 0)
						flag1 = app_epg_time_cmp(next_start, ctl->tmp_index[i-1].start_time);
					else
						flag1 = app_epg_time_cmp(next_finish, show_start);
					flag2 = app_epg_time_cmp(next_start, ctl->tmp_index[i].start_time);
					if(flag2 == 0)
						next_flag = 1;
					else if(flag1 > 0 && flag2 < 0)
					{
						if(cnt >= MAX_DISPLAY_EVENT)
							break;
						next_flag = 1;
						INSERT_NEXT_EVENT(cnt);
						++cnt;
					}
				}

				if(cnt >= MAX_DISPLAY_EVENT)
					break;
				memcpy(ctl->use_index + cnt, ctl->tmp_index + i, sizeof(EpgIndex));
				++cnt;
			}
			if(cur_flag == 0 && i == tmp_cnt && cnt < MAX_DISPLAY_EVENT)
			{
				int flag1 = 0;
				flag1 = app_epg_time_cmp(cur_start, ctl->tmp_index[tmp_cnt-1].start_time);
				if(flag1 > 0)
				{
					cur_flag = 1;
					INSERT_CUR_EVENT(cnt);
					++cnt;
				}
			}
			if(next_flag == 0 && i == tmp_cnt && cnt < MAX_DISPLAY_EVENT)
			{
				int flag1 = 0;
				flag1 = app_epg_time_cmp(next_start, ctl->tmp_index[tmp_cnt-1].start_time);
				if(flag1 > 0)
				{
					next_flag = 1;
					INSERT_NEXT_EVENT(cnt);
					++cnt;
				}
			}
			return cnt;
		}
	}
	return 0;
}

#if TMLIST_EPG_SUPPORT
static status_t app_epg_tmlist_data_init(EpgListData *data)
{
	if(data == NULL)
		return GXCORE_ERROR;

	data->detail = (char **)GxCore_Malloc(sizeof(char*));
	if(data->detail != NULL)
	{
		data->detail[0] = "<end>";
	}

	if((data->index = (EpgListIndex*)GxCore_Calloc(2*MAX_DISPLAY_EVENT, sizeof(EpgListIndex))) == NULL)
		goto err;
	if((data->detail = (char**)GxCore_Calloc(2*MAX_DISPLAY_EVENT+1, sizeof(char*))) == NULL)
		goto err;

	data->total = 0;
    return GXCORE_SUCCESS;
err:
	app_epg_list_data_release(data);
    return GXCORE_ERROR;
}
#endif

#if LVLIST_EPG_SUPPORT
static status_t app_epg_lvlist_data_init(EpgListData *data)
{
	if(data == NULL)
		return GXCORE_ERROR;

	if((data->index = (EpgListIndex*)GxCore_Calloc(MAX_DISPLAY_EVENT, sizeof(EpgListIndex))) == NULL)
		goto err;
	if((data->detail = (char**)GxCore_Calloc(MAX_DISPLAY_EVENT, sizeof(char*))) == NULL)
		goto err;

	data->total = 0;
    return GXCORE_SUCCESS;
err:
    app_epg_list_data_release(data);
    return GXCORE_ERROR;
}
#endif
#endif
/* -------- get epg event and get timelist/listview data   --------*/

static void app_epg_prog_info_update(void)
{
	int prog_sel = 0;
    uint32_t play_count = 0;
	EpgCtl *ctl = &s_epg_ctl;

	GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &prog_sel);
    play_count = g_AppChOps.real_pos(prog_sel);
	memset(&ctl->cur_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
	ctl->cur_prog.node_type = NODE_PROG;
	ctl->cur_prog.pos = play_count;
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ctl->cur_prog);

#if TMLIST_EPG_SUPPORT
	EpgProgInfo prog_info;
	GxMsgProperty_NodeByPosGet node;
	int i = 0, cur_sel = 0, first_sel = 0;

	GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
	first_sel = prog_sel - cur_sel;

	for(i = 0; i < s_epg_max_prog_num; i++)
	{
		if(first_sel + i < g_AppChOps.get_list_total())
		{
            prog_sel = first_sel + i;
            prog_sel = g_AppChOps.real_pos(prog_sel);
            s_epg_prog_map[i] = prog_sel;

			memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
			node.node_type = NODE_PROG;
			node.pos = s_epg_prog_map[i];
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
			{
				app_epg_prog_info_set(&prog_info, &node);
				s_tm_epg_ops[i].prog_info_update(&s_tm_epg_ops[i], &prog_info);
			}
		}
		else
		{
			s_epg_prog_map[i] = -1;
			memset(&s_tm_epg_ops[i].prog_info, 0, sizeof(EpgProgInfo));
		}
	}
#endif
#if LVLIST_EPG_SUPPORT
	EpgProgInfo prog_info;
	GxMsgProperty_NodeByPosGet node;
	memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
	node.node_type = NODE_PROG;
	node.pos = play_count;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	{
		app_epg_prog_info_set(&prog_info, &node);
		s_lv_epg_ops.prog_info_update(&s_lv_epg_ops, &prog_info);
	}
#endif
}


#define HOURMIN_STR_LEN     6
static void app_epg_hourmin_str_get(time_t sec, char *str)
{
	char *tmp_str = NULL;
	tmp_str = app_time_to_hourmin_edit_str(sec);
	if(tmp_str)
	{
		sprintf(str, "%s", tmp_str);
		GxCore_Free(tmp_str);
	}
}

#if TMLIST_EPG_SUPPORT
#define TMLIST_DAY_STR_LEN  3
static void app_epg_timlist_day_str_get(time_t sec, char *str)
{
	EpgCtl *ctl = &s_epg_ctl;
	time_t day_start = ctl->local_day + ctl->day_sel * SEC_PER_DAY;
	int day_sel = 0;

	if(sec >= day_start)
		day_sel = (sec - day_start) / SEC_PER_DAY;
	else
		day_sel = -1 - (day_start - 1 - sec) / SEC_PER_DAY;

	if(day_sel > 0)
		sprintf(str, "+%d", day_sel);
	else if(day_sel < 0)
		sprintf(str, "%d", day_sel);
}

static char* app_epg_tmlist_item_format(int vsel, time_t start_time, time_t finish_time, const char* event_name, uint16_t prog_id, uint16_t *flag)
{
// (<>len=2)*4 + (vsel_len=4) + (start_time_len=5) + (start_day_len=2) + (finish_time_len=5) + (finish_day_len=2) + ('\0'len=1) = 27
// TMLIST_BLANK_LEN + (<>len=2)*3 + (fore_len=7) + (back_len=7) + (font_len=9) = 56

#define TMLIST_BLANK_LEN  27
#define TMLIST_COLOR_LEN  56  //修改了font_str请修改这个值

#define TMLIST_BLANK_STR  "<%d><%s%s><%s%s><>"
#define TMLIST_EXIST_STR  "<%d><%s%s><%s%s><%s>"
#define TMLIST_COLOR_STR  "<%d><%s%s><%s%s><%s><%s><%s><%s>"

	EpgCtl *ctl = &s_epg_ctl;
	char *eve_str = NULL;
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};
	char start_day_str[TMLIST_DAY_STR_LEN] = {0};
	char finish_day_str[TMLIST_DAY_STR_LEN] = {0};

	app_epg_hourmin_str_get(start_time, start_time_str);
	app_epg_hourmin_str_get(finish_time, finish_time_str);
	app_epg_timlist_day_str_get(start_time, start_day_str);
	app_epg_timlist_day_str_get(finish_time, finish_day_str);

	if(event_name == NULL)
	{
		if((prog_id == ctl->cur_prog.prog_data.id)
				&& (ctl->local_sec >= start_time && ctl->local_sec < finish_time)) //current event
			ctl->next_event_sec = finish_time;
		if((eve_str = (char*)GxCore_Mallocz(TMLIST_BLANK_LEN)) == NULL)
			return NULL;
		sprintf(eve_str, TMLIST_BLANK_STR, vsel, start_time_str, start_day_str, finish_time_str, finish_day_str);
		return eve_str;
	}

	const char* name_str = (strlen(event_name)) ? (event_name) : ("No Name");
	const char* fore_str = NULL;
	const char* back_str = NULL;
	const char* font_str = "font_prog"; //reference font.xml

	if((prog_id == ctl->cur_prog.prog_data.id)
			&& (ctl->local_sec >= start_time && ctl->local_sec < finish_time)) //current event
	{
		ctl->next_event_sec = finish_time;
		fore_str = "#f4f4f4";
		back_str = "#005a99";
		*flag |= CUR_EPG_FLAG;
	}
	else if(ctl->local_sec >= start_time)
    {
        //不做处理
    }
    else
    {
		GxBook prog_book;
		memset(&prog_book, 0, sizeof(GxBook));
		if(app_get_same_start_play_or_pvr_book(prog_id, start_time - ctl->zone_sec, &prog_book) && (start_time >= ctl->local_sec))
		{
			fore_str= "#f4f4f4";
			if(prog_book.book_type == BOOK_PROGRAM_PLAY)
			{
				back_str = "#008000";
				*flag |= PLAY_EPG_FLAG;
			}
			else if(prog_book.book_type == BOOK_PROGRAM_PVR)
			{
				back_str = "#E6A200";
				*flag |= PVR_EPG_FLAG;
			}
		}
	}

	if(back_str == NULL)
	{
		if((eve_str = (char*)GxCore_Mallocz(TMLIST_BLANK_LEN + strlen(name_str))) == NULL)
			return NULL;
		sprintf(eve_str, TMLIST_EXIST_STR, vsel, start_time_str, start_day_str, finish_time_str, finish_day_str,
				name_str);
	}
	else
	{
		if((eve_str = (char*)GxCore_Mallocz(TMLIST_COLOR_LEN + strlen(name_str))) == NULL)
			return NULL;
		sprintf(eve_str, TMLIST_COLOR_STR, vsel, start_time_str, start_day_str, finish_time_str, finish_day_str,
				name_str, fore_str, back_str, font_str);
	}

	return eve_str;
}

static inline void app_epg_tmlist_insert_event(EpgListData *data, int *cnt, char* eve_str, uint16_t event_flag,
                                                uint16_t type, time_t start_time, time_t end_time, uint16_t event_id)
{
	int tmp_cnt = *cnt;
	data->detail[tmp_cnt] = eve_str;
	data->index[tmp_cnt].vsel = tmp_cnt;
	data->index[tmp_cnt].flag = event_flag;
	data->index[tmp_cnt].type = type;
    data->index[tmp_cnt].event_id = event_id;
    data->index[tmp_cnt].start_time = start_time;
    data->index[tmp_cnt].end_time = end_time;
	++(*cnt);
}

static int app_epg_tmlist_data_get(int sel)
{
	if(sel < 0 || sel >= s_epg_max_prog_num)
		return GXCORE_ERROR;

	int i = 0, cnt = 0, tmp_cnt = 0;
	time_t day_start = 0, show_start = 0, show_finish = 0;
	EpgCtl *ctl = &s_epg_ctl;
	AppEpgOps *ops = &s_tm_epg_ops[sel];
	EpgListData *data = &s_tm_epg_data[sel];

	app_epg_list_data_detail_release(data);
	if(s_epg_prog_map[sel] == -1)
		goto err;
	if((cnt = app_epg_cnt_get(ops, EPG_WITH_NAME_INFO)) == 0)
		goto err;

	if(app_epg_show_time_range_get(&day_start, &show_start, &show_finish) < 0)
		goto err;

    s_tm_cnt_bak[sel] = cnt;

	GxMsgProperty_NodeByPosGet node = {0};
	node.node_type = NODE_PROG;
	node.pos = s_epg_prog_map[sel];
	app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
	uint16_t prog_id = node.prog_data.id;
    time_t event_start_time = 0, event_finish_time = 0;

#define INSERT_TMLIST_EVENT(type, start_time, end_time, event_id) app_epg_tmlist_insert_event(data, &tmp_cnt, eve_str, event_flag, type, start_time, end_time, event_id)

	if(prog_id == ctl->cur_prog.prog_data.id)
		ctl->next_event_sec = 0;
	for(i = 0; i < cnt; i++)
	{
		if(i == 0)
		{
			uint16_t event_flag = 0;
			int flag = 0;
			flag = app_epg_time_cmp(show_start, ctl->use_index[i].start_time);
			if(flag < 0)
			{
				char* eve_str = NULL;
				eve_str = app_epg_tmlist_item_format(tmp_cnt, show_start, ctl->use_index[i].start_time, NULL, prog_id, &event_flag);
				if(eve_str)
				{
					INSERT_TMLIST_EVENT(EPG_INVALID_EVENT, 0, 0, 0);
				}
			}
		}
		else
		{
			uint16_t event_flag = 0;
			int flag = 0;
			flag = app_epg_time_cmp(ctl->use_index[i-1].finish_time, ctl->use_index[i].start_time);
			if(flag < 0)
			{
				char* eve_str = NULL;
				eve_str = app_epg_tmlist_item_format(tmp_cnt, ctl->use_index[i-1].finish_time, ctl->use_index[i].start_time, NULL, prog_id, &event_flag);
				if(eve_str)
				{
					INSERT_TMLIST_EVENT(EPG_INVALID_EVENT, 0, 0, 0);
				}
			}
		}

		uint16_t event_flag = 0;
		char* eve_str = NULL;

        event_start_time = ctl->use_index[i].start_time;
        event_finish_time = ctl->use_index[i].finish_time;

        if((i<(cnt-1)) && (ctl->use_index[i].finish_time > ctl->use_index[i+1].start_time))
            event_finish_time = ctl->use_index[i+1].start_time;

		if(event_start_time == event_finish_time)
			continue;

		eve_str = app_epg_tmlist_item_format(tmp_cnt, event_start_time, event_finish_time, ctl->use_index[i].event_name, prog_id, &event_flag);
		if(eve_str)
		{
			INSERT_TMLIST_EVENT(ctl->use_index[i].type, ctl->use_index[i].start_time, ctl->use_index[i].finish_time, ctl->use_index[i].event_id);
		}
	}

	data->detail[tmp_cnt] = "<end>";
	data->total = tmp_cnt;
	return GXCORE_SUCCESS;
err:
    s_tm_cnt_bak[sel] = 0;
    app_epg_list_data_detail_release(data);
    memset(data->index, 0, 2 * MAX_DISPLAY_EVENT * sizeof(EpgListIndex));
    memset(data->detail, 0, (2 * MAX_DISPLAY_EVENT + 1) * sizeof(char *));
    data->detail[0] = "<end>";
    data->total = 0;
	return GXCORE_ERROR;
}
#endif

#if LVLIST_EPG_SUPPORT
static char* app_epg_lvlist_item_format(int vsel, time_t start_time, time_t finish_time, const char* event_name, uint16_t *flag)
{
	// (start_time_len=5) + 1 + (finish_time_len=5) + 2 + 1 = 14

#define LVLIST_BLANK_LEN  14
#define LVLIST_BLANK_STR  "%s-%s  %s"

	char *eve_str = NULL;
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};
	const char* name_str = (strlen(event_name)) ? (event_name) : ("No Name");

	app_epg_hourmin_str_get(start_time, start_time_str);
	app_epg_hourmin_str_get(finish_time, finish_time_str);

	EpgCtl *ctl = &s_epg_ctl;
	if(ctl->local_sec >= start_time && ctl->local_sec < finish_time) //current event
	{
		ctl->now_sel = vsel;
		ctl->next_event_sec = finish_time;
		*flag |= CUR_EPG_FLAG;
	}
	else
	{
		GxBook prog_book;
		memset(&prog_book, 0, sizeof(GxBook));
		if(app_get_same_start_play_or_pvr_book(ctl->cur_prog.prog_data.id, start_time - ctl->zone_sec, &prog_book) && (start_time >= ctl->local_sec))
		{
			if (prog_book.book_type == BOOK_PROGRAM_PLAY)
			{
				*flag |= PLAY_EPG_FLAG;
			}
			else if (prog_book.book_type == BOOK_PROGRAM_PVR)
			{
				*flag |= PVR_EPG_FLAG;
			}
		}
	}

	if((eve_str = (char*)GxCore_Mallocz(LVLIST_BLANK_LEN + strlen(name_str))) == NULL)
		return NULL;
	sprintf(eve_str, LVLIST_BLANK_STR, start_time_str, finish_time_str, name_str);

	return eve_str;
}

static int app_epg_lvlist_data_get(void)
{
	int i = 0, cnt = 0, tmp_cnt = 0;
	time_t day_start = 0, show_start = 0, show_finish = 0;
	EpgCtl *ctl = &s_epg_ctl;
	AppEpgOps *ops = &s_lv_epg_ops;
	EpgListData *data = &s_lv_epg_data;

	ctl->next_event_sec = 0;
	ctl->now_sel = -1;
	app_epg_list_data_detail_release(data);
	if((cnt = app_epg_cnt_get(ops, EPG_WITH_NAME_INFO)) == 0)
		goto err;
	if(app_epg_show_time_range_get(&day_start, &show_start, &show_finish) < 0)
		goto err;

	s_lv_cnt_bak = cnt;
	for(i = 0; i < cnt; i++)
	{
		char* eve_str = NULL;
		uint16_t event_flag = 0;
		eve_str = app_epg_lvlist_item_format(i, ctl->use_index[i].start_time, ctl->use_index[i].finish_time, (char*)ctl->use_index[i].event_name, &event_flag);
		if(eve_str)
		{
			data->detail[tmp_cnt] = eve_str;
			data->index[tmp_cnt].vsel = i;
			data->index[tmp_cnt].flag = event_flag;
			data->index[tmp_cnt].type = ctl->use_index[i].type;
            data->index[tmp_cnt].event_id = ctl->use_index[i].event_id;
			data->index[tmp_cnt].start_time = ctl->use_index[i].start_time;
            data->index[tmp_cnt].end_time = ctl->use_index[i].finish_time;
			++tmp_cnt;
		}
	}

	data->total = tmp_cnt;
	return GXCORE_SUCCESS;
err:
	s_lv_cnt_bak = 0;
    app_epg_list_data_detail_release(data);
    memset(data->index, 0, MAX_DISPLAY_EVENT * sizeof(EpgListIndex));
    memset(data->detail, 0, MAX_DISPLAY_EVENT * sizeof(char *));
    data->total = 0;
	return GXCORE_ERROR;
}
#endif

static int app_epg_focus_event_get(void)
{
	int ret = GXCORE_ERROR;
	int sel = 0;
	EpgCtl *ctl = &s_epg_ctl;
	AppEpgOps *ops = NULL;
	EpgListData *data = NULL;

	memset(ctl->focus_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
#if TMLIST_EPG_SUPPORT
	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
        int cur_sel = 0;

        GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
		ops = &s_tm_epg_ops[cur_sel];
		data = &s_tm_epg_data[cur_sel];
		if(0 == data->total || NULL == data->detail)
			return ret;

        //need to force update of the widget data in the timer function,
        //otherwise the value of select_column maybe incorrect
        GUI_SetProperty(TIMELIST_EPG_EVENT, "draw_now", NULL);
		GUI_GetProperty(TIMELIST_EPG_EVENT, "select_column", &sel);
	}
#endif
#if LVLIST_EPG_SUPPORT
	if(ctl->epg_style == LVLIST_EPG_STYLE)
	{
		ops = &s_lv_epg_ops;
		data = &s_lv_epg_data;
		if(0 == data->total || NULL == data->detail)
			return ret;

		GUI_GetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
	}
#endif

	if(sel >= 0 && data && sel < data->total)
	{
		uint16_t type = data->index[sel].type;
        uint16_t event_id = data->index[sel].event_id;

		if(EPG_INVALID_EVENT != type)
			ret = ops->get_event_info_by_event_id(ops, ctl->focus_info, event_id);
	}

	return ret;
}

/*-------- GUI event detail --------*/
#if 0 //LVLIST_EPG_SUPPORT
static char* app_epg_get_event_weekday(time_t sec)
{
	struct tm _tm = {0};
	localtime_r(&sec, &_tm);

	switch(_tm.tm_wday)
	{
		case 0: return "Sun.";
		case 1: return "Mon.";
		case 2: return "Tue.";
		case 3: return "Wed.";
		case 4: return "Thu.";
		case 5: return "Fri.";
		case 6: return "Sat.";
		default: return NULL;
	}
}
#endif

static void app_epg_show_sys_time_data(void)
{
#define TEXT_EPG_SYS_TIME               "text_epg_sys_time"
#define TEXT_EPG_SYS_DATE               "text_epg_sys_ymd"

    char sys_time_str[24] = {0};
    char time_str_temp[24] = {0};

    g_AppTime.time_get(&g_AppTime, sys_time_str, sizeof(sys_time_str));
    if((strlen(sys_time_str) <= 24) && (strlen(sys_time_str) >= 5))
    {
        memcpy(time_str_temp, &sys_time_str[strlen(sys_time_str)-5], 5); // 00:00
        GUI_SetProperty(TEXT_EPG_SYS_TIME, "string", time_str_temp);

        memcpy(time_str_temp, sys_time_str, strlen(sys_time_str)); // 0000/00/00
        time_str_temp[strlen(sys_time_str)-5] = '\0';
        GUI_SetProperty(TEXT_EPG_SYS_DATE, "string", time_str_temp);
    }
}

static void app_epg_set_event_date(time_t sec)
{
	char* tm_str = NULL;

	tm_str = app_time_to_date_edit_str(sec);
	if(tm_str != NULL)
	{
		GUI_SetProperty(TEXT_EGP_DATE, "string", tm_str);
		GxCore_Free(tm_str);
		tm_str = NULL;
	}
}

static void app_epg_show_prog_name(void)
{
	char prog_name[MAX_PROG_NAME + 1] = {0};
	char *old_name = NULL;

	memcpy(prog_name, s_epg_ctl.cur_prog.prog_data.prog_name, MAX_PROG_NAME);
	GUI_GetProperty(TEXT_EPG_PROG_NAME, "string", &old_name);
	if ((old_name == NULL) || (strcmp(old_name, prog_name) != 0))
	{
		GUI_SetProperty(TEXT_EPG_PROG_NAME,"string", prog_name);
	}
}

static int app_epg_set_tmlist_start_time(const char* str, time_t sec)
{
	if(str)
	{
		GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", (void*)str);
		return 0;
	}

	char* tm_str = NULL;
	tm_str = app_time_to_hourmin_edit_str(sec);
	if(tm_str != NULL)
	{
		char tm_set_str[8] = {0};

		tm_set_str[0] = '<';
		memcpy(tm_set_str + 1, tm_str, 5);
		tm_set_str[6] = '>';
		tm_set_str[7] = '\0';
		GxCore_Free(tm_str);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "start_time", (void*)tm_set_str);

		return 0;
	}
	return -1;
}

static void app_epg_tips_hide(void)
{
	GUI_SetProperty(IMG_EPG_BOOK, "state", "hide");
	GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "hide");
	GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "hide");
	GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "hide");
	GUI_SetProperty(TEXT_EPG_BOOK, "state", "hide");
	GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "hide");
	GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "hide");
	GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "hide");

	GUI_SetProperty(IMG_EPG_MODE, "state", "hide");
	GUI_SetProperty(TEXT_EPG_MODE, "state", "hide");
}

static void app_epg_tips_show(void)
{
	GUI_SetProperty(IMG_EPG_BOOK, "state", "show");
	GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "show");
	GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "show");
	GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "show");
	GUI_SetProperty(TEXT_EPG_BOOK, "state", "show");
	GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "show");
	GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "show");
	GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "show");
#if TMLIST_EPG_SUPPORT && LVLIST_EPG_SUPPORT
	GUI_SetProperty(IMG_EPG_MODE, "state", "show");
	GUI_SetProperty(TEXT_EPG_MODE, "state", "show");
#endif
}


static void app_epg_brief_info_clear(void)
{
	const char *wnd_epg = NULL;
	wnd_epg = GUI_GetFocusWindow();
	if(0 == strcmp(wnd_epg,WND_EPG))
	{
		app_epg_show_prog_name();
		GUI_SetProperty(TEXT_EPG_EVENT_NAME, "string", STR_ID_BLANK);
		GUI_SetProperty(TEXT_EPG_START_TIME, "string", STR_ID_BLANK);
		GUI_SetProperty(TEXT_EPG_TIME_INTERVAL, "string", STR_ID_BLANK);
		GUI_SetProperty(TEXT_EPG_END_TIME, "string", STR_ID_BLANK);
	}
}

static void app_epg_brief_info_show(void)
{
	EpgCtl *ctl = &s_epg_ctl;

	time_t start_time = 0, finish_time = 0;
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};

	start_time = ctl->focus_info->start_time + ctl->zone_sec;
	finish_time = start_time + ctl->focus_info->duration;
	app_epg_hourmin_str_get(start_time, start_time_str);
	app_epg_hourmin_str_get(finish_time, finish_time_str);

    app_epg_show_prog_name();
    if(finish_time > (ctl->local_day + ctl->tmlist_start_sec))
    {
        GUI_SetProperty(TEXT_EPG_EVENT_NAME, "string", ctl->focus_info->event_name);
        GUI_SetProperty(TEXT_EPG_START_TIME, "string", start_time_str);
        GUI_SetProperty(TEXT_EPG_TIME_INTERVAL, "string", "--");
        GUI_SetProperty(TEXT_EPG_END_TIME, "string", finish_time_str);
    }
}

#if EPG_REALTIMER_GET_DETAIL_SUPPORT
static int app_epg_detail_update_exec(void* data)
{
    char* i18n_str = NULL;
    EpgCtl *ctl = &s_epg_ctl;
    uint16_t event_id = ctl->focus_info->event_id;
	AppEpgOps *ops = NULL;
#if TMLIST_EPG_SUPPORT
	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
        int cur_sel = 0;
        GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
		ops = &s_tm_epg_ops[cur_sel];
	}
#endif
#if LVLIST_EPG_SUPPORT
	if(ctl->epg_style == LVLIST_EPG_STYLE)
	{
		ops = &s_lv_epg_ops;
	}
#endif
	memset(ctl->focus_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
    if(ops->get_event_info_by_event_id(ops, ctl->focus_info, event_id) == GXCORE_SUCCESS)
    {
        uint8_t *detail = ctl->focus_info->event_detail;
        size_t len = strlen((char *)detail);
        if(0 == len)
        {
            if(ctl->show_timeout == 0)
                GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", STR_ID_NO_INFO);
            else
                ctl->show_timeout--;
            return 0;
        }
        uint8_t *p = detail + 2;
        APP_TIMER_REMOVE(ctl->detail_showtimer);
        if(detail[len - 1] == '\n')
        {
            detail[len - 1] = '\0';
        }
        if(*p =='\n')
        {
            *(--p) = 0x20;
            GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", p);
        }
        else
        {
            i18n_str =(char*)GXGDI_I18N_String((const char *)detail);
            GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", i18n_str);
        }
        app_epg_detail_filter_clean();
    }
    return 0;
}
#endif

static void app_epg_event_details_hide(void)
{
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
	EpgCtl *ctl = &s_epg_ctl;
	if(ctl->detail_showtimer != NULL)
	{
		APP_TIMER_REMOVE(ctl->detail_showtimer);
		ctl->show_timeout=60;
		app_epg_detail_filter_clean();
	}
#endif
	s_epg_ctl.detail_show = false;
	app_epg_tips_show();
	GUI_SetProperty(TEXT_EVENT_TITLE, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_TITLE, "state", "hide");
	GUI_SetProperty(IMG_EPG_EVENT_DETAILS_BACK, "state", "hide");
    GUI_SetProperty(IMG_EPG_EVENT_DETAILS_LINE5, "state", "hide");
    GUI_SetProperty(IMG_EPG_EVENT_DETAILS_LINE6, "state", "hide");
	GUI_SetProperty(NOTEPAD_EPG_EVENT, "state", "hide");
	GUI_SetProperty(IMG_EPG_DETAILS_BACK, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_START_TIME, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_START_TIME, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_TIME_INTERVAL, "state", "hide");
	GUI_SetProperty(TEXT_EVENT_END_TIME, "string", NULL);
	GUI_SetProperty(TEXT_EVENT_END_TIME, "state", "hide");
	GUI_SetProperty(BTN_EVNET_DETAILS_EXIT, "state", "hide");

	if(s_epg_ctl.epg_style == TMLIST_EPG_STYLE)
		GUI_SetFocusWidget(TIMELIST_EPG_EVENT);
	else
		GUI_SetFocusWidget(LISTVIEW_EPG_EVENT);
}

static void app_epg_event_details_show(void)
{
	EpgCtl *ctl = &s_epg_ctl;
	s_epg_ctl.detail_show = true;
	app_epg_tips_hide();
	GUI_SetProperty(IMG_EPG_EVENT_DETAILS_BACK, "state", "show");
    GUI_SetProperty(IMG_EPG_EVENT_DETAILS_LINE5, "state", "show");
    GUI_SetProperty(IMG_EPG_EVENT_DETAILS_LINE6, "state", "show");
	GUI_SetProperty(NOTEPAD_EPG_EVENT, "state", "show");
	GUI_SetProperty(IMG_EPG_DETAILS_BACK, "state", "show");
	GUI_SetProperty(TEXT_EVENT_TITLE, "state", "show");
	GUI_SetProperty(TEXT_EVENT_TIME_INTERVAL, "state", "show");
	GUI_SetProperty(TEXT_EVENT_START_TIME, "state", "show");
	GUI_SetProperty(TEXT_EVENT_END_TIME, "state", "show");
	GUI_SetProperty(BTN_EVNET_DETAILS_EXIT, "state", "show");

#if EPG_REALTIMER_GET_DETAIL_SUPPORT
	app_epg_detail_filter_set(ctl->cur_prog.prog_data.ts_id, ctl->cur_prog.prog_data.service_id, ctl->cur_prog.prog_data.original_id,ctl->focus_info->event_id);
	GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", STR_ID_WAITING);
	ctl->show_timeout=60;
	APP_TIMER_ADD(ctl->detail_showtimer, app_epg_detail_update_exec, 1000, TIMER_REPEAT);
#else
	char* i18n_str = NULL;
	uint8_t *detail = ctl->focus_info->event_detail;
	uint8_t *p = detail + 2;
	size_t len = strlen((char *)detail);

    if(0 == len)
        return;

	if(detail[len - 1] == '\n')
	{
		detail[len - 1] = '\0';
	}
	if(*p =='\n')
	{
		*(--p) = 0x20;
		GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", p);
	}
	else
	{
		i18n_str =(char*)GXGDI_I18N_String((const char *)detail);
		GUI_SetProperty(NOTEPAD_EPG_EVENT, "string", i18n_str);
	}
#endif


	time_t start_time = 0, finish_time = 0;
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};

	start_time = ctl->focus_info->start_time + ctl->zone_sec;
	finish_time = start_time + ctl->focus_info->duration;
	app_epg_hourmin_str_get(start_time, start_time_str);
	app_epg_hourmin_str_get(finish_time, finish_time_str);

	GUI_SetProperty(TEXT_EVENT_TITLE, "string", ctl->focus_info->event_name);
	GUI_SetProperty(TEXT_EVENT_START_TIME, "string", start_time_str);
	GUI_SetProperty(TEXT_EVENT_END_TIME, "string", finish_time_str);

	GUI_SetFocusWidget(BTN_EVNET_DETAILS_EXIT);
}

static void app_epg_details_hide(void)
{
    uint32_t sel = 0;
	app_epg_event_details_hide();
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
	GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
	GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);
	GUI_SetProperty(TEXT_EPG_PROG_NAME, "reset_rolling", NULL);
	GUI_SetProperty(TEXT_EPG_EVENT_NAME, "reset_rolling", NULL);
}

static void app_epg_details_show(void)
{
	if(app_epg_focus_event_get() == GXCORE_SUCCESS)
	{
		GUI_SetProperty(TEXT_EPG_PROG_NAME, "rolling_stop", NULL);
		GUI_SetProperty(TEXT_EPG_EVENT_NAME, "rolling_stop", NULL);
		app_epg_event_details_show();
	}
}

static void app_epg_event_details_keypress(uint16_t key)
{
	uint32_t value = 1;

	switch(key)
	{
		case STBK_UP:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "line_up", &value);
			break;
		case STBK_DOWN:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "line_down", &value);
			break;
		case STBK_PAGE_UP:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "page_up", &value);
			break;
		case STBK_PAGE_DOWN:
			GUI_SetProperty(NOTEPAD_EPG_EVENT, "page_down", &value);
			break;
		default:
			break;
	}
}

/* -------- timer exec --------*/

static int app_epg_show_update_exec(void* usrdata)
{
	EpgCtl *ctl = &s_epg_ctl;
	if(ctl->update_flag && !(ctl->detail_show))
	{
		const char *wnd = NULL;
		wnd = GUI_GetFocusWindow();
		if(wnd != NULL && (0 == strcmp(wnd,WND_EPG)))
		{
			ctl->update_flag = false;
			app_epg_set_event_date(ctl->local_day + (ctl->day_sel * SEC_PER_DAY));
#if TMLIST_EPG_SUPPORT
			if(ctl->epg_style == TMLIST_EPG_STYLE)
			{
				int i = 0;
				for(i = 0; i < s_epg_max_prog_num; i++)
				{
					app_epg_tmlist_data_get(i);
				}

				if(ctl->day_sel == 0)
					app_epg_set_tmlist_start_time(NULL, ctl->tmlist_start_sec);

				GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
			}
#endif
#if LVLIST_EPG_SUPPORT
			if(ctl->epg_style == LVLIST_EPG_STYLE)
			{
				app_epg_lvlist_data_get();

				GUI_SetProperty(LISTVIEW_EPG_EVENT, "update_all", NULL);
				GUI_SetProperty(LISTVIEW_EPG_EVENT, "active", &ctl->now_sel);
			}
#endif
		}

	}
	return 0;
}

static int app_epg_time_update_exec(void* usrdata)
{
	EpgCtl *ctl = &s_epg_ctl;

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG) && GXCORE_ERROR == GUI_CheckDialog(WND_SYSTEM_SETTING))
        app_epg_show_sys_time_data();

	time_t local_sec = _utc_time_get() + ctl->zone_sec;
	time_t local_hour = local_sec - local_sec % SEC_PER_HOUR;
	time_t local_day = local_hour - local_hour % SEC_PER_DAY;

	if(local_day != ctl->local_day)
	{
		ctl->update_flag = true;
		if(ctl->day_sel != 0)
			ctl->day_sel--;
        else
            ctl->tmlist_start_sec = local_hour % SEC_PER_DAY;
	}
	else if(local_hour != ctl->local_hour
			|| (local_sec >= ctl->next_event_sec && 0 < ctl->next_event_sec))
	{
		ctl->update_flag = true;
	}

	if(0 == ctl->day_sel)
	{
		if((local_sec >= ctl->next_event_sec && 0 < ctl->next_event_sec)
				|| (local_hour != ctl->local_hour && 0 < ctl->next_event_sec))
			ctl->tmlist_start_sec = local_hour % SEC_PER_DAY;
	}

	ctl->local_sec = local_sec;
	ctl->local_hour = local_hour;
	ctl->local_day = local_day;

	return 0;
}

static int app_epg_count_update_exec(void* usrdata)
{

	EpgCtl *ctl = &s_epg_ctl;
	if(!(ctl->update_flag) && !(ctl->detail_show))
	{
		const char *wnd = NULL;
		wnd = GUI_GetFocusWindow();
		if(wnd != NULL && (0 == strcmp(wnd,WND_EPG)))
		{
#if TMLIST_EPG_SUPPORT
			if(ctl->epg_style == TMLIST_EPG_STYLE)
			{
				int i = 0;
				for(i = 0; i < s_epg_max_prog_num; i++)
				{
					if(s_tm_cnt_bak[i] != app_epg_cnt_get(&s_tm_epg_ops[i], EPG_BASIC_INFO))
					{
						ctl->update_flag = true;
						return 0;
					}
				}
			}
#endif
#if LVLIST_EPG_SUPPORT
			if(ctl->epg_style == LVLIST_EPG_STYLE)
			{
				if(s_lv_cnt_bak != app_epg_cnt_get(&s_lv_epg_ops, EPG_BASIC_INFO))
				{
					ctl->update_flag = true;
					return 0;
				}
			}
#endif
		}
	}

	return 0;
}

static int app_epg_focus_update_exec(void* usrdata)
{
	EpgCtl *ctl = &s_epg_ctl;

	if(!(ctl->update_flag) && !(ctl->detail_show))
	{
		const char *wnd = NULL;
		wnd = GUI_GetFocusWindow();
		if(wnd != NULL && (0 == strcmp(wnd,WND_EPG)))
		{
			app_epg_focus_event_get();
			if(ctl->focus_time_bak != ctl->focus_info->start_time)
			{
				ctl->focus_time_bak = ctl->focus_info->start_time;
				if(ctl->focus_time_bak == 0)
				{
					app_epg_brief_info_clear();
					GUI_SetProperty(TEXT_EPG_EVENT_NAME, "string", STR_ID_NO_INFO);
				}
				else
				{
					app_epg_brief_info_show();
				}
			}

			if(ctl->focus_info->start_time == 0)
			{
				GUI_SetProperty(IMG_EPG_BOOK, "state", "hide");
				GUI_SetProperty(TEXT_EPG_BOOK, "state", "hide");
                GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "hide");
                GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "hide");
			}
			else
			{

                if(LVLIST_EPG_STYLE == ctl->epg_style)
                {
                    const char *widget = GUI_GetFocusWidget();

                    if(widget && 0 == strcmp(LISTVIEW_EPG_EVENT, widget))
                    {
                        GUI_SetProperty(IMG_EPG_BOOK, "state", "show");
                        GUI_SetProperty(TEXT_EPG_BOOK, "state", "show");
                    }
                    GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "show");
                    GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "show");
                }
                else if(TMLIST_EPG_STYLE == ctl->epg_style)
                {
                    GUI_SetProperty(IMG_EPG_BOOK, "state", "show");
                    GUI_SetProperty(TEXT_EPG_BOOK, "state", "show");
                    GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "show");
                    GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "show");
                }

				char *str = NULL;
				GxBook prog_book;

				memset(&prog_book, 0, sizeof(GxBook));
				GUI_GetProperty(TEXT_EPG_BOOK, "string", &str);
				if(app_get_same_start_play_or_pvr_book(ctl->cur_prog.prog_data.id, ctl->focus_info->start_time, &prog_book)
                        && ((ctl->focus_info->start_time+ctl->zone_sec) >= ctl->local_sec))
				{
					if(str == NULL || strcmp(str, STR_ID_DELETE) != 0)
						GUI_SetProperty(TEXT_EPG_BOOK, "string", STR_ID_DELETE);
				}
				else
				{
					if(str == NULL || strcmp(str, STR_ID_BOOK) != 0)
						GUI_SetProperty(TEXT_EPG_BOOK, "string", STR_ID_BOOK);
				}
			}
		}
	}

	return 0;
}

int app_epg_timer_add(void)
{
	EpgCtl *ctl = &s_epg_ctl;
	APP_TIMER_ADD(ctl->show_update, app_epg_show_update_exec, 30, TIMER_REPEAT);
	APP_TIMER_ADD(ctl->time_update, app_epg_time_update_exec, 800, TIMER_REPEAT);
	APP_TIMER_ADD(ctl->count_update, app_epg_count_update_exec, 1000, TIMER_REPEAT);
	APP_TIMER_ADD(ctl->focus_update, app_epg_focus_update_exec, 500, TIMER_REPEAT);
	return 0;
}

int app_epg_timer_rm(void)
{
	EpgCtl *ctl = &s_epg_ctl;
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
    APP_TIMER_REMOVE(ctl->detail_showtimer);
#endif
	APP_TIMER_REMOVE(ctl->show_update);
	APP_TIMER_REMOVE(ctl->time_update);
	APP_TIMER_REMOVE(ctl->count_update);
	APP_TIMER_REMOVE(ctl->focus_update);
	return 0;
}

int app_epg_show_update_immediately(void)
{
	EpgCtl *ctl = &s_epg_ctl;
	ctl->update_flag = true;
	app_epg_show_update_exec(NULL);
	return 0;
}

/* -------- play prog --------*/

static void epg_small_play_wnd_rect_get(PlayerWindow *video_wnd)
{
#define VIDEO_TEXT                      "txt_epg_for_pp"

#if CVBS_SCALE_SUPPORT
#define CVBS_OFFSET_X                   12
#define CVBS_OFFSET_W                   16
	extern int app_top_get_cvbs_mode_status(void);
#endif

	int x = 0, y = 0, w = 0, h = 0;

	if(!video_wnd)
		return;

	sscanf((widget_get_property(gui_get_widget(NULL, VIDEO_TEXT), "rect")), "[%d,%d,%d,%d]",&x, &y, &w, &h);
#if CVBS_SCALE_SUPPORT
	if((app_top_get_cvbs_mode_status()) && (w > CVBS_OFFSET_W) && (x > CVBS_OFFSET_X))
	{
		x = x - CVBS_OFFSET_X;
		w = w - CVBS_OFFSET_W;
	}
#endif
#if (UI_MIRROR_SUPPORT >0)
int app_menu_mirror_get(void);
    if(app_menu_mirror_get() == 1)
    {
        x= APP_THEME_XRES -(x+w);
    }
#endif

	video_wnd->x = x;
	video_wnd->y = y;
	video_wnd->width = w;
	video_wnd->height = h;
}

static void epg_macro_start_small_video(void)
{
	PlayerWindow video_wnd = {0};

	epg_small_play_wnd_rect_get(&video_wnd);
	g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL, &video_wnd);

	if(g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
	{
		g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	}

    GUI_SetInterface("flush", NULL);
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}

static void epg_macro_start_full_video(void)
{
	PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};

	GUI_SetInterface("flush", NULL);
	g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);

	if((g_AppPlayOps.normal_play.key == PLAY_KEY_UNLOCK)
			|| (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK))
	{
		g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
	}
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}
#define EPG_START_SMALL_VIDEO()     epg_macro_start_small_video()
#define EPG_START_FULL_VIDEO()      epg_macro_start_full_video()


static void _epg_prog_list_change(void)
{
	EpgCtl *ctl = &s_epg_ctl;
	int32_t sel = 0, cur_sel = 0;
	bool replay = false;
    uint32_t play_count = 0;

	GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "string", "Detail");
	GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
	GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
    play_count = g_AppChOps.real_pos(sel);
	if(play_count != g_AppPlayOps.normal_play.play_count)
	{
		replay = true;
	}

	app_epg_prog_info_update();
	app_epg_show_update_immediately();
    ctl->focus_time_bak = -1;
	if(ctl->detail_show)
    {
        app_epg_event_details_hide();
    }
    app_epg_brief_info_clear();
#if TMLIST_EPG_SUPPORT
	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
		GUI_SetProperty(TIMELIST_EPG_EVENT, "select_row", s_epg_event_row[cur_sel]);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
	}
#endif
	GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);

	if(replay)
	{
        GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "show");
        if(g_AppFullArb.state.tv  == STATE_ON)
        {
            GUI_SetInterface("video_off", NULL);
        }
        GUI_SetProperty(TEXT_EPG_SIGNAL, "draw_now", NULL);
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL, play_count);
	}
}

static bool _check_channels_exist(void)
{
    PopDlg pop;

    memset(&pop, 0, sizeof(PopDlg));
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.pos.x = APP_POP_DLG_POS_X;
    pop.pos.y = APP_POP_DLG_POS_Y;
    if(g_AppChOps.get_list_total() == 0)
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
            pop.str = STR_ID_NO_TV_CH;
        else
            pop.str = STR_ID_NO_RADIO_CH;

        popdlg_create(&pop);

        return false;
    }
    return true;
}

void app_create_epg_menu(void)
{
	app_epg_max_prog_init();
    if(false == _check_channels_exist())
        return;
    GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);

	if(app_epg_ctl_init() < 0)
		return;
	app_epg_ops_init();
#if TMLIST_EPG_SUPPORT || LVLIST_EPG_SUPPORT
#if TMLIST_EPG_SUPPORT
	int i = 0;
	for(i = 0; i < s_epg_max_prog_num; i++)
	{
		if(GXCORE_ERROR == app_epg_tmlist_data_init(&s_tm_epg_data[i]))
            goto err;
	}
	memset(s_tm_cnt_bak, 0, sizeof(int) * s_epg_max_prog_num);
#endif
#if LVLIST_EPG_SUPPORT
	if(GXCORE_ERROR == app_epg_lvlist_data_init(&s_lv_epg_data))
        goto err;
	s_lv_cnt_bak = 0;
#endif

	app_subt_pause();
    app_unset_window_back_ground(WND_FULL_SCREEN, true, false);
	GUI_CreateDialog(WND_EPG);

	return;
err:
#if TMLIST_EPG_SUPPORT
	for(i = 0; i < s_epg_max_prog_num; i++)
	{
        app_epg_list_data_release(&s_tm_epg_data[i]);
	}
#endif
#if LVLIST_EPG_SUPPORT
    app_epg_list_data_release(&s_lv_epg_data);
#endif
#endif
    app_epg_ctl_release();
    return;
}

static void app_normal_exit_epg_menu(void)
{
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
    {
        PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};

        GUI_EndDialog(WND_EPG);
        g_AppPlayOps.program_stop();
        g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
        GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
    }
    else
    {
        GUI_EndDialog(WND_EPG);
        app_channel_info_create_dialog();
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        EPG_START_FULL_VIDEO();
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        app_subt_change();

    }
}

static void app_book_trig_exit_epg_menu(void)
{
	g_AppPlayOps.program_stop();
	GUI_EndDialog("after wnd_full_screen");
	GUI_SetInterface("flush", NULL);
	PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
	g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL, &video_wnd);
	app_subt_change();
	GxPlayer_MediaVideoShow(PLAYER_FOR_NORMAL);
}

static bool app_epg_lock_prog_replay(void)
{
	char *detail_tip = NULL;
	GUI_GetProperty(TEXT_EPG_SHOW_DETAIL, "string", &detail_tip);
	if(strcmp(detail_tip, "Unlock") == 0)
	{
		int32_t sel = 0;
		GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
		PlayerWindow video_wnd = {0};
		epg_small_play_wnd_rect_get(&video_wnd);
		g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
        sel = g_AppChOps.real_pos(sel);
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL, sel);

		return true;
	}
	return false;
}

void app_epg_event_timer_set(bool bflag)
{
	static bool s_epg_event_timer = false;

	if(s_epg_event_timer == true) // app_timer_edit.c use this
	{
		GUI_SetProperty(TEXT_EPG_BOOK, "string", STR_ID_DELETE);
	}
	s_epg_event_timer = bflag;
}

static GxBook thiz_book;
static int _epg_book_pop_cb(PopDlgRet ret)
{
	if(POP_VAL_OK == ret)
	{
		g_AppBook.remove(&thiz_book);
		GUI_SetProperty(TEXT_EPG_BOOK, "string", STR_ID_BOOK);
	}
	if(g_AppFullArb.state.tv == STATE_ON && g_AppFullArb.tip == FULL_STATE_DUMMY)
	{
		GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "hide");
	}
	s_epg_ctl.update_flag = true;

	return 0;
}

static void app_epg_book_control(void)
{
	if(app_epg_focus_event_get() == GXCORE_ERROR)
		return;

	EpgCtl *ctl = &s_epg_ctl;

	memset(&thiz_book, 0, sizeof(GxBook));
	if(app_get_same_start_play_or_pvr_book(ctl->cur_prog.prog_data.id, ctl->focus_info->start_time, &thiz_book)
            && ((ctl->focus_info->start_time+ctl->zone_sec) >= ctl->local_sec))
	{
		PopDlg  pop;
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_SURE_DELETE;
		pop.exit_cb = _epg_book_pop_cb;
		pop.pos.x = APP_WND_EPG_POP_POS_X;
		pop.pos.y = APP_WND_EPG_POP_POS_Y;

		popdlg_create(&pop);
	}
	else  // add book
	{
		GxBookGet book_Get = {0};
		if(g_AppBook.get(&book_Get, BOOKMODE_ALL) >= APP_BOOK_NUM)
		{
			PopDlg  pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_NO_BTN;
			pop.str = STR_ID_TIMER_FULL;
			pop.mode = POP_MODE_UNBLOCK;
            pop.pos.x = APP_WND_EPG_POP_POS_X;
            pop.pos.y = APP_WND_EPG_POP_POS_Y;
			pop.timeout_sec = 3;
			popdlg_create(&pop);

			return;
		}

		g_AppFullArb.timer_stop();
		GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "show");
		GUI_SetProperty(TEXT_EPG_SIGNAL, "string", "");

		g_AppPlayOps.program_stop();
        GUI_SetProperty(IMG_EPG_BOOK, "state", "hide");
        GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "hide");
        GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "hide");
        GUI_SetProperty(TEXT_EPG_BOOK, "state", "hide");
        GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "hide");
        GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "hide");

        GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "string", "EXIT");
        GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "string", "Save");


		GUI_SetInterface("video_off",NULL);
		app_timer_edit_ex_func_register(app_epg_timer_edit_exit_cb);
		app_epg_timer_menu_exec(ctl->focus_info->start_time, ctl->focus_info->duration,
				ctl->cur_prog.prog_data.id, (char*)ctl->cur_prog.prog_data.prog_name);
	}
}

static void app_epg_change_show_day(int key)
{
	EpgCtl *ctl = &s_epg_ctl;

	if(key == STBK_YELLOW)
		(ctl->day_sel == MAX_EPG_DAY - 1) ? (ctl->day_sel = 0) : (ctl->day_sel++);
	else if(key == STBK_GREEN)
		(ctl->day_sel == 0) ? (ctl->day_sel =  MAX_EPG_DAY - 1) : (ctl->day_sel--);

	app_epg_set_event_date(ctl->local_day + (ctl->day_sel * SEC_PER_DAY));

#if TMLIST_EPG_SUPPORT
	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
		if(ctl->day_sel != 0)
		{
			ctl->tmlist_start_sec = 0;
			app_epg_set_tmlist_start_time("<00:00>", 0);
		}
		else
		{
			ctl->tmlist_start_sec = ctl->local_hour % SEC_PER_DAY;
			app_epg_set_tmlist_start_time(NULL, ctl->tmlist_start_sec);
		}
        GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);

		int sel = 0;
		uint32_t cur_sel = 0;//Force to the first item of Tmlist

		GUI_GetProperty(TIMELIST_EPG_EVENT, "select_row", &sel);
		GUI_SetProperty(s_epg_event_row[sel],"current_item", &cur_sel);
	}
#endif
#if LVLIST_EPG_SUPPORT
	if(ctl->epg_style == LVLIST_EPG_STYLE)
	{
		int sel = 0;
		GUI_GetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
	}
#endif
	app_epg_show_update_immediately();
}

static void app_epg_show_style_set(void)
{
	EpgCtl *ctl = &s_epg_ctl;
	int sel = 0, cur_sel = 0;

	if(ctl->epg_style == TMLIST_EPG_STYLE)
	{
		ctl->day_sel = 0;
		GUI_SetProperty(LISTVIEW_EPG_EVENT, "state", "hide");
		GUI_SetProperty(TIMELIST_EPG_EVENT, "state", "show");
		GUI_SetFocusWidget(TIMELIST_EPG_EVENT);

		GUI_GetProperty(LISTVIEW_EPG_PROG, "cur_sel", &cur_sel);
		GUI_SetProperty(TIMELIST_EPG_EVENT, "select_row", s_epg_event_row[cur_sel]);
		if(ctl->day_sel == 0)
		{
			ctl->tmlist_start_sec = ctl->local_hour % SEC_PER_DAY;
			app_epg_set_tmlist_start_time(NULL, ctl->tmlist_start_sec);
		}
		else
		{
			app_epg_set_tmlist_start_time("<00:00>", 0);
		}
         GUI_SetProperty(TIMELIST_EPG_EVENT, "update_all", NULL);
		s_epg_ctl.update_flag = true;
	}
	else if(ctl->epg_style == LVLIST_EPG_STYLE)
	{
		ctl->day_sel = 0;
		GUI_SetProperty(TIMELIST_EPG_EVENT, "state", "hide");
		GUI_SetProperty(LISTVIEW_EPG_EVENT, "state", "show");
		GUI_SetFocusWidget(LISTVIEW_EPG_PROG);

		GUI_SetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
		s_epg_ctl.update_flag = true;
	}
}

static void app_epg_gui_init(void)
{
    EpgCtl *ctl = &s_epg_ctl;

    app_epg_brief_info_clear();
    app_epg_show_sys_time_data();
	GUI_SetProperty(IMG_EPG_BOOK, "state", "hide");
	GUI_SetProperty(TEXT_EPG_BOOK, "state", "hide");
	GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "state", "hide");
	GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "state", "hide");
	app_epg_set_event_date(ctl->local_day + (ctl->day_sel * SEC_PER_DAY));
}

SIGNAL_HANDLER int app_epg_create(GuiWidget *widget, void *usrdata)
{
	int sel = 0;
	sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
#if !(TMLIST_EPG_SUPPORT && LVLIST_EPG_SUPPORT)
	GUI_SetProperty(IMG_EPG_MODE, "state", "hide");
	GUI_SetProperty(TEXT_EPG_MODE, "state", "hide");
#endif
    app_epg_gui_init();
	GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
	app_epg_show_style_set();

	int init_value = 0;
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
	    g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;

	EPG_START_SMALL_VIDEO();
	g_AppFullArb.timer_start();

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_destroy(GuiWidget *widget, void *usrdata)
{
	int init_value = 0;

	GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
	app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);

#if TMLIST_EPG_SUPPORT
	int i = 0;
	for(i = 0; i < s_epg_max_prog_num; i++)
	{
		app_epg_list_data_release(&s_tm_epg_data[i]);
	}
#endif
#if LVLIST_EPG_SUPPORT
	app_epg_list_data_release(&s_lv_epg_data);
	s_lv_cnt_bak = 0;
#endif
	app_epg_ctl_release();

	GxPlayer_MediaVideoHide(PLAYER_FOR_NORMAL);
	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.timer_stop();

	extern void back_to_main_menu_update_background(void);
	back_to_main_menu_update_background();

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_video_show(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_got_focus(GuiWidget *widget, void *usrdata)
{
	app_epg_event_timer_set(false);
	if(false == s_epg_ctl.detail_show)
	{
		GUI_SetProperty(TEXT_EPG_PROG_NAME, "reset_rolling", NULL);
		GUI_SetProperty(TEXT_EPG_EVENT_NAME, "reset_rolling", NULL);
	}

	if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_RADIO)
	{
		if(g_AppFullArb.tip == FULL_STATE_DUMMY)//74216
		{
			GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "show");
			GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "hide");
		}
		else
		{
			GUI_SetProperty(TEXT_EPG_SIGNAL, "state", "show");
		}
		GUI_SetProperty(TEXT_EPG_SIGNAL,"update",NULL);
	}
	s_epg_ctl.update_flag = true;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_lost_focus(GuiWidget *widget, void *usrdata)
{
	if(false == s_epg_ctl.detail_show)
	{
		GUI_SetProperty(TEXT_EPG_PROG_NAME, "rolling_stop", NULL);
		GUI_SetProperty(TEXT_EPG_EVENT_NAME, "rolling_stop", NULL);
	}

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_keypress(GuiWidget *widget, void *usrdata)
{
	EpgCtl *ctl = &s_epg_ctl;
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_EXIT:
					if(ctl->detail_show)
						app_epg_details_hide();
					else
						app_normal_exit_epg_menu();
					break;
				case STBK_MENU:
					app_normal_exit_epg_menu();
					break;
				case VK_BOOK_TRIGGER:
					if(ctl->detail_show)
						app_epg_event_details_hide();
					app_book_trig_exit_epg_menu();
					break;
				case STBK_OK:
					if(app_check_play_exec_flag())
						break;
					if(app_epg_lock_prog_replay())
						break;
					if(!(ctl->detail_show))
						app_epg_details_show();
					break;
				case STBK_UP:
				case STBK_DOWN:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					if(ctl->detail_show)
						app_epg_event_details_keypress(find_virtualkey_ex(event->key.scancode,event->key.sym));
					break;
				case STBK_RED:
#ifndef RADIO_REC_BOOK
					if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
						break;
#endif
                    if(LVLIST_EPG_STYLE == ctl->epg_style)
                    {
                        const char *widget = GUI_GetFocusWidget();
                        if(!(ctl->detail_show) && widget && 0 == strcmp(LISTVIEW_EPG_EVENT, widget))
                             app_epg_book_control();
                    }
                    else if(TMLIST_EPG_STYLE == ctl->epg_style)
                    {
                        if(!(ctl->detail_show))
                            app_epg_book_control();
                    }

					break;
				case STBK_GREEN:
				case STBK_YELLOW:
					if(!(ctl->detail_show))
						app_epg_change_show_day(find_virtualkey_ex(event->key.scancode,event->key.sym));
					break;
#if TMLIST_EPG_SUPPORT && LVLIST_EPG_SUPPORT
				case STBK_BLUE:
					if(!(ctl->detail_show))
					{
						if(ctl->epg_style == TMLIST_EPG_STYLE)
							ctl->epg_style = LVLIST_EPG_STYLE;
						else if(ctl->epg_style == LVLIST_EPG_STYLE)
							ctl->epg_style = TMLIST_EPG_STYLE;
						app_epg_show_style_set();
					}
					break;
#endif
				default:
					break;
			}
		default:
			break;
	}
	return ret;
}

SIGNAL_HANDLER int app_epg_prog_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	uint32_t sel;

	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;
		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
#if SAT2IP_SERVER_SUPPORT || DVB2IP_SERVER_SUPPORT
			if (g_AppChOps.get_list_total() != 0)
			{
				int prog_pos = 0;
				GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &prog_pos);
				switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
				{
					case STBK_UP:
						--prog_pos;
						if (prog_pos < 0)
							prog_pos = g_AppChOps.get_list_total() - 1;
						break;
					case STBK_DOWN:
						++prog_pos;
						if (prog_pos >= g_AppChOps.get_list_total())
							prog_pos = 0;
						break;
					case STBK_PAGE_UP:
						if (prog_pos == 0)
						{
							prog_pos = g_AppChOps.get_list_total() - 1;
						}
						else
						{
							prog_pos -= s_epg_max_prog_num;
							if (prog_pos < 0)
								prog_pos = 0;
						}
						break;
					case STBK_PAGE_DOWN:
						if (prog_pos == g_AppChOps.get_list_total() - 1)
						{
							prog_pos = 0;
						}
						else
						{
							prog_pos += s_epg_max_prog_num;
							if (prog_pos >= g_AppChOps.get_list_total())
								prog_pos = g_AppChOps.get_list_total() - 1;
						}
						break;
					default:
						break;
				}
#if SAT2IP_SERVER_SUPPORT
				extern int app_sat2ip_switch_tip_get_by_pos(int prog_pos);
				if (app_sat2ip_switch_tip_get_by_pos(prog_pos) < 0)
				{
					ret = EVENT_TRANSFER_STOP;
					break;
				}
#endif
#if DVB2IP_SERVER_SUPPORT
				extern int app_dvb2ip_switch_tip_get_by_pos(int prog_pos);
				if (app_dvb2ip_switch_tip_get_by_pos(prog_pos) < 0)
				{
					ret = EVENT_TRANSFER_STOP;
					break;
				}
#endif
			}
#endif
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_PAGE_UP:
					GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
					if(sel == 0)
					{
						sel = g_AppChOps.get_list_total() - 1;
						GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_PAGE_DOWN:
					GUI_GetProperty(LISTVIEW_EPG_PROG, "select", &sel);
					if(sel+1 == g_AppChOps.get_list_total())
					{
						sel = 0;
						GUI_SetProperty(LISTVIEW_EPG_PROG, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
#if LVLIST_EPG_SUPPORT
                    if(s_epg_ctl.epg_style == LVLIST_EPG_SUPPORT)
                    {
                        EpgListData *data = &s_lv_epg_data;
                        if(data->total != 0)
                        {
                            GUI_SetFocusWidget(LISTVIEW_EPG_EVENT);
                        }
                    }
#endif
                    ret = EVENT_TRANSFER_STOP;
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

SIGNAL_HANDLER int app_epg_prog_list_get_total(GuiWidget *widget, void *usrdata)
{
	return g_AppChOps.get_list_total();
}

SIGNAL_HANDLER int app_epg_prog_list_get_data(GuiWidget *widget, void *usrdata)
{
#define PROG_COUNT_LEN  (8)
	static GxMsgProperty_NodeByIdGet node;
	static char prog_name[MAX_PROG_NAME + 1];

	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if(NULL == item || 0 > item -> sel)
		return GXCORE_ERROR;

	node.node_type = NODE_PROG;
	node.id = g_AppChOps.get_id(item->sel);
	if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
        return GXCORE_ERROR;

	memset(prog_name, 0, MAX_PROG_NAME + 1);
	memcpy(prog_name, node.prog_data.prog_name, MAX_PROG_NAME);

	//col-0: num
	item->x_offset = 0;
	item->image = NULL;
	item->string = app_channel_num_str_get(&node, item->sel);

	//col-1: channel name
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->image = NULL;
	item->string = prog_name;

	//col-2: scramble
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	if(node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
		item->image = "s_icon_money";
	else
		item->image = NULL;

	//col-3: lock
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	if(g_AppPvrOps.state == PVR_RECORD && node.prog_data.id == g_AppPvrOps.env.prog_id)
		item->image = "s_ts_red"; // record
	else if(node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
		item->image = "s_icon_lock"; // lock
	else
		item->image = NULL;

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_prog_list_change(GuiWidget *widget, void *usrdata)
{
	_epg_prog_list_change();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_tmlist_get_data(GuiWidget *widget, void *usrdata)
{
	TimeItemPara* time = NULL;

	time = (TimeItemPara *)usrdata;

#if TMLIST_EPG_SUPPORT
	static int index = 0;
	char **pstr = NULL;

	if(time->from_start)
		index = 0;

	if((time->sel >= s_epg_max_prog_num) || (s_tm_epg_data[time->sel].detail == NULL))
		return EVENT_TRANSFER_STOP;
	pstr = s_tm_epg_data[time->sel].detail;
	time->string = pstr[index];
	index++;
#else
	if(time->sel >= s_epg_max_prog_num)
		return GXCORE_ERROR;
	time->string = "<end>";
#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_tmlist_change(GuiWidget *widget, void *usrdata)
{
#if TMLIST_EPG_SUPPORT
	bool day_change_flag = false;
	EpgCtl *ctl = &s_epg_ctl;
	char tm_str[8] = {0};
	time_t offset_sec = 0;
	char *move_direct = NULL;
	time_t day_start = 0;

	// timelist change 后直到这次的GUI_Exec完成，才能正确获取到 select_column, 所以focus timer先remove再add
	APP_TIMER_REMOVE(ctl->focus_update);

	GUI_GetProperty(TIMELIST_EPG_EVENT, "start", tm_str);
	tm_str[6] = '\0';
	offset_sec = app_edit_str_to_hourmin_sec(tm_str + 1);
	offset_sec %= SEC_PER_DAY;
	ctl->tmlist_start_sec = offset_sec;

	move_direct = (char*)usrdata;
	if((offset_sec == 0) && (strcmp(move_direct, "right") == 0))
	{
		day_change_flag = true;
		(ctl->day_sel == MAX_EPG_DAY - 1) ? (ctl->day_sel = 0) : (ctl->day_sel++);
		app_epg_set_tmlist_start_time("<00:00>", 0);
	}
	else if((offset_sec == 23*SEC_PER_HOUR) && (strcmp(move_direct, "left") == 0))
	{
		day_change_flag = true;
		(ctl->day_sel == 0) ? (ctl->day_sel =  MAX_EPG_DAY - 1) : (ctl->day_sel--);
		app_epg_set_tmlist_start_time("<23:00>", 0);
	}

	day_start = ctl->local_day + ctl->day_sel * SEC_PER_DAY;
	if(day_change_flag)
		app_epg_set_event_date(day_start);

	// 这里用定时器向右刷新会重置focus
	int i = 0;
	for(i = 0; i < s_epg_max_prog_num; i++)
	{
		app_epg_tmlist_data_get(i);
	}

	APP_TIMER_ADD(ctl->focus_update, app_epg_focus_update_exec, 500, TIMER_REPEAT);
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_tmlist_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
#if TMLIST_EPG_SUPPORT
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_UP:
				case STBK_DOWN:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					GUI_SendEvent(LISTVIEW_EPG_PROG, event);
					g_AppFullArb.state.pause = STATE_OFF;
					ret = EVENT_TRANSFER_STOP;
					break;

				default:
					break;
			}
		default:
			break;
	}
#endif
	return ret;
}

SIGNAL_HANDLER int app_epg_lvlist_get_total(GuiWidget *widget, void *usrdata)
{
#if LVLIST_EPG_SUPPORT
	return s_lv_epg_data.total;
#endif
	return 0;
}

SIGNAL_HANDLER int app_epg_lvlist_get_data(GuiWidget *widget, void *usrdata)
{
#if LVLIST_EPG_SUPPORT
#define CNT_STR_LEN  (8)
	EpgListData *data = &s_lv_epg_data;
	ListItemPara* item = NULL;

	item = (ListItemPara*)usrdata;
	if(NULL == item || 0 > item->sel)
		return GXCORE_ERROR;

	static char cnt_str[CNT_STR_LEN];
	memset(cnt_str, 0, CNT_STR_LEN);
	sprintf(cnt_str, " %d", (item->sel+1));

	//周总要求只显示 (time, title, flag)
	item->x_offset = 0;
	item->image = NULL;
	item->string = data->detail[item->sel];

	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = NULL;
	if(data->index[item->sel].flag & PLAY_EPG_FLAG)
		item->image = "s_ts_green";
	else if(data->index[item->sel].flag & PVR_EPG_FLAG)
		item->image = "s_ts_red";
	else
		item->image = NULL;
#endif

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_lvlist_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_epg_lvlist_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
#if LVLIST_EPG_SUPPORT
	int sel = 0;
	EpgListData *data = &s_lv_epg_data;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
            {
                case STBK_PAGE_UP:
                    GUI_GetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
                    if(sel == 0)
                    {
                        sel = data->total - 1;
                        GUI_SetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_PAGE_DOWN:
                    GUI_GetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
                    if(sel+1 >= data->total)
                    {
                        sel = 0;
                        GUI_SetProperty(LISTVIEW_EPG_EVENT, "select", &sel);
                        ret = EVENT_TRANSFER_STOP;
                    }
                    break;
                case STBK_LEFT:
                case STBK_RIGHT:
                    if(s_epg_ctl.epg_style == LVLIST_EPG_SUPPORT)
                    {
                        GUI_SetFocusWidget(LISTVIEW_EPG_PROG);
                        GUI_SetProperty(IMG_EPG_BOOK, "state", "hide");
                        GUI_SetProperty(TEXT_EPG_BOOK, "state", "hide");
                    }
                    ret = EVENT_TRANSFER_STOP;
                    break;
                default:
                    break;
            }
        default:
            break;
    }
#endif
	return ret;
}

int app_epg_cur_next_epg_info_str_get(GxMsgProperty_NodeByPosGet *node_prog, GxEpgInfo *epg_info,
		char **cur_str, char **next_str, int *cur_per)
{
#define CH_EPG_TIME_LEN 15
#define MAX_EVENT_LEN   50
	AppEpgOps epg_ops;
	EpgProgInfo s_epg_prog_info;

	uint32_t epg_str_len = CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1;
	static char cur_epg_str[CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1];
	static char next_epg_str[CH_EPG_TIME_LEN + MAX_EVENT_LEN + 1];

	time_t time = 0;
	time_t cur_event_time = 0;
	GxTime time_cur = {0};
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};

	if (!node_prog || !cur_str || !next_str)
	{
		app_log_error("[%s:%d], null ptr!\n", __func__, __LINE__);
		return GXCORE_ERROR;
	}

	epg_ops_init(&epg_ops);
	app_epg_prog_info_set(&s_epg_prog_info, node_prog);
	epg_ops.prog_info_update(&epg_ops, &s_epg_prog_info);
	if(epg_ops.get_cur_event_cnt(&epg_ops) <= 0)
	{
		*cur_str = STR_ID_NO_INFO;
		*next_str = STR_ID_NO_INFO;
		return GXCORE_SUCCESS;
	}
	GxCore_GetLocalTime(&time_cur);

	if ((epg_ops.get_cur_event_info(&epg_ops, epg_info) == GXCORE_SUCCESS)
			&& (epg_info->event_name_len > 0)
			&& (epg_info->start_time <= time_cur.seconds)
			&& (epg_info->start_time + epg_info->duration >= time_cur.seconds))
	{
        time = get_display_time_by_timezone(epg_info->start_time);
		app_epg_hourmin_str_get(time, start_time_str);
		time = get_display_time_by_timezone(epg_info->start_time + epg_info->duration);
		app_epg_hourmin_str_get(time, finish_time_str);
		snprintf(cur_epg_str, epg_str_len, "%s - %s  %s", start_time_str, finish_time_str, (char*)epg_info->event_name);
		*cur_str = cur_epg_str;
		if (cur_per)
			*cur_per= (epg_info->duration > 0) ? ((time_cur.seconds-epg_info->start_time) * 100 / epg_info->duration) : (0);
	}
	else
	{
		*cur_str = STR_ID_NO_INFO;
	}

    cur_event_time = epg_info->start_time;
	if ((epg_ops.get_next_event_info(&epg_ops, epg_info) == GXCORE_SUCCESS)
			&& (epg_info->event_name_len > 0)
            && (cur_event_time <= time_cur.seconds)
			&& (epg_info->start_time > time_cur.seconds))
	{
		time = get_display_time_by_timezone(epg_info->start_time);
		app_epg_hourmin_str_get(time, start_time_str);
		time = get_display_time_by_timezone(epg_info->start_time + epg_info->duration);
		app_epg_hourmin_str_get(time, finish_time_str);
		snprintf(next_epg_str, epg_str_len, "%s - %s  %s", start_time_str, finish_time_str, (char*)epg_info->event_name);
		*next_str = next_epg_str;
	}
	else
	{
		*next_str = STR_ID_NO_INFO;
	}

	return GXCORE_SUCCESS;
}

GxEpgInfo *app_epg_info_mem_alloc(void)
{
	GxEpgInfo *epg_info = NULL;

	if ((epg_info = (GxEpgInfo *)GxCore_Calloc(EVENT_GET_NUM, EVENT_GET_MAX_SIZE)) == NULL)
	{
		app_log_error("[%s:%d], malloc failed!\n", __func__, __LINE__);
		return NULL;
	}
	return epg_info;
}

void app_epg_timer_edit_exit_cb(void)
{
    if (GUI_CheckDialog(WND_EPG) != GXCORE_SUCCESS)
            return ;
    GUI_SetProperty(IMG_EPG_BOOK, "state", "show");
    GUI_SetProperty(IMG_EPG_PREV_DAY, "state", "show");
    GUI_SetProperty(IMG_EPG_NEXT_DAY, "state", "show");
    GUI_SetProperty(TEXT_EPG_BOOK, "state", "show");
    GUI_SetProperty(TEXT_EPG_PREV_DAY, "state", "show");
    GUI_SetProperty(TEXT_EPG_NEXT_DAY, "state", "show");

    GUI_SetProperty(IMG_EPG_SHOW_DETAIL, "string", "OK");
    GUI_SetProperty(TEXT_EPG_SHOW_DETAIL, "string", "Detail");
    EPG_START_SMALL_VIDEO();
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppFullArb.timer_start();
    return;
}

#if AUTO_UPDATE_SUPPORT || PMT_UPDATE_SUPPORT
void app_epg_preprocess_for_pmt(void)
{
    int sel = 0;
    EpgCtl *ctl = &s_epg_ctl;
    if(ctl->detail_show)
        return;
    GUI_SetProperty(LISTVIEW_EPG_PROG, "update_all", NULL);
    sel = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
	GUI_SetProperty(LISTVIEW_EPG_PROG, "active", &sel);
    app_update_pop_dlg(WND_POP_TIP);
}
#endif

#if AUTO_UPDATE_SUPPORT || PROGRAM_NETUP_SUPPORT
void app_epg_preprocess_for_acu(void)
{
    EpgCtl *ctl = &s_epg_ctl;
    if(ctl->detail_show)
        app_epg_details_hide();
#if LVLIST_EPG_SUPPORT
    GUI_SetFocusWidget(LISTVIEW_EPG_PROG);
#endif
    GUI_SetProperty(TEXT_EPG_EVENT_NAME, "string", STR_ID_BLANK);
    GUI_SetProperty(TEXT_EPG_START_TIME, "string", STR_ID_BLANK);
    GUI_SetProperty(TEXT_EPG_TIME_INTERVAL, "string", STR_ID_BLANK);
    GUI_SetProperty(TEXT_EPG_END_TIME, "string", STR_ID_BLANK);

}

void app_epg_reset_for_acu(void)
{
    if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
    {
        app_epg_prog_info_update();
        app_epg_brief_info_show();
        app_update_pop_dlg(WND_POP_TIP);
    }
}
#endif
