#include "recfs.h"

static struct gxlist_head record_inode_list;
static handle_t record_mutex = -1;

extern struct recinode_ops recinodeops_file;
extern struct recinode_ops recinodeops_mem;

static void recinode_delete(struct record_inode *inode, int force)
{
	struct record_file *rf = NULL;
	struct gxlist_head *pos;
	struct gxlist_head *n;

	GxCore_MutexLock(inode->mutex);
	gxlist_for_each_safe(pos, n, &inode->file) {
		rf = gxlist_entry(pos, struct record_file, head);
			rf->inode = NULL;
	}
	GxCore_MutexUnlock(inode->mutex);

	if(inode->fd > 0)
		inode->ops->close(inode->fd);
	if(force)
		GxCore_FileDelete(inode->filename);
	GxCore_MutexDelete(inode->mutex);
	gxlist_del(&inode->head);
	av_free(inode->filename);
	av_free(inode);
}

static void recinode_close(struct record_inode *inode)
{
	if(inode && inode->ref == 0) {
		recinode_delete(inode, 0);
	}
}

static struct recinode_ops *recinode_getops(const char* filename)
{
	if(filename[0] == '/') {
		return &recinodeops_file;
	}

	return &recinodeops_mem;
}

struct record_inode *recinode_open(const char *filename, int mode)
{
	struct record_inode* inode = NULL;
	struct gxlist_head* pos;
	struct gxlist_head *n;

	gxlist_for_each_safe(pos, n, &record_inode_list) {
		inode = gxlist_entry(pos, struct record_inode, head);
		if(strcmp(inode->filename, filename) == 0) {
			return inode;
		}
	}

	inode = av_mallocz(sizeof(struct record_inode));
	if (inode == NULL) {
		return NULL;
	}

	inode->ops = recinode_getops(filename);
	inode->fd = inode->ops->open(filename, mode, 0666);
	if (inode->fd < 0) {
		av_free(inode);
		return NULL;
	}

	inode->mode = mode;
//	inode->size = inode->ops->lseek(inode->fd, 0, SEEK_END);
	if (GxCore_FileExists(filename) == GXCORE_FILE_EXIST) {
		GxFileInfo info;
		GxCore_GetFileInfo(filename, &info);
		inode->size = info.size_by_bytes;
	}

	if (inode->size < 0)
		inode->error = 1;
	else
		inode->error = 0;
	inode->filename = av_strdup(filename);

	GX_INIT_LIST_HEAD(&inode->file);
	GxCore_MutexCreate(&(inode->mutex));
	gxlist_add_tail(&inode->head, &record_inode_list);

	return inode;
}

static void recinode_add_file(struct record_inode *inode, struct record_file *rf)
{
	GxCore_MutexLock(inode->mutex);
	rf->inode = inode;
	rf->inode->ref++;
	gxlist_add_tail(&rf->head, &inode->file);
	GxCore_MutexUnlock(inode->mutex);
}

static void recinode_del_file(struct record_inode *inode, struct record_file *file)
{
	struct record_file *rf = NULL;
	struct gxlist_head *pos;
	struct gxlist_head *n;

	if(inode == NULL || file == NULL)
		return;

	GxCore_MutexLock(inode->mutex);
	gxlist_for_each_safe(pos, n, &inode->file) {
		rf = gxlist_entry(pos, struct record_file, head);
		if(rf == file) {
			inode->ref--;
			gxlist_del(&rf->head);
			break;
		}
	}
	GxCore_MutexUnlock(inode->mutex);
}

struct record_file *recfile_open(const char* filename, int mode)
{
	struct record_inode *inode;
	struct record_file *rf;

	if(record_mutex == -1) {
		GxCore_MutexCreate(&record_mutex);
		GX_INIT_LIST_HEAD(&record_inode_list);
	}

	rf = (struct record_file*)av_mallocz(sizeof(struct record_file));
	if (rf == NULL) {
		return NULL;
	}

	GxCore_MutexLock(record_mutex);

	inode = recinode_open(filename, mode);
	if(inode == NULL) {
		av_free(rf);
		GxCore_MutexUnlock(record_mutex);
		return NULL;
	}
	recinode_add_file(inode, rf);

	GxCore_MutexUnlock(record_mutex);

	return rf;
}

int recfile_close(struct record_file *rf)
{
	GxCore_MutexLock(record_mutex);

	recinode_del_file(rf->inode, rf);
	recinode_close(rf->inode);

	GxCore_MutexUnlock(record_mutex);

	av_free(rf);

	return 0;
}

int recfile_eof(struct record_file *rf)
{
	if (rf->inode && rf->inode->ref == 1 && rf->offset >= rf->inode->size)
		return 1;
	else
		return 0;
}


static ssize_t recinode_pread(struct record_inode *inode, void* buf, size_t size, off_t* offset) {
	off_t o = 0;

	if((o = inode->ops->lseek(inode->fd, *offset, SEEK_SET)) < 0) {
		inode->error = 1;
		return -1;
	}

	*offset = o;

	return inode->ops->read(inode->fd, buf, size);
}

static ssize_t recinode_pwrite(struct record_inode *inode, void* buf, size_t size, off_t* offset) {
	off_t o = 0;

	if((o = inode->ops->lseek(inode->fd, *offset, SEEK_SET)) < 0) {
		inode->error = 1;
		return 0;
	}

	*offset = o;

	return inode->ops->write(inode->fd, buf, size);
}

ssize_t recfile_read(struct record_file *rf, void *buf, size_t size) {
	GxCore_MutexLock(record_mutex);
	if(rf->inode) {
		GxCore_MutexLock(rf->inode->mutex);
		GxCore_MutexUnlock(record_mutex);
		size = recinode_pread(rf->inode, buf, size, &rf->offset);
	}
	else {
		GxCore_MutexUnlock(record_mutex);
		return -1;
	}

	if (size > 0) {
		rf->offset += size;
	}
	GxCore_MutexUnlock(rf->inode->mutex);

	return size;
}

ssize_t recfile_readAt(struct record_file *rf, void *buf, size_t size, off_t offset) {
	GxCore_MutexLock(record_mutex);
	if(rf->inode) {
		GxCore_MutexLock(rf->inode->mutex);
		GxCore_MutexUnlock(record_mutex);
		size = recinode_pread(rf->inode, buf, size, &offset);
	}
	else {
		GxCore_MutexUnlock(record_mutex);
		return -1;
	}

	if (size > 0) {
		rf->offset = offset + size;
	}
	GxCore_MutexUnlock(rf->inode->mutex);

	return size;
}

ssize_t recfile_write(struct record_file *rf, void *buf, size_t size) {
	GxCore_MutexLock(rf->inode->mutex);
	size = recinode_pwrite(rf->inode, buf, size, &rf->offset);

	if (size > 0) {
		rf->offset += size;
		rf->inode->size += size;
	}
	GxCore_MutexUnlock(rf->inode->mutex);

	return size;
}

off_t recfile_getsize(struct record_file *rf) {
	return rf->inode->size;
}

int recfile_fsync(struct record_file *rf) {
	GxCore_MutexLock(rf->inode->mutex);
	fsync(rf->inode->fd);
	GxCore_MutexUnlock(rf->inode->mutex);
	return 0;
}

void recfile_rm(const char* filename) {
	struct record_inode* inode = NULL;
	struct gxlist_head* pos;
	struct gxlist_head *n;

	GxCore_MutexLock(record_mutex);
	gxlist_for_each_safe(pos, n, &record_inode_list) {
		inode = gxlist_entry(pos, struct record_inode, head);
		if(strcmp(inode->filename, filename) == 0 ) {
			recinode_delete(inode, 1);
			GxCore_MutexUnlock(record_mutex);
			return;
		}
	}
	GxCore_MutexUnlock(record_mutex);

	GxCore_FileDelete(filename);
}

off_t recfile_seek(struct record_file *rf, off_t off, int f) {
	switch (f) {
		case SEEK_SET:
			rf->offset = off;
			break;
		case SEEK_CUR:
			rf->offset += off;
			break;
		case SEEK_END:
			rf->offset = recfile_getsize(rf) -off;
			break;
		default:
			break;
	};

	return rf->offset;
}

off_t recfile_getpos(struct record_file *rf)
{
	return rf->offset;
}

int recfile_error(struct record_file *rf)
{
	int error = 0;

	GxCore_MutexLock(rf->inode->mutex);
	error = rf->inode->error;
	GxCore_MutexUnlock(rf->inode->mutex);

	return error;
}
