#include "vod_in_common_def.h"
#include "vod_in_typedef.h"

typedef struct 
{
	VOD_RECV_TYPE_t  recvtype;
	
	union
	{
		struct 
		{
			U8 serverip[16];
			U32 serverport;
			
			U8 localip[16];
			U32 localport;
		}tcp_t;
		
		struct 
		{
			U8 serverip[16];
			U32 serverport;
			
			U8 localip[16];
			U32 localport;
		}udp_t;
		
		struct
		{
			U8 serverip[16];
			U32 serverport;
			
			U8 localip[16];
			U32 localport;
		}igmp_t;
		
		struct
		{
			U32 frequency;
			U32 symborate;
			U32 qam_mode;

			U32 pid;
		}qam_t;
	}param;
	
} vod_recv_param_t;



