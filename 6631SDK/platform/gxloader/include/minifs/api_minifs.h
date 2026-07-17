#ifndef _API_MINIFS_H_
#define _API_MINIFS_H_

typedef void* minifs_handle_file;
typedef void* minifs_handle_dev;

// mode

/* open with MINIFS_CREAT, if the file/dir is not exist, will create new */
#ifndef MINIFS_CREAT
#define MINIFS_CREAT     00000100
#endif

/*------------------------ FUNCTION PROTOTYPE(S) -----------------------------*/
/*
 * minifs_mount
 *
 *  DESCRIPTION
 *      initialize the minifs filesystem
 *
 *  PARAMETER
 *      @start   : the minifs filesystem start addr in flash, must be align with 64KB
 *      @end     : the minifs filesystem end addr in flash, must be align with 64KB
 *      @devname : the flash device name
 *      @path    : the minifs root path
 *      @buffer  : the minifs data buffer
 *      @size    : the minifs data size
 *      @init    : the flash device init function
 *
 *  RETURN VALUE
 *      On success, a minifs_device point is returned
 *      On error, NULL is returned
 *
 *  NOTES
 *      the start and end must be align with 64KB
 *
 * */
minifs_handle_dev minifs_mount(int start, int end, const char *devname, const char *path, void *buffer, size_t size, void *init);

/*
 * minifs_umount
 *
 *  DESCRIPTION
 *      destory the minifs filesystem
 *
 *  PARAMETER
 *      @dev    : a minifs device
 *
 *  RETURN VALUE
 *
 *  NOTES
 *
 * */
void minifs_umount (minifs_handle_dev dev);

/*
 * minifs_open
 *
 *  DESCRIPTION
 *      open a minifs file
 *
 *  PARAMETER
 *      @dev      : a minifs device
 *      @filename : a minifs absolute path
 *      @flags    : file operation mode, create a new file must set flags = MINIFS_CREAT
 *                  open a existed file can set flags = 0
 *      @mode     : file operation permission
 *
 *  RETURN VALUE
 *      On success, a minifs_Handle point is returned
 *      On error, NULL is returned
 *
 *  NOTES
 *
 * */
minifs_handle_file minifs_open(minifs_handle_dev dev, const char *filename, int flags, int mode);

/*
 * minifs_close
 *
 *  DESCRIPTION
 *      close a minifs file
 *
 *  PARAMETER
 *      @h     : a minifs file minifs_Handle
 *
 *  RETURN VALUE
 *      On success, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_close   (minifs_handle_file h);

/*
 * minifs_sync
 *
 *  DESCRIPTION
 *      sync a minifs file
 *
 *  PARAMETER
 *      @h     : a minifs file minifs_Handle
 *
 *  RETURN VALUE
 *      On success, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_sync(minifs_handle_file *handle);
/*
 * minifs_read
 *
 *  DESCRIPTION
 *      attempts to read up to nbyte bytes to buf from file h starting at offset
 *
 *  PARAMETER
 *      @h      : a minifs file minifs_Handle
 *      @buf    : read data buffer
 *      @nbyte  : read data length
 *      @offset : the offset in the file
 *
 *  RETURN VALUE
 *      On success, read data length is returned
 *      On error, a negative value or zero is returned
 *
 *  NOTES
 *
 * */
int minifs_read    (minifs_handle_file h, void *buf, size_t nbyte, off_t offset);

/*
 * minifs_write
 *
 *  DESCRIPTION
 *      attempts to write up to nbyte bytes from buf to file h starting at offset
 *
 *  PARAMETER
 *      @h      : a minifs file minifs_Handle
 *      @buf    : write data buffer
 *      @nbyte  : write data length
 *      @offset : the offset in the file
 *
 *  RETURN VALUE
 *      On success, write data length is returned
 *      On error, a negative value or zero is returned
 *
 *  NOTES
 *
 * */
int minifs_write   (minifs_handle_file h, const void *buf, size_t nbyte, off_t offset);

/*
 * minifs_size
 *
 *  DESCRIPTION
 *      get the size of the minifs file
 *
 *  PARAMETER
 *      @h      : a minifs file minifs_Handle
 *
 *  RETURN VALUE
 *      On success, the size of the file is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_size    (minifs_handle_file h);

/*
 * minifs_truncate
 *
 *  DESCRIPTION
 *      truncate the file length to size
 *
 *  PARAMETER
 *      @h      : a minifs file minifs_Handle
 *      @size   : the file length after truncation
 *
 *  RETURN VALUE
 *      On success, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_truncate(minifs_handle_file h, size_t size);

/*
 * minifs_rename
 *
 *  DESCRIPTION
 *      rename a minifs file
 *
 *  PARAMETER
 *      @dev      : a minifs device
 *      @oldPath  : a old absolute path of a minifs file
 *      @newPath  : a new absolute path of the minifs file
 *
 *  RETURN VALUE
 *      On success, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_rename   (minifs_handle_dev dev, const char *oldPath, const char *newPath);

/*
 * minifs_rmfile
 *
 *  DESCRIPTION
 *      delete a minifs file
 *
 *  PARAMETER
 *      @dev      : a minifs device
 *      @filename : a minifs file want to be deleted
 *
 *  RETURN VALUE
 *      On success, 1 is returned
 *      On error, zero is returned
 *
 *  NOTES
 *
 * */

int minifs_rmfile   (minifs_handle_dev dev, const char *filename);

/*
 * minifs_mkdir
 *
 *  DESCRIPTION
 *      create a minifs directory
 *
 *  PARAMETER
 *      @dev      : a minifs device
 *      @path     : a absolute path of a minifs directory
 *
 *  RETURN VALUE
 *      On success, zero is returned
 *      On error, a negative value is returned
 *
 *  NOTES
 *
 * */
int minifs_mkdir    (minifs_handle_dev dev, const char *path, unsigned int mode);

#endif

