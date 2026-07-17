#include "app.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "gx_apps.h"
#include "app_common_view.h"
#include "app_netapps_common.h"

#define LIMIT_PIC_SIZE
#ifdef LIMIT_PIC_SIZE
#if MINI_MEM_SUPPORT
#define MAX_PIC_SIZE (512*1024)
#else
#define MAX_PIC_SIZE (1024*1024)
#endif
#endif
#define GIF_PATH                    WORK_PATH"theme/image/netapps/loading.gif"
#define FAIL_PIC_PATH               WORK_PATH"theme/image/netapps/fail.bmp"
#define DEFAULT_PIC_PATH            WORK_PATH"theme/image/netapps/default.png"
#ifdef ECOS_OS
#define PIC_PATH_PREFIX             "/mnt/netapps_"
#else
#define PIC_PATH_PREFIX             "/tmp/netapps/nv_"
#endif
#define PIC_PATH_SUFFIX             ".jpg"
#define PIC_PATH_MAX_LEN            32
#define ITEM_FOCUS_COLOR            "[net_video_focus_color,net_video_focus_color,net_video_focus_color]"
#define ITEM_UNFOCUS_COLOR          "[window_bg_color,window_bg_color,window_bg_color]"
#define LOGO_IMG_KEY  "app_common_logo_key"
#define COMMON_VIWE_CONFIG_PATH	"netapps>applets_path"
#define COMMON_VIWE_PATH_LEN 256
#ifdef ECOS_OS
#define COMMON_VIWE_PATH_DEFALT "/mnt/install"
#else
#define COMMON_VIWE_PATH_DEFALT "/tmp/install"
#endif


CommonViewControl g_CommonView = {
    .show_type = COMMON_VIEW_TABLE_ITEM_TYPE,
    .focus_area = COMMON_VIEW_AREA_LIST_GROUP,
    .net_video_flag = false,
    .first_in_flag = true,
    .popup_show_flag = false,

    .widget = {
        .wnd = WND_COMMON_VIEW,
        .img_title = "img_common_view_title",
        .text_title = "text_common_view_title",
        .item_title = "text_common_view_item_title",
        .sys_date = "text_common_view_date",
        .sys_time = "text_common_view_time",
        .list_group = "listview_common_view_group",
        .list_item = "listview_common_view_item",
        .item_page = "btn_common_view_item_page",
        .img_up = "img_common_view_arrow_up",
        .img_down = "img_common_view_arrow_down",
        .img_gif = "img_common_view_gif",
        .img_popup = "img_common_view_popup",
        .text_popup = "text_common_view_popup",
        .base_item = {
            .bg = "img_common_view_item_bg",
            .logo = "img_common_view_item_logo",
            .title = "text_common_view_item_title",
            .author = "text_common_view_item_author",
            .duration = "text_common_view_item_duration",
            .viewcnt = "text_common_view_item_viewcnt",
            .line = "img_common_view_item_line"
        },
        .tips = {
            .img_exit = "img_common_view_exit_tip",
            .text_exit = "text_common_view_exit_tip",
            .img_menu = "img_common_view_menu_tip",
            .text_menu = "text_common_view_menu_tip",
            .img_ok = "img_common_view_ok_tip",
            .text_ok = "text_common_view_ok_tip",
            .img_move = "img_common_view_move_tip",
            .text_move = "text_common_view_move_tip",

            .img_red = "img_common_view_red_tip",
            .text_red = "text_common_view_red_tip",
            .img_green = "img_common_view_green_tip",
            .text_green = "text_common_view_green_tip",
            .img_yellow = "img_common_view_yellow_tip",
            .text_yellow = "text_common_view_yellow_tip",
            .img_blue = "img_common_view_blue_tip",
            .text_blue = "text_common_view_blue_tip",
        }
    },

    .signal = {NULL},
    .keyfunc = {NULL},
    .timer = {NULL},
    .ctrl_data = {0}
};

static inline const char *_get_widget(const char *item_base, int index)
{
#define COMMON_VIEW_WIDGET_LEN      64
    static char s_widget[COMMON_VIEW_WIDGET_LEN];
    sprintf(s_widget, "%s%d", item_base, index+1);
    return s_widget;
}

void app_common_view_hide_one_item(int index)
{
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;

    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.bg, index), "state", "hide");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "state", "hide");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index), "state", "hide");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.author, index), "state", "hide");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.duration, index), "state", "hide");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.viewcnt, index), "state", "hide");
}

void app_common_view_show_one_item(int index)
{
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;

    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.bg, index), "state", "show");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "state", "show");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index), "state", "show");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.author, index), "state", "show");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.duration, index), "state", "show");
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.viewcnt, index), "state", "show");
}

void app_common_view_clear_one_item(int index, bool clear_logo)
{
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;

    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index), "string", STR_ID_BLANK);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.author, index), "string", STR_ID_BLANK);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.duration, index), "string", STR_ID_BLANK);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.viewcnt, index), "string", STR_ID_BLANK);
    if (clear_logo)
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "load_scal_img", NULL);
    else
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "load_scal_img", DEFAULT_PIC_PATH);
}

void app_common_view_display_one_item(int index, CommonViewItemData *item_data)
{
    char *str = NULL;

    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0 || item_data == NULL)
        return;

    str = (item_data->title != NULL) ? item_data->title : STR_ID_BLANK;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index), "string", str);
    str = (item_data->author != NULL) ? item_data->author : STR_ID_BLANK;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.author, index), "string", str);
    str = (item_data->duration != NULL) ? item_data->duration : STR_ID_BLANK;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.duration, index), "string", str);
    str = (item_data->viewcnt != NULL) ? item_data->viewcnt : STR_ID_BLANK;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.viewcnt, index), "string", str);
    str = (GXCORE_FILE_EXIST != GxCore_FileExists(item_data->logo)) ? DEFAULT_PIC_PATH : item_data->logo;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "load_scal_img", str);
}

void app_common_view_clear_all_item(bool clear_logo)
{
    int i = 0;
    for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
    {
        app_common_view_clear_one_item(i, clear_logo);
    }
    GUI_SetProperty(g_CommonView.widget.wnd, "update", NULL);
}

void app_common_view_focus_item(int index)
{
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.bg, index), "backcolor", ITEM_FOCUS_COLOR);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index),"update",NULL);
    int position=0;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index),"format","fix_r2l");;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"rolling_position",&position);;
}

void app_common_view_unfocus_item(int index)
{
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.bg, index), "backcolor", ITEM_UNFOCUS_COLOR);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index),"update",NULL);
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title, index),"format","static");;
}

void app_common_view_focus_item_change(int index)
{
    if (index >= 0 && index < COMMON_VIEW_MAX_PAGE_ITEM && index != g_CommonView.ctrl_data.page_item_sel)
    {
        app_common_view_unfocus_item(g_CommonView.ctrl_data.page_item_sel);
        g_CommonView.ctrl_data.page_item_sel = index;
        app_common_view_focus_item(g_CommonView.ctrl_data.page_item_sel);
    }
}

void app_common_view_update_item_logo(int index, const char *logo_path)
{
    if (GUI_CheckDialog(WND_POP_BOOK) == GXCORE_SUCCESS)
        return;
    if (index >= COMMON_VIEW_MAX_PAGE_ITEM || index < 0)
        return;
    if (logo_path)
    {
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.bg, index), "update", NULL);
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.logo, index), "load_scal_img", (void*)logo_path);
    }
}

void app_common_view_update_arrow_pic(bool up_show, bool down_show)
{
    if (up_show)
        GUI_SetProperty(g_CommonView.widget.img_up, "state", "show");
    else
        GUI_SetProperty(g_CommonView.widget.img_up, "state", "hide");
    if (down_show)
        GUI_SetProperty(g_CommonView.widget.img_down, "state", "show");
    else
        GUI_SetProperty(g_CommonView.widget.img_down, "state", "hide");
}

void app_common_view_update_page_str(int page_sel, int page_total)
{
    char buf[64] = {0};
    if (page_total > 0)
    {
        snprintf(buf, sizeof(buf), "[ %d / %d ]", page_sel + 1, page_total);
        GUI_SetProperty(g_CommonView.widget.item_page, "string", buf);
    }
    else
    {
        GUI_SetProperty(g_CommonView.widget.item_page, "string", STR_ID_BLANK);
    }
}

void app_common_view_update_page_info(int page_sel, int page_total)
{
    g_CommonView.ctrl_data.page_total = page_total;
    g_CommonView.ctrl_data.page_sel = page_sel;
    app_common_view_update_page_str(page_sel, page_total);
    if (strcmp(WND_COMMON_VIEW, GUI_GetFocusWindow()) == 0)
    {
         if (g_CommonView.ctrl_data.page_total <= 1)
             app_common_view_update_arrow_pic(false, false);
         else if (g_CommonView.ctrl_data.page_sel >= g_CommonView.ctrl_data.page_total - 1) //last page
             app_common_view_update_arrow_pic(true, false);
         else if (g_CommonView.ctrl_data.page_sel == 0)//first page
             app_common_view_update_arrow_pic(false, true);
         else
             app_common_view_update_arrow_pic(true, true);
    }
}

void app_common_view_update_page_item_all(int item_num, CommonViewItemData *item_data)
{
    int i = 0;
    int cur_sel = 0;

    if (item_num < 0 || item_num > COMMON_VIEW_MAX_PAGE_ITEM)
        return;
    g_CommonView.ctrl_data.page_item_num = item_num;
    if (g_CommonView.ctrl_data.page_item_data != item_data)
    {
        for (i = 0; i < g_CommonView.ctrl_data.page_item_num; i++)
            memcpy(g_CommonView.ctrl_data.page_item_data + i, item_data + i, sizeof(CommonViewItemData));
        for ( ; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
            memset(g_CommonView.ctrl_data.page_item_data + i, 0, sizeof(CommonViewItemData));
    }

    cur_sel = g_CommonView.ctrl_data.page_item_sel;
    app_common_view_unfocus_item(cur_sel);
    for (i = 0; i < g_CommonView.ctrl_data.page_item_num; i++)
        app_common_view_display_one_item(i, item_data + i);
    for ( ; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
        app_common_view_clear_one_item(i, true);
    if (cur_sel >= g_CommonView.ctrl_data.page_item_num)
    {
        cur_sel = g_CommonView.ctrl_data.page_item_num - 1;
    }
    if (g_CommonView.ctrl_data.page_item_num >0 && cur_sel < 0)
    {
        cur_sel = 0;
    }
    if (g_CommonView.focus_area == COMMON_VIEW_AREA_TABLE_ITEM)
    {
        app_common_view_focus_item(cur_sel);
    }
    if (cur_sel != g_CommonView.ctrl_data.page_item_sel)
    {
        g_CommonView.ctrl_data.page_item_sel = cur_sel;
        if (g_CommonView.keyfunc.page_item_sel_change)
            g_CommonView.keyfunc.page_item_sel_change(g_CommonView.ctrl_data.page_item_sel);
    }
}

static int app_common_view_clock_update(void *userdata)
{
    const char *wnd = NULL;
    wnd = GUI_GetFocusWindow();
    if (wnd && (strcmp(wnd, WND_COMMON_VIEW) == 0 || 0 == strcmp(wnd, WND_COMMON_TEXT)))
        app_show_sys_time_and_date(g_CommonView.widget.sys_time, g_CommonView.widget.sys_date);
    return 0;
}

static void app_common_view_clock_timer_rm(void)
{
    APP_TIMER_REMOVE(g_CommonView.timer.clock);
}

static void app_common_view_clock_timer_add(void)
{
    app_common_view_clock_update(NULL);
    APP_TIMER_ADD(g_CommonView.timer.clock, app_common_view_clock_update, 1000, TIMER_REPEAT);
}

int app_common_view_draw_gif(void *usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;
    GUI_SetProperty(g_CommonView.widget.img_gif, "draw_gif", &alu);
    return 0;
}

void app_common_view_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;
    GUI_SetProperty(g_CommonView.widget.img_gif, "load_img", GIF_PATH);
    GUI_SetProperty(g_CommonView.widget.img_gif, "init_gif_alu_mode", &alu);
}

void app_common_view_free_gif(void)
{
    GUI_SetProperty(g_CommonView.widget.img_gif, "load_img", NULL);
}

void app_common_view_show_gif(void)
{
    GUI_SetProperty(g_CommonView.widget.img_gif, "state", "show");
    if (g_CommonView.timer.gif == NULL)
        g_CommonView.timer.gif = create_timer(app_common_view_draw_gif, 100, 0, TIMER_REPEAT);
}

void app_common_view_hide_gif(void)
{
    APP_TIMER_REMOVE(g_CommonView.timer.gif);
    GUI_SetProperty(g_CommonView.widget.img_gif, "state", "hide");
}

bool app_common_view_popup_state_get(void)
{
    return g_CommonView.popup_show_flag;
}

void app_common_view_hide_popup_msg(void)
{
    APP_TIMER_REMOVE(g_CommonView.timer.popup);

    g_CommonView.popup_show_flag = false;
    GUI_SetProperty(g_CommonView.widget.text_popup, "state", "hide");
    GUI_SetProperty(g_CommonView.widget.img_popup, "state", "hide");
    int position=0;
    if(g_CommonView.focus_area == COMMON_VIEW_AREA_TABLE_ITEM)
    {
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"format","fix_r2l");
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"rolling_position",&position);
    }
    GUI_SetProperty(WND_COMMON_VIEW, "update", NULL);
}

static int app_common_view_popup_timer_timeout(void *userdata)
{
    g_CommonView.timer.popup = NULL;
    app_common_view_hide_popup_msg();
    return 0;
}

void app_common_view_show_popup_msg(const char *str, int time_out)
{
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"format","static");;
    GUI_SetProperty(g_CommonView.widget.img_popup, "state", "hide");
    GUI_SetProperty(g_CommonView.widget.text_popup, "state", "hide");
    GUI_SetInterface("flush", NULL);
    g_CommonView.popup_show_flag = true;
    GUI_SetProperty(g_CommonView.widget.text_popup, "string", (void*)str);
    GUI_SetProperty(g_CommonView.widget.img_popup, "state", "show");
    GUI_SetProperty(g_CommonView.widget.text_popup, "state", "show");
    app_update_pop_dlg(WND_POP_BOOK);
    app_update_pop_dlg(WND_POP_TIP);

    APP_TIMER_REMOVE(g_CommonView.timer.popup);
    if (time_out > 0)
        g_CommonView.timer.popup = create_timer(app_common_view_popup_timer_timeout, time_out, 0, TIMER_ONCE);
}

void app_common_view_item_list_update(int item_total, char **item_logo, char **item_str, bool item_num_flag)
{
    int sel = 0;

    g_CommonView.ctrl_data.item_total = item_total;
    g_CommonView.ctrl_data.item_logo = item_logo;
    g_CommonView.ctrl_data.item_str = item_str;
    g_CommonView.ctrl_data.item_num_flag = item_num_flag;

    GUI_SetProperty(g_CommonView.widget.list_item, "update_all", NULL);
    GUI_SetProperty(g_CommonView.widget.list_item, "select", &sel);
}

void app_common_view_group_list_update(int group_total, char **group_str)
{
    int sel = 0;

    g_CommonView.ctrl_data.group_total = group_total;
    g_CommonView.ctrl_data.group_str = group_str;
    GUI_SetProperty(g_CommonView.widget.list_group, "update_all", NULL);
    if (g_CommonView.first_in_flag)
    {
        g_CommonView.first_in_flag = false;
        sel = (g_CommonView.ctrl_data.group_total <= 1) ? 0 : g_CommonView.ctrl_data.orig_group_sel;
        GUI_SetProperty(g_CommonView.widget.list_group, "select", &sel);
        GUI_SetProperty(g_CommonView.widget.list_group, "active", &sel);
    }
}

CommonViewFocusArea app_common_view_focus_area_get(void)
{
    return g_CommonView.focus_area;
}

void app_common_view_focus_area_change(CommonViewFocusArea area)
{
    if (area >= COMMON_VIEW_AREA_TOTAL)
        return;
    if (area == g_CommonView.focus_area)
        return;

    if (g_CommonView.show_type == COMMON_VIEW_TABLE_ITEM_TYPE)
    {
        if (area == COMMON_VIEW_AREA_LIST_GROUP)
        {
            app_common_view_unfocus_item(g_CommonView.ctrl_data.page_item_sel);
            g_CommonView.focus_area = COMMON_VIEW_AREA_LIST_GROUP;
            GUI_SetFocusWidget(g_CommonView.widget.list_group);
            if (g_CommonView.keyfunc.focus_area_change)
                g_CommonView.keyfunc.focus_area_change(g_CommonView.focus_area);
        }
        else
        {
            if (g_CommonView.ctrl_data.page_item_num > 0)
            {
                g_CommonView.focus_area = COMMON_VIEW_AREA_TABLE_ITEM;
                GUI_SetFocusWidget(g_CommonView.widget.item_page);
                app_common_view_focus_item(g_CommonView.ctrl_data.page_item_sel);
                if (g_CommonView.keyfunc.focus_area_change)
                    g_CommonView.keyfunc.focus_area_change(g_CommonView.focus_area);
                if (g_CommonView.keyfunc.page_item_sel_change)
                    g_CommonView.keyfunc.page_item_sel_change(g_CommonView.ctrl_data.page_item_sel);
            }
        }
    }
    else
    {
        if (area == COMMON_VIEW_AREA_LIST_GROUP)
        {
            g_CommonView.focus_area = COMMON_VIEW_AREA_LIST_GROUP;
            GUI_SetFocusWidget(g_CommonView.widget.list_group);
            if (g_CommonView.keyfunc.focus_area_change)
                g_CommonView.keyfunc.focus_area_change(g_CommonView.focus_area);
        }
        else
        {
            if (g_CommonView.ctrl_data.item_total > 0)
            {
                g_CommonView.focus_area = COMMON_VIEW_AREA_LIST_ITEM;
                GUI_SetFocusWidget(g_CommonView.widget.list_item);
                if (g_CommonView.keyfunc.focus_area_change)
                    g_CommonView.keyfunc.focus_area_change(g_CommonView.focus_area);
            }
        }
    }
}

void app_common_view_change_tips_key(int key, bool enable, const char *tip, void (*func)(void))
{
    const char *img = NULL;
    const char *text = NULL;

    switch (key)
    {
        case STBK_MENU:
            g_CommonView.keyfunc.key_menu = func;
            g_CommonView.string.str_menu = tip;
            img = g_CommonView.widget.tips.img_menu;
            text = g_CommonView.widget.tips.text_menu;
            break;
        case STBK_OK:
            g_CommonView.keyfunc.key_ok = func;
            g_CommonView.string.str_ok = tip;
            img = g_CommonView.widget.tips.img_ok;
            text = g_CommonView.widget.tips.text_ok;
            break;
        case STBK_MUTE:
            g_CommonView.keyfunc.key_mute = func;
            return;
        case STBK_RED:
            g_CommonView.keyfunc.key_red = func;
            g_CommonView.string.str_red = tip;
            img = g_CommonView.widget.tips.img_red;
            text = g_CommonView.widget.tips.text_red;
            break;
        case STBK_GREEN:
            g_CommonView.keyfunc.key_green = func;
            g_CommonView.string.str_green = tip;
            img = g_CommonView.widget.tips.img_green;
            text = g_CommonView.widget.tips.text_green;
            break;
        case STBK_YELLOW:
            g_CommonView.keyfunc.key_yellow = func;
            g_CommonView.string.str_yellow = tip;
            img = g_CommonView.widget.tips.img_yellow;
            text = g_CommonView.widget.tips.text_yellow;
            break;
        case STBK_BLUE:
            g_CommonView.keyfunc.key_blue = func;
            g_CommonView.string.str_blue = tip;
            img = g_CommonView.widget.tips.img_blue;
            text = g_CommonView.widget.tips.text_blue;
            break;
        default:
            break;
    }

    if (enable)
    {
        GUI_SetProperty(text, "string", (void*)tip);
        GUI_SetProperty(text, "state", "show");
        GUI_SetProperty(img, "state", "show");
    }
    else
    {
        GUI_SetProperty(text, "state", "hide");
        GUI_SetProperty(img, "state", "hide");
    }
}


static int app_common_view_logo_timer_timeout(void *userdata)
{
    int i = 0;
    bool all_ok = true;
    char image_path[PIC_PATH_MAX_LEN];

    if (g_CommonView.ctrl_data.page_item_num <= 0 || g_CommonView.ctrl_data.page_item_data == NULL || strcmp(WND_COMMON_VIEW, GUI_GetFocusWindow()) != 0)
    {
        return 0;
    }

    for (i = 0; i < g_CommonView.ctrl_data.page_item_num; i++)
    {
        if (g_CommonView.ctrl_data.pic_show_ok[i] == 1)
            continue;

        all_ok = false;
        if (g_CommonView.ctrl_data.pic_download_ok[i] == 1)
        {
            snprintf(image_path, PIC_PATH_MAX_LEN, "%s%d%s", PIC_PATH_PREFIX, i, PIC_PATH_SUFFIX);
            if (GXCORE_FILE_UNEXIST != GxCore_FileExists(image_path))
            {
                app_common_view_update_item_logo(i, image_path);
            }
            else
            {
                if (!app_common_view_popup_state_get())
                {
                    app_common_view_update_item_logo(i, FAIL_PIC_PATH);
                }
            }
            g_CommonView.ctrl_data.pic_show_ok[i] = 1;
        }
        else if (g_CommonView.ctrl_data.pic_download_ok[i] == -1)
        {
            if (g_CommonView.ctrl_data.pic_download_handle[i] != 0)
            {
                gxapps_curl_async_abort(g_CommonView.ctrl_data.pic_download_handle[i]);
                g_CommonView.ctrl_data.pic_download_handle[i] = 0;
            }
            if (!app_common_view_popup_state_get())
            {
                app_common_view_update_item_logo(i, FAIL_PIC_PATH);
            }
            g_CommonView.ctrl_data.pic_show_ok[i] = 1;
        }
    }

    if (all_ok)
    {
        app_common_view_hide_gif();
        GUI_SetInterface("flush",NULL);
        timer_stop(g_CommonView.timer.logoup);
    }

    return 0;
}

static void app_common_view_logo_timer_rm(void)
{
    APP_TIMER_REMOVE(g_CommonView.timer.logoup);
}

static void app_common_view_logo_timer_add(void)
{
    APP_TIMER_ADD(g_CommonView.timer.logoup, app_common_view_logo_timer_timeout, 300, TIMER_REPEAT);
}

static void app_common_view_logo_download_delete(void)
{
    int i=0;
    char image_path[PIC_PATH_MAX_LEN];

    for (i=0; i<COMMON_VIEW_MAX_PAGE_ITEM; i++)
    {
        snprintf(image_path, PIC_PATH_MAX_LEN, "%s%d%s", PIC_PATH_PREFIX, i, PIC_PATH_SUFFIX);
        if (GXCORE_FILE_EXIST == GxCore_FileExists(image_path))
            GxCore_FileDelete(image_path);
    }
}

static void app_common_view_logo_download_cb(int message, int body)
{
    int i = 0;
    unsigned int handle = 0;
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

    if (g_CommonView.ctrl_data.pic_download_abort)
        return;

    if (message == GXCURL_STATE_COMPLETE)
    {
        handle = callback_data->handle;
        if (handle > 0)
        {
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
            {
                if (g_CommonView.ctrl_data.pic_download_handle[i] == handle)
                {
                    if (callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                        g_CommonView.ctrl_data.pic_download_ok[i] = 1;
                    else
                        g_CommonView.ctrl_data.pic_download_ok[i] = -1;
                    break;
                }
            }
        }
    }
#ifdef LIMIT_PIC_SIZE
    else if (message == GXCURL_STATE_PROCESS)
    {
        handle = callback_data->handle;
        if (handle > 0 && callback_data->process_data.content_length > MAX_PIC_SIZE)
        {
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
            {
                if (g_CommonView.ctrl_data.pic_download_handle[i] == handle)
                {
                    g_CommonView.ctrl_data.pic_download_ok[i] = -1;
                    break;
                }
            }
        }
    }
#endif
}

void app_common_view_logo_download_stop(void)
{
    int i = 0;

    app_common_view_logo_timer_rm();
    memset(&g_CommonView.ctrl_data.pic_download_ok, 0, sizeof(g_CommonView.ctrl_data.pic_download_ok));
    memset(&g_CommonView.ctrl_data.pic_show_ok, 0, sizeof(g_CommonView.ctrl_data.pic_show_ok));
    g_CommonView.ctrl_data.pic_download_abort = 1;
    for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
    {
        if (g_CommonView.ctrl_data.pic_download_handle[i] != 0)
        {
            gxapps_curl_async_abort(g_CommonView.ctrl_data.pic_download_handle[i]);
            g_CommonView.ctrl_data.pic_download_handle[i] = 0;
        }
    }
    app_common_view_logo_download_delete();
    g_CommonView.ctrl_data.pic_download_abort = 0;
}

void app_common_view_logo_download_start(void)
{
    int i = 0;
    char image_path[PIC_PATH_MAX_LEN];

    app_common_view_logo_download_stop();
    app_common_view_logo_timer_add();
    if (g_CommonView.ctrl_data.page_item_num > 0)
    {
        for (i = 0; i < g_CommonView.ctrl_data.page_item_num; i++)
        {
            if (g_CommonView.ctrl_data.pic_download_abort == 0)
            {
                if (g_CommonView.ctrl_data.page_item_data[i].logo)
                {
                    snprintf(image_path, PIC_PATH_MAX_LEN, "%s%d%s", PIC_PATH_PREFIX, i, PIC_PATH_SUFFIX);
                    g_CommonView.ctrl_data.pic_download_handle[i] = gxapps_curl_async_download(
                            g_CommonView.ctrl_data.page_item_data[i].logo, image_path, app_common_view_logo_download_cb);
                }
                else
                {
                    g_CommonView.ctrl_data.pic_download_ok[i] = -1;
                    g_CommonView.ctrl_data.pic_show_ok[i] = 1;
                }
            }
        }
    }
}

static int _item_table_keypress_proc(int key_val)
{
#define _CHANGE_PAGE_SEL(new_sel) do {                                                  \
    g_CommonView.ctrl_data.page_sel = new_sel;                                          \
    if (g_CommonView.keyfunc.page_sel_change)                                           \
        g_CommonView.keyfunc.page_sel_change(g_CommonView.ctrl_data.page_sel);          \
    app_common_view_update_page_info(g_CommonView.ctrl_data.page_sel,                   \
            g_CommonView.ctrl_data.page_total);                                         \
} while(0)

#define _CHANGE_PAGE_ITEM_SEL(new_sel) do {                                             \
    app_common_view_unfocus_item(g_CommonView.ctrl_data.page_item_sel);                 \
    g_CommonView.ctrl_data.page_item_sel = new_sel;                                     \
    app_common_view_focus_item(g_CommonView.ctrl_data.page_item_sel);                   \
    if (g_CommonView.keyfunc.page_item_sel_change)                                      \
        g_CommonView.keyfunc.page_item_sel_change(g_CommonView.ctrl_data.page_item_sel);\
} while(0)

    if (g_CommonView.focus_area != COMMON_VIEW_AREA_TABLE_ITEM)
        return EVENT_TRANSFER_STOP;

    switch (key_val)
    {
        case STBK_OK:
            if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
            {
                app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
            }
            else
            {
                if (g_CommonView.keyfunc.key_ok)
                    g_CommonView.keyfunc.key_ok();
            }
            break;

        case STBK_LEFT:
            if ((g_CommonView.ctrl_data.page_item_sel % COMMON_VIEW_MAX_COL_ITEM) == 0)
            {
                app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_GROUP);
            }
            else if (g_CommonView.ctrl_data.page_item_sel > 0)
            {
                _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel - 1);
            }
            break;
        case STBK_RIGHT:
            if (COMMON_VIEW_MAX_COL_ITEM == 1)
            {
                app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_GROUP);
                break;
            }
            if ((g_CommonView.ctrl_data.page_item_sel == g_CommonView.ctrl_data.page_item_num - 1)
              ||(g_CommonView.ctrl_data.page_item_num == 0))
            {
                if (g_CommonView.ctrl_data.page_sel < g_CommonView.ctrl_data.page_total - 1)
                {
                    if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
                    {
                        app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                        break;
                    }
                    _CHANGE_PAGE_ITEM_SEL(0);
                    _CHANGE_PAGE_SEL(g_CommonView.ctrl_data.page_sel + 1);
                }
            }
            else if (g_CommonView.ctrl_data.page_item_sel >= 0)
            {
                _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel + 1);
            }
            break;
        case STBK_UP:
            if (g_CommonView.ctrl_data.page_item_sel < COMMON_VIEW_MAX_COL_ITEM)
            {
                if (g_CommonView.ctrl_data.page_sel > 0)
                {
                    if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
                    {
                        app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                        break;
                    }
                    _CHANGE_PAGE_ITEM_SEL(COMMON_VIEW_MAX_PAGE_ITEM - COMMON_VIEW_MAX_COL_ITEM + g_CommonView.ctrl_data.page_item_sel);
                    _CHANGE_PAGE_SEL(g_CommonView.ctrl_data.page_sel - 1);
                }
            }
            else if (g_CommonView.ctrl_data.page_item_sel >= COMMON_VIEW_MAX_COL_ITEM)
            {
                _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel - COMMON_VIEW_MAX_COL_ITEM);
            }
            break;

        case STBK_DOWN:
            if (g_CommonView.ctrl_data.page_sel < g_CommonView.ctrl_data.page_total - 1)
            {
                if(g_CommonView.ctrl_data.page_item_sel > COMMON_VIEW_MAX_COL_ITEM* (COMMON_VIEW_MAX_ROW_ITEM-1)-1)
                {
                    if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
                    {
                        app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                        break;
                    }
                    _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel - COMMON_VIEW_MAX_COL_ITEM*(COMMON_VIEW_MAX_ROW_ITEM-1));
                    _CHANGE_PAGE_SEL(g_CommonView.ctrl_data.page_sel + 1);
                }
                else
                {
                    if((g_CommonView.ctrl_data.page_item_sel + COMMON_VIEW_MAX_COL_ITEM) < g_CommonView.ctrl_data.page_item_num)
                    {
                        _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel +COMMON_VIEW_MAX_COL_ITEM);
                    }
                    else
                    {
                        _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_num-1);
                    }
                }
            }
            else
            {
                if((g_CommonView.ctrl_data.page_item_sel + COMMON_VIEW_MAX_COL_ITEM) < g_CommonView.ctrl_data.page_item_num)
                {
                    _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_sel + COMMON_VIEW_MAX_COL_ITEM);
                }
                else
                {
                    if(g_CommonView.ctrl_data.page_item_sel != (g_CommonView.ctrl_data.page_item_num -1))
                        _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_num-1);
                }
            }
            break;
        case STBK_PAGE_UP:
            if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
            {
                app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                break;
            }
            if (g_CommonView.ctrl_data.page_sel > 0)
            {
                _CHANGE_PAGE_SEL(g_CommonView.ctrl_data.page_sel - 1);
            }
            else if (g_CommonView.ctrl_data.page_sel == 0)
            {
                if (g_CommonView.ctrl_data.page_item_sel > 0)
                {
                    _CHANGE_PAGE_ITEM_SEL(0);
                }
            }
            break;

        case STBK_PAGE_DOWN:
            if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
            {
                app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                break;
            }
            if (g_CommonView.ctrl_data.page_sel < g_CommonView.ctrl_data.page_total - 1)
            {
                _CHANGE_PAGE_SEL(g_CommonView.ctrl_data.page_sel + 1);
            }
            else if (g_CommonView.ctrl_data.page_sel == g_CommonView.ctrl_data.page_total - 1)
            {
                if (g_CommonView.ctrl_data.page_item_sel < g_CommonView.ctrl_data.page_item_num - 1)
                {
                    _CHANGE_PAGE_ITEM_SEL(g_CommonView.ctrl_data.page_item_num - 1);
                }
            }
            break;

        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

int app_common_view_keypress_func(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    int depth = 0;
    int key_val = 0;

    event = (GUI_Event *)usrdata;
    if (GUI_KEYDOWN == event->type)
    {
        key_val = find_virtualkey_ex(event->key.scancode,event->key.sym);
        if (app_common_view_popup_state_get())
        {
            switch(key_val)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                    app_common_view_hide_popup_msg();
                    break;
                default:
                    break;
            }
        }
        else
        {
            switch(key_val)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                    if (g_CommonView.keyfunc.get_item_depth)
                    {
                        depth = g_CommonView.keyfunc.get_item_depth();
                    }
                    if (depth > 0 && g_CommonView.keyfunc.exit_item_depth)
                    {
                        g_CommonView.keyfunc.exit_item_depth();
                    }
                    else
                    {
                        GUI_EndDialog(g_CommonView.widget.wnd);
                    }
                    break;

                case STBK_MENU:
                    if (g_CommonView.keyfunc.key_menu != NULL)
                    {
                        g_CommonView.first_in_flag = true;
                        g_CommonView.keyfunc.key_menu();
                    }
                    break;
                case STBK_MUTE:
                    if (g_CommonView.keyfunc.key_mute)
                        g_CommonView.keyfunc.key_mute();
                    break;
                case STBK_RED:
                    if (g_CommonView.keyfunc.key_red)
                        g_CommonView.keyfunc.key_red();
                    break;
                case STBK_GREEN:
                    if (g_CommonView.keyfunc.key_green)
                        g_CommonView.keyfunc.key_green();
                    break;
                case STBK_YELLOW:
                    if (g_CommonView.keyfunc.key_yellow)
                        g_CommonView.keyfunc.key_yellow();
                    break;
                case STBK_BLUE:
                    if (g_CommonView.keyfunc.key_blue)
                        g_CommonView.keyfunc.key_blue();
                    break;

                case STBK_OK:
                case STBK_LEFT:
                case STBK_RIGHT:
                case STBK_UP:
                case STBK_DOWN:
                case STBK_PAGE_UP:
                case STBK_PAGE_DOWN:
                    if (g_CommonView.focus_area == COMMON_VIEW_AREA_TABLE_ITEM)
                        _item_table_keypress_proc(key_val);
                    break;

                default:
                    break;
            }
        }
    }
    return EVENT_TRANSFER_STOP;
}

int app_common_view_group_list_get_total_func(GuiWidget *widget, void *usrdata)
{
    return g_CommonView.ctrl_data.group_total;
}

int app_common_view_group_list_get_data_func(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;

    item = (ListItemPara *)usrdata;
    if (NULL == item || 0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if (g_CommonView.ctrl_data.group_str == NULL)
        return EVENT_TRANSFER_STOP;

    //col-0: channel name
    item->image = NULL;
    item->string = g_CommonView.ctrl_data.group_str[item->sel];

    return EVENT_TRANSFER_STOP;
}

int app_common_view_group_list_change_func(GuiWidget *widget, void *usrdata)
{
    int sel = -1;

    GUI_GetProperty(g_CommonView.widget.list_group, "select", &sel);
    g_CommonView.ctrl_data.group_sel = sel;
    if (g_CommonView.keyfunc.group_sel_change)
        g_CommonView.keyfunc.group_sel_change(g_CommonView.ctrl_data.group_sel);
    return EVENT_TRANSFER_STOP;
}

static int _group_list_keypress_proc(int key_val)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int sel = 0;

    switch (key_val)
    {
        case STBK_LEFT:
        case STBK_RIGHT:
            if (!app_common_view_popup_state_get())
            {
                if (g_CommonView.show_type == COMMON_VIEW_TABLE_ITEM_TYPE)
                {
                    g_CommonView.ctrl_data.page_item_sel = 0;
                    app_common_view_focus_area_change(COMMON_VIEW_AREA_TABLE_ITEM);
                }
                else
                    app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_ITEM);
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_OK:
            if (g_CommonView.ctrl_data.group_total > 0 && g_CommonView.keyfunc.key_ok)
            {
                if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
                {
                    app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                }
                else
                {
                    g_CommonView.keyfunc.key_ok();
                    GUI_SetProperty(g_CommonView.widget.list_group, "active", &g_CommonView.ctrl_data.group_sel);
                }
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_PAGE_UP:
            GUI_GetProperty(g_CommonView.widget.list_group, "select", &sel);
            if (sel == 0)
            {
                sel = app_common_view_group_list_get_total_func(NULL, NULL) - 1;
                GUI_SetProperty(g_CommonView.widget.list_group, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        case STBK_PAGE_DOWN:
            sel = 0;
            GUI_GetProperty(g_CommonView.widget.list_group, "select", &sel);
            if (sel+1 == app_common_view_group_list_get_total_func(NULL, NULL))
            {
                sel = 0;
                GUI_SetProperty(g_CommonView.widget.list_group, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        default:
            break;
    }
    return ret;
}

int app_common_view_group_list_keypress_func(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if (GUI_KEYDOWN == event->type)
    {
        if (g_CommonView.focus_area == COMMON_VIEW_AREA_LIST_GROUP)
        {
            int key_val = find_virtualkey_ex(event->key.scancode, event->key.sym);
            ret =  _group_list_keypress_proc(key_val);
        }
        else
        {
            GUI_SendEvent(g_CommonView.widget.wnd, event); // proc by app_netvideo_keypress
            ret = EVENT_TRANSFER_STOP;
        }
    }
    return ret;
}

int app_common_view_item_list_get_total_func(GuiWidget *widget, void *usrdata)
{
    return g_CommonView.ctrl_data.item_total;
}

int app_common_view_item_list_get_data_func(GuiWidget *widget, void *usrdata)
{
    ListItemPara *item = NULL;
    char num_str[16] = {0};

    item = (ListItemPara *)usrdata;
    if (NULL == item || 0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if (g_CommonView.ctrl_data.item_str == NULL)
        return EVENT_TRANSFER_STOP;

    if (g_CommonView.ctrl_data.item_num_flag)
    {
        snprintf(num_str, sizeof(num_str), "%d", item->sel+1);
        item->image = NULL;
        item->string = num_str;
    }
    else
    {
        item->image = g_CommonView.ctrl_data.item_logo[item->sel];
        item->string = NULL;
    }

    item = item->next;
    item->image = NULL;
    item->string = g_CommonView.ctrl_data.item_str[item->sel];

    return EVENT_TRANSFER_STOP;
}

int app_common_view_item_list_change_func(GuiWidget *widget, void *usrdata)
{
    int sel = -1;

    GUI_GetProperty(g_CommonView.widget.list_item, "select", &sel);
    g_CommonView.ctrl_data.item_sel = sel;
    if (g_CommonView.keyfunc.item_sel_change)
        g_CommonView.keyfunc.item_sel_change(g_CommonView.ctrl_data.item_sel);
    return EVENT_TRANSFER_STOP;
}

static int _item_list_keypress_proc(int key_val)
{
    int ret = EVENT_TRANSFER_KEEPON;
    int sel = 0;

    if(g_CommonView.popup_show_flag)
        return EVENT_TRANSFER_STOP;

    switch(key_val)
    {
        case STBK_LEFT:
        case STBK_RIGHT:
            if (!app_common_view_popup_state_get())
            {
                app_common_view_focus_area_change(COMMON_VIEW_AREA_LIST_GROUP);
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_OK:
            if (g_CommonView.ctrl_data.item_total > 0 && g_CommonView.keyfunc.key_ok)
            {
                if (g_CommonView.keyfunc.busy_check && g_CommonView.keyfunc.busy_check())
                {
                    app_common_view_show_popup_msg("System Busy, Please Wait!", 1000);
                }
                else
                {
                    g_CommonView.keyfunc.key_ok();
                    //GUI_SetProperty(g_CommonView.widget.list_item, "active", &g_CommonView.ctrl_data.item_sel);
                }
            }
            ret = EVENT_TRANSFER_STOP;
            break;

        case STBK_PAGE_UP:
            GUI_GetProperty(g_CommonView.widget.list_item, "select", &sel);
            if (sel == 0)
            {
                sel = app_common_view_item_list_get_total_func(NULL, NULL) - 1;
                GUI_SetProperty(g_CommonView.widget.list_item, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        case STBK_PAGE_DOWN:
            sel = 0;
            GUI_GetProperty(g_CommonView.widget.list_item, "select", &sel);
            if (sel+1 == app_common_view_item_list_get_total_func(NULL, NULL))
            {
                sel = 0;
                GUI_SetProperty(g_CommonView.widget.list_item, "select", &sel);
                ret = EVENT_TRANSFER_STOP;
            }
            break;

        default:
            break;
    }
    return ret;
}

int app_common_view_item_list_keypress_func(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if (GUI_KEYDOWN == event->type)
    {
        if (g_CommonView.focus_area == COMMON_VIEW_AREA_LIST_ITEM)
        {
            int key_val = find_virtualkey_ex(event->key.scancode, event->key.sym);
            ret = _item_list_keypress_proc(key_val);
        }
        else
        {
            GUI_SendEvent(g_CommonView.widget.wnd, event); // proc by app_netvideo_keypress
            ret = EVENT_TRANSFER_STOP;
        }
    }
    return ret;
}

void app_common_view_change_item_title(const char *str)
{
    g_CommonView.string.str_item_title = str;
    GUI_SetProperty(g_CommonView.widget.item_title, "string", (void*)g_CommonView.string.str_item_title);
}

void app_common_view_change_show_type(void)
{
    int i = 0;

    switch (g_CommonView.show_type) {
        case COMMON_VIEW_TABLE_ITEM_TYPE:
            GUI_SetProperty(g_CommonView.widget.item_title, "state", "hide");
            GUI_SetProperty(g_CommonView.widget.list_item, "state", "hide");
            GUI_SetProperty(g_CommonView.widget.item_page, "state", STR_ID_BLANK);
            GUI_SetProperty(g_CommonView.widget.item_page, "state", "show");
            GUI_SetProperty(g_CommonView.widget.img_up, "state", "hide");
            GUI_SetProperty(g_CommonView.widget.img_down, "state", "hide");
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM-1; i++)
            {
                GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.line, i), "state", "show");
            }
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
            {
                app_common_view_show_one_item(i);
                app_common_view_unfocus_item(i);
                app_common_view_clear_one_item(i, false);
            }
            break;
        case COMMON_VIEW_LIST_ITEM_TYPE:
            GUI_SetProperty(g_CommonView.widget.item_title, "state", "show");
            GUI_SetProperty(g_CommonView.widget.item_title, "string", (void*)g_CommonView.string.str_item_title);
            GUI_SetProperty(g_CommonView.widget.list_item, "state", "show");
            GUI_SetProperty(g_CommonView.widget.item_page, "state", "hide");
            GUI_SetProperty(g_CommonView.widget.img_up, "state", "hide");
            GUI_SetProperty(g_CommonView.widget.img_down, "state", "hide");
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM-1; i++)
            {
                GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.line, i), "state", "hide");
            }
            for (i = 0; i < COMMON_VIEW_MAX_PAGE_ITEM; i++)
            {
                app_common_view_hide_one_item(i);
            }
            break;
        default:
            break;
    }
}

void app_common_view_all_tips_hide_show(void)
{
#define _TIP_SHOW_HIDE(arg) do {                                                \
    if (g_CommonView.keyfunc.key_##arg && g_CommonView.string.str_##arg)     \
    {                                                                           \
        GUI_SetProperty(g_CommonView.widget.tips.text_##arg, "string",          \
                (void*)g_CommonView.string.str_##arg);                          \
        GUI_SetProperty(g_CommonView.widget.tips.text_##arg, "state", "show");  \
        GUI_SetProperty(g_CommonView.widget.tips.img_##arg, "state", "show");   \
    }                                                                           \
    else                                                                        \
    {                                                                           \
        GUI_SetProperty(g_CommonView.widget.tips.text_##arg, "state", "hide");  \
        GUI_SetProperty(g_CommonView.widget.tips.img_##arg, "state", "hide");   \
    }                                                                           \
} while(0)

    _TIP_SHOW_HIDE(menu);
    _TIP_SHOW_HIDE(ok);
    _TIP_SHOW_HIDE(red);
    _TIP_SHOW_HIDE(green);
    _TIP_SHOW_HIDE(yellow);
    _TIP_SHOW_HIDE(blue);
}
void app_common_view_set_obj_logo(char* img_title)
{
	char installpath[COMMON_VIWE_PATH_LEN];
	char logo_path[COMMON_VIWE_PATH_LEN];
	memset(installpath,0,COMMON_VIWE_PATH_LEN);
	memset(logo_path,0,COMMON_VIWE_PATH_LEN);
	GxBus_ConfigGet(COMMON_VIWE_CONFIG_PATH, installpath, COMMON_VIWE_PATH_LEN, COMMON_VIWE_PATH_DEFALT);
	sprintf(logo_path,"%s/image/%s.png",installpath,img_title);
	if(GXCORE_FILE_EXIST == GxCore_FileExists(logo_path))
	{
		gal_add_key_path(LOGO_IMG_KEY, logo_path);
		GUI_SetProperty(g_CommonView.widget.img_title, "img", LOGO_IMG_KEY);
	}
	else
	{
		GUI_SetProperty(g_CommonView.widget.img_title, "img", (void*)"IPTV2");
	}
}

int app_common_view_create_dialog(CommonViewParas *paras)
{
    g_CommonView.show_type = paras->show_type;
    g_CommonView.focus_area = paras->focus_area;
    g_CommonView.net_video_flag = paras->net_video_flag;
    g_CommonView.first_in_flag = true;
    g_CommonView.popup_show_flag = false;

    memcpy(&g_CommonView.image, &paras->image, sizeof(CommonViewImageValue));
    memcpy(&g_CommonView.string, &paras->string, sizeof(CommonViewStringValue));
    memcpy(&g_CommonView.signal, &paras->signal, sizeof(CommonViewSignal));
    memcpy(&g_CommonView.keyfunc, &paras->keyfunc, sizeof(CommonViewKeyFunc));
    memset(&g_CommonView.ctrl_data, 0, sizeof(CommonViewCtrlData));
    g_CommonView.ctrl_data.orig_group_sel = paras->orig_group_sel;
    return GUI_CreateDialog(g_CommonView.widget.wnd);
}

SIGNAL_HANDLER int app_common_view_create(GuiWidget *widget, void *usrdata)
{
    CommonViewFocusArea focus_area_bak;

	if(g_CommonView.image.img_title)
	{
		if(gal_check_imagekey(g_CommonView.image.img_title) == GXCORE_SUCCESS)
		{
			GUI_SetProperty(g_CommonView.widget.img_title, "img", (void*)g_CommonView.image.img_title);
		}
		else
		{
			app_common_view_set_obj_logo((void*)g_CommonView.image.img_title);
		}
	}
    else
	{
		if(gal_check_imagekey("IPTV2") == GXCORE_SUCCESS)
		{
			GUI_SetProperty(g_CommonView.widget.img_title, "img", (void*)"IPTV2");
		}
	}

    GUI_SetProperty(g_CommonView.widget.text_title, "string", (void*)g_CommonView.string.str_title);
    app_common_view_change_show_type();
    app_common_view_all_tips_hide_show();
    focus_area_bak = g_CommonView.focus_area;
    g_CommonView.focus_area = COMMON_VIEW_AREA_TOTAL;
    app_common_view_focus_area_change(focus_area_bak);

    app_common_view_hide_gif();
    app_common_view_hide_popup_msg();
    if (g_CommonView.net_video_flag)
        app_common_view_load_gif();
    app_common_view_clock_timer_add();

    if (g_CommonView.signal.create)
        return g_CommonView.signal.create(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_view_destroy(GuiWidget *widget, void *usrdata)
{
    app_common_view_clock_timer_rm();
    app_common_view_logo_download_stop();
    app_common_view_hide_gif();
    app_common_view_hide_popup_msg();
    app_common_view_free_gif();

    if (g_CommonView.keyfunc.set_item_depth)
    {
        g_CommonView.keyfunc.set_item_depth(0);
    }
    if (g_CommonView.keyfunc.exit_item_depth)
    {
        g_CommonView.keyfunc.exit_item_depth();
    }
    if (g_CommonView.signal.destroy)
        return g_CommonView.signal.destroy(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_common_view_service(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.service)
        return g_CommonView.signal.service(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_view_got_focus(GuiWidget *widget, void *usrdata)
{
    int position=0;
    if(g_CommonView.focus_area == COMMON_VIEW_AREA_TABLE_ITEM)
    {
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"format","fix_r2l");
        GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"rolling_position",&position);
    }
    if (g_CommonView.signal.got_focus)
        return g_CommonView.signal.got_focus(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_view_lost_focus(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(_get_widget(g_CommonView.widget.base_item.title,g_CommonView.ctrl_data.page_item_sel),"format","static");;
    if (g_CommonView.signal.lost_focus)
        return g_CommonView.signal.lost_focus(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_view_keypress(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.keypress)
        return g_CommonView.signal.keypress(widget, usrdata);
    else
        return app_common_view_keypress_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.group_list_get_total)
    {
        g_CommonView.ctrl_data.group_total = g_CommonView.signal.group_list_get_total(widget, usrdata);
        return g_CommonView.ctrl_data.group_total;
    }
    else
        return app_common_view_group_list_get_total_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.group_list_get_data)
        return g_CommonView.signal.group_list_get_data(widget, usrdata);
    else
        return app_common_view_group_list_get_data_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_group_list_change(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.group_list_change)
        return g_CommonView.signal.group_list_change(widget, usrdata);
    else
        return app_common_view_group_list_change_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.group_list_keypress)
        return g_CommonView.signal.group_list_keypress(widget, usrdata);
    else
        return app_common_view_group_list_keypress_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_item_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.item_list_get_total)
    {
        g_CommonView.ctrl_data.item_total = g_CommonView.signal.item_list_get_total(widget, usrdata);
        return g_CommonView.ctrl_data.item_total;
    }
    else
        return app_common_view_item_list_get_total_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_item_list_get_data(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.item_list_get_data)
        return g_CommonView.signal.item_list_get_data(widget, usrdata);
    else
        return app_common_view_item_list_get_data_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_item_list_change(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.item_list_change)
        return g_CommonView.signal.item_list_change(widget, usrdata);
    else
        return app_common_view_item_list_change_func(widget, usrdata);
}

SIGNAL_HANDLER int app_common_view_item_list_keypress(GuiWidget *widget, void *usrdata)
{
    if (g_CommonView.signal.item_list_keypress)
        return g_CommonView.signal.item_list_keypress(widget, usrdata);
    else
        return app_common_view_item_list_keypress_func(widget, usrdata);
}

// wnd_common_text
#define COMMON_TEXT_TITLE       "text_common_text_title"
#define COMMON_TEXT_CONTENT     "notepad_common_text"
SIGNAL_HANDLER int app_common_text_keypress(GuiWidget *widget, void *usrdata)
{
    uint32_t value = 1;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;
        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_COMMON_TEXT);
                    break;
                case STBK_UP:
                    GUI_SetProperty(COMMON_TEXT_CONTENT, "line_up", &value);
                    break;
                case STBK_DOWN:
                    GUI_SetProperty(COMMON_TEXT_CONTENT, "line_down", &value);
                    break;
                case STBK_LEFT:
                case STBK_PAGE_UP:
                    GUI_SetProperty(COMMON_TEXT_CONTENT, "page_up", &value);
                    break;
                case STBK_RIGHT:
                case STBK_PAGE_DOWN:
                    GUI_SetProperty(COMMON_TEXT_CONTENT, "page_down", &value);
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

void app_common_text_set_title(const char *str)
{
    GUI_SetProperty(COMMON_TEXT_TITLE, "string", (void*)str);
}

void app_common_text_set_content(const char *str)
{
    GUI_SetProperty(COMMON_TEXT_CONTENT, "string", (void*)str);
}

#if VOICE_CONTROL_SUPPORT
int app_vc_common_view_table_area_select(void)
{
    if(g_CommonView.focus_area == COMMON_VIEW_AREA_TABLE_ITEM)
        return 1;
    else
        return 0;
}
#endif
