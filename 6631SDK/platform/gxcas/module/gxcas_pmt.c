#include "gxcas_dbg.h"
#include "gxcas_demux.h"
#include "gxcas_psi.h"

#define GXCAS_DATA3TO15(data1, data2) ((data1 & 0x1f)<< 8 | data2)
#define GXCAS_DATA4TO15(data1, data2) ((data1 & 0xf)<< 8 | data2)
#define GXCAS_DATA0TO15(data1, data2) ((data1 << 8) | data2)

static CheckCAId pmt_check_ca = NULL;

void GxCas_Pmt_Init(CheckCAId docheck)
{
	if(docheck)
		pmt_check_ca = docheck;
	else
		CAS_ERR(SI,"pmt init err, pls check param!");

}

int32_t GxCas_Pmt_Parse(uint8_t *section, uint32_t size,GxCas_EcmInfo *ecminfo)
{
	uint8_t *pinfo;
	int32_t sctlen = 0;//section段总长度
	int32_t infolen;
	int32_t parsedlen=0;//已经解析完成的section段长度
	int32_t descriptorLen = 0;//当前解析的section段长度

	sctlen = ((section[1]&0x0f)<<8) + section[2] + 3;
	if (sctlen  > 1024) {
		CAS_ERR(SI,"sctlen=%d#", sctlen);
		return -1;
	}

	infolen = ((section[10] & 0xF)<<8)|(section[11]);

	if (infolen>=sctlen) {
		CAS_ERR(SI,"infolen=%d sctlen=%d.",infolen,sctlen);
		return -1;
	}

	pinfo = section+12;
	if (infolen>0) {
		while(parsedlen < infolen) {
			descriptorLen = pinfo[1]+2;
			if (descriptorLen>infolen-parsedlen) {
				CAS_ERR(SI,"descriptorLen=%d infolen-parsedlen=%d.",descriptorLen,(infolen-parsedlen));
				return -1;
			}

			if(pinfo[0] == CA_DESCRIPTOR_TAG) {//同密
				uint16_t caSystemId = 0;
				uint16_t ecmPid = PSI_INVALID_PID;
				caSystemId = (pinfo[2] << 8) + pinfo[3];
				ecmPid = ((pinfo[4] & 0x1F) << 8) + pinfo[5];
				if (TRUE == pmt_check_ca(caSystemId, PSI_INVALID_PID) && (ecmPid != PSI_INVALID_PID)) {
					ecminfo->ecm_pid[ecminfo->ecm_count] = ecmPid;
					ecminfo->pid[ecminfo->ecm_count] = PSI_INVALID_PID;
					ecminfo->ecm_count++;
				}
			}
			pinfo += descriptorLen;
			parsedlen += descriptorLen;
		}
	}

	int32_t es_info_len = sctlen - 12 - 4 - infolen;//ES INFO
	uint8_t *pesinfo = section + 12 + infolen;
	while (es_info_len > 0) {
		uint16_t pid = GXCAS_DATA3TO15(pesinfo[1], pesinfo[2]);
		uint16_t eslen = GXCAS_DATA4TO15(pesinfo[3], pesinfo[4]);
		pesinfo += 5;
		if (eslen > es_info_len)
			break;

		while(eslen > 0 && eslen < es_info_len) {
			if(pesinfo[0] == CA_DESCRIPTOR_TAG) {//不同密
				uint16_t caSystemId = 0;
				uint16_t ecmPid = PSI_INVALID_PID;
				caSystemId = GXCAS_DATA0TO15(pesinfo[2], pesinfo[3]);
				ecmPid = GXCAS_DATA3TO15(pesinfo[4], pesinfo[5]);
				if (pmt_check_ca(caSystemId, pid) == TRUE && ecmPid != PSI_INVALID_PID) {
					ecminfo->ecm_pid[ecminfo->ecm_count] = ecmPid;
					ecminfo->pid[ecminfo->ecm_count] = pid;
					ecminfo->ecm_count++;
				}
			}
			eslen -= (pesinfo[1] + 2);
			pesinfo += (pesinfo[1] + 2);
		}
		es_info_len -= (eslen + 5);
	}

	return 0;
}


handle_t GxCas_Pmt_Open(uint16_t pid,uint32_t service_id,uint8_t version,uint8_t versionEQ)
{
	handle_t pmt_handle;
	sifilter_params_t filter = {0};

	CAS_DBG(SI,"[GxCas_Pmt_Open]pid(0x%x); servId(0x%x).", pid, service_id);
	filter.match[0] = PMT_TID;
	filter.mask[0] = 0xff;
	filter.match[3] = (service_id>>8)&0xff;
	filter.mask[3] = 0xff;
	filter.match[4] = service_id&0xff;
	filter.mask[4] = 0xff;

	if (FALSE== versionEQ) {
		filter.match[5] = version;
		filter.mask[5] = 0x3E;
		filter.depth = 6;
	} else
		filter.depth = 5;

	filter.pid = pid;
	filter.flags |= CRC_FLAG;
	filter.flags |= REPEAT_FLAG;

	if(versionEQ == TRUE)
		filter.flags |= EQ_FLAG;

	pmt_handle = GxCas_SiFilter_Start(filter);
	if (pmt_handle == -1)
		return 0;

	return pmt_handle;
}

int32_t GxCas_Pmt_Close(handle_t handle)
{
	return GxCas_SiFilter_Stop(handle);
}

