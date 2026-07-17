#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern int app_webapi_language_setting_busy(void);

static void app_webapi_server_language_read(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	cJSON *json2 = NULL;
	cJSON *json3 = NULL;
	char *out = NULL;
	int current = 0;
	int i = 0;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	json1 = cJSON_CreateObject();
	cJSON_AddItemToObject(root, WEBAPI_LANGUAGE_STR, json1);

	//menu
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_MENU_STR, json2);
	GxBus_ConfigGetInt(OSD_LANG_KEY, &current, OSD_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	//audio1
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_AUDIO1_STR, json2);
	GxBus_ConfigGetInt(AUDIO_1ST_KEY, &current, AUDIO1_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	//audio2
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_AUDIO2_STR, json2);
	GxBus_ConfigGetInt(AUDIO_2ND_KEY, &current, AUDIO2_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	//subtitle
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_SUBTITLE_STR, json2);
	GxBus_ConfigGetInt(SUBTILE_1ST_KEY, &current, SUB_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	cJSON_AddItemToArray(json3, cJSON_CreateString("Off"));
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	//ttx
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_TTX_STR, json2);
	GxBus_ConfigGetInt(TTX_LANG_KEY, &current, TTX_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	cJSON_AddItemToArray(json3, cJSON_CreateString("Auto"));
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	//epg
	json2 = cJSON_CreateObject();
	cJSON_AddItemToObject(json1, WEBAPI_EPG_STR, json2);
	GxBus_ConfigGetInt(EPG_LANG_KEY, &current, EPG_LANG);
	cJSON_AddNumberToObject(json2, WEBAPI_CURRENT_STR, current);
	json3 = cJSON_CreateArray();
	cJSON_AddItemToObject(json2, WEBAPI_OPTIONS_STR, json3);
	cJSON_AddItemToArray(json3, cJSON_CreateString("All"));
	for(i=0; i<LANG_MAX; i++)
	{
		cJSON_AddItemToArray(json3, cJSON_CreateString(g_LangName[i]));
	}

	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);
	free(out);
}

static void app_webapi_server_language_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	int menu_set = -1;
	int audio1_set = -1;
	int audio2_set = -1;
	int subtitle_set = -1;
	int ttx_set = -1;
	int epg_set = -1;
	int valid = 0;
	int invalid = 0;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//menu
				json1 = cJSON_GetObjectItem(root,"menu");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<LANG_MAX))
					{
						menu_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[menu=%d]\n",__FUNCTION__, menu_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid menu=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				//audio1
				json1 = cJSON_GetObjectItem(root,"audio1");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<LANG_MAX))
					{
						audio1_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[audio1=%d]\n",__FUNCTION__, audio1_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid audio1=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				//audio2
				json1 = cJSON_GetObjectItem(root,"audio2");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<LANG_MAX))
					{
						audio2_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[audio2=%d]\n",__FUNCTION__, audio2_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid audio2=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				//subtitle
				json1 = cJSON_GetObjectItem(root,"subtitle");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<LANG_MAX+1))
					{
						subtitle_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[subtitle=%d]\n",__FUNCTION__, subtitle_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid subtitle=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				//ttx
				json1 = cJSON_GetObjectItem(root,"ttx");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<(LANG_MAX+1)))
					{
						ttx_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[ttx=%d]\n",__FUNCTION__, ttx_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid ttx=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				//epg
				json1 = cJSON_GetObjectItem(root,"epg");
				if((json1) && (json1->type == cJSON_Number))
				{
					if((json1->valueint>=0)&&(json1->valueint<(LANG_MAX+1)))
					{
						epg_set = json1->valueint;
						valid++;
						WEBAPI_PRINTF("%s[epg=%d]\n",__FUNCTION__, epg_set);
					}
					else
					{
						invalid++;
						WEBAPI_PRINTF("%s[invalid epg=%d]\n",__FUNCTION__, json1->valueint);
					}
				}

				cJSON_Delete(root);
			}
			free(data);
		}
		if((valid>0)&&(invalid==0))
		{
			if(app_webapi_language_setting_busy())
			{
				WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_DEVICE_BUSY));
				app_webapi_server_fail(nc, WEBAPI_DEVICE_BUSY);
			}
			else
			{
				if(menu_set>=0)
				{
					GxBus_ConfigSetInt(OSD_LANG_KEY,menu_set);
					GUI_SetInterface("osd_language",g_LangName[menu_set]);
					char* focus_Window = (char*)GUI_GetFocusWindow();
					if(focus_Window)
					{
						WEBAPI_PRINTF("update window[%s]\n", focus_Window);
						GUI_SetProperty(focus_Window, "update", NULL);
						if(0 == strcasecmp(WND_MAIN_MENU_ITEM ,focus_Window))
						{
							GUI_SetProperty(WND_MAIN_MENU, "update", NULL);
						}

					}
				}
				if(audio1_set>=0)
				{
					GxBus_ConfigSetInt(AUDIO_1ST_KEY,audio1_set);
				}
				if(audio2_set>=0)
				{
					GxBus_ConfigSetInt(AUDIO_2ND_KEY,audio2_set);
				}
				if(subtitle_set>=0)
				{
					GxBus_ConfigSetInt(SUBTILE_1ST_KEY,subtitle_set);
				}
				if(ttx_set>=0)
				{
					GxBus_ConfigSetInt(TTX_LANG_KEY,ttx_set);
				}
				if(epg_set>=0)
				{
					GxBus_ConfigSetInt(EPG_LANG_KEY,epg_set);
				}
				app_webapi_server_success(nc);
			}
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

void app_webapi_server_language(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_language_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_language_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
