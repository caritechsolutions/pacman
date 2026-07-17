#include <string.h>
#include <stdio.h>
#include "dvbhal/gxdemux_hal.h"
#include "gxepg.h"
#include "si/si_public.h"
#include "epg_private.h"

#define EQ_MATCH    (1)
#define NEQ_MATCH   (0)

#define SW_BUFFER_DEFAULT_SIZE 64*1024

extern int32_t des_short_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_extended_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_parental_rating(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t des_eit_tms_event(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t eit_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
extern int32_t eit_lp_head_parser(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

enum
{
	EPG_TID_4E,
	EPG_TID_4F,
	EPG_TID_50,
	EPG_TID_60,
	EPG_TID_51,
	EPG_TID_61,
	EPG_TID_52,
	EPG_TID_62,
	EPG_TID_53,
	EPG_TID_63,
};

enum
{
	EIT_SHORT_EVENT_DESCRIPTOR               = 0x4d,
	EIT_EXTENDED_EVENT_DESCRIPTOR            = 0x4e,
	EIT_TIME_SHIFTED_EVENT_DESCRIPTOR        = 0x4f,
	EIT_PARENTAL_RATING_DESCRIPTOR           = 0x55,
};

typedef int32_t (*eit_des_parser)(uint8_t tag, uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
typedef int32_t (*eit_head_cb)(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);
typedef int32_t (*eit_loop_head_cb)(uint8_t *p_section_data, uint16_t len, uint8_t *p_parsed_data);

struct des_info
{
	uint32_t	tag;
	eit_des_parser des_parser_cb;
};

struct eit_section_ctrl
{
	uint8_t			    sec_hlen;		///<section head data length .
	uint8_t			    sec_lp_hlen;	///<section loop head data length .
	uint16_t			des_cnt;
	struct des_info*	desc_info;
	eit_head_cb		    sec_hcb;		///<sector head callback .
	eit_loop_head_cb	sec_lp_hcb;		///<loop head callback .
};

static struct des_info des_eit[] = {
	{EIT_SHORT_EVENT_DESCRIPTOR, des_short_event},
	{EIT_EXTENDED_EVENT_DESCRIPTOR, des_extended_event},
	{EIT_PARENTAL_RATING_DESCRIPTOR, des_parental_rating},
	{EIT_TIME_SHIFTED_EVENT_DESCRIPTOR, des_eit_tms_event},
};

#define DES_SIZEOF(ptr) (sizeof(ptr)/sizeof(ptr[0]))
static struct eit_section_ctrl eit_ctrl = {
	14,
	12,
	DES_SIZEOF(des_eit),
	des_eit,
	eit_head_parser,
	eit_lp_head_parser
};

// epg table id
static const uint32_t c_EpgTableId[EPG_TID_NUM] =
{0x4E, 0x4F, 0x50, 0x60, 0x51, 0x61, 0x52, 0x62, 0x53, 0x63};

// record the channel create number, one create one release
static uint32_t s_EpgChannelStatus = 0;

//record epg subtable ctrl
GxEpgSubtCtrl s_EpgSubtCtrl;

bool s_EpgPrintEnable = false;

uint32_t GxEpg_SubtChannelNumGet(void)
{
	int32_t i = 0;
	uint32_t channel_count = 0;
	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;

	for(i = 0; i < EPG_TID_NUM; i++)
	{
		if(p_subt_info[i].subtHandle)
			channel_count++;
	}

	return channel_count;
}

void GxEpg_SubtChannelCreate(GxMsgProperty_EpgCreate epg_cfg)
{
	uint16_t i, ts_id, service_id;
	uint16_t max_epg_tid_idx = 0;
	uint32_t table_count = 0;
	GxDmxFilterParams params;
	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;

	if (s_EpgChannelStatus != 0)
	{
		EPG_ERR("[epg]  channel already create\n");
		return;
	}

	EPG_DBG("filter start\n");
	//init demux hal
	GxDmxInit();
	GxDmxSetSource(epg_cfg.dmx_id, epg_cfg.ts_src);
	memset(&params, 0, sizeof(GxDmxFilterParams));

	ts_id = epg_cfg.ts_id & 0xffff;
	service_id = epg_cfg.service_id & 0xffff;

	params.pid = EIT_PID;
	params.depth = 1;

	if(epg_cfg.service_id <= 0xffff)
	{
		params.depth = 5;
		params.match[3] = (service_id>>8);
		params.mask[3] = 0xff;
		params.match[4] = (service_id&0xff);
		params.mask[4] = 0xff;
		EPG_DBG("match service_id\n");
	}

	if(epg_cfg.ts_id <= 0xffff)
	{
		params.depth = 10;
		params.match[8] = (ts_id >> 8);
		params.mask[8] = 0xff;
		params.match[9] = (ts_id & 0xff);
		params.mask[9] = 0xff;
		EPG_DBG("match ts_id\n");
	}

	if(epg_cfg.sw_buffer_size < SW_BUFFER_DEFAULT_SIZE)
		params.size = SW_BUFFER_DEFAULT_SIZE;
	else
		params.size = epg_cfg.sw_buffer_size;
	params.flags = params.flags | EQ_FLAG;
	params.flags = params.flags | REPEAT_FLAG;
	params.flags = params.flags | CRC_FLAG;
	params.timeout = 0;

	// accroding the "epg_day" to make sure which tid need get
	if (epg_cfg.epg_day <= 3)
		max_epg_tid_idx = EPG_TID_60;
	else if (epg_cfg.epg_day <= 7)
		max_epg_tid_idx = EPG_TID_61;
	else
		max_epg_tid_idx = EPG_TID_63;

	for (i = 0; i <= max_epg_tid_idx; i++)
	{
		table_count = i;

		if (i%2 == 0)
		{
			if (epg_cfg.cur_tp_only == 1)
				table_count = i/2;

			p_subt_info[table_count].table_id = c_EpgTableId[i];
		}
		else
		{
			// needn't to filter other tp's info
			if (epg_cfg.cur_tp_only == 1)
				continue;
			else
				p_subt_info[table_count].table_id = c_EpgTableId[i];
		}

		params.match[0] = p_subt_info[table_count].table_id;
		params.mask[0] = 0xff;

		// create epg subtable
		p_subt_info[table_count].subtHandle = GxDmxStart(epg_cfg.dmx_id, params);
		EPG_DBG("handle: %d\n", p_subt_info[table_count].subtHandle);

		// have no free channel for subtable
		if(-1 == p_subt_info[table_count].subtHandle || -2 == p_subt_info[table_count].subtHandle)
		{
			p_subt_info[table_count].subtHandle = 0;
			EPG_ERR("epg table id:%x GxDmxStart err!\n", p_subt_info[table_count].table_id);
			break;
		}

		// create one channel success
		s_EpgChannelStatus++;
	}

	return;
}

void GxEpg_SubtChannelFullSpeed(GxEpgSubtInfo *p_subt_info, GxMsgProperty_EpgCreate *epg_cfg)
{
	GxDmxFilterParams params = {0};

	if(NULL == p_subt_info || 0 == p_subt_info->subtHandle)
	{
		EPG_ERR("Epg subt channel has been released\n");
		return;
	}

	GxDmxGetInfo(p_subt_info->subtHandle, &params);

	params.flags = params.flags | EQ_FLAG;
	params.timeout = 0;

	params.match[3] = 0;
	params.mask[3] = 0;
	params.match[4] = 0;
	params.mask[4] = 0;
	params.match[5] = 0;
	params.mask[5] = 0;

	GxDmxStop(p_subt_info->subtHandle);
	p_subt_info->subtHandle = GxDmxStart(epg_cfg->dmx_id, params);
	if(-1 == p_subt_info->subtHandle || -2 == p_subt_info->subtHandle)
	{
		p_subt_info->subtHandle = 0;
		EPG_ERR("epg table id:%x GxDmxStart err!\n", p_subt_info->table_id);
		s_EpgChannelStatus--;
	}
	else
	{
		p_subt_info->status = MATCH_FULL_FILTER;
		epg_cfg->service_id = 0xffffffff;
	}

	return;
}

void GxEpg_SubtChannelStop(uint16_t dmx_id)
{
	uint32_t channel_count = 0, i = 0;
	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;

	EPG_DBG("filter stop\n");
	channel_count = GxEpg_SubtChannelNumGet();

	for (i = 0; i < channel_count; i++)
	{
		if (0 == p_subt_info[i].subtHandle)
		{
			EPG_ERR("[epg] channel index %d already released\n", i);
			continue;
		}

		if (s_EpgChannelStatus != 0)
			s_EpgChannelStatus--;

		GxDmxStop(p_subt_info[i].subtHandle);

		p_subt_info[i].subtHandle = 0;
		p_subt_info[i].status = STOP_FILTER;
		p_subt_info[i].table_id = 0;
	}

	GxDmxClearSource(dmx_id);

	return;
}


void GxEpg_SubtChannelInfoGet(GxMsgProperty_EpgFilterInfo *filter_info)
{
	uint32_t i = 0;
	GxDmxFilterParams params;
	uint32_t channel_count = 0;
	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;

	channel_count = GxEpg_SubtChannelNumGet();
	if(0 == channel_count)
	{
		EPG_ERR("Epg subt channel has been released\n");
		return;
	}

	for(i = 0; (i < channel_count) && (i < filter_info->num); i++)
	{
		memset(&params, 0, sizeof(GxDmxFilterParams));

		GxDmxGetInfo(p_subt_info[i].subtHandle, &params);

		filter_info->filter[i].depth = params.depth;

		if(EQ_FLAG == (params.flags & EQ_FLAG))
			filter_info->filter[i].eq_or_neq = EQ_MATCH;
		else
			filter_info->filter[i].eq_or_neq = NEQ_MATCH;

		memcpy(filter_info->filter[i].match, params.match, 18);
		memcpy(filter_info->filter[i].mask, params.mask, 18);
	}
	filter_info->real_num = i;

	return;
}

void GxEpg_SubtChannelModify(GxMsgProperty_EpgFilterModify *filter_modify, uint16_t dmx_id)
{
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t channel_count = 0;
	GxDmxFilterParams params = {0};
	GxEpgSubtInfo *p_subt_info = s_EpgSubtCtrl.subtInfo;

	channel_count = GxEpg_SubtChannelNumGet();
	if(0 == channel_count)
	{
		EPG_ERR("Epg subt channel has been released\n");
		return;
	}

	for(i = 0; i < filter_modify->num; i++)
	{
		for(j = 0; j < channel_count; j++)
		{
			if(p_subt_info[j].table_id == filter_modify->filter[i].match[0])
			{
				params.depth = filter_modify->filter[i].depth;
				if(EQ_MATCH == filter_modify->filter[i].eq_or_neq)
					params.flags = params.flags | EQ_FLAG;
				else
					params.flags = params.flags & (!EQ_FLAG);
				memcpy(params.match, filter_modify->filter[i].match, 18);
				memcpy(params.mask, filter_modify->filter[i].mask, 18);
				GxDmxStop(p_subt_info[j].subtHandle);
				p_subt_info[j].subtHandle = GxDmxStart(dmx_id, params);
				if(-1 == p_subt_info[j].subtHandle || -2 == p_subt_info[j].subtHandle)
				{
					p_subt_info[j].subtHandle = 0;
					EPG_ERR("epg table id:%x GxDmxStart err!\n", p_subt_info[j].table_id);
					s_EpgChannelStatus--;
					return;
				}
				p_subt_info[j].status = USER_FILTER;
				break;
			}
		}
	}

	return;
}

static int8_t eit_des_cb_idx_get(uint8_t des_tag, struct eit_section_ctrl *p_sec_ctr)
{
	uint8_t i;

	for (i=0; i<p_sec_ctr->des_cnt; i++)
	{
		if (p_sec_ctr->desc_info[i].tag == des_tag)
		{
			return i;
		}
	}

	return -1;
}

/**
 * @brief 根据tag来判断解析哪种描述符
 * @param
 * @Return
 */
static status_t eit_parser_des(uint8_t *p_section_data, int16_t len,
		uint8_t *p_parsed_data, struct eit_section_ctrl *p_sec_ctr)
{
	uint8_t dtag;
	int8_t didx;
	int16_t des_len=0;

	while(len>0)
	{
		dtag = p_section_data[0];
		des_len = p_section_data[1];

		len -= (des_len+2);
		if(len < 0)
			return GXCORE_ERROR;

		didx = eit_des_cb_idx_get(dtag,p_sec_ctr);
		if (didx!=-1)
		{
			if (GXCORE_ERROR == p_sec_ctr->desc_info[didx].des_parser_cb(dtag, p_section_data,des_len ,p_parsed_data))
				EPG_DBG("desc tag: [%x] failed!\n", dtag);
		}
		p_section_data += (des_len+2);
	}

	return GXCORE_SUCCESS;
}

/*
 *param section_data-原始数据, eit_head-用来存储解析出来的eit表头,需要上层开辟
 * */
status_t GxBus_EitHeadParse(uint8_t *section_data, si_info_t *eit_head)
{
	int16_t sec_len;
	struct eit_section_ctrl *p_sec_ctr;

	if(NULL == eit_head)
	{
		EPG_ERR("!!!!parameter error!!!!\n");
		return GXCORE_ERROR;
	}

	memset(eit_head, 0, sizeof(si_info_t));

	// get section length
	sec_len = TODATA12(section_data[1], section_data[2]);

	p_sec_ctr = &eit_ctrl;

	// special protect: 1 the section is dummy, let it go
	if (sec_len-4+3 < p_sec_ctr->sec_hlen)//se len 包含末尾的4字节crc，不包含头三个字节，但是如果没有crc的表就 囧了
		return GXCORE_ERROR;

	// special protect: 2 all table (pat,pmt,sdt,nit,eit,bat) here section_syntax_indicator value is 1
	if ((section_data[1] >> 7) != 1)
		return GXCORE_ERROR;

	eit_head->service_id = TODATA16(section_data[3], section_data[4]);
	eit_head->current_next_indicator = section_data[5] & 0x01;
	eit_head->ver_number = TODATA5(section_data[5]);				/* read the version_number(5) */
	eit_head->sec_number = section_data[6];								/* read the section_number(8) */
	eit_head->last_sec_number = section_data[7];							/* read the last_section_number(8) */
	eit_head->ts_id =  TODATA16(section_data[8], section_data[9]) ;/* read the transport_stream_id(16) */
	eit_head->segment_last_sec_number = section_data[12];

	return GXCORE_SUCCESS;
}

status_t GxBus_EpgParser(uint8_t *section_data, uint32_t len, uint8_t *p_parsed_data)
{
	// include table_id (1) and section_len (2)
#define SECTION_ONLY_APPEND_LEN   (3)
	int16_t des_len;
	int16_t sec_len;
	int16_t loop_len;
	struct eit_section_ctrl *p_sec_ctr;

	if((NULL == section_data) || (0 == len) || NULL == p_parsed_data)
	{
		EPG_ERR("!!!!parameter error!!!!\n");
		return GXCORE_ERROR;
	}

	memset(p_parsed_data, 0, LARGE_BUF_SIZE);

	// get section length
	sec_len = TODATA12(section_data[1], section_data[2]);

	p_sec_ctr = &eit_ctrl;

	// special protect: 1 the section is dummy, let it go
	if (sec_len-4+3 < p_sec_ctr->sec_hlen)//se len 包含末尾的4字节crc，不包含头三个字节，但是如果没有crc的表就 囧了
		return GXCORE_ERROR;

	// special protect: 2 all table (pat,pmt,sdt,nit,eit,bat) here section_syntax_indicator value is 1
	if ((section_data[1] >> 7) != 1)
		return GXCORE_ERROR;

	p_sec_ctr->sec_hcb(section_data, len, p_parsed_data);

	// offset to lp length
	section_data += p_sec_ctr->sec_hlen;
	loop_len = sec_len-p_sec_ctr->sec_hlen-4+3;//4 //== crc .

	// start deal with loop ----------------------------------------------
	while (loop_len > 0)
	{
		// return descriptor length
		des_len = p_sec_ctr->sec_lp_hcb(section_data, len, p_parsed_data);

		// start deal with descriptor
		if (des_len>=0)
		{
			// offset to descriptor
			section_data += p_sec_ctr->sec_lp_hlen;

			eit_parser_des(section_data, des_len, p_parsed_data, p_sec_ctr);
			section_data += des_len;
		}
		else
		{
			// special protect: 3  parse buffer over flow
			return GXCORE_ERROR;
		}

		// calc loop rest len
		loop_len -= (p_sec_ctr->sec_lp_hlen+des_len);
	}

	return GXCORE_SUCCESS;
}
