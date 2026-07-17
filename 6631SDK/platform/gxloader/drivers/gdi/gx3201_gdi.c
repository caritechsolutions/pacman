#include "../jpeg/gx3201_jpeg.h"
#include "stdio.h"
#include "io.h"
#include "string.h"
#include "time.h"
#include "cpu_config.h"
#include "gx_api.h"
#include "gx3201_gdi.h"
#include "gdi.h"
#include "../vout/gx3201_vout.h"
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#include "gxav_common.h"
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

#if(CONFIG_GDI_BPP == 8)
	#define COLOR_FMT_SELECT	GX_COLOR_FMT_CLUT8
#elif(CONFIG_GDI_BPP == 16)
	#define COLOR_FMT_SELECT	GX_COLOR_FMT_RGB565
#elif(CONFIG_GDI_BPP == 32)
	#define COLOR_FMT_SELECT	GX_COLOR_FMT_ARGB8888
#else
	#define COLOR_FMT_SELECT	GX_COLOR_FMT_RGB565
#endif
#define PALETTE_BUF_SIZE	(256*4)

typedef struct osd_region_s{
	unsigned char *osd_uncache_buf;
	unsigned int w;
	unsigned int h;
	unsigned int bpp;
	unsigned int *palette_buf;
	unsigned int act_palette_count;
}osd_region_t;

extern enum cvbs_mode_enum g_cvbs_mode;
extern enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode;
#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
extern volatile CygnusOsdReg *cygnusosd_reg;
#else
extern volatile Gx3201VpuReg *gx3201vpu_reg;
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
extern unsigned int g_hdmi_output_w;
extern unsigned int g_hdmi_output_h;

static u32 byte_seq;
static OsdRegionHead *gx3201osd_head_ptr;
static osd_region_t g_osd_region;

static int gxcolor_bpp[] = {
	1,  /* GX_COLOR_FMT_CLUT1           */
	2,  /* GX_COLOR_FMT_CLUT2           */
	4,  /* GX_COLOR_FMT_CLUT4           */
	8,  /* GX_COLOR_FMT_CLUT8           */
	16, /* GX_COLOR_FMT_RGBA4444        */
	16, /* GX_COLOR_FMT_RGBA5551        */
	16, /* GX_COLOR_FMT_RGB565          */
	32, /* GX_COLOR_FMT_RGBA8888        */
	24, /* GX_COLOR_FMT_RGB888          */
	24, /* GX_COLOR_FMT_BGR888          */

	16, /* GX_COLOR_FMT_ARGB4444        */
	16, /* GX_COLOR_FMT_ARGB1555        */
	32, /* GX_COLOR_FMT_ARGB8888        */
};

static unsigned int gx_cache2uncache_addr(unsigned int cache_addr)
{
#ifdef CONFIG_ARCH_ARM
	return (cache_addr);
#else
	return (cache_addr - 0x80000000 + 0xa0000000);
#endif
}

static unsigned int gx_cache2phys_addr(unsigned int cache_addr)
{
#ifdef CONFIG_ARCH_ARM
	return (cache_addr);
#else
	return (cache_addr - 0x80000000);
#endif
}

static unsigned int gx_uncache2phys_addr(unsigned int uncache_addr)
{
#ifdef CONFIG_ARCH_ARM
	return (uncache_addr);
#else
	return (uncache_addr - 0xa0000000);
#endif
}

static unsigned char get_bit_value(unsigned char input, unsigned char bit)
{
	return ((input >> bit) & 0x1);
}

static int gx_color_get_bpp(GxColorFormat format)
{
	if(format < sizeof(gxcolor_bpp)/sizeof(int))
		return gxcolor_bpp[format];
	else
		return -1;
}

static int gx3201osd_color_format(GxColorFormat format)
{
	int true_color_mode;

	if(format <= GX_COLOR_FMT_CLUT8)
#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
		VPU_OSD_CLR_ZOOM_MODE_EN_IPS(cygnusosd_reg->rOSD_CTRL);
#else
		VPU_OSD_CLR_ZOOM_MODE_EN_IPS(gx3201vpu_reg->rOSD_CTRL);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
	else
#if defined(CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
		VPU_OSD_SET_ZOOM_MODE_EN_IPS(cygnusosd_reg->rOSD_CTRL);
#else
		VPU_OSD_SET_ZOOM_MODE_EN_IPS(gx3201vpu_reg->rOSD_CTRL);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/

	if((format >= GX_COLOR_FMT_RGBA8888)&&(format <= GX_COLOR_FMT_BGR888)) {
		true_color_mode = format - GX_COLOR_FMT_RGBA8888;
		VPU_OSD_SET_COLOR_TYPE(gx3201osd_head_ptr->word1, 0x7);
		VPU_OSD_SET_TRUE_COLOR_MODE(gx3201osd_head_ptr->word1, true_color_mode);
	}else{
		if( (format==GX_COLOR_FMT_ARGB4444)||(format==GX_COLOR_FMT_ARGB1555)||
			(format==GX_COLOR_FMT_ARGB8888))
			VPU_OSD_SET_ARGB_CONVERT(gx3201osd_head_ptr->word1,1);
	        else
			VPU_OSD_SET_ARGB_CONVERT(gx3201osd_head_ptr->word1,0);

		switch(format)
		{
			case GX_COLOR_FMT_ARGB4444:
				format = GX_COLOR_FMT_RGBA4444;
				break;
			case GX_COLOR_FMT_ARGB1555:
				format = GX_COLOR_FMT_RGBA5551;
				break;
			case GX_COLOR_FMT_ARGB8888:
				format = GX_COLOR_FMT_RGBA8888;
				break;
			default:
				break;
		}
		VPU_OSD_SET_COLOR_TYPE(gx3201osd_head_ptr->word1, format);
	}

	return 0;
}

static int gx3201osd_set_pixel_alpha_mode(void)
{
	VPU_OSD_GLOBAL_ALHPA_DISABLE(gx3201osd_head_ptr->word1);
	VPU_OSD_SET_ALPHA_RATIO_DISABLE(gx3201osd_head_ptr->word7);
	return 0;
}

static int gx3201osd_palette(GxColorFormat format, unsigned int *palette, u32 palette_num)
{
	switch(format){
		case GX_COLOR_FMT_CLUT1:
			VPU_OSD_SET_CLUT_LENGTH(gx3201osd_head_ptr->word1, 3);;
			break;
		case GX_COLOR_FMT_CLUT2:
			VPU_OSD_SET_CLUT_LENGTH(gx3201osd_head_ptr->word1, 2);;
			break;
		case GX_COLOR_FMT_CLUT4:
			VPU_OSD_SET_CLUT_LENGTH(gx3201osd_head_ptr->word1, 1);;
			break;
		case GX_COLOR_FMT_CLUT8:
			VPU_OSD_SET_CLUT_LENGTH(gx3201osd_head_ptr->word1, 0);;
			break;
		default:
			return -1;
	}
	g_osd_region.palette_buf = (unsigned int *)gx_cache2uncache_addr((unsigned int)page_malloc(PALETTE_BUF_SIZE));
	memcpy(g_osd_region.palette_buf, palette, palette_num * sizeof(unsigned int));
	g_osd_region.act_palette_count = palette_num;
	VPU_OSD_SET_CLUT_PTR(gx3201osd_head_ptr->word2, (unsigned int)g_osd_region.palette_buf);
	VPU_OSD_CLUT_UPDATA_ENABLE(gx3201osd_head_ptr->word1);
	return 0;
}

static int get_index_by_rgba(unsigned int rgba, unsigned int *p_index)
{
	int i, r = 0, g = 0, b = 0, a = 0;

	if(g_osd_region.bpp == 8) {
		for(i = 0; i < g_osd_region.act_palette_count; i++){
			if(g_osd_region.palette_buf[i] == rgba){
				*p_index = i;
				return 0;
			}
		}
		printf("error: can't find rgba=0x%x from palette.\n", rgba);
	} else if(g_osd_region.bpp == 16) {
		r = (rgba & 0x000000FF) >> 3;
		g = ((rgba >> 8) & 0x000000FF) >> 2;
		b = ((rgba >> 16) & 0x000000FF) >> 3;
		*p_index = (r << 11) | (g << 5) | b;
		return 0;
	} else if(g_osd_region.bpp == 32) {
		r = rgba & 0x000000FF;
		g = (rgba >> 8) & 0x000000FF;
		b = (rgba >> 16) & 0x000000FF;
		a = (rgba >> 24) & 0x000000FF;
		*p_index = (a << 24) | (r << 16) | (g << 8) | b;
		return 0;
	}
	return -1;
}

static int gx3201osd_clear_buf(void)
{
	memset(g_osd_region.osd_uncache_buf, 0, g_osd_region.w * g_osd_region.h * g_osd_region.bpp / 8);
	return 0;
}

static int gx3201osd_main_surface(u32 width, u32 height, unsigned int *palette, u32 palette_num)
{
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
	int bpp = gx_color_get_bpp(COLOR_FMT_SELECT);
	unsigned int osd_buf_addr = 0;

	vpu_osd_set_pos(GX_LAYER_OSD, 0, 0);
	vpu_osd_set_clip_size(GX_LAYER_OSD, width, height);
	vpu_osd_set_size(GX_LAYER_OSD, width, height, width, COLOR_FMT_SELECT);
	osd_buf_addr = (unsigned int)page_malloc(width * height * bpp / 8);
	g_osd_region.osd_uncache_buf = (unsigned char *)osd_buf_addr;
	g_osd_region.w = width;
	g_osd_region.h = height;
        g_osd_region.bpp = gx_color_get_bpp(COLOR_FMT_SELECT);
	vpu_osd_set_addr(GX_LAYER_OSD, osd_buf_addr, 0, bpp);
#else   /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
	gdi_rect_t rect={0,0,0,0};
	unsigned int request_block=0;
	unsigned int osd_buf_addr;

	gx3201osd_head_ptr = (OsdRegionHead *)gx_cache2uncache_addr((unsigned int)page_malloc(sizeof(OsdRegionHead)));
	memset(gx3201osd_head_ptr, 0, sizeof(OsdRegionHead));
	rect.w = g_osd_region.w = width;
	rect.h = g_osd_region.h = height;
        g_osd_region.bpp = gx_color_get_bpp(COLOR_FMT_SELECT);
	request_block = rect.w*(g_osd_region.bpp>>3)/4/128*128;
	if(request_block < 128) {
		request_block = 128;
	}
	else if(request_block > 896) {
		request_block = 896;
	}

	/* set vpu byte-sequence(both data and command) */
	if(g_osd_region.bpp == 32)
		byte_seq = ABCD_EFGH;
	if(g_osd_region.bpp == 16)
		byte_seq = CDAB_GHEF;
	if(g_osd_region.bpp == 8)
		byte_seq = DCBA_HGFE;

#if defined (CONFIG_ARCH_CKMMU_TAURUS_REV) || defined (CONFIG_ARCH_ARM_SIRIUS_REV) || defined (CONFIG_ARCH_CKMMU_GEMINI_REV)
	VPU_SET_RW_BYTESEQ(gx3201vpu_reg->rOSD_PARA, byte_seq);
	byte_seq = ABCD_EFGH;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(gx3201vpu_reg->rOSD_PARA, byte_seq);
	REG_SET_FIELD(&(gx3201vpu_reg->rBUFF_CTRL2),0x7FF<<0,request_block,0);
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	VPU_SET_RW_BYTESEQ(cygnusosd_reg->rOSD_FRAME_PARA, byte_seq);
	byte_seq = ABCD_EFGH;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(cygnusosd_reg->rOSD_FRAME_PARA, byte_seq);
	REG_SET_FIELD(&(cygnusosd_reg->rOSD_FRAME_PARA), 0x7ff, request_block, 0);
#else
	VPU_SET_RW_BYTESEQ(gx3201vpu_reg->rSYS_PARA, byte_seq);
	byte_seq = ABCD_EFGH;
	VPU_OSD_SET_REGIONHEAD_BYTESEQ(gx3201vpu_reg->rSYS_PARA, byte_seq);
	REG_SET_FIELD(&(gx3201vpu_reg->rBUFF_CTRL2),0x7FF<<0,request_block,0);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/

	VPU_OSD_SET_WIDTH(gx3201osd_head_ptr->word3, rect.x, rect.w + rect.x - 1);
	VPU_OSD_SET_HEIGHT(gx3201osd_head_ptr->word4, rect.y, rect.h + rect.y - 1);
#if defined (CONFIG_ARCH_CKMMU_CYGNUS)  || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	VPU_OSD_SET_POSITION(cygnusosd_reg->rOSD_POSITION, rect.x, rect.y);
#else
	VPU_OSD_SET_POSITION(gx3201vpu_reg->rOSD_POSITION, rect.x, rect.y);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/

	//gx3201osd_alpha(0xff);
	gx3201osd_set_pixel_alpha_mode();
        gx3201osd_color_format(COLOR_FMT_SELECT);
	//gx3201osd_color_key(&surface->color_key, surface->color_key_en);
	if(IS_REFERENCE_COLOR(COLOR_FMT_SELECT)) {
		gx3201osd_palette(COLOR_FMT_SELECT, palette, palette_num);
	}

	osd_buf_addr = (unsigned int)page_malloc(rect.w * rect.h * g_osd_region.bpp / 8);
	g_osd_region.osd_uncache_buf = (unsigned char *)gx_cache2uncache_addr((unsigned int)osd_buf_addr);
	VPU_OSD_SET_DATA_ADDR(gx3201osd_head_ptr->word5, (unsigned int)g_osd_region.osd_uncache_buf);
	VPU_OSD_SET_DATA_ADDR(gx3201osd_head_ptr->word6, 0);

#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
	VPU_OSD_SET_FIRST_HEAD(cygnusosd_reg->rOSD_FIRST_HEAD_PTR, gx_uncache2phys_addr((unsigned int)gx3201osd_head_ptr));
#else
	VPU_OSD_SET_FIRST_HEAD(gx3201vpu_reg->rOSD_FIRST_HEAD_PTR, gx_uncache2phys_addr((unsigned int)gx3201osd_head_ptr));
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
	VPU_OSD_LIST_END_ENABLE(gx3201osd_head_ptr->word7);
	VPU_OSD_SET_BASE_LINE(gx3201osd_head_ptr->word7,rect.w);

#ifdef CONFIG_ARCH_CKMMU_GEMINI
	VPU_OSD_SET_COLOR_KEY(gx3201vpu_reg->rOSD_COLOR_KEY, 0x12, 0x34, 0x56, 0x17);
#endif

#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
#else
	if(VPU_OSD_PHASE_0_ANTI_FLICKER_DISABLE == VPU_OSD_GET_PHASE_0(gx3201vpu_reg->rOSD_PHASE_0))
		VPU_OSD_SET_ZOOM_MODE(gx3201vpu_reg->rOSD_CTRL);
	else
		VPU_OSD_CLR_ZOOM_MODE(gx3201vpu_reg->rOSD_CTRL);

        VPU_OSD_CLR_ZOOM_MODE(gx3201vpu_reg->rOSD_CTRL);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
#endif /* CONFIG_ARCH_CKMMU_SCORPIO_TAURUS */
	return 0;
}


int gdi_init(u32 width, u32 height, unsigned int *palette, u32 palette_num)
{
	static int gdi_init_flag = 0;
	if(gdi_init_flag == 0){
		vpu_svpu_hdmi_init();
		gx3201osd_main_surface(width, height, palette, palette_num);
		gx_layer_zoom(1, width, height, g_hdmi_output_w, g_hdmi_output_h, 0, 0);
		gx3201osd_clear_buf();
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		vpu_osd_set_enable(GX_LAYER_OSD, 1);
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined(CONFIG_ARCH_ARM_CANOPUS) || defined(CONFIG_ARCH_ARM_VEGA) || defined(CONFIG_ARCH_CKMMU_SCORPIO)
		VPU_OSD_ENABLE(cygnusosd_reg->rOSD_ENABLE);
#else
		VPU_OSD_ENABLE(gx3201vpu_reg->rOSD_CTRL);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
		gdi_init_flag = 1;
	}else{
		printf("warning: function %s has been called more than twice.\n", __func__);
	}

#ifdef CONFIG_ARCH_ARM
	flush_dcache_all();
#endif /*CONFIG_ARCH_ARM*/
	return 0;
}

#if 0
int gdi_draw_bitmap(gdi_rect_t rect, char *buf)
{
	printf("error: %s not support now.\n", __func__);
	return -1;
}
#endif

int gdi_draw_text(gdi_rect_t rect, u32 front_color, u32 back_color, const unsigned char *buf)
{
	int i, j, k;
	unsigned int front_color_index, back_color_index;
	unsigned char *p_src;
	unsigned char *p_dst8bpp;
	unsigned short *p_dst16bpp;
	unsigned int *p_dst32bpp;
	if((get_index_by_rgba(front_color, &front_color_index) ||
		get_index_by_rgba(back_color, &back_color_index)) == 0){
		p_src = (unsigned char *)buf;
		if(g_osd_region.bpp == 8)
			p_dst8bpp = g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8;
		else if(g_osd_region.bpp == 16)
			p_dst16bpp = (unsigned short *)(g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8);
		else if(g_osd_region.bpp == 32)
			p_dst32bpp = (unsigned int *)(g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8);
		k = 7;
		for(i = 0; i < rect.h; i++){
			for(j = 0; j < rect.w; j++){
				if(get_bit_value(*p_src, k) == 1){
					if(g_osd_region.bpp == 8)
						*p_dst8bpp++ = front_color_index;
					else if(g_osd_region.bpp == 16)
						*p_dst16bpp++ = front_color_index;
					else if(g_osd_region.bpp == 32)
						*p_dst32bpp++ = front_color_index;
				}else{
					if(g_osd_region.bpp == 8)
						*p_dst8bpp++ = back_color_index;
					else if(g_osd_region.bpp == 16)
						*p_dst16bpp++ = back_color_index;
					else if(g_osd_region.bpp == 32)
						*p_dst32bpp++ = back_color_index;
				}
				if(k == 0){
					k = 7;
					p_src++;
				}else{
					k--;
				}
			}
			if(g_osd_region.bpp == 8)
				p_dst8bpp += g_osd_region.w - rect.w;
			else if(g_osd_region.bpp == 16)
				p_dst16bpp += g_osd_region.w - rect.w;
			else if(g_osd_region.bpp == 32)
				p_dst32bpp += g_osd_region.w - rect.w;
		}
	}else{
		return -1;
	}
#ifdef CONFIG_ARCH_ARM
	flush_dcache_all();
#endif /*CONFIG_ARCH_ARM*/
	return 0;
}

int gdi_fill_rect(gdi_rect_t rect, u32 fill_color)
{
	int i, j;
	unsigned int index;
	unsigned char *p_dst8bpp;
	unsigned short *p_dst16bpp;
	unsigned int *p_dst32bpp;
	if(get_index_by_rgba(fill_color, &index) == 0){
		if(g_osd_region.bpp == 8)
			p_dst8bpp = g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8;
		else if(g_osd_region.bpp == 16)
			p_dst16bpp = (unsigned short *)(g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8);
		else if(g_osd_region.bpp == 32)
			p_dst32bpp = (unsigned int *)(g_osd_region.osd_uncache_buf + (rect.y * g_osd_region.w + rect.x) * g_osd_region.bpp / 8);
		for(i = 0; i < rect.h; i++){
			for(j = 0; j < rect.w; j++){
				if(g_osd_region.bpp == 8)
					*p_dst8bpp++ = index;
				else if(g_osd_region.bpp == 16)
					*p_dst16bpp++ = index;
				else if(g_osd_region.bpp == 32)
					*p_dst32bpp++ = index;
			}
			if(g_osd_region.bpp == 8)
				p_dst8bpp += g_osd_region.w - rect.w;
			else if(g_osd_region.bpp == 16)
				p_dst16bpp += g_osd_region.w - rect.w;
			else if(g_osd_region.bpp == 32)
				p_dst32bpp += g_osd_region.w - rect.w;
		}
	}else{
		return -1;
	}
#ifdef CONFIG_ARCH_ARM
	flush_dcache_all();
#endif /*CONFIG_ARCH_ARM*/
	return 0;
}

int gdi_draw_pixel(u32 x, u32 y, u8 color_index)
{
	unsigned char *p_dst;
	p_dst = g_osd_region.osd_uncache_buf + y * g_osd_region.w + x;
	*p_dst = color_index;
#ifdef CONFIG_ARCH_ARM
	flush_dcache_all();
#endif /*CONFIG_ARCH_ARM*/
	return 0;
}

int gdi_term(void)
{
	*(volatile u32*)(VPU_BASE_ADDR + 0x90) &= ~1;
	return 0;
}

