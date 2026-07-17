/************************************************************
 * Copyright (C), 2007-2009, GX S&T Co., Ltd.
 * FileName   : gxepg_manage.c
 * Author     : zhangling
 * Version    : 1.0
 * Date       :
 * Description:
 * Version    :
 * History    :
 * Date             Author      Modification
 * 2007.03.14     zhangling         create
 ***********************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "gxcore.h"
#include "module/epg/gxepg_manage.h"
#include "module/pm/gxpm_manage.h"
#include "module/config/gxconfig.h"

typedef enum
{
	GX_BUS_EPG_NAME_ADD = 0,
	GX_BUS_EPG_DETAILE_ADD
}GxBusEpgTypeAdd;

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


/*想要多少个service*/
static uint32_t GxBusEpgServiceCount = 0;
/*通过init 传下来的GxBusEpgServiceSize GxCore_Malloc空间*/
static GxBusEpgServiceHead* GxBusEpgService = NULL;

/*想要多少个event*/
static uint32_t GxBusEpgEventCount = 0;
/*通过init 传下来的GxBusEpgEventSize GxCore_Malloc空间*/
static GxBusEpgEventHead* GxBusEpgEvent = NULL;
/*每个event的大小 包括 event name和event describe*/
static uint32_t GxBusEpgEventSize = 0;
/*管理event头的栈*/
static uint32_t* GxBusEpgEventHeaderStack = NULL;
static int32_t GxBusEpgEventHeaderStackTemp = 0;///<栈的浮针

/*要保存的epg的天数 一般为7天*/
static uint32_t GxBusEpgDay = 0;

/*内存池的句柄 */
static handle_t GxBusEpgMemPool = GXCORE_INVALID_POINTER;
/*互斥量句柄*/
static handle_t GxBusEpgMutexHandle = GXCORE_INVALID_POINTER;
/*互斥量是否加锁标记 1--已经上锁 0--未上锁*/
static uint32_t GxBusEpgMutexLockFlag = 0;

/*已经添加到service头数组中的service*/
static GxBusEpgEventCurUseService* GxBusEpgCurUseService = NULL;

/*epg init的标记用于在init epg和release epg时候的保护*/
static uint32_t GxBusEpgInitFlag = 0;

#ifdef EPG_MALLOC
/*允许申请的最大字节数，用于控制epg耗用内存*/
static uint32_t GxBusEpgMaxSize = 0;
static uint32_t GxBusEpgCurSize = 0;
#endif

/**
 * @brief  默认的比较函数
 * @param void
 *
 */
static status_t  gx_epg_event_cmp(GxBusEpgEventComp* src,GxBusEpgEventComp* cur);
static int gx_epg_language_code_cmp(const char* language1,const char* language2);
GxBusEpgCheckEvent event_cmp = gx_epg_event_cmp;
GxBusEpgModifyEvent event_modify = NULL;

#define NO_INFORMATION "No Information"

/* Debug Defined ---------------------------------------------------------- */
#ifndef __DEBUG
#define GX_BUS_EPG_DBUG
#endif

#define EPG_MANAGE_PRINTF(...) gxlogd( __VA_ARGS__ )

#define GX_BUS_EPG_PRINTF(msg)\
	do{\
		EPG_MANAGE_PRINTF("\n\n*****epg manage error*****\n");\
		EPG_MANAGE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
		EPG_MANAGE_PRINTF("%s\n",msg);\
		EPG_MANAGE_PRINTF("~~~~~epg manage error~~~~~\n");\
	}while(0)
/* Private Functions ------------------------------------------------------ */


/**
 * @brief 申请service头的空间
 * @param void
 *
 * @return -1: 空间申请失败
 *         正常值:service头的下标
 */
static int32_t gx_epg_service_header_alloc(void);

/**
 * @brief 释放service头的空间
 * @param uint32_t temp :所要释放的service头的下标
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
static status_t gx_epg_service_header_free(uint32_t temp);

/**
 * @brief 申请event头的空间
 * @param void
 *
 * @return -1: 空间申请失败
 *        正常值: event头的下标
 */
static int32_t gx_epg_event_header_alloc(void);

/**
 * @brief 释放event头的空间
 * @param uint32_t temp :所要释放的event头的下标
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
static status_t gx_epg_event_header_GxCore_Free(uint32_t temp);

/**
 * @brief 判断event是否存在
 * @param
 *
 * @return TRUE:该event已经存在
 *         FALSE:该event没有存在
 */
static uint8_t gx_epg_event_exist_check(GxBusEpgEventAddInfo* info, int8_t* language,uint32_t service_pos);

/**
 * @brief 添加name和详细描述信息到event
 * @param
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_event_add(uint32_t event_pos,uint8_t* contentt,uint32_t size,GxBusEpgTypeAdd type);

/**
 * @brief 拷贝src_event到dst_event
 * @param GxBusEpgEventHead *dst_event:由调用GxBus_EpgEventGet申请
 *        GxBusEpgEventHead *src_event:来自GxBusEpgEvent数组里的某个事件
 *        int32_t dst_event_size:申请的dst_event内存大小
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_event_copy(GxBusEpgEventHead *dst_event, GxBusEpgEventHead *src_event, int32_t dst_event_size);

/**
 * @brief   释放一个event的name信息和detial信息
 * @param   uint32_t event_pos:event的位置
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *
 */
static status_t gx_epg_event_content_del(uint32_t event_pos);

/**
 * @brief 初始化申请到的buffer
 * @param
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
static status_t gx_epg_buffer_init(void);

/**
 * @brief 添加pf信息到service
 * @param
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_pf_add(uint32_t event_id,
		GxBusEpgEventTime* event_time,
		uint8_t* name,
		uint8_t*detaile_description,
		uint32_t name_size,
		uint32_t des_size,
		GxBusEpgEventType type,
		uint32_t service_pos);

/**
 * @brief 添加pf信息到service
 * @param
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 *          GX_BUS_EPG_PARAMETER_ERR:传入参数错误
 */
static status_t gx_epg_pf_copy(GxBusEpgEventPFHead *src_pf, GxBusEpgEventPFHead *dst_pf);

/**
 * @brief 释放pf信息
 * @param uint32_t service_pos:确定service
 * GxBusEpgEventPFHead* pf:所要释放的pf头部指针
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_pf_del(GxBusEpgEventPFHead* pf);

/**
 * @brief 删除指定的service的无效event
 * @param uint32_t service_pos:指定service
 *        time_t time_cur:比较的时间
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
static status_t gx_epg_invalide_event_del(uint32_t service_pos,time_t time);

static status_t gx_epg_cur_use_service_get(GxBusEpgEventCurUseService** service,uint32_t* num);
static status_t gx_epg_service_clean(uint32_t ts_id,uint32_t service_id);

/**
 * @brief  epg加锁 用于 get add 和GxBus_EpgInvalidInfoClean接口
 * @param  void
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgLock(void);

/**
 * @brief  epg 解锁锁用于 get add 和GxBus_EpgInvalidInfoClean接口
 * @param  void
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgUnlock(void);

#ifdef EPG_MALLOC
/**
 * @brief  epg malloc 初始化函数，设置最大malloc值
 * @param void
 *
 */
static status_t  gx_epg_malloc_init(uint32_t size);
/**
 * @brief  申请内存
 * @param void
 *
 */

#endif
static int8_t* gx_epg_malloc( uint32_t size);
/**
 * @brief  释放内存
 * @param void
 *
 */
static status_t gx_epg_free(void* p,uint32_t size);

/**
 * @brief  申请service头的空间
 * @param  void
 *
 * @return -1:空间申请失败
 *         正常值:service头的下标
 */
int32_t gx_epg_service_header_alloc(void)
{
	uint32_t i = 0;

	for(i=0;i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 0)
		{
			GxBusEpgService[i].use_flag = 1;
			return i;
		}
	}
	return -1;
}

/**
 * @brief   释放service头的空间
 * @param   uint32_t temp :所要释放的service头的下标
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
static status_t gx_epg_service_header_free(uint32_t temp)
{
	if(temp>=GxBusEpgServiceCount)
	{
		return GX_BUS_EPG_ERR;
	}
	GxBusEpgService[temp].use_flag = 0;

	return GX_BUS_EPG_OK;
}

/**
 * @brief 申请event头的空间
 * @param  void
 *
 * @return  -1: 空间申请失败
 *          正常值:event头的下标
 */
int32_t gx_epg_event_header_alloc(void)
{
	int32_t i = 0;

	if((uint32_t)GxBusEpgEventHeaderStackTemp<GxBusEpgEventCount)
	{
		i = GxBusEpgEventHeaderStack[GxBusEpgEventHeaderStackTemp];
		GxBusEpgEventHeaderStackTemp++;
		return i;
	}
	else
		return -1;
}

/**
 * @brief  释放event头的空间
 * @param  uint32_t temp :所要释放的event头的下标
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
status_t gx_epg_event_header_GxCore_Free(uint32_t temp)
{
	if(0<GxBusEpgEventHeaderStackTemp)
	{
		GxBusEpgEventHeaderStackTemp--;
		GxBusEpgEventHeaderStack[GxBusEpgEventHeaderStackTemp] = temp;
		return GX_BUS_EPG_OK;
	}

	return GX_BUS_EPG_ERR;
}

/**
 * @brief 判断event是否存在
 * @param
 *
 * @return TRUE:该event已经存在
 *         FALSE:该event没有存在
 */
extern uint8_t epg_cfg_get_filter_level(void);
uint8_t gx_epg_event_exist_check(GxBusEpgEventAddInfo* info, int8_t* language,uint32_t service_pos)
{
	uint32_t event_pos = 0;
	GxBusEpgFirstEvent* p = NULL;
	uint8_t *detail_info = NULL;

	p = GxBusEpgService[service_pos].first_event;
	while(p != NULL)
	{
		if(gx_epg_language_code_cmp((const char*)(p->language),(const char*)language) == 0)
		{
			event_pos = p->pos;
			break;
		}
		p = (GxBusEpgFirstEvent*)(p->next);
	}
	if(p == NULL || GX_BUS_EPG_INVALID_POS == event_pos)
	{
		return FALSE;
	}

	while(GxBusEpgEvent[event_pos].event_id != info->event_id
			|| GxBusEpgEvent[event_pos].service_id != info->service_id
			|| GxBusEpgEvent[event_pos].ts_id != info->ts_id)
	{
		event_pos = GxBusEpgEvent[event_pos].next_event;
		if(event_pos == GX_BUS_EPG_INVALID_POS)
		{
			return FALSE;
		}
	}
	if(epg_cfg_get_filter_level() == 0)
		return TRUE;
	if(info->detaile_description != NULL && info->name != NULL && info->event_time != NULL)
	{
		gx_epg_event_content_del(event_pos);
		gx_epg_event_add(event_pos,info->name,info->name_length,GX_BUS_EPG_NAME_ADD);
		detail_info = info->detaile_description;
		gx_epg_event_add(event_pos,detail_info,info->detaile_length,GX_BUS_EPG_DETAILE_ADD);
		GxBusEpgEvent[event_pos].time.start_time = info->event_time->start_time;
		GxBusEpgEvent[event_pos].time.duration = info->event_time->duration;
	}
	return TRUE;
}

#ifndef EPG_MALLOC
bool gx_epg_pf_event_cell_content_cmp(GxBusEpgMemPoolCell *cell, uint8_t *content, uint32_t length)
{
	uint8_t *temp_content = NULL;
	uint32_t i = 0, pool_count = 0, temp_length = 0, cmp_len = 0;

	GxBusEpgMemPoolCell *temp_cell = cell;
	while(temp_cell != NULL)
	{
		pool_count++;
		temp_cell = temp_cell->next_Cell;
	}

	temp_content = GxCore_Mallocz(pool_count * sizeof(cell->content));
	if(NULL == temp_content)
		return false;

	temp_cell = cell;
	for(i = 0; i < pool_count; i++)
	{
		memcpy(&temp_content[i], temp_cell->content, sizeof(temp_cell->content));
		temp_cell = temp_cell->next_Cell;
	}

	if(0 == strcmp((char *)content, (char *)temp_content))
	{
		GxCore_Free(temp_content);
		return true;
	}

	GxCore_Free(temp_content);
	return false;
}
#endif

/*
 * @brief 判断当前后续event是否存在
 * @param
 *
 * @return true:该event已经存在
 *         false:该event没有存在
 * */

uint8_t gx_epg_pf_event_exist_check(GxBusEpgEventPFHead* pf, GxBusEpgEventAddInfo* info)
{
#ifdef EPG_MALLOC
	if(NULL == pf->name.content || NULL == pf->detaile_description.content
			|| NULL == info->name || NULL == info->detaile_description)
		return false;

	uint32_t cmp_name_len = info->name_length > pf->name.size ? pf->name.size : info->name_length;
	uint32_t cmp_detaile_len = info->detaile_length > pf->detaile_description.size ? pf->detaile_description.size : info->detaile_length;

	if((0 == memcmp(pf->name.content, info->name, cmp_name_len))
			&& (0 == memcmp(pf->detaile_description.content, info->detaile_description, cmp_detaile_len))
			&& (pf->time.start_time == info->event_time->start_time && pf->time.duration == info->event_time->duration))
		return true;
#else
	if(NULL == pf->name
			|| NULL == pf->name->content
			|| NULL == pf->detaile_description
			|| NULL == pf->detaile_description->content)
		return false;

	if((true == gx_epg_pf_event_cell_content_cmp(pf->name, info->name, info->name_length))
			&& (true == gx_epg_pf_event_cell_content_cmp(pf->detaile_description, info->detaile_description, info->detaile_length))
			&& (pf->time.start_time == info->event_time->start_time && pf->time.duration == info->event_time->duration))
		return true;
#endif

	return false;
}

/**
 * @brief 添加name或者详细描述信息到event
 * @param
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
status_t gx_epg_event_add(uint32_t event_pos,uint8_t* content,uint32_t size,GxBusEpgTypeAdd type)
{
#ifdef EPG_MALLOC
	uint32_t length = 0;
	GxBusEpgMemPoolCell* cell = NULL;
#else
	uint32_t length = 0;
	uint32_t pool_count = 0; // 需要申请的内存池块数
	uint32_t i = 0;
	uint8_t* temp = NULL;
	uint32_t cp_size = 0;
	GxBusEpgMemPoolCell* cell = NULL;
	GxBusEpgMemPoolCell** cell_temp3 = NULL;
#endif
	switch(type)
	{
		case GX_BUS_EPG_NAME_ADD:
#ifdef EPG_MALLOC
			cell = &(GxBusEpgEvent[event_pos].name);
#else
			cell_temp3 = &(GxBusEpgEvent[event_pos].name);
#endif
			break;

		case GX_BUS_EPG_DETAILE_ADD:
#ifdef EPG_MALLOC
			cell = &(GxBusEpgEvent[event_pos].detaile_description);
#else
			cell_temp3 = &(GxBusEpgEvent[event_pos].detaile_description);
#endif
			break;
	}
#ifdef EPG_MALLOC
	length = size + 1;//算上一个休止符
	cell->content = gx_epg_malloc(length);
	if(cell->content == NULL)
	{
		gx_epg_event_content_del(event_pos);
		gx_epg_event_header_GxCore_Free(event_pos);
		return GX_BUS_EPG_MEM_POOL_FULL;
	}
	memcpy(cell->content,content,size);
	cell->size = length;
#else
	/*根据content的长度申请内存池空间*/
	length = size + 1;//算上一个休止符
	pool_count = length/(GX_BUS_MEM_POOL_CELL_SIZE-4);
	if(length%(GX_BUS_MEM_POOL_CELL_SIZE-4) !=0)
	{
		pool_count+=1;
	}
	temp = content;
	for(i=0; i<pool_count; i++)
	{
		cell = (GxBusEpgMemPoolCell*)GxCore_MemPoolAllocZero(GxBusEpgMemPool);
		if(cell == NULL)
		{
			gx_epg_event_content_del(event_pos);
			gx_epg_event_header_GxCore_Free(event_pos);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}

		/*申请到了内存池空间*/
		cp_size = length	>=GX_BUS_MEM_POOL_CELL_SIZE-4?GX_BUS_MEM_POOL_CELL_SIZE-4:length;

		length -= cp_size;
		memcpy(cell->content,temp,cp_size);

		temp+=cp_size;
		*(cell_temp3) = cell;
		cell_temp3 = (GxBusEpgMemPoolCell**)(&(cell->next_Cell));

	}
#endif
	return GX_BUS_EPG_OK;
}

/**
 * @brief 拷贝src_event到dst_event
 * @param GxBusEpgEventHead *dst_event:由调用GxBus_EpgEventGet申请
 *        GxBusEpgEventHead *src_event:来自GxBusEpgEvent数组里的某个事件
 *        int32_t dst_event_size:申请的dst_event内存大小
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_event_copy(GxBusEpgEventHead *dst_event, GxBusEpgEventHead *src_event, int32_t dst_event_size)
{
#define MEM_ALIGN_BYTES    4
	if(NULL == dst_event || NULL == src_event || 0 == dst_event_size)
		return GX_BUS_EPG_PARAMETER_ERR;

#ifdef EPG_MALLOC
	int32_t align_size = 0;
	int32_t name_len = 0, des_len = 0;

	name_len = src_event->name.size;
	des_len = src_event->detaile_description.size;

	if(name_len + des_len >= dst_event_size - sizeof(GxBusEpgEventHead) - 50)
		return GX_BUS_EPG_EVENT_TOO_SMALL;

	memset(dst_event, 0, dst_event_size);
	memcpy(dst_event, src_event, sizeof(GxBusEpgEventHead));
	dst_event->name.content = (int8_t *)dst_event + sizeof(GxBusEpgEventHead);
	memcpy(dst_event->name.content, src_event->name.content, name_len);
	dst_event->name.size = name_len;

	align_size = dst_event->name.size + MEM_ALIGN_BYTES - dst_event->name.size % MEM_ALIGN_BYTES;
	dst_event->detaile_description.content = (int8_t *)dst_event->name.content + align_size;
	memcpy(dst_event->detaile_description.content, src_event->detaile_description.content, des_len);
	dst_event->detaile_description.size = des_len;
#else
	int32_t i = 0;
	int32_t name_len = 0, des_len = 0;
	GxBusEpgMemPoolCell *cell = NULL;

	cell = src_event->name;
	while(cell != NULL)
	{
		name_len += sizeof(GxBusEpgMemPoolCell);
		cell = cell->next_Cell;
	}

	cell = src_event->detaile_description;
	while(cell != NULL)
	{
		des_len += sizeof(GxBusEpgMemPoolCell);
		cell = cell->next_Cell;
	}

	if(des_len + name_len >= dst_event_size - sizeof(GxBusEpgEventHead) - 50)
		return GX_BUS_EPG_EVENT_TOO_SMALL;

	memset(dst_event, 0, dst_event_size);
	*dst_event = *src_event;
	dst_event->name = (GxBusEpgMemPoolCell *)(dst_event + 1);
	cell = src_event->name;
	while(cell != NULL)
	{
		memcpy(dst_event->name[i].content, cell->content, GX_BUS_MEM_POOL_CELL_SIZE - 4);
		cell = cell->next_Cell;

		if(cell != NULL)
		{
			dst_event->name[i].next_Cell = &dst_event->name[i + 1];
			i++;
		}
	}

	dst_event->detaile_description = (GxBusEpgMemPoolCell *)((char *)dst_event->name + name_len);
	cell = src_event->detaile_description;
	while(cell != NULL)
	{
		memcpy(dst_event->detaile_description[i].content, cell->content, GX_BUS_MEM_POOL_CELL_SIZE - 4);
		cell = cell->next_Cell;

		if(cell != NULL)
		{
			dst_event->detaile_description[i].next_Cell = &dst_event->detaile_description[i + 1];
			i++;
		}
	}
#endif
	return GX_BUS_EPG_OK;
}

/**
 * @brief  释放一个event的name信息和detial信息
 * @param  uint32_t event_pos:event的位置
 *
 * @return GX_BUS_EPG_OK:执行正常
 *
 */
static status_t gx_epg_event_content_del(uint32_t event_pos)
{
#ifdef EPG_MALLOC
	GxBusEpgMemPoolCell* cell_temp1 = NULL;
#else
	GxBusEpgMemPoolCell* cell_temp1 = NULL;
	GxBusEpgMemPoolCell* cell_temp2 = NULL;
#endif

#ifdef EPG_MALLOC
	/*释放event的name content空间*/
	cell_temp1 = &GxBusEpgEvent[event_pos].name;
	if(cell_temp1->content)
		gx_epg_free(cell_temp1->content,cell_temp1->size);

	memset(cell_temp1, 0, sizeof(GxBusEpgMemPoolCell));

	/*释放event的detaile_description content空间*/
	cell_temp1 = &GxBusEpgEvent[event_pos].detaile_description;
	if(cell_temp1->content)
		gx_epg_free(cell_temp1->content,cell_temp1->size);

	memset(cell_temp1, 0, sizeof(GxBusEpgMemPoolCell));
#else
	cell_temp1 = GxBusEpgEvent[event_pos].name;
	while(cell_temp1 != NULL)
	{
		cell_temp2 = cell_temp1;
		cell_temp1 = (GxBusEpgMemPoolCell*)(cell_temp1->next_Cell);//偏移到下记录一块空间指针的地方
		GxCore_MemPoolFree(GxBusEpgMemPool,(void*)cell_temp2);
	}
	GxBusEpgEvent[event_pos].name = NULL;
	/*释放event的detaile_description空间*/
	cell_temp1 = GxBusEpgEvent[event_pos].detaile_description;
	while(cell_temp1 != NULL)
	{
		cell_temp2 = cell_temp1;
		cell_temp1 = (GxBusEpgMemPoolCell*)(cell_temp1->next_Cell);//偏移到下记录一块空间指针的地方
		GxCore_MemPoolFree(GxBusEpgMemPool,(void*)cell_temp2);
	}
	GxBusEpgEvent[event_pos].detaile_description = NULL;
#endif
	return GX_BUS_EPG_OK;
}

/**
 * @brief 初始化申请到的buffer
 * @param
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
status_t gx_epg_buffer_init(void)
{
	uint32_t i = 0;
	memset(GxBusEpgService,0,sizeof(GxBusEpgServiceHead)*GxBusEpgServiceCount);
	memset(GxBusEpgCurUseService,0,sizeof(GxBusEpgEventCurUseService)*GxBusEpgServiceCount);
	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		GxBusEpgService[i].first_event = NULL;
		GxBusEpgService[i].parental_rating = NULL;
		GxBusEpgService[i].parental_rating_num = 0;

		/*通过数据库恢复count,暂时没做*/
	}

	memset(GxBusEpgEvent,0,sizeof(GxBusEpgEventHead)*GxBusEpgEventCount);
	for(i=0; i<GxBusEpgEventCount; i++)
	{
		GxBusEpgEvent[i].next_event = GX_BUS_EPG_INVALID_POS;
	}

	/*初始化event头的栈*/
	for(i=0; i<GxBusEpgEventCount; i++)
	{
		GxBusEpgEventHeaderStack[i] = i;
	}
	GxBusEpgEventHeaderStackTemp = 0;

	return GX_BUS_EPG_OK;
}

/**
 * @brief 添加pf信息到service
 * @param
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_pf_add(uint32_t event_id,
		GxBusEpgEventTime* event_time,
		uint8_t* name,
		uint8_t*detaile_description,
		uint32_t name_size,
		uint32_t des_size,
		GxBusEpgEventType type,
		uint32_t service_pos)
{
#ifdef EPG_MALLOC
	uint32_t length = 0;
	GxBusEpgMemPoolCell* cell = NULL;
	GxBusEpgEventPFHead* pf = NULL;
#else
	uint32_t length = 0;
	uint32_t pool_count = 0;//需要申请的内存池块数
	uint32_t i = 0;
	uint8_t* temp = NULL;
	uint32_t cp_size = 0;
	GxBusEpgMemPoolCell* cell = NULL;
	GxBusEpgMemPoolCell** cell_temp3 = NULL;
	GxBusEpgEventPFHead* pf = NULL;
#endif
	switch(type)
	{
		case GX_EVENT_PRESENT:
			pf = &(GxBusEpgService[service_pos].present);
			break;

		case GX_EVENT_FOLLOW:
			pf = &(GxBusEpgService[service_pos].follow);
			break;

		default:
			break;
	}
	gx_epg_pf_del(pf);
#ifdef EPG_MALLOC
	/*添加新的当前后续信息的name*/
	cell = &(pf->name);
	length = name_size + 1;//算上一个休止符
	cell->content = gx_epg_malloc(length);
	if(cell->content == NULL)
	{
		gx_epg_pf_del(pf);
		return GX_BUS_EPG_MEM_POOL_FULL;
	}
	memcpy(cell->content,name,name_size);
	cell->size = length;

	/*添加新的当前后续信息的detaile_description*/
	if(detaile_description != NULL)
	{
		cell = &(pf->detaile_description);
		length = des_size + 1;//算上一个休止符
		cell->content = gx_epg_malloc(length);
		if(cell->content == NULL)
		{
			gx_epg_pf_del(pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}
		memcpy(cell->content,detaile_description,des_size);
		cell->size = length;
	}
#else
	/*添加新的当前后续信息的name*/
	cell_temp3 = &(pf->name);
	length = name_size + 1;//算上一个休止符
	pool_count = length/(GX_BUS_MEM_POOL_CELL_SIZE-4);
	if(length%(GX_BUS_MEM_POOL_CELL_SIZE-4) !=0)
	{
		pool_count+=1;
	}
	temp = name;
	for(i=0; i<pool_count; i++)
	{
		cell = (GxBusEpgMemPoolCell*)GxCore_MemPoolAllocZero(GxBusEpgMemPool);
		if(cell == NULL)
		{
			gx_epg_pf_del(pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}
		/*申请到了内存池空间*/
		cp_size = length >= GX_BUS_MEM_POOL_CELL_SIZE - 4 ? GX_BUS_MEM_POOL_CELL_SIZE - 4 : length;

		length -= cp_size;
		memcpy(cell->content,temp,cp_size);

		temp+=cp_size;
		*(cell_temp3) = cell;
		cell_temp3 = (GxBusEpgMemPoolCell**)(&(cell->next_Cell));

	}

	/*添加新的当前后续信息的detaile_description*/
	if(detaile_description != NULL)
	{
		cell_temp3 = &(pf->detaile_description);
		length = des_size + 1;//算上一个休止符
		pool_count = length/(GX_BUS_MEM_POOL_CELL_SIZE-4);
		if(length%(GX_BUS_MEM_POOL_CELL_SIZE-4) !=0)
		{
			pool_count+=1;
		}
		temp = detaile_description;
		for(i=0; i<pool_count; i++)
		{
			cell = (GxBusEpgMemPoolCell*)GxCore_MemPoolAllocZero(GxBusEpgMemPool);
			if(cell == NULL)
			{
				gx_epg_pf_del(pf);
				return GX_BUS_EPG_MEM_POOL_FULL;
			}
            /*申请到了内存池空间*/
			cp_size = length	>=GX_BUS_MEM_POOL_CELL_SIZE-4?GX_BUS_MEM_POOL_CELL_SIZE-4:length;

			length -= cp_size;
			memcpy(cell->content,temp,cp_size);

			temp+=cp_size;
			*(cell_temp3) = cell;
			cell_temp3 = (GxBusEpgMemPoolCell**)(&(cell->next_Cell));

		}
	}
#endif
	/*更新时间信息*/
	pf->time.start_time = event_time->start_time;
	pf->time.duration   = event_time->duration;
	pf->event_id        = event_id;
	return GX_BUS_EPG_OK;
}

static status_t  gx_epg_pf_free(GxBusEpgEventPFHead *pf)
{
	GxBusEpgMemPoolCell* temp1 = NULL;
#ifdef EPG_MALLOC
	temp1 = &pf->name;
	if(temp1->content)
		GxCore_Free(temp1->content);

	memset(temp1, 0, sizeof(GxBusEpgMemPoolCell));

	temp1 = &pf->detaile_description;
	if(temp1->content)
		GxCore_Free(temp1->content);
	memset(temp1, 0, sizeof(GxBusEpgMemPoolCell));
#else
	GxBusEpgMemPoolCell* temp2 = NULL;
	temp1 = pf->name;
	while(temp1 != NULL)
	{
		temp2 = temp1;
		temp1 = (GxBusEpgMemPoolCell*)(temp1->next_Cell);
		GxCore_Free(temp2);
	}
	pf->name = NULL;
	temp1 = pf->detaile_description;
	while(temp1 != NULL)
	{
		temp2 = temp1;
		temp1 = (GxBusEpgMemPoolCell*)(temp1->next_Cell);
		GxCore_Free(temp2);
	}
	pf->detaile_description = NULL;
#endif
     return GX_BUS_EPG_OK;
}

/**
 * @brief 拷贝src_pf信息到dst_pf, dst_pf内存需要用户释放
 * @param
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 */
static status_t gx_epg_pf_copy(GxBusEpgEventPFHead *dst_pf, GxBusEpgEventPFHead *src_pf)
{
	uint32_t length = 0;
	GxBusEpgMemPoolCell* cell = NULL;
#ifndef EPG_MALLOC
	GxBusEpgMemPoolCell** cell_temp3 = NULL;
#endif

	if(NULL == src_pf || NULL == dst_pf)
		return GX_BUS_EPG_PARAMETER_ERR;

	dst_pf->time = src_pf->time;
    dst_pf->event_id = src_pf->event_id;
#ifdef EPG_MALLOC
	/*拷贝当前后续信息的name*/
	if(src_pf->name.content)
	{
		cell = &(dst_pf->name);
		length = src_pf->name.size;

		cell->content = GxCore_Mallocz(length);
		if(NULL == cell->content)
		{
			gx_epg_pf_free(dst_pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}

		memcpy(cell->content, src_pf->name.content, src_pf->name.size);
		cell->size = length;
	}

	/*拷贝当前后续信息的detaile_description*/
	if(src_pf->detaile_description.content)
	{
		cell = &(dst_pf->detaile_description);
		length = src_pf->detaile_description.size;

		cell->content = GxCore_Mallocz(length);
		if(NULL == cell->content)
		{
			gx_epg_pf_free(dst_pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}

		memcpy(cell->content, src_pf->detaile_description.content, src_pf->detaile_description.size);
		cell->size = length;
    }
#else
	/*拷贝当前后续信息的name*/
	cell_temp3 = &(dst_pf->name);
	cell = src_pf->name;
	while(cell)
	{
		*cell_temp3 = (GxBusEpgMemPoolCell*)GxCore_Mallocz(sizeof(GxBusEpgMemPoolCell));
		if(NULL == *cell_temp3)
		{
			gx_epg_pf_free(dst_pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}

		/*申请到了内存池空间*/
		memcpy((*cell_temp3)->content, cell->content, GX_BUS_MEM_POOL_CELL_SIZE-4);
		cell_temp3 = (GxBusEpgMemPoolCell **)(&(*cell_temp3)->next_Cell);
		cell = cell->next_Cell;
	}

	/*添加新的当前后续信息的detaile_description*/
	cell_temp3 = &(dst_pf->detaile_description);
	cell = src_pf->detaile_description;
	while(cell)
	{
		*cell_temp3 = (GxBusEpgMemPoolCell*)GxCore_Mallocz(sizeof(GxBusEpgMemPoolCell));
		if(NULL == *cell_temp3)
		{
			gx_epg_pf_free(dst_pf);
			return GX_BUS_EPG_MEM_POOL_FULL;
		}

		/*申请到了内存池空间*/
		memcpy((*cell_temp3)->content, cell->content, GX_BUS_MEM_POOL_CELL_SIZE-4);
		cell_temp3 = (GxBusEpgMemPoolCell **)(&(*cell_temp3)->next_Cell);
		cell = cell->next_Cell;
	}
#endif

	return GX_BUS_EPG_OK;
}

/**
 * @brief 释放pf信息
 * @param GxBusEpgEventPFHead* pf:所要释放的pf头部指针
 *
 * @return GX_BUS_EPG_OK:执行正常
 */
static status_t gx_epg_pf_del(GxBusEpgEventPFHead* pf)
{
#ifdef EPG_MALLOC
	GxBusEpgMemPoolCell* temp1 = NULL;
#else
	GxBusEpgMemPoolCell* temp1 = NULL;
	GxBusEpgMemPoolCell* temp2 = NULL;
#endif

#ifdef EPG_MALLOC
	/*释放name content*/
	temp1 = &pf->name;
	if(temp1->content)
		gx_epg_free(temp1->content,temp1->size);

	memset(temp1, 0, sizeof(GxBusEpgMemPoolCell));

	/*释放detaile_description content*/
	temp1 = &pf->detaile_description;
	if(temp1->content)
		gx_epg_free(temp1->content,temp1->size);

	memset(temp1, 0, sizeof(GxBusEpgMemPoolCell));
#else
	/*释放name*/
	temp1 = pf->name;
	while(temp1 != NULL)
	{
		temp2 = temp1;
		temp1 = (GxBusEpgMemPoolCell*)(temp1->next_Cell);//偏移到下记录一块空间指针的地方
		GxCore_MemPoolFree(GxBusEpgMemPool,(void*)temp2);
	}
	pf->name = NULL;
	/*释放detaile_description*/
	temp1 = pf->detaile_description;
	while(temp1 != NULL)
	{
		temp2 = temp1;
		temp1 = (GxBusEpgMemPoolCell*)(temp1->next_Cell);//偏移到下记录一块空间指针的地方
		GxCore_MemPoolFree(GxBusEpgMemPool,(void*)temp2);
	}
	pf->detaile_description = NULL;
#endif
	return GX_BUS_EPG_OK;
}

/**
 * @brief 删除指定的service的无效event
 * @param uint32_t service_pos:指定service
 *        time_t time_cur:比较的时间
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
status_t gx_epg_invalide_event_del(uint32_t service_pos,time_t time)
{
	uint32_t event_pos = 0;
	uint32_t event_next_pos = 0;
	GxBusEpgFirstEvent* p = NULL;

	p = GxBusEpgService[service_pos].first_event;
first_event:
	if(p == NULL)
	{
		return GX_BUS_EPG_OK;
	}
	event_pos = p->pos;
	while(event_pos !=  GX_BUS_EPG_INVALID_POS)
	{
		event_next_pos = GxBusEpgEvent[event_pos].next_event;
		if(GxBusEpgEvent[event_pos].time.start_time + GxBusEpgEvent[event_pos].time.duration <= time)
		{
			gx_epg_event_content_del(event_pos);
			gx_epg_event_header_GxCore_Free(event_pos);
			/*重建event头链*/
			if(event_pos == p->pos)
			{
				p->pos = event_next_pos;
				if(p->pos == GX_BUS_EPG_INVALID_POS)
				{
					//所有event都被删除了
					break;
				}
			}
			else
			{
				//这种情况应该是不可能出现的,因为event在插入的时候就是按时间顺序排好的
				GX_BUS_EPG_PRINTF("ERR---epg del involid evnet er!!!\n");
				return GX_BUS_EPG_ERR;
			}
			event_pos = event_next_pos;
		}
		else
			break;
	}
	p = (GxBusEpgFirstEvent*)(p->next);
	goto first_event;

	return GX_BUS_EPG_OK;
}

static status_t gx_epg_cur_use_service_get(GxBusEpgEventCurUseService** service,uint32_t* num)
{
	uint32_t i = 0;
	uint32_t j = 0;

	if(GxBusEpgMutexLockFlag != 1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 1)
		{
			GxBusEpgCurUseService[j].ts_id = GxBusEpgService[i].ts_id;
			GxBusEpgCurUseService[j].service_id = GxBusEpgService[i].service_id;
			GxBusEpgCurUseService[j].original_id= GxBusEpgService[i].original_id;
			GxBusEpgCurUseService[j].pos = i;
			j++;
		}
	}
	*service = GxBusEpgCurUseService;
	*num = j;

	return GX_BUS_EPG_OK;
}

/**
 * @brief  拷贝src_service里的内容到dst_service,
 * @param  GxBusEpgServiceHead *src_service, 来自GxBusEpgService数组
 *         GxBusEpgServiceHead *dst_service, 调用函数的局部变量或申请的内存,
 *		　　使用完后调用GxBus_EpgServiceContentRelease释放其中申请的内存
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 */
static status_t gx_epg_service_copy(GxBusEpgServiceHead *dst_service, GxBusEpgServiceHead *src_service)
{
	GxBusEpgFirstEvent **p = NULL;
	GxBusEpgFirstEvent *d = NULL;
	status_t ret = GX_BUS_EPG_OK;

	if(NULL == src_service || NULL == dst_service)
		return GX_BUS_EPG_PARAMETER_ERR;

	memcpy(dst_service, src_service, sizeof(GxBusEpgServiceHead));
	memset(&dst_service->present, 0, sizeof(GxBusEpgEventPFHead));
	memset(&dst_service->follow, 0, sizeof(GxBusEpgEventPFHead));
	dst_service->parental_rating = NULL;
	dst_service->parental_rating_num = 0;
	dst_service->channel_ett = NULL;
	dst_service->channel_ett_num = 0;

	/*copy first event*/
	p = &dst_service->first_event;
	d = src_service->first_event;
	while(d != NULL)
	{
		*p = (GxBusEpgFirstEvent *)GxCore_Mallocz(sizeof(GxBusEpgFirstEvent));
		if(NULL == *p)
		{
			ret = GX_BUS_EPG_MEM_POOL_FULL;
			goto err;
		}

		memcpy(*p, d, sizeof(GxBusEpgFirstEvent));
		p = (GxBusEpgFirstEvent **)(&(*p)->next);
		d = d->next;
	}

	/*copy parental rating*/
	if(src_service->parental_rating != NULL && src_service->parental_rating_num != 0)
	{
		dst_service->parental_rating = (GxBusEpgParentalRating *)GxCore_Mallocz(sizeof(GxBusEpgParentalRating) * src_service->parental_rating_num);
		if(NULL == dst_service->parental_rating)
		{
			ret = GX_BUS_MEM_POOL_CELL_SIZE;
			goto err;
		}

		dst_service->parental_rating_num = src_service->parental_rating_num;
		memcpy(dst_service->parental_rating, src_service->parental_rating, sizeof(GxBusEpgParentalRating) * src_service->parental_rating_num);
	}

	/*copy present and follow events*/
	if((ret = gx_epg_pf_copy(&dst_service->present, &src_service->present)) != GX_BUS_EPG_OK)
		goto err;
	if((ret = gx_epg_pf_copy(&dst_service->follow, &src_service->follow)) != GX_BUS_EPG_OK)
		goto err;

	/*channel ett doesn't add in GxBus_EpgInfoAdd, so skip*/

	return GX_BUS_EPG_OK;
err:
	GxBus_EpgServiceContentRelease(dst_service);
	return ret;
}

static status_t gx_epg_service_clean(uint32_t ts_id,uint32_t service_id)
{
	GxBusEpgEventCurUseService* use_service = NULL;
	uint32_t num = 0;
	uint32_t i = 0;
	int32_t service_pos = -1;
	time_t time_cur = 0;
	GxBusEpgEventPFHead* pf = NULL;
	status_t ret = 0;
	GxBusEpgFirstEvent* p = NULL;
	GxBusEpgFirstEvent* d = NULL;


	gx_epg_cur_use_service_get(&use_service,&num);

	/*搜寻该event是否属于已有的service,并且获取在servcie头数组中的位置*/
	for(i=0; i<num; i++)
	{
		if(use_service[i].ts_id == ts_id&&
				use_service[i].service_id == service_id)
		{
			service_pos = use_service[i].pos;
			break;
		}
	}
	if(service_pos == -1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg have not existed!\n");
#endif
		return GX_BUS_EPG_PARAMETER_ERR;
	}
	time_cur = 0x7fffffff;
	ret = gx_epg_invalide_event_del(service_pos,time_cur);
	if(ret != GX_BUS_EPG_OK)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg del event err!\n");
#endif
		return GX_BUS_EPG_ERR;
	}

	/*释放当前空间*/
	pf = &(GxBusEpgService[service_pos].present);

	gx_epg_pf_del(pf);

	/*释放后续空间*/
	pf = &(GxBusEpgService[service_pos].follow);
	gx_epg_pf_del(pf);

	/*释放first event*/
	p = GxBusEpgService[service_pos].first_event;
	while(p != NULL)
	{
		d = (GxBusEpgFirstEvent*)(p->next);
		gx_epg_free(p,sizeof(GxBusEpgFirstEvent));
		p = d;
	}
	GxBusEpgService[service_pos].first_event = NULL;
	/*释放parental rating空间*/
	if(GxBusEpgService[service_pos].parental_rating != NULL)
	{
		gx_epg_free(GxBusEpgService[service_pos].parental_rating, sizeof(GxBusEpgParentalRating)*GxBusEpgService[service_pos].parental_rating_num);
		GxBusEpgService[service_pos].parental_rating = NULL;
	}
	GxBusEpgService[service_pos].parental_rating_num = 0;
	gx_epg_service_header_free(service_pos);

	return GX_BUS_EPG_OK;
}

static int _gx_epg_is_matched(char *src, char *dst)
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

static int _gx_epg_language_match(unsigned char* lang1, unsigned char* lang2, unsigned int lang_len)
{
#define TEMP_LEN  10 // i think it is enough forever
	int i = 0;
	int count = ISO639_MAP_COUNT;
	char str[TEMP_LEN] = {0};
	if(lang_len >= TEMP_LEN)
	{
		gxlogi("\nERROR, %s, %d\n", __func__, __LINE__);
		return -1;
	}
	memcpy(str, lang1, lang_len);
	for(i = 0; i < count; i++)
	{
		if(thiz_iso639_langmap[i])
		{
			if(_gx_epg_is_matched(thiz_iso639_langmap[i], str))
			{// find lang1
				memset(str, 0, TEMP_LEN);
				memcpy(str, lang2, lang_len);
				if(_gx_epg_is_matched(thiz_iso639_langmap[i], str))
				{// fine lang2
					return 1;
				}
				return 0;
			}
		}
	}
	return 0;
}

static int gx_epg_language_code_cmp(const char* language1,const char* language2)
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
		if(_gx_epg_language_match(language_1, language_2, LANGUAGE_LEN))
			return 0;
		else
			return -1;
	}
}
/**
 * @brief  默认的比较函数
 * @param void
 *
 */
static status_t  gx_epg_event_cmp(GxBusEpgEventComp* src,GxBusEpgEventComp* cur)
{

	if(src->service_id == cur->service_id)
	{
		return GX_BUS_EPG_OK;
	}
	return GX_BUS_EPG_ERR;
}
#ifdef EPG_MALLOC
/**
 * @brief  epg malloc 初始化函数，设置最大malloc值
 * @param void
 *
 */
static status_t  gx_epg_malloc_init(uint32_t size)
{
	GxBusEpgMaxSize = size;
	return GX_BUS_EPG_OK;
}
/**
 * @brief  申请内存
 * @param void
 *
 */
#endif
static int8_t* gx_epg_malloc( uint32_t size)
{
#ifdef EPG_MALLOC
	if(GxBusEpgCurSize + size + 64 > GxBusEpgMaxSize)//+64是因为malloc的管理头部至少64字节
	{
		return NULL;
	}

	GxBusEpgCurSize += size + 64;
#endif
	return (int8_t*)GxCore_Mallocz(size);
}
/**
 * @brief  释放内存
 * @param void
 *
 */
static status_t gx_epg_free(void* p, uint32_t size)
{
#ifdef EPG_MALLOC

	GxBusEpgCurSize -= (size + 64);//-64和上面+64对应
#endif
	GxCore_Free(p);

	return GX_BUS_EPG_OK;
}
/* Exported Functions ----------------------------------------------------- */

/**
 * @brief   初始化epg,使用epg前面必须得初始化
 * @param   service_count:需要保存的service头的个数
 *          event_count_per_day_service:需要保存的总的event数量
 *          uint32_t event_size:每个event的大小 包括 event name和event describe
 *          uint32_t epg_day:需要保存地epg的天数 一般是7天
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 *          GX_BUS_EPG_MEM_ERR:epg申请内存失败
 */
status_t GxBus_EpgInit(uint32_t service_count,
		uint32_t event_count,
		uint32_t event_size,
		uint32_t epg_day)
{
	if(GXCORE_INVALID_POINTER == GxBusEpgMutexHandle)
	{
		GxCore_MutexCreate(&GxBusEpgMutexHandle);
	}

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag != 1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(GxBusEpgInitFlag == 0)
	{
		GxBusEpgInitFlag = 1;
	}
	else
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg manege have initialized!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}
	if(service_count == 0
			||event_count == 0
			||event_size == 0)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---params err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_MEM_ERR;
	}
#ifdef EPG_MALLOC
	gx_epg_malloc_init(GX_BUS_MEM_POOL_CELL_SIZE*event_count*event_size/GX_BUS_MEM_POOL_CELL_SIZE+
			sizeof(uint32_t)*event_count+
			sizeof(GxBusEpgServiceHead)*service_count+
			sizeof(GxBusEpgEventHead)*event_count+
			sizeof(GxBusEpgEventCurUseService)*service_count
			);
#else
	/*申请详细描述和name的内存池,每块52Byte*/
	GxBusEpgMemPool = GxCore_MemPoolCreate(GX_BUS_MEM_POOL_CELL_SIZE,
			event_count*event_size/GX_BUS_MEM_POOL_CELL_SIZE,10);
	//	gxlogd("GxBusEpgMemPool= %p\n", (void*)GxBusEpgMemPool);
	if(GxBusEpgMemPool == GXCORE_INVALID_POINTER)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("create epg memery pool failure!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_MEM_ERR;
	}
#endif
	GxBusEpgServiceCount     = service_count;
	GxBusEpgEventCount       = event_count;
	GxBusEpgEventHeaderStack = (uint32_t*)gx_epg_malloc(sizeof(uint32_t)*GxBusEpgEventCount);
	GxBusEpgService         = (GxBusEpgServiceHead*)gx_epg_malloc(sizeof(GxBusEpgServiceHead)*GxBusEpgServiceCount);
	GxBusEpgEvent            = (GxBusEpgEventHead*)gx_epg_malloc(sizeof(GxBusEpgEventHead)*GxBusEpgEventCount);
	GxBusEpgCurUseService    = (GxBusEpgEventCurUseService*)gx_epg_malloc(sizeof(GxBusEpgEventCurUseService)*GxBusEpgServiceCount);
	GxBusEpgEventSize        = event_size;
	if((GxBusEpgService == NULL)
			||(GxBusEpgEvent == NULL)
			||(GxBusEpgEventHeaderStack == NULL)
			||(GxBusEpgCurUseService == NULL))
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---create epg memery failure!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_MEM_ERR;
	}

	gx_epg_buffer_init();
	GxBus_EpgUnlock();

	return GX_BUS_EPG_OK;
}

/**
 * @brief  释放epg所占资源,包括硬件资源,之后需要重新inti才能正常工作
 * @param  void
 *
 * @return GX_BUS_EPG_OK:执行正常
 *         GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgRelease(void)
{
	uint32_t i = 0;

	GxBus_EpgLock();
	if(GxBusEpgInitFlag == 1)
	{
		GxBusEpgInitFlag = 0;
	}
	else
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg manage has been realeased!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}

	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 1)
		{
			gx_epg_service_clean(GxBusEpgService[i].ts_id,GxBusEpgService[i].service_id);
		}
	}
	if(GxBusEpgService != NULL)
	{
		GxCore_Free(GxBusEpgService);
	}
	GxBusEpgService = NULL;
	GxBusEpgServiceCount = 0;

	if(GxBusEpgEvent != NULL)
	{
		GxCore_Free(GxBusEpgEvent);
	}
	GxBusEpgEvent = NULL;
	GxBusEpgEventCount = 0;
	GxBusEpgEventSize = 0;
	GxBusEpgDay = 0;

	if(GxBusEpgEventHeaderStack != NULL)
	{
		GxCore_Free(GxBusEpgEventHeaderStack);
	}
	GxBusEpgEventHeaderStack = NULL;
	GxBusEpgEventHeaderStackTemp = 0;

	if(GxBusEpgMemPool !=GXCORE_INVALID_POINTER)
	{
		//		gxlogd("GxBusEpgMemPool= %p\n", (void*)GxBusEpgMemPool);
		GxCore_MemPoolDestory(GxBusEpgMemPool);
		GxBusEpgMemPool = GXCORE_INVALID_POINTER;

	}

	if(GxBusEpgCurUseService !=NULL)
	{
		GxCore_Free(GxBusEpgCurUseService);
	}
	GxBusEpgCurUseService = NULL;
#ifdef EPG_MALLOC
	GxBusEpgCurSize = 0;
#endif
	GxBus_EpgUnlock();

	return GX_BUS_EPG_OK;
}

/**
 * @brief   释放epg存储空间,使所有存储空间可用,开始全速解析
 * @param   void
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgClean(void)
{
	uint32_t i = 0;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 1)
		{
			gx_epg_service_clean(GxBusEpgService[i].ts_id,GxBusEpgService[i].service_id);
		}
	}

	gx_epg_buffer_init();

	GxBus_EpgUnlock();

	return GX_BUS_EPG_OK;
}

/**
 * @brief   epg加锁 用于 get add 和GxBus_EpgInvalidInfoClean接口
 * @param   void
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgLock(void)
{
	int32_t ret = 0;
	if(GxBusEpgMutexHandle != GXCORE_INVALID_POINTER)
	{
		ret = GxCore_MutexLock(GxBusEpgMutexHandle);
		if(ret < 0)
		{
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("lock epg failure!\n");
#endif
			GxBusEpgMutexLockFlag = 0;
			return GX_BUS_EPG_ERR;
		}
		GxBusEpgMutexLockFlag = 1;
	}
	else
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("!!!!handle_t has been deleted!!!!!\n");
#endif
		return GX_BUS_EPG_ERR;
	}

	return GX_BUS_EPG_OK;
}

/**
 * @brief   epg解锁锁 用于 get add 和GxBus_EpgInvalidInfoClean接口
 * @param   void
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 */
status_t GxBus_EpgUnlock(void)
{
	int32_t ret = 0;
	if(GxBusEpgMutexHandle != GXCORE_INVALID_POINTER)
	{
		//这个变量需要在MutexUnlock之前,否则不受保护
		GxBusEpgMutexLockFlag = 0;
		ret = GxCore_MutexUnlock(GxBusEpgMutexHandle);
		if(ret < 0)
		{
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("unlock epg failure!\n");
#endif
			GxBusEpgMutexLockFlag = 1;
			return GX_BUS_EPG_ERR;
		}
	}
	else
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("!!!!!handle_t has been deleted!!!!!\n");
#endif
		return GX_BUS_EPG_ERR;
	}
	return GX_BUS_EPG_OK;
}

/**
 * @brief  获得某个service,
 * @param  uint32_t ts_id,
 *         uint32_t service_id
 *         uint32_t original_id
 *         GxBusEpgServiceHead* service_head:保存头信息的空间
 *		　　使用完后调用GxBus_EpgServiceContentRelease释放其中申请的内存
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 */
status_t GxBus_EpgServiceGet(uint32_t ts_id,
		uint32_t service_id,
		uint32_t original_id,
		GxBusEpgServiceHead* service_head)
{
	uint32_t i = 0;
	GxBusEpgEventComp src;
	GxBusEpgEventComp cur;
	status_t ret = GX_BUS_EPG_OK;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(service_head == NULL)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 1){
			src.ts_id = GxBusEpgService[i].ts_id;
			src.service_id = GxBusEpgService[i].service_id;
			src.original_id = GxBusEpgService[i].original_id;
			cur.ts_id = ts_id;
			cur.service_id = service_id;
			cur.original_id = original_id;
			if(event_cmp(&src,&cur) == GX_BUS_EPG_OK)
			{
				ret = gx_epg_service_copy(service_head, &GxBusEpgService[i]);
				GxBus_EpgUnlock();
				return ret;
			}
		}
	}

	GxBus_EpgUnlock();
	return GX_BUS_EPG_PARAMETER_ERR;
}


/**
 * @brief  释放serviceHead里申请的内存,
 * @param  GxBusEpgServiceHead *service,
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 */
status_t GxBus_EpgServiceContentRelease(GxBusEpgServiceHead *service)
{
	GxBusEpgFirstEvent *p = NULL;
	GxBusEpgFirstEvent *d = NULL;

	if(NULL == service)
		return GX_BUS_EPG_PARAMETER_ERR;

	/*release first event list*/
	p = service->first_event;
	while(p != NULL)
	{
		d = (GxBusEpgFirstEvent *)(p->next);
		GxCore_Free(p);
		p = d;
	}

	/*release parental rating*/
	if(service->parental_rating)
		GxCore_Free(service->parental_rating);

	/*release present follow event*/
	gx_epg_pf_free(&service->present);
	gx_epg_pf_free(&service->follow);
	return GX_BUS_EPG_OK;
}

/**
 * @brief   获得某个event的基本信息,不包含name和detail
 * @param   uint32_t event_pos:event的pos
 * GxBusEpgEventHead* event_head:保存头信息
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 */
status_t GxBus_EpgEventBasicInfoGet(uint32_t event_pos, GxBusEpgEventHead* event_head)
{
	status_t ret = GX_BUS_EPG_OK;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag != 1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	if((NULL == event_head) || (event_pos>=GxBusEpgEventCount))
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	memset(event_head, 0, sizeof(GxBusEpgEventHead));
	event_head->event_id = GxBusEpgEvent[event_pos].event_id;
	event_head->reference_event_id = GxBusEpgEvent[event_pos].reference_event_id;
	event_head->service_id = GxBusEpgEvent[event_pos].service_id;
	event_head->ts_id = GxBusEpgEvent[event_pos].ts_id;
	event_head->time = GxBusEpgEvent[event_pos].time;
	event_head->next_event = GxBusEpgEvent[event_pos].next_event;

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}

/**
 * @brief   获得某个event,
 * @param   uint32_t event_pos:event的pos
 * GxBusEpgEventHead* event_head:保存头信息的空间,由调用这申请内存
 * int32_t event_per_size:申请的event_head的内存大小
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *           GX_BUS_EPG_EVENT_TOO_SMALL:申请的event_head空间太小
 */
status_t GxBus_EpgEventGet(uint32_t event_pos, GxBusEpgEventHead* event_head, int32_t event_per_size)
{
    status_t ret = GX_BUS_EPG_OK;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	if((event_head == NULL)
            || (0 == event_per_size)
			|| (event_pos>=GxBusEpgEventCount)
#ifdef EPG_MALLOC
			|| (NULL == GxBusEpgEvent[event_pos].name.content)
#else
			|| (NULL == GxBusEpgEvent[event_pos].name)
#endif
			)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	ret = gx_epg_event_copy(event_head, &GxBusEpgEvent[event_pos], event_per_size);
	GxBus_EpgUnlock();
	return ret;
}

/**
 * @brief   获取当前已经添加到service头数组中的service
 * @param   GxBusEpgEventCurUseService** service:返回的指针
 * uint32_t* num:数量,如果为0代表service头数组是空的
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *          GX_BUS_EPG_PARAMETER_ERR:传入的参数错误
 */
status_t GxBus_EpgCurUseServiceGet(GxBusEpgEventCurUseService** service,uint32_t* num)
{
	uint32_t i = 0;
	uint32_t j = 0;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(service == NULL || num == NULL)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}
	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		if(GxBusEpgService[i].use_flag == 1)
		{
			GxBusEpgCurUseService[j].ts_id = GxBusEpgService[i].ts_id;
			GxBusEpgCurUseService[j].service_id = GxBusEpgService[i].service_id;
			GxBusEpgCurUseService[j].original_id = GxBusEpgService[i].original_id;
			GxBusEpgCurUseService[j].pos = i;
			j++;
		}
	}
	*service = GxBusEpgCurUseService;
	*num = j;

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}

/**
 * @brief   添加节目父母锁信息,
 * @param   GxBusEpgEventAddInfo* info:包含添加的父母锁信息

 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_SERVICE_FULL:service头数组满了
 *           GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数错误
 *           GX_BUS_EPG_EVENT_EXIST:传入的event已经存在
 */
status_t GxBus_EpgServiceParentalInfoAdd(GxBusEpgEventAddInfo *info)
{
	uint32_t i = 0;
	uint32_t num = 0;
	int32_t service_pos = -1;
	GxBusEpgEventCurUseService* use_service = NULL;

    if(NULL == info
            || (info->parental_rating != NULL && 0 == info->parental_rating_num)
            || (NULL == info->parental_rating && info->parental_rating_num != 0))
    {
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		return GX_BUS_EPG_PARAMETER_ERR;
    }

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	gx_epg_cur_use_service_get(&use_service, &num);

	/*搜寻该event是否属于已有的service,并且获取在servcie头数组中的位置*/
	for(i = 0; i < num; i++)
	{
        if(use_service[i].original_id == info->original_id
                && use_service[i].ts_id == info->ts_id
                && use_service[i].service_id == info->service_id)
		{
			service_pos = use_service[i].pos;
			break;
		}
	}

	if(service_pos == -1)
	{
		service_pos = gx_epg_service_header_alloc();
		if(service_pos == -1)
		{
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("ERR---epg service full!\n");
#endif
			GxBus_EpgUnlock();
			return GX_BUS_EPG_SERVICE_FULL;
		}
		memset((&(GxBusEpgService[service_pos])),0,sizeof(GxBusEpgServiceHead));
		GxBusEpgService[service_pos].use_flag = 1;
		GxBusEpgService[service_pos].ts_id = info->ts_id;
		GxBusEpgService[service_pos].service_id = info->service_id;
		GxBusEpgService[service_pos].reference_service_id = info->reference_service_id;
		GxBusEpgService[service_pos].original_id = info->original_id;
		GxBusEpgService[service_pos].first_event = NULL;
		GxBusEpgService[service_pos].parental_rating = NULL;
		GxBusEpgService[service_pos].parental_rating_num = 0;
		GxBusEpgService[service_pos].detaile_description_ver = info->ver;
	}

    if(GxBusEpgService[service_pos].parental_rating != NULL)
    {
        gx_epg_free(GxBusEpgService[service_pos].parental_rating,sizeof(GxBusEpgParentalRating)*GxBusEpgService[service_pos].parental_rating_num);
        GxBusEpgService[service_pos].parental_rating = NULL;
    }

    GxBusEpgService[service_pos].parental_rating_num = 0;
    GxBusEpgService[service_pos].parental_rating = (GxBusEpgParentalRating*)gx_epg_malloc(sizeof(GxBusEpgParentalRating)*info->parental_rating_num);
    if(GxBusEpgService[service_pos].parental_rating == NULL)
    {
#ifdef GX_BUS_EPG_DBUG
        GX_BUS_EPG_PRINTF("ERR---epg event update parental rating err!\n");
#endif
        GxBus_EpgUnlock();
        return GX_BUS_EPG_MEM_POOL_FULL;
    }
    GxBusEpgService[service_pos].parental_rating_num = info->parental_rating_num;
    memcpy(GxBusEpgService[service_pos].parental_rating,info->parental_rating,sizeof(GxBusEpgParentalRating)*info->parental_rating_num);

    GxBus_EpgUnlock();
    return GX_BUS_EPG_OK;
}

/**
 * @brief   添加一条event,
 * @param   GxBusEpgEventAddInfo* info:添加的信息

 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_SERVICE_FULL:service头数组满了
 *           GX_BUS_EPG_EVENT_FULL:event头数组已经满了
 *           GX_BUS_EPG_MEM_POOL_FULL:内存池满了
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数错误
 *           GX_BUS_EPG_EVENT_EXIST:传入的event已经存在
 */
status_t GxBus_EpgInfoAdd(GxBusEpgEventAddInfo* info)
{
	uint32_t num = 0;
	uint32_t i = 0;
	status_t ret = 0;
	int32_t service_pos = -1;
	uint8_t service_alloc_flag = 0;
	int32_t event_pos = -1;
	uint32_t event_pos_next = 0;
	int32_t event_pos_prvi = 0;
	uint32_t length = 0;
	GxBusEpgEventCurUseService* use_service = NULL;
	GxBusEpgFirstEvent** p = NULL;
	int8_t lang[4];
	GxBusEpgEventComp src;
	GxBusEpgEventComp cur;
	int32_t ver_act = 0;
	uint8_t *detail_info = NULL;

	if((info == NULL)||
			((info->detaile_description == NULL)&&(info->detaile_length != 0))||
			(info->detaile_description != NULL && 0 == info->detaile_length)||
			((info->name == NULL) || (info->name_length == 0))||
			(info->type > GX_EVENT_DETAILE) ||
			(NULL == info->event_time))
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	if(0 == GxBusEpgInitFlag)
	{
		GX_BUS_EPG_PRINTF("ERR---epg manage has been released\n");
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}

	if(event_modify != NULL)
	{
		event_modify(info);
	}

	// the funtion instead of "GxBus_EpgCurUseServiceGet" only for "GxBus_EpgInfoAdd"
	gx_epg_cur_use_service_get(&use_service,&num);

	/*搜寻该event是否属于已有的service,并且获取在servcie头数组中的位置*/
	for(i=0; i<num; i++)
	{
		src.ts_id = use_service[i].ts_id;
		src.service_id = use_service[i].service_id;
		src.original_id = use_service[i].original_id;
		cur.ts_id = info->ts_id;
		cur.service_id = info->service_id;
		cur.original_id = info->original_id;
		if(event_cmp(&src,&cur) == GX_BUS_EPG_OK )
		{
			service_pos = use_service[i].pos;

			GxBus_ConfigGetInt(EPG_MANAGER_VER_ACT,&ver_act,EPG_MANAGER_VER_ACT_DEFAULT);
			if(ver_act == EPG_MANAGER_VER_ACT_DEL_EVENT &&
					GxBusEpgService[service_pos].detaile_description_ver != info->ver &&
					info->type == GX_EVENT_DETAILE)
			{
				gx_epg_invalide_event_del(service_pos,0x7fffffff);//指定的时间无限大，删除所有event
				GxBusEpgService[service_pos].detaile_description_ver = info->ver;
			}
			break;
		}
	}

	/*拷贝本条event的语言代码*/
	for(i=0; i<3; i++)
	{
		lang[i] = info->language[i];
		lang[i] = tolower(lang[i]);
	}
	lang[3] = 0;
	/**该event不属于任何一个个service了,此时需要申请service头数组空间*/
	if(service_pos == -1)
	{
		service_pos = gx_epg_service_header_alloc();
		if(service_pos == -1)
		{
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("ERR---epg service full!\n");
#endif
			GxBus_EpgUnlock();
			return GX_BUS_EPG_SERVICE_FULL;
		}
		memset((&(GxBusEpgService[service_pos])),0,sizeof(GxBusEpgServiceHead));
		GxBusEpgService[service_pos].use_flag = 1;
		GxBusEpgService[service_pos].ts_id = info->ts_id;
		GxBusEpgService[service_pos].service_id = info->service_id;
		GxBusEpgService[service_pos].reference_service_id = info->reference_service_id;
		GxBusEpgService[service_pos].original_id = info->original_id;
		GxBusEpgService[service_pos].first_event = NULL;
		GxBusEpgService[service_pos].parental_rating = NULL;
		GxBusEpgService[service_pos].parental_rating_num = 0;
		GxBusEpgService[service_pos].detaile_description_ver = info->ver;
		service_alloc_flag = 1;
	}
	else //<判断是否存在同样的event
	{
		if(info->type == GX_EVENT_DETAILE)
		{
			if(GxBusEpgService[service_pos].reference_service_id == 0)
				GxBusEpgService[service_pos].reference_service_id = info->reference_service_id;
			if(gx_epg_event_exist_check(info, (int8_t*)lang,service_pos))
			{
#ifdef GX_BUS_EPG_DBUG
				fflush(stdout);
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_EVENT_EXIST;
			}
		}
		else if(GX_EVENT_PRESENT == info->type)
		{
			if(gx_epg_pf_event_exist_check(&GxBusEpgService[service_pos].present, info))
			{
#ifdef GX_BUS_EPG_DBUG
				fflush(stdout);
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_EVENT_EXIST;
			}
		}
		else if(GX_EVENT_FOLLOW == info->type)
		{
			if(gx_epg_pf_event_exist_check(&GxBusEpgService[service_pos].follow, info))
			{
#ifdef GX_BUS_EPG_DBUG
				fflush(stdout);
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_EVENT_EXIST;
			}
		}
	}

	if(0 == info->detaile_length)//如果没有详细描述定为"No Information"
	{
		if(epg_cfg_get_filter_level() == 0)
		{
			detail_info = (uint8_t *)NO_INFORMATION;
			info->detaile_length = strlen(NO_INFORMATION);
		}
	}
	else
	{
		detail_info = info->detaile_description;
	}

	switch(info->type)
	{
		case GX_EVENT_DETAILE:
			/*申请event头数组空间*/
			event_pos = gx_epg_event_header_alloc();
			if(event_pos == -1)
			{
				if(service_alloc_flag == 1)
				{
					gx_epg_service_header_free(service_pos);
				}
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg event full!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_EVENT_FULL;
			}
			memset((&(GxBusEpgEvent[event_pos])),0,sizeof(GxBusEpgEventHead));
			GxBusEpgEvent[event_pos].next_event = GX_BUS_EPG_INVALID_POS;

			ret = gx_epg_event_add(event_pos,info->name,info->name_length,GX_BUS_EPG_NAME_ADD);
			if(ret == GX_BUS_EPG_MEM_POOL_FULL)
			{
				if(service_alloc_flag == 1)
				{
					gx_epg_service_header_free(service_pos);
				}
				//gx_epg_event_header_GxCore_Free(event_pos);//gx_epg_event_add函数里面已经释放过了
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg mem pool full!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_MEM_POOL_FULL;
			}

			if(detail_info != NULL)
			{
				ret = gx_epg_event_add(event_pos,detail_info,info->detaile_length,GX_BUS_EPG_DETAILE_ADD);
				if(ret == GX_BUS_EPG_MEM_POOL_FULL)
				{
					if(service_alloc_flag == 1)
					{
						gx_epg_service_header_free(service_pos);
					}
					//gx_epg_event_header_GxCore_Free(event_pos);//gx_epg_event_add函数里面已经释放过了
#ifdef GX_BUS_EPG_DBUG
					GX_BUS_EPG_PRINTF("ERR---epg mem pool full!\n");
#endif
					GxBus_EpgUnlock();
					return GX_BUS_EPG_MEM_POOL_FULL;
				}
			}
			/*加入时间和event id*/
			if(info->event_time != NULL)
			{
				GxBusEpgEvent[event_pos].time.start_time = info->event_time->start_time;
				GxBusEpgEvent[event_pos].time.duration = info->event_time->duration;
			}
			else
			{
				GxBusEpgEvent[event_pos].time.start_time = 0;
				GxBusEpgEvent[event_pos].time.duration = 0;
			}

			GxBusEpgEvent[event_pos].event_id = info->event_id;
			GxBusEpgEvent[event_pos].reference_event_id = info->reference_event_id;
			GxBusEpgEvent[event_pos].ts_id = info->ts_id;
			GxBusEpgEvent[event_pos].service_id = info->service_id;

			/*把添加好内容的event插入service指向的event链中*/
			p = &(GxBusEpgService[service_pos].first_event);
			while(*p != NULL)
			{
				if(gx_epg_language_code_cmp((const char*)((*p)->language),(const char*)lang) == 0)
				{
					event_pos_prvi = event_pos_next = (*p)->pos;
					break;
				}
				p = (GxBusEpgFirstEvent**)&((*p)->next);
			}
			if(*p == NULL)
			{
				*p = (GxBusEpgFirstEvent*)gx_epg_malloc(sizeof(GxBusEpgFirstEvent));
				if(*p == NULL)
				{
					if(service_alloc_flag == 1)
					{
						gx_epg_service_header_free(service_pos);
					}
#ifdef GX_BUS_EPG_DBUG
					GX_BUS_EPG_PRINTF("ERR---epg mem pool full!\n");
#endif
					GxBus_EpgUnlock();
					return GX_BUS_EPG_MEM_POOL_FULL;
				}
				(*p)->pos = event_pos;
				memcpy((*p)->language,lang,3);
				(*p)->language[3] = 0;
				(*p)->next = NULL;
			}
			else
			{
				/*按照event的时间顺序插入*/
				if((*p)->pos == GX_BUS_EPG_INVALID_POS)//之前已经清空了所有event 只留first event
				{
					(*p)->pos = event_pos;
				}
				else if(GxBusEpgEvent[event_pos_next].time.start_time>=
						GxBusEpgEvent[event_pos].time.start_time)
				{
					(*p)->pos = event_pos;
					GxBusEpgEvent[event_pos].next_event = event_pos_next;
				}
				else
				{
					while(event_pos_next != GX_BUS_EPG_INVALID_POS)
					{
						if(GxBusEpgEvent[event_pos_next].time.start_time>=
								GxBusEpgEvent[event_pos].time.start_time)
						{
							GxBusEpgEvent[event_pos_prvi].next_event = event_pos;
							GxBusEpgEvent[event_pos].next_event= event_pos_next;
							break;

						}
						if(GxBusEpgEvent[event_pos_next].next_event == GX_BUS_EPG_INVALID_POS)
						{
							GxBusEpgEvent[event_pos_next].next_event= event_pos;
							break;
						}
						event_pos_prvi = event_pos_next;
						event_pos_next = GxBusEpgEvent[event_pos_next].next_event;
					}
				}


			}
			break;

		case GX_EVENT_PRESENT:

			ret = gx_epg_pf_add(info->event_id,
								info->event_time,
								info->name,
								detail_info,
								info->name_length,
								info->detaile_length,
								GX_EVENT_PRESENT,
								service_pos);

			if(ret == GX_BUS_EPG_MEM_POOL_FULL)
			{
				if(service_alloc_flag == 1)
				{
					gx_epg_service_header_free(service_pos);
				}
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg mem pool full!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_MEM_POOL_FULL;
			}
			break;

		case GX_EVENT_FOLLOW:

			ret = gx_epg_pf_add(info->event_id,
								info->event_time,
								info->name,
								detail_info,
								info->name_length,
								info->detaile_length,
								GX_EVENT_FOLLOW,
								service_pos);
			if(ret == GX_BUS_EPG_MEM_POOL_FULL)
			{
				if(service_alloc_flag == 1)
				{
					gx_epg_service_header_free(service_pos);
				}
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg mem pool full!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_MEM_POOL_FULL;
			}
			break;

		default:
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("ERR---epg event type err!\n");
#endif
			GxBus_EpgUnlock();
			return GX_BUS_EPG_PARAMETER_ERR;
			break;
	}
	/*更新父母锁信息*/
	if(info->parental_rating != NULL
			&&info->parental_rating_num != 0)
	{
		if(GxBusEpgService[service_pos].parental_rating != NULL)
		{
			gx_epg_free(GxBusEpgService[service_pos].parental_rating,sizeof(GxBusEpgParentalRating)*GxBusEpgService[service_pos].parental_rating_num);
			GxBusEpgService[service_pos].parental_rating = NULL;
		}
		GxBusEpgService[service_pos].parental_rating_num = 0;

		GxBusEpgService[service_pos].parental_rating = (GxBusEpgParentalRating*)gx_epg_malloc(sizeof(GxBusEpgParentalRating)*info->parental_rating_num);
		if(GxBusEpgService[service_pos].parental_rating == NULL)
		{
#ifdef GX_BUS_EPG_DBUG
			GX_BUS_EPG_PRINTF("ERR---epg event update parental rating err!\n");
#endif
			GxBus_EpgUnlock();
			return GX_BUS_EPG_OK;//该错误不影响其他event信息的记录
		}
		GxBusEpgService[service_pos].parental_rating_num = info->parental_rating_num;
		memcpy(GxBusEpgService[service_pos].parental_rating,info->parental_rating,sizeof(GxBusEpgParentalRating)*info->parental_rating_num);
	}
	GxBus_EpgUnlock();

	return GX_BUS_EPG_OK;
}

/**
 * @brief 清理指定的service的无效event信息,如果ts_id,service_id,original_id
 *        都为0xffffffff的话将会清除所有service的无效event
 * @param uint32_t ts_id:确定一条service
 *        uint32_t service_id:确定一条service
 *        uint32_t original_id:确定一条service
 *
 * @return   GX_BUS_EPG_OK:执行正常
 *           GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *           GX_BUS_EPG_PARAMETER_ERR:传入的参数错误
 */
status_t GxBus_EpgInvalidInfoClean(uint32_t ts_id,uint32_t service_id, uint32_t original_id)
{
	GxBusEpgEventCurUseService* use_service = NULL;
	uint32_t num = 0;
	uint32_t i = 0;
	int32_t service_pos = -1;
	GxTime time_cur = {0};
	GxBusEpgEventComp src;
	GxBusEpgEventComp cur;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	gx_epg_cur_use_service_get(&use_service,&num);
	GxCore_GetLocalTime(&time_cur);

	/*清除所有过期的event*/
	if(0xffffffff == ts_id
			&&0xffffffff == service_id)
	{
		for(i=0; i<num; i++)
		{
			service_pos = use_service[i].pos;
			if(service_pos == -1)
			{
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg have not existed!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_PARAMETER_ERR;
			}
			gx_epg_invalide_event_del(service_pos,time_cur.seconds);
		}
	}
	/*搜寻该event是否属于已有的service,并且获取在servcie头数组中的位置*/
	else
	{
		for(i=0; i<num; i++)
		{
			src.ts_id = use_service[i].ts_id;
			src.service_id = use_service[i].service_id;
			src.original_id = use_service[i].original_id;
			cur.ts_id = ts_id;
			cur.service_id = service_id;
			cur.original_id = original_id;
			if(event_cmp(&src,&cur) == GX_BUS_EPG_OK )
			{
				service_pos = use_service[i].pos;
				if(service_pos == -1)
				{
#ifdef GX_BUS_EPG_DBUG
					GX_BUS_EPG_PRINTF("ERR---epg have not existed!\n");
#endif
					GxBus_EpgUnlock();
					return GX_BUS_EPG_PARAMETER_ERR;
				}
				gx_epg_invalide_event_del(service_pos,time_cur.seconds);
				break;
			}
		}
	}

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}


/**
 * @brief 清理指定的service的全部event信息,并且释放service
 *
 * @param   uint32_t ts_id:确定一条service
 *          uint32_t service_id:确定一条service
 *          uint32_t original_id:确定一条service
 *
 * @return   GX_BUS_EPG_OK: 执行正常
 *           GX_BUS_EPG_NOT_LOCK: epg没有上锁
 *           GX_BUS_EPG_PARAMETER_ERR: 传入的参数错误
 */
status_t GxBus_EpgServiceClean(uint32_t ts_id,uint32_t service_id, uint32_t original_id)
{
	status_t ret = 0;
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	ret = gx_epg_service_clean(ts_id,service_id);
	if(ret != GX_BUS_EPG_OK)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---del service err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}
	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}

/**
 * @brief 清理离当前播放的service最远的service的epg信息,该函数是当add返回full
 *        时,如果不想自己处理就调用此函数
 * @param   void
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_ERR:执行失败
 *          GX_BUS_EPG_NOT_LOCK:epg没有上锁
 */
status_t GxBus_EpgFarServiceClean(void)
{
	GxBusEpgEventCurUseService* use_service = NULL;
	uint32_t num = 0;
	uint32_t ts_id_cur_play = 0;
	uint32_t service_id_cur_play = 0;
	uint32_t ts_id_del = 0;
	uint32_t service_id_del = 0;
	uint32_t i = 0;
	int32_t pos = 0;
	int32_t pos1 = 0;
	int32_t pos_temp1 = 0;
	int32_t pos_temp = -1;
	status_t ret = 0;
	uint32_t tp_id;
	uint32_t cur_prog;
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}

	//获取 ts_id_cur_play和service_id_cur_play 暂时无接口
	GxBus_PmViewInfoCurPlayGet(&tp_id, & cur_prog, &ts_id_cur_play, &service_id_cur_play);
	pos = GxBus_PmProgPosGetByServiceId(service_id_cur_play);
	if(pos == -1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg far service clean get pos err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}

	gx_epg_cur_use_service_get(&use_service,&num);
	/*只存在当前所看的service的epg*/
	if(num == 1 && use_service[i].ts_id == ts_id_cur_play && use_service[i].service_id == service_id_cur_play)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg far service clean no service can be del!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}

	for(i=0; i<num; i++)
	{
		if(use_service[i].ts_id != ts_id_cur_play||
				use_service[i].service_id != service_id_cur_play)
		{
			pos1 = GxBus_PmProgPosGetByServiceId(use_service[i].service_id);
			if(pos1 == -1 || pos1 == pos)
			{
				continue;//没有匹配的节目;
			}
			pos_temp1 = abs(pos1-pos);

			if(pos_temp1>pos_temp)
			{
				pos_temp = pos_temp1;
				ts_id_del = use_service[i].ts_id;
				service_id_del = use_service[i].service_id;
			}
		}
	}
	if(pos_temp == -1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg far service clean no service can be del!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}
	ret = gx_epg_service_clean(ts_id_del,service_id_del);
	if(ret != GX_BUS_EPG_OK)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg far service clean err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_ERR;
	}
	GxBus_EpgUnlock();

	return GX_BUS_EPG_OK;
}

/**
 * @brief 获得某个nvod 的event, 用于获取nvod的名字
 * @param   uint32_t reference_service_id:用于对比的 reference_service_id
 *          uint32_t reference_event_id:用于对比的 reference_event_id
 *          GxBusEpgEventHead* event:保存头信息的空间由调用者申请
 *          int32_t event_per_size:申请的event大小
 *
 * @return  GX_BUS_EPG_OK:执行正常
 *          GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
 *          GX_BUS_EPG_NOT_LOCK:epg没有上锁
 *          GX_BUS_EPG_EVENT_TOO_SMALL:申请的event空间太小
 */
status_t GxBus_NvodInfoGet(uint32_t ts_id,uint32_t reference_service_id,
							uint32_t reference_event_id,
							GxBusEpgEventHead* event,
							int32_t event_per_size)
{
	uint32_t event_pos = 0;
	uint32_t i = 0;
	GxBusEpgEventComp src;
	GxBusEpgEventComp cur;

	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(event == NULL || 0 == event_per_size)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}
	for(i=0; i<GxBusEpgServiceCount; i++)
	{
		src.ts_id = GxBusEpgService[i].ts_id;
		src.service_id = GxBusEpgService[i].service_id;
		src.original_id = 0;
		cur.ts_id = ts_id;
		cur.service_id = reference_service_id;
		cur.original_id = 0;
		if(event_cmp(&src,&cur) == GX_BUS_EPG_OK && GxBusEpgService[i].use_flag == 1)
		{
			if(GxBusEpgService[i].first_event != NULL)
			{
				event_pos = GxBusEpgService[i].first_event->pos;
			}
			else
			{
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_PARAMETER_ERR;
			}

			if((event_pos>=GxBusEpgEventCount)
#ifdef EPG_MALLOC
					|| (NULL == GxBusEpgEvent[event_pos].name.content)
#else
					|| (NULL == GxBusEpgEvent[event_pos].name)
#endif
					)
			{
#ifdef GX_BUS_EPG_DBUG
				GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
				GxBus_EpgUnlock();
				return GX_BUS_EPG_PARAMETER_ERR;
			}
			while(event_pos != GX_BUS_EPG_INVALID_POS)
			{
				if(GxBusEpgEvent[event_pos].event_id == reference_event_id)
				{
					status_t ret = GX_BUS_EPG_OK;
					ret = gx_epg_event_copy(event, &GxBusEpgEvent[event_pos], event_per_size);
					GxBus_EpgUnlock();
					return ret;
				}
				else
				{
					event_pos = GxBusEpgEvent[event_pos].next_event;
					if(event_pos == GX_BUS_EPG_INVALID_POS)
					{
						//gxlogd("not found event refer_id=%x\n",reference_event_id);
						GxBus_EpgUnlock();
						return GX_BUS_EPG_ERR;
					}
				}
			}
			GxBus_EpgUnlock();
			return GX_BUS_EPG_OK;
		}
	}
	GxBus_EpgUnlock();
	return GX_BUS_EPG_PARAMETER_ERR;
}

/**
 * @brief 		注册event是否相同的比较函数，没有注册则使用默认的比较方式，仅仅比较service id
 * @param
 * @return   	GX_BUS_EPG_OK:执行正常
GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
GX_BUS_EPG_NOT_LOCK:epg没有上锁
*/
status_t GxBus_RegisterCompFunc(GxBusEpgCheckEvent fun)
{
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(fun == NULL)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	event_cmp = fun;

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}
/**
 * @brief 		注销event是否相同的比较函数，从此以后使用默认的比较方式，仅仅比较service id
 * @param
 * @return   	GX_BUS_EPG_OK:执行正常
GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
GX_BUS_EPG_NOT_LOCK:epg没有上锁
*/
status_t GxBus_UnregisterCompFunc(void)
{
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	event_cmp = gx_epg_event_cmp;
	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}
/**
 * @brief 	    修改一条event的内容，包括name和descriptor

 * @param       uint32_t event_pos:用于定位修改哪条event
 *              GxBusEpgEventAddInfo* info:新的event信息

 * @return   	GX_BUS_EPG_OK:执行正常
GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
GX_BUS_EPG_NOT_LOCK:epg没有上锁
*/
status_t GxBus_EpgEventModify(uint32_t event_pos,GxBusEpgEventAddInfo* info)
{
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(event_pos>=GxBusEpgEventCount ||
			info == NULL ||
			GxBusEpgEvent[event_pos].event_id != info->event_id)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	gx_epg_event_content_del(event_pos);
	if(info->name != NULL)
	{
		gx_epg_event_add(event_pos,info->name,info->name_length,GX_BUS_EPG_NAME_ADD);
	}
	if(info->detaile_description != NULL)
	{
		gx_epg_event_add(event_pos,info->detaile_description,info->detaile_length,GX_BUS_EPG_DETAILE_ADD);
	}
	/*修改时间*/
	if(info->event_time != NULL)
	{
		GxBusEpgEvent[event_pos].time.start_time = info->event_time->start_time;
		GxBusEpgEvent[event_pos].time.duration = info->event_time->duration;
	}

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}
/**
 * @brief 		注册修改event的回调函数，在保存event前被调佣
 * @param
 * @return   	GX_BUS_EPG_OK:执行正常
GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
GX_BUS_EPG_NOT_LOCK:epg没有上锁
*/
status_t GxBus_RegisterModifyEventCb(GxBusEpgModifyEvent fun)
{
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	if(fun == NULL)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg parameter err!\n");
#endif
		GxBus_EpgUnlock();
		return GX_BUS_EPG_PARAMETER_ERR;
	}

	event_modify = fun;

	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}
/**
 * @brief 		注销修改event的回调函数
 * @param
 * @return   	GX_BUS_EPG_OK:执行正常
GX_BUS_EPG_PARAMETER_ERR:传入的参数不正确,无法匹配到正确值
GX_BUS_EPG_NOT_LOCK:epg没有上锁
*/
status_t GxBus_UnregisterModifyEventCb(void)
{
	GxBus_EpgLock();
	if(GxBusEpgMutexLockFlag !=1)
	{
#ifdef GX_BUS_EPG_DBUG
		GX_BUS_EPG_PRINTF("ERR---epg not lock!\n");
#endif
		return GX_BUS_EPG_NOT_LOCK;
	}
	event_modify = NULL;
	GxBus_EpgUnlock();
	return GX_BUS_EPG_OK;
}
