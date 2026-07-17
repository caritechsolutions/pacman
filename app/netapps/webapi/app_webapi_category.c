#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"
#include "app_channel_info.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

typedef enum
{
	WEBAPI_CATEGORY_TV = 0,
	WEBAPI_CATEGORY_RADIO,
	WEBAPI_CATEGORY_PROGRAM,
	WEBAPI_CATEGORY_FAV,
	WEBAPI_CATEGORY_UNKNOWN,
}WebapiCategoryType;

extern char *app_webapi_get_program_group_data(WebapiProgType prog_type);
extern char *app_webapi_get_fav_group_data(void);
extern webapi_ret_t app_webapi_set_fav_group_data(char *id, char *name);

static webapi_ret_t app_webapi_channel_category_read(struct mg_connection* nc, WebapiProgType prog_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	out = app_webapi_get_program_group_data(prog_type);
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

static webapi_ret_t app_webapi_fav_category_read(struct mg_connection* nc)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	out = app_webapi_get_fav_group_data();
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

static void app_webapi_server_category_read(struct mg_connection* nc, struct http_message* hm)
{
	char type[16];
	webapi_ret_t ret = WEBAPI_SUCCESS;
	WebapiCategoryType category_type = WEBAPI_CATEGORY_UNKNOWN;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(type, 0, sizeof(type));
	if(mg_get_http_var(&hm->query_string, "type", type, sizeof(type))>0)
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
		if(strcmp(type, "program") == 0)
		{
			category_type = WEBAPI_CATEGORY_PROGRAM;
		}
		else if(strcmp(type, "tv") == 0)
		{
			category_type = WEBAPI_CATEGORY_TV;
		}
		else if(strcmp(type, "radio") == 0)
		{
			category_type = WEBAPI_CATEGORY_RADIO;
		}
		else if(strcmp(type, "fav") == 0)
		{
			category_type = WEBAPI_CATEGORY_FAV;
		}

		if((category_type == WEBAPI_CATEGORY_TV)
			||((category_type == WEBAPI_CATEGORY_RADIO))
			||((category_type == WEBAPI_CATEGORY_PROGRAM)))
		{
			if((GXCORE_SUCCESS == app_channel_list_check_dialog())
				||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				ret = app_webapi_channel_category_read(nc, category_type);
			}
		}
		else if(category_type == WEBAPI_CATEGORY_FAV)
		{
			if((GXCORE_SUCCESS == app_channel_list_check_dialog())
				||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				ret = app_webapi_fav_category_read(nc);
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

static void app_webapi_server_category_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *id = NULL;
	char *name = NULL;
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
				}

				//name
				json1 = cJSON_GetObjectItem(root,WEBAPI_NAME_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					name = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]name=%s\n",__FUNCTION__, __LINE__, name);
				}

				if(id&&name)
				{
					if((GXCORE_SUCCESS == app_channel_list_check_dialog())
						||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
					{
						ret = WEBAPI_DEVICE_BUSY;
						WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
					}
					else
					{
						ret = app_webapi_set_fav_group_data(id, name);
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

void app_webapi_server_category(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_category_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_category_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
