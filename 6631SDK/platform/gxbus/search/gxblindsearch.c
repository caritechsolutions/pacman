/************************************************************
Copyright (C), 2007-2009, GX S&T Co., Ltd.
FileName   :	search_Service.c
Author     : 	zhangling
Version    : 	1.0
Date	   :
Description:
Version    :
History    :
Date				Author  	Modification
2007.03.14     zhangling		 create
***********************************************************/

/* Includes --------------------------------------------------------------- */
#ifdef MEMWATCH
#include <common/memwatch.h>
#else
#include <stdlib.h>
#endif
#include <stdio.h>
#include "gxavdev.h"
#include "service/gxsearch.h"
#include "gxcore.h"
#include "service/gxsi.h"
#include "module/frontend/gxfrontend_module.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include "module/config/gxconfig.h"
#include "gdi_core.h"
#include "search_private.h"

/* Exported Macros -------------------------------------------------------- */
#define MAX_PROG_PER_TP (300)
/* Exported Types --------------------------------------------------------- */
/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */

/* Private Macros --------------------------------_------------------------ */
#define GX_SEARCH_OK  GXCORE_SUCCESS
#define GX_SEARCH_ERR GXCORE_ERROR
#define GX_SEARCH_FINISH 0x7fffffff
#define GX_SEARCH_DBASE_FULL  0x7ffffffe
#define GX_SEARCH_PROG_EXIST  0x7ffffffd
#define GX_SEARCH_PROG_CONDITION_ERR  0x7ffffffc
#define GX_SEARCH_CONTINUE	(0x7ffffffb)

//stream type
#define  MPEG_1_VIDEO		0x01
#define  MPEG_2_VIDEO		0x02
#define  MPEG_1_AUDIO		0x03
#define  MPEG_2_AUDIO		0x04
#define  H264				0x1b
#define	 H265				0x24
#define  AAC_ADTS			0xf
#define  MPEG_4_VIDEO		0x10
#define  AAC_LATM			0x11
#define  AVS				0x42
#define	 PRIVATE_PES_STREAM  0x06//出现在pmt中一般是EAC3
#define	 LPCM  0x80
#define	 AC3  0x81
#define	 DTS  0x82
#define	 DOLBY_TRUEHD  0x83
#define	 AC3_PLUS  0x84
#define	 DTS_HD  0x85
#define	 DTS_MA  0x86
#define	 AC3_PLUS_SEC  0xa1
#define	 DTS_HD_SEC  0xa2

//搜索的各种状态
#define SEARCH_START 		0x1
#define SEARCH_RUNNING		0x2
#define SEARCH_STOP			0x4
#define SEARCH_PAUSE		0x8

#define SEARCH_NIT_CREATE 	0x10
#define SEARCH_NIT_TIMEOUT 	0x20
#define SEARCH_NIT_RECEIVED	0x40
#define SEARCH_NIT_STOPING	0x80
#define SEARCH_NIT_RELEASED	0x100

#define SEARCH_PAT_CREATE 	0x200
#define SEARCH_PAT_TIMEOUT 	0x400
#define SEARCH_PAT_RECEIVED	0x800
#define SEARCH_PAT_STOPING	0x1000
#define SEARCH_PAT_RELEASED	0x2000

#define SEARCH_SDT_CREATE 	0x4000
#define SEARCH_SDT_TIMEOUT	0x8000
#define SEARCH_SDT_RECEIVED	0x10000
#define SEARCH_SDT_STOPING	0x20000
#define SEARCH_SDT_RELEASED	0x40000

#define SEARCH_PMT_CREATE 	0x80000
#define SEARCH_PMT_TIMEOUT	0x100000
#define SEARCH_PMT_RECEIVED	0x200000
#define SEARCH_PMT_STOPING	0x400000
#define SEARCH_PMT_RELEASED	0x800000

#define SEARCH_STOPED			0x1000000

#define SEARCH_STATUS_FALSE	0

#define SEARCH_PAT_TIMEOUT_VALUE	(5000)
#define SEARCH_SDT_TIMEOUT_VALUE	(15000)
#define SEARCH_NIT_TIMEOUT_VALUE	(10000)
#define SEARCH_PMT_TIMEOUT_VALUE	(5000)

/*盲扫的各个阶段，用于进度计算*/
#define BLIND_SINGLE_ONE_POLAR           (1)//单本振单极化?
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

#define FILTER_MAX 64
#define BLIND_SEARCH_SEM_WAIT_TIMEOUT_VALUE	(1000)

//#ifdef __DEBUG
//#define GX_BUS_BLIND_SEARCH_DBUG
//#endif

#define BLIND_SEARCH_BASE_PRINTF(...) gxlogd( __VA_ARGS__ )

#define GX_BUS_BLIND_SEARCH_ERRO_PRINTF(msg)\
	do{\
		BLIND_SEARCH_BASE_PRINTF("\n\n*****blind search error*****\n");\
		BLIND_SEARCH_BASE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
		BLIND_SEARCH_BASE_PRINTF("%s\n",msg);\
		BLIND_SEARCH_BASE_PRINTF("~~~~~blind search error~~~~~\n");\
	}while(0)

#define GX_BUS_BLIND_SEARCH_NORMAL_PRINTF(msg)\
	do{\
		BLIND_SEARCH_BASE_PRINTF("\n\n*****blind search print*****\n");\
		BLIND_SEARCH_BASE_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
		BLIND_SEARCH_BASE_PRINTF("%s\n",msg);\
		BLIND_SEARCH_BASE_PRINTF("~~~~~blind search print~~~~~\n");\
	}while(0)

/* Private Types ---------------------------------------------------------- */

/*pat表的内容*/
typedef struct {
	uint16_t                prog_number;	///<相当于service id
	uint16_t                pmt_pid;
	uint16_t                ts_id;
	uint32_t 				pat_version;
} GxSearchPatBody;

/*sdt表的内容*/
typedef struct {
	uint8_t                 desc_valid;
	uint8_t                 service_type;
	uint8_t                 service_name[MAX_PROG_NAME];
	uint8_t                 service_provider_name[MAX_PROG_NAME];
} GxSearchSdtServiceDesc;
typedef struct {
	uint8_t stream_content_ext :4; /* in combination with the stream_content field specifies the type of stream */
	uint8_t stream_content     :4; /* in combination with the stream_content_ext field specifies the type of stream */
	uint8_t component_type;        /* specifies the type of the component */
	uint8_t component_tag;         /* same value as the component_tag field in the stream identifier descriptor */
	uint8_t iso639[3];             /* identifies the language of the component */
} GxSearchSdtComponentDesc;
typedef struct {
	uint8_t                 desc_valid;
	uint16_t                ca_system_id;
} GxSearchSdtCaIdentifierDesc;
typedef struct {
	uint8_t                 desc_valid;
	uint32_t                iso639_code;
	uint8_t                 service_name[MAX_PROG_NAME];
} GxSearchSdtMultServiceNameDesc;
typedef struct {
	uint16_t                 service_id;
	uint16_t                 ts_id;	//用来和service id唯一的标明一个service 需要保存到节目信息中
	uint8_t                  flag_sch:1;
	uint8_t                  flag_pf:1;
	uint8_t                  running_status:3;	//nvod里面用到
	uint8_t                  flag_free_ca_mode:1;	//0 表示没有加绕,但是prog是否加密的判断一般使用pmt body
	uint8_t                  reserved:2;
	GxSearchSdtServiceDesc   service_desc;
	GxSearchSdtComponentDesc component_desc;
	GxSearchSdtCaIdentifierDesc ca_identifier_desc;
	uint8_t                 multi_service_name_count;
	uint32_t   orig_network_id;

	GxSearchSdtMultServiceNameDesc
		multi_service_name_desc[SI_MAX_MULTI_NAME_COUNT];
	uint32_t 				sdt_version;
} GxSearchSdtBody;

/*pmt表的内容*/

typedef struct {
	uint32_t                ttx_desc_valid:1;
	uint32_t                cc_desc_valid:1;
	uint32_t                teletext_type:5;
	uint32_t                teletext_magazine_number:3;
	uint32_t                teletext_page_number:8;
	uint32_t                ttx_pid:13;
	uint32_t                reserved0:1;
} GxSearchPmtTeletextDesc;

typedef struct {
	uint8_t                 desc_valid:1;
	uint8_t                 reserved:7;
	uint8_t                 subtitling_type;
	uint16_t                composition_page_id;
	uint16_t                ancillary_page_id;
} GxSearchPmtSubtitlingDesc;

typedef struct {
	uint8_t                 desc_valid:1;
	uint8_t                 reserved:7;
	uint16_t                new_original_network_id;
	uint16_t                new_transport_streamId;
	uint16_t                new_service_id;
} GxSearchPmtServiceMoveDesc;

typedef struct {
	uint8_t                 desc_valid;
	uint16_t                cas_id;
	uint16_t                ecm_id;
} GxSearchPmtCaDesc;

typedef struct {
	uint16_t                desc_valid:1;
	uint16_t                ac3_pid:13;
	uint16_t                reserved0:2;
} GxSearchPmtAc3Desc;

typedef struct {
	uint16_t                video_pid;
	uint16_t                ecm_pid_video;
	uint16_t                audio_count;
	uint16_t                video_count;
	uint16_t                pcr_pid;
	uint16_t                service_id;
	GxBusPmDataProgVideoType service_type;	//avs or mpeg

	GxSearchPmtTeletextDesc pmt_ttx_desc;
	GxSearchPmtSubtitlingDesc pmt_subt_desc;
	GxSearchPmtServiceMoveDesc pmt_service_move_desc;
	GxSearchPmtCaDesc       pmt_ca_desc;
	GxSearchPmtAc3Desc      pmt_ac3_desc;
} GxSearchPmtBody;

/*nit表的内容*/
/*直接解析成tp信息 存入search_nit_tp_id*/


/*记录 si服务分配的subtable 信息*/
typedef struct {
	int32_t                subtable_id;
	uint32_t               request_id;
} GxSearchIdBody;
/*sdt指明的业务类型*/

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
	SI_DVB_MHP_SERVICE = 0x10,
	SI_MPEG2_HD_VIDEO_SERVICE = 0x11,
	SI_H264_SD_VIDEO_SERVICE = 0x16,
	SI_H264_SD_NVOD_TIMESHIFT_SERVICE = 0x17,
	SI_H264_SD_NVOD_REFERENCE_SERVICE = 0x18,
	SI_H264_HD_VIDEO_SERVICE = 0x19,
	SI_H264_HD_NVOD_TIMESHIFT_SERVICE = 0x1a,
	SI_H264_HD_NVOD_REFERENCE_SERVICE = 0x1b,
	SI_H264_3D_HD_VIDEO_SERVICE = 0x1c,
	SI_H264_3D_HD_NVOD_TIMESHIFT_SERVICE = 0x1d,
	SI_H264_3D_HD_NVOD_REFERENCE_SERVICE = 0x1e,
	SI_HEVC_VIDEO_SERVICE = 0x1f,
	SI_HEVC_UHD_VIDEO_SERVICE = 0x20
} GxSearchSiServiceType;

typedef enum {			/*NIT, BAT, SDT, EIT, TOT, PMT, SIT     */
	CA_DESC = 0x09,
	ISO_639_LANGUAGE_DESC = 0x0a,
	NETWORK_NAME_DESC = 0x40,
	SERVICE_LIST_DESC = 0x41,
	STUFFING_DESC = 0x42,
	SATELLITE_DELIVERY_SYSTEM_DESC = 0x43,
	CABLE_DELIVERY_SYSTEM_DESC = 0x44,
	VBI_DATA_DESC = 0x45,
	VBI_TELETEXT_DESC = 0x46,
	BOUQUET_NAME_DESC = 0x47,
	SERVICE_DESC = 0x48,
	COUNTRY_AVAILABILITY_DESC = 0x49,
	LINKAGE_DESC = 0x4A,
	NVOD_REFERENCE_DESC = 0x4B,
	TIME_SHIFTED_SERVICE_DESC = 0x4C,
	SHORT_EVENT_DESC = 0x4D,
	EXTENDED_EVENT_DESC = 0x4E,
	TIME_SHIFTED_EVENT_DESC = 0x4F,
	COMPONENT_DESC = 0x50,
	MOSAIC_DESC = 0x51,
	STREAM_IDENTIFIER_DESC = 0x52,
	CA_IDENTIFIER_DESC = 0x53,
	CONTENT_DESC = 0x54,
	PARENTAL_RATING_DESC = 0x55,
	TELTEXT_DESC = 0x56,
	TELEPHONE_DESC = 0x57,
	LOCAL_TIME_OFFSET_DESC = 0x58,
	SUBTITLE_DESC = 0x59,
	TERRESTRIAL_DELIEVERY_SYSTEM_DESC = 0x5A,
	MULTILINGUAL_NETWORK_NAME_DESC = 0x5B,
	MULTILINGUAL_BOUQUET_NAME_DESC = 0x5C,
	MULTILINGUAL_SERVICE_NAME_DESC = 0x5D,
	MULTILINGUAL_COMPONENT_DESC = 0x5E,
	PRIVATE_DATA_SPECIFIER_DESC = 0x5F,
	SERVICE_MOVE_DESC = 0x60,
	SHORT_SMOOTHING_BUFFER_DESC = 0x61,
	FREQUENCY_LIST_DESC = 0x62,
	PARTIAL_TRANSPORT_STREAM_DESC = 0x63,
	DATA_BROADCAST_DESC = 0x64,
	CA_SYSTEM_DESC = 0x65,
	DATA_BROADCAST_ID_DESC = 0x66,
	TRANSPORT_STREAM_DESC = 0x67,
	DSNG_DESC = 0x68,
	PDC_DESC = 0x69,
	AC3_DESC = 0x6A,
	ANCILLARY_DATA_DESC = 0x6B,
	CELL_LIST_DESC = 0x6C,
	CELL_FREQUENCY_LINK_DESC = 0x6D,
	ANNOUNCEMENT_SUPPORT_DESC = 0x6E,
	APPLICATION_SIGNALLING_DESC = 0x6F,
	ADAPTATION_FIELD_DATA_DESC = 0x70,
	SERVICE_IDENTIFIER_DESC = 0x71,
	SERVICE_AVAILABILITY_DESC = 0x72,
	DEFAULT_AUTHORITY_DESC = 0x73,
	RELATED_CONTENT_DESC = 0x74,
	TVA_IDE_DESC = 0x75,
	CONTENT_IDENTIFIER_DESC = 0x76,
	TIME_SLICE_FEC_IDENTIFIER_DESC = 0x77,
	ECM_REPETITION_RATE_DESC = 0x78
} GxSearchSiDescriptor;


/*搜台使用的前端类型 包括 s t c */
typedef enum
{
	GX_SEARCH_FRONT_S = 0,
	GX_SEARCH_FRONT_T ,
	GX_SEARCH_FRONT_C ,
	GX_SEARCH_FRONT_DTMB,
}GxSearchFrontType;

typedef struct
{
	GxSearchIdBody id;//记录申请的表的控制id，由si服务分配的
	uint16_t   pid;			///< 表的pid 的值
	uint16_t  time_out;

	uint8_t    match_depth;		///< 匹配深度
	uint32_t    reserve:24;

	uint8_t    match[18];		///< 匹配值
	uint8_t    mask[18];		///< 匹配值的掩码，对应位为1并且匹配深度足够，才会按匹配值来过滤
	private_table_parser table_parse_fun;  ///< 私有解析函数指针;
    uint64_t   completion_timeout;// 用于超时强退，数据依赖该结构体成员time_out大小
}GxSearchExtendFilter;

typedef struct
{
	struct gxlist_head      list;
	GxSearchPmtBody         pmt_body;
	GxBusPmDataStream      *stream_body;
	uint32_t                stream_body_count;
}PmtStreamCache;

typedef status_t (*gx_search_tp_lock)(void);
typedef status_t (*gx_search_init_params)(void*);


/*搜索的实例,包含了一次搜索需要的参数和函数指针*/
typedef struct
{
	GxSearchFrontType front_type;
	GxSearchType     search_type;
	GxSearchTVRadio  search_tv_radio;
	GxSearchFtaCas   search_fta_cas;
	GxSearchNitSwitch search_nit_switch;
	GxPidSearch      search_pid_params;

	uint32_t         search_pat_finish_flag;
	uint32_t         search_sdt_finish_flag;
	uint32_t         search_pmt_all_finish_flag;

	uint32_t         search_sat_num;	///<所要搜索的卫星的数量
	uint32_t        *search_sat_id;	///<所要搜索的卫星的id
	uint32_t         search_sat_finish_num ;	///<已经搜索完成的sat数量,当search_sat_finish_num == search_sat_num时搜索结束
	uint32_t         search_sat_id_for_prog;	///<节目所属的sat id 用于保存节目 当更换卫星的时候进行更新
	uint32_t        *search_ts;	///<所要搜索的ts号，和卫星id顺序是一一对应的.
	uint32_t         search_ts_cur;///<当前搜索的ts号,在换卫星的时候进行改变

	uint32_t         search_tp_num;	///<盲扫扫到的tp的数量
	uint32_t        *search_tp_id;	///<所要搜索的tp的id
	uint32_t         search_tp_finish_num ;	///<已经搜索完成的tp数量,当search_tp_finish_num == search_tp_num时搜索结束
	uint32_t         search_tp_id_for_prog;	///<节目所属的tp id 用于保存节目 当更换tp的时候进行更新

	uint32_t         search_tuner_num_for_prog;	///<节目所属的tuner用于保存节目 当更换sat的时候进行更新

	uint32_t         search_nit_tp_count;	///<nit搜索搜到的tp的数量
	uint32_t        *search_nit_tp_id;	///<nit搜索搜到的tp的id空间

	GxSearchIdBody          search_nit_subtable_id;///<nit的subtable id
	uint32_t        search_nit_time_out;///<nit的超时时间

	/*pat*/
	GxSearchIdBody          search_pat_subtable_id;///<pat的subtable id
	uint32_t        search_pat_time_out;///<patt的超时时间
	GxSearchPatBody *search_pat_body;
	uint32_t         search_pat_body_count;

	/*sdt*/
	GxSearchIdBody          search_sdt_subtable_id;///<sdt的subtable id
	uint32_t        search_sdt_time_out;///<sdt的超时时间
	GxSearchSdtBody *search_sdt_body;
	uint32_t         search_sdt_body_count;

	/*pmt*/
	GxSearchIdBody          search_pmt_subtable_id[FILTER_MAX];///<记录pmt的subtable id
	uint32_t        search_pmt_time_out;///<pmt的超时时间
	GxSearchPmtBody  search_pmt_body;
	uint32_t         search_pmt_count;	///<正在解析的pmt的数量,最后应该等于search_pat_body_count
	uint32_t         search_pmt_filter_count;	///<用于解析pmt的filter的数量,最好情况应该等于search_pat_body_count
	uint32_t         search_pmt_finish_count;	///<解析好的pmt的数量,最后应该等于search_pat_body_count
	struct gxlist_head pmt_stream_cache_list;

	/*bat*/
	GxSearchIdBody          search_bat_subtable_id;///<bat的subtable id

	/*stream 现在仅仅用于存储audio信息*/
	uint32_t         search_stream_body_count;
	GxBusPmDataStream *search_stream_body ;
	GxBusPmDataVideoStream     *search_video_stream_body ;

	GxMsgProperty_StatusReply search_reply  ;	///<服务的回复消息的结构
	GxMsgNitReplyCode nit_flg;///<表明当前搜索的tp是否是nit出来的
	uint32_t         search_transaction_bgin_flag;

	GxMsgProperty_NewProgGet search_new_prog_get  ;	///<找到新节目的消息的结构体

	uint32_t         search_status;//search_stop_flag;	///<停止搜索标志 该标志清除很特殊,在每次收到开始搜索的消息后清零

	/*前端设备的句柄*/
	handle_t         search_device_handle;
	handle_t         search_module_frontend;
	handle_t       	 search_demux_handle;
	handle_t 		 search_msg_handle;///<服务的句柄
	uint32_t         search_demux_id;///<获取的demux id

	/*记录下搜索到的节目 用于接受到save是保存*/
	uint32_t* 		 search_prog_id_arry;
	uint32_t         search_prog_id_num ;

	/*盲扫信号量*/
	handle_t         search_blind_sem;
	/*盲扫的参数*/
	GxMsgBlindSearchType search_blind_type;///<盲扫的方式
	GxMsgBlindPolarType  search_blind_polar;///<极化方向

	/*扩展功能，需要注册表的pid，table id，其他过滤条件和一个私有解析函数*/
	GxSearchExtendFilter* search_ext;
	uint32_t           search_ext_num;//扩展解析的表的数量
	uint32_t           search_ext_finish_num;//完成扩展解析的表的数量
	uint32_t           search_ext_start;//1-开始扩展解析 0-停止扩展解析
    uint32_t           search_ext_count;    ///<正在解析的ext表的数量,最后应该等于（search_ext_finish_num>0?search_ext_finish_num-1:0）
	uint32_t           need_search_ext; ///< 是否需要扩展过滤
	bool               only_filter_ext; ///< tp只进行扩展过滤
	GxSearchExtFinish  ext_finish;

	/*pmt的扩展解析*/
	private_table_parser pmt_parse_fun;

	gx_search_tp_lock	tp_lock_func;//锁频函数
	gx_search_init_params init_params_func;//初始化各个参数函数

	/*diseqc操作回调*/
	GxSearchDiseqc search_diseqc;

	uint8_t current_tp_streamsize;

	GxSearchCheckPmtPid check_pmtpid_fun;
	GxSearchModifyProg  modify_prog;
}GxSearchClass;


typedef struct
{
	uint32_t fre;//khz
	uint32_t symbol_rate;
	uint32_t pls_n;
	GxBlindSearchTpType type;
	fe_modulation_t qpsk;
	uint8_t stream_size;
}GxBlindSearchTp;

typedef struct
{
	uint16_t start_fre;
	uint16_t end_fre;

	uint8_t tp_polar;
	uint8_t polar_stage;//用来区别盲扫到哪个极化方向了
	uint16_t reserved;

	uint8_t start_flag;//开始调用前端驱动标志
	uint16_t window_total;//每个window包含的步进量
	uint8_t blind_exit;//退出盲扫标志

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

static uint32_t BLIND_SCAN_STEP_MHZ = 12;
static uint32_t BLIND_SCAN_WINDOW_SIZE_K = 45000;
static uint32_t BLIND_SCAN_LOW_FREQ_MHZ = 950;
static uint32_t BLIND_SCAN_HIGH_FREQ_MHZ = 2150;
static uint32_t BLIND_SCAN_LOW_FREQ_LNBF_MHZ = 300;
static uint32_t BLIND_SCAN_HIGH_FREQ_LNBF_MHZ = 2500;

static uint32_t  check_ca_system(uint16_t ca_system_id,uint16_t ele_pid,uint16_t ecm_pid)
{
	return 1;
}
static bool  check_pmt_pid(uint16_t pmt_pid,uint16_t prog_num)
{
	return true;
}
static GxSearchCheckCa gx_search_check_ca = check_ca_system;
/* Private Constants ------------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */

/*搜索的实例,现在暂时做成一个全局变量
  这样就意味着只能一次实例化一个搜索
  这个东西以后考虑做成每开始一次搜索实例化一次(使用malloc申请空间),
  这样的话就能做到同时开始多个搜索实例*/

static GxSearchClass search_class;

/*盲扫实例*/
static GxBlindSearchClass blind_class = {0};

/*盲扫状态*/
static int32_t blind_status = GX_SEARCH_OK; //GX_SEARCH_OK--正常 GX_SEARCH_FINISH--停止 GX_SEARCH_ERR--出错

static uint32_t search_exit = 0;

/* Debug Defined ---------------------------------------------------------- */

/* Private Functions ------------------------------------------------------ */
static status_t gx_search_init_frontend(void);

static status_t gx_search_realease_frontend(void);

static status_t gx_search_class_init(GxSearchFrontType front_type);

static status_t gx_search_sat_init_params(void* sat_search_params);

static status_t gx_search_start(void);

static status_t gx_search_nit_filter_create(void);

static status_t gx_search_nit_filter_release(void);

static status_t gx_search_nit_info_get(GxParseResult* parse_result);

static status_t gx_search_nit_section_ok(GxParseResult* parse_result);

static status_t gx_search_nit_subtable_ok(GxParseResult* parse_result);

//static status_t         gx_search_bat_filter_create(void);

static status_t gx_search_bat_filter_release(void);

static status_t gx_search_bat_subtable_ok(GxParseResult* parse_resul);

static status_t gx_search_pat_filter_create(void);

static status_t gx_search_pat_filter_release(void);

static status_t gx_search_pat_info_get(GxParseResult* parse_result);

static status_t gx_search_pat_subtable_ok(GxParseResult* parse_result);

static int32_t  gx_search_pat_find(GxSearchPmtBody search_pmt_body, GxSearchPatBody* pat);	//比较的参数来自于search_pmt_body

static status_t gx_search_sdt_filter_create(void);

static status_t gx_search_sdt_filter_release(void);

static status_t gx_search_sdt_info_get(GxParseResult* parse_result);

static status_t gx_search_sdt_section_ok(GxParseResult* parse_result);

static status_t gx_search_sdt_subtable_ok(GxParseResult* parse_result);

static int32_t  gx_search_sdt_find(GxSearchPmtBody search_pmt_body, GxSearchSdtBody* sdt);	//比较的参数来自于search_pmt_body

static status_t gx_search_pmt_filter_create(void);

static status_t gx_search_pmt_filter_release(GxMsgProperty_SiRelease* subtable_id);

static status_t gx_search_pmt_cas_get(uint32_t* cas1,
		uint32_t cas1_count,
		uint32_t* cas2,
		uint32_t cas2_count,
		uint32_t stream_type);

static status_t gx_search_pmt_info_get(GxParseResult* parse_result);

static status_t gx_search_pmt_subtable_ok(GxParseResult* parse_result);

static status_t gx_search_pmt_next_filte_start(GxParseResult* parse_result);

static status_t gx_search_prog_info_add(GxBusPmDataStream *search_stream_body,
										uint32_t search_stream_body_count,
										GxSearchPmtBody search_pmt_body);

static void gx_search_prog_check_audio_lang(uint16_t* audio_pid,
											GxBusPmDataProgAudioType* type,
											uint16_t* ecm_pid,
											GxBusPmDataStream *search_stream_body,
											uint32_t search_stream_body_count);

static void gx_search_error_reply(GxMsgStatusReplyCode error);

static void gx_search_new_prog_reply(GxMsgProperty_NewProgGet new_prog_get);

static void gx_search_variable_exit_init(void);

static void gx_search_variable_next_tp_init(void);

static status_t gx_search_ext_start(void);

static status_t gx_search_ext_release(void);

static status_t gx_search_ext_subtable_ok(GxParseResult* parse_result);

static status_t gx_search_ext_timeout_check(GxParseResult* parse_result);

static status_t gx_search_timeout_check(GxParseResult * parse_result);

static status_t gx_search_section_ok(GxParseResult * parse_result);

static status_t gx_search_subtable_ok(GxParseResult * parse_result);

static status_t gx_search_scan_start(GxMessage * Msg);

static status_t gx_search_table_filter_start(void);

static status_t gx_search_blind_start(GxMessage * Msg);

static status_t gx_blind_search(GxBlindSearchClass* blind_scan);

static void gx_search_blind_stop_reply(void);

static status_t gx_search_blind_new_tp_get(uint32_t i,GxBlindSearchClass* blind_scan);

static status_t gx_blind_progress(GxBlindSearchClass* blind_scan);

static status_t gx_blind_set_22k(uint32_t flag);

static status_t gx_blind_set_poarl(uint8_t polar);

static status_t gx_search_blind_get_params(GxBlindSearchClass* params);

static uint32_t gx_blind_single_double_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan);

static uint32_t  gx_blind_single_one_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan);

static uint32_t  gx_blind_ocs_double_poar_ku_progress(uint32_t start,GxBlindSearchClass* blind_scan);

static status_t gx_blind_send_progress(uint8_t progress, GxBlindSearchStage stage);

static status_t gx_blind_scan_tp(GxBlindSearchClass* blind_scan);

static uint32_t gx_blind_progress_check_stop(void);

static void gx_blind_set_status(int32_t status);

static uint32_t gx_blind_get_status(void);

static void gx_blind_scan_get_tp_info(GxBlindSearchClass* blind_scan,uint32_t i,struct dvb_frontend_parameters params);

static int32_t gx_blind_sort_tp(GxBlindSearchClass* blind_scan);

static int32_t gx_blind_sort_tp_by_fre(const void *p1, const void *p2);

static int32_t gx_blind_scan_window(GxBlindSearchClass* blind_scan);

static status_t gx_blind_start(uint8_t polar,int32_t sat22k,GxBlindSearchClass* blind_scan);

// pdmx
extern int search_pdmx_stop(int tuner, void* params);

extern int search_pdmx_init(int tuner, int demux_id, void *params);

extern int search_pdmx_set_params(int tuner, int ts_source, int demux_id, void *params);

extern int search_pdmx_get_params(void *params);

extern int search_pdmx_get_ts_size(void);

extern int search_pdmx_get_active_flag(void);

extern bool gx_search_noavprog_save_state(void);

// multistream
extern int32_t search_multistream_init(int32_t tuner);
extern int32_t search_multistream_stop(void);
extern int32_t search_multistream_get_params(void *params);
extern int32_t search_multistream_set_params(int32_t tuner);
extern bool    search_multistream_increase_ts_index(void);
extern bool    search_multistream_get_active_flag(void);

//plsn
extern bool search_plsn_set(int32_t tuner, uint32_t *tp_pls_n);

extern void GxFrontend_SetDev(int32_t tuner, int32_t dev, int32_t demux);

extern status_t gx_search_pmt_cache_add(struct gxlist_head *head,
										GxBusPmDataStream *stream_body,
										uint32_t stream_body_count,
										GxSearchPmtBody pmt_body);
extern PmtStreamCache *gx_search_pmt_cache_get(struct gxlist_head *head, uint16_t service_id);
extern void gx_search_pmt_cache_list_destroy(struct gxlist_head *head);

static status_t _prog_add_by_pmt_cache(PmtStreamCache *cache)
{
	status_t ret = GX_SEARCH_OK;

	ret = gx_search_prog_info_add(cache->stream_body, cache->stream_body_count, cache->pmt_body);

	if (ret == GX_SEARCH_DBASE_FULL) {
#ifdef GX_BUS_SEARCH_DBUG
		SEARCH_BASE_PRINTF("[SEARCH]---add prog err!!ret = %x\n",ret);
#endif
		gx_search_error_reply(SEARCH_DBASE_OVERFLOW);
	}
	else if(ret == GX_SEARCH_PROG_EXIST || GX_SEARCH_OK == ret)
	{
		gx_search_new_prog_reply(search_class.search_new_prog_get);
		ret = GX_SEARCH_OK;
	}
	else if(ret == GX_SEARCH_PROG_CONDITION_ERR)
	{
		ret = GX_SEARCH_OK;
	}

	return ret;
}

static status_t gx_search_prog_add_by_pmt_cache(void)
{
	status_t ret = GX_SEARCH_OK;
	PmtStreamCache *pos = NULL;

	gxlist_for_each_entry(pos, &search_class.pmt_stream_cache_list, list)
	{
		ret = _prog_add_by_pmt_cache(pos);
		if(ret != GX_SEARCH_OK)
			break;
	}

	return ret;
}

static status_t gx_search_init_frontend(void)
{
	struct dvb_frontend_info info;
	status_t                ret;
	GxDemuxProperty_ConfigDemux config_demux;

	search_class.search_device_handle = GxAvdev_CreateDevice(0);
	if (search_class.search_device_handle < 0) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---create device err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	//frontend
	search_class.search_module_frontend = GxFrontend_IdToHandle(search_class.search_tuner_num_for_prog);
	if (search_class.search_module_frontend < 0) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---open module  err!!\n");
#endif
		return GX_SEARCH_ERR;
	}

	ret = ioctl(search_class.search_module_frontend, FE_GET_INFO, &info);
	if (ret < 0) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---get av property err!!\n");
#endif
		return GX_SEARCH_ERR;
	}

#ifdef GX_BUS_BLIND_SEARCH_DBUG
	if (FE_DVB_C == info.type || FE_DVB_C2 == info.type)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---frontend is cable!!\n");
	}
	else if (FE_DVB_T == info.type || FE_DVB_T2 == info.type)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---frontend is T!!\n");
	}
	else if (FE_DVB_S == info.type || FE_DVB_S2 == info.type)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---frontend is S!!\n");
	}
	else if(info.type == FE_DTMB)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---frontend is DTMB!!\n");
	}
#endif

	//search_class.search_demux_id = GxAvdev_AllocDemux(search_class.search_ts_cur);
	search_class.search_demux_handle = GxAvdev_OpenModule(search_class.search_device_handle, GXAV_MOD_DEMUX, search_class.search_demux_id);
	if(search_class.search_demux_handle<0)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---open demux  err!!\n");
#endif
		return GX_SEARCH_ERR;
	}

	GxFrontend_SetDev(search_class.search_tuner_num_for_prog, search_class.search_device_handle, search_class.search_demux_handle);
	//绑定demux和ts
	config_demux.source = search_class.search_ts_cur;
	config_demux.ts_select = FRONTEND;
	config_demux.stream_mode = DEMUX_PARALLEL;
	config_demux.time_gate = 0xf;
	config_demux.byt_cnt_err_gate = 0x03;
	config_demux.sync_loss_gate = 0x03;
	config_demux.sync_lock_gate = 0x03;
	ret = GxAVSetProperty(search_class.search_device_handle,
			search_class.search_demux_handle,
			GxDemuxPropertyID_Config,
			&config_demux,
			sizeof(GxDemuxProperty_ConfigDemux));
	if(ret < 0)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---config demux  err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}

static status_t gx_search_realease_frontend(void)
{
	status_t                ret = 0;
	//frontend
	if(search_class.search_device_handle != GXCORE_INVALID_POINTER
			&&search_class.search_demux_handle != GXCORE_INVALID_POINTER)
	{
		ret = GxAvdev_CloseModule(search_class.search_device_handle, search_class.search_demux_handle);
		search_class.search_demux_handle = GXCORE_INVALID_POINTER;
		if (ret < 0)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			BLIND_SEARCH_BASE_PRINTF
				("[BLIND SEARCH]---close module err!!search_demux_handle = %x\n",
				 search_class.search_demux_handle);
#endif

		}
		//松绑demux和ts
		//GxAvdev_FreeDemux(search_class.search_demux_id);

		ret = GxAvdev_DestroyDevice(search_class.search_device_handle);
		if (ret < 0)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			BLIND_SEARCH_BASE_PRINTF
				("[BLIND SEARCH]---destroy device err!!search_device_handle = %x\n",
				 search_class.search_device_handle);
#endif

		}
		search_class.search_device_handle = GXCORE_INVALID_POINTER;
	}

	return GX_SEARCH_OK;
}

static status_t gx_search_class_init(GxSearchFrontType front_type)
{
	uint32_t i = 0;
	search_class.front_type = front_type;

	search_class.search_pat_finish_flag = 0;

	search_class.search_sdt_finish_flag = 0;

	search_class.search_pmt_all_finish_flag = 0;

	search_class.search_sat_num = 0;	///<所要搜索的卫星的数量
	if(search_class.search_sat_id != NULL)
	{
		GxCore_Free(search_class.search_sat_id);
	}
	search_class.search_sat_id = NULL;	///<所要搜索的卫星的id
	search_class.search_tp_num = 0;	///<所要搜索的tp的数量
	if(search_class.search_tp_id != NULL)
	{
		GxCore_Free(search_class.search_tp_id);
	}
	search_class.search_tp_id = NULL;	///<所要搜索的tp的id
	search_class.search_sat_finish_num = 0;	///<已经搜索完成的sat数量,当search_sat_finish_num == search_sat_num时搜索结束
	search_class.search_tp_finish_num = 0;	///<已经搜索完成的tp数量,当search_tp_finish_num == search_tp_num时搜索结束
	search_class.search_nit_tp_count = 0;	///<nit搜索搜到的tp的数量
	if(search_class.search_nit_tp_id != NULL)
	{
		GxCore_Free(search_class.search_nit_tp_id);
	}
	search_class.search_nit_tp_id = NULL;	///<nit搜索搜到的tp的id空间

	/*各个subtable_id*/
	search_class.search_pat_subtable_id.subtable_id = -1;	///<pat的subtable id
	search_class.search_sdt_subtable_id.subtable_id = -1;	///<sdt的subtable id
	search_class.search_nit_subtable_id.subtable_id = -1;	///<nit的subtable id
	search_class.search_bat_subtable_id.subtable_id = -1;	///<bat的subtable id
	for(i=0; i< FILTER_MAX; i++)
	{
		search_class.search_pmt_subtable_id[i].subtable_id = -1;

	}

	/*pat*/
	if(search_class.search_pat_body != NULL)
	{
		GxCore_Free(search_class.search_pat_body);
	}
	search_class.search_pat_body = NULL;

	search_class.search_pat_body_count = 0;

	/*sdt*/
	if(search_class.search_sdt_body != NULL)
	{
		GxCore_Free(search_class.search_sdt_body);
	}
	search_class.search_sdt_body = NULL;

	search_class.search_sdt_body_count = 0;

	/*pmt*/
	memset(&(search_class.search_pmt_body),0,sizeof(GxSearchPmtBody));

	GX_INIT_LIST_HEAD(&search_class.pmt_stream_cache_list);

	search_class.search_pmt_count = 0;	///<正在解析的pmt的数量,最后应该等于search_pat_body_count
	search_class.search_pmt_filter_count = 0;	///<用于解析pmt的filter的数量,最好情况应该等于search_pat_body_count
	search_class.search_pmt_finish_count = 0;	///<解析好的pmt的数量,最后应该等于search_pat_body_count

	/*stream 现在仅仅用于存储audio信息*/
	search_class.search_stream_body_count = 0;
	if(search_class.search_stream_body != NULL)
	{
		GxCore_Free(search_class.search_stream_body);
	}
	search_class.search_stream_body = NULL;

	if(search_class.search_video_stream_body != NULL)
	{
		GxCore_Free(search_class.search_video_stream_body);
	}
	search_class.search_video_stream_body = NULL;
	search_class.search_transaction_bgin_flag = 0;

	search_class.search_sat_id_for_prog = 0;	///<节目所属的sat id 用于保存节目 当更换卫星的时候进行更新
	search_class.search_tp_id_for_prog = 0;	///<节目所属的tp id 用于保存节目 当更换tp的时候进行更新

	search_class.nit_flg= NOT_NIT;	///<nit标记

	search_class.search_status = SEARCH_START;	///<停止搜索标志 该标志清除很特殊,在每次收到开始搜索的消息后清零

	/*前端设备的句柄*/
	search_class.search_device_handle = GXCORE_INVALID_POINTER;
	search_class.search_module_frontend = GXCORE_INVALID_POINTER;
	search_class.search_demux_handle = GXCORE_INVALID_POINTER;

	/*记录下搜索到的节目 用于接受到save是保存*/
	if(search_class.search_prog_id_arry != NULL)
	{
		GxCore_Free(search_class.search_prog_id_arry);
	}
	search_class.search_prog_id_arry = NULL;
	search_class.search_prog_id_num = 0;

	/*搜台的扩展*/
	if(search_class.search_ext != NULL)
	{
		GxCore_Free(search_class.search_ext);
	}
	search_class.search_ext = NULL;
	search_class.search_ext_num = 0;
	search_class.search_ext_finish_num = 0;
	search_class.search_ext_start = 0;
    search_class.search_ext_count = 0;
	search_class.pmt_parse_fun = NULL;
	gx_search_check_ca = check_ca_system;

	/*ts号信息*/
	if(search_class.search_ts != NULL)
	{
		GxCore_Free(search_class.search_ts);
	}
	search_class.search_ts = NULL;
	search_class.search_ts_cur = 0;

	switch(front_type)
	{
		case GX_SEARCH_FRONT_S:
			search_class.tp_lock_func = NULL;
			search_class.init_params_func = gx_search_sat_init_params;
			break;

		default:
			break;
	}
	return GX_SEARCH_OK;
}

static void _blind_scan_get_tp_fre(GxBlindSearchClass* blind_scan,uint32_t i, unsigned int cen_fre, unsigned short *fre)
{
	uint32_t if_frequency = 0;
	uint32_t local_frequency = 0;
	int32_t lnbf_tag = 0;

	if_frequency = (cen_fre + 500)/1000;

	if(blind_scan->lnb_type == BLIND_SCAN_LNB_C)
	{
		if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)//˫????
		{
			if(blind_scan->tp_polar == GXBUS_PM_TP_POLAR_H)
			{
				local_frequency = blind_scan->local_fre1;
			}
			else
			{
				local_frequency = blind_scan->local_fre2;
			}
		}
		else
		{
			local_frequency = blind_scan->local_fre1;
		}
		if(local_frequency > if_frequency)
		{
			*fre = local_frequency - if_frequency;
		}
		else
		{
			*fre = local_frequency + if_frequency;
		}
	}
	else
	{
		if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)
		{
			GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);//lnbf support #359333
			if((lnbf_tag == GXBUS_FRONTEND_LNBF_ON) &&
					((blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) ||
					(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ))){
				if(blind_scan->tp_polar == GXBUS_PM_TP_POLAR_H)
					local_frequency = blind_scan->local_fre1;
				else
					local_frequency = blind_scan->local_fre2;
			}else{
				if(blind_scan->switch_22k == GXBUS_PM_SAT_22K_ON)//22k on
				{
					if(blind_scan->local_fre1 > blind_scan->local_fre2)
					{
						local_frequency = blind_scan->local_fre1;
					}
					else
					{
						local_frequency = blind_scan->local_fre2;
					}
				}
				else
				{
					if(blind_scan->local_fre1 > blind_scan->local_fre2)
					{
						local_frequency = blind_scan->local_fre2;
					}
					else
					{
						local_frequency = blind_scan->local_fre1;
					}
				}
			}
		}
		else
		{
			local_frequency = blind_scan->local_fre1;
		}
		*fre = local_frequency + if_frequency;
	}
	return;
}

static void gx_blind_search_multistream_set_ts(uint32_t i, GxBlindSearchClass* blind_scan, unsigned int fre)
{
	int32_t tuner = search_class.search_tuner_num_for_prog;

	search_class.current_tp_streamsize = 188;

	// 1ST close pdmx
	search_pdmx_stop(tuner, NULL);

	// 2ST change multistream tsid
	search_multistream_set_params(tuner);

	// 3ST init pdmx
	search_pdmx_init(tuner, search_class.search_demux_id, NULL);
	search_class.current_tp_streamsize = search_pdmx_get_ts_size();
	// 4ST set the firist params
	PDMXSignalInfoClass  signal_info = {0};
	{
		GxBusPmDataSat          sat = { 0 };
		GxBus_PmSatGetById(search_class.search_sat_id_for_prog, &sat);

		signal_info.longitude = (sat.sat_s.longitude_direct > 0?(3600-sat.sat_s.longitude):sat.sat_s.longitude);
		_blind_scan_get_tp_fre(blind_scan, i, fre, &signal_info.fre_m);
		signal_info.sym_k = blind_scan->tp[i].symbol_rate;
		signal_info.pol   = blind_scan->tp_polar;
	}

	search_pdmx_set_params(tuner, search_class.search_ts_cur, search_class.search_demux_id, &signal_info);
}


status_t gx_search_sat_init_params(void *
		sat_search_params)
{
	uint32_t                exist_tp_count = 0;
	GxMsgProperty_BlindScanStart* params = NULL;
	uint32_t i = 0;

	params = (GxMsgProperty_BlindScanStart*)sat_search_params;

	// shenbin add for selfdefine Blind Mode 20131210
	if (params->blind_mode.enable == BLIND_SCAN_MODE_ENABLE)
	{
		BLIND_SCAN_STEP_MHZ = params->blind_mode.step_mhz;
		BLIND_SCAN_WINDOW_SIZE_K = params->blind_mode.window_size_k;
		BLIND_SCAN_LOW_FREQ_MHZ = params->blind_mode.low_freq_mhz;
		BLIND_SCAN_HIGH_FREQ_MHZ = params->blind_mode.high_freq_mhz;
	}

	if(params->max_num == 0
			||params->array == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---search params err!!\n");
#endif
		return GX_SEARCH_ERR;
	}


	// multistream
	search_multistream_stop();

	// pdmx
	search_pdmx_stop(search_class.search_tuner_num_for_prog, NULL);


	search_class.search_tv_radio = params->tv_radio;

	search_class.search_fta_cas = params->fta_cas;

	search_class.search_nit_switch = params->nit_switch;

	/*有多少个pat body就意味着有多少个总的sdt body,所以空间应该开总的
	  body,但是pat和sdt是同时解析的,所以无法通过pat body判断,直接开一个300 */

	search_class.search_sdt_body = (GxSearchSdtBody *)
		GxCore_Malloc(sizeof(GxSearchSdtBody) * MAX_PROG_PER_TP);	//每个tp最多300节目

	/*对于搜到的节目由于不定长 所以直接开最大节目数量 用完后释放 该地方会耗费几百k内存*/
	search_class.search_prog_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
	if(search_class.search_prog_id_arry == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc search_prog_arry space err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	search_class.search_prog_id_num = 0;

	/*开辟nit的tp空间,由于不确定具体数量,该空间为所能保存的tp最大数量减去
	  已经存在的tp的数量 */
	if (search_class.search_nit_switch == GX_SEARCH_NIT_ENABLE)
	{
		exist_tp_count = GxBus_PmTpNumGet();

		if (MAX_TP_COUNT > exist_tp_count)
		{
			search_class.search_nit_tp_id = (uint32_t *)
				GxCore_Malloc(sizeof(uint32_t) *
						(MAX_TP_COUNT - exist_tp_count));
		}
	}

	/*记录搜索扩展的信息*/
	if(params->ext != NULL && params->ext_num != 0)
	{
		search_class.search_ext_num = params->ext_num;
		search_class.search_ext = GxCore_Malloc(sizeof(GxSearchExtendFilter)*params->ext_num);
		if(search_class.search_ext == NULL)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc search_ext space err!!\n");
#endif
			return GX_SEARCH_ERR;
		}
		for(i=0; i<params->ext_num; i++)
		{
			search_class.search_ext[i].pid = params->ext[i].pid;
			search_class.search_ext[i].match_depth = params->ext[i].match_depth;
			search_class.search_ext[i].time_out = params->ext[i].time_out;
			memcpy(search_class.search_ext[i].match,params->ext[i].match,18);
			memcpy(search_class.search_ext[i].mask,params->ext[i].mask,18);
			search_class.search_ext[i].table_parse_fun = params->ext[i].table_parse_fun;
			search_class.search_ext[i].id.subtable_id = -1;
		}
		search_class.need_search_ext = 1;
		search_class.only_filter_ext = params->only_filter_ext;
		search_class.ext_finish = params->ext_finish;
	}

	if(params->time_out.pat != 0)
	{
		search_class.search_pat_time_out = params->time_out.pat;
	}
	else
	{
		search_class.search_pat_time_out = SEARCH_PAT_TIMEOUT_VALUE;
	}
	if(params->time_out.sdt != 0)
	{
		search_class.search_sdt_time_out = params->time_out.sdt;
	}
	else
	{
		search_class.search_sdt_time_out = SEARCH_SDT_TIMEOUT_VALUE;
	}
	if(params->time_out.nit != 0)
	{
		search_class.search_nit_time_out = params->time_out.nit;
	}
	else
	{
		search_class.search_nit_time_out = SEARCH_NIT_TIMEOUT_VALUE;
	}
	if(params->time_out.pmt != 0)
	{
		search_class.search_pmt_time_out = params->time_out.pmt;
	}
	else
	{
		search_class.search_pmt_time_out = SEARCH_PMT_TIMEOUT_VALUE;
	}

	if(params->search_diseqc != NULL)
	{
		search_class.search_diseqc = params->search_diseqc;
	}
	else
	{
		search_class.search_diseqc = NULL;
	}

	if(params->check_ca_fun != NULL)
	{
		gx_search_check_ca = params->check_ca_fun;
	}

	if(params->check_pmtpid_fun != NULL)
	{
		search_class.check_pmtpid_fun = params->check_pmtpid_fun;
	}
	else
	{
		search_class.check_pmtpid_fun = check_pmt_pid;
	}

	search_class.search_demux_id = params->demux_id;

	search_class.search_sat_num = params->max_num;
	search_class.search_sat_id =
		(uint32_t *) GxCore_Malloc(sizeof(uint32_t) * search_class.search_sat_num);
	if (search_class.search_sat_id == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc sat id space err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	memcpy(search_class.search_sat_id, params->array, sizeof(uint32_t) * search_class.search_sat_num);	//记录所要搜索的sat的id

	if(params->ts == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---ts num err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	search_class.search_ts = (uint32_t *) GxCore_Malloc(sizeof(uint32_t)*search_class.search_sat_num);
	if(search_class.search_ts == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---malloc ts num space err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	memcpy(search_class.search_ts, params->ts, sizeof(uint32_t) * search_class.search_sat_num);//记录所要搜索的sat的ts

	/*记录所要搜索的pid*/
	search_class.search_pid_params.audio_pid = params->params_pid.audio_pid;
	search_class.search_pid_params.video_pid= params->params_pid.video_pid;
	search_class.search_pid_params.pcr_pid= params->params_pid.pcr_pid;
	search_class.modify_prog = params->modify_prog;

	if(GXCORE_ERROR == search_cache_init(search_class.search_sat_num, search_class.search_sat_id, 0, NULL))
		return GX_SEARCH_ERR;

	return GX_SEARCH_OK;
}

static status_t gx_search_start(void)
{
	status_t                ret = 0;

	/*先进行扩展解析，再进入标准搜索流程*/
	if(search_class.search_ext_num != 0
			&& search_class.search_ext_finish_num != search_class.search_ext_num
			&& search_class.need_search_ext != 0)
	{
		ret = gx_search_ext_start();
		if (ret != GX_SEARCH_OK)
		{
			gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---creat ext filter err!!\n");
#endif
			return GX_SEARCH_ERR;
		}
	}
	else if(true == search_class.only_filter_ext)
	{
		return GX_SEARCH_OK;
	}

	/*首先判断需不需要nit搜索,如果需要nit搜索,就先进行当前锁定tp的nit搜索
	  并且建立nit tp列表,记录下数量 当前锁定tp的pat sdt的搜索在结束nit搜索,建立nit tp
	  列表之后开始,pmt在以上表全部完成之后开始,每一个锁定的tp都经过以上步骤,最后
	  完成了传下来的tp的搜索后,根据建立的nit tp列表进行搜索 ,lcn功能也是需要nit表
	  过滤的*/
	else if (search_class.search_nit_switch == GX_SEARCH_NIT_ENABLE)
	{
		ret = gx_search_nit_filter_create();
		if (ret != GX_SEARCH_OK)
		{
			gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---creat nit filter err!!\n");
#endif
			return GX_SEARCH_ERR;
		}

	}

	/*不需要nit搜索 直接开始pat和sdt的搜索,pmt在以上表全部完成之后开始 */
	else {
		ret = gx_search_pat_filter_create();

		if (ret != GX_SEARCH_OK) {
			gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF
				("[BLIND SEARCH]---creat pat filter err!!\n");
#endif
			return GX_SEARCH_ERR;
		}

		ret = gx_search_sdt_filter_create();

		if (ret != GX_SEARCH_OK) {
			gx_search_error_reply(SEARCH_ERROR);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF
				("[BLIND SEARCH]---creat sdt filter err!!\n");
#endif
			return GX_SEARCH_ERR;
		}

	}
	return GX_SEARCH_OK;
}

status_t gx_search_nit_filter_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = {0};

	GxMessage              *new_msg = NULL;

	//status_t                ret = 0;

	SubTable.ts_src = search_class.search_ts_cur;

	SubTable.time_out = search_class.search_nit_time_out;

	SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;
	SubTable.table_parse_cfg.table_parse_fun = NULL;

	SubTable.si_filter.pid = NIT_PID;

	SubTable.si_filter.match_depth = 1;	//需要匹配0字节和5字节,因此匹配深度为6
	SubTable.si_filter.eq_or_neq = 1;

	SubTable.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;

	SubTable.si_filter.mask[0] = 0xff;

	SubTable.si_filter.match[5] = 0x01;

	SubTable.si_filter.mask[5] = 0x00;//这一位必然是1,所以可以不匹配

	/*发送创建NIT的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
	params_create = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
	*params_create = &SubTable;

	GxBus_MessageSendWait(new_msg);

	if (SubTable.si_subtable_id == -1)	//创建pat失败
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---ceate nit err!!\n");
#endif
		GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE
		return GX_SEARCH_ERR;
	}

	search_class.search_nit_subtable_id.subtable_id = SubTable.si_subtable_id;
	search_class.search_nit_subtable_id.request_id = SubTable.request_id;

	GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE


	/*发送开始过滤nit的消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);
	*params_start = search_class.search_nit_subtable_id.subtable_id;
	GxBus_MessageSend(new_msg);
	search_class.search_status &= ~SEARCH_NIT_STOPING;
	return GX_SEARCH_OK;
}

status_t gx_search_nit_filter_release(void)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放nit的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);
	*params = search_class.search_nit_subtable_id.subtable_id;
	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE

	search_class.search_nit_subtable_id.subtable_id = -1;
	search_class.search_status |= SEARCH_NIT_STOPING;
	return GX_SEARCH_OK;

}


static status_t gx_search_nit_info_get(GxParseResult * parse_result)
{
	GxBusPmDataTP tp={0};
	uint32_t i = 0, j = 0;
	NitInfo* nit_info = NULL;
	ts_info_t* ts_info = NULL;
	niti_delivery_info_t *delivery_info = NULL;
	int32_t tp_exist_count = 0;
	uint16_t tp_id = 0;
	status_t ret = 0;
	//nit没有开启的时候是不需要nit数据的，这时候的nit是lcn功能启动的，数据由用
	//户自己管理，这里只需要知道nit表解好了
	if(search_class.search_nit_switch == GX_SEARCH_NIT_DISABLE)
	{
		GxBus_SiParserBufFree(parse_result->parsed_data);
		return GX_SEARCH_OK;
	}

	nit_info = (NitInfo *) (parse_result->parsed_data);

	if(search_class.search_nit_tp_id == NULL)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
		return GX_SEARCH_DBASE_FULL;
	}

	for(i=0; i<nit_info->ts_count; i++)
	{
		ts_info = (ts_info_t *)(nit_info->ts_info[i]);
		for(j=0; j<ts_info->delivery_count; j++)
		{
			delivery_info = ts_info->delivery_info;
			switch(search_class.front_type)
			{
				case GX_SEARCH_FRONT_S:
					if(delivery_info[j].delivery_type == SATELLITE_DELIVERY_SYSTEM_DESC)
					{
						tp.frequency = delivery_info[j].sat.freq;
						tp.sat_id = search_class.search_sat_id_for_prog;
						tp.tp_s.symbol_rate = delivery_info[j].sat.sym_rate;
						if((delivery_info[j].sat.polar == 0) || (delivery_info[j].sat.polar == 2))
						{
							tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
						}
						else if((delivery_info[j].sat.polar == 1) || (delivery_info[j].sat.polar == 3))
						{
							tp.tp_s.polar = GXBUS_PM_TP_POLAR_V;
						}
						else
						{
							tp.tp_s.polar = GXBUS_PM_TP_POLAR_H;
						}

						tp_exist_count = search_cache_tp_exist_check(tp.sat_id,
																		tp.frequency,
																		tp.tp_s.symbol_rate,
																		tp.tp_s.polar,
																		0,
																		&tp_id);
						if(tp_exist_count == 0)
						{
							ret = GxBus_PmTpAdd(&tp);
							if(ret == GXCORE_SUCCESS)
							{
								search_cache_tp_add(tp);
								search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp.id;
								search_class.search_nit_tp_count++;
								break;
							}
							else if(ret == GX_PM_DBASE_FULL)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_DBASE_FULL;
							}
							else if(ret == GXCORE_ERROR)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save err!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_ERR;
							}

						}
					}
					break;

				case GX_SEARCH_FRONT_T:
					if(delivery_info[j].delivery_type == TERRESTRIAL_DELIEVERY_SYSTEM_DESC)
					{
						tp.frequency = delivery_info[j].ter.centr_freq;
						tp.sat_id = search_class.search_sat_id_for_prog;
						if(delivery_info[j].ter.bandwidth == 0)
						{
							tp.tp_t.bandwidth = BANDWIDTH_8_MHZ;
						}
						else if(delivery_info[j].ter.bandwidth == 1)
						{
							tp.tp_t.bandwidth = BANDWIDTH_7_MHZ;
						}
						else
						{
							tp.tp_t.bandwidth = BANDWIDTH_8_MHZ;
						}
						tp_exist_count = search_cache_tp_exist_check(tp.sat_id,
																		tp.frequency,
																		0,
																		0,
																		0,
																		&tp_id);
						if(tp_exist_count == 0)
						{
							ret = GxBus_PmTpAdd(&tp);
							if(ret == GXCORE_SUCCESS)
							{
								search_cache_tp_add(tp);
								search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp.id;
								search_class.search_nit_tp_count++;
								break;
							}
							else if(ret == GX_PM_DBASE_FULL)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_DBASE_FULL;
							}
							else if(ret == GXCORE_ERROR)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save err!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_ERR;
							}
						}
					}
					break;
				case GX_SEARCH_FRONT_C:
					if(delivery_info[j].delivery_type == CABLE_DELIVERY_SYSTEM_DESC)
					{
						tp.frequency = delivery_info[j].ter.centr_freq/1000;
						tp.sat_id = search_class.search_sat_id_for_prog;
						tp.tp_c.symbol_rate = delivery_info[j].cab.sym_rate;

						if(delivery_info[j].cab.modulation == 0x1)
						{
							tp.tp_c.modulation= DVBC_16QAM;
						}
						else if(delivery_info[j].cab.modulation == 0x2)
						{
							tp.tp_c.modulation= DVBC_32QAM;
						}
						else if(delivery_info[j].cab.modulation == 0x3)
						{
							tp.tp_c.modulation= DVBC_64QAM;
						}
						else if(delivery_info[j].cab.modulation == 0x4)
						{
							tp.tp_c.modulation= DVBC_128QAM;
						}
						else if(delivery_info[j].cab.modulation == 0x5)
						{
							tp.tp_c.modulation= DVBC_256QAM;
						}
						else
						{
							tp.tp_c.modulation= DVBC_32QAM;
						}
						tp_exist_count = search_cache_tp_exist_check(tp.sat_id,
																		tp.frequency,
																		tp.tp_c.symbol_rate,
																		0,
																		tp.tp_c.modulation,
																		&tp_id);
						if(tp_exist_count == 0)
						{
							ret = GxBus_PmTpAdd(&tp);
							if(ret == GXCORE_SUCCESS)
							{
								search_cache_tp_add(tp);
								search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp.id;
								search_class.search_nit_tp_count++;
								break;
							}
							else if(ret == GX_PM_DBASE_FULL)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_DBASE_FULL;
							}
							else if(ret == GXCORE_ERROR)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save err!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_ERR;
							}
						}
						else
						{
							search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp_id;
							search_class.search_nit_tp_count++;
						}
					}
					break;

				case GX_SEARCH_FRONT_DTMB:
					if(delivery_info[j].delivery_type == TERRESTRIAL_DELIEVERY_SYSTEM_DESC)
					{
						tp.frequency = delivery_info[j].dtmb.centr_freq/1000;
						tp.sat_id = search_class.search_sat_id_for_prog;

						tp_exist_count = search_cache_tp_exist_check(tp.sat_id,
																		tp.frequency,
																		tp.tp_dtmb.symbol_rate,
																		0,
																		tp.tp_dtmb.modulation,
																		&tp_id);
						if(tp_exist_count == 0)
						{
							ret = GxBus_PmTpAdd(&tp);
							if(ret == GXCORE_SUCCESS)
							{
								search_cache_tp_add(tp);
								search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp.id;
								search_class.search_nit_tp_count++;
								break;
							}
							else if(ret == GX_PM_DBASE_FULL)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_DBASE_FULL;
							}
							else if(ret == GXCORE_ERROR)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save err!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//?龃砹艘顺鏊阉? 在realse si时会free的
								return GX_SEARCH_ERR;
							}
						}
						else
						{
							search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp_id;
							search_class.search_nit_tp_count++;
						}
					}
					else if(delivery_info[j].delivery_type == CABLE_DELIVERY_SYSTEM_DESC)
					{
						tp.frequency = delivery_info[j].ter.centr_freq/1000;
						tp.sat_id = search_class.search_sat_id_for_prog;

						tp_exist_count = search_cache_tp_exist_check(tp.sat_id,
																		tp.frequency,
																		tp.tp_c.symbol_rate,
																		0,
																		tp.tp_c.modulation,
																		&tp_id);
						if(tp_exist_count == 0)
						{
							ret = GxBus_PmTpAdd(&tp);
							if(ret == GXCORE_SUCCESS)
							{
								search_cache_tp_add(tp);
								search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp.id;
								search_class.search_nit_tp_count++;
								break;
							}
							else if(ret == GX_PM_DBASE_FULL)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save full!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_DBASE_FULL;
							}
							else if(ret == GXCORE_ERROR)
							{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
								GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---nit tp save err!!\n");
#endif
								//GxBus_SiParserBufFree(parse_result->parsed_data);//出错了要退出搜索 在realse si时会free的
								return GX_SEARCH_ERR;
							}
						}
						else
						{
							search_class.search_nit_tp_id[search_class.search_nit_tp_count] = tp_id;
							search_class.search_nit_tp_count++;
						}
					}
					break;

				default:
					break;
			}
		}
	}

	GxBus_SiParserBufFree(parse_result->parsed_data);

	return GX_SEARCH_OK;
}

static status_t gx_search_nit_section_ok(GxParseResult * parse_result)
{
	status_t ret = 0;

	if (parse_result->si_subtable_id !=search_class.search_nit_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}
	ret = gx_search_nit_info_get(parse_result);

	if(ret == GX_SEARCH_DBASE_FULL)
	{
		gx_search_nit_filter_release();
		gx_search_sdt_filter_create();
		gx_search_pat_filter_create();
		GxBus_PmSync(GXBUS_PM_SYNC_TP);
		return GX_SEARCH_OK;
	}
	else if(ret == GX_SEARCH_ERR)
	{
		gx_blind_set_status(GX_SEARCH_ERR);
	}
	return ret;
}

static status_t gx_search_nit_subtable_ok(GxParseResult * parse_result)
{
	status_t ret = 0;

	if (parse_result->si_subtable_id !=search_class.search_nit_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}
	ret = gx_search_nit_info_get(parse_result);

	if(ret == GX_SEARCH_ERR)
	{
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}
	GxBus_PmSync(GXBUS_PM_SYNC_TP);
	gx_search_nit_filter_release();
	gx_search_sdt_filter_create();
	gx_search_pat_filter_create();

	return ret;
}

static status_t gx_search_bat_filter_release(void)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放pat的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = search_class.search_bat_subtable_id.subtable_id;
	GxBus_MessageSendWait(new_msg);

	GxBus_MessageFree(new_msg);	//同步消息 使用完后要free,因为接收的地方是NOT_FREE
	search_class.search_bat_subtable_id.subtable_id = -1;
	return GX_SEARCH_OK;

}

static status_t gx_search_bat_subtable_ok(GxParseResult *parse_result)
{
	if (parse_result->si_subtable_id != search_class.search_bat_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		return GX_SEARCH_ERR;
	}

	gx_search_bat_filter_release();

	return GX_SEARCH_OK;
}
status_t gx_search_pat_filter_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = {
		0
	};

	GxMessage              *new_msg = NULL;

	//status_t                ret = 0;

	SubTable.ts_src = search_class.search_ts_cur;

	SubTable.time_out =  search_class.search_pat_time_out;

	SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;

	SubTable.table_parse_cfg.table_parse_fun = NULL;

	SubTable.si_filter.pid = PAT_PID;

	SubTable.si_filter.match_depth = 1;	//需要匹配0字节和5字节,因此匹配深度为6
	SubTable.si_filter.eq_or_neq = 1;

	SubTable.si_filter.match[0] = PAT_TID;

	SubTable.si_filter.mask[0] = 0xff;

	SubTable.si_filter.match[5] = 0x01;

	SubTable.si_filter.mask[5] = 0x00;

	/*发送创建pat的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
	params_create =
		GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
	*params_create = &SubTable;

	GxBus_MessageSendWait(new_msg);

	if (SubTable.si_subtable_id == -1)	//创建pat失败
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---ceate pat err!!\n");
#endif
		GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}
	search_class.search_pat_subtable_id.subtable_id = SubTable.si_subtable_id;
	search_class.search_pat_subtable_id.request_id = SubTable.request_id;

	GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE

	/*发送开始过滤pat的消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);
	*params_start = search_class.search_pat_subtable_id.subtable_id;
	GxBus_MessageSend(new_msg);
	search_class.search_status &= ~SEARCH_PAT_STOPING;
	return GX_SEARCH_OK;
}

status_t gx_search_pat_filter_release(void)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放pat的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = search_class.search_pat_subtable_id.subtable_id;
	GxBus_MessageSendWait(new_msg);

	GxBus_MessageFree(new_msg);	//同步消息 使用完后要free,因为接收的地方是NOT_FREE
	search_class.search_pat_subtable_id.subtable_id = -1;
	search_class.search_status |= SEARCH_PAT_STOPING;
	return GX_SEARCH_OK;

}

status_t gx_search_pat_info_get(GxParseResult * parse_result)
{
	PatInfo                *pat_info = NULL;

	prog_info_t            *prog_info = NULL;

	uint16_t                i = 0;

	pat_info = (PatInfo *) (parse_result->parsed_data);

	prog_info = (prog_info_t *) (pat_info->prog_info);

	search_class.search_pat_body_count = 0;

	for (i = 0; i <pat_info->prog_count; i++) {
		if(search_class.check_pmtpid_fun(prog_info[i].pmt_pid,prog_info[i].prog_num) == true)
		{
			search_class.search_pat_body_count++;
			search_class.search_pat_body = (GxSearchPatBody *)
				GxCore_Realloc(search_class.search_pat_body,
						sizeof(GxSearchPatBody) * search_class.search_pat_body_count);
			if(search_class.search_pat_body == NULL)
			{
#ifdef GX_BUS_SEARCH_DBUG
				GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---search_class.search_pat_body realloc err !!\n");
#endif

				return GX_SEARCH_ERR;
			}
			else
			{
				search_class.search_pat_body[search_class.search_pat_body_count-1].prog_number = prog_info[i].prog_num;
				search_class.search_pat_body[search_class.search_pat_body_count-1].pmt_pid = prog_info[i].pmt_pid;
				search_class.search_pat_body[search_class.search_pat_body_count-1].ts_id = pat_info->si_info.ts_id;
				search_class.search_pat_body[search_class.search_pat_body_count-1].pat_version =
					pat_info->si_info.ver_number;
			}
		}
	}

	//GxBus_SiParserBufFree(parse_result->parsed_data);在释放pat的时候si里面会free

	return GX_SEARCH_OK;
}

static status_t gx_search_pat_subtable_ok(GxParseResult          *parse_result)
{
	status_t ret = 0;
	if (parse_result->si_subtable_id != search_class.search_pat_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		return GX_SEARCH_ERR;
	}

	gx_search_pat_info_get(parse_result);
	search_class.search_pat_finish_flag = 1;

	gx_search_pat_filter_release();

	if (search_class.search_pat_body_count == 0) {
		gx_blind_set_status(GX_SEARCH_OK);
		return GX_SEARCH_OK;
	}

	ret = gx_search_pmt_filter_create();
	if(GX_SEARCH_OK != ret)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---pmt create faile!!\n");
#endif
	}
	return ret;
}

int32_t gx_search_pat_find(GxSearchPmtBody search_pmt_body, GxSearchPatBody * pat)
{
	uint32_t                i = 0;

	for (i = 0; i < search_class.search_pat_body_count; i++) {
		if (search_pmt_body.service_id == search_class.search_pat_body[i].prog_number) {
			memcpy(pat, &(search_class.search_pat_body)[i],
					sizeof(GxSearchPatBody));
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---pat pos = %d\n!!", i);
#endif
			return i;
		}
	}
#ifdef GX_BUS_BLIND_SEARCH_DBUG
	GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---pat can't find!!\n");
#endif
	return -1;
}

status_t gx_search_sdt_filter_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = {
		0
	};

	GxMessage              *new_msg = NULL;

	//status_t                ret = 0;

	SubTable.ts_src = search_class.search_ts_cur;

	SubTable.time_out = search_class.search_sdt_time_out;

	SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;

	SubTable.table_parse_cfg.table_parse_fun = NULL;

	SubTable.si_filter.pid = SDT_PID;

	SubTable.si_filter.match_depth = 1;	//需要匹配0字节和5字节,因此匹配深度为6

	SubTable.si_filter.eq_or_neq = 1;

	SubTable.si_filter.match[0] = SDT_ACTUAL_TS_TID;

	SubTable.si_filter.mask[0] = 0xff;

	SubTable.si_filter.match[5] = 0x00;

	SubTable.si_filter.mask[5] = 0x00;

	/*发送创建pat的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
	params_create =
		GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
	*params_create = &SubTable;

	GxBus_MessageSendWait(new_msg);

	if (SubTable.si_subtable_id == -1)	//创建sdt失败
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---create sdt err!\n");
#endif
		GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}

	search_class.search_sdt_subtable_id.subtable_id = SubTable.si_subtable_id;
	search_class.search_sdt_subtable_id.request_id = SubTable.request_id;
	GxBus_MessageFree(new_msg);	//同步消息 把值传回来 使用完后要free,因为接收的地方是NOT_FREE

	/*发送开始过滤sdt的消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);

	*params_start = search_class.search_sdt_subtable_id.subtable_id;

	GxBus_MessageSend(new_msg);
	search_class.search_status &= ~SEARCH_SDT_STOPING;
	return GX_SEARCH_OK;
}

status_t gx_search_sdt_filter_release(void)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放sdt的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = search_class.search_sdt_subtable_id.subtable_id;

	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);
	search_class.search_sdt_subtable_id.subtable_id = -1;
	search_class.search_status |= SEARCH_SDT_STOPING;
	return GX_SEARCH_OK;

}

static void getname(uint8_t *to, uint32_t buf_len, uint8_t *from, uint32_t name_len)
{
	int32_t utf8 = 0, y = 0;
	uint32_t prog_name_length = 0, psize = 0;
	int8_t name_temp[MAX_PROG_NAME];
    int32_t j = 0, i = 0;

	memset(name_temp,0, MAX_PROG_NAME);

	if (name_len >= buf_len)
		prog_name_length = buf_len - 1;
	else
		prog_name_length = name_len;

	if(prog_name_length == 0)
	{
		strcpy((char*)name_temp,"no name");
		prog_name_length = 8;
	}
	else
		memcpy(name_temp, from, prog_name_length);

	for(y = 0; y < prog_name_length; y++) {
		if( name_temp[y]<0x1f||name_temp[y]>0x20)
			break;
	}
	if(name_temp[y] == 0x13 || name_temp[y] == 0x14)
	{
		j = y+1;
		for(i = y+1; i < prog_name_length; i++)
		{
			if(name_temp[i] != 0x00)
			{
				name_temp[j] = name_temp[i];
				j++;
			}
		}
		name_temp[j] = '\0';
	}

	GxBus_ConfigGetInt(GXBUS_SEARCH_UTF8,&utf8,GXBUS_SEARCH_UTF8_NO);
	if(utf8 == GXBUS_SEARCH_UTF8_NO)
		memcpy(to, &(name_temp[y]), prog_name_length-y);
	else if(utf8 == GXBUS_SEARCH_UTF8_YES)
	{
		uint8_t *pstr = NULL;
		if(gxgdi_iconv((const unsigned char *) &(name_temp[y]),&pstr,prog_name_length-y,&psize, NULL) == 0)
		{
			if(psize >= buf_len)
				prog_name_length = buf_len - 1;
			else
				prog_name_length = psize;

			memcpy(to, pstr, prog_name_length);
			GxCore_Free(pstr);
		}
		else
		{
			BLIND_SEARCH_BASE_PRINTF("[SEARCH]---iconv utf8 err!!------\n");
			memcpy(to, &(name_temp[y]), prog_name_length-y);
		}
	}
	to[buf_len - 1] = 0;
}

status_t gx_search_sdt_info_get(GxParseResult * parse_result)
{
	SdtInfo                *sdt_info = NULL;
	service_info_t         *service_info = NULL;
	uint32_t                body_count_in_section = 0;
	uint16_t                i = 0;

	sdt_info = (SdtInfo *) (parse_result->parsed_data);

	service_info = (service_info_t *) (sdt_info->service_info);

	body_count_in_section = sdt_info->service_count;

	if (search_class.search_sdt_body_count + body_count_in_section > MAX_PROG_PER_TP) {

		body_count_in_section = MAX_PROG_PER_TP - search_class.search_sdt_body_count;
	}
	for (i = 0; i < body_count_in_section; i++) {
		search_class.search_sdt_body[search_class.search_sdt_body_count].orig_network_id = sdt_info->orig_network_id;
		search_class.search_sdt_body[search_class.search_sdt_body_count].service_id =
			service_info[i].service_id;

		search_class.search_sdt_body[search_class.search_sdt_body_count].ts_id = sdt_info->tsid;

		search_class.search_sdt_body[search_class.search_sdt_body_count].flag_sch =
			service_info[i].EIT_sch_flag;

		search_class.search_sdt_body[search_class.search_sdt_body_count].flag_pf =
			service_info[i].EIT_pf_flag;

		search_class.search_sdt_body[search_class.search_sdt_body_count].running_status =
			service_info[i].running_status;

		search_class.search_sdt_body[search_class.search_sdt_body_count].flag_free_ca_mode =
			service_info[i].free_ca_mode;

		search_class.search_sdt_body[search_class.search_sdt_body_count].sdt_version = sdt_info->si_info.ver_number;

		if (service_info[i].service_des_count != 0) {
			search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.desc_valid = TRUE;

			search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.service_type =
				service_info[i].service_des_info->service_type;

			memset(search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.service_name, 0 ,MAX_PROG_NAME);
			getname(search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.service_name, MAX_PROG_NAME,
					service_info[i].service_des_info->service_name, service_info[i].service_des_info->service_name_length);

			memset(search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.service_provider_name, 0 ,MAX_PROG_NAME);
			getname(search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.service_provider_name,
					MAX_PROG_NAME, service_info[i].service_des_info->service_provider_name,
					service_info[i].service_des_info->service_provider_name_length);
		} else {
			search_class.search_sdt_body[search_class.search_sdt_body_count].service_desc.desc_valid = FALSE;
		}

		search_class.search_sdt_body[search_class.search_sdt_body_count].component_desc.stream_content_ext = service_info[i].component_info.stream_content_ext;
		search_class.search_sdt_body[search_class.search_sdt_body_count].component_desc.stream_content = service_info[i].component_info.stream_content;
		search_class.search_sdt_body[search_class.search_sdt_body_count].component_desc.component_type = service_info[i].component_info.component_type;
		search_class.search_sdt_body[search_class.search_sdt_body_count].component_desc.component_tag  = service_info[i].component_info.component_tag;
		memcpy(search_class.search_sdt_body[search_class.search_sdt_body_count].component_desc.iso639, service_info[i].component_info.iso639, 3);

		//if()判断caindentify
		//if()判断mulit name
		search_class.search_sdt_body_count++;

	}
	GxBus_SiParserBufFree(parse_result->parsed_data);

	return GX_SEARCH_OK;
}

static status_t gx_search_sdt_section_ok(GxParseResult          *parse_result)
{
	if (parse_result->si_subtable_id !=search_class.search_sdt_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		gx_blind_set_status(GX_SEARCH_ERR);
		return GX_SEARCH_ERR;
	}
	gx_search_sdt_info_get(parse_result);
	return GX_SEARCH_OK;
}

static status_t gx_search_sdt_subtable_ok(GxParseResult          *parse_result)
{
	status_t ret = 0;
	if (parse_result->si_subtable_id != search_class.search_sdt_subtable_id.subtable_id)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---si_subtable_id = %d !!\n",parse_result->si_subtable_id);
#endif
		return GX_SEARCH_ERR;
	}

	gx_search_sdt_info_get(parse_result);
	search_class.search_sdt_finish_flag = 1;

	gx_search_sdt_filter_release();

	if(0 == search_class.search_pat_finish_flag)
		return GX_SEARCH_OK;

	ret = gx_search_prog_add_by_pmt_cache();

	if(1 == search_class.search_pat_finish_flag
			&& 1 == search_class.search_pmt_all_finish_flag
			&& 1 == search_class.search_sdt_finish_flag
                        &&  ret == GX_SEARCH_OK)
	{
		ret = GX_SEARCH_CONTINUE;
	}
	return ret;
}


int32_t gx_search_sdt_find(GxSearchPmtBody search_pmt_body, GxSearchSdtBody * sdt)
{
	int8_t   buff[MAX_PROG_NAME + 3] = {0};
	uint32_t i = 0;

	for (i = 0; i < search_class.search_sdt_body_count; i++) {
		if (search_pmt_body.service_id == search_class.search_sdt_body[i].service_id) {
			memcpy(sdt, &(search_class.search_sdt_body)[i], sizeof(GxSearchSdtBody));
			break;
		}
	}

	if(i == search_class.search_sdt_body_count)
	{
		sdt->sdt_version = 0xff;
		sdt->orig_network_id = 0xffff;
	}

	/*不通过sdt的type判断是radio还是tv因为不准 */
	if (search_pmt_body.video_pid == 0
			|| search_pmt_body.video_pid == 0x1fff) {
		if (search_pmt_body.audio_count == 0) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF
				("[BLIND SEARCH]---audio count = 0!!\n");
#endif
			if(gx_search_noavprog_save_state() == false)
				return -1;
		} else {
			sdt->service_desc.service_type = SI_AUDIO_SERVICE;
		}
	} else {
		sdt->service_desc.service_type = SI_VIDEO_SERVICE;
	}
	if (sdt->service_desc.desc_valid == FALSE)	//没有名字描述
	{
		int32_t len = 0;

		memset(sdt->service_desc.service_name, 0, MAX_PROG_NAME);
		i = (search_class.search_tp_id_for_prog << 16) | search_pmt_body.service_id;
		snprintf((char*)buff, sizeof(buff)-1, "%s%d","CH",i);
		len = (strlen((char *)buff) >= MAX_PROG_NAME)? (MAX_PROG_NAME - 1): strlen((char *)buff);
		strncpy((char *)(sdt->service_desc.service_name), (char*)buff, len);
	}

	return i;
}

status_t gx_search_pmt_filter_create(void)
{
	GxMsgProperty_SiCreate *params_create = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxSubTableDetail        SubTable = { 0 };
	GxMessage              *new_msg = NULL;

	//status_t                ret = 0;

	uint16_t                prog_number = 0;	//相当于service id

	uint16_t                i = 0;

	if (search_class.search_pat_body_count == 0) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---pat body = 0 !!\n");
#endif
		return GX_SEARCH_CONTINUE;
	}

	do {
		SubTable.time_out = search_class.search_pmt_time_out;
		SubTable.ts_src = search_class.search_ts_cur;

		if(search_class.pmt_parse_fun != NULL)
		{
			SubTable.table_parse_cfg.mode = PARSE_WITH_STANDARD;

			SubTable.table_parse_cfg.table_parse_fun = search_class.pmt_parse_fun;
		}
		else
		{
			SubTable.table_parse_cfg.mode = PARSE_STANDARD_ONLY;

			SubTable.table_parse_cfg.table_parse_fun = NULL;
		}
		SubTable.si_filter.pid =
			search_class.search_pat_body[search_class.search_pmt_count].pmt_pid;

		SubTable.si_filter.match_depth = 6;	//需要匹配0字节和5字节,因此匹配深度为6
		SubTable.si_filter.eq_or_neq = 1;

		SubTable.si_filter.match[0] = PMT_TID;

		SubTable.si_filter.mask[0] = 0xff;

		prog_number = search_class.search_pat_body[search_class.search_pmt_count].prog_number;

		SubTable.si_filter.match[3] = ((prog_number >> 8) & 0xff);

		SubTable.si_filter.mask[3] = 0xff;

		SubTable.si_filter.match[4] = (prog_number & 0xff);

		SubTable.si_filter.mask[4] = 0xff;

		SubTable.si_filter.match[5] = 0x00;

		SubTable.si_filter.mask[5] = 0x00;

		/*发送创建pat的subtable消息给si */
		new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
		params_create =
			GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
		*params_create = &SubTable;

		GxBus_MessageSendWait(new_msg);

		if (SubTable.si_subtable_id != -1)	//创建pmt成功
		{
			search_class.search_pmt_subtable_id[search_class.search_pmt_filter_count].subtable_id=
				SubTable.si_subtable_id;
			search_class.search_pmt_subtable_id[search_class.search_pmt_filter_count].request_id=
				SubTable.request_id;
			search_class.search_pmt_filter_count++;
			search_class.search_pmt_count++;
		}
		GxBus_MessageFree(new_msg);

	} while ((SubTable.si_subtable_id != -1)
			&& (search_class.search_pmt_count < search_class.search_pat_body_count));

	/*发送开始过滤pmt的消息给si */
	for (i = 0; i < search_class.search_pmt_filter_count; i++) {
		new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
		params_start =
			GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);

		*params_start =search_class.search_pmt_subtable_id[i].subtable_id;

		GxBus_MessageSend(new_msg);
	}

	search_class.search_pat_subtable_id.subtable_id = -1;//建立pmt的时候pat应该已经释放了
	if(search_class.search_pmt_filter_count==0)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---no pmt can be created!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	search_class.search_status &= ~SEARCH_PMT_STOPING;
	return GX_SEARCH_OK;
}

status_t gx_search_pmt_filter_release(GxMsgProperty_SiRelease * subtable_id)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_SiRelease *params = NULL;
	/*发送释放pat的subtable消息给si */
	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

	*params = *subtable_id;

	GxBus_MessageSend(new_msg);

	*subtable_id = -1;
	return GX_SEARCH_OK;

}

status_t gx_search_pmt_cas_get(uint32_t * cas1,
		uint32_t cas1_count,
		uint32_t * cas2,
		uint32_t cas2_count, uint32_t stream_type)
{
	uint32_t                i = 0;
	CaDescriptor           *cas_decriptor = NULL;
	switch (stream_type) {
		/*case MPEG_1_VIDEO:
		  case MPEG_2_VIDEO:
		  case AVS:
		  case H264:*/
		case 0:
			if (cas2 != NULL)	//以分开加密优先
			{
				cas_decriptor = (CaDescriptor *) (cas2[0]);
				search_class.search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
				search_class.search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
				for (i = 0; i < cas2_count; i++) {
					cas_decriptor = (CaDescriptor *) (cas2[i]);
					if (cas_decriptor->elem_pid ==
							search_class.search_pmt_body.video_pid) {
						if(gx_search_check_ca(cas_decriptor->ca_system_id,search_class.search_pmt_body.video_pid,cas_decriptor->ca_pid))
						{
							search_class.search_pmt_body.ecm_pid_video =
								cas_decriptor->ca_pid;
							search_class.search_pmt_body.pmt_ca_desc.cas_id =
								cas_decriptor->ca_system_id;
							break;
						}
					}

				}
			}
			if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加密
			{
				cas_decriptor = (CaDescriptor *) (cas1[0]);
				search_class.search_pmt_body.ecm_pid_video = cas_decriptor->ca_pid;
				search_class.search_pmt_body.pmt_ca_desc.cas_id = CAS_ID_UNKNOW;
				for (i = 0; i < cas1_count; i++) {
					cas_decriptor = (CaDescriptor *) (cas1[i]);
					//	if (cas_decriptor->elem_pid ==
					//	   search_class.search_pmt_body.video_pid)
					{
						if(gx_search_check_ca(cas_decriptor->ca_system_id,search_class.search_pmt_body.video_pid,cas_decriptor->ca_pid))
						{
							search_class.search_pmt_body.ecm_pid_video =
								cas_decriptor->ca_pid;
							search_class.search_pmt_body.pmt_ca_desc.cas_id =
								cas_decriptor->ca_system_id;
							break;
						}
					}

				}
			}
			break;

			/*	case MPEG_1_AUDIO:
				case MPEG_2_AUDIO:
				case AAC_ADTS:
				case AAC_LATM:*/
		case 1:
			if (cas2 != NULL)	//以分开加密优先
			{
				cas_decriptor = (CaDescriptor *) (cas2[0]);
				search_class.search_stream_body[search_class.search_stream_body_count].ecm_pid =
					cas_decriptor->ca_pid;

				for (i = 0; i < cas2_count; i++) {
					cas_decriptor = (CaDescriptor *) (cas2[i]);
					if (cas_decriptor->elem_pid ==
							search_class.search_stream_body
							[search_class.search_stream_body_count].audio_pid) {
						if(gx_search_check_ca(cas_decriptor->ca_system_id,
									search_class.search_stream_body[search_class.search_stream_body_count].audio_pid,
									cas_decriptor->ca_pid))
						{
							search_class.search_stream_body
								[search_class.search_stream_body_count].ecm_pid =
								cas_decriptor->ca_pid;

							break;
						}
					}

				}
			}
			if (i == cas2_count && cas1 != NULL)	//分开加密没找到,找相同加密
			{
				cas_decriptor = (CaDescriptor *) (cas1[0]);
				search_class.search_stream_body[search_class.search_stream_body_count].ecm_pid =
					cas_decriptor->ca_pid;

				for (i = 0; i < cas1_count; i++) {
					cas_decriptor = (CaDescriptor *) (cas1[i]);
					//if (cas_decriptor->elem_pid ==
					//    search_class.search_stream_body
					//    [search_class.search_stream_body_count].audio_pid)
					{
						if(gx_search_check_ca(cas_decriptor->ca_system_id,
									search_class.search_stream_body[search_class.search_stream_body_count].audio_pid,
									cas_decriptor->ca_pid))
						{
							search_class.search_stream_body
								[search_class.search_stream_body_count].ecm_pid =
								cas_decriptor->ca_pid;

							break;
						}
					}

				}
			}
			break;
	}
	return GX_SEARCH_OK;
}

status_t gx_search_pmt_info_get(GxParseResult * parse_result)
{
	uint32_t               *cas_info = NULL;
	uint32_t               *cas_info2 = NULL;
	PmtInfo                *pmt_info = NULL;
	stream_info_t          *stream_info = NULL;
	Ac3Descriptor          *ac3_descriptor = NULL;
	Eac3Descriptor         *eac3_descriptor = NULL;
	TeletextDescriptor     *ttx_descriptor = NULL;
	Teletext               *ttx_info = NULL;
	uint32_t                stream_info_count = 0;
	uint32_t                i = 0;
	uint32_t                j = 0;
	uint32_t                stream_type = 0;
	status_t                ret = 0;
	uint32_t video_audio = 0;

	pmt_info = (PmtInfo *) (parse_result->parsed_data);
	stream_info_count = (pmt_info->stream_count) + (pmt_info->ac3_count) +( pmt_info->eac3_count);	//加上ac3的空间
	stream_info = (stream_info_t *) (pmt_info->stream_info);

	search_class.search_stream_body =
		(GxBusPmDataStream *) GxCore_Malloc(sizeof(GxBusPmDataStream) *
				stream_info_count);
	memset(search_class.search_stream_body, 0, sizeof(GxBusPmDataStream) *stream_info_count);
	search_class.search_stream_body_count = 0;
	search_class.search_video_stream_body =
		(GxBusPmDataVideoStream *) GxCore_Malloc(sizeof(GxBusPmDataVideoStream) * stream_info_count);
	memset(search_class.search_video_stream_body, 0, sizeof(GxBusPmDataVideoStream) *stream_info_count);
	search_class.search_pmt_body.video_count = 0;

	search_class.search_pmt_body.pcr_pid = pmt_info->pcr_pid;
	search_class.search_pmt_body.service_id = pmt_info->prog_num;

	if (pmt_info->ca_count != 0)	//先判断音视频是不是同密
	{
		search_class.search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
		cas_info = pmt_info->ca_info;
	}
	if (pmt_info->ca_count2 != 0)	//再判断音视频是不是分开加密
	{
		search_class.search_pmt_body.pmt_ca_desc.desc_valid = TRUE;
		cas_info2 = pmt_info->ca_info2;
	}
	search_class.search_pmt_body.video_pid = 0;//避免上一张pmt的数据残留
	for (i = 0; i <pmt_info->stream_count ; i++)	//这里只获得pcm的信息
	{
		video_audio = 2;
		stream_type = stream_info[i].stream_type;
		switch (stream_type) {
			case MPEG_1_VIDEO:
			case MPEG_2_VIDEO:
				search_class.search_pmt_body.service_type = GXBUS_PM_PROG_MPEG;
				video_audio = 0;
				break;
			case MPEG_4_VIDEO:
				search_class.search_pmt_body.service_type = GXBUS_PM_PROG_MPEG4;
				video_audio = 0;
				break;
			case H264:
				search_class.search_pmt_body.service_type = GXBUS_PM_PROG_H264;
				video_audio = 0;
				break;
			case H265:
				search_class.search_pmt_body.service_type = GXBUS_PM_PROG_H265;
				video_audio = 0;
				break;
			case AVS:
				search_class.search_pmt_body.service_type = GXBUS_PM_PROG_AVS;
				video_audio = 0;
				break;
			case MPEG_1_AUDIO:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG1;
				video_audio = 1;
				break;
			case MPEG_2_AUDIO:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_MPEG2;
				video_audio = 1;
				break;
			case AAC_ADTS:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_ADTS;
				video_audio = 1;
				break;
			case AAC_LATM:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AAC_LATM;
				video_audio = 1;
				break;
				/*	case PRIVATE_PES_STREAM:  //出现在pmt中一般是EAC3
					search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
					video_audio = 1;
					break;*/
			case LPCM:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_LPCM;
				video_audio = 1;
				break;
			case AC3:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
				video_audio = 1;
				break;
			case DTS:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES)
					{
						search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS;
						video_audio = 1;
					}
				}
				break;
			case DOLBY_TRUEHD:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
				video_audio = 1;
				break;
			case AC3_PLUS:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS;
				video_audio = 1;
				break;
			case DTS_HD:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES)
					{
						search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD;
						video_audio = 1;
					}
				}
				break;
			case DTS_MA:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES)
					{
						search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_MA;
						video_audio = 1;
					}
				}
				break;
/*
			case AC3_PLUS_SEC:
				search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
				video_audio = 1;
				break;
*/
			case DTS_HD_SEC:
				{
					int32_t dts = 0;
					GxBus_ConfigGetInt(GXBUS_SEARCH_DTS_SUPPORT,&dts,GXBUS_SEARCH_DTS_YES);
					if(dts == GXBUS_SEARCH_DTS_YES)
					{
						search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DTS_HD_SEC;
						video_audio = 1;
					}
				}
				break;
			default:
				{
					uint8_t *d = pmt_info->registration;

					if (d[0] == 'D' && d[1] == 'R' && d[2] == 'A' && d[3] == '1') {
						search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_DRA;
						video_audio = 1;
					}
					else {
#ifdef GX_BUS_SEARCH_DBUG
						SEARCH_BASE_PRINTF("[SEARCH]---can't support stream type!!type = %x\n",stream_type);
#endif
						video_audio = 2;
					}
				}
				break;
		}
		if(video_audio == 0)
		{
			search_class.search_pmt_body.video_pid = stream_info[i].elem_pid;
			gx_search_pmt_cas_get(cas_info,
					pmt_info->ca_count,
					cas_info2,
					pmt_info->ca_count2,
					video_audio);
			search_class.search_video_stream_body[search_class.search_pmt_body.video_count].video_pid=
				stream_info[i].elem_pid;
			search_class.search_video_stream_body[search_class.search_pmt_body.video_count].video_type=
				search_class.search_pmt_body.service_type;
			if(search_class.search_pmt_body.video_count>0)
			{
				search_class.search_pmt_body.video_pid = search_class.search_video_stream_body[0].video_pid;
				search_class.search_pmt_body.service_type = search_class.search_video_stream_body[0].video_type;
			}
			search_class.search_pmt_body.video_count++;
			j = 1;
		}
		else if(video_audio == 1)
		{
			search_class.search_stream_body[search_class.search_stream_body_count].audio_pid =
				stream_info[i].elem_pid;


			gx_search_pmt_cas_get(cas_info,pmt_info->ca_count,cas_info2,pmt_info->ca_count2,video_audio);
			memset(search_class.search_stream_body[search_class.search_stream_body_count].name, 0, 4);
            if ((stream_info[i].iso639_count != 0) && (stream_info[i].iso639_info)){
                memcpy(search_class.search_stream_body[search_class.search_stream_body_count].name,stream_info[i].iso639_info[0].iso639, 3);
			}
			search_class.search_pmt_body.audio_count++;	//不包括ac3
			search_class.search_stream_body_count++;	//包括ac3
			j = 1;
		}
	}
	if(j == 0 && (gx_search_noavprog_save_state() == false))
	{
		ret = GX_SEARCH_ERR;
		goto end;
	}
#define MAX_AUDIO_DES_COUNT (32)
    if ((pmt_info->ac3_count > 0) && (pmt_info->ac3_info)){
        for (i = 0; i < pmt_info->ac3_count; i++) {
            ac3_descriptor = (Ac3Descriptor *) (pmt_info->ac3_info[i]);
            if(search_class.search_stream_body_count< MAX_AUDIO_DES_COUNT){
                bool flag=true;
                for(j=0; j<search_class.search_stream_body_count; j++){
                    if(ac3_descriptor->elem_pid == search_class.search_stream_body[j].audio_pid){
                        flag=false;
                        break;
                    }
                }
                if(flag){
                    search_class.search_stream_body[search_class.search_stream_body_count].audio_pid = ac3_descriptor->elem_pid;
                    search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;
                    for (j = 0; j < pmt_info->stream_count; j++){
                        if(stream_info[j].elem_pid == ac3_descriptor->elem_pid){
                            if((stream_info[j].iso639_count != 0) && (stream_info[j].iso639_info)){
                                memcpy(search_class.search_stream_body[search_class.search_stream_body_count].name,stream_info[j].iso639_info[0].iso639, 3);
                            }
                            break;
                        }
                    }
                    search_class.search_stream_body_count++;
                }
            }
        }
    }
    if((pmt_info->eac3_count > 0) && (pmt_info->eac3_info)){
        for (i = 0; i < pmt_info->eac3_count; i++) {
            eac3_descriptor = (Eac3Descriptor *) (pmt_info->eac3_info[i]);
            if(search_class.search_stream_body_count< MAX_AUDIO_DES_COUNT){
                bool flag=true;
                for(j=0; j<search_class.search_stream_body_count; j++){
                    if(eac3_descriptor->elem_pid == search_class.search_stream_body[j].audio_pid){
                        flag=false;
                        break;
                    }
                }
                if(flag){
                    search_class.search_stream_body[search_class.search_stream_body_count].audio_pid = eac3_descriptor->elem_pid;
                    search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
                    for (j = 0; j < pmt_info->stream_count; j++){
                        if(stream_info[j].elem_pid == eac3_descriptor->elem_pid){
                            if((stream_info[j].iso639_count != 0) && (stream_info[j].iso639_info)){
                                memcpy(search_class.search_stream_body[search_class.search_stream_body_count].name,stream_info[j].iso639_info[0].iso639, 3);
                            }
                            break;
                        }
                    }
                    search_class.search_stream_body_count++;
                }
            }
        }
    }
	search_class.search_pmt_body.pmt_subt_desc.desc_valid = FALSE;
	if (pmt_info->subt_count != 0)	//GxSearchPmtSubtitlingDesc等待si服务
	{
		search_class.search_pmt_body.pmt_subt_desc.desc_valid = TRUE;
	}

	search_class.search_pmt_body.pmt_ttx_desc.ttx_desc_valid = FALSE;
	search_class.search_pmt_body.pmt_ttx_desc.cc_desc_valid = FALSE;
	if (pmt_info->ttx_count != 0) {
		search_class.search_pmt_body.pmt_ttx_desc.ttx_desc_valid = TRUE;

		for (i = 0; i < pmt_info->ttx_count; i++) {
			ttx_descriptor =
				(TeletextDescriptor *) (pmt_info->ttx_info[i]);//因为沈斌的ttx_info是一个类似数组的东西，数组的元素是每一个ttx描俗的地址

			for (j = 0; j < ttx_descriptor->ttx_num; j++) {
				if(ttx_descriptor->ttx)
					ttx_info = (Teletext*)(((Teletext*)ttx_descriptor->ttx) + j);
				else
				{
					gxlogd("\nBLIND SEARCH, ttx descriptor error\n");
					break;
				}
				if (ttx_info->type == 0x02
						//|| ttx_info->type == 0x01   /*cc flag shenbin 0705*/
						||ttx_info->type == 0x05)	//0x02或者0x5是cc类型的ttx
				{
					search_class.search_pmt_body.pmt_ttx_desc.cc_desc_valid = TRUE;
					break;
				}
			}
			if (search_class.search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
				break;
			}
		}
	}

	search_class.search_pmt_body.pmt_ac3_desc.desc_valid = FALSE;
	if (pmt_info->ac3_count != 0)	//GxSearchPmtAc3Desc等待si服务search_stream_body_count++
	{
		search_class.search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

		for (i = 0; i < pmt_info->ac3_count; i++) {
			ac3_descriptor =
				(Ac3Descriptor *) (pmt_info->ac3_info[i]);
			search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_AC3;	//ac3
			search_class.search_stream_body[search_class.search_stream_body_count].audio_pid =
				ac3_descriptor->elem_pid;
			memset(search_class.search_stream_body
					[search_class.search_stream_body_count].name, 0, 4);
			search_class.search_stream_body_count++;
		}
	}
	if (pmt_info->eac3_count != 0)	//GxSearchPmtAc3Desc等待si服务search_stream_body_count++
	{
		search_class.search_pmt_body.pmt_ac3_desc.desc_valid = TRUE;

		for (i = 0; i < pmt_info->eac3_count; i++) {
			eac3_descriptor =
				(Eac3Descriptor *) (pmt_info->eac3_info[i]);
			search_class.search_stream_body[search_class.search_stream_body_count].audio_type = GXBUS_PM_AUDIO_EAC3;
			search_class.search_stream_body[search_class.search_stream_body_count].audio_pid =
				eac3_descriptor->elem_pid;
			memset(search_class.search_stream_body
					[search_class.search_stream_body_count].name, 0, 4);
			search_class.search_stream_body_count++;
		}
	}
	//if()GxSearchPmtServiceMoveDesc等待si服务
	if(1 == search_class.search_sdt_finish_flag)
	{
		ret = gx_search_prog_info_add(search_class.search_stream_body, search_class.search_stream_body_count, search_class.search_pmt_body);
		memset(&(search_class.search_pmt_body), 0, sizeof(GxSearchPmtBody));

		if ((ret == GX_SEARCH_ERR)||(ret == GX_SEARCH_DBASE_FULL)) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---add prog err!!ret = %x\n",ret);
#endif
			goto end;
		}
		else if(ret == GX_SEARCH_PROG_EXIST)
		{
			goto reply;
		}
		else if(ret == GX_SEARCH_PROG_CONDITION_ERR)
		{
			ret = GX_SEARCH_OK;
			goto end;
		}
reply:
		/*发送找到新节目的消息给应用 */
		gx_search_new_prog_reply(search_class.search_new_prog_get);

		ret = GX_SEARCH_OK;
	}
	else
	{
		ret = gx_search_pmt_cache_add(&search_class.pmt_stream_cache_list,
										search_class.search_stream_body,
										search_class.search_stream_body_count,
										search_class.search_pmt_body);
		memset(&(search_class.search_pmt_body), 0, sizeof(GxSearchPmtBody));
	}

end:
	search_class.search_stream_body_count = 0;
	if(search_class.search_stream_body)  // 释放stream body 为下一个节目做准备
	{
		GxCore_Free(search_class.search_stream_body);
		search_class.search_stream_body = NULL;
	}
	search_class.search_pmt_body.video_count = 0;
	memset(&search_class.search_pmt_body, 0, sizeof(GxSearchPmtBody));
	if(search_class.search_video_stream_body)  // 释放stream video body 为下一个节目做准备
	{
		GxCore_Free(search_class.search_video_stream_body);
		search_class.search_video_stream_body = NULL;
	}
	return ret;
}

static status_t gx_search_pmt_subtable_ok(GxParseResult * parse_result)
{
	uint32_t i = 0;
	status_t ret = 0;

	for (i = 0; i < FILTER_MAX; i++)
	{

		if (parse_result->si_subtable_id == search_class.search_pmt_subtable_id[i].subtable_id)
		{

			ret = gx_search_pmt_info_get(parse_result);	//要对返回值做严格判断
			if (ret == GX_SEARCH_DBASE_FULL)
			{
				gx_search_error_reply(SEARCH_DBASE_OVERFLOW);
				/*保存并且还原各个变量 */
				//GxBus_PmTransactionCommit();
				//GxBus_SiParserBufFree(parse_result->parsed_data);//释放的时候由si free
				gx_blind_set_status(GX_SEARCH_FINISH);
				return GX_SEARCH_DBASE_FULL;
			}
			else
			{
				search_class.search_pmt_finish_count++;
				if (search_class.search_pmt_count < search_class.search_pat_body_count)	//还有pmt没解析
				{
					gx_search_pmt_next_filte_start(parse_result);

				}
				else
				{
					if(search_class.search_pmt_finish_count == search_class.search_pat_body_count)
						search_class.search_pmt_all_finish_flag = 1;

					/*全部pmt解析完成并且sdt解析完成*/
					if (1 == search_class.search_pmt_all_finish_flag && 1 == search_class.search_sdt_finish_flag)
					{
						gx_blind_set_status(GX_SEARCH_OK);
						return GX_SEARCH_OK;//这里返回是为了避免GxBus_SiParserBufFree(parse_result->parsed_data);因为gx_search_variable_next_tp_init会释放si，si里面会释放buffer了
					}
					//if not free by GXMSG_SI_SUBTABLE_RELEASE, free by GxBus_SiParserBufFree
					GxBus_SiParserBufFree(parse_result->parsed_data);
				}
			}
			return GX_SEARCH_OK;
		}

	}

	return GX_SEARCH_ERR;
}

static status_t gx_search_pmt_next_filte_start(GxParseResult * parse_result)
{
	GxMessage              *new_msg = NULL;
	GxSubTableDetail      **params = NULL;
	GxMsgProperty_SiStart  *params_start = NULL;
	GxMsgProperty_SiCreate *params_create = NULL;

	GxSubTableDetail        subtable = { 0 };
	uint16_t                prog_number = 0;	//相当于service id
	int16_t                 subtable_id = 0;
	uint32_t                i = 0;

	subtable_id = subtable.si_subtable_id = parse_result->si_subtable_id;

	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_GET);
	params = GxBus_GetMsgPropertyPtr(new_msg, GxSubTableDetail *);
	*params = &subtable;
	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);

	subtable.si_filter.pid = search_class.search_pat_body[search_class.search_pmt_count].pmt_pid;
	prog_number = search_class.search_pat_body[search_class.search_pmt_count].prog_number;

	subtable.si_filter.match[3] = ((prog_number >> 8) & 0xff);

	subtable.si_filter.mask[3] = 0xff;

	subtable.si_filter.match[4] = (prog_number & 0xff);

	subtable.si_filter.mask[4] = 0xff;

	search_class.search_pmt_count++;

	for (i = 0; i < FILTER_MAX; i++) {
		if (search_class.search_pmt_subtable_id[i].subtable_id == subtable_id) {
			GxMessage              *new_msg = NULL;
			GxMsgProperty_SiRelease *params = NULL;
			new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
			params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);

			*params = subtable_id;
			GxBus_MessageSendWait(new_msg);
			GxBus_MessageFree(new_msg);
			search_class.search_pmt_subtable_id[i].subtable_id = -1;
			//gxlogd("[BUS]: %s[%d] sub: %d \n", __func__, __LINE__, subtable_id);
			break;
		}
	}
	if( i == FILTER_MAX)
	{
		for(i = 0; i < FILTER_MAX; i++)
		{
			if(search_class.search_pmt_subtable_id[i].subtable_id == -1)
			{
				break;
			}
		}
	}

	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
	params_create = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
	*params_create = &subtable;

	GxBus_MessageSendWait(new_msg);
	GxBus_MessageFree(new_msg);
	if (subtable.si_subtable_id != -1)
	{
		search_class.search_pmt_subtable_id[i].subtable_id = subtable.si_subtable_id;
		search_class.search_pmt_subtable_id[i].request_id = subtable.request_id;
		//gxlogd("[BUS]: %s[%d] sub[%d]: %d req[%d]: %d, pmtid[%d]: 0x%x\n", __func__, __LINE__,
		//		search_class.search_pmt_count - 1, search_class.search_pmt_subtable_id[i].subtable_id,
		//		search_class.search_pmt_count - 1, search_class.search_pmt_subtable_id[i].request_id,
		//		search_class.search_pmt_count - 1, search_class.search_pat_body[search_class.search_pmt_count - 1].pmt_pid);
	}
	else
	{
		gxlogd("\n\033[34mERROR, %s, %d\033[0m\n", __func__, __LINE__);
		search_class.search_pmt_count--;
		return GX_SEARCH_ERR;
	}

	new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
	params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);
	*params_start = subtable.si_subtable_id;

	GxBus_MessageSend(new_msg);
	return GX_SEARCH_OK;
}

static GxBusPmDataProgDefinition _prog_definition_get(GxSearchSdtBody *sdt_info)
{
	GxBusPmDataProgDefinition ret = GXBUS_PM_PROG_SD;

	if(NULL == sdt_info)
		return GXBUS_PM_PROG_SD;

	GxSearchSdtComponentDesc *component_desc = &sdt_info->component_desc;

    if(SI_MPEG2_HD_VIDEO_SERVICE == sdt_info->service_desc.service_type
			|| (sdt_info->service_desc.service_type >= SI_H264_HD_VIDEO_SERVICE
				&& sdt_info->service_desc.service_type <= SI_HEVC_UHD_VIDEO_SERVICE))
		return GXBUS_PM_PROG_HD;

    switch(component_desc->stream_content)
	{
		case 0x1:
			if(component_desc->component_type >= 0x9 && component_desc->component_type <= 0x10)
				ret = GXBUS_PM_PROG_HD;
			break;

		case 0x5:
			if(0xb == component_desc->component_type
					|| 0xc == component_desc->component_type
					|| 0xf == component_desc->component_type
					|| 0x10 == component_desc->component_type
					|| (component_desc->component_type >= 0x80 && component_desc->component_type <= 0x83))
				ret = GXBUS_PM_PROG_HD;
			break;

		case 0x9:
			if(0x0 == component_desc->stream_content_ext)
				ret = GXBUS_PM_PROG_HD;
			break;
	}

	return ret;
}

static status_t gx_search_prog_info_add(GxBusPmDataStream *search_stream_body,
										uint32_t search_stream_body_count,
										GxSearchPmtBody search_pmt_body)
{
	status_t                ret = 0;
	bool                    exist = 0;
	uint16_t                ts_id = 0;
	uint16_t                service_id = 0;
	int32_t                 num = 0;
	GxSearchSdtBody         sdt_info = { 0 };
	GxSearchPatBody         pat_info = { 0 };
	GxBusPmDataProg         prog = { 0 };
	uint32_t i = 0;
	GxBusPmDataProgAudioType type = 0xff;

	prog.data_plp_id      = 0xff;
	prog.common_plp_exist = 0xff;
	prog.common_plp_id    = 0xff;
	prog.common_status    = 0xff;
	prog.muti_ts_id       = 0xff;
	prog.muti_ts_exist    = 0x0;
	prog.muti_ts_code     = 0xff;
	prog.pdmx_flag        = 0x0;
	prog.pdmx_pid         = 0x1fff;

	if(NULL == search_stream_body)
		return GX_SEARCH_ERR;

	if (search_class.search_transaction_bgin_flag == 0) {
		//GxBus_PmTransactionBegin();//开始数据库事务,在出错的时候 或者客户选择保存的时候或者放弃的时候,做相应的提交或者回滚?
		search_class.search_transaction_bgin_flag = 1;
	}

	/*寻找该节目对应的sdt,sdt_num -1错误  其他值正常 */
	num = gx_search_sdt_find(search_pmt_body, &sdt_info);
	if (num == -1) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---find sdt err!!\n");
#endif
		return GX_SEARCH_PROG_CONDITION_ERR;
	}

	/*寻找该节目的pat */
	num = gx_search_pat_find(search_pmt_body, &pat_info);
	if (num == -1)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---find pat err!!\n");
#endif
		return GX_SEARCH_PROG_CONDITION_ERR;
	}
	ts_id = pat_info.ts_id;
	service_id = search_pmt_body.service_id;
	/*检测当前节目符不符合搜索条件*/
	if (sdt_info.service_desc.service_type == SI_AUDIO_SERVICE) {
		prog.service_type = GXBUS_PM_PROG_RADIO;
		search_class.search_new_prog_get.type = GXBUS_PM_PROG_RADIO;	//填充上传的信息
	} else {
		prog.service_type = GXBUS_PM_PROG_TV;
		search_class.search_new_prog_get.type = GXBUS_PM_PROG_TV;	//填充上传的信息
	}

	prog.definition = _prog_definition_get(&sdt_info);

	memcpy(search_class.search_new_prog_get.name, sdt_info.service_desc.service_name, MAX_PROG_NAME);	//填充上传的信息
	search_class.search_new_prog_get.name[MAX_PROG_NAME-1] = 0;
	search_class.search_new_prog_get.flag = GXBUS_PM_PROG_NOT_EXIST;
	search_class.search_new_prog_get.service_id = service_id;
	search_class.search_new_prog_get.ts_id = ts_id;
	search_class.search_new_prog_get.original_id = sdt_info.orig_network_id;
	prog.sdt_version = sdt_info.sdt_version;



	prog.video_type = search_pmt_body.service_type;
	if (search_pmt_body.pmt_ca_desc.desc_valid == TRUE) {
		prog.scramble_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		prog.cas_id = search_pmt_body.pmt_ca_desc.cas_id;
		prog.ecm_pid_v = search_pmt_body.ecm_pid_video;
	} else {
		prog.scramble_flag = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cas_id = CAS_ID_FREE;
	}
	prog.video_pid = search_pmt_body.video_pid;
	prog.pcr_pid = search_pmt_body.pcr_pid;
	search_class.search_new_prog_get.scramble_flag = prog.scramble_flag;

	switch(search_class.search_tv_radio)
	{
		case GX_SEARCH_TV:
			if(prog.service_type != GXBUS_PM_PROG_TV)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---tv radio err!!\n");
#endif
				return GX_SEARCH_PROG_CONDITION_ERR;
			}
			break;
		case GX_SEARCH_RADIO:
			if(prog.service_type != GXBUS_PM_PROG_RADIO)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---tv radio err!!\n");
#endif
				return GX_SEARCH_PROG_CONDITION_ERR;
			}
			break;

		default:
			break;
	}
	switch(search_class.search_fta_cas)
	{
		case GX_SEARCH_FTA:
			if(prog.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---cas fta err!!\n");
#endif
				return GX_SEARCH_PROG_CONDITION_ERR;
			}
			break;

		case GX_SEARCH_CAS:
			if(prog.scramble_flag == GXBUS_PM_PROG_BOOL_DISABLE)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---cas fta err!!\n");
#endif
				return GX_SEARCH_PROG_CONDITION_ERR;
			}
			break;

		default:
			break;
	}

	/*pid scan事需要比对搜索下来的pid*/
	if(search_class.search_type == GX_SEARCH_PID)
	{
		if(prog.video_pid == search_class.search_pid_params.video_pid
				||prog.pcr_pid == search_class.search_pid_params.pcr_pid)
		{
			for(i=0; i<search_stream_body_count; i++)
			{
				if(search_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3
						&&search_stream_body[i].audio_pid == search_class.search_pid_params.audio_pid)
				{
					prog.cur_audio_pid  = search_stream_body[i].audio_pid;
					prog.cur_audio_type = search_stream_body[i].audio_type;
					goto pm_prog_save;
				}
			}
		}
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---pid err!!\n");
#endif
		return GX_SEARCH_PROG_CONDITION_ERR;
	}
	/*检测当前节目存不存在 */
pm_prog_save:

	if(BLIND_SUPER == search_class.search_blind_type)
		search_multistream_get_params((void *)&prog);

	if(BLIND_SUPER == search_class.search_blind_type)
		search_pdmx_get_params((void*)&prog);

	exist = search_cache_prog_exist_check(search_class.search_sat_id_for_prog,
											search_class.search_tp_id_for_prog,
											service_id,
											prog.muti_ts_id,
											prog.pdmx_flag,
											prog.pdmx_pid,
											prog.data_plp_id,
											prog.common_plp_exist,
											prog.common_plp_id,
											prog.common_status);
	if (true == exist) {
		search_class.search_new_prog_get.flag = GXBUS_PM_PROG_EXIST;
		return GX_SEARCH_PROG_EXIST;
	}
	/*建立节目信息结构 */
	prog.tp_id = search_class.search_tp_id_for_prog;
	prog.sat_id = search_class.search_sat_id_for_prog;
	prog.service_id = service_id;
	prog.audio_volume = 50;
	prog.audio_mode = GXBUS_PM_PROG_AUDIO_MODE_LEFT;

	prog.skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	prog.lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	if (search_pmt_body.pmt_ttx_desc.ttx_desc_valid == TRUE) {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		if (search_pmt_body.pmt_ttx_desc.cc_desc_valid == TRUE) {
			prog.cc_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		}
	} else {
		prog.ttx_flag = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cc_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	if (search_pmt_body.pmt_subt_desc.desc_valid == TRUE) {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_ENABLE;
	} else {
		prog.subt_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}
	prog.favorite_flag = 0;
	prog.pmt_pid = pat_info.pmt_pid;
	prog.pat_version = pat_info.pat_version;
	prog.bouquet_id = 0;	//等待si解析bat表
	prog.pmt_version = 0;	//需要监控的话用初始0版本也是可以的
	prog.audio_count = search_pmt_body.audio_count;	//不包括ac3
	prog.audio_level = GXBUS_PM_PROG_AUDIO_LECEL_MID;
	if (search_pmt_body.pmt_ac3_desc.desc_valid == TRUE) {
		prog.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
		for(i=0; i<search_stream_body_count; i++)
		{
			if(search_stream_body[i].audio_type == GXBUS_PM_AUDIO_AC3)//ac3
			{
				prog.ac3_pid = search_stream_body[i].audio_pid;
				prog.cur_audio_type= GXBUS_PM_AUDIO_AC3;
				break;
			}
			else if(search_stream_body[i].audio_type == GXBUS_PM_AUDIO_EAC3)//eac3
			{
				prog.ac3_pid = search_stream_body[i].audio_pid;
				prog.cur_audio_type= GXBUS_PM_AUDIO_EAC3;
				//	break;
			}
		}

	} else {
		prog.ac3_flag = GXBUS_PM_PROG_BOOL_DISABLE;
	}

	gx_search_prog_check_audio_lang(&(prog.cur_audio_pid),
									&type,//&(prog.cur_audio_type),
									&(prog.cur_audio_ecm_pid),
									search_stream_body,
									search_stream_body_count);
	if(type != 0xff)
	{
		prog.cur_audio_type = type;
	}
	prog.original_id = sdt_info.orig_network_id;
	memcpy(prog.prog_name, sdt_info.service_desc.service_name,
			MAX_PROG_NAME);
	memcpy(prog.u2.service_provider_name, sdt_info.service_desc.service_provider_name, MAX_PROG_NAME);
	prog.prog_name[MAX_PROG_NAME -1] = 0;
#ifdef GX_BUS_BLIND_SEARCH_DBUG
	BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---name = %s\n", prog.prog_name);
#endif
	prog.ts_id = ts_id;
	prog.tuner = search_class.search_tuner_num_for_prog;

	if(search_class.modify_prog != NULL)
	{
		ret = search_class.modify_prog(&prog);
		if(ret != GX_SEARCH_OK)
		{
			return GX_SEARCH_ERR;
		}
	}
	ret = GxBus_PmProgAdd(&prog);
	if (ret == GX_PM_DBASE_FULL) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---dbase full!!\n");
#endif
		return GX_SEARCH_DBASE_FULL;
	} else if (ret == GXCORE_ERROR) {
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF
			("[BLIND SEARCH]---add prog to dbase err!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	search_cache_prog_add(prog);
	search_class.search_prog_id_arry[search_class.search_prog_id_num] = prog.id;
	search_class.search_prog_id_num++;

	return GX_SEARCH_OK;
}

static void  gx_search_prog_check_audio_lang(uint16_t* audio_pid,
												GxBusPmDataProgAudioType* type,
												uint16_t* ecm_pid,
												GxBusPmDataStream *search_stream_body,
												uint32_t search_stream_body_count)
{
	uint32_t i = 0;
	uint32_t j = 0;
	int8_t lang_buff[128];
	int8_t lang[3][4];
	int8_t flag = -1;//-1代表还没有赋值，0代表没有找到优先语言，1代表找到了最优语言，2代表着找到了第二优语言
	int8_t* p = 0;
	uint8_t count = 0;

	memset(lang,0,3*4);
	if(NULL ==  GxBus_ConfigGet(GXBUS_SEARCH_LANG_PRIORITY,(char*)lang_buff,128,GXBUS_SEARCH_LANG_DEFAULT))
	{
		strcpy((char*)lang_buff,GXBUS_SEARCH_LANG_DEFAULT);
	}
	//lang_buff = "eng_chn"等等表示一个或者师多个国家的iso9660代码组合
	//现在支持两种优先语言
	for(p=lang_buff; ;)
	{
		if(*p!='\0'&&count!=2 )
		{
			memcpy(lang[count],p,3);
			count++;
			p+=3;
		}
		if(*p!='\0'&&count!=2 )
		{
			p++;
		}
		else
		{
			break;
		}
	}
	if(search_stream_body_count != 0)
	{
		*audio_pid = search_stream_body[0].audio_pid;
		*type = search_stream_body[0].audio_type;
		*ecm_pid = search_stream_body[0].ecm_pid;
	}
	for(i=0; i<search_stream_body_count; i++)
	{
		if(search_stream_body[i].audio_type != GXBUS_PM_AUDIO_AC3)
		{
			for(j=0; j<sizeof(search_stream_body[i].name); j++)
			{
				search_stream_body[i].name[j] = tolower(search_stream_body[i].name[j]);
			}
			if(-1 == flag)
			{
				*audio_pid = search_stream_body[0].audio_pid;
				*type = search_stream_body[0].audio_type;
				*ecm_pid = search_stream_body[0].ecm_pid;
				flag = 0;
			}

			if(0 == strcmp((const char*)(search_stream_body[i].name),(const char*)(lang[0])))
			{
				*audio_pid = search_stream_body[i].audio_pid;
				*type = search_stream_body[i].audio_type;
				*ecm_pid = search_stream_body[i].ecm_pid;
				flag = 1;
				break;
			}
			else if(0 == strcmp((const char*)(search_stream_body[i].name),(const char*)(lang[1])))
			{
				*audio_pid = search_stream_body[i].audio_pid;
				*type = search_stream_body[i].audio_type;
				*ecm_pid = search_stream_body[i].ecm_pid;
				flag = 2;
			}
		}
	}
	return;
}

static void gx_search_new_prog_reply(GxMsgProperty_NewProgGet new_prog_get)
{
	GxMessage                *new_msg = NULL;
	GxMsgProperty_NewProgGet *params = NULL;

	new_msg = GxBus_MessageNew(GXMSG_SEARCH_NEW_PROG_GET);
	params = (GxMsgProperty_NewProgGet *) GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_NewProgGet);
	memcpy(params, &new_prog_get, sizeof(GxMsgProperty_NewProgGet));

	GxBus_MessageSend(new_msg);

	return;
}

static void gx_search_error_reply(GxMsgStatusReplyCode error)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_StatusReply *params = NULL;
	search_class.search_reply.type = error;

	search_class.search_status |= SEARCH_STOP;
	new_msg = GxBus_MessageNew(GXMSG_SEARCH_STATUS_REPLY);
	params =
		(GxMsgProperty_StatusReply *) GxBus_GetMsgPropertyPtr(new_msg,
				GxMsgProperty_StatusReply);

	memcpy(params, &(search_class.search_reply), sizeof(GxMsgProperty_StatusReply));

	GxBus_MessageSend(new_msg);

	return;
}

/*初始化全局变量 为下一个tp的搜索作准备,*/
void gx_search_variable_next_tp_init(void)
{

	uint8_t                 i = 0;
	uint32_t 	search_status = 0;

	search_status = search_class.search_status & SEARCH_PAT_STOPING;
	if (search_class.search_pat_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_pat_filter_release();


	}

	search_status = search_class.search_status & SEARCH_SDT_STOPING;
	if (search_class.search_sdt_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_sdt_filter_release();

	}

	search_status = search_class.search_status & SEARCH_NIT_STOPING;
	if (search_class.search_nit_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_nit_filter_release();

	}

	if(search_class.search_bat_subtable_id.subtable_id != -1)
	{
		gx_search_bat_filter_release();
	}

	search_status = search_class.search_status & SEARCH_PMT_STOPING;
	for (i = 0; i < FILTER_MAX; i++)
	{
		if (search_class.search_pmt_subtable_id[i].subtable_id != -1
				&&search_status == SEARCH_STATUS_FALSE)
		{
			gx_search_pmt_filter_release(&(search_class.search_pmt_subtable_id[i].subtable_id));
		}

	}
	search_class.search_status |= SEARCH_PMT_STOPING;

	search_class.search_pmt_filter_count = 0;
	search_class.search_pmt_count = 0;
	search_class.search_pmt_finish_count = 0;

	if (search_class.search_pat_body != NULL) {
		GxCore_Free(search_class.search_pat_body);

		search_class.search_pat_body = NULL;
	}
	search_class.search_pat_body_count = 0;

	search_class.search_sdt_body_count = 0;
	search_class.search_pat_finish_flag = 0;
	search_class.search_sdt_finish_flag = 0;
	search_class.search_pmt_all_finish_flag = 0;

	memset(&(search_class.search_pmt_body), 0, sizeof(GxSearchPmtBody));

	gx_search_pmt_cache_list_destroy(&search_class.pmt_stream_cache_list);

	if (search_class.search_stream_body != NULL) {
		GxCore_Free(search_class.search_stream_body);
		search_class.search_stream_body = NULL;
	}
	search_class.search_stream_body_count = 0;
	if (search_class.search_video_stream_body != NULL) {
		GxCore_Free(search_class.search_video_stream_body);
		search_class.search_video_stream_body = NULL;
	}
	search_class.search_ext_finish_num = 0;
	search_class.search_ext_start = 0;
    search_class.search_ext_count = 0;
	//search_transaction_bgin_flag = 0;

	return;
}

/*在退出search的时候清除变量*/
void gx_search_variable_exit_init(void)
{
	uint8_t                 i = 0;
	uint32_t search_status = 0;

	search_class.search_type = GX_SEARCH_AUTO;

	search_class.search_tv_radio = GX_SEARCH_TV;

	search_class.search_fta_cas = GX_SEARCH_FTA;

	search_class.search_nit_switch = GX_SEARCH_NIT_ENABLE;

	memset(&(search_class.search_pid_params), 0, sizeof(GxPidSearch));

	search_class.search_sat_num = 0;

	if (search_class.search_sat_id != NULL) {
		GxCore_Free(search_class.search_sat_id);

		search_class.search_sat_id = NULL;
	}

	search_class.search_tp_num = 0;

	if (search_class.search_tp_id != NULL) {
		GxCore_Free(search_class.search_tp_id);

		search_class.search_tp_id = NULL;
	}

	search_class.search_sat_finish_num = 0;

	search_class.search_tp_finish_num = 0;

	search_class.search_nit_tp_count = 0;

	if (search_class.search_nit_tp_id != NULL) {
		GxCore_Free(search_class.search_nit_tp_id);

		search_class.search_nit_tp_id = NULL;
	}

	search_status = search_class.search_status & SEARCH_NIT_STOPING;
	if (search_class.search_nit_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_nit_filter_release();


	}

	if(search_class.search_bat_subtable_id.subtable_id != -1)
	{
		gx_search_bat_filter_release();
	}

	search_status = search_class.search_status & SEARCH_PAT_STOPING;
	if (search_class.search_pat_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_pat_filter_release();


	}

	search_status = search_class.search_status & SEARCH_SDT_STOPING;
	if (search_class.search_sdt_subtable_id.subtable_id != -1
			&&search_status == SEARCH_STATUS_FALSE)
	{
		gx_search_sdt_filter_release();

	}

	search_status = search_class.search_status & SEARCH_PMT_STOPING;
	for (i = 0; i < FILTER_MAX; i++)
	{
		if (search_class.search_pmt_subtable_id[i].subtable_id != -1
				&&search_status == SEARCH_STATUS_FALSE)
		{
			gx_search_pmt_filter_release(&(search_class.search_pmt_subtable_id[i].subtable_id));
		}

	}
	search_class.search_status |= SEARCH_PMT_STOPING;

	search_class.search_pmt_filter_count = 0;
	search_class.search_pmt_count = 0;
	search_class.search_pmt_finish_count = 0;

	if (search_class.search_pat_body != NULL) {
		GxCore_Free(search_class.search_pat_body);

		search_class.search_pat_body = NULL;
	}
	search_class.search_pat_body_count = 0;
	if (search_class.search_sdt_body != NULL) {
		GxCore_Free(search_class.search_sdt_body);

		search_class.search_sdt_body = NULL;

	}
	search_class.search_sdt_body_count = 0;
	search_class.search_pat_finish_flag = 0;
	search_class.search_sdt_finish_flag = 0;
	search_class.search_pmt_all_finish_flag = 0;

	memset(&(search_class.search_pmt_body), 0, sizeof(GxSearchPmtBody));

	gx_search_pmt_cache_list_destroy(&search_class.pmt_stream_cache_list);

	if (search_class.search_stream_body != NULL) {
		GxCore_Free(search_class.search_stream_body);
		search_class.search_stream_body = NULL;

	}

	gx_search_ext_release();
	if(search_class.search_ext != NULL)
	{
		GxCore_Free(search_class.search_ext);
		search_class.search_ext = NULL;
	}
	search_class.search_ext_num = 0;
	search_class.search_ext_finish_num = 0;
	search_class.search_ext_start = 0;
    search_class.search_ext_count = 0;
	search_class.need_search_ext = 0;
	search_class.ext_finish = NULL;
	search_class.only_filter_ext = false;

	search_class.search_stream_body_count = 0;
	search_class.search_transaction_bgin_flag = 0;

	search_class.search_sat_id_for_prog = 0;
	search_class.search_tp_id_for_prog = 0;

	memset(&(search_class.search_new_prog_get), 0, sizeof(GxMsgProperty_NewProgGet));

	if(blind_class.tp != NULL)
	{
		GxCore_Free(blind_class.tp);
	}
	memset(&blind_class,0,sizeof(GxBlindSearchClass));
	blind_status = GX_SEARCH_OK;
	search_class.search_status |= SEARCH_STOP;

	// multistream
	search_multistream_stop();

	// pdmx
	search_pdmx_stop(search_class.search_tuner_num_for_prog, NULL);

	gx_search_realease_frontend();
	search_cache_destroy();
	return;
}
static status_t gx_search_ext_start(void)
{
    GxMsgProperty_SiCreate *params_create = NULL;
    GxMsgProperty_SiStart  *params_start = NULL;
    GxSubTableDetail        SubTable = { 0 };
    GxMessage              *new_msg = NULL;
    uint16_t                i = 0;

    for(i = search_class.search_ext_count; i<search_class.search_ext_num; i++)
    {
        SubTable.demux_id = 0;
        if(search_class.search_ext[i].time_out != 0)
        {
            SubTable.time_out = search_class.search_ext[i].time_out;
        }
        else
        {
            SubTable.time_out = 5000;
        }
        SubTable.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
        SubTable.table_parse_cfg.table_parse_fun = search_class.search_ext[i].table_parse_fun;
        SubTable.si_filter.pid = search_class.search_ext[i].pid;
        SubTable.si_filter.match_depth = search_class.search_ext[i].match_depth;
        SubTable.si_filter.eq_or_neq = 1;
        memcpy(SubTable.si_filter.match,search_class.search_ext[i].match,18);
        memcpy(SubTable.si_filter.mask,search_class.search_ext[i].mask,18);
        /*发送创建pat的subtable消息给si */
        new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_CREATE);
        params_create = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiCreate);
        *params_create = &SubTable;

        GxBus_MessageSendWait(new_msg);

        if (SubTable.si_subtable_id != -1)	//创建成功
        {
            search_class.search_ext[i].id.subtable_id= SubTable.si_subtable_id;
            search_class.search_ext[i].id.request_id= SubTable.request_id;
        }
        else
        {
            GxBus_MessageFree(new_msg);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
            GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---create ext err!!\n");
#endif
            return GX_SEARCH_ERR;

        }
        // 防止一次无法完全申请出硬件资源，那初始化记录的完成超时时间会不准确，或者确认开始过滤后，才能计时
        search_class.search_ext[i].completion_timeout = GxCore_TickStart(SubTable.time_out);
        search_class.search_ext_count++;

        GxBus_MessageFree(new_msg);

        /*发送开始过滤的消息给si */
        new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_START);
        params_start = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiStart);
        *params_start =search_class.search_ext[i].id.subtable_id;
        GxBus_MessageSend(new_msg);
    }
    search_class.search_ext_start = 1;
    return GX_SEARCH_OK;
}

static status_t gx_search_ext_release_by_id(int subtable_id)
{
    uint32_t i = 0;
    GxMessage              *new_msg = NULL;
    GxMsgProperty_SiRelease *params = NULL;

    if(-1 == subtable_id)
    {
        return GX_SEARCH_ERR;
    }

    /*发送释放的subtable消息给si */
    new_msg = GxBus_MessageNew(GXMSG_SI_SUBTABLE_RELEASE);
    params = GxBus_GetMsgPropertyPtr(new_msg, GxMsgProperty_SiRelease);
    *params = subtable_id;
    GxBus_MessageSendWait(new_msg);
    GxBus_MessageFree(new_msg);        //同步消息 使用完后要free,因为接收的地方是NOT_FREE
    return GX_SEARCH_OK;
}

static status_t gx_search_ext_release(void)
{
    uint32_t i = 0;
    GxMessage              *new_msg = NULL;
    GxMsgProperty_SiRelease *params = NULL;

    for(i=0; i<search_class.search_ext_num; i++)
    {
        if(search_class.search_ext[i].id.subtable_id != -1)
        {
            gx_search_ext_release_by_id(search_class.search_ext[i].id.subtable_id);
            search_class.search_ext[i].id.subtable_id = -1;
        }
    }
    return GX_SEARCH_OK;
}

// use for non-standard stream exit, timeout
static status_t gx_search_ext_section_ok(GxParseResult* parse_result)
{
    uint32_t i = 0;

    for(i=0; i<search_class.search_ext_num; i++)
    {
        if ((parse_result->si_subtable_id ==search_class.search_ext[i].id.subtable_id)
                && (0 != GxCore_TickEnd(search_class.search_ext[i].completion_timeout)))// time out control
        {
            search_class.search_ext_finish_num++;
            gx_search_ext_release_by_id(search_class.search_ext[i].id.subtable_id);
            search_class.search_ext[i].id.subtable_id = -1;
#ifdef GX_BUS_SEARCH_DBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---EXT search completion time out!!\n");
#endif
            break;
        }
    }
    if(search_class.search_ext_num == search_class.search_ext_finish_num)
    {
        search_class.search_ext_start = 0;//所有扩展解析结束了
        gx_search_ext_release();
        return GX_SEARCH_OK;
    }

    return GX_SEARCH_ERR;
}

static status_t gx_search_ext_subtable_ok(GxParseResult* parse_result)
{
    uint32_t i = 0;


    for(i=0; i<search_class.search_ext_num; i++)
    {
        if (parse_result->si_subtable_id ==search_class.search_ext[i].id.subtable_id)
        {
            search_class.search_ext_finish_num++;
            gx_search_ext_release_by_id(search_class.search_ext[i].id.subtable_id);
            search_class.search_ext[i].id.subtable_id = -1;
#ifdef GX_BUS_SEARCH_DBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---EXT search subtable ok!!\n");
#endif
            break;
        }
    }
    if(search_class.search_ext_num == search_class.search_ext_finish_num)
    {
        search_class.search_ext_start = 0;//所有扩展解析结束了
        gx_search_ext_release();
        if(search_class.ext_finish != NULL)
            search_class.ext_finish();

        if(true == search_class.only_filter_ext)
        {
            gx_blind_set_status(GX_SEARCH_OK);
            return GX_SEARCH_CONTINUE;//why return continue, because don't exec gx_search_start;
        }
        return GX_SEARCH_OK;
    }

    return GX_SEARCH_ERR;
}

static status_t gx_search_ext_timeout_check(GxParseResult* parse_result)
{
	uint32_t i = 0;

	for(i=0; i<search_class.search_ext_num; i++)
	{
		if (parse_result->si_subtable_id ==search_class.search_ext[i].id.subtable_id
				&&parse_result->table_id == search_class.search_ext[i].match[0])
		{
			search_class.search_ext_finish_num++;
            gx_search_ext_release_by_id(search_class.search_ext[i].id.subtable_id);
            search_class.search_ext[i].id.subtable_id = -1;
#ifdef GX_BUS_SEARCH_DBUG
            GX_BUS_SEARCH_NORMAL_PRINTF("[SEARCH]---EXT search no data timeout!!\n");
#endif
			break;
		}
	}
	if(search_class.search_ext_num == search_class.search_ext_finish_num)
	{
		search_class.search_ext_start = 0;//所有扩展解析结束了
		gx_search_ext_release();

		if(search_class.ext_finish != NULL)
			search_class.ext_finish();

		if(true == search_class.only_filter_ext)
		{
			gx_blind_set_status(GX_SEARCH_OK);
			return GX_SEARCH_CONTINUE;//why return continue, because don't exec gx_search_start;
		}

		return GX_SEARCH_OK;
	}

	return GX_SEARCH_ERR;
}

static status_t gx_search_section_ok(GxParseResult * parse_result)
{
	switch (parse_result->table_id)
	{
		case PAT_TID:	//只有一个section,所以直接等GXMSG_SI_SUBTABLE_OK

			break;

		case SDT_ACTUAL_TS_TID:
			gx_search_sdt_section_ok(parse_result);
			break;

		case PMT_TID:	//只有一个section,所以直接等GXMSG_SI_SUBTABLE_OK
			break;

		case NIT_ACTUAL_NETWORK_TID:
			gx_search_nit_section_ok(parse_result);
			break;

		default:
			break;
	}
	return GX_SEARCH_OK;

}

static status_t gx_search_subtable_ok(GxParseResult * parse_result)
{
	status_t ret = 0;
	switch (parse_result->table_id)
	{
		case PAT_TID:
			ret = gx_search_pat_subtable_ok(parse_result);
			if(ret == GX_SEARCH_ERR)//pat有可能存在只有头，没有pmt的描述，这时候应该去解下一个tp
			{

				gx_blind_set_status(GX_SEARCH_ERR);
				return GX_SEARCH_ERR;
			}
			else if(ret == GX_SEARCH_CONTINUE)
			{
				gx_blind_set_status(GX_SEARCH_OK);
				return GX_SEARCH_OK;
			}
			break;

		case SDT_ACTUAL_TS_TID:
			ret = gx_search_sdt_subtable_ok(parse_result);
			if(ret == GX_SEARCH_ERR)//这个时候应该进行下一个tp的搜索
			{
				gx_blind_set_status(GX_SEARCH_ERR);
				return GX_SEARCH_ERR;
			}
			else if(ret == GX_SEARCH_CONTINUE)
			{
				gx_blind_set_status(GX_SEARCH_OK);
				return GX_SEARCH_OK;
			}
			else if(GX_SEARCH_DBASE_FULL == ret)
			{
				gx_blind_set_status(GX_SEARCH_FINISH);
				return GX_SEARCH_DBASE_FULL;
			}
			break;

		case PMT_TID:
			gx_search_pmt_subtable_ok(parse_result);
			break;

		case NIT_ACTUAL_NETWORK_TID:
			gx_search_nit_subtable_ok(parse_result);
			break;

		case BAT_TID:
			gx_search_bat_subtable_ok(parse_result);
			break;

		default:
			break;
	}
	return GX_SEARCH_OK;

}
static status_t gx_search_timeout_check(GxParseResult * parse_result)
{
	static uint8_t          pat_timeout = 0;
	uint32_t                i = 0;

	if (parse_result->si_subtable_id == search_class.search_pat_subtable_id.subtable_id
			&&parse_result->request_id == search_class.search_pat_subtable_id.request_id)
	{
		pat_timeout = 1;
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---pat time out!!\n");
#endif
	}
	else if (parse_result->si_subtable_id == search_class.search_sdt_subtable_id.subtable_id
			&&parse_result->request_id == search_class.search_sdt_subtable_id.request_id)
	{
		/*sdt time out不能影响收台*/
		search_class.search_sdt_finish_flag = 1;
		gx_search_sdt_filter_release();

		if(gx_search_prog_add_by_pmt_cache() == GX_SEARCH_DBASE_FULL)
		{
			return GX_SEARCH_DBASE_FULL;
		}

#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---sdt time out!!\n");
#endif
		if(1 == search_class.search_pmt_all_finish_flag)
			return GX_SEARCH_OK;
		else
			return GX_SEARCH_ERR;
	}
	else if(parse_result->si_subtable_id == search_class.search_nit_subtable_id.subtable_id
			&&parse_result->request_id== search_class.search_nit_subtable_id.request_id)
	{
		gx_search_nit_filter_release();
		gx_search_sdt_filter_create();
		gx_search_pat_filter_create();

#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---nit time out!!\n");
#endif
		return GX_SEARCH_ERR;
	}
	else
	{
		for (i = 0; i < FILTER_MAX; i++)
		{
			if (parse_result->si_subtable_id == search_class.search_pmt_subtable_id[i].subtable_id
					&&parse_result->request_id == search_class.search_pmt_subtable_id[i].request_id)
			{
				search_class.search_pmt_finish_count++;
				if (search_class.search_pmt_count < search_class.search_pat_body_count)	//还有pmt没解析
				{
					gx_search_pmt_next_filte_start(parse_result);

				}
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---pmt time out!!\n");
#endif
				break;
			}
		}

		if(search_class.search_pmt_finish_count == search_class.search_pat_body_count)
			search_class.search_pmt_all_finish_flag = 1;
	}

	if (pat_timeout == 1)
	{
		pat_timeout = 0;

		return GX_SEARCH_OK;
	}
	else if (1 == search_class.search_pmt_all_finish_flag
			&& 1 == search_class.search_sdt_finish_flag)
	{
		pat_timeout = 0;
		return GX_SEARCH_OK;
	}

	return GX_SEARCH_ERR;
}

static status_t gx_search_scan_start(GxMessage * Msg)
{
	void* search_params = NULL;
	status_t ret = 0;

	search_class.pmt_parse_fun = NULL;
	switch(Msg->msg_id)
	{
		case GXMSG_SEARCH_BLIND_SCAN_START:
			gx_search_class_init(GX_SEARCH_FRONT_S);

#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---start blind scan!!\n");
#endif

			search_params = GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BlindScanStart);
			break;

		default:
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---start params err!!\n");
#endif
			return GX_SEARCH_ERR;
			break;
	}

	ret = search_class.init_params_func((void*)search_params);
	if(ret != GX_SEARCH_OK)
	{
		gx_search_error_reply(SEARCH_ERROR);
		return GX_SEARCH_ERR;
	}

	return GX_SEARCH_OK;
}

static status_t gx_search_table_filter_start(void)
{

	status_t ret = 0;

	ret = GX_SEARCH_OK;//search_class.tp_lock_func();

	ret = gx_search_start();
	if (ret != GX_SEARCH_OK)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---gx_search_start err!!\n");
#endif
	}
	return ret;
}

static status_t gx_blind_set_22k(uint32_t flag)
{
	int32_t ret = 0;
	if(flag == GXBUS_PM_SAT_22K_ON)
	{
		ret = ioctl(search_class.search_module_frontend, FE_SET_TONE, SEC_TONE_ON);
	}
	else
	{
		ret = ioctl(search_class.search_module_frontend, FE_SET_TONE, SEC_TONE_OFF);
	}
	if(ret == -1)
	{
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend 22K err!!\n");
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}

static status_t gx_blind_set_poarl(uint8_t polar)
{
	int32_t ret = -1;
	switch(polar)
	{
		case BLIND_SCAN_POLAR_H:
			ret = ioctl(search_class.search_module_frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);
			break;

		case BLIND_SCAN_POLAR_V:
			ret = ioctl(search_class.search_module_frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_13);
			break;

		default:
			break;
	}
	if(ret == -1)
	{
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend voltage err!!\n");
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;
}

static uint32_t  gx_blind_single_double_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
	uint32_t progress = 0;
	if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
	{
		if(blind_scan->window_total == 0)
		{
			return 0;
		}
		progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
	}
	else//还没有调用驱动或者调用驱动已经结束
	{
		if(blind_scan->window_num_filt == 0)
		{
			return 0;
		}
		progress = start + 10 + (blind_scan->try_tp_stage *40)/blind_scan->window_num_filt;
	}
	return progress;
}

static uint32_t  gx_blind_single_one_poar_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
	uint32_t progress = 0;;
	if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
	{
		if(blind_scan->window_total == 0)
		{
			return 0;
		}
		progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
	}
	else//还没有调用驱动或者调用驱动已经结束
	{
		if(blind_scan->window_num_filt == 0)
		{
			return 0;
		}
		progress = start + 10 + (blind_scan->try_tp_stage *90)/blind_scan->window_num_filt;
	}
	return progress;
}
static uint32_t  gx_blind_ocs_double_poar_ku_progress(uint32_t start,GxBlindSearchClass* blind_scan)
{
	uint32_t progress = 0;;
	if(blind_scan->start_flag == 1)//已经开始调用驱动了，进度计算使用window
	{
		if(blind_scan->window_total == 0)
		{
			return 0;
		}
		progress = start +(blind_scan->window_num *10)/blind_scan->window_total;
	}
	else//还没有调用驱动或者调用驱动已经结束
	{
		if(blind_scan->window_num_filt == 0)
		{
			return 0;
		}
		progress = start + 10 + (blind_scan->try_tp_stage *15)/blind_scan->window_num_filt;
	}
	return progress;
}

static uint32_t gx_blind_progress_check_stop(void)
{
	return blind_class.blind_exit;
}

static status_t gx_blind_progress(GxBlindSearchClass* blind_scan)
{
	uint32_t progress = 0;
	GxBlindSearchStage blind_stage = BLIND_STAGE_NONE;

	if(gx_blind_progress_check_stop())
	{
		return GX_SEARCH_FINISH;
	}

	switch(blind_scan->polar_stage)
	{
		case BLIND_SINGLE_ONE_POLAR:           //单本振单极化?
		case BLIND_OCS_C_ONE_POLAR:            //双本振c波段单极化
			progress = gx_blind_single_one_poar_progress(0,blind_scan);
			break;
		case BLIND_SINGLE_DOUBLE_POLAR_1:      //单本振双极化第一个极化
		case BLIND_OCS_C_DOUBLE_POLAR_1:       //双本振c波段双极化第一个极化
		case BLIND_OCS_KU_ONE_POLAR_LOW:       //双本振ku波段单极化low
			progress =  gx_blind_single_double_poar_progress(0,blind_scan);
			break;
		case BLIND_SINGLE_DOUBLE_POLAR_2:      //单本振双极化第二个极化
		case BLIND_OCS_C_DOUBLE_POLAR_2:       //双本振c波段双极化第二个极化
		case BLIND_OCS_KU_ONE_POLAR_HIGH:      //双本振ku波段单极化high
			progress =  gx_blind_single_double_poar_progress(50,blind_scan);
			break;
		case BLIND_OCS_KU_DOUBLE_POLAR_LOW1:   //双本振ku波段双极化第一个极化low
			progress = gx_blind_ocs_double_poar_ku_progress(0,blind_scan);
			break;
		case BLIND_OCS_KU_DOUBLE_POLAR_LOW2:   //双本振ku波段双极化第二个极化low
			progress = gx_blind_ocs_double_poar_ku_progress(25,blind_scan);
			break;
		case BLIND_OCS_KU_DOUBLE_POLAR_HIGH1:  //双本振ku波段双极化第一个极化high
			progress = gx_blind_ocs_double_poar_ku_progress(50,blind_scan);
			break;
		case BLIND_OCS_KU_DOUBLE_POLAR_HIGH2:  //双本振ku波段双极化第二个极化high
			progress = gx_blind_ocs_double_poar_ku_progress(75,blind_scan);
			break;

		default:
			return GX_SEARCH_OK;
	}
	(blind_scan->start_flag == 1) ? (blind_stage = BLIND_STAGE_TP) : (blind_stage = BLIND_STAGE_PROG);
	gx_blind_send_progress((uint8_t)progress, blind_stage);
	return GX_SEARCH_OK;
}

static void gx_blind_scan_get_tp_info(GxBlindSearchClass* blind_scan,uint32_t i, struct dvb_frontend_parameters params)
{
	uint32_t if_frequency = 0;
	uint32_t local_frequency = 0;
	int32_t lnbf_tag = 0;
	switch(params.u.dvbs_param.type)
	{
		case FE_DVB_S:
			blind_scan->tp[i].type = DVBS;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

		case FE_ATSC:
			blind_scan->tp[i].type = DIRECTV;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

		case FE_DVB_S2:
			blind_scan->tp[i].type = DVBS2;
			blind_scan->tp[i].qpsk = params.u.dvbs_param.modulation;
			break;

		default:
			blind_scan->tp[i].type = DVBS2;
			blind_scan->tp[i].qpsk = DVBS2_8PSK_23;
			break;
	}
	if_frequency = (params.frequency*1000+500)/1000;

	if(blind_scan->lnb_type == BLIND_SCAN_LNB_C)
	{
		if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)//双本振
		{
			if(blind_scan->tp_polar == GXBUS_PM_TP_POLAR_H)
			{
				local_frequency = blind_scan->local_fre1;
			}
			else
			{
				local_frequency = blind_scan->local_fre2;
			}
		}
		else
		{
			local_frequency = blind_scan->local_fre1;
		}
		if(local_frequency > if_frequency)
		{
			blind_scan->tp[i].fre = local_frequency - if_frequency;
		}
		else
		{
			blind_scan->tp[i].fre = local_frequency + if_frequency;
		}
	}
	else
	{
		GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);//lnbf support #359333
		if((lnbf_tag == GXBUS_FRONTEND_LNBF_ON) &&
				((blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) ||
				(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ))){//Multi-Point LNBF #359333
			if(blind_scan->tp_polar == GXBUS_PM_TP_POLAR_H)
				local_frequency = blind_scan->local_fre1;
			else
				local_frequency = blind_scan->local_fre2;
		}else{
			if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS)
			{
				if(blind_scan->switch_22k == GXBUS_PM_SAT_22K_ON)//22k on
				{
					if(blind_scan->local_fre1 > blind_scan->local_fre2)
					{
						local_frequency = blind_scan->local_fre1;
					}
					else
					{
						local_frequency = blind_scan->local_fre2;
					}
				}
				else
				{
					if(blind_scan->local_fre1 > blind_scan->local_fre2)
					{
						local_frequency = blind_scan->local_fre2;
					}
					else
					{
						local_frequency = blind_scan->local_fre1;
					}
				}
			}
			else
			{
				local_frequency = blind_scan->local_fre1;
			}
		}
		blind_scan->tp[i].fre = local_frequency + if_frequency;
	}
	blind_scan->tp[i].symbol_rate = params.u.dvbs_param.symbol_rate;
	return;
}

static int32_t gx_blind_sort_tp_by_fre(const void *p1, const void *p2)
{
	int32_t fre1 = 0;
	int32_t fre2 = 0;
	fre1 = ((GxBlindSearchTp*)p1)->fre;
	fre2 = ((GxBlindSearchTp*)p2)->fre;
	return fre1 - fre2;
}

static int32_t gx_blind_scan_window(GxBlindSearchClass* blind_scan)
{
	uint32_t fcenter = 0;
	struct fe_set_window_parameters params;
	struct fe_get_window_parameters get_params;
	struct dvb_frontend_parameters* tp_params = NULL;
	uint32_t i = 0;
	int32_t tp_num = 0;

	if((blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) ||
		(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ)){
		if(blind_scan->start_fre < BLIND_SCAN_LOW_FREQ_MHZ)
			blind_scan->start_fre = BLIND_SCAN_LOW_FREQ_LNBF_MHZ;
		if(blind_scan->end_fre > BLIND_SCAN_HIGH_FREQ_MHZ)
			blind_scan->end_fre = BLIND_SCAN_HIGH_FREQ_LNBF_MHZ;
	}else{
		if(blind_scan->start_fre < BLIND_SCAN_LOW_FREQ_MHZ)
			blind_scan->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
		if(blind_scan->end_fre > BLIND_SCAN_HIGH_FREQ_MHZ)
			blind_scan->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
	}

	blind_scan->window_total = (blind_scan->end_fre + BLIND_SCAN_STEP_MHZ - blind_scan->start_fre)/BLIND_SCAN_STEP_MHZ;
	blind_scan->window_num= 0;
	blind_scan->start_flag = 1;
	fcenter = blind_scan->start_fre;
	/*先进行正常符号率的盲扫*/
	do
	{
		i = 0;
		if(gx_blind_get_status() != GX_SEARCH_OK)
		{
			tp_num = -1;
			goto err;
		}
		params.fcenter = fcenter;
		params.lpf_bw_window = BLIND_SCAN_WINDOW_SIZE_K;
		if(ioctl(search_class.search_module_frontend, FE_SET_BLINDSCAN, &params) < 0)
		{
			goto err;
		}
		//gxlogd("~~~~~~~~~%s %d num = %d size = %d~~~~~~\n",__FILE__,__LINE__,params.tp_num,params.tp_num*sizeof(struct dvb_frontend_parameters));
		if(params.tp_num != 0)
		{
			tp_params = GxCore_Malloc(params.tp_num*sizeof(struct dvb_frontend_parameters));
			if(tp_params == NULL)
			{
				tp_num = -1;
				goto err;
			}
			get_params.params = tp_params;
			get_params.tp_num = params.tp_num;
			if(ioctl(search_class.search_module_frontend, FE_GET_BLINDSCAN, &get_params) < 0)
			{
				tp_num = -1;
				goto err;
			}
			for(i=0; i<params.tp_num; i++)
			{
				if(tp_num+i >= BLIND_SCAN_TP_COUNT)
				{
					goto finish;
				}
				blind_scan->tp[i+tp_num].fre = tp_params[i].frequency;
				blind_scan->tp[i+tp_num].symbol_rate = tp_params[i].u.dvbs_param.symbol_rate;
				gxlogd("get blind %d - fre:%d, symbol:%d\n", i+tp_num+1, tp_params[i].frequency, tp_params[i].u.dvbs_param.symbol_rate);
			}
		}
		gx_blind_progress(blind_scan);
		tp_num += i;
		fcenter += BLIND_SCAN_STEP_MHZ;
		blind_scan->window_num += 1;

		if(tp_params != NULL)
		{
			GxCore_Free(tp_params);
			tp_params = NULL;
		}
	}while(fcenter <= blind_scan->end_fre + BLIND_SCAN_STEP_MHZ);
	/*进行低符号率的盲扫*/
	//    fcenter = blind_scan->start_fre;
	/*do
	  {
	  params.fcenter = fcenter;
	  params.lpf_bw_window = BLIND_SCAN_WINDOW_SIZE_K;
	  if(ioctl(search_class.search_module_frontend, FE_SET_BLINDSCAN, &params) < 0)
	  {
	  goto err;
	  }
	  get_params = GxCore_Malloc(params.tp_num*sizeof(struct dvb_frontend_parameters));
	  if(get_params == NULL)
	  {
	  goto err;
	  }
	  if(ioctl(search_class.search_module_frontend, FE_GET_BLINDSCAN, get_params) < 0)
	  {
	  goto err;
	  }
	  for(i=0; i<params.tp_num; i++)
	  {
	  if(tp_num+i > BLIND_SCAN_TP_COUNT)
	  {
	  goto finish;
	  }

	  blind_scan->tp[i+tp_num].fre = get_params[i].frequency;
	  blind_scan->tp[i+tp_num].symbol_rate = get_params[i].symbol_rate;
	  }
	  tp_num += i;
	  fcenter += BLIND_SCAN_STEP_MHZ;
	  blind_scan->window_num += 1;
	  gx_blind_progress(blind_scan);
	  GxCore_Free(get_params);
	  get_params = NULL;
	  }while(fcenter <= blind_scan->end_fre + BLIND_SCAN_STEP_MHZ);*/
finish:
	if(blind_scan->window_num < blind_scan->window_total)
	{
		blind_scan->window_num = blind_scan->window_total;
		gx_blind_progress(blind_scan);
	}
	if(tp_params != NULL)
	{
		GxCore_Free(tp_params);
	}
	return tp_num;
err:
	if(blind_scan->window_num < blind_scan->window_total)
	{
		blind_scan->window_num = blind_scan->window_total;
		gx_blind_progress(blind_scan);
	}
	if(tp_params != NULL)
	{
		GxCore_Free(tp_params);
	}
	return tp_num;
}

static int32_t gx_blind_sort_tp(GxBlindSearchClass* blind_scan)
{
	int32_t tp_num = 0;
	int32_t fre_temp = 0;
	int32_t symbol_temp = 0;
	int32_t lnbf_tag = 0;
	uint32_t fre_low = 0, fre_high = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t sym_index = 4;
	GxBlindSearchTp* tp_temp = NULL;

	tp_num = gx_blind_scan_window(blind_scan);
	if(tp_num <=0)
	{
		return tp_num;
	}
	tp_temp = GxCore_Malloc(sizeof(GxBlindSearchTp)*BLIND_SCAN_TP_COUNT);
	if(tp_temp == NULL)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[blind search]---no memery!!\n");
		return 0;
	}
	//sort by fre
	qsort(blind_scan->tp,tp_num,sizeof(GxBlindSearchTp),gx_blind_sort_tp_by_fre);

	GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);
	if(lnbf_tag == GXBUS_FRONTEND_LNBF_OFF){
		fre_low  = 945000;
		fre_high = 2155000;
	}else{
		fre_low  = 250000;
		fre_high = 2400000;
	}

	//delete the semblable tp
	for(i=0; i<tp_num; i++)
	{
		if(gx_blind_get_status() != GX_SEARCH_OK)
		{
			GxCore_Free(tp_temp);
			return 0;
		}
		if(blind_scan->tp[i].fre < fre_low ||
				blind_scan->tp[i].fre > fre_high ||
				blind_scan->tp[i].symbol_rate < 800||
				blind_scan->tp[i].symbol_rate > 50000)
		{
			continue;
		}
		if(i != (tp_num-1))
		{
			fre_temp = blind_scan->tp[i+1].fre - blind_scan->tp[i].fre;
			symbol_temp = blind_scan->tp[i+1].symbol_rate - blind_scan->tp[i].symbol_rate;
			if(symbol_temp < 0)
			{
				symbol_temp = blind_scan->tp[i].symbol_rate - blind_scan->tp[i+1].symbol_rate;
			}
			// above 40M symbole, need to record, check it is ok or not after "get_tpinfo"
			if((blind_scan->tp[i].symbol_rate <= 40000) && (((fre_temp <= blind_scan->tp[i].symbol_rate/sym_index)&&
							(fre_temp <= blind_scan->tp[i+1].symbol_rate/sym_index)&&
							(symbol_temp < blind_scan->tp[i].symbol_rate/32)&&
							(symbol_temp < blind_scan->tp[i+1].symbol_rate/32))||
						fre_temp <= 500))
			{
				continue;
			}
		}
		//gxlogd("\n---[2][sort tp][fre = %d][sry = %d]-----\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
		memcpy(&tp_temp[j],&(blind_scan->tp[i]),sizeof(GxBlindSearchTp));
		j++;
	}
	memcpy(blind_scan->tp,tp_temp,sizeof(GxBlindSearchTp)*j);
	GxCore_Free(tp_temp);
	return j;
}

static status_t gx_blind_scan_tp(GxBlindSearchClass* blind_scan)
{
	int32_t tp_num;
	int32_t ret = 0;
	int32_t i = 0;
	struct dvb_frontend_parameters params;
	uint32_t retry = 0;
	uint32_t locked = 0;
	//fe_status_t festatus;
	unsigned long           tick = 0;
	GxDemuxProperty_TSLockQuery ts_lock_status = {TS_SYNC_UNLOCKED};
	GxBusPmDataTP tp_record = {0};
	int frequency = 0;
	int32_t pdmx_switch = 0;
	static int s_timeout_num = 0;
	uint8_t do_locked_once = 0;

	GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);

	tp_num = gx_blind_sort_tp(blind_scan);
	if(tp_num == -1)
	{
		ret = GX_SEARCH_FINISH;
		goto blind_no_ok;
	}
	blind_scan->window_num_filt = tp_num;
	blind_scan->try_tp_stage = 0;
	blind_scan->start_flag = 0;
	ret = gx_blind_progress(blind_scan);
	if(ret != GX_SEARCH_OK)
	{
		goto blind_no_ok;
	}

	memset(&tp_record,0,sizeof(GxBusPmDataTP));

	ioctl(search_class.search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 1);
	for(i=0; i<tp_num; i++)
	{
		ret = gx_blind_get_status();
		if(ret != GX_SEARCH_OK)
		{
			goto blind_no_ok;
		}

		memset(&params, 0, sizeof(struct dvb_frontend_parameters));
		params.frequency          = blind_scan->tp[i].fre;
		params.u.dvbs_param.symbol_rate = blind_scan->tp[i].symbol_rate;
		params.u.dvbs_param.fec_inner   = FEC_AUTO;
		blind_scan->tp[i].pls_n = 0;
		blind_scan->tp[i].stream_size = 188;
		search_class.current_tp_streamsize = 188;

lock_timeout:
		gxlogd("set :  fre = %d  symbolrate = %d\n",params.frequency,params.u.dvbs_param.symbol_rate);
		blind_scan->try_tp_stage = i+1;
		if (ioctl(search_class.search_module_frontend, FE_SET_FRONTEND, &params) != 0)
		{
			goto send_progress;
		}

		search_multistream_stop();
		locked = 0;

		if (blind_scan->tp[i].symbol_rate <= 5000)
			retry = 3500;
		else if (blind_scan->tp[i].symbol_rate > 10000)
			retry = 2500;
		else
			retry = 3000;

		if(BLIND_SUPER == search_class.search_blind_type
                && true == search_plsn_set(search_class.search_tuner_num_for_prog, &blind_scan->tp[i].pls_n))
		{
			params.u.dvbs_param.pls_n = blind_scan->tp[i].pls_n;
            retry = 8000;
		}

next_ts:
		// pdmx
		search_pdmx_stop(search_class.search_tuner_num_for_prog, NULL);

		{
			uint32_t query_count = 0;
			do_locked_once = 0;
			fe_status_t tp_lock_status = 0;

			tick = GxCore_TickStart(retry);
			do
			{
				query_count++;
				if(ioctl(search_class.search_module_frontend, FE_READ_STATUS, &tp_lock_status) >= 0)
				{
					if(tp_lock_status == FE_HAS_LOCK)
					{
						if(do_locked_once == 0
								&& BLIND_SUPER == search_class.search_blind_type
								&& GxFrontend_HasSpecialFeatures(search_class.search_tuner_num_for_prog))
						{
							do_locked_once = 1;
							search_multistream_init(search_class.search_tuner_num_for_prog);
							gx_blind_search_multistream_set_ts(i, blind_scan, params.frequency);
							blind_scan->tp[i].stream_size = search_class.current_tp_streamsize;
						}

next_plp:
						if(1 == search_pdmx_get_active_flag())
						{
							unsigned long temp = GxCore_TickStart(1500);
							do
							{
								ret = GxAVGetProperty(search_class.search_device_handle,
										search_class.search_demux_handle,
										GxDemuxPropertyID_TSLockQuery,
										&ts_lock_status,
										sizeof(GxDemuxProperty_TSLockQuery));
								if((ret < 0) || (ts_lock_status.ts_lock == TS_SYNC_LOCKED))
									break;
								GxCore_ThreadDelay(20);
							}while(!GxCore_TickEnd(temp));
						}
						ret = GxAVGetProperty(search_class.search_device_handle,
								search_class.search_demux_handle,
								GxDemuxPropertyID_TSLockQuery,
								&ts_lock_status,
								sizeof(GxDemuxProperty_TSLockQuery));
						if(ret < 0 )
						{
							s_timeout_num = 0;
							gxlogd("\nERROR,~~~~~~~~~~~~%s %d ~~~~~~~~~~\n",__FILE__,__LINE__);
							goto blind_no_ok;
						}
						if(ts_lock_status.ts_lock == TS_SYNC_LOCKED)
						{
							locked = 1;
							break;
						}

					}
					else if (tp_lock_status == FE_HAS_CARRIER)
					{// wrong signal
						s_timeout_num = 0;
						gxlogd("\nERROR,~~~~~~~~~~~~%s %d ~~~~~~~~~~\n",__FILE__,__LINE__);
						goto send_progress;
					}
					else
					{
						int interval = (query_count*1000)/retry;//tick - GxCore_TickStart(0);

						if(tp_lock_status == FE_EQU_LOCK){
							if(interval >= 80) tick-=5*10;
						} else if(tp_lock_status == FE_CRL_LOCK){
							if(interval>=60) tick-=5*10;
						} else if(tp_lock_status == FE_TIM_LOCK){
							if(interval>=50) tick-=10*10;
						} else if(tp_lock_status == FE_AGC_LOCK){
							if(interval>=50) tick-=15*10;
						} else{
							if(interval>=50) tick-=20*10;
						}
					}
				}
				else
				{
					s_timeout_num = 0;
					gxlogd("\nERROR,~~~~~~~~~~~~%s %d ~~~~~~~~~~\n",__FILE__,__LINE__);
					goto blind_no_ok;
				}
				GxCore_ThreadDelay(10);
			}while(!GxCore_TickEnd(tick));
			if((tp_lock_status == FE_HAS_LOCK) && (1 != locked))
			{
				if(BLIND_SUPER == search_class.search_blind_type
						&& GxFrontend_HasSpecialFeatures(search_class.search_tuner_num_for_prog))
				{
					PDMXSignalInfoClass  signal_info = {0};
					{
						GxBusPmDataSat          sat = { 0 };
						GxBus_PmSatGetById(search_class.search_sat_id_for_prog, &sat);

						signal_info.longitude = (sat.sat_s.longitude_direct > 0?(3600-sat.sat_s.longitude):sat.sat_s.longitude);
						_blind_scan_get_tp_fre(blind_scan, i, params.frequency, &signal_info.fre_m);
						signal_info.sym_k = blind_scan->tp[i].symbol_rate;
						signal_info.pol   = blind_scan->tp_polar;
					}

					if(1 == search_pdmx_set_params(search_class.search_tuner_num_for_prog, search_class.search_ts_cur,
								search_class.search_demux_id, &signal_info))
					{
						goto next_plp;
					}

					if(true == search_multistream_increase_ts_index())
					{
						s_timeout_num = 0;
						goto next_ts;
					}
				}
			}
		}
		if(locked == 2)
		{
			s_timeout_num = 0;
			goto send_progress;
		}
		else if(locked == 1)
		{
			struct dvb_frontend_parameters temp_params = {0};
			memcpy(&temp_params, &params, sizeof(struct dvb_frontend_parameters));
			if(ioctl(search_class.search_module_frontend, FE_GET_FRONTEND, &temp_params)>=0)
			{
				s_timeout_num = 0;
				gx_blind_scan_get_tp_info(blind_scan,i,temp_params);
				// filter the same tp, before scan program. Have already sort by frequency,so now only compare with the last one.
				{
					if(blind_scan->tp[i].symbol_rate > 20000)
						frequency = blind_scan->tp[i].symbol_rate/4000;
					else
						frequency = 3;
					if(tp_record.frequency != 0)
					{
						if(((blind_scan->tp[i].fre > (tp_record.frequency - frequency))
									&& (blind_scan->tp[i].fre < (tp_record.frequency + frequency)))
								&& ((blind_scan->tp[i].symbol_rate > (tp_record.tp_s.symbol_rate - 100))
									&& (blind_scan->tp[i].symbol_rate < (tp_record.tp_s.symbol_rate + 100))))
						{
							if(pdmx_switch != GXBUS_FRONTEND_PDMX_OFF
									&& (search_pdmx_get_active_flag())
									&& (BLIND_SUPER == search_class.search_blind_type))
							{
								gxlogd("\n--SUPERBLIND SCAN T2MI, find the same TP[%d:%d], continue it\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
							}
							else if(true == search_multistream_get_active_flag()
									&& (BLIND_SUPER == search_class.search_blind_type))
							{
								gxlogd("\n--SUPERBLIND SCAN MultiStream, find the same TP[%d:%d], continue it\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
							}
							else
							{
								// find the same tp, skip it
								gxlogd("\nBLIND SCAN, find the same TP[%d:%d], skip it\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate);
								goto send_progress;
							}
						}
					}
					tp_record.frequency = blind_scan->tp[i].fre;
					tp_record.tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
					tp_record.tp_s.pls_n = blind_scan->tp[i].pls_n;
					tp_record.tp_s.stream_size = blind_scan->tp[i].stream_size;
				}

				gxlogd("get :  fre = %d, symbolrate = %d, pls_n=%d\n",blind_scan->tp[i].fre,blind_scan->tp[i].symbol_rate, blind_scan->tp[i].pls_n);
				gx_blind_progress(blind_scan);
				ret = gx_search_blind_new_tp_get(i,blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
				if(search_class.search_blind_type != BLIND_ONLY_TP)
				{
					ret = gx_search_table_filter_start();
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
					GxCore_SemWait(search_class.search_blind_sem);
					ret = gx_blind_get_status();
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				gx_search_variable_next_tp_init();

				if(BLIND_SUPER == search_class.search_blind_type)
				{
					PDMXSignalInfoClass  signal_info = {0};
					{
						GxBusPmDataSat          sat = { 0 };
						GxBus_PmSatGetById(search_class.search_sat_id_for_prog, &sat);

						signal_info.longitude = (sat.sat_s.longitude_direct > 0?(3600-sat.sat_s.longitude):sat.sat_s.longitude);
						signal_info.fre_m = blind_scan->tp[i].fre;
						signal_info.sym_k = blind_scan->tp[i].symbol_rate;
						signal_info.pol   = blind_scan->tp_polar;
					}

					if(1 == search_pdmx_set_params(search_class.search_tuner_num_for_prog, search_class.search_ts_cur,
								search_class.search_demux_id, &signal_info))
					{
						goto next_plp;
					}
				}
				if(BLIND_SUPER == search_class.search_blind_type
						&& true == search_multistream_increase_ts_index())
				{
					s_timeout_num = 0;
					goto next_ts;
				}
			}
			else
			{
				s_timeout_num++;
				if((s_timeout_num < 3)&&(params.u.dvbs_param.symbol_rate == 0))
				{
					if(BLIND_SUPER == search_class.search_blind_type)
					{
						GxCore_ThreadDelay(500);
						goto lock_timeout;
					}
				}
			}
		}
send_progress:
		gx_blind_progress(blind_scan);
		continue;
	}

	ioctl(search_class.search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 0);

	return GX_SEARCH_OK;
blind_no_ok:
	ioctl(search_class.search_module_frontend, FE_SET_FRONTEND_TUNE_MODE, 0);

	return ret;
}

static status_t gx_blind_start(uint8_t polar,int32_t sat22k,GxBlindSearchClass* blind_scan)
{
	int32_t ret = 0;
	int32_t sat22k_temp = 0;

	ret = gx_blind_progress(blind_scan);
	if(ret != GX_SEARCH_OK)
	{
		goto blind_no_ok;
	}
	switch(polar)
	{
		case BLIND_SCAN_POLAR_V:
			blind_scan->tp_polar = GXBUS_PM_TP_POLAR_V;
			break;

		case BLIND_SCAN_POLAR_H:
			blind_scan->tp_polar = GXBUS_PM_TP_POLAR_H;
			break;

		default:
			break;
	}
	if(sat22k == GXBUS_PM_SAT_22K_ON)
	{
		sat22k_temp = SEC_TONE_ON;
	}
	else
	{
		sat22k_temp = SEC_TONE_OFF;
	}
	if(ret == -1)
	{
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---set frontend 22K err!!\n");
		return GX_SEARCH_ERR;
	}
	if(search_class.search_diseqc != NULL)
	{
		switch(polar)
		{
			case BLIND_SCAN_POLAR_H:
				search_class.search_diseqc(search_class.search_sat_id_for_prog,SEC_VOLTAGE_18,sat22k_temp);
				break;

			case BLIND_SCAN_POLAR_V:
				search_class.search_diseqc(search_class.search_sat_id_for_prog,SEC_VOLTAGE_13,sat22k_temp);
				break;

			default:
				break;
		}
	}
	else
	{
		ret = gx_blind_set_22k(sat22k);
		if(ret != GX_SEARCH_OK)
		{
			goto blind_no_ok;//
		}
		ret = gx_blind_set_poarl(polar);
		if(ret != GX_SEARCH_OK)
		{
			goto blind_no_ok;
		}
	}
	ret = gx_blind_scan_tp(blind_scan);
	if(ret != GX_SEARCH_OK)
	{
		goto blind_no_ok;
	}

	return GX_SEARCH_OK;

blind_no_ok:
	return ret;
}

static status_t gx_blind_search(GxBlindSearchClass* blind_scan)
{
	status_t ret = 0;
	uint8_t i = 0;
	int32_t lnbf_tag = 0;
	blind_scan->start_flag = 0;

	if(blind_scan->polar == BLIND_SCAN_POLAR_ALL)
	{
		i = 1;
	}
	if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_SINGLE)//单本振
	{
		if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫h
		{
			blind_scan->window_num_filt = 0;
			if(i == 0)
			{
				blind_scan->polar_stage = BLIND_SINGLE_ONE_POLAR;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			else
			{
				blind_scan->polar_stage = BLIND_SINGLE_DOUBLE_POLAR_1;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			ret = gx_blind_start(BLIND_SCAN_POLAR_H,blind_scan->switch_22k, blind_scan);
			if(ret != GX_SEARCH_OK)
			{
				goto blind_no_ok;
			}
			i = 1;
		}
		blind_scan->window_num_filt = 0;
		if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫v
		{
			if(i == 0)
			{
				blind_scan->polar_stage = BLIND_SINGLE_ONE_POLAR;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			else
			{
				blind_scan->polar_stage = BLIND_SINGLE_DOUBLE_POLAR_2;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			ret = gx_blind_start(BLIND_SCAN_POLAR_V,blind_scan->switch_22k, blind_scan);
			if(ret != GX_SEARCH_OK)
			{
				goto blind_no_ok;
			}
		}
	}
	else if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS
			&&blind_scan->lnb_type == BLIND_SCAN_LNB_C)//c 波段双本振
	{
		if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫h
		{
			blind_scan->window_num_filt = 0;
			if(i == 0)
			{
				blind_scan->polar_stage = BLIND_OCS_C_ONE_POLAR;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			else
			{
				blind_scan->polar_stage = BLIND_OCS_C_DOUBLE_POLAR_1;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			blind_scan->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
			blind_scan->end_fre = ((BLIND_SCAN_LOW_FREQ_MHZ + BLIND_SCAN_HIGH_FREQ_MHZ)/2-50);
			ret = gx_blind_start(BLIND_SCAN_POLAR_H,blind_scan->switch_22k, blind_scan);
			if(ret != GX_SEARCH_OK)
			{
				goto blind_no_ok;
			}
		}
		if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫v
		{
			blind_scan->window_num_filt = 0;
			if(i == 0)
			{
				blind_scan->polar_stage = BLIND_OCS_C_ONE_POLAR;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			else
			{
				blind_scan->polar_stage = BLIND_OCS_C_DOUBLE_POLAR_2;
				ret = gx_blind_progress(blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			blind_scan->start_fre = (BLIND_SCAN_LOW_FREQ_MHZ + BLIND_SCAN_HIGH_FREQ_MHZ)/2;
			blind_scan->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
			ret = gx_blind_start(BLIND_SCAN_POLAR_V,blind_scan->switch_22k, blind_scan);
			if(ret != GX_SEARCH_OK)
			{
				goto blind_no_ok;
			}
		}

	}
	else if(blind_scan->lnb_ocs == BLIND_SCAN_LNB_OCS
			&&blind_scan->lnb_type == BLIND_SCAN_LNB_KU)//KU 波段双本振
	{
		GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);
		if((lnbf_tag == GXBUS_FRONTEND_LNBF_ON) &&
			((blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) ||
			(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ))){
			//for Multi-Point LNBF #359333
			if(blind_scan->polar != BLIND_SCAN_POLAR_V){//盲扫h
				blind_scan->window_num_filt = 0;
				if(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ){// 10250/11400
					blind_scan->start_fre = GXBUS_FRONTEND_LNBF_LOW_START_SCAN_FREQ_MHZ;
					blind_scan->end_fre = GXBUS_FRONTEND_LNBF_LOW_END_SCAN_FREQ_MHZ;
				}else{// 11400/10250
					blind_scan->start_fre = GXBUS_FRONTEND_LNBF_HIGH_START_SCAN_FREQ_MHZ;
					blind_scan->end_fre = GXBUS_FRONTEND_LNBF_HIGH_END_SCAN_FREQ_MHZ;
				}
				if(i == 0){
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
						goto blind_no_ok;
				}else{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_LOW1;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
						goto blind_no_ok;
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_H,blind_scan->switch_22k,blind_scan);
				if(ret != GX_SEARCH_OK)
					goto blind_no_ok;
			}
			if(blind_scan->polar != BLIND_SCAN_POLAR_H){//盲扫low v
				blind_scan->window_num_filt = 0;
				if(blind_scan->local_fre1 == GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ){// 10250/11400
					blind_scan->start_fre = GXBUS_FRONTEND_LNBF_HIGH_START_SCAN_FREQ_MHZ;
					blind_scan->end_fre = GXBUS_FRONTEND_LNBF_HIGH_END_SCAN_FREQ_MHZ;
				}else{// 11400/10250
					blind_scan->start_fre = GXBUS_FRONTEND_LNBF_LOW_START_SCAN_FREQ_MHZ;
					blind_scan->end_fre = GXBUS_FRONTEND_LNBF_LOW_END_SCAN_FREQ_MHZ;
				}
				if(i == 0){
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
						goto blind_no_ok;
				}else{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_HIGH1;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
						goto blind_no_ok;
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_V,blind_scan->switch_22k,blind_scan);
				if(ret != GX_SEARCH_OK)
					goto blind_no_ok;
			}
		}else{//normal LNB
			if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫low h
			{
				blind_scan->window_num_filt = 0;
				blind_scan->start_fre = 1100;
				blind_scan->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
				blind_scan->switch_22k = GXBUS_PM_SAT_22K_ON;

				if(i == 0)
				{
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				else
				{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_LOW1;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_H,GXBUS_PM_SAT_22K_ON,blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫low v
			{
				blind_scan->window_num_filt = 0;
				blind_scan->start_fre = 1100;
				blind_scan->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
				blind_scan->switch_22k = GXBUS_PM_SAT_22K_ON;

				if(i == 0)
				{
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_LOW;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				else
				{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_LOW2;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_V,GXBUS_PM_SAT_22K_ON,blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			if(blind_scan->polar != BLIND_SCAN_POLAR_V)//盲扫high h
			{
				blind_scan->window_num_filt = 0;
				blind_scan->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
				blind_scan->end_fre = 1950;
				blind_scan->switch_22k = GXBUS_PM_SAT_22K_OFF;

				if(i == 0)
				{
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_HIGH;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				else
				{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_HIGH1;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_H,GXBUS_PM_SAT_22K_OFF,blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
			if(blind_scan->polar != BLIND_SCAN_POLAR_H)//盲扫high v
			{
				blind_scan->window_num_filt = 0;
				blind_scan->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
				blind_scan->end_fre = 1950;
				blind_scan->switch_22k = GXBUS_PM_SAT_22K_OFF;

				if(i == 0)
				{
					blind_scan->polar_stage = BLIND_OCS_KU_ONE_POLAR_HIGH;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				else
				{
					blind_scan->polar_stage = BLIND_OCS_KU_DOUBLE_POLAR_HIGH2;
					ret = gx_blind_progress(blind_scan);
					if(ret != GX_SEARCH_OK)
					{
						goto blind_no_ok;
					}
				}
				ret = gx_blind_start(BLIND_SCAN_POLAR_V,GXBUS_PM_SAT_22K_OFF,blind_scan);
				if(ret != GX_SEARCH_OK)
				{
					goto blind_no_ok;
				}
			}
		}
	}
	else
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---err blind type!!\n");
		return GX_SEARCH_ERR;
	}
	return GX_SEARCH_OK;

blind_no_ok:
	return ret;
}

static status_t gx_search_blind_start(GxMessage * Msg)
{
	GxMsgProperty_BlindScanStart* params = NULL;
	status_t ret = 0;

	params = (GxMsgProperty_BlindScanStart*)GxBus_GetMsgPropertyPtr(Msg,GxMsgProperty_BlindScanStart);
	blind_class.tp = GxCore_Malloc(sizeof(GxBlindSearchTp)*BLIND_SCAN_TP_COUNT);
	if(blind_class.tp == NULL)
	{
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[blind search]---no memery!!\n");
		return GX_SEARCH_ERR;
	}

	search_class.search_blind_polar = params->polar_type;
	search_class.search_blind_type = params->search_type;
	search_class.search_sat_finish_num = 0;

	ret = gx_search_blind_get_params(&blind_class);
	if(GX_SEARCH_OK != ret)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
		return GX_SEARCH_FINISH;
	}
	GxFrontendClearErrFreFlag();
	GxCore_SemPost(search_class.search_blind_sem);
	return GX_SEARCH_OK;

}

static status_t gx_search_blind_new_tp_get(uint32_t i,GxBlindSearchClass* blind_scan)
{
	int32_t count = 0;
	GxBusPmDataTP tp = {0};
	GxBusPmDataSat sat = {0};
	GxBusPmTpPolar temp_polar = 0;

	GxMessage              *new_msg = NULL;
	GxMsgProperty_BlindScanReply *params = NULL;
	temp_polar = blind_scan->tp_polar;
	count = search_cache_tp_exist_check(search_class.search_sat_id_for_prog,
										blind_scan->tp[i].fre,
										blind_scan->tp[i].symbol_rate,
										temp_polar,
										0,
										(uint16_t*)(&(tp.id)));
	if(0 == count)
	{
		status_t ret;
		tp.frequency = blind_scan->tp[i].fre;
		tp.sat_id = search_class.search_sat_id_for_prog;
		tp.tp_s.polar = temp_polar;
		tp.tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
		tp.tp_s.pls_n = blind_scan->tp[i].pls_n;
		tp.tp_s.stream_size = blind_scan->tp[i].stream_size;
		ret = GxBus_PmTpAdd(&tp);
		if(ret == GX_PM_DBASE_FULL)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]--- tp save full!!\n");
#endif
			gx_search_error_reply(SEARCH_DBASE_OVERFLOW);
			return GX_SEARCH_DBASE_FULL;
		}
		else if(ret == GXCORE_ERROR)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]--- tp save err!!\n");
#endif
			return GX_SEARCH_ERR;
		}
		search_cache_tp_add(tp);
	}
	else
	{
		GxBus_PmTpGetById(tp.id, &tp);
		if((blind_scan->tp[i].stream_size > 0 && tp.tp_s.stream_size != blind_scan->tp[i].stream_size)
				|| tp.tp_s.pls_n != blind_scan->tp[i].pls_n)
		{
			tp.tp_s.stream_size = blind_scan->tp[i].stream_size;
			tp.tp_s.pls_n = blind_scan->tp[i].pls_n;
			gxlogd("\033[32mModify TP data pls_n:%d stream size:%d\n\033[0m", tp.tp_s.pls_n, tp.tp_s.stream_size);
			GxBus_PmTpModify(&tp);
		}
	}
	search_class.search_tp_id_for_prog = tp.id;
	GxBus_PmSatGetById(search_class.search_sat_id_for_prog,&sat);
	new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
	params =
		(GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
				GxMsgProperty_BlindScanReply);

	params->tp_id = tp.id;
	params->sat_id = search_class.search_sat_id_for_prog;
	params->frequency = blind_scan->tp[i].fre;
	params->sat_max_count = search_class.search_sat_num;
	memcpy(params->sat_name,sat.sat_s.sat_name,MAX_SAT_NAME);
	params->sat_name[MAX_SAT_NAME-1] = 0;
	params->sat_num = search_class.search_sat_finish_num;
	params->tp_s.polar = temp_polar;
	params->tp_s.symbol_rate = blind_scan->tp[i].symbol_rate;
	params->type = BLIND_TP;
	params->mode = blind_scan->tp[i].type;
	params->qpsk = blind_scan->tp[i].qpsk;
	GxBus_MessageSend(new_msg);
	return GX_SEARCH_OK;
}

static status_t gx_blind_send_progress(uint8_t progress, GxBlindSearchStage stage)
{
	GxMessage              *new_msg = NULL;
	GxMsgProperty_BlindScanReply *params = NULL;

	new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
	params =(GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
			GxMsgProperty_BlindScanReply);
	params->progress = progress;
	params->type = BLIND_PROGRESS;
	params->stage = stage;
	params->sat_id = search_class.search_sat_id_for_prog;
	params->tp_id = 0;
	GxBus_MessageSend(new_msg);
	return GX_SEARCH_OK;
}
static void gx_search_blind_stop_reply(void)
{
	GxMessage              *new_msg = NULL;
	uint32_t i = 0;

	if (search_class.search_pat_subtable_id.subtable_id == -1
			&&search_class.search_sdt_subtable_id.subtable_id == -1
			&&search_class.search_nit_subtable_id.subtable_id == -1
			&&search_class.search_bat_subtable_id.subtable_id == -1)
	{
		for (i = 0; i < FILTER_MAX; i++)
		{
			if (search_class.search_pmt_subtable_id[i].subtable_id != -1)
			{
				return;
			}
		}

		search_class.search_status |= SEARCH_STOPED;
		search_exit = 1;
		GxBus_MessageEmpty(search_class.search_msg_handle);
		new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_FINISH);
		GxBus_MessageSend(new_msg);

	}
	return;
}


static status_t gx_search_blind_get_params(GxBlindSearchClass* params)
{
	uint32_t sat_id = 0;
	int32_t lnbf_tag = 0;
	GxBusPmDataSat sat = {0};
	status_t ret = 0;
	GxMessage              *new_msg = NULL;
	GxMsgProperty_BlindScanReply *msg_params = NULL;
	if(search_class.search_sat_num != 0
			&&search_class.search_sat_finish_num<search_class.search_sat_num)
	{
		sat_id = search_class.search_sat_id[search_class.search_sat_finish_num];
		if(GXCORE_ERROR == GxBus_PmSatGetById(sat_id,&sat))
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---get sat by id err!!\n");
#endif
			return GX_SEARCH_FINISH;
		}

		/*判断是双本振还是单本振*/
		params->local_fre1 = sat.sat_s.lnb1;
		params->local_fre2 = sat.sat_s.lnb2;
		if((sat.sat_s.lnb1 != sat.sat_s.lnb2)
				&& sat.sat_s.lnb2 != 0)
		{
			params->lnb_ocs =BLIND_SCAN_LNB_OCS;
		}
		else
		{
			params->lnb_ocs =BLIND_SCAN_LNB_SINGLE;
		}
		params->switch_22k = sat.sat_s.switch_22K;
		/*判断盲扫的极化方向*/
		if(sat.sat_s.lnb_power== GXBUS_PM_SAT_LNB_POWER_ON)
		{
			params->polar = BLIND_SCAN_POLAR_ALL;
		}
		else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_18V)
		{
			params->polar = BLIND_SCAN_POLAR_H;
		}
		else if(sat.sat_s.lnb_power == GXBUS_PM_SAT_LNB_POWER_13V)
		{
			params->polar = BLIND_SCAN_POLAR_V;
		}
		/*if(BLIND_POLAR_ALL == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_ALL;
		  }
		  else if(BLIND_POLAR_H == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_H;
		  }
		  else if(BLIND_POLAR_V == search_class.search_blind_polar)
		  {
		  params->polar = BLIND_SCAN_POLAR_H;
		  }*/
		/*设置起始频率和结束频率*/
		GxBus_ConfigGetInt(GXBUS_FRONTEND_LNBF,&lnbf_tag,GXBUS_FRONTEND_LNBF_OFF);//lnbf support #359333
		if(lnbf_tag == GXBUS_FRONTEND_LNBF_OFF){
			params->start_fre = BLIND_SCAN_LOW_FREQ_MHZ;
			params->end_fre = BLIND_SCAN_HIGH_FREQ_MHZ;
		}else{
			params->start_fre = BLIND_SCAN_LOW_FREQ_LNBF_MHZ;
			params->end_fre = BLIND_SCAN_HIGH_FREQ_LNBF_MHZ;
		}
		/*判断是ku波段还是c波段*/
		if(sat.sat_s.lnb1<7000 && sat.sat_s.lnb2<7000)
		{
			params->lnb_type = BLIND_SCAN_LNB_C;
		}
		else
		{
			params->lnb_type = BLIND_SCAN_LNB_KU;
		}
		gx_search_realease_frontend();//一定先释放前端，因为释放用到了search_class里面的东西
		search_class.search_ts_cur = search_class.search_ts[search_class.search_sat_finish_num];
		search_class.search_tuner_num_for_prog = sat.tuner;
		search_class.search_sat_id_for_prog = sat_id;
		ret = gx_search_init_frontend();
		if(GX_SEARCH_OK != ret)
		{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---init frontend err!!\n");
#endif
			return GX_SEARCH_FINISH;
		}
		search_class.search_sat_finish_num++;
		params->window_num  = 0;
		params->window_total = 0;

		params->try_tp_stage = 0;
		params->window_num_filt = 0;

		new_msg = GxBus_MessageNew(GXMSG_SEARCH_BLIND_SCAN_REPLY);
		msg_params =
			(GxMsgProperty_BlindScanReply *) GxBus_GetMsgPropertyPtr(new_msg,
					GxMsgProperty_BlindScanReply);

		msg_params->frequency = 0;
		msg_params->sat_max_count = search_class.search_sat_num;
		memcpy(msg_params->sat_name,sat.sat_s.sat_name,MAX_SAT_NAME);
		msg_params->sat_name[MAX_SAT_NAME-1] = 0;
		msg_params->sat_num = search_class.search_sat_finish_num;
		msg_params->tp_s.polar = 0;
		msg_params->tp_s.symbol_rate = 0;
		msg_params->type = BLIND_TP;
		msg_params->sat_id = search_class.search_sat_id_for_prog;
		msg_params->tp_id = 0;
		GxBus_MessageSend(new_msg);

		return GX_SEARCH_OK;

	}
	else
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
		return GX_SEARCH_FINISH;
	}
}

static void gx_blind_set_status(int32_t status)
{
	if(blind_status != GX_SEARCH_FINISH)
	{
		blind_status = status;
	}
	GxCore_SemPost(search_class.search_blind_sem);
	return;
}
static uint32_t gx_blind_get_status(void)
{
	return blind_status;
}
/* Exported Functions ----------------------------------------------------- */

/**
 * @brief 初始化盲扫服务
 * @param
 * @Return
 */
status_t GxBlindSearchInit(handle_t self,int priority_offset)
{
	handle_t                sch;
	uint32_t i = 0;

	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_START,
			sizeof(GxMsgProperty_BlindScanStart));
	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_STOP,0);
	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_FINISH,0);
	GxBus_MessageRegister(GXMSG_SEARCH_BLIND_SCAN_REPLY,
			sizeof(GxMsgProperty_BlindScanReply));
	GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_START);
	GxBus_MessageListen(self, GXMSG_SEARCH_BLIND_SCAN_STOP);
	GxBus_MessageListen(self, GXMSG_SI_SECTION_OK);
	GxBus_MessageListen(self, GXMSG_SEARCH_SAVE);
	GxBus_MessageListen(self, GXMSG_SEARCH_NOT_SAVE);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);

	memset(&search_class,0,sizeof(GxSearchClass));
	search_class.search_status = SEARCH_STOP;
	search_class.search_module_frontend = GXCORE_INVALID_POINTER;
	search_class.search_device_handle = GXCORE_INVALID_POINTER;
	search_class.search_demux_handle = GXCORE_INVALID_POINTER;
	search_class.search_blind_sem = GXCORE_INVALID_POINTER;
	if(GXCORE_SUCCESS != GxCore_SemCreate(&search_class.search_blind_sem,0))
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---create blind sem err !!\n");
#endif
		return GXCORE_ERROR;
	}
	/*各个subtable_id*/
	search_class.search_pat_subtable_id.subtable_id = -1;	///<pat的subtable id
	search_class.search_sdt_subtable_id.subtable_id = -1;	///<sdt的subtable id
	search_class.search_nit_subtable_id.subtable_id = -1;	///<nit的subtable id
	search_class.search_bat_subtable_id.subtable_id = -1;	///<bat的subtable id
	for(i=0; i<FILTER_MAX; i++)
	{
		search_class.search_pmt_subtable_id[i].subtable_id = -1;

	}
	sch =GxBus_SchedulerCreate("BlindSearchMsgScheduler", GXBUS_SCHED_MSG,1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);
	sch = GxBus_SchedulerCreate("BlindSearchConsoleScheduler", GXBUS_SCHED_CONSOLE, 1024 * 32, GXOS_DEFAULT_PRIORITY+priority_offset);
	GxBus_ServiceLink(self, sch);

	return GX_SEARCH_OK;
}

/**
 * @brief 销毁盲扫服务
 * @param
 * @Return
 */
void GxBlindSearchDestroy(handle_t self)
{

	GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_START);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_BLIND_SCAN_STOP);
	GxBus_MessageUnListen(self, GXMSG_SI_SECTION_OK);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_OK);
	GxBus_MessageUnListen(self, GXMSG_SI_SUBTABLE_TIME_OUT);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_SAVE);
	GxBus_MessageUnListen(self, GXMSG_SEARCH_NOT_SAVE);


	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_START);
	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_STOP);
	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_FINISH);
	GxBus_MessageUnregister(GXMSG_SEARCH_BLIND_SCAN_REPLY);

	GxBus_ServiceUnlink(self);

	if(search_class.search_blind_sem != GXCORE_INVALID_POINTER)
	{
		GxCore_SemDelete(search_class.search_blind_sem);
		search_class.search_blind_sem = GXCORE_INVALID_POINTER;
	}
	return;
}

/**
 * @brief
 * @param
 * @Return
 */
GxMsgStatus GxBlindSearchServiceRecvMsg(handle_t self, GxMessage * Msg)
{
	GxMsgStatus status = GXMSG_OK;
	status_t ret = 0;
	GxParseResult          *parse_result = NULL;
	GxBusPmViewInfo sys = {0};
#ifdef GX_BUS_BLIND_SEARCH_DBUG
	int32_t                 prog_num = 0;
#endif

	switch (Msg->msg_id)
	{
		case GXMSG_SI_SECTION_OK:
			if ((search_class.search_status&SEARCH_STOP) != SEARCH_STATUS_FALSE)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---search have stoped !!\n");
#endif
				return GX_SEARCH_ERR;
			}

			parse_result = GxBus_GetMsgPropertyPtr(Msg, GxParseResult);

            if(1 == search_class.search_ext_start)
            {
                ret =  gx_search_ext_section_ok(parse_result);
                if(ret == GX_SEARCH_OK)
                {
                    ret = gx_search_start();
                    if (ret != GX_SEARCH_OK)
                    {
                        gx_blind_set_status(GX_SEARCH_ERR);
#ifdef GX_BUS_BLIND_SEARCH_DBUG
						GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---gx_search_start err!!\n");
#endif
                    }
                }
                break;
            }
			gx_search_section_ok(parse_result);
			break;

		case GXMSG_SI_SUBTABLE_OK:
			if ((search_class.search_status&SEARCH_STOP) != SEARCH_STATUS_FALSE)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---search have stoped !!\n");
#endif
				return GX_SEARCH_ERR;
			}
			parse_result = GxBus_GetMsgPropertyPtr(Msg, GxParseResult);
			if(search_class.search_ext_start == 1)
			{
				ret =  gx_search_ext_subtable_ok(parse_result);
				if(ret == GX_SEARCH_OK)
				{
					ret = gx_search_start();
					if (ret != GX_SEARCH_OK)
					{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
						GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---gx_search_start err!!\n");
#endif
						gx_blind_set_status(GX_SEARCH_ERR);
					}
				}
				break;
			}
			gx_search_subtable_ok(parse_result);
			status = GXMSG_OK;
			break;

		case GXMSG_SEARCH_SAVE:
			if(search_exit != 1)
			{
				break;
			}
			search_exit = 0;
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---save msg!!\n");
#endif
			/*保存搜索到的节目 */
			//GxBus_PmTransactionCommit();
			//gx_search_variable_exit_init();

			if (search_class.search_prog_id_arry != NULL)
			{
				GxCore_Free(search_class.search_prog_id_arry);
				search_class.search_prog_id_arry = NULL;
				GxBus_PmViewInfoGet(&sys);
				if(VIEW_INFO_DISABLE == sys.status)
				{
					sys.status = VIEW_INFO_ENABLE;//当搜索到节目的时候需要建立节目列表
					GxBus_PmViewInfoModify(&sys);//通过整个函数创建了节目列表
				}
				GxBus_PmSync(GXBUS_PM_SYNC_PROG);//同步里面会重建prog list了
			}
			search_class.search_prog_id_num = 0;

#ifdef GX_BUS_BLIND_SEARCH_DBUG
			BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---get  num = %d!!\n", prog_num);
			prog_num = GxBus_PmProgNumGet();
			BLIND_SEARCH_BASE_PRINTF("[BLIND SEARCH]---search  num = %d!!\n", prog_num);
#endif

			status = GXMSG_OK;
			GxBus_PmDbaseBackupDel();
			break;

		case GXMSG_SEARCH_NOT_SAVE:
			if(search_exit != 1)
			{
				break;
			}
			search_exit = 0;
#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---not save msg!!\n");
#endif
			/*不保存搜到的节目 回滚事务 */
			//GxBus_PmTransactionRollback();
			//gx_search_variable_exit_init();
			if (search_class.search_prog_id_arry != NULL)
			{
				if(search_class.search_prog_id_num != 0)
					GxBus_PmProgDelete(search_class.search_prog_id_arry, search_class.search_prog_id_num);
				GxCore_Free(search_class.search_prog_id_arry);
				search_class.search_prog_id_arry = NULL;
				GxBus_PmSync(GXBUS_PM_SYNC_PROG);//同步里面会重建prog list了
			}
			search_class.search_prog_id_num = 0;
			//gx_search_blind_stop_reply();//此时硬件资源早就被gx_search_variable_next_tp_init释放光了,可以直接发stop ok
			//status = GXMSG_NOFREE;/*Disable by lixb 20091016*/
			GxBus_PmDbaseComebackFromMem();
			break;
		case GXMSG_SI_SUBTABLE_TIME_OUT:
			if ((search_class.search_status & SEARCH_STOP) != SEARCH_STATUS_FALSE)
			{
				break;
			}

#ifdef GX_BUS_BLIND_SEARCH_DBUG
			GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---recive timeout!!\n");
#endif
			parse_result = GxBus_GetMsgPropertyPtr(Msg, GxParseResult);
			if(search_class.search_ext_start == 1)
			{
				ret = gx_search_ext_timeout_check(parse_result);
				if(ret == GX_SEARCH_OK)
				{
					ret = gx_search_start();
					if (ret != GX_SEARCH_OK)
					{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
						GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---gx_search_start err!!\n");
#endif
						gx_blind_set_status(GX_SEARCH_ERR);
					}
				}
			}
			else
			{
				ret = gx_search_timeout_check(parse_result);
				if(ret == GX_SEARCH_OK)
				{
					gx_blind_set_status(GX_SEARCH_OK);
				}
				else if (ret == GX_SEARCH_DBASE_FULL)
				{
					gx_blind_set_status(GX_SEARCH_FINISH);
				}
			}
			break;
		case GXMSG_SEARCH_BLIND_SCAN_START:
			if((search_class.search_status & SEARCH_STOP) == SEARCH_STATUS_FALSE)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---search have runnig!!\n");
#endif
				return GX_SEARCH_ERR;
			}
			search_class.search_msg_handle = self;
			ret = gx_search_scan_start(Msg);
			if(ret != GX_SEARCH_OK)
			{
				gx_search_variable_exit_init();
				gx_search_blind_stop_reply();
				break;
			}
			ret = gx_search_blind_start(Msg);
			if(GX_SEARCH_OK != ret)
			{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
				GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind scan start err!!\n");
#endif
				gx_search_variable_exit_init();
				gx_search_blind_stop_reply();
				break;
			}

			break;

		case GXMSG_SEARCH_BLIND_SCAN_STOP:
			blind_status = GX_SEARCH_FINISH;
			break;

		default:
			break;
	}

	return status;
}

void GxBlindSearchServiceConsole(handle_t self)
{
	status_t ret = 0;
	if(GXCORE_SUCCESS != GxCore_SemTimedWait(search_class.search_blind_sem, BLIND_SEARCH_SEM_WAIT_TIMEOUT_VALUE))
	{
		return;
	}

start_blind:

	ret = gx_blind_get_status();
	if(ret != GX_SEARCH_OK)
	{
		gxlogd("gx_blind_get_status error\n");
		goto blind_no_ok;
	}
	ret = gx_blind_search(&blind_class);
	if(ret != GX_SEARCH_OK)
	{
		gxlogd("gx_blind_search error\n");
		goto blind_no_ok;
	}
	ret = gx_search_blind_get_params(&blind_class);
	if(GX_SEARCH_OK != ret)
	{
		gxlogd("gx_search_blind_get_params error\n");
		goto blind_no_ok;
	}
	GxCore_ThreadYield();
	goto start_blind;

blind_no_ok:
	if(ret == GX_SEARCH_FINISH)
	{
#ifdef GX_BUS_BLIND_SEARCH_DBUG
		GX_BUS_BLIND_SEARCH_NORMAL_PRINTF("[BLIND SEARCH]---blind finish!!\n");
#endif
		gx_search_variable_exit_init();
		gx_search_blind_stop_reply();
		GxBus_PmSync(GXBUS_PM_SYNC_TP);
	}
	else
	{
		GX_BUS_BLIND_SEARCH_ERRO_PRINTF("[BLIND SEARCH]---blind scan err!!\n");
		gx_search_variable_exit_init();
		gx_search_blind_stop_reply();
		GxBus_PmSync(GXBUS_PM_SYNC_TP);
	}
}

GxServiceClass blind_search_service = {
	"blind search service",
	GxBlindSearchInit,
	GxBlindSearchDestroy,
	GxBlindSearchServiceRecvMsg,
	GxBlindSearchServiceConsole,
};
