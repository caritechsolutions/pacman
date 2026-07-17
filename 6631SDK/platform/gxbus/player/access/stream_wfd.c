#include "gx_common.h"
#include "gx_network.h"
#include "gx_stream.h"
#include "stream_m3u8.h"
#include "gx_demux.h"
#include "gx_url.h"
#include "udp.h"
#include "tcp.h"
#include "rtp.h"
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>


#define WFD_BUFFER_SIZE 2048
#define WFD_RECV_TIMEOUT (30000)

#define WFD_SERVER "stagefright/1.2 (Linux;Android 4.3)"
#define WFD_PUBLIC "org.wfa.wfd1.0, GET_PARAMETER, SET_PARAMETER"
#define WFD_VIDEO_FORMATS "wfd_video_formats: 39 00 02 02 0001FFFF 1FFFFFFF 00000000 00 0000 0000 00 none none\r\nwfd_audio_codecs: AAC 00000001 00\r\nwfd_client_rtp_ports: RTP/AVP/UDP;unicast 50000 0 mode=play"
#define WFD_TRANS_PORT "RTP/AVP/UDP;unicast;client_port=50000"

#define STREAM_WFD_PVR_DEBUG 0
#if STREAM_WFD_PVR_DEBUG
static FILE* stream_wfd_pvr_fd = NULL;
static char* stream_wfd_pvr_file = "/home/gx/1-wfd.ts";
#endif

typedef struct {
	char* title;
	char* transport;
	char* content_type;
	char* content;
	char* keys;
	char* require;
	char* url;
	char* date;
	char* server;
} wfd_client_cmd;

typedef struct {
	char* date;
	char* public;
	char* content_type;
	char* content;
	char* keys;
	char* server;
} wfd_server_cmd;

typedef struct {
	int video_formats;
	int audio_codecs;
	int video_3d_formats;
	int content_protection;
	int display_edid;
	int coupled_sink;
	int client_rtp_ports;
}wfd_get_param;

typedef struct {
	int audio_format;
	int audio_rate;
	int video_format;
	int video_screen;
	int transport;
	char transurl[1024];
}wfd_set_param;

typedef struct {
	int    sock;
	int    timeout_ms;
	char*  session;
	char*  server_url;
	int    client_seq;
	int    server_seq;
	char   recvbuf[WFD_BUFFER_SIZE];
	char   sendbuf[WFD_BUFFER_SIZE];
	int    audio_format;
	int    audio_rate;
	int    video_format;
	int    video_screen;
	GxURL* udp_url;
	void*  udp_priv;
} WfdPriv;

static int wfd_thread_id = 0;
static int wfd_thread_quit_flag = 0;

typedef struct{
	GxStream parent;
	GxStreamingCtrl* streaming_ctrl;
} GxStreamWfd;

static char* string_get_word(char* line, char* word)
{
	char *lp, *wp;
	int quot_flag;

	wp = word;
	lp = line;
	if (lp == 0) {
		*wp = '\0';
		return 0;
	}

	while (*lp == ' ' || *lp == '\t' || *lp == '=' || *lp == ':' || *lp == ';' || *lp == '\r' || *lp == '\n')
		lp++;

	if (*lp == '"') {
		quot_flag = 1;
		lp++;
	}
	else
		quot_flag = 0;

	while (*lp) {
		switch (*lp) {
		case ' ' :
		case '\t' :
		case ';' :
			if (quot_flag) {
				*wp = *lp;
			} else {
				*wp = '\0';
				lp++;
				return ((wp - word > 0) ? lp : 0);
			}
			break;

		case '\r' :
		case '\n' :
			*wp = '\0';
			lp++;
			return ((wp - word > 0 ) ? lp : 0);

		case '"' :
			if (quot_flag) {
				*wp = '\0';
				lp++;
				return ((wp - word > 0) ? lp : 0);
			} else {
				*wp = *lp;
			}
			break;

		default :
			*wp = *lp;
			break;
		}
		lp++;
		wp++;
	}
	*wp = '\0';
	return ((wp - word > 0) ? lp : 0);
}

static char* string_str(char* pString, const char* pFind)
{
	char* char1 = NULL;
	char* char2 = NULL;

	if ((pString == NULL) || (pFind == NULL) || (strlen(pString) < strlen(pFind)))
		return NULL;

	for (char1 = (char*)pString; (*char1) != '\0'; ++char1) {
		char* char3 = char1;
		for (char2 = (char*)pFind; (*char2) != '\0' && (*char1) != '\0'; ++char2, ++char1) {
			char c1 = (*char1) & 0xDF;
			char c2 = (*char2) & 0xDF;
			if ((c1 != c2) || (((c1 > 0x5A) || (c1 < 0x41)) && (*char1 != *char2)))
				break;
		}

		if ((*char2) == '\0')
			return char3;

		char1 = char3;
	}
	return NULL;
}

static char* string_get_field(char* string, char* title, char* value)
{
	char* pos;

	pos = string_str(string, title);
	if (pos) {
		pos += strlen(title);
		return string_get_word(pos, value);
	}
	return 0;
}

static char* string_save(char* string)
{
	char *p;

	if (string == 0)
		return 0;

	p = (char*)av_mallocz(strlen(string)+1);
	if (p)
		strncpy(p, string, strlen(string));

	return p;
}

static int string_parseurl (char* url, char* host, int host_len, int* port)
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

		strncpy(host, str, host_len);
	}

	if (port_tmp == 0)
		port_tmp = 554;

	if (port)
		*port = port_tmp;

	return 0;
}

int select_timeout (int sock, unsigned int timeout_ms)
{
	fd_set read_set;
	struct timeval timeout;
	int ret = 0;

	if (timeout_ms != 0) {
		FD_ZERO(&read_set);
		FD_SET(sock, &read_set);
		timeout.tv_sec = (unsigned int)(((long)timeout_ms)/(long)1000);
		timeout.tv_usec = (unsigned int)((long)timeout_ms % 1000) * (long)1000;
	} else {
		return 1;
	}

	ret =  select(sock+1,&read_set, NULL, NULL, &timeout);
	if (ret > 0) {
		if (FD_ISSET(sock, &read_set))
			return ret;
		else
			return 0;
	}
	return ret;
}

static int tcp_socket_create(void)
{
	int sock = -1;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	return sock;
}

static unsigned long server_name_to_addr(char * servername)
{
	struct addrinfo hints, *ai;
	struct in_addr s_addr;
	struct sockaddr_in* snip;
	if ( servername == NULL )
	{
		return 0;
	}
	if (inet_aton(servername, &s_addr) != 0)
	{
		return s_addr.s_addr;
	}

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	int ret = getaddrinfo(servername, NULL, &hints, &ai);
	if (ret) {
		gxlogd("hostname error %s\n", gai_strerror(ret));
		return 0;
	}
	snip = (struct sockaddr_in*)ai->ai_addr;
	s_addr = snip->sin_addr;
	freeaddrinfo(ai);
	return s_addr.s_addr;
}

static int tcp_socket_connect(int sock, unsigned short sport, unsigned long saddr)
{
	struct sockaddr_in sockaddr;
	struct in_addr inadd;

	sockaddr.sin_family = AF_INET;

	sockaddr.sin_port = htons(sport);
	inadd.s_addr = saddr;
	sockaddr.sin_addr = inadd;

	{
		int error=-1, len;
		struct timeval tm;
		fd_set set;
		unsigned long ul = 1;

		len = sizeof(int);
		ioctl(sock, FIONBIO, &ul);
		int ret = 0;
		ret = connect(sock, (struct sockaddr *)&sockaddr, sizeof(sockaddr));
		if (ret < 0) {
			if (errno != EINPROGRESS && errno != EAGAIN) {
				gxlogd("[%s] [%d]: tcp in progress\n", __FUNCTION__, __LINE__);
				return -1;
			}

			tm.tv_sec = 5;
			tm.tv_usec = 0;
			FD_ZERO(&set);
			FD_SET(sock, &set);
			if (select(sock+1, NULL, &set, NULL, &tm) > 0) {
				getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, (socklen_t *)&len);
				if (error) {
					gxlogd("TCP connection to failed: (%s)\n", strerror(error));
					return -1;
				}
			} else{
				gxlogd("[%s] [%d]: select error\n", __FUNCTION__, __LINE__);
				return -1;
			}
		}

		ul = 0;
		ioctl(sock, FIONBIO, &ul);
	}
	return 0;
}

static int recv_by_linebreak(int sock, char* buffer, int buflen, int timeout_ms)
{
	int ret, len;
	char* p;

	len = 0;
	p = buffer;

	memset(p, 0, buflen);
	while (1) {
		ret = select_timeout(sock, timeout_ms);
		if (ret > 0) {
			ret = recv(sock, p, 1, 0);
			if (ret < 0) {
				gxlogd("recv by linebreak: recv err, received %d\n", len);
				return -1;
			} else if (ret == 0)
				return -2;

			if ((len+ret >= 4) && (strcmp(p + ret - 4, "\r\n\r\n") == 0)) {
				len += ret;
				buffer[len-1] = 0;
				break;
			} else {
				len += ret;
				p += ret;
			}

			if (len >= buflen)
				return 0;
		} else if (ret == 0) {
			return -2;
		} else {
			gxlogd("recv by linebreak: select err, received %d\n", ret);
			return -1;
		}
	}

	return len;
}

int recv_by_length(int sock, char* buffer, int length, int timeout)
{
	int ret, left;
	char* p;

	left = length;
	p = buffer;
	while (left > 0) {
		ret = select_timeout(sock, timeout);
		if (ret > 0) {
			ret = recv(sock, p, left, 0);
			if (ret > 0) {
				p += ret;
				left -= ret;
			} else {
				gxlogd("recv by length: recv err, received %d\n", length - left);
				return -1;
			}
		} else if (ret == 0) {
			gxlogd("recv by length: select timeout, received %d\n", length - left);
			return -2;
		} else {
			gxlogd("recv by length: select err, received %d\n", length - left);
			return -1;
		}
	}

	buffer[length] = '\0';

	return 0;
}

int recv_server_option(WfdPriv* priv)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;

	ret = recv_by_linebreak(priv->sock, priv->recvbuf, WFD_BUFFER_SIZE, priv->timeout_ms);
	if (ret <= 0)
		return GX_PLAYER_ERROR;

	gxlogd("<<<<< %s\n", priv->recvbuf);
	p = strstr(priv->recvbuf, "OPTIONS");
	if (p == NULL)
		return GX_PLAYER_ERROR;

	p = string_get_field(priv->recvbuf, "CSeq", word);
	if (p)
		resp_seq = atoi(word);
	else
		return GX_PLAYER_ERROR;

	priv->server_seq = resp_seq;
	return GX_PLAYER_OK;
}

int recv_server_getparam(WfdPriv *priv, wfd_get_param* wfd_getparam_resp)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int body_len = 0;

	ret = recv_by_linebreak(priv->sock, priv->recvbuf, WFD_BUFFER_SIZE, priv->timeout_ms);
	if (ret <= 0)
		return GX_PLAYER_ERROR;

	gxlogd("<<<<< %s\n", priv->recvbuf);
	p = strstr(priv->recvbuf, "GET_PARAMETER");
	if (p == NULL)
		return GX_PLAYER_ERROR;

	p = string_get_field(priv->recvbuf, "CSeq", word);
	if (p)
		resp_seq = atoi(word);

	if (resp_seq != priv->server_seq) {
		gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, priv->server_seq);
		return GX_PLAYER_ERROR;
	}

	p = string_get_field(priv->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if (body_len > 0) {
		if (recv_by_length(priv->sock, priv->recvbuf, body_len, priv->timeout_ms) < 0)
			return GX_PLAYER_ERROR;

		gxlogd("%s\n", priv->recvbuf);

		p = strstr(priv->recvbuf, "wfd_video_formats");
		if (p)
			wfd_getparam_resp->video_formats = 1;

		p = strstr(priv->recvbuf, "wfd_audio_codecs");
		if (p)
			wfd_getparam_resp->audio_codecs = 1;

		p = strstr(priv->recvbuf, "wfd_3d_video_formats");
		if (p)
			wfd_getparam_resp->video_3d_formats = 1;

		p = strstr(priv->recvbuf, "wfd_content_protection");
		if (p)
			wfd_getparam_resp->content_protection = 1;

		p = strstr(priv->recvbuf, "wfd_display_edid");
		if (p)
			wfd_getparam_resp->display_edid = 1;

		p = strstr(priv->recvbuf, "wfd_coupled_sink");
		if (p)
			wfd_getparam_resp->coupled_sink = 1;

		p = strstr(priv->recvbuf, "wfd_client_rtp_ports");
		if (p)
			wfd_getparam_resp->client_rtp_ports = 1;
	}

	return GX_PLAYER_OK;
}

int recv_server_setparam(WfdPriv *priv, wfd_set_param* wfd_setparam_resp)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int body_len = 0;

	ret = recv_by_linebreak(priv->sock, priv->recvbuf, WFD_BUFFER_SIZE, priv->timeout_ms);
	if (ret <= 0)
		return GX_PLAYER_ERROR;

	gxlogd("<<<<< %s\n", priv->recvbuf);
	p = strstr(priv->recvbuf, "SET_PARAMETER");
	if (p == NULL)
		return GX_PLAYER_ERROR;

	p = string_get_field(priv->recvbuf, "CSeq", word);
	if (p)
		resp_seq = atoi(word);

	if (resp_seq != priv->server_seq) {
		gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, priv->server_seq);
		return GX_PLAYER_ERROR;
	}

	p = string_get_field(priv->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if (body_len > 0) {
		if (recv_by_length(priv->sock, priv->recvbuf, body_len, priv->timeout_ms) < 0)
			return GX_PLAYER_ERROR;

		gxlogd("%s\n", priv->recvbuf);
		p = strstr(priv->recvbuf, "wfd_presentation_URL");
		if (p)
			string_get_field(priv->recvbuf, "wfd_presentation_URL", wfd_setparam_resp->transurl);

		p = strstr(priv->recvbuf, "wfd_client_rtp_ports");
		if (p) {
			p = string_get_field(priv->recvbuf, "unicast", word);
			if (p)
				wfd_setparam_resp->transport = atoi(word);
		}
	}

	return GX_PLAYER_OK;
}

int recv_client_cmd(WfdPriv* priv)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int resp_code = 0;
	int body_len = 0;

	ret = recv_by_linebreak(priv->sock, priv->recvbuf, WFD_BUFFER_SIZE, priv->timeout_ms);
	if (ret <= 0)
		return ret;

	gxlogd("<<<<< %s\n", priv->recvbuf);
	p = string_get_field(priv->recvbuf, "RTSP/1.0", word);
	if (p)
		resp_code = atoi(word);

	if (resp_code != 200) {
		gxlogd("[%s-%d]: warnning client code [%d]\n", __FUNCTION__, __LINE__, resp_code);
		return -1;
	}

	p = string_get_field(priv->recvbuf, "CSeq", word);
	if (p)
		resp_seq = atoi(word);

	if (priv->client_seq != resp_seq) {
		gxlogd("[%s-%d]: warnning client seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, priv->client_seq);
		return -1;
	}

	p = string_get_field(priv->recvbuf, "Session", word);
	if (p)
		priv->session = string_save(word);

	p = string_get_field(priv->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if (body_len > 0) {
		if (recv_by_length(priv->sock, priv->recvbuf, body_len, priv->timeout_ms) < 0)
			return -1;
		gxlogd("%s\n", priv->recvbuf);
	}

	priv->client_seq++;
	return 0;
}

int recv_common_response(WfdPriv* priv, int* is_server_resp)
{
	char word[64];
	int ret;
	char* p;

	ret = recv_by_linebreak(priv->sock, priv->recvbuf, WFD_BUFFER_SIZE, priv->timeout_ms);
	if (ret <= 0)
		return ret;

	gxlogd("<<<<< %s\n", priv->recvbuf);
	p = strstr(priv->recvbuf, "GET_PARAMETER");
	if (p) {
		int resp_seq = 0;

		*is_server_resp = 1;
		p = string_get_field(priv->recvbuf, "CSeq", word);
		if (p) {
			resp_seq = atoi(word);
			if (priv->server_seq != resp_seq) {
				gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, priv->server_seq);
				return 1;
			}
		}
	}else{
		int resp_code = 0;
		int resp_seq = 0;

		*is_server_resp = 0;
		p = string_get_field(priv->recvbuf, "RTSP/1.0", word);
		if (p) {
			resp_code = atoi(word);
			if (resp_code != 200) {
				gxlogd("[%s-%d]: warnning client code [%d]\n", __FUNCTION__, __LINE__, resp_code);
				return 1;
			}
		}
		p = string_get_field(priv->recvbuf, "CSeq", word);
		if (p) {
			resp_seq = atoi(word);
			if (priv->client_seq != resp_seq) {
				gxlogd("[%s-%d]: warnning client seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, priv->client_seq);
				return 1;
			}
		}
	}
	return 1;
}

#define WFD_ADD_FIELD(fmt, buffer_len, value) \
	ret = snprintf(buffer + *at, buffer_len - *at, (fmt), (value)); \
*at += ret;

static void build_server_cmd(char* buffer, int buffer_len, int *at, WfdPriv* priv, wfd_server_cmd *cmd)
{
	int ret;

	if (cmd && cmd->date) {
		WFD_ADD_FIELD("Date: %s\r\n", buffer_len, cmd->date);
	}

	WFD_ADD_FIELD("CSeq: %u\r\n", buffer_len, priv->server_seq);

	if (cmd && cmd->server) {
		WFD_ADD_FIELD("Server: %s\r\n", buffer_len, cmd->server);
	}

	if (cmd && priv->session)
	{
		WFD_ADD_FIELD("Session: %s\r\n", buffer_len, priv->session);
	}

	if (cmd && cmd->public) {
		WFD_ADD_FIELD("Public: %s\r\n", buffer_len, cmd->public);
	}

	if (cmd && cmd->content) {
		WFD_ADD_FIELD("Content-Length: %d\r\n", buffer_len, strlen(cmd->content) + 4);
	}

	if (cmd && cmd->content_type) {
		WFD_ADD_FIELD("Content-Type: %s\r\n", buffer_len, cmd->content_type);
	}

	if (cmd && cmd->content) {
		WFD_ADD_FIELD("\r\n%s\r\n", buffer_len, cmd->content);
	}

	if (cmd && cmd->keys) {
		WFD_ADD_FIELD("\r\n%s\r\n", buffer_len, cmd->keys);
	}

	WFD_ADD_FIELD("%s", buffer_len, "\r\n");
}

static int send_server_cmd(WfdPriv* priv, wfd_server_cmd *cmd)
{
	int len;

	len = snprintf(priv->sendbuf, sizeof(priv->sendbuf), "RTSP/1.0 200 OK\r\n");
	build_server_cmd(priv->sendbuf, sizeof(priv->sendbuf), &len, priv, cmd);

	if (send(priv->sock, priv->sendbuf, len, 0) != len)
	{
		gxlogd("[%s-%d]: send server command error\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	priv->server_seq++;
	gxlogd(">>>>> %s", priv->sendbuf);
	return GX_PLAYER_OK;
}

static void build_client_cmd(char *buffer, int buffer_len, int *at, WfdPriv *priv, wfd_client_cmd *cmd)
{
	int ret;

	if (cmd && cmd->date) {
		WFD_ADD_FIELD("Date: %s\r\n", buffer_len, cmd->date);
	}

	if (cmd && cmd->server) {
		WFD_ADD_FIELD("Server: %s\r\n", buffer_len, cmd->server);
	}

	WFD_ADD_FIELD("CSeq: %u\r\n", buffer_len, priv->client_seq);

	if (cmd && cmd->require) {
		WFD_ADD_FIELD("Require: %s\r\n", buffer_len, cmd->require);
	}

	if (cmd && priv->session) {
		WFD_ADD_FIELD("Session: %s\r\n", buffer_len, priv->session);
	}

	if (cmd && cmd->transport) {
		WFD_ADD_FIELD("Transport: %s\r\n", buffer_len, cmd->transport);
	}

	if (cmd && cmd->content) {
		WFD_ADD_FIELD("Content-Length: %d\r\n", buffer_len, strlen(cmd->content) + 4);
	}

	if (cmd && cmd->content_type) {
		WFD_ADD_FIELD("Content-Type: %s\r\n", buffer_len, cmd->content_type);
	}

	if (cmd && cmd->content) {
		WFD_ADD_FIELD("\r\n%s\r\n", buffer_len, cmd->content);
	}

	if (cmd && cmd->keys) {
		WFD_ADD_FIELD("\r\n%s\r\n", buffer_len, cmd->keys);
	}

	WFD_ADD_FIELD("%s", buffer_len, "\r\n");
}

static int send_client_cmd(WfdPriv *priv, wfd_client_cmd *cmd)
{
	int len;

	len = snprintf(priv->sendbuf, sizeof(priv->sendbuf), "%s %s RTSP/1.0\r\n", cmd->title, cmd->url);
	build_client_cmd(priv->sendbuf, sizeof(priv->sendbuf)-1, &len, priv, cmd);

	if (send(priv->sock, priv->sendbuf, len, 0) <= 0)
	{
		gxlogd("[%s-%d]: send client command error\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	gxlogd(">>>>> %s", priv->sendbuf);
	return GX_PLAYER_OK;
}

static WfdPriv* wfd_create_priv(void)
{
	WfdPriv* wfdpriv = NULL;

	wfdpriv =av_mallocz(sizeof(WfdPriv));
	return wfdpriv;
}

static void wfd_free_priv(WfdPriv* wfdpriv)
{
	if (wfdpriv == NULL)
		return;

	if (wfdpriv->sock > 0) {
		closesocket(wfdpriv->sock);
		wfdpriv->sock = 0;
	}

	if (wfdpriv->session) {
		av_free(wfdpriv->session);
		wfdpriv->session = NULL;
	}

	if (wfdpriv->server_url) {
		av_free(wfdpriv->server_url);
		wfdpriv->server_url = NULL;
	}

	if (wfdpriv->udp_url) {
		url_free(wfdpriv->udp_url);
		wfdpriv->udp_url = NULL;
	}

	if (wfdpriv->udp_priv) {
		Gx_UDP_Close(wfdpriv->udp_priv);
		wfdpriv->udp_priv = NULL;
	}

	av_free(wfdpriv);
	return;
}

static void wfd_common_thread(void* param)
{
	int ret;
	int is_server_resp;
	WfdPriv* wfdpriv = (WfdPriv*)param;

	while (!wfd_thread_quit_flag)
	{
		is_server_resp = -1;
		ret = recv_common_response(wfdpriv, &is_server_resp);
		if (ret <= 0) {
			GxCore_ThreadDelay(1000);
			continue;
		}else{
			if (is_server_resp == 1) {
				send_server_cmd(wfdpriv, NULL);
			}
		}
	}
	gxlogd("[%s-%d]: wfd thread exit\n", __FUNCTION__, __LINE__);
}

static int wfd_streaming_start(GxURL* url, WfdPriv* wfdpriv)
{
	int  ret = 0, retry = 0;
	int  udp_port = 0;
	char udp_host[64];
	char udp_url[128];
	unsigned long addr;
	wfd_server_cmd server_cmd;
	wfd_client_cmd client_cmd;
	wfd_get_param get_parameter;
	wfd_set_param set_parameter;

connect_retry:
	wfdpriv->sock = tcp_socket_create();
	if (wfdpriv->sock <= 0) {
		gxlogd("[%s] [%d]: create rtsp socket fail\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	addr = server_name_to_addr(url->hostname);
	if (addr == 0) {
		gxlogd("[%s] [%d]: rtsp socket addr err\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	if (tcp_socket_connect(wfdpriv->sock, url->port, addr) < 0) {
		gxlogd("[%s] [%d]: rtsp connect socket fail\n", __FUNCTION__, __LINE__);
		if (retry <= 3) {
			closesocket(wfdpriv->sock);
			GxCore_ThreadDelay(1000);
			retry++;
			goto connect_retry;
		}
		return GX_PLAYER_ERROR;
	}
	wfdpriv->timeout_ms = WFD_RECV_TIMEOUT;

	//first : server begin is or not
	ret = recv_server_option(wfdpriv);
	CHECK_RETURN_VALUE(ret);
	memset(&server_cmd, 0, sizeof(wfd_server_cmd));
	server_cmd.date = "Sun, 11 Aug 2013 04:41:40 +000";
	server_cmd.server = WFD_SERVER;
	server_cmd.public = WFD_PUBLIC;
	ret = send_server_cmd(wfdpriv, &server_cmd);
	CHECK_RETURN_VALUE(ret);

	//second : client call server can transfer
	memset(&client_cmd, 0, sizeof(wfd_client_cmd));
	client_cmd.title = "OPTIONS";
	client_cmd.url = "*";
	client_cmd.require = "org.wfa.wfd1.0";
	ret = send_client_cmd(wfdpriv, &client_cmd);
	CHECK_RETURN_VALUE(ret);
	ret = recv_client_cmd(wfdpriv);
	CHECK_RETURN_VALUE(ret);

	//third : get server support param
	memset(&get_parameter, 0, sizeof(wfd_get_param));
	ret = recv_server_getparam(wfdpriv, &get_parameter);
	CHECK_RETURN_VALUE(ret);
	memset(&server_cmd, 0, sizeof(wfd_server_cmd));
	server_cmd.content_type = "text/parameters";
	server_cmd.content = WFD_VIDEO_FORMATS;
	ret = send_server_cmd(wfdpriv, &server_cmd);
	CHECK_RETURN_VALUE(ret);
	udp_port = 50000;

	//four : get server set param
	memset(&set_parameter, 0, sizeof(wfd_set_param));
	ret = recv_server_setparam(wfdpriv, &set_parameter);
	CHECK_RETURN_VALUE(ret);
	memset(&server_cmd, 0, sizeof(wfd_server_cmd));
	ret = send_server_cmd(wfdpriv, &server_cmd);
	CHECK_RETURN_VALUE(ret);
	wfdpriv->server_url = string_save(set_parameter.transurl);
	string_parseurl(wfdpriv->server_url, udp_host, sizeof(udp_host), NULL);

	//five : get server setup or not
	memset(&set_parameter, 0, sizeof(wfd_set_param));
	ret = recv_server_setparam(wfdpriv, &set_parameter);
	CHECK_RETURN_VALUE(ret);
	memset(&server_cmd, 0, sizeof(wfd_server_cmd));
	ret = send_server_cmd(wfdpriv, &server_cmd);
	CHECK_RETURN_VALUE(ret);

	//six : client setup
	memset(&client_cmd, 0, sizeof(wfd_client_cmd));
	client_cmd.title = "SETUP";
	client_cmd.url = wfdpriv->server_url;
	client_cmd.transport = WFD_TRANS_PORT;
	ret = send_client_cmd(wfdpriv, &client_cmd);
	CHECK_RETURN_VALUE(ret);
	ret = recv_client_cmd(wfdpriv);
	CHECK_RETURN_VALUE(ret);

	//seven : client play
	memset(&client_cmd, 0, sizeof(wfd_client_cmd));
	client_cmd.title = "PLAY";
	client_cmd.url = wfdpriv->server_url;
	ret = send_client_cmd(wfdpriv, &client_cmd);
	CHECK_RETURN_VALUE(ret);
	ret = recv_client_cmd(wfdpriv);
	CHECK_RETURN_VALUE(ret);

	snprintf(udp_url, sizeof(udp_url), "udp://%s:%d", udp_host, udp_port);
	wfdpriv->udp_url = url_new(udp_url);
	if (!wfdpriv->udp_url) {
		gxlogd("%s %d wfd udp open failed\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	wfdpriv->udp_priv = Gx_UDP_Open(wfdpriv->udp_url);
	if (!wfdpriv->udp_priv) {
		gxlogd("%s %d wfd udp open failed\n", __FUNCTION__, __LINE__);
		return GX_PLAYER_ERROR;
	}

	init_rtp_cache();

	wfd_thread_quit_flag = 0;
	GxCore_ThreadCreate("wfd_common_thread",
			&wfd_thread_id, wfd_common_thread, (void*)wfdpriv, 1024*10, 10);

	return GX_PLAYER_OK;
}

static void wfd_streaming_stop(WfdPriv* wfdpriv)
{
	wfd_client_cmd client_cmd;

	memset(&client_cmd, 0, sizeof(wfd_client_cmd));
	client_cmd.title = "TEARDOWN";
	client_cmd.url = wfdpriv->server_url;
	send_client_cmd(wfdpriv, &client_cmd);

	GxCore_ThreadDelay(100);

	if (wfd_thread_id != 0) {
		wfd_thread_quit_flag = 1;
		GxCore_ThreadJoin(wfd_thread_id);
	}

	uninit_rtp_cache();
}

static int wfd_stream_open(GxStream*  s, int mode)
{
	GxStreamWfd* stream = (GxStreamWfd *) (s);
	GxStreamingCtrl* streaming_ctrl;
	WfdPriv* wfdpriv = NULL;

	GxPlayer_SystemGet(PSYS_CBK_INTERRUPT, &s->interrupt_cbk);
	gxlogf("STREAM_WFD, URL: %s\n", stream->parent.url);
	streaming_ctrl = streaming_ctrl_new();
	if (!streaming_ctrl)
		return GX_PLAYER_ERROR;

	streaming_ctrl->url = url_new(stream->parent.url);
	if (!streaming_ctrl->url) {
		streaming_ctrl_free(streaming_ctrl);
		return GX_PLAYER_ERROR;
	}

	if (streaming_ctrl->url->port == 0)
		streaming_ctrl->url->port = 554;

	wfdpriv = wfd_create_priv();
	if (!wfdpriv) {
		streaming_ctrl_free(streaming_ctrl);
		return GX_PLAYER_ERROR;
	}

	if (wfd_streaming_start(streaming_ctrl->url, wfdpriv) != GX_PLAYER_OK) {
		wfd_free_priv(wfdpriv);
		streaming_ctrl_free(streaming_ctrl);
		return GX_PLAYER_ERROR;
	}

	//预留一部份数据在buffer,以便于数据探测
	{
		char buffer[STREAM_UDP_MAX_PKT_SIZE];

		while (1) {
			UDPContext* udp = wfdpriv->udp_priv;

			if (s->interrupt_cbk && s->interrupt_cbk())
				return -1;

			int nread = read_rtp_from_server(udp->udp_fd, buffer, MAX_RTP_PACKET_SIZE, 0);
			if (nread < 0) {
				wfd_streaming_stop(wfdpriv);
				wfd_free_priv(wfdpriv);
				streaming_ctrl_free(streaming_ctrl);
				return GX_PLAYER_ERROR;
			}else if (nread > 0) {
				streaming_bufferize(streaming_ctrl, buffer, nread);
				break;
			}
		}
	}
	streaming_ctrl->priv = wfdpriv;
	streaming_ctrl->streaming_read = NULL;
	streaming_ctrl->streaming_seek = NULL;
	streaming_ctrl->buffering      = 0;
	streaming_ctrl->prebuffer_size = 0;

	stream->streaming_ctrl = streaming_ctrl;
	stream->parent.file_format          = GX_STREAMTYPE_STREAM;
	stream->parent.demuxer_type         = GX_DEMUXER_TYPE_MPEG_TS;
	stream->parent.nobuffer        = 1;
	stream->parent.no_check_priv_packet = 1;

#if STREAM_WFD_PVR_DEBUG
		if (stream_wfd_pvr_fd)
			fclose(stream_wfd_pvr_fd);
		stream_wfd_pvr_fd = fopen(stream_wfd_pvr_file, "w+");
#endif
	return GX_PLAYER_OK;
}

static int wfd_stream_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int recv_size = -1;
	GxStreamWfd* stream = (GxStreamWfd*)s;
	GxStreamingCtrl* stream_ctrl = stream->streaming_ctrl;
	WfdPriv* wfdpriv = stream_ctrl->priv;

read_again:
	if (stream->parent.interrupt_cbk && stream->parent.interrupt_cbk())
		return -1;

	if (stream_ctrl->buffer_size > 0) {
		int buffer_len = stream_ctrl->buffer_size - stream_ctrl->buffer_pos;
		recv_size = (max_len < buffer_len) ? max_len : buffer_len;
		memcpy(buffer, (stream_ctrl->buffer + stream_ctrl->buffer_pos), recv_size);
		stream_ctrl->buffer_pos += recv_size;
		if (stream_ctrl->buffer_pos >= stream_ctrl->buffer_size) {
			streaming_buffer_free(stream_ctrl);
		}
		gxlogd("%s %d, recv_size %d max_len %d\n", __FUNCTION__, __LINE__, recv_size, max_len);
		goto read_complete;
	}

	if (max_len < STREAM_UDP_MAX_PKT_SIZE) {
		char tmp_buffer[STREAM_UDP_MAX_PKT_SIZE];
		UDPContext* udp = wfdpriv->udp_priv;

		int nread = read_rtp_from_server(udp->udp_fd, tmp_buffer, MAX_RTP_PACKET_SIZE, 0);
		if (nread > 0) {
			streaming_bufferize(stream_ctrl, tmp_buffer, nread);
			goto read_again;
		}
		gxlogd("%s %d, recv_size %d max_len %d\n", __FUNCTION__, __LINE__, nread, max_len);
		return nread;
	}

	if (wfdpriv->udp_priv) {
		UDPContext* udp = wfdpriv->udp_priv;

		recv_size = read_rtp_from_server(udp->udp_fd, (char*)buffer, max_len, 0);
	}

read_complete:

#if STREAM_WFD_PVR_DEBUG
	if (stream_wfd_pvr_fd && recv_size > 0) {
		fwrite(buffer, recv_size, 1, stream_wfd_pvr_fd);
	}
#endif

	return recv_size;
}

static int wfd_streaming_rollback(GxStream* stream, char* buffer, int size)
{
	GxStreamingCtrl* streaming_ctrl = stream->streaming_ctrl;

	if (!streaming_ctrl) {
		return GX_PLAYER_ERROR;
	}
	if ((size <= streaming_ctrl->buffer_pos)&&(streaming_ctrl->buffer)) {
		streaming_ctrl->buffer_pos -= size;
	}else
		streaming_bufferize(streaming_ctrl, buffer, size);

#if STREAM_WFD_PVR_DEBUG
	if (stream_wfd_pvr_fd)
		fclose(stream_wfd_pvr_fd);
	stream_wfd_pvr_fd = fopen(stream_wfd_pvr_file, "w+");
#endif

	return GX_PLAYER_OK;
}

static int wfd_stream_control(GxStream *s, int cmd, void *arg)
{
	switch (cmd) {
		case GX_STREAM_CTRL_RESET:
		{
			s->buf_pos = 0;
			s->buf_len = 0;
			return GX_PLAYER_OK;
		}
		case GX_STREAM_CTRL_ROLL_BACK:
		{
			GxStream_RollBack* rol = arg;
			if (!rol)
				return GX_PLAYER_ERROR;
			wfd_streaming_rollback(s, rol->buffer, rol->size);
			return GX_PLAYER_OK;
		}
		default:
			break;
	}

	return GX_PLAYER_ERROR;
}

static void wfd_stream_close(GxStream* s)
{
	GxStreamWfd* stream = (GxStreamWfd*)s;
#if STREAM_WFD_PVR_DEBUG
	if (stream_wfd_pvr_fd)
		fclose(stream_wfd_pvr_fd);
	stream_wfd_pvr_fd = NULL;
#endif

	if (stream->streaming_ctrl) {
		if (stream->streaming_ctrl->priv) {
			wfd_streaming_stop(stream->streaming_ctrl->priv);
			wfd_free_priv(stream->streaming_ctrl->priv);
			stream->streaming_ctrl->priv = NULL;
		}
		streaming_ctrl_free(stream->streaming_ctrl);
		stream->streaming_ctrl = NULL;
	}

	wfd_thread_id = 0;
	wfd_thread_quit_flag = 0;
}

GxStreamClass gx_stream_wfd = {
	._inherit = {
		._inherit = {
			.name   = "StreamRnm",
			.parent = &gx_streambase,
			.size   = sizeof(GxStreamWfd),
		},
	},
	.protocols = {"wfd", NULL},
	DEF_AUTHOR("Stream","wfd","No description","L.F","No comment"),

	.flags   = 0,
	.open    = wfd_stream_open,
	.read    = wfd_stream_read,
	.write   = NULL,
	.seek    = NULL,
	.control = wfd_stream_control,
	.close   = wfd_stream_close,
};
