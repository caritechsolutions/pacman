#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "ts_file.h"
#include "gxcore.h"


//mode : STREAM_READ     STREAM_WRITE
static int file_open(GxTsFile * s, int mode)
{
	mode_t m = 0;
	uint32_t len;
	char *filename;

	if (s == NULL || s->url == NULL)
		return GXCORE_ERROR;

	if (mode == STREAM_READ)
		m = O_RDONLY;
	else if (mode == STREAM_WRITE)
		m = O_RDWR | O_CREAT | O_TRUNC;
	else {
		gxlogd("[file] Unknown open mode %d\n", mode);
		return GXCORE_ERROR;
	}

	filename = strstr(s->url, "://");

	if (filename != NULL)
		filename += 3;
	else
		filename = s->url;

	s->fd = open(filename, m, 0644);
	if (s->fd < 0) {
		return GXCORE_ERROR;
	}

	len = lseek(s->fd, 0, SEEK_END);
	lseek(s->fd, 0, SEEK_SET);
	s->end_pos = len;

	return GXCORE_SUCCESS;
}

static int file_read(GxTsFile * s, uint8_t * buffer, size_t max_len)
{

	int r = read(s->fd, buffer, max_len);

	return (r <= 0) ? -1 : r;
}

static int file_write(GxTsFile * s, uint8_t * buffer, size_t len)
{
	int r = write(s->fd, buffer, len);

	return (r <= 0) ? -1 : r;
}

static int file_seek(GxTsFile * s, uint32_t newpos)
{
	s->pos = newpos;
	if (lseek(s->fd, s->pos, SEEK_SET) < 0) {
		s->eof = 1;
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

/*static int file_control(GxTsFile * s, int cmd, va_list arg)
{
	switch (cmd) {
	case GX_STREAM_CTRL_GET_SIZE:
		{
			uint32_t *size;

			size = (uint32_t *) va_arg(arg, uint32_t *);

			*size = lseek(s->fd, 0, SEEK_END);
			lseek(s->fd, s->pos, SEEK_SET);
			if (*size != (uint32_t) - 1) {
				return GXCORE_SUCCESS;
			}
		}
	}

	return GXCORE_ERROR;
}*/

static void file_close(GxTsFile * s)
{
	close(s->fd);
}

GxTsFileCtrl TsFileCtrl = {(char *)"file", file_open, file_read, file_write, file_seek, file_close};
/*{
	.name = "file",
	.open    = file_open,
	.read    = file_read,
	.write   = file_write,
	.seek    = file_seek,
//	.control = file_control,
	.close   = file_close,
};*/

GxTsFile g_TsFile = {0,0,0,0,0,(char *)"/home/share/ts.ts"};
/*{
	.url = "/home/share/ts.ts"
};*/



