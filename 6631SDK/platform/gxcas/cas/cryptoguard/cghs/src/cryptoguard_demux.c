#include "gxcore.h"
#include "autoconf.h"
#include "gxcas_demux.h"
#include "gxcas_dbg.h"
#include "cryptoguard_misc.h"
#include "cg_cas.h"

#define MAX_CPG_FILTER_COUNT	MAX_FILTER_COUNT
#define MAX_FILTER_LEN 16

handle_t cpg_handle[MAX_FILTER_COUNT];
static handle_t cpg_lock = 0;
handle_t pmt;
handle_t cat;
struct cpg_service cpgsrv;

static uint8_t get_actual_filter_len(const uint8_t *filter, const uint8_t *mask)
{
	uint8_t i = 0;

	/* filter 或者 mask有一个非0, 表示数据过滤条件在这个位置被设置了 */
	for (i = 15; i>0; i--) {
		if ((filter[i] != 0x00) || (mask[i] != 0x00))
			break;
	}

	return (i+1);
}

int32_t CPGDemuxStartFilter(uint16_t pid, const uint8_t *pdata, const uint8_t *pmask)
{
	sifilter_params_t filter = {0};
	uint8_t         match[MAX_FILTER_LEN+2] = {0,};
	uint8_t         mask[MAX_FILTER_LEN+2] = {0,};
	const uint8_t   *pbyFilter = pdata;
	const uint8_t   *pbyMask = pmask;
	uint16_t        i=0;
	uint8_t         ucFilterLen = MAX_FILTER_LEN;
	handle_t        si = 0;

	if(pbyFilter == NULL || pbyMask == NULL ) {
		CAS_ERR(CAS,"%s pbyFilter=0x%x	pbyMask=0x%x\n",__FUNCTION__,(unsigned int)pbyFilter,(unsigned int)pbyMask);
		return -1;
	}
	/* 因为CAT表的整体长度可能都不够16, 如果使用默认16, 可能导致过滤不到CAT表, 所以要重新获取过滤条件长度 */
	ucFilterLen = get_actual_filter_len(pbyFilter, pbyMask);


	for (i = 0; i< MAX_FILTER_LEN; i++) {
		if (0 == i) {
			match[i] = pbyFilter[i];
			mask[i] = pbyMask[i];
		} else {
			match[i+2] = pbyFilter[i];
			mask[i+2] = pbyMask[i];
		}
	}

#if 0
	filter.flags = EQ_FLAG | REPEAT_FLAG;
	filter.depth = ucFilterLen+2;
	if(filter.depth <= 3)
		filter.depth = 4;

	memcpy(filter.match,match,ucFilterLen+2);
	memcpy(filter.mask,mask,ucFilterLen+2);
#else
	filter.flags = EQ_FLAG | REPEAT_FLAG;
	filter.depth = 1;
	memcpy(filter.match,match,1);
	memcpy(filter.mask,mask,1);

#endif
	filter.nWaitSeconds = 0;
	filter.pid = pid;

	filter.nWaitSeconds = 5;
	GxCore_MutexLock(cpg_lock);
	si = GxCas_SiFilter_Start(filter);

	if(si != -1){
		for (i = 0; i<MAX_FILTER_COUNT;i++) {
			if (cpg_handle[i] == 0) {
				cpg_handle[i] = si;
				break;
			}

		}
	}
	else{
		GxCore_MutexUnlock(cpg_lock);
		CAS_ERR(CAS,"GxCas_SiFilter_Start Failed, %s\n",__FUNCTION__);
		return -1;
	}

	GxCore_MutexUnlock(cpg_lock);
	return 0;
}

uint32_t CPGDemuxStopFilter(uint16_t pid)
{
	uint32_t i = 0;
	sifilter_params_t sifilter;

	GxCore_MutexLock(cpg_lock);
	for (i = 0; i< MAX_FILTER_COUNT;i++) {
		if(cpg_handle[i] != 0){
			memset(&sifilter, 0, sizeof(sifilter_params_t));
			GxCas_SiFilter_GetInfo(cpg_handle[i],&sifilter);
			if(pid == sifilter.pid){
				GxCas_SiFilter_Stop(cpg_handle[i]);
				cpg_handle[i] = 0;
				//GxCore_MutexUnlock(cpg_lock);
				//return 0;
			}
		}
	}
	GxCore_MutexUnlock(cpg_lock);

	//CAS_DBG(CAS,"[%s] pid=0x%x, done nothing\n", __FUNCTION__, pid);
	return 0;
}

static void cpg_deal_timeout(sifilter_params_t sfilter, const uint8_t* section, size_t size)
{
	uint8_t  match[MAX_FILTER_LEN+2] = {0,};
	uint8_t  mask[MAX_FILTER_LEN+2] = {0,};
	if(sfilter.match[0]==0x80 || sfilter.match[0]==0x81){
		CPGDemuxStopFilter(sfilter.pid);
		match[0] = (!(sfilter.match[0]&0x1))|0x80;
		mask[0] = 0xFF;
		CPGDemuxStartFilter(sfilter.pid,match,mask);
	}
	return;
}

static uint64_t array_to_longlong(uint8_t *p)
{
 	if(p != NULL) {
        return  ((unsigned long long)p[0] << 56) | ((unsigned long long)p[1] << 48) |
            ((unsigned long long)p[2] << 40) | ((unsigned long long)p[3] << 32) |
            ((unsigned long long)p[4] << 24) | ((unsigned long long)p[5] << 16) |
            ((unsigned long long)p[6] << 8) | (unsigned long long)p[7];
    }
    else {
//TODO        ASSERT(0);
        return 0xFFFFFFFF;
    }
}

static void cpg_deal_data(sifilter_params_t sfilter, const uint8_t* section, size_t size)
{
	uint16_t section_length;
	uint8_t* data = (uint8_t*)section;
	int len = size;
	uint8_t  match[MAX_FILTER_LEN+2] = {0,};
	uint8_t  mask[MAX_FILTER_LEN+2] = {0,};
	uint64_t addressed_to = 0;

#ifdef CONFIG_CARD
	if(FALSE == cpg_smartcard_ready())
		return;
#endif
	while(len > 0) {
		if (GxCas_SiFilter_CheckCrc((uint8_t*)section) == 0){
			section_length = ((data[1] & 0x0F) << 8) + data[2] + 3;
			CAS_DUMP(CASLIB, data, section_length, "invoked: wPid:0x%02x, pbyReceiveData: ",  sfilter.pid);
			CAS_DBG(CASLIB, ", wLen:%d\n", section_length);
			if(sfilter.match[0]==0x80 || sfilter.match[0]==0x81)
				cg_ecm_parser(0, 0, CA_1EC0, data, section_length);
			else if(sfilter.match[0]>=0x82 && sfilter.match[0]<=0x88){
				addressed_to = array_to_longlong(data + 3);
				if (cg_verify_cl_emm_address(addressed_to) == CG_SUCCESS) {
					cg_emm_parser(0, CA_1EC0, 0, data, section_length);
				}
			}
		}
		//return;
		data += section_length;
		len -= section_length;
	}
	if(sfilter.match[0]==0x80 || sfilter.match[0]==0x81){
		CPGDemuxStopFilter(sfilter.pid);
		match[0] = (!(sfilter.match[0]&0x1))|0x80;
		mask[0] = 0xFF;
		CPGDemuxStartFilter(sfilter.pid,match,mask);
	}
	return;
}

#define SECTION_SIZE (64*1024)
void cpgca_task(void* args)
{
	uint8_t *section;/* section  buffer */
	uint32_t length;
	int32_t ret;
	uint8_t i,version;
	unsigned short emm_pid_list[NR_OF_CASID];
	unsigned short ecm_pid_list[CAS_MAX_SESSIONS][NR_OF_CASID];

	sifilter_params_t params;

	if (cpg_lock == 0)
		GxCore_MutexCreate(&cpg_lock);

	if (cpgsrv.mutex == 0)
		GxCore_MutexCreate(&cpgsrv.mutex);

	while(1) {
		if(TRUE ==  cpg_cam_init_ready())
			break;
		GxCore_ThreadDelay(100);
	}

	GxCore_MutexLock(cpg_lock);
	cat = GxCas_Cat_Open(0, TRUE);
	GxCore_MutexUnlock(cpg_lock);

	section = GxCore_Malloc(SECTION_SIZE);
	if( section == NULL ) {
		CAS_ERR(SI, "alloc memory failure!");
		return;
	}

	while(1) {
		cg_cas_tick();
		static uint16_t emm_pid_bak = 0;
		uint16_t emm_pid = 0, emm_change = 0;
		GxCore_MutexLock(cpg_lock);
		if(cat) {
			ret = GxCas_SiFilter_Read(cat,section,SECTION_SIZE,&length);
			if((ret == 0) && (length > 0)){
				if (GxCas_SiFilter_CheckCrc(section) < 0) {
					GxCas_SiFilter_Reset(cat);
					CAS_ERR(SI,"cat crc err reset cat filter!");
					GxCore_MutexUnlock(cpg_lock);
					continue;
				}

				emm_change = 1;
				cg_get_emmp_pid(section, emm_pid_list);
				emm_pid = emm_pid_list[CA_1EC0];

				version = section[5] & 0x3E;
				if (cat) {
					GxCas_Cat_Close(cat);
					cat = 0;
				}
				cat = GxCas_Cat_Open(version,FALSE);
			}
		}
		GxCore_MutexUnlock(cpg_lock);
		if(emm_pid_bak != 0&&emm_change == 1){
			cpg_stop_emm_filter(emm_pid_bak);
			emm_pid_bak = 0;
		}
		if(emm_pid > 0){
			if(emm_pid_bak != emm_pid){
				emm_pid_bak = emm_pid;
			}
			cpg_start_emm_filter(emm_pid);
		}

		static uint16_t ecm_pid_bak = 0;
		uint16_t ecm_pid = 0, ecm_change = 0;
		GxCore_MutexLock(cpg_lock);
		if(pmt){
			ret = GxCas_SiFilter_Read(pmt,section,SECTION_SIZE,&length);
			if((ret == 0) && (length > 0)){
				if (GxCas_SiFilter_CheckCrc(section) < 0) {
					GxCas_SiFilter_Reset(pmt);
					CAS_ERR(SI,"pmt crc err reset pmt filter!");
					GxCore_MutexUnlock(cpg_lock);
					continue;
				}

				ecm_change = 1;
				version = section[5] & 0x3E;
				cg_get_ecm_pid(0, section, ecm_pid_list[0]);
				ecm_pid = ecm_pid_list[0][CA_1EC0];
				GxCas_SiFilter_GetInfo(pmt,&params);
				if (pmt) {
					GxCas_Pmt_Close(pmt);
					pmt = 0;
				}
				pmt = GxCas_Pmt_Open(params.pid,cpgsrv.service_id,version,FALSE);
			}
		}
		GxCore_MutexUnlock(cpg_lock);

		if((ecm_pid_bak != 0) && (ecm_change == 1)) {
			cpg_stop_ecm_filter(ecm_pid_bak);
			ecm_pid_bak = 0;
		}
		if(ecm_pid > 0) {
			GxCas_Service service[CPG_MAX_DESCRAMBLER_NUM];
			memset(service, 0, sizeof(GxCas_Service) * CPG_MAX_DESCRAMBLER_NUM);

			GxCore_MutexLock(cpgsrv.mutex);
			service[0].ecm_pid = ecm_pid;
			service[0].pid = cpgsrv.a_pid;

			service[1].ecm_pid = ecm_pid;
			service[1].pid = cpgsrv.v_pid;

			GxCore_MutexUnlock(cpgsrv.mutex);
			cpg_add_service(GXCAS_SERVICE_AV, service);
			if(ecm_pid_bak != ecm_pid){
				ecm_pid_bak = ecm_pid;
			}
			cpg_start_ecm_filter(ecm_pid);
		}

		for (i = 0; i<MAX_FILTER_COUNT;i++) {
			GxCore_MutexLock(cpg_lock);
			if (cpg_handle[i] == 0) {
				GxCore_MutexUnlock(cpg_lock);
				continue;
			}

			ret = -1;
			sifilter_params_t sfilter = {0};
			ret = GxCas_SiFilter_Read(cpg_handle[i],section,SECTION_SIZE,&length);
			GxCas_SiFilter_GetInfo(cpg_handle[i], &sfilter);

			GxCore_MutexUnlock(cpg_lock);
			//ret == -1 when timeout
			if (ret == 0 && length > 0)
				cpg_deal_data(sfilter, section, length);
			else if (ret == -1)
				cpg_deal_timeout(sfilter,section,length);
		}

		GxCore_ThreadDelay(10);
	}
	GxCore_Free(section);
}

int32_t cpg_switch_channel(uint16_t pmt_pid,uint32_t service_id,uint16_t audio_pid,uint16_t video_pid)
{
	int32_t ret =0;

	GxCas_Service service[CPG_MAX_DESCRAMBLER_NUM];
	memset(service, 0, sizeof(GxCas_Service) * CPG_MAX_DESCRAMBLER_NUM);
	GxCore_MutexLock(cpgsrv.mutex);
	service[0].pid = cpgsrv.a_pid;
	service[1].pid = cpgsrv.v_pid;
	cpg_del_service(GXCAS_SERVICE_AV, service);
	cpgsrv.a_pid = audio_pid;
	cpgsrv.v_pid = video_pid;
	cpgsrv.service_id = service_id;
	GxCore_MutexUnlock(cpgsrv.mutex);

	GxCore_MutexLock(cpg_lock);
	if (cat == 0)
		cat = GxCas_Cat_Open(0, TRUE);

	if (pmt) {
		GxCas_Pmt_Close(pmt);
		pmt = 0;
	}
	pmt = GxCas_Pmt_Open(pmt_pid, service_id, 0, TRUE);
	if(pmt == 0)
		ret = -1;
	GxCore_MutexUnlock(cpg_lock);
	return ret;
}

int32_t cpg_start_emm_filter(uint16_t emm_pid)
{
	uint8_t  match[MAX_FILTER_LEN+2] = {0,};
	uint8_t  mask[MAX_FILTER_LEN+2] = {0,};

	CPGDemuxStopFilter(emm_pid);
	match[0] = 0x82;
	mask[0] = 0xFF;
	CPGDemuxStartFilter(emm_pid,match,mask);
	match[0] = 0x83;
	mask[0] = 0xFF;
	CPGDemuxStartFilter(emm_pid,match,mask);
	match[0] = 0x86;
	mask[0] = 0xFF;
	CPGDemuxStartFilter(emm_pid,match,mask);
	return 0;
}

int32_t cpg_start_ecm_filter(uint16_t ecm_pid)
{
	uint8_t  match[MAX_FILTER_LEN+2] = {0,};
	uint8_t  mask[MAX_FILTER_LEN+2] = {0,};

	CPGDemuxStopFilter(ecm_pid);
	match[0] = 0x80;
	mask[0] = 0xFE;
	CPGDemuxStartFilter(ecm_pid,match,mask);
	return 0;
}

int32_t cpg_stop_emm_filter(uint16_t emm_pid)
{
	CPGDemuxStopFilter(emm_pid);
	return 0;
}

int32_t cpg_stop_ecm_filter(uint16_t ecm_pid)
{
	CPGDemuxStopFilter(ecm_pid);
	return 0;
}

int32_t cpg_stop_all_filter(void)
{
	uint8_t i = 0;

	GxCore_MutexLock(cpg_lock);
	for (i = 0; i< MAX_FILTER_COUNT;i++) {
		if(cpg_handle[i] != 0){
			GxCas_SiFilter_Stop(cpg_handle[i]);
			cpg_handle[i] = 0;
		}
	}
	GxCore_MutexUnlock(cpg_lock);

	return 0;
}
