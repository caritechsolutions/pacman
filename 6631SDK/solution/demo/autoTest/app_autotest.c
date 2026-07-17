#include "app.h"

static event_list *autotest_timer = NULL;
static int autotest_counter = 0;

static int _tv_radio_in_full_screen(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();
    static bool change = false;
    static GUI_Event event;
    event.type = GUI_KEYDOWN;

    if (0 == strcasecmp("wnd_full_screen", wnd)) {
        if (change) {
            event.key.sym = STBK_RECALL;
            GUI_SendEvent("wnd_full_screen", &event);
            change = false;
        }
        else {
            change = true;
        }
    }
    else if (0 == strcasecmp("wnd_channel_info", wnd)) {
        if (change) {
            event.key.sym = STBK_RECALL;
            GUI_SendEvent("wnd_channel_info", &event);
            change = false;
        }
        else {
            change = true;
        }
    }
    else if (0 == strcasecmp("wnd_passwd", wnd)) {
        event.key.sym = STBK_0;
        GUI_SendEvent("wnd_passwd", &event);
    }
    return 0;
}

//please add in func app_atn_box_keypress <autotest_handler(event->key.sym, STBK_1, STBK_2)>
static int _quicksearch_in_antenna_setting(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();
    static GUI_Event event;
    event.type = GUI_KEYDOWN;
    static int num = 0;

    if (0 == strcasecmp("wnd_antenna_setting", wnd)) {
        event.key.sym = STBK_OK;
        GUI_SendEvent("wnd_antenna_setting", &event);
    }
    else if (0 == strcasecmp("wnd_sat_search_opt", wnd)) {
        event.key.sym = STBK_OK;
        GUI_SendEvent("box_sat_search_opt", &event);
    }
    else if (0 == strcasecmp("wnd_search", wnd)) {
        if (num++ > 3) {
            num = 0;
            event.key.sym = STBK_EXIT;
            GUI_SendEvent("wnd_search", &event);
        }
    }
    else {
        event.key.sym = STBK_OK;
        GUI_SendEvent(wnd, &event);
    }
    return 0;
}

static int _search_in_antenna_setting(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();
    static GUI_Event event;
    event.type = GUI_KEYDOWN;

    if (0 == strcasecmp("wnd_antenna_setting", wnd)) {
        event.key.sym = STBK_OK;
        GUI_SendEvent("wnd_antenna_setting", &event);
    }
    else if (0 == strcasecmp("wnd_sat_search_opt", wnd)) {
        event.key.sym = STBK_OK;
        GUI_SendEvent("box_sat_search_opt", &event);
    }
    else if (0 == strcasecmp("wnd_search_tip", wnd)) {
        autotest_counter++;
        app_log_min("\033[1;32m[AUTOTEST] Search Counter : %d\n\033[0m", autotest_counter);
        event.key.sym = STBK_OK;
        GUI_SendEvent(wnd, &event);
    }
    else if (0 == strcasecmp("wnd_search", wnd)) {
    }
    else {
    }

    return 0;
}

/*
 * #89928
 */
#define SWITCH_INTERVAL 50
static int _switch_lnb_in_antenna_setting(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();
    static GUI_Event event;
    event.type = GUI_KEYDOWN;
    static int counter = 0;

    if (0 == strcasecmp("wnd_antenna_setting", wnd)) {
        if (counter > 50) {
            counter = 0;
            event.key.sym = STBK_OK;
            GUI_SendEvent("wnd_antenna_setting", &event);
            app_log_min("\e[1;31m Enter pop option list\n\e[0m");
        }
        else {
            counter++;
            event.key.sym = STBK_RIGHT;
            GUI_SendEvent("box_antenna_options", &event);
        }
    }
    else if (0 == strcasecmp("wnd_pop_option_list", wnd)) {
        if (counter > 50) {
            counter = 0;
            event.key.sym = STBK_EXIT;
            GUI_SendEvent("wnd_pop_option_list", &event);
            app_log_min("\e[1;31m Exit pop option list\n\e[0m");
        }
        else {
            counter++;
            event.key.sym = STBK_DOWN;
            GUI_SendEvent("listview_pop_option", &event);
        }
    }
    else {
    }

    return 0;
}

int app_autotest_handler(unsigned short key, unsigned short start, unsigned short stop)
{
    if (key == start) {
        app_log_min("\033[1;32m[AUTOTEST] Start!!\n\033[0m");
        autotest_counter = 0;
        APP_TIMER_ADD(autotest_timer, _search_in_antenna_setting, 1000, TIMER_REPEAT);
    }
    else if (key == stop) {
        app_log_min("\033[1;32m[AUTOTEST] Stop!! Counter : %d\n\033[0m", autotest_counter);
        autotest_counter = 0;
        APP_TIMER_REMOVE(autotest_timer);
    }

    return 0;
}
