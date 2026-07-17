#include "app.h"
#include "app_data_type.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "module/config/gxconfig.h"
#include "app_epg.h"
#include "app_module.h"


#if MINI_MEM_SUPPORT
#define EPG_SERVICE_CNT 100
#define EPG_EVENT_CNT 3000
#define SW_BUFFER_SIZE 64*1024
#define EPG_PER_EVENT_SIZE 512 //--- add new 20131228
#else
#define EPG_SERVICE_CNT 500
#define EPG_EVENT_CNT 6000
#define SW_BUFFER_SIZE 256*1024
#define EPG_PER_EVENT_SIZE 1024
#endif

#define EPG_DAY_CNT (MAX_EPG_DAY + 1)
#define SEC_PER_DAY (60*60*24)

#define ONE_HOUR (60 * 60 * 1000)
#define MAX_CACHE_COUNT      50

#define PER_READ_SIZE_16K (16 * 1024)
#define PER_READ_SIZE_28K (28 * 1024)
struct epg_switch
{
    uint32_t ts_src;
    uint32_t dmx_id;
    uint32_t flag;
#define INVALID_TS  (~(uint32_t)(0))
#define INVALID_DMX  (~(uint32_t)(0))
#define EPG_ON      1
#define EPG_OFF     0
}s_Switch = {INVALID_TS, INVALID_DMX, EPG_OFF};

static bool thiz_epg_filter_pause_flag = false;
static event_list *thiz_epg_section_timer = NULL;

static status_t epg_section_flag_clear(void *arg)
{
    if(EPG_ON == s_Switch.flag)
        app_send_msg_exec(GXMSG_EPG_SECTION_FLAG_RELEASE, (void *)NULL);

    return GXCORE_SUCCESS;
}

static status_t app_epg_event_cmp(GxBusEpgEventComp* src,GxBusEpgEventComp* cur)
{
    if(src->service_id == cur->service_id
            && src->ts_id == cur->ts_id)
    {
        return GX_BUS_EPG_OK;
    }

    return GX_BUS_EPG_ERR;
}

#define EPG_6937_SUPPORT 1

#if EPG_6937_SUPPORT
unsigned char* epg_lang_recode_event_info(unsigned char* event_info,int len)
{
    unsigned int relen = 0;
    int i = 1;
    unsigned char* iconv_str = NULL;

    if((event_info!=NULL)&&(event_info[0] > 0x20))
    {
        char* encoding_name = NULL;
        int init_value;
        char *lang = NULL;
        encoding_name = (char*)GxCore_Malloc(12);
        if(NULL == encoding_name)
        {
            app_log_error("No enough memory\n");
            return NULL;
        }

        memset(encoding_name,0,12);
        GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
        lang = app_get_epg_short_lang_str(init_value);
        if(strcmp(lang,"ara") ==0)
        {
            i = 6;
            sprintf(encoding_name,"iso8859-0%d",i);
        }
        else if(strcmp(lang,"eng") == 0)
        {
            i = 1;
            sprintf(encoding_name,"iso8859-0%d",i);//具体前缀有具体地区决定，1为西欧，6为阿拉伯
        }
        else if(strcmp(lang,"tur") == 0)
        {
            i = 9;
            sprintf(encoding_name,"iso8859-0%d",i);//具体前缀有具体地区决定，1为西欧，6为阿拉伯
        }
        else if(strcmp(lang,"cze") == 0)/*lartin 2 for 斯拉夫/中欧/捷克*/
        {
            i = 2;
            sprintf(encoding_name,"iso8859-0%d",i);
        }
#if FONT_GB2312_TO_UTF8
        else if((strcmp(lang, "all") == 0)||(strcmp(lang, "chi") == 0))
        {
            sprintf(encoding_name,"gb2312");
        }
#else
        else if(strcmp(lang, "all") == 0) //#54641
        {
            GxCore_Free(encoding_name);
            encoding_name = NULL;
        }
#endif

        gdi_encoding_convert("iso6937",encoding_name);//将iso6937的数据转换为encoding_name的编码方式解码。
        if(gxgdi_iconv(event_info,&iconv_str,len,(unsigned int *)&relen,NULL) == 1)
        {
            iconv_str = NULL;
            if(encoding_name)
                GxCore_Free(encoding_name);
            return NULL;
        }
        gdi_encoding_convert("iso6937",NULL);//还原
        if(encoding_name)
        {
            GxCore_Free(encoding_name);
        }
        return iconv_str;
    }
    else
    {
        if(gxgdi_iconv(event_info,&iconv_str,len,&relen,NULL) == 1)
        {
            iconv_str = NULL;
            relen = 0;
            return NULL;
        }
        return iconv_str;
    }
}
#endif

void app_epg_enable(uint32_t ts_src, uint32_t dmx_id)
{
    GxMsgProperty_EpgCreate epgCreate;
    GxMsgProperty_NodeByPosGet node = {0};
    int init_value;
    char *lang = NULL;
    int epg_ctr = 0;

    if(s_Switch.flag == EPG_ON)
        return;

    s_Switch.flag = EPG_ON;

    if(ts_src == s_Switch.ts_src
            && dmx_id == s_Switch.dmx_id)
        return;

    epgCreate.ts_src = ts_src;
    epgCreate.dmx_id = dmx_id;

    GxBus_ConfigSetInt(EPG_SERVICE_FILTER_MODE, EPG_SERVICE_FILTER_MODE_MAX);
    GxBus_ConfigSetInt(EPG_INFO_CLEAN_KEY, EPG_INFO_CLEAN_TP_CHANGE);
    GxBus_ConfigGetInt(EPG_INFO_CLEAN_KEY, &epg_ctr, EPG_INFO_CLEAN_TP_CHANGE);

    epgCreate.sw_buffer_size = SW_BUFFER_SIZE;
    epgCreate.service_count = EPG_SERVICE_CNT;
    epgCreate.event_count = EPG_EVENT_CNT;
    epgCreate.event_size = EPG_PER_EVENT_SIZE;
    epgCreate.epg_day = MAX_EPG_DAY;
    if(EPG_INFO_CLEAN_TP_CHANGE == epg_ctr)
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
        epgCreate.ts_id = node.prog_data.ts_id;
        epgCreate.cur_tp_only = 1;
        GxBus_ConfigSetInt(EPG_CONFIG_PER_READ_SIZE, PER_READ_SIZE_16K);
    }
    else
    {
        epgCreate.ts_id = 0xffffffff;
        epgCreate.cur_tp_only = 0;
        GxBus_ConfigSetInt(EPG_CONFIG_PER_READ_SIZE, PER_READ_SIZE_28K);
    }

    epgCreate.service_id = 0xffffffff;
    epgCreate.cache_gate = MAX_CACHE_COUNT;
#if EPG_6937_SUPPORT
    epgCreate.txt_format = EPG_TXT_PRIVATE;
    epgCreate.ttx_private = (gx_epg_ttx_private)epg_lang_recode_event_info;
#else
    epgCreate.txt_format = EPG_TXT_UTF8;
#endif
    GxBus_ConfigGetInt(EPG_LANG_KEY, &init_value, EPG_LANG);
    lang = app_get_epg_short_lang_str(init_value);
    memcpy(epgCreate.language, lang, sizeof(epgCreate.language));
#if EPG_REALTIMER_GET_DETAIL_SUPPORT
    app_epg_detail_filter_set(0xffff,0xffff,0xffff,0xffff);
#endif
    app_send_msg_exec(GXMSG_EPG_CREATE, (void *)(&epgCreate));

    //register after epg create
    GxBus_RegisterCompFunc(app_epg_event_cmp); //epg event parse
    s_Switch.ts_src = ts_src;
    s_Switch.dmx_id = dmx_id;

    APP_TIMER_ADD(thiz_epg_section_timer, epg_section_flag_clear, ONE_HOUR, TIMER_REPEAT);

    return;
}

void app_epg_disable(void)
{
    if(s_Switch.flag == EPG_OFF)
        return;
    s_Switch.flag = EPG_OFF;

    APP_TIMER_REMOVE(thiz_epg_section_timer);
    //unregist before release
    GxBus_UnregisterCompFunc();
    app_send_msg_exec(GXMSG_EPG_RELEASE, NULL);

    s_Switch.ts_src = INVALID_TS;
    s_Switch.dmx_id = INVALID_DMX;

    return;
}

int app_epg_filter_resume(void)
{
    GxMsgProperty_EpgCreate epgCfg;

    if(false == thiz_epg_filter_pause_flag || EPG_OFF == s_Switch.flag)
        return 0;

    app_send_msg_exec(GXMSG_EPG_CFG_GET, &epgCfg);
    app_send_msg_exec(GXMSG_EPG_FILTER_START, &epgCfg);
    thiz_epg_filter_pause_flag = false;

    return 0;
}

int app_epg_filter_pause(void)
{
    if(true == thiz_epg_filter_pause_flag || EPG_OFF == s_Switch.flag)
        return 0;

    app_send_msg_exec(GXMSG_EPG_FILTER_STOP, NULL);
    thiz_epg_filter_pause_flag = true;
    return 0;
}

int app_epg_ts_id_filter_modify(uint32_t ts_id)
{
    GxMsgProperty_EpgCreate epgCfg;

    if(s_Switch.flag == EPG_OFF)
        return -1;

    app_send_msg_exec(GXMSG_EPG_CFG_GET, &epgCfg);
    epgCfg.ts_id = ts_id;
    app_send_msg_exec(GXMSG_EPG_FILTER_STOP, NULL);
    app_send_msg_exec(GXMSG_EPG_FILTER_START, &epgCfg);

    return 0;
}

int app_epg_setfliter_byserviceid(int serviceid,int curnext_only)
{
    //to do
    return 0;
}

void app_epg_filter_reset(void)
{
    if(EPG_ON == s_Switch.flag)
    {
        app_send_msg_exec(GXMSG_EPG_RESET, NULL);
        thiz_epg_filter_pause_flag = false;
    }

    return;
}

void app_epg_info_clean(void)
{
    if(EPG_ON == s_Switch.flag)
        app_send_msg_exec(GXMSG_EPG_CLEAN,(void *)NULL);
}

void app_epg_info_full_speed(void)
{
    if(EPG_ON == s_Switch.flag)
        app_send_msg_exec(GXMSG_EPG_FULL_SPEED,(void *)NULL);
}

#if EPG_REALTIMER_GET_DETAIL_SUPPORT
void app_epg_detail_filter_clean(void)
{
    app_send_msg_exec(GXMSG_EPG_DETAIL_FILTER_CLEAN,(void *)NULL);
}

void app_epg_detail_filter_set(uint16_t ts_id,uint16_t service_id, uint16_t original_id, uint16_t event_id)
{
    GxMsgProperty_EpgDetailFilterSwitch filter_switch = {0};
    filter_switch.filter_level = 1;
    filter_switch.ts_id = ts_id;
    filter_switch.service_id = service_id;
    filter_switch.orig_network_id = original_id;
    filter_switch.event_id = event_id;
    app_send_msg_exec(GXMSG_EPG_DETAIL_FILTER_SET,&filter_switch);
}
#endif

static status_t epg_prog_info_update(AppEpgOps *ops, EpgProgInfo *prog_info)
{
    if((ops == NULL) || (prog_info == NULL))
        return GXCORE_ERROR;

    memcpy(&(ops->prog_info), prog_info, sizeof(EpgProgInfo));

    return GXCORE_SUCCESS;
}

static int32_t epg_get_cur_event_count(AppEpgOps *ops)
{
    GxMsgProperty_EpgPFNumGet event = {0};

    if(NULL == ops || EPG_OFF == s_Switch.flag)
        return 0;

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_PF_NUM_GET, &event))
        return event.real_event_count;

    return 0;
}

static status_t epg_get_cur_event_info(AppEpgOps *ops, GxEpgInfo *event_info)
{
    GxMsgProperty_EpgPFGet event = {0};

    if(NULL == ops || NULL == event_info || EPG_OFF == s_Switch.flag)
        return GXCORE_ERROR;

    memset(event_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;
    event.type = EPG_PRESENT_EVENT;
    event.epg_info_size = EVENT_GET_NUM*EVENT_GET_MAX_SIZE;
    event.epg_info = event_info;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_PF_GET, (void *)(&event))
            && EVENT_GET_NUM == event.get_event_count)
    {
        if(0 == event_info->start_time || 0 == event_info->duration)
            return GXCORE_ERROR;
        else
            return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static status_t epg_get_next_event_info(AppEpgOps *ops, GxEpgInfo *event_info)
{
    GxMsgProperty_EpgPFGet event = {0};

    if(NULL == ops || NULL == event_info || EPG_OFF == s_Switch.flag)
        return GXCORE_ERROR;

    memset(event_info, 0, EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;
    event.type = EPG_FOLLOW_EVENT;
    event.epg_info_size = EVENT_GET_NUM*EVENT_GET_MAX_SIZE;
    event.epg_info = event_info;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_PF_GET, (void *)(&event))
            && EVENT_GET_NUM == event.get_event_count)
    {
        if(0 == event_info->start_time || 0 == event_info->duration)
            return GXCORE_ERROR;
        else
            return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static int32_t epg_get_schedule_event_count(AppEpgOps *ops, time_t start_time, time_t end_time)
{
    GxMsgProperty_EpgNumGetByPeriod event = {0};

    if(NULL == ops || EPG_OFF == s_Switch.flag)
        return 0;

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;
    event.start_time = start_time;
    event.end_time = end_time;

    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_EPG_NUM_GET_BY_PERIOD, &event))
        return event.real_event_count;

    return 0;
}

static status_t epg_get_schedule_event_info(AppEpgOps *ops, GxEpgInfo *event_info, EpgPeriodPara para)
{
    GxMsgProperty_EpgGetByPeriod event = {0};

    if(NULL == ops || NULL == event_info || EPG_OFF == s_Switch.flag)
        return GXCORE_ERROR;

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;
    event.level = para.level;
    event.start_time = para.start_time;
    event.end_time = para.end_time;
    event.want_event_count = para.want_get_cnt;
    event.epg_info = event_info;
    if(EPG_COMPLETE_INFO == para.level)
        event.epg_info_size = para.want_get_cnt*EVENT_GET_MAX_SIZE;
    else if(EPG_WITH_NAME_INFO == para.level)
        event.epg_info_size = para.want_get_cnt*(sizeof(GxEpgInfo) + EVENT_NAME_MAX_SIZE);
    else
        event.epg_info_size = para.want_get_cnt*sizeof(GxEpgInfo);

    memset(event_info, 0, event.epg_info_size);

    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_EPG_GET_BY_PERIOD, &event) || para.want_get_cnt != event.get_event_count)
        return GXCORE_ERROR;

    return GXCORE_SUCCESS;
}

static status_t epg_get_event_info_by_event_id(AppEpgOps *ops, GxEpgInfo *event_info, uint16_t event_id)
{
    GxMsgProperty_EpgGetByEventId event = {0};

    if(NULL == ops || NULL == event_info || EPG_OFF == s_Switch.flag)
        return GXCORE_ERROR;

    event.orig_network_id = ops->prog_info.orig_network_id;
    event.service_id = ops->prog_info.service_id;
    event.ts_id = ops->prog_info.ts_id;
    event.event_id = event_id;
    event.level = EPG_COMPLETE_INFO;
    event.epg_info = event_info;
    event.epg_info_size = EVENT_GET_MAX_SIZE;

    memset(event_info, 0, event.epg_info_size);

    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_EPG_GET_BY_EVENT_ID, &event) || event.get_event_count != 1)
        return GXCORE_ERROR;

    return GXCORE_SUCCESS;
}

void epg_ops_init(AppEpgOps * ops)
{
    ops->prog_info_update = epg_prog_info_update;
    ops->get_cur_event_cnt = epg_get_cur_event_count;
    ops->get_cur_event_info = epg_get_cur_event_info;
    ops->get_next_event_info = epg_get_next_event_info;
    ops->get_schedule_event_cnt = epg_get_schedule_event_count;
    ops->get_schedule_event_info = epg_get_schedule_event_info;
    ops->get_event_info_by_event_id = epg_get_event_info_by_event_id;

    memset(&ops->prog_info, 0, sizeof(ops->prog_info));
}

void app_epg_prog_info_set(EpgProgInfo *epg_prog_info, GxMsgProperty_NodeByPosGet *node_prog)
{
	epg_prog_info->ts_id= node_prog->prog_data.ts_id;
	epg_prog_info->orig_network_id= node_prog->prog_data.original_id;
	epg_prog_info->service_id= node_prog->prog_data.service_id;
	epg_prog_info->prog_id= node_prog->prog_data.id;
	epg_prog_info->prog_pos = node_prog->pos;
	// add the tp id ,use to identify the different TS's EPG
	epg_prog_info->tp_id = node_prog->prog_data.tp_id;
}

