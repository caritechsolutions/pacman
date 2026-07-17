#include "gxcore.h"
#include "app_module.h"
#include "gxhotplug.h"
#include "app_default_params.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "app_utility.h"
#include "file_edit.h"
#include "module/app_psi.h"
#include "app_audio.h"
#include "app_dvbs_sat_opt.h"
#include "channel_common.h"
#include "module/utf8-validate.h"
#if PDMX_SUPPORT
#include "app_pdmx.h"
#endif

#define CHECK_ALLOC(p, size)  do {          \
    if (!(p)) p = GxCore_Calloc(size, 1);   \
    if (!(p)) return GXCORE_ERROR;          \
} while(0)
#define FREE_NULL(p) do { if (p) GxCore_Free(p); p = NULL; } while(0)
#define GET_PVR_DIR(partition, fname)  strncpy(partition, fname, strlen(fname)-PVR_DIR_STR_LEN)

#define PVR_PLAYER              PLAYER_FOR_REC
#define PVR_URL_LEN             PLAYER_URL_LONG
#define PVR_PATITION_SIZE_GATE  150    //MINSPACE (MB)
#define TMS_FILE_KEY            "pvr>tms_file"

typedef enum {
    NO_SECTION = 0,
    ON_SECTION = 3
} SectionNums;
#if UNICABLE_SUPPORT
extern int app_unicable_param_check(GxBusPmDataSat sat);
#endif
static handle_t s_pvr_mon_sem = -1;
static handle_t s_pvr_mon_thread = -1;
static int s_pvr_mon_enable = 0;
static event_list* s_pvr_mon_timer = NULL;
static int s_pvr_patition_pencent = 0;

#ifdef LINUX_OS
// 获取指定目录下是否存在"GXPVR"(忽略大小写)文件夹的名字并保存
status_t app_get_pvr_dir_name(const char *path)
{
    char *cmdline = NULL;
    char *p = NULL;
    char temp_buf[100] = {0};
    int len = 0;
    FILE *read_fp = NULL;
    status_t ret = GXCORE_ERROR;

    if((path == NULL) || (strlen(path) == 0))
        return GXCORE_ERROR;

    len = strlen(path) + 100;
    CHECK_ALLOC(cmdline, len);
    strcat(cmdline, "find ");
    strncat(cmdline, path, p - path);
    strcat(cmdline, " -maxdepth 1 -type d -name [Gg][Xx][Pp][Vv][Rr]");

    read_fp = popen(cmdline, "r");
    if (read_fp != NULL)
    {
        if(fread(temp_buf, 1, 99, read_fp) > 0)
        {
            p = strrchr(temp_buf, '/');
            if(p == NULL)
            {
                pclose(read_fp);
                FREE_NULL(cmdline);
                return GXCORE_ERROR;
            }
            GxBus_ConfigSet(PVR_TRUE_DIR_NAME_KEY, p);
            ret = GXCORE_SUCCESS;
        }
        pclose(read_fp);
    }
    FREE_NULL(cmdline);

    return ret;
}
#endif

int pvr_partition_get(HotplugPartition *partition)
{
    int i;
    char partition_path[PVR_DEV_STR_LEN+1] = {0};
    HotplugPartitionList *phpl = NULL;

    phpl = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (!phpl || phpl->partition_num < 1)
        return -1;
    GxBus_ConfigGet(PVR_PARTITION_KEY, partition_path, PVR_DEV_STR_LEN, PVR_PARTITION);
    for(i = 0; i < phpl->partition_num; i++)
    {
        if(strcmp(partition_path, phpl->partition[i].dev_name) == 0)
            break;
    }
    if(i >= phpl->partition_num)
    {
        i = 0;
        GxBus_ConfigSet(PVR_PARTITION_KEY, phpl->partition[i].dev_name);
    }
    /* eg: ecos partition = {
     * partition_name = "USB (C:)", partition_entry = "/mnt/usb01", dev_name = "/dev/usb0/1",
     * fs_type = "fatfs", disk_id = 67
     * } */
    if (partition)
        memcpy(partition,  &phpl->partition[i], sizeof(HotplugPartition));

    return 0;
}

// return: 0, failed; >0, path len
uint32_t pvr_partition_and_path_get(HotplugPartition *partition, char *path, uint32_t max_len)
{
    uint32_t path_len;
    char dir_name[PVR_DIR_STR_LEN+1] = {0};
    HotplugPartition partition_data;

    if (!path || max_len == 0)
        return 0;
    if (!partition)
    {
        memset(&partition_data, 0, sizeof(partition_data));
        partition = &partition_data;
    }
    if (pvr_partition_get(partition) < 0)
        return 0;

#ifdef LINUX_OS
    //获取真实的GxPvr目录的名字（主要解决存在的目录名字大小写的差异问题）
    if(app_get_pvr_dir_name(partition->partition_entry) == GXCORE_ERROR)
        strcpy(dir_name, PVR_DIR_STR);
    else
        GxBus_ConfigGet(PVR_TRUE_DIR_NAME_KEY, dir_name, PVR_DIR_STR_LEN, PVR_TRUE_DIR_NAME);
#else
    strcpy(dir_name, PVR_DIR_STR);
#endif

    path_len = strlen(partition->partition_entry) + strlen(dir_name);
    if (path_len > max_len)
        return 0;
    snprintf(path, max_len, "%s%s",partition->partition_entry, dir_name);

    return path_len;
}

static uint32_t pvr_path_get(char *path, uint32_t max_len)
{
    return pvr_partition_and_path_get(NULL, path, max_len);
}

static uint32_t pvr_dev_get(char *dev, uint32_t max_len)
{
    uint32_t dev_len;
    char *p = NULL;
    HotplugPartition partition;

    if (!dev || max_len == 0)
        return 0;
    memset(&partition, 0, sizeof(partition));
    if (pvr_partition_get(&partition) < 0)
        return 0;

    dev_len = strlen(partition.dev_name);
    if (dev_len > max_len)
        return 0;

    strncpy(dev, partition.dev_name, dev_len);
    p = dev + dev_len - 1;

    //remove partition num
    if(*p >= '0' && *p <= '9')
    {
        *p = '\0';
        dev_len--;
    }

    return partition.disk_id;
}

static uint32_t pvr_url_get(char *url, uint32_t max_len)
{
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_GetUrl url_get;
    char ts_path[30] = {0};
    char dmx_path[16] = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
        return 0;
    if(url == NULL)
        return 0;

    url_get.prog_info = node.prog_data;
    url_get.size = PVR_URL_LEN;
    url_get.url = (int8_t*)url;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_GET_URL, (void *)(&url_get)))
        return 0;

#if (QUICK_SWITCH_SUPPORT == 0)
    app_set_fre_zero(url);
#endif
    {
#if INDEPEND_RECORD
        GxMsgProperty_NodeByIdGet TpNode = {0};
        AppFrontend_SetTp params = {0};

        TpNode.node_type = NODE_TP;
        TpNode.id = node.prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

        params.fre = TpNode.tp_data.frequency;
        params.symb =TpNode.tp_data.tp_c.symbol_rate;
        params.qam = TpNode.tp_data.tp_c.modulation;
        params.type = FRONTEND_DVB_C;
        params.tune_mode = DVBC_TUNE_MODE;
        app_set_tp(1, &params, SET_TP_AUTO);
        uint16_t ts_src = 2;
        uint16_t demxid = 1;
#else
        uint16_t ts_src = g_AppPlayOps.normal_play.ts_src;
        uint16_t demxid = 0;
#endif
#if CA_SUPPORT
        int time = app_tsd_time_get();
        if(time > 0)
        {
            memset(ts_path,0,sizeof(ts_path));
            sprintf(ts_path, "&tsbuff:%d",time);
            strcat(url, ts_path);
        }
        /**
         * 此处，需要保证PVR使用的URL中的DEMUX和TS SOURCE和普通播放保持一致，无论是TSD模式和是普通模式。
         * 如果录制使用的TS SOURCE和普通播放URL使用的一样，那必须要使用普通播放URL的TS SOURCE。
         * 该处理是配合PLAYER中的数据记录，虽然在TSD模式下，PLAYER会使用两个TS SOOURCE，但是记录的会是实际DEMOD数据输入到DEMUX的TS SOURCE。
         * 换言，目前PLAYER中，通过DEMUX ID和TS SOURCE来判断是否要开启多路录制设备以及是否要单独申请DEMUX过滤资源。
         * 如果只有单独录制功能开启，那此处改与不改是没有影响的，但是当方案上已经开启时移，并调用了转录制的功能后，那此处的TS DOURCE就很关键：
         * 如果不保持和普通播放一致，那PLAYER就会开启多一个录制设备，多浪费2MB多的内存空间；
         * 并且会增加USB的负担，时移和录制的文件都会写入USB设备，系统负担会增大，USB的效率会成为问题。
         **/
        //app_demux_param_adjust(&ts_src, &demxid, 1);
#endif
        memset(ts_path,0,sizeof(ts_path));
        sprintf(ts_path, "&tsid:%d", ts_src);// need to keep the same ts source with the normal play
        sprintf(dmx_path, "&dmxid:%d", demxid);//used demux2
    }
    strcat(url, ts_path);
    strcat(url, dmx_path);
    if (strlen(url) > max_len)
    {
        return 0;
    }

    return strlen(url);
}

static void pvr_time_format(char * file_name)
{
#define ONE_HOUR   (60*60)
    time_t time;
    struct tm ptm;
    char buf[16];

    time = g_AppTime.utc_get(&g_AppTime);
    time = get_display_time_by_timezone(time);

    localtime_r(&time, &ptm);

    memset(buf, 0, 16);
    sprintf(buf, "_%d%02d%02d%02d%02d%02d", ptm.tm_year+1900, ptm.tm_mon+1,
            ptm.tm_mday, ptm.tm_hour, ptm.tm_min, ptm.tm_sec);

    strcat(file_name, buf);
}

#define PVR_TYPE_TS     0
#define PVR_TYPE_PS     1
#define PVR_REC_TYPE()  PVR_TYPE_TS
// ret: prog id
static uint32_t pvr_name_get(char *name, uint32_t max_len, pvr_state state)
{
    int i = 0;
    int len = 0;
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        return 0;
    }
    if(state == PVR_TP_RECORD)
    {
        GxMsgProperty_NodeByIdGet sat_get = {0};
        GxMsgProperty_NodeByIdGet tp_get = {0};
        char sat_record_name[MAX_PROG_NAME] = {0};
        char tp_record_name[PVR_DEV_LEN] = {0};

        sat_get.node_type = NODE_SAT;
        sat_get.id = node.prog_data.sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);

        tp_get.node_type = NODE_TP;
        tp_get.id = node.prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);

        if(strlen(app_get_sat_name(&sat_get.sat_data)) && 1 == utf8_validate_str((uint8_t *)app_get_sat_name(&sat_get.sat_data)))
            snprintf(sat_record_name, (MAX_PROG_NAME - PVR_DEV_LEN), "%s", app_get_sat_name(&sat_get.sat_data));
        else
            snprintf(sat_record_name, (MAX_PROG_NAME - PVR_DEV_LEN), "sat_%d", sat_get.sat_data.id);

#if DEMOD_DVB_S
        if(GXBUS_PM_SAT_S == sat_get.sat_data.type)
        {
            char polar_char = (tp_get.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H) ? 'H' : 'V';
            snprintf(tp_record_name, PVR_DEV_LEN, "_%d_%c_%d", tp_get.tp_data.frequency, polar_char, tp_get.tp_data.tp_s.symbol_rate);
        }
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
        if(GXBUS_PM_SAT_DVBT2 == sat_get.sat_data.type)
        {
            char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M", "6M"};
            if(tp_get.tp_data.tp_dvbt2.bandwidth < BANDWIDTH_AUTO)
                snprintf(tp_record_name, PVR_DEV_LEN, "_%d_%s", tp_get.tp_data.frequency, bandwidth_str[tp_get.tp_data.tp_dvbt2.bandwidth]);
        }
#endif
#if DEMOD_DVB_C
        if(GXBUS_PM_SAT_C == sat_get.sat_data.type)
        {
            char* modulation_str[5] = {"16QAM", "32QAM", "64QAM", "128QAM", "256QAM"};
            if((tp_get.tp_data.tp_c.modulation >= DVBC_16QAM) && (tp_get.tp_data.tp_c.modulation <= DVBC_256QAM))
                snprintf(tp_record_name, PVR_DEV_LEN, "_%d_%d_%s", tp_get.tp_data.frequency, tp_get.tp_data.tp_c.symbol_rate, modulation_str[tp_get.tp_data.tp_c.modulation-DVBC_16QAM]);
        }
#endif
#if DEMOD_DTMB
        if(GXBUS_PM_SAT_DTMB == sat_get.sat_data.type)
        {
            if(GXBUS_PM_SAT_1501_DTMB == sat_get.sat_data.sat_dtmb.work_mode)
            {
                char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};
                if(tp_get.tp_data.tp_dtmb.symbol_rate < BANDWIDTH_AUTO)
                    snprintf(tp_record_name, PVR_DEV_LEN, "_%d_%s", tp_get.tp_data.frequency, bandwidth_str[tp_get.tp_data.tp_dtmb.bandwidth]);
            }
            else
            {// work in dvbc mode
                char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};

                if((tp_get.tp_data.tp_dtmb.modulation >= DVBC_16QAM) && (tp_get.tp_data.tp_dtmb.modulation <= DVBC_256QAM))
                    snprintf(tp_record_name, PVR_DEV_LEN, "_%d_%d_%s", tp_get.tp_data.frequency, tp_get.tp_data.tp_dtmb.symbol_rate, modulation_str[tp_get.tp_data.tp_dtmb.modulation-DVBC_16QAM]);
            }
        }
#endif
        snprintf(name, max_len, "%s%s", sat_record_name, tp_record_name);
    }
    else
    {
        if(strlen((char  *)node.prog_data.prog_name) && 1 == utf8_validate_str(node.prog_data.prog_name))
            memcpy(name, node.prog_data.prog_name, MAX_PROG_NAME);
        else
            snprintf(name, max_len, "CH%d", (node.prog_data.tp_id << 16) | node.prog_data.service_id);
    }

    len = strlen(name);
    while(i < len)
    {
        if(name[i] == '/' || name[i] == '?')
            name[i] = '-';
        i++;
    }

    pvr_time_format(name);
    if (PVR_REC_TYPE() == PVR_TYPE_TS)
        strcat(name, PVR_FIX_SIFFIX);

    return node.prog_data.id;
}

static int pvr_dev_size_get(const char *partition_path, uint64_t *free_space, int *percent)
{
    GxDiskInfo disk_info = {0};

    if (!partition_path)
        return -1;
    if (GxCore_DiskInfo(partition_path, &disk_info) == GXCORE_ERROR)
        return -1;
    if (disk_info.total_size == 0 || disk_info.total_size < disk_info.used_size)
        return -2;

    if (free_space)
    {
        *free_space = disk_info.total_size - disk_info.used_size;
        *free_space /= SIZE_UNIT_M;
    }
    if (percent)
    {
        *percent = (disk_info.used_size*100)/disk_info.total_size;
    }

    return 0;
}

int app_pvr_get_free_space(AppPvrOps *pvr)
{
    char env_path[PVR_PATH_LEN] = {0};
    char partition_path[PVR_ENTRY_LEN] = {0};
    uint64_t free_space = 0;

    if (pvr_path_get(env_path, PVR_PATH_LEN) == 0)
        return 0;

    GET_PVR_DIR(partition_path, env_path);
    if (pvr_dev_size_get(partition_path, &free_space, NULL) < 0)
        return 0;
    if(free_space <=  PVR_PATITION_SIZE_GATE)
        return 0;

    free_space -= PVR_PATITION_SIZE_GATE;
    free_space = (free_space > 4096) ? 4096 : free_space;
    return free_space;
}

static int32_t pvr_percent_set(AppPvrOps *pvr)
{
    char partition_path[PVR_ENTRY_LEN] = {0};
    int32_t percent = 0;

    if(pvr->env.path != NULL)
    {
        GET_PVR_DIR(partition_path, pvr->env.path);
        if (pvr_dev_size_get(partition_path, NULL, &percent) == 0)
            return percent;
    }
    return -1;
}

static int32_t pvr_percent_get(AppPvrOps *pvr)
{
    return s_pvr_patition_pencent;
}

void pvr_percent_init(void)
{
    char env_path[PVR_PATH_LEN] = {0};
    char partition_path[PVR_ENTRY_LEN] = {0};
    if (pvr_path_get(env_path, PVR_PATH_LEN) == 0)
    {
        app_log_error("tms usb error!!!!!!!!!!!!\n");
        return;
    }
    GET_PVR_DIR(partition_path, env_path);
    pvr_dev_size_get(partition_path, NULL, &s_pvr_patition_pencent);
}

static usb_check_state pvr_usb_check(AppPvrOps* pvr)
{
    int32_t ret = 0;
    char env_path[PVR_PATH_LEN] = {0};
    char partition_path[PVR_ENTRY_LEN] = {0};
    uint64_t free_space = 0;
    usb_check_state state = USB_OK;

    if (pvr_path_get(env_path, PVR_PATH_LEN) == 0)
        return USB_ERROR;
    GET_PVR_DIR(partition_path, env_path);

    ret = pvr_dev_size_get(partition_path, &free_space, NULL);
    if(ret == -1)
        return USB_ERROR;
    else if (ret == -2)
        return USB_NO_SPACE;
    else if (free_space <= PVR_PATITION_SIZE_GATE)
        return USB_NO_SPACE;

    if (GxCore_FileExists(env_path) == GXCORE_FILE_UNEXIST && GxCore_Mkdir(env_path) < 0)
        return USB_ERROR;
    if (!check_usb_write_permissions(env_path))
        return USB_READ_ONLY;

    return state;
}

static status_t pvr_env_sync(AppPvrOps* pvr)
{
    if (pvr == NULL)  return GXCORE_ERROR;

    pvr->env_clear(pvr);

    app_log_flow("[sync] 1\n");
    // pvr file path
    CHECK_ALLOC(pvr->env.path, PVR_PATH_LEN);
    if (pvr_path_get(pvr->env.path, PVR_PATH_LEN) == 0)
    {
        FREE_NULL(pvr->env.path);
        return GXCORE_ERROR;
    }

    app_log_flow("[sync] 2\n");
    if (GxCore_FileExists(pvr->env.path) == GXCORE_FILE_UNEXIST)
    {
        if(GxCore_Mkdir(pvr->env.path) < 0)
        {
            FREE_NULL(pvr->env.path);
            return GXCORE_ERROR;
        }
    }

    app_log_flow("[sync] 3\n");
    // pvr url
    CHECK_ALLOC(pvr->env.url, PVR_URL_LEN);
    if (0 == pvr_url_get(pvr->env.url, PVR_URL_LEN))
    {
        FREE_NULL(pvr->env.path);
        FREE_NULL(pvr->env.url);
        return GXCORE_ERROR;
    }

    app_log_flow("[sync] 4\n");
    // pvr rec prog id
    CHECK_ALLOC(pvr->env.name, PVR_NAME_LEN);
    pvr->env.prog_id = pvr_name_get(pvr->env.name, PVR_NAME_LEN, pvr->state);

    CHECK_ALLOC(pvr->env.dev, PVR_DEV_LEN);
    pvr_dev_get(pvr->env.dev, PVR_DEV_LEN);

    app_log_flow("[sync] 5\n");
    return GXCORE_SUCCESS;
}

static void pvr_env_clear(AppPvrOps* pvr)
{
    if (pvr == NULL)  return;
    // sync env
    pvr->env.prog_id = 0;
    FREE_NULL(pvr->env.name);
    FREE_NULL(pvr->env.path);
    FREE_NULL(pvr->env.url);
    FREE_NULL(pvr->env.dev);
}

static bool s_pvr_patition_full_state = false;
static void _pvr_monitor(void* arg)
{
    pvr_state state = PVR_DUMMY;
    AppPvrOps *pvr = (AppPvrOps*)arg;
    char partition_path[PVR_ENTRY_LEN] = {0};
    uint64_t free_space = 0;
    state = pvr->state;

    while(1)
    {
        GxCore_SemWait(s_pvr_mon_sem);
        if(s_pvr_mon_enable == 0)
            break;

        if(pvr->env.path != NULL)
        {
            GET_PVR_DIR(partition_path, pvr->env.path);
            pvr_dev_size_get(partition_path, &free_space, &s_pvr_patition_pencent);
        }
        int section_record = 0;
        GxBus_ConfigGetInt(PVR_SECTIONRECORD_KEY, &section_record, PVR_SECTIONRECORD_FLAG);
        if(!((pvr->state == PVR_TIMESHIFT || (state == PVR_TIMESHIFT && pvr->state == PVR_TMS_AND_REC)) && section_record == 1))
        {
            if(free_space <= PVR_PATITION_SIZE_GATE)
            {
                s_pvr_patition_full_state = true;
                break;
            }
        }
    }
}

extern void app_pvr_patition_error_proc(char* str_tip);
static int pvr_mon_timer_cb(void *userdata)
{
    if(s_pvr_mon_thread > 0 && !s_pvr_patition_full_state)
    {
        GxCore_SemPost(s_pvr_mon_sem);
    }
    else
    {
        APP_TIMER_REMOVE(s_pvr_mon_timer);
        app_pvr_patition_error_proc(STR_ID_NO_SPACE);
        s_pvr_patition_pencent = 0;
    }

    return 0;
}

static status_t pvr_monitor_start(AppPvrOps *pvr)
{
    if(s_pvr_mon_thread <= 0)
    {
        s_pvr_patition_pencent = 0;
        s_pvr_mon_enable = 1;
        s_pvr_patition_pencent = pvr_percent_set(pvr);
        GxCore_SemCreate(&s_pvr_mon_sem, 1);
        GxCore_ThreadCreate("app_pvr_mon_proc", &s_pvr_mon_thread, _pvr_monitor, (void*)pvr, 10*1024, GXOS_DEFAULT_PRIORITY);
        s_pvr_patition_full_state = false;
        APP_TIMER_ADD(s_pvr_mon_timer, pvr_mon_timer_cb, 2000, TIMER_REPEAT);
    }

    return GXCORE_SUCCESS;
}

static status_t pvr_monitor_stop(void)
{
    if(s_pvr_mon_thread > 0)
    {
        APP_TIMER_REMOVE(s_pvr_mon_timer);

        s_pvr_mon_enable = 0;
        GxCore_SemPost(s_pvr_mon_sem);
        GxCore_ThreadJoin(s_pvr_mon_thread);
        s_pvr_mon_thread = -1;
        GxCore_SemDelete(s_pvr_mon_sem);
        s_pvr_mon_sem = -1;
        s_pvr_patition_pencent = 0;
    }

    return GXCORE_SUCCESS;
}

status_t _pvr_pat_ts_pack(uint16_t prog_id, AppTsData *ts_data)
{
    char *pat_data = NULL;
    int data_len = 0;
    status_t ret = GXCORE_ERROR;

    if(ts_data == NULL)
        return ret;

    pat_data = app_pat_generate(&prog_id, 1);
    if(pat_data != NULL)
    {
        data_len = ((pat_data[1] & 0x0f) << 8) | pat_data[2];
        data_len += 3;
        ret = app_ts_pack(0, pat_data, data_len, ts_data);
        GxCore_Free(pat_data);
        pat_data = NULL;
    }

    return ret;
}


status_t _pvr_pmt_ts_pack(uint16_t prog_id, AppTsData *ts_data)
{
    char *pmt_data = NULL;
    int data_len = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    status_t ret = GXCORE_ERROR;

    if(ts_data == NULL)
        return ret;

    PrivateProgInfo pvrlock;
    DescriptorInfo descriptorinfo;

    memset(&descriptorinfo, 0, sizeof(DescriptorInfo));
    node.node_type = NODE_PROG;
    node.id = prog_id;
    if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
    {
        pvrlock.lock_flag = node.prog_data.lock_flag;
        descriptorinfo.tag = PMTINFO_PRIVATE_TAG;
        descriptorinfo.length = sizeof(PrivateProgInfo);
        descriptorinfo.data = (void*)&pvrlock;
        if((pmt_data = app_pmt_generate(prog_id, &descriptorinfo)) != NULL)
        {
            data_len = ((pmt_data[1] & 0x0f) << 8) | pmt_data[2];
            data_len += 3;
            ret = app_ts_pack(node.prog_data.pmt_pid, pmt_data, data_len, ts_data);
            GxCore_Free(pmt_data);
            pmt_data = NULL;
        }
    }
    return ret;
}

static PlayerMediaContent app_get_audio_track_type(int audio_pid)
{
    PlayerMediaContent ret = PLAYER_MEDIA_AUDIO;
#if AUDIO_DESCRIPTOR_SUPPORT
    int i = 0;
    PlayerProgInfo *player_info = app_player_prog_info_get();
    GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, player_info);
    for(i = 0; i< player_info->audio_num; i++)
    {
        if(player_info->audio[i].id == audio_pid)
        {
            ret = (player_info->audio[i].track_type == AUDIO_DESCRIPTOR_TRACK ? PLAYER_MEDIA_AUDIO_DESC : PLAYER_MEDIA_AUDIO);
            break;
        }
    }
#else
    ret = PLAYER_MEDIA_AUDIO;
#endif
    return ret;
}

void app_player_record_config_get(int prog_id, PlayerRecordConfig *config)
{
    int i = 0;
    int cnt = 0;
    int k;
    AppTsData ts_data = {0};
    AudioLang_t *audio_list = NULL;
    GxMsgProperty_NodeByPosGet node = {0};

    config->ext_data.have_pmt = 1;
    _pvr_pat_ts_pack(prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(config->ext_data.userdata + config->ext_data.userdata_len, ts_data.pack_data[i], 188);
        config->ext_data.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);

    _pvr_pmt_ts_pack(prog_id, &ts_data);
    for(i = 0; i < ts_data.pack_num; i++)
    {
        memmove(config->ext_data.userdata + config->ext_data.userdata_len, ts_data.pack_data[i], 188);
        config->ext_data.userdata_len += 188;
    }
    app_ts_data_free(&ts_data);

#if 0
    app_log_debug("config->ext_data.userdata_len = %d\n", config->ext_data.userdata_len);
    for(i = 0; i < config->ext_data.userdata_len; i++)
    {
        app_log_debug("%02x ", config->ext_data.userdata[i]);
        if(i % 16 == 15)
            app_log_debug("\n");
    }
#endif

    ////other audio pid
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        config->ext_info.ext_num = 0;
        audio_list = app_get_audio_lang();
        if((audio_list != NULL) && (audio_list->audioLangList != NULL) && (audio_list->audioLangCnt != 0))
        {
            for(i = 0; i < audio_list->audioLangCnt; i++)
            {

                config->ext_info.ext_types[cnt] = DEMUX_SLOT_AUDIO;
                config->ext_info.ext_pids[cnt++] = audio_list->audioLangList[i].id;
                k = config->ext_info.ext_num;
                config->ext_info.ext_pids_content[k] = app_get_audio_track_type(audio_list->audioLangList[i].id);
                config->ext_info.ext_pids_content_type[k] = audio_list->audioLangList[i].audiotype;
                memcpy(&config->ext_info.ext_pids_lang[k],audio_list->audioLangList[i].name,(sizeof(config->ext_info.ext_pids_lang[k])-1));
                config->ext_info.ext_num++;
                //app_log_debug("---[%s] id = %d, num = %d ---\n", __func__, config->ext_info.ext_pids[i], config->ext_info.ext_num);
            }
        }
    }

#ifdef COLOR_16BIT_TTX_SUPPORT
    for(i = 0; i < g_AppTtxSubt.ttx_num + g_AppTtxSubt.subt_num; i++)
#else
        for(i = g_AppTtxSubt.ttx_num; i < g_AppTtxSubt.ttx_num + g_AppTtxSubt.subt_num; i++)
#endif
        {
            config->ext_info.ext_types[cnt] = DEMUX_SLOT_PES;
            config->ext_info.ext_pids[cnt++] = g_AppTtxSubt.content[i].elem_pid;
            k = config->ext_info.ext_num;
#ifdef COLOR_16BIT_TTX_SUPPORT
            if(i < g_AppTtxSubt.ttx_num)
                config->ext_info.ext_pids_content_type[k] = PLAYER_SUB_TYPE_DVB_TTX;
            else
#endif
                config->ext_info.ext_pids_content_type[k] = PLAYER_SUB_TYPE_DVB;
            config->ext_info.ext_pids_content[k] = PLAYER_MEDIA_SUB;
            config->ext_info.ext_pids_major[k] = g_AppTtxSubt.content[i].subt.composite_page_id;
            config->ext_info.ext_pids_minor[k] = g_AppTtxSubt.content[i].subt.ancillary_page_id;
            memcpy(&config->ext_info.ext_pids_lang[k],g_AppTtxSubt.content[i].lang,sizeof(g_AppTtxSubt.content[i].lang));
            config->ext_info.ext_num++;
        }
#if PARENTAL_LOCK_SUPPORT
    config->ext_info.ext_types[cnt] = DEMUX_SLOT_PSI;
    config->ext_info.ext_pids[cnt++] = EIT_PID;
    config->ext_info.ext_num++;
#endif
}

static void pvr_rec_config(AppPvrOps *pvr, const char *player)
{
    PlayerRecordConfig *config = NULL;

    if((pvr == NULL) || (player == NULL))
        return;

    config = (PlayerRecordConfig*)GxCore_Calloc(1, sizeof(PlayerRecordConfig));
    if(config == NULL)
        return;

    app_player_record_config_get(pvr->env.prog_id, config);
    GxPlayer_MediaRecordConfig(player, config);

    GxCore_Free(config);
    config = NULL;
}

static void pvr_rec_start(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerRecord* rec_start = NULL;
    GxTime tick;
#if UNICABLE_SUPPORT || MULTISTREAM_SUPPORT
    char ts_dmx[80] = {0};
#endif
    if (pvr == NULL)
        return;

    if(pvr->state != PVR_TIMESHIFT)
    {
        PlayerPVRConfig pvrConfig = {0};
        GxPlayer_GetPVRConfig(&pvrConfig);
        if(pvrConfig.volume_sizemb != PVR_FILE_SIZE_VALUE)
        {
            pvrConfig.volume_sizemb = PVR_FILE_SIZE_VALUE;
        }
        pvrConfig.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pvrConfig);
    }

    // player open
    rec_start = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerRecord));
    if(rec_start == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return ;
    }

    if (pvr->state == PVR_DUMMY)
    {
        if (pvr->env_sync(pvr) != GXCORE_SUCCESS
                || app_player_open(PVR_PLAYER) != PLAYER_OPEN_OK)
        {
            pvr->env_clear(pvr);
            GXCORE_FREE(rec_start);
            return;
        }

        pvr_monitor_start(pvr);
        rec_start->player = PVR_PLAYER;
        strcpy((char*)rec_start->url, pvr->env.url);
        sprintf((char*)rec_start->file, "%s/%s", pvr->env.path, pvr->env.name);
        GxMsgProperty_NodeByIdGet tp_get = {0};
        GxMsgProperty_NodeByPosGet prog_node = {0};

        prog_node.node_type = NODE_PROG;
        prog_node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));

        tp_get.node_type = NODE_TP;
        tp_get.id = prog_node.prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);

#if UNICABLE_SUPPORT
        {
            uint32_t centre_frequency=0 ;
            uint32_t lnb_index=0x20 ;
            uint32_t set_tp_req=0;
            GxMsgProperty_NodeByIdGet sat_get = {0};
            sat_get.node_type = NODE_SAT;
            sat_get.id = prog_node.prog_data.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);

            if(app_unicable_param_check(sat_get.sat_data))
            {
                lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data,&set_tp_req,&centre_frequency);
                sprintf(ts_dmx, "&ifindex:%d&lnbindex:%d&centrefre:%d&iffre:%d",sat_get.sat_data.sat_s.unicable_para.if_channel,\
                        lnb_index, centre_frequency,sat_get.sat_data.sat_s.unicable_para.centre_fre[sat_get.sat_data.sat_s.unicable_para.if_channel]);
                strcat((char *)rec_start->url, ts_dmx);
            }
        }
#endif

#if MULTISTREAM_SUPPORT
        memset(ts_dmx, 0, sizeof(ts_dmx));
        sprintf(ts_dmx, "&pls_num:%d",tp_get.tp_data.tp_s.pls_n);
        strcat((char*)rec_start->url, ts_dmx);
        {
            memset(ts_dmx, 0, sizeof(ts_dmx));
            GxMsgProperty_NodeByPosGet prog_node = {0};
            prog_node.node_type = NODE_PROG;
            prog_node.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));
            if(prog_node.prog_data.muti_ts_exist == 1)
            {
                sprintf(ts_dmx, "&pls_ts_id:%d&pls_code:%d&pls_change:%d&pls_status:%d",
                        prog_node.prog_data.muti_ts_id,
                        prog_node.prog_data.muti_ts_code,
                        0xff,
                        prog_node.prog_data.muti_ts_exist);
                strcat((char*)rec_start->url, ts_dmx);
            }
        }
#endif

        pvr_rec_config(pvr, PLAYER_FOR_REC);
#if AUTO_UPDATE_SUPPORT
        app_auto_update_stop();
#endif
#if BOUQUET_SUPPORT
        g_AppBat.stop(&g_AppBat);
#endif
        if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)rec_start))
        {
            app_player_close(PVR_PLAYER);
        }

        GxCore_GetTickTime(&tick);
        pvr->time.cur_tick = 0;
        pvr->time.total_tick = 0;
        pvr->time.remaining_tick = 0;
        pvr->enter_shift   = 0;

        pvr->state = PVR_RECORD;

        app_fuse_spec_pvr_file_info_update(pvr->env.path, pvr->env.name);
    }
    else if (pvr->state == PVR_TIMESHIFT)
    {
        if (app_player_open(PVR_PLAYER) == PLAYER_OPEN_OK) //for logic
        {
            rec_start->player = PVR_PLAYER;
            strcpy((char*)rec_start->url, pvr->env.url);
            sprintf((char*)rec_start->file, "%s/%s", pvr->env.path, pvr->env.name);
            if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)rec_start))
            {
                app_player_close(PVR_PLAYER);
            }
            else
            {
                pvr->state = PVR_TMS_AND_REC;
                GxBus_ConfigSet(TMS_FILE_KEY, "abc");

                app_fuse_spec_pvr_file_info_update(pvr->env.path, pvr->env.name);
            }
        }
    }
    GXCORE_FREE(rec_start);

    // play list lock current tp
#if CA_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
    if(pvr->env.prog_id && (pvr->state == PVR_RECORD))
    {
        int demux_id = 0;
        GxMsgProperty_NodeByIdGet node = {0};

        app_getvalue(pvr->env.url,"dmxid",&demux_id);
        if(app_tsd_check() > 0)
        {
            if(g_AppPlayOps.normal_play.dmx_id == demux_id)// use the same demux with the normal play,can not release the source
                return ;
        }
        node.node_type = NODE_PROG;
        node.id = pvr->env.prog_id;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        {
            app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_PVR);
        }
    }
#endif
#endif
}

static void pvr_tp_rec_start(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerRecord* rec_start = NULL;
    PlayerRecordConfig *config = NULL;
    GxTime tick;

    if (pvr == NULL)
        return;
    // player open
    if (pvr->state != PVR_DUMMY)
        return;
    config = (PlayerRecordConfig*)GxCore_Calloc(sizeof(PlayerRecordConfig), 1);
    if(config == NULL)
        return;

    pvr->state = PVR_TP_RECORD;
    if (GXCORE_SUCCESS != pvr->env_sync(pvr))
    {
        pvr->state = PVR_DUMMY;
        pvr->env_clear(pvr);
        return;
    }

    if (app_player_open(PVR_PLAYER) != PLAYER_OPEN_OK)
    {
        pvr->state = PVR_DUMMY;
        pvr->env_clear(pvr);
        return;
    }

    config->ext_info.is_rawts = 1;
    GxPlayer_MediaRecordConfig(PLAYER_FOR_REC, config);
    GxCore_Free(config);
    config = NULL;

    rec_start = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerRecord));
    if(rec_start == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return ;
    }


    pvr_monitor_start(pvr);
    rec_start->player = PVR_PLAYER;
    strcpy((char*)rec_start->url, pvr->env.url);
    sprintf((char*)rec_start->file, "%s/%s", pvr->env.path, pvr->env.name);

#if MULTISTREAM_SUPPORT
    GxMsgProperty_NodeByIdGet tp_get = {0};
    char ts_dmx[80] = {0};
    GxMsgProperty_NodeByPosGet prog_node = {0};

    prog_node.node_type = NODE_PROG;
    prog_node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node));

    tp_get.node_type = NODE_TP;
    tp_get.id = prog_node.prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);

    memset(ts_dmx, 0, sizeof(ts_dmx));
    sprintf(ts_dmx, "&pls_num:%d",tp_get.tp_data.tp_s.pls_n);
    strcat((char*)rec_start->url, ts_dmx);
    memset(ts_dmx, 0, sizeof(ts_dmx));
    if(prog_node.prog_data.muti_ts_exist == 1)
    {
        sprintf(ts_dmx, "&pls_ts_id:%d&pls_code:%d&pls_change:%d&pls_status:%d",
                prog_node.prog_data.muti_ts_id,
                prog_node.prog_data.muti_ts_code,
                0xff,
                prog_node.prog_data.muti_ts_exist);
        strcat((char*)rec_start->url, ts_dmx);
    }
#endif

    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)rec_start))
    {
        app_player_close(PVR_PLAYER);
    }
    GXCORE_FREE(rec_start);

    GxCore_GetTickTime(&tick);
    pvr->time.cur_tick = 0;
    pvr->time.total_tick = 0;
    pvr->time.remaining_tick = 0;
}

static status_t _pvr_tplist_rec_env_sync(AppPvrOps* pvr)
{
    //pvr file path
    CHECK_ALLOC(pvr->env.path, PVR_PATH_LEN);
    if (0 == pvr_path_get(pvr->env.path, PVR_PATH_LEN))
    {
        FREE_NULL(pvr->env.path);
        return GXCORE_ERROR;
    }
    if (GxCore_FileExists(pvr->env.path) == GXCORE_FILE_UNEXIST)
    {
        if(GxCore_Mkdir(pvr->env.path)< 0)
        {
            FREE_NULL(pvr->env.path);
            return GXCORE_ERROR;
        }
    }

    CHECK_ALLOC(pvr->env.dev, PVR_DEV_LEN);
    pvr_dev_get(pvr->env.dev, PVR_DEV_LEN);

    return GXCORE_SUCCESS;
}

static void pvr_tplist_rec_start(AppPvrOps *pvr)
{
    GxMsgProperty_FrontendSetTp Param = {0};
    uint32_t CurTuner = 0;
    GxMsgProperty_PlayerRecord* rec_start = NULL;
    PlayerRecordConfig *config = NULL;
    AppFrontend_Config frontend_config;
    GxTime tick;
    char url[2048] = {0};

    if (pvr == NULL)
        return;
    // player open
    if (pvr->state != PVR_DUMMY)
        return;

    config = (PlayerRecordConfig*)GxCore_Calloc(sizeof(PlayerRecordConfig), 1);
    if(config == NULL)
        return;

    pvr->state = PVR_TP_RECORD;
    if (GXCORE_SUCCESS != _pvr_tplist_rec_env_sync(pvr))
    {
        pvr->state = PVR_DUMMY;
        pvr->env_clear(pvr);
        return;
    }

    if (app_player_open(PVR_PLAYER) != PLAYER_OPEN_OK)
    {
        pvr->state = PVR_DUMMY;
        pvr->env_clear(pvr);
        return;
    }

    config->ext_info.is_rawts = 1;
    GxPlayer_MediaRecordConfig(PLAYER_FOR_REC, config);
    GxCore_Free(config);
    config = NULL;

    rec_start = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerRecord));
    if(rec_start == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return ;
    }

    pvr_monitor_start(pvr);
    rec_start->player = PVR_PLAYER;

    memset(url, 0, 2048);

    CurTuner = app_tuner_cur_tuner_get();
    app_ioctl(CurTuner, FRONTEND_CONFIG_GET, &frontend_config);
    GxFrontend_GetCurFre(CurTuner,&Param);
    if(Param.type == FRONTEND_DVB_T2)
    {
        sprintf(url,"dvbt2://fre:%d&bandwidth:%d&workmode:%d&vpid:0&apid:0&pcrpid:0&vcodec:0&acodec:0&tuner:%d&scramble:0&pmt:0&tsid:%d&data_plp_id:%d&common_plp_exist:%d&common_plp_id:%d",
                Param.fre,
                Param.bandwidth,
                Param.type_1501,
                Param.data_plp_id,
                CurTuner,
                frontend_config.ts_src,
                Param.common_plp_exist,
                Param.common_plp_id);
        sprintf((char*)rec_start->file, "%s/dvbt2_%d_TPRecord%s",pvr->env.path,Param.fre,PVR_FIX_SIFFIX);
    }
    else if(Param.type == FRONTEND_DVB_T)
    {
        sprintf(url,"dvbt://fre:%d&bandwidth:%d&vpid:0&apid:0&pcrpid:0&vcodec:0&acodec:0&tuner:%d&scramble:0&pmt:0&tsid:%d",Param.fre,Param.bandwidth,CurTuner,frontend_config.ts_src);
        sprintf((char*)rec_start->file, "%s/dvbt_%d_TPRecord%s",pvr->env.path,Param.fre,PVR_FIX_SIFFIX);
    }
    else if(Param.type == FRONTEND_DVB_C)
    {
        sprintf(url,"dvbc://fre:%d&qam:%d&symbol:%d&vpid:0&apid:0&pcrpid:0&vcodec:0&acodec:0&tuner:%d&scramble:0&pmt:0&tsid:%d",Param.fre,Param.qam,Param.symb,CurTuner,frontend_config.ts_src);
        sprintf((char*)rec_start->file, "%s/dvbc_%d_%dqam_%d_TPRecord%s",pvr->env.path,Param.fre,Param.qam,Param.symb,PVR_FIX_SIFFIX);
    }
    else if(Param.type == FRONTEND_DVB_S || Param.type == FRONTEND_DVB_S2)
    {
 #if (DEMOD_DVB_S > 0)
        const char *polar_str = NULL;
        unsigned int sel = 0;
        sprintf(url,"dvbs://fre:%d&polar:%d&symbol:%d&22k:%d&vpid:1&apid:1&pcrpid:1&tuner:%d&pmt:10&tsid:%d&dmxid:%d&pls_num:%d",
                s_dvbs_sat_data.cur_sat.sat_data.sat_s.lnb1-s_dvbs_sat_data.cur_tp.tp_data.frequency,
                s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar,
                s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate,
                s_dvbs_sat_data.cur_sat.sat_data.sat_s.switch_22K,
                s_dvbs_sat_data.cur_sat.sat_data.tuner,
                frontend_config.ts_src,
                frontend_config.dmx_id,
                s_dvbs_sat_data.cur_tp.tp_data.tp_s.pls_n);
        sel = s_dvbs_sat_data.cur_tp.tp_data.tp_s.polar;
        polar_str = (0 == sel) ? "H" : "V";
        sprintf((char*)rec_start->file, "%s/%s_%d%s%d_TPRecord%s",
                pvr->env.path, app_get_sat_name(&s_dvbs_sat_data.cur_sat.sat_data),
                s_dvbs_sat_data.cur_tp.tp_data.frequency, polar_str,
                s_dvbs_sat_data.cur_tp.tp_data.tp_s.symbol_rate, PVR_FIX_SIFFIX);
#endif
    }
    else if(Param.type == FRONTEND_DTMB)
    {
        sprintf(url,"dtmb://fre:%d&bandwidth:%d&qam:%d&symbol:%d&workmode:%d&vpid:0&apid:0&pcrpid:0&vcodec:0&acodec:0&tuner:%d&scramble:0&pmt:0",
                Param.fre,
                Param.bandwidth,
                Param.qam,
                Param.symb,
                Param.type_1501,
                CurTuner);
        sprintf((char*)rec_start->file, "%s/dvbt_dtmb_%d_%dqam_%d_TPRecord%s",pvr->env.path,Param.fre,Param.qam,Param.symb,PVR_FIX_SIFFIX);

    }
    strcpy((char*)rec_start->url, url);

    //dvbs://fre:1150&polar:1&symbol:27500&22k:1&vpid:256&apid:272&pcrpid:256&vcodec:0&acodec:1&tuner:1&scramble:0&pmt:32&tsid:1&dmxid:0&pls_num:0
    //path /mnt/usb01/GxPvr

    if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PLAYER_RECORD, (void *)rec_start))
    {
        app_player_close(PVR_PLAYER);
        app_log_error("RECORD fail, app_player_close!\n");
    }
    GXCORE_FREE(rec_start);


    app_log_debug("[%s]url:%s\n", __func__, url);

    GxCore_GetTickTime(&tick);
    pvr->time.cur_tick = 0;
    pvr->time.total_tick = 0;
    pvr->time.remaining_tick = 0;
}

static void pvr_rec_stop(AppPvrOps *pvr)
{
    if((pvr->state == PVR_RECORD)
            || (pvr->state == PVR_TMS_AND_REC)
            || (pvr->state == PVR_TP_RECORD))
    {
        app_player_close(PVR_PLAYER);
        pvr_monitor_stop();

        app_fuse_spec_pvr_file_info_save();
    }
#if CA_SUPPORT
    if(pvr->env.prog_id)
    {
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
        app_descrambler_close_by_control_type(CONTROL_PVR);
#if ECM_SUPPORT
        // TODO:20120702
        g_AppEcm2.stop(&g_AppEcm2);
#endif
#endif
    }
#endif

    if(pvr->state == PVR_RECORD)
    {
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
#if AUTO_UPDATE_SUPPORT
        app_auto_update_restart();
#endif
#if BOUQUET_SUPPORT
        g_AppBat.start(&g_AppBat);
#endif
    }
    else if(pvr->state == PVR_TP_RECORD)
    {
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
    }
    else if(pvr->state == PVR_TMS_AND_REC)
    {
        pvr->state = PVR_TIMESHIFT;
    }
}

static void pvr_tms_stop(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerTimeShift time_shift;

    if (pvr == NULL)
        return;

    memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
    pvr->enter_shift   = 0;
    time_shift.enable = 0;
    app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));

    if((pvr->state == PVR_TIMESHIFT)
            ||(pvr->state == PVR_TMS_AND_REC))
    {
        if (pvr->state == PVR_TMS_AND_REC)
        {
            app_fuse_spec_pvr_file_info_save();
        }
        app_player_close(PLAYER_FOR_TIMESHIFT);
        app_player_open(PLAYER_FOR_NORMAL);
    }

    if(pvr->state == PVR_TIMESHIFT)
    {

        PlayerPVRConfig pvrConfig = {0};

        GxPlayer_GetPVRConfig(&pvrConfig);
        if(pvrConfig.volume_sizemb != PVR_FILE_SIZE_VALUE)
        {
            pvrConfig.volume_sizemb = PVR_FILE_SIZE_VALUE;
        }
        pvrConfig.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pvrConfig);
        pvr_monitor_stop();
        pvr->env_clear(pvr);
        pvr->state = PVR_DUMMY;
        //g_AppPmt.start(&g_AppPmt);
        pvr->tms_delete(pvr);
#if AUTO_UPDATE_SUPPORT
        app_auto_update_restart();
#endif
#if BOUQUET_SUPPORT
        g_AppBat.start(&g_AppBat);
#endif
#if CA_SUPPORT
        {
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            app_descrambler_close_by_control_type(CONTROL_PVR);
#if ECM_SUPPORT
            // TODO:20120702
            g_AppEcm2.stop(&g_AppEcm2);
#endif
#endif
        }
#endif
    }
    else if(pvr->state == PVR_TMS_AND_REC)
    {
        PlayerPVRConfig pvrConfig = {0};

        GxPlayer_GetPVRConfig(&pvrConfig);
        if(pvrConfig.volume_sizemb != PVR_FILE_SIZE_VALUE)
        {
            pvrConfig.volume_sizemb = PVR_FILE_SIZE_VALUE;
        }
        pvrConfig.volume_maxnum = 0;
        GxPlayer_SetPVRConfig(&pvrConfig);
        pvr->state = PVR_RECORD;
    }
}

static void pvr_pause(AppPvrOps *pvr)
{
    GxMsgProperty_PlayerPause player_pause;
    GxMsgProperty_PlayerTimeShift time_shift;
    GxTime tick;

    if (pvr == NULL)   return;

    if (pvr->state == PVR_DUMMY)
    {
        // first time init pvr env
        if (GXCORE_SUCCESS != pvr->env_sync(pvr))
        {
            pvr->env_clear(pvr);
            return;
        }
#if AUTO_UPDATE_SUPPORT
        app_auto_update_stop();
#endif
#if BOUQUET_SUPPORT
        g_AppBat.stop(&g_AppBat);
#endif

        int section_record = 0;
        GxMsgProperty_PlayerPVRConfig  pvrConfig = {0};

        GxBus_ConfigGetInt(PVR_SECTIONRECORD_KEY, &section_record, PVR_SECTIONRECORD_FLAG);
        GxPlayer_GetPVRConfig(&pvrConfig);
        if(1 == section_record)
        {
            pvrConfig.volume_sizemb = app_pvr_get_free_space(pvr)/2;
            pvrConfig.volume_maxnum = 2;
        }
        else
        {
            if(pvrConfig.volume_sizemb != PVR_FILE_SIZE_VALUE)
            {
                pvrConfig.volume_sizemb = PVR_FILE_SIZE_VALUE;
            }
            pvrConfig.volume_maxnum = 0;
        }
        GxPlayer_SetPVRConfig(&pvrConfig);
        pvr->state = PVR_TIMESHIFT;
        pvr_monitor_start(pvr);
        pvr_rec_config(pvr, PLAYER_FOR_NORMAL);
        memset(&time_shift, 0, sizeof(GxMsgProperty_PlayerTimeShift));
        time_shift.enable = 1;
        // TODO: for new driver for DEMUX control, DEMX2(id = 1) for PVR timeshift
        time_shift.shift_dmxid = 0;
#if CA_SUPPORT
        {
            uint16_t ts_src = 0;
            uint16_t DemuxId = time_shift.shift_dmxid;
            app_demux_param_adjust(&ts_src, &DemuxId, 1);
            time_shift.shift_dmxid = DemuxId;
        }
#endif
        sprintf((char*)time_shift.url, "%s/%s", pvr->env.path, pvr->env.name);

        app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));

        GxBus_ConfigSet(TMS_FILE_KEY, pvr->env.name);

        GxCore_GetTickTime(&tick);
        pvr->time.cur_tick = 0;
        pvr->time.total_tick = 0;
        pvr->time.remaining_tick = 0;

    }
    else if (pvr->state == PVR_RECORD)
    {
        pvr->state = PVR_TMS_AND_REC;
        time_shift.enable = 1;
        // TODO: for new driver for DEMUX control, DEMX2(id = 1) for PVR timeshift
        time_shift.shift_dmxid = 0;
#if  CA_SUPPORT
        {
            uint16_t ts_src = 0;
            uint16_t DemuxId = time_shift.shift_dmxid;

            app_demux_param_adjust(&ts_src, &DemuxId, 1);
            time_shift.shift_dmxid = DemuxId;
        }
#endif
        sprintf((char*)time_shift.url, "%s/%s", pvr->env.path, pvr->env.name);

        app_send_msg_exec(GXMSG_PLAYER_TIME_SHIFT, (void*)(&time_shift));
    }

    player_pause.player = PLAYER_FOR_NORMAL;
    app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));

#if CA_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
    if(pvr->state == PVR_TIMESHIFT)
    {
        int demux_id = 0;
        GxMsgProperty_NodeByIdGet node = {0};

        app_getvalue(pvr->env.url,"dmxid",&demux_id);
        if(app_tsd_check() > 0)
        {
            if(g_AppPlayOps.normal_play.dmx_id == demux_id)// use the same demux with the normal play,can not release the source
                return ;
        }
        node.node_type = NODE_PROG;
        node.id = pvr->env.prog_id;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_SUCCESS)
        {
            app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_PVR);
        }
    }
#endif
#endif
}

static void pvr_resume(AppPvrOps *pvr)
{
    if (pvr == NULL)
        return;

    pvr->enter_shift   = 1;
    GxMsgProperty_PlayerResume player_resume;
    player_resume.player = PLAYER_FOR_NORMAL;
    app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
}

static void pvr_seek(AppPvrOps *pvr, int64_t offset)
{
#define SEC_TO_MSEL     (1000)
    GxMsgProperty_PlayerSeek seek;

    seek.player = PLAYER_FOR_NORMAL;
    seek.time = offset*SEC_TO_MSEL;
    seek.flag = SEEK_ORIGIN_SET;

    app_send_msg_exec(GXMSG_PLAYER_SEEK, (void *)(&seek));
}

static void pvr_play_speed(AppPvrOps *pvr, float speed)
{
    GxMsgProperty_PlayerSpeed player_speed;

    if (pvr == NULL)
        return;

    player_speed.player = PLAYER_FOR_NORMAL;
    player_speed.speed = (float)speed;
    switch ((int32_t)speed)
    {
        case PVR_SPD_1:  pvr->spd = 0; break;
        case PVR_SPD_2:  pvr->spd = 1; break;
        case PVR_SPD_4:  pvr->spd = 2; break;
        case PVR_SPD_8:  pvr->spd = 3; break;
        case PVR_SPD_16: pvr->spd = 4; break;
        case PVR_SPD_32: pvr->spd = 5; break;
        default:                       break;
    }
    app_log_debug("start speed %f", speed);
    app_send_msg_exec(GXMSG_PLAYER_SPEED, (void *)(&player_speed));
}

#ifdef MODIFY_FATMOUNT
void modify_disk_mount(void)
{
    int status;
    char cmdline[48] = {0};
    char partition_path[PVR_DEV_STR_LEN+1] = {0};
    GxBus_ConfigGet(PVR_PARTITION_KEY, partition_path, PVR_DEV_STR_LEN, PVR_PARTITION);
    sprintf(cmdline,"/home/root/bin/remount_fat.sh %s",partition_path);
    status=system(cmdline);
    status=((status&0xff00)>>8);
    if(status==2)
    {
        app_log_debug("remount udisk!!!\n");
    }
}
#endif

static void pvr_tms_delete(AppPvrOps* pvr)
{
    char file_path[PVR_PATH_LEN] = {0};
    char file_name[PVR_NAME_LEN] = {0};
    char tms_file[PVR_PATH_LEN+PVR_NAME_LEN] = {0};
    char tms_folder[PVR_PATH_LEN+PVR_NAME_LEN] = {0};
    char length;

    if (pvr->state != PVR_DUMMY)
        return;
    GxBus_ConfigGet(TMS_FILE_KEY, file_name, PVR_NAME_LEN-1, "abc");
    if(memcmp("abc", file_name, 3) == 0)
        return;
    if (pvr_path_get(file_path, PVR_PATH_LEN) == 0)
        return;
    if (GxCore_FileExists(file_path) == GXCORE_FILE_UNEXIST)
        return;
    sprintf(tms_file, "%s/%s", file_path, file_name);

    if (GxCore_FileExists(tms_file) == GXCORE_FILE_UNEXIST)
        return;
    GxCore_FileDelete(tms_file);
#ifdef MODIFY_FATMOUNT
    modify_disk_mount();
#endif

    length = strlen(tms_file);
    strncpy(tms_folder, tms_file, length-PVR_FIX_SIFFIX_LEN);
    if (GxCore_FileExists(tms_folder) == GXCORE_FILE_UNEXIST)
        return;
    file_edit_delete(tms_folder);
    GxCore_FsSync(tms_file);
    GxBus_ConfigSet(TMS_FILE_KEY, "abc");
}

int app_pvr_check_change_state(void)
{
    int pdmx_flag = 0;
    int ca_flag = 0;
    int fuse_flag = 0 ;

#if CA_SUPPORT
    ca_flag = app_extend_control_pvr_change();
#endif
#if PDMX_SUPPORT
    pdmx_flag = app_pdmx_get_active_flag();
#endif
    fuse_flag = app_fuse_spec_get_pvr_statue();
    return (ca_flag | pdmx_flag | fuse_flag);
}

pvr_state app_pvr_get_state(void)
{
    pvr_state cur_pvr = PVR_DUMMY;

    if (g_AppPvrOps.state != PVR_DUMMY)
    {
        if(g_AppPvrOps.state == PVR_TP_RECORD)
        {
            cur_pvr = PVR_TP_RECORD;
        }
        else if(g_AppPvrOps.state == PVR_TMS_AND_REC)
        {
            if(app_pvr_check_change_state())
            {
                cur_pvr = PVR_TMS_AND_REC;
            }
            else
            {
                cur_pvr = PVR_TIMESHIFT;
            }
        }
        else if(g_AppPvrOps.state == PVR_RECORD)
        {
            if(app_pvr_check_change_state())
            {
                cur_pvr = PVR_RECORD;
            }
        }
        else
        {
            cur_pvr = PVR_TIMESHIFT;
        }
    }

    return cur_pvr;
}

char *app_pvr_get_tips(pvr_state state)
{
    switch(state)
    {
        case PVR_SPEED:
        case PVR_DUMMY:
            return NULL;

        case PVR_RECORD:
        case PVR_TP_RECORD:
            return STR_ID_STOP_REC_INFO;
        case PVR_TMS_AND_REC:
            return STR_ID_STOP_TMS_AND_REC_INFO;
        case PVR_TIMESHIFT:
            return STR_ID_STOP_TMS_INFO;

        default:
            return NULL;
    }
}

PvrDealWithMode app_pvr_popdlg_handler(ExitCb exit_cb)
{
    PopDlg pop = {0};
    char * tips = NULL;
    pvr_state cur_pvr = app_pvr_get_state();
    tips= app_pvr_get_tips(cur_pvr);

    if(!tips || PVR_DUMMY == cur_pvr)
        return DEALWITH_KEEPON;

    pop.type = POP_TYPE_YES_NO;
    pop.mode = POP_MODE_UNBLOCK;
    pop.str = tips;
    pop.exit_cb = exit_cb;
    popdlg_create(&pop);

    return DEALWITH_STOP;
}

int app_pvr_create_no_btn_popdlg(ExitCb exit_cb)
{
    if(g_AppPvrOps.state != PVR_DUMMY)
    {
        PopDlg pop = {0};
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_STOP_PVR_FIRST;
        pop.mode = POP_MODE_UNBLOCK;
        pop.timeout_sec = 3;
        pop.exit_cb = exit_cb;
        popdlg_create(&pop);
    }

    return 0;
}

AppPvrOps g_AppPvrOps =
{
    .usb_check        =   pvr_usb_check,
    .env_sync         =   pvr_env_sync,
    .env_clear        =   pvr_env_clear,
    .rec_start        =   pvr_rec_start,
    .tp_rec_start     =   pvr_tp_rec_start,
    .tplist_rec_start =   pvr_tplist_rec_start,
    .rec_stop         =   pvr_rec_stop,
    .tms_stop         =   pvr_tms_stop,
    .pause            =   pvr_pause,
    .resume           =   pvr_resume,
    .seek             =   pvr_seek,
    .speed            =   pvr_play_speed,
    .percent          =   pvr_percent_get,
    .tms_delete       =   pvr_tms_delete,
    //.free_space     =   pvr_check_free_space
};

