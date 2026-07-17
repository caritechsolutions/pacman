#include "app.h"
#include "app_module.h"
#include "app_utility.h"
#include "app_send_msg.h"
#include "app_book.h"
#include "app_pop.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_channel_info.h"

#define LIST_VIEW_TM            "list_timer_setting_timer"
#define IMG_TIMER_SETTING_TITLE "img_timer_setting_title"
#define TXT_TIMER_SETTING_TITLE "text_timer_setting_title"
#define IMG_TIMER_LIST_BACK    "img_timer_list_back"
#define IMG_TIMER_LIST_BACK1    "img_timer_list_back1"
#define TXT_TIMER_SETTING_TITLE "text_timer_setting_title"

#define EVENT_PVR "PVR"
#define BLANK_PROG "----"
#define BLANK_DURA "--:--"

#define LOCAL_DATE_LEN 40

#define SEC_PER_DAY (60*60*24)

static GxBookGet bookGet;
static uint32_t book_id[APP_BOOK_NUM] = {0};
static bool s_timer_on_fullscreen = false;
static bool s_add_timer_flag = false;

extern bool app_add_timer_menu_exec(void);
extern bool app_edit_timer_menu_exec(GxBook *book);
extern char *app_timer_display_name_get(GxBusPmDataProg *prog);
void app_timer_setting_menu_exec(void);
void app_timer_play_menu_exec(void);

static event_list* s_app_timer_setting_update_timer = NULL;
static int app_timer_setting_update_func(void *userdata)
{
	const char *wnd = NULL;
	wnd = GUI_GetFocusWindow();
	if((NULL != wnd) && (strcmp(wnd, WND_TIMER_SETTING) == 0))
	{
		if (GXCORE_SUCCESS == g_AppBook.invalid_remove())
		{
			int count = 0;
			GUI_SetProperty(LIST_VIEW_TM, "select",&count);
			GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
		}
	}
	return 0;
}

void app_add_timer_flag_set(bool bflag)
{
    s_add_timer_flag = bflag;
}

int app_timer_setting_update_timer_list(void)
{
    GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
    return 0;
}

static char* app_time_mode_to_string(uint32_t repeat_mode)
{
    char *str = NULL;
    switch(repeat_mode)
    {
        case BOOK_REPEAT_ONCE:
            str = STR_ID_ONCE;
            break;

        case BOOK_REPEAT_EVERY_DAY:
            str = STR_ID_DAILY;
            break;

        case BOOK_REPEAT_MON:
            str =  STR_ID_MODE_MON;
            break;

        case BOOK_REPEAT_TUES:
            str =  STR_ID_MODE_TUES;
            break;

        case BOOK_REPEAT_WED:
            str =  STR_ID_MODE_WED;
            break;

        case BOOK_REPEAT_THURS:
            str =  STR_ID_MODE_THURS;
            break;

        case BOOK_REPEAT_FRI:
            str =  STR_ID_MODE_FRI;
            break;

        case BOOK_REPEAT_SAT:
            str =  STR_ID_MODE_SAT;
            break;

        case BOOK_REPEAT_SUN:
            str =  STR_ID_MODE_SUN;
            break;

        default:
            break;
    }

    return str;
}

static void _sort_book_id(time_t * a,uint32_t * b,uint32_t num)
{
    uint32_t i,j,tmp = 0;
    for(i=0;i<num;i++)
    {
        for(j=0;j<num-i-1;j++)
        {
            if(a[j]>a[j+1])
            {
                tmp = a[j];
                a[j] = a[j+1];
                a[j+1] = tmp;

                tmp = b[j];
                b[j] = b[j+1];
                b[j+1] = tmp;
            }
        }
    }
}

void app_create_timer_setting_fullsreeen(void)
{

    if(app_channel_info_check_dialog()== GXCORE_SUCCESS)
    {
        app_channel_info_end_dialog();
        GUI_SetInterface("flush", NULL);
    }

    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        app_pvr_create_no_btn_popdlg(NULL);
    }
    else
    {
        app_clean_fullscreen_tip(false, false, false);
        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        g_AppPlayOps.program_stop();
        if( APP_THEME_SKY_BLUE || APP_THEME_CLASSIC_PURPLE ){
            app_timer_play_menu_exec();
        }else{
            app_timer_setting_menu_exec();
        }
        s_timer_on_fullscreen = true;
    }
}

static void app_timer_setting_quit(void)
{
    GUI_EndDialog(WND_TIMER_SETTING);
    GUI_SetProperty(WND_TIMER_SETTING,"draw_now",NULL);

    if(s_timer_on_fullscreen == true)
    {
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        if (g_AppPlayOps.normal_play.play_total != 0)
        {
            app_play_current_prog(PLAY_MODE_POINT);
        }
    }
    s_timer_on_fullscreen = false;
}

int app_book_get_total_num(void)
{
    int count = 0;
    BookMode mode = BOOKMODE_ALL;

    app_booklist_type_get(&mode);
    count = g_AppBook.get(&bookGet, mode);
    return count;
}

static int _timer_setting_delete_cb(PopDlgRet ret)
{
    int sel;
    int tmp;
    GUI_GetProperty(LIST_VIEW_TM, "select", &sel);

    if(sel < 0 || sel >= APP_BOOK_NUM)
        return -1;

    tmp = book_id[sel];
    if(POP_VAL_OK == ret)
    {
        if(g_AppBook.remove(&bookGet.book[tmp]) == GXCORE_SUCCESS)
        {
            GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
            if( bookGet.book_number > 0)
            {
                if(sel >= bookGet.book_number )
                {
                    sel = bookGet.book_number - 1;
                }
                GUI_SetProperty(LIST_VIEW_TM, "select", &sel);
            }
        }
    }
    return 0;
}

static int _timer_setting_delete_all_cb(PopDlgRet ret)
{
    int sel;
    int tmp;
    if(POP_VAL_OK == ret)
    {
        for(sel = 0; sel < bookGet.book_number; sel++)
        {
            tmp = book_id[sel];
            if(g_AppBook.remove(&bookGet.book[tmp]) == GXCORE_SUCCESS)
            {
                GUI_SetProperty(LIST_VIEW_TM, "select", &sel);
            }
        }
        GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
    }

    return 0;
}

static bool _timer_exist_once_mode(void)
{
    int32_t i = 0;
    int32_t timer_num = 0;

    timer_num = app_book_get_total_num();

    for(i = 0; i < timer_num; i++)
    {
        if(bookGet.book[i].repeat_mode.mode == BOOK_REPEAT_ONCE)
            return true;
    }

    return false;
}

SIGNAL_HANDLER int app_timer_list_get_total(GuiWidget *widget, void *usrdata)
{
    return app_book_get_total_num();
}

SIGNAL_HANDLER int app_timer_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    static char tmStart[LOCAL_DATE_LEN] = {0};
    time_t display_time;
    uint32_t mode;
    WeekDay gmt_wday;
    WeekDay local_wday;
    BookProgStruct book_prog = {0};
    uint32_t i = 0;
    time_t time_for_sort[APP_BOOK_NUM] = {0};
    uint32_t book_sel_map = 0;
    bool timer_once_exist = false;
    int32_t book_total_num = 0;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
    {
        return EVENT_TRANSFER_KEEPON;
    }

    book_total_num = app_book_get_total_num();
    if(0 == book_total_num)
        return EVENT_TRANSFER_KEEPON;

    timer_once_exist = _timer_exist_once_mode();
    //for BUG #12748; Modified from item-sel to book_sel_map
    for(i = 0; i < book_total_num; i++)
    {
        if(true == timer_once_exist)
            time_for_sort[i] = get_display_time_by_timezone(bookGet.book[i].trigger_time_start);
        else
            time_for_sort[i] = get_display_time_by_timezone(bookGet.book[i].trigger_time_start) % SEC_PER_DAY;

        book_id[i] = i;
    }
    _sort_book_id(time_for_sort, book_id, book_total_num);
    book_sel_map = book_id[item->sel];
    display_time = get_display_time_by_timezone(bookGet.book[book_sel_map].trigger_time_start);

    if(bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_ONCE)
    {
        app_common_data_time_edit_str(display_time, LONG_DATE_EDIT | SHORT_TIME_EDIT, tmStart, LOCAL_DATE_LEN);
    }
    else
    {
        char *str = NULL;
        str = app_time_to_hourmin_edit_str(display_time);
        if(str != NULL)
        {
            strcpy(tmStart, str);
            GxCore_Free(str);
            str = NULL;
        }
    }
    item->string = tmStart;

    item = item->next;
    if(NULL == item)
        return EVENT_TRANSFER_KEEPON;

    char *event_name = NULL;
    if(BOOK_POWER_ON == bookGet.book[book_sel_map].book_type)
    {
        event_name = STR_ID_POWER_ON;
    }
    else if(BOOK_POWER_OFF == bookGet.book[book_sel_map].book_type)
    {
        event_name = STR_ID_POWER_OFF;
    }
    else if(BOOK_PROGRAM_PLAY == bookGet.book[book_sel_map].book_type)
    {
        event_name = STR_ID_PLAY;
    }
    else if(BOOK_PROGRAM_PVR== bookGet.book[book_sel_map].book_type)
    {
        event_name = EVENT_PVR;
    }
    else{}
    item->string = event_name;

    item = item->next;
    if(NULL == item)
        return EVENT_TRANSFER_KEEPON;

    char *prog_name = BLANK_PROG;;
    if((BOOK_PROGRAM_PLAY == bookGet.book[book_sel_map].book_type)
            || (BOOK_PROGRAM_PVR == bookGet.book[book_sel_map].book_type))
    {
        static GxMsgProperty_NodeByIdGet node;
        memcpy(&book_prog, (BookProgStruct*)bookGet.book[book_sel_map].struct_buf, sizeof(BookProgStruct));
        node.node_type = NODE_PROG;
        node.id = book_prog.prog_id;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node)) == GXCORE_SUCCESS)
        {
            prog_name = app_timer_display_name_get(&node.prog_data);
        }
    }

    item->string = prog_name;

    item = item->next;
    if(NULL == item)
        return EVENT_TRANSFER_KEEPON;

    char *dura_str = NULL;
    if(BOOK_PROGRAM_PVR == bookGet.book[book_sel_map].book_type)
    {
        static char time_value[10] = {0};
        //int duration = bookGet.book[book_sel_map].trigger_time_end - bookGet.book[book_sel_map].trigger_time_start;
        int hour = book_prog.duration / 3600;
        int min = (book_prog.duration % 3600) / 60;
        memset(time_value, 0, sizeof(time_value));
        sprintf(time_value, "%02d:%02d", hour, min);
        dura_str = time_value;
    }
    else
    {
        dura_str = BLANK_DURA;
    }
    item->string = dura_str;

    item = item->next;
    if(NULL == item)
        return EVENT_TRANSFER_KEEPON;

    if((bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_ONCE)
            || (bookGet.book[book_sel_map].repeat_mode.mode == BOOK_REPEAT_EVERY_DAY))
    {
        mode = bookGet.book[book_sel_map].repeat_mode.mode;
    }
    else
    {
        gmt_wday = g_AppBook.mode2wday(bookGet.book[book_sel_map].repeat_mode.mode);
		local_wday = app_weekday_gmt2local(gmt_wday, bookGet.book[book_sel_map].trigger_time_start % SEC_PER_DAY);
        mode = g_AppBook.wday2mode(local_wday);
    }

    item->string = app_time_mode_to_string(mode);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_create(GuiWidget *widget, void *usrdata)
{
    g_AppBook.invalid_remove();
    GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
	APP_TIMER_ADD(s_app_timer_setting_update_timer, app_timer_setting_update_func, 5000, TIMER_REPEAT);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_destroy(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(s_app_timer_setting_update_timer);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_got_focus(GuiWidget *widget, void *usrdata)
{

    g_AppBook.invalid_remove();
    if(s_add_timer_flag == true)
    {
        app_add_timer_flag_set(false);
        int count;
        count = app_book_get_total_num()-1;
        GUI_SetProperty(LIST_VIEW_TM, "select",&count);
    }
    GUI_SetProperty(LIST_VIEW_TM, "update_all", NULL);
	APP_TIMER_ADD(s_app_timer_setting_update_timer, app_timer_setting_update_func, 5000, TIMER_REPEAT);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_timer_setting_lost_focus(GuiWidget *widget, void *usrdata)
{
	APP_TIMER_REMOVE(s_app_timer_setting_update_timer);
	return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_timer_setting_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

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
                case STBK_YELLOW:
                    {
                        int sel;
                        BookMode mode = BOOKMODE_ALL;

                        GUI_GetProperty(LIST_VIEW_TM, "select", &sel);
                        if(sel < 0 || sel >= APP_BOOK_NUM)
                            break;

                        sel = book_id[sel];
                        if(sel >=  bookGet.book_number)
                        {
                            APP_PRINT("the sel item id ERROR when modified the timer sel = %d , book_number =  %d\n",sel,bookGet.book_number);
                            return EVENT_TRANSFER_STOP;
                        }

                        app_booklist_type_get(&mode);
                        g_AppBook.get(&bookGet, mode);

                        if((BOOK_POWER_ON == bookGet.book[sel].book_type)
                                ||(BOOK_POWER_OFF == bookGet.book[sel].book_type)
                                ||BOOK_PROGRAM_PLAY == bookGet.book[sel].book_type
                                ||BOOK_PROGRAM_PVR == bookGet.book[sel].book_type)
                        {
                            app_edit_timer_menu_exec(&bookGet.book[sel]);
                        }
                        break;
                    }

                case STBK_GREEN:
                    {
                        if(bookGet.book_number < APP_BOOK_NUM)
                        {
                            app_add_timer_menu_exec();
                        }
                        else
                        {
                            PopDlg  pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_OK;
                            pop.str = STR_ID_TIMER_FULL;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.format = POP_FORMAT_DLG;
                            pop.pos.x = APP_POP_DLG_POS_X;
                            pop.pos.y = APP_POP_DLG_POS_Y;
                            popdlg_create(&pop) ;
                        }
                        break;
                    }

                case STBK_RED:
                    {
                        int sel;
                        int tmp;
                        GUI_GetProperty(LIST_VIEW_TM, "select", &sel);
                        if(sel < 0 || sel >= APP_BOOK_NUM)
                        {
                            break;
                        }
                        tmp = book_id[sel];
                        if(tmp < bookGet.book_number)
                        {
                            PopDlg  pop;
                            memset(&pop, 0, sizeof(PopDlg));
                            pop.type = POP_TYPE_YES_NO;
                            pop.mode = POP_MODE_UNBLOCK;
                            pop.str = STR_ID_SURE_DELETE;
                            pop.format = POP_FORMAT_DLG;
                            pop.pos.x = APP_POP_DLG_POS_X;
                            pop.pos.y = APP_POP_DLG_POS_Y;
                            pop.exit_cb = _timer_setting_delete_cb;

                            popdlg_create(&pop);
                        }
                        break;
                    }
                case STBK_BLUE:
                    if (bookGet.book_number > 0)
                    {
                        PopDlg  pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_SURE_DELETE_ALL;
                        pop.format = POP_FORMAT_DLG;
                        pop.pos.x = APP_POP_DLG_POS_X;
                        pop.pos.y = APP_POP_DLG_POS_Y;
                        pop.exit_cb = _timer_setting_delete_all_cb;
                        popdlg_create(&pop);
                    }
                    break;

                case STBK_EXIT:
                case STBK_MENU:
                    app_timer_setting_quit();
                    break;

                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    GUI_SetInterface("flush",NULL);
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

void app_timer_setting_menu_exec(void)
{
    app_booklist_type_set(BOOKMODE_ALL);
    GUI_CreateDialog(WND_TIMER_SETTING);
}

void app_timer_play_menu_exec(void)
{
    app_booklist_type_set(BOOKMODE_PLAY);
    GUI_CreateDialog(WND_TIMER_SETTING);
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
    {
        GUI_SetProperty(IMG_TIMER_SETTING_TITLE, "state", "hide");
        GUI_SetProperty(IMG_TIMER_LIST_BACK, "state", "hide");
        GUI_SetProperty(IMG_TIMER_LIST_BACK1, "state", "show");
    }
    else
    {
        GUI_SetProperty(IMG_TIMER_SETTING_TITLE, "state", "show");
        GUI_SetProperty(TXT_TIMER_SETTING_TITLE, "state", "show");
        GUI_SetProperty(IMG_TIMER_LIST_BACK, "state", "show");
        GUI_SetProperty(IMG_TIMER_LIST_BACK1, "state", "hide");
        GUI_SetProperty(TXT_TIMER_SETTING_TITLE,"string", STR_ID_BOOK_LIST);
    }
}
