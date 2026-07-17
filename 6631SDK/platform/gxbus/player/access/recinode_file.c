
#include "recfs.h"


static int _file_open(const char *url, int mode, int flag)
{
	return open(url, mode, flag);
}

static void _file_close(int fd)
{
	close(fd);
}

static size_t _file_read(int fd, void* buf, size_t size)
{
	return read(fd, buf, size);
}

static size_t _file_write(int fd, void* buf, size_t size)
{
	return write(fd, buf, size);
}

static off_t _file_lseek(int fd, off_t pos, int flag)
{
	return lseek(fd, pos, flag);
}


struct recinode_ops recinodeops_file = {
	.open   = _file_open,
	.close  = _file_close,
	.read   = _file_read,
	.write  = _file_write,
	.lseek  = _file_lseek,
};
