#include "../../include/vod_in_common_def.h"
#include "../../include/vod_common_func.h"
#include "../../include/vod_porting_all.h"
#include "../../vod_trans/vod_trans.h"

#define MAX_DATA_BUF_SIZE 2048

static int sock = 0;
static unsigned int thread_id = 0;
static int quit_flag = 0;
static int align_flag = 0;
static char cmd_pattern[] = 
"VALIDATE %s RTSP/1.0\r\n\
CSeq: 1\r\n\
Session: %s\r\n\
Keys: %s\r\n\
\r\n";


static int block_recv_data(char* buffer, int buflen)
{
	int ret, left;
	char* p;

	left = buflen;
	p = buffer;
	while (left > 0)
	{
		ret = vod_porting_socket_select(sock, 1000);
		if (ret > 0)
		{
			ret = vod_porting_socket_recv(sock, p, left);
			if (ret > 0)
			{
				p += ret;
				left -= ret;
			}
			else
			{
				GxVod_debug("receiver: tcp recv err\n");
				return -1;
			}
		}
		else if (ret < 0)
		{
			GxVod_debug("receiver: tcp select err\n");
			return -1;
		}

		if (quit_flag)
		{
			return -1;
		}
	}
	return 0;
}

static void recv_thread(void* param)
{
	int state, len = 0;
	unsigned char seq = 0;
	unsigned char head[4];

	state = 0;
	align_flag = 1;
	while (!quit_flag)
	{
		if (state == 0)
		{
			if (block_recv_data((char*)head, 1) < 0)
				break;

			if (head[0] == '$')
				state = 1;
			else
				GxVod_debug("receiver: tcp get $ fail\n");
		}
		else if (state == 1)
		{
			if (block_recv_data((char*)head, 3) < 0)
				break;

			seq = head[0];
			len = head[2] << 8 | head[1];
			state = 2;
			if (seq == 0)
			{
				gxlogd("receiver: tcp seq = 0\n");
			}
		}
		else
		{
			if (len <= MAX_DATA_BUF_SIZE)
			{
				unsigned int token;
				char* buf;

				buf = (char*)vod_trans_malloc_buffer(len, &token);
				if (buf == 0)
				{
					vod_porting_task_delay(100);
					continue;
				}
				if (block_recv_data(buf, len) < 0)
				{
					vod_trans_insert_buffer(token, 0);
					break;
				}
				else
				{
					if (align_flag)
					{
						if (seq == 0)
							align_flag = 0;
						else
						{
							state = 0;
							gxlogd("receiver: remove data, seq=%d\n", seq);
							vod_trans_insert_buffer(token, 0);
							continue;
						}
					}
					vod_trans_insert_buffer(token, 1);
					state = 0;
				}
			}
			else
			{
				GxVod_debug("receiver: tcp size > MAX_BUFFER_SIZE, %d\n", len);
				break;
			}
		}
	}
	GxVod_debug("receiver: tcp thread quit\n");
}

static void send_cmd(char* url, char* session, char* key)
{
	char buffer[512];

	sprintf(buffer, cmd_pattern, url, session, key);
	vod_porting_socket_send(sock, buffer, strlen(buffer));
}

static int recv_resp()
{
	char buffer[512];

	if (socket_recv_by_linebreak(sock, buffer, 512, 1500) == 0)
	{
		if (strstr(buffer, "RTSP/1.0 200 OK"))
			return 0;
	}
	return -1;
}

int coship_receiver_tcp_open(char* ip, int port, char* url, char* session, char* key)
{
	unsigned int addr;

	quit_flag = 0;

	sock = vod_porting_tcp_socket_create();
	if (sock <= 0)
	{
		GxVod_debug("receiver: create tcp socket fail\n");
		return -1;
	}

	addr = vod_porting_server_name_to_addr(ip);
	if (addr == 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: tcp ip err\n");
		return -1;
	}

	if (vod_porting_socket_connect(sock, port, addr) < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: tcp connect err\n");
		return -1;
	}

	send_cmd(url, session, key);
	if (recv_resp() != 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: tcp resp err\n");
		return -1;
	}

	if (vod_porting_task_create(recv_thread, 0, 10240, 9, &thread_id, (const unsigned char*)"tcp_receiver") < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: tcp create task err\n");
		return -1;
	}

	return 0;
}

void coship_receiver_tcp_reset()
{
	align_flag = 1;
}

void coship_receiver_tcp_close()
{
	if (thread_id)
	{
		quit_flag = 1;
		vod_porting_task_waitdel(thread_id, 1500);
		thread_id = 0;
	}
	if (sock > 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
	}
}

