/*
 * unbuffered io for ffmpeg system
 * copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#ifndef AVIO_H
#define AVIO_H

#include "gx_common.h"
#include "internal.h"


#define AVIO_FLAG_DIRECT 0x8000

#define AVSEEK_FORCE 0x20000
#define AVSEEK_SIZE 0x10000

/**
 * Seeking works like for a local file.
 */
#define AVIO_SEEKABLE_NORMAL (1 << 0)

/**
 * Seeking by timestamp with avio_seek_time() is possible.
*/
#define AVIO_SEEKABLE_TIME   (1 << 1)

#if 0
/**
 * Callback for checking whether to abort blocking functions.
 * AVERROR_EXIT is returned in this case by the interrupted
 * function. During blocking operations, callback is called with
 * opaque as parameter. If the callback returns 1, the
 * blocking operation will be aborted.
 *
 * No members can be added to this struct without a major bump, if
 * new elements have been added after this struct in AVFormatContext
 * or AVIOContext.
 */
typedef struct AVIOInterruptCB {
    int (*callback)(void*);
    void *opaque;
} AVIOInterruptCB;
#endif

/* output byte stream handling */

typedef int64_t offset_t;

/* unbuffered I/O */

typedef struct URLContext {
	struct URLProtocol *prot;
	int flags;
	int is_streamed;
		      /**< true if streamed (no seek possible), default = false */
	int is_connected;
	unsigned int      debug_r_cost_time;
	unsigned int      debug_r_packet_size;
	int max_packet_size;
	int min_packet_size;
			  /**< if non zero, the stream is packetized with this max packet size */
	void *priv_data;
	char *filename; /**< specified filename */
	AVDictionary *url_context_options;
} URLContext;


typedef struct URLPollEntry {
	URLContext *handle;
	int events;
	int revents;
} URLPollEntry;

#define URL_RDONLY  1                                      /**< read-only */
#define URL_WRONLY 2                                      /**< write-only */
#define URL_RDWR (URL_RDONLY|URL_WRONLY)  /**< read-write pseudo flag */

#define AVIO_FLAG_READ  URL_RDONLY           /* *< read-only */
#define AVIO_FLAG_WRITE URL_WRONLY           /* *< write-only */
#define AVIO_FLAG_READ_WRITE URL_RDWR        /* *< read-write pseudo flag */

/**
 * Use non-blocking mode.
 * If this flag is set, operations on the context will return
 * AVERROR(EAGAIN) if they can not be performed immediately.
 * If this flag is not set, operations on the context will never return
 * AVERROR(EAGAIN).
 * Note that this flag does not affect the opening/connecting of the
 * context. Connecting a protocol will always block if necessary (e.g. on
 * network protocols) but never hang (e.g. on busy devices).
 * Warning: non-blocking protocols is work-in-progress; this flag may be
 * silently ignored.
 */
#define AVIO_FLAG_NONBLOCK 8

typedef int URLInterruptCB(void);

int url_connect(URLContext *uc);
int url_alloc(URLContext** h, const char *filename, int flags);
int url_open(URLContext ** h, const char *filename, int flags, AVDictionary **options);
int url_read(URLContext * h, unsigned char *buf, int size);
int url_read_complete(URLContext*  h, unsigned char *buf, int size);
int url_write(URLContext * h, unsigned char *buf, int size);
offset_t url_seek(URLContext * h, offset_t pos, int whence);
int url_closep(URLContext **hh);
int url_close(URLContext * h);
int url_exist(const char *filename);
offset_t url_filesize(URLContext * h);
int url_get_file_handle(URLContext *h);
int64_t url_size(URLContext *h);


/**
 * Return the maximum packet size associated to packetized file
 * handle. If the file is not packetized (stream like http or file on
 * disk), then 0 is returned.
 *
 * @param h file handle
 * @return maximum packet size in bytes
 */
int url_get_max_packet_size(URLContext * h);
void url_get_filename(URLContext * h, char *buf, int buf_size);

/**
 * the callback is called in blocking functions to test regulary if
 * asynchronous interruption is needed. AVERROR(EINTR) is returned
 * in this case by the interrupted function. 'NULL' means no interrupt
 * callback is given. i
 */
int url_check_interrupt_cb(void);

/* not implemented */
int url_poll(URLPollEntry * poll_table, int n, int timeout);

/**
 * Passing this as the "whence" parameter to a seek function causes it to
 * return the filesize without seeking anywhere. Supporting this is optional.
 * If it is not supported then the seek function will return <0.
 */

typedef struct URLProtocol {
	const char *name;
	int (*url_open) (URLContext * h, const char *filename, int flags);
	int (*url_read) (URLContext * h, unsigned char *buf, int size);
	int (*url_write) (URLContext * h, unsigned char *buf, int size);
	offset_t(*url_seek) (URLContext * h, offset_t pos, int whence);
	int (*url_close) (URLContext * h);
	int (*url_get_file_handle)(URLContext *h);
	int (*url_get_multi_file_handle)(URLContext *h, int **handles, int *numhandles);
	int (*url_read_pause)(URLContext *h, int pause);
	int64_t (*url_read_seek)(URLContext *h, int stream_index, int64_t timestamp, int flags);
	int (*url_shutdown)(URLContext *h, int flags);
	int priv_data_size;
	struct URLProtocol *next;
} URLProtocol;

extern URLProtocol *first_protocol;
extern URLInterruptCB *url_interrupt_cb;

typedef struct AVIOInternal {
    URLContext *h;
} AVIOInternal;

int register_protocol(URLProtocol * protocol);

typedef struct {
    unsigned char *buffer;
    int buffer_size;
    int orig_buffer_size;
    unsigned char *buf_ptr, *buf_end;
    void *opaque;
    int (*read_packet)(void *opaque, uint8_t *buf, int buf_size);
    int (*write_packet)(void *opaque, uint8_t *buf, int buf_size);
    int64_t (*seek)(void *opaque, int64_t offset, int whence);
    int64_t pos; /**< position in the file of the current buffer */
    int must_flush; /**< true if the next seek should flush */
    int eof_reached; /**< true if eof reached */
    int write_flag;  /**< true if open for writing */
    int seekable;
    int is_streamed;
    int max_packet_size;
    unsigned long checksum;
    unsigned char *checksum_ptr;
    unsigned long (*update_checksum)(unsigned long checksum, const uint8_t *buf, unsigned int size);
    int error;         ///< contains the error code or 0 if no error happened
    int (*read_pause)(void *opaque, int pause);
    int64_t (*read_seek)(void *opaque, int stream_index,
                         int64_t timestamp, int flags);
	int direct;
     /**
      * Try to buffer at least this amount of data before flushing it
      */
    int min_packet_size;
    /**
     * A callback that is used instead of short_seek_threshold.
     * This is current internal only, do not use from outside.
     */
    int (*short_seek_get)(void *opaque);
    AVDictionary *options;
} ByteIOContext;

typedef ByteIOContext AVIOContext;
//typedef struct {
//	unsigned char *buffer;
//	int buffer_size;
//	unsigned char *buf_ptr, *buf_end;
//	void *opaque;
//	int (*read_packet) (void *opaque, uint8_t * buf, int buf_size);
//	int (*write_packet) (void *opaque, uint8_t * buf, int buf_size);
//	 offset_t(*seek) (void *opaque, offset_t offset, int whence);
//	offset_t pos;
//		  /**< position in the file of the current buffer */
//	int must_flush;
//		    /**< true if the next seek should flush */
//	int eof_reached;
//		     /**< true if eof reached */
//	int write_flag;
//		     /**< true if open for writing */
//	int is_streamed;
//	int max_packet_size;
//	unsigned long checksum;
//	unsigned char *checksum_ptr;
//	unsigned long (*update_checksum) (unsigned long checksum, const uint8_t * buf, unsigned int size);
//	int error;		///< contains the error code or 0 if no error happened
//} ByteIOContext;

int ffio_init_context(ByteIOContext * s,
		  unsigned char *buffer,
		  int buffer_size,
		  int write_flag,
		  void *opaque,
		  int (*read_packet) (void *opaque, uint8_t * buf, int buf_size),
		  int (*write_packet) (void *opaque, uint8_t * buf, int buf_size),
		  offset_t(*seek) (void *opaque, offset_t offset, int whence));

void put_byte(ByteIOContext * s, int b);
void put_buffer(ByteIOContext * s, const unsigned char *buf, int size);
void write_buffer(ByteIOContext *s, const unsigned char *buf, int size);
void put_le64(ByteIOContext * s, uint64_t val);
void put_be64(ByteIOContext * s, uint64_t val);
void put_le32(ByteIOContext * s, unsigned int val);
void put_be32(ByteIOContext * s, unsigned int val);
void put_le24(ByteIOContext * s, unsigned int val);
void put_be24(ByteIOContext * s, unsigned int val);
void put_le16(ByteIOContext * s, unsigned int val);
void put_be16(ByteIOContext * s, unsigned int val);
void put_tag(ByteIOContext * s, const char *tag);

void put_strz(ByteIOContext * s, const char *buf);
int put_str16le(AVIOContext *s, const char *str);

offset_t url_fseek(ByteIOContext * s, offset_t offset, int whence);
offset_t url_fskip(ByteIOContext * s, offset_t offset);
offset_t url_ftell(ByteIOContext * s);
offset_t url_fsize(ByteIOContext * s);
int url_feof(ByteIOContext * s);
int url_ferror(ByteIOContext * s);

#define avio_tell(s)     url_ftell(s)
#define avio_seek(s,o,w) url_fseek(s,o,w)
#define avio_skip(s,o)   url_fskip(s,o)
#define avio_size(s)     url_fsize(s)
#define avio_feof(s)     url_feof(s)

#define URL_EOF (-1)

/** @warning currently size is limited */
#ifdef __GNUC__
int url_fprintf(ByteIOContext * s, const char *fmt, ...) __attribute__ ((__format__(__printf__, 2, 3)));
#else
int url_fprintf(ByteIOContext * s, const char *fmt, ...);
#endif

void put_flush(ByteIOContext * s);

int get_buffer(ByteIOContext * s, unsigned char *buf, int size);
int get_partial_buffer(ByteIOContext * s, unsigned char *buf, int size);

#define avio_read(s,b,size)  get_buffer(s, (unsigned char*)b, size)

/** @note return 0 if EOF, so you cannot use it if EOF handling is
   necessary */
int get_byte(ByteIOContext * s);
unsigned int get_le24(ByteIOContext * s);
unsigned int get_le32(ByteIOContext * s);
uint64_t get_le64(ByteIOContext * s);
unsigned int get_le16(ByteIOContext * s);

char *get_strz(ByteIOContext * s, char *buf, int maxlen);
unsigned int get_be16(ByteIOContext * s);
unsigned int get_be24(ByteIOContext * s);
unsigned int get_be32(ByteIOContext * s);
uint64_t get_be64(ByteIOContext * s);

int get_str(ByteIOContext *s, int maxlen, char *buf, int buflen);
int get_str16le(AVIOContext *pb, int maxlen, char *buf, int buflen);

#define avio_r8(s)    get_byte(s)
#define avio_rl16(s)  get_le16(s)
#define avio_rl24(s)  get_le24(s)
#define avio_rl32(s)  get_le32(s)
#define avio_rl64(s)  get_le64(s)
#define avio_rb16(s)  get_be16(s)
#define avio_rb24(s)  get_be24(s)
#define avio_rb32(s)  get_be32(s)
#define avio_rb64(s)  get_be64(s)
#define avio_get_str(s,a,b,c)      get_str(s,a,b,c)
#define avio_get_str16le(s,a,b,c)  get_str16le(s,a,b,c)


#define avio_w8(s, val)    put_byte(s, val)
#define avio_wl16(s, val)  put_le16(s, val)
#define avio_wl24(s, val)  put_le24(s, val)
#define avio_wl32(s, val)  put_le32(s, val)
#define avio_wl64(s, val)  put_le64(s, val)
#define avio_wb16(s, val)  put_be16(s, val)
#define avio_wb24(s, val)  put_be24(s, val)
#define avio_wb32(s, val)  put_be32(s, val)
#define avio_wb64(s, val)  put_be64(s, val)
#define avio_put_str(s, val)      put_str(s, val)
#define avio_put_str16le(s, val)  put_str16le(s, val)

static inline int url_is_streamed(ByteIOContext * s)
{
	return s->is_streamed;
}

/** @note when opened as read/write, the buffers are only used for
   writing */
int url_fdopen(ByteIOContext** s, URLContext * h);

/** @warning must be called before any I/O */
int url_setbufsize(ByteIOContext * s, int buf_size);

/** @note when opened as read/write, the buffers are only used for
   writing */
int url_fopen(ByteIOContext** s, const char *filename, int flags, AVDictionary **options);
int url_fclose(ByteIOContext * s);
int url_gxlogd(ByteIOContext *s, const char *fmt, ...);
int url_pause(ByteIOContext *s, int pause);
int64_t url_seek_time(ByteIOContext *s, int stream_index, int64_t timestamp, int flags);

/**
 * Open a write only memory stream.
 *
 * @param s new IO context
 * @return zero if no error.
 */
int url_open_dyn_buf(ByteIOContext** s);

/**
 * Open a write only packetized memory stream with a maximum packet
 * size of 'max_packet_size'.  The stream is stored in a memory buffer
 * with a big endian 4 byte header giving the packet size in bytes.
 *
 * @param s new IO context
 * @param max_packet_size maximum packet size (must be > 0)
 * @return zero if no error.
 */
int url_open_dyn_packet_buf(ByteIOContext** s, int max_packet_size);

/**
 * Return the written size and a pointer to the buffer. The buffer
 *  must be freed with av_free().
 * @param s IO context
 * @param pointer to a byte buffer
 * @return the length of the byte buffer
 */
int url_close_dyn_buf(ByteIOContext * s, uint8_t ** pbuffer);

int url_rewind_with_probe_data(ByteIOContext *s, unsigned char *buf, int buf_size);

unsigned long get_checksum(ByteIOContext * s);
void init_checksum(ByteIOContext * s,
		   unsigned long (*update_checksum) (unsigned long c, const uint8_t * p, unsigned int len),
		   unsigned long checksum);

/* udp.c */
int ff_udp_set_remote_url(URLContext * h, const char *uri);
int ff_udp_get_local_port(URLContext * h);
int udp_get_file_handle(URLContext * h);

AVIOContext *avio_alloc_context(
                  unsigned char *buffer,
                  int buffer_size,
                  int write_flag,
                  void *opaque,
                  int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
                  int64_t (*seek)(void *opaque, int64_t offset, int whence));

void ffio_fill(AVIOContext *s, int b, int count);
void avio_write(AVIOContext *s, const unsigned char *buf, int size);
void avio_flush(AVIOContext *s);

static av_always_inline void ffio_wfourcc(AVIOContext *pb, const uint8_t *s)
{
    avio_wl32(pb, MKTAG(s[0], s[1], s[2], s[3]));
}

#define avio_open_dyn_buf            url_open_dyn_buf
#define avio_close_dyn_buf           url_close_dyn_buf
#define avio_gxlogd url_gxlogd

int ffio_fdopen(AVIOContext **s, URLContext *h);
int avio_closep(AVIOContext **s);

int ffurl_close(URLContext *h);

int avio_close(AVIOContext *s);

void avio_context_free(AVIOContext **ps);
char *avio_find_protocol_name(char *url);
int ff_get_chomp_line(AVIOContext *s, char *buf, int maxlen);

URLContext* ffio_geturlcontext(AVIOContext *s);
void ffio_free_dyn_buf(AVIOContext **s);
int ffurl_get_multi_file_handle(URLContext *h, int **handles, int *numhandles);
int ffio_ensure_seekback(AVIOContext *s, int64_t buf_size);
unsigned long ffio_get_checksum(AVIOContext *s);

unsigned long ff_crcA001_update(unsigned long checksum, const uint8_t *buf, unsigned int len);

void ffio_init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum);
int avio_open2(AVIOContext **s, const char *filename, int flags, AVDictionary **options);
int ffio_read_indirect(AVIOContext *s, unsigned char *buf, int size, unsigned char **data);

#endif
