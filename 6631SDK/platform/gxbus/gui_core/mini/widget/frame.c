#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

static void *frame_create(GuiWidget *self)
{
	void *object = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (object);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_create(child);
		child = widget_get_nextsibling(child);
	}

	return (object);
}

static int frame_release(GuiWidget *self)
{
	int ret = 0;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_release(child);
		child = widget_get_nextsibling(child);
	}

	return (ret);
}

static int frame_draw(GuiWidget *self)
{
	int ret = 0;
	GuiWidget *child = NULL;
	MiniGUI_Rect rect = {0};
	char *logo = NULL;

	if(NULL == self)
	{
		return (1);
	}

	logo = (char *)widget_get_property(self, "logo");
	if(logo)
	{
		minigui_show_logo(logo, NULL);
	}

	rect.x = self->rect.x;
	rect.y = self->rect.y;
	rect.w = self->rect.w;
	rect.h = self->rect.h;

	minigui_fillrect(minigui_config.surface, &rect, self->back_color[0]);

	child = widget_get_firstchild(self);
	while(child)
	{
		widget_draw(child);
		child = widget_get_nextsibling(child);
	}

	return (ret);
}

GuiWidgetOps frame_ops = {
	.owner		=	"frame",
	.create		=	frame_create,
	.release	=	frame_release,
	.draw		=	frame_draw
};

