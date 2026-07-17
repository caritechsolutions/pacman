/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_nit.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.02	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "module/si/si_nit.h"


/* Exported Functions ----------------------------------------------------- */
/**
 * @brief
 * @param
 * @Return
 */
int32_t nit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	NitInfo *nit_table = (NitInfo *)p_parsed_data;
	uint16_t ret_len;

	nit_table->network_id = TODATA16(p_section_data[3], p_section_data[4]);	/* read the network_id(16) */
	nit_table->si_info.current_next_indicator =  p_section_data[5] & 0x01;
	nit_table->si_info.ver_number = TODATA5 (p_section_data[5]);				/* read the version_number(5) */
	nit_table->si_info.sec_number =  p_section_data[6];								/* read the section_number(8) */
	nit_table->si_info.last_sec_number =  p_section_data[7];							/* read the last_section_number(8) */

	if (nit_table->ts_count==0)
	{
		nit_table->ts_info[0] = (ts_info_t*)nit_table->buf;
		nit_table->tail = (uint8_t *)((uint32_t)(nit_table->buf)+SI_NIT_BUF_SIZE-4);
	}

	ret_len = TODATA12 ( p_section_data[8] , p_section_data[9]);
	return ret_len;

}

int32_t nit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	NitInfo *nit_table = (NitInfo *)p_parsed_data;
	ts_info_t 	*ts_info = NULL, *ts_tmp = NULL;
	uint16_t ret_len = 0, pos = 0;
	uint8_t *head;
	uint8_t i = 0, j = 0;

	if(nit_table->ts_count >= NIT_TSINFO_MAX)
	{
		gxlogd("ERR: NIT table ts info number overflow!\n");
		return -1;
	}

	for(i=0; i<nit_table->ts_count; i++)
	{
	    pos += sizeof(ts_info_t);
	    ts_tmp = nit_table->ts_info[i];
	    for(j=0;j<ts_tmp->delivery_count;j++)
	        pos += sizeof(niti_delivery_info_t)*ts_tmp->delivery_count;
	}
	// make sure the nit info will not overflow the nit_buf
	head = (uint8_t *)((uint8_t *)nit_table->buf + pos + sizeof(ts_info_t));
	if(head > nit_table->tail)
	{
		gxlogd("ERR: NIT table buf overflow!\n");
		return -1;
	}

	ts_info = nit_table->ts_info[nit_table->ts_count] = (ts_info_t *)((uint8_t *)nit_table->buf + pos);
	nit_table->ts_count++;

	ts_info->delivery_count = 0;
	ts_info->delivery_info = (niti_delivery_info_t *)((uint8_t *)ts_info + sizeof(ts_info_t));
	ts_info->ts_id = TODATA16 (p_section_data[0] , p_section_data[1]);/* read the transport_stream_id(16)*/
	ts_info->orig_network_id =TODATA16 (p_section_data[2], p_section_data[3]);/* read the original_network_id(16)*/
	ret_len = TODATA12 (p_section_data[4] , p_section_data[5]);

	return ret_len;
}

/* End of file -------------------------------------------------------------*/

