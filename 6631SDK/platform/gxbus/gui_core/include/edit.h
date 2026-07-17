#ifndef __EDIT_H__
#define __EDIT_H__
#include "gui_private.h"

typedef enum
{
	EDIT_STRING,
	EDIT_DIGIT,
	EDIT_IP_ADDRESS,
	EDIT_TIME_BOX,
	EDIT_DATE_BOX,
	EDIT_DATE_YMD,
	EDIT_DATE_YDM,
	EDIT_DATE_MDY,
	EDIT_DATE_DMY,
	EDIT_FLOAT
}EDIT_TYPE;

typedef struct _GuiEdit
{
	GuiWidget* base;
	char *font;
	char *string;
	char *str_head;
	char *str_middle;
	char *str_tail;
	image_desc *image[3];
	image_desc *image_g[3][3];
	int alignment;
	int cur_pos;
	int slider_pos;
	int focus_pos;
	int format;
	int maxlen;
	char intaglio;
	char default_intaglio;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
	GuiWidgetSignal *keypress_event;
	GuiWidgetSignal *clicked_event;
	GuiWidgetSignal *change_event;
	GuiWidgetSignal *reachend_event;
}GuiEdit;

extern GuiWidgetOps edit_ops;

#endif

