#include "gxtype.h"
#include "gx_subtitle_show_text.h"
#include "subtitle_config.h"

static GxSubtitleShowTextOps *registerOps = NULL;

static GxSubtitleShowTextOps *find_ops(void)
{
	GxSubtitleShowTextOps *ops = NULL;

#ifdef CONFIG_SUBTITLE_GDI_DRAW
	extern GxSubtitleShowTextOps gdiTextOps;
	ops = &gdiTextOps;
#endif

	if (registerOps)
		ops = registerOps;

	return ops;
}

void GxSubtitleShowTextRegister(GxSubtitleShowTextOps *ops)
{
	registerOps = ops;
}

void GxSubtitleShowTextUnRegister(void)
{
	registerOps= NULL;
}

handle_t GxSubtitleShowTextOpen(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->open)
		return ops->open(cv, sr);

	return -1;
}

void GxSubtitleShowTextClose(handle_t handle)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->close)
		ops->close(handle);
}

int GxSubtitleShowTextDraw(handle_t handle, GxSubtitleShowTextPage *txt)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->draw)
		return ops->draw(handle, txt);

	return -1;
}

int GxSubtitleShowTextClean(handle_t handle)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->clean)
		return ops->clean(handle);

	return -1;
}

int GxSubtitleShowTextSetScreenColor(handle_t handle, unsigned int color)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->setScreenColor)
		return ops->setScreenColor(handle, color);

	return -1;
}

int GxSubtitleShowTextSetDisplayPos(handle_t handle, GxSubtitleShowDisplay *pos)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->setDisplayPos)
		return ops->setDisplayPos(handle, pos);

	return -1;
}

int GxSubtitleShowTextSetFont(handle_t handle, GxSubtitleShowFont *font)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->setFont)
		return ops->setFont(handle, font);

	return -1;
}

int GxSubtitleShowTextShow(void)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->show)
		return ops->show();

	return -1;
}

int GxSubtitleShowTextHide(void)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->hide)
		return ops->hide();

	return -1;
}

int GxSubtitleShowTextScreen(GxSubtitleShowScreen *screen)
{
	GxSubtitleShowTextOps *ops = find_ops();

	if (ops && ops->setScreen)
		return ops->setScreen(screen);

	return -1;
}

