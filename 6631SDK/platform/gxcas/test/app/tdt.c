#include "gxcore.h"
#include "time.h"
#include "../../include/gxcas_psi.h"
#include "../../include/gxcas_demux.h"

#define TDT_PID                         (0x14)
#define TOT_PID                         (0x14)
#define TDT_TID                    0x70
#define TOT_TID                    0x73
handle_t si_handle[2] = {0,};
handle_t tdt_thread = 0;
static handle_t tdt_lock = 0;
#define SECTION_SIZE (64*1024)

static void  extra_mjd_to_tm(uint16_t mjd, struct tm *tm)
{
	uint8_t IK = 0;
	    
	tm->tm_year = (uint32_t)( (mjd -15078.2) /365.25 );
	tm->tm_mon = (uint32_t)( (mjd - 14956.1 - (int32_t)(tm->tm_year * 365.25) ) /30.6001);
	tm->tm_mday = (uint32_t)(mjd -14956 - (int32_t)(tm->tm_year * 365.25) - (int32_t)(tm->tm_mon * 30.6001));
				    
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
#define BCD2DEC(bcd)    (((bcd)>>4)*10+((bcd)&0xf))

	tm->tm_sec = BCD2DEC(bcd_time&0xff);
	tm->tm_min = BCD2DEC((bcd_time>>8)&0xff);
	tm->tm_hour = BCD2DEC((bcd_time>>16)&0xff);
}

void tdt_time_task(void *args)
{
	uint8_t *section;/* section  buffer */
	uint8_t i = 0;
	uint32_t length;
	uint32_t utc_mjd = 0, utc_time = 0;
	struct tm tm_time;
	time_t t_utc_time = 0;
	GxTime sys_time;
	if (tdt_lock == 0)
	    GxCore_MutexCreate(&tdt_lock);

	section = GxCore_Malloc(SECTION_SIZE);
	if( section == NULL ) {
	    printf("alloc memory failure!\n");
	    return;
	}
	while(1){
		for (i = 0; i<2;i++) {
		    GxCore_MutexLock(tdt_lock);
		    if (si_handle[i] == 0) {
		        GxCore_MutexUnlock(tdt_lock);
		        continue;
		    }

		    int32_t ret = -1;
		    sifilter_params_t sfilter = {0};
		    ret = GxCas_SiFilter_Read(si_handle[i],section,SECTION_SIZE,&length);

		    GxCore_MutexUnlock(tdt_lock);
		    if (ret == 0 && length > 0){
				if(section[0] == TDT_TID){
					utc_mjd = section[3]<<8|section[4];
					utc_time = section[5]<<16|section[6]<<8|section[7];
				}else if(section[0] == TOT_TID){
					utc_mjd = section[3]<<8|section[4];
					utc_time = section[5]<<16|section[6]<<8|section[7];
				}
				extra_mjd_to_tm((uint16_t)utc_mjd, &tm_time);
				extra_bcd_to_tm(utc_time, &tm_time);
				t_utc_time = mktime(&tm_time);
				sys_time.seconds = t_utc_time;
				sys_time.microsecs = 0;
				GxCore_SetLocalTime(&sys_time);
			}
		}
		
		GxCore_ThreadDelay(300);
	}
}

void tdt_time_init(void)
{
	sifilter_params_t filter = {0};
	handle_t si = 0;

	filter.pid = TDT_PID;
	filter.match[0] = TDT_TID;
	filter.mask[0] = 0xFF;
	filter.depth = 1;
	filter.flags = EQ_FLAG | REPEAT_FLAG;
	filter.nWaitSeconds = 10;
	si = GxCas_SiFilter_Start(filter);
	if(si == -1)
		printf("===============tdt filter start failed!!\n");
	else
		si_handle[0] = si;

	filter.pid = TOT_PID;
	filter.match[0] = TOT_TID;
	filter.mask[0] = 0xFF;
	filter.depth = 1;
	filter.flags = EQ_FLAG | REPEAT_FLAG;
	filter.nWaitSeconds = 10;
	si = GxCas_SiFilter_Start(filter);
	if(si == -1)
		printf("===============tot filter start failed!!\n");
	else
		si_handle[1] = si;

	GxCore_ThreadCreate("tdt",&tdt_thread, tdt_time_task, NULL, 32 * 1024, GXOS_DEFAULT_PRIORITY);
}

