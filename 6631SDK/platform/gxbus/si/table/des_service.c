/*****************************************************************************
* 						   CONFIDENTIAL								
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
#include "module/si/si_sdt.h"


/**
 * @brief
 * @param
 * @Return
 */
static uint8_t parser_service_des(uint8_t *p_section_data,service_des_info_t *des_info_data,uint8_t *tail)
{
	des_info_data->service_type = p_section_data[2]; 
	
	des_info_data->service_provider_name_length = p_section_data[3]; /* provider name length */;
	p_section_data += 4;
	if(des_info_data->service_provider_name_length>0)
	{
		tail=(uint8_t *)((uint32_t)tail-(des_info_data->service_provider_name_length+3)/4*4);
		des_info_data->service_provider_name=tail;
		memcpy(des_info_data->service_provider_name,p_section_data,des_info_data->service_provider_name_length);
	}
	
	p_section_data += des_info_data->service_provider_name_length;
	des_info_data->service_name_length = p_section_data[0]; /* service name length */;
	p_section_data += 1;

	if(des_info_data->service_name_length>0)
	{
		tail=(uint8_t *)((uint32_t)tail-(des_info_data->service_name_length+3)/4*4);

		des_info_data->service_name=tail;
		memcpy(des_info_data->service_name,p_section_data,des_info_data->service_name_length);
	}
	
	return((des_info_data->service_name_length+3)/4*4+(des_info_data->service_provider_name_length+3)/4*4);
}


int32_t des_service_descrip(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	SdtInfo *sdt_table=(SdtInfo *)(p_parsed_data);	
	service_des_info_t *p_service_des;
	service_info_t *si = &sdt_table->service_info[sdt_table->service_count-1];
	uint8_t *head;
	uint8_t des_len;

	GxSiDes_Trace();
	if(NULL == si->service_des_info){
		sdt_table->tail=(uint8_t *)(((uint32_t)(sdt_table->tail)/4)*4-sizeof(service_des_info_t));//default one service info have one service descriptor
		si->service_des_info=(service_des_info_t *)sdt_table->tail;
		si->service_des_count = 0;
	}

	p_service_des=si->service_des_info;
	head=(uint8_t *)((uint32_t)sdt_table->buf+(sdt_table->service_count)*sizeof(service_info_t));
	if(head > sdt_table->tail) 
	{
		gxlogd("ERR: SDT table buf overflow des!\n");
		return GXCORE_ERROR;
	}
	des_len=parser_service_des(p_section_data,p_service_des,sdt_table->tail);
	sdt_table->tail=(uint8_t *)(((uint32_t)sdt_table->tail/4)*4-des_len);

	si->service_des_count++;
	
	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

