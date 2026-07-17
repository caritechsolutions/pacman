#include "gxplayer_internal.h"

status_t GxPlayer_SetTimeShiftConfig(PlayerShiftConfig* config)
{
	if (config) {
		av_flow_log("[Flow] - [%s %d]: %d %d %s\n",
				__func__, __LINE__, config->enable, config->shift_dmxid, config->url);
		GxPlayer_SystemSet(PSYS_PVR_SHIFT_ENABLE, &config->enable);
		GxPlayer_SystemSet(PSYS_PVR_SHIFT_FILE, &config->url);
		GxPlayer_SystemSet(PSYS_PVR_SHIFT_DMXID, &config->shift_dmxid);

		return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

status_t GxPlayer_GetTimeShiftConfig(PlayerShiftConfig* config)
{
	if (config) {
		av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
		GxPlayer_SystemGet(PSYS_PVR_SHIFT_ENABLE, &config->enable);
		GxPlayer_SystemGet(PSYS_PVR_SHIFT_FILE, &config->url);
		GxPlayer_SystemGet(PSYS_PVR_SHIFT_DMXID, &config->shift_dmxid);

		return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

status_t GxPlayer_SetPlayPVRConfig(PlayerPlayPVRConfig *config)
{
	if (config) {
		GxPlayer_SystemSet(PSYS_PVR_PLAY_DEMUX_ID, &config->demux_id);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

status_t GxPlayer_SetPVRConfig(PlayerPVRConfig* config)
{
	if (config) {
		av_flow_log("[Flow] - [%s %d]: %d %d %d %d %d\n",
				__func__, __LINE__,
				config->volume_sizemb, config->volume_maxnum,
				config->volume_fullstop, config->reserve_sizemb,
				config->volume_maxtimes);
		GxPlayer_SystemSet(PSYS_PVR_VOLUME_SIZEMB,   &config->volume_sizemb);
		GxPlayer_SystemSet(PSYS_PVR_VOLUME_MAXNUM,   &config->volume_maxnum);
		GxPlayer_SystemSet(PSYS_PVR_VOLUME_FULLSTOP, &config->volume_fullstop);
		GxPlayer_SystemSet(PSYS_PVR_RESERVE_SIZEMB,  &config->reserve_sizemb);
		GxPlayer_SystemSet(PSYS_PVR_VOLUME_MAXTIMES, &config->volume_maxtimes);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

status_t GxPlayer_GetPVRConfig(PlayerPVRConfig* config)
{
	if (config) {
		av_flow_log("[Flow] - [%s %d]\n", __func__, __LINE__);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_SIZEMB,   &config->volume_sizemb);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXNUM,   &config->volume_maxnum);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_FULLSTOP, &config->volume_fullstop);
		GxPlayer_SystemGet(PSYS_PVR_RESERVE_SIZEMB,  &config->reserve_sizemb);
		GxPlayer_SystemGet(PSYS_PVR_VOLUME_MAXTIMES, &config->volume_maxtimes);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

status_t GxPlayer_MediaGetRecordConfigByName(const char* name, PlayerRecordConfig* config)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_get_recconfig(p, NULL, config));
}

status_t GxPlayer_MediaGetRecordConfigByUrl(const char* srcurl, PlayerRecordConfig* config)
{
	int ret = GX_PLAYER_ERROR;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, srcurl);
	if (config) {
		GxMedia* media;
		GxMediaPara MediaPara;

		memset(&MediaPara, 0, sizeof(MediaPara));
		MediaPara.prober.url = srcurl;
		media = GxMedia_New(GX_MEDIA_PROBER, &MediaPara);
		ret = gxplayer_get_recconfig(NULL, media, config);
		GxMedia_Destroy(media, 0);
	}
	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);

	return ret;
}

status_t GxPlayer_MediaRecordConfig(const char* name, PlayerRecordConfig* config)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_record_config(p, config));
}

status_t GxPlayer_MediaRecordEncrypt(const char* name, PlayerRecordEncrypt* encrpyt)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxplayer_record_encrypt(p, encrpyt));
}

status_t GxPlayer_MediaRecord(const char* name, const char* srcurl, const char* file)
{
	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	PLAYER_RUN_FUNC(gxplayer_media_record(p, srcurl, file));
}

status_t GxPlayer_MediaRecord2(const char* name, const char* srcurl, const char* file, PlayerRecordConfig* config)
{
	GxPlayer* p;
	status_t ret = GX_PLAYER_ERROR;
	av_flow_log("[FLOW] - [%s %d]: %s, %s\n", __func__, __LINE__, name, srcurl);
	p = GxPlayer_Create(name);
	if (p != NULL) {
		gxplayer_record_config(p, config);
		ret = gxplayer_media_record(p, srcurl, file);
	}
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	return ret;
}

status_t GxPlayer_MediaStopRecord(const char* name)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxplayer_media_stop_record(p));
}

status_t GxPVRPlayer_NewSegment(const char *name,
		const char *prefix,
		PlayerPvrSegmentInfo *info)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_RUN_FUNC(gxpvrplayer_new_segment(p, (char *)prefix, info));
}

status_t GxPVRPlayer_DeleteSegment(const char *name,
		const char *prefix)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_delete_segment(p, (char *)prefix));
}

status_t GxPVRPlayer_OpenEvent(const char *name,
		const char *prefix,
		PlayerPvrEventInfo *info)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_open_event(p, (char *)prefix, info));
}

status_t GxPVRPlayer_CloseEvent(const char *name,
		const char *prefix)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_close_event(p, (char *)prefix));
}

status_t GxPVRPlayer_DeleteEvent(const char *name,
		const char *prefix)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_delete_event(p, (char *)prefix));
}

status_t GxPVRPlayer_AddSegmentToEvent(const char *name,
		const char *prefix,
		PlayerPvrSegmentList *list)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_add_segment_in_event(p, (char *)prefix, list));
}

status_t GxPVRPlayer_RemoveSegmentInEvent(const char *name,
		const char *prefix,
		PlayerPvrSegmentList *list)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_remove_segment_in_event(p, (char *)prefix, list));
}

status_t GxPVRPlayer_OpenList(const char *name,
		const char *prefix,
		PlayerPvrListInfo *event)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_open_list(p, (char *)prefix, event));
}

status_t GxPVRPlayer_CloseList(const char *name,
		const char *prefix)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_close_list(p, (char *)prefix));
}

status_t GxPVRPlayer_DeleteList(const char *name,
		const char *prefix)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_delete_list(p, (char *)prefix));
}

status_t GxPVRPlayer_AddEventToList(const char *name,
		const char *prefix,
		PlayerPvrEventList *list)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_add_event_in_list(p, (char *)prefix, list));
}

status_t GxPVRPlayer_RemoveEventInList(const char *name,
		const char *prefix,
		PlayerPvrEventList *list)
{
	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, name);
	PLAYER_TRY_FUNC(gxpvrplayer_remove_event_in_list(p, (char *)prefix, list));
}

status_t GxPVR_DeletePVRFile(const char *file)
{
	PvrPvrInfo info;

	if (GxPVR_GetPVRInfo(file, &info) == -1)
		return -1;

	switch (info.type) {
		case PVR_FILE_TYPE_PVR:
			if (info.dir &&
					GxCore_FileExists(info.dir) == GXCORE_FILE_EXIST)
				GxCore_Rmdir(info.dir);
			break;
		case PVR_FILE_TYPE_EVENT:
			{
				PvrPvrInfo pvr_info;

				//list remove event
				if (GxPVR_GetPVRInfo(info.pvr_file, &pvr_info) == 0) {
					if (pvr_info.type == PVR_FILE_TYPE_PVR) {
						unsigned int i = 0;

						for (i = 0; i < pvr_info.file.list_count; i++) {
							const char *list_file = pvr_info.file.list_array[i].file;
							handle_t list_handle = GxPVREventList_Open(list_file);
							PvrEventListEdit list_edit;
							PvrFileAttr event_array;

							event_array.prefix = NULL;
							event_array.file   = file;
							list_edit.list.desc  = NULL;
							list_edit.list.event_count = 1;
							list_edit.list.event_array = &event_array;
							GxPVREventList_Remove(list_handle, &list_edit);
							GxPVREventList_Close(list_handle);
						}

						if (pvr_info.file.event_count == 1) {
							if (info.path &&
									GxCore_FileExists(info.path) == GXCORE_FILE_EXIST)
								GxCore_Rmdir(info.path);
						}
					}
					GxPVR_FreePVRInfo(&pvr_info);
				}
			}
			break;
		case PVR_FILE_TYPE_SEGMENT:
			{
				PvrPvrInfo pvr_info;

				if (GxPVR_GetPVRInfo(info.pvr_file, &pvr_info) == 0) {
					//event remove segment
					if (pvr_info.type == PVR_FILE_TYPE_PVR) {
						unsigned int i = 0;

						for (i = 0; i < pvr_info.file.event_count; i++) {
							const char *event_file = pvr_info.file.event_array[i].file;
							handle_t event_handle = GxPVREventList_Open(event_file);
							PvrEventListEdit event_edit;
							PvrFileAttr segment_array;

							segment_array.prefix = NULL;
							segment_array.file   = file;
							event_edit.event.desc  = NULL;
							event_edit.event.segment_count = 1;
							event_edit.event.segment_array = &segment_array;
							GxPVREventList_Remove(event_handle, &event_edit);
							GxPVREventList_Close(event_handle);
						}
					}

					//pvr remove segment
					if (pvr_info.type == PVR_FILE_TYPE_PVR) {
						PvrEventListEdit pvr_edit;
						PvrFileAttr segment_array;
						handle_t pvr_handle = GxPVREventList_Open(info.pvr_file);

						segment_array.prefix = info.prefix;
						segment_array.file   = NULL;
						pvr_edit.event.desc  = NULL;
						pvr_edit.event.segment_count = 1;
						pvr_edit.event.segment_array = &segment_array;
						GxPVREventList_Remove(pvr_handle, &pvr_edit);
						GxPVREventList_Close(pvr_handle);
					}

					if (info.dir &&
							GxCore_FileExists(info.dir) == GXCORE_FILE_EXIST)
						GxCore_Rmdir(info.dir);

					if (pvr_info.file.segment_count == 1) {
						if (info.path &&
								GxCore_FileExists(info.path) == GXCORE_FILE_EXIST)
							GxCore_Rmdir(info.path);
					}

					GxPVR_FreePVRInfo(&pvr_info);
				}
			}
			break;
		case PVR_FILE_TYPE_LIST:
		default:
			break;
	}
	GxPVR_FreePVRInfo(&info);

	if (GxCore_FileExists(file) == GXCORE_FILE_EXIST)
		GxCore_FileDelete(file);

	return 0;
}

status_t GxPVR_ClearSegmentFileInEvent(const char *pvr_file)
{
	PvrPvrInfo info;
	int i = 0;
	int   segment_count = 0;
	char *segment_flag  = NULL;

	if (GxPVR_GetPVRInfo(pvr_file, &info) == -1)
		return -1;

	if (info.type != PVR_FILE_TYPE_PVR) {
		GxPVR_FreePVRInfo(&info);
		return -1;
	}

	segment_count = info.file.segment_count;
	segment_flag  = av_mallocz(segment_count * sizeof(char));

	for (i = 0; i < info.file.event_count; i++) {
		int j = 0, k = 0;
		PvrPvrInfo evt_info;

		if (GxPVR_GetPVRInfo(info.file.event_array[i].file, &evt_info) == -1)
			continue;
		for (j = 0; j < evt_info.event.segment_count; j++) {
			const char *prefix = evt_info.event.segment_array[j].prefix;

			if (prefix == NULL)
				continue;
			for (k = 0; k < info.file.segment_count; k++) {
				if (info.file.segment_array[k].prefix == NULL)
					continue;
				if (strcmp(prefix, info.file.segment_array[k].prefix) == 0) {
					segment_flag[k] = 1;
					break;
				}
			}
		}
		GxPVR_FreePVRInfo(&evt_info);
	}

	for (i = 0; i < segment_count; i++) {
		if (segment_flag[i] == 0)
			GxPVR_DeletePVRFile(info.file.segment_array[i].file);
	}
	GxPVR_FreePVRInfo(&info);
	av_free(segment_flag);


	//check event/segment is null
	if (GxPVR_GetPVRInfo(pvr_file, &info) == -1)
		return -1;

	if ((info.file.event_count == 0) ||
			(info.file.segment_count == 0))
		GxPVR_DeletePVRFile(pvr_file);
	GxPVR_FreePVRInfo(&info);

	return 0;
}

status_t GxPVR_GetPVRInfo(const char *file, PvrPvrInfo *info)
{
#define GXPVR_FILE_COPY(c0, c1, s0, s1) do {                          \
	if (c0 > 0) {                                                     \
		unsigned int i = 0;                                           \
		PvrFileAttr *_array = av_malloc(c0 * sizeof(PvrFileAttr));    \
		GxRecordPVRFileElem *_elem = s0;                              \
		if (_array != NULL) {                                         \
			for (i = 0; i < c0; i++) {                                \
				if (_elem[i].prefix)                                  \
					_array[i].prefix = av_strdup(_elem[i].prefix);    \
				if (_elem[i].file)                                    \
					_array[i].file = av_strdup(_elem[i].file);        \
			}                                                         \
			c1 = c0;                                                  \
			s1 = _array;                                              \
		} else {                                                      \
			c1 = 0;                                                   \
			s1 = NULL;                                                \
		}                                                             \
	} else {                                                          \
		c1 = 0;                                                       \
		s1 = NULL;                                                    \
	}                                                                 \
} while(0)
	GxStream *stream = NULL;
	GxStreamParam param;
	GxRecordPVRControl pvr_ctrl;
	GxRecordPVRInfo    pvr_info;

	if ((NULL == info) || (NULL == file)) {
		gxloge("[%s] param error\n", file);
		return -1;
	}

	if (GxCore_FileExists(file) != GXCORE_FILE_EXIST) {
		gxloge("[%s] file not exist\n", file);
		return -1;
	}

	param.mode     = GX_STREAM_READ;
	param.prog     = 0;
	param.flags    = 0;
	param.priv     = NULL;
	param.options  = NULL;
	param.protocol = NULL;
	stream = GxStream_OpenByParam(file, &param);
	if (NULL == stream) {
		gxloge("[%s] stream open fail\n", file);
		return -1;
	}

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	memset(info, 0, sizeof(PvrPvrInfo));
	memset(&pvr_info, 0, sizeof(GxRecordPVRInfo));
	pvr_info.type = -1;
	pvr_ctrl.opt = GX_RECORD_PVR_GET_PVR_INFO;
	pvr_ctrl.arg = &pvr_info;
	GxStream_Control(stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&pvr_ctrl);
	switch (pvr_info.type) {
	case GX_RECORD_PVR_PVR_FILE:
		{
			GxRecordPVRFile *file0 = &pvr_info.file;
			PvrPvrFile      *file1 = &info->file;

			info->type = PVR_FILE_TYPE_PVR;
			GXPVR_FILE_COPY(file0->list_count,    file1->list_count,    file0->list_elem,    file1->list_array);
			GXPVR_FILE_COPY(file0->event_count,   file1->event_count,   file0->event_elem,   file1->event_array);
			GXPVR_FILE_COPY(file0->segment_count, file1->segment_count, file0->segment_elem, file1->segment_array);
		}
		break;
	case GX_RECORD_PVR_LST_FILE:
		{
			GxRecordPVRList *lst0 = &pvr_info.list;
			PvrPvrList      *lst1 = &info->list;

			info->type = PVR_FILE_TYPE_LIST;
			if (lst0->desc)
				lst1->desc = av_strdup(lst0->desc);
			GXPVR_FILE_COPY(lst0->event_count, lst1->event_count, lst0->event_elem, lst1->event_array);
		}
		break;
	case GX_RECORD_PVR_EVT_FILE:
		{
			GxRecordPVREvent *evt0 = &pvr_info.event;
			PvrPvrEvent      *evt1 = &info->event;

			info->type = PVR_FILE_TYPE_EVENT;
			if (evt0->desc)
				evt1->desc = av_strdup(evt0->desc);
			if (evt0->chapter)
				evt1->chapter = av_strdup(evt0->chapter);
			if (evt0->chapter_desc)
				evt1->chapter_desc = av_strdup(evt0->chapter_desc);
			if (evt0->start_time)
				evt1->start_time = av_strdup(evt0->start_time);
			if (evt0->end_time)
				evt1->end_time = av_strdup(evt0->end_time);
			evt1->filesize = evt0->filesize;
			evt1->duration = evt0->duration;
			GXPVR_FILE_COPY(evt0->segment_count, evt1->segment_count, evt0->segment_elem, evt1->segment_array);
		}
		break;
	case GX_RECORD_PVR_SEG_FILE:
		{
			GxRecordPVRSegment *seg0 = &pvr_info.segment;
			PvrPvrSegment      *seg1 = &info->segment;

			info->type = PVR_FILE_TYPE_SEGMENT;
			if (seg0->desc)
				seg1->desc = av_strdup(seg0->desc);
			seg1->filesize = seg0->filesize;
			seg1->duration = seg0->duration;
		}
		break;
	default:
		break;
	}
	if (pvr_info.pvr_file)
		info->pvr_file = av_strdup(pvr_info.pvr_file);
	if (pvr_info.dir)
		info->dir = av_strdup(pvr_info.dir);
	if (pvr_info.path)
		info->path = av_strdup(pvr_info.path);
	if (pvr_info.prefix)
		info->prefix = av_strdup(pvr_info.prefix);

	pvr_ctrl.opt = GX_RECORD_PVR_FREE_PVR_INFO;
	pvr_ctrl.arg = &pvr_info;
	GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
	GxStream_Close(stream);
	return 0;
}

status_t GxPVR_FreePVRInfo(PvrPvrInfo *info)
{
#define GXPVR_FILE_FREE(c, s) do {             \
	if (c > 0) {                               \
		unsigned int i = 0;                    \
		for (i = 0; i < c; i++) {              \
			if (s[i].prefix) {                 \
				av_free((void *)s[i].prefix);  \
				s[i].prefix = NULL;            \
			}                                  \
			if (s[i].file) {                   \
				av_free((void *)s[i].file);    \
				s[i].file = NULL;              \
			}                                  \
		}                                      \
		av_free(s);                            \
		c = 0;                                 \
		s = 0;                                 \
	}                                          \
} while(0)

#define GXPVR_PTR_FREE(p) do {                 \
	if (p) {                                   \
		av_free((void *)p);                    \
		p = NULL;                              \
	}                                          \
} while(0)

	if (NULL == info) {
		gxloge("param error\n");
		return -1;
	}

	GXPVR_PTR_FREE(info->pvr_file);
	GXPVR_PTR_FREE(info->dir);
	GXPVR_PTR_FREE(info->prefix);
	GXPVR_PTR_FREE(info->path);
	switch (info->type) {
	case PVR_FILE_TYPE_PVR:
		{
			PvrPvrFile *file = &info->file;

			GXPVR_FILE_FREE(file->list_count,    file->list_array);
			GXPVR_FILE_FREE(file->event_count,   file->event_array);
			GXPVR_FILE_FREE(file->segment_count, file->segment_array);
		}
		break;
	case PVR_FILE_TYPE_LIST:
		{
			PvrPvrList *list = &info->list;

			GXPVR_PTR_FREE(list->desc);
			GXPVR_FILE_FREE(list->event_count, list->event_array);
		}
		break;
	case PVR_FILE_TYPE_EVENT:
		{
			PvrPvrEvent *event = &info->event;

			GXPVR_PTR_FREE(event->desc);
			GXPVR_PTR_FREE(event->chapter);
			GXPVR_PTR_FREE(event->chapter_desc);
			GXPVR_PTR_FREE(event->start_time);
			GXPVR_PTR_FREE(event->end_time);
			GXPVR_FILE_FREE(event->segment_count, event->segment_array);
		}
		break;
	case PVR_FILE_TYPE_SEGMENT:
		{
			PvrPvrSegment *segment = &info->segment;

			GXPVR_PTR_FREE(segment->desc);
		}
		break;
	default:
		return -1;
	}

	return 0;
}

handle_t GxPVREventList_Open(const char *url)
{
	GxStream *stream = NULL;
	GxStreamParam param;

	av_flow_log("[FLOW] - [%s %d]: %s\n", __func__, __LINE__, url);
	param.mode     = GX_STREAM_TRUNC;
	param.prog     = 0;
	param.flags    = 0;
	param.priv     = NULL;
	param.options  = NULL;
	param.protocol = NULL;
	stream = GxStream_OpenByParam(url, &param);
	if (NULL == stream) {
		gxloge("[%s] stream open fail\n", url);
		return 0;
	}

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	return (handle_t)stream;
}

status_t GxPVREventList_Add(handle_t handle, PvrEventListEdit *edit)
{
	int ret = -1;
	GxStream *stream = (GxStream *)handle;
	GxRecordPVRControl  pvr_ctrl;
	GxRecordPVRFileType file_type = GX_RECORD_PVR_PVR_FILE;

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	if (NULL == stream) {
		gxloge("stream is NULL\n");
		return ret;
	}
	pvr_ctrl.opt = GX_RECORD_PVR_GET_PVR_TYPE;
	pvr_ctrl.arg = (void *)&file_type;
	GxStream_Control(stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&pvr_ctrl);
	switch (file_type) {
	case GX_RECORD_PVR_EVT_FILE:
		{
			unsigned int i = 0;
			GxRecordPVREvent evt;

			evt.desc         = edit->event.desc;
			evt.chapter      = edit->event.chapter;
			evt.chapter_desc = edit->event.chapter_desc;
			evt.start_time   = edit->event.start_time;
			evt.end_time     = edit->event.end_time;
			evt.filesize     = edit->event.filesize;
			evt.duration     = edit->event.duration;

			evt.segment_count = edit->event.segment_count;
			evt.segment_elem  = av_mallocz(evt.segment_count * sizeof(GxRecordPVRFileElem));
			for (i = 0; i < evt.segment_count; i++) {
				evt.segment_elem[i].prefix = (char *)edit->event.segment_array[i].prefix;
				evt.segment_elem[i].file   = (char *)edit->event.segment_array[i].file;
			}
			pvr_ctrl.opt = GX_RECORD_PVR_ADD_EVENT_LIST;
			pvr_ctrl.arg = (void *)&evt;
			ret = GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
			av_free(evt.segment_elem);
		}
		break;
	case GX_RECORD_PVR_LST_FILE:
		{
			unsigned int i = 0;
			GxRecordPVRList lst;

			lst.desc        = edit->list.desc;
			lst.event_count = edit->list.event_count;
			lst.event_elem  = av_mallocz(lst.event_count * sizeof(GxRecordPVRFileElem));
			for (i = 0; i < lst.event_count; i++) {
				lst.event_elem[i].prefix = (char *)edit->list.event_array[i].prefix;
				lst.event_elem[i].file   = (char *)edit->list.event_array[i].file;
			}
			pvr_ctrl.opt = GX_RECORD_PVR_ADD_EVENT_LIST;
			pvr_ctrl.arg = (void *)&lst;
			ret = GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
			av_free(lst.event_elem);
		}
		break;
	default:
		return -1;
	}

	av_flow_log("[FLOW] - [%s %d] ret = %d\n", __func__, __LINE__, ret);
	return ret;
}

status_t GxPVREventList_Remove(handle_t handle, PvrEventListEdit *edit)
{
	int ret = -1;
	GxStream *stream = (GxStream *)handle;
	GxRecordPVRControl  pvr_ctrl;
	GxRecordPVRFileType file_type = GX_RECORD_PVR_PVR_FILE;

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	if (NULL == stream) {
		gxloge("stream is NULL\n");
		return ret;
	}
	pvr_ctrl.opt = GX_RECORD_PVR_GET_PVR_TYPE;
	pvr_ctrl.arg = (void *)&file_type;
	GxStream_Control(stream, GX_STREAM_CTRL_PVR_GET_CONTROL, (void *)&pvr_ctrl);
	switch (file_type) {
	case GX_RECORD_PVR_EVT_FILE:
		{
			unsigned int i = 0;
			GxRecordPVREvent evt;

			evt.segment_count = edit->event.segment_count;
			evt.segment_elem  = av_mallocz(evt.segment_count * sizeof(GxRecordPVRFileElem));
			for (i = 0; i < evt.segment_count; i++) {
				evt.segment_elem[i].prefix = (char *)edit->event.segment_array[i].prefix;
				evt.segment_elem[i].file   = (char *)edit->event.segment_array[i].file;
			}
			pvr_ctrl.opt = GX_RECORD_PVR_REMOVE_EVENT_LIST;
			pvr_ctrl.arg = (void *)&evt;
			ret = GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
			av_free(evt.segment_elem);
		}
		break;
	case GX_RECORD_PVR_LST_FILE:
		{
			unsigned int i = 0;
			GxRecordPVRList lst;

			lst.event_count = edit->list.event_count;
			lst.event_elem  = av_mallocz(lst.event_count * sizeof(GxRecordPVRFileElem));
			for (i = 0; i < lst.event_count; i++) {
				lst.event_elem[i].prefix = (char *)edit->list.event_array[i].prefix;
				lst.event_elem[i].file   = (char *)edit->list.event_array[i].file;
			}
			pvr_ctrl.opt = GX_RECORD_PVR_REMOVE_EVENT_LIST;
			pvr_ctrl.arg = (void *)&lst;
			ret = GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
			av_free(lst.event_elem);
		}
		break;
	case GX_RECORD_PVR_PVR_FILE:
		{
			pvr_ctrl.opt = GX_RECORD_PVR_DELETE_SEGMENT;
			pvr_ctrl.arg = (void *)edit->event.segment_array[0].prefix;
			ret = GxStream_Control(stream, GX_STREAM_CTRL_PVR_SET_CONTROL, (void *)&pvr_ctrl);
		}
		break;
	default:
		return -1;
	}
	av_flow_log("[FLOW] - [%s %d](ret = %d)\n", __func__, __LINE__, ret);
	return ret;
}

status_t GxPVREventList_Close(handle_t handle)
{
	unsigned int i = 0;
	GxStream *stream = (GxStream *)handle;

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	if (NULL == stream) {
		gxloge("stream is NULL\n");
		return -1;
	}

	GxStream_Close(stream);

	av_flow_log("[FLOW] - [%s %d]\n", __func__, __LINE__);
	return 0;
}
