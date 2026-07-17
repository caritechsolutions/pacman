#include "../ts/vod_trans_ts_parse.h"
#include "../../include/vod_in_common_def.h"
#include "../../include/vod_trans_api.h"
#include "../../../vod_include/vod_decode_def.h"
#include "gxmedia_module.h"
#include "gx_common.h"

#define SECTION_LENGTH(a,b) (((uint16_t)((a)&0x0F) << 8) | ((uint16_t)(b)))
#define USHORT_LENGTH(a,b)  (((uint16_t)(a) << 8) | ((uint16_t)(b)))
#define PID_LENGTH(a,b)     (((uint16_t)((a)&0x1F) << 8) | ((uint16_t)(b)))

static int find_pat = 0;
static int pmt_ok = 0;
static struct ts_stream_info ts_info;
static GxMediaStc   *stc_handle = NULL;
static GxMediaDemux *dmx_handle = NULL;

static void _ts_stream_info_gxlogd(struct ts_stream_info* tsinfo)
{
	gxlogd("=====> pmt pid %d\n", tsinfo->pmt_pid);
	gxlogd("=====> aud pid %d\n", tsinfo->aud_pid);
	gxlogd("=====> pcr pid %d\n", tsinfo->pcr_pid);
	gxlogd("=====> vid pid %d\n", tsinfo->vid_pid);
	gxlogd("=====> aud type %d\n", tsinfo->aud_type);
	gxlogd("=====> vid type %d\n", tsinfo->vid_type);
}

static int _get_pat_head(uint8_t* pSectionData, PAT_HEAD *pHead)
{

	if(NULL == pSectionData || NULL == pHead){
		return -1;
	}else{
		pHead->m_uTableID = pSectionData[0];
		pHead->m_uSectLen = SECTION_LENGTH(pSectionData[1], pSectionData[2]);
		pHead->m_uTsId = USHORT_LENGTH(pSectionData[3], pSectionData[4]);
		pHead->m_uVersion = (pSectionData[5] & 0x3E) >> 1;
		pHead->m_uLoopLen = pHead->m_uSectLen - 9;
		pHead->m_pLoopData = pSectionData + 8;
	}
	return 0;
}

static int _get_loop_data(unsigned char ** ioData, unsigned short * ioInfoLength, int eType, unsigned char * * opDesc)
{
	unsigned short		length = 0;

	if (*ioInfoLength == 0){
		return FALSE;
	}

	switch(eType)
	{
		case LOOP_DESC:
			length = (*ioData)[1] + 2;		//为什么+2?
			break;
		case LOOP_PAT:
			length = 4;
			break;
		case LOOP_PMT:
		case LOOP_SDT:
			length = SECTION_LENGTH((*ioData)[3], (*ioData)[4]);
			length += 5;
			break;
		case LOOP_NIT:
		case LOOP_BAT:
			length = SECTION_LENGTH((*ioData)[4], (*ioData)[5]);
			length += 6;
			break;
		case LOOP_EIT:
			length = SECTION_LENGTH((*ioData)[10], (*ioData)[11]);
			length += 12;
			break;
		default:
			break;
	}

	// 如果循环内长度为0，则返回FALSE
	if (length == 0)
	{
		return FALSE;
	}

	// 如果循环内长度大于总长度，则返回FALSE
	if (length > *ioInfoLength)
	{
		*ioData += length - *ioInfoLength;
		return FALSE;
	}

	*opDesc = *ioData;

	*ioInfoLength -= length;
	*ioData += length;

	return TRUE;
}

static int _parse_pmt(const uint8_t *Buffer, struct ts_stream_info* info)
{
	int32_t SectionLen, ProgramInfoLen, AllStreamInfoLen,StreamInfoLen;
	const uint8_t *pData;

	if(info == NULL)
		return  -1;

	info->vid_pid = STPTI_InvalidPid();
	info->aud_pid = STPTI_InvalidPid();
	info->pcr_pid = STPTI_InvalidPid();

	SectionLen = Buffer[2];
	info->pcr_pid = (uint32_t)(Buffer[8]&0x1f)<<8;
	info->pcr_pid += (uint32_t)Buffer[9];

	ProgramInfoLen = (uint32_t)((Buffer[10]&0x0f)<<8);
	ProgramInfoLen += Buffer[11];
	AllStreamInfoLen = SectionLen -9 - ProgramInfoLen-4;

	pData = &Buffer[12+ProgramInfoLen];
	while (AllStreamInfoLen>0)
	{
		gxlogd("==========> data %x\n", pData[0]);
		if ((pData[0] == 2) && (info->vid_pid == STPTI_InvalidPid())){
			/*mpeg4以及mpeg2的定义里面 2 代表的是video pid*/
			info->vid_pid = (pData[1]&0x1f)<<8;
			info->vid_pid += pData[2];
			info->vid_type  = VOD_VIDEO_MPEG2;
		}else if((pData[0] == 0x1b) && (info->vid_pid == STPTI_InvalidPid())){
		/*264定义里面 0x1b 代表的是video pid*/
			info->vid_pid = (pData[1]&0x1f)<<8;
			info->vid_pid += pData[2];
			info->vid_type = VOD_VIDEO_H264;
		}else if((pData[0] == 3) && (info->aud_pid == STPTI_InvalidPid())){
			info->aud_pid = (pData[1]&0x1f)<<8;
			info->aud_pid += pData[2];
			info->aud_type = VOD_AUDIO_MPEG1;
		}else if((pData[0] == 4) && (info->aud_pid == STPTI_InvalidPid())){
			info->aud_pid = (pData[1]&0x1f)<<8;
			info->aud_pid += pData[2];
			info->aud_type = VOD_AUDIO_MPEG2;
		}else if(((pData[0]==0xf)||(pData[0] == 0x11)) && (info->aud_pid == STPTI_InvalidPid())){
			info->aud_pid = (pData[1]&0x1f)<<8;
			info->aud_pid += pData[2];
			info->aud_type = VOD_AUDIO_AAC;
		}else if(pData[0] == 0x81){
			info->aud_pid = (pData[1]&0x1f)<<8;
			info->aud_pid += pData[2];
			info->aud_type = VOD_AUDIO_AC3;
		}else{
			gxlogd("========> %d\n", ((pData[1]&0x1f)<<8)+pData[2]);
		}

		if((info->aud_pid != STPTI_InvalidPid()) && (info->vid_pid != STPTI_InvalidPid())){
			break;
		}

		StreamInfoLen = (pData[3]&0x0f)<<8;
		StreamInfoLen += pData[4];
		pData = pData + 5+ StreamInfoLen;
		AllStreamInfoLen = AllStreamInfoLen - 5 - StreamInfoLen;
	}

	if((info->vid_pid == STPTI_InvalidPid())
			|| (info->aud_pid == STPTI_InvalidPid())
			|| (info->pcr_pid == STPTI_InvalidPid())){
		gxlogd("_parse_pmt err\n");
		return -1;
	}else{
		pmt_ok = 1;
		return 0;
	}
	return -1;
}

int32_t _parse_avpid(uint8_t * pTsStream,
		uint16_t streamlen, struct ts_stream_info* session_info)
{
	uint16_t u16TsPid = 0;
	uint8_t u8PayloadUnitStartIndicator = 0;
	uint8_t *pTempData = NULL;
	uint32_t u32Count = 0;
	uint32_t i = 0;
	int ret = -1;
	unsigned char adaption_control = 0;
	unsigned char adaption_len = 0;
	uint8_t * s_pData = NULL;
	uint16_t s_SectLen = 0;
	PAT_HEAD pPatHead = {0};

	if(NULL == pTsStream){
		return -1;
	}

	if(((streamlen%TS_PACKET_LEN) != 0)||(streamlen < TS_PACKET_LEN)){
		return -1;
	}

	u32Count = streamlen/TS_PACKET_LEN;
	for(i=0; i < u32Count; i++){
		if( pTsStream[i*TS_PACKET_LEN] != 0x47 ){
			continue;
		}
		adaption_len = 0;

		u16TsPid = (((uint16_t)(pTsStream[i*TS_PACKET_LEN+1] & 0x1f)) << 8) | ((uint16_t)pTsStream[i*TS_PACKET_LEN+2]);
		u8PayloadUnitStartIndicator = pTsStream[i*TS_PACKET_LEN+1] & 0x40;
		adaption_control = ((pTsStream[i*TS_PACKET_LEN+3] & 0x30)>>4);
		if (adaption_control == 3 ){
			adaption_len = pTsStream[i*TS_PACKET_LEN+4] + 1;
		}else if (adaption_control == 2 ){
			continue;
		}
		if(0 == u16TsPid){
			gxlogd("u16TsPid:%d---adaption control:%d\n",u16TsPid,adaption_control);
		}
		if( 0 == u8PayloadUnitStartIndicator){
			continue;
		}

		s_SectLen = SECTION_LENGTH(pTsStream[i*TS_PACKET_LEN+6+adaption_len], pTsStream[i*TS_PACKET_LEN+7+adaption_len]);
		if(0 == u16TsPid){
			if(find_pat == 1){
				continue;
			}
			if( s_SectLen > 180 ){
				gxlogd(" 1s_SectLen > 180   : %s :%d\n",__FILE__,s_SectLen);
				continue;
			}
			s_pData = xmalloc(s_SectLen + 3);
			memcpy(s_pData, pTsStream+i*TS_PACKET_LEN + 5+adaption_len,  s_SectLen + 3);
			_get_pat_head(s_pData, &pPatHead);
			while (_get_loop_data(&(pPatHead.m_pLoopData), &(pPatHead.m_uLoopLen), LOOP_PAT, &pTempData)){
				session_info->pmt_pid = PID_LENGTH(pTempData[2], pTempData[3]);
				find_pat = 1;
			}

			xfree(s_pData);
			s_pData = NULL;
			s_SectLen = 0;
			continue;
		}else{
			if(find_pat == 0 || session_info->pmt_pid != u16TsPid){
				continue;
			}

			if( s_SectLen > 180 ){
				gxlogd(" 2s_SectLen > 180   : %s\n",__FILE__);
				continue;
			}

			s_pData = xmalloc(s_SectLen + 3);
			memcpy(s_pData, pTsStream+i*TS_PACKET_LEN + 5+adaption_len,  s_SectLen + 3);

			ret = _parse_pmt(s_pData, session_info);
			xfree(s_pData);
			s_pData = NULL;
			s_SectLen = 0;

			if(ret == 0){
				break;
			}else{
				continue;
			}
		}
	}

	return ret;
}

static void _parse_init(void)
{
	find_pat = 0;
	pmt_ok = 0;
	memset(&ts_info, 0, sizeof(struct ts_stream_info));
}

int32_t vod_trans_hwts_open(void)
{
	int ret = 0;
	_parse_init();

	stc_handle = GxMedia_StcOpen();
	dmx_handle = GxMedia_DemuxOpen(GXMEDIA_SOURCE_TS);
	ret = GxMedia_StcConfig(stc_handle, 45000, 2);
	CHECK_RETURN_VALUE(ret);
	ret = GxMedia_StcStart(stc_handle);
	CHECK_RETURN_VALUE(ret);

	return 0;
}

void vod_trans_hwts_close(void)
{
	GxMedia_DemuxStop(dmx_handle);
	GxMedia_StcStop(stc_handle);
	GxMedia_DemuxClose(dmx_handle);
	GxMedia_StcClose(stc_handle);
	dmx_handle = NULL;
	stc_handle = NULL;
	return;
}

uint8_t* vod_trans_hwts_malloc_buffer(int32_t len, uint32_t* token)
{
	uint8_t *pak = NULL;

	pak = xmalloc(len + RTP_PACKET_HEADER_SIZE);
	if (pak == 0){
		gxlogd("hw ts malloc fail, len=%d\n", len);
		return NULL;
	}
	memset(pak, 0, len + RTP_PACKET_HEADER_SIZE);
	((rtp_packet*)pak)->rtp_data_len = len;
	*token = (uint32_t)pak;
	return (pak + RTP_PACKET_HEADER_SIZE);
}

int32_t vod_trans_hwts_insert_buffer(uint32_t token, int32_t state)
{
	rtp_packet* pak = (rtp_packet*)token;
	uint8_t* buffer = NULL;
	int len = 0, ret = 0;

	if (state == 0){
		xfree(pak);
		return 0;
	}

	buffer = (uint8_t*)pak + RTP_PACKET_HEADER_SIZE;
	len = pak->rtp_data_len;
	if(pmt_ok == 0){
		_parse_avpid(buffer, len, &ts_info);
		_ts_stream_info_gxlogd(&ts_info);
		if(pmt_ok == 1){
			GxMedia_DemuxConfig(dmx_handle, -1, ts_info.aud_pid, ts_info.vid_pid, ts_info.pcr_pid);
			GxMedia_DemuxStart(dmx_handle);
		}
	}
	ret = GxMedia_DemuxInjectData(dmx_handle, buffer, len);
	xfree(pak);
	return ret;
}

int32_t vod_trans_hwts_sync(char* data, int len)
{
	int c = 0;
	int synced = 0;
	int pos = 0, first_pos=0;

resync:
	while(((c = data[pos]) != 0x47)){
		pos++;
		if(pos > len)
			return -1;
	}

	first_pos = pos;
	if(c == 0x47) {
		pos += TS_PACKET_LEN;
		if(pos < len){
			if(data[pos] != 0x47){
				pos = first_pos+1;
				goto resync;
			}
			else {
				pos += TS_PACKET_LEN;
				if(pos < len){
					if(data[pos] != 0x47){
						pos = first_pos+1;
						goto resync;
					}
					else {
						synced = 1;
					}
				}else{
					if(len == pos)
						synced = 1;
					else
						synced = 0;
				}
			}
		}else{
			if(len == pos)
				synced = 1;
			else
				synced = 0;
		}
	}
	else
		synced = 0;

	if(synced == 0)
		return -1;

	//gxlogd("[%s-%d]: find ts sync pos %d\n", __FUNCTION__, __LINE__, first_pos);
	return first_pos;
}

int32_t vod_trans_hwts_demux(uint32_t *handle)
{
	*handle = (uint32_t)dmx_handle;

	return 0;
}

int32_t vod_trans_hwts_get_info(struct ts_stream_info* tsinfo)
{
	if(pmt_ok){
		memcpy(tsinfo, &ts_info, sizeof(struct ts_stream_info));
		return 0;
	}
	return -1;
}
