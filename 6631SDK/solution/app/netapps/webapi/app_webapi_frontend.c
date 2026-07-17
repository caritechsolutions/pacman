#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern void app_webapi_make_frontend_data(cJSON *json);

static void app_webapi_server_frontend_read(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_FRONTEND_STR, json1);

	app_webapi_make_frontend_data(json1);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);

}

void app_webapi_server_frontend(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_frontend_read(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
