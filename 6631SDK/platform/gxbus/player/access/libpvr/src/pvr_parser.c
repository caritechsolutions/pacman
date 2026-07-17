#include "gx_common.h"
#include "gx_system.h"
#include "gx_mem.h"
#include "avstring.h"
#include "recfs.h"
#include "pvr_config.h"
#include "pvr_file.h"
#include "pvr_parser.h"
#include "pvr_utils.h"

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

typedef struct {
	unsigned int  count;
	char         *url;
	void         *priv;
} PVRManager;

#define PVR_MAX_MANAGER_COUNT (64)
static PVRManager pvr_manager[PVR_MAX_MANAGER_COUNT];
static handle_t   pvr_manager_mutex = -1;

#define PVR_PARSER_CHECK_NULL(ptr, ret) do {         \
	if (ptr == NULL) {                               \
		gxloge("ptr is NULL\n", __func__, __LINE__); \
		return ret;                                  \
	}                                                \
} while(0)

#define PVR_PARSER_DEBUG_PRINT(printf_func) do {             \
	int debug_flags = 0;                                     \
	GxPlayer_SystemGet(PSYS_DEBUG_STREAM_PVR, &debug_flags); \
	if (debug_flags)                                         \
		printf_func;                                         \
} while(0)


static void _manager_init(void)
{
	if (pvr_manager_mutex == -1) {
		GxCore_MutexCreate(&(pvr_manager_mutex));
		memset(pvr_manager, 0, PVR_MAX_MANAGER_COUNT * sizeof(PVRManager));
	}
}

static PVRManager *_manager_alloc(const char *url, PVRFileType type, char *newflag)
{
	PVRManager *manager = NULL;
	unsigned int i = 0;

	_manager_init();
	GxCore_MutexLock(pvr_manager_mutex);
	for (i = 0; i < PVR_MAX_MANAGER_COUNT; i++) {
		if (0 == pvr_manager[i].count) {
			manager = &pvr_manager[i];
			continue;
		}
		if (0 == strcmp(pvr_manager[i].url, url)) {
			manager = &pvr_manager[i];
			break;
		}
	}
	if (manager) {
		if (manager->count == 0) {
			if (manager->url || manager->priv)
				gxloge(" [%p %p] not NULL\n", manager->url, manager->priv);
			manager->priv = pvr_file_create(type);
			manager->url  = av_strdup(url);
			*newflag = 1;
		}
		manager->count++;
		PVR_PARSER_DEBUG_PRINT(gxlogi("--> [manager O(%d)]: %s\n", manager->count, manager->url));
	} else {
		gxloge(" manager is NULL\n");
	}

	GxCore_MutexUnlock(pvr_manager_mutex);

	return manager;
}

static void _manager_free(PVRManager *manager, PVRFileType type, char *endflag)
{
	unsigned int i = 0;

	_manager_init();
	GxCore_MutexLock(pvr_manager_mutex);
	if (manager->count > 0) {
		manager->count--;
		PVR_PARSER_DEBUG_PRINT(gxlogi("--> [manager C(%d)]: %s\n", manager->count, manager->url));
		if (manager->count == 0) {
			if (!manager->url || !manager->priv)
				gxloge(" [%p %p] is NULL\n", manager->url, manager->priv);
			av_free(manager->url);
			pvr_file_destory(type, manager->priv);
			manager->url  = NULL;
			manager->priv = NULL;
			*endflag = 1;
		}
	} else {
		gxloge(" manager is NULL\n");
	}
	GxCore_MutexUnlock(pvr_manager_mutex);
}

static PVRManager *_manager_find(const char *url)
{
	PVRManager *manager = NULL;
	unsigned int i = 0;

	_manager_init();
	GxCore_MutexLock(pvr_manager_mutex);
	for (i = 0; i < PVR_MAX_MANAGER_COUNT; i++) {
		if (0 == pvr_manager[i].count)
			continue;
		if (0 == strcmp(pvr_manager[i].url, url)) {
			manager = &pvr_manager[i];
			break;
		}
	}
	GxCore_MutexUnlock(pvr_manager_mutex);

	return manager;
}

PVRParser *pvr_parser_alloc(const char *url)
{
	int path_size = 0;
	char newflag = 0;
	PVRManager *manager = NULL;
	PVRParser  *parser = NULL;

	parser = av_mallocz(sizeof(PVRParser));
	if (NULL == parser)
		return NULL;

	if (av_stristr(url, PVR_SEGFILE_SUFFIX))
		parser->type = PVRFILE_SEG_FILE;
	else if (av_stristr(url, PVR_EVTFILE_SUFFIX))
		parser->type = PVRFILE_EVT_FILE;
	else if (av_stristr(url, PVR_LSTFILE_SUFFIX))
		parser->type = PVRFILE_LST_FILE;
	else if (av_stristr(url, PVR_PVRFILE_SUFFIX))
		parser->type = PVRFILE_PVR_FILE;
	else {
		av_free(parser);
		return NULL;
	}

	manager = _manager_alloc(url, parser->type, &newflag);
	if (NULL == manager) {
		gxloge(" [%d] manager alloc fail\n", url);
		goto SEG_ERROR_EXIT;
	}

	parser->fd = recfile_open(url, PVR_RW_ROOT);
	if (parser->fd == NULL) {
		gxloge(" [%s] url open fail\n", url);
		goto SEG_ERROR_EXIT;
	}
	parser->mpriv = (void *)(manager);
	parser->fpriv = (void *)(manager->priv);

	if (newflag) {
		PVRLineCache  *pcache = NULL;
		char *pline  = NULL;

		pcache = av_malloc(sizeof(PVRLineCache));
		pline  = av_malloc(PVR_MAX_LINE_LEN);
		pvr_cache_reset(pcache);
		while (1) {
			int ret = pvr_cache_read_line(parser->fd, pcache, pline, PVR_MAX_LINE_LEN);
			if (ret <= 0)
				break;
			pvr_file_parse(parser->type, parser->fpriv, pline);
		}
		av_free(pcache);
		av_free(pline);
	}
	parser->path = pvr_parse_path((char *)url, &path_size);
	parser->overlap_offset = -1;

	return parser;

SEG_ERROR_EXIT:
	if (parser) {
		if (manager) {
			char endflag = 0;
			_manager_free(manager, parser->type, &endflag);
		}
		if (parser->fd)
			recfile_close(parser->fd);
		av_free(parser);
	}
	return NULL;
}

int pvr_parser_set_seqno(PVRParser *parser, int seqno)
{
	int ret = -1;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	parser->seqno = seqno;

	return 0;
}

char *pvr_parser_get_curpath(PVRParser *parser)
{
	return parser->path;
}

int pvr_parser_exist(const char *url)
{
	PVRManager *manager = _manager_find(url);

	return (manager == NULL) ? 0 : 1;
}

void pvr_parser_free(PVRParser *parser)
{
	char endflag = 0;

	if (parser == NULL) {
		gxloge(" parser is NULL\n");
		return;
	}

	_manager_free(parser->mpriv, parser->type, &endflag);
	if (parser->fd) {
		if (endflag)
			recfile_fsync(parser->fd);
		recfile_close(parser->fd);
	}
	if (parser->path)
		av_free(parser->path);
	parser->fd    = NULL;
	parser->path  = NULL;
	parser->fpriv = NULL;
	parser->mpriv = NULL;
	parser->overlap_offset = -1;
	av_free(parser);

	return;
}

int pvr_parser_write_line(PVRParser *parser, char *line, PVRFileMode mode)
{
	if (parser == NULL) {
		gxloge(" parser is NULL\n");
		return -1;
	}
	PVR_PARSER_DEBUG_PRINT(gxlogi(">> %s\n", line));

	pvr_file_parse(parser->type, parser->fpriv, line);
	if (mode == PVRFILE_OVERLAP) {
		if (parser->overlap_offset != -1)
			recfile_seek(parser->fd, parser->overlap_offset, SEEK_SET);
		else
			parser->overlap_offset = recfile_getpos(parser->fd);
	} else if (mode == PVRFILE_APPEND) {
	} else
		parser->overlap_offset = -1;
	recfile_write(parser->fd, line, strlen(line));

	return 0;
}

int pvr_parser_sync(PVRParser *parser)
{
	recfile_fsync(parser->fd);

	return 0;
}

int pvr_parser_next_url(PVRParser *parser, char *url, unsigned int max_bytes)
{
	int seqno = -1;
	int ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		seqno = pvr_pvrfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &parser->seqno);
	else if (parser->type == PVRFILE_SEG_FILE)
		seqno = pvr_segfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &parser->seqno);
	else if (parser->type == PVRFILE_EVT_FILE)
		seqno = pvr_evtfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &parser->seqno);
	else if (parser->type == PVRFILE_LST_FILE)
		seqno = pvr_lstfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &parser->seqno);
	if (seqno < 0)
		return -1;

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_url(parser->fpriv, seqno, url, max_bytes);
	if (ret < 0)
		return -1;

	parser->seqno = seqno + 1;

	return seqno;
}

int32_t pvr_parser_get_url_byseqno(PVRParser *parser,
		int32_t seqno,
		char *url,
		unsigned int max_bytes)
{
	int ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_url(parser->fpriv, seqno, url, max_bytes);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_url(parser->fpriv, seqno, url, max_bytes);

	parser->seqno = seqno + 1;

	return seqno;
}

char *pvr_parser_get_arg(PVRParser *parser, PVRFileArgGetCmd cmd)
{
	char *arg = NULL;

	PVR_PARSER_CHECK_NULL(parser, arg);
	PVR_PARSER_CHECK_NULL(parser->fpriv, arg);

	if (parser->type == PVRFILE_PVR_FILE)
		arg = pvr_pvrfile_get_arg(parser->fpriv, cmd);
	else if (parser->type == PVRFILE_SEG_FILE)
		arg = pvr_segfile_get_arg(parser->fpriv, cmd);
	else if (parser->type == PVRFILE_EVT_FILE)
		arg = pvr_evtfile_get_arg(parser->fpriv, cmd);
	else if (parser->type == PVRFILE_LST_FILE)
		arg = pvr_lstfile_get_arg(parser->fpriv, cmd);

	return arg;
}

int64_t pvr_parser_get_duration(PVRParser *parser, PVRFileAttr attr)
{
	int64_t duration = 0;

	PVR_PARSER_CHECK_NULL(parser, duration);
	PVR_PARSER_CHECK_NULL(parser->fpriv, duration);

	if (parser->type == PVRFILE_PVR_FILE)
		duration = pvr_pvrfile_get_duration(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_SEG_FILE)
		duration = pvr_segfile_get_duration(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_EVT_FILE)
		duration = pvr_evtfile_get_duration(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_LST_FILE)
		duration = pvr_lstfile_get_duration(parser->fpriv, -1, attr);

	return duration;
}

off_t pvr_parser_get_filesize(PVRParser *parser, PVRFileAttr attr)
{
	off_t filesize = 0;

	PVR_PARSER_CHECK_NULL(parser, filesize);
	PVR_PARSER_CHECK_NULL(parser->fpriv, filesize);

	if (parser->type == PVRFILE_PVR_FILE)
		filesize = pvr_pvrfile_get_filesize(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_SEG_FILE)
		filesize = pvr_segfile_get_filesize(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_EVT_FILE)
		filesize = pvr_evtfile_get_filesize(parser->fpriv, -1, attr);
	else if (parser->type == PVRFILE_LST_FILE)
		filesize = pvr_lstfile_get_filesize(parser->fpriv, -1, attr);

	return filesize;
}

char pvr_parser_get_isfinish(PVRParser *parser)
{
	char isfinish = 1;

	PVR_PARSER_CHECK_NULL(parser, isfinish);
	PVR_PARSER_CHECK_NULL(parser->fpriv, isfinish);

	if (parser->type == PVRFILE_SEG_FILE)
		isfinish = pvr_segfile_get_isfinish(parser->fpriv);
	else
		gxloge("%s %d [%d]\n", __func__, __LINE__, parser->type);

	return isfinish;
}

int pvr_parser_set_duration_byseqno(PVRParser *parser, int seqno, int64_t duration)
{
	int ret = -1;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_set_duration_byseqno(parser->fpriv, seqno, duration);
	else
		gxloge("%s %d [%d]\n", __func__, __LINE__, parser->type);

	return ret;
}

int pvr_parser_set_filesize_byseqno(PVRParser *parser, int seqno, off_t filesize)
{
	int ret = -1;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_set_filesize_byseqno(parser->fpriv, seqno, filesize);
	else
		gxloge("%s %d [%d]\n", __func__, __LINE__, parser->type);

	return ret;
}

char pvr_parser_get_isfinish_byseqno(PVRParser *parser, int seqno)
{
	char isfinish = 1;
	PVR_PARSER_CHECK_NULL(parser, isfinish);
	PVR_PARSER_CHECK_NULL(parser->fpriv, isfinish);

	if (parser->type == PVRFILE_LST_FILE)
		isfinish = pvr_lstfile_get_isfinish_byseqno(parser->fpriv, seqno);
	else
		gxloge("%s %d [%d]\n", __func__, __LINE__, parser->type);

	return isfinish;
}

int pvr_parser_set_isfinish_byseqno(PVRParser *parser, int seqno, char isfinish)
{
	int ret = -1;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_set_isfinish_byseqno(parser->fpriv, seqno, isfinish);
	else
		gxloge("%s %d [%d]\n", __func__, __LINE__, parser->type);

	return ret;
}

char *pvr_parser_get_keydesc_byseqno(PVRParser *parser, int seqno)
{
	char *key_desc = NULL;

	PVR_PARSER_CHECK_NULL(parser, key_desc);
	PVR_PARSER_CHECK_NULL(parser->fpriv, key_desc);
	if (seqno < 0) return key_desc;

	if (parser->type == PVRFILE_SEG_FILE)
		key_desc = pvr_segfile_get_keydesc(parser->fpriv, seqno);

	return key_desc;
}

int64_t pvr_parser_get_duration_byseqno(PVRParser *parser, int seqno)
{
	int64_t duration = 0;

	PVR_PARSER_CHECK_NULL(parser, duration);
	PVR_PARSER_CHECK_NULL(parser->fpriv, duration);
	if (seqno < 0) return duration;

	if (parser->type == PVRFILE_PVR_FILE)
		duration = pvr_pvrfile_get_duration(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_SEG_FILE)
		duration = pvr_segfile_get_duration(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_EVT_FILE)
		duration = pvr_evtfile_get_duration(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_LST_FILE)
		duration = pvr_lstfile_get_duration(parser->fpriv, seqno, PVRFILE_TOTAL);

	return duration;
}

off_t pvr_parser_get_filesize_byseqno(PVRParser *parser, int seqno)
{
	off_t filesize = 0;

	PVR_PARSER_CHECK_NULL(parser, filesize);
	PVR_PARSER_CHECK_NULL(parser->fpriv, filesize);
	if (seqno < 0) return filesize;

	if (parser->type == PVRFILE_PVR_FILE)
		filesize = pvr_pvrfile_get_filesize(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_SEG_FILE)
		filesize = pvr_segfile_get_filesize(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_EVT_FILE)
		filesize = pvr_evtfile_get_filesize(parser->fpriv, seqno, PVRFILE_TOTAL);
	else if (parser->type == PVRFILE_LST_FILE)
		filesize = pvr_lstfile_get_filesize(parser->fpriv, seqno, PVRFILE_TOTAL);

	return filesize;
}

uint32_t pvr_parser_get_seqcount(PVRParser *parser)
{
	uint32_t seqcount = 0;

	PVR_PARSER_CHECK_NULL(parser, seqcount);
	PVR_PARSER_CHECK_NULL(parser->fpriv, seqcount);

	if (parser->type == PVRFILE_PVR_FILE)
		seqcount = pvr_pvrfile_get_seqcount(parser->fpriv);
	else if (parser->type == PVRFILE_SEG_FILE)
		seqcount = pvr_segfile_get_seqcount(parser->fpriv);
	else if (parser->type == PVRFILE_EVT_FILE)
		seqcount = pvr_evtfile_get_seqcount(parser->fpriv);
	else if (parser->type == PVRFILE_LST_FILE)
		seqcount = pvr_lstfile_get_seqcount(parser->fpriv);

	return seqcount;
}

int pvr_parser_add_timems(PVRParser *parser, int64_t timems)
{
	int ret = 0;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_add_timems(parser->fpriv, timems);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_add_timems(parser->fpriv, timems);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_add_timems(parser->fpriv, timems);

	if (ret < 0)
		gxloge("[%d - %lld] timems add fail\n", parser->type, timems);

	return ret;
}

int pvr_parser_add_offset(PVRParser *parser, int64_t offset)
{
	int ret = 0;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_add_offset(parser->fpriv, offset);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_add_offset(parser->fpriv, offset);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_add_offset(parser->fpriv, offset);

	if (ret < 0)
		gxloge("[%d - %lld] offset add fail\n", parser->type, offset);

	return ret;
}

int32_t pvr_parser_get_seqno_bypos(PVRParser *parser, off_t pos)
{
	int32_t ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPOS, (void*)&pos);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPOS, (void*)&pos);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPOS, (void*)&pos);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPOS, (void*)&pos);

	return ret;
}

int32_t pvr_parser_get_seqno_bytime(PVRParser *parser, int64_t timems)
{
	int32_t ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYTIME, (void*)&timems);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYTIME, (void*)&timems);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYTIME, (void*)&timems);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYTIME, (void*)&timems);

	return ret;
}

int32_t pvr_parser_get_seqno_byprefix(PVRParser *parser, char *prefix)
{
	int32_t ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPREFIX, (void*)prefix);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPREFIX, (void*)prefix);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPREFIX, (void*)prefix);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYPREFIX, (void*)prefix);

	return ret;
}

int32_t pvr_parser_get_seqno_byseqno(PVRParser *parser, int32_t seqno)
{
	int32_t ret = -1;

	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &seqno);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &seqno);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &seqno);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_seqno(parser->fpriv, PVRFILE_GET_SEQ_BYSEQ, &seqno);

	return ret;
}

off_t pvr_parser_get_startpos_byseqno(PVRParser *parser, int32_t seqno, PVRFileAttr attr)
{
	off_t ret = 0;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_startpos(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_startpos(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_startpos(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_startpos(parser->fpriv, seqno, attr);

	return ret;
}

int64_t pvr_parser_get_starttime_byseqno(PVRParser *parser, int32_t seqno, PVRFileAttr attr)
{
	int64_t ret = 0;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_starttime(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_starttime(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_starttime(parser->fpriv, seqno, attr);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_starttime(parser->fpriv, seqno, attr);

	return ret;
}

int64_t pvr_parser_get_mintime(PVRParser *parser)
{
	int64_t ret = 0;
	PVR_PARSER_CHECK_NULL(parser, ret);
	PVR_PARSER_CHECK_NULL(parser->fpriv, ret);

	if (parser->type == PVRFILE_PVR_FILE)
		ret = pvr_pvrfile_get_mintime(parser->fpriv);
	else if (parser->type == PVRFILE_SEG_FILE)
		ret = pvr_segfile_get_mintime(parser->fpriv);
	else if (parser->type == PVRFILE_EVT_FILE)
		ret = pvr_evtfile_get_mintime(parser->fpriv);
	else if (parser->type == PVRFILE_LST_FILE)
		ret = pvr_lstfile_get_mintime(parser->fpriv);

	return ret;
}
