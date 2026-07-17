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
extern char *app_webapi_get_dvbs_transponder_data(char *id);
extern webapi_ret_t app_webapi_set_dvbs_transponder_data(
					char *sat_id,
					char *id,
					int frequency,
					int polarization,
					int symbol_rate);
extern webapi_ret_t app_webapi_dvbs_transponder_delete(cJSON *json);

static webapi_ret_t app_webapi_dvbs_transponder_read(struct mg_connection* nc, char *id)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	out = app_webapi_get_dvbs_transponder_data(id);
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

#endif

static void app_webapi_server_transponder_read(struct mg_connection* nc, struct http_message* hm)
{
	char type[16];
	char id[16];
	webapi_ret_t ret = WEBAPI_SUCCESS;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(type, 0, sizeof(type));
	if((mg_get_http_var(&hm->query_string, "type", type, sizeof(type))>0)
		&&(mg_get_http_var(&hm->query_string, "id", id, sizeof(id))>0))
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
		if(strcmp(type, "dvbs") == 0)
		{
#if DEMOD_DVB_S
			if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				ret = app_webapi_dvbs_transponder_read(nc, id);
			}
#else
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

		}
		else if(strcmp(type, "dvbt") == 0)
		{
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

		}
		else if(strcmp(type, "dvbc") == 0)
		{
#if DEMOD_DVB_C
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

		}
		else if(strcmp(type, "dtmb") == 0)
		{
#if DEMOD_DTMB
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
			ret = WEBAPI_FUNCTION_PARAM_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

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

static void app_webapi_server_transponder_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *type = NULL;
#if DEMOD_DVB_S
	char *sat_id = NULL;
	char *id = NULL;
	int frequency = -1;
	int polarization = -1;
	int symbol_rate = -1;
#endif
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
				//type
				json1 = cJSON_GetObjectItem(root,WEBAPI_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					type = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
				}

#if DEMOD_DVB_S
				//sat id
				json1 = cJSON_GetObjectItem(root,WEBAPI_SATID_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					sat_id = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]sat_id=%s\n",__FUNCTION__, __LINE__, sat_id);
				}

				//id
				json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					id = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, id);
				}

				//frequency
				json1 = cJSON_GetObjectItem(root,WEBAPI_FREQUENCY_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					frequency = json1->valueint;
					WEBAPI_PRINTF("%s[%d]frequency=%d\n",__FUNCTION__, __LINE__, frequency);
				}

				//polarization
				json1 = cJSON_GetObjectItem(root,WEBAPI_POLARIZATION_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					polarization = json1->valueint;
					WEBAPI_PRINTF("%s[%d]polarization=%d\n",__FUNCTION__, __LINE__, polarization);
				}

				//symbol_rate
				json1 = cJSON_GetObjectItem(root,WEBAPI_SYMBOLRATE_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					symbol_rate = json1->valueint;
					WEBAPI_PRINTF("%s[%d]symbol_rate=%d\n",__FUNCTION__, __LINE__, symbol_rate);
				}
#endif
				if(type)
				{
					if(strcmp(type, "dvbs") == 0)
					{
#if DEMOD_DVB_S
						if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
						{
							ret = WEBAPI_DEVICE_BUSY;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
						else
						{
							ret = app_webapi_set_dvbs_transponder_data(sat_id, id, frequency, polarization, symbol_rate);
						}
#else
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

					}
					else if(strcmp(type, "dvbt") == 0)
					{
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

					}
					else if(strcmp(type, "dvbc") == 0)
					{
#if DEMOD_DVB_C
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

					}
					else if(strcmp(type, "dtmb") == 0)
					{
#if DEMOD_DTMB
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#else
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

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

static void app_webapi_server_transponder_delete(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *id = NULL;
	char *data = NULL;
	char *type = NULL;
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
				//type
				json1 = cJSON_GetObjectItem(root,WEBAPI_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					type = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
				}

				//id
				json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
				if((json1) && (json1->type == cJSON_Array))
				{
					id = json1;
				}

				if(type&&id)
				{
					if(strcmp(type, "dvbs") == 0)
					{
#if DEMOD_DVB_S
						if(GXCORE_SUCCESS == GUI_CheckDialog(WND_TP_LIST))
						{
							ret = WEBAPI_DEVICE_BUSY;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
						else
						{
							ret = app_webapi_dvbs_transponder_delete(id);
						}
#else
						ret = WEBAPI_FUNCTION_PARAM_ERR;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
#endif

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

void app_webapi_server_transponder(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_transponder_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_transponder_write(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "DELETE") == 0)
	{
		app_webapi_server_transponder_delete(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif

