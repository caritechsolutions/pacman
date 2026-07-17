/*****************************************************************************
 * 						   CONFIDENTIAL
 *        Hangzhou GuoXin Science and Technology Co., Ltd.
 *                      (C)2009, All right reserved
 ******************************************************************************

 ******************************************************************************
 * File Name :	gxepg.c
 * Author    : 	shenbin
 * Project   :	GoXceed
 * Type      :
 ******************************************************************************
 * Purpose   :
 ******************************************************************************
 * Release History:
 VERSION	Date			  AUTHOR         Description
 0.0  	2009.09.22	      shenbin	         creation
 *****************************************************************************/

/* Includes --------------------------------------------------------------- */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include "gxmsg.h"
#include "gxcore.h"
#include "module/si/si_eit.h"
#include "module/epg/gxepg_manage.h"
#include "gxavdev.h"
#include "gxservices.h"
#include "gdi_core.h"
#include "gxepg.h"
#include "module/pm/gxpm_manage.h"
#include "module/config/gxconfig.h"
#include "dvbhal/gxdemux_hal.h"
#include "epg_private.h"

#define ALL_LANG_STR    "all"
#define ENG_LANG_STR    "eng"
#define ARA_LANG_STR    "ara"
#define BLANK_LANG_STR    ""
#define UTF8_CODE       (0x15)
#define ARA_CODE_1      (0x02)
#define ARA_CODE_2      (0x06)

/* Private types/constants ------------------------------------------------ */

#define EPG_NOT_ADD     ((void*)(0xffffffff))
#define EPG_NVOD_ADD     ((void*)(0xfffffffe))

#define CHECK_POINT(point, err_ret) do{								\
	if (point == NULL){												\
		gxlogd("null point in %s %d\n", __func__, __LINE__);		\
		return err_ret;}											\
}while(0)

#define CHECK_NULL_POINT(point) do{									\
	if (point == NULL){												\
		gxlogd("null point in %s %d\n", __func__, __LINE__);		\
		return;}													\
}while(0)

/* Private Variables ------------------------------------------------------ */

// default epg cfg message
static GxMsgProperty_EpgCreate EpgCfg = {
	.service_count = 400,
	.event_count = 2000,
	.event_size = 100,
	.epg_day = 3,
	.ts_id = 0xffffffff,
	.service_id = 0xffffffff,

	.language = {'e', 'n', 'g'},
	.cur_tp_only = 1,
	.txt_format = EPG_TXT_ORIGINAL,
};

static GxMsgProperty_EpgDetailFilterSwitch EpgDetailFilter = {
	.filter_level = 0,
	.ts_id = 0xffff,
	.orig_network_id = 0xffff,
	.service_id = 0xffff,
	.event_id = 0xffff
};

#define DEFAULT_GATE           (20)
#define PER_EVENT_SIZE         (3072)
#define EVENT_INCREASE_SIZE    (1024)

static char *thiz_iso639_langmap[] = {
	"eng,en",
	"vi,vie",
	"ar,ara",
	"fr,fra,fre",
	"ru,rus",
	"pt,por",
	"es,spa",
	"tr,tur",
	"it,ita",
	"pl,pol",
	"de,deu,ger",
	"fa,fas,per",
	"ms,may,msa",//马来西亚
	"id,ind",//印地语
	"my,bur,mya",//缅甸语
	"th,tha",//泰语
	"hi,hin",//印尼语
	"lo,lao",//老挝语
	"ur,urd",//乌尔都语
	"ko,kor",//韩语
	"el,ell,gre",//希腊语
	"uk,ukr",//乌克兰语
	"ja,jap",//日语
	"zh,chi,zho",
};
#define ISO639_MAP_COUNT   sizeof(thiz_iso639_langmap)/sizeof(char*)


/* record the epg num when user send "GXMSG_EPG_NUM_GET", make sure the "GXMSG_EPG_GET"
   and the "GXMSG_EPG_NUM_GET" point to the same service*/
static GxMsgProperty_EpgNumGet s_EpgNum;

static handle_t s_EpgServiceHandle;

// count the epg event older than today
static uint32_t s_InvalidEpgNum = 0;

static bool s_EpgCreateFlag = false;
static GxEpgCacheCtrl s_EpgCacheCtrl = {0};
static GxEpgSectionCtrl s_EpgSectionCtrl;

static handle_t s_EpgParseThreadHandle = 0;
static handle_t s_EpgSem = 0;

static void epg_parse_thread(void *arg);
static void epg_cache_clean(void);
static void epg_cache_init(uint8_t cache_gate);

/* Private Macros --------------------------------------------------------- */
/* Private Functions ------------------------------------------------------ */

static void epg_cfg_init(void)
{
	EpgCfg.service_count = 400;
	EpgCfg.event_count = 2000;
	EpgCfg.event_size = 100;
	EpgCfg.epg_day = 3;
	EpgCfg.ts_id = 0xffffffff;
	EpgCfg.service_id = 0xffffffff;

	strncpy((char *)EpgCfg.language, "eng", 3);
	EpgCfg.cur_tp_only = 1;
	EpgCfg.txt_format = EPG_TXT_ORIGINAL;
	return;
}

static int32_t _add_service(ts_index* ts, uint32_t service_id, uint8_t sec_number)
{
	sec_index* service = NULL;

	ts->service = GxCore_Realloc(ts->service, (ts->count + 1) * sizeof(sec_index));
	if (ts->service == NULL)
	{
		EPG_ERR("!!!!!No memory to store!!!!!\n");
		while(1);
	}

	service = &ts->service[ts->count];
	memset(service, 0, sizeof(sec_index));
	service->service_id = service_id;
	service->sec_number[sec_number] = 1;
	ts->count++;
	return 0;
}

/*
 * @return:
 *  1: repeat section
 *  0: new section
 *
 * */
static int32_t check_section_exist(uint32_t table_id, si_info_t* p_si_info)
{
	uint32_t i = 0;
	int32_t free_pos = -1;
	sec_status *section_status = NULL;
	sec_status *p_sec_status = NULL;
	ts_index* ts = NULL;

	GxCore_MutexLock(s_EpgSectionCtrl.mutex);
	p_sec_status = s_EpgSectionCtrl.sectionStatus;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		if(table_id == p_sec_status[i].table_id)
		{
			section_status = &p_sec_status[i];
			break;
		}

		if(-1 == free_pos && 0 == p_sec_status[i].table_id)
			free_pos = i;
	}

	if(NULL == section_status)//new section
	{
		if(-1 == free_pos)//shouldn't exist
		{
			return 0;
		}
		else
		{
			p_sec_status[free_pos].table_id = table_id;
			section_status = &p_sec_status[free_pos];
		}
	}

	for (i = 0; i < section_status->count; i++) {
		if (section_status->ts[i].ts_id == p_si_info->ts_id) {
			ts = &section_status->ts[i];
			break;
		}
	}

	if (ts == NULL)
	{
		section_status->ts = GxCore_Realloc(section_status->ts, (section_status->count + 1) * sizeof(ts_index));
		if (section_status->ts == NULL)
		{
			EPG_ERR("!!!!!No memory to store!!!!!\n");
			while(1);
		}
		ts = &section_status->ts[section_status->count];
		memset(ts, 0, sizeof(ts_index));
		ts->ts_id = p_si_info->ts_id;
		_add_service(ts, p_si_info->service_id, p_si_info->sec_number);
		section_status->count++;
		GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
		return 0;
	}
	else
	{
		sec_index* service = NULL;
		for (i = 0; i < ts->count; i++)
		{
			if (ts->service[i].service_id == p_si_info->service_id)
			{
				service = &ts->service[i];
				break;
			}
		}

		if (service != NULL)
		{
			if (service->sec_number[p_si_info->sec_number] == 1)
			{
				GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
				return 1;//repeat section
			}
			else
			{
				service->sec_number[p_si_info->sec_number] = 1;
				GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
				return 0;//new section
			}
		}
		else
		{
			_add_service(ts, p_si_info->service_id, p_si_info->sec_number);
			GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
			return 0;//new section
		}
	}
}

static void epg_section_status_init(void)
{
	int i = 0;
	sec_status *p_sec_status = NULL;

	GxCore_MutexLock(s_EpgSectionCtrl.mutex);
	p_sec_status = s_EpgSectionCtrl.sectionStatus;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		memset(&p_sec_status[i], 0, sizeof(sec_status));
	}
	GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);

	return;
}

static void epg_section_status_release(void)
{
	int32_t i, j;
	sec_status *p_sec_status = NULL;

	GxCore_MutexLock(s_EpgSectionCtrl.mutex);
	p_sec_status = s_EpgSectionCtrl.sectionStatus;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		if(p_sec_status[i].table_id != 0)
		{
			ts_index* ts = NULL;
			for(j = 0; j < p_sec_status[i].count; j++) {
				ts = &p_sec_status[i].ts[j];
				GxCore_Free(ts->service);
			}
			if (p_sec_status[i].ts)
				GxCore_Free(p_sec_status[i].ts);
			memset(&p_sec_status[i], 0, sizeof(sec_status));
		}
	}

	GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
	return;
}

/*
 * according table_id, ts_id, service_id, sec_number to clear section flag
 * */
static void epg_section_status_clear(uint16_t table_id, uint16_t ts_id, uint32_t service_id, uint8_t sec_number)
{
	int32_t i, j, z;
	sec_status *p_sec_status = NULL;

	GxCore_MutexLock(s_EpgSectionCtrl.mutex);
	p_sec_status = s_EpgSectionCtrl.sectionStatus;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		if(p_sec_status[i].table_id != table_id)
			continue;

		for(j = 0; j < p_sec_status[i].count; j++)
		{
			ts_index *ts = &p_sec_status[i].ts[j];
			if(ts->ts_id != ts_id)
				continue;

			for(z = 0; z < ts->count; z++)
			{
				sec_index *service = &ts->service[z];
				if(service->service_id == service_id)
				{
					service->sec_number[sec_number] = 0;
					GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
					return;
				}
			}
		}
	}
	GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
	return;
}

static void epg_section_status_service_clear(uint16_t ts_id, uint32_t service_id)
{
	int32_t i, j, z;
	sec_status *p_sec_status = NULL;

	GxCore_MutexLock(s_EpgSectionCtrl.mutex);
	p_sec_status = s_EpgSectionCtrl.sectionStatus;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		for(j = 0; j < p_sec_status[i].count; j++)
		{
			ts_index *ts = &p_sec_status[i].ts[j];
			if(ts->ts_id != ts_id)
				continue;

			for(z = 0; z < ts->count; z++)
			{
				sec_index *service = &ts->service[z];
				if(service->service_id == service_id)
				{
					memset(service->sec_number,0,sizeof(service->sec_number));
					GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
					return;
				}
			}
		}
	}
	GxCore_MutexUnlock(s_EpgSectionCtrl.mutex);
	return;
}

// return  copy length
static uint16_t epg_mempool_string_copy(uint8_t *dst, GxBusEpgMemPoolCell *src, uint32_t *limit_size)
{
	uint16_t ret_len = 0;
	//	uint16_t rest_len;

	if (dst == NULL
			|| src == NULL
			|| limit_size == NULL)
	{
		EPG_ERR("null point in this function\n");
		return 0;
	}
#ifdef EPG_MALLOC
	if(src->size > *limit_size)
	{
		*limit_size = 0;
		return 0;
	}
	memcpy((dst+ret_len), src->content, src->size);
	ret_len += src->size;
#else
	while(src->next_Cell != NULL)
	{
		if ((GX_BUS_MEM_POOL_CELL_SIZE-4) > *limit_size)
		{
			*limit_size = 0;
			return 0;
		}

		memcpy((dst+ret_len), src->content, (GX_BUS_MEM_POOL_CELL_SIZE-4));
		src = (GxBusEpgMemPoolCell*)(src->next_Cell);
		ret_len += (GX_BUS_MEM_POOL_CELL_SIZE-4);
		*limit_size -= (GX_BUS_MEM_POOL_CELL_SIZE-4);
	}

	// copy all data in last cell
	if ((GX_BUS_MEM_POOL_CELL_SIZE-4) > *limit_size)
	{
		*limit_size = 0;
		return 0;
	}

	memcpy((dst+ret_len), src->content, (GX_BUS_MEM_POOL_CELL_SIZE-4));
	ret_len += (GX_BUS_MEM_POOL_CELL_SIZE-4);
	*limit_size -= (GX_BUS_MEM_POOL_CELL_SIZE-4);

#endif
	return ret_len;
}

// only for language_code compare

static int _is_matched(char *src, char *dst)
{
	char *p = src;
	char *str = dst;
LOOP:
	if((p = strstr((const char*)p, (const char*)str))
		&& ((*(p+strlen((const char*)str)) == ',')
			|| (*(p+strlen((const char*)str)) == '\0')
			|| (*(p+strlen((const char*)str)) == 0x20))
		&& ((p == src)
			|| ((p != src)
			&& ((*(p-1) == ',') || ((*(p-1) == 0x20)))))
		)
	{
		return 1;
	}
	if(p)
	{
		p += 1;
		goto LOOP;
	}
	return 0;
}

static int _language_match(unsigned char* lang1, unsigned char* lang2, unsigned int lang_len)
{
#define TEMP_LEN  10 // i think it is enough forever
	int i = 0;
	int count = ISO639_MAP_COUNT;
	char str[TEMP_LEN] = {0};
	if(lang_len >= TEMP_LEN)
	{
		gxlogd("\nERROR, %s, %d\n", __func__, __LINE__);
		return -1;
	}
	memcpy(str, lang1, lang_len);
	for(i = 0; i < count; i++)
	{
		if(thiz_iso639_langmap[i])
		{
			if(_is_matched(thiz_iso639_langmap[i], str))
			{// find lang1
				memset(str, 0, TEMP_LEN);
				memcpy(str, lang2, lang_len);
				if(_is_matched(thiz_iso639_langmap[i], str))
				{// fine lang2
					return 1;
				}
				return 0;
			}
		}
	}
	return 0;
}

static int language_code_cmp(char* language1, char* language2)
{
#define LANGUAGE_LEN    (3)
	uint8_t language_1[LANGUAGE_LEN];
	uint8_t language_2[LANGUAGE_LEN];
	uint16_t i;

	for (i=0; i<LANGUAGE_LEN; i++)
	{
		language_1[i] = tolower(language1[i]);
		language_2[i] = tolower(language2[i]);
	}
	if(memcmp((const void *)language_1, (const void *)language_2, LANGUAGE_LEN) == 0)
	{
		return 0;
	}
	else
	{
		if(_language_match(language_1, language_2, LANGUAGE_LEN))
			return 0;
		else
			return -1;
	}
}

static GxBusEpgFirstEvent* epg_get_first_event(GxBusEpgFirstEvent *p_first_event)
{
	while(p_first_event != NULL)
	{
		if (language_code_cmp((char*)(p_first_event->language), (char*)(EpgCfg.language)) == 0)
		{
			break;
		}
		p_first_event = p_first_event->next;
	}

	return p_first_event;
}

static status_t epg_one_event_get(uint32_t event_pos, GxBusEpgEventHead **event_head, int32_t *per_event_size)
{
	status_t ret = GX_BUS_EPG_OK;
	if(NULL == event_head || NULL == *event_head || NULL == per_event_size)
		return GX_BUS_EPG_PARAMETER_ERR;

	while(1)
	{
		ret = GxBus_EpgEventGet(event_pos, *event_head, *per_event_size);
		if(GX_BUS_EPG_EVENT_TOO_SMALL == ret)
		{
			*per_event_size += EVENT_INCREASE_SIZE;
			*event_head = GxCore_Realloc(*event_head, *per_event_size);
			if(NULL == *event_head)
			{
				EPG_ERR("!!!!!!GxCore_Realloc error!!!!!!\n");
				break;
			}
		}
		else
		{
			break;
		}
	}

	return ret;
}

static status_t epg_one_nvod_get(uint32_t ts_id, uint32_t reference_service_id, uint32_t reference_event_id,
									GxBusEpgEventHead **event_head, int32_t *per_nvod_size)
{
	status_t ret = GX_BUS_EPG_OK;

	if(NULL == event_head || NULL == *event_head || NULL == per_nvod_size)
		return GX_BUS_EPG_PARAMETER_ERR;

	while(1)
	{
		ret = GxBus_NvodInfoGet(ts_id, reference_service_id, reference_event_id, *event_head, *per_nvod_size);
		if(GX_BUS_EPG_EVENT_TOO_SMALL == ret)
		{
			*per_nvod_size += EVENT_INCREASE_SIZE;
			*event_head = GxCore_Realloc(*event_head, *per_nvod_size);
			if(NULL == *event_head)
			{
				EPG_ERR("!!!!!!GxCore_Realloc error!!!!!!\n");
				break;
			}
		}
		else
		{
			break;
		}
	}

	return ret;
}

static uint16_t epg_schedule_count_get(GxBusEpgServiceHead *p_service_head)
{
	GxBusEpgEventHead *event_head = NULL;
	GxBusEpgFirstEvent *p_first_event = NULL;
	int32_t per_event_size = PER_EVENT_SIZE;
	uint16_t i = 0, ret_count = 0;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_POINT(p_service_head, 0);

	p_first_event = epg_get_first_event(p_service_head->first_event);

	if (p_first_event == NULL)
		return 0;

	if (p_first_event->pos == GX_BUS_EPG_INVALID_POS)
		return 0;

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		return 0;

	if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
		goto end;

	// here first event must be exist, so i from 1
	for (i=1; i<EpgCfg.event_count; i++)
	{
		if (event_head->next_event == GX_BUS_EPG_INVALID_POS)
		{
			ret_count = i;
			goto end;
		}

		if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
			goto end;
	}

end:
	EPG_FREE(event_head);
	return ret_count;
}

static uint16_t epg_schedule_count_get_by_period(GxBusEpgServiceHead *p_service_head, time_t start_time, time_t end_time)
{
	GxBusEpgEventHead event_head = {0};
	GxBusEpgFirstEvent *p_first_event = NULL;
	int32_t per_event_size = PER_EVENT_SIZE;
	uint16_t i = 0, ret_count = 0;
	time_t event_start_time = 0;
	time_t event_end_time = 0;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_POINT(p_service_head, 0);

	p_first_event = epg_get_first_event(p_service_head->first_event);

	if(NULL == p_first_event)
		return 0;

	if(GX_BUS_EPG_INVALID_POS == p_first_event->pos)
		return 0;

	if(GX_BUS_EPG_OK != GxBus_EpgEventBasicInfoGet(p_first_event->pos, &event_head))
		return 0;

	event_start_time = event_head.time.start_time;
	event_end_time = event_head.time.start_time + event_head.time.duration;

	if(event_end_time > start_time && event_start_time < end_time)
		ret_count++;

	// here first event must be exist, so i from 1
	for (i = 1; i < EpgCfg.event_count; i++)
	{
		if (event_start_time > end_time || event_head.next_event == GX_BUS_EPG_INVALID_POS)
			break;

		if(GX_BUS_EPG_OK != GxBus_EpgEventBasicInfoGet(event_head.next_event, &event_head))
			break;

		event_start_time = event_head.time.start_time;
		event_end_time = event_head.time.start_time + event_head.time.duration;

		if(event_end_time > start_time && event_start_time < end_time)
			ret_count++;
	}

	return ret_count;
}

static status_t epg_event_type_get(GxParseResult *parse_result, GxBusEpgEventType *event_type)
{
	CHECK_POINT(parse_result, GXCORE_ERROR);
	CHECK_POINT(event_type, GXCORE_ERROR);

	if(0x4e == parse_result->table_id)
	{
		// it's hard to read, forgive me!!!
		if(((EitInfo*)(parse_result->parsed_data))->si_info.sec_number == 0)
			*event_type = GX_EVENT_PRESENT;
		else
			*event_type = GX_EVENT_FOLLOW;
	}
	else
	{
		*event_type = GX_EVENT_DETAILE;
	}

	return GXCORE_SUCCESS;
}

// today_time : today time 0:0:0
static void epg_event_sort_by_day(uint16_t *event_per_day, uint32_t today_time,
									GxBusEpgEventTime event_time, float time_zone)
{
#define PER_DAY_SEC  (3600*24)
	uint32_t day_count = 0;
	uint32_t event_end_time_with_zone = 0;
	uint32_t event_start_time_with_zone = 0;

	CHECK_NULL_POINT(event_per_day);

	if (time_zone > 0) {
		today_time += (uint32_t)(time_zone * 3600);
		today_time -= (uint32_t)(today_time % PER_DAY_SEC);
		event_start_time_with_zone = event_time.start_time + (uint32_t)(time_zone * 3600);
		event_end_time_with_zone = event_time.start_time + event_time.duration + (uint32_t)(time_zone * 3600);
	} else {
		time_zone = fabs(time_zone);
		today_time -= (uint32_t)(time_zone * 3600);
		today_time -= (uint32_t)(today_time % PER_DAY_SEC);
		event_start_time_with_zone = event_time.start_time - (uint32_t)(time_zone * 3600);
		event_end_time_with_zone = event_time.start_time + event_time.duration - (uint32_t)(time_zone * 3600);
	}

	// check invalid event number
	if (event_end_time_with_zone <= today_time)
	{
		s_InvalidEpgNum++;
		return;
	}

	day_count = (event_start_time_with_zone-today_time)/PER_DAY_SEC;

	if (day_count >= 7)
		return;

	// day_count store the info of present and follow, so the shedule need +1
	event_per_day[day_count+1]++;
}

static void epg_parental_rating_get(GxMsgProperty_EpgParentalInfo *parental_get)
{
	GxBusEpgServiceHead service_head = {0};
	int32_t parental_rating_size = 0;

	CHECK_NULL_POINT(parental_get);

	if (GxBus_EpgServiceGet(parental_get->ts_id, parental_get->service_id,
				parental_get->orig_network_id, &service_head) != GX_BUS_EPG_OK)
		return;

	if (service_head.parental_rating_num > MAX_MSG_PARENTAL_RATING_NUM)
	{
		EPG_ERR("Number of parental locks %d > 16  Some parental lock information lost !!!!!!!!!!!!\n",service_head.parental_rating_num);
	}
	parental_get->parental_rating_num = (service_head.parental_rating_num > MAX_MSG_PARENTAL_RATING_NUM) ? MAX_MSG_PARENTAL_RATING_NUM : service_head.parental_rating_num;
	memcpy(&(parental_get->parental_rating[0]), service_head.parental_rating, (parental_get->parental_rating_num * sizeof(GxBusEpgParentalRating)));
	GxBus_EpgServiceContentRelease(&service_head);
	return;
}

static void epg_service_epg_num_get(GxMsgProperty_EpgNumGet *p_epg)
{
	uint32_t i = 0;
	GxTime today_time = {0};
	GxBusEpgServiceHead service_head = {0};
	GxBusEpgFirstEvent *p_first_event = NULL;
	GxBusEpgEventHead *event_head = NULL;
	int32_t per_event_size = PER_EVENT_SIZE;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_NULL_POINT(p_epg);

	// record the invalid event, will use in epg_service_epg_get
	s_InvalidEpgNum = 0;

	//gxlogd("sizeof(p_epg->event_per_day) value is : %d\n",sizeof(p_epg->event_per_day));
	memset(p_epg->event_per_day, 0, sizeof(p_epg->event_per_day));

	// clr the count
	p_epg->real_event_count = 0;

	if (GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id,
				&service_head) != GX_BUS_EPG_OK)
	{
		// sync the internal and external variable about GxMsgProperty_EpgNumGet
		memcpy(&s_EpgNum, p_epg, sizeof(GxMsgProperty_EpgNumGet));
		return;
	}

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		goto end;

	//3present and follow
#ifdef EPG_MALLOC
	if (service_head.present.name.content != NULL)
#else
	if (service_head.present.name != NULL)
#endif
	{
		p_epg->real_event_count = 2;
		p_epg->event_per_day[0] = 2;
	}

	// shedule number
	p_epg->real_event_count += epg_schedule_count_get(&service_head);

	// no shedule epg get
	if (p_epg->real_event_count == p_epg->event_per_day[0])
	{
		memcpy(&s_EpgNum, p_epg, sizeof(GxMsgProperty_EpgNumGet)); //sync
		goto end;
	}

	//3 if have shedule info, set pf info even it is NULL
	if (p_epg->event_per_day[0] != 2)
	{
		p_epg->real_event_count += 2;
		p_epg->event_per_day[0] = 2;
	}

	//2 the first shedule event get
	p_first_event = epg_get_first_event(service_head.first_event);
	if (p_first_event == NULL)
		goto end;

	if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
		goto end;

	// set the today start time,  present or follow both ok
	GxCore_GetLocalTime(&today_time);

	epg_event_sort_by_day(p_epg->event_per_day, today_time.seconds,
			event_head->time, p_epg->time_zone);

	//2 the shedule event get
	for (i=3; i<p_epg->real_event_count; i++)
	{
		if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
			goto end;

		epg_event_sort_by_day(p_epg->event_per_day, today_time.seconds,
				event_head->time, p_epg->time_zone);
	}

	memcpy(&s_EpgNum, p_epg, sizeof(GxMsgProperty_EpgNumGet));  //sync

end:
	GxBus_EpgServiceContentRelease(&service_head);
	EPG_FREE(event_head);
	return;
}

static void epg_service_epg_get(GxMsgProperty_EpgGet *p_epg)
{
#define DUMMY_NAME_LEN  (1)
#define PF_NUMBER       (2)
	uint32_t limit_size = 0;
	GxBusEpgServiceHead service_head = {0};
	GxBusEpgEventHead *event_head = NULL;
	GxBusEpgFirstEvent *p_first_event = NULL;
	uint8_t *p_text_addr = NULL; // used to store event_name and event_detail
	uint16_t i = 0;
	uint16_t pos = p_epg->get_event_pos;
	int32_t per_event_size = PER_EVENT_SIZE;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_NULL_POINT(p_epg);
	CHECK_NULL_POINT(p_epg->epg_info);

	if (p_epg->ts_id != s_EpgNum.ts_id
			/*|| p_epg->orig_network_id != s_EpgNum.orig_network_id*/
			|| p_epg->service_id != s_EpgNum.service_id) {
		EPG_ERR("please send \"GXMSG_EPG_NUM_GET\" first!\n");
		return;
	}

	// get nothing
	if (s_EpgNum.real_event_count == 0)
	{
		p_epg->get_event_count = 0;
		return;
	}

	if (p_epg->get_event_pos > s_EpgNum.real_event_count-1)
	{
		EPG_ERR("get position larger than the total epg number!\n");
		p_epg->get_event_count = 0;
		return;
	}

	// clr the count
	p_epg->get_event_count = 0;

	if (GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head) != GX_BUS_EPG_OK)
		return;

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		goto end;

	// no space to store the head info of epg
	if (p_epg->want_event_count*sizeof(GxEpgInfo) > p_epg->epg_info_size)
	{
		EPG_ERR("[epg]   1epg_info_size too small\n");
		goto end;
	}

	// the space can save name and detail
	limit_size = p_epg->epg_info_size - p_epg->want_event_count*sizeof(GxEpgInfo);
	p_text_addr = (uint8_t*)(p_epg->epg_info) + p_epg->want_event_count*sizeof(GxEpgInfo);

	// "0" means present event
	if (pos == 0)
	{
		// present
#ifdef EPG_MALLOC
		if (service_head.present.name.content != NULL)
#else
		if (service_head.present.name != NULL)
#endif
		{
			p_epg->epg_info[0].event_id = service_head.present.event_id;
			p_epg->epg_info[0].start_time = service_head.present.time.start_time;
			p_epg->epg_info[0].duration = service_head.present.time.duration;

			p_epg->epg_info[0].event_name = p_text_addr;
			p_epg->epg_info[0].event_name_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &service_head.present.name, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, service_head.present.name, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[0].event_name_len;

			p_epg->epg_info[0].event_detail = p_text_addr;
			p_epg->epg_info[0].event_detail_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &service_head.present.detaile_description, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, service_head.present.detaile_description, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[0].event_detail_len;
		}
		else
		{
			p_epg->epg_info[0].event_id = 0;
			p_epg->epg_info[0].start_time = 0;
			p_epg->epg_info[0].duration = 0;
			p_epg->epg_info[0].event_name_len = DUMMY_NAME_LEN;
			p_epg->epg_info[0].event_detail_len = DUMMY_NAME_LEN;
			p_epg->epg_info[0].event_name = p_text_addr;
			memcpy(p_epg->epg_info[0].event_name, "", DUMMY_NAME_LEN);
			p_text_addr += DUMMY_NAME_LEN;
			p_epg->epg_info[0].event_detail = p_text_addr;
			memcpy(p_epg->epg_info[0].event_detail, "", DUMMY_NAME_LEN);
			p_text_addr += DUMMY_NAME_LEN;
		}
		if (limit_size == 0){
			EPG_ERR("[epg]   2epg_info_size too small\n");
			goto end;
		}
		p_epg->get_event_count++;

		if (p_epg->get_event_count >= p_epg->want_event_count)
			goto end;
	}

	// "1" means follow event
	if (pos <= 1)
	{
		// follow
#ifdef EPG_MALLOC
		if (service_head.follow.name.content != NULL)
#else
		if (service_head.follow.name != NULL)
#endif
		{
			p_epg->epg_info[1-pos].event_id = service_head.follow.event_id;
			p_epg->epg_info[1-pos].start_time = service_head.follow.time.start_time;
			p_epg->epg_info[1-pos].duration = service_head.follow.time.duration;

			p_epg->epg_info[1-pos].event_name = p_text_addr;
			p_epg->epg_info[1-pos].event_name_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &service_head.follow.name, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, service_head.follow.name, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[1-pos].event_name_len;

			p_epg->epg_info[1-pos].event_detail = p_text_addr;
			p_epg->epg_info[1-pos].event_detail_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &service_head.follow.detaile_description, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, service_head.follow.detaile_description, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[1-pos].event_detail_len;
		}
		else
		{
			p_epg->epg_info[1-pos].event_id = 0;
			p_epg->epg_info[1-pos].start_time = 0;
			p_epg->epg_info[1-pos].duration = 0;
			p_epg->epg_info[1-pos].event_name_len = DUMMY_NAME_LEN;
			p_epg->epg_info[1-pos].event_detail_len = DUMMY_NAME_LEN;
			p_epg->epg_info[1-pos].event_name = p_text_addr;
			memcpy(p_epg->epg_info[1-pos].event_name, "", DUMMY_NAME_LEN);
			p_text_addr += DUMMY_NAME_LEN;
			p_epg->epg_info[1-pos].event_detail = p_text_addr;
			memcpy(p_epg->epg_info[1-pos].event_detail, "", DUMMY_NAME_LEN);
			p_text_addr += DUMMY_NAME_LEN;
		}
		if (limit_size == 0){
			EPG_ERR("[epg]   2epg_info_size too small\n");
			goto end;
		}
		p_epg->get_event_count++;

		if (p_epg->get_event_count >= p_epg->want_event_count)
			goto end;
	}

	// the first shedule event get
	p_first_event = epg_get_first_event(service_head.first_event);
	if (p_first_event == NULL)
		goto end;

	if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
		goto end;

	if (s_InvalidEpgNum > 0)
	{
		// skip the invalid event and the first shedule have got
		for (i=0; i<s_InvalidEpgNum; i++)
		{
			if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
				goto end;
		}
	}

	if (pos<=PF_NUMBER)// && s_InvalidEpgNum==0
	{
		p_epg->epg_info[PF_NUMBER-pos].event_id = event_head->event_id;
		p_epg->epg_info[PF_NUMBER-pos].start_time = event_head->time.start_time;
		p_epg->epg_info[PF_NUMBER-pos].duration = event_head->time.duration;
		p_epg->epg_info[PF_NUMBER-pos].event_name = p_text_addr;
		p_epg->epg_info[PF_NUMBER-pos].event_name_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(p_text_addr, &event_head->name, &limit_size);
#else
			epg_mempool_string_copy(p_text_addr, event_head->name, &limit_size);
#endif
		p_text_addr += p_epg->epg_info[PF_NUMBER-pos].event_name_len;

		p_epg->epg_info[PF_NUMBER-pos].event_detail = p_text_addr;
		p_epg->epg_info[PF_NUMBER-pos].event_detail_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(p_text_addr, &event_head->detaile_description, &limit_size);
#else
			epg_mempool_string_copy(p_text_addr, event_head->detaile_description, &limit_size);
#endif
		p_text_addr += p_epg->epg_info[PF_NUMBER-pos].event_detail_len;

		if (limit_size == 0){
			EPG_ERR("[epg]   2epg_info_size too small\n");
			goto end;
		}
		// one write success
		p_epg->get_event_count++;

		if (p_epg->get_event_count >= p_epg->want_event_count)
			goto end;
	}


	// the shedule event get
	for (i=3; i<s_EpgNum.real_event_count; i++)
	{
		if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
			goto end;

		if (i >= pos)
		{
			p_epg->epg_info[i-pos].event_id = event_head->event_id;
			p_epg->epg_info[i-pos].start_time = event_head->time.start_time;
			p_epg->epg_info[i-pos].duration = event_head->time.duration;

			p_epg->epg_info[i-pos].event_name = p_text_addr;
			p_epg->epg_info[i-pos].event_name_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &event_head->name, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, event_head->name, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[i-pos].event_name_len;

			p_epg->epg_info[i-pos].event_detail = p_text_addr;
			p_epg->epg_info[i -pos].event_detail_len =
#ifdef EPG_MALLOC
				epg_mempool_string_copy(p_text_addr, &event_head->detaile_description, &limit_size);
#else
				epg_mempool_string_copy(p_text_addr, event_head->detaile_description, &limit_size);
#endif
			p_text_addr += p_epg->epg_info[i -pos].event_detail_len;

			if (limit_size == 0){
				EPG_ERR("[epg]   3epg_info_size too small\n");
				goto end;
			}
			// one write success
			p_epg->get_event_count++;

			if (p_epg->get_event_count >= p_epg->want_event_count)
				goto end;
		}
	}

end:
	GxBus_EpgServiceContentRelease(&service_head);
	EPG_FREE(event_head);
	return;
}

/*
 * 只统计schedule里的epg数量
 * */
static void epg_service_epg_num_get_by_period(GxMsgProperty_EpgNumGetByPeriod *p_epg)
{
	GxBusEpgServiceHead service_head = {0};

	CHECK_NULL_POINT(p_epg);

	//gxlogd("sizeof(p_epg->event_per_day) value is : %d\n",sizeof(p_epg->event_per_day));

	// clr the count
	p_epg->real_event_count = 0;

	if (GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head) != GX_BUS_EPG_OK)
		return;

	// shedule number
	p_epg->real_event_count = epg_schedule_count_get_by_period(&service_head, p_epg->start_time, p_epg->end_time);

	GxBus_EpgServiceContentRelease(&service_head);
	return;
}

static status_t epg_info_copy_to_user(GxEpgInfo *epg_info, EpgInfoLevel level, uint16_t get_event_count, GxBusEpgEventHead *event_head, uint8_t **p_text_addr, uint32_t *limit_size)
{
	if(NULL == epg_info || NULL == event_head || NULL == *p_text_addr)
		return GX_BUS_EPG_ERR;

	epg_info[get_event_count].event_id = event_head->event_id;
	epg_info[get_event_count].start_time = event_head->time.start_time;
	epg_info[get_event_count].duration = event_head->time.duration;

	if(EPG_WITH_NAME_INFO == level || EPG_COMPLETE_INFO == level)
	{
		epg_info[get_event_count].event_name = *p_text_addr;
		epg_info[get_event_count].event_name_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &event_head->name, limit_size);
#else
			epg_mempool_string_copy(*p_text_addr, event_head->name, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_name_len;
	}

	if(EPG_COMPLETE_INFO == level)
	{
		epg_info[get_event_count].event_detail = *p_text_addr;
		epg_info[get_event_count].event_detail_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &event_head->detaile_description, limit_size);
#else
			epg_mempool_string_copy(*p_text_addr, event_head->detaile_description, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_detail_len;
	}

	if(0 == *limit_size && EPG_BASIC_INFO != level)
	{
		EPG_ERR("[epg]   3epg_info_size too small\n");
		return GX_BUS_EPG_ERR;
	}

	return GX_BUS_EPG_OK;
}

static void epg_service_epg_pf_num_get(GxMsgProperty_EpgPFNumGet *p_epg)
{
	GxBusEpgServiceHead service_head = {0};

	CHECK_NULL_POINT(p_epg);
	// clr the count
	p_epg->real_event_count = 0;

	if(GX_BUS_EPG_OK != GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head))
		return;

	// present
#ifdef EPG_MALLOC
	if(service_head.present.name.content != NULL)
#else
	if(service_head.present.name != NULL)
#endif
	{
		p_epg->real_event_count++;
	}

	// follow
#ifdef EPG_MALLOC
	if(service_head.follow.name.content != NULL)
#else
	if(service_head.follow.name != NULL)
#endif
	{
		p_epg->real_event_count++;
	}

	GxBus_EpgServiceContentRelease(&service_head);
	return;
}

static status_t epg_present_info_copy_to_user(GxEpgInfo *epg_info, uint16_t get_event_count, GxBusEpgServiceHead *service_head, uint8_t **p_text_addr, uint32_t *limit_size)
{
	if(NULL == epg_info || NULL == *p_text_addr|| NULL == service_head)
		return GX_BUS_EPG_ERR;

#ifdef EPG_MALLOC
	if(service_head->present.name.content != NULL)
#else
	if(service_head->present.name != NULL)
#endif
	{
		epg_info[get_event_count].event_id = service_head->present.event_id;
		epg_info[get_event_count].start_time = service_head->present.time.start_time;
		epg_info[get_event_count].duration = service_head->present.time.duration;

		epg_info[get_event_count].event_name = *p_text_addr;
		epg_info[get_event_count].event_name_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &service_head->present.name, limit_size);
#else
		epg_mempool_string_copy(*p_text_addr, service_head->present.name, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_name_len;

		epg_info[get_event_count].event_detail = *p_text_addr;
		epg_info[get_event_count].event_detail_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &service_head->present.detaile_description, limit_size);
#else
		epg_mempool_string_copy(*p_text_addr, service_head->present.detaile_description, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_detail_len;
	}

	if(*limit_size == 0){
		EPG_ERR("[epg]   2epg_info_size too small\n");
		return GX_BUS_EPG_ERR;
	}

	return GX_BUS_EPG_OK;
}

static status_t epg_follow_info_copy_to_user(GxEpgInfo *epg_info, uint16_t get_event_count, GxBusEpgServiceHead *service_head, uint8_t **p_text_addr, uint32_t *limit_size)
{
	if(NULL == epg_info || NULL == *p_text_addr|| NULL == service_head)
		return GX_BUS_EPG_ERR;

#ifdef EPG_MALLOC
	if(service_head->follow.name.content != NULL)
#else
	if(service_head->follow.name != NULL)
#endif
	{
		epg_info[get_event_count].event_id = service_head->follow.event_id;
		epg_info[get_event_count].start_time = service_head->follow.time.start_time;
		epg_info[get_event_count].duration = service_head->follow.time.duration;

		epg_info[get_event_count].event_name = *p_text_addr;
		epg_info[get_event_count].event_name_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &service_head->follow.name, limit_size);
#else
		epg_mempool_string_copy(*p_text_addr, service_head->follow.name, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_name_len;

		epg_info[get_event_count].event_detail = *p_text_addr;
		epg_info[get_event_count].event_detail_len =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(*p_text_addr, &service_head->follow.detaile_description, limit_size);
#else
		epg_mempool_string_copy(*p_text_addr, service_head->follow.detaile_description, limit_size);
#endif
		*p_text_addr += epg_info[get_event_count].event_detail_len;
	}

	if(*limit_size == 0){
		EPG_ERR("[epg]   2epg_info_size too small\n");
		return GX_BUS_EPG_ERR;
	}

	return GX_BUS_EPG_OK;
}

/*
 * 只获取当前后续的epg事件
 * */
static void epg_service_epg_pf_get(GxMsgProperty_EpgPFGet *p_epg)
{
	uint32_t limit_size = 0;
	GxBusEpgServiceHead service_head = {0};
	uint8_t *p_text_addr = NULL; // used to store event_name and event_detail

	CHECK_NULL_POINT(p_epg);
	// clr the count
	p_epg->get_event_count = 0;

	if(GX_BUS_EPG_OK != GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head))
		return;

	// no space to store the head info of epg
	if(sizeof(GxEpgInfo) > p_epg->epg_info_size)
	{
		EPG_ERR("[epg]   1epg_info_size too small\n");
		goto end;
	}

	// the space can save name and detail
	limit_size = p_epg->epg_info_size - sizeof(GxEpgInfo);
	p_text_addr = (uint8_t*)(p_epg->epg_info) + sizeof(GxEpgInfo);

	if(EPG_PRESENT_EVENT == p_epg->type)
	{
		// present
		if(GX_BUS_EPG_OK != epg_present_info_copy_to_user(p_epg->epg_info, p_epg->get_event_count, &service_head, &p_text_addr, &limit_size))
			goto end;
		p_epg->get_event_count++;
	}
	else
	{
		// follow
		if(GX_BUS_EPG_OK != epg_follow_info_copy_to_user(p_epg->epg_info, p_epg->get_event_count, &service_head, &p_text_addr, &limit_size))
			goto end;
		p_epg->get_event_count++;
	}

end:
	GxBus_EpgServiceContentRelease(&service_head);
	return;
}

/*
 * 只获取schedule里的epg事件
 * */
static void epg_service_epg_get_by_period(GxMsgProperty_EpgGetByPeriod *p_epg)
{
	uint32_t limit_size = 0;
	GxBusEpgServiceHead service_head = {0};
	GxBusEpgEventHead *event_head = NULL;
	GxBusEpgFirstEvent *p_first_event = NULL;
	uint8_t *p_text_addr = NULL; // used to store event_name and event_detail
	uint16_t i = 0;
	int32_t per_event_size = PER_EVENT_SIZE;
	time_t event_start_time = 0;
	time_t event_end_time = 0;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_NULL_POINT(p_epg);
	CHECK_NULL_POINT(p_epg->epg_info);

	// clr the count
	p_epg->get_event_count = 0;

	if(GX_BUS_EPG_OK != GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head))
		return;

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		goto end;

	// no space to store the head info of epg
	if(p_epg->want_event_count*sizeof(GxEpgInfo) > p_epg->epg_info_size)
	{
		EPG_ERR("[epg]   1epg_info_size too small\n");
		goto end;
	}

	// the space can save name and detail
	limit_size = p_epg->epg_info_size - p_epg->want_event_count*sizeof(GxEpgInfo);
	p_text_addr = (uint8_t *)(p_epg->epg_info) + p_epg->want_event_count*sizeof(GxEpgInfo);

	// the first shedule event get
	p_first_event = epg_get_first_event(service_head.first_event);
	if (p_first_event == NULL)
		goto end;

	if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
		goto end;

	event_start_time = event_head->time.start_time;
	event_end_time = event_head->time.start_time + event_head->time.duration;
	if(event_start_time < p_epg->end_time && event_end_time > p_epg->start_time)
	{
		if(GX_BUS_EPG_OK != epg_info_copy_to_user(p_epg->epg_info, p_epg->level, p_epg->get_event_count, event_head, &p_text_addr, &limit_size))
			goto end;
		p_epg->get_event_count++;
	}

	// the shedule event get
	for (i = 1; i < EpgCfg.event_count; i++)
	{
		if(p_epg->get_event_count >= p_epg->want_event_count)
			goto end;

		if(event_start_time > p_epg->end_time || GX_BUS_EPG_INVALID_POS == event_head->next_event)
			goto end;

		if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
			goto end;

		event_start_time = event_head->time.start_time;
		event_end_time = event_head->time.start_time + event_head->time.duration;
		if(event_start_time < p_epg->end_time && event_end_time > p_epg->start_time)
		{
			if(GX_BUS_EPG_OK != epg_info_copy_to_user(p_epg->epg_info, p_epg->level, p_epg->get_event_count, event_head, &p_text_addr, &limit_size))
				goto end;
			p_epg->get_event_count++;
		}
	}

end:
	GxBus_EpgServiceContentRelease(&service_head);
	EPG_FREE(event_head);
	return;
}

static void epg_service_epg_get_by_event_id(GxMsgProperty_EpgGetByEventId *p_epg)
{
	uint32_t limit_size = 0;
	GxBusEpgServiceHead service_head = {0};
	GxBusEpgEventHead *event_head = NULL;
	GxBusEpgFirstEvent *p_first_event = NULL;
	uint8_t *p_text_addr = NULL; // used to store event_name and event_detail
	uint16_t i = 0;
	int32_t per_event_size = PER_EVENT_SIZE;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_NULL_POINT(p_epg);
	CHECK_NULL_POINT(p_epg->epg_info);

	// clr the count
	p_epg->get_event_count = 0;

	if(GX_BUS_EPG_OK != GxBus_EpgServiceGet(p_epg->ts_id, p_epg->service_id, p_epg->orig_network_id, &service_head))
		return;

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		goto end;

	// no space to store the head info of epg
	if(sizeof(GxEpgInfo) > p_epg->epg_info_size)
	{
		EPG_ERR("[epg]   1epg_info_size too small\n");
		goto end;
	}

	// the space can save name and detail
	limit_size = p_epg->epg_info_size - sizeof(GxEpgInfo);
	p_text_addr = (uint8_t *)(p_epg->epg_info) + sizeof(GxEpgInfo);

#ifdef EPG_MALLOC
	if(service_head.present.name.content != NULL)
#else
	if(service_head.present.name != NULL)
#endif
	{
		if(service_head.present.event_id == p_epg->event_id)
		{
			if(GX_BUS_EPG_OK == epg_present_info_copy_to_user(p_epg->epg_info, p_epg->get_event_count, &service_head, &p_text_addr, &limit_size))
				p_epg->get_event_count++;
			goto end;
		}
	}

#ifdef EPG_MALLOC
	if(service_head.follow.name.content != NULL)
#else
	if(service_head.follow.name != NULL)
#endif
	{
		if(service_head.follow.event_id == p_epg->event_id)
		{
			if(GX_BUS_EPG_OK == epg_follow_info_copy_to_user(p_epg->epg_info, p_epg->get_event_count, &service_head, &p_text_addr, &limit_size))
				p_epg->get_event_count++;
			goto end;
		}
	}

	// the first shedule event get
	p_first_event = epg_get_first_event(service_head.first_event);
	if (p_first_event == NULL)
		goto end;

	if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
		goto end;

	if(event_head->event_id == p_epg->event_id)
	{
		if(GX_BUS_EPG_OK != epg_info_copy_to_user(p_epg->epg_info, p_epg->level, p_epg->get_event_count, event_head, &p_text_addr, &limit_size))
			goto end;
		p_epg->get_event_count++;
	}
	else
	{
		// the shedule event get
		for (i = 1; i < EpgCfg.event_count; i++)
		{
			if(GX_BUS_EPG_INVALID_POS == event_head->next_event)
				goto end;

			if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
				goto end;

			if(event_head->event_id == p_epg->event_id)
			{
				if(GX_BUS_EPG_OK != epg_info_copy_to_user(p_epg->epg_info, p_epg->level, p_epg->get_event_count, event_head, &p_text_addr, &limit_size))
					goto end;

				p_epg->get_event_count++;
				break;
			}
		}
	}

end:
	GxBus_EpgServiceContentRelease(&service_head);
	EPG_FREE(event_head);
	return;
}

static void epg_service_nvod_get(GxMsgProperty_EpgNvodGet *p_nvod)
{
	uint32_t limit_size = 0;
	uint32_t nvod_count = 0;
	GxBusEpgServiceHead service_head = {0};
	GxBusEpgEventHead *event_head = NULL;
	GxBusEpgEventHead *nvod_event = NULL;
	GxBusEpgFirstEvent *p_first_event = NULL;
	uint8_t *p_text_addr = NULL; // used to store event_name and event_detail
	uint32_t i = 0;
	int32_t per_event_size = PER_EVENT_SIZE;
	int32_t per_nvod_size = PER_EVENT_SIZE;
	status_t ret = GX_BUS_EPG_OK;

	CHECK_NULL_POINT(p_nvod);
	CHECK_NULL_POINT(p_nvod->nvod_info);

	// clr the count
	p_nvod->get_nvod_count = 0;

	if (GxBus_EpgServiceGet(p_nvod->ts_id, p_nvod->service_id, p_nvod->orig_network_id, &service_head) != GX_BUS_EPG_OK)
		return;

	event_head = GxCore_Calloc(per_event_size, sizeof(char));
	if(NULL == event_head)
		goto end;

	nvod_event = GxCore_Calloc(per_nvod_size, sizeof(char));
	if(NULL == nvod_event)
		goto end;

	nvod_count = epg_schedule_count_get(&service_head);

	limit_size = p_nvod->nvod_info_size;

	p_first_event = epg_get_first_event(service_head.first_event);
	if (p_first_event == NULL)
		goto end;

	for (i=1; i<nvod_count; i++)
	{
		if(0 == i)
		{
			if(GX_BUS_EPG_OK != epg_one_event_get(p_first_event->pos, &event_head, &per_event_size))
				goto end;
		}
		else
		{
			if(GX_BUS_EPG_OK != epg_one_event_get(event_head->next_event, &event_head, &per_event_size))
				goto end;
		}

		if(GX_BUS_EPG_OK != epg_one_nvod_get(service_head.ts_id, service_head.reference_service_id,
											event_head->reference_event_id, &nvod_event, &per_nvod_size))
			goto end;

		if(p_text_addr == NULL)
			p_text_addr = (uint8_t*)(p_nvod->nvod_info) + nvod_count*sizeof(GxEpgInfo);

		p_nvod->nvod_info[p_nvod->get_nvod_count].start_time = event_head->time.start_time;
		p_nvod->nvod_info[p_nvod->get_nvod_count].duration   = event_head->time.duration;
		p_nvod->nvod_info[p_nvod->get_nvod_count].event_id   = event_head->event_id;

		p_nvod->nvod_info[p_nvod->get_nvod_count].event_name         = p_text_addr;
		p_nvod->nvod_info[p_nvod->get_nvod_count].event_name_len     =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(p_text_addr, &nvod_event->name, &limit_size);
#else
			epg_mempool_string_copy(p_text_addr, nvod_event->name, &limit_size);
#endif
		p_text_addr += p_nvod->nvod_info[p_nvod->get_nvod_count].event_name_len;

		p_nvod->nvod_info[p_nvod->get_nvod_count].event_detail		 = p_text_addr;
		p_nvod->nvod_info[p_nvod->get_nvod_count].event_detail_len	 =
#ifdef EPG_MALLOC
			epg_mempool_string_copy(p_text_addr, &nvod_event->detaile_description, &limit_size);
#else
			epg_mempool_string_copy(p_text_addr, nvod_event->detaile_description, &limit_size);
#endif
		p_text_addr += p_nvod->nvod_info[p_nvod->get_nvod_count].event_detail_len;
		p_nvod->nvod_info[p_nvod->get_nvod_count].reference_event_id = event_head->reference_event_id;
		p_nvod->nvod_info[p_nvod->get_nvod_count].reference_service_id = service_head.reference_service_id;
		EPG_DBG("nvod_get=%d,%d,%d,%ld,%s\n",p_nvod->service_id,p_nvod->nvod_info[p_nvod->get_nvod_count].reference_event_id,
				p_nvod->nvod_info[p_nvod->get_nvod_count].event_id,
				(unsigned long)p_nvod->nvod_info[p_nvod->get_nvod_count].start_time,
				p_nvod->nvod_info[p_nvod->get_nvod_count].event_name);


		if (limit_size == 0) {
			EPG_ERR("[nvod]   nvod_info_size too small\n");
			goto end;
		}
		// one write success
		p_nvod->get_nvod_count++;
	}

end:
	GxBus_EpgServiceContentRelease(&service_head);
	EPG_FREE(event_head);
	EPG_FREE(nvod_event);
	return;
}

static void epg_cfg_update(GxMsgProperty_EpgCreate* p_epg_cfg)
{
	GxMsgProperty_EpgCreate *p_cfg = &EpgCfg;

	CHECK_NULL_POINT(p_epg_cfg);

	if (p_epg_cfg->service_count != 0
			&& p_epg_cfg->event_count != 0
			&& p_epg_cfg->event_size != 0
			&& p_epg_cfg->epg_day != 0)
	{
		memcpy(p_cfg, p_epg_cfg, sizeof(GxMsgProperty_EpgCreate));
	}
	else
	{
		EPG_ERR("epg cfg parameter error\n");
	}

	return;
}

uint8_t epg_cfg_get_filter_level(void)
{
	return EpgDetailFilter.filter_level;
}

static void epg_service_epg_create(GxMsgProperty_EpgCreate* p_epg_cfg)
{
	if(true == s_EpgCreateFlag)
	{
		EPG_ERR("epg has been created\n");
		return;
	}

	s_EpgCreateFlag = true;
	epg_cfg_update(p_epg_cfg);

	if (GxBus_EpgInit(EpgCfg.service_count, EpgCfg.event_count,
				EpgCfg.event_size, EpgCfg.epg_day) != GX_BUS_EPG_OK)
	{
		EPG_ERR("epg init failed!\n");
		return;
	}

	epg_section_status_init();
	epg_cache_init(EpgCfg.cache_gate);

	// get channel for epg
	GxCore_MutexLock(s_EpgSubtCtrl.mutex);
	GxEpg_SubtChannelCreate(EpgCfg);
	GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
}

static void epg_detail_filter_update(GxMsgProperty_EpgDetailFilterSwitch* p_epg_detailfilter)
{
	GxMsgProperty_EpgDetailFilterSwitch *p_detailfilter = &EpgDetailFilter;

	CHECK_NULL_POINT(p_epg_detailfilter);
	memcpy(p_detailfilter, p_epg_detailfilter, sizeof(GxMsgProperty_EpgDetailFilterSwitch));
	epg_section_status_service_clear(p_detailfilter->ts_id,p_detailfilter->service_id);
	return;
}

static void epg_detail_filter_clean(void)
{
	uint16_t ts_id = 0;
	uint16_t service_id = 0;
	GxMsgProperty_EpgDetailFilterSwitch *p_detailfilter = &EpgDetailFilter;
	ts_id = p_detailfilter->ts_id;
	service_id = p_detailfilter->service_id;
	p_detailfilter->ts_id = 0xffff;
	p_detailfilter->orig_network_id = 0xffff;
	p_detailfilter->service_id = 0xffff;
	p_detailfilter->event_id = 0xffff;
	epg_section_status_service_clear(ts_id,service_id);
	return;
}

static void epg_service_epg_clean(void)
{
	epg_cache_clean();
	epg_section_status_release();

	if (GxBus_EpgClean() != GX_BUS_EPG_OK)
	{
		EPG_ERR("!!!!!!!!Epg Db Clean Failed!!!!!!!\n");
		return;
	}

	return;
}

static void epg_service_epg_release(void)
{
	if(false == s_EpgCreateFlag)
	{
		EPG_ERR("!!!!!!epg haven't been created!!!!!!!\n");
		return;
	}

	s_EpgCreateFlag = false;

	GxCore_MutexLock(s_EpgSubtCtrl.mutex);
	GxEpg_SubtChannelStop(EpgCfg.dmx_id);
	GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);

	epg_cache_clean();
	epg_section_status_release();
	epg_cfg_init();

	if (GxBus_EpgRelease() != GX_BUS_EPG_OK)
	{
		EPG_ERR("!!!!!!!!Epg Db Release Failed!!!!!!!\n");
		return;
	}

	GxBus_MessageEmpty(s_EpgServiceHandle);

	return;
}

static void epg_service_epg_reset(void)
{
	GxCore_MutexLock(s_EpgSubtCtrl.mutex);
	GxEpg_SubtChannelStop(EpgCfg.dmx_id);
	GxEpg_SubtChannelCreate(EpgCfg);
	GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);

	return;
}

void epg_filter_full_speed(uint16_t tid_value)
{
	uint16_t i;
	uint32_t channel_count = 0;
	GxEpgSubtInfo *p_subt_info = NULL;

	GxCore_MutexLock(s_EpgSubtCtrl.mutex);
	channel_count = GxEpg_SubtChannelNumGet();

	p_subt_info = s_EpgSubtCtrl.subtInfo;
	for (i = 0; i < channel_count; i++)
	{
		if(0 == p_subt_info[i].subtHandle)
		{
			GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			return;
		}

		if ((tid_value == 0xffff) || (p_subt_info[i].table_id == tid_value))
			GxEpg_SubtChannelFullSpeed(&p_subt_info[i], &EpgCfg);
	}

	GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
	return;
}

// parental rating get and send message
static void epg_send_parental_rating(GxMsgProperty_EpgParentalInfo * parental_send)
{
	GxMessage   *new_msg = NULL;
	GxMsgProperty_EpgParentalInfo *parental = NULL;

	if (parental_send == NULL)  return;

	new_msg = GxBus_MessageNew(GXMSG_EPG_PARENTAL_SEND);
	parental = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_EpgParentalInfo);

	memcpy(parental, parental_send, sizeof(GxMsgProperty_EpgParentalInfo));

	GxBus_MessageSend(new_msg);
}

//get the offset of extern event, for merge short and extern event
//return tag code lenth, if result is -1, extern event is NULL
//if return is 0, the 1st situation: don't need offset; 2nd situation: don't know offset how many bytes, so don't offset first
static int epg_event_merge_offset(uint8_t *iso_str, uint8_t *short_event, uint8_t *extern_event)
{
	uint8_t code_tag_short = 0;
	uint8_t code_tag_ext = 0;
	int32_t offset = -1;

	if(extern_event == NULL)
		return -1;

	if(short_event == NULL)
		return 0;

	code_tag_short = short_event[0];
	code_tag_ext = extern_event[0];

	if ((code_tag_short >= 0x00) && (code_tag_short <= 0x1f)) //has first  code
	{
		if (code_tag_short == code_tag_ext)
		{
			if(code_tag_short == 0x10) //0x10 for iso8859 1-9
			{
				if((extern_event[1] == short_event[1])
						&& (extern_event[2] == short_event[2]))
					offset = 3;
				else
					offset = 0;//don't know offset how many bytes
			}
			else
			{
				offset = 1;
			}
		}
		else
		{
			if((iso_str != NULL) && (language_code_cmp((char*)iso_str, (char*)ENG_LANG_STR) == 0))//eng
			{
				if ((code_tag_ext >= 0x00) && (code_tag_ext <= 0x1f)) //has first  code
					offset = 1;
				else
					offset = 0;
			}
			else
			{
				offset = 0;//don't know offset how many bytes
			}
		}
	}
	else
	{
		if(code_tag_ext >= 0x20)
		{
			offset = 0;
		}
		else
		{
			if((iso_str != NULL) && (language_code_cmp((char*)iso_str, (char*)ENG_LANG_STR) == 0))//eng
				offset = 1;
			else
				offset = 0;//don't know offset how many bytes
		}
	}

	return offset;
}

static status_t epg_add_parental_info(GxBusEpgEventAddInfo *event_add_info,
										EitInfo *p_eit_info,
										uint32_t event_count)
{
	uint32_t i = 0;
	int32_t ret = GXCORE_ERROR;

	// parental rating
	if(p_eit_info->si_info.sec_number == 0 && p_eit_info->event_info[event_count].parental_rating_count > 0)
	{
		uint32_t tp_id, prog_id, ts_id, service_id;

		event_add_info->parental_rating = GxCore_Calloc(1, p_eit_info->event_info[event_count].parental_rating_count\
				*sizeof(GxBusEpgParentalRating));
		if(event_add_info->parental_rating != NULL)
		{
			event_add_info->parental_rating_num = p_eit_info->event_info[event_count].parental_rating_count;
			for(i = 0; i < event_add_info->parental_rating_num; i++)
			{
#define COUNTRY_CODE_LEN    (3)
				memcpy(event_add_info->parental_rating[i].country,
						p_eit_info->event_info[event_count].parental_rating_des[i].country_code, COUNTRY_CODE_LEN);
				event_add_info->parental_rating[i].rating =  p_eit_info->event_info[event_count].parental_rating_des[i].rating;
			}
			if(EpgCfg.ts_id != 0xffffffff && EpgCfg.service_id != 0xffffffff)
			{
				ts_id = EpgCfg.ts_id;
				service_id = EpgCfg.service_id;
			}
			else
			{
				GxBus_PmViewInfoCurPlayGet(&tp_id, &prog_id, &ts_id, &service_id);
			}
			if (p_eit_info->service_id == service_id
					&& p_eit_info->si_info.ts_id == ts_id)
			{
				GxMsgProperty_EpgParentalInfo parental_get;

				parental_get.ts_id = ts_id;
				parental_get.orig_network_id = 0;
				parental_get.service_id = service_id;
                if (event_add_info->parental_rating_num > MAX_MSG_PARENTAL_RATING_NUM)
                {
                    EPG_ERR("Number of parental locks %d > 16  Some parental lock information lost !!!!!!!!!!!!\n",event_add_info->parental_rating_num);
                }
				parental_get.parental_rating_num = (event_add_info->parental_rating_num > MAX_MSG_PARENTAL_RATING_NUM) ? MAX_MSG_PARENTAL_RATING_NUM : event_add_info->parental_rating_num;
				memcpy(&(parental_get.parental_rating[0]), event_add_info->parental_rating, (parental_get.parental_rating_num * sizeof(GxBusEpgParentalRating)));

				epg_send_parental_rating(&parental_get);
			}
			ret = GXCORE_SUCCESS;
		}
	}

	return ret;
}

// if return success, need free the event name, details, parental_rating outside, if there aren't null;
static status_t epg_add_all_lang_event(GxBusEpgEventAddInfo *event_add_info,
										EitInfo *p_eit_info,
										uint32_t event_count)
{
#define LANGUAGE_CODE_LEN    (3)
	uint32_t i = event_count;
	uint16_t  j=0;
	uint16_t  k=0;
	int ret = GXCORE_SUCCESS;

	event_add_info->name = NULL;
	event_add_info->name_length = 0;
	event_add_info->detaile_description = NULL;
	event_add_info->detaile_length = 0;
	memset(event_add_info->language, 0, sizeof(event_add_info->language));

	if (p_eit_info->event_info[i].tms_event_des.reference_service_id!=0 && p_eit_info->event_info[i].tms_event_count != 0)
	{
		// NVOD
		event_add_info->reference_service_id = p_eit_info->event_info[i].tms_event_des.reference_service_id;
		event_add_info->reference_event_id = p_eit_info->event_info[i].tms_event_des.reference_event_id;
		// nvod info default as schedule
		memcpy(event_add_info->language, ALL_LANG_STR, LANGUAGE_CODE_LEN);
		event_add_info->type = GX_EVENT_DETAILE;
		return GXCORE_SUCCESS;
	}

	if (p_eit_info->event_info[i].short_event_count > 0) //short event
	{
		if(language_code_cmp((char*)ALL_LANG_STR, (char*)(EpgCfg.language)) == 0)
		{
			memcpy(event_add_info->language, ALL_LANG_STR, LANGUAGE_CODE_LEN);
			j = 0;
		}
		else
		{
			for (j = 0; j < p_eit_info->event_info[i].short_event_count; j++)
			{
				if (language_code_cmp((char*)(p_eit_info->event_info[i].short_event_des[j].language_code),
							(char*)(EpgCfg.language)) == 0)
				{
					memcpy(event_add_info->language, p_eit_info->event_info[i].short_event_des[j].language_code, LANGUAGE_CODE_LEN);
					break;
				}
			}
		}

		if (j < p_eit_info->event_info[i].short_event_count)
		{
			//name
			if(EpgCfg.txt_format == EPG_TXT_UTF8)//convert to utf8
			{
				if(gxgdi_iconv(p_eit_info->event_info[i].short_event_des[j].event_name,
							&event_add_info->name,
							p_eit_info->event_info[i].short_event_des[j].event_name_length,
							&event_add_info->name_length,
							p_eit_info->event_info[i].short_event_des[j].language_code) == 1)
				{
					event_add_info->name = NULL;
					event_add_info->name_length = 0;
				}
			}
			else if (EpgCfg.txt_format == EPG_TXT_ORIGINAL)
			{
				event_add_info->name = (uint8_t*)GxCore_Calloc(p_eit_info->event_info[i].short_event_des[j].event_name_length + 1, 1);
				if(event_add_info->name != NULL)
				{
					memcpy(event_add_info->name, p_eit_info->event_info[i].short_event_des[j].event_name, p_eit_info->event_info[i].short_event_des[j].event_name_length);
					event_add_info->name_length = p_eit_info->event_info[i].short_event_des[j].event_name_length;
				}
			}
			else if (EpgCfg.txt_format == EPG_TXT_PRIVATE && EpgCfg.ttx_private != NULL)
			{
				uint8_t *p = NULL;
				p = EpgCfg.ttx_private(p_eit_info->event_info[i].short_event_des[j].event_name,
						p_eit_info->event_info[i].short_event_des[j].event_name_length);
				if (p != NULL)
				{
					if(strlen((const char *)p) != 0)
					{
						event_add_info->name = (uint8_t *)GxCore_Strdup((const char *)p);
						event_add_info->name_length = strlen((const char *)p);
					}
					GxCore_Free(p);
				}
			}

			//short event
			if((EpgDetailFilter.filter_level == 1 )
					&& (EpgDetailFilter.ts_id != p_eit_info->si_info.ts_id
					|| EpgDetailFilter.service_id != p_eit_info->service_id
					|| EpgDetailFilter.orig_network_id != p_eit_info->orig_network_id
					|| EpgDetailFilter.event_id != p_eit_info->event_info[i].event_id))
				goto finish_parese_epg;
			if(EpgCfg.txt_format == EPG_TXT_UTF8)//convert to utf8
			{
				if(gxgdi_iconv(p_eit_info->event_info[i].short_event_des[j].text,
							&event_add_info->detaile_description,
							p_eit_info->event_info[i].short_event_des[j].text_length,
							&event_add_info->detaile_length,
							p_eit_info->event_info[i].short_event_des[j].language_code) == 1)
				{
					event_add_info->detaile_description = NULL;
					event_add_info->detaile_length = 0;
				}
				else
					event_add_info->detaile_length --;//iconv 最后字节增加了一个'\0',拼接有问题
			}
			else if (EpgCfg.txt_format == EPG_TXT_ORIGINAL)
			{
				if(p_eit_info->event_info[i].short_event_des[j].text_length != 0)
				{
					event_add_info->detaile_description = (uint8_t*)GxCore_Calloc(p_eit_info->event_info[i].short_event_des[j].text_length + 1, 1);
					if(event_add_info->detaile_description != NULL)
					{
						memcpy(event_add_info->detaile_description, p_eit_info->event_info[i].short_event_des[j].text, p_eit_info->event_info[i].short_event_des[j].text_length);
						event_add_info->detaile_length = p_eit_info->event_info[i].short_event_des[j].text_length;
					}
				}
				else
				{
					event_add_info->detaile_description = NULL;
					event_add_info->detaile_length = 0;
				}
			}
			else if (EpgCfg.txt_format == EPG_TXT_PRIVATE && EpgCfg.ttx_private != NULL)
			{
				if(p_eit_info->event_info[i].short_event_des[j].text_length != 0)
				{
					uint8_t *p = NULL;
					p = EpgCfg.ttx_private(p_eit_info->event_info[i].short_event_des[j].text,
							p_eit_info->event_info[i].short_event_des[j].text_length);
					if (p != NULL)
					{
						if(strlen((const char *)p) != 0)
						{
							event_add_info->detaile_description = (uint8_t *)GxCore_Strdup((const char *)p);
							event_add_info->detaile_length = strlen((const char *)p);
						}
						GxCore_Free(p);
					}
				}
				else
				{
					event_add_info->detaile_description = NULL;
					event_add_info->detaile_length = 0;
				}
			}
		}
	}

	if(p_eit_info->event_info[i].extended_event_count > 0)//extern event
	{
#define SEPARATOR_LEN   (1) //LF
		uint8_t *lang_str = NULL;
		uint8_t *p_ext = NULL;
		uint32_t ext_len = 0;
		uint8_t *p_details = NULL;
		uint32_t details_len = 0;
		int code_tag_offset = 0;
		char language_code[4] = {0};
		char splice = 0;

		if(event_add_info->detaile_length > 0) //copy short event
		{
			p_details = (uint8_t*)GxCore_Calloc(event_add_info->detaile_length + SEPARATOR_LEN + 1, 1);
			if(p_details != NULL)
			{
				if(event_add_info->detaile_description != NULL) //shrot event
				{
					memcpy(p_details, event_add_info->detaile_description, event_add_info->detaile_length);
					p_details[event_add_info->detaile_length] = '\n';//LF
					details_len = event_add_info->detaile_length + SEPARATOR_LEN;
					GxCore_Free(event_add_info->detaile_description);
					event_add_info->detaile_description = NULL;
					event_add_info->detaile_length = 0;
				}
			}
		}
		else if(event_add_info->detaile_description != NULL)//short event is only \0
		{
			GxCore_Free(event_add_info->detaile_description);
			event_add_info->detaile_description = NULL;
		}

		if(language_code_cmp((char*)ALL_LANG_STR, (char*)(EpgCfg.language)) == 0)
			lang_str = (uint8_t*)ALL_LANG_STR;
		else
			lang_str = EpgCfg.language;

		if(event_add_info->detaile_length == 0)
		{
			memcpy(event_add_info->language, lang_str, LANGUAGE_CODE_LEN);
		}

		memcpy(language_code, p_eit_info->event_info[i].extended_event_des[0].language_code, LANGUAGE_CODE_LEN);
		for (j = 0; j < p_eit_info->event_info[i].extended_event_count; j++)
		{
			if(language_code_cmp((char*)ALL_LANG_STR, (char*)(EpgCfg.language)) != 0)
			{
				if (language_code_cmp((char*)(p_eit_info->event_info[i].extended_event_des[j].language_code),
							(char*)(EpgCfg.language)) != 0)
				{
					continue;
				}
			}

			if(EpgCfg.txt_format != EPG_TXT_UTF8)
			{
				code_tag_offset = epg_event_merge_offset((uint8_t*)language_code, p_details,
						p_eit_info->event_info[i].extended_event_des[j].extended_text);


				GxBus_ConfigGet(SI_CONFIG_EVENT_DETAIL_SPLICE, &splice, sizeof(char), ")");
				if(p_eit_info->event_info[i].extended_event_des[j].extended_text[0] ==
						splice)//extended_text第一个字节是\0  extended_event_count又不是0是因为后面是ppv信息
				{
					code_tag_offset = 0;
				}
			}

			for(k = 0; k < p_eit_info->event_info[i].extended_event_des[j].item_count; k++)
			{
				if(EpgCfg.txt_format == EPG_TXT_UTF8)//convert to utf8
				{
					if(gxgdi_iconv(p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description,
								&p_ext,
								p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description_length,
								&ext_len,
								p_eit_info->event_info[i].extended_event_des[j].language_code) == 1)
					{
						p_ext = NULL;
						ext_len = 0;
					}
					else
					{
						ext_len--;
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_ORIGINAL)
				{
					ext_len = p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description_length;
					p_ext = GxCore_Malloc(ext_len);
					if(p_ext != NULL)
					{
						memcpy(p_ext,p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description,ext_len);
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_PRIVATE && EpgCfg.ttx_private != NULL)
				{
					p_ext = EpgCfg.ttx_private(p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description,
							p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_description_length);
					if (p_ext != NULL)
					{
						ext_len= strlen((const char *)p_ext);
					}
				}
				p_details = (uint8_t*)GxCore_Realloc(p_details, details_len + ext_len+1);
				if(p_details != NULL)
				{
					if(p_ext != NULL) //ext event
					{
						memcpy(p_details + details_len, p_ext, ext_len);
						details_len += ext_len+1;
						p_details[details_len-1] = ':';
						GxCore_Free(p_ext);
						p_ext = NULL;
					}
				}
				if(EpgCfg.txt_format == EPG_TXT_UTF8)//convert to utf8
				{
					if(gxgdi_iconv(p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item,
								&p_ext,
								p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_length,
								&ext_len,
								p_eit_info->event_info[i].extended_event_des[j].language_code) == 1)
					{
						p_ext = NULL;
						ext_len = 0;
					}
					else
					{
						ext_len--;
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_ORIGINAL)
				{
					ext_len = p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_length;
					p_ext = GxCore_Malloc(ext_len);
					if(p_ext != NULL)
					{
						memcpy(p_ext,p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item,
						ext_len);
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_PRIVATE && EpgCfg.ttx_private != NULL)
				{
					p_ext = EpgCfg.ttx_private(p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item,
							p_eit_info->event_info[i].extended_event_des[j].extended_item_info[k].item_length);
					if (p_ext != NULL)
					{
						ext_len= strlen((const char *)p_ext);
					}
				}
				p_details = (uint8_t*)GxCore_Realloc(p_details, details_len + ext_len+1);
				if(p_details != NULL)
				{
					if(p_ext != NULL) //ext event
					{
						memcpy(p_details + details_len, p_ext, ext_len);
						details_len += ext_len+1;
						p_details[details_len-1] = '\n';
						GxCore_Free(p_ext);
						p_ext = NULL;
					}
				}
			}

			if(code_tag_offset >= 0)
			{
				if(EpgCfg.txt_format == EPG_TXT_UTF8)//convert to utf8
				{
					if(gxgdi_iconv(p_eit_info->event_info[i].extended_event_des[j].extended_text,
								&p_ext,
								p_eit_info->event_info[i].extended_event_des[j].extended_text_length,
								&ext_len,
								p_eit_info->event_info[i].extended_event_des[j].language_code) == 1)
					{
						p_ext = NULL;
						ext_len = 0;
					}
					else
					{
						ext_len--;
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_ORIGINAL)
				{
					ext_len = p_eit_info->event_info[i].extended_event_des[j].extended_text_length - code_tag_offset;
					p_ext = GxCore_Malloc(ext_len);
					if(p_ext != NULL)
					{
						memcpy(p_ext,p_eit_info->event_info[i].extended_event_des[j].extended_text + code_tag_offset,
								ext_len);
						p_ext[ext_len-1] = 0;
					}
				}
				else if (EpgCfg.txt_format == EPG_TXT_PRIVATE && EpgCfg.ttx_private != NULL)
				{
					p_ext = EpgCfg.ttx_private(p_eit_info->event_info[i].extended_event_des[j].extended_text,
							p_eit_info->event_info[i].extended_event_des[j].extended_text_length);
					if (p_ext != NULL)
					{
						ext_len= strlen((const char *)p_ext);
					}
				}
				//最好是最后一次循环 补\0
				//但是前面可能会由于语言不符continue,导致任何一次拼接都有可能是最后一次循环
				//只好浪费点空间每次都+2
				p_details = (uint8_t*)GxCore_Realloc(p_details, details_len + ext_len + 2);

				if(p_details != NULL)
				{
					if(p_ext != NULL) //ext event
					{
						if(language_code_cmp((char*)(p_eit_info->event_info[i].extended_event_des[j].language_code),
									(char*)language_code) != 0)
						{
							//diffrent language
							p_details[details_len] = '\n';
							details_len += 1;
						}

						memcpy(p_details + details_len, p_ext, ext_len);
						details_len += ext_len;

						GxCore_Free(p_ext);
						p_ext = NULL;
					}
				}
			}

			memcpy(language_code, p_eit_info->event_info[i].extended_event_des[j].language_code, LANGUAGE_CODE_LEN);
		}

		if (details_len != 0) {
			p_details[details_len] = '\0';
			event_add_info->detaile_description = p_details;
			event_add_info->detaile_length = details_len;
		}
		else if(p_details != NULL)
		{
			GxCore_Free(p_details);
			p_details = NULL;
		}
	}
finish_parese_epg:
	if(0 == event_add_info->detaile_length && 0 == event_add_info->name_length)
	{
		if(event_add_info->name != NULL)
		{
			GxCore_Free(event_add_info->name);
		}
		event_add_info->name = NULL;
		event_add_info->name_length = 0;
		if(event_add_info->detaile_description != NULL)
		{
			GxCore_Free(event_add_info->detaile_description);
		}
		event_add_info->detaile_description = NULL;
		event_add_info->detaile_length = 0;

		// set language code "0000"
		memset(event_add_info->language, 0, sizeof(event_add_info->language));

		// parental rating set null
		event_add_info->parental_rating = NULL;
		event_add_info->parental_rating_num = 0;

		ret = GXCORE_ERROR;
	}

	return ret;
}

// subt_ok : if the function call in "epg_service_subtable_ok", set subt_ok=1, else set it 0
static status_t epg_service_section_ok(GxParseResult *p_parse_result)
{
	EitInfo *p_eit_info;
	GxBusEpgEventAddInfo event_add_info = {0};
	status_t ret = GXCORE_SUCCESS;
	uint32_t i = 0;

	CHECK_POINT(p_parse_result, GXCORE_ERROR);
	memset(&event_add_info, 0, sizeof(GxBusEpgEventAddInfo));
	EPG_DBG("[epg]    section ok!  tid --- 0x%x\n", p_parse_result->table_id);
	if (p_parse_result->table_id < 0x4e
			|| p_parse_result->table_id > 0x63)
	{
		return GXCORE_ERROR;
	}

	p_eit_info = (EitInfo*)(p_parse_result->parsed_data);

	ret = epg_event_type_get(p_parse_result, &(event_add_info.type));
	if(ret != GXCORE_SUCCESS)
	{
		EPG_DBG("[epg]: event type get err!\n");
		epg_section_status_clear(p_parse_result->table_id,
								p_eit_info->si_info.ts_id,
								p_eit_info->service_id,
								p_eit_info->si_info.sec_number);
		return GXCORE_ERROR;
	}
	// 1 get "service_id"
	// 2 use "service_id" to get "ts_id"

	event_add_info.service_id = p_eit_info->service_id;
	event_add_info.ts_id = p_eit_info->si_info.ts_id;
	event_add_info.original_id = p_eit_info->orig_network_id;

	for (i=0; i<p_eit_info->event_count; i++)
	{
		event_add_info.event_id = p_eit_info->event_info[i].event_id;
		event_add_info.ver = p_eit_info->si_info.ver_number;

		if(0x4e == p_parse_result->table_id && GXCORE_SUCCESS == epg_add_parental_info(&event_add_info, p_eit_info, i))
		{
			GxBus_EpgServiceParentalInfoAdd(&event_add_info);
			EPG_FREE(event_add_info.parental_rating);
			event_add_info.parental_rating_num = 0;
		}

		// struct first member's addr is the struct's addr, so give the start_time addr to event_time
		event_add_info.event_time = (GxBusEpgEventTime*)&(p_eit_info->event_info[i].start_time);

		if (epg_add_all_lang_event(&event_add_info, p_eit_info, i) == GXCORE_ERROR)
		{
			EPG_DBG("event detail and name are null");
			continue;
		}
		else
		{
			// EPG ADD and novd add
		}

		// novd : present and follow change to detail
		if ((event_add_info.type != GX_EVENT_DETAILE)
				&& (event_add_info.event_time->start_time == -1))
		{
			event_add_info.type = GX_EVENT_DETAILE;
		}

		//__ReAddEvent:
		ret = GxBus_EpgInfoAdd(&event_add_info);

		if (event_add_info.name != NULL)
		{
			GxCore_Free(event_add_info.name);
			event_add_info.name= NULL;
		}

		if (event_add_info.detaile_description != NULL)
		{
			GxCore_Free(event_add_info.detaile_description);
			event_add_info.detaile_description= NULL;
		}

		if (event_add_info.parental_rating != NULL)
		{
			GxCore_Free(event_add_info.parental_rating);
			event_add_info.parental_rating = NULL;
		}

		GxCore_ThreadYield();
		if (ret == GX_BUS_EPG_OK)
			continue;

		else if (ret == GX_BUS_EPG_SERVICE_FULL
				|| ret == GX_BUS_EPG_EVENT_FULL
				|| ret == GX_BUS_EPG_MEM_POOL_FULL)
		{
			GxMessage *new_msg = NULL;

			EPG_DBG("[epg] space full\n");

			GxBus_EpgInvalidInfoClean(0xffffffff, 0xffffffff, 0xffffffff);

			new_msg = GxBus_MessageNew(GXMSG_EPG_FULL);
			GxBus_MessageSend(new_msg);

		}
		else if (ret == GX_BUS_EPG_EVENT_EXIST)
		{
			GxMessage *new_msg = NULL;
			new_msg = GxBus_MessageNew(GXMSG_EPG_EXIST);
			GxBus_MessageSend(new_msg);
			continue;
		}
	}

	return GXCORE_SUCCESS;
}

static void epg_cache_init(uint8_t cache_gate)
{
	GxCore_MutexLock(s_EpgCacheCtrl.mutex);

	if(cache_gate < DEFAULT_GATE)
		s_EpgCacheCtrl.cache_gate = DEFAULT_GATE;
	else
		s_EpgCacheCtrl.cache_gate = cache_gate;

	GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);

	return;
}

static void epg_cache_clean(void)
{
	GxEpgCache *temp = NULL;

	GxCore_MutexLock(s_EpgCacheCtrl.mutex);
	if(NULL == s_EpgCacheCtrl.head && NULL == s_EpgCacheCtrl.tail)//cache空的
	{
		EPG_DBG("----The cache has been clean-----\n");
		GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
		return;
	}

	temp = (GxEpgCache *)s_EpgCacheCtrl.head;
	if(s_EpgCacheCtrl.head == s_EpgCacheCtrl.tail)//只有一个
	{
		GxCore_Free(temp->buffer);
		temp->buffer = NULL;
		GxCore_Free(temp);
		s_EpgCacheCtrl.head = NULL;
		s_EpgCacheCtrl.tail = NULL;
		s_EpgCacheCtrl.cache_count = 0;
		GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
		return;
	}

	while(temp != (GxEpgCache *)s_EpgCacheCtrl.tail)
	{
		GxCore_Free(temp->buffer);
		temp->buffer = NULL;
		temp = temp->next;
		GxCore_Free(temp->pre);
		temp->pre = NULL;
	}
	//释放尾巴
	GxCore_Free(temp->buffer);
	temp->buffer = NULL;
	GxCore_Free(temp);
	s_EpgCacheCtrl.head = NULL;
	s_EpgCacheCtrl.tail = NULL;
	s_EpgCacheCtrl.cache_count = 0;
	GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);

	return;
}

//从头开始加
static void epg_parse_data_add_to_cache(GxEpgCache *p)
{
	GxEpgCache *temp = NULL;
	GxCore_MutexLock(s_EpgCacheCtrl.mutex);
	if(NULL == s_EpgCacheCtrl.head && NULL == s_EpgCacheCtrl.tail)//p是第一个cache
	{
		s_EpgCacheCtrl.head = (void *)p;
		s_EpgCacheCtrl.tail = (void *)p;
		p->pre = NULL;
		p->next = NULL;
		s_EpgCacheCtrl.cache_count = 1;
	}
	else
	{
		if(s_EpgCacheCtrl.cache_gate == s_EpgCacheCtrl.cache_count)
		{
			EPG_DBG("!!!!!The cache count reach gate!!!!!\n");
			GxCore_Free(p->buffer);
			GxCore_Free(p);
			p = NULL;
			GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
			return;
		}
		temp = (GxEpgCache *)s_EpgCacheCtrl.head;
		temp->pre = (void *)p;
		p->next = (void *)temp;
		p->pre = NULL;
		s_EpgCacheCtrl.head = (void *)p;
		s_EpgCacheCtrl.cache_count++;
	}
	GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
	return;
}

//从尾巴开始取
static GxEpgCache* epg_parse_data_get_from_cache(void)
{
	GxEpgCache *temp = NULL;
	GxCore_MutexLock(s_EpgCacheCtrl.mutex);

	if(NULL == s_EpgCacheCtrl.tail)
	{
		EPG_DBG("Maybe parse data is released\n");
		s_EpgCacheCtrl.cache_count = 0;
		GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
		return NULL;
	}

	temp = (GxEpgCache *)s_EpgCacheCtrl.tail;
	if(NULL == temp->pre && NULL == temp->next)//只有一个cache
	{
		s_EpgCacheCtrl.head = NULL;
		s_EpgCacheCtrl.tail = NULL;
	}
	else
	{
		((GxEpgCache *)temp->pre)->next = NULL;
		s_EpgCacheCtrl.tail = (void *)(temp->pre);
	}
	s_EpgCacheCtrl.cache_count--;
	GxCore_MutexUnlock(s_EpgCacheCtrl.mutex);
	return temp;
}

static void epg_parse_seciton(uint8_t *parse_data_buf, uint32_t read_size)
{
	uint8_t *parsed_data = NULL;
	int32_t ret = 0, i = 0;
	uint32_t section_len = 0, table_id = 0;
	si_info_t eit_head = {0};

	parsed_data = (uint8_t *)GxCore_Calloc(LARGE_BUF_SIZE, sizeof(uint8_t));
	if(NULL == parsed_data)
	{
		EPG_ERR("!!!!!!GxCore_Calloc error!!!!!!\n");
		return;
	}

	for(i = 0; i < read_size;)
	{
		section_len = TODATA12(parse_data_buf[i + 1], parse_data_buf[i + 2]);
		table_id = GET_TABLE_ID(parse_data_buf);
		//当前后续不去重复
		if(0x4e != table_id && 0x4f != table_id)
		{
			ret = GxBus_EitHeadParse(&parse_data_buf[i], &eit_head);
			if(GXCORE_ERROR == ret)
				break;

			ret = check_section_exist(table_id, &eit_head);
			if(0 != ret)
			{
				i += (section_len + 3);
				continue;
			}
		}

		if(0 == GxDmxCheckSectionCrc(parse_data_buf + i))
			ret = GxBus_EpgParser(&parse_data_buf[i], section_len + 3, parsed_data);
		else
			ret = GXCORE_ERROR;

		if(GXCORE_ERROR == ret)
		{
			EPG_DBG("!!!!!!Parse/CheckSectionCrc Error!!!!!!\n");
			break;
		}

		GxParseResult parse_result = {0};
		parse_result.parsed_data = parsed_data;
		parse_result.table_id = GET_TABLE_ID(parse_data_buf);
		epg_service_section_ok(&parse_result);

		i += (section_len + 3);
	}

	GxCore_Free(parsed_data);
	parsed_data = NULL;
	return;
}

static void epg_parse_thread(void *arg)
{
	GxEpgCache *p = NULL;
	while(1)
	{
		GxCore_SemWait(s_EpgSem);

		p = epg_parse_data_get_from_cache();
		if(NULL == p)
			continue;

		epg_parse_seciton(p->buffer, p->size);
		GxCore_Free(p->buffer);
		GxCore_Free(p);
		GxCore_ThreadDelay(10);
	}
	return;
}

status_t GxEpgServiceInit(handle_t self,int priority_offset)
{
	handle_t sch;
	int i;

	s_EpgServiceHandle = self;

	GxBus_MessageRegister(GXMSG_EPG_NUM_GET, sizeof(GxMsgProperty_EpgNumGet));
	GxBus_MessageRegister(GXMSG_EPG_GET, sizeof(GxMsgProperty_EpgGet));
	GxBus_MessageRegister(GXMSG_EPG_PF_NUM_GET, sizeof(GxMsgProperty_EpgPFNumGet));
	GxBus_MessageRegister(GXMSG_EPG_PF_GET, sizeof(GxMsgProperty_EpgPFGet));
	GxBus_MessageRegister(GXMSG_EPG_NUM_GET_BY_PERIOD, sizeof(GxMsgProperty_EpgNumGetByPeriod));
	GxBus_MessageRegister(GXMSG_EPG_GET_BY_PERIOD, sizeof(GxMsgProperty_EpgGetByPeriod));
	GxBus_MessageRegister(GXMSG_EPG_GET_BY_EVENT_ID, sizeof(GxMsgProperty_EpgGetByEventId));
	GxBus_MessageRegister(GXMSG_EPG_CREATE, sizeof(GxMsgProperty_EpgCreate));
	GxBus_MessageRegister(GXMSG_EPG_CFG_GET, sizeof(GxMsgProperty_EpgCreate));
	GxBus_MessageRegister(GXMSG_EPG_FILTER_START, sizeof(GxMsgProperty_EpgCreate));
	GxBus_MessageRegister(GXMSG_EPG_FILTER_STOP, 0);
	GxBus_MessageRegister(GXMSG_EPG_RESET, 0);
	GxBus_MessageRegister(GXMSG_EPG_NVOD_GET, sizeof(GxMsgProperty_EpgNvodGet));
	GxBus_MessageRegister(GXMSG_EPG_CLEAN, 0);
	GxBus_MessageRegister(GXMSG_EPG_RELEASE,0);
	GxBus_MessageRegister(GXMSG_EPG_PARENTAL_GET, sizeof(GxMsgProperty_EpgParentalInfo));
	GxBus_MessageRegister(GXMSG_EPG_PARENTAL_SEND, sizeof(GxMsgProperty_EpgParentalInfo));
	GxBus_MessageRegister(GXMSG_EPG_FILTER_NUM_GET, sizeof(uint32_t));
	GxBus_MessageRegister(GXMSG_EPG_FILTER_INFO_GET, sizeof(GxMsgProperty_EpgFilterInfo));
	GxBus_MessageRegister(GXMSG_EPG_FILTER_MODIFY, sizeof(GxMsgProperty_EpgFilterModify));
	GxBus_MessageRegister(GXMSG_EPG_FULL, 0);
	GxBus_MessageRegister(GXMSG_EPG_EXIST, 0);
	GxBus_MessageRegister(GXMSG_EPG_TIMEOUT, 0);
	GxBus_MessageRegister(GXMSG_EPG_FULL_SPEED, 0);
	GxBus_MessageRegister(GXMSG_EPG_DETAIL_FILTER_SET, sizeof(GxMsgProperty_EpgDetailFilterSwitch));
	GxBus_MessageRegister(GXMSG_EPG_DETAIL_FILTER_CLEAN, 0);

	GxBus_MessageRegister(GXMSG_EPG_SECTION_FLAG_RELEASE, 0);
	GxBus_MessageListen(self, GXMSG_EPG_SECTION_FLAG_RELEASE);

	GxBus_MessageListen(self, GXMSG_EPG_NUM_GET);
	GxBus_MessageListen(self, GXMSG_EPG_GET);
	GxBus_MessageListen(self, GXMSG_EPG_NUM_GET_BY_PERIOD);
	GxBus_MessageListen(self, GXMSG_EPG_GET_BY_PERIOD);
	GxBus_MessageListen(self, GXMSG_EPG_GET_BY_EVENT_ID);
	GxBus_MessageListen(self, GXMSG_EPG_PF_NUM_GET);
	GxBus_MessageListen(self, GXMSG_EPG_PF_GET);
	GxBus_MessageListen(self, GXMSG_EPG_NVOD_GET);
	GxBus_MessageListen(self, GXMSG_EPG_CREATE);
	GxBus_MessageListen(self, GXMSG_EPG_CFG_GET);
	GxBus_MessageListen(self, GXMSG_EPG_FILTER_START);
	GxBus_MessageListen(self, GXMSG_EPG_FILTER_STOP);
	GxBus_MessageListen(self, GXMSG_EPG_RESET);
	GxBus_MessageListen(self, GXMSG_EPG_CLEAN);
	GxBus_MessageListen(self, GXMSG_EPG_RELEASE);
	GxBus_MessageListen(self, GXMSG_EPG_PARENTAL_GET);
	GxBus_MessageListen(self, GXMSG_EPG_FILTER_NUM_GET);
	GxBus_MessageListen(self, GXMSG_EPG_FILTER_INFO_GET);
	GxBus_MessageListen(self, GXMSG_EPG_FILTER_MODIFY);
	GxBus_MessageListen(self, GXMSG_EPG_FULL_SPEED);
	GxBus_MessageListen(self, GXMSG_EPG_DETAIL_FILTER_SET);
	GxBus_MessageListen(self, GXMSG_EPG_DETAIL_FILTER_CLEAN);

	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;
	for(i = 0; i < EPG_TID_NUM; i++)
	{
		p_subt_info[i].subtHandle = 0;
		p_subt_info[i].status = STOP_FILTER;
		p_subt_info[i].table_id = 0;
	}

	if(0 == s_EpgCacheCtrl.mutex)
		GxCore_MutexCreate(&s_EpgCacheCtrl.mutex);
	if(0 == s_EpgSubtCtrl.mutex)
		GxCore_MutexCreate(&s_EpgSubtCtrl.mutex);
	if(0 == s_EpgSectionCtrl.mutex)
		GxCore_MutexCreate(&s_EpgSectionCtrl.mutex);
	if(0 == s_EpgSem)
		GxCore_SemCreate(&s_EpgSem, 0);

	if(0 == s_EpgParseThreadHandle)
	{
		if(GXCORE_SUCCESS != GxCore_ThreadCreate("parse read", &s_EpgParseThreadHandle, epg_parse_thread, NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY))
			return GXCORE_ERROR;
	}
	sch = GxBus_SchedulerCreate("EpgMsgScheduler", GXBUS_SCHED_MSG, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	sch = GxBus_SchedulerCreate("EpgConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GXCORE_SUCCESS;
}

void GxEpgServiceDestroy(handle_t self)
{
	GxBus_MessageUnListen(self, GXMSG_EPG_NUM_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_PF_NUM_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_PF_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_NUM_GET_BY_PERIOD);
	GxBus_MessageUnListen(self, GXMSG_EPG_GET_BY_PERIOD);
	GxBus_MessageUnListen(self, GXMSG_EPG_GET_BY_EVENT_ID);
	GxBus_MessageUnListen(self, GXMSG_EPG_CREATE);
	GxBus_MessageUnListen(self, GXMSG_EPG_CFG_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_FILTER_START);
	GxBus_MessageUnListen(self, GXMSG_EPG_FILTER_STOP);
	GxBus_MessageUnListen(self, GXMSG_EPG_RESET);
	GxBus_MessageUnListen(self, GXMSG_EPG_CLEAN);
	GxBus_MessageUnListen(self, GXMSG_EPG_RELEASE);
	GxBus_MessageUnListen(self, GXMSG_EPG_PARENTAL_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_FILTER_NUM_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_FILTER_INFO_GET);
	GxBus_MessageUnListen(self, GXMSG_EPG_FILTER_MODIFY);
	GxBus_MessageUnListen(self, GXMSG_EPG_FULL_SPEED);
	GxBus_MessageUnListen(self, GXMSG_EPG_DETAIL_FILTER_SET);
	GxBus_MessageUnListen(self, GXMSG_EPG_DETAIL_FILTER_CLEAN);

	GxBus_MessageUnListen(self, GXMSG_EPG_SECTION_FLAG_RELEASE);
	GxBus_MessageUnregister(GXMSG_EPG_SECTION_FLAG_RELEASE);
	GxBus_MessageUnregister(GXMSG_EPG_NUM_GET);
	GxBus_MessageUnregister(GXMSG_EPG_GET);
	GxBus_MessageUnregister(GXMSG_EPG_CREATE);
	GxBus_MessageUnregister(GXMSG_EPG_CFG_GET);
	GxBus_MessageUnregister(GXMSG_EPG_NUM_GET_BY_PERIOD);
	GxBus_MessageUnregister(GXMSG_EPG_GET_BY_PERIOD);
	GxBus_MessageUnregister(GXMSG_EPG_GET_BY_EVENT_ID);
	GxBus_MessageUnregister(GXMSG_EPG_PF_NUM_GET);
	GxBus_MessageUnregister(GXMSG_EPG_PF_GET);
	GxBus_MessageUnregister(GXMSG_EPG_FILTER_START);
	GxBus_MessageUnregister(GXMSG_EPG_FILTER_STOP);
	GxBus_MessageUnregister(GXMSG_EPG_RESET);
	GxBus_MessageUnregister(GXMSG_EPG_CLEAN);
	GxBus_MessageUnregister(GXMSG_EPG_RELEASE);
	GxBus_MessageUnregister(GXMSG_EPG_PARENTAL_GET);
	GxBus_MessageUnregister(GXMSG_EPG_PARENTAL_SEND);
	GxBus_MessageUnregister(GXMSG_EPG_FILTER_NUM_GET);
	GxBus_MessageUnregister(GXMSG_EPG_FILTER_INFO_GET);
	GxBus_MessageUnregister(GXMSG_EPG_FILTER_MODIFY);
	GxBus_MessageUnregister(GXMSG_EPG_FULL);
	GxBus_MessageUnregister(GXMSG_EPG_EXIST);
	GxBus_MessageUnregister(GXMSG_EPG_TIMEOUT);
	GxBus_MessageUnregister(GXMSG_EPG_FULL_SPEED);
	GxBus_MessageUnregister(GXMSG_EPG_DETAIL_FILTER_SET);
	GxBus_MessageUnregister(GXMSG_EPG_DETAIL_FILTER_CLEAN);

	GxBus_ServiceUnlink(self);
	return;
}

GxMsgStatus GxEpgServiceRecvMsg(handle_t self, GxMessage* Msg)
{
#define EPG_GET_LOCK    (1)
#define EPG_GET_UNLOCK  (0)
	static uint32_t lock_flag = EPG_GET_UNLOCK;

	switch(Msg->msg_id)
	{
		case GXMSG_EPG_NUM_GET:
			{
				GxMsgProperty_EpgNumGet* epg_num_get = NULL;
				epg_num_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgNumGet);
				epg_service_epg_num_get(epg_num_get);
				break;
			}
		case GXMSG_EPG_GET:
			{
				GxMsgProperty_EpgGet* epg_get = NULL;
				epg_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgGet);
				epg_service_epg_get(epg_get);
				break;
			}
		case GXMSG_EPG_NUM_GET_BY_PERIOD:
			{
				GxMsgProperty_EpgNumGetByPeriod* epg_num_get_by_period = NULL;
				epg_num_get_by_period = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgNumGetByPeriod);
				epg_service_epg_num_get_by_period(epg_num_get_by_period);
				break;
			}
		case GXMSG_EPG_GET_BY_PERIOD:
			{
				GxMsgProperty_EpgGetByPeriod* epg_get_by_period = NULL;
				epg_get_by_period = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgGetByPeriod);
				epg_service_epg_get_by_period(epg_get_by_period);
				break;
			}
		case GXMSG_EPG_GET_BY_EVENT_ID:
			{
				GxMsgProperty_EpgGetByEventId* epg_get_by_event_id = NULL;
				epg_get_by_event_id = GxBus_GetMsgPropertyPtr(Msg, GxMsgProperty_EpgGetByEventId);
				epg_service_epg_get_by_event_id(epg_get_by_event_id);
				break;
			}
		case GXMSG_EPG_PF_NUM_GET:
			{
				GxMsgProperty_EpgPFNumGet* epg_pf_num_get = NULL;
				epg_pf_num_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgPFNumGet);
				epg_service_epg_pf_num_get(epg_pf_num_get);
				break;
			}
		case GXMSG_EPG_PF_GET:
			{
				GxMsgProperty_EpgPFGet* epg_pf_get = NULL;
				epg_pf_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgPFGet);
				epg_service_epg_pf_get(epg_pf_get);
				break;
			}
		case GXMSG_EPG_NVOD_GET:
			{
				GxMsgProperty_EpgNvodGet* nvod_get = NULL;
				nvod_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgNvodGet);
				epg_service_nvod_get(nvod_get);
				break;
			}
		case GXMSG_EPG_CREATE:
			{
				GxMsgProperty_EpgCreate* epg_create = NULL;
				epg_create = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgCreate);
				epg_service_epg_create(epg_create);
				break;
			}

		case GXMSG_EPG_RESET:
			epg_service_epg_reset();
			break;
		case GXMSG_EPG_CLEAN:
			epg_service_epg_clean();
			break;
		case GXMSG_EPG_RELEASE:
			epg_service_epg_release();
			break;
		case GXMSG_EPG_FULL_SPEED:
			epg_filter_full_speed(0xffff);
			break;
		case GXMSG_EPG_SECTION_FLAG_RELEASE:
			epg_section_status_release();
			break;
		case GXMSG_EPG_PARENTAL_GET:
			{
				GxMsgProperty_EpgParentalInfo* parental_get = NULL;
				parental_get = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgParentalInfo);
				epg_parental_rating_get(parental_get);
			}
			break;
		case GXMSG_EPG_GET_LOCK:
			lock_flag = EPG_GET_LOCK;
			break;
		case GXMSG_EPG_GET_UNLOCK:
			lock_flag = EPG_GET_UNLOCK;
			break;
		case GXMSG_EPG_CFG_GET:
			{
				GxMsgProperty_EpgCreate* epg_cfg = NULL;
				epg_cfg = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgCreate);
				memcpy(epg_cfg, &EpgCfg, sizeof(GxMsgProperty_EpgCreate));
			}
			break;
		case GXMSG_EPG_FILTER_START:
			{
				GxMsgProperty_EpgCreate* epg_cfg = NULL;

				epg_cfg = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgCreate);
				epg_cfg_update(epg_cfg);
				GxCore_MutexLock(s_EpgSubtCtrl.mutex);
				GxEpg_SubtChannelCreate(*epg_cfg);
				GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			}
			break;
		case GXMSG_EPG_FILTER_STOP:
			{
				GxCore_MutexLock(s_EpgSubtCtrl.mutex);
				GxEpg_SubtChannelStop(EpgCfg.dmx_id);
				GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			}
			break;
		case GXMSG_EPG_FILTER_NUM_GET:
			{
				uint32_t* filter_num = NULL;
				filter_num = GxBus_GetMsgPropertyPtr(Msg,uint32_t);
				GxCore_MutexLock(s_EpgSubtCtrl.mutex);
				*filter_num = GxEpg_SubtChannelNumGet();
				GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
				break;
			}
		case GXMSG_EPG_FILTER_INFO_GET:
			{
				GxMsgProperty_EpgFilterInfo* filter_info = NULL;

				filter_info = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgFilterInfo);
				GxCore_MutexLock(s_EpgSubtCtrl.mutex);
				GxEpg_SubtChannelInfoGet(filter_info);
				GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
				break;
			}
		case GXMSG_EPG_FILTER_MODIFY:
			{
				GxMsgProperty_EpgFilterModify* filter_modify = NULL;

				filter_modify = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgFilterModify);
				GxCore_MutexLock(s_EpgSubtCtrl.mutex);
				GxEpg_SubtChannelModify(filter_modify, EpgCfg.dmx_id);
				GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
				break;
			}
		case GXMSG_EPG_DETAIL_FILTER_SET:
			{
				GxMsgProperty_EpgDetailFilterSwitch* p_detailfilter = NULL;
				p_detailfilter = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_EpgDetailFilterSwitch);
				epg_detail_filter_update(p_detailfilter);
				break;
			}
		case GXMSG_EPG_DETAIL_FILTER_CLEAN:
			{
				epg_detail_filter_clean();
				break;
			}
		default:
			break;
	}

	return GXMSG_OK;
}

void GxEpgServiceConsole(handle_t self)
{
	int32_t per_read_size = 28 * 1024;
	GxEpgSubtInfo *p_subt_info = NULL;
	GxEpgCache *cache = NULL;
	uint8_t *parse_data_buf = NULL;
	uint32_t read_size = 0;
	uint32_t channel_count = 0;
	int i = 0;
	int32_t ret = 0;

	GxCore_MutexLock(s_EpgSubtCtrl.mutex);
	channel_count = GxEpg_SubtChannelNumGet();

	GxBus_ConfigGetInt(EPG_CONFIG_PER_READ_SIZE, &per_read_size, 28 * 1024);
	per_read_size = per_read_size < (4 * 1024) ? (4 * 1024): per_read_size;
	p_subt_info = s_EpgSubtCtrl.subtInfo;
	for(i = 0; i < channel_count; i++)
	{
		parse_data_buf = GxCore_Mallocz(per_read_size);
		if(NULL == parse_data_buf)
		{
			GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			GxCore_ThreadDelay(10);
			return;
		}

		if(0 == p_subt_info[i].subtHandle)
		{
			GxCore_Free(parse_data_buf);
			parse_data_buf = NULL;
			GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			GxCore_ThreadDelay(10);
			return;
		}

		ret = GxDmxRead(p_subt_info[i].subtHandle, parse_data_buf, per_read_size, &read_size);
		if(ret < 0 || 0 == read_size)
		{
			if(-3 == ret)
				GxDmxReset(p_subt_info[i].subtHandle);
			GxCore_Free(parse_data_buf);
			parse_data_buf = NULL;
			GxCore_ThreadDelay(10);
			continue;
		}

		if(0x4e == p_subt_info[i].table_id || 0x4f == p_subt_info[i].table_id)
		{
			epg_parse_seciton(parse_data_buf, read_size);
			GxCore_Free(parse_data_buf);
			parse_data_buf = NULL;
			GxCore_ThreadDelay(10);
			continue;
		}

		cache = (GxEpgCache *)GxCore_Calloc(1, sizeof(GxEpgCache));
		if(NULL == cache)
		{
			EPG_ERR("!!!!!!No memory to store!!!!!\n");
			GxCore_Free(parse_data_buf);
			parse_data_buf = NULL;
			GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
			GxCore_ThreadDelay(10);
			return;
		}

		memset(cache, 0, sizeof(GxEpgCache));
		cache->buffer = parse_data_buf;
		cache->size = read_size;
		epg_parse_data_add_to_cache(cache);
		GxCore_SemPost(s_EpgSem);
		GxCore_ThreadDelay(10);
	}

	GxCore_MutexUnlock(s_EpgSubtCtrl.mutex);
	GxCore_ThreadDelay(10);
	return;
}


GxServiceClass epg_service = {
	"epg service",
	GxEpgServiceInit,
	GxEpgServiceDestroy,
	GxEpgServiceRecvMsg,
	GxEpgServiceConsole,
	.priority_offset = 0,
};

/* End of file -------------------------------------------------------------*/

