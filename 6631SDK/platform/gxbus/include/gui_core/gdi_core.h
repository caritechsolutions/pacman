#ifndef __GXGDI_CORE_H__
#define __GXGDI_CORE_H__

/** \addtogroup  GDI API
 *  @{
 */

#ifndef NO_OS
#include "gxcore.h"
#else
#include "av/gxav.h"
#include "gxtype.h"
#include "common/config.h"
#include "common/gx_hw_malloc.h"
#endif

__BEGIN_DECLS

/**
 *
 * \brief 申请surface区域结构体
 */
typedef struct GAL_Rect {
	int x;       ///< 水平起始坐标
	int y;       ///< 垂直起始坐标
	int w;       ///< 水平宽度
	int h;       ///< 垂直高度
} GAL_Rect;

#define KSURFACE 0x1
#define VBUFFER  0x2

#define SURFACE_FORMAT_ANY     0x0
#define SURFACE_FORMAT_Y8_444  0x1
#define SURFACE_FORMAT_Y8_400  0x2
#define SURFACE_FORMAT_Y8_422V 0x3
#define SURFACE_FORMAT_Y8_422H 0x4
#define SURFACE_FORMAT_Y8_420  0x5

#define FONT_STYLE_NORMAL	0x00
#define FONT_STYLE_BOLD		0x01
#define FONT_STYLE_ITALIC	0x02
#define FONT_STYLE_UNDERLINE	0x04

typedef struct _GuiSurface GuiSurface;
/**
 *
 * \brief GDI接口的中surface的结构体，作为所有GDI接口的基础
 */
struct _GuiSurface {
	GxSurface      sf;       ///< gxavdev中GxSurface结构体
	GxHwMallocObj *vq;       ///< 硬件内存管理块
	uint8_t       *data;     ///< 内核态buffer数据指针
	size_t         buffer_size;   ///< buffer数据的大小，一般为surface->sf.width * surface->sf.height * surface->bpp / 8
	void          *hw_surface;    ///< gxavdev管理的硬件surface指针，一般用于传递给GA相关操作时使用
	int            bpp;           ///< surface的色深，即每一像素点占用几位色空间
	int            format;        ///< surface的颜色格式，如RGB565、ARGB8888或者CLUT8等
	int            hw;            ///< 是否通过gxavdev申请的，4K对齐的buffer空间，如果是4K对齐的，则表示为KSURFACE；若不是，则表示为VBUFFER
	int            alpha;         ///< surface是否需要alpha值的运算
	GuiSurface    *source;        ///< 源surface，一般为自己，如果是通过clone接口调用的到的则不是自己
	int            ref;           ///< 该surface引用次数
	GuiSurface    *next_cloned;   ///< 源自本 surface 的克隆 surface 链表，非克隆 surface 此项为 NULL
};

/**
 *
 * \brief 文字排列方式
 */
typedef enum _STRING_TYPE {
	GDI_STRING_PARAGRAPH,    ///< 断行自动换行
	GDI_STRING_SINGLE        ///< 断行自动去除
}STRING_TYPE;

/**
 *
 * \brief Blit方式
 */
typedef enum {
	GDI_BLIT_STRETCH,        ///< Blit过滤透明色，使用colorkey及透明度混合
	GDI_BLIT_COPY,           ///< Blit直接copy
	GDI_BLIT_BLEND,          ///< Blit无目标colorkey的混合
	GDI_ZOOM,                ///< 缩放
}BLIT_FORMAT;

/**
 *
 * \brief 文字滚动方式
 */
typedef enum {
	GDI_ROLL_RIGHT_LEFT,         ///< 自右向左滚动
	GDI_ROLL_LEFT_RIGHT,         ///< 自左向右滚动
	GDI_ROLL_AUTO                ///< 根据文字书写习惯实现滚动方向
}ROLL_DIRECT;

/**
 *
 * \brief 阿拉伯文字与英语、数字等左向右文字混排时的排版模式
 * \ detail 设阿拉伯语为A, 数字表示下标，第0个阿拉伯语字母表示为 A0
 * \ detail 设英文、数字等左向右文字为E，第0个英文、数字表示为 E0
 * \ detail 有阿拉伯语与英文数字混排字符串： A0 A1 A2 A3 A4 A5 A6 E0 E1 E2 E3 E4 E5 A7 A8 A9
 * \ ARABIC_PRINT_MODE0: A9 A8 A7 E0 E1 E2 E3 E4 E5 A6 A5 A4 A3 A2 A1 A0
 * \ ARABIC_PRINT_MODE1: A6 A5 A4 A3 A2 A1 A0 E0 E1 E2 E3 E4 E5 A9 A8 A7
 */
typedef enum {
	ARABIC_PRINT_MODE0,
	ARABIC_PRINT_MODE1
} ARABIC_STR_PRINT_MODE;

/**
 *
 * \brief 阿拉伯文字与标点混排时的排版模式
 * \ detail 设阿拉伯语为A, 数字表示下标，第0个阿拉伯语字母表示为 A0
 * \ detail 有阿拉伯语与英文数字混排字符串： A0 A1 A2 A3 A4 A5 A6 :
 * \ ARABIC_PUNCTUATION_MODE0: A6 A5 A4 A3 A2 A1 A0 :
 * \ ARABIC_PUNCTUATION_MODE1: : A6 A5 A4 A3 A2 A1 A0
 */
typedef enum {
	ARABIC_PUNCTUATION_MODE0,
	ARABIC_PUNCTUATION_MODE1
} ARABIC_STR_PUNCTUATION_MODE;

/**
 *
 * \brief 阿拉伯语模式与类阿拉伯语模式转换
 * \ detail 发现阿拉伯语模式与类阿拉伯语模式存在部分变形差异，为根本解决问题增加类阿拉伯语变形模式
 * \ ARABIC_NORMAL_MODE  普通阿拉伯语模式
 * \ ARABIC_UIGUR_MODE   类阿拉伯语维吾尔模式
 */
typedef enum {
	ARABIC_NORMAL_MODE,
	ARABIC_UIGUR_MODE,
} ARABIC_MODE;

/**
 *
 * \brief 绘制接口区域结构
 */
typedef struct GXGDI_Rect {
	unsigned int x;              ///< 左坐标
	unsigned int y;              ///< 上坐标
	unsigned int w;              ///< 宽值
	unsigned int h;              ///< 高值
} GXGDI_Rect;

/**
 *
 * \brief 显示字符串的字间距
 */
typedef struct GDI_Interval {
    unsigned int width;       ///< 水平间距
    unsigned int height;      ///< 垂直间距
} GDI_Interval;

typedef enum
{
    LAYER_MAIN_SPP_SURFACE,
    LAYER_MAIN_OSD_SURFACE,
    LAYER_BACK_OSD_SURFACE
}LayerSurface;

/**
 *
 * 申请surface，可以传入buffer，也可以不传入buffer；若不传入buffer系统优先从video memory申请，若申请不到则从系统内存申请
 * \param rect 申请的surface宽高区域，仅宽高有用，x、y无用
 * \param bpp surface色深
 * \param buffer 传入的buffer地址（内核态）
 * \force 是否强制申请，即为true时，强制调用gxavdev的CreateSurface属性创建，为false时，不调用gxavdev的CreateSurface属性创建
 * \return 申请的surface结构体空间
 * \retval 非NULL 申请到的surface结构体指针
 * \retval NULL surface结构体申请失败
 */
GuiSurface *hd_get_surface(GAL_Rect *rect, int bpp, void *buffer, int force);

/**
 *
 * 从现有层的主surface上克隆出一个专用surface，若宽高和色深参数值大于实际层的surface，则相当于hd_get_surface来重新申请
 * \param layer VPU层（GX_LAYER_OSD、GX_LAYER_SPP或GX_LAYER_SOSD等）
 * \param rect 克隆的surface宽高区域，仅宽高有用，x、y无用
 * \param bpp 克隆的surface色深
 * \return 克隆surface是否成功
 * \retval 非NULL 克隆的surface结构体指针
 * \retval NULL 克隆失败
 */
GuiSurface *hd_surface_clone(GxLayerID layer, GAL_Rect *rect, int bpp);
void hd_set_layer_surface(LayerSurface layer);
GuiSurface *gal_image_surface(const char *filepath, int *x, int *y, int width, int height);

/**
 *
 * 若目标区域与源区域相等，则直接混合；若目标区域与源区域不等，则进行缩放
 * \param src 源surface
 * \param src_rect 源surface中的区域
 * \param dst 目标surface
 * \param dst_rect 目标surface中的区域
 * \return 操作是否成功
 * \retval 0 操作成功
 * \retval 非0 操作失败（可能是源与目标大小，颜色格式等原因）
 */
int hd_blit_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect);
void hd_putpixel(GuiSurface *surface, int x, int y, unsigned int pixel);

/**
 *
 * 将源surface内容旋转到目标surface，源与目标的宽高必须定准，由于不会做缩放，否则会导致失败
 * \param src 源surface
 * \param src_rect 源surface中的区域
 * \param dst 目标surface
 * \param dst_rect 目标surface中的区域
 * \return 操作是否成功
 * \retval 0 操作成功
 * \retval 非0 操作失败
 */
int hd_rotate_surface(GuiSurface *src, GAL_Rect *src_rect, GuiSurface *dst, GAL_Rect *dst_rect, GxScrewMode mode);

/**
 *
 * 释放surface
 * \param surface 需要释放的surface
 * \return 无
 */
void hd_free_surface(GuiSurface *surface);

/**
 *
 * 检测当前surface是否存在越界
 * \param surface 需要检测的surface
 * \return 是否越界
 * \retval 0 没有越界
 * \retval 非0 越界（其实不会报这个值，一旦检测到越界，即被认为是一种非常危险的操作，会在此函数中死循环）
 */
int  hd_check_surface(GuiSurface *surface);

/**
 *
 * 清除对应层，OSD层清为OSD透明色，透到视频或图片层；SPP层清为黑色
 * \param layer VPU层（GX_LAYER_OSD、GX_LAYER_SPP或GX_LAYER_SOSD等）
 * \return 无
 */
void hd_clear(GxLayerID layer);

/**
 *
 * 从对应层上抓屏，并保存为output_file的JPEG图片格式
 * \param temp_dir 抓屏并生成相应JPEG文件需要的中间目录，一般临时中间目录在ramfs中
 * \param output_file 输出文件路径（JPEG文件）
 * \param rect 抓屏区域
 * \param layer VPU层（GX_LAYER_OSD、GX_LAYER_SPP或GX_LAYER_VPP等）
 * \return 抓屏是否成功
 * \retval GXCORE_SUCCESS 抓屏成功
 * \retval GXCORE_ERROR 抓屏失败
 */
status_t gal_capture_jpeg(const unsigned char *temp_dir, const unsigned char *output_file, GAL_Rect *rect, GxLayerID layer);


/**
 *
 * 从对应层的区域上抓屏存储到画布(surface)后返回
 * \param layer_id     抓取的层ID
 * \param src_rect         抓取的区域
 * \param dst_rect         目标区域，可以位空，若为空则不做缩放
 * \return  抓取后保存的画布(surface)，画布(surface)为ARGB8888格式
 * \retval   非NULL    抓取成功
 * \retval   NULL      抓取失败
 */
GuiSurface * gal_capture_surface(int layer_id, GAL_Rect *src_rect, GAL_Rect *dst_rect);

/**
 *
 * 在标清输出通路从对应层的区域上抓屏存储到画布(surface)后返回
 * \param src_rect         抓取的区域
 * \param dst_rect         目标区域，可以位空，若为空则不做缩放
 * \return  抓取后保存的画布(surface)，画布(surface)为ARGB8888格式
 * \retval   非NULL    抓取成功
 * \retval   NULL      抓取失败
 */
GuiSurface * gal_capture_surface_sd(GAL_Rect *src_rect, GAL_Rect *dst_rect);

/**
 *
 * 将对应的ARGB8888格式画布(surface)保存位BMP文件
 * \param surface      源画布(surface)
 * \param path         目标BMP文件路径
 * \return   保存是否成功
 * \retval GXCORE_SUCCESS 保存成功
 * \retval GXCORE_ERROR 保存失败
 */
status_t gal_save_surface_to_bmp(GuiSurface *surface, const unsigned char *path);

/**
 *
 * 将对应的BMP文件保存位JPEG文件
 * \param bmp_path      源BMP文件路径
 * \param jpeg_path     目标JPEG文件路径
 * \return   保存是否成功
 * \retval GXCORE_SUCCESS 保存成功
 * \retval GXCORE_ERROR 保存失败
 */
status_t gal_save_bmp_to_jpeg(const unsigned char *bmp_path, const unsigned char *jpeg_path);

/*OSD surface*/

/**
 *
 * 获取OSD层的主surface
 * \param 无
 * \return OSD层的主surface
 * \retval 非NULL OSD层surface，获取到的surface结构体指针，可以强转成GuiSurface结构体使用
 * \retval NULL 获取OSD层surface失败
 */
void *GXGDI_GetMainOSDSurface(void);

/**
 *
 * 获取OSD层的后备surface（针对双buffer才有，单buffer返回NULL）
 * \param 无
 * \return OSD层的后备surface
 * \retval 非NULL OSD层后备surface
 * \retval NULL 获取OSD层后备surface失败
 */
void *GXGDI_GetBackOSDSurface(void);

void *GDI_GetOSDSurface(GXGDI_Rect *rect, int bpp);

/**
 *
 * 设置SPP层开关
 * \param flag 为true时为开启SPP层；为false时为关闭SPP层
 * \return 开关SPP层是否成功，获取到的surface结构体指针，可以强转成GuiSurface结构体使用
 * \retval GXCORE_SUCCESS 开启SPP层成功
 * \retval GXCORE_ERROR 开启SPP层失败
 */
status_t GXGDI_SPP_OnOff(int flag);

/**
 *
 * 设置OSD层开关
 * \param flag 为true时为开启OSD层；为false时为关闭OSD层
 * \return 开关OSD层是否成功
 * \retval GXCORE_SUCCESS 开启OSD层成功
 * \retval GXCORE_ERROR 开启OSD层失败
 */
status_t GXGDI_OSD_OnOff(int flag);

/**
 *
 * GDI操作上锁，一般在非GUI服务中操作GDI需调用
 * \param 无
 * \return 无
 */
void GXGDI_Lock(void);

/**
 *
 * GDI操作解锁，一般在非GUI服务中操作GDI需调用
 * \param 无
 * \return 无
 */
void GXGDI_Unlock(void);

/*Color*/

/**
 *
 * 获取当前方案color key值
 * \param 无
 * \return color key 颜色值
 */
int GXGDI_GetColorKey(void);

/*SPP surface*/

/**
 *
 * 获取SPP层的主surface
 * \param 无
 * \return SPP层的主surface
 * \retval 非NULL SPP层surface，获取到的surface结构体指针，可以强转成GuiSurface结构体使用
 * \retval NULL 获取SPP层surface失败
 */
void *GXGDI_GetMainSPPSurface(void);

void *GXGDI_GetSPPSurface(GXGDI_Rect *rect);

/*Image*/

/**
 *
 * 向surface的x、y位置绘制pdesc图片
 * \param surface 目标surface
 * \param pdesc 源图片结构体指针，可以强转为image_desc
 * \param x 水平坐标
 * \param y 垂直坐标
 * \return 绘制图片是否成功
 * \retval GXCORE_SUCCESS 绘制图片成功
 * \retval GXCORE_ERROR 绘制图片失败
 */
status_t GXGDI_DrawImage(void *surface, const void *pdesc, int x, int y);

status_t GXGDI_DrawSPP(void *surface, const void *pdesc);

/**
 *
 * 获取图片宽度
 * \param pdesc 源图片结构体指针
 * \return 图片宽度值
 */
int      GXGDI_GetImageWidth(const void *pdesc);

/**
 *
 * 获取图片高度
 * \param pdesc 源图片结构体指针
 * \return 图片高度值
 */
int      GXGDI_GetImageHeight(const void *pdesc);

/**
 *
 * 获取图片色深
 * \param pdesc 源图片结构体指针
 * \return 图片色深值
 */
int      GXGDI_GetImageBpp(const void *pdesc);

/**
 *
 * 清除SPP层，将SPP层清为黑色
 * \param 无
 * \return 清除SPP层是否成功
 * \retval GXCORE_SUCCESS 清除成功
 * \retval GXCORE_ERROR 清除失败
 */
status_t GXGDI_ClearSPP(void);

/*Rectangle*/

/**
 *
 * 向目标surface的rect区域填充color颜色（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param surface 目标surface
 * \param rect 目标surface填充区域
 * \param color 颜色值
 * \return 填充矩形是否成功
 * \retval GXCORE_SUCCESS 填充矩形成功
 * \retval GXCORE_ERROR 填充矩形失败
 */
status_t GXGDI_FillRect(void *surface, GXGDI_Rect *rect, int color);

/**
 *
 * \brief 渐变色填充中渐变方向
 */
typedef enum{
	GRADUAL_FILL_VERTICAL,      ///< 垂直方向渐变
	GRADUAL_FILL_HORIZONTAL     ///< 水平方向渐变
}GradualFillType;

/**
 *
 * 向目标surface的dstrect区域填充从start_color到end_color的type方向的渐变色
 * \param surface 目标surface
 * \param dstrect 目标区域
 * \param start_color 起始颜色值（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param end_color 结束颜色值（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param type 颜色渐变方向
 * \return 填充渐变颜色是否成功
 * \retval GXCORE_SUCCESS 填充渐变颜色成功
 * \retval GXCORE_ERROR 填充渐变颜色失败
 */
int  GDI_FillGradualRect(GuiSurface *surface,
			GXGDI_Rect * dstrect,
			unsigned int start_color,
			unsigned int end_color,
			GradualFillType type);

/*String*/

/**
 *
 * 加载某字库文件为某 ID 名称
 * \param ID 字体名称
 * \param pFile 字体文件名
 * \param Size 字体大小
 * \param Style 字体风格, FONT_STYLE_NORMAL, FONT_STYLE_BOLD, FONT_STYLE_ITALIC, FONT_STYLE_UNDERLINE 任选组合
 * \return 加载是否成功
 * \retval GXCORE_SUCCESS 加载成功
 * \retval GXCORE_ERROR 加载失败
 */
int GXGDI_FontLoad(const unsigned char *ID, char* pFile, int Size, int Style);

/**
 *
 * 获取当前字体（应用不需要关心字体节点具体信息，故以void *类型返回，作为应用可以强转为char *类型，即为字体名称）
 * \param 无
 * \return 字体信息
 * \retval 非NULL 有效字体信息
 * \retval NULL 无效字体信息，即获取字体信息失败
 */
void* GXGDI_GetFont(void);

/**
 *
 * 设置当前字体名
 * \param name 字体名，与XML中定义的相同
 * \return 切换新字体前的旧字体
 * \retval 旧字体
 */
void* GXGDI_SetFont(const unsigned char *name);

/**
 *
 * 获取pfont字体的高度
 * \param pfont 字体节点，从GXGDI_GetFont接口获取；gxdocref GXGDI_GetFont
 * \return 字体高度值
 * \retval 字体高度值，非0为有效高度值（以像素为单位）
 */
int GXGDI_GetFontHeight(void* pfont);

/**
 *
 * 获取当前字体中，某编码的纵向跨度偏移与实际高度
 * \param pfont 字体节点，从GXGDI_GetFont接口获取；gxdocref GXGDI_GetFont
 * \param code 文字编码
 * \param yoffset 文字纵向跨度偏移
 * \param height  文字的实际高度
 * \return 无
 */
void GXGDI_GetFontBearingHeight(void *pfont, int code, int *yoffset, int *height);

/**
 * \brief 设置绘字接口门限值，即按照 fontfile 字库的 size 字号的高
 *        度为门限，大于此高度为硬绘字，小于此高度为软绘字
 * \param fontfile 字库路径，必须为应用所使用到的字库路径
 * \param size 字号，fontname 所对应字体文件的字号
 * \return 调用是否成功
 * \retval 0 成功
 * \retval 非0 不成功
 *
 */
int GXGDI_FontThresholdSize(char *fontfile, int size);

/**
 * 销毁名为 name 的字体
 * \param name   字体名
 * \return  是否成功
 * \retval 0 成功
 * \retval 非0 不成功
 */
int GXGDI_FontDestroy(const unsigned char *name);

/**
 *
 * 获取字符串string的当前语种翻译，翻译在相应的XML中
 * \param string 当前字符串ID
 * \return string的当前语言翻译
 * \retval 非NULL string的当前语言有效翻译, 若没有翻译，则返回string本身
 */
void* GXGDI_I18N_String(const char *string);

/**
 *
 * 向目标surface的rect区域用alignment的对齐方式，用color颜色绘制string字符串，字符串的断行处理采用flag的类型
 * \param surface 目标surface
 * \param string 源字符串编码格式
 * \param rect 绘制字符串区域
 * \param alignment 对齐方式
 * \param color 颜色值（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param flag 断行处理方式
 * \return 绘制到字符串结束的位置
 * \retval 绘制到字符串结束的位置，如有字符串"abcdefghijk"，当前值绘制到了'e'的位置，则返回5
 */
int GXGDI_DrawString(void *surface, void *string, GXGDI_Rect *rect, int alignment, int color, STRING_TYPE flag);

/**
 *
 * 向目标surface的rect区域用alignment的对齐方式，用color文字颜色，back_color背景颜色绘制string字符串，字符串的断行处理采用flag的类型
 * \param surface 目标surface
 * \param string 源字符串编码格式
 * \param rect 绘制字符串区域
 * \param alignment 对齐方式
 * \param color 文字颜色值（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param back_color 背景颜色值（颜色值一般为0xRRGGBB或0xAARRGGBB）
 * \param flag 断行处理方式
 * \return 绘制到字符串结束的位置
 * \retval 绘制到字符串结束的位置，如有字符串"abcdefghijk"，当前值绘制到了'e'的位置，则返回5
 */
status_t GXGDI_DrawString_back_color(void *surface, void *string, GXGDI_Rect *rect, int alignment, int color,int back_color, STRING_TYPE flag);

/**
 *
 * 获取当前字符串绘制所需要的横向像素数
 * \param string 源字符串
 * \return 返回像素数
 * \retval 非0 为合法像素数
 * \retval 0或-1 出错
 */
int GXGDI_GetStringPixels(void *string);

/**
 *
 * GDI级设置滚动字幕，并开始滚动
 * \param surface 目标surface
 * \param font 滚动字幕所使用的字体
 * \param string 滚动字幕文字
 * \param rect 滚动字幕在surface中的显示区域
 * \param fore_color 前景色（文字的颜色）
 * \param back_color 背景色（底色）
 * \param delay 文字移动间隔时间（由此控制滚动的快慢）
 * \param direct 文字滚动方向(GDI_ROLL_RIGHT_LEFT为从右往左滚动，GDI_ROLL_LEFT_RIGHT从左往右滚动，GDI_ROLL_AUTO根据文字书写习惯实现滚动方向，如阿拉伯语自左向右)
 * \return 设置启动滚动是否成功
 * \retval GXCORE_SUCCESS 设置并启动滚动成功
 * \retval GXCORE_ERROR 设置并启动滚动失败
 */
handle_t GXGDI_StartRollString(void *surface,
							   const char *font,
							   const char *string,
							   GXGDI_Rect *rect,
							   uint32_t fore_color,
							   uint32_t back_color,
							   uint32_t delay,
							   ROLL_DIRECT direct);

/**
 *
 * 停止滚动，停止后不能使用GXGDI_PauseRollString接口继续滚动，只能使用GXGDI_StartRollString接口重新滚动
 * \param handle 滚动句柄（调用此接口停止后，此句柄即销毁）
 * \return 设置停止滚动是否成功
 * \retval GXCORE_SUCCESS 设置停止滚动成功
 * \retval GXCORE_ERROR 设置停止滚动失败
 */
status_t GXGDI_StopRollString(handle_t handle);

/**
 *
 * 重启滚动
 * \param handle 滚动句柄（调用此接口停止后，此句柄即销毁）
 * \return 重启滚动是否成功
 * \retval GXCORE_SUCCESS 重启滚动成功
 * \retval GXCORE_ERROR 重启滚动失败
 */
status_t GXGDI_ResetRollString(handle_t handle);

/**
 *
 * 暂停滚动（可通过GXGDI_ResetRollString接口重启滚动，会在当前位置继续滚动）
 * \param handle: 滚动句柄
 * \return 暂停滚动是否成功
 * \retval GXCORE_SUCCESS 暂停滚动成功
 * \retval GXCORE_ERROR 暂停滚动失败
 */
status_t GXGDI_PauseRollString(handle_t handle);

/**
 *
 * 获取滚动字幕的当前滚动位置
 * \param handle 滚动句柄
 * \return 滚动位置
 * \retval 滚动字母所滚动到的位置，从最左（右）开始为0
 */
uint32_t GXGDI_GetRollStringPos(handle_t handle);

/**
 *
 * 获取滚动字母滚动次数
 * \param handle 滚动句柄
 * \return 滚动次数
 * \retval 滚动一轮的次数，滚动完第一轮即为1，后续依次类推
 */
uint32_t GXGDI_GetRollTimes(handle_t handle);
status_t GXGDI_SetRollStep(handle_t handle, uint32_t step);
int		 GDI_FontCache(char *fontname, char *subset);
int		 GDI_FontUnCache(char *fontname);

/*Blit*/

/**
 *
 * 将src的src_rect区域内容通过flag运算方式，提交到dst的dst_rect区域
 * \param src 源surface
 * \param src_rect 源surface的区域
 * \param dst 目标surface
 * \param dst_rect 目标surface的区域
 * \param flag 运算方式
 * \return Blit操作是否成功
 * \retval GXCORE_SUCCESS 操作成功
 * \retval GXCORE_ERROR 操作失败
 */
status_t GXGDI_Blit(void *src, GXGDI_Rect *src_rect, void *dst, GXGDI_Rect *dst_rect, BLIT_FORMAT flag);

/**
 *
 * 作为GA加速，批量处理GA任务的开始节点
 * \param 无
 * \return 开始批量提交是否成功
 * \retval GXCORE_SUCCESS 开始批量提交成功
 * \retval GXCORE_ERROR 开始批量提交失败
 */
status_t GXGDI_Begin(void);

status_t GXGDI_Sync(void);

/**
 *
 * 作为GA加速，批量处理GA任务的结束节点
 * \param 无
 * \return 结束批量提交是否成功
 * \retval GXCORE_SUCCESS 结束批量提交成功
 * \retval GXCORE_ERROR 结束批量提交失败
 */
status_t GXGDI_End(void);

/**
 *
 * 相当于调用一次GXGDI_End()后再调用一次GXGDI_Begin()，即结束一轮GA加速提交后，重新开启GA加速提交
 * \param 无
 * \return 操作是否成功
 * \retval GXCORE_SUCCESS 操作成功
 * \retval GXCORE_ERROR 操作失败
 */
status_t GXGDI_Flip(void);

/*Special Font*/

/**
 *
 * /brief 对齐方式
 */
typedef enum _ALIGNMENT_TYPE {
    /*Text alignment flags, horizontal*/
    ALIGNMENT_HORIZONTAL = (3<<0),   ///< 水平对齐方式
    ALIGNMENT_LEFT       = (0<<0),   ///< 水平左对齐
    ALIGNMENT_RIGHT      = (1<<0),   ///< 水平右对齐
    ALIGNMENT_HCENTRE    = (2<<0),   ///< 水平居中

    /*Text alignment flags, vertical*/
    ALIGNMENT_VERTICAL   = (3<<2),   ///< 垂直对齐方式
    ALIGNMENT_TOP        = (0<<2),   ///< 垂直上对齐
    ALIGNMENT_BOTTOM     = (1<<2),   ///< 垂直下对齐
    ALIGNMENT_BASELINE   = (2<<2),
    ALIGNMENT_VCENTRE    = (3<<2)    ///< 垂直居中
} ALIGNMENT_TYPE;

typedef enum _TEXT_FORMAT {
    PARAGRAPH_TEXT,
    SINGLELINE_TEXT
} TEXT_FORMAT;

typedef status_t GXGDI_GetPixelLen(void *string);
typedef status_t GXGDI_GetLines(void *string, GXGDI_Rect *rect, GDI_Interval *interval);
typedef status_t GXGDI_DrawText(void *screen,
				void *string,
				GXGDI_Rect *rect,
				GDI_Interval *interval,
				ALIGNMENT_TYPE alignment,
				int color,
				TEXT_FORMAT flag);
typedef int	 GXGDI_SkipLine(void *start,
				void *string,
				GDI_Interval *interval,
				int line_width,
				ALIGNMENT_TYPE alignment);
typedef status_t GXGDI_RollText(void *screen,
				void *string,
				GXGDI_Rect *rect,
				int pos,
				int color,
				TEXT_FORMAT flag);

typedef struct _GXGDI_FontOpt {
    char name[10];
    GXGDI_GetPixelLen *get_len;
    GXGDI_GetLines    *get_lines;
    GXGDI_DrawText    *draw_text;
    GXGDI_SkipLine    *skip_line;
    GXGDI_RollText    *roll_text;
}GXGDI_FontOpt;


/**
 *
 * 在当前字体下，string字符串在rect区域中，文字间隔按interval计算，工多少行
 * \param string 字符串
 * \param rect 区域
 * \param interval 文字间隔
 * \return 文字行数
 */
int GXGDI_GetStringLines(void *string, GXGDI_Rect *rect, GDI_Interval *interval);

status_t GXGDI_Font_RegisterOpt(GXGDI_FontOpt *opt);
status_t GXGDI_Font_UnRegisterOpt(GXGDI_FontOpt *opt);

/*Image Register*/

/**
 *
 * 注册JPEG硬解
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterJPEG(void);

/**
 *
 * 反（解除）注册JPEG硬解
 * \param 无
 * \return 反（解除）注册是否成功
 * \retval 0 反（解除）注册成功
 * \retval 非0 反（解除）注册不成功
 */
int GDI_UnRegisterJPEG(void);

/**
 *
 * 注册JPEG软解（由于硬解JPEG覆盖面不全）
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterJPEG_SOFT(void);

/**
 *
 * 注册JPEG软解（JPEG不带缩放）
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterJPEG_SOFT_Special(void);

/**
 *
 * 反（解除）注册JPEG软解
 * \param 无
 * \return 反（解除）注册是否成功
 * \retval 0 反（解除）注册成功
 * \retval 非0 反（解除）注册不成功
 */
int GDI_UnRegisterJPEG_SOFT(void);

/**
 *
 * 注册GIF解码
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterGIF(void);

/**
 *
 * 注册PNG解码
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterPNG(void);

/**
 *
 * 注册TIFF解码
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterTIFF(void);

/**
 *
 * 注册MPEG(I帧)解码
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterMPEG(void);

/**
 *
 * 注册WEBP解码
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterWEBP(void);

/**
 *
 * 注册所有解码方式
 * \param 无
 * \return 注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册不成功
 */
int GDI_RegisterAll(void);

/*DVB I18N Code Set*/

/*Single First Byte*/
#define DVB_FIRST_ISO8859_5              (0x01) /* Cyrillic*/
#define DVB_FIRST_ISO8859_6              (0x02) /* Arabic*/
#define DVB_FIRST_ISO8859_7              (0x03) /* Greek*/
#define DVB_FIRST_ISO8859_8              (0x04) /* Hebrew*/
#define DVB_FIRST_ISO8859_9              (0x05) /* Latin alphabet No. 5*/
#define DVB_FIRST_ISO8859_10             (0x06) /* Latin alphabet No. 6*/
#define DVB_FIRST_ISO8859_11             (0x07) /* Thai*/
#define DVB_FIRST_CODE_0x08              (0x08) /* Reserved for future use*/
#define DVB_FIRST_ISO8859_13             (0x09) /* Latin alphabet No. 7*/
#define DVB_FIRST_ISO8859_14             (0x0A) /* Latin alphabet No. 8                 ( Celic)*/
#define DVB_FIRST_ISO8859_15             (0x0B) /* Latin alphabet No. 9*/
#define DVB_FIRST_CODE_0x0C              (0x0C) /* Reserved for future use*/
#define DVB_FIRST_CODE_0x0D              (0x0D) /* Reserved for future use*/
#define DVB_FIRST_CODE_0x0E              (0x0E) /* Reserved for future use*/
#define DVB_FIRST_CODE_0x0F              (0x0F) /* Reserved for future use*/
#define DVB_FIRST_ISO8859                (0x10) /* Set Double Byte*/
#define DVB_FIRST_ISOIEC10646            (0x11) /* ISO/IEC 10646*/
#define DVB_FIRST_KSX101_2004            (0x12) /* Korean*/
#define DVB_FIRST_GB2312                 (0x13) /* Simplified Chinese*/
#define DVB_FIRST_BIG5                   (0x14) /* Big5 Traditional Chinese in Taiwan*/
#define DVB_FIRST_UTF8                   (0x15) /* UTF8*/
#define DVB_FIRST_CODE_16                (0x16) /* Reserved for future use*/
#define DVB_FIRST_CODE_17                (0x17) /* Reserved for future use*/
#define DVB_FIRST_CODE_18                (0x18) /* Reserved for future use*/
#define DVB_FIRST_CODE_19                (0x19) /* Reserved for future use*/
#define DVB_FIRST_CODE_1A                (0x1A) /* Reserved for future use*/
#define DVB_FIRST_CODE_1B                (0x1B) /* Reserved for future use*/
#define DVB_FIRST_CODE_1C                (0x1C) /* Reserved for future use*/
#define DVB_FIRST_CODE_1D                (0x1D) /* Reserved for future use*/
#define DVB_FIRST_CODE_1E                (0x1F) /* Reserved for future use*/

/*Second Byte*/
/*Reserved for future use*/

/*Third Byte*/
#define DVB_THIRD_ISO8859_1              (0x01) /* West European*/
#define DVB_THIRD_ISO8859_2              (0x02) /* East European*/
#define DVB_THIRD_ISO8859_3              (0x03) /* South European*/
#define DVB_THIRD_ISO8859_4              (0x04) /* North and North-East European*/
#define DVB_THIRD_ISO8859_5              (0x05) /* Cyrillic*/
#define DVB_THIRD_ISO8859_6              (0x06) /* Arabic*/
#define DVB_THIRD_ISO8859_7              (0x07) /* Greek*/
#define DVB_THIRD_ISO8859_8              (0x08) /* Hebrew*/
#define DVB_THIRD_ISO8859_9              (0x09) /* West European & Turkish*/
#define DVB_THIRD_ISO8859_10             (0x0A) /* North European*/
#define DVB_THIRD_ISO8859_11             (0x0B) /* Thai*/
#define DVB_THIRD_0x0C                   (0x0C) /* Reserved for future use*/
#define DVB_THIRD_ISO8859_13             (0x0D) /* Baltic*/
#define DVB_THIRD_ISO8859_14             (0x0E) /* Celtic*/
#define DVB_THIRD_ISO8859_15             (0x0F) /* West European*/

/*Discard*/
#define DVB_DISCARD                      (0x80) /* Discard the first code*/

/*Set special DVB code*/

/**
 *
 * 将源编码方式转换为目标编码方式
 * \param src_enc 源字符串编码格式
 * \param dst_enc 目标字符串编码格式
 * \return 设置转换是否成功
 * \retval 0 设置转换成功
 * \retval 非0 设置转换失败
 */
int gdi_encoding_convert(char *src_enc, char *dst_enc);

/*i18n convert interface*/

/**
 *
 * 按编码转换方式，将源字符串转换成utf8编码格式的目标字符串
 * \param from_str 源字符串
 * \param to_str 目标字符串，空间在此函数中申请，在此字符串周期结束后由应用释放
 * \param from_size 源字符串长度
 * \param to_size 目标字符串长度
 * \param iso_str ISO-639标准的国家语言代码，目前暂时没有启用此功能
 * \return 转换是否成功
 * \retval 0 转换成功
 * \retval 非0 转换失败
 */
int gxgdi_iconv(const unsigned char *from_str,
		unsigned char **to_str,
		unsigned int from_size,
		unsigned int *to_size,
		unsigned char *iso_str);

/**
 *
 * 将key与主题包中的image目录下的path相对路径关联
 * \param key image的关键字
 * \param path image的路径
 * \return 关联是否成功
 * \retval GXCORE_SUCCESS 关联成功
 * \retval GXCORE_ERROR 关联失败
 */
status_t gal_add_image(const char *key, const char *path);

/**
 *
 * 将key与绝对路径path关联
 * \param key image的关键字
 * \param path image图片的绝对路径
 * \return 关联是否成功
 * \retval GXCORE_SUCCESS 关联成功
 * \retval GXCORE_ERROR 关联失败
 */
status_t gal_add_key_path(const char *key, const char *path);

/**
 *
 * 获取 key 关键字是否存在此 image 图片
 * \param key image的关键字
 * \return 是否存在
 * \retval GXCORE_SUCCESS 存在
 * \retval GXCORE_ERROR 不存在
 */
status_t gal_check_imagekey(const char *key);

/*Hardware Layer*/

/**
 *
 * 将surface注册到物理层
 * \param layer 物理GX_LAYER_SOSD层或GX_LAYER_SOSD2层，目前只有Sirius芯片支持
 * \param surface 注册到层的主surface
 * \return 设置主surface是否成功
 * \retval 0 设置成功
 * \retval 非0 设置失败
 */
int GxGDI_HardwareLayerRegister(GxLayerID layer, GuiSurface *surface);

/**
 *
 * 获取当前物理层的surface
 * \param layer 物理GX_LAYER_SOSD层或GX_LAYER_SOSD2层，目前只有Sirius芯片支持
 * \param 无
 * \return 获取到的该层的主surface
 * \retval 非NULL 对应层的主surface
 * \retval NULL 没有获取到对应的主surface
 */
GuiSurface *GxGDI_HardwareLayerGet(GxLayerID layer);

/**
 *
 * 使能layer层
 * \param layer 物理GX_LAYER_SOSD层或GX_LAYER_SOSD2层，目前只有Sirius芯片支持
 * \param flag 设置当前物理层使能(true)或关闭(false)
 * \return 设置物理层使能/关闭是否成功
 * \retval 0 设置成功
 * \retval 非0 设置失败
 */
int GxGDI_HardwareLayerEnable(GxLayerID layer, bool flag);

/**
 *
 * 设置对应层的透明度
 * \param layer 物理GX_LAYER_SOSD层或GX_LAYER_SOSD2层，目前只有Sirius芯片支持
 * \param type 设置全局(GX_ALPHA_GLOBAL)或像素(GX_ALPHA_PIXEL)透明度
 * \param alpha 透明度(从0x0到0x7F)
 * \return 设置透明度是否成功
 * \retval 0 设置成功
 * \retval 非0 设置失败
 */
int GxGDI_HardwareLayerAlpha(GxLayerID layer, GxAlphaType type, int alpha);

/**
 *
 * 设置对应层的color key
 * \param layer 物理GX_LAYER_SOSD层或GX_LAYER_SOSD2层，目前只有Sirius芯片支持
 * \param color 设置为color key的颜色值，以0xRRGGBB或32位色方案以0xAARRGGBB的格式传入
 * \return 设置color key是否成功
 * \retval 0 设置成功
 * \retval 非0 设置失败
 */
int GxGDI_HardwareLayerColorKey(GxLayerID layer, int color);

/**
 *
 * 设置对应物理层的优先级
 * \param layer_array 优先级数组，下标越小优先级越高
 * \param count 数组大小
 * \return 设置层级优先是否成功
 * \retval 0 设置成功
 * \retval 非0 设置失败
 */
int GxGDI_HardwareLayerSetPriority(GxLayerID *layer_array, int count);

/**
 * 获取对应物理层的优先级
 * \param layer_array 优先级数组，下标越小优先级越高
 * \return 对应物理层优先级
 * \retval 数字越小优先级越高
 */
int GxGDI_HardwareLayerGetPriority(GxLayerID layer);

/*Virtual Layer*/

/**
 *
 * 通过将surface注册在x、y的位置来初始固化虚拟层次
 * \param surface 通过hd_get_surface申请的surface空间
 * \param x 水平方向位置
 * \param y 垂直方向位置
 * \return 虚拟层名（由系统给出）
 * \retval 非NULL 虚拟层名
 * \retval NULL 创建虚拟层失败
 */
char *GxGDI_LayerRegister(GuiSurface *surface, unsigned int x, unsigned int y);

/**
 *
 * 将名为name的虚拟层解除注册，并释放该层的surface(故surface必须通过调用hd_get_surface申请)
 * \param name 虚拟层的名字
 * \return 解除注册是否成功
 * \retval GXCORE_SUCCESS 解除注册成功
 * \retval GXCORE_ERROR 解除注册失败
 */
status_t GxGDI_LayerUnregister(const char *name);

/**
 *
 * 将指定为name的虚拟层清干净，清后的颜色为主GUI指定的GUI透明色
 * \param name 虚拟层名字
 * \param rect 需要清的范围，若为NULL则说明全层清
 * \return 清楚虚拟层是否成功
 * \retval GXCORE_SUCCESS 清除虚拟层成功
 * \retval GXCORE_ERROR 清除虚拟层失败
 */
status_t GXGDI_LayerClear(const char *name, GXGDI_Rect *rect);

/**
 *
 * 将名为name的虚拟层设置为更新，即一旦此层有更新，向GUI系统（或者叫GDI系统）提交需要更新此层(异步更新)
 * \param name 虚拟层的名字
 * \return 更新虚拟层是否成功
 * \retval GXCORE_SUCCESS 更新虚拟层成功
 * \retval GXCORE_ERROR 更新虚拟层失败
 */
status_t GxGDI_LayerUpdate(const char *name);

/**
 *
 * 将所有虚拟层更新，即一旦调用同步更新
 * \param 无
 * \return 更新虚拟层是否成功
 * \retval GXCORE_SUCCESS 更新虚拟层成功
 * \retval GXCORE_ERROR 更新虚拟层失败
 */
status_t GxGDI_LayerUpdateSync(void);

/**
 *
 * 获取name这一层的surface
 * \param name 虚拟层的名字
 * \return 获取到的虚拟层surface
 * \retval 非NULL 返回获取到的surface
 * \retval NULL 获取不到surface
 */
GuiSurface *GxGDI_LayerGetSurface(const char *name);

/**
 *
 * 将名为name的虚拟层设置使能或失效
 * \param name 虚拟层名字
 * \param flag 设置true或false来控制此虚拟层是否使能
 * \return 使能虚拟层是否成功
 * \retval GXCORE_SUCCESS 使能成功
 * \retval GXCORE_ERROR 使能失败
 */
status_t GxGDI_LayerEnable(const char *name, bool flag);

/**
 *
 * 打印当前状态下虚拟层情况
 * \param 无
 * \return 无
 */
void GxGDI_LayerInfomation(void);

/**
 *
 * 调整名为name的虚拟层位置，不可超出主GUI定义的边界
 * \param name  虚拟层名字
 * \param x 调整后的虚拟层水平位置
 * \param y 调整后的虚拟层垂直位置
 * \return 设置位置是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GxGDI_LayerSetPosition(const char *name, int32_t x, int32_t y);

/**
 *
 * 调整虚拟层的优先级，其中priority数据越大则越靠上
 * \param name  虚拟层名字
 * \param priority 层的优先级
 * \return 设置虚拟层优先级是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
status_t GXGDI_SetLayerPriority(const char *name, uint32_t priority);

/**
 *
 * 获取层的优先级
 * \param name 虚拟层名字
 * \return 获取的虚拟层优先级数
 * \retval 虚拟层优先级数
 */
int32_t GXGDI_GetLayerPriority(const char *name);

/**
 * 设置各层顺序
 * \param channel       设置层顺序通道，如高清(HD_MIXER)、标清(SD_MIXE) 两种通道
 * \param layers        层顺序，下标由小到大为由底层到上层的顺序
 * \param count         层数
 * \return  返回设置层顺序是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 *
 */
int32_t GXGDI_LayerSetSequence(int channel, int *layers, int count);

/**
 * 获取各层顺序
 * \param channel       获取层顺序通道，如高清(HD_MIXER)、标清(SD_MIXE) 两种通道
 * \param layers        层顺序，下标由小到大为由底层到上层的顺序
 * \param count         层数
 * \return  返回获取层顺序是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 *
 */
int32_t GXGDI_LayerGetSequence(int channel, int *layers, int *count);

/**
 * \brief 设置阿拉伯语模式下的排版方式
 * \param mode   阿拉伯语排版模式
 * \return 返回设置阿拉伯语模式是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
int32_t GXGDI_SetArabicMode(ARABIC_STR_PRINT_MODE mode);

/**
 * \brief 获取阿拉伯语模式下的排版方式
 * \param void 无
 * \return 返回当前阿拉伯语排版模式
 * \retval 阿拉伯语与英文、数字混排模式 gxdocref ARABIC_STR_PRINT_MODE
 */
ARABIC_STR_PRINT_MODE GXGDI_GetArabicMode(void);
/**
 * \brief 设置阿拉伯语模式下的标点排版方式
 * \param mode   阿拉伯语标点排版模式
 * \return 返回设置阿拉伯语标点模式是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
int32_t GXGDI_SetArabicPunctuationMode(ARABIC_STR_PUNCTUATION_MODE mode);

/**
 * \brief 获取阿拉伯语模式下的标点排版方式
 * \param void 无
 * \return 返回当前阿拉伯语标点排版模式
 * \retval 阿拉伯语标点模式 gxdocref ARABIC_STR_PUNCTUATION_MODE
 */
ARABIC_STR_PUNCTUATION_MODE GXGDI_GetArabicPunctuationMode(void);

/**
 * \brief 设置阿拉伯语变体模式
 * \param mode   阿拉伯语与类阿拉伯与变体模式  gxdocref ARABIC_MODE
 * \return 返回设置阿拉伯语标点模式是否成功
 * \retval GXCORE_SUCCESS 设置成功
 * \retval GXCORE_ERROR 设置失败
 */
int32_t GXGDI_SetArabicVaryMode(ARABIC_MODE mode);

/**
 * \brief 获取阿拉伯语变体模式
 * \param void 无
 * \return 阿拉伯语与类阿拉伯与变体模式  gxdocref ARABIC_MODE
 */
ARABIC_MODE GXGDI_GetArabicVaryMode(void);

__END_DECLS

/** @}*/

#endif /*__GXGDI_CORE_H__*/

