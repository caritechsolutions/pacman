#include "app_config.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_module.h"
#include "include/descrambler.h"

#if CA_SUPPORT
#include "gxcore.h"

#ifndef _EXTEND_DEBUG_
#define _EXTEND_DEBUG_
#endif

#ifdef _EXTEND_DEBUG_
#define ExtendPrintf(...) app_log_debug("[Extend]"__VA_ARGS__)
#else
#define ExtendPrintf(...) while(0)
#endif

enum {
    EXSHIFT_FLAG = 0,
    CMM_FLAG,
    NORMAL_FLAG,
    TOTAL_FLAG,
};
static int thiz_ca_flag[TOTAL_FLAG] = {0, 0, 0};
#if CMM_SUPPORT
// TODO:
#endif

// 1: vaild; 0: invaild


// 1: need emm data; 0: need not emm data; -1: error
int app_get_emm_status(unsigned short cas_id)
{
    int ret = 0;
    // TODO: 20160125
    return ret;
}

// 1: need ecm data; 0: need not ecm data ; -1: error
int app_get_ecm_status(unsigned short cas_id)
{
    int ret = 0;
    // TODO: 20160125
    return ret;
}

// data edit
int app_edit_data(EditInfoClass info , ServiceClass data)
{
    int ret = 0;
    // TODO: 20160125
    return ret;
}


int app_send_prepare(ServiceClass *param)
{
    int ret = 0;
    if(NULL == param)
    {
        return -1;
    }
#if CMM_SUPPORT
    // TODO:
#endif
    return ret;
}

int app_send_service(ServiceClass *param, ControlClass *control_info)
{
    int ret = 0;
    if((NULL == control_info) || (NULL == param))
    {
        return -1;
    }
#if CMM_SUPPORT
    // TODO:
#endif
    return ret;
}

int app_send_psi_data(DataInfoClass *data_info, ControlClass *control_info)
{
    int ret = 0;

    if((NULL == control_info) || (NULL == data_info))
    {
        return -1;
    }
#if CMM_SUPPORT
    // TODO:
#endif
    return ret;
}

int app_extend_register(void)
{
    int ret = 0;
#if CMM_SUPPORT
    // TODO: 
#endif
    return ret;
}


#if EX_SHIFT_SUPPORT
static int _exshift_check(void)
{
	int init_value = 0;
	GxBus_ConfigGetInt(EXSHIFT_KEY, &init_value, EXSHIFT_DEFAULT);
	if(init_value && thiz_ca_flag[EXSHIFT_FLAG])
		return TRUE;
	else
		return FALSE;
}

static int _exshift_time_get(void)//second
{
    int time = 0;
	GxBus_ConfigGetInt(EXSHIFT_TIME, &time, EXSHIFT_TIME_DEFAULT);
	return time;
}

void app_exshift_enable(void)
{
	int ValueTemp = 1;
	GxBus_ConfigSetInt(EXSHIFT_KEY, ValueTemp);
}

void  app_exshift_disable(void)
{
	int ValueTemp = 0;
	GxBus_ConfigSetInt(EXSHIFT_KEY, ValueTemp);
}

void  app_exshift_set_time(int Time)//second
{
	int ValueTemp = Time;

	GxBus_ConfigSetInt(EXSHIFT_TIME, ValueTemp);
}
#endif

int app_exshift_pid_add(int Num, int* pData)
{
    int i;
	GxMsgProperty_PlayerTrackAdd Params = {0};
	if((PLAYER_MAX_TRACK_ADD < Num) || (NULL == pData))
	{
		app_log_error("\nCA,parameters error, %s,%d\n",__FILE__,__LINE__);
		return -1;
	}
    Params.player = PLAYER_FOR_NORMAL;
    Params.track.num = Num;
    for(i=0 ;i <Num;i++)
    	Params.track.pid[i] = (short)(pData[i]);

    app_send_msg_exec(GXMSG_PLAYER_TRACK_ADD, (void *)(&Params));
	return 0;
}

int app_exshift_pid_delete(int Num, int* pData)
{
	GxStreamTrackDel Params = {0};
    if((PLAYER_MAX_TRACK_ADD < Num) || (NULL == pData))
	{
		app_log_error("\nCA,parameters error, %s,%d\n",__FILE__,__LINE__);
		return -1;
	}

    Params.num = Num;
    memcpy(Params.pid, pData, Num*sizeof(int));
    GxPlayer_MediaTrackDel(PLAYER_FOR_NORMAL, &Params);
    return 0;
}

int app_tsd_check(void)
{
    int exshift = 0;
    int cmshift = 0;
#if EX_SHIFT_SUPPORT
    exshift = _exshift_check();
#endif
#if CMM_SUPPORT
    // TODO:
#endif
    if(exshift || cmshift)
    {
        return 1;// support tsd
    }
    return 0;
}

int app_tsd_time_get(void)
{
    unsigned int exshift_time = 0xffffffff;
    unsigned int cmshift_time = 0xffffffff;
    unsigned int time = 0;
#if EX_SHIFT_SUPPORT
    if( _exshift_check())
    {
        exshift_time = _exshift_time_get();
    }
#endif
#if CMM_SUPPORT
    // TODO:
#endif
    if(((exshift_time != 0xffffffff) && (exshift_time != 0))
                || ((cmshift_time != 0xffffffff) && (cmshift_time != 0)))
    {
        if((exshift_time > 0) && (cmshift_time > 0))
        {
            time = (exshift_time > cmshift_time ? cmshift_time: exshift_time);
        }
        else
        {
            time = exshift_time + cmshift_time;
            time = ((time == 0xffffffff) ? 0:time);
        }
    }
    return time;
}

int app_demux_param_adjust(unsigned short *ts_source, unsigned short *demux_id, unsigned char ex_flag)
{
    int ret = 0;
    int exshift_flag = 0;
    int cmshift_flag = 0;

    if((NULL == ts_source) || (NULL == demux_id))
    {
        return -1;
    }
#if EX_SHIFT_SUPPORT
    exshift_flag = _exshift_check();
#endif
#if CMM_SUPPORT
    // TODO:
#endif
    if(exshift_flag || cmshift_flag)
    {
        *ts_source = 3;// from sdram
    }

    return ret;
}

int app_extend_control_pvr_change(void)
{
    uint32_t current_pvr_id = g_AppPvrOps.env.prog_id;
	uint8_t ret = 0;

    GxMsgProperty_NodeByIdGet node = {0};
    node.node_type = NODE_PROG;
    node.id = current_pvr_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

    if (GXBUS_PM_PROG_BOOL_ENABLE == node.prog_data.scramble_flag)
    {
        ret |= app_tsd_check();
#if (DUAL_CHANNEL_DESCRAMBLE_SUPPORT == 0)
        ret |= thiz_ca_flag[NORMAL_FLAG];
#endif
    }

	return ret;
}

void app_ca_set_flag(unsigned char index)
{
    if(index >= TOTAL_FLAG)
    {
        app_log_error("\nCA, error, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
	thiz_ca_flag[index] = 1;
}

void app_ca_clear_flag(void)
{
    int  i =0;
    for(i = 0; i < TOTAL_FLAG; i++)
    {
	    thiz_ca_flag[i] = 0;
    }
}

static ServiceClass thiz_service_info[CONTROL_TOTAL] = {{0}, {0}};
static ControlClass thiz_control_info[CONTROL_TOTAL] = {{0}, {0}};
void app_extend_send_service(unsigned int prog_id, unsigned int sat_id, unsigned int tp_id, unsigned int demux_id, ControlTypeEnum type)
{
    ServiceClass param = {0};
    ControlClass info = {0};
    GxMsgProperty_NodeByIdGet node = {0};

    param.longitude = 0xffff;
    param.direction = 0xffff;
    param.fre = 0xffff;
    param.polar = 0xffff;
    param.symb = 0xffff;

    node.node_type = NODE_PROG;
    node.id = prog_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        param.service_id = node.prog_data.service_id;
        param.cas_id = node.prog_data.cas_id;
        param.video_pid = node.prog_data.video_pid;
        param.audio_pid = node.prog_data.cur_audio_pid;
    }
    else
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    node.node_type = NODE_SAT;
    node.id = sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        if(node.sat_data.type == GXBUS_PM_SAT_S)
        {
            param.longitude = node.sat_data.sat_s.longitude;
            param.direction = node.sat_data.sat_s.longitude_direct;// 0: east; 1: west
        }
        // TODO...
    }
    else
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    node.node_type = NODE_TP;
    node.id = tp_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
    {
        param.fre = node.tp_data.frequency;
        if(param.longitude != 0xffff)
        {
            param.polar = node.tp_data.tp_s.polar;
            param.symb  = node.tp_data.tp_s.symbol_rate;
        }
        // TODO
    }
    else
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }

    //
    info.demux_id = demux_id;
    info.control_type = type;
    info.data_type = DATA_TYPE_UNIVERSAL;
    //

    app_descrambler_close_by_control_type(type);
    app_descrambler_init(demux_id, type, DATA_TYPE_VIDEO);
    app_descrambler_init(demux_id, type, DATA_TYPE_AUDIO);
    app_descrambler_set_pid(param.video_pid, type, DATA_TYPE_VIDEO);
    app_descrambler_set_pid(param.audio_pid, type, DATA_TYPE_AUDIO);
    //
    if(app_send_service(&param, &info) == 0)
    {
        memcpy(&thiz_service_info[type], &param, sizeof(ServiceClass));
        memcpy(&thiz_control_info[type], &param, sizeof(ControlClass));
    }
}

void app_extend_change_audio(unsigned short cur_audio_pid, unsigned short audio_pid, ControlTypeEnum control_type)
{
    unsigned char even[8] = {0};
    unsigned char odd[8]  = {0};
    int handle = 0;
    if((cur_audio_pid == 0x1fff) || (audio_pid == 0x1fff) || (control_type >= CONTROL_TOTAL))
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    app_descrambler_get_cw_by_pid(cur_audio_pid,
                                    control_type,
                                    DATA_TYPE_AUDIO,
                                    even, odd);
    handle = app_descrambler_get_handle_by_pid(cur_audio_pid,
                                                    control_type,
                                                    DATA_TYPE_AUDIO);
    app_descrambler_set_pid_by_handle(handle, audio_pid);

    thiz_service_info[control_type].audio_pid = audio_pid;
    app_send_service(&thiz_service_info[control_type], &thiz_control_info[control_type]);
}

void app_extend_add_element(unsigned short pid, ControlTypeEnum control_type, DataTypeEnum data_type)
{
   // unsigned char even[8] = {0};
   // unsigned char odd[8]  = {0};
    //int handle = 0;
    if((pid == 0x1fff) || (control_type >= CONTROL_TOTAL))
    {
        app_log_error("\nERROR, %s, %d\n", __FUNCTION__,__LINE__);
        return ;
    }
    app_descrambler_init(g_AppPlayOps.normal_play.dmx_id, CONTROL_NORMAL, data_type);
    app_descrambler_set_pid(pid, CONTROL_NORMAL, data_type);

    app_send_service(&thiz_service_info[control_type], &thiz_control_info[control_type]);
}

#endif


