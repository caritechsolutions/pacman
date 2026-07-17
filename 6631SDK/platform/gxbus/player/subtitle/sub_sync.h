#ifndef __SUB_SYNC_H__
#define __SUB_SYNC_H__

typedef enum {
	SUB_FREERUN = 0,
	SUB_NORMAL,
	SUB_SKIP,
	SUB_REPEAT,
	SUB_ERROR,
	SUB_SYNC_MAX
}GxSubSyncState;

extern GxSubSyncState sub_pts_sync(GxMedia *media, int64_t pts, int32_t delay);
#endif
