#include "gx_fileops.h"
#include "gx_system.h"
#include "gx_common.h"
#include "recfs.h"
#include "string.h"

#define MAX_FILE    (8)
#define MAX_POSTFIX (6)
#define MAX_LONG    (2048)
#define ALIGN_SIZE  (64*1024)
#define UNIT_SIZE   (16)
#define ALIGN_UNIT  (188)

#define MAKEVOL     "%s/%04d%s"
#define MAKEURL     "%s%s.dvr"
#define WRITEDVR(m) (m != O_RDONLY)
#define INFOVOL     "%s/info.hdr"

static handle_t volume_mutex = -1;

typedef struct {
	struct record_file *fd;
	off_t  size;
}_volume;

typedef struct {
	int   used;
	struct record_file *fd;
	char  *dir;
	char  postfix[MAX_POSTFIX];
	int   mode;

	int   nowvol;
	int   firstvol;
	int   lastvol;
	int   totlevol;

	int   maxvol;
	int   volsize;
	int   fullstop;

	int   masterfd;
	char* name;

	off_t totlesize;
	off_t turesize;
	off_t nowsize;

	size_t pos_w;
	unsigned char  *buffer_w;
	GxTime time_w;

	int    vol_r; // 当前正在读的分卷文件的 id
	size_t pos_r; // 当前正在读的分卷文件的读指针

	_volume *vol;
	int vol_alloc_count;

	unsigned int newtimestamp;
	unsigned int oldtimestamp;
	unsigned int cnttimestamp;
}VolumeFile;

static VolumeFile volume_file[MAX_FILE];

#if 0
#define VOL_LOCK(k)    gxlogd("[Player:  lock]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__); GxCore_MutexLock(volume_mutex);
#define VOL_UNLOCK(k)  gxlogd("[Player:unlock]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__); GxCore_MutexUnlock(volume_mutex);
#else
#define VOL_LOCK(k)    GxCore_MutexLock(volume_mutex);
#define VOL_UNLOCK(k)  GxCore_MutexUnlock(volume_mutex);
#endif

int GxVolumeFile_Init(void)
{
	if (volume_mutex == -1)
		GxCore_MutexCreate(&volume_mutex);

	return 0;
}

static struct record_file *volume_open_vol_file(int index, int number);
static void volume_clone_refresh(int clone)
{
	int i;
	struct record_file *rf = NULL;
	int master = volume_file[clone].masterfd;

	if (master >= 0)
	{
		volume_file[clone].masterfd = master;
		volume_file[clone].totlesize = volume_file[master].totlesize;
		volume_file[clone].turesize = volume_file[master].turesize;
		volume_file[clone].totlevol = volume_file[master].totlevol;
		volume_file[clone].firstvol = volume_file[master].firstvol;
		volume_file[clone].lastvol = volume_file[master].lastvol;
		volume_file[clone].maxvol = volume_file[master].maxvol;
		volume_file[clone].volsize = volume_file[master].volsize;

		if (volume_file[clone].vol) {
			rf = volume_file[clone].vol[volume_file[clone].nowvol].fd;
			if (volume_file[clone].vol_alloc_count != volume_file[master].vol_alloc_count) {
				av_free(volume_file[clone].vol);

				volume_file[clone].vol_alloc_count = volume_file[master].vol_alloc_count;
				volume_file[clone].vol = av_mallocz(volume_file[clone].vol_alloc_count * sizeof(_volume));
			}
		}else{
			volume_file[clone].vol_alloc_count = volume_file[master].vol_alloc_count;
			volume_file[clone].vol = av_mallocz(volume_file[clone].vol_alloc_count * sizeof(_volume));
		}

		if (volume_file[clone].vol && volume_file[master].vol) {
			for(i = volume_file[clone].firstvol; i <= volume_file[clone].lastvol; i++)
				volume_file[clone].vol[i].size = volume_file[master].vol[i].size;
		}

		if (volume_file[clone].nowvol < volume_file[clone].firstvol)
			volume_open_vol_file(clone, volume_file[clone].firstvol);
		else
			volume_file[clone].vol[volume_file[clone].nowvol].fd = rf;
	}
}

static void volume_delete_dvr(int index)
{
	int i = 0, nents;
	GxDirent* ents = NULL;

	nents = GxCore_GetDir(volume_file[index].dir, &ents, NULL);

	if (nents > 0)
	{
		char *name = av_mallocz(MAX_LONG);
		if (name == NULL)
			return;
		for(i = 0; i < nents; i++)
		{
			snprintf(name, MAX_LONG, MAKEVOL, volume_file[index].dir, i, volume_file[index].postfix);
			GxCore_FileDelete(name);
		}
		av_free(name);
	}

	GxCore_FreeDir(ents, nents);
	GxCore_Rmdir(volume_file[index].dir);
	GxCore_FileDelete((const char*)volume_file[index].name);
}

static void volume_delete_vol(int index)
{
	char *name = av_mallocz(MAX_LONG);
	if (name == NULL)
		return;
	snprintf(name, MAX_LONG, MAKEVOL, volume_file[index].dir, volume_file[index].firstvol, volume_file[index].postfix);

	gxlogf("[Player]: Del %d.ts\n", volume_file[index].firstvol);

	VOL_LOCK();
	volume_file[index].turesize = volume_file[index].turesize - volume_file[index].vol[volume_file[index].firstvol].size;
	volume_file[index].firstvol++;
	volume_file[index].totlevol--;
	recfile_rm(name);
	VOL_UNLOCK();
	av_free(name);
}

static int volume_alloc_file_entry(int mode)
{
	int i;

	VOL_LOCK();

	for(i=0;i<MAX_FILE;i++)
	{
		if (volume_file[i].used == 0)
		{
			memset(&volume_file[i], 0, sizeof(VolumeFile));
			if (WRITEDVR(mode)) {
				volume_file[i].buffer_w = GxCore_PageMalloc(ALIGN_SIZE, 10);
				if (volume_file[i].buffer_w == NULL)
					goto errout;
				GxCore_GetTickTime(&volume_file[i].time_w);
			}
			volume_file[i].masterfd = -1;
			volume_file[i].fd = NULL;
			volume_file[i].mode = mode;
			volume_file[i].used = 1;
			VOL_UNLOCK();
			return i;
		}
	}

errout:
	VOL_UNLOCK();
	return -1;
}

static void volume_free_file_entry(int index)
{
	if (WRITEDVR(volume_file[index].mode))
	{
		if (volume_file[index].totlesize == 0)
			volume_delete_dvr(index);
	}

	if (index < MAX_FILE)
	{
		volume_file[index].used = 0;
		if (volume_file[index].buffer_w)
			GxCore_PageFree(volume_file[index].buffer_w);
		if (volume_file[index].vol)
			av_free(volume_file[index].vol);
		if (volume_file[index].name)
			av_free(volume_file[index].name);
		if (volume_file[index].dir)
			av_free(volume_file[index].dir);
		memset(&volume_file[index], 0, sizeof(VolumeFile));
		volume_file[index].masterfd = -1;
		volume_file[index].fd = NULL;
	}
}

static void volume_getdir_postfix(int index, char* name)
{
	char postbak[MAX_POSTFIX];
	char *dot, *postfixptr = NULL;
	char *namebak = av_mallocz(MAX_LONG);
	if (namebak == NULL)
		return;

	dot = (char*)namebak;
	strncpy(namebak, name, MAX_LONG);

	while(dot)
	{
		postfixptr = dot++;
		dot = strchr(dot,'.');
	}
	*postfixptr = '\0';

	dot = (char*)namebak;
	while(dot)
	{
		postfixptr = dot++;
		dot = strchr(dot,'.');
	}

	strncpy(postbak,postfixptr,MAX_POSTFIX);
	*postfixptr = '\0';

	strncpy(volume_file[index].postfix, postbak, MAX_POSTFIX);
	volume_file[index].dir = namebak;
}

static int volume_get_master(int index)
{
	int ret = -1;
	char *name, *tmp;
	tmp  = av_mallocz(MAX_LONG);
	name = av_mallocz(MAX_LONG);
	if (tmp == NULL || name == NULL) {
		goto out;
	}

	if (index < MAX_FILE)
	{
		int i;
		snprintf(name, MAX_LONG, MAKEURL, volume_file[index].dir, volume_file[index].postfix);

		for(i=0;i<MAX_FILE;i++)
		{
			if (volume_file[i].used == 1 && index != i)
			{
				snprintf(tmp, MAX_LONG, MAKEURL, volume_file[i].dir, volume_file[i].postfix);
				if (!strcmp(tmp, name) && WRITEDVR(volume_file[i].mode)) {
					ret = i;
					goto out;
				}
			}
		}
	}

out:
	if (tmp) av_free(tmp);
	if (name) av_free(name);
	return ret;
}

static void volume_clone_link(int clone, int master)
{
	if (master >= 0)
	{
		volume_file[clone].masterfd = master;
		volume_clone_refresh(clone);
	}
}

static void volume_master_unlink(int master)
{
	int i;

	for(i=0; i<MAX_FILE; i++)
	{
		if (volume_file[i].masterfd == master && volume_file[i].used)
		{
			volume_file[i].masterfd = -1;
		}
	}
}

static void volume_file_flush(int index)
{
	size_t pos_w = volume_file[index].pos_w;
	unsigned char* buffer_w = volume_file[index].buffer_w;
	if (volume_file[index].vol) {
		struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;
		if (fd != NULL)
			recfile_write(fd, buffer_w, pos_w);
		volume_file[index].pos_w = 0;
		GxCore_GetTickTime(&volume_file[index].time_w);
	}
}

static struct record_file *volume_open_vol_file(int index, int number)
{
	if (number >= volume_file[index].vol_alloc_count) {
		int temp = (number/UNIT_SIZE + 1) * UNIT_SIZE;
		volume_file[index].vol = av_realloc(volume_file[index].vol, temp * sizeof(_volume));
		if (volume_file[index].vol == NULL) {
			volume_file[index].vol_alloc_count = 0;
			gxloge("[VOLUME FILE] OOM !!!\n");
			return NULL;
		}
		memset(&volume_file[index].vol[volume_file[index].vol_alloc_count], 0,
			(temp - volume_file[index].vol_alloc_count) * sizeof(_volume));
		volume_file[index].vol_alloc_count = temp;
	}

	if (volume_file[index].vol[volume_file[index].nowvol].fd != NULL) {
		if (number == volume_file[index].nowvol)
			return volume_file[index].vol[volume_file[index].nowvol].fd;
		recfile_close(volume_file[index].vol[volume_file[index].nowvol].fd);
		volume_file[index].vol[volume_file[index].nowvol].fd = NULL;
	}

	if (volume_file[index].vol[number].fd == NULL) {
		char *name = av_mallocz(MAX_LONG);
		if (name == NULL)
			return NULL;
		snprintf(name, MAX_LONG, MAKEVOL, volume_file[index].dir, number, volume_file[index].postfix);
		volume_file[index].vol[number].fd = recfile_open(name, volume_file[index].mode);
		if (volume_file[index].vol[number].fd == NULL) {
			gxloge("[VOLUME FILE] Open volume [%d-%d] error ! fd = %p\n",
					volume_file[index].totlevol, number, volume_file[index].vol[number].fd);
			av_free(name);
			return NULL;
		}
		av_free(name);

		if (WRITEDVR(volume_file[index].mode)) {
			volume_file[index].totlevol++;
			volume_file[index].nowsize = 0;
			volume_file[index].lastvol = number;
		}
	}

	volume_file[index].nowvol = number;

	return volume_file[index].vol[number].fd;
}

#define GET_PREFIX_LEN(prefix_len, name, final_prefix) \
	do{\
		char *p1 = NULL, *p2 = NULL;\
		if (name && final_prefix) {\
			p1 = strstr(name, final_prefix);\
			if (p1) {\
				p2 = p1-1;\
				while(p2 >= name) {\
					if (*p2 == '.')\
						break;\
					else\
						p2 --;\
				}\
				if (p2 == name)\
					prefix_len = 0;\
				else\
					prefix_len = p1-p2-1;\
			}\
			else\
				prefix_len = 0;\
		}\
	}while(0)

static int volume_open_dvr_file(int index, char* name)
{
	int prefix_len = 0;

	GET_PREFIX_LEN(prefix_len, name, ".dvr");
	if (!prefix_len)
		return -1;

	if (WRITEDVR(volume_file[index].mode))
	{
		if (GxCore_FileExists(volume_file[index].dir) != GXCORE_FILE_EXIST)
		{
			if (GxCore_Mkdir(volume_file[index].dir) != GXCORE_SUCCESS)
				return -1;
		}

		volume_file[index].fd = recfile_open(name, volume_file[index].mode);
		if (volume_file[index].fd == NULL)
			return -1;
	}
	else
	{
		int master;
		master = volume_get_master(index);
		if (master < 0)
		{
			int i, entries;
			GxDirent *ents = NULL;
			if (GxCore_FileExists(volume_file[index].dir) != GXCORE_FILE_EXIST)
				return -1;
			//filter "ts" or "ts;m2ts", not ".ts"
			entries = GxCore_GetDir(volume_file[index].dir, &ents, volume_file[index].postfix + 1);
			if (entries) {
				GxCore_SortDir(ents, entries, NULL);
				for(i = 0; i < entries; i++) {
					if (strlen(ents[i].fname) == prefix_len+5) {
						char fname[8];
						strncpy(fname, ents[i].fname, strlen(ents[i].fname));
						fname[4] = '\0';
						volume_file[index].lastvol = atoi(fname);
					}
				}
				GxCore_FreeDir(ents, entries);

				if (volume_file[index].lastvol >= 0) {
					for(i = volume_file[index].lastvol; i >= 0; i--) {
						struct record_file *fd = volume_open_vol_file(index, i);
						if (fd == NULL) {
							unsigned int idx = volume_file[index].firstvol;
							volume_file[index].totlesize += volume_file[index].vol[idx].size;
							continue;
						}
						volume_file[index].totlevol ++;
						volume_file[index].vol[i].fd = fd;
						volume_file[index].vol[i].size = recfile_getsize(fd);
						volume_file[index].totlesize += recfile_getsize(fd);
						volume_file[index].turesize += recfile_getsize(fd);
						volume_file[index].firstvol = i;
					}
				}
				else{
					return -1;
				}
			}
		}
		else
		{
			volume_clone_link(index, master);
		}
	}

	return index;
}

static int volume_close(int index)
{
	int i;

	VOL_LOCK();

	if (WRITEDVR(volume_file[index].mode)) {
		volume_file_flush(index);
		volume_master_unlink(index);
		if (volume_file[index].fd != NULL) {
			recfile_fsync(volume_file[index].fd);
			recfile_close(volume_file[index].fd);
		}
	}

	for(i = volume_file[index].firstvol; i <= volume_file[index].lastvol; i++) {
		if (volume_file[index].vol && volume_file[index].vol[i].fd != NULL)
			recfile_close(volume_file[index].vol[i].fd);
	}

	volume_free_file_entry(index);

	VOL_UNLOCK();

	return 0;
}

static int volume_open(char *name, mode_t mode)
{
	int index;
	int ret;

	index = volume_alloc_file_entry(mode);
	if (index < 0)
		return -1;

	volume_getdir_postfix(index, name);

	ret = volume_open_dvr_file(index,name);
	if (ret < 0)
		goto errorout;

	if (volume_open_vol_file(index, volume_file[index].firstvol) == NULL)
		goto errorout;

	volume_file[index].name = av_strdup(name);

	GxPlayer_SystemGet(PSYS_PVR_VOLUME_FULLSTOP, &volume_file[index].fullstop);

	return index;

errorout:
	volume_close(index);
	return -1;
}

static off_t volume_seek_ext(int index, off_t offset, int whence, off_t *outpos)
{
	int i=0;
	off_t ret = -1;
	off_t pos = 0, del_size = 0;

	VOL_LOCK();

	volume_clone_refresh(index);

	if (offset >= volume_file[index].totlesize)
		goto out;

	del_size = volume_file[index].totlesize - volume_file[index].turesize;
	if (offset >= del_size) {
		offset -= del_size;
	}else{
		offset = offset%ALIGN_UNIT;
	}
	pos = offset;
	for(i = volume_file[index].firstvol; i <= volume_file[index].lastvol; i++) {
		pos = offset;
		offset -= volume_file[index].vol[i].size;
		if (offset < 0)
			break;
	}

	if (volume_open_vol_file(index, i) == NULL)
		goto out;

	ret = recfile_seek(volume_file[index].vol[i].fd, pos, SEEK_SET);
	*outpos = volume_file[index].vol[volume_file[index].nowvol].fd->offset + del_size;
	for(i = volume_file[index].firstvol; i < volume_file[index].nowvol; i++) {
		*outpos += volume_file[index].vol[i].size;
	}


	if (1) {
		struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;
		volume_file[index].vol_r = volume_file[index].nowvol;
		volume_file[index].pos_r = fd->offset;
	}

out:
	VOL_UNLOCK();
	return ret;
}


static off_t volume_seek(int index, off_t offset, int whence)
{
	int i=0;
	off_t ret = -1;
	off_t pos=offset;

	VOL_LOCK();

	volume_clone_refresh(index);

	if (offset >= volume_file[index].totlesize)
		goto out;

	for(i = volume_file[index].firstvol; i <= volume_file[index].lastvol; i++) {
		pos = offset;
		offset -= volume_file[index].vol[i].size;
		if (offset < 0)
			break;
	}

	if (volume_open_vol_file(index, i) == NULL)
		goto out;

	ret = recfile_seek(volume_file[index].vol[i].fd, pos, SEEK_SET);

	if (1) {
		struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;
		volume_file[index].vol_r = volume_file[index].nowvol;
		volume_file[index].pos_r = fd->offset;
	}
out:
	VOL_UNLOCK();
	return ret;
}

static int volume_read_l(int index, uint8_t *buff, int len, int timeoutms)
{
	int pos = 0;
	int rlen, costms;
	GxTime start, current;
	struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;

	while (pos < len) {
		rlen = recfile_read(fd, buff + pos, len - pos);
		if (rlen > 0)
			pos += rlen;
		else if (rlen == 0) {
			if (recfile_eof(fd))
				goto end;

			if (timeoutms < 0) {
				GxCore_ThreadDelay(10);
				continue;
			}

			costms = 0;
			GxCore_GetTickTime(&start);
			while(costms<timeoutms && pos<len) {
				GxCore_ThreadDelay(10);
				rlen = recfile_read(fd, buff + pos, len - pos);
				if (rlen > 0)
					pos += rlen;

				GxCore_GetTickTime(&current);
				costms = TIME_MINUS(current, start);
			}
			gxlogf("--try-----ret = %d, time = %d\n", pos, costms);
			break;
		} else {
			pos = (pos > 0) ? pos : -1;
			break;
		}
	}

end:
	volume_file[index].vol_r = volume_file[index].nowvol;
	volume_file[index].pos_r = fd->offset;
	return pos;
}

static int volume_read(int index, uint8_t *buffer, size_t max_len, int timeoutms)
{
	int len=0;
	struct record_file *fd;

	VOL_LOCK();
	volume_clone_refresh(index);
	fd = volume_open_vol_file(index, volume_file[index].nowvol);
	VOL_UNLOCK();
	if (fd == NULL)
		return 0;

	len = volume_read_l(index, buffer, max_len, timeoutms);

	if (len < max_len) {
		if (volume_file[index].nowvol < volume_file[index].lastvol) {
			VOL_LOCK();
			fd = volume_open_vol_file(index, volume_file[index].nowvol+1);
			VOL_UNLOCK();
			if (fd != NULL)
				len += volume_read_l(index, buffer+len, max_len-len, timeoutms);
		}
	}

	return len;
}

static int volume_read_ext(int index, uint8_t *buffer, size_t max_len, int timeoutms, off_t *outpos)
{
	int len=0;
	off_t del_size = 0, i = 0;
	struct record_file *fd;

	VOL_LOCK();
	volume_clone_refresh(index);
	fd = volume_open_vol_file(index, volume_file[index].nowvol);
	if (volume_file[index].vol_r != volume_file[index].nowvol) {
		off_t pos = volume_file[index].pos_r % ALIGN_UNIT;
		recfile_seek(fd, pos, SEEK_SET);
	}
	VOL_UNLOCK();

	if (fd == NULL)
		return len;

	len = volume_read_l(index, buffer, max_len, timeoutms);
	if (len < max_len) {
		if (volume_file[index].nowvol < volume_file[index].lastvol) {
			struct record_file *rf;
			int read_len = 0;
			VOL_LOCK();
			rf = volume_open_vol_file(index, volume_file[index].nowvol+1);
			VOL_UNLOCK();
			if (rf != NULL) {
				read_len = volume_read_l(index, buffer+len, max_len-len, timeoutms);
				if (read_len > 0)
					len += read_len;
			}else{
				gxloge("[VOLUME FILE] Open volume  error ! rf = NULL\n");
				return len;
			}
		}
	}

	VOL_LOCK();
	if (volume_file[index].vol && volume_file[index].vol[volume_file[index].nowvol].fd) {
		del_size = volume_file[index].totlesize - volume_file[index].turesize;
		*outpos = volume_file[index].vol[volume_file[index].nowvol].fd->offset + del_size;
		for(i = volume_file[index].firstvol; i < volume_file[index].nowvol; i++) {
			*outpos += volume_file[index].vol[i].size;
		}
	}
	VOL_UNLOCK();

	return len;
}

static int volume_flush_l(int index)
{
	size_t len;
	unsigned char*buffer_w = volume_file[index].buffer_w;
	struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;
	size_t pos_w = volume_file[index].pos_w;

	len = recfile_write(fd, buffer_w, pos_w);
	volume_file[index].pos_w = 0;
	volume_file[index].vol[volume_file[index].nowvol].size += len;
	GxCore_GetTickTime(&volume_file[index].time_w);
	return 0;
}

static int volume_write_l(int index, uint8_t *buffer, size_t max_len)
{
	size_t size = max_len, wsize = 0, len;
	size_t pos_w = volume_file[index].pos_w;
	unsigned char* wptr, *buffer_w = volume_file[index].buffer_w;
	struct record_file *fd = volume_file[index].vol[volume_file[index].nowvol].fd;

	while(size) {
		if (size < (ALIGN_SIZE- pos_w)) {
			memcpy(buffer_w + pos_w, buffer + max_len - size, size);
			pos_w += size;
			break;
		} else {
			if (pos_w != 0) {
				memcpy(buffer_w + pos_w, buffer + max_len - size, ALIGN_SIZE - pos_w);
				wptr  = buffer_w;
				size -= (ALIGN_SIZE - pos_w);
				pos_w = 0;
			}
			else {
				wptr = buffer + max_len - size;
				size -= ALIGN_SIZE;
			}

			len = recfile_write(fd, wptr, ALIGN_SIZE);
			if (len != ALIGN_SIZE) {
				gxloge("[VOLUME FILE] write ret = %d\n", len);
				return 0;
			}
			GxCore_GetTickTime(&volume_file[index].time_w);
			wsize += len;
		}
	}

	volume_file[index].pos_w = pos_w;
	volume_file[index].vol[volume_file[index].nowvol].size += wsize;
	volume_file[index].totlesize += max_len;
	volume_file[index].turesize += max_len;
	volume_file[index].nowsize += max_len;

	return max_len;
}

static int volume_sync_data(int index)
{
	GxTime current;
	int costms = 0;
#define VOLUME_SYNC_TIMEMS (1000)

	GxCore_GetTickTime(&current);
	costms = TIME_MINUS(current, volume_file[index].time_w);
	if (costms > VOLUME_SYNC_TIMEMS) {
		volume_flush_l(index);
	}

	return 0;
}

static void volume_write_timestamp(int index)
{
	unsigned int align = 4*ALIGN_UNIT;
	PlayerRecIndexEntry entry;

	if (volume_file[index].newtimestamp - volume_file[index].oldtimestamp >= 200) {
		if (volume_file[index].oldtimestamp == 0) {
			entry.timestamp = 0;
			entry.pos = 0;
			recfile_write(volume_file[index].fd, &entry, sizeof(entry));
		}
		entry.timestamp = volume_file[index].newtimestamp;
		entry.pos = volume_file[index].totlesize/align*align;
		recfile_write(volume_file[index].fd, &entry, sizeof(entry));
		volume_file[index].oldtimestamp = volume_file[index].newtimestamp;
		if (volume_file[index].cnttimestamp++ >= 15) {
			recfile_fsync(volume_file[index].fd);
			volume_file[index].cnttimestamp = 0;
		}
	}
}

static int volume_need_newvol(int index, size_t max_len)
{
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_SIZEMB, &volume_file[index].volsize);

	if (volume_file[index].nowsize + max_len >= ((off_t)volume_file[index].volsize << 20))
		return 1;
	return 0;
}

static int volume_create_newvol(int index)
{
	GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXNUM, &volume_file[index].maxvol);

	if (volume_file[index].maxvol > 0 &&
			volume_file[index].fullstop == 1 &&
			volume_file[index].totlevol >= volume_file[index].maxvol) {
		return -1;
	}
	volume_flush_l(index);
	if (volume_open_vol_file(index, volume_file[index].nowvol+1) == NULL) {
		return -1;
	}
	gxlogf("[Player]: New %d.ts\n", volume_file[index].nowvol);
	return 0;
}

static int volume_write(int index, uint8_t *buffer, size_t max_len)
{
	int len = 0;

	VOL_LOCK();
	if (volume_need_newvol(index, max_len)) {
		if (volume_create_newvol(index) < 0) {
			VOL_UNLOCK();
			return -1;
		}
	}
	VOL_UNLOCK();

	if (volume_file[index].maxvol > 1) {
		if (volume_file[index].totlevol > volume_file[index].maxvol) {
			volume_delete_vol(index);
		}
	}

	len = volume_write_l(index, buffer, max_len);

	volume_write_timestamp(index);

	return len;
}

static off_t volume_getsize(int index)
{
	off_t size;

	VOL_LOCK();

	volume_clone_refresh(index);

	size = volume_file[index].totlesize;

	VOL_UNLOCK();

	return size;
}

static struct vol_info volume_getvolinfo(int index)
{
	struct vol_info info;

	VOL_LOCK();
	volume_clone_refresh(index);
	info.size = volume_file[index].totlesize;
	info.truesize = volume_file[index].turesize;
	VOL_UNLOCK();

	return info;
}

static int volume_geterrorcode(int index)
{
	int error = 0;

	VOL_LOCK();
	if (NULL != volume_file[index].vol[volume_file[index].nowvol].fd)
		error = recfile_error(volume_file[index].vol[volume_file[index].nowvol].fd);
	VOL_UNLOCK();

	return error;
}

static void volume_settimestamp(int index, unsigned int timems)
{
	volume_file[index].newtimestamp = timems;
}

static int volume_getnewvolume(int index, struct vol_new *vol)
{
	VOL_LOCK();

	vol->is_newvol = vol->newvol_ok = 0;

	if (volume_file[index].nowsize == 0) {
		vol->is_newvol = 1;
		vol->newvol_ok = 1;
		VOL_UNLOCK();
		return 0;
	}

	if (volume_need_newvol(index, vol->size)) {
		vol->is_newvol = 1;
		if (volume_create_newvol(index) < 0) {
			VOL_UNLOCK();
			return -1;
		}
		vol->newvol_ok = 1;
	}

	VOL_UNLOCK();

	return 0;
}

static int volume_read_hdr(int index, uint8_t *buffer, size_t max_len, int timeoutms)
{
	size_t size = 0;
	char *name = av_mallocz(MAX_LONG);
	struct record_file *fd;

	VOL_LOCK();
	snprintf(name, MAX_LONG, INFOVOL, volume_file[index].dir);
	fd = recfile_open(name, volume_file[index].mode);
	if (fd == NULL) {
		gxloge("[VOLUME FILE]: %s, not exist\n", name);
		VOL_UNLOCK();
		av_free(name);
		return -1;
	}

	size = recfile_read(fd, buffer, max_len);
	recfile_close(fd);
	VOL_UNLOCK();

	av_free(name);

	return size;
}

static int volume_write_hdr(int index, uint8_t *buffer, size_t max_len)
{
	size_t size = 0;
	char *name = av_mallocz(MAX_LONG);
	struct record_file *fd;

	VOL_LOCK();
	snprintf(name, MAX_LONG, INFOVOL, volume_file[index].dir);
	fd = recfile_open(name, volume_file[index].mode);
	if (fd == NULL) {
		gxloge("[VOLUME FILE]: %s, not exist\n", name);
		VOL_UNLOCK();
		av_free(name);
		return -1;
	}

	size = recfile_write(fd, buffer, max_len);
	recfile_close(fd);
	VOL_UNLOCK();

	av_free(name);

	return size;
}

static int volume_probe_hdr(int index)
{
	size_t size = 0;
	char *name = av_mallocz(MAX_LONG);
	struct record_file *fd;

	VOL_LOCK();
	snprintf(name, MAX_LONG, INFOVOL, volume_file[index].dir);
	fd = recfile_open(name, volume_file[index].mode);
	if (fd == NULL) {
		VOL_UNLOCK();
		av_free(name);
		return -1;
	}

	size = recfile_getsize(fd);
	recfile_close(fd);
	VOL_UNLOCK();

	av_free(name);

	return size;
}

GxFileOps gx_fileops_volume = {
	.open       = volume_open,
	.read       = volume_read,
	.read_ext   = volume_read_ext,
	.write      = volume_write,
	.seek       = volume_seek,
	.seek_ext   = volume_seek_ext,
	.close      = volume_close,
	.getsize    = volume_getsize,
	.getvolinfo = volume_getvolinfo,
	.geterrorcode = volume_geterrorcode,
	.settimestamp = volume_settimestamp,
	.getnewvolume = volume_getnewvolume,
	.sync_data    = volume_sync_data,
	.read_hdr     = volume_read_hdr,
	.write_hdr    = volume_write_hdr,
	.probe_hdr    = volume_probe_hdr,
};

