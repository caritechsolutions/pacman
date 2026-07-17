/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_extended_event.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.16	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "string.h"
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_eit.h"
#include "module/config/gxconfig.h"
#include "service/gxsi.h"

#define MAX_EXTENDED_EVENT_DESCRIPTOR 		20

static uint8_t parser_extended_check(uint8_t *p_section_data)
{
	uint16_t items_len = 0;
	uint16_t des_len = 0;
	uint16_t dlen = p_section_data[1];
	if(dlen < 5)
	{
		return 0;
	}

	items_len = p_section_data[6];
	des_len = p_section_data[items_len+7];

	/* p_section_data[0] : text length */
	if(des_len != (dlen-items_len-6))
	{
		p_section_data[0] += dlen;
		return 0;
	}
	return 1;
}


static int32_t parser_extended_event_des(uint8_t *head, uint8_t *p_section_data,
		extended_event_des_t *des_info_data, EitInfo *eit_table)
{
	int32_t des_len = 0;
	extended_item_info_t *item_info = NULL;
	uint8_t item_count = 0;
	char splice = 0;
	int16_t items_len = 0;

	uint8_t *p = NULL;
	memcpy(des_info_data->language_code, p_section_data+3, 3);
	items_len = p_section_data[6];
	p_section_data += 7;
	if(items_len > 0)
	{
		item_info = (extended_item_info_t *)GxCore_Malloc(MAX_EXTENDED_EVENT_DESCRIPTOR*sizeof(extended_item_info_t));
		if(item_info == NULL)
		{
			return -1;
		}
	}
	p = p_section_data;
	p_section_data += items_len;
	while(items_len>0)
	{
		if(item_count >= MAX_EXTENDED_EVENT_DESCRIPTOR)
		{
			break;
		}
		item_info[item_count].item_description_length = p[0];
		if(p[0]!=0)
		{
			eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(p[0] + 3) / 4 * 4);
			if (head > eit_table->tail)
			{
				gxlogd("EIT table buf overflow! head: %p tail: %p\n", head, eit_table->tail);
				goto err;
			}
			des_len += (p[0] + 3) / 4 * 4;
			item_info[item_count].item_description = eit_table->tail;
			memcpy(item_info[item_count].item_description, &p[1], p[0]);
		}
		else
		{
			item_info[item_count].item_description = NULL;
		}
		items_len -= (p[0]+1);
		p += p[0]+1;
		item_info[item_count].item_length = p[0];
		if(p[0]!=0)
		{
			eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(p[0] + 3) / 4 * 4);
			if (head > eit_table->tail)
			{
				gxlogd("EIT table buf overflow! head: %p tail: %p\n", head, eit_table->tail);
				goto err;
			}
			des_len += (p[0] + 3) / 4 * 4;
			item_info[item_count].item = eit_table->tail;
			memcpy(item_info[item_count].item, &p[1], p[0]);
		}
		else
		{
			item_info[item_count].item = NULL;
		}
		items_len -= (p[0]+1);
		p += p[0]+1;
		item_count++;
	}
	des_info_data->item_count = item_count;
	if(item_count > 0)
	{
		eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(item_count*sizeof(extended_item_info_t) + 3) / 4 * 4);
		if (head > eit_table->tail)
		{
			gxlogd("EIT table buf overflow! head: %p, tail: %p\n", head, eit_table->tail);
			goto err;
		}
		des_len += (item_count*sizeof(extended_item_info_t) + 3) / 4 * 4;
		des_info_data->extended_item_info = (extended_item_info_t*)eit_table->tail;
		memcpy(des_info_data->extended_item_info,item_info,item_count*sizeof(extended_item_info_t));
	}
	else
	{
		des_info_data->extended_item_info = NULL;
	}
	des_info_data->extended_text_length = p_section_data[0];
	p_section_data += 1;
	eit_table->tail=(uint8_t *)((uint32_t)eit_table->tail-(des_info_data->extended_text_length + 4) / 4 * 4);
	if (head > eit_table->tail)
	{
		gxlogd("EIT table buf overflow! head: %p tail: %p\n", head, eit_table->tail);
		goto err;
	}
	des_len += (des_info_data->extended_text_length + 4) / 4 * 4;
	des_info_data->extended_text=eit_table->tail;

	memcpy(des_info_data->extended_text, p_section_data, des_info_data->extended_text_length);
	GxBus_ConfigGet(SI_CONFIG_EVENT_DETAIL_SPLICE, &splice, sizeof(char), ")");
	if (splice == ')')
		splice = '\0';
	des_info_data->extended_text[des_info_data->extended_text_length] = splice;
	des_info_data->extended_text_length += 1;

	GxCore_Free(item_info);
	return des_len;
err:
	GxCore_Free(item_info);
	return -1;
}

int32_t des_extended_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	EitInfo *eit_table=(EitInfo *)(p_parsed_data);
	extended_event_des_t *p_extended_des;
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

	if (parser_extended_check(p_section_data) == 0)
	{
		return GXCORE_SUCCESS;
	}

	if (event_info->extended_event_des == NULL)
	{
		eit_table->tail=(uint8_t *)((uint32_t)(eit_table->tail)-sizeof(extended_event_des_t)*MAX_EXTENDED_EVENT_DESCRIPTOR);//default one event info have five extened event descriptor
		if(head > eit_table->tail)
		{
			gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
			return GXCORE_ERROR;
		}

		event_info->extended_event_des=(extended_event_des_t*)eit_table->tail;
		event_info->extended_event_count = 0;
	}

	if(event_info->extended_event_count < MAX_EXTENDED_EVENT_DESCRIPTOR)
	{
		p_extended_des=&(event_info->extended_event_des[event_info->extended_event_count]);
		des_len=parser_extended_event_des(head, p_section_data,p_extended_des,eit_table);

		if (des_len == -1)
		{
			gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
			return GXCORE_ERROR;
		}

		if (des_len != 0)
		{
			gxlogd("2  ext eit_table->tail - 0x%x\n", (uint32_t)(eit_table->tail));
			event_info->extended_event_count++;
		}
	}
	else
	{
		gxlogd("\n[descriptor more than MAX_EXTENDED_EVENT_DESCRIPTOR(%d)]:%s,%s,%d\n",MAX_EXTENDED_EVENT_DESCRIPTOR,__FILE__,__FUNCTION__,__LINE__);
	}
	return GXCORE_SUCCESS;

}

/* End of file -------------------------------------------------------------*/

