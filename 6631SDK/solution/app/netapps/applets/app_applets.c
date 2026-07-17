/*************************************************************************
	> File Name: app_applets.c
	> Author:
	> Mail:
	> Created Time: 2022/3/24
 ************************************************************************/

#include "app.h"
#if APPLETS_SUPPORT
#include "app_module.h"
#include "../app_netapps_service.h"
#include "app_windows.h"
#include "app_main_menu_tip.h"
#include "module/app_dir_module.h"
#include "service/gxhotplug.h"
#include "app_wnd_file_list.h"
#include <sys/stat.h>
#include <gx_applets.h>
#include <gxos/gxcore_os_core.h>
#include "app_applet_dll.h"
#include "app_wnd_system_setting_opt.h"

//#define APPLETS_DEBUG
#ifdef APPLETS_DEBUG
#define APPLETS_SUPPORT_USB
#define APPLETS_INFO(fmt, args...)  do {                             \
    printf(fmt, ##args);                                      \
} while(0)
#define APPLETS_ERR(fmt, args...)  do {                              \
        printf(fmt, ##args);                                     \
    } while(0)
#else
#define APPLETS_INFO(...) do{}while(0)
#define APPLETS_ERR(...) do{}while(0)
#endif


/*--------------------widget macro---------------------------*/

#define MAX_PAGE_ITEM                  (6)
#define MAX_PAGE_LINE                  (2)
#define MAX_PATH_LEN     (128)
#define APPLETS_GET_LIST_RETYR_TIMES (20)
#define APPLETS_INSTALL_TYPE_STR_LEN 128
#define MAX_RECENT_NUM         16
#define FLASH_MIN_FREE_SIZE   5*64*SIZE_UNIT_K
#define USB_MIN_FREE_SIZE   50*SIZE_UNIT_M
#define INSTALL_FLASH_MIN_NUM   3
#define INSTALL_FLASH_MAX_NUM   15

#ifdef ECOS_OS
#define IMAGE_DOWN_PATH             "/mnt/applets_"
#define TMP_INSTALL_PATH "/mnt/install"
#define TMP_ZIP_PATH "/mnt/applets.zip"
#else
#define IMAGE_DOWN_PATH             "/tmp/applets_"
#define TMP_INSTALL_PATH "/tmp/install"
#define TMP_ZIP_PATH "/tmp/applets.zip"
#endif

#define FLASH_ROOT_PATH "/home/gx"
#define APPLETS_SERVER_KEY "network>applets_server"
#define APPLETS_SERVER_DEFALT     "http://192.168.1.5:8080"

#define APPLETS_BUILTIN_LIST_PATH WORK_PATH"applets/applet_list.json"
#define APPLETS_BUILTIN_PATH WORK_PATH"applets/"
#define IMG_DEFAULT                    WORK_PATH"theme/image/netapps/default.png"
#define WND_SHORT_DESC                 "text_applets_item_shortdesc"
#define IMG_GIF                        "img_applets_gif"
#define GIF_PATH                       WORK_PATH"theme/image/netapps/loading.gif"
#define PIC_PATH_SUFFIX             ".jpg"

#define WND_APPLETS_PAGE_INFO    "text_applets_page_info"
#define WND_APPLETS_TITLE        "text_applets_title"
#define WND_LIST_GROUP                 "list_applets_group"
#define TXT_POPUP                      "txt_applets_popup"
#define IMG_POPUP                      "img_applets_popup"
#define TXT_RED           "text_applets_red"
#define IMG_RED           "img_applets_red"
#define TXT_GREEN           "text_applets_green"
#define IMG_GREEN           "img_applets_green"
#define TXT_YELLOW           "text_applets_yellow"
#define IMG_YELLOW           "img_applets_yellow"
#define TXT_BLUE           "text_applets_blue"
#define IMG_BLUE           "img_applets_blue"
#define TXT_OK           "text_applets_ok"
#define IMG_OK           "img_applets_ok"
#define BTN_RIGHT           "btn_applets_right"

#define ITEM_FOCUS_COLOR            "[net_video_focus_color,net_video_focus_color,net_video_focus_color]"
#define ITEM_UNFOCUS_COLOR          "[window_bg_color,window_bg_color,window_bg_color]"

#define APPLETS_INSTALL_TYPE_KEY	"netapps>applets_installtype"
#define APPLETS_DEFAULT_INSTALL_TYPE 0
#define APPLETS_CONFIG_ID	"netapps>applets_id"
#define APPLETS_CONFIG_PATH	"netapps>applets_path"

typedef enum {
    FOCUSAREA_LIST_GROUP = 0,
    FOCUSAREA_TABLE_TTEM,
    FOCUSAREA_TOTAL
} AppletsFocusArea;


enum MSG_CONTROL_STATUS{
    MSG_FOCUS_ITEM,
    MSG_UNFOCUS_ITEM,
    MSG_UNFOCUS_ALL_ITEM,
    MSG_CLEAN_ALL_ITEM,
    MSG_UPDATE_ITEM,
    MSG_UPDATE_ITEM_IMAGE,
    MSG_UPDATE_PAGE_INFO,
    MSG_GROUP_LIST_FOCUS,
    MSG_GROUP_LIST_UPDATE,
    MSG_GROUP_LIST_ACTIVE,
    MSG_SHOW_GIF,
    MSG_HIDE_GIF,
    MSG_SHOW_POPUP,
    MSG_HIDE_POPUP,
    MSG_UPDATE_ICON,
};
typedef enum {
    DOWNLOAD_MODE_ONLINE = 0,
    DOWNLOAD_MODE_INSTALL,
    DOWNLOAD_MODE_BUILTIN,
    DOWNLOAD_MODE_TOTAL
} AppletsLoadMode;
typedef enum {
	APPLETS_GET_LIST_NONE = 0,
	APPLETS_GET_LIST_PLEASE,
	APPLETS_GET_LIST_RUNNING,
	APPLETS_GET_LIST_SUCCESS,
	APPLETS_GET_LIST_FAILED
}AppletsLoginStatus;
typedef enum {
    INSTALL_PATH_FLASH,
    INSTALL_PATH_USB,
    DOWNLOAD_PATH_TOTAL
} AppletsInstallPath;

typedef struct applets_Infonode_s{
    int index; //
    int source; //0:cloud; 1:builtin
    int encryption;
    int fav;// 1:fav; 0:unfav
    int status;// 1:installed; 0:uninstall
    char *id;
    char *name;
    char *icon_url;
}applets_Infonode_t;

typedef struct applets_Infonodelist_s
{
	int applet_num;
	applets_Infonode_t *applets;
}applets_Infonodetlist_t;

typedef struct applets_download_data_s
{
	int download_num;
	char **download_urls;
	char **download_paths;
}applets_download_data_s;

struct AppletUICtrl{
    int type;
    int draw_mode;
    int item;
    int active;
    int total;
    char *image_path;
    char *status;
    applets_Infonode_t node;
};
typedef struct applets_focusinfo_s{
    int sel_group;
    int group_num;
    int sel_item;
    int item_num;
    int run_sel;
    bool run_from_mainmenu;
    AppletsLoadMode download_mode;
    AppletsFocusArea focus_area;
    applets_Infonodetlist_t * node;
}applets_focusinfo_t;

typedef struct
{
	AppletsInstallPath install_path;
}applets_setting_t;
typedef enum
{
	APPLETS_SETTING_INSTALL_MODE = 0,
	APPLETS_SETTING_MAX,
}applets_setting_itm;
typedef enum
{
    APPLETS_STATUS_SUCCESS = 0,
    APPLETS_STATUS_CRC_ERROR = -1,
    APPLETS_STATUS_FILE_ERROR = -2,
    APPLETS_STATUS_MKDIR_ERROR = -3,
    APPLETS_STATUS_UNZIP_ERROR = -4,
    APPLETS_STATUS_OBJ_CLOSE_ERROR = -5,
    APPLETS_STATUS_OBJ_OPEN_ERROR = -6,
    APPLETS_STATUS_DECRYPT_ERROR = -7,
    APPLETS_STATUS_UNKNOWN_ERROR = -8
}applets_open_status_t;

static int _applets_info_draw(void);
static char *this_pic_path[MAX_PAGE_ITEM] = {NULL};
static unsigned int download_pic_handle = 0;
static int pic_download_index[MAX_PAGE_ITEM];
static int pic_download_num = 0;
static int pic_download_ok[MAX_PAGE_ITEM];
static int pic_show_ok[MAX_PAGE_ITEM];
static int pic_download_abort = 0;
static event_list* s_applets_pic_update_timer = NULL;

static gxapps_ret_t applets_ret_status = GXAPPS_UNKNOWN_ERROR;
static applets_open_status_t applets_open_status = APPLETS_STATUS_SUCCESS;
static event_list* s_applets_gif_timer = NULL;
static event_list* s_applets_popup_timer = NULL;
static bool pop_msg_on = false;
static event_list* s_applets_data_update_timer = NULL;
static int applets_download_app_finish = 0;
static int  applets_clouddata_finish = 0;
static AppletsLoginStatus applets_get_list_status = APPLETS_GET_LIST_PLEASE;
static int applets_app_update_finish = 0;
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
static int applets_auto_update_list_running = 0;
static int applets_auto_update_list_count = 0;
#endif
typedef void(* applets_thread_func)(void);
static int applets_download_abort = 0;
static unsigned int applets_download_handle = 0;
static int is_app_downloading = 0;
static int is_applets_download = 0;
static unsigned int applets_data_handle = 0;
static gx_appletlist_t *applets_list = NULL;
#if APPLETS_FAV_SUPPORT
static gx_applets_favlist_t *fav_list = NULL;
#endif
#if APPLETS_RECENT_SUPPORT
static gx_applets_favlist_t *recent_list = NULL;
#endif
#if APPLETS_INSTALL_SUPPORT
static gx_applets_installlist_t *install_list = NULL;
static gx_applets_installlist_t *install_usb_list = NULL;
static SystemSettingOpt  *applets_setting_opt = NULL;
static SystemSettingItem *applets_setting_item = NULL;
static applets_setting_t *applets_setting = NULL;
static char *applets_install_type_array = NULL;
static char * flash_install_path = NULL;
static char * usb_install_path = NULL;
#endif
#if APPLETS_BUILTIN_SUPPORT||APPLETS_FAV_SUPPORT||APPLETS_RECENT_SUPPORT
static gx_applets_builtinlist_t * builtin_list = NULL;
#endif
static gx_applets_applist_t * install_app_list = NULL;
static applets_focusinfo_t applets_focus_info = {0};
static char *applets_gui_load_id = NULL;

static void _applets_ui_contral(struct AppletUICtrl *msg);
static applets_open_status_t app_applets_run(char * path,char *id);
static void _applets_group_item_init(void);
static void _update_focus_info_item_init(void);
static void app_applets_data_timer_stop(void);
static void applets_app_download_stop(void);
#if APPLETS_RECENT_SUPPORT
static void _applets_recent_node(void);
#endif
#if APPLETS_INSTALL_SUPPORT
static void _applets_update_installed(void);
static char *  _applets_usb_get_install_root_path(void);
static char *  _applets_flash_get_install_root_path(void);
static int applets_check_app_from_install(void);
static int _applets_check_disk_exist(void);
static int _applets_check_size_limit(void);
static void applets_setting_init(void);
static void applets_setting_get(void);
static void applets_free_install_list(void);
static void applets_free_install_usb_list(void);
#endif
#if APPLETS_FAV_SUPPORT
static void applets_free_fav_list(void);
#endif

extern char *gxapps_get_server_user_agent(void);
extern char *gxapps_get_app_version_str(void);
extern char *gxapps_get_customer_name_str(void);
extern char *gxapps_get_customer_model_str(void);

static char *s_text_manage_title[MAX_PAGE_ITEM] =
{
    "text_applets_item_title1",
    "text_applets_item_title2",
    "text_applets_item_title3",
    "text_applets_item_title4",
    "text_applets_item_title5",
    "text_applets_item_title6",
};

static char *s_img_manage_focus[MAX_PAGE_ITEM] =
{
    "img_applets_item_focus1",
    "img_applets_item_focus2",
    "img_applets_item_focus3",
    "img_applets_item_focus4",
    "img_applets_item_focus5",
    "img_applets_item_focus6",
};

static char *s_img_manage_unfocus[MAX_PAGE_ITEM] =
{
    "img_applets_item_unfocus1",
    "img_applets_item_unfocus2",
    "img_applets_item_unfocus3",
    "img_applets_item_unfocus4",
    "img_applets_item_unfocus5",
    "img_applets_item_unfocus6",
};

static char *s_text_manage_status[MAX_PAGE_ITEM] =
{
    "text_applets_item_status1",
    "text_applets_item_status2",
    "text_applets_item_status3",
    "text_applets_item_status4",
    "text_applets_item_status5",
    "text_applets_item_status6",
};

typedef enum gx_applets_category_e{
	APPLETS_CAGEGORY_CLOUD = 0,
#if APPLETS_BUILTIN_SUPPORT
	APPLETS_CAGEGORY_BUILTIN,
#endif
#if APPLETS_INSTALL_SUPPORT
	APPLETS_CAGEGORY_INSTALL,
#endif
#if APPLETS_FAV_SUPPORT
	APPLETS_CAGEGORY_FAV,
#endif
#if APPLETS_RECENT_SUPPORT
	APPLETS_CAGEGORY_RECENT,
#endif
	APPLETS_CATEGORY_MAX
}gx_applets_category_t;


static char* applets_category_str[APPLETS_CATEGORY_MAX]=
{
	"Online", //APPLETS_CAGEGORY_CLOUD
#if APPLETS_BUILTIN_SUPPORT
	"Builtin", //APPLETS_CAGEGORY_BUILTIN
#endif
#if APPLETS_INSTALL_SUPPORT
	"Installed", //APPLETS_CAGEGORY_INSTALL
#endif
#if APPLETS_FAV_SUPPORT
	"Favorites", //APPLETS_CAGEGORY_FAV
#endif
#if APPLETS_RECENT_SUPPORT
	"Recent", //APPLETS_CAGEGORY_RECENT
#endif

};
#define APPLETS_CATEGORY_NUM 	(sizeof(applets_category_str)/sizeof(char *))

#if APPLETS_INSTALL_SUPPORT
static char* applets_install_type_str[]=
{
	"Flash",
	"USB"
};
#define APPLETS_INSTALL_TYPE_NUM 	(sizeof(applets_install_type_str)/sizeof(char *))
#endif

int applets_get_ftype(const char *path)
{
    struct stat st;
    if(NULL == path)
    {
        APPLETS_INFO("path error!!!\n");
        return -1;
    }
    if (stat((char *)path, &st) != 0)
    {
        APPLETS_INFO("%s stat error!!!\n",path);
        return -1;
    }
    if (S_ISDIR(st.st_mode))
    {
        APPLETS_INFO("%s is dir!!!\n",path);
        return 0;
    }
    else if (S_ISREG(st.st_mode))
    {
        APPLETS_INFO("%s is reg!!!\n",path);
        return 1;
    }
    else
    {
        APPLETS_INFO("%s is unknown error!!!\n",path);
        return -1;
    }
}
int applets_dir_or_file_delete(const char *path, size_t max_depth)
{
    APPLETS_INFO("%s,%d,path=%s\n",__func__,__LINE__, path);
    return app_dir_or_file_delete(path,max_depth,NULL);
}
int _install_applist_check_id_exist(gx_applets_applist_t * list,char * id)
{
    int i = 0;
    if(NULL == list || NULL == id)
        return -1;
    for(i=0;i<list->app_num;i++)
    {
        if(0 == strcmp(list->app[i].id,id))
            return i;
    }
    return -1;
}
int _appletlist_check_id_exist(gx_appletlist_t *list,char *id)
{
	int i = 0;

	if(NULL == list || NULL == id)
		return -1;

	for(i=0;i<list->applet_num;i++)
	{
		if(0 == strcmp(list->applets[i].id,id))
			return i;
	}
	return -1;
}
char * app_applets_get_download_path(char * id,AppletsLoadMode download_mode)
{
    char * app_path = NULL;

    app_path = (char *)GxCore_Malloc(MAX_PATH_LEN*sizeof(char));
    if(NULL == app_path)
    {
        APPLETS_INFO("app_applets_get_download_path,malloc error\n");
        return NULL;
    }
    memset(app_path,0,MAX_PATH_LEN);
    if(DOWNLOAD_MODE_ONLINE == download_mode)
    {
        sprintf(app_path,"%s",TMP_INSTALL_PATH);
    }
#if APPLETS_INSTALL_SUPPORT
    else if(DOWNLOAD_MODE_INSTALL == download_mode)
    {
        char * install_path = NULL;
        if(INSTALL_PATH_FLASH == applets_setting->install_path)
        {
            install_path = _applets_flash_get_install_root_path();
        }
        else
        {
            install_path = _applets_usb_get_install_root_path();
        }
        if (NULL != install_path)
         {
             if(id)
                 sprintf(app_path,"%s/%s",install_path,id);
             else
                 sprintf(app_path,"%s",install_path);
         }
         else
         {
             APP_FREE(app_path);
             app_path = NULL;
         }
    }
#endif
#if APPLETS_BUILTIN_SUPPORT
    else if(DOWNLOAD_MODE_BUILTIN == download_mode)
    {
        if(id)
        {
            sprintf(app_path,"%s/%s",APPLETS_BUILTIN_PATH,id);
        }
        else
        {
            sprintf(app_path,"%s",APPLETS_BUILTIN_PATH);
        }
    }
#endif
    APPLETS_INFO("app_applets_get_download_path,path=%s\n",(app_path==NULL)?"null":app_path);

    return app_path;
}

void app_applets_set_info(char * install_path,char *id)
{
    char * id_str =NULL;
    char* focus_win = NULL;
    focus_win = (char*)GUI_GetFocusWindow();
    if(!focus_win)
        return ;
    id_str = id;
    if((NULL == id_str)&&(applets_focus_info.run_sel>=0))
    {
        id_str = applets_list->applets[applets_focus_info.run_sel].id;
    }
    if(id_str)
        app_netapps_config_set_string(APPLETS_CONFIG_ID, id_str);
    if(install_path)
        app_netapps_config_set_string(APPLETS_CONFIG_PATH, install_path);
    APPLETS_INFO("%s,%d,id_str=%s\n",__func__,__LINE__, id_str);
}

static int app_applets_video_busy(void)
{
	return is_applets_download;
}

static void applets_thread_body(void* arg)
{
	applets_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (applets_thread_func)arg;
	if(func)
	{
		func();
	}
}

void create_applets_thread(applets_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("applets", &thread_id, applets_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}
void _applets_free_infonode_list(applets_Infonodetlist_t* data)
{
    int i = 0;

    if(data)
    {
        if(data->applets)
        {
            if(data->applet_num>0)
            {
                for(i=0; i<data->applet_num; i++)
                {
                    if(data->applets[i].id)
                    {
                        free(data->applets[i].id);
                        data->applets[i].id = NULL;
                    }
                    if(data->applets[i].name)
                    {
                        free(data->applets[i].name);
                        data->applets[i].name = NULL;
                    }
                    if(data->applets[i].icon_url)
                    {
                        free(data->applets[i].icon_url);
                        data->applets[i].icon_url = NULL;
                    }
               }
            }
            free(data->applets);
            data->applets = NULL;
        }
        free(data);
        data = NULL;
    }
}

static void applets_free_cloud_list(void)
{
	if(applets_list)
	{
		gx_applets_free_appletlist(applets_list);
		applets_list = NULL;
	}
}



#if APPLETS_RECENT_SUPPORT
static void applets_free_recent_list(void)
{
	if(recent_list)
	{
		gx_applets_free_fav_list(recent_list);
		recent_list = NULL;
	}
}
#endif
#if !APPLETS_AUTO_UPDATE_LIST_SUPPORT
#if APPLETS_BUILTIN_SUPPORT
static void applets_free_builtin_list(void)
{
	if(builtin_list)
	{
		gx_applets_free_builtin_list(builtin_list);
		builtin_list = NULL;
	}
}
#endif
#endif
static void applets_free_infonode_list(void)
{
    if(applets_focus_info.node)
    {
       _applets_free_infonode_list(applets_focus_info.node);
       applets_focus_info.node = NULL;
    }
}

static void applets_free_install_app_list(void)
{
	if(install_app_list)
	{
        gx_applets_free_install_app_list(install_app_list);
		install_app_list = NULL;
	}
}


void applets_get_all_data(void)
{
#if APPLETS_INSTALL_SUPPORT
    char *app_path= NULL;
    app_path = app_applets_get_download_path(NULL,DOWNLOAD_MODE_INSTALL);
    if(applets_setting->install_path == INSTALL_PATH_FLASH)
    {
        applets_free_install_list();
        gx_applets_get_installlist(&install_list,app_path,applets_list);
    }
    else if(applets_setting->install_path == INSTALL_PATH_USB)
    {
        applets_free_install_usb_list();
        gx_applets_get_installlist(&install_usb_list,app_path,applets_list);
    }
    applets_check_app_from_install();
    APP_FREE(app_path);
#endif
#if APPLETS_FAV_SUPPORT
    applets_free_fav_list();
    gx_applets_get_favlist(&fav_list,applets_list,builtin_list);
#endif
#if APPLETS_RECENT_SUPPORT
    applets_free_recent_list();
    gx_applets_get_recentlist(&recent_list,applets_list,builtin_list);
#endif
    return;
}

int app_applets_get_data_by_id(char * id, int source)
{
    int i = 0;
    int num = 0;
#if APPLETS_BUILTIN_SUPPORT
    if(source)
    {
        num = builtin_list->builtin_num;
        for(i=0;i<num;i++)
        {
            if(0 == strcmp(id,builtin_list->builtin[i].id))
                return i;
        }
    }
    else
#endif
    {
        num = applets_list->applet_num;
        for(i=0;i<num;i++)
        {
            if(0 == strcmp(id,applets_list->applets[i].id))
                return i;
        }
    }
    return -1;
}

#if APPLETS_INSTALL_SUPPORT
static int applets_appletlist_check_status(char * id)
{
    int i = 0;
    gx_applets_installlist_t *list = NULL;

    if(INSTALL_PATH_FLASH == applets_setting->install_path)
    {
        list = install_list;
    }
    else
    {
        if(check_usb_status() == false)
        {
            list = NULL ;
        }
        else
            list = install_usb_list;
    }
    if(NULL == list|| NULL == id)
        return -1;
    for(i=0;i<list->install_num;i++)
    {
        if(0 == strcmp(list->install[i].id,id))
            return i;
    }
    return -1;
}
#else
static int applets_appletlist_check_status(char * id)
{
    return -1;
}
#endif

#if APPLETS_FAV_SUPPORT
int applets_appletlist_check_fav(char * id,int source)
{
    int i = 0;
    if(NULL == fav_list || NULL == id)
        return -1;
    for(i=0;i<fav_list->fav_num;i++)
    {
        if((0 == strcmp(fav_list->fav[i].id,id))&&(fav_list->fav[i].source == source))
            return i;
    }
    return -1;
}
#else
int applets_appletlist_check_fav(char * id,int source)
{
    return -1;
}
#endif
static void _group_get_item_data(int sel_group)
{
    int i = 0;
    int num = 0;
#if (APPLETS_FAV_SUPPORT||APPLETS_RECENT_SUPPORT||APPLETS_INSTALL_SUPPORT)
    int index = -1;
#endif

    applets_free_infonode_list();
    switch(sel_group)
    {
        case APPLETS_CAGEGORY_CLOUD:
            if(applets_list&&applets_list->applet_num>0)
            {
                num = applets_list->applet_num;
                applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                for(i=0;i<num;i++)
                {
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(applets_list->applets[i].id);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(applets_list->applets[i].name);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(applets_list->applets[i].icon_url);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = 0;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = applets_list->applets[i].encryption;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = i;
                    if(applets_appletlist_check_status(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 0;
                    if(applets_appletlist_check_fav(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].source) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 0;
                    APPLETS_INFO("%s,%d,id=%s\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id);
                    applets_focus_info.node->applet_num++;
                }
            }
             break;
#if APPLETS_BUILTIN_SUPPORT
        case APPLETS_CAGEGORY_BUILTIN:
            if(builtin_list&&builtin_list->builtin_num)
            {
                num = builtin_list->builtin_num;
                applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                for(i=0;i<num;i++)
                {
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(builtin_list->builtin[i].id);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(builtin_list->builtin[i].name);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(builtin_list->builtin[i].icon_url);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = 1;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = 0;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = i;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                    if(applets_appletlist_check_fav(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].source) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 0;
                    APPLETS_INFO("%s,%d,id=%s\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id);
                    applets_focus_info.node->applet_num++;
                }
            }
             break;
#endif
#if APPLETS_FAV_SUPPORT
        case APPLETS_CAGEGORY_FAV:
            if(fav_list&&fav_list->fav_num)
            {
                APPLETS_INFO("%s,%d,fav num = %d\n",__func__,__LINE__,fav_list->fav_num);
                num = fav_list->fav_num;
                applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                for(i=0;i<num;i++)
                {
                    index = app_applets_get_data_by_id(fav_list->fav[i].id,fav_list->fav[i].source);
                    if(index < 0)
                        continue;

                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = fav_list->fav[i].source;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(fav_list->fav[i].id);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = index;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                    if(fav_list->fav[i].source)
                    {
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(builtin_list->builtin[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(builtin_list->builtin[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = 0;

                    }
                    else
                    {
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(applets_list->applets[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(applets_list->applets[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = applets_list->applets[index].encryption;
                    }
                    if(applets_appletlist_check_status(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 0;
                    APPLETS_INFO("%s,%d,id=%s,status=%d\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].status);
                    applets_focus_info.node->applet_num++;
                }
            }
             break;
#endif
#if APPLETS_RECENT_SUPPORT
            case APPLETS_CAGEGORY_RECENT:
            if(recent_list&&recent_list->fav_num)
            {
                num = recent_list->fav_num;
                applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                for(i=0;i<num;i++)
                {
                    index = app_applets_get_data_by_id(recent_list->fav[i].id,recent_list->fav[i].source);
                    if(index < 0)
                        continue;

                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = recent_list->fav[i].source;
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(recent_list->fav[i].id);
                    applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = index;

                    if(recent_list->fav[i].source)
                    {
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(builtin_list->builtin[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(builtin_list->builtin[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = 0;

                    }
                    else
                    {
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(applets_list->applets[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(applets_list->applets[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = applets_list->applets[index].encryption;
                    }
                    if(applets_appletlist_check_status(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 0;
                    if(applets_appletlist_check_fav(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,recent_list->fav[i].source) >=0)
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                    else
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 0;
                    APPLETS_INFO("%s,%d,id=%s,source=%d\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].source);
                    applets_focus_info.node->applet_num++;
                }
            }
             break;
#endif
#if APPLETS_INSTALL_SUPPORT
        case APPLETS_CAGEGORY_INSTALL:
            if(INSTALL_PATH_FLASH == applets_setting->install_path)
            {
                if(install_list&&install_list->install_num)
                {
                    num = install_list->install_num;
                    if(!applets_focus_info.node)
                    {
                        applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                        memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                        applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                        memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                    }
                    for(i=0;i<num;i++)
                    {
                        index = app_applets_get_data_by_id(install_list->install[i].id,0);
                        if(index < 0)
                            continue;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = 0;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(install_list->install[i].id);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = index;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(applets_list->applets[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(applets_list->applets[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = install_list->install[i].encryption;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                        if(applets_appletlist_check_fav(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].source) >=0)
                            applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                        else
                            applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 0;
                        APPLETS_INFO("%s,%d,id=%s\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id);
                        applets_focus_info.node->applet_num++;
                    }
                }
            }
            else
            {
                if(install_usb_list&&install_usb_list->install_num)
                {
                    num = install_usb_list->install_num;
                    if(!applets_focus_info.node)
                    {
                        applets_focus_info.node = (applets_Infonodetlist_t *)malloc(sizeof(applets_Infonodetlist_t));
                        memset(applets_focus_info.node, 0, sizeof(applets_Infonodetlist_t));
                        applets_focus_info.node->applets = (applets_Infonode_t *)malloc(num * sizeof(applets_Infonode_t));
                        memset(applets_focus_info.node->applets, 0, num * sizeof(applets_Infonode_t));
                    }
                    for(i=0;i<num;i++)
                    {
                        index = app_applets_get_data_by_id(install_usb_list->install[i].id,0);
                        if(index < 0)
                            continue;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].source = 0;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].id = strdup(install_usb_list->install[i].id);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].index = index;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].name = strdup(applets_list->applets[index].name);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].icon_url = strdup(applets_list->applets[index].icon_url);
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].encryption = install_usb_list->install[i].encryption;
                        applets_focus_info.node->applets[applets_focus_info.node->applet_num].status = 1;
                        if(applets_appletlist_check_fav(applets_focus_info.node->applets[applets_focus_info.node->applet_num].id,applets_focus_info.node->applets[applets_focus_info.node->applet_num].source) >=0)
                            applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 1;
                        else
                            applets_focus_info.node->applets[applets_focus_info.node->applet_num].fav = 0;
                        APPLETS_INFO("%s,%d,id=%s\n",__func__,__LINE__,applets_focus_info.node->applets[applets_focus_info.node->applet_num].id);
                        applets_focus_info.node->applet_num++;
                    }
                }
            }
             break;
#endif
        default:
            break;
    }

}

static void app_applets_clean_menu_info(void)
{
	applets_free_infonode_list();
	applets_focus_info.focus_area = FOCUSAREA_LIST_GROUP;
	applets_focus_info.group_num = 0;
	applets_focus_info.sel_group = 0;
	applets_focus_info.sel_item = 0;
	applets_focus_info.item_num = 0;
	applets_focus_info.run_sel = -1;
	applets_focus_info.run_from_mainmenu = false;
}

static void applets_cloud_listfresh_stop(void)
{
	applets_clouddata_finish = 0;
}

static void applets_cloud_listfresh_start(void)
{
	applets_clouddata_finish = 1;
}
static void applets_app_update_stop(void)
{
	applets_app_update_finish = 0;
}

static void applets_app_update_start(void)
{
	applets_app_update_finish = 1;
}
static void applets_download_listfresh_stop(void)
{
	applets_download_app_finish = 0;
}

static void applets_download_listfresh_start(void)
{
	applets_download_app_finish = 1;
}

static void applets_get_list_feed(void)
{
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	gx_appletlist_t	*data = NULL;
	is_applets_download = 1;
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
    if(applets_get_list_status == APPLETS_GET_LIST_SUCCESS)
    {
        applets_ret_status = GXAPPS_SUCCESS;
		applets_cloud_listfresh_start();
        is_applets_download = 0;
        return;
    }
    applets_get_list_status = APPLETS_GET_LIST_RUNNING;
#endif
	gxapps_curl_set_timeout(30, 30);
	applets_data_handle++;
	handle = applets_data_handle;
#if APPLETS_BUILTIN_SUPPORT
    if((NULL == builtin_list)&&(GXCORE_FILE_EXIST == GxCore_FileExists(APPLETS_BUILTIN_LIST_PATH)))
    {
        gx_applets_get_builtinlist(APPLETS_BUILTIN_LIST_PATH,&builtin_list);
    }
#endif
	applets_free_cloud_list();
	ret = gx_applets_get_appletlist(&data);
    APPLETS_INFO("%s,%d,ret=%d,0x%x=0x%x\n",__func__,__LINE__, ret,handle,applets_data_handle);
	if(handle == applets_data_handle)
	{
		applets_ret_status = ret;
		if(ret == GXAPPS_SUCCESS)
		{
			applets_list = data;
            applets_get_list_status = APPLETS_GET_LIST_SUCCESS;
            APPLETS_INFO("%s,%d,applet_num=%d\n",__func__,__LINE__, applets_list->applet_num);
		}
        else
        {
            applets_get_list_status = APPLETS_GET_LIST_FAILED;
        }
		applets_cloud_listfresh_start();
	}
	else
	{
		if(ret == GXAPPS_SUCCESS)
		{
			gx_applets_free_appletlist(data);
		}
		applets_get_list_status = APPLETS_GET_LIST_FAILED;
	}
	gxapps_curl_set_timeout(0, 0);
	is_applets_download = 0;
}


static void _app_applets_hide_popup_msg(void)
{
    APP_TIMER_REMOVE(s_applets_popup_timer);
    pop_msg_on = false;
    GUI_SetProperty(TXT_POPUP, "state", "hide");
    GUI_SetProperty(IMG_POPUP, "state", "hide");
}
static void _app_applets_update_icon(char* status,int fav)
{
    if(applets_focus_info.focus_area == FOCUSAREA_LIST_GROUP)
     {
         GUI_SetProperty(TXT_GREEN, "state", "hide");
         GUI_SetProperty(IMG_GREEN, "state", "hide");
         GUI_SetProperty(TXT_YELLOW, "state", "hide");
         GUI_SetProperty(IMG_YELLOW, "state", "hide");
         GUI_SetProperty(TXT_OK, "state", "hide");
         GUI_SetProperty(IMG_OK, "state", "hide");
     }
     else
     {
         if(fav < 0 )
         {
             GUI_SetProperty(TXT_YELLOW, "state", "hide");
             GUI_SetProperty(IMG_YELLOW, "state", "hide");
         }
         else
         {
             GUI_SetProperty(TXT_YELLOW, "state", "show");
             GUI_SetProperty(IMG_YELLOW, "state", "show");
             if(1 == fav)
             {
                GUI_SetProperty(TXT_YELLOW, "string", "UnFav");
             }
             else
             {
                 GUI_SetProperty(TXT_YELLOW, "string", "Fav");
             }
         }
         if(NULL == status)
         {
             GUI_SetProperty(TXT_GREEN, "state", "hide");
             GUI_SetProperty(IMG_GREEN, "state", "hide");
         }
         else
         {
             GUI_SetProperty(TXT_RED, "state", "show");
             GUI_SetProperty(IMG_RED, "state", "show");
             GUI_SetProperty(TXT_GREEN, "state", "show");
             GUI_SetProperty(IMG_GREEN, "state", "show");
             GUI_SetProperty(TXT_GREEN, "string", status);
         }
         GUI_SetProperty(TXT_OK, "state", "show");
         GUI_SetProperty(IMG_OK, "state", "show");
     }
}


static void app_applets_hide_popup_msg(void)
{
    struct AppletUICtrl param = {0};
	char* focus_win = NULL;
	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return;
    if(GUI_CheckDialog(WND_MAINMENU_TIP) == GXCORE_SUCCESS)
        GUI_EndDialog(WND_MAINMENU_TIP);
    if(GUI_CheckDialog(WND_APPLETS) == GXCORE_SUCCESS)
    {
        param.type = MSG_HIDE_POPUP;
        _applets_ui_contral(&param);
    }
	if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
	{
        popdlg_destroy();
	}
	pop_msg_on = false;
   // GUI_SetInterface("flush", NULL);
}

static int _applets_popup_timer_timeout(void *userdata)
{
    APP_TIMER_REMOVE(s_applets_popup_timer);
    app_applets_hide_popup_msg();
    return 0;
}

static void _app_applets_show_popup_msg(char* pMsg, int time_out)
{
    pop_msg_on = true;
    GUI_SetProperty(TXT_POPUP, "string", pMsg);
    GUI_SetProperty(IMG_POPUP, "state", "show");
    GUI_SetProperty(TXT_POPUP, "state", "show");
    GUI_SetInterface("flush", NULL);

    if(time_out > 0)
    {
        APP_TIMER_ADD(s_applets_popup_timer, _applets_popup_timer_timeout, time_out, TIMER_REPEAT);
    }
}
static int _applets_pop_cb(PopDlgRet ret)
{
	if(1 == is_app_downloading)
	{
		applets_app_download_stop();
		app_applets_hide_popup_msg();
	}
    return 0;
}

static void app_applets_show_popup_msg(char* pMsg, int time_out)
{
	struct AppletUICtrl param = {0};
	char* focus_win = NULL;
	PopDlg pop;

	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return;

	if(0 == strcasecmp(focus_win, WND_APPLETS))
	{
        param.type = MSG_SHOW_POPUP;
        param.status = pMsg;
        param.item = time_out;
        _applets_ui_contral(&param);

	}
	else
	{
        if(NULL == pMsg )
            return;
        pop_msg_on = true;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_WAIT;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.exit_cb = _applets_pop_cb;
        pop.str = pMsg;
        if(time_out > 0)
            pop.timeout_sec = time_out/500;
        popdlg_create(&pop);
        return;

	}
}

static int app_applets_draw_gif(void* usrdata)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "draw_gif", &alu);
    return 0;
}
static void _applets_load_gif(void)
{
    int alu = GX_ALU_ROP_COPY_INVERT;

    GUI_SetProperty(IMG_GIF, "load_img", GIF_PATH);
    GUI_SetProperty(IMG_GIF, "init_gif_alu_mode", &alu);
}

static void _applets_free_gif(void)
{
    APP_TIMER_REMOVE(s_applets_gif_timer);
    GUI_SetProperty(IMG_GIF, "load_img", NULL);
}

static void app_applets_show_gif(void)
{
    GUI_SetProperty(IMG_GIF, "state", "show");
    APP_TIMER_ADD(s_applets_gif_timer, app_applets_draw_gif, 100, TIMER_REPEAT);
}

static void app_applets_hide_gif(void)
{
    if(s_applets_gif_timer)
        timer_stop(s_applets_gif_timer);

    GUI_SetProperty(IMG_GIF, "state", "hide");
}
static void _applets_ui_contral(struct AppletUICtrl *msg)
{
    char page_info[20] = {0};
    int page, max_page;
    if(msg == NULL)
        return;

    switch(msg->type)
    {
        case MSG_FOCUS_ITEM:
            {
                GUI_SetProperty(s_img_manage_focus[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#E78818,#E78818,#E78818]");
                GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#ee3300,#ee3300,#ee3300]");
                GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                GUI_SetProperty(s_img_manage_focus[(msg->item)%MAX_PAGE_ITEM], "backcolor", ITEM_FOCUS_COLOR);
                GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "update", NULL);
               // GUI_SetProperty(WND_SHORT_DESC, "string", this_focus_info.source.node->shortdesc);
                break;
            }
        case MSG_UNFOCUS_ITEM:
            {
                GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[text_color,text_color,text_color]");
                GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "forecolor", "[#005577,#005577,#005577]");
                GUI_SetProperty(s_img_manage_focus[msg->item%MAX_PAGE_ITEM], "state", "hide");
                GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "update", NULL);
                break;
            }
        case MSG_UNFOCUS_ALL_ITEM:
            {
                int32_t i;

                for(i = 0; i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_text_manage_title[i], "forecolor", "[text_color,text_color,text_color]");
                    GUI_SetProperty(s_text_manage_status[i], "forecolor", "[#005577,#005577,#005577]");
                    GUI_SetProperty(s_img_manage_focus[i], "state", "hide");
                    GUI_SetProperty(s_img_manage_unfocus[i], "update", NULL);
                }
                break;
            }
        case MSG_CLEAN_ALL_ITEM:
            {
                int32_t i;
                for(i = 0;i < MAX_PAGE_ITEM; i++)
                {
                    GUI_SetProperty(s_img_manage_unfocus[i], "state", "hide");
                    GUI_SetProperty(s_text_manage_title[i], "state", "hide");
                    GUI_SetProperty(s_text_manage_status[i], "state", "hide");
                }
                break;
            }
        case MSG_UPDATE_ITEM:
            {
                if(msg->node.name != NULL)
                {
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "string", msg->node.name);
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                    if(NULL == msg->status)
                    {
                        GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                    }
                    else
                    {
                        GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "string", msg->status);
                        GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "state", "show");
                    }
                }
                else
                {
                    GUI_SetProperty(s_img_manage_focus[msg->item%MAX_PAGE_ITEM], "state", "hide");
                    GUI_SetProperty(s_img_manage_unfocus[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                    GUI_SetProperty(s_text_manage_title[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                    GUI_SetProperty(s_text_manage_status[(msg->item)%MAX_PAGE_ITEM], "state", "hide");
                }
                break;
            }
        case MSG_UPDATE_ITEM_IMAGE:
            {
                if(msg->image_path != NULL)
                {
                    GUI_SetProperty(s_img_manage_unfocus[msg->item], "load_scal_img",msg->image_path);
                    GUI_SetProperty(s_img_manage_unfocus[msg->item], "state", "show");
                    if(pop_msg_on)
                    {
                        GUI_SetProperty(TXT_POPUP, "update", NULL);
                        GUI_SetProperty(IMG_POPUP, "update", NULL);
                    }
                    APP_FREE(msg->image_path);
                    if(!strcasecmp((char*)GUI_GetFocusWindow(),WND_POP_BOOK))
                    {
                        GUI_SetProperty(WND_POP_BOOK, "update", NULL);
                    }

                    if((is_app_downloading)||(pic_download_ok[1] == 1) )
                    {
                        app_applets_hide_popup_msg();
                    }
                }
                break;
            }
        case MSG_GROUP_LIST_FOCUS:
            {
                GUI_SetFocusWidget(WND_LIST_GROUP);
                GUI_SetProperty(WND_LIST_GROUP, "select", &applets_focus_info.sel_group);
                GUI_SetProperty(WND_SHORT_DESC, "string", NULL);
                break;
            }
        case MSG_GROUP_LIST_ACTIVE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "active", &applets_focus_info.sel_group);
                break;
            }
        case MSG_UPDATE_PAGE_INFO:
            {
                if(msg->total == 0)
                {
                    GUI_SetProperty(WND_APPLETS_PAGE_INFO, "state", "hide");
                }
                else
                {
                    page = 1 + (msg->item - (msg->item % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                    max_page = 1 + ((msg->total-1) - ((msg->total-1) % MAX_PAGE_ITEM)) / MAX_PAGE_ITEM;
                    snprintf(page_info, sizeof(page_info), "%d/%d", page, max_page);
                    GUI_SetProperty(WND_APPLETS_PAGE_INFO, "string", page_info);
                    GUI_SetProperty(WND_APPLETS_PAGE_INFO, "state", "show");
                }
                break;
            }
        case MSG_GROUP_LIST_UPDATE:
            {
                GUI_SetProperty(WND_LIST_GROUP, "update_all", NULL);
                break;
            }
        case MSG_SHOW_GIF:
            {
                app_applets_show_gif();
                break;
            }
        case MSG_HIDE_GIF:
            {
                app_applets_hide_gif();
                break;
            }
        case MSG_SHOW_POPUP:
            {
                _app_applets_show_popup_msg(msg->status, msg->item);
                break;
            }
        case MSG_HIDE_POPUP:
            {
                _app_applets_hide_popup_msg();
                break;
            }
        case MSG_UPDATE_ICON:
            {
                _app_applets_update_icon(msg->status,msg->node.fav);
                break;
            }
        default:
            break;
    }
}

static void _applets_focus_item(int index)
{
    struct AppletUICtrl param = {0};

    if(index >= 0)
    {
        param.type = MSG_FOCUS_ITEM;
        param.item = index;
       _applets_ui_contral(&param);
    }
}

static void _applets_unfocus_all_item(void)
{
    struct AppletUICtrl param = {0};

    param.type = MSG_UNFOCUS_ALL_ITEM;
    _applets_ui_contral(&param);

    return;
}
#if 0
static void _applets_show_state(void)
{
    struct AppletUICtrl param = {0};

    param.type = MSG_SHOW_GIF;
    _applets_ui_contral(&param);
}

static void _applets_hide_state(void)
{
    struct AppletUICtrl param = {0};

    param.type = MSG_HIDE_GIF;
    _applets_ui_contral(&param);
}
#endif

static void _applets_unfocus_item(int index)
{
	struct AppletUICtrl param = {0};

	if(index >= 0)
	{
		param.type = MSG_UNFOCUS_ITEM;
		param.item = index;
		_applets_ui_contral(&param);
	}
}

static void _applets_group_list_update(void)
{
	struct AppletUICtrl param = {0};

	param.type = MSG_GROUP_LIST_UPDATE;
	_applets_ui_contral(&param);

	return;
}

static void _applets_group_list_focus(void)
{
	struct AppletUICtrl param = {0};

	param.type = MSG_GROUP_LIST_FOCUS;
	_applets_ui_contral(&param);

	return;
}
static void _applets_group_list_active(void)
{
	struct AppletUICtrl param = {0};

	param.type = MSG_GROUP_LIST_ACTIVE;
	_applets_ui_contral(&param);

	return;
}

static void _applets_clean_all_item(void)
{
	struct AppletUICtrl param = {0};

	param.type = MSG_CLEAN_ALL_ITEM;
	_applets_ui_contral(&param);

	return;
}
static void _applets_update_icon(void)
{
	struct AppletUICtrl param = {0};
	char* focus_win = NULL;

	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return;
	if(strcasecmp(focus_win, WND_APPLETS))
		return;
	if(applets_get_list_status != APPLETS_GET_LIST_SUCCESS)
		return;
	if(NULL == applets_focus_info.node)
		return;
	if(NULL == applets_focus_info.node->applets)
		return;
	memset(&param, 0, sizeof(struct AppletUICtrl));
	param.type = MSG_UPDATE_ICON;
	param.item = applets_focus_info.sel_item;
#if APPLETS_FAV_SUPPORT
	param.node.fav = applets_focus_info.node->applets[param.item ].fav;
#else
	param.node.fav = -1;
#endif
#if APPLETS_INSTALL_SUPPORT
	if(0 == applets_focus_info.node->applets[param.item].source)
	{
		if(applets_focus_info.node->applets[param.item].status)
			param.status = STR_ID_UNINSTALL;
		else
			param.status = STR_ID_INSTALL;
	}
#endif
#if APPLETS_BUILTIN_SUPPORT
	if(applets_focus_info.node->applets[param.item].source)
	{
		param.status = NULL;
	}
#endif
	_applets_ui_contral(&param);
}

static int _applets_one_item_draw(void)
{
	struct AppletUICtrl param = {0};
	int sel = 0;
	char* focus_win = NULL;

	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return -1;
	if(strcasecmp(focus_win, WND_APPLETS))
		return -1;

	sel = applets_focus_info.sel_item;
	memset(&param, 0, sizeof(struct AppletUICtrl));
	param.type = MSG_UPDATE_ITEM;
	param.item = sel;
	param.node.name = applets_focus_info.node->applets[sel].name;
#if APPLETS_BUILTIN_SUPPORT
	if(applets_focus_info.node->applets[sel].source)
	{
		param.status = STR_ID_BUILTIN;
	}
#endif
#if APPLETS_INSTALL_SUPPORT
	if(0 == applets_focus_info.node->applets[sel].source)
	{
		if(applets_focus_info.node->applets[sel].status)
			param.status = STR_ID_INSTALLED;
		else
			param.status = STR_ID_UNINSTALL;
	}
#endif
	_applets_ui_contral(&param);

	return 0;
}



static void _applets_update_page_info(void)
{
	struct AppletUICtrl param = {0};

	param.type = MSG_UPDATE_PAGE_INFO;
	param.item = applets_focus_info.sel_item;
	param.total = applets_focus_info.item_num;
	_applets_ui_contral(&param);
}

static void app_applets_show_error_msg(void)
{
	if(applets_ret_status==GXAPPS_SERVER_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_NO_RESULTS, 1000);
	}
	else if(applets_ret_status==GXAPPS_NETWORK_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_SERVER_FAIL, 1000);
	}
	else if(applets_ret_status==GXAPPS_INTERNAL_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_INTER_ERROR, 1000);
	}
	else if(applets_ret_status==GXAPPS_UNKNOWN_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_UNKNOWN_ERROR, 1000);
	}
	else if(applets_ret_status==GXAPPS_AUTH_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_AUTH_ERROR, 1000);
	}
}

static void app_applets_show_open_error_msg(void)
{
	if(applets_open_status==APPLETS_STATUS_CRC_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_CRC_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_FILE_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_FILE_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_MKDIR_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_MKDIR_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_UNZIP_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_UNZIP_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_OBJ_CLOSE_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_CLOSE_TIP, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_OBJ_OPEN_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_OBJ_OPEN_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_DECRYPT_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_DECRYPT_ERROR, 1000);
	}
	else if(applets_open_status==APPLETS_STATUS_UNKNOWN_ERROR)
	{
		app_applets_show_popup_msg(STR_ID_APPLETS_RUN_ERROR, 1000);
	}
}

static int applets_clouddata_timer_timeout(void *usrdata)
{
	applets_cloud_listfresh_stop();
	_applets_group_item_init();
	applets_get_all_data();
	_applets_unfocus_all_item();
	_group_get_item_data(applets_focus_info.sel_group);
	_update_focus_info_item_init();
	app_applets_hide_popup_msg();
	applets_focus_info.focus_area = FOCUSAREA_LIST_GROUP;
	_applets_group_list_focus();
	_applets_group_list_update();
	_applets_group_list_active();
	app_applets_show_error_msg();
	return 0;
}
static int applets_download_app_timer_timeout(void *usrdata)
{
	applets_download_listfresh_stop();
	app_applets_hide_popup_msg();
	_applets_update_icon();
	_applets_one_item_draw();
	app_applets_show_error_msg();
	if(GXAPPS_SUCCESS == applets_ret_status)
	{
		if(DOWNLOAD_MODE_ONLINE == applets_focus_info.download_mode)
		{
			if(applets_focus_info.run_from_mainmenu)
			{
				app_applets_data_timer_stop();
			}
			if(APPLETS_STATUS_SUCCESS == applets_open_status)
			{
				app_applets_run(TMP_INSTALL_PATH,NULL);
			}
			else
			{
				app_applets_show_open_error_msg();
			}
		}
		else
		{
			app_applets_show_open_error_msg();
		}
	}
	return 0;
}

static int applets_data_timer_timeout(void *usrdata)
{
	if(applets_clouddata_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		applets_clouddata_timer_timeout(NULL);
	}
	if(applets_download_app_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		applets_download_app_timer_timeout(NULL);
	}
#if APPLETS_INSTALL_SUPPORT
	if(applets_app_update_finish && (GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS))
	{
		_applets_update_installed();
	}
#endif
	return 0;
}

static void app_applets_data_timer_stop(void)
{
	if(s_applets_data_update_timer)
	{
		remove_timer(s_applets_data_update_timer);
		s_applets_data_update_timer = NULL;
	}
}

static void applets_data_timer_start(void)
{
	app_applets_data_timer_stop();
	s_applets_data_update_timer = create_timer(applets_data_timer_timeout, 10, NULL, TIMER_REPEAT);
}

static void applets_cloud_list_update(void)
{
	applets_cloud_listfresh_stop();
	if(app_applets_video_busy()||(applets_get_list_status == APPLETS_GET_LIST_RUNNING))
	{
		app_applets_show_popup_msg(STR_ID_SYSTEM_BUSY_WAIT, 1000);
	}
	else
	{
		app_applets_show_popup_msg(STR_ID_UPDATING, 0);
		create_applets_thread(applets_get_list_feed);
	}
}
static void _applets_left_exit(void)
{
	_applets_unfocus_item(applets_focus_info.sel_item);
	applets_focus_info.focus_area = FOCUSAREA_LIST_GROUP;
	_applets_group_list_focus();
	_applets_update_icon();
	applets_app_update_stop();
}

static int app_applets_pic_update_timer_timeout(void *userdata)
{
	int i=0;
	bool b_all_ok = true;
	struct AppletUICtrl param = {0};

	for(i = 0; i < MAX_PAGE_ITEM; i++)
	{
		if(pic_show_ok[i] == 1)
		{
			continue;
		}
		b_all_ok = false;
		if(pic_download_ok[i] == 1)
		{
			if(GXCORE_FILE_UNEXIST != GxCore_FileExists(this_pic_path[i]))
			{
				memset(&param, 0, sizeof(struct AppletUICtrl));
				param.type = MSG_UPDATE_ITEM_IMAGE;
				param.item = i;
				param.image_path = GxCore_Strdup(this_pic_path[i]);
				_applets_ui_contral(&param);
			}
			pic_show_ok[i] = 1;
		}
		else if(pic_download_ok[i] == -1)
		{
			pic_show_ok[i] = 1;
		}
	}

	if(b_all_ok)
	{
		GUI_SetInterface("flush",NULL);
		timer_stop(s_applets_pic_update_timer);
		if(download_pic_handle>0)
		{
			gxapps_curl_async_abort(download_pic_handle);
			download_pic_handle = 0;
		}
	}
	return 0;
}

static void app_applets_pic_update_timer_rm(void)
{
	if(s_applets_pic_update_timer)
	{
		remove_timer(s_applets_pic_update_timer);
		s_applets_pic_update_timer = NULL;
	}
}

static void app_applets_pic_update_timer_stop(void)
{
	if(s_applets_pic_update_timer)
	{
		timer_stop(s_applets_pic_update_timer);
	}
}

static void app_applets_pic_update_timer_reset(void)
{
	if (reset_timer(s_applets_pic_update_timer) != 0)
	{
		s_applets_pic_update_timer = create_timer(app_applets_pic_update_timer_timeout, 300, NULL, TIMER_REPEAT);
	}
}

static void _pic_download_cb(int message, int body)
{
	gxcurl_callback_data_t *cb_data = (gxcurl_callback_data_t *)body;
	int index = 0;

	if(pic_download_abort)
		return;

	if(message == GXCURL_STATE_COMPLETE)
	{
		if((cb_data->handle > 0)&&(cb_data->complete_data.size > 0)&&(cb_data->handle == download_pic_handle)
			&&(cb_data->index>=0)&&(cb_data->index<pic_download_num))
		{
			index = pic_download_index[cb_data->index];
			if((index>=0)&&(index<MAX_PAGE_ITEM))
			{
				if(cb_data->complete_data.retcode == GXAPPS_SUCCESS)
				{
					pic_download_ok[index] = 1;
					APPLETS_INFO("*****%d****ok*****\n", index);

				}
				else
				{
					pic_download_ok[index] = -1;
					APPLETS_INFO("*****%d****error*****\n", index);
				}
			}
		}
	}
}

static void applets_logo_download_delete(void)
{
	int i=0;
	char image_path[MAX_PATH_LEN];

	for(i=0; i<MAX_PAGE_ITEM; i++)
	{
		snprintf(image_path, sizeof(image_path), "%s%d%s", IMAGE_DOWN_PATH, i, PIC_PATH_SUFFIX);
		if(GXCORE_FILE_EXIST == GxCore_FileExists(image_path))
		{
			GxCore_FileDelete(image_path);
		}
	}
}

static void applets_pic_download_stop(void)
{
	int i=0;

	app_applets_pic_update_timer_rm();
	memset(&pic_download_ok, 0, sizeof(pic_download_ok));
	memset(&pic_show_ok, 0, sizeof(pic_show_ok));
	pic_download_abort= 1;

	if(download_pic_handle>0)
	{
		gxapps_curl_async_abort(download_pic_handle);
		download_pic_handle = 0;
	}
	for(i=0; i<MAX_PAGE_ITEM; i++)
	{
		APP_FREE(this_pic_path[i]);
	}
	applets_logo_download_delete();
	memset(pic_download_index, 0, sizeof(pic_download_index));
	pic_download_num = 0;
	pic_download_abort= 0;
}

static char *applets_get_user_agent(void)
{
	char *user_agent = NULL;
	char *base_user_agent = NULL;
	char *version = NULL;
	char *customer = NULL;
	char *model = NULL;

	base_user_agent = gxapps_get_server_user_agent();
	version = gxapps_get_app_version_str();
	customer = gxapps_get_customer_name_str();
	model = gxapps_get_customer_model_str();
	user_agent = (char *)malloc(strlen(base_user_agent)+strlen(version)+strlen(customer)+strlen(model)+4);
	sprintf(user_agent, "%s_%s_%s_%s", base_user_agent, version, customer, model);

	return user_agent;
}

static applets_download_data_s *applets_malloc_download_data(void)
{
	applets_download_data_s *download_data = NULL;

	download_data = (applets_download_data_s *)GxCore_Calloc(1, sizeof(applets_download_data_s));
	download_data->download_urls = GxCore_Calloc(MAX_PAGE_ITEM, sizeof(char *));
	download_data->download_paths = GxCore_Calloc(MAX_PAGE_ITEM, sizeof(char *));

	return download_data;
}

static void applets_free_download_data(applets_download_data_s *data)
{
	int i = 0;

	if(data)
	{
		if(data->download_urls)
		{
			for(i=0; i<data->download_num; i++)
			{
				APP_FREE(data->download_urls[i]);
			}
			GxCore_Free(data->download_urls);
			data->download_urls = NULL;
		}
		if(data->download_paths)
		{
			for(i=0; i<data->download_num; i++)
			{
				APP_FREE(data->download_paths[i]);
			}
			GxCore_Free(data->download_paths);
			data->download_paths = NULL;
		}
		GxCore_Free(data);
	}
}

static void applets_add_download_data(applets_download_data_s *data, char *url, int index)
{
	char path[MAX_PATH_LEN];

	if(data&&(data->download_num<MAX_PAGE_ITEM)&&url&&(index>=0)&&(index<MAX_PAGE_ITEM)&&pic_download_num<MAX_PAGE_ITEM)
	{
		snprintf(path, sizeof(path)-1, "%s%d%s", IMAGE_DOWN_PATH, index, PIC_PATH_SUFFIX);
		APP_FREE(this_pic_path[index]);
		this_pic_path[index] = GxCore_Strdup(path);
		data->download_urls[data->download_num] = GxCore_Strdup(url);
		data->download_paths[data->download_num] = GxCore_Strdup(path);
		data->download_num++;
		pic_download_index[pic_download_num] = index;
		pic_download_num++;
	}
}

static void applets_logo_download_start(void)
{
	char *url = NULL;
	int i =0;
	struct AppletUICtrl param = {0};
	int sel = 0;
	applets_download_data_s *download_data = NULL;
	char *user_agent_str = NULL;
	gx_curl_http_options_t http_options;

	download_data =applets_malloc_download_data();
	applets_pic_download_stop();
	app_applets_pic_update_timer_reset();
	sel = (applets_focus_info.sel_item - applets_focus_info.sel_item % MAX_PAGE_ITEM);
	for(i=0; i < MAX_PAGE_ITEM; i++)
	{
		if(sel >= applets_focus_info.item_num)
			break;
		param.type = MSG_UPDATE_ITEM_IMAGE;
		param.item = i;
		url = applets_focus_info.node->applets[sel].icon_url;
		if(strncmp(url, "http", 4) == 0)
		{
			param.image_path = GxCore_Strdup(IMG_DEFAULT);
			applets_add_download_data(download_data, url, i);
		}
		else if(strcmp(url, WORK_PATH) == 0)
		{
			param.image_path = GxCore_Strdup(url);
		}
		else
		{
			param.image_path = GxCore_Mallocz(strlen(APPLETS_BUILTIN_PATH)+strlen(url)+1);
			sprintf(param.image_path,"%s%s",APPLETS_BUILTIN_PATH,url);
		}
		_applets_ui_contral(&param);
		sel++;
	}

	if(download_data&&(download_data->download_num>0))
	{
		memset(&http_options, 0, sizeof(gx_curl_http_options_t));
		user_agent_str = applets_get_user_agent();
		http_options.user_agent = user_agent_str;
		download_pic_handle = gxapps_curl_multi_async_download((const char **)download_data->download_urls, download_data->download_num, NULL, (const char **)download_data->download_paths, _pic_download_cb);
		APP_FREE(user_agent_str);
	}
	applets_free_download_data(download_data);
}

static int _applets_info_draw(void)
{
    int i = 0;
    struct AppletUICtrl param = {0};
    int sel = 0;
    sel = (applets_focus_info.sel_item - applets_focus_info.sel_item % MAX_PAGE_ITEM);
    for(i=0; i < MAX_PAGE_ITEM; i++)
    {
        memset(&param, 0, sizeof(struct AppletUICtrl));
        param.type = MSG_UPDATE_ITEM;
        param.item = sel;
        if(sel >= applets_focus_info.item_num)
        {
            param.node.name = NULL;
        }
        else
        {
            param.node.name = applets_focus_info.node->applets[sel].name;
#if APPLETS_INSTALL_SUPPORT
            if(0 == applets_focus_info.node->applets[sel].source)
            {
                if(applets_focus_info.node->applets[sel].status)
                    param.status = STR_ID_INSTALLED;
                else
                    param.status = STR_ID_UNINSTALL;
            }
#endif
#if APPLETS_BUILTIN_SUPPORT
            if(applets_focus_info.node->applets[sel].source)
            {
                param.status = STR_ID_BUILTIN;
            }
#endif
        }
        _applets_ui_contral(&param);
        sel++;
    }
    _applets_update_page_info();
    return 0;
}

static void _applets_group_item_init(void)
{
    applets_focus_info.group_num = APPLETS_CATEGORY_NUM;
    applets_focus_info.sel_group = APPLETS_CAGEGORY_CLOUD;
}

static void _update_focus_info_item_init(void)
{
    applets_focus_info.sel_item = 0;
    applets_focus_info.item_num = 0;
    if(applets_focus_info.node && applets_focus_info.node->applet_num >0)
    {
        applets_focus_info.item_num = applets_focus_info.node->applet_num;
        applets_focus_info.run_sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
        _applets_info_draw();
        applets_logo_download_start();
    }
    else
    {
        applets_pic_download_stop();
        _applets_update_page_info();
    }
}

static void _applets_up_keypress(int key)
{
	int i, focus_item,total_number;

	total_number = applets_focus_info.item_num;
	focus_item = i = applets_focus_info.sel_item;
	applets_app_update_stop();

	if(i == 0)
		i = total_number - 1;
	else if(key == STBK_UP)
		i -= MAX_PAGE_ITEM / MAX_PAGE_LINE;
	else
		i -= MAX_PAGE_ITEM;

	if(i < 0)
		i = 0;

	applets_focus_info.sel_item = i;

	if(focus_item/MAX_PAGE_ITEM != i/MAX_PAGE_ITEM)
	{
		app_applets_show_popup_msg(STR_ID_WAITING, 0);
		_applets_info_draw();
		applets_logo_download_start();
		app_applets_hide_popup_msg();
	}

	APPLETS_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
	applets_focus_info.run_sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
	_applets_unfocus_item(focus_item);
	_applets_focus_item(i);
	_applets_update_icon();
	applets_app_update_start();
}

static void _applets_down_keypress(int key)
{
	int i, total_number, focus_item;

	total_number = applets_focus_info.item_num;
	focus_item = i = applets_focus_info.sel_item;
	applets_app_update_stop();

	if(i == total_number - 1)
		i = 0;
	else if(key == STBK_DOWN)
		i += MAX_PAGE_ITEM / MAX_PAGE_LINE;
	else
		i += MAX_PAGE_ITEM;

	if(i >= total_number)
		i = total_number - 1;

	applets_focus_info.sel_item = i;

	if(focus_item/MAX_PAGE_ITEM != i/MAX_PAGE_ITEM)
	{
		app_applets_show_popup_msg(STR_ID_WAITING, 0);
		_applets_info_draw();
		applets_logo_download_start();
		app_applets_hide_popup_msg();
	}

	APPLETS_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
	applets_focus_info.run_sel =applets_focus_info.node->applets[applets_focus_info.sel_item].index;
	_applets_unfocus_item(focus_item);
	_applets_focus_item(i);
	_applets_update_icon();
	applets_app_update_start();
}

static void _applets_left_keypress(void)
{
    int i = 0;
    int focus_item = 0;

    focus_item = i = applets_focus_info.sel_item;
    applets_app_update_stop();
    if(0 == i%(MAX_PAGE_ITEM/MAX_PAGE_LINE))
    {
        _applets_left_exit();
        return;
    }
    else
    {
        i -= 1;
        applets_focus_info.sel_item = i;
    }
    APPLETS_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    applets_focus_info.run_sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
    _applets_unfocus_item(focus_item);
    _applets_focus_item(i);
    _applets_update_icon();
    applets_app_update_start();
}

static void _applets_right_keypress(void)
{
    int i = 0;
    int max_page, page, total_number, focus_item;

    total_number = applets_focus_info.item_num;
    focus_item = i = applets_focus_info.sel_item;
    applets_app_update_stop();
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
            applets_focus_info.sel_item = i;
            _applets_unfocus_item(focus_item);
            _applets_info_draw();
            applets_logo_download_start();
        }
    }
    else
    {
        i += 1;
        if(i > total_number-1)
            return;
        applets_focus_info.sel_item = i;
        _applets_unfocus_item(focus_item);
    }
    APPLETS_INFO("%s,%d,i=%d,focus_item=%d\n",__func__,__LINE__,i,focus_item);
    applets_focus_info.run_sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
    _applets_focus_item(i);
    _applets_update_icon();
    applets_app_update_start();
}
int app_applets_gui_unload(void)
{
	status_t ret = GXCORE_ERROR;
	if(NULL == applets_gui_load_id)
		return ret;
	APPLETS_INFO("%s,%d,id=%s\n",__func__,__LINE__, applets_gui_load_id);
	ret = GUI_Unload(applets_gui_load_id);
	if(GXCORE_SUCCESS == ret)
	{
		APPLETS_INFO("%s,%d,unload %s gui successfully\n",__func__,__LINE__, applets_gui_load_id);
		APP_FREE(applets_gui_load_id);
	}
	else
	{
		APPLETS_INFO("%s,%d,unload %s gui error\n",__func__,__LINE__, applets_gui_load_id);
		APP_FREE(applets_gui_load_id);
	}
	return ret ;
}

int app_applets_gui_load(char *id,char *installpath)
{
	status_t ret  = GXCORE_ERROR;
	char path[MAX_PATH_LEN];
	if((NULL == id)||(NULL == installpath))
		return ret;
	if(NULL != applets_gui_load_id)
	{
		return ret;
	}
	sprintf(path,"%s/theme.xml",installpath);
	if (GxCore_FileExists(path) != GXCORE_FILE_EXIST)
	{
		return ret;
	}
	APPLETS_INFO("%s,%d,path=%s\n",__func__,__LINE__, path);
	ret = GUI_Load(id,path);
	if(GXCORE_SUCCESS == ret)
	{
		applets_gui_load_id = strdup(id);
		APPLETS_INFO("%s,%d,load %s gui successfully\n",__func__,__LINE__, applets_gui_load_id);
	}
	else
	{
		APPLETS_INFO("%s,%d,load %s gui error\n",__func__,__LINE__, id);
	}
	return ret;
}
applets_open_status_t app_applet_get_app(char *buffer,int used_size,int sel,char *path)
{
    FILE *fp_des = NULL;
    char * data =NULL;
	applets_open_status_t ret = APPLETS_STATUS_UNKNOWN_ERROR;
    if(0 == gx_applets_checksum((unsigned char *)buffer,used_size,applets_list->applets[sel].md5))
    {
        if(applets_list->applets[sel].encryption)
        {
            data = gx_applets_decrypt(buffer,used_size);
        }
        else
        {
            data = buffer;
        }
        if(data)
        {
            fp_des = fopen(TMP_ZIP_PATH, "w+");
            if (fp_des != NULL)
            {
                APPLETS_INFO("start to write file %s size =%d \n", path, used_size);
                fwrite(data, used_size, 1, fp_des);
                fclose(fp_des);
                if(mkdir(path, 0777)>=0)
                {
                    if(gx_applets_unzip_app(TMP_ZIP_PATH,path) == 0)
                    {
                        APPLETS_INFO("unzip ok!!!!\n");
                        ret = APPLETS_STATUS_SUCCESS;
                    }
                    else
                    {
                        ret = APPLETS_STATUS_UNZIP_ERROR;
                        applets_dir_or_file_delete(path, 0);
                    }
                }
                else
                {
                    ret = APPLETS_STATUS_MKDIR_ERROR;
                }
                if (GxCore_FileExists(TMP_ZIP_PATH) == GXCORE_FILE_EXIST)
                {
                    GxCore_FileDelete(TMP_ZIP_PATH);
                }
            }
            else
            {
                ret = APPLETS_STATUS_FILE_ERROR;
            }
        }
        else
        {
            ret = APPLETS_STATUS_DECRYPT_ERROR;
        }
    }
    else
    {
        ret = APPLETS_STATUS_CRC_ERROR;
    }
    if(applets_list->applets[sel].encryption)
        APP_FREE(data);
    applets_open_status = ret;
    return ret;
}
static void applets_app_download_stop(void)
{
	applets_download_abort= 1;
    if(applets_download_handle!=0)
    {
        gxapps_curl_async_abort(applets_download_handle);
        applets_download_handle = 0;
        gxapps_curl_set_timeout(0, 0);
    }
    is_app_downloading = 0;
	applets_download_abort= 0;
}

static void _applets_download_cb(int message, int body)
{
    gxcurl_callback_data_t *callback_data = (gxcurl_callback_data_t *)body;
    char * app_path = NULL;
    APPLETS_INFO("%s,%d,message=%d\n",__func__,__LINE__,message);
    if(message == GXCURL_STATE_COMPLETE)
    {
        if(applets_download_abort)
        {
            return;
        }
        if(callback_data->handle > 0)
        {
            if(applets_download_handle == callback_data->handle)
            {
                applets_ret_status = callback_data->complete_data.retcode;
                if((callback_data->complete_data.size > 0) &&(applets_ret_status == GXAPPS_SUCCESS))
                {
                    app_path = app_applets_get_download_path(applets_list->applets[applets_focus_info.run_sel].id,applets_focus_info.download_mode);
                    if(NULL != app_path)
                    {
                        applets_dir_or_file_delete(app_path, 0);
                        if(APPLETS_STATUS_SUCCESS == app_applet_get_app(callback_data->complete_data.buffer,callback_data->complete_data.size,applets_focus_info.run_sel,app_path))
                        {
#if APPLETS_INSTALL_SUPPORT
                            if(DOWNLOAD_MODE_INSTALL == applets_focus_info.download_mode)
                            {
                                char *root_path = NULL;
                                root_path = app_applets_get_download_path(NULL,applets_focus_info.download_mode);
                                if(root_path)
                                {
                                    if(INSTALL_PATH_FLASH == applets_setting->install_path)
                                    {
                                        if(0 == gx_applets_modify_installlist(&install_list,applets_list->applets[applets_focus_info.run_sel].id,applets_list->applets[applets_focus_info.run_sel].encryption,1))
                                        {
                                            applets_focus_info.node->applets[applets_focus_info.sel_item].status = 1;
                                            gx_applets_save_installlist(root_path,install_list);
                                        }
                                    }
                                    else
                                    {
                                        if(0 == gx_applets_modify_installlist(&install_usb_list,applets_list->applets[applets_focus_info.run_sel].id,applets_list->applets[applets_focus_info.run_sel].encryption,1))
                                         {
                                             applets_focus_info.node->applets[applets_focus_info.sel_item].status = 1;
                                             gx_applets_save_installlist(root_path,install_usb_list);
                                         }
                                    }
                                    APP_FREE(root_path);
                                }
                            }
#endif
                        }
                        APP_FREE(app_path);
                    }
                    APPLETS_INFO("*********ok*****\n");
                }
                else
                {
                    APPLETS_INFO("*********error*****\n");
                }
                applets_download_listfresh_start();
            }
        }
        is_app_downloading = 0;
        gxapps_curl_set_timeout(0, 0);
    }
}

static void _applets_install(void)
{
	char *user_agent_str = NULL;
	gx_curl_http_options_t http_options;

	if(applets_download_handle > 0)
	{
		app_applets_show_popup_msg(STR_ID_SYSTEM_BUSY_WAIT, 1000);
		APPLETS_INFO("%s,%d,app download handle is exist,%d\n",__func__,__LINE__,applets_download_handle);
	}
	is_app_downloading = 1;
	applets_ret_status = GXAPPS_UNKNOWN_ERROR;
	gxapps_curl_set_timeout(30, 30);
	memset(&http_options, 0, sizeof(gx_curl_http_options_t));
	user_agent_str = applets_get_user_agent();
	http_options.user_agent = user_agent_str;
	applets_download_handle = gxapps_curl_async_download_ext(applets_list->applets[applets_focus_info.run_sel].url, &http_options, NULL, NULL, 0, _applets_download_cb);
	if(user_agent_str)
	{
		free(user_agent_str);
	}
}

static void _applets_download_install(AppletsLoadMode mode)
{
    applets_focus_info.download_mode = mode;
#if APPLETS_INSTALL_SUPPORT
    if(_applets_check_disk_exist()<0)
    {
        return;
    }
    if(_applets_check_size_limit()<0)
    {
        return;
    }
#endif
    if((applets_get_list_status != APPLETS_GET_LIST_SUCCESS)||(applets_focus_info.run_sel<0))
    {
        app_applets_show_popup_msg(STR_ID_APPLETS_GET_LIST_TIP, 1000);
        return;
    }
    if(!pop_msg_on)
    {
        app_applets_show_popup_msg(STR_ID_WAITING, 0);
    }
    applets_app_download_stop();
    _applets_install();

}
static applets_open_status_t app_applets_run(char * path,char *id)
{
    char obj_path[MAX_PATH_LEN];
    char obj_entry[MAX_PATH_LEN];
    char obj_exit[MAX_PATH_LEN];
    char install_path[MAX_PATH_LEN];
    gx_applets_app_t app;
    int ret = -1;
    if(!pop_msg_on)
    {
        app_applets_show_popup_msg(STR_ID_WAITING, 0);
    }
    memset(&app,0,sizeof(gx_applets_app_t));
    if(id)
    {
        sprintf(install_path,"%s/%s",path,id);
    }
    else
    {
        sprintf(install_path,"%s",path);
    }
    APPLETS_INFO("%s,%d,path=%s\n",__func__,__LINE__, install_path);
    memset(&app,0,sizeof(app));
    if(GXAPPS_SUCCESS == gx_applets_get_one_install_app(install_path,&app))
    {
        app_applets_pic_update_timer_stop();
#if APPLETS_RECENT_SUPPORT
        _applets_recent_node();
#endif
        app_applets_set_info(install_path,id);
        app_applets_gui_unload();
        gx_applets_destory_obj();
        sprintf(obj_entry,"%s",app.entry_func);
        sprintf(obj_path,"%s/%s",install_path,app.file);
        sprintf(obj_exit,"%s",app.exit_func);
        APPLETS_INFO("%s,%d,%s,%s,%s\n",__func__,__LINE__, obj_path,obj_entry,obj_exit);
        if(app.need_templet)
            app_applets_gui_load(app.id,install_path);
        ret = gx_applets_entry_obj(obj_path,obj_entry,obj_exit);
        app_applets_hide_popup_msg();
        gx_applets_free_install_one_app(&app);
        if(-1 == ret)
        {
            applets_open_status = APPLETS_STATUS_OBJ_CLOSE_ERROR;
        }
        else if(ret <= -2)
        {
            applets_open_status = APPLETS_STATUS_OBJ_OPEN_ERROR;
        }
        else
        {
            applets_open_status = APPLETS_STATUS_SUCCESS;
        }
    }
    else
    {
        app_applets_hide_popup_msg();
        applets_open_status = APPLETS_STATUS_FILE_ERROR;
    }
    app_applets_show_open_error_msg();
    return applets_open_status;
}


#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
static void app_applets_auto_update_list_func(void)
{
	int count = 0;
	while(applets_auto_update_list_running>0)
	{
		if(applets_auto_update_list_running == 1)
		{
			if(app_applets_video_busy() || (applets_get_list_status == APPLETS_GET_LIST_RUNNING))
			{
				GxCore_ThreadDelay(100);
			}
			else
			{
				applets_get_list_feed();
				if((applets_get_list_status == APPLETS_GET_LIST_SUCCESS))
				{
					applets_auto_update_list_running = 2;
					applets_auto_update_list_count = 0;
				}
				else
				{
					applets_auto_update_list_count++;
					if(applets_auto_update_list_count>=APPLETS_GET_LIST_RETYR_TIMES)
					{
						applets_auto_update_list_running = 2;
					}
					else
					{
						count = 0;
						while((count++<30)&&(applets_auto_update_list_running == 1))
						{
							GxCore_ThreadDelay(100);
						}
					}
				}
			}
		}
		else
		{
			GxCore_ThreadDelay(1000);
		}
	}
}
void app_applets_auto_update_list_start(void)
{
	applets_auto_update_list_count = 0;
	if(applets_auto_update_list_running == 0)
	{
		applets_auto_update_list_running = 1;
		create_applets_thread(app_applets_auto_update_list_func);
	}
	else
	{
		applets_auto_update_list_running = 1;
	}
}
void app_applets_auto_update_list_stop(void)
{
	applets_auto_update_list_count = 0;
	if(applets_auto_update_list_running>0)
	{
		applets_auto_update_list_running = 2;
	}
}
// source: 1->from builtin,0->from install or online
int _applets_mainmenu_entry(char *id,AppletsLoadMode mode)
{
    char *app_install_path = NULL;
    if(NULL == id)
    {
        app_applets_show_popup_msg(STR_ID_APPLETS_UNEXIST, 1000);
        return -1;
    }
    if((applets_get_list_status != APPLETS_GET_LIST_SUCCESS)||(applets_list ==NULL) ||(applets_list->applet_num<=0))
    {
        app_applets_show_popup_msg(STR_ID_APPLETS_GET_LIST_TIP, 1000);
        return -1;
    }
    applets_focus_info.run_from_mainmenu = false;
#if APPLETS_INSTALL_SUPPORT
    applets_setting_init();
    applets_setting_get();
#endif
#if (APPLETS_INSTALL_SUPPORT||APPLETS_BUILTIN_SUPPORT)
    app_install_path = app_applets_get_download_path(id,mode);
    if(applets_get_ftype(app_install_path) == 0) //app installed in flash or usb
    {
        app_mainmenu_tip_exec(MM_APPLETS_OPEN);
        GUI_SetInterface("flush", NULL);
        char * root_path = NULL;
        root_path = app_applets_get_download_path(NULL,mode);
        app_applets_run(root_path,id);
        APP_FREE(root_path);
    }
    else
#endif
    {
        applets_focus_info.run_sel = _appletlist_check_id_exist(applets_list,id);
        if(applets_focus_info.run_sel < 0)
        {
            app_applets_show_popup_msg(STR_ID_APPLETS_UNEXIST, 1000);
            APP_FREE(app_install_path);
            return -1;
        }
        applets_focus_info.run_from_mainmenu = true;
        applets_download_listfresh_stop();
        applets_cloud_listfresh_stop();
        applets_app_update_stop();
        applets_data_timer_start();
        APPLETS_INFO("%s,%d,run_sel=%d,url=%s\n",__func__,__LINE__,applets_focus_info.run_sel,applets_list->applets[applets_focus_info.run_sel].url);
        applets_dir_or_file_delete(TMP_INSTALL_PATH, 0);
        applets_dir_or_file_delete(TMP_ZIP_PATH, 0);
        _applets_download_install(DOWNLOAD_MODE_ONLINE);
    }
    APP_FREE(app_install_path);
    return 0;
}
int app_applets_mainmenu_builtin_entry(char *id)
{
    return _applets_mainmenu_entry(id,DOWNLOAD_MODE_BUILTIN);

}
int app_applets_mainmenu_entry(char *id)
{
    return _applets_mainmenu_entry(id,DOWNLOAD_MODE_INSTALL);

}

#endif

#if APPLETS_INSTALL_SUPPORT
static int _applets_flash_check_sections(void)
{
#define DATA_SECTION_NAME "DATA"
    struct partition* flash_tbl = NULL;
    struct partition_info* part = NULL;
    flash_tbl = GxCore_PartitionFlashInit();
    if(!flash_tbl)
        return -1;
    part = app_get_partition(flash_tbl, DATA_SECTION_NAME);
    if(part == NULL)
        return -1;
    if(part->total_size <= 512*SIZE_UNIT_K )
        return INSTALL_FLASH_MIN_NUM;
    else
        return INSTALL_FLASH_MAX_NUM;
}
static char *  _applets_usb_get_partition_entry(void)
{
	HotplugPartitionList *partition_list = NULL;
	if(check_usb_status() == false)
	{
        return NULL;
	}
	partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (partition_list == NULL || partition_list->partition_num == 0)
    {
        return NULL;
    }
    APPLETS_INFO("%s,%d,%s\n",__func__,__LINE__,partition_list->partition[0].partition_entry);

    return partition_list->partition[0].partition_entry;
}

static int applets_create_install_path(void)
{
	char * root_path = NULL;
	char install_path[MAX_PATH_LEN];
	memset(install_path,0,MAX_PATH_LEN);
	if(INSTALL_PATH_FLASH == applets_setting->install_path)
	{
	   if(0 == applets_get_ftype(_applets_flash_get_install_root_path()))
		   return -1;
	   sprintf(install_path,"%s/applets",FLASH_ROOT_PATH);
	}
	else
	{
		if(0 == applets_get_ftype(_applets_usb_get_install_root_path()))
			return -1;
		root_path = _applets_usb_get_partition_entry();
		if(root_path == NULL)
		{
			return -1;
		}
		sprintf(install_path,"%s/applets",root_path);
	}
	if(0 != applets_get_ftype(install_path))
	{
		if(mkdir(install_path, 0777) < 0)
			return -1;
	}
	strcat(install_path,"/");
	strcat(install_path,APPLETS_VERSION);
	APPLETS_INFO("install_path=%s\n",install_path);
	if(0 != applets_get_ftype(install_path))
	{
		if(mkdir(install_path, 0777) < 0)
			return -1;
	}
	strcat(install_path,"/");
	strcat(install_path,APPLETS_ARCH);
	APPLETS_INFO("install_path=%s\n",install_path);
	if(0 != applets_get_ftype(install_path))
	{
		if(mkdir(install_path, 0777) < 0)
			return -1;
	}
	return 0;
}

static char *  _applets_flash_get_install_root_path(void)
{
	char install_path[MAX_PATH_LEN];
	if(flash_install_path)
	{
		APPLETS_INFO("%s,%d,%s\n",__func__,__LINE__,flash_install_path);
		return flash_install_path;
	}
	memset(install_path,0,MAX_PATH_LEN);
	sprintf(install_path,"%s/applets/%s/%s",FLASH_ROOT_PATH,APPLETS_VERSION,APPLETS_ARCH);

	flash_install_path = strdup(install_path);
	APPLETS_INFO("%s,%d,%s\n",__func__,__LINE__,install_path);
	return flash_install_path;
}

static char *  _applets_usb_get_install_root_path(void)
{
	static char install_path[MAX_PATH_LEN];
	char * root_path = NULL;
	if(usb_install_path)
	{
		APPLETS_INFO("%s,%d,%s\n",__func__,__LINE__,usb_install_path);
		return usb_install_path;
	}
	root_path = _applets_usb_get_partition_entry();
	if(root_path == NULL)
	{
		return NULL;
	}
	memset(install_path,0,MAX_PATH_LEN);
	sprintf(install_path,"%s/applets/%s/%s",root_path,APPLETS_VERSION,APPLETS_ARCH);
	usb_install_path = strdup(install_path);
	APPLETS_INFO("%s,%d,%s\n",__func__,__LINE__,install_path);
	return usb_install_path;
}

static int _applets_usb_check_size_limit(unsigned int limit_size)
{
	GxDiskInfo disk_info;
	char * partition_entry = NULL;
	uint64_t freed_size = 0;

	partition_entry = _applets_usb_get_partition_entry();
	if (NULL == partition_entry)
	{
        goto err1;
	}
	APPLETS_INFO("%s,%d,partition_entry=%s\n",__func__,__LINE__,partition_entry);
	GxCore_DiskInfo(partition_entry, &disk_info);
    {
        freed_size = disk_info.freed_size - limit_size;
        if (freed_size <= 0)
            goto err1;
        APPLETS_INFO("free size=%u,limit_size=%u\n", (unsigned)disk_info.freed_size,limit_size);
        return freed_size;
    }
    err1:
        APPLETS_INFO("usb limit failed!free size=%u,limit_size=%u\n", (unsigned)disk_info.freed_size,limit_size);
        return -1;

}

int _applets_flash_check_size_limit(unsigned int limit_size)
{
	int freed_size = 0;
	GxDiskInfo disk_info = {0};
	int num = 0;

	if (GxCore_DiskInfo(FLASH_ROOT_PATH, &disk_info) == GXCORE_SUCCESS)
	{
		freed_size = disk_info.freed_size - limit_size;
		if(install_list)
			num = install_list->install_num;
		if(freed_size <= 0)
			goto err2;
		if(num >= _applets_flash_check_sections())
			goto err2;
		APPLETS_INFO("free size=%u,limit_size=%u,num=%d\n", (unsigned)disk_info.freed_size,limit_size,num);
		return freed_size;
	}

err2:
	APPLETS_INFO("flash limit failed!free size=%u,limit_size=%u,num=%d\n", (unsigned)disk_info.freed_size,limit_size,num);
	return -1;
}

static int _applets_check_disk_exist(void)
{
	if((DOWNLOAD_MODE_INSTALL == applets_focus_info.download_mode)&&(INSTALL_PATH_USB == applets_setting->install_path))
	{
		if(check_usb_status() == false)
		{
			app_applets_show_popup_msg(STR_ID_INSERT_USB, 1000);
			app_applets_pic_update_timer_reset();
			return -1;
		}
	}
	return 0;
}

static int _applets_check_size_limit(void)
{
    int ret = 0;

    if(DOWNLOAD_MODE_INSTALL == applets_focus_info.download_mode)
    {
        if(INSTALL_PATH_FLASH == applets_setting->install_path)
        {
            ret = _applets_flash_check_size_limit(FLASH_MIN_FREE_SIZE);
        }
        else
        {
            ret = _applets_usb_check_size_limit(USB_MIN_FREE_SIZE);
        }
        if(ret == -1)
        {
            app_applets_show_popup_msg(STR_ID_APPLETS_LIMIT_TIP, 1000);
            app_applets_pic_update_timer_reset();
            return -1;
        }
        return 0;
    }
    return 0;
}

static int applets_check_app_from_install(void)
{
    int i = 0;
    int j = 0;
    gx_applets_app_t app;
    char * app_path = NULL;
    gx_applets_installlist_t *list = NULL;
    char obj_path[MAX_PATH_LEN];

    if(NULL == applets_list)
        return -1;
    if(INSTALL_PATH_FLASH == applets_setting->install_path)
    {
        list = install_list;
    }
    else
    {
        list = install_usb_list;
    }
    if(NULL == list)
        return -1;
    for(i=0;i<list->install_num;i++)
    {
        APP_FREE(app_path);
        memset(&app,0,sizeof(app));
        app_path = app_applets_get_download_path(list->install[i].id,DOWNLOAD_MODE_INSTALL);
        if(GXAPPS_SUCCESS == gx_applets_get_one_install_app(app_path,&app))
        {
            sprintf(obj_path,"%s/%s",app_path,app.file);
            gx_applets_free_install_one_app(&app);
            if(GxCore_FileExists(obj_path) != GXCORE_FILE_EXIST)
            {
                applets_dir_or_file_delete(app_path,0);
                continue;
            }
        }
        for(j=0;j<applets_list->applet_num;j++)
        {
            if(0 == strcmp(list->install[i].id,applets_list->applets[j].id))
                break;
        }
        if(j == applets_list->applet_num)
        {
            applets_dir_or_file_delete(app_path,0);
        }
    }
    APP_FREE(app_path);
    return -1;
}

static void applets_free_install_list(void)
{
	if(install_list)
	{
        gx_applets_free_install_list(install_list);
		install_list = NULL;
	}
}
static void applets_free_install_usb_list(void)
{
	if(install_usb_list)
	{
        gx_applets_free_install_list(install_usb_list);
		install_usb_list = NULL;
	}
}

static void _applets_uninstall(void)
{
    int sel = 0;
    char * app_path = NULL ;
    char * root_path = NULL;
	gxapps_ret_t ret = GXAPPS_UNKNOWN_ERROR;
	unsigned int handle = 0;
	is_applets_download = 1;
	applets_data_handle++;
	handle = applets_data_handle;
	if(handle == applets_data_handle)
	{
        sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
        app_path = app_applets_get_download_path(applets_list->applets[sel].id,DOWNLOAD_MODE_INSTALL);
        if(0 == applets_get_ftype(app_path))
        {
            ret = applets_dir_or_file_delete(app_path, 0);
            applets_ret_status = ret;
            if(applets_ret_status == GXAPPS_SUCCESS)
            {
                applets_open_status = APPLETS_STATUS_SUCCESS;
                root_path = app_applets_get_download_path(NULL,DOWNLOAD_MODE_INSTALL);
                if(root_path)
                {
                    if(INSTALL_PATH_FLASH == applets_setting->install_path)
                    {
                        if(0 == gx_applets_modify_installlist(&install_list,applets_list->applets[sel].id,applets_list->applets[sel].encryption,0))
                        {
                            applets_focus_info.node->applets[applets_focus_info.sel_item].status = 0;
                            gx_applets_save_installlist(root_path,install_list);
                        }
                    }
                    else
                    {
                        if(0 == gx_applets_modify_installlist(&install_usb_list,applets_list->applets[sel].id,applets_list->applets[sel].encryption,0))
                         {
                             applets_focus_info.node->applets[applets_focus_info.sel_item].status = 0;
                             gx_applets_save_installlist(root_path,install_usb_list);
                         }
                    }
                    APP_FREE(root_path);
                }
            }
            else
            {
                applets_open_status = APPLETS_STATUS_MKDIR_ERROR;
            }
#ifdef APPLETS_DEBUG
            APPLETS_INFO("_applets_uninstall\n");
#endif
        }
        else
        {
            applets_open_status = APPLETS_STATUS_MKDIR_ERROR;
        }
        applets_download_listfresh_start();
	}
    APP_FREE(app_path);
	is_applets_download = 0;
}

static void _applets_download_uninstall(AppletsLoadMode mode)
{
	applets_download_listfresh_stop();
	if(app_applets_video_busy())
	{
		app_applets_show_popup_msg("System Busy, Please Wait!", 1000);
	}
	else
	{
		applets_focus_info.download_mode = mode;
		gx_applets_destory_obj();
		app_applets_gui_unload();
		app_applets_show_popup_msg("Waiting...", 0);
		create_applets_thread(_applets_uninstall);
	}
}

static void  app_applets_download_install_unstall(void)
{
    PopDlg  pop;
    if(1 == applets_focus_info.node->applets[applets_focus_info.sel_item].source)
    {
        return;
    }
    app_applets_pic_update_timer_stop();
    if(1 == applets_focus_info.node->applets[applets_focus_info.sel_item].status)
    {
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.str = STR_ID_APPLETS_UNINSTALL_TIP;
        if(popdlg_create(&pop) == POP_VAL_OK)
        {
            _applets_download_uninstall(DOWNLOAD_MODE_INSTALL);
        }
    }else
    {
		memset(&pop, 0, sizeof(PopDlg));
		pop.type = POP_TYPE_YES_NO;
		pop.str = STR_ID_APPLETS_INSTALL_TIP;
		if(popdlg_create(&pop) == POP_VAL_OK)
		{
            _applets_download_install(DOWNLOAD_MODE_INSTALL);
		}
    }
    app_applets_pic_update_timer_reset();
}

static int applets_version_check(char *oldver, char *newver)
{
	int old_ver1=0, old_ver2=0, old_ver3=0;
	int new_ver1=0, new_ver2=0, new_ver3=0;
	unsigned int old_ver = 0, new_ver = 0;

    APPLETS_INFO("%s,%d,%s,%s\n",__func__,__LINE__,oldver,newver);
	old_ver = ((old_ver1%256)<<16)+((old_ver2%256)<<8)+((old_ver3%256));
	new_ver = ((new_ver1%256)<<16)+((new_ver2%256)<<8)+((new_ver3%256));
	if(new_ver>old_ver)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}
static void _applets_update_installed(void)
{
    int sel = 0;
    gx_applets_app_t app;
    char *app_path = NULL;
    static int sel_item = -1;
    if(sel_item == applets_focus_info.sel_item)
    {
        return;
    }
    sel_item = applets_focus_info.sel_item;
    if((!applets_focus_info.node) || (applets_focus_info.node->applet_num <= sel_item))
    {
        return;
    }
    if((1 == applets_focus_info.node->applets[sel_item].source)||(0 == applets_focus_info.node->applets[sel_item].status))
    {
        return;
    }
    sel = applets_focus_info.node->applets[sel_item].index;
    memset(&app,0,sizeof(app));
    app_path = app_applets_get_download_path(applets_focus_info.node->applets[sel_item].id,DOWNLOAD_MODE_INSTALL);
    applets_app_update_stop();
    if(GXAPPS_SUCCESS == gx_applets_get_one_install_app(app_path,&app))
    {
        if(1 == applets_version_check(app.version,applets_list->applets[sel].version))
        {
            PopDlg  pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_YES_NO;
            pop.str = STR_ID_APPLETS_UPDATE_TIP;
            if(popdlg_create(&pop) == POP_VAL_OK)
            {
                _applets_download_install(DOWNLOAD_MODE_INSTALL);
            }
        }
        gx_applets_free_install_one_app(&app);
    }
    APP_FREE(app_path);
}
static char *app_applets_get_install_type_array_data(void)
{
	char *install_type_array = NULL;
	int i = 0;

	install_type_array = GxCore_Calloc(1, APPLETS_INSTALL_TYPE_STR_LEN);
	if(install_type_array)
	{
		memset(install_type_array, 0, APPLETS_INSTALL_TYPE_STR_LEN);
		strcpy(install_type_array, "[");
		for(i=0; i<APPLETS_INSTALL_TYPE_NUM; i++)
		{
			strcat(install_type_array, applets_install_type_str[i]);
			if(i == (APPLETS_INSTALL_TYPE_NUM-1))
			{
				strcat(install_type_array, "]");
			}
			else
			{
				strcat(install_type_array, ",");
			}
		}
	}

	return install_type_array;
}
static int app_applets_install_type_setting_get(void)
{
	int value = 0;

	GxBus_ConfigGetInt(APPLETS_INSTALL_TYPE_KEY, &value, APPLETS_DEFAULT_INSTALL_TYPE);
	if((value<0)||(value>=APPLETS_INSTALL_TYPE_NUM))
	{
		value = 0;
	}

	return value;
}
static int app_applets_install_type_setting_save(int value)
{
	if((value<0)||(value>=APPLETS_INSTALL_TYPE_NUM))
	{
		value = 0;
	}
	GxBus_ConfigSetInt(APPLETS_INSTALL_TYPE_KEY,value);

	return value;
}

static void applets_setting_init(void)
{
    if(NULL == applets_setting_opt)
	    applets_setting_opt = (SystemSettingOpt *)GxCore_Calloc(1, sizeof(SystemSettingOpt));
    if(NULL == applets_setting_item)
	    applets_setting_item = (SystemSettingItem *)GxCore_Calloc(1, sizeof(SystemSettingItem)*APPLETS_SETTING_MAX);
    if(NULL == applets_setting)
	    applets_setting = (applets_setting_t *)GxCore_Calloc(1, sizeof(applets_setting_t));
    if(NULL == applets_install_type_array)
	    applets_install_type_array = app_applets_get_install_type_array_data();
}

static void applets_setting_destroy(void)
{
	APP_FREE(applets_setting_opt);
	APP_FREE(applets_setting_item);
	APP_FREE(applets_setting);
	APP_FREE(applets_install_type_array);
	applets_setting_opt = NULL;
	applets_setting_item = NULL;
	applets_setting = NULL;
	applets_install_type_array = NULL;
}
static void applets_setting_get(void)
{
	if(!applets_setting)
	{
		return;
	}

	applets_setting->install_path = app_applets_install_type_setting_get();
}
static void applets_setting_result_get(applets_setting_t *ret_para)
{
	ret_para->install_path = applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemProperty.itemPropertyCmb.sel;
}

static int applets_setting_change(applets_setting_t *para)
{
	if(applets_setting->install_path == para->install_path)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
static int applets_setting_save(applets_setting_t *ret_para)
{
	applets_setting->install_path = app_applets_install_type_setting_save(ret_para->install_path);
	return 0;
}

static void applets_setting_all(void)
{
    app_applets_show_popup_msg(STR_ID_WAITING, 0);
    _applets_unfocus_item(applets_focus_info.sel_item);
    _applets_clean_all_item();
    applets_create_install_path();
    applets_get_all_data();
    applets_focus_info.focus_area = FOCUSAREA_LIST_GROUP;
    applets_app_update_stop();
    _group_get_item_data(applets_focus_info.sel_group);
    _update_focus_info_item_init();
    _applets_group_list_focus();
    _applets_group_list_update();
    _applets_group_list_active();
    app_applets_hide_popup_msg();
    _applets_update_icon();
    return;
}
static bool _applets_search_check_cb(void)
{
	bool ret = false;
	applets_setting_t para;

	applets_setting_result_get(&para);
	ret = applets_setting_change(&para);

	return ret;
}

static int _applets_setting_save_pop_cb(PopDlgRet ret)
{
	applets_setting_t para;

	if(POP_VAL_OK == ret)
	{
		applets_setting_result_get(&para);
		applets_setting_save(&para);
		applets_setting_all();
	}
	app_system_reset_unfocus_image();
	GUI_EndDialog(WND_SYSTEM_SETTING);
	GUI_SetProperty(WND_SYSTEM_SETTING,"draw_now",NULL);
	app_applets_pic_update_timer_reset();

	return 0;
}
static int applets_setting_exit_cb(ExitType Type)
{
	int ret = EXIT_RET_DEFAULT;

	if(EXIT_ABANDON == Type)
	{
		;//app_net_video_enable_group_roll();
	}
	else
	{
		if(false == app_system_set_callback_handler(_applets_search_check_cb, _applets_setting_save_pop_cb))
		{
			;//app_net_video_enable_group_roll();
		}
		ret = EXIT_RET_POP;
	}

	return ret;
}

static int applets_change_callback(int sel)
{
	return EVENT_TRANSFER_KEEPON;
}

static int app_applets_pop_install_type_list(int sel)
{
	PopList pop_list;
	int ret = 0;

	memset(&pop_list, 0, sizeof(PopList));
	pop_list.title = "Install type";
	pop_list.item_num = APPLETS_INSTALL_TYPE_NUM;
	pop_list.item_content = applets_install_type_str;
	pop_list.sel = sel;
	pop_list.show_num= false;
	pop_list.pos.x = APP_NETAPP_SETTING_POP_LIST_X;
	pop_list.pos.y = APP_NETAPP_SETTING_POP_LIST_Y;
	ret = poplist_create(&pop_list);

	return ret;
}

static int applets_cmb_install_type_press_callback(int key)
{
	int cmb_sel = -1;

	if(key == STBK_OK)
	{
		GUI_GetProperty(app_system_get_combobox(APPLETS_SETTING_INSTALL_MODE), "select", &cmb_sel);
		cmb_sel = app_applets_pop_install_type_list(cmb_sel);
		GUI_SetProperty(app_system_get_combobox(APPLETS_SETTING_INSTALL_MODE), "select", &cmb_sel);
	}

	return EVENT_TRANSFER_KEEPON;
}

static void applets_setting_item_init(void)
{
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemTitle = "Install Path";
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemType = ITEM_POP_CHOICE;
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemProperty.itemPropertyCmb.content= applets_install_type_array;
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemProperty.itemPropertyCmb.sel = applets_setting->install_path;
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemCallback.cmbCallback.CmbChange= applets_change_callback;
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemCallback.cmbCallback.CmbPress= applets_cmb_install_type_press_callback;
	applets_setting_item[APPLETS_SETTING_INSTALL_MODE].itemStatus = ITEM_NORMAL;
}

void applets_create_setting_menu(void)
{
	memset(applets_setting_opt, 0 ,sizeof(SystemSettingOpt));
	memset(applets_setting_item, 0 ,sizeof(SystemSettingItem)*APPLETS_SETTING_MAX);
	memset(applets_setting, 0 ,sizeof(applets_setting_t));

	applets_setting_get();
	applets_setting_item_init();
	app_applets_pic_update_timer_stop();

	applets_setting_opt->menuTitle = "Applets Setting";
	applets_setting_opt->itemNum = APPLETS_SETTING_MAX;
	applets_setting_opt->item = applets_setting_item;
	applets_setting_opt->exit = applets_setting_exit_cb;
	app_system_set_create(applets_setting_opt);
}

#endif
#if APPLETS_FAV_SUPPORT
static void applets_free_fav_list(void)
{
	if(fav_list)
	{
		gx_applets_free_fav_list(fav_list);
		fav_list = NULL;
	}
}

static void _applets_fav_unfav_node(int sel,int add_or_del)
{
    int ret = -1;
    if(sel <0)
    {
        return;
    }
    ret = gx_applets_modify_favlist(&fav_list,applets_focus_info.node->applets[sel].id,applets_focus_info.node->applets[sel].source,add_or_del);
    if(ret >=0)
    {
        gx_applets_save_favlist(fav_list);
        applets_focus_info.node->applets[sel].fav = add_or_del;
        _applets_update_icon();
    }
    return;
}
static void  app_applets_fav_node(void)
{
    PopDlg  pop;
    int sel = 0;
    int add_or_del;
    sel = applets_focus_info.sel_item;
    add_or_del = 1- applets_focus_info.node->applets[sel].fav;
    memset(&pop, 0, sizeof(PopDlg));
    if(0 == add_or_del)
    {
        pop.str = STR_ID_APPLETS_UNFAV_TIP;

    }else
    {
		pop.str = STR_ID_APPLETS_FAV_TIP;
    }
    pop.type = POP_TYPE_YES_NO;
    app_applets_pic_update_timer_stop();
    if(popdlg_create(&pop) == POP_VAL_OK)
    {
       _applets_fav_unfav_node(sel,add_or_del);
    }
    app_applets_pic_update_timer_reset();
}

#endif
#if APPLETS_RECENT_SUPPORT
int applets_recent_modify(gx_applets_favlist_t ** data,char *id,int source)
{
    int j = 0,i=0,index_callback =MAX_RECENT_NUM;
    gx_applets_favlist_t * list = *data;
    if(!list)
    {
        list = (gx_applets_favlist_t *)malloc(sizeof(gx_applets_favlist_t));
        memset(list, 0, sizeof(gx_applets_favlist_t));
        list->fav = (gx_applets_fav_t *)malloc(MAX_RECENT_NUM*sizeof(gx_applets_fav_t));
        memset(list->fav, 0, MAX_RECENT_NUM*sizeof(gx_applets_fav_t));
        *data = list;
    }
    else
    {
        for (j = MAX_RECENT_NUM - 1; j >= 0; j--)
        {
            if((j<list->fav_num)&&(0 == strcmp(list->fav[j].id,id))&&(list->fav[j].source == source))
            {
                index_callback = j;
                APPLETS_INFO("!!!!id[%d]=%s\n",j,list->fav[j].id);
                break;
            }
        }
    }
    if (index_callback > MAX_RECENT_NUM - 1)
    {
        list->fav_num ++;
        index_callback = MAX_RECENT_NUM - 1;
    }
    if (list->fav_num > MAX_RECENT_NUM)
    {
        list->fav_num = MAX_RECENT_NUM;
    }
    if((index_callback == MAX_RECENT_NUM - 1)&&(list->fav[index_callback].id))
    {
        APPLETS_INFO("free id[%d]=%s\n",index_callback,list->fav[index_callback].id);
        free(list->fav[index_callback].id);
        list->fav[index_callback].id = NULL;
    }
    for (j = index_callback; j > 0; j--)
    {
        list->fav[j].id = list->fav[j-1].id ;
        list->fav[j].source = list->fav[j-1].source;
    }
    list->fav[0].id = strdup(id);
    list->fav[0].source = source;
    for(i=0;i<list->fav_num;i++)
        APPLETS_INFO("applets_recent_modify add =%s\n",list->fav[i].id);
    return 0;
}

static void _applets_recent_node(void)
{
   int sel = 0;
   char* focus_win = NULL;
   focus_win = (char*)GUI_GetFocusWindow();
   if(!focus_win)
       return;
   if(0 == strcasecmp(focus_win, WND_APPLETS))
   {
        sel = applets_focus_info.sel_item;
        APPLETS_INFO("_applets_recent_node:sel=%d,%s\n",sel,applets_focus_info.node->applets[sel].id);
        applets_recent_modify(&recent_list,applets_focus_info.node->applets[sel].id,applets_focus_info.node->applets[sel].source);
   }
}
#endif
#ifdef APPLETS_SUPPORT_USB
static char *file_path = NULL;
static char *file_path_bak = NULL;
static int _app_applets_filedlg_exit_cb(WndStatus wnd_ret)
{
    int str_len = 0;
    char * before = NULL;
    char *end = NULL;
    char obj_entry[MAX_PATH_LEN];
    char obj_exit[MAX_PATH_LEN];
    char name[128];
    char install_path[MAX_PATH_LEN];
    int ret = -1;

    if (wnd_ret == WND_OK)
    {
        app_get_file_real_path(&file_path);
        if (file_path != NULL)
        {
            ////backup the path...////
            APP_FREE(file_path_bak);
            str_len = strlen(file_path);
            file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if (file_path_bak != NULL)
            {
                memcpy(file_path_bak, file_path, str_len);
                file_path_bak[str_len] = '\0';
            }
            sprintf(install_path,"%s",TMP_INSTALL_PATH);
            if(strstr(file_path,".zip")||strstr(file_path,".ZIP"))
            {
                APPLETS_INFO("%s,%d,path=%s,zippath=%s\n", __func__,__LINE__,install_path,file_path);
                app_applets_show_popup_msg(STR_ID_WAITING, 0);
                applets_dir_or_file_delete(install_path, 0);
                if(mkdir(install_path, 0777)>=0)
                {
                    ret = gx_applets_unzip_app(file_path,install_path);
                    if(ret == 0)
                    {
                        app_applets_run(install_path,NULL);
                    }
                    else
                    {
                        applets_dir_or_file_delete(install_path, 0);
                    }
                }
                app_applets_hide_popup_msg();
            }
            else if((end = strstr(file_path,".o")))
            {
                before = strrchr(file_path,'/');
                if(before)
                {
                    memset(name,0,MAX_PATH_LEN);
                    memset(install_path,0,MAX_PATH_LEN);
                    app_applets_show_popup_msg(STR_ID_WAITING, 0);
                    gx_applets_destory_obj();
                    app_applets_gui_unload();
                    app_applets_hide_popup_msg();
                    memcpy(name,before+1,end - before-1);
                    sprintf(obj_entry,"app_create_%s_menu",name);
                    sprintf(obj_exit,"app_%s_obj_exit",name);
                    APPLETS_INFO("%s,%d,%s,%s\n",__func__,__LINE__,name, file_path);
                    memcpy(install_path,file_path,strlen(file_path)-strlen(name)-3);
                    APPLETS_INFO("%s,%d,%s,%s,%s,%s\n",__func__,__LINE__,install_path, file_path,obj_entry,obj_exit);
                    app_applets_set_info(install_path,name);
                    ret = gx_applets_entry_obj(file_path,obj_entry,obj_exit);
                    if(-1 == ret)
                    {
                        app_applets_show_popup_msg(STR_ID_APPLETS_CLOSE_TIP, 1000);
                    }
                    else if(ret <= -2)
                    {
                        app_applets_show_popup_msg(STR_ID_APPLETS_OPEN_FAIL, 1000);
                    }
                }
            }
        }
    }
    return 0 ;
}
static void _applets_from_usb(void)
{
#define UPDATE_FILE_SUFFFIX    "o;zip;ZIP"
    FileListParam file_para;

    if (check_usb_status() == false)
    {
		app_applets_show_popup_msg(STR_ID_INSERT_USB, 1000);
        return;
    }
    if (GXCORE_FILE_EXIST != GxCore_FileExists(file_path_bak))
        APP_FREE(file_path_bak);
    memset(&file_para, 0, sizeof(file_para));
    file_para.cur_path = file_path_bak;
    file_para.dest_path = &file_path;
    file_para.suffix = UPDATE_FILE_SUFFFIX;
    file_para.dest_mode = DEST_MODE_FILE;
    file_para.exit_cb = _app_applets_filedlg_exit_cb;
    app_get_file_path_dlg(&file_para);
}
static void app_applets_from_usb(void)
{
    _applets_from_usb();
}
#endif
#ifdef APPLETS_DEBUG
static void _applets_input_release_cb(PopKeyboard *data)
{
    char applets_server[MAX_PATH_LEN];
	if(data->in_ret == POP_VAL_CANCEL)
		return;
	if(data->out_name == NULL)
		return;
    memset(applets_server,0,MAX_PATH_LEN);
	strcpy(applets_server, data->out_name);

    GxBus_ConfigSet(APPLETS_SERVER_KEY, applets_server);
    gx_applets_set_customer_server_url(applets_server);
    app_applets_show_popup_msg(STR_ID_APPLETS_SET_SERVER, 1000);
}

int app_applets_check_fun(int key) // 1 open
{
#define CODE_LEN 4
    static int32_t count = 0;
    PopKeyboard keyboard;
    char applets_server[MAX_PATH_LEN];
    int proper_code[CODE_LEN] = {STBK_8, STBK_8, STBK_1, STBK_6};
    if(key == proper_code[count])
    {
        count++;
        if(count == CODE_LEN)
        {
            count = 0;
            memset(applets_server,0,MAX_PATH_LEN);
            GxBus_ConfigGet(APPLETS_SERVER_KEY, applets_server, MAX_PATH_LEN, APPLETS_SERVER_DEFALT);
            memset(&keyboard, 0, sizeof(PopKeyboard));
            keyboard.in_name    = applets_server;
            keyboard.max_num = MAX_PATH_LEN;
            keyboard.out_name   = NULL;
            keyboard.change_cb  = NULL;
            keyboard.release_cb = _applets_input_release_cb;
            keyboard.pos.x = 500;
            multi_language_keyboard_create(&keyboard);
            return 1;
        }
    }
    else
    {
        count = 0;
    }
    return 0;
}
#endif

static void _applets_ok_keypress(void)
{
    char * app_pach = NULL;
    int status,source;
    applets_focus_info.run_sel = applets_focus_info.node->applets[applets_focus_info.sel_item].index;
    status = applets_focus_info.node->applets[applets_focus_info.sel_item].status;
    source = applets_focus_info.node->applets[applets_focus_info.sel_item].source;
    applets_focus_info.run_from_mainmenu = false;
    if(source)
    {
        app_pach = app_applets_get_download_path(NULL,DOWNLOAD_MODE_BUILTIN);
    }
    else
    {
        app_pach = app_applets_get_download_path(NULL,DOWNLOAD_MODE_INSTALL);
    }
    if(0 == status)
    {
        _applets_download_install(DOWNLOAD_MODE_ONLINE);
    }
    else
    {
        app_applets_show_popup_msg(STR_ID_WAITING, 0);
        app_applets_run(app_pach,applets_focus_info.node->applets[applets_focus_info.sel_item].id);
    }
    APP_FREE(app_pach);
}

static void app_applets_menu_exit_cb(void)
{
	applets_data_handle++;
    _applets_free_gif();
	applets_cloud_listfresh_stop();
    applets_download_listfresh_stop();
	app_applets_pic_update_timer_rm();
	app_applets_data_timer_stop();
	app_applets_clean_menu_info();
	applets_pic_download_stop();
    applets_app_download_stop();
#if APPLETS_INSTALL_SUPPORT
    applets_app_update_stop();
    applets_free_install_list();
    applets_free_install_usb_list();
    applets_setting_destroy();
#endif
#if !APPLETS_AUTO_UPDATE_LIST_SUPPORT
	applets_free_cloud_list();
#if APPLETS_BUILTIN_SUPPORT
    applets_free_builtin_list();
#endif
#endif
#if APPLETS_FAV_SUPPORT
    applets_free_fav_list();
#endif
#if APPLETS_RECENT_SUPPORT
    applets_free_recent_list();
#endif
    applets_free_infonode_list();
    applets_free_install_app_list();
    applets_dir_or_file_delete(TMP_INSTALL_PATH, 0);
    applets_dir_or_file_delete(TMP_ZIP_PATH, 0);
	app_netapps_app_destroy();
}
static void app_applets_menu_init(void)
{
#if APPLETS_INSTALL_SUPPORT
    applets_setting_init();
    applets_setting_get();
    applets_create_install_path();
#endif
    applets_cloud_listfresh_stop();
    applets_download_listfresh_stop();
    applets_app_update_stop();
    applets_data_timer_start();
    _applets_unfocus_all_item();
    _applets_load_gif();
    applets_cloud_list_update();

}
static int _list_update_exit_pop_cb(PopDlgRet ret)
{
    if(POP_VAL_OK == ret)
    {
        if(applets_get_list_status != APPLETS_GET_LIST_RUNNING)
        {
            applets_get_list_status = APPLETS_GET_LIST_FAILED;
        }
        applets_cloud_list_update();
    }
    return 0;
}

static void app_applets_list_update(void)
{
	PopDlg	pop;
	app_applets_pic_update_timer_stop();
	applets_cloud_listfresh_stop();
	applets_download_listfresh_stop();
	_applets_left_exit();
	memset(&pop, 0, sizeof(PopDlg));
	pop.str = STR_ID_APPLETS_UPDATE_TIP;
	pop.type = POP_TYPE_YES_NO;
	pop.mode = POP_MODE_UNBLOCK;
	pop.exit_cb = _list_update_exit_pop_cb;
	popdlg_create(&pop);
}
static void app_applets_close_menu(void)
{
	app_applets_show_popup_msg(STR_ID_WAITING, 0);
	gx_applets_destory_obj();
	app_applets_hide_popup_msg();
	GUI_EndDialog(WND_APPLETS);
	app_applets_gui_unload();
}
SIGNAL_HANDLER int app_applets_create(GuiWidget *widget, void *usrdata)
{
#if APPLETS_INSTALL_SUPPORT
    GUI_SetProperty(TXT_RED, "state", "show");
    GUI_SetProperty(IMG_RED, "state", "show");
#endif
    app_applets_menu_init();
    return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_applets_destroy(GuiWidget *widget, void *usrdata)
{
    app_applets_menu_exit_cb();
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_applets_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(GUI_KEYDOWN ==  event->type)
    {
#ifdef APPLETS_DEBUG
        if(app_applets_check_fun(event->key.sym))
            return EVENT_TRANSFER_STOP;
#endif
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_OK:
                if(!pop_msg_on)
                    _applets_ok_keypress();
                break;
            case STBK_EXIT:
            case STBK_MENU:
                if(!pop_msg_on)
                {
                    _applets_left_exit();
                }
                else
                {
                    if( is_app_downloading)
                    {
                        applets_app_download_stop();
                        app_applets_hide_popup_msg();
                    }
                }
                break;
            case STBK_UP:
                if(!pop_msg_on)
                    _applets_up_keypress(STBK_UP);
                break;
            case STBK_DOWN:
                if(!pop_msg_on)
                    _applets_down_keypress(STBK_DOWN);
                break;
            case STBK_PAGE_UP:
                if(!pop_msg_on)
                    _applets_up_keypress(STBK_PAGE_UP);
                break;
            case STBK_PAGE_DOWN:
                if(!pop_msg_on)
                    _applets_down_keypress(STBK_PAGE_DOWN);
                break;
            case STBK_LEFT:
                if(!pop_msg_on)
                    _applets_left_keypress();
                break;
            case STBK_RIGHT:
                if(!pop_msg_on)
                    _applets_right_keypress();
                break;
#if APPLETS_INSTALL_SUPPORT
            case STBK_RED:
                if(!pop_msg_on)
                    applets_create_setting_menu();
                break;
            case STBK_GREEN:
                if(!pop_msg_on)
                    app_applets_download_install_unstall();
                break;
#endif
#if APPLETS_FAV_SUPPORT
            case STBK_YELLOW:
                if(!pop_msg_on)
                    app_applets_fav_node();
                break;
#endif
            case STBK_BLUE:
                if(!pop_msg_on)
                    app_applets_list_update();
                break;
#ifdef APPLETS_SUPPORT_USB
            case STBK_INFO:
                if(!pop_msg_on)
                    app_applets_from_usb();
                break;
#endif
            default:
                break;
        }
    }
    return EVENT_TRANSFER_STOP;
}

static void _group_list_up_keypress(void)
{
    if(applets_focus_info.group_num >1)
    {
        applets_focus_info.sel_group -= 1;
        if(applets_focus_info.sel_group < 0)
        {
            applets_focus_info.sel_group = applets_focus_info.group_num - 1;
        }
        _applets_group_list_focus();
    }
}

static void _group_list_down_keypress(void)
{
    if(applets_focus_info.group_num > 1)
    {
        applets_focus_info.sel_group += 1;
        if( applets_focus_info.sel_group > ( applets_focus_info.group_num- 1))
        {
            applets_focus_info.sel_group = 0;
        }
        _applets_group_list_focus();
    }
}
static void _group_list_ok_keypress(void)
{
    if(applets_focus_info.group_num > 1)
    {
        app_applets_show_popup_msg(STR_ID_WAITING, 0);
        _applets_clean_all_item();
        _group_get_item_data(applets_focus_info.sel_group);
        _applets_group_list_active();
        _update_focus_info_item_init();
        app_applets_hide_popup_msg();
    }
}


SIGNAL_HANDLER int app_applets_group_list_keypress(GuiWidget *widget, void *usrdata)
{
    int ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = NULL;

    event = (GUI_Event *)usrdata;
    if(applets_focus_info.focus_area == FOCUSAREA_TABLE_TTEM)
    {
        GUI_SendEvent(WND_APPLETS, event);
        return ret;
    }

    if(GUI_KEYDOWN == event->type)
    {
        switch(find_virtualkey_ex(event->key.scancode,event->key.sym))
        {
            case VK_BOOK_TRIGGER:
                GUI_EndDialog("after wnd_full_screen");
                break;
            case STBK_EXIT:
            case STBK_MENU:
                app_applets_close_menu();
                break;
            case STBK_OK:
                if(!pop_msg_on)
                    _group_list_ok_keypress();
                break;
            case STBK_RIGHT:
                if((!pop_msg_on)&&(applets_focus_info.item_num > 0))
                {
                    applets_focus_info.focus_area = FOCUSAREA_TABLE_TTEM;
                    GUI_SetFocusWidget(BTN_RIGHT);
                    _applets_focus_item(applets_focus_info.sel_item);
                    _applets_update_icon();
                    applets_app_update_start();
                }
                break;
            case STBK_UP:
                _group_list_up_keypress();
                break;
            case STBK_DOWN:
                _group_list_down_keypress();
                break;
            case STBK_BLUE:
            case STBK_RED:
                ret = EVENT_TRANSFER_KEEPON;
                break;
            default:
                break;
        }
    }
    return ret;
}

SIGNAL_HANDLER int app_applets_group_list_get_total(GuiWidget *widget, void *usrdata)
{
    return applets_focus_info.group_num;
}

SIGNAL_HANDLER int app_applets_group_list_get_data(GuiWidget *widget, void *usrdata)
{
    ListItemPara* item = NULL;

    item = (ListItemPara*)usrdata;
    if(NULL == item)
        return EVENT_TRANSFER_STOP;
    if(0 > item->sel)
        return EVENT_TRANSFER_STOP;
    if(item->sel >= applets_focus_info.group_num)
        return EVENT_TRANSFER_STOP;
    item->image = NULL;
    item->string = applets_category_str[item->sel];

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_applets_group_list_change(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_applets_lost_focus(GuiWidget *widget, void *usrdata)
{
	app_applets_data_timer_stop();
	app_applets_pic_update_timer_stop();
	APPLETS_INFO("<=====\n");
	return EVENT_TRANSFER_STOP;
}
SIGNAL_HANDLER int app_applets_got_focus(GuiWidget *widget, void *usrdata)
{
    applets_data_timer_start();
    app_applets_pic_update_timer_reset();
    APPLETS_INFO("=====>\n");
    return EVENT_TRANSFER_STOP;
}
void app_applets_set_logout(void)
{
	char* focus_win = NULL;
	gx_applets_service_stop();
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
	applets_auto_update_list_count = 0;
	if (applets_auto_update_list_running > 0)
	{
		applets_auto_update_list_running = 2;
	}
	if (applets_get_list_status == APPLETS_GET_LIST_SUCCESS)
	{
		if (IF_STATE_CONNECTED != app_network_get_cur_dev_and_state(NULL))
		{
			applets_get_list_status = APPLETS_GET_LIST_FAILED;
		}
	}
#endif
	focus_win = (char*)GUI_GetFocusWindow();
	if(!focus_win)
		return;
	if(0 == strcasecmp(focus_win, WND_APPLETS))
	{
		app_applets_close_menu();
	}
}
int app_applets_msg_proc(GxMessage *msg)
{
	if(msg == NULL)
		return EVENT_TRANSFER_STOP;

	switch(msg->msg_id)
	{
		case GXMSG_NETWORK_STATE_REPORT:
			{
				GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
				if(dev_state->dev_state == IF_STATE_CONNECTED)
				{
					gx_applets_service_start();
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
					app_applets_auto_update_list_start();
#endif
				}
				else if((dev_state->dev_state !=IF_STATE_START)&&(dev_state->dev_state !=IF_STATE_CONNECTING))
				{
					app_applets_set_logout();
				}
			}
		break;
#if APPLETS_INSTALL_SUPPORT
		case GXMSG_HOTPLUG_IN:
			{
				if(applets_setting&&(applets_setting->install_path == INSTALL_PATH_USB))
				{
					char* focus_win = NULL;
					int sel = -1;
					HotplugPartitionList *partition_list = NULL;
					focus_win = (char*)GUI_GetFocusWindow();
					if(!focus_win)
						return EVENT_TRANSFER_STOP;
					if(0 == strcasecmp(focus_win, WND_APPLETS))
					{
						partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
						if(partition_list&&(partition_list->partition_num > 0))
						{
							GUI_GetProperty(WND_LIST_GROUP, "active", &sel);
							applets_focus_info.sel_group = sel;
							GUI_SetProperty(WND_LIST_GROUP, "select", &sel);
							applets_setting_all();
						}
					}
				}
			}
			break;
		case GXMSG_HOTPLUG_OUT:
			{
				if(applets_setting&&(applets_setting->install_path == INSTALL_PATH_USB))
				{
					char* focus_win = NULL;
					int sel = -1;
					HotplugPartitionList *partition_list = NULL;
					focus_win = (char*)GUI_GetFocusWindow();
					if(!focus_win)
						return EVENT_TRANSFER_STOP;
					if(0 == strcasecmp(focus_win, WND_APPLETS))
					{
						partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
						if(!partition_list||(partition_list->partition_num <=0))
						{
							GUI_GetProperty(WND_LIST_GROUP, "active", &sel);
							applets_focus_info.sel_group = sel;
							GUI_SetProperty(WND_LIST_GROUP, "select", &sel);
							applets_setting_all();
							app_applets_show_popup_msg(STR_ID_NO_DEVICE, 1000);
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

void app_create_applets_menu(void)
{
	app_netapps_app_init();
    if(GUI_CheckDialog(WND_APPLETS) == GXCORE_ERROR)
    {
        GUI_CreateDialog(WND_APPLETS);
    }
}

void app_create_applets_weather_menu(void)
{
#if APPLETS_AUTO_UPDATE_LIST_SUPPORT
    app_applets_mainmenu_entry("weather");
#endif
}

#endif
