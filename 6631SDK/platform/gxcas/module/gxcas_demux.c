#include "gxcore.h"
#include "gxcas_dbg.h"
#include "autoconf.h"
#include "gxcas_demux.h"

#ifdef CONFIG_DMX_OLD // old method
struct gxca_channel {
	int32_t              id; /*申请到的slot id*/
	int32_t              pid; /*通道过滤的匹配pid*/
	int32_t              demux; /*芯片的第几路demux*/
};

struct gxca_filter {
	int32_t              id;/*申请得到的filter的id*/
	struct gxca_channel *channel;/*此过滤器捆绑的slot的句柄*/
};

#ifndef CONFIG_ABV_PVR
struct gx_demux {
	int32_t              dev;/*设备句柄*/
	int32_t              dmx;/*打开的demux句柄*/
	handle_t             lock;/*模块用到的互斥锁句柄*/
	uint32_t             channel_num;/*模块初始化是设置的slot个数, 注意跟硬件提供的个数无关*/
	struct gxca_channel *channel_list;/*描述channel的结构体链表*/
	uint32_t             filter_num;	/*模块初始化时设置的filter个数*/
	struct gxca_filter  *filter_list;/*描述filter的结构体链表*/
};

static int32_t gxcas_channel_num = 0;
static int32_t gxcas_filter_num = 0;
static struct gx_demux _dmx;

/*内部函数,用于分配slot控制块*/
static struct gxca_channel* alloc_channel(void)
{
	int32_t i = 0;

	for (i = 0; i < _dmx.channel_num; i++) {
		if (_dmx.channel_list[i].id == -1) {
			return &_dmx.channel_list[i];
		}
	}
	return NULL;
}

/*内部函数,用于分配fliter控制块*/
static struct gxca_filter* alloc_filter(void)
{
	int32_t i = 0;

	for (i = 0; i < _dmx.filter_num; i++) {
		if (_dmx.filter_list[i].id == -1) {
			return &_dmx.filter_list[i];
		}
	}
	return NULL;
}

int32_t GxCas_Demux_Init(uint32_t tsid, uint32_t dmx_id)
{
//#define MAX_FILTER_COUNT		(64)
//#define MAX_DEMUX_NUM		(4)
	int32_t         i;
	GxDemuxProperty_ConfigDemux config_demux = {0};

	if (dmx_id > MAX_DEMUX_NUM) {
		CAS_ERR(DEMUX,"Demux id Invalid!!!");
		return -1;
	}

	if(_dmx.channel_num>0 || _dmx.filter_num >0) {
		CAS_ERR(DEMUX,"Demux will be initialed twice, may be lose some hardware resource!!! ");
		return -1;
	}

	memset(&_dmx,0,sizeof(_dmx));
	_dmx.channel_list = GxCore_Calloc(MAX_FILTER_COUNT, sizeof(struct gxca_channel));
	if(_dmx.channel_list == NULL) {
		CAS_ERR(DEMUX,"GxCore_Malloc failure");
		return -1;
	}

	_dmx.filter_list  = GxCore_Calloc(MAX_FILTER_COUNT, sizeof(struct gxca_filter));
	if(_dmx.filter_list == NULL) {
		CAS_ERR(DEMUX,"GxCore_Malloc failure");
		GxCore_Free(_dmx.channel_list);
		return -1;
	}

	_dmx.channel_num  = MAX_FILTER_COUNT;
	_dmx.filter_num   = MAX_FILTER_COUNT;

	for (i = 0; i < _dmx.channel_num; i++) {
		_dmx.channel_list[i].id = -1;
		_dmx.channel_list[i].pid = -1;
	}

	for (i = 0; i < _dmx.filter_num; i++) {
		_dmx.filter_list[i].id = -1;
	}

	_dmx.dev    = GxAVCreateDevice(0);
	_dmx.dmx    = GxAVOpenModule(_dmx.dev,GXAV_MOD_DEMUX, dmx_id);
	config_demux.source = tsid;
	config_demux.ts_select = FRONTEND;
	config_demux.stream_mode = DEMUX_PARALLEL;
	config_demux.time_gate = 0xf;
	config_demux.byt_cnt_err_gate = 0x3;
	config_demux.sync_loss_gate = 0x3;
	config_demux.sync_lock_gate = 0x3;
	GxAVSetProperty(_dmx.dev, _dmx.dmx, GxDemuxPropertyID_Config, &config_demux, sizeof(GxDemuxProperty_ConfigDemux));

	if(GxCore_MutexCreate(&_dmx.lock) < 0) {
		GxCore_Free(_dmx.channel_list);
		GxCore_Free(_dmx.filter_list);
		GxAVCloseModule(_dmx.dev, _dmx.dmx);
		GxAVDestroyDevice(_dmx.dev);
		CAS_ERR(DEMUX,"Create mutex failuer!");
		return -1;
	}
	return 0;
}

int32_t GxCas_ChannelFree(handle_t handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	CAS_DBG(DEMUX,"Free a channel=%p", channel);

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
#ifndef CONFIG_VMX
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotDisable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret == -1) {
		CAS_ERR(DEMUX,"Disable slot failure!");
		goto err;
	}
#endif

	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotFree,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Free slot failure!");
		goto err;
	}
	channel->id = -1;
	channel->pid = -1;
	gxcas_channel_num--;
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_Demux_Destroy(void)
{
	int32_t i;
	if(_dmx.channel_num == 0|| _dmx.filter_num == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		return -1;
	}
	for (i = 0; i < _dmx.channel_num; i++) {
		if( _dmx.channel_list[i].pid != -1 )
			GxCas_ChannelFree((handle_t)(_dmx.channel_list + i));
	}
	GxCore_Free(_dmx.channel_list);
	GxCore_Free(_dmx.filter_list);
	_dmx.channel_num  = 0;
	_dmx.filter_num   = 0;
	GxCore_MutexDelete(_dmx.lock);
	GxAVCloseModule(_dmx.dev, _dmx.dmx);
	GxAVDestroyDevice(_dmx.dev);
	return 0;

}

handle_t GxCas_ChannelAllocate(uint16_t pid)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if(_dmx.channel_num == 0 || _dmx.filter_num == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		goto err;
	}

	channel = alloc_channel();
	if (channel == NULL) {
		CAS_ERR(DEMUX,"There is no free slot!");
		goto err;
	}

	if (channel->pid != -1) {
		CAS_DBG(DEMUX,"Using a allocated solt_id: %d, pid=%d", channel->id, pid);
		goto err;
	}

	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	channel->demux = _dmx.dmx;
	slot.type = DEMUX_SLOT_PSI;
	slot.pid  = pid;
	ret = GxAVGetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotAlloc,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Hardware  alloc slot failure!");
		goto err;
	}

	slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotConfig,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Config slot failure!");
		goto err;
	}

	channel->id = slot.slot_id;
	channel->pid = pid;
	CAS_DBG(DEMUX,"Allocate a new solt_id: %d, pid=%d", channel->id, pid);
	gxcas_channel_num++;
	GxCore_MutexUnlock(_dmx.lock);
	return (handle_t)channel;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}


int32_t GxCas_ChannelSetPID(handle_t handle,  uint16_t pid,bool RepeatFlag)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Set channel=%p pid=%d", channel, pid);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id    = channel->id;
	slot.pid        = pid;
	slot.type       = DEMUX_SLOT_PSI;
	if (TRUE == RepeatFlag) {
		slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	} else {
		slot.flags      = (DMX_AVOUT_EN);
	}

	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotConfig,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Config slot failure!");
		goto err;
	}

	channel->pid = pid;
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_ChannelEnable(handle_t handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Enable a channel=%p", channel);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotEnable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Enable slot failure!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_ChannelDisable(handle_t  handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Enable a channel=%p", channel);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotDisable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Enable slot failure!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

handle_t GxCas_ChannelGetByPid(uint16_t pid)
{
	int32_t         i;
	GxCore_MutexLock(_dmx.lock);
	for (i = 0; i < _dmx.channel_num; i++) {
		if (_dmx.channel_list[i].pid == pid &&_dmx.channel_list[i].demux == _dmx.dmx) {
			GxCore_MutexUnlock(_dmx.lock);
			CAS_DBG(DEMUX,"Get channel=%p, pid=%d Successed", _dmx.channel_list + i,pid);
			return (handle_t)(_dmx.channel_list + i);
		}
	}
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

handle_t GxCas_FilterAllocate(handle_t handle)
{
	int32_t                     ret;
	struct gxca_filter*         filter;
	GxDemuxProperty_Filter      param;
	struct gxca_channel*        channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	if(_dmx.channel_num == 0|| _dmx.filter_num == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		goto err;
	}

	memset(&param,0,sizeof(param));
	filter = alloc_filter();
	if (filter == NULL) {
		CAS_ERR(DEMUX,"There is no free filter!");
		goto err;
	}

	filter->channel  = channel;
	param.slot_id    = channel->id;
	ret = GxAVGetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterAlloc,
			&param, sizeof(GxDemuxProperty_Filter));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Hardware  alloc slot failure!");
		goto err;
	}

	filter->id = param.filter_id;
	//printf("%s,%d,%d,%d,%d\n", __func__, __LINE__ , channel->pid ,channel->id, filter->id);
	CAS_DBG(DEMUX,"Allocate a filter: filter=%p, channel=%p, filter_id=%d",
			filter, channel, filter->id);
	GxCore_MutexUnlock(_dmx.lock);
	gxcas_filter_num++;
	return (handle_t)filter;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}
#else
struct gx_demux {
	int32_t              dev;/*设备句柄*/
	int32_t              dmx[MAX_DEMUX_NUM];/*打开的demux句柄*/
	handle_t             lock;/*模块用到的互斥锁句柄*/
	uint32_t             channel_num[MAX_DEMUX_NUM];/*模块初始化是设置的slot个数, 注意跟硬件提供的个数无关*/
	struct gxca_channel *channel_list[MAX_DEMUX_NUM];/*描述channel的结构体链表*/
	uint32_t             filter_num[MAX_DEMUX_NUM];	/*模块初始化时设置的filter个数*/
	struct gxca_filter  *filter_list[MAX_DEMUX_NUM];/*描述filter的结构体链表*/
};

static int32_t gxcas_channel_num = 0;
static int32_t gxcas_filter_num = 0;
static struct gx_demux _dmx;

/*内部函数,用于获取dmx_id*/
static int16_t get_dmx_id(int32_t demux_handle)
{
	int32_t i = 0;

	for (i = 0; i < MAX_DEMUX_NUM; i++) {
		if (_dmx.dmx[i] == demux_handle) {
			return i;
		}
	}
	return -1;
}

/*内部函数,用于分配slot控制块*/
static struct gxca_channel* alloc_channel(uint16_t dmx_id)
{
	int32_t i = 0;

	for (i = 0; i < _dmx.channel_num[dmx_id]; i++) {
		if (_dmx.channel_list[dmx_id][i].id == -1) {
			return &_dmx.channel_list[dmx_id][i];
		}
	}
	return NULL;
}

/*内部函数,用于分配fliter控制块*/
static struct gxca_filter* alloc_filter(uint16_t dmx_id)
{
	int32_t i = 0;

	for (i = 0; i < _dmx.filter_num[dmx_id]; i++) {
		if (_dmx.filter_list[dmx_id][i].id == -1) {
			return &_dmx.filter_list[dmx_id][i];
		}
	}
	return NULL;
}


int32_t GxCas_Demux_Init(uint32_t tsid, uint32_t dmx_id)
{
	int32_t         i;
	static uint8_t s_init_flag = 0;
	GxDemuxProperty_ConfigDemux config_demux = {0};

	if (dmx_id > MAX_DEMUX_NUM) {
		CAS_ERR(DEMUX,"Demux id Invalid!!!");
		return -1;
	}
	if(s_init_flag == 0){
		memset(&_dmx,0,sizeof(_dmx));
		s_init_flag = 1;
	}

	if(_dmx.channel_num[dmx_id]>0 || _dmx.filter_num[dmx_id] >0) {
		CAS_ERR(DEMUX,"Demux will be initialed twice, may be lose some hardware resource!!! ");
		return -1;
	}

	_dmx.channel_list[dmx_id] = GxCore_Calloc(MAX_FILTER_COUNT, sizeof(struct gxca_channel));
	if(_dmx.channel_list[dmx_id] == NULL) {
		CAS_ERR(DEMUX,"GxCore_Malloc failure");
		return -1;
	}

	_dmx.filter_list[dmx_id]  = GxCore_Calloc(MAX_FILTER_COUNT, sizeof(struct gxca_filter));
	if(_dmx.filter_list[dmx_id] == NULL) {
		CAS_ERR(DEMUX,"GxCore_Malloc failure");
		GxCore_Free(_dmx.channel_list[dmx_id]);
		return -1;
	}

	_dmx.channel_num[dmx_id]  = MAX_FILTER_COUNT;
	_dmx.filter_num[dmx_id]   = MAX_FILTER_COUNT;

	for (i = 0; i < _dmx.channel_num[dmx_id]; i++) {
		_dmx.channel_list[dmx_id][i].id = -1;
		_dmx.channel_list[dmx_id][i].pid = -1;
	}

	for (i = 0; i < _dmx.filter_num[dmx_id]; i++) {
		_dmx.filter_list[dmx_id][i].id = -1;
	}

	_dmx.dev    = GxAVCreateDevice(0);
	_dmx.dmx[dmx_id]    = GxAVOpenModule(_dmx.dev,GXAV_MOD_DEMUX, dmx_id);
	config_demux.source = tsid;
	if(tsid == 3)
		config_demux.ts_select = OTHER;
	else
		config_demux.ts_select = FRONTEND;
	config_demux.stream_mode = DEMUX_PARALLEL;
	config_demux.time_gate = 0xf;
	config_demux.byt_cnt_err_gate = 0x3;
	config_demux.sync_loss_gate = 0x3;
	config_demux.sync_lock_gate = 0x3;
	GxAVSetProperty(_dmx.dev, _dmx.dmx[dmx_id], GxDemuxPropertyID_Config, &config_demux, sizeof(GxDemuxProperty_ConfigDemux));

	if(_dmx.lock == 0 ){
		if(GxCore_MutexCreate(&_dmx.lock) < 0) {
			GxCore_Free(_dmx.channel_list[dmx_id]);
			GxCore_Free(_dmx.filter_list[dmx_id]);
			GxAVCloseModule(_dmx.dev, _dmx.dmx[dmx_id]);
			GxAVDestroyDevice(_dmx.dev);
			CAS_ERR(DEMUX,"Create mutex failuer!");
			return -1;
		}
	}
	return 0;
}

int32_t GxCas_ChannelFree(handle_t handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	CAS_DBG(DEMUX,"Free a channel=%p", channel);

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotDisable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret == -1) {
		CAS_ERR(DEMUX,"Disable slot failure!");
		goto err;
	}

	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotFree,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Free slot failure!");
		goto err;
	}
	channel->id = -1;
	channel->pid = -1;
	gxcas_channel_num--;
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_Demux_Destroy(uint32_t dmx_id)
{
	int32_t i;
	if(_dmx.channel_num[dmx_id] == 0|| _dmx.filter_num[dmx_id] == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		return -1;
	}
	for (i = 0; i < _dmx.channel_num[dmx_id]; i++) {
		if( _dmx.channel_list[dmx_id][i].pid != -1 )
			GxCas_ChannelFree((handle_t)(_dmx.channel_list[dmx_id] + i));
	}
	GxCore_Free(_dmx.channel_list[dmx_id]);
	GxCore_Free(_dmx.filter_list[dmx_id]);
	_dmx.channel_num[dmx_id]  = 0;
	_dmx.filter_num[dmx_id]   = 0;
	GxCore_MutexDelete(_dmx.lock);
	GxAVCloseModule(_dmx.dev, _dmx.dmx[dmx_id]);
	GxAVDestroyDevice(_dmx.dev);
	return 0;

}

handle_t GxCas_ChannelAllocate(uint16_t pid, uint32_t dmx_id)
{
	int32_t		ret;
	int32_t		i = 0;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);

	if (dmx_id > MAX_DEMUX_NUM) {
		CAS_ERR(DEMUX,"Demux id Invalid!!!");
		goto err;
	}
	if(dmx_id == MAX_DEMUX_NUM){
		for(i = 0; i < MAX_DEMUX_NUM; i++){
			if(_dmx.dmx[i] > 0){
				dmx_id = i;
				CAS_DBG(DEMUX,"dmx_id=%d", dmx_id);
				break;
			}
		}
		printf("====%s,%d, pid:%x, dmxId:%d\n", __FUNCTION__, __LINE__, pid, dmx_id);
		if(i == MAX_DEMUX_NUM){
			CAS_ERR(DEMUX,"Demux id Invalid!!!");
			goto err;
		}
	}

	if(_dmx.channel_num[dmx_id] == 0 || _dmx.filter_num[dmx_id] == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		goto err;
	}

	channel = alloc_channel(dmx_id);
	if (channel == NULL) {
		CAS_ERR(DEMUX,"There is no free slot!");
		goto err;
	}

	if (channel->pid != -1) {
		CAS_DBG(DEMUX,"Using a allocated solt_id: %d, pid=%d", channel->id, pid);
		goto err;
	}

	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	channel->demux = _dmx.dmx[dmx_id];
	slot.type = DEMUX_SLOT_PSI;
	slot.pid  = pid;
	ret = GxAVGetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotAlloc,
			&slot, sizeof(GxDemuxProperty_Slot));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Hardware  alloc slot failure!");
		goto err;
	}

	slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotConfig,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Config slot failure!");
		goto err;
	}

	channel->id = slot.slot_id;
	channel->pid = pid;
	CAS_DBG(DEMUX,"Allocate a new solt_id: %d, pid=%d", channel->id, pid);
	gxcas_channel_num++;
	GxCore_MutexUnlock(_dmx.lock);
	return (handle_t)channel;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}


int32_t GxCas_ChannelSetPID(handle_t handle,  uint16_t pid,bool RepeatFlag)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Set channel=%p pid=%d", channel, pid);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));
	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id    = channel->id;
	slot.pid        = pid;
	slot.type       = DEMUX_SLOT_PSI;
	if (TRUE == RepeatFlag) {
		slot.flags      = (DMX_REPEAT_MODE | DMX_AVOUT_EN);
	} else {
		slot.flags      = (DMX_AVOUT_EN);
	}

	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotConfig,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Config slot failure!");
		goto err;
	}

	channel->pid = pid;
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_ChannelEnable(handle_t handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Enable a channel=%p", channel);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotEnable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Enable slot failure!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_ChannelDisable(handle_t  handle)
{
	int32_t		ret;
	GxDemuxProperty_Slot	slot;
	struct gxca_channel*	channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;

	CAS_DBG(DEMUX,"Enable a channel=%p", channel);
	memset(&slot,0,sizeof(GxDemuxProperty_Slot));

	if (channel->pid == -1 || channel->id < 0){
		CAS_ERR(DEMUX,"Try to operate a free channel");
		goto err;
	}

	slot.slot_id = channel->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_SlotDisable,
			&slot, sizeof(GxDemuxProperty_Slot));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Enable slot failure!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

handle_t GxCas_ChannelGetByPid(uint16_t pid, uint32_t dmx_id)
{
	int32_t         i;
	GxCore_MutexLock(_dmx.lock);
	if(dmx_id == MAX_DEMUX_NUM){
		for(i = 0; i < MAX_DEMUX_NUM; i++){
			if(_dmx.dmx[i] > 0){
				dmx_id = i;
				break;
			}
		}
		if(i == MAX_DEMUX_NUM){
			CAS_ERR(DEMUX,"Demux id Invalid!!!");
			goto err;
		}
	}
	for (i = 0; i < _dmx.channel_num[dmx_id]; i++) {
		if (_dmx.channel_list[dmx_id][i].pid == pid &&_dmx.channel_list[dmx_id][i].demux == _dmx.dmx[dmx_id]) {
			GxCore_MutexUnlock(_dmx.lock);
			CAS_DBG(DEMUX,"Get channel=%p, pid=%d Successed", _dmx.channel_list[dmx_id] + i,pid);
			return (handle_t)(_dmx.channel_list[dmx_id] + i);
		}
	}
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

handle_t GxCas_FilterAllocate(handle_t handle)
{
	int32_t                     ret;
	struct gxca_filter*         filter;
	GxDemuxProperty_Filter      param;
	struct gxca_channel*        channel = NULL;
	int16_t dmx_id = -1;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	channel = (struct gxca_channel*)handle;
#if 0
	if(_dmx.channel_num == 0|| _dmx.filter_num == 0) {
		CAS_ERR(DEMUX,"U try to destroy demux before init!!! ");
		goto err;
	}
#endif
	memset(&param,0,sizeof(param));
	dmx_id = get_dmx_id(channel->demux);
	filter = alloc_filter(dmx_id);
	if (filter == NULL) {
		CAS_ERR(DEMUX,"There is no free filter!");
		goto err;
	}

	filter->channel  = channel;
	param.slot_id    = channel->id;
	ret = GxAVGetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterAlloc,
			&param, sizeof(GxDemuxProperty_Filter));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Hardware  alloc slot failure!");
		goto err;
	}

	filter->id = param.filter_id;
	//printf("%s,%d,%d,%d,%d\n", __func__, __LINE__ , channel->pid ,channel->id, filter->id);
	CAS_DBG(DEMUX,"Allocate a filter: filter=%p, channel=%p, filter_id=%d",
			filter, channel, filter->id);
	GxCore_MutexUnlock(_dmx.lock); gxcas_filter_num++;
	return (handle_t)filter;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

#endif
int32_t GxCas_FilterGetPID(handle_t handle,int16_t *pid)
{
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;
	*pid = channel->pid;
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_FilterFree(handle_t handle)
{
	int32_t                 ret;
	GxDemuxProperty_Filter  param = {0};
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;

	CAS_DBG(DEMUX,"Free a filter=%p", filter);

	if (filter->id == -1) {
		CAS_ERR(DEMUX,"Try to operate a free filter");
		goto err;
	}

	param.filter_id = filter->id;
	//printf("%s,%d,%d,%d,%d\n", __func__, __LINE__ , channel->pid ,channel->id, filter->id);
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterDisable,
			&param, sizeof(GxDemuxProperty_Filter));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Free filter failure!");
		goto err;
	}

	param.slot_id = channel->id;
	param.filter_id = filter->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterFree,
			(void*)&param, sizeof(GxDemuxProperty_Filter));
	if (ret != 0) {
		CAS_ERR(DEMUX,"Free filter failure!");
		goto err;
	}
	filter->id = -1;
	filter->channel = 0;
	GxCore_MutexUnlock(_dmx.lock);
	gxcas_filter_num--;
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_FilterSetup(handle_t    handle,
		const uint8_t*        Match,
		const uint8_t*        Mask,
		bool                  Equal,
		bool                  CRCFlag,
		size_t                Depth)
{
	int32_t	ret;
	uint32_t	i;
	GxDemuxProperty_Filter          param;
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;

	memset(&param,0,sizeof(param));
	CAS_DBG(DEMUX, "Setup filter=%p", filter);

	if (filter->id == -1) {
		CAS_ERR(DEMUX,"Try to operate a free filter");
		goto err;
	}

	param.slot_id      = channel->id;
	param.filter_id    = filter->id;

	CAS_DBG(DEMUX,"Match:");
	CAS_DUMP(DEMUX, Match,Depth, "depth");
	CAS_DBG(DEMUX,"Mask:");
	CAS_DUMP(DEMUX,Mask,Depth, "depth");

	for (i = 0; i < Depth; i++) {
		param.key[i].value = Match[i];
		param.key[i].mask  = Mask[i];
	}
	param.key[1].mask  = 0x00;
	param.key[2].mask  = 0x00;

	param.depth = Depth;
	if (Equal)
		param.flags = DMX_EQ;
	else
		param.flags = 0;

	if (CRCFlag)
		param.flags |= DMX_CRC_IRQ;

	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterConfig,
			&param, sizeof(GxDemuxProperty_Filter));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Setup fifter err!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_FilterEnable(handle_t handle)
{
	int32_t                         ret;
	GxDemuxProperty_Filter          param;
	GxDemuxProperty_FilterFifoReset fifo;
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;

	memset(&param,0,sizeof(param));
	CAS_DBG(DEMUX,"Enable filter=%p", filter);
	if (filter->id == -1) {
		CAS_ERR(DEMUX,"Try to operate a free filter");
		goto err;
	}

	fifo.filter_id = filter->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterFIFOReset,
			&fifo, sizeof(GxDemuxProperty_FilterFifoReset));
	if(ret != 0){
		CAS_ERR(DEMUX,"Rest filter fifo err!");
		goto err;
	}

	param.slot_id      = channel->id;
	param.filter_id    = filter->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterEnable,
			&param, sizeof(GxDemuxProperty_Filter));
	if(ret != 0) {
		CAS_ERR(DEMUX,"Enable filter fifo err!");
		goto err;
	}

	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_FilterDisable(handle_t handle)
{
	int32_t                 ret;
	GxDemuxProperty_Filter  param;
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;


	memset(&param,0,sizeof(param));
	CAS_DBG(DEMUX,"Diable filter=%p", filter);

	if (filter->id == -1) {
		CAS_ERR(DEMUX,"Try to operate a free filter");
		goto err;
	}

	param.filter_id = filter->id;
	ret = GxAVSetProperty(_dmx.dev, channel->demux,
			GxDemuxPropertyID_FilterDisable,
			&param, sizeof(GxDemuxProperty_Filter));
	if(ret != 0)
	{
		CAS_ERR(DEMUX,"Disable filter fifo err!");
		goto err;
	}
	GxCore_MutexUnlock(_dmx.lock);
	return 0;
err:
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}

int32_t GxCas_QueryAndGetData(handle_t handle,
		uint8_t *Buffer,
		uint32_t  BufferSize,
		uint32_t *Datalen)
{
	GxDemuxProperty_FilterFifoQuery query;
	int32_t ret = -1;
	GxDemuxProperty_FilterRead      read;
	struct gxca_filter*     filter = NULL;
	struct gxca_channel*    channel = NULL;

	GxCore_MutexLock(_dmx.lock);
	if((int32_t *)handle == NULL) goto err;
	filter = (struct gxca_filter*)handle;
	if(filter->channel == NULL) goto err;
	channel = filter->channel;


	if(Buffer == NULL || Datalen == NULL){
		CAS_ERR(DEMUX,"param is NULL");
		goto err;
	}

	if (filter->id == -1) {
		CAS_ERR(DEMUX,"Try to operate a free filter");
		goto err;
	}

//#include "av/gxav_demux_propertytypes.h"
//	GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};
//	if ((GxAVGetProperty(_dmx.dev, channel->demux, GxDemuxPropertyID_TSLockQuery, &ts_lock_status, sizeof(GxDemuxProperty_TSLockQuery)) >= 0) &&
//			ts_lock_status.ts_lock != TS_SYNC_LOCKED)
//		printf("\033[43 ------------------------------- %s,%d\033[0m\n", __func__, __LINE__);

	ret = GxAVGetProperty(_dmx.dev,channel->demux,
			GxDemuxPropertyID_FilterFIFOQuery,
			&query, sizeof(GxDemuxProperty_FilterFifoQuery));
	if(ret != 0) {
		CAS_ERR(DEMUX,"query filter err");
		goto err;
	}

	if (query.state == 0)
		goto err;

	if (((query.state >>filter->id) & 1ULL) == 1ULL) {
		read.filter_id = filter->id;
		read.max_size = BufferSize;
		read.buffer = Buffer;
		ret = GxAVGetProperty(_dmx.dev, channel->demux,
				GxDemuxPropertyID_FilterRead,
				&read, sizeof(GxDemuxProperty_FilterRead));
		if(ret != 0) {
			CAS_ERR(DEMUX,"Read data err");
			goto err;
		}
		*Datalen = read.read_size;
		GxCore_MutexUnlock(_dmx.lock);
		return 0;
	}
err:
	*Datalen = 0;
	GxCore_MutexUnlock(_dmx.lock);
	return -1;
}
#endif
#ifdef CONFIG_DMX_NEW //demux hal
#include "dvbhal/gxdemux_hal.h"
static uint16_t s_demux_id = 0;
int32_t GxCas_Demux_Init(uint32_t tsid, uint32_t dmx_id)
{
	int32_t ret = 0;
	ret = GxDmxInit();
	if(ret < 0){
		CAS_DBG(DEMUX,"GxDmxInit Error!!");
		return -1;
	}
	ret = GxDmxSetSource(dmx_id, tsid);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxSetSource Error, dmx id = %d, tsid = %d!!", dmx_id, tsid);
		return -1;
	}
	s_demux_id = dmx_id;
	return 0;
}

int32_t GxCas_ChannelFree(handle_t handle)
{
	int32_t ret = 0;
	ret = GxChannelFree(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelFree Error, handle = 0x%x", handle);
		return -1;
	}
	return 0;
}

int32_t GxCas_Demux_Destroy(void)
{
	int32_t ret = 0;

	ret = GxDmxDeinit();
	if(ret < 0){
		CAS_DBG(DEMUX,"GxDmxDeinit Error!!");
		return -1;
	}
	return 0;

}

#ifdef CONFIG_ABV_PVR
handle_t GxCas_ChannelAllocate(uint16_t pid, uint32_t dmx_id)
{
	int32_t ret = 0;

	ret = GxChannelAllocate(dmx_id, pid);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelAllocate Error!!");
		return -1;
	}
	return ret;
}
#else
handle_t GxCas_ChannelAllocate(uint16_t pid)
{
	int32_t ret = 0;

	ret = GxChannelAllocate(s_demux_id, pid);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelAllocate Error!!");
		return -1;
	}
	return ret;
}
#endif

handle_t GxCas_PesChannelAllocate(uint16_t pid)
{
	int32_t ret = 0;

	ret = GxPesChannelAllocate(s_demux_id, pid);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelAllocate Error!!");
		return -1;
	}
	return ret;
}

int32_t GxCas_ChannelSetPID(handle_t handle,  uint16_t pid,bool RepeatFlag)
{
	int32_t ret = 0;

	ret = GxChannelSetPID(handle, pid, RepeatFlag);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelSetPID Error!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_ChannelEnable(handle_t handle)
{
	int32_t ret = 0;

	ret = GxChannelEnable(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelEnable Error!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_ChannelDisable(handle_t  handle)
{
	int32_t ret = 0;

	ret = GxChannelDisable(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxChannelDisable Error!!");
		return -1;
	}
	return 0;
}

handle_t GxCas_FilterAllocate(handle_t handle)
{
	int32_t ret = 0;

	ret = GxFilterAllocate(handle, 0);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxFilterAllocate Error!!");
		return -1;
	}
	return (handle_t)ret;
}

int32_t GxCas_FilterFree(handle_t handle)
{
	int32_t ret = 0;

	ret = GxFilterFree(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxFilterFree Error!!");
		return -1;
	}
	return 0;
}

int32_t GxCas_FilterSetup(handle_t    handle,
		const uint8_t*        Match,
		const uint8_t*        Mask,
		bool                  Equal,
		bool                  CRCFlag,
		size_t                Depth)
{
	int32_t ret = 0;

	ret = GxFilterSetup(handle, Match, Mask, Equal, CRCFlag, Depth);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxFilterSetup Error!!");
		return -1;
	}
	return 0;
}

int32_t GxCas_FilterEnable(handle_t handle)
{
	int32_t ret;

	ret = GxFilterEnable(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxFilterEnable Error!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_FilterDisable(handle_t handle)
{
	int32_t ret = 0;

	ret = GxFilterDisable(handle);
	if(ret < 0){
		CAS_DBG(DEMUX,"GxFilterDisable Error!!");
		return -1;
	}

	return 0;
}

int32_t GxCas_QueryAndGetData(handle_t handle,
		uint8_t *Buffer,
		uint32_t  BufferSize,
		uint32_t *Datalen)
{
	int32_t ret = 0;

	ret = GxFilterRead(handle, Buffer, BufferSize, Datalen);
	if(ret < 0){
        if(ret == -3)
        {
            GxFilterDisable(handle);
            GxFilterEnable(handle);
        }
		CAS_DBG(DEMUX,"GxFilterRead Error!!");
		return -1;
	}

	return ret;
}
#endif
