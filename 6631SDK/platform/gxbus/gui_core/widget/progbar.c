#include "all_widget.h"

static int _get_progbar_format(GuiProgbar *progbar)
{
	char *pvalue = NULL;
	int ret = 0;

	if(NULL == progbar)
		return (1);

	pvalue = (char *)widget_get_property(progbar->base, "format");

	if (pvalue == NULL)
		return 1;

	if(0 == strcasecmp("normal", pvalue))
		ret = PROGBAR_NORMAL;
	else if(0 == strcasecmp("water", pvalue))
		ret = PROGBAR_WATER;
	else if(0 == strcasecmp("vertical", pvalue))
		ret = PROGBAR_VERTICAL;
	else
		ret = PROGBAR_NORMAL;

	return (ret);
}

static int _get_progbar_textformat(GuiProgbar *progbar)
{
	int ret = 0;
	char *pvalue = NULL;

	if(NULL == progbar)
		return (1);

	pvalue = (char *)widget_get_property(progbar->base, "text_format");
	if(NULL == pvalue)
		return (PROGBAR_DECIMAL);

	if(0 == strcasecmp("percentage", pvalue))
		ret = PROGBAR_PERCENTAGE;
	else if(0 == strcasecmp("decimal", pvalue))
		ret = PROGBAR_DECIMAL;
	else if(0 == strcasecmp("hide", pvalue))
		ret = PROGBAR_HIDE;
	else
		ret = PROGBAR_DECIMAL;

	return (ret);
}

static int _config_progbar_parametre(GuiWidget *self)
{
	GuiProgbar *progbar = NULL;
	const char *value = NULL;

	if(NULL == self)
		return (1);

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
		return (1);

	value = widget_get_property(self, "min");
	if((NULL != value) || (0 == progbar->minimum))
		progbar->minimum = widget_get_int(self, "min", 0);

	value = widget_get_property(self, "max");
	if((NULL != value) || (0 == progbar->maximum))
		progbar->maximum = widget_get_int(self, "max", 0);

	value = widget_get_property(self, "format");
	if((NULL != value) || (0 == progbar->format))
		progbar->format = _get_progbar_format(progbar);

	value = widget_get_property(self, "alignment");
	if((NULL != value) || (0 == progbar->text_position))
		progbar->text_position = widget_get_alignment(self);

	value = widget_get_property(self, "text_format");
	if((NULL != value) || (0 == progbar->text_format))
		progbar->text_format = _get_progbar_textformat(progbar);

	value = widget_get_property(self, "font");
	if((NULL != value) || (NULL == progbar->font))
		progbar->font = (char *)value;

	value = widget_get_property(self, "string");
	if((NULL != value) && (NULL == progbar->string))
		progbar->string = GUI_STRDUP(value);

	return (0);
}

static void *create_progbar(GuiWidget *self)
{
	GuiProgbar *progbar = NULL, *progbar_style = NULL;
	GuiWidget *parent = NULL, *style = NULL;

	if(NULL == self)
		return (NULL);

	progbar = (GuiProgbar *)GUI_MALLOC(sizeof(GuiProgbar));
	if(NULL == progbar)
		return (NULL);

	memset(progbar, 0, sizeof(GuiProgbar));

	self->object = (void *)(progbar);
	self->state = 0;

	progbar->base = self;

	/*Get Style*/
	style = (GuiWidget *)widget_get_style(self->stylename);
	if(style) {
		progbar_style = (GuiProgbar *)GUI_MALLOC(sizeof(GuiProgbar));
		if(NULL == progbar_style) {
			GUI_FREE(progbar);
			return (NULL);
		}

		memset(progbar_style, 0, sizeof(GuiProgbar));
		style->object = (void *)progbar_style;

		_config_progbar_parametre(style);

		memcpy(progbar, progbar_style, sizeof(GuiProgbar));

		progbar->base = self;
		GUI_FREE(style->object);
	}

	_config_progbar_parametre(self);
	if(progbar->minimum == progbar->maximum)
		return (NULL);

	parent = widget_get_parent(self);
	WIDGET_ADJUST_ACCORDING_PARENT(parent, self);

	progbar->value = widget_get_int(self, "value", 0);
	progbar->value = (progbar->value < progbar->minimum)?progbar->minimum:progbar->value;
	progbar->value = (progbar->value > progbar->maximum)?progbar->maximum:progbar->value;

	if(self->signal) {
		progbar->create_event = (GuiWidgetSignal *)hash_get(self->signal, "create");
		progbar->destroy_event = (GuiWidgetSignal *)hash_get(self->signal, "destroy");
		progbar->reachend_event = (GuiWidgetSignal *)hash_get(self->signal, "reach_end");
	}

	if(progbar->create_event)
		exec_widget_event_data(self, progbar->create_event, NULL);

	return (progbar);
}

static int progbar_release(GuiWidget *self)
{
	GuiProgbar *progbar = NULL;

	if (self == NULL || self->object == NULL)
		return (1);

	progbar = (GuiProgbar *)(self->object);

	if(progbar->back_surface)
		gal_free_surface(progbar->back_surface);

	if(progbar->fore_surface)
		gal_free_surface(progbar->fore_surface);

	if(progbar->destroy_event)
		exec_widget_event_data(self, progbar->destroy_event, NULL);

	if(progbar->string)
		GUI_FREE(progbar->string);

	GUI_FREE(self->object);


	return (0);
}

typedef enum
{
	IMAGE_TILE_VERTICAL,
	IMAGE_TILE_HORIZONTAL,
	IMAGE_ZOOM_VERTICAL,
	IMAGE_ZOOM_HORIZONTAL
}IMG_SEQUENCE;

static int _multiimage_draw(void* screen, image_desc *image, GUI_Rect *rect, IMG_SEQUENCE flag)
{
	GUI_Rect valid_rect = {0};
	int position = 0, point = 0, delta = 0;
	int x = 0, y = 0;

	if(NULL== image || NULL == rect)
	{
		return (1);
	}

	valid_rect = *rect;

	if((IMAGE_TILE_VERTICAL == flag) ||
			(IMAGE_ZOOM_VERTICAL == flag))
	{
		position = valid_rect.y;
		point = valid_rect.y + valid_rect.h;
		delta = image->height;
	}
	else if((IMAGE_TILE_HORIZONTAL == flag) ||
			(IMAGE_ZOOM_HORIZONTAL == flag))
	{
		position = valid_rect.x;
		point = valid_rect.x + valid_rect.w;
		delta = image->width;
	}

	image->effect = EFFECT_COPY;

	if((IMAGE_TILE_VERTICAL == flag) ||
			(IMAGE_TILE_HORIZONTAL == flag))
	{
		GuiSurface *src_surface = NULL;
		GAL_Rect src_rect = {0}, dst_rect = {0};

		src_surface = image_desc_surface(image);
		if(NULL == src_surface) {
			return (1);
		}

		dst_rect.x = valid_rect.x;
		dst_rect.y = valid_rect.y;
		dst_rect.w = valid_rect.w;
		dst_rect.h = valid_rect.h;

		src_rect.x = src_rect.y = 0;
		src_rect.w = image->width;
		src_rect.h = image->height;
		if((IMAGE_TILE_VERTICAL == flag) &&
		   (src_surface->sf.width != dst_rect.w)) {
			GuiSurface *tmp_surface = NULL;
			GAL_Rect tmp_rect = {0};

			tmp_rect.x = tmp_rect.y = 0;
			tmp_rect.w = dst_rect.w;
			tmp_rect.h = src_surface->sf.height;
			tmp_surface = hd_get_surface(&tmp_rect, src_surface->bpp, NULL, GUI_FALSE);
			if(tmp_surface) {
				gdi_begin();
				hd_zoom_surface(src_surface, &src_rect, tmp_surface, &tmp_rect);
				gdi_end();
				hd_free_surface(src_surface);
				src_surface = tmp_surface;
				src_rect.w = tmp_surface->sf.width;
			}
		} else if((IMAGE_TILE_HORIZONTAL == flag) &&
			  (src_surface->sf.height != dst_rect.h)) {
			GuiSurface *tmp_surface = NULL;
			GAL_Rect tmp_rect = {0};

			tmp_rect.x = tmp_rect.y = 0;
			tmp_rect.w = src_surface->sf.width;
			tmp_rect.h = dst_rect.h;
			tmp_surface = hd_get_surface(&tmp_rect, src_surface->bpp, NULL, GUI_FALSE);
			if(tmp_surface) {
				gdi_begin();
				hd_zoom_surface(src_surface, &src_rect, tmp_surface, &tmp_rect);
				gdi_end();
				hd_free_surface(src_surface);
				src_surface = tmp_surface;
				src_rect.h = tmp_surface->sf.height;
			}
		}
		hd_tile_surface(src_surface, &src_rect, screen, &dst_rect);
		hd_add_blit_element(src_surface);
	}
	else if((IMAGE_ZOOM_VERTICAL == flag) ||
			(IMAGE_ZOOM_HORIZONTAL == flag))
	{
		if(IMAGE_ZOOM_VERTICAL == flag)
		{
			x = valid_rect.x;
			y = position;
			gal_img_desc(screen,
					image,
					x,
					y,
					image->width,
					point - y);
		}
		else if(IMAGE_ZOOM_HORIZONTAL == flag)
		{
			x = position;
			y = valid_rect.y;
			gal_img_desc(screen,
					image,
					x,
					y,
					point - x,
					image->height);
		}
	}

	if((IMAGE_TILE_HORIZONTAL == flag) ||
			(IMAGE_ZOOM_HORIZONTAL == flag))
	{
		if((point - image->width) >= rect->x)
		{
			gal_img_desc(screen,
					image,
					point - image->width,
					valid_rect.y,
					image->width,
					image->height);
		}
	}

	image->effect = EFFECT_SRC_TO_DST;

	return (0);
}

static int _normal_progbar_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiProgbar *progbar = NULL;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	image_desc *back_img = NULL, *fore_img = NULL;
	GUI_Rect valid_rect = {0}, src_rect = {0}, dst_rect = {0};
	GAL_Interval interval = {0};
	int len = 0, value = 0, min = 0, max = 0, width = 0, alignment = 0;
	char value_text[6] = {0};
	Color color = 0;

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	progbar->font = (char*)widget_get_property(self, "font");
	old_font = gxgdi_set_font((char *)progbar->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	min = progbar->minimum;
	max = progbar->maximum;
	value = progbar->value;

	back_img = progbar->back_img[1];
	fore_img = progbar->fore_img[1];

	valid_rect = self->rect;
	value = (value - min) * valid_rect.w / (max - min);

	if(NULL != back_img)
	{
		valid_rect.x = valid_rect.y = 0;
		valid_rect.h = (self->rect.h < back_img->height) ? (self->rect.h) : (back_img->height);
		if(NULL == progbar->back_surface)
		{
			width = valid_rect.w;
			valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
			progbar->back_surface = gal_get_surface((GAL_Rect*)&valid_rect, gui_core->config.bpp);
			valid_rect.w = width;
			gdi_commit();
			//hd_new_blit_list();
			color = self->back_color[0];
			color = gal_color2index(screen, color);
			gdi_begin();
			gal_fillrect(progbar->back_surface,
					(GAL_Rect*)&valid_rect,
					(unsigned int)color);
			gdi_end();

			gdi_begin();
			_multiimage_draw(progbar->back_surface,
					back_img,
					&valid_rect,
					IMAGE_TILE_HORIZONTAL);
			gdi_end();
			gdi_commit();
		}
	}

	if(NULL != fore_img)
	{
		valid_rect.x = valid_rect.y = 0;
		valid_rect.h = (self->rect.h < fore_img->height) ? (self->rect.h) : (fore_img->height);
		if(NULL == progbar->fore_surface)
		{
			width = valid_rect.w;
			valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
			progbar->fore_surface = gal_get_surface((GAL_Rect*)&valid_rect, gui_core->config.bpp);
			valid_rect.w = width;
			gdi_commit();
			//hd_new_blit_list();
			color = self->back_color[1];
			color = gal_color2index(screen, color);
			gdi_begin();
			gal_fillrect(progbar->fore_surface,
					(GAL_Rect*)&valid_rect,
					(unsigned int)color);
			gdi_end();

			gdi_begin();
			_multiimage_draw(progbar->fore_surface,
					fore_img,
					&valid_rect,
					IMAGE_TILE_HORIZONTAL);
			gdi_end();
			gdi_commit();
		}
	}

	src_rect = dst_rect = valid_rect;
	src_rect.x = src_rect.y = 0;
	dst_rect.x = self->rect.x + src_rect.x;
	dst_rect.y = self->rect.y + src_rect.y;

	gdi_commit();

	if(back_img)
	{
		src_rect.h = dst_rect.h =
			(valid_rect.h < back_img->height) ? (valid_rect.h) : (back_img->height);
	}

	if(NULL != back_img)
	{
		gal_stretch_surface(progbar->back_surface,
				(GAL_Rect *)&src_rect,
				screen,
				(GAL_Rect *)&dst_rect);
	}
	else
	{
		color = self->back_color[0];
		color = gal_color2index(screen, color);
		valid_rect.x = self->rect.x;
		valid_rect.y = self->rect.y;
		gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);
	}

	if(NULL != fore_img)
	{
		if(value > 0)
		{
			src_rect.w = dst_rect.w = value;
			if((gui_core->config.mirror) && (!self->disable_mirror)) {
				dst_rect.x = self->rect.x + self->rect.w - dst_rect.w;
			}
			if(gui_core->config.mirror) {
				GuiSurface *tmp_surface = NULL;

				tmp_surface = gal_get_surface((GAL_Rect *)&src_rect, ((GuiSurface *)progbar->fore_surface)->bpp);
				gdi_begin();
				hd_mirror_image_surface(progbar->fore_surface,
						(GAL_Rect *)&src_rect,
						tmp_surface,
						(GAL_Rect *)&src_rect,
						REVERSE_HORIZONTAL);
				gdi_end();
				hd_stretch_surface(tmp_surface,
						(GAL_Rect *)&src_rect,
						screen,
						(GAL_Rect *)&dst_rect);
				hd_add_blit_element(tmp_surface);
			} else {
				gal_stretch_surface(progbar->fore_surface,
						(GAL_Rect *)&src_rect,
						screen,
						(GAL_Rect *)&dst_rect);
			}
		}
	}
	else
	{
		color = self->back_color[1];
		color = gal_color2index(screen, color);
		valid_rect.x = self->rect.x;
		valid_rect.y = self->rect.y;
		valid_rect.w = value;
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			valid_rect.x = valid_rect.x + valid_rect.w - dst_rect.w;
		}
		gal_fillrect(screen, (GAL_Rect *)&valid_rect, color);
	}

	gdi_commit();
	if(PROGBAR_HIDE != progbar->text_format)
	{
		valid_rect = self->rect;
		if(PROGBAR_PERCENTAGE == progbar->text_format)
		{
			value = progbar->value - min;
			sprintf(value_text,  "%02d", value * 100 / (max - min));
			strcat(value_text, "%");
		}
		else if(PROGBAR_DECIMAL == progbar->text_format)
		{
			value = progbar->value - min;
			sprintf(value_text,  "%02d", value);
		}
		else
		{
			;
		}

		len = gdi_text_len(value_text);

		if(GUI_TA_HCENTRE & progbar->text_position)
		{
			valid_rect.x = self->rect.x + (self->rect.w  -len) / 2;
			alignment = GUI_TA_LEFT | GUI_TA_TOP;
		}
		else
		{
			alignment = progbar->text_position;
			widget_string_alignment(alignment,
									len,
									valid_rect.h,
									&(valid_rect),
									&(valid_rect));
		}
		valid_rect.w = len;
		valid_rect.y = valid_rect.y + ((valid_rect.h - gdi_get_charheight()) / 2);
		valid_rect.h = valid_rect.h - ((valid_rect.h - gdi_get_charheight()) / 2);

		color = self->fore_color[0];
		color = gal_color2index(screen, color);
		gxgdi_draw_string(screen,
				value_text,
				(GAL_Rect*)(&valid_rect),
				&interval,
				alignment,
				color,
				SINGLELINE_MODE);
	}

	gxgdi_set_font((char *)old_font);

	return (0);
}

static int _water_progbar_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiProgbar *progbar = NULL;
	image_desc *back_img[3] = {NULL, NULL, NULL};
	image_desc *fore_img[3] = {NULL, NULL, NULL};
	GUI_Rect valid_rect = {0}, src_rect = {0}, dst_rect = {0};
	GAL_Interval interval = {0};
	int value = 0, len = 0, width = 0;
	int max = 0, min = 0, color = 0, alignment = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	char value_text[6] = {0};

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	back_img[0] = progbar->back_img[0];
	back_img[1] = progbar->back_img[1];
	back_img[2] = progbar->back_img[2];

	if((NULL == back_img[0]) && (NULL == back_img[1]) && (NULL == back_img[2]))
	{
		return (1);
	}

	fore_img[0] = progbar->fore_img[0];
	fore_img[1] = progbar->fore_img[1];
	fore_img[2] = progbar->fore_img[2];

	if((NULL == fore_img[0]) && (NULL == fore_img[1]) && (NULL == fore_img[2]))
	{
		return (1);
	}

	value = progbar->value;
	max = progbar->maximum;
	min = progbar->minimum;

	progbar->font = (char*)widget_get_property(self, "font");
	old_font = gxgdi_set_font((char *)progbar->font);

	new_font = gxgdi_get_font();
	if(NULL == new_font)
	{
		return (1);
	}

	color = gui_core->config.gui_trans;
	color = gal_color2index(screen, color);

	valid_rect = self->rect;
	valid_rect.x = valid_rect.y = 0;
	valid_rect.h = (self->rect.h < back_img[1]->height) ? (self->rect.h) : (back_img[1]->height);
	if(NULL == progbar->back_surface)
	{
		width = valid_rect.w;
		valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		progbar->back_surface = gal_get_surface((GAL_Rect*)&valid_rect, gui_core->config.bpp);
		valid_rect.w = width;
		gdi_begin();
		hd_fillrect(progbar->back_surface,
				(GAL_Rect *)(&valid_rect),
				color);

		back_img[0]->effect = EFFECT_COPY;
		gal_img_desc(progbar->back_surface,
				back_img[0],
				valid_rect.x,
				valid_rect.y,
				back_img[0]->width,
				back_img[0]->height);
		back_img[0]->effect = EFFECT_SRC_TO_DST;

		back_img[2]->effect = EFFECT_COPY;
		gal_img_desc(progbar->back_surface,
				back_img[2],
				valid_rect.x + valid_rect.w - back_img[2]->width,
				valid_rect.y,
				back_img[2]->width,
				back_img[2]->height);
		back_img[2]->effect = EFFECT_SRC_TO_DST;
		gdi_end();

		gdi_begin();
		valid_rect.x += back_img[0]->width;
		valid_rect.w -= (back_img[0]->width + back_img[2]->width);
		_multiimage_draw(progbar->back_surface,
				back_img[1],
				&valid_rect,
				IMAGE_TILE_HORIZONTAL);
		gdi_end();
	}

	valid_rect = self->rect;
	valid_rect.x = valid_rect.y = 0;
	valid_rect.h = (self->rect.h < fore_img[1]->height) ? (self->rect.h) : (fore_img[1]->height);
	if(NULL == progbar->fore_surface)
	{
		width = valid_rect.w;
		valid_rect.w = ((((valid_rect.w * gui_core->config.bpp) + 31) >> 5) << 2) / (gui_core->config.bpp / 8);
		progbar->fore_surface = gal_get_surface((GAL_Rect*)&valid_rect, gui_core->config.bpp);
		valid_rect.w = width;
		gdi_begin();
		hd_fillrect(progbar->fore_surface,
				(GAL_Rect *)(&valid_rect),
				color);

		fore_img[0]->effect = EFFECT_COPY;
		gal_img_desc(progbar->fore_surface,
				fore_img[0],
				valid_rect.x,
				valid_rect.y,
				fore_img[0]->width,
				fore_img[0]->height);
		fore_img[0]->effect = EFFECT_SRC_TO_DST;

		fore_img[2]->effect = EFFECT_COPY;
		gal_img_desc(progbar->fore_surface,
				fore_img[2],
				valid_rect.x + valid_rect.w - fore_img[2]->width,
				valid_rect.y,
				fore_img[2]->width,
				fore_img[2]->height);
		fore_img[2]->effect = EFFECT_SRC_TO_DST;
		gdi_end();

		gdi_begin();
		valid_rect.x += fore_img[0]->width;
		valid_rect.w -= (fore_img[0]->width + fore_img[2]->width);
		_multiimage_draw(progbar->fore_surface,
				fore_img[1],
				&valid_rect,
				IMAGE_TILE_HORIZONTAL);
		gdi_end();
	}

	src_rect = dst_rect = self->rect;
	src_rect.x = src_rect.y = 0;
	dst_rect.x = self->rect.x + src_rect.x;
	dst_rect.y = self->rect.y + src_rect.y;
	dst_rect.h = src_rect.h = valid_rect.h;
	gal_stretch_surface(progbar->back_surface,
			(GAL_Rect *)&src_rect,
			screen,
			(GAL_Rect *)&dst_rect);

	value = (value - min) * self->rect.w / (max - min);
	if(value >= (fore_img[0]->width + fore_img[2]->width))
	{
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			dst_rect.x = dst_rect.x + dst_rect.w - value;
		}
		gal_img_desc(screen,
				fore_img[0],
				dst_rect.x,
				dst_rect.y,
				fore_img[0]->width,
				fore_img[0]->height);
		gal_img_desc(screen,
				fore_img[2],
				dst_rect.x + value - fore_img[2]->width,
				dst_rect.y,
				fore_img[2]->width,
				fore_img[2]->height);

		src_rect.x += fore_img[0]->width;
		src_rect.w = (value - (fore_img[0]->width + fore_img[2]->width));
		dst_rect.x = self->rect.x + src_rect.x;
		if((gui_core->config.mirror) && (!self->disable_mirror)) {
			dst_rect.x = dst_rect.x + dst_rect.w - src_rect.w - fore_img[2]->width - fore_img[0]->width;
		}
		dst_rect.y = self->rect.y + src_rect.y;
		dst_rect.h = src_rect.h = valid_rect.h;
		dst_rect.w = src_rect.w;

		gal_stretch_surface(progbar->fore_surface,
				(GAL_Rect *)&src_rect,
				screen,
				(GAL_Rect *)&dst_rect);
	}
	gdi_commit();

	if(PROGBAR_HIDE != progbar->text_format)
	{
		valid_rect = self->rect;
		if(PROGBAR_PERCENTAGE == progbar->text_format)
		{
			value = progbar->value - min;
			sprintf(value_text,  "%02d", value * 100 / (max - min));
			strcat(value_text, "%");
		}
		else if(PROGBAR_DECIMAL == progbar->text_format)
		{
			value = progbar->value - min;
			sprintf(value_text,  "%02d", value);
		}
		else
		{
			;
		}

		len = gdi_text_len(value_text);

		if(GUI_TA_HCENTRE & progbar->text_position)
		{
			valid_rect.x = self->rect.x + (self->rect.w  -len) / 2;
			alignment = GUI_TA_LEFT | GUI_TA_TOP;
		}
		else
		{
			alignment = progbar->text_position;
			widget_string_alignment(alignment,
									len,
									valid_rect.h,
									&(valid_rect),
									&(valid_rect));
		}
		valid_rect.w = len;
		valid_rect.y = valid_rect.y + ((valid_rect.h - gdi_get_charheight()) / 2);
		valid_rect.h = valid_rect.h - ((valid_rect.h - gdi_get_charheight()) / 2);

		color = self->fore_color[0];
		color = gal_color2index(screen, color);
		gxgdi_draw_string(screen,
				value_text,
				(GAL_Rect*)(&valid_rect),
				&interval,
				alignment,
				color,
				SINGLELINE_MODE);
	}

	gxgdi_set_font((char *)old_font);
	return (0);
}

static int _vertical_progbar_draw(GuiWidget *self)
{
	GuiCore* gui_core = NULL;
	GuiProgbar *progbar = NULL;
	image_desc *fore_img = NULL, *back_img = NULL;
	int old_top = 0, old_bottom = 0, new_top = 0, new_bottom = 0;
	int min = 0, max = 0, delta = 0, value = 0, len = 0, y_divid = 0;
	GUI_Rect valid_rect = {0}, temp_rect = {0};
	Color color = 0;
	struct gdi_font* old_font = NULL, *new_font = NULL;
	char value_text[6] = {0};

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	valid_rect = self->rect;

	min = progbar->minimum;
	max = progbar->maximum;
	value = progbar->value;

	fore_img = progbar->fore_img[1];
	if(fore_img && (fore_img->width > self->rect.w))
		return (1);
	back_img = progbar->back_img[1];
	if(back_img && (back_img->width > self->rect.w))
		return (1);

	valid_rect = self->rect;
	valid_rect.x = valid_rect.y = 0;
	if(NULL == progbar->back_surface) {
		progbar->back_surface = hd_get_surface((GAL_Rect *)(&valid_rect), gui_core->config.bpp, NULL, GUI_FALSE);
		if(NULL != progbar->back_surface) {
			color = hd_color2index(progbar->back_surface, gui_core->config.gui_trans);
			gdi_begin();
			hd_fillrect(progbar->back_surface, (GAL_Rect *)(&valid_rect), color);
			gdi_end();
		}
		if(back_img) {
			gdi_begin();
			_multiimage_draw(progbar->back_surface,
					back_img,
					&valid_rect,
					IMAGE_TILE_VERTICAL);
			gdi_end();
		} else {
			color = hd_color2index(progbar->back_surface, self->back_color[0]);
			if((self->back_color[0] & 0x00FFFFFF) == (gui_core->config.gui_trans & 0x00FFFFFF)) {
				gdi_commit();
				gdi_begin();
				hd_copy_surface(screen, (GAL_Rect *)&(self->rect), progbar->back_surface, (GAL_Rect *)&(valid_rect));
				gdi_end();
			} else {
				gdi_begin();
				gal_fillrect(progbar->back_surface, (GAL_Rect *)&valid_rect, color);
				gdi_end();
			}
		}
	}

	if(NULL == progbar->fore_surface) {
		progbar->fore_surface = hd_get_surface((GAL_Rect *)(&valid_rect), gui_core->config.bpp, NULL, GUI_FALSE);
		if(NULL != progbar->fore_surface) {
			color = hd_color2index(progbar->fore_surface, gui_core->config.gui_trans);
			gdi_begin();
			hd_fillrect(progbar->fore_surface, (GAL_Rect *)(&valid_rect), color);
			gdi_end();
		}

		if(fore_img) {
			gdi_begin();
			_multiimage_draw(progbar->fore_surface,
					fore_img,
					&valid_rect,
					IMAGE_TILE_VERTICAL);
			gdi_end();
		} else {
			color = hd_color2index(progbar->fore_surface, self->back_color[1]);
			gdi_begin();
			gal_fillrect(progbar->fore_surface, (GAL_Rect *)&valid_rect, color);
			gdi_end();
		}
	}

	temp_rect = self->rect;
	gdi_begin();
	hd_stretch_surface(progbar->back_surface, (GAL_Rect *)&valid_rect, screen, (GAL_Rect *)&temp_rect);
	value = ((value - min) * self->rect.h) / (max - min);
	valid_rect.y += (self->rect.h - value);
	valid_rect.h = value;

	temp_rect = valid_rect;
	temp_rect.x = self->rect.x;
	temp_rect.y += self->rect.y;

	hd_stretch_surface(progbar->fore_surface, (GAL_Rect *)&valid_rect, screen, (GAL_Rect *)&temp_rect);
	gdi_end();

	gxgdi_set_font((char *)old_font);

	return (0);
}

typedef int (*progbar_class_draw)(GuiWidget *self);

progbar_class_draw progbar_draw_ops[3] =
{
	_normal_progbar_draw,
	_water_progbar_draw,
	_vertical_progbar_draw
};

static int progbar_draw(GuiWidget *self)
{
	int format = 0, ret = 0, len = 0;
	int color = 0;
	GuiProgbar *progbar = NULL;
	GUI_Rect valid_rect = {0};
	GAL_Interval interval = {0};
	struct gdi_font* old_font = NULL;

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	if(progbar->minimum >= progbar->maximum)
	{
		return (1);
	}

	format = progbar->format;

	progbar->font = (char*)widget_get_property(self, "font");

	ret = progbar_draw_ops[format](self);

	if(progbar->string)
	{
		len = gdi_text_len(progbar->string);

		valid_rect  = self->rect;
		valid_rect.x = self->rect.x + (self->rect.w  -len) / 2;
		valid_rect.w = len;
		valid_rect.y = valid_rect.y + ((valid_rect.h - gdi_get_charheight()) / 2);
		valid_rect.h = valid_rect.h - ((valid_rect.h - gdi_get_charheight()) / 2);

		color = self->fore_color[0];
		color = gal_color2index(screen, color);
		old_font = gxgdi_set_font((char *)progbar->font);
		gxgdi_draw_string(screen,
						  progbar->string,
						  (GAL_Rect*)(&valid_rect),
						  &interval,
						  GUI_TA_LEFT | GUI_TA_TOP,
						  color,
						  SINGLELINE_MODE);
		gxgdi_set_font((char *)old_font);
	}

	return (ret);
}

static int progbar_event(GuiWidget *self, void *data)
{
	return (0);
}

static int progbar_set_property(GuiWidget *self, char *name, void *data)
{
	GuiCore *gui_core = NULL;
	GuiProgbar *progbar = NULL;
	GuiWidget *top = NULL;
	int value;
	char string[100];

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	if((0 == strcasecmp("value", name)) && data)
	{
		value = *(int*)(data);

		if(value == progbar->maximum)
		{
			if(progbar->reachend_event)
			{
				exec_widget_event_data(self, progbar->reachend_event, NULL);
			}
		}

		value = (value < progbar->minimum)?progbar->minimum:value;
		value = (value > progbar->maximum)?progbar->maximum:value;
		progbar->value = value;

		sprintf(string, "%d", value);
		widget_set_property(self, name, string);

		top = stack_peek(gui_core->win_stack);
		if(NULL == top)
		{
			return (1);
		}

		widget_set_update(self, GUI_TRUE);
	}
	else if(0 == strcasecmp("back_image_l", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->back_surface)
			{
				gal_free_surface(progbar->back_surface);
				progbar->back_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("back_image_m", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->back_surface)
			{
				gal_free_surface(progbar->back_surface);
				progbar->back_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("back_image_r", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->back_surface)
			{
				gal_free_surface(progbar->back_surface);
				progbar->back_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("fore_image_l", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->fore_surface)
			{
				gal_free_surface(progbar->fore_surface);
				progbar->fore_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("fore_image_m", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->fore_surface)
			{
				gal_free_surface(progbar->fore_surface);
				progbar->fore_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("fore_image_r", name))
	{
		if(PROGBAR_WATER == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->fore_surface)
			{
				gal_free_surface(progbar->fore_surface);
				progbar->fore_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("back_image", name))
	{
		if(PROGBAR_NORMAL == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->back_surface)
			{
				gal_free_surface(progbar->back_surface);
				progbar->back_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("fore_image", name))
	{
		if(PROGBAR_NORMAL == progbar->format)
		{
			widget_set_property(self, name, data);
			widget_set_update(self, GUI_TRUE);
			if(progbar->fore_surface)
			{
				gal_free_surface(progbar->fore_surface);
				progbar->fore_surface = NULL;
			}
		}
	}
	else if(0 == strcasecmp("update", name))
	{
		progbar->old_value = 0;
		if(progbar->back_surface)
		{
			gal_free_surface(progbar->back_surface);
			progbar->back_surface = NULL;
		}
		if(progbar->fore_surface)
		{
			gal_free_surface(progbar->fore_surface);
			progbar->fore_surface = NULL;
		}
		widget_set_update(self, GUI_TRUE);
	}
	else if((0 == strcasecmp("string", name)) && (data))
	{
		progbar->string = GUI_STRDUP((char *)data);
		widget_set_update(self, GUI_TRUE);
	}
	else
	{
		widget_set_property(self, name, data);
		widget_set_update(self, GUI_TRUE);
	}

	return (0);
}

static int progbar_get_property(GuiWidget *self, char *name, void *data)
{
	GuiProgbar *progbar = NULL;
	int value;
	//	char *string = NULL;

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	if(0 == strcasecmp("value", name))
	{
		value = progbar->value;

		*(int*)data = value;
	}
	else
	{
		return (1);
	}

	return (0);
}

static int progbar_prepare_image(GuiWidget *self)
{
	GuiProgbar *progbar = NULL;
	const char *value = NULL;

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	if(PROGBAR_WATER == progbar->format)
	{
		value = widget_get_property(self, "back_image_l");
		if((NULL != value) || (NULL == progbar->back_img[0]))
		{
			progbar->back_img[0] = widget_img_load(value);
		}
		value = widget_get_property(self, "back_image_m");
		if((NULL != value) || (NULL == progbar->back_img[1]))
		{
			progbar->back_img[1] = widget_img_load(value);
		}
		value = widget_get_property(self, "back_image_r");
		if((NULL != value) || (NULL == progbar->back_img[2]))
		{
			progbar->back_img[2] = widget_img_load(value);
		}
	}
	else
	{
		value = widget_get_property(self, "back_image");
		if((NULL != value) || (NULL == progbar->back_img[1]))
		{
			progbar->back_img[1] = widget_img_load(value);
		}
	}

	if(PROGBAR_WATER == progbar->format)
	{
		value = widget_get_property(self, "fore_image_l");
		if((NULL != value) || (NULL == progbar->back_img[0]))
		{
			progbar->fore_img[0] = widget_img_load(value);
		}
		value = widget_get_property(self, "fore_image_m");
		if((NULL != value) || (NULL == progbar->back_img[1]))
		{
			progbar->fore_img[1] = widget_img_load(value);
		}
		value = widget_get_property(self, "fore_image_r");
		if((NULL != value) || (NULL == progbar->back_img[2]))
		{
			progbar->fore_img[2] = widget_img_load(value);
		}
	}
	else
	{
		value = widget_get_property(self, "fore_image");
		if((NULL != value) || (NULL == progbar->back_img[1]))
		{
			progbar->fore_img[1] = widget_img_load(value);
		}
	}

	return (0);
}

static int progbar_clear_image(GuiWidget *self)
{
	GuiProgbar *progbar = NULL;

	if(NULL == self)
	{
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	if(PROGBAR_WATER == progbar->format)
	{
		if(progbar->back_img[0])
		{
			widget_img_clear(&(progbar->back_img[0]));
		}

		if(progbar->back_img[1])
		{
			widget_img_clear(&(progbar->back_img[1]));
		}

		if(progbar->back_img[2])
		{
			widget_img_clear(&(progbar->back_img[2]));
		}
	}
	else
	{
		if(progbar->back_img[1])
		{
			widget_img_clear(&(progbar->back_img[1]));
		}
	}

	if(PROGBAR_WATER == progbar->format)
	{
		if(progbar->fore_img[0] )
		{
			widget_img_clear(&(progbar->fore_img[0]));
		}

		if(progbar->fore_img[1])
		{
			widget_img_clear(&(progbar->fore_img[1]));
		}

		if(progbar->fore_img[2])
		{
			widget_img_clear(&(progbar->fore_img[2]));
		}
	}
	else
	{
		if(progbar->fore_img[1])
		{
			widget_img_clear(&(progbar->fore_img[1]));
		}
	}

	return (0);
}

static int progbar_unactive(GuiWidget *self)
{
	GuiProgbar *progbar = NULL;

	if(NULL == self) {
		return (1);
	}

	progbar = (GuiProgbar *)(self->object);
	if(NULL == progbar)
	{
		return (1);
	}

	if(progbar->back_surface) {
		gal_free_surface(progbar->back_surface);
		progbar->back_surface = NULL;
	}

	if(progbar->fore_surface) {
		gal_free_surface(progbar->fore_surface);
		progbar->fore_surface = NULL;
	}

	return (0);
}

GuiWidgetOps progbar_ops = {
	.owner = "progbar",
	.create = create_progbar,
	.release = progbar_release,
	.prepare_image = progbar_prepare_image,
	.draw = progbar_draw,
	.clear_image   = progbar_clear_image,
	.unactive = progbar_unactive,
	.event = progbar_event,
	.set = progbar_set_property,
	.get = progbar_get_property
};


