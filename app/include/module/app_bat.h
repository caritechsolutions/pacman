#ifndef __APP_BAT_H__
#define __APP_BAT_H__

#include "gxcore.h"

typedef uint8_t (*bat_parse_descriptor)(uint8_t tag,uint8_t* pData,uint16_t ts_id);
#define SI_SECTION_NOT_RECEIVED		( 0x00)
#define SI_SECTION_RECEIVED			(0xFF)

#define MAX_BOUQUET_LIST 				(50)//100
#define MAX_BOUQUET_NAME 			(32)
#define MAX_SECTION_NUM 				(32)
#define MAX_BAT_NUM 					(100)
#define MAX_SERVICE_NUM 				(500)

typedef struct _bouquet_service_s
{
	 uint16_t ts_id;
	 uint16_t service_id;
	 uint16_t service_type;
	 uint16_t order;
}_bouquet_service_t;

typedef struct _bouquet_va_service_s
{
	 uint16_t service_lcn;
	 uint16_t prog_id;
	 uint16_t order;
}_bouquet_va_service_t;


typedef struct _bouquet_list_s
{
	uint8_t     bouquet_name[MAX_BOUQUET_NAME];
	uint16_t     service_num;
	uint8_t     valid_service_num;
	uint8_t     need_update;
	uint8_t     version;
	uint16_t    bouquet_id;
	_bouquet_service_t service_list[MAX_SERVICE_NUM];
	_bouquet_va_service_t valid_list[MAX_SERVICE_NUM];
}_bouquet_list_t;

typedef struct bat_section_info_s
{
	uint8_t 	last_section_num;
	uint8_t		section_data[MAX_SECTION_NUM];
}bat_section_info_t;

typedef struct _bouquet_update_s
{
	bat_section_info_t table_info;
	_bouquet_list_t    updateing_info;
}_bouquet_update_t;


typedef struct bouquet_full_info_s
{
	uint8_t   is_update;
	uint32_t  last_fetch_msec;
	_bouquet_update_t *p_update_info;
	_bouquet_list_t   bouquet_list;
}bouquet_full_info_t;

#if 0
typedef struct _bouquet_list_s
{
	uint8_t bouquet_name[MAX_BOUQUET_NAME];
	uint8_t service_num;
	uint8_t cur_prog_pos;
	uint16_t service_id[MAX_SERVICE_NUM];
	uint16_t valid_service_pos[MAX_SERVICE_NUM];
	uint16_t valid_service_num;
	uint8_t service_type[MAX_SERVICE_NUM];
	uint16_t bouquet_id;
	uint8_t  need_update;
	uint16_t service_lcn[MAX_SERVICE_NUM];
}BOUQUET_LIST_T;

typedef struct _bat_info_s
{
	int8_t	index;
	int8_t	total;
	uint8_t	version;
	uint8_t 	last_section_num;
	uint8_t	section_data[MAX_SECTION_NUM];
	uint16_t	bouquet_id;
}BAT_INFO_T;
#endif

typedef struct _bqt_sort_info_s
{
	uint8_t real_bqt_id;
	uint16_t bouquet_id;
}_bqt_sort_info_t;

typedef struct app_bat AppBat;
struct app_bat
{
	int32_t     subt_id;
	uint32_t    req_id;
	void        (*start)(AppBat*);
	uint32_t    (*stop)(AppBat*);
};

extern AppBat g_AppBat;
extern uint8_t g_bouquet_num;
extern int8_t g_group_direction;

uint8_t  *app_get_bouquet_name(int bouquet_index,int *bouquet_id);

int  app_bouquet_list_get_pos_by_progid(int bouquet_index, int prog_id, uint32_t *pos);
int app_bouquet_list_get_prog_id(int bouquet_index,int list_pos,int bouquet_id);
int app_bouquet_list_get_num(int bouquet_index,int bouquet_id);
int app_get_bouquet_num(void);

void app_bouquet_init(void);
void app_bouquet_reset_all_time(void);
void app_bouquet_clear_all(void);

void app_bouque_set_update(int ts_id,int service_id);

#endif

