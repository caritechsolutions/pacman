#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <zlib.h>
#include "gx_url.h"
#include "gx_system.h"

typedef struct {
	const char *mime_type;
	int demuxer_type;
}GxMimeStruct;

typedef enum {
	streaming_stopped_e,
	streaming_playing_e
} GxStreamingStatus;

typedef struct gxstreaming_control {
	GxURL *url;
	GxStreamingStatus status;
	int buffering;	// boolean
	unsigned int prebuffer_size;
	char *buffer;
	unsigned int buffer_size;
	unsigned int buffer_pos;
	unsigned int bandwidth;	// The downstream available
	int (*streaming_read)( int fd, uint8_t *buffer, size_t buffer_size, struct gxstreaming_control *stream_ctrl);
	int (*streaming_seek)( int fd, off_t pos, struct gxstreaming_control *stream_ctrl);
	void* data;
	off_t con_len;  //content length
	off_t res_len;  //content length
	off_t chk_size; //chunk size
	int  duration; //duration
	int  compress;
	z_stream inflate_stream;
	uint8_t  *inflate_buffer;
	unsigned char* uncompress_buffer;
	unsigned int uncompress_buffer_size;
	unsigned int uncompress_buffer_pos;
	void* priv;
	char* cookies;
} GxStreamingCtrl;

//int streaming_start( stream_t *stream, int *demuxer_type, URL_t *url );
GxStreamingCtrl *streaming_ctrl_new(void);
int streaming_bufferize( GxStreamingCtrl *streaming_ctrl, char *buffer, int size);
void streaming_buffer_free(GxStreamingCtrl* streaming_ctrl);

int nop_streaming_read( int fd, uint8_t *buffer, size_t size, GxStreamingCtrl *stream_ctrl );
int nop_streaming_seek(int fd, off_t pos, GxStreamingCtrl*  stream_ctrl);
void streaming_ctrl_free( GxStreamingCtrl *streaming_ctrl );

GxURL* check4proxies(GxURL *url);

#endif

