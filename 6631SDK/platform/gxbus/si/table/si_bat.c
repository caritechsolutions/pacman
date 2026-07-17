/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_bat.c
* Author    : 	shenbin
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.17	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "module/si/si_bat.h"


/* Exported Functions ----------------------------------------------------- */
int32_t bat_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	uint16_t ret_len = 0;
	BatInfo *bat_table = (BatInfo *)(p_parsed_data);		

	if(bat_table->ts_count==0)
	{
		bat_table->si_info.current_next_indicator=p_section_data[5] & 0x01;
		bat_table->si_info.ver_number =TODATA5 (p_section_data[5]);				/* read the version_number(5) */
		bat_table->si_info.sec_number = p_section_data[6];								/* read the section_number(8) */	
		bat_table->si_info.last_sec_number = p_section_data[7];							/* read the last_section_number(8) */
		bat_table->tail=(uint8_t *)((uint32_t)(bat_table->buf)+SI_BAT_BUF_SIZE-4);
	}
	
	bat_table->ts_info=(trans_stream_t*)(bat_table->buf);
	bat_table->bouquet_id =TODATA16 (p_section_data[3], p_section_data[4]);/* read the bouquet_id(16)*/
	ret_len = TODATA12(p_section_data[8], p_section_data[9]);

	return ret_len;
}


int32_t bat_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	BatInfo *bat_table = (BatInfo *)(p_parsed_data);
	trans_stream_t *ts_bat;
	uint16_t ret_len = 0;
	uint8_t *head;

	head = (uint8_t*)((uint32_t)(bat_table->buf)+(bat_table->ts_count+1)*sizeof(trans_stream_t));

	// make sure the sdt info will not overflow the sdt_buf
	if(head > bat_table->tail)
	{
		gxlogd("ERR: BAT table buf overflow!\n");
		return -1;
	}
	
	bat_table->ts_count++;
	
	ts_bat = &bat_table->ts_info[bat_table->ts_count-1];
	ts_bat->tp_stream_id =TODATA16(p_section_data[0] , p_section_data[1]);/* read the transport_stream_id(16) */		
	ts_bat->orig_network_id =TODATA16 (p_section_data[2], p_section_data[3]);/* read the original_network_id(16)*/

	ret_len = TODATA12(p_section_data[4], p_section_data[5]);
	return ret_len;
}

/* End of file -------------------------------------------------------------*/

