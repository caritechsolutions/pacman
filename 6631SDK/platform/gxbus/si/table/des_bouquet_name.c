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
#include "module/si/si_bat.h"


static uint8_t  bouquet_name_des(uint8_t *p_section_data,bouquet_info_t *des_info_data,uint8_t *tail)
{
	des_info_data->bouquet_name_length =p_section_data[1];

	p_section_data += 2;
	
	if(des_info_data->bouquet_name_length>0)
	{
		tail = (uint8_t *)((uint32_t)tail-(des_info_data->bouquet_name_length+3)/4*4);
		des_info_data->bouquet_name = tail;
		memcpy(des_info_data->bouquet_name,p_section_data,des_info_data->bouquet_name_length);
	}

	return ((des_info_data->bouquet_name_length+3)/4*4);
}


int32_t des_bouqust_name(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	BatInfo *bat_table = (BatInfo *)(p_parsed_data);
	bouquet_info_t *bouquet_name;
	uint8_t *head;
	uint8_t des_len;

	GxSiDes_Trace();
	if(NULL == bat_table->bouquet_info){
		bat_table->tail = (uint8_t *)(((uint32_t)(bat_table->tail)/4)*4-sizeof(bouquet_info_t));	//default one
		bat_table->bouquet_info = (bouquet_info_t *)bat_table->tail;
		bat_table->bouquet_count = 0;
	}

	if (bat_table->bouquet_count > 0)
	{
		gxlogd ("des_bouqust_name more than one!\n");
		return GXCORE_ERROR;
	}

	bouquet_name = bat_table->bouquet_info;
	head=(uint8_t *)((uint32_t)bat_table->buf+(bat_table->ts_count)*sizeof(trans_stream_t));
	if(head > bat_table->tail) 
	{
		gxlogd("ERR: bouquet name buf overflow!\n");
		return GXCORE_ERROR;
	}

	des_len = bouquet_name_des(p_section_data,bouquet_name,bat_table->tail);
	bat_table->tail=(uint8_t *)((uint32_t)bat_table->tail-des_len);

	bat_table->bouquet_count++;

	return GXCORE_SUCCESS;
}


/* End of file -------------------------------------------------------------*/

