#include "header.h"
#include "all_widget.h"

static int _get_header_flag(GuiHeader *header)
{
	int ret = 0;
	char *format_str = NULL;

	if(NULL == header)
	{
		return (-1);
	}

	format_str = (char *)widget_get_property(header->base, "format");
	if(NULL == format_str)
	{
		return (-1);
	}

	if(0 == strcmp("headershow", format_str))
	{
		ret = HEADER_SHOW;
	}
	else if(0 == strcmp("headerhide", format_str))
	{
		ret = HEADER_HIDE;
	}

	return (ret);
}

static void *create_header(GuiWidget *self)
{
	GuiHeader *header = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (NULL);
	}

	header = (GuiHeader *)GUI_MALLOC(sizeof(GuiHeader));
	if(NULL == header)
	{
		return (NULL);
	}

	header->base = self;
	self->object = (void *)(header);
	self->state = 0;

	header->num_col = widget_get_int(self, "colum_number", 1);
	header->visible = _get_header_flag(header);
	//header->grid_width = widget_get_int(self, "grid_width", 0);

	child = self->first_child;

	while(child)
	{
		if(0 == strcasecmp("image", child->classname))
		{
			widget_create(child);
		}
		if(0 == strcasecmp("text", child->classname))
		{
			widget_create(child);
		}

		child = child->next_sibling;
	}


	return ((void *)header);
}

static int header_release(GuiWidget *self)
{
	GuiHeader *header = NULL;
	GuiWidget *child = NULL;

	if(NULL == self)
	{
		return (1);
	}

	header = (GuiHeader *)(self->object);
	if(NULL == header)
	{
		return (1);
	}

	child = self->first_child;
	while(child)
	{
		widget_release(child);
		child = child->next_sibling;
	}

	GUI_FREE(self->object);

	return (0);
}

static void _adjust_header_text(GuiWidget *self, GuiWidget *child)
{
	const char *value = NULL;

	value = widget_get_property(self, "arrange");
	if(NULL == value) {
		return;
	}

	if(0 == strcasecmp("left_to_right", value)) {
		return;
	}

	child->rect.x = (self->rect.x + self->rect.w) - (child->rect.x - self->rect.x) - child->rect.w;
}

static int header_draw(GuiWidget *self)
{
	GuiHeader *header = NULL;
	GuiWidget *child = NULL;
	int Index = 0, color = 0;
	GUI_Rect /*grid_rect = {0}, */valid_rect = {0};

	if(NULL == self)
	{
		return (1);
	}

	if(GUI_FALSE == self->need_update) {
		return (0);
	}

	header = (GuiHeader*)(self->object);
	if(NULL == header)
	{
		return (1);
	}

	Index = widget_get_state_index(self);
	if(HEADER_SHOW == header->visible)
	{
		color = self->back_color[Index];
		color = gal_color2index(screen, color);
		gal_fillrect(screen, (GAL_Rect *)&(self->rect), color);

		color = self->fore_color[Index];
		color = gal_color2index(screen, color);

		child = self->first_child;

		while(child)
		{
			valid_rect = child->rect;
			child->rect.x = valid_rect.x + self->rect.x;
			child->rect.y = valid_rect.y + self->rect.y;
			widget_mirror_rect(child);
			_adjust_header_text(self, child);
			if((0 == strcasecmp("image", child->classname)) ||
					(0 == strcasecmp("text", child->classname))
			  )
			{
				widget_draw(child);
			}

			child->rect = valid_rect;

			child = child->next_sibling;
		}
	}


	return (0);
}

static int header_event(GuiWidget *self, void *data)
{
	return (0);
}

static int header_set_property(GuiWidget *self, char *name, void *data)
{
	return (0);
}

static int _get_right2left_xpos(GuiWidget *self, int left, int right, int xpos, int width)
{
	const char *value = NULL;

	value = widget_get_property(self, "arrange");
	if(NULL == value) {
		return (xpos);
	}

	if(0 == strcasecmp("left_to_right", value)) {
		return (xpos);
	}

	xpos = right - (xpos - left) - width;

	return (xpos);
}

static int _get_alignment(GuiWidget *self, int alignment)
{
	const char *value = NULL;

	value = widget_get_property(self, "arrange");
	if(NULL == value) {
		return (alignment);
	}

	if(0 == strcasecmp("left_to_right", value)) {
		return (alignment);
	}

	if(GUI_TA_LEFT == (alignment & 0x3)) {
		alignment &= (~0x3);
		alignment |= GUI_TA_RIGHT;
	} else if(GUI_TA_RIGHT == (alignment & 0x3)) {
		alignment &= (~0x3);
		alignment |= GUI_TA_LEFT;
	}

	return (alignment);
}

static int header_get_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = GUI_GetCurrent();
	Header_Para *head_para = NULL;
	GuiWidget *child = NULL, *item = NULL;
	GuiText *text = NULL;
	GuiHeader *header = NULL;
	char *string = NULL;
	int i = 0;

	if(NULL == self || NULL == name)
	{
		return (1);
	}

	header = (GuiHeader *)(self->object);
	if(NULL == header)
	{
		return (1);
	}

	if(0 == strcasecmp("get_xoff", name))
	{
		child = widget_get_firstchild(self);
		head_para = (Header_Para*)data;
		while(child)
		{
			if(0 == strcasecmp("text", child->classname))
			{
				if(0 == i)
				{
					header->offset = child->rect.x;
				}

				if(head_para->index == i)
				{
					break;
				}
				else
				{
					i++;
				}
			}
			child = widget_get_nextsibling(child);
		}

		if(child)
		{
			head_para->xPos = child->rect.x/* - header->offset*/;
			if(gui_core->config.mirror) {
				item = widget_get_nextsibling(self);
				while(item) {
					if(0 == strcasecmp("listitem", item->classname)) {
						break;
					}
					item = widget_get_nextsibling(item);
				}
				if(item) {
					head_para->xPos = item->rect.w - child->rect.x - child->rect.w;
				}
			}
			head_para->xPos = _get_right2left_xpos(self, 0, self->rect.w, head_para->xPos, child->rect.w);
			head_para->xSize = child->rect.w;
			head_para->disable_mirror = child->disable_mirror;
		}
	}
	else if(0 == strcasecmp("alignment", name))
	{
		child = widget_get_firstchild(self);
		head_para = (Header_Para*)data;
		while(child)
		{
			if(0 == strcasecmp("text", child->classname))
			{
				if(head_para->index == i)
				{
					break;
				}
				else
				{
					i++;
				}
			}
			child = widget_get_nextsibling(child);
		}

		if(child)
		{
			text = (GuiText *)(child->object);
			if(text)
			{
				text->alignment = widget_get_alignment(child);
				head_para->alignment = _get_alignment(self, text->alignment);
			}
		}
	}
	else if(0 == strcasecmp("format", name))
	{
		child = widget_get_firstchild(self);
		head_para = (Header_Para*)data;
		i = 0;
		while(child)
		{
			if(0 == strcasecmp("text", child->classname))
			{
				if(head_para->index == i)
				{
					break;
				}
				else
				{
					i++;
				}
			}
			child = widget_get_nextsibling(child);
		}

		if(child)
		{
			string = (char *)widget_get_property(child, "format");
			if(0 == strcasecmp("fix_r2l", string))
			{
				head_para->automatic = GUI_TRUE;
			} else if(0 == strcasecmp("automatic", string)) {
				head_para->string_wrap = GUI_TRUE;
			}
		}
	} else {
		return (1);
	}

	return (0);
}

GuiWidgetOps header_ops =
{
	.owner = "header",
	.create = create_header,
	.release = header_release,
	.draw = header_draw,
	.event = header_event,
	.set = header_set_property,
	.get = header_get_property
};

