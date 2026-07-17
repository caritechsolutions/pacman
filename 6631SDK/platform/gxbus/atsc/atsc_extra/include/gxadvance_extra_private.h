#ifndef __GXADVANCE_EXTRA_PRIVATE_H__
#define __GXADVANCE_EXTRA_PRIVATE_H__

#include "gxadvance_extra.h"
#define ADVANCE_EXTRA_PRINTF(...) gxlogd( __VA_ARGS__ )

#define ADVANCE_EXTRA_ERR_PRINTF(msg)\
    do{\
        ADVANCE_EXTRA_PRINTF("\n\n*****dmx sub system error*****\n");\
        ADVANCE_EXTRA_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
        ADVANCE_EXTRA_PRINTF("%s\n",msg);\
        ADVANCE_EXTRA_PRINTF("~~~~~dmx sub system error~~~~~\n");\
    }while(0) 


typedef status_t    (*GxAdvanceExtraTableCreate )(uint32_t ts,uint32_t dmx_id);
typedef void        (*GxAdvanceExtraTableRealease )(SiEngineHandle si);
typedef int32_t     (*GxAdvanceExtraTableGet )(SiEngineHandle si, void * table,SiEngineParseStatus status);
typedef int32_t     (*GxAdvanceExtraTableTimeOut )(SiEngineHandle si);

typedef struct{
    SiEngineHandle si_stt;

    GxAdvanceExtraTableCreate    stt_create; 
    GxAdvanceExtraTableGet       stt_get;
    GxAdvanceExtraTableTimeOut   stt_time_out_deal;
    GxAdvanceExtraTableRealease  stt_release;
}ProtocolSttObj;

typedef struct{
    SiEngineHandle si_tot;

    GxAdvanceExtraTableCreate    tot_create; 
    GxAdvanceExtraTableGet       tot_get;
    GxAdvanceExtraTableTimeOut   tot_time_out_deal;
    GxAdvanceExtraTableRealease  tot_release;
}ProtocolTotObj;

typedef struct{
    SiEngineHandle si_tdt;

    GxAdvanceExtraTableCreate    tdt_create; 
    GxAdvanceExtraTableGet       tdt_get;
    GxAdvanceExtraTableTimeOut   tdt_time_out_deal;
    GxAdvanceExtraTableRealease  tdt_release;
}ProtocolTdtObj;

typedef struct{
    ProtocolTotObj tot_obj_info;
    ProtocolTdtObj tdt_obj_info;
}GxAdvanceExtraDvbProtocol;

typedef struct{
    ProtocolSttObj stt_obj_info;
}GxAdvanceExtraAtscProtocol;

typedef struct{
    handle_t s_AdvanceExtraHandle;
    PROTOCOL_TYPE protocol;
    handle_t mutex;
    void * protocol_obj;
}GxAdvanceExtraServiceObj;

GxAdvanceExtraServiceObj AdvanceExtraServiceObj;
void gxadvance_extra_protocol_dvb_release();
void gxadvance_extra_protocol_atsc_release();
void extra_release_resource();
uint32_t gxadvance_extra_protocol_dvb_init();
uint32_t gxadvance_extra_protocol_atsc_init();
void extra_sync_time(GxMsgProperty_ExtraSyncTime *);



void Protocol_tot_release(SiEngineHandle si);
int32_t Protocol_tot_time_out_deal(SiEngineHandle si);
int32_t Protocol_tot_get(SiEngineHandle si, void * table,SiEngineParseStatus status);
int32_t Protocol_tot_create(uint32_t ts_id,uint32_t dmx_id);

void Protocol_tdt_release(SiEngineHandle si);
int32_t Protocol_tdt_time_out_deal(SiEngineHandle si);
int32_t Protocol_tdt_get(SiEngineHandle si, void * table,SiEngineParseStatus status);
int32_t Protocol_tdt_create(uint32_t ts_id,uint32_t dmx_id);

void Protocol_stt_release(SiEngineHandle si);
int32_t Protocol_stt_time_out_deal(SiEngineHandle si);
int32_t Protocol_stt_get(SiEngineHandle si, void * table,SiEngineParseStatus status);
int32_t Protocol_stt_create(uint32_t ts_id,uint32_t dmx_id);

#endif
