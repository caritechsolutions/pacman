#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"
#include "app_channel_info.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include "full_screen.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern char *app_webapi_get_program_data(WebapiProgType prog_type, uint32_t category_id, uint32_t page, uint32_t limit);
extern webapi_ret_t app_webapi_set_program_data(cJSON *json);
extern status_t app_get_current_program(GxMsgProperty_NodeByPosGet *node);

static webapi_ret_t app_webapi_category_program_read(
							struct mg_connection* nc,
							WebapiProgType prog_type,
							uint32_t category_id,
							uint32_t page,
							uint32_t limit)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]prog_type=%d,category_id=%u,page=%u, limit=%u\n",__FUNCTION__, __LINE__, prog_type, category_id, page, limit);
	out = app_webapi_get_program_data(prog_type, category_id, page, limit);
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

static void app_webapi_server_program_read(struct mg_connection* nc, struct http_message* hm)
{
	char buffer[32];
	uint32_t category_id = 0;
	uint32_t page = 0;
	uint32_t limit = 0;
	webapi_ret_t ret = WEBAPI_SUCCESS;
	WebapiProgType prog_type = WEBAPI_PROG_ALL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(buffer, 0, sizeof(buffer));
	if(mg_get_http_var(&hm->query_string, "type", buffer, sizeof(buffer))>0)
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, buffer);
		if(strcmp(buffer, "tv") == 0)
		{
			prog_type = WEBAPI_PROG_TV;
		}
		else if(strcmp(buffer, "radio") == 0)
		{
			prog_type = WEBAPI_PROG_RADIO;
		}
		else
		{
			prog_type = WEBAPI_PROG_ALL;
		}
		WEBAPI_PRINTF("%s[%d]prog_type=%d\n",__FUNCTION__, __LINE__, prog_type);
	}

	memset(buffer, 0, sizeof(buffer));
	if(mg_get_http_var(&hm->query_string, "category_id", buffer, sizeof(buffer))>0)
	{
		category_id = (uint32_t)strtoul(buffer, 0, 10);
		WEBAPI_PRINTF("%s[%d]category_id=%u\n",__FUNCTION__, __LINE__, category_id);
	}

	if(mg_get_http_var(&hm->query_string, "page", buffer, sizeof(buffer))>0)
	{
		page = (uint32_t)strtoul(buffer, 0, 10);
		WEBAPI_PRINTF("%s[%d]page=%u\n",__FUNCTION__, __LINE__, page);
	}
	if(mg_get_http_var(&hm->query_string, "limit", buffer, sizeof(buffer))>0)
	{
		limit = (uint32_t)strtoul(buffer, 0, 10);
		WEBAPI_PRINTF("%s[%d]limit=%u\n",__FUNCTION__, __LINE__, limit);
	}

	if(category_id>0)
	{
		if((GXCORE_SUCCESS == app_channel_list_check_dialog())
			||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
		{
			ret = WEBAPI_DEVICE_BUSY;
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
		else
		{
			ret = app_webapi_category_program_read(nc, prog_type, category_id, page, limit);
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

static void app_webapi_server_program_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	char *data = NULL;
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
				if((GXCORE_SUCCESS == app_channel_list_check_dialog())
					||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
				{
					ret = WEBAPI_DEVICE_BUSY;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
				else
				{
					ret = app_webapi_set_program_data(root);
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

static uint32_t app_webapi_get_playing_program_id(void)
{
	int sat_id = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	if(g_AppPlayOps.running&&(GXCORE_SUCCESS == app_get_current_program(&node)))
	{
		sat_id = node.prog_data.id;
	}

	return sat_id;
}

static webapi_ret_t app_webapi_program_delete(cJSON *json)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	GxMsgProperty_NodeDelete node= {0};
	cJSON *json1 = NULL;
	int prog_num = 0;
	uint32_t cur_play_prog = 0;
	int need_refresh_play = 0;
	int i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	prog_num = cJSON_GetArraySize(json);
	if(prog_num>0)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeDelete));
		node.node_type = NODE_PROG;
		node.id_array = GxCore_Malloc(prog_num*sizeof(uint32_t));
		if(node.id_array)
		{
			cur_play_prog = app_webapi_get_playing_program_id();
			for(i=0; i<prog_num; i++)
			{
				json1 = cJSON_GetArrayItem(json, i);
				if(json1 && (json1->type == cJSON_String))
				{
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, json1->valuestring);
					node.id_array[node.num] = (uint32_t)strtoul(json1->valuestring, 0, 10);
					if((cur_play_prog>0)&&(node.id_array[node.num] == cur_play_prog))
					{
						need_refresh_play = 1;
					}
					node.num++;
				}
			}
			if(node.num>0)
			{
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_DELETE, &node))
				{
					ret = WEBAPI_SUCCESS;
					if(need_refresh_play)
					{
						g_AppPlayOps.program_stop();
						g_AppPvrOps.tms_delete(&g_AppPvrOps);
						g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
						g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
						if(g_AppPlayOps.normal_play.play_total != 0)
						{
							app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
							g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
							if(g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
								g_AppFullArb.tip = FULL_STATE_LOCKED;
							else
								g_AppFullArb.tip = FULL_STATE_DUMMY;
						}
					}
					else if(g_AppPlayOps.running)
					{
						g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
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
			GxCore_Free(node.id_array);
		}
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	return ret;
}

static void app_webapi_server_program_delete(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *id = NULL;
	char *data = NULL;
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				if((GXCORE_SUCCESS == app_channel_list_check_dialog())
					||(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
				{
					ret = WEBAPI_DEVICE_BUSY;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
				else
				{
					//id
					json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
					if((json1) && (json1->type == cJSON_Array))
					{
						id = json1;
					}

					if(id)
					{
						ret = app_webapi_program_delete(id);
					}
				}
				cJSON_Delete(root);
			}
			free(data);
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

void app_webapi_server_program(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_program_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_program_write(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "DELETE") == 0)
	{
		app_webapi_server_program_delete(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
