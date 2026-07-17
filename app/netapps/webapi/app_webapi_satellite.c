#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include "full_screen.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"


#if DEMOD_DVB_S
extern int app_webapi_get_dvbs_demods(int *tuners);
extern status_t app_get_current_program(GxMsgProperty_NodeByPosGet *node);

static void app_webapi_satellite_dvbs_read(struct mg_connection* nc)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	cJSON *json4 = NULL;
	char *out = NULL;
	int i = 0;
	int j = 0;
	int total = 0;
	GxMsgProperty_NodeNumGet sat_num = {0};
	GxMsgProperty_NodeByPosGet node_sat = {0};
	char buffer[16];
	char sat_name[MAX_SAT_NAME+1];
	int tuner_num = 0;
	int sat_position = 0;
	int tuners[NIM_MODULE_NUM];

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_SATELLITE_STR, json1);
	json2 = cJSON_CreateArray();
	cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);

	memset(&sat_num, 0, sizeof(GxMsgProperty_NodeNumGet));
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &sat_num);

	for(i = 0; i < sat_num.node_num; i++)
	{
		memset(&node_sat, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node_sat.node_type = NODE_SAT;
		node_sat.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_sat);
		if(GXBUS_PM_SAT_S == node_sat.sat_data.type)
		{
			json3 = cJSON_CreateObject();
			cJSON_AddItemToArray(json2, json3);
			snprintf(buffer, sizeof(buffer)-1, "%d", node_sat.sat_data.id);
			cJSON_AddStringToObject(json3, WEBAPI_ID_STR, buffer);
			cJSON_AddStringToObject(json3, WEBAPI_TYPE_STR, WEBAPI_DVBS_STR);
			cJSON_AddNumberToObject(json3, WEBAPI_TUNER_STR, node_sat.sat_data.tuner);

			memset(tuners, 0, sizeof(tuners));
			tuner_num = app_webapi_get_dvbs_demods(tuners);
			if(tuner_num>0)
			{
				json4 = cJSON_CreateArray();
				cJSON_AddItemToObject(json3, WEBAPI_TUNER_OPTIONS_STR, json4);
				for(j=0; j<tuner_num; j++)
				{
					cJSON_AddItemToArray(json4, cJSON_CreateNumber(tuners[j]));
				}
			}

			memset(sat_name, 0, sizeof(sat_name));
			memcpy(sat_name, (char *)node_sat.sat_data.sat_s.sat_name, MAX_SAT_NAME);
			cJSON_AddStringToObject(json3, WEBAPI_NAME_STR, sat_name);

			sat_position = node_sat.sat_data.sat_s.longitude;
			if(node_sat.sat_data.sat_s.longitude_direct == GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST)
			{
				sat_position *= -1;
			}
			cJSON_AddNumberToObject(json3, WEBAPI_POSITION_STR, sat_position);

			total++;
		}
	}
	cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, total);

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);
}

static int app_webapi_is_satellite_exist(char *sat_name)
{
	uint32_t i = 0;
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node;

	node_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
	for(i = 0; i < node_num.node_num; i++)
	{
		node.node_type = NODE_SAT;
		node.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		if(strcmp((char*)node.sat_data.sat_s.sat_name, sat_name) == 0)
		{
			return 1;
		}
	}

	return 0;
}

static webapi_ret_t app_webapi_satellite_dvbs_write(
					struct mg_connection* nc,
					char *id,
					int tuner,
					char *name,
					int position)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	GxMsgProperty_NodeByIdGet sat_node = {0};
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeAdd node_add = {0};
	GxMsgProperty_NodeModify node_modify = {0};
	GxBusPmDataSatLongitudeDirect direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
	int tuner_num = 0;
	int tuners[NIM_MODULE_NUM];

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	tuner_num = app_webapi_get_dvbs_demods(tuners);
	if((tuner>=0)&&(tuner<tuner_num)
		&&name&&(strlen(name)>0)
		&&(position>=-1800)&&(position<=1800))
	{
		if(position>0)
			direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_EAST;
		else
			direct = GXBUS_PM_SAT_LONGITUDE_DIRECT_WEST;
		if(id)
		{
			sat_node.node_type = NODE_SAT;
			sat_node.id = (uint32_t)strtoul(id, 0, 10);
			if((GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_node))
				&&(sat_node.sat_data.type == GXBUS_PM_SAT_S))
			{
				memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
				node_modify.node_type = NODE_SAT;
				memcpy(&node_modify.sat_data, &sat_node.sat_data, sizeof(GxBusPmDataSat));
				node_modify.sat_data.tuner = tuner;
				memset((char *)node_modify.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
				memcpy((char *)node_modify.sat_data.sat_s.sat_name, name, MAX_SAT_NAME - 1);
				node_modify.sat_data.sat_s.longitude = abs(position);
				node_modify.sat_data.sat_s.longitude_direct = direct;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
				{
					ret = WEBAPI_SUCCESS;
					WEBAPI_PRINTF("%s[%d]modify success\n",__FUNCTION__, __LINE__);
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
		}
		else //new satellite
		{
			node_num.node_type = NODE_SAT;
			app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);
			if(node_num.node_num >= SYS_MAX_SAT)
			{
				ret = WEBAPI_SATELLITE_LIMIT_ERR;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else if((app_webapi_is_satellite_exist(name)>0))
			{
				ret = WEBAPI_SATELLITE_EXIST;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				memset(&node_add, 0, sizeof(GxMsgProperty_NodeAdd));
				node_add.node_type = NODE_SAT;
				node_add.sat_data.type = GXBUS_PM_SAT_S;
				node_add.sat_data.tuner = tuner;
				node_add.sat_data.sat_s.lnb1 = 5150;
				node_add.sat_data.sat_s.lnb2 = 5750;
				node_add.sat_data.sat_s.lnb_power = GXBUS_PM_SAT_LNB_POWER_ON;
				node_add.sat_data.sat_s.switch_22K = GXBUS_PM_SAT_22K_OFF;
				node_add.sat_data.sat_s.diseqc11 = 0;
				node_add.sat_data.sat_s.diseqc12_pos = DISEQ12_USELESS_ID;
				node_add.sat_data.sat_s.diseqc_version = GXBUS_PM_SAT_DISEQC_1_0;
				node_add.sat_data.sat_s.diseqc10 = 0;
				node_add.sat_data.sat_s.switch_12V = GXBUS_PM_SAT_12V_OFF;
				node_add.sat_data.sat_s.longitude_direct = direct;
				node_add.sat_data.sat_s.reserved = 0;
				node_add.sat_data.sat_s.longitude = abs(position);
				memset((char *)node_add.sat_data.sat_s.sat_name, 0, MAX_SAT_NAME);
				memcpy((char *)node_add.sat_data.sat_s.sat_name, name, MAX_SAT_NAME -1);
#if UNICABLE_SUPPORT
				{
					int index = 0;
					//centre fre
					for(index=0;index<MAX_IF_INDEX_NUM;index++)
					{
						node_add.sat_data.sat_s.unicable_para.centre_fre[index] = 0;
					}
                    node_add.sat_data.sat_s.unicable_para.type = GXBUS_PM_SAT_UNICABLE_OFF;
					node_add.sat_data.sat_s.unicable_para.lnb_fre_index = 0;
					node_add.sat_data.sat_s.unicable_para.if_channel = 0;
					node_add.sat_data.sat_s.unicable_para.sat_pos = 0;
				}
#endif
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_ADD, &node_add))
				{
					ret = WEBAPI_SUCCESS;
					WEBAPI_PRINTF("%s[%d]add success\n",__FUNCTION__, __LINE__);
				}
				else
				{
					ret = WEBAPI_INTERNAL_ERR;
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
				}
			}
		}
	}
	else
	{
		ret = WEBAPI_FUNCTION_PARAM_ERR;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}

	return ret;
}

static uint32_t app_webapi_get_playing_satellite_id(void)
{
	int sat_id = 0;
	GxMsgProperty_NodeByPosGet node = {0};

	if(g_AppPlayOps.running&&(GXCORE_SUCCESS == app_get_current_program(&node)))
	{
		sat_id = node.prog_data.sat_id;
	}

	return sat_id;
}

static webapi_ret_t app_webapi_satellite_dvbs_delete(
					cJSON *json)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	GxMsgProperty_NodeDelete node= {0};
	cJSON *json1 = NULL;
	int sat_num = 0;
	uint32_t cur_play_sat = 0;
	int need_refresh_play = 0;
	int i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	sat_num = cJSON_GetArraySize(json);
	if(sat_num>0)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeDelete));
		node.node_type = NODE_SAT;
		node.id_array = GxCore_Malloc(sat_num*sizeof(uint32_t));
		if(node.id_array)
		{
			cur_play_sat = app_webapi_get_playing_satellite_id();
			for(i=0; i<sat_num; i++)
			{
				json1 = cJSON_GetArrayItem(json, i);
				if(json1 && (json1->type == cJSON_String))
				{
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, json1->valuestring);
					node.id_array[node.num] = (uint32_t)strtoul(json1->valuestring, 0, 10);
					if((cur_play_sat>0)&&(node.id_array[node.num] == cur_play_sat))
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

#endif

static void app_webapi_server_satellite_read(struct mg_connection* nc, struct http_message* hm)
{
	char type[16];
	webapi_ret_t ret = WEBAPI_SUCCESS;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(type, 0, sizeof(type));
	if(mg_get_http_var(&hm->query_string, "type", type, sizeof(type))>0)
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
		if(strcmp(type, "dvbs") == 0)
		{
#if DEMOD_DVB_S
			if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
			{
				ret = WEBAPI_DEVICE_BUSY;
				WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
			}
			else
			{
				app_webapi_satellite_dvbs_read(nc);
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

static void app_webapi_server_satellite_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *type = NULL;
#if DEMOD_DVB_S
	char *id = NULL;
	int tuner = -1;
	char *name = NULL;
	int position = -1900;
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
				//id
				json1 = cJSON_GetObjectItem(root,WEBAPI_ID_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					id = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, id);
				}

				//tuner
				json1 = cJSON_GetObjectItem(root,WEBAPI_TUNER_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					tuner = json1->valueint;
					WEBAPI_PRINTF("%s[%d]tuner=%d\n",__FUNCTION__, __LINE__, tuner);
				}

				//name
				json1 = cJSON_GetObjectItem(root,WEBAPI_NAME_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					name = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]name=%s\n",__FUNCTION__, __LINE__, name);
				}

				//position
				json1 = cJSON_GetObjectItem(root,WEBAPI_POSITION_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					position = json1->valueint;
					WEBAPI_PRINTF("%s[%d]position=%d\n",__FUNCTION__, __LINE__, position);
				}
#endif
				if(type)
				{
					if(strcmp(type, "dvbs") == 0)
					{
#if DEMOD_DVB_S
						if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
						{
							ret = WEBAPI_DEVICE_BUSY;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
						else
						{
							ret = app_webapi_satellite_dvbs_write(nc, id, tuner, name, position);
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

static void app_webapi_server_satellite_delete(struct mg_connection* nc, struct http_message* hm)
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
						if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SAT_LIST))
						{
							ret = WEBAPI_DEVICE_BUSY;
							WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
						}
						else
						{
							ret = app_webapi_satellite_dvbs_delete(id);
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

void app_webapi_server_satellite(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_satellite_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_satellite_write(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "DELETE") == 0)
	{
		app_webapi_server_satellite_delete(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
