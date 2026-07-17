#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

#define WEBAPI_PATH_SIZE (4096)

extern char *app_webapi_get_file_data(char *path, WebapiMediaType media_type, uint32_t page, uint32_t limit);

static int app_webapi_check_disk(char *path)
{
	int exist = 0;
	int i = 0;
	HotplugPartitionList *partition_list = NULL;

	partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
	if(partition_list&&(partition_list->partition_num>0))
	{
		for(i=0; i<partition_list->partition_num; i++)
		{
			if(strncmp(path, partition_list->partition[i].partition_entry,strlen(partition_list->partition[i].partition_entry)) == 0)
			{
				exist = 1;
				break;
			}
		}
	}

	return exist;
}

static webapi_ret_t app_webapi_file_read(
							struct mg_connection* nc,
							char *path,
							WebapiMediaType media_type,
							uint32_t page,
							uint32_t limit)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]path=%s,media_type=%d,page=%u, limit=%u\n",__FUNCTION__, __LINE__, path, media_type, page, limit);
	out = app_webapi_get_file_data(path, media_type, page, limit);
	if(out)
	{
		mg_printf(nc, WEBAPI_SERVER_HEADER);
		mg_send_http_chunk(nc, out, strlen(out));
		mg_send_http_chunk(nc, "", 0);
		free(out);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

static void app_webapi_server_file_read(struct mg_connection* nc, struct http_message* hm)
{
	char *path = NULL;
	char buffer[32];
	uint32_t page = 0;
	uint32_t limit = 0;
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	WebapiMediaType media_type = WEBAPI_MEDIA_UNKNOWN;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	path = GxCore_Calloc(1, WEBAPI_PATH_SIZE);

	if(path)
	{
		memset(path, 0, WEBAPI_PATH_SIZE);
		if(mg_get_http_var(&hm->query_string, "path", path, WEBAPI_PATH_SIZE)>0)
		{
			WEBAPI_PRINTF("%s[%d]path=%s\n",__FUNCTION__, __LINE__, path);
		}

		memset(buffer, 0, sizeof(buffer));
		if(mg_get_http_var(&hm->query_string, "media_type", buffer, sizeof(buffer))>0)
		{
			media_type = app_webapi_get_media_type(buffer);
			WEBAPI_PRINTF("%s[%d]media_type[%d]=%s\n",__FUNCTION__, __LINE__, media_type, buffer);
		}

		if(mg_get_http_var(&hm->query_string, "page", buffer, sizeof(buffer))>0)
		{
			page = (uint32_t)strtoul(buffer, 0, 10);
			WEBAPI_PRINTF("%s[%d]page=%u\n",__FUNCTION__, __LINE__, page);
		}

		if(mg_get_http_var(&hm->query_string, "limit", buffer, sizeof(buffer))>0)
		{
			limit = (uint32_t)strtoul(buffer, 0, 10);
			WEBAPI_PRINTF("%s[%d]limit=%u\n",__FUNCTION__, __LINE__, limit);
		}

		if((strlen(path)>0)&&(path[0] == '/')&&(media_type!=WEBAPI_MEDIA_UNKNOWN))
		{
			if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_FILE_BROWSER))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				if(app_webapi_check_disk(path)>0)
				{
					ret = app_webapi_file_read(nc, path, media_type, page, limit);
				}
				else
				{
					ret = WEBAPI_DISK_NOT_EXIST;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
			}
		}
		else
		{
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
	}

	if(ret != WEBAPI_SUCCESS)
	{
		app_webapi_server_fail(nc, ret);
	}
	APP_FREE(path);
}

void app_webapi_server_file(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_file_read(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
