#include "gxcore.h"
#include "assert.h"
#include "gxaribcc.h"
#include "aribcc.h"
#include "module/app_log.h"

#if ARIBCC_SUPPORT
#define SUB_DEMUX_OPEN(ptr, id,pid)             ((ptr)->demux.handle = (ptr)->demux.ops->open((id),(pid)))
#define SUB_DEMUX_CLOSE(ptr)                ((ptr)->demux.ops->close((ptr)->demux.handle))
#define SUB_DEMUX_START(ptr)                ((ptr)->demux.ops->start((ptr)->demux.handle))
#define SUB_DEMUX_STOP(ptr)                 ((ptr)->demux.ops->stop((ptr)->demux.handle))

#define SUB_RENDER_OPEN(ptr, rect,surface,buffer)          ((ptr)->render.handle = (ptr)->render.ops->open((rect),(surface),(buffer)))
#define SUB_RENDER_REGISTER_CB(ptr,cb)                ((ptr)->render.ops->registercb((ptr)->render.handle,(cb)))
#define SUB_RENDER_CLOSE(ptr)               ((ptr)->render.ops->close((ptr)->render.handle))
#define SUB_RENDER_SHOW(ptr)                ((ptr)->render.ops->show((ptr)->render.handle))
#define SUB_RENDER_HIDE(ptr)                ((ptr)->render.ops->hide(ptr->render.handle))

#define SUB_DECODER_OPEN(ptr)               ((ptr)->decoder.ops->open(&(ptr)->demux, &(ptr)->render, &(ptr)->timer))
#define SUB_DECODER_CLOSE(ptr)              ((ptr)->decoder.ops->close((ptr)->decoder.handle))
#define SUB_DECODER_DECODING(ptr)           ((ptr)->decoder.ops->decode((ptr)->decoder.handle))
#define SUB_DECODER_START(ptr)              ((ptr)->decoder.ops->start(ptr->decoder.handle))
#define SUB_DECODER_STOP(ptr)               ((ptr)->decoder.ops->stop(ptr->decoder.handle))
#define SUB_DECODER_SET_STREAM(ptr,comp_page_id, anci_page_id) \
	(ptr->decoder.ops->set_stream(ptr->decoder.handle, comp_page_id, anci_page_id))
#define SUB_DECODER_SWITCH_LANG(ptr,langid) (ptr->decoder.ops->switch_lang(ptr->decoder.handle, (langid)))

#define SUB_TIMER_START(ptr)              ((ptr)->timer.ops->start())
#define SUB_TIMER_STOP(ptr)              ((ptr)->timer.ops->stop())
#define SUB_TIMER_TIMING(ptr)               ((ptr)->timer.ops->exec())
#define SUB_TIMER_DELAY(ptr,ms)               ((ptr)->timer.ops->delay(ms))

static void aribcc_decoding(void* arg)
{
	GxAribCC_Info*    sub = (GxAribCC_Info*)arg;

	while((sub != NULL) && (sub->decoding)) {
		SUB_DECODER_DECODING(sub);
	}
}

static void aribcc_timing(void* arg)
{
	GxAribCC_Info*    sub = (GxAribCC_Info*)arg;

	while((sub != NULL) && (sub->timing)) {
		SUB_TIMER_TIMING(sub);
	}
}

handle_t GxAribCC_Open(GxAribCC_Param* param)
{
	GxAribCC_Info*        sub;
	GxAribCC_RenderRect   rect = {0, 0, 720, 576};

	if (param == NULL) {
		return E_INVALID_HANDLE;
	}

	sub = CALLOC(1, sizeof(GxAribCC_Info));
	if (sub == NULL) {
		return E_INVALID_HANDLE;
	}
	memset(sub,0,sizeof(GxAribCC_Info));

	sub->demux.demux = param->demux_id;

	if(param->render_type == GXARIBCC_OSD)
		sub->render.ops = &aribcc_render_osd;
	else
		sub->render.ops = &aribcc_render_spp;
	sub->demux.ops = &aribcc_demux;
	sub->decoder.ops = &aribcc_decoder;
	sub->timer.ops = &aribcc_timer;

	rect.x = 0;
	rect.y = 0;
	rect.width  = param->render_size.width;
	rect.heigth = param->render_size.height;
	SUB_RENDER_OPEN(sub, &rect,
		param->render_size.surface,
		param->render_size.buffer);
	if (sub->render.handle == E_INVALID_HANDLE)
		FREE(sub);

	SUB_RENDER_REGISTER_CB(sub,param->render_cb);

	sub->decoder.handle = SUB_DECODER_OPEN(sub);
	if (sub->decoder.handle == E_INVALID_HANDLE) {
		SUB_RENDER_CLOSE(sub);
		FREE(sub);
	}

	return (handle_t)sub;
}

int32_t GxAribCC_Close(handle_t handle)
{
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	if (sub->decoding)
		GxAribCC_Stop(handle);

	SUB_DEMUX_CLOSE(sub);
	SUB_RENDER_CLOSE(sub);
	SUB_DECODER_CLOSE(sub);

	FREE(sub);
	return E_OK;
}

int32_t GxAribCC_StreamSet(handle_t handle, GxAribCC_Stream* stream)
{
	int32_t ret;
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE || stream == NULL)
		return E_FAILURE;

	printf("%s pid=%04x, comp_page_id=%d,anci_page_id=%d\n",
			__FUNCTION__,stream->pid, stream->comp_page_id, stream->anci_page_id);

	if(sub->demux.handle) {
		SUB_DEMUX_CLOSE(sub);
		sub->demux.handle = E_INVALID_HANDLE;
	}

	SUB_DEMUX_OPEN(sub, sub->demux.demux,stream->pid);

	if (sub->decoding)
		GxAribCC_Stop(handle);

	ret = SUB_DECODER_SET_STREAM(sub, stream->comp_page_id, stream->anci_page_id);
    if(ret != E_OK)
        app_log_error("aribcc SUB_DECODER_SET_STREAM failed!!!!!!!\n");

	ASSERT (ret == E_OK);

	if (!sub->decoding)
		GxAribCC_Start(handle);

	return E_OK;
}

int32_t GxAribCC_SwitchLang(handle_t handle, GxAribCC_LangID langid)
{
	int32_t ret;
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	ret = SUB_DECODER_SWITCH_LANG(sub, langid);

	return !ret ? E_OK : E_FAILURE;
}

int32_t GxAribCC_Start(handle_t handle)
{
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	sub->decoding  = TRUE;
	sub->timing = TRUE;

	SUB_DECODER_START(sub);
	GxCore_ThreadCreate("gxaribcc", &sub->decode_thread,
			aribcc_decoding, sub, 50*1024, GXOS_DEFAULT_PRIORITY+1);

	SUB_TIMER_START(sub);
	GxCore_ThreadCreate("gxaribcc", &sub->timer_thread, aribcc_timing, sub, 50*1024, GXOS_DEFAULT_PRIORITY+1);

	SUB_DEMUX_START(sub);

	return E_OK;
}

int32_t GxAribCC_Stop(handle_t handle)
{
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	SUB_DEMUX_STOP(sub);

	sub->decoding  = FALSE;
	sub->timing = FALSE;
	SUB_RENDER_HIDE(sub);

	SUB_DECODER_STOP(sub);
	SUB_TIMER_STOP(sub);
	GxCore_ThreadJoin(sub->decode_thread);
	GxCore_ThreadJoin(sub->timer_thread);

	return E_OK;
}

int32_t GxAribCC_Show(handle_t handle)
{
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	SUB_RENDER_SHOW(sub);

	return E_OK;
}

int32_t GxAribCC_Hide(handle_t handle)
{
	GxAribCC_Info* sub = (GxAribCC_Info*)handle;

	if (handle == E_INVALID_HANDLE)
		return E_FAILURE;

	SUB_RENDER_HIDE(sub);

	return E_OK;
}
#endif

