#include "../../include/vod_in_typedef.h"
#include "receiver.h"

int coship_receiver_tcp_open(char* ip, int port, char* url, char* session, char* key);
void coship_receiver_tcp_reset();
void coship_receiver_tcp_close();
int coship_receiver_udp_open(char* ip, int port);
void coship_receiver_udp_close();


static int receiver_io_type;

int coship_receiver_open(coship_receiver_param_t* param)
{
	int ret = 0;

	if (param->io_type == IO_TCP)
	{
		ret = coship_receiver_tcp_open(param->ip, 
				param->port,
				param->url,
				param->session,
				param->key);
	}
	else if (param->io_type == IO_UDP)
	{
		ret = coship_receiver_udp_open(param->ip, 
				param->port);
	}

	receiver_io_type = param->io_type;

	return ret;
}

void coship_receiver_reset()
{
	if (receiver_io_type == IO_TCP)
	{
		coship_receiver_tcp_reset();
	}
}

void coship_receiver_close()
{
	if (receiver_io_type == IO_TCP)
	{
		coship_receiver_tcp_close();
	}
	else if(receiver_io_type == IO_UDP)
	{
		coship_receiver_udp_close();
	}
}

