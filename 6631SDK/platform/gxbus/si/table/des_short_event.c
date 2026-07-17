/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_short_event.c
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
#include "module/si/si_eit.h"

#define MAX_SHORT_EVENT_DESCRIPTOR 		5

static int32_t parser_short_event_des(uint8_t *head, uint8_t *p_section_data,short_event_des_t *des_info_data, EitInfo *eit_table)
{
	int dlen = p_section_data[1];

	if(dlen < 4)
	{
		gxlogd("err: %s\n",__FUNCTION__);
		p_section_data += dlen;
		return 0;
	}
	gxlogd("1  tail - 0x%x\n", (uint32_t)eit_table->tail);
	memcpy(des_info_data->language_code ,p_section_data+2,3);
	des_info_data->event_name_length = p_section_data[5];
	p_section_data += 6;

	if(des_info_data->event_name_length>0)
	{
		eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(des_info_data->event_name_length+3)/4*4);
		if(head > eit_table->tail)
		{
			gxlogd("EIT table buf overflow! head: %p tail: %p\n", head, eit_table->tail);
			return -1;
		}

		des_info_data->event_name=eit_table->tail;
		memcpy(des_info_data->event_name,p_section_data,des_info_data->event_name_length);
	}

	p_section_data +=des_info_data->event_name_length;
	des_info_data->text_length = p_section_data[0];
	p_section_data += 1;

	if(des_info_data->text_length>0)
	{
		eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(des_info_data->text_length+3)/4*4);
		if(head > eit_table->tail)
		{
			gxlogd("EIT table buf overflow! head: %p tail: %p\n", head, eit_table->tail);
			return -1;
		}

		des_info_data->text=eit_table->tail;
		memcpy(des_info_data->text,p_section_data,des_info_data->text_length);
	}
	gxlogd("2  tail - 0x%x\n", (uint32_t)eit_table->tail);

	return ((des_info_data->event_name_length+3)/4*4+(des_info_data->text_length+3)/4*4);
}


int32_t des_short_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	EitInfo *eit_table=(EitInfo *)(p_parsed_data);
	short_event_des_t *p_short_des;
	event_info_t *event_info = &eit_table->event_info[eit_table->event_count-1];
	uint8_t *head;
	int32_t des_len;

	GxSiDes_Trace();

	head=(uint8_t *)((uint32_t)(eit_table->buf)+(eit_table->event_count)*sizeof(event_info_t));
	if(head > eit_table->tail)
	{
		gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
		return GXCORE_ERROR;
	}

	if (event_info->short_event_des == NULL)
	{
		eit_table->tail=(uint8_t *)((uint32_t)(eit_table->tail)-sizeof(short_event_des_t)*MAX_SHORT_EVENT_DESCRIPTOR);//default one event info have five short event descriptor
		if(head > eit_table->tail)
		{
			gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
			return GXCORE_ERROR;
		}

		event_info->short_event_des=(short_event_des_t*)eit_table->tail;
		event_info->short_event_count = 0;
	}
	if(event_info->short_event_count < MAX_SHORT_EVENT_DESCRIPTOR)
	{
		p_short_des=&(event_info->short_event_des[event_info->short_event_count]);

		des_len=parser_short_event_des(head, p_section_data,p_short_des,eit_table);
		if (des_len == -1)
		{
			gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
			return GXCORE_ERROR;
		}
		event_info->short_event_count++;
		gxlogd("2  eit_table->tail - 0x%x\n", (uint32_t)(eit_table->tail));
	}
	else
	{
		gxlogd("\n[descriptor more than MAX_SHORT_EVENT_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_SHORT_EVENT_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
	}
	return GXCORE_SUCCESS;

}

/* End of file -------------------------------------------------------------*/

