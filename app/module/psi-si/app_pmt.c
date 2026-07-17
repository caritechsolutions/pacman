/*****************************************************************************
*             CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :   app_pmt.c
* Author    :   shenbin
* Project   :   GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION   Date              AUTHOR         Description
   0.0      2010.4.14              shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app_module.h"
#include "module/config/gxconfig.h"
#include "gxcore.h"
#define NEQ_PARSE_TIME_MS   (24*60*60*1000UL)
#define PMT_DATA_FIFO_NUM   (1)

#define STREAM_VIDEO_TYPE   (0)
#define STREAM_AUDIO_TYPE   (1)
#define STREAM_PRIVATE_TYPE (2)
#define STREAM_UNKNOWN_TYPE (3)

#define MPEG_1_VIDEO        (0x01)
#define MPEG_2_VIDEO        (0x02)
#define MPEG_4_VIDEO        (0x10)
#define MPEG_1_AUDIO        (0x03)
#define MPEG_2_AUDIO        (0x04)
#define H264                (0x1b)
#define H265                (0x24)
#define AAC_ADTS            (0x0f)
#define AAC_LATM            (0x11)
#define AVS                 (0x42)
#define PRIVATE_PES_STREAM  (0x06)
#define LPCM                (0x80)
#define AC3                 (0x81)
#define DTS                 (0x82)
#define DOLBY_TRUEHD        (0x83)
#define AC3_PLUS            (0x84)
#define DTS_HD              (0x85)
#define DTS_MA              (0x86)
#define AC3_PLUS_SEC        (0xa1)
#define DTS_HD_SEC          (0xa2)

#define AC3_DESCRIPTOR      (0x6A)
#define EAC3_DESCRIPTOR     (0x7A)

#define MAX_AUDIO_TRACK     (32)
#define MAX_CAS_COUNT       (16)
#define MAX_MUIVIDEO_COUNT  (6)
#define MAX_AUDIO_DES_COUNT (32)

//#define MULIFEED_PMT_ENABLE
#define MULIFEED_TODATA(data1,data2)  ((data1 & 0x1f)<< 8 ) |data2

static handle_t s_pmt_mutex = -1;

extern void app_subt_change(void);
extern void app_track_set(PmtInfo* audio);
/* Private functions ------------------------------------------------------ */

typedef struct app_stream_info
{
	uint8_t stream_type;
	uint8_t descrip_tag;
	int16_t elementary_PID;
	int16_t es_info_length;
}app_stream_info_t;

typedef struct video_info
{
	uint16_t video_id;
	int8_t   video_type;
}video_info_t;

typedef struct audio_info
{
	uint16_t ad_audio_id;
	int8_t   audio_type;
	int8_t   audio_lang_type;
	char     iso_lang[4];
}audio_info_t;


typedef struct pmt_stream_para
{
	uint16_t service_id;
	uint16_t pcr_pid;
	int16_t           ac3_count;
	int16_t           ca_count;
	int16_t           cas_id[MAX_CAS_COUNT];
	audio_info_t      audio_descriptor[MAX_AUDIO_DES_COUNT];
	app_stream_info_t descrip_info[MAX_AUDIO_TRACK];
	video_info_t      v_info[MAX_MUIVIDEO_COUNT];
	int8_t            video_count;
	int8_t            audio_count;
}pmt_info_stream_t;

static pmt_info_stream_t gs_PmtInfoPara;

int app_get_mult_audio_type(int mode, uint8_t *pRetIndex)
{
	int i = 0, j = 0, flag = 0;
	if((mode >= MAX_AUDIO_TRACK)||(mode < 0) || (pRetIndex == NULL))
	{
		mode = 0;
	}
	for(i = mode; i < MAX_AUDIO_TRACK; i++)
	{
		if(gs_PmtInfoPara.descrip_info[i].stream_type == PRIVATE_PES_STREAM)
		{
			flag = 1;
			for(j = 0; j < mode; j++)
			{
				if(gs_PmtInfoPara.descrip_info[i].elementary_PID == gs_PmtInfoPara.descrip_info[j].elementary_PID)
				{//the same elementary_PID, the same audio track
					flag = 0;
					break;
				}
			}
			if(flag)
			{
				*pRetIndex = (uint8_t)i;
				return gs_PmtInfoPara.descrip_info[i].descrip_tag;
			}
		}
	}
	return -1;
}

int app_get_muli_audio_type_by_elempid(int elem_pid)
{
    int i = 0;
    for(i = 0; i<MAX_AUDIO_TRACK; i++)
    {
        if(gs_PmtInfoPara.descrip_info[i].elementary_PID == elem_pid)
            return gs_PmtInfoPara.descrip_info[i].descrip_tag;
    }
    return -1;
}

int app_get_video_type_from_pmt(int sel)
{
	if(sel >= MAX_MUIVIDEO_COUNT)
	{
		return 0;
	}

	return gs_PmtInfoPara.v_info[sel].video_type;
}

int app_get_ac3_count_num(void)
{
	int ret_value = 0;

	ret_value = gs_PmtInfoPara.ac3_count;

	return ret_value;
}

uint8_t app_get_audio_count_num(void)
{
	return gs_PmtInfoPara.audio_count;
}


uint16_t app_get_video_pid_status(int number,int *count)
{
	if((number >= MAX_MUIVIDEO_COUNT)||(number < 0))
	{
		number = 0;
	}

	if(number >gs_PmtInfoPara.video_count)
	{
		*count = 1;
		return 0;
	}

	*count = gs_PmtInfoPara.video_count;

	return gs_PmtInfoPara.v_info[number].video_id;

}

int app_get_ad_pid_and_type(int* ad_type,int *ad_pid)
{
	int index = 0;

	for(index=0; index < MAX_AUDIO_DES_COUNT; index++)
	{
		if((gs_PmtInfoPara.audio_descriptor[index].audio_lang_type==2)
			||(gs_PmtInfoPara.audio_descriptor[index].audio_lang_type==3))
		{
			*ad_type = gs_PmtInfoPara.audio_descriptor[index].audio_type;
			*ad_pid = gs_PmtInfoPara.audio_descriptor[index].ad_audio_id;
			break;
		}
	}

    if(index == MAX_AUDIO_DES_COUNT)
        return 0;
    return gs_PmtInfoPara.audio_descriptor[index].audio_lang_type;

}

int app_get_muli_audio_pid(int index,int *count)
{
	if(index >= MAX_AUDIO_DES_COUNT)
	{
		index = 0;
	}

	*count = gs_PmtInfoPara.audio_count;

	return gs_PmtInfoPara.audio_descriptor[index].ad_audio_id ;

}

int app_pmt_get_muli_audio_type_by_elempid(int elem_pid)
{
    int i = 0;
    for(i = 0; i<MAX_AUDIO_DES_COUNT; i++)
    {
        if(gs_PmtInfoPara.audio_descriptor[i].ad_audio_id == elem_pid)
            return gs_PmtInfoPara.audio_descriptor[i].audio_type;
    }
    return -1;
}

int app_pmt_get_muli_audio_type(int index)
{
	if(index >= MAX_AUDIO_DES_COUNT)
	{
		index = 0;
	}

	return gs_PmtInfoPara.audio_descriptor[index].audio_type;
}

char *app_pmt_get_audio_lang_str(uint32_t index)
{
	char *str = NULL;
	str = (char *)gs_PmtInfoPara.audio_descriptor[index].iso_lang;
    return str;
}

int app_pmt_check_service_id(unsigned short service_id)
{
    if (gs_PmtInfoPara.service_id == service_id)
        return 0;
    else
        return -1;
}

int app_pmt_check_apid(unsigned short apid)
{
    unsigned char index = 0;
    unsigned char audio_count = gs_PmtInfoPara.audio_count;
    unsigned char audio_total = (audio_count > MAX_AUDIO_DES_COUNT) ? 1 : audio_count;

    for (index = 0; index < audio_total; index++) {
        if(apid == (unsigned short)gs_PmtInfoPara.audio_descriptor[index].ad_audio_id) {
            return 0;
        }
    }

    return -1;
}

int app_pmt_check_vpid(unsigned short vpid)
{
    unsigned char index = 0;
    unsigned char video_total = (gs_PmtInfoPara.video_count > MAX_MUIVIDEO_COUNT) ? 1 : gs_PmtInfoPara.video_count;

    for (index = 0; index < video_total; index++) {
        if(vpid == (unsigned short)gs_PmtInfoPara.v_info[index].video_id) {
            return 0;
        }
    }

    return -1;
}


int app_get_ca_system_pid_status(int mode,int *count)
{
	if((mode >= MAX_AUDIO_TRACK) || (mode < 0))
	{
		mode = 0;
	}

	if(mode >gs_PmtInfoPara.ca_count)
	{
		*count = 1;
		return 0;
	}

	*count = gs_PmtInfoPara.ca_count;

	return gs_PmtInfoPara.cas_id[mode];

}

static bool check_cas_pid(int16_t cas_id )
{
	int16_t j;

	for(j=0;j<gs_PmtInfoPara.ca_count;j++)
	{
		if(gs_PmtInfoPara.cas_id[j] == cas_id)
		{
			return FALSE;
		}
	}

	return TRUE;
}

static bool check_video_pid(int16_t video_id )
{
	int16_t j;
	for(j=0;j<gs_PmtInfoPara.video_count;j++)
	{
		if(gs_PmtInfoPara.v_info[j].video_id== video_id)
		{
			return FALSE;
		}
	}
	return TRUE;
}

static bool check_audio_pid(int16_t audio_id )
{
	int16_t j;
	for(j=0;j<gs_PmtInfoPara.audio_count;j++)
	{
		if(gs_PmtInfoPara.audio_descriptor[j].ad_audio_id== audio_id)
		{
			return FALSE;
		}
	}
	return TRUE;
}

#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT
static void app_pmt_update_prog_data(PmtInfo *pmt_info)
{
	GxMsgProperty_NodeByPosGet node;
	GxMsgProperty_NodeModify prog_modify = {0};
    TeletextDescriptor     *ttx_descriptor = NULL;
    Teletext               *ttx_info = NULL;
	int modify_flag = 0;
    char* p = NULL;
    char* ptr = NULL;
    uint8_t count = 0;
    int i = 0;
    int j = 0;
    int flag = 0;
    int audio_index = 0;
    char lang_buff[60];
    char lang[3][4];
    char audiolang[4] = {0};

	node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node))
        return;

	prog_modify.node_type= NODE_PROG;
	memcpy(&prog_modify.prog_data, &node.prog_data, sizeof(GxBusPmDataProg));
	if (gs_PmtInfoPara.service_id == node.prog_data.service_id)
	{
        if(gs_PmtInfoPara.video_count > 0)
        {
            for(i=0; i < gs_PmtInfoPara.video_count; i++)
            {
                if(gs_PmtInfoPara.v_info[i].video_id == node.prog_data.video_pid && gs_PmtInfoPara.v_info[i].video_type == node.prog_data.video_type)
                {
                    flag = 1;
                    break;
                }
            }
            if(flag != 1)
            {
                prog_modify.prog_data.video_pid = gs_PmtInfoPara.v_info[0].video_id;
                prog_modify.prog_data.video_type = gs_PmtInfoPara.v_info[0].video_type;
                modify_flag = 1;
            }
            i = 0;
            flag = 0;
        }
        if(gs_PmtInfoPara.audio_count > 0)
        {
            if(NULL ==  GxBus_ConfigGet(PRIORITY_LANG_KEY,lang_buff,sizeof(lang_buff),PRIORITY_LANG_VALUE))
            {
                strcpy(lang_buff,PRIORITY_LANG_VALUE);
            }

            memset(lang,0,3*4);
            p = strtok_r(lang_buff, "_",&ptr);
            while(p)
            {
                if(count == 3)
                    break;
                memcpy(lang[count],p,3);
                count++;
                p = strtok_r(NULL, "_",&ptr);
            }
            for(i=0; i<gs_PmtInfoPara.audio_count; i++)
            {
                if(gs_PmtInfoPara.audio_descriptor[i].ad_audio_id == node.prog_data.cur_audio_pid && gs_PmtInfoPara.audio_descriptor[i].audio_type == node.prog_data.cur_audio_type)
                {
                    flag = 1;
                    break;
                }
                if(flag != 2)
                {
                    memset(audiolang,0,4);
                    memcpy(audiolang,gs_PmtInfoPara.audio_descriptor[i].iso_lang,3);
                    if(0 == app_language_code_cmp(audiolang, (char *)lang[0]))
                    {
                        audio_index = i;
                        flag = 2;
                    }
                    else if(0 == app_language_code_cmp(audiolang, (char *)lang[1]) && flag == 0)
                    {
                        audio_index = i;
                        flag = 3;
                    }
                }
            }
            if(flag != 1)
            {
                prog_modify.prog_data.cur_audio_pid = gs_PmtInfoPara.audio_descriptor[audio_index].ad_audio_id;
                prog_modify.prog_data.cur_audio_type = gs_PmtInfoPara.audio_descriptor[audio_index].audio_type;
                modify_flag = 1;
            }
        }
        if(gs_PmtInfoPara.ca_count != 0 )
        {
            if(prog_modify.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_DISABLE)
            {
                prog_modify.prog_data.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
                prog_modify.prog_data.cas_id = gs_PmtInfoPara.cas_id[0];
                modify_flag = 1;
            }
            else if(prog_modify.prog_data.cas_id != gs_PmtInfoPara.cas_id[0])
            {
                prog_modify.prog_data.cas_id = gs_PmtInfoPara.cas_id[0];
                modify_flag = 1;
            }
        }
        else if(gs_PmtInfoPara.ca_count == 0 && (prog_modify.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE))
        {
            prog_modify.prog_data.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
            modify_flag = 1;
        }
        if(gs_PmtInfoPara.pcr_pid != node.prog_data.pcr_pid)
        {
            prog_modify.prog_data.pcr_pid = gs_PmtInfoPara.pcr_pid;
            modify_flag = 1;
		}
		if(node.prog_data.logic_valid == 1)
		{
			prog_modify.prog_data.logic_valid = 0;
			modify_flag = 1;
		}
        if (pmt_info->ttx_count != 0)
        {
            if(prog_modify.prog_data.ttx_flag == GXBUS_PM_PROG_BOOL_DISABLE)
            {
                prog_modify.prog_data.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
                modify_flag = 1;
            }
            flag = 0;
            for (i = 0; i < pmt_info->ttx_count; i++)
            {
                ttx_descriptor = (TeletextDescriptor *) (pmt_info->ttx_info[i]);
                for (j = 0; j < ttx_descriptor->ttx_num; j++)
                {
                    if (ttx_descriptor->ttx)
                    {
                        ttx_info = (Teletext*)((Teletext*)(ttx_descriptor->ttx) + j);
                        if (ttx_info->type == 0x02 || ttx_info->type == 0x05)
                        {
                            if(prog_modify.prog_data.cc_flag == GXBUS_PM_PROG_BOOL_DISABLE)
                            {
                                prog_modify.prog_data.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
                                modify_flag = 1;
                            }
                            flag = 1;
                            break;
                        }
                    }
                }
                if(flag == 1)
                    break;
            }
            if(flag == 0 && prog_modify.prog_data.cc_flag == GXBUS_PM_PROG_BOOL_ENABLE)
            {
                prog_modify.prog_data.cc_flag = GXBUS_PM_PROG_BOOL_DISABLE;
                modify_flag =1;
            }
        }
        else if((pmt_info->ttx_count == 0) && (node.prog_data.ttx_flag == GXBUS_PM_PROG_BOOL_ENABLE))
        {
            prog_modify.prog_data.ttx_flag = GXBUS_PM_PROG_BOOL_DISABLE;
            modify_flag = 1;
        }
        if ((pmt_info->subt_count != 0) && (node.prog_data.subt_flag != GXBUS_PM_PROG_BOOL_ENABLE))
        {
             prog_modify.prog_data.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
             modify_flag = 1;
        }
        else if((pmt_info->subt_count == 0) && (node.prog_data.subt_flag == GXBUS_PM_PROG_BOOL_ENABLE))
        {
             prog_modify.prog_data.subt_flag = GXBUS_PM_PROG_BOOL_DISABLE;
             modify_flag = 1;
        }

	}
	if(modify_flag == 1)
	{
		modify_flag = 0;
		GxBus_PmProgInfoModify(&(prog_modify.prog_data));
		GxBus_PmSync(GXBUS_PM_SYNC_PROG);
        app_auto_update_send_ui_msg(ACU_UPDATE_PMT,0,NULL);
	}
}
#endif

static private_parse_status pmt_private_function(uint8_t *p_section_data, uint32_t len)
{
#if CI_SUPPORT
    {
        extern void cim_update_pmt(const unsigned char *pPMT,int len,unsigned int nCimNum);
        cim_update_pmt(p_section_data,len, 0/*UPDATE_PMT_CIM_0*/);
    }
#endif
    g_AppPmt.sync_flag = 0;
    return PRIVATE_SUBTABLE_OK;
}
//#endif

/* Exported functions ----------------------------------------------------- */
// return release pmt pid, error return 0
static uint32_t app_pmt_stop(AppPmt *pmt)
{
#if ECM_SUPPORT > 0
	g_AppEcm.stop(&g_AppEcm);
#endif
	// for ttx subtitle
	g_AppTtxSubt.ttx_pre_process_close(&g_AppTtxSubt);
	g_AppTtxSubt.clean(&g_AppTtxSubt);

    GxCore_MutexLock(s_pmt_mutex);
    pmt->ver = 0xff;
    if (pmt->subt_id != -1)
    {
        GxSubTableDetail subt_detail = {0};
        GxMsgProperty_SiGet si_get;
        GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)pmt->subt_id;

        subt_detail.si_subtable_id = pmt->subt_id;
        si_get = &subt_detail;
        app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

        app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
        pmt->subt_id = -1;
        GxCore_MutexUnlock(s_pmt_mutex);
        return subt_detail.si_filter.pid;
    }
    GxCore_MutexUnlock(s_pmt_mutex);
    return 0;
}

// ts_src TS1-0 TS2-1 TS3-2...
static void app_pmt_start(AppPmt *pmt)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_NodeByPosGet node;
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;

    if(s_pmt_mutex == -1)
    {
        GxCore_MutexCreate(&s_pmt_mutex);
    }

    pmt->stop(pmt);

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node);

    pmt->pid = node.prog_data.pmt_pid;

    // create pmt filter
    subt_detail.ts_src = (uint16_t)pmt->ts_src;

    AppFrontend_Config config = {0};
    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_CONFIG_GET, &config);

    subt_detail.demux_id = config.dmx_id;// TODO: 20120416

    // add in 20121212
#if CA_SUPPORT
    app_demux_param_adjust(&subt_detail.ts_src, &subt_detail.demux_id, 0);
#endif
#ifdef SUBTTRANS_SUPPORT
    int time_st=0;
    extern int Gx_SubtTrans_GetDelayms(void);
    time_st = Gx_SubtTrans_GetDelayms();
    if(time_st > 0) {
        subt_detail.demux_id = TS_2_DMX;
        subt_detail.ts_src = 0;
    }
#endif
    subt_detail.si_filter.pid = (uint16_t)pmt->pid;
    subt_detail.si_filter.match_depth = 5;
    subt_detail.si_filter.eq_or_neq = EQ_MATCH;
    subt_detail.si_filter.match[0] = PMT_TID;
    subt_detail.si_filter.mask[0] = 0xff;
    subt_detail.si_filter.match[3] = (node.prog_data.service_id >> 8);
    subt_detail.si_filter.mask[3] = 0xff;
    subt_detail.si_filter.match[4] = (node.prog_data.service_id & 0xff);
    subt_detail.si_filter.mask[4] = 0xff;
    subt_detail.si_filter.soft_filter = SOFT_ON;
    subt_detail.time_out = 5000000;

#if 1
    subt_detail.table_parse_cfg.mode = PARSE_WITH_STANDARD;
    subt_detail.table_parse_cfg.table_parse_fun = pmt_private_function;
#else
    subt_detail.table_parse_cfg.mode = PARSE_STANDARD_ONLY;
    subt_detail.table_parse_cfg.table_parse_fun = NULL;
#endif

    GxCore_MutexLock(s_pmt_mutex);

    params_create = &subt_detail;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

    if(-1 == subt_detail.si_subtable_id)
    {
        app_log_error("\033[31mcreate pmt subtable failed\033[0m\n");
        GxCore_MutexUnlock(s_pmt_mutex);
        return;
    }

    // get the return "si_subtable_id"
    pmt->subt_id = subt_detail.si_subtable_id;
    pmt->req_id = subt_detail.request_id;

    params_start = pmt->subt_id;
    app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

    pmt->ver = 0xff;
    GxCore_MutexUnlock(s_pmt_mutex);

// recoder
    pmt->dmx_id = subt_detail.demux_id;
}

static status_t app_pmt_get(AppPmt *pmt, GxMsgProperty_SiSubtableOk *data)
{
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiGet params_get = NULL;
    GxMsgProperty_SiModify params_modify = NULL;
    PmtInfo *pmt_info = (PmtInfo*)data->parsed_data;

    if (data->si_subtable_id == pmt->subt_id
            && data->table_id == PMT_TID
            && data->request_id == pmt->req_id)
    {
        APP_PRINT("app_pmt_get ver orgi: %d   cur: %d\n", pmt->ver, pmt_info->si_info.ver_number);
        if(pmt->ver != pmt_info->si_info.ver_number)
        {
            g_AppPmt.analyse(&g_AppPmt, data->parsed_data);
            GxCore_MutexLock(s_pmt_mutex);
            pmt->ver = pmt_info->si_info.ver_number;
            if(pmt->subt_id != -1)
            {
                // get original pmt info
                subt_detail.si_subtable_id = pmt->subt_id;
                params_get = &subt_detail;
                app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)(&params_get));

                // change to unmatch filter
                subt_detail.si_filter.match_depth = 6;
                subt_detail.si_filter.eq_or_neq = NEQ_MATCH;
                subt_detail.si_filter.match[5] = pmt->ver<<1;
                subt_detail.si_filter.mask[5] = 0x3e;
                subt_detail.time_out = NEQ_PARSE_TIME_MS;

                params_modify = &subt_detail;
                app_send_msg_exec(GXMSG_SI_SUBTABLE_MODIFY, (void*)&params_modify);
            }
            GxCore_MutexUnlock(s_pmt_mutex);
        }

        GxBus_SiParserBufFree(data->parsed_data);
        return GXCORE_SUCCESS;
    }

    return GXCORE_ERROR;
}

static int _app_pmt_cas_get(
		uint32_t * cas1,
		uint32_t cas1_count,
		uint32_t * cas2,
		uint32_t cas2_count,
		uint32_t * ca_num)
{
	uint32_t     i = 0;
	CaDescriptor *cas_decriptor = NULL;

	if (cas2 != NULL){ //以分开加密优先
		cas_decriptor = (CaDescriptor *)(cas2[0]);
		gs_PmtInfoPara.cas_id[*ca_num] = CAS_ID_UNKNOW;
		for (i = 0; i < cas2_count; i++) {
			cas_decriptor = (CaDescriptor *) (cas2[i]);
			if((check_cas_pid(cas_decriptor->ca_system_id))&&(*ca_num<MAX_CAS_COUNT)){
				gs_PmtInfoPara.cas_id[(*ca_num)++] = cas_decriptor->ca_system_id;
				gs_PmtInfoPara.ca_count++;
				app_log_debug("cas_id:%d\n", cas_decriptor->ca_system_id);
			}
		}
	}
	if (i == cas2_count && cas1 != NULL){ //分开加密没找到,找相同加密
		cas_decriptor = (CaDescriptor *) (cas1[0]);
		gs_PmtInfoPara.cas_id[*ca_num] = CAS_ID_UNKNOW;
		for (i = 0; i < cas1_count; i++) {
			cas_decriptor = (CaDescriptor *)(cas1[i]);
			if((check_cas_pid(cas_decriptor->ca_system_id))&&(*ca_num<MAX_CAS_COUNT)){
				gs_PmtInfoPara.cas_id[(*ca_num)++] = cas_decriptor->ca_system_id;
				gs_PmtInfoPara.ca_count++;
				app_log_debug("cas_id:%d\n", cas_decriptor->ca_system_id);
			}
		}
	}
	return 0;
}

static int _app_pmt_get_si_info(PmtInfo *pmt_info)
{
	uint32_t       *cas_info = NULL;
	uint32_t       *cas_info2 = NULL;
	stream_info_t  *stream_info = NULL;
	Ac3Descriptor  *ac3_descriptor = NULL;
	Eac3Descriptor *eac3_descriptor = NULL;
	uint32_t        i = 0;
	uint32_t        j = 0;
	uint32_t        stream_type = 0;
	status_t        ret = 0;
	uint32_t        video_audio = STREAM_VIDEO_TYPE;
	uint32_t        video_num = 0, audio_num = 0, ca_num = 0;

	memset(&gs_PmtInfoPara, 0, sizeof(pmt_info_stream_t));

	stream_info = (stream_info_t *) (pmt_info->stream_info);

	if (pmt_info->ca_count != 0)//先判断音视频是不是同密
		cas_info = pmt_info->ca_info;
	if (pmt_info->ca_count2 != 0)//再判断音视频是不是分开加密
		cas_info2 = pmt_info->ca_info2;

	for (i = 0; i < pmt_info->stream_count; i++){//这里只获得pcm的信息
		video_audio = STREAM_UNKNOWN_TYPE;
		stream_type = stream_info[i].stream_type;
		switch (stream_type) {
			case MPEG_1_VIDEO:
			case MPEG_2_VIDEO:
				gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_MPEG;
				video_audio = STREAM_VIDEO_TYPE;
				break;
			case MPEG_4_VIDEO:
				gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_MPEG4;
				video_audio = STREAM_VIDEO_TYPE;
				break;
			case H264:
				gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_H264;
				video_audio = STREAM_VIDEO_TYPE;
				break;
			case H265:
				gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_H265;
				video_audio = STREAM_VIDEO_TYPE;
				break;
			case AVS:
				gs_PmtInfoPara.v_info[video_num].video_type = GXBUS_PM_PROG_AVS;
				video_audio = STREAM_VIDEO_TYPE;
				break;
			case MPEG_1_AUDIO:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_MPEG1;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case MPEG_2_AUDIO:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_MPEG2;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case AAC_ADTS:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case AAC_LATM:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case PRIVATE_PES_STREAM://probably be EAC3
				{
					uint8_t *d = pmt_info->registration;
					if (strncmp((char*)d, "DRA1", 4) == 0) {
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DRA;
						video_audio = STREAM_AUDIO_TYPE;
					}
					else if(strncmp((char*)d,"AC-3",4) == 0 && stream_info[i].iso639_info)
					{
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AC3;
						video_audio = STREAM_AUDIO_TYPE;
					}
					else
						video_audio = STREAM_PRIVATE_TYPE;//for check EAC3
				}
				break;
			case LPCM:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_LPCM;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case AC3:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AC3;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case DTS:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES){
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DTS;
						video_audio = STREAM_AUDIO_TYPE;
					}
				}
				break;
			case DOLBY_TRUEHD:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case AC3_PLUS:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case DTS_HD:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES){
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DTS_HD;
						video_audio = STREAM_AUDIO_TYPE;
					}
				}
				break;
			case DTS_MA:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES){
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DTS_MA;
						video_audio = STREAM_AUDIO_TYPE;
					}
				}
				break;
			case AC3_PLUS_SEC:
				gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
				video_audio = STREAM_AUDIO_TYPE;
				break;
			case DTS_HD_SEC:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES){
						gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
						video_audio = STREAM_AUDIO_TYPE;
					}
				}
				break;
			default:
				app_log_error("can't support stream type!!type = %x\n", stream_type);
				video_audio = STREAM_UNKNOWN_TYPE;
				break;
		}
		if(video_audio == STREAM_VIDEO_TYPE){
			_app_pmt_cas_get(cas_info, pmt_info->ca_count, cas_info2, pmt_info->ca_count2, &ca_num);
			if(check_video_pid(stream_info[i].elem_pid))
				gs_PmtInfoPara.v_info[video_num].video_id = stream_info[i].elem_pid;
			gs_PmtInfoPara.video_count++;
			video_num++;
		}else if(video_audio == STREAM_AUDIO_TYPE){
			_app_pmt_cas_get(cas_info, pmt_info->ca_count, cas_info2, pmt_info->ca_count2, &ca_num);
			if((gs_PmtInfoPara.audio_count < MAX_AUDIO_DES_COUNT) && (check_audio_pid(stream_info[i].elem_pid))){
                gs_PmtInfoPara.audio_descriptor[audio_num].ad_audio_id = stream_info[i].elem_pid;
                if((stream_info[i].iso639_count != 0) && (stream_info[i].iso639_info)){
                    gs_PmtInfoPara.audio_descriptor[audio_num].audio_lang_type = stream_info[i].iso639_info->audio_type;
                    memcpy(gs_PmtInfoPara.audio_descriptor[audio_num].iso_lang, stream_info[i].iso639_info->iso639, 3);
                }
            }
			gs_PmtInfoPara.descrip_info[audio_num].stream_type    = stream_info[i].stream_type;
			gs_PmtInfoPara.descrip_info[audio_num].descrip_tag    = pmt_info->buf[0];
			gs_PmtInfoPara.descrip_info[audio_num].elementary_PID = stream_info[i].elem_pid;
			gs_PmtInfoPara.audio_count++;
			audio_num++;
		}else if(video_audio == STREAM_UNKNOWN_TYPE)
			continue;
	}

	gs_PmtInfoPara.service_id = pmt_info->prog_num;
	gs_PmtInfoPara.pcr_pid   = pmt_info->pcr_pid;
	gs_PmtInfoPara.ac3_count  = pmt_info->ac3_count + pmt_info->eac3_count;

	if ((pmt_info->ac3_count > 0) && (pmt_info->ac3_info)){
		for (i = 0; i < pmt_info->ac3_count; i++) {
			ac3_descriptor = (Ac3Descriptor *) (pmt_info->ac3_info[i]);
			if((gs_PmtInfoPara.audio_count < MAX_AUDIO_DES_COUNT) && (check_audio_pid(ac3_descriptor->elem_pid))){
                gs_PmtInfoPara.audio_descriptor[audio_num].ad_audio_id = ac3_descriptor->elem_pid;
                gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_AC3;
                for(j = 0; j < pmt_info->stream_count; j++){
                    if(stream_info[j].elem_pid == ac3_descriptor->elem_pid){
                        gs_PmtInfoPara.descrip_info[audio_num].stream_type = stream_info[j].stream_type;
                        if((stream_info[j].iso639_count != 0) && (stream_info[j].iso639_info)){
                            gs_PmtInfoPara.audio_descriptor[audio_num].audio_lang_type = stream_info[j].iso639_info->audio_type;
                            memcpy(gs_PmtInfoPara.audio_descriptor[audio_num].iso_lang, stream_info[j].iso639_info->iso639, 3);
                        }
                        break;
                    }
                }
                gs_PmtInfoPara.descrip_info[audio_num].descrip_tag    = AC3_DESCRIPTOR;
                gs_PmtInfoPara.descrip_info[audio_num].elementary_PID = ac3_descriptor->elem_pid;
                gs_PmtInfoPara.audio_count++;
                audio_num++;
                app_log_debug("stream_type:%d elementary_PID:0x%x\n",
                pmt_info->stream_info->stream_type,
                ac3_descriptor->elem_pid);
            }
        }
    }
    if ((pmt_info->eac3_count > 0) && (pmt_info->eac3_info)){
        for (i = 0; i < pmt_info->eac3_count; i++) {
            eac3_descriptor = (Eac3Descriptor *) (pmt_info->eac3_info[i]);
            if((gs_PmtInfoPara.audio_count < MAX_AUDIO_DES_COUNT) && (check_audio_pid(eac3_descriptor->elem_pid))){
                gs_PmtInfoPara.audio_descriptor[audio_num].ad_audio_id = eac3_descriptor->elem_pid;
                gs_PmtInfoPara.audio_descriptor[audio_num].audio_type = GXBUS_PM_AUDIO_EAC3;
                for (j = 0; j < pmt_info->stream_count; j++){
                    if(stream_info[j].elem_pid == eac3_descriptor->elem_pid){
                        gs_PmtInfoPara.descrip_info[audio_num].stream_type = stream_info[j].stream_type;
                        if ((stream_info[j].iso639_count != 0) && (stream_info[j].iso639_info)){
                            gs_PmtInfoPara.audio_descriptor[audio_num].audio_lang_type = stream_info[j].iso639_info->audio_type;
                            memcpy(gs_PmtInfoPara.audio_descriptor[audio_num].iso_lang, stream_info[j].iso639_info->iso639, 3);
                        }
                        break;
                    }
                }
                gs_PmtInfoPara.descrip_info[audio_num].descrip_tag    = EAC3_DESCRIPTOR;
                gs_PmtInfoPara.descrip_info[audio_num].elementary_PID = eac3_descriptor->elem_pid;
                gs_PmtInfoPara.audio_count++;
                audio_num++;
                app_log_debug("stream_type:%d elementary_PID:0x%x\n",pmt_info->stream_info->stream_type,eac3_descriptor->elem_pid);
            }
        }
    }
#if PMT_UPDATE_SUPPORT || AUTO_UPDATE_SUPPORT
	app_pmt_update_prog_data(pmt_info);
#endif
	return ret;
}

static status_t app_pmt_analyse(AppPmt *pmt, uint8_t *section_data)
{
    PmtInfo *pmt_info = (PmtInfo*)section_data;
    uint32_t i;
    int ret = GXCORE_ERROR;

    app_log_debug("app_pmt_get ver orgi: %d   cur: %d\n", pmt->ver, pmt_info->si_info.ver_number);
    GxMsgProperty_NodeByPosGet prog_node;

    if(_app_pmt_get_si_info(pmt_info) >= 0)
        g_AppPmt.sync_flag = 1;

    prog_node.node_type = NODE_PROG;
    prog_node.pos = g_AppPlayOps.normal_play.play_count;
    ret = app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&prog_node);

#if CA_SUPPORT > 0
    if(ret == GXCORE_SUCCESS)
    {
        app_extend_send_service(prog_node.prog_data.id,
                                prog_node.prog_data.sat_id,
                                prog_node.prog_data.tp_id,
                                pmt->dmx_id,
                                CONTROL_NORMAL);
    }
#endif

#if ECM_SUPPORT > 0
    g_AppEcm.DemuxId = pmt->dmx_id;//0;// TODO: 20120702
    g_AppEcm.CaManager.ProgId = prog_node.prog_data.id;
    g_AppEcm.CaManager.Type = CONTROL_NORMAL;
    g_AppEcm.storeinfo((void*)pmt_info,&g_AppEcm);
    // add in 20121212
#if CA_SUPPORT
{
    uint16_t DemuxId = 0;
    app_demux_param_adjust(&g_AppEcm.ts_src, &DemuxId, 0);
    g_AppEcm.DemuxId = DemuxId;
}
#endif
    g_AppEcm.start(&g_AppEcm);
#endif

    g_AppTtxSubt.sync(&g_AppTtxSubt, pmt_info);
    // app function embeded module
    for(i = 0; i<pmt_info->stream_count; i++)
    {
        // only video start subt
        if((MPEG_1_VIDEO == pmt_info->stream_info[i].stream_type)
                || (MPEG_2_VIDEO == pmt_info->stream_info[i].stream_type)
                || (H264 == pmt_info->stream_info[i].stream_type)
                || (H265 == pmt_info->stream_info[i].stream_type))
        {
            app_subt_change();
        }
    }
	// restart the ttx ree-process, if APP support the pre-process function
	g_AppTtxSubt.ttx_pre_process_init(&g_AppTtxSubt);

    app_track_set(pmt_info);
#if AUDIO_DESCRIPTOR_SUPPORT
    app_ad_track_set(pmt_info);
#endif
    return ret;
}

AppPmt g_AppPmt =
{
    .subt_id    = -1,
    .req_id     = 0,
    .ver        = 0xff,
    .start      = app_pmt_start,
    .stop       = app_pmt_stop,
    .get        = app_pmt_get,
    .analyse    = app_pmt_analyse,
    .cb         = NULL,
};

/* End of file -------------------------------------------------------------*/


