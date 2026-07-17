/*
 * @file        gxsubtitle.c
 * @brief
 * @version     1.1.0
 * @date        04/02/2010 08:25:26 AM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gxcore.h"
#include "gx_mem.h"
#include "subtitle/gxsubtitle.h"
#include "subtitle/sub_def.h"
#include "sub_debug.h"
#include "sub_render.h"
#include "sub_timer.h"
#include "sub_dvb.h"
#include "com_subtitle_porting.h"

#define SUB_RENDER_OPEN(ptr, rect, rect_max) ((ptr)->render.handle = (ptr)->render.ops->open(rect, rect_max))
#define SUB_RENDER_CLOSE(ptr)                ((ptr)->render.ops->close((ptr)->render.handle))
#define SUB_RENDER_SHOW(ptr)                 ((ptr)->render.ops->show((ptr)->render.handle))
#define SUB_RENDER_HIDE(ptr)                 ((ptr)->render.ops->hide(ptr->render.handle))
#define SUB_RENDER_CLEAR(ptr)                ((ptr)->render.ops->clear(ptr->render.handle))
#define SUB_RENDER_SET_NTSC(ptr, flag)       ((ptr)->render.ops->set_ntsc(ptr->render.handle, flag))
#define SUB_DECODER_OPEN(ptr)                ((ptr)->decoder.ops->open(&(ptr)->render, &(ptr)->timer))
#define SUB_DECODER_CLOSE(ptr)               ((ptr)->decoder.ops->close((ptr)->decoder.handle))
#define SUB_DECODER_DECODING(ptr, pkt, len)  ((ptr)->decoder.ops->decode((ptr)->decoder.handle, pkt, len))
#define SUB_DECODER_RENDERING(ptr)           ((ptr)->decoder.ops->render((ptr)->decoder.handle))
#define SUB_DECODER_SYNC(ptr, t)             ((ptr)->decoder.ops->synchronize(ptr->decoder.handle, (t)))
#define SUB_DECODER_START(ptr)               ((ptr)->decoder.ops->start(ptr->decoder.handle))
#define SUB_DECODER_STOP(ptr)                ((ptr)->decoder.ops->stop(ptr->decoder.handle))
#define SUB_TIMER_TIMING(ptr)                ((ptr)->timer.ops->exec())

#define SUB_DECODER_SET_STREAM(ptr,comp_page_id, anci_page_id) \
	(ptr->decoder.ops->set_stream(ptr->decoder.handle, comp_page_id, anci_page_id))

static void subtitle_rendering(void* arg)
{
	GxSubtitle_Info*    sub = (GxSubtitle_Info*)arg;
	while((sub != NULL) && (sub->decoder.ops->render != NULL) && sub->rendering) {
		SUB_DECODER_RENDERING(sub);
	}
}

static void subtitle_timing(void* arg)
{
	GxSubtitle_Info*    sub = (GxSubtitle_Info*)arg;

	while((sub != NULL) && (sub->timing)) {
		SUB_TIMER_TIMING(sub);
		sub->timer.ops->delayms(60);
	}
}

handle_t GxSubtitle_Open(GxSubtitle_Param* param)
{
	GxSubtitle_Info*        sub;
	GxSubtitle_RenderRect   rect     = {0, 0,  720, 576};
	GxSubtitle_RenderRect   rect_max = {0, 0, 1280, 720};

	if ((param->render_rect.width != 0) && (param->render_rect.height != 0))
		memcpy(&rect, &param->render_rect, sizeof(GxSubtitle_RenderRect));

	if ((param->render_rect_max.width != 0) && (param->render_rect_max.height != 0))
		memcpy(&rect_max, &param->render_rect_max, sizeof(GxSubtitle_RenderRect));

	if ((rect.width > rect_max.width) || (rect.height > rect_max.height))
		memcpy(&rect, &rect_max, sizeof(GxSubtitle_RenderRect));

	DBG_SUB("%s\n", __FUNCTION__);

	if (param == NULL) {
		return E_INVALID_HANDLE;
	}

	sub = (GxSubtitle_Info* )CALLOC(1, sizeof(GxSubtitle_Info));
	if (sub == NULL) {
		return E_INVALID_HANDLE;
	}

	if (param->render_type != GXSUBTITLE_SPP) {
		return E_INVALID_HANDLE;
	}

	sub->render.ops     = &gxsub_spp_render;
	sub->decoder.ops    = &com_subtitle_ops;
	sub->timer.ops      = &gxsub_timer;

	SUB_RENDER_OPEN(sub, &rect, &rect_max);
	if (sub->render.handle == E_INVALID_HANDLE) {
		FREE(sub);
		return E_INVALID_HANDLE;
	}

	sub->decoder.handle = SUB_DECODER_OPEN(sub);
	if (sub->decoder.handle == E_INVALID_HANDLE) {
		SUB_RENDER_CLOSE(sub);
		FREE(sub);
		return E_INVALID_HANDLE;
	}

	DBG_SUB("%s\n", __FUNCTION__);
	return (handle_t)sub;
}

int32_t GxSubtitle_Synchronize(handle_t handle, int32_t milliseconds)
{
	DBG_SUB("%s\n", __FUNCTION__);
	return E_OK;
}

int32_t GxSubtitle_StreamSet(handle_t handle, GxSubtitle_Stream* stream)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);
	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	SUB_DECODER_SET_STREAM(sub, stream->comp_page_id, stream->anci_page_id);

	return E_OK;
}

int32_t GxSubtitle_Close(handle_t handle)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);
	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	if (sub->decoding) {
		GxSubtitle_Stop(handle, 0);
	}

	SUB_RENDER_CLOSE(sub);
	SUB_DECODER_CLOSE(sub);

	FREE(sub);
	DBG_SUB("%s\n", __FUNCTION__);
	return E_OK;
}

int32_t GxSubtitle_Start(handle_t handle)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	sub->decoding  = TRUE;
	sub->rendering = TRUE;
	sub->timing = TRUE;

	SUB_DECODER_START(sub);

	GxCore_ThreadCreate("gxsubtitle", &sub->render_thread,
			subtitle_rendering, sub, 32*1024, GXOS_DEFAULT_PRIORITY-1);

	SUB_RENDER_CLEAR(sub);
	SUB_RENDER_SHOW(sub);

	GxCore_ThreadCreate("gxsubtitle", &sub->timer_thread,
			subtitle_timing, sub, 8*1024, GXOS_DEFAULT_PRIORITY-1);

	DBG_SUB("%s\n", __FUNCTION__);
	return E_OK;
}

int32_t GxSubtitle_Stop(handle_t handle, int freeze)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}
	sub->decoding  = FALSE;
	sub->rendering = FALSE;
	sub->timing = FALSE;

	if (freeze == 0) {
		SUB_RENDER_CLEAR(sub);
		SUB_RENDER_HIDE(sub);
	}

	SUB_DECODER_STOP(sub);
	GxCore_ThreadJoin(sub->render_thread);
	GxCore_ThreadJoin(sub->timer_thread);
	DBG_SUB("%s\n", __FUNCTION__);
	return E_OK;
}

int32_t GxSubtitle_Clear(handle_t handle)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}
	SUB_RENDER_CLEAR(sub);
	return E_OK;
}

int32_t GxSubtitle_Show(handle_t handle)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	SUB_RENDER_SHOW(sub);
	return E_OK;
}

int32_t GxSubtitle_SetNtsc(handle_t handle, bool flag)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	SUB_RENDER_SET_NTSC(sub, flag);
	return E_OK;
}

int32_t GxSubtitle_Hide(handle_t handle)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	DBG_SUB("%s\n", __FUNCTION__);

	if (handle == E_INVALID_HANDLE) {
		return E_FAILURE;
	}

	SUB_RENDER_HIDE(sub);
	return E_OK;
}

int32_t GxSubtitle_SendPesData(handle_t handle, uint8_t* packet, int packet_length)
{
	GxSubtitle_Info* sub = (GxSubtitle_Info*)handle;

	if ((sub != NULL) && sub->decoding) {
		SUB_DECODER_DECODING(sub, packet, packet_length);
		return E_OK;
	}

	return E_FAILURE;
}
