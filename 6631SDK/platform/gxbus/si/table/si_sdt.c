/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_sdt.c
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
#include "module/si/si_sdt.h"

/* Exported Functions ----------------------------------------------------- */
/**
 * @brief
 * @param
 * @Return
 */
int32_t sdt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	SdtInfo *sdt_table=(SdtInfo *)(p_parsed_data);

	if (sdt_table->service_count==0) {
		sdt_table->tail = (uint8_t *)((uint32_t)(sdt_table->buf)+SI_SDT_BUF_SIZE-4);
	}

	if((p_section_data[0] == SDT_ACTUAL_TS_TID)||(p_section_data[0] == SDT_OTHER_TS_TID))
	{
		sdt_table->si_info.ts_id =TODATA16(p_section_data[3],p_section_data[4]);		/* read the transport_stream_id(16) */
		sdt_table->si_info.current_next_indicator=p_section_data[5] & 0x01;
		sdt_table->si_info.ver_number =TODATA5 (p_section_data[5]);				/* read the version_number(5) */
		sdt_table->si_info.sec_number = p_section_data[6];								/* read the section_number(8) */
		sdt_table->si_info.last_sec_number = p_section_data[7];							/* read the last_section_number(8) */
	}
	
	sdt_table->tsid= TODATA16 (p_section_data[3], p_section_data[4]);
	sdt_table->orig_network_id= TODATA16 (p_section_data[8], p_section_data[9]);
	sdt_table->service_info = (service_info_t*)(sdt_table->buf);
	return 0;
}

/**
 * @brief
 * @param
 * @Return
 */
int32_t sdt_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	uint16_t ret_len = 0;
	SdtInfo *sdt_table=(SdtInfo *)(p_parsed_data);
	service_info_t *si;
	uint8_t *head;

	head = (uint8_t*)((uint32_t)(sdt_table->buf)+(sdt_table->service_count+1)*sizeof(service_info_t));

	// make sure the sdt info will not overflow the sdt_buf
	if(head > sdt_table->tail)
	{
		gxlogd("ERR: SDT table buf overflow!\n");
		return -1;
	}

	sdt_table->service_count++;
	
	si = &sdt_table->service_info[sdt_table->service_count-1];
	si->orig_network_id =sdt_table->orig_network_id;
	si->service_id =TODATA16(p_section_data[0], p_section_data[1]);/* read the service_id(16)*/
	si->ts_id = sdt_table->tsid;
	si->free_ca_mode = (p_section_data[3] >> 4 ) & 1;/* read the free_CA_mode(1) */
	ret_len = TODATA12(p_section_data[3], p_section_data[4]);
	return ret_len;
}

/* End of file -------------------------------------------------------------*/

