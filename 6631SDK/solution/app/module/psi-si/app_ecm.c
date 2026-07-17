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
#include "app_config.h"
#if ECM_SUPPORT > 0
#include "app_module.h"
#include "module/si/si_pmt.h"
#include "gxsi.h"
#include "gxmsg.h"
#include "app_send_msg.h"
#include "module/config/gxconfig.h"
#include "app_default_params.h"
#include "gxcore.h"

#define NEQ_PARSE_TIME_MS    (24*60*60*1000UL)
#define ECM_DATA_FIFO_NUM    (5)

typedef struct
{
    uint8_t section_data[ECM_DATA_LENGTH];
    uint32_t sub_idx;
}EcmInfo;

static handle_t s_ecm_mutex = -1;
static handle_t s_ecm_fifo = -1;
static handle_t s_ecm_thread_handle = -1;

/* Private functions ------------------------------------------------------ */

static void app_ecm_thread(void* arg)
{
    static EcmInfo ecm_info;
    while(1)
    {
        memset(&ecm_info, 0, sizeof(EcmInfo));
        if(app_fifo_read(s_ecm_fifo, (char*)&ecm_info, sizeof(EcmInfo)) > 0)
        {

            g_AppEcm.analyse(&g_AppEcm, ecm_info.sub_idx, ecm_info.section_data);
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            g_AppEcm2.analyse(&g_AppEcm2, ecm_info.sub_idx, ecm_info.section_data);
#endif
        }
    }
}

/* Exported functions ----------------------------------------------------- */
static uint32_t app_ecm_stop(AppEcm *Ecm)
{
	int32_t i = 0;

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);

    if(s_ecm_fifo == -1)
    {
        app_fifo_create(&s_ecm_fifo, sizeof(EcmInfo), ECM_DATA_FIFO_NUM);
        if(s_ecm_thread_handle == -1)
            GxCore_ThreadCreate("app_ecm_proc", &s_ecm_thread_handle, app_ecm_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
    }

	for(i = 0; i < Ecm->ecm_count; i++)
	{
	    if (Ecm->ecm_sub[i].subt_id != -1)
	    {
	        GxSubTableDetail subt_detail = {0};
	        GxMsgProperty_SiGet si_get;
	        GxMsgProperty_SiRelease params_release = (GxMsgProperty_SiRelease)Ecm->ecm_sub[i].subt_id;

	        subt_detail.si_subtable_id = Ecm->ecm_sub[i].subt_id;
	        si_get = &subt_detail;
	        app_send_msg_exec(GXMSG_SI_SUBTABLE_GET, (void*)&si_get);

	        app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
	        Ecm->ecm_sub[i].subt_id = -1;
	    }
	}

	Ecm->ecm_count = 0;
	memset(Ecm->ecm_sub,0,sizeof(AppEcmSub_t)*ECM_MAX_ID);
    for (i=0; i<ECM_MAX_ID; i++)
    {
        Ecm->ecm_sub[i].subt_id = -1;
    }

    GxCore_MutexUnlock(s_ecm_mutex);
    return 0;
}

// ts_src TS1-0 TS2-1 TS3-2...
uint8_t IDECM = 0;
uint8_t IDECM2 = 0;
static void app_ecm_start(AppEcm *ecm)
{
	int32_t i = 0;
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);
    //ecm->stop(ecm);

    if(s_ecm_fifo == -1)
    {
        app_fifo_create(&s_ecm_fifo, sizeof(EcmInfo), ECM_DATA_FIFO_NUM);
        if(s_ecm_thread_handle == -1)
            GxCore_ThreadCreate("app_ecm_proc", &s_ecm_thread_handle, app_ecm_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
    }

    // create pmt filter
	for(i = 0; i < ecm->ecm_count; i++)
    {
        if(app_tsd_check())
        {
            app_exshift_pid_add(1,(int*)&(ecm->ecm_sub[i].pid));
            //app_log_debug("\n\033[34mECM, %s, %d.....ts_src = %d, demux id = %d\033[0m\n", __FUNCTION__,__LINE__, ecm->ts_src, ecm->DemuxId);
        }

        memset(&subt_detail,0,sizeof(GxSubTableDetail));
        subt_detail.ts_src = (uint16_t)ecm->ts_src;
        subt_detail.demux_id = (uint16_t)ecm->DemuxId;// TODO: 20120416
        subt_detail.si_filter.pid = (uint16_t)ecm->ecm_sub[i].pid;
        subt_detail.si_filter.match_depth = 1;
        subt_detail.si_filter.eq_or_neq = EQ_MATCH;
        subt_detail.si_filter.match[0] = 0x80;
        //subt_detail.si_filter.mask[0] = 0xFE;
        subt_detail.si_filter.mask[0] = 0xF0;
        subt_detail.si_filter.crc = CRC_OFF;
        subt_detail.time_out = 100000000;//100s

        subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;//PARSE_WITH_STANDARD;
        subt_detail.table_parse_cfg.table_parse_fun = NULL;//_ecm_private;


        params_create = &subt_detail;
        app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

        // get the return "si_subtable_id"
        ecm->ecm_sub[i].subt_id = subt_detail.si_subtable_id;
        ecm->ecm_sub[i].req_id = subt_detail.request_id;

        params_start = ecm->ecm_sub[i].subt_id;
        //for test
        APP_PRINT("---[app_ecm_start][start]---\n");
        APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
                subt_detail.ts_src,subt_detail.si_filter.pid,ecm->ecm_sub[i].subt_id,ecm->ecm_sub[i].req_id);
        APP_PRINT("---[app_ecm_start][end]---\n");
        app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

        ecm->ecm_sub[i].ver = 0xff;
    }
	if(subt_detail.demux_id == 0)
		IDECM = subt_detail.si_subtable_id;
	else
		IDECM2 = subt_detail.si_subtable_id;
	 APP_PRINT("---[app_ecm_start][exit]---\n");

    GxCore_MutexUnlock(s_ecm_mutex);
}

static int _check_ECMINFO(int32_t ECMPID,int32_t elem_pid, int32_t CASPID,int32_t DescrambType,AppEcm *ecm)
{
	uint32_t i;
	uint32_t j;
	if(ECMPID == 0x1fff)
	{
		APP_PRINT("---[PARAMETER ERRER]\n");
		return FALSE;
	}
	for(i=0;i<ecm->ecm_count;i++)
	{
		if(ecm->ecm_sub[i].pid == ECMPID)
		{
            // TODO: 20160301
            for(j = 0; j < ecm->ecm_sub[i].element_count; j++)
            {
                if(ecm->ecm_sub[i].element_id[j] == elem_pid)
                {
                    break;
                }
            }
	        if((ecm->ecm_sub[i].element_count > 0)
                    && (j == ecm->ecm_sub[i].element_count)
                    && (ecm->ecm_sub[i].element_count < ELEMENT_MAX_ID))
			{
				ecm->ecm_sub[i].element_id[ecm->ecm_sub[i].element_count] = elem_pid;
				ecm->ecm_sub[i].element_count++;
			}

			for(j=0;j<ecm->ecm_sub[i].CasIDCount;j++)
			{
				if(ecm->ecm_sub[i].CasPID[j] == CASPID)
				{
					if(ecm->ecm_sub[i].ScrambleType[j] != DescrambType)
						ecm->ecm_sub[i].ScrambleType[j] = SCRAMBLE_COMMON;
					return FALSE;//same ecmpid & same casid & different scrambletype
				}
			}
			//add the new cassystem
			if(ecm->ecm_sub[i].CasIDCount < CAS_COUNT_SUPPORT)
			{
				ecm->ecm_sub[i].CasPID[ecm->ecm_sub[i].CasIDCount] = CASPID;
				ecm->ecm_sub[i].ScrambleType[ecm->ecm_sub[i].CasIDCount] = DescrambType;
				ecm->ecm_sub[i].CasIDCount++;
			}
			else
				APP_PRINT("---[too many casid]\n");
			return FALSE;
		}
	}
	// add new ecmid
	if(ecm->ecm_count < ECM_MAX_ID)
	{
		ecm->ecm_sub[ecm->ecm_count].pid = ECMPID;
        // TODO: 20160301
        ecm->ecm_sub[ecm->ecm_count].element_id[0] = elem_pid;
        ecm->ecm_sub[ecm->ecm_count].element_count = 1;
		ecm->ecm_sub[ecm->ecm_count].CasPID[0] = CASPID;
		ecm->ecm_sub[ecm->ecm_count].ScrambleType[0] = DescrambType;
		ecm->ecm_sub[ecm->ecm_count].CasIDCount = 1;
		ecm->ecm_count++;
	}
	else
		APP_PRINT("---[too many ecmid]\n");

	return TRUE;
}

static void app_ecm_init(AppEcm *ecm)
{
	int32_t i = 0;

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);

    if(s_ecm_fifo == -1)
    {
        app_fifo_create(&s_ecm_fifo, sizeof(EcmInfo), ECM_DATA_FIFO_NUM);
        if(s_ecm_thread_handle == -1)
            GxCore_ThreadCreate("app_ecm_proc", &s_ecm_thread_handle, app_ecm_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
    }

	//memset(ecm->ecm_sub,0,sizeof(AppEcmSub_t)*ECM_MAX_ID);
    for (i=0; i<ECM_MAX_ID; i++)
    {
        ecm->ecm_sub[i].subt_id = -1;
    }
    GxCore_MutexUnlock(s_ecm_mutex);
}

static void app_ecm_store_info(void *pInfo,AppEcm *ecm)
{
	int32_t i = 0;
	int32_t j = 0;
	PmtInfo* pmt_info = (PmtInfo*)pInfo;
	CaDescriptor *p = NULL;
	if(pmt_info == NULL)
	{
		APP_PRINT("---[parameter error]\n");
		return;
	}

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);

    if(s_ecm_fifo == -1)
    {
        app_fifo_create(&s_ecm_fifo, sizeof(EcmInfo), ECM_DATA_FIFO_NUM);
        if(s_ecm_thread_handle == -1)
            GxCore_ThreadCreate("app_ecm_proc", &s_ecm_thread_handle, app_ecm_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
    }

	//clear the data of the ecm info
	ecm->ts_src = pmt_info->si_info.ts_id;
	ecm->ecm_count = 0;
    for (i=0; i<ECM_MAX_ID; i++)
    {
        ecm->ecm_sub[i].pid = 0;
        // TODO:20160301
        for (j = 0; j < ELEMENT_MAX_ID; j++)
		{
			ecm->ecm_sub[i].element_id[j] = 0;
		}
		ecm->ecm_sub[i].element_count = 0;

		for (j=0; j<CAS_COUNT_SUPPORT; j++)
		{
			ecm->ecm_sub[i].CasPID[j] = 0;
			ecm->ecm_sub[i].ScrambleType[j] = 0;
		}
		ecm->ecm_sub[i].CasIDCount = 0;
    }
	// for ca des1
	if((pmt_info->ca_count != 0)&&(pmt_info->ca_info != NULL))
	{
		APP_PRINT("\033[31m---[ca_count][%d]\n\033[0m",pmt_info->ca_count);

		for(i = 0; i < pmt_info->ca_count; i++)
		{
			p = (CaDescriptor*)(pmt_info->ca_info[i]);
			APP_PRINT("\033[31m---[ca_pid = %x]-[ca_system_id = %x]\n\033[0m",p->ca_pid,p->ca_system_id);
			_check_ECMINFO(p->ca_pid,0x1fff,p->ca_system_id,SCRAMBLE_COMMON,ecm);
		}
	}

	// for ca des2
	if((pmt_info->ca_count2 != 0)&&(pmt_info->ca_info2 != NULL))
	{
		#define MPEG1_VIDEO     (0x01)
		#define MPEG2_VIDEO     (0x02)
		#define H264_VIDEO      (0x1b)
		#define H265_VIDEO      (0x24)
		#define AVS_VIDEO       (0x42)
		#define MPEG4_VIDEO     (0x10)
		#define MPEG2_AUDIO1    (0x03)
		#define MPEG2_AUDIO2    (0x04)
		#define AAC_AUDIO1      (0x0f)
		#define	AAC_AUDIO2		(0x11)
		#define A52_AUDIO       (0x81)
		uint32_t DescrambleType = SCRAMBLE_COMMON;
		uint32_t j = 0;
		APP_PRINT("\033[32m---[ca_count2][%d]\n\033[0m",pmt_info->ca_count2);

		for(i = 0; i < pmt_info->ca_count2; i++)
		{
			p = (CaDescriptor*)(pmt_info->ca_info2[i]);
			for(j = 0; j < pmt_info->stream_count; j++)
			{
				if(pmt_info->stream_info[j].elem_pid == p->elem_pid)
				{
					 if((MPEG1_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (MPEG2_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (H264_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (H265_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (AVS_VIDEO == pmt_info->stream_info[j].stream_type)
                        || (MPEG4_VIDEO == pmt_info->stream_info[j].stream_type))
                        DescrambleType = SCRAMBLE_VIDEO;
					 else if((MPEG2_AUDIO1 == pmt_info->stream_info[j].stream_type)
                        || (MPEG2_AUDIO2 == pmt_info->stream_info[j].stream_type)
                        || (AAC_AUDIO1 == pmt_info->stream_info[j].stream_type)
                        || (AAC_AUDIO2 == pmt_info->stream_info[j].stream_type)
                        || (A52_AUDIO == pmt_info->stream_info[j].stream_type))
                        DescrambleType = SCRAMBLE_AUDIO;
					 else
						APP_PRINT("---[can not support type]\n");
					 break;
				}
			}
            // TODO:20160301
            for(j = 0; j < pmt_info->ac3_count; j++)
            {
                Ac3Descriptor *temp = NULL;
                temp = (Ac3Descriptor*)(pmt_info->ac3_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = SCRAMBLE_AUDIO;
                    break;
                }
            }
            for(j = 0; j < pmt_info->eac3_count; j++)
            {
                Eac3Descriptor *temp = NULL;
                temp = (Eac3Descriptor*)(pmt_info->eac3_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = SCRAMBLE_AUDIO;
                    break;
                }
            }
            for(j = 0; j < pmt_info->ttx_count; j++)
            {
                TeletextDescriptor *temp = NULL;
                temp = (TeletextDescriptor*)(pmt_info->ttx_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = SCRAMBLE_TTX;
                    break;
                }
            }
            for(j = 0; j < pmt_info->subt_count; j++)
            {
                SubtDescriptor *temp = NULL;
                temp = (SubtDescriptor*)(pmt_info->subt_info[j]);
				if(temp->elem_pid == p->elem_pid)
                {
                    DescrambleType = SCRAMBLE_SUBT;
                    break;
                }
            }

			//
			APP_PRINT("\033[32m---[ca_pid2 = %x]-[ca_system_id2 = %x]-[DescrambleType = %d]-[element id = %d]\n\033[0m",p->ca_pid,p->ca_system_id,DescrambleType, p->elem_pid);
			if(DescrambleType != SCRAMBLE_COMMON)
            {
				_check_ECMINFO(p->ca_pid,p->elem_pid,p->ca_system_id,DescrambleType,ecm);
            }
		}
	}

	//test
	APP_PRINT("---------start[%d]----------\n",ecm->ecm_count);
	for(i = 0; i<ecm->ecm_count;i++)
	{
		uint32_t j = 0;
		APP_PRINT("\033[33m--ECMPID[%x]--CASCOUNT[%x]--\n\033[0m",ecm->ecm_sub[i].pid,ecm->ecm_sub[i].CasIDCount);
		for(j = 0; j<ecm->ecm_sub[i].CasIDCount;j++)
			APP_PRINT("\033[33m--CASPID[%x]--TYPE[%d]--\n\033[0m",ecm->ecm_sub[i].CasPID[j],ecm->ecm_sub[i].ScrambleType[j]);
	    for(j = 0; j<ecm->ecm_sub[i].element_count;j++)
			APP_PRINT("\033[33m--ELEMENT PID[%x]\n\033[0m",ecm->ecm_sub[i].element_id[j]);

		APP_PRINT("\n---------end----------\n");
	}
	APP_PRINT("---[app_ecm_store_info exit]\n");

    GxCore_MutexUnlock(s_ecm_mutex);
}

static status_t app_ecm_get(AppEcm *ecm, GxMsgProperty_SiSubtableOk *data)
{
    int32_t i = 0;
    static EcmInfo ecm_info;
    uint32_t length = 0;
    status_t ret = GXCORE_ERROR;

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);

    if(s_ecm_fifo == -1)
    {
        app_fifo_create(&s_ecm_fifo, sizeof(EcmInfo), ECM_DATA_FIFO_NUM);
        if(s_ecm_thread_handle == -1)
            GxCore_ThreadCreate("app_ecm_proc", &s_ecm_thread_handle, app_ecm_thread, NULL, 10*1024, GXOS_DEFAULT_PRIORITY);
    }

    for(i = 0; i < ecm->ecm_count; i++)
    {
        if (data->si_subtable_id == ecm->ecm_sub[i].subt_id
                && data->request_id == ecm->ecm_sub[i].req_id)
        {
            if( ((data->parsed_data[0] == 0x80)||(data->parsed_data[0] == 0x81)))
            {
            //APP_PRINT("---[app_ecm_analyse][count = %d]\n", ecm->ecm_sub[i].CasIDCount);
            //APP_PRINT("---[app_ecm_get][table_id = 0x%x][count = %d][data->si_subtable_id = %d]\n",data->table_id,ecm->ecm_sub[i].CasIDCount,data->si_subtable_id );
            memset(&ecm_info, 0, sizeof(EcmInfo));
            length = ((data->parsed_data[1] & 0x0f) << 8) | data->parsed_data[2];
            length += 3;
            if(length <= ECM_DATA_LENGTH)
            {
                memcpy(&ecm_info.section_data, data->parsed_data, length);
                ecm_info.sub_idx = i;
                //push data to analyse thread
                app_fifo_write(s_ecm_fifo, (char*)&ecm_info, sizeof(EcmInfo));
                ret = GXCORE_SUCCESS;
             }
            }
            GxBus_SiParserBufFree(data->parsed_data);
            break;
        }
    }

    GxCore_MutexUnlock(s_ecm_mutex);

    return ret;
}

static status_t app_ecm_analyse(AppEcm *ecm, uint32_t sub_idx, uint8_t *section_data)
{
#if CA_SUPPORT
    int i = 0;
    int j = 0;
    DataInfoClass data_info = {0};
    ControlClass control_info = {0};
    data_info.psi_type = DATA_ECM;
    data_info.data = section_data;
    data_info.data_len = ((((section_data[1] & 0xf) << 8) | section_data[2]) + 3);
    control_info.demux_id = ecm->DemuxId;
    control_info.pid = ecm->ecm_sub[sub_idx].pid;
    control_info.control_type = CONTROL_NORMAL;
    for(i = 0; i < ecm->ecm_sub[sub_idx].CasIDCount; i++)
    {
        control_info.data_type = ecm->ecm_sub[sub_idx].ScrambleType[i];
        data_info.cas_id  = ecm->ecm_sub[sub_idx].CasPID[i];

        for(j = 0; j < ecm->ecm_sub[sub_idx].element_count; j++)
        {
            control_info.element_pid = ecm->ecm_sub[sub_idx].element_id[j];
            app_send_psi_data(&data_info, &control_info);
        }
    }
#endif
    return GXCORE_SUCCESS;
}

static status_t app_ecm_timeout(AppEcm *ecm, GxParseResult * parse_result)
{
    int32_t i = 0;
    GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate  params_create;
    GxMsgProperty_SiStart params_start;
    GxMsgProperty_SiRelease params_release;

    if(s_ecm_mutex == -1)
    {
        GxCore_MutexCreate(&s_ecm_mutex);
    }

    GxCore_MutexLock(s_ecm_mutex);

    for(i = 0; i < ecm->ecm_count; i++)
    {
        if (parse_result->si_subtable_id == ecm->ecm_sub[i].subt_id
                && parse_result->request_id == ecm->ecm_sub[i].req_id)
        {
            APP_PRINT("---[app_ecm_timeout][table_id = 0x%x][count = %d][data->si_subtable_id = %d]\n",
                    parse_result->table_id, ecm->ecm_sub[i].CasIDCount, parse_result->si_subtable_id);

            params_release = (GxMsgProperty_SiRelease)ecm->ecm_sub[i].subt_id;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
            ecm->ecm_sub[i].subt_id = -1;

            memset(&subt_detail,0,sizeof(GxSubTableDetail));
            subt_detail.ts_src = (uint16_t)ecm->ts_src;
            subt_detail.demux_id = (uint16_t)ecm->DemuxId;
            subt_detail.si_filter.pid = (uint16_t)ecm->ecm_sub[i].pid;
            subt_detail.si_filter.match_depth = 1;
            subt_detail.si_filter.eq_or_neq = EQ_MATCH;
            subt_detail.si_filter.match[0] = 0x80;
            //subt_detail.si_filter.mask[0] = 0xFE;
            subt_detail.si_filter.mask[0] = 0xF0;
            subt_detail.si_filter.crc = CRC_OFF;
            subt_detail.time_out = 100000000;//100s
            subt_detail.table_parse_cfg.mode = PARSE_SECTION_ONLY;//PARSE_WITH_STANDARD;
            subt_detail.table_parse_cfg.table_parse_fun = NULL;//_ecm_private;

            params_create = &subt_detail;
            app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);
            // get the return "si_subtable_id"
            ecm->ecm_sub[i].subt_id = subt_detail.si_subtable_id;
            ecm->ecm_sub[i].req_id = subt_detail.request_id;

            params_start = ecm->ecm_sub[i].subt_id;
            //for test
            APP_PRINT("---[app_ecm_timeout][start]---\n");
            APP_PRINT("---[ts_src = %d][pid = 0x%x][subt_id = %d][req_id = %d]---\n",
                    subt_detail.ts_src,subt_detail.si_filter.pid,ecm->ecm_sub[i].subt_id,ecm->ecm_sub[i].req_id);
            APP_PRINT("---[app_ecm_timeout][end]---\n");
            app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);

            ecm->ecm_sub[i].ver = 0xff;

            break;
        }
    }

    GxCore_MutexUnlock(s_ecm_mutex);
    return GXCORE_SUCCESS;
}

AppEcm g_AppEcm =
{
	.ecm_count  = 0,
    .start      = app_ecm_start,
    .stop       = app_ecm_stop,
    .get        = app_ecm_get,
    .analyse    = app_ecm_analyse,
    .storeinfo  = app_ecm_store_info,
    .init		= app_ecm_init,
    .timeout    = app_ecm_timeout,
};

#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
AppEcm g_AppEcm2 =
{
	.ecm_count  = 0,
    .start      = app_ecm_start,
    .stop       = app_ecm_stop,
    .get        = app_ecm_get,
    .analyse    = app_ecm_analyse,
    .storeinfo  = app_ecm_store_info,
    .init		= app_ecm_init,
    .timeout    = app_ecm_timeout,
};
#endif
#endif
/* End of file -------------------------------------------------------------*/


