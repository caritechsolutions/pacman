/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_ter_del_sys.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.03.30	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_nit.h"

int32_t des_ter_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	NitInfo *nit_table = (NitInfo *)p_parsed_data;
	ts_info_t *ts_info = nit_table->ts_info[nit_table->ts_count-1];
	niti_delivery_info_t *info = NULL;
	uint8_t *tail = NULL;

	info = (niti_delivery_info_t *)((uint8_t *)ts_info + sizeof(ts_info_t) + ts_info->delivery_count*(sizeof(niti_delivery_info_t)));
	tail = (uint8_t *)(info) + sizeof(niti_delivery_info_t);

	GxSiDes_Trace();
	if(tail > nit_table->tail)
	{
		gxlogd("ERR: NIT table buf delivery_info overflow!\n");
		return GXCORE_ERROR;
	}
	memset(info, 0, sizeof(niti_delivery_info_t));
	// set delivery_type
	info->delivery_type = p_section_data[0];
	info->ter.centr_freq = TODATA32(p_section_data[2],p_section_data[3],p_section_data[4],p_section_data[5]);
	// change to KHz
	info->ter.centr_freq *= 10;
	info->ter.bandwidth = TODATAHI3(p_section_data[6]);
	info->ter.constellation = (p_section_data[7]>>6) & 3;
	info->ter.hierarchy = (p_section_data[7]>>3) & 7;
	info->ter.trans_mode = (p_section_data[8]>>1) & 3;
	info->ter.other_freq = p_section_data[8] & 1;
	info->ter.code_rate_HP = TODATALO3(p_section_data[7]);
	info->ter.code_rate_LP = TODATAHI3(p_section_data[8]);
	info->ter.guard_interval = (p_section_data[8]>>3) & 3;
	ts_info->delivery_count ++;

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

