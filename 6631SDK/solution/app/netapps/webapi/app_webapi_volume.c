#include "app.h"
#include "app_send_msg.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern void app_webapi_volume_get(int *current, int *max);

static void app_webapi_server_volume_read(struct mg_connection* nc, struct http_message* hm)
{
	int current = -1;
	int max = 0;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;

	app_webapi_volume_get(&current, &max);
	WEBAPI_PRINTF("%s[%d/%d]\n",__FUNCTION__, current, max);
	if((current>=0)&&(max>0)&&(current<=max))
	{
		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
		json1 = cJSON_CreateObject();
		cJSON_AddItemToObject(root, WEBAPI_VOLUME_STR, json1);
		cJSON_AddNumberToObject(json1, WEBAPI_CURRENT_STR, current);
		cJSON_AddNumberToObject(json1, WEBAPI_MAX_STR, max);
		out = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);

		mg_printf(nc, WEBAPI_SERVER_HEADER);
		mg_send_http_chunk(nc, out, strlen(out));
		mg_send_http_chunk(nc, "", 0);

		free(out);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_INTERNAL_ERR));
		app_webapi_server_fail(nc, WEBAPI_INTERNAL_ERR);
	}
}

static void app_webapi_server_volume_write(struct mg_connection* nc, struct http_message* hm)
{
	AppMsg_WebapiUIControl ui_msg = {0};
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	int value = -1;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				json1 = cJSON_GetObjectItem(root,"value");
				if((json1) && (json1->type == cJSON_Number))
				{
					value = json1->valueint;
					WEBAPI_PRINTF("value=%d\n",value);
				}
				cJSON_Delete(root);
			}
			free(data);
		}
		if((value>=0)&&(value<100))
		{
			ui_msg.type = WEBAPI_MSG_SET_VOLUME;
			ui_msg.result = value;
			app_send_msg_exec(APPMSG_WEBAPI_UI_CONTROL, &ui_msg);
			app_webapi_server_success(nc);
		}
		else
		{
			WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
			app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
		}
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
		app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
	}
}

void app_webapi_server_volume(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_volume_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_volume_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
