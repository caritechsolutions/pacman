#include "app.h"
#include "app_default_params.h"
#include "app_pop.h"
#include "app_book.h"
#include "app_module.h"
#include "app_windows.h"
#include "app_wnd_system_setting_opt.h"

static GxBookGet bookGet;

#define SEC_PER_DAY (60*60*24)

enum ITEM_POWER_SET
{
    ITEM_POWER_ON = 0,
    ITEM_POWER_ON_TIME,
    ITEM_POWER_OFF,
    ITEM_POWER_OFF_TIME,
    ITEM_POWER_SET_TOTAL
};
typedef struct PowerSet_tag
{
    int book_id;
    int enable ;
    time_t time_hhmm;
} PowerSet_t;
PowerSet_t sPowerSet[2];

static SystemSettingOpt s_PowerSetOpt;
static SystemSettingItem s_PowerSetItem[ITEM_POWER_SET_TOTAL];

static int app_power_on_change_callbak(int sel)
{
    if (sel == 0)
    {
        s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.string = "00:00";
        app_system_set_item_property(ITEM_POWER_ON_TIME, &(s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty));
        app_system_set_item_state_update(ITEM_POWER_ON_TIME, ITEM_DISABLE);
    }
    else
    {
        app_system_set_item_state_update(ITEM_POWER_ON_TIME, ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;

}

static int app_power_off_change_callbak(int sel)
{
    if (sel == 0)
    {
        s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.string = "00:00";
        app_system_set_item_property(ITEM_POWER_OFF_TIME, &(s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty));
        app_system_set_item_state_update(ITEM_POWER_OFF_TIME, ITEM_DISABLE);
    }
    else
    {
        app_system_set_item_state_update(ITEM_POWER_OFF_TIME, ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;

}

static void app_power_on_item_init(void)
{
    s_PowerSetItem[ITEM_POWER_ON].itemTitle = STR_ID_POWER_ON;
    s_PowerSetItem[ITEM_POWER_ON].itemType = ITEM_CHOICE;
    s_PowerSetItem[ITEM_POWER_ON].itemProperty.itemPropertyCmb.content = "[Disable,Enable]";

    s_PowerSetItem[ITEM_POWER_ON].itemProperty.itemPropertyCmb.sel = sPowerSet[0].enable;
    s_PowerSetItem[ITEM_POWER_ON].itemCallback.cmbCallback.CmbChange = app_power_on_change_callbak;
    s_PowerSetItem[ITEM_POWER_ON].itemStatus = ITEM_NORMAL;
}

static void app_power_on_time_item_init(void)
{
    static char tmStart[8] = {0};

    s_PowerSetItem[ITEM_POWER_ON_TIME].itemTitle = STR_ID_POWER_ON_TIME;
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemType = ITEM_EDIT;
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.format = "edit_time";
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.maxlen = "5";
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    if (sPowerSet[0].enable == 1)
    {
        char *str = NULL;
        str = app_time_to_hourmin_edit_str(sPowerSet[0].time_hhmm);
        if (str != NULL)
        {
            strcpy(tmStart, str);
            GxCore_Free(str);
            str = NULL;
        }
        s_PowerSetItem[ITEM_POWER_ON_TIME].itemStatus = ITEM_NORMAL;
        s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.string = tmStart;
    }
    else
    {
        s_PowerSetItem[ITEM_POWER_ON_TIME].itemStatus = ITEM_DISABLE;
        s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.string = "00:00";
    }

    s_PowerSetItem[ITEM_POWER_ON_TIME].itemCallback.editCallback.EditReachEnd = NULL;
    s_PowerSetItem[ITEM_POWER_ON_TIME].itemCallback.editCallback.EditPress = NULL;

}

static void app_power_off_item_init(void)
{
    s_PowerSetItem[ITEM_POWER_OFF].itemTitle = STR_ID_POWER_OFF;
    s_PowerSetItem[ITEM_POWER_OFF].itemType = ITEM_CHOICE;
    s_PowerSetItem[ITEM_POWER_OFF].itemProperty.itemPropertyCmb.content = "[Disable,Enable]";

    s_PowerSetItem[ITEM_POWER_OFF].itemProperty.itemPropertyCmb.sel = sPowerSet[1].enable;
    s_PowerSetItem[ITEM_POWER_OFF].itemCallback.cmbCallback.CmbChange = app_power_off_change_callbak;
    s_PowerSetItem[ITEM_POWER_OFF].itemStatus = ITEM_NORMAL;
}

static void app_power_off_time_item_init(void)
{
    static char tmStart[8] = {0};

    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemTitle = STR_ID_POWER_OFF_TIME;
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemType = ITEM_EDIT;
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.format = "edit_time";
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.maxlen = "5";
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.intaglio = NULL;
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.default_intaglio = NULL;

    if (sPowerSet[1].enable == 1)
    {
        char *str = NULL;
        str = app_time_to_hourmin_edit_str(sPowerSet[1].time_hhmm);
        if (str != NULL)
        {
            strcpy(tmStart, str);
            GxCore_Free(str);
            str = NULL;
        }
        s_PowerSetItem[ITEM_POWER_OFF_TIME].itemStatus = ITEM_NORMAL;
        s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.string = tmStart;
    }
    else
    {
        s_PowerSetItem[ITEM_POWER_OFF_TIME].itemStatus = ITEM_DISABLE;
        s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.string = "00:00";
    }

    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemCallback.editCallback.EditReachEnd = NULL;
    s_PowerSetItem[ITEM_POWER_OFF_TIME].itemCallback.editCallback.EditPress = NULL;

}

static void app_power_set_para_get(void)
{
    int i = 0;
    int count = 0;
    time_t time_temp1;
    time_t time_temp2;

    memset(sPowerSet, 0x0, sizeof(PowerSet_t) * 2);

    time_temp1 = g_AppTime.utc_get(&g_AppTime);
    time_temp1 = get_display_time_by_timezone(time_temp1);
    time_temp1 = time_temp1 - time_temp1 % SEC_PER_DAY;

    count = g_AppBook.get(&bookGet, BOOKMODE_POWER);

    for (i = 0; i < count; i++)
    {
        if (BOOK_POWER_ON == bookGet.book[i].book_type)
        {
            sPowerSet[0].book_id = i;
            sPowerSet[0].enable = 1;
            time_temp2 = get_display_time_by_timezone(bookGet.book[i].trigger_time_start);
            time_temp2 %= SEC_PER_DAY;
            sPowerSet[0].time_hhmm = time_temp1 + time_temp2;
        }
        else if (BOOK_POWER_OFF == bookGet.book[i].book_type)
        {
            sPowerSet[1].book_id = i;
            sPowerSet[1].enable = 1;
            time_temp2 = get_display_time_by_timezone(bookGet.book[i].trigger_time_start);
            time_temp2 %= SEC_PER_DAY;
            sPowerSet[1].time_hhmm = time_temp1 + time_temp2;
        }
    }
}

static void _exit_power_set(void)
{
    int sel = 0;
    GxBook tbook;
    GxBookType bookType;
    time_t time;
    int startT;
    int i = 0, count = 0;
    status_t ret = GXCORE_SUCCESS;
    if ((0 == strcmp(s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.string, s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.string))
            && ((s_PowerSetItem[ITEM_POWER_ON].itemProperty.itemPropertyCmb.sel == 1) && (s_PowerSetItem[ITEM_POWER_OFF].itemProperty.itemPropertyCmb.sel == 1)))
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_ERR_TIMER;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        pop.pos.x = APP_POP_DLG_POS_X;
        pop.pos.y = APP_POP_DLG_POS_Y;
        popdlg_create(&pop);
        return;
    }

    count = g_AppBook.get(&bookGet, BOOKMODE_POWER);
    for (i = 0; i < count; i++)
    {
        g_AppBook.remove(&bookGet.book[i]);
    }
    sel = s_PowerSetItem[ITEM_POWER_ON].itemProperty.itemPropertyCmb.sel;
    if (sel == 1)
    {
        tbook.book_type = BOOK_POWER_ON;
        tbook.trigger_mode = BOOK_TRIG_BY_SEGMENT;
        tbook.repeat_mode.mode = BOOK_REPEAT_EVERY_DAY;

        time = app_edit_str_to_hourmin_sec(s_PowerSetItem[ITEM_POWER_ON_TIME].itemProperty.itemPropertyEdit.string);
        startT = get_local_time_by_timezone(time);
        if (startT < 0)
        {
            tbook.trigger_time_start = startT + SEC_PER_DAY;
        }
        else
        {
            tbook.trigger_time_start = startT;
        }
        tbook.trigger_time_end = tbook.trigger_time_start + 3;
        tbook.trigger_time_advance = 0;
        tbook.struct_size = sizeof(GxBookType);
        bookType = tbook.book_type;
        memcpy(tbook.struct_buf, (uint8_t *)(&bookType), sizeof(GxBookType));
        ret = g_AppBook.creat(&tbook);

        if (ret != GXCORE_SUCCESS)
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.mode = POP_MODE_UNBLOCK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_ERR_TIMER;
            pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.pos.x = APP_POP_DLG_POS_X;
            pop.pos.y = APP_POP_DLG_POS_Y;
            popdlg_create(&pop);
            return;
        }
    }

    sel = s_PowerSetItem[ITEM_POWER_OFF].itemProperty.itemPropertyCmb.sel;
    if (sel == 1)
    {
        tbook.book_type = BOOK_POWER_OFF;
        tbook.trigger_mode = BOOK_TRIG_BY_SEGMENT;
        tbook.repeat_mode.mode = BOOK_REPEAT_EVERY_DAY;

        time = app_edit_str_to_hourmin_sec(s_PowerSetItem[ITEM_POWER_OFF_TIME].itemProperty.itemPropertyEdit.string);
        startT = get_local_time_by_timezone(time);
        if (startT < 0)
        {
            tbook.trigger_time_start = startT + SEC_PER_DAY;
        }
        else
        {
            tbook.trigger_time_start = startT;
        }
        tbook.trigger_time_end = tbook.trigger_time_start + 3;
        tbook.trigger_time_advance = 0;
        tbook.struct_size = sizeof(GxBookType);
        bookType = tbook.book_type;
        memcpy(tbook.struct_buf, (uint8_t *)(&bookType), sizeof(GxBookType));
        ret = g_AppBook.creat(&tbook);

        if (ret != GXCORE_SUCCESS)
        {
            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_OK;
            pop.mode = POP_MODE_UNBLOCK;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_ERR_TIMER;
            pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.pos.x = APP_POP_DLG_POS_X;
            pop.pos.y = APP_POP_DLG_POS_Y;
            popdlg_create(&pop);
            return;
        }
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING, "draw_now", NULL);
}

static int app_power_set_exit_callback(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if (EXIT_ABANDON == exit_type)
    {
        ;
    }
    else
    {
        _exit_power_set();
        ret = EXIT_RET_POP;
    }

    return ret;
}

void app_power_on_off_menu_exec(void)
{
    memset(&s_PowerSetOpt, 0, sizeof(SystemSettingOpt));
    app_booklist_type_set(BOOKMODE_POWER);
    app_power_set_para_get();

    s_PowerSetOpt.menuTitle = STR_ID_POWER_ON_OFF;
    s_PowerSetOpt.itemNum = ITEM_POWER_SET_TOTAL;
    s_PowerSetOpt.topTipImgDisplay = TIP_HIDE;
    s_PowerSetOpt.bottmTipDisplay = TIP_HIDE;
    s_PowerSetOpt.timeDisplay = TIP_HIDE;
    s_PowerSetOpt.item = s_PowerSetItem;

    app_power_on_item_init();
    app_power_on_time_item_init();
    app_power_off_item_init();
    app_power_off_time_item_init();

    s_PowerSetOpt.exit = app_power_set_exit_callback;

    app_system_set_create(&s_PowerSetOpt);
}
