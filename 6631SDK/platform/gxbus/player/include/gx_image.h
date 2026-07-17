#ifndef __GX_IMAGE_H__
#define __GX_IMAGE_H__


#define GX_MAX_PLANES	4

typedef enum 
{
    IMAGE_YUV,
    IMAGE_RGB
}GxImageFmt;

typedef struct  {
    unsigned short flags;
    unsigned char  type;
    unsigned char  bpp;  // bits/pixel. NOT depth! for RGB it will be n*8
    unsigned int   imgfmt;
	GxImageFmt     format;
    int            width,height;  // stored dimensions
    int            x,y,w,h;  // visible dimensions
    unsigned char  *planes[GX_MAX_PLANES];
    int            stride[GX_MAX_PLANES];
    char           *qscale;
    int            qstride;
    int            pict_type; // 0->unknown, 1->I, 2->P, 3->B
    int            fields;
    int            qscale_type; // 0->mpeg1/4/h263, 1->mpeg2
    int            num_planes;
    /* these are only used by planar formats Y,U(Cb),V(Cr) */
    int            chroma_width;
    int            chroma_height;
    int            chroma_x_shift; // horizontal
    int            chroma_y_shift; // vertical
    /* for private use by filter or vo driver (to store buffer id or dmpi) */
    void           *priv;
} GxImage;

#endif

