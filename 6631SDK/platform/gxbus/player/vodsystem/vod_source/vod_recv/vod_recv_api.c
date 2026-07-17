#include "../include/vod_in_common_def.h"
#include "../include/vod_porting_all.h"
#include "../include/vod_recv_api.h"

static vod_recv_param_t g_recvinit_param = {0};

int32_t vod_recv_init(vod_recv_param_t * initparam)
{
	if(initparam == NULL)
	{
		return ERRNO_VOD_PARAM;
	}

	memcpy(&g_recvinit_param, initparam, sizeof(vod_recv_param_t));

	switch(g_recvinit_param.recvtype)
	{
		case VOD_RECV_TYPE_TCPINTERLEAVE:
			break;
		case VOD_RECV_TYPE_UDP:
			break;
		case VOD_RECV_TYPE_IGMP:
			break;
		case VOD_RECV_TYPE_QAM:
			//vod_qam_recver_start();
			break;
		case VOD_RECV_TYPE_TCP:
			break;
		default:
			break;
	}
	return 0;
}

int32_t vod_recv_destroy(void)
{
	switch(g_recvinit_param.recvtype)
	{
		case VOD_RECV_TYPE_TCPINTERLEAVE:
			break;
		case VOD_RECV_TYPE_UDP:
			break;
		case VOD_RECV_TYPE_IGMP:
			break;
		case VOD_RECV_TYPE_QAM:
			//vod_qam_recver_stop();
			break;
		case VOD_RECV_TYPE_TCP:
			break;
		default:
			break;
	}
	return 0;
}


