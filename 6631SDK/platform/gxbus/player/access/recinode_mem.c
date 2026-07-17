#include "recfs.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define min(a, b) (a)<(b)?(a):(b)

#define DURATION 2
typedef struct _Mem {
#define POINT_COUNT_TOTAL    (4)
#define PER_COUNT_SIZE   (1024*128)

	size_t capacity;

	uint64_t head;
	uint64_t tail;
	uint64_t offset;

	uint64_t point[POINT_COUNT_TOTAL];
	uint64_t point_count;

	char index[1024];
	uint64_t index_count;

	int   id;
	char  *buffer;
}Mem;

#define MAX_MEM_NUM (8)
static Mem *mem_list[MAX_MEM_NUM] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};

static int find_mem(void)
{
	int i = 0;

	for (i = 0; i < MAX_MEM_NUM; i++) {
		if (mem_list[i] == NULL)
			break;
	}

	return (i >= MAX_MEM_NUM)? -1 : i;
}

static size_t mem_can_read(Mem *mem)
{
	assert(mem != NULL);

	if (mem->head - mem->offset > mem->capacity)
		return 0;

	return mem->head - mem->offset;
}

static size_t mem_can_write(Mem *mem)
{
	assert(mem != NULL);
	return mem->capacity;
}

static int write_index(int fd)
{
	Mem *mem = mem_list[fd];
	uint64_t i = 0;
	char tmp[128];

	memset(mem->index, 0, sizeof(mem->index));

	snprintf(mem->index, sizeof(mem->index), "#EXTM3U\n#EXT-X-MEDIA-SEQUENCE:%llu\n#EXT-X-TARGETDURATION:%d\n", ++mem->index_count, DURATION);

	if (mem->index_count < POINT_COUNT_TOTAL - 1) {
		for (i=0; i<mem->point_count; i++) {
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "#EXTINF:%d,\n%d.%llu\n", DURATION, mem->id, i);
			strncat(mem->index, tmp, sizeof(mem->index));
		}
	} else {
		for (i=mem->point_count - (POINT_COUNT_TOTAL - 1); i<mem->point_count; i++) {
			memset(tmp, 0, sizeof(tmp));
			snprintf(tmp, sizeof(tmp), "#EXTINF:%d,\ngxm3u8%d.%llu\n", DURATION, mem->id, i);
			strncat(mem->index, tmp, sizeof(mem->index));
		}
	}

//	gxlogd("%s\n=====================\n", mem->index);
	
	return 0;
}

#if 0
static size_t _read_index(int fd, void* buf, size_t size)
{
	Mem *mem = (Mem*)fd;

	int index_size = strlen(mem->index);
	if (size < index_size)
		return -1;

	memcpy(buf, mem->index, index_size);

	return index_size;
}

static size_t _get_part_size(int fd, uint64_t point)
{
	Mem *mem = (Mem*)fd;

	if (point >= mem->point_count-1 || mem->point_count >= point+POINT_COUNT_TOTAL)
		return 0;

	int cur = point % POINT_COUNT_TOTAL;
	int next = (point + 1) % POINT_COUNT_TOTAL;

	return (mem->point[next]-mem->point[cur]);
}

static size_t _read_part(int fd, void* buf, size_t size, uint64_t point, off_t offset)
{
	Mem *mem = (Mem*)fd;
	if (point >= mem->point_count-1 || mem->point_count >= point+POINT_COUNT_TOTAL)
		return 0;

	int cur = point % POINT_COUNT_TOTAL;
	int next = (point + 1) % POINT_COUNT_TOTAL;
	int can_read_size = 0;

	offset += mem->point[cur];
	if (offset < mem->point[next])
		can_read_size = mem->point[next] - offset;
	else
		return 0;

	size = min(can_read_size, size);

	int l = min(size, mem->capacity - (offset % mem->capacity));

	memcpy(buf, mem->buffer + (offset % mem->capacity), l);
	/* then get the rest (if any) from the beginning of the buffer*/
	memcpy(buf + l, mem->buffer, size - l);

	return size;
}
#endif


static int _mem_open(const char *url, int mode, int flag)
{
	int32_t fd = find_mem();
	int32_t capacity = 0;
	GxUrl_GetItem(url, "size", &capacity);

	if (fd < 0) return -1;

	Mem *mem = (Mem *) malloc(sizeof(Mem) + capacity);
	if (mem == NULL) return -1;

	mem->capacity = capacity;
	mem->buffer   = (char*)mem + sizeof(Mem);

	mem->head     = 0;
	mem->tail     = 0;
	mem->offset   = 0;

	mem->point_count     = 0;
	mem->index_count     = 0;

	memset(mem->point, 0, sizeof(uint64_t)*POINT_COUNT_TOTAL);

	GxUrl_GetItem(url, "id", &mem->id);

	mem_list[fd] = mem;

	return fd;
};

static void _mem_close(int fd)
{
	if (mem_list[fd])
		free(mem_list[fd]);

	mem_list[fd] = NULL;
}

static size_t _mem_read(int fd, void* buf, size_t size)
{
	Mem *mem = mem_list[fd];

	assert(buf != NULL);

	if (mem->offset >= mem->head) {
		mem->offset = mem->head;
		return 0;
	} else if (mem->offset < mem->tail) {
		mem->offset = mem->tail;
	} else {
		// valid mem
	}

	size = min(size, mem_can_read(mem));
	int l = min(size, mem->capacity - (mem->offset % mem->capacity));

	memcpy(buf, mem->buffer + (mem->offset % mem->capacity), l);
	/* then get the rest (if any) from the beginning of the buffer*/
	memcpy(buf + l, mem->buffer, size - l);

	return size;
}

static size_t _mem_write(int fd, void* buf, size_t size)
{
	Mem *mem = mem_list[fd];

	assert(buf != NULL);

	if (size >= mem_can_write(mem)) return -1;

	int l = min(size, mem->capacity - (mem->offset % mem->capacity));

	memcpy(mem->buffer + (mem->offset % mem->capacity), buf, l);
	/* then put the rest (if any) at the beginning of the buffer*/
	if ((size -l) > 0)
		memcpy(mem->buffer, buf + l, size - l);

	mem->head += size;
	if (mem->head > mem->tail + mem->capacity)
		mem->tail = mem->head - mem->capacity;

	if (mem->point_count == 0) {
		mem->point[mem->point_count] = 0;
		mem->point_count++;
	} else {
		// mark point
		int prev = (mem->point_count-1) % POINT_COUNT_TOTAL;

		if (mem->offset > mem->point[prev]+PER_COUNT_SIZE) {
			int cur = (mem->point_count++) % POINT_COUNT_TOTAL;
			mem->point[cur] = mem->offset;

			// update index
			write_index(fd);
		}
	}

	return size;
}


static off_t _mem_lseek(int fd, off_t pos, int flag)
{
	Mem *mem = mem_list[fd];

	switch (flag) {
		case SEEK_SET:
			if (pos > mem->head)
				mem->offset = mem->head;
			else if (pos < mem->tail)
				mem->offset = mem->tail;
			else
				mem->offset = pos;

			break;
		case SEEK_CUR:
			// not support
			break;
		case SEEK_END:
			// not support
			break;
		default:
			break;
	};

	return (off_t)mem->offset;
}

struct recinode_ops recinodeops_mem = {
	.open   = _mem_open,
	.close  = _mem_close,
	.read   = _mem_read,
	.write  = _mem_write,
	.lseek  = _mem_lseek,
//	.get_index = _read_index,
//	.get_part  = _read_part,
//	.get_part_size = _get_part_size,
};






