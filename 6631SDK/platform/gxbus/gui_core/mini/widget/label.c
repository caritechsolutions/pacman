#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif

static void *label_create(GuiWidget *self)
{
	void *object = NULL;

	WIDGET_ADJUST_ACCORDING_PARENT(self->parent, self);
	return (object);
}

static int label_release(GuiWidget *self)
{
	int ret = 0;

	return (ret);
}

static int label_draw(GuiWidget *self)
{
	int ret = 0;
	char *value = NULL;
	MiniGUI_Rect rect = {0};
	image_desc *desc = NULL;
	int alignment = 0;

	if(NULL == self)
	{
		return (1);
	}

	value = (char *)widget_get_property(self, "img");
	if(value)
	{
		desc = hash_get(minigui_config.image, value);
	}
	if(desc)
	{
		minigui_draw_image(minigui_config.surface,
						   desc,
						   self->rect.x,
						   self->rect.y,
						   self->rect.w,
						   self->rect.h);
	}
	else
	{
		rect.x = self->rect.x;
		rect.y = self->rect.y;
		rect.w = self->rect.w;
		rect.h = self->rect.h;

		minigui_fillrect(minigui_config.surface, &rect, self->back_color[0]);
	}

	value = (char *)widget_get_property(self, "string");
	if(value)
	{
		alignment = widget_get_alignment(self);
		minigui_draw_string(minigui_config.surface,
						    value,
							&rect,
							self->fore_color[0],
							alignment);
	}

	return (ret);
}

GuiWidgetOps label_ops = {
	.owner		=	"label",
	.create		=	label_create,
	.release	=	label_release,
	.draw		=	label_draw
};

