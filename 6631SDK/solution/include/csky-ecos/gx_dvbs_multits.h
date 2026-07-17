
#ifndef __GX_DVBS_MULTITS_H__
#define __GX_DVBS_MULTITS_H__

//#include "gx_common.h"
//#include "gx_mediafilter.h"



extern void* app_mutits_demux_init(char* url);
extern int app_mutits_demux_run(void* handle);
extern int app_mutits_demux_close(void);
extern int app_mutits_demux_track_add(GxStreamTrackAdd* ptrack);
extern int app_mutits_demux_track_del(GxStreamTrackDel* ptrack);
#endif

