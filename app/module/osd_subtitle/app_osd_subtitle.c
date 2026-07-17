#include "app_config.h"
#if OSD_SUBTITLE_SUPPORT
#include "module/app_osd_subtitle.h"
#include "module/app_log.h"
#include "app_main_menu_theme_type.h"

extern int cp125x2utf8(const unsigned char *from_str, unsigned char **to_str,unsigned int from_size,unsigned int *to_size,unsigned char codepage);

#define MAX_COLOR_NODE  (20)
#ifndef GX_TXT_MAX_SUB_LINE
#define GX_TXT_MAX_SUB_LINE         (12)
#endif

typedef struct
{
    struct gxlist_head list;
    uint32_t count;
    uint32_t rgb888;
}SubtColorNode;

typedef struct _PixelSubtCtrl
{
    GuiSurface             *surface;
    GXGDI_Rect              rect;
    SubtColorNode           color_node[MAX_COLOR_NODE];
    GxSubtitleShowDisplay   pos;
    unsigned int            color;
}PixelSubtCtrl;

typedef struct _TextSubtCtrl
{
    OsdSubtEncoding encoding;
    OsdSubtCodePage codepage;
    OsdSubtFont     font;
}TextSubtCtrl;

static GxSubtitleShowTextOps TextOps = {0};
static GxSubtitleShowPixelOps PixelOps = {0};

typedef struct _OsdSubtCtrl
{
    GuiSurface     *surface;
    char           *layer_name;
    GXGDI_Rect      rect;
    uint32_t        x;
    uint32_t        y;
    bool            init;
    bool            pause;
    handle_t        mutex;
    OsdSubtColor    color;
    OsdSubtType     type;
    PixelSubtCtrl   pixel;
    TextSubtCtrl    text;
}OsdSubtCtrl;

static OsdSubtCtrl s_OsdSubtCtrl = {
    .surface = NULL,
    .layer_name = NULL,
    .x = 0,
    .y = 0,
    .init = false,
    .pause = false,
    .mutex = GXCORE_INVALID_POINTER,
    .type = PIXEL_SUBT,
};

#define SUBT_SURFACE_FREE(x) if(x){hd_free_surface(x);x=NULL;}
#define SUBT_LAYER_UNREGISTER(x) if(x){GxGDI_LayerUnregister(x);x=NULL;}

static handle_t _subt_text_open(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
    return 0;
}

static void _subt_text_close(handle_t handle)
{
    return;
}

static int _subt_text_show(void)
{
    return 0;
}

static int _subt_text_hide(void)
{
    return 0;
}

static inline int32_t _virtual_layer_surface_fill_defcolor(void)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GXGDI_Lock();
    GXGDI_Begin();
    if(ctrl->surface)
        ret |= GXGDI_FillRect(ctrl->surface, &ctrl->rect, ctrl->color.defcolor);
    GXGDI_End();
    if(ctrl->layer_name)
        ret |= GxGDI_LayerUpdate(ctrl->layer_name);
    GXGDI_Unlock();
    return ret;
}

static int _virtual_layer_surface_get(uint32_t width, uint32_t height)
{
    int ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    if(0 == width || 0 == height || width > APP_THEME_XRES || height > APP_THEME_YRES )
        return -1;

    //size of surface is right
    if(ctrl->surface != NULL && ctrl->rect.w >= width && ctrl->rect.h >= height)
        return 0;

    SUBT_LAYER_UNREGISTER(ctrl->layer_name);
    SUBT_SURFACE_FREE(ctrl->surface);

    if(ctrl->color.forecolor == ctrl->color.backcolor)
    {
        ctrl->color.forecolor = 0xf4f4f4;
        ctrl->color.backcolor = APP_UI_SUBT_BG;
        ctrl->color.defcolor = APP_UI_GUI_TRANS;
    }

    ctrl->rect.w = width;
    ctrl->rect.h = height;
    ctrl->surface = hd_get_surface((GAL_Rect *)&ctrl->rect, APP_UI_THEME_BPP, NULL, false);
    if(NULL == ctrl->surface)
    {
        app_log_error("Create surface failed.\n");
        goto err;
    }

    ctrl->x = (APP_THEME_XRES - width) / 2;
    ctrl->y = APP_THEME_YRES - height - 50;
    GXGDI_Lock();
    GXGDI_Begin();
    ret |= GXGDI_FillRect(ctrl->surface, &ctrl->rect, ctrl->color.defcolor);
    GXGDI_End();
    GXGDI_Unlock();
    ctrl->layer_name = GxGDI_LayerRegister(ctrl->surface, ctrl->x, ctrl->y);
    if(NULL == ctrl->layer_name)
    {
        app_log_error("Register surface failed.\n");
        goto err;
    }
    GXGDI_Lock();
    if(ctrl->pause == true)
    {
        GxGDI_LayerEnable(ctrl->layer_name, false);
    }
    ret |= GxGDI_LayerUpdate(ctrl->layer_name);
    GXGDI_Unlock();

    return ret;
err:
    memset(&ctrl->rect, 0, sizeof(GXGDI_Rect));
    SUBT_LAYER_UNREGISTER(ctrl->layer_name);
    SUBT_SURFACE_FREE(ctrl->surface);
    return -1;
}

static int32_t _subt_text_draw(handle_t handle, GxSubtitleShowTextPage *txt)
{
    unsigned int relen = 0;
    unsigned char *iconv_str[GX_TXT_MAX_SUB_LINE] = {0};
    GXGDI_Rect fill_rect[GX_TXT_MAX_SUB_LINE];
    char encoding_name[16] = {0}, subt_font[10] = {0};
    int32_t height = 0, width = 0, i = 0, font_height = 0;
    uint32_t start_x = 0, start_y = 0;
    int add_long_txt_line= 1;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;
    TextSubtCtrl *text = &ctrl->text;

    if(NULL == txt)
        return -1;

    GxCore_MutexLock(ctrl->mutex);
    if(ctrl->pause == true)
        goto err;

    ctrl->type = TEXT_SUBT;

    if(txt->lines.lines > GX_TXT_MAX_SUB_LINE)
        txt->lines.lines = GX_TXT_MAX_SUB_LINE;
    GXGDI_Lock();

    /*ISO8859*/
    if(OSDSUBT_ENCODING_ISO8859 == text->encoding)
    {
        gdi_encoding_convert("iso6937", NULL);
        sprintf(encoding_name, "iso8859-%02d", text->codepage.isocode);
        if(text->codepage.isocode >= 1 && text->codepage.isocode <= 16)
            gdi_encoding_convert("iso6937", encoding_name);
    }

    if(OSDSUBT_FONT_MAIN == text->font)
        strcpy(subt_font, "font_main");
    else if(OSDSUBT_FONT_22 == text->font)
        strcpy(subt_font, "font_22");
    else
        strcpy(subt_font, "font_prog");

    GXGDI_SetFont((const unsigned char*)subt_font);
    font_height = GXGDI_GetFontHeight(GXGDI_GetFont());

    for(i = 0; i < txt->lines.lines; i++)
    {
        uint32_t w = 0;

        if(NULL == txt->lines.text[i])
            continue;

        if(OSDSUBT_ENCODING_ISO8859 == text->encoding)
        {
            /*iso8859*/
            if(gxgdi_iconv((const unsigned char *)(txt->lines.text[i]), &iconv_str[i], strlen(txt->lines.text[i]), &relen, NULL) != 0)
            {
                GXGDI_Unlock();
                goto err;
            }

            w = GXGDI_GetStringPixels(iconv_str[i]);
        }
        else if(OSDSUBT_ENCODING_WINDOWS125X == text->encoding)
        {
            /*window 125x*/
            if(cp125x2utf8((const unsigned char *)(txt->lines.text[i]), &iconv_str[i], strlen(txt->lines.text[i]), &relen, text->codepage.wincode) != 0)
            {
                GXGDI_Unlock();
                goto err;
            }

            w = GXGDI_GetStringPixels(iconv_str[i]);
        }
        else
        {
            w = GXGDI_GetStringPixels(txt->lines.text[i]);
        }

        if(w > width)
            width = w;
    }
    GXGDI_Unlock();
    if(width > (APP_THEME_XRES-APP_THEME_XRES/10))
    {
        add_long_txt_line=((width-1)/(APP_THEME_XRES-400)+1);
        width=APP_THEME_XRES-400;
        height = font_height * txt->lines.lines * add_long_txt_line;
    }
    else
    {
        height = font_height * txt->lines.lines;
    }
    if(width <= 0 || height <= 0 || height > APP_YRES)
        goto err;
    if(_virtual_layer_surface_get(width, height) < 0)
        goto err;

    start_x = (ctrl->rect.w - width) / 2;
    start_y = (ctrl->rect.h - height) / 2;

    memset(fill_rect, 0, sizeof(GXGDI_Rect) * GX_TXT_MAX_SUB_LINE);
    for(i = 0; i < txt->lines.lines; i++)
    {
        //width of display text subt area equal the longest text len
        fill_rect[i].x = start_x;
        fill_rect[i].y = start_y + font_height * i * add_long_txt_line;
        fill_rect[i].w = width;
        fill_rect[i].h = font_height * add_long_txt_line;
    }

    GXGDI_Lock();
    GXGDI_Begin();
    for(i = 0; i < txt->lines.lines; i++)
    {
        if(NULL == txt->lines.text[i])
            continue;

#if OSD_TRANS_BG_SUPPORT
        GXGDI_FillRect(ctrl->surface, &fill_rect[i], APP_UI_OSD_TRANS);
#else
        GXGDI_FillRect(ctrl->surface, &fill_rect[i], ctrl->color.backcolor);
#endif
        if(OSDSUBT_ENCODING_UTF8 == text->encoding)
        {
            GXGDI_DrawString(ctrl->surface, txt->lines.text[i], &fill_rect[i], ALIGNMENT_HCENTRE | ALIGNMENT_VCENTRE, ctrl->color.forecolor, GDI_STRING_PARAGRAPH);
        }
        else
        {
            GXGDI_DrawString(ctrl->surface, iconv_str[i], &fill_rect[i], ALIGNMENT_HCENTRE | ALIGNMENT_VCENTRE, ctrl->color.forecolor, GDI_STRING_PARAGRAPH);
            GxCore_Free(iconv_str[i]);
        }
    }
    GXGDI_End();
    GxGDI_LayerUpdate(ctrl->layer_name);
    GXGDI_Unlock();

    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
err:
    GxCore_MutexUnlock(ctrl->mutex);
    return -1;
}

static int _subt_text_clean(handle_t handle)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
	GXGDI_Lock();
    if(NULL != ctrl->layer_name)
        ret = GxGDI_LayerEnable(ctrl->layer_name, false);
	GXGDI_Unlock();
    ret = _virtual_layer_surface_fill_defcolor();
    SUBT_LAYER_UNREGISTER(ctrl->layer_name);
    SUBT_SURFACE_FREE(ctrl->surface);
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

static uint16_t _rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t r1, g1, b1;

    r1 = r & 0xf8;
    g1 = g & 0xfc;
    b1 = b & 0xf8;

    return (r1 << 8) | (g1 << 3) | (b1 >> 3);
}

static int32_t _color_node_add(uint32_t rgb888)
{
    int32_t i = 0, empty_pos = -1;
    PixelSubtCtrl *pixel = &s_OsdSubtCtrl.pixel;

    for(i = 0; i < MAX_COLOR_NODE; i++)
    {
        if(pixel->color_node[i].rgb888 == rgb888)
        {
            pixel->color_node[i].count++;
            return 0;
        }

        if(0 == pixel->color_node[i].count && -1 == empty_pos)
            empty_pos = i;
    }

    if(-1 == empty_pos)
    {
        int32_t min_count = pixel->color_node[0].count;

        for(i = 0; i < MAX_COLOR_NODE; i++)
        {
            if(pixel->color_node[i].count < min_count)
            {
                min_count = pixel->color_node[i].count;
                empty_pos = i;
            }
        }
    }

    pixel->color_node[empty_pos].rgb888 = rgb888;
    pixel->color_node[empty_pos].count++;

    return 0;
}

static void _background_color_get(uint8_t *br, uint8_t *bg, uint8_t *bb)
{
    int32_t i = 0;
    uint32_t second_frequent_color = 0;
    uint32_t most_count = 0, second_count = 0;
    PixelSubtCtrl *pixel = &s_OsdSubtCtrl.pixel;

    for(i = 0; i < MAX_COLOR_NODE; i++)
    {
        if(pixel->color_node[i].count > most_count)
            most_count = pixel->color_node[i].count;

        if(pixel->color_node[i].count > second_count && pixel->color_node[i].count < most_count)
        {
            second_count = pixel->color_node[i].count;
            second_frequent_color = pixel->color_node[i].rgb888;
        }
    }

    *br = ~(second_frequent_color >> 16);
    *bg = ~(second_frequent_color >> 8);
    *bb = ~(second_frequent_color);

    return;
}

static void _subt_data_color_count(void)
{
    int i = 0;
    PixelSubtCtrl *pixel = &s_OsdSubtCtrl.pixel;
    uint32_t *argb888 = (uint32_t *)pixel->surface->data;

    for(i = 0; i < pixel->rect.w * pixel->rect.h; i++)
    {
        _color_node_add(argb888[i]);
    }
}

static void _surface_fill_backcolor(void)
{
    int32_t i = 0;
    uint8_t br = 0, bg = 0, bb = 0;
    PixelSubtCtrl *pixel = &s_OsdSubtCtrl.pixel;
    uint8_t osd_trans_r = APP_UI_OSD_TRANS>>16&0xFF, osd_trans_g = APP_UI_OSD_TRANS>>8&0xFF, osd_trans_b = APP_UI_OSD_TRANS&0xFF;
    uint8_t gui_trans_r = APP_UI_GUI_TRANS>>16&0xFF, gui_trans_g = APP_UI_GUI_TRANS>>8&0xFF, gui_trans_b = APP_UI_GUI_TRANS&0xFF;
    uint16_t osd_trans_color = _rgb888_to_rgb565(osd_trans_r, osd_trans_g, osd_trans_b);
    uint16_t gui_trans_color = _rgb888_to_rgb565(gui_trans_r, gui_trans_g, gui_trans_b);
    uint32_t *argb888 = (uint32_t *)(pixel->surface->data);
    uint8_t close_osd_trans_g = osd_trans_g > 0xc ? osd_trans_g - 1:0xc;
    uint8_t close_gui_trans_g = gui_trans_g > 0xc ? gui_trans_g - 1:0xc;

    _background_color_get(&br, &bg, &bb);
    memset(pixel->color_node, 0, sizeof(SubtColorNode) * MAX_COLOR_NODE);

    if(osd_trans_color == _rgb888_to_rgb565(br, bg, bb))
        bg = close_osd_trans_g;
    else if(gui_trans_color == _rgb888_to_rgb565(br, bg, bb))
        bg = close_gui_trans_g;

    for(i = 0; i < pixel->rect.w * pixel->rect.h; i++)
    {
        if(0 == ((argb888[i] >> 24) & 0xff))
        {
#if OSD_TRANS_BG_SUPPORT
            argb888[i] = APP_UI_OSD_TRANS;
#else
            argb888[i] = (br << 16) | (bg << 8) | bb;
#endif
        }
        else
        {
            uint8_t r = pixel->surface->data[4 * i + 1];
            uint8_t g = pixel->surface->data[4 * i + 2];
            uint8_t b = pixel->surface->data[4 * i + 3];

            if(osd_trans_color == _rgb888_to_rgb565(r, g, b))
                pixel->surface->data[4 * i + 2] = close_osd_trans_g;
            else if(gui_trans_color == _rgb888_to_rgb565(r, g, b))
                pixel->surface->data[4 * i + 2] = close_gui_trans_g;
        }
    }
    return;
}


static handle_t  _subt_pixel_open(GxSubtitleShowCanvas *cv, GxSubtitleShowScreen *sr)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;
    return (handle_t)(&ctrl->pixel);
}

static void _subt_pixel_close(handle_t handle)
{
    return;
}

static int _subt_pixel_show(void)
{
    return 0;
}

static int _subt_pixel_hide(void)
{
    return 0;
}

static int32_t _subt_pixel_clean(handle_t handle)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
	GXGDI_Lock();
    if(NULL != ctrl->layer_name)
        ret = GxGDI_LayerEnable(ctrl->layer_name, false);
	GXGDI_Unlock();
    ret = _virtual_layer_surface_fill_defcolor();
    SUBT_LAYER_UNREGISTER(ctrl->layer_name);
    SUBT_SURFACE_FREE(ctrl->surface);
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

static uint32_t *_subt_pixel_alloc_argb8888_buffer(handle_t handle, uint32_t width, uint32_t height)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;
    PixelSubtCtrl *pixel = &ctrl->pixel;
    uint32_t vir_width = width, vir_height = height;

    GxCore_MutexLock(ctrl->mutex);
    if(ctrl->pause == true)
        goto err;
    ctrl->type = PIXEL_SUBT;
    if(width > APP_THEME_XRES)
    {
        vir_width = APP_THEME_XRES;
        vir_height = height > APP_THEME_YRES? APP_THEME_YRES: (height * APP_THEME_XRES / width);
    }

    if(height > APP_THEME_YRES)
    {
        vir_width = width > APP_THEME_XRES? APP_THEME_XRES: (width * APP_THEME_YRES / height);
        vir_height = APP_THEME_YRES;
    }

    if(_virtual_layer_surface_get(vir_width, vir_height) < 0)
        goto err;

    pixel->rect.x = 0;
    pixel->rect.y = 0;
    pixel->rect.w = width;
    pixel->rect.h = height;

    pixel->surface = hd_get_surface((GAL_Rect *)&pixel->rect, 32, NULL, TRUE);
    if(NULL == pixel->surface)
    {
        app_log_error("surface create failed(%d %d)\n", width, height);
        goto err;
    }

    GXGDI_Lock();
    GXGDI_Begin();
    GXGDI_FillRect(ctrl->pixel.surface, &pixel->rect, APP_UI_OSD_TRANS);
    GXGDI_End();
    GXGDI_Unlock();
    GxCore_MutexUnlock(ctrl->mutex);
    return (uint32_t *)pixel->surface->data;

err:
    GxCore_MutexUnlock(ctrl->mutex);
    return NULL;
}

static int _subt_pixel_free_argb8888_buffer(handle_t handle, unsigned int *buffer)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    SUBT_SURFACE_FREE(ctrl->pixel.surface);
    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
}

static int _subt_pixel_draw_argb8888_buffer(handle_t handle, unsigned int *buffer, int full_screen)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;
    PixelSubtCtrl *pixel = &ctrl->pixel;
    GXGDI_Rect dst_rect = {0};

    GxCore_MutexLock(ctrl->mutex);

    _subt_data_color_count();
    _surface_fill_backcolor();

    dst_rect.w = pixel->rect.w;
    dst_rect.h = pixel->rect.h;

    if(pixel->rect.w > APP_THEME_XRES)
    {
        dst_rect.w = APP_THEME_XRES;
        dst_rect.h = pixel->rect.h > APP_THEME_YRES? APP_THEME_YRES: (pixel->rect.h * APP_THEME_XRES / pixel->rect.w);
    }

    if(pixel->rect.h > APP_THEME_YRES)
    {
        dst_rect.w = pixel->rect.w > APP_THEME_XRES? APP_THEME_XRES: (pixel->rect.w * APP_THEME_YRES / pixel->rect.h);
        dst_rect.h = APP_THEME_YRES;
    }

    if(dst_rect.w < ctrl->rect.w)
        dst_rect.x = (ctrl->rect.w - pixel->rect.w) / 2;

    if(dst_rect.h < ctrl->rect.h)
        dst_rect.y = (ctrl->rect.h - pixel->rect.h) / 2;

    GXGDI_Lock();
    GXGDI_Begin();
    if(pixel->rect.w <= ctrl->rect.w && pixel->rect.h <= ctrl->rect.h)
        GXGDI_Blit(pixel->surface, &pixel->rect, ctrl->surface, &dst_rect, GDI_BLIT_COPY);
    else
        GXGDI_Blit(pixel->surface, &pixel->rect, ctrl->surface, &dst_rect, GDI_ZOOM);
    GXGDI_End();
    GxGDI_LayerUpdate(ctrl->layer_name);
    GXGDI_Unlock();
    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
}

static int _subt_pixel_display_pos_set(handle_t handle, GxSubtitleShowDisplay *pos)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    ctrl->pixel.pos = *pos;
    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
}

static int _subt_pixel_screen_color_set(handle_t handle, unsigned int color)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    ctrl->pixel.color = color;
    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
}

static int _subt_pixel_screen_set(GxSubtitleShowScreen *screen)
{
    return 0;
}

int32_t app_osd_subt_init(void)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    if(GXCORE_INVALID_POINTER == ctrl->mutex)
        GxCore_MutexCreate(&ctrl->mutex);

    GxCore_MutexLock(ctrl->mutex);

    if(true == ctrl->init)
    {
        app_log_error("please deinit first\n");
        goto err;
    }

    ctrl->color.forecolor = 0xf4f4f4;
    ctrl->color.backcolor = APP_UI_SUBT_BG;
    ctrl->color.defcolor = APP_UI_GUI_TRANS;
    ctrl->text.encoding = OSDSUBT_ENCODING_UTF8;
    ctrl->text.font = OSDSUBT_FONT_PROG;
    ctrl->type = UNKONWN_SUBT;

    TextOps.name  = "OSD Show Text";
    TextOps.open  = _subt_text_open;
    TextOps.close = _subt_text_close;
    TextOps.draw  = _subt_text_draw;
    TextOps.clean = _subt_text_clean;
    TextOps.show  = _subt_text_show;
    TextOps.hide  = _subt_text_hide;

    GxSubtitle_ModuleRegisterShowText(&TextOps);

    PixelOps.name  = "OSD Show Pixel";
    PixelOps.open  = _subt_pixel_open;
    PixelOps.close = _subt_pixel_close;
    PixelOps.clean = _subt_pixel_clean;
    PixelOps.show  = _subt_pixel_show;
    PixelOps.hide  = _subt_pixel_hide;
    PixelOps.setDisplayPos  = _subt_pixel_display_pos_set;
    PixelOps.setScreenColor = _subt_pixel_screen_color_set;
    PixelOps.setScreen      = _subt_pixel_screen_set;

    PixelOps.allocARGB8888Buffer = _subt_pixel_alloc_argb8888_buffer;
    PixelOps.freeARGB8888Buffer  = _subt_pixel_free_argb8888_buffer;
    PixelOps.drawARGB8888Buffer  = _subt_pixel_draw_argb8888_buffer;

    GxSubtitle_ModuleRegisterShowPixel(&PixelOps);
    ctrl->init = true;

    GxCore_MutexUnlock(ctrl->mutex);
    return 0;
err:
    GxCore_MutexUnlock(ctrl->mutex);
    return -1;
}

void app_osd_subt_deinit(void)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);

    if(false == ctrl->init)
    {
        app_log_debug("Have been deinited\n");
        goto end;
    }

    GxSubtitle_ModuleUnRegisterShowText();
    GxSubtitle_ModuleUnRegisterShowPixel();
    SUBT_LAYER_UNREGISTER(ctrl->layer_name);
    SUBT_SURFACE_FREE(ctrl->surface);
    SUBT_SURFACE_FREE(ctrl->pixel.surface);
    ctrl->x = 0;
    ctrl->y = 0;
    ctrl->type = UNKONWN_SUBT;
    memset(&ctrl->color, 0, sizeof(OsdSubtColor));
    memset(&ctrl->rect, 0, sizeof(GXGDI_Rect));
    memset(&ctrl->pixel, 0, sizeof(PixelSubtCtrl));
    memset(&ctrl->text, 0, sizeof(TextSubtCtrl));
    ctrl->init = false;
    ctrl->pause = false;

end:
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

int32_t app_osd_subt_pause(void)
{
    int ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
	GXGDI_Lock();
    if(NULL != ctrl->layer_name)
        ret = GxGDI_LayerEnable(ctrl->layer_name, false);
	GXGDI_Unlock();
    if(true == ctrl->init)
        ctrl->pause = true;
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

int32_t app_osd_subt_resume(void)
{
    int ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
	GXGDI_Lock();
    if(ctrl->layer_name)
        ret = GxGDI_LayerEnable(ctrl->layer_name, true);
	GXGDI_Unlock();
    ctrl->pause = false;
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

void app_osd_subt_color_set(OsdSubtColor color)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
    {
        ctrl->color.backcolor = color.backcolor;
        ctrl->color.forecolor = color.forecolor;
        ctrl->color.defcolor  = color.defcolor;
    }
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

void app_osd_subt_font_set(OsdSubtFont font)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ctrl->text.font = font;
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

int32_t app_osd_subt_font_get(void)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ret = (int32_t)ctrl->text.font;
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

void app_osd_subt_encoding_set(OsdSubtEncoding encoding)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;
    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ctrl->text.encoding = encoding;
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

int32_t app_osd_subt_encoding_get(void)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ret = (int32_t)ctrl->text.encoding;
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

void app_osd_subt_codepage_set(OsdSubtCodePage codepage)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ctrl->text.codepage = codepage;
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}

int32_t app_osd_subt_type_get(void)
{
    int32_t ret = 0;
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ret = (int32_t)ctrl->type;
    GxCore_MutexUnlock(ctrl->mutex);
    return ret;
}

void app_osd_subt_type_set(OsdSubtType type)
{
    OsdSubtCtrl *ctrl = &s_OsdSubtCtrl;

    GxCore_MutexLock(ctrl->mutex);
    if(true == ctrl->init)
        ctrl->type = type;
    GxCore_MutexUnlock(ctrl->mutex);
    return;
}
#endif
