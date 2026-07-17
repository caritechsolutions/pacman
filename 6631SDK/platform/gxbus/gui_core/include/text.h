#ifndef __TEXT_H__
#define __TEXT_H__

#include "gui_private.h"
#include "widget.h"
#include "gui_timer.h"

#define ROLLING_STEP        (1)
#define ROLLING_RECTANGLE   (8000)
#define ROLL_MAX_LEN		    (ROLLING_RECTANGLE)

#define ROLL_STATE_ROLLING				("ROLLING")
#define ROLL_STATE_END					("END")
#define ROLL_STATE_STOP					("STOP")

typedef enum{
	STATIC,
	ROLL_R2L,
	FIX_ROLL_R2L,
	STEP_ROLL_R2L,
	LONGER_ROLL_R2L,
	ROLL_L2R,
	ROLL_B2U,
	ROLL_U2B,
	AUTOMATIC,
}TEXT_STYLE;

typedef enum _TextRollState
{
	TEXT_STILL,
	TEXT_MOVE
}TextRollState;

typedef enum _TextRollMode {
	TEXT_ROLL_ANTIFLICKER,
	TEXT_ROLL_SMOOTH,
} TextRollMode;

typedef struct _GuiText{
	GuiWidget *base;
	char *string;
	char *font;
	void *surface;
	GuiSurface *surface_roll_text;
	GuiSurface *roll_dst_surface;
	TextRollState roll_state;
	int format;
	int alignment;
	int pos;
	int text_offset;
	int text_length;
	int text_tail;
	int pos_temp;
	int relay;
	int rolling_time;
	int prev_offset;
	int prev_pos;
	int roll_offset;
	int update;
	int step;
	int times;
	int text_roll_exit;
	TextRollMode roll_mode;
	handle_t text_mutex;
	event_list *timer;
	event_list *roll_timer;
	event_list *rolling_stop_timer;
	GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
}GuiText;

extern GuiWidgetOps text_ops;

#endif

