#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "all_widget.h"
#include "gdi_font.h"
#include "gui_timer.h"
#include "gui_script.h"
#include "hd_gal.h"
#include "hd_system.h"
#include "service/gxgui_view.h"
#include "gxmsg.h"
//#include "vq_malloc.h"
#include "image_cache.h"
#include "gui.h"
#include "module/config/gxconfig.h"

GuiCore gui;
static handle_t event_mutex_id = 0;
static handle_t virtual_gui_mutex_id = 0;
static int lock_flag = 0;
static int poll_key_flag;    /* For GUI Consonle thread and Message receive thread mutex */

extern int GxGuiViewServiceRecvMsgStop(void);
extern int GxGuiViewServiceRecvMsgStart(void);

#define GLOBAL_NOTICE_PROMPT		    "prompt_gui_warning_notice"
#define GLOBAL_SINGLE_PROMPT		    "prompt_gui_warning_single"
#define GLOBAL_DOUBLE_PROMPT		    "prompt_gui_warning_double"

static void gui_signal_release(void *data)
{
	if (data)
	{
		GUI_FREE(data);
	}
}

static void gui_table_release(void *data)
{
	if (data)
	{
		GUI_FREE(data);
	}
}

status_t GUI_Init(const char* theme_config)
{
	status_t ret = GXCORE_SUCCESS;
	int mem_size = GUIPOOL_SIZE;
	GuiSignal* root_signal = NULL;
	GuiSignal* top_signal = NULL;
	GxTime start;
	GxTime stop;

	GxCore_GetLocalTime(&start);

	//memset(&gui, 0, sizeof(GuiCore) );
	GxBus_ConfigGetInt(GUI_MEM_POOL_SIZE, &mem_size, GUIPOOL_SIZE);
	GuiPool_Init(mem_size);
	hd_gxavdev_init();
	GxBus_ConfigGetInt(GUI_SCHEDULE_TIME, &(gui.config.schedule_time), 60);

	//hd_enable_video(GUI_FALSE);

	widget_ops_init();
	gdi_encoding_init();

	if(NULL == gui.config.signal_table) {
		gui.config.signal_table = hash_new(40,gui_signal_release);
		if (gui.config.signal_table == NULL) return (GXCORE_ERROR);
	}
	top_signal = (GuiSignal*)hash_get(gui.config.signal_table, "top");
	if(top_signal)
		gui.top_event = top_signal->handler;

	root_signal = (GuiSignal*)hash_get(gui.config.signal_table, "root");
	if(root_signal)
		gui.root_event = root_signal->handler;

	gui.config.root_widget = widget_new("root", "window", NULL);
	GUI_ASSERT(NULL != gui.root_widget);
	gui.config.freedom_widget = widget_new("freedom_widget", "boxitem", NULL);
	GUI_ASSERT(NULL != gui.freedom_widget);
	gui.config.style_widget = widget_new("style_widget", "window", NULL);
	GUI_ASSERT(NULL != gui.freedom_widget);

	gui.config.focus_widget = gui.config.root_widget;
	//gui.signal_hash = hash_new(1, signal_Free);
	gui.win_stack   = stack_new(NULL);
	gui.ontop_stack = stack_new(NULL);
	gui.uncache_stack = stack_new(NULL);

	ret = gui_script_parse(&gui, theme_config);
	if(GXCORE_SUCCESS != ret)return (GXCORE_ERROR);

	ret = gxgdi_init();
	if(0 != ret)return GXCORE_ERROR;

	GxCore_GetLocalTime(&stop);
	GUI_Printf_Blue("\n[GUI] init finish! time take %d second\n\n", stop.seconds -= start.seconds);

	image_cache_thread(gui.config.image_cache);
	gui.current = &gui;
	gui.gui_num = 0;

	return GXCORE_SUCCESS;
}

static void _copy_global_parameter(GuiCore *gui_core)
{
	gui_core->config.gui_trans = gui.config.gui_trans;
	gui_core->config.osd_trans = gui.config.osd_trans;
	gui_core->config.chip_id = gui.config.chip_id;
}

status_t GUI_InitByName(const char *name, const char* theme_config)
{
	status_t ret = GXCORE_SUCCESS;
	GuiCore *gui_core = NULL;
	char *root_name = NULL;
	char value[100] = {0};

	if((NULL == name) || (NULL == theme_config))
	{
		return (GXCORE_ERROR);
	}

	if(0 == virtual_gui_mutex_id)
	{
		GxCore_MutexCreate(&virtual_gui_mutex_id);
		if(0 == virtual_gui_mutex_id)
		{
			return (GXCORE_ERROR);
		}
	}

	if((gui.gui_table) &&
	   (hash_get(gui.gui_table, name)))
	{
		return (GXCORE_SUCCESS);
	}

	if((NULL == gui.config.theme_path) ||
	   ((gui.config.theme_path) && (0 == strcasecmp(gui.config.theme_path, theme_config))))
	{
		return (GXCORE_ERROR);
	}

	gui_core = (GuiCore *)GUI_MALLOCZ(sizeof(GuiCore));
	if(NULL == gui_core)
	{
		return (GXCORE_ERROR);
	}
	gui_core->config.name = GxCore_Strdup(name);
	if(NULL == gui_core->config.signal_table) {
		gui_core->config.signal_table = hash_new(40,gui_signal_release);
		if (gui_core->config.signal_table == NULL) return (GXCORE_ERROR);
	}

	// Script Parse
	gui_core->config.image_cache = GxCore_Mallocz(sizeof(ImageCache));
	image_cache_init(gui_core->config.image_cache, gui.fb.size);

	// Window and Widget management
	root_name = GUI_MALLOCZ(strlen(name) + 10);
	if(NULL == root_name)
	{
		return (GXCORE_ERROR);
	}
	strcpy(root_name, name);
	strcat(root_name, "_window");
	GUI_ASSERT(NULL != gui_core->root_widget);
	gui_core->config.root_widget = widget_new(root_name, "window", NULL);
	gui_core->config.focus_widget = gui_core->config.root_widget;

	if(!gui_core->rcv_msg_id)
		gui_core->rcv_msg_id = GxCore_ThreadGetId();
	else if(!gui_core->console_id)
		gui_core->console_id = GxCore_ThreadGetId();

	GxCore_MutexLock(virtual_gui_mutex_id);
	if(NULL == gui.gui_table)
	{
		gui.gui_table = hash_new(40,gui_table_release);
		if(NULL == gui.gui_table)
		{
			GxCore_MutexUnlock(virtual_gui_mutex_id);
			return GXCORE_ERROR;
		}
	}

	hash_add(gui.gui_table, name, gui_core);

	if(NULL == gui.gui_stack)
	{
		gui.gui_stack = stack_new(NULL);
		if(NULL == gui.gui_stack)
		{
			GxCore_MutexUnlock(virtual_gui_mutex_id);
			return (GXCORE_ERROR);
		}
	}

	stack_push(gui.gui_stack, gui_core);
	gui.current = gui_core;

	// Parse script
	ret = gui_script_parse(gui_core, theme_config);
	if(GXCORE_SUCCESS != ret)
	{
		GxCore_MutexUnlock(virtual_gui_mutex_id);
		return (GXCORE_ERROR);
	}
	//Fix global parameter
	_copy_global_parameter(gui_core);

	sprintf(value, "[%d,%d,%d,%d]",
			0,
			0,
			gui_core->config.width,
			gui_core->config.height);
	widget_set_property(gui_core->config.root_widget, "rect", value);
	widget_get_rect(gui_core->config.root_widget, &(gui_core->config.root_widget->rect));
	memset(value, 0, 100);

	gui_core->win_stack   = stack_new(NULL);
	gui_core->ontop_stack = stack_new(NULL);

	// Hardware Initial
	if(0 != gxgdi_virtual_init(gui_core))
	{
		GxCore_MutexUnlock(virtual_gui_mutex_id);
		return GXCORE_ERROR;
	}

	gui.gui_num++;
	GUI_FREE(root_name);
	GxCore_MutexUnlock(virtual_gui_mutex_id);

	return (GXCORE_SUCCESS);
}

status_t GUI_Quit(void)
{
	gui_script_parse_freedata(&gui);

	if(gui.config.signal_table) {
		hash_release(gui.config.signal_table);
		gui.config.signal_table = NULL;
	}
	stack_release(gui.win_stack);
	gui.win_stack = NULL;

	remove_timer_list();

	widget_ops_clean();

	hd_quit();
	GxCore_Free(gui.config.image_cache);

	GuiPool_Destroy();

#ifdef MEMWATCH
	mwAbort();
#endif

	return GXCORE_SUCCESS;
}

status_t GUI_QuitByName(const char *name)
{
	GuiCore *gui_core = NULL, *tmp_gui = NULL;
	int i = 0;

	if(NULL == gui.gui_table)
	{
		return GXCORE_ERROR;
	}

	if(0 == virtual_gui_mutex_id)
	{
		GxCore_MutexCreate(&virtual_gui_mutex_id);
		if(0 == virtual_gui_mutex_id)
		{
			return (GXCORE_ERROR);
		}
	}

	GxCore_MutexLock(virtual_gui_mutex_id);
	gui_core = hash_get(gui.gui_table, name);
	if(NULL == gui_core)
	{
		GxCore_MutexUnlock(virtual_gui_mutex_id);
		return GXCORE_ERROR;
	}

	gxgdi_virtual_destroy(gui_core);
	gui_script_parse_freedata(gui_core);
	stack_release(gui_core->win_stack);
	gui_core->win_stack = NULL;
	stack_release(gui_core->ontop_stack);
	gui_core->ontop_stack = NULL;

	remove_timer_list();
	GUI_FREE(gui_core->config.image_cache);

	if(gui.gui_stack)
	{
		for(i = 0; i < stack_count(gui.gui_stack); i++)
		{
			tmp_gui = stack_get(gui.gui_stack, i);
			if((tmp_gui) && (0 == strcasecmp(name, tmp_gui->config.name)))
			{
				stack_del(gui.gui_stack, i);
				break;
			}
		}

		if(stack_peek(gui.gui_stack))
		{
			gui.current = stack_peek(gui.gui_stack);
		}
		else
		{
			gui.current = &gui;
			stack_release(gui.gui_stack);
			gui.gui_stack = NULL;
		}
	}
	else
	{
		gui.current = &gui;
	}

	GUI_FREE(gui_core->config.name);
	hash_drop(gui.gui_table, name);
	GxCore_HwCleanCache();
	gui.gui_num--;

	GxCore_MutexUnlock(virtual_gui_mutex_id);

	return GXCORE_SUCCESS;
}

static int _transfer_send_message(GUI_Event *event)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GuiCore *gui_core = gui.current;//GUI_GetCurrent();
	GxMessage *msg = NULL;
	GxMsgProperty_GUI_Transfer_Message *transfer_msg = NULL;

	if((NULL == gui_core) ||
	   (&gui == gui_core) ||
	   (gui_core == GUI_GetCurrent()) ||
	   ((&gui != GUI_GetCurrent()) && (gui.current != &gui)))
	{
		return (ret);
	}

	if(GUI_SERVICE_MSG == event->type)
	{
		msg = (GxMessage*)(event->msg.service_msg);
		if(GXMSG_GUI_TRANSFER_MESSAGE == msg->msg_id)
		{
			if(gui_core != &gui)
				return EVENT_TRANSFER_KEEPON;
			else
				return EVENT_TRANSFER_STOP;
		}
	}

	if(((GUI_KEYDOWN == event->type) && (REFUSE_MESSAGE == gui_core->config.key_mode)) ||
	   ((GUI_SERVICE_MSG == event->type) && (REFUSE_MESSAGE == gui_core->config.msg_mode)))
	{
		return EVENT_TRANSFER_KEEPON;
	}

	msg = GxBus_MessageNew(GXMSG_GUI_TRANSFER_MESSAGE);
	if (msg == NULL){
		return EVENT_TRANSFER_STOP;
	}

	transfer_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_GUI_Transfer_Message);
	transfer_msg->type = GXGUI_TRANSFER_RUNNING;
	transfer_msg->event = *event;
	GxBus_MessageSend(msg);
	if(((GUI_KEYDOWN == event->type) && (BLOCK_MESSAGE == gui_core->config.key_mode)) ||
	   ((GUI_SERVICE_MSG == event->type) && (BLOCK_MESSAGE == gui_core->config.msg_mode)))
	{
		ret = EVENT_TRANSFER_STOP;
	}

	return (ret);
}

static int _transfer_receive_message(GUI_Event *event)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GuiCore *gui_core = GUI_GetCurrent();
	GxMessage *msg = NULL;
	GxMsgProperty_GUI_Transfer_Message *transfer_msg = NULL;

	if(gui_core != gui.current)
	{
		if(gui_core == &gui)
		{
			return EVENT_TRANSFER_KEEPON;
		}
		return EVENT_TRANSFER_STOP;
	}

	if(GUI_SERVICE_MSG != event->type)
	{
		return EVENT_TRANSFER_KEEPON;
	}

	msg = (GxMessage*)(event->msg.service_msg);
	if((GXMSG_GUI_TRANSFER_MESSAGE == msg->msg_id) &&
			(gui_core == &gui))
	{
		return EVENT_TRANSFER_STOP;
	}

	if(GXMSG_GUI_TRANSFER_MESSAGE != msg->msg_id)
	{
		return EVENT_TRANSFER_KEEPON;
	}
	transfer_msg = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_GUI_Transfer_Message);
	if(NULL == transfer_msg)
	{
		return EVENT_TRANSFER_STOP;
	}

	*event = transfer_msg->event;

	return (ret);
}

void GUI_CurrentLink(const char* name)
{
	GuiCore *gui_core = NULL;

	if(NULL == name)
	{
		gui_core = gui.current;
	}
	else if(gui.gui_table)
	{
		gui_core = hash_get(gui.gui_table, name);
	}

	if(NULL == gui_core)
		return;

	if((0 == gui_core->console_id) &&
	   (GxCore_ThreadGetId() != gui_core->rcv_msg_id))
	{
		gui_core->console_id = GxCore_ThreadGetId();
		return;
	}

	if((0 == gui_core->rcv_msg_id) &&
	   (GxCore_ThreadGetId() != gui_core->console_id))
	{
		gui_core->rcv_msg_id = GxCore_ThreadGetId();
		return;
	}

	return;
}

static void check_uncache_window(gx_stack_t *win_stack)
{
	GuiWidget *widget = NULL;
	gx_stack_t *tmp_stack = NULL;

	if(NULL == win_stack) {
		return;
	}

	tmp_stack = stack_new(NULL);
	if(stack_count(win_stack)) {
		widget = (GuiWidget *)stack_pop(win_stack);
		while(widget) {
			if(NULL == widget->object) {
				if(widget == gui.config.focus_widget) {
					stack_push(tmp_stack, (void *)widget);
				} else {
					widget_destroy(widget);
				}
			}
			widget = stack_pop(win_stack);
		}

		widget = (GuiWidget *)stack_pop(tmp_stack);
		while(widget) {
			if(NULL == widget->object) {
				stack_push(win_stack, (void *)widget);
			}
			widget = stack_pop(tmp_stack);
		}
	}
	stack_release(tmp_stack);
}

status_t GUI_Exec(void)
{
	GUI_Event event = { 0 };
	int ret = 0;
	GuiCore *gui_core = GUI_GetCurrent();

	if(0 == event_mutex_id)
	{
		GxCore_MutexCreate(&event_mutex_id);
		if(0 == event_mutex_id)
		{
			return (GXCORE_ERROR);
		}
	}

	if(gui.event_cache.type != GUI_NOEVENT)
	{
		GUI_Event event = gui.event_cache;
		gui.event_cache.type = GUI_NOEVENT;
		GUI_ExecEvent(&event);
	}

	if((!poll_key_flag) && (gui_core == &gui))
	{
		ret = gui_poll_event(&event);
		if (GUI_NOEVENT != ret)
		{
			if(event.type == GUI_KEYDOWN)
			{
				GUI_Printf("[KEY]scancode:0x%x\n", event.key.scancode);
			}

			GUI_ExecEvent(&event);
			gui_delay(20);
		}
	}
	else
	{
		gui_delay(60);
	}

	GxCore_MutexLock(event_mutex_id);
	lock_flag++;
	ret = timer_exec();
	check_uncache_window(gui_core->uncache_stack);
	GxCore_MutexUnlock(event_mutex_id);
	lock_flag--;

	gdi_lock();
	wm_draw();
	wm_state_set_unactive();
	gdi_unlock();

#if defined(DEBUG) && defined(LINUX_OS)
	image_cache_log(gui.config.image_cache, "/tmp/frame_cache");
	GUI_GetMemInfo("/tmp/fb_memory");
#endif

	return GXCORE_SUCCESS;
}

status_t GUI_LoopEvent(void)
{
	GUI_Event event = { 0 };
	int ret = 0;
	int flag = 0;

	if(gui.event_cache.type != GUI_NOEVENT)
	{
		GUI_Event event = gui.event_cache;
		gui.event_cache.type = GUI_NOEVENT;
		if(lock_flag)
		{
			GxCore_MutexUnlock(event_mutex_id);
			lock_flag--;
			flag = 1;
		}
		GUI_ExecEvent(&event);
		if(flag == 1)
		{
			GxCore_MutexLock(event_mutex_id);
			lock_flag++;
		}
	}

	ret = gui_poll_event(&event);
	if (GUI_NOEVENT != ret)
	{
		if(event.type == GUI_KEYDOWN)
		{
			GUI_Printf("[KEY]scancode:0x%x\n", event.key.scancode);
		}
		if(lock_flag)
		{
			GxCore_MutexUnlock(event_mutex_id);
			lock_flag--;
			flag = 1;
		}
		GUI_ExecEvent(&event);
		if(flag == 1)
		{
			GxCore_MutexLock(event_mutex_id);
			lock_flag++;
		}
	}
	else
	{
		if(lock_flag)
		{
			GxCore_MutexUnlock(event_mutex_id);
			lock_flag--;
			flag = 1;
		}
		gui_delay(60);
		if(flag == 1)
		{
			GxCore_MutexLock(event_mutex_id);
			lock_flag++;
		}
	}

	ret = timer_exec();
	if (ret)
	{
		GUI_Error("Timer happen error!\n");
	}

	gdi_lock();
	wm_draw();
	wm_state_set_unactive();
	gdi_unlock();

	return GXCORE_SUCCESS;
}

void GUI_StartSchedule(void)
{
	GxGuiViewServiceRecvMsgStart();
	poll_key_flag = 0;
}

void GUI_StopSchedule(void)
{
	GxGuiViewServiceRecvMsgStop();
	poll_key_flag = 1;
}

status_t GUI_Loop(void)
{
	int ret = 0;
	GUI_StopSchedule();
	ret = GUI_LoopEvent();
	gui_delay(50);
	GUI_StartSchedule();

	return (ret);
}

status_t GUI_LinkDialog(const char *window_name, const char *path)
{
	GuiWidget *widget = NULL;
	GuiWidget *child = NULL;

	widget = gui_get_window(window_name);
	if (NULL == widget)
	{
		GUI_Error("[window] %s cant find.\n", window_name);
		if(gui.config.root_widget)
		{
			widget = widget_new_child(gui.config.root_widget,
					window_name,
					"window",
					NULL);
			if(NULL == widget)
			{
				gxlogd("[GUI] [window] widget new %s failed.\n", window_name);
			}
		}
	}

	if(widget->object)
	{
		GUI_Error("[GUI] [window] % is exist, cannot link to other XML file, please check!\n");
	}

	child = widget_get_firstchild(widget);
	while(child)
	{
		widget_quit(child);
		child = widget_get_nextsibling(child);
	}

	widget->init = GUI_FALSE;
	if(path)
	{
		widget->xml_filename = GxCore_Strdup(path);
	}
	else
	{
		widget->xml_filename = NULL;
	}

	return GXCORE_SUCCESS;
}

status_t GUI_CreateDialog(const char *window_name)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL;

	gui_core = GUI_GetCurrent();
	widget = gui_get_window(window_name);
	if (NULL == widget)
	{
#ifdef GXDBG
		GUI_Debug_Print("[window] %s cant find.\n", window_name);
#endif
		GUI_Error("[window] %s cant find.\n", window_name);
		return GXCORE_ERROR;
	}
	if (widget->object)
	{
		//GUI_Printf("[widget] '%s' is already created, set focus\n", window_name);
		wm_state_set_focus(widget);
		return GXCORE_ERROR;
	}

	if (widget_create(widget))
	{
		stack_push(gui_core->win_stack, widget);
	}


	/*exec create event*/
	GuiWindow *window = NULL;
	window = (GuiWindow *)(widget->object);
	if(NULL == window) return GXCORE_ERROR;

	exec_widget_signal_data(widget, "create", NULL);

	return GXCORE_SUCCESS;
}

status_t GUI_CheckDialog(const char *window_name)
{
	GuiWidget *widget = NULL;

	widget = gui_get_window(window_name);
	if(NULL == widget)
	{
		return (GXCORE_ERROR);
	}
	else if(NULL == widget->object)
	{
		return (GXCORE_ERROR);
	}

	return (GXCORE_SUCCESS);
}

static int _check_src_in_dst(GUI_Rect *src_rect, GUI_Rect *dst_rect)
{
	if((NULL == src_rect) || (NULL == dst_rect))
	{
		return (GUI_FALSE);
	}

	if((src_rect->x <= dst_rect->x) ||
			(src_rect->y <= dst_rect->y) ||
			((src_rect->x + src_rect->w) >= (dst_rect->x + dst_rect->w)) ||
			((src_rect->y + src_rect->h) >= (dst_rect->y + dst_rect->h)))
	{
		return (GUI_FALSE);
	}

	return (GUI_TRUE);
}


status_t GUI_EndDialog(const char *window_name)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL, *window = NULL;
	GuiWidget *new_widget = NULL;
	const char *value = NULL;
	int id = -1;


	if ((NULL != window_name) && (0 == strcasecmp(window_name, "all_window")))
	{
		wm_end_all_window();
		return GXCORE_SUCCESS;
	}
	if ((NULL != window_name) && (0 == strncasecmp(window_name, "after", 5)))
	{
		int ret = 1;
		ret = wm_end_after_window(window_name);
		if(0 == ret) return GXCORE_SUCCESS;

		return GXCORE_ERROR;
	}

	gui_core = GUI_GetCurrent();
	widget = gui_get_window(window_name);
	if (NULL == widget)
	{
#ifdef GXDBG
		GUI_Debug_Print("[window] %s cant find.\n", window_name);
#endif
		GUI_Error("[window] %s cant find.\n", window_name);
		return GXCORE_ERROR;
	}
	if (NULL == widget->object)
	{
#ifdef GXDBG
		GUI_Debug_Print("widget '%s' is not created\n", window_name);
#endif
		GUI_Error("widget '%s' is not created\n", window_name);
		return GXCORE_ERROR;
	}

	/*delete from stack*/
	id = stack_get_id(gui_core->win_stack, (void*)widget);
	if (-1 == id)
	{
		GUI_Error("widget is not in win_stack\n");
		return GXCORE_ERROR;
	}
	new_widget = stack_del(gui_core->win_stack, id);
	widget_release(widget);

	/*update other window*/
	if(((NULL != widget_get_property(widget, "format")) &&
				(0 != strcasecmp("popup", widget_get_property(widget, "format")))) ||
			(NULL == widget_get_property(widget, "format")))
	{
		wm_update_other_window(id, widget);
	}

	/*check focus*/
	if (new_widget)
	{
		/*new window got focus, when end focused window */
		if (widget_get_window(gui_core->config.focus_widget) == widget)
		{
			gui_core->config.focus_widget = NULL;
			wm_state_set_focus(new_widget);
		}
		else
		{
			if(id > 0)
			{
				id--;
				window = widget_get_window(gui_core->config.focus_widget);
				widget = stack_get(gui_core->win_stack, id);
				if(NULL == widget)
					return GXCORE_SUCCESS;
				while(id >= 0)
				{
					if(_check_src_in_dst(&(window->rect), &(widget->rect)))
					{
						widget_set_update(widget, GUI_TRUE);
						window = widget;
					}
					else
					{
						break;
					}
					widget = stack_get(gui_core->win_stack, id);
					id--;
				}
				widget_set_update(widget_get_window(gui_core->config.focus_widget), GUI_TRUE);
			}
		}
	}
	else
	{
		//GUI_Printf("=GUI_EndDialog= no valid window, focus on 'root'\n");
		gui_core->config.focus_widget = gui_core->config.root_widget;
		if(gui_core != &(gui))
		{
			widget_set_update(gui_core->config.focus_widget, GUI_TRUE);
			gui_core->widget_need_update_count++;
		}
	}

	//widget_release(widget);
	/*
	value = widget_get_property(widget, "cache");
	if((value) && (0 == strcasecmp("disable", value)))
	{
		widget_destroy(widget);
	}
	*/
	if((0 != strcasecmp("root", widget->name)) &&
	   (GUI_FALSE == gui_core->config.cache_widgets)) {
		value = widget_get_property(widget, "cache");
		if((NULL == value)) {
			widget_set_property(widget, "cache", "disable");
		}
	}
	value = widget_get_property(widget, "cache");
	widget = gui_get_window(window_name);
	if((value) &&
	   (0 == strcasecmp("disable", value)) &&
	   (widget) &&
	   (gui_core->uncache_stack)) {
		stack_push(gui_core->uncache_stack, (void *)widget);
	}

	return GXCORE_SUCCESS;
}

void _copy_double_surface(void)
{
	GAL_Rect rect = {0};
	int ret = 0;

	/*调此接口往往是需要即刻见到，故在flip后加此操作*/
	if(gui.config.enable_double_buffer)
	{
		rect.x = rect.y = 0;
		rect.w = gui.config.width;
		rect.h = gui.config.height;
		if(gui.layer_mode == GUI_GRADIANT_LAYER) {
			gui.layer_mode = gui.prev_layer_mode;
			ret = hd_copy_surface_gradiant(screen, &rect, back_screen, &rect, gui.config.window_fade_mode, gui.config.window_fade_steps);
		} else {
			ret = gal_copy_surface(screen, &rect, back_screen, &rect);
		}
		if(ret)
		{
			gxlogd("[GUI] Flush failed!\n");
		}
	}
}

status_t GUI_SetProperty(const char *widget_name, const char *property, void *value)
{
	GuiWidget *widget = NULL, *parent = NULL;
	int ret = 1;

	if (NULL == property)
	{
		return GXCORE_ERROR;
	}

	hd_check_layer();
	/*root property*/
	if (0 == strcasecmp(property, "update_all_window"))
	{
		wm_update_all_window();
		return GXCORE_SUCCESS;
	}
	else if (0 == strcasecmp(property, "end_all_window"))
	{
		wm_end_all_window();
		return GXCORE_SUCCESS;
	}
	else if (0 == strcasecmp(property, "draw_now"))
	{
		gdi_lock();
		//wm_copy_main_surface();
		wm_draw();
		wm_state_set_unactive();
		if((GUI_NO_VIRTUAL_LAYER == gui.layer_mode) || (GUI_GRADIANT_LAYER == gui.layer_mode))
		{
			_copy_double_surface();
		}
		wm_state_set_unactive();

		gdi_unlock();
		return GXCORE_SUCCESS;
	}


	/*get widget by name*/
	widget = gui_get_widget(NULL, widget_name);
	if (NULL == widget)
	{
		GUI_Error("can't find widget '%s'\n", widget_name);
		return GXCORE_ERROR;
	}

	/*widget pub property*/
	if (0 == strcasecmp(property, "state"))
	{
		gdi_lock();
		if (0 == strcasecmp((const char*)value, "disable"))
		{
			ret = wm_state_set_disable(widget, GUI_TRUE);
		}
		else if (0 == strcasecmp((const char*)value, "enable"))
		{
			ret = wm_state_set_disable(widget, GUI_FALSE);
		}
		else if (0 == strcasecmp((const char*)value, "osd_trans_hide"))
		{
			ret = wm_state_set_hide(widget, GUI_TRUE, &gui.config.osd_trans);
		}
		else if (0 == strcasecmp((const char*)value, "hide"))
		{
			ret = wm_state_set_hide(widget, GUI_TRUE, NULL);
		}
		else if (0 == strcasecmp((const char*)value, "show"))
		{
			ret = wm_state_set_hide(widget, GUI_FALSE, NULL);
		}
		else if (0 == strcasecmp((const char*)value, "focus"))
		{
			if(NULL == wm_state_set_focus(widget))
			{
				ret = 1;
			}
		}

		if(ret)
		{
			gdi_unlock();
			return GXCORE_ERROR;
		}
		else
		{
			if(value) {
				widget_set_property(widget, property, value);
				if(0 == strcasecmp((const char*)value, "hide")) {
					widget_set_update(widget, GUI_FALSE);
				}
			}
			gdi_unlock();
			return GXCORE_SUCCESS;
		}
	}
	else if((0 != strcasecmp("prompt", widget->classname)) &&
			(0 == strcasecmp(property, "rect")))
	{
		widget_set_property(widget, property, value);
		widget_get_rect(widget, &(widget->rect));
		parent = widget_get_parent(widget);
		if(parent)
		{
			widget->rect.x += parent->rect.x;
			widget->rect.y += parent->rect.y;
		}
		widget_set_update(widget, GUI_TRUE);
	}
	else if(0 == strcasecmp("encoding", property))
	{
		widget_set_property(widget, property, value);
		widget_set_update(widget, GUI_TRUE);
	}


#ifdef GXDBG
	if(NULL == widget->object)
	{
		GUI_Debug_Print("[GUI] The widget %s is not created or is destroyed, Please check(Set %s property)!\n", widget_name, property);
	}
#endif
	/*widget object property*/
	if (widget->ops && widget->ops->set)
	{
		parent = widget_get_parent(widget);
		if((parent) && (0 == strcasecmp("boxitem", parent->classname))) {
			parent->ops->set(parent, (char*)"update", NULL);
		}
		ret = widget->ops->set(widget, (char*)property, value);
	}

	if(ret)return GXCORE_ERROR;

	return GXCORE_SUCCESS;
}

status_t GUI_GetProperty(const char *widget_name, const char *property, void *value)
{
	GuiWidget *widget = NULL;
	int ret = 1;

	widget = gui_get_widget(NULL, widget_name);
	if ((NULL == widget) || (NULL == value))
	{
		GUI_Error("can't find widget '%s' or value is NULL\n", widget_name);
		return GXCORE_ERROR;
	}

#ifdef GXDBG
	if(NULL == widget->object)
	{
		GUI_Debug_Print("[GUI] The widget %s is not created or is destroyed, Please check(Get %s property)!\n", widget_name, property);
	}
#endif

	if (widget->ops && widget->ops->get)
	{
		ret = widget->ops->get(widget, (char*)property, value);
	}

	if(ret && widget->object)
	{
		if(value && widget_get_property(widget, (char*)property))
			strcpy(value, widget_get_property(widget, (char*)property));
	}

	return GXCORE_SUCCESS;
}

status_t GUI_SetFocusWidget(const char *widget_name)
{
	GuiWidget *widget = NULL;

	widget = gui_get_widget(NULL, widget_name);
	if ((NULL == widget) || (NULL == widget->object))
	{
		GUI_Error("can't find widget '%s'\n", widget_name);
		return GXCORE_ERROR;
	}

#ifdef GXDBG
	if(NULL == widget->object)
	{
		GUI_Debug_Print("[GUI] The widget %s is not created or is destroyed, Please check(Set Focus)!\n", widget_name);
	}
#endif

	wm_state_set_focus(widget);

	return GXCORE_SUCCESS;
}

const char *GUI_GetFocusWidget(void)
{
	GuiWidget *widget = NULL;

	widget = wm_state_get_focus();
	if (NULL == widget)
	{
		GUI_Error("can't find widget\n");
		return NULL;
	}

	return (const char *)widget_get_name(widget);
}

const char *GUI_GetFocusWindow(void)
{
	GuiWidget *widget = NULL;
	char* name = NULL;

	widget = wm_state_get_focus();
	if (NULL == widget)
	{
		GUI_Error("can't find widget\n");
		return NULL;
	}

	if(WIDGET_ISNOT_WINDOW(widget))
	{
		name = (char*)widget_get_name(widget->parent);
	}
	else
	{
		name = (char*)widget_get_name(widget);
	}

	return (const char *)name;
}

const char *GUI_GetPreviousWindow(const char *widget_name)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL;
	char* name = NULL;
	int id = 0;

	widget = gui_get_widget(NULL, widget_name);
	if(NULL == widget)
	{
		gxlogd("[GUI]%s window is not created.\n", widget_name);
		return (NULL);
	}

	gui_core = GUI_GetCurrent();
	id = stack_get_id(gui_core->win_stack, (void *)widget);

	widget = stack_get(gui_core->win_stack, id - 1);
	if(NULL == widget)
	{
		return (NULL);
	}

	name = widget->name;

	return (const char *)name;
}

const char *GUI_GetNextWindow(const char *widget_name)
{
	GuiCore *gui_core = NULL;
	GuiWidget *widget = NULL;
	char* name = NULL;
	int id = 0;

	widget = gui_get_widget(NULL, widget_name);
	if(NULL == widget)
	{
		gxlogd("[GUI]%s window is not created.\n", widget_name);
		return (NULL);
	}

	gui_core = GUI_GetCurrent();
	id = stack_get_id(gui_core->win_stack, (void *)widget);

	widget = stack_get(gui_core->win_stack, id + 1);
	if(NULL == widget)
	{
		return (NULL);
	}

	name = widget->name;

	return (const char *)name;
}

static int atoidef(const char *nptr, int v)
{
	if (nptr)
		return atoi(nptr);
	else
		return v;
}

extern int s_pal_index;

static void _free_space(char *str)
{
	char *tok = NULL;
	char *save_p = NULL;
	GAL_Rect rect = {0};

	tok = strtok_r(str, "|", &save_p);
	while(tok)
	{
		if(0 == strcasecmp("fragment", tok))
		{
			wm_draw();
			wm_state_set_unactive();
			/*调此接口往往是需要即刻见到，故在flip后加此操作*/
			if((GUI_NO_VIRTUAL_LAYER == gui.layer_mode) && (gui.config.enable_double_buffer))
			{
				rect.x = rect.y = 0;
				rect.w = gui.config.width;
				rect.h = gui.config.height;
				gal_copy_surface(screen, &rect, back_screen, &rect);
			}
		}
		else if(0 == strcasecmp("spp", tok))
			hd_free_layer_surface(LAYER_MAIN_SPP_SURFACE);
		else if(0 == strcasecmp("back_osd", tok))
			hd_free_layer_surface(LAYER_BACK_OSD_SURFACE);
		else if(0 == strcasecmp("osd", tok))
			hd_free_layer_surface(LAYER_MAIN_OSD_SURFACE);
		tok = strtok_r(NULL, "|", &save_p);
	}
}

VirtualGUIMode get_virtual_gui_mode(char *value)
{
	if(0 == strcasecmp("noblock", value))
	{
		return (NO_BLOCK_MESSAGE);
	}
	else if(0 == strcasecmp("block", value))
	{
		return (BLOCK_MESSAGE);
	}
	else if(0 == strcasecmp("refuse", value))
	{
		return (REFUSE_MESSAGE);
	}

	return (BLOCK_MESSAGE);
}

int gui_set_mirror0(int flag)
{
	GuiCore *gui_core = NULL;
	int ret = 0;

	gui_core = GUI_GetCurrent();
	ret = gui_core->config.mirror;

	gui_core->config.mirror = flag;

	return (ret);
}

int gui_set_mirror_flag(int flag)
{
	GuiCore *gui_core = NULL;
	int ret = 0;

	gui_core = GUI_GetCurrent();

    ret = gui_set_mirror0(flag);
	gui_core->config.keypress_revert = flag;

	return (ret);
}

static void _set_mirror(char *value)
{
	GuiCore *gui_core = NULL;
	int count = 0, i = 0;
	GuiWidget *widget = NULL;

	gui_core = GUI_GetCurrent();

	if((value) && (0 == strcasecmp("enable", (char *)value))) {
		gui_set_mirror_flag(GUI_TRUE);
	} else {
		gui_set_mirror_flag(GUI_FALSE);
	}

	if(gui_core->win_stack) {
		count = stack_count(gui_core->win_stack);
		for(i = 0; i < count; i++) {
			widget = stack_get(gui_core->win_stack, i);
			if(widget) {
				GUI_SetProperty(widget->name, "free_record", NULL);
				GUI_SetProperty(widget->name, "clear_surface", NULL);
			}
			widget_set_update(widget, GUI_TRUE);
		}
	}
}

status_t GUI_SetInterface(const char *property, void *value)
{
	int ret = 1, data = 0;
	char* osd_language = NULL;
	char *str_value = NULL, *tag = NULL;
	GuiCore *gui_core = NULL;

	if (NULL == property)
	{
		return GXCORE_ERROR;
	}

	gui_core = GUI_GetCurrent();
	hd_check_layer();
	if (0 == strcasecmp(property, "osd_language"))
	{
		if(NULL == value) return GXCORE_ERROR;

		osd_language = (char*)value;
		ret = i18n_set_language(osd_language);
	}
	else if (0 == strcasecmp(property, "osd_alpha_global"))
	{
		if(NULL == value) return GXCORE_ERROR;
		if(8 == gui_core->config.bpp)
		{
			gui_core->config.osd_alpha = *(int*)value;
			if(gui_core->config.clut)
			{
				gdi_begin();
				ret = hd_set_clut(screen,
						gui_core->config.clut->palette,
						0,
						gui_core->config.clut->num);
				gdi_end();
				hd_enable_osd(GUI_TRUE);

				if(gui_core->config.enable_double_buffer)
				{
					gdi_begin();
					ret = hd_set_clut(back_screen,
							gui_core->config.clut->palette,
							0,
							gui_core->config.clut->num);
					gdi_end();
					hd_enable_osd(GUI_TRUE);
				}
			}
			else
			{
				s_pal_index = -1;
			}
		}
		else
		{
			gui_core->config.osd_alpha = *(int*)value;
			data = *(int*)value;
			ret = gui_set_alpha((unsigned int)data);

			if(gui_core->config.enable_double_buffer)
			{
				ret = hd_set_alpha(back_screen, data);
			}
		}
	}
	else if(0 == strcasecmp(property, "osd_alpha"))
	{
		if(NULL == value) return GXCORE_ERROR;

		gui_core->config.osd_alpha = *(int*)value;
		if(gui_core->config.clut)
		{
			ret = hd_set_clut(screen,
					gui_core->config.clut->palette,
					0,
					gui_core->config.clut->num);
		}
		else
		{
			s_pal_index = -1;
		}
	}
	else if (0 == strcasecmp(property, "video_size"))
	{
		if(NULL == value) return GXCORE_ERROR;

		GUI_Rect *rect = NULL;

		rect = (GUI_Rect*)(value);
		ret = hd_set_video_size(rect->x, rect->y, rect->w, rect->h);

		//gal_background_desc(spp_surface, NULL);
		ret |= hd_enable_video(GUI_TRUE);
	}
	else if(0 == strcasecmp(property, "image_size"))
	{
		if(NULL == value) return GXCORE_ERROR;

		GUI_Rect *rect = NULL;

		rect = (GUI_Rect*)(value);

		hd_set_image_size(rect->x, rect->y, rect->w, rect->h);
	}
	else if(0 == strcasecmp(property, "interface_size"))
	{
		if(NULL == value) return GXCORE_ERROR;

		GUI_Rect *rect = NULL;

		rect = (GUI_Rect*)(value);

		hd_set_interface_size(rect->x, rect->y, rect->w, rect->h);
	}
	else if (0 == strcasecmp(property, "video_enable"))
	{
		gal_enable_img(GUI_FALSE);
		ret = hd_enable_video(GUI_TRUE);
	}
	else if (0 == strcasecmp(property, "video_disable"))
	{
		ret = hd_enable_video(GUI_FALSE);
		gal_enable_img(GUI_TRUE);
	}
	else if(0 == strcasecmp(property, "video_top"))
	{
		ret = hd_enable_video(GUI_TRUE);
	}
	else if(0 == strcasecmp(property, "image_top"))
	{
		ret = hd_enable_video(GUI_FALSE);
	}
	else if(0 == strcasecmp(property, "image_enable"))
	{
		gal_enable_img(GUI_TRUE);
	}
	else if(0 == strcasecmp(property, "image_disable"))
	{
		gal_enable_img(GUI_FALSE);
	}
	else if(0 == strcasecmp(property, "video_on"))
	{
		hd_enable_video(GUI_TRUE);
	}
	else if(0 == strcasecmp(property, "video_off"))
	{
		hd_enable_video(GUI_FALSE);
	}
	else if(0 == strcasecmp(property, "osd_enable")) {
		hd_enable_osd(GUI_TRUE);
	} else if(0 == strcasecmp(property, "osd_disable")) {
		hd_enable_osd(GUI_FALSE);
	} else if(0 == strcasecmp(property, "logic_clut")) {
		if(NULL == value) return GXCORE_ERROR;

		image_desc *img_desc = NULL;

		img_desc = widget_img_load(value);
		if(img_desc)
		{
			gal_set_logic_clut(screen, img_desc);
		}
	}
	else if(0 == strcasecmp(property, "width"))
	{
		GAL_Rect rect = {0};

		if(NULL == value)
		{
			return (GXCORE_ERROR);
		}

		rect.w = atoidef((char *)value, gui_core->config.width);
		rect.h = gui_core->config.height;

		hd_resolution(&rect);
	}
	else if(0 == strcasecmp(property, "height"))
	{
		GAL_Rect rect = {0};

		if(NULL == value)
		{
			return (GXCORE_ERROR);
		}

		rect.w = gui_core->config.width;
		rect.h = atoidef((char *)value, gui_core->config.width);

		hd_resolution(&rect);
	}
	else if(0 == strcasecmp(property, "font"))
	{
		gxgdi_set_font((char *)value);
	}
	else if(0 == strcasecmp(property, "clear_image"))
	{
		if(!gui_core->stop_update) {
			hd_set_layer_surface(LAYER_MAIN_SPP_SURFACE);
			hd_img_clear();
			gal_set_default_desc(NULL);
			hd_commit_blit();
		}
	}
	else if(0 == strcasecmp(property, "flush"))
	{
		gdi_lock();
		wm_draw();
		if((GUI_NO_VIRTUAL_LAYER == gui.layer_mode) || (GUI_GRADIANT_LAYER == gui.layer_mode))
		{
			_copy_double_surface();
		}
		wm_state_set_unactive();
		gdi_unlock();
	}
	else if(0 == strcasecmp(property, "free_space"))
	{
		gdi_lock();
		ret = 0;
		str_value = (char *)value;
		tag = GxCore_Strdup(str_value);
		if(NULL == tag)
		{
			gdi_unlock();
			return GXCORE_ERROR;
		}
		_free_space(tag);
		GUI_FREE(tag);
		gdi_unlock();
	}
	else if((0 == strcasecmp(property, "key_mode")) &&
			(&gui != gui_core))
	{
		gui_core->config.key_mode = get_virtual_gui_mode((char *)value);
	}
	else if((0 == strcasecmp(property, "message_mode")) &&
			(&gui != gui_core))
	{
		gui_core->config.msg_mode = get_virtual_gui_mode((char *)value);
	} else if(0 == strcasecmp(property, "clean_cache_image")) {
		image_cache_clean_all(gui_core->config.image_cache);
	} else if(0 == strcasecmp(property, "start_cache_image")) {
		image_cache_thread(gui_core->config.image_cache);
	}
	else if(0 == strcasecmp(property, "mirror")) {
		_set_mirror((char *)value);
		return GXCORE_SUCCESS;
	} else if(0 == strcasecmp(property, "keypress_revert")) {
		if(value) {
			str_value = (char *)value;
			if(0 == strcasecmp("enable", str_value)) {
				gui_core->config.keypress_revert = GUI_TRUE;
			} else {
				gui_core->config.keypress_revert = GUI_FALSE;
			}
		}
	}

	if(ret)return GXCORE_ERROR;

	return GXCORE_SUCCESS;
}

status_t GUI_GetInterface(const char *property, void *value)
{
	int ret = GXCORE_SUCCESS;
	GuiCore *gui_core = NULL;
	char *str_value = NULL;

	if (NULL == property) {
		return (GXCORE_ERROR);
	}

	gui_core = GUI_GetCurrent();

	if(0 == strcasecmp(property, "mirror")) {
		str_value = (char *)value;
		if(gui_core->config.mirror) {
			strncpy(str_value, "enable", 6);
		} else {
			strncpy(str_value, "disable", 7);
		}
	} else if(0 == strcasecmp(property, "keypress_revert")) {
		str_value = (char *)value;
		if(gui_core->config.keypress_revert) {
			strncpy(str_value, "enable", 6);
		} else {
			strncpy(str_value, "disable", 7);
		}
	} else if(0 == strcasecmp(property, "update")) {
		str_value = (char *)value;
		if(gui_core->widget_need_update_count) {
			strncpy(str_value, "true", 4);
		} else {
			strncpy(str_value, "false", 5);
		}
	}

	return (ret);
}


status_t GUI_SignalConnect(const char *handlername, GuiSignalFunc func)
{
	void* ret = NULL;

	if(NULL == handlername || NULL == func) return GXCORE_ERROR;

	ret = gui_signal_new(handlername, func, NULL);
	if(NULL == ret) return GXCORE_ERROR;

	return GXCORE_SUCCESS;
}

static void _get_position_size(uint8_t *style_name, short *x, short *y, int *w, int *h)
{
	GuiCore *gui_core = NULL;
	GuiWidget *style = NULL;
	const char *rect = NULL;

	style = (GuiWidget *)widget_get_style((char *)style_name);
	if(NULL == style)
		return;
	rect = widget_get_property(style, "rect");
	sscanf(rect, "[%hd,%hd,%d,%d]", x, y, w, h);

	gui_core = GUI_GetCurrent();
	if((*w > gui_core->config.width) || (*h > gui_core->config.height))
		return;

	/*Let the cordination in the center of the screen*/
	*x = (gui_core->config.width - *w) / 2;
	*y = (gui_core->config.height - *h) / 2;
}

static uint8_t *_prompt_event(GxMsgProperty_GUI_Warning *warning)
{
	const char *ret = NULL;
	uint8_t *mode = NULL, *style_name = NULL;
	GuiWidget *style = NULL;
	short x = 0, y = 0;
	int w = 0, h = 0;

	switch(warning->type)
	{
	case GXGUI_STATUS_NOTICE:
		if(GXCORE_SUCCESS == GUI_CheckPrompt(GLOBAL_NOTICE_PROMPT))
		{
			return (NULL);
		}

		if((warning->warning_string) && (0 == strcasecmp("end", (char* )warning->warning_string)))
		{
			GUI_EndPrompt(GLOBAL_NOTICE_PROMPT);
			return (warning->warning_string);
		}
		style_name = (uint8_t*)GUI_DEFAULT_MESSAGE_NOTICE;
		style = (GuiWidget *)widget_get_style((char *)style_name);
		if(NULL == style)
			return NULL;
		_get_position_size(style_name, &x, &y, &w, &h);
		mode = (uint8_t*)"ontop";
		ret = GUI_CreatePrompt(x,
				y,
				GLOBAL_NOTICE_PROMPT,
				(char *)style_name,
				(char *)warning->warning_string,
				(char *)mode);
		break;
	case GXGUI_STATUS_SINGLE_BUTTON:
		if(GXCORE_SUCCESS == GUI_CheckPrompt(GLOBAL_SINGLE_PROMPT))
		{
			return (NULL);
		}
		style_name = (uint8_t*)GUI_DEFAULT_MESSAGE_SINGLE;
		style = (GuiWidget *)widget_get_style((char *)style_name);
		if(NULL == style)
			return NULL;
		_get_position_size(style_name, &x, &y, &w, &h);
		mode = (uint8_t *)GxCore_Strdup((const char *)warning->return_string0);
		ret = GUI_CreatePrompt(x,
				y,
				GLOBAL_SINGLE_PROMPT,
				(char *)style_name,
				(char *)warning->warning_string,
				(char *)mode);
		GUI_FREE(mode);
		break;
	case GXGUI_STATUS_DOUBLE_BUTTON:
		if(GXCORE_SUCCESS == GUI_CheckPrompt(GLOBAL_DOUBLE_PROMPT))
		{
			return (NULL);
		}
		style_name = (uint8_t*)GUI_DEFAULT_MESSAGE_DOUBLE;
		style = (GuiWidget *)widget_get_style((char *)style_name);
		if(NULL == style)
			return NULL;
		_get_position_size(style_name, &x, &y, &w, &h);
		mode = (uint8_t *)GxCore_Mallocz(strlen((char *)warning->return_string0) + strlen((char *)warning->return_string1) + 10);
		strcpy((char *)mode, (char *)warning->return_string0);
		strcat((char *)mode, "|");
		strcat((char *)mode, (char *)warning->return_string1);
		ret = GUI_CreatePrompt(x,
				y,
				GLOBAL_DOUBLE_PROMPT,
				(char *)style_name,
				(char *)warning->warning_string,
				(char *)mode);
		GUI_FREE(mode);
		break;
	default:
		break;
	}

	return ((uint8_t *)ret);
}

static int _MessageEvent(GUI_Event* event)
{
	GxMessage*                  msg;
	uint8_t 			*ret_str = NULL;
	GxMsgProperty_GUI_Warning   *warning = NULL;
	GxMsgProperty_GUI_Return_Message *return_message = NULL;

	if(GUI_SERVICE_MSG != event->type)
		return EVENT_TRANSFER_KEEPON;

	msg = (GxMessage*)(event->msg.service_msg);
	if(GXMSG_GUI_WARNING != msg->msg_id)
		return EVENT_TRANSFER_KEEPON;

	warning = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_GUI_Warning);
	if(NULL == warning)
		return EVENT_TRANSFER_KEEPON;
	ret_str = _prompt_event(warning);

	switch(warning->type)
	{
	case GXGUI_STATUS_SINGLE_BUTTON:
	case GXGUI_STATUS_DOUBLE_BUTTON:
		msg = GxBus_MessageNew(GXMSG_GUI_RETURN_MESSAGE);
		if (msg == NULL){
			return EVENT_TRANSFER_STOP;
		}
		return_message = GxBus_GetMsgPropertyPtr(msg, GxMsgProperty_GUI_Return_Message);
		return_message->type = GXGUI_STATUS_RETURN_NOTICE;
		GUI_FREE(return_message->src_string);
		return_message->src_string = (uint8_t *)GxCore_Strdup((char *)(warning->warning_string));
		if(ret_str)
		{
			return_message->return_string = (uint8_t *)GxCore_Strdup((char *)ret_str);
			GxBus_MessageSend(msg);
		}
		break;
	default:
		break;
	}

	return EVENT_TRANSFER_KEEPON;
}

static unsigned short password_array[8] = {GUIK_8, GUIK_8, GUIK_1, GUIK_5, GUIK_6, GUIK_0, GUIK_8, GUIK_8};
static unsigned short key_array[8] = {0};
static int key_sum = 0;

static int _check_key(GUI_Event *event)
{
	if((GUIK_0 == event->key.sym) ||
			(GUIK_1 == event->key.sym) ||
			(GUIK_5 == event->key.sym) ||
			(GUIK_6 == event->key.sym) ||
			(GUIK_8 == event->key.sym))
	{
		key_array[key_sum] = event->key.sym;
		key_sum++;
		if(memcmp(password_array, key_array, key_sum * 2))
		{
			memset(key_array, 0, sizeof(key_array));
			key_sum = 0;
		}
		else if((sizeof(password_array) / sizeof(unsigned short)) == key_sum)
		{
			if(0 == memcmp(password_array, key_array, key_sum * 2))
			{
				return (GUI_TRUE);
			}
		}
	}
	else
	{
		memset(key_array, 0, sizeof(key_array));
		key_sum = 0;
	}

	return (GUI_FALSE);
}

void revert_gui_key(GUI_Event* event)
{
	if(NULL == event) {
		return;
	}

	if(GUI_KEYDOWN == event->type) {
		if(GUIK_LEFT == event->key.sym) {
			event->key.sym = GUIK_RIGHT;
		} else if(GUIK_RIGHT == event->key.sym) {
			event->key.sym = GUIK_LEFT;
		}
	}
}

status_t GUI_ExecEvent(GUI_Event* event)
{
	int32_t ret = 0;
	GuiCore *gui_core = NULL;
	GuiWidget *focus_widget = NULL;

	gui_core = GUI_GetCurrent();
	if(0 == event_mutex_id)
	{
		GxCore_MutexCreate(&event_mutex_id);
		if(0 == event_mutex_id)
		{
			return (GXCORE_ERROR);
		}
	}

	if((gui_core) &&
	   (0 == gui_core->rcv_msg_id) &&
	   (GUI_SERVICE_MSG == event->type))
	{
		gui_core->rcv_msg_id = GxCore_ThreadGetId();
	}

	if (NULL == event) return GXCORE_ERROR;

	if(EVENT_TRANSFER_KEEPON == _transfer_send_message(event))
	{
		if(EVENT_TRANSFER_STOP == _transfer_receive_message(event))
		{
			return (GXCORE_SUCCESS);
		}
	}
	else
	{
		return (GXCORE_SUCCESS);
	}

	GxCore_MutexLock(event_mutex_id);
	lock_flag++;

	/*quit by core*/
	if(GUI_QUIT == event->type)
	{
		GUI_Quit();
		GxCore_MutexUnlock(event_mutex_id);
		lock_flag--;
		return GXCORE_SUCCESS;
	}

	/*quit by keybord button x, temp*/
	if(GUI_KEYDOWN == event->type && SDLK_x == event->key.sym)
	{
		GUI_Quit();
		GxCore_MutexUnlock(event_mutex_id);
		lock_flag--;
		return GXCORE_SUCCESS;
	}
#ifndef KEY_RECEIVE
	else if(GUI_KEYDOWN == event->type && 0xFFFF == event->key.sym)
	{
		event->type = GUI_KEYUP;
	}
#endif

	if((GUI_KEYDOWN == event->type) && (gui.config.enable_key_shield))
	{
		if(GUI_TRUE == _check_key(event))
		{
			gui.config.key_shield ^= 1;
		}

		if(GUI_TRUE == gui.config.key_shield)
		{
			GxCore_MutexUnlock(event_mutex_id);
			lock_flag--;
			return GXCORE_SUCCESS;
		}
	}

	gui_core = GUI_GetCurrent();
	focus_widget = widget_get_parent(gui_core->config.focus_widget);
	if(gui_core->config.keypress_revert) {
		revert_gui_key(event);
	}
	/*exec event by top*/
	ret = _MessageEvent(event);
	if(EVENT_TRANSFER_KEEPON == ret)
	{
		ret = widget_notify_top(NULL, (void *)event);
	}

	/*exec event by focus widget/window*/
	if(EVENT_TRANSFER_KEEPON == ret)
	{
		focus_widget = gui_core->config.focus_widget;
		ret = widget_exec_event(focus_widget, (void *)event);
	}

	/*exec event by focus window*/
	if((EVENT_TRANSFER_KEEPON == ret) && WIDGET_ISNOT_WINDOW(focus_widget))
	{
		ret = widget_notify_parent(focus_widget, (void *)event);
	}

	/*exec event by root ??*/
	if((EVENT_TRANSFER_KEEPON == ret) && WIDGET_ISNOT_ROOT(focus_widget))
	{
		widget_notify_root(focus_widget, (void *)event);
	}

	GxCore_MutexUnlock(event_mutex_id);
	lock_flag--;

	return GXCORE_SUCCESS;
}


status_t GUI_SendEvent(const char *widget_name, GUI_Event* event) //be carefull
{
	int32_t ret = 0;
	GuiWidget *widget = NULL;

	widget = gui_get_widget(NULL, widget_name);
	if (NULL == widget)
	{
		GUI_Error("can't find widget '%s'\n", widget_name);
		return GXCORE_ERROR;
	}

	ret = widget_exec_event(widget, (void *)event);

	if(EVENT_TRANSFER_STOP == ret)
	{
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

status_t GUI_BroadcastEvent(GUI_Event *event)
{
	uint32_t count = 0, i = 0;
	GuiWidget *widget = NULL;

	if(NULL == gui.win_stack)
	{
		return GXCORE_ERROR;
	}

	count = stack_count(gui.win_stack);
	for(i = 0; i < count; i++)
	{
		widget = stack_get(gui.win_stack, i);
		if((NULL == widget) || (NULL == widget->object))
		{
			continue;
		}
		widget_exec_event(widget, (void *)event);
	}
	return GXCORE_SUCCESS;
}

status_t GUI_SetEvent(GUI_Event *event)
{
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	if(gui_core->event_cache.type != GUI_NOEVENT)
	{
		gxlogd("%s, cache full\n", __func__);
	}

	if (event && event->type != GUI_NOEVENT) {
		gui_core->event_cache = *event;
	}

	return GXCORE_SUCCESS;
}

status_t GUI_LoadWidget(const char *file_path)
{
	GuiCore *gui_core = GUI_GetCurrent();

	return gui_xml_reload_widget(gui_core, file_path);
}

status_t GUI_RegisterWidget(const char *widget_style)
{
	return widget_register_widget(widget_style);
}

const char *GUI_GetSignal(const char *widget_name, const char *signal_name)
{
	GuiWidget *widget = NULL;
	GuiWidgetSignal *signal = NULL;

	if((NULL == widget_name) || (NULL == signal_name))
	{
		return (NULL);
	}

	widget = gui_get_widget(NULL, widget_name);
	if(NULL == widget)
	{
		return (NULL);
	}

	if(widget->init == 0)
	{
		return (widget_get_signal(widget, signal_name));
	}

	if(NULL == widget->signal)
	{
		return (NULL);
	}

	signal = (GuiWidgetSignal *)hash_get(widget->signal, signal_name);
	if(NULL == signal)
	{
		return (NULL);
	}

	return (signal->handler_name);
}

GuiCore *GUI_GetCurrent(void)
{
	GuiCore *p_gui = NULL;
	int thread = 0, i = 0, count = 0;

	if(0 == gui.gui_num)
		return (&gui);

	thread = GxCore_ThreadGetId();
	if((gui.current == NULL) ||
	   ((thread == gui.console_id) ||
		(thread == gui.rcv_msg_id)))
	{
		p_gui = &gui;
	}
	else if((gui.current) &&
			(thread != gui.current->console_id) &&
			(thread != gui.current->rcv_msg_id))
	{
		if(gui.gui_stack)
		{
			count = stack_count(gui.gui_stack);
			for(i = 0; i < count; i++)
			{
				p_gui = stack_get(gui.gui_stack, i);
				if(p_gui)
				{
					if((p_gui->console_id == thread) ||
					   (p_gui->rcv_msg_id == thread))
					{
						break;
					}
				}
			}

			if((p_gui->console_id != thread) &&
				(p_gui->rcv_msg_id != thread))
			{
				p_gui = &gui;
			}
		}
		else
		{
			p_gui = &gui;
		}
	}
	else if((thread == gui.current->console_id) ||
			(thread == gui.current->rcv_msg_id))
	{
		p_gui = gui.current;
	}

	return (p_gui);
}

void GUI_StopUpdate(int flag)
{
	gui.stop_update = flag;
	if(gui.config.enable_double_buffer) {
		gui_delay(30);
	}
}

status_t GUI_Load(const char *name, const char *path)
{
	status_t ret = GXCORE_SUCCESS;
	GuiCore *gui_core = GUI_GetCurrent();

	if(gui_core) {
		ret = gui_resource_parse(name, gui_core, path);
	} else {
		ret = GXCORE_ERROR;
	}

	return (ret);
}

static int _gui_unload(void *data)
{
	const char *name = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	int ret = GXCORE_SUCCESS;

	if(NULL == data)
		return (-1);

	name = (const char *)data;
	GUI_SetInterface("flush", NULL);
	ret = gui_resource_parse_freedata(name, gui_core);
	GUI_FREE(data);

	return (ret);
}

status_t GUI_ImmediateUnload(const char *name)
{
	int ret = GXCORE_SUCCESS;
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;

	if(name && gui_core->resource_config_table) {
		config = (GuiConfig *)hash_get(gui_core->resource_config_table, name);
		if(config) {
			config->invalid = GUI_TRUE;
		}
	} else {
		return (GXCORE_ERROR);
	}
	GUI_SetInterface("flush", NULL);
	ret = gui_resource_parse_freedata(name, gui_core);

	return (ret);
}

status_t GUI_Unload(const char *name)
{
	event_list *ptimer = NULL;
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;

	if(wm_check_resource_window_exist(name)) {
		gxloge("Resource window Existed!\n");
		return (GXCORE_ERROR);
	}

	if(name && gui_core->resource_config_table) {
		config = (GuiConfig *)hash_get(gui_core->resource_config_table, name);
		if(config) {
			config->invalid = GUI_TRUE;
		}
	} else {
		return (GXCORE_ERROR);
	}

	ptimer = create_timer(_gui_unload, 100, GUI_STRDUP(name), TIMER_ONCE);
	if(NULL == ptimer) {
		return (GXCORE_ERROR);
	}

	return (GXCORE_SUCCESS);
}

status_t GUI_ResourceUnloaded(const char *name)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GuiConfig *config = NULL;

	if(name && gui_core->resource_config_table) {
		config = (GuiConfig *)hash_get(gui_core->resource_config_table, name);
		if(config) {
			return (GXCORE_SUCCESS);
		} else {
			return (GXCORE_ERROR);
		}
	} else {
		return (GXCORE_ERROR);
	}
}

status_t GUI_RegisterGlobalSignal(const char *key, const char *name)
{
	GuiCore *gui_core = GUI_GetCurrent();

	if(gui_core) {
		connect_global_signal(&(gui_core->config), key, (unsigned char *)name);
		return (GXCORE_SUCCESS);
	}
	return (GXCORE_ERROR);
}
