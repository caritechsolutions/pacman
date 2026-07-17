#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

static void app_webapi_make_signal_data(uint32_t tuner, cJSON *json)
{
	unsigned int strength = 0;
	unsigned int quality = 0;
	int lock = 0;

	app_ioctl(tuner, FRONTEND_STRENGTH_GET, &strength);
	cJSON_AddNumberToObject(json, WEBAPI_STRENGTH_STR, (int)strength);
	app_ioctl(tuner, FRONTEND_QUALITY_GET, &quality);
	cJSON_AddNumberToObject(json, WEBAPI_QUALITY_STR, (int)quality);
	if(1 == GxFrontend_QueryStatus(tuner))
	{
		lock = 1;
	}
	cJSON_AddNumberToObject(json, WEBAPI_LOCK_STR, lock);
}

static void app_webapi_server_signal_read(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;
	char buffer[32];
	uint32_t tuner_id = 0;

	tuner_id = (uint32_t)g_AppPlayOps.normal_play.tuner;
	if(mg_get_http_var(&hm->query_string, "tuner", buffer, sizeof(buffer))>0)
	{
		tuner_id = (uint32_t)strtoul(buffer, 0, 10);
		WEBAPI_PRINTF("%s[%d]tuner_id=%u\n",__FUNCTION__, __LINE__, tuner_id);
	}

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_SIGNAL_STR, json1);
	app_webapi_make_signal_data(tuner_id, json1);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);

}

void app_webapi_server_signal(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_signal_read(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
