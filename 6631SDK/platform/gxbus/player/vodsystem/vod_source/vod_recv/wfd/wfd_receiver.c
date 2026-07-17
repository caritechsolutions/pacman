#include "../../include/vod_in_typedef.h"
#include "wfd_receiver.h"

static int wfd_receiver_io_type;

int wfd_receiver_open(wfd_receiver_param_t* param)
{
	int ret = 0;

	if (param->io_type == IO_TCP)
	{
		ret = wfd_receiver_tcp_open(param->ip, 
				param->port,
				param->url,
				param->session,
				param->key);
	}
	else if (param->io_type == IO_UDP)
	{
		ret = wfd_receiver_udp_open(param->ip, 
				param->port);
	}

	wfd_receiver_io_type = param->io_type;

	return ret;
}

void wfd_receiver_reset()
{
	if (wfd_receiver_io_type == IO_TCP)
	{
		wfd_receiver_tcp_reset();
	}
}

void wfd_receiver_close()
{
	if (wfd_receiver_io_type == IO_TCP)
	{
		wfd_receiver_tcp_close();
	}
	else if(wfd_receiver_io_type == IO_UDP)
	{
		wfd_receiver_udp_close();
	}
}

