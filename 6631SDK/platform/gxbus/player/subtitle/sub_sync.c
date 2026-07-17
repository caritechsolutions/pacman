#include "gx_media.h"
#include "gx_avout.h"
#include "../gxplayer_internal.h"
#include "sub_sync.h"

#define LOW_TORLANCE        (200)
#define HIGH_TORLANCE       (20000)


GxSubSyncState sub_pts_sync(GxMedia *media, int64_t pts, int32_t delay)
{
	int32_t start = 0, current = 0;
	GxSubSyncState state = SUB_FREERUN;
	const char *syncStr[SUB_SYNC_MAX] = {"FR", "CM", "SK", "RP", "ER"};
	int debug_sub_sync = 0;

	current = GxMedia_GetCurrentPts(media);
	if (current == 0)
		return SUB_ERROR;

	if (pts != -1) {
		if (media->demuxer->type == GX_DEMUXER_TYPE_HW ||
				media->demuxer->type == GX_DEMUXER_TYPE_HW_TS) {
			start  = (int32_t)(pts>>1);
			start /= 45;
		} else if (media->demuxer->type == GX_DEMUXER_TYPE_LAVF) {
			start = (int32_t)(pts - media->demuxer->start_pts);
		} else {
			start = (int32_t)(pts/90.0f);
		}
		start += delay;
	} else
		start = 0;

	if (start == 0) {
		state = SUB_FREERUN;
	} else {
		if (abs(start-current) >= HIGH_TORLANCE) {
			if (media->stream->file_format == GX_STREAMTYPE_DVB)
				state = SUB_FREERUN;
			else
				state = SUB_SKIP;
		} else if (abs(start-current) <= LOW_TORLANCE) {
			state = SUB_NORMAL;
		} else if (((current-start) > LOW_TORLANCE) && ((current-start) < HIGH_TORLANCE)) {
			state = SUB_FREERUN;//why freerun: caption show right now, next caption come fast
		} else if (((start-current) > LOW_TORLANCE) && ((start-current) < HIGH_TORLANCE)) {
			state = SUB_REPEAT;
		}
	}

	GxPlayer_SystemGet(PSYS_DEBUG_SUBTITLE_SYNC, &debug_sub_sync);
	if (debug_sub_sync)
		gxlogi("[subtitle]-[%s] start:%d cur:%d start-current:%d\n", syncStr[state], start, current, start - current);

	return state;
}

