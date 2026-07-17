#include "../../include/vod_in_common_def.h"
#include "../../include/vod_in_typedef.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_common_func.h"
#include "gx_common.h"
#include "rtsp.h"
#include "rtsp_util.h"

#define RTSP_OPEN_STATUS 1
#define RTSP_PLAY_STATUS 2
#define RTSP_PAUSE_STATUS 3
#define RTSP_CLOSE_STATUS 4
#define RTSP_SEVER_EOS_STATUS 5
#define RTSP_SEVER_BOS_STATUS 6
#define RTSP_SEVER_CLOSE_STATUS 7
#define RTSP_SERVER_ASYN_STATUS 8

static int rtsp_status = RTSP_OPEN_STATUS;
static rtsp_client_t* rtsp_client = 0;
static coship_rtsp_info_t rtsp_info;
static rtsp_time_info_t rtsp_time_info;
static rtsp_callback_func_t rtsp_callback_func = 0;

static unsigned int rtsp_thread_id = 0;
static int rtsp_thread_quit_flag = 0;
static rtsp_resp_comm_t* asyn_resp = 0;
int coship_get_really_current_pos();

static void free_client (rtsp_client_t* client)
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
		if (client->sock)
			vod_porting_socket_close(client->sock);
		if (client->resp_mutex)
			vod_porting_sem_delete(client->resp_mutex);
		if (client->resp_sem)
			vod_porting_sem_delete(client->resp_sem);

		xfree(client);

	}
}

static void free_comm_resp(rtsp_resp_comm_t* resp)
{
	if (resp)
		xfree(resp);
}

static void rtsp_thread(void* param)
{
	int ret;
	rtsp_resp_comm_t* resp = 0;

	while (!rtsp_thread_quit_flag)
	{
		if (!resp)
		{
			resp = (rtsp_resp_comm_t*)xmalloc(sizeof(rtsp_resp_comm_t));
			if (!resp)
			{
				rtsp_callback_func(RTSP_EVENT_MEMORY_ERR, 0);
				GxVod_debug("rtsp_thread: malloc fail\n");
				continue;
			}
		}

		if(rtsp_info.error_count >= 3){
			rtsp_callback_func(RTSP_EVENT_SERVER_CLOSE, 0);
			vod_porting_socket_close(rtsp_client->sock);
			rtsp_client->sock = -1;
			break;
		}

		memset(resp, 0, sizeof(rtsp_resp_comm_t));
		ret = coship_rtsp_recv_response_comm(rtsp_client, resp, &rtsp_time_info);
		if (ret == RESP_ERR_SOCK){
			rtsp_callback_func(RTSP_EVENT_SERVER_CLOSE, 0);
			vod_porting_socket_close(rtsp_client->sock);
			rtsp_client->sock = -1;
			break;
		}else if(ret == RESP_ERR_SEQUENCE)
			rtsp_info.error_count = 0;
		else if (ret == RESP_ERR_TIMEOUT)
			rtsp_callback_func(RTSP_EVENT_SOCK_TIMEOUT, 0);
		else if (ret == RESP_ERR_MEMORY)
			rtsp_callback_func(RTSP_EVENT_MEMORY_ERR, 0);
		else if (ret == RESP_ERR_PARSE)
			rtsp_callback_func(RTSP_EVENT_INVALID_RESP, 0);
		else
		{
			if (resp->close)
			{
				switch (resp->close)
				{
					case RESP_CLOSE:
						rtsp_status = RTSP_SEVER_CLOSE_STATUS;
						rtsp_callback_func(RTSP_EVENT_CLOSE, 0);
						break;
					case RESP_EOS:
						rtsp_status = RTSP_SEVER_EOS_STATUS;
						rtsp_callback_func(RTSP_EVENT_EOS, 0);
						break;
					case RESP_BOS:
						rtsp_status = RTSP_SEVER_BOS_STATUS;
						rtsp_callback_func(RTSP_EVENT_BOS, 0);
						break;
				}
			}
			else
			{
				if (ret == RESP_ERR_CODE)
					rtsp_callback_func(RTSP_EVENT_ERROR_CODE, resp->retcode);

				vod_porting_sem_wait(rtsp_client->resp_mutex, 0xffffffff);
				if (asyn_resp)
				{
					GxVod_debug("rtsp comm resp have not fetch, seq=%d\n", asyn_resp->seq);
					free_comm_resp(asyn_resp);
					asyn_resp = 0;
				}

				asyn_resp = resp;
				resp = 0;
				vod_porting_sem_send(rtsp_client->resp_mutex);

				vod_porting_sem_send(rtsp_client->resp_sem);
			}
		}
	}
	if (resp)
		free_comm_resp(resp);
	if (asyn_resp)
		free_comm_resp(asyn_resp);

	asyn_resp = 0;
	rtsp_status = RTSP_SEVER_CLOSE_STATUS;
}

static rtsp_resp_comm_t* get_asyn_resp()
{
	int flag, time;
	rtsp_resp_comm_t* ret = 0;

	flag = 0;
	time = 0;
	while (time < rtsp_client->timeout + 200)
	{
		if (vod_porting_sem_wait(rtsp_client->resp_sem, 200) == 0)
		{
			flag = 1;
			break;
		}
		time += 200;
		rtsp_callback_func(RTSP_EVENT_BLOCK, 0);
	}

	if (flag == 0)
	{
		rtsp_client->seq++;
		rtsp_callback_func(RTSP_EVENT_SOCK_ERR, 1);
		GxVod_debug("rtsp wait response timeout\n");
		return 0;
	}

	vod_porting_sem_wait(rtsp_client->resp_mutex, 0xffffffff);
	ret = asyn_resp;
	asyn_resp = 0;
	vod_porting_sem_send(rtsp_client->resp_mutex);
	return ret;
}

static void create_rtsp_thread()
{
	rtsp_thread_quit_flag = 0;
	vod_porting_task_create((void(*)(void*))rtsp_thread, 0, 10240, 9, &rtsp_thread_id, (const unsigned char*)"rtsp_client");
}

static void destroy_rtsp_thread()
{
	if (rtsp_thread_id)
	{
		rtsp_thread_quit_flag = 1;
		vod_porting_task_waitdel(rtsp_thread_id, 2000);
		rtsp_thread_id = 0;
	}
}

static int parse_url (char* url, char* host, int* port)
{
	char *str, *slash, *colon;
	int hostlen, port_tmp;

	str = url;
	port_tmp = 0;

	if (strncmp("rtsp://", url, strlen("rtsp://")) == 0)
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

	*port = port_tmp;

	return 0;
}

static rtsp_client_t *create_client (char *url, char* host, int port, int timeout)
{
	rtsp_client_t *client;
	char* buf;
	int sock;
	unsigned int addr;

	client = (rtsp_client_t*)xmalloc(sizeof(rtsp_client_t));
	if (client == 0)
	{
		GxVod_debug("malloc rtsp client fail\n");
		return 0;
	}

	memset(client, 0, sizeof(rtsp_client_t));

	buf = (char*)xmalloc(RTSP_RECV_BUFFER_SIZE);
	if (buf == 0)
	{
		GxVod_debug("rtsp malloc fail\n");
		free_client(client);
		return 0;
	}
	client->recvbuf = buf;

	buf = (char*)xmalloc(RTSP_SEND_BUFFER_SIZE);
	if (buf == 0)
	{
		GxVod_debug("rtsp malloc fail\n");
		free_client(client);
		return 0;
	}
	client->sendbuf = buf;

	if (vod_porting_sem_create(0, &client->resp_sem) < 0)
	{
		GxVod_debug("rtsp create sem fail\n");
		free_client(client);
		return 0;
	}

	if (vod_porting_sem_create(1, &client->resp_mutex) < 0)
	{
		GxVod_debug("rtsp create mutex fail\n");
		free_client(client);
		return 0;
	}

	sock = vod_porting_tcp_socket_create();
	if (sock <= 0)
	{
		GxVod_debug("create rtsp socket fail\n");
		free_client(client);
		return 0;
	}
	client->sock = sock;

	addr = vod_porting_server_name_to_addr(host);
	if (addr == 0)
	{
		GxVod_debug("rtsp socket addr err\n");
		free_client(client);
		return 0;
	}

	if (vod_porting_socket_connect(sock, port, addr) < 0)
	{
		GxVod_debug("rtsp connect socket fail\n");
		free_client(client);
		return 0;
	}

	client->timeout = timeout;
	client->seq = 1;
	client->url = str_save(url);

	return client;
}


int coship_rtsp_open(char* url, char* hip, int hport, int io_type, int timeout, rtsp_callback_func_t func)
{
	int open_retry = 0;
	int ret, port;
	char host[64];
	rtsp_cmd_t cmd;
	rtsp_resp_describe_t describe_resp;
	rtsp_resp_setup_t setup_resp;

open_restart:
	if (rtsp_client != 0)
	{
		GxVod_debug("rtsp open: rtsp client not release\n");
		return -1;
	}

	if (!func)
	{
		GxVod_debug("rtsp open: callback function is null\n");
		return -1;
	}
	rtsp_callback_func = func;

	ret = parse_url(url, host, &port);
	if (ret != 0)
	{
		GxVod_debug("rtsp url is error, url=%s\n", url);
		return -1;
	}

	if(hip && strlen(hip)>0){
		memcpy(host, hip, 64);
		if(hport > 0)
			port = hport;
	}
	GxVod_debug("server ip %s, port %d\n", host, port);

	rtsp_client = create_client(url, host, port, timeout);
	if (rtsp_client == 0)
	{
		GxVod_debug("rtsp create client fail\n");
		return -1;
	}

	memset(&cmd, 0, sizeof(cmd));
	cmd.title = "DESCRIBE";
	coship_rtsp_send_command(rtsp_client, &cmd);

	memset(&describe_resp, 0, sizeof(describe_resp));
	if ((ret=coship_rtsp_recv_response_describe(rtsp_client, &describe_resp))!=0)
	{
		free_client(rtsp_client);
		rtsp_client = 0;
		if(open_retry >= 3){
			if(ret == RESP_ERR_CODE)
				rtsp_callback_func(RTSP_EVENT_ERROR_CODE, describe_resp.retcode);
			return -1;
		}else{
			open_retry++;
			GxVod_debug("rtsp describe fail\n");
			goto open_restart;
		}
	}

	memset(&cmd, 0, sizeof(cmd));
	if (io_type == IO_CABLE)
		cmd.transport = "CATV;unicast;Frequency=40000000-850000000;Annex=A-B;Modulation=16-256;SymbolRate=6000-8000;ChannelSpace=6000000-8000000";
	else if (io_type == IO_TCP)
		cmd.transport = "MP2T/RTP/TCP;unicast;";
	else if (io_type == IO_UDP)
		cmd.transport = "MP2T/UDP;unicast;";
	cmd.title = "SETUP";
	coship_rtsp_send_command(rtsp_client, &cmd);

	memset(&setup_resp, 0, sizeof(setup_resp));
	if ((ret=coship_rtsp_recv_response_setup(rtsp_client, &setup_resp, &describe_resp))!= 0)
	{
		if (describe_resp.session)
			str_free(describe_resp.session);
		describe_resp.session = 0;
		free_client(rtsp_client);
		rtsp_client = 0;
		if(open_retry >= 3){
			if(ret == RESP_ERR_CODE)
				rtsp_callback_func(RTSP_EVENT_ERROR_CODE, setup_resp.retcode);
			return -1;
		}else{
			open_retry++;
			GxVod_debug("rtsp setup fail\n");
			goto open_restart;
		}
	}

	memset(&rtsp_info, 0, sizeof(rtsp_info));
	memset(&rtsp_time_info, 0, sizeof(rtsp_time_info_t));
	rtsp_info.duration = describe_resp.duration;
	rtsp_info.trickmode = describe_resp.trickmode;
	rtsp_info.program_no = describe_resp.program_no;
	rtsp_info.videopid = describe_resp.videopid;
	rtsp_info.audiopid = describe_resp.audiopid;
	rtsp_info.pcrpid = describe_resp.pcrpid;
	rtsp_info.freq = describe_resp.freq;
	rtsp_info.symbrate = describe_resp.symbrate;
	rtsp_info.qamsize = describe_resp.qamsize;
	rtsp_info.media_port = setup_resp.dst_port;
	if (setup_resp.dst_ip)
		strcpy(rtsp_info.media_ip, setup_resp.dst_ip);

	if (setup_resp.keystring)
		strcpy(rtsp_info.keystring, setup_resp.keystring);

	if (describe_resp.session)
		strcpy(rtsp_info.session, describe_resp.session);

	uint32_t start_time = get_url_time(0, url);
	if (start_time)
	{
		unsigned int t;
		rtsp_info.start_time = start_time;
		t = get_url_time(1, url);
		if (t)
			rtsp_info.duration = t - rtsp_info.start_time;
		else
			rtsp_info.duration = 0;
	}
	else
	{
		rtsp_info.start_time = atol(describe_resp.start_time);
	}

	rtsp_client->session = describe_resp.session;

	if (setup_resp.dst_ip)
		str_free(setup_resp.dst_ip);
	if (setup_resp.keystring)
		str_free(setup_resp.keystring);
	if (describe_resp.start_time)
		str_free(describe_resp.start_time);

	create_rtsp_thread();
	rtsp_status = RTSP_OPEN_STATUS;
	return 0;
}

int	coship_rtsp_play(char* range, int rebuild_pos, int scale)
{
	rtsp_cmd_t cmd;
	rtsp_resp_comm_t* resp;

	if (rtsp_client == 0)
		return -1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.title = "PLAY";
	cmd.range = range;
	cmd.rebuild = rebuild_pos;
	cmd.scale = scale;

	if(rtsp_client->shift_vod){
		char word[64];
		char* p = str_get_field(range, "Clock", word);
		if(p){
			rtsp_client->shift_pos = rtspstr_to_time(word)-rtsp_info.start_time;
		}else{
			rtsp_client->shift_pos = coship_get_really_current_pos();
		}
	}

	if(coship_rtsp_send_command(rtsp_client, &cmd)<0)
		return -1;

	resp = get_asyn_resp();
	if (resp)
	{
		rtsp_info.position = rtsp_info.start_time + resp->position;
		free_comm_resp(resp);
		rtsp_status = RTSP_PLAY_STATUS;
		return 0;
	}
	return -1;
}

int	coship_rtsp_pause()
{
	rtsp_cmd_t cmd;
	rtsp_resp_comm_t* resp;

	if (rtsp_client == 0)
		return -1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.title = "PAUSE";

	if(rtsp_client->shift_vod){
		rtsp_client->shift_pos = coship_get_really_current_pos();
	}

	if(coship_rtsp_send_command(rtsp_client, &cmd)<0)
		return -1;

	resp = get_asyn_resp();
	if (resp)
	{
		rtsp_info.position = rtsp_info.start_time + resp->position;
		free_comm_resp(resp);
		rtsp_status = RTSP_PAUSE_STATUS;
		return 0;
	}
	return -1;
}


int coship_rtsp_get_position(int* position)
{
	rtsp_cmd_t cmd;
	rtsp_resp_comm_t* resp;

	if (rtsp_client == 0)
		return -1;

	memset(&cmd, 0, sizeof(cmd));
	cmd.title = "GET_PARAMETER";

	if(rtsp_client->shift_vod){
		rtsp_client->shift_pos = coship_get_really_current_pos();
	}

	if(coship_rtsp_send_command(rtsp_client, &cmd)<0)
		return -1;

	resp = get_asyn_resp();
	if (resp)
	{
		*position = rtsp_info.start_time + resp->position;
		rtsp_info.position = *position;
		free_comm_resp(resp);
		return 0;
	}
	return -1;
}

void coship_rtsp_close()
{
	rtsp_cmd_t cmd;
	rtsp_resp_comm_t* resp;

	if (rtsp_client == 0)
		return;

	memset(&cmd, 0, sizeof(cmd));
	cmd.title = "TEARDOWN";

	if(rtsp_client->shift_vod){
		rtsp_client->shift_pos = coship_get_really_current_pos();
	}

	coship_rtsp_send_command(rtsp_client, &cmd);

	rtsp_client->timeout = 0; //为了快速退出
	resp = get_asyn_resp();
	if (resp)
	{
		free_comm_resp(resp);
	}

	destroy_rtsp_thread();
	free_client(rtsp_client);
	rtsp_client = 0;
	rtsp_status = RTSP_CLOSE_STATUS;
}

void coship_rtsp_set_timeout(int timeout)
{
	if (rtsp_client == 0)
		return;
	rtsp_client->timeout = timeout;
}

coship_rtsp_info_t* coship_rtsp_get_info()
{
	return &rtsp_info;
}

char* coship_rtsp_version()
{
	return VERSION;
}


int coship_get_trick_mode()
{
	return rtsp_info.trickmode;
}

unsigned int coship_get_start_time()
{
	return rtsp_info.start_time;
}

int coship_get_current_pos()
{
	return rtsp_info.position;
}

int coship_get_duration()
{
	return rtsp_info.duration;
}

void coship_set_shift_vod(void)
{
	rtsp_client->shift_vod = 1;
	return;
}

int coship_get_really_current_pos()
{
	static int position = 0;

	//单位：s
	if(rtsp_status == RTSP_PLAY_STATUS)
		position = (int)((get_tick_time()-rtsp_time_info.read_position_time+rtsp_time_info.really_time)/1000);
	else if(rtsp_status == RTSP_PAUSE_STATUS)
		position = (rtsp_time_info.really_time/1000);
	else if(rtsp_status == RTSP_OPEN_STATUS || rtsp_status == RTSP_CLOSE_STATUS)
		position = 0;
	GxVod_debug("position %d really_time %lld\n", position, rtsp_time_info.really_time);
	return position;
}
