#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "app_wnd_system_setting_opt.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "richepg_block_mem.h"
#include "richepg_epg.h"
#include "richepg.h"

#define EUTEL_EPG_MAX_PROG              10
#define EUTEL_EPG_MAX_DAY               8

#define SEC_PER_MIN                     60
#define SEC_PER_HOUR                    3600  // 60*60
#define SEC_PER_DAY                     86400 // 60*60*24
#define DISPLAY_ZONE_HOUR               4
#define NO_EVNET_NAME_STR               "-"   // STR_ID_UNKNOW

enum {
	CUR_EPG_FLAG  = 1 << 0,
	PLAY_EPG_FLAG = 1 << 1,
	PVR_EPG_FLAG  = 1 << 2,
	INVALID_EPG_FLAG  = 1 << 3
};

typedef struct {
	int event_id;
	time_t start_time;
	time_t finish_time;
	int parent_rate;
	int genres_num;
	int *genres_ids;
	// char *event_logo;
	char *event_title;
	// char *event_desc;

	uint32_t event_flag;
	char *timelist_data;
} EutelEpgListIndex;


typedef struct {
	BlockMemMgr mem_mgr;
	uint16_t prog_id;
	int channel_id;
	int total;
	int vtotal;
	EutelEpgListIndex **index;
	EutelEpgListIndex **vindex;
} EutelEpgListData;

typedef struct {
	bool        detail_show;
	bool        update_flag;
	int         day_sel;
	int         content_sel;

	time_t      zone_sec;
	time_t      local_sec;      // = utc_time + zone_sec
	time_t      local_hour;     // = local_sec - local_sec % SEC_PER_HOUR
	time_t      local_day;      // = local_hour - local_hour % SEC_PER_DAY
	time_t      next_event_sec;
	time_t      tmlist_start_sec;
	time_t      focus_time_bak;

	int         prog_sel;
	int         cur_sel;
	GxBusPmDataProg cur_prog;

	EutelEpgListData data[EUTEL_EPG_MAX_PROG];
	EutelEpgListData lv_data;
	RichepgEpgEvent *use_event;
	RichepgEpgEvent *focus_event;

	event_list* time_update;
	event_list* show_update;
	event_list* focus_update;
} EutelEpgCtrl;

static EutelEpgCtrl s_eutel_epg_ctrl;

int app_eutel_epg_timer_add(void);
int app_eutel_epg_timer_rm(void);
extern bool app_get_same_start_play_or_pvr_book(uint16_t prog_id, time_t start_time, GxBook *book_ret);
extern void app_timer_edit_ex_func_register(void (*exit_func)(void));
extern bool app_epg_timer_menu_exec(time_t start_time, time_t duration, int prog_id, const char *prog_name);

typedef struct {
	const char *wnd;
	const char *sys_date;
	const char *sys_time;
	const char *epg_date;
	const char *lv_point;
	const char *lv_prog;
	const char *tl_event;
	char      **event_row;
	const char *ct_image;
	const char *ct_filter;
	const char *lv_event;

	const char *ch_filter;
	const char *ch_name;
	const char *ch_type;
	const char *ch_genres;

	const char *event_logo;
	const char *event_title;
	const char *event_parent;
	const char *event_genres;
	const char *event_duration;

	const char *details_back;
	const char *details_duration;
	const char *details_title;
	const char *details_info;
	const char *details_exit;

	const char *img_book;
	const char *text_book;
} EutelEpgWidget;

static char *s_eutel_epg_event_row[] = {
	"timeitem_eutel_epg_event_list1",
	"timeitem_eutel_epg_event_list2",
	"timeitem_eutel_epg_event_list3",
	"timeitem_eutel_epg_event_list4",
	"timeitem_eutel_epg_event_list5",
	"timeitem_eutel_epg_event_list6",
	"timeitem_eutel_epg_event_list7",
	"timeitem_eutel_epg_event_list8",
	"timeitem_eutel_epg_event_list9",
	"timeitem_eutel_epg_event_list10"
};

static EutelEpgWidget s_eutel_epg_wgt;
static void app_eutel_epg_widget_init(void)
{
	EutelEpgWidget *widget = &s_eutel_epg_wgt;

	widget->wnd = WND_EUTEL_EPG;
	widget->sys_date = "text_eutel_epg_sys_date";
	widget->sys_time = "text_eutel_epg_sys_time";
	widget->epg_date = "text_eutel_epg_date";
	widget->lv_point = "listview_eutel_epg_point";
	widget->lv_prog = "listview_eutel_epg_prog";
	widget->tl_event = "timelist_eutel_epg_event";
	widget->event_row = s_eutel_epg_event_row;
	widget->ct_image = "img_eutel_epg_content_filter";
	widget->ct_filter = "text_eutel_epg_content_filter";
	widget->lv_event = "listview_eutel_epg_event";

	widget->ch_filter = "text_eutel_epg_channel_filter";
	widget->ch_name = "text_eutel_epg_channel_name";
	widget->ch_type = "text_eutel_epg_channel_type";
	widget->ch_genres = "text_eutel_epg_channel_genres";

	widget->event_logo = "img_eutel_epg_event_logo";
	widget->event_title = "text_eutel_epg_event_title";
	widget->event_parent = "text_eutel_epg_event_parent";
	widget->event_genres = "text_eutel_epg_event_genres";
	widget->event_duration = "text_eutel_epg_event_duration";

	widget->details_back = "img_eutel_epg_event_details_back";
	widget->details_duration = "text_eutel_epg_event_details_duration";
	widget->details_title = "text_eutel_epg_event_details_title";
	widget->details_info = "notepad_eutel_epg_event_details";
	widget->details_exit = "btn_eutel_epg_event_details_exit";

	widget->img_book = "img_eutel_epg_fav_tip";
	widget->text_book = "text_eutel_epg_fav_tip";
}

static time_t _utc_time_get(void)
{
	GxTime time = {0};

	GxCore_GetLocalTime(&time);
	return time.seconds;
}

// EUTEL_MEM_EPG
static int app_eutel_epg_list_data_free(EutelEpgListData *data)
{
	if (!data)
		return -1;

	block_mem_mgr_free_data(&data->mem_mgr);
	data -> total = 0;
	data -> vtotal = 0;
	data -> index = NULL;
	data -> vindex = NULL;

	return 0;
}

static int app_eutel_epg_list_data_release(EutelEpgListData *data)
{
	if (!data)
		return -1;

	data -> prog_id = 0;
	data -> channel_id = 0;
	app_eutel_epg_list_data_free(data);

	return 0;
}

static int app_eutel_epg_list_data_release_all(void)
{
	int i = 0;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	for (i = 0; i < EUTEL_EPG_MAX_PROG; i++)
	{
		app_eutel_epg_list_data_release(&ctrl->data[i]);
	}
	app_eutel_epg_list_data_release(&ctrl->lv_data);

	return 0;
}

static int app_eutel_epg_ctrl_release(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	memset(&ctrl->cur_prog, 0, sizeof(ctrl->cur_prog));

	app_eutel_epg_list_data_release_all();
	app_eutel_epg_timer_rm();
	APP_FREE(ctrl->use_event);
	APP_FREE(ctrl->focus_event);

	return 0;
}

static int app_eutel_epg_ctrl_init(void)
{
	int i = 0;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	static bool head_init_flag = false;

	if (!head_init_flag)
	{
		head_init_flag = true;
		for (i = 0; i < EUTEL_EPG_MAX_PROG; i++)
		{
			ctrl->data[i].mem_mgr.attr.mem_id = i;
			ctrl->data[i].mem_mgr.attr.def_size = 1024 * 16;
			ctrl->data[i].mem_mgr.attr.align_byte = 4;
			ctrl->data[i].mem_mgr.attr.fast_alloc = 0;
            ctrl->data[i].mem_mgr.attr.max_size = 0;
			block_mem_mgr_init(&ctrl->data[i].mem_mgr);
		}
		ctrl->lv_data.mem_mgr.attr.mem_id = i;
		ctrl->lv_data.mem_mgr.attr.def_size = 1024 * 16;
		ctrl->lv_data.mem_mgr.attr.align_byte = 4;
		ctrl->lv_data.mem_mgr.attr.fast_alloc = 0;
		ctrl->lv_data.mem_mgr.attr.max_size = 0;
		block_mem_mgr_init(&ctrl->lv_data.mem_mgr);
	}
	app_eutel_epg_ctrl_release();

	ctrl->update_flag = false;
	ctrl->detail_show = false;
	ctrl->day_sel = 0;
	ctrl->content_sel = 0;

	ctrl->zone_sec = get_display_time_by_timezone(0);
	ctrl->local_sec = _utc_time_get() + ctrl->zone_sec;
	ctrl->local_hour = ctrl->local_sec - ctrl->local_sec % SEC_PER_HOUR;
	ctrl->local_day = ctrl->local_hour - ctrl->local_hour % SEC_PER_DAY;
	ctrl->next_event_sec = 0;
	ctrl->tmlist_start_sec = ctrl->local_hour % SEC_PER_DAY;
	ctrl->focus_time_bak = -1;

	if ((ctrl->use_event = GxCore_Calloc(1, EUTEL_EPG_EVENT_SIZE)) == NULL)
	{
		EUTEL_ERR("malloc failed!\n");
		goto err;
	}
	if ((ctrl->focus_event = GxCore_Calloc(1, EUTEL_EPG_EVENT_SIZE)) == NULL)
	{
		EUTEL_ERR("malloc failed!\n");
		goto err;
	}
	app_eutel_epg_timer_add();

	return 0;
err:
	app_eutel_epg_ctrl_release();
	return -1;
}

int app_eutel_epg_real_create_dialog(void)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	if (ctrl->use_total == 0)
	{
		EUTEL_INFO("No prog!\n");
		return -1;
	}
	app_eutel_epg_widget_init();
	if (app_eutel_epg_ctrl_init() < 0)
	{
		return -1;
	}

	if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_EUTEL_CHINFO);
	}
/*
	if (GUI_CheckDialog(WND_EUTEL_RCHINFO) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_EUTEL_RCHINFO);
	}
*/
	GUI_CreateDialog(s_eutel_epg_wgt.wnd);

	return 0;
}

int app_eutel_epg_create_dialog(void)
{
	if (richepg_epg_build_working_check())
	{
		PopDlg pop = {0};

		pop.type = POP_TYPE_NO_BTN;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_EPG_GENERATING_TIP;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
	}
	else if (richepg_epg_channel_total_get() == 0)
	{
		app_eutel_epg_maint_popup(STR_ID_EPG_EMPTY_TIP, true);
	}
	else
	{
		GxTime sys_time = {0};

		GxCore_GetLocalTime(&sys_time);
		if(sys_time.seconds > richepg_epg_last_finish_time_get())
		{
			app_eutel_epg_maint_popup(STR_ID_EPG_OUT_DATE_TIP, true);
		}
		else
		{
			return app_eutel_epg_real_create_dialog();
		}
	}

	return -1;
}

static inline bool _is_timelist(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	return (ctrl->content_sel == 0) ? true : false;
}

static void _set_focus_widget(const char *wgt)
{
#define UNFOCUS_FORECOLOR   "[text_color,text_color,text_color]"
#define FOCUS_FORECOLOR     "[key_yellow,key_yellow,key_yellow]"

	if (_is_timelist())
	{
		if (wgt == s_eutel_epg_wgt.tl_event)
		{
			GUI_SetProperty(s_eutel_epg_wgt.epg_date, "forecolor", UNFOCUS_FORECOLOR);
		}
	}
	else
	{
		if (wgt == s_eutel_epg_wgt.lv_event)
		{
			GUI_SetProperty(s_eutel_epg_wgt.epg_date, "forecolor", UNFOCUS_FORECOLOR);
			GUI_SetProperty(s_eutel_epg_wgt.ct_filter, "forecolor", FOCUS_FORECOLOR);
		}
		else if (wgt == s_eutel_epg_wgt.lv_prog)
		{
			GUI_SetProperty(s_eutel_epg_wgt.epg_date, "forecolor", FOCUS_FORECOLOR);
			GUI_SetProperty(s_eutel_epg_wgt.ct_filter, "forecolor", UNFOCUS_FORECOLOR);
		}
	}

	GUI_SetFocusWidget(wgt);
}

static inline int app_eutel_epg_time_cmp(time_t a, time_t b)
{
	return (a - a % SEC_PER_MIN) - (b - b % SEC_PER_MIN);
}

static int app_eutel_epg_show_time_range_get(time_t *show_start, time_t *show_finish)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	time_t day_start = 0;

	day_start = ctrl->local_day + ctrl->day_sel * SEC_PER_DAY;
	*show_start = day_start + ctrl->tmlist_start_sec;
	*show_finish = day_start + ctrl->tmlist_start_sec + (DISPLAY_ZONE_HOUR) * SEC_PER_HOUR;

	return 0;
}

static int app_eutel_epg_lv_show_time_range_get(time_t *show_start, time_t *show_finish)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	time_t day_start = 0;

	day_start = ctrl->local_day + ctrl->day_sel * SEC_PER_DAY;
	*show_start = (ctrl->day_sel > 0) ? day_start : ctrl->local_sec;
	*show_finish = day_start + SEC_PER_DAY;

	return 0;
}

#define HOURMIN_STR_LEN     6
#define TMLIST_DAY_STR_LEN  3
void app_eutel_epg_hourmin_str_get(time_t sec, char *str)
{
	char *tmp_str = NULL;
	tmp_str = app_time_to_hourmin_edit_str(sec);
	if (tmp_str)
	{
		sprintf(str, "%s", tmp_str);
		GxCore_Free(tmp_str);
	}
}

static void app_eutel_epg_timlist_day_str_get(time_t sec, char *str)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	time_t day_start = ctrl->local_day + ctrl->day_sel * SEC_PER_DAY;
	int day_sel = 0;

	if (sec >= day_start)
		day_sel = (sec - day_start) / SEC_PER_DAY;
	else
		day_sel = -1 - (day_start - 1 - sec) / SEC_PER_DAY;

	if (day_sel > 0)
		sprintf(str, "+%d", day_sel);
	else if (day_sel < 0)
		sprintf(str, "%d", day_sel);
}

static char* app_eutel_epg_tmlist_item_format(EutelEpgListIndex *index, int vsel, BlockMemMgr *mem_mgr, int array_sel)
{
	// (<>len=2)*4 + (vsel_len=4) + (start_time_len=5) + (start_day_len=2) + (finish_time_len=5) + (finish_day_len=2) + ('\0'len=1) = 27
	// TMLIST_BLANK_LEN + (<>len=2)*3 + (fore_len=7) + (back_len=7) + (font_len=5) = 52

#define TMLIST_BLANK_LEN  27
#define TMLIST_COLOR_LEN  52  //修改了font_str请修改这个值

#define TMLIST_BLANK_STR  "<%d><%s%s><%s%s><>"
#define TMLIST_EXIST_STR  "<%d><%s%s><%s%s><%s>"
#define TMLIST_COLOR_STR  "<%d><%s%s><%s%s><%s><%s><%s><%s>"

	int total_len = 0;
	char start_time_str[HOURMIN_STR_LEN] = {0};
	char finish_time_str[HOURMIN_STR_LEN] = {0};
	char start_day_str[TMLIST_DAY_STR_LEN] = {0};
	char finish_day_str[TMLIST_DAY_STR_LEN] = {0};
	time_t start_time = index->start_time;
	time_t finish_time = index->finish_time;
#if 1 // 长事件timelist会显示出错，应用规避
	time_t show_start = 0, show_finish = 0;

	// 多SEC_PER_HOUR的原因是防止出现00:00-00:00显示出错
	app_eutel_epg_show_time_range_get(&show_start, &show_finish);
	if (start_time < show_start - SEC_PER_HOUR)
		start_time = show_start - SEC_PER_HOUR;
	if (finish_time > show_finish + SEC_PER_HOUR)
		finish_time = show_finish + SEC_PER_HOUR;
#endif

	app_eutel_epg_hourmin_str_get(start_time, start_time_str);
	app_eutel_epg_hourmin_str_get(finish_time, finish_time_str);
	app_eutel_epg_timlist_day_str_get(start_time, start_day_str);
	app_eutel_epg_timlist_day_str_get(finish_time, finish_day_str);

	const char* name_str = (index->event_title && strlen(index->event_title) > 0) ? \
							(index->event_title) : (app_richepg_translate_str(NO_EVNET_NAME_STR));
	const char* fore_str = NULL;
	const char* back_str = NULL;
	const char* font_str = "Arial";

	if (index->event_flag & CUR_EPG_FLAG)
	{
		fore_str = "#f4f4f4";
		back_str = "#005a99";
	}
	else if (index->event_flag & PVR_EPG_FLAG)
	{
		back_str = "#E6A200";
		fore_str= "#f4f4f4";
	}
	else if (index->event_flag & PLAY_EPG_FLAG)
	{
		fore_str= "#f4f4f4";
		back_str = "#008000";
	}

	if (index->event_flag & INVALID_EPG_FLAG)
	{
		EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
		EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
		EutelChannelArg *arg = NULL;
		int prog_sel = 0;

		prog_sel = ctrl->prog_sel - ctrl->cur_sel + array_sel;
		if (prog_sel >= 0 && prog_sel < ch_ctrl->use_total)
			arg = &ch_ctrl->arg_array[ch_ctrl->use_array[prog_sel].pos];

		if (index->finish_time <= ctrl->local_sec)
			name_str = STR_ID_BLANK;
		else if (arg && arg->genre_num > 0)
			name_str = eutel_array_name_get(1, arg->genre_ids, EUTEL_CHANNEL_GENRE);
		else
			name_str = NO_EVNET_NAME_STR;
	}

	if (back_str == NULL)
		total_len = TMLIST_BLANK_LEN + strlen(name_str);
	else
		total_len = TMLIST_COLOR_LEN + strlen(name_str);

	if ((index->timelist_data = block_mem_mgr_alloc_data(total_len, mem_mgr)) == NULL)
	{
		EUTEL_ERR("block_mem_add_none failed!\n");
		return NULL;
	}

	if (back_str == NULL)
		sprintf(index->timelist_data, TMLIST_EXIST_STR, vsel, start_time_str, start_day_str, finish_time_str, finish_day_str,
				name_str);
	else
		sprintf(index->timelist_data, TMLIST_COLOR_STR, vsel, start_time_str, start_day_str, finish_time_str, finish_day_str,
				name_str, fore_str, back_str, font_str);

	return index->timelist_data;
}

static int app_eutel_epg_event_info_set_flag(EutelEpgListIndex *index, uint16_t prog_id, int array_sel)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	index->event_flag = 0;
	if ((prog_id == ctrl->cur_prog.id && ctrl->cur_sel == array_sel)
			&& (ctrl->local_sec >= index->start_time && ctrl->local_sec < index->finish_time))
	{
		// current event
		index->event_flag |= CUR_EPG_FLAG;
	}
	else if (ctrl->local_sec >= index->start_time)
	{
		// do nothing
	}
	else
	{
		GxBook prog_book;
		memset(&prog_book, 0, sizeof(GxBook));
		if (app_get_same_start_play_or_pvr_book(prog_id, index->start_time - ctrl->zone_sec, &prog_book)
				&& (index->start_time >= ctrl->local_sec))
		{
			if (prog_book.book_type == BOOK_PROGRAM_PLAY)
			{
				index->event_flag |= PLAY_EPG_FLAG;
			}
			else if (prog_book.book_type == BOOK_PROGRAM_PVR)
			{
				index->event_flag |= PVR_EPG_FLAG;
			}
		}
	}

	return 0;
}

static int app_eutel_epg_tmlist_data_get(EutelEpgListData *data, time_t show_start, time_t show_finish, int array_sel)
{
#define INSERT_INVALID_EPG_EVENT() do { \
	if ((data->vindex[cnt] = block_mem_mgr_alloc_data(sizeof(EutelEpgListIndex), &data->mem_mgr)) == NULL) { \
		EUTEL_ERR("block_mem_mgr_alloc_data failed!\n"); \
		goto err; \
	} \
	data->vindex[cnt]->start_time = start_time; \
	data->vindex[cnt]->finish_time = finish_time; \
	data->vindex[cnt]->event_title = app_richepg_translate_str(STR_ID_NO_EVENT); \
	app_eutel_epg_event_info_set_flag(data->vindex[cnt], data->prog_id, array_sel); \
	data->vindex[cnt]->event_flag |= INVALID_EPG_FLAG; \
	if (app_eutel_epg_tmlist_item_format(data->vindex[cnt], cnt, &data->mem_mgr, array_sel) == NULL) { \
		EUTEL_ERR("tmlist_item_format failed!\n"); \
		goto err; \
	} \
	cnt++; \
} while(0)

	int i = 0, cnt = 0;
	time_t start_time = 0, finish_time = 0;

	if (!data)
		return -1;

	if (data->total == 0)
	{
		start_time = show_start;
		finish_time = show_finish;
		INSERT_INVALID_EPG_EVENT();
	}
	else
	{
		for (i = 0; i < data->total; i++)
		{
			if (i == 0)
			{
				start_time = show_start;
				finish_time = data->index[i]->start_time;
			}
			else
			{
				start_time = data->index[i-1]->finish_time;
				finish_time = data->index[i]->start_time;
			}
			if (app_eutel_epg_time_cmp(start_time, finish_time) < 0)
			{
				INSERT_INVALID_EPG_EVENT();
			}

			data->vindex[cnt] = data->index[i];
			if (app_eutel_epg_tmlist_item_format(data->vindex[cnt], cnt, &data->mem_mgr, array_sel) == NULL)
			{
				EUTEL_ERR("tmlist_item_format failed!\n");
				goto err;
			}
			cnt++;
		}
		if (data->index[data->total-1]->finish_time < show_finish)
		{
			start_time = data->index[data->total-1]->finish_time;
			finish_time = show_finish;
			INSERT_INVALID_EPG_EVENT();
		}
	}

	data->vtotal = cnt;

	return cnt;
err:
	return -1;
}

static int app_eutel_epg_event_info_update(EutelEpgListData *data, int array_sel)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	int i = 0;
	int total = 0;
	int alloc_size = 0;
	int cnt = 0;
	time_t show_start = 0, show_finish = 0;
	char *event_title = NULL;

	if (!data)
		return -1;

	app_eutel_epg_list_data_free(data);
	app_eutel_epg_show_time_range_get(&show_start, &show_finish);

	if (ctrl->local_sec >= show_finish)
	{
		total = 0;
	}
	else if (ctrl->local_sec >= show_start)
	{
		total = richepg_epg_event_total_get(data->channel_id, ctrl->local_sec - ctrl->zone_sec, show_finish - ctrl->zone_sec);
	}
	else
	{
		total = richepg_epg_event_total_get(data->channel_id, show_start - ctrl->zone_sec, show_finish - ctrl->zone_sec);
	}
	alloc_size = (total == 0) ? (1 * sizeof(EutelEpgListIndex*)) : ((3 * total + 1) * sizeof(EutelEpgListIndex*));
	if ((data->index = block_mem_mgr_alloc_data(alloc_size, &data->mem_mgr)) == NULL)
	{
		EUTEL_ERR("block_mem_mgr_alloc_data failed!\n");
		goto err;
	}
	data->vindex = data->index + total;
	for (i = 0; i < total; i++)
	{
		richepg_epg_event_info_get_by_pos(i, ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false);
		if ((data->index[i] = block_mem_mgr_alloc_data(sizeof(EutelEpgListIndex), &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_alloc_data failed!\n");
			goto err;
		}
		data->index[i]->event_id = ctrl->use_event->event_id;
		data->index[i]->start_time = ctrl->use_event->start_time + ctrl->zone_sec;
		data->index[i]->finish_time = ctrl->use_event->finish_time + ctrl->zone_sec;
		data->index[i]->parent_rate = ctrl->use_event->parent_rate;
		data->index[i]->genres_num = ctrl->use_event->genres_num;
		if (ctrl->use_event->genres_num > 0 &&
				(data->index[i]->genres_ids = block_mem_mgr_dup_data(
					ctrl->use_event->genres_ids, ctrl->use_event->genres_num * sizeof(int), &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_dup_data failed!\n");
			goto err;
		}

		if (ctrl->use_event->event_title && strlen(ctrl->use_event->event_title) > 0)
			event_title = ctrl->use_event->event_title;
		else
			event_title = app_richepg_translate_str(NO_EVNET_NAME_STR);
		if ((data->index[i]->event_title = block_mem_mgr_dup_str(event_title, &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_dup_data failed!\n");
			goto err;
		}
		app_eutel_epg_event_info_set_flag(data->index[i], data->prog_id, array_sel);
	}
	data->total = total;

	if ((cnt = app_eutel_epg_tmlist_data_get(data, show_start, show_finish, array_sel)) < 0)
		goto err;

	block_mem_mgr_adjust_size(&data->mem_mgr);
	return cnt;
err:
	app_eutel_epg_list_data_free(data);
	return -1;
}

static int app_eutel_epg_filter_event_info_update(EutelEpgListData *data)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	int i = 0, j = 0;
	int total = 0;
	int alloc_size = 0;
	int cnt = 0;
	time_t show_start = 0, show_finish = 0;
	char *event_title = NULL;
	int content_id = 0;

	if (!data)
		return -1;

	app_eutel_epg_list_data_free(data);
	app_eutel_epg_lv_show_time_range_get(&show_start, &show_finish);
	total = richepg_epg_event_total_get(data->channel_id, show_start - ctrl->zone_sec, show_finish - ctrl->zone_sec);
	if (total == 0)
		return -1;

	alloc_size = total * sizeof(EutelEpgListIndex*);
	if ((data->index = block_mem_mgr_alloc_data(alloc_size, &data->mem_mgr)) == NULL)
	{
		EUTEL_ERR("block_mem_mgr_alloc_data failed!\n");
		goto err;
	}

	if (!_is_timelist())
	{
		if (ctrl->content_sel <= ch_ctrl->content_total)
			content_id = ch_ctrl->content_array[ctrl->content_sel-1].id;
	}

	for (i = 0; i < total; i++)
	{
		richepg_epg_event_info_get_by_pos(i, ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false);
		if (content_id)
		{
			if (ctrl->use_event->genres_num)
			{
				for (j = 0; j < ctrl->use_event->genres_num; j++)
				{
					if (ctrl->use_event->genres_ids[j] == content_id)
						break;
				}
				if (j == ctrl->use_event->genres_num)
					continue;
			}
			else
			{
				continue;
			}
		}

		if ((data->index[cnt] = block_mem_mgr_alloc_data(sizeof(EutelEpgListIndex), &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_alloc_data failed!\n");
			goto err;
		}
		data->index[cnt]->event_id = ctrl->use_event->event_id;
		data->index[cnt]->start_time = ctrl->use_event->start_time + ctrl->zone_sec;
		data->index[cnt]->finish_time = ctrl->use_event->finish_time + ctrl->zone_sec;
		data->index[cnt]->parent_rate = ctrl->use_event->parent_rate;
		data->index[cnt]->genres_num = ctrl->use_event->genres_num;
		if (ctrl->use_event->genres_num > 0 &&
				(data->index[cnt]->genres_ids = block_mem_mgr_dup_data(
					ctrl->use_event->genres_ids, ctrl->use_event->genres_num * sizeof(int), &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_dup_data failed!\n");
			goto err;
		}

		if (ctrl->use_event->event_title && strlen(ctrl->use_event->event_title) > 0)
			event_title = ctrl->use_event->event_title;
		else
			event_title = app_richepg_translate_str(NO_EVNET_NAME_STR);
		if ((data->index[cnt]->event_title = block_mem_mgr_dup_str(event_title, &data->mem_mgr)) == NULL)
		{
			EUTEL_ERR("block_mem_mgr_dup_data failed!\n");
			goto err;
		}
		app_eutel_epg_event_info_set_flag(data->index[cnt], data->prog_id, ctrl->cur_sel);
		++cnt;
	}
	data->total = cnt;

	block_mem_mgr_adjust_size(&data->mem_mgr);
	return cnt;
err:
	app_eutel_epg_list_data_free(data);
	return -1;
}

static int app_eutel_epg_epg_info_update(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	int i = 0, first_sel = 0, tmp_sel = 0;

	app_eutel_epg_list_data_release_all();
	first_sel = ctrl->prog_sel - ctrl->cur_sel;

	if (ctrl->prog_sel < 0 || ctrl->cur_sel < 0)
		return -1;
	if (ch_ctrl->use_total == 0)
		return -1;

	if (_is_timelist())
	{
		for (i = 0; i < EUTEL_EPG_MAX_PROG; i++)
		{
			tmp_sel = first_sel + i;
			if (tmp_sel < ch_ctrl->use_total)
			{
				arg = &ch_ctrl->arg_array[ch_ctrl->use_array[tmp_sel].pos];
				ctrl->data[i].prog_id = arg->prog_id;
				ctrl->data[i].channel_id = arg->channel_id;
				if (tmp_sel != ctrl->prog_sel)
					app_eutel_epg_event_info_update(&ctrl->data[i], i);
			}
		}

		if (ctrl->prog_sel < ch_ctrl->use_total)
		{
			app_eutel_epg_event_info_update(&ctrl->data[ctrl->cur_sel], ctrl->cur_sel);
			if (richepg_epg_event_info_get_cur(ctrl->data[ctrl->cur_sel].channel_id,
						ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false) == 0)
				ctrl->next_event_sec = ctrl->use_event->finish_time + ctrl->zone_sec;
			else if (richepg_epg_event_info_get_next(ctrl->data[ctrl->cur_sel].channel_id,
						ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false) == 0)
				ctrl->next_event_sec = ctrl->use_event->start_time + ctrl->zone_sec;
			else
				ctrl->next_event_sec = 0;
		}
		else
		{
			ctrl->next_event_sec = 0;
		}
	}
	else
	{
		tmp_sel = ctrl->prog_sel;
		if (tmp_sel < ch_ctrl->use_total)
		{
			arg = &ch_ctrl->arg_array[ch_ctrl->use_array[tmp_sel].pos];
			ctrl->lv_data.prog_id = arg->prog_id;
			ctrl->lv_data.channel_id = arg->channel_id;
			app_eutel_epg_filter_event_info_update(&ctrl->lv_data);
			if (richepg_epg_event_info_get_cur(ctrl->lv_data.channel_id,
						ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false) == 0)
				ctrl->next_event_sec = ctrl->use_event->finish_time + ctrl->zone_sec;
			else if (richepg_epg_event_info_get_next(ctrl->lv_data.channel_id,
						ctrl->use_event, EUTEL_EPG_EVENT_SIZE, false) == 0)
				ctrl->next_event_sec = ctrl->use_event->start_time + ctrl->zone_sec;
			else
				ctrl->next_event_sec = 0;
		}
		else
		{
			ctrl->next_event_sec = 0;
		}
	}

	return 0;
}

static int app_eutel_epg_set_tmlist_start_time(const char* str, time_t sec)
{
	if (str)
	{
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "start_time", (void*)str);
		return 0;
	}

	char* tm_str = NULL;
	tm_str = app_time_to_hourmin_edit_str(sec);
	if (tm_str != NULL)
	{
		char tm_set_str[8] = {0};

		tm_set_str[0] = '<';
		memcpy(tm_set_str + 1, tm_str, 5);
		tm_set_str[6] = '>';
		tm_set_str[7] = '\0';
		GxCore_Free(tm_str);
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "start_time", (void*)tm_set_str);

		return 0;
	}
	return -1;
}

static void app_eutel_epg_set_event_date(time_t sec)
{
	char *tm_str = app_richepg_date_str_get(sec, false);
	GUI_SetProperty(s_eutel_epg_wgt.epg_date, "string", tm_str);
}

static void app_eutel_epg_show_sys_time_date(void)
{
	app_show_sys_time_and_date(s_eutel_epg_wgt.sys_time, s_eutel_epg_wgt.sys_date);
}

static void app_eutel_epg_prog_info_update(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	char *tmp_str = NULL;

	if (ctrl->prog_sel >= 0 && ctrl->prog_sel < ch_ctrl->use_total)
		arg = &ch_ctrl->arg_array[ch_ctrl->use_array[ctrl->prog_sel].pos];

	if (arg)
	{
		tmp_str = ch_ctrl->arg_array[ch_ctrl->use_array[ctrl->prog_sel].pos].channel_name;
		if (!tmp_str || strlen(tmp_str) == 0)
			tmp_str = STR_ID_NO_CH;
		app_set_widget_string(s_eutel_epg_wgt.ch_name, tmp_str);
		tmp_str = eutel_array_name_get(arg->type_num, arg->type_ids, EUTEL_CHANNEL_TYPE);
		app_set_widget_string(s_eutel_epg_wgt.ch_type, tmp_str);
		tmp_str = eutel_array_name_get(arg->genre_num, arg->genre_ids, EUTEL_CHANNEL_GENRE);
		app_set_widget_string(s_eutel_epg_wgt.ch_genres, tmp_str);
	}
	else
	{
		tmp_str = STR_ID_NO_CH;
		app_set_widget_string(s_eutel_epg_wgt.ch_name, tmp_str);
		app_set_widget_string(s_eutel_epg_wgt.ch_type, "");
		app_set_widget_string(s_eutel_epg_wgt.ch_genres, "");
	}
}

static EutelEpgListIndex *app_eutel_epg_focus_index_get(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListData *data;
	int sel = 0;

	if (_is_timelist())
	{
		data = &ctrl->data[ctrl->cur_sel];
		GUI_GetProperty(s_eutel_epg_wgt.tl_event, "select_column", &sel);
		if (sel < 0 || sel >= data->vtotal)
			return NULL;
		return data->vindex[sel];
	}
	else
	{
		data = &ctrl->lv_data;
		GUI_GetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
		if (sel < 0 || sel >= data->total)
			return NULL;
		return data->index[sel];
	}
}

static void app_eutel_epg_brief_info_clear(void)
{
	const char *wnd = NULL;
	wnd = GUI_GetFocusWindow();
	if (wnd != NULL && strcmp(wnd, s_eutel_epg_wgt.wnd) == 0)
	{
		GUI_SetProperty(s_eutel_epg_wgt.event_title, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_epg_wgt.event_genres, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_epg_wgt.event_parent, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_epg_wgt.event_duration, "string", STR_ID_BLANK);
		GUI_SetProperty(s_eutel_epg_wgt.event_logo, "img", EUTEL_EPG_EVENT_KEY_DF);
	}
}

static void app_eutel_epg_brief_info_show(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListIndex *focus_index = NULL;

	char buffer[8] = {0};
	char *tmp_str = NULL;
	char *logo_key = EUTEL_EPG_EVENT_KEY_DF;

	focus_index = app_eutel_epg_focus_index_get();
	if (!focus_index)
		return;

	if (focus_index->finish_time > (ctrl->local_day + ctrl->tmlist_start_sec))
	{
		GUI_SetProperty(s_eutel_epg_wgt.event_title, "string", focus_index->event_title);

		tmp_str = eutel_array_name_get(focus_index->genres_num, focus_index->genres_ids, EUTEL_CONTENT_GENRE);
		GUI_SetProperty(s_eutel_epg_wgt.event_genres, "string", tmp_str);

		if (focus_index->parent_rate > 0)
		{
			snprintf(buffer, sizeof(buffer), "%d", focus_index->parent_rate);
			GUI_SetProperty(s_eutel_epg_wgt.event_parent, "string", buffer);
		}
		else
		{
			GUI_SetProperty(s_eutel_epg_wgt.event_parent, "string", STR_ID_BLANK);
		}

		tmp_str = app_richepg_duration_str_get(focus_index->start_time, focus_index->finish_time, 0);
		GUI_SetProperty(s_eutel_epg_wgt.event_duration, "string", tmp_str);

		tmp_str = NULL;
		if ((focus_index->event_flag & INVALID_EPG_FLAG) == 0)
		{
			if (richepg_epg_event_info_get_by_id(focus_index->event_id,
						ctrl->focus_event, EUTEL_EPG_EVENT_SIZE, true) == 0)
			{
				tmp_str = ctrl->focus_event->event_logo;
			}
		}
		if (tmp_str)
		{
			char *logo_path = richepg_get_logo_path(tmp_str, RICHEPG_LOGO_EVENT);
			if (logo_path) {
				gal_add_key_path(EUTEL_EPG_EVENT_KEY, logo_path);
				logo_key = EUTEL_EPG_EVENT_KEY;
			}
		}
		GUI_SetProperty(s_eutel_epg_wgt.event_logo, "img", logo_key);
	}
}

static void app_eutel_epg_event_details_hide(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	ctrl->detail_show = false;
	GUI_SetProperty(s_eutel_epg_wgt.details_duration, "string", STR_ID_BLANK);
	GUI_SetProperty(s_eutel_epg_wgt.details_title, "string", STR_ID_BLANK);
	GUI_SetProperty(s_eutel_epg_wgt.details_info, "string", STR_ID_BLANK);
	GUI_SetProperty(s_eutel_epg_wgt.details_duration, "state", "hide");
	GUI_SetProperty(s_eutel_epg_wgt.details_title, "state", "hide");
	GUI_SetProperty(s_eutel_epg_wgt.details_info, "state", "hide");
	GUI_SetProperty(s_eutel_epg_wgt.details_exit, "state", "hide");
	GUI_SetProperty(s_eutel_epg_wgt.details_back, "state", "hide");
	if (_is_timelist())
	{
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "update_all", NULL);
		_set_focus_widget(s_eutel_epg_wgt.tl_event);
	}
	else
	{
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "update_all", NULL);
		_set_focus_widget(s_eutel_epg_wgt.lv_event);
	}
}

static void app_eutel_epg_event_details_show(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListIndex *focus_index = NULL;
	char *tmp_str = NULL;
	char *info_str = NULL;

	focus_index = app_eutel_epg_focus_index_get();
	if (!focus_index)
		return;

	s_eutel_epg_ctrl.detail_show = true;
	GUI_SetProperty(s_eutel_epg_wgt.details_back, "state", "show");
	GUI_SetProperty(s_eutel_epg_wgt.details_duration, "state", "show");
	GUI_SetProperty(s_eutel_epg_wgt.details_title, "state", "show");
	GUI_SetProperty(s_eutel_epg_wgt.details_info, "state", "show");
	GUI_SetProperty(s_eutel_epg_wgt.details_exit, "state", "show");
	_set_focus_widget(s_eutel_epg_wgt.details_exit);

	if (focus_index->finish_time > (ctrl->local_day + ctrl->tmlist_start_sec))
	{
		tmp_str = app_richepg_duration_str_get(focus_index->start_time, focus_index->finish_time, 0);
		GUI_SetProperty(s_eutel_epg_wgt.details_duration, "string", tmp_str);
		GUI_SetProperty(s_eutel_epg_wgt.details_title, "string", focus_index->event_title);
		if ((focus_index->event_flag & INVALID_EPG_FLAG) == 0)
		{
			if (richepg_epg_event_info_get_by_id(focus_index->event_id,
						ctrl->focus_event, EUTEL_EPG_EVENT_SIZE, true) == 0)
			{
				info_str = ctrl->focus_event->event_desc;
                if(NULL == info_str || (info_str && 0 == strlen(info_str)))
                {
                    int channel_id = ctrl->data[ctrl->cur_sel].channel_id;
                    if(0 == richepg_epg_event_info_get_by_time_from_usb(ctrl->focus_event->start_time, channel_id, focus_index->event_id, ctrl->focus_event, EUTEL_EPG_EVENT_SIZE))
                        info_str = ctrl->focus_event->event_desc;
                }
			}
		}
		if (!info_str || strlen(info_str) == 0)
			info_str = app_richepg_epg_no_info_str_get();
		GUI_SetProperty(s_eutel_epg_wgt.details_info, "string", info_str);
	}
}

static void app_eutel_epg_event_details_keypress(uint16_t key)
{
	uint32_t value = 1;

	switch(key)
	{
		case STBK_UP:
			GUI_SetProperty(s_eutel_epg_wgt.details_info, "line_up", &value);
			break;
		case STBK_DOWN:
			GUI_SetProperty(s_eutel_epg_wgt.details_info, "line_down", &value);
			break;
		case STBK_PAGE_UP:
			GUI_SetProperty(s_eutel_epg_wgt.details_info, "page_up", &value);
			break;
		case STBK_PAGE_DOWN:
			GUI_SetProperty(s_eutel_epg_wgt.details_info, "page_down", &value);
			break;
		default:
			break;
	}
}

static int app_eutel_epg_time_update_exec(void* usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	const char *wnd = NULL;

	wnd = GUI_GetFocusWindow();
	if(wnd != NULL && strcmp(wnd, s_eutel_epg_wgt.wnd) == 0)
		app_eutel_epg_show_sys_time_date();

	time_t local_sec = _utc_time_get() + ctrl->zone_sec;
	time_t local_hour = local_sec - local_sec % SEC_PER_HOUR;
	time_t local_day = local_hour - local_hour % SEC_PER_DAY;

	if (local_day != ctrl->local_day)
	{
		ctrl->update_flag = true;
		if (ctrl->day_sel != 0)
			ctrl->day_sel--;
		else
			ctrl->tmlist_start_sec = local_hour % SEC_PER_DAY;
	}
	else if (local_hour != ctrl->local_hour
			|| (local_sec >= ctrl->next_event_sec && 0 < ctrl->next_event_sec))
	{
		ctrl->update_flag = true;
	}

	if (0 == ctrl->day_sel)
	{
		if ((local_sec >= ctrl->next_event_sec && 0 < ctrl->next_event_sec)
				|| (local_hour != ctrl->local_hour && 0 < ctrl->next_event_sec))
			ctrl->tmlist_start_sec = local_hour % SEC_PER_DAY;
	}

	ctrl->local_sec = local_sec;
	ctrl->local_hour = local_hour;
	ctrl->local_day = local_day;

	return 0;
}

static int app_eutel_epg_show_update_exec(void* usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	int sel = 0;

	if (ctrl->update_flag && !ctrl->detail_show)
	{
		const char *wnd = NULL;
		wnd = GUI_GetFocusWindow();
		if (wnd != NULL && strcmp(wnd, s_eutel_epg_wgt.wnd) == 0)
		{
			ctrl->update_flag = false;
			app_eutel_epg_set_event_date(ctrl->local_day + (ctrl->day_sel * SEC_PER_DAY));
			app_eutel_epg_epg_info_update();

			if (_is_timelist())
			{
				if (ctrl->day_sel == 0)
					app_eutel_epg_set_tmlist_start_time(NULL, ctrl->tmlist_start_sec);
				GUI_SetProperty(s_eutel_epg_wgt.tl_event, "update_all", NULL);
			}
			else
			{
				GUI_SetProperty(s_eutel_epg_wgt.lv_event, "update_all", NULL);
				GUI_SetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
			}
		}
	}
	return 0;
}

static int app_eutel_epg_focus_update_exec(void* usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListIndex *focus_index = NULL;

	if (!ctrl->update_flag && !ctrl->detail_show)
	{
		const char *wnd = NULL;
		wnd = GUI_GetFocusWindow();
		if (wnd != NULL && strcmp(wnd, s_eutel_epg_wgt.wnd) == 0)
		{
			focus_index = app_eutel_epg_focus_index_get();
			if (!focus_index)
			{
				if (ctrl->focus_time_bak != 0)
				{
					ctrl->focus_time_bak = 0;
					app_eutel_epg_brief_info_clear();
				}
			}
			else if (ctrl->focus_time_bak != focus_index->start_time)
			{
				ctrl->focus_time_bak = focus_index->start_time;
				if (ctrl->focus_time_bak == 0)
				{
					app_eutel_epg_brief_info_clear();
				}
				else
				{
					app_eutel_epg_brief_info_show();
				}
			}

			if(ctrl->focus_time_bak < ctrl->local_sec || (focus_index->event_flag & INVALID_EPG_FLAG))
			{
				GUI_SetProperty(s_eutel_epg_wgt.img_book, "state", "hide");
				GUI_SetProperty(s_eutel_epg_wgt.text_book, "state", "hide");
			}
			else
			{
				GUI_SetProperty(s_eutel_epg_wgt.img_book, "state", "show");
				GUI_SetProperty(s_eutel_epg_wgt.text_book, "state", "show");
				if((focus_index->event_flag & PLAY_EPG_FLAG) || (focus_index->event_flag & PVR_EPG_FLAG))
				{
					app_set_widget_string(s_eutel_epg_wgt.text_book, STR_ID_DELETE);
				}
				else
				{
					app_set_widget_string(s_eutel_epg_wgt.text_book, STR_ID_BOOK);
				}
			}
		}
	}

	return 0;
}

int app_eutel_epg_timer_rm(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	APP_TIMER_REMOVE(ctrl->time_update);
	APP_TIMER_REMOVE(ctrl->show_update);
	APP_TIMER_REMOVE(ctrl->focus_update);
	return 0;
}

int app_eutel_epg_timer_add(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;

	APP_TIMER_ADD(ctrl->time_update, app_eutel_epg_time_update_exec, 800, TIMER_REPEAT);
	APP_TIMER_ADD(ctrl->show_update, app_eutel_epg_show_update_exec, 30, TIMER_REPEAT);
	APP_TIMER_ADD(ctrl->focus_update, app_eutel_epg_focus_update_exec, 500, TIMER_REPEAT);

	return 0;
}

static int app_eutel_epg_show_update_immediately(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	ctrl->update_flag = true;
	app_eutel_epg_show_update_exec(NULL);
	return 0;
}

static GxBook thiz_book;
void app_eutel_epg_event_timer_set(bool bflag)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	static bool s_epg_event_timer = false;

	if(s_epg_event_timer == true) // app_timer_edit.c use this
	{
		GUI_SetProperty(s_eutel_epg_wgt.text_book, "string", STR_ID_DELETE);
		if (_is_timelist())
		{
			ctrl->update_flag = true;
		}
	}
	s_epg_event_timer = bflag;
}

static int _eutel_epg_book_pop_cb(PopDlgRet ret)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	int sel = 0;

	if(POP_VAL_OK == ret)
	{
		g_AppBook.remove(&thiz_book);
		GUI_SetProperty(s_eutel_epg_wgt.text_book, "string", STR_ID_BOOK);
		if (_is_timelist())
		{
			ctrl->update_flag = true;
		}
		else
		{
			GUI_GetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
			if (sel >= 0 && sel < ctrl->lv_data.total)
			{
				app_eutel_epg_event_info_set_flag(ctrl->lv_data.index[sel], ctrl->lv_data.prog_id, ctrl->cur_sel);
				GUI_GetProperty(s_eutel_epg_wgt.lv_event, "update_row", &sel);
			}
		}
	}

	return 0;
}

static void app_eutel_epg_timer_edit_exit_callback(void)
{
    if(GUI_CheckDialog(s_eutel_epg_wgt.wnd) != GXCORE_SUCCESS)
        return;
    app_unset_window_back_ground(s_eutel_epg_wgt.wnd, true, false);
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppFullArb.timer_start();
}

static void app_eutel_epg_book_control(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	EutelEpgListIndex *focus_index = NULL;
	char *prog_name = NULL;
	PopDlg pop = {0};

	focus_index = app_eutel_epg_focus_index_get();
	if (!focus_index
			|| (focus_index->event_flag & INVALID_EPG_FLAG)
			|| (focus_index->start_time < ctrl->local_sec))
		return;

	memset(&thiz_book, 0, sizeof(GxBook));
	if(app_get_same_start_play_or_pvr_book(ctrl->cur_prog.id, focus_index->start_time - ctrl->zone_sec, &thiz_book)
			&& (focus_index->start_time >= ctrl->local_sec))
	{
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
		pop.mode = POP_MODE_UNBLOCK;
		pop.str = STR_ID_SURE_DELETE;
		pop.exit_cb = _eutel_epg_book_pop_cb;
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
			pop.timeout_sec = 3;
			popdlg_create(&pop);

			return;
		}

		g_AppPlayOps.program_stop();

		if (ctrl->prog_sel >= 0 && ctrl->prog_sel < ch_ctrl->use_total)
			prog_name = ch_ctrl->arg_array[ch_ctrl->use_array[ctrl->prog_sel].pos].channel_name;
		if (!prog_name || strlen(prog_name) == 0)
			prog_name = (char*)ctrl->cur_prog.prog_name;

		app_timer_edit_ex_func_register(app_eutel_epg_timer_edit_exit_callback);
		app_epg_timer_menu_exec(focus_index->start_time - ctrl->zone_sec,
				focus_index->finish_time - focus_index->start_time,
				ctrl->cur_prog.id, prog_name);

/*
		if(GUI_CheckDialog(s_eutel_epg_wgt.wnd) != GXCORE_SUCCESS)
			return;
		app_unset_window_back_ground(s_eutel_epg_wgt.wnd, true, false);
		g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
		g_AppFullArb.timer_start();
*/
	}
}

static void app_eutel_epg_change_show_day(bool next_day_flag)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	int sel = 0;

	if(next_day_flag)
		(ctrl->day_sel == EUTEL_EPG_MAX_DAY - 1) ? (ctrl->day_sel = 0) : (ctrl->day_sel++);
	else
		(ctrl->day_sel == 0) ? (ctrl->day_sel =  EUTEL_EPG_MAX_DAY - 1) : (ctrl->day_sel--);

	app_eutel_epg_set_event_date(ctrl->local_day + (ctrl->day_sel * EUTEL_EPG_MAX_DAY));

	if (_is_timelist())
	{
#if 0
		if(ctrl->day_sel != 0)
		{
			ctrl->tmlist_start_sec = 0;
			app_eutel_epg_set_tmlist_start_time("<00:00>", 0);
		}
		else
		{
			ctrl->tmlist_start_sec = ctrl->local_hour % SEC_PER_DAY;
			app_eutel_epg_set_tmlist_start_time(NULL, ctrl->tmlist_start_sec);
		}
#else
		if(ctrl->day_sel == 0)
		{
			if (ctrl->tmlist_start_sec < ctrl->local_hour % SEC_PER_DAY)
				ctrl->tmlist_start_sec = ctrl->local_hour % SEC_PER_DAY;
		}
		app_eutel_epg_set_tmlist_start_time(NULL, ctrl->tmlist_start_sec);
#endif
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "update_all", NULL);
		GUI_SetProperty(s_eutel_epg_wgt.event_row[ctrl->cur_sel], "current_item", &sel);
	}
	else
	{
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "update_all", NULL);
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
	}
	app_eutel_epg_show_update_immediately();
}

static int _eutel_epg_prog_list_change(GuiWidget *widget, void *usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	int active_sel = -1;

	APP_TIMER_REMOVE(ctrl->focus_update);
	ctrl->focus_time_bak = -1;
	GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &ctrl->prog_sel);
	GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "cur_sel", &ctrl->cur_sel);
	if ((ctrl->cur_sel < 0 && ctrl->cur_sel >= EUTEL_EPG_MAX_PROG)
			|| ch_ctrl->use_total == 0)
		ctrl->cur_sel = 0;
	if (_is_timelist())
	{
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "select_row", s_eutel_epg_wgt.event_row[ctrl->cur_sel]);
	}
	active_sel = ctrl->prog_sel % EUTEL_EPG_MAX_PROG;
	GUI_SetProperty(s_eutel_epg_wgt.lv_point, "active", &active_sel);

	memset(&ctrl->cur_prog, 0, sizeof(ctrl->cur_prog));
	if (ctrl->prog_sel >= 0 && ctrl->prog_sel < ch_ctrl->use_total)
	{
		GxBus_PmProgGetById(ch_ctrl->arg_array[ch_ctrl->use_array[ctrl->prog_sel].pos].prog_id, &ctrl->cur_prog);
	}

	app_eutel_epg_prog_info_update();
	app_eutel_epg_epg_info_update();
	if (_is_timelist())
	{
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "update_all", NULL);
	}
	else
	{
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "update_all", NULL);
	}
	APP_TIMER_ADD(ctrl->focus_update, app_eutel_epg_focus_update_exec, 500, TIMER_REPEAT);

	return EVENT_TRANSFER_STOP;
}

static void app_eutel_epg_exit(void)
{
	GUI_EndDialog(s_eutel_epg_wgt.wnd);
	GUI_SetInterface("flush", NULL);
	app_eutel_chinfo_create_dialog();
}

static int _eutel_epg_ok_keypress(pvr_state cur_pvr)
{
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	int sel = 0;
	int lcn = 0;

	if (ctrl->use_total == 0)
		return -1;
	GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
	if (sel >= ctrl->use_total || sel < 0)
		return -1;

	ctrl->filter_sel_bak = ctrl->filter_sel;
	lcn = ctrl->use_array[sel].lcn;

	if ((lcn != ctrl->cur_tv_lcn && 0 == ctrl->cur_stream_type)
			|| (lcn != ctrl->cur_ra_lcn && 1 == ctrl->cur_stream_type)
			|| g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK
			|| g_AppFullArb.state.pause == STATE_ON)
	{
		g_AppFullArb.state.pause = STATE_OFF;
		g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
		eutel_channel_play_by_lcn(PLAY_MODE_POINT, lcn);
	}
	else
	{
		EutelEpgCtrl *epg_ctrl = &s_eutel_epg_ctrl;
		if(epg_ctrl->detail_show)
			app_eutel_epg_event_details_hide();
		app_eutel_epg_exit();
	}

	return 0;
}

static int _eutel_epg_stop_pvr_cb(PopDlgRet ret)
{
	pvr_state cur_pvr = app_pvr_get_state();

	if (POP_VAL_OK == ret)
	{
		if(PVR_TIMESHIFT == cur_pvr)
		{
			app_pvr_tms_stop();
		}
		else
		{
			app_pvr_stop();
		}
		_eutel_epg_ok_keypress(cur_pvr);
	}
	return 0;
}

static void app_eutel_epg_change_show_style(void)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	char *content_name = NULL;

	if (ctrl->content_sel > ch_ctrl->content_total)
	{
		ctrl->content_sel = 0;
	}

	if (_is_timelist())
	{
		GUI_SetProperty(s_eutel_epg_wgt.ct_image, "state", "hide");
		GUI_SetProperty(s_eutel_epg_wgt.ct_filter, "state", "hide");
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "state", "hide");
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "state", "show");
		_set_focus_widget(s_eutel_epg_wgt.tl_event);
		app_eutel_epg_set_tmlist_start_time(NULL, ctrl->tmlist_start_sec);
	}
	else
	{
		content_name = ch_ctrl->content_array[ctrl->content_sel-1].name;
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "state", "hide");
		GUI_SetProperty(s_eutel_epg_wgt.ct_image, "state", "show");
		GUI_SetProperty(s_eutel_epg_wgt.ct_filter, "state", "show");
		GUI_SetProperty(s_eutel_epg_wgt.ct_filter, "string", content_name);
		GUI_SetProperty(s_eutel_epg_wgt.lv_event, "state", "show");
		_set_focus_widget(s_eutel_epg_wgt.lv_prog);
	}

	app_eutel_epg_brief_info_clear();
	app_eutel_epg_show_sys_time_date();
	app_eutel_epg_set_event_date(ctrl->local_day + (ctrl->day_sel * SEC_PER_DAY));
	GUI_SetProperty(s_eutel_epg_wgt.img_book, "state", "hide");
	GUI_SetProperty(s_eutel_epg_wgt.text_book, "state", "hide");
}

int app_eutel_epg_plug_in_out(void)
{
	if (GUI_CheckDialog(WND_EUTEL_EPG) == GXCORE_SUCCESS
			|| GUI_CheckDialog(WND_EUTEL_EPG2) == GXCORE_SUCCESS)
        //	|| GUI_CheckDialog(WND_EUTEL_REPG) == GXCORE_SUCCESS
		//	|| GUI_CheckDialog(WND_EUTEL_REPG2) == GXCORE_SUCCESS
	{
		PopDlg pop = {0};
		bool replay = false;
		extern int app_pop_list_end_dialog(void);

		popdlg_destroy();
		app_pop_list_end_dialog();
		if (GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS)
		{
			replay = true;
			GUI_EndDialog(WND_SYSTEM_SETTING);
			app_system_set_destroy(EXIT_ABANDON);
		}
		if (GUI_CheckDialog(WND_EUTEL_EPG) == GXCORE_SUCCESS)
			GUI_EndDialog(WND_EUTEL_EPG);
		//if (GUI_CheckDialog(WND_EUTEL_REPG) == GXCORE_SUCCESS)
		//	GUI_EndDialog(WND_EUTEL_REPG);
		if (GUI_CheckDialog(WND_EUTEL_EPG2) == GXCORE_SUCCESS)
			GUI_EndDialog(WND_EUTEL_EPG2);
		//if (GUI_CheckDialog(WND_EUTEL_REPG2) == GXCORE_SUCCESS)
			//GUI_EndDialog(WND_EUTEL_REPG2);
		if (replay)
		{
			eutel_channel_play_current(PLAY_MODE_POINT);
			g_AppFullArb.timer_start();
		}
		else
		{
			app_eutel_chinfo_create_dialog();
		}
		pop.type = POP_TYPE_NO_BTN;
		pop.format = POP_FORMAT_DLG;
		pop.str = STR_ID_EPG_GENERATING_TIP;
		pop.mode = POP_MODE_UNBLOCK;
		pop.timeout_sec = 3;
		popdlg_create(&pop);
	}

	return 0;
}

static int _eutel_epg_num_reponse(int num_value)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	int filter_sel = ch_ctrl->filter_sel;
	int orig_sel = 0;
	int sel = ch_ctrl->use_total - 1;
	int i = 0;

	GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &orig_sel);
	for (i = 0; i < ch_ctrl->use_total; i++)
	{
		if (ch_ctrl->use_array[i].lcn == num_value)
		{
			sel = i;
			if (orig_sel != sel)
			{
				if(ctrl->detail_show)
					app_eutel_epg_event_details_hide();
				GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
			}
			return 0;
		}
	}

	if (filter_sel != 0)
	{
		if (ctrl->detail_show)
			app_eutel_epg_event_details_hide();
		eutel_channel_group_all_build();
		eutel_channel_group_title(s_eutel_epg_wgt.ch_filter);
		GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "update_all", NULL);
	}

	sel = ch_ctrl->use_total - 1;
	for (i = 0; i < ch_ctrl->use_total; i++)
	{
		if (ch_ctrl->use_array[i].lcn == num_value)
		{
			sel = i;
			break;
		}
		else if (ch_ctrl->use_array[i].lcn > num_value)
		{
			if (i == 0)
			{
				sel = i;
			}
			else
			{
				if (ch_ctrl->use_array[i].lcn - num_value < num_value - ch_ctrl->use_array[i-1].lcn)
					sel = i;
				else
					sel = i - 1;
			}
			break;
		}
	}

	if (orig_sel != sel || filter_sel != 0)
	{
		if(ctrl->detail_show)
			app_eutel_epg_event_details_hide();
		GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
	}

	return 0;
}

static int _eutel_epg_create(GuiWidget *widget, void *usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	int sel = 0, tmp_sel = 0;

	ch_ctrl->filter_sel_bak = ch_ctrl->filter_sel;
	eutel_channel_group_title(s_eutel_epg_wgt.ch_filter);
	sel = eutel_channel_play_use_sel_get();
	if (ch_ctrl->use_total > 0 && sel < 0)
		sel = 0;

	app_eutel_epg_change_show_style();
	GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &tmp_sel);
	if (tmp_sel != sel)
		GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
	else
		_eutel_epg_prog_list_change(NULL, NULL);
	ctrl->update_flag = true;
	app_number_response_cb_set(_eutel_epg_num_reponse);

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_destroy(GuiWidget *widget, void *usrdata)
{
	app_number_response_cb_set(NULL);
	app_eutel_epg_ctrl_release();
	app_richepg_chfilter_keep_sec_check_set();
	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_got_focus(GuiWidget *widget, void *usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	int sel = 0;

	app_eutel_epg_event_timer_set(false);
	if (_is_timelist())
	{
		GUI_SetProperty(s_eutel_epg_wgt.tl_event, "update_all", NULL);
	}
	else
	{
		GUI_GetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
		if (sel >= 0 && sel < ctrl->lv_data.total)
		{
			app_eutel_epg_event_info_set_flag(ctrl->lv_data.index[sel], ctrl->lv_data.prog_id, ctrl->cur_sel);
			GUI_GetProperty(s_eutel_epg_wgt.lv_event, "update_row", &sel);
		}
	}

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	EutelChannelArg *arg = NULL;
	int sel = 0, tmp_sel = 0, orig_sel = 0;
	GUI_Event *event = NULL;
	GxMsgProperty_NodeByIdGet node_prog = {0};
	unsigned int key = 0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			key = find_virtualkey_ex(event->key.scancode,event->key.sym);
			switch(key)
			{
				case VK_BOOK_TRIGGER:
					if(ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					GUI_EndDialog(s_eutel_epg_wgt.wnd);
					break;
				case STBK_EXIT:
				case STBK_MENU:
					if(ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					else
						app_eutel_epg_exit();
					break;
				case STBK_EPG:
					GUI_EndDialog(s_eutel_epg_wgt.wnd);
					app_eutel_epg2_create_dialog();
					break;

				case STBK_UP:
				case STBK_DOWN:
				case STBK_PAGE_UP:
				case STBK_PAGE_DOWN:
					if(ctrl->detail_show)
						app_eutel_epg_event_details_keypress(key);
					break;

				case STBK_INFO:
					if(!ctrl->detail_show)
						app_eutel_epg_event_details_show();
					break;
				case STBK_FAV:
#ifndef RADIO_REC_BOOK
					if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
						break;
#endif
					if (ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					app_eutel_epg_book_control();
					break;

				case STBK_OK:
					if (ch_ctrl->use_total == 0)
						break;
					GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
					if (sel >= ch_ctrl->use_total || sel < 0)
						break;
					arg = &ch_ctrl->arg_array[ch_ctrl->use_array[sel].pos];

					node_prog.node_type = NODE_PROG;
					node_prog.id = arg->prog_id;
					app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_prog);

#if SAT2IP_SERVER_SUPPORT
					if (app_sat2ip_switch_tip_get_by_prog(&node_prog.prog_data) < 0)
						break;
#endif
					if(DEALWITH_KEEPON == app_pvr_popdlg_handler(_eutel_epg_stop_pvr_cb))
					{
						_eutel_epg_ok_keypress(app_pvr_get_state());
					}
					break;

				case STBK_RED:
				case STBK_GREEN:
					if (ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					if (key == STBK_RED)
						app_eutel_epg_change_show_day(false);
					else
						app_eutel_epg_change_show_day(true);
					break;
				case STBK_YELLOW:
					if (ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					app_eutel_chlist_filter_pop(s_eutel_epg_wgt.ch_filter);
					GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "update_all", NULL);
					sel = eutel_channel_play_use_sel_get();
					if (ch_ctrl->use_total > 0 && sel < 0)
						sel = 0;
					GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &tmp_sel);
					if (tmp_sel != sel)
						GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
					else
						_eutel_epg_prog_list_change(NULL, NULL);
					break;
				case STBK_BLUE:
					orig_sel = ctrl->content_sel;
					if (ctrl->detail_show)
						app_eutel_epg_event_details_hide();
					ctrl->content_sel = app_eutel_evlist_filter_pop(s_eutel_epg_wgt.ct_filter, orig_sel);
					if (ctrl->content_sel == orig_sel)
						break;
					if ((orig_sel == 0 && ctrl->content_sel > 0)
							|| (orig_sel > 0 && ctrl->content_sel == 0))
					{
						app_eutel_epg_change_show_style();
						_eutel_epg_prog_list_change(NULL, NULL);
						ctrl->update_flag = true;
					}
					else
					{
						ctrl->update_flag = true;
					}
					break;
				case STBK_1:
				case STBK_2:
				case STBK_3:
				case STBK_4:
				case STBK_5:
				case STBK_6:
				case STBK_7:
				case STBK_8:
				case STBK_9:
				case STBK_0:
					GUI_EndDialog(WND_VOLUME);
					GUI_CreateDialog(WND_NUMBER);
					GUI_SendEvent(WND_NUMBER, event);
					break;
				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static int _eutel_epg_prog_list_get_total(GuiWidget *widget, void *usrdata)
{
	int total = 0;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	total = ctrl->use_total;
	return total;
}

static int _eutel_epg_prog_list_get_data(GuiWidget *widget, void *usrdata)
{
#define EUTEL_CH_KEY_STR_LEN    32
#define EUTEL_CH_LCN_STR_LEN    6
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	static char lcn_str[EUTEL_CH_LCN_STR_LEN];
    char *logo = NULL;

	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if (item == NULL || item->sel >= ctrl->use_total || item->sel < 0)
		return GXCORE_ERROR;

	snprintf(lcn_str, EUTEL_CH_LCN_STR_LEN, "%04d", ctrl->use_array[item->sel].lcn);

	//col-0: channel logo
	item->zoom = true;
    if(NULL == (logo = eutel_channel_logo_key_get(item->sel)))
    {
        item->string = lcn_str;
        item->x_offset = 5;
    }
    else
    {
        item->x_offset = 0;
        item->string = NULL;
    }
    item->image = logo;

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_prog_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	EutelChannelCtrl *ctrl = &s_eutel_ch_ctrl;
	GUI_Event *event = NULL;
	int sel = 0;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_PAGE_UP:
					if (ctrl->use_total == 0)
					{
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
					if (sel == 0)
					{
						sel = ctrl->use_total - 1;
						GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;

				case STBK_PAGE_DOWN:
					if (ctrl->use_total == 0)
					{
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					GUI_GetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
					if (ctrl->use_total == sel+1)
					{
						sel = 0;
						GUI_SetProperty(s_eutel_epg_wgt.lv_prog, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					if (s_eutel_epg_ctrl.lv_data.total > 0)
					{
						_set_focus_widget(s_eutel_epg_wgt.lv_event);
					}
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

static int _eutel_epg_event_list_get_total(GuiWidget *widget, void *usrdata)
{
	if (_is_timelist())
		return 0;

	int total = 0;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	total = ctrl->lv_data.total;
	return total;
}

static int _eutel_epg_event_list_get_data(GuiWidget *widget, void *usrdata)
{
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListIndex *arg = NULL;

	if (_is_timelist())
		return EVENT_TRANSFER_STOP;

	ListItemPara* item = NULL;
	item = (ListItemPara*)usrdata;
	if (item == NULL || item->sel >= ctrl->lv_data.total || item->sel < 0)
		return GXCORE_ERROR;
	arg = ctrl->lv_data.index[item->sel];

	//col-0: state logo
	item->x_offset = 0;
	item->string = NULL;
	if (arg->event_flag & CUR_EPG_FLAG)
		item->image = "s_ts_blue";
	else if (arg->event_flag & PLAY_EPG_FLAG)
		item->image = "s_ts_green";
	else if (arg->event_flag & PVR_EPG_FLAG)
		item->image = "s_ts_yellow";
	else
		item->image = NULL;

	//col-1: duration
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = app_richepg_duration_str_get(arg->start_time, arg->finish_time, 0);
	item->image = NULL;

	//col-2: event name
	item = item->next;
	if(item == NULL)
		return GXCORE_ERROR;
	item->x_offset = 0;
	item->string = arg->event_title;
	item->image = NULL;

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_event_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_event_list_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	GUI_Event *event = NULL;
	int sel = 0;

	if (_is_timelist())
		return EVENT_TRANSFER_STOP;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
			{
				case STBK_PAGE_UP:
					if (ctrl->lv_data.total == 0)
					{
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					GUI_GetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
					if (sel == 0)
					{
						sel = ctrl->lv_data.total - 1;
						GUI_SetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;

				case STBK_PAGE_DOWN:
					if (ctrl->lv_data.total == 0)
					{
						ret = EVENT_TRANSFER_STOP;
						break;
					}
					GUI_GetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
					if (ctrl->lv_data.total == sel+1)
					{
						sel = 0;
						GUI_SetProperty(s_eutel_epg_wgt.lv_event, "select", &sel);
						ret = EVENT_TRANSFER_STOP;
					}
					break;
				case STBK_LEFT:
				case STBK_RIGHT:
					_set_focus_widget(s_eutel_epg_wgt.lv_prog);
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

static int _eutel_epg_tmlist_get_data(GuiWidget *widget, void *usrdata)
{
	static int csel = 0;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	EutelEpgListData *data = NULL;
	TimeItemPara* time = NULL;

	if (!_is_timelist())
		return EVENT_TRANSFER_STOP;

	time = (TimeItemPara *)usrdata;
	if (time->from_start)
		csel = 0;

	if (time->sel >= EUTEL_EPG_MAX_PROG)
		return EVENT_TRANSFER_STOP;

	data = &ctrl->data[time->sel];
	if (csel < data->vtotal)
	{
		time->string = data->vindex[csel]->timelist_data;
		csel++;
	}
	else if (csel == data->vtotal)
	{
		time->string = "<end>";
	}

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_tmlist_change(GuiWidget *widget, void *usrdata)
{
	bool day_change_flag = false;
	EutelEpgCtrl *ctrl = &s_eutel_epg_ctrl;
	char tm_str[8] = {0};
	time_t offset_sec = 0;
	char *move_direct = NULL;
	time_t day_start = 0;

	if (!_is_timelist())
		return EVENT_TRANSFER_STOP;

	// timelist change 后直到这次的GUI_Exec完成，才能正确获取到 select_column, 所以focus timer先remove再add
	APP_TIMER_REMOVE(ctrl->focus_update);

	GUI_GetProperty(s_eutel_epg_wgt.tl_event, "start", tm_str);
	tm_str[6] = '\0';
	offset_sec = app_edit_str_to_hourmin_sec(tm_str + 1);
	offset_sec %= SEC_PER_DAY;
	ctrl->tmlist_start_sec = offset_sec;

	move_direct = (char*)usrdata;
	if ((offset_sec == 0) && (strcmp(move_direct, "right") == 0))
	{
		day_change_flag = true;
		(ctrl->day_sel == EUTEL_EPG_MAX_DAY - 1) ? (ctrl->day_sel = 0) : (ctrl->day_sel++);
		app_eutel_epg_set_tmlist_start_time("<00:00>", 0);
	}
	else if ((offset_sec == 23*SEC_PER_HOUR) && (strcmp(move_direct, "left") == 0))
	{
		day_change_flag = true;
		(ctrl->day_sel == 0) ? (ctrl->day_sel =  EUTEL_EPG_MAX_DAY - 1) : (ctrl->day_sel--);
		app_eutel_epg_set_tmlist_start_time("<23:00>", 0);
	}

	day_start = ctrl->local_day + ctrl->day_sel * SEC_PER_DAY;
	if (day_change_flag)
		app_eutel_epg_set_event_date(day_start);

	// 这里用定时器向右刷新会重置focus
	app_eutel_epg_epg_info_update();

	APP_TIMER_ADD(ctrl->focus_update, app_eutel_epg_focus_update_exec, 500, TIMER_REPEAT);

	return EVENT_TRANSFER_STOP;
}

static int _eutel_epg_tmlist_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;

	if (!_is_timelist())
		return EVENT_TRANSFER_STOP;

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
					GUI_SendEvent(s_eutel_epg_wgt.lv_prog, event);
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

SIGNAL_HANDLER int app_eutel_epg_create(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_create(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_destroy(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_got_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_got_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_lost_focus(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_lost_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_point_list_get_total(GuiWidget *widget, void *usrdata)
{
	return EUTEL_EPG_MAX_PROG;
}

SIGNAL_HANDLER int app_eutel_epg_point_list_get_data(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_epg_point_list_change(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_epg_point_list_keypress(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_epg_prog_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_prog_list_get_total(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_prog_list_get_data(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_prog_list_get_data(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_prog_list_change(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_prog_list_change(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_prog_list_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_prog_list_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_event_list_get_total(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_event_list_get_total(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_event_list_get_data(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_event_list_get_data(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_event_list_change(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_event_list_change(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_event_list_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_event_list_keypress(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_tmlist_get_data(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_tmlist_get_data(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_tmlist_change(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_tmlist_change(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_epg_tmlist_keypress(GuiWidget *widget, void *usrdata)
{
	return _eutel_epg_tmlist_keypress(widget, usrdata);
}

static RichepgEpgEvent *s_eutel_epg_info = NULL;

RichepgEpgEvent *app_eutel_epg_info_get(void)
{
	if(s_eutel_epg_info == NULL)
	{
		if ((s_eutel_epg_info = (RichepgEpgEvent *)GxCore_Calloc(1, EUTEL_EPG_EVENT_SIZE)) == NULL)
		{
			EUTEL_ERR("malloc failed!\n");
			return NULL;
		}
	}

	return s_eutel_epg_info;
}

void app_eutel_epg_info_release(void)
{
	APP_FREE(s_eutel_epg_info);
}

RichepgEpgEvent *app_eutel_cur_next_epg_info_get(int flag) // 0,cur; 1, next
{
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	RichepgEpgEvent *epg_info = NULL;
	int sel = 0;
	int channel_id = 0;
	int zone_sec = 0;

	if ((epg_info = app_eutel_epg_info_get()) == NULL)
	{
		EUTEL_ERR("null epg_info!\n");
		return NULL;
	}
	sel = eutel_channel_play_use_sel_get();
	if (sel < 0)
	{
		EUTEL_ERR("play prog sel not find!\n");
		return NULL;
	}
	channel_id = ch_ctrl->arg_array[ch_ctrl->use_array[sel].pos].channel_id;

	if (flag == 0)
	{
		if (richepg_epg_event_info_get_cur(channel_id,
					epg_info, EUTEL_EPG_EVENT_SIZE, true) < 0)
			return NULL;
	}
	else
	{
		if (richepg_epg_event_info_get_next(channel_id,
					epg_info, EUTEL_EPG_EVENT_SIZE, true) < 0)
			return NULL;
	}

	zone_sec = get_display_time_by_timezone(0);
	epg_info->start_time += zone_sec;
	epg_info->finish_time += zone_sec;

	return epg_info;
}

int app_eutel_epg_cur_next_epg_info_str_get(char **cur_time, char **cur_str, char **next_time, char **next_str)
{
#define CH_EPG_TIME_LEN 16
#define MAX_EVENT_LEN   256
	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	RichepgEpgEvent *epg_info = NULL;
	int sel = 0;
	EutelChannelArg *arg = NULL;
	static char cur_epg_time[CH_EPG_TIME_LEN];
	static char cur_epg_str[MAX_EVENT_LEN];
	static char next_epg_time[CH_EPG_TIME_LEN];
	static char next_epg_str[MAX_EVENT_LEN];
	char *event_duration = NULL;
	char *event_title = NULL;
	time_t start_sec = 0, finish_sec = 0;

	if (!cur_str || !next_str)
	{
		EUTEL_ERR("null ptr!\n");
		return GXCORE_ERROR;
	}
	if ((epg_info = app_eutel_epg_info_get()) == NULL)
	{
		EUTEL_ERR("null epg_info!\n");
		return GXCORE_ERROR;
	}
	sel = eutel_channel_play_use_sel_get();
	if (sel < 0 || sel >= ch_ctrl->use_total)
	{
		EUTEL_ERR("play prog sel not find!\n");
		return GXCORE_ERROR;
	}
	arg = &ch_ctrl->arg_array[ch_ctrl->use_array[sel].pos];

	if (richepg_epg_event_info_get_cur(arg->channel_id,
				epg_info, EUTEL_EPG_EVENT_SIZE, false) == 0)
	{
		start_sec = get_display_time_by_timezone(epg_info->start_time);
		finish_sec = get_display_time_by_timezone(epg_info->finish_time);
		event_duration = app_richepg_duration_str_get(start_sec, finish_sec, 0);
		if (epg_info->event_title && strlen((char*)epg_info->event_title) > 0)
			event_title = (char*)epg_info->event_title;
		else
			event_title = app_richepg_translate_str(NO_EVNET_NAME_STR);
		snprintf(cur_epg_time, CH_EPG_TIME_LEN, "%s", event_duration);
		*cur_time = cur_epg_time;
		snprintf(cur_epg_str, MAX_EVENT_LEN, "%s", event_title);
		*cur_str = cur_epg_str;
	}
	else
	{
		*cur_time = STR_ID_ZERO_DURATION;
		event_title = eutel_array_name_get(arg->genre_num, arg->genre_ids, EUTEL_CHANNEL_GENRE);
		if (event_title)
		{
			snprintf(cur_epg_str, MAX_EVENT_LEN, "%s", event_title);
			*cur_str = cur_epg_str;
		}
		else
		{
			*cur_str = STR_ID_NO_INFO;
		}
	}

	if (richepg_epg_event_info_get_next(arg->channel_id,
				epg_info, EUTEL_EPG_EVENT_SIZE, false) == 0)
	{
		start_sec = get_display_time_by_timezone(epg_info->start_time);
		finish_sec = get_display_time_by_timezone(epg_info->finish_time);
		event_duration = app_richepg_duration_str_get(start_sec, finish_sec, 0);
		if (epg_info->event_title && strlen((char*)epg_info->event_title) > 0)
			event_title = (char*)epg_info->event_title;
		else
			event_title = app_richepg_translate_str(NO_EVNET_NAME_STR);
		snprintf(next_epg_time, CH_EPG_TIME_LEN, "%s", event_duration);
		*next_time = next_epg_time;
		snprintf(next_epg_str, MAX_EVENT_LEN, "%s", event_title);
		*next_str = next_epg_str;
	}
	else
	{
		*next_time = STR_ID_ZERO_DURATION;
		event_title = eutel_array_name_get(arg->genre_num, arg->genre_ids, EUTEL_CHANNEL_GENRE);
		if (event_title)
		{
			snprintf(next_epg_str, MAX_EVENT_LEN, "%s", event_title);
			*next_str = next_epg_str;
		}
		else
		{
			*next_str = STR_ID_NO_INFO;
		}
	}

	return GXCORE_SUCCESS;
}

int app_eutel_parental_lock_info_check(void)
{
	int parent_rate = 0;
	int cur_rate = 0 ;
	int old_rate = 0 ;

	EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
	RichepgEpgEvent *epg_info = NULL;
	int sel = 0;
	int channel_id = 0;
	GxMessage   *new_msg = NULL;
	GxMsgProperty_EpgParentalInfo *parental = NULL;
	GxMsgProperty_NodeByPosGet temp_node = {0} ;
	extern int app_parental_rating_get(void);

#if PARENTAL_LOCK_SUPPORT
	GxBus_ConfigGetInt(PARENTAL_GUIDANCE_KEY, &parent_rate, PARENTAL_GUIDANCE_VALUE);
#else
	return 0 ;
#endif
	if (parent_rate < 4 || parent_rate > 18)
	{
		goto end;
	}
	if ((epg_info = app_eutel_epg_info_get()) == NULL)
	{
		EUTEL_ERR("null epg_info!\n");
		goto end;
	}

	sel = eutel_channel_play_use_sel_get();
	if (sel < 0)
	{
		goto end;
	}
	channel_id = ch_ctrl->arg_array[ch_ctrl->use_array[sel].pos].channel_id;
	if (richepg_epg_event_info_get_cur(channel_id, epg_info, EUTEL_EPG_EVENT_SIZE, false) < 0)
	{
		goto end;
	}
	cur_rate = epg_info->parent_rate ;

	if (cur_rate < 4 || cur_rate > 18 || cur_rate < parent_rate)
	{
		cur_rate = 0 ;
		goto end;
	}
	else
	{
		if ( app_parental_rating_get() == (cur_rate - 3))
			return -1 ;

		if (cur_rate >= parent_rate)
		{
			if (GUI_CheckDialog(WND_EUTEL_CHMORE) == GXCORE_SUCCESS)
				GUI_EndDialog(WND_EUTEL_CHMORE);
		}
		cur_rate = cur_rate - 3 ;
	}

end:
	old_rate = app_parental_rating_get() ;

	if (cur_rate ==  old_rate)
	{
		return -1;
	}
	else
	{
		if(g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK && GUI_CheckDialog(WIN_MOVIE_VIEW) != GXCORE_SUCCESS)
		{
			EUTEL_SINFO("parental lock manual %d,%d \n",cur_rate,old_rate);
			app_parental_rating_set(cur_rate);
			return -1 ;
		}
		EUTEL_SINFO("parental lock change %d,%d \n",cur_rate,old_rate);
	}

	new_msg = GxBus_MessageNew(GXMSG_EPG_PARENTAL_SEND);
	parental = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_EpgParentalInfo);
	if (parental == NULL )
	{
		EUTEL_ERR("GxBus_GetMsgPropertyPtr err \n");
		return -1 ;
	}

	temp_node.node_type = NODE_PROG;
	temp_node.pos = g_AppPlayOps.normal_play.play_count;
	if ( GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&temp_node)) )
	{
		parental->service_id = temp_node.prog_data.service_id;
	}

	parental->parental_rating_num = 1;
	parental->parental_rating[0].rating = cur_rate ;
	GxBus_MessageSend(new_msg);

	return 0;
}

#endif
