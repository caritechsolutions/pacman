#include "app.h"
#include "gui_core.h"
#include "app_default_params.h"
#include "app_windows.h"
#include "app_common_select.h"

CommonSelectControl g_CommonSelect = {
    .choice = COMMON_SELECT_TITLE_ONLY,
    .title = NULL,
    .content = NULL,
    .widget = {
        .wnd = WND_COMMON_SELECT,
        .title = "txt_common_select_title1",
        .group = "cmb_common_select_group1",
        .list = "listview_common_select",
        .tips = {
            .img_bg = "img_cmnsel_bottom",
            .img_line_bg = "img_cl_bottom",
            .img_red = "img_cmnsel_red",
            .text_red = "text_cmnsel_red",
            .img_green = "img_cmnsel_green",
            .text_green = "text_cmnsel_green",
            .img_yellow = "img_cmnsel_yellow",
            .text_yellow = "text_cmnsel_yellow",
            .img_blue = "img_cmnsel_blue",
            .text_blue = "text_cmnsel_blue",
            .img_ok = "img_cmnsel_ok",
            .text_ok = "text_cmnsel_ok",
            .img_exit = "img_cmnsel_exit",
            .text_exit = "text_cmnsel_exit"
        }
    },
    .signal = {NULL}
};

int app_common_select_create_dialog(CommonSelectParas *paras)
{
    switch (paras->choice) {
        case COMMON_SELECT_TITLE_ONLY:
            g_CommonSelect.widget.title = "txt_common_select_title1";
            g_CommonSelect.widget.group = "cmb_common_select_group1";
            break;
        case COMMON_SELECT_GROUP_ONLY:
            g_CommonSelect.widget.title = "txt_common_select_title1";
            g_CommonSelect.widget.group = "cmb_common_select_group1";
            break;
        case COMMON_SELECT_TITLE_GROUP:
            g_CommonSelect.widget.title = "txt_common_select_title2";
            g_CommonSelect.widget.group = "cmb_common_select_group2";
            break;
        default:
            break;
    }
    g_CommonSelect.choice = paras->choice;
    g_CommonSelect.title = paras->title;
    g_CommonSelect.content = paras->content;
    memcpy(&g_CommonSelect.tip_strs, &paras->tip_strs, sizeof(CommonSelectValueTips));
    memcpy(&g_CommonSelect.signal, &paras->signal, sizeof(CommonSelectSignal));
    return GUI_CreateDialog(g_CommonSelect.widget.wnd);
}

static void _common_select_change_choice_show(CommonSelectChoice choice)
{
    switch (choice) {
        case COMMON_SELECT_TITLE_ONLY:
            GUI_SetProperty("txt_common_select_title1", "state", "show");
            GUI_SetProperty("cmb_common_select_group1", "state", "hide");
            GUI_SetProperty("txt_common_select_title2", "state", "hide");
            GUI_SetProperty("cmb_common_select_group2", "state", "hide");
            GUI_SetProperty(g_CommonSelect.widget.title, "string", (void*)g_CommonSelect.title);
            break;
        case COMMON_SELECT_GROUP_ONLY:
            GUI_SetProperty("txt_common_select_title1", "state", "hide");
            GUI_SetProperty("cmb_common_select_group1", "state", "show");
            GUI_SetProperty("txt_common_select_title2", "state", "hide");
            GUI_SetProperty("cmb_common_select_group2", "state", "hide");
            GUI_SetProperty(g_CommonSelect.widget.group, "content", (void*)g_CommonSelect.content);
            break;
        case COMMON_SELECT_TITLE_GROUP:
            GUI_SetProperty("txt_common_select_title1", "state", "hide");
            GUI_SetProperty("cmb_common_select_group1", "state", "hide");
            GUI_SetProperty("txt_common_select_title2", "state", "show");
            GUI_SetProperty("cmb_common_select_group2", "state", "show");
            GUI_SetProperty(g_CommonSelect.widget.title, "string", (void*)g_CommonSelect.title);
            GUI_SetProperty(g_CommonSelect.widget.group, "content", (void*)g_CommonSelect.content);
            break;
        default:
            break;
    }
}

static void _common_select_change_tips_show(CommonSelectValueTips *val)
{
#define _TIP_SHOW_HIDE(arg) do {                                                \
    if (val->str_##arg) {                                                       \
        GUI_SetProperty(g_CommonSelect.widget.tips.text_##arg, "string",        \
                (void*)val->str_##arg);                                         \
        GUI_SetProperty(g_CommonSelect.widget.tips.text_##arg, "state", "show");\
        GUI_SetProperty(g_CommonSelect.widget.tips.img_##arg, "state", "show"); \
    } else {                                                                    \
        GUI_SetProperty(g_CommonSelect.widget.tips.text_##arg, "state", "hide");\
        GUI_SetProperty(g_CommonSelect.widget.tips.img_##arg, "state", "hide"); \
    }                                                                           \
} while(0)
    if (val->str_red || val->str_green || val->str_yellow || val->str_blue || val->str_ok || val->str_exit)
    {
        GUI_SetProperty(g_CommonSelect.widget.tips.img_bg, "state", "show");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_line_bg, "state", "show");
    }
    else
    {
        GUI_SetProperty(g_CommonSelect.widget.tips.img_bg, "state", "hide");
        GUI_SetProperty(g_CommonSelect.widget.tips.img_line_bg, "state", "hide");
    }
    _TIP_SHOW_HIDE(red);
    _TIP_SHOW_HIDE(green);
    _TIP_SHOW_HIDE(yellow);
    _TIP_SHOW_HIDE(blue);
    _TIP_SHOW_HIDE(ok);
    _TIP_SHOW_HIDE(exit);
}

SIGNAL_HANDLER int app_common_select_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetFocusWidget(g_CommonSelect.widget.list);
    _common_select_change_choice_show(g_CommonSelect.choice);
    _common_select_change_tips_show(&g_CommonSelect.tip_strs);
    if (g_CommonSelect.signal.create)
        return g_CommonSelect.signal.create(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_destroy(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.destroy)
        return g_CommonSelect.signal.destroy(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_got_focus(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.got_focus)
        return g_CommonSelect.signal.got_focus(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_lost_focus(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.lost_focus)
        return g_CommonSelect.signal.lost_focus(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_keypress(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.keypress)
        return g_CommonSelect.signal.keypress(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_group_change1(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.group_change)
        return g_CommonSelect.signal.group_change(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_group_change2(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.group_change)
        return g_CommonSelect.signal.group_change(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_list_get_total(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.list_get_total)
        return g_CommonSelect.signal.list_get_total(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_list_get_data(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.list_get_data)
        return g_CommonSelect.signal.list_get_data(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_list_change(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.list_change)
        return g_CommonSelect.signal.list_change(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_common_select_list_keypress(GuiWidget *widget, void *usrdata)
{
    if (g_CommonSelect.signal.list_keypress)
        return g_CommonSelect.signal.list_keypress(widget, usrdata);
    return EVENT_TRANSFER_KEEPON;
}

