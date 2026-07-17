/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_dtmb_del_sys.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.11.30	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "gxtype.h"
#include "gxcore.h"
#include "module/si/si_public.h"
#include "module/si/si_nit.h"

#define GET_DEC32(bcd)	(((bcd>>28)&0x0f)*10000000\
						+ ((bcd>>24)&0x0f)*1000000\
						+ ((bcd>>20)&0x0f)*100000\
						+ ((bcd>>16)&0x0f)*10000\
						+ ((bcd>>12)&0x0f)*1000\
						+ ((bcd>>8)&0x0f)*100\
						+ ((bcd>>4)&0x0f)*10\
						+ (bcd&0x0f))


int32_t des_dtmb_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
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
	info->dtmb.centr_freq = TODATA32(p_section_data[2],p_section_data[3],p_section_data[4],p_section_data[5])/100;
	// change to KHz
	//info.dtmb.centr_freq = GET_DEC32(info.dtmb.centr_freq)/10;
	info->dtmb.FEC = TODATAHI4(p_section_data[6]);
	info->dtmb.modulation = TODATALO4(p_section_data[6]);
	info->dtmb.number_of_subcarrier = TODATAHI4(p_section_data[7]);
	info->dtmb.frame_header_mode = TODATALO4(p_section_data[7]);
	info->dtmb.interleaving_mode = TODATAHI4(p_section_data[8]);
	info->dtmb.other_frequency_flag = (p_section_data[8]>>3)&1 ;
	info->dtmb.sfn_mfn_flag = (p_section_data[8]>>2)&1 ;
	ts_info->delivery_count++;

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

