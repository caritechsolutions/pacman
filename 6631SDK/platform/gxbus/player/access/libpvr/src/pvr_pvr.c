#include "gx_common.h"
#include "gx_system.h"
#include "avstring.h"
#include "recfs.h"
#include "../libpvr.h"
#include "pvr_config.h"
#include "pvr_file.h"
#include "pvr_parser.h"
#include "pvr_utils.h"
#include "pvr_enc.h"

typedef struct {
	PVRParser            *parser;
	int                   seg_no;
	struct record_file   *ts_fd;
	int                   ts_no;
} PVRPvrSegRW;

typedef struct {
	PVRParser            *parser;
	char                 *prefix;
	int                   evt_seg_no; //正在写数据的seg_no
} PVRPvrEvtRW;

typedef struct {
	PVRParser            *parser;
	char                 *prefix;
} PVRPvrLstRW;

typedef struct {
	char                 *path;
	PVRParser            *pvr_parser;
	PVRPvrLstRW           lst_rw[PVR_MAX_LST_COUNT];
	PVRPvrEvtRW           evt_rw[PVR_MAX_EVT_COUNT];
	PVRPvrSegRW           seg_rw;
	handle_t              mutex;
	int                   mkdir;
	int                   write_flag;
	unsigned int          new_timems;
	unsigned int          prev_segts_timems;
	unsigned int          prev_fsync_timems;
	unsigned int          prev_write_timems;
	char                  tmpbuf[PVR_MAX_BUF_SIZE];
	char                  tmpurl[PVR_MAX_BUF_SIZE];
	char                 *desc_key;
	unsigned char        *buffer_w;
	size_t                pos_w;
} PVRPvrContext;

#define PVR_WBLOCK_SIZE (64*1024)
#define PVR_PVR_CHECK_NULL(ptr) do {           \
	if (NULL == ptr) {                         \
		gxloge(" ptr is NULL\n");              \
		return -1;                             \
	}                                          \
} while(0)

#define PVR_PVR_DEBUG_PRINT(printf_func) do {                \
	int debug_flags = 0;                                     \
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_PVR, &debug_flags); \
	if (debug_flags)                                         \
		printf_func;                                         \
} while(0)

#ifndef CONFIG_FFMPEG_ENABLE
#define av_stristr gx_stristr

static int gx_toupper(int c)
{
	if (c >= 'a' && c <= 'z')
		c ^= 0x20;
	return c;
}

static int gx_stristart(const char *str, const char *pfx, const char **ptr)
{
	while (*pfx && gx_toupper((unsigned)*pfx) == gx_toupper((unsigned)*str)) {
		pfx++;
		str++;
	}
	if (!*pfx && ptr)
		*ptr = str;
	return !*pfx;
}

static char *gx_stristr(const char *s1, const char *s2)
{
	if (!*s2)
		return (char*)s1;

	do
		if (gx_stristart(s1, s2, NULL))
			return (char*)s1;
	while (*s1++);

	return NULL;
}
#endif

static int _find_pvr_lst_by_prefix(PVRPvrContext *pvr, char *prefix)
{
	int i = 0;

	for (i = 0; i < PVR_MAX_LST_COUNT; i++) {
		if (NULL == pvr->lst_rw[i].prefix)
			continue;
		if (NULL == pvr->lst_rw[i].parser)
			continue;
		if (0 == strcmp(prefix, pvr->lst_rw[i].prefix))
			break;
	}
	if (i < PVR_MAX_LST_COUNT)
		return i;

	return -1;
}

static int _find_pvr_lst_is_empty(PVRPvrContext *pvr)
{
	int i = 0;

	for (i = 0; i < PVR_MAX_LST_COUNT; i++) {
		if ((NULL == pvr->lst_rw[i].prefix) && (NULL == pvr->lst_rw[i].parser))
			break;
	}
	if (i >= PVR_MAX_LST_COUNT)
		return -1;

	return i;
}

static int _find_pvr_evt_by_prefix(PVRPvrContext *pvr, char *prefix)
{
	int i = 0;

	for (i = 0; i < PVR_MAX_EVT_COUNT; i++) {
		if (NULL == pvr->evt_rw[i].prefix)
			continue;
		if (NULL == pvr->evt_rw[i].parser)
			continue;
		if (0 == strcmp(prefix, pvr->evt_rw[i].prefix))
			break;
	}
	if (i < PVR_MAX_EVT_COUNT)
		return i;

	return -1;
}

static int _find_pvr_evt_is_empty(PVRPvrContext *pvr)
{
	int i = 0;

	for (i = 0; i < PVR_MAX_EVT_COUNT; i++) {
		if ((NULL == pvr->evt_rw[i].prefix) && (NULL == pvr->evt_rw[i].parser))
			break;
	}
	if (i >= PVR_MAX_EVT_COUNT)
		return -1;

	return i;
}

static void _save_evtfile(PVRPvrContext *pvr, int is_finish)
{
	char *line = pvr->tmpbuf;
	unsigned int i = 0;

	for (i = 0; i < PVR_MAX_EVT_COUNT; i++) {
		if (NULL == pvr->evt_rw[i].parser)
			continue;
		if (NULL == pvr->evt_rw[i].prefix)
			continue;
		if ((-1 != pvr->seg_rw.seg_no) &&
				(pvr->seg_rw.seg_no == pvr->evt_rw[i].evt_seg_no)) {
			int seg_no = pvr->seg_rw.seg_no;
			int64_t duration = pvr_parser_get_duration_byseqno(pvr->pvr_parser, seg_no);
			int64_t filesize = pvr_parser_get_filesize_byseqno(pvr->pvr_parser, seg_no);

			snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, duration);
			pvr_parser_write_line(pvr->evt_rw[i].parser, line, PVRFILE_OVERLAP);
			snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, filesize);
			pvr_parser_write_line(pvr->evt_rw[i].parser, line, PVRFILE_APPEND);
			pvr_parser_sync(pvr->evt_rw[i].parser);
			if (is_finish)
				pvr->evt_rw[i].evt_seg_no = -1;
		}
	}

	return;
}

static void _save_pvrfile(PVRPvrContext *pvr)
{
	char *line = pvr->tmpbuf;
	unsigned int segts_duration = 0;
	unsigned int segts_filesize = 0;
	int64_t seg_duration = 0;
	off_t   seg_filesize = 0;
	unsigned int dis_timems = 0;
	unsigned int i = 0;

	if (NULL == pvr->pvr_parser)
		return;

	if (NULL == pvr->seg_rw.parser)
		return;

	if (NULL == pvr->seg_rw.ts_fd)
		return;

	if (0 == pvr_parser_get_seqcount(pvr->seg_rw.parser))
		return;

	if ((pvr->new_timems == 0) &&
			(pvr->prev_segts_timems == (unsigned int)-1))
		return;

	segts_duration = pvr_parser_get_duration_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no);
	segts_filesize = recfile_getsize(pvr->seg_rw.ts_fd);
	snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_DURATION, segts_duration);
	pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_OVERLAP);
	snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_FILESIZE, segts_filesize);
	pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_APPEND);
	snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_ISFINISH, 1);
	pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
	pvr->prev_segts_timems = pvr->new_timems;

	if (pvr->prev_write_timems == (unsigned int)-1)
		pvr->prev_write_timems = pvr->new_timems;
	dis_timems = pvr->new_timems - pvr->prev_write_timems;
	if (dis_timems > 0) {
		pvr_parser_add_timems(pvr->seg_rw.parser, dis_timems);
		pvr_parser_add_timems(pvr->pvr_parser,    dis_timems);
		for (i = 0; i < PVR_MAX_EVT_COUNT; i++) {
			if (NULL == pvr->evt_rw[i].parser)
				continue;
			if (NULL == pvr->evt_rw[i].prefix)
				continue;
			if ((-1 != pvr->seg_rw.seg_no) &&
					(pvr->seg_rw.seg_no == pvr->evt_rw[i].evt_seg_no))
				pvr_parser_add_timems(pvr->evt_rw[i].parser, dis_timems);
		}
	}
	pvr->prev_write_timems = pvr->new_timems;

	seg_duration = pvr_parser_get_duration(pvr->seg_rw.parser, PVRFILE_TOTAL);
	seg_filesize = pvr_parser_get_filesize(pvr->seg_rw.parser, PVRFILE_TOTAL);
	snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, seg_duration);
	pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_OVERLAP);
	snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, seg_filesize);
	pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_APPEND);

	_save_evtfile(pvr, 1);

	return;
}

static void _clear_seg_rw(PVRPvrSegRW *rw)
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
	rw->seg_no = -1;
	return;
}

static void _clear_evt_rw(PVRPvrEvtRW *rw)
{
	if (rw->prefix) {
		av_free(rw->prefix);
		rw->prefix = NULL;
	}

	if (rw->parser) {
		pvr_parser_free(rw->parser);
		rw->parser = NULL;
	}

	rw->evt_seg_no = -1;

	return;
}

static void _clear_lst_rw(PVRPvrLstRW *rw)
{
	if (rw->prefix) {
		av_free(rw->prefix);
		rw->prefix = NULL;
	}

	if (rw->parser) {
		pvr_parser_free(rw->parser);
		rw->parser = NULL;
	}

	return;
}

static int _write_l(PVRPvrContext *pvr, struct record_file *fd, unsigned char *buffer, size_t max_len, int over)
{
	size_t size  = max_len, wsize = 0, len = 0;
	size_t pos_w = pvr->pos_w;
	unsigned char *buffer_w = pvr->buffer_w;
	unsigned char *wptr = NULL;

	if (buffer_w == NULL) {
		if (max_len > 0)
			len = recfile_write(fd, buffer, max_len);
		return len;
	}

	if (over) {
		if (pos_w > 0)
			len = recfile_write(fd, buffer_w, pos_w);
		pvr->pos_w = 0;
		return len;
	}

	while (size) {
		if (size < (PVR_WBLOCK_SIZE - pos_w)) {
			memcpy(buffer_w + pos_w, buffer + max_len - size, size);
			pos_w += size;
			break;
		} else {
			if (pos_w != 0) {
				memcpy(buffer_w + pos_w, buffer + max_len - size, PVR_WBLOCK_SIZE - pos_w);
				wptr  = buffer_w;
				size -= (PVR_WBLOCK_SIZE - pos_w);
				pos_w = 0;
			}
			else {
				wptr = buffer + max_len - size;
				size -= PVR_WBLOCK_SIZE;
			}

			len = recfile_write(fd, wptr, PVR_WBLOCK_SIZE);
			if (len != PVR_WBLOCK_SIZE) {
				gxloge("[PVR FILE] write ret = %d\n", len);
				return 0;
			}
			wsize += len;
		}
	}
	pvr->pos_w = pos_w;

	return max_len;
}

static unsigned int _get_file(char *path, char *suffix, char *tmpbuf, GxRecordPVRFileElem **elem)
{
	int entries = 0, count = 0;
	GxDirent *ents = NULL;
	unsigned int i = 0;
	char *postfix = suffix;
	char *dot = suffix;

	while (1) {
		dot = strchr(dot, '.');
		if (dot == NULL)
			break;
		dot++;
		postfix = dot;
	}
	entries = GxCore_GetDir(path, &ents, postfix);
	if (entries > 0) {
		for (i = 0; i < entries; i++) {
			if ((GX_FILE_REGULAR == ents[i].ftype) &&
					(av_stristr(ents[i].fname, suffix)))
			count++;
		}

		if (count > 0) {
			GxCore_SortDir(ents, entries, NULL);
			GxRecordPVRFileElem *file_elem = av_mallocz(count * sizeof(GxRecordPVRFileElem));

			if (NULL != file_elem) {
				char *url = tmpbuf;
				unsigned int j = 0;

				for (i = 0; i < entries; i++) {
					if ((GX_FILE_REGULAR != ents[i].ftype) ||
							(!av_stristr(ents[i].fname, suffix)))
						continue;
					snprintf(url, PVR_MAX_URL_LEN, "%s/%s", path, ents[i].fname);
					file_elem[j].file   = av_strdup(url);
					file_elem[j].prefix = pvr_parse_prefix(url, suffix);
					j++;
				}
				count = j;
			} else {
				gxloge("[%s %d]: malloc fail\n", __func__, __LINE__);
				count = 0;
			}
			*elem = file_elem;
		} else
			*elem = NULL;
		GxCore_FreeDir(ents, entries);
	}

	return count;
}

static void _alloc_file_info(PVRPvrContext *pvr, GxRecordPVRFile *file)
{
	char *lst_path = pvr->path;
	char *evt_path = pvr->tmpbuf;
	char *seg_path = pvr->tmpbuf;

	file->list_count = _get_file(lst_path, PVR_LSTFILE_SUFFIX, pvr->tmpurl, &file->list_elem);

	snprintf(evt_path, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_PVRFILE_EVTPATH);
	pvr_get_absolute_url(evt_path, PVR_MAX_LINE_LEN);
	file->event_count = _get_file(evt_path, PVR_EVTFILE_SUFFIX, pvr->tmpurl, &file->event_elem);

	snprintf(seg_path, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_PVRFILE_SEGPATH);
	pvr_get_absolute_url(seg_path, PVR_MAX_LINE_LEN);
	file->segment_count = _get_file(seg_path, PVR_SEGFILE_SUFFIX, pvr->tmpurl, &file->segment_elem);

	return;
}

static void _free_file_info(GxRecordPVRFile *file)
{
	unsigned int i = 0;

	if (file->list_elem) {
		GxRecordPVRFileElem *file_elem = file->list_elem;

		for (i = 0; i < file->list_count; i++) {
			if (file_elem[i].prefix)
				av_free(file_elem[i].prefix);
			if (file_elem[i].file)
				av_free(file_elem[i].file);
		}
		if (file->list_elem) {
			av_free(file->list_elem);
			file->list_elem  = NULL;
		}
	}
	file->list_count = 0;

	if (file->event_elem) {
		GxRecordPVRFileElem *file_elem = file->event_elem;

		for (i = 0; i < file->event_count; i++) {
			if (file_elem[i].prefix)
				av_free(file_elem[i].prefix);
			if (file_elem[i].file)
				av_free(file_elem[i].file);
		}
		if (file->event_elem) {
			av_free(file->event_elem);
			file->event_elem  = NULL;
		}
	}
	file->event_count = 0;

	if (file->segment_elem) {
		GxRecordPVRFileElem *file_elem = file->segment_elem;

		for (i = 0; i < file->segment_count; i++) {
			if (file_elem[i].prefix)
				av_free(file_elem[i].prefix);
			if (file_elem[i].file)
				av_free(file_elem[i].file);
		}
		if (file->segment_elem) {
			av_free(file->segment_elem);
			file->segment_elem  = NULL;
		}
	}
	file->segment_count = 0;

	return;
}

static int _new_pvr_seg(PVRPvrContext *pvr, GxRecordPVRSegment *conf)
{
	int count = 0, seqno = -1;
	char *url = NULL;

	PVR_PVR_CHECK_NULL(conf);
	PVR_PVR_CHECK_NULL(conf->prefix);

	if (pvr_strlen_line(conf->prefix) >= PVR_MAX_SEG_PREFIX_LEN) {
		gxloge(" [%s] prefix more than %d bytes\n", conf->prefix, PVR_MAX_SEG_PREFIX_LEN);
		return -1;
	}

	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s",
			pvr->path, PVR_PVRFILE_SEGPATH, conf->prefix, PVR_SEGFILE_SUFFIX);
	if (pvr_file_exist(url)) {
		gxloge(" [%s] prefix already exist\n", conf->prefix);
		return -1;
	}
	seqno = pvr_parser_get_seqno_byprefix(pvr->pvr_parser, conf->prefix);
	if (-1 != seqno) {
		gxloge(" [%s] prefix already exist in pvr parser\n", conf->prefix);
		return -1;
	}

	count = pvr_parser_get_seqcount(pvr->pvr_parser);
	if (0 == count) {
		char *line = pvr->tmpbuf;
		char *path = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_PVR_DESC, PVR_PVRFILE_DESC);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_NORMAL);
		snprintf(path, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_PVRFILE_SEGPATH);
		if (pvr_mk_dir(path) < 0) {
			gxloge(" [%s] mkdir fail\n", path);
			return -2;
		}
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_PATH, PVR_PVRFILE_SEGPATH);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_NORMAL);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_SUFFIX, PVR_SEGFILE_SUFFIX);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_NORMAL);
	}

	seqno = pvr->seg_rw.seg_no;
	if (pvr->seg_rw.ts_fd)
		_write_l(pvr, pvr->seg_rw.ts_fd, NULL, 0, 1);
	_save_pvrfile(pvr);
	_clear_seg_rw(&pvr->seg_rw);

	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s",
			pvr->path, PVR_PVRFILE_SEGPATH, conf->prefix, PVR_SEGFILE_SUFFIX);
	pvr->seg_rw.parser = pvr_parser_alloc(url);
	if (pvr->seg_rw.parser) {
		char *line = pvr->tmpbuf;
		char *path = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_DESC, conf->desc);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		if (pvr->desc_key) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_ENC_DESC, pvr->desc_key);
			pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		}
		snprintf(path, PVR_MAX_URL_LEN, "%s/%s/%s", pvr->path, PVR_PVRFILE_SEGPATH, conf->prefix);
		if (pvr_mk_dir(path) < 0) {
			gxloge(" [%s] mkdir fail\n", path);
			return -2;
		}
		pvr->seg_rw.seg_no = (seqno + 1);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_TS_PATH, conf->prefix);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_TS_SUFFIX, PVR_STSFILE_SUFFIX);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_PREFIX, conf->prefix);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_NORMAL);
		pvr_parser_sync(pvr->seg_rw.parser);
		pvr_parser_sync(pvr->pvr_parser);
	} else {
		gxloge(" [%s] seg parser alloc fail\n", url);
		return -3;
	}

	return 0;
}

static int _delete_pvr_seg(PVRPvrContext *pvr, const char *prefix)
{
	int seqno = 0;
	char *url = NULL;

	PVR_PVR_CHECK_NULL(prefix);

	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s", pvr->path, PVR_PVRFILE_SEGPATH, prefix, PVR_SEGFILE_SUFFIX);
	if (0 == pvr_file_exist(url)) {
		gxloge(" [%s] prefix not exist\n", prefix);
		return -1;
	}

	seqno = pvr_parser_get_seqno_byprefix(pvr->pvr_parser, (char *)prefix);
	if (-1 == seqno) {
		gxloge(" [%s] seqno not find in pvr parser\n", prefix);
		return -1;
	}

	if (!pvr_parser_exist(url)) {
		if (seqno != pvr->seg_rw.seg_no) {
			char *line = pvr->tmpbuf;
			char *path = pvr->tmpbuf;

			PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", url));
			if (-1 == pvr_rm_file(url))
				gxloge(" [%s] file rm fail\n", url);
			snprintf(path, PVR_MAX_URL_LEN, "%s/%s/%s", pvr->path, PVR_PVRFILE_SEGPATH, prefix);
			PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", path));
			if (-1 == pvr_rm_dir(path))
				gxloge(" [%s] path rm fail\n", path);
			snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_SEG_UNVAILD, seqno);
			pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_NORMAL);
			//?? 是否要增加PVR_SEG_UNVAILD到evtfile
		} else {
			gxloge(" [%s] prefix in writting !!!!!!!!!!!!\n", url);
			return -2;
		}
	} else {
		if (seqno == pvr->seg_rw.seg_no)
			gxloge(" [%s] prefix in writting\n", url);
		else
			gxloge(" [%s] prefix in  reading\n", url);
		return -2;
	}

	return 0;
}

static int _open_pvr_evt(PVRPvrContext *pvr, GxRecordPVREventSet *conf)
{
	int evtno = -1;
	char *url = NULL;
	char *path = NULL;
	char *line = NULL;

	PVR_PVR_CHECK_NULL(conf);
	PVR_PVR_CHECK_NULL(conf->prefix);

	evtno = _find_pvr_evt_by_prefix(pvr, conf->prefix);
	if (-1 != evtno) {
		gxloge(" [%s] event file already open\n", conf->prefix);
		return -1;
	}
	evtno = _find_pvr_evt_is_empty(pvr);
	if (-1 == evtno) {
		gxloge(" [%s] more than %d event file already open\n", conf->prefix, PVR_MAX_EVT_COUNT);
		return -1;
	}

	path = pvr->tmpbuf;
	url  = pvr->tmpbuf;
	line = pvr->tmpbuf;
	snprintf(path, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_PVRFILE_EVTPATH);
	if (pvr_mk_dir(path) < 0) {
		gxloge(" [%s] mkdir fail\n", path);
		return -2;
	}
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s",
			pvr->path, PVR_PVRFILE_EVTPATH, conf->prefix, PVR_EVTFILE_SUFFIX);
	pvr->evt_rw[evtno].parser = pvr_parser_alloc(url);
	if (NULL == pvr->evt_rw[evtno].parser) {
		gxloge(" [%s] event parser alloc fail\n", url);
		return -3;
	}
	pvr->evt_rw[evtno].prefix = av_strdup(conf->prefix);
	pvr->evt_rw[evtno].evt_seg_no = -1;

	if (conf->desc &&
			(NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_DESC))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_DESC, conf->desc);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (conf->chapter &&
			(NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_CHAPTER))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_CHAPTER, conf->chapter);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (conf->chapter_desc &&
			(NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_CHAPTER_DESC))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_CHAPTER_DESC, conf->chapter_desc);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (conf->start_time &&
			(NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_START_TIME))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_START_TIME, conf->start_time);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (conf->end_time &&
			(NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_END_TIME))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_END_TIME, conf->end_time);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_PATH)) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s%s\n", PVR_SEG_PATH, "../", PVR_PVRFILE_SEGPATH);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	if (NULL == pvr_parser_get_arg(pvr->evt_rw[evtno].parser, PVRFILE_GET_SUFFIX)) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_SUFFIX, PVR_SEGFILE_SUFFIX);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
	}
	pvr_parser_sync(pvr->evt_rw[evtno].parser);

	return 0;
}

static int _close_pvr_evt(PVRPvrContext *pvr, const char *prefix)
{
	int evtno = -1;

	PVR_PVR_CHECK_NULL(prefix);

	evtno = _find_pvr_evt_by_prefix(pvr, (char *)prefix);
	if (-1 == evtno) {
		gxloge(" [%s] event file already close\n", prefix);
		return -1;
	}

	if (-1 != pvr->evt_rw[evtno].evt_seg_no) {
		int seg_no = pvr->evt_rw[evtno].evt_seg_no;
		int64_t duration = pvr_parser_get_duration_byseqno(pvr->pvr_parser, seg_no);
		int64_t filesize = pvr_parser_get_filesize_byseqno(pvr->pvr_parser, seg_no);
		char *line = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, duration);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_OVERLAP);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, filesize);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_APPEND);
	}
	_clear_evt_rw(&pvr->evt_rw[evtno]);

	return 0;
}

static int _delete_pvr_evt(PVRPvrContext *pvr, const char *prefix)
{
	int evtno = -1;
	char *url = NULL;

	PVR_PVR_CHECK_NULL(prefix);

	evtno = _find_pvr_evt_by_prefix(pvr, (char *)prefix);
	if (-1 != evtno)
		_clear_evt_rw(&pvr->evt_rw[evtno]);

	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s",
			pvr->path, PVR_PVRFILE_EVTPATH, prefix, PVR_EVTFILE_SUFFIX);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", url));

	return (pvr_rm_file(url) == 1) ? 0 : -1;
}

static int _add_pvr_seg_to_evt(PVRPvrContext *pvr, GxRecordPVRElem *elem)
{
	unsigned int i = 0;
	int evtno = -1;

	PVR_PVR_CHECK_NULL(elem);
	PVR_PVR_CHECK_NULL(elem->prefix);
	PVR_PVR_CHECK_NULL(elem->elem_prefix);
	if (elem->elem_count <= 0) return -1;

	evtno = _find_pvr_evt_by_prefix(pvr, elem->prefix);
	if (-1 == evtno) {
		gxloge(" [%s] event file is not open\n", elem->prefix);
		return -1;
	}

	//write evtfile, evt not in writting
	if (-1 != pvr->evt_rw[evtno].evt_seg_no) {
		gxloge(" [%s] event file has segment in writting\n", pvr->evt_rw[evtno].prefix);
		return -1;
	}
	for (i = 0; i < elem->elem_count; i++) {
		int segno = -1;

		if (NULL == elem->elem_prefix[i]) {
			gxloge(" [%d] segment prefix is NULL\n", i);
			return -1;
		}
		//write evtfile, seg prefix must exist
		segno = pvr_parser_get_seqno_byprefix(pvr->pvr_parser, elem->elem_prefix[i]);
		if (-1 == segno) {
			gxloge(" [%s] segment not exist\n", elem->elem_prefix[i]);
			return -1;
		}
		//write evtfile, seg not in writting, except last elem seg
		if ((segno == pvr->seg_rw.seg_no) && ((i + 1) != elem->elem_count)) {
			gxloge(" [%d %d] segment in writting, must last count\n", i, elem->elem_count);
			return -1;
		}

		if (-1 != pvr_parser_get_seqno_byprefix(pvr->evt_rw[evtno].parser, elem->elem_prefix[i])) {
			gxloge(" [%s] segment already exist in event file\n", elem->elem_prefix[i]);
			return -1;
		}
	}

	//delete seg, not set unvaild to evtfile
	for (i = 0; i < elem->elem_count; i++) {
		int segno = pvr_parser_get_seqno_byprefix(pvr->pvr_parser, elem->elem_prefix[i]);
		int64_t duration = pvr_parser_get_duration_byseqno(pvr->pvr_parser, segno);
		int64_t filesize = pvr_parser_get_filesize_byseqno(pvr->pvr_parser, segno);
		char *line = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_SEG_PREFIX, elem->elem_prefix[i]);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
		if (segno != pvr->seg_rw.seg_no) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, duration);
			pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_OVERLAP);
			snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, filesize);
			pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_APPEND);
			pvr->evt_rw[evtno].evt_seg_no = -1;
		} else {
			pvr->evt_rw[evtno].evt_seg_no = segno;
		}
	}
	pvr_parser_sync(pvr->evt_rw[evtno].parser);

	return 0;
}

static int _remove_pvr_seg_in_evt(PVRPvrContext *pvr, GxRecordPVRElem *elem)
{
	unsigned int i = 0;
	int evtno = -1;

	PVR_PVR_CHECK_NULL(elem);
	PVR_PVR_CHECK_NULL(elem->prefix);
	PVR_PVR_CHECK_NULL(elem->elem_prefix);
	if (elem->elem_count <= 0) return -1;

	evtno = _find_pvr_evt_by_prefix(pvr, elem->prefix);
	if (-1 == evtno) {
		gxloge(" [%s] event file already close\n", elem->prefix);
		return -1;
	}

	for (i = 0; i < elem->elem_count; i++) {
		if (NULL == elem->elem_prefix[i]) {
			gxloge(" [%s] segment prefix is NULL\n", elem->elem_prefix[i]);
			return -1;
		}
		if (-1 == pvr_parser_get_seqno_byprefix(pvr->evt_rw[evtno].parser, elem->elem_prefix[i])) {
			gxloge(" [%s] segment not exist in event file\n", elem->elem_prefix[i]);
			return -1;
		}
	}

	for (i = 0; i < elem->elem_count; i++) {
		int segno = pvr_parser_get_seqno_byprefix(pvr->evt_rw[evtno].parser, elem->elem_prefix[i]);
		char *line = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_SEG_UNVAILD, segno);
		pvr_parser_write_line(pvr->evt_rw[evtno].parser, line, PVRFILE_NORMAL);
		if (-1 != pvr->evt_rw[evtno].evt_seg_no) {
			if (segno == pvr->evt_rw[evtno].evt_seg_no)
				pvr->evt_rw[evtno].evt_seg_no = -1;
		}
	}
	pvr_parser_sync(pvr->evt_rw[evtno].parser);

	return 0;
}


static int _open_pvr_lst(PVRPvrContext *pvr, GxRecordPVRListSet *conf)
{
	int lstno = -1;
	char *line = NULL;
	char *url  = NULL;

	PVR_PVR_CHECK_NULL(conf);
	PVR_PVR_CHECK_NULL(conf->prefix);

	lstno = _find_pvr_lst_by_prefix(pvr, conf->prefix);
	if (-1 != lstno) {
		gxloge(" [%s] lst file already open\n", conf->prefix);
		return -1;
	}

	lstno = _find_pvr_lst_is_empty(pvr);
	if (-1 == lstno) {
		gxloge(" [%s] more than %d list file already open\n", conf->prefix, PVR_MAX_LST_COUNT);
		return -1;
	}

	line = pvr->tmpbuf;
	url  = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s%s", pvr->path, conf->prefix, PVR_LSTFILE_SUFFIX);
	pvr->lst_rw[lstno].parser = pvr_parser_alloc(url);
	if (NULL == pvr->lst_rw[lstno].parser) {
		gxloge(" list parser alloc fail\n");
		return -2;
	}
	pvr->lst_rw[lstno].prefix = av_strdup(conf->prefix);

	if ((NULL != conf->desc) &&
			(NULL == pvr_parser_get_arg(pvr->lst_rw[lstno].parser, PVRFILE_GET_DESC))) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_LST_DESC, conf->desc);
		pvr_parser_write_line(pvr->lst_rw[lstno].parser, line, PVRFILE_NORMAL);
	}
	if (NULL == pvr_parser_get_arg(pvr->lst_rw[lstno].parser, PVRFILE_GET_PATH)) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_PATH, PVR_PVRFILE_EVTPATH);
		pvr_parser_write_line(pvr->lst_rw[lstno].parser, line, PVRFILE_NORMAL);
	}
	if (NULL == pvr_parser_get_arg(pvr->lst_rw[lstno].parser, PVRFILE_GET_SUFFIX)) {
		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_SUFFIX, PVR_EVTFILE_SUFFIX);
		pvr_parser_write_line(pvr->lst_rw[lstno].parser, line, PVRFILE_NORMAL);
	}
	pvr_parser_sync(pvr->lst_rw[lstno].parser);

	return 0;
}

static int _close_pvr_lst(PVRPvrContext *pvr, const char *prefix)
{
	int lstno = -1;
	char *url = NULL;

	PVR_PVR_CHECK_NULL(prefix);

	lstno = _find_pvr_lst_by_prefix(pvr, (char *)prefix);
	if (-1 == lstno) {
		gxloge(" [%s] list file already close\n", prefix);
		return -1;
	}
	_clear_lst_rw(&pvr->lst_rw[lstno]);

	return 0;
}

static int _delete_pvr_lst(PVRPvrContext *pvr, const char *prefix)
{
	int lstno = -1;
	char *url = NULL;

	PVR_PVR_CHECK_NULL(prefix);

	lstno = _find_pvr_lst_by_prefix(pvr, (char *)prefix);
	if (-1 != lstno)
		_clear_lst_rw(&pvr->lst_rw[lstno]);

	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s%s", pvr->path, prefix, PVR_LSTFILE_SUFFIX);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", url));

	return (pvr_rm_file(url) == 1) ? 0 : -1;
}

static int _add_pvr_evt_to_lst(PVRPvrContext *pvr, GxRecordPVRElem *elem)
{
	int lstno = -1;
	unsigned int i = 0;

	PVR_PVR_CHECK_NULL(elem);
	PVR_PVR_CHECK_NULL(elem->prefix);
	PVR_PVR_CHECK_NULL(elem->elem_prefix);
	if (elem->elem_count <= 0) return -1;

	lstno = _find_pvr_lst_by_prefix(pvr, elem->prefix);
	if (-1 == lstno) {
		gxloge(" [%s] list file is not open\n", elem->prefix);
		return -1;
	}

	for (i = 0; i < elem->elem_count; i++) {
		int evtno = -1;

		if (NULL == elem->elem_prefix[i]) {
			gxloge(" [%d] prefix is NULL\n", i);
			return -1;
		}

		if (-1 != pvr_parser_get_seqno_byprefix(pvr->lst_rw[lstno].parser, elem->elem_prefix[i])) {
			gxloge(" [%s] event already exist in list file\n", elem->elem_prefix[i]);
			return -1;
		}

		evtno = _find_pvr_evt_by_prefix(pvr, elem->elem_prefix[i]);
		if (-1 == evtno) {
			char *url = pvr->tmpbuf;
			snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s%s",
					pvr->path, PVR_PVRFILE_EVTPATH, elem->elem_prefix[i], PVR_EVTFILE_SUFFIX);
			if (0 == pvr_file_exist(url)) {
				gxloge(" [%s] event not exist\n", elem->elem_prefix[i]);
				return -1;
			}
		}
	}

	for (i = 0; i < elem->elem_count; i++) {
		char *line = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_EVT_PREFIX, elem->elem_prefix[i]);
		pvr_parser_write_line(pvr->lst_rw[lstno].parser, line, PVRFILE_NORMAL);
	}
	pvr_parser_sync(pvr->lst_rw[lstno].parser);

	return 0;
}

static int _remove_pvr_evt_in_lst(PVRPvrContext *pvr, GxRecordPVRElem *elem)
{
	int lstno = -1;
	unsigned int i = 0;

	PVR_PVR_CHECK_NULL(elem);
	PVR_PVR_CHECK_NULL(elem->prefix);
	PVR_PVR_CHECK_NULL(elem->elem_prefix);
	if (elem->elem_count <= 0) return -1;

	lstno = _find_pvr_lst_by_prefix(pvr, elem->prefix);
	if (-1 == lstno) {
		gxloge(" [%s] list file is not open\n", elem->prefix);
		return -1;
	}

	for (i = 0; i < elem->elem_count; i++) {
		if (NULL == elem->elem_prefix[i]) {
			gxloge(" [%d] prefix is NULL\n", i);
			return -1;
		}
		if (-1 == pvr_parser_get_seqno_byprefix(pvr->lst_rw[lstno].parser, elem->elem_prefix[i])) {
			gxloge(" [%s] event not find in list file\n", elem->elem_prefix[i]);
			return -1;
		}
	}

	for (i = 0; i < elem->elem_count; i++) {
		int seqno = pvr_parser_get_seqno_byprefix(pvr->lst_rw[lstno].parser, elem->elem_prefix[i]);
		char *line = pvr->tmpbuf;

		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_EVT_UNVAILD, seqno);
		pvr_parser_write_line(pvr->lst_rw[lstno].parser, line, PVRFILE_NORMAL);
	}
	pvr_parser_sync(pvr->lst_rw[lstno].parser);

	return 0;
}

static int64_t _get_pvr_curtime(PVRPvrContext *pvr)
{
	int64_t pvr_time = 0;
	PVRFileAttr attr = PVRFILE_TOTAL;

	if (pvr->seg_rw.seg_no >= 0) {
		pvr_time += pvr_parser_get_starttime_byseqno(pvr->pvr_parser, pvr->seg_rw.seg_no, attr);
		if (pvr->seg_rw.parser && (pvr->seg_rw.ts_no >= 0)) {
			pvr_time += pvr_parser_get_starttime_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no, attr);
			if (pvr->seg_rw.ts_fd) {
				int64_t timems = 0;
				off_t   offset = recfile_getpos(pvr->seg_rw.ts_fd);
				int64_t duration = pvr_parser_get_duration(pvr->pvr_parser, attr);
				off_t   filesize = pvr_parser_get_filesize(pvr->pvr_parser, attr);

				if (filesize > 0)
					timems = (int64_t)(offset * ((float)duration / (float)filesize));
				pvr_time = (((timems + pvr_time) > duration) ? duration : (timems + pvr_time));
			}
		}
	}

	return pvr_time;
}

static int64_t _get_pvr_mintime(PVRPvrContext *pvr)
{
	return pvr_parser_get_mintime(pvr->pvr_parser);
}

static int64_t _get_pvr_timebypos(PVRPvrContext *pvr, off_t pos)
{
	int seqno = -1;
	off_t seg_offset = 0;
	int64_t pvr_time = 0;

	seqno = pvr_parser_get_seqno_bypos(pvr->pvr_parser, pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in pvr parser\n", seqno);
		return -1;
	}
	pvr_time  += pvr_parser_get_starttime_byseqno(pvr->pvr_parser, seqno, PVRFILE_TOTAL);
	seg_offset = pvr_parser_get_startpos_byseqno (pvr->pvr_parser, seqno, PVRFILE_TOTAL);
	seg_offset = (pos - seg_offset);
	if (seg_offset > 0) {
		int64_t timems = 0;
		int64_t duration = pvr_parser_get_duration(pvr->pvr_parser, PVRFILE_TOTAL);
		off_t   filesize = pvr_parser_get_filesize(pvr->pvr_parser, PVRFILE_TOTAL);

		if (filesize > 0)
			timems = (int64_t)(seg_offset * ((float)duration / (float)filesize));
		pvr_time = (((timems + pvr_time) > duration) ? duration : (timems + pvr_time));
	}

	return pvr_time;
}

static off_t _get_pvr_posbytime(PVRPvrContext *pvr, int64_t timems)
{
	int seqno = -1;
	off_t pvr_pos = 0;
	int64_t seg_time = 0;

	seqno = pvr_parser_get_seqno_bytime(pvr->pvr_parser, timems);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find in pvr parser\n", timems);
		return -1;
	}
	pvr_pos += pvr_parser_get_startpos_byseqno (pvr->pvr_parser, seqno, PVRFILE_TOTAL);
	seg_time = pvr_parser_get_starttime_byseqno(pvr->pvr_parser, seqno, PVRFILE_TOTAL);
	seg_time = (timems - seg_time);
	if (seg_time > 0) {
		off_t pos = 0;
		int64_t duration = pvr_parser_get_duration(pvr->pvr_parser, PVRFILE_TOTAL);
		off_t   filesize = pvr_parser_get_filesize(pvr->pvr_parser, PVRFILE_TOTAL);

		if (duration > 0)
			pos = (int64_t)(seg_time * ((float)filesize / (float)duration));
		pvr_pos = (((pos + pvr_pos) > filesize) ? filesize : (pos + pvr_pos));
	}

	return pvr_pos;
}

static void _get_pvr_info(PVRPvrContext *pvr, PVRInfo *info)
{
	info->tl_filesize = pvr_parser_get_filesize(pvr->pvr_parser, PVRFILE_TOTAL);
	info->tr_filesize = pvr_parser_get_filesize(pvr->pvr_parser, PVRFILE_EXIST);
	info->tl_duration = pvr_parser_get_duration(pvr->pvr_parser, PVRFILE_TOTAL);
	info->tr_duration = pvr_parser_get_duration(pvr->pvr_parser, PVRFILE_EXIST);
	return;
}

static void *pvr_open(char *url, mode_t mode)
{
	PVRPvrContext *pvr = NULL;
	int path_size = 0;

	pvr = av_mallocz(sizeof(PVRPvrContext));
	if (!pvr) {
		gxloge(" malloc fail\n");
		return NULL;
	}

	pvr->path = pvr_parse_path_remove_suffix(url, PVR_PVRFILE_SUFFIX, &path_size);
	if (!pvr->path) {
		gxloge(" [%s] path parse fail\n", url);
		goto pvr_open_error;
	}
	pvr_get_absolute_url(pvr->path, path_size);

	if (PVR_IS_WROOT(mode)) {
		if (pvr_file_exist(url)) {
#if 0
			PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", url));
			pvr_rm_file(url);
			PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", pvr->path));
			pvr_rm_dir(pvr->path);
#endif
			gxlogd("file: %s exist\n", url);
		}
		pvr->mkdir = pvr_mk_dir(pvr->path);
		if (pvr->mkdir < 0) {
			gxloge(" [%s] mkdir fail\n", url);
			goto pvr_open_error;
		}
		pvr->buffer_w = GxCore_PageMalloc(PVR_WBLOCK_SIZE, 10);
		if (NULL == pvr->buffer_w) {
			gxloge(" [%s] buffer_w malloc fail\n", url);
			goto pvr_open_error;
		}
		pvr->write_flag = 1;
	}

	pvr->pvr_parser = pvr_parser_alloc((const char *)url);
	if (NULL == pvr->pvr_parser) {
		gxloge(" [%s] pvr parser alloc fail\n", url);
		goto pvr_open_error;
	}

	_clear_seg_rw(&pvr->seg_rw);

	pvr->prev_segts_timems = (unsigned int)-1;
	pvr->prev_fsync_timems = (unsigned int)-1;
	pvr->prev_write_timems = (unsigned int)-1;
	pvr->pos_w    = 0;
	GxCore_MutexCreate(&pvr->mutex);
	return (void *)pvr;

pvr_open_error:
	if (pvr) {
		if (pvr->pvr_parser)
			pvr_parser_free(pvr->pvr_parser);
		if (pvr->path) {
			if (pvr->mkdir > 0)
				pvr_rm_dir(pvr->path);
			av_free(pvr->path);
		}
		av_free(pvr);
	}
	return NULL;
}

static int pvr_close(void *priv)
{
	unsigned int i = 0;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	GxCore_MutexLock(pvr->mutex);

	if (pvr->write_flag) {
		if (pvr->seg_rw.ts_fd)
			_write_l(pvr, pvr->seg_rw.ts_fd, NULL, 0, 1);
		if (pvr->buffer_w != NULL) {
			GxCore_PageFree(pvr->buffer_w);
			pvr->buffer_w = NULL;
		}
		_save_pvrfile(pvr);
	}

	_clear_seg_rw(&pvr->seg_rw);
	for (i = 0; i < PVR_MAX_EVT_COUNT; i++)
		_clear_evt_rw(&pvr->evt_rw[i]);
	for (i = 0; i < PVR_MAX_LST_COUNT; i++)
		_clear_lst_rw(&pvr->lst_rw[i]);

	if (pvr->pvr_parser) {
		off_t   filesize = pvr_parser_get_filesize(pvr->pvr_parser, PVRFILE_TOTAL);
		int64_t duration = pvr_parser_get_duration(pvr->pvr_parser, PVRFILE_TOTAL);

		if ((0 == filesize) || (0 == duration)) {
			PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr D]: %s\n", pvr->path));
			pvr_rm_dir(pvr->path);
		}

		pvr_parser_free(pvr->pvr_parser);
	}

	if (pvr->path)
		av_free(pvr->path);

	GxCore_MutexUnlock(pvr->mutex);

	GxCore_MutexDelete(pvr->mutex);
	av_free(pvr);

	return 0;
}

static int pvr_write(void *priv, uint8_t *buffer, size_t len)
{
	PVRPvrContext *pvr = (PVRPvrContext *)priv;
	unsigned int dis_timems = 0;
	unsigned int dis_offset = 0;
	int ret = 0;
	unsigned char *outbuf = NULL;
	unsigned int   outlen = 0;
	int   key_change = 0;
	char *key_curr = NULL;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);
	PVR_PVR_CHECK_NULL(pvr->seg_rw.parser);

	outbuf   = buffer;
	outlen   = len;
	GxCore_MutexLock(pvr->mutex);
	key_curr = pvr_parser_get_keydesc_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no);
	if (pvr->desc_key) {
		if (key_curr == NULL || strcmp(pvr->desc_key, key_curr))
			key_change = 1;
	} else {
		if (key_curr)
			key_change = 1;
	}

	dis_timems = pvr->new_timems - pvr->prev_segts_timems;
	if ((dis_timems > PVR_MAX_SEGTS_TIMEMS) || ((unsigned int)-1 == pvr->prev_segts_timems) || key_change) {
		if (pvr->seg_rw.ts_fd) {
			//write prev ts info to xxx.seg.mdvr
			char *line = pvr->tmpbuf;

			_write_l(pvr, pvr->seg_rw.ts_fd, NULL, 0, 1);
			dis_offset = recfile_getsize(pvr->seg_rw.ts_fd);
			dis_timems = pvr_parser_get_duration_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no);
			snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_DURATION, dis_timems);
			pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_OVERLAP);
			snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_FILESIZE, dis_offset);
			pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_APPEND);

			recfile_close(pvr->seg_rw.ts_fd);
			pvr->seg_rw.ts_fd = NULL;
		}
		pvr->prev_segts_timems = pvr->new_timems;
	}

	if (NULL == pvr->seg_rw.ts_fd) {
		char *url  = pvr->tmpbuf;
		char *line = pvr->tmpbuf;
		char *path = pvr_parser_get_arg(pvr->seg_rw.parser, PVRFILE_GET_PATH);

		pvr->seg_rw.ts_no = pvr_parser_get_seqcount(pvr->seg_rw.parser);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s/%07d%s",
				pvr->path,
				PVR_PVRFILE_SEGPATH,
				path, pvr->seg_rw.ts_no, PVR_STSFILE_SUFFIX);
		PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr W]: %s\n", url));
		pvr->seg_rw.ts_fd = recfile_open(url, PVR_RW_ROOT);
		if (NULL == pvr->seg_rw.ts_fd) {
			GxCore_MutexUnlock(pvr->mutex);
			gxloge(" [%s] open fail\n", url);
			return -1;
		}
		snprintf(line, PVR_MAX_LINE_LEN, "%s%07d\n", PVR_TS_PREFIX, pvr->seg_rw.ts_no);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		if (pvr->desc_key) {
			snprintf(line, PVR_MAX_LINE_LEN, "%s%s\n", PVR_TS_ENCRPYT, pvr->desc_key);
			pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_NORMAL);
		}
		pvr_parser_sync(pvr->seg_rw.parser);
	}

	ret = _write_l(pvr, pvr->seg_rw.ts_fd, outbuf, outlen, 0);
	if (ret > 0 ) {
		unsigned int i = 0;

		if ((unsigned int)-1 == pvr->prev_write_timems)
			pvr->prev_write_timems = pvr->new_timems;
		dis_timems = pvr->new_timems - pvr->prev_write_timems;
		dis_offset = ret;
		pvr_parser_add_timems(pvr->seg_rw.parser, dis_timems);
		pvr_parser_add_offset(pvr->seg_rw.parser, dis_offset);
		pvr_parser_add_timems(pvr->pvr_parser, dis_timems);
		pvr_parser_add_offset(pvr->pvr_parser, dis_offset);
		for (i = 0; i < PVR_MAX_EVT_COUNT; i++) {
			if (NULL == pvr->evt_rw[i].parser)
				continue;
			if (NULL == pvr->evt_rw[i].prefix)
				continue;
			if ((-1 != pvr->seg_rw.seg_no) &&
					(pvr->seg_rw.seg_no == pvr->evt_rw[i].evt_seg_no)) {
				pvr_parser_add_timems(pvr->evt_rw[i].parser, dis_timems);
				pvr_parser_add_offset(pvr->evt_rw[i].parser, dis_offset);
			}
		}
		pvr->prev_write_timems = pvr->new_timems;
	}

	dis_timems = pvr->new_timems - pvr->prev_fsync_timems;
	if ((dis_timems > PVR_MAX_FSYNC_TIMEMS) || (pvr->prev_fsync_timems == (unsigned int)-1)) {
		int64_t seg_duration = 0;
		off_t   seg_filesize = 0;
		char *line = pvr->tmpbuf;

		seg_duration = pvr_parser_get_duration(pvr->seg_rw.parser, PVRFILE_EXIST);
		seg_filesize = pvr_parser_get_filesize(pvr->seg_rw.parser, PVRFILE_EXIST);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_DURATION, seg_duration);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_OVERLAP);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%lld\n", PVR_SEG_FILESIZE, seg_filesize);
		pvr_parser_write_line(pvr->pvr_parser, line, PVRFILE_APPEND);

		dis_timems = pvr_parser_get_duration_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no);
		dis_offset = recfile_getsize(pvr->seg_rw.ts_fd);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_DURATION, dis_timems);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_OVERLAP);
		snprintf(line, PVR_MAX_LINE_LEN, "%s%d\n", PVR_TS_FILESIZE, dis_offset);
		pvr_parser_write_line(pvr->seg_rw.parser, line, PVRFILE_APPEND);

		_save_evtfile(pvr, 0);

		recfile_fsync(pvr->seg_rw.ts_fd);
		pvr_parser_sync(pvr->seg_rw.parser);
		pvr_parser_sync(pvr->pvr_parser);
		pvr->prev_fsync_timems = pvr->new_timems;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return ret;
}

static int pvr_read(void *priv, uint8_t *buffer, size_t len)
{
	int ret = 0;
	int seqno = -1;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;
	unsigned char *inbuf = NULL;
	unsigned int   inlen = 0;
	unsigned int   rlen = 0;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);

	inbuf = buffer;
	inlen = len;
	GxCore_MutexLock(pvr->mutex);
pvr_read_retry0:
	if (NULL == pvr->seg_rw.parser) {
		char *url  = pvr->tmpbuf;
		char *url0 = pvr->tmpurl;

		seqno = pvr_parser_next_url(pvr->pvr_parser, url0, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			GxCore_MutexUnlock(pvr->mutex);
			return 0;
		}
		pvr->seg_rw.seg_no = seqno;
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s", pvr->path, url0);
		pvr->seg_rw.parser = pvr_parser_alloc((const char *)url);
		if (NULL == pvr->seg_rw.parser) {
			GxCore_MutexUnlock(pvr->mutex);
			gxloge(" [%s] seg parser alloc fail\n", url);
			return -1;
		}
	}
pvr_read_retry1:
	if (NULL == pvr->seg_rw.ts_fd) {
		char *url  = pvr->tmpbuf;
		char *url0 = pvr->tmpurl;
		char *path = pvr_parser_get_arg(pvr->pvr_parser, PVRFILE_GET_PATH);

		seqno = pvr_parser_next_url(pvr->seg_rw.parser, url0, PVR_MAX_URL_LEN);
		if (-1 == seqno) {
			pvr_parser_free(pvr->seg_rw.parser);
			pvr->seg_rw.parser = NULL;
			goto pvr_read_retry0;
		}
		pvr->seg_rw.ts_no = seqno;
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", pvr->path, path, url0);
		PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr R]: %s\n", url));
		pvr->seg_rw.ts_fd = recfile_open(url, PVR_RW_ROOT);
		if (NULL == pvr->seg_rw.ts_fd) {
			GxCore_MutexUnlock(pvr->mutex);
			gxloge(" [%s] open fail\n", url);
			return -1;
		}
	}

	ret = recfile_read(pvr->seg_rw.ts_fd, inbuf + rlen, inlen - rlen);
	if (ret < 0) {
		gxloge("read fail !!!!\n");
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	rlen += ret;
	if (rlen < len) {
		if (ret == 0) {
			int  ts_count = pvr_parser_get_seqcount(pvr->seg_rw.parser);
			int seg_count = pvr_parser_get_seqcount(pvr->pvr_parser);

			if (((pvr->seg_rw.ts_no + 1) < ts_count) ||
					((pvr->seg_rw.seg_no + 1) < seg_count)) {
				recfile_close(pvr->seg_rw.ts_fd);
				pvr->seg_rw.ts_fd = NULL;
				goto pvr_read_retry1;
			} else {
				GxCore_MutexUnlock(pvr->mutex);
				return rlen;
			}
		}
	}
	pvr->desc_key = pvr_parser_get_keydesc_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no);
	GxCore_MutexUnlock(pvr->mutex);

	return rlen;
}

static off_t pvr_seek(void *priv, off_t offset)
{
	int seqno = -1;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;
	off_t pvr_pos = 0, seg_pos = 0, ts_pos = 0;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);

	GxCore_MutexLock(pvr->mutex);
	if ((offset == 0) && (pvr->seg_rw.seg_no == -1)) {
		GxCore_MutexUnlock(pvr->mutex);
		return 0;
	}
	//find segfile by pvrfile
	pvr_pos = offset;
	seqno = pvr_parser_get_seqno_bypos(pvr->pvr_parser, pvr_pos);
	if (-1 == seqno) {
		if (pvr_parser_get_seqcount(pvr->pvr_parser) == 0) {
			GxCore_MutexUnlock(pvr->mutex);
			return 0;
		} else {
			gxloge(" [%lld] seqno not find in pvr parser\n", pvr_pos);
			goto pvr_seek_error;
		}
	} else {//check seqno exist or not
		int32_t tmp_seqno = pvr_parser_get_seqno_byseqno(pvr->pvr_parser, seqno);
		if (-1 == tmp_seqno) {
			gxloge(" [%lld] seqno not find in pvr parser\n", pvr_pos);
			goto pvr_seek_error;
		}
		if (seqno < tmp_seqno) {
			seqno = tmp_seqno;
			pvr_pos = pvr_parser_get_startpos_byseqno(pvr->pvr_parser, seqno, PVRFILE_TOTAL);
		}
	}

	if (seqno != pvr->seg_rw.seg_no) {
		_clear_seg_rw(&pvr->seg_rw);
	}
	if (NULL == pvr->seg_rw.parser) {
		char *url  = pvr->tmpbuf;
		char *url0 = pvr->tmpurl;

		pvr->seg_rw.seg_no = pvr_parser_get_url_byseqno(pvr->pvr_parser, seqno, url0, PVR_MAX_URL_LEN);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s", pvr->path, url0);
		pvr->seg_rw.parser = pvr_parser_alloc(url);
		if (NULL == pvr->seg_rw.parser) {
			gxloge(" [%s] seg parser alloc fail\n", url);
			goto pvr_seek_error;
		}
	}

	//find tsfile
	seg_pos = pvr_pos - pvr_parser_get_startpos_byseqno(pvr->pvr_parser, seqno, PVRFILE_TOTAL);
	if (seg_pos < 0) {
		gxloge(" [%d] startpos not find in pvr parser\n", seqno);
		goto pvr_seek_error;
	}
	seqno = pvr_parser_get_seqno_bypos(pvr->seg_rw.parser, seg_pos);
	if (-1 == seqno) {
		gxloge(" [%lld] seqno not find int seg parser\n", seg_pos);
		goto pvr_seek_error;
	}
	if (seqno != pvr->seg_rw.ts_no) {
		if (pvr->seg_rw.ts_fd)
			recfile_close(pvr->seg_rw.ts_fd);
		pvr->seg_rw.ts_fd = NULL;
	}
	if (NULL == pvr->seg_rw.ts_fd) {
		char *url  = pvr->tmpbuf;
		char *url0 = pvr->tmpurl;
		char *path = pvr_parser_get_arg(pvr->pvr_parser, PVRFILE_GET_PATH);

		pvr->seg_rw.ts_no = pvr_parser_get_url_byseqno(pvr->seg_rw.parser, seqno, url0, PVR_MAX_URL_LEN);
		snprintf(url, PVR_MAX_URL_LEN, "%s/%s/%s", pvr->path, path, url0);
		PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr S]: %s\n", url));
		pvr->seg_rw.ts_fd = recfile_open(url, PVR_RW_ROOT);
		if (NULL == pvr->seg_rw.ts_fd) {
			gxloge(" [%s] open fail\n", url);
			goto pvr_seek_error;
		}
	}
	ts_pos = seg_pos - pvr_parser_get_startpos_byseqno(pvr->seg_rw.parser, seqno, PVRFILE_TOTAL);
	if (ts_pos < 0) {
		gxloge("[%d] startpos not find in seg parser\n", seqno);
		goto pvr_seek_error;
	}
	recfile_seek(pvr->seg_rw.ts_fd, ts_pos, SEEK_SET);
	GxCore_MutexUnlock(pvr->mutex);
	return pvr_pos;

pvr_seek_error:
	GxCore_MutexUnlock(pvr->mutex);
	return -1;
}

static int pvr_getinfo(void *priv, PVRInfo *info)
{
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);

	GxCore_MutexLock(pvr->mutex);
	_get_pvr_info(pvr, info);
	GxCore_MutexUnlock(pvr->mutex);

	return 0;
}

static off_t pvr_getpos(void *priv)
{
	off_t pvr_pos = 0;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);

	GxCore_MutexLock(pvr->mutex);
	if (pvr->seg_rw.seg_no >= 0) {
		pvr_pos += pvr_parser_get_startpos_byseqno(pvr->pvr_parser, pvr->seg_rw.seg_no, PVRFILE_TOTAL);
		if (pvr->seg_rw.parser && (pvr->seg_rw.ts_no >= 0)) {
			pvr_pos += pvr_parser_get_startpos_byseqno(pvr->seg_rw.parser, pvr->seg_rw.ts_no, PVRFILE_TOTAL);
			if (pvr->seg_rw.ts_fd)
				pvr_pos += recfile_getpos(pvr->seg_rw.ts_fd);
		}
	}
	GxCore_MutexUnlock(pvr->mutex);

	return pvr_pos;
}

static int pvr_settime(void *priv, unsigned int timems)
{
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	GxCore_MutexLock(pvr->mutex);
	pvr->new_timems = timems;
	GxCore_MutexUnlock(pvr->mutex);

	return 0;
}

static size_t pvr_probehdr(void *priv)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	GxCore_MutexLock(pvr->mutex);
	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_HDRFILE);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr P]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(pvr->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_getsize(fd);
	recfile_close(fd);
	GxCore_MutexUnlock(pvr->mutex);

	return size;
}

static size_t pvr_readhdr(void *priv, uint8_t *buffer, size_t len)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	GxCore_MutexLock(pvr->mutex);
	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_HDRFILE);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr R]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(pvr->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_read(fd, buffer, len);
	recfile_close(fd);
	GxCore_MutexUnlock(pvr->mutex);

	return size;
}

static size_t pvr_writehdr(void *priv, uint8_t *buffer, size_t len)
{
	size_t size = 0;
	char *url = NULL;
	struct record_file *fd = NULL;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	GxCore_MutexLock(pvr->mutex);
	url = pvr->tmpbuf;
	snprintf(url, PVR_MAX_URL_LEN, "%s/%s", pvr->path, PVR_HDRFILE);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> [pvr pvr W]: %s\n", url));
	fd = recfile_open(url, PVR_RW_ROOT);
	if (NULL == fd) {
		GxCore_MutexUnlock(pvr->mutex);
		gxloge(" [%s] open fail\n", url);
		return -1;
	}
	size = recfile_write(fd, buffer, len);
	recfile_close(fd);
	GxCore_MutexUnlock(pvr->mutex);

	return size;
}

static int pvr_setctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);

	PVR_PVR_DEBUG_PRINT(gxlogi("--> %s %d [%x, %p]\n", __func__, __LINE__, ctrl->opt, ctrl->arg));
	GxCore_MutexLock(pvr->mutex);
	PVR_PVR_DEBUG_PRINT(gxlogi("--> %s %d [%x, %p]\n", __func__, __LINE__, ctrl->opt, ctrl->arg));
	switch (ctrl->opt) {
	case GX_RECORD_PVR_NEW_SEGMENT:
		{
			GxRecordPVRSegment *tmpseg = (GxRecordPVRSegment *)ctrl->arg;
			GxRecordPVRSegment seg = {
				.prefix = PVR_SEGFILE_DEF_PREFIX,
				.desc = PVR_SEGFILE_DEF_DESC
			};
			if (tmpseg) {
				if (tmpseg->prefix)
					seg.prefix = tmpseg->prefix;
				if (tmpseg->desc)
					seg.desc = tmpseg->desc;
			}
			ret = _new_pvr_seg(pvr, &seg);
		}
		break;
	case GX_RECORD_PVR_DELETE_SEGMENT:
		ret = _delete_pvr_seg(pvr, (const char *)ctrl->arg);
		break;
	case GX_RECORD_PVR_OPEN_EVENT:
		ret = _open_pvr_evt(pvr, (GxRecordPVREventSet *)ctrl->arg);
		break;
	case GX_RECORD_PVR_CLOSE_EVENT:
		ret = _close_pvr_evt(pvr, (const char *)ctrl->arg);
		break;
	case GX_RECORD_PVR_DELETE_EVENT:
		ret = _delete_pvr_evt(pvr, (const char *)ctrl->arg);
		break;
	case GX_RECORD_PVR_OPEN_LIST:
		ret = _open_pvr_lst(pvr, (GxRecordPVRListSet *)ctrl->arg);
		break;
	case GX_RECORD_PVR_CLOSE_LIST:
		ret = _close_pvr_lst(pvr, (const char *)ctrl->arg);
		break;
	case GX_RECORD_PVR_DELETE_LIST:
		ret = _delete_pvr_lst(pvr, (const char *)ctrl->arg);
		break;
	case GX_RECORD_PVR_ADD_SEGMENT_IN_EVENT:
		ret = _add_pvr_seg_to_evt(pvr, (GxRecordPVRElem *)ctrl->arg);
		break;
	case GX_RECORD_PVR_REMOVE_SEGMENT_IN_EVENT:
		ret = _remove_pvr_seg_in_evt(pvr, (GxRecordPVRElem *)ctrl->arg);
		break;
	case GX_RECORD_PVR_ADD_EVENT_IN_LIST:
		ret = _add_pvr_evt_to_lst(pvr, (GxRecordPVRElem *)ctrl->arg);
		break;
	case GX_RECORD_PVR_REMOVE_EVENT_IN_LIST:
		ret = _remove_pvr_evt_in_lst(pvr, (GxRecordPVRElem *)ctrl->arg);
		break;
	case GX_RECORD_PVR_FREE_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;

			_free_file_info(&info->file);
			if (info->pvr_file)
				av_free((void *)info->pvr_file);
			if (info->dir)
				av_free((void *)info->dir);
			if (info->path)
				av_free((void *)info->path);
			if (info->prefix)
				av_free((void *)info->prefix);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_SET_KEY_DESC:
		{
			pvr->desc_key = (char *)ctrl->arg;
			ret = 0;
		}
		break;
	default:
		gxloge(" [%x] pvr option not find\n", ctrl->opt);
		break;
	}
	PVR_PVR_DEBUG_PRINT(gxlogi("> %s %d\n", __func__, __LINE__));
	GxCore_MutexUnlock(pvr->mutex);
	PVR_PVR_DEBUG_PRINT(gxlogi("> %s %d\n", __func__, __LINE__));
	return ret;
}

static int pvr_getctrl(void *priv, GxRecordPVRControl *ctrl)
{
	int ret = -1;
	PVRPvrContext *pvr = (PVRPvrContext *)priv;

	PVR_PVR_CHECK_NULL(pvr);
	PVR_PVR_CHECK_NULL(pvr->pvr_parser);

	GxCore_MutexLock(pvr->mutex);
	switch (ctrl->opt) {
	case GX_RECORD_PVR_GET_DURATION:
	case GX_RECORD_PVR_GET_FILESIZE:
		{
			PVRInfo info;
			_get_pvr_info(pvr, &info);
			if (ctrl->opt == GX_RECORD_PVR_GET_DURATION)
				*(int64_t *)ctrl->arg = info.tl_duration;
			else if (ctrl->opt == GX_RECORD_PVR_GET_FILESIZE)
				*(int64_t *)ctrl->arg = info.tl_filesize;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_CURTIME:
		{
			*(int64_t *)ctrl->arg = _get_pvr_curtime(pvr);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_MINTIME:
		{
			*(int64_t *)ctrl->arg = _get_pvr_mintime(pvr);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_POS_BY_TIME:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->offset = _get_pvr_posbytime(pvr, time_pos->timems);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_TIME_BY_POS:
		{
			GxRecordPVRTimePos *time_pos = (GxRecordPVRTimePos *)ctrl->arg;

			time_pos->timems = _get_pvr_timebypos(pvr, time_pos->offset);
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_INFO:
		{
			GxRecordPVRInfo *info = (GxRecordPVRInfo *)ctrl->arg;
			char *pvr_file = pvr->tmpbuf;

			info->type = GX_RECORD_PVR_PVR_FILE;
			_alloc_file_info(pvr, &info->file);

			snprintf(pvr_file, PVR_MAX_URL_LEN, "%s%s", pvr->path, PVR_PVRFILE_SUFFIX);
			pvr_get_absolute_url(pvr_file, PVR_MAX_URL_LEN);
			info->pvr_file = av_strdup(pvr_file);
			info->dir      = av_strdup(pvr->path);
			info->prefix   = NULL;
			info->path     = NULL;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_PVR_TYPE:
		{
			GxRecordPVRFileType *type = (GxRecordPVRFileType *)ctrl->arg;

			*type = GX_RECORD_PVR_PVR_FILE;
			ret = 0;
		}
		break;
	case GX_RECORD_PVR_GET_KEY_DESC:
		{
			ctrl->arg = (void *)pvr->desc_key;
			ret = 0;
		}
		break;
	default:
		gxloge(" [%x] pvr option not find\n", ctrl->opt);
		break;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return ret;
}

PVROps pvr_pvr_ops = {
	.open         = pvr_open,
	.close        = pvr_close,
	.write        = pvr_write,
	.read         = pvr_read,
	.seek         = pvr_seek,
	.getinfo      = pvr_getinfo,
	.getpos       = pvr_getpos,
	.settime      = pvr_settime,
	.probehdr     = pvr_probehdr,
	.readhdr      = pvr_readhdr,
	.writehdr     = pvr_writehdr,
	.setctrl      = pvr_setctrl,
	.getctrl      = pvr_getctrl,
};
