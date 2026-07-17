#ifndef __APP_EUTEL_TABLE_INFORMATION__
#define __APP_EUTEL_TABLE_INFORMATION__

#include "gui_core.h"
#include "widget.h"

typedef struct {
    const char *title;
    char *(*value_get)(void);
} TableInfoItem;

typedef struct {
    const char *title;
    int page_sel;
    int page_total;
    int item_total;
    TableInfoItem *item_array;
    int (*create)(GuiWidget *widget, void *usrdata);
    int (*destroy)(GuiWidget *widget, void *usrdata);
} TableInfoPara;

int app_table_info_create_dialog(TableInfoPara *para);
#endif

