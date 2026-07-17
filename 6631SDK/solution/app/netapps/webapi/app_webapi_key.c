#include "app.h"
#include "app_send_msg.h"
#include "app_key.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

typedef struct webapi_keyinfo_s
{
	const char *webapi_key;
	int stb_keycode;
}webapi_keyinfo_t;

extern int Gui_WriteKeyBuf(unsigned int key);

static webapi_keyinfo_t webapi_key_array[]=
{
	{"WEBK_0", STBK_0},
	{"WEBK_1", STBK_1},
	{"WEBK_2", STBK_2},
	{"WEBK_3", STBK_3},
	{"WEBK_4", STBK_4},
	{"WEBK_5", STBK_5},
	{"WEBK_6", STBK_6},
	{"WEBK_7", STBK_7},
	{"WEBK_8", STBK_8},
	{"WEBK_9", STBK_9},
	{"WEBK_POWER", STBK_HALT},
	{"WEBK_MUTE", STBK_MUTE},
	{"WEBK_TV_RADIO", STBK_TV_RADIO},
	{"WEBK_MENU", STBK_MENU},
	{"WEBK_EPG", STBK_EPG},
	{"WEBK_INFO", STBK_INFO},
	{"WEBK_EXIT", STBK_EXIT},
	{"WEBK_UP", STBK_UP},
	{"WEBK_DOWN", STBK_DOWN},
	{"WEBK_LEFT", STBK_LEFT},
	{"WEBK_RIGHT", STBK_RIGHT},
	{"WEBK_OK", STBK_OK},
	{"WEBK_PAGE_UP", STBK_PAGE_UP},
	{"WEBK_PAGE_DOWN", STBK_PAGE_DOWN},
	{"WEBK_VOLUME_UP", STBK_VOLUP},
	{"WEBK_VOLUME_DOWN", STBK_VOLDOWN},
	{"WEBK_CH_UP", STBK_CH_UP},
	{"WEBK_CH_DOWN", STBK_CH_DOWN},
	{"WEBK_RED", STBK_RED},
	{"WEBK_BLUE", STBK_BLUE},
	{"WEBK_GREEN", STBK_GREEN},
	{"WEBK_YELLOW", STBK_YELLOW},
	{"WEBK_PLAY", STBK_PLAY},
	{"WEBK_PAUSE", STBK_PAUSE},
	{"WEBK_STOP", APPK_STOP},
	{"WEBK_PREV", STBK_LAST},
	{"WEBK_NEXT", STBK_NEXT},
	{"WEBK_FF", STBK_FF},
	{"WEBK_FB", STBK_FB},
	{"WEBK_REC", STBK_REC_START},
	{"WEBK_RECALL", STBK_RECALL},
	{"WEBK_AUDIO", STBK_AUDIO},
	{"WEBK_SUBT", STBK_SUBT},
	{"WEBK_TTX", STBK_TTX},
	{"WEBK_EPG", STBK_EPG},
	{"WEBK_FAV", STBK_FAV},
	{"WEBK_SAT", STBK_SAT},
	{"WEBK_FIND", STBK_FIND},
	{"WEBK_MEDIA", STBK_MEDIA},
/*	{"WEBK_ZOOM", STBK_ZOOM},*/
};
#define WEBAPI_KEY_NUM (sizeof(webapi_key_array)/sizeof(webapi_keyinfo_t))

static int app_webapi_get_stb_key(char *key)
{
	int stb_key = 0;
	int i = 0;

	if(key)
	{
		for(i=0; i<WEBAPI_KEY_NUM; i++)
		{
			if(strcmp(webapi_key_array[i].webapi_key, key) == 0)
			{
				stb_key = webapi_key_array[i].stb_keycode;
				break;
			}
		}
	}

	return stb_key;
}

void app_webapi_set_remote_key(int key)
{
	Gui_WriteKeyBuf(key);
}

static void app_webapi_server_key_read(struct mg_connection* nc, struct http_message* hm)
{
	int i = 0;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	char *out = NULL;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_KEY_STR, json1);
	cJSON_AddNumberToObject(json1, WEBAPI_TOTAL_STR, WEBAPI_KEY_NUM);
	json2 = cJSON_CreateArray();
	cJSON_AddItemToObject(json1, WEBAPI_DATA_STR, json2);
	for(i=0; i<WEBAPI_KEY_NUM; i++)
	{
		cJSON_AddItemToArray(json2, cJSON_CreateString(webapi_key_array[i].webapi_key));
	}
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);

	free(out);
}

static void app_webapi_server_key_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *key = NULL;
	int stb_key = 0;
	AppMsg_WebapiUIControl param = {0};

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				json1 = cJSON_GetObjectItem(root,"key");
				if((json1) && (json1->type == cJSON_String))
				{
					key = strdup(json1->valuestring);
					WEBAPI_PRINTF("key=%s\n",key);
				}
				cJSON_Delete(root);
			}
			free(data);
		}
		if(key)
		{
			stb_key = app_webapi_get_stb_key(key);
			free(key);
		}
		if(stb_key>0)
		{
			param.type = WEBAPI_MSG_REMOTE;
			param.result = stb_key;
			app_send_msg_exec(APPMSG_WEBAPI_UI_CONTROL, &param);
			app_webapi_server_success(nc);
		}
		else
		{
			WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
			app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
		}
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_PARAM_ERR));
		app_webapi_server_fail(nc, WEBAPI_FUNCTION_PARAM_ERR);
	}
}

void app_webapi_server_key(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_key_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_key_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
