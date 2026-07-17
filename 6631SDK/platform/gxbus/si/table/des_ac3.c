/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_ac3.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.10.10	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxcore.h"
#include "gxtype.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

#define MAX_AC3_DESCRIPTOR		12

extern int32_t des_table_memory_check_pmt_exit(PmtInfo *pmt_table,uint32_t max_size);
/**
 * @brief
 * @param
 * @Return
 */
static uint8_t parser_ac3_des(uint8_t *p_section_data, Ac3Descriptor *des_info_data, uint8_t *tail, uint16_t elem_pid)
{
//	uint8_t len = p_section_data[1];

	des_info_data->component_type_flag = (p_section_data[2]>>7);
	des_info_data->bsid_flag = ((p_section_data[2]>>6)&1);
	des_info_data->mainid_flag = ((p_section_data[2]>>5)&1);
	des_info_data->asvc_flag = ((p_section_data[2]>>4)&1);

	// get elementary pid
//	p_section_data -= 4;
	des_info_data->elem_pid = elem_pid;//TODATA13(p_section_data[0], p_section_data[1]);

	p_section_data += 3;

	if (des_info_data->component_type_flag == 1)
	{
		des_info_data->component_type = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->bsid_flag == 1)
	{
		des_info_data->bsid = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->mainid_flag == 1)
	{
		des_info_data->mainid = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->asvc_flag == 1)
	{
		des_info_data->asvc = *p_section_data;
		p_section_data++;
	}

	return 0;
}

int32_t des_pmt_ac3(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);
	uint16_t elem_pid = 0;
	uint8_t   des_len;

	GxSiDes_Trace();
	if(NULL== p_section_data)
	{
		return GXCORE_ERROR;
	}

	if(NULL == pmt_table->ac3_info){
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_AC3_DESCRIPTOR) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(uint32_t)*MAX_AC3_DESCRIPTOR);	//default max five ac3_des
		pmt_table->ac3_info= (uint32_t *)pmt_table->tail;
		pmt_table->ac3_count= 0;
	}

	if(pmt_table->stream_count > 0)
	{
		elem_pid = pmt_table->stream_info[pmt_table->stream_count-1].elem_pid;
	}

	if(pmt_table->ac3_count < MAX_AC3_DESCRIPTOR)
	{
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(Ac3Descriptor)) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(Ac3Descriptor));
		pmt_table->ac3_info[pmt_table->ac3_count] = (uint32_t)(pmt_table->tail);

		des_len = parser_ac3_des(p_section_data,
				(Ac3Descriptor*)(pmt_table->ac3_info[pmt_table->ac3_count]), pmt_table->tail,
				elem_pid);
		pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-des_len);

		pmt_table->ac3_count++;
	}
	else
	{
		gxlogd("\n[descriptor more than MAX_AC3_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_AC3_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
	}

	return GXCORE_SUCCESS;
}

static uint8_t parser_eac3_des(uint8_t *p_section_data, Eac3Descriptor *des_info_data, uint8_t *tail, uint16_t elem_pid)
{
//	uint8_t len = p_section_data[1];

	des_info_data->EAC3_type_flag = (p_section_data[2]>>7);
	des_info_data->bsid_flag = ((p_section_data[2]>>6)&1);
	des_info_data->mainid_flag = ((p_section_data[2]>>5)&1);
	des_info_data->asvc_flag = ((p_section_data[2]>>4)&1);
	des_info_data->mixinfoexists = ((p_section_data[2]>>3)&1);
	des_info_data->substream1_flag = ((p_section_data[2]>>2)&1);
	des_info_data->substream2_flag = ((p_section_data[2]>>1)&1);
	des_info_data->substream3_flag = ((p_section_data[2]>>0)&1);

	// get elementary pid
//	p_section_data -= 4;
	des_info_data->elem_pid = elem_pid;//TODATA13(p_section_data[0], p_section_data[1]);

	p_section_data += 3;

	if (des_info_data->EAC3_type_flag == 1)
	{
		des_info_data->EAC3_type = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->bsid_flag == 1)
	{
		des_info_data->bsid = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->mainid_flag == 1)
	{
		des_info_data->mainid = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->asvc_flag == 1)
	{
		des_info_data->asvc = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->substream1_flag == 1)
	{
		des_info_data->substream1 = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->substream2_flag == 1)
	{
		des_info_data->substream2 = *p_section_data;
		p_section_data++;
	}
	if (des_info_data->substream2_flag == 1)
	{
		des_info_data->substream2 = *p_section_data;
		p_section_data++;
	}

	return 0;
}

int32_t des_pmt_eac3(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);
	uint16_t elem_pid = 0;
	uint8_t   des_len =0;
	uint8_t  count = 0;

	GxSiDes_Trace();
	if((NULL== p_section_data) ||(len < 1))// for "p_section_data[2]"
	{
		return GXCORE_ERROR;
	}


	if(NULL == pmt_table->eac3_info){
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_AC3_DESCRIPTOR) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(uint32_t)*MAX_AC3_DESCRIPTOR);	//default max five ac3_des
		pmt_table->eac3_info= (uint32_t *)pmt_table->tail;
		pmt_table->eac3_count= 0;
	}

	if(pmt_table->stream_count > 0)
	{
		elem_pid = pmt_table->stream_info[pmt_table->stream_count-1].elem_pid;
	}

	if(pmt_table->eac3_count < MAX_AC3_DESCRIPTOR)
	{
		if(des_table_memory_check_pmt_exit(pmt_table,sizeof(Eac3Descriptor)) == GXCORE_ERROR)
		{
			return GXCORE_ERROR;
		}

		//((p_section_data[2]>>3) & 1)是mixinfoexists字段与substream 0有关
		//当前(EN 300 468 V1.15.1 03/2016)版本里eac3描述符里并无substream 0字段
		//所以计算count时不算在内
		count  = ((p_section_data[2]>>7) & 1) +
			    ((p_section_data[2]>>6) & 1) +
			    ((p_section_data[2]>>5) & 1) +
			    ((p_section_data[2]>>4) & 1) +
			    ((p_section_data[2]>>2) & 1) +
			    ((p_section_data[2]>>1) & 1) +
			    ((p_section_data[2]>>0) & 1);
		if(count > (len -1) )
		{
            //市场中存在非标准的eac3，竞争对手支持，所以我们把标准的eac3去掉来支
            //持非标准的eac3
			gxlogd("\n[descriptor  unstandard data (%d)]:%s,%s,%d\n",len, __FILE__,__FUNCTION__,__LINE__);
		}
		pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(Eac3Descriptor));
		pmt_table->eac3_info[pmt_table->eac3_count] = (uint32_t)(pmt_table->tail);

		des_len = parser_eac3_des(p_section_data,
				(Eac3Descriptor*)(pmt_table->eac3_info[pmt_table->eac3_count]), pmt_table->tail,
				elem_pid);

		pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-des_len);

		pmt_table->eac3_count++;
	}
	else
	{
		gxlogd("\n[descriptor more than MAX_EAC3_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_AC3_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
	}
	return GXCORE_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

