#include "gxtype.h"
#include "gxcas_dbg.h"
#include "gxcas_psi.h"
#include "gxcas_demux.h"
#include "gxcas_misc.h"
#include "autoconf.h"

#define PSI_BUF (64*1024)

struct si_filter {
	int32_t       id;
	handle_t      filter;
	handle_t      channel;
	int32_t       nms;                 /*未过滤到数据已过时间*/
	sifilter_params_t params;
};

static struct si_filter sifilter[MAX_FILTER_COUNT];
static handle_t si_lock;

static void _show_si_info(struct si_filter *si)
{
	CAS_ERR(SI, "id = %d\n",si->id);
	CAS_ERR(SI, "filter = 0x%x\n",si->filter);
	CAS_ERR(SI, "channel = %d\n",si->channel);
	CAS_ERR(SI, "pid = %d\n",si->params.pid);
	CAS_DUMP(SI, si->params.match, si->params.depth, " match: ");
	CAS_DUMP(SI,si->params.mask,  si->params.depth, " mask: ");
}

static void gxcas_show_si_info(int pid)
{
	int32_t i = 0;
	struct si_filter *si = NULL;
	for(i = 0; i < MAX_FILTER_COUNT; i++) {
		si = &sifilter[i];
		if (si->params.pid == pid ){
			_show_si_info(si);
		}
	}
}

int32_t GxCas_SiFilter_EmmEcm_CheckCrc(uint8_t* pData)
{
	uint32_t len = 0, crc1 = 0, crc2 = 0, tid = 0;
	uint8_t* p = pData;

	tid = p[0];
	if (tid == 0x70)
		return 0;

	len = ((p[1] & 0xf) << 8) | p[2];
	if (len < 1) {
		CAS_ERR(SI, "len < 1");
		return -1;
	}

	len = len + 3 - 4;
	crc1 = GxCas_Crc32(p, len);
	p += len;
	crc2 = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) |
		((p[2] & 0xff) << 8) | ((p[3] & 0xff));
	if (crc1 == crc2)
		return 0;
	else
		return -1;
}

int32_t GxCas_SiFilter_EmmEcm_CheckSame(uint8_t* pData)
{
	uint32_t len = 0, crc1 = 0,  tid = 0;
	uint8_t* p = pData;
	static uint32_t buf_crc_old= 0;

	tid = p[0];
	if (tid == 0x70)
		return 0;

	len = ((p[1] & 0xf) << 8) | p[2];
	if (len < 1) {
		CAS_ERR(SI, "len < 1");
		return -1;
	}

	len = len + 3 - 4;
	crc1 = GxCas_Crc32(p, len);
	if(buf_crc_old!= crc1) {
		buf_crc_old= crc1;
		return 0;
	}

	return -1;
}


int32_t GxCas_SiFilter_CheckCrc(uint8_t* pData)
{
	uint32_t len = 0, crc1 = 0, crc2 = 0, tid = 0;
	uint8_t* p = pData;

	tid = p[0];
	if (tid == 0x70)
		return 0;
	else if (tid >= 0x80 && tid <= 0xfe) {
		if ((p[1] & 0x80) == 0)
			return 0;
	}

	len = ((p[1] & 0xf) << 8) | p[2];
	if (len < 1) {
		CAS_ERR(SI, "len < 1");
		return -1;
	}

	len = len + 3 - 4;
	crc1 = GxCas_Crc32(p, len);
	p += len;
	crc2 = ((p[0] & 0xff) << 24) | ((p[1] & 0xff) << 16) |
		((p[2] & 0xff) << 8) | ((p[3] & 0xff));
	if (crc1 == crc2)
		return 0;
	else
		return -1;
}

void GxCas_SiFilter_Init(void)
{
	static int32_t init = 0;
	struct si_filter *si = NULL;
	int32_t i = 0;
	if(init == 0)
		init = 1;
	else
		return;

	for (i = 0; i < MAX_FILTER_COUNT; i++){
		si = &sifilter[i];
		si->filter = -1;
		si->channel = -1;
		si->id = -1;
		memset(&si->params,0,sizeof(sifilter_params_t)) ;
		si->params.pid = PSI_INVALID_PID;
	}

	GxCore_MutexCreate(&si_lock);
	return;
}

handle_t GxCas_Pes_Start(uint16_t      pid)
{
	handle_t	  channel;
	int32_t ret = 0;

	bool RepeatFlag = 0;	
	

	GxCore_MutexLock(si_lock);

	channel = GxCas_PesChannelAllocate(pid);
	if ( -1 == channel)
		goto err;
	
	RepeatFlag = REPEAT_FLAG;
	ret = GxCas_ChannelSetPID(channel,pid,RepeatFlag);
	if(ret < 0)
		goto err;
	
	ret = GxCas_ChannelEnable(channel);
	if (ret == -1)
		goto err;
	GxCore_MutexUnlock(si_lock);
	return (handle_t)channel;
	
err:
	GxCore_MutexUnlock(si_lock);
	return -1;
}

int32_t GxCas_Pes_Stop(handle_t handle)
{
	GxCore_MutexLock(si_lock);
	GxCas_ChannelFree(handle);
	GxCore_MutexUnlock(si_lock);
	return 0;
}


handle_t GxCas_SiFilter_Start(sifilter_params_t params)
{
	int32_t ret = 0;
	uint32_t i =0;
	GxTime start={0};
	struct si_filter *si = NULL;
	uint32_t eq = 0;
	uint32_t crc = 0;
	bool RepeatFlag = 0;	
	

	GxCore_MutexLock(si_lock);
#ifndef CONFIG_VMX
	for (i = 0; i< MAX_FILTER_COUNT; i++) {
		si = &sifilter[i];
		if ((si->params.pid == params.pid ) && (params.pid != PSI_INVALID_PID) &&
				(memcmp(params.match, si->params.match, sizeof(params.match)) == 0) &&
				(memcmp(params.mask, si->params.mask, sizeof(params.mask)) == 0)) {
			GxCore_MutexUnlock(si_lock);
			return (handle_t)si;
		}
	}
#endif

	for (i = 0; i< MAX_FILTER_COUNT; i++) {
		si = &sifilter[i];
		if (si->id == -1){
			si->id = i;
			break;
		}
	}
	if (si == NULL)
		goto err;

	memcpy(&si->params, &params, sizeof(sifilter_params_t));
#ifdef CONFIG_ABV_PVR
	si->channel = GxCas_ChannelAllocate(si->params.pid, MAX_DEMUX_NUM);
#else
	si->channel = GxCas_ChannelAllocate(si->params.pid);
#endif
	if ( -1 == si->channel)
		goto err;
	
	RepeatFlag = si->params.flags & REPEAT_FLAG;
	ret = GxCas_ChannelSetPID(si->channel,si->params.pid,RepeatFlag);
	if(ret < 0)
		goto err;

	si->filter = GxCas_FilterAllocate(si->channel);
	if (-1 == si->filter)
		goto err;

	eq = si->params.flags & EQ_FLAG;
	crc = si->params.flags & CRC_FLAG;
	ret = GxCas_FilterSetup(si->filter,
			si->params.match,
			si->params.mask,
			eq,crc,
			si->params.depth);
	if (ret == -1)
		goto err;

	ret = GxCas_ChannelEnable(si->channel);
	if (ret == -1)
		goto err;

	ret = GxCas_FilterEnable(si->filter);
	if (ret == -1)
		goto err;

	GxCore_GetTickTime(&start);
	si->nms  = start.seconds;

	GxCore_MutexUnlock(si_lock);
	return (handle_t)si;
err:
	if(si){
		_show_si_info(si);
		if(si->filter != -1){
			GxCas_FilterFree(si->filter);
		}
		if(si->channel != -1){
			GxCas_ChannelFree(si->channel);
		}
		si->filter = -1;
		si->channel = -1;
		si->id = -1;
		memset(&si->params,0,sizeof(sifilter_params_t));
		si->params.pid = PSI_INVALID_PID;
	}
	GxCore_MutexUnlock(si_lock);
	return -1;
}

int32_t GxCas_SiFilter_Stop(handle_t handle)
{
	GxCore_MutexLock(si_lock);
	struct si_filter *si = (struct si_filter *)handle;

	if ((si->filter != -1) && (si->channel != -1)) {
		GxCas_FilterFree(si->filter);
		si->filter = -1;
		GxCas_ChannelFree(si->channel);
		si->channel = -1;
		memset(&si->params,0,sizeof(sifilter_params_t));
		si->id = -1;
		si->params.pid = PSI_INVALID_PID;
	} else{
		GxCore_MutexUnlock(si_lock);
		return -1;
	}
	GxCore_MutexUnlock(si_lock);
	return 0;
}

int32_t GxCas_SiFilter_Read(handle_t handle, uint8_t *buffer, uint32_t size, uint32_t *len)
{
	int ret =-1;
	GxTime nowtime = {0};
	uint32_t ends;

	GxCore_MutexLock(si_lock);
	struct si_filter *si = (struct si_filter *)handle;
	if ((si->filter != -1) && (si->channel != -1)) {
		ret = GxCas_QueryAndGetData(si->filter, buffer, size, len);
#ifdef CONFIG_DMX_OLD // old method
		if(ret == 0) {
#else
		if((ret == 0) && (*len > 0)) {
#endif
			if (0 != si->params.nWaitSeconds) { /* * re calcate time counts */
				GxCore_GetTickTime(&nowtime);
				ends = nowtime.seconds;
				si->nms=ends;
			}
		} else {
			/* * deal without data */
			if (0 != si->params.nWaitSeconds) { /* * judge timeout or not */
				GxCore_GetTickTime(&nowtime);
				ends = nowtime.seconds;
				if (ends - si->nms >= si->params.nWaitSeconds) {
					si->nms=ends;
					goto err;
				}
			}
		}
	}

	GxCore_MutexUnlock(si_lock);
	return 0;
err:
	GxCore_MutexUnlock(si_lock);
	return -1;
}
void GxCas_SiFilter_Reset(handle_t handle)
{
	GxCore_MutexLock(si_lock);
	struct si_filter *si = (struct si_filter *)handle;
#ifndef CONFIG_VMX
	GxCas_ChannelDisable(si->channel);
#endif
	GxCas_FilterDisable(si->filter);
#ifndef CONFIG_VMX
	GxCas_ChannelEnable(si->channel);
#endif
	GxCas_FilterEnable(si->filter);
	GxCore_MutexUnlock(si_lock);
	return;
}

int32_t GxCas_SiFilter_GetInfo(handle_t handle, sifilter_params_t *params)
{
	GxCore_MutexLock(si_lock);
	struct si_filter *si = (struct si_filter *)handle;
	if ((si->filter != -1) && (si->channel != -1))
		memcpy(params, &si->params, sizeof(sifilter_params_t));
	GxCore_MutexUnlock(si_lock);
	return 0;
}

