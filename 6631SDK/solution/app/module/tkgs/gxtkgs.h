
#ifndef __GXTKGS_H__
#define __GXTKGS_H__
#if TKGS_SUPPORT

#include "gxmsg.h"
#include "gxbus.h"
#include "module/frontend/gxfrontend_module.h"
#include "module/pm/gxprogram_manage_berkeley.h"
#include "module/si/si_parser.h"
#include "gxcore.h"

//__BEGIN_DECLS
//#define GX_BUS_TKGS_DBUG
#ifdef GX_BUS_TKGS_DBUG
#define TKGS_DEBUG_PRINTF(...)	app_log_debug( __VA_ARGS__ )
#else
#define TKGS_DEBUG_PRINTF(...)	do{}while(0)
#endif
#define TKGS_ERRO_PRINTF(...)	app_log_error( __VA_ARGS__ )

#define TKGS_CONFLICTING_LCN 0xFFFE
#define TKGS_INVALID_LCN	0XFFFF
#define TKGS_INVALID_LOCATOR 0x8000
#define TKGS_PREFER_NAME_LEN	20
#define TKGS_UPDATE_MESSAGE_LEN	100
#define TKGS_MAX_VISIBLE_LOCATION	5
#define TKGS_MAX_INVISIBLE_LOCATION	20

#define TKGS_LOCATION_NOFOUNE 0
#define TKGS_LOCATION_FOUND 1

#define GX_TKGS_SEARCH_DBASE_FULL  0x7ffffffe
#define GX_TKGS_SEARCH_PROG_EXIST  0x7ffffffd
#define GX_TKGS_SEARCH_CONTINUE 0x7ffffffb
#define GX_TKGS_SEARCH_LOCATION  0x7ffffffa
#define GX_TKGS_SEARCH_LCN_INVALID  0x7ffffffc

typedef void (*GxTkgsSearchDiseqc)(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k);

/*升级类型*/
typedef enum{
	GX_TKGS_UPDATE_ALL=0,
	GX_TKGS_UPDATE_LOCATION_AND_BLOCKING
}GxTkgsUpdateType;

/*升级参数*/
typedef struct{
	int tp_id;//为负数表示检查location列表中所有频点，非负表示在当前在当前已锁定的频点检查
	uint16_t pid;//tp_id为非负时有效
	GxTkgsUpdateType type;
}GxTkgsUpdateParm;

/*升级结束状态*/
typedef enum{
	GX_TKGS_FOUND_NEW_VER=0,
	GX_TKGS_FOUND_SAME_VER,
	GX_TKGS_CHECK_VER_TIMEOUT,

	GX_TKGS_UPDATE_OK,
	GX_TKGS_UPDATE_TIMEOUT,
	GX_TKGS_UPDATE_CANCEL,
	GX_TKGS_UPDATE_ERROR
}GxTkgsUpdateStatus;
/*升级消息*/
typedef struct{
	GxTkgsUpdateStatus status;
	char message[TKGS_UPDATE_MESSAGE_LEN];
}GxTkgsUpdateMsg;


/*location 参数*/
typedef struct {
//	uint32_t tp_id;
	uint32_t sat_id;
	uint32_t	   search_demux_id;
	//GxTkgsSearchDiseqc search_diseqc;

	uint32_t frequency; // 频点的频率单位 MHz
	uint32_t       symbol_rate;   // 符号率
	GxBusPmTpPolar polar;         // 极化方向
	uint16_t pid;	///
} GxTkgsLocationParm;

/*location 列表*/
typedef struct {
	uint32_t visible_num;	///visible location 总数
	GxTkgsLocationParm visible_location[TKGS_MAX_VISIBLE_LOCATION];	///<如果scan_type选择了pid搜索,那么这个成员就指明了所要匹配的pid
	uint32_t invisible_num;	///invisible location 总数
	GxTkgsLocationParm invisible_location[TKGS_MAX_INVISIBLE_LOCATION];	///<如果scan_type选择了pid搜索,那么这个成员就指明了所要匹配的pid
} GxTkgsLocation;

/*操作模式*/
typedef enum {
	GX_TKGS_AUTOMATIC =0,	///该模式下TKGS是有效的，节目数据库完全受TKGS控制
	GX_TKGS_CUSTOMIZABLE = 1,	///该模式下TKGS是有效的，但是它接受用户的节目排序，用户可以修改节目列表
	GX_TKGS_OFF = 2  //该模式下在后台TKGS不控制节目列表， 管理节目列表的任务完全交给了用户，但是STB仍然在后台下载和解析TKGS表
} GxTkgsOperatingMode;

/*prefer 列表中元素定义*/
typedef struct{
	uint16_t 	locator;//唯一ID
	uint16_t	lcn;	//逻辑频道号
}GxTkgsService;
/*prefer名称*/
typedef struct{
	char 			name[TKGS_PREFER_NAME_LEN];
	uint32_t		serviceNum;
	GxTkgsService 	*list;
}GxTkgsPrefer;

/*prefer 列表*/
typedef struct {
	uint32_t listNum;	///list 总数
	GxTkgsPrefer *prefer;	///<如果scan_type选择了pid搜索,那么这个成员就指明了所要匹配的pid
} GxTkgsPreferredList;

/*location check by tp*/
typedef struct{
	uint32_t tp_id;
	uint32_t frequency; // 频点的频率单位 MHz
	uint32_t       symbol_rate;   // 符号率
	GxBusPmTpPolar polar;         // 极化方向
	uint16_t resault;	//输出，检查的结果
	uint16_t pid;
}GxTkgsTpCheckParm;

/*location check by pmt*/
typedef struct{
	uint8_t *pPmtSec;
	uint16_t resault;
	uint16_t pid;
}GxTkgsPmtCheckParm;

/*block check by program id*/
typedef struct{
	uint32_t prog_id;
	uint32_t resault;
}GxTkgsBlockCheckParm;

/////*blocking struct*/
typedef struct
{
    uint32_t frequency;
    uint16_t orbital_position;
    uint16_t west_east_flag:1;
    uint16_t polarization:2;
	uint16_t roll_off:2;
	uint16_t modulation_system:1;
    uint16_t modulation_type:2;
    uint16_t reserved:8;
    uint32_t symbol_rate:28;
    uint32_t FEC_inner:4;
}GxTkgsDelivery;

typedef struct{
	uint16_t			delivery_filter_flag;
	uint8_t				frequency_tolerance;
	uint8_t				symbolrate_tolerance;
	GxTkgsDelivery		delivery;
}GxTkgsBlockingDeliveryFilter;

/*name filter match_type define*/
typedef enum{
	GX_TKGS_EXACT_MATCH=0,
	GX_TKGS_MATCH_AT_START,
	GX_TKGS_MATCH_OCCUR_ANYWHERE,
	GX_TKGS_RESERVED
}GxTkgsMatchType;

typedef struct{
	uint8_t				match_type:2;
	uint8_t				case_sensitivity:1;
	char				name[MAX_PROG_NAME];
}GxTkgsBlockingNameFilter;


typedef struct{
	uint16_t				filter_flag;
	GxTkgsBlockingDeliveryFilter delivery_filter;//valid when delivery_filter_flag is 1
	uint16_t				service_id; //valid when service_id_filter_flag is 1
	uint16_t				network_id; //valid wnen network_id_filter_flag is 1
	uint16_t				transport_stream_id; //valid when transport_stream_id_filter_flag is 1
	uint16_t				video_pid; //valid when video_pid_filter_flag is 1
	uint16_t				audio_pid; //valid when audio_pid_filter_flag is 1
	GxTkgsBlockingNameFilter	name_filter;//valid when name_filter_flag is 1
}GxTkgsBlockingFilter;

#define GX_TKGX_MAX_BLOCKING 50
typedef struct{
	uint32_t block_num;
	GxTkgsBlockingFilter filter[GX_TKGX_MAX_BLOCKING];
}GxTkgsBlocking;


/*搜台使用的前端类型 包括 s t c */
typedef enum
{
	GX_SEARCH_FRONT_S = 0,
	GX_SEARCH_FRONT_T ,
	GX_SEARCH_FRONT_C ,
	GX_SEARCH_FRONT_DTMB,
	GX_SEARCH_FRONT_DVBT2,
}GxSearchFrontType;

/*记录 si服务分配的subtable 信息*/
typedef struct {
	int32_t                subtable_id;
	uint32_t               request_id;
} GxSearchIdBody;

/*sdt表的内容*/
typedef struct {
	uint8_t                 desc_valid;
	uint8_t                 service_type;
	uint8_t                 service_name[MAX_PROG_NAME];
} GxSearchSdtServiceDesc;

typedef struct {
	uint8_t                 desc_valid;
	uint16_t                ca_system_id;
} GxSearchSdtCaIdentifierDesc;

typedef struct {
	uint8_t                 desc_valid;
	uint32_t                iso639_code;
	uint8_t                 service_name[MAX_PROG_NAME];
} GxSearchSdtMultServiceNameDesc;

#define MAX_TKGS_CATEGORY	8
typedef struct{
	uint8_t	age_limit;
	uint8_t category_num;
	uint16_t category_id[MAX_TKGS_CATEGORY];
}GxSearchSdtExtendedServiceDesc;

#define SI_MAX_MULTI_NAME_COUNT	(2)
typedef struct {
	uint16_t                service_id;
	uint16_t                ts_id;	//用来和service id唯一的标明一个service 需要保存到节目信息中
	uint8_t                 flag_sch:1;
	uint8_t                 flag_pf:1;
	uint8_t                 running_status:3;	//nvod里面用到
	uint8_t                 flag_free_ca_mode:1;	//0 表示没有加绕,但是prog是否加密的判断一般使用pmt body
	uint8_t                 reserved:2;
	GxSearchSdtServiceDesc  service_desc;
	GxSearchSdtCaIdentifierDesc ca_identifier_desc;
	uint8_t                 multi_service_name_count;
	uint32_t   orig_network_id;

	GxSearchSdtMultServiceNameDesc   multi_service_name_desc[SI_MAX_MULTI_NAME_COUNT];
	GxSearchSdtExtendedServiceDesc	extended_service_desc;
	uint32_t 				sdt_version;
} GxSearchSdtBody;


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
	uint16_t                video_pid;
	uint16_t                ecm_pid_video;
	uint16_t                audio_count;
	uint16_t                pcr_pid;
	uint16_t                service_id;
	uint16_t				tkgs_pid;
	GxBusPmDataProgVideoType service_type;	//avs or mpeg

	GxSearchPmtTeletextDesc pmt_ttx_desc;
	GxSearchPmtSubtitlingDesc pmt_subt_desc;
	GxSearchPmtServiceMoveDesc pmt_service_move_desc;

	GxSearchPmtCaDesc       pmt_ca_desc;
	GxSearchPmtAc3Desc      pmt_ac3_desc;
} GxSearchPmtBody;

typedef struct {
	uint16_t	tv_num;
	uint16_t	radio_num;
	uint16_t	*tv_lcn_list;
	uint16_t	*radio_lcn_list;
}GxTkgsDbLcnList;

typedef status_t (*gx_search_tp_lock)(void);
typedef status_t (*gx_search_init_params)(void*);

/*搜索的实例,包含了一次搜索需要的参数和函数指针*/
typedef struct
{
	GxTkgsUpdateType search_type;//搜索类型
	uint32_t search_status;	//搜索状态
	GxSearchFrontType front_type;	//前端类型，目前只有GX_SEARCH_FRONT_S

	uint32_t        search_sat_id;	///<所要搜索的卫星的id
	uint32_t         search_tp_num;	///<所要搜索的tp的数量
	GxTkgsLocationParm        *location_tp;	///<所要搜索的tp的id
	uint32_t         search_tp_finish_num ;	///<已经搜索完成的tp数量,当search_tp_finish_num == search_tp_num时搜索结束
	uint32_t         search_sat_id_for_prog;	///<节目所属的sat id 用于保存节目 当更换卫星的时候进行更新
	uint32_t         search_tp_id_for_prog;	///<节目所属的tp id 用于保存节目 当更换tp的时候进行更新
	uint32_t         search_tuner_num_for_prog;	///<节目所属的tuner用于保存节目 当更换sat的时候进行更新
    uint32_t         search_ts_cur;///<当前搜索的ts号,在换卫星的时候进行改变

	uint16_t 		*pid;	//tkgs数据的PID, 每个location 一个
	uint32_t        tkgs_time_out;
	GxSearchIdBody  tkgs_subtable_id;///<nit的subtable id
	uint8_t 		last_section_number;
	char			tkgs_version;
	uint8_t			*received_flag;
	uint8_t			*tkgs_data;
	char			*tkgs_message;
	GxTkgsPrefer	cur_prefer_list;

	/*sdt*/
	GxSearchSdtBody *search_sdt_body;
	uint32_t         search_sdt_body_count;

	/*pmt*/
	uint16_t		 tkgs_service_locator;	//当前正在解析的PMT对应的ID
	GxSearchPmtBody  search_pmt_body;
	uint32_t         search_pmt_count;	///<正在解析的pmt的数量,最后应该等于search_pat_body_count
	uint32_t         search_pmt_finish_count;	///<解析好的pmt的数量,最后应该等于search_pat_body_count

    /*stream 现在仅仅用于存储audio信息*/
	uint32_t         search_stream_body_count;
	GxBusPmDataStream *search_stream_body ;

	/*前端设备的句柄*/
	handle_t         search_device_handle;
	handle_t       	 search_demux_handle;
	handle_t 		 search_msg_handle;///<服务的句柄
    uint32_t         search_demux_id;///<获取的demux id

	/*记录下搜索到的节目 用于接受到save是保存*/
	uint32_t* 		  search_prog_id_arry;
	uint32_t         search_prog_id_num ;

	/*记录当前数据库中已经存在的LCN等，便于添加新的时做对比*/
	GxTkgsDbLcnList         lcnList;

    /*pmt的扩展解析*/
    private_table_parser pmt_parse_fun;

	gx_search_tp_lock	tp_lock_func;//锁频函数
	gx_search_init_params init_params_func;//初始化各个参数函数

/*diseqc操作回调*/
	 GxSearchDiseqc search_diseqc;
}GxTkgsClass;

void gx_tkgs_set_disqec_callback(uint32_t sat_id,fe_sec_voltage_t polar,int32_t sat22k);

/*location fun*/
status_t GxBus_TkgsLocationGet(GxTkgsLocation* location);

status_t  GxBus_TkgsLocationSet(GxTkgsLocation* location);
status_t  GxBus_TkgsLocationCheckByTp(GxTkgsTpCheckParm *parm);
status_t  GxBus_TkgsLocationCheckByPmt(GxTkgsPmtCheckParm *parm);

status_t gx_TkgsGetLocationInOrder(uint32_t *num, GxTkgsLocationParm *locationTp, uint16_t *pid);
status_t gx_TkgsSaveLocation(GxTkgsLocationParm *locationTp);

/*blocking fun*/
status_t GxBus_TkgsBlockingSave(GxTkgsBlocking *pBlocking);
status_t GxBus_TkgsBlockingGet(GxTkgsBlocking *pBlocking);
status_t GxBus_TkgsBlockingCheck(GxTkgsBlockCheckParm *param);


/*preferred list fun*/
status_t GxBus_TkgsPreferredListSave(GxTkgsPreferredList *list);
status_t GxBus_TkgsPreferredListGet(GxTkgsPreferredList *list);
status_t GxBus_TkgsUserPreferredListNameSet(char *listName);
status_t GxBus_TkgsUserPreferredListNameGet(char *listName);

/*tkgs data parse*/
status_t gx_tkgs_data_parse(GxTkgsClass *tkgs_class);

//__END_DECLS
#endif
#endif				/* __SEARCH_SERVICE_H__ */
/*@}*/

