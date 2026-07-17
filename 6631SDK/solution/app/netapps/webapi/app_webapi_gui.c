#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern int app_webapi_osd_setting_busy(void);
extern void app_webapi_get_osd_settings(int *trans, int *timeout, int *freeze);
extern void app_webapi_make_osd_data(cJSON *trans, cJSON *timeout, cJSON *freeze);
extern void app_webapi_save_osd_settings(int trans, int timeout, int freeze);

extern int app_webapi_color_setting_busy(void);
extern void app_webapi_get_color_settings(int *bright, int *chroma, int *contrast);
extern void app_webapi_make_color_data(cJSON *bright, cJSON *chroma, cJSON *contrast);
extern void app_webapi_save_color_settings(int bright, int chroma, int contrast);

static void app_webapi_server_gui_read(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json_trans = NULL;
	cJSON *json_timeout = NULL;
	cJSON *json_freeze = NULL;
	cJSON *json_bright = NULL;
	cJSON *json_chroma = NULL;
	cJSON *json_contrast = NULL;
	char *out = NULL;
	int trans = 0;
	int timeout = 0;
	int freeze = 0;
	int bright = 0;
	int chroma = 0;
	int contrast = 0;

	app_webapi_get_osd_settings(&trans, &timeout, &freeze);
	app_webapi_get_color_settings(&bright, &chroma, &contrast);

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_GUI_STR, json1);

	//transparency
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_TRANSPARENCY_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, trans);
	json_trans = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_trans);

	//timeout
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_TIMEOUT_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, timeout);
	json_timeout = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_timeout);

	//channel switch
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_SWITCH_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, freeze);
	json_freeze = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_freeze);

	//brightness
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_BRIGHTNESS_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, bright);
	json_bright = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_bright);

	//chroma
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_CHROMA_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, chroma);
	json_chroma = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_chroma);

	//contrast
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_CONTRAST_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, contrast);
	json_contrast = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_contrast);

	app_webapi_make_osd_data(json_trans, json_timeout, json_freeze);
	app_webapi_make_color_data(json_bright, json_chroma, json_contrast);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);
}

static void app_webapi_server_gui_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	int trans = -1;
	int timeout = -1;
	int freeze = -1;
	int bright = -1;
	int chroma = -1;
	int contrast = -1;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//transparency
				json1 = cJSON_GetObjectItem(root,WEBAPI_TRANSPARENCY_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					trans = json1->valueint;
					WEBAPI_PRINTF("%s[trans=%d]\n",__FUNCTION__, trans);
				}

				//timeout
				json1 = cJSON_GetObjectItem(root,WEBAPI_TIMEOUT_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					timeout = json1->valueint;
					WEBAPI_PRINTF("%s[timeout=%d]\n",__FUNCTION__, timeout);
				}

				//channel_switch
				json1 = cJSON_GetObjectItem(root,WEBAPI_SWITCH_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					freeze = json1->valueint;
					WEBAPI_PRINTF("%s[freeze=%d]\n",__FUNCTION__, freeze);
				}

				//brightness
				json1 = cJSON_GetObjectItem(root,WEBAPI_BRIGHTNESS_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					bright = json1->valueint;
					WEBAPI_PRINTF("%s[bright=%d]\n",__FUNCTION__, bright);
				}

				//chroma
				json1 = cJSON_GetObjectItem(root,WEBAPI_CHROMA_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					chroma = json1->valueint;
					WEBAPI_PRINTF("%s[chroma=%d]\n",__FUNCTION__, chroma);
				}

				//contrast
				json1 = cJSON_GetObjectItem(root,WEBAPI_CONTRAST_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					contrast = json1->valueint;
					WEBAPI_PRINTF("%s[contrast=%d]\n",__FUNCTION__, contrast);
				}

				cJSON_Delete(root);
			}
			free(data);
		}
		if(app_webapi_osd_setting_busy()||app_webapi_color_setting_busy())
		{
			WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_DEVICE_BUSY));
			app_webapi_server_fail(nc, WEBAPI_DEVICE_BUSY);
		}
		else
		{
			app_webapi_save_osd_settings(trans, timeout, freeze);
			app_webapi_save_color_settings(bright, chroma, contrast);
			app_webapi_server_success(nc);
		}
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
		app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
	}
}

void app_webapi_server_gui(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_gui_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_gui_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
