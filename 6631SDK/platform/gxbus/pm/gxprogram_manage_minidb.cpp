/************************************************************
 * Copyright (C), 2007-2009, GX S&T Co., Ltd.
 * FileName   : gxprogram_manage_berkeley.c
 * Author     : zhangling
 * Version    : 1.0
 * Date       :
 * Description:
 * Version    :
 * History    :
 * Date                Author      Modification
 * 2007.03.14     zhangling         create
 ***********************************************************/

/* Includes --------------------------------------------------------------- */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/stat.h>
#include "gxcore.h"
#include "gxfrontend.h"
#include "module/pm/gxpm_manage.h"
#include "module/frontend/gxfrontend_module.h"
#include "minidb.hpp"
#include "module/pm/gxpm_urlcontrol.h"
//#include "minifs.h"

using namespace GoXceed;
/* Exported Macros -------------------------------------------------------- */

/* Exported Types --------------------------------------------------------- */

/* Exported Constants ----------------------------------------------------- */

/* Exported Variables ----------------------------------------------------- */
uint32_t MAX_SAT_COUNT = 100;	///<方案最多能容纳的sat数量
uint32_t MAX_TP_COUNT =4000;	///<方案最多能容纳的tp数量
uint32_t MAX_PROG_COUNT = 5000;	///<方案最多能容纳的prog数量
uint8_t* DEFAULT_PATH = NULL;

typedef struct
{
	uint32_t sat_id;
	uint32_t tp_pos;
}gx_pm_tp_pos_by_sat;

typedef struct
{
	uint32_t tp_id;
	uint32_t tp_pos;
}gx_pm_tp_pos_by_id;

/* Private Macros --------------------------------_------------------------ */
#if 0
#ifdef __DEBUG
#define GX_BUS_PM_DBUG
#endif
#endif

#define PM_PRINTF(...) gxlogd( __VA_ARGS__ )
#define SYS_INFO_PRINTF(...) gxlogd( __VA_ARGS__)

#define GX_BUS_PM_PRINTF(msg)\
	do{\
		PM_PRINTF("\n\n*****pm error*****\n");\
		PM_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
		PM_PRINTF("%s\n",msg);\
		PM_PRINTF("~~~~~pm error~~~~~\n");\
	}while(0)

#define GX_BUS_PM_VIEW_INFO_PRINTF(msg)\
	do{\
		SYS_INFO_PRINTF("\n\n*****view info *****\n");\
		SYS_INFO_PRINTF("%s:%s:%d\n", __FILE__, __FUNCTION__, __LINE__ );\
		SYS_INFO_PRINTF("%s\n",msg);\
		SYS_INFO_PRINTF("~~~~~ view  info~~~~~\n");\
	}while(0)


#define GX_PM_DBASE_TYPE DB_BTREE

#define GX_PM_DBASE_ID_USE (1)
#define GX_PM_DBASE_ID_NOT_USE (0)

/*每次malloc给gx_pm_prog_id_arry的数量*/
#define GX_PM_DBASE_MALLOC_PER_TIMR (100)

#define GX_PM_FS_OPEN(path,mode)               (GxCore_Open(path, mode))
#define GX_PM_FS_EXIST(path)                   (GxCore_FileExists(path))
#define GX_PM_FS_READ(file,buf,size,nmemb)     (GxCore_Read(file,buf,size,nmemb))
#define GX_PM_FS_WRITE(file,buf,size,nmemb)    (GxCore_Write(file,buf,size,nmemb))
#define GX_PM_FS_CLOSE(file)                   (GxCore_Close(file))
#define GX_PM_FS_RM(path)                      (GxCore_FileDelete(path))
#define GX_PM_FS_INFO(file,info)               (GxCore_GetFileStat(file,info))
#define GX_PM_FS_SEEK(file,offset,whence)      (GxCore_Seek(file,offset,whence))
#define GX_PM_FS_SYNC(file)                    (GxCore_Sync(file))
#define GX_PM_WRITEAT(file,buf,size,nmemb,pos) (GxCore_WriteAt(file, buf,size,nmemb,pos))
#define GX_PM_READAT(file,buf,size,nmemb,pos)  (GxCore_ReadAt(file,buf,size,nmemb,pos))
#define FILE_MODE  "c"

//uint32_t gx_pm_mutxt_locked = 0;

#define GX_PM_LOCK(mutex)\
	do{\
		GxCore_MutexLock(mutex);\
	}while(0)
#define GX_PM_UNLOCK(mutex)\
	do{\
		GxCore_MutexUnlock(mutex);\
	}while(0)

/* Private Types ---------------------------------------------------------- */
/*
   struct GxBusPmProgSort
   {
   uint32_t id;
   uint8_t name[MAX_PROG_NAME];//通过这个参数排序
   };*/

struct gx_pm_prog_pos_get {
	uint32_t i;		//数组的下标
	uint32_t *pos_id_arry;
};

enum gx_pm_verify_flag
{
	GX_PM_VERIFY_ERR = 0,
	GX_PM_VERIFY_OK
};

/*默认数据库的格式，由高雄的工具决定的*/
typedef struct
{
	/*20B*/
	uint8_t m_chSatName[16];
	uint32_t m_nTunerSelect:2;
	uint32_t reserverd: 30;

	/*4B*/
	uint32_t m_nLongitude:11;//0-1800
	uint32_t m_nLnbType:2;
	uint32_t m_Lnb1:4;
	uint32_t m_Lnb2:4;
	uint32_t m_nDiseqc12Pos:8;//1-255
	uint32_t m_nSat22k:2;
	uint32_t m_nLongitudeDirection:1;

	/*4B*/
	uint32_t m_nDiseqc10:3;
	uint32_t m_nSat12v:1;
	uint32_t m_nTpStartPos:13;
	uint32_t m_nTpNum:13;
	uint32_t m_nDiseqcMotor:2;//为了4B
}DefaultSat_t;


typedef struct
{
	uint32_t m_nFrequency:14;
	uint32_t m_nPolar:1;
	uint32_t m_nSymbol:16;
	uint32_t m_nTp22k:1;
} DefaultTp_t;

typedef struct
{
	/*4B*/
	uint32_t m_nSatId:13;
	uint32_t m_nTpId:13;
	uint32_t m_nReserved:4;
	uint32_t m_nAudioFormat:2;

	uint32_t m_nFavType;
	/*4B*/
	uint32_t m_nServiceId:16;
	uint32_t m_nCasId:13;
	uint32_t m_nFlagAv:1;
	uint32_t m_nFlagScramble:1;
	uint32_t m_nLock:1;

	/*4B*/
	uint32_t m_nVideoPid:13;
	uint32_t m_nAudioPid:13;
	uint32_t m_nVideoType:3;
	uint32_t m_nAudioType:3;

	/*4B*/
	uint32_t m_nPmtPid:13;
	uint32_t m_nPcrPid:13;
	uint32_t m_nHide:1;
	uint32_t m_nSkip:1;
	uint32_t m_nTunerSelect:2;
	uint32_t m_nReserved2:2;

#define MAX_MCHPROG_NAME_LEN 21
	uint8_t m_chProgName[MAX_MCHPROG_NAME_LEN];
} DefaultProg_t;

const uint16_t  DefaultSatLnb[]=
{
	5150,
	5700,
	5750,
	9750,
	10000,
	10600,
	10700,
	10750,
	11250,
	11300,
	10250,//for Multi-Point LNBF low  freq #359333
	11400,//for Multi-Point LNBF high freq #359333
};

struct gx_pm_file_backup
{
	uint8_t* sat_file;
	uint32_t sat_file_size;
	uint8_t* tp_file;
	uint32_t tp_file_size;
	uint8_t* prog_file;
	uint32_t prog_file_size;
};
/* Private Constants ------------------------------------------------------ */

/* Private Variables ------------------------------------------------------ */

GxBusPmDataCasInfot DefaultCaSystem[MAX_CA_SYSTEM] = {
	{CAS_ID_FREE   , CAS_ID_FREE   , "Free"}        ,
	{0x0500        , 0x05ff        , "Viaccess"}    ,
	{0x0100        , 0x01ff        , "SECA"}        ,
	{0x0600        , 0x06ff        , "IRDETO"}      ,
	{0x1800        , 0x18ff        , "Nagravision"} ,
	{0x001         , 0x001         , "Alphacrypt"}  ,
	{0x1700        , 0x17ff        , "Betacrypt"}   ,
	{0x0b00        , 0x0bff        , "Conax"}       ,
	{0x0d00        , 0x0dff        , "Cryptoworks"} ,
	{0x0900        , 0x09ff        , "NDS"}         ,
	{0x0e00        , 0x0eff        , "PowerVU"}     ,
	{0x001         , 0x001         , "Skycrypt"}    ,
	{0x001         , 0x001         , "Firecrpt"}    ,
	{0x4a00        , 0x4aff        , "Xcrypt"}      ,
	{0x2600        , 0x26ff        , "BISS"}        ,
	{CAS_ID_UNKNOW , CAS_ID_UNKNOW , "Unknow"}
};

uint8_t fav_name[9][MAX_FAV_GROUP_NAME] =
{ "Movies", "Music", "Cartoons", "News", "Sports", "Education", "Adult", "Leisure", "Fav"
};

uint32_t* gx_pm_sat_id            = NULL;// 用于开机的时候初始化,表明正在使用的id,如gx_pm_sat_id[0]     = 0;代表id = 1没有使用
uint32_t gx_pm_max_sat_id         = 0;   // 用于开机的时候找到最大的id号 操作的时候优先使用该id+1,
                                         // 作为新的数据的id,当gx_pm_max_sat_id    = MAX_SAT_COUNT时.需要从gx_pm_sat_id中搜索没有使用的id
int32_t gx_pm_sat_count           = 0;   // 记录当前卫星的数量 因为berkeley获取数量需要遍历 为了提高效率 记录一个数量

uint32_t* gx_pm_tp_id             = NULL;
uint32_t gx_pm_max_tp_id          = 0;
int32_t gx_pm_tp_count            = 0;
int32_t gx_pm_tp_count_by_sat     = 0;
uint16_t* gx_pm_tp_id_arry_by_sat = NULL;

uint32_t* gx_pm_prog_id           = NULL;
uint32_t gx_pm_max_prog_id        = 0;
int32_t gx_pm_prog_count          = 0;
int32_t gx_pm_hd_prog_count       = 0;
uint32_t gx_pm_max_stream_id      = 0;

void* gx_pm_prog_id_arry          = NULL;
uint32_t gx_pm_prog_id_arry_num   = 0; // 当前模式下的节目数量

/*以下两个参数每次在get prog num的时候更新*/
GxBusPmViewInfoGroupMode gx_pm_prog_group_mode = GROUP_MODE_ALL;
GxBusPmViewInfoTaxisMode gx_pm_prog_taxis_mode = TAXIS_MODE_NON;


IDB *dbp_sat      = NULL;
IDB *dbp_tp       = NULL;
IDB *dbp_prog     = NULL;
IDB *dbp_fav      = NULL;
IDB *dbp_stream   = NULL;
IDB *dbp_prog_ext = NULL;

// 用于保护一系列连贯操作的大块PM动作
static handle_t gx_pm_option_lock = 0;

/*互斥量句柄*/
static handle_t gx_pm_mutex_handle = 0;
/*pm模块初始化标记 用于inti 和realse的保护 pm当前并不支持多个实例*/
static uint32_t gx_pm_init_flag = 0;

/*校验文件的句柄 该文件用于数据库的完整性校验*/
static handle_t gx_pm_verify_file = 0;

static uint32_t gx_pm_prog_pos_data = 0;
/*观看模式的文件句柄*/
static handle_t gx_pm_view_info_file = 0;
static GxBusPmViewInfo* gx_pm_view_info_mem = NULL;

/*互斥量句柄*/
//static handle_t gx_pm_view_info_mutex_handle = 0;

/*备份数据库的各个文件，用于做恢复用*/
struct gx_pm_file_backup gx_pm_file_backup_mem = {0};

/**/
static GxBus_PmProgCheckCallback gx_pm_prog_check_callback = NULL ;
/* Debug Defined ---------------------------------------------------------- */

/* Private Functions ------------------------------------------------------ */
static status_t gx_pm_lowercase_to_majuscule(uint8_t*arg,uint32_t num);

static status_t gx_pm_dbase_table_check(void);

static status_t gx_pm_dbase_open(uint8_t* dbasename, IDB**dbp);

static status_t gx_pm_dbase_backup(handle_t file,uint8_t** ptr,uint32_t* size);

static status_t gx_pm_dbase_comeback(handle_t file,uint8_t** ptr,uint32_t* size);

status_t gx_pm_verify_write(enum gx_pm_verify_flag flag);

static enum gx_pm_verify_flag  gx_pm_verify_get(void);

static status_t gx_pm_load_default_data(void);

static status_t gx_pm_load_default_entry(void);

static status_t gx_pm_load_default_from_db_file(void);

static status_t gx_pm_sync(GxBusPmSyncType type);

static status_t gx_pm_sat_dbase_init(IDB *dbp);

static status_t gx_pm_sat_delete(uint32_t * id_arry, uint32_t num);

static status_t gx_pm_sat_get_by_id(uint32_t id, GxBusPmDataSat * sat);

static status_t gx_pm_sat_add(GxBusPmDataSat * sat) ;

static status_t gx_pm_tp_dbase_init(IDB *dbp);

static status_t gx_pm_tp_delete_by_sat(uint32_t * id_arry, uint32_t num);

static status_t gx_pm_tp_delete(uint32_t * id_arry, uint32_t num) ;

static status_t gx_pm_tp_get_by_id(uint32_t id, GxBusPmDataTP * tp) ;

static int32_t gx_pm_tp_num_get_by_sat(uint16_t sat_id);

static int32_t gx_pm_tp_cmp_pos(const void *p1, const void *p2);

static status_t gx_pm_tp_add(GxBusPmDataTP * tp) ;

static status_t gx_pm_prog_dbase_init(IDB *dbp);

static int32_t gx_pm_prog_num_get(void);

static int32_t gx_pm_prog_num_get_normal(GxBusPmViewInfo* sys);

static int32_t gx_pm_prog_num_get_user_list(uint8_t* file_path);

//static int32_t gx_pm_prog_num_get_pip(GxBusPmViewInfo* sys);

static status_t gx_pm_prog_delete_by_sat(uint32_t * id_arry, uint32_t num);

static status_t gx_pm_prog_delete_by_tp(uint32_t * id_arry, uint32_t num) ;

static uint8_t gx_pm_prog_check_group_mod(GxBusPmDataProg*prog,GxBusPmViewInfo* sys);

static uint8_t gx_pm_prog_get_taxis_mod(GxBusPmViewInfo* sys);

static uint8_t gx_pm_prog_check_taxis_mod(GxBusPmDataProg*prog,GxBusPmViewInfo* sys);

static int32_t gx_pm_prog_cmp_pos(const void *p1, const void *p2);

static int32_t gx_pm_prog_cmp_pos_reverse(const void *p1, const void *p2);

static int32_t gx_pm_prog_move_cmp_pos(const void *p1, const void *p2);

static int32_t gx_pm_prog_cmp_name(const void *p1, const void *p2);

static int32_t gx_pm_prog_cmp_name_reverse(const void *p1, const void *p2);

static status_t gx_pm_prog_max_min_pos_get(uint32_t * id_arry,
		uint32_t num,
		uint32_t target_id,
		uint32_t * max,
		uint32_t * min,
		IDB *dbp) ;

static int32_t gx_pm_prog_modify_count_get(uint32_t max_pos, uint32_t min_pos,IDB *dbp) ;

status_t gx_pm_prog_pos_init(uint32_t * pos_arry,
		uint32_t max_pos,
		uint32_t min_pos,
		uint32_t target_id,
		uint32_t * target_id_pos,
		IDB *dbp) ;

static status_t gx_pm_prog_before_after_buff_init(uint32_t * before_target_id,
		uint32_t * after_target_id,
		uint32_t * before_count,
		uint32_t * after_count,
		uint32_t max_pos,
		uint32_t min_pos,
		uint32_t target_id_pos,
		uint32_t * id_arry,
		uint32_t num,IDB*dbp) ;

static uint8_t gx_pm_prog_id_exist_check(uint32_t source_id, uint32_t * id_arry, uint32_t num);

status_t gx_pm_prog_pos_update(uint32_t * real_id_arry, uint32_t * real_pos_arry, uint32_t num,IDB*dbp) ;

static status_t	gx_pm_prog_url_fre_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,uint32_t* fre);

static status_t	gx_pm_prog_url_polar_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_voltage_t* polar);

static status_t gx_pm_prog_url_22k_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_tone_mode_t* sat22k);

static int32_t gx_pm_prog_get_by_pos(uint32_t pos, uint32_t num, GxBusPmDataProg * prog_arry);

static status_t gx_pm_prog_original_pos_get(uint32_t*pos_arry);

static status_t gx_pm_prog_id_get_by_pos(uint32_t*pos_arry,uint32_t*id_arry);

static status_t gx_pm_prog_add(GxBusPmDataProg* prog);

static status_t gx_pm_prog_ext_del(uint32_t id);

static status_t gx_pm_prog_info_modify(GxBusPmDataProg * prog);

static status_t gx_pm_prog_ext_modify(uint32_t id,uint32_t size,void* p);

static status_t gx_pm_prog_ext_get(uint32_t prog_id,uint32_t size,void* p);

static status_t gx_pm_fav_group_init(IDB *dbp);

//static status_t gx_pm_stream_dbase_init(IDB *dbp);
//static int32_t gx_pm_stream_get(uint32_t prog_id, uint32_t num, GxBusPmDataStream * stream_arry) ;
//static int32_t gx_pm_stream_num_get(uint32_t prog_id) ;

static status_t gx_pm_view_info_get(GxBusPmViewInfo* sys);

static status_t gx_pm_close_db(void);

static status_t gx_pm_open_db(void);

static status_t gx_pm_copy_default_file(uint8_t* src, uint8_t* target);

static int32_t gx_pm_create_default_sat(handle_t file,gx_pm_tp_pos_by_sat* list);//写入卫星数据

static int32_t gx_pm_create_default_tp(handle_t file, uint32_t start_pos, gx_pm_tp_pos_by_sat* sat_pos_list, gx_pm_tp_pos_by_id* tp_pos_list);//写入卫星数据

static int32_t gx_pm_create_default_prog(handle_t file, gx_pm_tp_pos_by_sat* sat_pos_list, gx_pm_tp_pos_by_id* tp_pos_list);//写入prog

static uint32_t gx_pm_sat_num_get_realy(void);

static	int32_t  gx_pm_tp_num_get_realy(void);

static int32_t gx_pm_default_tp_cmp_pos(const void *p1, const void *p2);

//通过view过滤节目的时候，再加一层回调函数判断，如果不设置，和之前的保持一致. 返回0，是成功
static int32_t gx_pm_prog_check_user_callback(GxBusPmDataProg* prog,GxBusPmViewInfo* sys,int mode)
{
	if ( gx_pm_prog_check_callback != NULL)
	{
		if( gx_pm_prog_check_callback(prog,sys,mode) == 0)
		{
			return 0 ;
		}
		else
		{
			return -1 ;
		}
	}
	return 0 ;
}

int32_t GxBus_PmSetPmProgCheckCallback(GxBus_PmProgCheckCallback callback)
{
	gx_pm_prog_check_callback = callback ;
	return 0 ;
}

/**
 * @brief 把一个buffer里的所有小写字母转为大写字母
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static status_t gx_pm_lowercase_to_majuscule(uint8_t*arg,uint32_t num)
{
	uint8_t* p = NULL;
	uint32_t i = 0;

	p = arg;
	for(i = 0 ; i<num; i++)
	{
		if(*p>='a' && *p<='z')
		{
			*p -=32;
		}
		p++;
	}
	return GXCORE_SUCCESS;
}

/**
 * @brief 检测各个数据库存不存在,如果不存在应该建立,并且从默认数据库进行数据的恢复
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_dbase_table_check(void)
{
	//DBT data, key;
	//	uint32_t berkeley_ret = 0;
	status_t ret = 0;

	/*检测数据库是否存在sat表 如果不存在 创建 */

	ret = gx_pm_sat_dbase_init(dbp_sat);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init sat table err !!\n");
#endif
		return GXCORE_ERROR;	//创建fav table失败
	}

	/*检测数据库是否存在tp表 如果不存在 创建 */
	ret = gx_pm_tp_dbase_init(dbp_tp);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init tp table err !!\n");
#endif
		return GXCORE_ERROR;	//创建fav table失败
	}

	/*检测数据库是否存在prog表 如果不存在 创建 */
	ret = gx_pm_prog_dbase_init(dbp_prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init prog table err !!\n");
#endif
		return GXCORE_ERROR;	//创建fav table失败
	}
	gx_pm_prog_num_get();//重建节目列表

	/*检测数据库是否存在stream表 如果不存在 创建 */
	//gx_pm_stream_dbase_init(dbp_stream);

	/*检测数据库是否存在fav表 如果不存在 创建 */

	ret = gx_pm_fav_group_init(dbp_fav);
	if (ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init fav table err !!\n");
#endif
		return GXCORE_ERROR;	//创建fav table失败
	}

	if(gx_pm_sat_count == 0 && (gx_pm_tp_count !=0 || gx_pm_prog_count != 0))//这种情况不应该发生
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init table err sat = 0 but tp or prog not 0 !!\n");
#endif
		return GXCORE_ERROR;
	}
	if(gx_pm_tp_count == 0 && gx_pm_prog_count != 0)//这种情况不应该发生
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---init table err tp = 0 but prog not 0 !!\n");
#endif
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

/**
 * @brief 打开数据库文件.
 * @param
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_dbase_open(uint8_t* dbase_name, IDB **dbp)
{
	*dbp = IDB::New();
	if(*dbp == NULL)
	{

		//#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open  dbase err !!\n");
		//#endif
		return GXCORE_ERROR;
	}
	if ((*dbp)->Open((const char*)dbase_name) != true)
	{
		//#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open  dbase err !!\n");
		//#endif
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief 备份文件
 * @param
 *
 * @return  GXCORE_SUCCESS:执行正常
 *          GXCORE_ERROR:执行失败
 */
static status_t gx_pm_dbase_backup(handle_t file,uint8_t** ptr,uint32_t* size)
{
	uint32_t read_size = 0;
	GxFileInfo info;
	if(*ptr != NULL)
	{
		GxCore_Free(*ptr);
		*ptr = NULL;
	}
	GX_PM_FS_INFO(file,&info);
	*size = info.size_by_bytes;
	*ptr = (uint8_t*)GxCore_Malloc(*size);
	if(*ptr == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no mem for backup file\n");
#endif
		return GXCORE_ERROR;
	}
	read_size = GX_PM_FS_READ(file,*ptr,1,*size);
	if(read_size != *size)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---backup file read err size != read_size !!\n");
#endif
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

/**
 * @brief 恢复文件
 * @param
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_dbase_comeback(handle_t file,uint8_t** ptr,uint32_t* size)
{
	uint32_t read_size = 0;
	if(*ptr == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---comeback file err!!\n");
#endif
		return GXCORE_ERROR;
	}
	read_size = GX_PM_FS_WRITE(file,*ptr,1,*size);
	if(read_size != *size)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---backup file write size != read_size !!\n");
#endif
		return GXCORE_ERROR;
	}
	GxCore_Free(*ptr);
	*ptr = NULL;
	*size = 0;
	return GXCORE_SUCCESS;
}

/**
 * @brief 该函数用于些操作数据库正确性的检验,写数据前先调用gx_pm_verify_write(GX_PM_VERIFY_ERR)
 * 操作完成后调用gx_pm_verify_write(GX_PM_VERIFY_OK),这样就可以用verify文件
 * 校验数据库在上一次写入后是否正常了,主要应用于开机时候的数据库检验,如果发生
 * 不正常关机,导致数据库损坏,那么必须要恢复默认数据库
 * @param   enum gx_pm_verify_flag flag:校验标记
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
status_t gx_pm_verify_write(enum gx_pm_verify_flag flag)
{
	static int old_flag = -1;
	int32_t buf = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;
	ret =  GX_PM_FS_EXIST  (GX_PM_VERIFY_FILE_PATH);

	if (old_flag == flag)
		return GXCORE_SUCCESS;
	old_flag = flag;

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
		if(gx_pm_verify_file<0)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]--create verify file err!\n");
#endif
			return GXCORE_ERROR;
		}
		GX_PM_WRITEAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));//第一次创建文件，修改记录清零
	}
	else
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH,FILE_MODE);
	}
	if(gx_pm_verify_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--create verify file err!\n");
#endif
		return GXCORE_ERROR;
	}
	if(flag == GX_PM_VERIFY_OK)
	{
		/*每进行一次成功的更新，记录号加一*/
		GX_PM_READAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));
		buf +=1;
		GX_PM_WRITEAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));
	}
	GX_PM_WRITEAT(gx_pm_verify_file,&flag,1,sizeof(enum gx_pm_verify_flag),0);
	//	GX_PM_FS_WRITE(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
	GX_PM_FS_SYNC(gx_pm_verify_file);
	GX_PM_FS_CLOSE(gx_pm_verify_file);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取数据库的校验值
 * @param
 * @return   GX_PM_VERIFY_ERR: 数据库上一次写入操作失败
 *           GX_PM_VERIFY_OK: 校验成功
 */
static enum gx_pm_verify_flag  gx_pm_verify_get(void)
{
	enum gx_pm_verify_flag flag = GX_PM_VERIFY_ERR;
	int32_t buf = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;
	ret =  GX_PM_FS_EXIST  (GX_PM_VERIFY_FILE_PATH);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
		GX_PM_FS_WRITE(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
		GX_PM_WRITEAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));//第一次创建文件，修改记录清零
	}
	else
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
	}
	if(gx_pm_verify_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--open verify file err!\n");
#endif
		return GX_PM_VERIFY_ERR;
	}
	GX_PM_READAT(gx_pm_verify_file,&flag,1,sizeof(enum gx_pm_verify_flag),0);
	//GX_PM_FS_READ(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
	GX_PM_FS_CLOSE(gx_pm_verify_file);
	return flag;
}

static status_t gx_pm_load_default_data(void)
{
	uint32_t buf1 = 0;
	uint32_t pos = 0;
	uint32_t tp_pos = 0;
	uint32_t tp_pos_start = 0;
	uint32_t prog_pos = 0;
	uint32_t sat_num = 0;
	uint32_t tp_num = 0;
	uint32_t prog_num = 0;
	handle_t default_info_file = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	DefaultSat_t default_Sat;
	GxBusPmDataSat sat ={0};
	DefaultTp_t default_tp ;
	GxBusPmDataTP tp = {0};
	DefaultProg_t default_prog ;
	GxBusPmDataProg prog ={0} ;
	//GxBusPmViewInfo sys = {0};
	//	status_t ret = 0;

	if(DEFAULT_PATH == NULL)
	{
		if (GxCore_FileExists((const char *)GX_PM_DEFAULT_PATH) == GXCORE_FILE_UNEXIST) // 没有预置节目
			return GXCORE_SUCCESS;
		default_info_file = GxCore_Open(GX_PM_DEFAULT_PATH, "r");
	}
	else
	{
		if (GxCore_FileExists((const char *)DEFAULT_PATH) == GXCORE_FILE_UNEXIST) // 没有预置节目
			return GXCORE_SUCCESS;
		default_info_file = GxCore_Open((const char*)DEFAULT_PATH, "r");
	}
	if(default_info_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--no defaule db file!\n");
#endif
		return GXCORE_ERROR;
	}
	/*读取其实标记 应该为0x4b386753*/
	GxCore_ReadAt(default_info_file, (void*)&buf1, 4, 1,pos);
	if(buf1 != 0x4b386753)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--defaule db file have broken!\n");
#endif
		GxCore_Close(default_info_file);
		return GXCORE_ERROR;
	}
	/*读取卫星数量*/
	pos = pos+4;
	GxCore_ReadAt(default_info_file, (void*)&sat_num, 4, 1, pos);

	/*读取tp数量*/
	pos = pos+4;
	GxCore_ReadAt(default_info_file, (void*)&tp_num, 4, 1, pos);

	/*读取节目数量*/
	pos = pos+4;
	GxCore_ReadAt(default_info_file, (void*)&prog_num, 4, 1, pos);
	if(sat_num > MAX_SAT_COUNT
			||tp_num > MAX_TP_COUNT
			||prog_num > MAX_PROG_COUNT)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--defaule db file err!\n");
#endif
		GxCore_Close(default_info_file);
		return GXCORE_ERROR;
	}
	/*读取卫星数据*/
	pos = pos+4;
	tp_pos_start = tp_pos = pos+sizeof(DefaultSat_t)*sat_num;
	prog_pos = pos+sizeof(DefaultSat_t)*sat_num+sizeof(DefaultTp_t)*tp_num;
	for(i=0; i<sat_num; i++)
	{
		GxCore_ReadAt(default_info_file,
				(void*)&default_Sat,
				sizeof(DefaultSat_t),
				1,
				pos);
		sat.type = GXBUS_PM_SAT_S;
		sat.tuner = default_Sat.m_nTunerSelect;
		sat.sat_s.lnb1 = DefaultSatLnb[default_Sat.m_Lnb1];
		sat.sat_s.lnb2 = DefaultSatLnb[default_Sat.m_Lnb2];
		sat.sat_s.diseqc10 = (GxBusPmSatDiseqc10)default_Sat.m_nDiseqc10;
		sat.sat_s.diseqc11 = 0;
		sat.sat_s.diseqc12_pos = default_Sat.m_nDiseqc12Pos;
		if (default_Sat.m_nDiseqcMotor == 0)
		{
			sat.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
		}
		else if (default_Sat.m_nDiseqcMotor == 1)
		{
			sat.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_2;
		}
		else if (default_Sat.m_nDiseqcMotor == 2)
		{
			sat.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_USALS;
		}
		sat.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
		sat.sat_s.longitude = default_Sat.m_nLongitude;
		sat.sat_s.longitude_direct = (GxBusPmDataSatLongitudeDirect)default_Sat.m_nLongitudeDirection;
		memcpy(sat.sat_s.sat_name,default_Sat.m_chSatName,MAX_SAT_NAME);
		sat.sat_s.sat_name[MAX_SAT_NAME-1] = 0;
		sat.sat_s.switch_12V = (GxBusPmSat12vSwitch)default_Sat.m_nSat12v;
		sat.sat_s.switch_22K = (GxBusPmSat22kSwitch)default_Sat.m_nSat22k;
		sat.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
		memset(sat.sat_s.unicable_para.centre_fre, 0, MAX_IF_INDEX_NUM * sizeof(uint16_t));
		sat.sat_s.unicable_para.if_channel = 0;
		sat.sat_s.unicable_para.lnb_fre_index = 0;
		sat.sat_s.unicable_para.sat_pos = 0;

		gx_pm_sat_add(&sat);
		/*读取tp数据*/
		tp_pos = tp_pos_start+sizeof(DefaultTp_t)*default_Sat.m_nTpStartPos;
		for(j=0; j<default_Sat.m_nTpNum; j++)
		{
			GxCore_ReadAt(default_info_file,
					(void*)&default_tp,
					sizeof(DefaultTp_t),
					1,
					tp_pos);

			tp.frequency = default_tp.m_nFrequency;
			tp.sat_id = sat.id;
			tp.tp_s.polar = (GxBusPmTpPolar)default_tp.m_nPolar;
			tp.tp_s.symbol_rate = default_tp.m_nSymbol;
			tp.tp_s.pls_n = 0;
            tp.tp_s.stream_size = 188;
			tp_pos = tp_pos+sizeof(DefaultTp_t);
			gx_pm_tp_add(&tp);
		}
		pos = pos+sizeof(DefaultSat_t);
	}
	gx_pm_sync(GXBUS_PM_SYNC_SAT);
	gx_pm_sync(GXBUS_PM_SYNC_TP);

	for(i = 0; i<prog_num; i++)
	{
		GxCore_ReadAt(default_info_file,
				(void*)&default_prog,
				sizeof(DefaultProg_t),
				1,
				prog_pos);

		if(default_prog.m_nAudioType == GXBUS_PM_AUDIO_AC3)
		{
			prog.ac3_flag = GXBUS_PM_PROG_BOOL_ENABLE;
			prog.ac3_pid = default_prog.m_nAudioPid;
		}
		else
		{
			prog.ac3_flag = GXBUS_PM_PROG_BOOL_DISABLE;
			prog.ac3_pid = 0x1fff;
		}
		prog.audio_count    = 1;
		prog.audio_level    = GXBUS_PM_PROG_AUDIO_LECEL_MID;
		prog.audio_mode     = (GxBusPmDataProgAduioTrack)default_prog.m_nAudioFormat;
		prog.audio_volume   = 50;
		prog.bouquet_id     = 0x1fff;
		prog.cas_id         = default_prog.m_nCasId;
		prog.cc_flag        = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.cur_audio_pid  = default_prog.m_nAudioPid;
		prog.cur_audio_type = (GxBusPmDataProgAudioType)default_prog.m_nAudioType;
		prog.ecm_pid_v      = 0x1fff;
		prog.favorite_flag  = default_prog.m_nFavType;
		prog.lock_flag      = (GxBusPmDataProgSwitch)default_prog.m_nLock;//GXBUS_PM_PROG_BOOL_DISABLE;
		prog.original_id    = 0x1fff;
		prog.pcr_pid        = default_prog.m_nPcrPid;
		prog.pmt_pid        = default_prog.m_nPmtPid;
		prog.pmt_version    = 0;
		memcpy(prog.prog_name,default_prog.m_chProgName,
				MAX_PROG_NAME < MAX_MCHPROG_NAME_LEN ? MAX_PROG_NAME : MAX_MCHPROG_NAME_LEN);
		prog.prog_name[MAX_PROG_NAME-1] = 0;

		prog.sat_id        = default_prog.m_nSatId+1;//默认数据的位置从0开始的加1代表id。刚刚好的
		prog.scramble_flag = (GxBusPmDataProgSwitch)default_prog.m_nFlagScramble;
		prog.service_id    = default_prog.m_nServiceId;
		prog.service_type  = (GxBusPmDataProgType)default_prog.m_nFlagAv;
		prog.skip_flag     = (GxBusPmDataProgSwitch)default_prog.m_nSkip;
		prog.subt_flag     = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.tp_id         = default_prog.m_nTpId+1;//默认数据的位置从0开始的加1代表id。刚刚好的
		prog.ts_id         = 0x1fff;
		prog.data_plp_id   = 0xff;
		prog.common_plp_exist = 0xff;
		prog.common_plp_id = 0xff;
		prog.common_status = 0xff;
		prog.muti_ts_id    = 0xff;
		prog.muti_ts_exist = 0x0;
		prog.muti_ts_code  = 0xff;
		prog.pdmx_flag     = 0x0;
		prog.pdmx_pid      = 0x1fff;
		prog.ttx_flag      = GXBUS_PM_PROG_BOOL_DISABLE;
		prog.tuner         = default_prog.m_nTunerSelect;
		prog.video_pid     = default_prog.m_nVideoPid;
		prog.video_type    = (GxBusPmDataProgVideoType)default_prog.m_nVideoType;//GXBUS_PM_PROG_MEPG;
		gx_pm_prog_add(&prog);
		prog_pos = prog_pos+sizeof(DefaultProg_t);
	}
	GxCore_Close(default_info_file);
	if(prog_num != 0)
	{
#if 0
		ret = GxBus_PmViewInfoGet(&sys);
		if(ret != GXCORE_SUCCESS)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]--GxBus_PmViewInfoGet err!\n");
#endif
			return GXCORE_ERROR;
		}
		sys.radio_prog_cur = 1;
		sys.tv_prog_cur = 1;
		sys.status = VIEW_INFO_ENABLE;
		GxBus_PmViewInfoModify(&sys);
#endif
		gx_pm_sync(GXBUS_PM_SYNC_PROG);
	}
	return GXCORE_SUCCESS;
}

static status_t gx_pm_load_default_from_db_file(void)
{
	gx_pm_close_db();//关闭打开的数据库文件

	gx_pm_copy_default_file((uint8_t*)GX_PM_DEFAULT_SAT_FILE_PATH,(uint8_t*)GX_PM_SAT_FILE_PATH);
	gx_pm_copy_default_file((uint8_t*)GX_PM_DEFAULT_TP_FILE_PATH,(uint8_t*)GX_PM_TP_FILE_PATH);
	gx_pm_copy_default_file((uint8_t*)GX_PM_DEFAULT_PROG_FILE_PATH,(uint8_t*)GX_PM_PROG_FILE_PATH);

	return gx_pm_open_db();//打开数据库文件
}

static status_t gx_pm_load_default_entry(void)
{
	if(GxCore_FileExists(GX_PM_DEFAULT_SAT_FILE_PATH) == GXCORE_FILE_EXIST &&
			GxCore_FileExists(GX_PM_DEFAULT_TP_FILE_PATH) == GXCORE_FILE_EXIST &&
			GxCore_FileExists(GX_PM_DEFAULT_PROG_FILE_PATH) == GXCORE_FILE_EXIST)
	{
		return gx_pm_load_default_from_db_file();
	}
	else
	{
		return gx_pm_load_default_data();
	}
	return GXCORE_SUCCESS;
}

static status_t gx_pm_sync(GxBusPmSyncType type)
{
	IDB *dbp = NULL;

	switch(type)
	{
	case GXBUS_PM_SYNC_SAT:
		dbp = dbp_sat;
		break;

	case GXBUS_PM_SYNC_TP:
		dbp = dbp_tp;
		break;

	case GXBUS_PM_SYNC_PROG:
		dbp = dbp_prog;
		break;

	case GXBUS_PM_SYNC_FAV:
		dbp = dbp_fav;
		break;
	default:
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---sync type err !!\n");
#endif
		return GXCORE_ERROR;
		break;
	}
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	dbp->Sync();
	return GXCORE_SUCCESS;
}

/**
 * @brief 卫星数据库的初始化,
 * @param
 *
 * @return  GXCORE_SUCCESS:执行正常
 *          GXCORE_ERROR:执行失败
 */
static status_t gx_pm_sat_dbase_init(IDB *dbp)
{
	uint32_t id = 0;
	ICursor *cursor = dbp->CreateCursor();
	if(gx_pm_sat_count != 0 )
	{
		delete cursor;
		return GXCORE_SUCCESS;//已经恢复默认数据了
	}
	while(!cursor->Eof())
	{
		if(gx_pm_sat_count == (int32_t)MAX_SAT_COUNT)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is overflow!!\n");
#endif
			delete cursor;
			return GXCORE_SUCCESS;
		}
		id = cursor->Key();
		if(id > MAX_SAT_COUNT || id == 0)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is err!!\n");
#endif
			delete cursor;
			return GXCORE_ERROR;
		}
		gx_pm_sat_id[id-1] = GX_PM_DBASE_ID_USE;
		if(id>gx_pm_max_sat_id)
		{
			gx_pm_max_sat_id = id;
		}
		gx_pm_sat_count++;
		cursor->Next();
	};
	delete cursor;
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除卫星数据
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_sat_delete(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t i = 0,j=0,count_bak=0,count=0;
	uint32_t input_id = 0;
	uint16_t new_pos=0;
	ICursor *cursor = NULL;
	uint32_t size = 0;
	GxBusPmDataSat *p = NULL;

	if (id_arry == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*删除sat */
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);//操作数据库前先把校验文件写为GX_PM_VERIFY_ERR
	GxBusPmDataSat *list = (GxBusPmDataSat*)GxCore_Malloc(gx_pm_sat_count*sizeof(GxBusPmDataSat));
	if(list == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	memset(list,0,gx_pm_sat_count*sizeof(GxBusPmDataSat));

	count = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		if(count == (uint32_t)gx_pm_sat_count)//找到了所有信息
		{
			break;
		}
		p = (GxBusPmDataSat *)cursor->GetRaw(&size);
		if (p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get tp data err !!\n");
#endif
			delete cursor;
			GxCore_Free(list);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		memcpy(&list[count],p,sizeof(GxBusPmDataSat));
		count++;
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;

	count_bak=(uint32_t)gx_pm_sat_count;
	for(i=0; i<num; i++)
	{
		input_id = id_arry[i];
		if(dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del sat err !!\n");
#endif
			dbp->Sync();
			GxCore_Free(list);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_sat_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_sat_count--;
		for(j=0;j<count_bak;j++)
		{
			if(list[j].id == input_id)
			{
				list[j].id =0;
				break;
			}
		}
	}
	if(gx_pm_sat_count !=0)
	{
		for(j=0;j<count_bak;j++)
		{
			if(list[j].id !=0)
			{
				list[j].pos=new_pos;
				new_pos++;
				if (dbp->Set(list[j].id, &list[j],sizeof(GxBusPmDataSat)) != true)
				{
#ifdef GX_BUS_PM_DBUG
					PM_PRINTF("[PM]---del sat err !!\n");
#endif
					GxCore_Free(list);
					return GXCORE_ERROR;
				}
			}
		}
	}
	GxCore_Free(list);
	//dbp->sync(dbp,0);这是对外的删除接口 由调用GxBus_PmSatDelete的地方自己同步
	//gx_pm_verify_write(GX_PM_VERIFY_OK);////放到同步里面去
	return GXCORE_SUCCESS;
}

/**
 * @brief 通过id号获取卫星数据
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_sat_get_by_id(uint32_t id, GxBusPmDataSat * sat)
{
	IDB *dbp = NULL;
	GxBusPmDataSat* p = NULL;
	uint32_t size = 0;
	if (sat == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	p = (GxBusPmDataSat*)(dbp->Get(id, &size));
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no sat exist id=%d!!\n",id);
#endif
		return GXCORE_ERROR;	//
	}
	memcpy(sat,p,sizeof(GxBusPmDataSat));
	GxCore_Free(p);
	return GXCORE_SUCCESS;
}

static status_t gx_pm_sat_add(GxBusPmDataSat * sat)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i,j;
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	if(gx_pm_max_sat_id == MAX_SAT_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_SAT_COUNT; i++)
		{
			if(gx_pm_sat_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}
	}
	else
	{
		start_id = gx_pm_max_sat_id+1;
		gx_pm_max_sat_id++;
		j = 1;//出错还原标记
	}
	sat->pos=(uint32_t)gx_pm_sat_count;
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	sat->id = start_id;
	if(dbp->Set(start_id, sat,sizeof(GxBusPmDataSat)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add sat err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_sat_id--;
		}
		dbp->Sync();
		return GXCORE_ERROR;
	}
	gx_pm_sat_count++;
	gx_pm_sat_id[start_id-1] = GX_PM_DBASE_ID_USE;
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;
}

/**
 * @brief tp数据库初始化
 * @param
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_tp_dbase_init(IDB *dbp)
{
	uint32_t id = 0;
	ICursor *cursor = dbp->CreateCursor();
	if(gx_pm_tp_count != 0)
	{
		delete cursor;
		return GXCORE_SUCCESS;//已经有恢复默认出场设置了
	}
	while(!cursor->Eof())
	{
		if(gx_pm_tp_count == (int32_t)MAX_TP_COUNT)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is overflow!!\n");
#endif
			delete cursor;
			return GXCORE_SUCCESS;
		}
		id = cursor->Key();
		if(id > MAX_TP_COUNT || id == 0)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is err!!\n");
#endif
			delete cursor;
			return GXCORE_ERROR;
		}
		gx_pm_tp_id[id-1] = GX_PM_DBASE_ID_USE;
		if(id > gx_pm_max_tp_id)
		{
			gx_pm_max_tp_id = id;
		}
		gx_pm_tp_count++;
		cursor->Next();
	};

	delete cursor;
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除某卫星下的tp数据
 * @param
 *
 * @return  GXCORE_SUCCESS:执行正常
 *          GXCORE_ERROR:执行失败
 */
static status_t gx_pm_tp_delete_by_sat(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	uint32_t *tp_id;
	uint32_t tp_num = 0;
	ICursor *cursor = NULL;

	if (id_arry == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	tp_id = (uint32_t *)GxCore_Malloc(sizeof(uint32_t) * MAX_TP_COUNT);
	if(tp_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]-TP--no memory !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();

	/*查找需要删除的tp*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataTP *p = NULL;
		p = (GxBusPmDataTP*)cursor->GetRaw(&size);
		if (p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get tp data err !!\n");
#endif
			delete cursor;
			GxCore_Free(tp_id);
			return GXCORE_ERROR;	//创建sat dbase失败
		}

		for(i=0; i<num; i++)
		{
			if(id_arry[i] == p->sat_id)
			{
				tp_id[tp_num] = p->id;
				tp_num++;
			}
		}
		GxCore_Free(p);
		cursor->Next();
	}
	/*没有需要删除的tp相当于删除完成*/
	if(tp_num == 0)
	{
		delete cursor;
		GxCore_Free(tp_id);
		return GXCORE_SUCCESS;
	}
	/*删除tp */
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<tp_num; i++)
	{
		input_id = tp_id[i];
		if (dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del tp err !!\n");
#endif
			dbp->Sync();
			delete cursor;
			GxCore_Free(tp_id);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_tp_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_tp_count--;
	}
	dbp->Sync();
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	delete cursor;
	GxCore_Free(tp_id);
	return GXCORE_SUCCESS;
}

/**
 * @brief 通过tp的id删除tp数据
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_tp_delete(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t i = 0,j=0,count_bak=0;
	uint32_t input_id = 0;
	uint16_t new_pos=0;
	uint32_t count = 0;
	ICursor *cursor = NULL;
	uint32_t size = 0;
	GxBusPmDataTP *p = NULL;

	if (id_arry == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	GxBusPmDataTP *list = (GxBusPmDataTP*)GxCore_Malloc(gx_pm_tp_count*sizeof(GxBusPmDataTP));
	if(list == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc err !!\n");
#endif
		return GXCORE_ERROR;
	}
	memset(list,0,gx_pm_tp_count*sizeof(GxBusPmDataTP));


	count = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		if(count == (uint32_t)gx_pm_tp_count)//找到了所有信息
		{
			break;
		}
		p = (GxBusPmDataTP *)cursor->GetRaw(&size);
		if (p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get tp data err !!\n");
#endif
			delete cursor;
			GxCore_Free(list);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		memcpy(&list[count],p,sizeof(GxBusPmDataTP));
		count++;
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;

	count_bak=(uint32_t)gx_pm_tp_count;
	/*删除tp */
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<num; i++)
	{
		input_id = id_arry[i];
		if(dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del tp err !!\n");
#endif
			dbp->Sync();
			GxCore_Free(list);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_tp_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_tp_count--;
		for(j=0;j<count_bak;j++)
		{
			if(list[j].id == input_id)
			{
				list[j].id =0;
				break;
			}
		}
	}
	if(gx_pm_tp_count > 0)
	{
		for(i=0;i<count_bak;i++)
		{
			if(list[i].id !=0)
			{
				list[i].pos = new_pos;
				new_pos++;
				input_id = list[i].id;
				if(input_id > MAX_TP_COUNT || input_id == 0)
				{
#ifdef GX_BUS_PM_DBUG
					GX_BUS_PM_PRINTF("[PM]---tp id is  err!!\n");
#endif
					GxCore_Free(list);
					return GXCORE_ERROR;
				}
				if (dbp->Set(input_id, &list[i], sizeof(GxBusPmDataTP)) != true)
				{
#ifdef GX_BUS_PM_DBUG
					PM_PRINTF("[PM]---add tp err !!\n");
#endif
					GxCore_Free(list);
					return GXCORE_ERROR;
				}
			}
		}
	}
	GxCore_Free(list);
	//dbp->sync(dbp,0);//对外接口调用，所以不再自己同步由调用GxBus_PmTpDelete的地方自己同步
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;
}

/**
 * @brief 通过id获取tp数据
 * @param
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_tp_get_by_id(uint32_t id, GxBusPmDataTP * tp)
{
	IDB *dbp = NULL;
	GxBusPmDataTP *p = NULL;
	uint32_t size = 0;
	if (tp == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	p = (GxBusPmDataTP *)dbp->Get(id, &size);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no tp exist id=%d!!\n",id);
#endif
		return GXCORE_ERROR;	//
	}
	memcpy(tp,p,sizeof(GxBusPmDataTP));
	GxCore_Free(p);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取某个卫星下的tp的数量
 * @param sat_id:所要查找的卫星
 *
 *
 * @return -1: 发生错误
 *        正常数字:tp数量
 */
static int32_t gx_pm_tp_num_get_by_sat(uint16_t sat_id)
{
	//GxBusPmViewInfo sys = {{0}};
	GxBusPmDataTP* tp = NULL;
	IDB *dbp = NULL;

	uint32_t i = 0;
	uint32_t k = 0;//记录relloc时需要的空间时原来空间的几倍
	uint32_t tp_num = 0;
	ICursor *cursor = NULL;


	//GxBus_PmViewInfoGet(&sys);

	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	if(gx_pm_tp_id_arry_by_sat != NULL)
	{
		GxCore_Free(gx_pm_tp_id_arry_by_sat);
		gx_pm_tp_id_arry_by_sat = NULL;
	}
	gx_pm_tp_count_by_sat = 0;
	/*先mallco 100空间*/
	gx_pm_tp_id_arry_by_sat = (uint16_t*)GxCore_Malloc(sizeof(uint16_t)*GX_PM_DBASE_MALLOC_PER_TIMR);
	if(gx_pm_tp_id_arry_by_sat == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc id_arry err !!\n");
#endif
		delete cursor;
		return -1;	//创建sat dbase失败
	}

	k = 2;//下一次GxCore_Realloc就是原来的两倍了
	while(!cursor->Eof())
	{
		uint32_t size = 0;

		tp = (GxBusPmDataTP*)cursor->GetRaw(&size);
		if(tp == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get tp data err !!\n");
#endif
			delete cursor;
			return -1;	//创建sat dbase失败
		}
		/*组相符*/
		if(tp->sat_id == sat_id)
		{
			gx_pm_tp_id_arry_by_sat[tp_num] = tp->id;
			tp_num++;
			i++;
			/*100个空间已经用完了 再追加100个*/
			if(i == GX_PM_DBASE_MALLOC_PER_TIMR)
			{
				gx_pm_tp_id_arry_by_sat = (uint16_t*)GxCore_Realloc(gx_pm_tp_id_arry_by_sat,
						sizeof(uint16_t)*GX_PM_DBASE_MALLOC_PER_TIMR*k);
				if(gx_pm_tp_id_arry_by_sat == NULL)
				{
#ifdef GX_BUS_PM_DBUG
					GX_BUS_PM_PRINTF("[PM]---GxCore_Realloc id_arry err !!\n");
#endif
					GxCore_Free(tp);
					delete cursor;
					return -1;	//创建sat dbase失败
				}
				k++;
				i = 0;
			}

		}
		GxCore_Free(tp);
		cursor->Next();
	}
	delete cursor;
	qsort(gx_pm_tp_id_arry_by_sat,tp_num,sizeof(uint16_t),gx_pm_tp_cmp_pos);
	gx_pm_tp_count_by_sat = tp_num;
	return tp_num;
}

static status_t gx_pm_tp_add(GxBusPmDataTP * tp)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i = 0;
	uint8_t j = 0;
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	if(gx_pm_max_tp_id == MAX_TP_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_TP_COUNT; i++)
		{
			if(gx_pm_tp_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}

	}
	else
	{
		start_id =gx_pm_max_tp_id+1;
		gx_pm_max_tp_id++;
		j = 1;//出错还原标记
	}
	tp->pos=gx_pm_tp_count;
	tp->id = start_id;
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	if(dbp->Set(start_id, tp, sizeof(GxBusPmDataTP)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add tp err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_tp_id--;
		}
		dbp->Sync();
		return GXCORE_ERROR;
	}
	gx_pm_tp_count++;
	gx_pm_tp_id[start_id-1] = GX_PM_DBASE_ID_USE;
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;
}

/**
 * @brief tp 按id排序
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static int32_t gx_pm_tp_cmp_pos(const void *p1, const void *p2)
{
	GxBusPmDataTP tp1 = {0},tp2={0};

	gx_pm_tp_get_by_id(*(uint16_t*)p1,&tp1);
	gx_pm_tp_get_by_id(*(uint16_t*)p2,&tp2);
	return tp1.pos-tp2.pos;
}

/**
 * @brief 节目数据库初始化
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static status_t gx_pm_prog_dbase_init(IDB *dbp)
{
	uint32_t id = 0;
	ICursor *cursor = dbp->CreateCursor();

	if(gx_pm_prog_count != 0)
	{
		delete cursor;
		return GXCORE_SUCCESS;//已经恢复默认数据了
	}
	while(!cursor->Eof())
	{
		if(gx_pm_prog_count == (int32_t)MAX_PROG_COUNT)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is overflow!!\n");
#endif
			delete cursor;
			return GXCORE_SUCCESS;
		}
		id = cursor->Key();
		if(id > MAX_PROG_COUNT || id == 0)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---dbase is err!!\n");
#endif
			delete cursor;
			return GXCORE_ERROR;
		}
		gx_pm_prog_id[id-1] = GX_PM_DBASE_ID_USE;
		if(id>gx_pm_max_prog_id)
		{
			gx_pm_max_prog_id = id;
		}
		gx_pm_prog_count++;
		cursor->Next();
	}
	delete cursor;
	return GXCORE_SUCCESS;
}


// the follow 3 function for phone app, to speed up shenbin
status_t GxBus_PmViewInfoMemSet(GxBusPmViewInfo* sys)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sys != NULL) {
		if (gx_pm_view_info_mem == NULL)
		{
			if ((gx_pm_view_info_mem = (GxBusPmViewInfo*)GxCore_Malloc(sizeof(GxBusPmViewInfo))) == NULL)
			{
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;
			}
		}
		else
		{
			if(memcmp(gx_pm_view_info_mem, sys, sizeof(GxBusPmViewInfo)) == 0)
			{
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_SUCCESS;
			}
		}
		memcpy(gx_pm_view_info_mem, sys, sizeof(GxBusPmViewInfo));
	}

	gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

status_t GxBus_PmViewInfoMemClear(void)
{
	GX_PM_LOCK(gx_pm_mutex_handle);

	if (gx_pm_view_info_mem != NULL) {
		GxCore_Free(gx_pm_view_info_mem);
		gx_pm_view_info_mem = NULL;
	}

	gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

status_t GxBus_PmViewInfoMemGet(GxBusPmViewInfo* sys)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sys != NULL)
	{
		if(gx_pm_view_info_mem != NULL)
		{
			memcpy(sys, gx_pm_view_info_mem, sizeof(GxBusPmViewInfo));
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_SUCCESS;
		}
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_ERROR;
}

static GxBusPmViewInfo* gx_pm_view_info_mem_get()
{
	return gx_pm_view_info_mem;
}

/**
 * @brief 根据是大画面还是小画面的节目列表进行数量的获取和列表的初始化
 * @param
 *
 * @return -1: 执行失败
 *         正常数字: 符合条件的节目数量
 */
static int32_t gx_pm_prog_num_get(void)//返回符合条件的prog的数量,并且生成对应的id数组
{
	GxBusPmViewInfo sys = {0};
	uint32_t prog_num = 0;
	int32_t ret = 0;
	uint8_t* file_path;

	// special for phone application
	GxBusPmViewInfo *sysMem = NULL;
	if ((sysMem=gx_pm_view_info_mem_get()) != NULL)
	{
		prog_num = gx_pm_prog_num_get_normal(sysMem);
		return prog_num;
	}
	// for phone application end

	gx_pm_view_info_get(&sys);

	if(sys.stream_type == GXBUS_PM_PROG_TV)
	{
		file_path = (uint8_t*)GX_PM_TV_USER_LIST_FILE_PATH;
	}
	else
	{
		file_path = (uint8_t*)GX_PM_RADIO_USER_LIST_FILE_PATH;
	}
	ret =  GX_PM_FS_EXIST((const char*)file_path);
	if(ret == GXCORE_FILE_EXIST)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---crate prog list from user list!!\n");
#endif
		prog_num = gx_pm_prog_num_get_user_list(file_path);
		return prog_num;
	}

	if(sys.status == VIEW_INFO_DISABLE)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---view info is invalid!!\n");
#endif
		return -1;
	}

	//if(sys.pip_switch == 0)
	//{
	prog_num = gx_pm_prog_num_get_normal(&sys);
	//}
	//else
	//{
	//	prog_num = gx_pm_prog_num_get_pip(&sys);
	//}

	return prog_num;
}

int32_t gx_pm_prog_add_list(uint32_t tuner_main, GxBusPmViewInfo* sys, GxBusPmDataProg* prog, GxBusPmProgSort* list)
{
	uint16_t tp_id = (sys->pip_main_tp_id)&0x7fff;
	switch(gx_pm_prog_taxis_mode)
	{
	case TAXIS_MODE_NON:
	case TAXIS_MODE_CAS:
	case TAXIS_MODE_TP:
	case TAXIS_MODE_TUNER:
		if(sys->pip_switch == 1)
		{
			uint16_t tuner_switch = (sys->pip_main_tp_id)&0x8000; // The highest bit indicates whether to filter only the current tuner
			if((prog->tuner == tuner_main && prog->tp_id == tp_id)||
					(tuner_main != prog->tuner && tuner_switch == 0))
			{
				list->id = prog->id;
				list->params1 = prog->pos;
				return 0;
			}
		}
		else
		{
			list->id = prog->id;
			list->params1 = prog->pos;
			return 0;
		}
		break;

	case TAXIS_MODE_LETTER:
		if(sys->pip_switch == 1)
		{
			if((prog->tuner == tuner_main && prog->tp_id == sys->pip_main_tp_id)||
					(tuner_main != prog->tuner))
			{
				list->id = prog->id;
				memcpy(list->params2,
						prog->prog_name,
						MAX_PROG_NAME);

				list->params2[MAX_PROG_NAME-1]=0;
				return 0;
			}
		}
		else
		{
			list->id = prog->id;
			memcpy(list->params2,
					prog->prog_name,
					MAX_PROG_NAME);

			list->params2[MAX_PROG_NAME-1]=0;
			return 0;
		}
		break;

	case TAXIS_MODE_SERVICE_ID:
		if(sys->pip_switch == 1)
		{
			if((prog->tuner == tuner_main && prog->tp_id == sys->pip_main_tp_id)||
					(tuner_main != prog->tuner))
			{
				list->id = prog->id;
				list->params1 = prog->service_id;
				return 0;
			}
		}
		else
		{
			list->id = prog->id;
			list->params1 = prog->service_id;
			return 0;
		}
		break;

	case TAXIS_MODE_SCRAMBLE:
		if(sys->pip_switch == 1)
		{
			if((prog->tuner == tuner_main && prog->tp_id == sys->pip_main_tp_id)||
					(tuner_main != prog->tuner))
			{
				list->id = prog->id;
				list->params1 = prog->scramble_flag;
				return 0;
			}
		}
		else
		{
			list->id = prog->id;
			list->params1 = prog->scramble_flag;
			return 0;
		}
		break;

	default:
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get prog num params err !!\n");
#endif
		return -1;	//创建sat dbase失败
		break;
	}
	return -1;
}

/**
 * @brief 获取节目的数量,函数内部会通过sys info中存储的观看信息进行初始化,因为
 * 不同的观看模式下,对于用户的节目可见数量是不同的,该接口还会同时提取
 * 符合观看模式的节目的id 组成数组,该数组用于用户通过pos取得节目信息时使用
 * 但是.这样做会导致每次获取节目数量是速度都很慢,下一版本考虑把根据sys info初始化
 * 节目id数组的工作提取出来,这样就需要用户在节目列表发生改变(更改观看模式,
 * 移动节目,删除节目,添加节目)时显示地调用GxBus_PmProgListInit接口.
 * @param
 *
 * @return  -1: 执行失败
 *          正常数字: 符合条件的节目数量
 */
static int32_t gx_pm_prog_num_get_normal(GxBusPmViewInfo* sys)//返回符合条件的prog的数量,并且生成对应的id数组
{
	//GxBusPmViewInfo sys = {{0}};
	GxBusPmDataProg* prog = NULL;
	IDB *dbp = NULL;
	GxBusPmDataTP tp = {0};
	GxBusPmDataSat sat = {0};
	uint32_t tuner_main = 0;
	ICursor *cursor = NULL;;

	uint32_t i = 0;
	uint32_t k = 0;//记录relloc时需要的空间时原来空间的几倍
	uint32_t prog_num = 0;


	//GxBus_PmViewInfoGet(&sys);
	if(sys->pip_switch == 1)
	{
		uint16_t tp_id = (sys->pip_main_tp_id)&0x7fff;
		gx_pm_tp_get_by_id(tp_id,&tp);
		gx_pm_sat_get_by_id(tp.sat_id,&sat);
		tuner_main = sat.tuner;
	}
	gx_pm_prog_group_mode = sys->group_mode;
	gx_pm_prog_taxis_mode = sys->taxis_mode;
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	if(gx_pm_prog_id_arry != NULL)
	{
		GxCore_Free(gx_pm_prog_id_arry);
		gx_pm_prog_id_arry = NULL;
	}
	gx_pm_prog_id_arry_num = 0;
	gx_pm_prog_id_arry = GxCore_Malloc(sizeof(GxBusPmProgSort)*GX_PM_DBASE_MALLOC_PER_TIMR);
	if(gx_pm_prog_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc id_arry err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	k = 2;//下一次GxCore_Realloc就是原来的两倍了
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if (prog == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			delete cursor;
			return -1;	//创建sat dbase失败
		}
		if(sys->gx_pm_prog_check != NULL
				&& sys->group_mode == GROUP_MODE_USER)//用户自定义过滤
		{
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			if(sys->gx_pm_prog_check(prog,&(((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num])) == 0)//符合要求
			{
				prog_num++;
				i++;
			}
			GX_PM_LOCK(gx_pm_mutex_handle);
		}
		else
		{
			/*组相符*/
			if(gx_pm_prog_check_group_mod(prog,sys))
			{
				/*排序模式相符*/
				if(gx_pm_prog_check_taxis_mod(prog,sys))
				{
					if (gx_pm_prog_check_user_callback(prog,sys,0) == 0)
					{
						if(gx_pm_prog_add_list(tuner_main, sys, prog, &(((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num])) == 0)
						{
							prog_num++;
							i++;
						}
					}
				}
			}
		}
		/*100个空间已经用完了 再追加100个*/
		if(i == GX_PM_DBASE_MALLOC_PER_TIMR)
		{
			gx_pm_prog_id_arry = GxCore_Realloc(gx_pm_prog_id_arry,
					sizeof(GxBusPmProgSort)*GX_PM_DBASE_MALLOC_PER_TIMR*k);
			if(gx_pm_prog_id_arry == NULL)
			{
#ifdef GX_BUS_PM_DBUG
				GX_BUS_PM_PRINTF("[PM]---GxCore_Realloc id_arry err !!\n");
#endif
				delete cursor;
				GxCore_Free(prog);
				return -1;	//创建sat dbase失败
			}
			k++;
			i = 0;
		}
		GxCore_Free(prog);
		cursor->Next();
	}
	delete cursor;
	switch(gx_pm_prog_taxis_mode)
	{
	case TAXIS_MODE_NON:
	case TAXIS_MODE_CAS:
	case TAXIS_MODE_TP:
	case TAXIS_MODE_TUNER:
	case TAXIS_MODE_SERVICE_ID:
	case  TAXIS_MODE_SCRAMBLE:
		if(sys->order == VIEW_INFO_REVERSE)
		{
			qsort(gx_pm_prog_id_arry,
					prog_num,
					sizeof(GxBusPmProgSort),
					gx_pm_prog_cmp_pos_reverse );
		}
		else
		{
			qsort(gx_pm_prog_id_arry,
					prog_num,
					sizeof(GxBusPmProgSort),
					gx_pm_prog_cmp_pos );
		}
		break;

	case TAXIS_MODE_LETTER:
		if(sys->order == VIEW_INFO_REVERSE)
		{
			qsort(gx_pm_prog_id_arry,
					prog_num,
					sizeof(GxBusPmProgSort),
					gx_pm_prog_cmp_name_reverse );
		}
		else
		{
			qsort(gx_pm_prog_id_arry,
					prog_num,
					sizeof(GxBusPmProgSort),
					gx_pm_prog_cmp_name);
		}
		break;
	case TAXIS_MODE_USER:
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		if(sys->gx_pm_prog_order != NULL)
		{
			qsort(gx_pm_prog_id_arry,
					prog_num,
					sizeof(GxBusPmProgSort),
					sys->gx_pm_prog_order);

		}
		GX_PM_LOCK(gx_pm_mutex_handle);
		break;
	}
	gx_pm_prog_id_arry_num = prog_num;
	return prog_num;
}

/**
 * @brief 读取用户自定义的节目列表，该节目列表是由
 *        GxBus_PmProgUserListSave创建的
 * @param
 *
 * @return  -1: 执行失败
 *              正常数字:符合条件的节目数量
 */
static int32_t gx_pm_prog_num_get_user_list(uint8_t* file_path)//返回符合条件的prog的数量,并且生成对应的id数组
{
	handle_t file = 0;
	uint32_t size = 0;
	uint32_t i = 0;
	uint32_t id = 0;

	if(gx_pm_prog_id_arry != NULL)
	{
		GxCore_Free(gx_pm_prog_id_arry);
		gx_pm_prog_id_arry = NULL;
	}
	file = GX_PM_FS_OPEN((const char*)file_path, FILE_MODE);
	if(file<0)
	{

#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--- prog user list open file err !!\n");
#endif
		return -1;
	}
	GX_PM_FS_READ(file,&size,1,sizeof(uint32_t));
	gx_pm_prog_id_arry_num = size;
	gx_pm_prog_id_arry = GxCore_Malloc(sizeof(GxBusPmProgSort)*size);
	if(gx_pm_prog_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--- user list no space !!\n");
#endif
		return -1;
	}
	// GX_PM_FS_SEEK(file,sizeof(uint32_t),1);
	for(i=0; i<size; i++)
	{
		GX_PM_FS_READ(file,&id,1,sizeof(uint32_t));
		((GxBusPmProgSort*)gx_pm_prog_id_arry)[i].id = id;
	}
	GX_PM_FS_CLOSE(file);
	return gx_pm_prog_id_arry_num;
}

#if 0
/**
 * @brief 获取小画面的节目的数量,函数内部会通过sys info中存储的观看信息进行初始化
 * 获取的是大画面锁定的tp的节目以及不属于大画面的tuner的另外一个tuner的节目
 * @param
 *
 * @return  -1: 执行失败
 *          正常数字: 符合条件的节目数量
 */
static int32_t gx_pm_prog_num_get_pip(GxBusPmViewInfo* sys)//返回符合条件的prog的数量,并且生成对应的id数组
{
	//GxBusPmViewInfo sys = {{0}};
	GxBusPmDataProg* prog = NULL;
	GxBusPmDataTP tp = {0};
	GxBusPmDataSat sat = {0};
	uint32_t tuner_main = 0;
	DB *dbp = NULL;
	DBT data, key;

	uint32_t i = 0;
	uint32_t k = 0;//记录relloc时需要的空间时原来空间的几倍
	uint32_t prog_num = 0;


	//GxBus_PmViewInfoGet(&sys);

	gx_pm_tp_get_by_id(sys->pip_main_tp_id,&tp);
	gx_pm_sat_get_by_id(tp.sat_id,&sat);
	tuner_main = sat.tuner;
	//gx_pm_prog_group_mode = sys->view_info.group_mode;
	//gx_pm_prog_taxis_mode = sys->view_info.taxis_mode;
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	if(gx_pm_prog_id_arry != NULL)
	{
		GxCore_Free(gx_pm_prog_id_arry);
		gx_pm_prog_id_arry = NULL;
	}
	gx_pm_prog_id_arry_num = 0;
	/*先mallco 100空间*/

	gx_pm_prog_id_arry = GxCore_Malloc(sizeof(GxBusPmProgSort)*GX_PM_DBASE_MALLOC_PER_TIMR);
	if(gx_pm_prog_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc id_arry err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	k = 2;//下一次GxCore_Realloc就是原来的两倍了
	if((dbp->seq(dbp,&key, &data, R_FIRST))==0)
	{
		prog = (GxBusPmDataProg*)(data.data);
		gx_pm_sat_get_by_id(tp.sat_id,&sat);
		/*tuner与大画面相同 tp也和大画面相同,或者所属tuner与大画面的tuner不同*/
		if(((prog->tuner == tuner_main && prog->tp_id == sys->pip_main_tp_id)||
					(tuner_main != prog->tuner))&&prog->service_type == sys->stream_type)
		{
			((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num].id = prog->id;
			((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num].pos = prog->pos;
			prog_num++;
			i++;
		}
	}
	while((dbp->seq(dbp,&key, &data, R_NEXT))==0)
	{
		prog = (GxBusPmDataProg*)(data.data);
		gx_pm_sat_get_by_id(tp.sat_id,&sat);
		/*tuner与大画面相同 tp也和大画面相同,或者所属tuner与大画面的tuner不同*/
		if(((prog->tuner == tuner_main && prog->tp_id == sys->pip_main_tp_id)||
					(tuner_main != prog->tuner))&&prog->service_type == sys->stream_type)
		{
			((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num].id = prog->id;
			((GxBusPmProgSort*)gx_pm_prog_id_arry)[prog_num].pos = prog->pos;
			prog_num++;
			i++;
			/*100个空间已经用完了 再追加100个*/
			if(i == GX_PM_DBASE_MALLOC_PER_TIMR)
			{
				gx_pm_prog_id_arry = GxCore_Realloc(gx_pm_prog_id_arry,
						sizeof(GxBusPmProgSort)*GX_PM_DBASE_MALLOC_PER_TIMR*k);
				if(gx_pm_prog_id_arry == NULL)
				{
#ifdef GX_BUS_PM_DBUG
					GX_BUS_PM_PRINTF("[PM]---GxCore_Realloc id_arry err !!\n");
#endif
					return -1;	//创建sat dbase失败
				}
				k++;
				i = 0;
			}
		}

	}

	qsort(gx_pm_prog_id_arry,
			prog_num,
			sizeof(GxBusPmProgSort),
			gx_pm_prog_cmp_pos);

	gx_pm_prog_id_arry_num = prog_num;
	return prog_num;
}
#endif

/**
 * @brief 删除某一tp下的节目
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static status_t gx_pm_prog_delete_by_tp(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	uint32_t* prog_id;
	uint32_t prog_num = 0;
	ICursor *cursor = NULL;

	if (id_arry == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	prog_id = (uint32_t *)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
	if(prog_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---no memory !!\n");
#endif
		return GXCORE_ERROR;
	}

	cursor = dbp->CreateCursor();
	/*查找需要删除的节目*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		for(i=0; i<num; i++)
		{
			if(id_arry[i] == p->tp_id)
			{
				prog_id[prog_num] = p->id;
				prog_num++;
			}
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	/*没有需要删除的节目相当于删除完成*/
	if(prog_num == 0)
	{
		GxCore_Free(prog_id);
		return GXCORE_SUCCESS;
	}
	/*删除节目 */
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<prog_num; i++)
	{
		input_id = prog_id[i];
		if (dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del prog err !!\n");
#endif
			dbp->Sync();
			GxCore_Free(prog_id);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_prog_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_prog_count--;
		/*删除节目扩展信息*/
		gx_pm_prog_ext_del(input_id);
	}
	if(dbp_prog_ext != NULL)
	{
		dbp_prog_ext->Sync();
	}
	dbp->Sync();
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	gx_pm_prog_num_get();;//这时候编辑了节目的状态，需要重建节目列表了
	GxCore_Free(prog_id);
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除某一卫星下的节目
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 * GXCORE_ERROR:执行失败
*/
static status_t gx_pm_prog_delete_by_sat(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	uint32_t* prog_id;
	uint32_t prog_num = 0;
	ICursor *cursor = NULL;

	if (id_arry == NULL) {
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	prog_id = (uint32_t *)GxCore_Malloc(sizeof(uint32_t) * MAX_PROG_COUNT);
	if(prog_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---no memory !!\n");
#endif
		return GXCORE_ERROR;
	}
	cursor = dbp->CreateCursor();
	/*查找需要删除的节目*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		for(i=0; i<num; i++)
		{
			if(id_arry[i] == p->sat_id)
			{
				prog_id[prog_num] = p->id;
				prog_num++;
			}
		}
		cursor->Next();
		GxCore_Free(p);
	}
	delete cursor;
	/*没有需要删除的节目相当于删除完成*/
	if(prog_num == 0)
	{
		GxCore_Free(prog_id);
		return GXCORE_SUCCESS;
	}
	/*删除节目 */
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<prog_num; i++)
	{
		input_id = prog_id[i];
		if (dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del prog err !!\n");
#endif
			dbp->Sync();
			GxCore_Free(prog_id);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_prog_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_prog_count--;
	}
	dbp->Sync();
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	gx_pm_prog_num_get();;//这时候编辑了节目的状态，需要重建节目列表了
	GxCore_Free(prog_id);
	return GXCORE_SUCCESS;
}

/**
 * @brief 检测节目是否符合观看模式
 * @param
 *
 * @return 1: 符合
 *         0: 不符合
 */
static uint8_t gx_pm_prog_check_group_mod(GxBusPmDataProg*prog,GxBusPmViewInfo* sys)
{
	uint32_t fav_id = 0;
	switch(sys->skip_view_switch)
	{
	case VIEW_INFO_SKIP_CONTAIN:
		break;

	case VIEW_INFO_SKIP_VANISH:
		if(GXBUS_PM_PROG_BOOL_ENABLE == prog->skip_flag)
		{
			return 0;
		}
		break;
	default:
		break;
	}
	switch(sys->group_mode)
	{
	case GROUP_MODE_ALL://all
		return 1;
		break;

	case GROUP_MODE_SAT://sat
		if(prog->sat_id == sys->sat_id)
		{
			return 1;
		}
		break;

	case GROUP_MODE_FAV://fav
		fav_id = sys->fav_id-1;//id是从1开始的
		if(((prog->favorite_flag)>>fav_id)&1)
		{
			return 1;
		}
		break;

	case GROUP_MODE_FAV|GROUP_MODE_SAT:
		if(prog->sat_id == sys->sat_id)
		{
			fav_id = sys->fav_id-1;//id是从1开始的
			if(((prog->favorite_flag)>>fav_id)&1)
			{
				return 1;
			}
		}
		break;
    case GROUP_MODE_HD:
        if (prog->definition == GXBUS_PM_PROG_HD)
        {
            return 1;
        }
        break;

	default:
		break;
	}
	return 0;
}

/**
 * @brief 获取字母排序排序模式
 * @param
 *
 * @return  0: a~z
 *          1: 字符排序
 *         ff: other
 *
 */
static uint8_t gx_pm_prog_get_taxis_mod(GxBusPmViewInfo* sys)
{
	uint8_t* p = NULL;
	uint8_t i = 0;

	p = sys->letter;
	for(i = 0; i<4; i++)
	{
		if(*p != '\0' && *p != 0xff)
		{
			return 1;
		}
		p++;
	}
	p--;
	if(*p == '\0')
	{
		return 0;
	}
	else if(*p == 0xff)
	{
		return 0xff;
	}

	return 1;
}

/**
 * @brief 检测节目时否符合排序模式
 * @param
 *
 * @return  1: 符合
 *          0: 不符合
 */
static uint8_t gx_pm_prog_check_taxis_mod(GxBusPmDataProg*prog,GxBusPmViewInfo* sys)
{
	uint8_t letter_mode = 0;
	uint8_t letter_first = 0;
	//uint8_t letter_capital = 0;
	//uint8_t letter_lowercase = 0;
	uint8_t prog_name_comp[MAX_CMP_NAME];
	uint8_t sys_letter_comp[MAX_CMP_NAME];
	uint32_t i = 0;

	//	GxBusPmDataSat  sat;
	if((prog->service_type != sys->stream_type) &&
			(sys->stream_type != GXBUS_PM_PROG_ALL))
	{
		return 0;
	}
	switch(sys->taxis_mode)
	{
	case TAXIS_MODE_NON://all
	case TAXIS_MODE_SERVICE_ID:
	case TAXIS_MODE_SCRAMBLE:
		return 1;

	case TAXIS_MODE_CAS://sat
		if(prog->cas_id == sys->cas_id)
		{
			return 1;
		}
		break;

	case TAXIS_MODE_LETTER:
		letter_mode = gx_pm_prog_get_taxis_mod(sys);
		letter_first = prog->prog_name[0];
		if(letter_mode == 0)//a~z
		{
			return 1;
		}
		else if(letter_mode == 0xff)//other
		{
			if(letter_first<'A'||
					letter_first>'Z'||
					letter_first<'a'||
					letter_first>'z')
			{
				return 1;
			}
		}
		else//某个字母
		{
			memcpy(prog_name_comp,prog->prog_name,MAX_CMP_NAME);
			memcpy(sys_letter_comp,sys->letter,MAX_CMP_NAME);
			gx_pm_lowercase_to_majuscule(prog_name_comp,MAX_CMP_NAME);
			gx_pm_lowercase_to_majuscule(sys_letter_comp,MAX_CMP_NAME);

			for(i = 0; i<MAX_CMP_NAME; i++)
			{
				if(prog_name_comp[i] != sys_letter_comp[i] &&
						sys_letter_comp[i] != '\0')
				{
					return 0;
				}
			}
			return 1;
		}

		break;

	case TAXIS_MODE_TP:
		if(prog->tp_id == sys->tp_id)
		{
			return 1;
		}
		break;

	case TAXIS_MODE_TUNER:
		if(prog->tuner== sys->tuner_num)
		{
			return 1;
		}
		break;

	default:
		break;
	}
	return 0;
}

/**
 * @brief 节目位置的比较,用于按照pos对接目进行排序
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static int32_t gx_pm_prog_cmp_pos(const void *p1, const void *p2)

{
	int32_t pos1 = 0;
	int32_t pos2 = 0;
	pos1 = ((GxBusPmProgSort*)p1)->params1;
	pos2 = ((GxBusPmProgSort*)p2)->params1;
	return pos1 - pos2;
}

/**
 * @brief 节目位置的比较,用于按照pos对接目进行逆向排序
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static int32_t gx_pm_prog_cmp_pos_reverse(const void *p1, const void *p2)

{
	int32_t pos1 = 0;
	int32_t pos2 = 0;
	int32_t i = 0;
	pos1 = ((GxBusPmProgSort*)p1)->params1;
	pos2 = ((GxBusPmProgSort*)p2)->params1;
	if(pos1 - pos2 >0)
	{
		i = -1;
	}
	else if(pos1 - pos2 == 0)
	{
		i = 0;
	}
	else
	{
		i = 1;
	}
	return i;
}

/**
 * @brief 在执行节目移动操作时对所要移动的节目的位置进行排序
 * @param

 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static int32_t gx_pm_prog_move_cmp_pos(const void *p1, const void *p2)
{
	return *(uint32_t*)p1 - *(uint32_t*)p2;
}

/**
 * @brief 节目名字的比较函数,主要用于按照字母排序节目名
 * @param
 *
 * @return  GXCORE_SUCCESS: 执行正常
 *          GXCORE_ERROR: 执行失败
 */
static int32_t gx_pm_prog_cmp_name(const void *p1, const void *p2)
{
	uint8_t* name1 = 0;
	uint8_t* name2 = 0;
	name1 = ((GxBusPmProgSort*)p1)->params2;
	name2 = ((GxBusPmProgSort*)p2)->params2;

	return strcasecmp((const char*)name1,(const char*)name2);
}

/**
 * @brief 节目名字的比较函数,主要用于按照字母逆向排序节目名
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
static int32_t gx_pm_prog_cmp_name_reverse(const void *p1, const void *p2)

{
	uint8_t* name1 = 0;
	uint8_t* name2 = 0;
	int32_t i = 0;
	name1 = ((GxBusPmProgSort*)p1)->params2;
	name2 = ((GxBusPmProgSort*)p2)->params2;
	if(strcasecmp((const char*)name1,(const char*)name2)>0)
	{
		i = -1;
	}
	else if(strcasecmp((const char*)name1,(const char*)name2)==0)
	{
		i = 0;
	}
	else
	{
		i = 1;
	}
	return i;
}

/**
 * @brief 移动节目时获取移动范围内的最大pos和最小pos
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static status_t gx_pm_prog_max_min_pos_get(uint32_t * id_arry,
		uint32_t num,
		uint32_t target_id,
		uint32_t * max,
		uint32_t * min,
		IDB *dbp)
{
	uint32_t i = 0;

	//      uint32_t j;
	//      uint32_t k =0;
	uint32_t * pos_arry = NULL;	//用于存储找到的pos值 等会儿比较
	uint32_t *real_id_arry = NULL;	//还得加上目标id
	uint32_t input_id = 0;

	struct gx_pm_prog_pos_get pos_get;
	pos_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * (num + 1));
	if(pos_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		return GXCORE_ERROR;
	}
	real_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * (num + 1));
	if(real_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		GxCore_Free(pos_arry);
		return GXCORE_ERROR;
	}
	memcpy(real_id_arry, id_arry, sizeof(uint32_t) * num);
	real_id_arry[num] = target_id;
	pos_get.i = 0;
	pos_get.pos_id_arry = pos_arry;

	/*获取传下来的所有id的pos */
	for (i = 0; i < num + 1; i++)
	{
		GxBusPmDataProg *p = NULL;
		uint32_t size = 0;

		input_id = real_id_arry[i];

		p = (GxBusPmDataProg *)dbp->Get(input_id, &size);

		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog by  err !!\n");
#endif
			GxCore_Free(pos_arry);
			GxCore_Free(real_id_arry);
			return GXCORE_ERROR;
		}

		pos_get.pos_id_arry[pos_get.i] = p->pos;
		pos_get.i+=1;
		GxCore_Free(p);
	}
	if (pos_get.i != num + 1)
	{
		GxCore_Free(pos_arry);
		GxCore_Free(real_id_arry);
		return GXCORE_ERROR;
	}
	*min = pos_arry[0];
	*max = pos_arry[0];
	for (i = 0; i < pos_get.i; i++)
	{
		*min = (*min) < (pos_arry[i]) ? (*min) : (pos_arry[i]);
		*max = (*max) > (pos_arry[i]) ? (*max) : (pos_arry[i]);
	}
	GxCore_Free(pos_arry);
	GxCore_Free(real_id_arry);
	return GXCORE_SUCCESS;
}

/**
 * @brief 当进行节目移动操作时,获取需要修改的节目的数量
 * @param
 *
 * @return count:所要修改的节目的总数
 *
 */
static int32_t gx_pm_prog_modify_count_get(uint32_t max_pos, uint32_t min_pos,IDB* dbp)
{
	int32_t count = 0;
	uint32_t pos = 0 ;
	ICursor *cursor = dbp->CreateCursor();

	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg*)cursor->GetRaw(&size);
		pos = p->pos;
		if((pos<=max_pos)&&(pos>=min_pos))
		{
			count++;
		}
		cursor->Next();
		GxCore_Free(p);
	}
	delete cursor;
	return count;
}

/**
 * @brief 移动节目时对所需要更改的pos进行了从小到大的排序排序
 * @param
 *
 * @return GXCORE_SUCCESS: 执行正常
 *         GXCORE_ERROR: 执行失败
 */
status_t gx_pm_prog_pos_init(uint32_t * pos_arry,
		uint32_t max_pos,
		uint32_t min_pos,
		uint32_t target_id,
		uint32_t * target_id_pos,
		IDB* dbp)
{
	uint32_t pos = 0;
	struct gx_pm_prog_pos_get pos_get;
	pos_get.i = 0;
	pos_get.pos_id_arry = pos_arry;
	ICursor *cursor = dbp->CreateCursor();


	/*先把游标拉回first,因为gx_pm_prog_modify_count_get函数会把游标移到最后一个数据*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		pos = p->pos;
		/*berkeley db里不需要这样作 直接往pos_arry赋值就好 暂时不改*/
		if((pos<=max_pos)&&(pos>=min_pos))
		{
			pos_get.pos_id_arry[pos_get.i] = pos;
			pos_get.i += 1;
		}
		if(p->id == target_id)
		{
			*target_id_pos = pos;
		}
		cursor->Next();
		GxCore_Free(p);
	}
	delete cursor;
	qsort(pos_arry,
			pos_get.i,
			sizeof(uint32_t),
			gx_pm_prog_move_cmp_pos);
	return 0;
}

/**
 * @brief 移动节目时以目标id为界线,分别初始化了目标id之前的节目和目标id之后的节目
 *        获取了哪些id时需要移动到目标id之前的 那些id时需要移动到目标id之后的
 * @param
 *
 * @return GXCORE_SUCCESS:执行正常
 *         GXCORE_ERROR:执行失败
 */
static status_t gx_pm_prog_before_after_buff_init(uint32_t * before_target_id,
		uint32_t * after_target_id,
		uint32_t * before_count,
		uint32_t * after_count,
		uint32_t max_pos,
		uint32_t min_pos,
		uint32_t target_id_pos,
		uint32_t * id_arry,
		uint32_t num,IDB*dbp)
{
	GxBusPmProgSort *id_temp1 = NULL;//保存target_id_pos前的id
	GxBusPmProgSort *id_temp2 = NULL;//保存target_id_pos后的id
	uint32_t count_temp = 0;
	uint32_t count = 0;
	uint32_t i = 0;
	uint32_t j =0;
	uint32_t k =0;
	uint32_t pos = 0;
	ICursor *cursor = dbp->CreateCursor();


	//该空间应该是所要修改的全部id数量那么大 传下来的before_count记录了最大数量
	id_temp1 = (GxBusPmProgSort*)GxCore_Malloc(sizeof(GxBusPmProgSort) * ((*before_count + num + 1)));
	if(id_temp1 == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		delete cursor;
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	id_temp2 =(GxBusPmProgSort*)GxCore_Malloc(sizeof(GxBusPmProgSort) * ((*before_count + num + 1)));
	if(id_temp2 == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		GxCore_Free(id_temp1);
		delete cursor;
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*获取before_target_id */
	/*先把游标拉回first,因为gx_pm_prog_pos_init函数会把游标移到最后一个数据*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg*) cursor->GetRaw(&size);
		pos = p->pos;
		if((pos<target_id_pos)&&(pos>min_pos))
		{
			id_temp1[i].id = p->id;
			id_temp1[i].params1 = p->pos;
			i += 1;
		}
		else if((pos<max_pos)&&(pos>target_id_pos))
		{
			id_temp2[j].id = p->id;
			id_temp2[j].params1 = p->pos;
			j += 1;
		}
		cursor->Next();
		GxCore_Free(p);
	}
	delete cursor;
	qsort(id_temp1,
			i,
			sizeof(GxBusPmProgSort),
			gx_pm_prog_cmp_pos);
	qsort(id_temp2,
			j,
			sizeof(GxBusPmProgSort),
			gx_pm_prog_cmp_pos);


	count_temp = i;

	/*去除掉所要移动的id */
	if (count_temp != 0) {
		for (k = 0; k < count_temp; k++) {
			if (gx_pm_prog_id_exist_check(id_temp1[k].id, id_arry, num)) {
				before_target_id[count] = id_temp1[k].id;
				count++;
			}
		}
		*before_count = count;
	}

	else {
		*before_count = 0;
	}

	count_temp = j;
	count = 0;

	/*去除掉所要移动的id */
	if (count_temp != 0) {
		for (k = 0; k < count_temp; k++) {
			if (gx_pm_prog_id_exist_check(id_temp2[k].id, id_arry, num)) {
				after_target_id[count] = id_temp2[k].id;
				count++;
			}
		}
		*after_count = count;
	}

	else {
		*after_count = 0;
	}
	GxCore_Free(id_temp1);
	GxCore_Free(id_temp2);
	return GXCORE_SUCCESS;
}

/**
 * @brief 检测source id是否已经在id arry中
 * @param
 *
 * @return 0: 已经在id arry中
 *         1: 不在id arry中
 */
static uint8_t gx_pm_prog_id_exist_check(uint32_t source_id, uint32_t * id_arry, uint32_t num)
{
	uint32_t i;
	for (i = 0; i < num; i++) {
		if (id_arry[i] == source_id) {
			return 0;
		}
	}
	return 1;
}

/**
 * @brief 移动节目时,进行节目位置的更新
 * @param
 *
 * @return  GXCORE_SUCCESS:执行正常
 *          GXCORE_ERROR:执行失败
 */
status_t gx_pm_prog_pos_update(uint32_t * real_id_arry, uint32_t * real_pos_arry, uint32_t num,IDB*dbp)
{
	GxBusPmDataProg prog = {0};
	uint32_t input_id = 0;
	uint32_t i = 0;

	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for (i = 0; i < num; i++)
	{
		GxBusPmDataProg *p = NULL;
		uint32_t size = 0;
		/*获取原始数据*/
		input_id = real_id_arry[i];
		p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---no prog exist id=%d!!\n",input_id);
#endif
			return GXCORE_ERROR;	//
		}
		memcpy(&prog,p,sizeof(GxBusPmDataProg));
		/*获取修改值 */
		prog.pos = real_pos_arry[i];
		if (dbp->Set(prog.id, &prog, sizeof(GxBusPmDataProg)) != true)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---modify prog err !!\n");
#endif
			GxCore_Free(p);
			return GXCORE_ERROR;
		}
		GxCore_Free(p);
	}
	return GXCORE_SUCCESS;
}

static status_t	gx_pm_prog_url_fre_get(GxBusPmDataTP* tp,
		GxBusPmDataSat* sat,
		uint32_t* fre)
{
	uint32_t lnb_fre = 0;
	GxBusPmSatLnbType type = GXBUS_PM_SAT_LNB_UNIVERSAL;

	if(sat->sat_s.lnb2 == 0
			||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
		type = GXBUS_PM_SAT_LNB_USER;
	}
	else if(sat->sat_s.lnb1<6000)
	{
		type = GXBUS_PM_SAT_LNB_OCS;
	}
	else if(sat->sat_s.lnb1>6000)
	{
		type = GXBUS_PM_SAT_LNB_UNIVERSAL;

		//for Multi-Point LNBF #359333
		if((sat->sat_s.lnb1==GXBUS_FRONTEND_LNBF_LOW_FREQ_MHZ) ||
				(sat->sat_s.lnb1==GXBUS_FRONTEND_LNBF_HIGH_FREQ_MHZ))
			type = GXBUS_PM_SAT_LNBF;
	}
	switch(type)
	{
	case GXBUS_PM_SAT_LNB_USER:
		lnb_fre = sat->sat_s.lnb1;
		break;

	case GXBUS_PM_SAT_LNB_UNIVERSAL:
		if(tp->frequency >= 11700)
		{
			lnb_fre = sat->sat_s.lnb2;
		}
		else
		{
			lnb_fre = sat->sat_s.lnb1;
		}
		break;

	case GXBUS_PM_SAT_LNB_OCS:
	case GXBUS_PM_SAT_LNBF:

		switch(tp->tp_s.polar)
		{
		case GXBUS_PM_TP_POLAR_H:
			lnb_fre = sat->sat_s.lnb1;
			break;

		case GXBUS_PM_TP_POLAR_V:
			lnb_fre = sat->sat_s.lnb2;
			break;

		case GXBUS_PM_TP_POLAR_AUTO:
			lnb_fre = sat->sat_s.lnb1;
			break;

		default:
#ifdef GX_BUS_SEARCH_DBUG
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat polar err!!\n");
#endif
			return GXCORE_ERROR;
			break;
		}
		break;

	default:
#ifdef GX_BUS_SEARCH_DBUG
		GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb type err!!\n");
#endif
		return GXCORE_ERROR;
		break;
	}
	if(lnb_fre >= tp->frequency)
	{
		*fre = lnb_fre - tp->frequency;
	}
	else
	{
		*fre = tp->frequency - lnb_fre;
	}
	return GXCORE_SUCCESS;
}

static status_t	gx_pm_prog_url_polar_get(GxBusPmDataTP* tp,
		GxBusPmDataSat* sat,
		fe_sec_voltage_t* polar)
{
	switch(sat->sat_s.lnb_power)
	{
	case GXBUS_PM_SAT_LNB_POWER_OFF:
		*polar = SEC_VOLTAGE_OFF;
		break;

	case GXBUS_PM_SAT_LNB_POWER_13V:
		*polar = SEC_VOLTAGE_13;
		break;

	case GXBUS_PM_SAT_LNB_POWER_18V:
		*polar = SEC_VOLTAGE_18;
		break;

	case GXBUS_PM_SAT_LNB_POWER_ON:
		switch(tp->tp_s.polar)
		{
		case GXBUS_PM_TP_POLAR_H:
			*polar = SEC_VOLTAGE_18;
			break;

		case GXBUS_PM_TP_POLAR_V:
			*polar = SEC_VOLTAGE_13;
			break;

		case GXBUS_PM_TP_POLAR_AUTO:
			*polar = SEC_VOLTAGE_18;
			break;

		default:
#ifdef GX_BUS_SEARCH_DBUG
			GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---tp polar type err!!\n");
#endif
			return GXCORE_ERROR;
			break;
		}
		break;

	default:
#ifdef GX_BUS_SEARCH_DBUG
		GX_BUS_SEARCH_ERRO_PRINTF("[SEARCH]---sat lnb power err!!\n");
#endif
		return GXCORE_ERROR;
		break;
	}

	return GXCORE_SUCCESS;
}

static status_t gx_pm_prog_url_22k_get(GxBusPmDataTP* tp,GxBusPmDataSat* sat,fe_sec_tone_mode_t* sat22k)
{
	if(sat->sat_s.lnb2 == 0
			||sat->sat_s.lnb2 == sat->sat_s.lnb1)
	{
		if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_OFF;
		}
		else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_ON;
		}
		else
		{
			*sat22k = SEC_TONE_ON;
		}
	}
	else if(sat->sat_s.lnb1<6000)
	{
		if(GXBUS_PM_SAT_22K_OFF == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_OFF;
		}
		else if(GXBUS_PM_SAT_22K_ON == sat->sat_s.switch_22K)
		{
			*sat22k = SEC_TONE_ON;
		}
		else
		{
			*sat22k = SEC_TONE_ON;
		}
	}
	else if(sat->sat_s.lnb1>6000)
	{
		if(tp->frequency >= 11700)
		{
			*sat22k = SEC_TONE_ON;
		}
		else
		{
			*sat22k = SEC_TONE_OFF;
		}
	}
	return GXCORE_SUCCESS;
}

static int32_t gx_pm_prog_get_by_pos(uint32_t pos, uint32_t num, GxBusPmDataProg * prog_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	dbp = dbp_prog;
	if(dbp ==NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	/*prog的数量是当前观看模式下的数量*/
	count = 0;
	for(i=0; i<num; i++)
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		input_id = ((GxBusPmProgSort*)(gx_pm_prog_id_arry))[pos+i].id;
		p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog by  err !!\n");
#endif
			return -1;
		}
		memcpy(&prog_arry[count],p,sizeof(GxBusPmDataProg));
		count++;
		GxCore_Free(p);
	}
	return count;
}

static status_t gx_pm_prog_original_pos_get(uint32_t*pos_arry)
{
	uint32_t i = 0;
	GxBusPmDataProg prog;

	for(i=0; i<gx_pm_prog_id_arry_num; i++ )
	{
		gx_pm_prog_get_by_pos(i,1,&prog);
		pos_arry[i] = prog.pos;
	}
	return GXCORE_SUCCESS;
}

static status_t gx_pm_prog_id_get_by_pos(uint32_t*pos_arry,uint32_t*id_arry)
{
	uint32_t i = 0;
	GxBusPmDataProg prog;

	for(i=0; i<gx_pm_prog_id_arry_num; i++ )
	{
		gx_pm_prog_get_by_pos(pos_arry[i],1,&prog);
		id_arry[i] = prog.id;
	}
	return GXCORE_SUCCESS;
}

static status_t gx_pm_prog_add(GxBusPmDataProg* prog)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	if(gx_pm_max_prog_id == MAX_PROG_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_PROG_COUNT; i++)
		{
			if(gx_pm_prog_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}

	}
	else
	{
		start_id =gx_pm_max_prog_id+1;
		gx_pm_max_prog_id++;
		j = 1;//出错还原标记
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	prog->id = start_id;
	prog->pos = start_id;//起始位置和id是一样的,这里有个问题,如果是通过查找未使用id的方式获得的id,那么起始位置就不是在最后!!!!!!
	if (dbp->Set(start_id, prog, sizeof(GxBusPmDataProg)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add prog err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_prog_id--;
		}
		dbp->Sync();
		return GXCORE_ERROR;
	}
	gx_pm_prog_count++;
	gx_pm_prog_id[start_id-1] = GX_PM_DBASE_ID_USE;

	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;
}

status_t gx_pm_prog_get_by_id(uint32_t id, GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	uint32_t size = 0;
	GxBusPmDataProg *p = NULL;
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = id;

	p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no prog exist id=%d!!\n",id);
#endif
		return GXCORE_ERROR;	//
	}
	memcpy(prog,p,sizeof(GxBusPmDataProg));
	GxCore_Free(p);
	return GXCORE_SUCCESS;
}

static status_t gx_pm_prog_info_modify(GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	input_id = prog->id;
	if (dbp->Set(input_id, prog, sizeof(GxBusPmDataProg)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---modify prog err !!\n");
#endif
		dbp->Sync();
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;
}

static status_t gx_pm_prog_ext_modify(uint32_t id,uint32_t size,void* p)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	dbp = dbp_prog_ext;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	input_id = id;
	if (dbp->Set(input_id, p, size) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---modify prog err !!\n");
#endif
		dbp->Sync();
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	return GXCORE_SUCCESS;

}
static status_t gx_pm_prog_ext_get(uint32_t prog_id,uint32_t size,void* p)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	dbp = dbp_prog_ext;
	uint32_t s = 0;
	void *pp = NULL;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog ext dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = prog_id;

	pp = dbp->Get(input_id, &s);
	if(pp == 0 || s != size)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no prog ext exist id!!\n");
#endif
		return GXCORE_ERROR;	//
	}
	memcpy(p,pp,size);
	GxCore_Free(pp);
	return GXCORE_SUCCESS;
}
static status_t gx_pm_prog_ext_del(uint32_t id)
{
	IDB *dbp = NULL;

	dbp = dbp_prog_ext;
	if(dbp == NULL)
	{
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	if (dbp->Remove(id) != true)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---del prog ext err !!\n");
#endif
		dbp->Sync();
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}
/**
 * @brief 初始化fav数据库,
 * @param

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
static status_t gx_pm_fav_group_init(IDB *dbp)
{
	uint32_t i = 0;
	uint32_t start_key = 0;
	GxBusPmDataFavItem fav = {0};
	GxBusPmDataFavItem *p = NULL;
	uint8_t unkonwn_name[MAX_FAV_GROUP_NAME];
	uint32_t size = 0;

	/*检测存不存在fav*/
	start_key = 1;
	p = (GxBusPmDataFavItem *)dbp->Get(start_key, &size);
	if(p != NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---fav have exsited !!\n");
#endif
		GxCore_Free(p);
		return GXCORE_SUCCESS;	//创建fav table失败
	}
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<8; i++)
	{
		start_key = i+1;//id 要从1开始
		memset(&fav,0,sizeof(GxBusPmDataFavItem));
		fav.id = start_key;
		memcpy(fav.fav_name,fav_name[i],MAX_FAV_GROUP_NAME);
		fav.fav_name[MAX_FAV_GROUP_NAME-1] = 0;
		if (dbp->Set(start_key, &fav, sizeof(GxBusPmDataFavItem)) != true)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---init fav dbase err !!\n ");
#endif
			return GXCORE_ERROR;	//创建fav table失败
		}
	}
	start_key = 0;
	for (i = 0; i < 24; i++)
	{
		start_key = 9+i;//id从9开始
		sprintf((char *)unkonwn_name, "%s%d",(char *)(fav_name[8]), i);
		memset(&fav,0,sizeof(GxBusPmDataFavItem));
		fav.id = start_key;
		memcpy(fav.fav_name,unkonwn_name,MAX_FAV_GROUP_NAME);
		fav.fav_name[MAX_FAV_GROUP_NAME-1] = 0;
		if (dbp->Set(start_key, &fav, sizeof(GxBusPmDataFavItem)) != true)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---init fav dbase err !!\n");
#endif
			return GXCORE_ERROR;	//创建fav table失败
		}
	}
	dbp->Sync();//把添加好的fav写入文件
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	return GXCORE_SUCCESS;
}

#if 0
/**
 * @brief 初始化stream数据库
 * @param

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
static status_t gx_pm_stream_dbase_init(DB *dbp)
{
	uint32_t id = 0;
	DBT  key;

	memset(&key,0,sizeof(DBT));
	if((dbp->seq(dbp,&key, NULL, R_FIRST))==0)
	{
		id = *((uint32_t*)(key.data));
		if(id>gx_pm_max_stream_id)
		{
			gx_pm_max_stream_id = id;
		}
	}
	while((dbp->seq(dbp,&key, NULL, R_NEXT))==0)
	{
		id = *((uint32_t*)(key.data));
		if(id>gx_pm_max_stream_id)
		{
			gx_pm_max_stream_id = id;
		}
	}

	return GXCORE_SUCCESS;
}

/**
 * @brief 获取由prog_id决定的节目的所有stream信息
 * @param

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
static int32_t gx_pm_stream_get(uint32_t prog_id, uint32_t num, GxBusPmDataStream * stream_arry)
{
	uint32_t count = 0;
	DB *dbp = NULL;
	DBT data, key;
	if(stream_arry==NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get stream params err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));
	if((dbp->seq(dbp,&key, &data, R_FIRST))==0)
	{
		if(((GxBusPmDataStream*)(data.data))->prog_id == prog_id)
		{
			memcpy(&stream_arry[count],(GxBusPmDataStream*)(data.data),sizeof(GxBusPmDataStream));
			count++;
			if(count == num)
			{
				return count;
			}

		}
	}
	while((dbp->seq(dbp,&key, &data, R_NEXT))==0)
	{

		if(((GxBusPmDataStream*)(data.data))->prog_id == prog_id)
		{
			memcpy(&stream_arry[count],(GxBusPmDataStream*)(data.data),sizeof(GxBusPmDataStream));
			count++;
			if(count == num)
			{
				break;
			}

		}

	}
	return count;
}

/**
 * @brief 获取由prog_id决定的节目的stream的数量
 * @param

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
static int32_t gx_pm_stream_num_get(uint32_t prog_id)
{
	uint32_t count = 0;
	DB *dbp = NULL;
	DBT data, key;
	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));
	if((dbp->seq(dbp,&key, &data, R_FIRST))==0)
	{
		if(((GxBusPmDataStream*)(data.data))->prog_id == prog_id)
		{
			count++;
		}
	}
	while((dbp->seq(dbp,&key, &data, R_NEXT))==0)
	{

		if(((GxBusPmDataStream*)(data.data))->prog_id == prog_id)
		{
			count++;
		}

	}
	return count;
}
#endif
/**
 * @brief 删除prog_id决定的?谀康膕tream
 * @param

 * @return     	GXCORE_SUCCESS:执行正??
GXCORE_ERROR:执行失败
*/
static status_t gx_pm_stream_del(uint32_t prog_id)
{
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	uint32_t stream_id[100];
	uint32_t stream_num = 0;
	ICursor *cursor = NULL;

	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	cursor = dbp->CreateCursor();
	/*查找需要删除的stream*/
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataStream *p = NULL;

		p = (GxBusPmDataStream*)cursor->GetRaw(&size);
		if(prog_id == p->prog_id)
		{
			stream_id[stream_num] = cursor->Key();
			stream_num++;
		}
		cursor->Next();
		GxCore_Free(p);
	}
	delete cursor;
	/*没有需要删除的stream相当于删除完成*/
	if(stream_num == 0)
	{
		return GXCORE_SUCCESS;
	}
	/*删除节目 */
	for(i=0; i<stream_num; i++)
	{
		input_id = stream_id[i];
		if (dbp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del stream err !!\n");
#endif
			dbp->Sync();
			return GXCORE_ERROR;
		}
	}
	dbp->Sync();
	return GXCORE_SUCCESS;
}


static status_t gx_pm_view_info_get(GxBusPmViewInfo* sys)
{
	size_t real_size = 0;
	GxBusPmViewInfo sys_info = {0};
	int32_t ret = 0;
	ret =  GX_PM_FS_EXIST  (GX_SYS_INFO_FILE_NAME);

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME, FILE_MODE);
	}
	else
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME,FILE_MODE);
	}
	if(gx_pm_view_info_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---GxBus_PmViewInfoGet open file err!\n");
#endif
		return GXCORE_ERROR;
	}

	real_size = GX_PM_FS_READ(gx_pm_view_info_file, sys, 1,sizeof(GxBusPmViewInfo));
	if(real_size != sizeof(GxBusPmViewInfo))//不存在系统信息,重建一个sys info
	{
		memset(&sys_info,0,sizeof(GxBusPmViewInfo));
		sys_info.status = VIEW_INFO_DISABLE;
		//sys_info.radio_prog_cur = 1;
		GX_PM_FS_WRITE(gx_pm_view_info_file, &sys_info, 1,sizeof(GxBusPmViewInfo));
		GX_PM_FS_SYNC(gx_pm_view_info_file);
		memcpy(sys,&sys_info,sizeof(GxBusPmViewInfo));
	}
	GX_PM_FS_CLOSE(gx_pm_view_info_file);
	return GXCORE_SUCCESS;
}

static status_t gx_pm_close_db(void)
{
	/*某个数据库文件关闭失败不能停止，应该继续关闭其他数据库文件*/
	if(dbp_sat != NULL)
	{
		dbp_sat->Close();
		dbp_sat->Delete(dbp_sat);
		dbp_sat = NULL;
	}
	if(dbp_tp != NULL)
	{
		dbp_tp->Close();
		dbp_tp->Delete(dbp_tp);
		dbp_tp = NULL;
	}
	if(dbp_prog != NULL)
	{
		dbp_prog->Close();
		dbp_prog->Delete(dbp_prog);
		dbp_prog = NULL;
	}
	if(dbp_fav != NULL)
	{
		dbp_fav->Close();
		dbp_fav->Delete(dbp_fav);
		dbp_fav = NULL;
	}
/*	berkeley_ret = dbp_stream->close(dbp_stream);
	if (berkeley_ret != 0) {
#ifdef GX_BUS_PM_DBUG
GX_BUS_PM_PRINTF("[PM]---close stream base err !!\n");
#endif
return GXCORE_ERROR;	//数据库关闭失败
}*/
if(dbp_prog_ext != NULL)
{
	dbp_prog_ext->Close();
	dbp_prog_ext->Delete(dbp_prog_ext);
	dbp_prog_ext = NULL;

}
return GXCORE_SUCCESS;
}

static status_t gx_pm_open_db(void)
{
	status_t ret = 0;
	/*检测数据库是否存在sat表 如果不存在 创建 */
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_SAT_FILE_PATH,
			&dbp_sat);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create sat dbase err !!\n");
#endif
		//gx_pm_verify_write(GX_PM_VERIFY_ERR);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_TP_FILE_PATH,
			&dbp_tp);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create tp dbase err !!\n");
#endif
		//gx_pm_verify_write(GX_PM_VERIFY_ERR);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_PROG_FILE_PATH,
			&dbp_prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create prog dbase err !!\n");
#endif
		//gx_pm_verify_write(GX_PM_VERIFY_ERR);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_FAV_FILE_PATH,
			&dbp_fav);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create fav dbase err !!\n");
#endif
		//gx_pm_verify_write(GX_PM_VERIFY_ERR);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*ret = gx_pm_dbase_open((uint8_t*)GX_PM_STREAM_FILE_PATH,sizeof(GxBusPmDataStream),&dbp_stream);
	  if(ret != GXCORE_SUCCESS)
	  {
#ifdef GX_BUS_PM_DBUG
GX_BUS_PM_PRINTF("[PM]---create stream dbase err !!\n");
#endif
return GXCORE_ERROR;	//创建sat dbase失败
}*/
return GXCORE_SUCCESS;
}

static status_t gx_pm_copy_default_file(uint8_t* src, uint8_t* target)
{
	handle_t src_file;
	handle_t target_file;
	int32_t size = 0;
	uint8_t* p = NULL;

	src_file = GxCore_Open((const char*)src,"r");
	if(src_file < 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open defaule db file err !!\n");
		PM_PRINTF("the err defaule db = %s\n",src);
#endif
		return GXCORE_ERROR;
	}
	target_file = GX_PM_FS_OPEN((const char*)target,FILE_MODE);
	if(target_file < 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open db file err !!\n");
		PM_PRINTF("the err db = %d\n",target_file);
#endif
		return GXCORE_ERROR;
	}
	p = (uint8_t*)GxCore_Malloc(64*1024);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---no memery !!\n");
#endif
		return GXCORE_ERROR;
	}
	do
	{
		size = GxCore_Read(src_file,p,1,64*1024);
		if(size == GXCORE_ERROR)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---read src file err !!\n");
#endif
			return GXCORE_ERROR;
		}
		if(size >0 )
		{
			GX_PM_FS_WRITE(target_file,p,1,size);
		}
	}while(size>0);
	GxCore_Close(src_file);
	GX_PM_FS_SYNC(target_file);
	GX_PM_FS_CLOSE(target_file);
	GxCore_Free(p);
	return GXCORE_SUCCESS;
}

uint32_t gx_pm_sat_num_get_realy(void)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;

	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		return 0;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		count++;
		cursor->Next();
	}
	delete cursor;
	return count;
}


static int32_t gx_pm_create_default_sat(handle_t file,gx_pm_tp_pos_by_sat* list)//写入卫星数据
{
	uint32_t count = 0;
	uint32_t tp_pos = 0;
	IDB *dbp = NULL;
	DefaultSat_t default_Sat ;
	GxBusPmDataSat* sat = NULL;
	uint32_t i = 0;
	ICursor *cursor = NULL;

	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		return -1;	//创建sat dbase失败
	}
	count = 0;
	tp_pos = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;


		sat = (GxBusPmDataSat*)cursor->GetRaw(&size);

		memcpy(default_Sat.m_chSatName,sat->sat_s.sat_name, 16);
		default_Sat.m_nTunerSelect = sat->tuner;
		default_Sat.m_nLongitude = sat->sat_s.longitude;//0-1800
		default_Sat.m_Lnb1 = default_Sat.m_Lnb2 = 0xf;
		for(i=0; i<sizeof(DefaultSatLnb)/sizeof(uint16_t); i++)
		{
			if(sat->sat_s.lnb1 == DefaultSatLnb[i])
			{
				default_Sat.m_Lnb1 = i;
			}
			else if(sat->sat_s.lnb2 == DefaultSatLnb[i])
			{
				default_Sat.m_Lnb2 = i;
			}

			if(default_Sat.m_Lnb1 != 0xf &&
					default_Sat.m_Lnb2 != 0xf)
			{
				break;
			}
		}
		default_Sat.m_nDiseqc12Pos = sat->sat_s.diseqc12_pos;//1-255
		default_Sat.m_nSat22k = sat->sat_s.switch_22K;
		default_Sat.m_nLongitudeDirection = sat->sat_s.longitude_direct;
		default_Sat.m_nDiseqc10 = sat->sat_s.diseqc10;
		default_Sat.m_nSat12v = sat->sat_s.switch_12V;
		if (sat->sat_s.diseqc_version == GXBUS_PM_SAT_DISEQC_1_0)
			default_Sat.m_nDiseqcMotor = 0;
		else if ((sat->sat_s.diseqc_version&GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
			default_Sat.m_nDiseqcMotor = 1;
		else if ((sat->sat_s.diseqc_version&GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
			default_Sat.m_nDiseqcMotor = 2;
		default_Sat.m_nTpNum = gx_pm_tp_num_get_by_sat(sat->id);
		default_Sat.m_nTpStartPos = tp_pos;
		list[count].sat_id = sat->id;//用于写入tp时查找写入的位置
		list[count].tp_pos = tp_pos;//用于写入tp时查找写入的位置
		tp_pos += default_Sat.m_nTpNum;//下一个卫星tp数据起始位置
		GxCore_Write(file,&default_Sat, 1,sizeof(DefaultSat_t));
		count ++;
		GxCore_Free(sat);
		cursor->Next();
	}
	delete cursor;
	return count;
}

static	int32_t  gx_pm_tp_num_get_realy(void)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return 0;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		count++;
		cursor->Next();
	}
	delete cursor;
	return count;

}
static int32_t gx_pm_create_default_tp(handle_t file, uint32_t start_pos, gx_pm_tp_pos_by_sat* sat_pos_list, gx_pm_tp_pos_by_id* tp_pos_list)//写入tp数据
{
	uint32_t count = 0;
	uint32_t sat_num = 0;
	uint32_t tp_pos = 0;
	IDB *dbp = NULL;
	DefaultTp_t default_tp ;
	GxBusPmDataTP* tp = NULL;
	uint32_t i = 0;
	ICursor *cursor = NULL;

	sat_num = gx_pm_sat_num_get_realy();
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		return -1;
	}
	count = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;

		tp = (GxBusPmDataTP*)cursor->GetRaw(&size);

		default_tp.m_nFrequency = tp->frequency;
		default_tp.m_nPolar = tp->tp_s.polar;
		default_tp.m_nSymbol = tp->tp_s.symbol_rate;
		for(i=0; i<sat_num; i++)
		{
			if(sat_pos_list[i].sat_id == tp->sat_id)
			{
				tp_pos = (sat_pos_list[i].tp_pos)*sizeof(DefaultTp_t) + start_pos;
				sat_pos_list[i].tp_pos ++;
				break;
			}
		}
		GxCore_Seek(file, tp_pos, (GxSeek)SEEK_SET);

		GxCore_Write(file,&default_tp, 1,sizeof(DefaultTp_t));
		/*为写入prog数据的tp pos做准备，下标代表该tp id在默认数据库中tp段的位置*/
		tp_pos_list[count].tp_id = tp->id;
		tp_pos_list[count].tp_pos = tp_pos;
		count ++;
		GxCore_Free(tp);
		cursor->Next();
	}
	delete cursor;
	qsort(tp_pos_list,count,sizeof(gx_pm_tp_pos_by_id),gx_pm_default_tp_cmp_pos);
	return count;
}
static int pos_cmp_(const void *a, const void *b)
{
	return *((uint32_t *)a) - *((uint32_t *)b);
}
static	int32_t  _prog_pos_get(uint32_t *pos, uint32_t *total)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;
	dbp = dbp_prog;
	GxBusPmDataProg* prog = NULL;
	if((dbp == NULL) || (pos == NULL) || (total == NULL) || (*total == 0))
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		return 0;
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		pos[count] = prog->pos;
		count++;
		GxCore_Free(prog);
		cursor->Next();
	}
	delete cursor;
	qsort(pos, count, sizeof(uint32_t), pos_cmp_);
	*total = count;
	return count;
}
static	uint32_t  gx_pm_prog_defpos_get(uint32_t progPos, uint32_t *pos, uint32_t total)
{
	uint32_t i  = 0;
	if ((pos == NULL) || (total == 0))
	{
		return -1;
	}
	for (i=0; i<total; i++)
	{
		if (pos[i] == progPos)
		{
			return i;
		}
	}
	return -1;
}
static void gx_set_pos_data(uint32_t pos_data)
{
	gx_pm_prog_pos_data = pos_data;
}
static uint32_t gx_get_pos_data(void)
{
	return gx_pm_prog_pos_data;
}
static int32_t gx_pm_create_default_prog(handle_t file, gx_pm_tp_pos_by_sat* sat_pos_list, gx_pm_tp_pos_by_id* tp_pos_list)//写入prog数据
{
	uint32_t def_pos = 0;
	uint32_t *prog_pos=NULL;
	uint32_t prog_num = MAX_PROG_COUNT;
	uint32_t start_pos = 0;
	uint32_t count = 0;
	uint32_t sat_num = 0;
	uint32_t tp_num = 0;
	IDB *dbp = NULL;
	DefaultProg_t default_prog ;
	GxBusPmDataProg* prog = NULL;
	uint32_t i = 0;
	ICursor *cursor = NULL;

	prog_pos = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*MAX_PROG_COUNT);
	if(prog_pos == NULL)
	{
		return -1;
	}
	sat_num = gx_pm_sat_num_get_realy();
	tp_num = gx_pm_tp_num_get_realy();
	if (0 == _prog_pos_get(prog_pos,&prog_num))
	{
		GxCore_Free(prog_pos);
		return -1;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GxCore_Free(prog_pos);
		return -1;
	}
	count = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;

		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);

		for(i=0; i<sat_num; i++)
		{
			if(sat_pos_list[i].sat_id == prog->sat_id)
			{
				//数组下标代表了默认数据库文件生成时sat的写入顺序
				default_prog.m_nSatId = i;
				break;
			}
		}
		for(i=0; i<tp_num; i++)
		{
			if(tp_pos_list[i].tp_id == prog->tp_id)
			{
				//数组下标代表了默认数据库文件生成时tp的写入顺序
				default_prog.m_nTpId = i;
				break;
			}
		}
		default_prog.m_nFavType = prog->favorite_flag;
		default_prog.m_nAudioFormat = prog->audio_mode;
		default_prog.m_nServiceId = prog->service_id;
		default_prog.m_nCasId = prog->cas_id;
		default_prog.m_nFlagAv = prog->service_type;
		default_prog.m_nFlagScramble = prog->scramble_flag;
		default_prog.m_nLock = prog->lock_flag;
		default_prog.m_nVideoPid = prog->video_pid;
		default_prog.m_nAudioPid = prog->cur_audio_pid;
		default_prog.m_nVideoType = prog->video_type;
		default_prog.m_nAudioType = prog->cur_audio_type;
		default_prog.m_nPmtPid = prog->pmt_pid;
		default_prog.m_nPcrPid = prog->pcr_pid;
		default_prog.m_nSkip = prog->skip_flag;
		default_prog.m_nTunerSelect = prog->tuner;
		memcpy(default_prog.m_chProgName,prog->prog_name, MAX_MCHPROG_NAME_LEN - 1);
		default_prog.m_chProgName[MAX_MCHPROG_NAME_LEN - 1] = 0;
		def_pos = gx_pm_prog_defpos_get(prog->pos,prog_pos,prog_num);
		start_pos = gx_get_pos_data();
		if (def_pos >= 0)
		{
			GxCore_Seek(file, start_pos+sizeof(DefaultProg_t)*def_pos, (GxSeek)SEEK_SET);
			GxCore_Write(file,&default_prog, 1,sizeof(DefaultProg_t));
			count ++;
		}

		GxCore_Free(prog);
		cursor->Next();
	}
	GxCore_Free(prog_pos);
	delete cursor;
	return count;
}
static int32_t gx_pm_default_tp_cmp_pos(const void *p1, const void *p2)

{
	return ((gx_pm_tp_pos_by_id*)p1)->tp_pos - ((gx_pm_tp_pos_by_id*)p2)->tp_pos;
}
/* Exported Functions ----------------------------------------------------- */

/**
 * @brief 数据库初始化,用于打开数据库,如果数据库不存在需要创建 并且初始化
 * @param void
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmDbaseInit(uint32_t max_sat,uint32_t max_tp,uint32_t max_prog,uint8_t* default_path)
{
	//DB *dbp = NULL;
	//DBT data, key;
	//int32_t berkeley_ret = 0;
	int32_t ret = 0;
	//	int8_t path[256];

	if(gx_pm_mutex_handle == 0)
	{
		GxCore_MutexCreate(&gx_pm_mutex_handle);
	}
	GX_PM_LOCK(gx_pm_mutex_handle);
	if(gx_pm_init_flag == 0)
	{
		gx_pm_init_flag = 1;
	}
	else
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm have initialized !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if(max_sat ==0
			||max_tp == 0
			||max_prog == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm  initialize params err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
reset_pm:
	MAX_SAT_COUNT = max_sat;
	MAX_TP_COUNT = max_tp;
	MAX_PROG_COUNT = max_prog;
	if(DEFAULT_PATH != NULL)
	{
		GxCore_Free(DEFAULT_PATH);
		DEFAULT_PATH = NULL;
	}
	if(default_path != NULL)
	{
		handle_t default_info_file = 0;
		uint32_t buf1 = 0;
		uint32_t pos = 0;
		if(GxCore_FileExists((const char *)default_path) == GXCORE_FILE_UNEXIST)
		{
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
		default_info_file = GxCore_Open((const char*)default_path, "r");
		if(default_info_file<0)
		{
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
		GxCore_ReadAt(default_info_file, (void*)&buf1, 4, 1,pos);
		if(buf1 != 0x4b386753)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]--defaule db file have broken!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			GxCore_Close(default_info_file);
			return GXCORE_ERROR;
		}

		DEFAULT_PATH = (uint8_t*)GxCore_Malloc(strlen((char*)default_path)+1);
		if(DEFAULT_PATH == NULL)
		{
			GX_BUS_PM_PRINTF("[PM]---pm load default err !!\n");
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
		memset(DEFAULT_PATH,0,strlen((char*)default_path)+1);
		strncpy((char*)DEFAULT_PATH ,(char*)default_path, strlen((char*)default_path));
	}
	ret = GX_PM_FS_EXIST(GX_PM_SAT_FILE_PATH);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
	}
	ret =  GX_PM_FS_EXIST  (GX_PM_VERIFY_FILE_PATH);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
	}
	if(gx_pm_verify_get() == GX_PM_VERIFY_ERR)
	{
		/*恢复出场设置*/\
			/*删除所有数据库文件*/
		gx_pm_close_db();//关闭打开的数据库文件
		GX_PM_FS_RM(GX_PM_SAT_FILE_PATH);
		GX_PM_FS_RM(GX_PM_TP_FILE_PATH);
		GX_PM_FS_RM(GX_PM_PROG_FILE_PATH);
		GX_PM_FS_RM(GX_PM_FAV_FILE_PATH);
		GX_PM_FS_RM(GX_PM_STREAM_FILE_PATH);
		GX_PM_FS_RM(GX_PM_PROG_EXT_FILE_PATH);
	}
	/*检测数据库是否存在sat表 如果不存在 创建 */
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_SAT_FILE_PATH,
			&dbp_sat);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create sat dbase err !!\n");
#endif
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
		//return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_TP_FILE_PATH,
			&dbp_tp);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create tp dbase err !!\n");
#endif
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
		//return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_PROG_FILE_PATH,
			&dbp_prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create prog dbase err !!\n");
#endif
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
		//return GXCORE_ERROR;	//创建sat dbase失败
	}
	ret = gx_pm_dbase_open((uint8_t*)GX_PM_FAV_FILE_PATH,
			&dbp_fav);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---create fav dbase err !!\n");
#endif
		gx_pm_verify_write(GX_PM_VERIFY_ERR);
		//return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*ret = gx_pm_dbase_open((uint8_t*)GX_PM_STREAM_FILE_PATH,sizeof(GxBusPmDataStream),&dbp_stream);
	  if(ret != GXCORE_SUCCESS)
	  {
#ifdef GX_BUS_PM_DBUG
GX_BUS_PM_PRINTF("[PM]---create stream dbase err !!\n");
#endif
return GXCORE_ERROR;	//创建sat dbase失败
}*/
/*初始化数据库的各个变量*/
if(NULL != gx_pm_sat_id)//程序运行途中调用init需要先释放再重新声请
{
	GxCore_Free(gx_pm_sat_id);
	gx_pm_sat_id = NULL;
}
gx_pm_sat_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*MAX_SAT_COUNT);
if(NULL == gx_pm_sat_id)
{
#ifdef GX_BUS_PM_DBUG
	GX_BUS_PM_PRINTF("[PM]---malloc gx_pm_sat_id err !!\n");
#endif
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_ERROR;
}
memset(gx_pm_sat_id,GX_PM_DBASE_ID_NOT_USE,sizeof(uint32_t)*MAX_SAT_COUNT);
gx_pm_max_sat_id = 0;
gx_pm_sat_count = 0;

if(NULL != gx_pm_tp_id)//程序运行途中调用init需要先释放再重新声请
{
	GxCore_Free(gx_pm_tp_id);
	gx_pm_tp_id = NULL;
}
gx_pm_tp_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*MAX_TP_COUNT);
if(NULL == gx_pm_sat_id)
{
#ifdef GX_BUS_PM_DBUG
	GX_BUS_PM_PRINTF("[PM]---malloc gx_pm_tp_id err !!\n");
#endif
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_ERROR;
}
memset(gx_pm_tp_id,GX_PM_DBASE_ID_NOT_USE,sizeof(uint32_t)*MAX_TP_COUNT);
gx_pm_max_tp_id = 0;
gx_pm_tp_count = 0;
gx_pm_tp_count_by_sat = 0;
if(NULL != gx_pm_tp_id_arry_by_sat)
{
	GxCore_Free(gx_pm_tp_id_arry_by_sat);
	gx_pm_tp_id_arry_by_sat = NULL;
}
if(NULL != gx_pm_prog_id)//程序运行途中调用init需要先释放再重新声请
{
	GxCore_Free(gx_pm_prog_id);
	gx_pm_prog_id = NULL;
}
gx_pm_prog_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*MAX_PROG_COUNT);
if(NULL == gx_pm_prog_id)
{
#ifdef GX_BUS_PM_DBUG
	GX_BUS_PM_PRINTF("[PM]---malloc gx_pm_prog_id err !!\n");
#endif
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_ERROR;
}
memset(gx_pm_prog_id,GX_PM_DBASE_ID_NOT_USE,sizeof(uint32_t)*MAX_PROG_COUNT);
gx_pm_max_prog_id = 0;
gx_pm_prog_count = 0;
gx_pm_max_stream_id = 0;

if(NULL != gx_pm_prog_id_arry)
{
	GxCore_Free(gx_pm_prog_id_arry);
	gx_pm_prog_id_arry = NULL;
}
gx_pm_prog_id_arry_num = 0;
gx_pm_prog_group_mode = GROUP_MODE_ALL;
gx_pm_prog_taxis_mode = TAXIS_MODE_NON;

if(gx_pm_verify_get() == GX_PM_VERIFY_ERR)
{
#ifdef GX_BUS_PM_DBUG
	GX_BUS_PM_PRINTF("[PM]---dbase have destroyed!\n");
#endif
	/*恢复出场设置*/
	/*恢复出场设置*/\
	/*删除所有数据库文件*/
	gx_pm_close_db();//关闭打开的数据库文件
	GX_PM_FS_RM(GX_PM_SAT_FILE_PATH);
	GX_PM_FS_RM(GX_PM_TP_FILE_PATH);
	GX_PM_FS_RM(GX_PM_PROG_FILE_PATH);
	GX_PM_FS_RM(GX_PM_FAV_FILE_PATH);
	GX_PM_FS_RM(GX_PM_STREAM_FILE_PATH);
	GX_PM_FS_RM(GX_SYS_INFO_FILE_NAME);//恢复出场设置，view info也没用了
	GX_PM_FS_RM(GX_PM_PROG_EXT_FILE_PATH);
	if (gx_pm_open_db() == GXCORE_ERROR) {
		GX_BUS_PM_PRINTF("[PM]gx pm open db error!\n");
		goto reset_pm;
	}
	if (gx_pm_load_default_entry() == GXCORE_ERROR) {
		GX_BUS_PM_PRINTF("[PM]gx pm open load default entry error!\n");
		goto reset_pm;
	}
	//gx_pm_load_default_data();
}


/*检测数据库是否存在sat表 tp表 prog表 stream表fav表 如果不存在 创建 */
ret = gx_pm_dbase_table_check();
if (ret != GXCORE_SUCCESS) {
#ifdef GX_BUS_PM_DBUG
	GX_BUS_PM_PRINTF("[PM]---dbase table check err !!\n");
#endif
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	goto reset_pm;	//创建表失败
}
GX_PM_UNLOCK(gx_pm_mutex_handle);

return GXCORE_SUCCESS;
}

/**
 * @brief 关闭数据库,下次再使用前必须再次调用初始化打开数据库
 * @param dbp:所要关闭的数据库的句柄,关闭的时候会把没有写入文件的修改自动写入
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmDbaseClose(void)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	if(gx_pm_init_flag == 1)
	{
		gx_pm_init_flag = 0;
	}
	else
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm have closed !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	if(dbp_sat != NULL)
	{
		dbp_sat->Close();
		dbp_sat->Delete(dbp_sat);
		dbp_sat = NULL;
	}

	if(dbp_tp != NULL)
	{
		dbp_tp->Close();
		dbp_tp->Delete(dbp_tp);
		dbp_tp = NULL;
	}

	if(dbp_prog != NULL)
	{
		dbp_prog->Close();
		dbp_prog->Delete(dbp_prog);
		dbp_prog = NULL;
	}

	if(dbp_fav != NULL)
	{
		dbp_fav->Close();
		dbp_fav->Delete(dbp_fav);
		dbp_fav = NULL;
	}
	/*	berkeley_ret = dbp_stream->close(dbp_stream);
		if (berkeley_ret != 0) {
#ifdef GX_BUS_PM_DBUG
GX_BUS_PM_PRINTF("[PM]---close stream base err !!\n");
#endif
return GXCORE_ERROR;	//数据库关闭失败
}*/
if(dbp_prog_ext != NULL)
{
	dbp_prog_ext->Close();
	dbp_prog_ext->Delete(dbp_prog_ext);
	dbp_prog_ext = NULL;
}
GX_PM_UNLOCK(gx_pm_mutex_handle);
return GXCORE_SUCCESS;
}

/**
 * @brief 	    备份数据库的sat tp prog veritfy 文件，用于做恢复
 * @param
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmDbaseBackup(void)
{
	handle_t file;
	status_t ret = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);

	/*backup sat file*/
	file = GX_PM_FS_OPEN(GX_PM_SAT_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_backup(file,
			&(gx_pm_file_backup_mem.sat_file),
			&(gx_pm_file_backup_mem.sat_file_size));
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	/*backup tp file*/
	file = GX_PM_FS_OPEN(GX_PM_TP_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_backup(file,
			&(gx_pm_file_backup_mem.tp_file),
			&(gx_pm_file_backup_mem.tp_file_size));
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	/*backup prog file*/
	file = GX_PM_FS_OPEN(GX_PM_PROG_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_backup(file,
			&(gx_pm_file_backup_mem.prog_file),
			&(gx_pm_file_backup_mem.prog_file_size));
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---backup dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}
/**
 * @brief 	    从GxBus_PmDbaseBackup备份的内存中恢复数据库
 * @param
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmDbaseComebackFromMem(void)
{
	handle_t file;
	status_t ret = 0;
	uint32_t max_sat = 0;
	uint32_t max_tp = 0;
	uint32_t max_prog = 0;

	if(gx_pm_file_backup_mem.sat_file == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---pm not need comeback !!\n");
#endif
		return GXCORE_ERROR;
	}
	max_sat = MAX_SAT_COUNT;
	max_tp = MAX_TP_COUNT;
	max_prog = MAX_PROG_COUNT;

	ret = GxBus_PmDbaseClose();
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm load default err !!\n");
#endif
		return GXCORE_ERROR;
	}
	/*删除要恢复的数据库文件*/
	GX_PM_FS_RM(GX_PM_SAT_FILE_PATH);
	GX_PM_FS_RM(GX_PM_TP_FILE_PATH);
	GX_PM_FS_RM(GX_PM_PROG_FILE_PATH);

	/*backup sat file*/
	file = GX_PM_FS_OPEN(GX_PM_SAT_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_comeback(file,
			&(gx_pm_file_backup_mem.sat_file),
			&(gx_pm_file_backup_mem.sat_file_size));
	GX_PM_FS_SYNC(file);
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}

	/*backup tp file*/
	file = GX_PM_FS_OPEN(GX_PM_TP_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_comeback(file,
			&(gx_pm_file_backup_mem.tp_file),
			&(gx_pm_file_backup_mem.tp_file_size));
	GX_PM_FS_SYNC(file);
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}
	/*backup prog file*/
	file = GX_PM_FS_OPEN(GX_PM_PROG_FILE_PATH, FILE_MODE);
	if(file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}
	ret = gx_pm_dbase_comeback(file,
			&(gx_pm_file_backup_mem.prog_file),
			&(gx_pm_file_backup_mem.prog_file_size));
	GX_PM_FS_SYNC(file);
	GX_PM_FS_CLOSE(file);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}

	ret = GxBus_PmDbaseInit(max_sat,max_tp,max_prog,NULL);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---comeback dbase err !!\n");
#endif
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}
/**
 * @brief 	    释放内存中的备份文件
 * @param
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmDbaseBackupDel(void)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	if(gx_pm_file_backup_mem.sat_file != NULL)
	{
		GxCore_Free(gx_pm_file_backup_mem.sat_file);
		gx_pm_file_backup_mem.sat_file = NULL;
		gx_pm_file_backup_mem.sat_file_size = 0;
	}
	if(gx_pm_file_backup_mem.tp_file != NULL)
	{
		GxCore_Free(gx_pm_file_backup_mem.tp_file);
		gx_pm_file_backup_mem.tp_file = NULL;
		gx_pm_file_backup_mem.tp_file_size = 0;
	}
	if(gx_pm_file_backup_mem.prog_file != NULL)
	{
		GxCore_Free(gx_pm_file_backup_mem.prog_file);
		gx_pm_file_backup_mem.prog_file = NULL;
		gx_pm_file_backup_mem.prog_file_size = 0;
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}
/**
 * @brief 获取卫星的数量
 * @param void
 * @return   	-1:发生错误
 正常数字:卫星数量
 */
int32_t GxBus_PmSatNumGet(void)
{
	return gx_pm_sat_count;
}

/**
 * @brief 实时从数据库获取卫星的数量,该接口没有开放出去先
 * @param void
 * @return
 */
uint32_t GxBus_PmSatNumGetRealy(void)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);

	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		count++;
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过位置获取卫星的信息
 * @param pos:所要获取信息的起始卫星的位置,注意 最小位置为0
Num:想要获取的卫星数量
sat_arry:获取的信息的指针,由该函数的使用者开辟空间
 * @return     	-1:发生错误
 正常数字:读取的真正的卫星数量
 */
int32_t GxBus_PmSatsGetByPos(uint32_t pos, uint32_t num, GxBusPmDataSat * sat_arry)
{
	uint32_t count = 0,i=0,size=0;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;
	GxBusPmDataSat *p;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sat_arry == NULL)
	{
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	if((int32_t)(num+pos)>gx_pm_sat_count)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get sat by pos params err !!\nnum=%d,pos=%d,count=%d\n",num,pos,gx_pm_sat_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	count = 0;
	i = 0;
	cursor = dbp->CreateCursor();
	for(i=0;i<num;i++)
	{
		cursor->First();
		while(!cursor->Eof())
		{
			p = (GxBusPmDataSat*)cursor->GetRaw(&size);
			if(p == NULL)
			{
#ifdef GX_BUS_PM_DBUG
				PM_PRINTF("[PM]---no sat exist!!\n");
#endif
				delete cursor;
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;	//
			}
			if(p->pos == pos+i)
			{
				memcpy(&sat_arry[count],p,sizeof(GxBusPmDataSat));
				GxCore_Free(p);
				count++;
				break;
			}
			GxCore_Free(p);
			cursor->Next();
		};
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过id获取卫星的信息
 * @param id:所要获取信息的卫星的id
sat:获取的信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmSatGetById(uint32_t id, GxBusPmDataSat * sat)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GxBusPmDataSat *p = NULL;
	uint32_t size = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sat == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = id;

	p = (GxBusPmDataSat *)dbp->Get(input_id,&size);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no sat exist id=%d!!\n",id);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//
	}
	memcpy(sat,p,sizeof(GxBusPmDataSat));
	GxCore_Free(p);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 编辑卫星参数,编辑前需要先获取原始参数 再进行修改 其中id号是
 不能改动的
 * @param sat:信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmSatModify(GxBusPmDataSat * sat)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sat == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp =dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);

	input_id = sat->id;
	if(input_id > MAX_SAT_COUNT || input_id == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---sat id is  err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if (dbp->Set(input_id,sat,sizeof(GxBusPmDataSat)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add sat err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除卫星,支持批量删除
 * @param id_arry:所要删除的卫星的id
num:所要删除的卫星数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmSatDelete(uint32_t * id_arry, uint32_t num)
{
	status_t ret = 0;
	IDB *dbp = NULL;


	GX_PM_LOCK(gx_pm_mutex_handle);
	if ((id_arry == NULL)||(num == 0)) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	if(num>(uint32_t)gx_pm_sat_count)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---del sat params err !!\nnum=%d,count=%d\n",num,gx_pm_sat_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	/*后续考虑采用事务控制 保证在出现错误的时候能够还原数据库 */
	ret = gx_pm_prog_delete_by_sat(id_arry, num);	//first删除卫星下的节目
	if (ret != GXCORE_SUCCESS) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//删除节目失败
	}
	ret = gx_pm_tp_delete_by_sat(id_arry, num);	//second删除卫星下的tp
	if (ret != GXCORE_SUCCESS) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//删除tp失败
	}
	ret = gx_pm_sat_delete(id_arry, num);	//third删除卫星
	if (ret != GXCORE_SUCCESS) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//删除卫星失败
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 添加卫星
 * @param sat:添加的卫星


 * @return     	GXCORE_SUCCESS:执行正常
GX_PM_DBASE_FULL:数据库已经满了
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmSatAdd(GxBusPmDataSat * sat)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i = 0,j=0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (sat == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---add sat  err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_sat;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open sat dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	if(gx_pm_sat_count== (int32_t)MAX_SAT_COUNT)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add sat err dbase full !!\n num = %d\n",gx_pm_sat_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GX_PM_DBASE_FULL;
	}
	if(gx_pm_max_sat_id == MAX_SAT_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_SAT_COUNT; i++)
		{
			if(gx_pm_sat_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}
	}
	else
	{
		start_id =gx_pm_max_sat_id+1;
		gx_pm_max_sat_id++;
		j = 1;//出错还原标记
	}
	sat->pos=gx_pm_sat_count;
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	sat->id = start_id;
	if (dbp->Set(start_id, sat, sizeof(GxBusPmDataSat)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add sat err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_sat_id--;
		}
		dbp->Sync();
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	gx_pm_sat_count++;
	gx_pm_sat_id[start_id-1] = GX_PM_DBASE_ID_USE;
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取tp的数量
 * @param void


 * @return     	-1:发生错误
 正常数字:卫星数量
 */
int32_t GxBus_PmTpNumGet(void)
{
	return gx_pm_tp_count;
}


/**
 * @brief 获取某个卫星下的tp的数量
 * @param sat_id:所要查找的卫星


 * @return     	-1:发生错误
 正常数字:tp数量
 */
int32_t GxBus_PmTpNumGetBySat(uint16_t sat_id)
{
	int32_t count = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	count = gx_pm_tp_num_get_by_sat(sat_id);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 实时从数据库获取tp的数量,该接口没有开放出去先
 * @param void
 * @return
 */
uint32_t GxBus_PmTpNumGetRealy(void)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	ICursor *cursor = NULL;

	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		count++;
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过位置获取tp的信息
 * @param pos:所要获取信息的起始tp的位置
Num:想要获取的tp数量
tp_arry:获取的信息的指针tp_arry:获取的信息的指针,由该函数的使用者开辟空间
 * @return     	-1:发生错误
 正常数字:读取的真正的tp数量
 */
int32_t GxBus_PmTpGetByPos(uint32_t pos, uint32_t num, GxBusPmDataTP * tp_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	ICursor *cursor = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (tp_arry == NULL)
	{
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	if((int32_t)(num+pos) > gx_pm_tp_count)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get tp by pos params err !!\nnum=%d,pos=%d,count=%d\n",num,pos,gx_pm_tp_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	count = 0;
	i = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataTP *p = NULL;
		if(count == num)//找到了所有信息
		{
			break;
		}
		p = (GxBusPmDataTP *)cursor->GetRaw(&size);
		if((pos == i)&&(count == 0))//找到了第一个信息
		{
			memcpy(&tp_arry[count],p,sizeof(GxBusPmDataTP));
			count++;
			i++;
		}
		else if(count>0)
		{
			memcpy(&tp_arry[count],p,sizeof(GxBusPmDataTP));
			count++;
		}
		else if(i<pos)//还没有找到第一个信息
		{
			i++;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过位置获取tp的信息,和GxBus_PmTpNumGetBySat配合使用
 * @param pos:所要获取信息的起始tp的位置
Num:想要获取的tp数量
tp_arry:获取的信息的指针tp_arry:获取的信息的指针,由该函数的使用者开辟空间
 * @return     	-1:发生错误
 正常数字:读取的真正的tp数量
 */
int32_t GxBus_PmTpGetByPosInSat(uint32_t pos, uint32_t num, GxBusPmDataTP * tp_arry)
{
	uint32_t count = 0;
	uint32_t input_id = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (tp_arry == NULL)
	{
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	if((num>(uint32_t)gx_pm_tp_count_by_sat)||(pos>((uint32_t)gx_pm_tp_count_by_sat-1)))
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get tp by pos params err !!\nnum=%d,pos=%d,count=%d\n",num,pos,gx_pm_tp_count_by_sat);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	count = 0;
	for(i=0; i<num; i++)
	{
		GxBusPmDataTP *p = NULL;
		uint32_t size = 0;
		input_id = gx_pm_tp_id_arry_by_sat[pos+i];

		p = (GxBusPmDataTP *)dbp->Get(input_id,&size);
		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get tp by id err !!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return -1;
		}
		memcpy(&tp_arry[count],p,sizeof(GxBusPmDataTP));
		count++;
		GxCore_Free(p);
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过id获取tp的信息
 * @param id:所要获取信息的tp的id
tp:获取的信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTpGetById(uint32_t id, GxBusPmDataTP * tp)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GxBusPmDataTP *p = NULL;
	uint32_t size = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (tp == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = id;

	p = (GxBusPmDataTP *)dbp->Get(input_id, &size);
	if(p == 0)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no tp exist id=%d!!\n",id);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//
	}
	memcpy(tp,p,sizeof(GxBusPmDataTP));
	GxCore_Free(p);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 编辑tp参数,编辑前需要先获取原始参数 再进行修改 其中id号是
 不能改动的
 * @param tp:信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTpModify(GxBusPmDataTP * tp)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (tp == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	input_id = tp->id;
	if(input_id > MAX_TP_COUNT || input_id == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---tp id is  err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if (dbp->Set(input_id, tp, sizeof(GxBusPmDataTP)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add tp err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除tp,支持批量删除
 * @param id_arry:所要删除的tp的id
num:所要删除的tp数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTpDelete(uint32_t * id_arry, uint32_t num)
{
	status_t ret = 0;
	IDB *dbp = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);

	if ((id_arry == NULL)||(num == 0)) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	if(num>(uint32_t)gx_pm_tp_count)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---del tp  params err !!\nnum=%d,count=%d\n",num,gx_pm_tp_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	/*后续考虑采用事务控制 保证在出现错误的时候能够还原数据库 */
	ret = gx_pm_prog_delete_by_tp(id_arry, num);	//first删除tp下的节目
	if (ret != GXCORE_SUCCESS) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//删除节目失败
	}
	ret = gx_pm_tp_delete(id_arry, num);	//second删除tp
	if (ret != GXCORE_SUCCESS) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//删除tp失败
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 检测tp是否存在 sat 缺少极化方向
 * @param sat_id:所要检测的sat
fre:所要检测的频率
symbol:所要检测的符号率,如果是t的方案该值传bandwidth
polar:检测的极化方向 只在卫星方案的tp检测中才有效
modulation:检测调制方式，只有在c方案的tp检测才有效
tp_id:如果存在相同tp会返回最后一个相同的tp的id，不存在则为0

 * @return     	0:该tp不存在
 >0:该tp已经存在
 -1:失败
 */
int32_t GxBus_PmTpExistChek(uint16_t sat_id,
		uint32_t fre,
		uint32_t symbol,
		GxBusPmTpPolar polar,
		fe_modulation_t modulation,
		uint16_t* tp_id)
{
	int32_t count = 0;
	IDB *dbp = NULL;
	uint32_t fre_temp = 0;
	uint32_t symbol_temp = 0;
	uint16_t sat_id_temp = 0;
	GxBusPmTpPolar polar_temp = GXBUS_PM_TP_POLAR_H;
	fe_modulation_t modulation_temp = (fe_modulation_t)0;
	GxBusPmDataSat sat = {0};
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_tp;
	if(dbp == NULL
			||tp_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建tp dbase失败
	}
	*tp_id = 0;
	gx_pm_sat_get_by_id(sat_id,&sat);
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataTP *p = NULL;

		p = (GxBusPmDataTP *)cursor->GetRaw(&size);
		sat_id_temp = p->sat_id;
		fre_temp = p->frequency;
		if(sat.type == GXBUS_PM_SAT_S)
		{
			symbol_temp = p->tp_s.symbol_rate;
			polar_temp = p->tp_s.polar;
			modulation_temp = modulation;
#if 0
			if((fre_temp-3) < fre
					&&(fre_temp+3)>fre
					&&(symbol_temp-20)<symbol
					&&(symbol_temp+20)>symbol)//fre和symbol的判断是有个范围的
#else
				if(sat_id_temp !=sat_id)
				{
					GxCore_Free(p);
					cursor->Next();
					continue;
				}
           //Hardware engineers say the following algorithms are used
			uint8_t FreInter = 5;
			if(symbol > 20000)
				FreInter = (symbol+2000)/4000;
			else if(symbol < 10000)
				FreInter = (symbol+1000)/2000;
			if((fre_temp-FreInter) <= fre
					&&(fre_temp+FreInter)>=fre
					&&(symbol_temp-100)<symbol
					&&(symbol_temp+100)>symbol)//fre和symbol的判断是有个范围的
#endif
			{
				fre_temp = fre;
				symbol_temp = symbol;
			}
		}
		else if(sat.type == GXBUS_PM_SAT_C)
		{
			polar_temp = polar;
			/*if symbol == 0 mean dvbc symbolrate & modulation auto detect.no need to check*/
			if(symbol == 0)
			{
				symbol_temp = symbol;
				modulation_temp = modulation;
			}
			else
			{
				symbol_temp = p->tp_c.symbol_rate;
				modulation_temp = p->tp_c.modulation;
			}
			if((fre_temp-3) < fre
					&&(fre_temp+3)>fre)
			{
				fre_temp = fre;
			}
		}
		else if(sat.type == GXBUS_PM_SAT_T||
				sat.type == GXBUS_PM_SAT_DVBT2)
		{
			symbol_temp = p->tp_dvbt2.bandwidth;
			polar_temp = polar;
			modulation_temp = modulation;
		}
		else if(sat.type == GXBUS_PM_SAT_DTMB)
		{
			symbol_temp = symbol;
			polar_temp = polar;
			modulation_temp = modulation;
		}
		else if(sat.type == GXBUS_PM_SAT_ATSC_C)
		{
			;//等待demod参数 ATSC wait
		}
		else if(sat.type == GXBUS_PM_SAT_ATSC_T)
		{
			;//等待demod参数 ATSC wait
		}
		if((sat_id_temp==sat_id)&&
				(fre_temp==fre)&&
				(symbol_temp == symbol)&&
				(polar_temp == polar)&&
				(modulation_temp == modulation))
		{
			count++;
			*tp_id = p->id;
		}

		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

status_t GxBus_PmNetUrlLoad(void)
{
	return GxNetUrlManageInit();
}

status_t GxBus_PmGetNetServerUrlByIndex(uint32_t *index_arry, char **service_url_arry, uint32_t num)
{
	if(index_arry == NULL || service_url_arry == NULL || num <= 0)
	{
		return GXCORE_ERROR;
	}
	return GxNetGetUrlByIndex(SERVER_URL, index_arry, service_url_arry, num);
}

status_t GxBus_PmGetNetStreamUrlByIndex(uint32_t *index_arry, char **stream_url_arry, uint32_t num)
{
	if(index_arry == NULL || stream_url_arry == NULL || num <= 0)
	{
		return GXCORE_ERROR;
	}
	return GxNetGetUrlByIndex(STREAM_URL, index_arry, stream_url_arry, num);
}

status_t GxBus_PmAddNetServerUrlToFile(char* server_url, uint32_t *index)
{
	if(server_url == NULL || index == NULL)
		return GXCORE_ERROR;
	return GxNetAddUrlToFile(SERVER_URL, server_url, index);
}

status_t GxBus_PmAddNetStreamUrlToFile(char *stream_url, uint32_t *index)
{
	if(stream_url == NULL || index == NULL)
		return GXCORE_ERROR;
	return GxNetAddUrlToFile(STREAM_URL, stream_url, index);
}

status_t GxBus_PmDeleteNetServerUrlByIndex(uint32_t *server_index_arry, uint32_t delete_num)
{
	if(server_index_arry == NULL || delete_num <= 0)
	{
		return GXCORE_ERROR;
	}
	return GxNetDeleteUrlByIndex(SERVER_URL, server_index_arry, delete_num);
}

status_t GxBus_PmDeleteNetStreamUrlByIndex(uint32_t *stream_index_arry, uint32_t delete_num)
{
	if(stream_index_arry == NULL || delete_num <= 0)
	{
		return GXCORE_ERROR;
	}
	return GxNetDeleteUrlByIndex(STREAM_URL, stream_index_arry, delete_num);
}

status_t GxBus_PmModifyNetServerUrlByIndex(uint32_t index, char *server_url)
{
	if(server_url == NULL)
	{
		return GXCORE_ERROR;
	}
	return GxNetModifyUrlByIndex(SERVER_URL, index, server_url);
}

status_t GxBus_PmModifyNetStreamUrlByIndex(uint32_t index, char *stream_url)
{
	if(stream_url == NULL)
	{
		return GXCORE_ERROR;
	}
	return GxNetModifyUrlByIndex(STREAM_URL, index, stream_url);
}

status_t GxBus_PmGetNetServerStreamUrlByTpId(uint32_t tp_id, char *server_stream_url)
{
	GxBusPmDataTP tpdata = {0};
	GxBusPmDataSat satdata = {0};
	char *certifi_info = NULL;
	char *temp_url = NULL;
	char *temp_net_url = NULL;
	char *url_head = NULL;
	status_t ret = GXCORE_ERROR;

	if(server_stream_url == NULL)
		ret = GXCORE_ERROR;
	certifi_info = (char*)GxCore_Calloc(PASSWORD_LEN + USER_NAME_LEN, sizeof(char));
	temp_net_url = (char*)GxCore_Calloc(GX_MAX_SERVER_LEN, sizeof(char));
	if(certifi_info == NULL || temp_net_url == NULL)
	{
		ret = GXCORE_ERROR;
		goto error;
	}
	if(GXCORE_ERROR ==  gx_pm_tp_get_by_id(tp_id,&tpdata))
	{
		ret = GXCORE_ERROR;
		goto error;
	}
	if(GXCORE_ERROR ==  gx_pm_sat_get_by_id(tpdata.sat_id, &satdata))
	{
		ret = GXCORE_ERROR;
		goto error;
	}
	temp_url = server_stream_url;
	if(strlen((char*)satdata.sat_net.user_name) > 0 && strlen((char*)satdata.sat_net.password) > 0)
	{
		strcat(certifi_info, (char*)satdata.sat_net.user_name);
		strcat(certifi_info, ":");
		strcat(certifi_info, (char*)satdata.sat_net.password);
		strcat(certifi_info, "@");
		GxNetGetUrlByIndex(SERVER_URL, &(satdata.sat_net.index), &temp_net_url, 1);
		url_head = strchr(temp_net_url,':');
		url_head = url_head + 3;
		snprintf(temp_url, ((url_head - temp_net_url)/sizeof(char)) + 1, "%s", temp_net_url);
		strcat(temp_url, certifi_info);
		strcat(temp_url,url_head);
	}
	else
	{
		GxNetGetUrlByIndex(SERVER_URL, &(satdata.sat_net.index), &temp_url, 1);
	}
	temp_url = temp_url + strlen(temp_url);
	GxNetGetUrlByIndex(STREAM_URL, &(tpdata.tp_net.index), &temp_url, 1);
	gxlogd("pm net url:%s\n", server_stream_url);
	ret = GXCORE_SUCCESS;

error:
	if(certifi_info)
	{
		GxCore_Free(certifi_info);
		certifi_info = NULL;
	}
	if(temp_net_url)
	{
		GxCore_Free(temp_net_url);
		temp_net_url = NULL;
	}
	return ret;
}

int32_t GxBus_PmTpExistCheckNet(uint16_t sat_id,
		uint32_t pos,
		uint32_t port,
		uint16_t* tp_id)
{
	int32_t count = 0;
	IDB *dbp = NULL;
	uint32_t pos_temp = 0;
	uint32_t port_temp = 0;
	uint16_t sat_id_temp = 0;
	GxBusPmDataSat sat = {0};
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_tp;
	if(dbp == NULL
			||tp_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建tp dbase失败
	}
	*tp_id = 0;
	gx_pm_sat_get_by_id(sat_id,&sat);
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataTP *p = NULL;

		p = (GxBusPmDataTP *)cursor->GetRaw(&size);
		sat_id_temp = p->sat_id;
		pos_temp = p->pos;
		if(sat.type == GXBUS_PM_SAT_NET)
		{
			port_temp = p->tp_net.port;

		}
		if((sat_id_temp == sat_id) &&
				(pos_temp==pos) &&
				(port_temp == port))
		{
			count++;
			*tp_id = p->id;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}


/**
 * @brief 检测tp是否存在 sat 缺少极化方向
 * @param sat_id:所要检测的sat
 fre1,fre2:所要检测的频率 当fre1<=...<=fre2时 认为是相同频点
 symbol1,symbol2:所要检测的符号率,如果是c和t的方案该值可以随意传 symbol1<=...<=symbol2
polar:检测的极化方向 只在卫星方案的tp检测中才有效
modulation:检测调制方式，只有在c方案的tp检测才有效
tp_id:如果存在相同tp会返回最后一个相同的tp的id，不存在则为0

 * @return     	0:该tp不存在
 >0:该tp已经存在
 -1:失败
 */
int32_t GxBus_PmTpExistChekBound(uint16_t sat_id,
		uint32_t fre1,
		uint32_t fre2,
		uint32_t symbol1,
		uint32_t symbol2,
		GxBusPmTpPolar polar,
		fe_modulation_t modulation,
		uint16_t* tp_id)
{
	int32_t count = 0;
	IDB *dbp = NULL;
	uint32_t fre_temp = 0;
	uint32_t symbol_temp = 0;
	uint16_t sat_id_temp = 0;
	GxBusPmTpPolar polar_temp = GXBUS_PM_TP_POLAR_H;
	fe_modulation_t modulation_temp = (fe_modulation_t)0;
	GxBusPmDataSat sat = {0};
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_tp;
	if(dbp == NULL
			||tp_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建tp dbase失败
	}
	*tp_id = 0;
	gx_pm_sat_get_by_id(sat_id,&sat);
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataTP *p = NULL;

		 p = (GxBusPmDataTP *)cursor->GetRaw(&size);
		sat_id_temp = p->sat_id;
		fre_temp = p->frequency;
		if(sat.type == GXBUS_PM_SAT_S)
		{
			symbol_temp = p->tp_s.symbol_rate;
			polar_temp = p->tp_s.polar;
			modulation_temp = modulation;
			if((fre_temp) <= fre2
					&&(fre_temp)>=fre1
					&&(symbol_temp)<=symbol2
					&&(symbol_temp)>=symbol1)//fre和symbol的判断是有个范围的
			{
				fre_temp = fre1;
				symbol_temp = symbol1;
			}
		}
		else if(sat.type == GXBUS_PM_SAT_C)
		{
			symbol_temp = symbol1;
			polar_temp = polar;
			modulation_temp = p->tp_c.modulation;
			if((fre_temp) <= fre2
					&&(fre_temp)>=fre1)
			{
				fre_temp = fre1;
			}
		}
		else if(sat.type == GXBUS_PM_SAT_T||
				sat.type == GXBUS_PM_SAT_DVBT2)
		{
			symbol_temp = symbol1;
			polar_temp = polar;
			modulation_temp = modulation;
			if((fre_temp) <= fre2
					&&(fre_temp)>=fre1)
			{
				fre_temp = fre1;
			}
		}
		else if(sat.type == GXBUS_PM_SAT_DTMB)
		{
			if(sat.sat_dtmb.work_mode == GXBUS_PM_SAT_1501_DTMB)
			{
				symbol_temp = symbol1;
				polar_temp = polar;
				modulation_temp = modulation;
				if((fre_temp) <= fre2
						&&(fre_temp)>=fre1)
				{
					fre_temp = fre1;
				}
			}
			else if(sat.sat_dtmb.work_mode == GXBUS_PM_SAT_1501_DVB_C)
			{
				symbol_temp = symbol1;
				polar_temp = polar;
				modulation_temp = p->tp_dtmb.modulation;
				if((fre_temp) <= fre2
						&&(fre_temp)>=fre1)
				{
					fre_temp = fre1;
				}
			}
		}
		else if(sat.type == GXBUS_PM_SAT_ATSC_C)
		{
			;//等待demod参数 ATSC wait
		}
		else if(sat.type == GXBUS_PM_SAT_ATSC_T)
		{
			;//等待demod参数 ATSC wait
		}
		if((sat_id_temp==sat_id)&&
				(fre_temp==fre1)&&
				(symbol_temp == symbol1)&&
				(polar_temp == polar)&&
				(modulation_temp == modulation))
		{
			count++;
			*tp_id = p->id;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 添加tp
 * @param tp:添加的tp


 * @return     	GXCORE_SUCCESS:执行正常
GX_PM_DBASE_FULL:数据库满
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTpAdd(GxBusPmDataTP * tp)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (tp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---add tp  err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_tp;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open tp dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	if(gx_pm_tp_count== (int32_t)MAX_TP_COUNT)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add tp err dbase full !!\n num = %d\n",gx_pm_tp_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GX_PM_DBASE_FULL;
	}
	if(gx_pm_max_tp_id == MAX_TP_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_TP_COUNT; i++)
		{
			if(gx_pm_tp_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}
	}
	else
	{
		start_id =gx_pm_max_tp_id+1;
		gx_pm_max_tp_id++;
		j = 1;//出错还原标记
	}
	tp->pos=gx_pm_tp_count;
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	tp->id = start_id;
	if (dbp->Set(start_id, tp, sizeof(GxBusPmDataTP)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add tp err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_tp_id--;
		}
		dbp->Sync();
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	gx_pm_tp_count++;
	gx_pm_tp_id[start_id-1] = GX_PM_DBASE_ID_USE;

	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取prog的数量 使用此接口时如果更改了观看模式
 应该先更新系统信息里的view_info
 再调用此接口获取新的观看模式下的节目数量
 * @param void


 * @return     	-1:发生错误
 正常数字:prog数量
 */
int GxBus_PmProgNumGet(void)
{
	int32_t count = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	count = gx_pm_prog_id_arry_num;//gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过位置获取prog的信息 使用此接口时如果更改了观看模式
 应该先更新系统信息里的view_info
 再调用此节口更新节目列表,并且注意调用此节口前一定要调用GxBus_PmProgNumGet
 确认当前节目的数量,除非保证没有删减增加过节目 那么是不需要重复调用GxBus_PmProgNumGet的
 * @param pos:所要获取信息的起始prog的位置
Num:想要获取的prog数量
prog_arry:获取的信息的指针prog_arry:获取的信息的指针,由该函数的使用者开辟空间
 * @return     	-1:发生错误
 正常数字:读取的真正的prog数量
 */
int32_t GxBus_PmProgGetByPos(uint32_t pos, uint32_t num, GxBusPmDataProg * prog_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (prog_arry == NULL)
	{
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_prog;
	if(dbp ==NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}
	/*prog的数量是当前观看模式下的数量*/
	if(num+pos > gx_pm_prog_id_arry_num)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get prog by pos params err !!\nnum=%d,pos=%d,count=%d\n",num,pos,gx_pm_prog_id_arry_num);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	count = 0;
	for(i=0; i<num; i++)
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		input_id = ((GxBusPmProgSort*)(gx_pm_prog_id_arry))[pos+i].id;
		p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog by  err !!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return -1;
		}
		memcpy(&prog_arry[count],p,sizeof(GxBusPmDataProg));
		count++;
		GxCore_Free(p);
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过id获取prog的信息
 * @param id:所要获取信息的prog的id
prog:获取的信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgGetById(uint32_t id, GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	uint32_t  size = 0;
	GxBusPmDataProg *p = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (prog == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = id;

	p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no prog exist id=%d!!\n",id);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//
	}
	memcpy(prog,p,sizeof(GxBusPmDataProg));
	GxCore_Free(p);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取指定的prog
 * @param uint32_t ts_id:和service_id确定一个service 不关心传入0xffffffff
 uint32_t service_id:和ts_id确定一个service 不关心传入0xffffffff
original_id:确定一个service 不关心传入0xffffffff
prog::获取的信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
 */
int32_t GxBus_PmProgGetBySpecialId(uint32_t ts_id,uint32_t service_id, uint32_t original_id, GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	status_t ret = 0;
	uint32_t ts_id_temp = ts_id;
	uint32_t service_id_temp = service_id;
	uint32_t original_id_temp = original_id;
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		if(ts_id !=0xffffffff)
		{
			ts_id_temp = p->ts_id;
		}
		if(service_id != 0xffffffff)
		{
			service_id_temp = p->service_id;
		}
		if(original_id != 0xffffffff)
		{
			original_id_temp = p->original_id;
		}
		if((ts_id==ts_id_temp)&&(service_id==service_id_temp)&&(original_id == original_id_temp))
		{
			memcpy(prog, p, sizeof(GxBusPmDataProg));
			GxCore_Free(p);
			delete cursor;
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_SUCCESS;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_ERROR;
}
/**
 * @brief 通过id获取prog的url信息,该信息主要是为了给player播放用的,prog_url的
 空间是由使用接口的地方分配的 大小为 GX_PM_MAX_PROG_URL_SIZE
 * @param prog:所要获取url信息的prog
prog_url:获取的url信息的buffer
uint32_t size:buffer的大小 如果小于GX_PM_MAX_PROG_URL_SIZE会返回错误


 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgUrlGet(GxBusPmDataProg* prog, int8_t* prog_url,uint32_t size)
{
	uint32_t tp_id = 0;
	uint32_t sat_id = 0;
	//	int32_t stream_count = 0;
	//	uint32_t i = 0;
	//	GxBusPmDataStream * stream_arry = NULL;
	//	GxBusPmDataStream stream = {0};
	uint32_t fre = 0;
	fe_sec_voltage_t polar = (fe_sec_voltage_t)0;
	status_t ret = GXCORE_SUCCESS;
	GxBusPmDataTP tp = {0};
	GxBusPmDataSat sat = {0};
	fe_sec_tone_mode_t sat22k = (fe_sec_tone_mode_t)0;
	//	CHIP_WORK_MODE_t   work_mode = 0;
	uint32_t   work_mode = 0;
	uint16_t audio_pid = 0;
	GxBusPmDataProgAudioType audio_type = GXBUS_PM_AUDIO_MPEG1;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if(prog == NULL||prog_url == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---params err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if(size<GX_PM_MAX_PROG_URL_SIZE)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---buffer size err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	tp_id = prog->tp_id;
	sat_id = prog->sat_id;

	gx_pm_tp_get_by_id(tp_id,&tp);
	gx_pm_sat_get_by_id(sat_id,&sat);
#if 0
	stream_count = gx_pm_stream_num_get(prog->id);
	if(stream_count != -1)
	{
		stream_arry = (GxBusPmDataStream *)GxCore_Malloc(sizeof(GxBusPmDataStream) * stream_count);
		gx_pm_stream_get(prog->id,stream_count,stream_arry);
		for(i=0; i<(uint32_t)stream_count; i++)
		{
			if((stream_arry[i].audio_type == 0)
					||stream_arry[i].audio_type == 2)//pcm
			{
				stream.audio_pid = stream_arry[i].audio_pid;
				stream.audio_type = stream_arry[i].audio_type;
				break;
			}
		}
		if(i == (uint32_t)stream_count)//没有pcm
		{
			stream.audio_pid = 0x1fff;
		}
		GxCore_Free(stream_arry);
	}
	else
	{
		stream.audio_pid = 0x1fff;
	}
#endif
	if ((prog->video_pid!=0)&&(prog->cur_audio_pid==0)&&(prog->ac3_pid!=0))
	{
		audio_pid = prog->ac3_pid;
		audio_type = GXBUS_PM_AUDIO_AC3;
	}
	else
	{
		audio_pid = prog->cur_audio_pid;
		audio_type = prog->cur_audio_type;
	}

	switch(sat.type)
	{
	case GXBUS_PM_SAT_S:
		gx_pm_prog_url_fre_get(&tp,&sat,&fre);
		gx_pm_prog_url_polar_get(&tp,&sat,&polar);
		gx_pm_prog_url_22k_get(&tp,&sat,&sat22k);
		sprintf((char *)prog_url,"dvbs://fre:%d&polar:%d&symbol:%d&22k:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d",
				fre,
				polar,
				tp.tp_s.symbol_rate,
				sat22k,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid);
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---prog_url = %s!!\n",prog_url);
#endif
		break;

	case GXBUS_PM_SAT_C:
		sprintf((char *)prog_url,"dvbc://fre:%d&qam:%d&symbol:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d",
				tp.frequency,
				tp.tp_c.modulation,
				tp.tp_c.symbol_rate,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid);
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---prog_url = %s!!\n",prog_url);
#endif
		break;

	case GXBUS_PM_SAT_T:
		sprintf((char *)prog_url,"dvbt://fre:%d&bandwidth:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d",
				tp.frequency,
				tp.tp_t.bandwidth,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid);
		break;

	case GXBUS_PM_SAT_NET:
		sprintf((char *)prog_url,"dvbnet://&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d",
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid);
		break;

	case GXBUS_PM_SAT_DTMB:
		if(GXBUS_PM_SAT_1501_DTMB == sat.sat_dtmb.work_mode)
		{
			work_mode = 0;//DTMB;
		}
		else if(GXBUS_PM_SAT_1501_DVB_C == sat.sat_dtmb.work_mode)
		{
			work_mode = 1;//DVB_C;
		}
		else
		{
			work_mode = 0;//DTMB;
		}
		sprintf((char *)prog_url,"dtmb://fre:%d&bandwidth:%d&qam:%d&symbol:%d&workmode:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d",
				tp.frequency,
				tp.tp_dtmb.bandwidth,
				tp.tp_dtmb.modulation,
				tp.tp_dtmb.symbol_rate,
				work_mode,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid);
		break;
	case GXBUS_PM_SAT_DVBT2:
		sprintf((char *)prog_url,"dvbt2://fre:%d&bandwidth:%d&workmode:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d&data_plp_id:%d&common_plp_exist:%d&common_plp_id:%d",
				tp.frequency,
				tp.tp_t.bandwidth,
				prog->common_status,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid,
				prog->data_plp_id,
				prog->common_plp_exist,
				prog->common_plp_id);
		break;
	case GXBUS_PM_SAT_ATSC_C:
		/*ATSC wait*/
#if 0
		sprintf((char *)prog_url,"atsc_c://fre:%d&bandwidth:%d&workmode:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d&data_plp_id:%d&common_plp_exist:%d&common_plp_id:%d",
				tp.frequency,
				tp.tp_t.bandwidth,
				prog->common_status,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid,
				prog->data_plp_id,
				prog->common_plp_exist,
				prog->common_plp_id);
#endif
		break;
	case GXBUS_PM_SAT_ATSC_T:
		/*ATSC wait*/
#if 0
		sprintf((char *)prog_url,"atsc_c://fre:%d&bandwidth:%d&workmode:%d&vpid:%d&apid:%d&pcrpid:%d&vcodec:%d&acodec:%d&tuner:%d&scramble:%d&pmt:%d&data_plp_id:%d&common_plp_exist:%d&common_plp_id:%d",
				tp.frequency,
				tp.tp_t.bandwidth,
				prog->common_status,
				prog->video_pid,
				audio_pid,
				prog->pcr_pid,
				prog->video_type,
				audio_type,
				prog->tuner,
				prog->scramble_flag,
				prog->pmt_pid,
				prog->data_plp_id,
				prog->common_plp_exist,
				prog->common_plp_id);
#endif
		break;

	default:
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---sat type err!!\n");
#endif
		ret = GXCORE_ERROR;
		break;
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return ret;
}


/**
 * @brief 通过id修改节目信息中以bool形式表示的信息,比如skip:0-跳过 1-不跳过
 * @param type:修改的内容
id_arry:所要修改的id
num:所要修改的节目的数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgBoolModify(GxBusPmProgModifyBoolType type, uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GxBusPmDataProg prog = {0};
	uint32_t i = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (id_arry == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	if(num==0)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---modify prog bool params err !!\nnum=%d\n",num);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	switch (type) {
	case GXBUS_PM_PROG_MODIFY_SKIP:

		for (i = 0; i < num; i++)
		{
			uint32_t size = 0;
			GxBusPmDataProg *p =NULL;
			/*获取原是数据*/
			input_id = id_arry[i];

			p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
			if(p == NULL)
			{
#ifdef GX_BUS_PM_DBUG
				PM_PRINTF("[PM]---no prog exist id=%d!!\n",input_id);
#endif
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;	//
			}
			memcpy(&prog,p,sizeof(GxBusPmDataProg));
			/*获取修改值 */
			prog.skip_flag = (GxBusPmDataProgSwitch)(!(prog.skip_flag));	//dbResult[0]是字段名 dbResult[1]是字段值
			if (dbp->Set(input_id, &prog, sizeof(GxBusPmDataProg)) != true)
			{
#ifdef GX_BUS_PM_DBUG
				PM_PRINTF("[PM]---modify prog err !!\n");
#endif
				dbp->Sync();
				GxCore_Free(p);
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;
			}
			GxCore_Free(p);
		}
		break;
	case GXBUS_PM_PROG_MODIFY_LOCK:
		for (i = 0; i < num; i++)
		{
			uint32_t size = 0;
			GxBusPmDataProg *p =NULL;
			/*获取原是数据*/
			input_id = id_arry[i];

			p = (GxBusPmDataProg *)dbp->Get(input_id, &size);
			if(p == NULL)
			{
#ifdef GX_BUS_PM_DBUG
				PM_PRINTF("[PM]---no prog exist id=%d!!\n",input_id);
#endif
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;	//
			}
			memcpy(&prog,p,sizeof(GxBusPmDataProg));
			/*获取修改值 */
			prog.lock_flag = (GxBusPmDataProgSwitch)(!(prog.lock_flag));	//dbResult[0]是字段名 dbResult[1]是字段值
			if (dbp->Set(input_id, &prog, sizeof(GxBusPmDataProg)) != true)
			{
#ifdef GX_BUS_PM_DBUG
				PM_PRINTF("[PM]---modify prog err !!\n");
#endif
				dbp->Sync();
				GxCore_Free(p);
				GX_PM_UNLOCK(gx_pm_mutex_handle);
				return GXCORE_ERROR;
			}
			GxCore_Free(p);
		}
		break;
	default:
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get prog bool modify type err !!\n type = %d\n",type);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
		break;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去

	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 修改节目的喜爱组信息 一次一个喜爱组
 * @param fav_id:所要修改的喜爱组
id_arry:所要修改的节目的id
Num:所要修改的节目的数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgFavModify(uint32_t fav_id, uint32_t * id_arry, uint32_t num)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GxBusPmDataProg prog = {0};
	uint32_t i = 0;
	uint32_t flag = 0;
	uint32_t modify_flag = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (id_arry == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if(num==0)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---modify prog bool params err !!\nnum=%d\n",num);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for (i = 0; i < num; i++) {
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		/*获取原是数据*/
		input_id = id_arry[i];

		p = (GxBusPmDataProg*)dbp->Get(input_id, &size);
		if(p == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---no prog exist id=%d!!\n",input_id);
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//
		}
		memcpy(&prog,p,sizeof(GxBusPmDataProg));

		/*获取修改值 */
		flag = prog.favorite_flag;
		modify_flag = flag;
		if (flag &= (1 << (fav_id - 1)))
		{
			modify_flag &= ~(1 << (fav_id - 1));
		}

		else
		{
			modify_flag |= (1 << (fav_id - 1));
		}
		prog.favorite_flag = modify_flag;
		if (dbp->Set(input_id, &prog, sizeof(GxBusPmDataProg)) != true)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---modify prog err !!\n");
#endif
			dbp->Sync();
			GxCore_Free(p);
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
		GxCore_Free(p);
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	//gx_pm_prog_num_get();;//这时候编辑了节目的状态，需要重建节目列表了
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 编辑prog参数,编辑前需要先获取原始参数 再进行修改 其中id号是
 不能改动的
 * @param prog:信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgInfoModify(GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (prog == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	input_id = prog->id;
	if(input_id == 0 || input_id > MAX_PROG_COUNT)//id 的最大值就等于MAX_PROG_COUNT
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---nodify prog dbase err id is err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;

	}
	if (dbp->Set(input_id,prog,sizeof(GxBusPmDataProg)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---modify prog err !!\n");
#endif
		dbp->Sync();
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除prog,支持批量删除
 * @param id_arry:所要删除的prog的id
num:所要删除的prog数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgDelete(uint32_t * id_arry, uint32_t num)
{
	IDB *dbp_prog_temp = NULL;
	//	DB *dbp_stream_temp = NULL;
	uint32_t i = 0;
	uint32_t input_id = 0;
	//	uint32_t* stream_id_arry = NULL;
	//	uint32_t count = 0;
	//	uint32_t j = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if (id_arry == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	/*prog的数量是当前观看模式下的数量*/
	if(num == 0)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---del prog  params err !!\nnum=%d,count=%d\n",num,gx_pm_prog_id_arry_num);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp_prog_temp = dbp_prog;
	if(dbp_prog_temp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	/*删除节目 */
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	for(i=0; i<num; i++)
	{
		input_id = id_arry[i];
		if (dbp_prog_temp->Remove(input_id) != true)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del prog err !!\n");
#endif
			dbp_prog_temp->Sync();
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		gx_pm_prog_id[input_id-1] = GX_PM_DBASE_ID_NOT_USE;
		gx_pm_prog_count--;
		/*删除扩展信息*/
		gx_pm_prog_ext_del(input_id);
	}
	if(dbp_prog_ext != NULL)
	{
		dbp_prog_ext->Sync();
	}

#if 0//现阶段不适用strem
	/*先申请100个*/
	stream_id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t)*GX_PM_DBASE_MALLOC_PER_TIMR);
	if(stream_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get stream malloc err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	dbp_stream_temp = dbp_stream;
	if(dbp_stream_temp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	memset(&data,0,sizeof(DBT));
	memset(&key,0,sizeof(DBT));
	/*找到需要删除的stream*/
	if((dbp_stream_temp->seq(dbp_stream_temp,&key, &data, R_FIRST))==0)
	{
		for(j=0; j<num; j++)
		{
			if(((GxBusPmDataStream*)(data.data))->prog_id == id_arry[j])
			{
				stream_id_arry[count] = *(uint32_t*)(key.data);
				count++;
				i++;
				break;
			}
		}

	}
	while((dbp_stream_temp->seq(dbp_stream_temp,&key, &data, R_NEXT))==0)
	{
		for(j=0; j<num; j++)
		{
			if(((GxBusPmDataStream*)(data.data))->prog_id == id_arry[j])
			{
				stream_id_arry[count] = *(uint32_t*)(key.data);
				count++;
				i++;
				/*100个空间用完了 在追加100个*/
				if(i == GX_PM_DBASE_MALLOC_PER_TIMR)
				{
					gxlogd("[PM]---start GxCore_Realloc!!\n");
					stream_id_arry = (uint32_t*)GxCore_Realloc(stream_id_arry,sizeof(uint32_t)*GX_PM_DBASE_MALLOC_PER_TIMR);
					if(stream_id_arry == NULL)
					{
#ifdef GX_BUS_PM_DBUG
						GX_BUS_PM_PRINTF("[PM]---get stream GxCore_Realloc err !!\n");
#endif
						GX_PM_UNLOCK(gx_pm_mutex_handle);
						return GXCORE_ERROR;	//创建sat dbase失败
					}
					i = 0;
				}
				break;
			}
		}

	}
	/*删除找到的stream*/
	for(i=0; i<count; i++)
	{
		input_id = stream_id_arry[i];
		memset(&key,0,sizeof(DBT));
		key.data = &input_id;
		key.size = sizeof(uint32_t);
		berkeley_ret = dbp_stream_temp->del(dbp_stream_temp,&key,0);
		if(berkeley_ret != 0)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---del stream err !!\n");
#endif
			dbp_stream_temp->sync(dbp_stream_temp,0);
			GxCore_Free(stream_id_arry);
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
	}
	GxCore_Free(stream_id_arry);
	dbp_stream_temp->sync(dbp_stream_temp,0);
#endif
	//dbp_prog_temp->sync(dbp_prog_temp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	//gx_pm_prog_num_get();;//这时候编辑了节目的状态，需要重建节目列表了
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 添加prog
 * @param prog:添加的prog


 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
GX_PM_DBASE_FULL:数据库已经满了
*/
status_t GxBus_PmProgAdd(GxBusPmDataProg * prog)
{
	IDB *dbp = NULL;
	static uint32_t start_id = 0;
	uint32_t i = 0;
	uint32_t j = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (prog == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---add prog  err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	if(gx_pm_prog_count== (int32_t)MAX_PROG_COUNT)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add prog err dbase full !!\n num = %d\n",gx_pm_prog_count);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GX_PM_DBASE_FULL;
	}
	if(gx_pm_max_prog_id == MAX_PROG_COUNT)//id已经分配的最大值了 这时候需要从id数组中搜索一个没有使用的id
	{
		for(i=0; i<MAX_PROG_COUNT; i++)
		{
			if(gx_pm_prog_id[i] == GX_PM_DBASE_ID_NOT_USE)
			{
				start_id = i+1;
				break;
			}
		}

	}
	else
	{
		start_id =gx_pm_max_prog_id+1;
		gx_pm_max_prog_id++;
		j = 1;//出错还原标记
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	prog->id = start_id;
	if(prog->pos == 0)
	{
		prog->pos = start_id;//起始位置和id是一样的,这里有个问题,如果是通过查找未使用id的方式获得的id,那么起始位置就不是在最后!!!!!!
	}
	if (dbp->Set(start_id, prog, sizeof(GxBusPmDataProg)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add prog err !!\n");
#endif
		if(j==1)//出错 还原gx_pm_max_sat_id
		{
			gx_pm_max_prog_id--;
		}
		dbp->Sync();
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	gx_pm_prog_count++;
	gx_pm_prog_id[start_id-1] = GX_PM_DBASE_ID_USE;

	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 移动节目 注意 target_id不能出现在id_arry中
 * @param id_arry:所要移动的节目id
Num:所要移动的节目数量
target_id:目标id,比如 次参数为5 代表要把选中的节目移动到节目id为5的节目前

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgMove(uint32_t * id_arry,
		uint32_t num,
		uint32_t target_id,
		GxBusPmProgMoveType type)
{
	uint32_t max_pos;	//所要修改的最大位置 用于和min一起确定修改的区间
	uint32_t min_pos;	//所要修改的最小位置 用于和max一起确定修改的区间
	uint32_t *pos_arry = NULL;	//由以上区间确定的位置 从小到大排列
	uint32_t modify_count;	//所要修改的节目的数量
	uint32_t target_id_pos;	//目标id的pos,用于建立before_target和after_target
	uint32_t *real_id_arry = NULL;	//重建好后的id数组 顺序和real_pos_arry是一一对应的,用于更新表
	uint32_t *real_pos_arry = NULL;	//重建好后的pos数组 顺序和real_id_arry是一一对应的,用于更新表
	uint32_t *before_target_id = NULL;	//目标id之前的id的数组
	uint32_t *after_target_id = NULL;	//目标id之后的id的数组
	uint32_t before_count = 0;	//目标id之前的id数量
	uint32_t after_count = 0;	//目标id之后的id数量
	uint32_t i = 0;
	uint32_t j = 0;
	uint32_t k = 0;
	uint32_t p = 0;
	uint32_t q = 0;
	uint32_t id_pos = 0;	//real_id_arry数组的下标
	status_t ret = 0;
	IDB *dbp = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (id_arry == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if(num>gx_pm_prog_id_arry_num)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---move prog err!!\nnum=%d\ncount=%d\n",num,gx_pm_prog_id_arry_num);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	/*目标id是不能被移动的 */
	for (i = 0; i < num; i++) {
		if (id_arry[i] == target_id) {
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---move prog err!! can't move the target id\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
	}
	dbp = dbp_prog;
	if(dbp ==  NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}


	/*找到最大的pos和最小的pos,作为要修改的区间 */
	ret= gx_pm_prog_max_min_pos_get(id_arry, num, target_id, &max_pos, &min_pos,dbp);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog get max and min pos err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*获取要修改的区间内的记录数量 */
	modify_count = gx_pm_prog_modify_count_get(max_pos, min_pos,dbp);
	pos_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * modify_count);
	if(pos_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	/*通过上面的pos区间获取这个区间内的pos的值 按从小到大排列 并且获得目标id的pos */
	gx_pm_prog_pos_init(pos_arry, max_pos, min_pos, target_id, &target_id_pos,dbp);

	/*通过最大最小pos和目标id的pos 建立before_target_id 不包括所要移动的id 记录下数量before_count */
	/*通过最大最小pos和目标id的pos  建立after_target_id  不包括所要移动的id 记录下数量after_count */
	before_count = modify_count - num - 1;//不包括要修改的和目标id 因为这三个id 首先分配了1 2 3 的位置 等待其他id插入
	if (before_count != 0) {
		before_target_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * before_count);
		if(before_target_id == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
			GxCore_Free(pos_arry);
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		after_target_id = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * before_count);
		if(after_target_id == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
			GxCore_Free(pos_arry);
			GxCore_Free(before_target_id);
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//创建sat dbase失败
		}
		ret= gx_pm_prog_before_after_buff_init(before_target_id, after_target_id, &before_count, &after_count,
				max_pos, min_pos, target_id_pos, id_arry, num,dbp);
		if(ret != GXCORE_SUCCESS)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---move prog init before and after buff err !!\n");
#endif
			GxCore_Free(pos_arry);
			GxCore_Free(before_target_id);
			GxCore_Free(after_target_id);
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;	//创建sat dbase失败
		}

	}

	else {
		before_count = 0;
		after_count = 0;
	}
	real_id_arry =(uint32_t*)GxCore_Malloc(sizeof(uint32_t) * modify_count);
	if(real_id_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		GxCore_Free(pos_arry);
		GxCore_Free(before_target_id);
		GxCore_Free(after_target_id);
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	real_pos_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * modify_count);
	if(real_pos_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---move prog malloc err !!\n");
#endif
		GxCore_Free(pos_arry);
		GxCore_Free(before_target_id);
		GxCore_Free(after_target_id);
		GxCore_Free(real_id_arry);
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}

	/*写入real_id_arry 前num个成员和目标id,也就是传下来的id 记录下下标 */
	switch(type)
	{
	case GXBUS_PM_PROG_MOVE_BEFORE:
		for (i = 0; i < num; i++)
		{
			real_id_arry[id_pos] = id_arry[i];
			id_pos++;
		}
		real_id_arry[id_pos] = target_id;
		id_pos++;
		break;

	case GXBUS_PM_PROG_MOVE_AFTER:
		real_id_arry[id_pos] = target_id;
		id_pos++;
		for (i = 0; i < num; i++)
		{
			real_id_arry[id_pos] = id_arry[i];
			id_pos++;
		}
		break;

	default:
		for (i = 0; i < num; i++)
		{
			real_id_arry[id_pos] = id_arry[i];
			id_pos++;
		}
		real_id_arry[id_pos] = target_id;
		id_pos++;
		break;
	}


	/*写入real_pos_arry 前num个成员 也就是pos的前num个值 */
	for (i = 0; i < id_pos; i++) {
		real_pos_arry[i] = pos_arry[i];
	}

	/*往real_id_arry 从后往前写入before_target的成员 数量是before_count 每写入一个成员
	  新的成员real_pos_arry就是pos[0],而之前的成员的real_pos_arry为pos[i] */
	p = id_pos;
	for (i = 0; i < before_count; i++) {
		real_id_arry[id_pos] = before_target_id[before_count - i - 1];

		//real_pos_arry[id_pos] =  pos_arry[0];
		k++;

		/*更新刚写入新的id的pos */
		for (q = 0; q < k; q++) {
			real_pos_arry[id_pos - q] = pos_arry[q];
		}

		/*更新之前写入的id的pos */
		for (j = 0; j < p; j++) {
			real_pos_arry[j] = pos_arry[j + k];
		}
		id_pos++;
	}

	/*往real_id_arry 从前往后写入after_target的成员 数量是after_count 每写入一个成员
	  新的成员real_pos_arry就是pos[num],num是上一个步骤后pos的下标 */
	for (i = 0; i < after_count; i++) {
		real_id_arry[id_pos] = after_target_id[i];
		real_pos_arry[id_pos] = pos_arry[id_pos];
		id_pos++;
	}
	gx_pm_prog_pos_update(real_id_arry, real_pos_arry, id_pos,dbp);
	GxCore_Free(pos_arry);
	GxCore_Free(before_target_id);
	GxCore_Free(after_target_id);
	GxCore_Free(real_id_arry);
	GxCore_Free(real_pos_arry);
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	//gx_pm_prog_num_get();//这时候编辑了节目的状态，需要重建节目列表了
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 另外一种特殊的移动节目，需要由调用的人组织好当前观看模式下的所有节目的顺序，
 如，当前观看模式下由10个台，那么初始顺序肯定是0~9，如果要把最后一个
 台移动到最前面，那么pos_arry={9,0,1,2,3,4,5,6,7,8};其他移动情况，依次类推
 * @param pos_arry:排列好的顺序，最小值为0最大值肯定为num－1
Num:所要移动的节目数量，肯定等于当前模式下的节目数量

 * @return     	gxcore_success:执行正常
gxcore_error:执行失败
*/
status_t GxBus_PmProgMoveSpecial(uint32_t * pos_arry, uint32_t num)
{
	//status_t ret = 0;
	IDB *dbp = NULL;
	uint32_t* original_pos_arry = NULL;
	uint32_t* id_arry = NULL;

	if(NULL == pos_arry || num != gx_pm_prog_id_arry_num)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---special move params err !!\n");
#endif
		return GXCORE_ERROR;
	}

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(dbp ==  NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	//获取当前模式下所有节目的pos，按照节目顺序排好
	original_pos_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * num);
	if(NULL == original_pos_arry)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc original_pos_arry err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	gx_pm_prog_original_pos_get(original_pos_arry);

	//获取排列好顺序的id
	id_arry = (uint32_t*)GxCore_Malloc(sizeof(uint32_t) * num);
	if(NULL == id_arry)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---malloc id_arry err !!\n");
#endif
		GxCore_Free(original_pos_arry);
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	gx_pm_prog_id_get_by_pos(pos_arry,id_arry);

	//进行最后的保存
	gx_pm_prog_pos_update(id_arry, original_pos_arry, num,dbp);

	GxCore_Free(original_pos_arry);
	GxCore_Free(id_arry);
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	//gx_pm_prog_num_get();//这时候编辑了节目的状态，需要重建节目列表了
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;

}

/**
 * @brief 监测节目时否存在,比如用于搜台时候的保存
 * @param tp_id:所要检测的tp_id
ts_id:所要检测的ts id
service_id:所要检测的service id
original_id:所要检测的原始网络id
tuner:所要检测的tuner号
uint8_t data_plp_id: dvbt2节目独有，不用请传入0xff
uint8_t common_plp_exist: dvbt2节目独有，不用请传入0xff
uint8_t common_plp_id: dvbt2节目独有，不用请传入0xff

 * @return     	0:该节目不存在
 >1:该节目已经存在
 -1:失败
 */
//增加一个tp id 再增加原始网络id 导致搜台时需要增加存储参数
int32_t GxBus_PmProgExistChek(uint16_t tp_id,
		uint16_t ts_id,
		uint16_t service_id,
		uint16_t original_id,
		uint16_t tuner,
		uint8_t data_plp_id,
		uint8_t common_plp_exist,
		uint8_t common_plp_id)
{
	int32_t count = 0;
	IDB *dbp = NULL;
	uint16_t tp_id_temp = 0;
	//uint16_t original_id_temp = 0;
	//uint16_t ts_id_temp = 0;
	uint16_t service_id_temp = 0;
	uint16_t tuner_temp = 0;
	uint8_t xindao_mode = 0;
	uint8_t xindao_mode_tmp = 0;
	ICursor *cursor = NULL;
	uint8_t data_plp_id_tmp = data_plp_id;
	uint8_t common_plp_exist_tmp = common_plp_exist;
	uint8_t common_plp_id_tmp = common_plp_id;
	GxFrontendInfo info;
	int frontend = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	frontend = GxFrontend_IdToHandle(tuner);
	GxFrontend_GetInfo(frontend, &info);
	if (FE_DVB_T == info.type)
	{
		xindao_mode = 1;
	}
	else if(FE_DVB_T2 == info.type)
	{
		xindao_mode = 2;
	}
	else
	{
		xindao_mode = 0xff;
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg*)cursor->GetRaw(&size);

		tp_id_temp = p->tp_id;
		//	original_id_temp = ((GxBusPmDataProg*)(data.data))->original_id;
		//	ts_id_temp = ((GxBusPmDataProg*)(data.data))->ts_id;
		service_id_temp = p->service_id;
		tuner_temp = p->tuner;

		if((xindao_mode == 1)||(xindao_mode == 2))
		{
			xindao_mode_tmp = p->common_status;
		}
		else
		{
		    xindao_mode_tmp = 0xff;
		}

		if(data_plp_id != 0xff)
		{
			data_plp_id_tmp = p->data_plp_id;
		}
		if(common_plp_exist != 0xff)
		{
			common_plp_exist_tmp = p->common_plp_exist;
		}
		if(common_plp_id != 0xff)
		{
			common_plp_id_tmp = p->common_plp_id;
		}
		if((tp_id == tp_id_temp)&&
				//(ts_id==ts_id_temp)&&//zhouzhh modify for default db dont have this ts_id
				(service_id==service_id_temp)&&
				//(original_id == original_id_temp)&&
				(tuner == tuner_temp)&&
				(xindao_mode == xindao_mode_tmp)&&
				(data_plp_id == data_plp_id_tmp)&&
				(common_plp_exist == common_plp_exist_tmp)&&
				(common_plp_id == common_plp_id_tmp))
		{
			count = p->id;
			GxCore_Free(p);
			delete cursor;
			goto finish;
			//	count++;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
finish:
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 获取指定的prog的pos,主要用于epg删除service时的判断
 * @param uint32_t ts_id:和service_id确定一个service
 uint32_t service_id:和ts_id确定一个service

 * @return     	-1:获取失败
 正常值:节目的pos
 */
int32_t GxBus_PmProgPosGet(uint32_t ts_id,uint32_t service_id)
{
	int32_t pos = -1;
	IDB *dbp = NULL;
	status_t ret = 0;
	uint16_t ts_id_temp = 0;
	uint16_t service_id_temp = 0;
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		ts_id_temp = p->ts_id;
		service_id_temp = p->service_id;
		if((ts_id==ts_id_temp)&&(service_id==service_id_temp))
		{
			pos = p->pos;
			GxCore_Free(p);
			break;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return pos;
}

/**
 * @brief 获取指定的prog的pos,主要用于epg删除service时的判断
 * @param uint32_t service_id:确定一个service

 * @return     	-1:获取失败
 正常值:节目的pos
 */
int32_t GxBus_PmProgPosGetByServiceId(uint32_t service_id)
{
	int32_t pos = -1;
	IDB *dbp = NULL;
	status_t ret = 0;
	uint16_t service_id_temp = 0;
	ICursor *cursor = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataProg *p = NULL;

		p = (GxBusPmDataProg *)cursor->GetRaw(&size);
		service_id_temp = p->service_id;
		if(service_id==service_id_temp)
		{
			pos = p->pos;
			GxCore_Free(p);
			break;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return pos;
}
/**
 * @brief 根据view info重新建立节目列表一般是不需要调用的，因为GxBus_PmSync
 和GxBus_PmViewInfoModify都会重建节目列表，除非特殊需要
 * @param

 * @return     	-1:获取失败
 正常值:节目的pos
 */
int32_t GxBus_PmProgListRebulid(void)
{
	int32_t ret = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	ret = gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return ret;
}

/**
 * @brief 保存自定义节目表，一旦保存了自定义节目表，那么view_info不再起
 *              作用，节目的顺序将按照自定义节目表排列。除非调用
 *              GxBus_PmProgUserListDel.移除掉自定义列表，那么节目列表又将
 *              恢复由view info控制的状态。所以要注意，在改变节目数量前最好先移
 *              除掉自定义节目表，再合适的时机重新save自定义表，不然节目会混乱。
 * @param uint32_t* id_arry：存储节目id号的空间，这些id号是用户排好序的。
 *              uint32_t num：id_arry中有几个id

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgUserListSave(uint32_t* id_arry,uint32_t num)
{
	handle_t file = 0;
	uint32_t i = 0;
	uint8_t*  file_path;
	GxBusPmViewInfo sys = {0};

	if(id_arry == NULL || num == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---save prog user list  para err !!\n");
#endif
		return GXCORE_ERROR;
	}
	GX_PM_LOCK(gx_pm_mutex_handle);
	gx_pm_view_info_get(&sys);
	if(sys.stream_type == GXBUS_PM_PROG_TV)
	{
		file_path = (uint8_t*)GX_PM_TV_USER_LIST_FILE_PATH;
	}
	else
	{
		file_path = (uint8_t*)GX_PM_RADIO_USER_LIST_FILE_PATH;
	}
	GX_PM_FS_RM((const char*)file_path);
	file = GX_PM_FS_OPEN((const char*)file_path, FILE_MODE);
	if(file<0)
	{

#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---save prog user list open file err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	GX_PM_FS_WRITE(file,&num,1,sizeof(uint32_t));
	i = GX_PM_FS_WRITE(file,id_arry,1,sizeof(uint32_t)*num);
	if(i != sizeof(uint32_t)*num)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---save prog user list write file err !!\n");
#endif
		GX_PM_FS_SYNC(file);
		GX_PM_FS_CLOSE(file);
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;

	}
	GX_PM_FS_SYNC(file);
	GX_PM_FS_CLOSE(file);
	gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 删除自定义节目表，一旦保存了自定义节目表，那么view_info不再起
 *              作用，节目的顺序将按照自定义节目表排列。除非调用
 *              GxBus_PmProgUserListDel.移除掉自定义列表，那么节目列表又将
 *              恢复由view info控制的状态。所以要注意，在改变节目数量前最好先移
 *              除掉自定义节目表，再合适的时机重新save自定义表，不然节目会混乱。
 *              del自定义节目列表的时候会根据最近一次保存的view info 重建节目
 *              列表
 * @param
 *
 * @return     	GXCORE_SUCCESS:执行正常
 */
status_t GxBus_PmProgUserListDel(void)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	GX_PM_FS_RM(GX_PM_TV_USER_LIST_FILE_PATH);
	GX_PM_FS_RM(GX_PM_RADIO_USER_LIST_FILE_PATH);
	gx_pm_prog_num_get();
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;

}

/**
 * @brief    通过id获取节目在索引表中的位置
 * @param id：每个节目唯一
 *
 * @return     -1:索引表中不存在该节目
 *             其他值：节目在索引表中的位置哦，该值可以用做GxBus_PmProgGetByPos的参数
 */
int32_t GxBus_PmProgGetIndexPosById(uint32_t id)
{
	int32_t pos = -1;
	unsigned int i=0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	for(i=0; i<gx_pm_prog_id_arry_num; i++)
	{
		if(((GxBusPmProgSort*)(gx_pm_prog_id_arry))[i].id == id)
		{
			pos= i;
			break;
		}

	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return pos;
}

/**
 * @brief 获取节目的stream记录数量
 * @param prog_id:节目的id

 * @return     	-1:发生错误
 正常数字:stream 记录数
 */
int32_t GxBus_PmStreamNumGet(uint32_t prog_id)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	ICursor *cursor = NULL;
	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataStream *p = NULL;

		p = (GxBusPmDataStream *)cursor->GetRaw(&size);
		if(p->prog_id == prog_id)
		{
			count++;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 获取节目的stream记录 调用此接口之前必须调用
 int32_t GxBus_PmStreamNumGet(uint32_t prog_id);先获取数量
 * @param prog_id:节目的id

 * @return     	-1:发生错误
 正常数字:获取的stream 记录数
 */
int32_t GxBus_PmStreamGet(uint32_t prog_id, uint32_t num, GxBusPmDataStream * stream_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	ICursor *cursor = NULL;
	if(stream_arry==NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get stream params err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}
	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataStream *p = NULL;

		p = (GxBusPmDataStream*)cursor->GetRaw(&size);
		if(p->prog_id == prog_id)
		{
			memcpy(&stream_arry[count],p,sizeof(GxBusPmDataStream));
			count++;
			if(count == num)
			{
				GxCore_Free(p);
				break;
			}
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 添加节目的stream记录,函数内部会先把之前的prog_id的记录全部删除
 * @param prog_id:节目的id
num:stream信息的条数
stream_arry:stream信息


 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmStreamAdd(uint32_t prog_id, uint32_t num, GxBusPmDataStream * stream_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	//	uint32_t j = 0;
	//	uint32_t input_id = 0;
	uint32_t start_id = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if(stream_arry == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get stream params err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	dbp = dbp_stream;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open stream dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	/*向数据库添加传入的stream*/
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	gx_pm_stream_del(prog_id);
	for(i=0; i<num; i++)
	{

		stream_arry[i].prog_id = prog_id;
		start_id = ++gx_pm_max_stream_id;//最大id号 先加1 再作当前的id号
		if (dbp->Set(start_id,&stream_arry[i],sizeof(GxBusPmDataStream)) != true)
		{
#ifdef GX_BUS_PM_DBUG
			PM_PRINTF("[PM]---add stream err !!\n");
#endif
			gx_pm_max_stream_id--;
			dbp->Sync();
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);//放到同步里面去
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 获取cas的数量
 * @param void

 * @return     	-1:发生错误
 正常数字:cas 数
 */
int32_t GxBus_PmCasNumGet(void)
{
	return MAX_CA_SYSTEM;
}


/**
 * @brief 通过位置获取cas信息
 * @param pos:位置
Cas:cas信息

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmCasGetByPos(uint32_t pos, GxBusPmDataCasInfot * cas)
{
	if (pos >= MAX_CA_SYSTEM) {
		return GXCORE_ERROR;
	}
	if(cas == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---case get by pos !!\n");
#endif
		return GXCORE_ERROR;
	}
	memcpy(cas, &DefaultCaSystem[pos], sizeof(GxBusPmDataCasInfot));
	return GXCORE_SUCCESS;
}


/**
 * @brief 获取节目的cas,首先得从节目中获取cas_id
 * @param cas_id:cas的id
cas:case信息

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmCasGetById(uint32_t cas_id, GxBusPmDataCasInfot * cas)
{
	uint32_t i = 0;
	if(cas == NULL)
	{
		return GXCORE_ERROR;
	}
	for (i = 0; i < MAX_CA_SYSTEM; i++) {
		if ((DefaultCaSystem[i].cas_id_begin <= cas_id) &&( DefaultCaSystem[i].cas_id_end >= cas_id)) {
			memcpy(cas, &DefaultCaSystem[i], sizeof(GxBusPmDataCasInfot));
			break;
		}
	}
	if (i == MAX_CA_SYSTEM) {
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}


/**
 * @brief 获取喜爱组的数量
 * @param void

 * @return     -1:发生错误
 正常数字:获取的fav group 记录数
 */
int GxBus_PmFavGroupNumGet(void)
{
	return 32;
}


/**
 * @brief 通过位置获取fav group信息
 * @param pos:位置
num:所要获取的数量
fav_arry:cas信息

 * @return     	-1:失败
 正常数字:获取的真是数量
 */
int32_t GxBus_PmFavGroupGetByPos(uint32_t pos, uint32_t num, GxBusPmDataFavItem * fav_arry)
{
	uint32_t count = 0;
	IDB *dbp = NULL;
	uint32_t i = 0;
	ICursor *cursor = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (fav_arry == NULL)
	{
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_fav;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open fav dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;	//创建sat dbase失败
	}

	if(num+pos >32)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---get fav by pos params err !!\nnum=%d,pos=%d,count=%d\n",num,pos,32);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	count = 0;
	i = 0;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		GxBusPmDataFavItem *p = NULL;
		if(count == num)//找到了所有信息
		{
			break;
		}

		p = (GxBusPmDataFavItem *)cursor->GetRaw(&size);
		if((pos == i)&&(count == 0))//找到了第一个信息
		{
			memcpy(&fav_arry[count],p,sizeof(GxBusPmDataFavItem));
			count++;
			i++;
		}
		else if(count>0)
		{
			memcpy(&fav_arry[count],p,sizeof(GxBusPmDataFavItem));
			count++;
		}
		else if(i<pos)//还没有找到第一个信息
		{
			i++;
		}
		GxCore_Free(p);
		cursor->Next();
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
}

/**
 * @brief 通过id获取喜爱组信息
 * @param id:喜爱组id
fav_arry:cas信息

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmFavGroupGetById(uint32_t id, GxBusPmDataFavItem * fav)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GxBusPmDataFavItem *p = NULL;
	uint32_t size = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (fav == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_fav;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open fav dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	input_id = id;

	p = (GxBusPmDataFavItem*)dbp->Get(input_id, &size);
	if(p == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---no fav exist id=%d!!\n",id);
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//
	}
	memcpy(fav,p,sizeof(GxBusPmDataFavItem));
	GxCore_Free(p);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 编辑fav参数,编辑前需要先获取原始参数 再进行修改 其中id号是
 不能改动的
 * @param fav:信息的指针

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmFavGroupModify(GxBusPmDataFavItem * fav)
{
	IDB *dbp = NULL;
	uint32_t input_id = 0;
	GX_PM_LOCK(gx_pm_mutex_handle);
	if (fav == NULL) {
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	dbp = dbp_fav;
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open fav dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	//gx_pm_verify_write(GX_PM_VERIFY_ERR);
	input_id = fav->id;
	if(input_id > 32 || input_id == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---fav id is  err!!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if (dbp->Set(input_id,fav, sizeof(GxBusPmDataFavItem)) != true)
	{
#ifdef GX_BUS_PM_DBUG
		PM_PRINTF("[PM]---add fav err !!\n");
#endif
		dbp->Sync();
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	//dbp->sync(dbp,0);
	//gx_pm_verify_write(GX_PM_VERIFY_OK);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

#if 0
/**
 * @brief 往一个喜爱组添加节目
 * @param fav_id:喜爱组id
prog_id:prog的id
num:节目数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmFavGroupProgAdd(uint32_t fav_id, uint32_t * prog_id, uint32_t num)
{
	int32_t sqlite_ret = 0;
	uint8_t sql_cmd[200];	//数据库的操作命令buffer
	uint32_t i = 0;
	if (prog_id == NULL) {
		return GXCORE_ERROR;
	}
	sqlite_ret = sqlite3_exec(db, (const char *)("begin transaction"), NULL, NULL, (char **)&err_msg);	//开始一个事务
	for (i = 0; i < num; i++) {
		sprintf((char *)sql_cmd, "update prog_table set favorite_flag =(favorite_flag|%d)\
				where id = %d;", 1 << (fav_id - 1), prog_id[i]);
		sqlite_ret = sqlite3_exec(db, (const char *)sql_cmd, NULL, NULL, (char **)&err_msg);
		if (sqlite_ret != SQLITE_OK) {
			sqlite_ret = sqlite3_exec(db, (const char *)("rollback transaction"), NULL, NULL, (char **)&err_msg);	//发生错误回滚事务
			return GXCORE_ERROR;	//数据库修改失败
		}
	}
	sqlite_ret = sqlite3_exec(db, (const char *)("commit transaction"), NULL, NULL, (char **)&err_msg);	//提交事务
	return GXCORE_SUCCESS;
}


/**
 * @brief 把节目从喜爱组中移除
 * @param fav_id:喜爱组id
prog_id:prog的id
num:节目数量

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmFavGroupProgDel(uint32_t fav_id, uint32_t * prog_id, uint32_t num)
{
	int32_t sqlite_ret = 0;
	uint8_t sql_cmd[200];	//数据库的操作命令buffer
	uint32_t i = 0;
	if (prog_id == NULL) {
		return GXCORE_ERROR;
	}
	sqlite_ret = sqlite3_exec(db, (const char *)("begin transaction"), NULL, NULL, (char **)&err_msg);	//开始一个事务
	for (i = 0; i < num; i++) {
		sprintf((char *)sql_cmd, "update prog_table set favorite_flag =(favorite_flag&%d)\
				where id = %d;", ~(1 << (fav_id - 1)), prog_id[i]);
		sqlite_ret = sqlite3_exec(db, (const char *)sql_cmd, NULL, NULL, (char **)&err_msg);
		if (sqlite_ret != SQLITE_OK) {
			sqlite_ret = sqlite3_exec(db, (const char *)("rollback transaction"), NULL, NULL, (char **)&err_msg);	//发生错误回滚事务
			return GXCORE_ERROR;	//数据库修改失败
		}
	}
	sqlite_ret = sqlite3_exec(db, (const char *)("commit transaction"), NULL, NULL, (char **)&err_msg);	//提交事务
	return GXCORE_SUCCESS;
}
#endif

/**
 * @brief 开始一个事务 典型应用:搜索存台时由search服务控制保存节目,berkeley下 还没有实现
 * @param void

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTransactionBegin(void)
{

	return GXCORE_SUCCESS;
}

/**
 * @brief 提交一个事务 典型应用:搜索存台时由search服务控制保存节目,berkeley下 还没有实现
 * @param void

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTransactionCommit(void)
{

	return GXCORE_SUCCESS;
}

/**
 * @brief 回滚一个事务 取消开始后的事务,恢复事务开始前的状态,berkeley下 还没有实现
 * @param void

 * @return     	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmTransactionRollback(void)
{

	return GXCORE_SUCCESS;
}


/**
 * @brief 获取观看模式
 * @param void
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmViewInfoGet(GxBusPmViewInfo* sys)
{
	size_t real_size = 0;
	GxBusPmViewInfo sys_info = {0};
	GX_PM_LOCK(gx_pm_mutex_handle);
	int32_t ret = 0;
	if(sys == NULL)
	{

#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---params err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret =  GX_PM_FS_EXIST  (GX_SYS_INFO_FILE_NAME);

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME, FILE_MODE);
	}
	else
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME,FILE_MODE);
	}
	if(gx_pm_view_info_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---GxBus_PmViewInfoGet open file err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	real_size = GX_PM_FS_READ(gx_pm_view_info_file, sys, 1,sizeof(GxBusPmViewInfo));
	if(real_size != sizeof(GxBusPmViewInfo))//不存在系统信息,重建一个sys info
	{
		memset(&sys_info,0,sizeof(GxBusPmViewInfo));
		sys_info.status = VIEW_INFO_DISABLE;
		//sys_info.radio_prog_cur = 1;
		GX_PM_FS_WRITE(gx_pm_view_info_file, &sys_info, 1,sizeof(GxBusPmViewInfo));
		GX_PM_FS_SYNC(gx_pm_view_info_file);
		memcpy(sys,&sys_info,sizeof(GxBusPmViewInfo));
	}
	GX_PM_FS_CLOSE(gx_pm_view_info_file);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 修改 观看模式
 * @param void
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t  GxBus_PmViewInfoModify(GxBusPmViewInfo* sys)
{
	GX_PM_LOCK(gx_pm_mutex_handle);
	int i = 0;
	uint8_t flag = 0;//用于表明是否需要重新筛选节目
	GxBusPmViewInfo sys_read = {0};
	int32_t ret = 0;
	if(sys == NULL)
	{

#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---params err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret =  GX_PM_FS_EXIST  (GX_SYS_INFO_FILE_NAME);

	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME, FILE_MODE);
	}
	else
	{
		gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME,FILE_MODE);
	}
	if(gx_pm_view_info_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---GxBus_PmViewInfoModify open file err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	GX_PM_FS_READ(gx_pm_view_info_file, (void*)&sys_read, 1,sizeof(GxBusPmViewInfo));
	GX_PM_FS_CLOSE(gx_pm_view_info_file);
	for(i=0; i<MAX_CMP_NAME; i++)
	{
		if(sys_read.letter[i] != sys->letter[i])
		{
			flag = 1;
			break;
		}
	}
	if(sys_read.group_mode != sys->group_mode
			||1 == flag
			||sys_read.pip_main_tp_id != sys->pip_main_tp_id
			||sys_read.pip_switch != sys->pip_switch
			||sys_read.sat_id != sys->sat_id
			||sys_read.skip_view_switch != sys->skip_view_switch
			||sys_read.stream_type != sys->stream_type
			||sys_read.taxis_mode != sys->taxis_mode
			||sys_read.tp_id != sys->tp_id
			||sys_read.tuner_num != sys->tuner_num
			||sys_read.fav_id!= sys->fav_id
			||sys_read.cas_id!= sys->cas_id
			||sys_read.order!= sys->order
			||sys_read.status != sys->status
			||sys->group_mode == GROUP_MODE_USER
			||sys->taxis_mode == TAXIS_MODE_USER)//需要重新筛选节目了
	{
		flag = 1;
	}
	if ( (flag == 0) && (gx_pm_prog_check_callback != NULL))
		flag = 1;

	gx_pm_view_info_file = GX_PM_FS_OPEN(GX_SYS_INFO_FILE_NAME, FILE_MODE);
	i = GX_PM_FS_WRITE(gx_pm_view_info_file, sys, 1,sizeof(GxBusPmViewInfo));
#ifdef GX_BUS_PM_DBUG
	SYS_INFO_PRINTF("[SYS INFO]---write ret = %d!\n",i);
#endif
	GX_PM_FS_SYNC(gx_pm_view_info_file);
	GX_PM_FS_CLOSE(gx_pm_view_info_file);
	if(1 == flag)
	{
		gx_pm_prog_num_get();//每次改变观看模式的时候筛选节目
	}
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

int32_t GxBus_PmHdProgNumGet(void)
{
	GxBusPmViewInfo sys={0};
	GxBusPmDataProg* prog = NULL;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	gx_pm_hd_prog_count = 0;

	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}

	if(gx_pm_view_info_get(&sys) != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---get file err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}

	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if (prog == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			delete cursor;
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return -1;
		}
		if(sys.skip_view_switch == VIEW_INFO_SKIP_CONTAIN)
		{
			if (prog->definition == GXBUS_PM_PROG_HD)
			{
				if (gx_pm_prog_check_user_callback(prog,&sys,1) == 0)
				{
					gx_pm_hd_prog_count++;
					GX_BUS_PM_PRINTF("[PM]---gx_pm_hd_prog_count++!!\n");
				}
			}

		}
		else if(sys.skip_view_switch == VIEW_INFO_SKIP_VANISH)
		{
			if (prog->definition == GXBUS_PM_PROG_HD && prog->skip_flag == GXBUS_PM_PROG_BOOL_DISABLE)
			{
				if (gx_pm_prog_check_user_callback(prog,&sys,1) == 0)
				{
					gx_pm_hd_prog_count++;
					GX_BUS_PM_PRINTF("[PM]---gx_pm_hd_prog_count++!!\n");
				}
			}
		}

		cursor->Next();
		GxCore_Free(prog);
	}
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return gx_pm_hd_prog_count;
}

/**
 * @brief 获取当前观看的节目信息
 * @param uint32_t tp_id,
 uint32_t cur_prog:当前观看节目的pos或者id 由GxBusPmViewInfo.type决定
 uint32_t ts_id,
 uint32_t service_id
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t  GxBus_PmViewInfoCurPlayGet(uint32_t* tp_id,
		uint32_t* cur_prog,
		uint32_t* ts_id,
		uint32_t* service_id)
{
	status_t ret = 0;
	GxBusPmDataProg prog = {0};
	GxBusPmViewInfo sys ;
	if(tp_id == NULL||cur_prog == NULL||ts_id == NULL||service_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---params err!\n");
#endif
		return GXCORE_ERROR;
	}
	GX_PM_LOCK(gx_pm_mutex_handle);

	ret = gx_pm_view_info_get(&sys);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---get file err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}


	if(sys.stream_type == GXBUS_PM_PROG_TV)
	{
		*cur_prog = sys.tv_prog_cur;
		if(sys.type == VIEW_INFO_ID)
		{
			ret =  gx_pm_prog_get_by_id(*cur_prog, &prog);
		}
		else if(sys.type == VIEW_INFO_POS)
		{
			ret = gx_pm_prog_get_by_pos(*cur_prog,1,&prog);
		}
	}
	else
	{
		*cur_prog = sys.radio_prog_cur;
		if(sys.type == VIEW_INFO_ID)
		{
			ret =  gx_pm_prog_get_by_id(*cur_prog, &prog);
		}
		else if(sys.type == VIEW_INFO_POS)
		{
			ret = gx_pm_prog_get_by_pos(*cur_prog,1,&prog);
		}
	}
	if(ret == GXCORE_ERROR
			||ret == -1)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_VIEW_INFO_PRINTF("[SYS INFO]---get prog err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	*tp_id = prog.tp_id;
	*ts_id = prog.ts_id;
	*service_id = prog.service_id;
	GX_PM_UNLOCK(gx_pm_mutex_handle);

	return GXCORE_SUCCESS;
}

/**
 * @brief 恢复数据库的默认信息
 * @param uint32_t max_sat:方案所能支持的最多卫星数量
 uint32_t max_tp:方案所能支持的最多tp数量
 uint32_t max_prog:方案所能支持的最多prog数量
 uint8_t* default_path:如果为null就是默认的路径
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmLoadDefault(uint32_t max_sat,uint32_t max_tp,uint32_t max_prog,uint8_t* default_path)
{
	status_t ret = 0;
	ret = GxBus_PmDbaseClose();
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm load default err !!\n");
#endif
		return GXCORE_ERROR;
	}
	/*删除所有数据库文件*/
	GX_PM_FS_RM(GX_PM_SAT_FILE_PATH);
	GX_PM_FS_RM(GX_PM_TP_FILE_PATH);
	GX_PM_FS_RM(GX_PM_PROG_FILE_PATH);
	GX_PM_FS_RM(GX_PM_FAV_FILE_PATH);
	GX_PM_FS_RM(GX_PM_STREAM_FILE_PATH);
	GX_PM_FS_RM(GX_PM_VERIFY_FILE_PATH);
	GX_PM_FS_RM(GX_PM_TV_USER_LIST_FILE_PATH);
	GX_PM_FS_RM(GX_PM_RADIO_USER_LIST_FILE_PATH);
	GX_PM_FS_RM(GX_PM_PROG_EXT_FILE_PATH);
	dbp_prog_ext = NULL;
	ret=GxBus_PmDbaseInit(max_sat,max_tp,max_prog,default_path);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm dbase init err !!\n");
#endif
		return GXCORE_ERROR;
	}

	//gx_pm_prog_num_get();//恢复出场设置后需要建立节目列表

	return GXCORE_SUCCESS;
}

/**
 * @brief 同步数据库 执行后数据被真正写入数据库
 * @param type:所要同步的数据类型
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmSync(GxBusPmSyncType type)
{
	IDB *dbp = NULL;
	GX_PM_LOCK(gx_pm_mutex_handle);
	switch(type)
	{
	case GXBUS_PM_SYNC_SAT:
		dbp = dbp_sat;
		break;

	case GXBUS_PM_SYNC_TP:
		dbp = dbp_tp;
		break;

	case GXBUS_PM_SYNC_PROG:
		dbp = dbp_prog;
		break;
	case GXBUS_PM_SYNC_FAV:
		dbp = dbp_fav;
		break;
	case GXBUS_PM_SYNC_EXT_PROG:
		dbp = dbp_prog_ext;
		break;

	default:
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---sync type err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
		break;
	}
	if(dbp == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;	//创建sat dbase失败
	}
	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	dbp->Sync();
	if(GXBUS_PM_SYNC_PROG == type)
	{
		gx_pm_prog_num_get();//这时候编辑了节目的状态，需要重建节目列表了
	}
	if(GXBUS_PM_SYNC_EXT_PROG == type)//同步ext时需要同步prog
	{
		dbp = dbp_prog;
		if(dbp == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---open dbase err !!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
		dbp->Sync();
	}
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 增加节目扩展信息
 * @param prog_id:增加扩展信息的节目id
 * 				size:扩展信息的大小
 * 				void* p:扩展信息
 * @return   	GXCORE_SUCCESS:执行正常
GXCORE_ERROR:执行失败
*/
status_t GxBus_PmProgExtInfoAdd(uint32_t prog_id,uint32_t size, void* p)
{
	status_t ret = 0;
	GxBusPmDataProg  prog;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if(dbp_prog_ext == NULL)
	{
		ret = gx_pm_dbase_open((uint8_t*)GX_PM_PROG_EXT_FILE_PATH,
				&dbp_prog_ext);
		if(ret != GXCORE_SUCCESS)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---create prog ext dbase err !!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
	}
	ret = gx_pm_prog_get_by_id(prog_id, &prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get prog by id err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	prog.ext_size = size;
	ret = gx_pm_prog_info_modify(&prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---modify prog err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret = gx_pm_prog_ext_modify(prog_id,size,p);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---modify prog err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}

/**
 * @brief 获取节目扩展信息
 * @param prog_id: 节目id
 * size:           扩展信息的大小
 * void* p:        用于保存扩展信息的空间
 * @return   GXCORE_SUCCESS:执行正常
 *           GXCORE_ERROR:执行失败
 */
status_t GxBus_PmProgExtInfoGet(uint32_t prog_id,uint32_t size, void* p)
{
	status_t ret = 0;
	GxBusPmDataProg  prog;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if(dbp_prog_ext == NULL)
	{
		ret = gx_pm_dbase_open((uint8_t*)GX_PM_PROG_EXT_FILE_PATH,
				&dbp_prog_ext);
		if(ret != GXCORE_SUCCESS)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---create prog ext dbase err !!\n");
#endif
			GX_PM_UNLOCK(gx_pm_mutex_handle);
			return GXCORE_ERROR;
		}
	}
	ret = gx_pm_prog_get_by_id(prog_id, &prog);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get prog by id err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	if(prog.ext_size != size)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---ext info size err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	ret = gx_pm_prog_ext_get(prog_id,size,p);
	if(ret != GXCORE_SUCCESS)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---get ext info err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}

	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return GXCORE_SUCCESS;
}


/**
 * @brief 获取pm的模块锁
 * @return   GXCORE_SUCCESS:执行正常
 *           GXCORE_ERROR:执行失败
 */
status_t GxBus_PmLock(void)
{
	if (gx_pm_option_lock == 0)
	{
		GxCore_MutexCreate(&gx_pm_option_lock);
	}

	GxCore_MutexLock(gx_pm_option_lock);

	return GXCORE_SUCCESS;
}
/**
 * @brief  释放pm的模块锁
 * @return   GXCORE_SUCCESS:执行正常
 *           GXCORE_ERROR:执行失败
 */
status_t GxBus_PmUnlock(void)
{
	if (gx_pm_option_lock == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---pm lock not create !!\n");
#endif
		return GXCORE_ERROR;
	}

	GxCore_MutexUnlock(gx_pm_option_lock);

	return GXCORE_SUCCESS;
}
/**
 * @brief  获取数据库变化记录号，用意判断数据库是否发生了变化
 * @return  记录号
 */
int32_t GxBus_PmGetRecord(void)
{
	enum gx_pm_verify_flag flag = GX_PM_VERIFY_ERR;
	int32_t buf = 0;
	handle_t ret = GXCORE_FILE_UNEXIST;

	GX_PM_LOCK(gx_pm_mutex_handle);
	ret =  GX_PM_FS_EXIST  (GX_PM_VERIFY_FILE_PATH);
	if(ret == GXCORE_FILE_UNEXIST)
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
		GX_PM_FS_WRITE(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
		GX_PM_WRITEAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));//第一次创建文件，修改记录清零
	}
	else
	{
		gx_pm_verify_file = GX_PM_FS_OPEN(GX_PM_VERIFY_FILE_PATH, FILE_MODE);
	}
	if(gx_pm_verify_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--open verify file err!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GX_PM_VERIFY_ERR;
	}
	GX_PM_READAT(gx_pm_verify_file,&buf,1,4,sizeof(enum gx_pm_verify_flag));
	//GX_PM_FS_READ(gx_pm_verify_file, &flag, 1,sizeof(enum gx_pm_verify_flag));
	GX_PM_FS_CLOSE(gx_pm_verify_file);
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return buf;
}

/*测试记录号*/
void GxBus_PmGetTest(void)
{
	int32_t buf = 0;

	gx_pm_verify_write(GX_PM_VERIFY_ERR);
	buf = GxBus_PmGetRecord();
	gxlogd("1 buf = %d\n",buf);
	GxCore_ThreadDelay(1000);
	gx_pm_verify_write(GX_PM_VERIFY_OK);
	buf = GxBus_PmGetRecord();
	gxlogd("2 buf = %d\n",buf);
	GxCore_ThreadDelay(1000);
}

/**
 * @brief 把当前数据库信息生成符合默认数据库工具格式的文件
 * @param file_patch:文件生成路径
 * @return   GXCORE_SUCCESS: 执行正常
 *           GXCORE_ERROR: 执行失败
 */
status_t GxBus_PmCreateDefaultDb(int8_t* file_patch)
{
	uint32_t pos_num = 0;
	uint32_t pos_data = 0;
	int32_t ret = 0;
	uint32_t flag = 0;
	handle_t default_info_file = 0;
	gx_pm_tp_pos_by_sat* sat_pos_by_id = NULL;
	gx_pm_tp_pos_by_id* tp_pos_by_id = NULL;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if(file_patch == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--default file  patch is NULL!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	default_info_file = GxCore_Open((const char*)file_patch, "w+");
	if(default_info_file<0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--no defaule db file!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return GXCORE_ERROR;
	}
	/*写入起始标记 应该为0x4b386753*/
	flag = 0x4b386753;
	ret = GxCore_Write(default_info_file, (void*)&flag, 1, 4);
	if(ret != 4)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--write start flag err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	pos_data = ret + 3*sizeof(uint32_t);//start flag 后3*sizeof(uint32_t)记录sat tp prog数量
	pos_num = ret;

	GxCore_Seek(default_info_file, pos_data, (GxSeek)SEEK_SET);
	sat_pos_by_id = (gx_pm_tp_pos_by_sat*)GxCore_Mallocz(gx_pm_sat_num_get_realy()*sizeof(gx_pm_tp_pos_by_sat));
	if(sat_pos_by_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---no memeroy !!\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	ret = gx_pm_create_default_sat(default_info_file,sat_pos_by_id);//写入卫星数据
	if(ret == -1)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--create defaule sat err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	pos_data += ret*sizeof(DefaultSat_t);
	GxCore_Seek(default_info_file, pos_num, (GxSeek)SEEK_SET);
	ret = GxCore_Write(default_info_file, &ret, 1, 4);//写入卫星数量
	if(ret != 4)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--write sat num err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	pos_num += sizeof(uint32_t);

	GxCore_Seek(default_info_file, pos_data, (GxSeek)SEEK_SET);
	//建立一个tp id和tp pos的关系表
	tp_pos_by_id =  (gx_pm_tp_pos_by_id*)GxCore_Mallocz(sizeof(gx_pm_tp_pos_by_id)*gx_pm_tp_num_get_realy());
	if(tp_pos_by_id == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---no memeroy !!\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	ret = gx_pm_create_default_tp(default_info_file,pos_data,sat_pos_by_id, tp_pos_by_id);//写入tp数据
	if(ret == -1)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--create defaule tp err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	pos_data += ret*sizeof(DefaultTp_t);
	GxCore_Seek(default_info_file, pos_num, (GxSeek)SEEK_SET);
	ret = GxCore_Write(default_info_file, &ret, 1, 4);//写入tp数量
	if(ret != 4)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--write tp num err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	pos_num += sizeof(uint32_t);

	GxCore_Seek(default_info_file, pos_data, (GxSeek)SEEK_SET);
	gx_set_pos_data(pos_data);
	ret = gx_pm_create_default_prog(default_info_file,sat_pos_by_id, tp_pos_by_id);//写入prog数据
	if(ret == -1)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--create defaule prog err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	GxCore_Seek(default_info_file, pos_num, (GxSeek)SEEK_SET);
	ret = GxCore_Write(default_info_file, &ret, 1, 4);//写入prog数量
	if(ret != 4)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--write prog num err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}

	GxCore_Seek(default_info_file, 0, (GxSeek)SEEK_END);
	/*写入结束标记 应该为0x4b386753*/
	flag = 0x4b386745;//Eg8K
	ret = GxCore_Write(default_info_file, (void*)&flag, 1, 4);
	if(ret != 4)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]--write end flag err\n");
#endif
		ret = GXCORE_ERROR;
		goto out;
	}
	ret = GXCORE_SUCCESS;
out:
	if(sat_pos_by_id != NULL)
	{
		GxCore_Free(sat_pos_by_id);
	}
	if(tp_pos_by_id != NULL)
	{
		GxCore_Free(tp_pos_by_id);
	}
	GxCore_Sync(default_info_file);
	GxCore_Close(default_info_file);
	GX_PM_UNLOCK(gx_pm_mutex_handle);

	return ret;
}

/**
 * @brief 在指定tp_id下的节目数量
 * @param tp_id:节目所属的tp_id

 * @return  -1:发生错误
            正常数字:prog数量
*/
int32_t GxBus_PmProgNumGetByTpId(uint32_t tp_id)
{
	IDB *dbp = NULL;
	ICursor *cursor = NULL;
	GxBusPmDataProg *prog = NULL;
	uint32_t size = 0;
	uint32_t count = 0;

	if(0 == tp_id)
		return -1;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if(NULL == dbp_prog)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}

	dbp = dbp_prog;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if(NULL == prog)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			goto err;
		}

		if(prog->tp_id == tp_id)
			count++;

		GxCore_Free(prog);
		cursor->Next();
	}

	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
err:
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return -1;
}

/**
 * @brief 在指定tp_id下,获取n个数量节目
 * @param tp_id:节目所属的tp_id
num:想要获取的prog数量
prog_arry:获取的信息的指针prog_arry:获取的信息的指针,由该函数的使用者开辟空间

 * @return  -1:发生错误
 正常数字:读取的真正的prog数量
 */
int32_t GxBus_PmProgGetByTpId(uint32_t tp_id, uint32_t num, GxBusPmDataProg *prog_arry)
{
	IDB *dbp = NULL;
	ICursor *cursor = NULL;
	GxBusPmDataProg *prog = NULL;
	uint32_t size = 0;
	int32_t count = 0;

	if(0 == tp_id || 0 == num || NULL == prog_arry)
		return -1;

	GX_PM_LOCK(gx_pm_mutex_handle);
	if(NULL == dbp_prog)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		GX_PM_UNLOCK(gx_pm_mutex_handle);
		return -1;
	}
	dbp = dbp_prog;
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if(NULL == prog)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			goto err;
		}

		if(prog->tp_id == tp_id)
		{
			memcpy(&prog_arry[count], prog, sizeof(GxBusPmDataProg));
			count++;
		}

		GxCore_Free(prog);
		if(count == (int32_t)num)
			break;

		cursor->Next();
	}

	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return count;
err:
	delete cursor;
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return -1;
}

/**
 * @brief 在指定GxBusPmViewInfo下,获取n个数量节目
 * @param GxBusPmViewInfo: 指定的GxBusPmViewInfo

 * @return 正常数字:返回符号条件的prog数量
 */
uint32_t GxBus_PmProgNumGetByViewInfo(GxBusPmViewInfo* sys)
{
	GxBusPmDataProg* prog = NULL;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;;
	uint32_t prog_num = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(dbp == NULL || sys == NULL)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		goto err_f;
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if (prog == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			goto err_s;	//创建sat dbase失败
		}
		if(sys->gx_pm_prog_check != NULL
				&& sys->group_mode == GROUP_MODE_USER)//用户自定义过滤
		{
			if((sys->gx_pm_prog_check(prog,NULL) == 0))//符合要求
			{
				prog_num++;
			}
		}
		else
		{
			/*组相符*/
			if(gx_pm_prog_check_group_mod(prog,sys))
			{
				if(gx_pm_prog_check_taxis_mod(prog,sys))
				{
					if (gx_pm_prog_check_user_callback(prog,sys,0) == 0)
						prog_num++;
				}
			}
		}
		GxCore_Free(prog);
		cursor->Next();
	}
err_s:
	delete cursor;
err_f:
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return prog_num;
}

/**
 * @brief 在指定GxBusPmViewInfo下,获取n个数量节目id
 * @param GxBusPmViewInfo: 指定的GxBusPmViewInfo
          num: 可获取最大节目数
          id_arry： id接收数组

 * @return 正常数字:获取的prog数量
 */
uint32_t GxBus_PmProgIdArryGetByViewInfo(GxBusPmViewInfo* sys, uint32_t num, uint32_t* id_arry)
{
	GxBusPmDataProg* prog = NULL;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;;
	uint32_t prog_num = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(dbp == NULL || sys == NULL || id_arry == NULL || num == 0)
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		goto err_f;
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if (prog == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			goto err_s;	//创建sat dbase失败
		}
		if(sys->gx_pm_prog_check != NULL
				&& sys->group_mode == GROUP_MODE_USER)//用户自定义过滤
		{
			if(sys->gx_pm_prog_check(prog,NULL) == 0)//符合要求
			{
				id_arry[prog_num] = prog->id;
				prog_num++;
			}
		}
		else
		{
			/*组相符*/
			if(gx_pm_prog_check_group_mod(prog,sys))
			{
				if(gx_pm_prog_check_taxis_mod(prog,sys))
				{
					if (gx_pm_prog_check_user_callback(prog,sys,0) == 0)
					{
						id_arry[prog_num] = prog->id;
						prog_num++;
					}
				}
			}
		}
		GxCore_Free(prog);
		if(prog_num == num)
			break;
		cursor->Next();
	}
err_s:
	delete cursor;
err_f:
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return prog_num;
}

/**
 * @brief 在指定GxBusPmViewInfo下,获取n个数量节目数据
 * @param GxBusPmViewInfo: 指定的GxBusPmViewInfo
          num: 可获取最大节目数
          id_arry： id接收数组
          name_arry: channel name

 * @return 正常数字:获取的prog数量
 */
uint32_t GxBus_PmProgInfoArryGetByViewInfo(GxBusPmViewInfo* sys, uint32_t num, uint32_t* id_arry, int8_t **name_arry)
{
	GxBusPmDataProg* prog = NULL;
	IDB *dbp = NULL;
	ICursor *cursor = NULL;;
	uint32_t prog_num = 0;

	GX_PM_LOCK(gx_pm_mutex_handle);
	dbp = dbp_prog;
	if(dbp == NULL || sys == NULL || id_arry == NULL || num == 0 || (NULL == name_arry))
	{
#ifdef GX_BUS_PM_DBUG
		GX_BUS_PM_PRINTF("[PM]---open prog dbase err !!\n");
#endif
		goto err_f;
	}
	cursor = dbp->CreateCursor();
	while(!cursor->Eof())
	{
		uint32_t size = 0;
		prog = (GxBusPmDataProg*)cursor->GetRaw(&size);
		if (prog == NULL)
		{
#ifdef GX_BUS_PM_DBUG
			GX_BUS_PM_PRINTF("[PM]---get prog data err !!\n");
#endif
			goto err_s;	//创建sat dbase失败
		}
		if(sys->gx_pm_prog_check != NULL
				&& sys->group_mode == GROUP_MODE_USER)//用户自定义过滤
		{
			if(sys->gx_pm_prog_check(prog,NULL) == 0)//符合要求
			{
				id_arry[prog_num] = prog->id;
                memcpy(name_arry[prog_num], prog->prog_name, MAX_PROG_NAME);
				prog_num++;
			}
		}
		else
		{
			/*组相符*/
			if(gx_pm_prog_check_group_mod(prog,sys))
			{
				if(gx_pm_prog_check_taxis_mod(prog,sys))
				{
					id_arry[prog_num] = prog->id;
                    memcpy(name_arry[prog_num], prog->prog_name, MAX_PROG_NAME);
					prog_num++;
				}
			}
		}
		GxCore_Free(prog);
		if(prog_num == num)
			break;
		cursor->Next();
	}
err_s:
	delete cursor;
err_f:
	GX_PM_UNLOCK(gx_pm_mutex_handle);
	return prog_num;
}


