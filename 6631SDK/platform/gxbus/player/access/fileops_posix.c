#include "gx_fileops.h"
#include "recfs.h"

#if 1
static int posix_open(char*name , mode_t mode)
{
	return open(name,mode, 0666);
}

static int posix_read(int fildes, uint8_t * buffer, size_t max_len, int timeoutms)
{
	return read(fildes, buffer, max_len);
}

static int posix_write(int fildes, uint8_t * buffer, size_t len)
{
	return write(fildes, buffer, len);
}

static off_t posix_seek(int fildes, off_t offset, int whence)
{
	return lseek(fildes, offset, whence);
}

static int posix_close(int fildes)
{
	close(fildes);
	return 0;
}

static off_t posix_getsize(int fildes)
{
	struct stat filestat;
	fstat(fildes, &filestat);
	return filestat.st_size;
}
#else
static int posix_open(char*name , mode_t mode)
{
	return (int)recfile_open(name, mode);
}

static int posix_read(int fildes, uint8_t * buffer, size_t max_len, int timeoutms)
{
	return recfile_read((struct record_file *)fildes, buffer, max_len);
}

static int posix_write(int fildes, uint8_t * buffer, size_t len)
{
	return recfile_write((struct record_file *)fildes, buffer, len);
}

static off_t posix_seek(int fildes, off_t offset, int whence)
{
	return recfile_seek((struct record_file *)fildes, offset, whence);
}

static int posix_close(int fildes)
{
	recfile_close((struct record_file *)fildes);
	return 0;
}

static off_t posix_getsize(int fildes)
{
	return recfile_getsize((struct record_file *)fildes);
}
#endif


GxFileOps gx_fileops_posix = {
	.open    = posix_open,
	.read    = posix_read,
	.write   = posix_write,
	.seek    = posix_seek,
	.close   = posix_close,
	.getsize = posix_getsize,
};

