#include "app_config.h"
#include <gxcore.h>
#include <gx_apps.h>
#include "app_iptv_list_api.h"
#include "app_module.h"
#include "app_windows.h"

#ifdef LINUX_OS
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>
#include <net/if_arp.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "net/if.h"
#include <net/if_var.h>
#include <net/if_arp.h>
#endif
#include "channel_common.h"
#include "app_default_params.h"
#include <signal.h>
#include "app_netaudio.h"


#if NET_AUDIO_SUPPORT
#define NETAUDIO_TSBUFF_NUM 60


#define MAX_NETAUDIO_URL_LEN 256
#define MAX_NETAUDIO_NAME_LEN 64

typedef struct _NetaudioItemDefault
{
    char  name[MAX_NETAUDIO_NAME_LEN];
    char  url[MAX_NETAUDIO_URL_LEN];
}netaudio_data_t;

typedef struct netaudioctrol
{
    int netaudio_current_play_index;
    int netaudio_channel;
    netaudio_data_t *netaudio_data;
    char dest_url[MAX_NETAUDIO_URL_LEN];
    int netaudio_flag;
    int exit_flag;
    int netaudio_auto_get_data_running;
    int netaudio_auto_downloading;
}netaudioctrol;

static netaudioctrol s_ctrol = {0};
static int s_tsbuff_time = 0;
extern void app_netaudio_set_flag(int flag);
extern status_t app_netaudio_start_play(char *url, int64_t start);
extern status_t app_system_vol_set(GxMsgProperty_PlayerAudioVolume val);
typedef void(* netaudio_get_data_thread_func) (void);
int app_copy_netaudio_group_data(IptvListClass *netaudio);
extern IptvListClass* app_iptv_api_get_list_from_file(const char *src_file);
static void app_net_audio_play_status(GxMessage* msg);
#endif

int app_netaudio_netaudio_play_stop(void)
{
#if NET_AUDIO_SUPPORT
	if (s_ctrol.exit_flag == 0 && s_ctrol.netaudio_flag == 1)
	{
		GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
		GxPlayer_MediaStop(PLAYER_FOR_IPTV);
		s_ctrol.netaudio_flag = 0;
	}
    s_ctrol.exit_flag = 0;
#endif
	return 0;
}

int app_netaudio_check_flag(void)
{
#if NET_AUDIO_SUPPORT
	return s_ctrol.netaudio_flag;
#endif
    return 0;
}

int app_netaudio_auto_msg_proc(GxMessage *msg)
{
#if NET_AUDIO_SUPPORT
    if(msg == NULL)
    {
        return EVENT_TRANSFER_STOP;
    }
    PopDlg pop;
    char *s = NULL;
    GUI_GetProperty("txt_common_select_title1", "string", &s);

    switch(msg->msg_id)
    {
        case GXMSG_NETWORK_STATE_REPORT:
            {
                if(s_ctrol.netaudio_flag == 0)
                    return EVENT_TRANSFER_STOP;
                GxMsgProperty_NetDevState *dev_state = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_NetDevState);
                if(dev_state->dev_state == IF_STATE_CONNECTED)
                {
                    app_netaudio_auto_get_data_start(NULL);
                }
                else
                {
                    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_TIP))
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_LINK_NETWORK;
                        pop.timeout_sec = 5;
                        popdlg_create(&pop);
                        app_update_pop_dlg(WND_POP_BOOK);
                        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT) && (strcmp(s,STR_ID_AUDIO) == 0))
                            GUI_EndDialog("after wnd_full_screen");
                    }
                }
                break;
            }
        case GXMSG_NETWORK_PLUG_OUT:
            {
                if(s_ctrol.netaudio_flag == 1)
                {
                    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
                        GUI_EndDialog("after wnd_epg");
                    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
                        GUI_EndDialog("after wnd_channel_edit");
                    else
                        GUI_EndDialog("after wnd_full_screen");
                    if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_TIP))
                    {
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_NO_BTN;
                        pop.format = POP_FORMAT_DLG;
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.str = STR_ID_LINK_NETWORK;
                        pop.timeout_sec = 5;
                        popdlg_create(&pop);
                        app_update_pop_dlg(WND_POP_BOOK);
                    }
                    app_netaudio_nomal_replay();
                }
                break;
            }
        case GXMSG_PLAYER_STATUS_REPORT:
            {
                if(s_ctrol.netaudio_flag == 0)
                    return EVENT_TRANSFER_STOP;
                app_net_audio_play_status(msg);
                break;
            }
        default:
            break;
    }
#endif
    return EVENT_TRANSFER_STOP;
}

#if NET_AUDIO_SUPPORT
static void app_net_audio_play_status(GxMessage* msg)
{
    GxMsgProperty_PlayerStatusReport* player_status = NULL;
    player_status = (GxMsgProperty_PlayerStatusReport*)GxBus_GetMsgPropertyPtr(msg,GxMsgProperty_PlayerStatusReport);
    PopDlg pop;


    if((player_status == NULL) || (strcmp((char*)(player_status->player), PLAYER_FOR_IPTV) != 0))
    {
        return;
    }

    switch(player_status->status)
    {
        case PLAYER_STATUS_ERROR:
            GUI_EndDialog("after wnd_full_screen");
            if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_TIP))
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_LINK_NETWORK;
                pop.timeout_sec = 5;
                popdlg_create(&pop);
                app_update_pop_dlg(WND_POP_BOOK);
                char *s = NULL;
                GUI_GetProperty("txt_common_select_title1", "string", &s);
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT) &&  (strcmp(s,STR_ID_AUDIO) == 0))
                    GUI_EndDialog("after wnd_full_screen");
            }
            app_netaudio_nomal_replay();
            app_log_min("\n[%s]%d NETAUDIO PLAYER_STATUS_ERROR, %d\n", __func__, __LINE__, player_status->error);
            break;
        case PLAYER_STATUS_PLAY_END:
            GUI_EndDialog("after wnd_full_screen");
            if(GXCORE_SUCCESS != GUI_CheckDialog(WND_POP_TIP))
            {
                memset(&pop, 0, sizeof(PopDlg));
                pop.type = POP_TYPE_NO_BTN;
                pop.format = POP_FORMAT_DLG;
                pop.mode = POP_MODE_UNBLOCK;
                pop.str = STR_ID_LINK_NETWORK;
                pop.timeout_sec = 5;
                popdlg_create(&pop);
                app_update_pop_dlg(WND_POP_BOOK);
                char *s = NULL;
                GUI_GetProperty("txt_common_select_title1", "string", &s);
                if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT) &&  (strcmp(s,STR_ID_AUDIO) == 0))
                    GUI_EndDialog("after wnd_full_screen");
            }
            app_netaudio_nomal_replay();
            app_log_min("\n[%s]%d NET AUDIO PLAYER_STATUS_PLAY_END\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_PLAY_RUNNING:
            app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_RUNNING\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_PLAY_START:
            app_log_flow("\n[%s]%d PLAYER_STATUS_PLAY_START\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_STOPPED:
            app_log_flow("\n[%s]%d PLAYER_STATUS_STOPPED\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_START_FILL_BUF:
            app_log_min("\n[%s]%d PLAYER_STATUS_START_FILL_BUF\n", __func__, __LINE__);
            break;

        case PLAYER_STATUS_END_FILL_BUF:
            app_log_min("\n[%s]%d PLAYER_STATUS_END_FILL_BUF\n", __func__, __LINE__);
            break;
        default:
            break;
    }
}


int app_netaudio_tsbuff_time_init(void)
{
    uint32_t mem_size = 0;
    struct Gx_Mem_Info info = {0};

    GxCore_MemInfoGet("mem", &info);
    mem_size = info.size;
    if (mem_size >= 100 * SIZE_UNIT_M)
    {
        s_tsbuff_time = 400;
    }
    else if (mem_size >= 50 * SIZE_UNIT_M)
    {
        s_tsbuff_time = 200;
    }
    else
    {
        s_tsbuff_time = 100;
    }
    return 0;
}


status_t app_netaudio_start_play(char *url, int64_t start)
{
    GxMsgProperty_PlayerPlay iptv_play = {0};
    GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);

    if(url == NULL)
    {
        return GXCORE_ERROR;
    }
    strlcpy(iptv_play.url, url, sizeof(iptv_play.url));
    iptv_play.player = PLAYER_FOR_IPTV;
    iptv_play.start = start;
    app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&iptv_play));
    return GXCORE_SUCCESS;
}

void app_netaudio_remove_nomalplay_audio(char *url)
{
    int len = 0;
    int len1 = 0;
	char *p1 = NULL;
	char *p2 = NULL;

	p1 = strstr(url,"apid");
	p2 = strstr(url,"pcrpid");
    len1 = strlen(p1);
	len = strlen(p2);
	if(p1&&p2)
	{
		memmove(p1,p2,len);
	}
    memset(p1+len,0,(len1-len));
	p1 = strstr(url,"acodec");
	p2 = strstr(url,"tuner");
	len = strlen(p2);
    len1 = strlen(p1);
	if(p1&&p2)
	{
		memmove(p1,p2,len);
	}
    memset(p1+len,0,(len1-len));
}

void app_netaudio_play_video(char *url)
{
	if(NULL == s_ctrol.netaudio_data || s_ctrol.netaudio_channel <=0\
		||0 == s_ctrol.netaudio_flag)
	{
		GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
		GxPlayer_SetVideoSyncMode(1);
		return ;
	}
	GxPlayer_SetVideoSyncMode(0);
	app_netaudio_remove_nomalplay_audio(url);
}


void app_netaudio_set_flag(int flag)
{
	s_ctrol.netaudio_flag = flag;
}

int  app_netaudio_nomal_replay(void)
{
    if(s_ctrol.netaudio_flag == 0)
        return -1;
	if(s_ctrol.netaudio_data == NULL || s_ctrol.netaudio_channel <= 0)
	{
        return -1;
	}
    GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
    s_ctrol.netaudio_flag = 0;
	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    s_ctrol.exit_flag = 1;
    g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
	g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
	return 0;
}

int  app_netaudio_tsbuff_replay(void)
{
    if(s_ctrol.netaudio_flag == 0)
        return -1;
    if(g_AppFullArb.tip != FULL_STATE_DUMMY)
        return -1;
	g_AppFullArb.state.pause = STATE_OFF;
	g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    s_ctrol.exit_flag = 1;
    g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
	g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
	return 0;
}

int app_get_netaudio_tatal_num(void)
{
    app_log_min("netaudio_channel = %d\n",s_ctrol.netaudio_channel);
    return s_ctrol.netaudio_channel;
}

static void netaudio_get_data_thread_body(void* arg)
{
	netaudio_get_data_thread_func func = NULL;
	GxCore_ThreadDetach();
	func = (netaudio_get_data_thread_func)arg;
	if(func)
	{
		func();
	}
}

static void create_netaudio_thread(netaudio_get_data_thread_func func)
{
	static int thread_id = 0;
	GxCore_ThreadCreate("iptv_get_data", &thread_id, netaudio_get_data_thread_body,
		(void *)func, 64 * 1024, GXOS_DEFAULT_PRIORITY);
}

int app_netaudio_get_download_data_state(void)
{
	return s_ctrol.netaudio_auto_downloading;
}

static void app_netaudio_get_data_func(void)
{
    IptvListClass *netaudio_list = NULL;
	int ret = 0;
    gx_curl_http_options_t opt = {0};
    gx_curl_data_t curldata = {0};
    FILE *fp = NULL;
    ret = gxapps_curl_httpfunc(s_ctrol.dest_url,GX_CURL_HTTP_GET,&opt,&curldata);
    if (ret == GXAPPS_SUCCESS)
    {
        fp = fopen("/mnt/iqgroupdata.xml", "w+");
        if (fp)
        {
            fwrite(curldata.buffer, curldata.used_size, 1, fp);
            fclose(fp);
            netaudio_list = app_iptv_api_get_list_from_file("/mnt/iqgroupdata.xml");
            unlink("/mnt/iqgroupdata.xml");
        }
    }
    gxapps_curl_free(&curldata);
	ret = app_copy_netaudio_group_data(netaudio_list);
	s_ctrol.netaudio_auto_downloading = 0;
    GxCore_ThreadDelay(500);
    s_ctrol.netaudio_auto_get_data_running = 0;

}

void app_netaudio_auto_get_data_start(const char* url)
{
    if(url != NULL)
    {
        memset(s_ctrol.dest_url,0,MAX_NETAUDIO_URL_LEN);
        memcpy(s_ctrol.dest_url,url,MAX_NETAUDIO_URL_LEN);
    }

	if(s_ctrol.netaudio_auto_get_data_running == 0)
	{
		s_ctrol.netaudio_auto_get_data_running = 1;
		s_ctrol.netaudio_auto_downloading = 1;
		create_netaudio_thread(app_netaudio_get_data_func);
	}
	else
	{
		s_ctrol.netaudio_auto_get_data_running = 1;
	}
}


static void app_free_netaudio(void)
{
    if(s_ctrol.netaudio_data != NULL)
    {
        GxCore_Free(s_ctrol.netaudio_data);
    }
}

static int app_malloc_netaudio(unsigned int num)
{
    app_free_netaudio();
    if(num == 0)
    {
        return -1;
    }
    s_ctrol.netaudio_channel = num;
    s_ctrol.netaudio_data    = GxCore_Malloc(num*sizeof(netaudio_data_t));
    if(s_ctrol.netaudio_data == NULL)
    {
        app_log_error("app_malloc_sat_to_iptv malloc fialed\n");
        s_ctrol.netaudio_channel = 0;
        return -1;
    }
    memset(s_ctrol.netaudio_data,0,num*sizeof(netaudio_data_t));
    return 0;
}

int app_copy_netaudio_group_data(IptvListClass * netaudio)
{
#define NETAUDIO_MAX_NUM 50
    if(netaudio == NULL)
    {
        return -1;
    }
    if(netaudio->iptv_item_num > NETAUDIO_MAX_NUM)
    {
        netaudio->iptv_item_num = NETAUDIO_MAX_NUM;
    }
    if(netaudio->iptv_item_num <= 0)
    {
        return -1;
    }
    if(0 == app_malloc_netaudio(netaudio->iptv_item_num))
    {
        int i = 0;
        for(i = 0; i < netaudio->iptv_item_num; i++)
        {
            memcpy(s_ctrol.netaudio_data[i].name,netaudio->iptv_item_list[i].iptv_key,64);
            memcpy(s_ctrol.netaudio_data[i].url,netaudio->iptv_item_list[i].iptv_value,256);

        }
    }
    else
    {
        app_log_error("malloc failed\n");
        return -1;
    }
    s_ctrol.netaudio_current_play_index = 0;
    return 0;
}

void app_netaudio_list_update(char *path)
{
extern char *net_m3u_convert_xml(char *File_path, char* Dest_file);
extern char *net_xml_convert_xml(char *File_path, char* Dest_file);
#ifdef ECOS_OS
#define NET_AUDIO_PATH  "/mnt/net_audio_xml.xml"
#else
#define NET_AUDIO_PATH  "/tmp/net_audio_xml.xml"
#endif
    IptvListClass * netaudio = NULL;
    int str_len = 0;
    str_len = strlen(path);
    if (strcasecmp(path+str_len-4,".m3u") == 0 || strcasecmp(path+str_len-5,".m3u8") == 0)
        net_m3u_convert_xml(path, NET_AUDIO_PATH);
    else
        net_xml_convert_xml(path, NET_AUDIO_PATH);
    netaudio = app_iptv_api_get_list_from_file(NET_AUDIO_PATH);
    app_copy_netaudio_group_data(netaudio);
}


int app_get_netaudio_url(unsigned int sel, char *url)
{
    if(s_ctrol.netaudio_data == NULL)
    {
        app_log_error("%d NULL\n",__LINE__);
        return -1;
    }
    if(sel < s_ctrol.netaudio_channel)
    {
        strcpy(url , s_ctrol.netaudio_data[sel].url);
        return 0;
    }
    return -1;
}

char * app_netaudio_get_name(unsigned int sel)
{
    if((s_ctrol.netaudio_data == NULL)||(sel < 0))
    {
        app_log_error("%d NULL\n",__LINE__);
        return NULL;
    }
    if(sel < s_ctrol.netaudio_channel)
    {
        return s_ctrol.netaudio_data[sel].name;
    }
    else
    {
        return NULL;
    }
}

int app_get_netaudio_current_play_index(void)
{
    return s_ctrol.netaudio_current_play_index;
}

int app_netaudio_online_data_play(int sel)
{
    if((s_ctrol.netaudio_data == NULL)
            ||(sel < 0)
            ||(sel >= s_ctrol.netaudio_channel))
    {
        return -1;
    }
    if (s_ctrol.netaudio_data[sel].url == NULL)
    {
        return -1;
    }
    app_netaudio_set_flag(1);
    if(g_AppFullArb.tip != FULL_STATE_DUMMY)
        return -1;
    s_ctrol.netaudio_current_play_index = sel;
    GxPlayer_MediaExitPlay(PLAYER_FOR_IPTV);
    g_AppFullArb.state.pause = STATE_OFF;
    g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
    s_ctrol.exit_flag = 1;
    g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
    g_AppPlayOps.program_play(PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
    app_netaudio_start_play(s_ctrol.netaudio_data[sel].url,0);
    return 0;
}

int  app_netaudio_online_remove_nomalplay_audio(void)
{
    if((s_ctrol.netaudio_data == NULL)
            ||(s_ctrol.netaudio_current_play_index < 0)
            ||(s_ctrol.netaudio_current_play_index >= s_ctrol.netaudio_channel))
    {
        return -1;
    }
    if (s_ctrol.netaudio_data[s_ctrol.netaudio_current_play_index].url == NULL)
    {
        return -1;
    }
    app_netaudio_play_video(s_ctrol.netaudio_data[s_ctrol.netaudio_current_play_index].url);
    return 0;
}

int app_get_netaudio_tsbuff_index(void)
{
    int init_value = 0;
    GxBus_ConfigGetInt(VIDEO_DELAY_TIME, &init_value, VIDEO_DELAY_TIME_VALUE);
	return init_value;
}

void app_set_netaudio_tsbuff_time(int tsbuffTime)
{
    if(tsbuffTime >= NETAUDIO_TSBUFF_NUM)
        tsbuffTime=0;
    GxBus_ConfigSetInt(VIDEO_DELAY_TIME, tsbuffTime);
    return;
}

int app_get_netaudio_max_tsbuff_time(void)
{
    return NETAUDIO_TSBUFF_NUM;
}

int app_get_netaudio_tsbuff_time(int tsbuffTime)
{
    return s_tsbuff_time*tsbuffTime;
}
#endif

