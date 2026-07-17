#include "file_cache.h"
#include "recfs.h"

#define MAX_BLK (4096)
#define MAX_LONG (1024)
#define MAKEVOL  "%s%08d.ts"

static int fcache_mkdir(const char* url)
{
	char* path = av_strdup(url);
	char* now = path+1;

	while (*now != '\0') {
		now = strchr(now,'/');
		if (now) {
			*now = '\0';
			if(GxCore_FileExists(path) != GXCORE_FILE_EXIST) {
				if(GxCore_Mkdir(path) != GXCORE_SUCCESS) {
					av_free(path);
					return -1;
				}
			}
			*now = '/';
			now++;
		}
		else {
			if(GxCore_FileExists(path) != GXCORE_FILE_EXIST) {
				if(GxCore_Mkdir(path) != GXCORE_SUCCESS) {
					av_free(path);
					return -1;
				}
			}
			av_free(path);
			return 0;
		}
	}

	return 0;
}

static struct record_file* fcache_open_file(struct file_cache* cache, int number, int mode)
{
	char name[MAX_LONG];
	snprintf(name, MAX_LONG, MAKEVOL, cache->dir, number);
	return recfile_open(name, mode);
}

static void fcache_delete_file(struct file_cache* cache, int number)
{
	char name[MAX_LONG];
	snprintf(name, MAX_LONG, MAKEVOL, cache->dir, number);
	GxCore_FileDelete(name);
}

static void fcache_delete_dir(struct file_cache* cache)
{
	int i = 0, nents;
	GxDirent* ents = NULL;

	nents = GxCore_GetDir(cache->dir, &ents, NULL);

	if(nents > 0) {
		for(i = 0; i < nents; i++) {
			fcache_delete_file(cache, i);
		}
		GxCore_FreeDir(ents, nents);
	}

	GxCore_Rmdir(cache->dir);
}

int file_cache_init(struct file_cache* cache, const char* dir, int cachesize)
{
	int ret;
	int len = strlen(dir);
	memset(cache, 0, sizeof(struct file_cache));

	if (fcache_mkdir(dir) != 0)
		return -1;

	if (dir[len - 1] == '/')
		cache->dir = av_strdup(dir);
	else {
		cache->dir = av_mallocz(len + 2);
		memcpy(cache->dir, dir, len);
		cache->dir[len] = '/';
		cache->dir[len + 1] = '\0';
	}

	cache->maxsize = cachesize/2;
	cache->rf_w = fcache_open_file(cache, cache->head, O_RDWR|O_CREAT|O_TRUNC);
	cache->rf_r = fcache_open_file(cache, cache->tail, O_RDONLY);
	cache->fblk = av_mallocz(MAX_BLK * sizeof(struct fcblk));
	ret = GxCore_MutexCreate(&cache->mutex);

	if (cache->dir == NULL || cache->rf_w == NULL || cache->rf_r == NULL || cache->fblk == NULL || ret != 0) {
		file_cache_uninit(cache);
		return -1;
	}

	return 0;
}

#if 1
size_t file_cache_read(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t* pts)
{
	struct record_file* rf = cache->rf_r;

	if (cache->fblk[cache->ridx].size > 0) {
		size_t rsize = cache->fblk[cache->ridx].size;
		*pts = cache->fblk[cache->ridx].pts;

		if (cache->fblk[cache->ridx].rollback == 1) {
			//gxlogd("file cache rrollback, rpos = %d\n", cache->rpos);
			GxCore_MutexLock(cache->mutex);
			cache->rpos = 0;
			GxCore_MutexUnlock(cache->mutex);
			if (rf)
				recfile_close(rf);
			rf = cache->rf_r = fcache_open_file(cache, (++cache->tail)%2, O_RDONLY);
		}

		if (rf == NULL) {
			gxlogd("%s, rf = NULL, index=%d\n", __func__, cache->head%2);
			return 0;
		}
		rsize = recfile_read(rf, buffer, rsize);

		GxCore_MutexLock(cache->mutex);
		cache->rpos += rsize;
		cache->fblk[cache->ridx].size = 0;
		cache->fblk[cache->ridx].rollback = 0;
		cache->ridx++;
		cache->ridx %= MAX_BLK;
		GxCore_MutexUnlock(cache->mutex);

		return rsize;
	}
	else {
		gxlogd("file cache empty ####, ridx=%d, widx=%d\n", cache->ridx, cache->widx);
	}

	return 0;
}

#define CACHE_ALGIN_SIZE (64*1024)
static int __cache_write(struct file_cache* cache, struct record_file* rf, unsigned char *buf, unsigned int size)
{
	unsigned int wsize = 0;

	if (!rf)
		return -1;

	if (cache->wbuf == NULL) {
		cache->wbuf = av_malloc(CACHE_ALGIN_SIZE);
		cache->wptr = 0;
	}

	if ((size > 0) && (buf != NULL)) {
		while (wsize < size) {
			unsigned int csize = (CACHE_ALGIN_SIZE - cache->wptr);

			csize = ((size - wsize) > csize) ? csize : (size - wsize);
			memcpy(cache->wbuf + cache->wptr, buf + wsize, csize);
			wsize     += csize;
			cache->wptr += csize;
			if (cache->wptr >= CACHE_ALGIN_SIZE) {
				recfile_write(rf, cache->wbuf, cache->wptr);
				cache->wptr = 0;
			}
		}
	}

	if (buf == NULL) {
		 recfile_write(rf, cache->wbuf, cache->wptr);
		av_free(cache->wbuf);
		cache->wbuf = NULL;
		cache->wptr = 0;
	}

	return size;
}

size_t file_cache_write(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t pts)
{
	size_t wsize = 0;
	struct record_file* rf = cache->rf_w;

	if(cache->wpos >= cache->maxsize) {
		//gxlogd("file cache wrollback, wpos = %d\n", cache->wpos);
		GxCore_MutexLock(cache->mutex);
		cache->wpos = 0;
		cache->fblk[cache->widx].rollback = 1;
		GxCore_MutexUnlock(cache->mutex);
		__cache_write(cache, rf, NULL, 0);
		recfile_close(rf);
		rf = cache->rf_w = fcache_open_file(cache, (++cache->head)%2, O_RDWR|O_CREAT|O_TRUNC);
	}

	if (cache->head == cache->tail && cache->wpos < cache->rpos && cache->wpos + size >= cache->rpos) {
		gxlogd("file cache oom, rpos=%d, wpos=%d\n", cache->rpos, cache->wpos);
		return 0;
	}

	if (cache->widx < cache->ridx && cache->widx + 1 >= cache->ridx) {
		gxlogd("file cache oom, ridx=%d, widx=%d\n", cache->ridx, cache->widx);
		return 0;
	}

	//gxlogd("file cache, ridx=%d, widx=%d, rpos=%d, wpos=%d\n", cache->ridx, cache->widx, cache->rpos, cache->wpos);
	if (rf == NULL) {
		gxlogd("%s, rf = NULL, index=%d\n", __func__, cache->head%2);
		return 0;
	}
	wsize = __cache_write(cache, rf, buffer, size);

	GxCore_MutexLock(cache->mutex);
	cache->fblk[cache->widx].pts = pts;
	cache->fblk[cache->widx].size = size;
	cache->widx++;
	cache->widx %= MAX_BLK;
	cache->wpos += wsize;
	GxCore_MutexUnlock(cache->mutex);

	return wsize;
}

#else
size_t file_cache_read(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t* pts)
{
	struct record_file* rf;

	if(cache->head == cache->tail) {
		gxlogd("file cache empty ####, nowsize = %d\n", cache->nowsize);
		return 0;
	}

	rf = fcache_open_file(cache, cache->tail, O_RDONLY);
	if (rf != NULL) {
		off_t fsize = recfile_getsize(rf);
		if (fsize > 4) {
			int val = recfile_read(rf, buffer, fsize-4);
			recfile_read(rf, pts, 4);
			recfile_close(rf);
			//fcache_delete_file(cache, cache->tail);
			//cache->nowsize -= fsize;
			cache->tail++;
			return val;
		}
		else {
			recfile_close(rf);
		}
	}

	return 0;
}

size_t file_cache_write(struct file_cache* cache, unsigned char* buffer, size_t size, uint32_t pts)
{
	struct record_file* rf;

	if(cache->nowsize >= cache->maxsize) {
		off_t fsize;
		rf = fcache_open_file(cache, 0, O_RDWR | O_CREAT | O_TRUNC);
		if (rf != NULL) {
			fsize = recfile_getsize(rf);
			recfile_close(rf);
			//fcache_delete_file(cache, 0);
			cache->tail = 1;
			cache->head = 0;
			cache->nowsize -= fsize;
			gxlogd("file cache oom, maxsize=%d, nowsize=%d\n", cache->maxsize, cache->nowsize);
		}
	}

	rf = fcache_open_file(cache, cache->head, O_RDWR | O_CREAT | O_TRUNC);
	if (rf != NULL) {
		int val = recfile_write(rf, buffer, size);
		recfile_write(rf, &pts, 4);
		recfile_close(rf);
		cache->nowsize += (size+4);
		cache->head++;
		return val;
	}

	return 0;
}

#endif

int file_cache_uninit(struct file_cache* cache)
{
	if (cache->rf_r)
		recfile_close(cache->rf_r);
	if (cache->rf_w) {
		__cache_write(cache, cache->rf_w, NULL, 0);
		recfile_close(cache->rf_w);
	}

	fcache_delete_dir(cache);

	if (cache->dir)
		av_free(cache->dir);
	if (cache->fblk)
		av_free(cache->fblk);
	if (cache->mutex)
		GxCore_MutexDelete(cache->mutex);
	return 0;
}

