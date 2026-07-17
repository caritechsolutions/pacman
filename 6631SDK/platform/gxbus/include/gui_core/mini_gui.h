#ifndef __MINI_GUI_H__
#define __MINI_GUI_H__

#include "gdi_core.h"
#include "gxtype.h"
#include "IMG.h"

__BEGIN_DECLS

/** \addtogroup  GDI API
 *@{
 */

/**
 *
 * 以GDI方式初始化系统
 * \param width 全屏宽度
 * \param height 全屏高度
 * \param bpp 方案色深
 * \return 创建的OSD层surface
 * \retval 非NULL OSD层surface
 * \retval NULL 初始化失败
 */
GuiSurface *minigui_init(uint32_t width, uint32_t height, uint32_t bpp);

/**
 *
 * 在系统初始化完成后，获取当前OSD层主surface，即可以向OSD的主surface绘制
 * \param 无
 * \return OSD主surface
 * \retval 非NULL OSD层surface
 * \retval NULL 获取OSD主surface失败，或是系统初始化未成功
 */
GuiSurface *minigui_get_osd(void);

/**
 *
 * 在系统SPP层显示logo
 * \param id 在XML中对应的图片ID，id与file任选其一，优先查询id，即有id对应的图片，不解析file对应图片
 * \param file 在id对应的图片不存在或无效情况下，解析file路径对应的图片（ID是XML中的别名，file是路径）
 * \return 显示logo是否成功
 * \retval 0 显示logo成功
 * \retval 非0 显示logo失败
 */

int32_t minigui_show_logo(const char *id, const char *file);

/**
 *
 * 从系统partition_name分区读取大小为size的数据作为logo文件来显示
 * \param partition_name 分区名（如LOGO分区、DATA分区等）
 * \param size 分区中文件数据的大小
 * \return 显示logo是否成功
 * \retval 0 显示logo成功
 * \retval 非0 显示logo失败
 */
int32_t minigui_show_logo_from_partition(const char *partition_name, int size);

/**
 *
 * 将data(YUV422格式数据)显示到SPP上作为LOGO
 * \param data 数据地址
 * \param width 数据宽度
 * \param height 数据高度
 * \return 显示logo是否成功
 * \retval 0 显示logo成功
 * \retval 非0 显示logo失败
 */
int32_t minigui_show_logo_from_data(void *data, int width, int height);
/*XML GUI*/

/**
 *
 * 将名为SIGNAL的信号函数进行连接
 */
#define MINIGUI_SIGNAL_CONNECT(SIGNAL) minigui_connect_signal(#SIGNAL, SIGNAL);

/**
 *
 * 通过XML初始化系统(GUI方式)
 * \param path XML入口文件路径
 * \return 初始化XML是否成功
 * \retval GXCORE_SUCCESS 初始化成功
 * \retval GXCORE_ERROR 初始化失败
 */
status_t minigui_init_byxml(char *path);

/**
 *
 * 创建对话框
 * \param name 窗口名(xml中所需的frame组件名)
 * \return 创建对话框是否成功
 * \retval GXCORE_SUCCESS 创建成功
 * \retval GXCORE_ERROR 创建失败
 */
status_t minigui_create_dialog(char *name);

/**
 *
 * 关闭(销毁)对话框
 * \param name 窗口名(xml中所需的frame组件名)
 * \return 关闭(销毁)对话框是否成功
 * \retval GXCORE_SUCCESS 关闭(销毁)成功
 * \retval GXCORE_ERROR 关闭(销毁)失败
 */
status_t minigui_end_dialog(char *name);

/**
 *
 * 向名为name的组件设置名为property的属性值指针为data
 * \param name 组件名
 * \property 属性名
 * \data 属性值指针
 * \return 设置属性是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t minigui_widget_set(char *name, char *property, void *data);

/**
 *
 * 从名为name的组件获取名为property的属性值指针
 * \param name 组件名
 * \property 属性名
 * \return 获取属性是否成功
 * \retval GXCORE_SUCCESS 获取成功
 * \retval GXCORE_ERROR 获取失败
 */
void *minigui_widget_get(char *name, char *property);

/**
 *
 * 向name组件添加data数据地址
 * \param name 组件名
 * \param data 添加数据指针，为组件约定字符串形式
 * \return 添加数据项是否成功
 * \retval GXCORE_SUCCESS 添加数据项成功
 * \retval GXCORE_ERROR 添加数据项失败
 */
status_t minigui_widget_add_data(char *name, void *data);

/**
 *
 * 将名为name的组件数据项清空
 * \param name 组件名
 * \return 清空数据项是否成功
 * \retval GXCORE_SUCCESS 清空数据项成功
 * \retval GXCORE_ERROR 清空数据项失败
 */
status_t minigui_widget_clear_data(char *name);

/**
 *
 * 设置name组件为聚焦（TRUE）/非聚焦（FALSE）状态
 * \param name 组件名
 * \param flag 为true表示聚焦，为false表示失焦
 * \return 设置聚焦状态是否成功
 * \retval GXCORE_SUCCESS 设置聚焦成功
 * \retval GXCORE_ERROR 设置聚焦失败
 */
status_t minigui_widget_set_focus(char *name, bool flag);

/**
 *
 * 获取当前聚焦组件
 * \param 无
 * \return 当前焦点组件
 * \retval 非NULL 当前焦点组件名
 * \retval NULL 获取当前焦点组件失败（或没有焦点组件）
 */
char *minigui_widget_get_focus(void);

/**
 *
 * 刷新当前界面
 * \param 无
 * \return 无
 */
void minigui_flush(void);

/**
 *
 * 信号函数原型
 * \param data 信号函数传入指针
 * \return 信号函数返回值
 */
typedef int (*minigui_signal_func)(void *data);

/**
 *
 * 连接signal句柄与函数
 * \param handlename signal句柄名
 * \param func 函数
 * \return 链接信号是否成功
 * \retval GXCORE_SUCCESS 连接成功
 * \retval GXCORE_ERROR 连接失败
 */
status_t minigui_connect_signal(const char *handlename, minigui_signal_func func);

#define MINI_TTF_STYLE_NORMAL			0x00
#define MINI_TTF_STYLE_BOLD				0x01
#define MINI_TTF_STYLE_UNDERLINE		0x04

#define GX_PARTITION_LOGO               ("LOGO")

typedef struct partition  GxMiniPartitionTable;

/**
 *
 * 加载路径为file的size大小style格式的字体，取名为name
 * \param name 字体名
 * \param file 字体文件路径
 * \param size 字体大小(或者称为字号)
 * \param style 可以设置正常模式(MINI_TTF_STYLE_NORMAL)、粗体模式(MINI_TTF_STYLE_BOLD)或下划线模式(MINI_TTF_STYLE_UNDERLINE)
 * \return 加载字体是否成功
 * \retval GXCORE_SUCCESS 加载成功
 * \retval GXCORE_ERROR 加载失败
 */
status_t minigui_load_font(const char *name, const char *file, uint32_t size, uint32_t style);

/**
 *
 * 获取当前正在是用的字体名
 * \param 无
 * \return 字体名
 * \retval 非NULL 当前字体名
 * \retval NULL 获取字体失败（或当前字体不存在）
 */
char *minigui_get_font(void);

/**
 *
 * 设置name字体名为当前字体
 * \param name 字体名
 * \return 设置当前字体是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t minigui_set_font(const char *name);

/**
 *
 * \brief MINI GUI中区域结构体
 */
typedef struct _MiniGUI_Rect {
		int32_t x;       ///< 水平方向坐标
		int32_t y;       ///< 垂直方向坐标
		int32_t w;       ///< 水平方向宽度
		int32_t h;       ///< 垂直方向高度
}MiniGUI_Rect;

/**
 *
 * 申请surface
 * \param rect 区域数据(x、y无效)
 * \param forma 创建的surface数据格式
 * \return 创建的surface结构指针
 * \retval 非NULL 创建的surface数据
 * \retval NULL 创建surface失败
 */
GuiSurface *minigui_get_surface(MiniGUI_Rect *rect, GxColorFormat format);

/**
 *
 * 释放surface
 * \param 申请出来的surface
 * \return 释放surface是否成功
 * \retval GXCORE_SUCCESS 释放surface成功
 * \retval GXCORE_ERROR 释放surface失败
 */
status_t minigui_free_surface(GuiSurface *surface);

typedef int MiniGUIColor;

/**
 *
 * 向目标surface的rect区域填充颜色为color的矩形
 * \param surface 目标surface
 * \param rect 填充区域
 * \param color 颜色
 * \return 填充颜色是否成功
 * \retval GXCORE_SUCCESS 填充成功
 * \retval GXCORE_ERROR 填充失败
 */
status_t minigui_fillrect(GuiSurface *surface, MiniGUI_Rect *rect, MiniGUIColor color);

/**
 *
 * 将src的src_rect区域的数据拷贝到dst的dst_rect区域
 * \param src 源surface
 * \param src_rect 源surface区域
 * \param dst 目标surface
 * \param dst_rect 目标surface区域
 * \return 拷贝操作是否成功
 * \retval GXCORE_SUCCESS 操作成功
 * \retval GXCORE_ERROR 操作失败
 */
status_t minigui_copy(GuiSurface *src, MiniGUI_Rect *src_rect, GuiSurface *dst, MiniGUI_Rect *dst_rect);

/*Text alignment flags, horizontal*/
 #define MINI_TA_HORIZONTAL			(3<<0)
 #define MINI_TA_LEFT				(0<<0)
 #define MINI_TA_RIGHT				(1<<0)
 #define MINI_TA_HCENTRE			(2<<0)

 /*Text alignment flags, vertical*/
 #define MINI_TA_VERTICAL			(3<<2)
 #define MINI_TA_TOP				(0<<2)
 #define MINI_TA_BOTTOM				(1<<2)
 #define MINI_TA_BASELINE			(2<<2)
 #define MINI_TA_VCENTRE			(3<<2)

/**
 *
 * 将string字符串以alignment的对齐方式，用color颜色绘制到目标surface的rect区域
 * \param surface 目标surface
 * \param string 源字符串
 * \param rect 目标surface绘字区域
 * \param color 文字颜色
 * \param alignment 对齐方式
 * \return 实际绘制文字个数
 */
int32_t minigui_draw_string(GuiSurface *surface,
							char *string,
							MiniGUI_Rect *rect,
							MiniGUIColor color,
							int32_t
							alignment);

/**
 *
 * 获取字符串横向绘制的像素数
 * \param string 源字符串
 * \return 横向占用的像素数
 */
int32_t minigui_get_string_pixel(const char *string);

/**
 *
 * 获取当前字体的像素高度
 * \param 无
 * \return 当前字体的像素高度
 */
int minigui_get_font_height(void);

/**
 *
 * 加载(解码)file文件路径的图片
 * \param file 图片文件路径
 * \return 图片数据结构体指针
 * \retval 非NULL 图片数据结构体指针
 * \retval NULL 加载(解码)失败
 */
image_desc *minigui_load_image(const char *file);

/**
 *
 * 将解码结果pDesc，绘制到surface的x、y位置，其目标宽高为w、h
 * \param surface 目标surface
 * \param pDesc 源图片数据
 * \param x 目标水平坐标
 * \param y 目标垂直坐标
 * \param w 目标宽度
 * \param h 目标高度
 * \return 绘制是否成功
 * \return 0 绘制成功
 * \return 非0 绘制失败
 */
int32_t minigui_draw_image(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h);

/**
 *
 * 释放图片数据
 * \param image 图片数据结构指针
 * \return 释放是否成功
 * \retval 0 释放成功
 * \retval 非0 释放失败
 */
int minigui_free_image(image_desc *image);


typedef struct {
	char filename[50];
	size_t size;
	uint8_t *data;
}GxMiniGUIFile;

typedef struct partition_info  GxMiniPartition;

/** @}*/

__END_DECLS

#endif /*__MINI_GUI_H__*/

