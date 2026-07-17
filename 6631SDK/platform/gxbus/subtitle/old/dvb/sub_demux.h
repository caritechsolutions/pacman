#ifndef __SUB_DEMUX_H__
#define __SUB_DEMUX_H__

#include "gxcore.h"
#include "subtitle/sub_def.h"
#include "sub_debug.h"

#ifdef DEBUG_SUB_DEMUX
#define DBGSubDemux(...)        DBG_SUB(__VA_ARGS__)
#endif

extern GxSubtitle_DemuxOps  gxsub_demux;

#endif
