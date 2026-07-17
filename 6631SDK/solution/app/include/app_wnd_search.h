#ifndef APP_SEARCH_H
#define APP_SEARCH_H

#include "app.h"

typedef enum
{
	SEARCH_SAT,
	SEARCH_BLIND_ONLY,
	SEARCH_TP,
	SEARCH_PID,
	SEARCH_CABLE,
    SEARCH_DVBT2,
    SEARCH_DTMB,
#if NET_SEARCH_SUPPORT
	SEARCH_SERVER,
	SEARCH_STREAM,
#endif
	SEARCH_NONE
}SearchType;

typedef enum
{
    AUTO_SEARCH,
    NIT_SEARCH,
    MANUAL_SEARCH,
    FULL_SEARCH
}SearchMode;

typedef struct
{
	uint32_t id_num;
	uint32_t *id_array;
}SearchIdSet;

uint32_t app_tuner_cur_tuner_get(void);
void app_tuner_cur_tuner_set(uint32_t CurTuner);
void app_search_set_search_mode(SearchMode  Mode);

#if (DEMOD_DVB_S > 0)
WndStatus app_search_opt_dlg(SearchType type, SearchIdSet *id);
void app_search_start(void);
int app_antenna_setting_init(int SatSel, int TpSel);
int app_satellite_list_init(int SatSel);
int app_tp_list_init(int SatSel, int TpSel);
bool app_get_super_search_flag(void);
#endif

#define TIMEOUT_PAT 5000
#define TIMEOUT_PMT 7000
#define TIMEOUT_SDT 7000
#define TIMEOUT_NIT 10000

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))||(DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
#define TIMEOUT_T_PAT                          (3000)
#define TIMEOUT_T_SDT                          (8000)
#define TIMEOUT_T_NIT                          (10000)
#define TIMEOUT_T_PMT                          (8000)
void app_search_set_search_type(SearchType type);
#endif


#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)||(DEMOD_ISDBT > 0)
#define DVBC_FRE_BEGIN_LOW       (115000)
#define DVBC_FRE_BEGIN_HIGH      (858000)
#define DTMB_FRE_BEGIN_LOW       (52500)
#define DTMB_FRE_BEGIN_HIGH      (866000)
#define SYMBOL_BEGIN_LOW    (2000)
#define SYMBOL_BEGIN_HIGH   (7000)

#define TP_MAX_NUM          (200)

typedef struct
{
	uint8_t app_buf_tv[400];
	uint8_t app_buf_radio[400];
	uint16_t  app_tv_num ;
	uint16_t  app_radio_num ;
	uint16_t tp_num; /*实际搜索TP个数，超过最大TP个数情况下，tp_num小于设置的个数*/
	uint16_t tp_cur; /*当前搜索TP索引*/
	uint32_t app_tpid[TP_MAX_NUM]; /*实际搜索TP对应ID，超过最大TP个数情况下，个数小于设置的个数*/
	uint32_t app_tv_num_perTP[TP_MAX_NUM]; /*每个TP对应搜索到的电视节目数*/
	uint32_t app_radio_num_perTP[TP_MAX_NUM]; /*每个TP对应搜索到的广播节目数*/
}search_result;

typedef struct
{
	uint32_t app_fre_array[TP_MAX_NUM];
	uint16_t app_qam_array[TP_MAX_NUM];
	uint16_t app_symb_array[TP_MAX_NUM];
	uint16_t app_fre_tsid[TP_MAX_NUM];
	uint16_t num;
	uint8_t nit_flag;
}search_fre_list;

extern search_fre_list searchFreList ;

int app_search_check_fre_valid(uint32_t fre,unsigned char demod_type);
int app_search_check_sym_valid(uint32_t sym);
int app_search_check_fre_range_valid(uint32_t lowfre,uint32_t highfre,unsigned char demod_type);
int app_search_dvbc_all_search_fre_info_get(uint32_t begin_fre, uint32_t end_fre, uint32_t symbol_rate, uint32_t qam);
int app_search_dtmb_all_search_fre_info_get(uint32_t begin_fre, uint32_t end_fre, uint32_t symbol_rate, uint32_t qam);
void app_search_scan_cable_dtmb_start(search_fre_list searchlist, GxBusPmDataSatType type);
void app_search_scan_manual_mode(uint32_t fre,uint32_t symbol_rate,uint32_t qam);
void app_search_scan_nit_mode(void);
void app_search_scan_all_mode(void);
void app_search_si_subtable_msg(GxMessage * msg);
int app_search_service_ext(GxMessage *pMsg);
#endif

#if LCN_SUPPORT
#define SEARCH_EXTEND_MAX (2)
typedef struct
{
	GxSearchExtend searchExtend[SEARCH_EXTEND_MAX];
	uint32_t extendnum;
}search_extend;

typedef int (*search_add_extend_table)(search_extend* searchExtendList);

extern search_extend g_SearchExtendList;
extern search_add_extend_table app_search_add_extend_callback;

void app_search_register_add_extend_table_callback(search_add_extend_table search_extend_call_back);
int app_search_add_extend_table_call_back(search_extend* searchExtendList );
#endif


#endif

