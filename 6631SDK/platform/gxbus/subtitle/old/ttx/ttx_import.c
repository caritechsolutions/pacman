#include  "module/ttx/ttx_import.h"
#include "av/gxav_vpu_propertytypes.h"
#include "av/avapi.h"
#include "gxos/gxcore_os.h"
#include "gxavdev.h"
#include "av/gxav_demux_propertytypes.h"
#include "av/gxav_module_property.h"
#include "av/gxav_event_type.h"
#include "module/config/gxconfig.h"
#include "module/ttx/ttx_sub.h"
#include "module/subtitle/gxsubtitle.h"
#include "gdi_core.h"
#include "gx_mem.h"

/*****************************************************************************
 * Function	   : epg_api_get_section
 * Description :
 * Arguments   : a:……
 * Returns     :
 * Other       :
 ****************************************************************************/

static GxColor stb_palette_color[256] ;
static  TtxCCSpp*         spp=NULL;
uint32_t  TTX_CLUT[32] =
{
    0x0c0c0c,0x061ff9,0x00ff01,0x00ffff,0xed5112,0xff00fe,0xffff00,0xffffff,
    0x000000,0x000077,0x007700,0x007777,0x770000,0x770077,0x777700,0x777777,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};


uint32_t  CC_CLUT[32] =
{
    6691, 20863, 37063, 53347, 11227, 27447, 43651, 59935,
    4640, 11695, 18775, 26915, 6879, 15019, 22099, 30239,
    0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0
};


extern void *screen;
extern int g_device_handle;
extern int vpu_handle;
extern uint16_t osd_trans_index;
//void *spp_screen;
GxVpuProperty_ColorFormat ColorFormat;
static TtxCommitJob ttx_commit_job = {0};
static TtxCreatePalette ttx_create_pal = {0};
// new OSD surface for TTX. new chip, after GX1131 is needed.
GuiSurface *TtxSurface = NULL;
void *TtxOsdScreen = NULL;
void *TtxOsdScreenBuffer = NULL;
extern int hd_commit_blit(void);

typedef enum{
    TTX_OSD_8BPP = 0,
    TTX_OSD_32BPP
}Osd_color_mode;

#define OSD_COLOR_BIT TTX_OSD_32BPP

static Osd_color_mode s_color_mode = OSD_COLOR_BIT;

void ttx_api_thread_delay(uint32_t millisecond )
{
    GxCore_ThreadDelay(millisecond );
}


static  GxVpuProperty_LayerMainSurface MainSurfaceRecover;
static  GxVpuProperty_LayerViewport    LayerViewPortRecover;
void    *g_TtxRecordPalette = NULL;

int ttx_hd_init(void)
{
    GAL_Rect rect;
    GxVpuProperty_LayerMainSurface MainSurface;
    GxVpuProperty_Alpha Alpha;
    int ret = 0;
    GxVpuProperty_LayerViewport     LayerViewPort = {0};
    GxVpuProperty_VirtualResolution Resolution = {0};
    int32_t Pos = 0;
    GxVpuProperty_CreatePalette CreatePalette = {0};
    uint32_t* pGuiClut = NULL;
    GxColor entries[256];
    GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};
    GxVpuProperty_RWPalette RWPalette = {0};
    GxPalette  palette = {0};
    memset(&MainSurfaceRecover, 0, sizeof(GxVpuProperty_LayerMainSurface));

    MainSurfaceRecover.layer = GX_LAYER_OSD;
    ret |= GxAVGetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerMainSurface, &MainSurfaceRecover, sizeof(GxVpuProperty_LayerMainSurface));

    if(ret)
    {
        gxlogd("\n----[TTX][recoder main surface failed]---\n");
        return 1;
    }
    LayerViewPortRecover.layer = GX_LAYER_OSD;
    ret = GxAVGetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortRecover,
            sizeof(GxVpuProperty_LayerViewport));
    if(ret)
    {
        gxlogd("\n----[TTX][recoder OSD view port failed]---\n");
        return 1;
    }

    rect.x = rect.y = 0;
	rect.w = TTX_SURFACE_WIDTH;
	rect.h = TTX_SURFACE_HIGHT;

    if(s_color_mode == TTX_OSD_8BPP)
         TtxSurface = hd_surface_clone(GX_LAYER_OSD, &rect, 8);
    else if(s_color_mode == TTX_OSD_32BPP)
    {
        TtxSurface = hd_surface_clone(GX_LAYER_OSD, &rect, 32);
        GXGDI_FillRect((void*)TtxSurface,(GXGDI_Rect *)&rect,0x00000000);
    }
    TtxOsdScreen = TtxSurface->hw_surface;
    TtxOsdScreenBuffer = TtxSurface->data;

    MainSurface.layer = GX_LAYER_OSD;
    MainSurface.surface = TtxOsdScreen;
    ret |= GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerMainSurface, &MainSurface, sizeof(GxVpuProperty_LayerMainSurface));

    if(ret)
    {
        gxlogd("\n----[TTX][main surface failed]---\n");
        return 1;
    }
    ret = GxAVGetProperty(g_device_handle,vpu_handle,
            GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));
    if(ret)
    {
        gxlogd("\n----[TTX][Get virtual resolution failed]---\n");
        return 1;
    }

    LayerViewPort.layer = GX_LAYER_OSD;
    LayerViewPort.rect.x = 0;
    LayerViewPort.rect.y = 0;
    LayerViewPort.rect.width = Resolution.xres;
    LayerViewPort.rect.height = Resolution.yres;
    ret = GxAVSetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPort,
            sizeof(GxVpuProperty_LayerViewport));
    if(ret)
    {
        gxlogd("\n----[TTX][set view port failed]---\n");
        return 1;
    }

    Alpha.surface = TtxOsdScreen;
    Alpha.alpha.type = GX_ALPHA_PIXEL;
    Alpha.alpha.value = 255;

    ret |= GxAVSetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_Alpha,
            &Alpha,
            sizeof(GxVpuProperty_Alpha));

    if(0 != ret)
    {
        gxlogd("[TTX]Set OSD Alpha failed!\n");
        return (1);
    }

    if(s_color_mode == TTX_OSD_8BPP)
    {
        // create the palette
        CreatePalette.num_entries = 256;
        CreatePalette.palette_num = 1;
        ret = GxAVGetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_CreatePalette,
                &CreatePalette,
                sizeof(GxVpuProperty_CreatePalette));
        if(ret)
        {
            gxlogd("\n----[TTX][create palette failed]---\n");
            return 1;
        }
        g_TtxRecordPalette = CreatePalette.palette;

        for(Pos=0;Pos<256;Pos++)
        {
            pGuiClut = &TTX_CLUT[Pos%32];
            entries[Pos].r = *(pGuiClut) & 0xff;
            entries[Pos].g = (*(pGuiClut) & 0xff00) >> 8;
            entries[Pos].b = *(pGuiClut) >> 16;
            switch(Pos/32)
            {
                case 0:
                    entries[Pos].a = 0xff;
                    break;
                case 1:
                    entries[Pos].a = 0xe0;
                    break;
                case 2:
                    entries[Pos].a = 0xb0;
                    break;
                case 3:
                    entries[Pos].a = 0x90;
                    break;
                case 4:
                    entries[Pos].a = 0x70;
                    break;
                case 5:
                    entries[Pos].a = 0x50;
                    break;
                case 6:
                    entries[Pos].a = 0x30;
                    break;
                case 7:
                    entries[Pos].a = 0x10;
                    break;
                case 8:
                    entries[Pos].a = 0x0;
                    break;
            }
            if(*pGuiClut == 0)
            {
                entries[Pos].a = 0x0;
            }
        }
        if(CreatePalette.palette)
        {
            RWPalette.k_palette = CreatePalette.palette;
            RWPalette.palette_id = 0;
            palette.entries = entries;
            palette.num_entries = 256;
            RWPalette.u_palette = &palette;
        }

        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_RWPalette,
                &RWPalette,
                sizeof(GxVpuProperty_RWPalette));
        if(ret)
        {
            gxlogd("\n----[TTX][RW palette failed]---\n");
            return 1;
        }

        SurfaceBindPalette.surface = TtxOsdScreen;
        SurfaceBindPalette.palette = CreatePalette.palette;
        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_SurfaceBindPalette,
                &SurfaceBindPalette,
                sizeof(GxVpuProperty_SurfaceBindPalette));
        if(ret)
        {
            gxlogd("\n----[TTX][BIND palette failed]---\n");
            return 1;
        }
    }
    return (ret);
}

int ttx_hd_exit(void)
{
    int ret = 0;
    GxVpuProperty_DestroyPalette DestroyPalette = {0};

    ret |= GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerMainSurface, &MainSurfaceRecover, sizeof(GxVpuProperty_LayerMainSurface));

    if(ret)
    {
        gxlogd("\n----[TTX][recover main surface failed]---\n");
        return 1;
    }

    ret = GxAVSetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortRecover,
            sizeof(GxVpuProperty_LayerViewport));

    if(ret)
    {
        gxlogd("\n----[TTX][recover view port failed]---\n");
        return 1;
    }

    if(g_TtxRecordPalette && s_color_mode == TTX_OSD_8BPP)
    {
        DestroyPalette.palette = g_TtxRecordPalette;
        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_DestroyPalette,
                &DestroyPalette,
                sizeof(GxVpuProperty_DestroyPalette));

        if(ret)
        {
            gxlogd("\n----[TTX][release palette failed]---\n");
            return 1;
        }
        g_TtxRecordPalette = NULL;
    }
    if(TtxOsdScreen != screen)
    {
        hd_free_surface(TtxSurface);
        TtxOsdScreen = NULL;
        TtxOsdScreenBuffer = NULL;
    }
    hd_clear(GX_LAYER_OSD);
    hd_commit_blit();

    return 0;
}


int ttx_hd_destroy_palette(void)
{
    if(s_color_mode == TTX_OSD_32BPP)
        return 0;
    GxVpuProperty_CreatePalette *palette = NULL;
    GxVpuProperty_DestroyPalette des_pal = {0};
    TtxPalette *pal_node = NULL, *pal_next = NULL;
    int ret = 0;

    if(ttx_create_pal.num_palette)
    {
        pal_node = ttx_create_pal.head;

        while(pal_node)
        {
            pal_next = pal_node->next;

            palette = pal_node->palette;
            des_pal.palette = palette->palette;
            ret = GxAVSetProperty(g_device_handle,
                    vpu_handle,
                    GxVpuPropertyID_DestroyPalette,
                    &des_pal,
                    sizeof(GxVpuProperty_DestroyPalette));

            if(0 != ret)
            {
                gxlogd("Destroy surface failed!\n");
                return (1);
            }

            TTX_FREE(pal_node->palette);
            TTX_FREE(pal_node);

            pal_node = pal_next;
        }

        ttx_create_pal.num_palette = 0;
        ttx_create_pal.head = ttx_create_pal.tail = NULL;
    }

    return (0);
}


GxVpuProperty_CreatePalette *ttx_hd_create_palette(void)
{
    GxVpuProperty_CreatePalette *new_pal = NULL;
    TtxPalette *pal_node = NULL;

    if(s_color_mode == TTX_OSD_32BPP)
        return 0;

    new_pal = (GxVpuProperty_CreatePalette *)av_malloc(sizeof(GxVpuProperty_CreatePalette));
    if(NULL == new_pal)
    {
        return (NULL);
    }
    memset(new_pal, 0, sizeof(GxVpuProperty_CreatePalette));

    pal_node = (TtxPalette *)av_malloc(sizeof(TtxPalette));
    if(NULL == pal_node)
    {
        av_free(new_pal);
        return (NULL);
    }
    memset(pal_node, 0, sizeof(TtxPalette));

    pal_node->palette = new_pal;

    ttx_create_pal.num_palette++;

    if((NULL == ttx_create_pal.head) && (NULL == ttx_create_pal.tail))
    {
        ttx_create_pal.head = ttx_create_pal.tail = pal_node;
    }
    else
    {
        ttx_create_pal.tail->next = pal_node;
        ttx_create_pal.tail = pal_node;
    }

    pal_node->next = NULL;

    return (new_pal);
}

int ttx_hd_new_blit_list(void)
{
    GxVpuProperty_BeginUpdate begin = {0};
    int ret = 0;

    begin.max_job_num = TTX_MAX_BLIT_COUNT;

    ret = GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_BeginUpdate,
            &begin,
            sizeof(GxVpuProperty_BeginUpdate));
    return (ret);
}


int ttx_hd_end_blit_list(void)
{
    GxVpuProperty_EndUpdate end = {0};
    int ret = 0;


    ret = GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_EndUpdate,
            &end,
            sizeof(GxVpuProperty_EndUpdate));
    return (ret);
}


int ttx_hd_commit_blit(void)
{
    GxVpuProperty_EndUpdate end = {0};
    int ret = 0, i = 0;
    TtxDrawNode *p_node = NULL, *p_next = NULL;
    GxVpuProperty_DestroySurface DesSurface = { 0 };

    if(ttx_commit_job.total_num)
    {
        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_EndUpdate,
                &end,
                sizeof(GxVpuProperty_EndUpdate));

        if(0 != ret)
        {
            gxlogd("Set OSD BLIT end failed!\n");
        }
    }

    if((0 == ttx_commit_job.total_num) || (TTX_MAX_BLIT_COUNT < ttx_commit_job.total_num))
    {
        return (0);
    }

    p_node = ttx_commit_job.list_head;

    while(p_node)
    {
        p_next = p_node->next;

        if(p_node->surface)
        {
            DesSurface.surface = p_node->surface;
            ret = GxAVSetProperty(g_device_handle,
                    vpu_handle,
                    GxVpuPropertyID_DestroySurface,
                    &DesSurface,
                    sizeof(GxVpuProperty_DestroySurface));

            if(0 != ret)
            {
                gxlogd("Set OSD Destroy surface failed!\n");
                return (1);
            }
        }
        TTX_FREE(p_node);

        i++;
        p_node = p_next;
        ttx_commit_job.list_head = p_node;
    }

    ttx_commit_job.total_num = 0;
    ttx_commit_job.list_head = ttx_commit_job.list_tail = NULL;

    ttx_hd_destroy_palette();
    ttx_hd_new_blit_list();

    return (ret);
}


int ttx_hd_add_blit_element(void *surface)
{
    TtxDrawNode *draw_node = NULL;

    ttx_commit_job.total_num++;
    draw_node = (TtxDrawNode *)av_malloc(sizeof(TtxDrawNode));
    if(NULL == draw_node)
    {
        return (1);
    }
    memset(draw_node, 0, sizeof(TtxDrawNode));

    draw_node->surface = surface;
    draw_node->next = NULL;

    if((NULL == ttx_commit_job.list_head) && (NULL == ttx_commit_job.list_tail))
    {
        ttx_commit_job.list_head = ttx_commit_job.list_tail = draw_node;
    }
    else
    {
        ttx_commit_job.list_tail->next = draw_node;
        ttx_commit_job.list_tail = ttx_commit_job.list_tail->next;
    }

    if(TTX_MAX_BLIT_COUNT <= ttx_commit_job.total_num)
    {
        ttx_hd_commit_blit();
    }

    return (0);
}


int ttx_hd_char_blit( uint32_t x, uint32_t y, uint32_t xsize, uint32_t ysize, const uint8_t *pData, uint32_t nFrontColor,uint32_t nBackColor)
{
    GxVpuProperty_Blit Blit;
    GxVpuProperty_CreateSurface BlitSurface = { 0 };
    GxVpuProperty_DestroySurface DesSurface = { 0 };
    GxVpuProperty_ColorKey ColorKey = { 0 };
    GxVpuProperty_ConvertColor ConvertColor;

    GxVpuProperty_CreatePalette *CreatePalette = NULL;
    GxPalette palette = {0};
    GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};
    GxVpuProperty_GetEntries GetEntries = {0};
    GxVpuProperty_RWPalette RWPalette = {0};
    GxColor entries[2];
    int ret = 0;
    char *MemBuf = NULL;
    int byteperline = 2;
    void *surface;

    memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

    surface = TtxOsdScreen;

    if(NULL == pData )
    {
        return (1);
    }

    BlitSurface.format = GX_COLOR_FMT_CLUT1;

    BlitSurface.width = byteperline * 8;
    BlitSurface.height = ysize;

    BlitSurface.mode = GX_SURFACE_MODE_IMAGE;
    BlitSurface.buffer = NULL;

    ret = GxAVGetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_CreateSurface,
            &BlitSurface, sizeof(GxVpuProperty_CreateSurface));
    if(0 != ret)
    {
        gxlogd("Create char surface failed!\n");
        return (1);
    }

    MemBuf = BlitSurface.buffer;

    memcpy(MemBuf, pData, ysize * byteperline);

    ColorKey.enable = 1;
    ColorKey.surface = BlitSurface.surface;
    ret = GxAVSetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_ColorKey, &ColorKey, sizeof(GxVpuProperty_ColorKey));
    if(0 != ret)
    {
        gxlogd("Set char color key failed!\n");
        return (1);
    }

    if(s_color_mode == TTX_OSD_8BPP)
    {
        /*Create Palette*/
        CreatePalette = ttx_hd_create_palette();

        CreatePalette->num_entries = 2;
        CreatePalette->palette_num = 1;
        ret = GxAVGetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_CreatePalette,
                CreatePalette,
                sizeof(GxVpuProperty_CreatePalette));
        if(0 != ret)
        {
            gxlogd("Create char palette failed!\n");
            return (1);
        }
        /*Bind Palette and surface*/
        SurfaceBindPalette.surface = BlitSurface.surface;
        SurfaceBindPalette.palette = CreatePalette->palette;
        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_SurfaceBindPalette,
                &SurfaceBindPalette,
                sizeof(GxVpuProperty_SurfaceBindPalette));
        if(0 != ret)
        {
            gxlogd("Bind char surface with palette failed!\n");
            return (1);
        }
        ret =0;
        entries[0].entry = ret;
        entries[0].a = 0;//(gui.config.gui_trans & 0x000F) << 4;
        entries[1].a = 0;// (color & 0x000F) << 4;
        entries[1].entry = nFrontColor;
        if(CreatePalette->palette)
        {
            RWPalette.k_palette = CreatePalette->palette;
            RWPalette.palette_id = 0;
            palette.entries = entries;
            palette.num_entries = 2;
            RWPalette.u_palette = &palette;
        }

        ret = GxAVSetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_RWPalette,
                &RWPalette,
                sizeof(GxVpuProperty_RWPalette));

        if(0 != ret)
        {
            gxlogd("Set char palette failed!\n");
            return (1);
        }
        /*Convert Color*/
        ret = 0;
        ConvertColor.surface = BlitSurface.surface;
        ConvertColor.src_palette.num_entries = 2;


        GetEntries.k_palette = CreatePalette->palette;
        ret = GxAVGetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_GetEntries,
                &GetEntries,
                sizeof(GxVpuProperty_GetEntries));

        if(0 != ret)
        {
            gxlogd("Get entries failed!\n");
        }

        ConvertColor.src_palette.entries = GetEntries.entries;
        ConvertColor.dst_format = GX_COLOR_FMT_CLUT8;
        ret = GxAVSetProperty(g_device_handle, vpu_handle,
                GxVpuPropertyID_ConvertColor,
                &ConvertColor, sizeof(GxVpuProperty_ConvertColor));

        if(0 != ret)
        {
            gxlogd("Set char Convert color failed!\n");
            return (1);
        }
    }
    Blit.srca.is_kcct = 1;
    Blit.srca.is_ksurface = 1;
    Blit.srcb.is_kcct = 1;
    Blit.srcb.is_ksurface = 1;

    Blit.mode = GX_ALU_ROP_COPY;//GX_ALU_MIX_B_TOP_A;
    Blit.srca.surface = BlitSurface.surface;
    Blit.srca.rect.x = 0;
    Blit.srca.rect.y = 0;
    Blit.srca.rect.width = byteperline * 8;
    Blit.srca.rect.height = ysize;
    Blit.srcb.surface = surface;
    Blit.srcb.rect = Blit.srca.rect;
    Blit.srcb.rect.x = x;
    Blit.srcb.rect.y = y;
    Blit.dst = Blit.srcb;
    Blit.srca.alpha_en = 1;
    Blit.srca.alpha = 0;
    Blit.srcb.alpha_en = 0;
    //针对3113 混合操作alpha丢失问题
    Blit.dst.alpha_en = 0;
    Blit.dst.alpha = 250;//Patch//0XF0;

    ret = GxAVSetProperty(g_device_handle, vpu_handle,
            GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));

    if(0 != ret)
    {
        gxlogd("Char Blit failed!\n");
        return (1);
    }

    DesSurface.surface = BlitSurface.surface;
    ttx_hd_add_blit_element(DesSurface.surface);



    return (ret);
}


static unsigned int _color_convert(unsigned int in_color)
{
    unsigned char trans[9] = {0xff, 0xe0, 0xb0, 0x90, 0x70, 0x50, 0x30, 0x10, 0x00};
    int trans_index = in_color/32;
    int color_inidex = in_color%32;
    unsigned int new_color = 0; // ARGB

    if(TTX_CLUT[color_inidex] != 0)
        new_color |= (trans[trans_index] << 24);// A

    new_color |= ((TTX_CLUT[color_inidex]&0xff0000));// R
    new_color |= (TTX_CLUT[color_inidex]&0xff00);// G
    new_color |= ((TTX_CLUT[color_inidex]&0xff));// B
    return new_color;
}

static int sw_char( uint32_t x, uint32_t y, uint32_t xsize, uint32_t ysize, const uint8_t *pData, uint32_t nFrontColor,uint32_t nBackColor)
{
    int i,j;
    char* buf ;
    char* pos;
    unsigned char* data = NULL;
    //xsize = s_AsciiFontWidth;//s_AsciiFontWidth;
    if((TtxOsdScreenBuffer == NULL)
		|| (pData == NULL)
		|| (x >= TTX_SURFACE_WIDTH)
		|| (y >= TTX_SURFACE_HIGHT))
   	{
		gxlogd("\nTTX, error, %s, %d\n", __FUNCTION__, __LINE__);
		return -1;
	}
    buf = (char*)TtxOsdScreenBuffer;
    data = (unsigned char*)pData;

    if(s_color_mode == TTX_OSD_8BPP)
    {
        for(i=0;i<ysize;i++)
        {
            //pos = buf + 720*(i+y) + x;
            pos = buf + TTX_SURFACE_WIDTH*(i+y) + x;
            for(j=0;j<xsize;j++)
            {
                *pos = (((data[j>>3])>>(7-(j%8)))&0x1)?nFrontColor:nBackColor;
                pos++;
            }
            data = data+(((xsize%8) > 0)?(xsize/8+1):(xsize/8));
        }
    }
    else if(s_color_mode == TTX_OSD_32BPP)
    {
        unsigned int color = 0;
        unsigned int f_color = _color_convert(nFrontColor);
        unsigned int b_color = _color_convert(nBackColor);
        for(i=0;i<ysize;i++)
        {
            pos = buf + TTX_SURFACE_WIDTH*(i+y)*4 + x*4;
            for(j=0;j<xsize;j++)
            {
                color = (((data[j>>3])>>(7-(j%8)))&0x1)?f_color:b_color;

                *pos++ = ((color >> 16)&0xff);// R
                *pos++ = ((color >> 8)&0xff);// G
                *pos++ = (color&0xff);// B
                *pos++ = ((color >> 24)&0xff);// A
            }
            data = data+(((xsize%8) > 0)?(xsize/8+1):(xsize/8));
        }
    }
    return 0;
}

int ttx_hd_char( uint32_t x, uint32_t y, uint32_t xsize, uint32_t ysize, const uint8_t *pData, uint32_t nFrontColor,uint32_t nBackColor)
{
    GxVpuProperty_Blit Blit;
    GxVpuProperty_CreateSurface BlitSurface = { 0 };
    GxVpuProperty_DestroySurface DesSurface = { 0 };
    //int byteperline = 2;
    int ret = 0;
    char *MemBuf = NULL;
    void *surface;
    int i = 0;
    int j = 0;

    int32_t showmean = 0;
    GxBus_ConfigGetInt(GXBUS_TTX_SHOWMEAN,&showmean,GXBUS_TTX_SHOWMEAN_WRITE);
    if(showmean == GXBUS_TTX_SHOWMEAN_WRITE)
    {
        sw_char(x,y,xsize,ysize,pData,nFrontColor,nBackColor);
        return 0;
    }
    else
    {
        memset(&Blit, 0, sizeof(GxVpuProperty_Blit));
        surface = TtxOsdScreen;
        if(NULL == pData )
        {
            return (1);
        }

        BlitSurface.format = GX_COLOR_FMT_CLUT8;
        BlitSurface.width = xsize;
        BlitSurface.height = ysize;
        BlitSurface.mode = GX_SURFACE_MODE_IMAGE;
        BlitSurface.buffer = NULL;

        ret = GxAVGetProperty(g_device_handle,
                vpu_handle,
                GxVpuPropertyID_CreateSurface,
                &BlitSurface, sizeof(GxVpuProperty_CreateSurface));

        if(0 != ret)
        {
            gxlogd("[HD CHAR] GET GxVpuPropertyID_CreateSurface\n");
        }
        MemBuf = GxCore_Map(g_device_handle, (uint32_t)BlitSurface.buffer, xsize*ysize);

        for(i=0;i<ysize;i++)
        {
            for(j=0;j<xsize;j++)
            {
                MemBuf[i*xsize+j] = (((pData[j>>3])>>(7-(j%8)))&0x1)?nFrontColor:nBackColor;
            }
            pData = pData+(((xsize%8) > 0)?(xsize/8+1):(xsize/8));
        }

        Blit.srca.is_kcct = 1;
        Blit.srca.is_ksurface = 1;
        Blit.srcb.is_kcct = 1;
        Blit.srcb.is_ksurface = 1;
        Blit.mode = GX_ALU_ROP_COPY;//GX_ALU_MIX_B_TOP_A;
        Blit.srca.surface = BlitSurface.surface;
        Blit.srca.rect.x = 0;
        Blit.srca.rect.y = 0;
        Blit.srca.rect.width = xsize;//s_AsciiFontWidth;
        Blit.srca.rect.height = ysize;
        Blit.srcb.dst_format = GX_COLOR_FMT_CLUT8;
        Blit.srcb.surface = surface;
        Blit.srcb.rect = Blit.srca.rect;
        Blit.srcb.rect.x = x;
        Blit.srcb.rect.y = y;
        Blit.dst = Blit.srcb;

        ret = GxAVSetProperty(g_device_handle, vpu_handle,
                GxVpuPropertyID_Blit, &Blit, sizeof(GxVpuProperty_Blit));
        if(0 != ret)
        {
            gxlogd("[HD CHAR]SET GxVpuPropertyID_Blit\n");
        }
        DesSurface.surface = BlitSurface.surface;
        GxCore_UnMap(g_device_handle, MemBuf, xsize*ysize);
        ret = GxAVSetProperty(g_device_handle, vpu_handle,
                GxVpuPropertyID_DestroySurface,
                &DesSurface, sizeof(GxVpuProperty_DestroySurface));
        if(0 != ret)
        {
            gxlogd("[HD CHAR]SET GxVpuPropertyID_DestroySurface\n");
        }
        return (ret);
    }
}

void ttx_sib_osd_draw_text(
        uint32_t nDstPositionX,
        uint32_t nDstPositionY,
        uint32_t nWidth,
        uint32_t nHeight,
        const uint8_t* pData,
        uint32_t nFrontColor,
        uint32_t nBackColor)
{
    ttx_hd_char(
            nDstPositionX,
            nDstPositionY,
            nWidth,
            nHeight,
            pData,
            nFrontColor,
            nBackColor);
}

void ttx_api_osd_draw_text(
        uint32_t nDstPositionX,
        uint32_t nDstPositionY,
        uint32_t nWidth,
        uint32_t nHeight,
        const uint8_t* pData,
        uint32_t nFrontColor,
        uint32_t nBackColor)
{
    ttx_hd_char(
            nDstPositionX,
            nDstPositionY,
            nWidth,
            nHeight,
            pData,
            nFrontColor,
            nBackColor);
}
/*void ttx_sib_spp_draw_text(
  GXSPP_Handle_t    DeviceHandle,
  uint32_t                         DstPositionX,
  uint32_t                         DstPositionY,
  uint32_t                         Width,
  uint32_t                         Height,
  const uint8_t                    *pData,
  uint32_t                         FrontColor,
  uint32_t                         BackColor,
  GXSPP_DrawMode_t            DrawMode)
  {
  extern Handle_t g_GpuHandle;

  com_os_scheduler_lock();
  GXSPP_DrawText(
  DeviceHandle,
  DstPositionX,
  DstPositionY,
  Width,
  Height,
  pData,
  FrontColor,
  BackColor,
  DrawMode);
  GXGPU_SubmitJob(g_GpuHandle);
  com_os_scheduler_unlock();

  }*/

//extern uint8_t  g_SppBuffer[];
/*void ttx_sib_spp_expang_draw_text(
  GXSPP_Handle_t    DeviceHandle,
  uint32_t                         DstPositionX,
  uint32_t                         DstPositionY,
  uint32_t                         Width,
  uint32_t                         Height,
  const uint8_t                    *pData,
  uint32_t                         FrontColor,
  uint32_t                         BackColor,
  GXSPP_DrawMode_t            DrawMode)
  {
  GXOSD_Expand_DrawText(DeviceHandle,
  DstPositionX,
  DstPositionY,
  Width,
  Height,
  pData,
  FrontColor,
  BackColor,
  DrawMode,
  g_SppBuffer);
  }*/

#define _REG_GET_BYTE0(reg)  ((reg)  &  0xFF)
#define _REG_GET_BYTE1(reg)  (((reg) >> 8) & 0xFF)

#define _GET_ENDIAN_16(reg)                     \
    (_REG_GET_BYTE0(reg) << 8 ) |              \
(_REG_GET_BYTE1(reg))


void ttx_spp_set_point(GxVpuProperty_Point * point)
{
    unsigned int addr;
    unsigned int valColor = 0;

    addr = (unsigned int)spp->buf +
        ((16 * (point->point.y * spp->rect.width + point->point.x)) >> 3);

    if(point->point.x>spp->rect.width || point->point.y >spp->rect.height)
        return;

    valColor = (((point->color.y) & 0xFC) << 8)
        | (((point->color.cb) & 0xF0) << 2)
        | (((point->color.cr) & 0xF0) >> 2)
        | (((point->color.a) & 0xC0) >> 6);

    *(unsigned short *)addr = _GET_ENDIAN_16(valColor);

}

extern GuiSurface *spp_screen;
static GuiSurface *spp_screen_bak = NULL;
void ttx_spp_open(TtxCCRect* rect)
{
    //int32_t                         ret;
    GxVpuProperty_ColorFormat       format;
    GxVpuProperty_CreateSurface     p;
    GxVpuProperty_LayerMainSurface  layer;
    GxVpuProperty_LayerEnable       enable;
    GxVpuProperty_LayerOnTop        top;
    GAL_Rect			    surface_rect;

    spp = (TtxCCSpp*)av_malloc(sizeof(TtxCCSpp));
    if (spp == NULL) {
        return ;
    }

    GxAvdev_SppLock();

    spp->dev    = GxAvdev_CreateDevice(0);
    spp->spp    = GxAvdev_OpenModule(spp->dev, GXAV_MOD_VPU, 0);

    surface_rect.x = rect->x;
    surface_rect.y = rect->y;
    surface_rect.w = rect->width;
    surface_rect.h = rect->height;

    spp->rect.x      = rect->x;
    spp->rect.y      = rect->y;
    spp->rect.width  = rect->width;
    spp->rect.height = rect->height;

    layer.layer = GX_LAYER_SPP;
    GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerMainSurface,
            &layer,
            sizeof(GxVpuProperty_LayerMainSurface));
    //gxlogd("\n********GxVpuPropertyID_LayerMainSurface ret:%d*********\n",ret);

    spp->old_surface = layer.surface;

    // for recover
    GxVpuProperty_LayerViewport     LayerViewPortForRecover = {0};
    LayerViewPortForRecover.layer = GX_LAYER_SPP;
    GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortForRecover,
            sizeof(GxVpuProperty_LayerViewport));
    spp->RecoverRect.x = LayerViewPortForRecover.rect.x;
    spp->RecoverRect.y = LayerViewPortForRecover.rect.y;
    spp->RecoverRect.width = LayerViewPortForRecover.rect.width;
    spp->RecoverRect.height = LayerViewPortForRecover.rect.height;

    top.layer  = GX_LAYER_SPP;
    GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop,
            &top,
            sizeof(GxVpuProperty_LayerOnTop));
    //gxlogd("\n********GxVpuPropertyID_LayerOnTop ret:%d*********\n",ret);

    spp->old_top = top.enable;

    enable.layer  = GX_LAYER_SPP;
    GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &enable,
            sizeof(GxVpuProperty_LayerEnable));
    //gxlogd("\n********GxVpuPropertyID_LayerEnable ret:%d*********\n",ret);
    spp->old_enable = enable.enable;
    spp_screen_bak = spp_screen;
    spp->gui_surface = hd_surface_clone(GX_LAYER_SPP, &surface_rect, 2);
    //gxlogd("\n********GxVpuPropertyID_CreateSurface ret:%d*********\n",ret);
    if (spp->gui_surface->hw_surface == NULL) {
        GxAvdev_SppUnlock();
        GxAvdev_CloseModule(spp->dev, spp->spp);
        GxAvdev_DestroyDevice(spp->dev);
        av_free(spp);
	return;
    }

    spp->buf = (void *)spp->gui_surface->data;
    spp->surface = spp->gui_surface->hw_surface;
    p.surface = spp->surface;

    format.surface  = p.surface;
    format.format   = GX_COLOR_FMT_YCBCRA6442;
    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_ColorFormat,
            &format,
            sizeof(GxVpuProperty_ColorFormat));

    // gxlogd("\n********GxVpuPropertyID_ColorFormat ret:%d*********\n",ret);
    return ;
}

extern int hd_commit_blit(void);
void ttx_spp_close(void)
{
    int32_t                         ret;
    GxVpuProperty_LayerEnable       enable;
    GxVpuProperty_LayerOnTop        top;

    ttx_hd_end_blit_list();
    hd_free_surface(spp->gui_surface);
    if (!spp_screen_bak){
        if(spp_screen){
            hd_free_surface(spp_screen);
            spp_screen = spp_screen_bak;
        }
    }
    else{
        spp_screen = spp_screen_bak;
        hd_set_layer_surface(0);//0, LAYER_MAIN_SPP_SURFACE
    }
    //gxlogd("\n********GxVpuPropertyID_DestroySurface ret:%d*********\n",ret);
#if 0
	if(spp_screen)
	{
		if(spp_screen->hw_surface == spp->old_surface)
		{
	    	hd_clear(GX_LAYER_SPP);
			hd_commit_blit();// import for new driver, UI and AV share the same memory hole.
		}

		//gxlogd("\ncc, close2, old surface = 0x%x, gui surface = 0x%x\n", spp->old_surface,spp_screen->hw_surface);
	    if(spp->old_surface)
	    {
	        layer.layer      = GX_LAYER_SPP;
	        layer.surface    = spp_screen->hw_surface;//spp->old_surface;
	        ret = GxAVSetProperty(spp->dev, spp->spp,
	                GxVpuPropertyID_LayerMainSurface,
	                &layer,
	                sizeof(GxVpuProperty_LayerMainSurface));
	    }
	}
#endif
    // for recover
    GxVpuProperty_LayerViewport     LayerViewPortForRecover = {0};
    LayerViewPortForRecover.layer = GX_LAYER_SPP;
    LayerViewPortForRecover.rect.x = spp->RecoverRect.x;
    LayerViewPortForRecover.rect.y = spp->RecoverRect.y;
    LayerViewPortForRecover.rect.width = spp->RecoverRect.width;
    LayerViewPortForRecover.rect.height = spp->RecoverRect.height;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPortForRecover,
            sizeof(GxVpuProperty_LayerViewport));

    //gxlogd("\n********GxVpuPropertyID_LayerMainSurface ret:%d*********\n",ret);
    top.layer  = GX_LAYER_SPP;
    top.enable = spp->old_top;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop, (void*)&top,
            sizeof(GxVpuProperty_LayerOnTop));
    //gxlogd("\n********GxVpuPropertyID_LayerOnTop ret:%d*********\n",ret);

    enable.enable        = spp->old_enable;
    enable.layer         = GX_LAYER_SPP;
    ret = GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &enable,
            sizeof(GxVpuProperty_LayerEnable));
    //gxlogd("\n********GxVpuPropertyID_LayerEnable ret:%d*********\n",ret);

    GxAvdev_SppUnlock();
    GxAvdev_CloseModule(spp->dev, spp->spp);
    GxAvdev_DestroyDevice(spp->dev);
    av_free(spp);

    return ;
}

void ttx_spp_draw_text( uint32_t DstPositionX,
        uint32_t DstPositionY,
        uint32_t Width,
        uint32_t Height,
        const uint8_t  *pData,
        uint32_t FrontColor,
        uint32_t BackColor)

{

    uint8_t i=0;
    uint8_t j=0;
    uint8_t u=0;
    uint8_t *p = NULL;
    uint32_t x=0;
    uint32_t y=0;
    GxVpuProperty_Point  point;
    GxColor front_color;
    GxColor back_color;
    uint16_t gxcolorlen ;
    //uint16_t pointlen ;
    p = (uint8_t*)pData;

    gxcolorlen = sizeof(GxColor);
    //pointlen = sizeof(GxVpuProperty_Point);

    memset((void*)&front_color,0,gxcolorlen);
    memset((void*)&back_color,0,gxcolorlen);

    front_color.y = (FrontColor & 0xfc00)>>8;
    front_color.cb = (FrontColor & 0x03c0)>>2;
    front_color.cr = (FrontColor & 0x3c)<<2;
    if(FrontColor & 0x3){
        front_color.alpha= 0xff;
    }

    back_color.y = (BackColor & 0xfc00)>>8;
    back_color.cb = (BackColor & 0x03c0)>>2;
    back_color.cr = (BackColor & 0x3c)<<2;
    if(BackColor & 0x3){
        back_color.alpha= 0xff;
    }


    for(i=0;i<Height;i++)
    {

        x = DstPositionX;
        if(i==0){
            y = DstPositionY;
        }else{
            p++;
            y++;
        }
        u = 0;
        for(j=0;j<Width;j++)
        {
            point.surface = spp->surface;
            point.point.x = x++;
            point.point.y = y;

            if ((j != 0) && ( (j&0x7) == 0))
            {
                p++;
                u = 0;
            }
            if((*p)&(0x80>>u++)){
                memcpy((void*)&(point.color),(void*)&front_color,gxcolorlen);
            }else{
                memcpy((void*)&(point.color),(void*)&back_color,gxcolorlen);
            }
            ttx_spp_set_point(&point);

        }
    }

}


void ttx_spp_fill_rect(uint32_t y,uint32_t color)
{
#if (1)
	int ret;
	GxVpuProperty_FillRect FillRect;

	memset(&FillRect,0,sizeof(GxVpuProperty_FillRect));
	FillRect.surface = spp->surface;
	FillRect.color.y = (color & 0xfc00)>>8;
	FillRect.color.cb = (color & 0x03c0)>>2;
	FillRect.color.cr = (color & 0x3c)<<2;
	if(color & 0x3){
		FillRect.color.alpha= 0xff;
	}
	FillRect.color.y        = 16;
	FillRect.color.cb       = 128;
	FillRect.color.cr       = 128;
	FillRect.color.alpha    = 0;
	FillRect.rect.x = 0;
	FillRect.rect.y = y;
	FillRect.rect.width = TTX_SURFACE_WIDTH;
	FillRect.rect.height = 16;
	FillRect.is_ksurface = 1;
	ttx_hd_end_blit_list();
	ttx_hd_new_blit_list();
	ret = GxAVSetProperty(spp->dev,
						spp->spp,
						GxVpuPropertyID_FillRect,
						&FillRect,
						sizeof(GxVpuProperty_FillRect));
	if(ret < 0)
	{
		gxlogd("\nCC, fill rect failed, %s\n",__FUNCTION__);
	}
	ttx_hd_end_blit_list();
	ttx_hd_new_blit_list();

#else
	{
		int i = 0;
		int max = ((spp->rect.width)) / 2;
		uint32_t *buf = (uint32_t*)spp->buf + 2*(spp->rect.width)*y;
		for (i = 0; i < max; i++) {
			buf[i] = 0x20022002;
		}
	}
#endif
    return;
}
void ttx_spp_clear(uint32_t color)
{

#if (0)
    {
        //int32_t                         ret;
        GxVpuProperty_FillRect          fill;
        memset(&fill,0,sizeof(GxVpuProperty_FillRect));
        fill.is_ksurface    = 1;
        fill.surface        = spp->surface;
        fill.rect.x         = spp->rect.x;
        fill.rect.y         = spp->rect.y;
        fill.rect.width     = spp->rect.width;
        fill.rect.height    = spp->rect.height;
        fill.color.y = (color & 0xfc00)>>8;
        fill.color.cb = (color & 0x03c0)>>2;
        fill.color.cr = (color & 0x3c)<<2;
        if(color & 0x3){
            fill.color.alpha= 0xff;
        }
        fill.color.y        = 16;
        fill.color.cb       = 128;
        fill.color.cr       = 128;
        fill.color.alpha    = 0;
        ttx_hd_end_blit_list();
        ttx_hd_new_blit_list();
        GxAVSetProperty(spp->dev,
                spp->spp,
                GxVpuPropertyID_FillRect,
                &fill,
                sizeof(GxVpuProperty_FillRect));
        ttx_hd_end_blit_list();
        ttx_hd_new_blit_list();
        // gxlogd("\n********ttx_spp_clear ret:%d*********\n",ret);
    }
#else
    {
        int i = 0;
        int max = ((spp->rect.width) * (spp->rect.height)) / 2;
        uint32_t *buf = (uint32_t*)spp->buf;
        for (i = 0; i < max; i++) {
            buf[i] = 0x20022002;
        }
    }
#endif
    return;
}



void ttx_spp_enable(void)
{
    //int32_t                         ret;
    GxVpuProperty_LayerMainSurface  layer;
    GxVpuProperty_LayerEnable       p;
    GxVpuProperty_LayerOnTop        top;

    layer.layer      = GX_LAYER_SPP;
    layer.surface    = spp->surface;
    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerMainSurface,
            &layer,
            sizeof(GxVpuProperty_LayerMainSurface));
    //gxlogd("\n********GxVpuPropertyID_LayerMainSurface ret:%d*********\n",ret);

    // for GA ZOOM
    GxVpuProperty_LayerViewport     LayerViewPort = {0};
    GxVpuProperty_VirtualResolution Resolution = {0};
    GxAVGetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_VirtualResolution, (void*)&Resolution, sizeof(GxVpuProperty_VirtualResolution));

    LayerViewPort.layer = GX_LAYER_SPP;
    LayerViewPort.rect.x = 0;
    LayerViewPort.rect.y = 0;
    LayerViewPort.rect.width = Resolution.xres;
    LayerViewPort.rect.height = Resolution.yres;

    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerViewport,
            &LayerViewPort,
            sizeof(GxVpuProperty_LayerViewport));

    top.layer  = GX_LAYER_SPP;
    top.enable = 1;
    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerOnTop, (void*)&top,
            sizeof(GxVpuProperty_LayerOnTop));

    //gxlogd("\n********GxVpuPropertyID_LayerOnTop ret:%d*********\n",ret);
    p.enable        = 1;
    p.layer         = GX_LAYER_SPP;
    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &p,
            sizeof(GxVpuProperty_LayerEnable));


    //gxlogd("\n********GxVpuPropertyID_LayerEnable ret:%d*********\n",ret);
    return ;
}

void  ttx_spp_disable(void)
{
    //int32_t                         ret;
    GxVpuProperty_LayerEnable       p;

    p.enable        = 0;
    p.layer         = GX_LAYER_SPP;
    GxAVSetProperty(spp->dev, spp->spp,
            GxVpuPropertyID_LayerEnable,
            &p,
            sizeof(GxVpuProperty_LayerEnable));
    //gxlogd("\n********GxVpuPropertyID_LayerEnable ret:%d*********\n",ret);
    return ;
}


void ttx_osd_enable(void)
{

    GxVpuProperty_LayerEnable SetLayerEnable;
    int ret =0;


    SetLayerEnable.layer = GX_LAYER_OSD;
    SetLayerEnable.enable = TRUE;
    ret |= GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

    gxlogd("\n********ttx_osd_ensable ret:%d*********\n",ret);
}
void ttx_osd_disable(void)
{

    GxVpuProperty_LayerEnable SetLayerEnable;
    int ret =0;


    SetLayerEnable.layer = GX_LAYER_OSD;
    SetLayerEnable.enable = FALSE;
    ret |= GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_LayerEnable, &SetLayerEnable, sizeof(GxVpuProperty_LayerEnable));

    gxlogd("\n********ttx_osd_disable ret:%d*********\n",ret);
}


void ttx_record_stb_palette(void)
{
    //int ret =0;
    int i=0;
    GxVpuProperty_Palette stb_palette;

    stb_palette.surface = TtxOsdScreen;
    GxAVGetProperty(g_device_handle,
            vpu_handle, GxVpuPropertyID_Palette, &stb_palette, sizeof(GxVpuProperty_Palette));

    for(i=0;i<256;i++)
    {
        stb_palette_color[i].r = stb_palette.palette.entries[i].r;
        stb_palette_color[i].g = stb_palette.palette.entries[i].g;
        stb_palette_color[i].b = stb_palette.palette.entries[i].b;
        stb_palette_color[i].a = stb_palette.palette.entries[i].a;
        if(stb_palette_color[i].a == 0)
        {
            osd_trans_index = i;
        }
    }

}


int ttx_set_stb_palette(void)
{
    GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};
    GxVpuProperty_Palette stb_palette;
    int ret = 0;
    int i=0;

    stb_palette.surface = TtxOsdScreen;
    ret = GxAVGetProperty(g_device_handle,
            vpu_handle, GxVpuPropertyID_Palette, &stb_palette, sizeof(GxVpuProperty_Palette));

    SurfaceBindPalette.surface = TtxOsdScreen;
    SurfaceBindPalette.palette = &(stb_palette.palette);
    ret = GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_SurfaceBindPalette,
            &SurfaceBindPalette,
            sizeof(GxVpuProperty_SurfaceBindPalette));

    for(i=0;i<256;i++)
    {
        stb_palette.palette.entries[i].r = stb_palette_color[i].r;
        stb_palette.palette.entries[i].g = stb_palette_color[i].g;
        stb_palette.palette.entries[i].b = stb_palette_color[i].b;
        stb_palette.palette.entries[i].a = stb_palette_color[i].a;
    }

    return (ret);
}



int ttx_set_ttx_palette(void)
{
    GxVpuProperty_SurfaceBindPalette SurfaceBindPalette = {0};
    GxVpuProperty_Palette stb_palette;
    int ret = 0;
    uint16_t Pos;
    uint32_t* pGuiClut;


    stb_palette.surface = TtxOsdScreen;
    ret = GxAVGetProperty(g_device_handle,
            vpu_handle, GxVpuPropertyID_Palette, &stb_palette, sizeof(GxVpuProperty_Palette));

    SurfaceBindPalette.surface = TtxOsdScreen;
    SurfaceBindPalette.palette = &(stb_palette.palette);
    ret = GxAVSetProperty(g_device_handle,
            vpu_handle,
            GxVpuPropertyID_SurfaceBindPalette,
            &SurfaceBindPalette,
            sizeof(GxVpuProperty_SurfaceBindPalette));

    for(Pos=0;Pos<256;Pos++)
    {
        pGuiClut = &TTX_CLUT[Pos%32];
        stb_palette.palette.entries[Pos].r = *(pGuiClut) & 0xff;
        stb_palette.palette.entries[Pos].g = (*(pGuiClut) & 0xff00) >> 8;
        stb_palette.palette.entries[Pos].b = *(pGuiClut) >> 16;
        switch(Pos/32)
        {
            case 0:
                stb_palette.palette.entries[Pos].a = 0xff;
                break;
            case 1:
                stb_palette.palette.entries[Pos].a = 0xe0;
                break;
            case 2:
                stb_palette.palette.entries[Pos].a = 0xb0;
                break;
            case 3:
                stb_palette.palette.entries[Pos].a = 0x90;
                break;
            case 4:
                stb_palette.palette.entries[Pos].a = 0x70;
                break;
            case 5:
                stb_palette.palette.entries[Pos].a = 0x50;
                break;
            case 6:
                stb_palette.palette.entries[Pos].a = 0x30;
                break;
            case 7:
                stb_palette.palette.entries[Pos].a = 0x10;
                break;
            case 8:
                stb_palette.palette.entries[Pos].a = 0x0;
                break;
        }
        if(*pGuiClut == 0)
        {
            stb_palette.palette.entries[Pos].a = 0x0;
        }
    }


    return (ret);
}


/* End of file -------------------------------------------------------------*/

