#include "app.h"
#include "app_send_msg.h"
#include "app_module.h"
#include "app_pop.h"
#include "app_windows.h"

#if WEBAPI_SUPPORT
#include "app_webapi.h"
#include "app_webapi_priv.h"

static const char *webapi_status_str[] = {
	"ok", //WEBAPI_SUCCESS
	"Internal error!", //WEBAPI_INTERNAL_ERR
	"Function not exist!", //WEBAPI_FUNCTION_NOT_EXIST
	"Device busy!", //WEBAPI_DEVICE_BUSY
	"Function parameters error!", //WEBAPI_FUNCTION_PARAM_ERR
	"Version error!", //WEBAPI_VERSION_ERR
	"Method not support!", //WEBAPI_METHOD_NOT_SUPPORT
	"Max Satellite!", //WEBAPI_SATELLITE_LIMIT_ERR
	"Satellite not exist!", //WEBAPI_SATELLITE_NOT_EXIST
	"Invalid TP!", //WEBAPI_TP_INVALID
	"Max TP!", //WEBAPI_TP_LIMIT_ERR
	"TP already exist!", //WEBAPI_TP_EXIST
	"TP not exist!", //WEBAPI_TP_NOT_EXIST
	"Operation invalid!", //WEBAPI_OPERATION_INVALID
	"Disk not exist!", //WEBAPI_DISK_NOT_EXIST
	"Satellite already exist!", //WEBAPI_SATELLITE_EXIST
};

const char *webapi_get_status_str(webapi_ret_t ret)
{
	if((ret<0)||(ret>=WEBAPI_RET_MAX))
	{
		ret = WEBAPI_SUCCESS;
	}
	return webapi_status_str[ret];
}

static void app_webapi_server_status_read(struct mg_connection* nc, struct http_message* hm)
{
	WEBAPI_PRINTF("%s[%d]\n",__FUNCTION__, __LINE__);
	app_webapi_server_success(nc);
}

void app_webapi_server_status(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_status_read(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
