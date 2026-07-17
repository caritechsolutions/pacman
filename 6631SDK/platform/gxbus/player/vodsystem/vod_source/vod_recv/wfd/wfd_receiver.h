#ifndef _WFD_RECEIVER_H_
#define _WFD_RECEIVER_H_ 

typedef struct
{
	int io_type;
	char* ip;
	int port;
	char* url;
	char* session;
	char* key;
}wfd_receiver_param_t;

int wfd_receiver_open(wfd_receiver_param_t* param);
void wfd_receiver_close();

int wfd_receiver_tcp_open(char* ip, int port, char* url, char* session, char* key);
void wfd_receiver_tcp_reset();
void wfd_receiver_tcp_close();
int wfd_receiver_udp_open(char* ip, int port);
void wfd_receiver_udp_close();
#endif

