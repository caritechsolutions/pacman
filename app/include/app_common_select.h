#include "gui_core.h"
#ifndef __APP_COMMON_SELECT_H__
#define __APP_COMMON_SELECT_H__

typedef enum {
    COMMON_SELECT_TITLE_ONLY = 0,
    COMMON_SELECT_GROUP_ONLY,
    COMMON_SELECT_TITLE_GROUP
} CommonSelectChoice;

typedef int (*common_select_signal_callback)(GuiWidget *widget, void *usrdata);

typedef struct {
    const char *img_bg;
    const char *img_line_bg;
    const char *img_red;
    const char *text_red;
    const char *img_green;
    const char *text_green;
    const char *img_yellow;
    const char *text_yellow;
    const char *img_blue;
    const char *text_blue;
    const char *img_ok;
    const char *text_ok;
    const char *img_exit;
    const char *text_exit;
} CommonSelectWidgetTips;

typedef struct {
    const char *str_red;
    const char *str_green;
    const char *str_yellow;
    const char *str_blue;
    const char *str_ok;
    const char *str_exit;
} CommonSelectValueTips;

typedef struct {
    const char *wnd;
    const char *title;
    const char *group;
    const char *list;
    CommonSelectWidgetTips tips;
} CommonSelectWidget;

typedef struct {
    common_select_signal_callback create;
    common_select_signal_callback destroy;
    common_select_signal_callback got_focus;
    common_select_signal_callback lost_focus;
    common_select_signal_callback keypress;
    common_select_signal_callback group_change;
    common_select_signal_callback list_get_total;
    common_select_signal_callback list_get_data;
    common_select_signal_callback list_change;
    common_select_signal_callback list_keypress;
} CommonSelectSignal;

typedef struct {
    CommonSelectChoice choice;
    const char *title;
    const char *content;
    CommonSelectWidget widget;
    CommonSelectValueTips tip_strs;
    CommonSelectSignal signal;
} CommonSelectControl;

typedef struct {
    CommonSelectChoice choice;
    const char *title;
    const char *content;
    CommonSelectValueTips tip_strs;
    CommonSelectSignal signal;
} CommonSelectParas;

extern int app_common_select_create_dialog(CommonSelectParas *paras);
extern CommonSelectControl g_CommonSelect;
#endif

