#include <stdio.h>
#include <string.h>
#include "include/gxdev.h"
#include "include/io.h"
#include "linux/errno.h"

#ifdef DBG
#define ERR(fmt, ...) printf("ERR: " "func : <%s>, line : <%d> " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define ERR(fmt, ...)
#endif

#define IS_INVALID_STREAM(stream) ( ((unsigned long)stream < (unsigned long)&file[0]) || \
		(unsigned long)stream > (unsigned long)&file[MAX_FOPEN_NUM - 1])

#define MAX_FOPEN_NUM  256
static int file[MAX_FOPEN_NUM];

FILE *fopen(const char *path, const char *mode)
{
	static int init = 0;
	int i;
	int fd;

	if (!init) {
		for (i = 0; i < MAX_FOPEN_NUM; i++)
			file[i] = -1;
		init = 1;
	}

	if (!path) {
		ERR("invalid parameter\n");
		return NULL;
	}

	for (i = 0; i < MAX_FOPEN_NUM; i++) {
		if (file[i] == -1)
			break;
	}

	if (i == MAX_FOPEN_NUM) {
		ERR("no free fd\n");
		return NULL;
	}

	fd = open(path, 0);
	if (fd < 0) {
		ERR("open failed\n");
		return NULL;
	}

	file[i] = fd;

	return (FILE *)&file[i];
}

int fclose(FILE *stream)
{
	int fd;
	int ret;

	if (!stream || IS_INVALID_STREAM(stream)) {
		ERR("invalid parameter\n");
		return -1;
	}

	fd = *(int *)stream;

	ret = close(fd);

	if (ret == 0)
		*(int *)stream = -1;

	return ret;
}

#define FILE_DO_READ  0
#define FILE_DO_WRITE 1
static size_t f_read_write(void *ptr, size_t size, size_t nmemb, FILE *stream, int action)
{
	int fd;
	ssize_t ret = 0;

	if ((size * nmemb > 0 && !ptr) || !stream || IS_INVALID_STREAM(stream)) {
		ERR("invalid parameter\n");
		return 0;
	}

	fd = *(int *)stream;

	if (action == FILE_DO_READ)
		ret = read(fd, ptr, size * nmemb);
	else if (action == FILE_DO_WRITE)
		ret = write(fd, ptr, size * nmemb);

	if (ret >= 0)
		return (size_t)(ret / size);

	return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return f_read_write((void *)ptr, size, nmemb, stream, FILE_DO_WRITE);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	return f_read_write(ptr, size, nmemb, stream, FILE_DO_READ);
}

long ftell(FILE *stream)
{
	int fd;

	if (!stream || IS_INVALID_STREAM(stream)) {
		ERR("invalid parameter\n");
		return -1;
	}

	fd = *(int *)stream;

	return lseek(fd, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int whence)
{
	int fd;
	signed int ret;

	if (!stream || IS_INVALID_STREAM(stream)) {
		ERR("invalid parameter\n");
		return -1;
	}

	fd = *(int *)stream;
	ret = lseek(fd, offset, whence);
	if (ret < 0) {
		ERR("lseek failed\n");
		return -1;
	}

	return 0;
}

size_t _fwrite(const void *buf, size_t count, FILE * f)
{
	size_t bytes = 0;
	ssize_t rv;
	const char *p = buf;

	while (count) {
		rv = write(fileno(f), p, count);
		if (rv == -1) {
			if (errno == EINTR || errno == EAGAIN)
				continue;
			else
				break;
		} else if (rv == 0) {
			break;
		}

		p += rv;
		bytes += rv;
		count -= rv;
	}

	return bytes;
}
