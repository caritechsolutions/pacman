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
#include <dirent.h>
#include "gx_apps.h"

#include "app_skin_setting.h"
#include "app_skin_config.h"
#include "app_skin_xml.h"
#include "app_skin_flash.h"
#include "app_skin_download.h"
#include "app_skin_str_id.h"

/*
  recovery 模式，默认是支持usb，为了扩展方便，用宏定义不支持USB。
  由于代码是先实现online和usb 2种模式一起支持。当后续为实现不支持usb的时候，方便起见,只是
  在创建菜单初始化不去获取usb数据，消息不处理，同时菜单左右切换不支持，模式一直online,
  其它全局数据和代码都是2种支持的。后续需要优化的化，可以再考虑，因为这些数据和代码不是很大。
  2023.12.12
*/
#define SK_RECOVERY_USB_SUPPORT 1

/////
#define SK_RECOVERY_MODE_ONLNE 1
#define SK_RECOVERY_MODE_USB   2

static int s_skrecovery_update_mode = 0 ;
static int s_skrecovery_update_key[4] ;
static char s_skrecovery_content_mode[20] = {"[Online,USB]"} ;

static int s_skrecovery_mode = SK_RECOVERY_MODE_USB;
static int s_skrecovery_online_status = 0 ;
static int s_skrecovery_usb_status = 0 ;
static int s_skrecovery_status = 0 ;
static int s_skrecovery_img_sel = 0 ;
static int s_skrecovery_list_num = 0 ;
static char* s_skrecovery_simg_md5 = NULL ;
static char* s_skrecovery_dimg_md5 = NULL ;

static skin_data_node_t s_skrecovery_node_online;
static skin_data_node_t s_skrecovery_node_usb;

static uint32_t s_skrecovery_flash_size = 0;
#ifdef ECOS_OS
static GxHwMallocObj s_skrecovery_cache_buf;
#endif

//////////////////

extern int app_upgrade_crc_check(char *path) ;
extern int app_upgrade_crc_check_buf(char *p_data, int size);

static int app_skrecovery_ui_update_mode(int update_list);
static void _app_skrecovery_ui_update_img_tips(char *tips);
static void _app_skrecovery_ui_update_img_proc(char *file_name,char *smd5,char *dmd5);
static void _app_skrecovery_ui_xml_download(void);

static void _app_skrecovery_online_callback_get_data(void)
{
	int cur = 0 ;
	int ret = 0 ;

	int down_index = 0 ;
	char xml_path[FULL_XML_PATH_LENGTH] ;

	cur = s_skrecovery_node_online.num ;
	if (s_skrecovery_node_online.cur_down_num <= 0 )
	{
		SKIN_DBG("[skin recovery] callback get data down index err, nums=%d \n",cur);
		return ;
	}
	down_index = s_skrecovery_node_online.cur_down_num - 1 ;

	if ( (cur < SKIN_MAX_LIST) && ( down_index < SKIN_MAX_LIST) && (cur <= down_index) )
	{
		SKIN_DBG("[skin recovery] callback get data nums = %d, down id = %d \n",cur,down_index);

		if (s_skrecovery_node_online.node[down_index].ddir == NULL)
		{
			SKIN_DBG("[skin recovery] callback get data ddir err \n");
			return ;
		}

		memset(xml_path,0,FULL_XML_PATH_LENGTH);
		snprintf(xml_path, FULL_XML_PATH_LENGTH, "%s/%s",PATH_VER_MAIN,
				s_skrecovery_node_online.node[down_index].ddir);

		APP_FREE(s_skrecovery_node_online.node[cur].ddir);
		s_skrecovery_node_online.node[cur].ddir = GxCore_Strdup(xml_path);

		ret = gxapp_file_get_data(XML_DOWN_PATH,&s_skrecovery_node_online.node[cur]);
		if (ret == 0 )
		{
			s_skrecovery_node_online.num++ ;
			/*
			if (s_skrecovery_node_online.node[cur].uiid > 0)
			{
				if ( gxapp_exist_installed_uiid(s_skrecovery_node_online.node[cur].uiid) )
				{
					s_skrecovery_node_online.node[cur].uninstall = 1 ;
				}
			}*/
		}
		else
		{
			SKIN_DBG("[skin recovery] callback ret = %d \n",ret);
		}
	}
	else
	{
		SKIN_DBG("[skin recovery] callback over cur =%d, %d \n",cur,down_index);
	}
}

static int _app_skrecovery_download_xml_callback(int down_flag,unsigned int down_size)
{
	SKIN_DBG("[skin recovery] xml callback =%d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if ((down_flag <=0) || (GxCore_FileExists(XML_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
	{
		SKIN_DBG("[skin recovery] #### xml down failer \n");
	}
	else
	{
		SKIN_DBG("[skin recovery] xml down ok \n");
		_app_skrecovery_online_callback_get_data();
	}

	_app_skrecovery_ui_xml_download();

	if (s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE)
	{
		SKIN_DBG("[skin recovery] online focus \n");

		app_skrecovery_ui_update_mode(1);
	}

	return 0;
}

static int _app_skrecovery_download_img_callback(int down_flag,unsigned int down_size)
{
	SKIN_DBG("[skin recovery] img callback = %d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if (s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE)
	{
		SKIN_DBG("[skin recovery] img focus \n");

		if ((down_flag <=0) || (GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
		{
			SKIN_DBG("[skin recovery] #### img down failer \n");
			_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_TIPS_DOWNLOAD_FAILER);
		}
		else
		{
			SKIN_DBG("[skin recovery] img down ok \n");
			_app_skrecovery_ui_update_img_proc(IMG_DOWN_PATH,s_skrecovery_simg_md5,s_skrecovery_dimg_md5);
		}
	}

	return 0;
}

static void _app_skrecovery_ui_xml_download(void)
{
	int i = 0 ;
	int cur = 0 ;
	int down_all_num = 0 ;
	char file_path[FULL_XML_PATH_LENGTH] ;

	down_all_num = s_skrecovery_node_online.dir_all_num ;
	cur = s_skrecovery_node_online.cur_down_num ;
	if ( (down_all_num <=0) || (cur>=down_all_num) )
	{
		SKIN_DBG("[skin recovery] online xml download exit,down id = %d,down all = %d,nums=%d \n",cur,
				down_all_num,s_skrecovery_node_online.num);

		if (s_skrecovery_node_online.num < down_all_num)
		{
			i= s_skrecovery_node_online.num;
			for(;i<down_all_num;i++)
			{
				APP_FREE(s_skrecovery_node_online.node[i].ddir);
			}
			s_skrecovery_node_online.dir_all_num = s_skrecovery_node_online.num ;
		}

		return ;
	}
	if ( s_skrecovery_node_online.node[cur].ddir == NULL )
	{
		SKIN_DBG("[skin recovery] online xml download exit2,index = %d,%d \n",cur,down_all_num);
		return ;
	}

	SKIN_DBG("[skin recovery] online xml download,down id=%d,down all=%d, nums = %d \n",cur,
				down_all_num,s_skrecovery_node_online.num);

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skrecovery_download_xml_callback, 10);

	memset(file_path,0,FULL_XML_PATH_LENGTH);
	snprintf(file_path, FULL_XML_PATH_LENGTH, "%s/%s/%s/%s",gxapp_skin_online_get_url(),PATH_VER_MAIN,
					s_skrecovery_node_online.node[cur].ddir,SKIN_XML_NAME);

	SKIN_DBG("[skin recovery] online xml url = %s \n",file_path);

	gxapp_skin_download_api_async_download(file_path,XML_DOWN_PATH);

	s_skrecovery_node_online.cur_down_num++;
}

static int _app_skrecovery_download_dir_callback(int down_flag,unsigned int down_size)
{
	SKIN_DBG("[skin recovery] dir callback =%d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if ((down_flag <=0) || (GxCore_FileExists(DIR_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
	{
		SKIN_DBG("[skin recovery] #### dir down failer \n");
	}
	else
	{
		SKIN_DBG("[skin recovery] dir down ok \n");

		gxapp_htmlfile_get_data(DIR_DOWN_PATH,&s_skrecovery_node_online);

		if ( s_skrecovery_node_online.dir_all_num > 0 )
		{
			_app_skrecovery_ui_xml_download();
		}
	}

	return 0;
}

static void _app_skrecovery_ui_dir_download(void)
{
	char file_path[FULL_XML_PATH_LENGTH] ;

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skrecovery_download_dir_callback, 10);

	memset(file_path,0,FULL_XML_PATH_LENGTH);
	snprintf(file_path, FULL_XML_PATH_LENGTH, "%s/%s",gxapp_skin_online_get_url(),PATH_VER_MAIN);

	SKIN_DBG("[skin recovery] online dir url = %s \n",file_path);

	if ( s_skrecovery_node_online.node != NULL )
	{
		SKIN_DBG("[skin recovery] online dir node err ################## \n");
	}
	s_skrecovery_node_online.node = malloc(SKIN_MAX_LIST * sizeof(skin_item_node_t));
	if (s_skrecovery_node_online.node == NULL)
	{
		SKIN_DBG("[skin recovery] malloc failed!\n");
		return ;
	}
	memset(s_skrecovery_node_online.node, 0, SKIN_MAX_LIST*sizeof(skin_item_node_t));

	gxapp_skin_download_api_async_download(file_path,DIR_DOWN_PATH);
}

static void _app_skrecovery_ui_img_download(int sel)
{
	char file_path[FULL_XML_PATH_LENGTH] ;

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skrecovery_download_img_callback, 20);

	memset(file_path,0,FULL_XML_PATH_LENGTH);
	snprintf(file_path, FULL_XML_PATH_LENGTH, "%s/%s",gxapp_skin_online_get_url(),s_skrecovery_node_online.node[sel].simg_url);
	SKIN_DBG("[skin recovery] online img url = %s \n",file_path);

	gxapp_skin_download_api_async_download(file_path,IMG_DOWN_PATH);
}

//////////////////////

static void _app_skrecovery_node_init(void)
{
	memset(&s_skrecovery_node_online,0,sizeof(skin_data_node_t));
	memset(&s_skrecovery_node_usb,0,sizeof(skin_data_node_t));
}

static void _app_skrecovery_node_uninit(void)
{
	gxapp_skin_download_api_download_stop();

	gxapp_free_data(&s_skrecovery_node_online);
	gxapp_free_data(&s_skrecovery_node_usb);
}

static int _app_skrecovery_get_online_data(void)
{
	gxapp_free_data(&s_skrecovery_node_online);

	if ( gxapp_skin_online_is_disconnect() )
	{
		s_skrecovery_online_status = 0 ;
		SKIN_DBG("[skin recovery] online is disconnect \n");
		return -1 ;
	}
	else
	{
		s_skrecovery_online_status = 1 ;
	}

	_app_skrecovery_ui_dir_download();

	return 0 ;
}

static int _app_skrecovery_get_usb_data(void)
{
	int ret = 0 ;
	char *pserver = NULL ;
	extern int _common_get_usb_data(skin_data_node_t *pnode);
	ret = _common_get_usb_data(&s_skrecovery_node_usb);
	if (ret == 0 )
	{
		//999 this code for set network url. read usb xml
		pserver = gxapp_get_xml_parse_server();

		if ( pserver != NULL )
		{
			SKIN_DBG("[skin recovery] online set server = %s \n",pserver);
			gxapp_skin_online_set_url(pserver);
		}

		//
		s_skrecovery_usb_status = 1 ;
	}
	else
	{
		s_skrecovery_usb_status = 0 ;
		//printf("[sk recovery] can't get usb path, ret=%d \n",ret);
		return ret;
	}

	return -1 ;
}

static char* _app_skrecovery_get_img_name(int sel)
{
	skin_data_node_t *pnode = NULL ;

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		pnode = &s_skrecovery_node_online;
	}
	else
	{
		pnode = &s_skrecovery_node_usb;
	}

	if ( pnode->num <= 0 )
		return NULL ;

	if ( (sel < 0 ) || (sel >= pnode->num) )
		return NULL ;

	return pnode->node[sel].name ;
}

static char* _app_skrecovery_get_img_path(int sel)
{
	int len1 = 0 ;
	int len2 = 0 ;
	char *usb_entry_path = NULL ;
	char *path = NULL ;

	skin_data_node_t *pnode = NULL ;

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		pnode = &s_skrecovery_node_online;
	}
	else
	{
		pnode = &s_skrecovery_node_usb;
	}

	if ( pnode->num <= 0 )
		return " " ;

	if ( (sel < 0 ) || (sel >= pnode->num) )
		return " " ;

	path = pnode->node[sel].simg_url ;

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_USB )
	{
		//skip usb entry path : eg :/mnt/usb01/
		usb_entry_path = gxapp_skin_usb_get_url();
		if ( (usb_entry_path != NULL) && (path != NULL) )
		{
			len1 = strlen(pnode->node[sel].simg_url);
			len2 = strlen(usb_entry_path);
			if ((len2+1) < len1 )
			{
				path = path+(len2+1) ;
			}
		}
	}

	return path ;
	//return pnode->node[sel].simg_url ;
}

static void _app_skrecovery_get_list_num(void)
{
	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		s_skrecovery_list_num = s_skrecovery_node_online.num ;
	}
	else
	{
		s_skrecovery_list_num = s_skrecovery_node_usb.num ;
	}
}

static int app_skrecovery_data_init(void)
{
	s_skrecovery_update_mode = 0 ;

	_app_skrecovery_node_init();

	gxapp_skin_setting_init();

	s_skrecovery_mode    = gxapp_skin_flash_get_update_mode();
	if ( s_skrecovery_mode < SK_RECOVERY_MODE_ONLNE || s_skrecovery_mode > SK_RECOVERY_MODE_USB )
	{
		s_skrecovery_mode = SK_RECOVERY_MODE_USB ;
	}
#if SK_RECOVERY_USB_SUPPORT
#else
	if ( s_skrecovery_mode != SK_RECOVERY_MODE_ONLNE )
	{
		s_skrecovery_mode = SK_RECOVERY_MODE_ONLNE ;
	}
#endif

	s_skrecovery_img_sel = gxapp_skin_flash_get_update_imgsel() ;

	return 0 ;
}

static void app_skrecovery_data_uninit(void)
{
	_app_skrecovery_node_uninit();

	APP_FREE(s_skrecovery_simg_md5);
	APP_FREE(s_skrecovery_dimg_md5);
}

static int app_skrecovery_ui_update_mode(int update_list)
{
	char *img_url = NULL ;
	char *url = NULL;
	int sel = 0 ;

	GUI_SetProperty("text_skrecovery_img_url", "string", " ");
	GUI_SetProperty("text_skrecovery_update_img_tips", "string"," ");

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		url = gxapp_skin_online_get_url();
		sel = 0 ;
		s_skrecovery_status = s_skrecovery_online_status ;
	}
	else
	{
		url = gxapp_skin_usb_get_url();
		sel = 1 ;
		s_skrecovery_status = s_skrecovery_usb_status ;
	}

	if ( url != NULL )
	{
		GUI_SetProperty("text_skrecovery_main_url", "string", url);
	}

	_app_skrecovery_get_list_num();

	if ( s_skrecovery_list_num > 0 )
	{
		img_url = _app_skrecovery_get_img_path(0);
		GUI_SetProperty("text_skrecovery_img_url", "string", img_url);
	}

	if ( s_skrecovery_status )
		GUI_SetProperty("text_skrecovery_status", "string", "valid");
	else
		GUI_SetProperty("text_skrecovery_status", "string", "invalid");

	GUI_SetProperty("cmb_skrecovery_mode", "select", &sel);

	if ( update_list )
	{
		GUI_SetProperty("listview_skrecovery_list", "update_all", NULL);
	}

	return 0 ;
}

static int app_skrecovery_ui_update_data(void)
{
	char *img_url = NULL ;
	int sel = 0 ;

	SKIN_DBG("[skin recovery] update data \n");

	GUI_SetProperty("listview_skrecovery_list", "select", &sel);
	GUI_SetProperty("listview_skrecovery_list", "active", &sel);

	GUI_SetProperty("text_skrecovery_img_url", "string", " ");

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		_app_skrecovery_get_online_data();
		s_skrecovery_status = s_skrecovery_online_status ;
	}
	else
	{
		_app_skrecovery_get_usb_data();
		s_skrecovery_status = s_skrecovery_usb_status ;
	}

	_app_skrecovery_get_list_num();

	if ( s_skrecovery_list_num > 0 )
	{
		img_url = _app_skrecovery_get_img_path(0);
		GUI_SetProperty("text_skrecovery_img_url", "string", img_url);
	}

	if ( s_skrecovery_status )
		GUI_SetProperty("text_skrecovery_status", "string", "valid");
	else
		GUI_SetProperty("text_skrecovery_status", "string", "invalid");

	GUI_SetProperty("listview_skrecovery_list", "update_all", NULL);

	return 0 ;
}

static void _app_skrecovery_ui_left_right(void)
{
#if SK_RECOVERY_USB_SUPPORT
	int sel = 0 ;

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
		s_skrecovery_mode = SK_RECOVERY_MODE_USB ;
	else
		s_skrecovery_mode = SK_RECOVERY_MODE_ONLNE ;

	if ( s_skrecovery_list_num > 0 )
	{
		GUI_SetProperty("listview_skrecovery_list", "select", &sel);
		GUI_SetProperty("listview_skrecovery_list", "active", &sel);
	}

	app_skrecovery_ui_update_mode(1);
#else
	// do nothing
#endif
}

static void _app_skrecovery_ui_up_down(void)
{
	char *img_url = NULL;
	int sel = 0 ;
	GUI_GetProperty("listview_skrecovery_list", "select", &sel);
	GUI_SetProperty("listview_skrecovery_list", "active", &sel);

	img_url = _app_skrecovery_get_img_path(sel);
	GUI_SetProperty("text_skrecovery_img_url", "string", img_url);
}

static void _app_skrecovery_update_first_info(void)
{
	char buff[40];
	memset(buff,0,40);
	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		sprintf(buff,"Last update: Online, img num = %d",s_skrecovery_img_sel+1);
	}
	else
	{
		sprintf(buff,"Last update: USB, img num = %d",s_skrecovery_img_sel+1);
	}

	GUI_SetProperty("text_skrecovery_back_info", "string", buff);
}

static void _app_skrecovery_ui_update_img_tips(char *tips)
{
	if ( tips != NULL )
	{
		GUI_SetProperty("text_skrecovery_update_img_tips", "string", tips);
	}
	else
	{
		GUI_SetProperty("text_skrecovery_update_img_tips", "string", "normal tips");
	}
}

static void _app_skrecovery_ui_update_img_proc(char *file_name,char *smd5,char *dmd5)
{
	int ret ;

	if ( (smd5 == NULL ) || (dmd5 == NULL) )
	{
		SKIN_DBG("[skin recovery] #### check para error \n");
		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_ERR_ERR_PARA);
		return ;
	}

	ret = gxapp_skin_flash_update_partition(0,file_name,smd5,dmd5);
	if (ret != 0)
	{
		_app_skrecovery_ui_update_img_tips(gxapp_skin_flash_get_err_tips(ret));
		return ;
	}

	gxapp_skin_flash_recovery_info();

	gxapp_skin_app_reboot();
}

static void _app_skrecovery_ui_update_green(void)
{
	int sel = 0 ;

	if ( s_skrecovery_list_num <= 0 )
		return ;

	GUI_GetProperty("listview_skrecovery_list", "select", &sel);
	if ( (sel < 0 ) || (sel >= s_skrecovery_list_num) )
		return ;

	gxapp_skin_flash_set_update_mode(s_skrecovery_mode);
    gxapp_skin_flash_set_update_imgsel(sel);

	_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_START);
	GUI_SetInterface("flush",NULL);

	if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
	{
		APP_FREE(s_skrecovery_simg_md5);
		APP_FREE(s_skrecovery_dimg_md5);
		if (s_skrecovery_node_online.node[sel].simg_md5)
		{
			s_skrecovery_simg_md5 = GxCore_Strdup(s_skrecovery_node_online.node[sel].simg_md5);
		}
		if (s_skrecovery_node_online.node[sel].dimg_md5)
		{
			s_skrecovery_dimg_md5 = GxCore_Strdup(s_skrecovery_node_online.node[sel].dimg_md5);
		}
		_app_skrecovery_ui_img_download(sel);
	}
	else
	{
		_app_skrecovery_ui_update_img_proc(s_skrecovery_node_usb.node[sel].simg_url,
										   s_skrecovery_node_usb.node[sel].simg_md5,
										   s_skrecovery_node_usb.node[sel].dimg_md5);
	}
}

static void _app_skrecovery_save_key(unsigned int key)
{
	int i ;

    for(i=0;i<3;i++)
    {
        s_skrecovery_update_key[i] = s_skrecovery_update_key[i+1] ;
    }
    s_skrecovery_update_key[i] = key ;
}

/////////////////////////////////

static void _app_skrecovery_clean_buf(void)
{
#ifdef ECOS_OS
    if(s_skrecovery_cache_buf.usr_p != NULL)
    {
        GxCore_HwFree(&s_skrecovery_cache_buf);
        memset(&s_skrecovery_cache_buf, 0, sizeof(GxHwMallocObj));
    }
#endif
#ifdef LINUX_OS
	if (GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_EXIST)
	{
		GxCore_FileDelete(IMG_DOWN_PATH);
	}
#endif
}

static int _app_skrecovery_bin_size_check(char *file_name)
{
	FILE *fp = NULL;
	int len = 0;

	fp = fopen(file_name,"rb");
	if(fp == NULL )
	{
		SKIN_ERR("[skin recovery] #### file open err \n");
		return 0 ;
	}

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if( (len<=0) || (len != s_skrecovery_flash_size) )
	{
		fclose(fp);
		SKIN_ERR("[skin recovery] #### file len=%d,flash size=%d \n",len,s_skrecovery_flash_size);
		return 0 ;
	}

    fclose(fp);
	return 1 ;
}

static int _app_skrecovery_download_bin_callback(int down_flag,unsigned int down_size)
{
	int ret = 0 ;
	SKIN_DBG("[skin recovery] bin callback = %d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if (s_skrecovery_mode != SK_RECOVERY_MODE_ONLNE)
	{
		SKIN_DBG("[skin recovery] online unfocus \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_MODE);
		_app_skrecovery_clean_buf();
		return -1;
	}
	if ( down_flag <=0 )
	{
		SKIN_DBG("[skin recovery] #### down flag failer \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_TIPS_DOWNLOAD_FAILER);
		_app_skrecovery_clean_buf();

		return -2;
	}

#ifdef ECOS_OS
	if ( down_size != s_skrecovery_flash_size )
	{
		SKIN_DBG("[skin recovery] #### file not exist \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_FILE);
		_app_skrecovery_clean_buf();

		return -3;
	}
	if ( 0 == app_upgrade_crc_check_buf(s_skrecovery_cache_buf.usr_p, s_skrecovery_flash_size) )
	{
		SKIN_DBG("[skin recovery] #### ebin crc failer \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_CRC);
		_app_skrecovery_clean_buf();

		return -4;
	}
	SKIN_DBG("[skin recovery] ebin update start \n");

	s_skrecovery_update_mode = 1 ;

	ret = gxapp_skin_flash_update_all_ext(s_skrecovery_cache_buf.usr_p, s_skrecovery_flash_size);

#endif
#ifdef LINUX_OS
	if ( GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_UNEXIST )
	{
		SKIN_DBG("[skin recovery] #### file not exist \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_FILE);
		_app_skrecovery_clean_buf();

		return -3;
	}
	ret = _app_skrecovery_bin_size_check(IMG_DOWN_PATH);
	if (ret == 0 )
	{
		SKIN_DBG("[skin recovery] #### bin size not same \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_SIZE);
		_app_skrecovery_clean_buf();

		return -4;
	}
	if( 0 == app_upgrade_crc_check(IMG_DOWN_PATH))
	{
		SKIN_DBG("[skin recovery] #### lbin crc failer \n");

		_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_CRC);
		_app_skrecovery_clean_buf();

		return -5;
	}
	SKIN_DBG("[skin recovery] lbin update start \n");

	s_skrecovery_update_mode = 1 ;

	ret = gxapp_skin_flash_update_all_ext(IMG_DOWN_PATH,0);

#endif

	if ( ret < 0 )
	{
		s_skrecovery_update_mode = 0;
		_app_skrecovery_ui_update_img_tips(gxapp_skin_flash_get_err_tips(ret));

		_app_skrecovery_clean_buf();
	}

	if ( ret > 0 )
	{
		gxapp_skin_app_api_reboot();
	}

	return 0;
}

static void _app_skrecovery_ui_bin_download(void)
{
	char file_path[FULL_XML_PATH_LENGTH] ;

#ifdef ECOS_OS
    memset(&s_skrecovery_cache_buf, 0, sizeof(GxHwMallocObj));
    s_skrecovery_cache_buf.size = s_skrecovery_flash_size + 4;/*need add 4bytes*/
    GxCore_HwMalloc(&s_skrecovery_cache_buf, MALLOC_WITH_CACHE);
    if(s_skrecovery_cache_buf.usr_p == NULL)
    {
        SKIN_DBG("[skin recovery] malloc s_skrecovery_cache_buf error \n");
        return ;
    }
#endif

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skrecovery_download_bin_callback, 200);

	memset(file_path,0,FULL_XML_PATH_LENGTH);
	snprintf(file_path, FULL_XML_PATH_LENGTH, "%s/%s/%s",gxapp_skin_online_get_url(),
				PATH_VER_MAIN,SKIN_UPDATE_BIN_NAME);
	SKIN_DBG("[skin recovery] online bin url = %s \n",file_path);

#ifdef ECOS_OS
	gxapp_skin_download_api_async_download_ext(file_path,s_skrecovery_cache_buf.usr_p,
				s_skrecovery_cache_buf.size);
#endif
#ifdef LINUX_OS
	gxapp_skin_download_api_async_download(file_path,IMG_DOWN_PATH);
#endif
}

static void _app_skrecovery_update_bin(void)
{
	int ret = 0;
	char *usb_entry_path = NULL ;
	char full_path[FULL_XML_PATH_LENGTH] ;

	if ( (s_skrecovery_update_key[0] == STBK_RED) &&
	     (s_skrecovery_update_key[1] == STBK_1) &&
		 (s_skrecovery_update_key[2] == STBK_2) &&
		 (s_skrecovery_update_key[3] == STBK_YELLOW))
	{
		s_skrecovery_flash_size = app_get_flash_size();
		SKIN_DBG("[skin recovery] flash size = %d \n",s_skrecovery_flash_size);

		if ( s_skrecovery_mode == SK_RECOVERY_MODE_USB )
		{
			SKIN_DBG("[skin recovery] update bin usb \n");
			usb_entry_path = gxapp_skin_usb_get_url();
			if ( usb_entry_path != NULL )
			{
				_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_START);
				GUI_SetInterface("flush",NULL);

				memset(full_path,0,FULL_XML_PATH_LENGTH);
				snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s/%s",usb_entry_path,PATH_VER_MAIN,SKIN_UPDATE_BIN_NAME);

				ret = access(full_path, F_OK) ;
				SKIN_DBG("[skin recovery] update bin skin path=%s,ret=%d\n",full_path,ret);
				if (0 != ret)
				{
					memset(full_path,0,FULL_XML_PATH_LENGTH);
					snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",usb_entry_path,SKIN_UPDATE_BIN_NAME);

					ret = access(full_path, F_OK) ;
					SKIN_DBG("[skin recovery] update bin base path=%s,ret=%d\n",full_path,ret);
					if (0 != ret)
					{
						_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_FILE);
						return ;
					}
				}
				ret = _app_skrecovery_bin_size_check(full_path);
				if (0 == ret)
				{
					_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_SIZE);
					return ;
				}
				ret = app_upgrade_crc_check(full_path);
				if (0 == ret)
				{
					_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_ETIP_CRC);
					return ;
				}

				s_skrecovery_update_mode = 1 ;

				ret = gxapp_skin_flash_update_all(full_path);
				if ( ret < 0 )
				{
					s_skrecovery_update_mode = 0;
					_app_skrecovery_ui_update_img_tips(gxapp_skin_flash_get_err_tips(ret));
				}

				if ( ret > 0 )
				{
					gxapp_skin_app_api_reboot();
				}
			}
		}
		else
		{
			if ( gxapp_skin_online_is_disconnect() )
			{
				SKIN_DBG("[skin recovery] online need connect \n");
				return  ;
			}
			SKIN_DBG("[skin recovery] online update bin start \n");
			_app_skrecovery_ui_update_img_tips(SKIN_STR_ID_RECOVERY_START);
			_app_skrecovery_ui_bin_download();
		}
	}
	else
	{
		if ( (s_skrecovery_update_key[0] == STBK_MENU) &&
			 (s_skrecovery_update_key[1] == STBK_9) &&
			 (s_skrecovery_update_key[2] == STBK_8) &&
			 (s_skrecovery_update_key[3] == STBK_YELLOW))
		{
			SKIN_DBG("[skin recovery] only clean update flag \n");
			_app_skrecovery_ui_update_img_tips("Clean update flag,please reboot");
			gxapp_skin_flash_clear_update_error_flag();
		}
	}
}

SIGNAL_HANDLER int app_skrecovery_create(GuiWidget *widget, void *usrdata)
{
	app_skrecovery_data_init();

	//g_AppFullArb.init(&g_AppFullArb);

	GUI_SetProperty("cmb_skrecovery_mode","content",s_skrecovery_content_mode);

	app_skrecovery_ui_update_mode(0);

	_app_skrecovery_update_first_info();

	GUI_SetFocusWidget("listview_skrecovery_list");

	_app_skrecovery_get_online_data();
#if SK_RECOVERY_USB_SUPPORT
	_app_skrecovery_get_usb_data();
#endif

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_destroy(GuiWidget *widget, void *usrdata)
{
	app_skrecovery_data_uninit();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_keypress(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_list_keypress(GuiWidget *widget, void *usrdata)
{
	unsigned int key = 0 ;
	GUI_Event *event = NULL;

	if ( s_skrecovery_update_mode == 1 )
		return EVENT_TRANSFER_STOP;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
		key = find_virtualkey_ex(event->key.scancode,event->key.sym) ;
		_app_skrecovery_save_key(key);

		switch(key)
        {
            case VK_BOOK_TRIGGER:
                break;
            case STBK_OK:
                break;
            case STBK_EXIT:
            case STBK_MENU:
                break;
            case STBK_UP:
            case STBK_DOWN:
				return EVENT_TRANSFER_KEEPON;
                break;
            case STBK_LEFT:
            case STBK_RIGHT:
				{
					_app_skrecovery_ui_left_right();
				}
                break;
            case STBK_RED:
                break;
            case STBK_GREEN:
				{
					_app_skrecovery_ui_update_green();
				}
                break;
            case STBK_YELLOW:
				{
					_app_skrecovery_update_bin();
				}
                break;
            case STBK_BLUE:
				{
					app_skrecovery_ui_update_data();
				}
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_list_change(GuiWidget *widget, void *usrdata)
{
	if ( s_skrecovery_update_mode == 1 )
		return EVENT_TRANSFER_STOP;

	_app_skrecovery_ui_up_down();
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_skrecovery_list_num ;
    //return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skrecovery_list_get_data(GuiWidget *widget, void *usrdata)
{
	static char num[10] ;
	ListItemPara* item = NULL;

	if ( s_skrecovery_list_num <= 0 )
	{
		SKIN_DBG("[skin recovery] list get data is empty \n");
		return EVENT_TRANSFER_STOP;
	}
	if ( s_skrecovery_update_mode == 1 )
		return EVENT_TRANSFER_STOP;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return GXCORE_ERROR;
    if( (0 > item->sel) || (item->sel >=s_skrecovery_list_num) )
        return GXCORE_ERROR;

    //col-0: num
    item->x_offset = 2;
    item->image = NULL;
	memset(num,0,10);
	sprintf(num,"%d",(item->sel+1));
    item->string = num ;

	//col-1: channel name
    item = item->next;
    item->x_offset = 0;
    item->image = NULL;

    item->string = _app_skrecovery_get_img_name(item->sel);

    return EVENT_TRANSFER_STOP;
}

static void network_report_to_update(int connect)
{
	if(connect)
	{
		if (  0 == s_skrecovery_online_status )
		{
			if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
			{
				app_skrecovery_ui_update_data();
			}
			else
			{
				_app_skrecovery_get_online_data();
			}
		}
	}
	else
	{
		if ( 1 == s_skrecovery_online_status )
		{
			if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
			{
				app_skrecovery_ui_update_data();
			}
			else
			{
				_app_skrecovery_get_online_data();
			}
		}
	}
}

int app_skrecover_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	if ( s_skrecovery_update_mode == 1 )
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
#if NETWORK_SUPPORT
		case GXMSG_NETWORK_STATE_REPORT:
			{
				GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
				SKIN_DBG("[skin recovery] msg state = %d,%d \n",dev_state->dev_state,IF_STATE_CONNECTED);

				if(dev_state->dev_state == IF_STATE_CONNECTED)
				{
					network_report_to_update(1);
				}
				else
				{
					network_report_to_update(0);
				}
			}
			break;
#endif

		case GXMSG_USBWIFI_HOTPLUG_OUT:
		case GXMSG_USBNET_HOTPLUG_OUT:
			{
				SKIN_DBG("[skin recovery] wifi net out, msg id = %d \n",msg->msg_id);
				network_report_to_update(0);
			}
			break;
#if SK_RECOVERY_USB_SUPPORT
		case GXMSG_HOTPLUG_IN:
			{
				char* focus_win = NULL;
				HotplugPartitionList *partition_list = NULL;
				focus_win = (char*)GUI_GetFocusWindow();
				if(!focus_win)
					return EVENT_TRANSFER_STOP;
				if(0 == strcasecmp(focus_win, WND_SKRECOVERY) )
				{
					partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
					if(partition_list&&(partition_list->partition_num > 0))
					{
						SKIN_DBG("[skin recovery] usb in \n");
						if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
						{
							_app_skrecovery_get_usb_data();
						}
						else
						{
							app_skrecovery_ui_update_data();
						}
					}
				}
			}
			break;
		case GXMSG_HOTPLUG_OUT:
			{
				char* focus_win = NULL;
				focus_win = (char*)GUI_GetFocusWindow();
				if(!focus_win)
					return EVENT_TRANSFER_STOP;
				if(0 == strcasecmp(focus_win, WND_SKRECOVERY) )
				{
					SKIN_DBG("[skin recovery] usb out \n");
					if ( s_skrecovery_mode == SK_RECOVERY_MODE_ONLNE )
					{
						_app_skrecovery_get_usb_data();
					}
					else
					{
						app_skrecovery_ui_update_data();
					}
				}
			}
			break;
#endif
		default:
			break;
	}
	return EVENT_TRANSFER_STOP;
}

#endif
