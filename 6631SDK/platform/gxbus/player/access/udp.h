#ifndef UDP_H
#define UDP_H
#include <sys/socket.h>

#define STREAM_UDP_MAX_COUNT    64
#define STREAM_UDP_MAX_PKT_SIZE 1472//1316

typedef struct {
    int udp_fd;
    int ttl;
    int buffer_size;
    int is_multicast;
    int is_broadcast;
    int local_port;
    int reuse_socket;
    int overrun_nonfatal;
    struct sockaddr_storage dest_addr;
    int dest_addr_len;
	int error_count;
	int udp_data_len;
	int udp_data_pos;
	char udp_data[STREAM_UDP_MAX_PKT_SIZE];
} UDPContext;

UDPContext* Gx_UDP_Open4Server(GxURL *uri);
UDPContext* Gx_UDP_Open(GxURL *uri);
int Gx_UDP_Read(UDPContext* s, uint8_t *buf, int size);
int Gx_UDP_Write(UDPContext* s, uint8_t *buf, int size);
void Gx_UDP_Close(UDPContext* s);
#endif /* UDP_H */

