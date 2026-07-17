/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	gxsi.c
* Author    :	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0 	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "gxcore.h"
#include "gxmsg.h"
#include "gxservices.h"
#include "service/gxsi.h"
#include "module/si/si_public.h"
#include "module/si/si_filter.h"
#include "module/config/gxconfig.h"

//test begin ---------------------------------------
#include "gxos/gxcore_os_core.h"
#include "module/si/si_pmt.h"
#include "module/si/si_sdt.h"

extern void test_table(void);
extern uint32_t MAX_DEMUX_NUM;
extern uint32_t MAX_SI_FILTER_NUM;
#define VIRTUAL_DATA_SELF	(0)

//test end ------------------------------------------
/* Private types/constants ------------------------------------------------ */
#define GxSi_gxlogd(...)      //gxlogd( __VA_ARGS__ )
#define INVALID_ID	(-1)

#define FILTER_STOP		(0)
#define FILTER_START	(1)
/**
 * si subtable ctrl block
 */
struct si_subtable_ctrl
{
	int16_t      si_subtable_id;
	int16_t      si_filter_id;

	uint16_t     demux_id;
	uint16_t      filter_type;       ///< EQ  or  Not EQ
	uint32_t     crc_check;          ///< crc check or not

	uint16_t    ts_src;
	uint16_t    filter_status;     ///< filter stop 0   filter start 1

	uint32_t    time_out_ms;         ///<  time out , duration ms
	GxTime      time_out;            ///< subtable time out, the end time

	uint32_t	request_id;		///< 在"GXBUS_SI_MSG_CREATE"时每次加1, 供对应服务标识唯一

	private_table_cfg   table_parse_cfg;	///< private table parse funtion, null means use normal parse function

	uint8_t     section_state[32];   ///< used to mark if this section have received, per bit relatively per section
	uint32_t    section_count;      ///< when this value lange than last_section_number*3, report SUBTABLE_OK
};


/* Private Variables ------------------------------------------------------ */
#define MAX_SI_SUBTABLE_NUM	    MAX_SI_FILTER_NUM
#define SI_SUPPORT_CRC
// per bit record the s_SiSubtableStatus used status, 1 means in use
//static uint32_t s_SiSubtableStatus=0;

// SiSubtable control block, record the content of every SiSubtable
static struct si_subtable_ctrl  * s_SiSubtableBlk;

///< 在"GXBUS_SI_MSG_CREATE"时每次加1, 供对应服务标识唯一,0无效
static uint32_t s_RequestId = 1;

handle_t gx_si_realease_mutex = -1;
/* Private Functions ------------------------------------------------------ */
#if 0
/**
 * @brief get free si_subtable_id
 * @param void
 * @Return si_subtable_id
	   -1  invalid id
 */
static int16_t si_subtable_alloc(void)
{
	uint8_t i;

//	GxCore_MutexLock(si_mutex);

	for (i=0; i<MAX_SI_FILTER_NUM; i++)
	{
		if (((s_SiSubtableStatus>>i)&1) == 0)
		{
			s_SiSubtableStatus |= (1<<i);
			return i;
		}
	}

//	GxCore_MutexUnlock(si_mutex);

	return -1;
}

/**
 * @brief free si_subtable_id
 * @param si_subtable_id
 * @Return void
 */
static void si_subtable_GxCore_Free(int16_t si_subtable_id)
{
//	GxCore_MutexLock(si_mutex);

	if (si_subtable_id > MAX_SI_SUBTABLE_NUM-1)
	{
		return;
	}
	memset(&s_SiSubtableBlk[si_subtable_id], 0xff, sizeof(struct si_subtable_ctrl));
	memset(s_SiSubtableBlk[si_subtable_id].section_state, 0, sizeof(uint32_t)*8);

	s_SiSubtableStatus &= (~(1<<si_subtable_id));

//	GxCore_MutexUnlock(si_mutex);
}
#endif

static int32_t  si_subtable_init(void)
{
	uint32_t i , ret = 0;

	ret = GxBus_SiDmxInit();
	if(ret < 0)
	{
#ifdef SI_BASE_DEBUG
		GX_BUS_SI_DRIVER_ERRO_PRINTF("get hard params failed\n");
#endif
		return -1;
	}
	if(s_SiSubtableBlk != NULL)
	{
		GxCore_Free(s_SiSubtableBlk);
		s_SiSubtableBlk = NULL;
	}
	s_SiSubtableBlk = GxCore_Mallocz(sizeof(struct si_subtable_ctrl) * MAX_SI_SUBTABLE_NUM);
	if(s_SiSubtableBlk == NULL)
	{
#ifdef SI_BASE_DEBUG
		GX_BUS_SI_DRIVER_ERRO_PRINTF("s_SiSubtableBlk malloc failed\n");
#endif
		return -2;
	}

	for (i=0; i<MAX_SI_SUBTABLE_NUM; i++)
	{
		memset(&s_SiSubtableBlk[i], 0xff, sizeof(struct si_subtable_ctrl));
		memset(s_SiSubtableBlk[i].section_state, 0, sizeof(uint32_t)*8);

		s_SiSubtableBlk[i].si_filter_id = INVALID_ID;
		s_SiSubtableBlk[i].si_subtable_id = INVALID_ID;
	}
	return 0;
}

/**
 * @brief free si_subtable_record
 * @param si_subtable_id
 * @Return void
 */
static void si_subtable_record(int16_t si_subtable_id, GxMsgProperty_SiCreate p_si_subtable)
{
	struct si_subtable_ctrl *p_subt_ctr = &s_SiSubtableBlk[p_si_subtable->si_subtable_id];

	p_subt_ctr->ts_src = p_si_subtable->ts_src;
	p_subt_ctr->request_id = p_si_subtable->request_id;
	p_subt_ctr->demux_id = p_si_subtable->demux_id;
	p_subt_ctr->si_subtable_id = p_si_subtable->si_subtable_id;

	// filter_id and subtable_id is the same, from Demux module
	p_subt_ctr->si_filter_id = p_si_subtable->si_subtable_id;

	p_subt_ctr->time_out_ms = p_si_subtable->time_out;
	p_subt_ctr->filter_type = p_si_subtable->si_filter.eq_or_neq;
	p_subt_ctr->table_parse_cfg = p_si_subtable->table_parse_cfg;
	p_subt_ctr->crc_check = p_si_subtable->crc_check;

	// init count as 0, this function only do in create
	p_subt_ctr->section_count = 0;
	memset(p_subt_ctr->section_state, 0, 8*sizeof(uint32_t));
}

/**
 * @brief free si_subtable_clear
 * @param si_subtable_id
 * @Return void
 */
static void si_subtable_clear(int16_t si_subtable_id)
{
	if (si_subtable_id > MAX_SI_SUBTABLE_NUM-1||si_subtable_id == -1)
	{
		return;
	}
	memset(&s_SiSubtableBlk[si_subtable_id], 0xff, sizeof(struct si_subtable_ctrl));
	memset(s_SiSubtableBlk[si_subtable_id].section_state, 0, sizeof(uint32_t)*8);

	s_SiSubtableBlk[si_subtable_id].si_filter_id = INVALID_ID;
	s_SiSubtableBlk[si_subtable_id].si_subtable_id = INVALID_ID;
}


/**
 * @brief get si_subtable_id
 * @param si_filter_id
 * @Return si_subtable_id
 * @Return -1   error
 */
static int16_t si_subtable_get_id(int16_t si_filter_id)
{
	uint16_t i;

	//	GxCore_MutexLock(si_mutex);

	for (i=0; i<MAX_SI_SUBTABLE_NUM; i++)
	{
		if (s_SiSubtableBlk[i].si_filter_id == si_filter_id)
		{
			return i;
		}
	}

	//	GxCore_MutexUnlock(si_mutex);

	return -1;
}


static GxMsgID si_eit_table_status_get(struct si_subtable_ctrl *p_subt_ctr,
										si_info_t *p_si_info, uint16_t total_section)
{
	uint16_t i;
	uint16_t dummy_sec_per_segment;


	dummy_sec_per_segment = (0x7 - (p_si_info->segment_last_sec_number & 0x7));

	// section get, set the bit 1
	p_subt_ctr->section_state[p_si_info->sec_number/8] |= (1<<(p_si_info->sec_number%8));

	// set dummy section 1
	for(i=0; i<dummy_sec_per_segment; i++)
	{
		p_subt_ctr->section_state[p_si_info->sec_number/8] |= (1<<(7-i));
	}

	for (i=0; i<total_section; i++)
	{
		if (((p_subt_ctr->section_state[i/8]>>(i%8))&1) != 1)
		{
			return GXMSG_SI_SECTION_OK;
		}
	}

	GxBus_SiFilterStop(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);//subtable ok need stop si filter
	GxSi_gxlogd("GXMSG_SI_SUBTABLE_OK\n");
	p_subt_ctr->filter_status = FILTER_STOP;
	return GXMSG_SI_SUBTABLE_OK;
}

// count 0-reset counter 0xffff-only get return MsgID
GxMsgID si_eit_section_set(uint32_t count)
{
	static uint32_t sec_count = 0;
	int32_t gate = 0;

	if (count == 0)
		sec_count = 0;

	GxBus_ConfigGetInt(SI_CONFIG_EIT_GATE,&gate,1000);
	if (sec_count==gate)
	{
		sec_count=0;
		return GXMSG_SI_SUBTABLE_OK;
	}

	sec_count++;
	return GXMSG_SI_SECTION_OK;
}

/**
 * @brief get the subtable status
 * @param
 * @Return GXMSG_SI_SECTION_OK
 GXMSG_SI_SUBTABLE_OK
 */
static uint32_t si_subtable_status_get(struct si_subtable_ctrl *p_subt_ctr, si_info_t *p_si_info)
{
#define FORCE_SUBTABLE_OK_TIMES (3)
	const GxSiFilter *p_si_filter;
	uint16_t total_section;

	/*the two point never be NULL, so needn't to check them*/

	p_si_filter = GxBus_SiFilterGet(p_subt_ctr->si_filter_id);

	total_section = p_si_info->last_sec_number+1;

	if ((p_si_filter->match[0] >= 0x4E) && (p_si_filter->match[0] <= 0x6F))
	{
		if ((p_si_filter->mask[3]==0xff)&&(p_si_filter->mask[4]==0xff))
		{
			return si_eit_table_status_get(p_subt_ctr, p_si_info, total_section);
		}
		else
		{
			return si_eit_section_set(0xffff);
#if 0
			static uint32_t record_first = 0;
			static uint32_t service_id = 0;
			GxMsgID msg_id;
			if (p_si_filter->match[0] == 0x4E
					|| ((p_si_filter->match[0] >= 0x50) && (p_si_filter->match[0] <= 0x5F)))
			{
				// 全速过滤的当前频点，取出第一个过滤到的service id
				if (record_first == 0) {
					service_id = (p_si_filter->match[3]<<8 | p_si_filter->match[4]);
					record_first++;
				}

				if (service_id == (p_si_filter->match[3]<<8 | p_si_filter->match[4]))
				{
					msg_id = si_eit_table_status_get(p_subt_ctr, p_si_info, total_section);
					if (msg_id == GXMSG_SI_SUBTABLE_OK)
					{
						record_first = 0;
						service_id = 0;
					}

					return msg_id;
				}
			}
			else
			{
				return GXMSG_SI_SECTION_OK;
			}
#endif
		}
	}
	/*else if (p_si_filter->match[0]== TDT_TID)
	  {
	  GxBus_SiFilterStop(p_subt_ctr->si_filter_id);//subtable ok need stop si filter
	  p_subt_ctr->filter_status = FILTER_STOP;
	  return GXMSG_SI_SUBTABLE_OK;
	  }*/
	else
	{
		uint16_t i;

		p_subt_ctr->section_state[p_si_info->sec_number/8] |= (1<<(p_si_info->sec_number%8));

		p_subt_ctr->section_count++;
		if (p_subt_ctr->section_count < p_si_info->last_sec_number*FORCE_SUBTABLE_OK_TIMES)
		{
			for (i=0; i<total_section; i++)
			{
				if (((p_subt_ctr->section_state[i/8]>>(i%8))&1) != 1)
				{
					return GXMSG_SI_SECTION_OK;
				}
			}
		}

		GxBus_SiFilterStop(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);//subtable ok need stop si filter
		p_subt_ctr->filter_status = FILTER_STOP;
		return GXMSG_SI_SUBTABLE_OK;
	}

	// this return no means, need a return value
	return GXMSG_SI_SUBTABLE_OK;
}


/**
 * @brief
 * @param
 * @Return
 */
static void si_service_subtable_get(GxMsgProperty_SiGet p_si_subtable)
{
	struct si_subtable_ctrl *p_subt_ctr;

	if (p_si_subtable == NULL) {
		return;
	}

	p_subt_ctr = &s_SiSubtableBlk[p_si_subtable->si_subtable_id];

	p_si_subtable->ts_src = p_subt_ctr->ts_src;
	p_si_subtable->demux_id = p_subt_ctr->demux_id;
	p_si_subtable->table_parse_cfg = p_subt_ctr->table_parse_cfg;
	p_si_subtable->time_out = p_subt_ctr->time_out_ms;
	p_si_subtable->request_id = p_subt_ctr->request_id;
	memcpy(&(p_si_subtable->si_filter), GxBus_SiFilterGet(p_subt_ctr->si_filter_id), sizeof(GxSiFilter));
}

/**
 * @brief
 * @param
 * @Return
 */
static status_t si_check_tid(uint8_t tid)
{
	uint32_t i = 0;
	status_t ret = GXCORE_ERROR;
	uint8_t support_tid[] = {PAT_TID, CAT_TID, PMT_TID, NIT_ACTUAL_NETWORK_TID, NIT_OTHER_NETWORK_TID,
		SDT_ACTUAL_TS_TID, SDT_OTHER_TS_TID, BAT_TID, EIT_ACTUAL_PF_TID, EIT_OTHER_PF_TID, TDT_TID, TOT_TID};

	for (i=0; i<sizeof(support_tid); i++)
	{
		if (tid == support_tid[i])
		{
			ret = GXCORE_SUCCESS;
			break;
		}
	}

	if (tid>=0x50 && tid<=0x6f)
	{
		ret = GXCORE_SUCCESS;
	}

	return ret;
}

uint32_t dvb_crc32(uint8_t* pBuffer, uint32_t nSize);
static int32_t si_section_filter_crc32_check(uint8_t *pSection)
{
	uint16_t nlength = 0;
	uint32_t nCrc32 = 0;
	uint32_t nCrc32Result = 0;
	uint8_t *pdata = NULL;
	uint8_t *pndata = NULL;
	uint8_t	chTableId = 0;
	uint8_t	chSectionNumber          = 0;
	//uint8_t	chLastSectionNumber     = 0;

	pdata					= pSection;
	chTableId				= pdata[0];
	nlength					= (uint16_t)((pdata[1]&0x0F)<<8)|pdata[2];
	chSectionNumber			= pdata[6];
	//chLastSectionNumber		= pdata[7];


	/*TDT表没CRC校验*/
	if(0x70 == chTableId)
		return 0;
	else if ((chTableId >= 0x80) && (chTableId <= 0xFE)){
		if(((pdata[1])&0x80) == 0)
			return 0;
	}

	if(nlength<1)
		return 1;
	nlength = nlength + 3 - 4;
	nCrc32Result = dvb_crc32(pdata, nlength);

	pndata = pdata + nlength;
	nCrc32 = ((pndata[0]<<24)&0xff000000) | ((pndata[1]<<16)&0x00ff0000)
		| ((pndata[2]<<8)&0x0000ff00) | ((pndata[3]<<0)&0x000000ff);

	if(nCrc32Result == nCrc32)
		return  0;
	else{
		GxSi_gxlogd("[SI] [table_id]=0x%x [length]=0x%x  [nCrc32Result]=0x%x  [nCrc32]=0x%x \n"
				,chTableId, nlength, nCrc32Result, nCrc32);
		return  1;
	}
}

static status_t si_service_subtable_create(GxMsgProperty_SiCreate p_si_subtable)
{
	int16_t si_filter_id;

	if (p_si_subtable == NULL) {
		return GXCORE_ERROR;
	}

	if (((p_si_subtable->table_parse_cfg.mode==PARSE_WITH_STANDARD)
				|| (p_si_subtable->table_parse_cfg.mode==PARSE_STANDARD_ONLY))
			&& (si_check_tid(p_si_subtable->si_filter.match[0])!=GXCORE_SUCCESS))
	{
		return GXCORE_ERROR;
	}

	// get si_filter_id from av_core
	si_filter_id = GxBus_SiFilterCreate(p_si_subtable->ts_src, p_si_subtable->demux_id, &(p_si_subtable->si_filter));
	if (si_filter_id == -1)
	{
		// when failed let other service konw
		p_si_subtable->si_subtable_id = -1;
		return GXCORE_ERROR;
	}

	// get from si filter create , si_subtable_id same as si_filter_id
	p_si_subtable->si_subtable_id = si_filter_id;
	p_si_subtable->request_id = s_RequestId++;

	// set internal control block
	si_subtable_record(p_si_subtable->si_subtable_id, p_si_subtable);

	return GXCORE_SUCCESS;
}

/**
 * @brief
 * @param
 * @Return
 */
static status_t si_service_subtable_modify(GxMsgProperty_SiModify p_si_subtable)
{
	struct si_subtable_ctrl *p_subt_ctr;

	if (p_si_subtable == NULL) {
		return GXCORE_ERROR;
	}

	if (p_si_subtable->si_subtable_id > MAX_SI_SUBTABLE_NUM || p_si_subtable->si_subtable_id == -1)
	{
		return GXCORE_ERROR;
	}

	p_subt_ctr = &s_SiSubtableBlk[p_si_subtable->si_subtable_id];

	GxCore_GetTickTime(&(p_subt_ctr->time_out));

	p_subt_ctr->ts_src = p_si_subtable->ts_src;
	p_subt_ctr->demux_id = p_si_subtable->demux_id;
	p_subt_ctr->si_subtable_id = p_si_subtable->si_subtable_id;
	p_subt_ctr->time_out_ms = p_si_subtable->time_out;
	p_subt_ctr->filter_type = p_si_subtable->si_filter.eq_or_neq;
	p_subt_ctr->table_parse_cfg = p_si_subtable->table_parse_cfg;
	p_subt_ctr->time_out.seconds += p_subt_ctr->time_out_ms/1000;//如果modify的时候不把time out 还原 有可能 一直超时
	p_subt_ctr->time_out.microsecs += (p_subt_ctr->time_out_ms%1000)*1000;
	p_subt_ctr->section_count = 0; // recount section count
	memset(p_subt_ctr->section_state, 0, 8*sizeof(uint32_t));

	// si_filter_id will not change
	GxBus_SiFilterModify(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id, &(p_si_subtable->si_filter));

	// 有可能在stop状态进行modify,所以必须重新start.
	p_subt_ctr->filter_status = FILTER_START;

	return GXCORE_SUCCESS;
}

/**
 * @brief
 * @param
 * @Return
 */
static status_t si_service_subtable_release(GxMsgProperty_SiRelease si_subtable_id)
{
	//GxMessage *msg;
	//GxMsgProperty_SiReleaseOk *p_release_ok;
	//uint32_t request_id_record;

	GxCore_MutexLock(gx_si_realease_mutex);//补丁，避免后台使用控制块时被释放
	struct si_subtable_ctrl *p_subt_ctr = &s_SiSubtableBlk[si_subtable_id];

	if (p_subt_ctr->si_subtable_id != si_subtable_id || si_subtable_id == -1)
	{
		GxSi_gxlogd("si_subtable_id err = %d!\n",si_subtable_id);

		GxCore_MutexUnlock(gx_si_realease_mutex);
		return GXCORE_ERROR;
	}
	GxSi_gxlogd("si_service_subtable_release!\n");

	// record request id first, will free it in "GxBus_SiFilterDestroy"
	//request_id_record = p_subt_ctr->request_id;

	GxBus_SiFilterDestroy(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);


	si_subtable_clear(si_subtable_id);
	GxCore_MutexUnlock(gx_si_realease_mutex);

	return GXCORE_SUCCESS;
}

/**
 * @brief
 * @param
 * @Return
 */
static status_t si_service_subtable_start(GxMsgProperty_SiStart si_subtable_id)
{
	struct si_subtable_ctrl *p_subt_ctr = &s_SiSubtableBlk[si_subtable_id];

	if (p_subt_ctr->si_subtable_id != si_subtable_id || si_subtable_id == -1)
	{
		return GXCORE_ERROR;
	}

	GxCore_GetTickTime(&(p_subt_ctr->time_out));
	p_subt_ctr->time_out.seconds += p_subt_ctr->time_out_ms/1000;
	p_subt_ctr->time_out.microsecs += (p_subt_ctr->time_out_ms%1000)*1000;
	GxBus_SiFilterStart(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);

	p_subt_ctr->filter_status = FILTER_START;

	return GXCORE_SUCCESS;
}


/**
 * @brief
 * @param
 * @Return
 */
static status_t si_service_subtable_stop(GxMsgProperty_SiStop si_subtable_id)
{
	struct si_subtable_ctrl *p_subt_ctr = &s_SiSubtableBlk[si_subtable_id];

	if (p_subt_ctr->si_subtable_id != si_subtable_id || si_subtable_id == -1)
	{
		return GXCORE_ERROR;
	}

	GxBus_SiFilterStop(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);

	p_subt_ctr->filter_status = FILTER_STOP;

	return GXCORE_SUCCESS;
}

/*static status_t si_table_len_protect(uint8_t tid, uint16_t sec_len)
{
	#define SECTIN_LEN_1K		(1024)
	#define SECTIN_LEN_4K		(4096)

	uint32_t sec_standard_len;

	if (tid == PMT_TID
		|| tid == PAT_TID
		|| tid == SDT_ACTUAL_TS_TID
		|| tid == SDT_OTHER_TS_TID)
	{
		sec_standard_len = SECTIN_LEN_1K;
	}
	else if (tid == EIT_ACTUAL_PF_TID
		||tid == EIT_OTHER_PF_TID)
	{
		sec_standard_len = SECTIN_LEN_4K;
	}
	else
	{
		sec_standard_len = SECTIN_LEN_1K;
	}

	if (sec_len > sec_standard_len)
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}*/

/* Exported Functions ----------------------------------------------------- */
/*void GxSi_SyncTimeDo(void)
{
	uint32_t i;
	struct si_subtable_ctrl *p_subt_ctr;

	for (i=0; i<MAX_SI_SUBTABLE_NUM; i++)
	{
		p_subt_ctr = &s_SiSubtableBlk[i];

		if (p_subt_ctr->si_subtable_id != INVALID_ID)
		{
			GxCore_GetTickTime(&(p_subt_ctr->time_out));
			p_subt_ctr->time_out.seconds += p_subt_ctr->time_out_ms/1000;
			p_subt_ctr->time_out.microsecs += (p_subt_ctr->time_out_ms%1000)*1000;
		}
	}
}*/


status_t  GxSiServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;
	int32_t ret = 0;

	GxBus_SiFilterOpenAv();

	if (GxBus_SiParserBufCreate() != GXCORE_SUCCESS) {
		return GXCORE_ERROR;
	}

	/*if (GxCore_MutexCreate(&si_mutex) != GXCORE_SUCCESS) {
	  return GXCORE_ERROR;
	  }*/

	// init "s_SiSubtableBlk"
	//memset(s_SiSubtableBlk, 0xff, sizeof(struct si_subtable_ctrl)*MAX_SI_SUBTABLE_NUM);
	ret = si_subtable_init();
	if(ret < 0)
	{
#ifdef SI_BASE_DEBUG
		GX_BUS_SI_DRIVER_ERRO_PRINTF("subtable init failed\n");
#endif
		return GXCORE_ERROR;
	}

	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_GET,        sizeof(GxMsgProperty_SiGet)       );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_CREATE,     sizeof(GxMsgProperty_SiCreate)    );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_MODIFY,     sizeof(GxMsgProperty_SiModify)    );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_RELEASE,    sizeof(GxMsgProperty_SiRelease)   );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_START,      sizeof(GxMsgProperty_SiStart)     );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_STOP,       sizeof(GxMsgProperty_SiStop)      );
	GxBus_MessageRegister(GXMSG_SI_SECTION_OK_FOR_TEST, sizeof(GxParseResult)             );
	GxBus_MessageRegister(GXMSG_SI_SECTION_OK,          sizeof(GxMsgProperty_SiSectionOk)             );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_OK,         sizeof(GxMsgProperty_SiSubtableOk)             );
	//	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_RELEASE_OK, sizeof(GxMsgProperty_SiReleaseOk) );
	GxBus_MessageRegister(GXMSG_SI_SUBTABLE_TIME_OUT,   sizeof(GxMsgProperty_SiTimeout)   );
	GxBus_MessageRegister(GXMSG_PLAYER_PLAY,            sizeof(GxMsgProperty_PlayerPlay)  );

	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_GET);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_CREATE);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_MODIFY);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_RELEASE);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_START);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_STOP);
	GxBus_MessageListen(self, GXMSG_SI_SECTION_OK_FOR_TEST);
	GxBus_MessageListen(self, GXMSG_PLAYER_PLAY);

	GxBus_SiFilterSemInit();
	GxCore_MutexCreate(&gx_si_realease_mutex);

	sch = GxBus_SchedulerCreate("SiMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("SiConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+1+priority_offset);
	GxBus_ServiceLink(self, sch);
	return GXCORE_SUCCESS;
}

void GxSiServiceDestroy(handle_t self)
{
	GxBus_SiParserBufRelease();
	GxBus_SiFilterCloseAv();

	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_GET);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_CREATE);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_MODIFY);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_RELEASE);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_START);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_STOP);
	GxBus_MessageUnListen(self, GXMSG_SI_SECTION_OK_FOR_TEST);
	GxBus_MessageUnListen(self, GXMSG_PLAYER_PLAY);

	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_GET);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_CREATE);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_MODIFY);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_RELEASE);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_START);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_STOP);
	GxBus_MessageUnregister(GXMSG_SI_SECTION_OK_FOR_TEST);
	GxBus_MessageUnregister(GXMSG_SI_SECTION_OK);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_OK);
	//	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_RELEASE_OK);
	GxBus_MessageUnregister(GXMSG_SI_SUBTABLE_TIME_OUT);
	GxBus_MessageUnregister(GXMSG_PLAYER_PLAY);

	GxBus_ServiceUnlink(self);
	GxBus_SiFilterSemDel();
	GxCore_MutexDelete(gx_si_realease_mutex);
	return;
}


GxMsgStatus GxSiServiceRecvMsg(handle_t self, GxMessage* Msg)
{
	switch(Msg->msg_id)
	{
		case GXMSG_SI_SUBTABLE_GET:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_GET\n");
			si_service_subtable_get(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiGet));
			break;
		case GXMSG_SI_SUBTABLE_CREATE:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_CREATE\n");
			si_service_subtable_create(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiCreate));
			break;
		case GXMSG_SI_SUBTABLE_MODIFY:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_MODIFY\n");
			si_service_subtable_modify(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiModify));
			break;
		case GXMSG_SI_SUBTABLE_RELEASE:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_RELEASE\n");
			si_service_subtable_release(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiRelease));
			break;
		case GXMSG_SI_SUBTABLE_START:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_START\n");
			si_service_subtable_start(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiStart));

			break;
		case GXMSG_SI_SUBTABLE_STOP:
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_STOP\n");
			si_service_subtable_stop(*GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_SiStop));
			break;
		case GXMSG_PLAYER_PLAY:
			si_eit_section_set(0);
			break;
#if VIRTUAL_DATA_SELF
		case GXMSG_SI_SECTION_OK_FOR_TEST:
			{
				GxMsgProperty_SiSectionOk* parse_result;

				parse_result = GxBus_GetMsgPropertyPtr(Msg, GxMsgProperty_SiSectionOk);

				GxSi_gxlogd("tid:  0x%x\n",parse_result->table_id);
				GxSi_gxlogd("----------------------------------------------\n");
				if (parse_result->table_id == 2)
				{
					GxSi_gxlogd("vpid : %d\napid : %d\npcr pid : %d\n",
							((PmtInfo*)(parse_result->parsed_data))->stream_info[0].elem_pid,
							((PmtInfo*)(parse_result->parsed_data))->stream_info[1].elem_pid,
							((PmtInfo*)(parse_result->parsed_data))->pcr_pid);
				}
				else if (parse_result->table_id == 0x42)
				{
					GxSi_gxlogd("prog name: %s\n",((SdtInfo*)(parse_result->parsed_data))->service_info->service_des_info->service_name);
				}
				else
				{}
				GxSi_gxlogd("----------------------------------------------\n");
			}
			break;
#endif
		default:
			break;
	}

	return GXMSG_OK;
}

void GxSiServiceConsole(handle_t self)
{
#if (VIRTUAL_DATA_SELF == 0)
	uint8_t *parse_data_buf = NULL;
	int32_t per_read_size = 0;
	GxMessage *reply_msg;
	GxParseResult *parse_result;
	void *parsed_data;
	uint64_t si_filter_status =0;
	uint32_t ts_lock_flag = 0;
	struct si_subtable_ctrl *p_subt_ctr;
	int16_t si_filter_id;
	int16_t si_subtable_id;
	uint16_t read_size;
	uint16_t sec_len;
	uint16_t i;
	uint32_t message_id = 0;
	GxTime local_time;

	GxBus_SiFilterSemWait();
	// 1 Query channel
	for (i=0; i<MAX_DEMUX_NUM; i++)
	{
		uint64_t status = 0;

		if (GXCORE_SUCCESS == GxBus_SiFilterQuery(i, &status))
		{
			si_filter_status |= status;
			ts_lock_flag = 1;
		}
	}

	if (ts_lock_flag == 0)
	{
		// ts all unlock
		return;
	}
	for (si_filter_id=0; si_filter_id<MAX_SI_FILTER_NUM; si_filter_id++)
	{
		GxCore_MutexLock(gx_si_realease_mutex);

		// get "p_subt_ctr"
		si_subtable_id = si_subtable_get_id(si_filter_id);
		if (si_subtable_id != -1)
		{
			//GxSi_gxlogd("find the subtable blk!\n");
			p_subt_ctr = &s_SiSubtableBlk[si_subtable_id];

			if (p_subt_ctr->filter_status == FILTER_STOP) {
				GxSi_gxlogd("continue filter stop!!!!\n");
				GxCore_MutexUnlock(gx_si_realease_mutex);
				continue;
			}
		}
		else
		{
			GxSi_gxlogd("error: can't find the subtable blk!\n");
			GxCore_MutexUnlock(gx_si_realease_mutex);
			continue;
		}

		// TODO: deal time out need put here !!! 或者在"none filter get data"每次查一个通道是否超时
		//time out

		GxCore_GetTickTime(&local_time);
		if (p_subt_ctr->time_out.seconds <= local_time.seconds)
		{
			reply_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_TIME_OUT);
			if(reply_msg == NULL) {
				GxSi_gxlogd("\n\nmsg err %s : %d\n\n",__FUNCTION__,__LINE__);
				GxCore_MutexUnlock(gx_si_realease_mutex);
				return;
			}

			parse_result = GxBus_GetMsgPropertyPtr(reply_msg,GxParseResult);
			parse_result->table_id = GxBus_SiFilterGet(si_filter_id)->match[0];
			parse_result->si_subtable_id = si_subtable_id;
			parse_result->request_id = p_subt_ctr->request_id;
			parse_result->parsed_data = NULL;

			GxBus_SiFilterStop(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);//subtable ok need stop si filter
			p_subt_ctr->filter_status = FILTER_STOP;
			GxSi_gxlogd("GXMSG_SI_SUBTABLE_TIME_OUT si_subtable_id = %d\n",si_filter_id);
			GxBus_MessageSend(reply_msg);
			GxCore_MutexUnlock(gx_si_realease_mutex);
			continue;
		}

		GxSi_gxlogd("si_filter_status = %d!\n",si_filter_status);
		if (((si_filter_status>>si_filter_id)&1ULL) == 1ULL)
		{
			// sifilter already get data
			GxBus_ConfigGetInt(SI_CONFIG_PER_READ_SIZE, &per_read_size, 64 * 1024);

			parse_data_buf = GxCore_Mallocz(per_read_size);
			if (parse_data_buf == NULL) {
				GxCore_MutexUnlock(gx_si_realease_mutex);
				return;
			}

			read_size = GxBus_SiFilterRead(p_subt_ctr->demux_id, si_filter_id, parse_data_buf, per_read_size);
			GxSi_gxlogd("read_size --- %d\n",read_size);

			// demux get data, but data len is 0, so needn't advance time, let it timeout
			if (read_size != 0)
			{
				// update terminal time
				p_subt_ctr->time_out.seconds = local_time.seconds + p_subt_ctr->time_out_ms/1000;
				p_subt_ctr->time_out.microsecs = local_time.microsecs + (p_subt_ctr->time_out_ms%1000)*1000;
			}

			// compare TID need "& mask_value"
			const GxSiFilter* p = GxBus_SiFilterGet(si_filter_id);
			for (i=0; i<read_size;)
			{
				if ((p->match[0]&p->mask[0])
						!= (parse_data_buf[i]&p->mask[0]))// compare tid
				{
					GxSi_gxlogd("demux hw get data err! si_filter_id = %d\n",si_filter_id);
					GxCore_Free(parse_data_buf);
					si_service_subtable_stop(p_subt_ctr->si_subtable_id);
					si_service_subtable_start(p_subt_ctr->si_subtable_id);
					GxCore_MutexUnlock(gx_si_realease_mutex);
					return;
				}

				sec_len = TODATA12(parse_data_buf[i+1],parse_data_buf[i+2]);



				// section length protect 100819 shenbin
				/*if (si_table_len_protect(parse_data_buf[i], sec_len) != GXCORE_SUCCESS)
				  {
				  si_service_subtable_stop(p_subt_ctr->si_subtable_id);
				  si_service_subtable_start(p_subt_ctr->si_subtable_id);
				  return;
				  }*/

#ifdef  SI_SUPPORT_CRC
				if (CRC_OFF == p_subt_ctr->crc_check || si_section_filter_crc32_check(parse_data_buf+i) ==0 )
					parsed_data = GxBus_SiParser(&(p_subt_ctr->table_parse_cfg),
							&parse_data_buf[i], sec_len+3, si_subtable_id);
				else
				{
					parsed_data = SEC_LEN_ERROR;
				}
#else
				parsed_data = GxBus_SiParser(&(p_subt_ctr->table_parse_cfg),
						&parse_data_buf[i], sec_len+3, si_subtable_id);
#endif


				// for standard parse
				if (parsed_data == SEC_NO_MEMORY)
				{
					gxlogd("have no more mempool for store!\n");
					break;
				}
				else if (parsed_data == SEC_SYNTAX_INDICATOR)
				{
					// offset to next section data
					//i += (sec_len+3);
					//continue;
					// if syntax indicator error, make it time out, abandon it. shenbin 110406
					GxCore_Free(parse_data_buf);
					p_subt_ctr->time_out.seconds = 0;
					GxCore_MutexUnlock(gx_si_realease_mutex);
					return;
				}
				else if (parsed_data == SEC_LEN_ERROR)
				{
					si_service_subtable_stop(p_subt_ctr->si_subtable_id);
					si_service_subtable_start(p_subt_ctr->si_subtable_id);

					// if analyse table error, make it time out, abandon it. shenbin 110321
					GxCore_Free(parse_data_buf);
					p_subt_ctr->time_out.seconds = 0;
					GxCore_MutexUnlock(gx_si_realease_mutex);
					return;
				}
				else
				{}

				if (p_subt_ctr->table_parse_cfg.mode == PARSE_SECTION_ONLY)
				{
					// for section only
					message_id = GXMSG_SI_SECTION_OK;
				}
				else if (p_subt_ctr->table_parse_cfg.mode == PARSE_PRIVATE_ONLY)
				{
					// for private parse
					if (parsed_data == SEC_SECTION_OK)
					{
						message_id = GXMSG_SI_SECTION_OK;
						parsed_data = NULL;
					}
					else if (parsed_data == SEC_SUBTABLE_OK)
					{
						message_id = GXMSG_SI_SUBTABLE_OK;
						parsed_data = NULL;

						GxBus_SiFilterStop(p_subt_ctr->demux_id, p_subt_ctr->si_filter_id);//subtable ok need stop si filter
						p_subt_ctr->filter_status = FILTER_STOP;
					}
					else{}

				}
				else
				{
					// for standard parse
					message_id = si_subtable_status_get(p_subt_ctr, (si_info_t*)(parsed_data));
				}

				reply_msg = GxBus_MessageNew(message_id);
				GxSi_gxlogd("message_id --- 0x%x\nreply --- 0x%x\n",message_id,(uint32_t)reply_msg);
				if(reply_msg == NULL)
				{
					GxSi_gxlogd("\n\nmsg err %s : %d\n\n",__FUNCTION__,__LINE__);
					GxCore_Free(parse_data_buf);
					GxCore_MutexUnlock(gx_si_realease_mutex);
					return;
				}

				parse_result = GxBus_GetMsgPropertyPtr(reply_msg,GxParseResult);

				parse_result->parsed_data = (uint8_t*)parsed_data;
				parse_result->table_id = GET_TABLE_ID(parse_data_buf);
				parse_result->si_subtable_id = si_subtable_id;
				parse_result->request_id = p_subt_ctr->request_id;

				GxSi_gxlogd("[si]   send section(subtable) ok!\n");
				GxBus_MessageSend(reply_msg);
				GxCore_ThreadYield();

				if (message_id == GXMSG_SI_SUBTABLE_OK)
				{
					// subtable ok , needn't to analyse the rest data
					break;
				}
				// offset to next section data
				i += (sec_len+3);
			}
			GxCore_Free(parse_data_buf);
			parse_data_buf = NULL;
		}
		GxCore_MutexUnlock(gx_si_realease_mutex);

	}
	// 2 get channel data,save to buffer
	// 3 judge the data, section or subtable ok
	// 4 send message

	// section save buffer 5  ->  subtable ok .  total 6 sec

#else
	test_table();
	while(1);
#endif
}

GxServiceClass si_service = {
	"si service",
	GxSiServiceInit,
	GxSiServiceDestroy,
	GxSiServiceRecvMsg,
	GxSiServiceConsole
};

/* End of file -------------------------------------------------------------*/

