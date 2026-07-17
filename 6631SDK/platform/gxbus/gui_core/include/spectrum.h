#ifndef __SPECTRUM_H__
#define __SPECTRUM_H__

#include "gui_private.h"

typedef struct _GuiSpectrum{
	GuiWidget *base;
	int channal_num;
	int channal_width;
	int update_channel;
	int max;
	int color_level;
	int format;
	SpectrumInfo *information;
}GuiSpectrum;

extern GuiWidgetOps spectrum_ops;

#endif
