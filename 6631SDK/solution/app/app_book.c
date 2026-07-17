#include "app_book.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "module/channel_edit.h"
#include "app_windows.h"
#include "app_channel_info.h"
#include "app_wnd_system_setting_opt.h"
#include "app_netaudio.h"

#define SEC_PER_DAY (60 * 60 * 24)
static GxBookType s_sleep_type = BOOK_PROGRAM_PLAY;
static bool s_book_enable = false;
static event_list* s_book_ignore_timer = NULL;
static event_list* s_book_pop_timer = NULL;
static event_list* sp_PowerOffChTimer = NULL;
static const char *s_focus_wnd = NULL;

static BookMode g_booklist_mode = BOOKMODE_ALL;
extern int app_timer_setting_update_timer_list(void);
extern void app_set_avcodec_flag(bool state);
extern void app_low_power(time_t sleep_time, uint32_t scan_code);
extern bool app_check_av_running(void);
extern bool app_ota_idle(void);
extern void music_view_mode_pause(void);
extern void app_upgrade_clear_path(void);
extern bool app_program_need_predemux(GxMsgProperty_NodeByPosGet *node);
extern bool app_check_prog_visible(uint32_t prog_id);
#if BLUETOOTH_SUPPORT
extern void sink_view_mode_pause(void);
#endif
#if NETWORK_SUPPORT && WIFI_SUPPORT
extern void app_wifi_tip_flush(void);
extern void app_wifi_tip_lost_focus_hide(void);
#endif
extern void file_brower_book_cancel(void);
static int32_t app_get_book(GxBookGet *pbookGet, BookMode BookListType);
extern void app_multi_lang_keyborad_menu_destroy(void);

int app_booklist_type_set(BookMode type)
{
    if ((BOOKMODE_ALL <= type ) && (BOOKMODE_POWER >= type ))
    {
        g_booklist_mode = type;
    }
    else
    {
        g_booklist_mode = BOOKMODE_ALL;
    }
    return 0;
}

int app_booklist_type_get(BookMode *type)
{
    *type = g_booklist_mode;
    return 0;
}

static int app_book_sleep_type_set(GxBookType type)
{
    s_sleep_type = type;
    return 0;
}

GxBookType app_book_sleep_type_get(void)
{
    return s_sleep_type;
}

static WeekDay app_tm_get_wday(time_t time)
{
    struct tm ptm = {0};
    localtime_r(&time, &ptm);
    return ptm.tm_wday;
}

static WeekDay app_book_mode2wday(uint32_t repeat_mode)
{
    WeekDay wday = WEEK_SUN;
    switch(repeat_mode)
    {
        case BOOK_REPEAT_MON:
            wday =  WEEK_MON;
            break;

        case BOOK_REPEAT_TUES:
            wday =  WEEK_TUES;
            break;

        case BOOK_REPEAT_WED:
            wday =  WEEK_WED;
            break;

        case BOOK_REPEAT_THURS:
            wday =  WEEK_THURS;
            break;

        case BOOK_REPEAT_FRI:
            wday =  WEEK_FRI;
            break;

        case BOOK_REPEAT_SAT:
            wday =  WEEK_SAT;
            break;

        case BOOK_REPEAT_SUN:
            wday =  WEEK_SUN;
            break;

        default:
            break;
    }

    return wday;
}

static uint32_t app_book_wday2mode(WeekDay wday)
{
    uint32_t mode = BOOK_REPEAT_ONCE;

    switch(wday)
    {
        case WEEK_MON:
            mode = BOOK_REPEAT_MON;
            break;

        case WEEK_TUES:
            mode = BOOK_REPEAT_TUES;
            break;

        case WEEK_WED:
            mode = BOOK_REPEAT_WED;
            break;

        case WEEK_THURS:
            mode = BOOK_REPEAT_THURS;
            break;

        case WEEK_FRI:
            mode = BOOK_REPEAT_FRI;
            break;

        case WEEK_SAT:
            mode = BOOK_REPEAT_SAT;
            break;

        case WEEK_SUN:
            mode = BOOK_REPEAT_SUN;
            break;

        default:
            break;
    }
    return mode;
}

static time_t app_get_book_time(time_t cur_time, int32_t type)
{
    GxBookGet bookGet;
    int count = 0;
    int i;
    time_t sleep_time = 0;
    time_t time_interval;
    time_t time_off;
    time_t time_on;

    count = app_get_book(&bookGet, BOOKMODE_ALL);
    time_interval = sleep_time;
    for(i = 0; i < count; i++)
    {
        if(bookGet.book[i].book_type & type)
        {
            if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
            {
                if(cur_time < bookGet.book[i].trigger_time_start)
                {
                    time_interval = bookGet.book[i].trigger_time_start - cur_time;
                }
            }
            else if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
            {

                time_off = cur_time % SEC_PER_DAY;
                time_on = bookGet.book[i].trigger_time_start % SEC_PER_DAY;

                if(time_on < time_off)
                    time_on += SEC_PER_DAY;

                time_interval = time_on - time_off;
            }
            else
            {
                WeekDay wday1;
                WeekDay wday2;
                time_t time_temp1;
                time_t time_temp2;
                uint8_t day_count;

                time_temp1 = cur_time % SEC_PER_DAY;
                wday1 = app_tm_get_wday(cur_time);

                time_temp2 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;
                wday2 = app_book_mode2wday(bookGet.book[i].repeat_mode.mode);

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

                time_on = (cur_time - time_temp1) + (day_count * SEC_PER_DAY) + time_temp2;
                time_interval = time_on - cur_time;
            }

            if((sleep_time == 0) || (time_interval < sleep_time))
            {
                sleep_time = time_interval > bookGet.book[i].trigger_time_advance ? time_interval - bookGet.book[i].trigger_time_advance : 1;
                app_book_sleep_type_set(bookGet.book[i].book_type);
            }
        }
    }

    return sleep_time;
}

static time_t app_get_sleep_time(time_t cur_time)
{
    return app_get_book_time(cur_time, BOOK_POWER_ON | BOOK_PROGRAM_PLAY | BOOK_PROGRAM_PVR);
}

static int app_book_get_week(time_t sec)
{
    int ret = -1;

    struct tm *temp_tm = NULL;
    temp_tm = localtime(&sec);

    switch(temp_tm->tm_wday)
    {
        case 0: ret = BOOK_REPEAT_SUN  ; break;
        case 1: ret = BOOK_REPEAT_MON  ; break;
        case 2: ret = BOOK_REPEAT_TUES ; break;
        case 3: ret = BOOK_REPEAT_WED  ; break;
        case 4: ret = BOOK_REPEAT_THURS; break;
        case 5: ret = BOOK_REPEAT_FRI  ; break;
        case 6: ret = BOOK_REPEAT_SAT  ; break;
        default:                       ; break;
    }

    return ret;
}

static bool app_book_pvr_time_valid(time_t cycle_time,
        time_t start_time1, time_t end_time1,
        time_t start_time2, time_t end_time2)
{
    bool ret = false;
    if(end_time1 <= cycle_time)
    {
        if((start_time1 >= end_time2)
                || ((start_time1+cycle_time >= end_time2) && (end_time1 <= start_time2)))
        {
            ret = true;
        }
    }
    else
    {
        if((start_time1 >= end_time2) && (end_time1-cycle_time <= start_time2))
        {
            ret = true;
        }
    }
    return ret;
}

bool app_get_same_start_play_or_pvr_book(uint16_t prog_id, time_t start_time, GxBook *book_ret)
{
	GxBookGet bookGet;
	BookProgStruct book_prog;
	int count = 0;
	int i;

	g_AppBook.invalid_remove();
	if((prog_id == 0) || (book_ret == NULL))
		return false;

	count = g_AppBook.get(&bookGet, BOOKMODE_PLAY);
	if(count == 0)
		return false;

	for(i = 0; i < count; i++)
	{
        memcpy(&book_prog, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));
        if(prog_id == book_prog.prog_id)
        {
            time_t day_time1 =start_time % SEC_PER_DAY;
            time_t day_time2 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;

            time_t trig_time1 = start_time;
            time_t trig_time2 = bookGet.book[i].trigger_time_start;

            if(day_time1 == day_time2)
            {
                if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
                {
                    if(trig_time1 == trig_time2)
                        goto end;
                }
                else if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
                {
                    goto end;
                }
                else if(bookGet.book[i].repeat_mode.mode == app_book_get_week(trig_time1))
                {
                    goto end;
                }
            }
        }
	}

	return false;
end:
	memcpy(book_ret, bookGet.book + i, sizeof(GxBook));
	return true;
}
static bool app_book_time_valid(GxBook *book) //check new book time
{
    GxBookGet bookGet;
    int count = 0;
    int i;
    bool ret = true;

    if(book == NULL)
        return false;

    count = app_get_book(&bookGet, BOOKMODE_ALL);
    if(count == 0)
        return true;

    for(i = 0; i < count; i++)
    {
        if(bookGet.book[i].id == book->id) //same timer
        {
            continue;
        }

        time_t day_time1 = book->trigger_time_start % SEC_PER_DAY;
        time_t day_time2 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;

        time_t trig_time1 = book->trigger_time_start;
        time_t trig_time2 = bookGet.book[i].trigger_time_start;

        if(day_time1 == day_time2)
        {
            if((bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE) && (book->repeat_mode.mode == BOOK_REPEAT_ONCE))
            {
                if(trig_time1 == trig_time2)
                {
                    ret = false;
                    break;
                }
            }
            else
            {
                if ((book->repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
                        || (bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
                        || (book->repeat_mode.mode == bookGet.book[i].repeat_mode.mode)
                        || ((book->repeat_mode.mode == BOOK_REPEAT_ONCE) && (app_book_get_week(trig_time1) == bookGet.book[i].repeat_mode.mode))
                        || ((bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE) && (app_book_get_week(trig_time2) == book->repeat_mode.mode))
                   )
                {
                    ret = false;
                    break;
                }
            }
        }
        else
        {
            if((book->book_type == BOOK_PROGRAM_PVR) && (bookGet.book[i].book_type == BOOK_PROGRAM_PVR))
            {
                BookProgStruct book_data1 = {0};
                BookProgStruct book_data2 = {0};
                memcpy(&book_data1, (BookProgStruct*)book->struct_buf, sizeof(BookProgStruct));
                memcpy(&book_data2, (BookProgStruct*)bookGet.book[i].struct_buf, sizeof(BookProgStruct));

                /**
                 *目标：比较两个PVR事件的区间，重合时 ret = false
                 *分类：两个事件都无周期 once，有一个事件的周期为天 repeat，有一个事件的周期为星期 weekn
                 *区间：将周期事件的起始时间、结束时间 将两个周期划分为6个区间，分割线为 A0 A1 A2 B0 B1 B2 C0
                 *事件的起始时间必然处于(A0, B0)区间
                 *不跨周期事件，(A1, A2) (B1, B2)区间:
                 *与不跨周期事件对比的事件只有在(A0, A1) (A2, B0) (A2, B1)区间才会不重合，ret = true
                 *跨周期事件，(A2, B1)区间:
                 *与跨周期事件对比的事件只有在(A1, A2)区间才会不重合，ret = true
                 */
                if((book->repeat_mode.mode == BOOK_REPEAT_ONCE) && (bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE))
                {
                    time_t start_time1 = book->trigger_time_start;
                    time_t end_time1 = start_time1 + book_data1.duration;
                    time_t start_time2 = bookGet.book[i].trigger_time_start;
                    time_t end_time2 = start_time2 + book_data2.duration;

                    if( !(
                                ((end_time1<=start_time2) || (start_time1>=end_time2))
                         ))
                    {
                        ret = false;
                        break;
                    }
                }
                else if((book->repeat_mode.mode == BOOK_REPEAT_EVERY_DAY) || (bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY))
                {
                    time_t start_time1 = book->trigger_time_start % SEC_PER_DAY;
                    time_t end_time1 = start_time1 + book_data1.duration;
                    time_t start_time2 = bookGet.book[i].trigger_time_start % SEC_PER_DAY;
                    time_t end_time2 = start_time2 + book_data2.duration;

                    if(book->repeat_mode.mode == BOOK_REPEAT_EVERY_DAY)
                    {
                        ret = app_book_pvr_time_valid(SEC_PER_DAY, start_time1, end_time1, start_time2, end_time2);
                        if (!ret) break;
                    }
                    else
                    {
                        ret = app_book_pvr_time_valid(SEC_PER_DAY, start_time2, end_time2, start_time1, end_time1);
                        if (!ret) break;
                    }
                }
                else
                {
                    time_t start_time1;
                    if(book->repeat_mode.mode != BOOK_REPEAT_ONCE)
                    {
                        start_time1 = (book->trigger_time_start % SEC_PER_DAY)
                            + (app_book_mode2wday(book->repeat_mode.mode) * SEC_PER_DAY);
                    }
                    else
                    {
                        start_time1 = (book->trigger_time_start % SEC_PER_DAY)
                            + (app_tm_get_wday(book->trigger_time_start) * SEC_PER_DAY);
                    }
                    time_t end_time1 = start_time1 + book_data1.duration;

                    time_t start_time2;
                    if(bookGet.book[i].repeat_mode.mode != BOOK_REPEAT_ONCE)
                    {
                        start_time2 = (bookGet.book[i].trigger_time_start % SEC_PER_DAY)
                            + (app_book_mode2wday(bookGet.book[i].repeat_mode.mode) * SEC_PER_DAY);
                    }
                    else
                    {
                        start_time2 = (bookGet.book[i].trigger_time_start % SEC_PER_DAY)
                            + (app_tm_get_wday(bookGet.book[i].trigger_time_start) * SEC_PER_DAY);
                    }
                    time_t end_time2 = start_time2 + book_data2.duration;

                    if(book->repeat_mode.mode != BOOK_REPEAT_ONCE)
                    {
                        ret = app_book_pvr_time_valid(SEC_PER_DAY*7, start_time1, end_time1, start_time2, end_time2);
                        if (!ret) break;
                    }
                    else
                    {
                        ret = app_book_pvr_time_valid(SEC_PER_DAY*7, start_time2, end_time2, start_time1, end_time1);
                        if (!ret) break;
                    }
                }
            }
        }
    }

    return ret;
}

static _BookExecWnd app_exec_prog_book_wnd(void)
{
    s_focus_wnd = NULL;

    if((GUI_CheckDialog(WND_SEARCH) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WND_OTA_UPGRADE) == GXCORE_SUCCESS))
        return EXEC_WND_NONE;

    if(GUI_CheckDialog(WND_UPGRADE_PROCESS) == GXCORE_SUCCESS)
    {
        if(app_get_update_state() == UPGRADE_START)
        {
            return EXEC_WND_NONE;
        }
        else
        {
            GUI_EndDialog(WND_UPGRADE_PROCESS);
        }
    }

    if((GUI_CheckDialog(WIN_MEDIA_CENTRE) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WND_NETVIDEO_PLAY) == GXCORE_SUCCESS)|| (GUI_CheckDialog(WND_FM_PLAY) == GXCORE_SUCCESS))
    {
        return EXEC_WND_MENU;
    }

    if(GUI_CheckDialog(WND_POP_SORT) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_POP_SORT);
        GUI_SetInterface("flush", NULL);
    }
    if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_PASSWD);
        GUI_SetInterface("flush", NULL);
    }
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
    {
        app_multi_lang_keyborad_menu_destroy();
    }
#if NETWORK_SUPPORT && WIFI_SUPPORT
    if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
    {
        app_wifi_tip_lost_focus_hide();
    }
#endif
    s_focus_wnd = GUI_GetFocusWindow();
    if(s_focus_wnd == NULL)
        return EXEC_WND_NONE;
#if (OTA_SUPPORT > 0)
    if(strcmp(s_focus_wnd, WND_OTA_UPGRADE) == 0)
    {
        if(app_ota_idle() == true)
        {
            return  EXEC_WND_MENU;
        }
        else
        {
            s_focus_wnd = NULL;
            return EXEC_WND_NONE;
        }
    }
#endif
    if((strcmp(s_focus_wnd, WND_FULL_SCREEN) == 0)
            || (app_channel_info_compare_dialog(s_focus_wnd) == 0)
            || (app_channel_info_signal_compare_dialog(s_focus_wnd) == 0)
            || (app_channel_list_compare_dialog(s_focus_wnd) == 0)
#if BOUQUET_SUPPORT
            || (strcmp(s_focus_wnd, WND_BOUQUET) == 0)
#endif
            || (strcmp(s_focus_wnd, WND_NUMBER) == 0)
#ifdef MUITI_SCREEN_SUPPORE
            || (strcmp(s_focus_wnd, "wnd_multiscreen") == 0)
#endif
#if QR_POP_SUPPORT
            || (strcmp(s_focus_wnd, WND_POP_QR) == 0)
#endif
            || (strcmp(s_focus_wnd, WND_NETAUDIO) == 0)
            || (strcmp(s_focus_wnd, WND_ZOOM) == 0)
            || (strcmp(s_focus_wnd, WND_COMMON_SELECT) == 0
                && GXCORE_ERROR == GUI_CheckDialog(WND_COMMON_VIEW))
            || (strcmp(s_focus_wnd, WND_PVR_BAR) == 0))
    {
        return  EXEC_WND_FULLSCREEN;
    }
    else if(strcmp(s_focus_wnd, WND_DURATION_SET) == 0)
    {
        GUI_EndDialog(WND_DURATION_SET);
        GUI_EndDialog(WND_PVR_BAR);
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            app_channel_info_create_dialog();
        return  EXEC_WND_FULLSCREEN;
    }
    else if((GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
            || (app_channel_epg_compare_dialog(s_focus_wnd) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WIN_MEDIA_CENTRE) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WND_TIMER_SETTING) == GXCORE_SUCCESS)
            || (GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS)
            || (strcmp(s_focus_wnd, WND_SYSTEM_SETTING) == 0)
            || (strcmp(s_focus_wnd, WND_VOLUME) == 0)
            || (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS))
    {
        return  EXEC_WND_MENU;
    }
    else
    {
        s_focus_wnd = NULL;
    }

    return EXEC_WND_NONE;
}

static status_t app_prog_book_wnd_proc(void)
{
    status_t ret = GXCORE_ERROR;
    static GUI_Event event;
    s_focus_wnd = GUI_GetFocusWindow();
    if(s_focus_wnd != NULL)
    {
        event.type = GUI_KEYDOWN;
        event.key.sym = VK_BOOK_TRIGGER;
        GUI_SendEvent(s_focus_wnd, &event);
        ret = GXCORE_SUCCESS;
    }
    else
    {
        if((GUI_CheckDialog(WIN_MEDIA_CENTRE) == GXCORE_SUCCESS)
                || (GUI_CheckDialog(WIN_MOVIE_VIEW) == GXCORE_SUCCESS))
        {
            GxMsgProperty_PlayerStop player_stop;

            //don't stop play , when second play av in media,can keep the end time
            //just enddialog after wnd_full_screen
            //player_stop.player = PMP_PLAYER_AV;
            //ret = app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

            player_stop.player = PMP_PLAYER_PIC;
            ret = app_send_msg_exec(GXMSG_PLAYER_STOP, &player_stop);

            GUI_SetInterface("clear_image", NULL);
            GUI_SetInterface("flush", NULL);
            GUI_EndDialog("after wnd_full_screen");
            GUI_SetInterface("video_enable", NULL);
        }
    }

    return ret;
}

static int _no_program_pop_timer(void* userdata)
{
    if(GUI_CheckDialog(WND_POP_TIP) != GXCORE_SUCCESS)
    {
        APP_TIMER_REMOVE(s_book_pop_timer);
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_NO_PROGRAM;
        pop.title = STR_ID_BOOK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.timeout_sec = 3;
        popdlg_create(&pop);
    }
    return 0;
}

static void _book_play_pvr_cb_cancel(void)
{
    s_focus_wnd = GUI_GetFocusWindow();
    if(s_focus_wnd == NULL)
        return;
#if GAME_ENABLE
    else if(strcmp(s_focus_wnd, WND_TETRIS) == 0)
    {
        extern int app_tetris_get_focus(void);
        app_tetris_get_focus();
    }
    else if(strcmp(s_focus_wnd, WND_SNAKE) == 0)
    {
        extern int app_snake_get_focus(void);
        app_snake_get_focus();
    }
#endif
#if NETWORK_SUPPORT && WIFI_SUPPORT
    else if(strcmp(s_focus_wnd, WND_WIFI) == 0)
    {
        app_wifi_tip_flush();
    }
#endif
    else if(strcmp(s_focus_wnd, WIN_FILE_BROWSER) == 0)
    {
        usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
        if(usb_state == USB_ERROR)
        {
	 	file_brower_book_cancel();
        }
        GUI_SetProperty(s_focus_wnd,"update",NULL);
    }
    else
    {
        GUI_SetProperty(s_focus_wnd,"update",NULL);
    }

    if(GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS)
    {
        if(g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_ERROR)
        {
            if(app_system_get_title() != NULL && 0 == strcmp(app_system_get_title(), STR_ID_FIRMWARE_UPGRADE))
            {
                app_upgrade_clear_path();
            }
        }
    }
}

static int _book_exec_pvr_play_cb(BookDlgRet *book_ret)
{
    uint32_t mode = 0;
    int32_t book_prog_pos = -1;
    BookProgStruct book_data = {0};
    uint32_t pause_bak = g_AppFullArb.state.pause;
    GxBusPmDataProgType book_prog_stream_type = 0;
    GxMsgProperty_NodeByIdGet book_prog_node = {0};

    if(NULL == book_ret)
    {
        return -1;
    }

    memcpy(&book_data, (BookProgStruct*)book_ret->cache.book.struct_buf, sizeof(BookProgStruct));
    book_prog_node.node_type = NODE_PROG;
    book_prog_node.id = book_data.prog_id;
    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&book_prog_node)))
    {
        //this situation should not exist, because if the book prog doesn't exist, the book pop wiil not be created
        return -1;
    }
    APP_PRINT("[%s] %s Book Program Id:%d Sat:%d Tp:%d St:%d Sc:%d Sk:%d Name:%s\n",
            __FUNCTION__,
            book_ret->ret == POP_VAL_OK?"Exec":"Cancel",
            book_prog_node.prog_data.id,
            book_prog_node.prog_data.sat_id,
            book_prog_node.prog_data.tp_id,
            book_prog_node.prog_data.service_type,
            book_prog_node.prog_data.scramble_flag,
            book_prog_node.prog_data.skip_flag,
            book_prog_node.prog_data.prog_name);
    book_prog_stream_type = book_prog_node.prog_data.service_type;

    if(POP_VAL_OK == book_ret->ret)
    {
        if(BOOK_PROGRAM_PVR == book_ret->cache.book.book_type)
        {
            usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);
            if (usb_state != USB_OK || (app_netaudio_check_flag() == 1))
            {
                PopDlg pop;
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.create_cb = NULL;
                pop.exit_cb = NULL;
                pop.timeout_sec = 3;
                pop.title = STR_ID_BOOK;
                if(app_netaudio_check_flag() == 1)
                {
                    pop.str = STR_ID_CANT_PVR;
                }
                else
                {
                    if(usb_state == USB_ERROR) {
                        pop.str = STR_ID_INSERT_USB;
                    } else if(usb_state == USB_NO_SPACE) {
                        pop.str = STR_ID_NO_SPACE;
                    }
                    else if(usb_state == USB_READ_ONLY) {
                        pop.str = STR_ID_NOWRITE_PERMISSIONS;
                    }
                }
                popdlg_create(&pop);

                return GXCORE_ERROR;
            }
        }
        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_PASSWD);
        }
        if(book_ret->cache.book_wnd == EXEC_WND_MENU)
        {
            app_prog_book_wnd_proc();
        }
        if(g_AppPvrOps.state != PVR_DUMMY)
        {
            app_pvr_stop();
            app_set_avcodec_flag(false);
        }
        if (app_get_prog_pos_by_id_all(book_prog_node.id, &book_prog_pos) < 0)
        {
            if ( !app_fuse_spec_switch_work_env(GROUP_MODE_SAT,book_prog_node.prog_data.sat_id))
                return GXCORE_ERROR;
            if (app_get_prog_pos_by_id_all(book_prog_node.id, &book_prog_pos) < 0)
                return GXCORE_ERROR;
        }

        if(BOOK_PROGRAM_PVR == book_ret->cache.book.book_type)
        {
            if(book_prog_node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
                    && !app_program_need_predemux(&book_prog_node))
            {
                if(g_AppFullArb.state.pause == STATE_ON)
                {
                    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                    g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
                }

                mode = (pause_bak == STATE_ON) ? (PLAY_MODE_WITH_REC|PLAY_TYPE_FORCE) : PLAY_MODE_WITH_REC;
                time_t rec_time = book_data.duration;
                g_AppPlayOps.set_rec_time(rec_time);
            }
            else
            {
                mode = PLAY_MODE_WITH_REC;
            }
#ifdef SUBTTRANS_SUPPORT
            extern int Gx_SubtTransGetStatus(void);
            extern void Gx_SubtTransStop(void);
            if(1 == Gx_SubtTransGetStatus()) {
                Gx_SubtTransStop();
            }
#endif
        }
        else
        {
            mode = PLAY_TYPE_NORMAL|PLAY_MODE_POINT;
        }
        g_AppPlayOps.program_play(mode, book_prog_pos);

        g_AppFullArb.state.tv = (book_prog_stream_type == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    }
    else
    {
        _book_play_pvr_cb_cancel();
    }
    return 0;
}

#if DVB2IP_SERVER_SUPPORT || SAT2IP_SERVER_SUPPORT
static void _cannot_exec_tip(char *str)
{
     PopDlg pop = {0};
     pop.type = POP_TYPE_NO_BTN;
     pop.format = POP_FORMAT_DLG;
     pop.str = str ;
     pop.mode = POP_MODE_UNBLOCK;
     pop.timeout_sec = 3;
     pop.exit_cb = NULL;
     popdlg_create(&pop);
}
#endif

static status_t app_exec_prog_book(GxBook *book)
{
    BookProgStruct book_data = {0};
    GxMsgProperty_NodeByIdGet book_prog_node = {0};
    _BookExecWnd book_wnd = EXEC_WND_NONE;
    s_focus_wnd = NULL;

#if SAT2IP_SERVER_SUPPORT
    extern bool app_sat2ip_check_playing(void);
    if (app_sat2ip_check_playing())
    {
        app_log_error("\033[31m sat2ip is playing, ignore book!\033[0m\n");
        _cannot_exec_tip(STR_ID_SAT2IP_CANNOT_BOOK);
        return GXCORE_ERROR;
    }
#endif
#if DVB2IP_SERVER_SUPPORT
    extern bool app_dvb2ip_check_playing(void);
    if (app_dvb2ip_check_playing())
    {
        app_log_error("\033[31m dvb2ip is playing, ignore book!\033[0m\n");
        _cannot_exec_tip(STR_ID_DVB2IP_CANNOT_BOOK);
        return GXCORE_ERROR;
    }
#endif

    book_wnd = app_exec_prog_book_wnd();
    if(book_wnd == EXEC_WND_NONE)
    {
        return GXCORE_ERROR;
    }

    memcpy(&book_data, (BookProgStruct *)book->struct_buf, sizeof(BookProgStruct));
    book_prog_node.node_type = NODE_PROG;
    book_prog_node.id = book_data.prog_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&book_prog_node)))
    {
        if(GXBUS_PM_PROG_BOOL_ENABLE == book_prog_node.prog_data.skip_flag)
        {
            APP_TIMER_ADD(s_book_pop_timer, _no_program_pop_timer, 500, TIMER_REPEAT);
            return GXCORE_ERROR;
        }
        if(app_check_prog_visible(book_data.prog_id) == false)
        {
            APP_TIMER_ADD(s_book_pop_timer, _no_program_pop_timer, 500, TIMER_REPEAT);
            return GXCORE_ERROR;
        }
    }
    else
    {
        APP_TIMER_ADD(s_book_pop_timer, _no_program_pop_timer, 500, TIMER_REPEAT);
        return GXCORE_ERROR;
    }

    if(book_wnd == EXEC_WND_FULLSCREEN)
    {
        app_prog_book_wnd_proc();
    }

    g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
    if((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
            && (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MEDIA_INFO))
            && (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MUSIC_SET)))
    {
        music_view_mode_pause();
        GxCore_ThreadDelay(200);
        GUI_SetInterface("flush", NULL);
    }
#if BLUETOOTH_SUPPORT
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SINK_VIEW))
    {
        sink_view_mode_pause();
    }
#endif
    if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_VOLUME);
    }

    BookDlg book_pop;
    memset(&book_pop, 0, sizeof(BookDlg));
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
    {
        book_pop.pos.x = APP_MM_BOOK_POS_X;
        book_pop.pos.y = APP_MM_BOOK_POS_Y;
    }
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
    {
		book_pop.pos.x = APP_MM_EDIT_BOOK_X;
		book_pop.pos.y = APP_MM_EDIT_BOOK_Y;
    }
    memcpy(&book_pop.cache.book, book, sizeof(GxBook));
    book_pop.cache.book_wnd = book_wnd;
    book_pop.type = book->book_type;
    book_pop.format = POP_FORMAT_DLG;
    book_pop.exit_cb = _book_exec_pvr_play_cb;
    book_popdlg_create(&book_pop);

    return GXCORE_SUCCESS;
}

int app_trigger_end_window(void)
{
    _BookExecWnd book_wnd = EXEC_WND_NONE;

    if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_VOLUME);
    }

    book_wnd = app_exec_prog_book_wnd();
    if(book_wnd == EXEC_WND_NONE)
    {
        return GXCORE_ERROR;
    }

    app_prog_book_wnd_proc();
    g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);

    GUI_SetInterface("flush", NULL);

    return GXCORE_SUCCESS;
}

static void _power_off(GxBook *book)
{
    time_t timerDur;
    time_t cur_time;

    if(NULL == book)
    {
        return;
    }
    if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
    {
        cur_time = book->trigger_time_start;
    }
    else
    {
        time_t time_temp1;
        time_t time_temp2;

        time_temp1 = g_AppTime.utc_get(&g_AppTime);
        time_temp1 = time_temp1 - time_temp1 % SEC_PER_DAY;
        time_temp2 = book->trigger_time_start % SEC_PER_DAY;
        cur_time = time_temp1 + time_temp2;
    }

    cur_time = g_AppTime.utc_get(&g_AppTime);
    timerDur =  g_AppBook.get_sleep_time(cur_time);
    GxCore_ThreadDelay(10);
    g_AppPlayOps.program_stop();
    app_low_power(timerDur, 0);
}

static bool app_ched_fullstate_change(FullArb *arb)
{
    bool ret = false;
    static int channeledit_fullstate_bak = -1;

    if(channeledit_fullstate_bak != (int)arb->tip)
    {
        ret = true;
        channeledit_fullstate_bak = (int)arb->tip;
    }

    return ret;
}

static int timer_power_off_refresh_ch_ed(void *userdata)
{
#define CH_ED_SIGNAL        "txt_channel_edit_signal"

    static uint8_t play_delay = 0;

    if (g_AppFullArb.tip == FULL_STATE_DUMMY)
    {
        app_ched_fullstate_change(&g_AppFullArb);//update epg_fullstate_bak
        if((g_AppFullArb.state.tv == STATE_ON) && (app_check_av_running() == true))//
        {
            if(play_delay >= 0)
            {
                GUI_SetProperty(CH_ED_SIGNAL, "state", "hide");
                GUI_SetInterface("video_on", NULL);
                play_delay = 0;
            }
            else
            {
                play_delay++;
            }
        }
    }
    else
    {
        if(app_ched_fullstate_change(&g_AppFullArb) == true)
        {
            //app_log_info("\n################ line:%d	 [ channeledit_fullstate have changed ]\n",__LINE__);
            if(g_AppFullArb.state.tv == STATE_ON)
            {
                //app_log_info("---[%s]%d video_off---\n", __func__, __LINE__);
                GUI_SetInterface("video_off", NULL);
            }
        }
    }
    return 0;
}

static int _book_power_off_cb(BookDlgRet *book_ret)
{
    if(NULL == book_ret)
    {
        return -1;
    }

    if(POP_VAL_OK == book_ret->ret)
    {
        if(g_AppPvrOps.state != PVR_DUMMY)
        {
            if(g_AppPvrOps.state == PVR_TIMESHIFT)
            {
                app_pvr_tms_stop();
            }
            else
            {
                app_pvr_stop();
            }
        }
        GxCore_ThreadDelay(50);
        _power_off(&book_ret->cache.book);
    }
    else
    {
        if(GUI_CheckDialog(WND_SYSTEM_SETTING) == GXCORE_SUCCESS)
        {
            if(g_AppPvrOps.usb_check(&g_AppPvrOps) == USB_ERROR)
            {
                if(app_system_get_title() != NULL && 0 == strcmp(app_system_get_title(), STR_ID_FIRMWARE_UPGRADE))
                {
                    app_upgrade_clear_path();
                }
            }
        }
    }
#if NETWORK_SUPPORT && WIFI_SUPPORT
    if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
    {
        app_wifi_tip_flush();
    }
#endif
    APP_TIMER_REMOVE(sp_PowerOffChTimer);
    return 0;
}

static status_t app_exec_off_book(GxBook *book)
{
    status_t ret = GXCORE_ERROR;

    s_focus_wnd = NULL;
    s_focus_wnd = GUI_GetFocusWindow();
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_KEYBOARD_LANGUAGE);
        GUI_SetInterface("flush", NULL);
    }

    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_SORT))
        GUI_EndDialog(WND_POP_SORT);


    if(strcmp(GUI_GetFocusWindow(), WND_CHANNEL_LIST) == 0)
    {
        static GUI_Event event;
        event.type = GUI_KEYDOWN;
        event.key.sym = VK_BOOK_TRIGGER;
        GUI_SendEvent(WND_CHANNEL_LIST, &event);
    }
#if BOUQUET_SUPPORT
	if(strcmp(GUI_GetFocusWindow(), WND_BOUQUET) == 0)
    {
        static GUI_Event event;
        event.type = GUI_KEYDOWN;
        event.key.sym = VK_BOOK_TRIGGER;
        GUI_SendEvent(WND_BOUQUET, &event);
    }
#endif
#if (OTA_SUPPORT > 0)
    if(strcmp(s_focus_wnd, WND_OTA_UPGRADE) == 0)
    {
        if(app_ota_idle() == false)
        {
            return  ret;
        }
    }
#endif
#if QR_POP_SUPPORT
	if(strcmp(GUI_GetFocusWindow(), WND_POP_QR) == 0)
    {
        GUI_EndDialog(WND_POP_QR);
    }
#endif

    if((GUI_CheckDialog(WND_SEARCH) != GXCORE_SUCCESS)
            && (GUI_CheckDialog(WND_UPGRADE_PROCESS) != GXCORE_SUCCESS)
            && (GUI_CheckDialog(WND_OTA_UPGRADE) != GXCORE_SUCCESS))
    {
        if(GUI_CheckDialog(WIN_MUSIC_VIEW) == GXCORE_SUCCESS
                && GXCORE_SUCCESS != GUI_CheckDialog(WIN_MUSIC_SET)
                && GXCORE_SUCCESS != GUI_CheckDialog(WIN_MEDIA_INFO))
        {
            music_view_mode_pause();
            GxCore_ThreadDelay(200);
            GUI_SetInterface("flush", NULL);
        }
#if NETWORK_SUPPORT && WIFI_SUPPORT
        if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
        {
            app_wifi_tip_lost_focus_hide();
        }
#endif
        if(app_channel_info_signal_check_dialog() == GXCORE_SUCCESS)
        {
            app_channel_info_signal_end_dialog();
            GUI_SetInterface("flush", NULL);
        }
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD))
        {
            GUI_EndDialog(WND_PASSWD);
            GUI_SetInterface("flush", NULL);
        }
        if(GUI_CheckDialog(WND_ZOOM) == GXCORE_SUCCESS)
        {
            static GUI_Event event;
            event.type = GUI_KEYDOWN;
            event.key.sym = VK_BOOK_TRIGGER;
            GUI_SendEvent(WND_ZOOM, &event);
        }

        g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
        {
            if( reset_timer(sp_PowerOffChTimer) != 0 )
            {
                sp_PowerOffChTimer = create_timer(timer_power_off_refresh_ch_ed, 1000, NULL, TIMER_REPEAT);
            }
        }

        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PVR_BAR))
        {
            GUI_EndDialog(WND_PVR_BAR);
            GUI_SetInterface("flush", NULL);
        }

        BookDlg book_pop;
        memset(&book_pop, 0, sizeof(BookDlg));
        memcpy(&book_pop.cache.book, book, sizeof(GxBook));
        book_pop.type = book->book_type;
        book_pop.format = POP_FORMAT_DLG;
        book_pop.exit_cb = _book_power_off_cb;
        book_popdlg_create(&book_pop);
        ret = GXCORE_SUCCESS;
    }

    return ret;
}

static int32_t app_get_book(GxBookGet *pbookGet, BookMode BookListType)
{
    int32_t count = 0;

    if(pbookGet == NULL)
        return 0;

    memset(pbookGet,0,sizeof(GxBookGet));

    if(BookListType == BOOKMODE_ALL)
         pbookGet->book_type = BOOK_POWER_ON|BOOK_POWER_OFF|BOOK_PROGRAM_PLAY|BOOK_PROGRAM_PVR;
    else if(BookListType == BOOKMODE_PLAY)//play
        pbookGet->book_type = BOOK_PROGRAM_PLAY | BOOK_PROGRAM_PVR;
    else if(BookListType == BOOKMODE_POWER)//power on/off
        pbookGet->book_type = BOOK_POWER_ON|BOOK_POWER_OFF;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_BOOK_GET, (void *)(&pbookGet)))
        count = pbookGet->book_number;
    else
        APP_PRINT("get book ERROR\n");
    return count;
}

static status_t app_creat_book(GxBook *book)
{
    status_t ret = GXCORE_ERROR;

    if(book == NULL)
    {
        return GXCORE_ERROR;
    }

    if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
    {
        time_t sec;
        sec = g_AppTime.utc_get(&g_AppTime);
        if(book->trigger_time_start <= sec)
        {
            return GXCORE_ERROR;
        }
    }

    book->id = -1;
    if(app_book_time_valid(book) == true)
    {
        ret = app_send_msg_exec(GXMSG_BOOK_CREATE, (void *)(&book));
    }

    return ret;
}

static status_t app_modify_book(GxBook *book)
{
    status_t ret = GXCORE_ERROR;

    if(book == NULL)
    {
        return GXCORE_ERROR;
    }

    if(book->repeat_mode.mode == BOOK_REPEAT_ONCE)
    {
        time_t sec;
        sec = g_AppTime.utc_get(&g_AppTime);
        if(book->trigger_time_start <= sec)
        {
            return GXCORE_ERROR;
        }
    }

    if(app_book_time_valid(book) == true)
    {
        ret = app_send_msg_exec(GXMSG_BOOK_MODIFY, (void *)(&book));
    }

    return ret;
}

static status_t app_remove_book(GxBook *book)
{
    status_t ret = GXCORE_ERROR;

    if(book == NULL)
    {
        return GXCORE_ERROR;
    }

    ret = app_send_msg_exec(GXMSG_BOOK_DESTROY, (void *)(&book));

    return ret;

}

static status_t app_remove_invalid_book(void)
{
    status_t ret = GXCORE_ERROR;
    GxBookGet bookGet;
    int count = 0;
    int i;
    time_t sec;

    sec = g_AppTime.utc_get(&g_AppTime);

    count = app_get_book(&bookGet, BOOKMODE_ALL);

    for(i = 0; i < count; i++)
    {
        if(((bookGet.book[i].book_type&BOOK_PROGRAM_PLAY) == BOOK_PROGRAM_PLAY)
                || ((bookGet.book[i].book_type&BOOK_PROGRAM_PVR) == BOOK_PROGRAM_PVR))
        {
            BookProgStruct book_data = {0};
            GxMsgProperty_NodeByIdGet node = {0};

            memcpy(&book_data, (BookProgStruct*)(bookGet.book[i].struct_buf), sizeof(BookProgStruct));

            node.node_type = NODE_PROG;
            node.id = book_data.prog_id;
            if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node)) == GXCORE_ERROR)
            {
                app_remove_book(&(bookGet.book[i]));
                ret = GXCORE_SUCCESS;
                continue;
            }
        }

        if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
        {
            if((sec > bookGet.book[i].trigger_time_end)
                    || (bookGet.book[i].trigger_mode == BOOK_TRIG_OFF))
            {
                app_remove_book(&(bookGet.book[i]));
                ret = GXCORE_SUCCESS;
            }
        }
    }

    return ret;
}

static status_t app_exec_book(GxBook *book)
{
    status_t ret = GXCORE_ERROR;

    if((book == NULL)
            || (s_book_enable == false))
    {
        return GXCORE_ERROR;
    }

    if(book->repeat_mode.mode ==  BOOK_REPEAT_ONCE)
    {
        const char *wnd = NULL;

        wnd = GUI_GetFocusWindow();
        app_remove_book(book);
        if(NULL != wnd)
        {
            if(strcmp(wnd, WND_TIMER_SETTING) == 0)
            {
                app_timer_setting_update_timer_list();
            }
        }
    }

    switch(book->book_type)
    {
        case BOOK_PROGRAM_PLAY:
        case BOOK_PROGRAM_PVR:
            ret = app_exec_prog_book(book);
            break;

        case BOOK_POWER_OFF:
            ret = app_exec_off_book(book);
            break;

        default:
            break;
    }

    return ret;
}

static void app_enable_book(void)
{
    s_book_enable = true;
    GxBookSetStopFlag(0);
}

static int _timer_enable_book(void* usrdata)
{
    app_enable_book();
    s_book_ignore_timer = NULL;
    return 0;
}

static void app_disable_book(time_t sec) //0, forever; 1,ignor sec
{
    time_t ignore_ms = sec * 1000;

    s_book_enable = false;
    if((sec != 0) && (0 != reset_timer(s_book_ignore_timer)) )
    {
        s_book_ignore_timer = create_timer(_timer_enable_book, ignore_ms, NULL, TIMER_ONCE);
    }

}

AppBook g_AppBook =
{
    .get          = app_get_book,
    .creat           = app_creat_book,
    .modify           = app_modify_book,
    .remove       = app_remove_book,
    .invalid_remove       = app_remove_invalid_book,
    .exec        = app_exec_book,
    .get_sleep_time = app_get_sleep_time,
    .get_pvr_time   = app_get_book_time,
    .time2wday = app_tm_get_wday,
    .mode2wday = app_book_mode2wday,
    .wday2mode = app_book_wday2mode,
    .enable = app_enable_book,
    .disable = app_disable_book,
};

