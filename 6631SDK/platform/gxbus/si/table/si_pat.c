/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_pat.c
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
#include "module/si/si_pat.h"


/* Exported Functions ----------------------------------------------------- */
/**
 * @brief
 * @param
 * @Return
 */
int32_t pat_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PatInfo *pat_table = (PatInfo *)(p_parsed_data);	
	
	pat_table->si_info.ts_id =  TODATA16(p_section_data[3],p_section_data[4]) ;/* read the transport_stream_id(16) */
	pat_table->si_info.current_next_indicator=p_section_data[5] & 0x01;
	pat_table->si_info.ver_number =TODATA5 ( p_section_data[5]);				/* read the version_number(5) */
	pat_table->si_info.sec_number = p_section_data[6];								/* read the section_number(8) */	
	pat_table->si_info.last_sec_number = p_section_data[7];							/* read the last_section_number(8) */

	pat_table->prog_info=(prog_info_t *)(pat_table->buf);

	return 0;
}

/**
 * @brief
 * @param
 * @Return
 */
int32_t pat_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	PatInfo *pat_table = (PatInfo *)(p_parsed_data);	
	uint16_t ret_len = 0;
	uint16_t num;	// choose pid is network_pid or program_map_PID 
	uint16_t pid;

	num =TODATA16 ( p_section_data[0],p_section_data[1]);
	pid =TODATA13 ( p_section_data[2],p_section_data[3]);

	if (num == 0)
	{
		pat_table->nit_pid = pid;
		if (pid != NIT_PID) 
		{
			gxlogd("pat nit pid is error: %d\n",pid);
		}
	} 
	else if((pat_table->prog_count+1)*sizeof(prog_info_t) <SI_PAT_BUF_SIZE)
	{
		pat_table->prog_info[pat_table->prog_count].prog_num=num;
		pat_table->prog_info[pat_table->prog_count].pmt_pid=pid;
		
		++pat_table->prog_count;
	} 
	else
	{
		gxlogd("ERR:  PAT table buf overflow!\n");
		return -1;
	}		

	return ret_len;
}

/* End of file -------------------------------------------------------------*/

