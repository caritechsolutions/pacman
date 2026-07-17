/*
*  native ebml reader for the Matroska demuxer
*/
#include <math.h>

#include "ebml.h"
#include "../avutil/bswap.h"
#include "../avutil/intfloat_readwrite.h"

#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif

uint32_t ebml_read_id(GxStream*  s, int *length)
{
	int i, len_mask = 0x80;
	uint32_t id;

	for (i = 0, id = GxStream_ReadChar(s); i < 4 && !(id & len_mask); i++)
		len_mask >>= 1;
	if (i >= 4)
		return EBML_ID_INVALID;
	if (length)
		*length = i + 1;
	while (i--)
		id = (id << 8) | GxStream_ReadChar(s);
	return id;
}

/*
*  Read a variable length unsigned int.
*/
uint64_t ebml_read_vlen_uint(uint8_t*  buffer, int *length)
{
	int i, j, num_ffs = 0, len_mask = 0x80;
	uint64_t num;

	for (i = 0, num =* buffer++; i < 8 && !(num & len_mask); i++)
		len_mask >>= 1;
	if (i >= 8)
		return EBML_UINT_INVALID;
	j = i + 1;
	if (length)
		*length = j;
	if ((int)(num &= (len_mask - 1)) == len_mask - 1)
		num_ffs++;
	while (i--) {
		num = (num << 8) |* buffer++;
		if ((num & 0xFF) == 0xFF)
			num_ffs++;
	}
	if (j == num_ffs)
		return EBML_UINT_INVALID;
	return num;
}

/*
*  Read a variable length signed int.
*/
int64_t ebml_read_vlen_int(uint8_t*  buffer, int *length)
{
	uint64_t unum;
	int l;

	/* read as unsigned number first*/
	unum = ebml_read_vlen_uint(buffer, &l);
	if (unum == EBML_UINT_INVALID)
		return EBML_INT_INVALID;
	if (length)
		*length = l;

	return unum - ((1 << ((7*  l) - 1)) - 1);
}

/*
*  Read: element content length.
*/
uint64_t ebml_read_length(GxStream*  s, int *length)
{
	int i, j, num_ffs = 0, len_mask = 0x80;
	uint64_t len = 0;

	for (i = 0, len = GxStream_ReadChar(s); i < 8 && !(len & len_mask); i++)
		len_mask >>= 1;
	if (i >= 8)
		return EBML_UINT_INVALID;
	j = i + 1;
	if (length)
		*length = j;
	if ((int)(len &= (len_mask - 1)) == len_mask - 1)
		num_ffs++;
	while (i--) {
		len = (len << 8) | GxStream_ReadChar(s);
		if ((len & 0xFF) == 0xFF)
			num_ffs++;
	}
	if (j == num_ffs)
		return EBML_UINT_INVALID;
	return len;
}

/*
*  Read the next element as an unsigned int.
*/
uint64_t ebml_read_uint(GxStream*  s, uint64_t * length)
{
	uint64_t len, value = 0;
	int l;

	len = ebml_read_length(s, &l);
	if (len == EBML_UINT_INVALID || len < 1 || len > 8)
		return EBML_UINT_INVALID;
	if (length)
		*length = len + l;

	while (len--)
		value = (value << 8) | GxStream_ReadChar(s);

	return value;
}

/*
*  Read the next element as a signed int.
*/
int64_t ebml_read_int(GxStream*  s, uint64_t * length)
{
	int64_t value = 0;
	uint64_t len;
	int l;

	len = ebml_read_length(s, &l);
	if (len == EBML_UINT_INVALID || len < 1 || len > 8)
		return EBML_INT_INVALID;
	if (length)
		*length = len + l;

	len--;
	l = GxStream_ReadChar(s);
	if (l & 0x80)
		value = -1;
	value = (value << 8) | l;
	while (len--)
		value = (value << 8) | GxStream_ReadChar(s);

	return value;
}

/*
*  Read the next element as a float.
*/
long double ebml_read_float(GxStream*  s, uint64_t * length)
{
	long double value;
	uint64_t len;
	int l;

	len = ebml_read_length(s, &l);
	switch (len) {
	case 4:
		value = av_int2flt(GxStream_ReadDword(s));
		break;

	case 8:
		value = av_int2dbl(GxStream_ReadQword(s));
		break;

	default:
		return EBML_FLOAT_INVALID;
	}

	if (length)
		*length = len + l;

	return value;
}

/*
*  Read the next element as an ASCII string.
*/
char* ebml_read_ascii(GxStream * s, uint64_t * length)
{
	uint64_t len;
	uint8_t* str;
	int l;

	len = ebml_read_length(s, &l);
	if (len == EBML_UINT_INVALID)
		return NULL;
	if (len > SIZE_MAX - 1)
		return NULL;
	if (length)
		*length = len + l;

	str = (uint8_t* ) av_malloc(len + 1);
	if(str)
	{
		memset(str,'\0',len + 1);
		if (GxStream_Read(s, str, len) != (int)len) {
			av_free(str);
			return NULL;
		}
		str[len] = '\0';
	}

	return (char* )str;
}

/*
*  Read the next element as a UTF-8 string.
*/
char* ebml_read_utf8(GxStream * s, uint64_t * length)
{
	return ebml_read_ascii(s, length);
}

/*
*  Skip the next element.
*/
int ebml_read_skip(GxStream*  s, uint64_t * length)
{
	uint64_t len;
	int l;

	len = ebml_read_length(s, &l);
	if (len == EBML_UINT_INVALID)
		return 1;
	if (length)
		*length = len + l;

	GxStream_Skip(s, len);

	return 0;
}

/*
*  Read the next element, but only the header. The contents
*  are supposed to be sub-elements which can be read separately.
*/
uint32_t ebml_read_master(GxStream*  s, uint64_t * length)
{
	uint64_t len;
	uint32_t id;

	id = ebml_read_id(s, NULL);
	if (id == EBML_ID_INVALID)
		return id;

	len = ebml_read_length(s, NULL);
	if (len == EBML_UINT_INVALID)
		return EBML_ID_INVALID;
	if (length)
		*length = len;

	return id;
}

/*
*  Read an EBML header.
*/
char* ebml_read_header(GxStream * s, int *version)
{
	uint64_t length, l, num;
	uint32_t id;
	char* str = NULL;

	if (ebml_read_master(s, &length) != EBML_ID_HEADER)
		return 0;

	if (version)
		*version = 1;

	while (length > 0) {
		id = ebml_read_id(s, NULL);
		if (id == EBML_ID_INVALID)
			return NULL;
		length -= 2;

		switch (id) {
			/* is our read version uptodate?*/
		case EBML_ID_EBMLREADVERSION:
			num = ebml_read_uint(s, &l);
			if (num != EBML_VERSION)
				return NULL;
			break;

			/* we only handle 8 byte lengths at max*/
		case EBML_ID_EBMLMAXSIZELENGTH:
			num = ebml_read_uint(s, &l);
			if (num != sizeof(uint64_t))
				return NULL;
			break;

			/* we handle 4 byte IDs at max*/
		case EBML_ID_EBMLMAXIDLENGTH:
			num = ebml_read_uint(s, &l);
			if (num != sizeof(uint32_t))
				return NULL;
			break;

		case EBML_ID_DOCTYPE:
			str = ebml_read_ascii(s, &l);
			if (str == NULL)
				return NULL;
			break;

		case EBML_ID_DOCTYPEREADVERSION:
			num = ebml_read_uint(s, &l);
			if (num == EBML_UINT_INVALID)
				return NULL;
			if (version)
				*version = num;
			break;

			/* we ignore these two, they don't tell us anything we care about*/
		case EBML_ID_VOID:
		case EBML_ID_EBMLVERSION:
		case EBML_ID_DOCTYPEVERSION:
		default:
			if (ebml_read_skip(s, &l))
				return NULL;
			break;
		}
		length -= l;
	}

	return str;
}
