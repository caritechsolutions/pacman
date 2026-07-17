#include "app_config.h"
#include "module/app_log.h"

#if PDMX_SUPPORT
#include "app_data_type.h"
#include "app_msg.h"
#include "app_pdmx.h"
#include "pdmx.h"
#include "service/gxsearch.h"
#include "service/gxfrontend.h"
#include "module/config/gxconfig.h"

#define PDMX_T2MI_FUNC
//#define PDMX_ULE_FUNC

typedef struct _PDMXT2MISlotClass{
    unsigned short pid;
    unsigned char plp_id;
    unsigned char plp_type;
}PDMXT2MISlotClass;

typedef struct _PDMXInfoUnitClass{
    unsigned char ts_package_size;
    unsigned int t2mi_num;
    PDMXT2MISlotClass *t2mi_info;
    unsigned int ule_num;
    ULEClass *ule_info;
    unsigned int mpe_num;
    MPEClass *mpe_info;
    unsigned int normal_ts_num;
}PDMXInfoUnitClass;

typedef struct _PDMXInfoClass{
    PDMXInfoUnitClass info;
    int index;
    int total;
}PDMXInfoClass;

static PDMXInfoClass thiz_pdmx_info;

static int __init_params(PDMXProbeClass *info)
{
    int ret = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    thiz_pdmx_info.info.ts_package_size = info->ts_package_size;

#ifdef PDMX_T2MI_FUNC
    // t2mi
    if(0 < info->t2mi_num)
    {
        if(thiz_pdmx_info.info.t2mi_info)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            goto err;
        }

        // one plp one slot
        for(i = 0; i < info->t2mi_num; i++)
        {
            thiz_pdmx_info.info.t2mi_num += info->t2mi_info[i].plp_num;
        }

        if(0 == thiz_pdmx_info.info.t2mi_num)
        {// have no plp num
            thiz_pdmx_info.info.t2mi_num = 1;
        }
        thiz_pdmx_info.info.t2mi_info = GxCore_Mallocz(thiz_pdmx_info.info.t2mi_num*sizeof(PDMXT2MISlotClass));
        if(NULL == thiz_pdmx_info.info.t2mi_info)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            goto err;
        }

        for(i = 0; i < info->t2mi_num; i++)
        {
            for(j = 0; j < info->t2mi_info[i].plp_num; j++)
            {
                thiz_pdmx_info.info.t2mi_info[k].pid = info->t2mi_info[i].pid;
                thiz_pdmx_info.info.t2mi_info[k].plp_id = info->t2mi_info[i].plp_id[j];
                k++;
            }
        }
        thiz_pdmx_info.total += thiz_pdmx_info.info.t2mi_num;
    }
#endif
#ifdef PDMX_ULE_FUNC
    // ule
    if(0 < info->ule_num)
    {
        if(thiz_pdmx_info.info.ule_info)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            goto err;
        }
        thiz_pdmx_info.info.ule_info = GxCore_Mallocz(info->ule_num*sizeof(ULEClass));
        if(NULL == thiz_pdmx_info.info.ule_info)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            goto err;
        }
        thiz_pdmx_info.info.ule_num = info->ule_num;
        memcpy(thiz_pdmx_info.info.ule_info, info->ule_info, (info->ule_num*sizeof(ULEClass)));

        for(i = 0; i < info->ule_num; i++)
        {
            memcpy(&thiz_pdmx_info.info.ule_info[i], &info->ule_info[info->ule_num - 1 - i], sizeof(ULEClass));
        }
        // one pid one slot
        thiz_pdmx_info.total += info->ule_num;
    }
#endif
    // TODO: other protocol
    if(188 != info->ts_package_size)
    {
        thiz_pdmx_info.total += 1;
    }
    // normal ts
    if(1 ==  (info->type & 1))
    {
        thiz_pdmx_info.info.normal_ts_num = 1;
        thiz_pdmx_info.total += 1;
    }

    return ret;
err:
    if(thiz_pdmx_info.info.t2mi_info)
    {
        GxCore_Free(thiz_pdmx_info.info.t2mi_info);
        thiz_pdmx_info.info.t2mi_info = NULL;
    }
    if(thiz_pdmx_info.info.ule_info)
    {
        GxCore_Free(thiz_pdmx_info.info.ule_info);
        thiz_pdmx_info.info.ule_info = NULL;
    }
    if(thiz_pdmx_info.info.mpe_info)
    {
        GxCore_Free(thiz_pdmx_info.info.mpe_info);
        thiz_pdmx_info.info.mpe_info = NULL;
    }
    return -1;
}

static int _pdmx_exit(int tuner, void* params)
{
    int ret = 0;

    if(0 >= thiz_pdmx_info.total)
    {
        return 0;
    }

    int pdmx_switch = 0;
    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if((GxFrontend_HasT2MIFeatures(tuner))
            && (pdmx_switch == GXBUS_FRONTEND_PDMX_AUTO))
	{
		GxFrontend_set_T2MI_contrl(tuner, 0);
	}
    if(GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
    {
        // stop pdmx module
        pdmx_stop();
    }

    if((0 < thiz_pdmx_info.info.t2mi_num)
            && NULL != thiz_pdmx_info.info.t2mi_info)
    {
        GxCore_Free(thiz_pdmx_info.info.t2mi_info);
    }
    if((0 < thiz_pdmx_info.info.ule_num)
            && NULL != thiz_pdmx_info.info.ule_info)
    {
        GxCore_Free(thiz_pdmx_info.info.ule_info);
    }
    if((0 < thiz_pdmx_info.info.mpe_num)
            && NULL != thiz_pdmx_info.info.mpe_info)
    {
        GxCore_Free(thiz_pdmx_info.info.mpe_info);
    }
    memset(&thiz_pdmx_info, 0, sizeof(PDMXInfoClass));

    //app_log_flow("\n%s, %d\n", __func__, __LINE__);
    return ret;
}

static int _pdmx_init(int tuner,int demux_id,  void* params)
{
    int ret = 0;
    int pdmx_switch = 0;

    PDMXProbeClass *info = (PDMXProbeClass*)GxCore_Mallocz(sizeof(PDMXProbeClass));
    if(NULL == info)
    {
        app_log_error("\nERRROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);

    info->dmx_id = demux_id;
    info->style =  PDMX_S_ALL;

    if(pdmx_probe(info) < 0)
    {
        app_log_error("pdmx_probe failed\n");
        return -1;
    }

    app_log_debug("\033[31mTSInfo type:%d size:%d t2mi_num:%d ule_num:%d mpe_num=%d\n\033[0m",
            info->type, info->ts_package_size, info->t2mi_num, info->ule_num, info->mpe_num);

    // init params
    __init_params(info);

    GxCore_Free(info);

    return ret;
}

// return 1: next slot; return 0: finish
static int _pdmx_set_params(int tuner, int ts_source, int demux_id, void* params)
{
    int ret = 1;
	unsigned short pid = 0;
	unsigned char plp_id = 0;
    int pdmx_switch = 0;
    PDMXLevelInforUnitClass info = {0};
    unsigned char hard_mode = 0;
    unsigned int normal_num = thiz_pdmx_info.info.normal_ts_num > 0?1:0;

    if((0 == thiz_pdmx_info.index) && (0 < normal_num))
    {// normal ts mode
        thiz_pdmx_info.index++;
        return 0;
    }

next_index:
    if(thiz_pdmx_info.index >= thiz_pdmx_info.total)
    {// finish
        return 0;
    }

    info.output_info.demux_id = demux_id;
    info.output_info.demux_source = 3;
    info.input_info.demux_id = 1;// need control
    info.input_info.demux_source = ts_source;
    // for protect
    info.protocol = PDMX_P_TOTAL;

    // for 19x ts
    if(thiz_pdmx_info.info.ts_package_size > 188)
    {
        // init params
        info.protocol = PDMX_P_TS_DEAL;
        info.u.ts_deal.ts_size = thiz_pdmx_info.info.ts_package_size;
    }

#ifdef PDMX_T2MI_FUNC
    // check for t2mi
    if((0 < thiz_pdmx_info.info.t2mi_num)
            && ((thiz_pdmx_info.index - normal_num) < thiz_pdmx_info.info.t2mi_num))
    {
	    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
        pid = thiz_pdmx_info.info.t2mi_info[thiz_pdmx_info.index - normal_num].pid;
        plp_id = thiz_pdmx_info.info.t2mi_info[thiz_pdmx_info.index - normal_num].plp_id;

        if((GXBUS_FRONTEND_PDMX_AUTO == pdmx_switch)
            &&(GxFrontend_HasT2MIFeatures(tuner))) // HARD T2MI
        {
            //app_log_debug("HARD T2MI index = %d, t2mi pid = 0x%x plpid = %d\n", thiz_pdmx_info.index, pid, plp_id);
            GxFrontend_set_T2MI_contrl(tuner, 1);
            GxFrontend_T2miSetPlp(tuner, pid, plp_id);
            hard_mode = 1;
        }
        else
        {
            //app_log_debug("SOFT T2MI index = %d, t2mi pid = 0x%x plpid = %d\n", thiz_pdmx_info.index, pid, plp_id);
        }

        // init params
        if(0 == hard_mode)
        {
            info.protocol = PDMX_P_T2MI;
            info.u.t2mi_infor.pid = pid;
            info.u.t2mi_infor.plp_id = plp_id;
        }
    }
#endif
#ifdef PDMX_ULE_FUNC
    //cheack for ule
    if((0 < thiz_pdmx_info.info.ule_num)
            && ((thiz_pdmx_info.index - normal_num) >= thiz_pdmx_info.info.t2mi_num)
            && ((thiz_pdmx_info.index - normal_num) < (thiz_pdmx_info.info.t2mi_num + thiz_pdmx_info.info.ule_num)))
    {
        pid = thiz_pdmx_info.info.ule_info[thiz_pdmx_info.index - normal_num - thiz_pdmx_info.info.t2mi_num].pid;
        // init params
        info.protocol = PDMX_P_ULE;
        info.u.pid_infor.pid = pid;
        // key control
#if 0
        if(params)
        {
            PDMXSignalInfoClass *signal_info = (PDMXSignalInfoClass*)params;
            char *keys = NULL;

            unsigned short sat_longitude = signal_info->longitude;
            unsigned int fre = signal_info->fre_m;
            unsigned int pol = signal_info->pol;
            unsigned int sym = signal_info->sym_k;

            keys = app_pdmx_get_keys(pid, sat_longitude, fre, pol, sym);
            if(keys)
            {
                info.u.pid_infor.key_num = 1;
                memcpy(info.u.pid_infor.keys[0].keys, keys, 16);
                info.u.pid_infor.keys[0].key_size = 16;
                info.u.pid_infor.keys[0].key_type = PDMX_CSA;
                info.u.pid_infor.keys[0].key_mode = PDMX_NULL;
            }
        }
#endif
    }
#endif

    // TODO: others protocols
    //
    if(0 == hard_mode)
    {
        if((info.protocol < PDMX_P_TOTAL) && (0 <= info.protocol))
        {
            pdmx_stop();
            pdmx_start(&info);
        }
        else
        {// for next pdmx mode
            thiz_pdmx_info.index++;
            goto next_index;
        }
        //app_log_flow("\n%s, %d, pdmx start\n", __func__, __LINE__);
    }
    thiz_pdmx_info.index++;
    return ret;
}

static int _pdmx_get_params(void* params)
{
    int ret = 0;
    GxBusPmDataProg *p = NULL;
    int index = thiz_pdmx_info.index - 1;
    unsigned int normal_num = thiz_pdmx_info.info.normal_ts_num > 0?1:0;
    if((NULL == params) || (index < 0))
    {
        return -1;
    }
    p = (GxBusPmDataProg*)params;
    // t2mi
    if((0 < thiz_pdmx_info.info.t2mi_num)
            && (index < (thiz_pdmx_info.info.t2mi_num + normal_num))
            && (normal_num > 0?(index > 0):1)
            && (NULL != thiz_pdmx_info.info.t2mi_info))
    {
        p->pdmx_flag = 0x1;
        p->pdmx_pid = thiz_pdmx_info.info.t2mi_info[index-normal_num].pid;
        p->common_plp_id = thiz_pdmx_info.info.t2mi_info[index-normal_num].plp_id;
    }
    // ule
    if((0 < thiz_pdmx_info.info.ule_num)
            && (index < (thiz_pdmx_info.info.t2mi_num + thiz_pdmx_info.info.ule_num + normal_num))
            && ((index + 1) >= (thiz_pdmx_info.info.t2mi_num + normal_num))
            && (NULL != thiz_pdmx_info.info.ule_info))
    {
        p->pdmx_flag = 0x2;
        p->pdmx_pid = thiz_pdmx_info.info.ule_info[index - thiz_pdmx_info.info.t2mi_num - normal_num].pid;
    }

    // TODO:
    return ret;
}

static int _pdmx_get_ts_size(void)
{
    int ret = 188;

    if(188 <= thiz_pdmx_info.info.ts_package_size)
    {
        ret = thiz_pdmx_info.info.ts_package_size;
    }
    return ret;
}

static int _pdmx_get_active_flag(void)
{
    int ret = 0;
    unsigned int normal_num = thiz_pdmx_info.info.normal_ts_num > 0?1:0;

    if((0 < thiz_pdmx_info.total)
            && (thiz_pdmx_info.total >= thiz_pdmx_info.index)
            && (normal_num > 0?(thiz_pdmx_info.index > 1):1))
    {
        ret = 1;
    }
    return ret;
}



static PDMXSearchClass thiz_pdmx_ops = {
    .init = _pdmx_init,
    .exit = _pdmx_exit,
    .set_params = _pdmx_set_params,
    .get_params = _pdmx_get_params,
    .get_ts_size = _pdmx_get_ts_size,
    .get_active_flag = _pdmx_get_active_flag,
};

void app_pdmx_register(void)
{
    int pdmx_switch = 0;
	GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if(GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
    {
        GxSearchRegistPDMX(&thiz_pdmx_ops);
    }
}

void app_pdmx_unregister(void)
{
    int pdmx_switch = 0;

	GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if(GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
        GxSearchUnregistPDMX();

    return;
}

static char thiz_pdmx_active_flag = 0;
void app_pdmx_play_stop(int tuner)
{
    int pdmx_switch = 0;
    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if(GxFrontend_HasT2MIFeatures(tuner)
            && (GXBUS_FRONTEND_PDMX_AUTO == pdmx_switch))
    {
        GxFrontend_set_T2MI_contrl(tuner, 0);
        GxFrontend_T2miSetPlp(tuner, 0xffff, 0xff);
        app_log_info("normal_play_stop stage:0\n");
    }

    if(GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
    {
        pdmx_stop();
    }
    thiz_pdmx_active_flag = 0;
}

void app_pdmx_play_start(int tuner, int ts_source, int demux_id, int ts_size,  void *prog_params)
{
    int pdmx_switch = 0;
    GxBusPmDataProg *p = NULL;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if((GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
            || (NULL != prog_params))
    {
        PDMXLevelInforUnitClass info = {0};
        unsigned char hard_support = 0;
        unsigned char pdmx_flag = 0;
        memset(&info, 0, sizeof(PDMXLevelInforUnitClass));
        p = (GxBusPmDataProg*)prog_params;
        info.output_info.demux_id = demux_id;
        info.output_info.demux_source = 3;
        info.input_info.demux_id = 1;// need control
        info.input_info.demux_source = ts_source;
        if(p->pdmx_flag == 1)
        {
            info.protocol = PDMX_P_T2MI;
            info.u.t2mi_infor.plp_id = p->common_plp_id;
            info.u.t2mi_infor.pid = p->pdmx_pid;
            if(GxFrontend_HasT2MIFeatures(tuner)
                    && (GXBUS_FRONTEND_PDMX_AUTO == pdmx_switch))
            {
                hard_support = 1;
            }
            pdmx_flag = 1;
        }
        else if(p->pdmx_flag == 2)
        {
            info.protocol = PDMX_P_ULE;
            info.u.pid_infor.pid = p->pdmx_pid;
            // TODO: add by client
#if 0
            // key control
            {
                char *keys = NULL;
                GxMsgProperty_NodeByIdGet node = {0};
                unsigned short sat_longitude = 0;
                unsigned int fre = 0;
                unsigned int pol = 0;
                unsigned int sym = 0;

                node.node_type = NODE_SAT;
                node.id = p->sat_id;
                if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
                {
                    sat_longitude = (node.sat_data.sat_s.longitude_direct > 0) ? (3600 - node.sat_data.sat_s.longitude):node.sat_data.sat_s.longitude;
                }
                node.node_type = NODE_TP;
                node.id = p->tp_id;
                if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
                {
                    fre = node.tp_data.frequency;
                    pol = node.tp_data.tp_s.polar;
                    sym = node.tp_data.tp_s.symbol_rate;
                }

                keys = app_pdmx_get_keys(p->pdmx_pid, sat_longitude, fre, pol, sym);
                if(keys)// odd+even
                {
                    info.u.pid_infor.key_num = 1;
                    memcpy(info.u.pid_infor.keys[0].keys, keys, 16);
                    info.u.pid_infor.keys[0].key_size = 16;
                    info.u.pid_infor.keys[0].key_type = PDMX_CSA;
                    info.u.pid_infor.keys[0].key_mode = PDMX_NULL;
                }
            }
#endif
            pdmx_flag = 1;
        }
        else if(ts_size > 188)
        {
            info.protocol = PDMX_P_TS_DEAL;
            info.u.ts_deal.ts_size = ts_size;
            pdmx_flag = 1;
        }

        info.output_info.output_track_num = 0;
        info.output_info.tracks[0] = p->video_pid;
        info.output_info.tracks[1] = p->cur_audio_pid;
        info.output_info.tracks[2] = p->pcr_pid;
        info.output_info.tracks[3] = p->pmt_pid;
        if((0 == hard_support)
                && (1 == pdmx_flag))
        {
            pdmx_stop();
            pdmx_start(&info);
        }
        if(hard_support || pdmx_flag)
        {
            thiz_pdmx_active_flag = 1;
        }
    }
}

void app_pdmx_prepare(int tuner, void* prog_params)
{
    int pdmx_switch = 0;
    GxBusPmDataProg *p = NULL;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if(GxFrontend_HasT2MIFeatures(tuner)
            && (GXBUS_FRONTEND_PDMX_AUTO == pdmx_switch))
    {
        if(NULL == prog_params)
        {
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            return ;
        }
        p = (GxBusPmDataProg*)prog_params;

        if(p->pdmx_flag == 0x1)
        {// 1:t2mi, 2: ule, 3: others
            GxFrontend_set_T2MI_contrl(tuner, 1);
            GxFrontend_T2miSetPlp(tuner, p->pdmx_pid, p->common_plp_id);
            app_log_debug(" plpid = %d\n",  p->common_plp_id);
        }
        else
        {
            GxFrontend_set_T2MI_contrl(tuner, 0);
            GxFrontend_T2miSetPlp(tuner, 0xffff, 0xff);
            app_log_debug("normal_play\n");
        }
    }

    if(GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
    {
        pdmx_stop();
    }

    thiz_pdmx_active_flag = 0;
}

void app_pdmx_start_ts_deal(int ts_src, int dmx_id, int ts_size)
{
    int pdmx_switch = 0;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    if(GXBUS_FRONTEND_PDMX_OFF == pdmx_switch)
    {
        return ;
    }

    if(ts_size > 188)
    {
        PDMXLevelInforUnitClass info = {0};
        info.output_info.demux_id = dmx_id;
        info.output_info.demux_source = 3;
        info.input_info.demux_id = dmx_id==0?1:0;
        info.input_info.demux_source = ts_src;
        info.protocol = PDMX_P_TS_DEAL;
        info.u.ts_deal.ts_size = ts_size;
        pdmx_stop();
        pdmx_start(&info);
        thiz_pdmx_active_flag = 1;
    }
    else
    {
        pdmx_stop();
        thiz_pdmx_active_flag = 0;
    }
}

void app_pdmx_stop(void)
{
    pdmx_stop();
    thiz_pdmx_active_flag = 0;
}

int app_pdmx_get_active_flag(void)
{
    return thiz_pdmx_active_flag;
}

#endif

