/**
 *
 * @file        gxsubtitle_display.c
 * @brief
 * @version     1.1.0
 * @date        04/08/2010 02:41:05 PM
 * @author      Li Xiaobin (lixb), lixb@nationalchip.com
 *
 */
#include "gx_mem.h"
#include "sub_render.h"
#include "gxavdev.h"
#include "sub_debug.h"
#include "subtitle/gxsubtitle.h"
#include "module/config/gxconfig.h"
#include "gdi_core.h"

#if ((SIMULATOR  & RENDER_SIMULATOR) != RENDER_SIMULATOR)
#define USED_DISPLAY_MEM    (TRUE)

struct gxsub_spp {
    int32_t                 dev;
    handle_t                spp;
    void*                   surface;
    GuiSurface*             gui_surface;
    GxSubtitle_RenderRect   rect;
    GxSubtitle_RenderRect   rect_max;
    GxSubtitle_RenderClut   clut;
    void*                   old_surface;
    bool                    old_top;
    bool                    old_enable;
    bool                    ntsc_flag;
    GxSubtitle_RenderRect   Recoverrect;
    uint16_t*               buf;
};

static int32_t spp_clear(handle_t handle)
{
    GuiSurface *spp_surface;
    struct gxsub_spp*               spp;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    spp_surface = spp->gui_surface;
#if (!USED_DISPLAY_MEM)
    {
        int32_t                         ret;
        GxVpuProperty_FillRect          fill;
        memset(&fill,0,sizeof(GxVpuProperty_FillRect));
        fill.is_ksurface    = spp_surface->hw;
        fill.surface        = spp_surface->hw_surface;
        fill.rect.x         = rect->x;
        fill.rect.y         = rect->y;
        fill.rect.width     = rect->width;
        fill.rect.height    = rect->height;
        fill.color.y        = 16;
        fill.color.cb       = 128;
        fill.color.cr       = 128;
        fill.color.alpha    = 0;
        ret = GxAVSetProperty(spp->dev,
                spp->spp,
                GxVpuPropertyID_FillRect,
                &fill,
                sizeof(GxVpuProperty_FillRect));
        ASSERT(ret == GXCORE_SUCCESS);
    }
#else
    {
        int i = 0;
        int max = ((spp->rect.width) * (spp->rect.height)) / 2;
        uint32_t *buf = (uint32_t*)spp->buf;
        for (i = 0; i < max; i++)
        {
            buf[i] = 0x20022002;
        }
    }
#endif
    return E_OK;
}


static handle_t spp_open(GxSubtitle_RenderRect* rect, GxSubtitle_RenderRect* rect_max)
{
    int32_t                         ret;
    struct gxsub_spp*               spp;
    GAL_Rect gui_rect = {0};
    GuiSurface *spp_surface = NULL;
    GxVpuProperty_ColorFormat       format;
    GxVpuProperty_LayerMainSurface  layer;
    GxVpuProperty_LayerEnable       enable;
    GxVpuProperty_LayerOnTop        top;

    spp = (struct gxsub_spp*) CALLOC(1, sizeof(struct gxsub_spp));
    if (spp == NULL){
        return E_INVALID_HANDLE;
    }

    GxAvdev_SppLock();
    spp->dev    = GxAvdev_CreateDevice(0);
    spp->spp    = GxAvdev_OpenModule(spp->dev, GXAV_MOD_VPU, 0);

    spp->rect.x      = rect->x;
    spp->rect.y      = rect->y;
    spp->rect.width  = rect->width;
    spp->rect.height = rect->height;

    spp->rect_max.x      = rect_max->x;
    spp->rect_max.y      = rect_max->y;
    spp->rect_max.width  = rect_max->width;
    spp->rect_max.height = rect_max->height;

    layer.layer = GX_LAYER_SPP;
    ret = GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerMainSurface,
            &layer,
            sizeof(GxVpuProperty_LayerMainSurface));
    ASSERT(ret == GXCORE_SUCCESS);
    spp->old_surface = layer.surface;

    top.layer  = GX_LAYER_SPP;
    ret = GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop,
            &top,
            sizeof(GxVpuProperty_LayerOnTop));
    ASSERT(ret == GXCORE_SUCCESS);
    spp->old_top = top.enable;

    enable.layer  = GX_LAYER_SPP;
    ret = GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &enable,
            sizeof(GxVpuProperty_LayerEnable));
    ASSERT(ret == GXCORE_SUCCESS);
    spp->old_enable = enable.enable;

    gui_rect.x = rect->x;
    gui_rect.y = rect->y;
    gui_rect.w = rect->width;
    gui_rect.h = rect->height;
    spp_surface = hd_surface_clone(GX_LAYER_SPP, &gui_rect, 2);

    spp->surface = spp_surface->hw_surface;
    spp->buf = (void *)spp_surface->data;
    spp->gui_surface = spp_surface;

    format.surface  = spp_surface->hw_surface;
    format.format   = GX_COLOR_FMT_YCBCRA6442;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_ColorFormat,
            &format,
            sizeof(GxVpuProperty_ColorFormat));

    ASSERT(ret == GXCORE_SUCCESS);
    GxVpuProperty_LayerViewport     LayerViewPortForRecover;
    LayerViewPortForRecover.layer = GX_LAYER_SPP;
    ret = GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortForRecover,
            sizeof(GxVpuProperty_LayerViewport));

    ASSERT(ret == GXCORE_SUCCESS);
    spp->Recoverrect.x = LayerViewPortForRecover.rect.x;
    spp->Recoverrect.y = LayerViewPortForRecover.rect.y;
    spp->Recoverrect.width = LayerViewPortForRecover.rect.width;
    spp->Recoverrect.height = LayerViewPortForRecover.rect.height;
    spp_clear((handle_t)spp);
    GxAvdev_SppUnlock();

    return (handle_t)spp;
}

static int32_t spp_close(handle_t handle)
{
    int32_t                         ret;
    GuiSurface			    *spp_surface;
    struct gxsub_spp*               spp;
    GxVpuProperty_LayerEnable       enable;
    GxVpuProperty_LayerOnTop        top;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    GxAvdev_SppLock();
    spp_surface = spp->gui_surface;

    hd_free_surface(spp_surface);
    hd_set_layer_surface(0);//0, LAYER_MAIN_SPP_SURFACE

#if 0
    if(spp_screen)
    {
        if(spp_screen->hw_surface == spp->old_surface)
        {
            hd_clear(GX_LAYER_SPP);
            hd_commit_blit();// import for new driver, UI and AV share the same memory hole.
        }

        //gxlogd("\nSubt, close2, old surface = 0x%x, gui surface = 0x%x\n", spp->old_surface,spp_screen->hw_surface);
        if(spp->old_surface)
        {
            layer.layer      = GX_LAYER_SPP;
            layer.surface    = spp_screen->hw_surface;//spp->old_surface;
            ret = GxAVSetProperty(spp->dev, spp->spp,
                    GxVpuPropertyID_LayerMainSurface,
                    &layer,
                    sizeof(GxVpuProperty_LayerMainSurface));
            ASSERT(ret == GXCORE_SUCCESS);
        }
    }
#endif

    top.layer  = GX_LAYER_SPP;
    top.enable = spp->old_top;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop, (void*)&top,
            sizeof(GxVpuProperty_LayerOnTop));
    ASSERT(ret == GXCORE_SUCCESS);

    enable.enable        = spp->old_enable;
    enable.layer         = GX_LAYER_SPP;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &enable,
            sizeof(GxVpuProperty_LayerEnable));
    ASSERT(ret == GXCORE_SUCCESS);

    GxVpuProperty_LayerViewport     LayerViewPortForRecover = {0};
    LayerViewPortForRecover.layer = GX_LAYER_SPP;
    LayerViewPortForRecover.rect.x = spp->Recoverrect.x;
    LayerViewPortForRecover.rect.y = spp->Recoverrect.y;
    LayerViewPortForRecover.rect.width = spp->Recoverrect.width;
    LayerViewPortForRecover.rect.height = spp->Recoverrect.height;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortForRecover,
            sizeof(GxVpuProperty_LayerViewport));

    ASSERT(ret == GXCORE_SUCCESS);
    GxAvdev_SppUnlock();

    GxAvdev_CloseModule(spp->dev, spp->spp);
    GxAvdev_DestroyDevice(spp->dev);
    FREE(spp);

    return E_OK;
}


int32_t spp_set_color(handle_t handle, uint8_t color_index, GxSubtitle_RenderColor* color)
{
    struct gxsub_spp*               spp;
    uint16_t                        value;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    value = ((color->color.ycc.y & 0xFC)<<8 )
        |((color->color.ycc.cb & 0xF0)<<2)
        |((color->color.ycc.cr & 0xF0)>>2)
        |((color->color.ycc.t&0xC0) >> 6);

    spp->clut.entries_list[color_index].color.value = ((value>>8)&0xff)|((value&0xff)<<8);
    return E_OK;
}


static int32_t spp_show(handle_t handle)
{
    int32_t                         ret;
    GxVpuProperty_LayerMainSurface  layer;
    GxVpuProperty_LayerEnable       p;
    GxVpuProperty_LayerOnTop        top;
    struct gxsub_spp*               spp;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    layer.layer      = GX_LAYER_SPP;
    layer.surface    = spp->gui_surface->hw_surface;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerMainSurface,
            &layer,
            sizeof(GxVpuProperty_LayerMainSurface));
    ASSERT(ret == GXCORE_SUCCESS);

    GxVpuProperty_LayerViewport     LayerViewPort = {0};
    GxVpuProperty_VirtualResolution Resolution = {0};
    ret = GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));
    ASSERT(ret == GXCORE_SUCCESS);

    LayerViewPort.layer = GX_LAYER_SPP;
    LayerViewPort.rect.x = 0;
    LayerViewPort.rect.y = 0;
    LayerViewPort.rect.width = Resolution.xres;
    LayerViewPort.rect.height = Resolution.yres;

    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPort,
            sizeof(GxVpuProperty_LayerViewport));
    ASSERT(ret == GXCORE_SUCCESS);

    top.layer  = GX_LAYER_SPP;
    top.enable = 1;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop, (void*)&top,
            sizeof(GxVpuProperty_LayerOnTop));
    ASSERT(ret == GXCORE_SUCCESS);

    p.enable        = 1;
    p.layer         = GX_LAYER_SPP;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &p,
            sizeof(GxVpuProperty_LayerEnable));
    ASSERT(ret == GXCORE_SUCCESS);


    return E_OK;
}

static int32_t spp_hide(handle_t handle)
{
    int32_t                         ret;
    GxVpuProperty_LayerEnable       p;
    struct gxsub_spp*           spp;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    p.enable        = 0;
    p.layer         = GX_LAYER_SPP;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &p,
            sizeof(GxVpuProperty_LayerEnable));
    ASSERT(ret == GXCORE_SUCCESS);

    return E_OK;
}





static int32_t spp_draw_line(handle_t handle, GxSubtitle_RenderLine* line, uint8_t color_index)
{
    struct gxsub_spp*           spp;
    GxSubtitle_RenderColor*     color;

    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }
    color = spp->clut.entries_list;
    if (spp->ntsc_flag) {
        if (line->start_point.y > 240) {
            line->start_point.y -= 140;
        } else {
            line->start_point.y += 35;
        }
    }
#if (!USED_DISPLAY_MEM)
    {
        int32_t                     ret;
        GxVpuProperty_DrawLine      p;

        spp = (struct gxsub_spp*)handle;
        if (spp == NULL) {
            return E_FAILURE;
        }

        p.surface = spp->gui_surface->hw_surface;
        p.start.x = line->start_point.x;
        p.start.y = line->start_point.y + spp->y_offset;
        p.end.x   = line->start_point.x + line->len;
        p.end.y   = line->start_point.y;
        p.width   = line->thickness;

        p.color.y           = color[color_index].ycc.y;
        p.color.cb          = color[color_index].ycc.cb;
        p.color.cr          = color[color_index].ycc.cr;
        p.color.alpha       = ~(color[color_index].ycc.t);


        ret = GxAVSetProperty(spp->dev,
                spp->spp,
                GxVpuPropertyID_DrawLine,
                &p,
                sizeof(GxVpuProperty_DrawLine));
        ASSERT(ret == GXCORE_SUCCESS);
    }
#else
    {
        unsigned int len, width;
        int x,y;
        uint16_t* buf;
        uint32_t c = color[color_index].color.value;

        width = line->thickness + line->start_point.y;
        len = line->len + line->start_point.x;
        for (y = line->start_point.y; y < width; y++) {
            buf = spp->buf + (spp->rect.width)*y;
            for (x = line->start_point.x; x < len; x++) {
                //point_ycc(spp->buf, x, y, c);
                buf[x] = c;
            }
        }
    }
#endif
    return E_OK;
}



static int32_t spp_fill_region(handle_t handle, GxSubtitle_RenderRect* rect, uint8_t color_index)
{
    GxSubtitle_RenderLine line;
    line.start_point.x = rect->x;
    line.start_point.y = rect->y;
    line.thickness = rect->height;
    line.len = rect->width;
    spp_draw_line(handle, &line, color_index);
    return E_OK;
}

static void spp_set_ntsc(handle_t handle, bool flag)
{
    struct gxsub_spp*           spp = (struct gxsub_spp*)handle;;

    if (spp == NULL) {
        return;
    }

    spp->ntsc_flag = FALSE;
    return;
}

static bool spp_get_ntsc(handle_t handle)
{
    struct gxsub_spp*           spp = (struct gxsub_spp*)handle;;

    if (spp == NULL) {
        return FALSE;
    }

    return spp->ntsc_flag;
}

static void* spp_get_buf(handle_t handle)
{

    struct gxsub_spp*           spp = (struct gxsub_spp*)handle;;

    if (spp == NULL) {
        return NULL;
    }

    return spp->buf;

}

static int32_t spp_change_buf(handle_t handle, GxSubtitle_RenderRect* rect)
{
    int32_t                         ret;
    GxVpuProperty_ColorFormat       format;
    GAL_Rect                        gui_rect;
    struct gxsub_spp*               spp;
    GxVpuProperty_LayerMainSurface  layer;

    //Detroy the old surface
    spp = (struct gxsub_spp*)handle;
    if (spp == NULL) {
        return E_FAILURE;
    }

    GxAvdev_LayerEnable(spp->dev, spp->spp, GX_LAYER_SPP, 0);

    hd_free_surface(spp->gui_surface);
    spp->gui_surface = NULL;

    gui_rect.x = rect->x;
    gui_rect.y = rect->y;
    gui_rect.w = rect->width;
    gui_rect.h = rect->height;

    spp->rect = *rect;

    //Create New Buffer
    spp->gui_surface = hd_surface_clone(GX_LAYER_SPP, &gui_rect, 2);

    spp->buf = (void *)spp->gui_surface->data;
    spp->surface = spp->gui_surface->hw_surface;

    spp_clear(handle);  //clear the data of new spp buf!

    format.surface  = spp->surface;
    format.format   = GX_COLOR_FMT_YCBCRA6442;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_ColorFormat,
            &format,
            sizeof(GxVpuProperty_ColorFormat));
    ASSERT(ret == GXCORE_SUCCESS);

    layer.layer      = GX_LAYER_SPP;
    layer.surface    = spp->surface;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerMainSurface,
            &layer,
            sizeof(GxVpuProperty_LayerMainSurface));
    ASSERT(ret == GXCORE_SUCCESS);
    {
        GxVpuProperty_LayerViewport     LayerViewPort = {0};
        GxVpuProperty_VirtualResolution Resolution = {0};
        ret = GxAVGetProperty(spp->dev, spp->spp,
                GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));
        ASSERT(ret == GXCORE_SUCCESS);

        LayerViewPort.layer = GX_LAYER_SPP;
        LayerViewPort.rect.x = 0;
        LayerViewPort.rect.y = 0;
        LayerViewPort.rect.width = Resolution.xres;
        LayerViewPort.rect.height = Resolution.yres;

        ret = GxAVSetProperty(spp->dev, spp->spp,
                GxVpuPropertyID_LayerViewport,
                &LayerViewPort,
                sizeof(GxVpuProperty_LayerViewport));
        ASSERT(ret == GXCORE_SUCCESS);
    }

    GxAvdev_LayerEnable(spp->dev, spp->spp, GX_LAYER_SPP, 1);

    //spp_clear(handle);  //clear the data of new spp buf!
    return E_OK;
}


static GxSubtitle_RenderRect* spp_get_rect(handle_t handle)
{
    struct gxsub_spp*           spp = (struct gxsub_spp*)handle;;

    if (spp == NULL) {
        return NULL;
    }

    return &(spp->rect);
}

static GxSubtitle_RenderRect* spp_get_rect_max(handle_t handle)
{
    struct gxsub_spp*           spp = (struct gxsub_spp*)handle;;

    if (spp == NULL) {
        return NULL;
    }

    return &(spp->rect_max);
}

GxSubtitle_RenderOps gxsub_spp_render =
{
    spp_open,
    spp_close,
    spp_set_color,
    spp_clear,
    spp_show,
    spp_hide,
    spp_fill_region,
    spp_draw_line,
    spp_set_ntsc,
    spp_get_ntsc,
    spp_get_buf,
    spp_change_buf,
    spp_get_rect,
    spp_get_rect_max,
};

#endif
