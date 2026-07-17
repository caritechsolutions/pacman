#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

typedef enum
{
	WEBAPI_SEARCH_ACTION_START = 0,
	WEBAPI_SEARCH_ACTION_STOP,
	WEBAPI_SEARCH_ACTION_UNKNOWN
}SearchActionType;

#if DEMOD_DVB_S
extern webapi_ret_t app_webapi_dvbs_search_start(
					WebapiSearchType search_type,
					cJSON *id,
					WebapiProgType prog_type,
					int fta,
					int nit);
extern webapi_ret_t app_webapi_dvbs_search_stop(void);
extern char *app_webapi_dvbs_get_search_data(void);
#endif

static SearchActionType app_webapi_get_search_action(char *action_str)
{
	SearchActionType action = WEBAPI_SEARCH_ACTION_UNKNOWN;

	if(action_str)
	{
		if(strcmp(action_str, "start") == 0)
		{
			action = WEBAPI_SEARCH_ACTION_START;
		}
		else if(strcmp(action_str, "stop") == 0)
		{
			action = WEBAPI_SEARCH_ACTION_STOP;
		}
	}

	return action;
}

static WebapiDemodType app_webapi_get_demod_type(char *demod_str)
{
	if(demod_str)
	{
#if DEMOD_DVB_S
		if(strcmp(demod_str, "dvbs") == 0)
		{
			return WEBAPI_DEMOD_DVBS;
		}
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
		if(strcmp(demod_str, "dvbt") == 0)
		{
			return WEBAPI_DEMOD_DVBT;
		}
#endif
#if DEMOD_DVB_C
		if(strcmp(demod_str, "dvbc") == 0)
		{
			return WEBAPI_DEMOD_DVBC;
		}
#endif
#if DEMOD_DTMB
		if(strcmp(demod_str, "dtmb") == 0)
		{
			return WEBAPI_DEMOD_DTMB;
		}
#endif
	}

	return WEBAPI_DEMOD_UNKNOWN;
}

static WebapiSearchType app_webapi_get_search_type(char *type_str)
{
	WebapiSearchType type = WEBAPI_SEARCH_UNKNOWN;

	if(type_str)
	{
		if(strcmp(type_str, "tp") == 0)
		{
			type = WEBAPI_SEARCH_TP;
		}
		else if(strcmp(type_str, "sat_preset") == 0)
		{
			type = WEBAPI_SEARCH_PRESET;
		}
		else if(strcmp(type_str, "sat_blind") == 0)
		{
			type = WEBAPI_SEARCH_BLIND;
		}
	}

	return type;
}


static WebapiProgType app_webapi_get_prog_type(char *type_str)
{
	WebapiProgType type = WEBAPI_PROG_ALL;

	if(type_str)
	{
		if(strcmp(type_str, "all") == 0)
		{
			type = WEBAPI_PROG_ALL;
		}
		else if(strcmp(type_str, "tv") == 0)
		{
			type = WEBAPI_PROG_TV;
		}
		else if(strcmp(type_str, "radio") == 0)
		{
			type = WEBAPI_PROG_RADIO;
		}
	}

	return type;
}

static webapi_ret_t app_webapi_search_get(struct mg_connection* nc, struct http_message* hm, WebapiDemodType demod_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	if(demod_type == WEBAPI_DEMOD_DVBS)
	{
#if DEMOD_DVB_S
		out = app_webapi_dvbs_get_search_data();
#endif
	}
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

static webapi_ret_t app_webapi_search_start(
					WebapiDemodType demod_type,
					WebapiSearchType search_type,
					cJSON *id,
					WebapiProgType prog_type,
					int fta,
					int nit)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	if(demod_type == WEBAPI_DEMOD_DVBS)
	{
#if DEMOD_DVB_S
		if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
		{
			ret = WEBAPI_DEVICE_BUSY;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
		else
		{
			ret = app_webapi_dvbs_search_start(search_type, id, prog_type, fta, nit);
		}
#endif
	}

	return ret;
}

static webapi_ret_t app_webapi_search_stop(WebapiDemodType demod_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	if(demod_type == WEBAPI_DEMOD_DVBS)
	{
#if DEMOD_DVB_S
		if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
		{
			ret = app_webapi_dvbs_search_stop();
		}
#endif
	}

	return ret;
}

static void app_webapi_server_search_read(struct mg_connection* nc, struct http_message* hm)
{
	char type[16];
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	WebapiDemodType demod_type = WEBAPI_DEMOD_UNKNOWN;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(type, 0, sizeof(type));
	if(mg_get_http_var(&hm->query_string, "type", type, sizeof(type))>0)
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
		demod_type = app_webapi_get_demod_type(type);
		if(demod_type != WEBAPI_DEMOD_UNKNOWN)
		{
			ret = app_webapi_search_get(nc, hm, demod_type);
		}
	}

	if(ret != WEBAPI_SUCCESS)
	{
		app_webapi_server_fail(nc, ret);
	}
}

static void app_webapi_server_search_write(struct mg_connection* nc, struct http_message* hm)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	cJSON *id = NULL;
	WebapiDemodType demod_type = WEBAPI_DEMOD_UNKNOWN;
	WebapiSearchType search_type = WEBAPI_SEARCH_UNKNOWN;
	SearchActionType action = WEBAPI_SEARCH_ACTION_UNKNOWN;
	WebapiProgType prog_type = WEBAPI_PROG_ALL;
	int fta = -1;
	int nit = -1;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//frontend type
				json1 = cJSON_GetObjectItem(root,WEBAPI_FRONTEND_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					demod_type = app_webapi_get_demod_type(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]frontend_type[%d]=%s\n",__FUNCTION__, __LINE__, demod_type, json1->valuestring);
				}

				//search_type
				json1 = cJSON_GetObjectItem(root,WEBAPI_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					search_type = app_webapi_get_search_type(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]type[%d]=%s\n",__FUNCTION__, __LINE__, search_type, json1->valuestring);
				}

				//action
				json1 = cJSON_GetObjectItem(root,WEBAPI_ACTION_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					action = app_webapi_get_search_action(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]search action[%d]=%s\n",__FUNCTION__, __LINE__, action, json1->valuestring);
				}

				//prog_type
				json1 = cJSON_GetObjectItem(root,WEBAPI_PROGTYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					prog_type = app_webapi_get_prog_type(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]prog_type[%d]=%s\n",__FUNCTION__, __LINE__, prog_type, json1->valuestring);
				}

				//id
				json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
				if((json1) && (json1->type == cJSON_Array))
				{
					id = json1;
				}

				//fta
				json1 = cJSON_GetObjectItem(root,WEBAPI_FTA_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					fta = json1->valueint;
					WEBAPI_PRINTF("%s[%d]fta=%d\n",__FUNCTION__, __LINE__, fta);
				}

				//nit
				json1 = cJSON_GetObjectItem(root,WEBAPI_NIT_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					nit = json1->valueint;
					WEBAPI_PRINTF("%s[%d]nit=%d\n",__FUNCTION__, __LINE__, nit);
				}


				if(action == WEBAPI_SEARCH_ACTION_START)
				{
					if(id&&(demod_type!=WEBAPI_DEMOD_UNKNOWN)
						&&(search_type!=WEBAPI_SEARCH_UNKNOWN))
					{
						ret = app_webapi_search_start(demod_type, search_type, id, prog_type, fta, nit);
					}
				}
				else if(action == WEBAPI_SEARCH_ACTION_STOP)
				{
					if(demod_type!=WEBAPI_DEMOD_UNKNOWN)
					{
						ret = app_webapi_search_stop(demod_type);
					}
				}
				cJSON_Delete(root);
			}
			free(data);
		}
		else
		{
			ret = WEBAPI_INTERNAL_ERR;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
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

void app_webapi_server_search(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_search_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_search_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif

