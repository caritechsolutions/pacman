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
} PVREvtSegRW;

typedef struct {
	PVRParser             *parser;
	int                    seg_no;
	char                  *path;
	char                  *file;
	PVREvtSegRW            seg_rw;
	char                   tmpbuf0[PVR_MAX_BUF_SIZE];
	char                   tmpbuf1[PVR_MAX_BUF_SIZE];
	char                   tmpbuf2[PVR_MAX_BUF_SIZE];
	handle_t               mutex;
	char                   mkdir;
	char                   write_flag;
	const char            *desc_key;
} PVREvtContext;

#define PVR_EVT_DEBUG_PRINT(printf_func) do {                \
	int debug_flags = 0;                                     \
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_PVR, &debug_flags); \
	if (debug_flags)                                         \
		printf_func;                                         \
} while(0)

#define PVR_EVT_CHECK_NULL(ptr) do {           \
	if (NULL == ptr) {                         \
		gxloge(" ptr is NULL\n");              \
		return -1;                             \
	}                                          \
} while(0)

static void _clear_evt_seg_rw(PVREvtSegRW *rw)
{
	if (rw->ts_fd) {
		recfile_close(rw->ts_fd);
		rw->ts_fd = NULL;
	}

	if (rw->parser) {
		pvr_parser_free(rw->parser);
		rw->parser = NULL;
	}

	rw->ts_no = -1;
	return;
}

static void _get_evt_info(PVREvtContext *evt, PVRInfo *info)
{
	info->tl_filesize = pvr_parser_get_filesize(evt->parser, PVRFILE_TOTAL);
	info->tr_filesize = pvr_parser_get_filesize(evt->parser, PVRFILE_EXIST);
	info->tl_duration = pvr_parser_get_duration(evt->parser, PVRFILE_TOTAL);
	info->tr_duration = pvr_parser_get_duration(evt->parser, PVRFILE_EXIST);
	return;
}

static int64_t _get_evt_curtime(PVREvtContext *evt)
{
	int64_t evt_time = 0;

	if (evt->parser && (evt->seg_no >= 0))
		evt_time += pvr_parser_get_starttime_byseqno(evt->parser, evt->seg_no, PVRFILE_EXIST);
	else
		return evt_time;

	if (evt->seg_rw.parser && (evt->seg_rw.ts_no >= 0)) {
		evt_time += pvr_parser_get_starttime_byseqno(evt->seg_rw.parser, evt->seg_rw.ts_no, PVRFILE_EXIST);
		if (evt->seg_rw.ts_fd) {
			int64_t timems = 0;
			off_t   offset = recfile_getpos(evt->seg_rw.ts_fd);
			int64_t duration = pvr_parser_get_duration(evt->parser, PVRFILE_EXIST);
			int64_t seg_time = pvr_parser_get_duration(evt->seg_rw.parser, PVRFILE_EXIST);
			off_t   seg_size = pvr_parser_get_filesize(evt->seg_rw.parser, PVRFILE_EXIST);

			if (seg_size > 0)
				timems = (int64_t)(offset * ((float)seg_time / (float)seg_size));
			evt_time = (((timems + evt_time) > duration) ? duration : (timems + evt_time));
		}
	}

	return evt_time;
}

static int64_t _get_evt_timebypos(PVREvtContext *evt, off_t pos)
{
	off_t evt_offset = pos;
	int64_t evt_time = 0;

	if (evt_offset > 0) {
		int64_t timems = 0;
		int64_t duration = pvr_parser_get_duration(evt->parser, PVRFILE_EXIST);
		off_t   filesize = pvr_parser_get_filesize(evt->parser, PVRFILE_EXIST);

		if (filesize > 0)
			timems = (int64_t)(evt_offset * ((float)duration / (float)filesize));
		evt_time = (((timems + evt_time) > duration) ? duration : (timems + evt_time));
	}

	return evt_time;
}

static off_t _get_evt_posbytime(PVREvtContext *evt, int64_t timems)
{
	off_t evt_pos = 0;
	int64_t evt_time = timems;

	if (evt_time > 0) {
		off_t pos = 0;
		int64_t duration = pvr_parser_get_duration(evt->parser, PVRFILE_EXIST);
		off_t   filesize = pvr_parser_get_filesize(evt->parser, PVRFILE_EXIST);

		if (duration > 0)
			pos = (int64_t)(evt_time * ((float)filesize / (float)duration));
		evt_pos = (((pos + evt_pos) > filesize) ? filesize : (pos + evt_pos));
	}

	return evt_pos;
}

static void _alloc_evt_info(PVREvtContext *evt, GxRecordPVREvent *event)
{
	PVRParser *parser = evt->parser;
	GxRecordPVRFileElem *segment_elem = NULL;
	unsigned int segment_count = 0;
	char *segment_url0 = evt->tmpbuf0;
	char *segment_url1 = evt->tmpbuf1;
	unsigned int i = 0;

	event->desc          = pvr_parser_get_arg(parser, PVRFILE_GET_DESC);
	event->start_time    = pvr_parser_get_arg(parser, PVRFILE_GET_START_TIME);
	event->end_time      = pvr_parser_get_arg(parser, PVRFILE_GET_END_TIME);
	event->chapter       = pvr_parser_get_arg(parser, PVRFILE_GET_CHAPTER);
	event->chapter_desc  = pvr_parser_get_arg(parser, PVRFILE_GET_CHAPTER_DESC);
	event->filesize      = pvr_parser_get_filesize(parser, PVRFILE_EXIST);
	event->duration      = pvr_parser_get_duration(parser, PVRFILE_EXIST);

	segment_count = pvr_parser_get_seqcount(parser);
	if (segment_count <= 0) {
		event->segment_count = 0;
		event->segment_elem  = NULL;
		return;
	}

	segment_elem  = (GxRecordPVRFileElem *)av_malloc(segment_count * sizeof(GxRecordPVRFileElem));
	if (NULL == segment_elem) {
		gxloge("alloc event info error\n");
		event->segment_count = 0;
		event->segment_elem  = NULL;
		return;
	}

	pvr_parser_set_seqno(parser, 0);
	while (1) {
		int segno = pvr_parser_next_url(parser, segment_url0, PVR_MAX_LINE_LEN);

		if (segno < 0)
			break;
		if (i >= segment_count)
			break;
		snprintf(segment_url1, PVR_MAX_URL_LEN, "%s/%s", evt->path, segment_url0);
		pvr_get_absolute_url(segment_url1, PVR_MAX_URL_LEN);
		segment_elem[i].file   = av_strdup(segment_url1);
		segment_elem[i].prefix = pvr_parse_prefix(segment_url1, PVR_SEGFILE_SUFFIX);
		i++;
	}
	event->segment_count = i;
	event->segment_elem  = segment_elem;

	return;
}

static void _free_evt_info(GxRecordPVREvent *event)
{
	if (NULL != event->segment_elem) {
		unsigned int i = 0;

		for (i = 0; i < event->segment_count; i++) {
			if (event->segment_elem[i].prefix)
				av_free(event->segment_elem[i].prefix);
			if (event->segment_elem[i].file)
				av_free(event->segment_elem[i].file);
		}
		av_free(event->segment_elem);
		event->segment_elem = NULL;
	}
	event->segment_count = 0;
	return;
}

static int _get_seg_seqno(PVREvtContext *evt, char *segment_url)
{
	unsigned int segment_count = 0;
	unsigned int i = 0;
	char *segment_url0 = evt->tmpbuf1;
	char *segment_url1 = evt->tmpbuf2;
	int segno = -1;

	segment_count = pvr_parser_get_seqcount(evt->parser);
	pvr_parser_set_seqno(evt->parser, 0);
	while (1) {
		segno = pvr_parser_next_url(evt->parser, segment_url0, PVR_MAX_URL_LEN);

		if (segno < 0)
			break;
		if (i >= segment_count)
			break;
		snprintf(segment_url1, PVR_MAX_URL_LEN, "%s/%s", evt->path, segment_url0);
		pvr_get_absolute_url(segment_url1, PVR_MAX_URL_LEN);
		if (0 == strcmp(segment_url1, segment_url))
			break;
	}

	return segno;
}

static int _add_evt_seg(PVREvtContext *evt, GxRecordPVREvent *conf)
{
	unsigned int i = 0;

	if (1) {
		char *line = evt->tmpbuf0;

		if (conf->desc &&
				(NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_DESC))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_DESC, conf->desc);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (conf->chapter &&
				(NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_CHAPTER))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_CHAPTER, conf->chapter);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (conf->chapter_desc &&
				(NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_CHAPTER_DESC))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_CHAPTER_DESC, conf->chapter_desc);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (conf->start_time &&
				(NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_START_TIME))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_START_TIME, conf->start_time);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (conf->end_time &&
				(NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_END_TIME))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_END_TIME, conf->end_time);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_PATH)) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s%s\n", PVR_SEG_PATH, "../", PVR_PVRFILE_SEGPATH);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
		if (NULL == pvr_parser_get_arg(evt->parser, PVRFILE_GET_SUFFIX)) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_SUFFIX, PVR_SEGFILE_SUFFIX);
			pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
		}
	}

	for (i = 0; i < conf->segment_count; i++) {
		char *seg_file = evt->tmpbuf0;

		if (conf->segment_elem[i].file) {
			snprintf(seg_file, PVR_MAX_URL_LEN, "%s", conf->segment_elem[i].file);
		} else if (conf->segment_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(evt->parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(evt->parser, PVRFILE_GET_SUFFIX);

			snprintf(seg_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", evt->path, path, conf->segment_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(seg_file, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt W]: %s\n", seg_file));
		if (0 == pvr_file_exist(seg_file)) {
			gxloge(" [%s] segment not exist\n", seg_file);
			return -1;
		}
		if (-1 != _get_seg_seqno(evt, seg_file)) {
			gxlogi(" [%s] segment already exist in event file\n", seg_file);
			return -1;
		}
	}

	for (i = 0; i < conf->segment_count; i++) {
		char *seg_file = evt->tmpbuf0;
		char *seg_xurl = evt->tmpbuf1;
		char *line     = evt->tmpbuf0;
		off_t filesize = 0, duration = 0;
		PVRParser *seg_parser = NULL;

		if (conf->segment_elem[i].file) {
			snprintf(seg_file, PVR_MAX_URL_LEN, "%s", conf->segment_elem[i].file);
		} else if (conf->segment_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(evt->parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(evt->parser, PVRFILE_GET_SUFFIX);

			snprintf(seg_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", evt->path, path, conf->segment_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(seg_file, PVR_MAX_URL_LEN);

		seg_parser = pvr_parser_alloc((const char *)seg_file);
		if (seg_parser == NULL) {
			gxloge(" [%s] segment parser alloc failed\n", seg_file);
			return -1;
		}
		pvr_get_relative_url(evt->file, seg_file, seg_xurl);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_XURL, seg_xurl);
		pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);

		filesize = pvr_parser_get_filesize(seg_parser, PVRFILE_EXIST);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, filesize);
		pvr_parser_write_line(evt->parser, line, PVRFILE_OVERLAP);

		duration = pvr_parser_get_duration(seg_parser, PVRFILE_EXIST);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, duration);
		pvr_parser_write_line(evt->parser, line, PVRFILE_APPEND);
		pvr_parser_free(seg_parser);
	}

	return 0;
}

static int _remove_evt_seg(PVREvtContext *evt, GxRecordPVREvent *conf)
{
	unsigned int i = 0;

	for (i = 0; i < conf->segment_count; i++) {
		char *seg_file = evt->tmpbuf0;

		if (conf->segment_elem[i].file) {
			snprintf(seg_file, PVR_MAX_URL_LEN, "%s", conf->segment_elem[i].file);
		} else if (conf->segment_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(evt->parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(evt->parser, PVRFILE_GET_SUFFIX);

			snprintf(seg_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", evt->path, path, conf->segment_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(seg_file, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt W]: %s\n", seg_file));
		if (-1 == _get_seg_seqno(evt, seg_file)) {
			gxlogi(" [%s] segment not exist in event file\n", seg_file);
			return -1;
		}
	}

	for (i = 0; i < conf->segment_count; i++) {
		char *seg_file = evt->tmpbuf0;
		char *line     = evt->tmpbuf0;
		int seqno = 0;

		if (conf->segment_elem[i].file) {
			snprintf(seg_file, PVR_MAX_URL_LEN, "%s", conf->segment_elem[i].file);
		} else if (conf->segment_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(evt->parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(evt->parser, PVRFILE_GET_SUFFIX);

			snprintf(seg_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", evt->path, path, conf->segment_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(seg_file, PVR_MAX_URL_LEN);
		seqno = _get_seg_seqno(evt, seg_file);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_SEG_UNVAILD, seqno);
		pvr_parser_write_line(evt->parser, line, PVRFILE_NORMAL);
	}

	return 0;
}

static void *evt_open(char *url, mode_t mode)
{
	PVREvtContext *evt = NULL;
	int path_size = 0;

	evt = av_mallocz(sizeof(PVREvtContext));
	if (!evt) {
		gxloge(" malloc fail\n");
		return NULL;
	}

	evt->path = pvr_parse_path(url, &path_size);
	if (!evt->path) {
		gxloge(" [%s] path parse fail\n", url);
		goto evt_open_error;
	}

	if (PVR_IS_WROOT(mode)) {
		if (!pvr_file_exist(url)) {
			evt->mkdir = pvr_mk_dir(evt->path);
			if (evt->mkdir < 0) {
				gxloge(" [%s] mkdir fail\n", url);
				goto evt_open_error;
			}
		}
		evt->write_flag = 1;
	}
	evt->file = av_strdup(url);

	evt->parser = pvr_parser_alloc((const char *)url);
	if (NULL == evt->parser) {
		gxloge(" [%s] evt parser alloc fail\n", url);
		goto evt_open_error;
	}
	evt->seg_no = -1;
	_clear_evt_seg_rw(&evt->seg_rw);
	GxCore_MutexCreate(&evt->mutex);
	return (void *)evt;

evt_open_error:
	if (evt) {
		if (evt->parser)
			pvr_parser_free(evt->parser);
		if (evt->path) {
			if (evt->mkdir > 0)
				pvr_rm_dir(evt->path);
			av_free(evt->path);
		}
		if (evt->file)
			av_free(evt->file);
		av_free(evt);
	}
	return NULL;
}

static int evt_close(void *priv)
{
	unsigned int i = 0;
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);

	GxCore_MutexLock(evt->mutex);
	if (evt->parser)
		pvr_parser_free(evt->parser);
	evt->seg_no = -1;

	_clear_evt_seg_rw(&evt->seg_rw);

	if (evt->path)
		av_free(evt->path);
	if (evt->file)
		av_free(evt->file);

	GxCore_MutexUnlock(evt->mutex);
	GxCore_MutexDelete(evt->mutex);
	av_free(evt);

	return 0;
}

static int evt_read(void *priv, uint8_t *buffer, size_t len)
{
	int ret = 0;
	int seqno = -1;
	PVREvtContext *evt = (PVREvtContext *)priv;
	unsigned char *inbuf = NULL;
	unsigned int   inlen = 0;
	unsigned int   rlen  = 0;

	PVR_EVT_CHECK_NULL(evt);
	PVR_EVT_CHECK_NULL(evt->parser);

	inbuf = buffer;
	inlen = len;
	GxCore_MutexLock(evt->mutex);
evt_read_retry1:
	if (NULL == evt->seg_rw.parser) {
		char *url0 = evt->tmpbuf0;
		char *url1 = evt->tmpbuf1;

		seqno = pvr_parser_next_url(evt->parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			GxCore_MutexUnlock(evt->mutex);
			return 0;
		}
		evt->seg_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", evt->path, url1);
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url0));
		evt->seg_rw.parser = pvr_parser_alloc((const char *)url0);
		if (NULL == evt->seg_rw.parser) {
			gxloge(" [%s] seg parser alloc fail\n", url0);
			GxCore_MutexUnlock(evt->mutex);
			return -1;
		}
	}
evt_read_retry2:
	if (NULL == evt->seg_rw.ts_fd) {
		char *url0 = evt->tmpbuf0;
		char *url1 = evt->tmpbuf1;
		char *segpath = pvr_parser_get_curpath(evt->seg_rw.parser);

		seqno = pvr_parser_next_url(evt->seg_rw.parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			_clear_evt_seg_rw(&evt->seg_rw);
			goto evt_read_retry1;
		}
		evt->seg_rw.ts_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", segpath, url1);
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url0));
		evt->seg_rw.ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == evt->seg_rw.ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			GxCore_MutexUnlock(evt->mutex);
			return -1;
		}
	}
	ret = recfile_read(evt->seg_rw.ts_fd, inbuf + rlen, inlen - rlen);
	if (ret < 0) {
		gxloge("read fail !!!!\n");
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	rlen += ret;
	if (rlen < len) {
		recfile_close(evt->seg_rw.ts_fd);
		evt->seg_rw.ts_fd = NULL;
		if (ret == 0)
			goto evt_read_retry2;
	}
	evt->desc_key = pvr_parser_get_keydesc_byseqno(evt->seg_rw.parser, evt->seg_rw.ts_no);
	GxCore_MutexUnlock(evt->mutex);

	return rlen;
}

static off_t evt_seek(void *priv, off_t offset)
{
	int seqno = -1;
	PVREvtContext *evt = (PVREvtContext *)priv;
	off_t evt_pos = 0, seg_pos = 0, ts_pos = 0;

	PVR_EVT_CHECK_NULL(evt);
	PVR_EVT_CHECK_NULL(evt->parser);

	GxCore_MutexLock(evt->mutex);
	//find segpos in evtfile
	evt_pos = offset;
	seqno = pvr_parser_get_seqno_bypos(evt->parser, evt_pos);
	if (-1 == seqno) {
		if (evt_pos == 0) {
			GxCore_MutexUnlock(evt->mutex);
			return evt_pos;
		} else {
			gxloge(" [%lld] seqno not find in evt parser\n", evt_pos);
			goto evt_seek_error;
		}
	} else {//check seqno exist or not
		int32_t tmp_seqno = pvr_parser_get_seqno_byseqno(evt->parser, seqno);
		if (-1 == tmp_seqno) {
			gxloge(" [%lld] seqno not find in evt parser\n", evt_pos);
			goto evt_seek_error;
		}
		if (seqno < tmp_seqno) {
			seqno = tmp_seqno;
			evt_pos = pvr_parser_get_startpos_byseqno(evt->parser, seqno, PVRFILE_EXIST);
		}
	}
	if (seqno != evt->seg_no)
		_clear_evt_seg_rw(&evt->seg_rw);

	if (NULL == evt->seg_rw.parser) {
		char *url0 = evt->tmpbuf0;
		char *url1 = evt->tmpbuf1;

		evt->seg_no = pvr_parser_get_url_byseqno(evt->parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", evt->path, url1);
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url0));
		evt->seg_rw.parser = pvr_parser_alloc(url0);
		if (NULL == evt->seg_rw.parser) {
			gxloge(" [%s] seg parser alloc fail\n", url0);
			goto evt_seek_error;
		}
	}
	seg_pos = evt_pos - pvr_parser_get_startpos_byseqno(evt->parser, seqno, PVRFILE_EXIST);
	if (seg_pos < 0) {
		gxloge(" [%d] startpos not find in evt parser\n", seqno);
		goto evt_seek_error;
	}
	//find tspos in segfile
	seqno = pvr_parser_get_seqno_bypos(evt->seg_rw.parser, seg_pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in seg parser\n", seg_pos);
		goto evt_seek_error;
	}
	if (seqno != evt->seg_rw.ts_no) {
		if (evt->seg_rw.ts_fd)
			recfile_close(evt->seg_rw.ts_fd);
		evt->seg_rw.ts_fd = NULL;
	}
	if (NULL == evt->seg_rw.ts_fd) {
		char *url0 = evt->tmpbuf0;
		char *url1 = evt->tmpbuf1;
		char *segpath = pvr_parser_get_curpath(evt->seg_rw.parser);

		evt->seg_rw.ts_no = pvr_parser_get_url_byseqno(evt->seg_rw.parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", segpath, url1);
		pvr_get_absolute_url(url0, PVR_MAX_URL_LEN);
		PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt S]: %s\n", url0));
		evt->seg_rw.ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == evt->seg_rw.ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			goto evt_seek_error;
		}
	}
	ts_pos = seg_pos - pvr_parser_get_startpos_byseqno(evt->seg_rw.parser, seqno, PVRFILE_EXIST);
	if (ts_pos < 0) {
		gxloge("[%d] startpos not find in seg parser\n", seqno);
		goto evt_seek_error;
	}
	recfile_seek(evt->seg_rw.ts_fd, ts_pos, SEEK_SET);
	GxCore_MutexUnlock(evt->mutex);
	return offset;

evt_seek_error:
	GxCore_MutexUnlock(evt->mutex);
	return -1;
}

static int evt_getinfo(void *priv, PVRInfo *info)
{
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);
	PVR_EVT_CHECK_NULL(evt->parser);

	GxCore_MutexLock(evt->mutex);
	_get_evt_info(evt, info);
	GxCore_MutexUnlock(evt->mutex);

	return 0;
}

static off_t evt_getpos(void *priv)
{
	off_t evt_pos = 0;
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);
	PVR_EVT_CHECK_NULL(evt->parser);

	GxCore_MutexLock(evt->mutex);
	if (evt->seg_no >= 0) {
		evt_pos += pvr_parser_get_startpos_byseqno(evt->parser, evt->seg_no, PVRFILE_EXIST);
		if (evt->seg_rw.parser && (evt->seg_rw.ts_no >= 0)) {
			evt_pos += pvr_parser_get_startpos_byseqno(evt->seg_rw.parser, evt->seg_rw.ts_no, PVRFILE_EXIST);
			if (evt->seg_rw.ts_fd)
				evt_pos += recfile_getpos(evt->seg_rw.ts_fd);
		}
	}
	GxCore_MutexUnlock(evt->mutex);

	return evt_pos;
}

static size_t evt_probehdr(void *priv)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);

	GxCore_MutexLock(evt->mutex);
	url = evt->tmpbuf0;
	if (evt->seg_rw.parser) {
		char *segpath = pvr_parser_get_curpath(evt->seg_rw.parser);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", segpath, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	} else {
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", evt->path, PVR_EVT_TO_HDR_PATH, PVR_HDRFILE);
	}
	PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt P]: %s\n", url));
	pvr_get_absolute_url(url, PVR_MAX_URL_LEN);
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(evt->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_getsize(fd);
	recfile_close(fd);
	GxCore_MutexUnlock(evt->mutex);

	return size;
}

static size_t evt_readhdr(void *priv, uint8_t *buffer, size_t len)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);

	GxCore_MutexLock(evt->mutex);
	url = evt->tmpbuf0;
	if (evt->seg_rw.parser) {
		char *segpath = pvr_parser_get_curpath(evt->seg_rw.parser);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", segpath, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	} else {
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", evt->path, PVR_EVT_TO_HDR_PATH, PVR_HDRFILE);
	}
	pvr_get_absolute_url(url, PVR_MAX_URL_LEN);
	PVR_EVT_DEBUG_PRINT(gxlogi("--> [pvr evt R]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(evt->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_read(fd, buffer, len);
	recfile_close(fd);
	GxCore_MutexUnlock(evt->mutex);

	return size;
}

static int evt_setctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVREvtContext *evt = (PVREvtContext *)priv;

	GxCore_MutexLock(evt->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_ADD_EVENT_LIST:
		{
			ret = _add_evt_seg(evt, (GxRecordPVREvent *)ctrl->arg);
		}
		break;
	case GX_RECORD_PVR_REMOVE_EVENT_LIST:
		{
			ret = _remove_evt_seg(evt, (GxRecordPVREvent *)ctrl->arg);
		}
		break;
	case GX_RECORD_PVR_FREE_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;

			_free_evt_info(&info->event);
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
	GxCore_MutexUnlock(evt->mutex);

	return ret;
}

static int evt_getctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVREvtContext *evt = (PVREvtContext *)priv;

	PVR_EVT_CHECK_NULL(evt);
	PVR_EVT_CHECK_NULL(evt->parser);

	GxCore_MutexLock(evt->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_GET_DURATION:
	case GX_RECORD_PVR_GET_FILESIZE:
		{
			PVRInfo info;
			_get_evt_info(evt, &info);
			if (ctrl->opt == GX_RECORD_PVR_GET_DURATION)
				*(int64_t *)ctrl->arg = info.tr_duration;
			else if (ctrl->opt == GX_RECORD_PVR_GET_FILESIZE)
				*(int64_t *)ctrl->arg = info.tr_filesize;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_CURTIME:
		{
			*(int64_t *)ctrl->arg = _get_evt_curtime(evt);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_POS_BY_TIME:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->offset = _get_evt_posbytime(evt, time_pos->timems);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_TIME_BY_POS:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->timems = _get_evt_timebypos(evt, time_pos->offset);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;
			char *pvr_path = NULL;
			char *pvr_file = evt->tmpbuf0;
			int path_size = 0;

			info->type = GX_RECORD_PVR_EVT_FILE;
			_alloc_evt_info(evt, &info->event);

			pvr_path = pvr_parse_path(evt->path, &path_size);
			pvr_get_absolute_url(pvr_path, path_size);
			snprintf(pvr_file, PVR_MAX_URL_LEN, "%s%s", pvr_path, PVR_PVRFILE_SUFFIX);
			av_free(pvr_path);
			info->pvr_file = av_strdup(pvr_file);
			info->dir      = NULL;
			info->prefix   = pvr_parse_prefix(evt->file, PVR_EVTFILE_SUFFIX);
			info->path     = av_strdup(evt->path);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_TYPE:
		{
			GxRecordPVRFileType *type = (GxRecordPVRFileType *)ctrl->arg;

			*type = GX_RECORD_PVR_EVT_FILE;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_KEY_DESC:
		{
			ctrl->arg = (void *)evt->desc_key;
			ret = 0;
		}
		break;
	default:
		gxloge(" [%d] evt option not find\n", ctrl->opt);
		break;
	}
	GxCore_MutexUnlock(evt->mutex);

	return ret;
}

PVROps pvr_evt_ops = {
	.open         = evt_open,
	.close        = evt_close,
	.write        = NULL,
	.read         = evt_read,
	.seek         = evt_seek,
	.getinfo      = evt_getinfo,
	.getpos       = evt_getpos,
	.settime      = NULL,
	.probehdr     = evt_probehdr,
	.readhdr      = evt_readhdr,
	.writehdr     = NULL,
	.setctrl      = evt_setctrl,
	.getctrl      = evt_getctrl,
};
