#ifndef __GUI_CORE_H__
#define __GUI_CORE_H__

/** \addtogroup  GUI API
 * @{
 */

#ifndef NO_OS
#include "gxtype.h"
#include "gxcore.h"

#include "gui_types.h"
#include "widget.h"
#include "stack.h"
#include "linklist.h"
#include "gui_event.h"
#include "gui_key.h"
#include "gui_timer.h"
#include "gui_hash.h"
#include "framebuffer.h"
#include "gui_pool.h"

__BEGIN_DECLS

#if 1
#define GUI_STRDUP(str) GuiPool_Strdup(str)
#define GUI_MALLOC(size)  GuiPool_Alloc(size)
#define GUI_MALLOCZ(size) GuiPool_Allocz(size)
#define GUI_REALLOC(ptr, size) GuiPool_Realloc(ptr, size)
#define GUI_FREE(p) if(p) {GuiPool_Free(p); p = NULL;}
#else
#define GUI_STRDUP(str)   GxCore_Strdup(str)
#define GUI_MALLOC(size)  GxCore_Malloc(size)
#define GUI_MALLOCZ(size) GxCore_Mallocz(size)
#define GUI_REALLOC(ptr, size) GxCore_Realloc(ptr, size)
#define GUI_FREE(p) if(p) {GxCore_Free(p); p = NULL;}
#endif

static inline int gui_str_cmp(const char *str0, const char *str1)
{
	if((str0) && (str1)) {
		return (strcasecmp(str0, str1));
	} else {
		return (-1);
	}
}

#define WIDGET_IS_FOCUS(name)   ((name)&&(0 == strcasecmp(name, GUI_GetFocusWidget())))
#define WIDGET_ISNOT_FOCUS(name)   ((name)&&(strcasecmp(name, GUI_GetFocusWidget())))

#define SIGNAL_HANDLER
#define SIGNAL_CONNECT(SIGNAL) GUI_SignalConnect(#SIGNAL, SIGNAL);

#define GUI_MEM_POOL_SIZE "ui>gui_mem_pool"
#define GUI_HW_POOL_SIZE  "ui>hw_mem_pool"
#define GUI_HW_POOL_THRESHOLD_KEY    "ui>hw_pool_threashold"
#define GUI_SCHEDULE_TIME "ui>schedule_time"
#define GUI_FONT_THRESHOLD_SIZE              (50);

#else
#include "linklist.h"
#include "gui_hash.h"
#include "gui_types.h"
#include "gxtype.h"
#include "framebuffer.h"
__BEGIN_DECLS
#endif /*NO_OS*/

typedef struct {
	char*       file_xml_config;
	char*       file_xml_keymap;
	char*       file_xml_widget;
	char*       file_xml_style;
	char*       file_xml_color;
	char*       file_xml_i18n;
	char*       file_xml_image;
	char*       file_xml_font;
	char*       file_pal;
}GuiExtFiles;

typedef struct _GuiClut {
	unsigned     int num;
	Color*       palette;
	void*        create_pal;
	struct _GuiClut *next;
}GuiClut;

typedef struct _GuiI18n {
    char*        language;
    hash_t*      hash_text;
    struct _GuiI18n*    next;
}GuiI18n;

typedef struct _GuiColors {
	hash_t*      hash_colors;
}GuiColors;

typedef enum
{
	REFUSE_MESSAGE,
	NO_BLOCK_MESSAGE,
	BLOCK_MESSAGE
}VirtualGUIMode;

typedef hash_t GuiSignalTable;

typedef struct {
	char*        theme_path;
	char*        cur_language;
	char*		 name;

	int          invalid;
	int          chip_id;
	int          is_align;
	int			 x;
	int			 y;
	int          width;
	int          height;
	int          resolution_width;
	int          resolution_height;
	int          only_gdi;
	int          buffer_share;
	int	     key_mask;
	int          bpp;
	int	     color_format;
	int          bpp8_multclut;
	int          enable_ga;
	int          enable_i18n;
	int          ga_count;
	int          image_limit;
	int          enable_antiflicker;
	int          enable_double_buffer;
        int          enable_key_shield;
	int          irr_key_mute;
	int			 esoteric_language;
        int          key_shield;
	int          hardware_layer;
	int          font_overlay;
	int          font_in_pool;
	int          font_threshold_height;
	int          font_threshold_size;
	int          cache_widgets;
	int          uncache_context;
	GxLayerID    layer_id;

#ifndef NO_OS
	/*Widget*/
	GuiWidget*		root_widget;
	GuiWidget*		freedom_widget;
	GuiWidget*		style_widget;
	GuiWidget*		focus_widget;

	GuiSignalTable*	        signal_table;
#endif /*NO_OS*/

	/*Image  Cache*/
	void			*image_cache;

	/*Font*/
	struct gdi_font	*pFirstFont;
	struct gdi_font *pCurrentFont;
	struct gdi_font *pDefaultFont;
	struct gdi_font *pLastFont;

	GuiClut*     clut;
	GuiClut*     firstclut;
	quefr_t*     job_link;
	hash_t*      color_hash;
	Color        osd_trans;
	Color        gui_trans;
	int          osd_alpha;
	int          logical_clut;

	GuiI18n*     i18n;
	GuiI18n*     cur_i18n;

	void*        gsub_opt;

	GuiColors    colors;

	GuiExtFiles  files;
	int          block_join;
	int	          image_buffer_size;
	int			schedule_time;
	int          mirror;
	int          keypress_revert;
	handle_t			widget_thread_handle;
	VirtualGUIMode	key_mode;
	VirtualGUIMode	msg_mode;
#ifndef NO_OS
	struct keys_table real_key_table;
	struct keys_table virtual_key_table;
	ARABIC_STR_PRINT_MODE arabic_mode;
	ARABIC_STR_PUNCTUATION_MODE arabic_punctuation_mode;
	ARABIC_MODE arabic_vary_mode;
	ImagePlayMode play_image_mode;
	ImagePlayMode window_fade_mode;
	int window_fade_steps;
	hash_t *signal;
#endif
} GuiConfig;

#ifndef NO_OS

typedef struct _VirtualLayer {
	int x;
	int y;
	char *name;
	GuiSurface *surface;
	int enable;
}VirtualLayer;

typedef enum
{
	GUI_NO_VIRTUAL_LAYER,
	GUI_HAS_VIRTUAL_LAYER,
	GUI_GRADIANT_LAYER,
}LayerMode;

typedef struct _GuiCore {
	GuiConfig		config;

	/*Stack*/
	gx_stack_t*		win_stack;
	gx_stack_t*		ontop_stack;
	gx_stack_t*		layer_stack;
	gx_stack_t*		gui_stack;
	gx_stack_t*             uncache_stack;

	/*Thread ID*/
	int				console_id;
	int				rcv_msg_id;

	/*Num of GUI*/
	int                     gui_num;

	int				widget_need_update_count;
	int                             stop_update;

	/*Signal & Event*/
	//hash_t*      signal_hash;
	GuiSignal*		key_process;
	GuiSignalFunc	root_event;
	GuiSignalFunc	top_event;
	GUI_Event		event_cache;

	/*GUI's Surface & Layer*/
	GuiSurface		*osd_surface;
	GuiSurface		*dst_surface;
	char			*layer_name;

	/*Image FB*/
	struct			framebuffer fb;
	int				image_size;

	/*Layer*/
	hash_t*			layer_table;
	LayerMode		layer_mode;
	LayerMode               prev_layer_mode;
	int				layer_need_update;

	/*Timer*/
	event_list		*timer_list;
	event_list		*last_timer;

	/*Virtual GUI*/
	struct _GuiCore *current;
	hash_t*			gui_table;

	// resource 总 config 通过名字查找实际 config
	hash_t     *resource_config_table;
	// resource 总 config 栈
	gx_stack_t *resource_config_stack;
} GuiCore;

/*Delayed service message structure*/
typedef struct _MessagePara{
	event_list *timer;
	GUI_Event *event;
}MessagePara;

/**
 * 初始化GUI
 *
 *\param theme_config  GUI入口XML文件路径
 *\return 返回初始化是否成功
 *\retval GXCORE_SUCCESS GUI初始化成功
 *\retval GXCORE_ERROR   GUI初始化出错
 *
 */
status_t GUI_Init(const char* theme_config);


/**
 *
 * 初始化可命名（虚拟）GUI，该操作要在主GUI初始化（GUI_Init）之后，且可以在与主GUI不同的服务（线程）中完成
 * \param name 虚拟GUI名字
 * \param theme_config 虚拟GUI的XML入口文件路径
 * \return 初始化是否成功
 * \retval GXCORE_SUCCESS 初始化成功
 * \retval GXCORE_ERROR 初始化失败
 */
status_t GUI_InitByName(const char *name, const char* theme_config);
status_t GUI_Quit(void);

/**
 *
 * 加载动态资源包，该资源包与主题包共存，需在 GUI_Init 后调用
 * \param name 资源名，必须唯一
 * \param path 资源包路径，以 theme.xml 总 XML 文件路径传入
 * \return 资源包加载是否成功
 * \retval GXCORE_SUCCESS 初始化成功
 * \retval GXCORE_ERROR 初始化失败
 */
status_t GUI_Load(const char *name, const char *path);

/**
 *
 * 卸载动态资源包，该资源包通过 GUI_Load 加载
 * \param name 资源名，为 GUI_Load 传入的唯一资源名
 * \return 资源包卸载是否成功
 * \retval GXCORE_SUCCESS 初始化成功
 * \retval GXCORE_ERROR 初始化失败
 */
status_t GUI_Unload(const char *name);

/**
 *
 * 即时间卸载动态资源包，该资源包通过 GUI_Load 加载，与 GUI_Unload 不同点在于 GUI_Unload
 * 在确保所有资源不用情况下才正式卸载，该接口调用较 GUI_Unload 风险高
 * \param name 资源名，为 GUI_ImmediateUnload 传入的唯一资源名
 * \return 资源包卸载是否成功
 * \retval GXCORE_SUCCESS 初始化成功
 * \retval GXCORE_ERROR 初始化失败
 */
status_t GUI_ImmediateUnload(const char *name);

/**
 *
 * 查询动态资源包已经卸载
 * \param name 资源名，为 GUI_Load 传入的唯一资源名
 * \return 资源包是否存在
 * \retval GXCORE_SUCCESS 已经卸载
 * \retval GXCORE_ERROR 未卸载
 */
status_t GUI_ResourceUnloaded(const char *name);

/**
 *
 * 将名为name的虚拟GUI进行连接，所谓链接即在如Console与RcvMsg服务中调用一次，让该GUI确认只能在此服务（线程）中运行。GUI支持虚拟“高架桥”模式后，更加严格控制了多线程对GUI的操作，即GUI可以多线程，但每个GUI只能操作本服务（线程）中的所有数据。
 * \param name 虚拟GUI名字
 * \return 无
 */
void GUI_CurrentLink(const char* name);

/**
 *
 * 退出名为name的虚拟GUI，接下来对此GUI下窗口、组件的操作都无效
 * \param name 虚拟GUI名字
 * \return 退出名为name的GUI是否成功
 * \retval GXCORE_SUCCESS 退出成功
 * \retval GXCORE_ERROR 退出失败
 */
status_t GUI_QuitByName(const char *name);
status_t GUI_Exec(void);

/**
 *
 * 关调度的GUI消息调度循环 (不推荐)
 *\param 无
 *\return 调度是否成功
 *\retval GXCORE_SUCCESS 成功调度
 *\retval GXCORE_ERROR 不成功调度
 */
status_t GUI_Loop(void);
status_t GUI_LoopEvent(void);
void GUI_StartSchedule(void);
/**
 *
 * 该函数用以创建一个对话框，对话框必须是在XML中定义或已经在组件树中生成的，如果XML描述或组件树中没有此对话框，会打印"cant find widget"。
 * \param window_name 对话框名称，即window组件的名称
 * \return 创建对话框是否成功
 * \retval GXCORE_SUCCESS 创建成功
 * \retval GXCORE_ERROR 创建失败
 */
status_t GUI_CreateDialog(const char* window_name);
/**
 *
 * 用于强制将将组件与某个具体的XML关联，主要用于动态改变widget的风格，但window_name参数必须在整个方案中唯一
 * \param window_name 窗口名字
 * \param path 关联的XML路径
 * \return 关联是否成功
 * \retval GXCORE_SUCCESS 关联成功
 * \retval GXCORE_ERROR 关联失败
 */
status_t GUI_LinkDialog(const char *window_name, const char *path);

/**
 *
 * 判断该窗口是否已经创建
 * \param window_name 窗口名字
 * \return 窗口是否已经创建
 * \retval GXCORE_SUCCESS 已经创建
 * \retval GXCORE_ERROR 未创建
 */
status_t GUI_CheckDialog(const char *window_name);

/**
 *
 * 创建一个弹框。
 * \param x 弹框横坐标位置
 * \param y 弹框纵坐标位置
 * \param name 弹框名字
 * \param style 对应style.xml中的style名字
 * \param string 显示字符 
 * \param mode 模式，分为如("Yes|No"、"OK"或"ontop")
 * \return 创建弹框是否成功
 * \retval GXCORE_SUCCESS 创建成功
 * \retval GXCORE_ERROR 创建失败
 */
const char *GUI_CreatePrompt(int x,
                             int y,
                             const char *name,
                             const char *style,
                             const char *string,
                             const char *mode);
/**
 *
 * 判断该弹框是否已经创建
 * \param name 窗口名字
 * \return 窗口是否已经创建
 * \retval GXCORE_SUCCESS 已经创建
 * \retval GXCORE_ERROR 未创建
 */
status_t GUI_CheckPrompt(const char *name);

/**
 *
 * 设置该弹框显示字符串
 * \param name 窗口名字
 * \param string 显示字符串
 * \return 设置string是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GUI_PromptString(const char *name, const char *string);

/**
 *
 * 关闭一个名为name的弹框
 * \param name 弹框名称
 * \return 关闭弹框是否成功
 * \retval GXCORE_SUCCESS 关闭成功
 * \retval GXCORE_ERROR 关闭失败
 */
status_t GUI_EndPrompt(const char *name);

/**
 *
 * 关闭一个名为window_name的窗体，并释放所有资源，对话框必须是在XML中定义或已经在组件树中生成的，如果XML描述或组件树中没有此对话框，会打印"cant find widget"。
 * \param window_name 对话框名称，即window组件的名称(name)
 * \return 关闭对话框是否成功
 * \retval GXCORE_SUCCESS 关闭成功
 * \retval GXCORE_ERROR 关闭失败
 */
status_t GUI_EndDialog(const char* window_name);

/**
 *
 * 主要用以设置组件的属性
 * \param widget_name 表示对话框名称，此名称与XML中描述的对话框名称相同
 * \param property 表示对话框的属性名称，此名称部分与XML中描述的组件property相同，部分为控制属性名，如edit组件的clear属性，来清空edit框
 * \param value 表示属性值，不是每个属性都需要传入属性值的，属性值一定为指针形式
 * \return 设置属性是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GUI_SetProperty(const char* widget_name, const char* property, void* value);

/**
 *
 * 与GUI_SetProperty函数相反，是从组件中获取某一个属性的值
 * \param widget_name 表示组件名称
 * \param property 表示组件的属性名称
 * \param value 表示组件属性的值
 * \return 获取属性是否成功
 * \retval GXCORE_SUCCESS 获取属性成功
 * \retval GXCORE_ERROR 获取属性失败
 */
status_t GUI_GetProperty(const char* widget_name, const char* property, void* value);

/**
 *
 * 用以强制设置焦点组件
 * \param widget_name 组件名称，在XML中声明，如果XML中不存在此组件则打印"cant find widget"
 * \return 设置焦点组件是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GUI_SetFocusWidget(const char* widget_name);

/**
 *
 * 返回当前聚焦的组件名称
 * \param 无
 * \return 当前焦点组件名称
 * \retval 非NULL 当前聚焦组件
 * \retval NULL 当前没有焦点组件
 */
const char* GUI_GetFocusWidget(void);

/**
 *
 * 获取当前聚焦窗口名称
 * \param 无
 * \return 当前焦点窗口名称
 * \retval 非NULL 当前焦点窗口名称
 * \retval NULL 当前无焦点窗口
 */
const char *GUI_GetFocusWindow(void);

/**
 *
 * 获取在widget_name前创建的窗口名称
 * \param widget_name 需要传入的窗口名称
 * \return 在创建widget_name前，所创建的窗口名称
 * \retval 非NULL 窗口名称
 * \retval NULL widget_name已经是唯一窗口
 */
const char *GUI_GetPreviousWindow(const char *widget_name);

/**
 *
 * 获取在widget_name后创建的窗口名称
 * \param widget_name 需要传入的窗口名称
 * \return 在创建widget_name后，创建的窗口名称
 * \retval 非NULL 窗口名称
 * \retval NULL 在widget_name窗口创建后，没有再创建窗口
 */
const char *GUI_GetNextWindow(const char *widget_name);

/**
 *
 * 用以设置全局interface一级的属性，以及部分控制
 * \param property 需要传入的属性名
 * \parame value 需要传入的属性参数
 * \return 设置属性是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 *
 * \[参数说明](./interface.md)
 *
 */
status_t GUI_SetInterface(const char *property, void *value);

/**
 *
 * * 用以获取全局interface一级的属性，以及部分控制
 * \param property 需要传入的属性名
 * \parame value 需要传入并获取的属性参数
 * \return 设置属性是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 *
 * \[参数说明](./interface.md)
 */
status_t GUI_GetInterface(const char *property, void *value);

/**
 *
 * 用于将XML文件中声明的信号名与具体函数邦定
 * \param handlername 在XML中声明的相应信号名称
 * \param func 具体需要与之邦定的函数名，具体函数原型见实例介绍
 * \return 绑定是否成功
 * \retval GXCORE_SUCCESS 绑定成功
 * \retval GXCORE_ERROR 绑定失败
 * GuiSignalFunc的原型为：
 * typedef int (*GuiSignalFunc)(const char* widgetname, void *userdata);
 *
 * \param widgetname 组件名称
 * \param userdata 用户传入的数据，如按键处理数据，并非每个组件的每个信号处理函数都有用户数据传入
 */
status_t GUI_SignalConnect(const char *handlername, GuiSignalFunc func);

/**
 *
 * 用于将事件设置给GUI调度器，与GUI_SendEvent不同的是，此接口不立即执行事件，且无指定组件
 * \param event 事件指针
 * \return 设置是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GUI_SetEvent(GUI_Event *event);
status_t GUI_ExecEvent(GUI_Event *event);

/**
 *
 * 将消息传递给指定组件
 * \param widget_name 需要传递的目标组件的名称
 * \param event 需要传递的目标组件事件的地址。与GUI_ExecEvent中的相同
 * \return 传递事件调用是否成功
 * \retval GXCORE_SUCCESS 传递事件调用成功
 * \return GXCORE_ERROR 传递事件调用失败
 */
status_t GUI_SendEvent(const char *widget_name, GUI_Event* event);
status_t GUI_BroadcastEvent(GUI_Event *event);

/**
 *
 * 重新加载组件描述文件
 * \param file_path XML文件路径
 * \return 加载是否成功
 * \retval GXCORE_SUCCESS 加载成功
 * \return GXCORE_ERROR 加载失败
 */
status_t GUI_LoadWidget(const char *file_path);
status_t GUI_RegisterWidget(const char *widget_style);
const char *GUI_GetSignal(const char *widget_name, const char *signal_name);
void GUI_StartSchedule(void);
void GUI_StopSchedule(void);
void GUI_StopUpdate(int flag);

/**
 *
 * 加载全局注册函数，与 XML 中在 widget 目录下的总目录 XML 中增加 signal 效果一样
 * \param    key   信号关键字（可以是 create、destroy、got_focus 或 lost_focus）
 * \param    name  信号名，信号名再用 GUI_SignalConnect 函数与具体函数绑定    gxdocref GUI_SignalConnect
 * \return 返回注册是否成功
 * \retval GXCORE_SUCCESS 加载成功
 * \return GXCORE_ERROR 加载失败
 */
status_t GUI_RegisterGlobalSignal(const char *key, const char *name);

/**
 *
 * 注册GBK编码转换表，注册表大约260K Bytes
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int gdi_register_gbk(void);

/**
 *
 * 注册GB2312编码转换表，与GBK为同一注册表，只是部分应用工程师习惯这个名字，注册表大约260K Bytes
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int gdi_register_gb2312(void);

/**
 *
 * 注册中国台湾BIG5编码转换表，注册表大约160K Bytes
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int gdi_register_big5(void);

/**
 *
 * 注册韩国KSX编码转换表，注册表大约136K Bytes
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int gdi_register_ksx(void);

/**
 *
 * 注册不被GXGUI收纳的“小众”编码转换标准;
 * 按照{0xXXXX(原始编码), 0xXXXX(UNICODE编码)}
 * \param name 编码名，必须唯一，不能与GBK、GB2312、KSX和BIG5等任何重名
 * \param table 编码表地址
 * \param size 编码表大小
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int gdi_register_encoding(char *name, unsigned int (*table)[2], unsigned int size);

/**
 *
 * 反（解除）注册某编码标准，释放内存
 * \param name 编码名
 * \return 反（解除）注册是否成功
 * \retval 0 反（解除）注册成功
 * \retval 非0 反（解除）注册不成功
 */
int gdi_unregister_encoding(char *name);

/**
 *
 * 设置默认本地化编码方式，编码方式必须是已注册的，系统默认已经注册的本地化编码方式有：
 *
 *(主要一些欧洲语言的本地化标准，也有泰国语言的标准)
 *
 * iso6937
 *
 * iso8859-01
 *
 * iso8859-02
 *
 * iso8859-03
 *
 * iso8859-04
 *
 * iso8859-05
 *
 * iso8859-06
 *
 * iso8859-07
 *
 * iso8859-08
 *
 * iso8859-09
 *
 * iso8859-10
 *
 * iso8859-11
 *
 * iso8859-13
 *
 * iso8859-14
 *
 * iso8859-15
 *
 * iso8859-16
 *
 *  \param name 编码名
 *  \return 设置编码方式是否成功
 *  \retval 0 设置成功
 *  \retval 非0 设置失败
 */
int gdi_set_default_local(char *name);
#else
typedef struct _GuiCore {
	GuiConfig    config;
	struct framebuffer fb;
} GuiCore;
#endif


__END_DECLS

#endif

/** @}*/


