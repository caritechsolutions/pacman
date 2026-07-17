#ifndef __SEARCH_PRIVATE_H__
#define __SEARCH_PRIVATE_H__

#include "module/pm/gxpm_manage.h"

extern bool thiz_SearchPrintEnable;

#define SEARCH_ERR(fmt, args...)  do {                              \
	gxlogd("\033[31m");                                             \
	gxlogd("[SEARCH_ERR][%s:%d]: ", __func__, __LINE__);            \
	gxlogd(fmt, ##args);                                            \
	gxlogd("\033[0m\n");                                            \
} while(0)

#define SEARCH_DBG(fmt, args...)  do {                              \
	if (thiz_SearchPrintEnable) {                                   \
		gxlogd("\033[33m");                                         \
		gxlogd("[SEARCH_DBG][%s:%d]: ", __func__, __LINE__);        \
		gxlogd(fmt, ##args);                                        \
		gxlogd("\033[0m\n");                                        \
	}                                                               \
} while(0)

#define SEARCH_FREE(x)  if(x){GxCore_Free(x);x=NULL;}

typedef struct
{
	struct gxlist_head  list;
	uint16_t            service_id;
	uint8_t             data_plp_id;
	uint8_t             common_plp_exist;
	uint8_t             common_plp_id;
	uint8_t             common_status;
	uint8_t             muti_ts_id;
	uint8_t             muti_ts_code;
	uint16_t            muti_ts_exist:1;
	uint16_t            pdmx_flag:2;//1:t2mi, 2:ule, 3:...
	uint16_t            pdmx_pid:13;
}ProgCache;

typedef struct
{
	struct gxlist_head  list;
	struct gxlist_head  prog_cache_list;
	GxBusPmDataTP       tp;
}TpCache;

typedef struct
{
	struct gxlist_head  list;
	struct gxlist_head  tp_cache_list;
	uint32_t            sat_id;
	GxBusPmDataSatType  type;
}SatCache;

typedef struct
{
	struct gxlist_head  sat_cache_list;
	handle_t            mutex;
}SearchCacheCtrl;

extern bool search_cache_prog_exist_check(uint32_t sat_id,
											uint32_t tp_id,
											uint16_t service_id,
											uint8_t muti_ts_id,
											uint8_t pdmx_flag,
											uint16_t pdmx_pid,
											uint8_t data_plp_id,
											uint8_t common_plp_exist,
											uint8_t common_plp_id,
											uint8_t common_status);

extern bool search_cache_tp_exist_check(uint32_t sat_id,
											uint32_t fre,
											uint32_t symbol,
											GxBusPmTpPolar polar,
											fe_modulation_t modulation,
											uint16_t* tp_id);

extern status_t search_cache_init(uint32_t sat_num, uint32_t *sat_id_array, uint32_t tp_num, uint32_t *tp_id_array);

extern void search_cache_destroy(void);

extern status_t search_cache_tp_add(GxBusPmDataTP tp);

extern status_t search_cache_prog_add(GxBusPmDataProg prog);
#endif
