#include "gxbasic_graph.h"
#include "gdi.h"
#include <stdio.h>
#include <stdlib.h>


/*---------------OSD---------------*/

int BG_OSD_On(void)
{
    int ret = 0;
    
    ret = gdi_enable(1);

    return (ret);
}

int BG_OSD_Off(void)
{
    int ret = 0;

    ret = gdi_enable(0);

    return (ret);
}


int BG_OSD_Rect(BG_Rect *rect, int color)
{
    int ret = 0;
    
    gdi_fillrect(rect, color);

    return (ret);
}

int BG_OSD_Matrix(void *data, BG_Rect *rect, int byteperline, int color)
{
    int ret = 0;
    
    if(NULL == rect)
    {
	return (1);
    }
    
    ret = gdi_draw_matrix(data, rect, byteperline, color);

    return (ret);
}

int BG_OSD_Image(void *data, BG_Rect *rect, int bpp)
{
    int ret = 0;
    
    if((NULL == data) || (NULL == rect))
    {
	return (1);
    }
    
    ret = gdi_image_data(data, rect->x, rect->y, rect->w, rect->h, bpp);

    return (ret);
}

int BG_OSD_Pixel(int x, int y, int color)
{
    int ret = 0;

    ret = gdi_setpixel(x, y, color);

    return (ret);
}

int BG_OSD_SetCLUT(void *data, int number)
{
    int ret = 0;

    ret = gdi_set_clut(data, number);

    return (ret);
}


void *BG_OSD_GetCLUT(void)
{
    void *data = NULL;
    
    data = gdi_get_clut();

    return (data);
}


/*---------------SPP---------------*/

int BG_SPP_On(void)
{
    int ret = 0;

    ret = gdi_image_enable(1);

    return (ret);
}


int BG_SPP_Off(void)
{
    int ret = 0;
    
    ret = gdi_image_enable(0);

    return (ret);
}

int BG_SPP_Rect(BG_Rect *rect, int color)
{
    int ret = 0;
    
    ret = gdi_image_rect(rect, color);

    return (ret);
}


int BG_SPP_Pixel(int x, int y, int color)
{
    int ret = 0;

    gdi_image_pixel(x, y, color); 

    return (ret);
}

int BG_SPP_Image(void *data, BG_Rect *rect)
{
    int ret = 0;

    if((NULL == data) || (NULL == rect))
    {
	return (1);
    }
    
	// 未定义的符号
    //ret = gdi_draw_image(data, rect);

    return (ret);
}


/*---------------BLIT---------------*/

int BG_Blit(void)
{
    int ret = 0;
    
    ret = gdi_begin();

    return (ret);
}

int BG_Begin(void)
{
    int ret = 0;
    
    ret = gdi_end();

    return (ret);
}




