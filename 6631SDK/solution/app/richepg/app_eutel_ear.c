#include "app.h"
#include "app_windows.h"
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "app_eutel_control.h"
#include "app_eutel_common.h"

typedef struct {
	const char *pop;
	const char *prompt;
	const char *image;
	const char *key;
	char *path;
	event_list *timer;
	int x;
	int y;
} EutelEarOps;

static EutelEarOps s_eutel_ear_ops = {
	.pop = "richepg_ear_logo_pop",
	.prompt = "richepg_ear_logo_prompt",
	.image = "richepg_ear_logo_image",
	.key = "richepg_ear_logo_key",
	.path = NULL,
	.timer = NULL,
	.x = 600,
	.y = 240
};

void app_eutel_ear_hide(void)
{
	EutelEarOps *ops = &s_eutel_ear_ops;
	if(GUI_CheckPrompt(ops->pop) == GXCORE_SUCCESS)
	{
		GUI_EndPrompt(ops->pop);
		APP_FREE(ops->path);
	}
}

void app_eutel_ear_show(void)
{
	EutelEarOps *ops = &s_eutel_ear_ops;
	char *path = eutel_cur_ear_logo_path_get();

	if (path)
	{
		if (ops->path && strcmp(ops->path, path) == 0)
		{
			if (GUI_CheckPrompt(ops->pop) != GXCORE_SUCCESS)
			{
				GUI_CreatePrompt(ops->x, ops->y, ops->pop, ops->prompt, "", "ontop");
				GUI_SetProperty(ops->image, "img", (void *)ops->key);
			}
		}
		else
		{
			APP_FREE(ops->path);
			ops->path = GxCore_Strdup(path);
			gal_add_key_path(ops->key, path);
			if (GUI_CheckPrompt(ops->pop) != GXCORE_SUCCESS)
			{
				GUI_CreatePrompt(ops->x, ops->y, ops->pop, ops->prompt, "", "ontop");
			}
			GUI_SetProperty(ops->image, "img", (void *)ops->key);
		}
	}
	else
	{
		app_eutel_ear_hide();
	}
}

static int _eutel_ear_timer_cb(void *userdata)
{
	const char *wnd = NULL;
	wnd = GUI_GetFocusWindow();
	if(wnd && (strcasecmp(WND_EUTEL_CHINFO, wnd) == 0))// || strcasecmp(WND_EUTEL_RCHINFO, wnd) == 0))
	{
		app_eutel_ear_show();
	}
	else
	{
		app_eutel_ear_hide();
	}

	return 0;
}

void app_eutel_ear_create(void)
{
	EutelEarOps *ops = &s_eutel_ear_ops;

/*
	if (app_richepg_gui_reverse())
	{
		ops->x = 130;
	}
	else
	{
		ops->x = 600;
	}
*/

	APP_TIMER_ADD(ops->timer, _eutel_ear_timer_cb, 100, TIMER_REPEAT);
}

void app_eutel_ear_destroy(void)
{
	EutelEarOps *ops = &s_eutel_ear_ops;
	APP_TIMER_REMOVE(ops->timer);
	app_eutel_ear_hide();
}
#endif

