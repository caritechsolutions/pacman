#ifndef __RECFS__
#define __RECFS__

#ifdef __cplusplus
extern "C" {
#endif

#include "gx_fileops.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct recinode_ops {
	int (*open)(const char *url, int mode, int flag);
	void (*close)(int fd);
	size_t (*read)(int fd, void* buf, size_t size);
	size_t (*write)(int fd, void* buf, size_t size);
	off_t (*lseek)(int fd, off_t pos, int flag);
};

struct record_inode {
	struct gxlist_head head;
	struct gxlist_head file;

	int fd;
	int ref;
	int mode;
	off_t size;
	int error;

	void *filename;

	struct recinode_ops *ops;

	handle_t mutex;
};

struct record_file {
	struct gxlist_head head;
	struct record_inode *inode;
	handle_t mutex;
	off_t offset;
};

struct record_file *recfile_open(const char* filename, int mode);
ssize_t recfile_read(struct record_file *rf, void *buf, size_t size);
ssize_t recfile_readAt(struct record_file *rf, void *buf, size_t size, off_t offset);
ssize_t recfile_write(struct record_file *rf, void *buf, size_t size);
off_t recfile_seek(struct record_file *rf, off_t off, int w);
int   recfile_close(struct record_file *rf);
int   recfile_eof(struct record_file *rf);
off_t recfile_getsize(struct record_file *rf);
int   recfile_fsync(struct record_file *rf);
void  recfile_rm(const char* filename);
off_t recfile_getpos(struct record_file *rf);
int   recfile_error(struct record_file *rf);

#ifdef __cplusplus
}
#endif

#endif
