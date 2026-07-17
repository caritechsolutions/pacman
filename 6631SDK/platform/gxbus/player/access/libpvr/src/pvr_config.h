#ifndef __PVR_CONFIG_H__
#define __PVR_CONFIG_H__

#include "gxplayer_define.h"

#define PVR_MAX_EVT_COUNT      (GXPLAYER_PVR_EVENT_COUNT)
#define PVR_MAX_LST_COUNT      (GXPLAYER_PVR_LIST_COUNT)
#define PVR_MAX_SEGTS_TIMEMS   (30000)
#define PVR_MAX_FSYNC_TIMEMS   (1000)
#define PVR_MAX_SEG_PREFIX_LEN (16)
#define PVR_MAX_STS_PREFIX_LEN (8)

#define PVR_SEGFILE_SUFFIX   ".seg"
#define PVR_EVTFILE_SUFFIX   ".evt"
#define PVR_LSTFILE_SUFFIX   ".lst"
#define PVR_PVRFILE_SUFFIX   ".pvr"
#define PVR_STSFILE_SUFFIX  ".ts"

#define PVR_PVRFILE_DESC    "tms"
#define PVR_PVRFILE_SEGPATH "seg"
#define PVR_PVRFILE_EVTPATH "evt"

#define PVR_EVT_TO_HDR_PATH ".."
#define PVR_SEG_TO_HDR_PATH ".."
#define PVR_HDRFILE         "pi.hdr"

#define PVR_SEGFILE_DEF_PREFIX "gds"
#define PVR_SEGFILE_DEF_DESC   "PVR Default Segment"

#endif
