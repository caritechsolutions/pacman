#include "app.h"
#include "app_wnd_search.h"
#include "app_module.h"
#include "app_send_msg.h"
#include "app_utility.h"
#if !OTT_SUPPORT
#include "module/app_frontend.h"
#include "app_epg.h"
#include "app_default_params.h"
#include "full_screen.h"
#include "app_volume_value.h"
#include "module/channel_edit.h"
#if TKGS_SUPPORT
#include "tkgs/app_tkgs_background_upgrade.h"
#endif
#if FACTORY_TEST_SUPPORT
#include "app_factory_test_date.h"
#endif
#include "app_windows.h"
#if PDMX_SUPPORT
#include "app_pdmx.h"
#endif
#if MULTISTREAM_SUPPORT
#include "module/app_multistream.h"
#endif
#if SAT2IP_SERVER_SUPPORT
#include "sat2ip_server/sat2ip_server_platform.h"
#endif
#if DVB2IP_SERVER_SUPPORT
#include "dvb2ip_server/app_dvb2ip_platform.h"
#endif
#if LCN_SUPPORT
#include "app_lcn.h"
#endif

#if WEBAPI_SUPPORT
#include <cJSON.h>
#include <gx_apps.h>
#include "app_webapi.h"
#include "netapps/webapi/app_webapi_priv.h"
#endif

#if SEARCH_AUTO_SAVE_SUPPORT
static event_list  *s_SearchTipsTimer = NULL;
#endif

//satellite search opt content
#define SAT_SEARCH_MODE_CONTENT	"[Preset,Blind]"
#define SAT_SEARCH_SUPER_MODE_CONTENT "[Super Satellite,Super Blind]"
#define SAT_SEARCH_BLIND_CONTENT	"[Blind]"
#define SAT_SEARCH_SUPER_BLIND_CONTENT	"[Super Blind]"


#define YES_NO_CONTENT	"[No,Yes]"
#define SCAN_CHANNEL_CONTENT	"[All,TV,Radio]"
//tp search opt content
//#define TP_SEARCH_MODE_CONTENT	"[Normal]"//"[Normal, PID]"

// MAX time is 65535

#define DRA_STREAM_TYPE  (0x06)
#define DRA_TAG_TYPE     (0x05)

typedef enum
{
	SAERCH_ERROR = 0,
	SEARCH_OVERFLOW,
	SEARCH_OK,
}SearchStopType_e;

typedef struct
{
	SearchStopType_e m_StopType;
	uint32_t m_nHaveNewProg;
}SearchStopType_t;

#if (DEMOD_DVB_S > 0)
extern bool scan_mode_disable_flag;
#endif

/* Private variable------------------------------------------------------- */
static event_list* sp_SearchTimerSignal = NULL;
static SearchStopType_t sg_StopType = {0};
static uint32_t sg_TvCount=0;
static uint32_t sg_RadioCount=0;
static int thizDraAudioId = -1;
static handle_t thiz_search_mutex = GXCORE_INVALID_POINTER;

static SearchType sg_search_type = SEARCH_NONE;
static SearchMode sg_search_mode = MANUAL_SEARCH;
static SearchIdSet *sg_search_id_set = NULL;
static SearchIdSet src_sg_search_id_set ;
static GxMsgProperty_ScanSatStart s_Search;
static GxMsgProperty_BlindScanStart s_Blind;
static uint32_t *sp_scan_id = NULL;
static uint32_t *sp_ts_src = NULL;
static uint32_t s_sat_search_sat_total = 0;
#if WEBAPI_SUPPORT
static uint32_t s_search_auto_save = 0;
#endif
static enum
{
	SAT_SEARCH_AUTO,
	SAT_SEARCH_BLINDE
}sg_sat_search_mode = SAT_SEARCH_AUTO;

// add in 20121012
static uint32_t sg_SatId = 0xffff;// for search moto control

#if WEBAPI_SUPPORT
uint32_t s_search_state = 0;
uint32_t s_search_progress = 0;
uint32_t s_search_prog_total = 0;
static gx_list_t *s_search_prog_list = NULL;
extern void gxapps_mutex_lock(void);
extern void gxapps_mutex_unlock(void);
#endif

//add in 20130410
static GxBlindSearchStage s_blind_stage =  BLIND_STAGE_NONE;
/* widget macro -------------------------------------------------------- */

#if TKGS_SUPPORT
extern status_t app_tkg_lcn_flush(void);
extern void app_tkgs_sort_channels_by_lcn(void);
extern int app_play_update_prog_tp_id(uint16_t tp_id);
#endif
void app_search_clr_msg_process_si_subtable(void);

#if LCN_SUPPORT
static bool s_SearchNitMode = false;
static GxSearchExtend s_SearchExtParse[SEARCH_EXTEND_MAX];
search_extend g_SearchExtendList;
search_add_extend_table app_search_add_extend_callback = NULL;

int app_search_add_extend_table_call_back(search_extend* searchExtendList )
{
	if (NULL == searchExtendList)
		return 0;

	memset(searchExtendList,0,sizeof(search_extend));

	searchExtendList->searchExtend[searchExtendList->extendnum].pid = NIT_PID;
	searchExtendList->searchExtend[searchExtendList->extendnum].time_out = TIMEOUT_NIT;
	searchExtendList->searchExtend[searchExtendList->extendnum].match_depth = 1;
	searchExtendList->searchExtend[searchExtendList->extendnum].match[0] = NIT_ACTUAL_NETWORK_TID;
	searchExtendList->searchExtend[searchExtendList->extendnum].mask[0] = 0xff;
	searchExtendList->searchExtend[searchExtendList->extendnum].table_parse_fun = app_table_nit_extend_descriptor_parse;
	searchExtendList->extendnum ++;
	return 0;
}

void app_search_register_add_extend_table_callback(search_add_extend_table search_extend_call_back)
{
	if (NULL != search_extend_call_back)
	{
		app_search_add_extend_callback = search_extend_call_back;
	}
	return ;
}
#endif


static int timer_search_show_signal(void *userdata)
{
	unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};
    static int IconCount = 0;
    uint32_t tuner = app_tuner_cur_tuner_get();

    app_ioctl(tuner, FRONTEND_STRENGTH_GET, &value);
    siganl_bar.strength_bar = "progbar_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_strength_value", "string", Buffer);

    // progbar quality
    //app_log_info("[APP Search] get quality\n");
    app_ioctl(tuner, FRONTEND_QUALITY_GET, &value);
    siganl_bar.quality_bar= "progbar_search_quality";
    siganl_val.quality_val = value;

    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_quality_value", "string", Buffer);

    app_signal_progbar_update(tuner, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY,&value);

    IconCount++;
    if( APP_THEME_CLASSIC_BLUE ){
    if(IconCount == 1)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat1");
    }
    else if(IconCount == 2)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat2");
    }
    else if(IconCount == 3)
    {
        GUI_SetProperty("img_search_magnifier","img","s_icon_sat3");
        IconCount = 0;
    }
    }
    return 0;
}

static void app_seach_clear_signal_show(void)
{
	unsigned int value = 0;
    char Buffer[20] = {0};
    SignalBarWidget siganl_bar = {0};
    SignalValue siganl_val = {0};

    siganl_bar.strength_bar = "progbar_search_strength";
    siganl_val.strength_val = value;

    // strength value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_strength_value", "string", Buffer);

    // progbar quality
    siganl_bar.quality_bar= "progbar_search_quality";
    siganl_val.quality_val = value;

    // quality value
    memset(Buffer,0,sizeof(Buffer));
    sprintf(Buffer,"%d%%",value);
    GUI_SetProperty("text_search_quality_value", "string", Buffer);

    app_signal_progbar_update(0, &siganl_bar, &siganl_val);
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY,&value);
}

static void _set_auto_search_display(void)
{
	GUI_SetProperty("text_search_tp_title", "state", "hide");
	GUI_SetProperty("img_search_tp_back", "state", "hide");
	GUI_SetProperty("text_search_tp_name1", "state", "hide");
	GUI_SetProperty("text_search_tp_name2", "state", "hide");
	GUI_SetProperty("text_search_tp_name3", "state", "hide");
	GUI_SetProperty("text_search_tp_name4", "state", "hide");
	GUI_SetProperty("text_search_tp_name5", "state", "hide");
	GUI_SetProperty("text_search_tp_name6", "state", "hide");

	GUI_SetProperty("text_search_tv_title", "state", "show");
	GUI_SetProperty("img_search_tv_back", "state", "show");
	GUI_SetProperty("text_search_tv_name1", "state", "show");
	GUI_SetProperty("text_search_tv_name2", "state", "show");
	GUI_SetProperty("text_search_tv_name3", "state", "show");
	GUI_SetProperty("text_search_tv_name4", "state", "show");
	GUI_SetProperty("text_search_tv_name5", "state", "show");
	GUI_SetProperty("text_search_tv_name6", "state", "show");

	GUI_SetProperty("text_search_radio_title", "state", "show");
	GUI_SetProperty("img_search_radio_back", "state", "show");
	GUI_SetProperty("text_search_radio_name1", "state", "show");
	GUI_SetProperty("text_search_radio_name2", "state", "show");
	GUI_SetProperty("text_search_radio_name3", "state", "show");
	GUI_SetProperty("text_search_radio_name4", "state", "show");
	GUI_SetProperty("text_search_radio_name5", "state", "show");
	GUI_SetProperty("text_search_radio_name6", "state", "show");
}

static void _set_blind_search_display(void)
{
    _set_auto_search_display();
}

#if SEARCH_SHOW_LCN_SUPPORT
static void _search_show_lcn_tv_channel(uint16_t prog_lcn, uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define TV_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
    static char s_TvName[6][TV_NAME_BUFFER_LENGTH];
    char* s_TextName[6] = {"text_search_tv_name1", "text_search_tv_name2",
        "text_search_tv_name3", "text_search_tv_name4",
        "text_search_tv_name5", "text_search_tv_name6"};

    char* s_ImgName[6] = {"text_search_scrambler1", "text_search_scrambler2",
        "text_search_scrambler3", "text_search_scrambler4",
        "text_search_scrambler5", "text_search_scrambler6"};

    static char s_ImgState[6][20];
    uint8_t CurLine=0;
    uint8_t i=0;
	uint8_t buf[10]= {0};

    CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;

    if(pNewProgGet->scramble_flag)
        sprintf(s_ImgState[CurLine],"%s","$");
    else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[TV_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, TV_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%d    %s",prog_lcn,pNewProgGet->name);
    app_log_debug("------%d    %s------\n",prog_lcn,pNewProgGet->name);
    strcpy(s_TvName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_TvName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

	memset(buf,0,10);
	sprintf((void*)buf,"%d",channel_num+1);
	GUI_SetProperty("text_search_tv_num", "string", buf);
    return;
}

static void _search_show_lcn_radio_channel(uint16_t prog_lcn, uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define RADIO_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
	static char s_RadioName[6][RADIO_NAME_BUFFER_LENGTH];
	char* s_TextName[6] = {"text_search_radio_name1", "text_search_radio_name2",
					"text_search_radio_name3", "text_search_radio_name4",
					"text_search_radio_name5", "text_search_radio_name6"};

	char* s_ImgName[6] = {"text_search_scrambler7", "text_search_scrambler8",
					"text_search_scrambler9", "text_search_scrambler10",
					"text_search_scrambler11", "text_search_scrambler12"};
	static char s_ImgState[6][20];

	uint8_t CurLine=0;
	uint8_t i=0;
	uint8_t buf[10]= {0};

	CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;

    if(pNewProgGet->scramble_flag)
		sprintf(s_ImgState[CurLine],"%s","$");
	else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[RADIO_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, RADIO_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%d    %s",prog_lcn,pNewProgGet->name);
    strcpy(s_RadioName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_RadioName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

	memset(buf,0,10);
	sprintf((void*)buf,"%d",channel_num+1);
	GUI_SetProperty("text_search_tv_num", "string", buf);
    return;
}

#else

static void _search_show_tv_channel(uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define TV_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
    static char s_TvName[6][TV_NAME_BUFFER_LENGTH];
    char* s_TextName[6] = {"text_search_tv_name1", "text_search_tv_name2",
        "text_search_tv_name3", "text_search_tv_name4",
        "text_search_tv_name5", "text_search_tv_name6"};

    char* s_ImgName[6] = {"text_search_scrambler1", "text_search_scrambler2",
        "text_search_scrambler3", "text_search_scrambler4",
        "text_search_scrambler5", "text_search_scrambler6"};

    static char s_ImgState[6][20];
    uint8_t CurLine=0;
    uint8_t i=0;

    CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;

    if(pNewProgGet->scramble_flag)
        sprintf(s_ImgState[CurLine],"%s","$");
    else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[TV_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, TV_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%d    %s",channel_num+1,pNewProgGet->name);
    app_log_debug("------%d    %s------\n",channel_num+1,pNewProgGet->name);
    strcpy(s_TvName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_TvName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_TvName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

    return;
}

static void _search_show_radio_channel(uint32_t channel_num, GxMsgProperty_NewProgGet* pNewProgGet)
{
#define CHANNEL_NUM_ONE_PAGE	6
#define RADIO_NAME_BUFFER_LENGTH	(MAX_PROG_NAME+10)
	static char s_RadioName[6][RADIO_NAME_BUFFER_LENGTH];
	char* s_TextName[6] = {"text_search_radio_name1", "text_search_radio_name2",
					"text_search_radio_name3", "text_search_radio_name4",
					"text_search_radio_name5", "text_search_radio_name6"};

	char* s_ImgName[6] = {"text_search_scrambler7", "text_search_scrambler8",
					"text_search_scrambler9", "text_search_scrambler10",
					"text_search_scrambler11", "text_search_scrambler12"};
	static char s_ImgState[6][20];

	uint8_t CurLine=0;

	uint8_t i=0;

	CurLine = channel_num%CHANNEL_NUM_ONE_PAGE;


    if(pNewProgGet->scramble_flag)
		sprintf(s_ImgState[CurLine],"%s","$");
	else
        sprintf(s_ImgState[CurLine],"%s"," ");

    char Buffer[RADIO_NAME_BUFFER_LENGTH] = {0};
    memset(Buffer, 0, RADIO_NAME_BUFFER_LENGTH);
    sprintf((char*)Buffer,"%d    %s",channel_num+1,pNewProgGet->name);
    strcpy(s_RadioName[CurLine], Buffer);

    //to roll
    if(channel_num<CHANNEL_NUM_ONE_PAGE)
    {
        GUI_SetProperty(s_TextName[CurLine], "string", s_RadioName[CurLine]);
        GUI_SetProperty(s_ImgName[CurLine], "string", s_ImgState[CurLine]);
    }
    else
    {
        for(i=0; i<CHANNEL_NUM_ONE_PAGE; i++)
        {
            if((CurLine+i)<5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i+1]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i+1]);
            }
            else if((CurLine+i)>=5)
            {
                GUI_SetProperty(s_TextName[i], "string", s_RadioName[CurLine+i-5]);
                GUI_SetProperty(s_ImgName[i], "string", s_ImgState[CurLine+i-5]);
            }
        }
    }

    return;
}
#endif

static void _search_show_tp(uint32_t tp_num, GxMsgProperty_BlindScanReply* pBSReply)
{
}
#if DEMOD_DVB_C
#if(DVBC_TUNE_AUTO_SUPPORT > 0)
int gSearchDvbc_TpID = -1;
static status_t _search_modify_dvbc_tp(int tpid)
{
    int32_t frontend_handle = 0;
	struct dvb_frontend_parameters params = { 0 };
	struct dvb_frontend_info frontend_info = {{0}};
    GxMsgProperty_NodeByIdGet node_tp = {0};
    GxBusPmDataSat sat_arry = {0};
    GxMsgProperty_NodeModify  modify_node = {0};

    unsigned char      ui8QAMIndex = DVBC_64QAM;
    unsigned int       ui32SymRate = 6875;

    if(tpid ==-1)
    {
        return GXCORE_ERROR;
    }
	app_get_sat_params(&sat_arry, DEMOD_TYPE_DVBC);

    frontend_handle = GxFrontend_IdToHandle(sat_arry.tuner);
    if(frontend_handle == -1)
    {
        return GXCORE_ERROR;
    }
    if(ioctl(frontend_handle,FE_GET_INFO, &frontend_info) < 0)
    {
        return GXCORE_ERROR;
    }
    if(frontend_info.type == FE_DVB_C)
    {
        if(ioctl(frontend_handle,FE_GET_FRONTEND,&params)>=0)
        {
            ui8QAMIndex = params.u.dvbc_param.modulation;
            ui32SymRate = params.u.dvbc_param.symbol_rate;
        }
        else
        {
            return GXCORE_ERROR;
        }
    }
    else
    {
        return GXCORE_ERROR;
    }

    node_tp.node_type = NODE_TP;
    node_tp.id = tpid;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

    if((node_tp.tp_data.tp_c.modulation != ui8QAMIndex)
        ||node_tp.tp_data.tp_c.symbol_rate != (ui32SymRate))
    {
        modify_node.node_type = NODE_TP;
        modify_node.tp_data.id= node_tp.id;
        modify_node.tp_data.sat_id= node_tp.tp_data.sat_id;
        modify_node.tp_data.frequency= node_tp.tp_data.frequency;
        modify_node.tp_data.tp_c.modulation = ui8QAMIndex;
        modify_node.tp_data.tp_c.symbol_rate = ui32SymRate;
        app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &modify_node);

        APP_Printf("_modify_dvbc_tp freq = %d,modulation =%d symbol rate =%d\n",modify_node.tp_data.frequency,modify_node.tp_data.tp_c.modulation,modify_node.tp_data.tp_c.symbol_rate);
    }

    return GXCORE_SUCCESS;
}
#endif
#endif

static int app_get_nit_search_sat_num(uint32_t sat_id)
{
    uint8_t sat_num = 1;
    int i = 0;
    for(i = 0; i < s_sat_search_sat_total; i++)
    {
        if(sat_id == sp_scan_id[i])
        {
            sat_num = i + 1;
        }
    }

    return sat_num;
}

static int app_clear_invalid_tp_buffer(char *p, uint8_t buffer_length)
{
    int i = 0;
    while ((')' != *p)&&(i < buffer_length-2))
    {
        i++;
        p++;
    }
    memset(p+1, ' ', buffer_length-i-1);

    return 0;
}

#if SEARCH_AUTO_SAVE_SUPPORT
#if DEMOD_DVB_C && DEMOD_DVB_T && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
extern void app_t2_sort_program(int32_t sort_flag);

static void _search_exit(void)
{
    int32_t ret = 0;
    int32_t index = 0;
    int32_t sat_num = SYS_MAX_SAT;
    GxBusPmDataSat	sat = {0};

    int32_t c_t2_mode;

    GxBus_ConfigGetInt(SEARCH_MODE_TYPE_KEY, &c_t2_mode, SEARCH_MODE_TYPE_VALUE);
    for (index = 0; index < sat_num; index++)
    {
        memset(&sat, 0, sizeof(GxBusPmDataSat));
        // read one sat from database once
        ret = GxBus_PmSatsGetByPos(index, 1, &sat);
        if (1 == ret)
        {
            if ((GXBUS_PM_SAT_C == sat.type) && (c_t2_mode == 0))
            {
                g_AppPlayOps.normal_play.view_info.sat_id = sat.id;
                break;
            }
            else if ((GXBUS_PM_SAT_DVBT2 == sat.type) && (c_t2_mode == 1))
            {
                g_AppPlayOps.normal_play.view_info.sat_id = sat.id;
                break;
            }
        }
    }

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_ALL) {
        if(g_AppFullArb.state.tv == STATE_ON)
            g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_TV;
        else
            g_AppPlayOps.normal_play.view_info.stream_type = GXBUS_PM_PROG_RADIO;
    }
    g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_SAT;
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
#if LCN_SUPPORT
	int32_t sort_flag;
	GxBus_ConfigGetInt(T2_SORT_FLAG, &sort_flag, T2_SORT_FLAG_VALUE);
	app_t2_sort_program(sort_flag);
#endif

    if (g_AppPlayOps.normal_play.play_total == 0)
    {
		if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
		{
			if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
				g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
            return;
		}

        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SEARCH_SETTING))
            return;
    }


#if RECALL_LIST_SUPPORT
    g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];
#else
    g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
#endif

    if(GXCORE_ERROR == GUI_CheckDialog(WND_COMMON_SEARCH_SETTING))
    {
#if BOUQUET_SUPPORT
		if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
        {
            app_bouquet_init();
            g_AppBat.start(&g_AppBat);
        }
#endif
        GUI_EndDialog("after wnd_full_screen");
        app_log_info("play_count:%d\n", g_AppPlayOps.normal_play.play_count);

        app_play_current_prog(PLAY_TYPE_NORMAL | PLAY_MODE_POINT);

        if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
            g_AppFullArb.tip = FULL_STATE_LOCKED;
        else
            g_AppFullArb.tip = FULL_STATE_DUMMY;

        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    }

    return ;
}

#elif DEMOD_DVB_C || DEMOD_DTMB
#if LCN_SUPPORT
static void app_sort_program_by_lcn(void)
{
    uint32_t focus = 0;
    uint32_t type;
    GxBusPmViewInfo view_info;
    GxBusPmViewInfo view_info_bak;
    GxMsgProperty_NodeNumGet node_num_get = {0};

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
    view_info_bak = view_info;	//bak

    for (type = GXBUS_PM_PROG_TV; type < GXBUS_PM_PROG_RADIO + 1; type++)
    {
        view_info.stream_type = type;

        GxBus_PmViewInfoMemSet(&view_info);
        node_num_get.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
        if(node_num_get.node_num == 0)
            continue;

        g_AppChOps.create(node_num_get.node_num);
        g_AppChOps.sort(SORT_BY_LCN, &focus);
        g_AppChOps.save();
    }

    GxBus_PmViewInfoMemClear();
    if (GXCORE_ERROR == g_AppPlayOps.play_list_create(&view_info_bak))
    {
        return ;
    }
}
#endif

static void _search_dvbc_dtmb_exit(void)
{
#if LCN_SUPPORT
    app_sort_program_by_lcn();
#endif
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
    if (g_AppPlayOps.normal_play.play_total == 0)
    {
        if(GUI_CheckDialog(WND_MAIN_MENU) != GXCORE_SUCCESS)
        {
            if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
                g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
            return;
        }

        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SEARCH_SETTING))
            return;
    }

#if RECALL_LIST_SUPPORT
    g_AppPlayOps.current_play = g_AppPlayOps.recall_play[0];
#else
    g_AppPlayOps.current_play = g_AppPlayOps.recall_play;
#endif

    if(GXCORE_ERROR == GUI_CheckDialog(WND_COMMON_SEARCH_SETTING))
    {
        GUI_EndDialog("after wnd_full_screen");
        app_log_info("play_count:%d\n", g_AppPlayOps.normal_play.play_count);

        app_play_current_prog(PLAY_TYPE_NORMAL | PLAY_MODE_POINT);

        if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
            g_AppFullArb.tip = FULL_STATE_LOCKED;
        else
            g_AppFullArb.tip = FULL_STATE_DUMMY;

        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    }
    return ;
}
#endif

static int _searchtips_timeout_cb(void *usrdata)
{
    APP_TIMER_REMOVE(s_SearchTipsTimer);

#if SAT2IP_SERVER_SUPPORT
        app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
        app_dvb2ip_prog_num_update();
#endif
    GUI_EndDialog(WND_SEARCH_TIP);
    GUI_EndDialog(WND_SEARCH);

    if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
        GUI_SetProperty(WND_MAIN_MENU,"update",NULL);

#if DEMOD_DVB_C && DEMOD_DVB_T && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
    _search_exit();
#elif DEMOD_DVB_C || DEMOD_DTMB
    _search_dvbc_dtmb_exit();
#endif
    return 0;
}
#endif

#if WEBAPI_SUPPORT
static void app_webapi_search_auto_save(void)
{
    int abort = 0;

    WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
    if(s_search_auto_save == 2)
    {
        abort = 1;
    }
    s_search_auto_save = 0;
    if((abort == 0) &&(SEARCH_OK == sg_StopType.m_StopType)&&(sg_StopType.m_nHaveNewProg))
    {
        WEBAPI_PRINTF("%s[%d]save\n",__FUNCTION__, __LINE__);
        GxBus_PmLock();
        app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
        GxBus_PmUnlock();
        GxCore_ThreadDelay(800);
#if SAT2IP_SERVER_SUPPORT
        app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
        app_dvb2ip_prog_num_update();
#endif
    }
    else
    {
        WEBAPI_PRINTF("%s[%d]not save\n",__FUNCTION__, __LINE__);
        app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
    }
    GUI_EndDialog(WND_SEARCH);
    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_MAIN_MENU))
    {
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);

        if(g_AppPlayOps.normal_play.play_total != 0)
        {
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
            if(g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
                g_AppFullArb.tip = FULL_STATE_LOCKED;
            else
                g_AppFullArb.tip = FULL_STATE_DUMMY;
        }
    }

}

#define WEBAPI_SAVE_PROG_NUM (10)
static void app_webapi_search_prog_save(GxMsgProperty_NewProgGet* pNewProgGet)
{
	char *prog_type = NULL;
	char buffer[64];

	gxapps_mutex_lock();
	if(!s_search_prog_list)
	{
		s_search_prog_list = gx_list_new();
	}
	if(s_search_prog_list)
	{
		if(s_search_prog_list->item_num>=WEBAPI_SAVE_PROG_NUM)
		{
			gx_list_delete_item(s_search_prog_list, 0);
		}
		if(GXBUS_PM_PROG_TV == pNewProgGet->type)
		{
			prog_type = "tv";
		}
		else
		{
			prog_type = "radio";
		}
		snprintf(buffer, sizeof(buffer)-1, "%d", s_search_prog_total+1);
		gx_list_add_item(s_search_prog_list, buffer, (char *)pNewProgGet->name, prog_type);
		s_search_prog_total++;
	}
	gxapps_mutex_unlock();
}

#endif

#if FONT_GB2312_TO_UTF8
static void app_str_gb2312_convert_utf8(unsigned char* name, unsigned int len )
{
	int relen = 0;
	unsigned int convert_len = 0;
	unsigned char* iconv_str = NULL;

	if(name == NULL || len == 0 || name[0] == '\0')
		return;

	if (name[0] <=0x20)
	{
		unsigned char* buf = GxCore_Mallocz(len);
		if (NULL != buf)
		{
			memcpy(buf,name,len);
			memset(name, 0x00, len);
			memcpy(name,buf+1,len-1);
			GxCore_Free(buf);
		}
	}

	gdi_encoding_convert("iso6937","gb2312");

	if(gxgdi_iconv(name,&iconv_str,len,(unsigned int *)&relen,NULL) == 1)
	{
		iconv_str = NULL;
		return;
	}

	memset(name, 0x00, len);
	convert_len = strlen((const char *)iconv_str);
	app_log_debug("%s %s len=%d convert_len=%d\n",__FILE__,__func__,len,convert_len);
	if (convert_len >= len)
	{
		memcpy(name, iconv_str, len);
		name[len-1]='\0';
	}
	else
	{
		memcpy(name, iconv_str, convert_len);
	}
	GxCore_Free(iconv_str);
	gdi_encoding_convert("iso6937",NULL);
	return ;
}

#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
static int app_search_modify_prog_cb(GxBusPmDataProg* prog )
{
	int relen;
	unsigned int conver_len = 0;
	unsigned int len = 0;
	unsigned char* iconv_str = NULL;

	if ((NULL == prog)||('\0' == prog->prog_name[0]))
	{
		return -1;
	}
	len = sizeof(prog->prog_name);
	if(prog->prog_name[0] > 0x20)
	{
		gdi_encoding_convert("iso6937","gb2312");
		if(gxgdi_iconv(prog->prog_name,&iconv_str,strlen((const char *)prog->prog_name),(unsigned int *)&relen,NULL) == 1)
		{
			iconv_str = NULL;
			return -1;
		}

		memset(prog->prog_name, 0x00, sizeof(prog->prog_name));
		conver_len = strlen((const char *)iconv_str);
		if (conver_len >= len)
		{
			memcpy(prog->prog_name, iconv_str, len);
			prog->prog_name[len-1]='\0';
		}
		else
		{
			memcpy(prog->prog_name, iconv_str, conver_len);
		}
		GxCore_Free(iconv_str);
		gdi_encoding_convert("iso6937",NULL);
	}
	else
	{
		if(gxgdi_iconv(prog->prog_name,&iconv_str,strlen((const char *)prog->prog_name),(unsigned int *)&relen,NULL) == 1)
		{
			iconv_str = NULL;
			return -1;
		}

		memset(prog->prog_name, 0x00, sizeof(prog->prog_name));
		conver_len = strlen((const char *)iconv_str);
		if (conver_len >= len)
		{
			memcpy(prog->prog_name, iconv_str, len);
			prog->prog_name[len-1]='\0';
		}
		else
		{
			memcpy(prog->prog_name, iconv_str, conver_len);
		}
		GxCore_Free(iconv_str);
	}
    if(prog->service_type == GXBUS_PM_PROG_RESERVED_TV || prog->service_type == GXBUS_PM_PROG_RESERVED_RADIO)
    {
#if RESERVED_SERVICE_NOSAVE_SUPPORT
        return GXCORE_ERROR;
#endif
        if(prog->service_type == GXBUS_PM_PROG_RESERVED_TV)
            prog->service_type = GXBUS_PM_PROG_TV;
        else
            prog->service_type = GXBUS_PM_PROG_RADIO;
    }
#if LCN_SUPPORT
    LCNInfoOne_t data;

    memset(&data, 0, sizeof(LCNInfoOne_t));
    data.original_id = prog->original_id;
    data.ts_id = prog->ts_id;
    data.service_id = prog->service_id;
    data.network_id = 0;
    data.lcn = INVALID_LCN;
    data.frequency = app_lcn_searching_frequency_get();
    app_lcn_searching_SQ_get(&(data.freq_strength), &(data.freq_quality));
    data.tp_id = app_lcn_searching_tp_id_get();
    data.nvod_service_id = prog->nvod_service_id;
    memcpy(data.prog_name, prog->prog_name, MAX_PROG_NAME);
    data.service_type = prog->service_type;
    app_lcn_list_add_noexist_one(&data);
#endif
	return 0;
}
#endif
#endif

int app_search_service_ext(GxMessage *pMsg)
{
#define BUFFER_LENGTH_	200
	GxMsgProperty_NewProgGet* pNewProgGet = NULL;
	GxMsgProperty_SatTpReply* pSatTpReply = NULL;
	GxMsgProperty_StatusReply* pStatusReply = NULL;
	GxMsgProperty_BlindScanReply *pBlindReply = NULL;
    char Buffer[BUFFER_LENGTH_] = {0};
    char* PolarStr[2] = {"H", "V"};
    float Progress=0, MaxSat=0, CurSat=0, MaxTp=0, CurTp=0;
    uint32_t intProgress=0;
    static uint32_t TPNumber = 0;
    static uint32_t blind_progress = 0;
	static uint32_t blind_progress_bak = 0;
//    app_log_debug("[APP Search], service get, id = %d\n", pMsg->msg_id);
	switch(pMsg->msg_id)
	{
		case GXMSG_SEARCH_NEW_PROG_GET:
			{
				pNewProgGet = (GxMsgProperty_NewProgGet*)GxBus_GetMsgPropertyPtr(pMsg,
										GxMsgProperty_NewProgGet);
				//need to save new program
				if(GXBUS_PM_PROG_NOT_EXIST == pNewProgGet->flag)
				{
					sg_StopType.m_nHaveNewProg = TRUE;
				}
				if( GXBUS_PM_PROG_RESERVED_TV == pNewProgGet->type || GXBUS_PM_PROG_RESERVED_RADIO == pNewProgGet->type)
				{
#if RESERVED_SERVICE_NOSAVE_SUPPORT
					break;
#endif
                    if(GXBUS_PM_PROG_RESERVED_TV == pNewProgGet->type)
                        pNewProgGet->type = GXBUS_PM_PROG_TV;
                    else
                        pNewProgGet->type = GXBUS_PM_PROG_RADIO;
				}
			#if FONT_GB2312_TO_UTF8
				app_str_gb2312_convert_utf8(pNewProgGet->name, MAX_PROG_NAME);
			#endif
			#if WEBAPI_SUPPORT
				app_webapi_search_prog_save(pNewProgGet);
			#endif
			#if SEARCH_SHOW_LCN_SUPPORT
				uint16_t prog_lcn;

                prog_lcn = app_lcn_list_lcn_get(pNewProgGet->original_id, pNewProgGet->ts_id,
                        pNewProgGet->service_id, app_lcn_searching_frequency_get());
				if(INVALID_LCN == prog_lcn)
				{
					app_log_debug("search no lcn find\n");
					prog_lcn = pNewProgGet->service_id;
				}
				if(GXBUS_PM_PROG_TV == pNewProgGet->type)
				{
					_search_show_lcn_tv_channel(prog_lcn, sg_TvCount, pNewProgGet);
					sg_TvCount++;
				}
				else if(GXBUS_PM_PROG_RADIO == pNewProgGet->type)
				{
					_search_show_lcn_radio_channel(prog_lcn, sg_RadioCount, pNewProgGet);
					sg_RadioCount++;
				}

			#else
				if(GXBUS_PM_PROG_TV == pNewProgGet->type)
				{
					_search_show_tv_channel(sg_TvCount, pNewProgGet);
					sg_TvCount++;
				}
				else if(GXBUS_PM_PROG_RADIO == pNewProgGet->type)
				{
					_search_show_radio_channel(sg_RadioCount, pNewProgGet);
					sg_RadioCount++;
				}
            #endif
#if(DVBC_TUNE_AUTO_SUPPORT > 0)
            _search_modify_dvbc_tp(gSearchDvbc_TpID);
            gSearchDvbc_TpID = -1;
#endif
			}
			break;

		case GXMSG_SEARCH_SAT_TP_REPLY:
			{

    			GxMsgProperty_NodeByIdGet pmdata_sat = {0};
    			GxMsgProperty_NodeByIdGet pmdata_tp = {0};

				//app_log_debug("[APP Search] recieved msg: GXMSG_SEARCH_SAT_TP_REPLY\n");
				pSatTpReply = (GxMsgProperty_SatTpReply*)GxBus_GetMsgPropertyPtr(pMsg, GxMsgProperty_SatTpReply);
                //app_log_debug("[APP Search] sat num: %d, name: %s, len:%d\n",pSatTpReply->sat_num,pSatTpReply->sat_name,strlen((const char *)pSatTpReply->sat_name));

                if((pSatTpReply->tp_max_count != 0)&&(pSatTpReply->tp_num == 0))
                {
                    break;
                }

                pmdata_tp.node_type = NODE_TP;
                pmdata_tp.id = pSatTpReply->tp_id;
                app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_tp);

                pmdata_sat.node_type = NODE_SAT;
                pmdata_sat.id = pmdata_tp.tp_data.sat_id;
                app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &pmdata_sat);
                app_tuner_cur_tuner_set(pmdata_sat.sat_data.tuner);
#if LCN_SUPPORT
                app_lcn_searching_tp_id_set(pSatTpReply->tp_id);
                app_lcn_searching_frequency_set(pSatTpReply->frequency);
                app_lcn_searching_SQ_set(0, 0);
#endif

                //show sat info
                memset(Buffer, 0, BUFFER_LENGTH_);
                if((pSatTpReply->nit_flag == NIT)&&(s_sat_search_sat_total > 1))
                {
                    uint32_t sat_num = 1;
                    sat_num = app_get_nit_search_sat_num(pmdata_sat.id);
                    pSatTpReply->sat_num = sat_num;
                }

                if(GXBUS_PM_SAT_S == pmdata_sat.sat_data.type)
                {
                    sprintf((char*)Buffer, "(%d / %d)  %s",
                            pSatTpReply->sat_num,
                            pSatTpReply->sat_max_count,
						pSatTpReply->sat_name); //(char*)pmdata_sat.sat_data.sat_s.sat_name  //bug60370
                    GUI_SetProperty("text_search_sat", "string", Buffer);
                }

                //show tp info
                memset(Buffer, 0, BUFFER_LENGTH_);
				char BufferCnt[BUFFER_LENGTH_] = {0};
                app_log_debug("search show tp freq = %d \n",pSatTpReply->frequency);
				sprintf((char*)BufferCnt, "(%d / %d) ",
						pSatTpReply->tp_num,
						pSatTpReply->tp_max_count);
				if(pSatTpReply->nit_flag == NIT)
					strcat((char*)BufferCnt, "[NIT]");

				#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
				if(GXBUS_PM_SAT_DVBT2 == pmdata_sat.sat_data.type)
				{
					#if DEMOD_DVB_T
					sprintf((char*)Buffer, "%s %d.%d", BufferCnt,
							pSatTpReply->frequency/1000,
							(pSatTpReply->frequency/100)%10);
					#endif
					#if DEMOD_ISDBT
					sprintf((char*)Buffer, "%s %d.%d", BufferCnt,
							pSatTpReply->frequency/1000,
							pSatTpReply->frequency%1000);
					#endif
				}
				#endif

#if DEMOD_DVB_C
                if(GXBUS_PM_SAT_C == pmdata_sat.sat_data.type)
                {
#if	(DVBC_TUNE_AUTO_SUPPORT > 0)
                    sprintf((char *)Buffer,
		                    "%s %d.%d / %s	/%s", BufferCnt,
		                    pSatTpReply->frequency / 1000,
		                    (pSatTpReply->frequency / 100) % 10,
		                    "Auto",
		                    "Auto");
                     gSearchDvbc_TpID = pSatTpReply->tp_id;
#else
                    char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};
                    if((pSatTpReply->tp_c.modulation >= DVBC_16QAM) && (pSatTpReply->tp_c.modulation <= DVBC_256QAM))
                        sprintf((char*)Buffer,
                                "%s %d.%d / %s  /%d", BufferCnt,
                                pSatTpReply->frequency/1000,
                                (pSatTpReply->frequency/100)%10,
                                modulation_str[pSatTpReply->tp_c.modulation-DVBC_16QAM],
                                pSatTpReply->tp_c.symbol_rate);
#endif
                    }
#endif

#if DEMOD_DTMB
			  if(GXBUS_PM_SAT_DTMB == pmdata_sat.sat_data.type)
                {
                    if(GXBUS_PM_SAT_1501_DTMB == pmdata_sat.sat_data.sat_dtmb.work_mode)
                   	{
                        char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};
                        sprintf((char*)Buffer, "%s %d.%d / %s", BufferCnt,
                                pSatTpReply->frequency/1000,
                                (pSatTpReply->frequency/100)%10,
                                bandwidth_str[pSatTpReply->tp_dtmb.bandwidth]);
					}
					else
					{
						char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};
						if((pSatTpReply->tp_dtmb.modulation >= DVBC_16QAM) && (pSatTpReply->tp_dtmb.modulation <= DVBC_256QAM))
                            sprintf((char*)Buffer, "%s %d.%d / %s  /%d", BufferCnt,
                                    pSatTpReply->frequency/1000,
                                    (pSatTpReply->frequency/100)%10,
                                    modulation_str[pSatTpReply->tp_dtmb.modulation-DVBC_16QAM],
                                    pSatTpReply->tp_dtmb.symbol_rate);
					}
                }
#endif

#if DEMOD_DVB_S
                if(GXBUS_PM_SAT_S == pmdata_sat.sat_data.type)
                {
					sprintf((char*)Buffer, "%s %d / %s / %d", BufferCnt,
                                pSatTpReply->frequency,
                                (strlen((const char*)pSatTpReply->sat_name) == 0) ?  "NULL" : (char *)PolarStr[pSatTpReply->tp_s.polar],
                                pSatTpReply->tp_s.symbol_rate);
                    }
#endif
                if(pSatTpReply->tp_max_count == 0)
                {
                    app_clear_invalid_tp_buffer(Buffer,BUFFER_LENGTH_);
                }
                GUI_SetProperty("text_scan_tp_info", "state", "hide");
                GUI_SetProperty("text_scan_tp_value", "state", "hide");
                GUI_SetProperty("text_search_tp", "string", Buffer);

                //show progress
                MaxSat = pSatTpReply->sat_max_count;
                CurSat = pSatTpReply->sat_num;
                MaxTp = pSatTpReply->tp_max_count;
                CurTp = pSatTpReply->tp_num;
                if(sg_search_type == SEARCH_TP)
                {
                    Progress = (MaxTp==1)? 60: (100*((CurTp/MaxTp)) -1);
                }
                else
                {
                    Progress = 100*((CurSat-1)/MaxSat)+100*((1/MaxSat)*(CurTp/MaxTp)) -1;
                }
                intProgress = Progress;
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetInterface("flush",NULL);

#if WEBAPI_SUPPORT
                s_search_progress = Progress;
#endif

                memset(Buffer,0,sizeof(Buffer));
                sprintf((char*)Buffer, "%d%%", intProgress);
                GUI_SetProperty("text_search_progress_percent","string",Buffer);
                app_update_pop_dlg(WND_POP_TIP);
            }
            break;

        case GXMSG_SEARCH_STATUS_REPLY:
            {
                pStatusReply = (GxMsgProperty_StatusReply*)GxBus_GetMsgPropertyPtr(pMsg,
                        GxMsgProperty_StatusReply);
                if(SEARCH_DBASE_OVERFLOW == pStatusReply->type)
                {
                    sg_StopType.m_StopType = SEARCH_OVERFLOW;
#if SEARCH_AUTO_SAVE_SUPPORT
                    if(sg_StopType.m_nHaveNewProg)
                    {
                        GxBus_PmLock();
                        app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
                        GxBus_PmUnlock();
                    }
                    #if LCN_SUPPORT
                    #if LCN_CONFLICT_HANDLE_SUPPORT
                    if(sg_StopType.m_nHaveNewProg)
                        app_lcn_handle_conflict_exec();
                    #endif
                    if(app_lcn_update_flag_get())
                        app_lcn_list_info_save();
                    #endif
                    APP_TIMER_ADD(s_SearchTipsTimer, _searchtips_timeout_cb, 1000, TIMER_REPEAT);
#endif
                }
                else if(SEARCH_ERROR == pStatusReply->type)
                {
                    sg_StopType.m_StopType = SAERCH_ERROR;
                }
            }
            break;

        case GXMSG_SEARCH_STOP_OK:
            {
                s_blind_stage = BLIND_STAGE_NONE;
                intProgress = 100;
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetProperty("text_search_progress_percent","string","100%");
#if WEBAPI_SUPPORT
                s_search_progress = intProgress;
                if(s_search_auto_save>0)
                {
                    WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
                    app_webapi_search_auto_save();
                }
                else
#endif
                {
                    GUI_CreateDialog(WND_SEARCH_TIP);
                    #if SEARCH_AUTO_SAVE_SUPPORT
                    GUI_SetInterface("flush", NULL);
                    GxCore_ThreadDelay(500);

                    if(sg_StopType.m_nHaveNewProg)
                    {
                        GxBus_PmLock();
                        app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
                        GxBus_PmUnlock();
                    }
#if LCN_SUPPORT
                    if(sg_TvCount != 0 || sg_RadioCount != 0)
                    {
#if LCN_CONFLICT_HANDLE_SUPPORT
                        app_lcn_handle_conflict_exec();
#endif
                        if(app_lcn_update_flag_get())
                            app_lcn_list_info_save();
                    }
#endif
                    GxCore_ThreadDelay(1000);
                    #if FACTORY_TEST_SUPPORT
                    app_test_sort_channels_by_fta();
                    #endif
                    APP_TIMER_ADD(s_SearchTipsTimer, _searchtips_timeout_cb, 1000, TIMER_REPEAT);
                    #endif
                }
            }
            break;

        case GXMSG_SEARCH_BLIND_SCAN_REPLY:
            if(blind_progress >= blind_progress_bak ||(blind_progress == 0 && blind_progress_bak == 100))
            {
                blind_progress_bak = blind_progress;
            }
            pBlindReply = (GxMsgProperty_BlindScanReply*)GxBus_GetMsgPropertyPtr(pMsg,
                    GxMsgProperty_BlindScanReply);
            if(pBlindReply->type == BLIND_PROGRESS)
            {
                blind_progress = pBlindReply->progress;
            }
            if(pBlindReply->type == BLIND_TP)
            {
                //show sat info
                memset(Buffer, 0, BUFFER_LENGTH_);
                sprintf((char*)Buffer,
                        "(%d / %d)  %s",
                        pBlindReply->sat_num,
                        pBlindReply->sat_max_count,
                        pBlindReply->sat_name);
                GUI_SetProperty("text_search_sat", "string", Buffer);

                if((pBlindReply->frequency != 0) && (pBlindReply->tp_s.symbol_rate != 0))
                {
                    GxMsgProperty_NodeByIdGet sat_data = {0};

                    #if LCN_SUPPORT
                    app_lcn_searching_tp_id_set(pBlindReply->tp_id);
                    app_lcn_searching_frequency_set(pBlindReply->frequency);
                    #endif
                    sat_data.node_type = NODE_SAT;
                    sat_data.id = pBlindReply->sat_id;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_data);
                    app_tuner_cur_tuner_set(sat_data.sat_data.tuner);
                    //show tp info
                    memset(Buffer, 0, BUFFER_LENGTH_);
                    sprintf((char*)Buffer,
                            "(%d / %d)  %d / %s / %d",
                            TPNumber + 1, TPNumber + 1,
                            pBlindReply->frequency,
                            PolarStr[pBlindReply->tp_s.polar],
                            pBlindReply->tp_s.symbol_rate);

                    GUI_SetProperty("text_scan_tp_info", "state", "hide");
                    GUI_SetProperty("text_scan_tp_value", "state", "hide");
                    GUI_SetProperty("text_search_tp", "string", Buffer);

                    _search_show_tp(TPNumber, pBlindReply);

                    TPNumber++;
                }
            }
            else if(pBlindReply->type == BLIND_PROGRESS)
            {
                if(blind_progress >= blind_progress_bak)
                {
                    GUI_SetProperty("progbar_search_progress", "value", &pBlindReply->progress);
                    memset(Buffer,0,sizeof(Buffer));
                    sprintf((char*)Buffer, "%d%%", pBlindReply->progress);
                    GUI_SetProperty("text_search_progress_percent","string",Buffer);
#if WEBAPI_SUPPORT
                    s_search_progress = pBlindReply->progress;
#endif
                }
                if(pBlindReply->stage == BLIND_STAGE_TP)
                {
                    static int start = 0;
                    if(s_blind_stage == BLIND_STAGE_NONE)
                    {
                        start = 0;
                    }
                    else if(s_blind_stage == BLIND_STAGE_PROG)
                    {
						char* txt_buf = NULL;
						GUI_GetProperty("text_search_tp", "string", &txt_buf);
                        start = pBlindReply->progress;
						if(txt_buf != NULL)
						{
							GUI_SetProperty("text_search_tp", "string", NULL);
						}
                    }
                    GUI_SetProperty("text_scan_tp_info", "state", "show");
                    GUI_SetProperty("text_scan_tp_value", "state", "show");
                    GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
                    memset(Buffer,0,sizeof(Buffer));
                    sprintf(Buffer, "%d%%", (pBlindReply->progress - start) * 10);
                    GUI_SetProperty("text_scan_tp_value", "string", Buffer);
                }
                s_blind_stage = pBlindReply->stage;
            }
            break;

        case GXMSG_SEARCH_BLIND_SCAN_FINISH:
            {
                blind_progress = 0;
                blind_progress_bak = 0;
                s_blind_stage = BLIND_STAGE_NONE;
                intProgress = 100;
                GUI_SetInterface("flush",NULL);
                GUI_SetProperty("progbar_search_progress", "value", &intProgress);
                GUI_SetProperty("text_search_progress_percent","string","100%");
                GUI_SetInterface("flush",NULL);
#if WEBAPI_SUPPORT
                s_search_progress = intProgress;
                if(s_search_auto_save>0)
                {
                    WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
                    app_webapi_search_auto_save();
                }
                else
#endif
                {
                    GUI_CreateDialog(WND_SEARCH_TIP);
#if SEARCH_AUTO_SAVE_SUPPORT
                    GUI_SetInterface("flush",NULL);
                    GxCore_ThreadDelay(500);

                    if(sg_StopType.m_nHaveNewProg)
                    {
                        GxBus_PmLock();
                        app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
                        GxBus_PmUnlock();
                    }
                    if(sg_TvCount != 0 || sg_RadioCount != 0)
                    {
#if LCN_SUPPORT
#if LCN_CONFLICT_HANDLE_SUPPORT
                        if(sg_StopType.m_nHaveNewProg)
                            app_lcn_handle_conflict_exec();
#endif
                        if(app_lcn_update_flag_get())
                            app_lcn_list_info_save();
#endif
                    }
                    GxCore_ThreadDelay(1000);
                    #if FACTORY_TEST_SUPPORT
                    app_test_sort_channels_by_fta();
                    #endif
                    APP_TIMER_ADD(s_SearchTipsTimer, _searchtips_timeout_cb, 1000, TIMER_REPEAT);
#endif
                }
                TPNumber = 0;
                break;
            }
        default:
            break;
    }

    return GXMSG_OK;
}

uint32_t  _check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
    //app_log_debug("\n------[CAS filter control, ca_system_id = 0x%x]------\n",ca_system_id);
    if(((ca_system_id >= 0x900) && (ca_system_id <= 0x9ff))
        || ((ca_system_id >= 0xe00) && (ca_system_id <= 0xeff)))
        return 1;
    else
        return 0;
}


static int s_motor_move_cnt = 0;
void app_motor_move_cnt_set(int cnt)
{
	s_motor_move_cnt = cnt;
}

void app_send_motor_state_msg(MotorState tip, int32_t motor_state)
{
    if(tip != DISH_NONE)
    {
        GxMessage *msg_start = GxBus_MessageNew(GXMSG_FRONTEND_DISH_MOVE);
        GxMessage *msg_end = GxBus_MessageNew(GXMSG_FRONTEND_DISH_MOVE);
        MotorTip *motor_tip = GxBus_GetMsgPropertyPtr(msg_start, MotorTip);

        if(DISH_MOVING == tip)
            app_motor_move_cnt_set(DISH_MOVE_SEC+1);
        else
            app_motor_move_cnt_set(2);//tip of "out of range" show 2 seconds
        motor_tip->state = tip;
        motor_tip->motor_state = motor_state;
        motor_tip->flag = 1;//pop dialog
        GxBus_MessageSend(msg_start);
        for( ; s_motor_move_cnt > 0; s_motor_move_cnt--)
        {
            GxCore_ThreadDelay(1000);
        }
        motor_tip = GxBus_GetMsgPropertyPtr(msg_end, MotorTip);
        motor_tip->flag = 0;//end dialog
        GxBus_MessageSend(msg_end);
    }
}

/*#154997
 *_set_disqec_callback里之前每次设置22k,polar,diseqc都会延时100ms.
 *现在改为只有22k,polar,diseqc与上回设置不同才会在最后延时800ms,
 *每次都发diseqc命令是为了避免像拔掉信号线之后又插上这样的情况,如果
 *因为与上次设置的相同就不再发diseqc命令会导致之后的节目都搜不到.
 * */
void _set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k)
{
#define DISEQC_SET_DELAY_MS 800
    bool change_flag = false;
    GxMsgProperty_NodeByIdGet sat_node = {0};
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    AppFrontend_DiseqcChange diseqc_change = {0};
    AppFrontend_22KChange _22k_change = {0};
    AppFrontend_PolarChange polar_change = {0};

    sat_node.node_type = NODE_SAT;
    sat_node.id = sat_id;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
    {
        // add in 20121012
        // for polar
        if(sat_node.sat_data.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_OFF)
            return;
        AppFrontend_PolarState Polar = polar ;
        polar_change.polar = polar;

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_POLAR_CHANGE, &polar_change);
        if(FRONTEND_CHANGED == polar_change.state)
        {
            change_flag = true;
            app_ioctl(sat_node.sat_data.tuner, FRONTEND_POLAR_SET, (void*)&Polar);
        }

        // for 22k
        AppFrontend_22KState _22K = FRONTEND_22K_NULL;
        if(sat22k == SEC_TONE_OFF)
            _22K = FRONTEND_22K_OFF;
        else if(sat22k == SEC_TONE_ON)
            _22K = FRONTEND_22K_ON;
        else
            _22K = FRONTEND_22K_NULL;

        _22k_change._22k = _22K;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_22K_CHANGE, &_22k_change);
        if(FRONTEND_CHANGED == _22k_change.state)
        {
            change_flag = true;
            app_ioctl(sat_node.sat_data.tuner, FRONTEND_22K_SET, (void*)&_22K);
        }

        // for diseqc
        diseqc10.type = DiSEQC10;
        diseqc10.u.params_10.bPolar = polar;
        diseqc10.u.params_10.b22k = sat22k;
        diseqc10.u.params_10.chDiseqc = sat_node.sat_data.sat_s.diseqc10;

        diseqc_change.diseqc = &diseqc10;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &diseqc_change);
        if(FRONTEND_CHANGED == diseqc_change.state)
            change_flag = true;

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc10);

        diseqc11.type = DiSEQC11;
        diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
        diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
        diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc;
        diseqc11.u.params_11.chUncom = sat_node.sat_data.sat_s.diseqc11;

        diseqc_change.diseqc = &diseqc11;
        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &diseqc_change);
        if(FRONTEND_CHANGED == diseqc_change.state)
            change_flag = true;

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_DISEQC_SET_EX, &diseqc11);

        // TODO: change with SAT change
        if(sat_id != sg_SatId)
        {
            int32_t motor_state = app_get_motor_state_in_fullscreen();
            MotorState tip = DISH_NONE;
            sg_SatId = sat_id;
            if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
            {
                tip = app_diseqc12_goto_position(sat_node.sat_data.id, sat_node.sat_data.tuner,sat_node.sat_data.sat_s.diseqc12_pos, false, motor_state);
                app_send_motor_state_msg(tip, motor_state);
                change_flag = true;
            }
            else if((sat_node.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
            {
                LongitudeInfo local_longitude;
                local_longitude.longitude = sat_node.sat_data.sat_s.longitude;
                local_longitude.longitude_direct = sat_node.sat_data.sat_s.longitude_direct;
                tip = app_usals_goto_position(sat_node.sat_data.id, sat_node.sat_data.tuner,&local_longitude, NULL,false, motor_state);
                app_send_motor_state_msg(tip, motor_state);
                change_flag = true;
            }
            app_diseqc_sat_id_bak_set(sat_node.sat_data.id);
        }

        if(true == change_flag)
            GxCore_ThreadDelay(DISEQC_SET_DELAY_MS);
    }
}

int detect_specify_str(char *src_str)
{
 #define MAX_BUF_LEN     MAX_PROG_NAME+1
    char *specify_str[] = {
     "HD",
     "hd",
	 };
    int  i = 0;
    char *p = NULL;

    #define SPECIFY_COUNT sizeof(specify_str)/sizeof(char*)
    if(MAX_BUF_LEN <= (strlen(src_str) + 1))
    {// do not deal with
        return 0;
    }
    for(i = 0; i < SPECIFY_COUNT; i++)
    {
        if(NULL != (p = strstr(src_str, specify_str[i])))
        {
	    // such as "HD CCTV", "CCTV HD", or "CCTV HD CHANNEL 1"
			if( ((*(p-1) >= 0x20) || (p == src_str))
					&& ((*(p+2) == 0x20) || (*(p+2) == '\0')))
            {
				return 1;
			}
		}
	}
    return 0;
 }

static int _delete_progname_preamble_code(char *src_str)
{
    char standard_str[MAX_PROG_NAME + 2] = {0};
    char *standard_tmp_str = NULL;
    if(src_str[0] < 0x20)
    {
        memcpy(standard_str, src_str, MAX_PROG_NAME);
        standard_tmp_str = &(standard_str[0]);
        standard_tmp_str++;
        memcpy(src_str,standard_tmp_str, MAX_PROG_NAME);
    }
    return 0;
}

status_t _modify_prog_callback(GxBusPmDataProg* prog)
{
    //for dra audio type
    if(thizDraAudioId > 0)
    {
        if(prog->audio_count <= 0)
        {
            prog->audio_count = 1;
            prog->cur_audio_pid = thizDraAudioId;
            prog->cur_audio_type = GXBUS_PM_AUDIO_DRA;
        }
    }

    _delete_progname_preamble_code((char*)prog->prog_name);//部分不做处理的字符串需要去除前导码
    if (detect_specify_str((char*)prog->prog_name) || GXBUS_PM_PROG_H265 == prog->video_type)
    {
       prog->definition = GXBUS_PM_PROG_HD;
    }

    prog->audio_mode = GXBUS_PM_PROG_AUDIO_MODE_STEREO;

    if(app_volume_scope_status() == VOLUME_CHANNEL)
    {
        prog->audio_volume = AUDIO_VOLUME;
    }
    if(prog->service_type == GXBUS_PM_PROG_RESERVED_TV || prog->service_type == GXBUS_PM_PROG_RESERVED_RADIO)
    {
#if RESERVED_SERVICE_NOSAVE_SUPPORT
        return GXCORE_ERROR;
#endif
        if(prog->service_type == GXBUS_PM_PROG_RESERVED_TV)
            prog->service_type = GXBUS_PM_PROG_TV;
        else
            prog->service_type = GXBUS_PM_PROG_RADIO;
    }
#if LCN_SUPPORT
    LCNInfoOne_t data;

    memset(&data, 0, sizeof(LCNInfoOne_t));
    data.original_id = prog->original_id;
    data.ts_id = prog->ts_id;
    data.service_id = prog->service_id;
    data.network_id = INVALID_NETWORK_ID;
    data.lcn = INVALID_LCN;
    data.frequency = app_lcn_searching_frequency_get();
    app_lcn_searching_SQ_get(&(data.freq_strength), &(data.freq_quality));

    data.tp_id = app_lcn_searching_tp_id_get();
    data.nvod_service_id = prog->nvod_service_id;
    memcpy(data.prog_name, prog->prog_name, MAX_PROG_NAME);
    data.service_type = prog->service_type;
    app_lcn_list_add_noexist_one(&data);
#endif

    return GXCORE_SUCCESS;
}

int app_dra_audio_update_pmt(uint8_t *pPmtSection, int MaxLength)
{
    uint8_t *pPmt = NULL;
    int16_t Length = 0, pmtSectionLen = 0, progInfoLen = 0, esInfoLen = 0, descriptorLen = 0;
    uint8_t pDraTagBuf[5] = {0};
    int16_t draPID = 0;

    if(NULL == pPmtSection)
    {
        app_log_error("\n[Parameter Err] %s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    Length = ((int16_t)((pPmtSection[10]<<8)|(pPmtSection[11])))&0x0FFF;
    if(Length > 1024)
    {
        app_log_error("\n[Length Err] %s,%d\n",__FILE__,__LINE__);
        return -1;
    }

    progInfoLen = ((int16_t)((pPmtSection[10]<<8)|pPmtSection[11]))&0x0FFF; // prog info length
    pmtSectionLen = (((int16_t)pPmtSection[1]<<8)|pPmtSection[2])&0x0FFF;   // pmt section length

    pPmt = pPmtSection + 12 + progInfoLen;   //go to stream type
    // section length - program info length - offset[between section length to prog info length] - CRC
    Length = pmtSectionLen - (progInfoLen + 9 + 4); // The whole components's length

    while(Length > 0)
    {
        esInfoLen = ((int16_t)pPmt[3]<<8|pPmt[4])&0x0FFF;
        Length -= (esInfoLen + 5);

        if(pPmt[0] == DRA_STREAM_TYPE)
        {
            draPID = ((int16_t)pPmt[1]<<8 | pPmt[2]) & 0x1FFF;  //elementary_PID
            //if find dra id then dump out

            pPmt += 5;
            while(esInfoLen > 0) //To find DRA1 word
            {
                descriptorLen = (int16_t)pPmt[1];
                esInfoLen -= (descriptorLen + 2);

#if 0
                int j = 0;
                app_log_debug("\n**********************[CA2 DATA]****************************\n");
                for(j = 0; j<descriptorLen; j++)
                {
                    app_log_debug("%02x ",pPmt[j]);
                    if(((j + 1)%16) == 0)
                        app_log_debug("\n");
                }
                app_log_debug("\n**************************[END]*****************************\n");
#endif

                if((pPmt[0] == DRA_TAG_TYPE))  //DRA's private tag
                {
                    if(descriptorLen == 4)
                    {
                        memcpy(pDraTagBuf, (pPmt+2), 4);
                        if(strcmp((char *)pDraTagBuf, "DRA1") == 0)
                            return draPID;
                    }
                }
                if(esInfoLen >= 0)
                    pPmt = pPmt + descriptorLen + 2;
            }
        }
        else
        {
            if(Length >= 0)
                pPmt = pPmt + 5 + esInfoLen;
        }
    }

    return 0;
}

private_parse_status _pmt_private_function(uint8_t *p_section_data, uint32_t len)
{
#if 0 //pmt data log
    int i = 0;
    app_log_debug("\n**************************** PMT DATE len = %d******************************\n", len);
    for(i = 0; i < len; i++)
    {
        app_log_debug("%02x ",p_section_data[i]);
        if(((i + 1)%16) == 0)   app_log_debug("\n");
    }
    app_log_debug("\n***************************************************************\n");
#endif

    thizDraAudioId = app_dra_audio_update_pmt(p_section_data,len);

    //return PRIVATE_SECTION_OK;
    return PRIVATE_SUBTABLE_OK;
}

void app_search_set_search_type(SearchType type)
{
    sg_search_type = type;
    return;
}
void app_search_set_search_mode(SearchMode  Mode)
{
    sg_search_mode = Mode;
    return;
}

typedef struct _app_search_si_subtable__ok_for_nit
{
    uint32_t    msg_si_subtable_ok_for_nit;
    void        (*app_si_subtable_stopfilter)(void);
    int        (*app_si_subtable_afterfilter_start)(void);
}app_search_si_subtable__ok_for_nit;
static app_search_si_subtable__ok_for_nit thizFlagProcessMsgSiSubtableOkForNitSection;

uint32_t app_search_get_msg_process_si_subtable(void)
{
    return thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit;
}

void app_search_set_msg_process_si_subtable(void (*p1)(void),int (*p2)(void))
{
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter =p1;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start = p2;
    thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit = 1;
}

void app_search_clr_msg_process_si_subtable(void)
{
    thizFlagProcessMsgSiSubtableOkForNitSection.msg_si_subtable_ok_for_nit = 0;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter = NULL;
    thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start = NULL;
}

void app_search_stopfilter_si_subtable(void)
{
    if(NULL!=thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter)
    {
        thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_stopfilter();
    }
}

void app_search_afterfilterok_si_subtable(void)
{
    if(NULL!=thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start)
    {
        thizFlagProcessMsgSiSubtableOkForNitSection.app_si_subtable_afterfilter_start();
    }
}

SIGNAL_HANDLER int app_search_create(GuiWidget *widget, void *usrdata)
{
#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_unset_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(0);
#endif

    char Buffer[50] = {0};
    uint32_t intProgress=0;

#if (OTA_SUPPORT > 0)
    extern void app_ota_backend_polling_stop(void);
    app_ota_backend_polling_stop();
#endif

    g_AppFullArb.timer_stop();
    //app_log_info("\n[APP Search], create search dialog\n");
    g_AppPmt.stop(&g_AppPmt);

#if EMM_SUPPORT
    app_emm_release_all();
    app_cat_release();
#endif

#if TKGS_SUPPORT
    g_AppTkgsBackUpgrade.stop();
    app_play_update_prog_tp_id(0); //clean played progam tp id
#endif

    if(GXCORE_INVALID_POINTER == thiz_search_mutex)
    {
        if(GXCORE_SUCCESS != GxCore_MutexCreate(&thiz_search_mutex))
            app_log_error("\033[31m create mutex failed\033[0m\n");
    }

    sg_TvCount=0;
    sg_RadioCount=0;

    sg_StopType.m_StopType = SEARCH_OK;
    sg_StopType.m_nHaveNewProg = FALSE;
    app_seach_clear_signal_show();
    if(reset_timer(sp_SearchTimerSignal) != 0)
    {
        sp_SearchTimerSignal = create_timer(timer_search_show_signal, 300, NULL, TIMER_REPEAT);
    }

    if(sg_search_type == SEARCH_SAT)
    {
        if(sg_sat_search_mode == SAT_SEARCH_AUTO)
        {
            _set_auto_search_display();
        #if (DEMOD_DVB_S > 0)
            if(true == app_get_super_search_flag())
                GUI_SetProperty("text_search_title", "string", STR_ID_SUPER_SAT_SEARCH);
            else
            #endif
                GUI_SetProperty("text_search_title", "string", STR_ID_SAT_SEARCH);
        }
        else
        {
            s_blind_stage = BLIND_STAGE_NONE;
            _set_blind_search_display();
            #if (DEMOD_DVB_S > 0)
            if(true == app_get_super_search_flag())
                GUI_SetProperty("text_search_title", "string", STR_ID_SUPER_BLIND_SEARCH);
            else
            #endif
                GUI_SetProperty("text_search_title", "string", STR_ID_BLIND_SEARCH);

            GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
            GUI_SetProperty("text_scan_tp_value", "string", "0%");
        }
    }
    else if(sg_search_type == SEARCH_BLIND_ONLY)
    {
        s_blind_stage = BLIND_STAGE_NONE;
        _set_blind_search_display();
#if (DEMOD_DVB_S > 0)
		if(true == app_get_super_search_flag())
			GUI_SetProperty("text_search_title", "string", STR_ID_SUPER_BLIND_SEARCH);
		else
#endif
        	GUI_SetProperty("text_search_title", "string", STR_ID_BLIND_SEARCH);

        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }
    else if(sg_search_type == SEARCH_TP)
    {
        _set_auto_search_display();
#if (DEMOD_DVB_S > 0)
        if(true == app_get_super_search_flag())
            GUI_SetProperty("text_search_title", "string", STR_ID_SUPER_TP_SEARCH);
        else
#endif
            GUI_SetProperty("text_search_title", "string", STR_ID_TP_SEARCH);
    }
    else if(sg_search_type == SEARCH_CABLE)
    {
        _set_auto_search_display();
#if (DEMOD_DVB_C > 0)
        GUI_SetProperty("text_search_title", "string", STR_ID_DVBC_SEARCH);
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
#endif
    }
    else if(sg_search_type == SEARCH_DVBT2)
    {
        _set_auto_search_display();
#if (DEMOD_ISDBT > 0)
        GUI_SetProperty("text_search_title", "string", STR_ID_ISDBT_SEARCH);
#elif (DEMOD_DVB_T > 0)
        if(sg_search_mode == AUTO_SEARCH)
            GUI_SetProperty("text_search_title", "string", STR_ID_AUTO_SEARCH);
        else if (sg_search_mode == MANUAL_SEARCH)
            GUI_SetProperty("text_search_title", "string", STR_ID_MANUAL_SEARCH);
        else
            GUI_SetProperty("text_search_title", "string", STR_ID_DVBT_SEARCH);
#endif
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
    }
	else if(sg_search_type == SEARCH_DTMB)
    {
        _set_auto_search_display();
#if (DEMOD_DTMB > 0)
        GUI_SetProperty("text_search_title", "string", STR_ID_DTMB_SEARCH);
#endif
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");

    }
#if (NET_SEARCH_SUPPORT > 0)
    else if(sg_search_type == SEARCH_SERVER)
	{
        _set_auto_search_display();
		GUI_SetProperty("text_search_title", "string", STR_ID_SERVER_SERACH);
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");

	}
	else if(sg_search_type == SEARCH_STREAM)
	{
        _set_auto_search_display();
		GUI_SetProperty("text_search_title", "string", STR_ID_STREAM_SERACH);
        GUI_SetProperty("text_scan_tp_info", "string", STR_ID_SCANTP_INFO);
        GUI_SetProperty("text_scan_tp_value", "string", "0%");
	}
#endif

    memset(Buffer,0,sizeof(Buffer));
    sprintf((char*)Buffer, "%d%%", intProgress);
    GUI_SetProperty("text_search_progress_percent","string",Buffer);
    GUI_SetProperty("progbar_search_progress", "value", &intProgress);
#if WEBAPI_SUPPORT
    s_search_progress = intProgress;
    s_search_state = 1;
    s_search_prog_total = 0;
#endif
    app_panel_show_prog_num(PANEL_UNDISPLAY_TIME);

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_destroy(GuiWidget *widget, void *usrdata)
{
    int value = 0;
    remove_timer(sp_SearchTimerSignal);
    sp_SearchTimerSignal = NULL;

    s_blind_stage = BLIND_STAGE_NONE;

    if(GXCORE_INVALID_POINTER != thiz_search_mutex)
    {
        GxCore_MutexDelete(thiz_search_mutex);
        thiz_search_mutex = GXCORE_INVALID_POINTER;
    }

#if DOUBLE_S2_SUPPORT
    if(GxFrontend_HasSpecialFeatures(0))
    {
        GxFrontend_CleanRecordPara(0);
        GxFrontend_PlsNSourceSelect(0, FE_PLSN_FROM_AUTO_CALC);
    }
    if(GxFrontend_HasSpecialFeatures(1))
    {
        GxFrontend_CleanRecordPara(1);
        GxFrontend_PlsNSourceSelect(1, FE_PLSN_FROM_AUTO_CALC);
    }
#else
    if(GxFrontend_HasSpecialFeatures(app_tuner_cur_tuner_get()))
    {
        GxFrontend_CleanRecordPara(app_tuner_cur_tuner_get());
        GxFrontend_PlsNSourceSelect(app_tuner_cur_tuner_get(), FE_PLSN_FROM_AUTO_CALC);
    }
#endif

#if NDEMOD_WITH_1TUNER
    app_only_cur_frontend_monitor_start(app_tuner_cur_tuner_get());
#else
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
#endif
    app_panel_ioctl_handle(PANEL_SHOW_QUALITY,&value);
    g_AppTime.start(&g_AppTime);//for bug 142508
    g_AppChOps.update_prog_num();

#if SAT2IP_SERVER_SUPPORT
	app_sat2ip_set_gui_ok();
#endif
#if DVB2IP_SERVER_SUPPORT
    app_dvb2ip_gui_flag_set(1);
#endif
#if WEBAPI_SUPPORT
    s_search_state = 0;
    s_search_prog_total = 0;
    gxapps_mutex_lock();
    if(s_search_prog_list)
    {
        gx_list_free(s_search_prog_list);
        s_search_prog_list = NULL;
    }
    gxapps_mutex_unlock();
#endif
#if LCN_SUPPORT
    app_lcn_list_destroy();
    s_SearchNitMode = false;
#endif
#if PANEL_SETTING_SUPPORT
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
	int32_t pNitSubtId=0;
	uint32_t pNitRequestId=0;
#endif

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    {
                        switch(sg_search_type)
                        {
                            case SEARCH_SAT:
                            case SEARCH_BLIND_ONLY:
                            case SEARCH_TP:
                                if(SAT_SEARCH_BLINDE == sg_sat_search_mode)
                                    app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_STOP, (void *)NULL);
                                else
                                    app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
                                break;

                            case SEARCH_CABLE:
                            case SEARCH_DTMB:
                                #if (DEMOD_DVB_C > 0) || (DEMOD_DTMB > 0)
                                if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                                    GxCore_MutexLock(thiz_search_mutex);

                                app_table_nit_get_search_filter_info(&pNitSubtId, &pNitRequestId);
                                if (-1 == pNitSubtId)
                                    app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);

                                if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                                    GxCore_MutexUnlock(thiz_search_mutex);
                                #endif
                                break;

                            case SEARCH_DVBT2:
                                app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
                                break;
#if (NET_SEARCH_SUPPORT > 0)
							case SEARCH_SERVER:
							case SEARCH_STREAM:
								app_send_msg_exec(GXMSG_SEARCH_SCAN_STOP, (void *)NULL);
								break;
#endif
                            default:
                                break;
                        }
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}

SIGNAL_HANDLER int app_search_service(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_KEEPON;
    GUI_Event *event = NULL;
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            app_search_service_ext((GxMessage*)(event->msg.service_msg));
            break;
        default:
            break;
    }
    return ret;
}

#define SEARCH_TIP
SIGNAL_HANDLER int app_search_tip_create(GuiWidget *widget, void *usrdata)
{
    if(SEARCH_OK == sg_StopType.m_StopType)
    {
#if SEARCH_AUTO_SAVE_SUPPORT
        uint8_t buf[10] = {0};
        if (sg_StopType.m_nHaveNewProg) //need to save
        {
            GUI_SetProperty("text_search_tip", "string", STR_ID_SAVING_WAIT);
            GUI_SetProperty("text_search_tip", "state", "show");
            memset(buf, 0, 10);
            sprintf((void *)buf, "%03d", sg_TvCount);
            GUI_SetProperty("text_search_tip_tv_num", "string", buf);
            GUI_SetProperty("text_search_tip_tv_total", "state", "show");
            GUI_SetProperty("text_search_tip_tv_num", "state", "show");
            memset(buf, 0, 10);
            sprintf((void *)buf, "%03d", sg_RadioCount);
            GUI_SetProperty("text_search_tip_radio_num", "string", buf);
            GUI_SetProperty("text_search_tip_radio_total", "state", "show");
            GUI_SetProperty("text_search_tip_radio_num", "state", "show");
        }
        else//no new channel,need not to save
        {
            GUI_SetProperty("text_search_tip", "string", STR_ID_NO_NEW_CH);
            GUI_SetProperty("text_search_tip", "state", "show");
            GUI_SetProperty("text_search_tip_tv_total", "state", "hide");
            GUI_SetProperty("text_search_tip_tv_num", "state", "hide");
            GUI_SetProperty("text_search_tip_radio_total", "state", "hide");
            GUI_SetProperty("text_search_tip_radio_num", "state", "hide");
        }
        GUI_SetProperty("img_search_tip_opt", "state", "hide");
        GUI_SetProperty("btn_search_tip_ok", "state", "hide");
        GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
#else
        if(sg_StopType.m_nHaveNewProg)//need to save
        {
            GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
            GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4");
            //cancel and ok
        }
        else//no new channel,need not to save
		{
			GUI_SetProperty("text_search_tip", "string", STR_ID_NO_NEW_CH);
			//only ok
			GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
		}
#endif
	}
	else if(SEARCH_OVERFLOW== sg_StopType.m_StopType)//overflow
	{
        uint32_t tp_num = GxBus_PmTpNumGet();
#if SEARCH_AUTO_SAVE_SUPPORT
        GUI_SetProperty("text_search_tip", "state", "show");
        GUI_SetProperty("text_search_tip_tv_total", "state", "hide");
        GUI_SetProperty("text_search_tip_tv_num", "state", "hide");
        GUI_SetProperty("text_search_tip_radio_total", "state", "hide");
        GUI_SetProperty("text_search_tip_radio_num", "state", "hide");
#endif
        if(tp_num >= SYS_MAX_TP)
        {
		    GUI_SetProperty("text_search_tip", "string", STR_ID_MAX_TP);
        }
        else
        {
		    GUI_SetProperty("text_search_tip", "string", STR_ID_MAX_CHANNEL);
        }
		//only ok
		GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
	}
	else //error
	{
	    GUI_SetProperty("text_search_tip", "string", STR_ID_ERR_OCCUR);
		//only ok
	    GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
	}

	GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4");
	GUI_SetFocusWidget("btn_search_tip_ok");

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_tip_destroy(GuiWidget *widget, void *usrdata)
{
#if SEARCH_AUTO_SAVE_SUPPORT
#if LCN_SUPPORT
#if DEMOD_DVB_C && DEMOD_DVB_T && 0 == DEMOD_DVB_S && NDEMOD_WITH_1TUNER
	int32_t sort_flag;
	GxBus_ConfigGetInt(T2_SORT_FLAG, &sort_flag, T2_SORT_FLAG_VALUE);
	app_t2_sort_program(sort_flag);
#elif DEMOD_DVB_C || DEMOD_DTMB
    int sort_value = 0;

    GxBus_ConfigGetInt(PROGRAM_LCN_SORT_KEY, &sort_value, PROGRAM_LCN_SORT_VALUE);
    if(0 == sort_value)
        return EVENT_TRANSFER_STOP;

    app_sort_program_by_lcn();
#endif
#endif
#endif
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_search_tip_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;
	uint8_t FocusdOnOk=0;

#if SEARCH_AUTO_SAVE_SUPPORT
    return ret;
#endif
	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_MENU:
                case STBK_EXIT:
                    {
                        if(SEARCH_OK == sg_StopType.m_StopType)
                        {
                            app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                            GUI_EndDialog(WND_SEARCH_TIP);
                            GUI_EndDialog(WND_SEARCH);
                        }
                        else//error and overflow
                        {
                            if(sg_StopType.m_nHaveNewProg)
                            {
                                GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
                                GUI_SetProperty("btn_search_tip_cancel", "state", "show");
                                sg_StopType.m_StopType = SEARCH_OK;
                            }
                            else
                            {
                                GUI_EndDialog(WND_SEARCH_TIP);
                                GUI_EndDialog(WND_SEARCH);
                            }
                        }
                    }
                    break;

                case STBK_LEFT:
                case STBK_RIGHT:
                    if((SEARCH_OK == sg_StopType.m_StopType)
						&& (sg_StopType.m_nHaveNewProg) )//choose save or not?
					{
						if(0 == strcasecmp("btn_search_tip_ok", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 1;
						}
						else if(0 == strcasecmp("btn_search_tip_cancel", GUI_GetFocusWidget()))
						{
							FocusdOnOk = 0;
						}

						if(1 == FocusdOnOk)
						{
							GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4_l");
							GUI_SetFocusWidget("btn_search_tip_cancel");
						}
						else
						{
							GUI_SetProperty("img_search_tip_opt","img","s_bar_choice_blue4");
							GUI_SetFocusWidget("btn_search_tip_ok");
						}

					}
					break;

				case STBK_OK:
                    {
                        if(SEARCH_OK == sg_StopType.m_StopType)
                        {
                            if(sg_StopType.m_nHaveNewProg)//need to save
                            {
                                if(0 == strcasecmp("btn_search_tip_ok", GUI_GetFocusWidget()))
                                {
                                    //save
                                    GUI_SetProperty("text_search_tip", "string", STR_ID_SAVING_WAIT);
                                    GUI_SetProperty("text_search_tip", "draw_now", NULL);

                                    GUI_SetProperty("btn_search_tip_cancel", "state", "hide");
                                    GUI_SetProperty("btn_search_tip_cancel", "draw_now", NULL);

                                    GUI_SetProperty("btn_search_tip_ok", "state", "hide");
                                    GUI_SetProperty("btn_search_tip_ok", "draw_now", NULL);

                                    GUI_SetProperty("img_search_tip_opt", "state", "hide");
                                    GUI_SetProperty("img_search_tip_opt", "draw_now", NULL);

                                    GxBus_PmLock();
                                    app_send_msg_exec(GXMSG_SEARCH_SAVE, (void *)NULL);
                                    GxBus_PmUnlock();
									#if 0//def AUDIO_DESCRIPTOR_SUPPORT
									GxBus_PmSync(GXBUS_PM_SYNC_EXT_PROG);
									#endif
									#if FACTORY_TEST_SUPPORT
									app_test_sort_channels_by_fta();
									#endif
#if TKGS_SUPPORT	//liqg m, 2014.9.21
									app_tkg_lcn_flush();
									app_tkgs_sort_channels_by_lcn();//2014.09.25 add
#endif
                                    #if LCN_SUPPORT
                                    #if LCN_CONFLICT_HANDLE_SUPPORT
                                    app_lcn_handle_conflict_exec();
                                    #endif
                                    if(app_lcn_update_flag_get())
                                        app_lcn_list_info_save();
                                    #endif
                                    GxCore_ThreadDelay(800);
#if SAT2IP_SERVER_SUPPORT
                                    app_sat2ip_update_prog_total();
#endif
#if DVB2IP_SERVER_SUPPORT
                                    app_dvb2ip_prog_num_update();
#endif
                                }
                                else//cancel, no need to save
                                {
                                    app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                                }
                            }
                            else
                            {
                                app_send_msg_exec(GXMSG_SEARCH_NOT_SAVE, (void *)NULL);
                            }
                            GUI_EndDialog(WND_SEARCH_TIP);
                            GUI_EndDialog(WND_SEARCH);

#if FACTORY_TEST_SUPPORT

                            if(GUI_CheckDialog(WND_FACTORY_TEST) == GXCORE_SUCCESS)
                            {
                                GxMsgProperty_NodeNumGet prog_num = {0};
                                prog_num.node_type = NODE_PROG;
                                app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &prog_num);
                                if(prog_num.node_num)
                                {
                                    event->key.scancode = GUIK_ESCAPE;//STBK_EXIT
                                    GUI_SendEvent(WND_FACTORY_TEST, event);
                                }
                            }

							app_test_auto_search_timer_remove();
#endif

                        }
                        else//error and overflow
                        {
                            if(sg_StopType.m_nHaveNewProg)
                            {
                                GUI_SetProperty("text_search_tip", "string", STR_ID_SAVE_INFO);
                                GUI_SetProperty("btn_search_tip_cancel", "state", "show");
                                sg_StopType.m_StopType = SEARCH_OK;
                            }
                            else
                            {
                                GUI_EndDialog(WND_SEARCH_TIP);
                                GUI_EndDialog(WND_SEARCH);
                            }

                        }
                    }
                    break;

                default:
                    break;
            }
        default:
            break;
    }

    return ret;
}


int ex_sat_search_mode = 0;
static status_t app_satellite_search_para_get(void)
{
	uint32_t box_sel;
    GxMsgProperty_NodeByIdGet sat_node = {0};
    AppFrontend_Config cfg = {0};
    uint32_t i = 0;

    thizDraAudioId = -1;
	s_Search.scan_type = GX_SEARCH_AUTO;

	GUI_GetProperty("cmb_sat_scan_channel", "select", &box_sel);
	if(0 == box_sel)//all
		s_Search.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	else if(1 == box_sel)//tv
		s_Search.tv_radio = GX_SEARCH_TV;
	else if(2 == box_sel)//radio
		s_Search.tv_radio = GX_SEARCH_RADIO;

	GUI_GetProperty("cmb_sat_fta_only", "select", &box_sel);
	if(0 == box_sel)//no: all
		s_Search.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	else if(1 == box_sel)//yes: fta only
		s_Search.fta_cas = GX_SEARCH_FTA;

	GUI_GetProperty("cmb_sat_network", "select", &box_sel);
	if(0 == box_sel)//no
		s_Search.nit_switch = GX_SEARCH_NIT_DISABLE;
	else if(1 == box_sel)//yes
		s_Search.nit_switch = GX_SEARCH_NIT_ENABLE;

	if (sp_scan_id != NULL)
	{
		GxCore_Free(sp_scan_id);
		sp_scan_id = NULL;
	}
	if (sp_ts_src != NULL)
	{
		GxCore_Free(sp_ts_src);
		sp_ts_src = NULL;
	}
	sp_scan_id = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
    sp_ts_src = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
	if ((sp_scan_id == NULL) || (sp_ts_src == NULL))
			return -1;

	memcpy(sp_scan_id, sg_search_id_set->id_array,  sg_search_id_set->id_num * sizeof(uint32_t));
    for (i=0; i<sg_search_id_set->id_num; i++)
    {
        sat_node.node_type = NODE_SAT;
        sat_node.id = sp_scan_id[i];
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

        app_ioctl(sat_node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
        sp_ts_src[i] = cfg.ts_src;
    }

	s_Search.params_s.array = sp_scan_id;
    s_Search.params_s.ts = sp_ts_src;
    s_Search.demux_id = 0;
    s_Search.params_s.max_num = sg_search_id_set->id_num;

	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
    s_Search.modify_prog = _modify_prog_callback;
    s_Search.pmt_parse_fun = _pmt_private_function;

    s_Search.check_ca_fun = _check_ca_system;

	if(sg_search_type == SEARCH_BLIND_ONLY)
	{
		sg_sat_search_mode = SAT_SEARCH_BLINDE;

		s_Blind.max_num = sg_search_id_set->id_num;
		s_Blind.array = sp_scan_id;
		s_Blind.polar_type = BLIND_POLAR_ALL;
		s_Blind.search_type = BLIND_FAST;
#if (DEMOD_DVB_S > 0)
        if(true == app_get_super_search_flag())
            s_Blind.search_type = BLIND_SUPER;
#endif
		// add 20120618
		s_Blind.fta_cas		= s_Search.fta_cas;
		s_Blind.tv_radio	= s_Search.tv_radio;
		s_Blind.nit_switch  = s_Search.nit_switch;

        s_Blind.search_diseqc = _set_disqec_callback;
        s_Blind.modify_prog =  _modify_prog_callback;


        s_Blind.check_ca_fun = _check_ca_system;

        s_Blind.ts = sp_ts_src;
        memset(&(s_Blind.params_pid), 0 , sizeof(GxPidSearch));
        s_Blind.ext = NULL;
        s_Blind.ext_num = 0;
        s_Blind.time_out.pat = TIMEOUT_PAT;
        s_Blind.time_out.pmt = TIMEOUT_PMT;
        s_Blind.time_out.sdt = TIMEOUT_SDT;
        s_Blind.time_out.nit = TIMEOUT_NIT;
    }
    else if(sg_search_type == SEARCH_SAT)
    {
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode", "state", "disable");
            sg_sat_search_mode = SAT_SEARCH_AUTO;
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
			GUI_GetProperty("cmb_sat_scan_mode", "select", &box_sel);
			if(0 == box_sel)
			{
				sg_sat_search_mode = SAT_SEARCH_AUTO;
				ex_sat_search_mode = SAT_SEARCH_AUTO;
			}
			else if(1 == box_sel)//satellite blind search
			{
				sg_sat_search_mode = SAT_SEARCH_BLINDE;
				ex_sat_search_mode = SAT_SEARCH_BLINDE;

				s_Blind.max_num = sg_search_id_set->id_num;
				s_Blind.array = sp_scan_id;
				s_Blind.polar_type = BLIND_POLAR_ALL;
				s_Blind.search_type = BLIND_FAST;
				// add 20120618
				s_Blind.fta_cas		= s_Search.fta_cas;
				s_Blind.tv_radio	= s_Search.tv_radio;
				s_Blind.nit_switch  = s_Search.nit_switch;

	            s_Blind.search_diseqc = _set_disqec_callback;
                s_Blind.modify_prog = _modify_prog_callback;


				s_Blind.check_ca_fun = _check_ca_system;

                s_Blind.ts = sp_ts_src;
                //s_Blind.nit_switch = GX_SEARCH_NIT_DISABLE;
                memset(&(s_Blind.params_pid), 0 , sizeof(GxPidSearch));
                s_Blind.ext = NULL;
                s_Blind.ext_num = 0;
                s_Blind.time_out.pat = TIMEOUT_PAT;
                s_Blind.time_out.pmt = TIMEOUT_PMT;
                s_Blind.time_out.sdt = TIMEOUT_SDT;
                s_Blind.time_out.nit = TIMEOUT_NIT;
            }
        }
	}

#if LCN_SUPPORT
	g_SearchExtendList.extendnum = 0;

	if(NULL != app_search_add_extend_callback && false == s_SearchNitMode)
		app_search_add_extend_callback(&g_SearchExtendList);

	if(g_SearchExtendList.extendnum > 0)
	{
		memset(s_SearchExtParse, 0, SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        memcpy(&s_SearchExtParse[0], &g_SearchExtendList.searchExtend[0], SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        s_Blind.ext_num = s_Search.ext_num = g_SearchExtendList.extendnum;
        s_Blind.ext = s_Search.ext = &s_SearchExtParse[0];
	}

	app_lcn_list_init(sp_scan_id, sg_search_id_set->id_num);
#endif

    return GXCORE_SUCCESS;
}

SIGNAL_HANDLER int app_search_cmbox_change(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
    GUI_GetProperty("cmb_sat_scan_mode", "select", &box_sel);
    if(0 == box_sel) //Auto search
    {
		GUI_SetProperty("boxitem_sat_search_network", "enable", NULL);
        GUI_SetProperty("boxitem_sat_search_network", "state", "enable");
        GUI_SetProperty("cmb_sat_network","content",YES_NO_CONTENT);
        sg_search_type = SEARCH_SAT;
    }
    else if(1 == box_sel)//Blind search
    {
	uint32_t netsel = 0;
	GUI_SetProperty("cmb_sat_network","select",&netsel);
        GUI_SetProperty("boxitem_sat_search_network", "state", "disable");
        sg_search_type = SEARCH_BLIND_ONLY;
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_satellite_search_create(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	box_sel = 4;
	if(sg_search_type == SEARCH_BLIND_ONLY)
    {
        GUI_SetProperty("boxitem_sat_search_network", "state", "disable");
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode","state","disable");
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
        }
#if (DEMOD_DVB_S > 0)
		if(true == app_get_super_search_flag())
			GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_SUPER_BLIND_CONTENT);
		else
#endif
        	GUI_SetProperty("cmb_sat_scan_mode","content",SAT_SEARCH_BLIND_CONTENT);
	}
	else if(sg_search_type == SEARCH_SAT)
	{
		int32_t search_mode;
		GxBus_ConfigGetInt(SEARCH_MODE_KEY, &search_mode, SEARCH_MODE_VALUE);

		GUI_SetProperty("boxitem_sat_search_network", "enable", NULL);
        GUI_SetProperty("boxitem_sat_search_network", "state", "enable");
#if (DEMOD_DVB_S > 0)
        if(scan_mode_disable_flag == true)
        {
            GUI_SetProperty("boxitem_sat_search_mode","state","disable");
            sg_sat_search_mode = SAT_SEARCH_AUTO;
        }
        else
#endif
        {
            GUI_SetProperty("boxitem_sat_search_mode", "enable", NULL);
            GUI_SetProperty("boxitem_sat_search_mode", "state", "enable");
            sg_sat_search_mode = ((search_mode==0)?SAT_SEARCH_AUTO:SAT_SEARCH_BLINDE);
		}
#if (DEMOD_DVB_S > 0)
        if(true == app_get_super_search_flag())
            GUI_SetProperty("cmb_sat_scan_mode", "content", SAT_SEARCH_SUPER_MODE_CONTENT);
        else
#endif
            GUI_SetProperty("cmb_sat_scan_mode", "content", SAT_SEARCH_MODE_CONTENT);

		GUI_SetProperty("cmb_sat_scan_mode", "select", &search_mode);


	}
	GUI_SetProperty("cmb_sat_fta_only","content",YES_NO_CONTENT);
	GUI_SetProperty("cmb_sat_scan_channel","content",SCAN_CHANNEL_CONTENT);
	GUI_SetProperty("cmb_sat_network","content",YES_NO_CONTENT);
	GUI_SetProperty("box_sat_search_opt","select",&box_sel);
	return EVENT_TRANSFER_STOP;
}

#if (DEMOD_DVB_S > 0)
extern void app_antenna_setting_to_search(WndStatus wnd_ok);
extern void app_sat_list_to_search(WndStatus wnd_ok);
extern void app_tp_list_to_search(WndStatus wnd_ok);
#endif

static void _app_search_wnd_free(void)
{
    if ( sg_search_id_set->id_array != NULL )
    {
        GxCore_Free(sg_search_id_set->id_array);
        sg_search_id_set->id_array = NULL;
        sg_search_id_set->id_num = 0;
    }
}

static int _app_search_wnd_destroy(WndStatus wnd_ok)
{
    char* focus_win = NULL;

    GUI_SetInterface("flush",NULL);
    focus_win = (char*)GUI_GetFocusWindow();
    if(!focus_win)
    {
        _app_search_wnd_free();
        app_log_error("no focus window \n");
        return -2;
    }

    if ( wnd_ok != WND_OK )
    {
#if (DEMOD_DVB_S > 0)
        if(0 == strcasecmp(focus_win,WND_TP_LIST))
        {
            app_tp_list_to_search(WND_CANCLE);
        }
#endif
        _app_search_wnd_free();
        return 0 ;
    }

#if (DEMOD_DVB_S > 0)
    if(0 == strcasecmp(focus_win,WND_ANTENNA_SETTING))
    {
        app_antenna_setting_to_search(WND_OK);
    }
    else if(0 == strcasecmp(focus_win,WND_SAT_LIST))
    {
        app_sat_list_to_search(WND_OK);
    }
    else if(0 == strcasecmp(focus_win,WND_TP_LIST))
    {
        app_tp_list_to_search(WND_OK);
    }
#endif
    _app_search_wnd_free();

    return 0 ;
}

SIGNAL_HANDLER int app_satellite_search_box_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;
    uint32_t box_sel;
    uint32_t box_first_num = 0;

#if (DEMOD_DVB_S > 0)
    box_first_num = (scan_mode_disable_flag == true)?1:0;
#endif
    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog(WND_SAT_SEARCH_OPT);
						_app_search_wnd_destroy(WND_CANCLE);
					}
					break;

				case STBK_DOWN:
					{
						GUI_GetProperty("box_sat_search_opt", "select", &box_sel);
						if(4 == box_sel)
						{
							box_sel = box_first_num;
							GUI_SetProperty("box_sat_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_UP:
					{
						GUI_GetProperty("box_sat_search_opt", "select", &box_sel);

						if(box_first_num == box_sel)
						{
							box_sel = 4;
							GUI_SetProperty("box_sat_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_OK:
					{
						if(app_satellite_search_para_get() == GXCORE_SUCCESS)
						{
							GUI_EndDialog(WND_SAT_SEARCH_OPT);
							_app_search_wnd_destroy(WND_OK);
						}
						else
						{
							GUI_EndDialog(WND_SAT_SEARCH_OPT);
							_app_search_wnd_destroy(WND_CANCLE);
						}
						ret = EVENT_TRANSFER_STOP;
					}

					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static WndStatus app_sat_search_opt_dlg(void)
{
	GUI_CreateDialog(WND_SAT_SEARCH_OPT);
	return WND_EXEC;
}

static status_t app_tp_search_para_get(void)
{
    GxMsgProperty_NodeByIdGet node = {0};
	status_t ret = GXCORE_SUCCESS;
	uint32_t box_sel;

	s_Search.scan_type = GX_SEARCH_MANUAL;

	GUI_GetProperty("cmb_tp_scan_channel", "select", &box_sel);
	if(0 == box_sel)//all
		s_Search.tv_radio = GX_SEARCH_TV_RADIO_ALL;
	else if(1 == box_sel)//tv
		s_Search.tv_radio = GX_SEARCH_TV;
	else if(2 == box_sel)//radio
		s_Search.tv_radio = GX_SEARCH_RADIO;

	GUI_GetProperty("cmb_tp_fta_only", "select", &box_sel);
	if(0 == box_sel)//no: all
		s_Search.fta_cas = GX_SEARCH_FTA_CAS_ALL;
	else if(1 == box_sel)//yes: fta only
		s_Search.fta_cas = GX_SEARCH_FTA;

	GUI_GetProperty("cmb_tp_network", "select", &box_sel);
	if(0 == box_sel)//no
		s_Search.nit_switch = GX_SEARCH_NIT_DISABLE;
	else if(1 == box_sel)//yes
		s_Search.nit_switch = GX_SEARCH_NIT_ENABLE;

	if (sp_scan_id != NULL)
	{
		GxCore_Free(sp_scan_id);
		sp_scan_id = NULL;
	}
	if (sp_ts_src != NULL)
	{
		GxCore_Free(sp_ts_src);
		sp_ts_src = NULL;
	}
		sp_scan_id = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
		sp_ts_src = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
	if ((sp_scan_id == NULL) || (sp_ts_src == NULL))
		return -1;

	memcpy(sp_scan_id, sg_search_id_set->id_array,  sg_search_id_set->id_num * sizeof(uint32_t));
	if(sg_search_id_set->id_num)
	{
		AppFrontend_Config cfg = {0};
		uint32_t i = 0;

		node.node_type = NODE_TP;
		node.id = sp_scan_id[0];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		node.node_type = NODE_SAT;
		node.id = node.tp_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
		for(i = 0; i < sg_search_id_set->id_num; i++)
            sp_ts_src[i] = cfg.ts_src;
	}

	s_Search.params_s.array = sp_scan_id;
	s_Search.params_s.ts = sp_ts_src;
	s_Search.params_s.max_num = sg_search_id_set->id_num;

	// needn't ext
	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
    s_Search.modify_prog = _modify_prog_callback;
    s_Search.pmt_parse_fun = _pmt_private_function;


    s_Search.check_ca_fun = _check_ca_system;

#if LCN_SUPPORT
	g_SearchExtendList.extendnum = 0;

	if(NULL != app_search_add_extend_callback && false == s_SearchNitMode)
		app_search_add_extend_callback(&g_SearchExtendList);

	if(g_SearchExtendList.extendnum > 0)
	{
		memset(s_SearchExtParse, 0, SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        memcpy(&s_SearchExtParse[0], &g_SearchExtendList.searchExtend[0], SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        s_Search.ext_num = g_SearchExtendList.extendnum;
        s_Search.ext = &s_SearchExtParse[0];
	}

	app_lcn_list_init(&node.sat_data.id, 1);
#endif

	return ret;
}

SIGNAL_HANDLER int app_tp_search_create(GuiWidget *widget, void *usrdata)
{
	uint32_t box_sel;
	box_sel = 3;

	GUI_SetProperty("cmb_tp_fta_only","content",YES_NO_CONTENT);
	GUI_SetProperty("cmb_tp_scan_channel","content",SCAN_CHANNEL_CONTENT);
	GUI_SetProperty("cmb_tp_network","content",YES_NO_CONTENT);
	GUI_SetProperty("box_tp_search_opt","select",&box_sel);
	sg_sat_search_mode = SAT_SEARCH_AUTO;
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_tp_search_box_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type)
	{
		case GUI_SERVICE_MSG:
			break;

		case GUI_MOUSEBUTTONDOWN:
			break;

		case GUI_KEYDOWN:
			switch(event->key.sym)
			{
				case STBK_EXIT:
				case STBK_MENU:
					{
						GUI_EndDialog(WND_TP_SEARCH_OPT);
						_app_search_wnd_destroy(WND_CANCLE);
					}
					break;

				case STBK_DOWN:
					{
						uint32_t box_sel;

						GUI_GetProperty("box_tp_search_opt", "select", &box_sel);
						if(3 == box_sel)
						{
							box_sel = 0;
							GUI_SetProperty("box_tp_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_UP:
					{
						uint32_t box_sel;

						GUI_GetProperty("box_tp_search_opt", "select", &box_sel);
						if(0 == box_sel)
						{
							box_sel = 3;
							GUI_SetProperty("box_tp_search_opt", "select", &box_sel);
							ret = EVENT_TRANSFER_STOP;
						}
						else
						{
							ret = EVENT_TRANSFER_KEEPON;
						}
					}
					break;

				case STBK_OK:
					{
						if(app_tp_search_para_get() ==  GXCORE_SUCCESS)
						{
							GUI_EndDialog(WND_TP_SEARCH_OPT);
							_app_search_wnd_destroy(WND_OK);
						}
						else
						{
							GUI_EndDialog(WND_TP_SEARCH_OPT);
							_app_search_wnd_destroy(WND_CANCLE);
						}
					}
					break;

				default:
					break;
			}
		default:
			break;
	}

	return ret;
}

static WndStatus app_tp_search_opt_dlg(void)
{
	GUI_CreateDialog(WND_TP_SEARCH_OPT);
	return WND_EXEC;
}

WndStatus app_search_opt_dlg(SearchType type,  SearchIdSet *id)
{
	WndStatus ret = WND_CANCLE;

    if((id == NULL) || (id->id_array == NULL))
		return ret ;

    src_sg_search_id_set.id_num = id->id_num ;
    src_sg_search_id_set.id_array = (uint32_t *)GxCore_Malloc(id->id_num * sizeof(uint32_t));
    if ( src_sg_search_id_set.id_array == NULL )
        return ret ;

    memcpy(src_sg_search_id_set.id_array, id->id_array,  id->id_num * sizeof(uint32_t));

    sg_search_id_set = &src_sg_search_id_set ; //id;
    s_sat_search_sat_total = id->id_num;
	switch(type)
	{
		case SEARCH_SAT:
			sg_search_type = SEARCH_SAT;
			ret = app_sat_search_opt_dlg();
			break;

		case SEARCH_BLIND_ONLY:
			sg_search_type = SEARCH_BLIND_ONLY;
			ret = app_sat_search_opt_dlg();
			break;

		case SEARCH_TP:
			sg_search_type = SEARCH_TP;
			ret = app_tp_search_opt_dlg();
			break;
		default:
			sg_search_type = SEARCH_NONE;
			break;
	}

	return ret;
}

#if FACTORY_TEST_SUPPORT
/*
function:
data :2012 11 1
*/
void app_factory_test_search_start(GxMsgProperty_ScanSatStart * PSearch_para)
{
    sg_search_type = SEARCH_TP;

    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);

    // add in 20121012
    sg_SatId = 0xffff;// for search moto control

    GUI_CreateDialog(WND_SEARCH);
    app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, PSearch_para);
}
#endif

static int app_init_search_reply_wnd(void)
{
    if(sg_search_id_set != NULL && sg_search_id_set->id_array != NULL)
    {
#define BUFFER_LENGTH_  200
        char Buffer[BUFFER_LENGTH_] = {0};
        if(sg_search_type == SEARCH_TP)
        {
            uint32_t first_selected_tp_id = 0;
            first_selected_tp_id = sg_search_id_set->id_array[0];
            GxMsgProperty_NodeByIdGet node_tp = {0};
            node_tp.node_type = NODE_TP;
            node_tp.id = first_selected_tp_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

            GxMsgProperty_NodeByIdGet node_sat;
            node_sat.node_type = NODE_SAT;
            node_sat.id = node_tp.tp_data.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

            memset(Buffer, 0, BUFFER_LENGTH_);
            sprintf((char*)Buffer, "(%d / %d)  %s", 1, 1, node_sat.sat_data.sat_s.sat_name);
            GUI_SetProperty("text_search_sat", "string", Buffer);
            memset(Buffer, 0, BUFFER_LENGTH_);
            if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
            {
                sprintf((char*)Buffer,"(%d / %d)  %d / %s / %d", 1, sg_search_id_set->id_num, node_tp.tp_data.frequency, "V", node_tp.tp_data.tp_s.symbol_rate);
            }
            else if(node_tp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
            {
                sprintf((char*)Buffer,"(%d / %d)  %d / %s / %d", 1, sg_search_id_set->id_num, node_tp.tp_data.frequency, "H", node_tp.tp_data.tp_s.symbol_rate);
            }
            GUI_SetProperty("text_scan_tp_info", "state", "hide");
            GUI_SetProperty("text_scan_tp_value", "state", "hide");
            GUI_SetProperty("text_search_tp", "string", Buffer);
            GUI_SetInterface("flush", NULL);
        }
        else if(sg_search_type == SEARCH_SAT || sg_search_type == SEARCH_BLIND_ONLY)
        {
            uint32_t first_selected_sat_id = sg_search_id_set->id_array[0];
            GxMsgProperty_NodeByPosGet firstTp = {0};
            GxMsgProperty_NodeNumGet node_num = {0};

            GxMsgProperty_NodeByIdGet node_sat;
            node_sat.node_type = NODE_SAT;
            node_sat.id = first_selected_sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

            memset(Buffer, 0, BUFFER_LENGTH_);
            sprintf((char*)Buffer, "(%d / %d)  %s", 1, sg_search_id_set->id_num, node_sat.sat_data.sat_s.sat_name);
            GUI_SetProperty("text_search_sat", "string", Buffer);

            node_num.node_type = NODE_TP;
            node_num.sat_id = first_selected_sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
            if((node_num.node_num > 0) && (sg_search_type == SEARCH_SAT))
            {
                memset(Buffer, 0, BUFFER_LENGTH_);
                firstTp.node_type = NODE_TP;
                firstTp.pos = 0;
                app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &firstTp);
                if(firstTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_V)
                {
                    sprintf(Buffer,"(%d / %d)  %d / %s / %d",1, node_num.node_num, firstTp.tp_data.frequency, "V" ,firstTp.tp_data.tp_s.symbol_rate);
                }
                else if(firstTp.tp_data.tp_s.polar == GXBUS_PM_TP_POLAR_H)
                {
                    sprintf(Buffer,"(%d / %d)  %d / %s / %d",1, node_num.node_num, firstTp.tp_data.frequency, "H" ,firstTp.tp_data.tp_s.symbol_rate);
                }
                GUI_SetProperty("text_scan_tp_info", "state", "hide");
                GUI_SetProperty("text_scan_tp_value", "state", "hide");
                GUI_SetProperty("text_search_tp", "string", Buffer);
                GUI_SetInterface("flush", NULL);
            }
        }
        else
        {}
    }

    return 0;
}

void app_search_start(void)
{
	if(SEARCH_BLIND_ONLY == sg_search_type)
	{
		unsigned int value = 0;
		app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_STRENGTH_GET, &value);
		if(value == 0)
		{
			AppFrontend_SetTp params = {0};
			params.fre = 1150;
			params.symb = 27500;
			params.type = FRONTEND_DVB_S;
			app_set_tp(app_tuner_cur_tuner_get(), &params, SET_TP_FORCE);
		}
	}

    if(GxFrontend_HasSpecialFeatures(app_tuner_cur_tuner_get()))
    {
#if (DEMOD_DVB_S > 0)
        if(true == app_get_super_search_flag())
            GxFrontend_PlsNSourceSelect(app_tuner_cur_tuner_get(), FE_PLSN_FROM_AUTO_CALC);
        else
#endif
            GxFrontend_PlsNSourceSelect(app_tuner_cur_tuner_get(), FE_PLSN_FROM_MANUAL_SET);
    }

    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);

    g_AppTime.stop(&g_AppTime);//for bug 142508
    GxFrontend_CleanRecordPara(app_tuner_cur_tuner_get());
// pdmx
#if PDMX_SUPPORT
    app_pdmx_stop();
#endif


    // add in 20121012
    sg_SatId = 0xffff;// for search moto control

    switch(sg_search_type)
    {
        case SEARCH_SAT:
            if(sg_sat_search_mode == SAT_SEARCH_AUTO)
            {
                if(s_Search.nit_switch == GX_SEARCH_NIT_ENABLE)
                {
                    GxBus_ConfigSetInt(SEARCH_NIT_LOOP_MODE, APP_SEARCH_NIT_LOOP_NO);//20140322
                }
                GUI_CreateDialog(WND_SEARCH);
                app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, &s_Search);
                app_init_search_reply_wnd();
            }
            else
            {
                GUI_CreateDialog(WND_SEARCH);
                app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_START, &s_Blind);
                app_init_search_reply_wnd();
            }
            break;

        case SEARCH_BLIND_ONLY:
            {
                GUI_CreateDialog(WND_SEARCH);
                app_send_msg_exec(GXMSG_SEARCH_BLIND_SCAN_START, &s_Blind);
                app_init_search_reply_wnd();
                break;
            }

        case SEARCH_TP:
            if(s_Search.nit_switch == GX_SEARCH_NIT_ENABLE)
            {
            	if(s_Search.params_s.max_num == 1)
            	{
                	GxBus_ConfigSetInt(SEARCH_NIT_LOOP_MODE, APP_SEARCH_NIT_LOOP_YES);//20140322
            	}
				else
				{
					GxBus_ConfigSetInt(SEARCH_NIT_LOOP_MODE, APP_SEARCH_NIT_LOOP_NO);
				}
            }
            GUI_CreateDialog(WND_SEARCH);
            app_send_msg_exec(GXMSG_SEARCH_SCAN_SAT_START, &s_Search);
            app_init_search_reply_wnd();
            break;

		default:
			break;
	}
}

#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)

search_result searchresultpara = {{0}};
search_fre_list searchFreList = {{0}};

int app_search_check_fre_valid(uint32_t fre, unsigned char demod_type)
{
    char buf[100] = {0};
    PopDlg pop = {0};
    uint32_t min_fre = 0;
    uint32_t max_fre = 0;
    char *i18n_str = NULL;
#if DEMOD_DVB_C
    if(demod_type == DEMOD_TYPE_DVBC)
    {
        min_fre = DVBC_FRE_BEGIN_LOW;
        max_fre = DVBC_FRE_BEGIN_HIGH;
    }
#endif
#if DEMOD_DTMB
    if(demod_type == DEMOD_TYPE_DTMB)
    {
        min_fre = DTMB_FRE_BEGIN_LOW;
        max_fre = DTMB_FRE_BEGIN_HIGH;
    }
#endif
    if (fre < min_fre)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_FREQ_LESS_THAN);
        memset(buf, 0, 100);
        sprintf((void *)buf, "%s %.1fMHz", i18n_str,(float)(min_fre) / 1000);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    if (fre > max_fre)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_FREQ_LARGER_THAN);
        memset(buf, 0, 100);
        sprintf((void *)buf, "%s %.1fMHz", i18n_str,(float)(max_fre) / 1000);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    return TRUE;
}

int app_search_check_sym_valid(uint32_t sym)
{
    char buf[100] = {0};
    PopDlg pop = {0};
    char *i18n_str = NULL;

    if (sym < SYMBOL_BEGIN_LOW)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_SYM_LESS_THAN);
        memset(buf, 0, 100);
        sprintf((void *)buf, "%s %03d",i18n_str, SYMBOL_BEGIN_LOW);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }

    if (sym > SYMBOL_BEGIN_HIGH)
    {
        i18n_str =(char*)GXGDI_I18N_String(STR_ID_SYM_LARGER_THAN);
        memset(buf, 0, 100);
        sprintf((void *)buf, "%s %03d",i18n_str, SYMBOL_BEGIN_HIGH);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    return TRUE;
}

int app_search_check_fre_range_valid(uint32_t lowfre,uint32_t highfre,unsigned char demod_type)
{
    char buf[100] = {0};
    PopDlg pop;

    if(FALSE == app_search_check_fre_valid(lowfre,demod_type))
        return FALSE;

    if(FALSE == app_search_check_fre_valid(highfre,demod_type))
        return FALSE;

    if((lowfre + 8000 > highfre)&&((lowfre != 467000)||(highfre != 474000)))
    {
        memset(buf,0,100);
        sprintf((void*)buf,"%s", STR_ID_FRE_ERROR);
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = buf;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    return TRUE;
}

int app_search_dvbc_all_search_fre_info_get(uint32_t begin_fre, uint32_t end_fre, uint32_t symbol_rate, uint32_t qam)
{
    uint16_t i;
    uint32_t fre;
    uint16_t freArrayCount =0;
    uint32_t end_fre_tmp = end_fre;
    PopDlg pop;

    memset(&searchFreList, 0, sizeof(search_fre_list));

    if(begin_fre <= 115000)
        begin_fre = 115000;
    else if(begin_fre <= 467000)
        begin_fre = (467000 - (abs(begin_fre - 467000)/8000)*8000);
    else if(begin_fre < 474000)
        begin_fre = 474000;
    else
        begin_fre = (858000 - (abs(begin_fre - 858000)/8000)*8000);

    if(end_fre <= 467000)
    {
        end_fre = (467000 - (abs(end_fre - 467000)/8000)*8000);

        if(end_fre > end_fre_tmp)
            end_fre -= 8000;
    }
    else if(end_fre < 474000)
    {
        end_fre = 467000;
    }
    else if(end_fre <= 866000 )
    {
        end_fre = (866000 - (abs(end_fre - 866000)/8000)*8000);
        if(end_fre > end_fre_tmp)
            end_fre -= 8000;
    }
    else
    {
        end_fre = 866000;
    }

    freArrayCount = 0;
    fre = begin_fre;

    for(i=0; i < TP_MAX_NUM; i++)
    {
        if(fre == 475000)
            fre = 474000;

        if(fre > end_fre)
            break;

        searchFreList.app_fre_array[i] = fre;
        searchFreList.app_qam_array[i] = qam;
        searchFreList.app_symb_array[i] = symbol_rate;
        freArrayCount++;

        fre = fre + 8000;
    }

    app_log_info("====DVBC All Search Freq Info,total:%d=====\n",freArrayCount);
    if(freArrayCount == 0)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_FRE_ERROR;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    else
    {
        searchFreList.num = freArrayCount;
        searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
        return TRUE;
    }
}

int app_search_dtmb_all_search_fre_info_get(uint32_t begin_fre, uint32_t end_fre, uint32_t symbol_rate, uint32_t qam)
{
    uint16_t i;
    uint32_t fre;
    uint16_t freArrayCount =0;
    uint32_t end_fre_tmp = end_fre;
    PopDlg pop;

    memset(&searchFreList, 0, sizeof(search_fre_list));
    if(begin_fre <= 52500)
        begin_fre = 52500;
    else if(begin_fre <= 60500)
        begin_fre = 60500;
    else if(begin_fre <= 68500)
        begin_fre = 68500;
    else if(begin_fre <= 80000)
        begin_fre = 80000;
    else if(begin_fre <= 88000)
        begin_fre = 88000;
    else if (begin_fre > 88000 && begin_fre < 171000)
        begin_fre = 171000;
    else if ((begin_fre >= 171000) && (begin_fre <= 219000))
        begin_fre = (219000 - (abs(begin_fre - 219000)/8000)*8000);
    else if(begin_fre < 474000)
        begin_fre = 474000;
    else
        begin_fre = (858000 - (abs(begin_fre - 858000)/8000)*8000);

    if(end_fre <= 52500)
    {
        end_fre = 52500;
    }
    else if (end_fre > 52500 && end_fre < 60500)
    {
        end_fre = 52500;
    }
    else if (end_fre >= 60500 && end_fre < 68500)
    {
        end_fre = 60500;
    }
    else if (end_fre >= 68500 && end_fre < 80000)
    {
        end_fre = 68500;
    }
    else if (end_fre >= 80000 && end_fre < 88000)
    {
        end_fre = 80000;
    }
    else if (end_fre >= 88000 && end_fre < 171000)
    {
        end_fre = 88000;
    }
    else if ((end_fre >= 171000) && (end_fre <= 219000))
    {
        end_fre = (219000 - (abs(end_fre - 219000)/8000)*8000);
    }
    else if(end_fre < 474000)
    {
        end_fre = 219000;
    }
    else if(end_fre <= 866000)
    {
        end_fre = (866000 - (abs(end_fre - 866000)/8000)*8000);
        if(end_fre > end_fre_tmp)
            end_fre -= 8000;
    }
    else
    {
        end_fre = 866000;
    }

    freArrayCount = 0;
    fre = begin_fre;

    for(i = 0; i<TP_MAX_NUM; i++)
    {
        if(fre == 76500)
            fre = 80000;
        else if(fre == 96000)
            fre = 171000;
        else if(fre == 227000)
            fre = 474000;
        else if(fre == 570000)
            fre = 610000;

        if(fre > end_fre)
            break;

        searchFreList.app_fre_array[i] = fre;
        searchFreList.app_qam_array[i] = qam;
        searchFreList.app_symb_array[i] = symbol_rate;
        freArrayCount++;

        fre = fre + 8000;
    }

    app_log_info("====DTMB All Search Freq Info,total:%d=====\n",freArrayCount);
    if(freArrayCount == 0)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_FRE_ERROR;
        pop.create_cb = NULL;
        pop.exit_cb = NULL;
        popdlg_create(&pop);
        return FALSE;
    }
    else
    {
        searchFreList.num = freArrayCount;
        searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
        return TRUE;
    }
}

void app_search_scan_cable_dtmb_start(search_fre_list searchlist, GxBusPmDataSatType type)
{
	GxBusPmDataSat sat_arry = {0};
	GxBusPmDataTP tp = {0};
	uint16_t i = 0;
	uint16_t tpid_temp = 0;
	int32_t ret = 0;
    int32_t bandwidth = 0;
	static uint32_t ts_id = 0;
	AppFrontend_Config cfg = {0};
    GxMsgProperty_ScanCableStart dvbc_scan = {0};
    GxMsgProperty_ScanDtmbStart dtmb_scan = {0};

    if((type != GXBUS_PM_SAT_C) && (type != GXBUS_PM_SAT_DTMB))
        return;

#if DEMOD_DVB_C
    if(GXBUS_PM_SAT_C == type)
        app_get_sat_params(&sat_arry, DEMOD_TYPE_DVBC);
#endif
#if DEMOD_DTMB
    if(GXBUS_PM_SAT_DTMB == type)
        app_get_sat_params(&sat_arry, DEMOD_TYPE_DTMB);
#endif

	app_ioctl(sat_arry.tuner, FRONTEND_CONFIG_GET, &cfg);
	memset(&sat_arry, 0, sizeof(GxBusPmDataSat));
	ts_id = cfg.ts_src;

    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, true);

    if(GXBUS_PM_SAT_C == type)
    {
        memset(&dvbc_scan,0,sizeof(GxMsgProperty_ScanCableStart));
    }
    else if(GXBUS_PM_SAT_DTMB == type)
    {
        memset(&dtmb_scan,0,sizeof(GxMsgProperty_ScanDtmbStart));
        GxBus_ConfigGetInt(MAIN_FREQ_BANDWIDTH_KEY, &bandwidth, MAIN_FREQ_BANDWIDTH_VALUE);
    }

	memset(&searchresultpara,0,sizeof(search_result));

	sat_arry.id = app_sat_id_get_by_type(type);
	for(i = 0; i < searchlist.num; i++)
	{
		uint32_t symbol = searchlist.app_symb_array[i];
#if (DVBC_TUNE_AUTO_SUPPORT > 0)
        symbol = 0;
#endif
		ret = GxBus_PmTpExistChek(sat_arry.id, searchlist.app_fre_array[i] , symbol , 0, searchlist.app_qam_array[i]+3, &tpid_temp);
		if(ret == 0)
		{
			tp.sat_id = sat_arry.id;
			tp.frequency = searchlist.app_fre_array[i] ;
            if(GXBUS_PM_SAT_C == type)
            {
                tp.tp_c.modulation  = searchlist.app_qam_array[i]+3;
                tp.tp_c.symbol_rate = searchlist.app_symb_array[i];
            }
            else if(GXBUS_PM_SAT_DTMB == type)
            {
                tp.tp_dtmb.bandwidth = bandwidth;
            }
			ret = GxBus_PmTpAdd(&tp);
			if (GXCORE_SUCCESS != ret)
			{
				app_log_info("can't add Tp\n");
				break;
			}
			searchresultpara.app_tpid[i] = tp.id;
		}
		else if(ret > 0)
		{
            if(GXCORE_SUCCESS == GxBus_PmTpGetById(tpid_temp, &tp))
            {
                if(GXBUS_PM_SAT_C == type && searchlist.app_symb_array[i] != tp.tp_c.symbol_rate)
                {
                    tp.tp_c.symbol_rate = searchlist.app_symb_array[i];
                    GxBus_PmTpModify(&tp);
                }

                if(GXBUS_PM_SAT_DTMB == type && bandwidth != tp.tp_dtmb.bandwidth)
                {
                    tp.tp_dtmb.symbol_rate = searchlist.app_symb_array[i];
                    tp.tp_dtmb.bandwidth = bandwidth;
                    GxBus_PmTpModify(&tp);
                }
                searchresultpara.app_tpid[i] = tpid_temp;
            }
		}
		else
		{
			app_log_info("can't add Tp\n");
			return;
		}
	}
	GxBus_PmSync(GXBUS_PM_SYNC_TP);

	if(0 == i)
		return;
	searchresultpara.tp_num = i;

#if LCN_SUPPORT
	g_SearchExtendList.extendnum = 0;

	if(NULL != app_search_add_extend_callback)
		app_search_add_extend_callback(&g_SearchExtendList);

	if (g_SearchExtendList.extendnum > 0)
	{
		memset(s_SearchExtParse,0,SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
        if(GXBUS_PM_SAT_C == type)
        {
            dvbc_scan.ext_num = g_SearchExtendList.extendnum;
            memcpy(&s_SearchExtParse[0],&g_SearchExtendList.searchExtend[0],SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
            dvbc_scan.ext = &s_SearchExtParse[0];
        }
	    else if(GXBUS_PM_SAT_DTMB == type)
        {
            dtmb_scan.ext_num = g_SearchExtendList.extendnum;
            memcpy(&s_SearchExtParse[0],&g_SearchExtendList.searchExtend[0],SEARCH_EXTEND_MAX*sizeof(GxSearchExtend));
            dtmb_scan.ext = &s_SearchExtParse[0];
        }
	}

	app_lcn_list_init(&sat_arry.id, 1);
#endif

    if(GXBUS_PM_SAT_C == type)
    {
        int32_t fta_cas_mode;
        GxBus_ConfigGetInt(SEARCH_FTA_CAS_KEY, &fta_cas_mode, SEARCH_FTA_CAS_VALUE);

        dvbc_scan.scan_type = GX_SEARCH_MANUAL;
        dvbc_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
        dvbc_scan.fta_cas = fta_cas_mode;
        dvbc_scan.nit_switch = GX_SEARCH_NIT_DISABLE;

        dvbc_scan.params_c.sat_id = sat_arry.id;
        dvbc_scan.params_c.max_num = i;//searchlist.num;
        dvbc_scan.params_c.array = searchresultpara.app_tpid;
        dvbc_scan.params_c.ts = &ts_id;
        dvbc_scan.time_out.pat = TIMEOUT_T_PAT;
        dvbc_scan.time_out.sdt = TIMEOUT_T_SDT;
        dvbc_scan.time_out.nit = TIMEOUT_T_NIT;
        dvbc_scan.time_out.pmt = TIMEOUT_T_PMT;
#if FONT_GB2312_TO_UTF8
        dvbc_scan.modify_prog = app_search_modify_prog_cb;
#else
        dvbc_scan.modify_prog = _modify_prog_callback;
#endif
        dvbc_scan.tune_mode = DVBC_TUNE_MODE;

        app_send_msg_exec(GXMSG_SEARCH_SCAN_CABLE_START,(void*)&dvbc_scan);
    }
    else if(GXBUS_PM_SAT_DTMB == type)
    {
        dtmb_scan.scan_type = GX_SEARCH_MANUAL;
        dtmb_scan.tv_radio = GX_SEARCH_TV_RADIO_ALL;
        dtmb_scan.fta_cas = GX_SEARCH_FTA_CAS_ALL;
        dtmb_scan.nit_switch = GX_SEARCH_NIT_DISABLE;

        dtmb_scan.params_dtmb.sat_id = sat_arry.id;
        dtmb_scan.params_dtmb.max_num = i;//searchlist.num;
        dtmb_scan.params_dtmb.array = searchresultpara.app_tpid;
        dtmb_scan.params_dtmb.ts = &ts_id;
        dtmb_scan.time_out.pat = TIMEOUT_T_PAT;
        dtmb_scan.time_out.sdt = TIMEOUT_T_SDT;
        dtmb_scan.time_out.nit = TIMEOUT_T_NIT;
        dtmb_scan.time_out.pmt = TIMEOUT_T_PMT;
#if FONT_GB2312_TO_UTF8
        dtmb_scan.modify_prog = app_search_modify_prog_cb;
#else
        dtmb_scan.modify_prog = _modify_prog_callback;
#endif
        app_send_msg_exec(GXMSG_SEARCH_SCAN_DTMB_START, (void *)&dtmb_scan);
    }

}

void app_search_scan_manual_mode(uint32_t fre, uint32_t symbol_rate, uint32_t qam)
{
	memset(&searchFreList,0,sizeof(search_fre_list));
	searchFreList.app_fre_array[0] = fre;
	searchFreList.app_qam_array[0] = qam;
	searchFreList.app_symb_array[0] = symbol_rate;
	searchFreList.num = 1;
	searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;

    if(GXCORE_INVALID_POINTER != thiz_search_mutex)
        GxCore_MutexLock(thiz_search_mutex);

	app_table_nit_search_filter_close();

#if LCN_SUPPORT
	app_lcn_list_destroy();
#endif

    if(GXCORE_INVALID_POINTER != thiz_search_mutex)
        GxCore_MutexUnlock(thiz_search_mutex);

    if(SEARCH_CABLE == sg_search_type)
        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_C);
    else if(SEARCH_DTMB == sg_search_type)
        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_DTMB);

	return;
}

void app_search_scan_nit_mode(void)
{
	int32_t symbol_rate = 0;
	int32_t qam = 0;
	int32_t fre = 0;

	GxBus_ConfigGetInt(MAIN_FREQ_KEY, &fre, MAIN_FREQ_VALUE);
	GxBus_ConfigGetInt(MAIN_FREQ_SYM_KEY, &symbol_rate, MAIN_FREQ_SYM_VALUE);
	GxBus_ConfigGetInt(MAIN_FREQ_QAM_KEY, &qam, MAIN_FREQ_QAM_VALUE);

    if(GXCORE_INVALID_POINTER != thiz_search_mutex)
        GxCore_MutexLock(thiz_search_mutex);

	app_table_nit_search_filter_close();

	memset(&searchFreList,0,sizeof(search_fre_list));
	searchFreList.num = 1;
	searchFreList.app_fre_array[0] = fre;
	searchFreList.app_qam_array[0]=qam;
	searchFreList.app_symb_array[0]=symbol_rate;

#if BOUQUET_SUPPORT
	app_bouquet_clear_all();
#endif
#if LCN_SUPPORT
	app_lcn_list_destroy();
    s_SearchNitMode = true;
#endif

	app_table_nit_search_filter_open();

    if(GXCORE_INVALID_POINTER != thiz_search_mutex)
        GxCore_MutexUnlock(thiz_search_mutex);

	return ;
}

void app_search_scan_all_mode()
{
#if LCN_SUPPORT
	app_lcn_list_destroy();
#endif
    if(SEARCH_CABLE == sg_search_type)
        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_C);
    else if(SEARCH_DTMB == sg_search_type)
        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_DTMB);
}
void app_search_scan_preset_mode(void)
{
    int i =0;
    uint32_t freqlist[] =
    {
         52500,  62000, 80000,  88000, 96000,
        114000, 122000, 130000, 138000, 146000, 154000, 162000,170000,
        178000, 186000, 194000, 202000, 210000, 218000, 226000,
        234000, 242000, 250000, 258000, 266000, 274000, 282000, 290000, 298000, 306000, 314000,322000,330000,
        338000, 346000, 354000, 362000, 370000, 378000, 386000, 394000, 402000, 410000,
        418000, 426000, 434000, 442000, 450000, 458000, 466000, 474000, 482000, 490000,
        498000, 506000, 522000, 530000, 538000, 546000, 554000, 562000, 570000, 578000,
        586000, 594000, 602000, 610000, 618000, 626000, 634000, 642000, 650000, 658000,
        666000, 674000, 682000, 690000, 698000, 706000, 714000, 722000, 730000, 738000,
        746000, 754000, 762000, 770000, 778000, 786000, 794000, 802000, 810000, 818000,
        826000, 834000, 842000, 850000, 858000
    };
#if LCN_SUPPORT
	app_lcn_list_destroy();
#endif

    memset(&searchFreList, 0, sizeof(search_fre_list));
    searchFreList.num = sizeof(freqlist)/sizeof(freqlist[0]);
    for(i = 0;i<searchFreList.num ;i++)
    {
        searchFreList.app_fre_array[i]=freqlist[i];
        searchFreList.app_symb_array[i]= MAIN_FREQ_SYM_VALUE;
        searchFreList.app_qam_array[i]= MAIN_FREQ_QAM_VALUE;
    }
    searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;

    app_table_nit_search_filter_close();

    app_search_scan_cable_dtmb_start(searchFreList,GXBUS_PM_SAT_C);

    return ;
}

void app_search_si_subtable_msg(GxMessage * msg)
{
    GxMsgProperty_SiSubtableOk *parse_result = NULL;
    int32_t NitSubtId=0;
    uint32_t NitRequestId=0;
    if(NULL == msg)
        return;

    switch(msg->msg_id)
    {
        case GXMSG_SI_SUBTABLE_OK:
            if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                GxCore_MutexLock(thiz_search_mutex);

            app_table_nit_get_search_filter_info(&NitSubtId,&NitRequestId);
            parse_result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_SiSubtableOk);
            if((NitSubtId == parse_result->si_subtable_id) && (NitRequestId == parse_result->request_id))
            {
                app_table_nit_search_filter_close();
                if(parse_result->table_id == 0x40)
                {
                    searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
                    if(SEARCH_CABLE == sg_search_type)
                        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_C);
                    else if(SEARCH_DTMB == sg_search_type)
                        app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_DTMB);
                }
                else
                {
                    app_log_debug("\n####error####service_msg_to_app:%d#######\n",parse_result->table_id );
                }

            }

            if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                GxCore_MutexUnlock(thiz_search_mutex);
            return;
        case GXMSG_SI_SUBTABLE_TIME_OUT:
            if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                GxCore_MutexLock(thiz_search_mutex);

            app_table_nit_get_search_filter_info(&NitSubtId,&NitRequestId);
            parse_result = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_SiSubtableOk);
            if((NitSubtId == parse_result->si_subtable_id) && (NitRequestId == parse_result->request_id))
            {

                app_log_debug("filter nit table timeout  fre = %d!!\n",searchFreList.app_fre_array[0]);
                app_table_nit_search_filter_close();
                searchFreList.nit_flag = GX_SEARCH_NIT_DISABLE;
                if(SEARCH_CABLE == sg_search_type)
                    app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_C);
                else if(SEARCH_DTMB == sg_search_type)
                    app_search_scan_cable_dtmb_start(searchFreList, GXBUS_PM_SAT_DTMB);
            }

            if(GXCORE_INVALID_POINTER != thiz_search_mutex)
                GxCore_MutexUnlock(thiz_search_mutex);
            return ;
        default:
            break;
    }

    return ;
}

#endif

#if WEBAPI_SUPPORT

#if DEMOD_DVB_S
static void app_webapi_tp_search_param_set(void)
{
	uint32_t i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	sg_sat_search_mode = SAT_SEARCH_AUTO;
	if(sg_search_id_set->id_num)
	{
		GxMsgProperty_NodeByIdGet node = {0};
		AppFrontend_Config cfg = {0};

		node.node_type = NODE_TP;
		node.id = sp_scan_id[0];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		node.node_type = NODE_SAT;
		node.id = node.tp_data.sat_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

		app_ioctl(node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
		for(i = 0; i < sg_search_id_set->id_num; i++)
			sp_ts_src[i] = cfg.ts_src;
	}

	s_Search.params_s.array = sp_scan_id;
	s_Search.params_s.ts = sp_ts_src;
	s_Search.params_s.max_num = sg_search_id_set->id_num;

	// needn't ext
	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
	s_Search.modify_prog = _modify_prog_callback;
	s_Search.pmt_parse_fun = _pmt_private_function;
	s_Search.check_ca_fun = _check_ca_system;
}

static void app_webapi_satellite_search_param_set(void)
{
	GxMsgProperty_NodeByIdGet sat_node = {0};
	AppFrontend_Config cfg = {0};
	uint32_t i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	thizDraAudioId = -1;
	s_Search.scan_type = GX_SEARCH_AUTO;

	for (i=0; i<sg_search_id_set->id_num; i++)
	{
		sat_node.node_type = NODE_SAT;
		sat_node.id = sp_scan_id[i];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node);

		app_ioctl(sat_node.sat_data.tuner, FRONTEND_CONFIG_GET, &cfg);
		sp_ts_src[i] = cfg.ts_src;
	}

	s_Search.params_s.array = sp_scan_id;
	s_Search.params_s.ts = sp_ts_src;
	s_Search.demux_id = 0;
	s_Search.params_s.max_num = sg_search_id_set->id_num;

	s_Search.ext = NULL;
	s_Search.ext_num = 0;
	s_Search.time_out.pat = TIMEOUT_PAT;
	s_Search.time_out.pmt = TIMEOUT_PMT;
	s_Search.time_out.sdt = TIMEOUT_SDT;
	s_Search.time_out.nit = TIMEOUT_NIT;
	s_Search.search_diseqc = _set_disqec_callback;
	s_Search.modify_prog = _modify_prog_callback;
	s_Search.pmt_parse_fun = _pmt_private_function;

	s_Search.check_ca_fun = _check_ca_system;

	if(sg_search_type == SEARCH_BLIND_ONLY)
	{
		sg_sat_search_mode = SAT_SEARCH_BLINDE;

		s_Blind.max_num = sg_search_id_set->id_num;
		s_Blind.array = sp_scan_id;
		s_Blind.polar_type = BLIND_POLAR_ALL;
		s_Blind.search_type = BLIND_FAST;
#if (DEMOD_DVB_S > 0)
        if(true == app_get_super_search_flag())
            s_Blind.search_type = BLIND_SUPER;
#endif

		// add 20120618
		s_Blind.fta_cas		= s_Search.fta_cas;
		s_Blind.tv_radio	= s_Search.tv_radio;
		s_Blind.nit_switch  = s_Search.nit_switch;

		s_Blind.search_diseqc = _set_disqec_callback;
		s_Blind.modify_prog =  _modify_prog_callback;
		s_Blind.check_ca_fun = _check_ca_system;

		s_Blind.ts = sp_ts_src;
		memset(&(s_Blind.params_pid), 0 , sizeof(GxPidSearch));
		s_Blind.ext = NULL;
		s_Blind.ext_num = 0;
		s_Blind.time_out.pat = TIMEOUT_PAT;
		s_Blind.time_out.pmt = TIMEOUT_PMT;
		s_Blind.time_out.sdt = TIMEOUT_SDT;
		s_Blind.time_out.nit = TIMEOUT_NIT;
	}
	else if(sg_search_type == SEARCH_SAT)
	{
		sg_sat_search_mode = SAT_SEARCH_AUTO;
		ex_sat_search_mode = SAT_SEARCH_AUTO;
	}
}

webapi_ret_t app_webapi_dvbs_search_start(
					WebapiSearchType search_type,
					cJSON *id,
					WebapiProgType prog_type,
					int fta,
					int nit)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	SearchIdSet search_id;
	cJSON *json = NULL;
	uint32_t temp_id = 0;
	int id_num = 0;
	int i = 0;
	int exist = 0;
	GxMsgProperty_NodeByIdGet node_sat = {0};
	GxMsgProperty_NodeByIdGet node_tp = {0};
	GxMsgProperty_NodeNumGet node_num = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(&search_id, 0, sizeof(SearchIdSet));
	id_num = cJSON_GetArraySize(id);
	if(id_num>0)
	{
		search_id.id_num = 0;
		search_id.id_array = (uint32_t *)GxCore_Malloc(search_id.id_num*sizeof(uint32_t));
		for(i=0; i<id_num; i++)
		{
			json = cJSON_GetArrayItem(id, i);
			if(json && (json->type == cJSON_String))
			{
				WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, json->valuestring);
				temp_id = (uint32_t)strtoul(json->valuestring, 0, 10);
				if(temp_id>0)
				{
					exist = 0;
					if(search_type == WEBAPI_SEARCH_TP)
					{
						node_tp.node_type = NODE_TP;
						node_tp.id = temp_id;
						if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp))
						{
							exist = 1;
						}
					}
					else if(search_type == WEBAPI_SEARCH_PRESET)
					{
						node_sat.node_type = NODE_SAT;
						node_sat.id = temp_id;
						if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat))
						{
							memset(&node_num, 0, sizeof(GxMsgProperty_NodeNumGet));
							node_num.node_type = NODE_TP;
							node_num.sat_id = temp_id;
							app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
							if(node_num.node_num > 0)
							{
								exist = 1;
							}
						}
					}
					else if(search_type == WEBAPI_SEARCH_BLIND)
					{
						node_sat.node_type = NODE_SAT;
						node_sat.id = temp_id;
						if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat))
						{
							exist = 1;
						}
					}
					if(exist>0)
					{
						search_id.id_array[search_id.id_num] = temp_id;
						search_id.id_num++;
					}
				}

			}
		}
	}

	if(search_id.id_num>0)
	{
		sg_search_id_set = &search_id;
		s_sat_search_sat_total = search_id.id_num;

		if(prog_type == WEBAPI_PROG_TV)
		{
			s_Search.tv_radio = GX_SEARCH_TV;
		}
		else if(prog_type == WEBAPI_PROG_RADIO)
		{
			s_Search.tv_radio = GX_SEARCH_RADIO;
		}
		else
		{
			s_Search.tv_radio = GX_SEARCH_TV_RADIO_ALL;
		}

		if(fta == 0)
		{
			s_Search.fta_cas = GX_SEARCH_FTA_CAS_ALL;
		}
		else
		{
			s_Search.fta_cas = GX_SEARCH_FTA;
		}

		if(nit == 0)
		{
			s_Search.nit_switch = GX_SEARCH_NIT_DISABLE;
		}
		else
		{
			s_Search.nit_switch = GX_SEARCH_NIT_ENABLE;
		}

		s_Search.scan_type = GX_SEARCH_MANUAL;
		APP_FREE(sp_scan_id);
		APP_FREE(sp_ts_src);
		sp_scan_id = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
		sp_ts_src = (uint32_t*)GxCore_Calloc(1, sg_search_id_set->id_num * sizeof(uint32_t));
		if(sp_scan_id&&sp_ts_src)
		{
			memcpy(sp_scan_id, sg_search_id_set->id_array,  sg_search_id_set->id_num * sizeof(uint32_t));
			if(search_type == WEBAPI_SEARCH_TP)
			{
				sg_search_type = SEARCH_TP;
				app_webapi_tp_search_param_set();

			}
			else if(search_type == WEBAPI_SEARCH_PRESET)
			{
				sg_search_type = SEARCH_SAT;
				app_webapi_satellite_search_param_set();
			}
			else if(search_type == WEBAPI_SEARCH_BLIND)
			{
				sg_search_type = SEARCH_BLIND_ONLY;
				app_webapi_satellite_search_param_set();
			}
			s_search_auto_save = 1;
            if(GXCORE_ERROR == GUI_CheckDialog(WND_MAIN_MENU))
                g_AppPlayOps.program_stop();

			app_search_start();
			ret = WEBAPI_SUCCESS;
		}
		else
		{
			APP_FREE(sp_scan_id);
			APP_FREE(sp_ts_src);
		}
	}
	APP_FREE(search_id.id_array);

	return ret;
}

webapi_ret_t app_webapi_dvbs_search_stop(void)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	const char *wnd = NULL;
	GUI_Event event;

	wnd = GUI_GetFocusWindow();
	if(wnd&&(0 == strcasecmp(WND_SEARCH, wnd)))
	{
		s_search_auto_save = 2;//abort save
		WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
		event.type = GUI_KEYDOWN;
		event.key.sym = STBK_EXIT;
		GUI_SendEvent(wnd, &event);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

char *app_webapi_dvbs_get_search_data(void)
{
	int i = 0;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_SEARCH_STR, json1);
	if(s_search_state>0)
	{
		cJSON_AddStringToObject(json1, WEBAPI_STATE_STR, "running");
		cJSON_AddNumberToObject(json1, WEBAPI_PROGRESS_STR, s_search_progress);
		cJSON_AddNumberToObject(json1, WEBAPI_PROGNUM_STR, s_search_prog_total);
		json2 = cJSON_CreateArray();
		cJSON_AddItemToObject(json1, WEBAPI_PROGRAM_STR, json2);
		gxapps_mutex_lock();
		if(s_search_prog_list&&(s_search_prog_list->item_num>0))
		{
			for(i=0; i<s_search_prog_list->item_num; i++)
			{
				json3 = cJSON_CreateObject();
				cJSON_AddItemToArray(json2, json3);
				cJSON_AddStringToObject(json3, WEBAPI_ID_STR, s_search_prog_list->items[i].key);
				cJSON_AddStringToObject(json3, WEBAPI_PROGTYPE_STR, s_search_prog_list->items[i].type);
				cJSON_AddStringToObject(json3, WEBAPI_NAME_STR, s_search_prog_list->items[i].value);
			}
		}
		gxapps_mutex_unlock();
	}
	else
	{
		cJSON_AddStringToObject(json1, WEBAPI_STATE_STR, "idle");
	}

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return out;
}

#endif
#endif
#endif
