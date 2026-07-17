#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern int app_webapi_av_setting_busy(void);
extern void app_webapi_get_av_settings(int *standard, int *ratio, int *aspect, int *sound, int *spdif, int *ad);
extern void app_webapi_make_av_data(cJSON *standard, cJSON *ratio, cJSON *aspect, cJSON *sound, cJSON *spd, cJSON *ad);
extern int app_webapi_save_av_settings(int standard, int ratio, int aspect, int sound, int spdif, int ad);


static void app_webapi_server_av_read(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json_standard = NULL;
	cJSON *json_ratio = NULL;
	cJSON *json_aspect = NULL;
	cJSON *json_sound = NULL;
	cJSON *json_spdif = NULL;
	cJSON *json_ad = NULL;
	char *out = NULL;
	int standard = 0;
	int ratio = 0;
	int aspect = 0;
	int sound = 0;
	int spdif = 0;
	int ad = 0;

	app_webapi_get_av_settings(&standard, &ratio, &aspect, &sound, &spdif, &ad);

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_AV_STR, json1);

	//tv standard
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_STANDARD_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, standard);
	json_standard = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_standard);

	//tv ratio
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_RATIO_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, ratio);
	json_ratio = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_ratio);

	//aspect mode
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_ASPECT_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, aspect);
	json_aspect = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_aspect);

	//sound mode
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_SOUND_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, sound);
	json_sound = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_sound);

	//spdif
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_SPDIF_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, spdif);
	json_spdif = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_spdif);

	//ad mode
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_AD_STR, json2);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, ad);
	json_ad = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json_ad);

	app_webapi_make_av_data(json_standard, json_ratio, json_aspect, json_sound, json_spdif, json_ad);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);
}

static void app_webapi_server_av_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	int tv_standard = -1;
	int tv_ratio = -1;
	int aspect_mode = -1;
	int sound_mode = -1;
	int spdif = -1;
	int ad_mode = -1;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//tv_standard
				json1 = cJSON_GetObjectItem(root,WEBAPI_STANDARD_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					tv_standard = json1->valueint;
					WEBAPI_PRINTF("%s[tv_standard=%d]\n",__FUNCTION__, tv_standard);
				}

				//tv_ratio
				json1 = cJSON_GetObjectItem(root,WEBAPI_RATIO_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					tv_ratio = json1->valueint;
					WEBAPI_PRINTF("%s[tv_ratio=%d]\n",__FUNCTION__, tv_ratio);
				}

				//aspect_mode
				json1 = cJSON_GetObjectItem(root,WEBAPI_ASPECT_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					aspect_mode = json1->valueint;
					WEBAPI_PRINTF("%s[aspect_mode=%d]\n",__FUNCTION__, aspect_mode);
				}

				//sound_mode
				json1 = cJSON_GetObjectItem(root,WEBAPI_SOUND_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					sound_mode = json1->valueint;
					WEBAPI_PRINTF("%s[sound_mode=%d]\n",__FUNCTION__, sound_mode);
				}

				//spdif
				json1 = cJSON_GetObjectItem(root,WEBAPI_SPDIF_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					spdif = json1->valueint;
					WEBAPI_PRINTF("%s[spdif=%d]\n",__FUNCTION__, spdif);
				}

				//ad_mode
				json1 = cJSON_GetObjectItem(root,WEBAPI_AD_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					ad_mode = json1->valueint;
					WEBAPI_PRINTF("%s[ad_mode=%d]\n",__FUNCTION__, ad_mode);
				}

				cJSON_Delete(root);
			}
			free(data);
		}
		if(app_webapi_av_setting_busy())
		{
			WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_DEVICE_BUSY));
			app_webapi_server_fail(nc, WEBAPI_DEVICE_BUSY);
		}
		else
		{
			if(app_webapi_save_av_settings(tv_standard, tv_ratio, aspect_mode, sound_mode, spdif, ad_mode)>0)
			{
				app_webapi_server_success(nc);
			}
			else
			{
				WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
				app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
			}
		}
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
		app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
	}
}

void app_webapi_server_av(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_av_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_av_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
