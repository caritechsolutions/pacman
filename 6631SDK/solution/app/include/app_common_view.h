#include "gui_core.h"
#ifndef __APP_COMMON_VIEW_H__
#define __APP_COMMON_VIEW_H__

#include "app_config.h"

#if COMMON_VIEW_V2_SUPPORT
#define COMMON_VIEW_MAX_PAGE_ITEM   12
#define COMMON_VIEW_MAX_ROW_ITEM    3
#define COMMON_VIEW_MAX_COL_ITEM    4
#else
#define COMMON_VIEW_MAX_PAGE_ITEM   4
#define COMMON_VIEW_MAX_ROW_ITEM    4
#define COMMON_VIEW_MAX_COL_ITEM    1
#endif
typedef struct {
    char *logo;
    char *title;
    char *author;
    char *viewcnt;
    char *duration;
} CommonViewItemData;

typedef enum {
    COMMON_VIEW_TABLE_ITEM_TYPE = 0,
    COMMON_VIEW_LIST_ITEM_TYPE
} CommonViewShowType;

typedef enum {
    COMMON_VIEW_AREA_LIST_GROUP = 0,
    COMMON_VIEW_AREA_TABLE_ITEM,
    COMMON_VIEW_AREA_LIST_ITEM,
    COMMON_VIEW_AREA_TOTAL
} CommonViewFocusArea;

typedef enum {
    COMMON_VIEW_NETVIDEO = 0,
    COMMON_VIEW_IPTVVIDEO,
    COMMON_VIEW_USBFILE
} CommonViewWndType;

typedef struct {
    const char *bg;
    const char *logo;
    const char *title;
    const char *author;
    const char *duration;
    const char *viewcnt;
    const char *line;
} CommonViewBaseItem;

typedef struct {
    const char *img_exit;
    const char *text_exit;
    const char *img_menu;
    const char *text_menu;
    const char *img_ok;
    const char *text_ok;
    const char *img_move;
    const char *text_move;

    const char *img_red;
    const char *text_red;
    const char *img_green;
    const char *text_green;
    const char *img_yellow;
    const char *text_yellow;
    const char *img_blue;
    const char *text_blue;
} CommonViewWidgetTips;

typedef struct {
    const char *img_title;
} CommonViewImageValue;

typedef struct {
    const char *str_title;
    const char *str_item_title;
    const char *str_menu;
    const char *str_ok;
    const char *str_red;
    const char *str_green;
    const char *str_yellow;
    const char *str_blue;
} CommonViewStringValue;

typedef struct {
    const char *wnd;
    const char *img_title;
    const char *text_title;
    const char *item_title;
    const char *sys_date;
    const char *sys_time;
    const char *list_group;
    const char *list_item;
    const char *item_page;
    const char *img_up;
    const char *img_down;
    const char *img_gif;
    const char *img_popup;
    const char *text_popup;
    CommonViewBaseItem base_item;
    CommonViewWidgetTips tips;
} CommonViewWidget;

typedef int (*common_view_signal_callback)(GuiWidget *widget, void *usrdata);
typedef struct {
    common_view_signal_callback create;
    common_view_signal_callback destroy;
    common_view_signal_callback service;
    common_view_signal_callback got_focus;
    common_view_signal_callback lost_focus;
    common_view_signal_callback keypress;

    common_view_signal_callback group_list_get_total;
    common_view_signal_callback group_list_get_data;
    common_view_signal_callback group_list_change;
    common_view_signal_callback group_list_keypress;

    common_view_signal_callback item_list_get_total;
    common_view_signal_callback item_list_get_data;
    common_view_signal_callback item_list_change;
    common_view_signal_callback item_list_keypress;
} CommonViewSignal;

typedef void (*common_view_key_callback)(void);
typedef int (*common_view_get_callback)(void);
typedef void (*common_view_set_callback)(int);

typedef struct {
    common_view_key_callback key_menu;
    common_view_key_callback key_ok;
    common_view_key_callback key_mute;
    common_view_key_callback key_red;
    common_view_key_callback key_green;
    common_view_key_callback key_yellow;
    common_view_key_callback key_blue;
    common_view_key_callback exit_item_depth;

    common_view_get_callback busy_check;
    common_view_get_callback get_item_depth;

    common_view_set_callback set_item_depth;
    common_view_set_callback focus_area_change;
    common_view_set_callback group_sel_change;
    common_view_set_callback item_sel_change;

    common_view_set_callback page_sel_change;
    common_view_set_callback page_item_sel_change;
} CommonViewKeyFunc;

typedef struct {
    event_list *clock;
    event_list *gif;
    event_list *popup;
    event_list *logoup;
} CommonViewTimer;

typedef struct {
    // group list
    int orig_group_sel;
    int group_total;
    int group_sel;
    char **group_str;

    // item list
    int item_total;
    int item_sel;
    char **item_logo;
    char **item_str;
    bool item_num_flag;

    // item table
    int page_total;
    int page_sel;
    int page_item_num;
    int page_item_sel;
    CommonViewItemData page_item_data[COMMON_VIEW_MAX_PAGE_ITEM];
    unsigned int pic_download_handle[COMMON_VIEW_MAX_PAGE_ITEM];
    int pic_download_ok[COMMON_VIEW_MAX_PAGE_ITEM];
    int pic_show_ok[COMMON_VIEW_MAX_PAGE_ITEM];
    int pic_download_abort;
} CommonViewCtrlData;

typedef struct {
    CommonViewShowType show_type;
    CommonViewFocusArea focus_area;
    bool net_video_flag;
    bool first_in_flag;
    bool popup_show_flag;

    CommonViewWidget widget;
    CommonViewImageValue image;
    CommonViewStringValue string;
    CommonViewSignal signal;
    CommonViewKeyFunc keyfunc;
    CommonViewTimer timer;
    CommonViewCtrlData ctrl_data;
} CommonViewControl;

typedef struct {
    CommonViewShowType show_type;
    CommonViewFocusArea focus_area;
    bool net_video_flag;
    int orig_group_sel;

    CommonViewImageValue image;
    CommonViewStringValue string;
    CommonViewSignal signal;
    CommonViewKeyFunc keyfunc;
} CommonViewParas;

extern CommonViewControl g_CommonView;

void app_common_view_hide_one_item(int index);
void app_common_view_show_one_item(int index);
void app_common_view_clear_one_item(int index, bool clear_logo);
void app_common_view_display_one_item(int index, CommonViewItemData *item_data);
void app_common_view_clear_all_item(bool clear_logo);
void app_common_view_focus_item(int index);
void app_common_view_unfocus_item(int index);
void app_common_view_focus_item_change(int index);
void app_common_view_update_item_logo(int index, const char *logo_path);
void app_common_view_update_arrow_pic(bool up_show, bool down_show);
void app_common_view_update_page_str(int page_sel, int page_total);
void app_common_view_update_page_info(int page_sel, int page_total);
void app_common_view_update_page_item_all(int item_num, CommonViewItemData *item_data);
int app_common_view_draw_gif(void *usrdata);
void app_common_view_load_gif(void);
void app_common_view_free_gif(void);
void app_common_view_show_gif(void);
void app_common_view_hide_gif (void);
bool app_common_view_popup_state_get(void);
void app_common_view_hide_popup_msg(void);
void app_common_view_show_popup_msg(const char *str, int time_out);
void app_common_view_item_list_update(int item_total, char **item_logo, char **item_str, bool item_num_flag);
void app_common_view_group_list_update(int group_total, char **group_str);
CommonViewFocusArea app_common_view_focus_area_get(void);
void app_common_view_focus_area_change(CommonViewFocusArea area);
void app_common_view_change_tips_key(int key, bool enable, const char *tip, void (*func)(void));
void app_common_view_logo_download_stop(void);
void app_common_view_logo_download_start(void);
int app_common_view_keypress_func(GuiWidget *widget, void *usrdata);
int app_common_view_group_list_get_total_func(GuiWidget *widget, void *usrdata);
int app_common_view_group_list_get_data_func(GuiWidget *widget, void *usrdata);
int app_common_view_group_list_change_func(GuiWidget *widget, void *usrdata);
int app_common_view_group_list_keypress_func(GuiWidget *widget, void *usrdata);
int app_common_view_item_list_get_total_func(GuiWidget *widget, void *usrdata);
int app_common_view_item_list_get_data_func(GuiWidget *widget, void *usrdata);
int app_common_view_item_list_change_func(GuiWidget *widget, void *usrdata);
int app_common_view_item_list_keypress_func(GuiWidget *widget, void *usrdata);
void app_common_view_change_item_title(const char *str);
void app_common_view_change_show_type(void);
void app_common_view_all_tips_hide_show(void);
int app_common_view_create_dialog(CommonViewParas *paras);

// wnd_common_text
void app_common_text_set_title(const char *str);
void app_common_text_set_content(const char *str); // str不能立即释放，notepad窗口关闭才能释放

#endif

