#ifndef __NO_OS_FS_H_
#define __NO_OS_FS_H_

#include "sys/types.h"

struct mtab_entry;
struct file_desc;

/* filesystem's max name length*/
#define FS_NAME_MAX_LEN 20
#define DEVFS_NAME "devfs"
#define FAKE_RAMFS_NAME "fake ramfs"

#define MAX_MTAB 10
#define MAX_FD 256

struct stat {
//    unsigned short  st_mode;     /* File mode */
//    ino_t   st_ino;      /* File serial number */
//    dev_t   st_dev;      /* ID of device containing file */
//    nlink_t st_nlink;    /* Number of hard links */
//    uid_t   st_uid;      /* User ID of the file owner */
//    gid_t   st_gid;      /* Group ID of the file's group */
      long    st_size;     /* File size (regular files only) */
//    time_t  st_atime;    /* Last access time */
//    time_t  st_mtime;    /* Last data modification time */
//    time_t  st_ctime;    /* Last file status change time */
};

typedef ssize_t fileop_read   (struct file_desc *fd_p, void *buf, size_t count);
typedef ssize_t fileop_write  (struct file_desc *fd_p, const void *buf, size_t count);
typedef signed  fileop_lseek  (struct file_desc *fd_p, signed offset, int whence);
typedef int     fileop_ioctl  (struct file_desc *fd_p, int request, void *private);
typedef int     fileop_select (struct file_desc *fd_p); // no use now
typedef int     fileop_close  (struct file_desc *fd_p);
typedef int     fileop_fstat  (struct file_desc *fd_p, struct stat *buf);
typedef int     fileop_getinfo(struct file_desc *fd_p, void *private);
typedef int     fileop_setinfo(struct file_desc *fd_p, void *private);

typedef struct file_ops {
	fileop_read    *fo_read;
	fileop_write   *fo_write;
	fileop_lseek   *fo_lseek;
	fileop_ioctl   *fo_ioctl;
	fileop_select  *fo_select;
	fileop_close   *fo_close;
	fileop_fstat   *fo_fstat;
	fileop_getinfo *fo_getinfo;
	fileop_setinfo *fo_setinfo;
}file_ops;

typedef int fsop_open (struct file_desc *fd_p, const char *name, int mode);

typedef struct fstab_entry {
	const char *name;
	void       *private;

	fsop_open  *fo_open;
}fstab_entry;

/* mount table entry
 * name: mound point name, no use now
 * fsname: filesystem name, can't be NULL
 * private: private data
 * valid: Indicates whether the current data structure is available
 * fs: fs table entry
 */
typedef struct mtab_entry {
	const char    *name;
	char          *fsname;
	void          *private;
	unsigned char  valid;
	fstab_entry   *fs;
}mtab_entry;


struct file_desc {
    unsigned char in_use;
    int           mode;
    size_t        offset;
    size_t        type;  //descriptor type
    file_ops     *f_ops;
    mtab_entry   *mte;

	void *private;
};

#endif
