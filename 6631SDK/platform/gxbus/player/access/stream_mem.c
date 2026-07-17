#include "gx_stream.h"
#include "gx_demux.h"
#include "gx_pvr.h"

#define MAX_FILE     (8)
#define MAX_HDR_SIZE (128)
#define HDR_OFFSET       (0)
#define DURATION_OFFSET  (8)
#define FILESIZE_OFFSET  (20)
#define INVALID_INDEX(x) ((x<0 || x>=MAX_FILE) ? 1 : 0)

typedef struct memfile
{
	int             used;
	int             rptr;
	int             wptr;
	int             size;
	int             fmt;
	int             hwsafe;
	unsigned int    filesize;
	unsigned int    duration;
	unsigned char  *addr;
	unsigned char  *extra_addr;
	unsigned int    extra_size;
} memfile_t;

static handle_t mutex = -1;

static memfile_t memfiles[MAX_FILE];

static int _alloc_file_entry()
{
	int i;

	for(i = 0; i < MAX_FILE; i++)
	{
		if(memfiles[i].used == 0)
		{
			memset(&memfiles[i], 0, sizeof(memfile_t));
			memfiles[i].used = 1;
			return i;
		}
	}

	return -1;
}

static void _fill_hdr(int index, unsigned int size)
{
	unsigned char *hdr = memfiles[index].extra_addr;

	hdr += HDR_OFFSET;
	hdr[0] = 'H';
	hdr[1] = 'D';
	hdr[2] = 'R';
	hdr[3] = '0';
	hdr[4] = (size >> 24) & 0xff;
	hdr[5] = (size >> 16) & 0xff;
	hdr[6] = (size >>  8) & 0xff;
	hdr[7] = (size >>  0) & 0xff;
}

static void _fill_filesize(int index, unsigned int size)
{
	unsigned char *hdr = memfiles[index].extra_addr;

	hdr += FILESIZE_OFFSET;
	hdr[0] = 'F';
	hdr[1] = 'I';
	hdr[2] = 'L';
	hdr[3] = 'E';
	hdr[4] = 'S';
	hdr[5] = 'I';
	hdr[6] = 'Z';
	hdr[7] = 'E';
	hdr += 8;
	hdr[0] = (size >> 24) & 0xff;
	hdr[1] = (size >> 16) & 0xff;
	hdr[2] = (size >>  8) & 0xff;
	hdr[3] = (size >>  0) & 0xff;
}

static void _fill_duration(int index, unsigned int size)
{
	unsigned char *hdr = memfiles[index].extra_addr;

	hdr += DURATION_OFFSET;
	hdr[0] = 'D';
	hdr[1] = 'U';
	hdr[2] = 'R';
	hdr[3] = 'A';
	hdr[4] = 'T';
	hdr[5] = 'I';
	hdr[6] = 'O';
	hdr[7] = 'N';
	hdr += 8;
	hdr[0] = (size >> 24) & 0xff;
	hdr[1] = (size >> 16) & 0xff;
	hdr[2] = (size >>  8) & 0xff;
	hdr[3] = (size >>  0) & 0xff;
}

static int _get_filesize(int index)
{
#define TS_PACKET_LEN (188)
	int filesize = 0;
	unsigned char *hdr = memfiles[index].extra_addr;

	hdr += FILESIZE_OFFSET;
	if ((hdr[0] == 'F') &&
			(hdr[1] == 'I') &&
			(hdr[2] == 'L') &&
			(hdr[3] == 'E') &&
			(hdr[4] == 'S') &&
			(hdr[5] == 'I') &&
			(hdr[6] == 'Z') &&
			(hdr[7] == 'E')) {
		hdr += 8;
		filesize |= (hdr[0] << 24);;
		filesize |= (hdr[1] << 16);;
		filesize |= (hdr[2] <<  8);;
		filesize |= (hdr[3] <<  0);;
		if (memfiles[index].fmt == GX_MEM_FMT_TS)
			filesize = filesize / TS_PACKET_LEN * TS_PACKET_LEN;
	}

	return filesize;
}

static int _get_duration(int index)
{
	int duration = 0;
	unsigned char *hdr = memfiles[index].extra_addr;

	hdr += DURATION_OFFSET;
	if ((hdr[0] == 'D') &&
			(hdr[1] == 'U') &&
			(hdr[2] == 'R') &&
			(hdr[3] == 'A') &&
			(hdr[4] == 'T') &&
			(hdr[5] == 'I') &&
			(hdr[6] == 'O') &&
			(hdr[7] == 'N')) {
		hdr += 8;
		duration |= (hdr[0] << 24);;
		duration |= (hdr[1] << 16);;
		duration |= (hdr[2] <<  8);;
		duration |= (hdr[3] <<  0);;
	}

	return duration;
}

static int _mem_contorl(int index, GxRecordPVRControl *ctrl)
{
	int ret = GX_PLAYER_OK;

	switch (ctrl->opt) {
	case GX_RECORD_PVR_GET_POS_BY_TIME:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			if (memfiles[index].fmt == GX_MEM_FMT_TS) {
				off_t offset = 0;

				if (memfiles[index].duration > 0)
					offset = time_pos->timems * (int64_t)memfiles[index].filesize / (int64_t)memfiles[index].duration;
				time_pos->offset = offset / TS_PACKET_LEN * TS_PACKET_LEN;
			} else
				time_pos->offset = 0;
			break;
		}
	case GX_RECORD_PVR_GET_DURATION:
		*(int64_t *)ctrl->arg = memfiles[index].duration;
		break;
	case GX_RECORD_PVR_GET_FILESIZE:
		*(int64_t *)ctrl->arg = memfiles[index].filesize;
		break;
	case GX_RECORD_PVR_GET_CURTIME:
		if (memfiles[index].fmt == GX_MEM_FMT_TS) {
			int64_t times = 0;

			if (memfiles[index].filesize > 0)
				times = (int64_t)memfiles[index].rptr * (int64_t)memfiles[index].duration / (int64_t)memfiles[index].filesize;
			*(int64_t *)ctrl->arg = times;
		} else
			*(int64_t *)ctrl->arg = 0;
		break;
	case GX_RECORD_PVR_GET_HWSAFE:
		*(int *)ctrl->arg = memfiles[index].hwsafe;
		break;
	default:
		ret = GX_PLAYER_ERROR;
		break;
	}

	return ret;
}

static int mem_open(GxStream*  s, int mode)
{
	int ret, index;
	int addr, size;
	int fmt, vcodec, acodec, hwsafe = 0;

	if (mutex == -1) {
		ret = GxCore_MutexCreate(&mutex);
		if (ret != GXCORE_SUCCESS) {
			gxlogd("create mutex for memfiles failed.\n");
			return -1;
		}
	}

	GxCore_MutexLock(mutex);
	index = _alloc_file_entry();
	if (INVALID_INDEX(index)) {
		GxCore_MutexUnlock(mutex);
		return -1;
	}

	s->fd = index;
	s->file_format = GX_STREAMTYPE_MEM;

	GxUrl_GetItem(s->url, GX_URL_KEY_ADDR,   &addr);
	GxUrl_GetItem(s->url, GX_URL_KEY_SIZE,   &size);
	GxUrl_GetItem(s->url, GX_URL_KEY_FMT,    &fmt);
	GxUrl_GetItem(s->url, GX_URL_KEY_VCODEC, &vcodec);
	GxUrl_GetItem(s->url, GX_URL_KEY_ACODEC, &acodec);
	GxUrl_GetItem(s->url, GX_URL_KEY_HWSAFE, &hwsafe);

	memfiles[index].addr = (unsigned char*)addr;
	memfiles[index].size = (unsigned int)size;
	memfiles[index].rptr = 0;
	memfiles[index].wptr = 0;
	memfiles[index].duration = 0;
	memfiles[index].filesize = 0;
	memfiles[index].extra_addr = NULL;
	memfiles[index].extra_size = 0;
	memfiles[index].fmt    = fmt;
	memfiles[index].hwsafe = hwsafe;

	if (fmt == GX_MEM_FMT_MP3)
		s->demuxer_type = GX_DEMUXER_TYPE_AUDIO;
	else if (fmt == GX_MEM_FMT_ES)
		s->demuxer_type = GX_DEMUXER_TYPE_ES;
	else if (fmt == GX_MEM_FMT_PES)
		s->demuxer_type = GX_DEMUXER_TYPE_MPEG_PES;
	else if (fmt == GX_MEM_FMT_TS)
		s->demuxer_type = GX_DEMUXER_TYPE_HW_TS;
	else
		s->demuxer_type = GX_DEMUXER_TYPE_UNKNOWN;

	s->prog_now = 0;
	if (vcodec != -1) {
		s->prog[0].acodec = -1;
		s->prog[0].vcodec = vcodec_url2fourcc(vcodec);
		s->prog_max = 1;
	} else if (acodec != -1) {
		s->prog[0].acodec = acodec_url2fourcc(acodec);
		s->prog[0].vcodec = -1;
		s->prog_max = 1;
	} else
		s->prog_max = 0;

	GxCore_MutexUnlock(mutex);

	return GX_PLAYER_OK;
}


static int mem_read(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int len, index, size, rptr;
	unsigned char *addr = NULL;

	GxCore_MutexLock(mutex);
	index = s->fd;
	if (INVALID_INDEX(index)) {
		GxCore_MutexUnlock(mutex);
		return -1;
	}

	addr = memfiles[index].addr;
	size = memfiles[index].size;
	rptr = memfiles[index].rptr;

	if (memfiles[index].fmt == GX_MEM_FMT_TS) {
		if ((memfiles[index].extra_addr != NULL) &&
				(memfiles[index].extra_size > 0)) {
			memfiles[index].duration = _get_duration(index);
			memfiles[index].filesize = _get_filesize(index);
			size = memfiles[index].filesize;
		}
	}

	len = ((rptr + max_len) < size) ? max_len : (size - rptr);
	memcpy(buffer, addr+rptr, len);
	memfiles[index].rptr += len;
	GxCore_MutexUnlock(mutex);

	return len;
}

static int mem_write(GxStream*  s, uint8_t * buffer, size_t max_len)
{
	int len, index, size, wptr;
	unsigned char *addr = NULL;

	GxCore_MutexLock(mutex);
	index = s->fd;
	if (INVALID_INDEX(index)) {
		GxCore_MutexUnlock(mutex);
		return -1;
	}

	addr = memfiles[index].addr;
	size = memfiles[index].size;
	wptr = memfiles[index].wptr;

	len = ((wptr + max_len) < size) ? max_len : (size - wptr);
	memcpy(addr+wptr, buffer, len);
	memfiles[index].wptr += len;

	if (memfiles[index].fmt == GX_MEM_FMT_TS) {
		if ((memfiles[index].extra_addr != NULL) &&
				(memfiles[index].extra_size > 0)) {
			_fill_duration(index, memfiles[index].duration);
			_fill_filesize(index, memfiles[index].wptr);
			memfiles[index].filesize = memfiles[index].wptr;
		}
	}
	GxCore_MutexUnlock(mutex);

	return len;
}

static int mem_seek(GxStream*  s, off_t newpos)
{
	int index;

	GxCore_MutexLock(mutex);
	index = s->fd;
	if (INVALID_INDEX(index)) {
		GxCore_MutexUnlock(mutex);
		return -1;
	}

	if (newpos > memfiles[index].size) {
		s->eof = 1;
		GxCore_MutexUnlock(mutex);
		return -1;
	} else {
		memfiles[index].rptr = newpos;
		memfiles[index].wptr = newpos;
	}
	GxCore_MutexUnlock(mutex);

	return GX_PLAYER_OK;
}

static int mem_control(GxStream*  s, int cmd, void* arg)
{
	int index;
	int ret = GX_PLAYER_OK;

	GxCore_MutexLock(mutex);
	index = s->fd;
	if (INVALID_INDEX(index)) {
		GxCore_MutexUnlock(mutex);
		return -1;
	}

	switch (cmd) {
	case GX_STREAM_CTRL_GET_SIZE:
		{
			off_t* size = (off_t*)arg;

			*size = memfiles[index].size;
			break;
		}
	case GX_STREAM_CTRL_PROBE_HDR:
		{
			unsigned char *hdr_buffer = memfiles[index].addr;

			if ((hdr_buffer[0] == 'H') &&
					(hdr_buffer[1] == 'D') &&
					(hdr_buffer[2] == 'R') &&
					(hdr_buffer[3] == '0')) {
				unsigned int hdr_size = 0;

				hdr_size |= (hdr_buffer[4] << 24);
				hdr_size |= (hdr_buffer[5] << 16);
				hdr_size |= (hdr_buffer[6] <<  8);
				hdr_size |= (hdr_buffer[7] <<  0);
				memfiles[index].extra_addr = memfiles[index].addr;
				memfiles[index].extra_size = hdr_size;
				hdr_size  = ((hdr_size + MAX_HDR_SIZE) / 1024 + 1) * 1024;
				memfiles[index].addr = memfiles[index].addr + hdr_size;
				memfiles[index].size = memfiles[index].size - hdr_size;
				memfiles[index].filesize = _get_filesize(index);
				memfiles[index].duration = _get_duration(index);
				*(int *)arg = memfiles[index].extra_size;;
			} else
				*(int *)arg = 0;
			break;
		}
	case GX_STREAM_CTRL_READ_HDR:
		{
			struct vol_hdr *hdr = (struct vol_hdr *)arg;

			if (memfiles[index].extra_size > 0) {
				memcpy(hdr->buffer, memfiles[index].extra_addr + MAX_HDR_SIZE, memfiles[index].extra_size);
				hdr->size = memfiles[index].extra_size;
			}
			break;
		}
	case GX_STREAM_CTRL_WRITE_HDR:
		{
			struct vol_hdr *hdr = (struct vol_hdr *)arg;

			if (memfiles[index].extra_size != 0)
				break;

			if ((hdr->buffer != NULL) && (hdr->size != 0)) {
				unsigned int hdr_size = ((hdr->size + MAX_HDR_SIZE) / 1024 + 1) * 1024;

				if (hdr_size < memfiles[index].size) {
					memfiles[index].extra_addr = memfiles[index].addr;
					memfiles[index].extra_size = hdr->size;
					memfiles[index].addr = memfiles[index].addr + hdr_size;
					memfiles[index].size = memfiles[index].size - hdr_size;
					if (memfiles[index].wptr != 0)
						memcpy(memfiles[index].addr, memfiles[index].extra_addr, memfiles[index].wptr);
					_fill_hdr     (index, hdr->size);
					_fill_duration(index, memfiles[index].duration);
					_fill_filesize(index, memfiles[index].wptr);
					memcpy(memfiles[index].extra_addr + MAX_HDR_SIZE, hdr->buffer, hdr->size);
				}
			}
			break;
		}
	case GX_STREAM_CTRL_SET_TIMESTAMP:
		memfiles[index].duration = *(unsigned int *)arg;
		break;
	case GX_STREAM_CTRL_PVR_GET_CONTROL:
		ret = _mem_contorl(index, (GxRecordPVRControl *)arg);
		break;
	default:
		ret = GX_PLAYER_ERROR;
		break;
	}
	GxCore_MutexUnlock(mutex);

	return ret;
}

static void mem_close(GxStream*  s)
{
	int index;

	GxCore_MutexLock(mutex);
	index = s->fd;
	if(INVALID_INDEX(index))
	{
		GxCore_MutexUnlock(mutex);
		return;
	}

	gxlogd ("%s: r - %p - %d\n", __func__, memfiles[index].addr, memfiles[index].rptr);
	gxlogd ("%s: w - %p - %d\n", __func__, memfiles[index].addr, memfiles[index].wptr);
	memset(&memfiles[index], 0, sizeof(memfile_t));
	s->fd = -1;
	GxCore_MutexUnlock(mutex);
}


static int mem_stop(GxMediaFilter*  filter)
{
	GxStream*  s = GXSTREAM(filter);

	if(s && s->fd >= 0) {
		mem_close(s);
		s->fd  = -1;
	}
	return GX_PLAYER_OK;
}

GxStreamClass gx_stream_mem = {
	._inherit = {
		._inherit = {
			.name    = "StreamMem",
			.parent  = &gx_streambase,
			.size    = sizeof(GxStream),
		},
		.run     = NULL,
		.stop    = mem_stop,
		.pause   = NULL,
		.resume  = NULL,
		.config  = NULL,
	},
	.protocols = {"mem", NULL},
	DEF_AUTHOR("Stream","file","No description","zhaogf","No comment"),

	.flags   = 0,
	.open    = mem_open,
	.read    = mem_read,
	.write   = mem_write,
	.seek    = mem_seek,
	.control = mem_control,
	.close   = mem_close,
};
