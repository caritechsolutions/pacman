#ifndef TCP_H
#define TCP_H
#include "gx_system.h"
#include "gx_stream.h"

/* Connect to a server using a TCP connection */
int connect_2_server (char *host, int port, int verb, PLAYER_INTERRUPT_CBK interrupt_cbk);
#endif /* TCP_H*/
