/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_tot.c
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
#include "module/si/si_tot.h"
#include "time.h"
/* Private Functions ------------------------------------------------------ */

/* Exported Functions ----------------------------------------------------- */


static void  extra_mjd_to_tm(uint16_t mjd, struct tm *tm)
{
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

	tm->tm_year += (IK+1900);
	tm->tm_mon = tm->tm_mon - 1 - IK*12;
}

static void extra_bcd_to_tm(uint32_t bcd_time, struct tm *tm)
{
#define BCD2DEC(bcd)	(((bcd)>>4)*10+((bcd)&0xf))

	tm->tm_sec = BCD2DEC(bcd_time&0xff);
	tm->tm_min = BCD2DEC((bcd_time>>8)&0xff);
	tm->tm_hour = BCD2DEC((bcd_time>>16)&0xff);
}


int32_t  tot_head_parser(uint8_t *p_section_data, uint32_t len, uint8_t *p_parsed_data)
{
	uint32_t	wMjd;
	uint32_t   utc_time;
	uint8_t 	*pData = NULL;

	uint16_t LoopLength;
	//uint32_t CountryCode = 0;
//	uint32_t CountryCodeTot = 0;

//	uint8_t DiscTag;
	uint8_t DiscLength;
	uint8_t DiscLengthTimeOffset;
	uint8_t Polarity;
	int8_t CurTimeOffSet;
	int8_t NextTimeOffSet;
	uint8_t LoopTimes = 0;
	uint32_t now_seconds = 0;
	uint32_t change_seconds = 0;
	struct tm tm_time;
	TotInfo *tot_table = (TotInfo*)p_parsed_data;

	pData= p_section_data;

	if (len < 12) {
		return -1;
	}

	tot_table->si_info.sec_number = 0;
	tot_table->si_info.last_sec_number = 0;

	tot_table->utc_mjd = TODATA16(pData[3], pData[4]);
	tot_table->utc_time = TODATA24(pData[5], pData[6], pData[7]);
	extra_mjd_to_tm((uint16_t)(tot_table->utc_mjd), &tm_time);
	extra_bcd_to_tm(tot_table->utc_time, &tm_time);

	tm_time.tm_mon -= 1;
	tm_time.tm_year -= 1900;

	now_seconds = mktime(&tm_time);

	LoopLength = ((pData[8]&0x0f)<<8) + pData[9];

	pData = &pData[10];

	while(LoopLength>0 && pData-p_section_data<len)
	{

//		DiscTag = pData[0];
		DiscLength = pData[1];
		if(*pData == 0x58)//local_time_offset_descriptor
		{
			DiscLengthTimeOffset = DiscLength;
			if(tot_table->offset_time != NULL)
			{
				GxCore_Free(tot_table->offset_time);
				tot_table->offset_time = NULL;
			}
			tot_table->offset_time = GxCore_Malloc(MAX_OFFSET_TIME_COUNT * sizeof(time_offset));
			if(tot_table->offset_time == NULL)
			{
				gxlogd("err: %s malloc failed!!!!\n",__FUNCTION__);
				return -1;
			}
			memset(tot_table->offset_time,0,MAX_OFFSET_TIME_COUNT * sizeof(time_offset));
			tot_table->offset_time_count = 0;
			LoopTimes = 0;
			while(DiscLengthTimeOffset>0 && pData+14-p_section_data<len)
			{
				if(tot_table->offset_time_count < MAX_OFFSET_TIME_COUNT)
				{
					memcpy(tot_table->offset_time[LoopTimes].lang_code, &pData[2+LoopTimes*13],3);
					tot_table->offset_time[LoopTimes].country_region_id =  pData[5+LoopTimes*13]&0xfc;
					Polarity = pData[5+LoopTimes*13]&0x01;
					CurTimeOffSet = ((pData[6+LoopTimes*13]&0xf0)>>4)*10 + (pData[6+LoopTimes*13]&0x0f);
					tot_table->offset_time[LoopTimes].offset_mjd = TODATA16(pData[8+LoopTimes*13], pData[9+LoopTimes*13]);
					tot_table->offset_time[LoopTimes].offset_bcd = TODATA24(pData[10+LoopTimes*13], pData[11+LoopTimes*13], pData[12+LoopTimes*13]);
					NextTimeOffSet = ((pData[13+LoopTimes*13]&0xf0)>>4)*10 + (pData[13+LoopTimes*13]&0x0f);
					if(Polarity == 1 && NextTimeOffSet <= CurTimeOffSet)
					{
						tot_table->offset_time[LoopTimes].zone = -CurTimeOffSet;
						tot_table->offset_time[LoopTimes].summer = 0;
					}
					else if(Polarity == 1 && NextTimeOffSet > CurTimeOffSet)
					{
						tot_table->offset_time[LoopTimes].zone = -NextTimeOffSet;
						tot_table->offset_time[LoopTimes].summer = 1;
					}
					else if(Polarity == 0 && NextTimeOffSet >= CurTimeOffSet)
					{
						tot_table->offset_time[LoopTimes].zone = CurTimeOffSet;
						tot_table->offset_time[LoopTimes].summer = 0;
					}
					else if(Polarity == 0 && NextTimeOffSet < CurTimeOffSet)
					{
						tot_table->offset_time[LoopTimes].zone = NextTimeOffSet;
						tot_table->offset_time[LoopTimes].summer = 1;
					}
					tot_table->offset_time_count++;
				}
				DiscLengthTimeOffset -= 13;
				LoopTimes ++;
			}
		}
		pData += (DiscLength +2);
		LoopLength -= (DiscLength + 2);
	}

return 0;
}



/* End of file -------------------------------------------------------------*/

