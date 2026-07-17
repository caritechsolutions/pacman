#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "../../vod_trans/vod_trans.h"

#define MAX_BUF_SIZE 2048

static int sock = 0;
static unsigned int thread_id = 0;
static int quit_flag = 0;
static unsigned int server_addr = 0;

static void recv_thread(void* param)
{
	int ret, len, pak_len=0, first;
	unsigned short peer_port;
	unsigned long peer_addr;

	first = 1;
	while (!quit_flag)
	{
		ret = vod_porting_socket_select(sock, 1000);
		if (ret > 0)
		{
			if (first == 1)
			{
				char buf[MAX_BUF_SIZE];
				len = vod_porting_socket_recvfrom(sock, &peer_port, (unsigned long*)&peer_addr, buf, MAX_BUF_SIZE);
				if (len > 0)
				{
					char* p;
					unsigned int token;
					p = (char*)vod_trans_malloc_buffer(len, &token);
					if(p)
					{
						memcpy(p, buf, len);
						vod_trans_insert_buffer(token, 1);
					}
					pak_len = len;
					first = 0;
				}
				else
				{
					GxVod_debug("receiver: udp recv fail\n");
					break;
				}

			}
			else
			{
				unsigned int addr;
				unsigned short port;
				char* buf;
				unsigned int token;

				buf = (char*)vod_trans_malloc_buffer(pak_len, &token);
				if (buf == 0)
				{
					vod_porting_task_delay(100);
					continue;
				}

				len = vod_porting_socket_recvfrom(sock, &port, (unsigned long*)&addr, buf, pak_len);
				if (len > 0)
				{
					if (len != pak_len)
					{
						GxVod_debug("receiver: udp recv size not same, %d %d\n", len, pak_len);
						vod_trans_insert_buffer(token, 0);
						continue;
					}

					if ((port != peer_port) || (addr != peer_addr))
					{
						GxVod_debug("receiver: udp recv addr not same\n");
						vod_trans_insert_buffer(token, 0);
						continue;
					}

					vod_trans_insert_buffer(token, 1);
				}
				else
				{
					vod_trans_insert_buffer(token, 0);
					GxVod_debug("receiver: udp recv fail\n");
					break;				
				}
			}
		}
		else if (ret < 0)
		{
			GxVod_debug("receiver: udp select fail\n");		
			break;
		}
	}

	GxVod_debug("receiver: udp thread quit\n");
}

int coship_receiver_udp_open(char* ip, int port)
{
	quit_flag = 0;

	sock = vod_porting_udp_socket_create();
	if (sock <= 0)
	{
		GxVod_debug("receiver: create udp socket fail\n");
		return -1;
	}

	server_addr = vod_porting_server_name_to_addr(ip);
	if (server_addr == 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: udp ip err\n");
		return -1;
	}

	if (vod_porting_socket_bind(sock, 0, port) < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: udp bind err\n");
		return -1;
	}

	if (vod_porting_socket_join_multicast(sock, 0, server_addr) < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: udp join multicast err\n");
		return -1;
	}

	if (vod_porting_task_create(recv_thread, 0, 10240, 9, &thread_id, (const unsigned char*)"udp_receiver") < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		GxVod_debug("receiver: udp create task err\n");
		return -1;
	}

	return 0;
}

void coship_receiver_udp_close()
{
	if (thread_id)
	{
		quit_flag = 1;
		vod_porting_task_waitdel(thread_id, 1500);
		thread_id = 0;
	}
	if (sock > 0)
	{
		vod_porting_socket_leave_multicast(sock, 0, server_addr);
		vod_porting_socket_close(sock);
		sock = 0;
	}
}

