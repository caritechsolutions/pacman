#include <stdio.h>
#include <limits.h>
#include "gxhotplug.h"
#include <time.h>
#include "app.h"
#include "app_wnd_system_setting_opt.h"

#if ADVANCED_PRINT_SUPPORT  // only for ecos
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DELAY_MS            100
#define NET_PRINT_SEND_SZIE 1472        // IP包不用分片时sendto发送的最大长度
#define NET_PRINT_IP_ADDR   "224.0.0.8"
#define NET_PRINT_IP_PORT   9001

typedef enum {
    PRINT_TO_NULL = 0,
    PRINT_TO_TTY,
    PRINT_TO_USB,
    PRINT_TO_NET
} AdvancedPrintMode;

typedef struct {
    size_t total;
    size_t used;
    char *buf;
} AdvancedPrintBuf;

typedef struct {
    int enable;
    size_t cache_size;
    int flush_max;
    int flush_cnt;
    AdvancedPrintMode mode;
    AdvancedPrintBuf pbuf;
    AdvancedPrintBuf wbuf;
    handle_t pmtx;
    handle_t wmtx;
    handle_t tid;
} AdvancedPrintCtrl;

AdvancedPrintCtrl s_advanced_print_ctrl;

extern void take_over_puts(int (*puts_cb)(const char *s));
extern void take_over_printf(int (*print_cb)(const char *format, va_list ap));
extern void take_over_printk(int (*print_cb)(const char *format, va_list ap));

static int _print_enable_check(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int flag = 0;
    GxCore_MutexLock(ctrl->pmtx);
    flag = ctrl->enable;
    GxCore_MutexUnlock(ctrl->pmtx);
    return flag;
}

static int _print_thread_check(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int flag = 0;
    GxCore_MutexLock(ctrl->pmtx);
    flag = ctrl->tid;
    GxCore_MutexUnlock(ctrl->pmtx);
    return flag;
}

static int _file_name_get(char *file_name, size_t len)
{
    HotplugPartitionList *phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (!phpl || phpl->partition_num < 1)
        return -1;
    memset(file_name, 0, len);
    strcpy(file_name, phpl->partition[0].partition_entry);
    strcat(file_name, "/gxlog");
    if (GxCore_FileExists(file_name) != GXCORE_FILE_EXIST)
        GxCore_Mkdir(file_name);
    strcat(file_name, "/");

    GxTime time = {0};
    GxCore_GetLocalTime(&time);
    time_t t = time.seconds;
    struct tm *p = gmtime(&t);
    size_t tmp_len = strlen(file_name);
    strftime(file_name+tmp_len, len-tmp_len, "%Y-%m-%d_%H-%M-%S", p);
    strcat(file_name, ".log");
    return 0;
}

static void _print_swap_buf(int check_flag, int no_lock)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    char *buf = NULL;

    if (!no_lock) GxCore_MutexLock(ctrl->wmtx);
    if (check_flag == 0 || ctrl->wbuf.used == 0)
    {
        buf = ctrl->wbuf.buf;
        ctrl->wbuf.buf = ctrl->pbuf.buf;
        ctrl->pbuf.buf = buf;
        ctrl->wbuf.used = ctrl->pbuf.used;
        ctrl->pbuf.used = 0;
    }
    if (!no_lock) GxCore_MutexUnlock(ctrl->wmtx);
}

static int _puts_close_cb(const char *s)
{
    return 1;
}

static int _print_close_cb(const char *format, va_list ap)
{
    return 1;
}

static int _puts_cb(const char *s)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int rc = -1;

    GxCore_MutexLock(ctrl->pmtx);
    if (ctrl->enable)
    {
        rc = snprintf(ctrl->pbuf.buf + ctrl->pbuf.used, ctrl->pbuf.total - ctrl->pbuf.used, "%s\n", s);
        ctrl->pbuf.used += rc;

        if (ctrl->pbuf.total - ctrl->pbuf.used < 1024)
        {
            _print_swap_buf(0, 0);
        }
    }
    GxCore_MutexUnlock(ctrl->pmtx);

    return rc;
}

static int _printf_cb(const char *format, va_list ap)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int rc = -1;

    GxCore_MutexLock(ctrl->pmtx);
    if (ctrl->enable)
    {
        rc = vsnprintf(ctrl->pbuf.buf + ctrl->pbuf.used, ctrl->pbuf.total - ctrl->pbuf.used, format, ap);
        ctrl->pbuf.used += rc;

        if (ctrl->pbuf.total - ctrl->pbuf.used < 1024)
        {
            _print_swap_buf(0, 0);
        }
    }
    GxCore_MutexUnlock(ctrl->pmtx);

    return rc;
}

static int _printk_cb(const char *format, va_list ap)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int rc = -1;

    if (ctrl->enable)
    {
        rc = vsnprintf(ctrl->pbuf.buf + ctrl->pbuf.used, ctrl->pbuf.total - ctrl->pbuf.used, format, ap);
        ctrl->pbuf.used += rc;

        if (ctrl->pbuf.total - ctrl->pbuf.used < 1024)
        {
            _print_swap_buf(0, 1);
        }
    }

    return rc;
}

static int _print_to_usb(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    static char file_name[128];
    FILE *fp = NULL;

    if (ctrl->wbuf.used == 0)
        return 0;
    if (strlen(file_name) == 0 || access(file_name, F_OK) != 0)
    {
        if (_file_name_get(file_name, sizeof(file_name)) < 0)
            return -1;
    }
    if ((fp = fopen(file_name, "a+")) == NULL)
        return -1;
    fwrite(ctrl->wbuf.buf, 1, ctrl->wbuf.used, fp);
    ctrl->wbuf.used = 0;
    ctrl->flush_cnt = 0;
    fclose(fp);

    return 0;
}

static int _print_to_net(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    int sockfd = -1;
    struct sockaddr_in serveraddr;
    int send_size = 0;
    int total_size = 0;

    if (ctrl->wbuf.used == 0)
        return 0;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        return -1;

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(NET_PRINT_IP_ADDR);
    serveraddr.sin_port = htons(NET_PRINT_IP_PORT);
    while (total_size < ctrl->wbuf.used)
    {
        send_size = NET_PRINT_SEND_SZIE;
        if (send_size > ctrl->wbuf.used - total_size)
            send_size = ctrl->wbuf.used - total_size;
        if (send_size != sendto(sockfd, ctrl->wbuf.buf + total_size, send_size,
                    0, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) {
            break;
        }
        total_size += send_size;
    }
    ctrl->wbuf.used = 0;
    ctrl->flush_cnt = 0;
    close(sockfd);

    return 0;
}

static int advanced_print_flush(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;

    GxCore_MutexLock(ctrl->wmtx);
    (ctrl->mode == PRINT_TO_USB) ? _print_to_usb() : _print_to_net();
    GxCore_MutexUnlock(ctrl->wmtx);

    GxCore_MutexLock(ctrl->pmtx);
    _print_swap_buf(0, 0);
    GxCore_MutexUnlock(ctrl->pmtx);

    GxCore_MutexLock(ctrl->wmtx);
    (ctrl->mode == PRINT_TO_USB) ? _print_to_usb() : _print_to_net();
    GxCore_MutexUnlock(ctrl->wmtx);

    return 0;
}

static void _print_flush(void *arg)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;

    GxCore_ThreadDetach();
    while (_print_enable_check())
    {
        if (++ctrl->flush_cnt >= ctrl->flush_max)
        {
            ctrl->flush_cnt = 0;
            GxCore_MutexLock(ctrl->pmtx);
            if (ctrl->pbuf.used)
            {
                _print_swap_buf(1, 0);
            }
            GxCore_MutexUnlock(ctrl->pmtx);
        }

        GxCore_MutexLock(ctrl->wmtx);
        (ctrl->mode == PRINT_TO_USB) ? _print_to_usb() : _print_to_net();
        GxCore_MutexUnlock(ctrl->wmtx);

        GxCore_ThreadDelay(DELAY_MS);
    }

    advanced_print_flush();
    GxCore_MutexLock(ctrl->pmtx);
    ctrl->tid = 0;
    GxCore_MutexUnlock(ctrl->pmtx);
}

int advanced_print_stop(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;

    if (ctrl->pmtx == 0)
    {
        GxCore_MutexCreate(&ctrl->pmtx);
        GxCore_MutexCreate(&ctrl->wmtx);
    }

    GxCore_MutexLock(ctrl->pmtx);
    ctrl->enable = 0;
    take_over_puts(_puts_close_cb);
    take_over_printf(_print_close_cb);
    take_over_printk(_print_close_cb);
    GxCore_MutexUnlock(ctrl->pmtx);

    while(_print_thread_check())
        GxCore_ThreadDelay(DELAY_MS);

    GxCore_MutexLock(ctrl->pmtx);
    GxCore_Free(ctrl->pbuf.buf);
    GxCore_Free(ctrl->wbuf.buf);
    memset(&ctrl->pbuf, 0, sizeof(AdvancedPrintBuf));
    memset(&ctrl->wbuf, 0, sizeof(AdvancedPrintBuf));
    GxCore_MutexUnlock(ctrl->pmtx);

    return 0;
}

int advanced_print_reset(void)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;
    if (ctrl->pmtx == 0)
    {
        GxCore_MutexCreate(&ctrl->pmtx);
        GxCore_MutexCreate(&ctrl->wmtx);
    }

    GxCore_MutexLock(ctrl->pmtx);
    take_over_puts(NULL);
    take_over_printf(NULL);
    take_over_printk(NULL);
    GxCore_MutexUnlock(ctrl->pmtx);
    return 0;
}

int advanced_print_start(AdvancedPrintMode mode, size_t cache_size, int flush_sec)
{
    AdvancedPrintCtrl *ctrl = &s_advanced_print_ctrl;

    if (ctrl->pmtx == 0)
    {
        GxCore_MutexCreate(&ctrl->pmtx);
        GxCore_MutexCreate(&ctrl->wmtx);
    }
    advanced_print_stop();
    if (mode != PRINT_TO_USB && mode != PRINT_TO_NET)
        return -1;

    GxCore_MutexLock(ctrl->pmtx);
    ctrl->mode = mode;
    ctrl->cache_size = cache_size;
    ctrl->flush_max = flush_sec * 1000 / DELAY_MS;

    memset(&ctrl->pbuf, 0, sizeof(AdvancedPrintBuf));
    memset(&ctrl->wbuf, 0, sizeof(AdvancedPrintBuf));
    if ((ctrl->pbuf.buf = GxCore_Malloc(ctrl->cache_size)) == NULL)
    {
        goto err;
    }
    if ((ctrl->wbuf.buf = GxCore_Malloc(ctrl->cache_size)) == NULL)
    {
        GxCore_Free(ctrl->pbuf.buf);
        goto err;
    }
    ctrl->pbuf.total = ctrl->cache_size;
    ctrl->wbuf.total = ctrl->cache_size;
    ctrl->flush_cnt = 0;

    ctrl->enable = 1;
    take_over_puts(_puts_cb);
    take_over_printf(_printf_cb);
    take_over_printk(_printk_cb);
    GxCore_ThreadCreate("print_flush", &ctrl->tid, _print_flush, NULL, 8192, GXOS_DEFAULT_PRIORITY);
    GxCore_MutexUnlock(ctrl->pmtx);

    return 0;
err:
    memset(&ctrl->pbuf, 0, sizeof(AdvancedPrintBuf));
    memset(&ctrl->wbuf, 0, sizeof(AdvancedPrintBuf));
    GxCore_MutexUnlock(ctrl->pmtx);
    return -1;
}

/******** setting screen ********/

typedef enum {
    ITEM_PRINT_MODE = 0,
    ITEM_PRINT_BUFFER,
    ITEM_PRINT_FLUSH_TIME,
    ITEM_PRINT_MANUAL_FLUSH,
    ITEM_PRINT_TOTAL
} ITEM_PRINT;
#define PRINT_SET_TOTAL ITEM_PRINT_MANUAL_FLUSH

static char *s_print_item_title[ITEM_PRINT_TOTAL] = {
    "Output",
    "Buffer",
    "Flush Time",
    "Manual Flush"
};

static char *s_print_item_content[ITEM_PRINT_TOTAL] = {
    "[Off, TTY, USB, NET]",
    "[64KB, 128KB, 256KB, 512KB]",
    "[1min, 5min, 10min, 30min]",
    STR_ID_PRESS_OK
};
static int s_print_cache_array[] = {64 * 1024, 128 * 1024, 256 *1024, 512 * 1024};
static int s_print_sec_array[] = {60, 300, 600, 1800};

#define PRINT_MODE_DF           PRINT_OUTPUT_DEFAULT
#define PRINT_BUFFER_DF         1
#define PRINT_FLUSH_TIME_DF     1
#define PRINT_MODE_KEY          "print>mode"
#define PRINT_BUFFER_KEY        "print>buffer"
#define PRINT_FLUSH_TIME_KEY    "print>flush_time"

static SystemSettingOpt  s_PrintOpt;
static SystemSettingItem s_PrintItem[ITEM_PRINT_TOTAL];
static int s_print_item_flag[PRINT_SET_TOTAL];

int advanced_print_exec(void)
{
    GxBus_ConfigGetInt(PRINT_MODE_KEY, &s_print_item_flag[ITEM_PRINT_MODE], PRINT_MODE_DF);

    switch (s_print_item_flag[ITEM_PRINT_MODE])
    {
        case PRINT_TO_NULL:
            advanced_print_stop();
            break;
        case PRINT_TO_TTY:
            advanced_print_stop();
            advanced_print_reset();
            break;
        case PRINT_TO_USB:
        case PRINT_TO_NET:
            {
                GxBus_ConfigGetInt(PRINT_BUFFER_KEY, &s_print_item_flag[ITEM_PRINT_BUFFER], PRINT_BUFFER_DF);
                GxBus_ConfigGetInt(PRINT_FLUSH_TIME_KEY, &s_print_item_flag[ITEM_PRINT_FLUSH_TIME], PRINT_FLUSH_TIME_DF);
                AdvancedPrintMode mode = s_print_item_flag[ITEM_PRINT_MODE];
                int cache_size = s_print_cache_array[s_print_item_flag[ITEM_PRINT_BUFFER]];
                int flush_sec = s_print_sec_array[s_print_item_flag[ITEM_PRINT_FLUSH_TIME]];
                advanced_print_start(mode, cache_size, flush_sec);
            }
            break;
        default:
            break;
    }

    return 0;
}

static bool _print_setting_check_cb(void)
{
    bool change_flag = false;
    int new_flag[PRINT_SET_TOTAL];
    int i = 0;

    new_flag[ITEM_PRINT_MODE] = s_PrintItem[ITEM_PRINT_MODE].itemProperty.itemPropertyCmb.sel;
    if (new_flag[ITEM_PRINT_MODE] != s_print_item_flag[ITEM_PRINT_MODE])
    {
        change_flag = true;
        s_print_item_flag[ITEM_PRINT_MODE] = new_flag[ITEM_PRINT_MODE];
    }

    if (new_flag[ITEM_PRINT_MODE])
    {
        for (i = ITEM_PRINT_MODE + 1; i < PRINT_SET_TOTAL; i++)
        {
            //app_system_set_item_data_update(i);
            new_flag[i] = s_PrintItem[i].itemProperty.itemPropertyCmb.sel;
            if (new_flag[i] != s_print_item_flag[i])
            {
                change_flag = true;
                s_print_item_flag[i] = new_flag[i];
            }
        }
    }

    return change_flag;
}

static int _print_setting_save_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        GxBus_ConfigSetInt(PRINT_MODE_KEY, s_print_item_flag[ITEM_PRINT_MODE]);
        GxBus_ConfigSetInt(PRINT_BUFFER_KEY, s_print_item_flag[ITEM_PRINT_BUFFER]);
        GxBus_ConfigSetInt(PRINT_FLUSH_TIME_KEY, s_print_item_flag[ITEM_PRINT_FLUSH_TIME]);
        advanced_print_exec();
    }

    app_system_reset_unfocus_image();
    GUI_EndDialog(WND_SYSTEM_SETTING);
    GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);

    return 0;
}

static int app_print_choice_exit_cb(ExitType exit_type)
{
    int ret = EXIT_RET_DEFAULT;

    if(exit_type != EXIT_ABANDON)
    {
        app_system_set_callback_handler(_print_setting_check_cb, _print_setting_save_pop_cb);
        ret = EXIT_RET_POP;
    }

    return ret;
}

static int app_print_mode_change_cb(int sel)
{
    int i = 0;
    if(sel == PRINT_TO_NULL || sel == PRINT_TO_TTY)
    {
        for (i = ITEM_PRINT_MODE + 1; i < ITEM_PRINT_TOTAL; i++)
            app_system_set_item_state_update(i, ITEM_DISABLE);
    }
    else
    {
        for (i = ITEM_PRINT_MODE + 1; i < ITEM_PRINT_TOTAL; i++)
            app_system_set_item_state_update(i, ITEM_NORMAL);
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_print_choice_item_init(ITEM_PRINT sel)
{
    s_PrintItem[sel].itemTitle = s_print_item_title[sel];
    s_PrintItem[sel].itemType = ITEM_CHOICE;
    s_PrintItem[sel].itemProperty.itemPropertyCmb.content = s_print_item_content[sel];
    s_PrintItem[sel].itemProperty.itemPropertyCmb.sel = s_print_item_flag[sel];
    if (sel == ITEM_PRINT_MODE)
    {
        s_PrintItem[sel].itemCallback.cmbCallback.CmbChange = app_print_mode_change_cb;
        s_PrintItem[sel].itemStatus = ITEM_NORMAL;
    }
    else
    {
        s_PrintItem[sel].itemCallback.cmbCallback.CmbChange = NULL;
        s_PrintItem[sel].itemStatus = (s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_NULL \
                || s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_TTY) ? ITEM_DISABLE : ITEM_NORMAL;
    }
}

static int app_print_manual_flush_cb(unsigned short key)
{
    if(key == STBK_OK)
    {
        GxBus_ConfigGetInt(PRINT_MODE_KEY, &s_print_item_flag[ITEM_PRINT_MODE], PRINT_MODE_DF);
        if (s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_USB
                || s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_NET)
        {
            PopDlg pop = {0};
            popdlg_destroy();
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.str = STR_ID_SAVING;
            pop.mode = POP_MODE_UNBLOCK;
            pop.timeout_sec = 1;
            pop.exit_cb = NULL;
            popdlg_create(&pop);
            advanced_print_flush();

        }
        return EVENT_TRANSFER_STOP;
    }
    return EVENT_TRANSFER_KEEPON;
}

static void app_print_flush_item_init(ITEM_PRINT sel)
{
    s_PrintItem[sel].itemTitle = s_print_item_title[sel];
    s_PrintItem[sel].itemType = ITEM_PUSH;
    s_PrintItem[sel].itemProperty.itemPropertyBtn.string = s_print_item_content[sel];
    s_PrintItem[sel].itemCallback.btnCallback.BtnPress = app_print_manual_flush_cb;
    s_PrintItem[sel].itemStatus = (s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_NULL \
                || s_print_item_flag[ITEM_PRINT_MODE] == PRINT_TO_TTY) ? ITEM_DISABLE : ITEM_NORMAL;
}

void app_print_setting_menu_exec(void)
{
    int i = 0;

    GxBus_ConfigGetInt(PRINT_MODE_KEY, &s_print_item_flag[ITEM_PRINT_MODE], PRINT_MODE_DF);
    GxBus_ConfigGetInt(PRINT_BUFFER_KEY, &s_print_item_flag[ITEM_PRINT_BUFFER], PRINT_BUFFER_DF);
    GxBus_ConfigGetInt(PRINT_FLUSH_TIME_KEY, &s_print_item_flag[ITEM_PRINT_FLUSH_TIME], PRINT_FLUSH_TIME_DF);

    memset(&s_PrintOpt, 0, sizeof(SystemSettingOpt));
    s_PrintOpt.menuTitle = "Print Setting";
    s_PrintOpt.titleImage = "s_title_left_utility";
    s_PrintOpt.itemNum = ITEM_PRINT_TOTAL;
    s_PrintOpt.item = s_PrintItem;
    s_PrintOpt.timeDisplay = TIP_HIDE;

    for (i = 0; i < PRINT_SET_TOTAL; i++)
        app_print_choice_item_init(i);
    app_print_flush_item_init(ITEM_PRINT_MANUAL_FLUSH);
    s_PrintOpt.exit = app_print_choice_exit_cb;

    app_system_set_create(&s_PrintOpt);
}

#endif

