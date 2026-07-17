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
} PVRLstSegRW;

typedef struct {
	PVRParser          *parser;
	int                 seg_no;
} PVRLstEvtRW;

typedef struct {
	PVRParser          *parser;
	int                 evt_no;
} PVRLstLstRW;

typedef struct {
	PVRParser          *parser;
	int                 evt_no;
} PVRLstEvtMG;

typedef struct {
	PVRLstLstRW           lst_rw;
	PVRLstEvtRW           evt_rw;
	PVRLstSegRW           seg_rw;
	char                 *path;
	char                 *file;
	char                  tmpbuf0[PVR_MAX_BUF_SIZE];
	char                  tmpbuf1[PVR_MAX_BUF_SIZE];
	char                  tmpbuf2[PVR_MAX_BUF_SIZE];
	handle_t              mutex;
	const char           *desc_key;
} PVRLstContext;

#define PVR_LST_DEBUG_PRINT(printf_func) do {                \
	int debug_flags = 0;                                     \
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_PVR, &debug_flags); \
	if (debug_flags)                                         \
		printf_func;                                         \
} while(0)

#define PVR_LST_CHECK_NULL(ptr) do {           \
	if (NULL == ptr) {                         \
		gxloge(" ptr is NULL\n");              \
		return -1;                             \
	}                                          \
} while(0)

static void _clear_lst_seg_rw(PVRLstSegRW *rw)
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

static void _clear_lst_evt_rw(PVRLstEvtRW *rw)
{
	if (rw->parser) {
		pvr_parser_free(rw->parser);
		rw->parser = NULL;
	}

	rw->seg_no = -1;

	return;
}

static void _update_lstfile(PVRLstContext *lst)
{
	int evtno = 0;
	int evtcn = pvr_parser_get_seqcount(lst->lst_rw.parser);

	for (evtno = 0; evtno < evtcn; evtno++) {
		if (pvr_parser_get_seqno_byseqno(lst->lst_rw.parser, evtno) == -1)
			continue;
		if (pvr_parser_get_isfinish_byseqno(lst->lst_rw.parser, evtno) == 0) {
			PVRParser *evt_parser = NULL;
			char *url0 = lst->tmpbuf0;
			char *url1 = lst->tmpbuf1;
			int is_finish = 1;
			int64_t duration = 0;
			off_t filesize = 0;

			pvr_parser_get_url_byseqno(lst->lst_rw.parser, evtno, url1, PVR_MAX_URL_LEN);
			snprintf(url0, PVR_MAX_LINE_LEN, "%s/%s", lst->path, url1);
			pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
			PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst update]: %s\n", url0));
			evt_parser = pvr_parser_alloc(url0);
			if (NULL == evt_parser) {
				gxloge(" [%s] event file open fail\n", url0);
				continue;
			}

			duration += pvr_parser_get_duration(evt_parser, PVRFILE_EXIST);
			filesize += pvr_parser_get_filesize(evt_parser, PVRFILE_EXIST);

			//fix: last segment in file is not finish, must update list
			if (1) {
				PVRParser *seg_parser = NULL;
				char *path = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH);
				int segcn  = pvr_parser_get_seqcount(evt_parser);

				url1[0] = '\0';
				pvr_parser_get_url_byseqno(evt_parser, segcn - 1, url1, PVR_MAX_URL_LEN);
				if (url1[0] != '\0') {
					snprintf(url0, PVR_MAX_URL_LEN, "%s/%s/%s", lst->path, path, url1);
					pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
					PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst update]: %s\n", url0));
					seg_parser = pvr_parser_alloc(url0);
					if (NULL != seg_parser) {
						is_finish = pvr_parser_get_isfinish(seg_parser);
						pvr_parser_free(seg_parser);
					}
				}
			}
			pvr_parser_set_isfinish_byseqno(lst->lst_rw.parser, evtno, is_finish);
			pvr_parser_free(evt_parser);

			pvr_parser_set_duration_byseqno(lst->lst_rw.parser, evtno, duration);
			pvr_parser_set_filesize_byseqno(lst->lst_rw.parser, evtno, filesize);
		}
	}

	return;
}

static void _get_lst_info(PVRLstContext *lst, PVRInfo *info)
{
	info->tl_filesize = pvr_parser_get_filesize(lst->lst_rw.parser, PVRFILE_TOTAL);
	info->tr_filesize = pvr_parser_get_filesize(lst->lst_rw.parser, PVRFILE_EXIST);
	info->tl_duration = pvr_parser_get_duration(lst->lst_rw.parser, PVRFILE_TOTAL);
	info->tr_duration = pvr_parser_get_duration(lst->lst_rw.parser, PVRFILE_EXIST);
	return;
}

static int64_t _get_lst_curtime(PVRLstContext *lst)
{
	int64_t lst_time = 0;

	if (lst->lst_rw.evt_no >= 0)
		lst_time += pvr_parser_get_starttime_byseqno(lst->lst_rw.parser, lst->lst_rw.evt_no, PVRFILE_EXIST);
	else
		return lst_time;

	if (lst->evt_rw.parser && (lst->evt_rw.seg_no >= 0))
		lst_time += pvr_parser_get_starttime_byseqno(lst->evt_rw.parser, lst->evt_rw.seg_no, PVRFILE_EXIST);
	else
		return lst_time;

	if (lst->seg_rw.parser && (lst->seg_rw.ts_no >= 0)) {
		lst_time += pvr_parser_get_starttime_byseqno(lst->seg_rw.parser, lst->seg_rw.ts_no, PVRFILE_EXIST);
		if (lst->seg_rw.ts_fd) {
			int64_t timems = 0;
			off_t   offset = recfile_getpos(lst->seg_rw.ts_fd);
			int64_t duration = pvr_parser_get_duration(lst->lst_rw.parser, PVRFILE_EXIST);
			int64_t seg_time = pvr_parser_get_duration(lst->seg_rw.parser, PVRFILE_EXIST);
			off_t   seg_size = pvr_parser_get_filesize(lst->seg_rw.parser, PVRFILE_EXIST);

			if (seg_size > 0)
				timems = (int64_t)(offset * ((float)seg_time / (float)seg_size));
			lst_time = (((timems + lst_time) > duration) ? duration : (timems + lst_time));
		}
	}

	return lst_time;
}

static int64_t _get_lst_timebypos(PVRLstContext *lst, off_t pos)
{
	int seqno = -1;
	off_t lst_offset = 0;
	int64_t lst_time = 0;

	seqno = pvr_parser_get_seqno_bypos(lst->lst_rw.parser, pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in pvr parser\n", seqno);
		return -1;
	}
	lst_time  += pvr_parser_get_starttime_byseqno(lst->lst_rw.parser, seqno, PVRFILE_EXIST);
	lst_offset = pvr_parser_get_startpos_byseqno (lst->lst_rw.parser, seqno, PVRFILE_EXIST);
	lst_offset = (pos - lst_offset);
	if (lst_offset > 0) {
		int64_t timems = 0;
		int64_t duration = pvr_parser_get_duration(lst->lst_rw.parser, PVRFILE_EXIST);
		off_t   filesize = pvr_parser_get_filesize(lst->lst_rw.parser, PVRFILE_EXIST);

		if (filesize > 0)
			timems = (int64_t)(lst_offset * ((float)duration / (float)filesize));
		lst_time = (((timems + lst_time) > duration) ? duration : (timems + lst_time));
	}

	return lst_time;
}

static off_t _get_lst_posbytime(PVRLstContext *lst, int64_t timems)
{
	int seqno = -1;
	off_t lst_pos = 0;
	int64_t lst_time = 0;

	seqno = pvr_parser_get_seqno_bytime(lst->lst_rw.parser, timems);
	if (-1 == seqno) {
		gxlogi(" [%lld] seqno not find in lst parser\n", timems);
		return -1;
	}
	lst_pos += pvr_parser_get_startpos_byseqno (lst->lst_rw.parser, seqno, PVRFILE_EXIST);
	lst_time = pvr_parser_get_starttime_byseqno(lst->lst_rw.parser, seqno, PVRFILE_EXIST);
	lst_time = (timems - lst_time);
	if (lst_time > 0) {
		off_t pos = 0;
		int64_t duration = pvr_parser_get_duration(lst->lst_rw.parser, PVRFILE_EXIST);
		off_t   filesize = pvr_parser_get_filesize(lst->lst_rw.parser, PVRFILE_EXIST);

		if (duration > 0)
			pos = (int64_t)(lst_time * ((float)filesize / (float)duration));
		lst_pos = (((pos + lst_pos) > filesize) ? filesize : (pos + lst_pos));
	}

	return lst_pos;
}

static void _alloc_lst_info(PVRLstContext *lst, GxRecordPVRList *list)
{
	PVRParser *parser = lst->lst_rw.parser;
	GxRecordPVRFileElem *event_elem = NULL;
	unsigned int event_count = 0;
	char *event_url0  = lst->tmpbuf0;
	char *event_url1  = lst->tmpbuf1;
	unsigned int i = 0;

	list->desc  = pvr_parser_get_arg(parser, PVRFILE_GET_DESC);
	event_count = pvr_parser_get_seqcount(parser);
	if (event_count <= 0) {
		list->event_count = 0;
		list->event_elem  = NULL;
		return;
	}

	event_elem = (GxRecordPVRFileElem *)av_malloc(event_count * sizeof(GxRecordPVRFileElem));
	if (NULL == event_elem) {
		gxloge("alloc list info error\n");
		list->event_count = 0;
		list->event_elem  = NULL;
		return;
	}

	pvr_parser_set_seqno(parser, 0);
	while (1) {
		int evtno = pvr_parser_next_url(parser, event_url0, PVR_MAX_LINE_LEN);

		if (evtno < 0)
			break;
		if (i >= event_count)
			break;
		snprintf(event_url1, PVR_MAX_URL_LEN, "%s/%s", lst->path, event_url0);
		pvr_get_absolute_url(event_url1, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst alloc]: %s\n", event_url1));
		event_elem[i].file   = av_strdup(event_url1);
		event_elem[i].prefix = pvr_parse_prefix(event_url1, PVR_EVTFILE_SUFFIX);
		i++;
	}
	list->event_count = i;
	list->event_elem  = event_elem;

	return;
}

static void _free_lst_info(GxRecordPVRList *list)
{
	if (NULL != list->event_elem) {
		unsigned int i = 0;

		for (i = 0; i < list->event_count; i++) {
			if (list->event_elem[i].prefix)
				av_free(list->event_elem[i].prefix);
			if (list->event_elem[i].file)
				av_free(list->event_elem[i].file);
		}
		av_free(list->event_elem);
		list->event_elem = NULL;
	}
	list->event_count = 0;

	return;
}

static int _get_evt_seqno(PVRLstContext *lst, char *event_url)
{
	unsigned int event_count = 0;
	unsigned int i = 0;
	char *event_url0 = lst->tmpbuf1;
	char *event_url1 = lst->tmpbuf2;
	int segno = -1;

	event_count = pvr_parser_get_seqcount(lst->lst_rw.parser);
	pvr_parser_set_seqno(lst->lst_rw.parser, 0);
	while (1) {
		segno = pvr_parser_next_url(lst->lst_rw.parser, event_url0, PVR_MAX_URL_LEN);

		if (segno < 0)
			break;
		if (i >= event_count)
			break;
		snprintf(event_url1, PVR_MAX_URL_LEN, "%s/%s", lst->path, event_url0);
		pvr_get_absolute_url(event_url1, PVR_MAX_LINE_LEN);
		if (0 == strcmp(event_url1, event_url))
			break;
	}

	return segno;
}

static int _add_lst_evt(PVRLstContext *lst, GxRecordPVRList *conf)
{
	unsigned int i = 0;

	if (1) {
		char *line = lst->tmpbuf0;

		if (conf->desc &&
				(NULL == pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_DESC))) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_LST_DESC, conf->desc);
			pvr_parser_write_line(lst->lst_rw.parser, line, PVRFILE_NORMAL);
		}
		if (NULL == pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH)) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s%s\n", PVR_EVT_PATH, "../", PVR_PVRFILE_EVTPATH);
			pvr_parser_write_line(lst->lst_rw.parser, line, PVRFILE_NORMAL);
		}
		if (NULL == pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_SUFFIX)) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_SUFFIX, PVR_EVTFILE_SUFFIX);
			pvr_parser_write_line(lst->lst_rw.parser, line, PVRFILE_NORMAL);
		}
	}

	for (i = 0; i < conf->event_count; i++) {
		char *evt_file = lst->tmpbuf0;

		if (conf->event_elem[i].file) {
			snprintf(evt_file, PVR_MAX_URL_LEN, "%s", conf->event_elem[i].file);
		} else if (conf->event_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_SUFFIX);

			snprintf(evt_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", lst->path, path, conf->event_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(evt_file, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst W]: %s\n", evt_file));
		if (0 == pvr_file_exist(evt_file)) {
			gxloge(" [%s] event not exist\n", evt_file);
			return -1;
		}
		if (-1 != _get_evt_seqno(lst, evt_file)) {
			gxloge(" [%s] event already exist in event file\n", evt_file);
			return -1;
		}
	}

	for (i = 0; i < conf->event_count; i++) {
		char *evt_file = lst->tmpbuf0;
		char *evt_xurl = lst->tmpbuf1;
		char *line     = lst->tmpbuf0;
		off_t filesize = 0, duration = 0;

		if (conf->event_elem[i].file) {
			snprintf(evt_file, PVR_MAX_URL_LEN, "%s", conf->event_elem[i].file);
		} else if (conf->event_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_SUFFIX);

			snprintf(evt_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", lst->path, path, conf->event_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(evt_file, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr evt W]: %s\n", evt_file));

		pvr_get_relative_url(lst->file, evt_file, evt_xurl);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_XURL, evt_xurl);
		pvr_parser_write_line(lst->lst_rw.parser, line, PVRFILE_NORMAL);
	}

	return 0;
}

static int _remove_lst_evt(PVRLstContext *lst, GxRecordPVRList *conf)
{
	unsigned int i = 0;

	for (i = 0; i < conf->event_count; i++) {
		char *evt_file = lst->tmpbuf0;

		if (conf->event_elem[i].file) {
			snprintf(evt_file, PVR_MAX_URL_LEN, "%s", conf->event_elem[i].file);
		} else if (conf->event_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_SUFFIX);

			snprintf(evt_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", lst->path, path, conf->event_elem[i].prefix, suffix);
		}
		pvr_get_absolute_url(evt_file, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst W]: %s\n", evt_file));
		if (-1 == _get_evt_seqno(lst, evt_file)) {
			gxloge(" [%s] event not exist in event file\n", evt_file);
			return -1;
		}
	}

	for (i = 0; i < conf->event_count; i++) {
		char *evt_file = lst->tmpbuf0;
		char *line     = lst->tmpbuf0;
		int seqno = 0;

		if (conf->event_elem[i].file) {
			snprintf(evt_file, PVR_MAX_URL_LEN, "%s", conf->event_elem[i].file);
		} else if (conf->event_elem[i].prefix) {
			char *path   = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_PATH);
			char *suffix = pvr_parser_get_arg(lst->lst_rw.parser, PVRFILE_GET_SUFFIX);

			snprintf(evt_file, PVR_MAX_URL_LEN, "%s/%s/%s%s", lst->path, path, conf->event_elem[i].prefix, suffix);
		}
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst W]: %s\n", evt_file));
		pvr_get_absolute_url(evt_file, PVR_MAX_LINE_LEN);
		seqno = _get_evt_seqno(lst, evt_file);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_EVT_UNVAILD, seqno);
		pvr_parser_write_line(lst->lst_rw.parser, line, PVRFILE_NORMAL);
	}

	return 0;
}

static void *lst_open(char *url, mode_t mode)
{
	int path_size = 0;
	PVRLstContext *lst = NULL;

	lst = av_mallocz(sizeof(PVRLstContext));
	if (!lst) {
		gxloge(" malloc fail\n");
		return NULL;
	}

	lst->path = pvr_parse_path(url, &path_size);
	if (!lst->path) {
		gxloge(" [%s] path parse fail\n", url);
		goto lst_open_error;
	}
	lst->file = av_strdup(url);

	lst->lst_rw.parser = pvr_parser_alloc((const char *)url);
	if (NULL == lst->lst_rw.parser) {
		gxloge(" [%s] lst parser alloc fail\n", url);
		goto lst_open_error;
	}
	lst->lst_rw.evt_no = -1;
	_clear_lst_evt_rw(&lst->evt_rw);
	_clear_lst_seg_rw(&lst->seg_rw);
	_update_lstfile(lst);
	GxCore_MutexCreate(&lst->mutex);
	return (void *)lst;

lst_open_error:
	if (lst) {
		if (lst->lst_rw.parser)
			pvr_parser_free(lst->lst_rw.parser);
		if (lst->path)
			av_free(lst->path);
		if (lst->file)
			av_free(lst->file);
		av_free(lst);
	}
	return NULL;
}

static int lst_close(void *priv)
{
	unsigned int i = 0;
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);

	GxCore_MutexLock(lst->mutex);
	if (lst->lst_rw.parser)
		pvr_parser_free(lst->lst_rw.parser);
	lst->lst_rw.evt_no = -1;

	_clear_lst_evt_rw(&lst->evt_rw);
	_clear_lst_seg_rw(&lst->seg_rw);

	if (lst->path)
		av_free(lst->path);
	if (lst->file)
		av_free(lst->file);

	GxCore_MutexUnlock(lst->mutex);
	GxCore_MutexDelete(lst->mutex);
	av_free(lst);

	return 0;
}

static int lst_read(void *priv, uint8_t *buffer, size_t len)
{
	int ret = 0;
	int seqno = -1;
	PVRLstContext *lst = (PVRLstContext *)priv;
	char *key_desc = NULL;
	unsigned char *inbuf = NULL;
	unsigned int   inlen = 0;
	unsigned int   rlen  = 0;

	PVR_LST_CHECK_NULL(lst);
	PVR_LST_CHECK_NULL(lst->lst_rw.parser);

	inbuf = buffer;
	inlen = len;
	GxCore_MutexLock(lst->mutex);
	_update_lstfile(lst);
lst_read_retry0:
	if (NULL == lst->evt_rw.parser) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;

		seqno = pvr_parser_next_url(lst->lst_rw.parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			GxCore_MutexUnlock(lst->mutex);
			return 0;
		}
		lst->lst_rw.evt_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", lst->path, url1);
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst R]: %s\n", url0));
		lst->evt_rw.parser = pvr_parser_alloc((const char *)url0);
		if (NULL == lst->evt_rw.parser) {
			gxloge(" [%s] evt parser alloc fail\n", url0);
			GxCore_MutexUnlock(lst->mutex);
			return -1;
		}
	}
lst_read_retry1:
	if (NULL == lst->seg_rw.parser) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;
		char *evtpath = pvr_parser_get_curpath(lst->evt_rw.parser);

		seqno = pvr_parser_next_url(lst->evt_rw.parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			_clear_lst_evt_rw(&lst->evt_rw);
			goto lst_read_retry0;
		}
		lst->evt_rw.seg_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", evtpath, url1);
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst R]: %s\n", url0));
		lst->seg_rw.parser = pvr_parser_alloc((const char *)url0);
		if (NULL == lst->seg_rw.parser) {
			gxloge(" [%s] seg parser alloc fail\n", url0);
			GxCore_MutexUnlock(lst->mutex);
			return -1;
		}
	}
lst_read_retry2:
	if (NULL == lst->seg_rw.ts_fd) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;
		char *segpath = pvr_parser_get_curpath(lst->seg_rw.parser);

		seqno = pvr_parser_next_url(lst->seg_rw.parser, url1, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			_clear_lst_seg_rw(&lst->seg_rw);
			goto lst_read_retry1;
		}
		lst->seg_rw.ts_no = seqno;
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", segpath, url1);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst R]: %s\n", url0));
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		lst->seg_rw.ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == lst->seg_rw.ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			GxCore_MutexUnlock(lst->mutex);
			return -1;
		}
	}
	ret = recfile_read(lst->seg_rw.ts_fd, inbuf + rlen, inlen - rlen);
	if (ret < 0) {
		gxloge("read fail !!!!\n");
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	rlen += ret;
	if (rlen < len) {
		recfile_close(lst->seg_rw.ts_fd);
		lst->seg_rw.ts_fd = NULL;
		if (ret == 0)
			goto lst_read_retry2;
	}
	lst->desc_key = pvr_parser_get_keydesc_byseqno(lst->seg_rw.parser, lst->seg_rw.ts_no);
	GxCore_MutexUnlock(lst->mutex);

	return rlen;
}

static off_t lst_seek(void *priv, off_t offset)
{
	int seqno = -1;
	PVRLstContext *lst = (PVRLstContext *)priv;
	off_t lst_pos = 0, evt_pos = 0, seg_pos = 0, ts_pos = 0;

	PVR_LST_CHECK_NULL(lst);
	PVR_LST_CHECK_NULL(lst->lst_rw.parser);

	GxCore_MutexLock(lst->mutex);
	_update_lstfile(lst);
	lst_pos = offset;
	//find evtpos in lstfile
	seqno = pvr_parser_get_seqno_bypos(lst->lst_rw.parser, lst_pos);
	if (-1 == seqno) {
		if (lst_pos == 0) {
			GxCore_MutexUnlock(lst->mutex);
			return lst_pos;
		} else {
			gxloge(" [%lld] seqno not find in lst parser\n", lst_pos);
			goto lst_seek_error;
		}
	} else {//check seqno exist or not
		int32_t tmp_seqno = pvr_parser_get_seqno_byseqno(lst->lst_rw.parser, seqno);
		if (-1 == tmp_seqno) {
			gxloge(" [%lld] seqno not find in lst parser\n", lst_pos);
			goto lst_seek_error;
		}
		if (seqno < tmp_seqno) {
			seqno = tmp_seqno;
			lst_pos = pvr_parser_get_startpos_byseqno(lst->lst_rw.parser, seqno, PVRFILE_EXIST);
		}
	}

	if (seqno != lst->lst_rw.evt_no)
		_clear_lst_evt_rw(&lst->evt_rw);
	if (NULL == lst->evt_rw.parser) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;

		lst->lst_rw.evt_no = pvr_parser_get_url_byseqno(lst->lst_rw.parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", lst->path, url1);
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst S]: %s\n", url0));
		lst->evt_rw.parser = pvr_parser_alloc(url0);
		if (NULL == lst->evt_rw.parser) {
			gxloge(" [%s] evt parser alloc fail\n", url0);
			goto lst_seek_error;
		}
	}
	evt_pos = lst_pos - pvr_parser_get_startpos_byseqno(lst->lst_rw.parser, seqno, PVRFILE_EXIST);
	if (evt_pos < 0) {
		gxlogd(" [%d] startpos not find in lst parser\n", seqno);
		goto lst_seek_error;
	}
	//find segpos in evtfile
	seqno = pvr_parser_get_seqno_bypos(lst->evt_rw.parser, evt_pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in evt parser\n", evt_pos);
		goto lst_seek_error;
	} else {//check seqno exist or not
		int32_t tmp_seqno = pvr_parser_get_seqno_byseqno(lst->evt_rw.parser, seqno);
		if (-1 == tmp_seqno) {
			gxloge(" [%lld] seqno not find in lst parser\n", evt_pos);
			goto lst_seek_error;
		}
		if (seqno < tmp_seqno) {
			seqno = tmp_seqno;
			evt_pos = pvr_parser_get_startpos_byseqno(lst->evt_rw.parser, seqno, PVRFILE_EXIST);
		}
	}
	if (seqno != lst->evt_rw.seg_no)
		_clear_lst_seg_rw(&lst->seg_rw);
	if (NULL == lst->seg_rw.parser) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;
		char *evtpath = pvr_parser_get_curpath(lst->evt_rw.parser);

		lst->evt_rw.seg_no = pvr_parser_get_url_byseqno(lst->evt_rw.parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", evtpath, url1);
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst S]: %s\n", url0));
		lst->seg_rw.parser = pvr_parser_alloc(url0);
		if (NULL == lst->seg_rw.parser) {
			gxloge(" [%s] seg parser alloc fail\n", url0);
			goto lst_seek_error;
		}
	}
	seg_pos = evt_pos - pvr_parser_get_startpos_byseqno(lst->evt_rw.parser, seqno, PVRFILE_EXIST);
	if (seg_pos < 0) {
		gxloge(" [%d] startpos not find in evt parser\n", seqno);
		goto lst_seek_error;
	}
	//find tspos in segfile
	seqno = pvr_parser_get_seqno_bypos(lst->seg_rw.parser, seg_pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in seg parser\n", seg_pos);
		goto lst_seek_error;
	}
	if (seqno != lst->seg_rw.ts_no) {
		if (lst->seg_rw.ts_fd)
			recfile_close(lst->seg_rw.ts_fd);
		lst->seg_rw.ts_fd = NULL;
	}
	if (NULL == lst->seg_rw.ts_fd) {
		char *url0 = lst->tmpbuf0;
		char *url1 = lst->tmpbuf1;
		char *segpath = pvr_parser_get_curpath(lst->seg_rw.parser);

		lst->seg_rw.ts_no = pvr_parser_get_url_byseqno(lst->seg_rw.parser, seqno, url1, PVR_MAX_URL_LEN);
		snprintf(url0, PVR_MAX_URL_LEN, "%s/%s", segpath, url1);
		pvr_get_absolute_url(url0, PVR_MAX_LINE_LEN);
		PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst S]: %s\n", url0));
		lst->seg_rw.ts_fd = recfile_open(url0, PVR_RW_ROOT);
		if (NULL == lst->seg_rw.ts_fd) {
			gxloge(" [%s] open fail\n", url0);
			goto lst_seek_error;
		}
	}
	ts_pos = seg_pos - pvr_parser_get_startpos_byseqno(lst->seg_rw.parser, seqno, PVRFILE_EXIST);
	if (ts_pos < 0) {
		gxloge("[%d] startpos not find in seg parser\n", seqno);
		goto lst_seek_error;
	}
	recfile_seek(lst->seg_rw.ts_fd, ts_pos, SEEK_SET);
	GxCore_MutexUnlock(lst->mutex);
	return offset;

lst_seek_error:
	GxCore_MutexUnlock(lst->mutex);
	return -1;
}

static int lst_getinfo(void *priv, PVRInfo *info)
{
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);
	PVR_LST_CHECK_NULL(lst->lst_rw.parser);

	GxCore_MutexLock(lst->mutex);
	_update_lstfile(lst);
	_get_lst_info(lst, info);
	GxCore_MutexUnlock(lst->mutex);

	return 0;
}

static off_t lst_getpos(void *priv)
{
	off_t evt_pos = 0;
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);

	GxCore_MutexLock(lst->mutex);
	_update_lstfile(lst);
	if (lst->lst_rw.evt_no >= 0) {
		evt_pos += pvr_parser_get_startpos_byseqno(lst->lst_rw.parser, lst->lst_rw.evt_no, PVRFILE_EXIST);
		if (lst->evt_rw.parser && (lst->evt_rw.seg_no >= 0)) {
			evt_pos += pvr_parser_get_startpos_byseqno(lst->evt_rw.parser, lst->evt_rw.seg_no, PVRFILE_EXIST);
			if (lst->seg_rw.parser && (lst->seg_rw.ts_no >= 0)) {
				evt_pos += pvr_parser_get_startpos_byseqno(lst->seg_rw.parser, lst->seg_rw.ts_no, PVRFILE_EXIST);
				if (lst->seg_rw.ts_fd)
					evt_pos += recfile_getpos(lst->seg_rw.ts_fd);
			}
		}
	}
	GxCore_MutexUnlock(lst->mutex);

	return evt_pos;
}

static size_t lst_probehdr(void *priv)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);

	GxCore_MutexLock(lst->mutex);
	url = lst->tmpbuf0;
	if (lst->seg_rw.parser) {
		char *segpath = pvr_parser_get_curpath(lst->seg_rw.parser);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", segpath, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	} else
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s", lst->path, PVR_HDRFILE);
	pvr_get_absolute_url(url, PVR_MAX_LINE_LEN);
	PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst P]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(lst->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_getsize(fd);
	recfile_close(fd);
	GxCore_MutexUnlock(lst->mutex);

	return size;
}

static size_t lst_readhdr(void *priv, uint8_t *buffer, size_t len)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);

	GxCore_MutexLock(lst->mutex);
	url = lst->tmpbuf0;
	if (lst->seg_rw.parser) {
		char *segpath = pvr_parser_get_curpath(lst->seg_rw.parser);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", segpath, PVR_SEG_TO_HDR_PATH, PVR_HDRFILE);
	} else
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s", lst->path, PVR_HDRFILE);
	pvr_get_absolute_url(url, PVR_MAX_LINE_LEN);
	PVR_LST_DEBUG_PRINT(gxlogi("--> [pvr lst R]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(lst->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_read(fd, buffer, len);
	recfile_close(fd);
	GxCore_MutexUnlock(lst->mutex);

	return size;
}

static int lst_setctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRLstContext *lst = (PVRLstContext *)priv;

	GxCore_MutexLock(lst->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_ADD_EVENT_LIST:
		{
			ret = _add_lst_evt(lst, (GxRecordPVRList *)ctrl->arg);
		}
		break;
	case GX_RECORD_PVR_REMOVE_EVENT_LIST:
		{
			ret = _remove_lst_evt(lst, (GxRecordPVRList *)ctrl->arg);
		}
		break;
	case GX_RECORD_PVR_FREE_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;

			_free_lst_info(&info->list);
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
	GxCore_MutexUnlock(lst->mutex);

	return ret;
}

static int lst_getctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRLstContext *lst = (PVRLstContext *)priv;

	PVR_LST_CHECK_NULL(lst);
	PVR_LST_CHECK_NULL(lst->lst_rw.parser);

	GxCore_MutexLock(lst->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_GET_DURATION:
	case GX_RECORD_PVR_GET_FILESIZE:
		{
			PVRInfo info;
			_update_lstfile(lst);
			_get_lst_info(lst, &info);
			if (ctrl->opt == GX_RECORD_PVR_GET_DURATION)
				*(int64_t *)ctrl->arg = info.tr_duration;
			else if (ctrl->opt == GX_RECORD_PVR_GET_FILESIZE)
				*(int64_t *)ctrl->arg = info.tr_filesize;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_CURTIME:
		{
			*(int64_t *)ctrl->arg = _get_lst_curtime(lst);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_POS_BY_TIME:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			_update_lstfile(lst);
			time_pos->offset = _get_lst_posbytime(lst, time_pos->timems);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_TIME_BY_POS:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			_update_lstfile(lst);
			time_pos->timems = _get_lst_timebypos(lst, time_pos->offset);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;
			char *pvr_file = lst->tmpbuf0;

			info->type = GX_RECORD_PVR_LST_FILE;
			_alloc_lst_info(lst, &info->list);

			snprintf(pvr_file, PVR_MAX_URL_LEN, "%s%s", lst->path, PVR_PVRFILE_SUFFIX);
			pvr_get_absolute_url(pvr_file, PVR_MAX_LINE_LEN);
			info->pvr_file = av_strdup(pvr_file);
			info->dir      = NULL;
			info->prefix   = NULL;
			info->path     = av_strdup(lst->path);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_TYPE:
		{
			GxRecordPVRFileType *type = (GxRecordPVRFileType *)ctrl->arg;

			*type = GX_RECORD_PVR_LST_FILE;
		}
		break;
	case GX_RECORD_PVR_GET_KEY_DESC:
		{
			ctrl->arg = (void *)lst->desc_key;
			ret = 0;
		}
		break;
	default:
		gxloge(" [%d] lst option not find\n", ctrl->opt);
		break;
	}
	GxCore_MutexUnlock(lst->mutex);

	return ret;
}

PVROps pvr_lst_ops = {
	.open         = lst_open,
	.close        = lst_close,
	.write        = NULL,
	.read         = lst_read,
	.seek         = lst_seek,
	.getinfo      = lst_getinfo,
	.getpos       = lst_getpos,
	.settime      = NULL,
	.probehdr     = lst_probehdr,
	.readhdr      = lst_readhdr,
	.writehdr     = NULL,
	.setctrl      = lst_setctrl,
	.getctrl      = lst_getctrl,
};
