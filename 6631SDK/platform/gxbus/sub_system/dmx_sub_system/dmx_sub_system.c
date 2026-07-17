/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	dmx_sub_system.c
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
#include <string.h>
#include "sub_system/dmx_sub_system/dmx_sub_system.h"
#include "include/dmx_sub_driver.h"
#include "include/dmx_sub_private.h"

//GxSubsystemDmxCtr dmx_sub_ctr[DMX_SUB_NUM];
GxSubsystemDmxCtr * dmx_sub_ctr;
static int32_t dmx_sub_first_init = 0;


#define GX_DMX_SUB_CHECK_DMX_HANDLE(p,err) \
if(strcmp((const char*)(p->name),(const char*)(dmx_sub_ctr[p->dmx_id].name)) != 0)\
{\
    DMX_SUB_ERR_PRINTF("handle err!\n");\
    return err;\
}\

uint32_t gx_dmx_sub_init(void)
{
    int32_t i = 0 , j = 0;
    if(dmx_sub_ctr != NULL)
    {
        GxCore_Free(dmx_sub_ctr);
        dmx_sub_ctr = NULL;
    }
    dmx_sub_ctr = GxCore_Mallocz(sizeof(GxSubsystemDmxCtr)*DMX_SUB_NUM);
    if(dmx_sub_ctr == NULL)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx_sub_ctr malloc failed\n");
#endif
        return -1;
    }
    //memset(dmx_sub_ctr,0,sizeof(GxSubsystemDmxCtr)*DMX_SUB_NUM);
    for(i = 0 ; i<DMX_SUB_NUM ; i++)
    {
        dmx_sub_ctr[i].slot = GxCore_Mallocz(sizeof(GxSubsystemSlotCtr) * DMX_SUB_SLOT_NUM);
        if(dmx_sub_ctr[i].slot == NULL)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("slot ctr malloc failed\n");
#endif
            return -2;
        }
        for(j = 0 ; j < DMX_SUB_SLOT_NUM ; j++)
        {
            dmx_sub_ctr[i].slot[j].filter = GxCore_Mallocz(sizeof(GxSubsystemFilterCtr) * DMX_SUB_FILTER_NUM);
            if(dmx_sub_ctr[i].slot[j].filter == NULL)
            {
#ifdef GX_DMX_SUB_ERR_DBUG
                DMX_SUB_ERR_PRINTF("filter ctr malloc failed\n");
#endif
                return -3;
            }
        }

    }
    
    for(i=0; i<DMX_SUB_NUM; i++)
    {
        dmx_sub_ctr[i].mutex = -1;
        dmx_sub_ctr[i].device = -1;
        dmx_sub_ctr[i].module = -1;
        dmx_sub_ctr[i].dmx_id = i;
		GxCore_MutexCreate(&dmx_sub_ctr[i].mutex);
    }
    return 0;
}
/*所有的dmx子系统控制块都要查询有没有相同pid的slot，因为我们的硬件dmx实际上还
 * 是只有一个，没有真正的多dmx硬件*/
slot_handle_t gx_dmx_sub_check_pid(GxSubsystemDmxCtr* dmx,uint16_t pid)
{
    uint32_t i = 0;
    uint32_t j = 0;

    for(i=0; i<DMX_SUB_NUM; i++)
    {
        for(j=0; j<DMX_SUB_SLOT_NUM; j++)
        {
            if(dmx_sub_ctr[i].slot[j].pid == pid && 
                    dmx_sub_ctr[i].slot[j].dmx == (dmx_handle_t)dmx &&
                    dmx_sub_ctr[i].slot[j].count != 0)
            {
                return (slot_handle_t)&(dmx_sub_ctr[i].slot[j]);
            }
            else if(dmx_sub_ctr[i].slot[j].pid == pid && 
                         dmx_sub_ctr[i].slot[j].dmx != (dmx_handle_t)dmx &&
                              dmx_sub_ctr[i].slot[j].count != 0)//另外一路dmx分配了该pid的slot
            {
#ifdef GX_DMX_SUB_ERR_DBUG
                DMX_SUB_PRINTF("\ndmx = %x have the same pid slot !!\n",dmx_sub_ctr[i].slot[j].dmx);
#endif
                return 0xffffffff;
            }
        }
    }
    return 0;
}

filter_handle_t gx_dmx_sub_alloc_filter(slot_handle_t slot,GxSubsystemDmxAllocFilter* para)
{
    GxSubsystemSlotCtr* slot_ctr = (GxSubsystemSlotCtr*)slot;    
    GxSubsystemDmxCtr* dmx_ctr = (GxSubsystemDmxCtr*)(slot_ctr->dmx);
    int32_t filter_id = 0;

    filter_id = gx_dmx_sub_driver_alloc_filter(dmx_ctr,slot_ctr,para);
    if(filter_id < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub  alloc filter err !!\n");
#endif
        return -1;
    }
    slot_ctr->filter[filter_id].id = filter_id;
    slot_ctr->filter[filter_id].dmx = slot_ctr->dmx;
    slot_ctr->filter[filter_id].slot = slot;
    slot_ctr->count++;

    return (filter_handle_t)&(slot_ctr->filter[filter_id]);
}

filter_handle_t gx_dmx_sub_alloc_slot_filter(GxSubsystemDmxCtr* dmx,GxSubsystemDmxAllocFilter* para)
{
    GxSubsystemSlotCtr* slot_ctr = NULL;    
    int32_t slot_id = 0;
    int32_t filter_id = 0;
    
	slot_id = gx_dmx_sub_driver_alloc_slot(dmx,para);
    if(slot_id < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub alloc slot err !!\n");
#endif  
        return -1;
    }
    dmx->slot[slot_id].id = slot_id;
    dmx->slot[slot_id].dmx = (dmx_handle_t)dmx;
    dmx->slot[slot_id].pid = para->pid;
    slot_ctr = (GxSubsystemSlotCtr*)&(dmx->slot[slot_id]);

    filter_id = gx_dmx_sub_driver_alloc_filter(dmx,slot_ctr,para);
    if(filter_id < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub  alloc filter err !!\n");
#endif
        return -1;
    }
    slot_ctr->filter[filter_id].id = filter_id;
    slot_ctr->filter[filter_id].dmx = slot_ctr->dmx;
    slot_ctr->filter[filter_id].slot = (slot_handle_t)slot_ctr;
    slot_ctr->count++;

    return (filter_handle_t)&(slot_ctr->filter[filter_id]);
}

static uint32_t get_hard_num(void)
{
    DMX_SUB_NUM = 2;
    DMX_SUB_SLOT_NUM = 64;
    DMX_SUB_FILTER_NUM = 64;
    return GX_DMX_SUB_SUCCESS; 
}

void GxSubsystem_DmxGetHardwareNum(uint32_t * dmx,uint32_t * channel_num)
{
    *dmx = DMX_SUB_NUM;
    (*channel_num) = (DMX_SUB_SLOT_NUM > DMX_SUB_FILTER_NUM)?(DMX_SUB_SLOT_NUM):(DMX_SUB_FILTER_NUM);
}
/**
 * @brief      初始化dmx子系统
 * 
 * @param     
 *           
 * 
 * @return     GX_DMX_SUB_ERR:执行错误
 *             GX_DMX_SUB_SUCCESS：执行成功  
 */
int32_t GxSubSystem_DmxInit(void)
{
    uint32_t ret = 0;

    ret = get_hard_num();
    if(ret != GX_DMX_SUB_SUCCESS)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub get hard num failed\n");
#endif
        return GX_DMX_SUB_PARAMETER_ERR; 
    }
    
    if(dmx_sub_first_init == 0)
    {
        dmx_sub_first_init = 1;
        ret = gx_dmx_sub_init();
        if(ret < 0)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("dmx ctr init failed\n");
#endif
            return GX_DMX_SUB_ERR;
        }
    }
    return GX_DMX_SUB_SUCCESS;
}
/**
 * @brief      打开dmx子系统，获取控制句柄
 * 
 * @param       uint32_t dmx_id:dmx的id，在我们的系统中有两路dmx，id为0或者是1
 *              uint32_t ts：和dmx绑定的ts编号，从0开始
 * 
 * @return      dmx子系统的控制块,如果是0表示打开失败
 */
dmx_handle_t GxSubSystem_DmxOpen(uint32_t dmx_id,uint32_t ts)
{
    int32_t ret = 0;
	int8_t*  name = NULL;

    if(dmx_id >= DMX_SUB_NUM)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub open paramter!\n");
#endif
        return -1;
    }
    GxCore_MutexLock(dmx_sub_ctr[dmx_id].mutex);
    /*相同的dmx已经打开过*/
    if(dmx_sub_ctr[dmx_id].count != 0 && dmx_sub_ctr[dmx_id].ts == ts)
    {
        dmx_sub_ctr[dmx_id].count++;
		GxCore_MutexUnlock(dmx_sub_ctr[dmx_id].mutex);
        return (dmx_handle_t)&dmx_sub_ctr[dmx_id];
    }
    else if(dmx_sub_ctr[dmx_id].count != 0 && dmx_sub_ctr[dmx_id].ts != ts)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub already open!\n");
#endif
		GxCore_MutexUnlock(dmx_sub_ctr[dmx_id].mutex);
        return -1;
    }
    dmx_sub_ctr[dmx_id].count++;
    dmx_sub_ctr[dmx_id].ts = ts;
    dmx_sub_ctr[dmx_id].device = gx_dmx_sub_driver_open_device(0);
    if(dmx_sub_ctr[dmx_id].device < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub open device err!\n");
#endif
		GxCore_MutexUnlock(dmx_sub_ctr[dmx_id].mutex);
        return -1;
    }
    dmx_sub_ctr[dmx_id].module = gx_dmx_sub_driver_open_module(dmx_sub_ctr[dmx_id].device,dmx_id);
    if(dmx_sub_ctr[dmx_id].module < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub open module err!\n");
#endif
		GxCore_MutexUnlock(dmx_sub_ctr[dmx_id].mutex);
        return -1;
    }
    ret = gx_dmx_sub_driver_config(dmx_sub_ctr[dmx_id].device,dmx_sub_ctr[dmx_id].module,ts);
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub config module err!\n");
#endif
        return -1;
    }
	name = GxCore_Malloc(DMX_SUB_MAX_NAME);
	memset((void*)name,0,DMX_SUB_MAX_NAME);
	snprintf((char*)name, DMX_SUB_MAX_NAME,"dmx%d:%d",dmx_id,ts);
    memcpy(dmx_sub_ctr[dmx_id].name,name,DMX_SUB_MAX_NAME-1);
    dmx_sub_ctr[dmx_id].name[DMX_SUB_MAX_NAME-1] = 0;
	GxCore_Free(name);
	GxCore_MutexUnlock(dmx_sub_ctr[dmx_id].mutex);
    return (dmx_handle_t)&dmx_sub_ctr[dmx_id];
}

/**
 * @brief      获取dmx子系统中的device句柄
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄
 * 
 * @return      dmx子系统所对应的device句柄 -1代表发生了错误
 */
handle_t  GxSubSystem_DmxGetDevice(dmx_handle_t dmx)
{
    GxSubsystemDmxCtr* dmx_ctr;
    if(dmx == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub get device paramter err dmx hanle is 0!\n");
#endif
        return -1;
    }
    dmx_ctr = (GxSubsystemDmxCtr*)dmx;
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,-1); 
    if(dmx_ctr->count == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub not open!\n");
#endif
        return -1;
    }
    return dmx_ctr->device;
}

/**
 * @brief      获取dmx子系统中的module句柄
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄
 * 
 * @return      dmx子系统所对应的module句柄 -1代表发生了错误
 */
handle_t  GxSubSystem_DmxGetModule(dmx_handle_t dmx)
{
    GxSubsystemDmxCtr* dmx_ctr;
    if(dmx == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub get device paramter err dmx hanle is 0!\n");
#endif
        return -1;
    }
    dmx_ctr = (GxSubsystemDmxCtr*)dmx;
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,-1); 
    if(dmx_ctr->count == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub not open!\n");
#endif
        return -1;
    }
    return dmx_ctr->module;
}

/**
 * @brief      查询ts是否锁定
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄
 * 
 * @return      1：ts锁定
 *              0：ts没有锁定
 *              -1：运行错误
 */
int32_t GxSubSystem_DmxQueryStatus(dmx_handle_t dmx)
{
    GxSubsystemDmxCtr* dmx_ctr;
    int32_t ret = 0;
    if(dmx == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub query status err dmx hanle is 0!\n");
#endif
        return -1;
    }
    dmx_ctr = (GxSubsystemDmxCtr*)dmx;
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,-1); 
    if(dmx_ctr->count == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub not open!\n");
#endif
        return -1;
    }
    ret = gx_dmx_sub_driver_query_status(dmx_ctr);
    return ret;
}
/**
 * @brief      关闭打开的dmx子系统
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄
 * 
 * @return     GX_DMX_SUB_ERR:执行错误
 *             GX_DMX_SUB_SUCCESS：执行成功  
 */
int32_t  GxSubSystem_DmxClose(dmx_handle_t dmx)
{
    GxSubsystemDmxCtr* dmx_ctr = NULL;
    int32_t ret = 0;
    uint32_t i = 0;
    if(dmx == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub get device paramter err dmx hanle is 0!\n");
#endif
        return GX_DMX_SUB_ERR;
    }
    dmx_ctr = (GxSubsystemDmxCtr*)dmx;
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_ERR); 
    if(dmx_ctr->count == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub not open!\n");
#endif
        return GX_DMX_SUB_ERR;
    }
    GxCore_MutexLock(dmx_ctr->mutex);
    /*打开次数减一*/
    dmx_ctr->count--;
    for(i=0; i<DMX_SUB_SLOT_NUM; i++)//检测slot有没有被释放完
    {
        if(dmx_ctr->slot[i].count != 0)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            //DMX_SUB_ERR_PRINTF("some slots are used in the dmx!\n");
#endif  
            GxCore_MutexUnlock(dmx_ctr->mutex);
            return GX_DMX_SUB_SUCCESS;
        }
    }
    if(dmx_ctr->count == 0)//所有打开者都关闭了
    {
        ret = gx_dmx_sub_driver_close_module(dmx_ctr->device,dmx_ctr->module);
        if(ret < 0)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("dmx sub close module err!\n");
#endif
            GxCore_MutexUnlock(dmx_ctr->mutex);
            return GX_DMX_SUB_ERR;
        }
        dmx_ctr->module = -1;
        //松绑demux和ts
        gx_dmx_sub_driver_free_demux(dmx_ctr->dmx_id);
        
        ret = gx_dmx_sub_driver_close_device(dmx_ctr->device);
        if(ret < 0)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("dmx sub close device err!\n");
#endif
            GxCore_MutexUnlock(dmx_ctr->mutex);
            return GX_DMX_SUB_ERR;
        }
        dmx_ctr->device = -1;
    }
	GxCore_MutexUnlock(dmx_ctr->mutex);
    return GX_DMX_SUB_SUCCESS;
}

/**
 * @brief      申请过滤
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄
 *              GxSubsystemDmxAllocFilter* para：过滤参数
 * 
 * @return     -1:执行错误
 *              其他值：过滤的控制句柄
 *             
 */
filter_handle_t GxSubSystem_DmxFilterAlloc(dmx_handle_t dmx, GxSubsystemDmxAllocFilter* para)
{
    GxSubsystemDmxCtr* dmx_ctr = NULL;
    filter_handle_t filter = 0;
    slot_handle_t  slot = 0;
    if(dmx == 0 || para == NULL)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub filter alloc paramter err !!\n");
#endif
        return -1;
    }

    dmx_ctr = (GxSubsystemDmxCtr*)dmx;
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,0); 
   
    GxCore_MutexLock(dmx_ctr->mutex);
    slot = gx_dmx_sub_check_pid(dmx_ctr,para->pid);

    if(slot == 0xffffffff)//另外一路dmx分配了该pid的slot
    {
        GxCore_MutexUnlock(dmx_ctr->mutex);
        return -1;
    }
    else if(slot != 0 )
    {
       filter = gx_dmx_sub_alloc_filter(slot,para);//存在相同pid的slot，只要分配filer
    }
    else
    {
        filter = gx_dmx_sub_alloc_slot_filter(dmx_ctr,para);//不存在相同pid的slot，那么需要跑分配一个slot和一个filter
    }

    if(filter == -1)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub alloc  filte err !!\n");
#endif  
    }

    GxCore_MutexUnlock(dmx_ctr->mutex);
    return filter;
}

/**
 * @brief      开始过滤
 * 
 * @param      filt_handle_t filter:filter的句柄
 * 
 * @return     GX_DMX_SUB_ERR: 执行错误
 *             GX_DMX_SUB_PARAMETER_ERR：参数错误
 *             GX_DMX_SUB_SUCCESS：执行成功
 */
int32_t  GxSubSystem_DmxFilterStart(filter_handle_t filter)
{
    GxSubsystemFilterCtr* filter_ctr = (GxSubsystemFilterCtr*)filter ;
    GxSubsystemDmxCtr* dmx_ctr = (GxSubsystemDmxCtr*)(filter_ctr->dmx);
    int32_t ret = 0;

    if(filter == -1)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filt  err  filter is 0!!\n");
#endif  
        return GX_DMX_SUB_PARAMETER_ERR;
    }
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_PARAMETER_ERR); 
    GxCore_MutexLock(dmx_ctr->mutex);

    ret = gx_dmx_sub_driver_start_filter(filter_ctr);
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filter  err!!\n");
#endif  
        GxCore_MutexUnlock(dmx_ctr->mutex);
        return GX_DMX_SUB_ERR;
    }
    GxCore_MutexUnlock(dmx_ctr->mutex);
    return GX_DMX_SUB_SUCCESS;
}

/**
 * @brief      停止过滤
 * 
 * @param      filt_handle_t filter:filter的句柄
 * 
 * @return     GX_DMX_SUB_ERR: 执行错误
 *             GX_DMX_SUB_PARAMETER_ERR：参数错误
 *             GX_DMX_SUB_SUCCESS：执行成功
 */
int32_t  GxSubSystem_DmxFilterStop(filter_handle_t filter)
{
    GxSubsystemFilterCtr* filter_ctr = (GxSubsystemFilterCtr*)filter ;
    GxSubsystemDmxCtr* dmx_ctr = (GxSubsystemDmxCtr*)(filter_ctr->dmx);
    int32_t ret = 0;

    if(filter == -1)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filt  err  filter is 0!!\n");
#endif  
        return GX_DMX_SUB_PARAMETER_ERR;
    }
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_PARAMETER_ERR); 
    GxCore_MutexLock(dmx_ctr->mutex);

    ret = gx_dmx_sub_driver_stop_filter(filter_ctr);
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filter  err!!\n");
#endif  
        GxCore_MutexUnlock(dmx_ctr->mutex);
        return GX_DMX_SUB_ERR;
    }
    GxCore_MutexUnlock(dmx_ctr->mutex);
    return GX_DMX_SUB_SUCCESS;
}

/**
 * @brief      释放过滤
 * 
 * @param      filt_handle_t filter:filter的句柄
 * 
 * @return     GX_DMX_SUB_ERR: 执行错误
 *             GX_DMX_SUB_PARAMETER_ERR：参数错误
 *             GX_DMX_SUB_SUCCESS：执行成功
 */
int32_t  GxSubSystem_DmxFilterRelease(filter_handle_t filter)
{
    GxSubsystemFilterCtr* filter_ctr = (GxSubsystemFilterCtr*)filter ;
    GxSubsystemDmxCtr* dmx_ctr = (GxSubsystemDmxCtr*)(filter_ctr->dmx);
    GxSubsystemSlotCtr* slot_ctr = (GxSubsystemSlotCtr*)(filter_ctr->slot);    
    int32_t ret = 0;

    if(filter == -1)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub start filt  err  filter is 0!!\n");
#endif  
        return GX_DMX_SUB_PARAMETER_ERR;
    }
    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_ERR); 
    GxCore_MutexLock(dmx_ctr->mutex);

    ret = gx_dmx_sub_driver_release_filter(filter_ctr);
    if(ret < 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub release filter  err!!\n");
#endif  
        GxCore_MutexUnlock(dmx_ctr->mutex);
        return GX_DMX_SUB_ERR;
    }
    slot_ctr->count--;//打开的slot次数减一 如果为零代表挂在这个slot上的filter已经全部释放，slot也可以被释放了
    if(slot_ctr->count == 0)
    {
        ret = gx_dmx_sub_driver_release_slot(slot_ctr);
        if(ret < 0)
        {
#ifdef GX_DMX_SUB_ERR_DBUG
            DMX_SUB_ERR_PRINTF("dmx sub release slot err!!\n");
#endif  
            GxCore_MutexUnlock(dmx_ctr->mutex);
            return GX_DMX_SUB_ERR;
        }
    }
    GxCore_MutexUnlock(dmx_ctr->mutex);
    return GX_DMX_SUB_SUCCESS;
}

/**
 * @brief      等待过滤数据,当正确申请了filter后该接口在等待过程中会引起线程休
 * 眠
 * 
 * @param       dmx_handle_t dmx:dmx子系统的句柄 
 *              filt_handle_t* filter:返回有数据的filter的句柄，空间由应用开，
 *              为了安全空间大小最好等于
 *              sizeof(filter_handle_t)*DMX_SUB_FILTER_NUM
 * 
 * @return     0: 没有任何filter过滤导数据
 *              1~64：有数据的filter数量
 *              GX_DMX_SUB_ERR: 执行错误
 */
int32_t  GxSubSystem_DmxFilterWait(dmx_handle_t dmx,filter_handle_t* filter)
{
    uint64_t status = 0;
    uint32_t count = 0;
    uint32_t i = 0;
    uint32_t y = 0;
    GxSubsystemDmxCtr* dmx_ctr = (GxSubsystemDmxCtr*)dmx;

    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_ERR); 
    if(filter == NULL||dmx == -1)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub wait err filter is NULL or dmx is 0!!\n");
#endif  
        return GX_DMX_SUB_ERR;
    }

    status = gx_dmx_sub_driver_wait(dmx_ctr);
    if(status == 0)
    {
        return 0;
    }
    for(i=0; i<DMX_SUB_FILTER_NUM; i++)
    {
        if (((status>>i)&1ULL) == 1ULL)
        {
            for(y=0; y<DMX_SUB_SLOT_NUM; y++)//判断该filter属于哪个slot，filter id是唯一分配的,即id为1的fliter只可能分配给一个slot
            {
               if(dmx_ctr->slot[y].count != 0 && 
                       dmx_ctr->slot[y].filter[i].dmx != 0 &&
                       dmx_ctr->slot[y].filter[i].slot != 0)
               {
                   filter[count] = (filter_handle_t)&(dmx_ctr->slot[y].filter[i]);
                   count++;
               }
            }
        }
    }
    return count;
}

/**
 * @brief      读取过滤到的数据，是连续的n个section数据
 * 
 * @param      filt_handle_t filter:filter的句柄
 *              data_out:存放读取到得数据的buf
 *              buf_size：data_out的大小
 * 
 * @return     实际读取的数据量
 *             GX_DMX_SUB_ERR: 执行错误
 */
int32_t  GxSubSystem_DmxFilterRead(filter_handle_t filter,uint8_t* data_out, uint32_t buf_size)
{
    GxSubsystemFilterCtr* filter_ctr = NULL;
    GxSubsystemDmxCtr* dmx_ctr = NULL;
    int32_t size = 0;

    if(filter == -1 || data_out == NULL || buf_size == 0)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub read filter  err  filter is 0 or data_out is null or buf_size is 0!!\n");
#endif  
        return GX_DMX_SUB_ERR;
    }
    
	filter_ctr = (GxSubsystemFilterCtr*)filter ;
    dmx_ctr = (GxSubsystemDmxCtr*)(filter_ctr->dmx);
    
	GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,GX_DMX_SUB_ERR); 

    size = gx_dmx_sub_driver_read(filter_ctr,data_out,buf_size);

    return size;
}
/**
 * @brief      改变过滤 条件，之前必须先stop
 * 
 * @param      filt_handle_t filter:filter的句柄
 *              GxSubsystemDmxAllocFilter* para：过滤参数
 * 
 * @return     GX_DMX_SUB_ERR: 执行错误
 *             GX_DMX_SUB_PARAMETER_ERR：参数错误
 *             GX_DMX_SUB_SUCCESS：执行成功
 *             
 */
int32_t GxSubSystem_DmxFilterModify(filter_handle_t filter, GxSubsystemDmxAllocFilter* para)
{
    GxSubsystemFilterCtr* filter_ctr = NULL ;
    GxSubsystemDmxCtr* dmx_ctr = NULL;
	int32_t ret = 0;
    
	if(filter == -1 || para == NULL)
    {
#ifdef GX_DMX_SUB_ERR_DBUG
        DMX_SUB_ERR_PRINTF("dmx sub filter modify paramter err !!\n");
#endif
        return GX_DMX_SUB_ERR;
    }
    filter_ctr = (GxSubsystemFilterCtr*)filter ;
    dmx_ctr = (GxSubsystemDmxCtr*)(filter_ctr->dmx);

    GX_DMX_SUB_CHECK_DMX_HANDLE(dmx_ctr,0); 
   
    GxCore_MutexLock(dmx_ctr->mutex);
    
	ret =  gx_dmx_sub_driver_modify_filter(filter_ctr,para);
	if(ret < 0)
	{
#ifdef GX_DMX_SUB_ERR_DBUG
		DMX_SUB_ERR_PRINTF("dmx sub modify  err!!\n");
#endif  
		GxCore_MutexUnlock(dmx_ctr->mutex);
		return GX_DMX_SUB_ERR;
	}
	GxCore_MutexUnlock(dmx_ctr->mutex);

	return GX_DMX_SUB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

