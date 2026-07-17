#include "gx_mediafilter.h"

extern GxFifoClass gx_fifo_sdc;
extern GxFifoClass gx_fifo_sw;
extern GxFifoClass gx_fifo_muxts;
extern GxFifoClass gx_fifo_muxer;
extern GxFifoClass gx_fifo_hwts;

GxFifo *GxFifo_Create(size_t size, int subflag)
{
	GxFifo *fifo;
	GxFifoClass* cls;

	switch(subflag)
	{
		case GX_PINFLAG_SW:
			cls = &gx_fifo_sw;
			break;
		case GX_PINFLAG_MUXTS:
			cls = &gx_fifo_muxts;
			break;
		case GX_PINFLAG_MUXER:
			cls = &gx_fifo_muxer;
			break;
		case GX_PINFLAG_TSA:
			cls = &gx_fifo_hwts;
			break;
		default:
			cls = &gx_fifo_sdc;
			break;
	}

	fifo = cls->create(size, subflag);
	if (fifo){
		fifo->size = size;
		fifo->cls = cls;
	}

	return fifo;
};

int GxFifo_Destroy(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->destroy)
		return cls->destroy(fifo);
	else
		return 0;
};

int GxFifo_Config(GxFifo *fifo, GxFifoConfig * configure)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->config)
		return cls->config(fifo, (void* )configure);
	else
		return 0;
};

size_t GxFifo_Write(GxFifo *fifo, unsigned char *buffer, unsigned int len,int timeout_us)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->write)
		return cls->write(fifo, buffer, len, timeout_us);
	else
		return 0;
};

size_t GxFifo_WriteWithPts(GxFifo *fifo, unsigned char *buffer, unsigned int len, int pts, int timeout_us)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->write)
		return cls->writepts(fifo, buffer, len, pts, timeout_us);
	else
		return 0;
};

size_t GxFifo_Read(GxFifo *fifo, unsigned char *buffer, unsigned int len, int timeout_us)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(fifo->extdata){
		memcpy(buffer, fifo->extdata, fifo->extsize);
		av_free(fifo->extdata);
		fifo->extdata = NULL;
		return fifo->extsize;
	}

	if(cls && cls->read)
		return cls->read(fifo, buffer, len, timeout_us);

	else
		return 0;
};

size_t GxFifo_ReadWithPts(GxFifo *fifo, unsigned char *buffer, unsigned int len, int *pts, int timeout_us)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(fifo->extdata){
		memcpy(buffer, fifo->extdata, fifo->extsize);
		av_free(fifo->extdata);
		fifo->extdata = NULL;
		*pts = -1;
		return fifo->extsize;
	}

	if(cls && cls->readpts)
		return cls->readpts(fifo, buffer, len, pts, timeout_us);

	else
		return 0;
};

size_t GxFifo_Peek(GxFifo *fifo, unsigned char *buffer, unsigned int len)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->peek)
		return cls->peek(fifo, buffer, len);
	else
		return 0;
}

size_t GxFifo_Skip(GxFifo *fifo, unsigned int len)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->skip)
		return cls->skip(fifo, len);
	else
		return 0;
}

size_t GxFifo_Rollback(GxFifo *fifo, int len)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->rollback)
		return cls->rollback(fifo, len);
	else
		return 0;
}

size_t GxFifo_GetLength(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_len)
		return cls->get_len(fifo);
	else
		return 0;
};

size_t GxFifo_GetFreeSize(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_free)
		return cls->get_free(fifo);
	else
		return 0;
};

size_t GxFifo_GetPtsLength(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_pts_len)
		return cls->get_pts_len(fifo);
	else
		return 0;
};

size_t GxFifo_GetPtsFreeSize(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_pts_free)
		return cls->get_pts_free(fifo);
	else
		return 0;
};

size_t GxFifo_GetCap(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_cap)
		return cls->get_cap(fifo);
	else
		return fifo->size;
};

size_t GxFifo_GetPtsCap(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->get_pts_cap)
		return cls->get_pts_cap(fifo);
	else
		return 0;
};

int GxFifo_Reset(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->reset)
		return cls->reset(fifo);
	else
		return 0;
};

int GxFifo_PtsIsFull(GxFifo *fifo)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->pts_isfull)
		return cls->pts_isfull(fifo);
	else
		return 0;
};

int GxFifo_Link(GxFifo *fifo, GxFifoLink* configure)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->link)
		return cls->link(fifo, (void* )configure);
	else
		return 0;
};

int GxFifo_Unlink(GxFifo *fifo, GxFifoLink* configure)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->unlink)
		return cls->unlink(fifo, (void* )configure);
	else
		return 0;
};

size_t GxFifo_ShallowRead(GxFifo *fifo, unsigned char **start, unsigned int len, int timeout_us)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->shallow_read)
		return cls->shallow_read(fifo, start, len, timeout_us);
	else
		return 0;
}

int GxFifo_ShallowWrite(GxFifo *fifo, unsigned char *start,  unsigned int len)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->shallow_write)
		return cls->shallow_write(fifo, start, len);
	else
		return 0;
};


int GxFifo_GetShallowInfo(GxFifo *fifo, GxFifoShallowInfo *info)
{
	GxFifoClass* cls = (fifo && fifo->cls) ? fifo->cls : NULL;

	if(cls && cls->shallow_info)
		return cls->shallow_info(fifo, info);
	else
		return 0;
}
