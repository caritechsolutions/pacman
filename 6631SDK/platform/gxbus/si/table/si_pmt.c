/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_pmt.c
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
#include "module/si/si_pmt.h"

/* Private Functions ------------------------------------------------------ */
/**
 * @brief
 * @param
 * @Return
 */
/*uint8_t *si_pmt_buf_alloc(PmtInfo * pmt,uint32_t siz)
{
	uint8_t *st_buf = pmt->bpoint;
	uint8_t *ed_buf = (uint8_t *)&pmt->buf[SI_PMT_BUF_SIZE-1];

	siz =(siz+3)/4*4;
	if((st_buf+siz)>ed_buf)
	{
		gxlogd("pmt_buf_alloc overflow!\n");
		return NULL;
	}
	pmt->bpoint = (uint8_t *)(st_buf +siz);
	return st_buf;
}*/

/* Exported Functions ----------------------------------------------------- */
/**
 * @brief
 * @param
 * @Return
 */
int32_t pmt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	uint16_t ret_len = 0;
	PmtInfo *	pmt_table = (PmtInfo *)(p_parsed_data);	

	if (pmt_table->stream_count==0) {
		pmt_table->tail = (uint8_t *)((uint32_t)(pmt_table->buf)+SI_PMT_BUF_SIZE-4);
	}
	
	pmt_table->prog_num = TODATA16(p_section_data[3],p_section_data[4]) ;		/* read the program_number(16) */
	pmt_table->si_info.ver_number =TODATA5 (p_section_data[5]);				/* read the version_number(5) */
	pmt_table->si_info.current_next_indicator=p_section_data[5] & 0x01;
	pmt_table->si_info.sec_number = p_section_data[6];								/* read the section_number(8) */	
	pmt_table->si_info.last_sec_number = p_section_data[7];							/* read the last_section_number(8) */
	pmt_table->pcr_pid = TODATA13 (p_section_data[8], p_section_data[9]);		/* read the PCR_PID(13) */
	pmt_table->stream_info = (stream_info_t*)(pmt_table->buf);
	
	ret_len = TODATA12 (p_section_data[10] , p_section_data[11]);
	
	return ret_len;
}

int32_t pmt_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PmtInfo *pmt_table = (PmtInfo *)(p_parsed_data);	
	stream_info_t *ts_info;
	uint16_t ret_len = 0;
	uint8_t *head;

	head = (uint8_t*)((uint32_t)(pmt_table->buf)+(pmt_table->stream_count+1)*sizeof(stream_info_t));

	// make sure the pmt info will not overflow the pmt_buf
	if(head > pmt_table->tail)
	{
		gxlogd("ERR: PMT table buf overflow!\n");
		return -1;
	}

	/*if(NULL == pmt_table->stream_info)
	{
		uint32_t size = pmt_table->stream_count*sizeof(stream_info_t);
		pmt_table->stream_info =(stream_info_t *)si_pmt_buf_alloc(pmt_table, size);
		pmt_table->stream_count = 0; 
	}*/
	
	ts_info = &pmt_table->stream_info[pmt_table->stream_count];
	
	ts_info->stream_type =p_section_data[0];	
	ts_info->elem_pid=TODATA13( p_section_data[1],p_section_data[2]);

	ret_len = TODATA12 (p_section_data[3],p_section_data[4]);		/* read the ES_info_length(12) */
	pmt_table->stream_count++;

	return ret_len;
}

/* End of file -------------------------------------------------------------*/

