/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_sub_engine.c
* Author    : 	zhangling
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
#include <stdio.h>
#include "include/si_sub_engine_private.h"

#define SI_READ_STACK_SIZE         (16*1024)
#define SI_PARSE_STACK_SIZE        (16*1024)
#define SI_READ_PRI                (10)
#define SI_PARSE_PRI               (11)


//GxSubsystemSiEngineObj si[SI_ENGINE_NUM];
GxSubsystemSiEngineObj * si;
static handle_t obj_mutex;
static handle_t sem;
static int32_t si_first_init = 0;
static handle_t si_read_thread = 0;
static handle_t si_parse_thread = 0;
static uint32_t p_thread_count = 0;
static GxSubsystemSiEnginCacheHead g_cache = {0};
//static GxSubsystemSiEnginDmx si_dmx[SI_DMX_SUB_NUM];//和dmx的实际硬件数量一致
static GxSubsystemSiEnginDmx * si_dmx;//和dmx的实际硬件数量一致


static void read_thread(void *arg);
static void parse_thread(void *arg);
static void del_cache(GxSubsystemSiEnginCache* p);



static void si_sub_engine_add_to_cache(GxSubsystemSiEnginCache* p);
static GxSubsystemSiEnginCache* si_sub_engine_get_from_cache(void);
static void si_sub_engine_free_all_cache(GxSubsystemSiEngineObj* obj);
static void si_sub_deal_filter_data(int32_t filter_count, filter_handle_t* filter, dmx_handle_t dmx);
static void si_sub_deal_time_out(int32_t filter_count,dmx_handle_t dmx,filter_handle_t * filter);
static GxSubsystemSiEngineObj* si_sub_find_obj(filter_handle_t filter);
static void si_sub_parse_section(GxSubsystemSiEnginCache* p);

void show_filter_use_info(int32_t dmx_id)
{
    int32_t i= 0,filter_count = 64,using_filter_count= 0;
    for(i = 0;i<filter_count;i++)
    {
        gxlogd("i:%d,%x\n",i,si[i].filter);
        if(si[i].filter != -1)
        {
            using_filter_count++;
        }
    }
    gxlogd("%d\n",using_filter_count);

}
static void del_cache(GxSubsystemSiEnginCache* p)
{
	if(p->pre != NULL && p->next != NULL)//中间cache
	{
		((GxSubsystemSiEnginCache*)(p->pre))->next = p->next;
		((GxSubsystemSiEnginCache*)(p->next))->pre = p->pre;

	}

	if(p->pre == NULL && p->next != NULL && g_cache.head == (void*)p)//开头cache
	{
		g_cache.head = p->next;
		((GxSubsystemSiEnginCache*)(p->next))->pre = NULL;
	}
	else if(p->pre != NULL && p->next == NULL && g_cache.taile == (void*)p)//末尾cache
	{
		((GxSubsystemSiEnginCache*)(p->pre))->next = NULL;
		g_cache.taile = p->pre;
	}
	else if(p->pre == NULL && 
			p->next == NULL && 
			g_cache.head == (void*)p && 
			g_cache.taile == (void*)p)//既是开头又是末尾cache
	{
		g_cache.taile = NULL;
		g_cache.head = NULL;
	}
	
	return;
}
static void si_sub_engine_add_to_cache(GxSubsystemSiEnginCache* p)//规定从头开始添加
{
	GxSubsystemSiEnginCache* p_temp = NULL;
	
	GxCore_MutexLock(g_cache.mutex);
	if(g_cache.head == NULL && g_cache.taile == NULL)//第一个cache
	{
		g_cache.head = (void*)p;
		g_cache.taile = (void*)p;
		p->next = NULL;
		p->pre = NULL;
	}
	else
	{
		p_temp = (GxSubsystemSiEnginCache*)(g_cache.head);
		g_cache.head = (void*)p;
		p->next = (void*)p_temp;
		p_temp->pre = p;
		p->pre = NULL;
	}
	GxCore_MutexUnlock(g_cache.mutex);
	
	return;
}
static GxSubsystemSiEnginCache* si_sub_engine_get_from_cache(void)//规定从屁股开始拿
{
	GxSubsystemSiEnginCache* p_temp = NULL;

	GxCore_MutexLock(g_cache.mutex);
	if(g_cache.head == NULL && g_cache.taile == NULL)//cache里面没有东西
	{
		GxCore_MutexUnlock(g_cache.mutex);
		return NULL;
	}
	p_temp = (GxSubsystemSiEnginCache*)(g_cache.taile);
	if(p_temp->pre == NULL && 
			p_temp->next == NULL &&
			g_cache.head == p_temp)//唯一一个cache
	{
		g_cache.taile = NULL;
		g_cache.head = NULL;
	}
	else
	{

		((GxSubsystemSiEnginCache*)(p_temp->pre))->next = NULL;
		g_cache.taile = (GxSubsystemSiEnginCache*)(p_temp->pre);
	}
	GxCore_MutexUnlock(g_cache.mutex);

	return p_temp;
}

//如果以后发现遍历太慢，可以为每个obj建立自己的cache
static void si_sub_engine_free_all_cache(GxSubsystemSiEngineObj* obj)
{
	GxSubsystemSiEnginCache* p = NULL;
	GxSubsystemSiEnginCache* p_temp = NULL;
	
	GxCore_MutexLock(g_cache.mutex);
    p = (GxSubsystemSiEnginCache*)(g_cache.head);
	while(p != NULL)
	{
		p_temp = (GxSubsystemSiEnginCache*)(p->next);
		if(p->obj.me == obj)
		{
			del_cache(p);
			GxCore_Free(p->buffer);
			GxCore_Free(p);
		}
		p = p_temp;
	}
	GxCore_MutexUnlock(g_cache.mutex);
	return;
}

//重置OBJ控制块中的超时时间
static void si_sub_reset_time_out(SiEngineHandle si,uint32_t sec)
{
    GxSubsystemSiEngineObj * obj = NULL;
    GxCore_MutexLock(obj_mutex);
    if(si == -1)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_PRINTF("si already free\n");
#endif
        GxCore_MutexUnlock(obj_mutex);
        return;
    }
    obj = ((GxSubsystemSiEngineObj *)si)->me;
    if(obj == NULL)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_PRINTF("obj->me already free\n");
#endif
        GxCore_MutexUnlock(obj_mutex);
        return;
    }
    obj->time_ms_out = sec;
    GxCore_MutexUnlock(obj_mutex);
    return;
}

//当TABLE_OK时如果想继续过滤，需要重置控制块中的记录，否则过滤出来都是repeat
static void si_sub_reset_sectionnum(SiEngineHandle si,SiEngineParseStatus status)
{
	GxSubsystemSiEngineObj * obj = NULL;
	GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_PRINTF("si already free\n");
#endif
		GxCore_MutexUnlock(obj_mutex);
		return;
	}
	obj = ((GxSubsystemSiEngineObj *)si)->me;
	if(obj == NULL)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_PRINTF("obj->me already free\n");
#endif
		GxCore_MutexUnlock(obj_mutex);
		return;
	}
	if(status == TABLE_OK)
	{
		memset(obj->sec,0,256);
		obj->max_sec_count = 0;
		obj->sec_count = 0;
	}
	GxCore_MutexUnlock(obj_mutex);
	return;
}

static void si_sub_deal_time_out(int32_t filter_count,dmx_handle_t dmx,filter_handle_t *filter)
{
	int32_t i = 0,j = 0;
	SiEngineParseRet ret = 0;
	GxTime local_time;
    GxTime callbackstart_time;
    GxTime callbackend_time;
    GxSubsystemSiEngineObj obj = {0};
	
	GxCore_GetTickTime(&local_time);
	for(i=0; i<SI_ENGINE_NUM; i++)
    {
        GxCore_MutexLock(obj_mutex);
        if((si[i].status == CONTINUE) && (si[i].dmx == dmx))
        {
            memset(&obj,0,sizeof(GxSubsystemSiEngineObj));
            memcpy(&obj,&si[i],sizeof(GxSubsystemSiEngineObj));
            GxCore_MutexUnlock(obj_mutex);
            if(obj.time_ms_out <= local_time.seconds)
            {
                //基于设计考虑，一个dmx对应一个read线程，所以filter不用加锁保护
                for(j = 0 ; j<filter_count ; j++)
                {
                    if(obj.filter == filter[j])
                    {
                        filter[j] = -1;
                    }
                }

                if(obj.time_out != NULL)
                {
                    GxCore_GetTickTime(&callbackstart_time);
                    //gxlogd("goto time out function\n");
                    ret = obj.time_out((SiEngineHandle)&obj);
                    GxCore_GetTickTime(&callbackend_time);
                    callback_too_long_point(callbackstart_time.seconds,callbackend_time.seconds);
                }
                else
                {
                    ret = 0;
                }
                switch(ret)
                {
                    case 0:
                        //由用户停止过滤
                        break;
                    case 1:
                        si_sub_reset_time_out((SiEngineHandle)&obj,local_time.seconds+obj.time_ms/1000); 
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            GxCore_MutexUnlock(obj_mutex);
        }
	}

        return;
}

static GxSubsystemSiEngineObj* si_sub_find_obj(filter_handle_t filter)
{
	int32_t i = 0;
	 if(filter == -1)
    {
        return NULL;
    }

	GxCore_MutexLock(obj_mutex);
	for(i=0; i<SI_ENGINE_NUM; i++)
	{
		if(si[i].filter == filter && si[i].status == CONTINUE)
		{
			GxCore_MutexUnlock(obj_mutex);
			return &si[i];
		}
	}
	GxCore_MutexUnlock(obj_mutex);

	return NULL;
}

static void si_sub_deal_filter_data(int32_t filter_count, filter_handle_t* filter, dmx_handle_t dmx)
{
    GxSubsystemSiEngineObj* obj = NULL;
    GxSubsystemSiEnginCache* cache = NULL;
    uint32_t size = 0;
    int32_t i = 0;
    uint8_t* buffer = NULL;
    GxTime local_time;

    GxCore_GetTickTime(&local_time);


    for(i=0; i<filter_count; i++)
    {
        obj = si_sub_find_obj(filter[i]);
        GxCore_MutexLock(obj->mutex);
        if(obj == NULL)
        {
            GxCore_MutexUnlock(obj->mutex);
            continue;//很可能被释放了，找不到
        }
        buffer = GxCore_Malloc(SI_READ_SIZE);
        if(buffer == NULL)
        {
#ifdef GX_SI_ENGINE_ERR_DBUG
            SI_ENGINE_ERR_PRINTF("no memery!!\n");
#endif 
            GxCore_Free(buffer);
            GxCore_MutexUnlock(obj->mutex);
            return;
        }

        size =  GxSubSystem_DmxFilterRead(filter[i],buffer, SI_READ_SIZE);
        if(size == GX_DMX_SUB_ERR || size == 0)//size为0是因为驱动crc矫验失败，丢弃数据
        {
#ifdef GX_SI_ENGINE_ERR_DBUG
            SI_ENGINE_ERR_PRINTF("si read filter err!!\n");
#endif 
            gxlogd("si read size:%d\n",size);
            GxCore_Free(buffer);
            GxCore_MutexUnlock(obj->mutex);
            continue;
        }

        cache = GxCore_Malloc(sizeof(GxSubsystemSiEnginCache));
        if(cache == NULL)
        {
#ifdef GX_SI_ENGINE_ERR_DBUG
            SI_ENGINE_ERR_PRINTF("no memery!!\n");
#endif 
            GxCore_Free(buffer);
            GxCore_MutexUnlock(obj->mutex);
            return;
        }
        obj->time_ms_out = local_time.seconds+obj->time_ms/1000;
        memset(cache,0,sizeof(GxSubsystemSiEnginCache));
        memcpy(&(cache->obj),obj,sizeof(GxSubsystemSiEngineObj));
        //cache->obj = obj;
        cache->buffer = buffer;
        cache->size = size;

        si_sub_engine_add_to_cache(cache);
        GxCore_MutexUnlock(obj->mutex);
        GxCore_SemPost(sem);
    }
    return;
}

static void read_thread(void *arg)
{
	GxSubsystemSiEnginDmx* dmx = (GxSubsystemSiEnginDmx*)arg;
	int32_t ret = 0;
	
	while(1)
	{
		GxCore_MutexLock(dmx->mutex);
		while(dmx->count == 0)
		{
			GxCore_CondWait(dmx->cond, dmx->mutex, -1);
		}
		GxCore_MutexUnlock(dmx->mutex);

		ret = GxSubSystem_DmxFilterWait(dmx->dmx,dmx->filter);

		if(ret == GX_DMX_SUB_ERR)
		{
#ifdef GX_SI_ENGINE_ERR_DBUG
			SI_ENGINE_ERR_PRINTF("si wait evet err!!\n");//很可能是dmx被释放了造成读失败，多线程操作硬件时
			                                             //没发避免的
#endif  
			continue;
		}

		si_sub_deal_time_out(ret,dmx->dmx,dmx->filter);
		si_sub_deal_filter_data(ret, dmx->filter, dmx->dmx);
	}
	return;
}

SiEngineParseStatus si_sub_parse_sec_num(GxSubsystemSiEngineObj*obj,uint8_t* p)
{
	uint8_t sec_num = 0;
	uint32_t last_sec = 0;
    GxSubsystemSiEngineObj* obj_temp = NULL;

    GxCore_MutexLock(obj_mutex);
    obj_temp = ((GxSubsystemSiEngineObj *)obj)->me;
	if((obj_temp == NULL)||(obj_temp->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return ERR_PARAMS;
	}
    sec_num = SI_TODATA0_7BIT(p[6]);
    last_sec = SI_TODATA0_7BIT(p[7]);
	

	GxCore_MutexLock(obj_temp->mutex);
    GxCore_MutexUnlock(obj_mutex);
	if(obj_temp->max_sec_count == 0)
	{
		obj_temp->max_sec_count = last_sec + 1;//sec num 是从0开始的
	}
	if(obj_temp->sec[sec_num] == 0)
	{
		obj_temp->sec[sec_num] = 1;
		obj_temp->sec_count++;
		if(obj_temp->sec_count == obj_temp->max_sec_count)
		{
			GxCore_MutexUnlock(obj_temp->mutex);
			return TABLE_OK;
		}
		GxCore_MutexUnlock(obj_temp->mutex);
		return SECTION_OK;
	}
	else
	{
		GxCore_MutexUnlock(obj_temp->mutex);
		return REPEAT_SECTION;	
	}
}


#if 0
static void si_sub_obj_copy(GxSubsystemSiEngineObj * dest,GxSubsystemSiEngineObj * original)
{
    GxCore_MutexLock(obj_mutex);
	if((original == NULL)||(original->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return;
	}
    GxCore_MutexLock(original->mutex);
    GxCore_MutexUnlock(obj_mutex);
    memcpy(dest,original,sizeof(GxSubsystemSiEngineObj));
    GxCore_MutexUnlock(original->mutex);
}
#endif

static void si_sub_parse_section(GxSubsystemSiEnginCache* p)
{
	uint8_t* n_section = p->buffer;
	uint32_t total_size = p->size;
	uint32_t section_size = 0;
	GxSubsystemSiEngineObj * obj = &(p->obj);
    uint32_t ret = 0;
	SiEngineParseStatus sec_status = 0;
	void* table = NULL;
    uint32_t i =0;
    GxSubsystemSiEnginDmx * dmx_temp = NULL;
	GxTime local_time;
    GxTime callbackstart_time,callbackend_time;
	
	GxCore_GetTickTime(&local_time);
    //si_sub_obj_copy(&obj,p->obj);

	while(total_size >0 )
	{
		section_size = SI_TODATA0_11BIT(n_section[1],n_section[2]);
		if(section_size+3 > total_size)
		{
			return;
		}

		if(si_sub_crc32_check(n_section) != 0)
		{
#ifdef GX_SI_ENGINE_ERR_DBUG
			SI_ENGINE_PRINTF("si crc err!!\n");
#endif  
			GxSubSystem_SiEngineRestart((SiEngineHandle)obj->me);
			return;
		}

		if(obj->sec_num_check != NULL)
        {
            memset(&callbackstart_time,0,sizeof(GxTime));
            memset(&callbackend_time,0,sizeof(GxTime));
            GxCore_GetTickTime(&callbackstart_time);
            sec_status = obj->sec_num_check((SiEngineHandle)obj->me,n_section);
            GxCore_GetTickTime(&callbackend_time);
            callback_too_long_point(callbackstart_time.seconds,callbackend_time.seconds);
        }
		else
        {
            sec_status = si_sub_parse_sec_num(obj,n_section);
            if(sec_status == ERR_PARAMS)
            {
                return;
            }
        }
		if(sec_status == REPEAT_SECTION)//重复收到的sec
		{
			n_section += section_size+3;
			total_size -= section_size+3;
			continue;
		}
		if(obj->original != NULL)
        {   
            memset(&callbackstart_time,0,sizeof(GxTime));
            memset(&callbackend_time,0,sizeof(GxTime));
            GxCore_GetTickTime(&callbackstart_time);
            ret = obj->original((SiEngineHandle)obj->me,n_section, section_size+3,sec_status);
            GxCore_GetTickTime(&callbackend_time);
            callback_too_long_point(callbackstart_time.seconds,callbackend_time.seconds);
        }
		if(obj->table != NULL)
        {
            for(i = 0;i<SI_DMX_SUB_NUM;i++)      
            {                                 
                if(obj->dmx == si_dmx[i].dmx) 
                    dmx_temp = &si_dmx[i];    
            }                                 
            //在parse_thread读到时有可能已经被释放了
            if(dmx_temp)
            {
                table = si_sub_parse_section_standard(dmx_temp,n_section, section_size+3);
            }

            if(table != NULL)
            {
                memset(&callbackstart_time,0,sizeof(GxTime));
                memset(&callbackend_time,0,sizeof(GxTime));
                GxCore_GetTickTime(&callbackstart_time);
                ret = obj->table((SiEngineHandle)obj->me,table, sec_status);
                GxCore_GetTickTime(&callbackend_time);
                callback_too_long_point(callbackstart_time.seconds,callbackend_time.seconds);
            }
            else
            {
                si_sub_reset_time_out((SiEngineHandle)obj->me,0);
            }

            if(table != NULL)
            {
                si_sub_release_section(dmx_temp,table);//释放si lib
            }
        }
		switch(ret)
		{
			case 0:
                //所有停止操作由应用确定
				return;
				break;
			case 1:
                si_sub_reset_time_out((SiEngineHandle)obj->me,local_time.seconds+obj->time_ms/1000);
                    si_sub_reset_sectionnum((SiEngineHandle)obj->me,sec_status); 
				break;
			default:

                while(1)
                {
#ifdef GX_SI_ENGINE_ERR_DBUG
                    SI_ENGINE_ERR_PRINTF("err return value\n");

#endif
                }
				break;
		}
		n_section += section_size+3;
		total_size -= section_size+3;
	}
}

static void parse_thread(void *arg)
{
	GxSubsystemSiEnginCache* p = NULL;
	while(1)
	{
		GxCore_SemWait(sem);
		p = si_sub_engine_get_from_cache();
		if(p == NULL)
		{
			continue;//很可能被释放了，找不到
		}
		si_sub_parse_section(p);
		GxCore_Free(p->buffer);
		GxCore_Free(p);
	}
	return;
}

static int32_t init_si_ctr()
{
    uint32_t i = 0;

    if(si != NULL)
    {
        GxCore_Free(si);
        si = NULL;
    }
    si = GxCore_Mallocz(sizeof(GxSubsystemSiEngineObj) * SI_ENGINE_NUM);
    if(si == NULL)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("si ctr malloc failed\n");
#endif
        return GX_SI_ENGINE_ERR;
    }
    for(i = 0; i<SI_ENGINE_NUM;i++)
    {
        si[i].filter = -1;
    }
	
    if(si_dmx != NULL)
    {
        GxCore_Free(si_dmx);
        si_dmx = NULL;
    }
    si_dmx = GxCore_Mallocz(sizeof(GxSubsystemSiEnginDmx) * SI_DMX_SUB_NUM); 
    if(si_dmx == NULL)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("si_dmx malloc failed\n");
#endif
        return GX_SI_ENGINE_ERR;
    }
    
    for(i = 0 ; i<SI_DMX_SUB_NUM ; i++)
    {
        si_dmx[i].filter = GxCore_Mallocz(sizeof(filter_handle_t) * SI_DMX_SUB_FILTER_NUM);
        if(si_dmx[i].filter == NULL)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("filter ctr malloc failed\n");
#endif
            return -3;
        }
    }
    return 0;
}
/**
 * @brief      初始化si
 * 
 * @param 
 * 
 * @return    GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 *            GX_SI_ENGINE_PARAMETER_ERR 传入参数错误
 */
status_t GxSubSystem_SiEngineInit(GxSubsystemSiEngineInit* init)
{
	status_t ret = 0;
	uint32_t i = 0;
	uint32_t size = 0;
	uint32_t pri = 0;
    PROTOCOL_TYPE protocol = 0; 


	if(si_first_init != 0)
	{
		return GX_SI_ENGINE_SUCCESS;
	}
	si_first_init =1;
    ret = GxSubSystem_DmxInit();
    if(ret < 0)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("GxSubSystem_DmxInit failed\n");
#endif
        return GX_SI_ENGINE_ERR;
    }
    GxSubsystem_DmxGetHardwareNum(&SI_DMX_SUB_NUM,&SI_DMX_SUB_FILTER_NUM);
    ret = init_si_ctr();
    if(ret < 0)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("si ctr init failed\n");
#endif
        return GX_SI_ENGINE_ERR;
    } 
    GxCore_MutexCreate(&obj_mutex);
	GxCore_MutexCreate(&g_cache.mutex);
    //dmx_num = (init->init_dmx_num>SI_DMX_SUB_NUM)?(SI_DMX_SUB_NUM):(init->init_dmx_num);	
    if((init->init_dmx_num > SI_DMX_SUB_NUM)||(init->init_dmx_num <= 0))
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("too big init_dmx_num\n");
#endif
        return GX_SI_ENGINE_PARAMETER_ERR;
    }

/* 
 *3个for循环分别作用为:
 *1.初始化dmx控制块
 *2.按照用户需要的初始化dmx注册函数
 *3.为了预防用户没有初始化全部的dmx而在使用时传入错误的dmx_id，进行默认初始化操作
 * */
	for(i=0; i<SI_DMX_SUB_NUM; i++)
    {
        si_dmx[i].dmx = -1;
        memset(si_dmx[i].filter,-1,SI_DMX_SUB_FILTER_NUM*sizeof(filter_handle_t));
    }    

    //正常的初始化
    for(i = 0 ; i< init->init_dmx_num ; i++)
    {
        if(init->dmx_protocol[i].dmx_id >= SI_DMX_SUB_NUM)
        {
#ifdef GX_SI_ENGINE_ERR_DBUG
            SI_ENGINE_ERR_PRINTF("si create dmx_id too big\n");
#endif
            return GX_SI_ENGINE_PARAMETER_ERR;
        }

        si_dmx[init->dmx_protocol[i].dmx_id].usr_init_flag = 1;
        if(si_dmx[init->dmx_protocol[i].dmx_id].parse_arry != NULL)//重复注册
        {
#ifdef GX_SI_ENGINE_ERR_DBUG
            SI_ENGINE_ERR_PRINTF("si Repeat registration\n");
#endif
            return GX_SI_ENGINE_ERR;
        }
        else
        {
            protocol = init->dmx_protocol[i].protocol;
            if(init->dmx_protocol[i].protocol == PROTOCOL_DVB)
            {
                si_dmx[init->dmx_protocol[i].dmx_id].parse_arry = si_sub_get_parser_ops(0);
            }else if(init->dmx_protocol[i].protocol == PROTOCOL_ATSC)
            {
                si_dmx[init->dmx_protocol[i].dmx_id].parse_arry = si_sub_get_parser_ops(1);
            }
            else
            {
#ifdef GX_SI_ENGINE_ERR_DBUG
                SI_ENGINE_ERR_PRINTF("unknow protocol\n");
#endif
                return GX_SI_ENGINE_PARAMETER_ERR;
            }
        }
    }

    //当用户初始化dmx小于实际dmx数时，对没有初始化的dmx进行默认初始化注册函数
    for(i = 0;i < SI_DMX_SUB_NUM;i++)
    {
        if(si_dmx[i].parse_arry == NULL)
        {
            if(protocol == PROTOCOL_DVB)
            {
                si_dmx[i].parse_arry = si_sub_get_parser_ops(0);
            }
            else
            {
                si_dmx[i].parse_arry = si_sub_get_parser_ops(1);
            }
        }
    }

        
	GxCore_MutexCreate(&(si_dmx[0].mutex));
	GxCore_MutexCreate(&(si_dmx[1].mutex));
	GxCore_CondInit(&(si_dmx[0].cond));
	GxCore_CondInit(&(si_dmx[1].cond));


	GxCore_SemCreate(&sem, 0);
	if(init->parse_thread_count == 0)
	{
		p_thread_count = 1;
	}
	else
	{
		p_thread_count = init->parse_thread_count;
	}

	if(init->read_stack ==0 )
	{
		size = SI_READ_STACK_SIZE;
	}
	else
	{
		size = init->read_stack;
	}
	if(init->read_pri == 0)
	{
		pri = SI_READ_PRI;
	}
	else
	{
		pri = init->read_pri;
	}

	ret = GxCore_ThreadCreate("si read dmx0", &si_read_thread,read_thread, (void*)(&si_dmx[0]), size, pri);
	if (ret != GXCORE_SUCCESS) 
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si create read dmx0 thread err!!\n");
#endif  
		return GX_SI_ENGINE_ERR;
	}
	ret = GxCore_ThreadCreate("si read dmx1", &si_read_thread,read_thread, (void*)(&si_dmx[1]), size, pri);
	if (ret != GXCORE_SUCCESS) 
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si create read dmx1 thread err!!\n");
#endif  
		return GX_SI_ENGINE_ERR;
	}
	if(init->parse_stack ==0 )
	{
		size = SI_PARSE_STACK_SIZE;
	}
	else
	{
		size = init->parse_stack;
	}
	if(init->parse_pri == 0)
	{
		pri = SI_PARSE_PRI;
	}
	else
	{
		pri = init->parse_pri;
	}

	for(i=0; i<p_thread_count; i++)
	{
		ret = GxCore_ThreadCreate("parse read", &si_parse_thread,parse_thread, NULL, size, pri);
		if (ret != GXCORE_SUCCESS) 
		{
#ifdef GX_SI_ENGINE_ERR_DBUG
			SI_ENGINE_ERR_PRINTF("si create parse thread err!!\n");
#endif  
			return GX_SI_ENGINE_ERR;
		}
	}
	return GX_SI_ENGINE_SUCCESS;
}
/**
 * @brief      申请一次过滤
 * 
 * @param      
 * 
 * @return     成功申请到的si句柄，如果返回-1 则申请失败
 */

SiEngineHandle GxSubSystem_SiEngineAlloc(GxSubsystemSiEngineAlloc* para)
{
	int32_t i = 0;
	GxSubsystemSiEngineObj* si_obj = NULL;
    dmx_handle_t dmx = 0;
	GxSubsystemSiEnginDmx* si_dmx_temp = NULL;
	filter_handle_t filter = 0;
	GxTime local_time;

	GxCore_MutexLock(obj_mutex);
	for(i=0; i<SI_ENGINE_NUM; i++)
	{
		if(si[i].status == STOP)
		{
			si_obj = &si[i];
			si_obj->status = PAUSE;
			break;
		}
	}
    if(si_obj == NULL)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("no free si!!\n");
#endif  
		GxCore_MutexUnlock(obj_mutex);
        return -1;
    }
    if(para->dmx_id >= SI_DMX_SUB_NUM)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("err dmx id\n");
#endif
		GxCore_MutexUnlock(obj_mutex);
        return -1;
    }

    if(0 == si_dmx[para->dmx_id].usr_init_flag)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("error DMX_ID\n");
#endif
    }
	dmx = GxSubSystem_DmxOpen(para->dmx_id,para->ts);
	if(dmx == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si open dmx err!!\n");
#endif  
		GxCore_MutexUnlock(obj_mutex);
        return -1;
	}
	si_obj->dmx = dmx;

	filter =  GxSubSystem_DmxFilterAlloc(dmx, &(para->filter));
	if(filter == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si alloc filter  err!!\n");
#endif  
		GxCore_MutexUnlock(obj_mutex);
        return -1;
	}
	si_obj->filter = filter;

	si_obj->original = para->original;
	si_obj->table = para->table;
	si_obj->time_out = para->time_out;
	si_obj->sec_num_check = para->sec_num_check;
	si_obj->time_ms = para->time_ms;//超时时间
    si_obj->ptr = para->p;
    si_obj->me = si_obj;


	GxCore_GetTickTime(&local_time);
	//用于计算超时当time_ms_out=time_ms+本地时间时 本次过滤超时
	si_obj->time_ms_out = local_time.seconds + para->time_ms/1000;
	GxCore_MutexCreate(&si_obj->mutex);
	

    /* 
     *考虑一个dmx在使用时只能从一个ts流入，所以在使用一个dmx时只能有一个dmx句柄
     * */
    if((si_dmx[para->dmx_id].dmx == -1) ||(si_dmx[para->dmx_id].dmx == dmx) )
    {
        si_dmx_temp = &si_dmx[para->dmx_id];
    }else
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("dmx ts id already used\n");
#endif  
		GxCore_MutexUnlock(obj_mutex);
        return -1;
    }
	GxCore_MutexLock(si_dmx_temp->mutex);
	GxCore_MutexUnlock(obj_mutex);

	si_dmx_temp->dmx = dmx;
    if(si_dmx_temp->count == 0)
    {
        si_dmx_temp->front_type = para->front_type;
    }
	si_dmx_temp->count++;
	GxCore_CondSignal(si_dmx_temp->cond);
	GxCore_MutexUnlock(si_dmx_temp->mutex);
	return (SiEngineHandle)si_obj;
}

/**
 * @brief      启动过滤
 * 
 * @param      
 * 
 * @return    GX_SI_ENGINE_PARAMETER_ERR 传入的参数不对
 *            GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 */
status_t  GxSubSystem_SiEngineStart(SiEngineHandle si)
{
	int32_t ret = 0;
	GxSubsystemSiEngineObj* si_obj = (GxSubsystemSiEngineObj*)si;
    GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si start err  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}
		
	si_obj = ((GxSubsystemSiEngineObj*)si)->me;
  
	if((si_obj == NULL)||(si_obj->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}
	
	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);

	if(si_obj->status == CONTINUE)
	{
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_SUCCESS;
	}
	ret = GxSubSystem_DmxFilterStart(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si start err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}
	si_obj->status = CONTINUE;
	GxCore_MutexUnlock(si_obj->mutex);
	return GX_SI_ENGINE_SUCCESS;
}

/**
 * @brief      释放过滤
 * 
 * @param      
 * 
 * @return    GX_SI_ENGINE_PARAMETER_ERR 传入的参数不对
 *            GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 */
status_t  GxSubSystem_SiEngineFree(SiEngineHandle si)
{
	int32_t ret = 0;
	handle_t mutex = 0;
	int32_t i = 0;
	GxSubsystemSiEngineObj* si_obj = NULL;

	GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si free err  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}

    si_obj = ((GxSubsystemSiEngineObj *)si)->me;

	if((si_obj == NULL)||(si_obj->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}
    /*先使用针对整体的全局锁（obj_mutex）保证在获取me之前不被其他线程改变SI句柄
     *在找到me（该句柄对应的OBJ控制块指针）之后,为了提高效率所以应该尽量避免加全局锁,
     *所以使用obj内私有锁，解全局锁
     * */
	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);

	ret = GxSubSystem_DmxFilterStop(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si stop err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	ret = GxSubSystem_DmxFilterRelease(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si release err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	ret = GxSubSystem_DmxClose(si_obj->dmx); 
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si close dmx err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	si_sub_engine_free_all_cache(si_obj);

	mutex = si_obj->mutex;
	
	for(i=0; i<SI_DMX_SUB_NUM; i++)//释放一次过滤read的条件变量要跟上
	{
		if(si_dmx[i].dmx == si_obj->dmx)
		{
			GxCore_MutexLock(si_dmx[i].mutex);
			si_dmx[i].count--;
			if(si_dmx[i].count == 0)
			{
				si_dmx[i].dmx = -1;
			}
			GxCore_MutexUnlock(si_dmx[i].mutex);
			break;
		}
	}

	GxCore_MutexLock(obj_mutex);
	memset((void*)si_obj,0,sizeof(GxSubsystemSiEngineObj));//需要和alloc互斥
	si_obj->dmx = -1;
	si_obj->filter = -1;
	GxCore_MutexUnlock(obj_mutex);
	
	GxCore_MutexUnlock(mutex);
	GxCore_MutexDelete(mutex);
	return GX_SI_ENGINE_SUCCESS;
}

/**
 * @brief      暂停过滤 不会释放硬件，但是会清除所有数据缓冲
 * 
 * @param      
 * 
 * @return    GX_SI_ENGINE_PARAMETER_ERR 传入的参数不对
 *            GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 */
status_t  GxSubSystem_SiEnginePause(SiEngineHandle si)
{
	int32_t ret = 0;
	GxSubsystemSiEngineObj* si_obj = NULL;

    GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si free err  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}

    si_obj = ((GxSubsystemSiEngineObj *)si)->me;
	if((si_obj == NULL)||(si_obj->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}

	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);

	ret = GxSubSystem_DmxFilterStop(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si stop err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	si_sub_engine_free_all_cache(si_obj);
	
	si_obj->status = PAUSE;
	
	GxCore_MutexUnlock(si_obj->mutex);
	return GX_SI_ENGINE_SUCCESS;
}

/**
 * @brief      改变过滤参数，调用次接口前需要先GxSubSystem_SiEnginePause。
 * 之后如果要启动过滤需要GxSubSystem_SiEngineStarti,dmx和ts是不能改变的
 * 
 * @param      
 * 
 * @return    GX_SI_ENGINE_PARAMETER_ERR 传入的参数不对
 *            GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 */
status_t  GxSubSystem_SiEngineModify(SiEngineHandle si,GxSubsystemSiEngineAlloc* para)
{
	int32_t ret = 0;
	GxSubsystemSiEngineObj* si_obj = NULL;

    GxCore_MutexLock(obj_mutex);
	if(si == -1 || para == NULL)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si free err  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}

    si_obj = ((GxSubsystemSiEngineObj *)si)->me;
    if(si_obj == NULL)
    {
#ifdef GX_SI_ENGINE_ERR_DBUG
        SI_ENGINE_ERR_PRINTF("si free already\n");
#endif
        GxCore_MutexUnlock(obj_mutex);
        return GX_SI_ENGINE_PARAMETER_ERR;
    }
	
	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);

	ret = GxSubSystem_DmxFilterStop(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si stop err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	si_sub_engine_free_all_cache(si_obj);
	
	si_obj->status = PAUSE;
	
	ret = GxSubSystem_DmxFilterModify(si_obj->filter,&(para->filter));
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si modify err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}
	si_obj->original = para->original;
	si_obj->table = para->table;
	si_obj->time_out = para->time_out;
	si_obj->sec_num_check = para->sec_num_check;
	si_obj->time_ms = para->time_ms;//超时时间
    si_obj->ptr = para->p;
	memset(si_obj->sec,0,256);//每个表最多256个段
	si_obj->sec_count = 0;
	si_obj->max_sec_count = 0;

//	si_sub_engine_free_all_cache(si_obj);PAUSE是已经把cache清除了
	
	ret = GxSubSystem_DmxFilterStart(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si start err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}
	si_obj->status = CONTINUE;
	GxCore_MutexUnlock(si_obj->mutex);
	return GX_SI_ENGINE_SUCCESS;
}
/**
 * @brief      重启过滤 不会释放硬件，不会清除数据缓冲
 * 
 * @param      
 * 
 * @return    GX_SI_ENGINE_PARAMETER_ERR 传入的参数不对
 *            GX_SI_ENGINE_ERR  失败
 *            GX_SI_ENGINE_SUCCESS   成功
 */
status_t  GxSubSystem_SiEngineRestart(SiEngineHandle si)
{
	int32_t ret = 0;
	GxSubsystemSiEngineObj* si_obj = NULL;
	GxTime local_time;
	
	GxCore_GetTickTime(&local_time);
    GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si free err  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}
	
	si_obj = ((GxSubsystemSiEngineObj*)si)->me;
   
	if((si_obj == NULL)||(si_obj->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return GX_SI_ENGINE_PARAMETER_ERR;
	}

	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);

	ret = GxSubSystem_DmxFilterStop(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si stop err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}
	ret = GxSubSystem_DmxFilterStart(si_obj->filter);
	if(ret != GX_DMX_SUB_SUCCESS)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si start err!!\n");
#endif  
		GxCore_MutexUnlock(si_obj->mutex);
		return GX_SI_ENGINE_ERR;
	}

	si_obj->time_ms_out = local_time.seconds + si_obj->time_ms/1000;

	GxCore_MutexUnlock(si_obj->mutex);
	return GX_SI_ENGINE_SUCCESS;
}
/**
 * @brief      获取用户私有信息
 * 
 * @param      
 * 
 * @return    用户申请si时传入的私有信息指针
 */
void* GxSubSystem_SiEngineGetUser(SiEngineHandle si)
{
	void* ret = NULL;
	GxSubsystemSiEngineObj* si_obj = NULL;
	
    GxCore_MutexLock(obj_mutex);
	if(si == -1)
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si get user  handle = -1!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return NULL;
	}
	
	si_obj = ((GxSubsystemSiEngineObj*)si)->me;

	if((si_obj == NULL)||(si_obj->status == STOP))//必须在GxCore_MutexLock(si_obj->mutex)之前判断
	{
#ifdef GX_SI_ENGINE_ERR_DBUG
		SI_ENGINE_ERR_PRINTF("si already free!!\n");
#endif  
        GxCore_MutexUnlock(obj_mutex);
		return NULL;
	}
	GxCore_MutexLock(si_obj->mutex);
    GxCore_MutexUnlock(obj_mutex);
	ret = si_obj->ptr;
	GxCore_MutexUnlock(si_obj->mutex);
	return ret;
}
/* End of file -------------------------------------------------------------*/

