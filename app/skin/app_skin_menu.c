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
#include <common/gxpm.h>
#include <gx_apps.h>
#include <dirent.h>
#include "app_wnd_system_setting_opt.h"

#include "app_skin_setting.h"
#include "app_skin_config.h"

#include "app_skin_xml.h"
#include "app_skin_flash.h"
#include "app_skin_download.h"
#include "app_skin_str_id.h"


/*--------------------widget macro---------------------------*/

#define MAX_PAGE_ITEM                  (6)
#define MAX_PAGE_LINE                  (2)

#define SKIN_WIDGET_NAME_LEN              (30)

#define ITEM_FOCUS_COLOR               "[net_video_focus_color,net_video_focus_color,net_video_focus_color]"

#define WND_LIST_GROUP                 "list_skin_group"
#define BTN_RIGHT                      "btn_skin_right"
#define WND_SKIN_PAGE_INFO             "text_skin_page_info"

#define TXT_RED           "text_skin_red"
#define IMG_RED           "img_skin_red"
#define TXT_GREEN         "text_skin_green"
#define IMG_GREEN         "img_skin_green"
#define TXT_YELLOW        "text_skin_yellow"
#define IMG_YELLOW        "img_skin_yellow"
#define TXT_BLUE          "text_skin_blue"
#define IMG_BLUE          "img_skin_blue"
#define TXT_PREVIEW       "text_skin_info"
#define IMG_PREVIEW       "img_skin_info"

#define DEFAULT_ICON_PATH    WORK_PATH"theme/image/netapps/default.png"

typedef enum {
    SKIN_FOCUSAREA_LIST_GROUP = 0,
    SKIN_FOCUSAREA_TABLE_ITEM,
    SKIN_FOCUSAREA_TOTAL
} SkinFocusArea;

enum SKIN_MSG_CONTROL_STATUS{
    SKIN_MSG_FOCUS_ITEM,
    SKIN_MSG_UNFOCUS_ITEM,
    SKIN_MSG_UNFOCUS_ALL_ITEM,
    SKIN_MSG_CLEAN_ALL_ITEM,
    SKIN_MSG_UPDATE_ITEM,
    SKIN_MSG_UPDATE_ITEM_IMAGE,
    SKIN_MSG_UPDATE_PAGE_INFO,
    SKIN_MSG_GROUP_LIST_FOCUS,
    SKIN_MSG_GROUP_LIST_UPDATE,
    SKIN_MSG_GROUP_LIST_ACTIVE,
    SKIN_MSG_SHOW_GIF,
    SKIN_MSG_HIDE_GIF,
};

typedef enum gx_skin_category_e{
	SKIN_CAGEGORY_INSTALLED = 0,
	SKIN_CAGEGORY_ONLINE ,
#if SKUSB_SUPPORT
	SKIN_CAGEGORY_USB,
#endif
	SKIN_CATEGORY_MAX
}gx_skin_category_t;

struct SkinUICtrl{
    int type;
    int group_sel;
    int item;
    int active;
    int total;
	char *image_path ;
    skin_item_node_t node ;
};

typedef struct {
	unsigned char  focus_area      ;
	unsigned char left_group_nums ;
	unsigned char left_group_sel  ;
	unsigned char left_group_active  ;
	unsigned short right_table_nums ;
	unsigned short right_table_sel ;
	char *simg_url ;
	char *simg_md5 ;
	char *dimg_md5   ;
	skin_data_node_t   Inode[SKIN_CATEGORY_MAX];
}skin_app_menu_info_t;

static skin_app_menu_info_t s_skin_menu_info ;

static void _skin_ui_contral(struct SkinUICtrl *msg);
static void _table_exit_keypress(void);
static int  _table_widget_draw(void);
static void _table_icon_draw(void);
static void _skin_ui_dir_download(void);
static void _skin_ui_xml_download(void);
static void _skin_ui_update_skin_data(int type);

static char* skin_category_str[SKIN_CATEGORY_MAX]=
{
	SKIN_STR_ID_INSTALLED,
	SKIN_STR_ID_ONLINE,
#if SKUSB_SUPPORT
	SKIN_STR_ID_USB
#endif
};

static char *s_skin_install_id[ALL_SKIN_PARTITION_MAX_NUM];

static void _skin_ui_contral(struct SkinUICtrl *msg)
{
	char ay_bg[SKIN_WIDGET_NAME_LEN] = {0};
	char ay_img[SKIN_WIDGET_NAME_LEN] = {0};
	char ay_title[SKIN_WIDGET_NAME_LEN] = {0};

    int page, max_page;
	int value ;
    if(msg == NULL)
        return;

	snprintf(ay_bg, SKIN_WIDGET_NAME_LEN, "text_skin_item_bg%d",((msg->item)%MAX_PAGE_ITEM));
	snprintf(ay_img, SKIN_WIDGET_NAME_LEN, "img_skin_item_preview%d",((msg->item)%MAX_PAGE_ITEM));
	snprintf(ay_title, SKIN_WIDGET_NAME_LEN, "text_skin_item_title%d",((msg->item)%MAX_PAGE_ITEM));

    switch(msg->type)
    {
        case SKIN_MSG_FOCUS_ITEM:
            {
                GUI_SetProperty(ay_bg, "state", "show");
				GUI_SetProperty(ay_bg, "backcolor", ITEM_FOCUS_COLOR);
				GUI_SetProperty(ay_title, "forecolor", "[#E78818,#E78818,#E78818]");
                GUI_SetProperty(ay_img, "update", NULL);
                break;
            }
        case SKIN_MSG_UNFOCUS_ITEM:
            {
                GUI_SetProperty(ay_title, "forecolor", "[text_color,text_color,text_color]");
				GUI_SetProperty(ay_bg, "state", "hide");
                GUI_SetProperty(ay_img, "update", NULL);
                break;
            }
        case SKIN_MSG_UNFOCUS_ALL_ITEM:
            {
                int32_t i;

                for(i = 0; i < MAX_PAGE_ITEM; i++)
                {
					memset(ay_bg,0,SKIN_WIDGET_NAME_LEN);
					memset(ay_img,0,SKIN_WIDGET_NAME_LEN);
					memset(ay_title,0,SKIN_WIDGET_NAME_LEN);

					snprintf(ay_bg, SKIN_WIDGET_NAME_LEN, "text_skin_item_bg%d",i);
					snprintf(ay_img, SKIN_WIDGET_NAME_LEN, "img_skin_item_preview%d",i);
					snprintf(ay_title, SKIN_WIDGET_NAME_LEN, "text_skin_item_title%d",i);

                    GUI_SetProperty(ay_title, "forecolor", "[text_color,text_color,text_color]");
                    GUI_SetProperty(ay_bg, "state", "hide");
                    GUI_SetProperty(ay_img, "update", NULL);
                }
                break;
            }
        case SKIN_MSG_CLEAN_ALL_ITEM:
            {
                int32_t i;
                for(i = 0;i < MAX_PAGE_ITEM; i++)
                {
					memset(ay_img,0,SKIN_WIDGET_NAME_LEN);
					memset(ay_title,0,SKIN_WIDGET_NAME_LEN);

					snprintf(ay_img, SKIN_WIDGET_NAME_LEN, "img_skin_item_preview%d",i);
					snprintf(ay_title, SKIN_WIDGET_NAME_LEN, "text_skin_item_title%d",i);

                    GUI_SetProperty(ay_img, "state", "hide");
                    GUI_SetProperty(ay_title, "state", "hide");
                }
                break;
            }
        case SKIN_MSG_UPDATE_ITEM:
            {
                if(msg->node.name != NULL)
                {
                    GUI_SetProperty(ay_title, "string", msg->node.name);
					GUI_SetProperty(ay_title, "state", "show");
				}
				else
				{
					GUI_SetProperty(ay_title, "state", "hide");
					GUI_SetProperty(ay_img, "state", "hide");
				}

                break;
            }
        case SKIN_MSG_UPDATE_ITEM_IMAGE:
            {
                if(msg->image_path != NULL)
                {
					memset(ay_bg,0,SKIN_WIDGET_NAME_LEN);
					snprintf(ay_bg, SKIN_WIDGET_NAME_LEN, "img_skin_item_key%d",((msg->item)%MAX_PAGE_ITEM));

					gal_add_key_path(ay_bg, msg->image_path);
					GUI_SetProperty(ay_img, "img", (void*)ay_bg);
					GUI_SetProperty(ay_img, "state", "show");

                }
                break;
            }
        case SKIN_MSG_GROUP_LIST_FOCUS:
            {
                GUI_SetFocusWidget(WND_LIST_GROUP);
				value = s_skin_menu_info.left_group_sel ;
                GUI_SetProperty(WND_LIST_GROUP, "select", &value);
                break;
            }
        case SKIN_MSG_GROUP_LIST_ACTIVE:
            {
				value = s_skin_menu_info.left_group_sel ;
                GUI_SetProperty(WND_LIST_GROUP, "active", &value);

                break;
            }
        case SKIN_MSG_UPDATE_PAGE_INFO:
            {
                page = 1 + (msg->item - (msg->item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                max_page = 1 + ((msg->total-1) - ((msg->total-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
				memset(ay_bg,0,SKIN_WIDGET_NAME_LEN);
                snprintf(ay_bg, sizeof(ay_bg), "%d/%d", page, max_page);
                GUI_SetProperty(WND_SKIN_PAGE_INFO, "string", ay_bg);
                GUI_SetProperty(WND_SKIN_PAGE_INFO, "state", "show");
                break;
            }
        case SKIN_MSG_GROUP_LIST_UPDATE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
                break;
            }

        default:
            break;
    }
}

static void _skin_focus_item(int index)
{
    struct SkinUICtrl param = {0};

    if(index >= 0)
    {
        param.type = SKIN_MSG_FOCUS_ITEM;
        param.item = index;
       _skin_ui_contral(&param);
    }
}
/*
static void _skin_unfocus_all_item(void)
{
    struct SkinUICtrl param = {0};

    param.type = SKIN_MSG_UNFOCUS_ALL_ITEM;
    _skin_ui_contral(&param);

    return;
}
*/

static void _skin_unfocus_item(int index)
{
    struct SkinUICtrl param = {0};

    if(index >= 0)
    {
        param.type = SKIN_MSG_UNFOCUS_ITEM;
        param.item = index;
        _skin_ui_contral(&param);
    }
}

static void _skin_group_list_focus(void)
{
    struct SkinUICtrl param = {0};

    param.type = SKIN_MSG_GROUP_LIST_FOCUS;
    _skin_ui_contral(&param);

    return;
}
static void _skin_group_list_active(void)
{
    struct SkinUICtrl param = {0};

    param.type = SKIN_MSG_GROUP_LIST_ACTIVE;
    _skin_ui_contral(&param);

    return;
}

static void _skin_clean_all_item(void)
{
    struct SkinUICtrl param = {0};

    param.type = SKIN_MSG_CLEAN_ALL_ITEM;
    _skin_ui_contral(&param);

    return;
}

static void _skin_update_page_info(void)
{
    struct SkinUICtrl param = {0};

    param.type = SKIN_MSG_UPDATE_PAGE_INFO;
    param.item = s_skin_menu_info.right_table_sel;
    param.total = s_skin_menu_info.right_table_nums;
    _skin_ui_contral(&param);
}

static void _skin_right_draw_button(void)
{
	int active_id = 0 ;
	int status = 0 ;
	int left = 0 ;
	int right = 0 ;

	left  = s_skin_menu_info.left_group_active ;
	right = s_skin_menu_info.right_table_sel ;

    if( left == SKIN_CAGEGORY_INSTALLED )
    {
		if ( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
		{
			active_id = gxapp_skin_flash_get_active_partition_id();
			status    = gxapp_skin_flash_get_status(right);

			if ( (right == active_id) || (0 == status) )
			{
				GUI_SetProperty(TXT_GREEN, "state", "hide");
				GUI_SetProperty(IMG_GREEN, "state", "hide");

				GUI_SetProperty(TXT_YELLOW, "state", "hide");
				GUI_SetProperty(IMG_YELLOW, "state", "hide");
			}
			else
			{
				GUI_SetProperty(TXT_GREEN, "state", "show");
				GUI_SetProperty(IMG_GREEN, "state", "show");
				GUI_SetProperty(TXT_GREEN, "string", "UnInstall");

				GUI_SetProperty(TXT_YELLOW, "state", "show");
				GUI_SetProperty(IMG_YELLOW, "state", "show");
				GUI_SetProperty(TXT_YELLOW, "string", "Activation");
			}
		}
		else
		{
			GUI_SetProperty(TXT_GREEN, "state", "hide");
			GUI_SetProperty(IMG_GREEN, "state", "hide");

			GUI_SetProperty(TXT_YELLOW, "state", "hide");
			GUI_SetProperty(IMG_YELLOW, "state", "hide");
		}
    }
	else
	{
		if ( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
		{
			if (s_skin_menu_info.Inode[left].node[right].uninstall)
			{
				GUI_SetProperty(TXT_GREEN, "state", "hide");
				GUI_SetProperty(IMG_GREEN, "state", "hide");
			}
			else
			{
				GUI_SetProperty(TXT_GREEN, "state", "show");
				GUI_SetProperty(IMG_GREEN, "state", "show");
				GUI_SetProperty(TXT_GREEN, "string", "Install");
			}
		}
	}
}

static void _skin_left_focus_draw_button(void)
{
	GUI_SetProperty(TXT_RED, "state", "show");
    GUI_SetProperty(IMG_RED, "state", "show");

	GUI_SetProperty(TXT_PREVIEW, "state", "hide");
    GUI_SetProperty(IMG_PREVIEW, "state", "hide");

	GUI_SetProperty(TXT_GREEN, "state", "hide");
    GUI_SetProperty(IMG_GREEN, "state", "hide");

	GUI_SetProperty(TXT_YELLOW, "state", "hide");
    GUI_SetProperty(IMG_YELLOW, "state", "hide");

    if( s_skin_menu_info.left_group_active == SKIN_CAGEGORY_INSTALLED )
    {
		GUI_SetProperty(TXT_BLUE, "state", "hide");
        GUI_SetProperty(IMG_BLUE, "state", "hide");
    }
	else
	{
		GUI_SetProperty(TXT_BLUE, "state", "show");
        GUI_SetProperty(IMG_BLUE, "state", "show");
	}
}

static void _skin_right_focus_draw_button(void)
{
	int active_id = 0 ;
	int status = 0 ;
	int left = 0 ;
	int right = 0 ;

	GUI_SetProperty(TXT_RED, "state", "hide");
    GUI_SetProperty(IMG_RED, "state", "hide");

	GUI_SetProperty(TXT_PREVIEW, "state", "show");
    GUI_SetProperty(IMG_PREVIEW, "state", "show");

	left      = s_skin_menu_info.left_group_active ;
	right     = s_skin_menu_info.right_table_sel ;

    if( left == SKIN_CAGEGORY_INSTALLED )
    {
		active_id = gxapp_skin_flash_get_active_partition_id();
		status    = gxapp_skin_flash_get_status(right);

		if ( (right == active_id) || (0 == status) )
		{
			GUI_SetProperty(TXT_GREEN, "state", "hide");
			GUI_SetProperty(IMG_GREEN, "state", "hide");

			GUI_SetProperty(TXT_YELLOW, "state", "hide");
			GUI_SetProperty(IMG_YELLOW, "state", "hide");
		}
		else
		{
			GUI_SetProperty(TXT_GREEN, "state", "show");
			GUI_SetProperty(IMG_GREEN, "state", "show");
			GUI_SetProperty(TXT_GREEN, "string", "UnInstall");

			GUI_SetProperty(TXT_YELLOW, "state", "show");
			GUI_SetProperty(IMG_YELLOW, "state", "show");
			GUI_SetProperty(TXT_YELLOW, "string", "Activation");
		}

		GUI_SetProperty(TXT_BLUE, "state", "hide");
        GUI_SetProperty(IMG_BLUE, "state", "hide");
    }
	else
	{
		GUI_SetProperty(TXT_YELLOW, "state", "hide");
        GUI_SetProperty(IMG_YELLOW, "state", "hide");

		if (s_skin_menu_info.Inode[left].node[right].uninstall)
		{
			GUI_SetProperty(TXT_GREEN, "state", "hide");
			GUI_SetProperty(IMG_GREEN, "state", "hide");
		}
		else
		{
			GUI_SetProperty(TXT_GREEN, "state", "show");
			GUI_SetProperty(IMG_GREEN, "state", "show");
			GUI_SetProperty(TXT_GREEN, "string", "Install");
		}

		GUI_SetProperty(TXT_BLUE, "state", "show");
        GUI_SetProperty(IMG_BLUE, "state", "show");
	}
}

static int _ui_get_flash_data(void)
{
	int ret ;
	//int i ;

	gxapp_free_data(&s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED]);

	ret = gxapp_skin_flash_get_data(&s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED]);

	/*
	SKIN_DBG("[skin menu] get flash data, num = %d \n",s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].num);

	for(i=0;i<s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].num;i++)
	{
		SKIN_DBG("[skin menu] name=%s \n",s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].node[i].name);
	}
	*/
	return ret ;
}

int _common_get_usb_data(skin_data_node_t *pnode)
{
	int n = 0 ;
	int i,j,k ;
	int ret = 0;
	struct stat st;
	DIR *dir = NULL;
    struct dirent *ddir = NULL;
	char *ay_dir[SKIN_MAX_LIST];
	char *sort_1,*sort_2 ;
	char sort_src,sort_dst ;
	char *usb_entry_path = NULL ;

	char full_path[FULL_XML_PATH_LENGTH];

	if ( pnode == NULL )
		return -1 ;

	gxapp_free_data(pnode);
	gxapp_xml_module_uninit();

	ret = gxapp_skin_get_usb_entry_path();
	if ( ret != 0 )
	{
		SKIN_DBG("[skin comm] #### usb path empty, ret=%d \n",ret);
		return ret;
	}

	usb_entry_path = gxapp_skin_usb_get_url();
	if ( usb_entry_path == 0 )
	{
		SKIN_DBG("[skin comm] #### usb path null \n");
		return -1;
	}

	SKIN_DBG("[skin comm] usb entry path = %s \n",usb_entry_path);

	memset(full_path,0,FULL_XML_PATH_LENGTH);
	snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",usb_entry_path,PATH_VER_MAIN);

	dir = opendir(full_path);
	if (!dir)
	{
		SKIN_DBG("[skin comm] opendir %s failed!\n", full_path);
		return -1;
	}
	SKIN_DBG("[skin comm] opendir = %s \n", full_path);

	for(i=0;i<SKIN_MAX_LIST;i++)
	{
		ay_dir[i] = NULL ;
	}

	n = 0 ;
	while ((ddir = readdir(dir)))
	{
		if (strcmp (ddir->d_name, ".") == 0 || strcmp (ddir->d_name, "..") == 0)
			continue;

		memset(full_path,0,FULL_XML_PATH_LENGTH);
		snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s/%s",usb_entry_path,PATH_VER_MAIN,ddir->d_name);

		if (stat((char *)full_path, &st) != 0)
		{
			SKIN_DBG("[skin comm] dir path=%s stat err \n",full_path);
			continue ;
		}
		if (S_ISDIR(st.st_mode))
		{
			//SKIN_DBG("[skin comm] is dir \n");
		}
		else
		{
			SKIN_DBG("[skin comm] path=%s not dir \n",full_path);
			continue ;
		}

		memset(full_path,0,FULL_XML_PATH_LENGTH);
		snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s/%s/%s",usb_entry_path,PATH_VER_MAIN,ddir->d_name,SKIN_XML_NAME);

		ret = access(full_path, F_OK) ;
		SKIN_DBG("[skin comm] xml path=%s,ret=%d,n=%d \n",full_path,ret,n);
		if (0 == ret)
		{
			ay_dir[n] = GxCore_Strdup(ddir->d_name);
			n++ ;
		}
		if (n>=SKIN_MAX_LIST)
			break;
	}
	closedir(dir);

	SKIN_DBG("[skin comm] dir num = %d \n",n);
#if 1
	for (k = 0; k < n - 1; k++)
	{
		for (j = 0; j < n - k - 1; j++)
		{
			sort_1 = ay_dir[j];
			sort_2 = ay_dir[j+1];

			if ( (sort_1 == NULL) || (sort_2 == NULL) )
				continue ;

			sort_src = sort_1[0] ;
			if ( sort_src >= 0x41 && sort_src <= 0x5A ) //A-Z
				sort_src = sort_src + 32 ;

			sort_dst = sort_2[0] ;
			if ( sort_dst >= 0x41 && sort_dst <= 0x5A ) //A-Z
				sort_dst = sort_dst + 32 ;

			//SKIN_DBG("[skin comm] sort = 0x%x,0x%x \n", sort_src,sort_dst);

			if (sort_src > sort_dst)
			{
				sort_1 = ay_dir[j];
				ay_dir[j] = ay_dir[j + 1];
				ay_dir[j + 1] = sort_1;
			}
		}
	}
#endif

	pnode->node = malloc(n * sizeof(skin_item_node_t));
	if (pnode->node == NULL)
	{
		SKIN_DBG("[skin comm] malloc failed!\n");
		for(i=0;i<SKIN_MAX_LIST;i++)
		{
			APP_FREE(ay_dir[i]) ;
		}
		return -2;
	}
	memset(pnode->node, 0, n*sizeof(skin_item_node_t));

	k=0 ;
	for(j=0;j<n;j++)
	{
		if ( ay_dir[j] == NULL )
		{
			SKIN_DBG("[skin comm] usb path err, j=%d \n",j);
			continue ;
		}

		memset(full_path,0,FULL_XML_PATH_LENGTH);
		snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",PATH_VER_MAIN,ay_dir[j]);
		pnode->node[k].ddir =  GxCore_Strdup(full_path);

		memset(full_path,0,FULL_XML_PATH_LENGTH);
		snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s/%s/%s",usb_entry_path,PATH_VER_MAIN,ay_dir[j],SKIN_XML_NAME);

		ret = gxapp_file_get_data(full_path,&pnode->node[k]);

		SKIN_DBG("[skin comm] usb xml path=%s,ret=%d,k=%d,j=%d \n",full_path,ret,k,j);

		if (ret == 0 )
		{
			k++ ;
		}
		else
		{
			APP_FREE(pnode->node[k].ddir);
		}
	}

	SKIN_DBG("[skin comm] usb, get total num = %d \n",k);

	pnode->num = k ;

	for(i=0;i<pnode->num ;i++)
	{
		if ( pnode->node[i].icon_url != NULL )
		{
			memset(full_path,0,FULL_XML_PATH_LENGTH);
			snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",usb_entry_path,pnode->node[i].icon_url);

			APP_FREE(pnode->node[i].icon_url);
			if ( 0 == access(full_path, F_OK) )
			{
				pnode->node[i].icon_url = GxCore_Strdup(full_path);
			}
		}

		if ( pnode->node[i].simg_url != NULL )
		{
			memset(full_path,0,FULL_XML_PATH_LENGTH);
			snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",usb_entry_path,pnode->node[i].simg_url);

			APP_FREE(pnode->node[i].simg_url);
			pnode->node[i].simg_url = GxCore_Strdup(full_path);
		}

		if ( gxapp_exist_installed_uiid(pnode->node[i].uiid) )
		{
			pnode->node[i].uninstall = 1 ;
		}
		else
		{
			pnode->node[i].uninstall = 0 ;
		}

		for(j=0;j<MAX_PREVIEW_NODE_NUM;j++)
		{
			if ( pnode->node[i].preview[j].pre_url != NULL )
			{
				memset(full_path,0,FULL_XML_PATH_LENGTH);
				snprintf(full_path, FULL_XML_PATH_LENGTH, "%s/%s",usb_entry_path,
							pnode->node[i].preview[j].pre_url);

				APP_FREE(pnode->node[i].preview[j].pre_url);
				pnode->node[i].preview[j].pre_url = GxCore_Strdup(full_path);
			}
		}
	}

	for(i=0;i<SKIN_MAX_LIST;i++)
	{
		APP_FREE(ay_dir[i]) ;
	}

	return 0 ;
}

#if SKUSB_SUPPORT
static int _ui_get_usb_data(void)
{
	int ret = 0 ;
	ret = _common_get_usb_data(&s_skin_menu_info.Inode[SKIN_CAGEGORY_USB]);
	return ret ;
}
#endif

static void _ui_get_online_data(void)
{
	gxapp_free_data(&s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE]);

	if ( gxapp_skin_online_is_disconnect() )
	{
		SKIN_DBG("[skin menu] #### network disconnect \n");
		return ;
	}

	_skin_ui_dir_download();
}

static void _table_up_keypress(void)
{
    int i, focus_item;
    focus_item = i = s_skin_menu_info.right_table_sel ;

    if(((i%MAX_PAGE_ITEM)-MAX_PAGE_ITEM/MAX_PAGE_LINE) < 0)
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i < 0)
            return;
        s_skin_menu_info.right_table_sel = i;
		_table_widget_draw();
		_table_icon_draw();
    }
    else
    {
        i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i < 0)
            return;
        s_skin_menu_info.right_table_sel = i;

    }
	//SKIN_DBG("up i=%d,focus_item=%d\n",i,focus_item);
    SKIN_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    _skin_unfocus_item(focus_item);
    _skin_focus_item(i);

	_skin_right_draw_button();
}

static void _table_down_keypress(void)
{
    int i, total_number, focus_item;

    total_number = s_skin_menu_info.right_table_nums;
    focus_item = i = s_skin_menu_info.right_table_sel;

    if(((i%MAX_PAGE_ITEM)+MAX_PAGE_ITEM/MAX_PAGE_LINE) < MAX_PAGE_ITEM)
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(i > total_number-1)
            return;
        s_skin_menu_info.right_table_sel = i;
    }
    else
    {
        i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
        if(((i/MAX_PAGE_ITEM)*MAX_PAGE_ITEM)>=total_number)
            return;
        if(i > total_number-1)
            i = total_number-1;
        s_skin_menu_info.right_table_sel = i;
		_table_widget_draw();
		_table_icon_draw();
    }

    SKIN_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    _skin_unfocus_item(focus_item);
    _skin_focus_item(i);

	_skin_right_draw_button();

}

static void _table_left_keypress(void)
{
    int i = 0;
    int focus_item = 0;

    focus_item = i = s_skin_menu_info.right_table_sel;

    if(0 == i%(MAX_PAGE_ITEM/MAX_PAGE_LINE))
    {
        _table_exit_keypress();
        return;
    }
    else
    {
        i -= 1;
        s_skin_menu_info.right_table_sel = i;
    }
    SKIN_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    _skin_unfocus_item(focus_item);
    _skin_focus_item(i);

	_skin_right_draw_button();
}

static void _table_right_keypress(void)
{
    int i = 0;
    int max_page, page, total_number, focus_item;

    total_number = s_skin_menu_info.right_table_nums;
    focus_item = i = s_skin_menu_info.right_table_sel;

    if(0 == (i+1)%MAX_PAGE_ITEM || i == (total_number - 1))
    {
        if(total_number <= MAX_PAGE_ITEM)
        {
            return;
        }
        else
        {
            max_page = ((total_number-1) - ((total_number-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
            page = (i - i % MAX_PAGE_ITEM) / MAX_PAGE_ITEM;
            if((page + 1) > max_page)
                page = 0;
            else
                page += 1;

            i = page * MAX_PAGE_ITEM;
            s_skin_menu_info.right_table_sel = i;
            _skin_unfocus_item(focus_item);
			_table_widget_draw();
			_table_icon_draw();
        }
    }
    else
    {
        i += 1;
        if(i > total_number-1)
            return;
        s_skin_menu_info.right_table_sel = i;
        _skin_unfocus_item(focus_item);
    }
    SKIN_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    _skin_focus_item(i);

	_skin_right_draw_button();
}

static void _skin_ui_disable_active_id_img(void)
{
	GUI_SetProperty("img_skin_active_id0", "state", "hide");
	GUI_SetProperty("img_skin_active_id1", "state", "hide");
	GUI_SetProperty("img_skin_active_id2", "state", "hide");
	GUI_SetProperty("img_skin_active_id3", "state", "hide");
	GUI_SetProperty("img_skin_active_id4", "state", "hide");
	GUI_SetProperty("img_skin_active_id5", "state", "hide");
}

static void _skin_ui_enable_active_id_img(void)
{
	int start ;
	int active_id = 0 ;
	char win_avtive_id[SKIN_WIDGET_NAME_LEN];

	if ( s_skin_menu_info.left_group_active == SKIN_CAGEGORY_INSTALLED )
	{
		active_id = gxapp_skin_flash_get_active_partition_id() ;

		start = (s_skin_menu_info.right_table_sel - s_skin_menu_info.right_table_sel % MAX_PAGE_ITEM);

		if ( (active_id >= start) && (active_id < (start+MAX_PAGE_ITEM)) )
		{
			memset(win_avtive_id,0,SKIN_WIDGET_NAME_LEN);
			snprintf(win_avtive_id, SKIN_WIDGET_NAME_LEN, "img_skin_active_id%d",(active_id%MAX_PAGE_ITEM));

			GUI_SetProperty(win_avtive_id, "state", "show");

			//SKIN_DBG("[skin menu] set active icon, start = %d, active id=%d \n",start,active_id);
		}
	}
}

static int _table_widget_draw(void)
{
    int i = 0;
    struct SkinUICtrl param = {0};
    int sel = 0;
    sel = (s_skin_menu_info.right_table_sel - s_skin_menu_info.right_table_sel % MAX_PAGE_ITEM);
    for(i=0; i < MAX_PAGE_ITEM; i++)
    {
        memset(&param, 0, sizeof(struct SkinUICtrl));
        param.type = SKIN_MSG_UPDATE_ITEM;
        param.item = sel;
        if(sel >= s_skin_menu_info.right_table_nums)
        {
            param.node.name = NULL;
        }
        else
        {
            param.node.name = s_skin_menu_info.Inode[s_skin_menu_info.left_group_active].node[sel].name;
        }
        _skin_ui_contral(&param);
        sel++;
    }

    _skin_update_page_info();

	_skin_ui_disable_active_id_img();
	_skin_ui_enable_active_id_img();

    return 0;
}

static void _right_table_update(void)
{
	s_skin_menu_info.right_table_nums = s_skin_menu_info.Inode[s_skin_menu_info.left_group_active].num ;
	s_skin_menu_info.right_table_sel  = 0 ;

    if(s_skin_menu_info.right_table_nums >0)
    {
        _table_widget_draw();//
    }
}

static void _table_exit_keypress(void)
{
    _skin_unfocus_item(s_skin_menu_info.right_table_sel);
    s_skin_menu_info.focus_area = SKIN_FOCUSAREA_LIST_GROUP;
    _skin_group_list_focus();

	_skin_left_focus_draw_button();
}

static void _skin_info_global_init(void)
{
    s_skin_menu_info.focus_area    = SKIN_FOCUSAREA_LIST_GROUP;

	if( check_usb_status() == false )
	{
		s_skin_menu_info.left_group_nums = SKIN_CATEGORY_MAX -1 ;
	}
	else
	{
		s_skin_menu_info.left_group_nums = SKIN_CATEGORY_MAX ;
	}

	s_skin_menu_info.left_group_sel  = SKIN_CAGEGORY_INSTALLED ;
	s_skin_menu_info.left_group_active = SKIN_CAGEGORY_INSTALLED ;

	s_skin_menu_info.right_table_nums = 0 ;
	s_skin_menu_info.right_table_sel  = 0 ;

	memset(&s_skin_menu_info.Inode,0,SKIN_CATEGORY_MAX*sizeof(skin_data_node_t));
}

static void _skin_info_global_uninit(void)
{
	int i ;
	for(i=0;i<SKIN_CATEGORY_MAX;i++)
	{
		gxapp_free_data(&s_skin_menu_info.Inode[i]);
	}

	gxapp_xml_module_uninit();

	APP_FREE(s_skin_menu_info.simg_url);
	APP_FREE(s_skin_menu_info.simg_md5);
	APP_FREE(s_skin_menu_info.dimg_md5);
}

static void app_skin_close_menu(void)
{
	GUI_EndDialog(WND_SKIN);
}

//////////////////////

static void _app_skin_online_callback_get_data(void)
{
	int cur_add = 0 ;
	int ret = 0 ;
	int down_index = 0 ;
	char xml_path[FULL_XML_PATH_LENGTH] ;

	cur_add = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num ;
	if (s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].cur_down_num <= 0 )
	{
		SKIN_DBG("[skin menu] callback get data down index err, nums=%d \n",cur_add);
		return ;
	}

	down_index = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].cur_down_num -1 ;

	if ( (cur_add < SKIN_MAX_LIST) && (down_index <SKIN_MAX_LIST) && (cur_add <= down_index) )
	{
		SKIN_DBG("[skin menu] callback get data, nums = %d, down id = %d \n",cur_add,down_index);

		memset(xml_path,0,FULL_XML_PATH_LENGTH);
		snprintf(xml_path, FULL_XML_PATH_LENGTH, "%s/%s",PATH_VER_MAIN,
				s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[down_index].ddir);

		APP_FREE(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add].ddir);
		s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add].ddir = GxCore_Strdup(xml_path);

		ret = gxapp_file_get_data(XML_DOWN_PATH,&s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add]);
		if (ret == 0 )
		{
			if ( gxapp_exist_installed_uiid(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add].uiid) )
			{
				s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add].uninstall = 1 ;
			}
			else
			{
				s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur_add].uninstall = 0 ;
			}

			s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num++ ;
		}
	}
	else
	{
		SKIN_DBG("[skin menu] callback over cur =%d, %d \n",cur_add,down_index);
	}
}

static int _app_skin_download_xml_callback(int down_flag,unsigned int down_size)
{
	int focus_item ;
	SKIN_DBG("[skin menu] xml callback = %d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if ( (down_flag <=0) || (GxCore_FileExists(XML_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
	{
		SKIN_ERR("[skin menu] xml down failer \n");
	}
	else
	{
		SKIN_DBG("[skin menu] xml down ok \n");

		_app_skin_online_callback_get_data();
	}

	_skin_ui_xml_download();

	if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE)
	{
		SKIN_DBG("[skin menu] left focus \n");

		focus_item = s_skin_menu_info.right_table_sel ;
		_skin_unfocus_item(focus_item);

		_skin_clean_all_item();

		_right_table_update();

		if( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
		{
			if (s_skin_menu_info.right_table_nums <=0)
			{
				_table_exit_keypress();
			}
			else
			{
				if (s_skin_menu_info.right_table_sel < s_skin_menu_info.right_table_nums)
				{
					focus_item = s_skin_menu_info.right_table_sel ;
				}
				else
				{
					s_skin_menu_info.right_table_sel = 0 ;
					focus_item = 0 ;
				}
				_skin_focus_item(focus_item);
			}
		}

		_table_icon_draw();
	}

	return 0;
}

static void _skin_ui_xml_download(void)
{
	int i = 0 ;
	int cur = 0 ;
	int down_all_num = 0 ;
	char xml_path[FULL_XML_PATH_LENGTH] ;

	down_all_num = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].dir_all_num ;
	cur = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].cur_down_num ;
	if ( (down_all_num <=0) || (cur>=down_all_num) )
	{
		SKIN_DBG("[skin menu] online xml download exit,down id=%d,all=%d,nums=%d \n",cur,
				down_all_num,s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num);

		if (s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num < down_all_num)
		{
			i= s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num;
			for(;i<down_all_num;i++)
			{
				APP_FREE(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[i].ddir);
			}
			s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].dir_all_num = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num ;
		}

		return ;
	}
	if ( s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur].ddir == NULL )
	{
		SKIN_DBG("[skin menu] online xml download exit2,index = %d,%d \n",cur,down_all_num);
		return ;
	}

	SKIN_DBG("[skin menu] online xml download,down id=%d,down all=%d, nums = %d \n",cur,
				down_all_num,s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num);

	gxapp_skin_download_api_multi_download_stop();

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skin_download_xml_callback, 10);

	memset(xml_path,0,FULL_XML_PATH_LENGTH);
	snprintf(xml_path, FULL_XML_PATH_LENGTH, "%s/%s/%s/%s",gxapp_skin_online_get_url(),
	   PATH_VER_MAIN,s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[cur].ddir,SKIN_XML_NAME);

	SKIN_DBG("[skin menu] online xml url = %s \n",xml_path);

	gxapp_skin_download_api_async_download(xml_path,XML_DOWN_PATH);//

	s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].cur_down_num++;
}

static int _app_skin_download_dir_callback(int down_flag,unsigned int down_size)
{
	SKIN_DBG("[skin menu] dir callback = %d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if ( (down_flag <=0) || (GxCore_FileExists(DIR_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
	{
		SKIN_ERR("[skin menu] dir down failer \n");
	}
	else
	{
		SKIN_DBG("[skin menu] dir down ok \n");

		gxapp_htmlfile_get_data(DIR_DOWN_PATH,&s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE]);

		if ( s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].dir_all_num > 0 )
		{
			_skin_ui_xml_download();
		}
	}

	return 0;
}

static void _skin_ui_dir_download(void)
{
	char *p_url = NULL ;

	char xml_path[FULL_XML_PATH_LENGTH] ;

	p_url = gxapp_skin_online_get_url();
	if ( p_url == NULL )
	{
		SKIN_DBG("[skin menu] online url error \n");
		return ;
	}

	gxapp_skin_download_api_multi_download_stop();

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skin_download_dir_callback, 10);

	memset(xml_path,0,FULL_XML_PATH_LENGTH);
	snprintf(xml_path, FULL_XML_PATH_LENGTH, "%s/%s/",p_url,PATH_VER_MAIN);

	SKIN_DBG("[skin menu] online dir url = %s \n",xml_path);

	s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node = malloc(SKIN_MAX_LIST * sizeof(skin_item_node_t));
	if (s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node == NULL)
	{
		SKIN_DBG("[skin menu] malloc failed!\n");
		return ;
	}
	memset(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node, 0, SKIN_MAX_LIST*sizeof(skin_item_node_t));

	gxapp_skin_download_api_async_download(xml_path,DIR_DOWN_PATH);//
}

//////

static int _app_skin_download_pic_callback(int index,char *path)
{
	struct SkinUICtrl param = {0};

	if ( s_skin_menu_info.left_group_active != SKIN_CAGEGORY_ONLINE )
	{
		return 0;
	}

	if (path == NULL )
	{
		SKIN_ERR("[skin menu] pic callback, path err \n");
		return -1 ;
	}
	else
	{
		if(GXCORE_FILE_UNEXIST == GxCore_FileExists(path))
		{
			SKIN_ERR("[skin menu] pic file err \n");
			return -1 ;
		}
	}

	memset(&param, 0, sizeof(struct SkinUICtrl));
    param.type = SKIN_MSG_UPDATE_ITEM_IMAGE;
    param.item = index;
    param.image_path = path ;
    _skin_ui_contral(&param);

	return 0;
}

static void _app_skin_ui_pic_download(char *url, int index)
{
	char *p_url ;

	char *server_url = NULL ;

	server_url = gxapp_skin_online_get_url();
	if ( server_url == NULL )
	{
		SKIN_DBG("[skin menu] online url error \n");
		return ;
	}

    if(url == NULL)
	{
		SKIN_ERR("[skin menu] #### no pic url \n");
		gxapp_skin_download_api_multi_async_download(NULL,index);
        return;
	}
	if( index >= MAX_PAGE_ITEM )
    {
        SKIN_ERR("[skin menu] #### no server url \n");
		gxapp_skin_download_api_multi_async_download(NULL,index);
        return;
    }

	p_url = GxCore_Calloc(sizeof(char), FULL_XML_PATH_LENGTH);
    if(NULL == p_url)
    {
        SKIN_ERR("[skin menu] #### No memory to GxCore_Calloc\n");
        return;
    }

	snprintf(p_url, FULL_XML_PATH_LENGTH, "%s/%s", server_url, url);

	//SKIN_DBG("[skin menu] icon url = %s \n",p_url);

	gxapp_skin_download_api_multi_async_download(p_url,index);

	APP_FREE(p_url);

    return;
}

static void _table_icon_draw(void)
{
    char *url = NULL;
    int i =0;
    struct SkinUICtrl param = {0};
    int sel = 0;

	if ( s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE )
	{
		gxapp_skin_download_api_multi_download_stop();
		gxapp_skin_download_api_multi_download_start();
		gxapp_skin_download_api_multi_timer_start(_app_skin_download_pic_callback,200);
	}

    sel = (s_skin_menu_info.right_table_sel - s_skin_menu_info.right_table_sel % MAX_PAGE_ITEM);
    for(i=0; i < MAX_PAGE_ITEM; i++)
    {
        if(sel >= s_skin_menu_info.right_table_nums)
            break;
        param.type = SKIN_MSG_UPDATE_ITEM_IMAGE;
        param.item = i;

		if ( s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE )
		{
			param.image_path = DEFAULT_ICON_PATH;
			url = s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[sel].icon_url;
            _app_skin_ui_pic_download(url, i);
		}
		else
		{
			if ( s_skin_menu_info.Inode[s_skin_menu_info.left_group_active].node[sel].icon_url != NULL )
				param.image_path = s_skin_menu_info.Inode[s_skin_menu_info.left_group_active].node[sel].icon_url ;
			else
				param.image_path = DEFAULT_ICON_PATH;
		}

        _skin_ui_contral(&param);
        sel++;
    }
}

static void _skin_ui_update_uiid_data(void)
{
	int i = 0;
	int left  = 0 ;
	int right = 0 ;

	for(i=0;i<s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num ;i++)
	{
		if ( gxapp_exist_installed_uiid(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[i].uiid) )
		{
			SKIN_DBG("[skin menu] update online id=%d, uiid=%d \n",i,s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[i].uiid);

			s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[i].uninstall = 1 ;
		}
		else
		{
			s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[i].uninstall = 0 ;
		}
	}

#if SKUSB_SUPPORT
	for(i=0;i<s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].num ;i++)
	{
		if ( gxapp_exist_installed_uiid(s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].node[i].uiid) )
		{
			SKIN_DBG("[skin menu] update usb id=%d, uiid=%d \n",i,s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].node[i].uiid);

			s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].node[i].uninstall = 1 ;
		}
		else
		{
			s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].node[i].uninstall = 0 ;
		}
	}
#endif

	left  = s_skin_menu_info.left_group_active ;
    right = s_skin_menu_info.right_table_sel   ;

	if ( left != SKIN_CAGEGORY_INSTALLED )
	{
		if ( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
		{
			if (s_skin_menu_info.Inode[left].node[right].uninstall)
			{
				GUI_SetProperty(TXT_GREEN, "state", "hide");
				GUI_SetProperty(IMG_GREEN, "state", "hide");
			}
			else
			{
				GUI_SetProperty(TXT_GREEN, "state", "show");
				GUI_SetProperty(IMG_GREEN, "state", "show");
				GUI_SetProperty(TXT_GREEN, "string", "Install");
			}
		}
	}
}

/////////////////////
// bin download

static int _skin_ui_update_failer_tips(char *str)
{
	PopDlg  pop;

	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_OK;
	if ( str != NULL )
		pop.str = str ;
	else
		pop.str = "normal tips";

	pop.mode = POP_MODE_UNBLOCK;
	pop.pos.x = APP_POP_DLG_POS_X;
	pop.pos.y = APP_POP_DLG_POS_Y;
	pop.show_time = true;
	pop.timeout_sec = 5;
	popdlg_create(&pop);

	return 0 ;
}

static void _skin_ui_write_theme(int write_id)
{
	char *file_name = NULL ;
	char *simg_md5 = NULL ;
	char *dimg_md5 = NULL ;
	int ret ;
	int active_id ;
	int is_reboot ;
	PopDlg  pop;

	file_name = s_skin_menu_info.simg_url ;
	simg_md5  = s_skin_menu_info.simg_md5 ;
	dimg_md5  = s_skin_menu_info.dimg_md5 ;
	if ( (file_name == NULL ) || (simg_md5 == NULL) )
	{
		SKIN_ERR("[skin menu] #### check parition para error \n");
		_skin_ui_update_failer_tips(SKIN_STR_ID_ERR_ERR_PARA);
		return ;
	}

	active_id = gxapp_skin_flash_get_active_partition_id();
	if ( write_id == active_id )
		is_reboot = 1;
	else
		is_reboot = 0 ;

	SKIN_DBG("[skin menu] write id=%d, active id=%d, reboot=%d \n", write_id, active_id,is_reboot);

	memset(&pop, 0, sizeof(PopDlg));

	pop.type = POP_TYPE_NO_EXIT;
	pop.str = SKIN_STR_ID_TIPS_WAIT;
	pop.mode = POP_MODE_UNBLOCK;
	pop.pos.x = APP_POP_DLG_POS_X;
	pop.pos.y = APP_POP_DLG_POS_Y;
	popdlg_create(&pop);

	GUI_SetInterface("flush",NULL);

	ret = gxapp_skin_flash_update_partition(write_id,file_name,simg_md5,dimg_md5);
	if (ret != 0)
	{
		popdlg_destroy();
		_skin_ui_update_failer_tips(gxapp_skin_flash_get_err_tips(ret));
		return ;
	}

	if ( is_reboot )
	{
		gxapp_skin_app_reboot();
	}
	else
	{
		popdlg_destroy();
		ret = _ui_get_flash_data();
		if (ret != 0 )
		{
			_skin_ui_update_failer_tips("Skin simple data err!");
		}

		_skin_ui_update_uiid_data();
		/*
		if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE)
		{
			_skin_ui_update_skin_data(SKIN_CAGEGORY_ONLINE);
			_ui_get_usb_data();
		}
		else if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_USB)
		{
			_skin_ui_update_skin_data(SKIN_CAGEGORY_USB);
			_ui_get_online_data();
		}*/
	}
}

static int _skin_ui_install_reboot_tip_exit_cb(PopDlgRet ret)
{
	int active_id = 0 ;

	if ( POP_VAL_OK == ret )
	{
		active_id = gxapp_skin_flash_get_active_partition_id() ;

		SKIN_DBG("[skin menu] install_exit_cb id =%d \n", active_id);

		_skin_ui_write_theme(active_id);
	}
	return 0 ;
}

static int _skin_ui_install_id_poplist_cb(int32_t ret_sel, unsigned short key)
{
	PopDlg  pop;

	if (key != STBK_OK )
		return -1 ;

	SKIN_DBG("[skin menu] poplist ui sel id =%d \n", ret_sel);

	if (ret_sel >= 0 )
	{
		if (ret_sel == gxapp_skin_flash_get_active_partition_id() )
		{
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_YES_NO;
			pop.str = SKIN_STR_ID_TIPS_REBOOT;
			pop.exit_cb = _skin_ui_install_reboot_tip_exit_cb;
			pop.mode = POP_MODE_UNBLOCK;
			pop.pos.x = APP_POP_DLG_POS_X;
			pop.pos.y = APP_POP_DLG_POS_Y;
			popdlg_create(&pop);

			return 0;
		}
		else
		{
			_skin_ui_write_theme(ret_sel);

			if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE)
			{
				SKIN_DBG("[skin menu] del img file \n");
				gxapp_skin_download_delete_img_file();
			}
		}
	}

	return 0;
}

int _skin_ui_select_install_id(void)
{
	int i ;
	int num ;
	PopList pop_list;

	num = s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].num ;

	for(i=0;i<num;i++)
	{
		s_skin_install_id[i] = s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].node[i].name ;
	}

	i = gxapp_skin_flash_get_active_partition_id();
	if ( (i>=0) && (i<num) )
	{
		//SKIN_CAGEGORY_INSTALLED，simg_url变量就是名字加(active)
		if ( s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].node[i].simg_url != NULL )
		{
			s_skin_install_id[i] = s_skin_menu_info.Inode[SKIN_CAGEGORY_INSTALLED].node[i].simg_url ;
		}
	}

	memset(&pop_list, 0, sizeof(PopList));

	pop_list.title = SKIN_STR_ID_INSTALL_POP_TITLE;
	pop_list.item_num = num;
	pop_list.item_content = (char **)s_skin_install_id;
	pop_list.sel = 0 ;
	pop_list.show_num= true;
	pop_list.mode = POP_MODE_UNBLOCK;
	pop_list.exit_cb = _skin_ui_install_id_poplist_cb;
	poplist_create(&pop_list);

	return -1 ;
}

static int _skin_ui_update_img_proc(void)
{
	_skin_ui_select_install_id();
	return 0 ;
}

static int _app_skin_download_img_callback(int down_flag,unsigned int down_size)
{
	int sel = 0 ;

	SKIN_DBG("[skin menu] img callback = %d,%d \n",down_flag,down_size);

	gxapp_skin_download_api_callback_stop();

	if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE)
	{
		SKIN_DBG("[skin menu] online focus \n");

		if ( (down_flag <=0) || (GxCore_FileExists(IMG_DOWN_PATH) == GXCORE_FILE_UNEXIST) )
		{
			SKIN_ERR("[skin menu] #### img down failer \n");
			_skin_ui_update_failer_tips(SKIN_STR_ID_TIPS_DOWNLOAD_FAILER);
		}
		else
		{
			sel = s_skin_menu_info.right_table_sel ;

			SKIN_DBG("[skin menu] img down ok ,sel = %d \n",sel);

			gxapp_skin_flash_set_update_mode(SKIN_CAGEGORY_ONLINE);
			gxapp_skin_flash_set_update_imgsel(sel);

			APP_FREE(s_skin_menu_info.simg_url);
			APP_FREE(s_skin_menu_info.simg_md5);
			APP_FREE(s_skin_menu_info.dimg_md5);

			s_skin_menu_info.simg_url = GxCore_Strdup(IMG_DOWN_PATH);
			if ( s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[sel].simg_md5 != NULL )
				s_skin_menu_info.simg_md5 = GxCore_Strdup(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[sel].simg_md5);
			if ( s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[sel].dimg_md5 != NULL )
				s_skin_menu_info.dimg_md5   = GxCore_Strdup(s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[sel].dimg_md5);

			_skin_ui_update_img_proc();
		}
	}

	return 0;
}

static void _app_skin_ui_img_download(void)
{
	char *p_server = NULL ;
	char *p_url ;
	int index ;

	if (s_skin_menu_info.left_group_active != SKIN_CAGEGORY_ONLINE)
	{
		SKIN_DBG("[skin menu] #### not focus online \n");
		return ;
	}

	index =s_skin_menu_info.right_table_sel ;
	if ( index >= s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].num )
	{
		SKIN_DBG("[skin menu] #### index = %d over \n",index);
        return;
	}

	p_server = gxapp_skin_online_get_url() ;
	if(NULL == p_server)
    {
        SKIN_ERR("[skin menu] #### No Server url \n");
        return;
    }

	p_url = GxCore_Calloc(sizeof(char), FULL_XML_PATH_LENGTH);
    if(NULL == p_url)
    {
        SKIN_ERR("[skin menu] #### No memory to GxCore_Calloc\n");
        return;
    }

	gxapp_skin_download_api_multi_download_stop();

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_download_start();

	gxapp_skin_download_api_timer_start(_app_skin_download_img_callback, 20);

	snprintf(p_url, FULL_XML_PATH_LENGTH, "%s/%s", p_server, s_skin_menu_info.Inode[SKIN_CAGEGORY_ONLINE].node[index].simg_url);
	SKIN_DBG("[skin menu] index= %d, img url = %s \n",index,p_url);

	gxapp_skin_download_api_async_download(p_url,IMG_DOWN_PATH);

	APP_FREE(p_url);
}

////////////////////

static void app_skin_menu_init(void)
{
	SKIN_DBG("[skin menu] time = %s,%s \n",__DATE__,__TIME__);

	gxapp_skin_setting_init();

	_skin_info_global_init();

	_ui_get_flash_data();
	_ui_get_online_data();

#if SKUSB_SUPPORT
	_ui_get_usb_data();
#endif

	_skin_group_list_active();

	_right_table_update();

	_table_icon_draw();

	_skin_left_focus_draw_button();

	_skin_ui_enable_active_id_img();
}

static void _skin_menu_online_download_uninit(void)
{
	SKIN_DBG("[skin menu] stop all download \n");

	gxapp_skin_download_api_download_stop();

	gxapp_skin_download_api_multi_download_stop();

	gxapps_curl_set_timeout(0, 0);
}

static void app_skin_menu_destroy(void)
{
	_skin_menu_online_download_uninit();

	gxapp_skin_setting_uninit();

	_skin_info_global_uninit();
}

static void _skin_ui_update_skin_data(int type)
{
	int focus_item = 0 ;

	SKIN_DBG("[skin menu] #### update now \n");

	if ( SKIN_CAGEGORY_ONLINE == type )
	{
		_skin_menu_online_download_uninit();
	}

	focus_item = s_skin_menu_info.right_table_sel ;
	_skin_unfocus_item(focus_item);

	_skin_clean_all_item();

	if ( SKIN_CAGEGORY_INSTALLED == type )
	{
		_ui_get_flash_data();
	}
	else if ( SKIN_CAGEGORY_ONLINE == type )
	{
		_ui_get_online_data();
	}
#if SKUSB_SUPPORT
	else if ( SKIN_CAGEGORY_USB == type )
	{
		_ui_get_usb_data();
	}
#endif

    _right_table_update();

	if( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
	{
		if (s_skin_menu_info.right_table_nums <=0)
		{
			_table_exit_keypress();
		}
		else
		{
			if (s_skin_menu_info.right_table_sel < s_skin_menu_info.right_table_nums)
			{
				focus_item = s_skin_menu_info.right_table_sel ;
			}
			else
			{
				s_skin_menu_info.right_table_sel = 0 ;
				focus_item = 0 ;
			}
			_skin_focus_item(focus_item);
		}
	}

	_table_icon_draw();
}

static int _exit_cb_set_active_partition_id(PopDlgRet ret)
{
	int left , right ;

	SKIN_DBG("[skin menu] ui user ret = %d \n",ret);

	if ( ret == POP_VAL_OK )
	{
		left  = s_skin_menu_info.left_group_active ;
		right = s_skin_menu_info.right_table_sel ;
		if ( left == SKIN_CAGEGORY_INSTALLED )
		{
			SKIN_DBG("[skin menu] ui user set active id = %d \n",right);
			gxapp_skin_flash_set_active_partition_id(right);

			gxapp_skin_app_reboot();
		}
	}

	return 0 ;
}

static int _skin_set_active_partition_id_ui_ctrl(void)
{
	PopDlg  pop;

	memset(&pop, 0, sizeof(PopDlg));
	pop.type = POP_TYPE_YES_NO;
	pop.str = "Do you want to reboot by set skin active?\n" ;
	pop.mode = POP_MODE_UNBLOCK;
	pop.pos.x = APP_POP_DLG_POS_X;
	pop.pos.y = APP_POP_DLG_POS_Y;
    pop.exit_cb = _exit_cb_set_active_partition_id ;
	popdlg_create(&pop);

	return 0 ;
}

static void _skin_preview_ui_ctrl(void)
{
	int i ;
	skin_item_node_t *pnode = NULL ;
	skin_preview_node_t preview[MAX_PREVIEW_NODE_NUM];

	int left ;
	int right ;
	unsigned int uiid  ;

	if(GUI_CheckDialog(WND_SKPREVIEW) == GXCORE_SUCCESS)
	{
		return ;
	}

	left  = s_skin_menu_info.left_group_active ;
	right = s_skin_menu_info.right_table_sel ;

	pnode = &s_skin_menu_info.Inode[left].node[right];

	uiid = pnode->uiid ;

	for(i=0;i<MAX_PREVIEW_NODE_NUM;i++)
	{
		preview[i].pre_url = NULL ;
	}

	for(i=0;i<MAX_PREVIEW_NODE_NUM;i++)
	{
		preview[i].pre_url = pnode->preview[i].pre_url ;
	}

	if ( left == SKIN_CAGEGORY_INSTALLED )
	{
		if ( gxapp_skin_flash_get_status(right))
		{
			if ( right == gxapp_skin_flash_get_active_partition_id() )
			{
				gxapp_set_preview_info(-1,uiid,preview);
			}
			else
			{
				gxapp_skin_flash_other_mount(right);

				gxapp_set_preview_info(0,uiid,preview);
			}
		}
	}
	else if ( left == SKIN_CAGEGORY_ONLINE )
	{
		gxapp_skin_download_api_multi_download_stop();
		gxapp_set_preview_info(1,uiid,preview);
	}
#if SKUSB_SUPPORT
	else if ( left == SKIN_CAGEGORY_USB )
	{
		gxapp_set_preview_info(-2,uiid,preview);
	}
#endif
}

SIGNAL_HANDLER int app_skin_create(GuiWidget *widget, void *usrdata)
{
    app_skin_menu_init();
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_skin_destroy(GuiWidget *widget, void *usrdata)
{
    app_skin_menu_destroy();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skin_keypress(GuiWidget *widget, void *usrdata)
{
	int left ;
	int right ;

    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
			case STBK_INFO:
				_skin_preview_ui_ctrl();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                _table_exit_keypress();
                break;
            case STBK_UP:
                _table_up_keypress();
                break;
            case STBK_DOWN:
                _table_down_keypress();
                break;
            case STBK_LEFT:
                _table_left_keypress();
                break;
            case STBK_RIGHT:
                _table_right_keypress();
                break;
            case STBK_RED:
                break;

            case STBK_GREEN:
				{
					left  = s_skin_menu_info.left_group_active ;
					right = s_skin_menu_info.right_table_sel ;
					if ( left == SKIN_CAGEGORY_INSTALLED )
					{
						int id ;
						id = gxapp_skin_flash_get_active_partition_id();
						if (right == id ) //active can not uninstall
						{
							return EVENT_TRANSFER_STOP;
						}
						if ( 0 == gxapp_skin_flash_get_status(right) )//status is 0
						{
							return EVENT_TRANSFER_STOP;
						}

						gxapp_skin_flash_set_status(right,0);
						gxapp_set_installed_uiid(right,0);

						_skin_ui_update_skin_data(left);
                        // to update uiid
						_skin_ui_update_uiid_data();
						/*
						_ui_get_online_data();
						_ui_get_usb_data();
						*/
					}
					else
					{
						if ( s_skin_menu_info.Inode[left].node[right].uninstall )
						{
							return EVENT_TRANSFER_STOP;
						}
						if ( left == SKIN_CAGEGORY_ONLINE )
						{
							_app_skin_ui_img_download();
						}
#if SKUSB_SUPPORT
						else if ( left == SKIN_CAGEGORY_USB )
						{
							APP_FREE(s_skin_menu_info.simg_url);
							APP_FREE(s_skin_menu_info.simg_md5);
							APP_FREE(s_skin_menu_info.dimg_md5);

							if ( s_skin_menu_info.Inode[left].node[right].simg_url != NULL )
								s_skin_menu_info.simg_url = GxCore_Strdup(s_skin_menu_info.Inode[left].node[right].simg_url);
							if ( s_skin_menu_info.Inode[left].node[right].simg_md5 != NULL )
								s_skin_menu_info.simg_md5 = GxCore_Strdup(s_skin_menu_info.Inode[left].node[right].simg_md5);
							if ( s_skin_menu_info.Inode[left].node[right].dimg_md5 != NULL )
								s_skin_menu_info.dimg_md5 = GxCore_Strdup(s_skin_menu_info.Inode[left].node[right].dimg_md5);

							gxapp_skin_flash_set_update_mode(SKIN_CAGEGORY_USB);
							gxapp_skin_flash_set_update_imgsel(right);

							_skin_ui_update_img_proc();
						}
#endif
					}
				}
                break;
            case STBK_YELLOW:
				{
					left  = s_skin_menu_info.left_group_active ;
					right = s_skin_menu_info.right_table_sel ;
					if ( left == SKIN_CAGEGORY_INSTALLED )
					{
						int id ;
						id = gxapp_skin_flash_get_active_partition_id();
						if (right == id ) //active can not active
						{
							return EVENT_TRANSFER_STOP;
						}
						if ( 0 == gxapp_skin_flash_get_status(right) )//status is 0
						{
							return EVENT_TRANSFER_STOP;
						}

						SKIN_DBG("[skin menu] ui right id = %d \n",right);
						_skin_set_active_partition_id_ui_ctrl();
					}
				}
                break;
            case STBK_BLUE:
					if ( s_skin_menu_info.left_group_active != SKIN_CAGEGORY_INSTALLED )
					{
						_skin_ui_update_skin_data(s_skin_menu_info.left_group_active);
					}
                break;
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

static void _skin_group_list_up_keypress(void)
{
    if(s_skin_menu_info.left_group_nums >1)
    {
        if(s_skin_menu_info.left_group_sel <= 0)
        {
            s_skin_menu_info.left_group_sel = s_skin_menu_info.left_group_nums - 1;
        }
		else
		{
			s_skin_menu_info.left_group_sel -= 1;
		}
        _skin_group_list_focus();
    }
}

static void _skin_group_list_down_keypress(void)
{
    if(s_skin_menu_info.left_group_nums > 1)
    {
        s_skin_menu_info.left_group_sel += 1;
        if( s_skin_menu_info.left_group_sel > ( s_skin_menu_info.left_group_nums- 1))
        {
            s_skin_menu_info.left_group_sel = 0;
        }
        _skin_group_list_focus();
    }
}

static void _skin_group_list_ok_keypress(void)
{
	if ( s_skin_menu_info.left_group_active == s_skin_menu_info.left_group_sel )
	{
		SKIN_DBG("[skin menu] group ok same \n");
		return ;
	}
	//防止在更新的时候，切换到其它标签，比如usb等，停止更新
	if (s_skin_menu_info.left_group_active == SKIN_CAGEGORY_ONLINE )
	{
		if ( s_skin_menu_info.right_table_nums > 0 )
		{
			gxapp_skin_download_api_multi_download_stop();
			gxapp_skin_download_api_download_stop();
		}
	}

    _skin_clean_all_item();

	s_skin_menu_info.left_group_active = s_skin_menu_info.left_group_sel ;
    _skin_group_list_active();

    _right_table_update();

	_skin_left_focus_draw_button();

	_table_icon_draw();

	if (s_skin_menu_info.left_group_active != SKIN_CAGEGORY_INSTALLED )
	{
		_skin_ui_disable_active_id_img();
	}
}


SIGNAL_HANDLER int app_skin_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;

	gxapp_skin_check_key_to_debug(event->key.sym);
/*
    if(s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM)
    {
        GUI_SendEvent(WND_SKIN, event);
        return ret;
    }
*/
    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                app_skin_close_menu();
                break;
            case STBK_OK:
                _skin_group_list_ok_keypress();
                break;
            case STBK_RIGHT:
				if (s_skin_menu_info.right_table_nums > 0 )
				{
					s_skin_menu_info.focus_area = SKIN_FOCUSAREA_TABLE_ITEM;
					GUI_SetFocusWidget(BTN_RIGHT);
					_skin_focus_item(s_skin_menu_info.right_table_sel);
					_skin_right_focus_draw_button();
				}
                break;
            case STBK_UP:
                _skin_group_list_up_keypress();
                break;
            case STBK_DOWN:
                _skin_group_list_down_keypress();
                break;
            case STBK_BLUE:
				ret = EVENT_TRANSFER_KEEPON;
                break;
            case STBK_RED:
				{
					extern void app_create_skin_setting_menu(void);
					app_create_skin_setting_menu();
				}
                break;
            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_skin_group_list_get_total(GuiWidget *widget, void *usrdata)
{
	return s_skin_menu_info.left_group_nums ;
    //return SKIN_CATEGORY_MAX;
}

SIGNAL_HANDLER int app_skin_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if(item->sel >= SKIN_CATEGORY_MAX)
        return EVENT_TRANSFER_STOP;
    item->image = NULL;
    item->string = skin_category_str[item->sel];

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skin_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skin_lost_focus(GuiWidget *widget, void *usrdata)
{
	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_skin_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

static int _app_skin_ui_close_other_win(char *focus_win)
{
	if (focus_win == NULL )
		return 0 ;

	if ( 0 == strcasecmp(focus_win, WND_POP_TIP) )
	{
		popdlg_destroy();
	}
	else if ( 0 == strcasecmp(focus_win, WND_SKPREVIEW) )
	{
		GUI_EndDialog(WND_SKPREVIEW);
	}
	else if ( 0 == strcasecmp(focus_win, WND_SYSTEM_SETTING) )
	{
		GUI_EndDialog(WND_SYSTEM_SETTING);
	}
	return 0 ;
}

int app_skin_msg_proc(GxMessage *msg)
{
	char* focus_win = NULL;

	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

    extern int app_skrecover_msg_proc(GxMessage *msg);

	if (GUI_CheckDialog(WND_SKRECOVERY) == GXCORE_SUCCESS)
    {
		return app_skrecover_msg_proc(msg);
	}

	if(GUI_CheckDialog(WND_SKIN) == GXCORE_ERROR)
        return EVENT_TRANSFER_STOP;

    focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return EVENT_TRANSFER_STOP;

	_app_skin_ui_close_other_win(focus_win);

	switch(msg->msg_id)
	{
#if NETWORK_SUPPORT
		case GXMSG_NETWORK_STATE_REPORT:
			{
				GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
				if ( dev_state != NULL )
				{
					SKIN_ERR("[skin menu] #### msg state = %d,%d \n",dev_state->dev_state,IF_STATE_CONNECTED);
					if(dev_state->dev_state == IF_STATE_CONNECTED)
					{
						if (  0 == gxapp_skin_online_get_connect_state() )
						{
							if (s_skin_menu_info.left_group_active  == SKIN_CAGEGORY_ONLINE)
							{
								_skin_ui_update_skin_data(SKIN_CAGEGORY_ONLINE);
							}
							else
							{
								_ui_get_online_data();
							}
						}
					}
					else
					{
						if ( 1 == gxapp_skin_online_get_connect_state() )
						{
							if (s_skin_menu_info.left_group_active  == SKIN_CAGEGORY_ONLINE)
							{
								_skin_ui_update_skin_data(SKIN_CAGEGORY_ONLINE);
							}
							else
							{
								_ui_get_online_data();
							}
						}
					}
				}
			}
			break;
#endif

		case GXMSG_USBWIFI_HOTPLUG_OUT:
		case GXMSG_USBNET_HOTPLUG_OUT:
			{
				SKIN_ERR("[skin menu] #### wifi net out, msg id = %d \n",msg->msg_id);

				if ( 1 == gxapp_skin_online_get_connect_state() )
				{
					if (s_skin_menu_info.left_group_active  == SKIN_CAGEGORY_ONLINE)
					{
						_skin_ui_update_skin_data(SKIN_CAGEGORY_ONLINE);
					}
					else
					{
						_ui_get_online_data();
					}
				}
			}
			break;

#if SKUSB_SUPPORT
		case GXMSG_HOTPLUG_IN:
			{
				int sel = -1;
				int ret ;
				HotplugPartitionList *partition_list = NULL;

				if( GUI_CheckDialog(WND_SKIN) == GXCORE_SUCCESS )
				{
					partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
					if(partition_list&&(partition_list->partition_num > 0))
					{
						SKIN_ERR("[skin menu] #### usb in \n");
						if (s_skin_menu_info.left_group_nums == (SKIN_CATEGORY_MAX -1) )
						{
							ret = _ui_get_usb_data();
							GUI_GetProperty(WND_LIST_GROUP, "active", &sel);
							if (ret != SKIN_NO_USB_RETURN)
							{
								s_skin_menu_info.left_group_nums = SKIN_CATEGORY_MAX ;
							}
							GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
							GUI_SetProperty(WND_LIST_GROUP, "active", &sel);
						}
						else if ((s_skin_menu_info.left_group_nums == SKIN_CATEGORY_MAX) && (s_skin_menu_info.Inode[SKIN_CAGEGORY_USB].num == 0))
						{
							ret = _ui_get_usb_data();
							if (ret == 0)
							{
								if ( s_skin_menu_info.left_group_active == SKIN_CAGEGORY_USB)
								{
									SKIN_ERR("[skin menu] #### usb active \n");
									_skin_clean_all_item();
									_right_table_update();
									_skin_left_focus_draw_button();
									_table_icon_draw();
									_skin_ui_disable_active_id_img();
								}
							}
						}
					}
				}
			}
			break;
		case GXMSG_HOTPLUG_OUT:
			{
				int sel = -1;
				int ret = 0 ;

				if( GUI_CheckDialog(WND_SKIN) == GXCORE_SUCCESS )
				{
					SKIN_ERR("[skin menu] #### usb out \n");

					if (s_skin_menu_info.left_group_nums == SKIN_CATEGORY_MAX )
					{
						ret = _ui_get_usb_data();
						if (ret == SKIN_NO_USB_RETURN ) //no usb
						{
							s_skin_menu_info.left_group_nums = SKIN_CATEGORY_MAX -1 ;
						}
						GUI_GetProperty(WND_LIST_GROUP, "active", &sel);
						if (sel == (SKIN_CATEGORY_MAX -1))
						{
							if (ret == SKIN_NO_USB_RETURN )
							{
								SKIN_DBG("[skin menu] #### group update\n");
								sel = s_skin_menu_info.right_table_sel ;
								_skin_unfocus_item(sel);

								s_skin_menu_info.left_group_sel = 0 ;
								sel = 0 ;
								GUI_SetProperty(WND_LIST_GROUP, "select", &sel);

								_skin_group_list_ok_keypress();
								GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
								GUI_SetProperty(WND_LIST_GROUP, "active", &sel);

								if( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
								{
									sel = s_skin_menu_info.right_table_sel ;
									_skin_focus_item(sel);
									_skin_right_focus_draw_button();
								}
							}
							else
							{
								if (ret != 0 )
								{
									SKIN_DBG("[skin menu] #### no data in other usb \n");
									_skin_unfocus_item(s_skin_menu_info.right_table_sel);
									_skin_clean_all_item();
									if( s_skin_menu_info.focus_area == SKIN_FOCUSAREA_TABLE_ITEM )
									{
										s_skin_menu_info.focus_area = SKIN_FOCUSAREA_LIST_GROUP;
										_skin_group_list_focus();
									}
									_right_table_update();
									_skin_left_focus_draw_button();
									_skin_update_page_info();
								}
								else
								{
									SKIN_DBG("[skin menu] #### usb update\n");
									_skin_clean_all_item();
									_right_table_update();
									_skin_left_focus_draw_button();
									_table_icon_draw();
									_skin_ui_disable_active_id_img();
								}
							}
						}
						else
						{
							GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
							GUI_SetProperty(WND_LIST_GROUP, "select", &sel);
							GUI_SetProperty(WND_LIST_GROUP, "active", &sel);
						}
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

void app_create_skin_menu(void)
{
    if(GUI_CheckDialog(WND_SKIN) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_SKIN);
    }
}

#endif
