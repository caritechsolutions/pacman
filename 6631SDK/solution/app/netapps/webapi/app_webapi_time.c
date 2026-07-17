#include "app.h"
#include "app_send_msg.h"

#if WEBAPI_SUPPORT
#include <gx_apps.h>
#include "module/app_time.h"
#include "app_webapi.h"
#include "app_webapi_priv.h"

extern void app_webapi_time_get(int *timezone, int *summer_time, int *year, int *month, int *day, int *hour, int *min, int *sec);
extern void app_webapi_time_set(int timezone, int summer_time, int year, int month, int day, int hour, int min, int sec);

static void app_webapi_server_time_read(struct mg_connection* nc, struct http_message* hm)
{
	int timezone = 0;
	int summer_time =0;
	int year = 0;
	int month = 0;
	int day = 0;
	int hour = 0;
	int min = 0;
	int sec = 0;
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *out = NULL;

	app_webapi_time_get(&timezone, &summer_time, &year, &month, &day, &hour, &min, &sec);
	WEBAPI_PRINTF("%s[%d/%d/%d/%d/%d/%d/%d/%d]\n",__FUNCTION__, timezone, summer_time, year, month, day, hour, min, sec);
	if((timezone>=-12)&&(timezone<=12)&&(year>=1970)&&(year<=2038)
		&&(month>=1)&&(month<=12)&&(day>=1)&&(day<=31)
		&&(hour>=0)&&(hour<=23)&&(min>=0)&&(min<=59)
		&&(sec>=0)&&(sec<=59))
	{
		if(summer_time!=0)
		{
			summer_time = 1;
		}
		root = cJSON_CreateObject();
		cJSON_AddStringToObject(root, WEBAPI_STATUS_STR, WEBAPI_OK_STR);
		json1 = cJSON_CreateObject();
		cJSON_AddItemToObject(root, WEBAPI_TIME_STR, json1);
		cJSON_AddNumberToObject(json1, WEBAPI_TIMEZONE_STR, timezone);
		cJSON_AddNumberToObject(json1, WEBAPI_SUMMERTIME_STR, summer_time);
		cJSON_AddNumberToObject(json1, WEBAPI_YEAR_STR, year);
		cJSON_AddNumberToObject(json1, WEBAPI_MONTH_STR, month);
		cJSON_AddNumberToObject(json1, WEBAPI_DAY_STR, day);
		cJSON_AddNumberToObject(json1, WEBAPI_HOUR_STR, hour);
		cJSON_AddNumberToObject(json1, WEBAPI_MIN_STR, min);
		cJSON_AddNumberToObject(json1, WEBAPI_SEC_STR, sec);
		out = cJSON_PrintUnformatted(root);
		cJSON_Delete(root);

		mg_printf(nc, WEBAPI_SERVER_HEADER);
		mg_send_http_chunk(nc, out, strlen(out));
		mg_send_http_chunk(nc, "", 0);

		free(out);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_INTERNAL_ERR));
		app_webapi_server_fail(nc, WEBAPI_INTERNAL_ERR);
	}
}

static void app_webapi_server_time_write(struct mg_connection* nc, struct http_message* hm)
{
	cJSON *root = NULL;
	cJSON *json1 = NULL;
	char *data = NULL;
	int timezone = 100;
	int summer_time =-1;
	int year = -1;
	int month = -1;
	int day = -1;
	int hour = -1;
	int min = -1;
	int sec = -1;
	time_t calc_sec = 0;

	WEBAPI_PRINTF("%s\n",__FUNCTION__);
	if(hm&&(hm->body.p)&&(hm->body.len>0))
	{
		data = gx_strndup(hm->body.p, hm->body.len);
		if(data)
		{
			root = cJSON_Parse(data);
			if(root)
			{
				json1 = cJSON_GetObjectItem(root,"timezone");
				if((json1) && (json1->type == cJSON_Number))
				{
					timezone = json1->valueint;
					WEBAPI_PRINTF("timezone=%d\n",timezone);
				}
				json1 = cJSON_GetObjectItem(root,"summer_time");
				if((json1) && (json1->type == cJSON_Number))
				{
					summer_time = json1->valueint;
					WEBAPI_PRINTF("summer_time=%d\n",summer_time);
				}
				json1 = cJSON_GetObjectItem(root,"year");
				if((json1) && (json1->type == cJSON_Number))
				{
					year = json1->valueint;
					WEBAPI_PRINTF("year=%d\n",year);
				}
				json1 = cJSON_GetObjectItem(root,"month");
				if((json1) && (json1->type == cJSON_Number))
				{
					month = json1->valueint;
					WEBAPI_PRINTF("month=%d\n",month);
				}
				json1 = cJSON_GetObjectItem(root,"day");
				if((json1) && (json1->type == cJSON_Number))
				{
					day = json1->valueint;
					WEBAPI_PRINTF("day=%d\n",day);
				}
				json1 = cJSON_GetObjectItem(root,"hour");
				if((json1) && (json1->type == cJSON_Number))
				{
					hour = json1->valueint;
					WEBAPI_PRINTF("hour=%d\n",hour);
				}
				json1 = cJSON_GetObjectItem(root,"min");
				if((json1) && (json1->type == cJSON_Number))
				{
					min = json1->valueint;
					WEBAPI_PRINTF("min=%d\n",min);
				}
				json1 = cJSON_GetObjectItem(root,"sec");
				if((json1) && (json1->type == cJSON_Number))
				{
					sec = json1->valueint;
					WEBAPI_PRINTF("sec=%d\n",sec);
				}
				cJSON_Delete(root);
			}
			free(data);
		}
		if((timezone>=-12)&&(timezone<=12)&&(year>=1970)&&(year<=2038)
			&&(month>=1)&&(month<=12)&&(day>=1)&&(day<=31)
			&&(hour>=0)&&(hour<=23)&&(min>=0)&&(min<=59)
			&&(sec>=0)&&(sec<=59))
		{
			if(app_month_date_valid(year, month, day) == true)
			{
				calc_sec = app_mktime(year, month, day, hour, min, sec);
				calc_sec = app_time_to_utc_time(calc_sec, timezone, 0, summer_time);
				if(app_time_range_valid(calc_sec))
				{
					app_webapi_time_set(timezone, summer_time, year, month, day, hour, min, sec);
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

void app_webapi_server_time(struct mg_connection* nc, struct http_message* hm)
{
	if(mg_vcasecmp(&hm->method, "GET") == 0)
	{
		app_webapi_server_time_read(nc, hm);
	}
	else if(mg_vcasecmp(&hm->method, "POST") == 0)
	{
		app_webapi_server_time_write(nc, hm);
	}
	else
	{
		WEBAPI_PRINTF("%s\n", webapi_get_status_str(WEBAPI_METHOD_NOT_SUPPORT));
		app_webapi_server_fail(nc, WEBAPI_METHOD_NOT_SUPPORT);
	}
}

#endif
