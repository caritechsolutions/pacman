#include "app.h"
#include "app_send_msg.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include "app_module.h"
#include <gx_apps.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

typedef struct _CMDInClass{
	char *intension;
	char *mode;
	char *para;
}CMDInClass;
#if VOICE_CONTROL_SUPPORT
extern int app_vc_voicecmd_process(CMDInClass cmdin);
#endif
static void app_webapi_server_cmd_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	char *intention = NULL;
	char *mode = NULL;
	char *para = NULL;
	char buffer[32];
	CMDInClass cmdin = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				//cmd name
				json1 = cJSON_GetObjectItem(root,"intention");
				if((json1) && (json1->type == cJSON_String))
				{
					intention = json1->valuestring;
					WEBAPI_PRINTF("%s[%d] intention=%s\n",__FUNCTION__, __LINE__, intention);
				}

				//cmd mode
				json1 = cJSON_GetObjectItem(root,"mode");
				if((json1) && (json1->type == cJSON_String))
				{
					mode = json1->valuestring;
					WEBAPI_PRINTF("%s[%d] mode=%s\n",__FUNCTION__, __LINE__, mode);
				}

				//cmd para
				json1 = cJSON_GetObjectItem(root, "para");
				if(json1)
				{
					if(json1->type == cJSON_String)
					{
						para = json1->valuestring;
						WEBAPI_PRINTF("%s[%d] para=%s\n",__FUNCTION__, __LINE__, para);
					}
					else if (json1->type == cJSON_Number)
					{
						memset(buffer, 0, sizeof(buffer));
						sprintf(buffer, "%d", json1->valueint);
						para = buffer;
						WEBAPI_PRINTF("%s[%d] para=%s\n",__FUNCTION__, __LINE__, para);
					}
				}

				if(intention)
				{
					cmdin.intension = intention;
					if(mode)
					{
						cmdin.mode = mode;
					}
					else
					{
						cmdin.mode = "none";
					}
					if(para)
					{
						cmdin.para = para;
					}
					else
					{
						cmdin.para = "none";
					}
					WEBAPI_PRINTF("%s[%d] cmdin.intension=%s, cmdin.mode=%s, cmdin.para=%s\n",__FUNCTION__, __LINE__, cmdin.intension, cmdin.mode, cmdin.para);
#if VOICE_CONTROL_SUPPORT
					app_vc_voicecmd_process(cmdin);
#endif
					app_webapi_server_success(nc);
				}
				else
				{
					WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
					app_webapi_server_fail(nc, WEBAPI_INTERNAL_ERR);
				}
				cJSON_Delete(root);
			}
			free(data);
		}
		else
		{
			WEBAPI_PRINTF("%s[%d]error=%s\n",__FUNCTION__, __LINE__, webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
			app_webapi_server_fail(nc, WEBAPI_INTERNAL_ERR);
		}
	}
	else
	{
		app_webapi_server_fail(nc, WEBAPI_INTERNAL_ERR);
	}
}

void app_webapi_server_cmd(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_cmd_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
