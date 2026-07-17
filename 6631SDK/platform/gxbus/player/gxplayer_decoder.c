#include "gxplayer_internal.h"

static struct {
	int        dev;
	handle_t   mod;

	int        thread;
	GxVideoDeocderEventCallback cbk;
} _vdec_event;

static void _vdecoder_event_monitor_thread(void* arg)
{
	while (1) {
		int ret;
		unsigned int event;
		//GxVideoDecoderProperty_FrameInfo frameinfo = {0};

		ret = GxAVWaitEvents(_vdec_event.dev, _vdec_event.mod, EVENT_VIDEO_ONE_FRAME_OVER, 20000, &event);
		if (ret == 0) {
			if (event & EVENT_VIDEO_ONE_FRAME_OVER) {
				//GxAVGetProperty(_vdec_event.dev, _vdec_event.mod, GxVideoDecoderPropertyID_FrameInfo, &frameinfo,sizeof(frameinfo));
				//gxlogi("[vdec-0]: decode one frame!, dec_frame_cnt = %lld\n", frameinfo.stat.decode_frame_cnt);
				_vdec_event.cbk(0, GX_VIDEO_EVENT_DECODE_ONE_FRAME);
			}
		}
	}
}

int GxVideoDecoder_RegisterEventCallback(GxVideoDeocderEventCallback cbk)
{
	_vdec_event.dev = GxAvdev_CreateDevice(0);
	_vdec_event.mod = GxAvdev_OpenModule(_vdec_event.dev, GXAV_MOD_VIDEO_DECODE, 0);
	_vdec_event.cbk = cbk;

	GxCore_ThreadCreate("vdec_event_monitor", &_vdec_event.thread, _vdecoder_event_monitor_thread, NULL, 1024*8, GXOS_DEFAULT_PRIORITY-1);

	return 0;
}
