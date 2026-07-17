#include "all_widget.h"

static int _config_spectrum_parametre(GuiWidget *self)
{
	const char *value = NULL;
	GuiSpectrum *spectrum = NULL;

	if(NULL == self)
	{
		return (1);
	}

	spectrum = (GuiSpectrum *)(self->object);
	if(NULL == spectrum)
	{
		return (1);
	}

	if(NULL == self->property)
	{
		return (0);
	}

	value = (const char *)hash_get(self->property, "channel_count");
	if((NULL != value) || (0 == spectrum->channal_num))
	{
		spectrum->channal_num = widget_get_int(self, "channel_count", 0);
	}

	value = (const char *)hash_get(self->property, "channel_width");
	if((NULL != value) || (0 == spectrum->channal_width))
	{
		spectrum->channal_width = widget_get_int(self, "channel_width", 0);
	}

	value = (const char *)hash_get(self->property, "color_level");
	if((NULL != value) || (0 == spectrum->color_level))
	{
		spectrum->color_level = widget_get_int(self, "color_level", 0);
	}

	value = (const char *)hash_get(self->property, "maximum");
	if((NULL != value) || (0 == spectrum->max))
	{
		spectrum->max = widget_get_int(self, "maximum", 0);
	}

	return (0);
}

static void* create_spectrum(GuiWidget *self)
{
	GuiSpectrum *spectrum = NULL, *spectrum_style = NULL;
	GuiWidget *style = NULL;
	SpectrumInfo *cur_spec = NULL, *new_spec = NULL;
	int i = 0;

	if(NULL == self)
	{
		return (NULL);
	}

	if(NULL != self->object)
	{
		return (self->object);
	}

	spectrum = (GuiSpectrum *)GUI_MALLOC(sizeof(GuiSpectrum));
	if(NULL == spectrum)
	{
		return (NULL);
	}

	memset(spectrum, 0, sizeof(GuiSpectrum));

	spectrum->base = self;
	self->object = (void *)spectrum;

	style = (GuiWidget *)widget_get_style(self->stylename);

	if(style)
	{
		spectrum_style = (GuiSpectrum *)GUI_MALLOC(sizeof(GuiSpectrum));

		if(NULL == spectrum_style)
		{
			GUI_FREE(spectrum);
			return (NULL);
		}

		memset(spectrum_style, 0, sizeof(GuiSpectrum));
		style->object = (void *)spectrum_style;

		_config_spectrum_parametre(style);

		memcpy(spectrum, spectrum_style, sizeof(GuiSpectrum));

		GUI_FREE(style->object);
	}

	_config_spectrum_parametre(self);

	spectrum->update_channel = 1;

	if(spectrum->channal_num)
	{
		for(i = 0; i < spectrum->channal_num; i++)
		{
			new_spec = (SpectrumInfo *)GUI_MALLOC(sizeof(SpectrumInfo));
			if(NULL == new_spec)
			{
				return (NULL);
			}

			memset(new_spec, 0, sizeof(SpectrumInfo));
			new_spec->channal_num = spectrum->channal_num - 1 - i;
			new_spec->value = 0;

			new_spec->next = cur_spec;
			cur_spec = new_spec;
		}

		spectrum->information = new_spec;
	}

	widget_set_update(self, GUI_TRUE);

	return (spectrum);
}

static int spectrum_release(GuiWidget *self)
{
	GuiSpectrum *spectrum = NULL;
	SpectrumInfo *cur_spec = NULL, *next = NULL;

	if(NULL == self)
	{
		return (1);
	}

	spectrum = (GuiSpectrum *)(self->object);
	if(NULL == spectrum)
	{
		return (1);
	}

	if(spectrum->channal_num)
	{
		cur_spec = spectrum->information;
		while(cur_spec)
		{
			next = cur_spec->next;
			GUI_FREE(cur_spec);
			cur_spec = next;
		}

		spectrum->information = NULL;
	}

	GUI_FREE(self->object);

	return (0);
}

static int spectrum_draw(GuiWidget *self)
{
	GuiCore *gui_core = NULL;
	GuiSpectrum *spectrum = NULL;
	int back_color = 0, fore_color = 0, len = 0;
	int interval = 0, value = 0;
	int position = 0;
	GUI_Rect channel_rect = {0};
	SpectrumInfo *cur_spec = NULL;

	if(NULL == self)
	{
		return (1);
	}

	spectrum = (GuiSpectrum *)(self->object);
	if(NULL == spectrum)
	{
		return (1);
	}

	gui_core = GUI_GetCurrent();

	back_color = self->back_color[0];
	back_color = gal_color2index(screen, back_color);
	fore_color = self->fore_color[0];
	fore_color = gal_color2index(screen, fore_color);

	interval = (self->rect.w - (spectrum->channal_width * spectrum->channal_num)) /
		(spectrum->channal_num - 1);
	if(0 > interval)
	{
		return (1);
	}

	if(spectrum->update_channel)
	{
		if(back_color)
		{
			gal_fillrect(screen, (GAL_Rect *)&(self->rect), back_color);
		}

		/*Draw Channel*/
		cur_spec = spectrum->information;

		while(cur_spec)
		{
			//	channel_num = cur_spec->channal_num;
			value = cur_spec->value;

			if(value < spectrum->max)
			{
				len = (value * self->rect.h) / spectrum->max;

				position = cur_spec->channal_num * (interval + spectrum->channal_width);

				channel_rect.x = self->rect.x + position;
				channel_rect.y = self->rect.y + (self->rect.h - len);
				channel_rect.w = spectrum->channal_width;
				channel_rect.h = len;

				if((spectrum->color_level) && (cur_spec->value != spectrum->max))
				{
					int shift = 0;

					fore_color = cur_spec->channal_num * spectrum->color_level / spectrum->channal_num;
					fore_color = fore_color * 0xFF / spectrum->color_level;

					shift = cur_spec->channal_num / (spectrum->channal_num / 3);

					fore_color = (0xFF - fore_color) << (shift * 8);

					if(gui_core->config.osd_trans == fore_color)
					{
						fore_color = gal_color2index(screen, fore_color);
						fore_color += 1;
					}
					else
					{
						fore_color = gal_color2index(screen, fore_color);
					}
					if(0 == fore_color)
					{
						fore_color += 1;
					}
				}

				gal_fillrect(screen, (GAL_Rect *)(&channel_rect), fore_color);
			}

			cur_spec = cur_spec->next;
		}

		spectrum->update_channel = 0;

	}
	else
	{
		cur_spec = spectrum->information;

		while(cur_spec)
		{
			value = cur_spec->value;

			if(cur_spec->need_update)
			{
				len = (value * self->rect.h) / spectrum->max;

				position = cur_spec->channal_num * (interval + spectrum->channal_width);

				channel_rect.x = self->rect.x + position;
				channel_rect.y = self->rect.y;
				channel_rect.w = spectrum->channal_width;
				channel_rect.h = self->rect.h;

				gal_fillrect(screen, (GAL_Rect *)(&channel_rect), back_color);

				channel_rect.x = self->rect.x + position;
				channel_rect.y = self->rect.y + (self->rect.h - len);
				channel_rect.w = spectrum->channal_width;
				channel_rect.h = len;

				if((spectrum->color_level) && (cur_spec->value != spectrum->max))
				{
					int shift = 0;

					fore_color = cur_spec->channal_num * spectrum->color_level / spectrum->channal_num;
					fore_color = fore_color * 0xFF / spectrum->color_level;

					shift = cur_spec->channal_num / (spectrum->channal_num / 3);

					fore_color = (0xFF - fore_color) << (shift * 8);

					fore_color = gal_color2index(screen, fore_color);
					if(0 == fore_color)
					{
						fore_color += 1;
					}
				}

				gal_fillrect(screen, (GAL_Rect *)(&channel_rect), fore_color);
				cur_spec->need_update = 0;
			}
			cur_spec = cur_spec->next;
		}
	}

	return (0);
}

static int spectrum_event(GuiWidget *self, void *data)
{
	return (0);
}

static int spectrum_set_property(GuiWidget *self, char *name, void *data)
{
	GuiSpectrum *spectrum = NULL;
	SpectrumInfo *new_info = NULL, *cur_info = NULL;


	if(NULL == self)
	{
		return (1);
	}

	spectrum = (GuiSpectrum *)(self->object);
	if(NULL == spectrum)
	{
		return (1);
	}

	if(0 == strcasecmp("channel", name))
	{
		new_info = (SpectrumInfo *)(data);
		if(NULL == new_info)
		{
			return (1);
		}

		cur_info = spectrum->information;
		while(cur_info)
		{
			if(new_info->channal_num == cur_info->channal_num)
			{
				break;
			}
			cur_info = cur_info->next;
		}

		if(cur_info)
		{
			cur_info->value = new_info->value;
			cur_info->need_update = 1;
			widget_set_update(self, GUI_TRUE);
		}
	}

	return (0);
}

static int spectrum_get_property(GuiWidget *self, char *name, void *data)
{
	return (1);
}

GuiWidgetOps spectrum_ops =
{
	.owner = "spectrum",
	.create = create_spectrum,
	.release = spectrum_release,
	.draw = spectrum_draw,
	.event = spectrum_event,
	.set = spectrum_set_property,
	.get = spectrum_get_property
};


