#ifndef __GX_SEARCH_PRIVATE_H__
#define __GX_SEARCH_PRIVATE_H__
#include "gxadvance_search.h"
#include "sub_system/si_sub_engine/si_sub_engine.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_vct.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_pat.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_pmt.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_sdt.h"
#include "sub_system/si_parse_lib/si_parse_lib_table/si_parse_lib_nit.h"

#define GX_SEARCH_OK  GXCORE_SUCCESS
#define GX_SEARCH_ERR GXCORE_ERROR
//#define GX_SEARCH_DEBUG

#define GXBUS_SEARCH_LANG_PRIORITY "gxsearch_lang_priority"///<搜索时优先存储的语言，如“eng_chn”则第一优先语言为英语第二优先语言为中文，
                                                           ///<不同语言间用_隔开，并且国家代码要符合iso9660规范
#define  GXBUS_SEARCH_LANG_DEFAULT "eng_chn"///<默认英语优先

#define GXBUS_SEARCH_NIT_LOOP "gxsearch_nit_loop"///<nit搜索时是否进行nit循环搜索
#define GXBUS_SEARCH_NIT_LOOP_YES 1///<nit搜索时进行tp的循环搜索
#define GXBUS_SEARCH_NIT_LOOP_NO  0///<nit搜索时不进行tp的循环搜索

#define ADVANCE_SEARCH_PMT_TID      0x02 
#define ADVANCE_SEARCH_SDT_ACTUAL_TS_TID    0x42


//搜索服务状态
#define SEARCH_START    0x01
#define SEARCH_RUNNING  0x02
#define SEARCH_STOP     0x04
#define SEARCH_PAUSE    0x08
#define SEARCH_VCT_TIMEOUT_VALUE    (15000)
#define SEARCH_PAT_TIMEOUT_VALUE    (5000)
#define SEARCH_PMT_TIMEOUT_VALUE    (5000)
#define SEARCH_SDT_TIMEOUT_VALUE     (15000)
#define SEARCH_NIT_TIMEOUT_VALUE     (10000)
#define GX_SEARCH_FINISH 0x7fffffff
#define GX_SEARCH_DBASE_FULL  0x7ffffffe
#define GX_SEARCH_PROG_EXIST  0x7ffffffd
#define GX_SEARCH_PROG_CONDITION_ERR  0x7ffffffc
#define GX_SEARCH_CONTINUE 0x7ffffffb
#define GX_BLIND_SEARCH_DEFAULT_STEP_MHZ	(12)
#define GX_BLIND_SEARCH_DEFAULT_WINDOW_SIZE_K	(45000)
#define GX_BLIND_SEARCH_DEFAULT_LOW_LIMIT_FREQ_MHZ	(950)
#define GX_BLIND_SEARCH_DEFAULT_HIGH_LIMIT_FREQ_MHZ	(2150)
#define     BLIND_SCAN_TP_COUNT         (200)

//DVB业务类型(service_type)
#define  MPEG_1_VIDEO       0x01
#define  MPEG_2_VIDEO       0x02
#define  MPEG_1_AUDIO       0x03
#define  MPEG_2_AUDIO       0x04
#define  H264               0x1b
#define  H265				0x24
#define  AAC_ADTS           0xf
#define  MPEG_4_VIDEO		0x10
#define  AAC_LATM           0x11
#define  AVS                0x42
#define  PRIVATE_PES_STREAM  0x06//出现在pmt中一般是EAC3
#define  LPCM  0x80
#define  AC3  0x81
#define  DTS  0x82
#define  DOLBY_TRUEHD  0x83
#define  AC3_PLUS  0x84
#define  DTS_HD  0x85
#define  DTS_MA  0x86
#define  AC3_PLUS_SEC  0xa1
#define  DTS_HD_SEC  0xa2
//ATSC业务类型
#define SI_RESERVED_SERVICE  0x00
#define SI_VIDEO_SERVICE  0x01
#define SI_AUDIO_SERVICE 0x02
#define SI_VBI_SERVICE  0x03
#define SI_NVOD_REFERENCE_SERVICE  0x04
#define SI_NVOD_TIMESHIFT_SERVICE  0x05
#define SI_MOSAIC_SERVICE  0x06
#define SI_PAL_SERVICE  0x07
#define SI_SECAM_SERVICE  0x08
#define SI_DD2_MAC_SERVICE  0x09
#define SI_FRE_BROADCAST_SERVICE  0x0a
#define SI_NTSC_SERVICE  0x0b
#define SI_DATA_BROADCAST_SERVICE  0x0c
#define SI_COMMON_RESERVED_SERVICE  0x0d
#define SI_RCS_MAP_SERVICE  0x0e
#define SI_RCS_FLS_SERVICE  0x0f
#define SI_DVB_MHP_SERVICE  0x10 

/* 盲扫的各个阶段，用于进度计算*/
#define BLIND_SINGLE_ONE_POLAR           (1)//单本振单极化
#define BLIND_SINGLE_DOUBLE_POLAR_1      (2)//单本振双极化第一个极化
#define BLIND_SINGLE_DOUBLE_POLAR_2      (3)//单本振双极化第二个极化
#define BLIND_OCS_C_ONE_POLAR            (4)//双本振c波段单极化
#define BLIND_OCS_C_DOUBLE_POLAR_1       (5)//双本振c波段双极化第一个极化
#define BLIND_OCS_C_DOUBLE_POLAR_2       (6)//双本振c波段双极化第二个极化
#define BLIND_OCS_KU_ONE_POLAR_LOW       (7)//双本振ku波段单极化low
#define BLIND_OCS_KU_ONE_POLAR_HIGH      (8)//双本振ku波段单极化high
#define BLIND_OCS_KU_DOUBLE_POLAR_LOW1   (9)//双本振ku波段双极化第一个极化low
#define BLIND_OCS_KU_DOUBLE_POLAR_LOW2   (10)//双本振ku波段双极化第二个极化low
#define BLIND_OCS_KU_DOUBLE_POLAR_HIGH1  (11)//双本振ku波段双极化第一个极化high
#define BLIND_OCS_KU_DOUBLE_POLAR_HIGH2  (12)//双本振ku波段双极化第二个极化high


#define GET_DEC32(bcd)  (((bcd>>28)&0x0f)*10000000\
                                + ((bcd>>24)&0x0f)*1000000\
                                + ((bcd>>20)&0x0f)*100000\
                                + ((bcd>>16)&0x0f)*10000\
                                + ((bcd>>12)&0x0f)*1000\
                                + ((bcd>>8)&0x0f)*100\
                                + ((bcd>>4)&0x0f)*10\
                                + (bcd&0x0f))
#define GET_DEC16(bcd)  (((bcd>>12)&0x0f)*1000\
                                + ((bcd>>8)&0x0f)*100\
                                + ((bcd>>4)&0x0f)*10\
                                + (bcd&0x0f))


#define VCT_NAME_LEN    (14)
//将ATSC协议中定义的name长度和数据库定义的name长度比较，取较小的值
#define MIN_VCT_PM_NAME_LEN  ((VCT_NAME_LEN>MAX_PROG_NAME)?MAX_PROG_NAME:VCT_NAME_LEN) 

#define SEARCH_BASE_PRINTF(...) gxlogd( __VA_ARGS__ )
#define GX_BUS_SEARCH_ERRO_PRINTF(msg)\
do{ SEARCH_BASE_PRINTF("\n\n*****search error*****\n");\
    SEARCH_BASE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
    SEARCH_BASE_PRINTF("%s\n",msg);\
    SEARCH_BASE_PRINTF("~~~~~search error~~~~~\n");\
}while(0)   

#define GX_BUS_SEARCH_NORMAL_PRINTF(msg)\
do{\
    SEARCH_BASE_PRINTF("\n\n*****search print*****\n");\
    SEARCH_BASE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
    SEARCH_BASE_PRINTF("%s\n",msg);\
    SEARCH_BASE_PRINTF("~~~~~search print~~~~~\n");\
}while(0)   


#if 0
/* vct指明的业务类型*/   
typedef enum {
    SI_RESERVED_SERVICE = 0x00,
    SI_VIDEO_SERVICE = 0x01,
    SI_AUDIO_SERVICE = 0x02,
    SI_VBI_SERVICE = 0x03,
    SI_NVOD_REFERENCE_SERVICE = 0x04,
    SI_NVOD_TIMESHIFT_SERVICE = 0x05,
    SI_MOSAIC_SERVICE = 0x06,
    SI_PAL_SERVICE = 0x07,
    SI_SECAM_SERVICE = 0x08,
    SI_DD2_MAC_SERVICE = 0x09,
    SI_FRE_BROADCAST_SERVICE = 0x0a,
    SI_NTSC_SERVICE = 0x0b,
    SI_DATA_BROADCAST_SERVICE = 0x0c,
    SI_COMMON_RESERVED_SERVICE = 0x0d,
    SI_RCS_MAP_SERVICE = 0x0e,
    SI_RCS_FLS_SERVICE = 0x0f,
    SI_DVB_MHP_SERVICE = 0x10 
} GxSearchSiServiceType;
#endif
/* ====================================PMT OBJ基类 ================================================*/
typedef struct{
    SiEngineHandle si_pmt;//过滤pmt表的si句柄
    uint16_t    pid;//pmt_pid
    uint16_t    service_id;//为了在添加节目是能找到当前PMT表的PID
}PmtInfo_t;

//开启PMT表过滤，返回GX_SEARCH_OK表示创建成功，返回GX_SEARCH_ERR表示创建失败
typedef status_t    (*PmtCreate )(PmtInfo_t *pmt_info,uint32_t flag);

/* 定义获取到解析好的section数据时的行为,当返回1时将会继续进行表过滤;
 * 如果要停止过滤，需要调用GxSubSystem_SiEngineFree或者GxSubSystem_SiEnginePause,并返回0
 * 如果不注册务必写NULL,此时会执行默认的过滤处理,此函数执行时间不能超过5S.
 * table可以强制转换成本次解析的表结构
 * status中存放了当前的表是否过滤完全
 * si是本次过滤的句柄*/
typedef int32_t     (*SearchTableGet )(SiEngineHandle si, void * table,SiEngineParseStatus status);

//各个表的释放动作，包括释放硬件和在过滤时各自malloc出来的内存空间
typedef void        (*PmtTableRealease )(PmtInfo_t * pmt);

/* 定义超时的行为当返回1时将会继续进行表过滤;
 * 如果返回0，将会停止本次过滤，并且释放本次过滤所有信息,并且清除数据缓冲，但是不会释放硬件
 * 如果不注册务必写NULL，此时会执行默认的超时处理，相当于返回0，函数执行时间不能超过5S
 * si是本次过滤的句柄*/
typedef int32_t     (*SearchTableTimeOut )(SiEngineHandle si);

typedef struct{
    uint32_t    pmt_start_count;//过滤到第几张PMT表
    uint32_t    pmt_finish_count;
    uint32_t    pmt_count;//PMT表总数
    uint32_t    pmt_time_out;//pmt超时时间，单位ms，默认5000MS
    handle_t    pmt_mutex;
    PmtInfo_t   *pmt_info;//

    PmtCreate           pmt_create;
    SearchTableGet      pmt_get;
    PmtTableRealease      pmt_realease;
    SearchTableTimeOut  pmt_time_out_deal;
}ProtocolPmtObj;

/* ================================================VCT OBJ 基类============================= */
//开启VCT表过滤，返回GX_SEARCH_OK表示创建成功，返回GX_SEARCH_ERR表示创建失败
typedef status_t (*VctCrete)(void);
typedef void    (*VctTableRealease )(SiEngineHandle si);

typedef struct{
    uint32_t vct_time_out;
    handle_t vct_mutex;
    SiEngineHandle si_vct;

    VctCrete            vct_create;
    SearchTableGet      vct_get;
    VctTableRealease    vct_realease;
    SearchTableTimeOut  vct_time_out_deal;
}ProtocolVctObj;

/* ================================================SDT OBJ 基类============================= */
typedef status_t (*SdtCreate)(void);
typedef void     (*SdtTableRealease)(SiEngineHandle si);

typedef struct{
    uint32_t sdt_time_out;
    handle_t sdt_mutex;
    SiEngineHandle si_sdt;

    SdtCreate           sdt_create;
    SearchTableGet      sdt_get;
    SdtTableRealease    sdt_release;  
    SearchTableTimeOut  sdt_time_out_deal;
}ProtocolSdtObj;
/* ================================================PAT OBJ 基类============================= */
typedef status_t    (*PatCreate )(void);
typedef void        (*PatTableRealease )(SiEngineHandle si);

typedef struct{
    uint32_t pat_time_out;
    handle_t pat_mutex;
    SiEngineHandle si_pat;

    PatCreate           pat_create;
    SearchTableGet      pat_get;
    PatTableRealease    pat_realease;
    SearchTableTimeOut  pat_time_out_deal;
}ProtocolPatObj;


/* ================================================NIT OBJ 基类============================= */
typedef status_t (*NitCreate)(void * p);
typedef void     (*NitTableRealease)(SiEngineHandle si);

typedef struct{
    uint32_t    nit_time_out;
    SiEngineHandle si_nit;

    NitCreate           nit_create;
    SearchTableGet      nit_get;
    NitTableRealease    nit_realsease;
    SearchTableTimeOut  nit_time_out_deal;
}ProtocolNitObj;

/* ================================================私有 OBJ 基类============================= */

typedef struct{
    SiEngineHandle si_private;//过滤pmt表的si句柄
    uint16_t    pid;//pmt_pid
}PrivateInfo_t;
typedef struct{
    uint32_t    private_start_count;
    uint32_t    parivate_finish_count;
    uint32_t    private_count;
    uint32_t    private_time_out;
    PrivateInfo_t   *private_info;


    PmtCreate           private_create;
    SearchTableGet      private_get;
    PmtTableRealease    private_realease;
    SearchTableTimeOut  private_time_out_deal;
}ProtocolPrivateObj;


/* ATSC协议基类 */
typedef struct {
    ProtocolVctObj vct_obj_info;
    ProtocolPatObj pat_obj_info;
    ProtocolPmtObj pmt_obj_info;
}GxAtscProtocolObj;

/* DVB协议基类 */
typedef struct{
    ProtocolPmtObj pmt_obj_info;
    ProtocolPatObj pat_obj_info;
    ProtocolSdtObj sdt_obj_info;
}GxDvbProtocolObj;

/*//在锁屏时用到的参数，现在先都保留
typedef enum              
{      
    GX_SEARCH_FRONT_S = 0,
    GX_SEARCH_FRONT_T ,
    GX_SEARCH_FRONT_C ,   
    GX_SEARCH_FRONT_DTMB, 
}GxSearchFrontType;       
*/
typedef struct{
/*扩展功能，需要注册表的pid，table id，其他过滤条件和一个私有解析函数*/

#if 0
    GxSearchExtend* search_ext;
    uint32_t           search_ext_num;//扩展解析的表的数量
    uint32_t           search_ext_finish_num;//完成扩展解析的表的数量
    uint32_t           search_ext_start;//1-开始扩展解析 0-停止扩展解析
    
    ProtocolPrivateObj *private_obj;
#endif
    uint32_t           *ext_tp_id;         ///< 哪些tp需要进行本次扩展搜索
    uint32_t           ext_tp_num;          ///< tp的数量
    uint32_t           need_search_ext;          ///< 是否需要扩展过滤
}GxSearchExtObj;


typedef enum
{
	NIT = 0 ,
	NOT_NIT,
}GxMsgNitReplyCode;

typedef struct{
    GxSearchNitSwitch       nit_switch;	///<是否启用nit搜索
	uint32_t                search_nit_tp_count;	///<nit搜索搜到的tp的数量
	uint32_t                *search_nit_tp_id;	///<nit搜索搜到的tp的id空间
	//GxSearchIdBody          search_nit_subtable_id;///<nit的subtable id
    ProtocolNitObj          *nit_obj;
	GxMsgNitReplyCode       nit_flg;///<表明当前搜索的tp是否是nit出来的
    GxSearchNitCheck        nit_tp_private_check;
}GxSearchNitObj;

typedef struct
{
	uint8_t    data_plp_id;
	uint8_t    common_plp_exist;
	uint8_t    common_plp_id;
    uint8_t    reserve;
}GxSearchDvbt2Id;

typedef struct{
    uint32_t dvbt2_id_num;
    uint32_t dvbt2_id_finish;
    GxSearchDvbt2Id* dvbt2_id;
}GxSearchDvbt2Obj;

#if 0
/**pid搜索时指定的各个pid参数,
  可以和s以及c t的搜索结构体搭配使用组合出各种pid搜索方式
  pid搜索时如果各pid的值为0xffffffff将不做对比*/
typedef struct {
	uint32_t video_pid;	///<所要搜索的节目的video pid
	uint32_t audio_pid;	///<所要搜索的节目的audio pid
	uint32_t pcr_pid;	///<所要搜索的节目的pcr pid
} GxPidSearch;
#endif

typedef status_t (*gx_search_tp_lock)(void * searchclass);
typedef status_t (*gx_search_init_params)(void * searchclass,void * params);

typedef enum{
	NORMAL=0,
	BLIND,
}GxFrontendStatus;


typedef struct
{
    uint32_t fre;//khz
    uint32_t symbol_rate;
    GxBlindSearchTpType type;
    fe_modulation_t qpsk;
}GxBlindSearchTp;


typedef struct{
	handle_t search_blind_sem;

	uint32_t BLIND_SCAN_STEP_MHZ;
	uint32_t BLIND_SCAN_WINDOW_SIZE_K;
	uint32_t BLIND_SCAN_LOW_FREQ_MHZ;
	uint32_t BLIND_SCAN_HIGH_FREQ_MHZ;
	handle_t search_module_frontend;


	uint16_t start_fre;
	uint16_t end_fre;
	uint8_t tp_polar;
	uint8_t polar_stage;//用来区别盲扫到哪个极化方向了                          
	uint16_t reserved;

	uint8_t start_flag;//开始调用前端驱动标志                                   
	uint16_t window_total;//每个window包含的步进量                              
	uint8_t blind_status;//盲扫过程中状态控制                                           

	uint16_t window_num;//计算每个window的盲扫进度                              
	uint16_t try_tp_stage;//尝试锁频时的进度加权值                              

	uint16_t tp_stage;//计算进度的一个加权值                                    
	uint16_t window_num_filt;//当结束window搜索，剔除semblable后剩余的tp数量    

	uint16_t local_fre1;
	uint16_t local_fre2;

	uint8_t switch_22k;
	uint8_t polar;
	uint8_t lnb_ocs;//单本振或双本振                                            
	uint8_t lnb_type;//c波段或者ku波段                                          

	uint32_t tp_count;//盲扫到的tp数量                                          

	GxBlindSearchTp* tp;

}GxBlindSearchClass;


typedef struct {
    handle_t    SearchServiceMsgHandle;//search服务句柄
    GxSearchFrontType   front_type;
    GxSearchScanType    scan_type;
    GxSearchType        prog_type;
    GxSearchFtaCas          fta_cas;
    uint16_t                ts_id;
    uint16_t                original_network_id;
    uint16_t                pat_version;
    uint32_t sdt_version;
    handle_t                search_mutex;//目前只用于在获取和修改GxSearchClassObj参数时加锁
    handle_t                list_mutex;//用于对整个服务的同步，包括协议相关和服务相关



	GxTime					start_time;
	GxTime					stop_time;
    //GxSatSearch             params_s;
    GxSearchExtObj          ext;
    GxSearchNitObj          nit;
    GxSearchDvbt2Obj        dvbt2_info;
	GxPidSearch             search_pid_params;

    uint32_t                search_sat_num;
    uint32_t                *search_sat_id;
    uint32_t                search_sat_finish_num;
    uint32_t                search_sat_id_for_prog;
    uint32_t                *search_ts;
    uint32_t                search_ts_cur;

    uint32_t                search_tp_num;
    uint32_t                *search_tp_id;
    uint32_t                search_tp_finish_num;
    uint32_t                search_tp_id_for_prog;

    uint32_t                search_tuner_num_for_prog;

    GxMsgProperty_NewProgGet    search_new_prog_get;
    uint32_t                search_status;
//前端设备句柄
    handle_t                search_device_handle;
    handle_t                search_demux_handle;
    uint32_t                search_demux_id;
	GxBlindSearchClass* blindclass;

//记录搜索到的节目，用于在not save时从数据库中删除
    uint32_t                *search_prog_id_arry;
    uint32_t                search_prog_id_num;
    GxSearchTranscodingFlag     transcodingflag;

/*考虑现在保存节目的流程，取消这几个变量，在获取pmt信息时直接添加到prog中
    uint32_t                search_stream_body_count;
    GxBusPmDataStream       *search_stream_body;

    //uint32_t                audio_count;//不包括ac3,
*/
    gx_search_tp_lock       tp_lock_func;
    gx_search_init_params   init_params_func;

    GxSearchModifyProg      modify_prog;
    GxSearchDiseqc          search_diseqc;
    GxSearchCheckCa         check_ca_fun;///<检测ca系统是否正确
    GxSearchGetSdtInfo      get_sdt_info_func;    
    GxGetOriginalSection pmt_original_parse_fun;
    GxGetOriginalSection sdt_original_parse_fun;
	GxSearchCheckPmtPid		check_pmtpid_fun;
}GxSearchClassObj;

typedef struct{
    uint32_t initflag;
    PROTOCOL_TYPE type;//区分协议 ATSC和DVB
    void* protocol_obj;//兼容GxAtscSearchObj和GxDvbSearchObj,在初始化时malloc分配内存
    GxSearchClassObj  searchclass;//针对search服务，与协议无关，所以直接实例化
    //FrontObj* demod;
}GxSearchServiceObj;

GxBlindSearchClass	blindclassobj;
GxSearchServiceObj  SearchServiceObj;

//typedef  GxSubsystemSiParseVCTloop GxAtscSearchVctInfo;
//typedef  GxSubsystemSiParsePMT  GxAtscSearchPmtInfo;

//VCT中有效数据，需要保存在链表中
typedef struct {
    uint16_t service_id;
    uint16_t major_num;
    uint16_t minor_num;
    uint16_t source_id;
    //uint32_t access_controlled:1;
    
    uint8_t prog_name[VCT_NAME_LEN];
    uint32_t service_type:6;
    GxBusPmDataProgAudioType audio_type:8;
    uint32_t reserve:2;

	GxBusPmDataProgVideoType video_type;
    uint8_t path_select;//cvtc独有
    uint8_t out_of_band;//cvtc独有
}Available_Vct_Data_Obj;

typedef struct {
    uint16_t    service_id;

    //uint8_t     flag_sch:1;
    //uint8_t     flag_pf:1;
    //uint8_t     running_status:3;   
    //uint8_t     flag_free_ca_mode:1;
    uint8_t     service_type;
    uint32_t    reserve:24;

    uint8_t     service_name[MAX_PROG_NAME];
	uint8_t		service_provider_name[MAX_PROG_NAME];
}Available_Sdt_Data_Obj;

//PMT中有效数据，需要保存在链表中,考虑DVB中可以复用所以独立成OBJ
typedef struct {
    GxBusPmDataProgSwitch     scramble_flag : 1; // 节目加密标志位
    GxBusPmDataProgSwitch     ttx_flag      : 1; // ttx是否存在的标志
    GxBusPmDataProgSwitch     subt_flag     : 1; // subt是否存在的标志
    GxBusPmDataProgSwitch     cc_flag       : 1; // cc是否存在的标志
    GxBusPmDataProgSwitch     ac3_flag      : 1; // ac3是否存在的标志
    uint32_t reserve:11;
    uint16_t service_id;

    uint16_t pcr_pid;
    uint16_t pmt_pid;
    uint16_t video_pid;
    uint16_t video_type;
    uint16_t ecm_pid_v;
    uint16_t ac3_pid;
    uint16_t cur_audio_pid;
    uint16_t cur_audio_ecm_pid;

    GxBusPmDataProgAudioType cur_audio_type:8;
    uint8_t audio_count;
    uint16_t cas_id;
}Available_Pmt_Data_Obj;

typedef struct{
    Available_Pmt_Data_Obj pmt_info;
    Available_Vct_Data_Obj vct_info;
}AtscSearchNode_info_t;

typedef struct{
    Available_Pmt_Data_Obj pmt_info;
    Available_Sdt_Data_Obj sdt_info;
}DvbSearchNode_info_t;

typedef struct{
    struct gxlist_head list_head;
    uint32_t program_number;
    uint32_t vct_sdt_flag;
    uint32_t pmt_flag;
    //AtscSearchNode_info_t * prog;
    void * prog;
}SearchLlist_info_t;

status_t gx_search_init_frontend(GxSearchClassObj * searchclass);
status_t gx_search_realease_frontend(GxSearchClassObj * searchclass);

status_t gx_dvb_search_dtmb_tp_lock();
//status_t gx_dvb_search_dtmb_init_params(void *);

status_t gx_atsc_search_t_tp_lock();
//status_t gx_atsc_search_t_init_params(void *);

status_t gx_dvb_search_dvbt2_tp_lock();
//status_t gx_dvb_search_dvbt2_init_params(void *);

status_t gx_atsc_search_c_tp_lock();
//status_t gx_atsc_search_c_init_params(void *);


status_t gx_dvb_blind_search_tp_lock();
status_t gx_dvb_search_sat_tp_lock();
//status_t gx_dvb_search_sat_init_params(void *);

status_t gx_dvb_search_t_tp_lock();
//status_t gx_dvb_search_t_init_params(void *);

status_t gx_search_nit_exsit_check(GxSearchClassObj * searchclass);
status_t Protocol_pat_create();
int32_t Protocol_pat_get(SiEngineHandle si, void * table,SiEngineParseStatus status);
void Protocol_pat_realease(SiEngineHandle si);
int32_t Protocol_pat_time_out_deal(SiEngineHandle si);

status_t Protocol_vct_create();
int32_t Protocol_vct_get(SiEngineHandle si,void * table,SiEngineParseStatus status);
int32_t Protocol_vct_timeout_deal(SiEngineHandle si);
void Protocol_vct_realease(SiEngineHandle si);


status_t Protocol_sdt_create();
int32_t Protocol_sdt_get(SiEngineHandle si,void * table,SiEngineParseStatus status);
int32_t Protocol_sdt_time_out_deal(SiEngineHandle si);
void Protocol_sdt_realease(SiEngineHandle si);

status_t Protocol_nit_create(void * p);
int32_t Protocol_nit_get(SiEngineHandle si,void * table,SiEngineParseStatus status);
int32_t Protocol_nit_time_out_deal(SiEngineHandle si);
void Protocol_nit_release(SiEngineHandle si);

status_t Protocol_pmt_create(PmtInfo_t * pmt , uint32_t flag);
int32_t Protocol_pmt_get(SiEngineHandle si, void * table,SiEngineParseStatus status);
void Protocol_pmt_realease(PmtInfo_t * pmt);
int32_t Protocol_pmt_timeout_deal(SiEngineHandle si);

status_t gx_search_blind_scan_init_start(GxMessage* msg,GxSearchClassObj* blindclass);
status_t gx_search_scan_init_start(GxMessage * Msg,GxSearchClassObj * searchclass);
void gx_search_service_variable_exit_realease(GxSearchClassObj * searchclass);
void gx_search_stop_reply(GxSearchClassObj * searchclass);
status_t gx_search_table_filter_start();
uint32_t gx_blind_get_status(void);
status_t gx_blind_search(GxSearchClassObj* searchclass);


void gx_search_variable_next_tp_init();
status_t gx_dvb_search_normal_start();
void search_merge(void * info,GxSearchClassObj * searchclass);
status_t add_pmt_info(void * prog,GxSubsystemSiParsePMT * pmt,GxSearchClassObj * searchclass);
status_t add_vct_info(AtscSearchNode_info_t * prog,GxSubsystemSiParseVCTloop * vct);
status_t add_sdt_info(DvbSearchNode_info_t * prog,GxSubsystemSiParseSdtInfo * sdt);
void clear_search_list();
void gx_blind_set_status(int32_t status,GxBlindSearchClass* blindclass);

GxSearchClassObj *  gx_search_get_service_paramsptr();

status_t gx_atsc_search_start(GxSearchClassObj * searchclass);
status_t gx_dvb_search_start(GxSearchClassObj * searchclass);
status_t gx_search_blind_get_params(GxSearchClassObj* searchclass);
status_t gx_dvb_blind_scan_init_params(void*p,void* search_params);

void gx_search_sat_tp_reply(uint16_t sat_count,
        uint16_t sat_num,
        uint16_t tp_count,
        uint16_t tp_num,
        uint32_t tp_id,
        GxBusPmDataTP* tp,
        GxBusPmDataSat* sat,
        GxSearchFrontLockFlag lock_flag,
        GxSearchFrontType front_type);

#endif

