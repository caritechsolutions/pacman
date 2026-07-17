#ifndef __PROMPT_H__
	#define __PROMPT_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _GuiPrompt
{
    GuiWidget *base;
    void *surface;
	GuiSurface *locale_surface;
	char *layer_name;
    GuiWidget *active_widget;
    int end_draw;
    GuiWidgetSignal *create_event;
	GuiWidgetSignal *destroy_event;
}GuiPrompt;

extern GuiWidgetOps prompt_ops;

#ifdef __cplusplus
}
#endif

#endif  /*__PROMPT_H__*/

