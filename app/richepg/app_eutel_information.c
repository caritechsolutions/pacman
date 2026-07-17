#include "app.h"

#if RICHEPG_SUPPORT
#include "app_eutel_windows.h"
#include "app_eutel_information.h"
#include "app_eutel_common.h"

#define MAX_ITEM_NUM        10
#define TEXT_TIELE          "text_table_info_title"
#define TEXT_PAGE           "text_table_info_page"
#define TEXT_IETEM_TIELE    "text_table_info_title%d"
#define TEXT_IETEM_COLON    "text_table_info_colon%d"
#define TEXT_IETEM_VALUE    "text_table_info_value%d"

static TableInfoPara s_table_info_para;

static void app_table_info_hide_one_item(int sel)
{
    char buf[32] = {0};
    sprintf(buf, TEXT_IETEM_TIELE, sel);
    GUI_SetProperty(buf, "string", STR_ID_BLANK);
    sprintf(buf, TEXT_IETEM_COLON, sel);
    GUI_SetProperty(buf, "string", STR_ID_BLANK);
    sprintf(buf, TEXT_IETEM_VALUE, sel);
    GUI_SetProperty(buf, "string", STR_ID_BLANK);
}

static void app_table_info_show_one_item(int sel, TableInfoItem *item)
{
    char buf[32] = {0};
    char *str = NULL;

    if (!item->title || !item->value_get)
    {
        app_table_info_hide_one_item(sel);
        return ;
    }

    sprintf(buf, TEXT_IETEM_TIELE, sel);
    GUI_SetProperty(buf, "string", (void*)item->title);
    sprintf(buf, TEXT_IETEM_COLON, sel);
    GUI_SetProperty(buf, "string", ":");
    sprintf(buf, TEXT_IETEM_VALUE, sel);
    if (item->value_get)
        str = item->value_get();
    GUI_SetProperty(buf, "string", (str && str[0] != 0) ? str : "-");
}

static void app_table_info_show_page(void)
{
    TableInfoPara *para = &s_table_info_para;
    char buf[32] = {0};

    if (para->item_total <= MAX_ITEM_NUM)
    {
        GUI_SetProperty(TEXT_PAGE, "string", STR_ID_BLANK);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%d / %d", para->page_sel+1, para->page_total);
        GUI_SetProperty(TEXT_PAGE, "string", buf);
    }
}

static void app_table_info_update(void)
{
    TableInfoPara *para = &s_table_info_para;
    int i = 0;
    int first_sel = 0;
    int page_item_num = 0;

    app_table_info_show_page();
    if (para->item_total == 0)
    {
        for (i = 0; i < MAX_ITEM_NUM; i++)
            app_table_info_hide_one_item(i);
    }
    else
    {
        first_sel = para->page_sel * MAX_ITEM_NUM;
        page_item_num = para->item_total - first_sel;
        if (page_item_num > MAX_ITEM_NUM)
            page_item_num = MAX_ITEM_NUM;

        for (i = 0; i < page_item_num; i++)
            app_table_info_show_one_item(i, para->item_array + (first_sel+i));
        for (i = page_item_num; i < MAX_ITEM_NUM; i++)
            app_table_info_hide_one_item(i);
    }
}

SIGNAL_HANDLER int app_table_info_create(GuiWidget *widget, void *usrdata)
{
    TableInfoPara *para = &s_table_info_para;
    if (para->create)
        para->create(widget, usrdata);

    GUI_SetProperty(TEXT_TIELE, "string", (void*)para->title);
    app_table_info_update();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_table_info_destroy(GuiWidget *widget, void *usrdata)
{
    TableInfoPara *para = &s_table_info_para;
    if (para->destroy)
        para->destroy(widget, usrdata);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_table_info_keypress(GuiWidget *widget, void *usrdata)
{
    TableInfoPara *para = &s_table_info_para;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    switch (event->type) {
        case GUI_SERVICE_MSG:
            break;
        case GUI_MOUSEBUTTONDOWN:
            break;
        case GUI_KEYDOWN:
            switch(find_virtualkey_ex(event->key.scancode, event->key.sym))
            {
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog("after wnd_full_screen");
                    break;
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_EUTEL_INFORMATION);
                    break;

                case STBK_LEFT:
                case STBK_UP:
                    if (para->item_total <= MAX_ITEM_NUM)
                        break;
                    --para->page_sel;
                    if (para->page_sel < 0)
                        para->page_sel = para->page_total - 1;
                    app_table_info_update();
                    break;

                case STBK_RIGHT:
                case STBK_DOWN:
                    if (para->item_total <= MAX_ITEM_NUM)
                        break;
                    ++para->page_sel;
                    if (para->page_sel >= para->page_total)
                        para->page_sel = 0;
                    app_table_info_update();
                    break;
                default:
                    break;
            }
        default:
            break;
    }

    return EVENT_TRANSFER_STOP;
}

int app_table_info_create_dialog(TableInfoPara *para)
{
    if (!para)
        return -1;
    if (para->page_total == 0)
        para->page_total = (para->item_total-1) / MAX_ITEM_NUM + 1;
    if (para->page_sel < 0)
        para->page_sel = 0;
    else if (para->page_sel >= para->page_total)
        para->page_sel = para->page_total - 1;
    memcpy(&s_table_info_para, para, sizeof(TableInfoPara));
    GUI_CreateDialog(WND_EUTEL_INFORMATION);

    return 0;
}
#endif

