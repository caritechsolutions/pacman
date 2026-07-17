#ifndef __NO_OS_IO_H_
#define __NO_OS_IO_H_
#include "gxdev.h"
#include "fs.h"

#define O_RDONLY    (0)
#define O_WRONLY    (1)
#define O_RDWR      (2)

int     open(const char *name, int flags);
int     close(int fd);
/* blow use int instead ssize_t and use unsigned instead size_t */
int     write(int fd, const void *buf, unsigned size);
int     read(int fd, void *buf, unsigned size);
int     lseek(int fd, int offset, int whence);
int     fstat(int fd, struct stat *buf);
/* Different from the c Lib */
int     ioctl(int fd, int request, void *private, ...);
int     stat(const char *file_name, struct stat *buf);
/* io freamwork init */
int io_framework_init(void);

/* dev operate */
int  dev_register(struct gx_devio_ops *dev, char *name,void *priv_dev);
void dev_unregister(char *name);

/* fake ramfs operate */
int file_add(const char *name, void *buf, unsigned int size);
int file_del(const char *name);

void stdio_init(void);
#endif
