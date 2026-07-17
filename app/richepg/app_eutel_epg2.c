#include "app.h"
#include "app_windows.h"
#include "full_screen.h"
#include "app_book.h"
#include "app_send_msg.h"
#include "app_module.h"
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif

#if RICHEPG_SUPPORT
#include "app_eutel_control.h"
#include "app_eutel_common.h"
#include "richepg_block_mem.h"
#include "richepg_file_splice.h"
#include "richepg_epg.h"
#include "richepg_common.h"
#include "richepg.h"

#define CONTENT_TIMER_SUPPORT   1
#define CONTENT_ALL_SUPPORT     0
#define MORE_GROUP_SUPPORT      0

#if (CONTENT_TIMER_SUPPORT && CONTENT_ALL_SUPPORT)
#define CONTENT_PRIORITY_NUM    1
#define CONTENT_RESERVED_NUM    2
#elif CONTENT_TIMER_SUPPORT
#define CONTENT_PRIORITY_NUM    0
#define CONTENT_RESERVED_NUM    1
#elif CONTENT_ALL_SUPPORT
#define CONTENT_PRIORITY_NUM    1
#define CONTENT_RESERVED_NUM    1
#else
#define CONTENT_PRIORITY_NUM    0
#define CONTENT_RESERVED_NUM    0
#endif

#if CONTENT_ALL_SUPPORT
#define CONTENT_ALL_GROUP_SEL   0
#endif
#if CONTENT_TIMER_SUPPORT
#define CONTENT_TIMER_GROUP_SEL (s_eutel_ch_ctrl.content_total+CONTENT_RESERVED_NUM-1)
#endif

#define LEFT_RIGHT_SERIAL_MODE  1
#define FOCUS_LAST_ITEM_SUPPORT 1
#define FOCUS_GROUP_DEFAULT     1
#define TIMER_FLUSH_LOGO        1
#if TIMER_FLUSH_LOGO
#ifdef LINUX_OS
#define CACHE_LOGO_COMMON       "/tmp/record_logo%d.jpg"
#else
#define CACHE_LOGO_COMMON       "/mnt/record_logo%d.jpg"
#endif
#endif

#define ITEM_COMMON_SIZE        32
#define EUTEL_EPG_MAX_DAY       8
#define SEC_PER_MIN             60
#define SEC_PER_HOUR            3600  // 60*60
#define SEC_PER_DAY             86400 // 60*60*24

#if MORE_GROUP_SUPPORT
#define EPG_GROUPS_PER_PAGE     5
#else
#define EPG_GROUPS_PER_PAGE     6
#endif
#define EPG_DAYS_PER_PAGE       6
#define EPG_ITEMS_PER_PAGE      6
#define EPG_ITEMS_COL_NUMS      2
#define EPG_ITEMS_ROW_NUMS      3
#define ITEM_KEY_COMMON         "epg2_logo_key%d"

#define ITEM_FOCUS_IMG          "richepg_epg_focus"
#define ITEM_UNFOCUS_IMG        "richepg_epg_unfocus"
#define GROUP_FOCUS_IMG         "richepg_group_focus"
#define GROUP_SELECT_IMG        "richepg_group_select"
#define GROUP_UNFOCUS_IMG       "richepg_group_unfocus"
#define GROUP_UNFOCUS2_IMG      "richepg_group_unfocus2"

#define IDLE_BG_COLOR           "[window_bg_color,window_bg_color,window_bg_color]"
#define IDLE_BG2_COLOR          "[#212121,#212121,#212121]"
#define CUR_BG_COLOR            "[#005a99,#005a99,#005a99]"
//#define PVR_BG_COLOR            "[#e6a200,#e6a200,#e6a200]"
#define PVR_BG_COLOR            "[#aa5588,#aa5588,#aa5588]"
#define PLAY_BG_COLOR           "[#008000,#008000,#008000]"
#define GROUP_FG_COLOR          "[text_color,text_color,text_color]"
#define TIMER_FG_COLOR          "[#ffff00,#ffff00,#ffff00]"

#define ITEM_FG_COLOR           "[text_color,text_color,text_color]"
#define PLAY_FG_COLOR           "[#bbe00b,#bbe00b,#bbe00b]"

#define EPG2_REFRESH_BY_TIMER_SUPPROT

enum EPG_DAY_SHOW {
    EPG_DAY_NOW = 0,
    EPG_DAY_SOON,
    EPG_DAY_EVENING,
    EPG_DAY_TOMORROW,
    EPG_DAY_P2,
    EPG_DAY_P3,
    EPG_DAY_P4,
    EPG_DAY_P5,
    EPG_DAY_P6,
    EPG_DAY_P7,
    EPG_DAY_TOTAL
};
#define EPG_OTHER_DAY_NUM       2

static char* s_day_array[EPG_DAY_TOTAL] = {
    STR_ID_NOW,
    STR_ID_SOON,
    STR_ID_EVENING,
    STR_ID_TOMORROW,
    STR_ID_DAY_P2,
    STR_ID_DAY_P3,
    STR_ID_DAY_P4,
    STR_ID_DAY_P5,
    STR_ID_DAY_P6,
    STR_ID_DAY_P7
};

typedef struct {
    bool reverse_flag;
    bool chlogo_bg_flag;
    const char *wnd_name;
    const char *group_item_common;
    const char *day_item_common;
    const char *item_back_common;
    const char *item_logo_common;
    const char *item_title_common;
    const char *item_time_common;
    const char *item_ch_common;

    const char *text_item_page;
    const char *text_item_duration;
    const char *text_item_name;
    const char *text_item_parent;
    const char *text_item_genres;
    const char *text_item_brief;
    const char *text_ch_name;
    const char *text_ch_logo;
    const char *img_ch_logo;

    const char *details_back;
    const char *details_duration;
    const char *details_title;
    const char *details_notepad;
    const char *details_exit;

    const char *img_ok_tip;
    const char *txt_ok_tip;
    const char *txt_red_tip;
    const char *txt_green_tip;
    const char *img_book;
    const char *txt_book;
    const char *text_time;
    const char *text_date;
} EutelEpg2Widget;

enum {
    CUR_EPG_FLAG  = 1 << 0,
    PLAY_EPG_FLAG = 1 << 1,
    PVR_EPG_FLAG  = 1 << 2,
    INVALID_EPG_FLAG = 1 << 3,
    NEXT_EPG_FLAG = 1 << 4,
    CUR_PLAY_FLAG = 1 << 5
};

typedef enum {
    AREA_ITEM = 0,
    AREA_GROUP,
#if MORE_GROUP_SUPPORT
    AREA_MORE,
#endif
    AREA_DAY
} AreaSel;

typedef struct {
    int ch_pos;
    int epg_pos;
} EutelPos;

#if TIMER_FLUSH_LOGO
typedef enum {
    LOGO_IDLE = 0,
    LOGO_ADD,
    LOGO_COPY,
    LOGO_FIN
} LogoState;

typedef struct {
#define EPG_FULL_NAME_LEN 128
    LogoState state;
    char *name;
} LogoPara;

typedef struct {
    handle_t mutex;
    event_list *time_refresh;
    uint8_t thread_cache_run;
    uint8_t cache_stop_rq;
    uint8_t logo_copy_rq;
    uint8_t logo_stop_rq;
    LogoPara logo_para[EPG_ITEMS_PER_PAGE];
} EutelEpg2Mgr;
#endif

#if CONTENT_TIMER_SUPPORT
typedef struct {
    int prog_pos;
    int prog_id;
    int book_id;
    GxBookType book_type;
    GxBookRepeatMode  repeat_mode;
    time_t start_time;
    time_t duration;
} EutelTimerData;

typedef struct {
    int epg_sel;
    int timer_sel;
    time_t start_time;
} EutelTimerIndex;
#endif

#if FOCUS_LAST_ITEM_SUPPORT
typedef struct {
    bool bak_flag;
    int group_sel;
    int day_sel;
    int cur_index;
    event_list *bak_timer;
} EutelIndexBak;
#endif

typedef struct {
    RichepgEpgBriefManage data_mgr;
    EutelEpg2Widget wgt;
    BlockMemMgr mem_mgr;
#if TIMER_FLUSH_LOGO
    EutelEpg2Mgr mgr;
#endif

    AreaSel area_sel;
    int group_sel;
    int day_sel;
    int cur_index;
    int focus_sel;
    int rfocus_sel; // 有两个界面，最后聚焦的位置可能不同

    int day_total;
    int epg_total;
    EutelPos *epg_array;
    int use_total;
    int *use_array;
#if CONTENT_TIMER_SUPPORT
    int group_return;
    int timer_data_total;
    EutelTimerData *timer_data_array;
    int timer_index_total;
    EutelTimerIndex *timer_index_array;
#endif
    int detail_flag;
    event_list *time_update;
    RichepgEpgEvent *use_event;
#if FOCUS_LAST_ITEM_SUPPORT
    EutelIndexBak index_bak;
#endif
} EutelEpg2Ctrl;

static EutelEpg2Ctrl s_eutel_epg2_ctrl;
extern bool app_get_same_start_play_or_pvr_book(uint16_t prog_id, time_t start_time, GxBook *book_ret);
extern void app_timer_edit_ex_func_register(void (*exit_func)(void));
extern bool app_epg_timer_menu_exec(time_t start_time, time_t duration, int prog_id, const char *prog_name);
static int app_eutel_epg2_content_filter_pop(AreaSel area_sel);
static void app_eutel_epg2_change_ok_tip(void);

#ifdef EPG2_REFRESH_BY_TIMER_SUPPROT
static char *_get_duration_str(time_t start_time, time_t finish_time, bool long_flag);
static int _cal_cur_index_by_last_sel(void);
static int app_eutel_epg2_update_item_all(int last_sel, int new_sel, bool focus_change);
#endif

static int app_eutel_epg2_data_free(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    block_mem_mgr_free_data(&ctrl->mem_mgr);
    memset(&ctrl->data_mgr, 0, sizeof(RichepgEpgBriefManage));
    ctrl->epg_total = 0;
    ctrl->epg_array = NULL;
    ctrl->use_total = 0;
    ctrl->use_array = NULL;
#if CONTENT_TIMER_SUPPORT
    ctrl->timer_data_total = 0;
    ctrl->timer_data_array = NULL;
    ctrl->timer_index_total = 0;
    ctrl->timer_index_array = NULL;
#endif
    return 0;
}

static int app_eutel_epg2_ctrl_release(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    APP_TIMER_REMOVE(ctrl->time_update);
    APP_FREE(ctrl->use_event);
    app_eutel_epg2_data_free();

    return 0;
}

static void* _epg2_alloc(size_t size)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    return block_mem_mgr_alloc_data(size, &ctrl->mem_mgr);
}

static int _eutel_epg2_cmp(const void *a, const void *b)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    EutelPos *pa = (EutelPos *)a;
    EutelPos *pb = (EutelPos *)b;

    time_t ta = ctrl->data_mgr.channel_array[pa->ch_pos].event_array[pa->epg_pos].start_time;
    time_t tb = ctrl->data_mgr.channel_array[pb->ch_pos].event_array[pb->epg_pos].start_time;
    if (ta != tb)
        return ta - tb;

    int lcna = ch_ctrl->arg_array[ctrl->data_mgr.channel_array[pa->ch_pos].ch_channel_pos].lcn_ids[0];
    int lcnb = ch_ctrl->arg_array[ctrl->data_mgr.channel_array[pb->ch_pos].ch_channel_pos].lcn_ids[0];
    return lcna - lcnb;
}

static void _update_book_flag(int sec, int prog_id, RichepgEpgBriefEvent *index)
{
    GxBook prog_book;

    memset(&prog_book, 0, sizeof(GxBook));
    if (app_get_same_start_play_or_pvr_book(prog_id, index->start_time, &prog_book)
            && (index->start_time >= sec))
    {
        if (prog_book.book_type == BOOK_PROGRAM_PLAY)
        {
            index->event_flag |= PLAY_EPG_FLAG;
        }
        else if (prog_book.book_type == BOOK_PROGRAM_PVR)
        {
            index->event_flag |= PVR_EPG_FLAG;
        }
        else
        {
            index->event_flag &= ~(PLAY_EPG_FLAG | PVR_EPG_FLAG);
        }
    }
    else
    {
        index->event_flag &= ~(PLAY_EPG_FLAG | PVR_EPG_FLAG);
    }
}

static bool _check_play_flag(int cur_id, int check_id)
{
    if (cur_id != check_id
            || g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK
            || g_AppFullArb.state.pause == STATE_ON)
    {
        return true;
    }

    return false;
}

static int app_euel_epg2_update_cur_play_flag(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int channel_id = eutel_cur_channel_id_get();
    RichepgEpgBriefEvent *index = NULL;
    int i = 0, j = 0;

    for (i = 0; i < ctrl->data_mgr.channel_total; i++)
    {
        for (j = 0; j < ctrl->data_mgr.channel_array[i].event_total; j++)
        {
            index = &ctrl->data_mgr.channel_array[i].event_array[j];
            if (ctrl->data_mgr.channel_array[i].channel_id == channel_id)
            {
                index->event_flag |= CUR_PLAY_FLAG;
            }
            else
            {
                index->event_flag &= ~(CUR_PLAY_FLAG);
            }
        }
    }

    return 0;
}

static int app_euel_epg2_update_flag(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int channel_id = eutel_cur_channel_id_get();
    RichepgEpgBriefEvent *index = NULL;
    RichepgEpgBriefEvent *last_index = NULL;
    GxTime time = {0};
    int i = 0, j = 0;

    GxCore_GetLocalTime(&time);

    for (i = 0; i < ctrl->data_mgr.channel_total; i++)
    {
        for (j = 0; j < ctrl->data_mgr.channel_array[i].event_total; j++)
        {
            index = &ctrl->data_mgr.channel_array[i].event_array[j];
            last_index = (j > 0) ? &ctrl->data_mgr.channel_array[i].event_array[j-1] : NULL;
            index->event_flag &= ~(CUR_EPG_FLAG | NEXT_EPG_FLAG | INVALID_EPG_FLAG | CUR_PLAY_FLAG);
            if (time.seconds >= index->finish_time)
            {
                index->event_flag |= INVALID_EPG_FLAG;
            }
            else if (time.seconds >= index->start_time && time.seconds < index->finish_time)
            {
                index->event_flag |= CUR_EPG_FLAG;
            }
            else if (j == 0 || (last_index->event_flag & CUR_EPG_FLAG) || (last_index->event_flag & INVALID_EPG_FLAG))
            {
                index->event_flag |= NEXT_EPG_FLAG;
            }

            if (ctrl->data_mgr.channel_array[i].channel_id == channel_id)
            {
                index->event_flag |= CUR_PLAY_FLAG;
            }
        }
    }

    return 0;
}

int _epg2_get_duration(enum EPG_DAY_SHOW day, time_t *start_time, time_t *finish_time)
{
    GxTime time = {0};
    time_t utc_sec = 0, t1 = 0, t2 = 0;

    GxCore_GetLocalTime(&time);
    utc_sec = time.seconds;
    switch (day)
    {
        case EPG_DAY_NOW:
            {
                t1 = utc_sec;
                t2 = utc_sec + EUTEL_EPG_MAX_DAY * SEC_PER_DAY;
            }
            break;
        case EPG_DAY_SOON:
            {
                t1 = utc_sec;
                t2 = utc_sec + 2 * SEC_PER_HOUR;
            }
            break;
        case EPG_DAY_EVENING: case EPG_DAY_TOMORROW: case EPG_DAY_P2: case EPG_DAY_P3:
        case EPG_DAY_P4: case EPG_DAY_P5: case EPG_DAY_P6: case EPG_DAY_P7:
            {

                int zone_sec = get_display_time_by_timezone(0);
                t1 = utc_sec + zone_sec;
                t1 -= t1 % SEC_PER_DAY;

                if (day == EPG_DAY_EVENING)
                {
                    t1 += app_richepg_epg_evening_start_get() * SEC_PER_HOUR;
                    t2 = t1 + 12 * SEC_PER_HOUR;
                    t1 -= 10 * SEC_PER_MIN;
                }
                else if (day == EPG_DAY_TOMORROW)
                {
                    t1 += SEC_PER_DAY + RICHEPG_EPG_TOMORROW_START * SEC_PER_HOUR;
                    t2 = t1 + SEC_PER_DAY;
                }
                else
                {
                    t1 += (day - EPG_DAY_P2 + 2) * SEC_PER_DAY;
                    t2 = t1 + SEC_PER_DAY;
                }
                t1 -= zone_sec;
                t2 -= zone_sec;
            }
            break;
        default:
            break;
    }

    *start_time = t1;
    *finish_time = t2;
    return 0;
}

bool _epg2_check_day(RichepgEpgBriefEvent *event, enum EPG_DAY_SHOW day, time_t start_time, time_t finish_time)
{
    bool flag = false;

    if (event->event_flag & INVALID_EPG_FLAG)
        return false;

    switch (day)
    {
        case EPG_DAY_NOW:
            flag = true;
            break;
        case EPG_DAY_SOON:
            flag = (event->event_flag & NEXT_EPG_FLAG) ? true : false;
            break;
        case EPG_DAY_EVENING: case EPG_DAY_TOMORROW:
            flag = (event->start_time >= start_time && event->start_time < finish_time) ? true : false;
            break;
        case EPG_DAY_P2: case EPG_DAY_P3: case EPG_DAY_P4:
        case EPG_DAY_P5: case EPG_DAY_P6: case EPG_DAY_P7:
            flag = (event->start_time >= finish_time || event->finish_time <= start_time) ? false : true;
            break;
        default:
            break;
    }

    return flag;
}

#if CONTENT_TIMER_SUPPORT
static int _eutel_timer_cmp(const void *a, const void *b)
{
    return ((EutelTimerIndex*)a)->start_time - ((EutelTimerIndex*)b)->start_time;
}

static int app_eutel_epg2_timer_data_filter(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    RichepgEpgBriefEvent *event = NULL;
    EutelTimerData *timer_data = NULL;
    GxBookGet bookGet;
    BookProgStruct book_data = {0};
    int total = 0, count = 0;
    int i = 0, j = 0;
    time_t start_time = 0, finish_time = 0;
    time_t add_time = 0, base_time = 0, real_time = 0;
    WeekDay wday1, wday2;
    time_t time_temp1, time_temp2;
    uint8_t day_count;
    int prog_id = 0;

    if  (ctrl->timer_data_array == NULL || ctrl->timer_index_array == NULL)
    {
        ctrl->timer_data_total = 0;
        ctrl->timer_index_total = 0 ;
        ctrl->use_total = 0;
        return 0 ;
    }

    total = g_AppBook.get(&bookGet, BOOKMODE_ALL);
    for (i = 0; i < total; i++)
    {
        if (bookGet.book[i].book_type == BOOK_PROGRAM_PVR || bookGet.book[i].book_type == BOOK_PROGRAM_PLAY)
        {
            memcpy(&book_data, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));
            ctrl->timer_data_array[count].prog_pos = -1;
            for (j = 0; j < ch_ctrl->arg_total; j++)
            {
                if (ch_ctrl->arg_array[j].prog_id == book_data.prog_id)
                {
                    ctrl->timer_data_array[count].prog_pos = j;
                    break;
                }
            }
            if (ctrl->timer_data_array[count].prog_pos < 0)
                continue;
            ctrl->timer_data_array[count].prog_id = book_data.prog_id;
            ctrl->timer_data_array[count].book_id = bookGet.book[i].id;
            ctrl->timer_data_array[count].book_type = bookGet.book[i].book_type;
            ctrl->timer_data_array[count].repeat_mode = bookGet.book[i].repeat_mode;
            ctrl->timer_data_array[count].start_time = bookGet.book[i].trigger_time_start;
            ctrl->timer_data_array[count].duration = book_data.duration;
            ++count;
        }
    }
    ctrl->timer_data_total = count;

    _epg2_get_duration(ctrl->day_sel, &start_time, &finish_time);
    count = 0;
    for (i = 0; i < ctrl->timer_data_total; i++)
    {
        if (ctrl->timer_data_array[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
        {
            if (ctrl->timer_data_array[i].start_time < finish_time && ctrl->timer_data_array[i].start_time > start_time)
            {
                ctrl->timer_index_array[count].epg_sel = -1;
                ctrl->timer_index_array[count].timer_sel = i;
                ctrl->timer_index_array[count].start_time = ctrl->timer_data_array[i].start_time;
                ++count;
            }
        }
        else if (ctrl->timer_data_array[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
        {
            add_time = ctrl->timer_data_array[i].start_time % SEC_PER_DAY;
            base_time = start_time - start_time % SEC_PER_DAY;
            for (j = 0; j < EUTEL_EPG_MAX_DAY; j++)
            {
                real_time = add_time + base_time + j * SEC_PER_DAY;
                if (real_time < finish_time && real_time > start_time)
                {
                    ctrl->timer_index_array[count].epg_sel = -1;
                    ctrl->timer_index_array[count].timer_sel = i;
                    ctrl->timer_index_array[count].start_time = real_time;
                    ++count;
                }
            }
        }
        else
        {
            time_temp1 = start_time % SEC_PER_DAY;
            wday1 = g_AppBook.time2wday(start_time);

            time_temp2 = ctrl->timer_data_array[i].start_time % SEC_PER_DAY;
            wday2 = g_AppBook.mode2wday(ctrl->timer_data_array[i].repeat_mode.mode);

            if(wday2 > wday1)
            {
                day_count = wday2 - wday1;
            }
            else if(wday2 < wday1)
            {
                day_count = (wday2 + 7) - wday1;
            }
            else
            {
                if(time_temp2 < time_temp1)
                    day_count = 7;
                else
                    day_count = 0;
            }
            real_time = (start_time - time_temp1) + (day_count * SEC_PER_DAY) + time_temp2;

            if (real_time < finish_time && real_time > start_time)
            {
                ctrl->timer_index_array[count].epg_sel = -1;
                ctrl->timer_index_array[count].timer_sel = i;
                ctrl->timer_index_array[count].start_time = real_time;
                ++count;
            }
        }
    }
    ctrl->timer_index_total = count;
    qsort(ctrl->timer_index_array, ctrl->timer_index_total, sizeof(EutelTimerIndex), _eutel_timer_cmp);

    for (i = 0; i < ctrl->timer_index_total; i++)
    {
        timer_data = &ctrl->timer_data_array[ctrl->timer_index_array[i].timer_sel];
        if (timer_data->book_type == BOOK_PROGRAM_PVR)
            real_time = ctrl->timer_index_array[i].start_time + timer_data->duration / 2;
        else
            real_time = ctrl->timer_index_array[i].start_time;

        for (j = 0; j < ctrl->epg_total; j++)
        {
            prog_id = ch_ctrl->arg_array[ctrl->data_mgr.channel_array[ctrl->epg_array[j].ch_pos].ch_channel_pos].prog_id;
            if (prog_id != timer_data->prog_id)
                continue;
            event = &ctrl->data_mgr.channel_array[ctrl->epg_array[j].ch_pos].event_array[ctrl->epg_array[j].epg_pos];
            if (real_time < event->finish_time && real_time >= event->start_time)
            {
                ctrl->timer_index_array[i].epg_sel = j;
                break;
            }
        }
    }

    for (i = 0; i < ctrl->timer_index_total; i++)
    {
        ctrl->use_array[i] = ctrl->timer_index_array[i].epg_sel;
    }
    ctrl->use_total = ctrl->timer_index_total;

    return ctrl->use_total;
}
#endif

static int app_eutel_epg2_data_filter(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    RichepgEpgBriefEvent *event = NULL;
    int content_id = 0;
    time_t start_time = 0, finish_time = 0;
    int i = 0, j = 0;

    app_euel_epg2_update_flag();
    if (ctrl->group_sel >= ch_ctrl->content_total + CONTENT_RESERVED_NUM)
        ctrl->group_sel = 0;
    if (ctrl->day_sel >= ctrl->day_total)
        ctrl->day_sel = 0;

#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        app_eutel_epg2_timer_data_filter();
        return ctrl->use_total;
    }
#endif

#if CONTENT_ALL_SUPPORT
    if (ctrl->group_sel == CONTENT_ALL_GROUP_SEL)
    {
        _epg2_get_duration(ctrl->day_sel, &start_time, &finish_time);
        ctrl->use_total = 0;
        for (i = 0; i < ctrl->epg_total; i++)
        {
            event = &ctrl->data_mgr.channel_array[ctrl->epg_array[i].ch_pos].event_array[ctrl->epg_array[i].epg_pos];
            if (_epg2_check_day(event, ctrl->day_sel, start_time, finish_time))
            {
                ctrl->use_array[ctrl->use_total++] = i;
            }
        }
        return ctrl->use_total;
    }
#endif

    _epg2_get_duration(ctrl->day_sel, &start_time, &finish_time);
    ctrl->use_total = 0;
    if (CONTENT_RESERVED_NUM == 0 && ch_ctrl->content_total == 0)
        return 0;
    content_id = ch_ctrl->content_array[ctrl->group_sel-CONTENT_PRIORITY_NUM].id;
    for (i = 0; i < ctrl->epg_total; i++)
    {
        event = &ctrl->data_mgr.channel_array[ctrl->epg_array[i].ch_pos].event_array[ctrl->epg_array[i].epg_pos];
        for (j = 0; j < event->genres_num; j++)
        {
            if (event->genres_ids[j] == content_id)
            {
                if (_epg2_check_day(event, ctrl->day_sel, start_time, finish_time))
                {
                    ctrl->use_array[ctrl->use_total++] = i;
                }
                break;
            }
        }
    }

    return ctrl->use_total;
}

#ifdef EPG2_REFRESH_BY_TIMER_SUPPROT
static int app_eutel_epg2_refresh_epg_data(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    RichepgEpgBriefEvent *event = NULL;
    int content_id = 0;
    time_t start_time = 0, finish_time = 0;
    int i = 0, j = 0;

    int last_sel, new_sel;
    int count = 0 ;

#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
         return 0 ;
    }
#endif

    app_euel_epg2_update_flag();

#if CONTENT_ALL_SUPPORT
    if (ctrl->group_sel == CONTENT_ALL_GROUP_SEL)
    {
        _epg2_get_duration(ctrl->day_sel, &start_time, &finish_time);
        count = 0 ;
        for (i = 0; i < ctrl->epg_total; i++)
        {
            event = &ctrl->data_mgr.channel_array[ctrl->epg_array[i].ch_pos].event_array[ctrl->epg_array[i].epg_pos];
            if (_epg2_check_day(event, ctrl->day_sel, start_time, finish_time))
            {
                count ++ ;
            }
        }
    }
    else
#endif
    {
        _epg2_get_duration(ctrl->day_sel, &start_time, &finish_time);
	    count = 0 ;
        if (CONTENT_RESERVED_NUM == 0 && ch_ctrl->content_total == 0)
            return 0;
        content_id = ch_ctrl->content_array[ctrl->group_sel-CONTENT_PRIORITY_NUM].id;
        for (i = 0; i < ctrl->epg_total; i++)
        {
            event = &ctrl->data_mgr.channel_array[ctrl->epg_array[i].ch_pos].event_array[ctrl->epg_array[i].epg_pos];
            for (j = 0; j < event->genres_num; j++)
            {
                if (event->genres_ids[j] == content_id)
                {
                    if (_epg2_check_day(event, ctrl->day_sel, start_time, finish_time))
                    {
                        count ++ ;
                    }
                    break;
                }
            }
        }
    }

    if (ctrl->use_total != count )
    {
        EUTEL_DBG("sat.tv refresh epg data count = %d ,%d \n",ctrl->use_total,count);
        app_eutel_epg2_data_filter();
        last_sel = ctrl->cur_index;

        if (last_sel <= (ctrl->use_total-1))
        {
            new_sel = last_sel;
        }
        else
        {
            new_sel = ctrl->use_total-1;
        }
        EUTEL_DBG("sat.tv last = %d,new =%d,use_total=%d \n",last_sel,new_sel,ctrl->use_total);
        app_eutel_epg2_update_item_all(-1, new_sel, true);
        return 1 ;
    }
    else
    {
        return 0 ;
    }
}
#endif

static int app_eutel_epg2_data_build(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    GxTime time = {0};
    int i = 0, j = 0, cnt = 0;
    int use_total = 0;

    app_eutel_epg2_data_free();
    if (ch_ctrl->use_total == 0)
        return -1;

    ctrl->data_mgr.channel_array = _epg2_alloc(ch_ctrl->arg_total * sizeof(RichepgEpgBriefChannel));
    if (ctrl->data_mgr.channel_array == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        return -1;
    }
    cnt = 0;
    for (i = 0; i < ch_ctrl->arg_total; i++)
    {
        if (eutel_channel_use_sel_get_by_prog_id(ch_ctrl->arg_array[i].prog_id) < 0)
            continue;
        ctrl->data_mgr.channel_array[cnt].channel_id = ch_ctrl->arg_array[i].channel_id;
        ctrl->data_mgr.channel_array[cnt].ch_channel_pos = i;
        ++cnt;
    }
    ctrl->data_mgr.channel_total = cnt;

    GxCore_GetLocalTime(&time);
    ctrl->epg_total = richepg_epg_brief_manage_update(&ctrl->data_mgr, time.seconds, time.seconds + (ctrl->day_total-EPG_OTHER_DAY_NUM) * SEC_PER_DAY, _epg2_alloc);
    if (ctrl->epg_total <= 0)
    {
        EUTEL_ERR("epg total = %d \n",ctrl->epg_total);
        goto err;
    }

    ctrl->epg_array = _epg2_alloc(ctrl->epg_total * sizeof(EutelPos));
    if (ctrl->epg_array == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }

    cnt = 0;
    for (i = 0; i < ctrl->data_mgr.channel_total; i++)
    {
        for (j = 0; j < ctrl->data_mgr.channel_array[i].event_total; j++)
        {
            ctrl->epg_array[cnt].ch_pos = i;
            ctrl->epg_array[cnt].epg_pos = j;
            ++cnt;
        }
    }
    qsort(ctrl->epg_array, ctrl->epg_total, sizeof(EutelPos), _eutel_epg2_cmp);

#if CONTENT_TIMER_SUPPORT
    use_total = (ctrl->epg_total >= APP_BOOK_NUM * EUTEL_EPG_MAX_DAY) ? (ctrl->epg_total) : (APP_BOOK_NUM * EUTEL_EPG_MAX_DAY);
#else
    use_total = ctrl->epg_total;
#endif
    ctrl->use_total = 0;
    ctrl->use_array = _epg2_alloc(use_total * sizeof(int));
    if (ctrl->use_array == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    app_euel_epg2_update_flag();

#if CONTENT_TIMER_SUPPORT
    ctrl->timer_data_total = 0;
    ctrl->timer_data_array = _epg2_alloc(APP_BOOK_NUM * sizeof(EutelTimerData));
    if (ctrl->timer_data_array == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
    ctrl->timer_index_total = 0;
    ctrl->timer_index_array = _epg2_alloc(APP_BOOK_NUM * EUTEL_EPG_MAX_DAY * sizeof(EutelTimerIndex));
    if (ctrl->timer_index_array == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
#endif

    block_mem_mgr_adjust_size(&ctrl->mem_mgr);
    return 0;
err:
    app_eutel_epg2_data_free();
    return -1;
}

static int _eutel_epg2_time_update_exec(void* usrdata)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    const char *focus = GUI_GetFocusWindow();

    if (focus && strcmp(ctrl->wgt.wnd_name, focus) == 0){
        app_show_sys_time_and_date(ctrl->wgt.text_time, ctrl->wgt.text_date);
#ifdef EPG2_REFRESH_BY_TIMER_SUPPROT
        app_eutel_epg2_refresh_epg_data();
#endif
    }
    return 0;
}

static int app_eutel_epg2_ctrl_init(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    ctrl->mem_mgr.attr.mem_id = 0;
    ctrl->mem_mgr.attr.def_size = 1024 * 128;
    ctrl->mem_mgr.attr.align_byte = 4;
    ctrl->mem_mgr.attr.fast_alloc = 0;
    ctrl->mem_mgr.attr.max_size = 0;
    block_mem_mgr_init(&ctrl->mem_mgr);

    if (app_eutel_epg2_data_build() < 0)
        return -1;

    ctrl->detail_flag = 0;
    if ((ctrl->use_event = GxCore_Calloc(1, EUTEL_EPG_EVENT_SIZE)) == NULL)
    {
        EUTEL_ERR("malloc failed!\n");
        goto err;
    }
#ifdef EPG2_REFRESH_BY_TIMER_SUPPROT
    APP_TIMER_ADD(ctrl->time_update, _eutel_epg2_time_update_exec, 10*1000, TIMER_REPEAT);
#else
    APP_TIMER_ADD(ctrl->time_update, _eutel_epg2_time_update_exec, 800, TIMER_REPEAT);
#endif

    return 0;
err:
    app_eutel_epg2_ctrl_release();
    return -1;
}

EutelChannelArg* _get_channel(int use_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    int epg_sel = -1;
    int sel = 0;

    if (use_sel >= ctrl->use_total)
        return NULL;
#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        sel = ctrl->timer_data_array[ctrl->timer_index_array[use_sel].timer_sel].prog_pos;
    }
    else
#endif
    {
        epg_sel = ctrl->use_array[use_sel];
        sel = ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos].ch_channel_pos;
    }
    if (sel >= ch_ctrl->arg_total)
        return NULL;
    return &ch_ctrl->arg_array[sel];
}

RichepgEpgEvent* _get_event(int use_sel, bool desc_flag)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    RichepgEpgBriefChannel *cinfo = NULL;
    RichepgEpgBriefEvent *einfo = NULL;
    int epg_sel = -1;

    if (use_sel >= ctrl->use_total)
        return NULL;
    epg_sel = ctrl->use_array[use_sel];
    if (epg_sel < 0)
        return NULL;
    cinfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos];
    einfo = &cinfo->event_array[ctrl->epg_array[epg_sel].epg_pos];
    if (richepg_epg_event_info_get(cinfo->channel_id, cinfo->epg_channel_pos, einfo->event_id,
                cinfo->start_event_pos + ctrl->epg_array[epg_sel].epg_pos,
                ctrl->use_event, EUTEL_EPG_EVENT_SIZE, desc_flag) < 0)
        return NULL;

    return ctrl->use_event;
}

static char *_get_duration_str(time_t start_time, time_t finish_time, bool long_flag)
{
    time_t start_sec = get_display_time_by_timezone(start_time);
    time_t finish_sec = get_display_time_by_timezone(finish_time);

    if (long_flag)
        return app_richepg_duration_str_get(start_sec, finish_sec, DURATION_WITH_DATE | DURATION_FULL_DATE);
    else
        return app_richepg_duration_str_get(start_sec, finish_sec, DURATION_SHORT_JOINER);
}

static void _update_bg_color(int item_sel, int event_flag)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, item_sel+1);
    if (event_flag & CUR_EPG_FLAG)
        GUI_SetProperty(wgt, "backcolor", CUR_BG_COLOR);
    else if (event_flag & PVR_EPG_FLAG)
        GUI_SetProperty(wgt, "backcolor", PVR_BG_COLOR);
    else if (event_flag & PLAY_EPG_FLAG)
        GUI_SetProperty(wgt, "backcolor", PLAY_BG_COLOR);
    else
        GUI_SetProperty(wgt, "backcolor", IDLE_BG_COLOR);

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, item_sel+1);
    if (event_flag & CUR_PLAY_FLAG)
        GUI_SetProperty(wgt, "forecolor", PLAY_FG_COLOR);
    else
        GUI_SetProperty(wgt, "forecolor", ITEM_FG_COLOR);
}

static int _update_book_display(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    RichepgEpgBriefChannel *cinfo = NULL;
    RichepgEpgBriefEvent *einfo = NULL;
    EutelChannelArg *channel = NULL;
    RichepgEpgEvent *event = NULL;
    GxTime time = {0};
    int epg_sel = -1;

    if (ctrl->cur_index >= ctrl->use_total)
    {
        GUI_SetProperty(ctrl->wgt.img_book, "state", "hide");
        GUI_SetProperty(ctrl->wgt.txt_book, "state", "hide");
        return -1;
    }
#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        GUI_SetProperty(ctrl->wgt.img_book, "state", "show");
        GUI_SetProperty(ctrl->wgt.txt_book, "state", "show");
        app_set_widget_string(ctrl->wgt.txt_book, STR_ID_DELETE);
        return 0;
    }
#endif
    epg_sel = ctrl->use_array[ctrl->cur_index];
    cinfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos];
    einfo = &cinfo->event_array[ctrl->epg_array[epg_sel].epg_pos];
    channel = _get_channel(ctrl->cur_index);
    event = _get_event(ctrl->cur_index, false);
    if (!channel || !event)
        return -1;
    GxCore_GetLocalTime(&time);

    _update_book_flag(time.seconds, channel->prog_id, einfo);
    _update_bg_color(ctrl->cur_index % EPG_ITEMS_PER_PAGE, einfo->event_flag);

    if (einfo->start_time < time.seconds)
    {
        GUI_SetProperty(ctrl->wgt.img_book, "state", "hide");
        GUI_SetProperty(ctrl->wgt.txt_book, "state", "hide");
    }
    else
    {
        GUI_SetProperty(ctrl->wgt.img_book, "state", "show");
        GUI_SetProperty(ctrl->wgt.txt_book, "state", "show");
        if ((einfo->event_flag & PLAY_EPG_FLAG) || (einfo->event_flag & PVR_EPG_FLAG))
        {
            app_set_widget_string(ctrl->wgt.txt_book, STR_ID_DELETE);
        }
        else
        {
            app_set_widget_string(ctrl->wgt.txt_book, STR_ID_BOOK);
        }
    }

    return 0;
}

static void clear_eutel_epg2_info(bool clear_ch, bool clear_epg)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    if (clear_ch)
    {
        app_set_widget_string(ctrl->wgt.text_ch_name, STR_ID_BLANK);
        GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", STR_ID_BLANK);
    }
    if (clear_epg)
    {
        app_set_widget_string(ctrl->wgt.text_item_duration, STR_ID_BLANK);
        app_set_widget_string(ctrl->wgt.text_item_name, STR_ID_BLANK);
        app_set_widget_string(ctrl->wgt.text_item_parent, STR_ID_BLANK);
        app_set_widget_string(ctrl->wgt.text_item_genres, STR_ID_BLANK);
        app_set_widget_string(ctrl->wgt.text_item_brief, STR_ID_BLANK);
    }
}

static void display_eutel_epg2_info(void)
{
#define _BUFFER_LEN 256
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelArg *channel = NULL;
    RichepgEpgEvent *event = NULL;
    char *tmp_str = NULL;
    char rate_buf[8] = {0};
    char key[32] = {0};

    _update_book_display();
    app_eutel_epg2_change_ok_tip();
    channel = _get_channel(ctrl->cur_index);
    if (!channel)
    {
        clear_eutel_epg2_info(true, false);
    }
    else
    {
        app_set_widget_string(ctrl->wgt.text_ch_name, channel->channel_name);
        if (ctrl->wgt.chlogo_bg_flag)
        {
            ctrl->wgt.chlogo_bg_flag = false;
            GUI_SetProperty(ctrl->wgt.text_ch_logo, "backcolor", IDLE_BG2_COLOR);
        }
        else
        {
            ctrl->wgt.chlogo_bg_flag = true;
            GUI_SetProperty(ctrl->wgt.text_ch_logo, "backcolor", IDLE_BG_COLOR);
        }
        if (channel->logo_flag == 1)
        {
            snprintf(key, sizeof(key), EUTEL_CH_KEY, channel->channel_id);
            GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", key);
        }
        else
        {
            GUI_SetProperty(ctrl->wgt.img_ch_logo, "img", eutel_lineup_logo_key_get());
        }
    }

    event = _get_event(ctrl->cur_index, true);
    if (!event || event->start_time == 0)
    {
        clear_eutel_epg2_info(false, true);
#if CONTENT_TIMER_SUPPORT
        if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
        {
            EutelTimerIndex *timer_index = &ctrl->timer_index_array[ctrl->cur_index];
            EutelTimerData *timer_data = &ctrl->timer_data_array[timer_index->timer_sel];
            tmp_str = _get_duration_str(timer_index->start_time, timer_index->start_time + timer_data->duration, true);
            app_set_widget_string(ctrl->wgt.text_item_duration, tmp_str);
        }
#endif
    }
    else
    {
        RichepgEpgBriefChannel *cinfo = NULL;
        RichepgEpgBriefEvent *einfo = NULL;
        int epg_sel = -1;

        epg_sel = ctrl->use_array[ctrl->cur_index];
        if (epg_sel < 0)
            return;
        cinfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos];
        einfo = &cinfo->event_array[ctrl->epg_array[epg_sel].epg_pos];

        tmp_str = _get_duration_str(event->start_time, event->finish_time, true);
        app_set_widget_string(ctrl->wgt.text_item_duration, tmp_str);

        app_set_widget_string(ctrl->wgt.text_item_name, event->event_title);

        if (event->parent_rate)
        {
            snprintf(rate_buf, sizeof(rate_buf), "%d", event->parent_rate);
            app_set_widget_string(ctrl->wgt.text_item_parent, rate_buf);
        }
        else
        {
            app_set_widget_string(ctrl->wgt.text_item_parent, STR_ID_BLANK);
        }

        tmp_str = eutel_array_name_get(event->genres_num, event->genres_ids, EUTEL_CONTENT_GENRE);
        app_set_widget_string(ctrl->wgt.text_item_genres, tmp_str);

        tmp_str = event->event_desc;
        if (!tmp_str || strlen(tmp_str) == 0)
        {
            if (0 == richepg_epg_event_info_get_by_time_from_usb(ctrl->use_event->start_time, cinfo->channel_id, einfo->event_id, event, EUTEL_EPG_EVENT_SIZE))
                tmp_str = event->event_desc;
        }
        if (!tmp_str || strlen(tmp_str) == 0)
            tmp_str = app_richepg_translate_str(STR_ID_NO_INFO);
        app_set_widget_string(ctrl->wgt.text_item_brief, tmp_str);
    }
}

static void app_eutel_epg2_info_update(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    if (ctrl->use_total == 0  || ctrl->cur_index < 0 || ctrl->cur_index >= ctrl->use_total)
        clear_eutel_epg2_info(true, true);
    else
        display_eutel_epg2_info();
}

static void app_eutel_epg2_change_focus_sel(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};
    int old_sel = 0;
    int new_sel = 0;
    int first_day_sel = 0;

#if MORE_GROUP_SUPPORT
    first_day_sel = EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE + 1;
#else
    first_day_sel = EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE;
#endif

    // cal new_focus_sel
    old_sel = (ctrl->wgt.reverse_flag) ? ctrl->rfocus_sel : ctrl->focus_sel;
    switch(ctrl->area_sel)
    {
        case AREA_ITEM:
            if (ctrl->use_total == 0)
                new_sel = -1;
            else
                new_sel = ctrl->cur_index % EPG_ITEMS_PER_PAGE;
            break;
        case AREA_GROUP:
            new_sel = ctrl->group_sel % EPG_GROUPS_PER_PAGE + EPG_ITEMS_PER_PAGE;
            break;
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            new_sel = EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE;
            break;
#endif
        case AREA_DAY:
            new_sel = ctrl->day_sel % EPG_DAYS_PER_PAGE + first_day_sel;
            break;
        default:
            break;
    }
    (ctrl->wgt.reverse_flag) ? (ctrl->rfocus_sel = new_sel) : (ctrl->focus_sel = new_sel);
    if (old_sel == new_sel)
        return;

    // set unfocus img
    if (old_sel < 0)
    {
        // do nothing
    }
    else if (old_sel < EPG_ITEMS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, old_sel+1);
        GUI_SetProperty(wgt, "img", ITEM_UNFOCUS_IMG);
    }
    else if (old_sel < EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, old_sel-EPG_ITEMS_PER_PAGE+1);
        GUI_SetProperty(wgt, "focus_img", GROUP_FOCUS_IMG);
    }
#if MORE_GROUP_SUPPORT
    else if (old_sel == EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, EPG_GROUPS_PER_PAGE+1);
        GUI_SetProperty(wgt, "focus_img", GROUP_FOCUS_IMG);
    }
#endif
    else
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.day_item_common, old_sel-first_day_sel+1);
        if (new_sel >= first_day_sel)
        {
            if (old_sel == first_day_sel + EPG_DAYS_PER_PAGE - 1)
                GUI_SetProperty(wgt, "unfocus_img", GROUP_UNFOCUS2_IMG);
            else
                GUI_SetProperty(wgt, "unfocus_img", GROUP_UNFOCUS_IMG);
        }
        else
        {
            GUI_SetProperty(wgt, "unfocus_img", GROUP_FOCUS_IMG);
        }
    }

    // set focus img
    if (new_sel < 0)
    {
        // do nothing
    }
    else if (new_sel < EPG_ITEMS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, new_sel+1);
        GUI_SetProperty(wgt, "img", ITEM_FOCUS_IMG);
    }
    else if (new_sel < EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, new_sel-EPG_ITEMS_PER_PAGE+1);
        GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
    }
#if MORE_GROUP_SUPPORT
    else if (new_sel == EPG_ITEMS_PER_PAGE + EPG_GROUPS_PER_PAGE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, EPG_GROUPS_PER_PAGE+1);
        GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
    }
#endif
    else
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.day_item_common, new_sel-first_day_sel+1);
        GUI_SetProperty(wgt, "unfocus_img", GROUP_SELECT_IMG);
    }

    // set focus btn
#if MORE_GROUP_SUPPORT
    if (ctrl->area_sel == AREA_MORE)
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, EPG_GROUPS_PER_PAGE+1);
        GUI_SetFocusWidget(wgt);
    }
    else
#endif
    {
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, ctrl->group_sel % EPG_GROUPS_PER_PAGE + 1);
        GUI_SetFocusWidget(wgt);
    }
}

static void app_eutel_epg2_change_ok_tip(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    switch(ctrl->area_sel)
    {
        case AREA_ITEM:
            {
                EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
                EutelChannelArg *channel = _get_channel(ctrl->cur_index);

                GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", "s_ts_ok");
                if (!channel)
                {
                    app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_PLAY);
                }
                else
                {
                    int channel_id = eutel_cur_channel_id_get();
                    if (_check_play_flag(channel_id, channel->channel_id))
                    {
                        app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_PLAY);
                    }
                    else
                    {
                        app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_EXIT);
                    }
                }
            }
            break;
#if CONTENT_TIMER_SUPPORT
        case AREA_GROUP:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", "s_ts_ok");
            if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
            {
                app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_RETURN);
                app_set_widget_string(ctrl->wgt.txt_red_tip, STR_ID_ADD);
                app_set_widget_string(ctrl->wgt.txt_green_tip, STR_ID_EDIT);
            }
            else
            {
                app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_TIMERS);
                app_set_widget_string(ctrl->wgt.txt_red_tip, STR_ID_PAGE_SUB);
                app_set_widget_string(ctrl->wgt.txt_green_tip, STR_ID_PAGE_PLUS);
            }
            break;
#else
        case AREA_GROUP:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", STR_ID_BLANK);
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_BLANK);
            break;
#endif
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", "s_ts_ok");
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_MORE);
            break;
#endif
        case AREA_DAY:
            GUI_SetProperty(ctrl->wgt.img_ok_tip, "img", STR_ID_BLANK);
            app_set_widget_string(ctrl->wgt.txt_ok_tip, STR_ID_BLANK);
            break;
        default:
            break;
    }
}

void app_eutel_epg2_ok_tip_refresh(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    const char *focus = GUI_GetFocusWindow();
    if (focus && ctrl->wgt.wnd_name && strcmp(ctrl->wgt.wnd_name, focus) == 0)
        app_eutel_epg2_change_ok_tip();
}

static void app_eutel_epg2_change_area_sel(AreaSel new_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    ctrl->area_sel = new_sel;
    app_eutel_epg2_change_focus_sel();
#if MORE_GROUP_SUPPORT
    if (new_sel == AREA_MORE)
        app_eutel_epg2_content_filter_pop(AREA_GROUP);
#endif
    app_eutel_epg2_change_ok_tip();
}

#if TIMER_FLUSH_LOGO
static void _copy_epg_logo_task_exec(void *arg)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int ret = -1;
    int i = 0;
    int choice = 0;
    char src[EPG_FULL_NAME_LEN];
    char dst[EPG_FULL_NAME_LEN];
    char *path = NULL;

    GxCore_ThreadDetach();
    while (1)
    {
        for (i = 0; i < EPG_ITEMS_PER_PAGE; i++)
        {
            GxCore_MutexLock(ctrl->mgr.mutex);
            if (ctrl->mgr.cache_stop_rq == 1)
                choice = 1;
            else if (ctrl->mgr.logo_stop_rq == 1)
                choice = 2;
            else if (ctrl->mgr.logo_copy_rq == 1)
            {
                if (ctrl->mgr.logo_para[i].state == LOGO_ADD && ctrl->mgr.logo_para[i].name != NULL)
                {
                    choice = 3;
                    ctrl->mgr.logo_para[i].state = LOGO_COPY;
                    snprintf(src, EPG_FULL_NAME_LEN, "%s", ctrl->mgr.logo_para[i].name);
                }
                else
                {
                    choice = 4;
                }
            }
            else
            {
                choice = 5;
            }
            GxCore_MutexUnlock(ctrl->mgr.mutex);

            if (choice == 1)
                goto end;
            if (choice == 2)
                break;
            if (choice == 3)
            {
                snprintf(dst, EPG_FULL_NAME_LEN, CACHE_LOGO_COMMON, i+1);
                if(true == richepg_file_splice_special_path_check(src))
                    path = richepg_files_splice_memory_file_path_get(src);

                ret = richepg_copy_file_to_file(path, dst, 8192);
                if(true == richepg_file_splice_special_path_check(src))
                    richepg_files_splice_memory_file_delete(src, path);
                GxCore_MutexLock(ctrl->mgr.mutex);
                if (ctrl->mgr.logo_para[i].state == LOGO_COPY)
                {
                    ctrl->mgr.logo_para[i].state = (ret > 0) ? LOGO_FIN : LOGO_IDLE;
                    if (i+1 == EPG_ITEMS_PER_PAGE)
                        ctrl->mgr.logo_copy_rq = 0;
                }
                GxCore_MutexUnlock(ctrl->mgr.mutex);
            }
        }

        GxCore_MutexLock(ctrl->mgr.mutex);
        if (ctrl->mgr.logo_stop_rq == 1)
            ctrl->mgr.logo_stop_rq = 0;
        GxCore_MutexUnlock(ctrl->mgr.mutex);
        GxCore_ThreadDelay(10);
    }

end:
    GxCore_MutexLock(ctrl->mgr.mutex);
    for (i = 0; i < EPG_ITEMS_PER_PAGE; i++)
    {
        snprintf(dst, EPG_FULL_NAME_LEN, CACHE_LOGO_COMMON, i+1);
        if (GxCore_FileExists(dst) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(dst);
    }
    ctrl->mgr.thread_cache_run = 0;
    ctrl->mgr.cache_stop_rq = 0;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}

static void app_eutel_epg2_clean_logo_data(bool set_flag)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int i = 0;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if (set_flag)
    {
        ctrl->mgr.logo_stop_rq = 1;
        ctrl->mgr.logo_copy_rq = 0;
    }
    for (i = 0; i < EPG_ITEMS_PER_PAGE; i++)
    {
        APP_FREE(ctrl->mgr.logo_para[i].name);
    }
    memset(ctrl->mgr.logo_para, 0, sizeof(ctrl->mgr.logo_para));
    GxCore_MutexUnlock(ctrl->mgr.mutex);
}

static int _refresh_logo_timer_exec(void *userdata)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    const char *focus = NULL;

    if (ctrl->detail_flag == 0)
    {
        focus = GUI_GetFocusWindow();
        if (focus && strcmp(ctrl->wgt.wnd_name, focus) == 0)
        {
            bool flag = false;
            int i, new_sel, first_sel, show_num;
            char key[ITEM_COMMON_SIZE] = {0};
            char wgt[ITEM_COMMON_SIZE] = {0};
            char logo[EPG_FULL_NAME_LEN] = {0};

            new_sel = ctrl->cur_index;
            first_sel = new_sel - new_sel % EPG_ITEMS_PER_PAGE;
            show_num = ctrl->use_total - first_sel;
            if (show_num > EPG_ITEMS_PER_PAGE)
                show_num = EPG_ITEMS_PER_PAGE;
            for (i = 0; i < show_num; i++)
            {
                flag = false;
                GxCore_MutexLock(ctrl->mgr.mutex);
                if (ctrl->mgr.logo_para[i].state == LOGO_FIN)
                {
                    flag = true;
                    ctrl->mgr.logo_para[i].state = LOGO_IDLE;
                }
                GxCore_MutexUnlock(ctrl->mgr.mutex);

                if (flag)
                {
                    snprintf(key, ITEM_COMMON_SIZE, ITEM_KEY_COMMON, i+1);
                    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i+1);
                    snprintf(logo, EPG_FULL_NAME_LEN, CACHE_LOGO_COMMON, i+1);
                    if (GxCore_FileExists(logo) == GXCORE_FILE_EXIST)
                    {
                        gal_add_key_path(key, logo);
                        GUI_SetProperty(wgt, "img", key);
                    }
                }
            }

            flag = true;
            GxCore_MutexLock(ctrl->mgr.mutex);
            for (i = 0; i < EPG_ITEMS_PER_PAGE; i++)
            {
                if (ctrl->mgr.logo_para[i].state != LOGO_IDLE)
                {
                    flag = false;
                    break;
                }

            }
            GxCore_MutexUnlock(ctrl->mgr.mutex);

            if (flag)
            {
                APP_TIMER_REMOVE(ctrl->mgr.time_refresh);
            }
        }
    }

    return 0;
}

static void app_eutel_epg2_thread_stop_set(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    if (ctrl->mgr.mutex == 0)
        GxCore_MutexCreate(&ctrl->mgr.mutex);
    GxCore_MutexLock(ctrl->mgr.mutex);
    if (ctrl->mgr.thread_cache_run == 1)
        ctrl->mgr.cache_stop_rq = 1;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
    APP_TIMER_REMOVE(ctrl->mgr.time_refresh);
}

static bool app_eutel_epg2_thread_run_check(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    bool flag = false;

    GxCore_MutexLock(ctrl->mgr.mutex);
    if (ctrl->mgr.thread_cache_run == 1)
        flag = true;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    return flag;
}

static int app_eutel_epg2_thread_task_start(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int thread_id = 0;
    GxCore_MutexLock(ctrl->mgr.mutex);
    ctrl->mgr.thread_cache_run = 1;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
    GxCore_ThreadCreate("copy_epg_logo", &thread_id, _copy_epg_logo_task_exec, NULL, 1024*10, GXOS_DEFAULT_PRIORITY+1);
    return 0;
}

static int app_eutel_epg2_timer_task_start(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    GxCore_MutexLock(ctrl->mgr.mutex);
    ctrl->mgr.logo_stop_rq = 1;
    ctrl->mgr.logo_copy_rq = 1;
    GxCore_MutexUnlock(ctrl->mgr.mutex);
    APP_TIMER_ADD(ctrl->mgr.time_refresh, _refresh_logo_timer_exec, 20, TIMER_REPEAT);
    return 0;
}
#endif

static void _eutel_epg2_hide_one_item(int i)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, i+1);
    GUI_SetProperty(wgt, "img", ITEM_UNFOCUS_IMG);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i+1);
    GUI_SetProperty(wgt, "img", STR_ID_BLANK);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, i+1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
    GUI_SetProperty(wgt, "backcolor", IDLE_BG_COLOR);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_time_common, i+1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, i+1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
}

static void _eutel_epg2_show_one_item(int i, int first_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelArg *channel = NULL;
    RichepgEpgEvent *event = NULL;
    RichepgEpgBriefEvent *einfo = NULL;
    GxTime time = {0};
    char *logo_key = EUTEL_EPG_EVENT_KEY_DF;
    char *tmp_str = NULL;
    char *logo_path = NULL;
    int sel = 0;
    int epg_sel = -1;
#if !TIMER_FLUSH_LOGO
    char key[ITEM_COMMON_SIZE] = {0};
#endif
    char wgt[ITEM_COMMON_SIZE] = {0};
    char buf[128] = {0};

    sel = first_sel + i;
    channel = _get_channel(sel);
    if (!channel)
    {
        _eutel_epg2_hide_one_item(i);
        return;
    }
    if (channel->channel_name)
        snprintf(buf, sizeof(buf), "ch%d %s", channel->lcn_ids[0], channel->channel_name);
    else
        snprintf(buf, sizeof(buf), "ch%d", channel->lcn_ids[0]);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, i+1);
    app_set_widget_string(wgt, buf);

#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        EutelTimerIndex *timer_index = &ctrl->timer_index_array[sel];
        EutelTimerData *timer_data = &ctrl->timer_data_array[timer_index->timer_sel];
        int event_flag = 0;

        event = _get_event(sel, false);
        tmp_str = (event) ? event->event_title : channel->channel_name;
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, i+1);
        app_set_widget_string(wgt, tmp_str);

        if (timer_data->book_type == BOOK_PROGRAM_PLAY)
        {
            event_flag |= PLAY_EPG_FLAG;
        }
        else if (timer_data->book_type == BOOK_PROGRAM_PVR)
        {
            event_flag |= PVR_EPG_FLAG;
        }
        if (timer_data->prog_id == eutel_cur_channel_prog_id_get())
        {
            event_flag |= CUR_PLAY_FLAG;
        }
        _update_bg_color(i, event_flag);

        tmp_str = _get_duration_str(timer_index->start_time, timer_index->start_time + timer_data->duration, false);
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_time_common, i+1);
        app_set_widget_string(wgt, tmp_str);

        GUI_SetProperty(wgt, "img", EUTEL_EPG_EVENT_KEY_DF);
        GxCore_MutexLock(ctrl->mgr.mutex);
        ctrl->mgr.logo_para[i].state = LOGO_ADD;
        ctrl->mgr.logo_para[i].name = logo_path;
        logo_path = NULL;
        GxCore_MutexUnlock(ctrl->mgr.mutex);

        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i+1);
        tmp_str = (event) ? event->event_logo : NULL;
        if (tmp_str)
            logo_path = richepg_get_logo_path(tmp_str, RICHEPG_LOGO_EVENT);
        if (logo_path) {
#if TIMER_FLUSH_LOGO
            GUI_SetProperty(wgt, "img", logo_key);
            GxCore_MutexLock(ctrl->mgr.mutex);
            ctrl->mgr.logo_para[i].state = LOGO_ADD;
            APP_FREE(ctrl->mgr.logo_para[i].name);
            ctrl->mgr.logo_para[i].name = GxCore_Strdup(logo_path);
            GxCore_MutexUnlock(ctrl->mgr.mutex);
#else
            snprintf(key, ITEM_COMMON_SIZE, ITEM_KEY_COMMON, i+1);
            gal_add_key_path(key, logo_path);
            logo_key = key;
#endif
        }
        else
        {
            GUI_SetProperty(wgt, "img", logo_key);
        }
        return;
    }
#endif

    event = _get_event(sel, false);
    if (!event)
    {
        _eutel_epg2_hide_one_item(i);
        return;
    }
    GxCore_GetLocalTime(&time);
    epg_sel = ctrl->use_array[sel];
    einfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos].event_array[ctrl->epg_array[epg_sel].epg_pos];
    _update_book_flag(time.seconds, channel->prog_id, einfo);

    tmp_str = event->event_title;
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_title_common, i+1);
    app_set_widget_string(wgt, tmp_str);
    _update_bg_color(i, einfo->event_flag);

    tmp_str = _get_duration_str(event->start_time, event->finish_time, false);
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_time_common, i+1);
    app_set_widget_string(wgt, tmp_str);

    GUI_SetProperty(wgt, "img", EUTEL_EPG_EVENT_KEY_DF);
    GxCore_MutexLock(ctrl->mgr.mutex);
    ctrl->mgr.logo_para[i].state = LOGO_ADD;
    ctrl->mgr.logo_para[i].name = logo_path;
    logo_path = NULL;
    GxCore_MutexUnlock(ctrl->mgr.mutex);

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_logo_common, i+1);
    tmp_str = event->event_logo;
    if (tmp_str)
        logo_path = richepg_get_logo_path(tmp_str, RICHEPG_LOGO_EVENT);
    if (logo_path) {
#if TIMER_FLUSH_LOGO
        GUI_SetProperty(wgt, "img", logo_key);
        GxCore_MutexLock(ctrl->mgr.mutex);
        ctrl->mgr.logo_para[i].state = LOGO_ADD;
        APP_FREE(ctrl->mgr.logo_para[i].name);
        ctrl->mgr.logo_para[i].name = GxCore_Strdup(logo_path);
        GxCore_MutexUnlock(ctrl->mgr.mutex);
#else
        snprintf(key, ITEM_COMMON_SIZE, ITEM_KEY_COMMON, i+1);
        gal_add_key_path(key, logo_path);
        logo_key = key;
#endif
    }
    else
    {
        GUI_SetProperty(wgt, "img", logo_key);
    }
}

static int app_eutel_epg2_update_item_all(int last_sel, int new_sel, bool focus_change)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int first_sel, show_num;
    int last_page, new_page, total_page;
    char buffer[32] = {0};
    int i;

    ctrl->cur_index = new_sel;
    if (focus_change)
        app_eutel_epg2_change_focus_sel();

    if (ctrl->use_total == 0 || new_sel < 0)
    {
#if TIMER_FLUSH_LOGO
        app_eutel_epg2_clean_logo_data(true);
#endif
        for (i = 0; i < EPG_ITEMS_PER_PAGE; i++)
        {
            _eutel_epg2_hide_one_item(i);
        }
        app_set_widget_string(ctrl->wgt.text_item_page, "0/0");
    }
    else
    {
        last_page = last_sel / EPG_ITEMS_PER_PAGE + 1;
        new_page = new_sel / EPG_ITEMS_PER_PAGE + 1;
        if (last_sel < 0 || last_page != new_page)
        {
#if TIMER_FLUSH_LOGO
            app_eutel_epg2_clean_logo_data(true);
#endif
            first_sel = new_sel - new_sel % EPG_ITEMS_PER_PAGE;
            show_num = ctrl->use_total - first_sel;
            if (show_num > EPG_ITEMS_PER_PAGE)
                show_num = EPG_ITEMS_PER_PAGE;
            for (i = 0; i < show_num; i++)
            {
                _eutel_epg2_show_one_item(i, first_sel);
            }
            for (i = show_num; i < EPG_ITEMS_PER_PAGE; i++)
            {
                _eutel_epg2_hide_one_item(i);
            }
#if TIMER_FLUSH_LOGO
            app_eutel_epg2_timer_task_start();
#endif
        }

        total_page = (ctrl->use_total - 1) / EPG_ITEMS_PER_PAGE + 1;
        snprintf(buffer, sizeof(buffer), "%d/%d", new_page, total_page);
        app_set_widget_string(ctrl->wgt.text_item_page, buffer);
    }

    app_eutel_epg2_info_update();

    return 0;
}

static int app_eutel_epg2_update_play_font_color(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int sel, first_sel, show_num;
    char wgt[ITEM_COMMON_SIZE] = {0};
    int prog_id = eutel_cur_channel_prog_id_get();
    int i;

    if (ctrl->use_total == 0)
        return -1;

    app_euel_epg2_update_cur_play_flag();
    first_sel = ctrl->cur_index - ctrl->cur_index % EPG_ITEMS_PER_PAGE;
    show_num = ctrl->use_total - first_sel;
    if (show_num > EPG_ITEMS_PER_PAGE)
        show_num = EPG_ITEMS_PER_PAGE;
    for (i = 0; i < show_num; i++)
    {
        EutelChannelArg *channel = NULL;
        sel = first_sel + i;
        channel = _get_channel(sel);
        if (channel)
        {
            snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_ch_common, i+1);
            if (channel->prog_id == prog_id)

                GUI_SetProperty(wgt, "forecolor", PLAY_FG_COLOR);
            else
                GUI_SetProperty(wgt, "forecolor", ITEM_FG_COLOR);
        }
    }

    return 0;
}

static void _eutel_epg2_hide_one_group(int i)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, i+1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
}

static void _eutel_epg2_show_one_group(int i, int first_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};
    int sel = 0;

    sel = first_sel + i;
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, i+1);
#if CONTENT_TIMER_SUPPORT
    if (sel == CONTENT_TIMER_GROUP_SEL)
    {
        GUI_SetProperty(wgt, "string", STR_ID_TIMERS);
        GUI_SetProperty(wgt, "forecolor", TIMER_FG_COLOR);
    }
    else
    {
        GUI_SetProperty(wgt, "forecolor", GROUP_FG_COLOR);
    }
#endif
#if CONTENT_ALL_SUPPORT
    if (sel == CONTENT_ALL_GROUP_SEL)
        GUI_SetProperty(wgt, "string", STR_ID_ALL);
#endif
    if (sel >= CONTENT_PRIORITY_NUM
#if CONTENT_TIMER_SUPPORT
            && sel != CONTENT_TIMER_GROUP_SEL
#endif
            )
        GUI_SetProperty(wgt, "string", ch_ctrl->content_array[sel-CONTENT_PRIORITY_NUM].name);

    if (sel == ctrl->group_sel)
    {
#if MORE_GROUP_SUPPORT
        if (ctrl->area_sel != AREA_MORE)
#endif
            GUI_SetFocusWidget(wgt);
    }
}

static void _eutel_epg2_hide_one_day(int i)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};

    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.day_item_common, i+1);
    GUI_SetProperty(wgt, "string", STR_ID_BLANK);
}

static void _eutel_epg2_show_one_day(int i, int first_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};
    int sel = 0;

    sel = first_sel + i;
    snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.day_item_common, i+1);
    GUI_SetProperty(wgt, "string", s_day_array[sel]);

    if (sel == ctrl->day_sel)
    {
        if (ctrl->area_sel != AREA_DAY)
            GUI_SetProperty(wgt, "unfocus_img", GROUP_FOCUS_IMG);
        else
            GUI_SetProperty(wgt, "unfocus_img", GROUP_SELECT_IMG);
    }
    else
    {
        if ((sel + 1) % EPG_DAYS_PER_PAGE != 0)
            GUI_SetProperty(wgt, "unfocus_img", GROUP_UNFOCUS_IMG);
        else
            GUI_SetProperty(wgt, "unfocus_img", GROUP_UNFOCUS2_IMG);
    }
}

static int app_eutel_epg2_refresh_all(int last_group, int new_group, int last_day, int new_day, AreaSel area_sel, int (*set_cur_index)(void))
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    int first_sel, show_num;
    int last_page, new_page;
    int i;

    if (new_group >= ch_ctrl->content_total+CONTENT_RESERVED_NUM)
        new_group = 0;
    if (new_day >= ctrl->day_total)
        new_day = 0;
    if (last_group == new_group && last_day == new_day)
        return 0;

#if CONTENT_TIMER_SUPPORT
    if (new_group != CONTENT_TIMER_GROUP_SEL)
        ctrl->group_return = new_group;
#endif
    ctrl->area_sel = area_sel;
    ctrl->group_sel = new_group;
    ctrl->day_sel = new_day;

    if (last_group < 0 || last_group != new_group
            || last_day < 0 || last_day != new_day)
    {
        app_eutel_epg2_data_filter();
        if (set_cur_index)
            app_eutel_epg2_update_item_all(-1, set_cur_index(), false);
        else
            app_eutel_epg2_update_item_all(-1, 0, false);
        app_eutel_epg2_change_focus_sel();
    }

    last_page = last_group / EPG_GROUPS_PER_PAGE + 1;
    new_page = new_group / EPG_GROUPS_PER_PAGE + 1;
    if (last_group < 0 || last_page != new_page)
    {
        first_sel = new_group - new_group % EPG_GROUPS_PER_PAGE;
        show_num = ch_ctrl->content_total+CONTENT_RESERVED_NUM - first_sel;
        if (show_num > EPG_GROUPS_PER_PAGE)
            show_num = EPG_GROUPS_PER_PAGE;
        for (i = 0; i < show_num; i++)
        {
            _eutel_epg2_show_one_group(i, first_sel);
        }
        for (i = show_num; i < EPG_GROUPS_PER_PAGE; i++)
        {
            _eutel_epg2_hide_one_group(i);
        }
    }

    last_page = last_day / EPG_DAYS_PER_PAGE + 1;
    new_page = new_day / EPG_DAYS_PER_PAGE + 1;
    if (last_day < 0 || last_page != new_page)
    {
        first_sel = new_day - new_day % EPG_DAYS_PER_PAGE;
        show_num = ctrl->day_total - first_sel;
        if (show_num >= EPG_DAYS_PER_PAGE)
            show_num = EPG_DAYS_PER_PAGE;
        for (i = 0; i < show_num; i++)
        {
            _eutel_epg2_show_one_day(i, first_sel);
        }
        for (i = show_num; i < EPG_DAYS_PER_PAGE; i++)
        {
            _eutel_epg2_hide_one_day(i);
        }
    }

    return 0;
}

static int app_eutel_epg2_update_all(int last_group, int new_group, int last_day, int new_day, AreaSel area_sel)
{
    return app_eutel_epg2_refresh_all(last_group, new_group, last_day, new_day, area_sel, NULL);
}

void app_eutel_epg_timer_show_update(const char *focus_wnd)
{
#if CONTENT_TIMER_SUPPORT
    if (strcmp(focus_wnd, WND_EUTEL_EPG2) == 0 ) //|| strcmp(focus_wnd, WND_EUTEL_REPG2) == 0)
    {
        EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
        if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
        {
            app_eutel_epg2_update_all(-1, ctrl->group_sel, ctrl->day_sel, ctrl->day_sel, ctrl->area_sel);
        }
    }
#endif
}

#if FOCUS_LAST_ITEM_SUPPORT
int app_eutel_epg2_bak_timer_stop(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    ctrl->index_bak.bak_flag = false;
    APP_TIMER_REMOVE(ctrl->index_bak.bak_timer);
    return 0;
}

static int _eutel_epg2_bak_timer_exec(void* usrdata)
{
    return app_eutel_epg2_bak_timer_stop();
}

static int app_eutel_epg2_bak_timer_start(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int msec = app_richepg_epg_keep_sec_get() * 1000;

    app_eutel_epg2_bak_timer_stop();
    if (msec > 0)
    {
        ctrl->index_bak.bak_flag = true;
        ctrl->index_bak.group_sel = ctrl->group_sel;
        ctrl->index_bak.day_sel = ctrl->day_sel;
        ctrl->index_bak.cur_index = ctrl->cur_index;
        APP_TIMER_ADD(ctrl->index_bak.bak_timer, _eutel_epg2_bak_timer_exec, msec, TIMER_REPEAT);
    }

    return 0;
}

static int _get_cur_index_by_bak_index(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    if (ctrl->index_bak.cur_index < ctrl->use_total)
        return ctrl->index_bak.cur_index;
    return 0;
}

#else
int app_eutel_epg2_bak_timer_stop(void)
{
    // NOP
}
#endif

static int app_eutel_epg2_content_filter_pop(AreaSel area_sel)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;

    int sel = 0;
    int i = 0;
    int total = 0;
    char **name_array = NULL;
    PopList pop_list;
    total = ch_ctrl->content_total + CONTENT_RESERVED_NUM;
    if ((name_array = GxCore_Calloc(total, sizeof(char*))) == NULL)
    {
        EUTEL_ERR("malloc failed!");
        return 0;
    }
#if CONTENT_ALL_SUPPORT
    name_array[i++] = STR_ID_ALL;
#endif
#if CONTENT_TIMER_SUPPORT
    --total;
    for (i = CONTENT_PRIORITY_NUM; i < total; i++)
    {
        name_array[i] = ch_ctrl->content_array[i-CONTENT_PRIORITY_NUM].name;
    }
    ++total;
    name_array[i++] = STR_ID_TIMERS;
#else
    for (i = CONTENT_PRIORITY_NUM; i < total; i++)
    {
        name_array[i] = ch_ctrl->content_array[i-CONTENT_PRIORITY_NUM].name;
    }
#endif

    memset(&pop_list, 0, sizeof(PopList));
    pop_list.sel = ctrl->group_sel;
    pop_list.show_num= false;
    pop_list.title = STR_ID_CONTENT_FILTER;
    pop_list.item_num = total;
    pop_list.item_content = name_array;
    sel = poplist_create(&pop_list);

    if ((sel < total && sel >= 0) && sel != ctrl->group_sel)
    {
        app_eutel_epg2_update_all(-1, sel, ctrl->day_sel, ctrl->day_sel, area_sel);
    }

    APP_FREE(name_array);
    return sel;
}

static int app_eutel_epg2_change_item_keypress(uint16_t key)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int total;
    int total_page, last_page;
    int last_sel, new_sel, item_sel, first_sel;

    if (ctrl->use_total == 0)
    {
        if (key == STBK_UP)
            app_eutel_epg2_change_area_sel(AREA_GROUP);
        else if (key == STBK_DOWN)
            app_eutel_epg2_change_area_sel(AREA_DAY);
        return EVENT_TRANSFER_STOP;
    }

    total = ctrl->use_total;
    total_page = (total - 1) / EPG_ITEMS_PER_PAGE + 1;

    last_sel = ctrl->cur_index;
    item_sel = last_sel % EPG_ITEMS_PER_PAGE;
    first_sel = last_sel - item_sel;
    last_page = last_sel / EPG_ITEMS_PER_PAGE + 1;

    switch(key)
    {
        case STBK_PAGE_UP:
            if (last_page == 1)
            {
                new_sel = (total_page - 1) * EPG_ITEMS_PER_PAGE + item_sel;
                if (new_sel >= total)
                    new_sel = total - 1;
            }
            else
            {
                new_sel = last_sel - EPG_ITEMS_PER_PAGE;
            }
            break;
        case STBK_PAGE_DOWN:
            if (last_page == total_page)
            {
                new_sel = item_sel;
            }
            else
            {
                new_sel = last_sel + EPG_ITEMS_PER_PAGE;
                if (new_sel >= total)
                    new_sel = total - 1;
            }
            break;
        case STBK_UP:
            if (last_sel - first_sel < EPG_ITEMS_ROW_NUMS)
            {
                app_eutel_epg2_change_area_sel(AREA_GROUP);
                return EVENT_TRANSFER_STOP;
            }
            else
            {
                new_sel = last_sel - EPG_ITEMS_ROW_NUMS;
            }
            break;
        case STBK_DOWN:
            if (last_sel - first_sel >= EPG_ITEMS_PER_PAGE - EPG_ITEMS_ROW_NUMS)
            {
                app_eutel_epg2_change_area_sel(AREA_DAY);
                return EVENT_TRANSFER_STOP;
            }
            else
            {
                new_sel = last_sel + EPG_ITEMS_ROW_NUMS;
                if (new_sel >= total)
                {
                    app_eutel_epg2_change_area_sel(AREA_DAY);
                    return EVENT_TRANSFER_STOP;
                }
            }
            break;
#if LEFT_RIGHT_SERIAL_MODE
        case STBK_LEFT:
            new_sel = last_sel - 1;
            if (new_sel < 0)
                new_sel = total - 1;
            break;
        case STBK_RIGHT:
            new_sel = last_sel + 1;
            if (new_sel >= total)
                new_sel = 0;
            break;
#else
        case STBK_LEFT:
            if (last_sel % EPG_ITEMS_ROW_NUMS == 0)
            {
                if (last_page == 1)
                {
                    new_sel = last_sel + (total_page - 1) * EPG_ITEMS_PER_PAGE  + EPG_ITEMS_ROW_NUMS - 1 ;
                }
                else
                {
                    new_sel = last_sel - EPG_ITEMS_PER_PAGE + EPG_ITEMS_ROW_NUMS - 1;
                }
                if (new_sel >= total)
                    new_sel = total - 1;
            }
            else
            {
                new_sel = last_sel - 1;
            }
            break;
        case STBK_RIGHT:
            if (last_sel == total - 1)
            {
                new_sel = item_sel - item_sel % EPG_ITEMS_ROW_NUMS;
            }
            else if ((last_sel + 1) % EPG_ITEMS_ROW_NUMS == 0)
            {
                if (last_page == total_page)
                {
                    new_sel = item_sel - item_sel % EPG_ITEMS_ROW_NUMS;
                }
                else
                {
                    new_sel = last_sel + EPG_ITEMS_PER_PAGE - item_sel % EPG_ITEMS_ROW_NUMS;
                }
            }
            else
            {
                new_sel = last_sel + 1;
            }

            if (new_sel >= total)
                new_sel = total - 1;
            break;
#endif
        default:
            return EVENT_TRANSFER_STOP;
    }

    app_eutel_epg2_update_item_all(last_sel, new_sel, true);

    return EVENT_TRANSFER_STOP;
}

static int app_eutel_epg2_change_group_keypress(uint16_t key)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    int group_sel = ctrl->group_sel;
    int group_max = 0;
#if MORE_GROUP_SUPPORT
    int item_sel = 0;
#endif

    group_max = ch_ctrl->content_total - 1 + CONTENT_RESERVED_NUM;
    switch (key)
    {
        case STBK_UP:
            app_eutel_epg2_change_area_sel(AREA_DAY);
            break;
        case STBK_DOWN:
            app_eutel_epg2_change_area_sel(AREA_ITEM);
            break;
#if MORE_GROUP_SUPPORT
        case STBK_LEFT:
            item_sel = group_sel % EPG_GROUPS_PER_PAGE;
            if (item_sel == 0)
                app_eutel_epg2_change_area_sel(AREA_MORE);
            else
                app_eutel_epg2_update_all(group_sel, group_sel-1, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            break;
        case STBK_RIGHT:
            item_sel = group_sel % EPG_GROUPS_PER_PAGE;
            if (item_sel == EPG_GROUPS_PER_PAGE-1 || group_sel == group_max)
                app_eutel_epg2_change_area_sel(AREA_MORE);
            else
                app_eutel_epg2_update_all(group_sel, group_sel+1, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            break;
#else
        case STBK_LEFT:
            if (group_sel == 0)
                app_eutel_epg2_update_all(group_sel, group_max, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            else
                app_eutel_epg2_update_all(group_sel, group_sel-1, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            break;
        case STBK_RIGHT:
            if (group_sel == group_max)
                app_eutel_epg2_update_all(group_sel, 0, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            else
                app_eutel_epg2_update_all(group_sel, group_sel+1, ctrl->day_sel, ctrl->day_sel, AREA_GROUP);
            break;
#endif
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

#if MORE_GROUP_SUPPORT
static int app_eutel_epg2_change_more_keypress(uint16_t key)
{
    switch (key)
    {
        case STBK_UP:
            app_eutel_epg2_change_area_sel(AREA_DAY);
            break;
        case STBK_DOWN:
            app_eutel_epg2_change_area_sel(AREA_ITEM);
            break;
        case STBK_LEFT:
        case STBK_RIGHT:
            app_eutel_epg2_change_area_sel(AREA_GROUP);
            break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}
#endif

static int app_eutel_epg2_change_day_keypress(uint16_t key)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int day_sel = ctrl->day_sel;
    int new_sel = 0;

    switch (key)
    {
        case STBK_UP:
            app_eutel_epg2_change_area_sel(AREA_ITEM);
            break;
        case STBK_DOWN:
            app_eutel_epg2_change_area_sel(AREA_GROUP);
            break;
        case STBK_LEFT:
            new_sel = day_sel-1;
            if (new_sel < 0) new_sel = ctrl->day_total - 1;
            app_eutel_epg2_update_all(ctrl->group_sel, ctrl->group_sel, day_sel, new_sel, AREA_DAY);
            break;
        case STBK_RIGHT:
            new_sel = day_sel+1;
            if (new_sel >= ctrl->day_total) new_sel = 0;
            app_eutel_epg2_update_all(ctrl->group_sel, ctrl->group_sel, day_sel, new_sel, AREA_DAY);
            break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

static int app_eutel_epg2_change_keypress(uint16_t key)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int ret = EVENT_TRANSFER_STOP;

    if (ctrl->wgt.reverse_flag)
    {
        if (key == STBK_LEFT)
            key = STBK_RIGHT;
        else if (key == STBK_RIGHT)
            key = STBK_LEFT;
    }

    if (key == STBK_PAGE_UP || key == STBK_PAGE_DOWN)
        app_eutel_epg2_change_item_keypress(key);

    switch (ctrl->area_sel)
    {
        case AREA_ITEM:
            app_eutel_epg2_change_item_keypress(key);
            break;
        case AREA_GROUP:
            app_eutel_epg2_change_group_keypress(key);
            break;
#if MORE_GROUP_SUPPORT
        case AREA_MORE:
            app_eutel_epg2_change_more_keypress(key);
            break;
#endif
        case AREA_DAY:
            app_eutel_epg2_change_day_keypress(key);
            break;
        default:
            break;
    }
    app_eutel_epg2_change_ok_tip();
    return ret;
}

static void app_eutel_epg2_details_hide(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    ctrl->detail_flag = 0;
    GUI_SetProperty(ctrl->wgt.details_duration, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_title, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_notepad, "string", STR_ID_BLANK);
    GUI_SetProperty(ctrl->wgt.details_duration, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_title, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_notepad, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_exit, "state", "hide");
    GUI_SetProperty(ctrl->wgt.details_back, "state", "hide");
}

static void app_eutel_epg2_details_show(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    RichepgEpgEvent *event = NULL;
    RichepgEpgBriefChannel *cinfo = NULL;
    RichepgEpgBriefEvent *einfo = NULL;
    char *tmp_str = NULL;
    int epg_sel = -1;

    if (ctrl->use_total == 0)
        return;
    event = _get_event(ctrl->cur_index, true);
    if (!event || event->start_time == 0)
    {
        return;
    }

    epg_sel = ctrl->use_array[ctrl->cur_index];
    if (epg_sel < 0)
        return;
    cinfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos];
    einfo = &cinfo->event_array[ctrl->epg_array[epg_sel].epg_pos];

    ctrl->detail_flag = 1;
    GUI_SetProperty(ctrl->wgt.details_back, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_duration, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_title, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_notepad, "state", "show");
    GUI_SetProperty(ctrl->wgt.details_exit, "state", "show");

    tmp_str = _get_duration_str(event->start_time, event->finish_time, true);
    GUI_SetProperty(ctrl->wgt.details_duration, "string", tmp_str);
    GUI_SetProperty(ctrl->wgt.details_title, "string", event->event_title);
    tmp_str = event->event_desc;
    if (!tmp_str || strlen(tmp_str) == 0)
    {
        if (0 == richepg_epg_event_info_get_by_time_from_usb(ctrl->use_event->start_time, cinfo->channel_id, einfo->event_id, event, EUTEL_EPG_EVENT_SIZE))
            tmp_str = event->event_desc;
    }
    if (!tmp_str || strlen(tmp_str) == 0)
        tmp_str = app_richepg_translate_str(STR_ID_NO_INFO);

    GUI_SetProperty(ctrl->wgt.details_notepad, "string", tmp_str);
}

static int app_eutel_epg2_details_keypress(uint16_t key, const char *widget)
{
    uint32_t value = 1;

    switch(key)
    {
        case STBK_UP:
            GUI_SetProperty(widget, "line_up", &value);
            break;
        case STBK_DOWN:
            GUI_SetProperty(widget, "line_down", &value);
            break;
        case STBK_PAGE_UP:
            GUI_SetProperty(widget, "page_up", &value);
            break;
        case STBK_PAGE_DOWN:
            GUI_SetProperty(widget, "page_down", &value);
            break;
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

int app_eutel_epg2_widget_init(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    ctrl->detail_flag = 0;
    ctrl->group_sel = 0;
    ctrl->cur_index = 0;

    ctrl->wgt.reverse_flag = app_richepg_gui_reverse();

    ctrl->wgt.wnd_name           = WND_EUTEL_EPG2;
    ctrl->wgt.group_item_common  = "btn_eutel_show_group%d";
    ctrl->wgt.day_item_common    = "btn_eutel_show_class%d";
    ctrl->wgt.item_back_common   = "img_eutel_show_back%d";
    ctrl->wgt.item_logo_common   = "img_eutel_show_logo%d";
    ctrl->wgt.item_title_common  = "text_eutel_show_title%d";
    ctrl->wgt.item_time_common   = "text_eutel_show_time%d";
    ctrl->wgt.item_ch_common     = "text_eutel_show_ch%d";

    ctrl->wgt.text_item_page     = "text_eutel_show_page";

    ctrl->wgt.text_item_duration = "text_eutel_show_duration";
    ctrl->wgt.text_item_name     = "text_eutel_show_name";
    ctrl->wgt.text_item_parent   = "text_eutel_show_parent";
    ctrl->wgt.text_item_genres   = "text_eutel_show_genres";
    ctrl->wgt.text_item_brief    = "text_eutel_show_brief";
    ctrl->wgt.text_ch_name       = "text_eutel_show_ch_name";
    ctrl->wgt.text_ch_logo       = "text_eutel_show_ch_logo";
    ctrl->wgt.img_ch_logo        = "img_eutel_show_ch_logo";

    ctrl->wgt.details_back       = "img_eutel_show_details_back";
    ctrl->wgt.details_duration   = "text_eutel_show_details_duration";
    ctrl->wgt.details_title      = "text_eutel_show_details_title";
    ctrl->wgt.details_notepad    = "notepad_eutel_show_details";
    ctrl->wgt.details_exit       = "btn_eutel_show_details_exit";

    ctrl->wgt.img_ok_tip         = "img_eutel_show_ok_tip";
    ctrl->wgt.txt_ok_tip         = "text_eutel_show_ok_tip";
    ctrl->wgt.txt_red_tip        = "text_eutel_show_red_tip";
    ctrl->wgt.txt_green_tip      = "text_eutel_show_green_tip";
    ctrl->wgt.img_book           = "img_eutel_show_fav_tip";
    ctrl->wgt.txt_book           = "text_eutel_show_fav_tip";
    ctrl->wgt.text_time          = "text_eutel_show_sys_time";
    ctrl->wgt.text_date          = "text_eutel_show_sys_date";

    return 0;
}

int app_eutel_epg2_real_create_dialog(bool first_in)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
#if EPG_VIEW_DAYS_SET_SUPPORT
    int days = app_richepg_epg_view_days_get();
#else
    int days = EUTEL_EPG_MAX_DAY;
#endif

    if (first_in)
    {
        ch_ctrl->filter_sel_bak = ch_ctrl->filter_sel;
        if (ch_ctrl->filter_sel_bak > 0)
            eutel_channel_group_all_build();
        if (ch_ctrl->use_total == 0)
        {
            EUTEL_INFO("No prog!\n");
            if (ch_ctrl->filter_sel_bak > 0)
                eutel_channel_group_build(ch_ctrl->filter_sel_bak);
            return -1;
        }
        app_eutel_epg2_widget_init();
    }

#define WAIT_POP 0
#if WAIT_POP
    if (days > 2)
    {
        PopDlg pop = {0};
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_WAITING;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 30;
        popdlg_create(&pop);
        GUI_SetInterface("flush", 0);
    }
#endif

    ctrl->day_total = days + EPG_OTHER_DAY_NUM;
    if (app_eutel_epg2_ctrl_init() < 0)
    {
#if WAIT_POP
        if (days > 2)
            popdlg_destroy();
#endif
        if (first_in)
        {
            if (ch_ctrl->filter_sel_bak > 0)
                eutel_channel_group_build(ch_ctrl->filter_sel_bak);

            if ( ctrl->epg_total <=0 )
                app_eutel_epg_maint_popup(STR_ID_EPG_EMPTY_TIP, false);
        }
        else
        {
            GUI_EndDialog(ctrl->wgt.wnd_name);
            GUI_SetInterface("flush", NULL);
            app_eutel_chinfo_create_dialog();
        }
        return -1;
    }
#if WAIT_POP
    if (days > 2)
        popdlg_destroy();
#endif

    if (first_in)
    {
#if TIMER_FLUSH_LOGO
        while (app_eutel_epg2_thread_run_check())
            GxCore_ThreadDelay(10);
#endif
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
        GUI_CreateDialog(ctrl->wgt.wnd_name);
    }
    else
    {
        app_eutel_epg2_update_all(-1, ctrl->group_sel, -1, ctrl->day_sel, ctrl->area_sel);
        app_eutel_epg2_change_ok_tip();
    }

    return 0;
}

int app_eutel_epg2_create_dialog(void)
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
        app_eutel_epg_maint_popup(STR_ID_EPG_EMPTY_TIP, false);
    }
    else
    {
        GxTime sys_time = {0};

        GxCore_GetLocalTime(&sys_time);
        if(sys_time.seconds > richepg_epg_last_finish_time_get())
        {
            app_eutel_epg_maint_popup(STR_ID_EPG_OUT_DATE_TIP, false);
        }
        else
        {
            return app_eutel_epg2_real_create_dialog(true);
        }
    }

    return -1;
}

static void app_eutel_epg2_exit(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    GUI_EndDialog(ctrl->wgt.wnd_name);
    GUI_SetInterface("flush", NULL);
    app_eutel_chinfo_create_dialog();
}

static int _eutel_epg2_play(pvr_state cur_pvr)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    EutelChannelArg *channel = NULL;
    int lcn = 0;

    if (ch_ctrl->use_total == 0)
        return -1;
    channel = _get_channel(ctrl->cur_index);
    if (!channel)
        return -1;
    lcn = channel->lcn_ids[0];
    ch_ctrl->filter_sel_bak = 0;

    if ((lcn != ch_ctrl->cur_tv_lcn && 0 == ch_ctrl->cur_stream_type)
            || (lcn != ch_ctrl->cur_ra_lcn && 1 == ch_ctrl->cur_stream_type)
            || g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK
            || g_AppFullArb.state.pause == STATE_ON)
    {
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        eutel_channel_play_by_lcn(PLAY_MODE_POINT, lcn);
        app_eutel_epg2_update_play_font_color();
        app_eutel_epg2_change_ok_tip();
    }

    return 0;
}

static int _eutel_epg2_stop_pvr_cb(PopDlgRet ret)
{
    pvr_state cur_pvr = app_pvr_get_state();

    if (POP_VAL_OK == ret)
    {
        if (PVR_TIMESHIFT == cur_pvr)
        {
            app_pvr_tms_stop();
        }
        else
        {
            app_pvr_stop();
        }
        _eutel_epg2_play(cur_pvr);
    }
    return 0;
}

static int app_eutel_epg2_play(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    EutelChannelArg *channel = NULL;
    GxMsgProperty_NodeByIdGet node_prog = {0};

    if (ch_ctrl->use_total == 0)
        return -1;
    channel = _get_channel(ctrl->cur_index);
    if (!channel)
        return -1;

    node_prog.node_type = NODE_PROG;
    node_prog.id = channel->prog_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_prog);

    if ((PVR_DUMMY != g_AppPvrOps.state)
            && (node_prog.prog_data.id == g_AppPvrOps.env.prog_id))
    {
        return -1;
    }
#if SAT2IP_SERVER_SUPPORT
    if (app_sat2ip_switch_tip_get_by_prog(&node_prog.prog_data) < 0)
        return -1;
#endif
    if (DEALWITH_KEEPON == app_pvr_popdlg_handler(_eutel_epg2_stop_pvr_cb))
    {
        _eutel_epg2_play(app_pvr_get_state());
    }

    return 0;
}

static GxBook thiz_book;
static int s_book_id = 0;
static int s_last_sel = 0;

#if CONTENT_TIMER_SUPPORT
static int _get_cur_index_by_book_id(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int i = 0;

    for (i = 0; i < ctrl->timer_index_total; i++)
    {
        if (s_book_id == ctrl->timer_data_array[ctrl->timer_index_array[i].timer_sel].book_id)
            return i;
    }
    return 0;
}

static int _cal_cur_index_by_last_sel(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    --s_last_sel;
    if (s_last_sel > 0 && s_last_sel < ctrl->use_total)
        return s_last_sel;
    return 0;
}
#endif

void app_eutel_epg2_event_timer_set(bool bflag, int book_id)
{
    static bool s_epg_event_timer = false;
    if(s_epg_event_timer == true) // app_timer_edit.c use this
    {
#if CONTENT_TIMER_SUPPORT
        EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
        if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
        {
            app_eutel_epg2_refresh_all(ctrl->group_sel, ctrl->group_sel, -1, 0, ctrl->area_sel, _get_cur_index_by_book_id);
        }
        else
#endif
        {
            _update_book_display();
        }
    }
    s_epg_event_timer = bflag;
    s_book_id = book_id;
}

static int _eutel_epg2_book_pop_cb(PopDlgRet ret)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    s_last_sel = ctrl->cur_index;

    if (POP_VAL_OK == ret)
    {
        g_AppBook.remove(&thiz_book);
    }
#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        app_eutel_epg2_refresh_all(-1, ctrl->group_sel, ctrl->day_sel, ctrl->day_sel, ctrl->area_sel, _cal_cur_index_by_last_sel);
    }
    else
#endif
    {
        _update_book_display();
    }

    return 0;
}

static void app_eutel_epg2_timer_edit_exit_callback(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    if (GUI_CheckDialog(ctrl->wgt.wnd_name) != GXCORE_SUCCESS)
        return ;
    _update_book_display();
    app_unset_window_back_ground(ctrl->wgt.wnd_name, true, false);
    eutel_channel_play_current(PLAY_MODE_POINT);
    g_AppFullArb.timer_start();
}

static int app_eutel_epg2_book_control(void)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    RichepgEpgBriefChannel *cinfo = NULL;
    RichepgEpgBriefEvent *einfo = NULL;
    EutelChannelArg *channel = NULL;
    RichepgEpgEvent *event = NULL;
    GxTime time = {0};
    char *prog_name = NULL;
    PopDlg pop = {0};
    int epg_sel = -1;

    if (ctrl->cur_index >= ctrl->use_total)
        return -1;

#if CONTENT_TIMER_SUPPORT
    if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
    {
        int timer_sel = ctrl->timer_index_array[ctrl->cur_index].timer_sel;
        memset(&thiz_book, 0, sizeof(GxBook));
        if (app_get_same_start_play_or_pvr_book(ctrl->timer_data_array[timer_sel].prog_id, ctrl->timer_data_array[timer_sel].start_time, &thiz_book))
        {
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.mode = POP_MODE_UNBLOCK;
            pop.str = STR_ID_SURE_DELETE;
            pop.exit_cb = _eutel_epg2_book_pop_cb;
            popdlg_create(&pop);
        }
        return 0;
    }
#endif
    epg_sel = ctrl->use_array[ctrl->cur_index];
    cinfo = &ctrl->data_mgr.channel_array[ctrl->epg_array[epg_sel].ch_pos];
    einfo = &cinfo->event_array[ctrl->epg_array[epg_sel].epg_pos];
    channel = _get_channel(ctrl->cur_index);
    event = _get_event(ctrl->cur_index, false);
    if (!channel || !event)
        return -1;
    GxCore_GetLocalTime(&time);
    if ((einfo->event_flag & INVALID_EPG_FLAG)
            || (einfo->start_time < time.seconds))
        return -1;

    memset(&thiz_book, 0, sizeof(GxBook));
    if (app_get_same_start_play_or_pvr_book(channel->prog_id, einfo->start_time, &thiz_book)
            && (einfo->start_time >= time.seconds))
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = STR_ID_SURE_DELETE;
        pop.exit_cb = _eutel_epg2_book_pop_cb;
        popdlg_create(&pop);
    }
    else  // add book
    {
        GxBookGet book_Get = {0};
        if (g_AppBook.get(&book_Get, BOOKMODE_ALL) >= APP_BOOK_NUM)
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.str = STR_ID_TIMER_FULL;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 3;
            popdlg_create(&pop);

            return -1;
        }

        g_AppPlayOps.program_stop();
        prog_name = channel->channel_name;
        if (!prog_name || strlen(prog_name) == 0)
            prog_name = STR_ID_BLANK;

        app_timer_edit_ex_func_register(app_eutel_epg2_timer_edit_exit_callback);
        app_epg_timer_menu_exec(einfo->start_time, einfo->finish_time - einfo->start_time, channel->prog_id, prog_name);

/*
        if (GUI_CheckDialog(ctrl->wgt.wnd_name) != GXCORE_SUCCESS)
            return -1;
        _update_book_display();
        app_unset_window_back_ground(ctrl->wgt.wnd_name, true, false);
        eutel_channel_play_current(PLAY_MODE_POINT);
        g_AppFullArb.timer_start();
*/
    }

    return 0;
}

static int _eutel_epg2_num_reponse(int num_value)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    int orig_sel = 0, sel = 0, total = 0;

    if (ctrl->use_total == 0)
        return -1;

    orig_sel = ctrl->cur_index / EPG_ITEMS_PER_PAGE;
    sel = num_value - 1;
    total = (ctrl->use_total - 1) / EPG_ITEMS_PER_PAGE + 1;

    if ((sel < total && sel >= 0) && sel != orig_sel)
    {
        if (ctrl->detail_flag)
            app_eutel_epg2_details_hide();
        app_eutel_epg2_update_item_all(ctrl->cur_index, sel * EPG_ITEMS_PER_PAGE, true);
    }

    return 0;
}

static int _eutel_epg2_create(GuiWidget *widget, void *usrdata)
{
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;
    char wgt[ITEM_COMMON_SIZE] = {0};
    int32_t init_value = 0;

#ifdef EPG2_REFRESH_BY_TIMER_SUPPROT
    app_show_sys_time_and_date(ctrl->wgt.text_time, ctrl->wgt.text_date);
#endif

    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
#if TIMER_FLUSH_LOGO
    app_eutel_epg2_thread_task_start();
#endif
    GUI_SetProperty(ctrl->wgt.txt_red_tip, "string", STR_ID_PAGE_SUB);
    GUI_SetProperty(ctrl->wgt.txt_green_tip, "string", STR_ID_PAGE_PLUS);

#if FOCUS_LAST_ITEM_SUPPORT
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    if (ctrl->index_bak.bak_flag && ctrl->group_sel < ch_ctrl->content_total + CONTENT_RESERVED_NUM)
    {
        if ( ctrl->index_bak.group_sel == CONTENT_TIMER_GROUP_SEL)
        {
            app_set_widget_string(ctrl->wgt.txt_red_tip, STR_ID_ADD);
            app_set_widget_string(ctrl->wgt.txt_green_tip, STR_ID_EDIT);
        }
        app_eutel_epg2_refresh_all(-1, ctrl->index_bak.group_sel, -1, ctrl->index_bak.day_sel, AREA_ITEM, _get_cur_index_by_bak_index);
        if (ctrl->use_total > 0)
        {
            snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, 1 + ctrl->cur_index % EPG_ITEMS_PER_PAGE);
            GUI_SetProperty(wgt, "img", ITEM_FOCUS_IMG);
        }
    }
    else
#endif
    {
#if FOCUS_GROUP_DEFAULT
        app_eutel_epg2_update_all(-1, 0, -1, 0, AREA_GROUP);
        snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.group_item_common, 1);
        GUI_SetProperty(wgt, "focus_img", GROUP_SELECT_IMG);
#else
        app_eutel_epg2_update_all(-1, 0, -1, 0, AREA_ITEM);
        int focus_sel = (ctrl->wgt.reverse_flag) ? ctrl->rfocus_sel : ctrl->focus_sel;
        if (focus_sel == 0 && ctrl->use_total > 0)
        {
            snprintf(wgt, ITEM_COMMON_SIZE, ctrl->wgt.item_back_common, 1);
            GUI_SetProperty(wgt, "img", ITEM_FOCUS_IMG);
        }
#endif
    }

    app_eutel_epg2_change_ok_tip();
    app_number_response_cb_set(_eutel_epg2_num_reponse);

    return EVENT_TRANSFER_STOP;
}

static int _eutel_epg2_destroy(GuiWidget *widget, void *usrdata)
{
    int32_t init_value = 0;

    app_number_response_cb_set(NULL);
#if FOCUS_LAST_ITEM_SUPPORT
    app_eutel_epg2_bak_timer_start();
#endif
    app_eutel_epg2_ctrl_release();
    pmpset_exit();
    GxBus_ConfigGetInt(FREEZE_SWITCH, &init_value, FREEZE_SWITCH_VALUE);
    app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &init_value);
#if TIMER_FLUSH_LOGO
    app_eutel_epg2_thread_stop_set();
#endif
    app_richepg_chfilter_keep_sec_check_set();
    return EVENT_TRANSFER_STOP;
}

static int _eutel_epg2_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    EutelEpg2Ctrl *ctrl = &s_eutel_epg2_ctrl;

    event = (GUI_Event *)usrdata;
    if (GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                GUI_EndDialog(ctrl->wgt.wnd_name);
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                else
                    app_eutel_epg2_exit();
                break;
            case STBK_EPG:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                GUI_EndDialog(ctrl->wgt.wnd_name);
                app_eutel_epg_create_dialog();
                break;
            case STBK_OK:
                if (ctrl->area_sel == AREA_ITEM)
                {
                    EutelChannelArg *channel = _get_channel(ctrl->cur_index);
                    int channel_id = eutel_cur_channel_id_get();
                    if (channel)
                    {
                        if (ctrl->detail_flag)
                            app_eutel_epg2_details_hide();
                        if (_check_play_flag(channel_id, channel->channel_id))
                            app_eutel_epg2_play();
                        else
                            app_eutel_epg2_exit();
                    }
                }
#if CONTENT_TIMER_SUPPORT
                if (ctrl->area_sel == AREA_GROUP)
                {
                    if (ctrl->detail_flag)
                        app_eutel_epg2_details_hide();
                    if (ctrl->group_sel != CONTENT_TIMER_GROUP_SEL)
                        app_eutel_epg2_update_all(ctrl->group_sel, CONTENT_TIMER_GROUP_SEL, ctrl->day_sel, ctrl->day_sel, ctrl->area_sel);
                    else
                        app_eutel_epg2_update_all(ctrl->group_sel, ctrl->group_return, ctrl->day_sel, ctrl->day_sel, ctrl->area_sel);
                    app_eutel_epg2_change_ok_tip();
                }
#endif
#if MORE_GROUP_SUPPORT
                else if (ctrl->area_sel == AREA_MORE)
                {
                    if (ctrl->detail_flag)
                        app_eutel_epg2_details_hide();
                    app_eutel_epg2_content_filter_pop(AREA_GROUP);
                    app_eutel_epg2_change_ok_tip();
                }
#endif
                break;

            case STBK_RED:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
#if CONTENT_TIMER_SUPPORT
                if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
                {
                    GxBookGet book_Get = {0};
                    if (g_AppBook.get(&book_Get, BOOKMODE_ALL) < APP_BOOK_NUM)
                    {
                        extern bool app_add_timer_menu_exec(void);
                        g_AppPlayOps.program_stop();
                        app_add_timer_menu_exec();
                        if (GUI_CheckDialog(ctrl->wgt.wnd_name) != GXCORE_SUCCESS)
                            return -1;
                        app_unset_window_back_ground(ctrl->wgt.wnd_name, true, false);
                        eutel_channel_play_current(PLAY_MODE_POINT);
                        g_AppFullArb.timer_start();
                    }
                    else
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.str = STR_ID_TIMER_FULL;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.format = POP_FORMAT_DLG;
                        popdlg_create(&pop) ;
                    }
                    break;
                }
#endif
                app_eutel_epg2_change_item_keypress(STBK_PAGE_UP);
                app_eutel_epg2_change_ok_tip();
                break;

            case STBK_GREEN:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
#if CONTENT_TIMER_SUPPORT
                if (ctrl->group_sel == CONTENT_TIMER_GROUP_SEL)
                {
                    if (ctrl->cur_index >= ctrl->use_total)
                        break;
                    int timer_sel = ctrl->timer_index_array[ctrl->cur_index].timer_sel;
                    memset(&thiz_book, 0, sizeof(GxBook));
                    if (app_get_same_start_play_or_pvr_book(ctrl->timer_data_array[timer_sel].prog_id, ctrl->timer_data_array[timer_sel].start_time, &thiz_book))
                    {
                        extern bool app_edit_timer_menu_exec(GxBook *book);
                        g_AppPlayOps.program_stop();
                        app_edit_timer_menu_exec(&thiz_book);
                        if (GUI_CheckDialog(ctrl->wgt.wnd_name) != GXCORE_SUCCESS)
                            return -1;
                        app_unset_window_back_ground(ctrl->wgt.wnd_name, true, false);
                        eutel_channel_play_current(PLAY_MODE_POINT);
                        g_AppFullArb.timer_start();
                    }
                    break;
                }
#endif
                app_eutel_epg2_change_item_keypress(STBK_PAGE_DOWN);
                app_eutel_epg2_change_ok_tip();
                break;

            case STBK_YELLOW:
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                if (ctrl->area_sel == AREA_ITEM)
                    app_eutel_epg2_content_filter_pop(AREA_ITEM);
                else
                    app_eutel_epg2_content_filter_pop(AREA_GROUP);
                app_eutel_epg2_change_ok_tip();
                break;
            case STBK_BLUE:
                if (ctrl->cur_index < EPG_ITEMS_PER_PAGE)
                    break;
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                app_eutel_epg2_real_create_dialog(false);
                break;

            case STBK_FAV:
#ifndef RADIO_REC_BOOK
                if (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
                    break;
#endif
                if (ctrl->detail_flag)
                    app_eutel_epg2_details_hide();
                app_eutel_epg2_book_control();
                break;
            case STBK_INFO:
                if (!ctrl->detail_flag)
                    app_eutel_epg2_details_show();
                break;
            case STBK_UP:
            case STBK_DOWN:
            case STBK_LEFT:
            case STBK_RIGHT:
            case STBK_PAGE_UP:
            case STBK_PAGE_DOWN:
                if (!ctrl->detail_flag)
                    app_eutel_epg2_change_keypress(event->key.sym);
                else
                    app_eutel_epg2_details_keypress(event->key.sym, ctrl->wgt.details_notepad);
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
    }
    return EVENT_TRANSFER_STOP;
}

static int _eutel_epg2_got_focus(GuiWidget *widget, void *usrdata)
{
    app_eutel_epg2_event_timer_set(false, 0);
    app_eutel_epg2_change_ok_tip();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_show_create(GuiWidget *widget, void *usrdata)
{
    return _eutel_epg2_create(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_show_destroy(GuiWidget *widget, void *usrdata)
{
    return _eutel_epg2_destroy(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_show_got_focus(GuiWidget *widget, void *usrdata)
{
    return _eutel_epg2_got_focus(widget, usrdata);
}

SIGNAL_HANDLER int app_eutel_show_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_eutel_show_keypress(GuiWidget *widget, void *usrdata)
{
    return _eutel_epg2_keypress(widget, usrdata);
}

#endif

