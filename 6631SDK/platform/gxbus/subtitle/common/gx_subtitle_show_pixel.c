#include "gxtype.h"
#include "gx_subtitle_show_pixel.h"
#include "subtitle_config.h"

static GxSubtitleShowPixelOps *registerOps = NULL;

static GxSubtitleShowPixelOps *find_ops(void)
{
	GxSubtitleShowPixelOps *ops = NULL;

#ifdef CONFIG_SUBTITLE_GDI_DRAW
	extern GxSubtitleShowPixelOps gdiPixelOps;
	ops = &gdiPixelOps;
#endif

	if (registerOps)
		ops = registerOps;

	return ops;
}

void GxSubtitleShowPixelRegister(GxSubtitleShowPixelOps *ops)
{
	registerOps = ops;
}

void GxSubtitleShowPixelUnRegister(void)
{
	registerOps = NULL;
}

handle_t GxSubtitleShowPixelOpen(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->open)
		return ops->open(cv, sr);

	return -1;
}

void GxSubtitleShowPixelClose(handle_t handle)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->close)
		ops->close(handle);
}

int GxSubtitleShowPixelSetDisplayPos(handle_t handle, GxSubtitleShowDisplay *pos)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->setDisplayPos)
		return ops->setDisplayPos(handle, pos);

	return -1;
}

int GxSubtitleShowPixelSetScreenColor(handle_t handle, unsigned int color)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->setScreenColor)
		return ops->setScreenColor(handle, color);

	return -1;
}

int GxSubtitleShowPixelDraw(handle_t handle, GxSubtitleShowPixelPage *page)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->draw)
		return ops->draw(handle, page);

	return -1;
}

int GxSubtitleShowPixelClean(handle_t handle)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->clean)
		return ops->clean(handle);

	return -1;
}

int GxSubtitleShowPixelShow(void)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->show)
		return ops->show();

	return -1;
}

int GxSubtitleShowPixelHide(void)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->hide)
		return ops->hide();

	return -1;
}

int GxSubtitleShowPixelSetScreen(GxSubtitleShowScreen *screen)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->setScreen)
		return ops->setScreen(screen);

	return -1;
}

unsigned int *GxSubtitleShowPixelAllocARGB888Buffer(handle_t handle, unsigned int width, unsigned int height)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->allocARGB8888Buffer)
		return ops->allocARGB8888Buffer(handle, width, height);

	return NULL;
}

int GxSubtitleShowPixelFreeARGB8888Bufeer(handle_t handle, unsigned int *buffer)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->freeARGB8888Buffer)
		return ops->freeARGB8888Buffer(handle, buffer);

	return -1;
}

int GxSubtitleShowPixelDrawARGB8888Buffer(handle_t handle, unsigned int *buffer, int full_screen)
{
	GxSubtitleShowPixelOps *ops = find_ops();

	if (ops && ops->drawARGB8888Buffer)
		return ops->drawARGB8888Buffer(handle, buffer, full_screen);

	return -1;
}
