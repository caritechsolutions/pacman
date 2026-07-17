#include "app.h"
#if ARIBCC_SUPPORT
#include "gxaribcc.h"
#include "module/ttx/ttx_sub.h"
#include "module/subtitle/gxsubtitle.h"

static GxAribCC_RenderType	render_type;
static GxAribCC_RenderSize render_size;
static unsigned char render_size_refill = 0;
static handle_t subtitle_t = 0;
static handle_t cc_t = 0;

#define OSD_720P 0
#define ARIB_TEXT_COLOR 0xffffffff
#define ARIB_FILL_COLOR 0x80000000
#define ARIB_BLACK_COLOR 0x00000000
#define ARIB_RECT_X1 0
#define ARIB_RECT_X2 1024  //720
#define ARIB_RECT_Y1 (460+0)
#define ARIB_RECT_Y2 (ARIB_RECT_Y1+ARIB_RECT_HEIGHT)
#define ARIB_RECT_HEIGHT 30
#define VIDEO_WINDOW_W   1024
#define VIDEO_WINDOW_H   576
#define ARIB_SPP_WIN_W   VIDEO_WINDOW_W

#define ARIBCC_SPP_USBER_GUI_MODE
typedef struct GXARIBCC_Rect_t {
	unsigned int x, y;
	unsigned int w, h;
} GXARIBCC_Rect;

typedef struct rect_arib_t
{
	int x1;
	int y1;
	int x2;
	int y2;
}RECT_ARIB;

typedef struct RECT_t
{
	int iLeft;
	int iTop;
	int iWidth;
	int iHeight;
}RECT;

typedef struct _TT_BITMAP_t
{
	int iLeft;
	int iTop;
	int iWidth;
	int iHeight;
	unsigned char *pucBuffer;
}_TT_BITMAP;

typedef struct _TT_OUTLINE_t
{
	int iLeft;
	int iTop;
	int iWidth;
	int iHeight;
	unsigned char *pucBuffer;
}_TT_OUTLINE;


typedef enum {
	ENGLISH_LANGUAGE = 0x01,
	CHINESE_LANGUAGE,
	ARABIC_LANGUAGE,
	GERMAN_LANGUAGE,
	FRENCH_LANGUAGE,
	SPANISH_LANGUAGE,
	ITALIANO_LANGUAGE,
	PORTUGU_LANGUAGE,
	RUSSIAN_LANGUAGE,
	GREEK_LANGUAGE,
} GxAribCCLanuage;


#define	MAX(_a,_b)	((_a)<(_b)?(_b):(_a))
#define CC_REG_GET_BYTE0(reg)  ((reg)  &  0xFF)
#define CC_REG_GET_BYTE1(reg)  (((reg) >> 8) & 0xFF)

#define CC_GET_ENDIAN_16(reg)                     \
    (CC_REG_GET_BYTE0(reg) << 8 ) |              \
(CC_REG_GET_BYTE1(reg))


#define ARGB_SKIP_SIZE	1024*1024
static int thiz_cc_dev_hdl = 0;
static int thiz_cc_spp_hdl = 0;
static GxVpuProperty_CreateSurface cc_surface;
static void *CC_MmapBuffer = NULL;
extern uint32_t CLUT[4][8];
extern uint32_t  CC_CLUT[32];

#ifdef ARIBCC_SPP_USBER_GUI_MODE

extern int main_surface_flag[2];
extern GuiSurface *hd_get_surface0(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force);
extern int gdi_text_len(void *string);
extern void* GXGDI_GetFont(void);
extern int GXGDI_GetFontHeight(void* pfont);

static GuiSurface *cc_hd_get_surface(GAL_Rect *rect, int bpp, int color_format, void *buffer, int force)
{
	int ret = 0;
	GuiSurface *surface;
	int real_bpp = 0;
	GxVpuProperty_CreateSurface CreateSurface = {0};

	if (rect == NULL)
		return NULL;

	if (rect->w == 0 || rect->h == 0) {
		printf("%s error: width: %d, height: %d\n", __func__, rect->w, rect->h);
		return NULL;
	}

	surface = (GuiSurface *)GxCore_Mallocz(sizeof(GuiSurface));
	if (surface == NULL) {
		printf("[cc] Alloc GuiSurface error.\n");
		return NULL;
	}

	real_bpp = bpp;
	if ((bpp == 0) || (bpp == 2))
		real_bpp = 16;
	else if(bpp == 3)
		real_bpp = 8;
	else if(bpp == 4)
		real_bpp = 2;

	surface->bpp = real_bpp;
	if(bpp == 4)
		surface->buffer_size = rect->w * rect->h * 3 / 2;
	else
		surface->buffer_size = rect->w * rect->h * real_bpp / 8;
	surface->sf.width = rect->w;
	surface->sf.height = rect->h;
	surface->sf.color_format = color_format;
	surface->hw  = 0;

	memset(&CreateSurface, 0, sizeof(CreateSurface));
	CreateSurface.format = color_format;
	CreateSurface.width  = rect->w;
	CreateSurface.height = rect->h;
	CreateSurface.mode = GX_SURFACE_MODE_IMAGE;
	CreateSurface.buffer = surface->sf.buffer;

	ret |= GxAVGetProperty(thiz_cc_dev_hdl,
			thiz_cc_spp_hdl,
			GxVpuPropertyID_CreateSurface,
			&CreateSurface,
			sizeof(GxVpuProperty_CreateSurface));

	if(ret != 0) {
		printf("[cc]Cannot create surface : %d\n", ret);
		GxCore_Free(surface);
		return (NULL);
	}

	surface->hw |= 1;
	surface->hw_surface = CreateSurface.surface;
	if (surface->sf.buffer == 0)
	{
		surface->sf.buffer = CreateSurface.buffer;
		surface->data = (uint8_t *)GxCore_Map(thiz_cc_dev_hdl, (uint32_t)surface->sf.buffer, surface->buffer_size);
	}
	return surface;
}

static void cc_hd_free_surface(GuiSurface *surface)
{
	if (surface == NULL)
		return;

	if (surface->hw) {
		int ret;
		GxVpuProperty_DestroySurface DesSurface = { 0 };

		memset(&DesSurface, 0, sizeof(DesSurface));
		DesSurface.surface = surface->hw_surface;
		GxCore_UnMap(thiz_cc_dev_hdl, surface->data, surface->buffer_size);

		ret = GxAVSetProperty(thiz_cc_dev_hdl,
				thiz_cc_spp_hdl,
				GxVpuPropertyID_DestroySurface,
				&DesSurface,
				sizeof(GxVpuProperty_DestroySurface));
		if(0 != ret)
			printf("[CC]Set OSD Destroy surface failed!\n");
	}

	GxCore_Free(surface);
}

static int aribcc_hd_enable_img(int flag)
{
	int ret = 0;//GxAvdev_LayerEnable(thiz_cc_dev_hdl, thiz_cc_spp_hdl, GX_LAYER_SPP, flag);
	if(0 != ret) {
		printf("[CC]Set Layer enable failed!\n");
		return (1);
	}

	return ret;
}

static int aribcc_hd_enable_video(int flag)
{
	int ret;

	flag = 1 ^ flag;
	ret = 0;//GxAvdev_SetLayerTop(thiz_cc_dev_hdl, thiz_cc_spp_hdl, GX_LAYER_SPP, flag);
	if(0 != ret) {
		printf("[CC]Set SPP top or bottom failed!\n");
		return 1;
	}

	return 0;
}

static int cc_hd_end_blit_list(void)
{
	GxVpuProperty_EndUpdate end = {0};
	int ret = 0;

	ret = GxAVSetProperty(thiz_cc_dev_hdl,
		thiz_cc_spp_hdl,
		GxVpuPropertyID_EndUpdate,
		&end,
		sizeof(GxVpuProperty_EndUpdate));
	return (ret);
}

int cc_spp_draw_init(void)
{
	return 0;
}

static int aribcc_clean_fill_rect(
		int	x,
		int	y,
		int	width,
		int	height,
		void *surface,
		unsigned int	color)
{
	int ret = 0;
	GxVpuProperty_FillRect FillRect={0};

	FillRect.surface = surface;
	FillRect.is_ksurface    = 3;//font_surface->hw;
	FillRect.rect.x = x;
	FillRect.rect.y = y;
	FillRect.rect.width = width;
	FillRect.rect.height = height;
	if(surface == render_size.surface)
	{
		FillRect.color.y = (color & 0xfc00)>>8;
		FillRect.color.cb = (color & 0x03c0)>>2;
		FillRect.color.cr = (color & 0x3c)<<2;
		FillRect.color.alpha = 0;
		if(color & 0x3)
		{
			FillRect.color.alpha= 0xff;
		}
	}
	else
	{
		FillRect.color.r       = ((color >> 16) & 0x0000FF);//0xff;
		FillRect.color.g      = (color >> 8) & 0x0000FF;
		FillRect.color.b      = (color & 0x0000FF);
		FillRect.color.r = (color & 0xF800) >> 8;
		FillRect.color.g = (color & 0x07E0) >> 3;
		FillRect.color.b = (color & 0x001F) << 3;
		FillRect.color.alpha   = 0;
	}

	ret = GxAVSetProperty(thiz_cc_dev_hdl, thiz_cc_spp_hdl,GxVpuPropertyID_FillRect,
		&FillRect,
		sizeof(GxVpuProperty_FillRect));

	return ret;
}

static int aribcc_hd_copy_surface(void *src, GXARIBCC_Rect *src_rect, GXARIBCC_Rect *dst_rect)
{
	int ret = 0;
	GxVpuProperty_Blit Blit;
	GxVpuProperty_BeginUpdate Begin = {0};

	if(src == NULL || src_rect == NULL || render_size.surface == NULL || dst_rect == NULL)
		return 1;

	if((src_rect->w <= 0) || (src_rect->h) <= 0)
		return 1;

	Begin.max_job_num = 2;

	ret = GxAVSetProperty(thiz_cc_dev_hdl,
				thiz_cc_spp_hdl,
				GxVpuPropertyID_BeginUpdate,
				&Begin,
				sizeof(GxVpuProperty_BeginUpdate));
	if(ret != 0)
	{
		printf("[CC]Stretch set begin failed!\n");
		return (1);
	}

	memset(&Blit, 0, sizeof(GxVpuProperty_Blit));

	Blit.srca.dst_format  = GX_COLOR_FMT_RGB565;//src->sf.color_format;//get_color_format(0, gui.config.bpp);
	Blit.srca.surface     = src;
	Blit.srca.is_ksurface = 1;
	Blit.srca.alpha       = 1;
	Blit.srca.rect.x      = src_rect->x;
	Blit.srca.rect.y      = src_rect->y;
	Blit.srca.rect.width  = src_rect->w;
	Blit.srca.rect.height = src_rect->h;

	Blit.srcb.surface     = render_size.surface;
	Blit.srcb.dst_format  = GX_COLOR_FMT_YCBCRA6442;
	Blit.srcb.is_ksurface = 1;
	Blit.srcb.rect        = Blit.srca.rect;
	Blit.srcb.rect.x      = dst_rect->x;
	Blit.srcb.rect.y      = dst_rect->y;

	Blit.dst = Blit.srcb;
	Blit.mode = GX_ALU_ROP_COPY;
	Blit.colorkey_info.mode = GX_BLIT_COLORKEY_BASIC_MODE;
	Blit.dst.alpha = 230;
	Blit.colorkey_info.src_colorkey_en = 0;

	ret = GxAVSetProperty(thiz_cc_dev_hdl,
			thiz_cc_spp_hdl,
			GxVpuPropertyID_Blit,
			&Blit,
			sizeof(GxVpuProperty_Blit));
	if(0 != ret)
	{
		printf("[CC]----------Copy Failed-----------\n");
	}

	ret |= cc_hd_end_blit_list();

	return (ret);
}


int aribcc_spp_draw_string(void *string,uint32_t x, uint32_t y, int alignment, int color, STRING_TYPE flag)
{
	int ret = -1;
	GXARIBCC_Rect rect={0};
	GAL_Rect create_rect = {0};
	GuiSurface *font_surface=NULL;
	int width = 0;
	int height = 0;
	void* font=NULL;
	GXARIBCC_Rect font_rect={0};
	GxColorFormat font_color_format = GX_COLOR_FMT_RGB565;
	GxVpuProperty_FillRect FillRect={0};
	uint8_t font_bpp=16;

	aribcc_hd_enable_img(FALSE);
	font=GXGDI_GetFont();
	width = gdi_text_len(string);
	height = GXGDI_GetFontHeight(font);
	create_rect.x = 0;
	create_rect.y = 0;
	create_rect.w = width;
	create_rect.h = height;
	font_surface = cc_hd_get_surface(&create_rect, font_bpp, font_color_format, NULL, TRUE);
	if (NULL == font_surface)
	{
		cc_hd_free_surface(font_surface);
		return -1;
	}

	memset(&FillRect,0,sizeof(FillRect));
	FillRect.surface = font_surface->hw_surface;
	FillRect.is_ksurface    = 3;//font_surface->hw;
	FillRect.rect.x = 0;
	FillRect.rect.y = 0;
	FillRect.rect.width = width;
	FillRect.rect.height = height;
	FillRect.color.r       = 0;
	FillRect.color.g       = 0;
	FillRect.color.b       = 0;
	FillRect.color.alpha   = 0;
	#if 1
	//if(strstr((char *)string,bak_buffer)==NULL)
	{
	ret = GxAVSetProperty(thiz_cc_dev_hdl, thiz_cc_spp_hdl,GxVpuPropertyID_FillRect,
		&FillRect,
		sizeof(GxVpuProperty_FillRect));
	//cc_hd_end_blit_list();
	}
	#endif

	memset(&FillRect,0,sizeof(FillRect));
	FillRect.surface = render_size.surface;//spp_sureface_screen->hw_surface;
	FillRect.is_ksurface    = 3;//spp_sureface_screen->hw;
	FillRect.rect.x = x;
	FillRect.rect.y = y;
	FillRect.rect.width = VIDEO_WINDOW_W-x;
	FillRect.rect.height = height;//ARIB_RECT_HEIGHT*3;//VIDEO_WINDOW_H;
	FillRect.color.y        = 16;//C81818
	FillRect.color.cb       = 128;
	FillRect.color.cr       = 128;
	FillRect.color.alpha    = 0;
	/*ret = GxAVSetProperty(thiz_cc_dev_hdl, thiz_cc_spp_hdl,GxVpuPropertyID_FillRect,
		&FillRect,
		sizeof(GxVpuProperty_FillRect));*/
	//cc_hd_end_blit_list();
//	return 0;

	font_rect.x=0;
	font_rect.y=0;
	font_rect.w=width;
	font_rect.h=height;
	GXGDI_DrawString(font_surface,string,(GXGDI_Rect*)&font_rect,alignment,color,flag);
	//GXGDI_DrawString(cc_surface.surface,string,(GXGDI_Rect*)&font_rect,alignment,color,flag);
	rect.x=x;
	rect.y=y;
	rect.w=width;
	rect.h=height;
	//ret =  GXGDI_Blit(font_surface, (GXGDI_Rect*)&font_rect, render_size.surface, (GXGDI_Rect*)&rect, GDI_BLIT_COPY);
	//ret = aribcc_hd_copy_surface(font_surface, &font_rect, &rect);
	ret = aribcc_hd_copy_surface(font_surface->hw_surface, &font_rect, &rect);
	//ret = aribcc_hd_copy_surface(cc_surface.surface, &font_rect, &rect);

	//memset(bak_buffer,0,256);
	//memcpy(bak_buffer,string,strlen(string));
	//cc_hd_end_blit_list();
	cc_hd_free_surface(font_surface);
	//aribcc_hd_free_surface(font_surface);
	aribcc_hd_enable_img(TRUE);
	aribcc_hd_enable_video(FALSE);
	if (GXCORE_SUCCESS != ret)
	{
		return -1;
	}
	return ret;
}

#endif

static unsigned int BlendBackColor32(unsigned int uiBackColor, unsigned int uiFrontColor)
{
	unsigned int color = 0;
	color = CLUT[1][0];
	return color;
}

static bool RectAnd(RECT stObj, RECT stCanvas, RECT *pstBuff, RECT *pstPaint)
{
	pstBuff->iLeft = stObj.iLeft;
	pstBuff->iTop = stObj.iTop;
	pstBuff->iWidth = stObj.iWidth;
	pstBuff->iHeight = stObj.iHeight;

	pstPaint->iLeft = 40;//stCanvas.iLeft;
	pstPaint->iTop = stCanvas.iTop;
	pstPaint->iWidth = s_AsciiFontWidth;//stCanvas.iWidth;
	pstPaint->iHeight = s_AsciiFontHeight;//stCanvas.iHeight;
	return TRUE;
}
static int GetCharMetricsCommon(int iLan, char cCharCode, _TT_OUTLINE *pstOutline, int *piHeight, int *piAscender, int *piAdvance)
{
	*piAscender = s_AsciiFontWidth;
	*piAdvance = s_AsciiFontHeight;
	return 0;
}

static int RenderGlyph(_TT_OUTLINE *stOutline, _TT_BITMAP *pstBitmap, int iXTimes, int iYTimes)
{
	pstBitmap->iLeft = 40;
	pstBitmap->iTop = 400;
	pstBitmap->iWidth = 25;
	pstBitmap->iHeight = 25;
	return 0;
}

static void SetRect(RECT *pstRect, int iLeft, int iTop, int iWidth, int iHeight)
{
	pstRect->iLeft = iLeft;
	pstRect->iTop = iTop;
	pstRect->iWidth = iWidth;
	pstRect->iHeight = iHeight;
	return;
}

static void ARIB_Flip(void)
{
#ifdef ARIBCC_SPP_USBER_GUI_MODE
	cc_hd_end_blit_list();
	return ;
#endif

	if(render_type ==GXARIBCC_OSD )
	{
		//mwom_flip();
	}
	else
	{
		GxVpuProperty_Blit Blit;
		int Ret = 0;

		memset(&Blit,0,sizeof(GxVpuProperty_Blit));

		Blit.mode = GX_ALU_ROP_COPY;
		Blit.srca.surface = cc_surface.surface;
		Blit.srca.is_ksurface = 1;
		Blit.srca.rect.x = 0;
		Blit.srca.rect.y = 0;
		Blit.srca.rect.width = cc_surface.width;
		Blit.srca.rect.height = cc_surface.height;
		Blit.srcb.surface = render_size.surface;
		Blit.srcb.is_ksurface = 1;
		Blit.srcb.rect.x = 0;
		Blit.srcb.rect.y = 0;
		Blit.srcb.rect.width = render_size.width;
		Blit.srcb.rect.height = render_size.height;
		Blit.dst = Blit.srcb;

		Ret = GxAVSetProperty(thiz_cc_dev_hdl,
				thiz_cc_spp_hdl,
				GxVpuPropertyID_Blit,
				&Blit,
				sizeof(GxVpuProperty_Blit));
		if(Ret != GXCORE_SUCCESS)
		{
			printf("GxVpuPropertyID_Blit Error!!!! %d\n",Ret);
		}
	}
}

static int ARIB_FillRect(
		int	iPosX,
		int	iPosY,
		int	uiWidth,
		int	uiHeight,
		unsigned int	uiColor)
{
#ifdef ARIBCC_SPP_USBER_GUI_MODE
	aribcc_clean_fill_rect(iPosX,iPosY,uiWidth,uiHeight,render_size.surface,uiColor);
	return 0;
#endif
	if(render_type ==GXARIBCC_OSD )
	{
#if OSD_720P
		iPosX = GetXPos(iPosX);
		iPosY = GetYPos(iPosY);
		uiWidth = GetXPos(uiWidth);
		uiHeight = GetYPos(uiHeight);
#endif
		if(uiWidth==0 || uiHeight==0 /*|| (iPosX + uiWidth) > GetOSDWidth() || (iPosY + uiHeight) > GetOSDHeight() */)
		{
			return 0;
		}
		else
		{
			return 0;//FB_FillRect(iPosX,iPosY,uiWidth,uiHeight,uiColor);
		}
	}
	else
	{
		if(uiWidth==0 || uiHeight==0 /*|| (iPosX + uiWidth) > GetOSDWidth() || (iPosY + uiHeight) > GetOSDHeight()*/ )
		{
			return 0;
		}
		else
		{
			uint32_t *spp32bpp_buf = (uint32_t*)cc_surface.buffer;
			int xres = render_size.width;
			int i,j;
			for (i=0; i < uiHeight; ++i)
			{
				for (j=0; j < uiWidth; ++j)
					*(spp32bpp_buf+xres*iPosY+iPosX+xres*i+j) = uiColor;
			}
		}
		return 0;
	}
}

static void cc_spp_set_point(GxVpuProperty_Point * point)
{
	unsigned int addr;
	unsigned int valColor = 0;

	addr = (unsigned int)cc_surface.buffer + ((16 * (point->point.y * cc_surface.width + point->point.x)) >> 3);

	if(point->point.x>cc_surface.width || point->point.y >cc_surface.height)
		return;

	valColor = (((point->color.y) & 0xFC) << 8)
		| (((point->color.cb) & 0xF0) << 2)
		| (((point->color.cr) & 0xF0) >> 2)
		| (((point->color.a) & 0xC0) >> 6);

	*(unsigned short *)addr = CC_GET_ENDIAN_16(valColor);
}

void cc_spp_draw_text( uint32_t DstPositionX,
	uint32_t DstPositionY,
	uint32_t Width,
	uint32_t Height,
	const uint8_t  *pData,
	uint32_t FrontColor,
	uint32_t BackColor)
{
	uint8_t i=0;
	uint8_t j=0;
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
		for(j=0;j<Width;j++)
		{
			point.surface = cc_surface.surface;
			point.point.x = x++;
			point.point.y = y;

			if ((j != 0) && ( (j&0x7) == 0))
			{
				p++;
			}
			if(1)//(*p)&(0x80>>u++)
			{
				memcpy((void*)&(point.color),(void*)&front_color,gxcolorlen);
			}
			else
			{
				memcpy((void*)&(point.color),(void*)&back_color,gxcolorlen);
			}
			cc_spp_set_point(&point);
		}
	}
}

extern unsigned char txtlib_bin[];
extern uint32_t g_OsdTtxLang;

#define G0_CHARACTER_COUNT 96
#define LATIN_G0_START_ADD   0
#define LATIN_NATIONAL_SUB_START_ADD   (LATIN_G0_START_ADD+576)          //96*6
#define G1_BLOCK_CONTIGUOUS_START_ADD   (LATIN_NATIONAL_SUB_START_ADD+1014) //13*13*6
#define G1_BLOCK_SEPARATED_START_ADD  (G1_BLOCK_CONTIGUOUS_START_ADD+576)
#define G3_SMOOTH_START_ADD  (G1_BLOCK_SEPARATED_START_ADD+576)//reserve
#define LATIN_G2_START_ADD	 (G3_SMOOTH_START_ADD+576)
#define CYRILLIC_G0_1  (LATIN_G2_START_ADD+576)
#define CYRILLIC_G0_2  (CYRILLIC_G0_1+576)
#define CYRILLIC_G0_3  (CYRILLIC_G0_2+576)
#define CYRILLIC_G2  (CYRILLIC_G0_3+576)
#define GREEK_G0  (CYRILLIC_G2+576)
#define GREEK_G2  (GREEK_G0+576)
#define ARABIC_G0  (GREEK_G2+576)
#define ARABIC_G2  (ARABIC_G0+576)
#define HEBREW_G0   (ARABIC_G2+576)

static void cc_spp_draw_context(int x,int y,struct osd_view*osd_view)
{
	uint8_t i = 0, chLanSel = 0;
	int X = 0, Y = 0;
//	uint32_t* p = NULL;
	uint32_t nStartAdd;
	uint8_t LatinSubCharIndex[SUBCHAR_NUMBER] = {G0_2_3,G0_2_4,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,G0_6_0,G0_7_B,G0_7_C,G0_7_D,G0_7_E};
	uint8_t BlockSubCharIndex[SUBCHAR_NUMBER] = {-1,-1,G0_4_0,G0_5_B,G0_5_C,G0_5_D,G0_5_E,G0_5_F,-1,-1,-1,-1,-1};
	uint32_t SectionAddr = 0;
	uint8_t chSubLangSel = 0;
	uint8_t* pTemp = (uint8_t*)txtlib_bin;

	if((osd_view->m_chSetSelect==0)||(osd_view->m_chSetSelect==8)||(osd_view->m_chX26ColumnMode16==1))
	{
		if((g_OsdTtxLang != 0xFF) && (osd_view->m_chSetSelect==0))
		{
			chLanSel = Polish;//g_OsdTtxLang;
			switch(chLanSel)
			{
				case Arabic:
					chLanSel = 0x57;
					goto setup2;
					break;
				case Russian:
					chLanSel = Serbian_Cyrillic_2_Set;
					goto setup2;
					break;
				case Greek:
					chLanSel = Greek_Set;
					goto setup2;
					break;
				default:
					break;
			}
		}

		if(osd_view->m_chX26ColumnMode16==1)
		{
			chLanSel = English;
		}

		switch(chLanSel)
		{
			case German://german
				chSubLangSel = 4;
				break;
			case Swedish://Swedish/Finnish/Hungarian
				chSubLangSel = 11;
				break;
			case Italian://Italian
				chSubLangSel = 5;
				break;
			case French://French
				chSubLangSel = 3;
				break;
			case Portuguese://Portuguese/Spanish
				chSubLangSel = 8;
				break;
			case Czech:
				chSubLangSel = 0;//Czech/Slovak
				break;
			case Turkish:
				chSubLangSel=12;//Turkish
				break;
			case Rumanian:
				chSubLangSel=9;//Rumanian
				break;
			case Polish:
				chSubLangSel=7;
				break;
			default:
				chSubLangSel = 1;//english
				break;
		}
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(osd_view->m_chChar==LatinSubCharIndex[i])
			{
				SectionAddr=*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+5));
				break;
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
			if(osd_view->m_chSetSelect==0)
			{
				chLanSel=osd_view->m_chG0G2LanguageSelect;
			}
			else
			{
				chLanSel=osd_view->m_chG02LanguageSelect;
			}
setup2:
			switch(chLanSel)
			{
				case Serbian_Cyrillic_1_Set:
					nStartAdd=CYRILLIC_G0_1;
					break;
				case Serbian_Cyrillic_2_Set:
					nStartAdd=CYRILLIC_G0_2;
					break;
				case Serbian_Cyrillic_3_Set:
					nStartAdd=CYRILLIC_G0_3;
					break;
				case Greek_Set:
					nStartAdd=GREEK_G0;
					break;
				case Hebrew_Set:
					nStartAdd=HEBREW_G0;
					break;
				case 0x47:
				case 0x57:
					nStartAdd=ARABIC_G0;
					break;
				default:
					nStartAdd=LATIN_G0_START_ADD;
					break;
			}

			SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
			SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
		}
	}
	//G2×Ö·û¼¯
	else if(osd_view->m_chSetSelect==1)
	{
		if(g_OsdTtxLang != 0xFF)
		{
			chLanSel = g_OsdTtxLang;
			switch(chLanSel)
			{
				case Arabic:
					chLanSel = 0x57;
					break;
				case Russian:
					chLanSel = Serbian_Cyrillic_2_Set;
					break;
				case Greek:
					chLanSel = Greek_Set;
					break;
				default:
					chSubLangSel = 0xff;//latin g2
					break;
			}
		}
		else
		{
			chLanSel=osd_view->m_chG0G2LanguageSelect;
		}
		switch(chLanSel)
		{
			case Serbian_Cyrillic_1_Set:
			case Serbian_Cyrillic_2_Set:
			case Serbian_Cyrillic_3_Set:
				nStartAdd=CYRILLIC_G2;
				break;
			case Greek_Set:
				nStartAdd=GREEK_G2;
				break;
			case 0x40:
			case 0x41:
			case 0x42:
			case 0x43:
			case 0x44:
			case 0x45:
			case 0x46:
			case 0x47:
			case 0x50:
			case 0x51:
			case 0x52:
			case 0x53:
			case 0x54:
			case 0x55:
			case 0x56:
			case 0x57:
				nStartAdd=ARABIC_G2;
				break;
			default:
				nStartAdd=LATIN_G2_START_ADD;
				break;
		}
		SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
	}
	//ÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect&2)
	{
		if(g_OsdTtxLang != 0XFF)
		{
			switch(g_OsdTtxLang)
			{
				case German://german
					chSubLangSel = 4;
					break;
				case Swedish://Swedish/Finnish/Hungarian
					chSubLangSel = 11;
					break;
				case Italian://Italian
					chSubLangSel = 5;
					break;
				case French://French
					chSubLangSel = 3;
					break;
				case Portuguese://Portuguese/Spanish
					chSubLangSel = 8;
					break;
				case Czech:
					chSubLangSel = 0;//Czech/Slovak
					break;
				case Turkish:
					chSubLangSel=12;//Turkish
					break;
				case Rumanian:
					chSubLangSel=9;//Rumanian
					break;
				case Polish:
					chSubLangSel=7;
					break;
				case Arabic:
					osd_view->m_chG0G2LanguageSelect = Arabic_Set;
					goto setup3;
					break;
				case Russian:
					osd_view->m_chG0G2LanguageSelect = Serbian_Cyrillic_2_Set;
					goto setup3;
					break;
				case Greek:
					osd_view->m_chG0G2LanguageSelect = Greek_Set;
					goto setup3;
					break;
				default:
					chSubLangSel = 1;//english
					break;
			}
		}
		else
		{
			switch(osd_view->m_chG0G2LanguageSelect)
			{
				case German://german
					chSubLangSel = 4;
					break;
				case Swedish://Swedish/Finnish/Hungarian
					chSubLangSel = 11;
					break;
				case Italian://Italian
					chSubLangSel = 5;
					break;
				case French://French
					chSubLangSel = 3;
					break;
				case Portuguese://Portuguese/Spanish
					chSubLangSel = 8;
					break;
				case Czech:
					chSubLangSel = 0;//Czech/Slovak
					break;
				case Turkish:
					chSubLangSel=12;//Turkish
					break;
				case Rumanian:
					chSubLangSel=9;//Rumanian
					break;
				case Polish:
					chSubLangSel=7;
					break;
				default:
					chSubLangSel = 1;//english
					break;
			}
		}
		for(i=0;i<SUBCHAR_NUMBER;i++)
		{
			if(osd_view->m_chChar==BlockSubCharIndex[i])
			{
				SectionAddr=*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i*6+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+LATIN_NATIONAL_SUB_START_ADD+13*chSubLangSel*6+i+5));
				break;
			}
		}
		if(i==SUBCHAR_NUMBER)//·ÇÒõÓ°×Ö·û
		{
setup3:
			if((osd_view->m_chChar>0x3f)&&(osd_view->m_chChar<0x60))
			{
				switch(osd_view->m_chG0G2LanguageSelect)
				{
					case Serbian_Cyrillic_1_Set:
						nStartAdd=CYRILLIC_G0_1;
						break;
					case Serbian_Cyrillic_2_Set:
						nStartAdd=CYRILLIC_G0_2;
						break;
					case Serbian_Cyrillic_3_Set:
						nStartAdd=CYRILLIC_G0_3;
						break;
					case Greek_Set:
						nStartAdd=GREEK_G0;
						break;
					case Arabic_Set:
						nStartAdd=ARABIC_G0;
						break;
					default:
						nStartAdd=LATIN_G0_START_ADD;
						break;
				}
				SectionAddr=*(pTemp+6+nStartAdd+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+nStartAdd+6+6*(osd_view->m_chChar-0x20)+5));
			}
			else if(osd_view->m_chSetSelect==2)//·ÖÁÑÏâÇ¶×Ö·û¼¯
			{
				SectionAddr=*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_SEPARATED_START_ADD+6*(osd_view->m_chChar-0x20)+5));
			}
			else
			{
				SectionAddr=*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+2);
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+3));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+4));
				SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G1_BLOCK_CONTIGUOUS_START_ADD+6*(osd_view->m_chChar-0x20)+5));
			}

		}
	}
	//Æ½»¬ÏâÇ¶×Ö·û¼¯
	else if(osd_view->m_chSetSelect==4)
	{
		SectionAddr=*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+2);
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+3));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+4));
		SectionAddr=(SectionAddr<<8)+(*(pTemp+6+G3_SMOOTH_START_ADD+6*(osd_view->m_chChar-0x20)+5));
	}
	X=x*s_AsciiFontWidth;
	Y=y*16;

	cc_spp_draw_text(X+100,Y+ARIB_RECT_Y1,s_AsciiFontWidth, s_AsciiFontHeight, (uint8_t*)SectionAddr,osd_view->m_chFrontColor,osd_view->m_chBackColor);
}

int cc_draw_screen(int x, int y,uint8_t m_chChar)
{
	static struct osd_view OsdView;

	OsdView.m_chSetSelect = 0;
	OsdView.m_chFlash =0;
	OsdView.m_chConceal = 0;
	OsdView.m_chMosaics = 0;
	OsdView.m_chContiguousM = 0xFF;
	OsdView.m_chHoldMosaics = 0xFF;
	OsdView.m_chHoldChar = 0xFF;
	OsdView.m_chX26ChangeDataEnable=0;
	OsdView.m_chX26ColumnMode16 = 0;
	OsdView.m_chBackColor = CLUT[1][0];
	OsdView.m_chFrontColor = CLUT[0][1];


	//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, CLUT[0][3]);
	//ARIB_Flip();
	OsdView.m_chChar = m_chChar;

	cc_spp_draw_context(x,y,&OsdView);
	//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+60, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, CLUT[0][1]);
	//ARIB_Flip();

	return 0;
}
static void aribcc_spp_clut_init(void)
{
	int i = 0;

	for(i=0;i<16;i++)
	{
		CLUT[i/8][i%8]= CC_CLUT[i];
	}
	return;
}

static int ARIB_PutRect(
		int	iPosX,
		int	iPosY,
		int	iWidth,
		int	iHeight,
		unsigned char*	pucData)
{
	uint32_t *spp32bpp_buf = (uint32_t*)cc_surface.buffer;
	int xres = render_size.width;
	int base = iWidth;
	int posx = iPosX;
	int posy = iPosY*xres;
	int i,j;
#ifdef ARIBCC_SPP_USBER_GUI_MODE
	return 0;
#endif

	if(render_type ==GXARIBCC_OSD )
	{
		return 0;//FB_PutRect(iPosX,iPosY,iWidth,iHeight,pucData);
	}
	else
	{
		for (i=0; i < iHeight; ++i) {
			for (j=0; j < iWidth; ++j)
				*(spp32bpp_buf+posy+xres*i+posx+j) = *((uint32_t *)pucData+base*i+j);
		}
		return 0;
	}
}

static int ARIB_GetRect(
		int	iPosX,
		int	iPosY,
		int	iWidth,
		int	iHeight,
		unsigned char*	pucData)
{
#ifdef ARIBCC_SPP_USBER_GUI_MODE
		return 0;
#endif

	if(render_type ==GXARIBCC_OSD ) {
		return 0;//FB_GetRect(iPosX,iPosY,iWidth,iHeight,pucData);
	}
	else {
		uint32_t *spp32bpp_buf = (uint32_t*)cc_surface.buffer;
		int xres = render_size.width;
		int base = iWidth;
		int posx = iPosX;
		int posy = iPosY*xres;
		int i,j;
		for (i=0; i < iHeight; ++i) {
			for (j=0; j < iWidth; ++j)
				*((uint32_t *)pucData+base*i+j) = *(spp32bpp_buf+posy+xres*i+posx+j);
		}
		return 0;
	}
}

static int ConvertCCLang_ARIB(GxAribCC_LangID langid)
{
	switch(langid)
	{
		case ARIB_LAN_ID_ENG:
			return ENGLISH_LANGUAGE;
		case ARIB_LAN_ID_CHN:
			return CHINESE_LANGUAGE;
		case ARIB_LAN_ID_ARB:
			return ARABIC_LANGUAGE;
		case ARIB_LAN_ID_DEU:
			return GERMAN_LANGUAGE;
		case ARIB_LAN_ID_FRH:
			return FRENCH_LANGUAGE;
		case ARIB_LAN_ID_SPA:
			return SPANISH_LANGUAGE;
		case ARIB_LAN_ID_ITA:
			return ITALIANO_LANGUAGE;
		case ARIB_LAN_ID_POR:
			return PORTUGU_LANGUAGE;
		case ARIB_LAN_ID_RUS:
			return RUSSIAN_LANGUAGE;
		case ARIB_LAN_ID_GRK:
			return GREEK_LANGUAGE;
		default:
			return ENGLISH_LANGUAGE;
	}
}

static int GetTextWidthByLang(int size, const char* acline2buffer, int langid)
{
	int ret = 100;
	int char_len = 0;
	if(acline2buffer!=NULL)
	{
		char_len = strlen(acline2buffer);
		ret = char_len*s_AsciiFontWidth;
		#ifdef ARIBCC_SPP_USBER_GUI_MODE
		ret = gdi_text_len((void*)acline2buffer);
		#endif
	}
	return ret;
}

static int GetTextLanguage(const char *pcText, int iLang)
{
	return 0;
}

static int IsValidEncoding(uint8_t ch_str)//language code from ISO-8859-1
{
	uint8_t tmp = 0;

	tmp = (ch_str & 0xf0) >> 4;
	if ((tmp == 0) || (tmp == 1) || (tmp == 8) || (tmp == 9))
	{
		return 0;
	}
	else
	{
		return 0;
	}
}

static void DrawBMPNoScale_ARIB(int iCol, int iRow, int iWidth, int iHeight, unsigned char *pucBmpData)
{
	if(pucBmpData == NULL)
	{
		return;
	}
	ARIB_PutRect(iCol, iRow, iWidth, iHeight, pucBmpData);
}

static void GetRectRegionNoScale_ARIB(int iX, int iY, int iWidth, int iHeight, unsigned char *pucDest)
{
	if(pucDest == NULL)
	{
		return;
	}
	ARIB_GetRect(iX, iY, iWidth, iHeight, pucDest);
}

static void DrawFontBitmap_ARIB(unsigned char *pucBuf, int iBufWidth, int iBufHeight, int iX, int iY, _TT_BITMAP *pstBitmap, unsigned int uiFrontColor)
{
	unsigned int uiForeColor;
	int iBufOffset;
	int i, j;
	RECT stCanvas, stObj, stBuff, stPaint;
	unsigned int *puiOsdBuf;
	//extern unsigned int BlendBackColor32(unsigned int uiBackColor, unsigned int uiFrontColor);
	//extern bool RectAnd(RECT stObj, RECT stCanvas, RECT *pstResult, RECT *pstBuff, RECT *pstPaint);

	SetRect(&stCanvas, 0, 0, iBufWidth, iBufHeight);
	SetRect(&stObj, iX, iY, pstBitmap->iWidth, pstBitmap->iHeight);

	if (!RectAnd(stObj, stCanvas, &stBuff, &stPaint))	//out of canvas region
		return;

	puiOsdBuf = GxCore_Malloc( stPaint.iWidth * stPaint.iHeight * 4 );
	if( puiOsdBuf == NULL )
		return;

	if(uiFrontColor == 1)
		uiFrontColor = 0xffffffff;

//	printf("---%d---%d--%d--%d\n",stPaint.iLeft,  stPaint.iTop, stPaint.iWidth, stPaint.iHeight);
	GetRectRegionNoScale_ARIB(stPaint.iLeft,  stPaint.iTop, stPaint.iWidth, stPaint.iHeight, (unsigned char*)puiOsdBuf);
	for (j = 0; j < stPaint.iHeight; j++)
	{
		for (i = 0; i < stPaint.iWidth; i++)
		{
			unsigned int uiBackColor;

			iBufOffset = (stBuff.iTop + j) * pstBitmap->iWidth + stBuff.iLeft + i;
			uiForeColor = (uiFrontColor & 0x00ffffff) | (pstBitmap->pucBuffer[ iBufOffset ] << 24);

			iBufOffset = (stBuff.iTop + j) * stPaint.iWidth + stBuff.iLeft + i;
			uiBackColor = puiOsdBuf[ iBufOffset ];

			puiOsdBuf[ iBufOffset ] = BlendBackColor32(uiBackColor, uiForeColor);//----
		}
	}
	DrawBMPNoScale_ARIB(stPaint.iLeft,  stPaint.iTop, stPaint.iWidth, stPaint.iHeight, (unsigned char*)puiOsdBuf);
	GxCore_Free( puiOsdBuf );
}

static int DrawAmplifyTextToBufferCommon_ARIB(int iLan, unsigned char *pucBuf, int iBufWidth, int iBufHeight, int iX, int iY, char *pcText,
		unsigned int uiFrontColor, int iXTimes, int iYTimes, bool bDrawSmall)
{
	int iCharNo = 0;
	int iCharWidth = 0;
	int iPenPos;
	const int FONT_MAX_SIZE = 50*50;// MaxFontSize(); /*return 50*50*/
	int iFontHeight = 0;
	//int iFontMaxHeight = 0;
	int iAscender = 0;
	_TT_OUTLINE stOutline;
	_TT_BITMAP stBitmap;
	int iDrawWidth = 0;
	int iDrawHeight = 0;
	//extern int GetCharMetricsCommon(int iLan, char cCharCode, GS_TT_OUTLINE *pstOutline, int *piHeight, int *piAscender, int *piAdvance);

	if (NULL == pcText)
		return 0;

	iPenPos = iX;

	stBitmap.pucBuffer = GxCore_Malloc(FONT_MAX_SIZE);
	if (NULL == stBitmap.pucBuffer)
		return 0;

	for (iCharNo = 0; iCharNo < strlen(pcText); iCharNo++)
	{
		GetCharMetricsCommon(iLan, pcText[ iCharNo ], &stOutline, &iFontHeight, &iAscender, &iCharWidth);
		//iFontMaxHeight = MAX(iFontMaxHeight, iFontHeight);
		memset(stBitmap.pucBuffer, 0, FONT_MAX_SIZE);

		RenderGlyph(&stOutline, &stBitmap, iXTimes, iYTimes);

		if (iPenPos == iX && stBitmap.iLeft < 0)
		{
			stBitmap.iLeft = 0;	//never draw out of area
		}

		/* draw a char */
		iDrawHeight = 0;
#if 1
		DrawFontBitmap_ARIB(pucBuf, iBufWidth, iBufHeight, iPenPos + stBitmap.iLeft, iY + iAscender - stBitmap.iTop + iDrawHeight, &stBitmap, uiFrontColor);
#else
		cc_draw_screen(iCharNo,0,pcText[iCharNo]);
#endif
//		iPenPos += (iCharWidth * iXTimes);
		iPenPos += (iCharWidth * iXTimes)>>6;
	}
	GxCore_Free(stBitmap.pucBuffer);
	iDrawWidth = iPenPos - iX;

	return iDrawWidth;
}

static int DrawAmplifyTextToBuffer_ARIB(unsigned char *pucBuf, int iBufWidth, int iBufHeight, int iX, int iY, char *pcText, unsigned int uiFrontColor, int iLan,
		int iXTimes, int iYTimes)
{
	int eLang;

	printf("DrawAmplifyTextToBuffer__ARIB === %s  %x\n", pcText, uiFrontColor);
	if (NULL == pcText)
		return 0;

	if(iX < 0 || iY < 0)
	{
		printf("[%s][%d]  Error, iX or iY is smaller than 0!\n", __FUNCTION__, __LINE__);
		return 0;
	}

#ifdef ARIBCC_SPP_USBER_GUI_MODE
	aribcc_spp_draw_string(pcText,iX,iY,0,uiFrontColor,GDI_STRING_SINGLE);
	return 0;
#endif

	eLang = GetTextLanguage(pcText, iLan);
	if (IsValidEncoding(pcText[ 0 ]))
	{
		pcText++;
	}
//	return DrawText(iX, iY, pcText, 0, uiFrontColor);
	return DrawAmplifyTextToBufferCommon_ARIB(eLang, pucBuf, iBufWidth, iBufHeight, iX, iY, pcText, uiFrontColor, 64, 64, FALSE);
//	return DrawAmplifyTextToBufferCommon_ARIB(eLang, pucBuf, iBufWidth, iBufHeight, iX, iY, pcText, uiFrontColor, iXTimes, iYTimes, FALSE);
}

static int32_t aribcc_osd_draw_data(GxAribCC_LangID langid, GxAribCC_RenderSize *render,int8_t *data, uint32_t len)
{
	int text_len, x1;
	RECT_ARIB rect;
	int iIndex=1;
	bool  bWrap=FALSE;
	int  iWrapIndex;
	static char  acLine1Buffer[256];
	static char  acLine2Buffer[256];
	int iLine2Begin=len;
	int iLine1Begin=0;

	printf("aribcc_osd_draw_data==len:%d\n",len);
	if (len == 0)
	{
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, 0);
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		ARIB_Flip();
		return 0;
	}
	else
	{
		memset(acLine1Buffer,0,256);
		memset(acLine2Buffer,0,256);
	}
	while(iLine1Begin<len&&(data[iLine1Begin]==0x20||(unsigned char)data[iLine1Begin]==0x90))
		iLine1Begin++;
	if(iLine1Begin>=len)
		return 0;
	for(iIndex=iLine1Begin;iIndex<len;iIndex++)
	{
		if((unsigned char)data[iIndex]==0x90)
		{
			bWrap = TRUE;
			iWrapIndex = iIndex;
			iLine2Begin = iIndex;
			iIndex++;
			for(;iIndex<len&&data[iIndex]==0x20;iIndex++);
			if(iIndex==len)
				bWrap=FALSE;
			else
				iLine2Begin = iIndex;
			break;
		}
	}
	if(bWrap)
	{
		memcpy(acLine1Buffer,data+iLine1Begin,iWrapIndex-iLine1Begin+1);
		acLine1Buffer[iWrapIndex-iLine1Begin]='\0';
		memcpy(acLine2Buffer,data+iLine2Begin,len-iLine2Begin);
		acLine2Buffer[len - iLine2Begin]='\0';
		for(iIndex=0;iIndex<len - iLine2Begin + 1;iIndex++)
		{
			if((unsigned char)acLine2Buffer[iIndex]==0x90)
				acLine2Buffer[iIndex]=0x20;
		}
	}
	else
	{
		int iTextLen;
		int iText3_1,iText3_2,iFindCutPoint=0;
		int iIndex;

		memcpy(acLine1Buffer,data+iLine1Begin,iLine2Begin-iLine1Begin);
		acLine1Buffer[iLine2Begin-iLine1Begin]='\0';
		iTextLen = strlen(acLine1Buffer);
		if(iTextLen > 45)  // text too long ,cut up   two text
		{
			bWrap = TRUE;
			iText3_1 = iTextLen/3;
			iText3_2 = (iTextLen/3)*2;
			for(iIndex = iText3_1;iIndex<iText3_2;iIndex++)
			{
				if( (acLine1Buffer[iIndex]>0x20 && acLine1Buffer[iIndex]<0x30)||(acLine1Buffer[iIndex]>0x39 && acLine1Buffer[iIndex]<0x40))
				{
					iFindCutPoint = iIndex;
					break;
				}
			}
			if(iIndex == iText3_2)
			{
				iFindCutPoint = iTextLen/2;
			}
			memcpy(acLine2Buffer,acLine1Buffer+iFindCutPoint+1,iTextLen - iFindCutPoint-1);
			acLine2Buffer[iTextLen - iFindCutPoint-1] = '\0';
			acLine1Buffer[iFindCutPoint+1]='\0';
		}
	}

	if(bWrap == FALSE)
	{
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, 0);
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine1Buffer), (const char*) acLine1Buffer, ConvertCCLang_ARIB(langid));
		x1 = 100;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0)
			rect.x1 = 0;
		rect.x2 = x1 + text_len + 4;
		if (rect.x2 >ARIB_SPP_WIN_W)
			rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1;
		rect.y2 = ARIB_RECT_Y2;
	#if OSD_720P
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#else
		//ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#endif
		//DrawAmplifyTextToBuffer_ARIB(NULL, GetSDOsdWidth(), GetSDOsdHeight(), x1, y1, acLine1Buffer, ARIB_TEXT_COLOR, ConvertCCLang_ARIB(langid), 1, 1);
	}
	else
	{
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine1Buffer), (const char*) acLine1Buffer, ConvertCCLang_ARIB(langid));
		x1 = 100;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0)
			rect.x1 = 0;
		rect.x2 = x1 + text_len + 4;
		if (rect.x2 >ARIB_SPP_WIN_W)
			rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1;
		rect.y2 = ARIB_RECT_Y2;
	#if OSD_720P
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#else
		//ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#endif
		//DrawAmplifyTextToBuffer_ARIB(NULL, GetSDOsdWidth(), GetSDOsdHeight(), x1, y1, acLine1Buffer, ARIB_TEXT_COLOR, ConvertCCLang_ARIB(langid), 1, 1);

		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine2Buffer), (const char*) acLine2Buffer, ConvertCCLang_ARIB(langid));
		x1 = 100;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0) rect.x1 = 0;
		rect.x2 = x1 + text_len + 4;
		if (rect.x2 >ARIB_SPP_WIN_W) rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1+ARIB_RECT_HEIGHT;
		rect.y2 = ARIB_RECT_Y2+ARIB_RECT_HEIGHT;
	#if OSD_720P
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#else
		//ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
	#endif
		//DrawAmplifyTextToBuffer_ARIB(NULL, GetSDOsdWidth(), GetSDOsdHeight(), x1, y1, acLine2Buffer, ARIB_TEXT_COLOR, ConvertCCLang_ARIB(langid), 1, 1);
	}
	ARIB_Flip();
	return 0;
}

static int32_t aribcc_osd_clear_data(GxAribCC_RenderSize *render)
{
	ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2, ARIB_RECT_Y2, 0);
	ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2, ARIB_RECT_Y2+ARIB_RECT_HEIGHT, 0);
	ARIB_Flip();
	return 0;
}

static int32_t aribcc_spp_draw_data(GxAribCC_LangID langid, GxAribCC_RenderSize *render,int8_t *data, uint32_t len)
{
	int text_len, x1, y1;
	RECT_ARIB rect;
	int iIndex=1;
	bool  bWrap=FALSE;
	int  iWrapIndex;
	int font_color = 0;
	static int bak_len = 0;
//	static int bak_len2 = 0;
	static char  acLine1Buffer[256];
	static char  acLine2Buffer[256];
	//static char  bakupbuf[256];
	//static char  bakupbuf2[256];
	int iLine2Begin=len;
	int iLine1Begin=0;
	int iSpaceLen=0;

	printf("aribcc_-_spp__data:%s  len=%d\n",data,len);

	if(render_size_refill == 0) {
		memcpy(&render_size,render,sizeof(GxAribCC_RenderSize));
		render_size_refill = 1;
	}

	if (len == 0)
	{
		ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT*3, 0);
		//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		ARIB_Flip();
		return 0;
	}
	else
	{
#ifdef ARIBCC_SPP_USBER_GUI_MODE
		x1 = 200;
		y1 = ARIB_RECT_Y1 + ARIB_RECT_HEIGHT;
		iLine1Begin = strlen((char*)data);
		if((iLine1Begin > 256)||(iLine1Begin < 0))
		{
			iLine1Begin = 250;
		}
		if(acLine1Buffer!=NULL)
		{
			if(memcmp(acLine1Buffer,data,iLine1Begin)==0)
			{
				printf("__not display__data:%s--len=%d\n",data,len);
				return 0;
			}
		}
		//aribcc_spp_draw_string(data,x1,y1,0,ARIB_TEXT_COLOR,GDI_STRING_PARAGRAPH);
		memset(acLine1Buffer,0,256);
		memcpy(acLine1Buffer,data,iLine1Begin);
		iLine1Begin = 0;
		//return 0;
#else
		memset(acLine1Buffer,0,256);
		memset(acLine2Buffer,0,256);
#endif
	}
	while(iLine1Begin<len&&(data[iLine1Begin]==0x20||(unsigned char)data[iLine1Begin]==0x90))
		iLine1Begin++;
	if(iLine1Begin>=len)
		return 0;
	for(iIndex=iLine1Begin;iIndex<len;iIndex++)
	{
		if((unsigned char)data[iIndex]==0x90)
		{
			bWrap = TRUE;
			iWrapIndex = iIndex;
			iLine2Begin = iIndex;
			iIndex++;
			for(;iIndex<len&&data[iIndex]==0x20;iIndex++);
			if(iIndex==len)
				bWrap=FALSE;
			else
				iLine2Begin = iIndex;
			break;
		}
	}
	if(bWrap)
	{
		memcpy(acLine1Buffer,data+iLine1Begin,iWrapIndex-iLine1Begin+1);
		acLine1Buffer[iWrapIndex-iLine1Begin]='\0';
		memcpy(acLine2Buffer,data+iLine2Begin,len-iLine2Begin);
		acLine2Buffer[len - iLine2Begin]='\0';
		for(iIndex=0;iIndex<len - iLine2Begin + 1;iIndex++)
		{
			if((unsigned char)acLine2Buffer[iIndex]==0x90)
			{
				acLine2Buffer[iIndex]=0x20;
				iSpaceLen++;
			}
		}
	}
	else
	{
		int iTextLen;
		int iText3_1,iText3_2,iFindCutPoint=0;
		int iIndex;

		iSpaceLen = 0;
		memcpy(acLine1Buffer,data+iLine1Begin,iLine2Begin-iLine1Begin);
		acLine1Buffer[iLine2Begin-iLine1Begin]='\0';
		iTextLen = strlen(acLine1Buffer);
		if(iTextLen > 45)  // text too long ,cut up   two text
		{
			bWrap = TRUE;
			iText3_1 = iTextLen/3;
			iText3_2 = (iTextLen/3)*2;
			for(iIndex = iText3_1;iIndex<iText3_2;iIndex++)
			{
				if( (acLine1Buffer[iIndex]>0x20 && acLine1Buffer[iIndex]<0x30)||(acLine1Buffer[iIndex]>0x39 && acLine1Buffer[iIndex]<0x40))
				{
					iFindCutPoint = iIndex;
					break;
				}
			}
			if(iIndex == iText3_2)
			{
				iFindCutPoint = iTextLen/2;
				while(acLine1Buffer[iFindCutPoint] != 0x20)
				{
					if(acLine1Buffer[iFindCutPoint] == '\0')
					{
						break;
					}
					iFindCutPoint++;
				}
			}
			if(iFindCutPoint < iTextLen)
			{
				memcpy(acLine2Buffer,acLine1Buffer+iFindCutPoint+1,iTextLen - iFindCutPoint-1);
				acLine2Buffer[iTextLen - iFindCutPoint-1] = '\0';
				acLine1Buffer[iFindCutPoint+1]='\0';
			}
		}
	}

	if(bWrap == FALSE)
	{
		//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, 0);
		//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine1Buffer), (const char*) acLine1Buffer, ConvertCCLang_ARIB(langid));
		x1 = 200;
		y1 = ARIB_RECT_Y1 + 6;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0)
			rect.x1 = 0;
		rect.x2 = x1 + text_len + 280;
		if (rect.x2 >ARIB_SPP_WIN_W)
			rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1;
		rect.y2 = ARIB_RECT_Y2;
		font_color = ARIB_TEXT_COLOR;
		//ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);
		//
		#ifdef ARIBCC_SPP_USBER_GUI_MODE
		if(bak_len != text_len)
		{
			font_color=0xffff1220;//ARIB_FILL_COLOR;
			aribcc_clean_fill_rect(x1, y1-6, rect.x2-rect.x1,
					rect.y2-rect.y1,render_size.surface,font_color);
		}
		#else
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
		#endif

		bak_len = text_len;
		DrawAmplifyTextToBuffer_ARIB(NULL, render_size.width, render_size.height, x1, y1, acLine1Buffer,ARIB_TEXT_COLOR , ConvertCCLang_ARIB(langid), 1, 1);
	}
	else
	{
		//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine1Buffer), (const char*) acLine1Buffer, ConvertCCLang_ARIB(langid));
		x1 = 200;
		y1 = ARIB_RECT_Y1 + 6;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0)
			rect.x1 = 0;
		rect.x2 = x1 + text_len + 280;
		if (rect.x2 >ARIB_SPP_WIN_W)
			rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1;
		rect.y2 = ARIB_RECT_Y2;
		//--ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);

		#ifdef ARIBCC_SPP_USBER_GUI_MODE
		if(bak_len != text_len)
		{
			font_color=0xffff1220;//ARIB_FILL_COLOR;
			aribcc_clean_fill_rect(x1, y1-6, rect.x2-rect.x1,
					ARIB_RECT_HEIGHT*2,render_size.surface,font_color);
		}
		#else
		ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
		#endif
		bak_len = text_len;
		DrawAmplifyTextToBuffer_ARIB(NULL, render_size.width, render_size.height, x1, y1, acLine1Buffer, ARIB_TEXT_COLOR, ConvertCCLang_ARIB(langid), 1, 1);

		//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2-ARIB_RECT_X1, ARIB_RECT_HEIGHT+ARIB_RECT_HEIGHT, 0);
		text_len = GetTextWidthByLang(sizeof(acLine2Buffer), (const char*) acLine2Buffer, ConvertCCLang_ARIB(langid));
		x1 = 200;
		y1 = ARIB_RECT_Y1 + ARIB_RECT_HEIGHT;

		if (x1+text_len > ARIB_RECT_X2)
		{
			x1 = (ARIB_RECT_X1 + ARIB_RECT_X2 - text_len) / 2;
		}

		rect.x1 = x1 - 4;
		if (rect.x1 < 0) rect.x1 = 0;
		rect.x2 = x1 + text_len + 4;
		if (rect.x2 >ARIB_SPP_WIN_W) rect.x2 = ARIB_SPP_WIN_W;
		rect.y1 = ARIB_RECT_Y1+ARIB_RECT_HEIGHT;
		rect.y2 = ARIB_RECT_Y2+ARIB_RECT_HEIGHT;
		//ARIB_FillRect(rect.x1, rect.y1, (rect.x2-rect.x1)*1.2, rect.y2-rect.y1, ARIB_FILL_COLOR);
		//ARIB_FillRect(rect.x1, rect.y1, rect.x2-rect.x1, rect.y2-rect.y1, ARIB_FILL_COLOR);
		//iLine2Begin = strlen(acLine2Buffer);
		font_color = 0;
		iSpaceLen = 0;
		for(iIndex=0;iIndex<12;iIndex++)
		{
			if((unsigned char)acLine2Buffer[iIndex]==0x20)
			{
				iSpaceLen++;
			}
		}
		if(iSpaceLen<12)
		{
		DrawAmplifyTextToBuffer_ARIB(NULL, render_size.width, render_size.height, x1, y1, acLine2Buffer, ARIB_TEXT_COLOR, ConvertCCLang_ARIB(langid), 1, 1);
		}
	}
	ARIB_Flip();
	return 0;
}

static int32_t aribcc_spp_clear_data(GxAribCC_RenderSize *render)
{
	if(render_size_refill == 0) {
		memcpy(&render_size,render,sizeof(GxAribCC_RenderSize));
		render_size_refill = 1;
	}

	ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2, VIDEO_WINDOW_H-ARIB_RECT_Y1, ARIB_BLACK_COLOR);
	//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1, ARIB_RECT_X2, ARIB_RECT_Y2, 0);
	//ARIB_FillRect(ARIB_RECT_X1, ARIB_RECT_Y1+ARIB_RECT_HEIGHT, ARIB_RECT_X2, ARIB_RECT_Y2+ARIB_RECT_HEIGHT, 0);
	ARIB_Flip();
	return 0;
}

static int aribcc_sub_start(int iDemux,unsigned int iPID,int iCompositionPageID,int iAncillaryPageID)
{
	GxSubtitle_Param param;
	GxSubtitle_Stream stream;
	GxAribCC_Param ccparam;
	GxAribCC_Stream ccstream;
	//int byteperline = 2;
	int ret = 0;

	int dmxno = iDemux;
	if(dmxno != 0 && dmxno != 1)
		return GXCORE_ERROR;

	if(iCompositionPageID == 0x0008 && iAncillaryPageID == 0xffff)
	{
		printf(" aribcc Sub_Start\n");
		memset(&render_size,0,sizeof(GxAribCC_RenderSize));

		ccparam.demux_id = dmxno;
		ccparam.render_type = GXARIBCC_SPP;
		ccparam.render_size.width = ARIB_SPP_WIN_W;//720;
		ccparam.render_size.height= 576;
		if(ccparam.render_type == GXARIBCC_OSD)
		{
			ccparam.render_size.surface= NULL;
			ccparam.render_size.buffer = NULL;
			ccparam.render_cb.clean_data = aribcc_osd_clear_data;
			ccparam.render_cb.draw_data = aribcc_osd_draw_data;

			memset(&cc_surface,0,sizeof(GxVpuProperty_CreateSurface));
		}
		else
		{
			ccparam.render_size.surface= NULL;
			ccparam.render_size.buffer = NULL;
			ccparam.render_cb.clean_data = aribcc_spp_clear_data;
			ccparam.render_cb.draw_data = aribcc_spp_draw_data;

			thiz_cc_dev_hdl    = GxAvdev_CreateDevice(0);
			thiz_cc_spp_hdl    = GxAvdev_OpenModule(thiz_cc_dev_hdl, GXAV_MOD_VPU, 0);

			memset(&cc_surface,0,sizeof(GxVpuProperty_CreateSurface));
			#ifdef ARIBCC_SPP_USBER_GUI_MODE
			cc_surface.format = GX_COLOR_FMT_YCBCRA6442;//GX_COLOR_FMT_ARGB8888;
			#else
			cc_surface.format = GX_COLOR_FMT_ARGB8888;
			#endif
			cc_surface.width	= ARIB_SPP_WIN_W;//720
			cc_surface.height	= 576;
			cc_surface.mode   = GX_SURFACE_MODE_IMAGE;
			cc_surface.buffer    = NULL;
			#ifdef ARIBCC_SPP_USBER_GUI_MODE
			cc_spp_draw_init();
			#else
			ret = GxAVGetProperty(thiz_cc_dev_hdl,
					thiz_cc_spp_hdl,
					GxVpuPropertyID_CreateSurface,
					&cc_surface,
					sizeof(GxVpuProperty_CreateSurface));
			if(ret != 0)
				printf("GxVpuPropertyID_CreateSurface fail\n");
			#endif

			CC_MmapBuffer = cc_surface.buffer;
		}
		render_type = ccparam.render_type;
		render_size_refill = 0;

		if(cc_t == 0)
		{
			cc_t = GxAribCC_Open(&ccparam);
			aribcc_spp_clut_init();
			ccstream.pid = iPID;
			ccstream.comp_page_id = iCompositionPageID;
			ccstream.anci_page_id   = iAncillaryPageID;

			ret = GxAribCC_StreamSet(cc_t, &ccstream);
		}
	}
	else
	{
		param.render_type = GXSUBTITLE_SPP;
		param.render_rect.width = 720;
		param.render_rect.height = 576;

		if(subtitle_t == 0)
		{
			subtitle_t = GxSubtitle_Open(&param);

			stream.pid = iPID;
			stream.comp_page_id = iCompositionPageID;
			stream.anci_page_id   = iAncillaryPageID;

			ret = GxSubtitle_StreamSet(subtitle_t, &stream);
		}
	}
	return ret;
}

int app_aribcc_sub_start(int demux,unsigned int pid,int composition,int ancillary)
{
	int composition_page_id = 0;
	int ancillary_page_id = 0;
	int ret = 0;

	composition_page_id = 0x0008;
	ancillary_page_id = 0xffff;
	ret = aribcc_sub_start(demux,pid,composition_page_id,ancillary_page_id);

	return ret;
}

void app_aribcc_sub_stop(void)
{
	if(subtitle_t)
	{
		GxSubtitle_Stop(subtitle_t,0);
		GxSubtitle_Close(subtitle_t);
		subtitle_t = 0;
	}

	if(cc_t)
	{
		GxVpuProperty_DestroySurface    p;

		GxAribCC_Stop(cc_t);
		GxAribCC_Close(cc_t);
		cc_t = 0;

		memset(&render_size,0,sizeof(GxAribCC_RenderSize));
		render_type = 0;
		render_size_refill = 0;
		if(cc_surface.surface) {
			p.surface = cc_surface.surface;
			GxAVSetProperty(thiz_cc_dev_hdl, thiz_cc_spp_hdl,
				GxVpuPropertyID_DestroySurface,
				&p,
				sizeof(GxVpuProperty_DestroySurface));

			GxAvdev_CloseModule(thiz_cc_dev_hdl, thiz_cc_spp_hdl);
			GxAvdev_DestroyDevice(thiz_cc_dev_hdl);
		}
	}
}

void app_aribcc_sub_display(bool bDisplay )
{
	if(subtitle_t) {
		if(bDisplay == TRUE)
			GxSubtitle_Show(subtitle_t);
		else
			GxSubtitle_Hide(subtitle_t);
	}

	if(cc_t) {
		printf("Play cc\n");
		if(bDisplay == TRUE)
			GxAribCC_Show(cc_t);
		else
			GxAribCC_Hide(cc_t);
	}
}

bool app_aribcc_sub_running(void)
{
	return subtitle_t;
}

bool app_aribcc_is_running(void)
{
	return cc_t;
}

int app_aribcc_switch_lang(int lang_id)
{
	GxAribCC_LangID langid = 0;
	int ret = 0;

	langid = lang_id;

	ret =GxAribCC_SwitchLang(cc_t,langid);

	return ret;
}

char *app_aribcc_get_caption_lang(int index)
{
	extern char *arib_set_langid(int langid);
	char *str="Pol";
	if (cc_t == E_INVALID_HANDLE)
		return str;
	return arib_set_langid(index);
}
#endif
