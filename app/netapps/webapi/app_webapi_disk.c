#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

static void app_webapi_server_disk_read(struct mg_connection* nc, struct http_message* hm)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	HotplugPartitionList *partition_list = NULL;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;
	int i = 0;

	if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_FILE_BROWSER))
	{
		ret = WEBAPI_DEVICE_BUSY;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else
	{
		partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
		if(partition_list&&(partition_list->partition_num>0))
		{
			root = cJSON_CreateObject();
			cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
			json1 = cJSON_CreateObject();
			cJSON_AddItemToObject(root, WEBAPI_DISK_STR, json1);
			cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, partition_list->partition_num);
			json2 = cJSON_CreateArray();
			cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);

			for(i=0; i<partition_list->partition_num; i++)
			{
				json3 = cJSON_CreateObject();
				cJSON_AddItemToArray(json2, json3);
				cJSON_AddStringToObject(json3, WEBAPI_NAME_STR, partition_list->partition[i].partition_name);
				cJSON_AddStringToObject(json3, WEBAPI_PATH_STR, partition_list->partition[i].partition_entry);
			}

			out = cJSON_PrintUnformatted(root);
			cJSON_Delete(root);
			mg_printf(nc, WEBAPI_SERVER_HEADER);
			mg_send_http_chunk(nc, out, strlen(out));
			mg_send_http_chunk(nc, "", 0);
			free(out);
			ret = WEBAPI_SUCCESS;
		}
		else
		{
			ret = WEBAPI_DISK_NOT_EXIST;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
	}

	if(ret != WEBAPI_SUCCESS)
	{
		app_webapi_server_fail(nc, ret);
	}
}

void app_webapi_server_disk(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_disk_read(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
