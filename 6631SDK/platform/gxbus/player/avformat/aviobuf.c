/*
*  Buffered I/O for ffmpeg system
*  Copyright (c) 2000,2001 Fabrice Bellard
*
*  This file is part of FFmpeg.
*
*  FFmpeg is free software; you can redistribute it and/or
*  modify it under the terms of the GNU Lesser General Public
*  License as published by the Free Software Foundation; either
*  version 2.1 of the License, or (at your option) any later version.
*
*  FFmpeg is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU Lesser General Public
*  License along with FFmpeg; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "avformat.h"
#include "avio.h"
#include <stdarg.h>
#include "../avutil/opt.h"
#include "../avutil/crc.h"

#define IO_BUFFER_SIZE 32768

/**
 * Do seeks within this distance ahead of the current buffer by skipping
 * data instead of calling the protocol seek function, for seekable
 * protocols.
 */
#define SHORT_SEEK_THRESHOLD 32768//4096
static void fill_buffer(ByteIOContext *s);
static int url_resetbuf(ByteIOContext *s, int flags);

int ffio_init_context(ByteIOContext *s,
		unsigned char *buffer,
		int buffer_size,
		int write_flag,
		void *opaque,
		int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
		int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
		int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
	s->buffer = buffer;
	s->orig_buffer_size =
	s->buffer_size = buffer_size;
	s->buf_ptr = buffer;
	s->opaque = opaque;
	s->direct = 0;
	url_resetbuf(s, write_flag ? URL_WRONLY : URL_RDONLY);
	s->write_packet = write_packet;
	s->read_packet = read_packet;
	s->seek = seek;
	s->pos = 0;
	s->must_flush = 0;
	s->eof_reached = 0;
	s->error = 0;
	s->seekable = seek ? AVIO_SEEKABLE_NORMAL : 0;
	s->max_packet_size = 0;
	s->update_checksum= NULL;
	if(!read_packet && !write_flag){
		s->pos = buffer_size;
		s->buf_end = s->buffer + buffer_size;
	}
	s->read_pause = NULL;
	s->read_seek  = NULL;
	return 0;
}

ByteIOContext *avio_alloc_context(
		unsigned char *buffer,
		int buffer_size,
		int write_flag,
		void *opaque,
		int (*read_packet)(void *opaque, uint8_t *buf, int buf_size),
		int (*write_packet)(void *opaque, uint8_t *buf, int buf_size),
		int64_t (*seek)(void *opaque, int64_t offset, int whence))
{
	ByteIOContext *s = av_mallocz(sizeof(ByteIOContext));
	if (!s)
		return NULL;
	ffio_init_context(s, buffer, buffer_size, write_flag, opaque,
			read_packet, write_packet, seek);
	return s;
}

static void writeout(ByteIOContext *s, const uint8_t *data, int len)
{
    if (s->write_packet && !s->error){
        int ret= s->write_packet(s->opaque, (uint8_t*)data, len);
        if(ret < 0){
            s->error = ret;
        }
    }
    s->pos += len;
}

static void flush_buffer(ByteIOContext*  s)
{
    if (s->buf_ptr > s->buffer) {
        writeout(s, s->buffer, s->buf_ptr - s->buffer);
        if(s->update_checksum){
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
            s->checksum_ptr= s->buffer;
        }
    }
    s->buf_ptr = s->buffer;
}

void put_byte(ByteIOContext*  s, int b)
{
	*(s->buf_ptr)++ = b;
	if (s->buf_ptr >= s->buf_end)
		flush_buffer(s);
}

void put_buffer(ByteIOContext*  s, const unsigned char *buf, int size)
{
	int len;

	while (size > 0) {
		len = (s->buf_end - s->buf_ptr);
		if (len > size)
			len = size;
		memcpy(s->buf_ptr, buf, len);
		s->buf_ptr += len;

		if (s->buf_ptr >= s->buf_end)
			flush_buffer(s);

		buf += len;
		size -= len;
	}
}

void write_buffer(ByteIOContext *s, const unsigned char *buf, int size)
{
    if (s->direct && !s->update_checksum) {
        put_flush(s);
        writeout(s, buf, size);
        return;
    }
    while (size > 0) {
        int len = FFMIN(s->buf_end - s->buf_ptr, size);
        memcpy(s->buf_ptr, buf, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        buf += len;
        size -= len;
    }
}

void put_flush(ByteIOContext*  s)
{
	flush_buffer(s);
	s->must_flush = 0;
}

offset_t url_fseek(ByteIOContext*  s, offset_t offset, int whence)
{
    offset_t offset1;
	offset_t pos;
    int force = whence & AVSEEK_FORCE;
    whence &= ~AVSEEK_FORCE;

    if (!s)
        return AVERROR(EINVAL);

    pos = s->pos - (s->write_flag ? 0 : (s->buf_end - s->buffer));

    if (whence != SEEK_CUR && whence != SEEK_SET)
        return AVERROR(EINVAL);

    if (whence == SEEK_CUR) {
        offset1 = pos + (s->buf_ptr - s->buffer);
        if (offset == 0)
            return offset1;
        offset += offset1;
    }
    offset1 = offset - pos;
    if (!s->must_flush && (!s->direct || !s->seek) &&
        offset1 >= 0 && offset1 <= (s->buf_end - s->buffer)) {
        /* can do the seek inside the buffer */
        s->buf_ptr = s->buffer + offset1;
    } else if ((!s->seekable ||
               offset1 <= s->buf_end + SHORT_SEEK_THRESHOLD - s->buffer) &&
               !s->write_flag && offset1 >= 0 &&
               (!s->direct || !s->seek) &&
              (whence != SEEK_END || force)) {
        while(s->pos < offset && !s->eof_reached)
            fill_buffer(s);
        if (s->eof_reached)
            return AVERROR_EOF;
        s->buf_ptr = s->buf_end + offset - s->pos;
    } else {
        offset_t res;

        if (s->write_flag) {
            flush_buffer(s);
            s->must_flush = 1;
        }
        if (!s->seek)
            return AVERROR(ENOSYS);
        if ((res = s->seek(s->opaque, offset, SEEK_SET)) < 0)
            return res;
        if (!s->write_flag)
            s->buf_end = s->buffer;
        s->buf_ptr = s->buffer;
        s->pos = offset;
    }
    s->eof_reached = 0;
    return offset;
}

offset_t url_fskip(ByteIOContext*  s, offset_t offset)
{
	return url_fseek(s, offset, SEEK_CUR);
}

offset_t url_ftell(ByteIOContext*  s)
{
	return url_fseek(s, 0, SEEK_CUR);
}

offset_t url_fsize(ByteIOContext*  s)
{
	offset_t size;

    if(!s)
        return AVERROR(EINVAL);

	if (!s->seek)
		return AVERROR(ENOSYS);
	size = s->seek(s->opaque, 0, AVSEEK_SIZE);
	if (size < 0) {
		if ((size = s->seek(s->opaque, -1, SEEK_END)) < 0)
			return size;
		size++;
		s->seek(s->opaque, s->pos, SEEK_SET);
	}
	return size;
}

int url_feof(ByteIOContext*  s)
{
    if(!s)
        return 0;
    if(s->eof_reached){
        s->eof_reached=0;
        fill_buffer(s);
    }
    return s->eof_reached;
}

int url_ferror(ByteIOContext*  s)
{
	return s->error;
}

void put_le32(ByteIOContext*  s, unsigned int val)
{
	put_byte(s, val);
	put_byte(s, val >> 8);
	put_byte(s, val >> 16);
	put_byte(s, val >> 24);
}

void put_be32(ByteIOContext*  s, unsigned int val)
{
	put_byte(s, val >> 24);
	put_byte(s, val >> 16);
	put_byte(s, val >> 8);
	put_byte(s, val);
}

void put_strz(ByteIOContext*  s, const char *str)
{
	if (str)
		put_buffer(s, (const unsigned char* )str, strlen(str) + 1);
	else
		put_byte(s, 0);
}

int put_str16le(ByteIOContext *s, const char *str)
{
    const uint8_t *q = (const uint8_t*)str;
    int ret = 0;

    while (*q) {
        uint32_t ch;
        uint16_t tmp;

        GET_UTF8(ch, *q++, break;)
        PUT_UTF16(ch, tmp, put_le16(s, tmp);ret += 2;)
    }
    put_le16(s, 0);
    ret += 2;
    return ret;
}

void put_le64(ByteIOContext*  s, uint64_t val)
{
	put_le32(s, (uint32_t) (val & 0xffffffff));
	put_le32(s, (uint32_t) (val >> 32));
}

void put_be64(ByteIOContext*  s, uint64_t val)
{
	put_be32(s, (uint32_t) (val >> 32));
	put_be32(s, (uint32_t) (val & 0xffffffff));
}

void put_le16(ByteIOContext*  s, unsigned int val)
{
	put_byte(s, val);
	put_byte(s, val >> 8);
}

void put_be16(ByteIOContext*  s, unsigned int val)
{
	put_byte(s, val >> 8);
	put_byte(s, val);
}

void put_le24(ByteIOContext*  s, unsigned int val)
{
	put_le16(s, val & 0xffff);
	put_byte(s, val >> 16);
}

void put_be24(ByteIOContext*  s, unsigned int val)
{
	put_be16(s, val >> 8);
	put_byte(s, val);
}

void put_tag(ByteIOContext*  s, const char *tag)
{
	while (*tag) {
		put_byte(s,* tag++);
	}
}

/* Input stream*/

static void fill_buffer(ByteIOContext*  s)
{
    uint8_t *dst= !s->max_packet_size && s->buf_end - s->buffer < s->buffer_size ? s->buf_end : s->buffer;
    int len= s->buffer_size - (dst - s->buffer);
    int max_buffer_size = s->max_packet_size ? s->max_packet_size : IO_BUFFER_SIZE;

    /* can't fill the buffer without read_packet, just set EOF if appropiate */
    if (!s->read_packet && s->buf_ptr >= s->buf_end)
        s->eof_reached = 1;

    /* no need to do anything if EOF already reached */
    if (s->eof_reached)
        return;

    if(s->update_checksum && dst == s->buffer){
        if(s->buf_end > s->checksum_ptr)
            s->checksum= s->update_checksum(s->checksum, s->checksum_ptr, s->buf_end - s->checksum_ptr);
        s->checksum_ptr= s->buffer;
    }

    /* make buffer smaller in case it ended up large after probing */
    if (s->read_packet && s->orig_buffer_size && s->buffer_size > s->orig_buffer_size && len >= s->orig_buffer_size) {
      if (dst == s->buffer && s->buf_ptr != dst) {
            int ret = url_setbufsize(s, s->orig_buffer_size);
            if (ret < 0)
                gxlogd ("Failed to decrease buffer size\n");

            s->checksum_ptr = dst = s->buffer;
        }
        len = s->orig_buffer_size;
    }

    if(s->read_packet) {
        len = s->read_packet(s->opaque, dst, len);
    } else
        len = 0;
    if (len == AVERROR_EOF) {
        /* do not modify buffer if EOF reached so that a seek back can
           be done without rereading data */
        s->eof_reached = 1;
    } else if (len < 0) {
        s->eof_reached = 1;
        s->error= len;
    } else {
        s->pos += len;
        s->buf_ptr = dst;
        s->buf_end = dst + len;
    }
}

unsigned long crc04C11DB7_update(unsigned long checksum, const uint8_t *buf,
                                    unsigned int len)
{
    return av_crc(av_crc_get_table(AV_CRC_32_IEEE), checksum, buf, len);
}

unsigned long get_checksum(ByteIOContext*  s)
{
	s->checksum = s->update_checksum(s->checksum, s->checksum_ptr, s->buf_ptr - s->checksum_ptr);
	s->update_checksum = NULL;
	return s->checksum;
}

void init_checksum(ByteIOContext*  s,
		   unsigned long (*update_checksum) (unsigned long c, const uint8_t*  p, unsigned int len),
		   unsigned long checksum)
{
	s->update_checksum = update_checksum;
	if (s->update_checksum) {
		s->checksum = checksum;
		s->checksum_ptr = s->buf_ptr;
	}
}

/* XXX: put an inline version*/
int get_byte(ByteIOContext*  s)
{
	if (s->buf_ptr < s->buf_end) {
		return* s->buf_ptr++;
	} else {
		fill_buffer(s);
		if (s->buf_ptr < s->buf_end)
			return* s->buf_ptr++;
		else
			return 0;
	}
}

int get_buffer(ByteIOContext*  s, unsigned char *buf, int size)
{
	int len, size1;

	size1 = size;
	while (size > 0) {
		len = FFMIN(s->buf_end - s->buf_ptr, size);
		if (len == 0) {
			if (size > s->buffer_size && !s->update_checksum && s->read_packet) {
				len = s->read_packet(s->opaque, buf, size);
				if (len == AVERROR_EOF) {
                    /* do not modify buffer if EOF reached so that a seek back can be done without rereading data */
					s->eof_reached = 1;
					break;
				} else if (len < 0) {
					s->eof_reached = 1;
					s->error = len;
					break;
				} else {
					s->pos += len;
					size -= len;
					buf += len;
					s->buf_ptr = s->buffer;
					s->buf_end = s->buffer /* + len*/ ;
				}
			} else {
				fill_buffer(s);
				len = s->buf_end - s->buf_ptr;
				if (len == 0)
					break;
			}
		} else {
			memcpy(buf, s->buf_ptr, len);
			buf += len;
			s->buf_ptr += len;
			size -= len;
		}
	}
	if (size1 == size) {
		if (s->error)
			return s->error;
		if (avio_feof(s))
			return AVERROR_EOF;
	}
	return size1 - size;
}

int get_partial_buffer(ByteIOContext*  s, unsigned char *buf, int size)
{
	int len;

	if (size < 0)
		return -1;

	len = s->buf_end - s->buf_ptr;
	if (len == 0) {
		fill_buffer(s);
		len = s->buf_end - s->buf_ptr;
        if (s->eof_reached)
            return AVERROR_EOF;
	}
	if (len > size)
		len = size;
	memcpy(buf, s->buf_ptr, len);
	s->buf_ptr += len;
	return len;
}

unsigned int get_le16(ByteIOContext*  s)
{
	unsigned int val;
	val = get_byte(s);
	val |= get_byte(s) << 8;
	return val;
}

unsigned int get_be16(ByteIOContext*  s)
{
	unsigned int val;
	val = get_byte(s) << 8;
	val |= get_byte(s);
	return val;
}

unsigned int get_le24(ByteIOContext*  s)
{
	unsigned int val;
	val = get_le16(s);
	val |= get_byte(s) << 16;
	return val;
}

unsigned int get_be24(ByteIOContext*  s)
{
	unsigned int val;
	val = get_be16(s) << 8;
	val |= get_byte(s);
	return val;
}

unsigned int get_le32(ByteIOContext*  s)
{
	unsigned int val;
	val = get_le16(s);
	val |= get_le16(s) << 16;
	return val;
}

uint64_t get_le64(ByteIOContext*  s)
{
	uint64_t val;
	val = (uint64_t) get_le32(s);
	val |= (uint64_t) get_le32(s) << 32;
	return val;
}

unsigned int get_be32(ByteIOContext*  s)
{
	unsigned int val;
	val = get_be16(s) << 16;
	val |= get_be16(s);
	return val;
}

char* get_strz(ByteIOContext * s, char *buf, int maxlen)
{
	int i = 0;
	char c;

	while ((c = get_byte(s))) {
		if (i < maxlen - 1)
			buf[i++] = c;
	}

	buf[i] = 0;		/* Ensure null terminated, but may be truncated*/

	return buf;
}

int get_line(ByteIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    char c;

    do {
        c = get_byte(s);
        if (c && i < maxlen-1)
            buf[i++] = c;
    } while (c != '\n' && c);

    buf[i] = 0;
    return i;
}

int get_str(ByteIOContext *s, int maxlen, char *buf, int buflen)
{
	int i;

	if (buflen <= 0)
		return AVERROR(EINVAL);
	//reserve 1 byte for terminating 0
	buflen = GXMIN(buflen - 1, maxlen);
	for (i = 0; i < buflen; i++)
		if (!(buf[i] = get_byte(s)))
			return i + 1;
	buf[i] = 0;
	for (; i < maxlen; i++)
		if (!get_byte(s))
			return i + 1;
	return maxlen;
}

#define GET_STR16(type, read) \
    int get_str16 ##type(ByteIOContext *pb, int maxlen, char *buf, int buflen)\
{\
    char* q = buf;\
    int ret = 0;\
    if (buflen <= 0) \
        return AVERROR(EINVAL); \
    while (ret + 1 < maxlen) {\
        uint8_t tmp;\
        uint32_t ch;\
        GET_UTF16(ch, (ret += 2) <= maxlen ? read(pb) : 0, break;)\
        if (!ch)\
            break;\
        PUT_UTF8(ch, tmp, if (q - buf < buflen - 1) *q++ = tmp;)\
    }\
    *q = 0;\
    return ret;\
}\

GET_STR16(le, get_le16)
GET_STR16(be, get_be16)

#undef GET_STR16

uint64_t get_be64(ByteIOContext*  s)
{
	uint64_t val;
	val = (uint64_t) get_be32(s) << 32;
	val |= (uint64_t) get_be32(s);
	return val;
}

uint64_t grt_varlen(ByteIOContext *bc)
{
    uint64_t val = 0;
    int tmp;

    do{
        tmp = get_byte(bc);
        val= (val<<7) + (tmp&127);
    }while(tmp&128);
    return val;
}

int url_fdopen(ByteIOContext**  s, URLContext * h)
{
    uint8_t *buffer;
    int buffer_size, max_packet_size;

    max_packet_size = h->max_packet_size;
    if (max_packet_size) {
        buffer_size = max_packet_size; /* no need to bufferize more than one packet */
    } else {
        buffer_size = IO_BUFFER_SIZE;
    }
    if (!(h->flags & AVIO_FLAG_WRITE) && h->is_streamed) {
        if (buffer_size > INT_MAX/2)
            return AVERROR(EINVAL);
        buffer_size *= 2;
    }
    buffer = av_malloc(buffer_size);
    if (!buffer)
        return AVERROR(ENOMEM);

    *s = avio_alloc_context(buffer, buffer_size, h->flags & AVIO_FLAG_WRITE, h,
                            (void*)url_read, (void*)url_write, (void*)url_seek);
    if (!*s) {
        av_free(buffer);
        return AVERROR(ENOMEM);
    }
    (*s)->direct = h->flags & AVIO_FLAG_DIRECT;
    (*s)->seekable = h->is_streamed ? 0 : 1;
    (*s)->max_packet_size = max_packet_size;
    if(h->prot) {
        (*s)->read_pause = (int (*)(void *, int))h->prot->url_read_pause;
        (*s)->read_seek  = (int64_t (*)(void *, int, int64_t, int))h->prot->url_read_seek;
    }
    return 0;
}

int url_setbufsize(ByteIOContext*  s, int buf_size)
{
    uint8_t *buffer;
    buffer = av_malloc(buf_size);
    if (!buffer)
        return AVERROR(ENOMEM);
    if (s->buffer)
        av_free(s->buffer);
    s->buffer = buffer;
    s->orig_buffer_size =
    s->buffer_size = buf_size;
    s->buf_ptr = buffer;
    url_resetbuf(s, s->write_flag ? URL_WRONLY : URL_RDONLY);
    return 0;
}

static int url_resetbuf(ByteIOContext *s, int flags)
{
	av_assert1(flags == AVIO_FLAG_WRITE || flags == AVIO_FLAG_READ);

	if (flags & AVIO_FLAG_WRITE) {
		s->buf_end = s->buffer + s->buffer_size;
		s->write_flag = 1;
	} else {
		s->buf_end = s->buffer;
		s->write_flag = 0;
	}
	return 0;
}

int url_rewind_with_probe_data(ByteIOContext *s, unsigned char *buf, int buf_size)
{
    int64_t buffer_start;
    int buffer_size;
    int overlap, new_size, alloc_size;

    if (s->write_flag)
        return AVERROR(EINVAL);

    buffer_size = s->buf_end - s->buffer;

    /* the buffers must touch or overlap */
    if ((buffer_start = s->pos - buffer_size) > buf_size)
        return AVERROR(EINVAL);

    overlap = buf_size - buffer_start;
    new_size = buf_size + buffer_size - overlap;

    alloc_size = FFMAX(s->buffer_size, new_size);
    if (alloc_size > buf_size)
        if (!(buf = av_realloc(buf, alloc_size)))
            return AVERROR(ENOMEM);

    if (new_size > buf_size) {
        memcpy(buf + buf_size, s->buffer + overlap, buffer_size - overlap);
        buf_size = new_size;
    }

    av_free(s->buffer);
    s->buf_ptr = s->buffer = buf;
    s->buffer_size = alloc_size;
    s->pos = buf_size;
    s->buf_end = s->buf_ptr + buf_size;
    s->eof_reached = 0;
    s->must_flush = 0;

    return 0;
}

int url_fopen(ByteIOContext**  s, const char *filename, int flags, AVDictionary **options)
{
    URLContext* h;
    int err = -1;

    err = url_open(&h, filename, flags, options);
    if (err < 0)
        return err;
    err = url_fdopen(s, h);
    if (s && *s) {
        av_dict_copy(&((*s)->options), h->url_context_options, 0);
    }

    if (err < 0) {
        url_close(h);
        return err;
    }
    return 0;
}

int url_fclose(ByteIOContext*  s)
{
    URLContext *h;

    if (!s)
        return 0;

    h = s->opaque;
    if (s->buffer)
        av_freep(&s->buffer);
    if (s->options) {
        av_dict_free(&s->options);
        s->options = NULL;
    }
    av_freep(&s);
    return url_close(h);
}

int url_gxlogd(ByteIOContext *s, const char *fmt, ...)
{
    va_list ap;
    char buf[4096];
    int ret;

    va_start(ap, fmt);
    ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    write_buffer(s, (const unsigned char*)buf, strlen(buf));
    return ret;
}

int url_pause(ByteIOContext *s, int pause)
{
    if (!s->read_pause)
        return AVERROR(ENOSYS);
    return s->read_pause(s->opaque, pause);
}

int64_t url_seek_time(ByteIOContext *s, int stream_index,
                       int64_t timestamp, int flags)
{
    URLContext *h = s->opaque;
    int64_t ret;
    if (!s->read_seek)
        return AVERROR(ENOSYS);
    ret = s->read_seek(h, stream_index, timestamp, flags);
    if(ret >= 0) {
        int64_t pos;
        s->buf_ptr = s->buf_end; // Flush buffer
        pos = s->seek(h, 0, SEEK_CUR);
        if (pos >= 0)
            s->pos = pos;
        else if (pos != AVERROR(ENOSYS))
            ret = pos;
    }
    return ret;
}


/* output in a dynamic buffer*/

typedef struct DynBuffer {
	int pos, size, allocated_size;
	uint8_t* buffer;
	int io_buffer_size;
	uint8_t io_buffer[1];
} DynBuffer;

static int dyn_buf_write(void* opaque, uint8_t * buf, int buf_size)
{
    DynBuffer *d = opaque;
    unsigned new_size, new_allocated_size;

    /* reallocate buffer if needed */
    new_size = d->pos + buf_size;
    new_allocated_size = d->allocated_size;
    if(new_size < d->pos || new_size > INT_MAX/2)
        return -1;
    while (new_size > new_allocated_size) {
        if (!new_allocated_size)
            new_allocated_size = new_size;
        else
            new_allocated_size += new_allocated_size / 2 + 1;
    }

    if (new_allocated_size > d->allocated_size) {
        int err;
        if ((err = av_reallocp(&d->buffer, new_allocated_size)) < 0) {
            d->allocated_size = 0;
            d->size = 0;
            return err;
        }
        d->allocated_size = new_allocated_size;
    }
    memcpy(d->buffer + d->pos, buf, buf_size);
    d->pos = new_size;
    if (d->pos > d->size)
        d->size = d->pos;
    return buf_size;
}

static int dyn_packet_buf_write(void* opaque, uint8_t * buf, int buf_size)
{
	unsigned char buf1[4];
	int ret;

	/* packetized write: output the header*/
	buf1[0] = (buf_size >> 24);
	buf1[1] = (buf_size >> 16);
	buf1[2] = (buf_size >> 8);
	buf1[3] = (buf_size);
	ret = dyn_buf_write(opaque, buf1, 4);
	if (ret < 0)
		return ret;

	/* then the data*/
	return dyn_buf_write(opaque, buf, buf_size);
}

static offset_t dyn_buf_seek(void* opaque, offset_t offset, int whence)
{
    DynBuffer *d = opaque;

    if (whence == SEEK_CUR)
        offset += d->pos;
    else if (whence == SEEK_END)
        offset += d->size;
    if (offset < 0 || offset > 0x7fffffffLL)
        return -1;
    d->pos = offset;
    return 0;
}

static int url_open_dyn_buf_internal(ByteIOContext**  s, int max_packet_size)
{
    DynBuffer *d;
    unsigned io_buffer_size = max_packet_size ? max_packet_size : 1024;

    if(sizeof(DynBuffer) + io_buffer_size < io_buffer_size)
        return -1;
    d = av_mallocz(sizeof(DynBuffer) + io_buffer_size);
    if (!d)
        return AVERROR(ENOMEM);
    d->io_buffer_size = io_buffer_size;
    *s = avio_alloc_context(d->io_buffer, d->io_buffer_size, 1, d, NULL,
                            max_packet_size ? dyn_packet_buf_write : dyn_buf_write,
                            max_packet_size ? NULL : dyn_buf_seek);
    if(!*s) {
        av_free(d);
        return AVERROR(ENOMEM);
    }
    (*s)->max_packet_size = max_packet_size;
    return 0;
}

int url_open_dyn_buf(ByteIOContext**  s)
{
	return url_open_dyn_buf_internal(s, 0);
}

int url_open_dyn_packet_buf(ByteIOContext**  s, int max_packet_size)
{
	if (max_packet_size <= 0)
		return -1;
	return url_open_dyn_buf_internal(s, max_packet_size);
}

int url_close_dyn_buf(ByteIOContext*  s, uint8_t ** pbuffer)
{
    DynBuffer *d = s->opaque;
    int size;
    static const unsigned char padbuf[FF_INPUT_BUFFER_PADDING_SIZE] = {0};
    int padding = 0;

    /* don't attempt to pad fixed-size packet buffers */
    if (!s->max_packet_size) {
        write_buffer(s, padbuf, sizeof(padbuf));
        padding = FF_INPUT_BUFFER_PADDING_SIZE;
    }

    put_flush(s);

    *pbuffer = d->buffer;
    size = d->size;
    av_free(d);
    av_free(s);
    return size - padding;
}


void ffio_fill(AVIOContext *s, int b, int count)
{
    while (count > 0) {
        int len = FFMIN(s->buf_end - s->buf_ptr, count);
        memset(s->buf_ptr, b, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        count -= len;
    }
}

void avio_write(AVIOContext *s, const unsigned char *buf, int size)
{
    if (s->direct && !s->update_checksum) {
        avio_flush(s);
        writeout(s, buf, size);
        return;
    }
    while (size > 0) {
        int len = FFMIN(s->buf_end - s->buf_ptr, size);
        memcpy(s->buf_ptr, buf, len);
        s->buf_ptr += len;

        if (s->buf_ptr >= s->buf_end)
            flush_buffer(s);

        buf += len;
        size -= len;
    }
}

void avio_flush(AVIOContext *s)
{
    flush_buffer(s);
    s->must_flush = 0;
}

void avio_context_free(AVIOContext **ps)
{
    av_freep(ps);
}

int avio_close(AVIOContext *s)
{
    AVIOInternal *internal;
    URLContext *h;

    if (!s)
        return 0;

    avio_flush(s);
    internal = s->opaque;
    h        = internal->h;

    av_freep(&s->opaque);
    av_freep(&s->buffer);
    av_opt_free(s);
    if (s)
        avio_context_free(&s);
    return ffurl_close(h);
}


static int io_read_packet(void *opaque, uint8_t *buf, int buf_size)
{
    AVIOInternal *internal = opaque;
    return url_read(internal->h, buf, buf_size);
}

static int io_write_packet(void *opaque, uint8_t *buf, int buf_size)
{
    AVIOInternal *internal = opaque;
    return url_write(internal->h, buf, buf_size);
}

static int64_t io_seek(void *opaque, int64_t offset, int whence)
{
    AVIOInternal *internal = opaque;
    return url_seek(internal->h, offset, whence);
}



static int io_read_pause(void *opaque, int pause)
{
    AVIOInternal *internal = opaque;
    if (!internal->h->prot->url_read_pause)
        return AVERROR(ENOSYS);
    return internal->h->prot->url_read_pause(internal->h, pause);
}

static int64_t io_read_seek(void *opaque, int stream_index, int64_t timestamp, int flags)
{
    AVIOInternal *internal = opaque;
    if (!internal->h->prot->url_read_seek)
        return AVERROR(ENOSYS);
    return internal->h->prot->url_read_seek(internal->h, stream_index, timestamp, flags);
}

int avio_closep(AVIOContext **s)
{
    int ret = avio_close(*s);
    *s = NULL;
    return ret;
}

int ffio_fdopen(AVIOContext **s, URLContext *h)
{
    AVIOInternal *internal = NULL;
    uint8_t *buffer = NULL;
    int buffer_size, max_packet_size;

    max_packet_size = h->max_packet_size;
    if (max_packet_size) {
        buffer_size = max_packet_size; /* no need to bufferize more than one packet */
    } else {
        buffer_size = IO_BUFFER_SIZE;
    }
    if (!(h->flags & AVIO_FLAG_WRITE) && h->is_streamed) {
        if (buffer_size > INT_MAX/2)
            return AVERROR(EINVAL);
        buffer_size *= 2;
    }
    buffer = av_malloc(buffer_size);
    if (!buffer) {
        gxlogd ("ffio fdopen, ### malloc fail fail. %d\n", buffer_size);
        return AVERROR(ENOMEM);
    }

    internal = av_mallocz(sizeof(*internal));
    if (!internal) {
        goto fail;
    }

    internal->h = h;

    *s = avio_alloc_context(buffer, buffer_size, h->flags & AVIO_FLAG_WRITE,
                            internal, io_read_packet, io_write_packet, io_seek);
    if (!*s) {
        gxlogd ("ffio fdopen, ### malloc fail fail.\n");
        goto fail;
    }

    (*s)->direct = h->flags & AVIO_FLAG_DIRECT;

    (*s)->seekable = h->is_streamed ? 0 : AVIO_SEEKABLE_NORMAL;
    (*s)->max_packet_size = max_packet_size;
    (*s)->min_packet_size = h->min_packet_size;
    if(h->prot) {
        (*s)->read_pause = io_read_pause;
        (*s)->read_seek  = io_read_seek;

        if (h->prot->url_read_seek)
            (*s)->seekable |= AVIO_SEEKABLE_TIME;
    }
    //(*s)->short_seek_get = io_short_seek;
    //(*s)->av_class = &ff_avio_class;
    return 0;
fail:
	if (internal)
    	av_freep(&internal);
	if (buffer)
    	av_freep(&buffer);
    return AVERROR(ENOMEM);
}

extern int ffurl_open_whitelist(URLContext **puc, const char *filename, int flags,
                         AVDictionary **options, const char *whitelist, const char* blacklist,
                         URLContext *parent);
int ffio_open_whitelist(AVIOContext **s, const char *filename, int flags,
                         AVDictionary **options, const char *whitelist, const char *blacklist
                        )
{
    URLContext *h;
    int err = -1;

    err = ffurl_open_whitelist(&h, filename, flags, options, whitelist, blacklist, NULL);
    if (err < 0)
        return err;
    err = ffio_fdopen(s, h);
    if (err < 0) {
        url_close(h);
        return err;
    }
    return 0;
}

int avio_open2(AVIOContext **s, const char *filename, int flags, AVDictionary **options)
{
    return ffio_open_whitelist(s, filename, flags, options, NULL, NULL);
}

int ff_get_line(AVIOContext *s, char *buf, int maxlen)
{
    int i = 0;
    char c;

    do {
        c = avio_r8(s);
        if (c && i < maxlen-1)
            buf[i++] = c;
    } while (c != '\n' && c != '\r' && c);
    if (c == '\r' && avio_r8(s) != '\n' && !avio_feof(s))
        avio_skip(s, -1);

    buf[i] = 0;
    return i;
}

int ff_get_chomp_line(AVIOContext *s, char *buf, int maxlen)
{
    int len = ff_get_line(s, buf, maxlen);
    while (len > 0 && av_isspace(buf[len - 1]))
        buf[--len] = '\0';
    return len;
}

URLContext* ffio_geturlcontext(AVIOContext *s)
{
    AVIOInternal *internal;
    if (!s)
        return NULL;

    internal = s->opaque;
    if (internal && s && s->read_packet == io_read_packet) {
        return internal->h;
    } else {
        return NULL;
    }
}

void ffio_free_dyn_buf(AVIOContext **s)
{
    uint8_t *tmp;
    if (!*s)
        return;
    avio_close_dyn_buf(*s, &tmp);
    av_free(tmp);
    *s = NULL;
}

int ffio_ensure_seekback(AVIOContext *s, int64_t buf_size)
{
    uint8_t *buffer;
    int max_buffer_size = s->max_packet_size ?
                          s->max_packet_size : IO_BUFFER_SIZE;
    int filled = s->buf_end - s->buffer;
    int checksum_ptr_offset = s->checksum_ptr ? s->checksum_ptr - s->buffer : -1;

    if (buf_size <= s->buf_end - s->buf_ptr) {
        return 0;
    }

    if (buf_size > INT_MAX - max_buffer_size) {
        return AVERROR(EINVAL);
    }
    buf_size += s->buf_ptr - s->buffer + max_buffer_size;

    if (buf_size < filled || s->seekable || !s->read_packet)
        return 0;
    av_assert0(!s->write_flag);

    buffer = av_malloc(buf_size);
    if (!buffer)
        return AVERROR(ENOMEM);

    memcpy(buffer, s->buffer, filled);
    av_free(s->buffer);
    s->buf_ptr = buffer + (s->buf_ptr - s->buffer);
    s->buf_end = buffer + (s->buf_end - s->buffer);
    s->buffer = buffer;
    s->buffer_size = buf_size;
    if (checksum_ptr_offset >= 0)
        s->checksum_ptr = s->buffer + checksum_ptr_offset;
    return 0;
}

void ffio_init_checksum(AVIOContext *s,
                   unsigned long (*update_checksum)(unsigned long c, const uint8_t *p, unsigned int len),
                   unsigned long checksum)
{
    s->update_checksum = update_checksum;
    if (s->update_checksum) {
        s->checksum     = checksum;
        s->checksum_ptr = s->buf_ptr;
    }
}

unsigned long ffio_get_checksum(AVIOContext *s)
{
    s->checksum = s->update_checksum(s->checksum, s->checksum_ptr,
                                     s->buf_ptr - s->checksum_ptr);
    s->update_checksum = NULL;
    return s->checksum;
}


unsigned long ff_crcA001_update(unsigned long checksum, const uint8_t *buf, unsigned int len)
{
    return av_crc(av_crc_get_table(AV_CRC_16_ANSI_LE), checksum, buf, len);
}

int64_t gx_read_file_line(AVIOContext *s, char *bp, int bp_len)
{
    int len = 0, end = 0;
    int64_t read = 0;
    char tmp[1024] = "";
    char c;

    memset (tmp, 0, sizeof(tmp));

    do {
        len = 0;
        do {
            c = avio_r8(s);
            end = (c == '\r' || c == '\n' || c == '\0');
            if (!end)
                tmp[len++] = c;
        } while (!end && len < sizeof(tmp));
        snprintf (bp, bp_len-1, "%s", tmp);
        read += len;
    } while (!end);

    if (c == '\r' && avio_r8(s) != '\n' && !avio_feof(s))
        avio_skip(s, -1);

    if (!c && s->error)
        return s->error;

    if (!c && !read && avio_feof(s))
        return AVERROR_EOF;

    return read;
}


int64_t gx_read_line_to_overwrite(AVIOContext *s, char *bp, int len)
{
    int64_t ret;

    if (bp)
        memset (bp, 0, len);

    ret = gx_read_file_line(s, bp, len);
    if (ret < 0)
        return ret;
    return ret;
}

int ffio_read_indirect(AVIOContext *s, unsigned char *buf, int size, unsigned char **data)
{
    if (s->buf_end - s->buf_ptr >= size && !s->write_flag) {
        *data = s->buf_ptr;
        s->buf_ptr += size;
        return size;
    } else {
        *data = buf;
        return get_buffer(s, buf, size);
    }
}

