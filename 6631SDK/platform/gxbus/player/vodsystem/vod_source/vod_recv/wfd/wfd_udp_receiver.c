#include "../../include/vod_in_common_def.h"
#include "../../include/vod_porting_all.h"
#include "../../vod_trans/vod_trans.h"
#include "wfd_receiver.h"

#define WFD_UDP_PACKET_SIZE 1470
#define WFD_UDP_PVR_DEBUG 0

#if WFD_UDP_PVR_DEBUG
#define wfd_upd_pvr_path "/media/sda1/net_pvr/wfd_1.ts"
static FILE* wfd_pvr_fd = NULL;
#endif

static int sock = 0;
static unsigned int wfd_thread_id = 0;
static int quit_flag = 0;
static unsigned long server_addr = 0;
static int64_t udp_packet_num=0;

static int wfd_receiver_one_packt(char* buffer, int recv_len)
{
	int ret = 0;
	unsigned short peer_port;
	unsigned long peer_addr;

    ret = vod_porting_socket_select(sock, 100);
    if(ret > 0){
        ret = vod_porting_socket_recvfrom(sock, &peer_port, (unsigned long*)&peer_addr, buffer, recv_len);
        if(ret > 0){
            if(server_addr != peer_addr){
                gxlogd("[%s-%d]: udp port not same[%ld %ld]\n", __FUNCTION__, __LINE__, server_addr,  peer_addr);
                ret = -1;
            }
        }else{
            gxlogd("[%s-%d]: udp recv fail\n", __FUNCTION__, __LINE__);
            return -1;
        }
    }else if(ret < 0){
        gxlogd("[%s-%d]: udp select fail\n", __FUNCTION__, __LINE__);
        return -1;
    }
    udp_packet_num++;
	return ret;
}

static int wfd_ts_sync(char* buf, int len, int* l, int* sync_l)
{
    int sync_len, recv_len;

    recv_len = wfd_receiver_one_packt(buf, len);
	if(recv_len < 0){
		gxlogd("[%s-%d]: ts sync error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	sync_len = vod_trans_sync(buf, recv_len);
	if(sync_len < 0){
		gxlogd("[%s-%d]: ts sync error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if(sync_len >= recv_len){
		gxlogd("[%s-%d]: ts sync error\n", __FUNCTION__, __LINE__);
		return -1;
	}

	*l = recv_len-sync_len;
	*sync_l = sync_len;

	return 0;
}

static void wfd_recv_thread(void* param)
{
	int ret = 0, len = 0, sync_len = 0;
	char buf[WFD_UDP_PACKET_SIZE];
	unsigned int token = 0;

    while (!quit_flag){
        ret = wfd_ts_sync(buf, WFD_UDP_PACKET_SIZE, &len, &sync_len);
        if(ret < 0){
            gxlogd("[%s-%d]: wfd packet num [%lld]\n", __FUNCTION__, __LINE__, udp_packet_num);
            continue;
        }else {
            char *p = (char*)vod_trans_malloc_buffer(len, &token);
            if(p == NULL){
                gxlogd("[%s-%d]: wfd malloc failed\n", __FUNCTION__, __LINE__);
                break;
            }
            memcpy(p, buf+sync_len, len);
            if((len%188)){
                gxlogd("[%s-%d]: recv len(%d) not eq 188*10\n", __FUNCTION__, __LINE__, len);
            }
			
#if WFD_UDP_PVR_DEBUG
			if(wfd_pvr_fd && len > 0){
				fwrite(p, len, 1, wfd_pvr_fd);
			}
#endif
            ret = vod_trans_insert_buffer(token, 1);
			if(ret < 0)
				gxlogd("[%s-%d]: lost packet [%lld]\n", __FUNCTION__, __LINE__, udp_packet_num);
        }
    }

    gxlogd("[%s-%d]: udp thread quit\n", __FUNCTION__, __LINE__);
}

int wfd_receiver_udp_open(char* ip, int port)
{
	quit_flag = 0;
    udp_packet_num = 0;

	sock = vod_porting_udp_socket_create();
	if (sock <= 0)
	{
		gxlogd("[%s-%d]: create udp socket fail\n", __FUNCTION__, __LINE__);
		return -1;
	}

	server_addr = vod_porting_server_name_to_addr(ip);
	if (server_addr == 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		gxlogd("[%s-%d]: udp ip err\n", __FUNCTION__, __LINE__);
		return -1;
	}

	if (vod_porting_socket_bind(sock, 0, port) < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		gxlogd("[%s-%d]: udp bind err\n", __FUNCTION__, __LINE__);
		return -1;
	}

//	if (vod_porting_socket_join_multicast(sock, 0, server_addr) < 0)
//	{
//		vod_porting_socket_close(sock);
//		sock = 0;
//		gxlogd("[%s-%d]: udp join multicast err\n", __FUNCTION__, __LINE__);
//		return -1;
//	}

#if WFD_UDP_PVR_DEBUG
	if(wfd_pvr_fd)
		fclose(wfd_pvr_fd);
	wfd_pvr_fd = fopen(wfd_udp_prv_path, "w+");
#endif

	if (vod_porting_task_create(wfd_recv_thread, 0, 8*1024, 9,
				&wfd_thread_id, (const unsigned char*)"udp_receiver") < 0)
	{
		vod_porting_socket_close(sock);
		sock = 0;
		gxlogd("[%s-%d]: udp create task err\n", __FUNCTION__, __LINE__);
		return -1;
	}

	return 0;
}

void wfd_receiver_udp_close()
{
	if (wfd_thread_id)
	{
		quit_flag = 1;
		vod_porting_task_waitdel(wfd_thread_id, 1500);
		wfd_thread_id = 0;
	}
	if (sock > 0)
	{
//		vod_porting_socket_leave_multicast(sock, 0, server_addr);
		vod_porting_socket_close(sock);
		sock = 0;
	}
	
#if WFD_UDP_PVR_DEBUG
	if(wfd_pvr_fd)
		fclose(wfd_pvr_fd);
	wfd_pvr_fd = NULL;
#endif
}

