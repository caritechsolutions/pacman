#ifndef __GX_AV_FLOW_H__
#define __GX_AV_FLOW_H__

#ifdef AV_FLOW_DEBUG
#define av_flow_log(...)  gxlogi_raw(__VA_ARGS__)
#else
#define av_flow_log(...)  (void)0
#endif
#endif
