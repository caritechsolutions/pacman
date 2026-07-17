/*--------------------------------------------------------

	COPYRIGHT 2008 (C) Hangzhou Motorola Technology Ltd

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	coship vod lib
	CREATED:	2008-7-7

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#ifndef _RTSP_UTIL_H_
#define _RTSP_UTIL_H_ 

#define RTSP_RECV_BUFFER_SIZE	2048
#define RTSP_SEND_BUFFER_SIZE	1024

#define RESP_CLOSE		1
#define RESP_EOS		2
#define RESP_BOS		3

#define RESP_ERR_SOCK		-1
#define RESP_ERR_TIMEOUT	-2
#define RESP_ERR_MEMORY	-3
#define RESP_ERR_PARSE		-4
#define RESP_ERR_SEQUENCE	-5
#define RESP_ERR_CODE		-6

typedef struct rtsp_client_t 
{
	int sock;
	char* url;
	char* session;
	int seq;
	int timeout;
	unsigned int resp_sem;
	unsigned int resp_mutex;
	char* recvbuf;
	char* sendbuf;
	int32_t shift_vod;
	int32_t shift_pos;
}rtsp_client_t;

typedef struct
{
	char* title;
	char* range;
	char* transport;
	char* content_type;
	char* content;
	char* keys;
	int scale; 
	int rebuild;
}rtsp_cmd_t;

typedef struct
{
	int retcode;
	int seq;
	char* session;
	int program_no;
	int audiopid;
	int videopid;
	int pcrpid;
	int freq;
	int symbrate;
	int qamsize;
	int trickmode;
	int duration;	
	char* start_time;
}rtsp_resp_describe_t;

typedef struct
{
	int retcode;
	int seq;
	char* dst_ip;
	int dst_port;	
	char* keystring;	
}rtsp_resp_setup_t;

typedef struct
{
	int retcode;
	int seq;
	int position;
	int close;
}rtsp_resp_comm_t;

typedef struct
{
	int64_t start_pos;
	int64_t really_time;
	int64_t read_position_time;
}rtsp_time_info_t;

int coship_rtsp_send_command(rtsp_client_t *client, rtsp_cmd_t *cmd);
int coship_rtsp_recv_response_comm(rtsp_client_t *client, rtsp_resp_comm_t* resp, rtsp_time_info_t* rtsp_time_info);
int coship_rtsp_recv_response_describe(rtsp_client_t * client, rtsp_resp_describe_t * resp);
int coship_rtsp_recv_response_setup(rtsp_client_t * client, rtsp_resp_setup_t * resp, rtsp_resp_describe_t * describe_resp);

#endif

