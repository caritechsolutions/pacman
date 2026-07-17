/*****************************************
  Copyright (c) 2003-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the boot loader */

/*
 * vsprintf.c
 *
 * Simple implemention of vsprintf()
 *
 * Written by Ho Lee 11/17/2003
 *
 * Supported directives :
 *   conversion specifier :
 *     %c : character
 *     %d : decimal
 *     %u : unsigned decimal
 *     %x, %X : hexadecimal
 *     %p : pointer (%08x)
 *     %s : string (pointer to array of characters)
 *     %% : '%'
 *   flag characters :
 *     0 : zero padded
 *   field width :
 *     number : width
 *     * : width given in the next argument
 *   precision : not supported
 *   length modifier :
 *     l : long integer or unsigned long integer (no difference)
 */
#include "stdio.h"
#include "string.h"
#include "util.h"
#include <div64.h>

char *itoa(char *buf, long long data, int issigned, int printsign, int fmtwidth, int leftalign, int dividor, int zeropad,
		int upper)
{
	unsigned long long udata, udata_tmp;
	int width, remainder, minus = 0, plus = 0;
	char *cp = buf;

	// option check
	if (fmtwidth == 0)
		leftalign = 0;

	// printsign
	if (printsign && ((issigned && data > 0) || (!issigned && data != 0))) {
		if (!zeropad)
			plus = 1;
		else
			*cp++ = '+';
	}

	// signed integer
	if (issigned && data < 0) {
		if (!zeropad)
			minus = 1;
		else
			*cp++ = '-';
		udata = (unsigned long long)-data;
	} else
		udata = (unsigned long long)data;

	// calculate actual width
	for (udata_tmp = udata, width = 1; udata_tmp >= dividor; do_div(udata_tmp, (dividor == 16 ? 16 : 10)), ++width) ;

	// sign and left padding
	if (!leftalign)
		while (fmtwidth-- > width)
			*cp++ = zeropad ? '0' : ' ';
	if (minus)
		*cp++ = '-';
	if (plus)
		*cp++ = '+';

	cp += width;

	// number
	do {
		remainder = do_div(udata, (dividor == 16 ? 16 : 10));
		*--cp = (remainder < 10) ? (remainder + '0') : (remainder - 10 + (upper ? 'A' : 'a'));
	} while (udata != 0);

	cp += width;

	// right padding
	if (leftalign)
		while (fmtwidth-- > width)
			*cp++ = ' ';

	*cp = 0;

	return buf;
}

enum { STAT_NORMAL, STAT_FLAGWIDTH, STAT_CONVERSION };
enum { CONV_CHAR, CONV_INT, CONV_UINT, CONV_HEX_LOWER, CONV_HEX_UPPER,
	CONV_PTR, CONV_STRING
};

#define ALT             0x001           /* alternate form */
#define HEXPREFIX       0x002           /* add 0x or 0X prefix */
#define LADJUST         0x004           /* left adjustment */
#define LONGDBL         0x008           /* long double; unimplemented */
#define LONGINT         0x010           /* long integer */
#define QUADINT         0x020           /* quad integer */
#define SHORTINT        0x040           /* short integer */
#define ZEROPAD         0x080           /* zero (as opposed to blank) pad */
#define FPT             0x100           /* Floating point number */
#define SIZET           0x200           /* size_t */

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int stat = STAT_NORMAL;
	int zeropad = 0, leftalign = 0, printsign = 0, width = 0;
	long long quad_data;
	unsigned int flags = 0;
	char *saved_buf = buf, *cp;
	int precision;		/* min. # of digits for integers; max  number of chars for from string */

	for (; *fmt != 0; ++fmt) {
		switch (stat) {
			case STAT_NORMAL:
				if (*fmt == '%')
					stat = STAT_FLAGWIDTH;
				else
					*buf++ = *fmt;
				break;

			case STAT_FLAGWIDTH:
				// sign
				if (*fmt == '+') {
					++fmt;
					printsign = 1;
				} else
					printsign = 0;

				// left align
				if (*fmt == '-') {
					++fmt;
					leftalign = 1;
				} else
					leftalign = 0;

				// zero padding
				if (*fmt == '0') {
					zeropad = 1;
					++fmt;
				} else
					zeropad = 0;

				// width
				if (*fmt == '*') {
					width = va_arg(args, int);
				} else if (isdigit(*fmt)) {
					for (width = 0; isdigit(*fmt); ++fmt)
						width = width * 10 + *fmt - '0';
					--fmt;
				} else {
					width = 0;
					--fmt;
				}

				/* get the precision */
				precision = -1;
				if (*(++fmt) == '.') {
					++fmt;
					if (*fmt == '*') {
						precision = va_arg(args, int);
					} else if (isdigit(*fmt)) {
						for (precision = 0; isdigit(*fmt); ++fmt)
							precision = width * 10 + *fmt - '0';
						--fmt;
					}

					if (precision < 0)
						precision = 0;
				} else
					--fmt;
				stat = STAT_CONVERSION;
				break;

			case STAT_CONVERSION:
				// length modifier
				if (*fmt == 'l') {
					++fmt;
					if (*fmt == 'l') {
						fmt++;
						flags |= LONGINT;
					}
				}

				// conversion directive
				switch (*fmt) {
					case 'c':
						quad_data = va_arg(args, char);
						*buf++ = quad_data;
						break;
					case 'p':
						zeropad = 1;
						width = sizeof(void *) * 2;
					case 'd':
					case 'u':
					case 'x':
					case 'X':
						if (flags & LONGINT) {
							if (*fmt == 'x' || *fmt == 'X')
								quad_data = va_arg(args, unsigned long long);
							else
								quad_data = va_arg(args, long long);
							flags &= ~LONGINT;
						}
						else {
							if (*fmt == 'x' || *fmt == 'X')
								quad_data = va_arg(args, unsigned int);
							else
								quad_data = va_arg(args, int);
						}
						// integer
						itoa(buf, quad_data,
								*fmt == 'd' ? 1 : 0, printsign, width, leftalign,
								(*fmt == 'x' || *fmt == 'X' || *fmt == 'p') ? 16 : 10,
								zeropad, *fmt == 'x' ? 0 : 1);
						while (*buf)
							++buf;
						break;
					case 's':
						cp = va_arg(args, char *);
						// right align
						if (!leftalign && width > 0) {
							width -= strlen(cp);
							while (width-- > 0)
								*buf++ = ' ';
						}
						// string
						if (precision >= 0) {
							while (*cp && precision--) {
								*buf++ = *cp++;
								if (width > 0)
									--width;
							}
						} else {
							while (*cp) {
								*buf++ = *cp++;
								if (width > 0)
									--width;
							}
						}
						// left align
						if (leftalign && width > 0) {
							while (width-- > 0)
								*buf++ = ' ';
						}
						break;
					case '%':
						*buf++ = '%';
						break;
					default:
						// unknown conversion, ignore it!
						break;
				}
				stat = STAT_NORMAL;
				break;
		}
	}

	*buf = 0;

	return buf - saved_buf;
}

int sprintf(char *buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsprintf(buf, fmt, args);
	va_end(args);

	return i;
}
