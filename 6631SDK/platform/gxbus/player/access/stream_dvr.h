#ifndef __STREAM_DVR_H__
#define __STREAM_DVR_H__

#include "gx_stream.h"

typedef struct {
	GxStream             parent;
	GxStream            *children;
	GxStreamClass       *sl;
	off_t                fpos;            //读写fpos
	unsigned int         crypt_enable;    //用户加密.
	unsigned int         security_flag;   //安全录制.
	unsigned int         block_size;      //安全录制或者用户加密.
	unsigned char       *data_buf;
	unsigned int         data_size;       //剩余数据大小
	unsigned int         process_flag;    //1:播放器需要处于停止播放状态
	PlayerPvrEncryptMode enc_mode;
} GxStreamDVR;

#endif
