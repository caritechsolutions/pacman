#include "gx_common.h"
#include "avstring.h"
#include "recfs.h"
#include "pvr_config.h"
#include "pvr_file.h"
#include "pvr_utils.h"

#ifndef CONFIG_FFMPEG_ENABLE
#define av_strstart gx_strstart
int gx_strstart(const char *str, const char *pfx, const char **ptr)
{
    while (*pfx && *pfx == *str) {
        pfx++;
        str++;
    }
    if (!*pfx && ptr)
        *ptr = str;
    return !*pfx;
}
#endif

static int _check_seg_var(PVRSegFile *seg)
{
	if (seg->ts_c >= seg->ts_t) {
		if (seg->ts_t == 0)
			seg->ts_t = 1;
		else if (seg->ts_t < 32)
			seg->ts_t *= 2;
		else if (seg->ts_t >= 32)
			seg->ts_t += 32;

		seg->var = av_realloc(seg->var, sizeof(TSVariant) * seg->ts_t);
		if (!seg->var)
			return -1;
	}

	return 0;
}

static int _check_pvr_var(PVRPvrFile *pvr)
{
	if (pvr->seg_c >= pvr->seg_t) {
		if (pvr->seg_t == 0)
			pvr->seg_t = 1;
		else if (pvr->seg_t < 8)
			pvr->seg_t *= 2;
		else if (pvr->seg_t >= 8)
			pvr->seg_t += 8;

		pvr->var = av_realloc(pvr->var, sizeof(SegVariant) * pvr->seg_t);
		if (!pvr->var)
			return -1;
	}

	return 0;
}

static int _check_evt_var(PVREvtFile *evt)
{
	if (evt->seg_c >= evt->seg_t) {
		if (evt->seg_t == 0)
			evt->seg_t = 1;
		else if (evt->seg_t < 8)
			evt->seg_t *= 2;
		else if (evt->seg_t >= 8)
			evt->seg_t += 8;

		evt->var = av_realloc(evt->var, sizeof(SegVariant) * evt->seg_t);
		if (!evt->var)
			return -1;
	}

	return 0;
}

static int _check_lst_var(PVRLstFile *lst)
{
	if (lst->evt_c >= lst->evt_t) {
		if (lst->evt_t == 0)
			lst->evt_t = 1;
		else if (lst->evt_t < 8)
			lst->evt_t *= 2;
		else if (lst->evt_t >= 8)
			lst->evt_t += 8;

		lst->var = av_realloc(lst->var, sizeof(EvtVariant) * lst->evt_t);
		if (!lst->var)
			return -1;
	}

	return 0;
}

static void _seg_file_parse(PVRSegFile *seg, char *line)
{
	const char *ptr = NULL;

	if (av_strstart((const char*)line, PVR_TS_PREFIX, &ptr)) {
		int bytes = pvr_strlen_line((char *)ptr);

		if (bytes >= PVR_MAX_STS_PREFIX_LEN) {
			gxloge(" [%s] more than %d bytes\n", ptr, PVR_MAX_STS_PREFIX_LEN);
			return;
		}
		if (_check_seg_var(seg) < 0) {
			gxloge(" [%d - %d] realloc fail\n", seg->ts_c, seg->ts_t);
			return;
		}
		memset(&seg->var[seg->ts_c], 0, sizeof(TSVariant));
		memcpy(&seg->var[seg->ts_c].prefix, ptr, bytes);
		seg->ts_c++;
	} else if (av_strstart((const char*)line, PVR_TS_ENCRPYT, &ptr)) {
		if (seg->ts_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", seg->ts_c, line);
			return;
		}
		if (seg->var[seg->ts_c - 1].key_desc)
			av_freep(&seg->var[seg->ts_c - 1].key_desc);
		if (ptr)
			seg->var[seg->ts_c - 1].key_desc = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_TS_DURATION, &ptr)) {
		if (seg->ts_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", seg->ts_c, line);
			return;
		}
		if (ptr)
			seg->var[seg->ts_c - 1].duration = atoi(ptr);
	} else if (av_strstart((const char*)line, PVR_TS_FILESIZE, &ptr)) {
		if (seg->ts_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", seg->ts_c, line);
			return;
		}
		if (ptr)
			seg->var[seg->ts_c - 1].filesize = atoi(ptr);
	} else if (av_strstart((const char*)line, PVR_TS_PATH, &ptr)) {
		if (seg->path)
			av_freep(&seg->path);
		if (ptr)
			seg->path = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_TS_SUFFIX, &ptr)) {
		if (seg->suffix)
			av_freep(&seg->suffix);
		if (ptr)
			seg->suffix = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_TS_APID, &ptr)) {
		if (ptr)
			seg->apid = atoi(ptr);
	} else if (av_strstart((const char*)line, PVR_TS_VPID, &ptr)) {
		if (ptr)
			seg->vpid = atoi(ptr);
	} else if (av_strstart((const char*)line, PVR_TS_ACODEC, &ptr)) {
		if (seg->acodec)
			av_freep(&seg->acodec);
		if (ptr)
			seg->acodec = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_TS_VCODEC, &ptr)) {
		if (seg->vcodec)
			av_freep(&seg->vcodec);
		if (ptr)
			seg->vcodec = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_DESC, &ptr)) {
		if (seg->desc)
			av_freep(&seg->desc);
		if (ptr)
			seg->desc = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_ENC_DESC, &ptr)) {
		if (seg->enc_desc)
			av_freep(&seg->enc_desc);
		if (ptr)
			seg->enc_desc = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_TS_ISFINISH, &ptr)) {
		if (seg)
			seg->is_finish = (char)atoi((char *)ptr);
	}

	return;
}

static void _pvr_file_parse(PVRPvrFile *pvr, char *line)
{
	const char *ptr = NULL;

	if (av_strstart((const char*)line, PVR_SEG_PREFIX, &ptr)) {
		int bytes = pvr_strlen_line((char *)ptr);

		if (bytes >= PVR_MAX_SEG_PREFIX_LEN) {
			gxloge(" [%s] more than %d bytes\n", ptr, PVR_MAX_SEG_PREFIX_LEN);
			return;
		}
		if (_check_pvr_var(pvr) < 0) {
			gxloge(" [%d - %d] realloc fail\n", pvr->seg_c, pvr->seg_t);
			return;
		}
		memset(&pvr->var[pvr->seg_c], 0, sizeof(SegVariant));
		memcpy(&pvr->var[pvr->seg_c].prefix, ptr, bytes);
		pvr->seg_c++;
	} else if (av_strstart((const char*)line, PVR_SEG_DURATION, &ptr)) {
		if (pvr->seg_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", pvr->seg_c, line);
			return;
		}
		if (ptr)
			pvr->var[pvr->seg_c - 1].duration = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_FILESIZE, &ptr)) {
		if (pvr->seg_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", pvr->seg_c, line);
			return;
		}
		if (ptr)
			pvr->var[pvr->seg_c - 1].filesize = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_UNVAILD, &ptr)) {
		if (ptr)
			pvr->var[atoi(ptr)].unvaild = 1;
	} else if (av_strstart((const char*)line, PVR_SEG_PATH, &ptr)) {
		if (pvr->path)
			av_freep(&pvr->path);
		if (ptr)
			pvr->path = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_SUFFIX, &ptr)) {
		if (pvr->suffix)
			av_freep(&pvr->suffix);
		if (ptr)
			pvr->suffix = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_PVR_DESC, &ptr)) {
		if (pvr->desc)
			av_freep(&pvr->desc);
		if (ptr)
			pvr->desc = pvr_strdup_line((char *)ptr);
	}

	return;
}

static void _evt_file_parse(PVREvtFile *evt, char *line)
{
	const char *ptr = NULL;

	if (av_strstart((const char*)line, PVR_SEG_PREFIX, &ptr)) {
		int bytes = pvr_strlen_line((char *)ptr);

		if (bytes >= PVR_MAX_SEG_PREFIX_LEN) {
			gxloge(" [%s] more than %d bytes\n", ptr, PVR_MAX_SEG_PREFIX_LEN);
			return;
		}
		if (_check_evt_var(evt) < 0) {
			gxloge(" [%d - %d] realloc fail\n", evt->seg_c, evt->seg_t);
			return;
		}
		memset(&evt->var[evt->seg_c], 0, sizeof(SegVariant));
		memcpy(&evt->var[evt->seg_c].prefix, ptr, bytes);
		evt->seg_c++;
	} else if (av_strstart((const char*)line, PVR_SEG_XURL, &ptr)) {
		if (_check_evt_var(evt) < 0) {
			gxloge(" [%d - %d] realloc fail\n", evt->seg_c, evt->seg_t);
			return;
		}
		memset(&evt->var[evt->seg_c], 0, sizeof(SegVariant));
		evt->var[evt->seg_c].xurl = pvr_strdup_line((char *)ptr);
		evt->seg_c++;
	} else if (av_strstart((const char*)line, PVR_SEG_DURATION, &ptr)) {
		if (evt->seg_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", evt->seg_c, line);
			return;
		}
		if (ptr)
			evt->var[evt->seg_c - 1].duration = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_FILESIZE, &ptr)) {
		if (evt->seg_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", evt->seg_c, line);
			return;
		}
		if (ptr)
			evt->var[evt->seg_c - 1].filesize = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_UNVAILD, &ptr)) {
		if (ptr)
			evt->var[atoi(ptr)].unvaild = 1;
	} else if (av_strstart((const char*)line, PVR_SEG_PATH, &ptr)) {
		if (evt->path)
			av_freep(&evt->path);
		if (ptr)
			evt->path = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_SEG_SUFFIX, &ptr)) {
		if (evt->suffix)
			av_freep(&evt->suffix);
		if (ptr)
			evt->suffix = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_DESC, &ptr)) {
		if (evt->desc)
			av_freep(&evt->desc);
		if (ptr)
			evt->desc = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_CHAPTER, &ptr)) {
		if (evt->chapter)
			av_freep(&evt->chapter);
		if (ptr)
			evt->chapter = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_CHAPTER_DESC, &ptr)) {
		if (evt->chapter_desc)
			av_freep(&evt->chapter_desc);
		if (ptr)
			evt->chapter_desc = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_START_TIME, &ptr)) {
		if (evt->start_time)
			av_freep(&evt->start_time);
		if (ptr)
			evt->start_time = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_END_TIME, &ptr)) {
		if (evt->end_time)
			av_freep(&evt->end_time);
		if (ptr)
			evt->end_time = pvr_strdup_line((char *)ptr);
	}

	return;
}

static void _lst_file_parse(PVRLstFile *lst, char *line)
{
	const char *ptr = NULL;

	if (av_strstart((const char*)line, PVR_EVT_PREFIX, &ptr)) {
		if (_check_lst_var(lst) < 0) {
			gxloge(" [%d - %d] realloc fail\n", lst->evt_c, lst->evt_t);
			return;
		}
		memset(&lst->var[lst->evt_c], 0, sizeof(EvtVariant));
		lst->var[lst->evt_c].prefix = pvr_strdup_line((char *)ptr);
		lst->evt_c++;
	} else if (av_strstart((const char*)line, PVR_EVT_XURL, &ptr)) {
		if (_check_lst_var(lst) < 0) {
			gxloge(" [%d - %d] realloc fail\n", lst->evt_c, lst->evt_t);
			return;
		}
		memset(&lst->var[lst->evt_c], 0, sizeof(EvtVariant));
		lst->var[lst->evt_c].xurl = pvr_strdup_line((char *)ptr);
		lst->evt_c++;
	} else if (av_strstart((const char*)line, PVR_EVT_DURATION, &ptr)) {
		if (lst->evt_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", lst->evt_c, line);
			return;
		}
		if (ptr)
			lst->var[lst->evt_c - 1].duration = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_FILESIZE, &ptr)) {
		if (lst->evt_c <= 0) {
			gxloge(" [%d - %s] parse fail\n", lst->evt_c, line);
			return;
		}
		if (ptr)
			lst->var[lst->evt_c - 1].filesize = atoll(ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_UNVAILD, &ptr)) {
		if (ptr)
			lst->var[atoi(ptr)].unvaild = 1;
	} else if (av_strstart((const char*)line, PVR_EVT_PATH, &ptr)) {
		if (lst->path)
			av_freep(&lst->path);
		if (ptr)
			lst->path = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_EVT_SUFFIX, &ptr)) {
		if (lst->suffix)
			av_freep(&lst->suffix);
		if (ptr)
			lst->suffix = pvr_strdup_line((char *)ptr);
	} else if (av_strstart((const char*)line, PVR_LST_DESC, &ptr)) {
		if (lst->desc)
			av_freep(&lst->desc);
		if (ptr)
			lst->desc = pvr_strdup_line((char *)ptr);
	}
	return;
}

static void _seg_file_free(PVRSegFile *seg)
{
	unsigned int i = 0;

	if (seg->desc)
		av_freep(&seg->desc);
	if (seg->enc_desc)
		av_freep(&seg->enc_desc);
	if (seg->path)
		av_freep(&seg->path);
	if (seg->suffix)
		av_freep(&seg->suffix);
	if (seg->acodec)
		av_freep(&seg->acodec);
	if (seg->vcodec)
		av_freep(&seg->vcodec);

	for (i = 0; i < seg->ts_c; i++) {
		if (seg->var[i].key_desc)
			av_freep(&seg->var[i].key_desc);
	}
	if (seg->var)
		av_freep(&seg->var);
	seg->apid = -1;
	seg->vpid = -1;
	seg->ts_c = 0;
	seg->ts_t = 0;

	return;
}

static void _pvr_file_free(PVRPvrFile *pvr)
{
	if (pvr->desc)
		av_freep(&pvr->desc);
	if (pvr->path)
		av_freep(&pvr->path);
	if (pvr->suffix)
		av_freep(&pvr->suffix);
	if (pvr->var)
		av_freep(&pvr->var);

	pvr->seg_c = 0;
	pvr->seg_t = 0;

	return;
}

static void _evt_file_free(PVREvtFile *evt)
{
	if (evt->desc)
		av_freep(&evt->desc);
	if (evt->path)
		av_freep(&evt->path);
	if (evt->suffix)
		av_freep(&evt->suffix);
	if (evt->chapter)
		av_freep(&evt->chapter);
	if (evt->chapter_desc)
		av_freep(&evt->chapter_desc);
	if (evt->start_time)
		av_freep(&evt->start_time);
	if (evt->end_time)
		av_freep(&evt->end_time);
	if (evt->var) {
		unsigned int i = 0;
		for (i = 0; i < evt->seg_c; i++) {
			if (evt->var[i].xurl)
				av_free(evt->var[i].xurl);
		}
		av_freep(&evt->var);
	}

	evt->seg_c = 0;
	evt->seg_t = 0;

	return;
}

static void _lst_file_free(PVRLstFile *lst)
{
	if (lst->desc)
		av_freep(&lst->desc);
	if (lst->path)
		av_freep(&lst->path);
	if (lst->suffix)
		av_freep(&lst->suffix);
	if (lst->var) {
		unsigned int i = 0;
		for (i = 0; i < lst->evt_c; i++) {
			if (lst->var[i].xurl)
				av_free(lst->var[i].xurl);
			if (lst->var[i].prefix)
				av_free(lst->var[i].prefix);
		}
		av_freep(&lst->var);
	}

	lst->evt_c = 0;
	lst->evt_t = 0;

	return;
}

void *pvr_file_create(PVRFileType type)
{
	void *priv = NULL;

	switch (type) {
	case PVRFILE_PVR_FILE:
		{
			PVRPvrFile *pvr = av_mallocz(sizeof(PVRPvrFile));

			if (NULL == pvr) {
				gxloge("%s %d\n", __func__, __LINE__);
				return NULL;
			}
			GxCore_MutexCreate(&(pvr->mutex));
			priv = (void *)pvr;
			break;
		}
	case PVRFILE_SEG_FILE:
		{
			PVRSegFile *seg = av_mallocz(sizeof(PVRSegFile));

			if (NULL == seg) {
				gxloge("%s %d\n", __func__, __LINE__);
				return NULL;
			}
			GxCore_MutexCreate(&(seg->mutex));
			priv = (void *)seg;
			break;
		}
	case PVRFILE_EVT_FILE:
		{
			PVREvtFile *evt = av_mallocz(sizeof(PVREvtFile));

			if (NULL == evt) {
				gxloge("%s %d\n", __func__, __LINE__);
				return NULL;
			}
			GxCore_MutexCreate(&(evt->mutex));
			priv = (void *)evt;
			break;
		}
	case PVRFILE_LST_FILE:
		{
			PVRPvrFile *lst = av_mallocz(sizeof(PVRLstFile));

			if (NULL == lst) {
				gxloge("%s %d\n", __func__, __LINE__);
				return NULL;
			}
			GxCore_MutexCreate(&(lst->mutex));
			priv = (void *)lst;
			break;
		}
	default:
		break;
	}

	return priv;
}

void pvr_file_parse(PVRFileType type, void *priv, char *line)
{
	if (priv == NULL) {
		gxloge("%s %d\n", __func__, __LINE__);
		return;
	}

	switch (type) {
	case PVRFILE_PVR_FILE:
		{
			PVRPvrFile *pvr = (PVRPvrFile *)priv;

			GxCore_MutexLock(pvr->mutex);
			_pvr_file_parse(pvr, line);
			GxCore_MutexUnlock(pvr->mutex);
			break;
		}
	case PVRFILE_SEG_FILE:
		{
			PVRSegFile *seg = (PVRSegFile *)priv;

			GxCore_MutexLock(seg->mutex);
			_seg_file_parse(seg, line);
			GxCore_MutexUnlock(seg->mutex);
			break;
		}
	case PVRFILE_EVT_FILE:
		{
			PVREvtFile *evt = (PVREvtFile *)priv;

			GxCore_MutexLock(evt->mutex);
			_evt_file_parse(evt, line);
			GxCore_MutexUnlock(evt->mutex);
			break;
		}
	case PVRFILE_LST_FILE:
		{
			PVRLstFile *lst = (PVRLstFile *)priv;

			GxCore_MutexLock(lst->mutex);
			_lst_file_parse(lst, line);
			GxCore_MutexUnlock(lst->mutex);
			break;
		}
	default:
		break;
	}

	return;
}

void pvr_file_destory(PVRFileType type, void *priv)
{
	if (priv == NULL) {
		gxloge("%s %d\n", __func__, __LINE__);
		return;
	}

	switch (type) {
		case PVRFILE_PVR_FILE:
			{
				PVRPvrFile *pvr = (PVRPvrFile *)priv;

				GxCore_MutexLock(pvr->mutex);
				_pvr_file_free(pvr);
				GxCore_MutexUnlock(pvr->mutex);
				GxCore_MutexDelete(pvr->mutex);
				av_free((void*)pvr);
				break;
			}
		case PVRFILE_SEG_FILE:
			{
				PVRSegFile *seg = (PVRSegFile *)priv;

				GxCore_MutexLock(seg->mutex);
				_seg_file_free(seg);
				GxCore_MutexUnlock(seg->mutex);
				GxCore_MutexDelete(seg->mutex);
				av_free((void*)seg);
				break;
			}
		case PVRFILE_EVT_FILE:
			{
				PVREvtFile *evt = (PVREvtFile *)priv;

				GxCore_MutexLock(evt->mutex);
				_evt_file_free(evt);
				GxCore_MutexUnlock(evt->mutex);
				GxCore_MutexDelete(evt->mutex);
				av_free((void*)evt);
				break;
			}
		case PVRFILE_LST_FILE:
			{
				PVRLstFile *lst = (PVRLstFile *)priv;

				GxCore_MutexLock(lst->mutex);
				_lst_file_free(lst);
				GxCore_MutexUnlock(lst->mutex);
				GxCore_MutexDelete(lst->mutex);
				av_free((void*)lst);
				break;
			}
		default:
			break;
	}

	return;
}

int pvr_pvrfile_get_url(PVRPvrFile *pvr, int c, char *buf, unsigned int max_size)
{
	GxCore_MutexLock(pvr->mutex);
	if (pvr->seg_c <= c) {
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	snprintf(buf, max_size, "%s/%s%s", pvr->path, pvr->var[c].prefix, pvr->suffix);
	GxCore_MutexUnlock(pvr->mutex);

	return 0;
}

char* pvr_pvrfile_get_arg(PVRPvrFile *pvr, PVRFileArgGetCmd cmd)
{
	char *arg = NULL;

	switch (cmd) {
	case PVRFILE_GET_PATH:
		arg = pvr->path;
		break;
	case PVRFILE_GET_DESC:
		arg = pvr->desc;
		break;
	case PVRFILE_GET_SUFFIX:
		arg = pvr->suffix;
		break;
	default:
		break;
	}
	return arg;
}

int64_t pvr_pvrfile_get_duration(PVRPvrFile *pvr, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	int64_t duration = 0;

	GxCore_MutexLock(pvr->mutex);
	if (-1 == seqno) {
		for (i = 0; i < pvr->seg_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!pvr->var[i].unvaild)
					duration += pvr->var[i].duration;
			} else
				duration += pvr->var[i].duration;
		}
	} else {
		if (seqno < pvr->seg_c)
			duration = pvr->var[seqno].duration;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return duration;
}

off_t pvr_pvrfile_get_filesize(PVRPvrFile *pvr, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	off_t filesize = 0;

	GxCore_MutexLock(pvr->mutex);
	if (-1 == seqno) {
		for (i = 0; i < pvr->seg_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!pvr->var[i].unvaild)
					filesize += pvr->var[i].filesize;
			} else
				filesize += pvr->var[i].filesize;
		}
	} else {
		if (seqno < pvr->seg_c)
			filesize = pvr->var[seqno].filesize;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return filesize;
}

uint32_t pvr_pvrfile_get_seqcount(PVRPvrFile *pvr)
{
	return pvr->seg_c;
}

int pvr_pvrfile_add_timems(PVRPvrFile *pvr, int64_t timems)
{
	GxCore_MutexLock(pvr->mutex);
	if (pvr->seg_c == 0) {
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	pvr->var[pvr->seg_c - 1].duration += timems;
	GxCore_MutexUnlock(pvr->mutex);

	return 0;
}

int pvr_pvrfile_add_offset(PVRPvrFile *pvr, off_t offset)
{
	GxCore_MutexLock(pvr->mutex);
	if (pvr->seg_c == 0) {
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	pvr->var[pvr->seg_c - 1].filesize += offset;
	GxCore_MutexUnlock(pvr->mutex);

	return 0;
}

int32_t pvr_pvrfile_get_seqno(PVRPvrFile *pvr, PVRFileSeqGetCmd cmd, void *arg)
{
	int32_t seqno = -1;
	off_t tmpfilsize = 0;
	int64_t tmpduration = 0;
	unsigned int i = 0;

	GxCore_MutexLock(pvr->mutex);
	for (i = 0; i < pvr->seg_c; i++) {
		if (cmd == PVRFILE_GET_SEQ_BYPREFIX) {
			const char *prefix = (const char *)arg;

			if ((0 == pvr->var[i].unvaild)
					&& (0 == strcmp(prefix, pvr->var[i].prefix))) {
				seqno = i;
				break;
			}
		} else if (cmd == PVRFILE_GET_SEQ_BYPOS) {
			off_t pos = *(off_t *)arg;

			if (pos <= (pvr->var[i].filesize + tmpfilsize)) {
				seqno = i;
				break;
			}
			tmpfilsize += pvr->var[i].filesize;
		} else if (cmd == PVRFILE_GET_SEQ_BYTIME) {
			int64_t timems = *(int64_t *)arg;

			if (timems <= (pvr->var[i].duration + tmpduration)) {
				seqno = i;
				break;
			}
			tmpduration += pvr->var[i].duration;
		} else if (cmd == PVRFILE_GET_SEQ_BYSEQ) {
			int32_t minseq = *(int32_t *)arg;

			if ((0 == pvr->var[i].unvaild) && (i >= minseq)) {
				seqno = i;
				break;
			}
		}
	}
	GxCore_MutexUnlock(pvr->mutex);

	return seqno;
}

off_t pvr_pvrfile_get_startpos(PVRPvrFile *pvr, int seqno, PVRFileAttr attr)
{
	off_t startpos = 0;
	unsigned int i = 0;

	GxCore_MutexLock(pvr->mutex);
	if (seqno >= pvr->seg_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!pvr->var[i].unvaild)
				startpos += pvr->var[i].filesize;
		} else
			startpos += pvr->var[i].filesize;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return startpos;
}

int64_t pvr_pvrfile_get_starttime(PVRPvrFile *pvr, int seqno, PVRFileAttr attr)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(pvr->mutex);
	if (seqno >= pvr->seg_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(pvr->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!pvr->var[i].unvaild)
				starttime += pvr->var[i].duration;
		} else
			starttime += pvr->var[i].duration;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return starttime;
}

int64_t pvr_pvrfile_get_mintime(PVRPvrFile *pvr)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(pvr->mutex);
	for (i = 0; i < pvr->seg_c; i++) {
		if (!pvr->var[i].unvaild)
			break;
		starttime += pvr->var[i].duration;
	}
	GxCore_MutexUnlock(pvr->mutex);

	return starttime;
}

int pvr_segfile_get_url(PVRSegFile *seg, int c, char *buf, unsigned int max_size)
{
	GxCore_MutexLock(seg->mutex);
	if (seg->ts_c <= c) {
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	snprintf(buf, max_size, "%s/%s%s", seg->path, seg->var[c].prefix, seg->suffix);
	GxCore_MutexUnlock(seg->mutex);

	return 0;
}

char* pvr_segfile_get_arg(PVRSegFile *seg, PVRFileArgGetCmd cmd)
{
	char *arg = NULL;

	switch (cmd) {
	case PVRFILE_GET_PATH:
		arg = seg->path;
		break;
	case PVRFILE_GET_DESC:
		arg = seg->desc;
		break;
	case PVRFILE_GET_SUFFIX:
		arg = seg->suffix;
		break;
	default:
		break;
	}
	return arg;
}

char *pvr_segfile_get_keydesc(PVRSegFile *seg, int seqno)
{
	char *key_desc = NULL;

	GxCore_MutexLock(seg->mutex);
	if ((seqno >= 0) && (seqno < seg->ts_c))
		key_desc = seg->var[seqno].key_desc;
	GxCore_MutexUnlock(seg->mutex);

	return key_desc;

}

int64_t pvr_segfile_get_duration(PVRSegFile *seg, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	int64_t duration = 0;

	GxCore_MutexLock(seg->mutex);
	if (-1 == seqno) {
		for (i = 0; i < seg->ts_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!seg->var[i].unvaild)
					duration += seg->var[i].duration;
			} else
				duration += seg->var[i].duration;
		}
	} else {
		if (seqno < seg->ts_c)
			duration = seg->var[seqno].duration;
	}
	GxCore_MutexUnlock(seg->mutex);

	return duration;
}

off_t pvr_segfile_get_filesize(PVRSegFile *seg, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	int64_t filesize = 0;
	GxCore_MutexLock(seg->mutex);
	if (-1 == seqno) {
		for (i = 0; i < seg->ts_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!seg->var[i].unvaild)
					filesize += seg->var[i].filesize;
			} else
				filesize += seg->var[i].filesize;
		}
	} else {
		if (seqno < seg->ts_c)
			filesize += seg->var[seqno].filesize;
	}
	GxCore_MutexUnlock(seg->mutex);

	return filesize;
}

uint32_t pvr_segfile_get_seqcount(PVRSegFile *seg)
{
	return seg->ts_c;
}

int pvr_segfile_add_timems(PVRSegFile *seg, int64_t timems)
{
	GxCore_MutexLock(seg->mutex);
	if (seg->ts_c == 0) {
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	seg->var[seg->ts_c - 1].duration += timems;
	GxCore_MutexUnlock(seg->mutex);

	return 0;
}

int pvr_segfile_add_offset(PVRSegFile *seg, off_t offset)
{
	GxCore_MutexLock(seg->mutex);
	if (seg->ts_c == 0) {
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	seg->var[seg->ts_c - 1].filesize += offset;
	GxCore_MutexUnlock(seg->mutex);

	return 0;
}

int32_t pvr_segfile_get_seqno(PVRSegFile *seg, PVRFileSeqGetCmd cmd, void *arg)
{
	int32_t seqno = -1;
	off_t tmpfilsize = 0;
	int64_t tmpduration = 0;
	unsigned int i = 0;

	GxCore_MutexLock(seg->mutex);
	for (i = 0; i < seg->ts_c; i++) {
		if (cmd == PVRFILE_GET_SEQ_BYPREFIX) {
			const char *prefix = (const char *)arg;

			if ((0 == seg->var[i].unvaild)
					&& (0 == strcmp(prefix, seg->var[i].prefix))) {
				seqno = i;
				break;
			}
		} else if (cmd == PVRFILE_GET_SEQ_BYPOS) {
			off_t pos = *(off_t *)arg;

			if (pos <= (seg->var[i].filesize + tmpfilsize)) {
				seqno = i;
				break;
			}
			tmpfilsize += seg->var[i].filesize;
		} else if (cmd == PVRFILE_GET_SEQ_BYTIME) {
			int64_t timems = *(int64_t *)arg;

			if (timems <= (seg->var[i].duration + tmpduration)) {
				seqno = i;
				break;
			}
			tmpduration += seg->var[i].duration;
		} else if (cmd == PVRFILE_GET_SEQ_BYSEQ) {
			int32_t minseq = *(int32_t *)arg;

			if ((0 == seg->var[i].unvaild) && (i >= minseq)) {
				seqno = i;
				break;
			}
		}
	}
	GxCore_MutexUnlock(seg->mutex);

	return seqno;
}

off_t pvr_segfile_get_startpos(PVRSegFile *seg, int seqno, PVRFileAttr attr)
{
	off_t startpos = 0;
	unsigned int i = 0;

	GxCore_MutexLock(seg->mutex);
	if (seqno >= seg->ts_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!seg->var[i].unvaild)
				startpos += seg->var[i].filesize;
		} else
			startpos += seg->var[i].filesize;
	}
	GxCore_MutexUnlock(seg->mutex);

	return startpos;
}

int64_t pvr_segfile_get_starttime(PVRSegFile *seg, int seqno, PVRFileAttr attr)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(seg->mutex);
	if (seqno >= seg->ts_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(seg->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!seg->var[i].unvaild)
				starttime += seg->var[i].duration;
		} else
			starttime += seg->var[i].duration;
	}
	GxCore_MutexUnlock(seg->mutex);

	return starttime;
}

int64_t pvr_segfile_get_mintime(PVRSegFile *seg)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(seg->mutex);
	for (i = 0; i < seg->ts_c; i++) {
		if (!seg->var[i].unvaild)
			break;
		starttime += seg->var[i].duration;
	}
	GxCore_MutexUnlock(seg->mutex);

	return starttime;
}

char pvr_segfile_get_isfinish(PVRSegFile *seg)
{
	return seg->is_finish;
}

int pvr_evtfile_get_url(PVREvtFile *evt, int c, char *buf, unsigned int max_size)
{
	GxCore_MutexLock(evt->mutex);
	if (evt->seg_c <= c) {
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	if (evt->var[c].xurl)
		snprintf(buf, max_size, "%s", evt->var[c].xurl);
	else
		snprintf(buf, max_size, "%s/%s%s", evt->path, evt->var[c].prefix, evt->suffix);
	pvr_get_absolute_url(buf, max_size);
	GxCore_MutexUnlock(evt->mutex);

	return 0;
}

char* pvr_evtfile_get_arg(PVREvtFile *evt, PVRFileArgGetCmd cmd)
{
	char *arg = NULL;

	switch (cmd) {
	case PVRFILE_GET_PATH:
		arg = evt->path;
		break;
	case PVRFILE_GET_DESC:
		arg = evt->desc;
		break;
	case PVRFILE_GET_SUFFIX:
		arg = evt->suffix;
		break;
	case PVRFILE_GET_CHAPTER:
		arg = evt->chapter;
		break;
	case PVRFILE_GET_CHAPTER_DESC:
		arg = evt->chapter_desc;
		break;
	case PVRFILE_GET_START_TIME:
		arg = evt->start_time;
		break;
	case PVRFILE_GET_END_TIME:
		arg = evt->end_time;
		break;
	default:
		break;
	}

	return arg;
}

int64_t pvr_evtfile_get_duration(PVREvtFile *evt, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	int64_t duration = 0;

	GxCore_MutexLock(evt->mutex);
	if (-1 == seqno) {
		for (i = 0; i < evt->seg_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!evt->var[i].unvaild)
					duration += evt->var[i].duration;
			} else
				duration += evt->var[i].duration;
		}
	} else {
		if (seqno < evt->seg_c)
			duration = evt->var[seqno].duration;
	}
	GxCore_MutexUnlock(evt->mutex);

	return duration;
}

off_t pvr_evtfile_get_filesize(PVREvtFile *evt, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	off_t filesize = 0;

	GxCore_MutexLock(evt->mutex);
	if (-1 == seqno) {
		for (i = 0; i < evt->seg_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!evt->var[i].unvaild)
					filesize += evt->var[i].filesize;
			} else
				filesize += evt->var[i].filesize;
		}
	} else {
		if (seqno < evt->seg_c)
			filesize = evt->var[seqno].filesize;
	}
	GxCore_MutexUnlock(evt->mutex);

	return filesize;
}

uint32_t pvr_evtfile_get_seqcount(PVREvtFile *evt)
{
	return evt->seg_c;
}

int pvr_evtfile_add_timems(PVREvtFile *evt, int64_t timems)
{
	GxCore_MutexLock(evt->mutex);
	if (evt->seg_c == 0) {
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	evt->var[evt->seg_c - 1].duration += timems;
	GxCore_MutexUnlock(evt->mutex);

	return 0;
}

int pvr_evtfile_add_offset(PVREvtFile *evt, off_t offset)
{
	GxCore_MutexLock(evt->mutex);
	if (evt->seg_c == 0) {
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	evt->var[evt->seg_c - 1].filesize += offset;
	GxCore_MutexUnlock(evt->mutex);

	return 0;
}

int32_t pvr_evtfile_get_seqno(PVREvtFile *evt, PVRFileSeqGetCmd cmd, void *arg)
{
	int32_t seqno = -1;
	off_t tmpfilsize = 0;
	int64_t tmpduration = 0;
	unsigned int i = 0;

	GxCore_MutexLock(evt->mutex);
	for (i = 0; i < evt->seg_c; i++) {
		if (cmd == PVRFILE_GET_SEQ_BYPREFIX) {
			const char *prefix = (const char *)arg;

			if ((0 == evt->var[i].unvaild)
					&& (0 == strcmp(prefix, evt->var[i].prefix))) {
				seqno = i;
				break;
			}
		} else if (cmd == PVRFILE_GET_SEQ_BYPOS) {
			off_t pos = *(off_t *)arg;

			if (pos <= (evt->var[i].filesize + tmpfilsize)) {
				seqno = i;
				break;
			}
			tmpfilsize += evt->var[i].filesize;
		} else if (cmd == PVRFILE_GET_SEQ_BYTIME) {
			int64_t timems = *(int64_t *)arg;

			if (timems <= (evt->var[i].duration + tmpduration)) {
				seqno = i;
				break;
			}
			tmpduration += evt->var[i].duration;
		} else if (cmd == PVRFILE_GET_SEQ_BYSEQ) {
			int32_t minseq = *(int32_t *)arg;

			if ((0 == evt->var[i].unvaild) && (i >= minseq)) {
				seqno = i;
				break;
			}
		}
	}
	GxCore_MutexUnlock(evt->mutex);

	return seqno;
}

off_t pvr_evtfile_get_startpos(PVREvtFile *evt, int seqno, PVRFileAttr attr)
{
	off_t startpos = 0;
	unsigned int i = 0;

	GxCore_MutexLock(evt->mutex);
	if (seqno >= evt->seg_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!evt->var[i].unvaild)
				startpos += evt->var[i].filesize;
		} else
			startpos += evt->var[i].filesize;
	}
	GxCore_MutexUnlock(evt->mutex);

	return startpos;
}

int64_t pvr_evtfile_get_starttime(PVREvtFile *evt, int seqno, PVRFileAttr attr)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(evt->mutex);
	if (seqno >= evt->seg_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(evt->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!evt->var[i].unvaild)
				starttime += evt->var[i].duration;
		} else
			starttime += evt->var[i].duration;
	}
	GxCore_MutexUnlock(evt->mutex);

	return starttime;
}

int64_t pvr_evtfile_get_mintime(PVREvtFile *evt)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(evt->mutex);
	for (i = 0; i < evt->seg_c; i++) {
		if (!evt->var[i].unvaild)
			break;
		starttime += evt->var[i].duration;
	}
	GxCore_MutexUnlock(evt->mutex);

	return starttime;
}

int pvr_lstfile_get_url(PVRLstFile *lst, int c, char *buf, unsigned int max_size)
{
	GxCore_MutexLock(lst->mutex);
	if (lst->evt_c <= c) {
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	if (lst->var[c].xurl)
		snprintf(buf, max_size, "%s", lst->var[c].xurl);
	else
		snprintf(buf, max_size, "%s/%s%s", lst->path, lst->var[c].prefix, lst->suffix);
	GxCore_MutexUnlock(lst->mutex);

	return 0;
}

char* pvr_lstfile_get_arg(PVRLstFile *lst, PVRFileArgGetCmd cmd)
{
	char *arg = NULL;

	switch (cmd) {
	case PVRFILE_GET_PATH:
		arg = lst->path;
		break;
	case PVRFILE_GET_DESC:
		arg = lst->desc;
		break;
	case PVRFILE_GET_SUFFIX:
		arg = lst->suffix;
		break;
	default:
		break;
	}

	return arg;
}

int64_t pvr_lstfile_get_duration(PVRLstFile *lst, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	int64_t duration = 0;

	GxCore_MutexLock(lst->mutex);
	if (-1 == seqno) {
		for (i = 0; i < lst->evt_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!lst->var[i].unvaild)
					duration += lst->var[i].duration;
			} else
				duration += lst->var[i].duration;
		}
	} else {
		if (seqno < lst->evt_c)
			duration = lst->var[seqno].duration;
	}
	GxCore_MutexUnlock(lst->mutex);

	return duration;
}

off_t pvr_lstfile_get_filesize(PVRLstFile *lst, int seqno, PVRFileAttr attr)
{
	unsigned int i = 0;
	off_t filesize = 0;

	GxCore_MutexLock(lst->mutex);
	if (-1 == seqno) {
		for (i = 0; i < lst->evt_c; i++) {
			if (attr == PVRFILE_EXIST) {
				if (!lst->var[i].unvaild)
					filesize += lst->var[i].filesize;
			} else
				filesize += lst->var[i].filesize;
		}
	} else {
		if (seqno < lst->evt_c)
			filesize = lst->var[seqno].filesize;
	}
	GxCore_MutexUnlock(lst->mutex);

	return filesize;
}

uint32_t pvr_lstfile_get_seqcount(PVRLstFile *lst)
{
	return lst->evt_c;
}

int32_t pvr_lstfile_get_seqno(PVRLstFile *lst, PVRFileSeqGetCmd cmd, void *arg)
{
	int32_t seqno = -1;
	off_t tmpfilsize = 0;
	int64_t tmpduration = 0;
	unsigned int i = 0;

	GxCore_MutexLock(lst->mutex);
	for (i = 0; i < lst->evt_c; i++) {
		if (cmd == PVRFILE_GET_SEQ_BYPREFIX) {
			const char *prefix = (const char *)arg;

			if ((0 == lst->var[i].unvaild)
					&& (0 == strcmp(prefix, lst->var[i].prefix))) {
				seqno = i;
				break;
			}
		} else if (cmd == PVRFILE_GET_SEQ_BYPOS) {
			off_t pos = *(off_t *)arg;

			if (pos <= (lst->var[i].filesize + tmpfilsize)) {
				seqno = i;
				break;
			}
			tmpfilsize += lst->var[i].filesize;
		} else if (cmd == PVRFILE_GET_SEQ_BYTIME) {
			int64_t timems = *(int64_t *)arg;

			if (timems <= (lst->var[i].duration + tmpduration)) {
				seqno = i;
				break;
			}
			tmpduration += lst->var[i].duration;
		} else if (cmd == PVRFILE_GET_SEQ_BYSEQ) {
			int32_t minseq = *(int32_t *)arg;

			if ((0 == lst->var[i].unvaild) && (i >= minseq)) {
				seqno = i;
				break;
			}
		}
	}
	GxCore_MutexUnlock(lst->mutex);

	return seqno;
}

off_t pvr_lstfile_get_startpos(PVRLstFile *lst, int seqno, PVRFileAttr attr)
{
	off_t startpos = 0;
	unsigned int i = 0;

	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!lst->var[i].unvaild)
				startpos += lst->var[i].filesize;
		} else
			startpos += lst->var[i].filesize;
	}
	GxCore_MutexUnlock(lst->mutex);

	return startpos;
}

int64_t pvr_lstfile_get_starttime(PVRLstFile *lst, int seqno, PVRFileAttr attr)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		gxloge("%s %d\n", __func__, __LINE__);
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	for (i = 0; i < seqno; i++) {
		if (attr == PVRFILE_EXIST) {
			if (!lst->var[i].unvaild)
				starttime += lst->var[i].duration;
		} else
			starttime += lst->var[i].duration;
	}
	GxCore_MutexUnlock(lst->mutex);

	return starttime;
}

int64_t pvr_lstfile_get_mintime(PVRLstFile *lst)
{
	off_t starttime = 0;
	unsigned int i = 0;

	GxCore_MutexLock(lst->mutex);
	for (i = 0; i < lst->evt_c; i++) {
		if (!lst->var[i].unvaild)
			break;
		starttime += lst->var[i].duration;
	}
	GxCore_MutexUnlock(lst->mutex);

	return starttime;
}

int pvr_lstfile_set_duration_byseqno(PVRLstFile *lst, int seqno, int64_t duration)
{
	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	lst->var[seqno].duration = duration;
	GxCore_MutexUnlock(lst->mutex);

	return 0;
}

int pvr_lstfile_set_filesize_byseqno(PVRLstFile *lst, int seqno, off_t filesize)
{
	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	lst->var[seqno].filesize = filesize;
	GxCore_MutexUnlock(lst->mutex);

	return 0;
}

char pvr_lstfile_get_isfinish_byseqno(PVRLstFile *lst, int seqno)
{
	char is_finish = 0;

	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		GxCore_MutexUnlock(lst->mutex);
		return 1;
	}
	is_finish = lst->var[seqno].is_finish;
	GxCore_MutexUnlock(lst->mutex);

	return is_finish;
}

int pvr_lstfile_set_isfinish_byseqno(PVRLstFile *lst, int seqno, char is_finish)
{
	GxCore_MutexLock(lst->mutex);
	if (seqno >= lst->evt_c) {
		GxCore_MutexUnlock(lst->mutex);
		return -1;
	}
	lst->var[seqno].is_finish = is_finish;
	GxCore_MutexUnlock(lst->mutex);

	return 0;
}
