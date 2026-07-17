/*
 * copyright (c) 2006 Michael Niedermayer <michaelni@gmx.at>
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

/**
 * @file common.h
 * common internal and external api header.
 */

#ifndef _AVUTIL_COMMON_H_
#define _AVUTIL_COMMON_H_

#include "gx_common.h"

#ifndef intptr_t
#define intptr_t unsigned long
#endif

#ifndef av_always_inline
#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define av_always_inline __attribute__((always_inline)) inline
#else
#define av_always_inline inline
#endif
#endif

#ifndef av_noinline
#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#define av_noinline __attribute__((noinline))
#else
#define av_noinline
#endif
#endif

#ifndef attribute_deprecated
#if defined(__GNUC__) && (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ > 0)
#    define attribute_deprecated __attribute__((deprecated))
#else
#    define attribute_deprecated
#endif
#endif

#ifndef av_unused
#if defined(__GNUC__)
#    define av_unused __attribute__((unused))
#else
#    define av_unused
#endif
#endif

#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif


#define MKTAG(a,b,c,d) (a | (b << 8) | (c << 16) | (d << 24))
#define MKBETAG(a,b,c,d) (d | (c << 8) | (b << 16) | (a << 24))
#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))

#define FF_MAX_EXTRADATA_SIZE ((1 << 28) - FF_INPUT_BUFFER_PADDING_SIZE)
#define FF_INPUT_BUFFER_PADDING_SIZE 8

//rounded divison & shift
#define RSHIFT(a,b) ((a) > 0 ? ((a) + ((1<<(b))>>1))>>(b) : ((a) + ((1<<(b))>>1)-1)>>(b))
/* assume b>0 */
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSIGN(a) ((a) > 0 ? 1 : -1)

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

#define rint(x)   (floor((x)+0.5f))

/* error handling */
#if EINVAL > 0
#define AVERROR(e) (-(e)) /**< Returns a negative error code from a POSIX error code, to return from library functions. */
#define AVUNERROR(e) (-(e)) /**< Returns a POSIX error code from a library function error return value. */
#else
/* Some platforms have E* and errno already negated. */
#define AVERROR(e) (e)
#define AVUNERROR(e) (e)
#endif
#define AVERROR_UNKNOWN     AVERROR(EINVAL)  /**< unknown error */
#define AVERROR_IO          AVERROR(EIO)     /**< I/O error */
#define AVERROR_NUMEXPECTED AVERROR(EDOM)    /**< Number syntax expected in filename. */
#define AVERROR_INVALIDDATA -MKTAG( 'I','N','D','A') ///< Invalid data found when processing input
#define AVERROR_NOMEM       AVERROR(ENOMEM)  /**< not enough memory */
#define AVERROR_NOFMT       AVERROR(EINVAL)  /**< unknown format */
#define AVERROR_NOTSUPP     AVERROR(ENOSYS)  /**< Operation not supported. */
#define AVERROR_NOENT       AVERROR(ENOENT)  /**< No such file or directory. */
#define AVERROR_PATCHWELCOME       -MKTAG('P','A','W','E')	/**< Not yet implemented in FFmpeg. Patches welcome. */
#define AVERROR_EOF                -MKTAG( 'E','O','F',' ') ///< End of file
#define AVERROR_OPTION_NOT_FOUND   -MKTAG(0xF8,'O','P','T') ///< Option not found
#define AVERROR_PROTOCOL_NOT_FOUND -MKTAG(0xF8,'P','R','O') ///< Protocol not found
#define AVERROR_STREAM_NOT_FOUND   -MKTAG(0xF8,'S','T','R') ///< Stream not found
#define AVERROR_EXIT               -MKTAG('E','X','I','T')
#define AVERROR_DEMUXER_NOT_FOUND  -MKTAG(0xF8,'D','E','M') ///< Demuxer not found
#define AVERROR_BUG                -MKTAG( 'B','U','G','!') ///< Internal bug, also see AVERROR_BUG2
#define AVERROR_RSTRT              -MKTAG( 'R','S','T','R') ///<restart.
#define AVERROR_NEXT              -MKTAG( 'N','E','X','T') ///<next program.

/* HTTP & RTSP errors */
#define AVERROR_HTTP_BAD_REQUEST   -MKTAG(0xF8,'4','0','0')
#define AVERROR_HTTP_UNAUTHORIZED  -MKTAG(0xF8,'4','0','1')
#define AVERROR_HTTP_FORBIDDEN     -MKTAG(0xF8,'4','0','3')
#define AVERROR_HTTP_NOT_FOUND     -MKTAG(0xF8,'4','0','4')
#define AVERROR_HTTP_OTHER_4XX     -MKTAG(0xF8,'4','X','X')
#define AVERROR_HTTP_SERVER_ERROR  -MKTAG(0xF8,'5','X','X')
#define AVERROR_HTTP_TOO_MANY_REQUESTS -MKTAG(0xF8,'4','2','9')

#define AV_MAX(a, b) (a>=b)?a:b
#define AV_MIN(a, b) (a<=b)?a:b
#define AV_POLLING_TIME 100 /// Time in milliseconds between interrupt check


/* misc math functions */
extern const uint8_t log2_tab[256];
extern const uint8_t av_reverse[256];

static inline int av_log2(unsigned int v)
{
	int n;

	n = 0;
	if (v & 0xffff0000) {
		v >>= 16;
		n += 16;
	}
	if (v & 0xff00) {
		v >>= 8;
		n += 8;
	}
	n += log2_tab[v];

	return n;
}

static inline int av_log2_16bit(unsigned int v)
{
	int n;

	n = 0;
	if (v & 0xff00) {
		v >>= 8;
		n += 8;
	}
	n += log2_tab[v];

	return n;
}

/* median of 3 */
static inline int mid_pred(int a, int b, int c)
{
	if (a > b) {
		if (c > b) {
			if (c > a)
				b = a;
			else
				b = c;
		}
	} else {
		if (b > c) {
			if (c > a)
				b = c;
			else
				b = a;
		}
	}
	return b;
}

/**
 * clip a signed integer value into the amin-amax range
 * @param a value to clip
 * @param amin minimum value of the clip range
 * @param amax maximum value of the clip range
 * @return clipped value
 */
static inline int av_clip(int a, int amin, int amax)
{
	if (a < amin)
		return amin;
	else if (a > amax)
		return amax;
	else
		return a;
}

/**
 * clip a signed integer value into the 0-255 range
 * @param a value to clip
 * @return clipped value
 */
static inline uint8_t av_clip_uint8(int a)
{
	if (a & (~255))
		return (-a) >> 31;
	else
		return a;
}

/**
 * clip a signed integer value into the -32768,32767 range
 * @param a value to clip
 * @return clipped value
 */
static inline int16_t av_clip_int16(int a)
{
	if ((a + 32768) & ~65535)
		return (a >> 31) ^ 32767;
	else
		return a;
}

/* math */
int64_t ff_gcd(int64_t a, int64_t b);
#define av_gcd ff_gcd

/**
 * converts fourcc string to int
 */
static inline int ff_get_fourcc(const char *s)
{
	assert(strlen(s) == 4);
	return (s[0]) + (s[1] << 8) + (s[2] << 16) + (s[3] << 24);
}

static inline const int av_popcount(uint32_t x)
{
	x -= (x >> 1) & 0x55555555;
	x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F;
	x += x >> 8;
	return (x + (x >> 16)) & 0x3F;
}

/*!
 * \def GET_UTF8(val, GET_BYTE, ERROR)
 * converts a UTF-8 character (up to 4 bytes long) to its 32-bit UCS-4 encoded form
 * \param val is the output and should be of type uint32_t. It holds the converted
 * UCS-4 character and should be a left value.
 * \param GET_BYTE gets UTF-8 encoded bytes from any proper source. It can be
 * a function or a statement whose return value or evaluated value is of type
 * uint8_t. It will be executed up to 4 times for values in the valid UTF-8 range,
 * and up to 7 times in the general case.
 * \param ERROR action that should be taken when an invalid UTF-8 byte is returned
 * from GET_BYTE. It should be a statement that jumps out of the macro,
 * like exit(), goto, return, break, or continue.
 */
#define GET_UTF8(val, GET_BYTE, ERROR)\
    val= GET_BYTE;\
    {\
        int ones= 7 - av_log2(val ^ 255);\
        if(ones==1)\
            ERROR;\
        val&= 127>>ones;\
        while(--ones > 0){\
            int tmp= GET_BYTE - 128;\
            if(tmp>>6)\
                ERROR;\
            val= (val<<6) + tmp;\
        }\
    }

/**
 * Convert a UTF-16 character (2 or 4 bytes) to its 32-bit UCS-4 encoded form.
 *
 * @param val       Output value, must be an lvalue of type uint32_t.
 * @param GET_16BIT Expression returning two bytes of UTF-16 data converted
 *                  to native byte order.  Evaluated one or two times.
 * @param ERROR     Expression to be evaluated on invalid input,
 *                  typically a goto statement.
 */
#define GET_UTF16(val, GET_16BIT, ERROR)\
    val = GET_16BIT;\
    {\
        unsigned int hi = val - 0xD800;\
        if (hi < 0x800) {\
            val = GET_16BIT - 0xDC00;\
            if (val > 0x3FFU || hi > 0x3FFU)\
                ERROR\
            val += (hi<<10) + 0x10000;\
        }\
    }\

/*!
 * \def PUT_UTF8(val, tmp, PUT_BYTE)
 * converts a 32-bit unicode character to its UTF-8 encoded form (up to 4 bytes long).
 * \param val is an input only argument and should be of type uint32_t. It holds
 * a ucs4 encoded unicode character that is to be converted to UTF-8. If
 * val is given as a function it's executed only once.
 * \param tmp is a temporary variable and should be of type uint8_t. It
 * represents an intermediate value during conversion that is to be
 * outputted by PUT_BYTE.
 * \param PUT_BYTE writes the converted UTF-8 bytes to any proper destination.
 * It could be a function or a statement, and uses tmp as the input byte.
 * For example, PUT_BYTE could be "*output++ = tmp;" PUT_BYTE will be
 * executed up to 4 times for values in the valid UTF-8 range and up to
 * 7 times in the general case, depending on the length of the converted
 * unicode character.
 */
#define PUT_UTF8(val, tmp, PUT_BYTE)\
    {\
        int bytes, shift;\
        uint32_t in = val;\
        if (in < 0x80) {\
            tmp = in;\
            PUT_BYTE\
        } else {\
            bytes = (av_log2(in) + 4) / 5;\
            shift = (bytes - 1) * 6;\
            tmp = (256 - (256 >> bytes)) | (in >> shift);\
            PUT_BYTE\
            while (shift >= 6) {\
                shift -= 6;\
                tmp = 0x80 | ((in >> shift) & 0x3f);\
                PUT_BYTE\
            }\
        }\
    }

/**
 * @def PUT_UTF16(val, tmp, PUT_16BIT)
 * Convert a 32-bit Unicode character to its UTF-16 encoded form (2 or 4 bytes).
 * @param val is an input-only argument and should be of type uint32_t. It holds
 * a UCS-4 encoded Unicode character that is to be converted to UTF-16. If
 * val is given as a function it is executed only once.
 * @param tmp is a temporary variable and should be of type uint16_t. It
 * represents an intermediate value during conversion that is to be
 * output by PUT_16BIT.
 * @param PUT_16BIT writes the converted UTF-16 data to any proper destination
 * in desired endianness. It could be a function or a statement, and uses tmp
 * as the input byte.  For example, PUT_BYTE could be "*output++ = tmp;"
 * PUT_BYTE will be executed 1 or 2 times depending on input character.
 */
#define PUT_UTF16(val, tmp, PUT_16BIT)\
    {\
        uint32_t in = val;\
        if (in < 0x10000) {\
            tmp = in;\
            PUT_16BIT\
        } else {\
            tmp = 0xD800 | ((in - 0x10000) >> 10);\
            PUT_16BIT\
            tmp = 0xDC00 | ((in - 0x10000) & 0x3FF);\
            PUT_16BIT\
        }\
    }\

#ifndef INT_MAX
#define INT_MAX (0xFFFFFFFF)
#endif

#endif				/* COMMON_H */
