#ifndef __IMG_GIF_H__
#define __IMG_GIF_H__

#include "gui_core.h"
#include "gxcore.h"

__BEGIN_DECLS

typedef enum
{
	BMP_TYPE,
	JPEG_TYPE,
	GIF_TYPE,
	PNG_TYPE,
	FILE_TYPE
}image_type;

typedef enum
{
    EFFECT_SRC_TO_DST = 0,
    EFFECT_COPY,
    EFFECT_SRC_AND_DST
}image_effect;

typedef struct _image_desc
{
	char *filename;
	void *data;
	void *pal;
	image_type type;
	int pal_index;
	int width;
	int height;
	int bpp;
	int bkgd;
	int color;
	image_effect effect;
}image_desc;

typedef struct _GIF_Image
{
	int img_top;
	int img_left;
	int img_width;
	int img_height;
	int num_color;
	int enable_trans;
	Color img_trans;
	Color *img_pal;
	unsigned char *img_data;
	struct _GIF_Image *next;
}GIF_Image;

typedef struct _GIF_Para
{
	int screen_width;
	int screen_height;
	int bpp;
	int num_color;
	Color *pal;
	GIF_Image *each_image;
	GIF_Image *cur_image;
}GIF_Para;

image_desc *IMG_GIF_Load(const unsigned char * pName, const unsigned char *pFile);
int IMG_GIF_Free(void *data);

__END_DECLS

#endif



