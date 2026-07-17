#include "gx_common.h"
#include "gx_system.h"
#include "recfs.h"
#include "../libpvr.h"
#include "pvr_config.h"
#include "pvr_file.h"
#include "pvr_parser.h"
#include "pvr_utils.h"
#include "pvr_enc.h"

typedef struct {
	PVRParser          *parser;
	struct record_file *ts_fd;
	int                 ts_no;
	char               *path;
	char               *file;
	char                tmpbuf0[PVR_MAX_BUF_SIZE];
	char                tmpbuf1[PVR_MAX_BUF_SIZE];
	handle_t            mutex;
	char               *desc_key;
} PVRSegContext;

#define PVR_SEG_DEBUG_PRINT(printf_func) do {                \
	int debug_flags = 0;                                     \
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_PVR, &debug_flags); \
	if (debug_flags)                                         \
		printf_func;                                         \
} while(0)

#define PVR_SEG_CHECK_NULL(ptr) do {           \
	if (NULL == ptr) {                         \
		gxloge(" ptr is NULL\n");              \
		return -1;                             \
	}                                          \
} while(0)

static void _get_seg_info(PVRSegContext *seg, PVRInfo *info)
{
	info->tl_filesize = pvr_parser_get_filesize(seg->parser, PVRFILE_TOTAL);
	info->tr_filesize = pvr_parser_get_filesize(seg->parser, PVRFILE_EXIST);
	info->tl_duration = pvr_parser_get_duration(seg->parser, PVRFILE_TOTAL);
	info->tr_duration = pvr_parser_get_duration(seg->parser, PVRFILE_EXIST);
	return;
}

static int64_t _get_seg_curtime(PVRSegContext *seg)
{
	int64_t seg_time = 0;

	if (seg->ts_no >= 0) {
		seg_time += pvr_parser_get_starttime_byseqno(seg->parser, seg->ts_no, PVRFILE_TOTAL);
		if (seg->ts_fd) {
			int64_t timems = 0;
			off_t   offset = recfile_getpos(seg->ts_fd);
			int64_t duration = pvr_parser_get_duration(seg->parser, PVRFILE_TOTAL);
			off_t   filesize = pvr_parser_get_filesize(seg->parser, PVRFILE_TOTAL);

			if (filesize > 0)
				timems = (int64_t)(offset * ((float)duration / (float)filesize));
			seg_time = (timems + seg_time);
		}
	}

	return seg_time;
}

static int64_t _get_seg_timebypos(PVRSegContext *seg, off_t pos)
{
	off_t seg_offset = pos;
	int64_t seg_time = 0;

	if (seg_offset > 0) {
		int64_t timems = 0;
		int64_t duration = pvr_parser_get_duration(seg->parser, PVRFILE_TOTAL);
		off_t   filesize = pvr_parser_get_filesize(seg->parser, PVRFILE_TOTAL);

		if (filesize > 0)
			timems = (int64_t)(seg_offset * ((float)duration / (float)filesize));
		seg_time = (((timems + seg_time) > duration) ? duration : (timems + seg_time));
	}

	return seg_time;
}

static off_t _get_seg_posbytime(PVRSegContext *seg, int64_t timems)
{
	off_t seg_pos = 0;
	int64_t seg_time = timems;

	if (seg_time > 0) {
		off_t pos = 0;
		int64_t duration = pvr_parser_get_duration(seg->parser, PVRFILE_TOTAL);
		off_t   filesize = pvr_parser_get_filesize(seg->parser, PVRFILE_TOTAL);

		if (duration > 0)
			pos = (int64_t)(seg_time * ((float)filesize / (float)duration));
		seg_pos = (((pos + seg_pos) > filesize) ? filesize : (pos + seg_pos));
	}

	return seg_pos;
}

static void *seg_open(char *url, mode_t mode)
{
	PVRSegContext *seg = NULL;
	int path_size = 0;

	seg = av_mallocz(sizeof(PVRSegContext));
	if (!seg) {
		gxloge(" malloc fail\n");
		return NULL;
	}

	seg->path = pvr_parse_path(url, &path_size);
	if (!seg->path) {
		gxloge(" [%s] path parse fail\n", url);
		goto seg_open_error;
	}
	seg->file = av_strdup(url);

	seg->parser = pvr_parser_alloc((const char *)url);
	if (NULL == seg->parser) {
		gxloge(" [%s] seg parser alloc fail\n", url);
		goto seg_open_error;
	}
	seg->ts_no = -1;
	GxCore_MutexCreate(&seg->mutex);
	return (void *)seg;

seg_open_error:
	if (seg) {
		if (seg->parser)
			pvr_parser_free(seg->parser);
		if (seg->path)
			av_free(seg->path);
		if (seg->file)
			av_free(seg->file);
		av_free(seg);
	}
	return NULL;
}

static int seg_close(void *priv)
{
	unsigned int i = 0;
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);

	GxCore_MutexLock(seg->mutex);
	if (seg->parser)
		pvr_parser_free(seg->parser);
	if (seg->ts_fd)
		recfile_close(seg->ts_fd);
	seg->ts_no = -1;
	if (seg->path)
		av_free(seg->path);
	if (seg->file)
		av_free(seg->file);
	GxCore_MutexUnlock(seg->mutex);
	GxCore_MutexDelete(seg->mutex);
	av_free(seg);

	return 0;
}

static int seg_read(void *priv, uint8_t *buffer, size_t len)
{
	int ret = 0;
	int seqno = -1;
	PVRSegContext *seg = (PVRSegContext *)priv;
	unsigned char *inbuf = NULL;
	unsigned int   inlen = 0;
	unsigned int   rlen = 0;

	PVR_SEG_CHECK_NULL(seg);
	PVR_SEG_CHECK_NULL(seg->parser);

	inbuf = buffer;
	inlen = len;
	GxCore_MutexLock(seg->mutex);
seg_read_retry2:
	if (NULL == seg->ts_fd) {
		char *url0 = seg->tmpbuf0;
		char *url1 = seg->tmpbuf1;

		seqno = pvr_parser_next_url(seg->parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			GxCore_MutexUnlock(seg->mutex);
			return 0;
		}
		seg->ts_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", seg->path, url1);
		PVR_SEG_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url0));
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		seg->ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == seg->ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			GxCore_MutexUnlock(seg->mutex);
			return -1;
		}
	}
	ret = recfile_read(seg->ts_fd, inbuf + rlen, inlen - rlen);
	if (ret < 0) {
		gxloge("read fail !!!!\n");
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	rlen += ret;
	if (rlen < len) {
		recfile_close(seg->ts_fd);
		seg->ts_fd = NULL;
		if (ret == 0)
			goto seg_read_retry2;
	}
	seg->desc_key = pvr_parser_get_keydesc_byseqno(seg->parser, seg->ts_no);
	GxCore_MutexUnlock(seg->mutex);

	return rlen;
}

static off_t seg_seek(void *priv, off_t offset)
{
	int seqno = -1;
	PVRSegContext *seg = (PVRSegContext *)priv;
	off_t seg_pos = 0, ts_pos = 0;

	PVR_SEG_CHECK_NULL(seg);
	PVR_SEG_CHECK_NULL(seg->parser);

	GxCore_MutexLock(seg->mutex);
	//find tspos in segfile
	seg_pos = offset;
	seqno = pvr_parser_get_seqno_bypos(seg->parser, seg_pos);
	if (-1 == seqno) {
		gxlogi(" [%lld] seqno not find in seg parser\n", seg_pos);
		goto seg_seek_error;
	}
	if (seqno != seg->ts_no) {
		if (seg->ts_fd)
			recfile_close(seg->ts_fd);
		seg->ts_fd = NULL;
	}
	if (NULL == seg->ts_fd) {
		char *url0 = seg->tmpbuf0;
		char *url1 = seg->tmpbuf1;

		seg->ts_no = pvr_parser_get_url_byseqno(seg->parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", seg->path, url1);
		PVR_SEG_DEBUG_PRINT(gxlogi("--> [pvr evt S]: %s\n", url0));
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		seg->ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == seg->ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			goto seg_seek_error;
		}
	}
	ts_pos = seg_pos - pvr_parser_get_startpos_byseqno(seg->parser, seqno, PVRFILE_TOTAL);
	if (ts_pos < 0) {
		gxloge("[%d] startpos not find in seg parser\n", seqno);
		goto seg_seek_error;
	}
	recfile_seek(seg->ts_fd, ts_pos, SEEK_SET);
	GxCore_MutexUnlock(seg->mutex);
	return offset;

seg_seek_error:
	GxCore_MutexUnlock(seg->mutex);
	return -1;
}

static int seg_getinfo(void *priv, PVRInfo *info)
{
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);
	PVR_SEG_CHECK_NULL(seg->parser);

	GxCore_MutexLock(seg->mutex);
	_get_seg_info(seg, info);
	GxCore_MutexUnlock(seg->mutex);

	return 0;
}

static off_t seg_getpos(void *priv)
{
	off_t seg_pos = 0;
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);
	PVR_SEG_CHECK_NULL(seg->parser);

	GxCore_MutexLock(seg->mutex);
	if (seg->ts_no >= 0) {
		seg_pos += pvr_parser_get_startpos_byseqno(seg->parser, seg->ts_no, PVRFILE_TOTAL);
		if (seg->ts_fd)
			seg_pos += recfile_getpos(seg->ts_fd);
	}
	GxCore_MutexUnlock(seg->mutex);

	return seg_pos;
}

static size_t seg_probehdr(void *priv)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);

	GxCore_MutexLock(seg->mutex);
	url = seg->tmpbuf0;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", seg->path, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	pvr_get_absolute_url(url, PVR_MAX_URL_LEN);
	PVR_SEG_DEBUG_PRINT(gxlogi("--> [pvr evt P]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(seg->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_getsize(fd);
	recfile_close(fd);
	GxCore_MutexUnlock(seg->mutex);

	return size;
}

static size_t seg_readhdr(void *priv, uint8_t *buffer, size_t len)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);

	GxCore_MutexLock(seg->mutex);
	url = seg->tmpbuf0;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", seg->path, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	pvr_get_absolute_url(url, PVR_MAX_URL_LEN);
	PVR_SEG_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(seg->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_read(fd, buffer, len);
	recfile_close(fd);
	GxCore_MutexUnlock(seg->mutex);

	return size;
}

static int seg_setctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRSegContext *seg = (PVRSegContext *)priv;

	GxCore_MutexLock(seg->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_FREE_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;

			if (info->pvr_file)
				av_free((void *)info->pvr_file);
			if (info->dir)
				av_free((void *)info->dir);
			if (info->prefix)
				av_free((void *)info->prefix);
			if (info->path)
				av_free((void *)info->path);
			ret = 0;
		}
		break;
	default:
		break;
	}
	GxCore_MutexUnlock(seg->mutex);

	return ret;
}

static int seg_getctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRSegContext *seg = (PVRSegContext *)priv;

	PVR_SEG_CHECK_NULL(seg);
	PVR_SEG_CHECK_NULL(seg->parser);

	GxCore_MutexLock(seg->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_GET_DURATION:
	case GX_RECORD_PVR_GET_FILESIZE:
		{
			PVRInfo info;
			_get_seg_info(seg, &info);
			if (ctrl->opt == GX_RECORD_PVR_GET_DURATION)
				*(int64_t *)ctrl->arg = info.tl_duration;
			else if (ctrl->opt == GX_RECORD_PVR_GET_FILESIZE)
				*(int64_t *)ctrl->arg = info.tl_filesize;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_CURTIME:
		{
			*(int64_t *)ctrl->arg = _get_seg_curtime(seg);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_POS_BY_TIME:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->offset = _get_seg_posbytime(seg, time_pos->timems);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_TIME_BY_POS:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->timems = _get_seg_timebypos(seg, time_pos->offset);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;
			PVRParser *parser = seg->parser;
			char *pvr_path = NULL;;
			char *pvr_file = seg->tmpbuf0;
			char *seg_file = seg->tmpbuf0;
			int dir_size = 0;

			info->type             = GX_RECORD_PVR_SEG_FILE;
			info->segment.desc     = pvr_parser_get_arg(parser, PVRFILE_GET_DESC);
			info->segment.filesize = pvr_parser_get_filesize(parser, PVRFILE_EXIST);
			info->segment.duration = pvr_parser_get_duration(parser, PVRFILE_EXIST);

			//SEG prev path: ..
			pvr_path = pvr_parse_path(seg->path, &dir_size);
			pvr_get_absolute_url(pvr_path, dir_size);
			snprintf(pvr_file, PVR_MAX_URL_LEN, "%s%s", pvr_path, PVR_PVRFILE_SUFFIX);
			av_free(pvr_path);
			info->pvr_file = av_strdup(pvr_file);
			snprintf(seg_file, PVR_MAX_URL_LEN, "%s", seg->file);
			pvr_get_absolute_url(seg_file, PVR_MAX_URL_LEN);
			info->dir    = pvr_parse_path_remove_suffix(seg_file, PVR_SEGFILE_SUFFIX, &dir_size);
			info->prefix = pvr_parse_prefix(seg_file, PVR_SEGFILE_SUFFIX);
			info->path   = av_strdup(seg->path);

			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_KEY_DESC:
		{
			ctrl->arg = (void *)seg->desc_key;
			ret = 0;
		}
		break;
	default:
		gxloge(" [%d] seg option not find\n", ctrl->opt);
		break;
	}
	GxCore_MutexUnlock(seg->mutex);

	return ret;
}

PVROps pvr_seg_ops = {
	.open         = seg_open,
	.close        = seg_close,
	.write        = NULL,
	.read         = seg_read,
	.seek         = seg_seek,
	.getinfo      = seg_getinfo,
	.getpos       = seg_getpos,
	.settime      = NULL,
	.probehdr     = seg_probehdr,
	.readhdr      = seg_readhdr,
	.writehdr     = NULL,
	.setctrl      = seg_setctrl,
	.getctrl      = seg_getctrl,
};
