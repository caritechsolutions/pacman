#include "../include/vod_in_common_def.h"
#ifdef LINUX_OS
#include <linux/socket.h>
#endif

extern int vod_porting_mktime_clock_to_npt(unsigned char * timestr, unsigned int * npt);
extern int vod_porting_mktime_npt_to_clock(unsigned int npt, unsigned char * timestr);

unsigned int vod_func_parse_range_clocktype(char* str)
{
	char *p;
	unsigned int ret = 0;

	p = strchr(str, '=');
	if(p)
	{
		p = p + 1;
		ADV_SPACE(p);
	}
	else
	{
		p = str;
	}
	vod_porting_mktime_clock_to_npt((unsigned char*)p, &ret);

	return ret;
}

int32_t vod_func_make_range_clock(uint8_t * range_str, uint32_t npt)
{
	uint8_t tmpstr[20] = {0};

	if(range_str == NULL)
	{
		return -1;
	}

	vod_porting_mktime_npt_to_clock(npt, tmpstr);

	strcpy((char*)range_str, "Clock=");
	strcat((char*)range_str, (char*)tmpstr);
	strcat((char*)range_str, "-");

	return 0;
}

