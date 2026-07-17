#include "gxcore.h"
#ifndef __GX_URL_CONTROL_H__
#define __GX_URL_CONTROL_H__

__BEGIN_DECLS
#define GX_MAX_SERVER_LEN								(1024)

typedef struct
{
	status_t (*modify_server_by_index)(uint32_t server_index, char *server_url);
	status_t (*modify_stream_url_by_index)(uint32_t stream_index, char *stream_url);
	status_t (*delete_server_url_by_index)(uint32_t *server_index_arry, uint32_t del_num);
	status_t (*delete_stream_url_by_index)(uint32_t *stream_index_arry, uint32_t del_num);
	int (*add_server_url_to_file)(char *server_url, uint32_t *index);
	int (*add_stream_url_to_file)(char *stream_url, uint32_t *index);
	status_t (*get_server_url_by_index)(uint32_t *server_index_arry, char **server_url_arry, uint32_t get_num);
	status_t (*get_stream_url_by_index)(uint32_t *stream_index_arry, char **stream_url_arry, uint32_t get_num);
	status_t (*load_url_from_file)(void);
}NetPmCtrl;

typedef enum
{
	SERVER_URL = 0,
	STREAM_URL,
}GxBusNetUrlType;

status_t GxNetAddUrlToFile(GxBusNetUrlType url_type, char *sat_url, uint32_t *index);
status_t GxNetDeleteUrlByIndex(GxBusNetUrlType url_type, uint32_t *index_arry, uint32_t delete_num);
status_t GxNetModifyUrlByIndex(GxBusNetUrlType url_type, uint32_t index, char *net_url);
status_t GxNetGetUrlByIndex(GxBusNetUrlType url_type, uint32_t *index_arry, char **url_arry, uint32_t get_num);
status_t GxNetUrlManageInit(void);

void GxNetPm_ModuleRegister(void);

__END_DECLS
#endif
