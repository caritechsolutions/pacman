#ifndef __GX_ATSC_EPG_PRIVATE_H__
#define __GX_ATSC_EPG_PRIVATE_H__
#include "gxatsc_epg.h"
#include "common/list.h"
#include "module/epg/gxepg_manage.h"
#include "sub_system/si_sub_engine/si_sub_engine.h"

typedef struct{
	SiEngineHandle si;        ///< psip过滤通道的句柄
	uint16_t pid;             ///< 对应表的pid
	uint8_t  tid;             ///< 对应表的table id
	uint8_t    eq_or_neq;     ///< 相等过滤还是不等过滤
	TableSection table;       ///< section完成的回调
	TimeOut timeout;          ///< 过滤超时回调
	uint32_t    version_number;     ///< 版本号不等过滤的时候用
	void *p;
}PsipInfo_t;

typedef struct{
	struct gxlist_head list_head;
	GxBusEpgEventAddInfo *event;
}EventList_t;

typedef struct{
	struct gxlist_head list_head;
    uint32_t real_count;
    GxEpgEventInfo *epg_info;
}EpgInfoList_t;
typedef struct{
	bool       init;
	handle_t   mutex;
	handle_t   s_EpgServiceHandle;
	/*mgt*/
	PsipInfo_t mgt_info;
	/*eit*/
	uint32_t   eit_count;
	uint32_t   eit_start_count;
	uint32_t   eit_finish_count;
	PsipInfo_t  *eit_info;
	/*ett*/
	uint32_t   ett_count;
	uint32_t   ett_start_count;
	uint32_t   ett_finish_count;
	PsipInfo_t  *ett_info;

	uint32_t   one_shot_flag;//表示一次ATSC EPG过滤完成标志(EIT_OK|ETT_OK)
	GxMsgProperty_AtscEpgCreate create_info;
}AtscEpgService_t;

extern AtscEpgService_t g_aes;

void service_epg_get(GxMsgProperty_AtscEpgInfo *egp_get);
void service_epg_free(GxMsgProperty_AtscEpgInfo *epg_free);
void service_epg_create(GxMsgProperty_AtscEpgCreate *epg_create);
void service_epg_clean(void);
void service_epg_release(void);
#endif

