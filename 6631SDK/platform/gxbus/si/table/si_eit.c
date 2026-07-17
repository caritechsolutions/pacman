/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_eit.c
* Author    : 	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.16	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "module/si/si_eit.h"

#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

/* Private Functions ----------------------------------------------------- */
static void  time_mjd_to_tm(uint16_t mjd, struct tm *tm)
{
// mjd month from 1-12 but tm month from 0-11, so need del 1
#define MJD_TO_TM (1)
	uint8_t IK = 0;

	tm->tm_year = (uint32_t)( (mjd -15078.2) /365.25 );
	tm->tm_mon = (uint32_t)( (mjd - 14956.1 - (int32_t)(tm->tm_year * 365.25) )
																/30.6001);
	tm->tm_mday = (uint32_t)(mjd -14956 - (int32_t)(tm->tm_year * 365.25)
										- (int32_t)(tm->tm_mon * 30.6001));

	if(tm->tm_mon == 14 || tm->tm_mon == 15)
	{
		IK = 1;
	}
	else
	{
		IK = 0;
	}

	tm->tm_year += IK;
	tm->tm_mon = tm->tm_mon - 1 - IK*12 - MJD_TO_TM;
}

static void time_bcd_to_tm(uint32_t bcd_time, struct tm *tm)
{
	tm->tm_sec = BCD2DEC(bcd_time&0xff);
	tm->tm_min = BCD2DEC((bcd_time>>8)&0xff);
	tm->tm_hour = BCD2DEC((bcd_time>>16)&0xff);
}

// return seconds
static time_t get_duration(uint8_t *duration)
{
	uint32_t second = 0;

	second += BCD2DEC(duration[2]);
	second += BCD2DEC(duration[1])*60;
	second += BCD2DEC(duration[0])*3600;

	return (time_t)second;
}


static time_t get_start_time(uint8_t *start_time)
{
	struct tm tm;
	uint32_t buf_to_val;

	memset(&tm, 0, sizeof(struct tm));

	buf_to_val = TODATA16(start_time[0], start_time[1]);
	time_mjd_to_tm((uint16_t)buf_to_val, &tm);

	buf_to_val = TODATA24(start_time[2], start_time[3], start_time[4]);
	time_bcd_to_tm(buf_to_val, &tm);

	return mktime(&tm);
}

/* Exported Functions ----------------------------------------------------- */
/**
 * @brief
 * @param
 * @Return
 */
int32_t eit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	EitInfo *eit_table = (EitInfo *)(p_parsed_data);

	if (eit_table->event_count==0) {
		eit_table->tail = (uint8_t *)((uint32_t)(eit_table->buf)+SI_EIT_BUF_SIZE-4);
		gxlogd("init tail --- 0x%x\n",(uint32_t)(eit_table->tail));
	}
	eit_table->service_id = TODATA16(p_section_data[3],p_section_data[4]) ;
	eit_table->si_info.service_id = eit_table->service_id;
	eit_table->si_info.current_next_indicator=p_section_data[5] & 0x01;
	eit_table->si_info.ver_number =TODATA5 ( p_section_data[5]);				/* read the version_number(5) */
	eit_table->si_info.sec_number = p_section_data[6];								/* read the section_number(8) */
	eit_table->si_info.last_sec_number = p_section_data[7];							/* read the last_section_number(8) */
	eit_table->si_info.ts_id =  TODATA16(p_section_data[8],p_section_data[9]) ;/* read the transport_stream_id(16) */
	eit_table->orig_network_id = TODATA16(p_section_data[10],p_section_data[11]) ;
	eit_table->si_info.segment_last_sec_number = p_section_data[12];

	eit_table->event_info=(event_info_t*)(eit_table->buf);

	return 0;
}

/**
 * @brief
 * @param
 * @Return
 */
int32_t eit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data)
{
	EitInfo *eit_table = (EitInfo *)(p_parsed_data);
	event_info_t *event_info;
	uint8_t *head;

	head = (uint8_t*)((uint32_t)(eit_table->buf)+(eit_table->event_count+1)*sizeof(event_info_t));

	// make sure the eit info will not overflow the eit_buf
	if(head > eit_table->tail)
	{
		gxlogd("[%s]: EIT table buf overflow!\n",__FUNCTION__);
		return -1;
	}

	eit_table->event_count++;

	event_info = &(eit_table->event_info[eit_table->event_count-1]);
	event_info->event_id =TODATA16(p_section_data[0], p_section_data[1]);
	event_info->start_time = get_start_time(&p_section_data[2]);
	event_info->duration = get_duration(&p_section_data[7]);
	event_info->running_status = TODATAHI3(p_section_data[10]);
	event_info->free_CA_mode = (p_section_data[10]>>4)&1;
	event_info->descriptors_loop_length = TODATA12(p_section_data[10], p_section_data[11]);

	return event_info->descriptors_loop_length;
}

/* End of file -------------------------------------------------------------*/

