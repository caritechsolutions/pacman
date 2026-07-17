#ifndef _WFD_UTIL_H_
#define _WFD_UTIL_H_ 

#define WFD_RECV_BUFFER_SIZE	2048
#define WFD_SEND_BUFFER_SIZE	1024

typedef void (*wfd_callback_func_t)(int event, int errcode);

typedef struct wfd_client_t 
{
	int sock;
	char* url;
	char* session;
	int client_seq;
	int server_seq;
	int timeout;
	unsigned int resp_sem;
	unsigned int resp_mutex;
	char* recvbuf;
	char* sendbuf;
	char* server_url;
	int server_port;
	int client_port;
	int audio_format;
	int audio_rate;
	int video_format;
	int video_screen;
}wfd_client_t;

typedef struct wfd_client_cmd_t
{
	char* title;
	char* transport;
	char* content_type;
	char* content;
	char* keys;
	char* require;
	char* url;
	char* date;
	char* server;
}wfd_clientcmd_t;

typedef struct wfd_server_cmd_t
{
	char* date;
	char* public;
	char* content_type;
	char* content;
	char* keys;
	char* server;
}wfd_servercmd_t;

typedef struct wfd_getparam_t
{
	int video_formats;
	int audio_codecs;
	int video_3d_formats;
	int content_protection;
	int display_edid;
	int coupled_sink;
	int client_rtp_ports;
}wfd_getparam_t;

typedef struct wfd_setparam_t
{
	int audio_format;
	int audio_rate;
	int video_format;
	int video_screen;
	int transport;
	char transurl[1024];
}wfd_setparam_t;

int wfd_recv_server_option_cmd(wfd_client_t *client);
int wfd_recv_server_getparam_cmd(wfd_client_t *client, wfd_getparam_t* wfd_getparam_resp);
int wfd_recv_server_setparam_cmd(wfd_client_t *client, wfd_setparam_t* wfd_setparam_resp);
int wfd_recv_client_cmd(wfd_client_t* client);
int wfd_recv_common_response(wfd_client_t* client, int* is_server_resp);
int wfd_send_server_cmd(wfd_client_t *client, wfd_servercmd_t *cmd);
int wfd_send_client_cmd(wfd_client_t *client, wfd_clientcmd_t *cmd);
int wfd_rtsp_open(char* url, char* udp_host, int* udp_port, int timeout, wfd_callback_func_t func);
void wfd_rtsp_close(void);
#endif
