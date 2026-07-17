/**
 *
 * @file        gxsubtitle_display_pc.c
 * @brief
 * @version     1.1.0
 * @date        04/08/2010 06:06:57 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gx_mem.h"
#include "sub_render.h"
#include "sub_debug.h"

#if ((SIMULATOR  & RENDER_SIMULATOR) == RENDER_SIMULATOR)


static  void *my_screen;
extern void hd_putpixel(void *screen, int x, int y, unsigned int pixel);
extern int hd_init(void);
extern int SDL_Init(uint32_t flags);
extern SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags);
extern int hd_set_clut(void *surface, Color * colors, int firstcolor, int ncolors);

#define MY_SDL_INIT_VIDEO		0x00000020
#define MY_SDL_HWSURFACE	    0x00000001
#define MY_CLUT_NUM             256



void     GxSubtitle_DisplayInit      (void)
{
    extern int hd_init(void);
    extern GuiCore gui;
    gui.config.width = 720;
    gui.config.height = 576;
    gui.config.bpp = 8;
    gui.config.clut = av_malloc(sizeof(GuiClut));
    gui.config.clut->num = 256;
    gui.config.clut->palette = av_malloc(256*sizeof(int));

    gui.config.clut->palette[0] = 0;
    gui.config.clut->palette[1] = (113 << 24) + (170 << 16) + (234 << 8) + 234;


    hd_init();
    return;
}
void        GxSubtitle_DisplayDestroy   (void)
{
}
void        GxSubtitle_DisplayCtrl      (bool                       Enable)
{
}
handle_t    GxSubtitle_BitmapCreate     (handle_t                   Display,
                                        GxSubtitle_DisplayRect*     Rect)
{
    return E_INVALID_HANDLE;
}

int32_t GxSubtitle_BitmapClear(handle_t                Bitmap)
{
    return E_OK;
}

int32_t     GxSubtitle_BitmapDisplay    (handle_t                   Bitmap,
                                         bool                       Enable)
{
    return E_FAILURE;
}
int32_t     GxSubtitle_BitmapClose      (handle_t                   Bitmap)
{
     return E_FAILURE;
}
int32_t     GxSubtitle_BitmapClutSet    (handle_t                   Bitmap,
                                        uint32_t                    ClutId,
                                        GxSubtitle_Color*           Clut,
                                        size_t                      size)
{
    int32_t i;
    int32_t num = size/sizeof(GxSubtitle_Color);
    Color a[256];

    for (i = 0; i< num; i++) {
        a[i] = yuv2rgb(Clut[i].y, Clut[i].cb, Clut[i].cr);
    }

    hd_set_clut(my_screen, (int*)a, 0, num);

    return E_FAILURE;
}
int32_t     GxSubtitle_BitmapRegionFill (handle_t                   Bitmap,
                                        GxSubtitle_DisplayRect*     Rect,
                                        uint32_t                    ClutId,
                                        int32_t                     ColorIndex)
{
    int x, y;
    for (x = Rect->x; x < Rect->width; x++) {
        for (y = Rect->y; y <Rect->height; y++) {
            hd_putpixel(my_screen, x, y, ColorIndex);
        }
    }
    return E_OK;
}
int32_t     GxSubtitle_BitmapDrawLine   (handle_t                   Bitmap,
                                        int32_t                     X,
                                        int32_t                     Y,
                                        int32_t                     Len,
                                        uint32_t                    ClutId,
                                        int32_t                     ColorIndex)
{
    int32_t i;
    for (i = X; i < (X+Len); i++) {
        hd_putpixel(my_screen, i, Y, ColorIndex);
    }
    return E_OK;
}

#endif
