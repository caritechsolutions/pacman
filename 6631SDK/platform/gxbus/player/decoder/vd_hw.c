#include "gx_decoder.h"
#include "gx_config.h"
#include "gx_system.h"
#include "gx_avtick.h"

extern GxAVCodecClass gx_VideoDecoderBase;
static int vd_hw_reset(GxMediaFilter *filter);

#define MAX_NODES (2)
typedef struct runinfo_pool {
	int current;
	struct runinfo_node {
		int busy_flag;
		GxVideoDecoderProperty_Run runinfo;
	} nodes[MAX_NODES];
} RuninfoPool;

typedef struct {
	int      dev;
	int      vdec_timeout;
	handle_t mod;
	handle_t vpu;
	GxFifo*  fifo_in;
	int mosaic_drop;
	int mosaic_gate;
	int sync_flag;
	int running;
	int headerok;
	int freezed;
	int ppzoom_enable;
	int max_fb_size;
	int now_fb_size;
	struct {
		int enable;
		int max_width;
		int max_height;
	}deinterlace;

	struct {
		struct {
			int enable;
			int display;
		} cc;
		struct {
			int enable;
		} hdr;
		int length;
	} userdata;

	int speed_mode;
	int decoded_first_frame;
	int displayed_first_frame;
	int decode_finish_tick;
	GxAvChipId chip;
	GxVideoDecoderProperty_Config config;
	RuninfoPool runinfo_pool;
	//GxVideoDecoderProperty_Run runinfo;
	GxVideoDecoderProperty_Capability capability;
	handle_t mutex;
}VdHwPriv;

#define SIZE_ALIGN(val, align) ((val+align-1)&~(align-1))

#define FOLAT2INT(fps) ((fps-(int)fps >=0.5)?(fps+1):fps)

#define wkb_alloc(fb, siz) \
	do { \
		fb.size = siz;\
		fb.addr = (unsigned int)av_memhole_malloc(siz, GX_PINFLAG_WKV);\
	}while (0);

#define wkb_free(fb) \
	do { \
		if (fb.addr) {\
			av_memhole_free((void*)(fb.addr), GX_PINFLAG_WKV);\
			fb.addr = 0;\
			fb.size = 0;\
		} \
	}while (0);

#define fb_alloc(priv, fb, siz) \
	do { \
		if (priv->now_fb_size + siz <= priv->max_fb_size) { \
			fb.size = siz; \
			fb.addr = (unsigned int)av_memhole_malloc(siz, GX_PINFLAG_FBV); \
			if (fb.addr != 0) { \
				priv->now_fb_size += siz; \
			} \
		} \
	}while (0);

#define fb_free(priv, fb) \
	do { \
		av_memhole_free((void*)(fb.addr), GX_PINFLAG_FBV);\
		priv->now_fb_size -= fb.size; \
		fb.addr = 0;\
		fb.size = 0;\
	}while (0);

#define col_alloc(priv, fb, siz) \
	do { \
		if (priv->now_fb_size + siz <= priv->max_fb_size) { \
			fb.size = siz; \
			fb.addr = (unsigned int)av_memhole_malloc(siz, GX_PINFLAG_FCOL); \
			if (fb.addr != 0) { \
				priv->now_fb_size += siz; \
			} \
		} \
	}while (0);

#define col_free(priv, fb) \
	do { \
		av_memhole_free((void*)(fb.addr), GX_PINFLAG_FCOL);\
		priv->now_fb_size -= fb.size; \
		fb.addr = 0;\
		fb.size = 0;\
	}while (0);

static void release_run_resource(VdHwPriv *priv, struct runinfo_node *node)
{
	int i;
	GxVideoDecoderProperty_Run *runinfo = &node->runinfo;

	col_free(priv, runinfo->colbuf);
	for (i = 0; i < runinfo->dec_framebuf.fb_num; i++) {
		if (i != FREEZE_FB_ID) {
			fb_free(priv, runinfo->dec_framebuf.fb[i]);
		} else {
			priv->now_fb_size -= runinfo->dec_framebuf.fb[i].size;
		}
	}
	for (i = 0; i < runinfo->pp_framebuf.fb_num; i++) {
		fb_free(priv, runinfo->pp_framebuf.fb[i]);
	}
	runinfo->pp_framebuf.fb_num = 0;
	runinfo->dec_framebuf.fb_num = 0;
	node->busy_flag = 0;
	av_memhole_flush();
}

static GxVideoDecoderProperty_Run* alloc_runinfo(VdHwPriv *priv)
{
	int id = (priv->runinfo_pool.current + 1) % MAX_NODES;

	if (priv->runinfo_pool.nodes[id].busy_flag == 1)
		release_run_resource(priv, &priv->runinfo_pool.nodes[id]);
	priv->runinfo_pool.current = id;
	priv->runinfo_pool.nodes[id].busy_flag = 1;

	return &priv->runinfo_pool.nodes[id].runinfo;
}

static struct runinfo_node *current_runinfo(VdHwPriv *priv)
{
	int id = priv->runinfo_pool.current;
	return &priv->runinfo_pool.nodes[id];
}

static void set_speed_mod(VdHwPriv *priv,int mode,int factor)
{
	GxVideoDecoderProperty_Speed Speed;

	Speed.mode   = mode;
	Speed.factor = factor;

	GxAVSetProperty(priv->dev, priv->mod, \
			GxVideoDecoderPropertyID_Speed, &Speed, sizeof(GxVideoDecoderProperty_Speed));

	priv->speed_mode = mode;
}

static void set_dec_mod(VdHwPriv *priv,int mode)
{
	GxVideoDecoderProperty_DecMode DecMode;

	DecMode.dec_mode = mode;

	GxAVSetProperty(priv->dev, priv->mod, \
			GxVideoDecoderPropertyID_DecMode, &DecMode, sizeof(GxVideoDecoderProperty_DecMode));
}

static void set_sync_mod(VdHwPriv *priv, int mode)
{
	GxVideoDecoderProperty_SyncMode sync_mode;
	sync_mode = mode;
	GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_SyncMode, &sync_mode, sizeof(GxVideoDecoderProperty_SyncMode));
}

static float rate2fps(GxVideoDecoderProperty_FrameRate FrameRate)
{
	switch(FrameRate)
	{
	case FRAMERATE_23976: return 23.976;
	case FRAMERATE_24: return 24;
	case FRAMERATE_25: return 25.0;
	case FRAMERATE_50: return 50.0;
	case FRAMERATE_29397: return 29.397;
	case FRAMERATE_299: return 29.9;
	case FRAMERATE_301:
	case FRAMERATE_30:
						return 30.0;
	case FRAMERATE_5994: return 59.94;
	case FRAMERATE_60: return 60.0;
	default: return FrameRate/1000.0;
	}
}

static void update_dr_info(GxStreamVideoHeader* sh, GxVideoColorInfo *color)
{
	if (sh && color) {
		if (color->colorimetry == GXCOL_PRI_BT2020 && color->transfer_characteristic == GXCOL_TRC_ARIB_STD_B67)
			sh->drt = VIDEO_DRT_HLG;
		else if (color->colorimetry == GXCOL_PRI_BT2020 && color->transfer_characteristic == GXCOL_TRC_SMPTE2084) {
			//if (color->mastering_display_metadata_flag)
			sh->drt = VIDEO_DRT_HDR10;
			if (color->dynamic_metadata_flag && color->dolby_flag == 0)
				sh->drt = VIDEO_DRT_HDR10_PLUS;
			if (color->dynamic_metadata_flag && color->dolby_flag == 1)
				sh->drt = VIDEO_DRT_DOLBYVISION;
		} else {
			sh->drt = VIDEO_DRT_SDR;
		}

		if (color->dolby_flag)
			sh->dolby_flag = color->dolby_flag;
	}
}

static int vd_hw_run(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		if (priv->running == 1){
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Probe, NULL,0);
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_wait(GxMediaFilter *filter)
{
	int ret;
	uint32_t event;
	GxAVCodec *vd = GXDECODEROBJECT(filter);
	GxVpuProperty_LayerEnable layer_en;

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		GxPlayer_SystemGet(PSYS_VDEC_TIMEOUT, &priv->vdec_timeout);
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		ret = GxAVWaitEvents(priv->dev, priv->mod, EVENT_VIDEO_0_DECOVER, priv->vdec_timeout, &event);
		if (event != EVENT_VIDEO_0_DECOVER) {
			layer_en.enable = 0 ;
			layer_en.layer = GX_LAYER_VPP;
			GxAVSetProperty(priv->dev, priv->vpu, GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));
			layer_en.enable = 1 ;
			GxAVSetProperty(priv->dev, priv->vpu, GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));
		}
		gxlogf("v_event = 0x%x\n",event);
		return ret;
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_abort(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv) {
		GxVideoDecoderProperty_State state;
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_State, &state,sizeof(GxVideoDecoderProperty_State));

		if (state.state == VIDEODEC_STOPED || state.state == VIDEODEC_OVER || state.state == VIDEODEC_READY) {
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			GxAVSetEvent(priv->dev, priv->mod, EVENT_VIDEO_0_DECOVER);
		}

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int  vd_hw_pause(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		GxCore_MutexLock(priv->mutex);
		if (priv->speed_mode != STEP_SPEED) {
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Pause, NULL,0);
		}
		GxCore_MutexUnlock(priv->mutex);

		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int vd_hw_resume(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		GxCore_MutexLock(priv->mutex);
		if (priv->speed_mode != STEP_SPEED) {
			GxVideoDecoderProperty_State state ;
			GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_State, &state,sizeof(GxVideoDecoderProperty_State));
			if ((state.state == VIDEODEC_PAUSED) || (state.state == VIDEODEC_READY)) {
				GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Resume, NULL, 0);
			}
		}
		GxCore_MutexUnlock(priv->mutex);

		return GX_PLAYER_OK;
	}
	return GX_PLAYER_ERROR;
}

static int vd_hw_stop(GxMediaFilter *filter)
{
	int i;
	GxAVCodec *vd = GXDECODEROBJECT(filter);
	GxVideoDecoderProperty_Stop Stop;
	GxStreamVideoHeader* header = (GxStreamVideoHeader*)vd->header;

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;
		if (priv->running) {
			GxFifoLink FifoLink;
			Stop.freeze = priv->freezed;
			gxlogi("[Player]: %s in.\n", __func__);
			GxAVSetProperty(priv->dev, priv->mod,GxVideoDecoderPropertyID_Stop, &Stop, sizeof(Stop));
			gxlogi("[Player]: %s out.\n", __func__);

			GxAVResetEvent(priv->dev, priv->mod, EVENT_VIDEO_0_DECOVER);
			GxAVResetEvent(priv->dev, priv->mod, EVENT_VIDEO_0_FRAMEINFO);
			GxAVResetEvent(priv->dev, priv->mod, EVENT_VIDEO_0_ONE_FRAME_OVER);
			GxAVResetEvent(priv->dev, priv->mod, EVENT_VIDEO_0_SEQ_CHANGED);

			FifoLink.module = priv->mod;
			GxFifo_Unlink(priv->fifo_in, &FifoLink);

			wkb_free(priv->config.workbuf);
			for (i = 0; i < MAX_NODES; i++)
				release_run_resource(priv, &priv->runinfo_pool.nodes[i]);

			header->disp_w = header->disp_h = 0;
			memset(&header->stat, 0, sizeof(header->stat));
			priv->running = 0;
			priv->headerok = 0;
			priv->decoded_first_frame   = 0;
			priv->displayed_first_frame = 0;
			priv->decode_finish_tick = 0;
			priv->now_fb_size = 0;
			memset(&priv->config, 0, sizeof(priv->config));
			memset(&priv->runinfo_pool, 0, sizeof(priv->runinfo_pool));
			memset(&priv->capability, 0, sizeof(priv->capability));
			gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
			return GX_PLAYER_OK;
		}
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_config(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv && vd->header) {
		int ret, codec_mask = 0, fps_source = 0, speed_mode = NORMAL_SPEED;
		GxPin* vd_in_pin;
		GxFifoLink FifoLink;
		GxStreamVideoHeader* header = (GxStreamVideoHeader*)vd->header;
		VdHwPriv* priv = (VdHwPriv*)vd->priv;
		GxVideoDecoderProperty_Tolerance videodec_tolerance;

		memset(&priv->config, 0, sizeof(priv->config));

		GxPlayer_SystemGet(PSYS_VDEC_MAX_FB_SIZE, &priv->max_fb_size);
		GxPlayer_SystemGet(PSYS_VDEC_ENABLE_PPZOOM, &priv->ppzoom_enable);
		GxPlayer_SystemGet(PSYS_VDEC_ENABLE_DEINTERLACE, &priv->deinterlace.enable);
		GxPlayer_SystemGet(PSYS_VDEC_DEINTERLACE_MAX_WIDTH, &priv->deinterlace.max_width);
		GxPlayer_SystemGet(PSYS_VDEC_DEINTERLACE_MAX_HEIGHT, &priv->deinterlace.max_height);
		GxPlayer_SystemGet(PSYS_VDEC_MOSAIC_DROP, &priv->mosaic_drop);
		GxPlayer_SystemGet(PSYS_VDEC_MOSAIC_GATE, &priv->mosaic_gate);
		GxPlayer_SystemGet(PSYS_VDEC_SYNC_FLAG, &priv->sync_flag);
		GxPlayer_SystemGet(PSYS_VDEC_CC_ENABLE,  &priv->userdata.cc.enable);
		GxPlayer_SystemGet(PSYS_VDEC_CC_DISPLAY, &priv->userdata.cc.display);
		GxPlayer_SystemGet(PSYS_VDEC_HDR_ENABLE,  &priv->userdata.hdr.enable);
		GxPlayer_SystemGet(PSYS_VDEC_USERDATA_LENGTH,  &priv->userdata.length);
		GxPlayer_SystemGet(PSYS_VIDEOCODEC_MASK, &codec_mask);
		GxPlayer_SystemGet(PSYS_VDEC_FPS_SOURCE, &fps_source);
		GxPlayer_SystemGet(PSYS_VDEC_SPEED_MODE, &speed_mode);


		priv->speed_mode = NORMAL_SPEED;
		priv->config.mosaic_freez       = priv->mosaic_drop;
		priv->config.err_gate           = priv->mosaic_gate;
		priv->config.fps                = FOLAT2INT(header->fps);
		priv->config.fps_source         = fps_source;
		priv->config.pts_reorder        = header->ds->demuxer->pts_reorder;
		priv->config.deinterlace.uv_deinterlace_en = (priv->deinterlace.enable&VD_FLAG_DEINTERLACE_UV);
		priv->config.deinterlace.max_width = priv->deinterlace.max_width;
		priv->config.deinterlace.max_height = priv->deinterlace.max_height;
		priv->config.userdata.cc.enable  = priv->userdata.cc.enable;
		priv->config.userdata.cc.display = priv->userdata.cc.display;
		priv->config.userdata.hdr.enable = priv->userdata.hdr.enable;
		priv->config.userdata.length = priv->userdata.length;

		if (header->format & codec_mask) {
			gxloge("[Player]: vcodec [0x%x] not support ! \n", header->format);
			return GX_PLAYER_ERROR;
		}

		priv->config.type = vcodec_std2hw(header->format);
		if (priv->config.type == -1) {
			gxloge("[Player]: vcodec [0x%x] not support ! \n", header->format);
			return GX_PLAYER_ERROR;
		}

		priv->capability.type = priv->config.type;
		ret = GxAVGetProperty(priv->dev, priv->mod, \
				GxVideoDecoderPropertyID_Capability, &priv->capability, sizeof(GxVideoDecoderProperty_Capability));
		if (ret != GX_PLAYER_OK){
			return GX_PLAYER_ERROR;
		}

		if(header->disp_w > 0 && header->disp_h > 0) {
			if (header->disp_w > priv->capability.max_width ||
					header->disp_h > priv->capability.max_height) {
				gxloge("[Player]: --------------------------------------------------------\n");
				gxloge("[Player]: Capability: width = %d, height = %d\n", priv->capability.max_width, priv->capability.max_height);
				gxloge("[Player]: FrameInfo : width = %d, height = %d\n", header->disp_w, header->disp_h);
				gxloge("[Player]: --------------------------------------------------------\n");
				return GX_PLAYER_ERROR;
			}
		}

		GxPlayer_SystemGet(PSYS_VDEC_SYNC_TIMEMS,      &videodec_tolerance.low_tolerance);
		GxPlayer_SystemGet(PSYS_VDEC_SYNC_HIGH_TIMEMS, &videodec_tolerance.high_tolerance);
		if (header->ds->demuxer->type == GX_DEMUXER_TYPE_HW_TS || header->ds->demuxer->type == GX_DEMUXER_TYPE_HW)
			;//undo;
		else if (header->ds->demuxer->stream->file_format == GX_STREAMTYPE_STREAM)
			videodec_tolerance.high_tolerance = 60*1000;
		else {
			if (header->ds->demuxer->duration > 1000){
				int ibps, bufsize =0;
				int high_tolerance = 0;
				bufsize = header->ds->demuxer->swdmx->cachesize+header->ds->demuxer->swdmx->esvsize;
				ibps = header->ds->demuxer->stream->end_pos/(header->ds->demuxer->duration/1000);
				ibps = ibps>0?ibps:500000;
				high_tolerance = bufsize/ibps*1000;
				high_tolerance = (high_tolerance > 60000) ? 60000 : high_tolerance;
				high_tolerance = (high_tolerance <  1000) ?  1000 : high_tolerance;
				videodec_tolerance.high_tolerance = high_tolerance;
			}
			else{
				videodec_tolerance.high_tolerance = 5000;
			}
		}

		ret = GxAVSetProperty(priv->dev, priv->mod, \
				GxVideoDecoderPropertyID_Tolerance, &videodec_tolerance, sizeof(GxVideoDecoderProperty_Tolerance));
		if (ret != GX_PLAYER_OK) {
			return GX_PLAYER_ERROR;
		}

		if (priv->capability.workbuf_size) {
			wkb_alloc(priv->config.workbuf, priv->capability.workbuf_size);
			if (priv->config.workbuf.addr == 0){
				gxloge("[Player]: vcodec workbuf Alloc Failed !\n");
				return GX_PLAYER_ERROR;
			}
		}

		priv->config.pts_sync = header->priv.pts_sync == 1 ?  priv->sync_flag : 0;
		gxlogd("[Player]: [vcodec = 0x%x, pts_sync = %d, high_tor = %d, low_tor = %d\n ]\n",
				priv->config.type, priv->config.pts_sync, videodec_tolerance.high_tolerance, videodec_tolerance.low_tolerance);

		ret = GxAVSetProperty(priv->dev, priv->mod, \
				GxVideoDecoderPropertyID_Config, &priv->config, sizeof(GxVideoDecoderProperty_Config));
		if (ret != GX_PLAYER_OK) {
			gxloge("[Player]: vcodec config failed !\n");
			wkb_free(priv->config.workbuf);
			return GX_PLAYER_ERROR;
		}

		vd_in_pin = GxMediaFilter_FindPin(filter, GX_VD_PIN_NAME_IN);

		if (vd_in_pin && vd_in_pin->source) {
			if (vd_in_pin->source->fifo) {

				FifoLink.module = priv->mod;
				FifoLink.pin_id = 0;
				FifoLink.direction = GXAV_PIN_INPUT;
				ret = GxFifo_Link(vd_in_pin->source->fifo, &FifoLink);
				if (ret < 0) {
					gxloge("[Player]: vcodec link failed !\n");
					wkb_free(priv->config.workbuf);
					return GX_PLAYER_ERROR;
				}
				priv->fifo_in = vd_in_pin->source->fifo;
				priv->running = 1;
			}
		}

		set_speed_mod(priv, speed_mode, 0);
		set_dec_mod(priv, VIDEODEC_MODE_NORMAL);

		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_open(GxAVCodec *vd)
{
	VdHwPriv* priv = NULL;

	priv = (void* )av_mallocz(sizeof(VdHwPriv));
	if (priv == NULL)
		return GX_PLAYER_ERROR;

	vd->priv = (void* )priv;

	priv->dev = GxAvdev_CreateDevice(0);
	if (priv->dev < 0) {
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->mod = GxAvdev_OpenModule(priv->dev, GXAV_MOD_VIDEO_DECODE, 0);
	if (priv->mod < 0) {
		GxAvdev_DestroyDevice(priv->dev);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->vpu = GxAvdev_OpenModule(priv->dev, GXAV_MOD_VPU, 0);
	if (priv->vpu < 0) {
		GxAvdev_CloseModule(priv->dev, priv->vpu);
		GxAvdev_DestroyDevice(priv->dev);
		av_free(priv);
		return GX_PLAYER_ERROR;
	}

	priv->chip = GxChipDetect(priv->dev);
	GxCore_MutexCreate(&priv->mutex);

	gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
	return GX_PLAYER_OK;
}

static int vd_hw_start(GxMediaFilter *filter)
{
	int i, j;
	int ret, fb_ext = 1, fb_format = 0xffffffff;
	GxAVCodec *vd = GXDECODEROBJECT(filter);
	VdHwPriv* priv = (VdHwPriv*)vd->priv;
	GxVideoDecoderProperty_FrameInfo frameinfo;
	GxVideoDecoderProperty_Run *runinfo = alloc_runinfo(priv);
	GxStreamVideoHeader* header = (GxStreamVideoHeader*)vd->header;

	if (priv->running == 0) {
		priv->headerok = -1;
		return GX_PLAYER_ERROR;
	}

	ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_FrameInfo, &frameinfo,sizeof(frameinfo));
	if (ret == GXCORE_SUCCESS) {
		if (frameinfo.width < priv->capability.min_width ||
				frameinfo.width > priv->capability.max_width ||
				frameinfo.height < priv->capability.min_height ||
				frameinfo.height > priv->capability.max_height) {
			gxloge("[Player]: --------------------------------------------------------\n");
			gxloge("[Player]: Capability: width = %d, height = %d\n", priv->capability.max_width, priv->capability.max_height);
			gxloge("[Player]: FrameInfo : width = %d, height = %d\n", frameinfo.width, frameinfo.height);
			gxloge("[Player]: --------------------------------------------------------\n");
			goto errout;
		}

		if (frameinfo.dec_fb_size * frameinfo.dec_fb_num + frameinfo.colbuf_size > priv->max_fb_size) {
			gxloge("[Player]: --------------------------------------------------------\n");
			gxloge("[Player]: FrameBuffer Need: %d, %d not enough!\n",
					frameinfo.dec_fb_size * frameinfo.dec_fb_num + frameinfo.colbuf_size, priv->max_fb_size);
			gxloge("[Player]: FrameBuffer Number: %d, size: %d, col: %d\n",
					frameinfo.dec_fb_num, frameinfo.dec_fb_size, frameinfo.colbuf_size);
			gxloge("[Player]: --------------------------------------------------------\n");
			goto errout;
		}

		if (frameinfo.colbuf_size > 0) {
			col_alloc(priv, runinfo->colbuf, frameinfo.colbuf_size);
			if (runinfo->colbuf.addr == 0){
				gxloge("[Player]: Colbuf Alloc Failed !\n");
				goto memerr;
			}
		}

		for (i = 0; i < frameinfo.dec_fb_num; i++) {
			if (i == FREEZE_FB_ID) {
				runinfo->dec_framebuf.fb[i].size = frameinfo.dec_fb_size;
				ret = GxPlayer_SystemFreezeFrameBufferAlloc(0, &runinfo->dec_framebuf.fb[i].addr, &runinfo->dec_framebuf.fb[i].size);
				if (ret != 0) {
					gxloge("[Player]: FreezeFrameBuffer Alloc Failed !\n");
					goto memerr;
				}
				priv->now_fb_size += runinfo->dec_framebuf.fb[i].size;
			} else {
				fb_alloc(priv, runinfo->dec_framebuf.fb[i], frameinfo.dec_fb_size);
				if (runinfo->dec_framebuf.fb[i].addr == 0){
					gxloge("[Player]: VideoFrameBuffer Alloc Failed !\n");
					goto memerr;
				}
			}
			runinfo->dec_framebuf.fb_num++;
		}


		if (priv->deinterlace.enable) {
			if (frameinfo.width <= priv->deinterlace.max_width && frameinfo.height <= priv->deinterlace.max_height) {
				runinfo->deinterlace_en = 1;
				for (i = 0; i < frameinfo.pp_fb_num; i++) {
					fb_alloc(priv, runinfo->pp_framebuf.fb[i], frameinfo.pp_fb_size);
					if (runinfo->pp_framebuf.fb[i].addr == 0) {
						for (j = 0 ; j < i; j++) {
							fb_free(priv, runinfo->pp_framebuf.fb[j]);
						}
						runinfo->deinterlace_en = 0;
						goto memok;
					}
					runinfo->pp_framebuf.fb_num++;
				}
			}
		}

		GxPlayer_SystemGet(PSYS_VDEC_EXTRA_FB, &fb_format);
		fb_format &= header->format;

		if ((fb_format == 0) &&
				(runinfo->pp_framebuf.fb_num == 0))
			fb_ext = 0;

		if (fb_ext) {
			int i = runinfo->dec_framebuf.fb_num;
			fb_alloc(priv, runinfo->dec_framebuf.fb[i], frameinfo.dec_fb_size);
			if (runinfo->dec_framebuf.fb[i].addr != 0) {
				runinfo->dec_framebuf.fb_num++;
				gxlogi("[Player]: Video Extra FB Alloc Successful ! mask = 0x%x .\n", fb_format);
			}
			else {
				gxlogi("[Player]: Video Extra FB Alloc Failed ! mask = 0x%x .\n", fb_format);
			}
		}

memok:
		gxlogi("[Player]: Video FB Alloc Successful ! MinNum = %d, NowNum = %d, width = %d, height = %d .\n",
				frameinfo.dec_fb_num, runinfo->dec_framebuf.fb_num, frameinfo.width, frameinfo.height);

		for (i=0; i<runinfo->dec_framebuf.fb_num; i++) {
			gxlogf("[Player]: %s[%d]:%s : dec_framebuffer[%d], addr=0x%x, size=%d\n",
					__FILE__, __LINE__, __FUNCTION__, i, runinfo->dec_framebuf.fb[i].addr, runinfo->dec_framebuf.fb[i].size);
		}

		for (i=0; i<runinfo->pp_framebuf.fb_num; i++) {
			gxlogf("[Player]: %s[%d]:%s : pp_framebuffer[%d], addr=0x%x, size=%d\n",
					__FILE__, __LINE__, __FUNCTION__, i, runinfo->pp_framebuf.fb[i].addr, runinfo->pp_framebuf.fb[i].size);
		}

		priv->headerok = 1;
		{
			float fps = rate2fps(frameinfo.rate);
			GxPlayer_SystemSet(PSYS_VOUT_AUTOCONFIG, &fps);
		}
		gxlogi("[Player]: %s in.\n", __func__);
		ret = GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Run, runinfo, sizeof(GxVideoDecoderProperty_Run));
		gxlogi("[Player]: %s out.\n", __func__);
		return ret;
	}

memerr:
	for (i=0; i<runinfo->dec_framebuf.fb_num; i++){
		if (i != FREEZE_FB_ID) {
			fb_free(priv, runinfo->dec_framebuf.fb[i]);
		} else {
			priv->now_fb_size -= runinfo->dec_framebuf.fb[i].size;
		}
	}
	for (i=0; i<runinfo->pp_framebuf.fb_num; i++){
		fb_free(priv, runinfo->pp_framebuf.fb[i]);
	}
	col_free(priv, runinfo->colbuf);

errout:
	if (header->ds->demuxer->type == GX_DEMUXER_TYPE_HW_TS || header->ds->demuxer->type == GX_DEMUXER_TYPE_HW) {
		// DVB Reset;
		return vd_hw_reset((GxMediaFilter *)vd);
	}
	else {
		priv->headerok = -1;
		return GX_PLAYER_ERROR;
	}
}

static int vd_hw_check_display(GxAVCodec *vd)
{
	int ret = 0;
	unsigned int event = 0;
	GxVideoDecoderProperty_FrameInfo frameinfo = {0};

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;
		if (priv->displayed_first_frame == 0) {
			GxAVWaitEvents(priv->dev, priv->mod, EVENT_VIDEO_0_DECOVER, 0, &event);
			if (event == EVENT_VIDEO_0_DECOVER) {
				priv->displayed_first_frame = 1;
				ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_FrameInfo, &frameinfo,sizeof(frameinfo));
				if (ret == GXCORE_SUCCESS) {
					GxPlayer_SystemSet(PSYS_STREAM_WIDTH,  &frameinfo.width);
					GxPlayer_SystemSet(PSYS_STREAM_HEIGHT, &frameinfo.height);
					GxPlayer_SystemSet(PSYS_STREAM_STRIDE, &frameinfo.stride);
				}
			}
		}
		ret = priv->displayed_first_frame;
	}

	return ret;
}

static int vd_hw_check_decode(GxAVCodec *vd)
{
	int ret = 0;
	GxVideoDecoderProperty_FrameInfo frameinfo = {0};

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;
		if (priv->decoded_first_frame == 0) {
			ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_FrameInfo, &frameinfo,sizeof(frameinfo));
			if (ret == GXCORE_SUCCESS && frameinfo.stat.decode_frame_cnt) {
				priv->decoded_first_frame = 1;
				priv->decode_finish_tick = av_get_tick(NULL, __LINE__);
			}
		}
		ret = priv->decoded_first_frame;
	}

	return ret;
}

static int vd_hw_control(GxAVCodec *vd, int cmd, void* args)
{
	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;

		switch (cmd) {
		case GX_VD_CTRL_GET_FRAME_INFO:
			{
				int ret;
				GxVideoDecoderProperty_State state ;
				if (((GxStreamVideoHeader*)vd->header)->format == PLAYER_VCODEC_DUMMY)
					return GX_PLAYER_OK;

				ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_State, &state,sizeof(GxVideoDecoderProperty_State));
				if (ret == GXCORE_SUCCESS) {
					if (state.state == VIDEODEC_READY || state.state == VIDEODEC_RUNNING || state.state == VIDEODEC_PAUSED || state.state == VIDEODEC_OVER) {
						GxStreamVideoHeader* sh = NULL;
						GxVideoDecoderProperty_FrameInfo frame;
						ret = GxAVGetProperty(priv->dev, priv->mod,  \
								GxVideoDecoderPropertyID_FrameInfo, &frame,sizeof(GxVideoDecoderProperty_FrameInfo));
						if (ret == GXCORE_SUCCESS) {
							sh = (args == NULL) ? vd->header : args;
							sh->disp_w = frame.width;
							sh->disp_h = frame.height;
							sh->dar    = frame.dar;
							sh->sar    = frame.sar;
							sh->bpp    = frame.bpp;
							sh->ratio  = frame.ratio;
							sh->bitrate = frame.bitrate_bps;
							if (frame.rate != FRAMERATE_NONE)
								sh->fps = rate2fps(frame.rate);
							sh->stat = frame.stat;
							sh->interlace = (frame.type==INTERLACE);
							update_dr_info(sh, &frame.color);
							return GX_PLAYER_OK;
						}
					}
				}
				return GX_PLAYER_ERROR;
			}
		case GX_VD_CTRL_SET_SPEED:
			{
				int ispeed = *(int*)args;
				int speedmode,decmode,syncmode,factor;

				priv->decoded_first_frame = 0;
				priv->decode_finish_tick = 0;
				if (GX_SPEED_NORMAL(ispeed) || GX_SPEED_SLOW(ispeed)) {
					syncmode  = GX_SPEED_SLOW(ispeed) ? VSYNC_NORNAL : priv->config.pts_sync;
					decmode   = VIDEODEC_MODE_NORMAL;
					speedmode = DEFINE_SPEED;
					factor    = ispeed / 10;//100为底的基数
				} else if (GX_SPEED_FAST(ispeed)) {
					syncmode  = 0;
					decmode   = VIDEODEC_MODE_NORMAL;
					speedmode = FAST_SPEED;
					factor    = 0;
				} else if (GX_SPEED_JUMP(ispeed)) {
					syncmode  = 0;
					decmode   = VIDEODEC_MODE_IONLY;
					speedmode = STEP_SPEED;
					factor    = 0;
				}
				else
					return GX_PLAYER_ERROR;

				set_speed_mod(priv, speedmode, factor);
				set_dec_mod(priv, decmode);
				set_sync_mod(priv, syncmode);

				return GX_PLAYER_OK;
			}
		case GX_VD_CTRL_SET_XSPEED:
			{
				int ispeed = *(int*)args;
				int speedmode = 0;
				GxVideoDecoderProperty_Speed Speed;

				if (ispeed == 1000) {
					speedmode = NORMAL_SPEED;
				} else if (ispeed >= 500 && ispeed < 1000) {
					speedmode = SLOW_SPEED_X2;
				} else if (ispeed >= 250 && ispeed < 500) {
					speedmode = SLOW_SPEED_X4;
				} else if (ispeed < 250 && ispeed > 0) {
					speedmode = SLOW_SPEED_X8;
				} else if (ispeed > 1000 && ispeed <= 2000) {
					speedmode = FAST_SPEED_X2;
				} else if (ispeed > 2000 && ispeed <= 4000) {
					speedmode = FAST_SPEED_X4;
				} else if (ispeed > 4000 && ispeed <= 8000) {
					speedmode = FAST_SPEED_X8;
				} else {
					gxloge("[Player]: set xspeed [%d] not support !\n", ispeed);
					return GX_PLAYER_ERROR;
				}

				Speed.mode = speedmode;
				GxAVSetProperty(priv->dev, priv->mod, \
						GxVideoDecoderPropertyID_Speed, &Speed, sizeof(GxVideoDecoderProperty_Speed));
				return GX_PLAYER_OK;
			}
		case GX_VD_CTRL_PAUSE:
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Pause, NULL, 0);
			return GX_PLAYER_OK;
		case GX_VD_CTRL_RESUME:
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Resume, NULL, 0);
			return GX_PLAYER_OK;
		case GX_VD_CTRL_CONTINUE:
			priv->decoded_first_frame = 0;
			priv->decode_finish_tick   = 0;
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Resume, NULL, 0);
			return GX_PLAYER_OK;
		case GX_VD_CTRL_JUMP_RESET:
			{
				int speedmode,decmode,syncmode;
				int hw_reset = (args == NULL) ? 0 : *(int *)args;

				syncmode = 0;
				decmode  = VIDEODEC_MODE_IONLY;
				speedmode = STEP_SPEED;

				GxCore_MutexLock(priv->mutex);
				if (hw_reset) {
					vd_hw_reset((GxMediaFilter *)vd);
					set_speed_mod(priv, speedmode, 0);
					set_dec_mod(priv, decmode);
					set_sync_mod(priv, syncmode);
				} else {
					GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Reset, NULL, 0);
				}

				GxCore_MutexUnlock(priv->mutex);
			}
			return GX_PLAYER_OK;
		case GX_VD_CTRL_UPDATE:
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Update, NULL, 0);
			return GX_PLAYER_OK;
		case GX_VD_CTRL_REFRESH:
			GxAVSetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Refresh, NULL, 0);
			return GX_PLAYER_OK;
		case GX_VD_CTRL_ONE_FRAME_OVER:
			{
				int *ok = (int*)args;
				GxVideoDecoderProperty_OneFrameOver OneFrameOver ;

				if (ok) {
					GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_OneFrameOver, &OneFrameOver,sizeof(GxVideoDecoderProperty_OneFrameOver));
					*ok = OneFrameOver.over;
					return GX_PLAYER_OK;
				}
				return GX_PLAYER_ERROR;
			}
		case GX_VD_CTRL_CHECK_FRAME_OK:
			{
				int *ok = (int*)args;

				if (ok) {
					if (vd_hw_check_display(vd) == 1)
						*ok = 1;
					else if (vd_hw_check_decode(vd) == 1) {
						if (av_get_tick(NULL, __LINE__) - priv->decode_finish_tick > 50)
							*ok = 2;//display timeout
					}
					return GX_PLAYER_OK;
				}
				return GX_PLAYER_ERROR;
			}
		case GX_VD_CTRL_GET_CUR_PTS:
			{
				GxVideoDecoderProperty_PTS vpts ;
				GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Pts, &vpts,sizeof(vpts));
				*(uint32_t*)args = vpts.sync_pts;
				return GX_PLAYER_OK;
			}
		case GX_VD_CTRL_GET_DISPLAY_PTS:
			{
				GxVideoDecoderProperty_PTS vpts ;
				GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_Pts, &vpts,sizeof(vpts));
				*(uint32_t*)args = vpts.disp_pts;
				return GX_PLAYER_OK;
			}
		case GX_VD_CTRL_GET_STATUS:
			{
				int ret;
				AVCodecStatus  *pstate = (AVCodecStatus*)args;
				GxVideoDecoderProperty_State state ;

				if (((GxStreamVideoHeader*)vd->header)->format == PLAYER_VCODEC_DUMMY) {
					pstate->state = AVCODEC_RUNNING;
					return GX_PLAYER_OK;
				}

				ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_State, &state,sizeof(GxVideoDecoderProperty_State));
				if (ret == GXCORE_SUCCESS) {
					pstate->err_code = AVCODEC_ERR_NONE;
					switch(state.state) {
					case VIDEODEC_RUNNING:
						pstate->state = AVCODEC_RUNNING;
						break;
					case VIDEODEC_PAUSED:
						pstate->state = AVCODEC_PAUSED;
						break;
					case VIDEODEC_STOPED:
						pstate->state = AVCODEC_STOPPED;
						break;
					case VIDEODEC_OVER:
						pstate->state = AVCODEC_FINISHED;
						break;
					case VIDEODEC_ERROR:
						pstate->state = AVCODEC_ERROR;
						switch(state.err_code)
						{
						case VIDEODEC_ERR_NONE:
							pstate->err_code = AVCODEC_ERR_NONE;
							break;
						case VIDEODEC_ERR_ESV_OVERFLOW:
							pstate->err_code = AVCODEC_ERR_ESBUF_OVERFLOW;
							break;
						case VIDEODEC_ERR_SIZE_UNSUPPORT:
							pstate->err_code = AVCODEC_ERR_SIZE_UNSUPPORT;
							break;
						case VIDEODEC_ERR_SIZE_CHANGED:
							pstate->err_code = AVCODEC_ERR_SIZE_CHANGED;
							break;
						case VIDEODEC_ERR_FB_OVERFLOW:
							pstate->err_code = AVCODEC_ERR_FB_OVERFLOW;
							break;
						case VIDEODEC_ERR_CODECTYPE_UNSUPPORT:
							pstate->err_code = AVCODEC_ERR_CODECTYPE_UNSUPPORT;
							break;
						default:
							pstate->err_code = AVCODEC_ERR_NONE;
							break;
						}
						break;
					case VIDEODEC_BUSY:
						pstate->state = AVCODEC_BUSY;
						break;
					case VIDEODEC_READY:
						pstate->state = AVCODEC_READY;
						priv->headerok = 0;
						break;
					case VIDEODEC_STARTED:
						pstate->state = AVCODEC_STARTED;
						break;
					default:
						pstate->state = AVCODEC_STOPPED;
					}
					return GX_PLAYER_OK;
				}
				return GX_PLAYER_ERROR;
			}
		case GX_VD_CTRL_START_DECODE:
			if (priv->headerok == 0) {
				int ret;
				GxVideoDecoderProperty_State state ;
				ret = GxAVGetProperty(priv->dev, priv->mod, GxVideoDecoderPropertyID_State, &state,sizeof(GxVideoDecoderProperty_State));
				if (ret == GXCORE_SUCCESS && state.state == VIDEODEC_READY) {
					unsigned int event = 0;
					GxAVWaitEvents(priv->dev, priv->mod, EVENT_VIDEO_0_FRAMEINFO|EVENT_VIDEO_0_SEQ_CHANGED, 0, &event);
					if (event == EVENT_VIDEO_0_FRAMEINFO) {
						release_run_resource(priv, current_runinfo(priv));
						*(uint32_t*)args = 1;
						return vd_hw_start((GxMediaFilter *)vd);
					}
					if (event == EVENT_VIDEO_0_SEQ_CHANGED) {
						//release_run_resource(priv, current_runinfo(priv));
						GxCore_ThreadDelay(40);
						ret = vd_hw_start((GxMediaFilter *)vd);
						if (ret != GX_PLAYER_OK)
							return vd_hw_reset((GxMediaFilter *)vd);
					}
				}
			}
			break;
		case GX_VD_CTRL_SET_SYNC:
			if (priv->running == 1) {
				((GxStreamVideoHeader*)vd->header)->priv.pts_sync = *(int*)args;
				set_sync_mod(priv, *(int*)args);
			}
			break;
		case GX_VD_CTRL_SET_DELAY:
			{
				GxVideoDecoderProperty_PtsOffset pts;

				pts.offset_ms = *(int*)args;
				return GxAVSetProperty(priv->dev, priv->mod,
						GxVideoDecoderPropertyID_PtsOffset, &pts, sizeof(GxVideoDecoderProperty_PtsOffset));
			}
		case GX_VD_CTRL_READ_SUBCC:
			{
				AVCodecSubCC *subcc = (AVCodecSubCC *)args;
				GxVideoDecoderProperty_UserData userdata;

				userdata.data = subcc->data;
				userdata.size = subcc->size;
				subcc->size = GxAVGetProperty(priv->dev, priv->mod,
						GxVideoDecoderPropertyID_UserData, &userdata, sizeof(GxVideoDecoderProperty_UserData));
			}
			break;
		case GX_VD_CTRL_CHECK_DYNAMIC_RANGE_CHANGE:
			{
				int ret;
				unsigned int event = 0;
				ret = GxAVWaitEvents(priv->dev, priv->mod, EVENT_VIDEO_0_COLORINFO_CHANGED, 0, &event);
				if (event == EVENT_VIDEO_0_COLORINFO_CHANGED)
					return GX_PLAYER_OK;
				else
					return GX_PLAYER_ERROR;
			}
			break;
		case GX_VD_CTRL_FREEZE_VIDEO:
			priv->freezed = *(int*)args;
			break;
		default:
			return GX_PLAYER_ERROR;
		}
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_close(GxAVCodec *vd)
{
	if (vd && vd->priv) {
		VdHwPriv *priv = (VdHwPriv*)vd->priv;

		vd_hw_stop((GxMediaFilter*)vd);
		gxlogf("[Player]: %s[%d]:%s\n", __FILE__, __LINE__, __FUNCTION__);
		GxAvdev_CloseModule(priv->dev, priv->mod);
		GxAvdev_CloseModule(priv->dev, priv->vpu);
		GxAvdev_DestroyDevice(priv->dev);
		GxCore_MutexDelete(priv->mutex);
		av_free(vd->priv);
		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

static int vd_hw_decode(GxAVCodec *vd, void **outdata, int *outdata_size, uint8_t * buf, int buf_size)
{
	return GX_PLAYER_OK;
}

static int vd_hw_reset(GxMediaFilter *filter)
{
	GxAVCodec *vd = GXDECODEROBJECT(filter);

	if (vd && vd->priv) {
		VdHwPriv* priv = (VdHwPriv*)vd->priv;
		vd_hw_stop(filter);
		GxFifo_Reset(priv->fifo_in);
		av_memhole_flush();
		vd_hw_config(filter);
		vd_hw_run(filter);

		return GX_PLAYER_OK;
	}

	return GX_PLAYER_ERROR;
}

GxAVCodecClass gx_decoder_vd_hw = {
	._inherit = {
		._inherit = {
			.name    = "VideoDecoderHardWare",
			.parent  = (void* )&gx_VideoDecoderBase,
			.size    = sizeof(GxVideoCodec),
		},
		.run    = vd_hw_run,
		.pause  = vd_hw_pause,
		.resume = vd_hw_resume,
		.config = vd_hw_config,
		.stop   = vd_hw_stop,
		.abort  = vd_hw_abort,
		.reset  = vd_hw_reset,
		.wait   = vd_hw_wait,
	},
	DEF_AUTHOR("VideoDecoder","hw","No description","L.F","No comment"),

	.name    = "HW  DECODER",
	.format  = {
		VIDEO_CODEC_MPEG12,
		VIDEO_CODEC_MPEG4,
		VIDEO_CODEC_H263,
		VIDEO_CODEC_H264,
		VIDEO_CODEC_REAL,
		VIDEO_CODEC_AVS,
		VIDEO_CODEC_H265,
		VIDEO_CODEC_VC1,
		VIDEO_CODEC_MJPEG,
		PLAYER_VCODEC_DUMMY,
		-1 },
	.open    = vd_hw_open,
	.close   = vd_hw_close,
	.control = vd_hw_control,
	.decode  = vd_hw_decode,
};
