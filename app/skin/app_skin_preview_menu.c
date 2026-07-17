/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"

#if SKIN_SUPPORT
#include "app_module.h"
#include "netapps/app_netapps_service.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "module/app_dir_module.h"
#include "service/gxhotplug.h"
#include "app_wnd_file_list.h"
#include <sys/stat.h>
#include <gxos/gxcore_os_core.h>
#include <gx_apps.h>

#include "app_skin_setting.h"
#include "app_skin_config.h"
#include "app_skin_xml.h"
#include "app_skin_flash.h"
#include "app_skin_download.h"


static int s_preview_mode = 1 ;
static int s_preview_sel = 0 ;
static int s_preview_num = 0 ;
static int s_preview_uiid = 0 ;
static skin_preview_node_t s_preview_node[MAX_PREVIEW_NODE_NUM] = {{NULL},{NULL},{NULL},{NULL},{NULL}};

/*--------------------widget macro---------------------------*/


//#define IMG_SKPREVIEW_LEFT                 "img_skpreview_arrow_left"
//#define IMG_SKPREVIEW_RIGHT                "img_skpreview_arrow_right"
#define IMG_SKPREVIEW_WIDGET                 "img_skpreview_item_view"

#define IMG_SKPREVIEW_KEY                    "img_skpreview_item_key"
#define PREVIEW_IMG_PATH_SUFFIX              ".jpg"


#define PREVIEW_MODE_FLASH_ACTIVE  (-1)
#define PREVIEW_MODE_FLASH_OTHER   (0)
#define PREVIEW_MODE_ONLINE        (1)

static void _skpreview_set_uuid(void)
{
	char info[40] = {0};

	snprintf(info, sizeof(info), "UIID: %d", s_preview_uiid);	
    GUI_SetProperty("text_skin_uiid", "string", info);
    //GUI_SetProperty("text_skin_uiid", "state", "show");
}

static void _skpreview_set_pageinfo(int index)
{
	char page_info[20] = {0};

	if ( s_preview_num <= 0 )
	{
		GUI_SetProperty("text_skpreview_pageinfo", "string", "0/0");
		return ;
	}

	snprintf(page_info, sizeof(page_info), "%d/%d", index+1, s_preview_num);	
    GUI_SetProperty("text_skpreview_pageinfo", "string", page_info);
    //GUI_SetProperty(WND_SKIN_PAGE_INFO, "state", "show");
}

static void _skpreview_show_image_byid(int index)
{
	gal_add_key_path(IMG_SKPREVIEW_KEY, s_preview_node[index].pre_url);
	GUI_SetProperty(IMG_SKPREVIEW_WIDGET, "img", IMG_SKPREVIEW_KEY);
	GUI_SetProperty(IMG_SKPREVIEW_WIDGET, "state", "show");
}

static void _skpreview_show_image_bypath(char *path)
{
	gal_add_key_path(IMG_SKPREVIEW_KEY, path);
	GUI_SetProperty(IMG_SKPREVIEW_WIDGET, "img", IMG_SKPREVIEW_KEY);
	GUI_SetProperty(IMG_SKPREVIEW_WIDGET, "state", "show");

	if ( path == NULL )
	{
		GUI_SetProperty(IMG_SKPREVIEW_WIDGET, "state", "hide");
		GUI_SetProperty("text_skpreview_tip_empty", "state", "show");
	}
	else
		GUI_SetProperty("text_skpreview_tip_empty", "state", "hide");
}

static void _skpreview_show_image_bycheckpath(int i)
{
	char image_path[30];
	memset(image_path,0,30);
	snprintf(image_path, sizeof(image_path), "%s%d%s", PIC_DOWN_PATH, i, PIC_PATH_SUFFIX);

	if(GXCORE_FILE_UNEXIST != GxCore_FileExists(image_path))
	{
		_skpreview_show_image_bypath(image_path);
	}
	else
	{
		_skpreview_show_image_bypath(NULL);
	}
}

//////////////////

static int _app_skpreview_download_pic_callback(int index,char *path)
{
	if (s_preview_sel == index)
	{
		if (path == NULL )
		{
			SKIN_DBG("[skin prev] pic down callback, path null \n");
			_skpreview_show_image_bypath(NULL);

			return 0 ;
		}
		else
		{
			if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
			{
				SKIN_DBG("[skin prev] pic callback, path unexist \n");
				_skpreview_show_image_bypath(NULL);
			}
			else
			{
				_skpreview_show_image_bycheckpath(index);
			}
		}	
	}

	return 0;
}

static void _app_skpreview_download_pic_start(void)
{
    int i =0;

	gxapp_skin_download_api_multi_download_stop();

	gxapp_skin_download_api_multi_download_start();

	gxapp_skin_download_api_multi_timer_start(_app_skpreview_download_pic_callback,300);

    for(i=0; i < MAX_PREVIEW_NODE_NUM; i++)
    {
		/*
		if ( s_preview_node[i].pre_url != NULL )
		{
			SKIN_DBG("[sk preview] %d, url = %s \n",i,s_preview_node[i].pre_url);
		}*/
		if ( i< (SKIN_DOWNLOAD_PIC_MAX_NUM-1) )
		{
			gxapp_skin_download_api_multi_async_download(s_preview_node[i].pre_url,i);
		}
    }
	if ( i < SKIN_DOWNLOAD_PIC_MAX_NUM )
    {
		for(;i<SKIN_DOWNLOAD_PIC_MAX_NUM;i++)
		{
			gxapp_skin_download_api_multi_async_download(NULL,i);
		}
	}
}

//////////////////

static void _preview_free_info(void)
{
	int i ;

	if ( PREVIEW_MODE_FLASH_OTHER == s_preview_mode )
	{
		gxapp_skin_flash_unmount(OTHER_MOUNT_THEME_PATH);
	}

	for(i=0;i<MAX_PREVIEW_NODE_NUM;i++)
	{
		if (s_preview_node[i].pre_url != NULL )
		{
			APP_FREE(s_preview_node[i].pre_url);
			s_preview_node[i].pre_url = NULL ;
		}
	}
	
	s_preview_mode = 1 ;
    s_preview_sel  = 0 ;
    s_preview_num  = 0 ;
	s_preview_uiid  = 0 ;
}

/*
mode = -2 usb
mode = -1 flash active parition
mode = 0  flash other parition
mode = 1  ; network
*/
int gxapp_set_preview_info(int mode,unsigned int uiid, skin_preview_node_t *pPreview)
{
	int i ;
	int ret ;
	int real_num = 0 ;
	char path[FULL_XML_PATH_LENGTH];
	
    if ( pPreview == NULL )
	{
		s_preview_num = 0 ;
		return 0;
	}

	_preview_free_info();

	for(i=0;i<MAX_PREVIEW_NODE_NUM;i++)
	{
		if (pPreview[i].pre_url != NULL )
		{
			if ( PREVIEW_MODE_ONLINE == mode )
			{
				memset(path,0,FULL_XML_PATH_LENGTH);
				snprintf(path, FULL_XML_PATH_LENGTH, "%s/%s",gxapp_skin_online_get_url(),
							pPreview[i].pre_url);
				s_preview_node[i].pre_url = GxCore_Strdup(path) ;
			}
			else
			{
				s_preview_node[i].pre_url = GxCore_Strdup(pPreview[i].pre_url) ;
				
				ret = access(s_preview_node[i].pre_url, F_OK) ;
				if (ret != 0 )
				{
					SKIN_DBG("[skin prev] file not exist, i=%d, path=%s \n",i,s_preview_node[i].pre_url);
					continue ;
				}
			}

			SKIN_DBG("[skin prev] i=%d, path=%s \n",i,s_preview_node[i].pre_url);
			real_num ++ ;
		}
	}

	SKIN_DBG("[skin prev] num = %d, uiid = %d \n",real_num,uiid);

	s_preview_uiid = uiid ;
	s_preview_num = real_num ;
	s_preview_mode = mode ;
	
	GUI_CreateDialog(WND_SKPREVIEW);
	
	return 0 ;
}

static void _preview_left_right(void)
{
	if ( s_preview_mode > 0 )
	{
		if ( s_preview_node[s_preview_sel].pre_url != NULL )
		{
			if ( gxapp_skin_download_api_multi_is_download_finish(s_preview_sel) )
			{
				_skpreview_show_image_bycheckpath(s_preview_sel);
			}
			else
			{
				SKIN_DBG("[skin prev] pic is not download \n");
				_skpreview_show_image_bypath(NULL);
			}
		}
		else
		{
			_skpreview_show_image_bypath(NULL);
		}
	}
	else
	{
		_skpreview_show_image_byid(s_preview_sel);
	}
	_skpreview_set_pageinfo(s_preview_sel);
}

SIGNAL_HANDLER int app_skpreview_create(GuiWidget *widget, void *usrdata)
{
	if ( s_preview_num >= 2 )
	{
		GUI_SetProperty("img_skpreview_arrow_left", "state", "show");
		GUI_SetProperty("img_skpreview_arrow_right", "state", "show");

		GUI_SetProperty("img_skprview_tip_left", "state", "show");
		GUI_SetProperty("img_skpreview_tip_right", "state", "show");
		GUI_SetProperty("text_skpreview_tip_switch", "state", "show");
	}
	else if ( s_preview_num <= 0 )
	{
		GUI_SetProperty("text_skpreview_tip_empty", "state", "show");
	}
	
	if (s_preview_num > 0 )
	{
		if ( s_preview_mode > 0 )
		{
			_app_skpreview_download_pic_start();
		}
		else
		{
			_skpreview_show_image_byid(0);
		}
	}

	_skpreview_set_pageinfo(0);

	_skpreview_set_uuid();

    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_skpreview_destroy(GuiWidget *widget, void *usrdata)
{
	if ( s_preview_mode > 0 )
	{
		gxapp_skin_download_api_multi_download_stop();

		gxapps_curl_set_timeout(0, 0);
	}
	_preview_free_info();
	gal_add_key_path(IMG_SKPREVIEW_KEY, NULL);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skpreview_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                GUI_EndDialog(WND_SKPREVIEW);
                break;
            case STBK_LEFT:
				if ( s_preview_num >= 2)
				{
					if ( s_preview_sel <=0 )
					{
						s_preview_sel = s_preview_num - 1 ;
					}
					else
					{
						s_preview_sel -- ;
					}
					_preview_left_right();
				}
                break;
            case STBK_RIGHT:
				if ( s_preview_num >= 2)
				{
					s_preview_sel++ ;
					if ( s_preview_sel >= s_preview_num )
					{
						s_preview_sel = 0 ;
					}
					_preview_left_right();
				}
                break;
            case STBK_INFO:
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}


#endif
