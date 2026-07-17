
#include "gx_mediafilter.h"

GxPin* GxPin_Create(const char *pin_name, void *obj, GxPinDirection dir, unsigned int fifo_size,int subflag)
{
	GxPin* pin;
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter == NULL)
		return NULL;

	pin = (GxPin* ) av_malloc(sizeof(GxPin));

	if (pin == NULL)
		return NULL;

	memset(pin, 0, sizeof(GxPin));

	GX_INIT_LIST_HEAD(&pin->head);

	pin->filter = filter;
	pin->link = NULL;
	pin->name = av_strdup(pin_name);
	pin->dir = dir;

	if (dir == GX_PINDIR_OUTPUT && fifo_size>0) {
		pin->fifo = GxFifo_Create(fifo_size,subflag);
		if (pin->fifo == NULL) {
			av_free(pin->name);
			av_free(pin);
			return NULL;
		}
		gxlist_add(&pin->head, &filter->out_pin.head);
	} else
		gxlist_add(&pin->head, &filter->in_pin.head);

	return pin;
}

void GxPin_Destroy(GxPin*  pin)
{
	if (pin) {
		if (pin->name)
			av_free(pin->name);
		if (pin->fifo)
			GxFifo_Destroy(pin->fifo);
		av_free(pin);
	}
}

void GxPin_Connect(GxPin*  pin, GxPin * out_pin)
{
	if (pin && out_pin && pin->dir == GX_PINDIR_OUTPUT && out_pin->dir == GX_PINDIR_INPUT) {
		pin->link = out_pin;
		out_pin->source = pin;
	}
}

void GxPin_Disconnect(GxPin*  pin)
{
	if (pin->link) {
		pin->link->source = NULL;
		pin->link = NULL;
	}
}

int GxPin_Write(GxPin*  pin, void *data, size_t size)
{
	if (pin)
		return GxFifo_Write(pin->fifo, data, size,-1);
	return 0;
}

int GxPin_Read(GxPin*  pin, void *data, size_t size)
{
	if (pin)
		return GxFifo_Read(pin->fifo, data, size,-1);
	return 0;
}

int GxPin_Peek(GxPin*  pin, void *data, size_t size)
{
	if (pin)
		return GxFifo_Peek(pin->fifo, data, size);
	return 0;
}

void GxMediaFilter_Init(void)
{
	GxClassRegister(&gx_mediafilter_base);
}

void GxMediaFilter_Destroy(void)
{
	GxClassDestroy();
}

GxPin* GxMediaFilter_FindPin(void *obj, const char *pin_name)
{
	GxPin* pin;
	struct gxlist_head* pos;
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	gxlist_for_each(pos, &filter->in_pin.head) {
		pin = gxlist_entry(pos, GxPin, head);

		if (strcmp(pin->name, pin_name) == 0)
			return pin;
	}

	gxlist_for_each(pos, &filter->out_pin.head) {
		pin = gxlist_entry(pos, GxPin, head);

		if (strcmp(pin->name, pin_name) == 0)
			return pin;
	}

	return NULL;
}

int GxMediaFilter_Run(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);
		
		filter->status = GX_MFT_STATE_RUNNING;
		if (mfc->run)
			return mfc->run(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->run)
				return mfc->run(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Pause(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter && filter->status == GX_MFT_STATE_RUNNING) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		filter->status = GX_MFT_STATE_PAUSED;
		if (mfc->pause)
			return mfc->pause(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->pause)
				return mfc->pause(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Resume(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter)// && filter->status == GX_MFT_STATE_PAUSED)
	{
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		filter->status = GX_MFT_STATE_RUNNING;
		if (mfc->resume)
			return mfc->resume(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->resume)
				return mfc->resume(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Stop(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter)//&& filter->status != GX_MFT_STATE_STOPPED) 
	{
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		filter->status = GX_MFT_STATE_STOPPED;
		if (mfc->stop)
			return mfc->stop(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->stop)
				return mfc->stop(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Config(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		if (mfc->config)
			return mfc->config(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->config)
				return mfc->config(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Abort(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		if (mfc->abort)
			return mfc->abort(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->abort)
				return mfc->abort(filter);
		}
	}

	return 0;
}


int GxMediaFilter_Wait(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		if (mfc->wait)
			return mfc->wait(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->wait)
				return mfc->wait(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Reset(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		GxMediaFilterClass* mfc = GetGxMediaFilterClassFromObject(filter);

		if (mfc->reset)
			return mfc->reset(filter);
		else {
			mfc = GXMEDIAFILTERCLASS((GetGxClassFromObject(obj))->parent);
			if (mfc->reset)
				return mfc->reset(filter);
		}
	}

	return 0;
}

int GxMediaFilter_Connect(void* source, const char *source_pin_name, void *link, const char *link_pin_name)
{
	GxPin* spin;
	GxPin* lpin;

	if (source && link) {
		spin = GxMediaFilter_FindPin(source, source_pin_name);
		lpin = GxMediaFilter_FindPin(link, link_pin_name);
		if (spin && lpin)
			GxPin_Connect(spin, lpin);
	}

	return 0;
}

int GxMediaFilter_Disconnect(void* source, const char *source_pin_name, void *link, const char *link_pin_name)
{
	GxPin* spin;
	GxPin* lpin;

	if (source && link) {
		spin = GxMediaFilter_FindPin(source, source_pin_name);
		lpin = GxMediaFilter_FindPin(link, link_pin_name);
		if (spin && lpin)
			GxPin_Disconnect(spin);
	}

	return 0;
}

GxMediaFilterStatus GxMediaFilter_Status(void* obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		return filter->status;
	}

	return 0;
}

void GxMediaFilter_SetEvent(void* obj, void (*func)(void* priv, void* args), void* priv)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter) {
		filter->event.priv = priv;
		filter->event.func = func;
	}
}

static void* mediafilter_create(GxObject * obj)
{
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	filter->status = GX_MFT_STATE_STOPPED;
	filter->type = GX_MFT_TYPE_UNKNOWN;
	GX_INIT_LIST_HEAD(&(filter->in_pin.head));
	GX_INIT_LIST_HEAD(&(filter->out_pin.head));

	return filter;
}

static void mediafilter_release(GxObject*  obj)
{
	GxPin* pin;
	struct gxlist_head* pos, *n, *pin_head;
	GxMediaFilter* filter = GXMEDIAFILTER(obj);

	if (filter == NULL)
		return;

	pin_head = &filter->out_pin.head;
	gxlist_for_each_safe(pos, n, pin_head) {
		pin = gxlist_entry(pos, GxPin, head);
		gxlist_del(pos);
		GxPin_Destroy(pin);
	}

	pin_head = &filter->in_pin.head;
	gxlist_for_each_safe(pos, n, pin_head) {
		pin = gxlist_entry(pos, GxPin, head);
		gxlist_del(pos);
		GxPin_Destroy(pin);
	}
}

GxMediaFilterClass gx_mediafilter_base = {
	._inherit = {
		.name    = "MediaFilterBase",
		.parent  = &gxRootClass,
		.size    = sizeof(GxMediaFilter),
		.create  = mediafilter_create,
		.release = mediafilter_release,
	}
};


