#include "stdio.h"
#include "sys/types.h"
//#include "asm/string.h"
#include "string.h"
#include "config.h"

#ifdef CONFIG_ARCH_CKMMU
/*#define __HAVE_ARCH_STRRCHR
#define __HAVE_ARCH_STRCHR
//#define __HAVE_ARCH_MEMCPY
//#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMCHR
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMCMP
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRNCMP*/
#elif defined(CONFIG_ARCH_ARM)
/*#define __HAVE_ARCH_STRRCHR
#define __HAVE_ARCH_STRCHR
#define __HAVE_ARCH_MEMCPY
#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMCHR
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMCMP
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRNCMP*/
#else
#define __HAVE_ARCH_STRRCHR
#define __HAVE_ARCH_STRCHR
//#define __HAVE_ARCH_MEMCPY
//#define __HAVE_ARCH_MEMMOVE
#define __HAVE_ARCH_MEMCHR
#define __HAVE_ARCH_MEMSET
#define __HAVE_ARCH_MEMCMP
#define __HAVE_ARCH_STRLEN
#define __HAVE_ARCH_STRCMP
#define __HAVE_ARCH_STRNCMP
#endif

/* How many bytes are copied each iteration of the 4X unrolled loop.  */
#define BIGBLOCKSIZE    (sizeof (long) << 2)

/* How many bytes are copied each iteration of the word copy loop.  */
#define LITTLEBLOCKSIZE (sizeof (long))

#define UNALIGNED1(X)   ((long)X & (LITTLEBLOCKSIZE - 1))
#define TOO_SMALL(LEN) ((LEN) < LITTLEBLOCKSIZE)
#define DETECTNULL(X) (((X) - 0x01010101) & ~(X) & 0x80808080)

/* Nonzero if either X or Y is not aligned on a "long" boundary.  */
#define UNALIGNED2(X, Y) \
	(((long)X & (sizeof (long) - 1)) | ((long)Y & (sizeof (long) - 1)))

/* Threshhold for punting to the byte copier.  */

int strnicmp(const char *s1, const char *s2, size_t len)
{
	/* Yes, Virginia, it had better be unsigned */
	unsigned char c1, c2;

	if (!len)
		return 0;

	do {
		c1 = *s1++;
		c2 = *s2++;
		if (!c1 || !c2)
			break;
		if (c1 == c2)
			continue;
		c1 = tolower(c1);
		c2 = tolower(c2);
		if (c1 != c2)
			break;
	} while (--len);
	return (int)c1 - (int)c2;
}

int strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while (c1 == c2 && c1 != 0);
	return c1 - c2;
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
	int c1, c2;

	do {
		c1 = tolower(*s1++);
		c2 = tolower(*s2++);
	} while ((--n > 0) && c1 == c2 && c1 != 0);
	return c1 - c2;
}

char *strcpy(char *dest, const char *src)
{
	char *tmp = dest;

	while ((*dest++ = *src++) != '\0')
		/* nothing */;
	return tmp;
}

char *strncpy(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	while (count) {
		if ((*tmp = *src) != 0)
			src++;
		tmp++;
		count--;
	}
	return dest;
}

size_t strlcpy(char *dest, const char *src, size_t size)
{
	size_t ret = strlen(src);

	if (size) {
		size_t len = (ret >= size) ? size - 1 : ret;
		memcpy(dest, src, len);
		dest[len] = '\0';
	}
	return ret;
}

char *strcat(char *dest, const char *src)
{
	char *tmp = dest;

	while (*dest)
		dest++;
	while ((*dest++ = *src++) != '\0')
		;
	return tmp;
}

char *strncat(char *dest, const char *src, size_t count)
{
	char *tmp = dest;

	if (count) {
		while (*dest)
			dest++;
		while ((*dest++ = *src++) != 0) {
			if (--count == 0) {
				*dest = '\0';
				break;
			}
		}
	}
	return tmp;
}

size_t strlcat(char *dest, const char *src, size_t count)
{
	size_t dsize = strlen(dest);
	size_t len = strlen(src);
	size_t res = dsize + len;

	/* This would be a bug */
	BUG_ON(dsize >= count);

	dest += dsize;
	count -= dsize;
	if (len >= count)
		len = count-1;
	memcpy(dest, src, len);
	dest[len] = 0;
	return res;
}

#ifndef __HAVE_ARCH_STRDUP
char * strdup(const char *s)
{
	char *new;

	if ((s == NULL) ||
			((new = malloc (strlen(s) + 1)) == NULL) ) {
		return NULL;
	}

	strcpy (new, s);
	return new;
}
#endif

#ifndef __HAVE_ARCH_STRCMP
int strcmp(const char *cs, const char *ct)
{
	unsigned char c1, c2;

	while (1) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
	}
	return 0;
}
#endif

#ifndef __HAVE_ARCH_STRNCMP
int strncmp(const char *cs, const char *ct, size_t count)
{
	unsigned char c1, c2;

	while (count) {
		c1 = *cs++;
		c2 = *ct++;
		if (c1 != c2)
			return c1 < c2 ? -1 : 1;
		if (!c1)
			break;
		count--;
	}
	return 0;
}
#endif

char *strchr(const char *s, int c)
{
	for (; *s != (char)c; ++s)
		if (*s == '\0')
			return NULL;
	return (char *)s;
}

char *strrchr(const char *s, int c)
{
	const char *p = s + strlen(s);
	do {
		if (*p == (char)c)
			return (char *)p;
	} while (--p >= s);
	return NULL;
}

char *strnchr(const char *s, size_t count, int c)
{
	for (; count-- && *s != '\0'; ++s)
		if (*s == (char)c)
			return (char *)s;
	return NULL;
}

char *skip_spaces(const char *str)
{
	while (isspace(*str))
		++str;
	return (char *)str;
}

char *strim(char *s)
{
	size_t size;
	char *end;

	s = skip_spaces(s);
	size = strlen(s);
	if (!size)
		return s;

	end = s + size - 1;
	while (end >= s && isspace(*end))
		end--;
	*(end + 1) = '\0';

	return s;
}

#ifndef __HAVE_ARCH_STRLEN
size_t strlen(const char *str)
{
	const char *start = str;
	unsigned long *aligned_addr;

	if (!UNALIGNED1 (str))
	{
		/* If the string is word-aligned, we can check for the presence of
		   a null in each word-sized block.  */
		aligned_addr = (unsigned long*)str;
		while (!DETECTNULL (*aligned_addr))
			aligned_addr++;

		/* Once a null is detected, we check each byte in that block for a
		   precise position of the null.  */
		str = (char*)aligned_addr;
	}

	while (*str)
		str++;
	return str - start;
}
#endif


size_t strnlen(const char *s, size_t count)
{
	const char *sc;

	for (sc = s; count-- && *sc != '\0'; ++sc)
		/* nothing */;
	return sc - s;
}

size_t strspn(const char *s, const char *accept)
{
	const char *p;
	const char *a;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (a = accept; *a != '\0'; ++a) {
			if (*p == *a)
				break;
		}
		if (*a == '\0')
			return count;
		++count;
	}
	return count;
}

size_t strcspn(const char *s, const char *reject)
{
	const char *p;
	const char *r;
	size_t count = 0;

	for (p = s; *p != '\0'; ++p) {
		for (r = reject; *r != '\0'; ++r) {
			if (*p == *r)
				return count;
		}
		++count;
	}
	return count;
}

char *strpbrk(const char *cs, const char *ct)
{
	const char *sc1, *sc2;

	for (sc1 = cs; *sc1 != '\0'; ++sc1) {
		for (sc2 = ct; *sc2 != '\0'; ++sc2) {
			if (*sc1 == *sc2)
				return (char *)sc1;
		}
	}
	return NULL;
}

char *strsep(char **s, const char *ct)
{
	char *sbegin = *s;
	char *end;

	if (sbegin == NULL)
		return NULL;

	end = strpbrk(sbegin, ct);
	if (end)
		*end++ = '\0';
	*s = end;
	return sbegin;
}

int sysfs_streq(const char *s1, const char *s2)
{
	while (*s1 && *s1 == *s2) {
		s1++;
		s2++;
	}

	if (*s1 == *s2)
		return 1;
	if (!*s1 && *s2 == '\n' && !s2[1])
		return 1;
	if (*s1 == '\n' && !s1[1] && !*s2)
		return 1;
	return 0;
}

#ifndef __HAVE_ARCH_MEMSET
void *memset(void *m, int c, size_t n)
{
	char *s = (char *) m;
	int i;
	unsigned long buffer;
	unsigned long *aligned_addr;
	unsigned int d = c & 0xff;	/* To avoid sign extension, copy C to an
					   unsigned variable.  */

	if (!TOO_SMALL (n) && !UNALIGNED1 (m))
	{
		/* If we get this far, we know that n is large and m is word-aligned. */
		aligned_addr = (unsigned long*)m;

		/* Store D into each char sized location in BUFFER so that
		   we can set large blocks quickly.  */
		if (LITTLEBLOCKSIZE == 4)
		{
			buffer = (d << 8) | d;
			buffer |= (buffer << 16);
		}
		else
		{
			buffer = 0;
			for (i = 0; i < LITTLEBLOCKSIZE; i++)
				buffer = (buffer << 8) | d;
		}

		while (n >= LITTLEBLOCKSIZE*4)
		{
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			*aligned_addr++ = buffer;
			n -= 4*LITTLEBLOCKSIZE;
		}

		while (n >= LITTLEBLOCKSIZE)
		{
			*aligned_addr++ = buffer;
			n -= LITTLEBLOCKSIZE;
		}
		/* Pick up the remainder with a bytewise loop.  */
		s = (char*)aligned_addr;
	}

	while (n--)
	{
		*s++ = (char)d;
	}

	return m;
}
#endif

#ifndef __HAVE_ARCH_MEMCPY
void *memcpy(void *dst0, const void *src0, size_t len0)
{
	char *dst = dst0;
	const char *src = src0;
	long *aligned_dst;
	const long *aligned_src;
	int   len =  len0;

	/* If the size is small, or either SRC or DST is unaligned,
	   then punt into the byte copy loop.  This should be rare.  */
	if (!TOO_SMALL(len) && !UNALIGNED2 (src, dst))
	{
		aligned_dst = (long*)dst;
		aligned_src = (long*)src;

		/* Copy 4X long words at a time if possible.  */
		while (len >= BIGBLOCKSIZE)
		{
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			*aligned_dst++ = *aligned_src++;
			len -= BIGBLOCKSIZE;
		}

		/* Copy one long word at a time if possible.  */
		while (len >= LITTLEBLOCKSIZE)
		{
			*aligned_dst++ = *aligned_src++;
			len -= LITTLEBLOCKSIZE;
		}

		/* Pick up any residual with a byte copier.  */
		dst = (char*)aligned_dst;
		src = (char*)aligned_src;
	}

	while (len--)
		*dst++ = *src++;

	return dst0;
}

#endif

#ifndef __HAVE_ARCH_MEMMOVE
void *memmove(void *to, const void *from, size_t n)
{
	// memcpy may overwrite when source and destination overlap and
	// destination start before the end of source
	// otherwise, memcpy() is enough
	if (to > from && ((unsigned long)from + n > (unsigned long)to)) {
		unsigned long i;
		unsigned char *_to = (unsigned char *)to + n;
		unsigned char *_from = (unsigned char *)from + n;

		for (i = 0; i < n; ++i)
			*--_to = *--_from;
	} else if (to != from)
		memcpy(to, from, n);

	return to;
}
#endif

#ifndef __HAVE_ARCH_MEMCMP
int memcmp(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}
#endif

void *memscan(void *addr, int c, size_t size)
{
	unsigned char *p = addr;

	while (size) {
		if (*p == c)
			return (void *)p;
		p++;
		size--;
	}
	return (void *)p;
}

char *strstr(const char *s1, const char *s2)
{
	size_t l1, l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *)s1;
	l1 = strlen(s1);
	while (l1 >= l2) {
		l1--;
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}

char *strnstr(const char *s1, const char *s2, size_t len)
{
	size_t l2;

	l2 = strlen(s2);
	if (!l2)
		return (char *)s1;
	while (len >= l2) {
		len--;
		if (!memcmp(s1, s2, l2))
			return (char *)s1;
		s1++;
	}
	return NULL;
}

void *memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = s;
	while (n-- != 0) {
		if ((unsigned char)c == *p++) {
			return (void *)(p - 1);
		}
	}
	return NULL;
}

char *strtok(char *str, const char *token)
{
	static char *s_last = NULL;
	int i;

	if (str == NULL)
		str = s_last;
	if (str == NULL)
		return NULL;

	for (i = 0; str[i] != 0; ++i)
		if (strchr(token, str[i]) != NULL)
			break;

	if (str[i] != 0)
		str[i++] = 0;

	for (; str[i] != 0; ++i)
		if (strchr(token, str[i]) == NULL)
			break;

	s_last = (str[i] == 0) ? NULL : str + i;

	return str;
}

char *trim(char *str)
{
	char *cp = str;

	while (*cp != 0 && isspace(*cp))
		++cp;

	if (cp == str)
		cp += strlen(str) - 1;
	else {
		char *cpdes;
		for (cpdes = str; *cp != 0;)
			*cpdes++ = *cp++;
		cp = cpdes;
		*cp-- = 0;
	}

	while (cp > str && isspace(*cp))
		*cp-- = 0;

	return str;
}

unsigned char *string_to_hex(const char *str, int *len)
{
	unsigned char *hexbuf, *q;
	unsigned char ch, cl, *p;

	if (!str) {
		return NULL;
	}
	if (!(hexbuf = malloc(strlen(str) >> 1)))
		goto err;
	for (p = (unsigned char *)str, q = hexbuf; *p;) {
		ch = *p++;
		if (ch == ':')
			continue;
		cl = *p++;
		if (!cl) {
			free(hexbuf);
			return NULL;
		}
		if (isupper(ch))
			ch = tolower(ch);
		if (isupper(cl))
			cl = tolower(cl);

		if ((ch >= '0') && (ch <= '9'))
			ch -= '0';
		else if ((ch >= 'a') && (ch <= 'f'))
			ch -= 'a' - 10;
		else
			goto badhex;

		if ((cl >= '0') && (cl <= '9'))
			cl -= '0';
		else if ((cl >= 'a') && (cl <= 'f'))
			cl -= 'a' - 10;
		else
			goto badhex;

		*q++ = (ch << 4) | cl;
	}

	if (len)
		*len = q - hexbuf;

	return hexbuf;

err:
	if (hexbuf)
		free(hexbuf);
	return NULL;

badhex:
	free(hexbuf);
	return NULL;
}
