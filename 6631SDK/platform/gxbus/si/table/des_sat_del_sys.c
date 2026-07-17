/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	des_sat_del_sys.c
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


int32_t des_sat_del_sys(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
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
	info->sat.freq = TODATA32 (p_section_data[2],p_section_data[3],p_section_data[4],p_section_data[5]) ;
	// change to MHz
	info->sat.freq = GET_DEC32(info->sat.freq)/100;
	info->sat.orbit  =TODATA16  (p_section_data[6],p_section_data[7]) ;
	// change to degree*10
	info->sat.orbit = GET_DEC16(info->sat.orbit);
	info->sat.we_flag = (p_section_data[8]>>7) & 0x01 ;
	info->sat.polar =   (p_section_data[8]>>5) & 0x03 ;
	info->sat.mod_system =  p_section_data[8]& 0x1f ;
	info->sat.fec_inner =  p_section_data[12] & 0x0f ;
	info->sat.sym_rate = (p_section_data[9]<<20 )|(p_section_data[10]<<12 )
						|(p_section_data[11]<< 4 )|((p_section_data[12]>>4 )& 0x0f);
	// change to Ksymbol/s
	info->sat.sym_rate = GET_DEC32(info->sat.sym_rate)/10;
	ts_info->delivery_count ++;

	return GXCORE_SUCCESS;
}

/* End of file -------------------------------------------------------------*/

