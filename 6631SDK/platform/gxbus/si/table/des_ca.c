/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_service_descrip.c
* Author    : 	shenbin
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_pmt.h"

#define MAX_CA_DESCRIPTOR		64

int32_t des_table_memory_check_pmt_exit(PmtInfo *pmt_table,uint32_t max_size)
{
	uint8_t *head = NULL;
	uint8_t *ptr = NULL;
	
	ptr = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-max_size);	//default max five ca_des
	head = (uint8_t*)((uint32_t)(pmt_table->buf)+(pmt_table->stream_count)*sizeof(stream_info_t));
	// make sure the pmt info will not overflow the pmt_buf
	if(head > ptr)
	{
		gxlogd("[pmt]ERR: PMT table buf overflow!------[%s]----------\n",__FUNCTION__);
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

/**
 * @brief
 * @param
 * @Return
 */
static uint8_t parser_ca_des(uint8_t *p_section_data, CaDescriptor *des_info_data, uint8_t *tail, uint16_t elem_pid)
{
	if(p_section_data[1] < 4)
	{
		return 0;
	}

	des_info_data->private_data_len = p_section_data[1]-4;
	des_info_data->ca_system_id = TODATA16(p_section_data[2], p_section_data[3]);
	des_info_data->ca_pid = TODATA13(p_section_data[4], p_section_data[5]);

	// get elementary pid
	des_info_data->elem_pid = elem_pid;

	p_section_data += 6;

	if(des_info_data->private_data_len>0)
	{
		tail = (uint8_t *)((uint32_t)tail-(des_info_data->private_data_len+3)/4*4);
		des_info_data->private_data = tail;
		memcpy(des_info_data->private_data,p_section_data,des_info_data->private_data_len);
	}

	return  ((des_info_data->private_data_len+3)/4*4);
}

int32_t des_pmt_ca(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);
	uint8_t   des_len;
	

	GxSiDes_Trace();
	if(NULL== p_section_data)
	{
		return GXCORE_ERROR;
	}
	if(pmt_table->stream_count>0)
	{
		if(NULL == pmt_table->ca_info2){
			if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_CA_DESCRIPTOR) == GXCORE_ERROR)
			{
				return GXCORE_ERROR;
			}
			pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(uint32_t)*MAX_CA_DESCRIPTOR);
			pmt_table->ca_info2 = (uint32_t *)pmt_table->tail;
			pmt_table->ca_count2 = 0;
		}
		if(pmt_table->ca_count2 <  MAX_CA_DESCRIPTOR)
		{
			if(des_table_memory_check_pmt_exit(pmt_table,sizeof(CaDescriptor)) == GXCORE_ERROR)
			{
				return GXCORE_ERROR;
			}
			pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(CaDescriptor));
			pmt_table->ca_info2[pmt_table->ca_count2] = (uint32_t)(pmt_table->tail);

			des_len = parser_ca_des(p_section_data, 
					(CaDescriptor*)(pmt_table->ca_info2[pmt_table->ca_count2]), pmt_table->tail,
					pmt_table->stream_info[pmt_table->stream_count-1].elem_pid);
			pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-des_len);

			pmt_table->ca_count2++;
		}
		else
		{
			gxlogd("\n[descriptor more than MAX_CA_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_CA_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
		}
	}
	else
	{

		if(NULL == pmt_table->ca_info){
			if(des_table_memory_check_pmt_exit(pmt_table,sizeof(uint32_t)*MAX_CA_DESCRIPTOR) == GXCORE_ERROR)
			{
				return GXCORE_ERROR;
			}
			pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(uint32_t)*MAX_CA_DESCRIPTOR);	//default max five ca_des
			pmt_table->ca_info = (uint32_t *)pmt_table->tail;
			pmt_table->ca_count = 0;
		}
		if(pmt_table->ca_count  <  MAX_CA_DESCRIPTOR)
		{
			if(des_table_memory_check_pmt_exit(pmt_table,sizeof(CaDescriptor)) == GXCORE_ERROR)
			{
				return GXCORE_ERROR;
			}
			pmt_table->tail = (uint8_t *)(((uint32_t)(pmt_table->tail)/4)*4-sizeof(CaDescriptor));
			pmt_table->ca_info[pmt_table->ca_count] = (uint32_t)(pmt_table->tail);

			des_len = parser_ca_des(p_section_data, 
					(CaDescriptor*)(pmt_table->ca_info[pmt_table->ca_count]), pmt_table->tail,0);
			pmt_table->tail=(uint8_t *)((uint32_t)pmt_table->tail-des_len);

			pmt_table->ca_count++;
		}
		else
		{
			gxlogd("\n[descriptor more than MAX_CA_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_CA_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
		}
	}

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

