#ifndef __SI_SI_TOT_H__
#define __SI_SI_TOT_H__

#include "si_public.h"
typedef struct
{
	uint8_t  lang_code[3];
	uint8_t  country_region_id;
	uint8_t summer;
	int8_t  zone;
	uint32_t offset_mjd;
	uint32_t offset_bcd;
} time_offset;

typedef struct {
	si_info_t  si_info;      /**< si  basic information storage structure  */
	uint32_t   utc_mjd;
	uint32_t   utc_time;     /**< bcd time   0x123456 means  12:34:56 */
	uint8_t offset_time_count;
	time_offset* offset_time;
} TotInfo;

#define MAX_OFFSET_TIME_COUNT 16
#endif

