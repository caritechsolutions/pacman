/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"
#include "app_windows.h"
#include "service/gxhotplug.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <common/gxpm.h>
#include "gx_apps.h"

#include "app_skin_download.h"

#define MAX_PATH_LEN     (128)



static handle_t     download_file_handle = 0;
static int          download_file_abort  = 0;
static int          download_file_is_downloading = 0 ;
static unsigned int download_file_size   = 0 ;
static event_list*  download_file_timer = NULL;

skin_gxapp_timer_callback        download_timer_cb = NULL ;
skin_gxapp_timer_multi_callback  download_timer_multi_cb = NULL ;


static int          download_pic_abort  = 0;
static event_list*  download_pic_timer = NULL;

static handle_t     download_pic_handle[SKIN_DOWNLOAD_PIC_MAX_NUM] = {0};
static char        *download_pic_path[SKIN_DOWNLOAD_PIC_MAX_NUM] = {NULL};
static int          download_pic_download_ok[SKIN_DOWNLOAD_PIC_MAX_NUM];
static int          download_pic_show_ok[SKIN_DOWNLOAD_PIC_MAX_NUM];


/////

int gxapp_skin_download_api_callback_stop(void)
{
	gxapp_skin_download_api_timer_stop();

	if(download_file_handle!=0)
    {
		//printf("[sk download] callback stop \n");
        gxapps_curl_async_abort(download_file_handle);
        download_file_handle = 0;
        gxapps_curl_set_timeout(0, 0);
    }

	return 0;
}

int   gxapp_skin_download_delete_img_file(void)
{
	if (GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(IMG_DOWN_PATH);
	}
	return 0 ;
}

static void _gxapp_skin_download_delete_download_file(void)
{
	if (GxCore_FileExists(DIR_DOWN_PATH) == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(DIR_DOWN_PATH);
	}
	if (GxCore_FileExists(XML_DOWN_PATH) == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(XML_DOWN_PATH);
	}
	if (GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(IMG_DOWN_PATH);
	}
}

int gxapp_skin_download_api_download_stop(void)
{
	download_file_abort = 1;

	gxapp_skin_download_api_timer_stop();

    if(download_file_handle!=0)
    {
		//printf("[sk download] download stop \n");
        gxapps_curl_async_abort(download_file_handle);
        download_file_handle = 0;
        gxapps_curl_set_timeout(0, 0);
    }
    download_file_is_downloading = 0;

	_gxapp_skin_download_delete_download_file();

	return 0 ;
}

int gxapp_skin_download_api_download_start(void)
{
	download_file_abort = 0;
	download_file_is_downloading = 2;
	download_file_size = 0 ;

	gxapps_curl_set_timeout(30, 30);

	return 0 ;
}

static int _gxapp_skin_download_api_timer_func(void *usrdata)
{
	if( download_file_is_downloading <= 1)
	{
		if ( NULL != download_timer_cb )
		{
			download_timer_cb(download_file_is_downloading,download_file_size);
		}
	}
	return 0;
}

int gxapp_skin_download_api_timer_start(skin_gxapp_timer_callback cb, int time)
{
	download_timer_cb = cb ;

	if (reset_timer(download_file_timer) != 0)
	{
		download_file_timer = create_timer(_gxapp_skin_download_api_timer_func, time, NULL, TIMER_REPEAT);
	}

	//download_file_timer = create_timer(_gxapp_skin_download_api_timer_func, time, NULL, TIMER_REPEAT);
	return 0 ;
}

int gxapp_skin_download_api_timer_stop(void)
{
	if(download_file_timer)
	{
		remove_timer(download_file_timer);
		download_file_timer = NULL;
	}

	download_timer_cb = NULL ;

	return 0 ;
}

static void _gxapp_skin_download_api_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;

    if(download_file_abort)
        return;
    //printf("[sk download] cb msg = %d,%d \n",message,GXCURL_STATE_COMPLETE);
    if(message == GXCURL_STATE_COMPLETE)
    {
        if( (callback_data->handle > 0) && (download_file_handle == callback_data->handle) )
        {
            if(callback_data->complete_data.size > 0)
            {
                if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                {
                    //printf("[sk download] *****file ok = %d *****\n",callback_data->complete_data.size);
					download_file_size = callback_data->complete_data.size ;
					download_file_is_downloading = 1;
                }
                else
                {
				    download_file_size = 0 ;
                    app_log_error("[skin download]*****file error*****\n");
                }
            }
		    else
		    {
				download_file_size = 0 ;
			    app_log_error("[skin download]*****file size=0****error*****\n");
		    }
        }
		if ( download_file_is_downloading != 1 )
		{
			download_file_is_downloading = 0 ;
		}
		gxapps_curl_set_timeout(0, 0);
    }
}

int gxapp_skin_download_api_async_download(const char *url,char *path)
{
    download_file_handle = gxapps_curl_async_download(url,path,_gxapp_skin_download_api_cb);
	if ( download_file_handle == 0 )
	{
		gxapp_skin_download_api_timer_stop();
		return -1 ;
	}

	return 0 ;
}

int gxapp_skin_download_api_async_download_ext(const char *url,char *buffer,unsigned int size)
{
	download_file_handle = gxapps_curl_async_download_ext(url,NULL,NULL,
			buffer,size,_gxapp_skin_download_api_cb);

	if ( download_file_handle == 0 )
	{
		gxapp_skin_download_api_timer_stop();
		return -1 ;
	}

	return 0 ;
}

////////////////////

static int _gxapp_skin_download_api_multi_timer_func(void *userdata)
{
	int i=0;
	bool b_all_ok = true;

	for(i = 0; i < SKIN_DOWNLOAD_PIC_MAX_NUM; i++)
	{
		if(download_pic_show_ok[i] == 1)
		{
			continue;
		}
		b_all_ok = false;
		if(download_pic_download_ok[i] == 1)
		{
			if ( NULL != download_timer_multi_cb )
			{
				download_timer_multi_cb(i,download_pic_path[i]);
			}

			if(download_pic_handle[i]!=0)
			{
				gxapps_curl_async_abort(download_pic_handle[i]);
				download_pic_handle[i] = 0;
			}

			download_pic_show_ok[i] = 1;
		}
		else if(download_pic_download_ok[i] == -1)
		{
			if(download_pic_handle[i]!=0)
			{
				gxapps_curl_async_abort(download_pic_handle[i]);
				download_pic_handle[i] = 0;
			}

			if ( NULL != download_timer_multi_cb )
			{
				download_timer_multi_cb(i,NULL);
			}

			download_pic_show_ok[i] = 1;
		}
	}

	if(b_all_ok)
	{
		GUI_SetInterface("flush",NULL);

		gxapp_skin_download_api_multi_timer_stop();
	}
	return 0;
}

int gxapp_skin_download_api_multi_timer_start(skin_gxapp_timer_multi_callback cb, int time)
{
	download_timer_multi_cb = cb ;

	if (reset_timer(download_pic_timer) != 0)
	{
		download_pic_timer = create_timer(_gxapp_skin_download_api_multi_timer_func, time, NULL, TIMER_REPEAT);
	}

	return 0 ;
}

int gxapp_skin_download_api_multi_timer_stop(void)
{
	if(download_pic_timer)
	{
		remove_timer(download_pic_timer);
		download_pic_timer = NULL;
	}

	download_timer_multi_cb = NULL ;

	return 0;
}

static void _gxapp_skin_download_api_multi_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
    int i = 0;

    if(download_pic_abort)
        return;

    if(message == GXCURL_STATE_COMPLETE)
    {
        if(callback_data->handle > 0)
        {
            for(i = 0; i < SKIN_DOWNLOAD_PIC_MAX_NUM; i++)
            {
                if(download_pic_handle[i] == callback_data->handle && callback_data->complete_data.size > 0)
                {
                    if(callback_data->complete_data.retcode == GXAPPS_SUCCESS)
                    {
                        download_pic_download_ok[i] = 1;
                        app_log_info("[skin download] **%d**ok**\n", i);
                    }
                    else
                    {
                        download_pic_download_ok[i] = -1;
                        app_log_error("[skin download] **%d**error**\n", i);
                    }
                }
				else if(download_pic_handle[i] == callback_data->handle && callback_data->complete_data.size <= 0)
				{
					download_pic_download_ok[i] = -1;
                    app_log_error("[skin download] *****%d****size error*****\n", i);
				}
            }
        }
    }
}

static void _app_skin_download_delete_download_pic(void)
{
	int i=0;
	char image_path[MAX_PATH_LEN];

	for(i=0; i<SKIN_DOWNLOAD_PIC_MAX_NUM; i++)
	{
        snprintf(image_path, sizeof(image_path), "%s%d%s", PIC_DOWN_PATH, i, PIC_PATH_SUFFIX);
        if(GXCORE_FILE_EXIST == GxCore_FileExists(image_path))
		{
			GxCore_FileDelete(image_path);
		}
	}
}

int gxapp_skin_download_api_multi_download_start(void)
{
	download_pic_abort = 0 ;

	gxapps_curl_set_timeout(30, 30);

	return 0 ;
}

int gxapp_skin_download_api_multi_download_stop(void)
{
	int i=0;

	download_pic_abort = 1;

	gxapp_skin_download_api_multi_timer_stop();

	memset(&download_pic_download_ok, 0, sizeof(download_pic_download_ok));
	memset(&download_pic_show_ok, 0, sizeof(download_pic_show_ok));

	for(i=0; i<SKIN_DOWNLOAD_PIC_MAX_NUM; i++)
	{
		if(download_pic_handle[i]!=0)
		{
			gxapps_curl_async_abort(download_pic_handle[i]);
			download_pic_handle[i] = 0;
		}
        APP_FREE(download_pic_path[i]);
	}

	_app_skin_download_delete_download_pic();

	return 0 ;
}

int gxapp_skin_download_api_multi_async_download(const char *url,int index)
{
	uint32_t length = 0;
	char *path = NULL;

	if ( url == NULL )
	{
		//printf(" skin download multi url null,id=%d \n",index);
		download_pic_show_ok[index] = 1 ;
        return -1;
	}

	length = strlen(PIC_DOWN_PATH) + strlen(PIC_PATH_SUFFIX) + 20;
    if(length >= MAX_PATH_LEN)
    {
        app_log_error("[skin download] path length (%d) limit\n", MAX_PATH_LEN);
        return -1;
    }

    path = GxCore_Calloc(sizeof(char), length);
    if(NULL == path)
    {
        app_log_error("[skin download] No memory to GxCore_Calloc\n");
        return -2;
    }

	snprintf(path, length, "%s%d%s", PIC_DOWN_PATH, index, PIC_PATH_SUFFIX);
	APP_FREE(download_pic_path[index]);

    download_pic_path[index] = path;
    download_pic_handle[index] = gxapps_curl_async_download(url, path, _gxapp_skin_download_api_multi_cb);

	return 0 ;
}

int gxapp_skin_download_api_multi_is_download_finish(int index)
{
	if ( (index >=0) && (index < SKIN_DOWNLOAD_PIC_MAX_NUM) )
	{
		return download_pic_show_ok[index] ;
	}
	return 0 ;
}

#endif
