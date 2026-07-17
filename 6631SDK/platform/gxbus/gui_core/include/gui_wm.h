#ifndef __GUI_WM_H__
#define __GUI_WM_H__

#include "widget.h"
#include "gxcore.h"

__BEGIN_DECLS

int wm_update_contain_widget(GuiWidget* widget);
int wm_redraw_prior_widget(GuiWidget* widget);

int wm_update_all_window(void);
int wm_update_other_window(int end_id, GuiWidget* end_win);
int wm_end_after_window(const char* after_window);
int wm_end_all_window(void);
int wm_draw_window(GuiWidget * window);
int wm_draw(void);

/*Double Buffer*/
int wm_copy_main_surface(void);

int wm_state_set_disable(GuiWidget *widget, int flag);
int wm_state_set_hide(GuiWidget *widget, int flag, void* hide_fill_color);
GuiWidget* wm_state_set_focus(GuiWidget* widget);
GuiWidget* wm_state_get_focus(void);

void wm_state_set_unactive(void);
int wm_unactive_widget(GuiWidget *widget);
int wm_check_resource_window_exist(const char *resource_name);

__END_DECLS

#endif /* __GUI_WM_H__ */

