#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gxcore.h>
#include "gui_wm.h"
#include "gui_private.h"
#include "gdi_font.h"
#include "gui_timer.h"
#include "gui_xml_parse.h"
#include "hd_gal.h"
#include "gui.h"

typedef struct {
	signed short x0, y0;
	signed short x1, y1;
}GuiRect;

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define _gxlogd_widget_rect(rect) GUI_Printf("[%d, %d, %d, %d]\n", rect.x, rect.y, rect.x+rect.w-1, rect.y+rect.h-1);
#define _gxlogd_gui_rect(rect) GUI_Printf("[%d, %d, %d, %d]\n", rect.x0, rect.y0, rect.x1, rect.y1);

static void _rect_change(GuiRect* rect_dst, GUI_Rect* rect_src)
{
	if(NULL == rect_dst || NULL == rect_src) return;

	rect_dst->x0 = rect_src->x;
	rect_dst->x1 = rect_src->x + rect_src->w -1;
	rect_dst->y0 = rect_src->y;
	rect_dst->y1 = rect_src->y + rect_src->h -1;
}

static bool _have_intersect(GuiWidget* win0, GuiWidget* win1)
{
	GuiRect intersect;
	GuiRect rect_0;
	GuiRect rect_1;

	intersect.x0 = 0;
	intersect.y0 = 0;
	intersect.x1 = 0;
	intersect.y1 = 0;

	if(NULL == win0 || NULL == win1) return true;

	_rect_change(&rect_0, &win0->rect);
	_rect_change(&rect_1, &win1->rect);


	intersect.x0 = MAX (rect_0.x0, rect_1.x0);
	intersect.y0 = MAX (rect_0.y0, rect_1.y0);
	intersect.x1 = MIN (rect_0.x1, rect_1.x1);
	intersect.y1 = MIN (rect_0.y1, rect_1.y1);

	if(intersect.x1 < intersect.x0 || intersect.y1 < intersect.y0)
	{
		//GUI_Printf("win '%s' and win '%s' no intersect!\n", win0->name, win1->name);
		return false;
	}

	//GUI_Printf("win '%s' and win '%s' have intersect: ", win0->name, win1->name);
	//_gxlogd_gui_rect(intersect);
	return true;
}

bool widget_have_intersect(GuiWidget* widget0, GuiWidget *widget1){
	return (_have_intersect(widget0, widget1));
}

static bool _have_contain(GuiWidget* this, GuiWidget* widget)
{
	GuiRect rect_this_win = {0};
	GuiRect rect_end_win = {0};

	_rect_change(&rect_this_win, &this->rect);
	_rect_change(&rect_end_win, &widget->rect);

	if(rect_this_win.x0 <= rect_end_win.x0 &&
			rect_this_win.y0 <= rect_end_win.y0 &&
			rect_this_win.x1 >= rect_end_win.x1  &&
			rect_this_win.y1 >= rect_end_win.y1 )
	{
		//GUI_Printf("thiswin '%s' contain endwin? YES\n", this->name);
		return true;
	}

	//GUI_Printf("thiswin '%s' contain endwin? NO\n", this->name);

	return false;
}

bool widget_have_contain(GuiWidget* widget0, GuiWidget *widget1){
	return (_have_contain(widget0, widget1));
}

static bool _nowin_under_endwin(GuiWidget* end)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	int i = 0;
	int count = 0;

	count = stack_count(gui_core->win_stack);
	for (i = count - 1; i >= 0; i--)
	{
		widget = (GuiWidget *) stack_get(gui_core->win_stack, i);
		if (widget)
		{
			if(_have_intersect(widget, end))
			{
				break;
			}
		}
	}

	if(-1 == i)
	{
		//GUI_Printf("nowin under endwin? YES, undo\n");
		return true;
	}

	//GUI_Printf("nowin under endwin? NO, check contain\n");
	return false;
}


static int _update_middle_win(int startwin, GuiWidget* start, GuiWidget* end)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	int i = 0;
	int count = 0;

	count = stack_count(gui_core->win_stack);
	for (i = startwin; i < count; i++)
	{
		widget = (GuiWidget *) stack_get(gui_core->win_stack, i);
		if (widget)
		{
			if(WIDGET_IS_WINDOW(widget))
			{
			    widget_set_update(widget, GUI_TRUE);
			}
			else
			{
			    widget->need_update = GUI_TRUE;
			    (gui_core->widget_need_update_count)++;
			    if((0 == gui_str_cmp("progbar", widget->classname)) ||
					    (0 == gui_str_cmp("sliderbar", widget->classname)))
			    {
				    GUI_SetProperty(widget->name, "update", NULL);
			    }
			}
		}
	}

	return 0;
}

static int _update_below_win(GuiWidget* end_win)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	int i = 0, flag = GUI_FALSE;
	int count = 0;

	// GUI_Printf("\n=end win '%s' =", end_win->name);
	//_gxlogd_widget_rect(end_win->rect);

	if(_nowin_under_endwin(end_win))
	{
		WIDGET_SET_PROPERTY(end_win, "clear_window", NULL);
		return 0;
	}

	count = stack_count(gui_core->win_stack);
	for (i = count - 1; i >= 0; i--)
	{
		widget = (GuiWidget *) stack_get(gui_core->win_stack, i);
		if (widget)
		{
			//GUI_Printf("\n=check win '%s' =", widget->name);
			//_gxlogd_widget_rect(widget->rect);
			if(_have_contain(widget, end_win))
			{
				_update_middle_win(i, widget, end_win);
				break;
			}
			if(!flag) {
				GUI_SetProperty(widget->name, "update", NULL);
				if((widget->rect.w >= end_win->rect.w) &&
				   (widget->rect.h >= end_win->rect.h)) {
					flag = GUI_TRUE;
				}
			}
		}
	}

	if(-1 == i)
	{
		//GUI_Printf("\n=nowin contain endwin=\n");
		WIDGET_SET_PROPERTY(end_win, "clear_window", NULL);
		wm_update_all_window();
	}

	return 0;
}



static int _update_above_window(GuiWidget* window)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget* widget = NULL;
	int i = 0;
	int id = -1;

	id = stack_get_id(gui_core->win_stack, (void*)window);
	if(-1 ==  id)
	{
		return 1;
	}

	for(i = id; i < stack_count(gui_core->win_stack); i++)
	{
		widget = (GuiWidget*)stack_get(gui_core->win_stack, i);
		if(widget)
		{
			widget_set_update(widget, GUI_TRUE);
			if((0 == gui_str_cmp("progbar", widget->classname)) ||
			   (0 == gui_str_cmp("window", widget->classname))||
			   (0 == gui_str_cmp("sliderbar", widget->classname)))
			{
				GUI_SetProperty(widget->name, "update", NULL);
			}

		}
	}

	return 0;
}



static int _order_win_stack_by_display_layer(GuiWidget* window)
{
	/*change window stack, by display order, sorry to stack operate*/
	//GuiWidget* widget_stack = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	int id = 0;
	int i = 0;
	int count = 0;

	if(NULL == window)
	{
		return 1;
	}

	count = stack_count(gui_core->win_stack);
	id = stack_get_id(gui_core->win_stack, (void*)window);
	if(-1 == id)
	{
		//GUI_Printf("window '%s' is not in win_stack now\n", window->name);
		return 1;
	}


	/*GUI_Printf("=STACK=1=");
	  for(i = 0; i < count; i ++)
	  {
	  widget_stack = (GuiWidget*)stack_get(gui_core->win_stack, i);
	//GUI_Printf(" '%s', ", widget_stack->name);
	}
	/GUI_Printf("\n");*/


	for(i = id; (i + 1) < count; i++)
	{
		gui_core->win_stack->stack[i] = gui_core->win_stack->stack[i+1];
	}
	if(id != count-1)
	{
		gui_core->win_stack->stack[count-1] = (void*)window;
	}


	/*GUI_Printf("=STACK=2=");
	  for(i = 0; i < count; i ++)
	  {
	  widget_stack = (GuiWidget*)stack_get(gui_core->win_stack, i);
	  GUI_Printf(" '%s', ", widget_stack->name);
	  }
	  GUI_Printf("\n");*/

	return 0;
}

static GuiWidget* _set_focus_on_new_window(GuiWidget* window, GuiWidget* widget)
{
	GuiWidget* old_focus = NULL;
	GuiWidget* old_window = NULL;
	GuiWidget* active_widget = NULL;
	GuiWidget* first_focussable_widget = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	void *surface = NULL;
	int id = 0;

	if(NULL == window)
	{
		GUI_Printf("[GUI][FOCUS] new window is NULL\n");
		return NULL;
	}
	if( WIDGET_ISNOT_WINDOW(window) )
	{
		GUI_Printf("[GUI][FOCUS] '%s' is not a window\n", window->name);
		return NULL;
	}


	/*old window and widget lost focus*/
	old_focus = gui_core->config.focus_widget;
	if(old_focus)
	{
		if(WIDGET_IS_WINDOW(old_focus))
		{
			old_window = old_focus;
		}
		else
		{
			//GUI_Printf("=FOCUS=old widget '%s' lost focus\n", old_focus->name);
			old_focus->state &= ~WIDGET_STATE_FOCUSED;
			old_window = old_focus->parent;
			if(((old_focus->rect.x < window->rect.x) ||
						(old_focus->rect.y < window->rect.y) ||
						((old_focus->rect.x + old_focus->rect.w) > (window->rect.x + window->rect.w)) ||
						((old_focus->rect.y + old_focus->rect.h) > (window->rect.y + window->rect.h))))
			{
				widget_set_update(old_focus, GUI_TRUE);
			}
		}

		if(old_window && (old_window != window))
		{
			//GUI_Printf("=FOCUS=old window '%s' lost focus\n", old_window->name);
			old_window->state &= ~WIDGET_STATE_FOCUSED;
			// TODO: necessary?
			//widget_set_update(old_window, GUI_TRUE);

			/*window signal: do lost_focus*/
			id = stack_get_id(gui_core->win_stack, (void*)old_window);
			if (-1 != id)//when enddialog, not exec this singal
			{
				old_window->ops->set(old_window, "lost_focus", NULL);
			}
		}

	}


	/*new window got focus*/
	if(old_window != window)
	{
		//GUI_Printf("=FOCUS=new window '%s' got focus\n", window->name);
		window->state |= WIDGET_STATE_FOCUSED;
		GUI_GetProperty(window->name, "surface", &surface);
		if(surface)
		{
			window->need_update = GUI_TRUE;
			(gui_core->widget_need_update_count)++;
		}
		else
		{
			widget_set_update(window, GUI_TRUE);
		}

		id = stack_get_id(gui_core->win_stack, (void*)window);
	}

	/*new widget got focus*/
	if(widget && widget_get_focussable(widget))
	{
		//GUI_Printf("=FOCUS=new window's widget '%s' got focus\n", widget->name);
		widget->state |= WIDGET_STATE_FOCUSED;
		widget_set_update(widget, GUI_TRUE);
		gui_core->config.focus_widget = widget;

	}
	/*find widget in new window*/
	else if(window->ops && window->ops->get)
	{
		window->ops->get(window, "get_active_widget", (void*)&active_widget);
		if(active_widget && widget_get_focussable(active_widget))
		{
			//GUI_Printf("=FOCUS=new window's active widget '%s' got focus\n", active_widget->name);
			active_widget->state |= WIDGET_STATE_FOCUSED;
			widget_set_update(active_widget, GUI_TRUE);
			gui_core->config.focus_widget = active_widget;
		}
		else
		{
			window->ops->get(window, "get_first_focussable_widget", (void*)&first_focussable_widget);
			if(first_focussable_widget)
			{
				//GUI_Printf("=FOCUS=new window's first focussable widget '%s' got focus\n", first_focussable_widget->name);
				first_focussable_widget->state |= WIDGET_STATE_FOCUSED;
				widget_set_update(first_focussable_widget, GUI_TRUE);
				window->ops->set(window, "save_active_widget", first_focussable_widget);
				gui_core->config.focus_widget = first_focussable_widget;
			}
			else
			{
				//GUI_Printf("=FOCUS=new window not have focussable widget, so focus on self\n");
				gui_core->config.focus_widget = window;
			}
		}
	}

	_order_win_stack_by_display_layer(window);
	/*window signal: do got_focus*/
	if ((old_window != window) && (-1 != id)) //when createdialog, not exec this singal
	{
		window->ops->set(window, "got_focus", NULL);
	}

	return gui_core->config.focus_widget;
}


static int _check_widget_in_same_window(GuiWidget* widget1, GuiWidget* widget2)
{
	GuiWidget* window1 = NULL;
	GuiWidget* window2 = NULL;

	if(NULL == widget1 || NULL == widget2)
	{
		return GUI_FALSE;
	}

	if(WIDGET_IS_WINDOW(widget1))
	{
		window1 = widget1;
	}
	else
	{
		window1 = widget1->parent;
	}

	if(WIDGET_IS_WINDOW(widget2))
	{
		window2 = widget2;
	}
	else
	{
		window2 = widget2->parent;
	}

	if(0 == strcasecmp(window1->name, window2->name))
	{
		return GUI_TRUE;
	}
	else
	{
		//GUI_Printf("=CHECK='%s' in window '%s', '%s' in  window '%s'\n", widget1->name, window1->name, widget2->name, window2->name);
		return GUI_FALSE;
	}

	return GUI_FALSE;
}


int _hide_check_focus(GuiWidget* widget)
{
	int id = -1;
	GuiWidget* pre_win = NULL;
	GuiCore *gui_core = GUI_GetCurrent();

	if(0 == (widget->state & WIDGET_STATE_FOCUSED))
	{
		return 0;
	}

	/*hide focus window*/
	if(WIDGET_IS_WINDOW(widget))
	{
		id = stack_get_id(gui_core->win_stack, (void*)widget);
		if(id >= 1)
		{
			pre_win = stack_get(gui_core->win_stack, id - 1);
			if(pre_win)
			{
				wm_state_set_focus(pre_win);
			}
			return 0;
		}
		else
		{
			gui_core->config.focus_widget = gui_core->config.root_widget;
			return 0;
		}
	}
	/*hide focus widget*/
	else if(WIDGET_IS_WINDOW(widget->parent))
	{
		wm_state_set_focus(widget->parent);
	}

	return 0;
}


int _hide_window(GuiWidget* widget)
{
	//GUI_Printf("=HIDE=hide window, redraw all above window\n");
	//_update_above_window(widget);
	_update_below_win(widget);//20100510
	_hide_check_focus(widget);

	return 0;
}

int _hide_widget(GuiWidget* widget)
{
	if(widget->parent->state & WIDGET_STATE_FOCUSED)
	{
		//GUI_Printf("=HIDE= hide focused window's widget, redraw contian widget\n");
		wm_update_contain_widget(widget);//20100510
		_hide_check_focus(widget);
	}
	else
	{
		//GUI_Printf("=HIDE= hide unfocused window's widget, redraw all above window\n");
		_update_above_window(widget->parent);
	}

	return 0;
}


int wm_update_contain_widget(GuiWidget* widget)
{
	GuiWidget* prior = NULL;
	GuiWidget* next = NULL;

	if(NULL == widget) return 1;
	if(WIDGET_IS_WINDOW(widget)) return 1;
	//if(WIDGET_ISNOT_WINDOW(widget->parent)) return 1;

	//contianer contain this widget
	if(    (0 == gui_str_cmp(widget->parent->classname, "boxitem"))
			|| (0 == gui_str_cmp(widget->parent->classname, "box"))
			|| (0 == gui_str_cmp(widget->parent->classname, "listitem"))
			|| (0 == gui_str_cmp(widget->parent->classname, "listview")))
	{
		widget_set_update(widget->parent, GUI_TRUE);
		return 0;
	}

	//prior(below) widget contain this widget
	for(prior = widget->prior_sibling; prior != NULL; prior = prior->prior_sibling)
	{
		if(WIDGET_ISNOT_HIDE(prior) && _have_contain(prior, widget))
		{
			widget_set_update(prior, GUI_TRUE);
			if((0 == gui_str_cmp("progbar", prior->classname)) ||
			   (0 == gui_str_cmp("sliderbar", prior->classname)) ||
			   (0 == gui_str_cmp("box", prior->classname)))
			{
				GUI_SetProperty(prior->name, "update", NULL);
			}
			for(next = prior->next_sibling; next != NULL; next = next->next_sibling)
			{
				if(_have_contain(prior, next))
				{
					widget_set_update(next, GUI_TRUE);
					if((0 == gui_str_cmp("progbar", next->classname)) ||
							(0 == gui_str_cmp("sliderbar", next->classname)))
					{
						GUI_SetProperty(next->name, "update", NULL);
					}
				}
				else if(_have_intersect(prior, next))
				{
					//TODO: if next have intersect to other widget, update window?
					widget_set_update(widget->parent, GUI_TRUE);
					if((0 == gui_str_cmp("progbar", widget->parent->classname)) ||
							(0 == gui_str_cmp("sliderbar", widget->parent->classname)))
					{
						GUI_SetProperty(widget->parent->name, "update", NULL);
					}

					return 0;
				}
			}

			return 0;
		}
	}

	//only window contain this widget
	widget_set_update(widget->parent, GUI_TRUE);

	return 0;
}

int wm_redraw_prior_widget(GuiWidget* widget)
{
	GuiWidget* prior = NULL;
	GuiWidget* next = NULL;
	GUI_Rect rect = {0};

	if(NULL == widget) return 1;
	if(WIDGET_IS_WINDOW(widget)) return 1;

	rect = widget->rect;

	for(prior = widget->prior_sibling; prior != NULL; prior = prior->prior_sibling)
	{
		if(_have_contain(prior, widget))
		{
			// TODO:Redraw rectangle
			GUI_SetProperty(prior->name, "update_rect", &rect);
			for(next = prior->next_sibling; next != NULL; next = next->next_sibling)
			{
				if(_have_contain(prior, next))
				{
					// TODO:Redraw rectangle
					//widget_set_update(next, GUI_TRUE);
				}
				else if(_have_intersect(prior, next))
				{
					// TODO:Redraw rectangle
					//GUI_SetProperty(widget->parent->name, "update_rect", &rect);
					return 0;
				}
			}

			return 0;
		}
	}

	return 0;
}


int wm_update_all_window(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	int i = 0;

	GUI_Printf("[window] update all window.\n");

	for (i = 0; i < stack_count(gui_core->win_stack); i++)
	{
		widget = (GuiWidget *) stack_get(gui_core->win_stack, i);
		if (widget)
		{
			widget_set_update(widget, GUI_TRUE);
		}
	}

	return 0;
}


int wm_update_other_window(int end_id, GuiWidget* end_win)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	int i = 0;
	int count = 0;

	/*check above win, if end unfocus win*/
	if (widget_get_window(gui_core->config.focus_widget) != end_win)
	{
		count = stack_count(gui_core->win_stack);
		for (i = end_id; i < count; i++)
		{
			widget = (GuiWidget *) stack_get(gui_core->win_stack, i);
			if (widget)
			{
				if(_have_contain(widget, end_win))
				{
					return 0;
				}
			}
		}
	}

	return _update_below_win(end_win);
}

int wm_end_after_window(const char* after_window)
{
	char* window_name = NULL;
	GuiWidget *window_widget = NULL;
	int after_len = 5;
	GuiWidget *widget = NULL;
	const char *value = NULL;
	int i = 0;
	int id = 0;
	int count = 0;
	GuiCore *gui_core = GUI_GetCurrent();

	if(NULL == after_window) return 1;
	if(strlen(after_window) <= after_len + 1) return 1;

	window_name = (char*)(after_window + after_len + 1);
	if(NULL == window_name) return 1;
	gxlogd("[window] end windows after %s\n", window_name);

	window_widget = gui_get_window(window_name);
	if (NULL == window_widget)
	{
		GUI_Error("[window] %s cant find.\n", window_name);
		return 1;
	}
	if (NULL == window_widget->object)
	{
		GUI_Error("[window] %s is not created\n", window_name);
		return 1;
	}


	/*end after window*/
	id = stack_get_id(gui_core->win_stack, (void*)window_widget);
	if (-1 == id)
	{
		GUI_Error("[window] '%s' is not in win_stack\n", window_name);
		return 1;
	}

	count = stack_count(gui_core->win_stack);
	for (i = id + 1; i < count; i++)
	{
		widget = (GuiWidget *) stack_pop(gui_core->win_stack);
		if (widget)
		{
			if (widget->ops && widget->ops->set)
			{
				widget->ops->set(widget, "clear_window", NULL);
			}
			widget_release(widget);
			if(0 == gui_str_cmp("window", widget->classname))
			{
			    value = widget_get_property(widget, "cache");
			    if((value) && (0 == strcasecmp("disable", value)))
			    {
				widget_destroy(widget);
			    }
			}
		}
	}

	wm_state_set_focus(window_widget);

	return 0;
}

int wm_end_all_window(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *widget = NULL;
	const char *value = NULL;
	int i = 0;
	int count = 0;

	count = stack_count(gui_core->win_stack);
	for (i = 0; i < count; i++)
	{
		widget = (GuiWidget *) stack_pop(gui_core->win_stack);
		if ((widget) && (widget != gui_core->config.root_widget))
		{
			if (widget->ops && widget->ops->set)
			{
				widget->ops->set(widget, "clear_window", NULL);
			}
			widget_release(widget);
			if(0 == gui_str_cmp("window", widget->classname))
			{
			    value = widget_get_property(widget, "cache");
			    if((value) && (0 == strcasecmp("disable", value)))
			    {
				widget_destroy(widget);
			    }
			}
		}
	}

	gui_core->config.focus_widget = gui_core->config.root_widget;

	return 0;
}

extern void *idle_surface;
GuiSurface *swap_surface = NULL;

void layer_lock(void);
void layer_unlock(void);

int wm_copy_main_surface(void)
{
	int ret = 0;
	GAL_Rect rect = {0};
	GxVpuProperty_FlipSurface FlipSurface = {0};
	GuiCore *gui_core = GUI_GetCurrent();
	GuiSurface *surface = NULL;

	if(gui_core != &gui)
		return (0);

	surface = screen;
	if((gui.osd_surface) && (GUI_NO_VIRTUAL_LAYER == gui.layer_mode)) {
		screen = gui.osd_surface;
	} else if(((NULL == screen) || (NULL == gui.osd_surface)) &&
		  (GUI_HAS_VIRTUAL_LAYER == gui.layer_mode)) {
		return (1);
	}

	if(gui.config.enable_double_buffer)
	{
		if(GUI_NO_VIRTUAL_LAYER == gui.layer_mode)
		{
			if(swap_surface){
				rect.x = rect.y = 0;
				rect.w = gui.config.width;
				rect.h = gui.config.height;
				if(screen == hd_get_idle_surface(screen)){
					hd_new_blit_list();
					hd_copy_surface1(screen, &rect, back_screen, &rect);
					hd_end_blit_list();
				}
				hd_free_surface(swap_surface);
				swap_surface = NULL;
			}

			ret = gdi_commit();
			if(NULL == idle_surface)
			{
				return (0);
			}

			FlipSurface.layer = GX_LAYER_OSD;
			FlipSurface.surface = idle_surface;//screen->hw_surface;
			ret = GxAVSetProperty(g_device_handle,
					      vpu_handle,
					      GxVpuPropertyID_FlipSurface,
					      &FlipSurface,
					      sizeof(GxVpuProperty_FlipSurface));
			idle_surface = NULL;

		}
		else if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode)
		{
			rect.x = rect.y = 0;
			rect.w = gui.config.width;
			rect.h = gui.config.height;
			gdi_commit();
			if(NULL == swap_surface)
			{
				layer_lock();
				swap_surface = hd_get_surface(&rect, gui.config.bpp, NULL, GUI_FALSE);
				layer_unlock();
				if(NULL == swap_surface)
				{
					screen = surface;
					return (ret);
				}
			}
			layer_lock();
			if(screen != hd_get_idle_surface(screen)){
				gdi_change_surface((void **)&screen, (void **)&back_screen);
			}
			hd_copy_surface1(screen, &rect, swap_surface, &rect);
			hd_add_blit_element(NULL);
			layer_unlock();
			gxgdi_updatelayers(swap_surface);
			layer_lock();
			hd_stretch_surface(swap_surface, &rect, back_screen, &rect);
			hd_add_blit_element(NULL);
			layer_unlock();
#if 0
			if(32 == gui.config.bpp)
			{
				hd_stretch_surface(swap_surface, &rect, back_screen, &rect);
				hd_add_blit_element(NULL);
			}
#endif
			gdi_commit();
		}
		else if(GUI_GRADIANT_LAYER == gui.layer_mode)
		{
			rect.x = rect.y = 0;
			rect.w = gui.config.width;
			rect.h = gui.config.height;
			gui.layer_mode = gui.prev_layer_mode;
			hd_copy_surface_gradiant(screen,
						 &rect,
						 back_screen,
						 &rect,
						 gui.config.window_fade_mode,
						 gui.config.window_fade_steps);
		}
		else
		{
			memcpy(back_screen->data, screen->data, screen->buffer_size);
//			ret |= gdi_change_surface(&screen_buffer, &back_screen_buffer);
		}
		//ret |= gdi_change_surface(&screen, &back_screen);
	}
	else
	{
		if(GUI_NO_VIRTUAL_LAYER == gui.layer_mode)
		{
			if(back_screen)
			{
				ret = gdi_change_surface((void **)&screen, (void **)&back_screen);
				hd_free_surface(back_screen);
				back_screen = NULL;
			}
			if(swap_surface) {
				hd_free_surface(swap_surface);
				swap_surface = NULL;
			}
		}
		else if(GUI_HAS_VIRTUAL_LAYER == gui.layer_mode)
		{
			rect.x = rect.y = 0;
			rect.w = gui.config.width;
			rect.h = gui.config.height;

			if(NULL == back_screen)
			{
				back_screen = hd_get_surface(&rect, gui.config.bpp, NULL, GUI_TRUE);
				if(NULL == back_screen)
				{
					screen = surface;
					return (1);
				}
				ret = hd_color2index(back_screen, gui.config.osd_trans);
				hd_fillrect(back_screen, &rect, ret);
				ret = 0;
				gal_copy_surface(screen, &rect, back_screen, &rect);
				ret = gdi_change_surface((void **)&screen, (void **)&back_screen);
			}
			if(NULL == swap_surface)
			{
				layer_lock();
				swap_surface = hd_get_surface(&rect, gui.config.bpp, NULL, GUI_FALSE);
				layer_unlock();
				if(NULL == swap_surface)
				{
					screen = surface;
					return (ret);
				}
			}

			gdi_commit();

			layer_lock();
			hd_copy_surface1(screen, &rect, swap_surface, &rect);
			hd_add_blit_element(NULL);
			layer_unlock();
			gxgdi_updatelayers(swap_surface);
			layer_lock();
			hd_stretch_surface(swap_surface, &rect, back_screen, &rect);
			hd_add_blit_element(NULL);
			layer_unlock();
#if 0
			if(32 == gui.config.bpp)
			{
				hd_stretch_surface(swap_surface, &rect, back_screen, &rect);
				hd_add_blit_element(NULL);
			}
#endif
			gdi_commit();
		}
		else
		{
			;
		}
	}

	if(GXCORE_ERROR == check_layer_enable())
	{
		gui.layer_mode = GUI_NO_VIRTUAL_LAYER;
	}
	else
	{
		gui.layer_mode = GUI_HAS_VIRTUAL_LAYER;
	}
	ret |= hd_enable_osd(GUI_TRUE);

	//screen = surface;
	return (ret);
}

static int _check_contain_widget(GuiWidget *widget)
{
    if((0 == gui_str_cmp("button", widget->classname)) ||
       (0 == gui_str_cmp("combobox", widget->classname)) ||
       (0 == gui_str_cmp("edit", widget->classname)) ||
       (0 == gui_str_cmp("menuitem", widget->classname)) ||
       (0 == gui_str_cmp("timeitem", widget->classname)) ||
       (0 == gui_str_cmp("image", widget->classname)) ||
       (0 == gui_str_cmp("file_image", widget->classname)) ||
       (0 == gui_str_cmp("text", widget->classname)))
    {
	return (GUI_FALSE);
    }
    else
    {
	return (GUI_TRUE);
    }
}

int wm_draw_window(GuiWidget * window)
{
    GuiCore *gui_core = GUI_GetCurrent();
    GuiWidget *sibling = NULL, *focus = NULL;
    const char *value = NULL;
    int ret = 0;

    if (NULL == window) return 1;

    if (widget_get_update(window))
    {
	if(window->disable_mirror) {
		ret = gui_set_mirror0(GUI_FALSE);
	}
	widget_draw(window);
	if(window->disable_mirror) {
		gui_set_mirror0(ret);
	}
    }
    else
    {
	/*draw widgets in window */
    value = widget_get_property(window, "accelerate");
	GuiWidget *child = window->first_child;
	while (child)
	{
	    if (widget_get_update(child))
	    {
		if((!_check_contain_widget(child)) &&
           ((value) && (0 == strcasecmp("false", value))))
		{
		    widget_update(window, &(child->rect));
		    sibling = widget_get_firstchild(window);
		    while(sibling != child)
		    {
			if((sibling != child) &&
			   (_have_intersect(sibling, child) ||
			   _have_contain(sibling, child)))
			{
			    widget_update(sibling, &(child->rect));
			}
			sibling = widget_get_nextsibling(sibling);
		    }
		}
		if((gui_core) && (gui_core->config.focus_widget == child)) {
			focus = child;
		} else {
			if(window->disable_mirror) {
				ret = gui_set_mirror0(GUI_FALSE);
			}
			widget_draw(child);
			if(window->disable_mirror) {
				gui_set_mirror0(ret);
			}
		}
	    }

	    child = child->next_sibling;
	}

	if(focus) {
		if(window->disable_mirror) {
			ret = gui_set_mirror0(GUI_FALSE);
		}
		widget_draw(focus);
		if(window->disable_mirror) {
			gui_set_mirror0(ret);
		}
	}
    }

    return 0;
}

int wm_draw(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int count = 0, draw_count = 0;
	int i = 0, j = 0, need_update = 0;
	GuiWidget *cur_window = NULL;
	GuiWidget *ontop_window = NULL, *other_window = NULL;

	if(gui_core->stop_update) {
		return (0);
	}

	draw_count = gui_core->widget_need_update_count;
	if (0 == gui_core->widget_need_update_count) {
		if(GUI_FALSE == gui.layer_need_update) {
			wm_copy_main_surface();
			return 0;
		}
	}

	if(gui_core->win_stack)
	{
		count = stack_count(gui_core->win_stack);
		for (i = 0; i < count; i++)
		{
			cur_window = (GuiWidget *) stack_get(gui_core->win_stack, i);
			if (NULL == cur_window) return 0;

			need_update = widget_get_update(cur_window);

			wm_draw_window(cur_window);
		}
	}

	if((gui_core != &gui) && (0 == count))
	{
		GXGDI_LayerClear(gui_core->layer_name, NULL);
	}

	if(gui_core->ontop_stack)
	{
		count = stack_count(gui_core->ontop_stack);
		for(i = 0; i < count; i++)
		{
			ontop_window = (GuiWidget *) stack_get(gui_core->ontop_stack, i);
			if(NULL == ontop_window)
			{
				gdi_commit();
				wm_copy_main_surface();
				return (0);
			}

			gdi_commit();
			if(ontop_window->object) {
				widget_draw(ontop_window);
			}
			gdi_commit();
		}
	}

	if(0 != gui_core->widget_need_update_count)
		gui_core->widget_need_update_count -= draw_count;

	gdi_commit();
	// Secondary GUI
	if((gui_core != &gui) && (gui_core->layer_name))
	{
		GxGDI_LayerUpdate(gui_core->layer_name);
	}
	// Main GUI
	else
	{
		wm_copy_main_surface();
		gdi_flip();
	}

	gdi_post();
	return 0;
}

int wm_state_set_disable(GuiWidget *widget, int flag)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget *child = NULL;

	if(NULL == widget || NULL == widget->object)
	{
		return (1);
	}
	if(WIDGET_IS_WINDOW(widget))
	{
		GUI_Printf("[GUI][DISABLE] cant disable window '%s', undo\n", widget->name);
		return 1;
	}
	else if(WIDGET_ISNOT_WINDOW(widget->parent))
	{
		//add for shencz's special widget
		if(gui_str_cmp(widget->parent->classname, "box") &&
				gui_str_cmp(widget->parent->classname, "boxitem") )
		{
			GUI_Printf("[GUI][DISABLE] '%s''s parent is not a window/box/boxitem, undo\n", widget->name);
			return 1;
		}
	}


	if(GUI_TRUE == flag)
	{
		//widget->state |= WIDGET_STATE_DISABLE;
		if( 0 == (widget->state & WIDGET_STATE_DISABLE) )
		{
			widget->state |= WIDGET_STATE_DISABLE;
			widget_set_update(widget, GUI_TRUE);

			child = widget_get_firstchild(widget);
			while(child)
			{
				child->state |= WIDGET_STATE_DISABLE;
				child = widget_get_nextsibling(child);
			}

			if(gui_core->config.focus_widget == widget)
			{
				//gxlogd("=DISABLE= disable focus widget, set focus on other widget\n");
				wm_state_set_focus(widget->parent);
			}

			return (0);
		}
	}
	else
	{
		if(widget->state & WIDGET_STATE_DISABLE)
		{
			widget->state &= ~WIDGET_STATE_DISABLE;
			widget_set_update(widget, GUI_TRUE);

			child = widget_get_firstchild(widget);
			while(child)
			{
				child->state &= ~WIDGET_STATE_DISABLE;
				child = widget_get_nextsibling(child);
			}
			widget_children_set_update(widget, GUI_TRUE);//listview box update all
			return (0);
		}
	}

	return 0;
}

int wm_state_set_hide(GuiWidget *widget, int flag, void* hide_fill_color)
{
	GuiCore *gui_core = GUI_GetCurrent();
	//GUI_Printf("=HIDE=set widget '%s' flag:%d\n", widget->name, flag);
	if(NULL == widget || NULL == widget->object)
	{
		return 1;
	}

	if(GUI_TRUE == flag)
	{
		if( 0 == (widget->state & WIDGET_STATE_HIDE) )
		{
			widget->state |= WIDGET_STATE_HIDE;

			/*hide region fill with usr specify color*/
			if((!gui_core->stop_update) && (NULL != hide_fill_color))//bean 20100510
			{
				//wm_copy_main_surface();
				gal_fillrect(screen, (GAL_Rect *)&widget->rect, gal_color2index(screen, *(int*)hide_fill_color));
				wm_copy_main_surface();
				_hide_check_focus(widget);
				gui_core->widget_need_update_count++;
				return 0;
			}


			/*redraw hide region*/
			if(WIDGET_IS_WINDOW(widget))
			{
				_hide_window(widget);
			}
			else if(WIDGET_IS_WINDOW(widget->parent))
			{
				_hide_widget(widget);
			}
			else if(WIDGET_IS_BOXITEM(widget->parent))
			{
				widget_set_update(widget->parent, GUI_TRUE);
			}
			else
			{
				//GUI_Printf("=HIDE=widget '%s''s parent is not a window, undo\n", widget->name);
				gui_core->widget_need_update_count++;
				return 1;
			}

			gui_core->widget_need_update_count++;
			return (0);
		}
	}
	else
	{
		if(widget->state & WIDGET_STATE_HIDE)
		{
			widget->state &= ~WIDGET_STATE_HIDE;
			widget_set_update(widget, GUI_TRUE);
			widget_children_set_update(widget, GUI_TRUE);//listview box update all
			gui_core->widget_need_update_count++;
			return (0);
		}
		else
		{
		    return (0);
		}
	}

	//gui_core->widget_need_update_count++;
	return (1);
}

int wm_get_cache_context(GuiWidget *widget)
{
	GuiCore *gui_core = GUI_GetCurrent();
	const char *value = NULL;

	if(NULL == widget) {
		return (GUI_TRUE);
	}

	value = widget_get_property(widget, (const char *)CACHE_CONTEXT_STR);
	if((value) && (0 == strcasecmp("disable", value))) {
		return (GUI_FALSE);
	} else {
		return (gui_core->config.uncache_context ? GUI_FALSE : GUI_TRUE);
	}
}

int wm_unactive_widget(GuiWidget *widget)
{
	if(wm_get_cache_context(widget) == GUI_TRUE) {
		return (0);
	}

	if(widget && widget->ops->unactive) {
		widget->ops->unactive(widget);
	}

	return (0);
}

void wm_state_set_unactive(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int count = 0, i = 0;
	GuiWidget *widget = NULL;

	if(NULL == gui_core->win_stack) {
		return;
	}

	count = stack_count(gui_core->win_stack);
	for(i = 0; i < count; i++) {
		widget = (GuiWidget *)stack_get(gui_core->win_stack, i);
		if(widget->checked == GUI_TRUE) {
			continue;
		}
		if((widget) && (GUI_FALSE == widget->active)) {
			wm_unactive_widget(widget);
		}
		widget->checked = GUI_TRUE;
	}
}

int wm_check_resource_window_exist(const char *resource_name)
{
	GuiCore *gui_core = GUI_GetCurrent();
	int i = 0, count = 0;
	GuiWidget *window = NULL;

	if(NULL == resource_name) {
		return (GUI_FALSE);
	}

	count = stack_count(gui_core->win_stack);
	for(i = 0; i < count; i++) {
		window = (GuiWidget *)stack_get(gui_core->win_stack, i);
		if((window->from_resource) && (0 == strcasecmp(window->from_resource, resource_name))) {
			gxlogd("You must End %s dialog from %s resource !\n", window->name, window->from_resource);
			return (GUI_TRUE);
		}
	}

	return (GUI_FALSE);
}

GuiWidget *_get_window(GuiWidget *widget)
{
	if(widget) {
		if((widget->classname) && (0 == strcasecmp("window", widget->classname))) {
			return (widget);
		} else {
			while(widget) {
				widget = widget_get_parent(widget);
				if((widget) &&
				   ((0 == gui_str_cmp("window", widget->classname)) ||
				    (0 == gui_str_cmp("prompt", widget->classname)))) {
					return (widget);
				}
			}
		}
	}

	return (NULL);
}

static void _unfocus_special_process(GuiWidget *widget)
{
	if(widget) {
		if(0 == gui_str_cmp("listview", widget->classname)) {
			GUI_SetProperty(widget->name, "stop_rolling", NULL);
		}
	}
}

static void _focus_special_process(GuiWidget *widget)
{
	if(widget) {
		if(0 == gui_str_cmp("listview", widget->classname)) {
			GUI_SetProperty(widget->name, "enable_roll", NULL);
		}
	}
}

GuiWidget*  wm_state_set_focus(GuiWidget* new_widget)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiWidget* old_widget = NULL;
	GuiWidget* widget_focus = NULL;
	GuiWidget* parent = NULL;

	old_widget = gui_core->config.focus_widget;

	if(NULL == new_widget)
	{
		return NULL;
	}
	if(old_widget == new_widget)
	{
		return old_widget;
	}

	parent = _get_window(old_widget);
	widget_set_active(parent, GUI_FALSE);
	parent = _get_window(new_widget);
	widget_set_active(parent, GUI_TRUE);
	if(gui_core->config.root_widget == new_widget)
	{
		gui_core->config.focus_widget = new_widget;
		return new_widget;
	}
	if(NULL == new_widget->object)
	{
		//GUI_Printf("=FOCUS=widget '%s' is not created\n", new_widget->name);
		return NULL;
	}


	/**********LEVEL-1**************set focus on window************************/
	if(WIDGET_IS_WINDOW(new_widget))
	{
		if(GUI_TRUE == _check_widget_in_same_window(old_widget, new_widget))
		{
			if(widget_get_focussable(old_widget))//focus maybe hide or disable
			{
				_unfocus_special_process(old_widget);
				_focus_special_process(new_widget);
				return old_widget;
			}
		}

		widget_focus = _set_focus_on_new_window(new_widget, NULL);
		_unfocus_special_process(old_widget);
		_focus_special_process(new_widget);
		return widget_focus;
	}



	/***********LEVEL-2*************set focus on widget************************/
	if(WIDGET_ISNOT_WINDOW(new_widget->parent) )
	{
		GUI_Printf("[GUI][FOCUS] widget '%s''s parent is not a window, undo\n", new_widget->name);
		return NULL;
	}
	if(0 == widget_get_focussable(new_widget))
	{
		GUI_Printf("[GUI][FOCUS] widget '%s' is unfocussable, undo\n", new_widget->name);
		return NULL;
	}


	if(GUI_FALSE == _check_widget_in_same_window(old_widget, new_widget))
	{
		//active widget's parent window
		widget_focus = _set_focus_on_new_window(new_widget->parent, new_widget);
		new_widget->parent->ops->set(new_widget->parent, "save_active_widget", new_widget);
		_unfocus_special_process(old_widget);
		_focus_special_process(new_widget);
		return widget_focus;
	}


	/*old widget lost focus*/
	if(WIDGET_ISNOT_WINDOW(old_widget))
	{
		const char *value = NULL;

		value = widget_get_property(old_widget, "virtual_focus");
		//GUI_Printf("=FOCUS=old widget '%s' lost focus\n", old_widget->name);
		if((NULL == value) ||
		   (value && (0 != strcasecmp("enable", value))))
		{
		    old_widget->state &= ~WIDGET_STATE_FOCUSED;
		}
		widget_set_update(old_widget, GUI_TRUE);
	}

	/*new widget got focus*/
	//GUI_Printf("=FOCUS=new widget '%s' got focus\n", new_widget->name);
	new_widget->state |= WIDGET_STATE_FOCUSED;
	widget_set_update(new_widget, GUI_TRUE);

	/*parent window save active widget*/
	parent = widget_get_parent(new_widget);
	parent->ops->set(parent, "save_active_widget", new_widget);

	gui_core->config.focus_widget = new_widget;
	_unfocus_special_process(old_widget);
	_focus_special_process(new_widget);

	return new_widget;
}

GuiWidget* wm_state_get_focus(void)
{
	GuiCore *gui_core = GUI_GetCurrent();
	return gui_core->config.focus_widget;
}

