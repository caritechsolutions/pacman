#ifndef _VOD_IN_COMMON_FUNC_H_
#define _VOD_IN_COMMON_FUNC_H_

/*############################需要修改的接口###################################*/
char* str_get_field(char* buffer, char* str, char* word);
char* str_save(char* word);
void str_free(char* word);
char* time_to_rtspstr(unsigned int time);
unsigned int get_url_time(int word, char* url);
int socket_recv_by_linebreak(int sock, char* buffer, int len, int timeout);
int socket_recv_by_length(int sock, char* recvbuf, int body_len, int timeout);
unsigned int rtspstr_to_time(char* timestr);
unsigned int urlstr_to_time(char* timestr);
int64_t get_tick_time(void);
/*#############################################################################*/

#endif
