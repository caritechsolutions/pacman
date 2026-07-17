#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"
#include "app_default_params.h"
#include "channel_common.h"
#include "app_wnd_system_setting_opt.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include "full_screen.h"
#include <gx_apps.h>
#include <gx_webapi.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern void app_webapi_netplay_message_send(int action, char *url, WebapiMediaType type, uint32_t time);
extern int app_webapi_netplay_exist(void);
extern char *app_webapi_netplay_get_play_data(void);

static WebapiPlayActionType app_webapi_get_play_action(char *action_str)
{
	WebapiPlayActionType action = WEBAPI_PLAY_ACTION_UNKNOWN;

	if(action_str)
	{
		if(strcmp(action_str, "play") == 0)
		{
			action = WEBAPI_PLAY_ACTION_START;
		}
		else if(strcmp(action_str, "pause") == 0)
		{
			action = WEBAPI_PLAY_ACTION_PAUSE;
		}
		else if(strcmp(action_str, "stop") == 0)
		{
			action = WEBAPI_PLAY_ACTION_STOP;
		}
		else if(strcmp(action_str, "seek") == 0)
		{
			action = WEBAPI_PLAY_ACTION_SEEK;
		}
	}

	return action;
}

static WebapiStreamType app_webapi_get_stream_type(char *stream_str)
{
	if(stream_str)
	{
		if(strcmp(stream_str, "dvb") == 0)
		{
			return WEBAPI_STREAM_DVB;
		}
		else if(strcmp(stream_str, "file") == 0)
		{
			return WEBAPI_STREAM_FILE;
		}
		else if(strcmp(stream_str, "net") == 0)
		{
			return WEBAPI_STREAM_NET;
		}
	}

	return WEBAPI_STREAM_UNKNOWN;
}

WebapiMediaType app_webapi_get_media_type(char *media_str)
{
	if(media_str)
	{
		if(strcmp(media_str, "video") == 0)
		{
			return WEBAPI_MEDIA_VIDEO;
		}
		else if(strcmp(media_str, "audio") == 0)
		{
			return WEBAPI_MEDIA_AUDIO;
		}
		else if(strcmp(media_str, "picture") == 0)
		{
			return WEBAPI_MEDIA_PICTURE;
		}
	}

	return WEBAPI_MEDIA_UNKNOWN;
}

static webapi_ret_t app_webapi_dvb_try_play(uint32_t program_id)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	int prog_pos = -1;
	uint32_t prog_total = 0;
	int i = 0;
	GxMsgProperty_NodeNumGet node_num = {0};
	GxMsgProperty_NodeByPosGet node_prog = {0};
	GxBusPmViewInfo view_info_bak = {0};
	GxBusPmViewInfo view_info = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	for(i=0;  i<g_AppPlayOps.normal_play.play_total; i++)
	{
		node_prog.node_type = NODE_PROG;
		node_prog.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
		if(node_prog.prog_data.id == program_id)
		{
			prog_pos = i;
			break;
		}
	}

	if(prog_pos<0)
	{
		app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_bak));
		memcpy(&view_info,&view_info_bak,sizeof(GxBusPmViewInfo));
		view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
		view_info.group_mode = GROUP_MODE_ALL;
		app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

		node_num.node_type = NODE_PROG;
		if(app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void*)(&node_num)) == GXCORE_SUCCESS)
		{
			prog_total = node_num.node_num;
		}
		if(prog_total>0)
		{
			for(i=0; i<prog_total; i++)
			{
				memset(&node_prog, 0, sizeof(GxMsgProperty_NodeByPosGet));
				node_prog.node_type = NODE_PROG;
				node_prog.pos = i;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)(&node_prog)))
				{
					if(node_prog.prog_data.id == program_id)
					{
						prog_pos = i;
						break;
					}
				}
			}
		}
		if(prog_pos<0)
		{
			app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info_bak));
		}
	}

	if(prog_pos>=0)
	{
		ret = WEBAPI_SUCCESS;
		if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING))
			app_system_reset_unfocus_image();
		GUI_EndDialog("after wnd_full_screen");
		g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
		g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, prog_pos);
		g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);
		g_AppFullArb.state.pause = STATE_OFF;
		g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
	}

	return ret;
}

static webapi_ret_t app_webapi_dvb_try_stop(void)
{
	g_AppPlayOps.program_stop();
	g_AppPvrOps.tms_delete(&g_AppPvrOps);

	return WEBAPI_SUCCESS;
}

static char *app_webapi_dvb_get_play_data(void)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;
	GxMsgProperty_NodeByPosGet node_prog = {0};
	PlayerProgInfo *info = NULL;
	char type_str[16];
	AppProgAudioType a_type;
	AppProgVideoType v_type;
	char buffer[64];

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_PLAYINFO_STR, json1);

	if(g_AppPlayOps.normal_play.play_total>0)
	{
		node_prog.node_type = NODE_PROG;
		node_prog.pos = g_AppPlayOps.normal_play.play_count;
		if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog))
		{
			cJSON_AddStringToObject(json1, WEBAPI_NAME_STR, (const char *)node_prog.prog_data.prog_name);

			memset(type_str,0,sizeof(type_str));
			a_type = channel_get_app_audio_type(node_prog.prog_data.cur_audio_type);
			channel_get_audio_type_str(type_str,a_type,sizeof(type_str));
			cJSON_AddStringToObject(json1, WEBAPI_ACODEC_STR, type_str);

			if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
			{
				memset(type_str,0,sizeof(type_str));
				v_type = channel_get_app_video_type(node_prog.prog_data.video_type);
				channel_get_video_type_str(type_str,v_type,sizeof(type_str));
				cJSON_AddStringToObject(json1, WEBAPI_VCODEC_STR, type_str);

				info = GxCore_Calloc(1, sizeof(PlayerProgInfo));
				if(NULL == info)
				{
					app_log_error("calloc failed\n");
					goto end;
				}
				if(GXCORE_SUCCESS == GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info))
				{
					if(info->video.width)
					{
						cJSON_AddNumberToObject(json1, WEBAPI_VWIDTH_STR, info->video.width);
					}
					if(info->video.height)
					{
						cJSON_AddNumberToObject(json1, WEBAPI_VHEIGHT_STR, info->video.height);
					}
					if(info->video.bitrate)
					{
						cJSON_AddNumberToObject(json1, WEBAPI_BITRATE_STR, info->video.bitrate);
					}
					if(info->video.fps)
					{
						memset(buffer,0,sizeof(buffer));
						sprintf(buffer, "%3.1f fps",info->video.fps);
						cJSON_AddStringToObject(json1, WEBAPI_FPS_STR, buffer);
					}
				}
				GxCore_Free(info);
			}
		}
	}

end:
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	return out;
}

static webapi_ret_t app_webapi_play_get(struct mg_connection* nc, struct http_message* hm, WebapiStreamType stream_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	char *out = NULL;

	if(stream_type == WEBAPI_STREAM_DVB)
	{
		out = app_webapi_dvb_get_play_data();
	}
	else if((stream_type == WEBAPI_STREAM_NET)||(stream_type == WEBAPI_STREAM_FILE))
	{
		ret = WEBAPI_OPERATION_INVALID;
		if(app_webapi_netplay_exist() == 0)
		{
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
		}
		else
		{
			out = app_webapi_netplay_get_play_data();
		}
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

static webapi_ret_t app_webapi_dvb_play(char *id)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	uint32_t program_id = 0;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
	{
		ret = WEBAPI_DEVICE_BUSY;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else
	{
		program_id = (uint32_t)strtoul(id, 0, 10);
		if(program_id>0)
		{
			ret = app_webapi_dvb_try_play(program_id);
		}
	}

	return ret;
}

static webapi_ret_t app_webapi_dvb_stop(void)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH))
	{
		ret = WEBAPI_DEVICE_BUSY;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else
	{
		ret = app_webapi_dvb_try_stop();
	}

	return ret;
}

static webapi_ret_t app_webapi_net_play(char *url, WebapiMediaType media_type, uint32_t time)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d][%d/%u]%s\n",__FUNCTION__, __LINE__, media_type, time, url);
	if(url&&(media_type != WEBAPI_MEDIA_UNKNOWN))
	{
		app_webapi_netplay_message_send(WEBAPI_PLAY_ACTION_START, url, media_type, time);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

static webapi_ret_t app_webapi_net_stop(WebapiMediaType media_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d][%d]\n",__FUNCTION__, __LINE__, media_type);
	if(app_webapi_netplay_exist() == 0)
	{
		ret = WEBAPI_OPERATION_INVALID;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
	{
		app_webapi_netplay_message_send(WEBAPI_PLAY_ACTION_STOP, NULL, media_type, 0);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

static webapi_ret_t app_webapi_net_pause(WebapiMediaType media_type)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d][%d]\n",__FUNCTION__, __LINE__, media_type);
	if(app_webapi_netplay_exist() == 0)
	{
		ret = WEBAPI_OPERATION_INVALID;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
	{
		app_webapi_netplay_message_send(WEBAPI_PLAY_ACTION_PAUSE, NULL, media_type, 0);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

static webapi_ret_t app_webapi_net_seek(WebapiMediaType media_type, uint32_t time)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;

	WEBAPI_PRINTF("%s[%d][%d/%u]\n",__FUNCTION__, __LINE__, media_type, time);
	if(app_webapi_netplay_exist() == 0)
	{
		ret = WEBAPI_OPERATION_INVALID;
		WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(ret));
	}
	else if((media_type == WEBAPI_MEDIA_VIDEO)||(media_type == WEBAPI_MEDIA_AUDIO))
	{
		app_webapi_netplay_message_send(WEBAPI_PLAY_ACTION_SEEK, NULL, media_type, time);
		ret = WEBAPI_SUCCESS;
	}

	return ret;
}

static void app_webapi_server_play_read(struct mg_connection* nc, struct http_message* hm)
{
	char type[16];
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	WebapiStreamType stream_type = WEBAPI_STREAM_UNKNOWN;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	memset(type, 0, sizeof(type));
	if(mg_get_http_var(&hm->query_string, WEBAPI_STREAM_TYPE_STR, type, sizeof(type))>0)
	{
		WEBAPI_PRINTF("%s[%d]type=%s\n",__FUNCTION__, __LINE__, type);
		stream_type = app_webapi_get_stream_type(type);
	}

	if(stream_type != WEBAPI_STREAM_UNKNOWN)
	{
		ret = app_webapi_play_get(nc, hm, stream_type);
	}

	if(ret != WEBAPI_SUCCESS)
	{
		app_webapi_server_fail(nc, ret);
	}
}

static void app_webapi_server_play_write(struct mg_connection* nc, struct http_message* hm)
{
	webapi_ret_t ret = WEBAPI_FUNCTION_PARAM_ERR;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	WebapiStreamType stream_type = WEBAPI_STREAM_UNKNOWN;
	WebapiMediaType media_type = WEBAPI_MEDIA_UNKNOWN;
	WebapiPlayActionType action = WEBAPI_PLAY_ACTION_UNKNOWN;
	char *uri = NULL;
	char *final_uri = NULL;
	uint32_t time = 0;
	char *findStr = NULL;

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//stream type
				json1 = cJSON_GetObjectItem(root,WEBAPI_STREAM_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					stream_type = app_webapi_get_stream_type(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]stream_type[%d]=%s\n",__FUNCTION__, __LINE__, stream_type, json1->valuestring);
				}

				//media type
				json1 = cJSON_GetObjectItem(root,WEBAPI_MEDIA_TYPE_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					media_type = app_webapi_get_media_type(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]media_type[%d]=%s\n",__FUNCTION__, __LINE__, media_type, json1->valuestring);
				}

				//action
				json1 = cJSON_GetObjectItem(root,WEBAPI_ACTION_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					action = app_webapi_get_play_action(json1->valuestring);
					WEBAPI_PRINTF("%s[%d]action[%d]=%s\n",__FUNCTION__, __LINE__, action, json1->valuestring);
				}

				//uri
				json1 = cJSON_GetObjectItem(root,WEBAPI_URI_STR);
				if((json1) && (json1->type == cJSON_String))
				{
					uri = json1->valuestring;
					WEBAPI_PRINTF("%s[%d]uri=%s\n",__FUNCTION__, __LINE__, uri);
					if((strlen(uri)>strlen(WEBAPI_URI_ENC_FLAG))
						&&(memcmp(uri, WEBAPI_URI_ENC_FLAG, strlen(WEBAPI_URI_ENC_FLAG)) == 0))
					{
						final_uri = gx_webapi_str_decrypt(uri+strlen(WEBAPI_URI_ENC_FLAG));
					}
					else
					{
						final_uri = strdup(uri);
					}
					findStr = strchr(final_uri,'#');
					if (NULL != findStr)
						*findStr = '\0';
					WEBAPI_PRINTF("%s[%d]final_uri=%s\n",__FUNCTION__, __LINE__, final_uri);
				}

				//time
				json1 = cJSON_GetObjectItem(root,WEBAPI_TIME_STR);
				if((json1) && (json1->type == cJSON_Number))
				{
					time = json1->valueint;
					WEBAPI_PRINTF("%s[%d]time=%d\n",__FUNCTION__, __LINE__, time);
				}

				if(stream_type == WEBAPI_STREAM_DVB)
				{
					if((action == WEBAPI_PLAY_ACTION_START)&&final_uri)
					{
						ret = app_webapi_dvb_play(final_uri);
					}
					else if(action == WEBAPI_PLAY_ACTION_STOP)
					{
						ret = app_webapi_dvb_stop();
					}
				}
				else if((stream_type == WEBAPI_STREAM_NET)||(stream_type == WEBAPI_STREAM_FILE))
				{
					if(action == WEBAPI_PLAY_ACTION_START)
					{
						ret = app_webapi_net_play(final_uri, media_type, time);
					}
					else if(action == WEBAPI_PLAY_ACTION_STOP)
					{
						ret = app_webapi_net_stop(media_type);
					}
					else if(action == WEBAPI_PLAY_ACTION_PAUSE)
					{
						ret = app_webapi_net_pause(media_type);
					}
					else if(action == WEBAPI_PLAY_ACTION_SEEK)
					{
						ret = app_webapi_net_seek(media_type, time);
					}
				}
				cJSON_Delete(root);
			}
			free(data);
			if(final_uri)
			{
				free(final_uri);
			}
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

void app_webapi_server_play(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_play_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_play_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif

