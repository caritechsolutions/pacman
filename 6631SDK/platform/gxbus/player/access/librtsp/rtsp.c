/*
*  This file was ported to MPlayer from xine CVS rtsp.c,v 1.9 2003/04/10 02:30:48
*/

/*
*  Copyright (C) 2000-2002 the xine project
*
*  This file is part of xine, a free video player.
*
*  xine is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2 of the License, or
*  (at your option) any later version.
*
*  xine is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
*
*
*  a minimalistic implementation of rtsp protocol,
*  *not* RFC 2326 compilant yet.
*
*     2006, Benjamin Zores and Vincent Mussard
*       fixed a lot of RFC compliance issues.
*/

#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include "gx_common.h"
#include "rtsp.h"

#define BUF_SIZE 4096
#define HEADER_SIZE 1024
#define MAX_FIELDS 256

struct rtsp_s {

	int s;

	char* host;
	int port;
	char* path;
	char* param;
	char* mrl;
	char* user_agent;

	char* server;
	unsigned int server_state;
	uint32_t server_caps;

	unsigned int cseq;
	char* session;

	int recv_answer;
	char* answers[MAX_FIELDS];	/* data of last message */
	char* scheduled[MAX_FIELDS];	/* will be sent with next message */
};

/*
*  constants
*/

#define RTSP_PROTOCOL_VERSION "RTSP/1.0"

/* server states*/
#define RTSP_CONNECTED 1
#define RTSP_INIT      2
#define RTSP_READY     4
#define RTSP_PLAYING   8
#define RTSP_RECORDING 16
#define RTSP_PAUSE     32

/* server capabilities*/
#define RTSP_OPTIONS       0x001
#define RTSP_DESCRIBE      0x002
#define RTSP_ANNOUNCE      0x004
#define RTSP_SETUP         0x008
#define RTSP_GET_PARAMETER 0x010
#define RTSP_SET_PARAMETER 0x020
#define RTSP_TEARDOWN      0x040
#define RTSP_PLAY          0x080
#define RTSP_RECORD        0x100
/*
*  network utilities
*/

static int write_stream(int fd, const char* buf, int size)
{
	int len = 0;
	int retry = 10;
	while(retry && (len < size)){
		int ret;
		fd_set fds_wrd;
		struct timeval tv;

		FD_ZERO(&fds_wrd);
		FD_SET(fd,&fds_wrd);
		tv.tv_sec = 0;
		tv.tv_usec = 100*1000;
		ret = select(fd+1, NULL, &fds_wrd, NULL, &tv);
		if(ret == -1){
			gxlogd("sock error (%s)\n", strerror(errno));
			return -1;
		}else if(ret > 0){
			if(FD_ISSET(fd, &fds_wrd)){
				ret = send(fd, buf + len, size - len, 0);
				if (ret < 0) {
					break;
				}else if (ret == 0){
					gxlogd("send over (%s)\n", strerror(errno));
					return -1;
				}else{
					len += ret;
					continue;
				}
			}
		}
		retry--;
	}
	return len;
}

static ssize_t read_stream(int fd, void* buf, size_t size)
{
	int len = 0;
	int retry = 10;
	while(retry && (len < size)){
		int ret;
		fd_set fds_red;
		struct timeval tv;

		FD_ZERO(&fds_red);
		FD_SET(fd,&fds_red);
		tv.tv_sec = 0;
		tv.tv_usec = 100*1000;
		ret = select(fd+1, &fds_red, NULL, NULL, &tv);
		if(ret == -1){
			gxlogd("sock error (%s)\n", strerror(errno));
			return -1;
		}else if(ret > 0){
			if(FD_ISSET(fd, &fds_red)){
				ret = recv(fd, buf + len, size - len, 0);
				if (ret < 0) {
					len = -1;
					break;
				}else if (ret == 0){
					gxlogd("recv over (%s)\n", strerror(errno));
					if(errno != EINPROGRESS){
						return -1;
					}
				}else{
					len += ret;
					continue;
				}
			}
		}
		retry--;
	}
	return len;
}

/*
*  rtsp_get gets a line from stream
*  and returns a null terminated string.
*/
static char* rtsp_get(rtsp_t * s)
{
	int ret = 0;
	int n = 1;
	char* buffer = av_malloc(BUF_SIZE);
	char* string = NULL;

	ret = read_stream(s->s, buffer, 1);
	if(ret != 1)
		goto error_read;

	while (n < BUF_SIZE) {
		ret = read_stream(s->s, &(buffer[n]), 1);
		if(ret <= 0)
			goto error_read;
		if ((buffer[n - 1] == 0x0d) && (buffer[n] == 0x0a))
			break;
		n++;
	}

	if (n >= BUF_SIZE) {
		gxlogd("librtsp: buffer overflow in rtsp_get\n");
		exit(1);
	}
	string = av_malloc(n);
	memcpy(string, buffer, n - 1);
	string[n - 1] = 0;
//	gxlogd("%s\n", string);

error_read:
	av_free(buffer);
	return string;
}

/*
*  rtsp_put puts a line on stream
*/
static void rtsp_put(rtsp_t*  s, const char *string)
{
	write_stream(s->s, string, strlen(string));
//	gxlogd("%s",string);
}

/*
*  extract server status code
*/

static int get_code(const char* string)
{

	char buf[4];
	int code = 0;

	if (!strncmp(string, RTSP_PROTOCOL_VERSION, strlen(RTSP_PROTOCOL_VERSION))) {
		memcpy(buf, string + strlen(RTSP_PROTOCOL_VERSION) + 1, 3);
		buf[3] = 0;
		code = atoi(buf);
	} else if (!strncmp(string, RTSP_METHOD_SET_PARAMETER, 8)) {
		return RTSP_STATUS_SET_PARAMETER;
	}

	if (code != RTSP_STATUS_OK)
		gxlogd("librtsp: server responds: '%s'\n", string);

	return code;
}

/*
*  send a request
*/
static void send_request(rtsp_t*  s, const char *type, const char *what)
{
	int res_len;
	char* string = av_malloc(BUF_SIZE);
	char* p = NULL;
	char* *payload = s->scheduled;

	if(string)
	{
		res_len = BUF_SIZE-3;
		p = string;
		snprintf(p, BUF_SIZE, "%s %s %s\r\n", type, what, RTSP_PROTOCOL_VERSION);
		res_len -= strlen(p);
		p  += strlen(p);
		if (payload)
			while (*payload &&(res_len > (strlen(*payload)+3))) {
				snprintf(p, BUF_SIZE, "%s\r\n",*payload);
				res_len -= strlen(p);
				p += strlen(p);
				payload++;
			}
		snprintf(p, BUF_SIZE, "%s\r\n","");
		rtsp_put(s,string);
		av_free(string);
	}
	rtsp_unschedule_all(s);
	s->recv_answer = 0;
}

/*
*  schedule standard fields
*/
static void set_schedule(rtsp_t*  s)
{
	char tmp[17] = {0};

	snprintf(tmp, 17, "CSeq: %u", s->cseq);
	rtsp_schedule_field(s, tmp);

	if (s->session) {
		char* buf;
		buf = av_malloc(strlen(s->session) + 15);
		snprintf(buf, strlen(s->session) + 15, "Session: %s", s->session);
		rtsp_schedule_field(s, buf);
		av_free(buf);
	}
}

/*
*  get the answers, if server responses with something != 200, return NULL
*/
static int get_answers(rtsp_t*  s)
{
	char* answer = NULL;
	unsigned int answer_seq;
	char* *answer_ptr = s->answers;
	int code;
	int ans_count = 0;

	answer = rtsp_get(s);
	if (!answer)
		return 0;
	code = get_code(answer);
	av_free(answer);

	rtsp_free_answers(s);

	do {			/* while we get answer lines*/
		answer = rtsp_get(s);
		if (!answer)
			return 0;

		if (!strncasecmp(answer, "CSeq:", 5)) {
			sscanf(answer, "%*s %u", &answer_seq);
			if (s->cseq != answer_seq) {
				s->cseq = answer_seq;
			}
		}
		if (!strncasecmp(answer, "Server:", 7)) {
			char* buf = av_malloc(strlen(answer));
			sscanf(answer, "%*s %s", buf);
			if (s->server)
				av_free(s->server);
			s->server = av_strdup(buf);
			av_free(buf);
		}
		if (!strncasecmp(answer, "Session:", 8)) {
			char* p = NULL;
			char* buf = av_calloc(1, strlen(answer));
			sscanf(answer, "%*s %s", buf);
			if(buf && (p = strstr(buf, ";")))
				memset(p, 0, strlen(answer)+buf-p);
			if (s->session) {
				if (strcmp(buf, s->session)) {
					gxlogd("rtsp: warning: setting NEW session: %s\n", buf);
					av_free(s->session);
					s->session = av_strdup(buf);
				}
			} else {
				gxlogd("rtsp: setting session id to: %s\n", buf);
				s->session = av_strdup(buf);
			}
			av_free(buf);
		}
		*answer_ptr = answer;
		answer_ptr++;
	} while ((strlen(answer) != 0) && (++ans_count < MAX_FIELDS));

	s->cseq++;
	*answer_ptr = NULL;
	set_schedule(s);
	s->recv_answer = 1;

	return code;
}
/*
*  parse the answer in the wait time
 */
int rtsp_parse_answers(rtsp_t*  s)
{
	char* *answer_ptr = s->answers;
	char* answer;
	int code;
	unsigned int answer_seq;

	code = get_code(*answer_ptr);
	answer_ptr++;
	answer = *answer_ptr;
	while(answer)
	{
		if (!strncasecmp(answer, "CSeq:", 5)) {
			sscanf(answer, "%*s %u", &answer_seq);
			if (s->cseq != answer_seq) {
				s->cseq = answer_seq;
			}
		}
		if (!strncasecmp(answer, "Server:", 7)) {
			char* buf = av_malloc(strlen(answer));
			sscanf(answer, "%*s %s", buf);
			if (s->server)
				av_free(s->server);
			s->server = av_strdup(buf);
			av_free(buf);
		}
		if (!strncasecmp(answer, "Session:", 8)) {
			char* buf = av_calloc(1, strlen(answer));
			sscanf(answer, "%*s %s", buf);
			if (s->session) {
				if (strcmp(buf, s->session)) {
					gxlogd("rtsp: warning: setting NEW session: %s\n", buf);
					av_free(s->session);
					s->session = av_strdup(buf);
				}
			} else {
				gxlogd("rtsp: setting session id to: %s\n", buf);
				s->session = av_strdup(buf);
			}
			av_free(buf);
		}
		answer_ptr++;
		answer = *answer_ptr;
	}
	s->cseq++;
	set_schedule(s);
	s->recv_answer = 1;

	return code;
}
/*
*  send an ok message
*/
int rtsp_send_ok(rtsp_t*  s)
{
	char buf[100] = {0};
	char *p = buf;

	snprintf(p, sizeof(buf), "%s\r\n","RTSP/1.0 200 OK");
	p += strlen(p);
	snprintf(p, sizeof(buf), "CSeq: %u\r\n", s->cseq);
	p += strlen(p);
	snprintf(p, sizeof(buf), "%s\r\n","");
	rtsp_put(s, buf);
	return 0;
}

/*
 *send an error para message
 */
int rtsp_send_error_param(rtsp_t* s, int seq)
{
	char buf[100] ={0};
	char *p = buf;
	/* let's make the server happy*/

	snprintf(p, sizeof(buf), "%s\r\n", "RTSP/1.0 451 Parameter Not Understood");
	p += strlen(p);
	snprintf(p, sizeof(buf), "CSeq: %u\r\n", seq);
	p += strlen(p);
	snprintf(p, sizeof(buf), "%s\r\n", "");
	rtsp_put(s,buf);
	return 0 ;
}

/*
*  implementation of must-have rtsp requests; functions return
*  server status code.
*/
int rtsp_request_options(rtsp_t*  s, const char *what)
{

	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + 16);
		snprintf(buf, strlen(s->host) + 16, "rtsp://%s:%i", s->host, s->port);
	}
	send_request(s, RTSP_METHOD_OPTIONS, buf);
	av_free(buf);

	return get_answers(s);
}

int rtsp_request_describe(rtsp_t*  s, const char *what)
{

	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_DESCRIBE, buf);
	av_free(buf);

	return get_answers(s);
}

int rtsp_request_setup(rtsp_t*  s, const char *what, char *control)
{
	char* buf = NULL;
	if (what)
		buf = av_strdup(what);
	else {
		int len = strlen(s->host) + strlen(s->path) + 16;
		if (control)
			len += strlen(control) + 1;

		buf = av_malloc(len);
		snprintf(buf, len, "rtsp://%s:%i/%s%s%s", s->host, s->port, s->path,
			control ? "/" : "", control ? control : "");
	}

	send_request(s, RTSP_METHOD_SETUP, buf);
	av_free(buf);
	return get_answers(s);
}

int rtsp_request_setparameter(rtsp_t*  s, const char *what)
{
	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_SET_PARAMETER, buf);
	av_free(buf);

	return get_answers(s);
}

int rtsp_request_getparameter(rtsp_t*  s, const char *what)
{
	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_GET_PARAMETER, buf);
	av_free(buf);

	return get_answers(s);

}

int rtsp_request_play(rtsp_t*  s, const char *what)
{
	char* buf;
	int ret;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_PLAY, buf);
	av_free(buf);

	ret = get_answers(s);
	if (ret == RTSP_STATUS_OK)
		s->server_state = RTSP_PLAYING;

	return ret;
}

int rtsp_request_pause(rtsp_t* s, const char *what)
{
	char* buf;

	if(what) {
		buf = av_strdup(what);
	}else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_PAUSE, buf);
	av_free(buf);

	return RTSP_STATUS_OK;
}

int rtsp_request_resume(rtsp_t*  s, const char *what)
{
	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_PLAY, buf);
	av_free(buf);

	return RTSP_STATUS_OK;
}

int rtsp_request_seek(rtsp_t*  s, const char *what)
{
	char* buf;

	if (what) {
		buf = av_strdup(what);
	} else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_PLAY, buf);
	av_free(buf);

	return RTSP_STATUS_OK;
}

int rtsp_request_teardown(rtsp_t*  s, const char *what)
{

	char* buf;

	if (what)
		buf = av_strdup(what);
	else {
		buf = av_malloc(strlen(s->host) + strlen(s->path) + 16);
		snprintf(buf, strlen(s->host) + strlen(s->path) + 16, "rtsp://%s:%i/%s", s->host, s->port, s->path);
	}
	send_request(s, RTSP_METHOD_TEARDOWN, buf);
	av_free(buf);

	/* after teardown we're done with RTSP streaming, no need to get answer as
	   reading more will only result to garbage and buffer overflow*/
	return RTSP_STATUS_OK;
}

/*
*  read opaque data from stream
*/
int rtsp_read_data(rtsp_t*  s, unsigned char *buffer, unsigned int size)
{
	int i, seq;

	if (size >= 4) {
		i = read_stream(s->s, buffer, 4);
		if (i < 4)
			return i;
		if (((buffer[0] == 'S') && (buffer[1] == 'E') && (buffer[2] == 'T') && (buffer[3] == '_')) || ((buffer[0] == 'O') && (buffer[1] == 'P') && (buffer[2] == 'T') && (buffer[3] == 'I')))	// OPTIONS
		{
//			if(buffer[0] == 'S')
//				gxlogd("SET_");
//			else if(buffer[0] == 'O')
//				gxlogd("OPTI");
			char* rest = rtsp_get(s);
			if (!rest)
				return -1;

			seq = -1;
			do {
				av_free(rest);
				rest = rtsp_get(s);
				if (!rest)
					return -1;
				if (!strncasecmp(rest, "CSeq:", 5))
					sscanf(rest, "%*s %u", &seq);
			} while (strlen(rest) != 0);
			av_free(rest);
			if (seq < 0) {
				gxlogd("rtsp: warning: CSeq not recognized!\n");
				seq = 1;
			}
			rtsp_send_error_param(s,seq);
			return -1;
		} else if((buffer[0] == 'R') && (buffer[1] == 'T') && (buffer[2] == 'S') && (buffer[3] == 'P')){
			char* *answer_ptr = s->answers;
			char* answer = NULL;
			int ans_count = 0;

			rtsp_free_answers(s);
//			gxlogd("RTSP");
			answer = rtsp_get(s);
			if (!answer)
				return -1;
			*answer_ptr = av_malloc(strlen(answer)+5);
			if(!(*answer_ptr))
				return -1;
			snprintf(*answer_ptr,strlen(answer)+5, "RTSP%s",answer);
			answer_ptr++;
			*answer_ptr = NULL;
			av_free(answer);
			do {
				answer = rtsp_get(s);
				if (!answer)
					return 0;
				*answer_ptr = answer;
				answer_ptr++;
			} while ((strlen(answer) != 0) && (++ans_count < MAX_FIELDS));
			*answer_ptr = NULL;
			rtsp_parse_answers(s);
			return -1;
		}else {
			i = read_stream(s->s, buffer + 4, size - 4);
			i += 4;
		}
	} else
		i = read_stream(s->s, buffer, size);

	return i;
}

/*
*  connect to a rtsp server
*/
rtsp_t* rtsp_connect(int fd, char *mrl, char *path, char *host, int port, char *user_agent)
{

	rtsp_t* s = av_malloc(sizeof(rtsp_t));
	int i;

	for (i = 0; i < MAX_FIELDS; i++) {
		s->answers[i] = NULL;
		s->scheduled[i] = NULL;
	}

	s->server = NULL;
	s->server_state = 0;
	s->server_caps = 0;
	s->recv_answer = 0;
	s->cseq = 0;
	s->session = NULL;

	if (user_agent)
		s->user_agent = av_strdup(user_agent);
	else
		s->user_agent =
		    av_strdup("User-Agent: RealMedia Player Version 6.0.9.1235 (linux-2.0-libc6-i386-gcc2.95)");

	s->mrl = av_strdup(mrl);
	s->host = av_strdup(host);
	s->port = port;
	s->path = av_strdup(path);
	while (*path == '/')
		path++;
	if ((s->param = strchr(s->path, '?')) != NULL)
		s->param++;
	//gxlogd("path=%s\n", s->path);
	//gxlogd("param=%s\n", s->param ? s->param : "NULL");
	s->s = fd;

	if (s->s < 0) {
		gxlogd("rtsp: failed to connect to '%s'\n", s->host);
		rtsp_close(s);
		return NULL;
	}

	s->server_state = RTSP_CONNECTED;

	/* now let's send an options request.*/
	rtsp_schedule_field(s, "CSeq: 1");
	rtsp_schedule_field(s, s->user_agent);
	rtsp_schedule_field(s, "ClientChallenge: 9e26d33f2984236010ef6253fb1887f7");
	rtsp_schedule_field(s, "PlayerStarttime: [28/03/2003:22:50:23 00:00]");
	rtsp_schedule_field(s, "CompanyID: KnKV4M4I/B2FjJ1TToLycw==");
	rtsp_schedule_field(s, "GUID: 00000000-0000-0000-0000-000000000000");
	rtsp_schedule_field(s, "RegionData: 0");
	rtsp_schedule_field(s, "ClientID: Linux_2.4_6.0.9.1235_play32_RN01_EN_586");
	/*rtsp_schedule_field(s, "Pragma: initiate-session");*/
	rtsp_request_options(s, NULL);

	return s;
}

/*
*  closes an rtsp connection
*/
void rtsp_close(rtsp_t*  s)
{

	if (s->server_state) {
		if ((s->server_state == RTSP_PLAYING)||(s->server_state == RTSP_PAUSE))
			rtsp_request_teardown(s, NULL);
		closesocket(s->s);
	}

	if (s->path)
		av_free(s->path);
	if (s->host)
		av_free(s->host);
	if (s->mrl)
		av_free(s->mrl);
	if (s->session)
		av_free(s->session);
	if (s->user_agent)
		av_free(s->user_agent);
	rtsp_free_answers(s);
	rtsp_unschedule_all(s);
	av_free(s);
}

int rtsp_get_answers(rtsp_t * s)
{
	return s->recv_answer;
}
/*
*  search in answers for tags. returns a pointer to the content
*  after the first matched tag. returns NULL if no match found.
*/
char* rtsp_search_answers(rtsp_t * s, const char *tag)
{

	char* *answer;
	char* ptr;

	if (!s->answers)
		return NULL;
	answer = s->answers;

	while (*answer) {
		if (!strncasecmp(*answer, tag, strlen(tag))) {
			ptr = strchr(*answer, ':');
			if (!ptr)
				return NULL;
			ptr++;
			while (*ptr == ' ')
				ptr++;
			return ptr;
		}
		answer++;
	}

	return NULL;
}

/*
*  session id management
*/
void rtsp_set_session(rtsp_t*  s, const char *id)
{

	if (s->session)
		av_free(s->session);

	s->session = av_strdup(id);
}

char* rtsp_get_session(rtsp_t * s)
{
	return s->session;
}

char* rtsp_get_mrl(rtsp_t * s)
{
	return s->mrl;
}

char* rtsp_get_param(rtsp_t * s, const char *p)
{
	int len;
	char* param;
	if (!s->param)
		return NULL;
	if (!p)
		return av_strdup(s->param);
	len = strlen(p);
	param = s->param;
	while (param &&* param) {
		char* nparam = strchr(param, '&');
		if (strncmp(param, p, len) == 0 && param[len] == '=') {
			param += len + 1;
			len = nparam ? nparam - param : strlen(param);
			nparam = av_malloc(len + 1);
			memcpy(nparam, param, len);
			nparam[len] = 0;
			return nparam;
		}
		param = nparam ? nparam + 1 : NULL;
	}
	return NULL;
}

/*
*  schedules a field for transmission
*/

void rtsp_schedule_field(rtsp_t*  s, const char *string)
{

	int i = 0;

	if (!string)
		return;

	while (s->scheduled[i]) {
		i++;
	}
	s->scheduled[i] = av_strdup(string);
}

/*
*  removes the first scheduled field which prefix matches string.
*/

void rtsp_unschedule_field(rtsp_t*  s, const char *string)
{

	char* *ptr = s->scheduled;

	if (!string)
		return;

	while (*ptr) {
		if (!strncmp(*ptr, string, strlen(string)))
			break;
		else
			ptr++;
	}
	if (*ptr)
		av_free(*ptr);
	ptr++;
	do {
		*(ptr - 1) =* ptr;
	} while (*ptr);
}

/*
*  unschedule all fields
*/
void rtsp_unschedule_all(rtsp_t*  s)
{

	char* *ptr;

	if (!s->scheduled)
		return;
	ptr = s->scheduled;

	while (*ptr) {
		av_free(*ptr);
		*ptr = NULL;
		ptr++;
	}
}

/*
*  free answers
*/
void rtsp_free_answers(rtsp_t*  s)
{

	char* *answer;

	if (!s->answers)
		return;
	answer = s->answers;

	while (*answer) {
		av_free(*answer);
		*answer = NULL;
		answer++;
	}
}

int rtsp_get_fd(rtsp_t* s)
{
	return s->s;
}

int rtsp_get_respone_answers(rtsp_t* s)
{
	return get_answers(s);
}
