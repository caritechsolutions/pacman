#include "../../include/vod_in_common_def.h"
#include "../../include/vod_in_typedef.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_common_func.h"
#include "gx_common.h"
#include "wfd_util.h"

static wfd_client_t* wfd_client = NULL;
static wfd_callback_func_t wfd_callback_func = 0;

static int wfd_thread_quit_flag = 0;
static unsigned int wfd_thread_id = 0;
static int wfd_asyn_resp = -1;

#define WFD_SERVER "stagefright/1.2 (Linux;Android 4.3)"
#define WFD_PUBLIC "org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER"
#define WFD_VIDEO_FORMATS "wfd_video_formats: 39 00 02 02 0001FFFF 1FFFFFFF 00000000 00 0000 0000 00 none none\r\nwfd_audio_codecs: AAC 00000001 00\r\nwfd_client_rtp_ports: RTP/AVP/UDP;unicast 50000 0 mode=play"
#define WFD_TRANS_PORT "RTP/AVP/UDP;unicast;client_port=50000"

static int  wfd_get_asyn_resp(void)
{
	int ret = -1;
	int flag, time;

	flag = 0;
	time = 0;
	while (time < wfd_client->timeout + 200)
	{
		if (vod_porting_sem_wait(wfd_client->resp_sem, 200) == 0)
		{
			flag = 1;
			break;
		}
		time += 200;
//		wfd_callback_func(RTSP_EVENT_BLOCK, 0);
	}

	if (flag == 0)
	{
		wfd_client->client_seq++;
		gxlogd("rtsp wait response timeout\n");
		return -1;
	}

	vod_porting_sem_wait(wfd_client->resp_mutex, 0xffffffff);
	ret = wfd_asyn_resp;
	wfd_asyn_resp = -1;
	vod_porting_sem_send(wfd_client->resp_mutex);

	return ret;
}

static int wfd_parse_url (char* url, char* host, int* port)
{
	char *str, *slash, *colon;
	int hostlen, port_tmp;

	str = url;
	port_tmp = 0;

	if (strncmp("wfd://", url, strlen("wfd://")) == 0)
		str += strlen("wfd://");
	else if (strncmp("rtsp://", url, strlen("rtsp://")) == 0)
		str += strlen("rtsp://");
	else
		return -1;

	slash = strchr(str, '/');
	colon = strchr(str, ':');

	if (slash != 0 || colon != 0)
	{
		if (colon != 0 && (colon < slash || slash == 0))
		{
			hostlen = colon - str;
			colon++;
			port_tmp = 0;
			while (isdigit(*colon))
			{
				port_tmp *= 10;
				port_tmp += *colon - '0';
				colon++;
			}
			if (port_tmp == 0 || (*colon != '/' && *colon != '\0'))
				return -1;
		}
		else
			hostlen = slash - str;

		if (hostlen == 0)
			return -1;

		memcpy(host, str, hostlen);
		host[hostlen] = '\0';
	}
	else
	{
		if (*str == '\0')
			return -1;

		strcpy(host, str);
	}

	if (port_tmp == 0)
		port_tmp = 554;

	if(port)
		*port = port_tmp;

	return 0;
}

static void wfd_free_client (wfd_client_t* client)
{
	if (client)
	{
		if (client->session)
			str_free(client->session);
		if (client->recvbuf)
			xfree(client->recvbuf);
		if (client->sendbuf)
			xfree(client->sendbuf);
		if (client->url)
			str_free(client->url);
		if(client->server_url)
			str_free(client->server_url);
		if (client->sock)
			vod_porting_socket_close(client->sock);
		if (client->resp_mutex)
			vod_porting_sem_delete(client->resp_mutex);
		if (client->resp_sem)
			vod_porting_sem_delete(client->resp_sem);

		xfree(client);
	}
}

static wfd_client_t *wfd_create_client (char *url, char* host, int port, int timeout)
{
	wfd_client_t *client;
	char* buf;
	int sock;
	unsigned int addr;

	client = (wfd_client_t*)xmalloc(sizeof(wfd_client_t));
	if (client == 0)
	{
		gxlogd("[%s-%d]: malloc rtsp client fail\n", __FUNCTION__, __LINE__);
		return 0;
	}

	memset(client, 0, sizeof(wfd_client_t));

	buf = (char*)xmalloc(WFD_RECV_BUFFER_SIZE);
	if (buf == 0)
	{
		gxlogd("[%s-%d]: rtsp malloc fail\n", __FUNCTION__, __LINE__);
		wfd_free_client(client);
		return 0;
	}
	client->recvbuf = buf;

	buf = (char*)xmalloc(WFD_SEND_BUFFER_SIZE);
	if (buf == 0)
	{
		gxlogd("[%s-%d]: rtsp malloc fail\n", __FUNCTION__, __LINE__);
		wfd_free_client(client);
		return 0;
	}
	client->sendbuf = buf;

	if (vod_porting_sem_create(0, &client->resp_sem) < 0)
	{
		gxlogd("[%s-%d]: rtsp create sem fail\n", __FUNCTION__, __LINE__);
		wfd_free_client(client);
		return 0;
	}

	if (vod_porting_sem_create(1, &client->resp_mutex) < 0)
	{
		gxlogd("[%s-%d]: rtsp create mutex fail\n", __FUNCTION__, __LINE__);
		wfd_free_client(client);
		return 0;
	}

	sock = vod_porting_tcp_socket_create();
	if (sock <= 0)
	{
		GxVod_debug("create rtsp socket fail\n");
		wfd_free_client(client);
		return 0;
	}
	client->sock = sock;

	addr = vod_porting_server_name_to_addr(host);
	if (addr == 0)
	{
		GxVod_debug("rtsp socket addr err\n");
		wfd_free_client(client);
		return 0;
	}

	if (vod_porting_socket_connect(sock, port, addr) < 0)
	{
		GxVod_debug("rtsp connect socket fail\n");
		wfd_free_client(client);
		return 0;
	}

	client->timeout = timeout;
	client->client_seq = 2;
	client->server_seq = 1;
	client->url = str_save(url);
	client->server_url = NULL;

	return client;
}

static void wfd_thread(void* param)
{
	int ret;
	int is_server_resp;

	while (!wfd_thread_quit_flag)
	{
		is_server_resp = -1;
		ret = wfd_recv_common_response(wfd_client, &is_server_resp);
		if (ret != 0){
			continue;
		}else{
			if (is_server_resp == 0)
			{
				vod_porting_sem_wait(wfd_client->resp_mutex, 0xffffffff);
				wfd_asyn_resp = 1;
				vod_porting_sem_send(wfd_client->resp_mutex);
				vod_porting_sem_send(wfd_client->resp_sem);
			}else{
				wfd_send_server_cmd(wfd_client, NULL);
			}
		}
	}
	gxlogd("[%s-%d]: wfd thread exit\n", __FUNCTION__, __LINE__);
}

static void create_wfd_thread(void)
{
	wfd_thread_quit_flag = 0;
	vod_porting_task_create((void(*)(void*))wfd_thread, 0, 2*1024, 9, &wfd_thread_id, (const unsigned char*)"wfd_client");
}

static void destroy_wfd_thread(void)
{
	if (wfd_thread_id)
	{
		wfd_thread_quit_flag = 1;
		vod_porting_task_waitdel(wfd_thread_id, 2000);
		wfd_thread_id = 0;
	}
}

int wfd_rtsp_open(char* url, char* udp_host, int* udp_port, int timeout, wfd_callback_func_t func)
{
	int ret, port;
	char host[64];
	wfd_servercmd_t server_cmd;
	wfd_clientcmd_t client_cmd;
	wfd_getparam_t get_parameter;
	wfd_setparam_t set_parameter;

	if (wfd_client != NULL)
	{
		gxlogd("[%s-%d]: client is not null\n", __FUNCTION__, __LINE__);
		return -1;
	}

	wfd_callback_func = func;

	ret = wfd_parse_url(url, host, &port);
	if (ret != 0)
	{
		gxlogd("[%s-%d]: url format error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	gxlogd("server ip %s, port %d\n", host, port);

	wfd_client = wfd_create_client(url, host, port, timeout);
	if (wfd_client == 0)
	{
		gxlogd("[%s-%d]: create client error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	//first : server begin is or not
	wfd_recv_server_option_cmd(wfd_client);
	memset(&server_cmd, 0, sizeof(wfd_servercmd_t));
	server_cmd.date = "Sun, 11 Aug 2013 04:41:40 +000";
	server_cmd.server = WFD_SERVER;
	server_cmd.public = WFD_PUBLIC;
	wfd_send_server_cmd(wfd_client, &server_cmd);

	//second : client call server can transfer
	memset(&client_cmd, 0, sizeof(wfd_clientcmd_t));
	client_cmd.title = "OPTIONS";
	client_cmd.url = "*";
	server_cmd.server = WFD_SERVER;
	client_cmd.require = "org.wfa.wfd1.0";
	wfd_send_client_cmd(wfd_client, &client_cmd);
	wfd_recv_client_cmd(wfd_client);

	//third : get server support param
	memset(&get_parameter, 0, sizeof(wfd_getparam_t));
	wfd_recv_server_getparam_cmd(wfd_client, &get_parameter);
	memset(&server_cmd, 0, sizeof(wfd_servercmd_t));
	server_cmd.server = WFD_SERVER;
	server_cmd.content_type = "text/parameters";
	server_cmd.content = WFD_VIDEO_FORMATS;
	wfd_send_server_cmd(wfd_client, &server_cmd);
	*udp_port = wfd_client->client_port = 50000;

	//four : get server set param
	memset(&set_parameter, 0, sizeof(wfd_setparam_t));
	wfd_recv_server_setparam_cmd(wfd_client, &set_parameter);
	memset(&server_cmd, 0, sizeof(wfd_servercmd_t));
	server_cmd.server = WFD_SERVER;
	wfd_send_server_cmd(wfd_client, &server_cmd);
	wfd_client->server_url = str_save(set_parameter.transurl);
	wfd_client->server_port = set_parameter.transport;
	wfd_parse_url(wfd_client->server_url, udp_host, NULL);

	//five : get server setup or not
	memset(&set_parameter, 0, sizeof(wfd_setparam_t));
	wfd_recv_server_setparam_cmd(wfd_client, &set_parameter);
	memset(&server_cmd, 0, sizeof(wfd_servercmd_t));
	server_cmd.server = WFD_SERVER;
	wfd_send_server_cmd(wfd_client, &server_cmd);

	//six : client setup
	memset(&client_cmd, 0, sizeof(wfd_clientcmd_t));
	client_cmd.title = "SETUP";
	client_cmd.url = wfd_client->server_url;
	server_cmd.server = WFD_SERVER;
	client_cmd.transport = WFD_TRANS_PORT;
	wfd_send_client_cmd(wfd_client, &client_cmd);
	wfd_recv_client_cmd(wfd_client);

	//seven : client play
	memset(&client_cmd, 0, sizeof(wfd_clientcmd_t));
	client_cmd.title = "PLAY";
	client_cmd.url = wfd_client->server_url;
	server_cmd.server = WFD_SERVER;
	wfd_send_client_cmd(wfd_client, &client_cmd);
	wfd_recv_client_cmd(wfd_client);

	create_wfd_thread();
	return 0;
}

void wfd_rtsp_close(void)
{
	wfd_clientcmd_t client_cmd;

	if (wfd_client == 0)
		return;

	memset(&client_cmd, 0, sizeof(wfd_clientcmd_t));
	client_cmd.title = "TEARDOWN";
	client_cmd.url = wfd_client->server_url;
	wfd_send_client_cmd(wfd_client, &client_cmd);

	wfd_get_asyn_resp();

	destroy_wfd_thread();
	wfd_free_client(wfd_client);
	wfd_client = 0;
}
