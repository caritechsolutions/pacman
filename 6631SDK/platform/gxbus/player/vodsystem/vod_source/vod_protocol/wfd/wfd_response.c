#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "../../include/vod_common_func.h"
#include "wfd_util.h"
#include "gx_common.h"

int wfd_recv_server_option_cmd(wfd_client_t *client)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;

	if(client->sock < 0)
		return -1;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, WFD_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	gxlogd("<<<<< %s\n", client->recvbuf);
	p = strstr(client->recvbuf, "OPTIONS");
	if(p == NULL)
		return -1;

	p = str_get_field(client->recvbuf, "CSeq", word);
	if (p)
		resp_seq = atoi(word);
	else
		return -1;

	client->server_seq = resp_seq;
	return 0;
}

int wfd_recv_server_getparam_cmd(wfd_client_t *client, wfd_getparam_t* wfd_getparam_resp)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int body_len = 0;

	if(client->sock < 0)
		return -1;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, WFD_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	gxlogd("<<<<< %s\n", client->recvbuf);
	p = strstr(client->recvbuf, "GET_PARAMETER");
	if(p == NULL)
		return -1;

	p = str_get_field(client->recvbuf, "CSeq", word);
	if(p)
		resp_seq = atoi(word);

	if(resp_seq != client->server_seq){
		gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, client->server_seq);
		return -1;
	}

	p = str_get_field(client->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if(body_len > 0){
		if (socket_recv_by_length(client->sock, client->recvbuf, body_len, client->timeout) < 0)
			return -1;

		gxlogd("%s\n", client->recvbuf);

		p = strstr(client->recvbuf, "wfd_video_formats");
		if(p)
			wfd_getparam_resp->video_formats = 1;

		p = strstr(client->recvbuf, "wfd_audio_codecs");
		if(p)
			wfd_getparam_resp->audio_codecs = 1;

		p = strstr(client->recvbuf, "wfd_3d_video_formats");
		if(p)
			wfd_getparam_resp->video_3d_formats = 1;

		p = strstr(client->recvbuf, "wfd_content_protection");
		if(p)
			wfd_getparam_resp->content_protection = 1;

		p = strstr(client->recvbuf, "wfd_display_edid");
		if(p)
			wfd_getparam_resp->display_edid = 1;

		p = strstr(client->recvbuf, "wfd_coupled_sink");
		if(p)
			wfd_getparam_resp->coupled_sink = 1;

		p = strstr(client->recvbuf, "wfd_client_rtp_ports");
		if(p)
			wfd_getparam_resp->client_rtp_ports = 1;
	}

	return 0;
}

int wfd_recv_server_setparam_cmd(wfd_client_t *client, wfd_setparam_t* wfd_setparam_resp)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int body_len = 0;

	if(client->sock < 0)
		return -1;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, WFD_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	gxlogd("<<<<< %s\n", client->recvbuf);
	p = strstr(client->recvbuf, "SET_PARAMETER");
	if(p == NULL)
		return -1;

	p = str_get_field(client->recvbuf, "CSeq", word);
	if(p)
		resp_seq = atoi(word);

	if(resp_seq != client->server_seq){
		gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, client->server_seq);
		return -1;
	}

	p = str_get_field(client->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if(body_len > 0){
		if (socket_recv_by_length(client->sock, client->recvbuf, body_len, client->timeout) < 0)
			return -1;
		gxlogd("%s\n", client->recvbuf);
		p = strstr(client->recvbuf, "wfd_presentation_URL");
		if(p)
			str_get_field(client->recvbuf, "wfd_presentation_URL", wfd_setparam_resp->transurl);

		p = strstr(client->recvbuf, "wfd_client_rtp_ports");
		if(p){
			p = str_get_field(client->recvbuf, "unicast", word);
			if(p)
				wfd_setparam_resp->transport = atoi(word);
		}
	}

	return 0;
}

int wfd_recv_client_cmd(wfd_client_t* client)
{
	char word[64];
	int ret;
	char* p;
	int resp_seq = 0;
	int resp_code = 0;
	int body_len = 0;

	if(client->sock < 0)
		return -1;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, WFD_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	gxlogd("<<<<< %s\n", client->recvbuf);
	p = str_get_field(client->recvbuf, "RTSP/1.0", word);
	if (p)
		resp_code = atoi(word);

	if(resp_code != 200){
		gxlogd("[%s-%d]: warnning client code [%d]\n", __FUNCTION__, __LINE__, resp_code);
		return -1;
	}

	p = str_get_field(client->recvbuf, "CSeq", word);
	if(p)
		resp_seq = atoi(word);

	if(client->client_seq != resp_seq){
		gxlogd("[%s-%d]: warnning client seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, client->client_seq);
		return -1;
	}

	p = str_get_field(client->recvbuf, "Session", word);
	if (p)
		client->session = str_save(word);

	p = str_get_field(client->recvbuf, "Content-Length", word);
	if (p)
		body_len = atoi(word);

	if(body_len > 0){
		if (socket_recv_by_length(client->sock, client->recvbuf, body_len, client->timeout) < 0)
			return -1;
		gxlogd("%s\n", client->recvbuf);
	}

	client->client_seq++;
	return 0;
}

int wfd_recv_common_response(wfd_client_t* client, int* is_server_resp)
{
	char word[64];
	int ret;
	char* p;

	if(client->sock < 0)
		return -1;

	ret = socket_recv_by_linebreak(client->sock, client->recvbuf, WFD_RECV_BUFFER_SIZE, client->timeout);
	if (ret != 0)
		return ret;

	gxlogd("<<<<< %s\n", client->recvbuf);
	p = strstr(client->recvbuf, "GET_PARAMETER");
	if(p){
		int resp_seq = 0;

		*is_server_resp = 1;
		p = str_get_field(client->recvbuf, "CSeq", word);
		if(p){
			resp_seq = atoi(word);
			if(client->server_seq != resp_seq){
				gxlogd("[%s-%d]: warnning server seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, client->server_seq);
				return 1;
			}
		}
	}else{
		int resp_code = 0;
		int resp_seq = 0;

		*is_server_resp = 0;
		p = str_get_field(client->recvbuf, "RTSP/1.0", word);
		if (p){
			resp_code = atoi(word);
			if(resp_code != 200){
				gxlogd("[%s-%d]: warnning client code [%d]\n", __FUNCTION__, __LINE__, resp_code);
				return 1;
			}
		}
		p = str_get_field(client->recvbuf, "CSeq", word);
		if(p){
			resp_seq = atoi(word);
			if(client->client_seq != resp_seq){
				gxlogd("[%s-%d]: warnning client seq [%d %d]\n", __FUNCTION__, __LINE__, resp_seq, client->client_seq);
				return 1;
			}
		}
	}
	return 0;
}
