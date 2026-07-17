#include "gx_common.h"
#include "gx_system.h"
#include "gx_mem.h"

#ifndef MEMORY_DEBUG

#define AV_USER_MAX (1)
#define AV_HOLE_MAX (3)
#define AV_HOLE_SIZE_0   (0x200000)
#define AV_HOLE_SIZE_1   (0x500000)
#define AV_HOLE_SIZE_2   (0x700000)
#define AV_SMALLMEM_RESV (0x10000)
#define AV_SMALLMEM_SIZE (1024)

#ifdef AV_MEMORY_DEBUG
typedef struct {
	const char   *func;
	unsigned int line;
	void         *ptr;
	unsigned int size;
	unsigned int vaild;
} __debug_list;

typedef struct {
	__debug_list *list;
	int  max;
} __debug_ptr;
#endif

static struct {
	struct {
		size_t maxsiz;
		size_t maxuse;
		size_t nowuse;
		size_t max_smallmem_use;
		size_t now_smallmem_use;
#ifdef AV_MEMORY_DEBUG
		__debug_ptr dp;
#endif
	} sys;

	struct {
		handle_t h;
		unsigned int start;
		unsigned int end;
		size_t maxsiz;
		size_t maxuse;
		size_t nowuse;
#ifdef AV_MEMORY_DEBUG
		__debug_ptr dp;
#endif
	} user[AV_USER_MAX], hole[AV_HOLE_MAX];

	handle_t mutex;
} _av_heap;


#define _av_heap_lock()    GxCore_MutexLock(_av_heap.mutex)
#define _av_heap_unlock()  GxCore_MutexUnlock(_av_heap.mutex)

static int av_heap_inited = 0;

#ifdef AV_MEMORY_DEBUG
static __debug_list *__empty_list(__debug_ptr *dp)
{
	unsigned int i = 0;

	for (i = 0; i < dp->max; i++) {
		if (!dp->list[i].vaild)
			return &(dp->list[i]);
	}

	return NULL;
}

static __debug_list *__ptr_list(__debug_ptr *dp, void *ptr)
{
	unsigned int i = 0;

	for (i = 0; i < dp->max; i++) {
		if (dp->list[i].ptr == ptr)
			return &(dp->list[i]);
	}

	return NULL;
}

static void __printf_list_no_free(__debug_ptr *dp)
{
	unsigned int i = 0;

	gxlogi_raw("[not free]\n");

	for (i = 0; i < dp->max; i++) {
		if (dp->list[i].vaild) {
			__debug_list *list = &dp->list[i];
			gxloge("     %-30s %-8d %-8p 0x%-8x\n",
					list->func, list->line, list->ptr, list->size);
		}
	}
}

static void __init_dp_list(__debug_ptr *dp)
{
	dp->max  = 0;
	dp->list = NULL;
}

static void __destroy_dp_list(__debug_ptr *dp)
{
	GxCore_Free(dp->list);
	dp->max  = 0;
	dp->list = NULL;
}

static void __add_dp_list(__debug_ptr *dp, const char *func, int line, void *ptr, unsigned int size)
{
	__debug_list  *list = __empty_list(dp);
	unsigned char *check_ptr = &(((unsigned char *)ptr)[size - sizeof(unsigned int)]);

	if (NULL == list) {
		unsigned int i = 0;
#define __MAX_LIST__ (256)
		dp->list = GxCore_Realloc(dp->list, (sizeof(__debug_list) * (dp->max + __MAX_LIST__)));
		for (i = 0; i < __MAX_LIST__; i++)
			dp->list[i + dp->max].vaild = 0;
		list = &dp->list[dp->max];
		dp->max += __MAX_LIST__;
	}

	check_ptr[0] = 0xde;
	check_ptr[1] = 0xad;
	check_ptr[2] = 0xbe;
	check_ptr[3] = 0xef;
	list->vaild = 1;
	list->ptr   = ptr;
	list->func  = func;
	list->line  = line;
	list->size  = size;
}

static void __del_dp_list(__debug_ptr *dp, const char *func, int line, void *ptr, unsigned int size)
{
	__debug_list *list = __ptr_list(dp, ptr);
	unsigned char *check_ptr = &(((unsigned char *)ptr)[size - sizeof(unsigned int)]);

	if (NULL == list) {
		gxloge("[not exit]: %s\t %d\t %p\t %d\n", func, line, ptr, size);
		return;
	}

	if (!list->vaild) {
		gxloge("[has free]: %s\t %d\t %p\t %d\n", func, line, ptr, size);
		return;
	}

	if (list->size != size) {
		gxloge("[err size]: %s\t %d\t %p\t (%d, %d)\n", func, line, ptr, size, list->size);
	}

	if ((check_ptr[0] != 0xde) || (check_ptr[1] != 0xad) || (check_ptr[2] != 0xbe) || (check_ptr[3] != 0xef)) {
		gxloge("[mem bound]: %s\t %d\t %p\t %d\n", func, line, ptr, size);
	}

	list->vaild = 0;
}

#endif

void GxPlayer_HeapInit(void)
{
	if (av_heap_inited == 0) {
		int i, holesize, syssize;

		memset(&_av_heap, 0, sizeof(_av_heap));
		GxPlayer_SystemGet(PSYS_PACKET_CACHE, &syssize);
		GxPlayer_SystemGet(PSYS_HOLEMEM_SIZE, &holesize);
		_av_heap.sys.maxsiz = syssize;
		if (holesize < AV_HOLE_SIZE_1) {
			_av_heap.hole[0].maxsiz = holesize;
			if (holesize <= 0)
				gxlogi ("info! holesize=%d.\n", holesize);
		} else if ((holesize >= AV_HOLE_SIZE_1) && (holesize < AV_HOLE_SIZE_2)) {
			_av_heap.hole[0].maxsiz = AV_HOLE_SIZE_0;
			_av_heap.hole[1].maxsiz = (holesize - AV_HOLE_SIZE_0);
		} else if (holesize >= AV_HOLE_SIZE_2) {
			_av_heap.hole[0].maxsiz = AV_HOLE_SIZE_0;
			_av_heap.hole[1].maxsiz = AV_HOLE_SIZE_1;
			_av_heap.hole[2].maxsiz = holesize  - (AV_HOLE_SIZE_0 + AV_HOLE_SIZE_1);
		}
#ifdef AV_MEMORY_DEBUG
		__init_dp_list(&_av_heap.sys.dp);
#endif
		GxCore_MutexCreate(&_av_heap.mutex);
		av_heap_inited = 1;
	}
}

#ifdef AV_MEMORY_DEBUG
void *av_malloc_debug(const char *func, int line, unsigned int size)
#else
void *av_malloc(unsigned int size)
#endif
{
	int i, sys_malloc_flag = 0;
	void *p = NULL;

	_av_heap_lock();

	size += sizeof(void *);
#ifdef AV_MEMORY_DEBUG
	size += sizeof(unsigned int);
#endif

	for (i=0; i< AV_USER_MAX; i++) {
		if (_av_heap.user[i].h) {
			p = tlsf_malloc(_av_heap.user[i].h, size);
			if (p) {
#ifdef AV_MEMORY_DEBUG
				__add_dp_list(&_av_heap.user[i].dp, func, line, p, size);
#endif
				_av_heap.user[i].nowuse += size;
				if (_av_heap.user[i].nowuse > _av_heap.user[i].maxuse)
					_av_heap.user[i].maxuse = _av_heap.user[i].nowuse;
				goto end;
			}
		}
	}

	if (size <= AV_SMALLMEM_SIZE)
		sys_malloc_flag = ((_av_heap.sys.nowuse + size) <= _av_heap.sys.maxsiz) ? 1 : 0;
	else
		sys_malloc_flag = ((_av_heap.sys.nowuse + size) <= (_av_heap.sys.maxsiz - AV_SMALLMEM_RESV)) ? 1 : 0;

	if (sys_malloc_flag) {
		p = GxCore_Malloc(size);
		if (p) {
#ifdef AV_MEMORY_DEBUG
			__add_dp_list(&_av_heap.sys.dp, func, line, p, size);
#endif
			_av_heap.sys.nowuse += size;
			if (_av_heap.sys.nowuse > _av_heap.sys.maxuse)
				_av_heap.sys.maxuse = _av_heap.sys.nowuse;

			if (size <= AV_SMALLMEM_SIZE) {
				_av_heap.sys.now_smallmem_use += size;
				if (_av_heap.sys.now_smallmem_use > _av_heap.sys.max_smallmem_use)
					_av_heap.sys.max_smallmem_use = _av_heap.sys.now_smallmem_use;
			}
			goto end;
		}
	}

	for (i=0; i< AV_HOLE_MAX; i++) {
		if (_av_heap.hole[i].h) {
			p = tlsf_malloc(_av_heap.hole[i].h, size);
			if (p) {
#ifdef AV_MEMORY_DEBUG
				__add_dp_list(&_av_heap.hole[i].dp, func, line, p, size);
#endif
				_av_heap.hole[i].nowuse += size;
				if (_av_heap.hole[i].nowuse > _av_heap.hole[i].maxuse)
					_av_heap.hole[i].maxuse = _av_heap.hole[i].nowuse;
				goto end;
			}
		}
	}

	for (i=0; i< AV_HOLE_MAX; i++) {
		if ((_av_heap.hole[i].h == 0) &&
				(_av_heap.hole[i].maxsiz > size)) {
			void *s = av_memhole_malloc_user(_av_heap.hole[i].maxsiz);
			if (s) {
#ifdef AV_MEMORY_DEBUG
				__init_dp_list(&_av_heap.hole[i].dp);
#endif
				*(unsigned int *)s = 0xffffffff;
				_av_heap.hole[i].h = tlsf_init(_av_heap.hole[i].maxsiz, s);
				_av_heap.hole[i].start = (unsigned int)s;
				_av_heap.hole[i].end = (unsigned int)(s+_av_heap.hole[i].maxsiz);
				_av_heap.hole[i].nowuse = 0;
				p = tlsf_malloc(_av_heap.hole[i].h, size);
				if (p) {
#ifdef AV_MEMORY_DEBUG
					__add_dp_list(&_av_heap.hole[i].dp, func, line, p, size);
#endif
					_av_heap.hole[i].nowuse += size;
					if (_av_heap.hole[i].nowuse > _av_heap.hole[i].maxuse)
						_av_heap.hole[i].maxuse = _av_heap.hole[i].nowuse;
					goto end;
				}
			} else {
				gxloge("av memhole (%d) malloc failed!!!!\n", _av_heap.hole[i].maxsiz);
			}
			goto end;
		}
	}

end:
	if (p) {
		*(unsigned int *)p = size;
		p += sizeof(void *);
	}
	_av_heap_unlock();
	if (p == NULL) {
#ifdef AV_MEMORY_DEBUG
		extern void gxplayer_heap_info(void);
		gxplayer_heap_info();
		gxloge("(%s %d) size %d malloc failed!!!!\n", func, line, size);
		while(1);
#else
		gxloge("size %d malloc failed!!!!\n", size);
#endif
	}
	return p;
}

#ifdef AV_MEMORY_DEBUG
void av_free_debug(const char *func, int line, void *ptr)
#else
void av_free(void *ptr)
#endif
{
	unsigned int i, start;

	if (ptr == NULL)
		return;

	_av_heap_lock();

	ptr -= sizeof(void *);
	start = (unsigned int)ptr;

	for (i=0; i< AV_USER_MAX; i++) {
		if (_av_heap.user[i].h) {
			if (start > _av_heap.user[i].start && start < _av_heap.user[i].end) {
#ifdef AV_MEMORY_DEBUG
				__del_dp_list(&_av_heap.user[i].dp, func, line, ptr, *(unsigned int*)ptr);
#endif
				_av_heap.user[i].nowuse -= *(unsigned int*)ptr;
				tlsf_free(_av_heap.user[i].h, ptr);
				goto end;
			}
		}
	}

	for (i=0; i< AV_HOLE_MAX; i++) {
		if (_av_heap.hole[i].h) {
			if (start > _av_heap.hole[i].start && start < _av_heap.hole[i].end) {
#ifdef AV_MEMORY_DEBUG
				__del_dp_list(&_av_heap.hole[i].dp, func, line, ptr, *(unsigned int*)ptr);
#endif
				_av_heap.hole[i].nowuse -= *(unsigned int*)ptr;
				tlsf_free(_av_heap.hole[i].h, ptr);
				if (_av_heap.hole[i].nowuse == 0) {
#ifdef AV_MEMORY_DEBUG
					__destroy_dp_list(&_av_heap.hole[i].dp);
#endif
					tlsf_destroy(_av_heap.hole[i].h);
					av_memhole_free_user((void*)(_av_heap.hole[i].start));
					_av_heap.hole[i].h = _av_heap.hole[i].maxuse = 0;
				}
				goto end;
			}
		}
	}

#ifdef AV_MEMORY_DEBUG
	__del_dp_list(&_av_heap.sys.dp, func, line, ptr, *(unsigned int*)ptr);
#endif
	_av_heap.sys.nowuse -= *(unsigned int*)ptr;
	if (*(unsigned int*)ptr <= AV_SMALLMEM_SIZE)
		_av_heap.sys.now_smallmem_use -= *(unsigned int*)ptr;
	*(unsigned int*)ptr = 0;
	GxCore_Free(ptr);
end:
	_av_heap_unlock();
	return;
}

#ifdef AV_MEMORY_DEBUG
void av_freep_debug(const char *func, int line, void* arg)
#else
void av_freep(void* arg)
#endif
{
	void* *ptr = (void **)arg;
#ifdef AV_MEMORY_DEBUG
	av_free_debug(func, line, *ptr);
#else
	av_free(*ptr);
#endif
	*ptr = NULL;
}

#ifdef AV_MEMORY_DEBUG
void *av_realloc_debug(const char *func, int line, void *ptr, unsigned int size)
#else
void *av_realloc(void *ptr, unsigned int size)
#endif
{
	void *newptr;
	unsigned int oldsize;

	if (ptr == NULL) {
#ifdef AV_MEMORY_DEBUG
		return av_mallocz_debug(func, line, size);
#else
		return av_malloc(size);
#endif
	}

	if (size == 0) {
#ifdef AV_MEMORY_DEBUG
		av_free_debug(func, line, ptr);
#else
		av_free(ptr);
#endif
		return NULL;
	}

	oldsize = *(unsigned int *)(ptr - sizeof(void *));
	oldsize -= sizeof(void *);
#ifdef AV_MEMORY_DEBUG
	oldsize -= sizeof(unsigned int);
#endif
	if (oldsize >= size)
		return ptr;

#ifdef AV_MEMORY_DEBUG
	newptr = av_mallocz_debug(func, line, size);
#else
	newptr = av_malloc(size);
#endif
	if (newptr) {
		memcpy(newptr, ptr, oldsize);

#ifdef AV_MEMORY_DEBUG
		av_free_debug(func, line, ptr);
#else
		av_free(ptr);
#endif
	}

	return newptr;
}

#ifdef AV_MEMORY_DEBUG
void *av_mallocz_debug(const char *func, int line, unsigned int size)
#else
void *av_mallocz(unsigned int size)
#endif
{
#ifdef AV_MEMORY_DEBUG
	void *p = av_malloc_debug(func, line, size);
#else
	void *p = av_malloc(size);
#endif

	if (p)
		memset(p, 0, size);

	return p;
}

#ifdef AV_MEMORY_DEBUG
void *av_calloc_debug(const char *func, int line, int n, size_t size)
#else
void *av_calloc(int n, size_t size)
#endif
{
#ifdef AV_MEMORY_DEBUG
	return av_mallocz_debug(func, line, n * size);
#else
	return av_mallocz(n * size);
#endif
}

#ifdef AV_MEMORY_DEBUG
char *av_strdup_debug(const char *func, int line, const char *s)
#else
char *av_strdup(const char *s)
#endif
{
	int len = strlen(s);
#ifdef AV_MEMORY_DEBUG
	char *p = av_malloc_debug(func, line, len + 1);
#else
	char *p = av_malloc(len + 1);
#endif

	if (p) {
		strncpy(p, s, len);
		p[len] = 0;
	}

	return p;
}

int gxplayer_heap_register(unsigned int start, unsigned int size)
{
	int i;

	_av_heap_lock();

	for (i=0; i< AV_USER_MAX; i++) {
		if ((_av_heap.user[i].h == 0) && (size > 0)) {
			_av_heap.user[i].start = start;
			_av_heap.user[i].end = start + size;
			_av_heap.user[i].maxsiz = size;
			_av_heap.user[i].maxuse = 0;
			_av_heap.user[i].nowuse = 0;

#ifdef AV_MEMORY_DEBUG
			__init_dp_list(&_av_heap.user[i].dp);
#endif
			if (start) {
				*(unsigned int *)start = 0xffffffff;
				_av_heap.user[i].h = tlsf_init(size, (void *)start);
			}
			_av_heap_unlock();
			return 0;
		}
	}

	_av_heap_unlock();
	return -1;
}

int gxplayer_heap_unregister(unsigned int start)
{
	int i;

	_av_heap_lock();

	gxlogi_raw("-------------------------------------------\n");
	for (i=0; i< AV_USER_MAX; i++) {
		if (_av_heap.user[i].h != 0 && start == _av_heap.user[i].start) {
			if (_av_heap.user[i].nowuse != 0) {
				gxloge("heap in use !\n");
				_av_heap_unlock();
				return -1;
			}
#ifdef AV_MEMORY_DEBUG
			__printf_list_no_free(&_av_heap.user[i].dp);
			__destroy_dp_list(&_av_heap.user[i].dp);
#endif
			tlsf_destroy(_av_heap.user[i].h);
			memset(&_av_heap.user[i], 0, sizeof(_av_heap.user[i]));
			_av_heap_unlock();
			return 0;
		}
	}
	gxlogi_raw("-------------------------------------------\n");

	_av_heap_unlock();
	return -1;
}

int gxplayer_heap_modify(PlayerHeapInfo *info)
{
	int i;

	_av_heap_lock();

	_av_heap.sys.maxsiz = info->sysmem;
	for (i=0; i<AV_HOLE_MAX; i++)
		_av_heap.hole[i].maxsiz = info->holemem / AV_HOLE_MAX;

	_av_heap_unlock();
	return 0;
}

void gxplayer_heap_info(void)
{
	int i;

	if (av_heap_inited) {
		gxlogi_raw("============================================\n");
		gxlogi_raw("-[sys]                               \n");
		gxlogi_raw("   [0] maxsiz = 0x%-8x, maxuse = 0x%-8x, nowuse = 0x%-8x\n",
				_av_heap.sys.maxsiz, _av_heap.sys.maxuse, _av_heap.sys.nowuse);
		gxlogi_raw("             <= %-8dkB, maxuse = 0x%-8x, nowuxe = 0x%-8x\n",
				AV_SMALLMEM_SIZE/1024, _av_heap.sys.max_smallmem_use, _av_heap.sys.now_smallmem_use);
		gxlogi_raw("-[user]                              \n");
		for (i = 0; i < AV_USER_MAX; i++)
			gxlogi_raw("   [%1d] maxsiz = 0x%-8x, maxuse = 0x%-8x, nowuse = 0x%-8x\n",
					i, _av_heap.user[i].maxsiz, _av_heap.user[i].maxuse, _av_heap.user[i].nowuse);
		gxlogi_raw("-[hole]                              \n");
		for (i = 0; i < AV_HOLE_MAX; i++)
			gxlogi_raw("   [%1d] maxsiz = 0x%-8x, maxuse = 0x%-8x, nowuse = 0x%-8x\n",
					i, _av_heap.hole[i].maxsiz, _av_heap.hole[i].maxuse, _av_heap.hole[i].nowuse);
#ifdef AV_MEMORY_DEBUG
		gxlogi_raw("-------------------------------------------\n");
		gxlogi_raw("[not free]\n");
		__printf_list_no_free(&_av_heap.sys.dp);
		for (i = 0; i < AV_USER_MAX; i++)
			__printf_list_no_free(&_av_heap.user[i].dp);
		for (i = 0; i < AV_HOLE_MAX; i++)
			__printf_list_no_free(&_av_heap.hole[i].dp);
#endif
		gxlogi_raw("============================================\n");
	}
}

#else
void GxPlayer_HeapInit(void)
{
}

int gxplayer_heap_register(unsigned int start, unsigned int size)
{
	return 0;
}

int gxplayer_heap_unregister(unsigned int start)
{
	return 0;
}

void gxplayer_heap_info(void)
{
}

int gxplayer_heap_modify(PlayerHeapInfo *info)
{
	return 0;
}
#endif
