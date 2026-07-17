/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_tms_event.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2012.06.05	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_eit.h"


int32_t des_eit_tms_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
#define TMS_EVENT_DES_LEN   4
	EitInfo *eit_table = (EitInfo *)(p_parsed_data);
	uint8_t   des_len = p_section_data[1];
	time_shifted_event_des_t *tms_event_des = &(eit_table->event_info[eit_table->event_count-1].tms_event_des);

	GxSiDes_Trace();

	if((NULL==p_section_data) || (des_len!=TMS_EVENT_DES_LEN))
	{
		return GXCORE_ERROR;
	}

	if (eit_table->event_info[eit_table->event_count-1].tms_event_count > 0)
	{
		gxlogd("tms event des error!\n");
		return GXCORE_ERROR;
	}

	tms_event_des->reference_service_id = TODATA16(p_section_data[2], p_section_data[3]);
	tms_event_des->reference_event_id = TODATA16(p_section_data[4], p_section_data[5]);

	eit_table->event_info[eit_table->event_count-1].tms_event_count = 1;

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

