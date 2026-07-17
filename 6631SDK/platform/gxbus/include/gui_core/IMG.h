#ifndef __IMG_H__
#define __IMG_H__

#include <stdio.h>
#include "gui_types.h"
#ifndef NO_OS
#include "gdi_play.h"
#endif /*NO_OS*/
#include "gdi_core.h"
#include "common/list.h"

__BEGIN_DECLS

/** \addtogroup  GDI API
 *  @{
 */

/**
 *
 * \brief 图片文件类型
 */

typedef enum {
	BMP_TYPE,    ///< BMP格式图片文件
	JPEG_TYPE,   ///< JPEG格式图片文件
	GIF_TYPE,    ///< GIF格式图片文件
	PNG_TYPE,    ///< PNG格式图片文件
	TIFF_TYPE,   ///< TIFF格式图片文件
	MPEG_TYPE,   ///< MPEG格式图片文件
	WEBP_TYPE,   ///< WEBP格式图片文件
	FILE_TYPE,
	UNKOWN_TYPE
}image_type;

/**
 * \brief 图片操作方式
 *
 */
typedef enum {
	EFFECT_SRC_TO_DST = 0,  ///< 带color key与像素透明（32bpp格式）混合
	EFFECT_COPY,            ///< 直接copy，无运算
	EFFECT_SRC_AND_DST      ///< 矢量字运算
}image_effect;


enum load_mode {
	NO_LOAD  = 0,     // 不自动加载片
	DELAY_LOAD = 1,  // 延迟加载图片
};

typedef struct image_file_node image_file_node;

/**
 *
 * /brief 解码结果图片信息
 */
typedef struct _image_desc {
	unsigned int hot;       ///< 该图片热度，即被使用次数
	int ref;                ///< 该图片引用度，即被组件使用的个数
	char *filename;         ///< 图片源所在的路径
	int  hw_accel;          ///< 图片内存若是从video memory中分配，则此变量设置为false；若是从系统中分配，则此变量设置为true

	void *data;             ///< 图片数据地址
	GxHwMallocObj *vq;      ///< 图片数据若是从video memory中分配，此变量为管理块；若不是从video memory中分配，则此变量为NULL
	int xoff;               ///< 产生的矢量字信息水平方向偏移
        int yoff;           ///< 产生的矢量字信息垂直方向偏移
	int width;              ///< 图片宽
	int height;             ///< 图片高
	int bpp;                ///< 图片色深
	GxColorFormat fmt;      ///< 图片颜色格式

	void *pal;              ///< 图片色表
	image_type type;        ///< 源图片格式
	int pal_index;          ///< 系统色表索引号
	int bkgd;               ///< 矢量字信息背景颜色（一般不用）
	int color;              ///< 矢量字信息前景颜色
	int size;               ///< 图片数据大小
	int alpha;              ///< 是否启用图片中的Alpha为（针对有alpha位的图片）
	image_effect effect;    ///< 贴图运算方式
	enum load_mode load_mode;
	char *theme;
	image_file_node *ops;
	struct gxlist_head head;
}image_desc;

#define MAX_FILE_TYPE      (4)

#define IMAGE_1_BPP         (1)
#define IMAGE_4_BPP         (4)
#define IMAGE_8_BPP         (8)
#define IMAGE_16_BPP        (16)
#define IMAGE_24_BPP        (24)
#define IMAGE_32_BPP        (32)

typedef int (*image_covert)(const image_desc *image, int i, int j);

typedef int         (*gal_image_cb)(void *dst, const char *filename, int x, int y, int screen_width, int screen_height);
typedef int         (*gal_image_stop_cb)(void);
typedef image_desc *(*gal_image_load_cb)(image_desc *desc, const char *filename);
typedef int         (*gal_image_free)(void *data);
#ifndef NO_OS
typedef int         (*gal_image_getinfo)(const char *filename, Image_Exif *img_info);
typedef image_desc *(*_special_zoom_cut)(const char *pFile, int zw, int zh);
typedef image_desc *(*_special_load)(const char *filename, int color);
typedef image_desc *(*_img_load_size)(image_desc *pDes, handle_t hfile, unsigned int dwSize);
typedef int			(*_img_check)(const char *filename);
#else
typedef int         (*gal_image_getinfo)(const char *filename, void *img_info);
#endif /*NO_OS*/

struct image_file_node {
	char *file_type[MAX_FILE_TYPE];
	image_type type;

	gal_image_cb            img_cb;
	gal_image_load_cb       img_load_cb;
	gal_image_getinfo       img_get_info_cb;
	gal_image_free          img_free;
	gal_image_stop_cb       img_stop;
#ifndef NO_OS
	struct image_file_node *reserve;
#endif /*NO_OS*/
};

#ifndef NO_OS
struct image_special_node {
	_special_zoom_cut		img_zoom_cut;
	_special_load			img_load;
	_img_load_size			img_load_size;
	_img_check				img_check;
};
#endif /*NO_OS*/

image_desc *gal_img_new(void);
void *gal_img_alloc_memory(image_desc *desc);
void  gal_img_free_memory(image_desc *desc);

/*
 * params
 * * filename: GUI保存的图片路径
 * return: 真实存在的图片路径
 */
typedef char* (*preprocessing)(const char *filename);
/*
 * params
 * * filename: GUI保存的图片路径
 * * ptr: 由preprocessing返回的真实存在的图片路径
 * return: void
 */
typedef void (*postprocessing)(const char *filename, char *ptr);

/**
 *
 * 注册函数集
 */
struct image_register_func {
    preprocessing pre;      ///< 打开文件前函数
    postprocessing post;    ///< 关闭文件后函数
};

/*
 *
 * 注册图片解码打开文件前后关闭文件后的处理回调函数 gxdocref : gal_img_processing_func_unregister image_register_func
 * \param func 应用根据需要注册图片打开文件前和关闭文件后的回调处理函数，必须同时注册
 * \return  注册是否成功
 * \retval 0 注册成功
 * \retval 非0 注册失败
 */
int gal_img_processing_func_register(struct image_register_func *func);
/*
 *
 * 反注册图片解码打开文件前后关闭文件后的处理回调函数，反注册后所述回调函数失效 gxdocref : gal_img_processing_func_register
 * \param  void
 * \return  无
 */
void gal_img_processing_func_unregister(void);

/**
 *
 * 从图片文件路径生成图片数据
 * \param desc 如果已经申请好相应的图片数据节点，则可以传入；一般传入为NULL，由系统自动分配
 * \param filename 图片文件路径
 * \return 解码生成图片数据
 * \retval 非NULL 合法图片数据
 * \retval NULL 图片解码出错
 */
image_desc *gal_img_load(image_desc *desc, const char *filename);

/**
 *
 * 图片数据释放，将filename和图片内部解码数据，以及image_desc结构体全部释放
 * \param img 图片解码后数据
 * \return 释放是否成功
 * \retval 0 释放数据成功
 * \retval 非0 释放数据失败
 */
int gal_img_release(image_desc *img);

/**
 *
 * 图片数据清除，只将解码后的data数据释放，其他数据继续保留
 * \param img 图片解码后数据
 * \return 清除是否成功
 * \retval 0 清除数据成功
 * \retval 非0 清除数据失败
 */
int gal_img_cleanup(image_desc *img);

int gal_img_get_width(const char *filename);
int gal_img_get_height(const char *filename);

void gal_set_default_desc(const image_desc * pDesc);

/**
 *
 * 将源文件filename，按x、y的坐标，与screen_width、screen_height的宽高值向screen贴，若screen_width与screen_height与图片宽高不同，则进行一边靠缩放（宽或高比例更接近的优先缩放）
 * \param screen 目标surface
 * \param filename 源文件
 * \param x 水平坐标
 * \param y 垂直坐标
 * \param screen_width 水平宽度（若超过目标surface宽度，则会报错）
 * \param screen_height 垂直高度（若超过目标surface高度，则会报错）
 * \return 解码、贴图操作是否成功
 * \retval 0 操作成功
 * \retval 非0 操作失败
 */
int gal_img_file       (void *screen, const char *filename, int x, int y, int screen_width, int screen_height);

/**
 *
 * 将解码结果贴到screen的x、y位置上，若w、h与图片宽高不等，则进行缩放
 * \param screen 目标surface
 * \param pDesc 图片数据地址
 * \param x 贴到目标的水平位置
 * \param y 贴到目标的垂直位置
 * \param w 图片贴到目标位置后的宽
 * \param h 图片贴到目标位置后的高
 * \return 贴图是否成功
 * \retval 0 贴图成功
 * \retval 非0 贴图失败
 */
int gal_img_desc(void *screen, const image_desc *pDesc, int x, int y, int w, int h);

/**
 *
 * \brief 设置图片解码的内存分配上限，分配上限按：size = width * height * bpp / 8 进行计算，当达图片需要内存大于此值后不解码
 * \param size       图片解码上限值，当此值设置位0时，表示图片解码无上限
 * \return
 * \retval 无
 */
void gal_img_threshod_size(int size);

/**
 *
 * \brief 图片解码内存类型
 */
typedef enum {
	IMAGE_FROM_VIDEO_MEMORY,      ///< 从视频解码内存（即洞内存）申请
	IMAGE_FROM_SYSTEM_MEMORY      ///< 从系统内存申请
} ImageMemorySource;

/**
 *
 * \brief  设置图片解码内存申请的优先级，即优先从level申请
 * \param  level     优先申请的内存源
 * \param  size      优先申请的内存源界限，当小于此值则优先从level定义的内存源申请，若大于此值则优先级倒置；当size为0时，则无倒置情况发生，永远按此优先级
 * \return
 * \reval 无
 */
void gal_img_memory_set_priority(ImageMemorySource level, int size);

int gal_img_desc_mirror(void *screen, const image_desc *pDesc, int x, int y, int w, int h, GxReverseMode mode);
int gal_img_stretch(GuiSurface *surface, const image_desc *pDesc, GAL_Rect *src_rect, GAL_Rect *dst_rect);
int gal_set_clut       (void *screen, const image_desc * pDesc);
int gal_set_logic_clut (void *screen, const image_desc * pDesc);
int gal_background_desc(void *screen, const image_desc * pDesc);

int gal_get_clut_index(void);
int gal_set_clut_index(int pal_index);
image_desc *gal_get_background(void);

int gal_get_img_flag(void);
int gal_enable_img(int flag);
int gal_rgb2yuv(int color);

//void imgs_add_img(GuiImgs* img, const char* id, const char *value, int flag);
int image_convert2current(image_desc *image);
int GDI_ColorSpaceConvert(image_desc *image);

/*Special*/
image_desc *IMG_JPEG_Load_and_Zoom(const char *filename, int zw, int zh);
image_desc *IMG_JPEG_Load_and_Cut (const char *filename, int zw, int zh);


static inline unsigned short YUV2YUV422(int yuv, int index)
{
	if(index % 2)
		return ((yuv >> 8) & 0x0000FF00) | (yuv & 0x000000FF);
	else
		return yuv >> 8;
}

int check_cpu(void);

extern unsigned short FILE_Read16(const FILE* filename);
extern unsigned int FILE_Read32(const FILE* filename);
extern unsigned int FILE_Seek(const FILE* filename, int pos);

void convert_rgb_2_yuv422(void* pin, void* pout, unsigned int width, unsigned int height, unsigned int bpp, void *pclut);

/* BMP */
image_desc *IMG_BMP_Load(image_desc *desc, const char *filename);

int IMG_BMP_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
#ifndef NO_OS
int IMG_BMP_GetInfo(const char *filename, Image_Exif *img_info);
#endif /*NO_OS*/
int IMG_BMP_STOP(void);

/* JPEG */
image_desc *IMG_JPEG_Load(image_desc *desc, const char *filename);
int IMG_JPEG_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
#ifndef NO_OS
int IMG_JPEG_GetInfo(const char *filename, Image_Exif *img_info);
void IMG_GrayLevelEnable(bool flag);
#endif /*NO_OS*/
int IMG_JPEG_STOP(void);

image_desc *IMG_JPEG_SOFT_Load(image_desc *desc, const char *filename);
image_desc *IMG_JPEG_NO_Zoom_Load(image_desc *img_desc, const char *pFile);
int IMG_JPEG_SOFT_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
int IMG_JPEG_SOFT_STOP(void);

/* PNG */
image_desc *IMG_PNG_Load(image_desc *dec, const char *filename);
int IMG_PNG_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
#ifndef NO_OS
int IMG_PNG_GetInfo(const char *filename, Image_Exif *img_info);
#endif /*NO_OS*/

/* GIF */
typedef struct _GIF_Image {
	int img_top;
	int img_left;
	int img_width;
	int img_height;
	int disposal;
	int delaytime;
	int num_color;
	int enable_trans;
	int color_key_index;
	Color img_trans;
	Color *img_pal;
	unsigned char *img_data;
	GxHwMallocObj *img_vq;
	struct _GIF_Image *next;
} GIF_Image;

typedef struct _GIF_Para {
	int screen_width;
	int screen_height;
	int bpp;
	int num_color;
	int frame_num;
	Color *pal;
	Color backcolor;
	GIF_Image *each_image;
	GIF_Image *cur_image;
} GIF_Para;

image_desc *IMG_GIF_Load(image_desc *desc, const char *filename);
int IMG_GIF_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
void draw_gif_desc(GuiSurface *surface, image_desc *gif_desc, uint32_t x, uint32_t y);
int IMG_GIF_Stop(void);
int IMG_GIF_Free(void *data);
#ifndef NO_OS
int IMG_GIF_GetInfo(const char *filename, Image_Exif *img_info);
#endif /*NO_OS*/

/* TIFF */
image_desc *IMG_TIFF_Load(image_desc *desc, const char *filename);
int IMG_TIFF_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
#ifndef NO_OS
int IMG_TIFF_GetInfo(const char *filename, Image_Exif *img_info);
#endif /*NO_OS*/

/* MPEG */
image_desc *IMG_MPEG_Load(image_desc *desc, const char *filename);
int IMG_MPEG_Draw(void *screen, const char *filename, int x0, int y0, int screen_width, int screen_height);
#ifndef NO_OS
int IMG_MPEG_GetInfo(const char *filename, Image_Exif *img_info);
#endif /*NO_OS*/

/*WEBP*/
image_desc *IMG_WEBP_Load(image_desc *webp_desc, const char *pFile);
int IMG_WEBP_Draw(void *screen, const char *pWEBP, int x0, int y0, int screen_width, int screen_height);
/* Image Play Mode */
typedef enum _ImagePlayMode {
	IMAGE_EFFECT_NONE            ,
	IMAGE_SLIDE_LEFT             ,
	IMAGE_SLIDE_RIGHT            ,
	IMAGE_SLIDE_TOP              ,
	IMAGE_SLIDE_BOTTOM           ,
	IMAGE_DIAGONAL_TOP_LEFT      ,
	IMAGE_DIAGONAL_TOP_RIGHT     ,
	IMAGE_DIAGONAL_BOTTOM_LEFT   ,
	IMAGE_DIAGONAL_BOTTOM_RIGHT  ,
	IMAGE_HSTEP_TOP_LEFT         ,
	IMAGE_HSTEP_TOP_RIGHT        ,
	IMAGE_HSTEP_BOTTOM_LEFT      ,
	IMAGE_HSTEP_BOTTOM_RIGHT     ,
	IMAGE_CHECKER_BOARD          ,
	IMAGE_EFFECT_MAX             ,
} ImagePlayMode;
#ifndef NO_OS
int IMG_WEBP_GetInfo(const char *pFile, Image_Exif *img_info);
int IMG_WEBP_STOP(void);

int IMG_Set_PlayMode(ImagePlayMode mode);
ImagePlayMode IMG_Get_PlayMode(void);

#endif /*NO_OS*/

 /** @}*/

__END_DECLS

#endif  /*end __IMG_H__*/


