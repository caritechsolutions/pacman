# keypress信号函数使用

 ## 非默认按键处理

	一般使用在窗口级按键统一处理的场景中，如在某些窗口界面中处理MENU、RED、BLUE以及F1、SUBTITLE等非通用性按键的处理。如通常所见到的fullscreen界面的按键处理：

```cpp
SIGNAL_HANDLER int app_full_screen_keypress(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_STOP;
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;
	switch(event->type) {
		case GUI_KEYDOWN:
			switch(event->key.sym) {
				case STBK_MENU:
					GUI_CreateDialog("wnd_main_menu");
					break;
				case STBK_EPG:
					if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
						GUI_EndDialog("wnd_channel_info");
					app_create_epg_menu();
					break;
				case STBK_SAT:
				case STBK_FAV:
					if (g_AppPlayOps.normal_play.play_total != 0) {
						if (g_AppPlayOps.normal_play.play_total != 0) {
							if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
								GUI_EndDialog("wnd_channel_info");

							GUI_CreateDialog("wnd_channel_list");
							GUI_SendEvent("wnd_channel_list", event);
						}
					} else {
						pop_dlg("The program not exist!");
					}
					break;
				case STBK_REC_START:
					if(g_AppPlayOps.normal_play.view_info.stream_type != GXBUS_PM_PROG_TV)
						break;

					if(usb_state == USB_NO_SPACE) {
						pop_dlg("No enough disk space!");
						break;
					}

					GUI_CreateDialog("wnd_pvr_bar");
					break;
				case STBK_REC_STOP:
					app_pvr_stop();
					GUI_EndDialog("wnd_pvr_bar");
					break;
				case STBK_VOLDOWN:
				case STBK_VOLUP:
				case STBK_LEFT:
				case STBK_RIGHT:
					if(GUI_CheckDialog("wnd_number") == GXCORE_SUCCESS)
						GUI_EndDialog("wnd_number");
					GUI_CreateDialog("wnd_volume_value");
					GUI_SendEvent("wnd_volume_value", event);
					break;
				case STBK_TTX:
					app_ttx_magz_open();
					break;
				case STBK_SUBT:
					if(GUI_CheckDialog("wnd_channel_info") == GXCORE_SUCCESS)
						GUI_EndDialog("wnd_channel_info");

					GUI_CreateDialog("wnd_subtitling");
					break;
				case STBK_INFO:
					if (g_AppPlayOps.normal_play.play_total != 0) {
						if (pvr->state != PVR_DUMMY)
							GUI_CreateDialog("wnd_pvr_bar");
						else
							GUI_CreateDialog("wnd_channel_info");
					}
					break;
				default:
					break;
			}
			break;
	}
}
```
这段代码，描述了普通全屏情况下，各种按键的普遍使用方式。从代码上比较容易理解。

 ## 按键转换

	GUI中各种组件对于按键有默认的处理方式，但部分的默认处理方式并非应用所需要的处理方式。针对这种应用场景，需要注册keypress信号函数，来完成按键转换：

```cpp
SIGNAL_HANDLER int app_test_listview_keypress(GuiWidget *widget, void *usrdata)
{
	GUI_Event *event = NULL;

	event = (GUI_Event *)usrdata;

	switch(event->type) {
		case GUI_KEYDOWN:
			switch(event->key.sym) {
				case STBK_UP:
					return (EVENT_TRANSFER_KEEPON);
				case STBK_DOWN:
					return (EVENT_TRANSFER_KEEPON);
				case STBK_LEFT:
					event->key.sym = STBK_PAGEUP;
					return (EVENT_TRANSFER_KEEPON);
				case STBK_RIGHT:
					event->key.sym = STBK_PAGEDOWN;
					return (EVENT_TRANSFER_KEEPON);
				default:
					return (EVENT_TRANSFER_STOP);
			}
			break;
	}
}

```

由于应用需求是使用左/右键来作为翻页，就在收到左右键后，将按键转换为PageUp/PageDown按键，继续传给组件作处理。


- keypress注册信号作用总结：

| 作用   |   说明     |
|--------|------------|
|处理按键前的应用处理| 需要在GUI组件处理按键前处理某些情况的，需要注册此信号函数来作处理 |
|转换按键| 由于GUI组件只能处理默认键值，针对方案的特殊性需要将某些按键转换为默认按键键值进行处理 |
|拦截按键|某些按键键值不让GUI的某些组件处理，需要将其拦截 |

- keypress注册信号返回值说明：

| 返回值     |       作用           |
|------------|----------------------|
|EVENT_TRANSFER_KEEPON|  继续默认处理  |
|EVENT_TRANSFER_STOP  |  停止默认处理  |

# change信号函数使用

具有change注册信号函数的组件有：listview、timelist、combobox、box、window和edit。触发的时机是当焦点、位置和内容发生改变是回调此信号函数(函数输入data参数为空)。举例为输入拼点信息去锁频：

```cpp
SIGNAL_HANDLER int app_freq_edit_change(GuiWidget *widget, void *usrdata)
{
	char *str = NULL;
	int freq_value = 0;

	if(NULL == widget) {
		return (1);
	}

	GUI_GetProperty(widget->name, "string", &str);
	freq_value = atoi(str);
	if(fe_locked(freq_value) == FE_LOCKED) {
		gxlogd("Frequency : %d locked\n", freq_value);
		// ...
	} else {
		gxlogd("Frequency : %d unlock\n", freq_value);
		// ...
	}
}

```

一般输入锁频界面的应用基本都是如上代码。

# get_data信号函数使用

针对listview和timelist需要数据的，在PM（Program Management）模块中存储一份，如果再在组件中申请内存存储，会造成内存浪费。所以针对这两个组件，都是通过回调get_data信号函数来获取数据，完成绘制后GUI不记录和存储数据，其中listview组件还有获取总数的接口。


```flow
st=>start: 开始
op0=>operation: 获取单条数据
op1=>operation: 绘制单条数据
cond=>condition: 数据获取是否完毕
e=>end: 结束

st->op0->op1->cond
cond(yes)->e
cond(no)->op0(right)

&```

```cpp

SIGNAL_HANDLER int app_channel_list_list_get_total(GuiWidget *widget, void *usrdata)
{
	uint32_t total = 0;

	total = get_service_total();

	return (total);
}

```

```cpp

SIGNAL_HANDLER int app_channel_list_list_get_data(GuiWidget *widget, void *usrdata)
{
	ListItemPara* item = NULL;

	if((NULL == widget) || (NULL == data)) {
		return (GXCORE_ERROR);
	}

	item = (ListItemPara*)usrdata;

	item->x_offset = 2;
    item->string = get_service_name(item->sel);

	item = item->next;
	if(NULL == item) {
		return (GXCORE_ERROR);
	}

	if(get_service_lock((item->sel)) {
		item->image = "s_icon_money.bmp";
	} else {
		item->image = NULL;
	}

	return (GXCORE_SUCCESS);
}

```

返回的字符串类变量必须是全局变量或者是通过Malloc出来的，否则局部变量在出函数后，就释放，当绘制函数拿到这块地址时，就会显示乱码。

# reach_end信号函数使用

目前GUI中，只有edit组件和progbar组件具有此信号函数，主要用途是通知应用此edit组件光标已移到最后一个位置或者progbar已推进到满格。此时，应用可以针对性做出业务逻辑方面的操作。

# service信号函数使用

GUI作为gxbus架构下的一个服务，主要工作是应用接收其他服务的消息，以及应用发送消息给其他服务，并根据应用需求显示数据。

service信号函数，作为应用接收消息的主要接口函数，在需要接收其他服务消息的状态下做出相应业务逻辑的操作，以搜索界面接收消息为例：

```cpp

static int _search_service_ext(GxMessage *pMsg)
{
	GxMsgProperty_NewProgGet* pNewProgGet = NULL;

	switch(pMsg->msg_id) {
		case GXMSG_SEARCH_NEW_PROG_GET:
			pNewProgGet = (GxMsgProperty_NewProgGet*)GxBus_GetMsgPropertyPtr(pMsg, GxMsgProperty_NewProgGet);
			if(GXBUS_PM_PROG_NOT_EXIST == pNewProgGet->flag) {
				sg_StopType.m_nHaveNewProg = TRUE;
			}

			if(GXBUS_PM_PROG_TV == pNewProgGet->type) {
				_search_show_tv_channel(sg_TvCount, pNewProgGet);
				sg_TvCount++;
			} else {
				_search_show_radio_channel(sg_RadioCount, pNewProgGet);
				sg_RadioCount++;
			}
			break;
		case GXMSG_SEARCH_STOP_OK:
		case GXMSG_SEARCH_BLIND_SCAN_FINISH:
			s_blind_stage = BLIND_STAGE_NONE;
			intProgress = 100;
			GUI_SetProperty("progbar_search_progress", "value", &intProgress);
			GUI_SetProperty("text_search_progress_percent","string","100%");
			GUI_CreateDialog("wnd_search_tip");
			break;
		default:
			break;
	}
}

SIGNAL_HANDLER int app_search_service(GuiWidget *widget, void *usrdata)
{
	int ret = EVENT_TRANSFER_KEEPON;
	GUI_Event *event = NULL;
	event = (GUI_Event *)usrdata;

	switch(event->type) {
		case GUI_SERVICE_MSG:
			_search_service_ext((GxMessage*)(event->msg.service_msg));
			break;
		default:
			break;
	}
}

```

# 定时器使用

所谓定时器，并不是精确计时的。而是在定时器超时时作某一事件操作。这些都是应用需要利用GUI经常需要的操作，所以定时器也就成为了GUI的重要部分。 应用场合主要在：不断获取锁频状态、不断获取某些系统参数和不断根据变化更新文字等。

- 定时器有一次执行(TIMER_ONCE)和多次执行(TIMER_REPEAT)，一次执行定时器到期后改定时器即被停职删除（禁止应用去删除），多次执行则不由应用程序主动调用删除接口(remove_timer)不会主动停止和删除。

```cpp
static event_list* ptest_time = NULL;

static  int timer(void *userdata)
{
	GUI_SetProperty("testtext1", "string", "141914");
	return (0);
}

SIGNAL_HANDLER  int timer_test_create(const char* widgetname, void *usrdata)
{
	uint32_t duration = 20000;

	ptest_time = create_timer(timer, duration, NULL,  TIMER_REPEAT);
	return (0);
}

SIGNAL_HANDLER  int password_destroy(const char* widgetname, void *usrdata)
{
	return remove_timer(ptest_time);
}

```

