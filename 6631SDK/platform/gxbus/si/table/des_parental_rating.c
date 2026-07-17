/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :   des_parental_rating.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.09.26	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_eit.h"


int32_t des_parental_rating(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	EitInfo *eit_table = (EitInfo *)(p_parsed_data);
	event_info_t *event_info = &eit_table->event_info[eit_table->event_count-1];
	uint8_t *head = NULL;

	uint8_t  des_len = p_section_data[1];
	uint8_t  i;

	GxSiDes_Trace();
	if(NULL== p_section_data)
	{
		return GXCORE_ERROR;
	}

	head=(uint8_t *)((uint32_t)(eit_table->buf)+(eit_table->event_count)*sizeof(event_info_t));
	if(head > eit_table->tail)
	{
		gxloge("[%s]: EIT table buf overflow!\n",__FUNCTION__);
		return GXCORE_ERROR;
	}

	if (event_info->parental_rating_des == NULL)
	{
		eit_table->tail = (uint8_t *)((uint32_t)(eit_table->tail)-(des_len+3)/4*4);
		if(head > eit_table->tail)
		{
			gxloge("[%s]: EIT table buf overflow!\n",__FUNCTION__);
			return GXCORE_ERROR;
		}

		// 4 - the length of parental rating witch except tag and length
		event_info->parental_rating_count = des_len/4;

		event_info->parental_rating_des=(parental_rating_t*)eit_table->tail;
	}

	p_section_data += 2;

	for(i=0; i<event_info->parental_rating_count; i++)
	{
		#define COUNTRY_CODE_LEN   (3)
		memcpy(event_info->parental_rating_des[i].country_code, p_section_data, COUNTRY_CODE_LEN);
		event_info->parental_rating_des[i].rating = *(uint8_t*)(p_section_data+COUNTRY_CODE_LEN);

		// +1  rating length
		p_section_data += COUNTRY_CODE_LEN+1;
	}

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

