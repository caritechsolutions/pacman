#include "app.h"
#include "app_module.h"
#include "app_windows.h"

#if NETAPPS_SUPPORT
#include "app_iptv_list_service.h"

#define IPTV_SERVICE_NAME    "iptv list service"

static status_t AppIptvServiceInit(handle_t self,int priority_offset);
static void AppIptvServiceDestroy(handle_t self);
static GxMsgStatus AppIptvServiceRecvMsg(handle_t self, GxMessage * Msg);

GxServiceClass app_iptv_service =
{
    IPTV_SERVICE_NAME,
    AppIptvServiceInit,
    AppIptvServiceDestroy,
    AppIptvServiceRecvMsg,
    NULL,
};
#ifdef IPTV_CLOUD_SUPPORT
extern char *m3u_convert_xml(char *File_path);
extern char *xml_convert_xml(char *File_path);
extern IptvListClass* app_iptv_api_get_cloud_list(char *file_path);
extern IptvListClass* app_iptv_api_get_remote_bak_list(void);
extern void app_iptv_api_rm_remote_bak_file(void);
extern int app_iptv_service_update_remote_bak_list(char *file_path);
extern int get_active_item(void);

#define IPTVSLIST_ERVER_XML_FILE	"iptv_server.xml"

#ifdef ECOS_OS
#define IPTV_DEFAULT_SERVER_PATH "/dvb/theme/iptv_server.xml"
#define M3U_PATH  "/mnt/des.m3u"
#define XML_PATH  "/mnt/des1.xml"
#else
#define IPTV_DEFAULT_SERVER_PATH WORK_PATH"theme/iptv_server.xml"
#define M3U_PATH  "/tmp/des.m3u"
#define XML_PATH  "/tmp/des1.xml"
#endif
#define IPTV_USER_SERVER_PATH "/home/gx/iptv_userserver.xml"

#define IPTVLIST_FILE_URL "http://192.168.1.153/iptv.m3u"

static gx_list_t *iptv_addserver = NULL;
static handle_t thiz_get_remote_channels_thread_running = 0;

static void app_iptv_server_usb_add_tip(const char *tip)
{
    const char *focus = NULL;
    focus = GUI_GetFocusWindow();
    if (focus && strcmp(focus, WND_COMMON_VIEW) == 0)
    {
        extern void app_common_view_show_popup_msg(const char *str, int time_out);
        app_common_view_show_popup_msg(tip, 1000);
    }
    else
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_NO_BTN;
        pop.format = POP_FORMAT_DLG;
        pop.mode = POP_MODE_UNBLOCK;
        pop.str = (char*)tip;
        pop.timeout_sec= 3;
        pop.pos.x = 320;
        pop.pos.y = 240;
        popdlg_create(&pop);
    }
}

int app_iptv_server_usb_add(void)
{
#define IPTVSLIST_SERVER_XML_FILE   "iptv_server.xml"
    int i = 0;
    char filepath[32] = {0};
    HotplugPartitionList *partition_list = NULL;
    gx_list_t *addserver = NULL;

    if (check_usb_status() == false)
    {
        app_iptv_server_usb_add_tip(STR_ID_INSERT_USB);
        return -1;
    }

    partition_list = GxHotplugPartitionGet(HOTPLUG_TYPE_USB);
    if (partition_list->partition_num > 0)
    {
        for (i = 0; i < partition_list->partition_num; i++)
        {
            sprintf(filepath, "%s/%s", partition_list->partition[i].partition_entry, IPTVSLIST_SERVER_XML_FILE);
            if (GXCORE_FILE_EXIST == GxCore_FileExists(filepath))
            {
                addserver = gx_get_list_from_file(filepath);
                if (addserver)
                {
                    gx_list_save_to_file(addserver, IPTV_USER_SERVER_PATH);
                    gx_list_free(addserver);
                    addserver = NULL;
                    app_iptv_server_usb_add_tip("Import Success!");
                    return 0;
                }
                else
                {
                    app_iptv_server_usb_add_tip("Add Channel failed!");
                    return -1;
                }
            }
        }
    }
    app_iptv_server_usb_add_tip(STR_ID_IPTV_NO_FIND_SER_XML);

    return -1;
}

static int app_iptv_service_send_remote_list(bool force_send)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = IPTV_LIST_REMOTE;
    iptv_list.iptv_list = app_iptv_api_get_remote_bak_list();

    if (force_send || (iptv_list.iptv_list && iptv_list.iptv_list->iptv_item_num > 0))
    {
        app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);
        return 0;
    }

    if (iptv_list.iptv_list)
    {
        iptv_list_free(iptv_list.iptv_list);
    }
    return -1;
}

static void app_iptv_list_download(char *download_url)
{
    char *xml_path = NULL;
    gx_curl_data_t data;

    FILE *fp_des = NULL;
    char *tmpfilepath = NULL;
    if (download_url == NULL)
        return;
    memset(&data, 0, sizeof(gx_curl_data_t));
    tmpfilepath = download_url;
    if (GXAPPS_SUCCESS == gxapps_curl_download(download_url, &data))
    {
        if ((strncmp(data.buffer, "#EXTM3U", 7) == 0)
            || (strcasecmp(tmpfilepath + strlen(download_url) - 4, ".m3u") == 0)
            || (strcasecmp(tmpfilepath + strlen(download_url) - 4, ".M3U") == 0)
            || (strcasecmp(tmpfilepath + strlen(download_url) - 5, ".m3u8") == 0)
            || (strcasecmp(tmpfilepath + strlen(download_url) - 5, ".M3U8") == 0))
        {
            fp_des = fopen(M3U_PATH, "w+");
            if (fp_des != NULL)
            {
                fwrite(data.buffer, data.used_size, 1, fp_des);
                fclose(fp_des);
                xml_path = m3u_convert_xml(M3U_PATH);
                if (GxCore_FileExists(M3U_PATH) == GXCORE_FILE_EXIST)
                    GxCore_FileDelete(M3U_PATH);
            }
        }
        else
        {
            fp_des = fopen(XML_PATH, "w+");
            if (fp_des != NULL)
            {
                fwrite(data.buffer, data.used_size, 1, fp_des);
                fclose(fp_des);
                xml_path = xml_convert_xml(XML_PATH);
                if (GxCore_FileExists(XML_PATH) == GXCORE_FILE_EXIST)
                    GxCore_FileDelete(XML_PATH);
            }
        }
        app_iptv_service_update_remote_bak_list(xml_path);
    }
    else
    {
        app_log_error("app_iptv_list_download %s failed \n", download_url);
    }

    gxapps_curl_free(&data);
    return;
}

int  _get_remote_channels_total(void)
{
    if (iptv_addserver != NULL)
    {
        return iptv_addserver->item_num;
    }
    else
    {
        return 0;
    }
}

char *_get_remote_server_name(int value)
{
    if (iptv_addserver != NULL && value < iptv_addserver->item_num)
    {
        return iptv_addserver->items[value].key;
    }
    else
    {
        return  NULL;
    }
}

char *_get_remote_server_url(int value)
{
    if (iptv_addserver != NULL && value < iptv_addserver->item_num)
    {
        return iptv_addserver->items[value].value;
    }
    else
    {
        return  NULL;
    }
}

gx_list_t  *_get_remote_iptv_addserver(void)
{
    return iptv_addserver;
}

gx_list_t *_load_remote_iptv_server_list(void)
{
	if (GXCORE_FILE_EXIST == GxCore_FileExists(IPTV_USER_SERVER_PATH))
	   {
		   if ( iptv_addserver != NULL )
		   {
			   gx_list_free(iptv_addserver);
		   }
		   iptv_addserver = gx_get_list_from_file(IPTV_USER_SERVER_PATH);
	   }
	   if (!iptv_addserver)
	   {
		   if (GXCORE_FILE_EXIST == GxCore_FileExists(IPTV_DEFAULT_SERVER_PATH))
		   {
			   iptv_addserver = gx_get_list_from_file(IPTV_DEFAULT_SERVER_PATH);
		   }
		   if (!iptv_addserver)
		   {
			  // iptv_addserver = gx_list_new();
			   return NULL;
		   }
		   gx_list_save_to_file(iptv_addserver, IPTV_USER_SERVER_PATH);
	   }

	   return iptv_addserver;
}

static void _get_remote_channels_proc(void *arg)
{
    char *Iptv_Server = NULL;
    int value = 0;

    GxCore_ThreadDetach();
    thiz_get_remote_channels_thread_running = 1;
    _load_remote_iptv_server_list();

    value = get_active_item();
    if (iptv_addserver != NULL && value < iptv_addserver->item_num)
        Iptv_Server = iptv_addserver->items[value].value;

    if (Iptv_Server != NULL)
    {
        app_iptv_list_download(Iptv_Server);
    }
    app_iptv_service_send_remote_list(true);
    thiz_get_remote_channels_thread_running = 0;
}

static status_t app_iptv_service_get_remote_list(int remote_fastget)
{
    handle_t thread_id = 0;

    if (remote_fastget)
    {
        if (app_iptv_service_send_remote_list(false) == 0)
            return GXCORE_SUCCESS;
    }

    app_iptv_api_rm_remote_bak_file();
    if (thiz_get_remote_channels_thread_running > 0)
    {
        app_log_error("[%s][%d]!!!!!!!!!! busy busy busy!!!!!!!!! \n", __func__, __LINE__);
        return GXCORE_ERROR;
    }
    return GxCore_ThreadCreate("iptv_get_remote_channels", &thread_id, _get_remote_channels_proc,
                               NULL, 64* 1024, GXOS_DEFAULT_PRIORITY);
}

#endif

static status_t app_iptv_service_get_local_list(void)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = IPTV_LIST_LOCAL;
    iptv_list.iptv_list = app_iptv_api_get_local_list();

    app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

    return GXCORE_SUCCESS;
}

static status_t app_iptv_service_get_fav_list(void)
{
    AppMsg_IptvList iptv_list;

    memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
    iptv_list.iptv_source = IPTV_LIST_FAV;
    iptv_list.iptv_list = app_iptv_api_get_fav_list();

    app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);

    return GXCORE_SUCCESS;
}

static status_t app_iptv_service_get_list(IptvListSource list_source, IptvServerType list_server, int remote_fastget)
{
    status_t ret = GXCORE_ERROR;

    switch(list_source)
    {
        case IPTV_LIST_REMOTE:
            if (list_server == IPTV_SERVER_GX)
            {
#ifdef IPTV_CLOUD_SUPPORT
                ret = app_iptv_service_get_remote_list(remote_fastget);
#endif
            }
#ifdef STALKER_SUPPORT
            else if (list_server == IPTV_SERVER_STALKER)
            {
                extern status_t app_stalkeriptv_service_get_list(void);
                ret = app_stalkeriptv_service_get_list();
            }
#endif
#ifdef XTREAM_SUPPORT
            else if (list_server == IPTV_SERVER_XTREAM)
            {
                extern status_t app_xtreamiptv_service_get_list(void);
                ret = app_xtreamiptv_service_get_list();
            }
#endif
#ifdef FREEIPTV_SUPPORT
            else if (list_server == IPTV_SERVER_FREEIPTV)
            {
                extern status_t app_freeiptv_service_get_list(void);
                ret = app_freeiptv_service_get_list();
            }
#endif
            break;

        case IPTV_LIST_LOCAL:
            ret = app_iptv_service_get_local_list();
            break;

        case IPTV_LIST_FAV:
            ret = app_iptv_service_get_fav_list();
            break;

        default:
            {
                AppMsg_IptvList iptv_list;
                memset(&iptv_list, 0, sizeof(AppMsg_IptvList));
                iptv_list.iptv_source = IPTV_LIST_UNKOWN;
                app_send_msg_exec(APPMSG_IPTV_LIST_DONE, &iptv_list);
            }
            break;
    }

    return ret;
}

static status_t AppIptvServiceInit(handle_t self,int priority_offset)
{
    handle_t sch;

    GxBus_MessageRegister(APPMSG_IPTV_GET_LIST, sizeof(AppMsg_IptvList));
    GxBus_MessageRegister(APPMSG_IPTV_LIST_DONE, sizeof(AppMsg_IptvList));
    GxBus_MessageRegister(APPMSG_IPTV_UPDATE_LOCAL_LIST, sizeof(char*));
    GxBus_MessageRegister(APPMSG_IPTV_GET_AD_PIC, sizeof(char*));
    GxBus_MessageRegister(APPMSG_IPTV_AD_PIC_DONE, sizeof(char*));

    GxBus_MessageListen(self, APPMSG_IPTV_GET_LIST);
    GxBus_MessageListen(self, APPMSG_IPTV_UPDATE_LOCAL_LIST);
    GxBus_MessageListen(self, APPMSG_IPTV_GET_AD_PIC);

    sch = GxBus_SchedulerCreate("IptvMsgScheduler", GXBUS_SCHED_MSG,
                                    1024 * 8, GXOS_DEFAULT_PRIORITY+priority_offset);
    GxBus_ServiceLink(self, sch);

    return GXCORE_SUCCESS;
}

static void AppIptvServiceDestroy(handle_t self)
{
    GxBus_MessageUnListen(self, APPMSG_IPTV_GET_LIST);

    GxBus_ServiceUnlink(self);

    return;
}

static GxMsgStatus AppIptvServiceRecvMsg(handle_t self, GxMessage * msg)
{
    GxMsgStatus status = GXMSG_OK;

    switch (msg->msg_id)
    {
        case APPMSG_IPTV_GET_LIST:
        {
            AppMsg_IptvList *iptv_list = NULL;
            iptv_list = GxBus_GetMsgPropertyPtr(msg, AppMsg_IptvList);
            app_iptv_service_get_list(iptv_list->iptv_source, iptv_list->iptv_server, iptv_list->remote_fastget);
        }
        break;

        case APPMSG_IPTV_UPDATE_LOCAL_LIST:
            {
                char **file_path = GxBus_GetMsgPropertyPtr(msg, char*);
                if(file_path != NULL)
                    app_iptv_service_update_local_list(*file_path);
            }
            break;

#ifdef LINUX_OS
        case APPMSG_IPTV_GET_AD_PIC:
            {
            }
            break;
#endif

        default:
            break;
    }

    return status;
}

status_t app_iptv_service_start(void)
{
    extern handle_t g_app_msg_self;
    status_t ret = GXCORE_ERROR;
    if(GxBus_ServiceFindByName(IPTV_SERVICE_NAME) == GXCORE_INVALID_POINTER)
    {
        GxBus_ServiceCreate(&app_iptv_service);
        GxBus_MessageListen(g_app_msg_self, APPMSG_IPTV_LIST_DONE);
        GxBus_MessageListen(g_app_msg_self, APPMSG_IPTV_AD_PIC_DONE);
        ret = GXCORE_SUCCESS;
    }
    return ret;
}

#endif
