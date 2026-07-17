#include <string.h>
#include "gxavdev.h"
#include "module/app_demux_api.h"
#include "gxcore.h"
#include "module/app_log.h"

#define CA_FILTER_REPEAT_FLAG       (1)
#define CA_FILTER_ONCE_FLAG         (2)
#define SLOT_MAGIC_NUM              (0x88156088)
#define FILTER_MAGIC_NUM            (0x88156083)

#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE        (0)
#endif
#ifndef E_INVALID_PARAM
#define E_INVALID_PARAM         (-2)
#endif

/*描述pid通道的结构体pid通道,又被称为slot*/
struct gxca_channel {
    uint32_t                magic; /*分配的魔数,用于判断句柄的有效性放在第一个方便检测*/
    int32_t                 demux; /*芯片的第几路demux*/
    int32_t                 slot_id; /*申请到的slot id*/
    int32_t                 repeat_mode; /*通道的过滤模式.repeat,section*/
    int32_t                 pid; /*通道过滤的匹配pid*/
};

/*描述过滤器的结构体*/
struct gxca_filter {
    uint32_t                magic;/*分配是写的魔数,用于判断句柄的有效性放在第一个方便检测*/
    struct gxca_channel*    p_channel;/*此过滤器捆绑的slot的句柄*/
    int32_t                 filter_id;/*申请得到的filter的id*/
};

/*描述demux整体的结构体*/
struct gx_demux {
    int32_t                 dev;/*设备句柄*/
    int32_t                 demux0;/*打开的demux句柄*/
    int32_t                 demux1;
#define GXCA_DEMUX_STOP             0
#define GXCA_DEMUX_ENABLE           1
#define GXCA_DEMUX_DISABLE          2
    handle_t                lock;/*模块用到的互斥锁句柄*/

    uint32_t                channel_num;/*模块初始化是设置的slot个数,
                                    注意跟硬件提供的个数无关*/
    struct gxca_channel*    channel_list;/*描述channel的结构体链表*/

    uint32_t                filter_num; /*模块初始化时设置的filter个数*/
    struct gxca_filter*     filter_list;/*描述filter的结构体链表*/
};

#define DMX_ERR(fmt, args...)  do {                                                  \
                                    app_log_error("\033[31m");                              \
                                    app_log_error("[DMX_ERR][%s:%d]: ", __func__, __LINE__);\
                                    app_log_error(fmt, ##args);                             \
                                    app_log_error("\033[0m\n");                             \
                                } while(0)

#define DMX_DBG(fmt, args...)  do {                                 \
    if (s_DmxDebug) {                                         \
        app_log_debug("\033[33m");                                         \
        app_log_debug("[DMX_DBG][%s:%d]: ", __func__, __LINE__);           \
        app_log_debug(fmt, ##args);                                        \
        app_log_debug("\033[0m\n");                                        \
    }                                                               \
} while(0)

#define DMX_DUMP(len,p,fmt) \
	do {	\
		if (s_DmxDebug){		\
			int i__;\
			app_log_debug("%s():%d: ", __func__, __LINE__);		\
			app_log_debug("len = %d>>>",len);\
			for(i__=0;i__<len;i__++){\
				app_log_debug(fmt",",p[i__]);\
				if (i__&&i__%20 == 0)app_log_debug("\n");\
			}\
			app_log_debug("\n");\
		}\
	}while (0)

#define DMX_CHECK_SLOT_H(handle)       do{\
                                            if(*(uint32_t*)handle != SLOT_MAGIC_NUM)\
                                            {\
                                                DMX_ERR("This is a wrong hanle!!!");\
                                                return E_INVALID_PARAM;\
                                            }\
                                        }while(0)

#define DMX_CHECK_FILTER_H(handle)     do{\
                                            if(*(uint32_t*)handle != FILTER_MAGIC_NUM)\
                                            {\
                                                DMX_ERR("This is a wrong hanle!!!");\
                                                return E_INVALID_PARAM;\
                                            }\
                                        }while(0)

#define DMX_CHECK_P(p, r) if(NULL == p){                                            \
                            app_log_debug("\033[033m");                                    \
                            app_log_debug("\n[%s: %d] %s\n", __FILE__, __LINE__, __func__);\
                            app_log_debug("%s is NULL\n", #p);                             \
                            app_log_debug("\033[0m");                                      \
                            return r;}

static struct gx_demux _dmx;
bool s_DmxDebug = false;

/*内部函数,用于分配slot控制块*/
static struct gxca_channel* alloc_channel_block(uint32_t Demux,uint16_t pid)
{
    int32_t     i;

    if (_dmx.channel_list == NULL)
    {
        DMX_ERR("U try to allco a channel before demux init\n");
        return NULL;
    }

    for (i = 0; i < _dmx.channel_num; i++)
    {
        if (_dmx.channel_list[i].pid == -1)
        {
            _dmx.channel_list[i].magic = SLOT_MAGIC_NUM;
            return &_dmx.channel_list[i];
        }
    }
    return NULL;
}

/*内部函数,用于分配fliter控制块*/
static struct gxca_filter* alloc_filter_block(void)
{
    int32_t     i;

    if (_dmx.filter_list == NULL)
    {
        DMX_ERR("U try to allco a filter before demux init\n");
        return NULL;
    }

    for (i = 0; i < _dmx.filter_num; i++)
    {
        if (_dmx.filter_list[i].filter_id == -1)
        {
            _dmx.filter_list[i].magic = FILTER_MAGIC_NUM;
            return &_dmx.filter_list[i];
        }
    }
    return NULL;
}


/**
* @brief    init Demux
* @param [] void
* @return   int32_t  >= 0, sucess.<0 failure
* @note     it support max 64 channel & filter. It can be called only once;
*/
int32_t GxDemux_Init(void)
{
#define MAX_FLITER_NUM      (64)
    int32_t         i;
    if(_dmx.channel_num>0 || _dmx.filter_num >0)
    {
        DMX_ERR("Demux will be initialed twice, may be lose some hardware resource!!! ");
        return -1;
    }

    memset(&_dmx,0,sizeof(_dmx));
    _dmx.channel_list = GxCore_Calloc(MAX_FLITER_NUM, sizeof(struct gxca_channel));
    if(_dmx.channel_list == NULL)
    {
        DMX_ERR( "GxCore_Malloc failure\n");
        return -1;
    }

    _dmx.filter_list  = GxCore_Calloc(MAX_FLITER_NUM, sizeof(struct gxca_filter));
    if(_dmx.filter_list == NULL)
    {
        DMX_ERR( "GxCore_Malloc failure\n");
        GxCore_Free(_dmx.channel_list);
        return -1;
    }

    _dmx.channel_num  = MAX_FLITER_NUM;
    _dmx.filter_num   = MAX_FLITER_NUM;

    for (i = 0; i < _dmx.channel_num; i++)
    {
        _dmx.channel_list[i].magic = SLOT_MAGIC_NUM;
        _dmx.channel_list[i].pid = -1;
    }

    for (i = 0; i < _dmx.filter_num; i++)
    {
        _dmx.filter_list[i].magic = FILTER_MAGIC_NUM;
        _dmx.filter_list[i].filter_id = -1;
    }

    _dmx.dev       = GxAvdev_CreateDevice(0);
    _dmx.demux0    = GxAvdev_OpenModule(_dmx.dev,GXAV_MOD_DEMUX, 0);
    _dmx.demux1    = GxAvdev_OpenModule(_dmx.dev,GXAV_MOD_DEMUX, 1);

    if(GxCore_MutexCreate(&_dmx.lock) < 0)
    {
        GxCore_Free(_dmx.channel_list);
        GxCore_Free(_dmx.filter_list);
        GxAvdev_CloseModule(_dmx.dev, _dmx.demux0);
        GxAvdev_CloseModule(_dmx.dev, _dmx.demux1);
        GxAvdev_DestroyDevice(_dmx.dev);
        DMX_ERR( "Create mutex failuer!\n");
        return -1;
    }
    return 0;
}

/**
* @brief    Destroy Demux
* @param [] void
* @return   int32_t  >= 0, sucess.<0 failure
* @note     must be called after GxDemux_Init()
*/
int32_t GxDemux_Destroy(void)
{
    int32_t i;
    if(_dmx.channel_num == 0|| _dmx.filter_num == 0)
    {
        DMX_ERR("U try to destroy demux before init!!! ");
        return -1;
    }
    for (i = 0; i < _dmx.channel_num; i++)
    {
        if( _dmx.channel_list[i].pid != -1 )
        {
            GxDemux_ChannelFree((handle_t)(_dmx.channel_list + i));
        }
    }
    GxCore_Free(_dmx.channel_list);
    GxCore_Free(_dmx.filter_list);
    _dmx.channel_num  = 0;
    _dmx.filter_num   = 0;
    GxCore_MutexDelete(_dmx.lock);
    GxAvdev_CloseModule(_dmx.dev, _dmx.demux0);
    GxAvdev_CloseModule(_dmx.dev, _dmx.demux1);
    GxAvdev_DestroyDevice(_dmx.dev);
    return 0;

}

/**
* @brief    allocate slot(pid channel)
* @param [] DemuxID :demux id .0~1 is valid
* @param [] PID :the pid value that need channel,it is used to find
*               if have channel used this pid!You must to set pid after
*               alloc channel.
* @return   handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDemux_ChannelAllocate(uint32_t DemuxID,uint16_t PID)
{
    int32_t     ret,_Demux=0;
    struct gxca_channel*    p_channel;
    GxDemuxProperty_Slot    slot;

    if(_dmx.channel_num == 0|| _dmx.filter_num == 0)
    {
        DMX_ERR("U try to destroy demux before init!!! ");
        return -1;
    }
    memset(&slot,0,sizeof(slot));

    if (DemuxID == 0)
    {
        _Demux = _dmx.demux0;
    }
    else if (DemuxID == 1)
    {
        _Demux = _dmx.demux1;
    }

    GxCore_MutexLock(_dmx.lock);
    p_channel = alloc_channel_block(_Demux,PID);
    if (p_channel == NULL)
    {
        DMX_ERR( "There is no free slot!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return E_INVALID_HANDLE;
    }
    /*if the pid have allocate a channel*/
    if (p_channel->pid == -1)
    {
        if (DemuxID == 0)
        {
            p_channel->demux = _dmx.demux0;
        }
        else if (DemuxID == 1)
        {
            p_channel->demux = _dmx.demux1;
        }

        slot.type = DEMUX_SLOT_PSI;
        slot.pid  = PID;
        ret = GxAVGetProperty(_dmx.dev, p_channel->demux,
                GxDemuxPropertyID_SlotAlloc,
                &slot, sizeof(GxDemuxProperty_Slot));
        if(ret != 0)
        {
            DMX_ERR( "Hardware  alloc slot failure!\n");
            GxCore_MutexUnlock(_dmx.lock);
            return E_INVALID_HANDLE;

        }

        slot.type       = DEMUX_SLOT_PSI;
        p_channel->repeat_mode = CA_FILTER_REPEAT_FLAG;
        slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
        ret = GxAVSetProperty(_dmx.dev, p_channel->demux,
                GxDemuxPropertyID_SlotConfig,
                &slot, sizeof(GxDemuxProperty_Slot));
        if (ret != 0)
        {
            DMX_ERR( "Config slot failure!\n");
            GxCore_MutexUnlock(_dmx.lock);
            return -1;
        }

        p_channel->slot_id = slot.slot_id;
        p_channel->pid = PID;
        DMX_DBG("Allocate a new solt_id: %d, pid=%d\n", p_channel->slot_id, PID);
    }
    else
    {
        DMX_DBG("Using a allocated solt_id: %d, pid=%d\n", p_channel->slot_id, PID);
    }
    GxCore_MutexUnlock(_dmx.lock);
    DMX_DBG("The allocated channel=%p\n", p_channel);
    return (handle_t)p_channel;
}


/**
* @brief    release a slot(pid channel)
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @return   int32_t  >= 0, sucess.<0 failure
* @note     function will release all filter under the channel;
*/
int32_t GxDemux_ChannelFree(handle_t Channel)
{
    int32_t     ret;
    GxDemuxProperty_Slot    slot;
    struct gxca_channel*    p_channel = (struct gxca_channel*)Channel;


    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);
    memset(&slot,0,sizeof(slot));
    DMX_DBG("Free a channel=%p\n", p_channel);

    GxCore_MutexLock(_dmx.lock);
    if (p_channel->pid == -1)
    {
        DMX_DBG("Try to operate a free channel\n");
    }

    slot.slot_id = p_channel->slot_id;
    GxAVSetProperty(_dmx.dev, p_channel->demux,
                        GxDemuxPropertyID_SlotDisable,
                        &slot, sizeof(GxDemuxProperty_Slot));

    if (p_channel->slot_id >= 0)
    {
        slot.slot_id = p_channel->slot_id;
        ret = GxAVSetProperty(_dmx.dev, p_channel->demux,
                                GxDemuxPropertyID_SlotFree,
                                &slot, sizeof(GxDemuxProperty_Slot));
        if(ret != 0)
        {
            DMX_ERR( "Free slot failure!\n");
            GxCore_MutexUnlock(_dmx.lock);
            return -1;
        }
        p_channel->slot_id = -1;
    }
    p_channel->repeat_mode = -1;
    p_channel->pid = -1;
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    set pid
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @param [] pid :value of pid;
* @param [] RepeatFlag :if filter data non stop;0,if get one section, channel will stop and wait to be restart
* @return   int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_ChannelSetPID(handle_t Channel,  uint16_t pid,bool RepeatFlag)
{
    int32_t                 ret;

    struct gxca_channel*    p_channel = (struct gxca_channel*)Channel;
    GxDemuxProperty_Slot    slot;


    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);

    DMX_DBG("Set channel=%p pid=%d\n", p_channel, pid);
    memset(&slot,0,sizeof(slot));
    GxCore_MutexLock(_dmx.lock);
    if (p_channel->pid == -1)
    {
        DMX_ERR("Try to operate a free channel\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    slot.slot_id    = p_channel->slot_id;
    slot.pid        = pid;
    slot.type       = DEMUX_SLOT_PSI;
    if (TRUE == RepeatFlag)
    {
        p_channel->repeat_mode = CA_FILTER_REPEAT_FLAG;
        slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
    }
    else
    {
        p_channel->repeat_mode = CA_FILTER_ONCE_FLAG;
        slot.flags      = (DMX_AVOUT_EN);
    }
    ret = GxAVSetProperty(_dmx.dev, p_channel->demux,
                        GxDemuxPropertyID_SlotConfig,
                        &slot, sizeof(GxDemuxProperty_Slot));
    if (ret != 0)
    {
        DMX_ERR( "Config slot failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    p_channel->pid = pid;
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    enable channel
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_ChannelEnable(handle_t Channel)
{
    int32_t                ret;
    struct gxca_channel*   p_channel = (struct gxca_channel*)Channel;
    GxDemuxProperty_Slot   slot;


    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);

    DMX_DBG("Enable a channel=%p\n", p_channel);
    memset(&slot,0,sizeof(slot));
    GxCore_MutexLock(_dmx.lock);
    if (p_channel->pid == -1)
    {
        DMX_ERR("Try to operate a free channel\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    slot.slot_id = p_channel->slot_id;
    ret = GxAVSetProperty(_dmx.dev, p_channel->demux,
                        GxDemuxPropertyID_SlotEnable,
                        &slot, sizeof(GxDemuxProperty_Slot));
    if (ret != 0)
    {
        DMX_ERR( "Enable slot failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    disable channel
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @return   int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_ChannelDisable(handle_t  Channel)
{
    int32_t                 ret;
    GxDemuxProperty_Slot    slot;
    struct gxca_channel*    p_channel = (struct gxca_channel*)Channel;



    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);
    memset(&slot,0,sizeof(slot));
    DMX_DBG("Disable a channel=%p\n", p_channel);
    GxCore_MutexLock(_dmx.lock);
    if (p_channel->pid == -1)
    {
        DMX_DBG("Try to operate a free channel\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    slot.slot_id = p_channel->slot_id;
    ret = GxAVSetProperty(_dmx.dev, p_channel->demux,
                        GxDemuxPropertyID_SlotDisable,
                        &slot, sizeof(GxDemuxProperty_Slot));
    if (ret != 0)
    {
        DMX_ERR( "Disable slot failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}
/**
* @brief    get channel that bind by the pid
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @return      handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDemux_ChannelGetByPid(uint32_t DemuxID ,uint16_t pid)
{
    int32_t         i;
    uint32_t _Demux = 0;

    DMX_DBG("Get a channel from pid=%d\n", pid);
    GxCore_MutexLock(_dmx.lock);

    if (DemuxID == 0)
    {
        _Demux = _dmx.demux0;
    }
    else if (DemuxID == 1)
    {
        _Demux = _dmx.demux1;
    }
    for (i = 0; i < _dmx.channel_num; i++)
    {
        if (_dmx.channel_list[i].pid == pid &&_dmx.channel_list[i].demux == _Demux)
        {
            GxCore_MutexUnlock(_dmx.lock);
            DMX_DBG("Get channel=%p, pid=%d Successed\n", _dmx.channel_list + i,pid);
            return (handle_t)(_dmx.channel_list + i);
        }
    }
    GxCore_MutexUnlock(_dmx.lock);
    return E_INVALID_HANDLE;
}

/**
* @brief    allocate a filter and tie up with the channel
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @return      handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDemux_FilterAllocate(handle_t Channel)
{
    int32_t                     ret;
    struct gxca_filter*         p_filter;
    GxDemuxProperty_Filter      param;
    struct gxca_channel*        p_channel = (struct gxca_channel*)Channel;

    if(_dmx.channel_num == 0|| _dmx.filter_num == 0)
    {
        DMX_ERR("U try to destroy demux before init!!! ");
        return -1;
    }

    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);
    memset(&param,0,sizeof(param));
    GxCore_MutexLock(_dmx.lock);
    p_filter = alloc_filter_block();
    if (p_filter == NULL)
    {
        DMX_ERR( "There is no free filter!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return E_INVALID_HANDLE;
    }

    p_filter->p_channel  = p_channel;
    param.slot_id = p_channel->slot_id;
    ret = GxAVGetProperty(_dmx.dev, p_channel->demux,
                            GxDemuxPropertyID_FilterAlloc,
                            &param, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
    {
        DMX_ERR( "Hardware  alloc slot failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return E_INVALID_HANDLE;
    }

    p_filter->filter_id = param.filter_id;
    GxCore_MutexUnlock(_dmx.lock);
    DMX_DBG("Allocate a filter: filter=%p, channel=%p, filter_id=%d\n",
                            p_filter, p_channel, p_filter->filter_id);

    return (handle_t)p_filter;
}

/**
* @brief    allocate a filter and tie up with the channel
* @param [] Channel :handle of channel, get via "GxDemux_ChannelAllocate()";
* @param [] sw_buffer_size: the software buffer size of filter, if size < mini sw buffer size, will use mini sw buffer size
* @return      handle == E_INVALID_HANDLE failure;or sucess
*/
handle_t GxDemux_FilterAllocateSize(handle_t Channel, uint32_t sw_buffer_size)
{
    int32_t                     ret;
    struct gxca_filter*         p_filter;
    GxDemuxProperty_Filter      param;
    struct gxca_channel*        p_channel = (struct gxca_channel*)Channel;

    if(_dmx.channel_num == 0|| _dmx.filter_num == 0)
    {
        DMX_ERR("U try to destroy demux before init!!! ");
        return -1;
    }

    DMX_CHECK_P(p_channel, -1);
    DMX_CHECK_SLOT_H(Channel);
    memset(&param,0,sizeof(param));
    GxCore_MutexLock(_dmx.lock);
    p_filter = alloc_filter_block();
    if (p_filter == NULL)
    {
        DMX_ERR( "There is no free filter!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return E_INVALID_HANDLE;
    }

    p_filter->p_channel  = p_channel;
    param.slot_id = p_channel->slot_id;
    param.sw_buffer_size = sw_buffer_size;
    ret = GxAVGetProperty(_dmx.dev, p_channel->demux,
                            GxDemuxPropertyID_FilterAlloc,
                            &param, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
    {
        DMX_ERR( "Hardware  alloc slot failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return E_INVALID_HANDLE;
    }

    p_filter->filter_id = param.filter_id;
    GxCore_MutexUnlock(_dmx.lock);
    DMX_DBG("Allocate a filter: filter=%p, channel=%p, filter_id=%d\n",
                            p_filter, p_channel, p_filter->filter_id);

    return (handle_t)p_filter;
}

/**
* @brief    get channel's pid via handle of filter
* @param [] Filter :handle of filter, get via "GxDemux_FilterAllocate()";
* @param [out] pid :the value of pid
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_FilterGetPID(handle_t Filter, int16_t *pid)
{
    struct gxca_filter*     filter = (struct gxca_filter*)Filter;
    DMX_CHECK_P(filter, -1);
    GxCore_MutexLock(_dmx.lock);
    if (filter->filter_id == -1)
    {
        DMX_ERR("Get a freed filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    *pid = filter->p_channel->pid;
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}


/**
* @brief    release a filter
* @param [] Filter :handle of filter, get via "GxDemux_FilterAllocate()";
* @return   int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_FilterFree(handle_t  Filter)
{
    int32_t                 ret;
    GxDemuxProperty_Filter  param = {0};
    struct gxca_filter*     filter = (struct gxca_filter*)Filter;


    DMX_CHECK_P(filter, -1);
    DMX_CHECK_FILTER_H(Filter);
    DMX_DBG("Free a filter=%p\n", filter);

    GxCore_MutexLock(_dmx.lock);
    if (filter->filter_id == -1)
    {
        DMX_ERR("Try to operate a free filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    param.filter_id = filter->filter_id;
    GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                            GxDemuxPropertyID_FilterDisable,
                            &param, sizeof(GxDemuxProperty_Filter));

    param.slot_id = filter->p_channel->slot_id;
    param.filter_id = filter->filter_id;
    ret = GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                            GxDemuxPropertyID_FilterFree,
                            (void*)&param, sizeof(GxDemuxProperty_Filter));
    if (ret != 0)
    {
        DMX_ERR( "Free filter failure!\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    filter->filter_id = -1;
    filter->p_channel = 0;
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    set up  a filter
* @param [] Filter :handle of filter, get via "GxDemux_FilterAllocate()";
* @param [in] Match :value need to be match when filter running
* @param [in] Mask : mask value
* @param [] Depth : number of match, filter depth
* @param [] Equal : if false, just when the version number is not same as (match[5]&0x3e)>>1 & mask[5],
                  filter can get data. it will not affect to other byte of match.
* @param [] CRCFlag : if open crc verify
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_FilterSetup(handle_t              Filter,
                           const uint8_t*        Match,
                           const uint8_t*        Mask,
                           bool                  Equal,
                           bool                  CRCFlag,
                           bool                  SWFilter,
                           size_t                Depth)
{
    int32_t ret;
    uint32_t    i;
    struct gxca_filter*             filter = (struct gxca_filter*)Filter;
    GxDemuxProperty_Filter          param;


    DMX_CHECK_P(filter, -1);
    DMX_CHECK_FILTER_H(Filter);
    memset(&param,0,sizeof(param));
    DMX_DBG( "Setup filter=%p\n", filter);
    GxCore_MutexLock(_dmx.lock);

    if (filter->filter_id == -1)
    {
        DMX_ERR( "Try to operate a free filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    param.slot_id      = filter->p_channel->slot_id;
    param.filter_id    = filter->filter_id;

    DMX_DBG("Match:");
    DMX_DUMP(Depth,Match, "%x");
    DMX_DBG("Mask:");
    DMX_DUMP(Depth,Mask, "%x");

    for (i = 0; i < Depth; i++)
    {
        param.key[i].value = Match[i];
        param.key[i].mask  = Mask[i];
    }
    param.depth = Depth;
    if (Equal)
    {
        param.flags = DMX_EQ;
    }
    else
    {
        param.flags = 0;
    }
    if (CRCFlag)
    {
        param.flags |= DMX_CRC_IRQ;
    }
    if(SWFilter)
    {
        param.flags |= DMX_SW_FILTER;
    }
    ret = GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                            GxDemuxPropertyID_FilterConfig,
                            &param, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
    {
        DMX_ERR("Setup fifter err!");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}


/**
* @brief    enable filter
* @param [] Filter :handle of channel, get via "GxDemux_FilterAllocate()";
* @return      int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_FilterEnable(handle_t Filter)
{
    int32_t                         ret;
    struct gxca_filter*             filter = (struct gxca_filter*)Filter;
    GxDemuxProperty_Filter          param;
    GxDemuxProperty_FilterFifoReset fifo;


    DMX_CHECK_P(filter, -1);
    DMX_CHECK_FILTER_H(Filter);
    memset(&param,0,sizeof(param));
    DMX_DBG("Enable filter=%p\n", filter);
    GxCore_MutexLock(_dmx.lock);
    if (filter->filter_id == -1)
    {
        DMX_ERR("Try to operate a free filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    fifo.filter_id = filter->filter_id;
    ret = GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                            GxDemuxPropertyID_FilterFIFOReset,
                            &fifo, sizeof(GxDemuxProperty_FilterFifoReset));
    if(ret != 0)
    {
        DMX_ERR("Rest filter fifo err!");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }


    param.slot_id      = filter->p_channel->slot_id;
    param.filter_id    = filter->filter_id;
    ret = GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                        GxDemuxPropertyID_FilterEnable,
                        &param, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
    {
        DMX_ERR("Enable filter fifo err!");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    disable filter
* @param [] Filter :handle of channel, get via "GxDemux_FilterAllocate()";
* @return   int32_t  >= 0, sucess.<0 failure
*/
int32_t GxDemux_FilterDisable(handle_t Filter)
{
    int32_t                 ret;
    GxDemuxProperty_Filter  param;
    struct gxca_filter*     filter = (struct gxca_filter*)Filter;


    DMX_CHECK_P(filter, -1);
    DMX_CHECK_FILTER_H(Filter);
    memset(&param,0,sizeof(param));
    DMX_DBG("Diable filter=%p\n", filter);

    GxCore_MutexLock(_dmx.lock);
    if (filter->filter_id == -1)
    {
        DMX_ERR("Try to operate a free filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }

    param.filter_id = filter->filter_id;
    ret = GxAVSetProperty(_dmx.dev, filter->p_channel->demux,
                            GxDemuxPropertyID_FilterDisable,
                            &param, sizeof(GxDemuxProperty_Filter));
    if(ret != 0)
    {
        DMX_ERR("Disable filter fifo err!");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }
    GxCore_MutexUnlock(_dmx.lock);
    return 0;
}

/**
* @brief    query if filter have get data and copy data to buffer. User must call this function
            frequently, and need to judge the time interval betweed two callings according to your requirement
* @param[]  Filter :handle of channel, get via "GxDemux_FilterAllocate()";
* @param[out] Buffer :buffer used to save section data.The data may be more than one section
            :cn
* @param[]  BufferSize: size of the buffer that use to save data,the min size is 1024byte(size of one section).
* @param[out] Datalen: size of data have read.

* @return      int32_t  >= 0, sucess, will return *DataLen.<0 failure
*/
int32_t GxDemux_QueryAndGetData(handle_t Filter,
                                    uint8_t *Buffer,
                                    uint32_t  BufferSize,
                                    uint32_t *Datalen)
{
    GxDemuxProperty_FilterFifoQuery query;
    int32_t ret;
    struct gxca_filter*     filter = (struct gxca_filter*)Filter;
    GxDemuxProperty_FilterRead      filter_read;

    DMX_CHECK_P(Buffer, -1);
    DMX_CHECK_P(Datalen, -1);
    DMX_CHECK_FILTER_H(Filter);
    GxCore_MutexLock(_dmx.lock);
    if (filter->filter_id == -1)
    {
        DMX_ERR("Try to operate a free filter\n");
        GxCore_MutexUnlock(_dmx.lock);
        return -1;
    }


    ret = GxAVGetProperty(_dmx.dev,filter->p_channel->demux,
            GxDemuxPropertyID_FilterFIFOQuery, &query, sizeof(GxDemuxProperty_FilterFifoQuery));
    if(ret == 0)
    {
        if (query.state == 0)
        {
            *Datalen = 0;
            GxCore_MutexUnlock(_dmx.lock);
            return -1;
        }
        if (((query.state >>filter->filter_id) & 1ULL) == 1ULL)
        {
            filter_read.filter_id = filter->filter_id;
            filter_read.max_size = BufferSize;
            filter_read.buffer = Buffer;
            ret = GxAVGetProperty(_dmx.dev, filter->p_channel->demux, GxDemuxPropertyID_FilterRead, &filter_read, sizeof(GxDemuxProperty_FilterRead));
            if(ret == 0)
            {
                *Datalen = filter_read.read_size;
                GxCore_MutexUnlock(_dmx.lock);
                return *Datalen;
            }
            DMX_ERR("Read data err\n");
            GxCore_MutexUnlock(_dmx.lock);
            return -1;
        }
    }
    GxCore_MutexUnlock(_dmx.lock);
    return -1;
}



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  gx_demux_test
 *  Description:  test demu
 * =====================================================================================
 */

static void CHECK_FILTER_DATA(int count,handle_t filter)
{
    uint8_t * buffer;
    uint32_t readed;
    uint32_t buffersize = 4096;
    buffer = GxCore_Malloc(buffersize);
    if (buffer == NULL)
    {
        DMX_ERR("GxCore_Malloc buffer failure!\n");
        return;
    }

    do
    {
        if (GxDemux_QueryAndGetData(filter,buffer,buffersize,&readed)>=0)
        {
            uint16_t sec_len;
            uint16_t offset = 0;
            DMX_DBG("Have get data!\n");
            DMX_DBG("Data len = %d!\n",readed);
            while(readed > offset)
            {
                DMX_DBG("tid = %d\n",buffer[offset]);
                sec_len = ((buffer[offset+1]<<8)&0xf) + buffer[offset+2];
                offset += sec_len+3;
            }
        }
        GxCore_ThreadDelay(500);
    }while (count -- );
    GxCore_Free(buffer);
}
int32_t gx_demux_test(void)
{
    handle_t shandle1;
    handle_t fhandle1,fhandle2;
    int32_t ret;
    uint8_t mask[18];
    uint8_t match[18];

    memset(mask,0,sizeof(mask));
    memset(match,0,sizeof(match));

    /* init demux module */
    ret = GxDemux_Init();
    if (ret!=0)
        DMX_ERR("Demux init err!\n");
    /* alloc channel,demux 0,pid 0 used to check if the pid have bind one channel, */
    shandle1 = GxDemux_ChannelAllocate(0,0);
    if (shandle1 == E_INVALID_HANDLE)
        DMX_ERR("alloc channel failure!\n");

    /* set pid 0 to channel,set repeat mode */
    ret = GxDemux_ChannelSetPID(shandle1,0,1);
    if( ret != 0 )
        DMX_ERR("channel set pid err!\n");

    /* alloc filter,bind to the channel */
    fhandle1 =GxDemux_FilterAllocate(shandle1);
    if (fhandle1 == E_INVALID_HANDLE)
        DMX_ERR("alloc filter failure!\n");

    /* set filter */
    mask[0] = 0xff;
    match[0] = 0;
    /* check version number not equal */
    match[5] = (0xe<<1);
    mask[5] = 0x3e;
    if (GxDemux_FilterSetup(fhandle1,match,mask,1,0,0,7)<0)
        DMX_ERR("filter set up failure!\n");

    /* open channel  filter */
    GxDemux_ChannelEnable(shandle1);
    GxDemux_FilterEnable(fhandle1);

    DMX_DBG("set channel finish!\n");

    DMX_DBG("now try to get data that have filtered.\n");
    CHECK_FILTER_DATA(5,fhandle1);

    /* test restart filter */
    GxDemux_FilterDisable(fhandle1);
    DMX_DBG("disable filter!\n");
    CHECK_FILTER_DATA(5,fhandle1);

    GxDemux_FilterEnable(fhandle1);
    DMX_DBG("restart filter finish!\n");
    CHECK_FILTER_DATA(5,fhandle1);

    /* test channel disable, enable */
    GxDemux_ChannelDisable(shandle1);
    DMX_DBG("channel disabled1\n");
    CHECK_FILTER_DATA(5,fhandle1);

    GxDemux_ChannelEnable(shandle1);
    DMX_DBG("channel enabled!\n");
    CHECK_FILTER_DATA(5,fhandle1);

    /* test set pid, filter state*/
    GxDemux_ChannelDisable(shandle1);
    GxDemux_ChannelSetPID(shandle1,0x1,1);
    GxDemux_ChannelEnable(shandle1);
    DMX_DBG("change pid finish,demux can't get any data!\n");
    CHECK_FILTER_DATA(5,fhandle1);

    GxDemux_FilterDisable(fhandle1);
    mask[0] = 0xff;
    match[0] = 0x1;
    /* check version number not equal */
    match[5] = 0;
    mask[5] = 0;
    if (GxDemux_FilterSetup(fhandle1,match,mask,1,1,0,1)<0)
        DMX_ERR("filter set up failure!\n");
    GxDemux_FilterEnable(fhandle1);
    DMX_DBG("set filter finish!u can get data from demux!");
    CHECK_FILTER_DATA(50,fhandle1);

    /* test free */
    GxDemux_FilterFree(fhandle1);
    DMX_DBG("free filter,get data will wrong!\n");
    CHECK_FILTER_DATA(10,fhandle1);
    GxDemux_FilterEnable(fhandle1);

    DMX_DBG("alloc new filter!\n");
    fhandle2 = GxDemux_FilterAllocate(shandle1);
    if (fhandle1 == E_INVALID_HANDLE)
        DMX_ERR("alloc filter failure!\n");

    mask[0] = 0xff;
    match[0] = 0x1;
    /* check version number not equal */
    match[5] = 0;
    mask[5] = 0;
    if (GxDemux_FilterSetup(fhandle2,match,mask,1,1,0,1)<0)
        DMX_ERR("filter set up failure!\n");
    GxDemux_FilterEnable(fhandle2);
    DMX_DBG("set filter finish!u can get data from demux!");
    CHECK_FILTER_DATA(50,fhandle2);

    /* test free channel*/
    DMX_DBG("free channel!\n");
    GxDemux_FilterDisable(fhandle2);
    GxDemux_FilterFree(fhandle2);
    GxDemux_ChannelDisable(shandle1);
    GxDemux_ChannelFree(shandle1);

    /* close */
    GxDemux_Destroy();
    DMX_DBG("test finish!\n");

    return 0;

}
