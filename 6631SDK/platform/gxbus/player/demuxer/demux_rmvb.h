#ifndef _DEMUX_RMVB_H_
#define _DEMUX_RMVB_H_

#include "gx_demux.h"
#include "stheader.h"


#include "rm_parse.h"
#include "rv_format_info.h"
#include "rv_depack.h"
#include "ra_depack.h"
#include "pack_utils.h"

typedef struct {
	rv_format_info 	rv_format;
	ra_format_info 	ra_format;
	UINT16			rv_stream_id;
	UINT16 		   	ra_stream_id;
	UINT32          ra_pts;
	UINT32          rv_pts;
} DemuxRmvbPriv ;




#endif
