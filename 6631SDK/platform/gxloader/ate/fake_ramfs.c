#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fs.h"

#ifdef DBG
#define FR_ERR(fmt, ...) printf("Fake ramfs ERR : " "func : <%s> line : <%d> " fmt, __func__, __LINE__, ##__VA_ARGS__)
#else
#define FR_ERR(fmt, ...)
#endif

/* In order to reduce memory fragmentation and frequent memory requests */
#define FAKE_RAMFS_MALLOC_MIN_SIZE 256

#define FAKE_RAMFS_FILENAME_MAX_LEN 32
typedef struct fake_ramfs_file {
	int    valid;
	char   file_name[FAKE_RAMFS_FILENAME_MAX_LEN];
	int    use_count;
	int    mode;
	void  *data;
	size_t file_length;   /* file actual length */
	size_t malloc_length; /* memory length */
} fake_ramfs_file;

typedef struct fake_ramfs_info {
	int valid;
	fake_ramfs_file *fp;
} fake_ramfs_info;

#define FAKE_RAM_MAX_FILE_NUM  300
#define FAKE_RAMFS_MAX_FILE_OPEN_NUM 256

static fake_ramfs_info fr_info[FAKE_RAMFS_MAX_FILE_OPEN_NUM];
static fake_ramfs_file fr_file[FAKE_RAM_MAX_FILE_NUM];
static int fr_file_init = 0;

static ssize_t fr_read(struct file_desc *fd_p, void *buf, size_t count);
static ssize_t fr_write(struct file_desc *fd_p, const void *buf, size_t count);
static signed  fr_lseek(struct file_desc *fd_p, signed int offset, int whence);
static int     fr_fstat(struct file_desc *fd_p, struct stat *stat);
static int     fr_open(struct file_desc *fp, const char *name, int mode);
static int     fr_close(struct file_desc *fp);

static fstab_entry fake_ramfs_fte = {
	.name = FAKE_RAMFS_NAME,
	.fo_open = &fr_open,
};

mtab_entry fake_ramfs_mte = {
	.fsname = FAKE_RAMFS_NAME,
	.fs = &fake_ramfs_fte,
};

static file_ops fake_ramfs_op =
{
	.fo_read    = &fr_read,
	.fo_write   = &fr_write,
	.fo_lseek   = &fr_lseek,
	.fo_ioctl   = NULL,
	.fo_select  = NULL,
	.fo_close   = &fr_close,
	.fo_fstat   = &fr_fstat,
	.fo_getinfo = NULL,
	.fo_setinfo = NULL,
};

static ssize_t fr_read(struct file_desc *fd_p, void *buf, size_t count)
{
	size_t offset;
	size_t max_read_size = 0;
	fake_ramfs_info *info = NULL;
	fake_ramfs_file *file = NULL;

	if (!fd_p || (count != 0 && !buf)) {
		FR_ERR("invalid paramters\n");
		return -1;
	}

	offset = fd_p->offset;
	info = fd_p->private;
	file = info->fp;

	if ((offset + count) > file->file_length)
		max_read_size = file->file_length - offset;
	else
		max_read_size = count;

	memmove(buf, (void *)((size_t)file->data + offset), max_read_size);
	fd_p->offset += max_read_size;

	return (ssize_t)max_read_size;
}

static ssize_t fr_write(struct file_desc *fd_p, const void *buf, size_t count)
{
	fake_ramfs_info *info = NULL;
	fake_ramfs_file *file = NULL;
	size_t offset;
	size_t malloc_size = 0;
	unsigned char need_malloc = 0;


	if (!fd_p || (count != 0 && !buf)) {
		FR_ERR("invalid paramters\n");
		return -1;
	}

	offset = fd_p->offset;
	info = fd_p->private;
	file = info->fp;

	if ((offset + count) > file->malloc_length) {
		need_malloc = 1;
		malloc_size = (offset + count) / FAKE_RAMFS_MALLOC_MIN_SIZE * FAKE_RAMFS_MALLOC_MIN_SIZE \
					  + FAKE_RAMFS_MALLOC_MIN_SIZE;
	}

	if (need_malloc) {
		void *tp = malloc(malloc_size);
		if (!tp) {
			FR_ERR("zalloc failed\n");
			return -1;
		}
		memset(tp, 0, malloc_size);

		if (file->data)
			memmove(tp, file->data, offset);
		memmove((void *)((size_t)tp + offset), buf, count);
		offset += count;

		if (file->data)
			free(file->data);
		file->data = tp;

		/* update file size */
		file->file_length   = offset;
		file->malloc_length = malloc_size;
	} else {
		if (offset + count > file->file_length)
			file->file_length = offset + count;
		memmove((void *)((size_t)file->data + offset), buf, count);
		offset += count;
	}

	fd_p->offset = offset;

	return (ssize_t)count;
}

static signed int fr_lseek(struct file_desc *fd_p, signed int offset, int whence)
{
	fake_ramfs_info *info = NULL;
	fake_ramfs_file *file = NULL;
	size_t cur_offset;

	if (!fd_p)
		return -1;

	info = fd_p->private;
	file = info->fp;

	cur_offset = fd_p->offset;
	switch (whence) {
	case SEEK_SET:
		if (offset < 0 || offset > file->file_length) {
			FR_ERR("invalid operating\n");
			return -1;
		}

		fd_p->offset = offset;
		break;
	case SEEK_CUR:
		if (((signed int)cur_offset + offset < 0) || (cur_offset + offset > file->file_length)) {
			FR_ERR("invalid operating\n");
			return -1;
		}

		fd_p->offset = (signed int)cur_offset + offset;
		break;
	case SEEK_END:
		if (offset > 0 || (offset + (signed int)file->file_length < 0)) {
			FR_ERR("invalid operating\n");
			return -1;
		}

		fd_p->offset = offset + (signed int)file->file_length;
		break;
	default:
		FR_ERR("invalid operating\n");
		return -1;
	}

	return (signed int)fd_p->offset;
}

static int fr_fstat(struct file_desc *fd_p, struct stat *stat)
{
	fake_ramfs_info *info;
	fake_ramfs_file *file;

	if (!fd_p || !stat) {
		FR_ERR("invalid parameters\n");
		return -1;
	}

	info = fd_p->private;
	file = info->fp;

	stat->st_size = file->file_length;

	return 0;
}

static fake_ramfs_info *alloc_fake_ramfs_info(void)
{
	int i;

	for (i = 0; i < FAKE_RAMFS_MAX_FILE_OPEN_NUM; i++) {
		if (!fr_info[i].valid) {
			fr_info[i].valid = 1;
			return &fr_info[i];
		}
	}

	FR_ERR("exceed the max file open num\n");
	return NULL;
}

static void free_fake_ramfs_info(fake_ramfs_info *info)
{
	if (!info)
		return ;

	if (info->valid)
		memset(info, 0, sizeof(fake_ramfs_info));
}

static fake_ramfs_file *fake_ramfs_find_file(const char *name)
{
	int i;

	for (i = 0; i < FAKE_RAM_MAX_FILE_NUM; i++)
		if (fr_file[i].valid && (strcmp(fr_file[i].file_name, name) == 0))
			return &fr_file[i];

	return NULL;
}

/*
 * fr_open : fake ramfs open function
 * return : 0 on success, -1 on failed
 */
static int file_create(const char *name, void *buf, unsigned int size);
static int fr_open(struct file_desc *fd_p, const char *name, int mode)
{
	static int init = 0;
	fake_ramfs_info *info = NULL;
	fake_ramfs_file *file = NULL;
	int ret = 0;

	if (!fr_file_init) {
		memset(fr_file, 0, sizeof(fr_file));
		fr_file_init = 1;
	}
	if (!init) {
		memset(fr_info, 0, sizeof(fr_info));
		init = 1;
	}

	info = alloc_fake_ramfs_info();
	if (!info) {
		FR_ERR("no free handle\n");
		return -1;
	}

	file = fake_ramfs_find_file(name);
	if (!file) {
		ret = file_create(name, NULL, 0);
		if (ret) {
			FR_ERR("file create failed\n");
			goto failed;
		}
		file = fake_ramfs_find_file(name);
	}

	file->use_count++;
	info->fp = file;

	fd_p->offset = 0;
	fd_p->f_ops = &fake_ramfs_op;
	fd_p->private = info;
	return 0;

failed:
	free_fake_ramfs_info(info);
	return -1;
}

static int fr_close(struct file_desc *fd_p)
{
	fake_ramfs_file *file;

	if (!fd_p)
		return -1;

	file = ((fake_ramfs_info *)fd_p->private)->fp;
	file->use_count--;

	free_fake_ramfs_info((fake_ramfs_info *)fd_p->private);

	fd_p->f_ops = NULL;
	fd_p->private = NULL;

	return 0;
}

/*
 * size:
 *  > 0 will malloc memory to save
 *  = 0 just create a empty file 
 *  < 0 invalid
 */
static int file_create(const char *name, void *buf, unsigned int size)
{
	int i;

	if (!fr_file_init) {
		memset(fr_file, 0, sizeof(fr_file));
		fr_file_init = 1;
	}

	if (!name || (size > 0 && !buf)) {
		FR_ERR("invalid parameters\n");
		return -1;
	}

	if (strlen(name) >= FAKE_RAMFS_FILENAME_MAX_LEN) {
		FR_ERR("file name too long, max support len : %d\n", FAKE_RAMFS_FILENAME_MAX_LEN);
		return -1;
	}

	for (i = 0; i < FAKE_RAM_MAX_FILE_NUM; i++) {
		if (!fr_file[i].valid) {
			memset(&fr_file[i], 0, sizeof(fake_ramfs_file));
			fr_file[i].valid = 1;
			break;
		}
	}
	if (i == FAKE_RAM_MAX_FILE_NUM) {
		FR_ERR("exceed the max file num\n");
		return -1;
	}

	/* The name length has been guaranteed to be less than or equal to (FAKE_RAMFS_FILENAME_MAX_LEN - 1) */
	memcpy(fr_file[i].file_name, name, strlen(name));
	if (size > 0) {
		unsigned int malloc_size = size / FAKE_RAMFS_MALLOC_MIN_SIZE * FAKE_RAMFS_MALLOC_MIN_SIZE \
								   + FAKE_RAMFS_MALLOC_MIN_SIZE;
		fr_file[i].data = malloc(malloc_size);
		if (!fr_file[i].data) {
			FR_ERR("calloc failed\n");
			goto out;
		}
		memset(fr_file[i].data, 0, malloc_size);
		memmove(fr_file[i].data, buf, size);
		fr_file[i].file_length = size;
		fr_file[i].malloc_length = malloc_size;
	}

	return 0;
out:
	memset(&fr_file[i], 0, sizeof(fake_ramfs_file));
	return -1;
}

static int file_delete(const char *name)
{
	int i;

	if (!name || strlen(name) >= FAKE_RAMFS_FILENAME_MAX_LEN) {
		FR_ERR("invalid parameters\n");
		return -1;
	}

	for (i = 0; i < FAKE_RAM_MAX_FILE_NUM; i++) {
		if (fr_file[i].valid && strcmp(fr_file[i].file_name, name) == 0) {
			if (fr_file[i].use_count != 0) {
				FR_ERR("file is being used\n");
				return -1;
			}

			if (fr_file[i].data)
				free(fr_file[i].data);
			memset(&fr_file[i], 0, sizeof(fake_ramfs_file));
			return 0;
		}
	}

	FR_ERR("no such file\n");
	return -1;
}

int file_add(const char *name, void *buf, unsigned int size)
{
	return file_create(name, buf, size);
}

int file_del(const char *name)
{
	return file_delete(name);
}
