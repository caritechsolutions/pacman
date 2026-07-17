#include "mini_gui_private.h"
#include "mini_gui.h"
#include "av/avapi.h"
#include "gxcore_os.h"
#include "tree.h"
#include "parser.h"
#ifdef GUI_TTF
#include "gal_ttf.h"
#endif
#include "gdi_font.h"

static void *progress_create(GuiWidget *self)
{
	void *object = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	WIDGET_ADJUST_ACCORDING_PARENT(self->parent, self);
	return (object);
}

static int progress_release(GuiWidget *self)
{
	int ret = 0;

	return (ret);
}

static int progress_draw(GuiWidget *self)
{
	int ret = 0, value = 0;
	char text_str[10] = {0};
	char *content = NULL;
	MiniGUI_Rect rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	rect.x = self->rect.x;
	rect.y = self->rect.y;
	rect.w = self->rect.w;
	rect.h = self->rect.h;

	minigui_fillrect(minigui_config.surface, &rect, self->back_color[0]);

	value = widget_get_int(self, "value", 0);
	if((0 <= value) && (value <= 100))
	{
		rect.w = rect.w * value / 100;
		minigui_fillrect(minigui_config.surface, &rect, self->back_color[1]);
	}

	rect.w = self->rect.w;

	content = (char *)widget_get_property(self, "string");
	if((content) && (0 == strcasecmp(content, "true")))
	{
		sprintf(text_str, "%02d", value);
		strcat(text_str, "%");
		minigui_draw_string(minigui_config.surface,
							text_str,
							&rect,
							self->fore_color[0],
							GUI_TA_VCENTRE | GUI_TA_HCENTRE);
	}

	return (ret);
}

GuiWidgetOps progress_ops = {
	.owner		=	"progress",
	.create		=	progress_create,
	.release	=	progress_release,
	.draw		=	progress_draw
};
