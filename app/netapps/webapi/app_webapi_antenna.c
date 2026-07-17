#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"


#if DEMOD_DVB_S
extern cJSON *app_webapi_make_antenna_data(uint32_t sat_id);
extern int app_webapi_set_antenna_data(
					uint32_t sat_id,
					int power_sel,
					int freq_sel,
					int s22k_sel,
					int diseqc10_sel,
					int diseqc11_sel);

static webapi_ret_t app_webapi_antenna_read(struct mg_connection* nc, uint32_t sat_id)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	cJSON *root = NULL;
	cJSON *json = NULL;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	json = app_webapi_make_antenna_data(sat_id);
	if(json)
	{
		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
		cJSON_AddItemToObject(root, WEBAPI_ANTENNA_STR, json);
		out = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);

		ret = WEBAPI_SUCCESS;
		mg_printf(nc, WEBAPI_SERVER_HEADER);
		mg_send_http_chunk(nc, out, strlen(out));
		mg_send_http_chunk(nc, "", 0);
		free(out);
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	return ret;
}

static void app_webapi_server_antenna_read(struct mg_connection* nc, struct http_message* hm)
{
	char buffer[16];
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	uint32_t id = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(buffer, 0, sizeof(buffer));
	if(mg_get_http_var(&hm->query_string, "id", buffer, sizeof(buffer))>0)
	{
		WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, buffer);
		id = (uint32_t)strtoul(buffer, 0, 10);
		if(id>0)
		{
			if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				ret = app_webapi_antenna_read(nc, id);
			}
		}
		else
		{
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	if(ret != WEBAPI_SUCCESS)
	{
		app_webapi_server_fail(nc, ret);
	}
}

static void app_webapi_server_antenna_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *id = NULL;
	uint32_t sat_id = 0;
	int power_sel = -1;
	int freq_sel = -1;
	int s22k_sel = -1;
	int diseqc10_sel = -1;
	int diseqc11_sel = -1;
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//id
				json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					id = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, id);
					sat_id = (uint32_t)strtoul(id, 0, 10);
				}

				//lnb power
				json1 = cJSON_GetObjectItem(root,WEBAPI_LNB_POWER_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					power_sel = json1->valueint;
					WEBAPI_PRINTF("%s[%d]power_sel=%d\n",__FUNCTION__, __LINE__, power_sel);
				}

				//lnb frequency
				json1 = cJSON_GetObjectItem(root,WEBAPI_LNB_FREQUENCY_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					freq_sel = json1->valueint;
					WEBAPI_PRINTF("%s[%d]freq_sel=%d\n",__FUNCTION__, __LINE__, freq_sel);
				}

				//22k
				json1 = cJSON_GetObjectItem(root,WEBAPI_SWITCH_22K_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					s22k_sel = json1->valueint;
					WEBAPI_PRINTF("%s[%d]s22k_sel=%d\n",__FUNCTION__, __LINE__, s22k_sel);
				}

				//diseqc1.0
				json1 = cJSON_GetObjectItem(root,WEBAPI_DISEQC10_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					diseqc10_sel = json1->valueint;
					WEBAPI_PRINTF("%s[%d]diseqc10_sel=%d\n",__FUNCTION__, __LINE__, diseqc10_sel);
				}

				//diseqc1.1
				json1 = cJSON_GetObjectItem(root,WEBAPI_DISEQC11_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					diseqc11_sel = json1->valueint;
					WEBAPI_PRINTF("%s[%d]diseqc11_sel=%d\n",__FUNCTION__, __LINE__, diseqc11_sel);
				}

				if((sat_id>0)&&(power_sel>=0)&&(freq_sel>=0)&&(s22k_sel>=0)&&(diseqc10_sel>=0)&&(diseqc11_sel>=0))
				{
					if(GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
					{
						ret = WEBAPI_DEVICE_BUSY;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
					}
					else
					{
						if(0 == app_webapi_set_antenna_data(sat_id, power_sel, freq_sel, s22k_sel, diseqc10_sel, diseqc11_sel))
						{
							ret = WEBAPI_SUCCESS;
						}
						else
						{
							ret = WEBAPI_FUNCTION_PARAM_ERR;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
					}
				}
				else
				{
					ret = WEBAPI_FUNCTION_PARAM_ERR;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
				cJSON_Delete(root);
			}
			else
			{
				ret = WEBAPI_FUNCTION_PARAM_ERR;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			free(data);
		}
		else
		{
			ret = WEBAPI_INTERNAL_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	if(ret == WEBAPI_SUCCESS)
	{
		app_webapi_server_success(nc);
	}
	else
	{
		app_webapi_server_fail(nc, ret);
	}
}
#endif

void app_webapi_server_antenna(struct mg_connection* nc, struct http_message* hm)
{
#if DEMOD_DVB_S
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_antenna_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_antenna_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
#else
	WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_NOT_EXIST));
	app_webapi_server_fail(nc, WEBAPI_FUNCTION_NOT_EXIST);
#endif
}

#endif
