/*--------------------------------------------------------

	COPYRIGHT 2008 (C) Hangzhou Motorola Technology Ltd

	AUTHOR:		wangry@dvnchina.com
	PURPOSE:	coship vod lib
	CREATED:	2008-7-7

	MODIFICATION HISTORY
	Date        By     Details
	----------  -----  -------

--------------------------------------------------------*/
#ifndef _COSHIP_RECEIVER_H_
#define _COSHIP_RECEIVER_H_ 

typedef struct
{
	int io_type;
	char* ip;
	int port;
	char* url;
	char* session;
	char* key;
}coship_receiver_param_t;

int coship_receiver_open(coship_receiver_param_t* param);
void coship_receiver_reset();
void coship_receiver_close();

#endif

