/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

/* This file is part of the boot loader */

/*
 * util.c
 *
 * utility functions
 *
 * first revision by Ho Lee 10/10/2002
 */
#include "stdio.h"
#include "string.h"

#include "config.h"
#include "util.h"

#define DIVIDE_FACTOR       2

void unsigned_divide(unsigned int dividened, unsigned int divisor, unsigned int *pquotient, unsigned int *premainder)
{
	unsigned int quotient = 0, remainder = 0;
	int bits, mdivisor;

	// divide by zero
	if (divisor == 0)
		return;

	for (bits = 1, mdivisor = divisor; dividened >= (mdivisor << DIVIDE_FACTOR);
			bits <<= DIVIDE_FACTOR, mdivisor <<= DIVIDE_FACTOR) ;

	do {
		while (dividened >= mdivisor) {
			quotient += bits;
			dividened -= mdivisor;
		}
		bits >>= DIVIDE_FACTOR;
		mdivisor >>= DIVIDE_FACTOR;
	} while (bits > 0);

	remainder = dividened;

	if (pquotient)
		*pquotient = quotient;
	if (premainder)
		*premainder = remainder;
}

void signed_divide(int dividened, int divisor, int *pquotient, int *premainder)
{
	int quotient = 0, remainder = 0;
	unsigned int abs_dividened = ABS(dividened);
	unsigned int abs_divisor = ABS(divisor);
	unsigned int quot, rem;

	unsigned_divide(abs_dividened, abs_divisor, &quot, &rem);

	quotient = quot;
	if (dividened < 0) {
		remainder = -rem;
		if (divisor > 0)
			quotient = -quot;
	} else {
		remainder = rem;
		if (divisor < 0)
			quotient = -quot;
	}

	if (pquotient)
		*pquotient = quotient;
	if (premainder)
		*premainder = remainder;
}

int idivide(int dividened, int divisor)
{
	int quotient;

	signed_divide(dividened, divisor, &quotient, NULL);

	return quotient;
}

int imodulus(int dividened, int divisor)
{
	int remainder;

	signed_divide(dividened, divisor, NULL, &remainder);

	return remainder;
}

//
// Miscellaneous
//

// decode ip address or MAC address string to series of number
// retrun :
//   0 : valid address
//   non-zero : invalid address
int parse_netaddr(char *str, unsigned char *addr, int len)
{
	int i, j, value;
	char *cp;

	trim(str);
	cp = strtok(str, ".:");

	for (i = 0; i < len && cp != NULL; ++i) {
		for (j = 0; cp[j] != 0; ++j)
			if (!isdigit(cp[j]))
				return 1;
		value = atoi(cp);
		if (value < 0 || value >= 0x100)
			return 1;
		addr[i] = (unsigned char)value;
		cp = strtok(NULL, ".");
	}

	return (i == len && cp == NULL) ? 0 : 1;
}

int parse_ipaddr(char *str, unsigned int *ipaddr)
{
	unsigned int addr;
	unsigned char *cp = (unsigned char *)&addr;

	if (parse_netaddr(str, cp, 4) == 0) {
		*ipaddr = ntohl(addr);
		return 0;
	}

	return 1;
}

char *ipaddr_to_str(unsigned int ipaddr)
{
	static char s_buf[16];

	unsigned int addr = htonl(ipaddr);
	unsigned char *cp = (unsigned char *)&addr;

	sprintf(s_buf, "%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);

	return s_buf;
}

static inline unsigned long get_dword_from_buf(unsigned char *buf)
{
	unsigned long val = 0;
	val = buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	return val;
}

// Given start address and size, check if checksum is OK.
unsigned binfile_crc_check(void *startptr, unsigned int size)
{
	unsigned long sum = 0;
	unsigned int i;
	unsigned char *ptr;

	printf("Verify checksum from 0x%08lx, size %d (0x%x).\n", (unsigned long)startptr, size, size);

	ptr = (unsigned char *)startptr;
	for (i = 0; i < (size >> 2); i++, ptr += 4)
		sum += get_dword_from_buf(ptr);

	if (size & 0x3) {
		union {
			unsigned char c[4];
			unsigned long l;
		} buf;

		buf.l = 0;
		for (i = 0; i < (size & 0x3); i++)
			buf.c[i] = *ptr++;
		sum += get_dword_from_buf(&buf.c[0]);
	}
	return (sum);
}

void data_reverse(unsigned char *val, unsigned int size)
{
	int i = 0;
	unsigned char tmp_val;
	for(i = 0; i < size / 2; i++) {
		tmp_val = val[i];
		val[i] = val[size - i - 1];
		val[size - i - 1] = tmp_val;
	}
}

#ifdef CONFIG_ENABLE_LOGO
static unsigned char bmp_get_4bits(unsigned char **pin, int *toggle)
{
	unsigned char val;

	val = ((**pin) >> (*toggle * 4)) & 0xf;
	*toggle = 1 - *toggle;
	if (*toggle == 1)
		*pin = *pin + 1;

	return val;
}

static int bmp_inflate_4bpp(unsigned char *in_buf, unsigned char *out_buf, unsigned int width, unsigned int len)
{
	unsigned char color;
	unsigned char *pin, *pout;
	int count, color_count, line_count, i, toggle = 1;
	unsigned long x;

	pout = out_buf;
	pin = in_buf;
	x = 0;
	line_count = 0;

	while (pin < in_buf + len) {
		color = bmp_get_4bits(&pin, &toggle);
		count = bmp_get_4bits(&pin, &toggle);

		if (count == 0)
			color_count = width - x;
		else {
			color_count = 0;
			i = 0;
			while (1) {
				color_count += (count & 0x7) << i;

				if ((count & 0x8) == 0)
					break;

				count = bmp_get_4bits(&pin, &toggle);
				i += 3;
			}
		}

		while (color_count > 0) {
			if (x & 0x1) {
				*pout |= color & 0xf;
				pout++;
			} else {
				*pout = color << 4;
			}
			color_count--;
			x++;
		}

		if (x == width) {
			if (x & 0x1)
				pout++;

			if (toggle == 0) {
				toggle = 1;
				pin++;
			}

			x = 0;
			line_count++;
		}
	}

	return (line_count * ((width + 1) / 2));
}

static int bmp_inflate_7bpp(unsigned char *in_buf, unsigned char *out_buf, unsigned int width, unsigned int len)
{
	unsigned char color, b;
	unsigned char *pin = in_buf, *pout = out_buf;
	int count, line_count = 0, i, color_count, toggle = 1;
	unsigned long x = 0;

	while (pin < in_buf + len) {
		b = bmp_get_4bits(&pin, &toggle);
		b = (b << 4) | bmp_get_4bits(&pin, &toggle);

		if (b & 0x80) {
			color = b & 0x7f;

			count = bmp_get_4bits(&pin, &toggle);
			if (count == 0)
				color_count = width - x;
			else {
				color_count = 1;
				i = 0;
				while (1) {
					color_count += (count & 0x7) << i;

					if ((count & 0x8) == 0)
						break;

					count = bmp_get_4bits(&pin, &toggle);
					i += 3;
				}
			}
		} else {
			color = b;
			color_count = 1;
		}

		while (color_count > 0) {
			(*pout) = color;
			pout++;
			x++;
			color_count--;
		}

		if (x == width) {
			if (toggle == 0) {
				pin++;
				toggle = 1;
			}
			x = 0;
			line_count++;
		}
	}

	return (line_count * width);
}

#define RLED_DEPTH_4BPP 0
#define RLED_DEPTH_7BPP 1
int bmp_inflate(unsigned char *in_buf, unsigned char *out_buf, unsigned int len)
{
	//get width
	int *pwidth, width, type;
	pwidth = (int *)in_buf;
	width = 0x0fff & (*pwidth);
	type = ((*pwidth) >> 24) & 0xf;
	printf("RLED section has width %d\n", width);
	printf("RLED section is of type %d\n", type);

	switch ((type)) {
		case RLED_DEPTH_4BPP:
			in_buf += sizeof(int);
			return bmp_inflate_4bpp(in_buf, out_buf, width, len);
			break;
		case RLED_DEPTH_7BPP:
			in_buf += sizeof(int);
			return bmp_inflate_7bpp(in_buf, out_buf, width, len);
			break;
		default:
			break;
	}

	return 0;
}

#endif
