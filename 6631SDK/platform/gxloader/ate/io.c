#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "fs.h"


#ifdef DBG
#define IO_ERR(fmt, ...) printf("ERR : " "func : <%s> line : <%d> " fmt, __func__, __LINE__, #__VA_ARGS__)
#else
#define IO_ERR(fmt, ...)
#endif

static mtab_entry mtab[MAX_MTAB];
static struct file_desc f_d[MAX_FD];

extern mtab_entry fake_ramfs_mte;
extern mtab_entry devfs_mte;

// todo, 文件系统重复注册检查
static int mtab_register(mtab_entry *mte)
{
	static int init = 0;
	int i;

	if (!init) {
		memset(mtab, 0, sizeof(mtab));
		init = 1;
	}

	if (!mte) {
		IO_ERR("invalid parameter\n");
		return -1;
	}

	for (i = 0; i < MAX_MTAB; i++) {
		if (!mtab[i].valid) {
			memcpy(&mtab[i], mte, sizeof(mtab_entry));
			mtab[i].valid = 1;
			break;
		}
	}

	if (i == MAX_MTAB) {
		IO_ERR("no free mtab\n");
		return -1;
	}

	return 0;
}

static void mtab_unregister(mtab_entry *mte)
{
	int i;

	if (!mte)
		return ;

	for (i = 0; i < MAX_MTAB; i++) {
		if (mtab[i].valid) {
			if (0 == strcmp(mtab[i].fsname, mte->fsname)) {
				memset(&mtab[i], 0, sizeof(mtab_entry));
				break;
			}
		}
	}
}

int io_framework_init(void)
{
	mtab_register(&fake_ramfs_mte);
	mtab_register(&devfs_mte);

	return 0;
}

static int mtab_lookup(const char *filename, mtab_entry **mte)
{
	char fs_name[FS_NAME_MAX_LEN] = {0};
	int i;

	if (!filename || !mte)
		return -1;

	if (strlen(filename) > 4 && (0 == strncmp(filename, "/dev", 4)))
		memcpy(fs_name, DEVFS_NAME, strlen(DEVFS_NAME));
	else
		memcpy(fs_name, FAKE_RAMFS_NAME, strlen(FAKE_RAMFS_NAME));

	for (i = 0; i < MAX_MTAB; i++) {
		if (mtab[i].valid)
			if (strcmp(mtab[i].fsname, fs_name) == 0) {
				*mte = &mtab[i];
				break;
			}
	}

	if (i == MAX_MTAB) {
		IO_ERR("not found file %s in mtab\n", filename);
		return -1;
	}

	return 0;
}

#define FD_INVALID(fd) ((fd > (MAX_FD - 1)) || (fd < 0))
static int fd_alloc(void)
{
	static int init = 0;
	int i;

	if (!init) {
		memset(f_d, 0, sizeof(f_d));
		init = 1;
	}

	for (i = 0; i < MAX_FD; i++) {
		if (!f_d[i].in_use) {
			memset(&f_d[i], 0, sizeof(struct file_desc));
			f_d[i].in_use = 1;
			break;
		}
	}

	if (i == MAX_FD) {
		IO_ERR("fd is not enough\n");
		return -1;
	}

	return i;
}

static void fd_free(int fd)
{
	if (FD_INVALID(fd)) {
		IO_ERR("fd is invalid\n");
		return ;
	}

	if (f_d[fd].in_use)
		memset(&f_d[fd], 0, sizeof(struct file_desc));
}

/*
 * name: open file name
 * flags: not support now
 */
int open(const char *name, int flags)
{
	mtab_entry *mte;
	int ret;
	int fd;

	if (!name)
		return -1;

	fd = fd_alloc();
	if (fd == -1)
		return -1;

	ret = mtab_lookup(name, &f_d[fd].mte);
	if (ret == -1)
		goto out;

	mte = f_d[fd].mte;
	if (mte->fs->fo_open)
		ret = mte->fs->fo_open(&f_d[fd], name, flags);
	else {
		IO_ERR("no fo_open function ?\n");
		ret = -1;
	}

	if (ret != 0)
		goto out;

	return fd;
out:
	fd_free(fd);
	return ret;
}

int close(int fd)
{
	int ret;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_close)
		ret = f_d[fd].f_ops->fo_close(&f_d[fd]);
	else {
		IO_ERR("no fo_close function ?\n");
		ret = -1;
	}

	if (ret == 0)
		fd_free(fd);

	return ret;
}

int write(int fd, const void *buf, size_t size)
{
	int ret;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_write)
		ret = f_d[fd].f_ops->fo_write(&f_d[fd], buf, size);
	else {
		IO_ERR("no fo_write function ?\n");
		ret = -1;
	}

	return ret;
}

int read(int fd, void *buf, size_t size)
{
	int ret;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_read)
		ret = f_d[fd].f_ops->fo_read(&f_d[fd], buf, size);
	else {
		IO_ERR("no fo_read function ?\n");
		ret = -1;
	}

	return ret;
}

int ioctl(int fd, int request, void *private, ...)
{
	int ret;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_ioctl) {
		ret = f_d[fd].f_ops->fo_ioctl(&f_d[fd], request, private);
		if (ret != 0) {
			errno = -ret;
			ret = -1;
		}
	} else {
		IO_ERR("no fo_ioctl function ?\n");
		ret = -1;
	}

	return ret;
}

int lseek(int fd, int offset, int whence)
{
	int ret;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_lseek)
		ret = f_d[fd].f_ops->fo_lseek(&f_d[fd], offset, whence);
	else {
		IO_ERR("no fo_lseek function ?\n");
		ret = -1;
	}

	return ret;
}

int fstat(int fd, struct stat *buf)
{
	int ret = 0;

	if (!buf)
		return -1;

	if (FD_INVALID(fd) || !f_d[fd].in_use) {
		IO_ERR("invalid fd\n");
		return -1;
	}

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_fstat)
		ret = f_d[fd].f_ops->fo_fstat(&f_d[fd], buf);
	else {
		IO_ERR("no fo_fstat function ?\n");
		ret = -1;
	}

	return ret;
}

int stat(const char *name, struct stat *buf)
{
	int fd = 0;
	int ret = 0;
	int open = 0;
	mtab_entry *mte = NULL;

	if ((!name) || (!buf))
		return -1;

	fd = fd_alloc();
	if (fd == -1)
		return -1;

	ret = mtab_lookup(name, &f_d[fd].mte);
	if (ret == -1)
		goto out;

	mte = f_d[fd].mte;
	if (mte->fs->fo_open)
		ret = mte->fs->fo_open(&f_d[fd], name, 0);
	else {
		IO_ERR("no fo_open function ?\n");
		ret = -1;
	}

	if (ret < 0)
		goto out;
	else
		open = 1;

	if (f_d[fd].f_ops && f_d[fd].f_ops->fo_fstat)
		ret = f_d[fd].f_ops->fo_fstat(&f_d[fd], buf);
	else {
		IO_ERR("no fo_fstat function ?\n");
		ret = -1;
	}

out:
	if (open > 0) {
		if (f_d[fd].f_ops && f_d[fd].f_ops->fo_close)
			ret = f_d[fd].f_ops->fo_close(&f_d[fd]);
		else {
			IO_ERR("no fo_close function ?\n");
			ret = -1;
		}
		open = 0;
	}
	if (fd > 0) {
		fd_free(fd);
		fd = 0;
	}

	return ret;
}
