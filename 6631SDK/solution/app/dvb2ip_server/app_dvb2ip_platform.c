#include "app_dvb2ip_platform.h"
#if DVB2IP_SERVER_SUPPORT
#include "app.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "full_screen.h"
#include "module/channel_edit.h"
#include "module/app_play_control.h"
#include "app_windows.h"
#include "app_ts_record.h"
#if FM_SUPPORT
#include "app_fm_record.h"
#include "app_fm.h"
#endif

#define USE_ONE_IP_ONE_CLIENT           0                       // 1, 同一个IP只能存在一个客户端
#define USE_FOLLOW_TP_CHANGE            1                       // 1, 如果新的客户端切换了TP，其它客户端跟随切换播放
#define DVB2IP_SEND_CACHE_SIZE          (188*100*6)              // 给每个客户端分配的发送缓冲大小，至少3倍于DVB2IP_SEND_MIN_SIZE
#define DVB2IP_SEND_MIN_SIZE            (188*20)                // 给每个客户端分配的发送缓冲大小
#define DVB2IP_SEND_MAX_SIZE            (188*60)                // 给每个客户端分配的发送缓冲大小
#define DVB2IP_READ_MIN_SIZE            (188)                   // 使用fifo测试最佳结果

#ifdef LINUX_OS
#define DVB2IP_SERVER_MAX_CLIENTS       16                      // 文件服务器最大监听socket, 不要设为0
#define DVB2IP_STREAM_MAX_CLIENTS       8                       // http流服务器的最大客户端数目, 不要设为0
#define TS_RECORD_FIFO_SIZE             (30 * 256 * 188)
#else
#define DVB2IP_SERVER_MAX_CLIENTS       8
#define DVB2IP_STREAM_MAX_CLIENTS       4
#define TS_RECORD_FIFO_SIZE             (20 * 256 * 188)
#endif
#define DVB2IP_SERVER_PORTS    8998
#define DVB2IP_STREAM_PORTS     8999
#define DVB2IP_USE_FLAG_KEY             "dvb2ip>use_flag"       // 是否启用功能
#define DVB2IP_PRIO_FLAG_KEY            "dvb2ip>prio_flag"      // 是否开启高优先级，ecos开高优先级才能播凤凰高清
#define DVB2IP_FOLLOW_FLAG_KEY          "dvb2ip>follow_flag"    // 是否启用跟随模式，跟随模式只有一个主客户端
#define DVB2IP_SWITCH_FLAG_KEY          "dvb2ip>switch_flag"    // 是否允许跨频点切台

#define DVB2IP_USE_FLAG_DEFAULT         1
#define DVB2IP_PRIO_FLAG_DEFAULT        1
#define DVB2IP_FOLLOW_FLAG_DEFAULT      0
#define DVB2IP_SWITCH_FLAG_DEFAULT      0

#if 0
#define DVB2IP_ERR(fmt, args...)        do { printf("[FERR][%s:%d] ", __func__, __LINE__); printf(fmt, ##args); } while(0)
#define DVB2IP_INFO                     printf
#else
#define DVB2IP_ERR                      app_log_error
#define DVB2IP_INFO                     app_log_info
#endif

/* follow_flag 说明:
 * 0: 独立模式，所有客户端都可以切台和切TP
 * 1: 跟随模式，主客户端可以切台和切TP，从客户端不能切台，只能跟随播放
 * 2: 管理模式，主客户端可以切台和切TP，从客户端可以同TP切台，不能切TP
 */

typedef struct {
    int prog_id;
    int rec_flag;
    int stop_flag;
    char addr[DVB2IP_IPADDR_LEN];
    handle_t rec_handle;
} Dvb2ipClient;

typedef struct {
    int run_flag;
    int start_flag;
    int change_flag;
    int use_flag;
    int prio_flag;
    int follow_flag;
    int switch_flag;
    int net_flag;
    int gui_flag;
    int prog_num;
    int prog_id; //上一次的prog id
    int fm_num;
    int fm_id; //上一次的fm id
    int tp_id;
    int tuner;
    unsigned lost_sec;
    char main_addr[DVB2IP_IPADDR_LEN];
    char ip_addr[DVB2IP_IPADDR_LEN];
    handle_t mtx;
    Dvb2ipClient *client;
} Dvb2ipArg;

Dvb2ipArg s_dvb2ip_arg = {0};

#if USE_FOLLOW_TP_CHANGE
static int app_dvb2ip_play_follow(int pos, TsRecConfig *config)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    handle_t *old_array = NULL;
    handle_t *new_array = NULL;
    int i = 0;

    old_array = GxCore_Calloc(DVB2IP_STREAM_MAX_CLIENTS, sizeof(handle_t));
    new_array = GxCore_Calloc(DVB2IP_STREAM_MAX_CLIENTS, sizeof(handle_t));

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (i != pos)
        {
            old_array[i] = arg->client[i].rec_handle;
            arg->client[i].rec_handle = 0;
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (old_array[i])
        {
            app_ts_record_stop(old_array[i]);
        }
    }

    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (old_array[i])
        {
            new_array[i] = app_ts_record_start(config);
        }
    }

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (old_array[i])
        {
            if (new_array[i])
            {
                if (arg->client[i].prog_id)
                {
                    arg->client[i].rec_handle = new_array[i];
                    new_array[i] = 0;
                }
            }
            else
            {
                DVB2IP_ERR("app_ts_record_start failed!\n");
                if (arg->client[i].prog_id)
                    arg->client[i].stop_flag = 1;
            }
        }
    }
    GxCore_MutexUnlock(arg->mtx);
    dvb2ip_stream_buffer_reset();

    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (new_array[i])
        {
            DVB2IP_INFO("[%s:%d] app_ts_record_stop %d!\n", __func__, __LINE__, i);
            app_ts_record_stop(new_array[i]);
        }
    }

    APP_FREE(old_array);
    APP_FREE(new_array);
    return 0;
}
#endif

static int app_dvb2ip_play_start(int id, const char *ip_addr)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    TsRecConfig config = {0};
    GxBusPmDataProg prog = {0};
    int ret = -1;
    int follow_flag = 0;
    handle_t rec_handle;
    int pos = -1;
    int i = 0;
    int old_tp_id = 0;

    GxBus_PmLock();
    ret = GxBus_PmProgGetById(id, &prog);
    if ( (prog.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
          || (prog.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE))
    {
        GxBus_PmUnlock();
        DVB2IP_ERR("prog id = %d is pass %d,%d !\n", id,prog.skip_flag,prog.lock_flag);
        return -1;
    }
    GxBus_PmUnlock();
    if (ret == GXCORE_ERROR)
    {
        DVB2IP_ERR("prog id = %d is invalid!\n", id);
        return -1;
    }

    GxCore_MutexLock(arg->mtx);
    if (arg->follow_flag)
    {
        if (strlen(arg->main_addr) > 0 && strcmp(arg->main_addr, ip_addr) != 0)
        {
            if (arg->follow_flag == 1 || arg->tp_id != prog.tp_id)
            {
                id = arg->prog_id;
                follow_flag = 1;
            }
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    if (follow_flag)
    {
        GxBus_PmLock();
        ret = GxBus_PmProgGetById(id, &prog);
        if ( (prog.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
              || (prog.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE))
        {
            GxBus_PmUnlock();
            DVB2IP_ERR("prog id = %d is pass %d,%d !\n", id,prog.skip_flag,prog.lock_flag);
            return -1;
        }
        GxBus_PmUnlock();
        if (ret == GXCORE_ERROR)
        {
            DVB2IP_ERR("prog id = %d is invalid!\n", id);
            return -1;
        }
    }

#if USE_ONE_IP_ONE_CLIENT
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        rec_handle = 0;
        GxCore_MutexLock(arg->mtx);
        if (strcmp(arg->client[i].addr, ip_addr) == 0)
        {
            DVB2IP_INFO("[%s:%d] same_ip_addr!\n", __func__, __LINE__);
            arg->client[i].stop_flag = 1;
            rec_handle = arg->client[i].rec_handle;
            arg->client[i].rec_handle = 0;
        }
        GxCore_MutexUnlock(arg->mtx);
        if (rec_handle)
        {
            app_ts_record_stop(rec_handle);
        }
    }
#endif

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag == 0)
        {
            pos = i;
            arg->client[i].rec_flag = 1;
            break;
        }
    }
    GxCore_MutexUnlock(arg->mtx);
    if (pos < 0)
    {
        DVB2IP_ERR("client is full!\n");
        return -1;
    }

    config.prog_id = prog.id;
    config.user_pmt = true;
    rec_handle = app_ts_record_start(&config);
    if (rec_handle == 0)
    {
        DVB2IP_ERR("app_ts_record_start failed!\n");
        GxCore_MutexLock(arg->mtx);
        arg->client[pos].rec_flag = 0;
        GxCore_MutexUnlock(arg->mtx);
        return -1;
    }
    DVB2IP_INFO("\033[31m[%s:%d] %s app_ts_record_start %d %p!\033[0m\n", __func__, __LINE__, ip_addr, pos, (void*)rec_handle);

    GxCore_MutexLock(arg->mtx);
    arg->start_flag = 1;
    arg->client[pos].prog_id = id;
    arg->client[pos].rec_handle = rec_handle;
    strcpy(arg->client[pos].addr, ip_addr);
    if (strlen(arg->main_addr) == 0)
    {
        strcpy(arg->main_addr, ip_addr);
    }
    old_tp_id = arg->tp_id;
    if (!follow_flag)
    {
        arg->prog_id = prog.id;
        arg->tp_id = prog.tp_id;
        arg->tuner = prog.tuner;
    }
    GxCore_MutexUnlock(arg->mtx);
#if USE_FOLLOW_TP_CHANGE
    if (old_tp_id != 0 && old_tp_id != prog.tp_id)
        app_dvb2ip_play_follow(pos, &config);
#endif

    return pos;
}

static bool _check_duplicate_client(int pos)
{
    int i = 0;
    Dvb2ipArg *arg = &s_dvb2ip_arg;

    for(i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if(pos == i)
            continue;

        if(0 == strcmp(arg->client[pos].addr, arg->client[i].addr))
            return true;
    }

    return false;
}

static int app_dvb2ip_play_stop(int pos)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int clear_flag = 1;
    int i = 0;

    if (pos < 0 || pos >= DVB2IP_STREAM_MAX_CLIENTS)
        return -1;

    if (arg->client[pos].rec_handle)
    {
        DVB2IP_INFO("\033[31m[%s:%d] %s app_ts_record_stop %d %p!\033[0m\n", __func__, __LINE__, arg->client[pos].addr, pos, (void*)arg->client[pos].rec_handle);
        app_ts_record_stop(arg->client[pos].rec_handle);
    }

    GxCore_MutexLock(arg->mtx);
    if (strcmp(arg->client[pos].addr, arg->main_addr) == 0 && false == _check_duplicate_client(pos))
    {
        memset(arg->main_addr, 0, DVB2IP_IPADDR_LEN);
    }
    memset(&arg->client[pos], 0, sizeof(Dvb2ipClient));
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag)
        {
            clear_flag = 0;
            break;
        }
    }
    if (clear_flag)
    {
        arg->start_flag = 0;
        arg->prog_id = 0;
        arg->tp_id = 0;
        arg->tuner = 0;
    }
    GxCore_MutexUnlock(arg->mtx);

    return 0;
}

static int app_dvb2ip_prog_list_free(dvb2ip_prog_list_t *list)
{
    if (list)
    {
        APP_FREE(list->tv_array);
        APP_FREE(list->ra_array);
#if FM_SUPPORT
        APP_FREE(list->fm_array);
#endif
        memset(list, 0, sizeof(dvb2ip_prog_list_t));
    }
    return 0;
}

static int app_dvb2ip_prog_num_get(dvb2ip_prog_list_t *list)
{
    int i, j;
    int cnt = 0;
    int num = 0;
    int prog_num = 0;
    int *id_array = NULL;
    GxBusPmDataProg prog;
    GxBusPmViewInfo view_info_use;

    GxBus_PmLock();
    GxBus_PmViewInfoGet(&view_info_use);

    for (j = 0; j < 2; j++)
    {
        view_info_use.group_mode = GROUP_MODE_ALL;
        view_info_use.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        view_info_use.pip_switch = 0;
        view_info_use.pip_main_tp_id = 0;
        view_info_use.stream_type = (j == 0) ? GXBUS_PM_PROG_TV : GXBUS_PM_PROG_RADIO;
        GxBus_PmViewInfoMemSet(&view_info_use);
        num = GxBus_PmProgNumGet();

        cnt = 0;
        if (num > 0)
        {
            if (list)
            {
                if ((id_array = GxCore_Calloc(num, sizeof(int))) == NULL)
                {
                    DVB2IP_ERR("malloc failed!\n");
                    goto err;
                }
                for (i = 0; i < num; i++)
                {
                    if ((GxBus_PmProgGetByPos(i, 1, &prog) > 0)
                            && (prog.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (prog.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE))
                    {
                        id_array[cnt++] = prog.id;
                    }
                }
                (j == 0) ? (list->tv_array = id_array) : (list->ra_array = id_array);
                (j == 0) ? (list->tv_num = cnt) : (list->ra_num = cnt);
            }
            else
            {
                for (i = 0; i < num; i++)
                {
                    if ((GxBus_PmProgGetByPos(i, 1, &prog) > 0)
                            && (prog.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (prog.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE))
                    {
                        cnt++;
                    }
                }
            }
            prog_num += cnt;
        }
    }

    GxBus_PmViewInfoMemClear();
    GxBus_PmUnlock();

#if FM_SUPPORT
    num = app_fm_freq_num_get();
    if (num > 0)
    {
        if (list)
        {
            if ((id_array = GxCore_Calloc(num, sizeof(int))) == NULL)
            {
                DVB2IP_ERR("malloc failed!\n");
                goto err;
            }
            for (i = 0; i < num; i++)
            {
                id_array[i] = i+1;
            }
            list->fm_array = id_array;
            list->fm_num = num;
        }
        prog_num += num;
    }
#endif
    return prog_num;
err:
    if (list)
    {
        app_dvb2ip_prog_list_free(list);
    }
    return 0;
}

static int app_dvb2ip_tv_radio_num_get(dvb2ip_prog_list_t *list, int tp_id)
{
    int i, j;
    int cnt = 0;
    int num = 0;
    int prog_num = 0;
    int *id_array = NULL;
    GxBusPmDataProg prog;
    GxBusPmViewInfo view_info_use;

    GxBus_PmLock();
    GxBus_PmViewInfoGet(&view_info_use);

    for (j = 0; j < 2; j++)
    {
        view_info_use.group_mode = GROUP_MODE_ALL;
        view_info_use.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        view_info_use.pip_switch = 0;
        view_info_use.pip_main_tp_id = 0;
        view_info_use.stream_type = (j == 0) ? GXBUS_PM_PROG_TV : GXBUS_PM_PROG_RADIO;
        GxBus_PmViewInfoMemSet(&view_info_use);
        num = GxBus_PmProgNumGet();

        cnt = 0;
        if (num > 0)
        {
            if (list)
            {
                if ((id_array = GxCore_Calloc(num, sizeof(int))) == NULL)
                {
                    DVB2IP_ERR("malloc failed!\n");
                    goto err;
                }
                for (i = 0; i < num; i++)
                {
                    if ((GxBus_PmProgGetByPos(i, 1, &prog) > 0)
                            && (prog.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (prog.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (tp_id == 0 || tp_id == prog.tp_id))
                    {
                        id_array[cnt++] = prog.id;
                    }
                }
                (j == 0) ? (list->tv_array = id_array) : (list->ra_array = id_array);
                (j == 0) ? (list->tv_num = cnt) : (list->ra_num = cnt);
            }
            else
            {
                for (i = 0; i < num; i++)
                {
                    if ((GxBus_PmProgGetByPos(i, 1, &prog) > 0)
                            && (prog.lock_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (prog.skip_flag != GXBUS_PM_PROG_BOOL_ENABLE)
                            && (tp_id == 0 || tp_id == prog.tp_id))
                    {
                        cnt++;
                    }
                }
            }
            prog_num += cnt;
        }
    }

    GxBus_PmViewInfoMemClear();
    GxBus_PmUnlock();
    return prog_num;
err:
    if (list)
    {
        app_dvb2ip_prog_list_free(list);
    }
    return 0;
}

static int _dvb2ip_all_ids_dump(dvb2ip_prog_list_t *list)
{
    return app_dvb2ip_prog_num_get(list);
}

static int _dvb2ip_cur_ids_dump(dvb2ip_prog_list_t *list, int *play_num)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int tp_id = 0;
    int work_num = 0;
    int i = 0;

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag)
        {
            ++work_num;
            tp_id = arg->tp_id;
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    memset(list, 0, sizeof(dvb2ip_prog_list_t));
    *play_num = work_num;
    if (tp_id)
        return app_dvb2ip_tv_radio_num_get(list, tp_id);

    return 0;
}

static int _dvb2ip_ids_free(dvb2ip_prog_list_t *list)
{
    return app_dvb2ip_prog_list_free(list);
}

static int _dvb2ip_des_get(int id, char *buf, int size)
{
    GxBusPmDataProg prog = {0};

    GxBus_PmLock();
    GxBus_PmProgGetById(id, &prog);
    GxBus_PmUnlock();

    memset(buf, 0, size);
    if (prog.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        snprintf(buf, size, "$%s", (char*)prog.prog_name);
    }
    else
    {
        strncpy(buf, (char*)prog.prog_name, size-1);
    }

    return 0;
}

static int _dvb2ip_data_read(int pos, char *buf, int size)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    handle_t rec_handle;
    int len = 0;

    GxCore_MutexLock(arg->mtx);
    if (arg->client[pos].stop_flag)
    {
        GxCore_MutexUnlock(arg->mtx);
        return -1;
    }
    if (arg->client[pos].rec_handle == 0)
    {
        GxCore_MutexUnlock(arg->mtx);
        return 0;
    }
    rec_handle = arg->client[pos].rec_handle;
    GxCore_MutexUnlock(arg->mtx);

    len = app_ts_record_read(rec_handle, (uint8_t *)buf, size);
    return len;
}

static int _dvb2ip_send_start(int id, const char *ip_addr)
{
    Dvb2ipStartMsg param;
    param.mode =0;
    param.play_support = 0;
    int pos = app_dvb2ip_play_start(id, ip_addr);
    if (pos >= 0)
        app_send_msg_exec(GXMSG_DVB2IP_PLAY_START, &param);
    return pos;
}

static int _dvb2ip_send_stop(int pos)
{
    app_dvb2ip_play_stop(pos);
    app_send_msg_exec(GXMSG_DVB2IP_PLAY_STOP, NULL);
    return 0;
}

#if FM_SUPPORT
static int app_dvb2ip_fm_play_start(int id, const char *ip_addr)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    FmRecConfig config = {0};
    int need_lock_tp = 0;
    int follow_flag = 0;
    handle_t rec_handle;
    int pos = -1;
    int i = 0;

    if((arg->follow_flag != 1) && (strlen(arg->main_addr) > 0) && strcmp(ip_addr,arg->main_addr)!=0)
    {
        DVB2IP_INFO("[%s:%d] app_fm play err,no allow mode!\n", __func__, __LINE__);
        return -1;
    }
    GxCore_MutexLock(arg->mtx);
    if (arg->follow_flag == 1)
    {
        if (strlen(arg->main_addr) > 0 && strcmp(arg->main_addr, ip_addr) != 0)
        {
            if (arg->fm_id != id)
            {
                id = arg->fm_id;
                follow_flag = 1;
            }
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        rec_handle = 0;
        GxCore_MutexLock(arg->mtx);
        if (strcmp(arg->client[i].addr, ip_addr) == 0)
        {
            DVB2IP_INFO("[%s:%d] same_ip_addr!\n", __func__, __LINE__);
            arg->client[i].stop_flag = 1;
            rec_handle = arg->client[i].rec_handle;
            arg->client[i].rec_handle = 0;
        }
        GxCore_MutexUnlock(arg->mtx);
        if (rec_handle)
        {
            app_fm_record_stop(rec_handle);
        }
    }

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag == 0)
        {
            pos = i;
            arg->client[i].rec_flag = 1;
            break;
        }
    }
    GxCore_MutexUnlock(arg->mtx);
    if (pos < 0)
    {
        DVB2IP_ERR("client is full!\n");
        return -1;
    }

    config.index = id;
    if (arg->fm_id != id)
        need_lock_tp = 1;
    rec_handle = app_fm_record_start(&config, need_lock_tp);
    if (rec_handle == 0)
    {
        DVB2IP_ERR("app_ts_record_start failed!\n");
        GxCore_MutexLock(arg->mtx);
        arg->client[pos].rec_flag = 0;
        GxCore_MutexUnlock(arg->mtx);
        return -1;
    }
    DVB2IP_INFO("\033[31m[%s:%d] %s app_ts_record_start %d %p!\033[0m\n", __func__, __LINE__, ip_addr, pos, (void*)rec_handle);

    GxCore_MutexLock(arg->mtx);
    arg->start_flag = 1;
    arg->client[pos].rec_handle = rec_handle;
    strcpy(arg->client[pos].addr, ip_addr);
    if (strlen(arg->main_addr) == 0)
    {
        strcpy(arg->main_addr, ip_addr);
    }
    if (!follow_flag)
    {
        arg->fm_id = id;
    }
    GxCore_MutexUnlock(arg->mtx);
    GxBus_ConfigSetInt(FM_PLAY_INDEX, id-1);

    return pos;
}

static int app_dvb2ip_fm_play_stop(int pos)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int clear_flag = 1;
    int i = 0;

    if (pos < 0 || pos >= DVB2IP_STREAM_MAX_CLIENTS)
        return -1;

    if (arg->client[pos].rec_handle)
    {
        DVB2IP_INFO("\033[31m[%s:%d] %s app_ts_record_stop %d %p!\033[0m\n", __func__, __LINE__, arg->client[pos].addr, pos, (void*)arg->client[pos].rec_handle);
        app_fm_record_stop(arg->client[pos].rec_handle);
    }

    GxCore_MutexLock(arg->mtx);
    if (strcmp(arg->client[pos].addr, arg->main_addr) == 0 && false == _check_duplicate_client(pos))
    {
        memset(arg->main_addr, 0, DVB2IP_IPADDR_LEN);
    }
    memset(&arg->client[pos], 0, sizeof(Dvb2ipClient));
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag)
        {
            clear_flag = 0;
            break;
        }
    }
    if (clear_flag)
    {
        arg->start_flag = 0;
        arg->fm_id = 0;
    }
    GxCore_MutexUnlock(arg->mtx);

    return 0;
}

static int app_dvb2ip_fm_num_get(dvb2ip_prog_list_t *list)
{
    int i = 0;
    int num = 0;
    int *id_array = NULL;
    num = app_fm_freq_num_get();

    if (num > 0)
    {
        if (list)
        {
            if ((id_array = GxCore_Calloc(num, sizeof(int))) == NULL)
            {
                DVB2IP_ERR("malloc failed!\n");
                goto err;
            }
            for (i = 0; i < num; i++)
            {
                id_array[i] = i+1;
            }
            list->tv_array = id_array;
            list->tv_num = num;
        }
    }
    return num;
err:
    if (list)
    {
        app_dvb2ip_prog_list_free(list);
    }
    return 0;
}

static int _dvb2ip_fm_cur_ids_dump(dvb2ip_prog_list_t *list, int *play_num)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int work_num = 0;
    int i = 0;

    GxCore_MutexLock(arg->mtx);
    for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
    {
        if (arg->client[i].rec_flag)
        {
            ++work_num;
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    memset(list, 0, sizeof(dvb2ip_prog_list_t));
    *play_num = work_num;
    return app_dvb2ip_fm_num_get(list);
}

static int _dvb2ip_fm_des_get(int id, char *buf, int size)
{
    memset(buf, 0, size);
    if(id > 0)
        sprintf(buf, "%.1f MHz", app_fm_get_freq(id-1)/1000.0);

    return 0;
}

static int _dvb2ip_fm_data_read(int pos, char *buf, int size)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    handle_t rec_handle;
    int len = 0;

    GxCore_MutexLock(arg->mtx);
    if (arg->client[pos].stop_flag)
    {
        GxCore_MutexUnlock(arg->mtx);
        return -1;
    }
    if (arg->client[pos].rec_handle == 0)
    {
        GxCore_MutexUnlock(arg->mtx);
        return 0;
    }
    rec_handle = arg->client[pos].rec_handle;
    GxCore_MutexUnlock(arg->mtx);

    len = app_fm_record_read(rec_handle, (uint8_t *)buf, size);
    return len;
}

static int _dvb2ip_fm_send_start(int id, const char *ip_addr)
{
    int pos = -1;
    Dvb2ipStartMsg param;

    param.mode =1;
    param.play_support = 0;
    app_send_msg_exec(GXMSG_DVB2IP_PLAY_START, &param);
    if(param.play_support)
    {
        pos = app_dvb2ip_fm_play_start(id, ip_addr);
    }
    return pos;
}

static int _dvb2ip_fm_send_stop(int pos)
{
    app_dvb2ip_fm_play_stop(pos);
    app_send_msg_exec(GXMSG_DVB2IP_PLAY_STOP, NULL);
    return 0;
}

#endif

static int _dvb2ip_start_para_get(dvb2ip_start_t *para)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    memset(para, 0, sizeof(dvb2ip_start_t));

    para->info.server_num = DVB2IP_SERVER_MAX_CLIENTS;
    para->info.stream_num = DVB2IP_STREAM_MAX_CLIENTS;
    strcpy(para->info.ip_addr, arg->ip_addr);
    para->info.server_port = DVB2IP_SERVER_PORTS;
    para->info.stream_port = DVB2IP_STREAM_PORTS;
    para->info.stream_prio = (arg->prio_flag) ? (-1) : (0);
    para->info.send_cache_size = DVB2IP_SEND_CACHE_SIZE;
    para->info.send_min_size = DVB2IP_SEND_MIN_SIZE;
    para->info.send_max_size = DVB2IP_SEND_MAX_SIZE;
    para->info.read_min_size = DVB2IP_READ_MIN_SIZE;
    para->info.vir_play_file = "/play_file"; // 当前客户端播放TP的所有节目id文件
    para->info.dev_name = "GX STB DVB2IP"; // 设备名称

    para->prog_cb.ids_dump = _dvb2ip_all_ids_dump;
    para->prog_cb.cur_ids_dump = _dvb2ip_cur_ids_dump;
    para->prog_cb.ids_free = _dvb2ip_ids_free;
    para->prog_cb.des_get = _dvb2ip_des_get;
    para->prog_cb.data_read = _dvb2ip_data_read;
    para->prog_cb.send_start = _dvb2ip_send_start;
    para->prog_cb.send_stop = _dvb2ip_send_stop;

#if FM_SUPPORT
   // para->prog_cb.ids_dump = _dvb2ip_fm_all_ids_dump;
    para->fm_prog_cb.cur_ids_dump = _dvb2ip_fm_cur_ids_dump;
   // para->prog_cb.ids_free = _dvb2ip_ids_free;
    para->fm_prog_cb.des_get = _dvb2ip_fm_des_get;
    para->fm_prog_cb.data_read = _dvb2ip_fm_data_read;
    para->fm_prog_cb.send_start = _dvb2ip_fm_send_start;
    para->fm_prog_cb.send_stop = _dvb2ip_fm_send_stop;
#endif
    return 0;
}

int app_dvb2ip_resource_alloc(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int ret = 0;

    GxCore_MutexLock(arg->mtx);
    if (!arg->client)
    {
        arg->client = GxCore_Calloc(DVB2IP_STREAM_MAX_CLIENTS, sizeof(Dvb2ipClient));
        if (!arg->client)
        {
            DVB2IP_ERR("malloc failed!\n");
            ret = -1;
            goto end;
        }
        if(arg->prog_num)
        {
            if (app_ts_record_init(TS_RECORD_FIFO_SIZE, DVB2IP_STREAM_MAX_CLIENTS) < 0)
            {
                GxCore_Free(arg->client);
                arg->client = NULL;
                DVB2IP_ERR("app_ts_record_init failed!\n");
                ret = -1;
                goto end;
            }
        }
#if FM_SUPPORT
        if(arg->fm_num)
        {
            if (app_fm_record_init(TS_RECORD_FIFO_SIZE, DVB2IP_STREAM_MAX_CLIENTS) < 0)
            {
                GxCore_Free(arg->client);
                arg->client = NULL;
                DVB2IP_ERR("app_ts_record_init failed!\n");
                ret = -1;
                goto end;
            }
        }
#endif
        DVB2IP_INFO("\033[32malloc dvb2ip resource\033[0m\n");
    }
end:
    GxCore_MutexUnlock(arg->mtx);

    return ret;
}

int app_dvb2ip_resource_free(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int ret = 0;
    int i = 0, cnt = 0;

    GxCore_MutexLock(arg->mtx);
    if (arg->client)
    {
        for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
        {
            if (arg->client[i].rec_flag)
            {
                ++cnt;
            }
        }
        if (cnt)
        {
            ret = -1;
            goto end;
        }

        if(arg->prog_num)
            app_ts_record_destroy();
#if FM_SUPPORT
        if(arg->fm_num)
            app_fm_record_destroy();
#endif
        GxCore_Free(arg->client);
        arg->client = NULL;
        DVB2IP_INFO("\033[32mfree dvb2ip resource\033[0m\n");
    }
end:
    GxCore_MutexUnlock(arg->mtx);

    return ret;
}

static int app_dvb2ip_service_change(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int do_stop = 0;
    int do_start = 0;
    dvb2ip_start_t para;

    memset(&para, 0, sizeof(dvb2ip_start_t));
    GxCore_MutexLock(arg->mtx);
    if (arg->change_flag)
    {
        arg->change_flag = 0;
        if (arg->run_flag)
        {
            do_stop = 1;
        }
        if (arg->use_flag && arg->net_flag && arg->gui_flag && (arg->prog_num + arg->fm_num))
        {
            do_start = 1;
            _dvb2ip_start_para_get(&para);
        }
    }
    if (do_start == 0 && arg->run_flag == 0)
    {
        if (arg->use_flag && arg->net_flag && arg->gui_flag && (arg->prog_num + arg->fm_num))
        {
            do_start = 1;
            _dvb2ip_start_para_get(&para);
        }
    }
    GxCore_MutexUnlock(arg->mtx);

    /* Always-on serial diagnostic (independent of s_http_printf_level): the
     * server starts only when all four gates are true, so this line pinpoints
     * which one is blocking on a fresh bring-up. */
    printf("[DVB2IP] service_change: use=%d net=%d gui=%d prog=%d fm=%d run=%d -> %s (ip=%s ports=%d/%d)\n",
           arg->use_flag, arg->net_flag, arg->gui_flag, arg->prog_num, arg->fm_num, arg->run_flag,
           do_start ? "START" : (do_stop ? "STOP" : "no-op"),
           arg->ip_addr, DVB2IP_SERVER_PORTS, DVB2IP_STREAM_PORTS);

    if (do_stop)
    {
        DVB2IP_INFO("\033[31mstop dvb2ip server\033[0m\n");
        GxCore_MutexLock(arg->mtx);
        arg->run_flag = 0;
        GxCore_MutexUnlock(arg->mtx);
        dvb2ip_service_stop();
        if (!do_start)
        {
            app_dvb2ip_resource_free();
        }
    }
    if (do_start)
    {
        DVB2IP_INFO("\033[31mstart dvb2ip server\033[0m\n");
        if (app_dvb2ip_resource_alloc() < 0)
            return -1;
        GxCore_MutexLock(arg->mtx);
        arg->run_flag = 1;
        GxCore_MutexUnlock(arg->mtx);
        if (dvb2ip_service_start(&para) < 0)
        {
            DVB2IP_ERR("dvb2ip_service_start failed!\n");
            printf("[DVB2IP] dvb2ip_service_start FAILED (bind %s:%d/%d in use? bad ip?)\n",
                   arg->ip_addr, DVB2IP_SERVER_PORTS, DVB2IP_STREAM_PORTS);
            GxCore_MutexLock(arg->mtx);
            arg->run_flag = 0;
            GxCore_MutexUnlock(arg->mtx);
            return -1;
        }
        printf("[DVB2IP] server STARTED  ->  playlist: http://%s:%d/play_file   stream: http://%s:%d/stream=<prog_id>.ts\n",
               arg->ip_addr, DVB2IP_SERVER_PORTS, arg->ip_addr, DVB2IP_STREAM_PORTS);
    }

    return 0;
}

void app_dvb2ip_arg_init(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int init_value = 0;
    int use_flag = 0;

#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    GxBus_ConfigGetInt(DVB2IP_USE_FLAG_KEY, &init_value, DVB2IP_USE_FLAG_DEFAULT);
    use_flag = init_value;
#endif
    GxBus_ConfigGetInt(DVB2IP_PRIO_FLAG_KEY, &init_value, DVB2IP_PRIO_FLAG_DEFAULT);
    arg->prio_flag = init_value;
    GxBus_ConfigGetInt(DVB2IP_FOLLOW_FLAG_KEY, &init_value, DVB2IP_FOLLOW_FLAG_DEFAULT);
    arg->follow_flag = init_value;
    GxBus_ConfigGetInt(DVB2IP_SWITCH_FLAG_KEY, &init_value, DVB2IP_SWITCH_FLAG_DEFAULT);
    arg->switch_flag = init_value;

    GxCore_MutexCreate(&arg->mtx);
    arg->change_flag = 1;
    app_dvb2ip_use_flag_set(use_flag);
}

int app_dvb2ip_use_flag_get(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int flag = 0;
    GxCore_MutexLock(arg->mtx);
    flag = arg->use_flag;
    GxCore_MutexUnlock(arg->mtx);
    return flag;
}

int app_dvb2ip_prio_flag_get(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int flag = 0;
    GxCore_MutexLock(arg->mtx);
    flag = arg->prio_flag;
    GxCore_MutexUnlock(arg->mtx);
    return flag;
}

int app_dvb2ip_follow_flag_get(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int flag = 0;
    GxCore_MutexLock(arg->mtx);
    flag = arg->follow_flag;
    GxCore_MutexUnlock(arg->mtx);
    return flag;
}

int app_dvb2ip_switch_flag_get(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int flag = 0;
    GxCore_MutexLock(arg->mtx);
    flag = arg->switch_flag;
    GxCore_MutexUnlock(arg->mtx);
    return flag;
}

void app_dvb2ip_use_flag_set(int flag)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;

#if SAT2IP_SERVER_SUPPORT
    if (flag)
    {
        extern void app_sat2ip_use_flag_set(int flag);
        app_sat2ip_use_flag_set(0);
    }
#endif
    GxCore_MutexLock(arg->mtx);
    if (arg->use_flag != flag)
    {
        arg->use_flag = flag;
        arg->change_flag = 1;
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
        GxBus_ConfigSetInt(DVB2IP_USE_FLAG_KEY, flag);
#endif
    }
    GxCore_MutexUnlock(arg->mtx);
    app_dvb2ip_service_change();
}

void app_dvb2ip_prio_flag_set(int flag)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxCore_MutexLock(arg->mtx);
    if (arg->prio_flag != flag)
    {
        arg->prio_flag = flag;
        arg->change_flag = 1;
        GxBus_ConfigSetInt(DVB2IP_PRIO_FLAG_KEY, flag);
    }
    GxCore_MutexUnlock(arg->mtx);
}

void app_dvb2ip_follow_flag_set(int flag)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxCore_MutexLock(arg->mtx);
    if (arg->follow_flag != flag)
    {
        arg->follow_flag = flag;
        arg->change_flag = 1;
        GxBus_ConfigSetInt(DVB2IP_FOLLOW_FLAG_KEY, flag);
    }
    GxCore_MutexUnlock(arg->mtx);
}

void app_dvb2ip_switch_flag_set(int flag)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxCore_MutexLock(arg->mtx);
    if (arg->switch_flag != flag)
    {
        arg->switch_flag = flag;
        GxBus_ConfigSetInt(DVB2IP_SWITCH_FLAG_KEY, flag);
    }
    GxCore_MutexUnlock(arg->mtx);
}

void app_dvb2ip_net_flag_set(int flag, const char *ip_addr)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxCore_MutexLock(arg->mtx);
    if (arg->net_flag != flag)
    {
        arg->net_flag = flag;
        arg->change_flag = 1;
    }
    if (flag == 1 && strcmp(arg->ip_addr, ip_addr) != 0)
    {
        strcpy(arg->ip_addr, ip_addr);
        arg->change_flag = 1;
    }
    GxCore_MutexUnlock(arg->mtx);
    app_dvb2ip_service_change();
}

void app_dvb2ip_gui_flag_set(int flag)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxCore_MutexLock(arg->mtx);
    if (arg->gui_flag != flag)
    {
        arg->gui_flag = flag;
        arg->change_flag = 1;
    }
    GxCore_MutexUnlock(arg->mtx);
    app_dvb2ip_service_change();
}

void app_dvb2ip_prog_num_update(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int fm_num = 0;
    int prog_num;

    /* Safe to call from the play path before dvb2ip is initialised: the mutex
     * is 0 until arg_init runs GxCore_MutexCreate, so bail until then. */
    if (arg->mtx == 0)
        return;

    prog_num = app_dvb2ip_tv_radio_num_get(NULL, 0);
#if FM_SUPPORT
    fm_num = app_dvb2ip_fm_num_get(NULL);
#endif
    GxCore_MutexLock(arg->mtx);
    /* No change -> do nothing: this runs on every zap, and re-flagging change
     * would make service_change stop+restart a running server each time. */
    if (arg->prog_num == prog_num && arg->fm_num == fm_num)
    {
        GxCore_MutexUnlock(arg->mtx);
        return;
    }
    printf("[DVB2IP] prog_num %d->%d fm %d->%d\n", arg->prog_num, prog_num, arg->fm_num, fm_num);
    arg->prog_num = prog_num;
    arg->fm_num = fm_num;
    arg->change_flag = 1;
    GxCore_MutexUnlock(arg->mtx);
    app_dvb2ip_service_change();
}

static void _dvb2ip_switch_tip(void)
{
    PopDlg pop = {0};

    popdlg_destroy();
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_DVB2IP_STOP_TIP;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;
    popdlg_create(&pop);
}

int app_dvb2ip_switch_tip_get_by_prog(GxBusPmDataProg *prog)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    if (arg->tp_id && !arg->switch_flag)
    {
        if (arg->tp_id != prog->tp_id && arg->tuner == prog->tuner)
        {
            _dvb2ip_switch_tip();
            return -1;
        }
    }
    return 0;
}

bool app_dvb2ip_fm_get_client_connect(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    if(strlen(arg->main_addr) != 0)
        return true;
    else
        return 0;
}

#if FM_SUPPORT
static void _dvb2ip_fm_switch_tip(void)
{
    PopDlg pop = {0};

    popdlg_destroy();
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.str = STR_ID_FM_STOP_REC;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;
    popdlg_create(&pop);
}
int app_dvb2ip_fm_stop_rec_tip_by_freq(uint32_t freq_khz)
{
    uint32_t freq=0;
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    if (strlen(arg->main_addr) != 0)
    {
        freq= app_fm_get_freq(arg->fm_id-1);
        if(freq != freq_khz)
        {
            _dvb2ip_fm_switch_tip();
            return -1;
        }
    }
    return 0;
}
#endif

int app_dvb2ip_switch_tip_get_by_pos(int prog_pos)
{
    GxMsgProperty_NodeByPosGet prog_node = {0};
    prog_node.node_type = NODE_PROG;
    prog_node.pos = prog_pos;
    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node)))
    {
        return 0;
    }
    return app_dvb2ip_switch_tip_get_by_prog(&prog_node.prog_data);
}

int app_dvb2ip_switch_tip_get_by_id(int prog_id)
{
    GxMsgProperty_NodeByIdGet prog_node = {0};
    prog_node.node_type = NODE_PROG;
    prog_node.id = prog_id;
    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
    {
        return 0;
    }
    return app_dvb2ip_switch_tip_get_by_prog(&prog_node.prog_data);
}

int app_dvb2ip_switch_tip_get_force(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    if (arg->tp_id && !arg->switch_flag)
    {
        _dvb2ip_switch_tip();
        return -1;
    }
    return 0;
}

int app_dvb2ip_operate_tip_get_force(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    if (arg->run_flag && !arg->switch_flag)
    {
        _dvb2ip_switch_tip();
        return -1;
    }
    return 0;
}

bool app_dvb2ip_check_playing(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    bool flag = false;
    flag = (arg->tp_id > 0) ? true : false;
    return flag;
}

static int app_dvb2ip_change_tp_lock(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxBusPmDataProg play_prog = {0};
    GxBusPmDataProg rec_prog = {0};
    int rec_id = 0;
    bool do_lock = false;
    int ret = -1;

    GxCore_MutexLock(arg->mtx);
    if (arg->start_flag)
    {
        do_lock = true;
        arg->start_flag = 0;
        rec_id = arg->prog_id;
    }
    GxCore_MutexUnlock(arg->mtx);

    if (!do_lock)
        return 0;

    ret = GxBus_PmProgGetById(rec_id, &rec_prog);
    if (ret == GXCORE_ERROR)
    {
        DVB2IP_ERR("rec_prog id = %d is invalid!\n", rec_id);
        return -1;
    }

    if (g_AppPlayOps.running)
    {
        GxBus_PmProgGetByPos(g_AppPlayOps.normal_play.play_count, 1, &play_prog);
        if (play_prog.tp_id == rec_prog.tp_id)
        {
            goto end;
        }
        if (app_dvb2ip_set_play(rec_id) < 0)
            return -1;
    }
    else
    {
        if (app_dvb2ip_set_tp(rec_id) < 0)
            return -1;
    }

end:
#if !DVB2IP_SERVER_USE_STATIC_SCREEN
    {
        PopDlg pop = {0};
        popdlg_destroy();
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_DVB2IP_PLAY;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
    }
#endif

    return 0;
}

bool app_dvb2ip_resume_normal_play(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    GxBusPmDataProg rec_prog = {0};
    int rec_id = 0;
    int ret = -1;

    GxCore_MutexLock(arg->mtx);
    rec_id = arg->prog_id;
    GxCore_MutexUnlock(arg->mtx);

    if (rec_id == 0)
        return true;

    ret = GxBus_PmProgGetById(rec_id, &rec_prog);
    if (ret == GXCORE_ERROR)
    {
        DVB2IP_ERR("rec_prog id = %d is invalid!\n", rec_id);
        return true;
    }
    if (app_dvb2ip_set_play(rec_id) < 0)
    {
        return true;
    }

    return false;
}
char *app_dvb2ip_get_prog_url(int prog_id)
{
#define DVB2IP_PROG_URL_LEN 128
	char *url = NULL;
    Dvb2ipArg *arg = &s_dvb2ip_arg;

	if(arg->use_flag && arg->net_flag && arg->prog_num)
	{
		url = (char *)GxCore_Mallocz(DVB2IP_PROG_URL_LEN);
		if(url)
		{
			snprintf(url, DVB2IP_PROG_URL_LEN-1, "http://%s:%d/stream=%d.ts", arg->ip_addr, DVB2IP_STREAM_PORTS, prog_id);
		}
	}
	return url;
}

#if DVB2IP_SERVER_USE_STATIC_SCREEN
static void app_dvb2ip_play_update_info(void);
#endif
#if FM_SUPPORT
int app_msg_dvb2ip_fm_play_start(Dvb2ipStartMsg* param)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    uint32_t tuner = 0;
    extern void app_fm_play_exec(void);

    AppFrontend_Monitor monitor;

    if(GUI_CheckDialog(WND_FM_PLAY) == GXCORE_SUCCESS)
    {
        if(strlen(arg->main_addr)== 0)
        {
            GUI_EndDialog("after wnd_fm_play");
            GUI_CreateDialog(WND_FM_PLAY_INFO);
        }
        param->play_support=1;
    }
    else if((strcmp(GUI_GetFocusWindow(),WND_MAIN_MENU_ITEM) == 0) ||(strcmp(GUI_GetFocusWindow(),WND_COMMON_2ND_MENU) == 0))
    {
        param->play_support=1;
        tuner = app_tuner_cur_tuner_get();
        monitor = FRONTEND_MONITOR_OFF;
        app_ioctl(tuner, FRONTEND_MONITOR_SET, &monitor);
        GxFrontend_SetUnlock(tuner);
        GxFrontend_CleanRecordPara(tuner);
        app_fm_play_exec();

        PopDlg pop = {0};
        popdlg_destroy();
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_DVB2IP_PLAY;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
    }
    else
    {
        param->play_support=0;
    }
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    app_dvb2ip_play_update_info();
#endif
    return 0;
}

int app_msg_dvb2ip_fm_play_stop(void)
{
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    app_dvb2ip_play_update_info();
#endif
    return 0;
}
#endif
int app_msg_dvb2ip_play_start(Dvb2ipStartMsg *param)
{
    if(param->mode == 0)
    {
        app_dvb2ip_change_tp_lock();
#if DVB2IP_SERVER_USE_STATIC_SCREEN
        app_dvb2ip_play_update_info();
#endif
    }
    else
    {
#if FM_SUPPORT
        app_msg_dvb2ip_fm_play_start(param);
#endif
    }
    return 0;
}

int app_msg_dvb2ip_play_stop(void)
{
#if DVB2IP_SERVER_USE_STATIC_SCREEN
    app_dvb2ip_play_update_info();
#endif
    return 0;
}

#if DVB2IP_SERVER_USE_STATIC_SCREEN
#define DVB2IP_PLAY_TEXT1   "dvb2ip_play_text1"
#define DVB2IP_PLAY_TEXT2   "dvb2ip_play_text2"
#define DVB2IP_PLAY_TEXT3   "dvb2ip_play_text3"
#define DVB2IP_PLAY_TEXT4   "dvb2ip_play_text4"

static void app_dvb2ip_play_update_info(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    int i = 0, cnt = 0;
    char buf[8] = {0};

    GxCore_MutexLock(arg->mtx);
    if (arg->client)
    {
        for (i = 0; i < DVB2IP_STREAM_MAX_CLIENTS; i++)
        {
            if (arg->client[i].rec_flag)
            {
                ++cnt;
            }
        }
    }
    GxCore_MutexUnlock(arg->mtx);
    snprintf(buf, sizeof(buf), "%d", cnt);
    GUI_SetProperty(DVB2IP_PLAY_TEXT3, "string", buf);
    app_update_pop_dlg(WND_POP_BOOK);
    app_update_pop_dlg(WND_POP_TIP);
}

int app_dvb2ip_play_entry(void)
{
    Dvb2ipArg *arg = &s_dvb2ip_arg;
    PopDlg pop = {0};

    popdlg_destroy();
    pop.type = POP_TYPE_NO_BTN;
    pop.format = POP_FORMAT_DLG;
    pop.mode = POP_MODE_UNBLOCK;
    pop.timeout_sec = 3;
    pop.exit_cb = NULL;

    if (!arg->net_flag)
    {
        pop.str = STR_ID_LINK_NETWORK;
        popdlg_create(&pop);
        return -1;
    }
    if (!(arg->prog_num + arg->fm_num))
    {
        pop.str = STR_ID_NO_PROGRAM;
        popdlg_create(&pop);
        return -1;
    }
    app_dvb2ip_use_flag_set(1);
    if (!arg->run_flag)
    {
        app_dvb2ip_use_flag_set(0);
        pop.str = STR_ID_DVB2IP_START_FAILED;
        popdlg_create(&pop);
        return -1;
    }

    GUI_CreateDialog(WND_DVB2IP_PLAY);
    return 0;
}

static int _dvb2ip_exit_pop_cb(PopDlgRet ret)
{
    if (ret == POP_VAL_OK)
    {
        GUI_EndDialog(WND_DVB2IP_PLAY);
    }
    return 0;
}

SIGNAL_HANDLER  int app_dvb2ip_play_create(GuiWidget *widget, void *usrdata)
{
    GUI_SetProperty(DVB2IP_PLAY_TEXT1, "string", STR_ID_DVB2IP_SERVER);
    GUI_SetProperty(DVB2IP_PLAY_TEXT2, "string", STR_ID_DVB2IP_CLIENT);
    GUI_SetProperty(DVB2IP_PLAY_TEXT4, "string", STR_ID_DVB2IP_EXIT_TIP);
    app_dvb2ip_play_update_info();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_dvb2ip_play_destroy(GuiWidget *widget, void *usrdata)
{
    app_dvb2ip_use_flag_set(0);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_dvb2ip_play_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(event->type == GUI_KEYDOWN)
    {
        switch(event->key.sym)
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_MENU:
            case STBK_EXIT:
                {
                    PopDlg pop = {0};
                    pop.type = POP_TYPE_YES_NO;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_DVB2IP_SURE_STOP;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.exit_cb = _dvb2ip_exit_pop_cb;
                    popdlg_create(&pop);
                }
                break;
            default:
                break;
        }
    }

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_dvb2ip_play_got_focus(GuiWidget *widget, void *usrdata)
{
    app_dvb2ip_play_update_info();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER  int app_dvb2ip_play_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

int app_dvb2ip_play_hotplug_msg_proc(GxMessage *msg)
{
    GxMsgProperty_NetDevIpcfg ip_cfg;

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_PLUG_OUT:
            {
                memset(&ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
                app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &ip_cfg.dev_name);
                if(ip_cfg.dev_name[0] == 0)
                {
                    GUI_EndDialog(WND_DVB2IP_PLAY);
                }
            }
            break;
    }
    return EVENT_TRANSFER_STOP;
}



#endif

#endif

