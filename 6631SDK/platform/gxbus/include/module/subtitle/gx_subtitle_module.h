/** \addtogroup gxsubtitle
 *  @{
 */

#ifndef __GX_SUBTITLE_REGISTER_H__
#define __GX_SUBTITLE_REGISTER_H__

#include "gx_subtitle_setting.h"

/**
 * 文本字符大小类型
 */
typedef enum {
	GX_SUBTITLE_SHOW_TEXT_NORMAL_SIZE = 0, ///< 正常尺寸文本字符.
	GX_SUBTITLE_SHOW_TEXT_HALF_SIZE,       ///< 一半尺寸文本字符.
	GX_SUBTITLE_SHOW_TEXT_DOUBLE_SIZE,     ///< 双倍尺寸文本字符.
} GxSubtitleShowTextCharSize;

/**
 * 画布在屏幕显示的位置参数
 */
typedef struct {
	unsigned int x;     ///< x 轴
	unsigned int y;     ///< y 轴.
	unsigned int w;     ///< 宽.
	unsigned int h;     ///< 高.
} GxSubtitleShowScreen;

/**
 * 画布宽高参数
 */
typedef struct {
	unsigned int w;     ///< 宽
	unsigned int h;     ///< 高
} GxSubtitleShowCanvas;

/**
 * 字幕在画布上绘画位置参数
 */
typedef struct {
	unsigned int autoPos;  ///< 选择模式: 1 - 自动模式,根据流自动获取, xl, xr, yt, yb 配置无效; 0 - 强制模式, xl, xr, yt, yb 配置有效.
	unsigned int xl, xr;   ///< xr, xl: x偏左,右边缘距离.
	unsigned int yt, yb;   ///< yt, yb: y偏上,下边缘距离.
} GxSubtitleShowDisplay;

/**
 * 字库和颜色参数
 */
typedef struct {
	const char   *str;    ///< XML 字体库别名.
	unsigned int  color;  ///< 字体颜色,格式: ARGB8888.
} GxSubtitleShowFont;

/**
 * 字符属性参数
 */
typedef struct {
	unsigned int               underline : 1; ///< 下划线.
	unsigned int               bold      : 1; ///< 黑体.
	unsigned int               italic    : 1; ///< 斜体.
	unsigned int               conceal   : 1; ///< 透明.
	unsigned int               reserved  : 12;///< 预留 12bit.
	unsigned int               unicode   : 16;///< 字符编码, 格式为 unicode 编码.
	GxSubtitleShowTextCharSize size;          ///< 字符大小.
	unsigned int               foreground;    ///< 前景颜色.
	unsigned int               background;    ///< 背景颜色.
} GxSubtitleShowTextChar;

/**
 * 文本字幕页参数.
 */
typedef struct {
#define MAX_PAGE_TEXT_LINES         (12)                       ///< 一个页面最大拥有12行数据.
	unsigned int                    attr;                      ///< 页属性: 0 - 选择 chars; 1 - 选择 lines.
	union {
		struct {
			unsigned int            rows;                      ///< 行数.
			unsigned int            columns;                   ///< 列数.
			GxSubtitleShowTextChar* text;                      ///< 字符数据属性.
		} chars;
		struct {
			unsigned int            lines;                     ///< 行数.
			char*                   text[MAX_PAGE_TEXT_LINES]; ///< 字符数据.
		} lines;
	};
} GxSubtitleShowTextPage;

/**
 * 文本字幕绘画实例参数.
 */
typedef struct {
	const char *name;                                                        ///< 自定名称.
	handle_t (*open)  (GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr);  ///< 打开文本字幕.
	void     (*close) (handle_t handle);                                     ///< 关闭文本字幕.
	int      (*draw)  (handle_t handle, GxSubtitleShowTextPage *page);       ///< 绘画一页字幕.
	int      (*clean) (handle_t handle);                                     ///< 清除一页字幕.
	int      (*show)  (void);                                                ///< 对隐藏的字幕进行显示.
	int      (*hide)  (void);                                                ///< 对显示的字幕进行隐藏.
	int      (*setScreen)(GxSubtitleShowScreen *screen);                     ///< 设置显示的位置.
	int      (*setScreenColor)(handle_t handle, unsigned int argb8888);      ///< 设置画布填充颜色.
	int      (*setDisplayPos) (handle_t handle, GxSubtitleShowDisplay *pos); ///< 在画布中设置绘画的位置.
	int      (*setFont)       (handle_t handle, GxSubtitleShowFont    *font);///< 设置字体信息.
} GxSubtitleShowTextOps;

/**
 * 位图格式类型
 */
typedef enum {
	SHOW_PIXEL_RGB,    ///< RGB格式.
	SHOW_PIXEL_YUV     ///< YUV格式.
} GxSubtitleShowPixelType;

/**
 * 像素点字幕页参数
 */
typedef struct {
	unsigned int            width;  ///< 宽度.
	unsigned int            height; ///< 高度.
	GxSubtitleShowPixelType type;   ///< 位图格式.
	union {
		unsigned char*      rgb888; ///< rgb888类型数据.
		unsigned char*      yuv888; ///< yuv888类型数据.
	};
	unsigned char*          alpha;  ///< 透明度.
} GxSubtitleShowPixelPage;

/**
 * 像素点字幕实例参数
 */
typedef struct {
	const char *name;                                                         ///< 自定义名称.
	handle_t (*open)    (GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr); ///< 打开像素点绘画字幕.
	void     (*close)   (handle_t handle);                                    ///< 关闭像素点绘画字幕.
	int      (*draw)    (handle_t handle, GxSubtitleShowPixelPage *page);     ///< 在屏幕上画出字幕.
	int      (*clean)   (handle_t handle);                                    ///< 清除一页字幕.
	int      (*show)    (void);                                               ///< 对隐藏的字幕进行显示.
	int      (*hide)    (void);                                               ///< 对显示的字幕进行隐藏.
	int      (*setDisplayPos) (handle_t handle, GxSubtitleShowDisplay *pos);  ///< 设置在画布上绘画的位置.
	int      (*setScreenColor)(handle_t handle, unsigned int argb8888);       ///< 设置画布上填充的颜色.
	int      (*setScreen)     (GxSubtitleShowScreen *screen);                 ///< 设置画布在屏幕的位置.
	unsigned int* (*allocARGB8888Buffer)(handle_t handle, unsigned int width, unsigned int height); ///< 申请一块 ARGB8888 内存.
	int           (*freeARGB8888Buffer )(handle_t handle, unsigned int *buffer);                    ///< 释放一块 ARGB8888 内存.
	int           (*drawARGB8888Buffer )(handle_t handle, unsigned int *buffer, int full_screen);   ///< 显示一块 ARGB8888 内存的内容.
} GxSubtitleShowPixelOps;

/**
 * \brief   注册文本字幕绘制实例.
 * \details 1. 字幕优先使用注册文本绘制实例,恢复默认绘制需要注销实例.
 * \details 2. 最多注册一个文本字幕绘制实例,最后一次注册的文本字幕绘制实例生效.
 * \details 3. 使用场景: 字幕默认在 SPP 层显示, 通过该接口可实现字幕 OSD 层显示.
 *
 * \param   ops [in] 文本字幕绘制实例参数,详见: gxdocref GxSubtitleShowTextOps.
 * \param            NULL - 注销文本绘制实例; 非NULL - 注册文本绘制实例.
 * \retval  void.
 **/
extern void GxSubtitle_ModuleRegisterShowText(GxSubtitleShowTextOps *ops);

/**
 * \brief   注销文本字幕绘制实例.
 *
 * \param   void.
 * \retval  void.
 **/
extern void GxSubtitle_ModuleUnRegisterShowText(void);

/**
 * \brief   注册像素点字幕绘制实例.
 * \details 1. 字幕优先使用注册像素点字幕绘制实例,恢复默认绘制需要注销像素点字幕绘制实例.
 * \details 2. 最多注册一个像素点字幕绘制实例,最后一次注册的像素点字幕绘制实例生效.
 * \details 3. 使用场景: 字幕默认在 SPP 层显示,通过该接口可实现字幕 OSD 层显示.
 *
 * \param   ops [in] 操作类参数.
 * \retval  void.
 **/
extern void GxSubtitle_ModuleRegisterShowPixel(GxSubtitleShowPixelOps *ops);

/**
 * \brief   注销像素点字幕绘制实例.
 *
 * \param   void.
 * \retval  void.
 **/
extern void GxSubtitle_ModuleUnRegisterShowPixel(void);

#endif

/** @}*/
