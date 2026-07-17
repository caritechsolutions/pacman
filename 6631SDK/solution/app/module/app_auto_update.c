#include "app.h"
#include "app_module.h"
#include "app_windows.h"
#include "channel_common.h"
#include "app_channel_info.h"
#include "module/app_frontend.h"
#if AUTO_UPDATE_SUPPORT
#if LCN_AUTO_UPDATE
#include "app_lcn.h"
#endif

#define AUTO_UPDATE_NIT_FINISH 0x06
#define AUTO_UPDATE_SDT_FINISH 0x05
#define AUTO_UPDATE_PAT_FINISH 0x03
#define AUTO_UPDATE_FINISH 0X07
#define MAX_SERVICE_PER_TP 200
#if OTHER_TS_UPDATE_SUPPORT
#define PER_MALLOC_TP_NUM 20
#else
#define PER_MALLOC_TP_NUM 1
#endif
#define UPDATE_MAX_TP_NUM 200
#define AUTO_UPDATE_NO_VALID_VER 32
#define AUTO_UPDATE_TIMEOUT 60
static int _auto_update_update_ts_info(int ts_id,int tp_id);

typedef enum
{
    AUTO_UPDATE_NO_CHANGE = 0,
    AUTO_UPDATE_ADD_PROG,
    AUTO_UPDATE_DEL_PROG
}auto_update_prog_type;

typedef enum
{
    AUTO_UPDATE_SDT_OTHER = 0,
    AUTO_UPDATE_SDT_ACT
}auto_update_sdt_type;

typedef struct auto_update_service_info{
    char is_new_add_in_sdt;
    char is_new_add_in_nit;
    char is_in_flash;
    char is_in_sdt;
    char is_in_pat;
    char is_in_nit;
    char free_ca_mode;
    uint8_t sdt_ver;
    uint8_t pat_ver;
    uint8_t service_type;
    uint8_t common_status;
    uint8_t data_plp_id;
    uint8_t common_plp_exist;
    uint8_t common_plp_id;
    uint16_t pmt_pid;
    uint16_t ts_id;
    uint16_t orig_network_id;
    uint16_t service_id;
    uint16_t lcn;
    char prog_name[MAX_PROG_NAME];
}auto_update_service_info;

typedef struct auto_update_ts_info{
    uint8_t nit_version;
    uint16_t sat_id;
    uint16_t tp_id;
    int ts_id;
    int ts_valid;
    int freq;//KHZ
    int service_num;    //total services maybe not same as in flash
    auto_update_service_info* service_info[MAX_SERVICE_PER_TP];
}auto_update_ts_info;

typedef struct auto_update_control{
    uint8_t pat_id;
    uint8_t nit_id;
    uint8_t sdt_act_id;
    uint8_t sdt_other_id;
    uint8_t pat_ver;
    uint8_t nit_ver;
    uint8_t sdt_ver;
    uint8_t data_plp_id;
    uint8_t common_plp_exist;
    uint8_t common_plp_id;
    uint16_t tp_id;
    uint32_t ts_id;
}auto_update_control;

typedef struct nitservice
{
    uint16_t service_id;
    uint8_t  service_type;
}nit_service_info;

typedef struct nitlcn
{
    uint16_t service_id;
    uint16_t lcn;
    uint8_t visible_flag;
    uint8_t lcn_tag;
}nit_lcn_info;

typedef struct nitts
{
    uint16_t ts_id;
    uint16_t    orig_network_id;
    uint16_t     service_count;
    uint16_t     lcn_count;
    GxBusPmDataTP tp;
    nit_service_info  *service_info;
    nit_lcn_info   *lcn_info;
}nit_ts_info;


typedef struct nitdata
{
    uint8_t     secNum;
    uint8_t      ver;
    uint16_t regionId;
    uint16_t     network_id;             /**< network_id(16) */
    uint16_t     ts_count;               /**< transport stream count  */
    nit_ts_info  *ts_info;               /**< transport stream information array */
}nit_data_info;

typedef struct plp_table
{
    uint8_t     num;
    uint8_t     search_index;
    uint8_t     current_index;
    uint8_t     use_flag;
    struct dvbt_plp_parameters* plp_id;
}plp_table;

static uint8_t auto_update_flag = 0;
static uint8_t add_common_status = 0xff;
static handle_t ts_info_lock = -1;
static int ts_info_num = 0;
static auto_update_ts_info* s_ts_info = NULL;
static auto_update_control s_auto_back_control = {0};
static uint8_t nit_status[256]={0};
static int nit_version_changed;
static int nit_in_update = 0;
static bool update_flag = false;
static plp_table s_plp_table = {0};

extern uint32_t app_play_id_get(void);
extern void app_tv_radio_key_exec(void);
extern void app_epg_preprocess_for_acu(void);
extern void app_epg_reset_for_acu(void);
extern void app_epg_disable(void);
#if BOUQUET_SUPPORT
extern void app_bouquet_backup_refresh(int type, uint32_t pos);
#endif
#if OTHER_TS_UPDATE_SUPPORT
static int _auto_update_update_search_tp(void);
#endif

static void _auto_update_remove_service(auto_update_service_info **service_arry,int *num,int idx);
static void _auto_update_sdt_act_start(uint16_t ts_id,uint8_t ver);
static void _auto_update_pat_start(uint16_t ts_id,uint8_t ver);
static void _auto_update_nit_start(uint16_t ts_id, uint8_t ver);
static void _auto_update_clear_ts_info(void);
static int _auto_update_set_next_plp(void);
static void _auto_update_table_stop(uint8_t *psi_id);
static void _auto_update_set_update_flag(bool flag);
static status_t _auto_update_set_plp(struct dvbt_plp_parameters plp_id);

static event_list *s_check_feinfo_timer = NULL;

static int _auto_update_check_timer(void *userdata)
{
    int32_t ts_lock = -1, tp_lock = -1;
    handle_t frontend = 0;
    uint32_t tuner = 0;
    GxFrontendInfo info = {0};
    if(auto_update_flag != 0)
        return 0;

    tuner = app_tuner_cur_tuner_get();
    frontend = GxFrontend_IdToHandle(app_tuner_cur_tuner_get());
    GxFrontend_GetInfo(frontend, &info);
    if(FE_DVB_T2 != info.type)
        return 0;

    ts_lock = GxFrontend_QueryStatus(tuner);
    tp_lock = GxFrontend_QueryFrontendStatus(tuner);
    if(ts_lock != 1 && tp_lock == 1)
    {
        struct dvbt_plp_table table = { 0 };
        struct dvbt_plp_parameters* get_params = NULL;

        get_params = GxCore_Malloc(256*sizeof(struct dvbt_plp_parameters));
        if(get_params == NULL)
            return 0;

        table.params = get_params;
        if(ioctl(frontend, FE_GET_PLPID, &table) < 0)
        {
            GxCore_Free(get_params);
            return 0;
        }
        if(0 != table.tp_num)
        {
            app_log_min("******[%s] [%d] table.tp_num = %d ts is change muti plp \n",__func__,__LINE__,table.tp_num);
            struct dvbt_plp_parameters plp_id;
            plp_id.data_plp_id = table.params[0].data_plp_id;
            plp_id.common_plp_exist = table.params[0].common_plp_exist;
            plp_id.common_plp_id = table.params[0].common_plp_id;
            _auto_update_set_plp(plp_id);
        }
        GxCore_Free(get_params);
    }
    return 0;
}

static void _auto_update_init_plp_table(void)
{
    if(s_plp_table.plp_id != NULL)
    {
        GxCore_Free(s_plp_table.plp_id);
        s_plp_table.plp_id = NULL;
    }
    memset(&s_plp_table,0,sizeof(plp_table));
}

static status_t _auto_update_set_plp(struct dvbt_plp_parameters plp_id)
{
#if DEMOD_DVB_T > 0
    AppFrontend_SetTp params = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_TP;
    node.id = s_auto_back_control.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node));
    params.fre = node.tp_data.frequency;
    params.bandwidth = node.tp_data.tp_dvbt2.bandwidth;
    params.type = FRONTEND_DVB_T2;
    params.type_1501 = 2;
    params.set_tp_mode = SET_MANUAL_PLP;
    params.data_plp_id = plp_id.data_plp_id;
    params.common_plp_exist = plp_id.common_plp_exist;
    params.common_plp_id = plp_id.common_plp_id;
    s_auto_back_control.data_plp_id = plp_id.data_plp_id;
    s_auto_back_control.common_plp_id = plp_id.data_plp_id;
    s_auto_back_control.common_plp_exist = plp_id.common_plp_exist;
    app_set_tp(app_tuner_cur_tuner_get(), &params, SET_TP_AUTO);
#endif
    return GXCORE_SUCCESS;
}

static void _plp_timeout_dis_thread(void* arg)
{
    uint8_t index = s_plp_table.search_index;
    unsigned long tick = 0;
    GxCore_ThreadDetach();
    tick = GxCore_TickStart(10000);
    do
    {
        if(index != s_plp_table.search_index || auto_update_flag == 0)
            return;
        GxCore_ThreadDelay(200);
    }while(!GxCore_TickEnd(tick));

    if(_auto_update_set_next_plp() != 0)
        return;
    app_log_error("timeout index = %d s_plp_table.search_index = %d flag = %d\n",index,s_plp_table.search_index,auto_update_flag);
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
    _auto_update_update_ts_info(-1,s_auto_back_control.tp_id);
#else
    _auto_update_update_ts_info(s_auto_back_control.ts_id,s_auto_back_control.tp_id);
#endif
    auto_update_flag = 0;
    _auto_update_clear_ts_info();
    if(s_plp_table.num != 0)
        _auto_update_init_plp_table();

    _auto_update_set_update_flag(false);
    return;
}

static int _auto_get_current_plpindex(void)
{
    int i = 0;
    for(i = 0; i < s_plp_table.num; i ++)
    {
        if(s_auto_back_control.data_plp_id == s_plp_table.plp_id[i].data_plp_id
                && s_auto_back_control.common_plp_id == s_plp_table.plp_id[i].common_plp_id
                && s_auto_back_control.common_plp_exist == s_plp_table.plp_id[i].common_plp_exist)
        {
            return i;
        }
    }
    return 0;
}

static void _auto_update_get_plp(void)
{

    handle_t frontend = 0;
    struct dvbt_plp_table table = { 0 };
    struct dvbt_plp_parameters* get_params = NULL;
    GxFrontendInfo info = {0};

    if(s_plp_table.use_flag == 1)
        return;

    s_plp_table.use_flag = 1;

    frontend = GxFrontend_IdToHandle(app_tuner_cur_tuner_get());
    GxFrontend_GetInfo(frontend, &info);
    if(FE_DVB_T2 != info.type)
    {
        if(FE_DVB_T == info.type)
            add_common_status = 1;
        else
            add_common_status = 0xff;
       return;
    }
    else
        add_common_status = 2;

    get_params = GxCore_Malloc(256*sizeof(struct dvbt_plp_parameters));
    if(get_params == NULL)
        return;

    table.params = get_params;
    if(ioctl(frontend, FE_GET_PLPID, &table) < 0)
    {
        GxCore_Free(get_params);
        return;
    }
    if(s_plp_table.num != table.tp_num)
    {
        if(ts_info_num != 0)
            _auto_update_clear_ts_info();
        s_plp_table.num = table.tp_num;
        s_plp_table.search_index = 0;
        if(s_plp_table.plp_id != NULL)
        {
            GxCore_Free(s_plp_table.plp_id);
            s_plp_table.plp_id = NULL;
        }
        s_plp_table.plp_id = GxCore_Malloc(table.tp_num*sizeof(struct dvbt_plp_parameters));
        if(s_plp_table.plp_id == NULL)
        {
            GxCore_Free(get_params);
            _auto_update_init_plp_table();
            return;
        }
        memcpy(s_plp_table.plp_id,get_params,table.tp_num*sizeof(struct dvbt_plp_parameters));
        s_plp_table.current_index = _auto_get_current_plpindex();
        s_plp_table.search_index = s_plp_table.current_index;
    }
    GxCore_Free(get_params);
}


static int _auto_update_set_next_plp(void)
{
    if(s_plp_table.num <= 1 || s_plp_table.plp_id == NULL)
        return 0;

    app_log_min("search_index = %d  current_index = %d s_plp_table.num = %d\n",s_plp_table.search_index,s_plp_table.current_index,s_plp_table.num);
    if(s_plp_table.search_index != 0 && s_plp_table.search_index == s_plp_table.current_index)
    {
        s_plp_table.search_index = 0;
    }
    else
    {
        s_plp_table.search_index++;
        if(s_plp_table.search_index == s_plp_table.current_index)
            s_plp_table.search_index++;
    }

    if(s_plp_table.search_index >= s_plp_table.num)
        return 0;
    if(_auto_update_set_plp(s_plp_table.plp_id[s_plp_table.search_index]) == GXCORE_SUCCESS)
    {
        static int thread_id = 0;
        GxCore_ThreadCreate("plp_timeout_judgment",&thread_id, _plp_timeout_dis_thread, NULL, 8 * 1024, GXOS_DEFAULT_PRIORITY);
        auto_update_flag = ((~(AUTO_UPDATE_PAT_FINISH & AUTO_UPDATE_SDT_FINISH)) & AUTO_UPDATE_FINISH);
        s_auto_back_control.sdt_ver = AUTO_UPDATE_NO_VALID_VER;
        _auto_update_sdt_act_start(s_auto_back_control.ts_id,AUTO_UPDATE_NO_VALID_VER);
        s_auto_back_control.pat_ver = AUTO_UPDATE_NO_VALID_VER;
        _auto_update_pat_start(s_auto_back_control.ts_id,AUTO_UPDATE_NO_VALID_VER);
        return 1;
    }
    return 0;
}

static void _auto_update_set_update_flag(bool flag)
{
    if(update_flag == false && flag == true)
    {
        update_flag = true;
        app_auto_update_send_ui_msg(ACU_UPDATE_START, AUTO_UPDATE_TIMEOUT, AUTO_UPDATE_SHOW_INFO);
    }
    else if(update_flag == true && flag == false)
    {
        update_flag = false;
        app_auto_update_send_ui_msg(ACU_UPDATE_END,0,NULL);
    }
}

static auto_update_service_info *_auto_update_service_info_malloc(void)
{
    auto_update_service_info* ServiceInfo = GxCore_Mallocz(sizeof(auto_update_service_info));
    if(ServiceInfo == NULL)
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
    return ServiceInfo;
}

static void _auto_update_service_info_free(auto_update_service_info* ServiceInfo)
{
    if(ServiceInfo != NULL)
    {
        GxCore_Free(ServiceInfo);
        ServiceInfo = NULL;
    }
}

static void _auto_update_ts_info_lock_create(void)
{
    if(ts_info_lock == -1)
        GxCore_MutexCreate(&ts_info_lock);
}

static void _auto_update_ts_info_lock_delete(void)
{
    if(ts_info_lock == -1)
    {
        return;
    }
    GxCore_MutexDelete(ts_info_lock);
    ts_info_lock = -1;
}

static void _auto_update_ts_info_lock(void)
{
    if(ts_info_lock == -1)
    {
        return;
    }
    GxCore_MutexLock(ts_info_lock);
}

static void _auto_update_ts_info_unlock(void)
{
    if(ts_info_lock == -1)
    {
        return;
    }
    GxCore_MutexUnlock(ts_info_lock);
}

static void _auto_update_remove_service(auto_update_service_info **service_arry,int *num,int idx)
{
    int i = 0;
    if((idx < 0)||(idx > *num))  //if idx == num means del last one
    {
        return;
    }
    _auto_update_service_info_free(service_arry[idx]);
    *num = *num-1;

    for(i=idx;i<*num;i++)
    {
        service_arry[i]=service_arry[i+1];
    }
    service_arry[i] = NULL;
}

static int _auto_update_bulid_list_add_prog(int tp_index,int prog_index,GxBusPmDataProg Prog)
{
    s_ts_info[tp_index].service_info[prog_index] = _auto_update_service_info_malloc();
    if(s_ts_info[tp_index].service_info[prog_index] == NULL)
    {
        return -1;
    }
    s_ts_info[tp_index].service_info[prog_index]->lcn = Prog.pos;
    s_ts_info[tp_index].service_info[prog_index]->ts_id = Prog.ts_id;
    s_ts_info[tp_index].service_info[prog_index]->service_id = Prog.service_id;
    s_ts_info[tp_index].service_info[prog_index]->service_type = Prog.service_type;
    s_ts_info[tp_index].service_info[prog_index]->sdt_ver = Prog.sdt_version;
    s_ts_info[tp_index].service_info[prog_index]->orig_network_id = Prog.original_id;
    s_ts_info[tp_index].service_info[prog_index]->pat_ver = Prog.pat_version;
    s_ts_info[tp_index].service_info[prog_index]->pmt_pid = Prog.pmt_pid;
    s_ts_info[tp_index].service_info[prog_index]->common_status = Prog.common_status;
    s_ts_info[tp_index].service_info[prog_index]->data_plp_id = Prog.data_plp_id;
    s_ts_info[tp_index].service_info[prog_index]->common_plp_exist = Prog.common_plp_exist;
    s_ts_info[tp_index].service_info[prog_index]->common_plp_id = Prog.common_plp_id;
    s_ts_info[tp_index].service_info[prog_index]->is_in_flash = 1;
    s_ts_info[tp_index].service_info[prog_index]->is_in_sdt = 0;
    s_ts_info[tp_index].service_info[prog_index]->is_in_nit = 0;
    s_ts_info[tp_index].service_info[prog_index]->is_in_pat = 0;
    s_ts_info[tp_index].service_info[prog_index]->is_new_add_in_nit = 0;
    s_ts_info[tp_index].service_info[prog_index]->is_new_add_in_sdt = 0;
    strcpy(s_ts_info[tp_index].service_info[prog_index]->prog_name,(char*)Prog.prog_name);
    s_ts_info[tp_index].service_num++;
    return 0;
}

static int _auto_update_realloc_ts_info(void)
{
    auto_update_ts_info* ts_info = NULL;
    if(ts_info_num%PER_MALLOC_TP_NUM == 0 && ts_info_num !=0)
    {
        ts_info = GxCore_Realloc(s_ts_info,sizeof(auto_update_ts_info)*(ts_info_num+PER_MALLOC_TP_NUM));
        if(ts_info == NULL)
        {
            app_log_error("malloc s_ts_info is failed!!!!!!!!!\n");
            return -1;
        }
        s_ts_info = ts_info;
        memset(&(s_ts_info[ts_info_num]),0,sizeof(auto_update_ts_info)*PER_MALLOC_TP_NUM);
    }
    return 0;
}

static int _auto_update_build_sat_tp_info(int sat_id)
{
    GxBusPmDataTP  Tp = {0};
	GxBusPmDataProg *prog_arry = NULL;
    int i = 0;
    int j = 0;
    int tp_num;
    int prog_num;

    tp_num = GxBus_PmTpNumGetBySat(sat_id);
    if(tp_num <= 0)
    {
        app_log_error("no tp in this sat n");
        return -1;
    }
    if(tp_num > UPDATE_MAX_TP_NUM)
    {
        app_log_error("program service is to much%d\n",tp_num);
        return -1;
    }
    s_ts_info = GxCore_Mallocz(sizeof(auto_update_ts_info)*PER_MALLOC_TP_NUM);
    if(s_ts_info == NULL)
    {
        app_log_error("malloc s_ts_info is failed!!!!!!!!!\n");
        return -1;
    }
    _auto_update_ts_info_lock();

    for(i = 0;i < tp_num; i++)
    {
        GxBus_PmTpGetByPosInSat(i, 1, &Tp);
#if !OTHER_TS_UPDATE_SUPPORT
        if(s_auto_back_control.tp_id != Tp.id)
        {
            continue;
        }
#endif
		prog_num = GxBus_PmProgNumGetByTpId(Tp.id);
		if(prog_num <= 0)
        {
            if(_auto_update_realloc_ts_info() != 0)
                goto error_1;
            s_ts_info[ts_info_num].sat_id = Tp.sat_id;
            s_ts_info[ts_info_num].tp_id = Tp.id;
            s_ts_info[ts_info_num].ts_id = -1;
            s_ts_info[ts_info_num].freq = Tp.frequency;
            s_ts_info[ts_info_num].nit_version = AUTO_UPDATE_NO_VALID_VER;
            s_ts_info[ts_info_num].service_num = 0;
            ts_info_num++;
#if OTHER_TS_UPDATE_SUPPORT
            continue;
#else
            break;
#endif
        }
		if(prog_num > MAX_SERVICE_PER_TP)
        {
            app_log_error("program service is to much%d\n",prog_num);
            goto error_1;
        }
		prog_arry = GxCore_Malloc(prog_num * sizeof(GxBusPmDataProg));
		if(prog_arry == NULL)
		{
			app_log_error("%s mallpc failed!!!\n",__LINE__);
			goto error_1;
		}
		memset(prog_arry, 0, prog_num * sizeof(GxBusPmDataProg));
		GxBus_PmProgGetByTpId(Tp.id, prog_num, prog_arry);
        if(_auto_update_realloc_ts_info() != 0)
            goto error_2;
        for(j = 0; j < prog_num; j++)
        {
            if(_auto_update_bulid_list_add_prog(ts_info_num, j, prog_arry[j]) == -1)
            {
                app_log_error("program service malloc failed!!!!!!!!!!\n");
                ts_info_num++;
                goto error_2;
            }
        }
        s_ts_info[ts_info_num].ts_valid = 1;
        s_ts_info[ts_info_num].sat_id = Tp.sat_id;
        s_ts_info[ts_info_num].ts_id = prog_arry[0].ts_id;
        s_ts_info[ts_info_num].tp_id = Tp.id;
        s_ts_info[ts_info_num].freq = Tp.frequency;
        s_ts_info[ts_info_num].nit_version = s_auto_back_control.nit_ver;
        ts_info_num++;
        APP_FREE(prog_arry);
#if !OTHER_TS_UPDATE_SUPPORT
        break;
#endif
    }
    _auto_update_ts_info_unlock();
    return 0;
error_2:
    APP_FREE(prog_arry);
error_1:
    _auto_update_ts_info_unlock();
    return -1;
}

static void _auto_update_clear_ts_info(void)
{
    app_log_debug("_auto_update_clear_ts_info is clear!!!!!!!!!!!!\n");
    int i = 0;
    int j = 0;
    _auto_update_ts_info_lock();
    for(i=0;i<ts_info_num;i++)
    {
        for(j=0;j<s_ts_info[i].service_num;j++)
        {
            _auto_update_service_info_free(s_ts_info[i].service_info[j]);
        }
        memset(&s_ts_info[i],0,sizeof(auto_update_ts_info));
        s_ts_info[i].ts_id = -1;
    }
    ts_info_num = 0;
    GxCore_Free(s_ts_info);
    s_ts_info = NULL;
    _auto_update_ts_info_unlock();
    _auto_update_ts_info_lock_delete();
}

static status_t _auto_update_build_ts_info(void)
{
    _auto_update_clear_ts_info();
    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_TP;
    node.id = s_auto_back_control.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node));
    _auto_update_ts_info_lock_create();
    app_log_debug("_auto_update_clear_ts_info is build!!!!!!!!!!!!\n");
    if(_auto_update_build_sat_tp_info(node.tp_data.sat_id) == 0)
    {
        return GXCORE_SUCCESS;
    }
    else
    {
        return GXCORE_ERROR;
    }
}

static void _auto_update_finish_check(uint8_t mode,int ts_id, int version, int tp_id)
{
    if(mode == AUTO_UPDATE_NIT_FINISH)
    {
         if((auto_update_flag&1) != 1)
             return;
    }
    if((auto_update_flag + mode == AUTO_UPDATE_FINISH)&& app_get_auto_update_flag() == 1)
    {
        if(s_plp_table.num != 0 &&_auto_update_set_next_plp() != 0)
            return;
        if(mode == AUTO_UPDATE_NIT_FINISH)
        {
            _auto_update_update_ts_info(-1,-1);
#if OTHER_TS_UPDATE_SUPPORT
            _auto_update_update_search_tp();
#endif
        }
        else
        {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
            _auto_update_update_ts_info(-1,tp_id);
#else
            _auto_update_update_ts_info(ts_id,tp_id);
#endif
        }
    }
    auto_update_flag &= mode;
    if(mode == AUTO_UPDATE_SDT_FINISH)
    {
        s_auto_back_control.ts_id = ts_id;
        s_auto_back_control.sdt_ver = version;
    }
    else if(mode == AUTO_UPDATE_PAT_FINISH)
    {
        s_auto_back_control.pat_ver = version;
        s_auto_back_control.ts_id = ts_id;
    }
    if(auto_update_flag == 0)
    {
        if(s_plp_table.num != 0)
            _auto_update_init_plp_table();
        _auto_update_clear_ts_info();
        _auto_update_set_update_flag(false);
    }
}

static bool _auto_update_check_ts_info_is_build(uint8_t flag)
{
    if(ts_info_num == 0)
    {
        if(flag == AUTO_UPDATE_PAT_FINISH || flag == AUTO_UPDATE_SDT_FINISH)
        {

           if(_auto_update_build_ts_info() == GXCORE_ERROR)
               return false;
#if OTHER_TS_UPDATE_SUPPORT
            auto_update_flag = ((~(AUTO_UPDATE_PAT_FINISH & AUTO_UPDATE_SDT_FINISH & AUTO_UPDATE_NIT_FINISH)) & AUTO_UPDATE_FINISH);
#else
            auto_update_flag = ((~(AUTO_UPDATE_PAT_FINISH & AUTO_UPDATE_SDT_FINISH )) & AUTO_UPDATE_FINISH);
#endif
            if(flag == AUTO_UPDATE_PAT_FINISH)
            {
                s_auto_back_control.sdt_ver = AUTO_UPDATE_NO_VALID_VER;
                _auto_update_sdt_act_start(s_auto_back_control.ts_id,AUTO_UPDATE_NO_VALID_VER);
            }
            else if(flag == AUTO_UPDATE_SDT_FINISH)
            {
                s_auto_back_control.pat_ver = AUTO_UPDATE_NO_VALID_VER;
                _auto_update_pat_start(s_auto_back_control.ts_id,AUTO_UPDATE_NO_VALID_VER);
            }
            s_auto_back_control.nit_ver = AUTO_UPDATE_NO_VALID_VER;
#if OTHER_TS_UPDATE_SUPPORT
            _auto_update_nit_start(s_auto_back_control.ts_id,AUTO_UPDATE_NO_VALID_VER);
#endif
            return true;
        }
        return false;
    }
    else
    {
        if((((auto_update_flag & (~AUTO_UPDATE_PAT_FINISH)) == 0) && flag == AUTO_UPDATE_PAT_FINISH)
            || (((auto_update_flag & (~AUTO_UPDATE_SDT_FINISH)) == 0) && flag == AUTO_UPDATE_SDT_FINISH))
        {
#if OTHER_TS_UPDATE_SUPPORT
            auto_update_flag = ((~(AUTO_UPDATE_PAT_FINISH & AUTO_UPDATE_SDT_FINISH & AUTO_UPDATE_NIT_FINISH)) & AUTO_UPDATE_FINISH);
#else
            auto_update_flag = ((~(AUTO_UPDATE_PAT_FINISH & AUTO_UPDATE_SDT_FINISH )) & AUTO_UPDATE_FINISH);
#endif
        }
        app_log_debug("auto_update_flag = %d \n",auto_update_flag);
        return true;
    }
}

#if OTHER_TS_UPDATE_SUPPORT
static int _auto_update_find_index_of_ts_info(int freq,int ts_id,int *flag)
{
    int i = 0;

    for(i=0;i<ts_info_num;i++)
    {
        if(s_ts_info[i].freq == freq)
        {
            if((s_ts_info[i].ts_id == ts_id)||(s_ts_info[i].ts_id == -1))
            {
                *flag = 0;
                return i;
            }
            else
            {
                *flag = -2;
                return i;
            }
        }
    }
    return -1;
}

static void _auto_update_remove_ts(auto_update_ts_info *tp_arry,int *num,int idx)
{
    int i = 0;
    int j = 0;
    if((idx < 0)||(idx > *num))  //if idx == num means del last one
    {
        return;
    }
    for(j=0;j<tp_arry[idx].service_num;j++)
    {
        _auto_update_service_info_free(tp_arry[idx].service_info[j]);
    }
    *num = *num-1;

    for(i=idx;i<*num;i++)
    {
        tp_arry[i]=tp_arry[i+1];
    }
    memset(&(tp_arry[i]),0,sizeof(auto_update_ts_info));
}

static int _auto_update_check_tp_valid_reset(void)
{
    int i =0;

    _auto_update_ts_info_lock();

    for(i=0;i<ts_info_num;i++)
    {
        s_ts_info[i].ts_valid = 0;
    }
    _auto_update_ts_info_unlock();
    return 0;
}


static int _auto_update_update_search_tp(void)
{
    int i = 0;
    int num = 0;
    int tp_deleted = 0;
    _auto_update_ts_info_lock();
    num = ts_info_num;
    for(i=(num -1);i>=0;i--)
    {
        if(s_ts_info[i].ts_valid == 0)
        {
            uint32_t tp_id = 0;
            tp_id = s_ts_info[i].tp_id;
            GxBus_PmTpDelete(&tp_id, 1);
            tp_deleted = 1;
            _auto_update_remove_ts(s_ts_info,&ts_info_num,i);
        }
    }
    if(tp_deleted == 1)
    {
        GxBus_PmSync(GXBUS_PM_SYNC_TP);
    }
    _auto_update_ts_info_unlock();
    return 0;
}

static int _auto_update_add_ts_info(GxBusPmDataSatType type,GxBusPmDataTP tp, int ts_id)
{
    int ts_index = 0;
    int exsit_flag = 0;
    int ret = 0;
    uint16_t tpid_temp = 0;
    uint32_t tpid_del = 0;
    GxBusPmDataProg *prog_arry = NULL;

    _auto_update_ts_info_lock();

    if( ts_info_num >= UPDATE_MAX_TP_NUM)
    {
        app_log_error("program service is to much ts_info_num=%d\n",ts_info_num);
        goto error_1;
    }

    ts_index = _auto_update_find_index_of_ts_info(tp.frequency,ts_id,&exsit_flag);
    if(ts_index != -1)
    {
        s_ts_info[ts_index].ts_id = ts_id;
        s_ts_info[ts_index].ts_valid = 1;
        if(exsit_flag == -2)
        {
            _auto_update_set_update_flag(true);
            app_log_debug("ts_index=%d ts %d %d id changed\n",ts_index,tp.frequency,ts_id);
            tpid_del = s_ts_info[ts_index].tp_id;
            tp.id = tpid_del;
            GxBus_PmTpModify(&tp);
            GxBus_PmSync(GXBUS_PM_SYNC_TP);
            s_ts_info[ts_index].tp_id = tp.id;
            _auto_update_ts_info_unlock();
            return -2;
        }
        else
        {
            app_log_debug("ts_index=%d  is no changed\n",ts_index);
            _auto_update_ts_info_unlock();
            return 0;
        }
    }
    else
    {
        if(type == GXBUS_PM_SAT_C)
        {
            ret = GxBus_PmTpExistChek(tp.sat_id, tp.frequency, tp.tp_c.symbol_rate, 0, tp.tp_c.modulation,&tpid_temp);
        }
        else if(type == GXBUS_PM_SAT_DVBT2)
        {
            ret = GxBus_PmTpExistChek(tp.sat_id, tp.frequency, 0, 0, 0,&tpid_temp);
        }
        else if( type == GXBUS_PM_SAT_DTMB)
        {
            ret  = GxBus_PmTpExistChek(tp.sat_id, tp.frequency, tp.tp_dtmb.symbol_rate, 0, tp.tp_dtmb.modulation,&tpid_temp);
        }
        else if (type == GXBUS_PM_SAT_S )
        {
            ret  = GxBus_PmTpExistChek(tp.sat_id, tp.frequency, tp.tp_s.symbol_rate, tp.tp_s.polar, 0,&tpid_temp);
        }
        if(ret == 0)
        {
            _auto_update_set_update_flag(true);
            ret = GxBus_PmTpAdd(&tp);
            if (GXCORE_SUCCESS != ret)
            {
                app_log_error("can't add Tp\n");
                goto error_1;
           }
            GxBus_PmSync(GXBUS_PM_SYNC_TP);
            if(_auto_update_realloc_ts_info() != 0)
                goto error_1;
            s_ts_info[ts_info_num].service_num = 0;
        }
        else
        {
            int j = 0;
            int prog_num = 0;
			GxBusPmDataProg *prog_arry = NULL;

			prog_num = GxBus_PmProgNumGetByTpId(tpid_temp);
			prog_arry = GxCore_Malloc(prog_num * sizeof(GxBusPmDataProg));
			if(prog_arry == NULL)
			{
				app_log_error("%s mallpc failed!!!\n",__LINE__);
                goto error_1;
			}
			memset(prog_arry, 0, prog_num * sizeof(GxBusPmDataProg));
			GxBus_PmProgGetByTpId(tpid_temp, prog_num, prog_arry);
            if(_auto_update_realloc_ts_info() != 0)
                goto error_2;
			for(j = 0; j < prog_num; j++)
            {
                if(_auto_update_bulid_list_add_prog(ts_info_num, j, prog_arry[j]) == -1)
                {
                    ts_info_num++;
                    app_log_error("program service malloc failed!!!!!!!!!!\n");
                    goto error_2;
                }
            }
            s_ts_info[ts_info_num].tp_id = tpid_temp;
			APP_FREE(prog_arry);
        }
        s_ts_info[ts_info_num].ts_valid = 1;
        s_ts_info[ts_info_num].ts_id = ts_id;
        s_ts_info[ts_info_num].sat_id = tp.sat_id;
        s_ts_info[ts_info_num].freq = tp.frequency;
        s_ts_info[ts_info_num].nit_version = AUTO_UPDATE_NO_VALID_VER;
        ts_info_num++;
        _auto_update_ts_info_unlock();
        return 0;
    }
error_2:
     APP_FREE(prog_arry);
error_1:
    _auto_update_ts_info_unlock();
    return -1;

}

static int _auto_update_check_ts_service_nit(int ts_id,int tp_id, auto_update_service_info s_info)
{
    int i= 0;
    int j=0;
    _auto_update_ts_info_lock();

    for(i=0;i<ts_info_num;i++)
    {
        if((s_ts_info[i].ts_id == ts_id || ts_id == -1)
                &&((s_ts_info[i].tp_id == tp_id)||(tp_id == -1)))
        {
            for(j=0;j<s_ts_info[i].service_num;j++)
            {
                if(s_ts_info[i].service_info[j]->service_id == s_info.service_id && s_ts_info[i].service_info[j]->ts_id == s_info.ts_id)
                {
                    if(s_ts_info[i].service_info[j]->is_new_add_in_sdt == 1)
                    {
                        s_ts_info[i].service_info[j]->is_new_add_in_nit = 1;
                    }
                    s_ts_info[i].service_info[j]->is_in_nit = 1;
                    s_ts_info[i].service_info[j]->orig_network_id = s_info.orig_network_id;
                    s_ts_info[i].service_info[j]->lcn = s_info.lcn;
                    _auto_update_ts_info_unlock();
                    return 1;
                }
            }
            if(j == s_ts_info[i].service_num)  //new service
            {
                if(s_ts_info[i].service_num >= MAX_SERVICE_PER_TP)
                {
                    app_log_error("service full \n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }
                s_ts_info[i].service_info[j] = _auto_update_service_info_malloc();
                if(s_ts_info[i].service_info[j] == NULL)
                {
                    app_log_error(" s_ts_info[i].service_info[s_ts_info[i].service_num] service malloc failed!!!!!!!!!!\n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }

                s_ts_info[i].service_info[j]->is_new_add_in_nit = 1;
                s_ts_info[i].service_info[j]->service_type = s_info.service_type;
                s_ts_info[i].service_info[j]->is_in_flash = 0;
                s_ts_info[i].service_info[j]->is_in_nit = 1;
                s_ts_info[i].service_info[j]->is_in_sdt = 0;
                s_ts_info[i].service_info[j]->is_in_pat = 0;
                s_ts_info[i].service_info[j]->service_id = s_info.service_id;
                s_ts_info[i].service_info[j]->ts_id = s_info.ts_id;
                s_ts_info[i].service_info[j]->orig_network_id = s_info.orig_network_id;
                s_ts_info[i].service_info[j]->lcn = s_info.lcn;
                s_ts_info[i].service_info[j]->pmt_pid = 0x1fff;
                s_ts_info[i].service_info[j]->pat_ver = AUTO_UPDATE_NO_VALID_VER;
                s_ts_info[i].service_info[j]->sdt_ver = AUTO_UPDATE_NO_VALID_VER;
                s_ts_info[i].service_info[j]->prog_name[0]='\0';
                s_ts_info[i].service_info[j]->common_status = 0xff;
                s_ts_info[i].service_info[j]->common_plp_id = 0xff;
                s_ts_info[i].service_info[j]->data_plp_id = 0xff;
                s_ts_info[i].service_info[j]->common_plp_exist = 0xff;
                s_ts_info[i].service_num++;
                _auto_update_ts_info_unlock();
                return 0;
            }
        }
    }
    _auto_update_ts_info_unlock();
    return 0;
}
#endif

static auto_update_prog_type _auto_update_check_service_isvalid(int i,int j)
{
    if(s_ts_info[i].service_info[j]->is_in_flash == 1)
    {
        if((s_ts_info[i].service_info[j]->is_in_sdt == 0)
#if OTHER_TS_UPDATE_SUPPORT
                || (s_ts_info[i].service_info[j]->is_in_nit == 0)
#endif
                || (s_ts_info[i].service_info[j]->is_in_pat == 0))
        {
            s_ts_info[i].service_info[j]->is_in_flash = 0;
            return AUTO_UPDATE_DEL_PROG;
        }
    }
    else
    {
        if((s_ts_info[i].service_info[j]->is_in_sdt == 1)
                &&(s_ts_info[i].service_info[j]->is_in_pat == 1))
        {
            s_ts_info[i].service_info[j]->is_in_flash = 1;
            return AUTO_UPDATE_ADD_PROG;
        }
#if OTHER_TS_UPDATE_SUPPORT
        if((s_ts_info[i].service_info[j]->is_in_nit == 1)&&
                (s_ts_info[i].service_info[j]->is_new_add_in_nit == 1))
        {
            s_ts_info[i].service_info[j]->is_in_flash = 1;
            s_ts_info[i].service_info[j]->is_in_pat = 1;  //add pat pmt incase version change again but program not update
            s_ts_info[i].service_info[j]->is_in_sdt = 1;
            s_ts_info[i].service_info[j]->is_new_add_in_sdt = 0;
            s_ts_info[i].service_info[j]->is_new_add_in_nit = 0;
            return AUTO_UPDATE_ADD_PROG;
        }
#endif
    }
    return AUTO_UPDATE_NO_CHANGE;
}



static int _auto_update_update_ts_info(int ts_id,int tp_id)
{
	GxBusPmDataProg *prog_arry = NULL;
    int prog_num = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    int num = 0;
    uint32_t id = 0;
    GxBusPmDataProg Prog={0};
    int ret = 0;
    int program_change = 0;
    int prog_status = 0;

    _auto_update_ts_info_lock();
    for(i=0;i<ts_info_num;i++)
    {
        if((s_ts_info[i].ts_id == ts_id || ts_id == -1)
                &&((s_ts_info[i].tp_id == tp_id ) || tp_id == -1 ))
        {
            if(tp_id != -1)
            {
                prog_num = GxBus_PmProgNumGetByTpId(tp_id);
                if(prog_num > 0)
                {
                    prog_arry = GxCore_Malloc(prog_num * sizeof(GxBusPmDataProg));
                    if(prog_arry == NULL)
                    {
                        app_log_error("Malloc prog_arry faild !!!\n\n");
                        break;
                    }
                    memset(prog_arry, 0, prog_num * sizeof(GxBusPmDataProg));
                    GxBus_PmProgGetByTpId(tp_id, prog_num, prog_arry);
                }
            }


            num = s_ts_info[i].service_num;
            for(j=(num-1);j>=0;j--)
            {
                prog_status = _auto_update_check_service_isvalid(i,j);
                if(prog_status ==  AUTO_UPDATE_DEL_PROG)//need remove ram and flash
                {
                    if(tp_id == -1)
                    {
                        ret = GxBus_PmProgGetBySpecialId(s_ts_info[i].service_info[j]->ts_id,s_ts_info[i].service_info[j]->service_id,-1,&Prog);
                        if(ret < 0)
                        {
                            app_log_error("del program service id %d not found\n",s_ts_info[i].service_info[j]->service_id);
                            continue;
                        }
#if BOUQUET_SUPPORT
                        app_bouque_set_update(s_ts_info[i].service_info[j]->ts_id,Prog.service_id);
#endif
                        _auto_update_remove_service(&s_ts_info[i].service_info[0],&s_ts_info[i].service_num,j);
                        id = Prog.id;
                        program_change = 1;
                        GxBus_PmProgDelete(&id,1);
                    }
                    else
                    {
                        for(k = 0; k < prog_num; k++)
                        {
                            if(s_ts_info[i].service_info[j]->ts_id == prog_arry[k].ts_id && s_ts_info[i].service_info[j]->service_id == prog_arry[k].service_id)
                            {
                                id =  prog_arry[k].id;
                                app_log_debug("###################remov some prog serviceid %d lcn %d\n",s_ts_info[i].service_info[j]->service_id, s_ts_info[i].service_info[j]->lcn);
#if BOUQUET_SUPPORT
                                app_bouque_set_update(s_ts_info[i].service_info[j]->ts_id,Prog.service_id);
#endif
                                _auto_update_remove_service(&s_ts_info[i].service_info[0],&s_ts_info[i].service_num,j);
                                program_change = 1;
                                GxBus_PmProgDelete(&id,1);
                                break;

                            }
                        }
                    }
                }
                else if(prog_status == AUTO_UPDATE_ADD_PROG)
                {
                    //need add to flash //how to deal with 0xcc in sdt
                    int count = 0;
                    int track_value = 0;
                    GxBusPmDataProg  prog_data = {0};
                    GxBus_ConfigGetInt(SOUND_MODE_KEY, &track_value, SOUND_MODE);
                    prog_data.tuner = app_tuner_cur_tuner_get();
                    prog_data.prog_name[MAX_PROG_NAME -1] = 0;
                    prog_data.ts_id = s_ts_info[i].service_info[j]->ts_id;
                    prog_data.service_id = s_ts_info[i].service_info[j]->service_id;
                    prog_data.service_type = s_ts_info[i].service_info[j]->service_type;
                    prog_data.scramble_flag = s_ts_info[i].service_info[j]->free_ca_mode;  // 1 maybe need modify later
                    prog_data.sdt_version = s_ts_info[i].service_info[j]->sdt_ver;
                    prog_data.original_id = s_ts_info[i].service_info[j]->orig_network_id;
                    prog_data.tp_id = s_ts_info[i].tp_id;
                    prog_data.audio_mode = track_value;
                    prog_data.audio_volume = AUDIO_VOLUME;
                    prog_data.pmt_pid = s_ts_info[i].service_info[j]->pmt_pid;
                    prog_data.video_pid = 0x1fff;
                    prog_data.cur_audio_pid = 0x1fff;
                    prog_data.pcr_pid = 0x1fff;
                    prog_data.pat_version = s_ts_info[i].service_info[j]->pat_ver;
                    prog_data.pmt_version = AUTO_UPDATE_NO_VALID_VER;
                    prog_data.sat_id = s_ts_info[i].sat_id;
                    prog_data.pos = 0;

                    prog_data.data_plp_id = s_ts_info[i].service_info[j]->data_plp_id;
                    prog_data.common_plp_exist = s_ts_info[i].service_info[j]->common_plp_exist;
                    prog_data.common_plp_id = s_ts_info[i].service_info[j]->common_plp_id;
                    prog_data.common_status = s_ts_info[i].service_info[j]->common_status;
                    prog_data.muti_ts_id       = 0xff;
                    prog_data.muti_ts_exist    = 0x0;
                    prog_data.muti_ts_code     = 0xff;
                    prog_data.pdmx_flag        = 0x0;
                    prog_data.pdmx_pid         = 0x1fff;
                    strcpy((char*)prog_data.prog_name,s_ts_info[i].service_info[j]->prog_name);
                    count = GxBus_PmProgExistChek(prog_data.tp_id,
                            prog_data.ts_id,
                            prog_data.service_id,
                            prog_data.original_id,
                            prog_data.tuner,prog_data.data_plp_id,prog_data.common_plp_exist,prog_data.common_plp_id);

                    if (count != 0) {
                        continue;
                    }
                    program_change = 1;
                    GxBus_PmProgAdd(&prog_data);
#if BOUQUET_SUPPORT
                    app_bouque_set_update(s_ts_info[i].service_info[j]->ts_id,s_ts_info[i].service_info[j]->service_id);
#endif
                    app_log_debug("add  prog id %d,prog_service_id %d prog lcn %d, prog_name %s\n",prog_data.id,prog_data.service_id,prog_data.pos,prog_data.prog_name);
                }
            }
            if(tp_id != -1)
            {
                APP_FREE(prog_arry);
            }
            if(program_change == 1)
            {
                GxBus_PmSync(GXBUS_PM_SYNC_PROG);
            }
        }
    }
    _auto_update_ts_info_unlock();
    return 0;
}

static int _auto_update_check_ts_service_sdt(int ts_id, int tp_id, auto_update_service_info s_info)
{
    int i= 0;
    int j=0;
    _auto_update_ts_info_lock();

    for(i=0;i<ts_info_num;i++)
    {
        if((s_ts_info[i].ts_id == ts_id || ts_id == -1)
                &&((s_ts_info[i].tp_id == tp_id)||(tp_id == -1)))
        {
            for(j=0;j<s_ts_info[i].service_num;j++)
            {
                if(s_ts_info[i].service_info[j]->service_id ==  s_info.service_id)
                {
                    s_ts_info[i].service_info[j]->ts_id = s_info.ts_id;
                    s_ts_info[i].service_info[j]->is_in_sdt = 1;
                    s_ts_info[i].service_info[j]->free_ca_mode = s_info.free_ca_mode;
                    s_ts_info[i].service_info[j]->service_type = s_info.service_type;
                    s_ts_info[i].service_info[j]->sdt_ver = s_info.sdt_ver;
                    s_ts_info[i].service_info[j]->orig_network_id = s_info.orig_network_id;
                    if(s_ts_info[i].service_info[j]->data_plp_id == 0xff)
                        s_ts_info[i].service_info[j]->data_plp_id = s_info.data_plp_id;
                    if(s_ts_info[i].service_info[j]->common_plp_exist == 0xff)
                        s_ts_info[i].service_info[j]->common_plp_exist = s_info.common_plp_exist;
                    if(s_ts_info[i].service_info[j]->common_plp_id == 0xff)
                        s_ts_info[i].service_info[j]->common_plp_id = s_info.common_plp_id;
                    s_ts_info[i].service_info[j]->common_status = s_info.common_status;
                    strcpy(s_ts_info[i].service_info[j]->prog_name,s_info.prog_name);
                    _auto_update_ts_info_unlock();
                    return 1;
                }

            }
            if(j==s_ts_info[i].service_num)  //new service
            {
                _auto_update_set_update_flag(true);
                if(j >= MAX_SERVICE_PER_TP)
                {
                    app_log_error("service full \n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }
                s_ts_info[i].service_info[j] = _auto_update_service_info_malloc();
                if(s_ts_info[i].service_info[j] == NULL)
                {
                    app_log_error(" s_ts_info[i].service_info[j] service malloc failed!!!!!!!!!!\n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }
                s_ts_info[i].service_info[j]->is_in_flash = 0;
                s_ts_info[i].service_info[j]->is_in_nit = 0;
                s_ts_info[i].service_info[j]->is_new_add_in_sdt = 1;
                s_ts_info[i].service_info[j]->is_in_sdt = 1;
                s_ts_info[i].service_info[j]->is_in_pat = 0;
                s_ts_info[i].service_info[j]->service_id = s_info.service_id;
                s_ts_info[i].service_info[j]->orig_network_id = s_info.orig_network_id;
                s_ts_info[i].service_info[j]->free_ca_mode = s_info.free_ca_mode;
                s_ts_info[i].service_info[j]->service_type = s_info.service_type;
                s_ts_info[i].service_info[j]->sdt_ver = s_info.sdt_ver;
                s_ts_info[i].service_info[j]->ts_id = s_info.ts_id;
                s_ts_info[i].service_info[j]->data_plp_id = s_info.data_plp_id;
                s_ts_info[i].service_info[j]->common_plp_exist = s_info.common_plp_exist;
                s_ts_info[i].service_info[j]->common_plp_id = s_info.common_plp_id;
                s_ts_info[i].service_info[j]->common_status = s_info.common_status;
                strcpy(s_ts_info[i].service_info[j]->prog_name,s_info.prog_name);
                s_ts_info[i].service_num++;
                _auto_update_ts_info_unlock();
                return 1;
            }
        }

    }
    _auto_update_ts_info_unlock();
    return 0;
}

static int _auto_update_check_ts_service_pat(int ts_id, int tp_id, auto_update_service_info s_info)
{
    int i= 0;
    int j=0;
    _auto_update_ts_info_lock();
    for(i=0;i<ts_info_num;i++)
    {
        if((s_ts_info[i].ts_id == ts_id || ts_id == -1)
                &&((s_ts_info[i].tp_id == tp_id)||(tp_id == -1)))
        {
            for(j=0;j<s_ts_info[i].service_num;j++)
            {
                if(s_ts_info[i].service_info[j]->service_id ==  s_info.service_id)
                {
                    s_ts_info[i].service_info[j]->ts_id = s_info.ts_id;
                    s_ts_info[i].service_info[j]->is_in_pat = 1;
                    s_ts_info[i].service_info[j]->pmt_pid = s_info.pmt_pid;
                    s_ts_info[i].service_info[j]->pat_ver = s_info.pat_ver;
                    _auto_update_ts_info_unlock();
                    return 1;
                }

            }
            if(j==s_ts_info[i].service_num)  //new service
            {
                _auto_update_set_update_flag(true);
                if(j >= MAX_SERVICE_PER_TP)
                {
                    app_log_error("service full \n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }
                s_ts_info[i].service_info[j] = _auto_update_service_info_malloc();
                if(s_ts_info[i].service_info[j] == NULL)
                {
                    app_log_error(" s_ts_info[i].service_info[j] service malloc failed!!!!!!!!!!\n");
                    _auto_update_ts_info_unlock();
                    return 0;
                }
                s_ts_info[i].service_info[j]->is_in_flash = 0;
                s_ts_info[i].service_info[j]->is_in_nit = 0;
                s_ts_info[i].service_info[j]->is_in_pat = 1;
                s_ts_info[i].service_info[j]->is_in_sdt = 0;
                s_ts_info[i].service_info[j]->service_id = s_info.service_id;
                s_ts_info[i].service_info[j]->ts_id = s_info.ts_id;
                s_ts_info[i].service_info[j]->pmt_pid = s_info.pmt_pid;
                s_ts_info[i].service_info[j]->pat_ver = s_info.pat_ver;
                s_ts_info[i].service_info[j]->service_type = 0;
                s_ts_info[i].service_info[j]->data_plp_id = 0xff;
                s_ts_info[i].service_info[j]->common_plp_exist = 0xff;
                s_ts_info[i].service_info[j]->common_plp_id = 0xff;
                s_ts_info[i].service_info[j]->common_status = 0xff;
                s_ts_info[i].service_num++;
                _auto_update_ts_info_unlock();
                return 1;
            }
        }

    }
    _auto_update_ts_info_unlock();
    return 0;
}

static void _auto_update_nit_update(nit_data_info* info ,psi_module_status state)
{
#if OTHER_TS_UPDATE_SUPPORT
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeByIdGet SatNode = {0};
    int i = 0;
    int j = 0;
    uint8_t service_type = 0;
    auto_update_service_info  app_service={0};
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
    SatNode.node_type = NODE_SAT;
    SatNode.id = node.prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);
    for(i = 0 ;i < info->ts_count; i++)
    {
        info->ts_info[i].tp.sat_id = node.prog_data.sat_id;
        _auto_update_add_ts_info(SatNode.sat_data.type,info->ts_info[i].tp,info->ts_info[i].ts_id);
        for(j= 0; j <  info->ts_info[i].service_count; j++)
        {
            app_service.ts_id = info->ts_info[i].ts_id;
            service_type = info->ts_info[i].service_info[j].service_type;
            if ((service_type == 0x2) ||(service_type == 0x7) ||(service_type  == 0xa) || (service_type  == 0x80))
            {
                app_service.service_type = GXBUS_PM_PROG_RADIO;
            }
            else if(service_type  == 0x1)
            {
                app_service.service_type = GXBUS_PM_PROG_TV;
            }
            else if((service_type  == 0x4) ||(service_type  == 0x5))
            {
                app_service.service_type = GXBUS_PM_PROG_NVOD;
            }
            else
            {
                if(service_type != 0x0c)
                {
                    app_service.service_type = GXBUS_PM_PROG_TV;
                }
                else
                {
                    app_service.service_type = GXBUS_PM_PROG_DATA;
                }
            }
            app_service.service_id = info->ts_info[i].service_info[j].service_id;
            app_service.orig_network_id = info->ts_info[i].orig_network_id;
            _auto_update_check_ts_service_nit(info->ts_info[i].ts_id,-1,app_service);
        }
#if RESERVED_SERVICE_NOSAVE_SUPPORT
        if(service_type == 0x0)
            continue;
#endif
    }
#endif
}

#if LCN_AUTO_UPDATE
static int _auto_update_modify_prog_lcn(GxBusPmDataProg prog, AppProgExtInfo* info,nit_lcn_info lcninfo)
{
    GxBusPmDataTP  Tp={0};
    GxBusPmDataProg *prog_arry = NULL;
    int16_t tp_num = 0;
    int16_t prog_count = 0;
    int16_t tp_pos = 0;
    int16_t prog_pos = 0;
    AppProgExtInfo ext_info;

    memset(&ext_info, 0, sizeof(AppProgExtInfo));
    tp_num = GxBus_PmTpNumGetBySat(prog.sat_id);
    for(tp_pos = 0; tp_pos < tp_num; tp_pos++)
    {
        GxBus_PmTpGetByPosInSat(tp_pos,1,&Tp);
        if(s_auto_back_control.tp_id == Tp.id)
            continue;
        prog_count = GxBus_PmProgNumGetByTpId(Tp.id);
        if(prog_count == 0)
            continue;
        prog_arry = GxCore_Malloc(prog_count * sizeof(GxBusPmDataProg));
        if(prog_arry == NULL)
        {
            app_log_error("Malloc prog_arry failed!!!!!!!!!!!\n");
            return 0;
        }
        memset(prog_arry,0 ,prog_count * sizeof(GxBusPmDataProg));
        GxBus_PmProgGetByTpId(Tp.id,prog_count,prog_arry);
        for(prog_pos = 0; prog_pos < prog_count; prog_pos++)
        {
            if(prog.service_type != prog_arry[prog_pos].service_type)
                continue;
            if(GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(prog_arry[prog_pos].id, sizeof(AppProgExtInfo), &ext_info))
            {
                if((lcninfo.lcn == ext_info.logicnum)
#if LCN_CONFLICT_HANDLE_SUPPORT
                        && ((info->logicnum >= VIRTUAL_LCN_START) && (info->logicnum < (VIRTUAL_LCN_START + MAX_VIRTUAL_LCN_NUM)))
#endif
                  )
                {
                    GxCore_Free(prog_arry);
                    prog_arry = NULL;
                    return 0;
                }
            }
        }
        GxCore_Free(prog_arry);
        prog_arry = NULL;
    }
    info->logicnum = lcninfo.lcn;
#if LCN_VISIBLE_FLAG_SUPPORT
    if(lcninfo.visible_flag == 0)
    {
        info->visible_flag = LCN_INVISIBLE;
    }
    else
    {
        info->visible_flag = LCN_VISIBLE;
    }
#endif
    if(GxBus_PmProgExtInfoAdd(prog.id, sizeof(AppProgExtInfo), info) == GXCORE_SUCCESS)
        return 1;
    return 0;
}

static void _auto_update_lcn_update(nit_data_info* info )
{
    AppProgExtInfo prog_ext_info;
    GxBusPmDataProg Prog = {0};
    int ret = 0;
    int i = 0;
    int j = 0;
    int need_update = 0;

    memset(&prog_ext_info, 0, sizeof(AppProgExtInfo));
    for(i = 0 ;i < info->ts_count; i++)
    {
        for(j= 0; j <  info->ts_info[i].lcn_count; j++)
        {
            ret = GxBus_PmProgGetBySpecialId(info->ts_info[i].ts_id,info->ts_info[i].lcn_info[j].service_id,info->ts_info[i].orig_network_id,&Prog);
            if(ret == GXCORE_SUCCESS)
            {
                if(GXCORE_SUCCESS == GxBus_PmProgExtInfoGet(Prog.id, sizeof(AppProgExtInfo), &prog_ext_info))
                {
                    if(prog_ext_info.logicnum == info->ts_info[i].lcn_info[j].lcn)
                    {
                        continue;
                    }
                }
                app_log_info("Prog.id = %d prog_name = %s lcn = %d,visible_flag = %d\n",Prog.id,Prog.prog_name,info->ts_info[i].lcn_info[j].lcn,info->ts_info[i].lcn_info[j].visible_flag);
                ret = _auto_update_modify_prog_lcn(Prog, &prog_ext_info,info->ts_info[i].lcn_info[j]);
                if(ret == 1)
                    need_update = 1;
            }
        }
    }
    if(need_update == 1)
    {
        GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
        app_auto_update_send_ui_msg(ACU_UPDATE_LCN,0,NULL);
    }
}

static void _auto_lcn_handle_conflict_exec(void)
{
    GxMsgProperty_NodeNumGet node_num_get = {0};
    uint32_t focus = 0;
    int ched_mode = 0;
#if LCN_CONFLICT_HANDLE_SUPPORT
    uint32_t sat_id = 0;
    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_TP;
    node.id = s_auto_back_control.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node));
    sat_id = node.tp_data.sat_id;
    app_lcn_list_init(&sat_id,1);
    app_lcn_handle_conflict_exec();
    app_lcn_list_info_save();
    app_lcn_list_destroy();
#endif
    GxBus_ConfigGetInt(CHED_SORTMODE_KEY, &ched_mode, CHED_SORTMODE_KEY_VALUE);
    if(ched_mode == SORT_BY_LCN)
    {
        GxBusPmViewInfo view_info;
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
        view_info.stream_type = GXBUS_PM_PROG_ALL;
        GxBus_PmViewInfoMemSet(&view_info);
        node_num_get.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
        g_AppChOps.create(node_num_get.node_num);
        g_AppChOps.sort(ched_mode, &focus);
        g_AppChOps.save();
        GxBus_PmViewInfoMemClear();
    }
}
#endif

static void _auto_update_sdt_getname(uint8_t *to, uint32_t buf_len, uint8_t *from, uint32_t name_len)
{
    int32_t utf8 = 0, y = 0;
    uint32_t prog_name_length = 0, psize = 0;
    uint8_t name_temp[MAX_PROG_NAME];

    memset(name_temp,0, MAX_PROG_NAME);

    if (name_len >= buf_len)
        prog_name_length = buf_len - 1;
    else
        prog_name_length = name_len;

    if(prog_name_length == 0)
    {
        strcpy((char*)name_temp,"no name");
        prog_name_length = 8;
    }
    else
    {
        memcpy(name_temp, from, prog_name_length);
    }

    for(y = 0; y < prog_name_length; y++) {
        if( name_temp[y]<0x1f||name_temp[y]>0x20)
            break;
    }

    GxBus_ConfigGetInt(SEARCH_PROGRAM_CODE,&utf8,GXBUS_SEARCH_UTF8_NO);
    if(utf8 == GXBUS_SEARCH_UTF8_NO)
        memcpy(to, &(name_temp[y]), prog_name_length-y);
    else if(utf8 == GXBUS_SEARCH_UTF8_YES)
    {
        uint8_t *pstr = NULL;
        if(gxgdi_iconv((const unsigned char *) &(name_temp[y]),&pstr,prog_name_length-y,&psize, NULL) == 0)
        {
            if(psize >= buf_len)
                prog_name_length = buf_len - 1;
            else
                prog_name_length = psize;

            memcpy(to, pstr, prog_name_length);
            GxCore_Free(pstr);
        }
        else
        {
            memcpy(to, &(name_temp[y]), prog_name_length-y);
        }
    }
    to[buf_len - 1] = 0;
}


static psi_module_status _auto_update_sdt_update(SdtInfo* sdt_info, psi_module_status state,auto_update_sdt_type type)
{
    uint8_t sdt_name_ch = 0;
    uint8_t version = 0;
    uint16_t ts_id = 0;
    int i = 0;

    GxBusPmDataProg prog_data = {0};
    GxBusPmDataProg *prog_arry = NULL;
    int prog_num = 0;
    auto_update_service_info  app_service={0};
    char service_name[MAX_PROG_NAME]={0};
    uint8_t service_name_len = 0;
    int j = 0;
    uint8_t program_in_flash = 0;
    uint8_t service_type = 0;
    int tp_id;
    tp_id = s_auto_back_control.tp_id;

    if(type == AUTO_UPDATE_SDT_ACT)
    {
        ts_id = sdt_info->si_info.ts_id;
        version = sdt_info->si_info.ver_number;
#if !AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        if(ts_id != s_auto_back_control.ts_id)
        {
            return PSI_MODULE_SECTION_OK;
        }
#endif
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
        GxBus_PmProgGetById(app_play_id_get(),&prog_data);
        if(prog_data.ts_id != ts_id || prog_data.tp_id != tp_id || prog_data.original_id != sdt_info->orig_network_id)
        {
            _auto_update_set_update_flag(true);
        }
#else
        if(s_auto_back_control.sdt_ver != AUTO_UPDATE_NO_VALID_VER && s_auto_back_control.sdt_ver ==  version)
        {
            return PSI_MODULE_SECTION_OK;
        }
        _auto_update_set_update_flag(true);
#endif
        _auto_update_get_plp();
        app_log_min("sdt_info ts_id = %d old version = %d  new version= %d prog_count = %d!!!!!!!!\n",ts_id,s_auto_back_control.sdt_ver,version,sdt_info->service_count);

        if(_auto_update_check_ts_info_is_build(AUTO_UPDATE_SDT_FINISH) == false)
        {
            return PSI_MODULE_SECTION_OK;
        }
        if(tp_id != -1)
        {
            prog_num = GxBus_PmProgNumGetByTpId(tp_id);
            if(prog_num > 0)
            {
                prog_arry = GxCore_Malloc(prog_num * sizeof(GxBusPmDataProg));
                if(prog_arry == NULL)
                {
                    app_log_error("Malloc prog_arry faild !!!\n\n");
                    return PSI_MODULE_SECTION_OK;
                }
                memset(prog_arry, 0, prog_num * sizeof(GxBusPmDataProg));
                GxBus_PmProgGetByTpId(tp_id, prog_num, prog_arry);
            }
        }
    }
    else
    {
        ts_id = sdt_info->tsid;
        version = sdt_info->si_info.ver_number;
    }

    if(sdt_info->service_count == 0 && AUTO_UPDATE_SDT_ACT == type)
        _auto_update_set_update_flag(true);

    for(i=0; i< sdt_info->service_count;i++)
    {
        app_service.ts_id = ts_id;
        app_service.free_ca_mode = sdt_info->service_info[i].free_ca_mode;
        app_service.service_id = sdt_info->service_info[i].service_id;
        app_service.sdt_ver = version;
        app_service.orig_network_id = sdt_info->orig_network_id;
        program_in_flash = 0;
        if(tp_id == -1 || type == AUTO_UPDATE_SDT_OTHER)
        {
            if(GxBus_PmProgGetBySpecialId(ts_id,app_service.service_id,app_service.orig_network_id,&prog_data) == GXCORE_SUCCESS)
            {
                program_in_flash = 1;
            }
            else
            {
                if(type == AUTO_UPDATE_SDT_OTHER)
                    continue;
            }
        }
        else
        {
            if(prog_num > 0)
            {
                for(j = 0; j < prog_num; j++)
                {
                    if(app_service.service_id == prog_arry[j].service_id)
                    {
                        memcpy(&prog_data,&prog_arry[j],sizeof(prog_data));
                        program_in_flash = 1;
                        break;
                    }
                }
            }
        }

        prog_data.bouquet_id = 0;
        memset(prog_data.u2.long_name,0,MAX_PROG_NAME);
        if(sdt_info->service_info[i].service_des_count != 0)
        {
            j = 0;
            service_type = sdt_info->service_info[i].service_des_info[j].service_type;
            app_log_debug("i = %d ts_id=%d service_id =%d service_type =0x%x !!!!!!!!!!!!!!!\n",i,ts_id,sdt_info->service_info[i].service_id,service_type);
            if (service_type == 0x2 || service_type == 0x7 || service_type  == 0xa || service_type  == 0x80)//0x80 海啸预警
            {
                app_service.service_type = GXBUS_PM_PROG_RADIO;
            }
            else if(service_type  == 0x1)
            {
                app_service.service_type = GXBUS_PM_PROG_TV;
            }
            else if((service_type == 0x4) ||
                    (service_type  == 0x5))
            {
                app_service.service_type = GXBUS_PM_PROG_NVOD;
            }
            else
            {
                if(service_type != 0x0c)
                {
                    app_service.service_type = GXBUS_PM_PROG_TV;
                }
                else
                {
                    app_service.service_type = GXBUS_PM_PROG_DATA;
                }
            }
            memset(service_name,0,MAX_PROG_NAME);
            service_name_len =sdt_info->service_info[i].service_des_info[j].service_name_length;
            _auto_update_sdt_getname((uint8_t*)service_name,MAX_PROG_NAME, sdt_info->service_info[i].service_des_info[j].service_name,service_name_len);
            if(service_name[0] < 0x20)//部分不做处理的字符串需要去除前导码
            {
                char standard_str[MAX_PROG_NAME] = {0};
                memcpy(standard_str, service_name, MAX_PROG_NAME);
                memset(service_name,0,MAX_PROG_NAME);
                memcpy(service_name,&standard_str[1], (MAX_PROG_NAME-1));
            }
            if((program_in_flash == 1) && ((prog_data.tp_id == tp_id && type == AUTO_UPDATE_SDT_ACT) || type != AUTO_UPDATE_SDT_ACT))
            {
                if((prog_data.sdt_version != version)
                            || (prog_data.scramble_flag != app_service.free_ca_mode)
                            || (memcmp((char *)prog_data.prog_name,service_name,MAX_PROG_NAME)!= 0)
                            || (app_service.service_type != prog_data.service_type)
                            || (prog_data.original_id != app_service.orig_network_id)
                            || ((prog_data.tp_id == tp_id) && (s_plp_table.num != 0)
                                && ((prog_data.data_plp_id != s_plp_table.plp_id[s_plp_table.search_index].data_plp_id)
                                || (prog_data.common_plp_id != s_plp_table.plp_id[s_plp_table.search_index].common_plp_id)
                                || (prog_data.common_plp_exist != s_plp_table.plp_id[s_plp_table.search_index].common_plp_exist))))
                {
                    if(type == AUTO_UPDATE_SDT_ACT)
                        _auto_update_set_update_flag(true);
                    sdt_name_ch = 1;
                    prog_data.sdt_version = version;
                    prog_data.scramble_flag = app_service.free_ca_mode;
                    if(memcmp((char *)prog_data.prog_name,service_name,MAX_PROG_NAME)!= 0)
                    {
                        memset(prog_data.prog_name,0,MAX_PROG_NAME);
                        memcpy(prog_data.prog_name,service_name,MAX_PROG_NAME);
                    }
                    if(app_service.service_type != prog_data.service_type)
                    {
                        prog_data.service_type = app_service.service_type;
                    }
                    prog_data.original_id = app_service.orig_network_id;
                    prog_data.ts_id = ts_id;
                    prog_data.common_status = add_common_status;
                    if(s_plp_table.num != 0)
                    {
                        prog_data.data_plp_id = s_plp_table.plp_id[s_plp_table.search_index].data_plp_id;
                        prog_data.common_plp_id = s_plp_table.plp_id[s_plp_table.search_index].common_plp_id;
                        prog_data.common_plp_exist = s_plp_table.plp_id[s_plp_table.search_index].common_plp_exist;
                    }
                    prog_data.video_pid = 0x1fff;
                    prog_data.cur_audio_pid = 0x1fff;
                    GxBus_PmLock();
                    GxBus_PmProgInfoModify(&prog_data);
                    GxBus_PmUnlock();
                }
            }
            memset(app_service.prog_name,0,MAX_PROG_NAME);
            memcpy(app_service.prog_name,service_name,MAX_PROG_NAME);
            app_service.common_status = add_common_status;
            if(s_plp_table.num != 0)
            {
                app_service.data_plp_id = s_plp_table.plp_id[s_plp_table.search_index].data_plp_id;
                app_service.common_plp_id = s_plp_table.plp_id[s_plp_table.search_index].common_plp_id;
                app_service.common_plp_exist = s_plp_table.plp_id[s_plp_table.search_index].common_plp_exist;
            }
            else
            {
                app_service.data_plp_id = 0xff;
                app_service.common_plp_id = 0xff;
                app_service.common_plp_exist = 0xff;
            }
            app_log_debug("[%d] %s data_plp_id = %d  common_plp_exist = %d common_plp_id = %d ,common_status = %d\n",s_plp_table.current_index,app_service.prog_name,app_service.data_plp_id,app_service.common_plp_exist,app_service.common_plp_id,app_service.common_status);
        }
#if RESERVED_SERVICE_NOSAVE_SUPPORT
        if(service_type == 0x0)
            continue;
#endif
        if(type == AUTO_UPDATE_SDT_ACT)
        {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
            _auto_update_check_ts_service_sdt(-1,tp_id,app_service);
#else
            _auto_update_check_ts_service_sdt(ts_id,tp_id,app_service);
#endif
        }
    }
    if((tp_id != -1) && (type != AUTO_UPDATE_SDT_OTHER))
    {
        APP_FREE(prog_arry);
    }
    if(sdt_name_ch == 1)
    {
        GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        sdt_name_ch = 0;
    }
    if(state == PSI_MODULE_TABLE_OK)
    {
        if(type == AUTO_UPDATE_SDT_ACT)
        {
            _auto_update_finish_check(AUTO_UPDATE_SDT_FINISH,ts_id,version,tp_id);
        }
       return state;
    }
    return state;
}

static psi_module_status _auto_update_pat_update(PatInfo* pat_info,psi_module_status state)
{
    uint8_t version = 0;
    uint16_t ts_id = 0;
    int i = 0;
    int j = 0;
	GxBusPmDataProg* prog_arry = NULL;
    int prog_num = 0;

	int tp_id;
    uint8_t pat_name_ch = 0;
    uint8_t program_in_flash = 0;
    auto_update_service_info  app_service={0};
    tp_id = s_auto_back_control.tp_id;
    GxBusPmDataProg prog_data = {0};
    version = pat_info->si_info.ver_number;
    ts_id = pat_info->si_info.ts_id;
#if !AUTO_UPDATE_MISMATCH_TSID_SUPPORT
    if(ts_id != s_auto_back_control.ts_id)
    {
        return PSI_MODULE_SECTION_OK;
    }
#endif

    if((s_auto_back_control.pat_ver == version) && (s_auto_back_control.pat_ver != AUTO_UPDATE_NO_VALID_VER))
    {
        return PSI_MODULE_SECTION_OK;
    }

    app_log_min("pat_info ts_id = %d old version = %d  new version= %d prog_count = %d!!!!!!!!\n",ts_id,s_auto_back_control.pat_ver,version,pat_info->prog_count);
    if(_auto_update_check_ts_info_is_build(AUTO_UPDATE_PAT_FINISH) == false)
    {
        return PSI_MODULE_SECTION_OK;
    }
    if(tp_id != -1)
    {
        prog_num = GxBus_PmProgNumGetByTpId(tp_id);
        if(prog_num > 0)
        {
            prog_arry = GxCore_Malloc(prog_num * sizeof(GxBusPmDataProg));
            if(prog_arry == NULL)
            {
                app_log_error("Malloc prog_arry faild !!!\n\n");
                return PSI_MODULE_SECTION_OK;
            }
            memset(prog_arry, 0, prog_num * sizeof(GxBusPmDataProg));
            GxBus_PmProgGetByTpId(tp_id, prog_num, prog_arry);
        }
    }

    for(i = 0; i < pat_info->prog_count;i++)
    {
        app_service.ts_id = ts_id;
        app_service.service_id = pat_info->prog_info[i].prog_num;
        app_service.pmt_pid = pat_info->prog_info[i].pmt_pid;
        app_service.pat_ver = version;
        program_in_flash = 0;
        if(tp_id == -1)
        {
            if(GxBus_PmProgGetBySpecialId(ts_id,app_service.service_id,-1,&prog_data) == GXCORE_SUCCESS)
            {
                program_in_flash = 1;
            }
        }
        else
        {
            if(prog_num > 0)
            {
                for(j = 0; j < prog_num; j++)
                {
                    if(app_service.service_id == prog_arry[j].service_id)
                    {
                        program_in_flash = 1;
                        memcpy(&prog_data,&prog_arry[j],sizeof(prog_data));
                        break;
                    }
                }
            }
        }

        if((prog_data.tp_id == tp_id) && (program_in_flash == 1))
        {
            if((prog_data.pat_version != version || prog_data.pmt_pid != app_service.pmt_pid || prog_data.ts_id != ts_id))
            {
                _auto_update_set_update_flag(true);
                prog_data.video_pid = 0x1fff;
                prog_data.cur_audio_pid = 0x1fff;
                prog_data.ts_id = ts_id;
                pat_name_ch = 1;
                prog_data.pat_version = version;
                prog_data.pmt_pid = app_service.pmt_pid;
                GxBus_PmLock();
                GxBus_PmProgInfoModify(&prog_data);
                GxBus_PmUnlock();
            }
        }

#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        _auto_update_check_ts_service_pat(-1,tp_id,app_service);
#else
        _auto_update_check_ts_service_pat(ts_id,tp_id,app_service);
#endif
    }
    if(tp_id != -1)
    {
        APP_FREE(prog_arry);
    }
    if(pat_name_ch == 1)
    {
        GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        pat_name_ch = 0;
    }
    if(state == PSI_MODULE_TABLE_OK)
    {
        _auto_update_finish_check(AUTO_UPDATE_PAT_FINISH,ts_id,version,tp_id);
    }
    return state;
}

static void _auto_update_table_stop(uint8_t *psi_id)
{
    app_psi_module_stop(*psi_id);
    *psi_id = 0;
}

#if OTHER_TS_UPDATE_SUPPORT && !AUTO_UPDATE_ALL_VERSION_SUPPORT
static status_t _auto_update_other_sdt_solv(void *data,psi_module_status state)
{
    GxMsgProperty_NodeByPosGet node = {0};
    GxBusPmDataTP  Tp={0};
    GxBusPmDataProg *prog_arry = NULL;
    int16_t tp_pos = 0;
    int16_t prog_count = 0;
    uint16_t ts_num = 0;

    SdtInfo* sdt_info = (SdtInfo*)data;
    _auto_update_sdt_update(sdt_info,state,AUTO_UPDATE_SDT_OTHER);
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
    ts_num = GxBus_PmTpNumGetBySat(node.prog_data.sat_id);
    for(tp_pos = 0; tp_pos < ts_num; tp_pos++)
    {
        GxBus_PmTpGetByPosInSat(tp_pos,1,&Tp);
        prog_count = GxBus_PmProgNumGetByTpId(Tp.id);
        if(prog_count == 0)
            continue;
        prog_arry = GxCore_Malloc(prog_count * sizeof(GxBusPmDataProg));
        if(prog_arry == NULL)
        {
            app_log_error("Malloc prog_arry failed!!!!!!!!!!!\n");
            return 0;
        }
        memset(prog_arry,0 ,prog_count * sizeof(GxBusPmDataProg));
        GxBus_PmProgGetByTpId(Tp.id,prog_count,prog_arry);
        if(strlen((char*)prog_arry[0].prog_name) == 0)
        {
            GxCore_Free(prog_arry);
            prog_arry = NULL;
            return GXCORE_SUCCESS;
        }
        GxCore_Free(prog_arry);
        prog_arry = NULL;
    }
    _auto_update_table_stop(&s_auto_back_control.sdt_other_id);
    return GXCORE_SUCCESS;
}

static void _auto_update_sdt_other_start(void)
{
    if(s_auto_back_control.sdt_other_id != 0)
    {
        _auto_update_table_stop(&s_auto_back_control.sdt_other_id);
    }
    psi_module_config config={0};
    config.demux_id = 0;
    config.params.pid = SDT_PID;
    config.params.depth = 1;
    config.params.match[0] = SDT_OTHER_TS_TID;
    config.params.mask[0] = 0xff;
    config.params.flags |= EQ_FLAG|CRC_FLAG;
    config.params.size = 8*1024;
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_priv_cb = NULL;
    config.node.parse_solv_cb = _auto_update_other_sdt_solv;
    config.node.independent_thread = true;
    config.node.no_stop = true;
    s_auto_back_control.sdt_other_id = app_psi_module_start(config);
}
#endif

static status_t _auto_update_act_sdt_solv(void *data,psi_module_status state)
{
    SdtInfo* sdt_info = (SdtInfo*)data;
    _auto_update_sdt_update(sdt_info,state,AUTO_UPDATE_SDT_ACT);
    return GXCORE_SUCCESS;
}

static void _auto_update_sdt_act_start(uint16_t ts_id,uint8_t ver)
{
    if(s_auto_back_control.sdt_act_id != 0)
    {
        _auto_update_table_stop(&s_auto_back_control.sdt_act_id);
    }
    psi_module_config config={0};
    config.demux_id = 0;
    config.params.pid = SDT_PID;
    config.params.match[0] = SDT_ACTUAL_TS_TID;
    config.params.mask[0] = 0xff;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    ver = AUTO_UPDATE_NO_VALID_VER;
#endif
    if(ver >= AUTO_UPDATE_NO_VALID_VER)
    {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        config.params.depth = 1;
#else
        config.params.depth = 5;
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.flags |= EQ_FLAG;
    }
    else
    {
        config.params.depth = 6;
#if !AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.match[5] = (ver<<1);
        config.params.mask[5] = 0x3e;
        config.params.flags &= (~EQ_FLAG);
    }
    config.params.flags |= CRC_FLAG;
    config.node.no_stop = true;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    config.node.no_same = true;
#endif
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_priv_cb = NULL;
    config.node.parse_solv_cb = _auto_update_act_sdt_solv;
    s_auto_back_control.sdt_act_id = app_psi_module_start(config);
}

static void _auto_update_nit_section_parse(uint8_t* pSectionData,size_t Size,nit_data_info* info)
{
    uint8_t*  sectionData;
    int32_t  sectionLen = 0;
    int32_t  parseTotalLen=0;
    int32_t  descriptorLen = 0;

    int i = 0;
    uint16_t j = 0;
    int32_t	   secondDesLen=0;
    uint32_t 	   symbol = 0;
    uint32_t  freq = 0;
    uint8_t modulation = 0;
    int slist_issue = 0;
    char *p_temp_slist = NULL;
    uint16_t num = 0;
    uint16_t lcn_count = 0;
    uint16_t service_id = 0;
    int base = 0;

    if (NULL == pSectionData)
        return;

    info->secNum = pSectionData[6];
    info->regionId = (pSectionData[3] << 8)|(pSectionData[4]);
    info->network_id = (pSectionData[3]<<8)|pSectionData[4];
    info->ver = (pSectionData[5] & 0x3E) >> 1;
    sectionLen = ((pSectionData[8] & 0xF)<<8)|(pSectionData[9]);
    sectionData = pSectionData + 10;

    if (sectionLen>0)
    {
        descriptorLen = 0;
        parseTotalLen = 0;

        while (parseTotalLen < sectionLen)
        {
            descriptorLen = sectionData[1]+2;
            switch(sectionData[0])
            {
                default:
                    break;

            }
            sectionData += descriptorLen;
            parseTotalLen += descriptorLen;
        }
    }

    sectionLen  =	((sectionData[0] & 0xF)<<8)|(sectionData[1]);
    sectionData += 2;

    while (sectionLen > 0)
    {
        info->ts_info = GxCore_Realloc(info->ts_info,sizeof(nit_ts_info)*(info->ts_count+1));
        memset(&(info->ts_info[info->ts_count]),0,sizeof(nit_ts_info));
        if( info->ts_info == NULL)
        {
            app_log_error("malloc info->ts_info faild!!!!!!!!!!\n");
            return;
        }

        info->ts_info[info->ts_count].ts_id = (sectionData[0]<<8)|(sectionData[1]);
        info->ts_info[info->ts_count].orig_network_id = (sectionData[2]<<8)|(sectionData[3]);
        descriptorLen = ((sectionData[4] << 8)&0xF00) | sectionData[5];
        sectionData += 6;
        if (descriptorLen>0)
        {
            secondDesLen = 0;
            parseTotalLen = 0;
            while (parseTotalLen < descriptorLen)
            {
                secondDesLen = sectionData[1]+2;
                switch(sectionData[0])
                {
                    case 0x82:
                    case 0x83:
                    case 0x87:
                    case 0x88:
                        lcn_count = info->ts_info[info->ts_count].lcn_count;
                        if(sectionData[0] == 0x87)
                        {
                            num = lcn_count+(sectionData[2 + 1 + 1 + sectionData[3] + 3]/4);
                            base = 6 + sectionData[3];
                        }
                        else
                        {
                            num = lcn_count+(sectionData[1]/4);
                            base = 0;
                        }
                        if(num == 0)
                            return;
                        info->ts_info[info->ts_count].lcn_info = GxCore_Realloc(info->ts_info[info->ts_count].lcn_info,sizeof(nit_lcn_info)*num);
                        if(info->ts_info[info->ts_count].lcn_info == NULL)
                        {
                            app_log_error("malloc info->ts_info[info->ts_count].lcn_info failed!!!!!!!!!!\n");
                            return;
                        }
                        for(i = 0; i< (num - lcn_count); i++)
                        {
                            service_id = ((sectionData[base+2+i*4]<<8)&0xff00) + sectionData[base+3+i*4];
                            for(j = 0; j < lcn_count; j++)
                            {
                                if(info->ts_info[info->ts_count].lcn_info[j].service_id == service_id)
                                {
                                    if(info->ts_info[info->ts_count].lcn_info[j].lcn_tag == 0x88)
                                    {
                                        break;
                                    }
                                    info->ts_info[info->ts_count].lcn_info[j].lcn_tag = sectionData[0];
                                    info->ts_info[info->ts_count].lcn_info[j].visible_flag = ((sectionData[base+4+i*4] & 0x80) >> 7);
                                    info->ts_info[info->ts_count].lcn_info[j].lcn = ((sectionData[base+4+i*4]<<8)&0xff00) + sectionData[base+5+i*4];
                                    info->ts_info[info->ts_count].lcn_info[j].lcn &= 0x3ff;
                                    break;
                                }
                            }
                            if(j == lcn_count)
                            {
                                info->ts_info[info->ts_count].lcn_info[info->ts_info[info->ts_count].lcn_count].lcn_tag = sectionData[0];
                                info->ts_info[info->ts_count].lcn_info[info->ts_info[info->ts_count].lcn_count].service_id =  service_id;
                                info->ts_info[info->ts_count].lcn_info[info->ts_info[info->ts_count].lcn_count].visible_flag = ((sectionData[base+4+i*4] & 0x80) >> 7);
                                info->ts_info[info->ts_count].lcn_info[info->ts_info[info->ts_count].lcn_count].lcn = ((sectionData[base+4+i*4]<<8)&0xff00) + sectionData[base+5+i*4];
                                info->ts_info[info->ts_count].lcn_info[info->ts_info[info->ts_count].lcn_count].lcn &= 0x3ff;
                                info->ts_info[info->ts_count].lcn_count++;
                            }
                        }
                        break;
                    case 0x41:
                        slist_issue = sectionData[1];
                        if(slist_issue)
                        {
                            p_temp_slist = GxCore_Malloc(sectionData[1]);
                            if(p_temp_slist == NULL)
                            {
                                app_log_error("malloc p_temp_slist error!!!!!!!!!!\n");
                                return;
                            }
                            info->ts_info[info->ts_count].service_count = slist_issue/3;
                            if (info->ts_info[info->ts_count].service_info == NULL)
                            {
                                info->ts_info[info->ts_count].service_info = GxCore_Malloc(sizeof(nit_service_info)*info->ts_info[info->ts_count].service_count);
                            }
                            if(info->ts_info[info->ts_count].service_info == NULL)
                            {
                                GxCore_Free(p_temp_slist);
                                app_log_error("malloc info->ts_info[info->ts_count].service_info error!!!!!!!!!!\n");
                                return;
                            }
                            memcpy(p_temp_slist,&sectionData[2],sectionData[1]);
                            for(i=0;i<slist_issue/3;i++)
                            {
                                info->ts_info[info->ts_count].service_info[i].service_id = ((p_temp_slist[i*3]<<8)&0xff00) +p_temp_slist[1+i*3];
                                info->ts_info[info->ts_count].service_info[i].service_type = p_temp_slist[2+i*3];
                            }
                            GxCore_Free(p_temp_slist);
                        }

                        break;
                    case 0x44:
                        { // -c
                            freq = ((sectionData[2] & 0xf0)>>4) * 10000000 + (sectionData[2] & 0xf) * 1000000
                                + ((sectionData[3] & 0xf0)>>4) * 100000 + (sectionData[3] & 0xf) * 10000
                                + ((sectionData[4] & 0xf0)>>4) * 1000 + (sectionData[4] & 0xf) * 100
                                + ((sectionData[5] & 0xf0) >> 4) * 10 + (sectionData[5] & 0xf);

                            symbol = ((sectionData[9] & 0xf0)>>4) * 1000000 + (sectionData[9] & 0xf) * 100000
                                + ((sectionData[10] & 0xf0)>>4) * 10000 + (sectionData[10] & 0xf) * 1000
                                + ((sectionData[11] & 0xf0)>>4) * 100 + (sectionData[11] & 0xf) * 10
                                + ((sectionData[12] & 0xf0) >> 4) ;
                            symbol = symbol /10;  // k
                            freq= freq/10; // khz
                            //	freq = freq/1000;// mhz
                            modulation = sectionData[8];
                            info->ts_info[info->ts_count].tp.frequency = freq;
                            info->ts_info[info->ts_count].tp.tp_c.symbol_rate = symbol;
                            info->ts_info[info->ts_count].tp.tp_c.modulation = modulation+2;
                        }
                        break;
                    default:
                        break;
                }
                sectionData += secondDesLen;
                parseTotalLen += secondDesLen;
            }
        }
        sectionLen =	sectionLen -	descriptorLen - 6;
        info->ts_count++;
    }
}

static void _auto_update_clear_nit_info(nit_data_info* info)
{
    int i = 0;
    if(info->ts_info == NULL)
        return;
    for(i = 0; i<info->ts_count; i++)
    {
        if(info->ts_info[i].service_info != NULL)
        {
            GxCore_Free(info->ts_info[i].service_info);
            info->ts_info[i].service_info = NULL;
        }
        if(info->ts_info[i].lcn_info != NULL)
        {
            GxCore_Free(info->ts_info[i].lcn_info);
            info->ts_info[i].lcn_info = NULL;
        }
    }
    GxCore_Free(info->ts_info);
    info->ts_info = NULL;
    memset(info,0,sizeof(nit_data_info));
}

static psi_module_status _auto_update_nit_private(uint8_t* Section, size_t Size)
{
    uint16_t            section_length;
    uint8_t*            data = (uint8_t*)Section;
    uint8_t version = 0;
    int32_t i=0;
    int                 len = Size;
    uint32_t ret = PSI_MODULE_SECTION_OK;
    nit_data_info nit_info = {0};

    if (NULL == Section)
        return ret;


    while(len > 0) {
        if (1 != nit_status[data[6]])
        {
            nit_status[data[6]] = 1;
        }

        if(nit_status [0] != 1)
        {
            nit_status[data[6]] = 0;
            return ret;
        }
        version = (data[5] & 0x3E) >> 1;
        if((data[6] == 0) && (nit_in_update == 0))
        {
            if(version !=  s_auto_back_control.nit_ver)
            {
                if(_auto_update_check_ts_info_is_build(AUTO_UPDATE_NIT_FINISH) == false)
                {
                    app_log_debug("new version = %d,old_version%d!!!!!\n",version,s_auto_back_control.nit_ver);
                }
                else
                {
#if OTHER_TS_UPDATE_SUPPORT
                    if(nit_in_update == 0)
                    {
                        _auto_update_check_tp_valid_reset();
                    }
#endif
                    nit_in_update = 1;
                    nit_version_changed = 1;
                }
            }
        }
        for (i = 0;i<=data[7];i++)
        {
            if (0 == nit_status[i])
                break;
        }

        section_length = ((data[1] & 0x0F) << 8) + data[2] + 3;
        if(nit_version_changed == 1 && app_get_auto_update_flag() != 0)
        {
            memset(&nit_info,0, sizeof(nit_data_info));
            _auto_update_nit_section_parse(data, section_length,&nit_info);
            _auto_update_nit_update(&nit_info,ret);
            _auto_update_clear_nit_info(&nit_info);
        }
#if LCN_AUTO_UPDATE
        else if(s_auto_back_control.nit_ver != version && app_get_auto_update_flag() == 0)
        {
            memset(&nit_info,0, sizeof(nit_data_info));
            _auto_update_nit_section_parse(data, section_length,&nit_info);
            _auto_update_lcn_update(&nit_info);
            _auto_update_clear_nit_info(&nit_info);
        }
#endif

        if(i>data[7])
        {
            memset(nit_status,0,256);
            ret = PSI_MODULE_TABLE_OK;
            if((s_auto_back_control.nit_ver != version && app_get_auto_update_flag() == 0))
            {
                GxBus_ConfigSetInt(MAIN_FREQ_NITVERSION,version);
#if !AUTO_UPDATE_ALL_VERSION_SUPPORT
                app_auto_update_send_ui_msg(ACU_UPDATE_NIT_START,version,NULL);
#endif
            }
            if(nit_version_changed == 1)
            {
                _auto_update_finish_check(AUTO_UPDATE_NIT_FINISH,-1,version,-1);
                nit_version_changed = 0;
                nit_in_update = 0;
            }
            return ret;
        }

        data += section_length;
        len -= section_length;
    }

    return ret;
}



static void _auto_update_nit_start(uint16_t ts_id, uint8_t ver)
{
    if(s_auto_back_control.nit_id != 0)
    {
        _auto_update_table_stop(&s_auto_back_control.nit_id);
    }
    memset(nit_status,0,256);
    psi_module_config config={0};
    config.demux_id = 0;
    config.params.pid = NIT_PID;
    config.params.match[0] = NIT_ACTUAL_NETWORK_TID;
    config.params.mask[0] = 0xff;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    ver = AUTO_UPDATE_NO_VALID_VER;
#endif
    if(ver >= AUTO_UPDATE_NO_VALID_VER)
    {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT || OTHER_TS_UPDATE_SUPPORT
        config.params.depth = 1;
#else
        config.params.depth = 5;
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.flags |= EQ_FLAG;
    }
    else
    {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT || OTHER_TS_UPDATE_SUPPORT
        config.params.depth = 6;
#else
        config.params.depth = 6;
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.match[5] = (ver << 1);
        config.params.mask[5] = 0x3e;
        config.params.flags &= (~EQ_FLAG);
    }
    config.params.size = 8*1024;
    config.params.flags |= CRC_FLAG;
    config.node.no_stop = true;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    config.node.no_same = true;
#endif
    config.node.mode = PSI_MODULE_PRIVATE_ONLY;
    config.node.parse_priv_cb = _auto_update_nit_private;
    config.node.parse_solv_cb = NULL;
    s_auto_back_control.nit_id = app_psi_module_start(config);
}

static status_t _auto_update_pat_solv(void *data,psi_module_status state)
{

    PatInfo *pat_info = (PatInfo*)data;
    _auto_update_pat_update(pat_info,state);
    return GXCORE_SUCCESS;

}

static void _auto_update_pat_start(uint16_t ts_id,uint8_t ver)
{
    if(s_auto_back_control.pat_id != 0)
    {
        _auto_update_table_stop(&s_auto_back_control.pat_id);
    }
    psi_module_config config={0};
    config.demux_id = 0;
    config.params.pid = PAT_PID;
    config.params.match[0] = PAT_TID;
    config.params.mask[0] = 0xff;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    ver = AUTO_UPDATE_NO_VALID_VER;
#endif
    if(ver >= AUTO_UPDATE_NO_VALID_VER)
    {
#if AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        config.params.depth = 1;
#else
        config.params.depth = 5;
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.flags |= EQ_FLAG;
    }
    else
    {
        config.params.depth = 6;
#if !AUTO_UPDATE_MISMATCH_TSID_SUPPORT
        config.params.match[3] = (ts_id>>8);
        config.params.mask[3] = 0xff;
        config.params.match[4] = (ts_id & 0xff);
        config.params.mask[4] = 0xff;
#endif
        config.params.match[5] = (ver << 1);
        config.params.mask[5] = 0x3e;
        config.params.flags &= (~EQ_FLAG);
    }

    config.params.flags |= CRC_FLAG;
    config.node.no_stop = true;
#if AUTO_UPDATE_ALL_VERSION_SUPPORT
    config.node.no_same = true;
#endif
    config.node.mode = PSI_MODULE_STANDARD_ONLY;
    config.node.parse_priv_cb = NULL;
    config.node.parse_solv_cb = _auto_update_pat_solv;
    s_auto_back_control.pat_id = app_psi_module_start(config);
}

static bool _auto_update_nit_sdt_filter_check(void)
{
    bool ret = false;
    if(s_auto_back_control.nit_id
#if OTHER_TS_UPDATE_SUPPORT
            && s_auto_back_control.sdt_other_id
#endif
            && s_auto_back_control.sdt_act_id)
        ret = true;
    return ret;
}

void app_auto_update_start(GxMsgProperty_NodeByPosGet *prog_node)
{
    int32_t acu_config;
    GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);
    if(acu_config)
    {
        _auto_update_init_plp_table();
        s_auto_back_control.ts_id = prog_node->prog_data.ts_id;
        if(false == _auto_update_nit_sdt_filter_check()
                || s_auto_back_control.tp_id != prog_node->prog_data.tp_id
                || s_auto_back_control.data_plp_id != prog_node->prog_data.data_plp_id
                || s_auto_back_control.common_plp_id != prog_node->prog_data.common_plp_id
                || s_auto_back_control.common_plp_exist != prog_node->prog_data.common_plp_exist)
        {
            s_auto_back_control.data_plp_id = prog_node->prog_data.data_plp_id;
            s_auto_back_control.common_plp_id = prog_node->prog_data.common_plp_id;
            s_auto_back_control.common_plp_exist = prog_node->prog_data.common_plp_exist;
            s_auto_back_control.nit_ver = AUTO_UPDATE_NO_VALID_VER;
            _auto_update_nit_start(s_auto_back_control.ts_id,s_auto_back_control.nit_ver);
            s_auto_back_control.tp_id = prog_node->prog_data.tp_id;
            s_auto_back_control.sdt_ver = prog_node->prog_data.sdt_version;
            _auto_update_sdt_act_start(s_auto_back_control.ts_id,s_auto_back_control.sdt_ver);
#if BOUQUET_SUPPORT
            app_bouquet_reset_all_time();
#endif
            s_auto_back_control.pat_ver = prog_node->prog_data.pat_version;
            _auto_update_pat_start(s_auto_back_control.ts_id,s_auto_back_control.pat_ver);
            APP_TIMER_ADD(s_check_feinfo_timer, _auto_update_check_timer, 10000, TIMER_REPEAT);
        }
    }
}

void app_auto_update_stop(void)
{
    int32_t acu_config;
    GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);
    if(acu_config)
    {
        APP_TIMER_REMOVE(s_check_feinfo_timer);
       _auto_update_table_stop(&s_auto_back_control.nit_id);
        _auto_update_table_stop(&s_auto_back_control.sdt_act_id);
#if OTHER_TS_UPDATE_SUPPORT
        _auto_update_table_stop(&s_auto_back_control.sdt_other_id);
#endif
        _auto_update_init_plp_table();
        _auto_update_table_stop(&s_auto_back_control.pat_id);
        _auto_update_clear_ts_info();
        memset(nit_status,0,256);
        auto_update_flag = 0;
        update_flag = false;
    }
}

void app_auto_update_restart(void)
{
    int32_t acu_config;
    GxBus_ConfigGetInt(T2_ACU_KEY, &acu_config, T2_ACU_VALUE);
    if(acu_config)
    {
        _auto_update_init_plp_table();
        _auto_update_nit_start(s_auto_back_control.ts_id,s_auto_back_control.nit_ver);
        _auto_update_sdt_act_start(s_auto_back_control.ts_id,s_auto_back_control.sdt_ver);
        _auto_update_pat_start(s_auto_back_control.ts_id,s_auto_back_control.pat_ver);
        APP_TIMER_ADD(s_check_feinfo_timer, _auto_update_check_timer, 10000, TIMER_REPEAT);
    }
}

uint8_t app_get_auto_update_flag(void)
{
    if(update_flag == false)
        return 0;
    return 1;
}

static int32_t _auto_update_play_program(int32_t play_count)
{
    int32_t pos;
    g_AppChOps.update_prog_num();
    if(g_AppChOps.get_list_total() > 0)
    {
        g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(play_count);
        app_play_current_prog(PLAY_MODE_POINT);
    }
    else
    {
        if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV && g_AppChOps.get_tv_num() != 0)
                || (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO && g_AppChOps.get_radio_num() != 0))
        {
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
            g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
            app_play_current_prog(PLAY_MODE_POINT);
        }
        else if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV && g_AppChOps.get_radio_num() != 0)
                || (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO && g_AppChOps.get_tv_num() != 0))
        {
            if(GXCORE_SUCCESS != GUI_CheckDialog(WND_EPG))
            {
                app_clean_fullscreen_tip(true, false, false);
                g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
                g_AppFullArb.exec[EVENT_TV_RADIO](&g_AppFullArb);
                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
                GUI_SetProperty(WND_FULL_SCREEN, "draw_now", NULL);
                app_channel_info_create_dialog();
            }
            else
            {
                g_AppFullArb.exec[EVENT_TV_RADIO](&g_AppFullArb);
            }
        }
        else
        {
            g_AppPlayOps.normal_play.play_count = 0;
            GUI_EndDialog("after wnd_full_screen");
            g_AppPlayOps.program_stop();
            g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
            return -1;
        }
    }
    pos = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
    return pos;
}

static int _auto_update_get_playing_pos_in_group(void)
{
    uint32_t prog_pos = 0;
    bool get_pos_flag = false;
    int i ;
    uint32_t prog_num = 0;
    uint32_t prog_id = 0;
    GxBusPmDataProg prog_data;
    GxBusPmViewInfo sysinfo;
    GxBus_PmViewInfoGet(&sysinfo);
    prog_num = GxBus_PmProgNumGet();
    if(sysinfo.stream_type == GXBUS_PM_PROG_TV)
    {
        prog_id = sysinfo.tv_prog_cur;
    }
    else if(sysinfo.stream_type == GXBUS_PM_PROG_RADIO)
    {
        prog_id = sysinfo.radio_prog_cur;
    }
    for(i=0;i<prog_num;i++)
    {
        GxBus_PmProgGetByPos(i,1,(void*)&prog_data);
        if(prog_data.tp_id == s_auto_back_control.tp_id && sysinfo.stream_type == prog_data.service_type && get_pos_flag == false)
        {
            get_pos_flag = true;
            prog_pos = i;
        }
        if(prog_id == prog_data.id)
        {
            return i;
        }
    }
    return prog_pos;
}

static int app_auto_update_prog_msg_proc(GxMessage *msg)
{
    GxMsgProperty_NodeNumGet node_num_get = {0};
    uint32_t pos = 0;
    uint32_t focus = 0;
    int ched_mode = 0;

    AppMsgAcuStateUiCotrol* propmtinfo = GxBus_GetMsgPropertyPtr(msg, AppMsgAcuStateUiCotrol);
    switch(propmtinfo->type)
    {
        case ACU_UPDATE_START:
            {
                book_popdlg_exec(POP_VAL_CANCEL);
                popdlg_destroy();
                if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                {
                    app_epg_preprocess_for_acu();
                }
                else
                {
                    g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt,STBK_EXIT);
                }
                if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
                {
                    GUI_EndDialog(WND_PASSWD);
                }
#if AUTO_UPDATE_PROPMT_SUPPORT
                if(GXCORE_SUCCESS != GUI_CheckDialog(WND_EPG))
                {
                    GUI_EndDialog("after wnd_full_screen");
                    if(g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
                    {
                       app_channel_info_create_dialog();
                    }
                }
                if(propmtinfo->show_str != NULL)
                {
                    PopDlg  pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.format = POP_FORMAT_DLG;
                    pop.default_ret = POP_VAL_CANCEL;
                    pop.str = propmtinfo->show_str;
                    pop.timeout_sec = propmtinfo->timeout;
                    popdlg_create(&pop);
                }
#endif
                g_AppFullArb.timer_stop();
            }
            break;
        case ACU_UPDATE_NIT_START:
            s_auto_back_control.nit_ver = propmtinfo->version;
            _auto_update_nit_start(s_auto_back_control.ts_id,s_auto_back_control.nit_ver);
            break;
#if LCN_AUTO_UPDATE
        case ACU_UPDATE_LCN:
            {
                popdlg_destroy();
                _auto_lcn_handle_conflict_exec();
                g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
                channel_get_playing_pos_in_group(&pos);
                if(g_AppChOps.get_visible_flag(pos) == true)
                {
                    g_AppChOps.update_prog_num();
                    g_AppPlayOps.normal_play.play_count = pos;
                    pos = g_AppChOps.real_select(pos);
                }
                else
                {
                    pos =_auto_update_play_program(pos);
                    if(pos == -1)
                        break;
                }
                if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                {
                    book_popdlg_exec(POP_VAL_CANCEL);
                    GUI_SetProperty("listview_epg_prog", "update_all", NULL);
                    GUI_SetProperty("listview_epg_prog", "select", &pos);
                    GUI_SetProperty("listview_epg_prog", "active", &pos);
                }
#if BOUQUET_SUPPORT
                const char *wnd = GUI_GetFocusWindow();
                if(NULL == wnd)
                    return -1;
                else if(0 == strcmp(wnd,WND_BOUQUET))
                {
                    app_bouquet_backup_refresh(1,pos);

                }
#endif
                else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                {
                    GUI_EndDialog("after wnd_keyboard_language");
                    app_multi_lang_keyborad_menu_destroy();
                    GUI_EndDialog(WND_CHANNEL_LIST);
                }

                else if(0 == app_channel_list_compare_dialog(WND_CHANNEL_LIST) && LIST_MODE_PROG == app_get_channel_list_mode())
                {
                    GUI_SetProperty("listview_channel_list", "update_all", NULL);
                    GUI_SetProperty("listview_channel_list", "select", &pos);
                    GUI_SetProperty("listview_channel_list", "active", &pos);
                }
            }
            break;
#endif
        case ACU_UPDATE_END:
            {
                app_pop_reset_timeout(3);
#if !AUTO_UPDATE_ALL_VERSION_SUPPORT
               _auto_update_sdt_act_start(s_auto_back_control.ts_id,s_auto_back_control.sdt_ver);
               _auto_update_pat_start(s_auto_back_control.ts_id,s_auto_back_control.pat_ver);
#if OTHER_TS_UPDATE_SUPPORT
                _auto_update_sdt_other_start();
#endif
#endif
#if LCN_AUTO_UPDATE
                _auto_lcn_handle_conflict_exec();
                s_auto_back_control.nit_ver = AUTO_UPDATE_NO_VALID_VER;
                _auto_update_nit_start(s_auto_back_control.ts_id,s_auto_back_control.nit_ver);
#endif
                GxBus_ConfigGetInt(CHED_SORTMODE_KEY, &ched_mode, CHED_SORTMODE_KEY_VALUE);
                if(ched_mode != 0xff
#if LCN_SUPPORT
                        && ched_mode != SORT_BY_LCN
#endif
                        )
                {
                    GxBusPmViewInfo view_info;
                    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
                    view_info.stream_type = GXBUS_PM_PROG_ALL;
                    GxBus_PmViewInfoMemSet(&view_info);
                    node_num_get.node_type = NODE_PROG;
                    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
                    if(node_num_get.node_num != 0)
                    {
                        g_AppChOps.create(node_num_get.node_num);
                        g_AppChOps.sort(ched_mode, &focus);
                        g_AppChOps.save();
                    }
                    GxBus_PmViewInfoMemClear();
                }
                GxBus_PmProgListRebulid();
                node_num_get.node_type = NODE_PROG;
                app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
                g_AppPlayOps.normal_play.play_total = node_num_get.node_num;
                g_AppChOps.create(g_AppPlayOps.normal_play.play_total);
                pos = _auto_update_get_playing_pos_in_group();
                app_epg_disable();
                pos = _auto_update_play_program(pos);
                if(pos == -1)
                    break;

                if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                {
                    book_popdlg_exec(POP_VAL_CANCEL);
                    GUI_SetProperty("listview_epg_prog", "update_all", NULL);
                    GUI_SetProperty("listview_epg_prog", "select", &pos);
                    GUI_SetProperty("listview_epg_prog", "active", &pos);
                    app_update_pop_dlg(WND_POP_TIP);
                }
#if !AUTO_UPDATE_PROPMT_SUPPORT
#if BOUQUET_SUPPORT
                const char *wnd = GUI_GetFocusWindow();
                if(NULL == wnd)
                    return -1;
                else if(0 == strcmp(wnd,WND_BOUQUET))
                {
                    extern void app_bouquet_backup_refresh(int type, uint32_t pos);
                    app_bouquet_backup_refresh(2,0);

                }
#endif
                else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
                {
                    GUI_EndDialog("after wnd_keyboard_language");
                    app_multi_lang_keyborad_menu_destroy();
                    GUI_EndDialog(WND_CHANNEL_LIST);
                }
                else if(0 == app_channel_list_compare_dialog(WND_CHANNEL_LIST) && LIST_MODE_PROG == app_get_channel_list_mode())
                {
                    GUI_SetProperty("listview_channel_list", "update_all", NULL);
                    GUI_SetProperty("listview_channel_list", "select", &pos);
                    GUI_SetProperty("listview_channel_list", "active", &pos);
                }
#endif
                app_epg_reset_for_acu();
            }
            break;
        default:
            break;
    }
    return 0;
}

#endif

#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT || BOUQUET_SUPPORT
void app_auto_update_send_ui_msg(AcuStateType type,int sec, const char* string)
{
    GxMessage   *new_msg = NULL;
    AppMsgAcuStateUiCotrol* param=NULL;
    new_msg = GxBus_MessageNew(APPMSG_ACU_STATE_REPORT);
    param = (AppMsgAcuStateUiCotrol*)GxBus_GetMsgPropertyPtr(new_msg, AppMsgAcuStateUiCotrol);
    if(param != NULL)
    {
        param->type = type;
        if(ACU_UPDATE_START == type)
        {
            param->timeout = sec;
        }
        else
        {
            param->version = sec;
        }
        param->show_str = (char*)string;
        GxBus_MessageSend(new_msg);
    }
    return;
}

extern void app_epg_preprocess_for_pmt(void);
int app_auto_update_msg_proc(GxMessage *msg)
{
    AppMsgAcuStateUiCotrol* propmtinfo = GxBus_GetMsgPropertyPtr(msg, AppMsgAcuStateUiCotrol);
    switch(propmtinfo->type)
    {
#if BOUQUET_SUPPORT
        case ACU_UPDATE_BAT:
            {
                const char *wnd = GUI_GetFocusWindow();
                if(NULL == wnd)
                    return -1;
                if(0 == strcmp(wnd,WND_BOUQUET))
                {
                    extern void app_bouquet_backup_refresh(int type, uint32_t pos);
                    app_bouquet_backup_refresh(2,0);
                }
            }
            break;
#endif
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT
        case ACU_UPDATE_PMT:
            if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
                app_epg_preprocess_for_pmt();
            app_play_replay_from_pmt(50);
            break;
#endif
        default:
#if AUTO_UPDATE_SUPPORT
            app_auto_update_prog_msg_proc(msg);
#endif
            break;
    }
    return 0;
}
#endif
