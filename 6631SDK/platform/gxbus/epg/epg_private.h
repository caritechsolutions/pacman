#ifndef __EPG_PRIVATE_H
#define __EPG_PRIVATE_H

extern bool s_EpgPrintEnable;
#define EPG_ERR(fmt, args...)  do {                                 \
	gxloge("\033[31m"fmt"\033[0m\n", ##args);                       \
} while(0)

#define EPG_DBG(fmt, args...)  do {                                 \
	if (s_EpgPrintEnable) {                                         \
		gxlogd("\033[33m"fmt"\033[0m\n", ##args);                   \
	}                                                               \
} while(0)

/*LARGE_BUF_SIZE must be greater then SI_EIT_BUF_SIZE*/
#define LARGE_BUF_SIZE  (SI_EIT_BUF_SIZE + 1024)
#define EPG_TID_NUM     (10)

typedef enum {
	STOP_FILTER = 0,
	START_FILTER ,
	MATCH_SERV_ID_FILTER,
	MATCH_FULL_FILTER,
	UNMATCH_FILTER,
	USER_FILTER
} filterStatus;


/*
 * 查询一个event是否重复相关结构体*/
typedef struct{
	uint32_t service_id;
	char sec_number[256];
}sec_index;

typedef struct{
	uint32_t ts_id;
	uint32_t count;
	sec_index* service;
}ts_index;

typedef struct{
	uint32_t count;
	uint32_t table_id;
	ts_index* ts;
}sec_status;

typedef struct{
	sec_status sectionStatus[EPG_TID_NUM];
	handle_t mutex;
}GxEpgSectionCtrl;

typedef struct{
	handle_t subtHandle;
	filterStatus status;
	uint32_t table_id;
}GxEpgSubtInfo;

typedef struct{
	GxEpgSubtInfo subtInfo[EPG_TID_NUM];
	handle_t mutex;
}GxEpgSubtCtrl;

/*
 * cache相关结构体*/
typedef struct
{
	uint32_t size;
	uint8_t* buffer;
	void* pre;
	void* next;
} GxEpgCache;

typedef struct
{
	void* head;
	void* tail;
	uint8_t cache_count;
	uint8_t cache_gate;
	handle_t mutex;
} GxEpgCacheCtrl;

extern status_t GxBus_EpgParser(uint8_t *section_data, uint32_t len, uint8_t *p_parsed_data);
extern status_t GxBus_EitHeadParse(uint8_t *section_data, si_info_t *eit_head);

extern void GxEpg_SubtChannelCreate(GxMsgProperty_EpgCreate epg_cfg);
extern void GxEpg_SubtChannelStop(uint16_t dmx_id);
extern void GxEpg_SubtChannelModify(GxMsgProperty_EpgFilterModify *filter_modify, uint16_t dmx_id);
extern void GxEpg_SubtChannelInfoGet(GxMsgProperty_EpgFilterInfo *filter_info);
extern void GxEpg_SubtChannelFullSpeed(GxEpgSubtInfo *p_subt_info, GxMsgProperty_EpgCreate *epg_cfg);
extern uint32_t GxEpg_SubtChannelNumGet(void);

extern GxEpgSubtCtrl s_EpgSubtCtrl;

#endif
