#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#if WEBAPI_SUPPORT
#include "channel_common.h"
#include <cJSON.h>
#include "app_webapi.h"
#include "netapps/webapi/app_webapi_priv.h"

char *app_webapi_get_program_group_data(WebapiProgType prog_type)
{
	GxBusPmDataProgType  stream_type = GXBUS_PM_PROG_ALL;
	AppIdOps* id_ops = NULL;
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	char buffer[32] = {0};
	int i = 0;
	GxMsgProperty_NodeByIdGet node;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	id_ops = new_id_ops();
	if(id_ops)
	{
		if(prog_type == WEBAPI_PROG_TV)
		{
			stream_type = GXBUS_PM_PROG_TV;
		}
		else if(prog_type == WEBAPI_PROG_RADIO)
		{
			stream_type = GXBUS_PM_PROG_RADIO;
		}
		else
		{
			stream_type = GXBUS_PM_PROG_ALL;
		}
		if(GXCORE_SUCCESS == webapi_channel_get_group_id(id_ops, stream_type))
		{
			if(GXCORE_SUCCESS == channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer))
			{
				root = cJSON_CreateObject();
				cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
				json1 = cJSON_CreateObject();
				cJSON_AddItemToObject(root, WEBAPI_CATEGORY_STR, json1);
				cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, id_total);
				json2 = cJSON_CreateArray();
				cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);
				for(i=0; i<id_total; i++)
				{
					json3 = cJSON_CreateObject();
					cJSON_AddItemToArray(json2, json3);
					memset(buffer, 0, sizeof(buffer));
					snprintf(buffer, sizeof(buffer), "%u", id_buffer[i]);
					cJSON_AddStringToObject(json3, WEBAPI_ID_STR, buffer);
					memset(buffer, 0, sizeof(buffer));
					if(i ==0)
					{
						snprintf(buffer, sizeof(buffer), "%s", STR_ID_ALL);
					}
					else
					{
						memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
						if((id_buffer[i] & FAV_MASK) == 0)//sat
						{
							node.node_type = NODE_SAT;
							node.id = id_buffer[i];
							app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
							snprintf(buffer, sizeof(buffer), "%s",node.sat_data.sat_s.sat_name);
						}
						else if(id_buffer[i] == HD_MASK)
						{
							snprintf(buffer, sizeof(buffer), "%s", STR_ID_HD);
						}
						else
						{
							node.node_type = NODE_FAV_GROUP;
							node.id = (id_buffer[i] & (~FAV_MASK));
							app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
							snprintf(buffer, sizeof(buffer),"%s",node.fav_item.fav_name);
						}
					}
					cJSON_AddStringToObject(json3, WEBAPI_NAME_STR, buffer);
				}
				out = cJSON_PrintUnformatted(root);
				cJSON_Delete(root);
   				APP_FREE(id_buffer);
			}
		}
		del_id_ops(id_ops);
	}

	return out;
}

static int app_webapi_get_fav_id_array(uint32_t *total, uint32_t **id_array)
{
	GxMsgProperty_NodeNumGet num_node = {0};
	GxMsgProperty_NodeByPosGet node = {0};
	int i = 0;

	memset(&num_node, 0, sizeof(GxMsgProperty_NodeNumGet));
	num_node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&num_node));
	*total = num_node.node_num;
	if(*total == 0)
	{
		return GXCORE_ERROR;
	}

	*id_array = GxCore_Calloc(*total, sizeof(uint32_t));
	if(*id_array == NULL)
	{
		return GXCORE_ERROR;
	}

	for(i = 0; i<num_node.node_num; i++)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node.pos = i;
		node.node_type = NODE_FAV_GROUP;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		(*id_array)[i] = node.fav_item.id;
	}

	return GXCORE_SUCCESS;
}

char *app_webapi_get_fav_group_data(void)
{
	GxMsgProperty_NodeNumGet num_node = {0};
	GxMsgProperty_NodeByPosGet node = {0};
	char buffer[32] = {0};
	int i = 0;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(&num_node, 0, sizeof(GxMsgProperty_NodeNumGet));
	num_node.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&num_node));

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_CATEGORY_STR, json1);
	cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, num_node.node_num);
	json2 = cJSON_CreateArray();
	cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);

	for(i = 0; i<num_node.node_num; i++)
	{
		json3 = cJSON_CreateObject();
		cJSON_AddItemToArray(json2, json3);
		memset(&node, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node.pos = i;
		node.node_type = NODE_FAV_GROUP;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node);
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%u", node.fav_item.id);
		cJSON_AddStringToObject(json3, WEBAPI_ID_STR, buffer);
		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%s", node.fav_item.fav_name);
		cJSON_AddStringToObject(json3, WEBAPI_NAME_STR, buffer);
	}
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return out;
}

webapi_ret_t app_webapi_set_fav_group_data(char *id, char *name)
{
	webapi_ret_t ret = WEBAPI_INTERNAL_ERR;
	GxMsgProperty_NodeByIdGet fav_node = {0};
	GxMsgProperty_NodeModify node_modify = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(&fav_node, 0, sizeof(GxMsgProperty_NodeByIdGet));
	fav_node.id = (uint32_t)strtoul(id, 0, 10);
	fav_node.node_type = NODE_FAV_GROUP;
	if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &fav_node))
	{
		memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
		node_modify.node_type = NODE_FAV_GROUP;
		memcpy(&node_modify.fav_item, &fav_node.fav_item, sizeof(GxBusPmDataFavItem));
		strncpy((char*)node_modify.fav_item.fav_name, name, MAX_FAV_GROUP_NAME-1);
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
		{
			ret = WEBAPI_SUCCESS;
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

	return ret;
}

static void app_webapi_get_program_pos(uint32_t page, uint32_t limit, uint32_t total, uint32_t *start, uint32_t *num)
{
	uint32_t start_pos = 0;
	uint32_t prog_num = 0;

	if(page>0)
	{
		if(limit == 0)
		{
			limit = WEBAPI_DEFAULT_PROGRAM_LIMIT;
		}
		start_pos = limit*(page-1);
		if(start_pos<total)
		{
			if((start_pos+limit)>total)
			{
				prog_num = total -start_pos;
			}
			else
			{
				prog_num = limit;
			}
		}
	}
	else
	{
		start_pos = 0;
		if((limit>=total)||(limit == 0))
		{
			prog_num = total;
		}
		else
		{
			prog_num = limit;
		}
	}
	*start = start_pos;
	*num = prog_num;
	WEBAPI_PRINTF("%s[%d]start_pos=%u,prog_num=%u\n",__FUNCTION__, __LINE__,start_pos,prog_num);
}

static cJSON *app_webapi_get_program_node_json(GxMsgProperty_NodeByPosGet *node, uint32_t fav_total, uint32_t *fav_ids)
{
	cJSON *json = NULL;
	cJSON *json1 = NULL;
	char buffer[64] = {0};
	int value = 0;
	int i = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	json = cJSON_CreateObject();

	//id
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%u", node->prog_data.id);
	cJSON_AddStringToObject(json, WEBAPI_ID_STR, buffer);
	WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, buffer);

	//sat id
	cJSON_AddNumberToObject(json, WEBAPI_SATID_STR, (int)node->prog_data.sat_id);
	WEBAPI_PRINTF("%s[%d]sat_id=%d\n",__FUNCTION__, __LINE__, (int)node->prog_data.sat_id);

	//tuner
	cJSON_AddNumberToObject(json, WEBAPI_TUNER_STR, (int)node->prog_data.tuner);
	WEBAPI_PRINTF("%s[%d]tuner=%d\n",__FUNCTION__, __LINE__, (int)node->prog_data.tuner);

	//program type
	memset(buffer, 0, sizeof(buffer));
	if(GXBUS_PM_PROG_TV == node->prog_data.service_type)
	{
		snprintf(buffer, sizeof(buffer), "%s", WEBAPI_TV_STR);
	}
	else
	{
		snprintf(buffer, sizeof(buffer), "%s", WEBAPI_RADIO_STR);
	}
	cJSON_AddStringToObject(json, WEBAPI_TYPE_STR, buffer);
	WEBAPI_PRINTF("%s[%d]prog type=%s\n",__FUNCTION__, __LINE__, buffer);

	//program name
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, node->prog_data.prog_name, MAX_PROG_NAME);
	cJSON_AddStringToObject(json, WEBAPI_NAME_STR, buffer);
	WEBAPI_PRINTF("%s[%d]prog name=%s\n",__FUNCTION__, __LINE__, buffer);
	//video type

	if(GXBUS_PM_PROG_TV == node->prog_data.service_type)
	{
		memset(buffer, 0, sizeof(buffer));
		channel_get_video_type_str(buffer,channel_get_app_video_type(node->prog_data.video_type),sizeof(buffer));
		cJSON_AddStringToObject(json, WEBAPI_VIDEOTYPE_STR, buffer);
		WEBAPI_PRINTF("%s[%d]video type=%s\n",__FUNCTION__, __LINE__, buffer);
	}

	//audio type
	memset(buffer, 0, sizeof(buffer));
	channel_get_audio_type_str(buffer,channel_get_app_audio_type(node->prog_data.cur_audio_type),sizeof(buffer));
	cJSON_AddStringToObject(json, WEBAPI_AUDIOTYPE_STR, buffer);
	WEBAPI_PRINTF("%s[%d]audio type=%s\n",__FUNCTION__, __LINE__, buffer);

	//scramble flag
	if(node->prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_SCRAMBLE_STR, value);
	WEBAPI_PRINTF("%s[%d]scramble=%d\n",__FUNCTION__, __LINE__, value);

	//skip flag
	if(node->prog_data.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_SKIP_STR, value);

	//lock flag
	if(node->prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_LOCK_STR, value);
	WEBAPI_PRINTF("%s[%d]lock=%d\n",__FUNCTION__, __LINE__, value);

	//fav
	if((fav_total>0)&&fav_ids)
	{
		WEBAPI_PRINTF("%s[%d]favorite_flag=0x%x\n",__FUNCTION__, __LINE__, node->prog_data.favorite_flag);
		json1 = cJSON_CreateArray();
		cJSON_AddItemToObject(json, WEBAPI_FAV_STR, json1);
		for(i=0; (i<(int)fav_total)&&(i<32); i++)
		{
			if(((node->prog_data.favorite_flag&(1<<i))>0)&&(fav_ids[i]>0))
			{
				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer)-1, "%u", fav_ids[i]);
				cJSON_AddItemToArray(json1, cJSON_CreateString(buffer));
			}
		}
	}

	//ttx flag
	if(node->prog_data.ttx_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_TTX_STR, value);
	WEBAPI_PRINTF("%s[%d]ttx=%d\n",__FUNCTION__, __LINE__, value);

	//subt flag
	if(node->prog_data.subt_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_SUBTITLE_STR, value);
	WEBAPI_PRINTF("%s[%d]subt=%d\n",__FUNCTION__, __LINE__, value);

	//ac3 flag
	if(node->prog_data.ac3_flag == GXBUS_PM_PROG_BOOL_ENABLE)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_AC3_STR, value);
	WEBAPI_PRINTF("%s[%d]ac3=%d\n",__FUNCTION__, __LINE__, value);

	//hd flag
	if(node->prog_data.definition == GXBUS_PM_PROG_HD)
	{
		value = 1;
	}
	else
	{
		value = 0;
	}
	cJSON_AddNumberToObject(json, WEBAPI_HD_STR, value);
	WEBAPI_PRINTF("%s[%d]hd=%d\n",__FUNCTION__, __LINE__, value);

	//share path
#if DVB2IP_SERVER_SUPPORT
	{
		char *dvb2ip_url = NULL;
		dvb2ip_url = app_dvb2ip_get_prog_url(node->prog_data.id);
		if(dvb2ip_url)
		{
			cJSON_AddStringToObject(json, WEBAPI_SHAREPATH_STR, dvb2ip_url);
			WEBAPI_PRINTF("%s[%d]share_path=%s\n",__FUNCTION__, __LINE__, dvb2ip_url);
			GxCore_Free(dvb2ip_url);
		}
	}
#endif
#if SAT2IP_SERVER_SUPPORT
	char *sat2ip_url = NULL;
	if(app_sat2ip_use_flag_get() > 0)
	{
		sat2ip_url = app_satip_get_prog_url(node->prog_data.id);
		if(sat2ip_url)
		{
			cJSON_AddStringToObject(json, WEBAPI_SHAREPATH_STR, sat2ip_url);
			WEBAPI_PRINTF("%s[%d]share_path=%s\n",__FUNCTION__, __LINE__, sat2ip_url);
			GxCore_Free(sat2ip_url);
		}
	}
#endif

	return json;
}

char *app_webapi_get_program_data(WebapiProgType prog_type, uint32_t category_id, uint32_t page, uint32_t limit)
{
	AppIdOps* id_ops = NULL;
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	uint32_t prog_total = 0;
	uint32_t fav_total = 0;
	uint32_t *fav_idarray = NULL;
	int i = 0;
	GxMsgProperty_NodeByPosGet ProgNode = {0};
	GxMsgProperty_NodeNumGet nodeNum = {0};
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;
	uint32_t group_id = 0;
	uint32_t start_pos = 0;
	uint32_t prog_num = 0;
	GxBusPmDataProgType  stream_type = GXBUS_PM_PROG_ALL;
	GxBusPmViewInfo view_info_bak = {0};
	GxBusPmViewInfo view_info = {0};

	WEBAPI_PRINTF("%s[%d]prog_type=%d,category_id=%u,page=%u, limit=%u\n",__FUNCTION__, __LINE__, prog_type, category_id, page, limit);
	if(prog_type == WEBAPI_PROG_TV)
	{
		stream_type = GXBUS_PM_PROG_TV;
	}
	else if(prog_type == WEBAPI_PROG_RADIO)
	{
		stream_type = GXBUS_PM_PROG_RADIO;
	}
	else
	{
		stream_type = GXBUS_PM_PROG_ALL;
	}
	id_ops = new_id_ops();
	if(id_ops)
	{
		if(GXCORE_SUCCESS == webapi_channel_get_group_id(id_ops, stream_type))
		{
			if(GXCORE_SUCCESS == channel_id_total_and_buffer_dump(id_ops, &id_total, &id_buffer))
			{
				for(i = 0; i<id_total; i++)
				{
					if(id_buffer[i] == category_id)
					{
						group_id = id_buffer[i];
						break;
					}
				}
			}
		}
	}

	if(group_id>0)
	{
		app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_bak));
		memcpy(&view_info,&view_info_bak,sizeof(GxBusPmViewInfo));
		view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
		view_info.stream_type = stream_type;
		if(group_id == ALL_SAT)//all satellite
		{
			view_info.group_mode = GROUP_MODE_ALL;
		}
		else//sat or fav
		{
			if(group_id == HD_MASK)
			{
				view_info.group_mode = GROUP_MODE_HD;
			}
			else if((group_id & FAV_MASK) == 0)//sat
			{
				view_info.group_mode = GROUP_MODE_SAT;
				view_info.sat_id = group_id;
			}
			else//fav
			{
				view_info.group_mode = GROUP_MODE_FAV;
				view_info.fav_id = (group_id & (~FAV_MASK));
			}
		}
        GxBus_PmViewInfoMemSet(&view_info);// Only memory, not flash

		nodeNum.node_type = NODE_PROG;
		if(app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void*)(&nodeNum)) == GXCORE_SUCCESS)
		{
			prog_total = nodeNum.node_num;
			WEBAPI_PRINTF("%s[%d]prog_total=%u\n",__FUNCTION__, __LINE__, prog_total);
		}

		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
		json1 = cJSON_CreateObject();
		cJSON_AddItemToObject(root, WEBAPI_PROGRAM_STR, json1);
		cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, prog_total);
		if(prog_total>0)
		{
			app_webapi_get_program_pos(page, limit, prog_total, &start_pos, &prog_num);
			if(prog_num>0)
			{
				json2 = cJSON_CreateArray();
				cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);
				app_webapi_get_fav_id_array(&fav_total, &fav_idarray);
				for(i=start_pos; (i<(start_pos+prog_num))&&(i<prog_total); i++)
				{
					memset(&ProgNode, 0, sizeof(GxMsgProperty_NodeByPosGet));
					ProgNode.node_type = NODE_PROG;
					ProgNode.pos = i;
					if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)(&ProgNode)))
					{
						json3 = app_webapi_get_program_node_json(&ProgNode, fav_total, fav_idarray);
						cJSON_AddItemToArray(json2, json3);
					}
				}
			}
		}
        GxBus_PmViewInfoMemClear();//recovery flash viewinfo
		out = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);
	}

	APP_FREE(id_buffer);
	APP_FREE(fav_idarray);
	if(id_ops)
	{
		del_id_ops(id_ops);
	}

	return out;
}

static uint32_t app_webapi_get_fav_value(cJSON *json)
{
	cJSON *json1 = NULL;
	uint32_t fav_flag = 0;
	int num = 0;
	int i = 0;
	int j = 0;
	uint32_t fav_total = 0;
	uint32_t *fav_idarray = NULL;
	uint32_t fav_id = 0;

	app_webapi_get_fav_id_array(&fav_total, &fav_idarray);
	num = cJSON_GetArraySize(json);
	if((num>0)&&(fav_total>0)&&fav_idarray)
	{
		for(i=0; i<num; i++)
		{
			json1 = cJSON_GetArrayItem(json, i);
			if(json1 && (json1->type == cJSON_String))
			{
				WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, json1->valuestring);
				fav_id = (uint32_t)strtoul(json1->valuestring, 0, 10);
				if(fav_id>0)
				{
					for(j=0; j<(int)fav_total; j++)
					{
						if(fav_id == fav_idarray[j])
						{
							fav_flag |= 1<<(j%32);
							break;
						}
					}
				}
			}
		}
	}
	APP_FREE(fav_idarray);
	WEBAPI_PRINTF("%s[%d]fav_flag=%u\n",__FUNCTION__, __LINE__, fav_flag);

	return fav_flag;
}

webapi_ret_t app_webapi_set_program_data(cJSON *json)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	GxMsgProperty_NodeByIdGet node = {0};
	GxMsgProperty_NodeModify node_modify = {0};
	cJSON *json1 = NULL;
	cJSON *fav = NULL;
	char *id = NULL;
	char *name = NULL;
	int skip = -1;
	int lock = -1;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);

	//id
	json1 = cJSON_GetObjectItem(json,WEBAPI_ID_STR);
	if((json1) && (json1->type == cJSON_String))
	{
		id = json1->valuestring;
		WEBAPI_PRINTF("%s[%d]id=%s\n",__FUNCTION__, __LINE__, id);
	}

	//name
	json1 = cJSON_GetObjectItem(json,WEBAPI_NAME_STR);
	if((json1) && (json1->type == cJSON_String))
	{
		name = json1->valuestring;
		WEBAPI_PRINTF("%s[%d]name=%s\n",__FUNCTION__, __LINE__, name);
	}

	//skip
	json1 = cJSON_GetObjectItem(json,WEBAPI_SKIP_STR);
	if((json1) && (json1->type == cJSON_Number))
	{
		skip = json1->valueint;
		WEBAPI_PRINTF("%s[%d]skip=%d\n",__FUNCTION__, __LINE__, skip);
	}

	//lock
	json1 = cJSON_GetObjectItem(json,WEBAPI_LOCK_STR);
	if((json1) && (json1->type == cJSON_Number))
	{
		lock = json1->valueint;
		WEBAPI_PRINTF("%s[%d]lock=%d\n",__FUNCTION__, __LINE__, lock);
	}

	//fav
	json1 = cJSON_GetObjectItem(json,WEBAPI_FAV_STR);
	if((json1) && (json1->type == cJSON_Array))
	{
		fav = json1;
		WEBAPI_PRINTF("%s[%d]need set fav\n",__FUNCTION__, __LINE__);
	}

	if(id)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
		node.id = (uint32_t)strtoul(id, 0, 10);
		node.node_type = NODE_PROG;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node))
		{
			memset(&node_modify, 0, sizeof(GxMsgProperty_NodeModify));
			node_modify.node_type = NODE_PROG;
			memcpy(&node_modify.prog_data, &node.prog_data, sizeof(GxBusPmDataProg));
			if(name&&(strlen(name)>0))
			{
    				strncpy((char*)node_modify.prog_data.prog_name, name, MAX_PROG_NAME-1);
			}
			if(skip>=0)
			{
				if(skip>0)
				{
					node_modify.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_ENABLE;
				}
				else
				{
					node_modify.prog_data.skip_flag = GXBUS_PM_PROG_BOOL_DISABLE;
				}
			}
			if(lock>=0)
			{
				if(lock>0)
				{
					node_modify.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_ENABLE;
				}
				else
				{
					node_modify.prog_data.lock_flag = GXBUS_PM_PROG_BOOL_DISABLE;
				}
			}
			if(fav)
			{
				node_modify.prog_data.favorite_flag = app_webapi_get_fav_value(fav);
			}
			if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				ret = WEBAPI_SUCCESS;
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
	}

	return ret;
}
#endif
