#include "gxmedia_module.h"
#include "gxmedia_common.h"

static int _alloc_frame_info(GxMediaVideo* mvideo, GxVideoDecoderProperty_Run* runinfo)
{
	int ret=0, i=0, j;
	GxVideoDecoderProperty_FrameInfo frameinfo;

	ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
			GxVideoDecoderPropertyID_FrameInfo, &frameinfo, sizeof(GxVideoDecoderProperty_FrameInfo));
	CHECK_RETURN_VALUE(ret);

	if(frameinfo.colbuf_size > 0) {
		runinfo->colbuf.addr = (unsigned int)GxMediaAPI_MemAlloc(frameinfo.colbuf_size, GX_PINFLAG_FCOL);
		if(runinfo->colbuf.addr == 0){
			GxMediaAPI_debug("[%s %d]: colbuf size 0x%x error\n",__FUNCTION__, __LINE__, frameinfo.colbuf_size);
			goto memerr;
		}
		runinfo->colbuf.size = frameinfo.colbuf_size;
	} else {
		runinfo->colbuf.addr = 0;
		runinfo->colbuf.size = 0;
	}

	for(i=0; i<frameinfo.dec_fb_num; i++) {
		if(i == FREEZE_FB_ID) {
			runinfo->dec_framebuf.fb[i].size = frameinfo.dec_fb_size;
			GxMediaAPI_FreezeFrameAlloc(mvideo->mod_id, runinfo->dec_framebuf.fb[i].addr, runinfo->dec_framebuf.fb[i].size);
			if(runinfo->dec_framebuf.fb[i].addr == 0) {
				GxMediaAPI_debug("[%s %d]: frame size error\n", __FUNCTION__, __LINE__);
				goto memerr;
			}
		}
		else {
			runinfo->dec_framebuf.fb[i].addr = (unsigned int)GxMediaAPI_MemAlloc(frameinfo.dec_fb_size, GX_PINFLAG_FBV);
			runinfo->dec_framebuf.fb[i].size = frameinfo.dec_fb_size;
			if(runinfo->dec_framebuf.fb[i].addr == 0){
				GxMediaAPI_debug("[%s %d]: frame size 0x%x error\n", __FUNCTION__, __LINE__, frameinfo.dec_fb_size);
				goto memerr;
			}
		}
		runinfo->dec_framebuf.fb_num++;
	}

	for(i=0; i<frameinfo.pp_fb_num; i++) {
		runinfo->pp_framebuf.fb[i].addr = (unsigned int)GxMediaAPI_MemAlloc(frameinfo.pp_fb_size, GX_PINFLAG_FBV);
		runinfo->pp_framebuf.fb[i].size = frameinfo.pp_fb_size;
		if(runinfo->pp_framebuf.fb[i].addr == 0){
			for(j = 0; j < i; j++) {
				if(runinfo->pp_framebuf.fb[i].addr)
					GxMediaAPI_MemFree((unsigned char*)runinfo->pp_framebuf.fb[i].addr, GX_PINFLAG_FBV);
				runinfo->pp_framebuf.fb[i].size = 0;
				runinfo->pp_framebuf.fb[i].addr = 0;
			}
			goto memok;
		}
		runinfo->pp_framebuf.fb_num++;
	}

memok:
	GxMediaAPI_debug("[%s %d]: colbuf addr=0x%x, size=%d\n",
			__FUNCTION__, __LINE__, runinfo->colbuf.addr, runinfo->colbuf.size);
	for(i=0; i<runinfo->dec_framebuf.fb_num; i++) {
		GxMediaAPI_debug("[%s %d]: dec_framebuffer[%d], addr=0x%x, size=%d\n",
				__FUNCTION__, __LINE__, i, runinfo->dec_framebuf.fb[i].addr, runinfo->dec_framebuf.fb[i].size);
	}
	for(i=0; i<runinfo->pp_framebuf.fb_num; i++) {
		GxMediaAPI_debug("[%s %d]: pp_framebuffer[%d], addr=0x%x, size=%d\n",
				__FUNCTION__, __LINE__, i, runinfo->pp_framebuf.fb[i].addr, runinfo->pp_framebuf.fb[i].size);
	}
	return 0;

memerr:
	for(i=0; i<runinfo->dec_framebuf.fb_num; i++){
		if(i != FREEZE_FB_ID){
			if(runinfo->dec_framebuf.fb[i].addr)
				GxMediaAPI_MemFree((unsigned char*)runinfo->dec_framebuf.fb[i].addr, GX_PINFLAG_FBV);
			runinfo->dec_framebuf.fb[i].addr = 0;
			runinfo->dec_framebuf.fb[i].size = 0;
		}
	}
	if(runinfo->colbuf.addr) {
		GxMediaAPI_MemFree((unsigned char*)runinfo->colbuf.addr, GX_PINFLAG_FCOL);
		runinfo->colbuf.addr = 0;
	}
	av_memhole_flush();
	runinfo->pp_framebuf.fb_num  = 0;
	runinfo->dec_framebuf.fb_num = 0;

	return -1;
}

static void _free_frame_info(GxVideoDecoderProperty_Run* runinfo)
{
	int i = 0;

	for(i=0; i<runinfo->dec_framebuf.fb_num; i++){
		if(i != FREEZE_FB_ID){
			if(runinfo->dec_framebuf.fb[i].addr)
				GxMediaAPI_MemFree((unsigned char*)runinfo->dec_framebuf.fb[i].addr, GX_PINFLAG_FBV);
			runinfo->dec_framebuf.fb[i].addr = 0;
			runinfo->dec_framebuf.fb[i].size = 0;
		}
	}

	for(i=0; i<runinfo->pp_framebuf.fb_num; i++){
		if (runinfo->pp_framebuf.fb[i].addr)
			GxMediaAPI_MemFree((unsigned char*)runinfo->pp_framebuf.fb[i].addr, GX_PINFLAG_FBV);
		runinfo->pp_framebuf.fb[i].addr = 0;
		runinfo->pp_framebuf.fb[i].size = 0;
	}

	if(runinfo->colbuf.addr) {
		GxMediaAPI_MemFree((unsigned char*)runinfo->colbuf.addr, GX_PINFLAG_FCOL);
		runinfo->colbuf.addr = 0;
	}

	av_memhole_flush();
	runinfo->dec_framebuf.fb_num = 0;
	runinfo->pp_framebuf.fb_num = 0;
}

static int _videodec_alloc(GxMediaVideo *mvideo)
{
	int ret = 0;
	int idx = 0;

	if (mvideo->run_info_id >= 0)
		idx = (mvideo->run_info_id + 1) % VIDEO_MAX_RUN_INFO;

	_free_frame_info(&mvideo->run_info[idx]);
	ret = _alloc_frame_info(mvideo, &mvideo->run_info[idx]);
	if (ret == 0)
		mvideo->run_info_id = idx;

	return ret;
}

static void _videodec_free(GxMediaVideo *mvideo)
{
	int i = 0;

	for (i = 0; i < VIDEO_MAX_RUN_INFO; i++)
		_free_frame_info(&mvideo->run_info[i]);
	mvideo->run_info_id = -1;

	return;
}

static void _videodec_callback(GxMediaVideo *mvideo)
{
	if (mvideo->params.vf_cb) {
		GxVideoDecoderProperty_FrameInfo frameinfo;
		GxMediaVideoFrame frame;

		GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
				GxVideoDecoderPropertyID_FrameInfo, &frameinfo, sizeof(GxVideoDecoderProperty_FrameInfo));
		frame.width  = frameinfo.width;
		frame.height = frameinfo.height;
		mvideo->params.vf_cb(&frame);
	}

	return;
}

static int GxVideoDec_Open(GxMediaVideo* mvideo, int modid)
{
	mvideo->dec_dev = GxAvdev_CreateDevice(0);
	if (mvideo->dec_dev < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder device create failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	mvideo->dec_mod = GxAvdev_OpenModule(mvideo->dec_dev, GXAV_MOD_VIDEO_DECODE, modid);
	if (mvideo->dec_mod < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder module create failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static void GxVideoDec_Close(GxMediaVideo* mvideo)
{
	if (mvideo->mdemux == NULL) {
		if(mvideo->es_fifo != -1)
			GxAVFifoDestroy(mvideo->dec_dev, mvideo->es_fifo);

		if(mvideo->es_buffer)
			GxMediaAPI_MemFree(mvideo->es_buffer, GX_PINFLAG_SESV);
	}

	if(mvideo->dec_mod != -1)
		GxAvdev_CloseModule(mvideo->dec_dev, mvideo->dec_mod);

	if(mvideo->dec_dev != -1)
		GxAvdev_DestroyDevice(mvideo->dec_dev);

	mvideo->es_fifo = -1;
	mvideo->es_buffer = NULL;
	mvideo->dec_mod = -1;
	mvideo->dec_dev = -1;
}

static int GxVideoDec_Config(GxMediaVideo* mvideo, GxMediaSyncParam *sync)
{
	int ret = -1;
	GxVideoDecoderProperty_Config     dec_config;
	GxVideoDecoderProperty_Tolerance  dec_tolerance;
	GxVideoDecoderProperty_Capability videodec_capability;
	GxVideoDecoderProperty_Speed      speed;
	GxAvGateInfo fifo_gate;

	if (mvideo->mdemux == NULL) {
		mvideo->es_buffer = GxMediaAPI_MemAlloc(VIDEO_ESV_SIZE, GX_PINFLAG_SESV);
		if(mvideo->es_buffer == NULL){
			GxMediaAPI_debug("[%s %d]: video esv hole create error\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}

		mvideo->es_fifo = GxAVFifoCreate(mvideo->dec_dev, mvideo->es_buffer, VIDEO_ESV_SIZE,  GXAV_PTS_FIFO);
		if(mvideo->es_fifo == -1){
			GxMediaAPI_debug("[%s %d]: video esv fifo create error\n", __FUNCTION__, __LINE__);
			GxMediaAPI_MemFree(mvideo->es_buffer, GX_PINFLAG_SESV);
			mvideo->es_buffer = NULL;
			return GX_MEDIAAPI_ERROR;;
		}

		GxMediaAPI_debug("[%s %d]: esa fifo size %d, used %d\n", __FUNCTION__, __LINE__,
				GxAVFifoGetCap(mvideo->dec_dev, mvideo->es_fifo), GxAVFifoGetLength(mvideo->dec_dev, mvideo->es_fifo));
	} else {
		mvideo->es_fifo = GxMedia_DemuxGetVfifo(mvideo->mdemux);
	}

	fifo_gate.almost_empty =  VIDEO_ESV_SIZE*7/8;
	fifo_gate.almost_full  =  1024;
	GxAVFifoConfig(mvideo->dec_dev, mvideo->es_fifo, &fifo_gate);

	memset(&dec_config, 0, sizeof(dec_config));
	dec_config.type = vcodec_std2hw(mvideo->params.codectype);
	if (dec_config.type == -1) {
		GxMediaAPI_debug("[%s %d]: video decoder config format %x error\n", __FUNCTION__, __LINE__, mvideo->params.codectype);
		return GX_MEDIAAPI_ERROR;
	}

	videodec_capability.type = dec_config.type;
	ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
			GxVideoDecoderPropertyID_Capability, &videodec_capability, sizeof(GxVideoDecoderProperty_Capability));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder get capability error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	if ((mvideo->params.width > 0) && (mvideo->params.height > 0)) {
		if(mvideo->params.width < videodec_capability.min_width ||
				mvideo->params.width > videodec_capability.max_width ||
				mvideo->params.height < videodec_capability.min_height ||
				mvideo->params.height > videodec_capability.max_height){
			GxMediaAPI_debug("[%s %d]: video decoder width %d and height %d error\n",
					__FUNCTION__, __LINE__, mvideo->params.width, mvideo->params.height);
			return GX_MEDIAAPI_ERROR;
		}
	}

	if (videodec_capability.workbuf_size)  {
		mvideo->work_mem = GxMediaAPI_MemAlloc(videodec_capability.workbuf_size, GX_PINFLAG_WKV);
		if(mvideo->work_mem == NULL){
			GxMediaAPI_debug("[%s %d]: video decoder work mem malloc error\n", __FUNCTION__, __LINE__);
			return GX_MEDIAAPI_ERROR;
		}
	}


	dec_tolerance.high_tolerance = sync->high_gate;
	dec_tolerance.low_tolerance  = sync->low_gate;
	ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, \
			GxVideoDecoderPropertyID_Tolerance, &dec_tolerance, sizeof(GxVideoDecoderProperty_Tolerance));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder config tolerance error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	switch (mvideo->params.speed_mode)
	{
		case GXMEDIA_VIDEO_FAST_SPEED:
			speed.mode = FAST_SPEED;
			break;
		case GXMEDIA_VIDEO_SLOW_SPEED:
			speed.mode = SLOW_SPEED;
			break;
		case GXMEDIA_VIDEO_STEP_SPEED:
			speed.mode = STEP_SPEED;
			break;
		case GXMEDIA_VIDEO_NORMAL_SPEED:
		default:
			speed.mode = NORMAL_SPEED;
			break;
	}
	GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, \
			GxVideoDecoderPropertyID_Speed, &speed, sizeof(GxVideoDecoderProperty_Speed));

	dec_config.pts_sync = sync->enable;
	dec_config.workbuf.addr = (unsigned int)mvideo->work_mem;
	dec_config.workbuf.size = videodec_capability.workbuf_size;
	ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, \
			GxVideoDecoderPropertyID_Config, &dec_config, sizeof(GxVideoDecoderProperty_Config));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder config error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}
	GxAVFifoReset(mvideo->dec_dev, mvideo->es_fifo);

	return GX_MEDIAAPI_SUCCESS;
}

static void GxVideoDec_WaitEvent(void* data)
{
	int ret = 0;
	int dst_event = 0;
	GxMediaVideo* mvideo = (GxMediaVideo*)data;
	int vdec_running = 0;

	while (mvideo->waiting) {
		unsigned int event = 0;
		if (mvideo->mod_id == 0)
			dst_event = EVENT_VIDEO_0_SEQ_CHANGED|EVENT_VIDEO_0_FRAMEINFO|EVENT_VIDEO_0_ONE_FRAME_OVER|EVENT_VIDEO_0_FINISH;
		else
			dst_event = EVENT_VIDEO_1_SEQ_CHANGED|EVENT_VIDEO_1_FRAMEINFO|EVENT_VIDEO_1_ONE_FRAME_OVER|EVENT_VIDEO_1_FINISH;
		ret = GxAVWaitEvents(mvideo->dec_dev, mvideo->dec_mod, dst_event, 1000*1000, &event);
		if (ret == GXCORE_SUCCESS) {
			gxlogd ("[APP-%d] event = 0x%x\n", mvideo->mod_id, event);
			if (event&EVENT_VIDEO_0_FRAMEINFO || event&EVENT_VIDEO_1_FRAMEINFO ) {
				mvideo->run_info_id = -1;
				ret = _videodec_alloc(mvideo);
				if (ret < 0) {
					GxMediaAPI_debug("[%s %d]: video decoder probe frame error\n", __FUNCTION__, __LINE__);
					return;
				}

				ret = GxAVSetProperty(mvideo->dec_dev,
						mvideo->dec_mod,
						GxVideoDecoderPropertyID_Run,
						&mvideo->run_info[mvideo->run_info_id],
						sizeof(GxVideoDecoderProperty_Run));
				if (ret < 0) {
					GxMediaAPI_debug("[%s %d]: video decoder probe frame error\n", __FUNCTION__, __LINE__);
					return;
				}
				_videodec_callback(mvideo);
				vdec_running = 1;
#if defined(NO_OS)
				break;
#endif
			}


#if(defined(ECOS_OS) || defined(LINUX_OS))

			if (event & EVENT_VIDEO_0_SEQ_CHANGED || event & EVENT_VIDEO_1_SEQ_CHANGED) {
				ret = _videodec_alloc(mvideo);
				if (ret < 0) {
					GxVideoDecoderProperty_Stop Stop;

					Stop.freeze = 1;
					GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod,
						GxVideoDecoderPropertyID_Stop, &Stop, sizeof(GxVideoDecoderProperty_Stop));
					_videodec_free(mvideo);
					GxAVFifoReset(mvideo->dec_dev, mvideo->es_fifo);
					ret = _videodec_alloc(mvideo);
					if (ret < 0) {
						GxMediaAPI_debug("[%s %d]: video decoder probe frame error\n", __FUNCTION__, __LINE__);
						return;
					}
				}

				ret = GxAVSetProperty(mvideo->dec_dev,
						mvideo->dec_mod,
						GxVideoDecoderPropertyID_Run,
						&mvideo->run_info[mvideo->run_info_id],
						sizeof(GxVideoDecoderProperty_Run));
				if (ret < 0) {
					GxMediaAPI_debug("[%s %d]: video decoder probe frame error\n", __FUNCTION__, __LINE__);
					return;
				}
				_videodec_callback(mvideo);
			}
#endif
			if (event&EVENT_VIDEO_0_ONE_FRAME_OVER || event&EVENT_VIDEO_1_ONE_FRAME_OVER) {
				if (mvideo->params.speed_mode == GXMEDIA_VIDEO_STEP_SPEED) {
					gxlogd("\n[MediaAPI-%d] one frame over!\n", mvideo->mod_id);
					if (mvideo->params.each_frame_cb)
						mvideo->params.each_frame_cb(mvideo->mod_id);
					//ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Resume, NULL, 0);
				}
			}

			if (event&EVENT_VIDEO_0_FINISH|| event&EVENT_VIDEO_1_FINISH) {
				gxlogd("\n[MediaAPI-%d] stream finish!\n", mvideo->mod_id);
				if (mvideo->params.stream_finish_cb)
					mvideo->params.stream_finish_cb(mvideo->mod_id);
			}
		}
		//have a rest!
		GxCore_ThreadDelay(DELAY10MS);
	}

	return;
}

static int GxVideoDec_Start(GxMediaVideo* mvideo)
{
	int ret = 0;

	GxAVLinkFifo(mvideo->dec_dev,  mvideo->dec_mod, mvideo->es_fifo, 0, GXAV_PIN_INPUT);

	ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Probe, NULL, 0);
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder probe frame error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	mvideo->waiting = 1;

	GxMediaAPI_CreateThread("video wait", &mvideo->waitthread,
			GxVideoDec_WaitEvent, (void*)mvideo, 1024*16, GXOS_DEFAULT_PRIORITY);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_Stop(GxMediaVideo* mvideo, int freeze)
{
	GxVideoDecoderProperty_Stop Stop;

	mvideo->waiting = 0;
	if (mvideo->waitthread)
		GxMediaAPI_JoinThread(mvideo->waitthread);
	Stop.freeze = (freeze==0)?0:1;
	GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod,
			GxVideoDecoderPropertyID_Stop, &Stop, sizeof(GxVideoDecoderProperty_Stop));
	GxAVResetEvent(mvideo->dec_dev, mvideo->dec_mod, EVENT_VIDEO_0_DECOVER);
	GxAVResetEvent(mvideo->dec_dev, mvideo->dec_mod, EVENT_VIDEO_0_FRAMEINFO);
	GxAVUnLinkFifo(mvideo->dec_dev, mvideo->dec_mod, mvideo->es_fifo);
	_videodec_free(mvideo);
	if (mvideo->work_mem) {
		GxMediaAPI_MemFree(mvideo->work_mem, GX_PINFLAG_WKV);
		mvideo->work_mem = NULL;
	}

	if (mvideo->mdemux == NULL) {
		if(mvideo->es_fifo != -1)
			GxAVFifoDestroy(mvideo->dec_dev, mvideo->es_fifo);

		if(mvideo->es_buffer)
			GxMediaAPI_MemFree(mvideo->es_buffer, GX_PINFLAG_SESV);
	}
	mvideo->es_fifo = -1;
	mvideo->es_buffer = NULL;

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_Pause(GxMediaVideo* mvideo)
{
	GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Pause, NULL,0);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_Resume(GxMediaVideo* mvideo)
{
	GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Resume, NULL,0);

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_GetEsvInfo(GxMediaVideo* mvideo, GxMediaModuleVideoFifo *info)
{
	int esv_fifo_size = 0;
	int esv_used_size = 0;

	if (mvideo->es_fifo == -1)
		return GX_MEDIAAPI_ERROR;

	esv_fifo_size = GxAVFifoGetCap   (mvideo->dec_dev, mvideo->es_fifo);
	esv_used_size = GxAVFifoGetLength(mvideo->dec_dev, mvideo->es_fifo);

	if (info) {
		info->esv_free_size = esv_fifo_size - esv_used_size;
		info->esv_used_size = esv_used_size;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_GetDecState(GxMediaVideo* mvideo, GxMediaDecState *state, GxMediaDecErrCode *err_code)
{
	int ret = GXCORE_SUCCESS;
	GxVideoDecoderProperty_State dec_state ;

	ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
			GxVideoDecoderPropertyID_State, &dec_state, sizeof(GxVideoDecoderProperty_State));
	if (ret != GXCORE_SUCCESS)
		return GX_MEDIAAPI_ERROR;

	if ((state == NULL) || (err_code == NULL))
		return GX_MEDIAAPI_ERROR;

	switch (dec_state.state) {
	case VIDEODEC_BUSY:
	case VIDEODEC_READY:
	case VIDEODEC_STARTED:
	case VIDEODEC_RUNNING:
	case VIDEODEC_PAUSED:
		*state    = GXMEDIA_STATE_RUNNING;
		*err_code = GXMEDIA_ERR_NO_ERROR;
		break;
	case VIDEODEC_STOPED:
	case VIDEODEC_OVER:
	case VIDEODEC_TIMEOUT:
		*state    = GXMEDIA_STATE_STOPPED;
		*err_code = GXMEDIA_ERR_NO_ERROR;
		break;
	case VIDEODEC_ERROR:
		*state    = GXMEDIA_STATE_ERROR;
		switch (dec_state.err_code) {
		case VIDEODEC_ERR_ESV_OVERFLOW:
			*err_code = GXMEDIA_ERR_ES_OVERFLOW;
			break;
		case VIDEODEC_ERR_SIZE_UNSUPPORT:
			*err_code = GXMEDIA_ERR_SIZE_UNSPPORT;
			break;
		case VIDEODEC_ERR_SIZE_CHANGED:
			*err_code = GXMEDIA_ERR_SIZE_CHANGED;
			break;
		case VIDEODEC_ERR_FB_OVERFLOW:
			*err_code = GXMEDIA_ERR_FB_OVERFLOW;
			break;
		case VIDEODEC_ERR_CODECTYPE_UNSUPPORT:
			*err_code = GXMEDIA_ERR_DEC_UNSUPPORT;
			break;
		default:
			*err_code = GXMEDIA_ERR_UNKOWN_ERROR;
			break;
		}
		break;
	default:
		*state    = GXMEDIA_STATE_NO_DEC;
		*err_code = GXMEDIA_ERR_NO_ERROR;
	}
	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_GetFrameInfo(GxMediaVideo* mvideo, GxMediaVideoFrameInfo *info)
{
	int ret = GXCORE_SUCCESS;
	GxVideoDecoderProperty_FrameInfo frameinfo;

	ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
			GxVideoDecoderPropertyID_FrameInfo, &frameinfo, sizeof(GxVideoDecoderProperty_FrameInfo));
	CHECK_RETURN_VALUE(ret);

	if (info) {
		info->width  = frameinfo.width;
		info->height = frameinfo.height;
		info->dec_frame_cnt  = frameinfo.stat.decode_frame_cnt;
		info->disp_frame_cnt = frameinfo.stat.play_frame_cnt;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_GetTime(GxMediaVideo* mvideo, int64_t* dec_time)
{
	GxVideoDecoderProperty_PTS vpts ;

	GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Pts, &vpts,sizeof(vpts));
	if(dec_time)
		*dec_time = (int64_t)vpts.sync_pts;

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_GetHash(GxMediaVideo* mvideo, GxMediaVideoFrameHash *hash)
{
	int ret = -1;
	GxVideoDecoderProperty_FrameHash frame_hash = {0};

	if (hash) {
		ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_FrameHash, &frame_hash,sizeof(frame_hash));
		if (ret == 0) {
			hash->frame_id = frame_hash.frame_id;
			memcpy(hash->hash, frame_hash.hash, sizeof(hash->hash));
		}
	}

	return ret;
}

static int GxVideoDec_Eos(GxMediaVideo* mvideo)
{
	int ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Update, NULL, 0);

	return (ret == 0) ? GX_MEDIAAPI_SUCCESS : GX_MEDIAAPI_ERROR;
}

static int GxVideoDec_Finished(GxMediaVideo* mvideo)
{
	int ret = 0;
	GxVideoDecoderProperty_State state ;

	ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, GxVideoDecoderPropertyID_Update, NULL, 0);
	while (ret >= 0) {
		ret = GxAVGetProperty(mvideo->dec_dev, mvideo->dec_mod,
				GxVideoDecoderPropertyID_State, &state, sizeof(GxVideoDecoderProperty_State));
		if(state.state == VIDEODEC_OVER ||
				state.state == VIDEODEC_STOPED ||
				state.state == VIDEODEC_READY ||
				state.state == VIDEODEC_ERROR)
			break;
		GxCore_ThreadDelay(DELAY10MS);
	}

	return GX_MEDIAAPI_SUCCESS;

}

static int GxVideoDec_Speed(GxMediaVideo* mvideo, float speed)
{
	int ret = 0;
	GxVideoDecoderProperty_Speed Speed;

	if (speed < 0) return GX_MEDIAAPI_ERROR;

	Speed.mode   = DEFINE_SPEED;
	Speed.factor = (unsigned int)(100 * speed);
	ret = GxAVSetProperty(mvideo->dec_dev, mvideo->dec_mod, \
			GxVideoDecoderPropertyID_Speed, &Speed, sizeof(GxVideoDecoderProperty_Speed));
	if (ret < 0) {
		GxMediaAPI_debug("[%s %d]: video decoder set speed error\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

static int GxVideoDec_InjectData(GxMediaVideo* mvideo, uint8_t* buffer, int len, int pts)
{
	int ret = 0;
	int write_size = 0;
	GxMediaModuleVideoFifo info;

	ret = GxVideoDec_GetEsvInfo(mvideo, &info);
	if (ret != GX_MEDIAAPI_SUCCESS)
		return -1;

	if (info.esv_free_size <= len)
		return 0;

	write_size = GxAVFifoWriteDataPts(mvideo->dec_dev, mvideo->es_fifo, buffer, len, -1, pts);
	while(write_size < len){
		ret = GxAVFifoWriteData(mvideo->dec_dev, mvideo->es_fifo, buffer+write_size, len-write_size, -1);
		if(ret < 0){
			GxMediaAPI_debug("[%s %d]: video decoder write data error\n", __FUNCTION__, __LINE__);
			return -1;
		}
		write_size += ret;
		GxCore_ThreadDelay(DELAY10MS);
	}
	return write_size;
}

static int GxVideoOut_Open(GxMediaVideo* mvideo, int modid)
{
	GxVpuProperty_VirtualResolution Resolution;
	GxVpuProperty_LayerMainSurface  MainSurface;

	mvideo->out_dev = GxAvdev_CreateDevice(0);
	if (mvideo->out_dev < 0) {
		GxMediaAPI_debug("[%s %d]: video out device create failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	mvideo->out_mod = GxAvdev_OpenModule(mvideo->out_dev, GXAV_MOD_VIDEO_OUT, modid);
	if (mvideo->out_mod < 0) {
		GxMediaAPI_debug("[%s %d]: video out module create failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	mvideo->vpu_mod = GxAvdev_OpenModule(mvideo->out_dev, GXAV_MOD_VPU, 0);
	if (mvideo->vpu_mod < 0) {
		GxMediaAPI_debug("[%s %d]: video out module create failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	memset((void*)&Resolution, 0, sizeof(GxVpuPropertyID_VirtualResolution));
	GxAVGetProperty(mvideo->out_dev, mvideo->vpu_mod, \
			GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));
	if (Resolution.xres == 0 || Resolution.yres == 0) {
		Resolution.xres = 1920;
		Resolution.yres = 1080;
		GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod, \
				GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));
	}

	memset((void*)&MainSurface, 0, sizeof(GxVpuProperty_LayerMainSurface));
	MainSurface.layer   = GX_LAYER_VPP;
	GxAVGetProperty(mvideo->out_dev, mvideo->vpu_mod, \
			GxVpuPropertyID_LayerMainSurface, (void*)&MainSurface, sizeof(GxVpuProperty_LayerMainSurface));
	if (MainSurface.surface == NULL) {
		GxVpuProperty_CreateSurface     CreateSurface;

		CreateSurface.width  = Resolution.xres;
		CreateSurface.height = Resolution.yres;
		CreateSurface.buffer = NULL;
		CreateSurface.format = GX_COLOR_FMT_YCBCR420;
		CreateSurface.mode   = GX_SURFACE_MODE_VIDEO;
		GxAVGetProperty(mvideo->out_dev, mvideo->vpu_mod, \
				GxVpuPropertyID_CreateSurface, (void*)&CreateSurface, sizeof(GxVpuProperty_CreateSurface));

		MainSurface.layer   = GX_LAYER_VPP;
		MainSurface.surface = CreateSurface.surface;
		GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod, \
				GxVpuPropertyID_LayerMainSurface, (void*)&MainSurface, sizeof(GxVpuProperty_LayerMainSurface));
	}

	return GX_MEDIAAPI_SUCCESS;
}

static void GxVideoOut_Close(GxMediaVideo* mvideo)
{
	if(mvideo->vpu_mod != -1)
		GxAvdev_CloseModule(mvideo->out_dev, mvideo->vpu_mod);

	if(mvideo->out_mod != -1)
		GxAvdev_CloseModule(mvideo->out_dev, mvideo->out_mod);

	if(mvideo->out_dev != -1)
		GxAvdev_DestroyDevice(mvideo->out_dev);

	mvideo->vpu_mod = -1;
	mvideo->out_mod = -1;
	mvideo->out_dev = -1;

	return;
}

static int GxVideoOut_Config(GxMediaVideo* mvideo)
{
	int ret = 0;
	GxVpuProperty_LayerViewport vp;

	vp.layer = GX_LAYER_VPP;
	GxMediaAPI_SetVideoDisply(vp.rect);

	ret = GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod, \
			GxVpuPropertyID_LayerViewport, (void*)&vp, sizeof(GxVpuProperty_LayerViewport));
	CHECK_RETURN_VALUE(ret);
	return 0;
}

static int GxVideoOut_SetWindow(GxMediaVideo* mvideo, GxMediaVideoWindow win)
{
	int ret = 0;
	GxVpuProperty_LayerViewport vp;

	vp.layer = GX_LAYER_VPP;
	vp.rect.x = win.x;
	vp.rect.y = win.y;
	vp.rect.width  = win.w;
	vp.rect.height = win.h;

	ret = GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod, \
			GxVpuPropertyID_LayerViewport, (void*)&vp, sizeof(GxVpuProperty_LayerViewport));
	CHECK_RETURN_VALUE(ret);

	return 0;
}

static int GxVideoOut_Start(GxMediaVideo* mvideo)
{
	int ret = 0;
	GxVpuProperty_LayerEnable layer_en;

	layer_en.layer = GX_LAYER_VPP;
	layer_en.enable = 1 ;
	ret = GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod, GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));
	CHECK_RETURN_VALUE(ret);
	return 0;
}

static int GxVideoOut_Stop(GxMediaVideo* mvideo, int freeze)
{
	GxVpuProperty_LayerEnable layer_en;

	layer_en.layer  = GX_LAYER_VPP;
	layer_en.enable = (freeze == 0)?0:1;
	GxAVSetProperty(mvideo->out_dev, mvideo->vpu_mod,
			GxVpuPropertyID_LayerEnable, &layer_en, sizeof(GxVpuProperty_LayerEnable));

	return GX_MEDIAAPI_SUCCESS;
}

GxMediaVideo* GxMedia_VideoOpen(GxMediaVideoMod mod)
{
	int ret = 0;
	GxMediaVideo* media_video = av_mallocz(sizeof(GxMediaVideo));

	if (media_video == NULL) return NULL;

	media_video->mdemux  = NULL;
	media_video->dec_dev = -1;
	media_video->dec_mod = -1;
	media_video->out_dev = -1;
	media_video->out_dev = -1;
	media_video->es_fifo = -1;
	media_video->valid = 1;
	media_video->mod_id = mod.vdec_modid;
	ret = GxVideoDec_Open(media_video, mod.vdec_modid);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxMediaAPI_debug("[%s %d]: video decoder open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	ret = GxVideoOut_Open(media_video, mod.vout_modid);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxMediaAPI_debug("[%s %d]: video out open failed\n", __FUNCTION__, __LINE__);
		goto open_error;
	}

	return media_video;

open_error:
	GxMedia_VideoClose(media_video);
	return NULL;
}

int GxMedia_VideoBindDemux(GxMediaVideo* media_video, GxMediaDemux *media_demux)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	media_video->mdemux = media_demux;

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoClose(GxMediaVideo* media_video)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	GxMedia_VideoStop(media_video, 0);
	GxVideoDec_Close(media_video);
	GxVideoOut_Close(media_video);
	media_video->valid = 0;
	av_free(media_video);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoConfig(GxMediaVideo* media_video, GxMediaVideoPara param)
{
	int ret = 0;

	CHECK_MEDIA_MOLUDE_VALID(media_video);

	media_video->params = param;
	ret = GxVideoDec_Config(media_video, &param.sync);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxVideoDec_Stop(media_video, 0);
		GxVideoOut_Stop(media_video, 0);
		GxMediaAPI_debug("[%s %d]: video decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxVideoOut_Config(media_video);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxVideoDec_Stop(media_video, 0);
		GxVideoOut_Stop(media_video, 0);
		GxMediaAPI_debug("[%s %d]: video out config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoSetWindow(GxMediaVideo* media_video, GxMediaVideoWindow win)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoOut_SetWindow(media_video, win);
}

int GxMedia_VideoStart(GxMediaVideo* media_video)
{
	int ret = 0;

	CHECK_MEDIA_MOLUDE_VALID(media_video);
	if (media_video->running == 1)
		return GX_MEDIAAPI_SUCCESS;

	ret = GxVideoDec_Start(media_video);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxVideoDec_Stop(media_video, 0);
		GxVideoOut_Stop(media_video, 0);
		GxMediaAPI_debug("[%s %d]: video decoder config failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	ret = GxVideoOut_Start(media_video);
	if (ret != GX_MEDIAAPI_SUCCESS) {
		GxVideoDec_Stop(media_video, 0);
		GxVideoOut_Stop(media_video, 0);
		GxMediaAPI_debug("[%s %d]: video decoder out failed\n", __FUNCTION__, __LINE__);
		return GX_MEDIAAPI_ERROR;
	}

	media_video->running = 1;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoStop(GxMediaVideo* media_video, int freeze)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	if (media_video->running == 0)
		return GX_MEDIAAPI_SUCCESS;

	GxVideoDec_Stop(media_video, freeze);
	GxVideoOut_Stop(media_video, freeze);

	media_video->running = 0;
	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoPause(GxMediaVideo* media_video)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	GxVideoDec_Pause(media_video);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoResume(GxMediaVideo* media_video)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	GxVideoDec_Resume(media_video);

	return GX_MEDIAAPI_SUCCESS;
}

int GxMedia_VideoInjectData(GxMediaVideo* media_video, GxMediaSourceType type, uint8_t* buffer, int len, int pts)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_InjectData(media_video, buffer, len, pts);
}

int GxMedia_VideoGetFifoInfo(GxMediaVideo* media_video, GxMediaModuleVideoFifo *info)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_GetEsvInfo(media_video, info);
}

int GxMedia_VideoGetDecState(GxMediaVideo* media_video, GxMediaDecState *state, GxMediaDecErrCode *err_code)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_GetDecState(media_video, state, err_code);
}

int GxMedia_VideoGetFrameInfo(GxMediaVideo* media_video, GxMediaVideoFrameInfo *info)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_GetFrameInfo(media_video, info);
}

int GxMedia_VideoGetPts(GxMediaVideo* media_video, int64_t* vpts)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_GetTime(media_video, vpts);
}

int GxMedia_VideoGetHash(GxMediaVideo* media_video, GxMediaVideoFrameHash *hash)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_GetHash(media_video, hash);
}

int GxMedia_VideoFinished(GxMediaVideo* media_video)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_Finished(media_video);
}

int GxMedia_VideoSpeed(GxMediaVideo* media_video, float speed)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_Speed(media_video, speed);
}

int GxMedia_VideoEos(GxMediaVideo* media_video)
{
	CHECK_MEDIA_MOLUDE_VALID(media_video);

	return GxVideoDec_Eos(media_video);
}
