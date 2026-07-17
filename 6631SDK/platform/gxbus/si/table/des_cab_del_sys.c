/*****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_cab_del_sys.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.21	      shenbin	         creation
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
#define GET_DEC16(bcd)	(((bcd>>12)&0x0f)*1000\
						+ ((bcd>>8)&0x0f)*100\
						+ ((bcd>>4)&0x0f)*10\
						+ (bcd&0x0f))


int32_t des_cab_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
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
	info->cab.freq = TODATA32 (p_section_data[2],p_section_data[3],p_section_data[4],p_section_data[5]) ;
	// change to KHz
	info->cab.freq = GET_DEC32(info->cab.freq)/10;
	info->cab.fec_outer = TODATALO4(p_section_data[7]);
	info->cab.modulation = p_section_data[8];
	info->cab.sym_rate = TODATA28(p_section_data[9], p_section_data[10], p_section_data[11], p_section_data[12]);
	info->cab.fec_inner = TODATALO4(p_section_data[12]);
	// change to Ksymbol/s
	info->cab.sym_rate = GET_DEC32(info->cab.sym_rate)/10;
	ts_info->delivery_count++;

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

