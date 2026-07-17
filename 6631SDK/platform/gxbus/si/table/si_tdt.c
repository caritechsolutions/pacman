/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_tdt.c
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
#include "module/si/si_tdt.h"

/* Private Functions ------------------------------------------------------ */

/* Exported Functions ----------------------------------------------------- */
int32_t tdt_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	TdtInfo *tdt_table = (TdtInfo*)p_parsed_data;

	// in order to unify with other table, add si_info by manual
	tdt_table->si_info.sec_number = 0;
	tdt_table->si_info.last_sec_number = 0;
	
	tdt_table->utc_mjd = TODATA16(p_section_data[3], p_section_data[4]);
	tdt_table->utc_time = TODATA24(p_section_data[5], p_section_data[6], p_section_data[7]);

	return 8;/*tdt total len 8*/
}

/* End of file -------------------------------------------------------------*/

