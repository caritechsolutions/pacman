#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "app_pop.h"

#if WEBAPI_SUPPORT
#include "app_windows.h"
#include <gx_apps.h>
#include <gx_webapi.h>
#include "app_webapi.h"
#include "app_webapi_priv.h"

static int webapi_server_run = 0;
static int webapi_server_threadid = 0;

static int webapi_get_ip(char *ip)
{
    GxMsgProperty_NetDevIpcfg s_ip_cfg;

    memset(&s_ip_cfg, 0, sizeof(GxMsgProperty_NetDevIpcfg));
    app_send_msg_exec(GXMSG_NETWORK_GET_CUR_DEV, &s_ip_cfg.dev_name);
    if(s_ip_cfg.dev_name[0] == 0)
        return -1;
    app_send_msg_exec(GXMSG_NETWORK_GET_IPCFG, &s_ip_cfg);
    strcpy(ip, s_ip_cfg.ip_addr);
    return 0;
}

static void webapi_ev_handler(struct mg_connection* nc, int ev, void* ev_data)
{
	struct http_message* hm = (struct http_message*)ev_data;
	char *full_uri = NULL;
	char *resource_url = NULL;
	int version = 0;

	if(ev != MG_EV_HTTP_REQUEST)
	{
		return;
	}

	if(hm->uri.len>1)
	{
		full_uri = gx_strndup(hm->uri.p, hm->uri.len);
		WEBAPI_PRINTF("full_uri=%s\n", full_uri);
		resource_url = (char *)malloc(hm->uri.len+1);
		memset(resource_url, 0, hm->uri.len+1);
		if(2 == sscanf(full_uri, " /api/v%d/%s", &version, resource_url))
		{
			WEBAPI_PRINTF("version=%d,resource=%s\n", version, resource_url);
			if(version == WEBAPI_VERSION)
			{
				if(strcmp(resource_url, "volume") == 0)
				{
					app_webapi_server_volume(nc, hm);
				}
				else if(strcmp(resource_url, "time") == 0)
				{
					app_webapi_server_time(nc, hm);
				}
				else if(strcmp(resource_url, "language") == 0)
				{
					app_webapi_server_language(nc, hm);
				}
				else if(strcmp(resource_url, "av") == 0)
				{
					app_webapi_server_av(nc, hm);
				}
				else if(strcmp(resource_url, "gui") == 0)
				{
					app_webapi_server_gui(nc, hm);
				}
				else if(strcmp(resource_url, "infor") == 0)
				{
					app_webapi_server_infor(nc, hm);
				}
				else if(strcmp(resource_url, "key") == 0)
				{
					app_webapi_server_key(nc, hm);
				}
				else if(strcmp(resource_url, "frontend") == 0)
				{
					app_webapi_server_frontend(nc, hm);
				}
				else if(strcmp(resource_url, "satellite") == 0)
				{
					app_webapi_server_satellite(nc, hm);
				}
				else if(strcmp(resource_url, "antenna") == 0)
				{
					app_webapi_server_antenna(nc, hm);
				}
				else if(strcmp(resource_url, "transponder") == 0)
				{
					app_webapi_server_transponder(nc, hm);
				}
				else if(strcmp(resource_url, "category") == 0)
				{
					app_webapi_server_category(nc, hm);
				}
				else if(strcmp(resource_url, "program") == 0)
				{
					app_webapi_server_program(nc, hm);
				}
				else if(strcmp(resource_url, "signal") == 0)
				{
					app_webapi_server_signal(nc, hm);
				}
				else if(strcmp(resource_url, "search") == 0)
				{
					app_webapi_server_search(nc, hm);
				}
				else if(strcmp(resource_url, "play") == 0)
				{
					app_webapi_server_play(nc, hm);
				}
				else if(strcmp(resource_url, "disk") == 0)
				{
					app_webapi_server_disk(nc, hm);
				}
				else if(strcmp(resource_url, "file") == 0)
				{
					app_webapi_server_file(nc, hm);
				}
				else if(strcmp(resource_url, "status") == 0)
				{
					app_webapi_server_status(nc, hm);
				}
				else if (strcmp(resource_url, "cmd") == 0)
				{
					app_webapi_server_cmd(nc, hm);
				}
				else
				{
					WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_FUNCTION_NOT_EXIST));
					app_webapi_server_fail(nc, WEBAPI_FUNCTION_NOT_EXIST);
				}
			}
			else
			{
				WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_VERSION_ERR));
				app_webapi_server_fail(nc, WEBAPI_VERSION_ERR);
			}
		}
		else
		{
			WEBAPI_PRINTF("%s, %s, %d error\n",__FILE__,__FUNCTION__,__LINE__);
			mg_http_send_error(nc, 403, 0);
		}
		free(resource_url);
		free(full_uri);
	}
	else
	{
		WEBAPI_PRINTF("%s, %s, %d error\n",__FILE__,__FUNCTION__,__LINE__);
		mg_http_send_error(nc, 403, 0);
	}
}

static void webapi_serverThread(void* arg)
{
	struct mg_mgr mgr;
	struct mg_connection* nc;
	struct mg_bind_opts bind_opts;
	const char* err_str;

	GxCore_ThreadDetach();
	mg_mgr_init(&mgr, 0);

	/* Set HTTP server options */
	memset(&bind_opts, 0, sizeof(bind_opts));
	bind_opts.error_string = &err_str;

	nc = mg_bind_opt(&mgr, WEBAPI_PORT, webapi_ev_handler, bind_opts);
	if(!nc)
	{
		webapi_server_run = 0;
		WEBAPI_PRINTF("Error starting server on port %s: %s\n", WEBAPI_PORT, *bind_opts.error_string);
		return;
	}

	mg_set_protocol_http_websocket(nc);

	WEBAPI_PRINTF("Starting webapi server on...\n");
	while(webapi_server_run) mg_mgr_poll(&mgr, 1000);
	mg_mgr_free(&mgr);

	WEBAPI_PRINTF("webapi server off...\n");
	return;
}

void app_webapi_start(char *device_name)
{
	ubyte32 ip_addr = {0};

	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(webapi_get_ip(ip_addr) < 0)
	{
		WEBAPI_PRINTF("%s[%d]ip address get error!\n",__FUNCTION__, __LINE__);
		return;
	}
	if(webapi_server_run == 0)
	{
		webapi_server_run = 1;
		GxCore_ThreadCreate("webapi_server",&webapi_server_threadid,webapi_serverThread,NULL,32*1024,GXOS_DEFAULT_PRIORITY);
	}
	gx_webapi_start(ip_addr, atoi(WEBAPI_PORT), WEBAPI_VERSION, device_name);
}

void app_webapi_stop(void)
{
	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	if(GUI_CheckDialog(WND_WEBAPI_NETPLAY) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_WEBAPI_NETPLAY);
	}
	gx_webapi_stop();
	if(webapi_server_run && webapi_server_threadid)
	{
		webapi_server_run = 0;
		//GxCore_ThreadJoin(webapi_server_threadid);
	}
	webapi_server_run = 0;
	webapi_server_threadid = 0;
}

void app_webapi_server_success(struct mg_connection* nc)
{
	cJSON *root = NULL;
	char *out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);

	free(out);
}

void app_webapi_server_fail(struct mg_connection* nc, webapi_ret_t err_ret)
{
	cJSON *root = NULL;
	char *out = NULL;

	root = cJSON_CreateObject();
	cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_FAIL_STR);
	cJSON_AddStringToObject(root, WEBAPI_ERRMSG_STR, webapi_get_status_str(err_ret));
	cJSON_AddNumberToObject(root, WEBAPI_ERRCODE_STR, err_ret);
	out = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);

	mg_printf(nc, WEBAPI_SERVER_HEADER);
	mg_send_http_chunk(nc, out, strlen(out));
	mg_send_http_chunk(nc, "", 0);

	free(out);
}

#endif
