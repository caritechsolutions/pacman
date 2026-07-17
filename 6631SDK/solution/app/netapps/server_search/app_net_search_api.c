#include "app.h"
#if NET_SEARCH_SUPPORT
#include <cJSON.h>
#include "module/app_frontend_board_config.h"
#include "app_net_search_api.h"
#include "module/app_log.h"

#define GX_URL_FS_RM(path)                      (GxCore_FileDelete(path))
#define GX_URL_FS_CLOSE(file)                   (GxCore_Close(file))
#define GX_URL_FS_EXIST(path)                   (GxCore_FileExists(path))
#define GX_URL_FS_OPEN(path,mode)               (GxCore_Open(path, mode))
#define GX_URL_FS_SEEK(file,offset,whence)      (GxCore_Seek(file,offset,whence))
#define GX_URL_FS_READ(file,buf,size,nmemb)     (GxCore_Read(file,buf,size,nmemb))
#define GX_URL_FS_WRITE(file,buf,size,nmemb)    (GxCore_Write(file,buf,size,nmemb))
#define GX_URL_READAT(file,buf,size,nmemb,pos)  (GxCore_ReadAt(file,buf,size,nmemb,pos))
#define GX_URL_WRITEAT(file,buf,size,nmemb,pos) (GxCore_WriteAt(file, buf,size,nmemb,pos))
#define FILE_MODE  "c"

#define USERNAME_INDEX		"username"
#define PASSWORD_INDEX		"password"
#define STREAM_INDEX		"stream url"
#define SERVER_INDEX		"server url"
#define SERVERNAME_INDEX	"server name"
#define SERVERINFO_INDEX	"server info"
#define SERVER_TUNER_ID		APP_MULTI_DEMOD_NUM

#if DVB_SERVER_SEARCH_SUPPORT
static cJSON * app_get_json_object(char *path)
{
	handle_t file;
    uint32_t data_len = 0;
    cJSON *root_json = NULL;
    char *buffer = NULL;

    if(path == NULL)
    {
        return NULL;
    }
    file = GX_URL_FS_OPEN(path, FILE_MODE);
    if(file < 0)
    {
        return NULL;
    }
    data_len = GX_URL_FS_SEEK(file, 0, SEEK_END);
    GX_URL_FS_SEEK(file, 0, SEEK_SET);
    buffer = (char*)GxCore_Calloc(data_len + 1, sizeof(buffer));
    if(buffer == NULL)
    {
        GX_URL_FS_CLOSE(file);
        return NULL;
    }
	if(data_len != 0)
    {
        GX_URL_FS_READ(file, buffer, 1, data_len);
        root_json = cJSON_Parse(buffer);
    }
    GX_URL_FS_CLOSE(file);
	APP_FREE(buffer);
	return root_json;
}

static int32_t app_get_object_array_num_by_name(cJSON *object, char* name)
{
    cJSON *arrayItem = NULL;
	int32_t count = 0;

	if(object == NULL || name == NULL)
		return -1;
    arrayItem = cJSON_GetObjectItem(object, name);
	if(arrayItem == NULL)
		return -1;
	count = cJSON_GetArraySize(arrayItem);
	return count;
}

int32_t app_get_server_num_from_json(cJSON *object)
{
	int32_t server_num = 0;
	if(object == NULL)
		return -1;
	server_num = app_get_object_array_num_by_name(object, SERVERINFO_INDEX);
	return server_num;
}

int32_t app_get_stream_num_from_json(cJSON *object)
{
	int32_t stream_num = 0;
	if(object == NULL)
		return -1;
	stream_num = app_get_object_array_num_by_name(object, STREAM_INDEX);
	return stream_num;
}


status_t app_add_net_server_from_json(char *json_path)
{
#define GX_MAX_NET_URL_LEN  1024
	cJSON *root_json = NULL;
	int32_t server_num = 0;
	int32_t stream_num = 0;
	status_t ret = GXCORE_ERROR;
	cJSON* server_item = NULL;
	cJSON* server_array_item = NULL;
	cJSON* stream_array_item = NULL;
	cJSON* server_info_item = NULL;
	uint32_t pm_server_index = 0;
	GxMsgProperty_NodeAdd ServerAdd;
	GxMsgProperty_NodeAdd StreamAdd;
	uint32_t pm_stream_index = 0;
	uint8_t name_count = 0;
	uint32_t server_id = 0;
	int i = 0;
	int j = 0;

	if(json_path == NULL)
	{
		ret = GXCORE_ERROR;
		return ret;
	}
	root_json = app_get_json_object(json_path);
	if(root_json == NULL)
		return GXCORE_ERROR;
	server_num = app_get_server_num_from_json(root_json);
	app_log_debug("###server num ============%d\n", server_num);
	if(server_num < 0)
	{
		app_log_error("###app_json_parse: This json file format error\n");
		ret = GXCORE_ERROR;
		goto out;
	}
    server_info_item = cJSON_GetObjectItem(root_json, SERVERINFO_INDEX);
	for(i = 0; i < server_num; i++)
	{
		memset(&ServerAdd, 0, sizeof(GxMsgProperty_NodeAdd));
		ServerAdd.node_type = NODE_SAT;
		ServerAdd.sat_data.type = GXBUS_PM_SAT_NET;
		ServerAdd.sat_data.tuner = SERVER_TUNER_ID;
		server_array_item = cJSON_GetArrayItem(server_info_item, i);
		server_item = cJSON_GetObjectItem(server_array_item, SERVER_INDEX);
		if(server_item != NULL)
		{
			GxBus_PmAddNetServerUrlToFile(server_item->valuestring, &pm_server_index);
			if(pm_server_index > 0)
			{
				ServerAdd.sat_data.sat_net.index = pm_server_index;
				server_item = NULL;
				server_item = cJSON_GetObjectItem(server_array_item, SERVERNAME_INDEX);
				if(server_item != NULL)
				{
					memcpy((char*)ServerAdd.sat_data.sat_net.server_name, server_item->valuestring, (MAX_SAT_NAME <= strlen(server_item->valuestring) ? MAX_SAT_NAME - 1 : strlen(server_item->valuestring)));
				}
				else
				{
					snprintf((char*)ServerAdd.sat_data.sat_net.server_name, MAX_SAT_NAME, "New server%d", name_count++);
				}
				server_item = NULL;
				server_item = cJSON_GetObjectItem(server_array_item, USERNAME_INDEX);
				if(server_item != NULL)
				{
					memcpy((char*)ServerAdd.sat_data.sat_net.user_name, server_item->valuestring, (USER_NAME_LEN <= strlen(server_item->valuestring) ? USER_NAME_LEN - 1: strlen(server_item->valuestring)));
				}
				else
				{
					;
				}
				server_item = NULL;
				server_item = cJSON_GetObjectItem(server_array_item, PASSWORD_INDEX);
				if(server_item != NULL)
				{
					memcpy((char*)ServerAdd.sat_data.sat_net.password, server_item->valuestring, (PASSWORD_LEN <= strlen(server_item->valuestring) ? PASSWORD_LEN - 1: strlen(server_item->valuestring)));
				}
				else
				{
					;
				}
				if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_ADD, &ServerAdd))
				{
					app_log_error("========add net sat error\n");
					continue;
				}
				server_id = ServerAdd.sat_data.id;
				server_item = NULL;
				stream_array_item = cJSON_GetObjectItem(server_array_item, STREAM_INDEX);
				if(stream_array_item != NULL)
				{
					stream_num = app_get_stream_num_from_json(server_array_item);
					for(j = 0; j < stream_num; j++)
					{
						server_item = cJSON_GetArrayItem(stream_array_item, j);
						if(server_item != NULL)
						{
							GxBus_PmAddNetStreamUrlToFile(server_item->valuestring, &pm_stream_index);
							if(pm_server_index <= 0)
								continue;
							app_log_debug("tp index is %d\n",pm_stream_index);
							memset(&StreamAdd, 0, sizeof(GxMsgProperty_NodeAdd));
						    StreamAdd.node_type = NODE_TP;
						    StreamAdd.tp_data.tp_net.index = pm_stream_index;
						    StreamAdd.tp_data.sat_id = server_id;
							if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_ADD, &StreamAdd))
							{
								app_log_error("========add net tp error\n");
							}
						}
					}
				}

			}
		}
		else
		{
			app_log_error("###app_json_parse: This object file not find server url\n");
		}
	}
	ret = GXCORE_SUCCESS;
out:
	cJSON_Delete(root_json);
	return ret;
}
#endif

#if DVB_FILE_SEARCH_SUPPORT
status_t app_add_ts_path_from_usb(char *ts_path)
{
#define FILE_MNT_PATH	"/mnt/usb"
#define FILE_MEDIA_PATH	"/media/sd"
	GxMsgProperty_NodeAdd ServerAdd = {0};
	GxMsgProperty_NodeAdd StreamAdd = {0};
	uint32_t pm_stream_index = 0;
	uint32_t pm_server_index = 0;
	char file_server_url[15] = {0};
	uint8_t head_len = 0;
	static uint8_t name_count = 0;
	uint32_t server_id = 0;

	if(ts_path == NULL)
	{
		return GXCORE_ERROR;
	}
	if(strncmp(ts_path, FILE_MNT_PATH, strlen(FILE_MNT_PATH) - 1) == 0)
	{
		head_len = strlen(FILE_MNT_PATH) + 4;
		snprintf(file_server_url, head_len,"%s", ts_path);
	}
	else if(strncmp(ts_path, FILE_MEDIA_PATH, strlen(FILE_MEDIA_PATH) - 1) == 0)
	{
		head_len = strlen(FILE_MEDIA_PATH) + 3;
		snprintf(file_server_url, head_len,"%s", ts_path);
	}
	else
		return GXCORE_ERROR;

	memset(&ServerAdd, 0, sizeof(GxMsgProperty_NodeAdd));
	ServerAdd.node_type = NODE_SAT;
	ServerAdd.sat_data.type = GXBUS_PM_SAT_NET;
	ServerAdd.sat_data.tuner = SERVER_TUNER_ID;
	GxBus_PmAddNetServerUrlToFile(file_server_url, &pm_server_index);
	if(pm_server_index > 0)
	{
		ServerAdd.sat_data.sat_net.index = pm_server_index;
	}
	snprintf((char*)ServerAdd.sat_data.sat_net.server_name, MAX_SAT_NAME, "New File%d", ++name_count);
	if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_ADD, &ServerAdd))
	{
		app_log_error("========add net sat error\n");
		return GXCORE_ERROR;
	}
	server_id = ServerAdd.sat_data.id;
	GxBus_PmAddNetStreamUrlToFile(ts_path + head_len - 1, &pm_stream_index);
	memset(&StreamAdd, 0, sizeof(GxMsgProperty_NodeAdd));
	StreamAdd.node_type = NODE_TP;
	StreamAdd.tp_data.tp_net.index = pm_stream_index;
	StreamAdd.tp_data.sat_id = server_id;
	if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_ADD, &StreamAdd))
	{
		app_log_error("========add net tp error\n");
		return GXCORE_ERROR;
	}
	return GXCORE_SUCCESS;
}

#endif
#endif
