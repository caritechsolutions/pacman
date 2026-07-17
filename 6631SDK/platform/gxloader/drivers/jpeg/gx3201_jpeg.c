#include "gx3201_jpeg.h"
#include "stdio.h"
#include "io.h"
#include "string.h"
#include "time.h"
#include "cpu_config.h"
#include "gx_api.h"
#include "gdi.h"
#include "get_mem_info.h"
#include "../vout/gx3201_vout.h"
#include "jpeg_feature.h"
#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA)  || defined (CONFIG_ARCH_CKMMU_SCORPIO)
#include "../vout/cygnus_vpu_vout.h"
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO */

static DecoderParameter g_dec_par;
static DecoderParameter *p_dec_par = &g_dec_par;
extern unsigned int g_hdmi_output_w;
extern unsigned int g_hdmi_output_h;
#define CHECK_BUF_MALLOC(buf) {if (NULL == (void *)buf) printf("%d, buf alloc failed\n", __LINE__);}
#define CHECK_BUF(buf) \
	do { \
		if (!buf || !buf->y_buf || !buf->u_buf || !buf->v_buf) { \
			printf("%d, buf is null \n", __LINE__); \
			return; \
		} \
	} while(0)

static void WRITE_REG(u32 reg,u32 value,u32 width,u32 offset)
{
	unsigned int result;
	unsigned int original;
	unsigned int mask;
	unsigned int value_masked;

	original = *(volatile unsigned int *)reg;

	if(width==32)
	{
		mask = 0xffffffff;
	}
	else
	{
		mask = (1<<width)-1;
	}
	value_masked = value&mask;
	result = (original & (~(mask<<offset))) | (value_masked<<offset);

	*(volatile unsigned int *)reg = result;
}

static u32 READ_REG(u32 reg, u32 width, u32 offset)
{
	unsigned int result;
	unsigned int original;
	unsigned int mask;
	unsigned int mask_shifted;

	original = *(volatile unsigned int *)reg;
	if (width == 32)
		mask = 0xffffffff;
	else
		mask = (1 << width) - 1;

	mask_shifted = mask << offset;
	result = (original & mask_shifted) >> offset;

	return result;
}

static void gx_allocate_bs_buffer(DecoderParameter *p_dec_par)
{
	jpeg_set_bs_buf(p_dec_par->bs_buffer_start_addr, p_dec_par->bs_buffer_size);
	jpeg_set_almostempty_th(0);
	jpeg_set_wr_ptr(0);
	//printf("[JPEG] BS BUFFER ADDR = 0x%x  SIZE = 0x%x\n",p_dec_par->bs_buffer_start_addr,p_dec_par->bs_buffer_size);
}

static void gx_allocate_frame_buffer(DecoderParameter *p_dec_par)
{
	int real_fb_h = p_dec_par->frame_buffer_H;

	jpeg_set_framebuffer(p_dec_par->frame_buffer_Y_addr & 0xFFFFFFFF,
			     p_dec_par->frame_buffer_Cb_addr & 0xFFFFFFFF,
			     p_dec_par->frame_buffer_Cr_addr & 0xFFFFFFFF);
	jpeg_set_stride(p_dec_par->frame_buffer_stride);
	jpeg_set_max_size(p_dec_par->frame_buffer_W, real_fb_h);
	//printf("[JPEG] Y ADDR = 0x%x  Cb = 0x%x  Cr = 0x%x\n",p_dec_par->frame_buffer_Y_addr,p_dec_par->frame_buffer_Cb_addr,p_dec_par->frame_buffer_Cr_addr);
}

static int gx_jpeg_run(void)
{
	int i = 0, status = 0, start_ms = 0, end_ms = 0;

	for(i = 0; i < GX_JPEG_INT_MAX; i++) {
		jpeg_int_clr(i);
	}
	jpeg_set_resample_mode(GX_JPEG_RESAMPLE_ZOOM);     // config jpeg zoom mode
	dcache_flush();
	start_ms = gx_time_get_ms();
	jpeg_set_start(1);         // start jpeg decode
	while(1){
		status = jpeg_int_status();
		if(status & (1 << GX_JPEG_INT_DEC_PIC_FINISH)){	//decode finish
			jpeg_int_clr(GX_JPEG_INT_DEC_PIC_FINISH);
			printf("jpeg decode ok !\r\n");
			jpeg_set_start(0);      // stop jpeg decode
			return 0;
		}
		if(status & (1 << GX_JPEG_INT_FRAME_BUF_ERROR)){	//frame buf error
			jpeg_int_clr(GX_JPEG_INT_FRAME_BUF_ERROR);
			printf("error: jpeg frame buf error, will return.\n");
			return -1;
		}
		if(status & (1 << GX_JPEG_INT_BS_BUF_ERROR)){	//bs buf error
			jpeg_int_clr(GX_JPEG_INT_BS_BUF_ERROR);
			printf("error: jpeg bs buf error, will return.\n");
			return -1;
		}
		if(status & (1 << GX_JPEG_INT_BS_BUFFER_EMPTY)){
			jpeg_int_clr(GX_JPEG_INT_BS_BUFFER_EMPTY);
			jpeg_set_wr_ptr((p_dec_par->bs_buffer_size)&0x0FFFFFFF);
			end_ms = gx_time_get_ms();
		}
		if(status & (1 << GX_JPEG_INT_DEC_ERROR)){	//decode error
			jpeg_int_clr(GX_JPEG_INT_DEC_ERROR);
			printf("error: jpeg decode error, will return.\n");
			return -1;
		} else {
			end_ms = gx_time_get_ms();
		}
		if((end_ms - start_ms) >= 5000)
			return -1;

	}
}

static void gx_get_jpeg_info(DecoderParameter *p_dec_par, JpegOutBuf *out_buf)
{
	jpeg_get_wb_size(&(p_dec_par->jpeg_wb_width), &(p_dec_par->jpeg_wb_height));
	p_dec_par->jpeg_format = jpeg_get_format();
	jpeg_get_zoom(&(p_dec_par->jpeg_h_zoom), &(p_dec_par->jpeg_v_zoom));
	jpeg_get_display_size(&(p_dec_par->jpeg_disp_width), &(p_dec_par->jpeg_disp_height));
	p_dec_par->jpeg_disp_width  &= 0xfffffffffc;
	out_buf->w = p_dec_par->jpeg_disp_width;
	out_buf->h = p_dec_par->jpeg_disp_height;
	out_buf->stride = p_dec_par->frame_buffer_stride;
	//printf("jpeg_disp %d x %d\n",p_dec_par->jpeg_disp_width,p_dec_par->jpeg_disp_height);

	if((p_dec_par->jpeg_disp_width && p_dec_par->jpeg_disp_height) == 0)
		printf("%s jpeg_disp_width/jpeg_disp_height == 0, decode error, please check jpeg file.\n", __func__);
}

/* change format to 444 */
static int gx_jpeg_extend_cbcr_444(u8 *src_y, u8 *src_cb, u8 *src_cr,
		JpegOutBuf *out_buf, DecoderParameter *p_dec_par)
{
	u32 i,j;
	u8  *copy_y, *copy_cb, *copy_cr;
	u8	h_extend , v_extend;
	u8  *dest_y, *dest_cb, *dest_cr;
	u32 max_x,max_y;

	dest_y  = (u8 *)out_buf->y_buf;
	dest_cb = (u8 *)out_buf->u_buf;
	dest_cr = (u8 *)out_buf->v_buf;

	if((p_dec_par->jpeg_format & 0x2)  || (p_dec_par->jpeg_format & 0x4))  //4:2:2 or 4:4:4 or 4:0:0
	{
		h_extend = 0;
		max_x = p_dec_par->jpeg_disp_width;
	}
	else
	{
		max_x  = (p_dec_par->jpeg_disp_width) >>1;
		h_extend = 1;
	}


	if((p_dec_par->jpeg_format & 0x1)  || (p_dec_par->jpeg_format & 0x4))  //2:2:4 or 4:4:4 or 4:0:0
	{
		max_y = p_dec_par->jpeg_disp_height;
		v_extend = 0;
	}
	else
	{
		max_y = (p_dec_par->jpeg_disp_height) >> 1;
		v_extend = 1;
	}
	//printf("cbcr_width X cbcr_height = %d X %d h_extend = %d , v_extend = %d \n",p_dec_par->jpeg_disp_width,p_dec_par->jpeg_disp_height,h_extend,v_extend);

	copy_y = src_y;
	for(j = 0; j < out_buf->h; j++)
	{
		for(i = 0; i < out_buf->stride; i++)
			*dest_y++ = *copy_y++;
	}

	if(h_extend)
	{
		for(j= 0;j<max_y;j++)
		{
			copy_cb = dest_cb;
			copy_cr = dest_cr;
			for(i= 0;i<max_x;i++)
			{
				*dest_cr++  = *src_cr;
				*dest_cr++  = *src_cr++;

				*dest_cb++  = *src_cb;
				*dest_cb++  = *src_cb++;
			}

			if(v_extend)
			{
				for(i= 0;i<max_x*2;i++)
				{
					*dest_cr++  = *copy_cr++;
					*dest_cb++  = *copy_cb++;
				}
			}
			src_cb = src_cb + (out_buf->stride - max_x);
			src_cr = src_cr + (out_buf->stride - max_x);
		}
	}
	else
	{
		for(j= 0;j<max_y;j++)
		{
			copy_cb = dest_cb ;
			copy_cr = dest_cr ;
			for(i= 0;i<max_x;i++)
			{
				//4:0:0 fill 0x80 in cbcr
				if(p_dec_par->jpeg_format == 4)
				{
					//printf("[JPEG] white_and_black\n");
					*dest_cr++  = 0x80;

					*dest_cb++  = 0x80;
				}
				else
				{
					*dest_cr++  = *src_cr++;

					*dest_cb++  = *src_cb++;
				}
			}
			if(v_extend)
			{
				for(i= 0;i<max_x;i++)
				{
					*dest_cr++  = *copy_cr++;
					*dest_cb++  = *copy_cb++;
				}
			}
			src_cb = src_cb + (out_buf->stride - max_x);
			src_cr = src_cr + (out_buf->stride - max_x);
		}
	}
	dcache_flush();
	return 0;
}

/* change 444 to UYVY */
static int gx_jpeg_UYVY(JpegOutBuf *src, u8 *dest, u32 stride)
{
	u32	i,j;
	u8 tmp;
	u8 *src_y, *src_cb, *src_cr;

	src_y  = (u8 *)src->y_buf;
	src_cb = (u8 *)src->u_buf;
	src_cr = (u8 *)src->v_buf;
	for(j = 0; j < src->h; j++)
	{
		for(i = 0; i < (src->w>>1); i++)
		{
			tmp = *src_cb++;
			*dest++ = (tmp + *src_cb++)>>1;
			*dest++ = *src_y++;
			tmp = *src_cr++;
			*dest++ = (tmp + *src_cr++)>>1;
			*dest++ = *src_y++;
		}
		src_y += src->stride - src->w;
	}
    	dcache_flush();
	return 0;
}

#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
#include "gxav_vpu_propertytypes.h"
#endif /*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/

static int gx_jpeg_disp(int pp_frame_stride , int pp_addr , int source_height , int	jpg_mode)
{
	 if((jpg_mode == 0 )||(jpg_mode ==1))
	{
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		vpu_pic_set_size(pp_frame_stride, source_height, pp_frame_stride, GX_COLOR_FMT_YCBCR422);
		vpu_pic_set_addr(pp_addr, pp_addr + pp_frame_stride * source_height * 2, 16);
		vpu_pic_set_enable(1);
		return 0;
/*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
		*(volatile u32*)(PIC_BASE_ADDR + 0x28) = pp_addr ;	//JPG_Y_TOP_ADDR
		*(volatile u32*)(PIC_BASE_ADDR + 0x2c) = pp_addr + pp_frame_stride;	//JPG_Y_BOTTOM_ADDR
		*(volatile u32*)(PIC_BASE_ADDR + 0x30) = pp_addr + pp_frame_stride * source_height;	//JPG_UV_TOP_ADDR
		*(volatile u32*)(PIC_BASE_ADDR + 0x34) = pp_addr + pp_frame_stride * source_height + pp_frame_stride;	//JPG_UV_BOTTOM_ADDR
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) &= ~(0x1fff);	//JPG_PARA
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) |= pp_frame_stride;
#else /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
		*(volatile u32*)(VPU_BASE_ADDR + 0x68) = pp_addr ;	//JPG_Y_TOP_ADDR
		*(volatile u32*)(VPU_BASE_ADDR + 0x6c) = pp_addr + pp_frame_stride;	//JPG_Y_BOTTOM_ADDR
		*(volatile u32*)(VPU_BASE_ADDR + 0x70) = pp_addr + pp_frame_stride * source_height;	//JPG_UV_TOP_ADDR
		*(volatile u32*)(VPU_BASE_ADDR + 0x74) = pp_addr + pp_frame_stride * source_height + pp_frame_stride;	//JPG_UV_BOTTOM_ADDR
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) &= ~(0x1fff);	//JPG_PARA
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) |= pp_frame_stride;
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO */
	}
	else
	{
#if defined (CONFIG_ARCH_CKMMU_SCORPIO_TAURUS)
		vpu_pic_set_size(pp_frame_stride, source_height, pp_frame_stride, GX_COLOR_FMT_YCBCR422);
		vpu_pic_set_addr(pp_addr, pp_addr + pp_frame_stride * source_height * 2, 16);
		vpu_pic_set_enable(1);
		return 0;
/*CONFIG_ARCH_CKMMU_SCORPIO_TAURUS*/
#elif defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
		*(volatile u32*)(PIC_BASE_ADDR + 0x28) = pp_addr ;
		*(volatile u32*)(PIC_BASE_ADDR + 0x2c) = pp_addr + (pp_frame_stride << 1);
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) &= ~(0x1fff);
		*(volatile u32*)(PIC_BASE_ADDR + 0x18) |= pp_frame_stride;
#else /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO*/
		*(volatile u32*)(VPU_BASE_ADDR + 0x68) = pp_addr ;
		*(volatile u32*)(VPU_BASE_ADDR + 0x6c) = pp_addr + (pp_frame_stride << 1);
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) &= ~(0x1fff);
		*(volatile u32*)(VPU_BASE_ADDR + 0x58) |= (pp_frame_stride << 1);
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO */
	}
#if defined (CONFIG_ARCH_CKMMU_CYGNUS) || defined (CONFIG_ARCH_ARM_CANOPUS) || defined (CONFIG_ARCH_ARM_VEGA) || defined (CONFIG_ARCH_CKMMU_SCORPIO)
	REG_SET_FIELD(PIC_BASE_ADDR + 0x1c, 0x7 << 24, 0, 24);
	*(volatile u32*)(PIC_BASE_ADDR + 0x00) |= 1;	//JPG_CTRL
	*(volatile u32*)(PIC_BASE_ADDR + 0x18) &= ~(0x3<<30);
	*(volatile u32*)(PIC_BASE_ADDR + 0x18) |= jpg_mode<<30;
#else /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO */
	*(volatile u32*)(VPU_BASE_ADDR + 0x40) |= 1;	//JPG_CTRL
	*(volatile u32*)(VPU_BASE_ADDR + 0x58) &= ~(0x3<<30);
	*(volatile u32*)(VPU_BASE_ADDR + 0x58) |= jpg_mode<<30;
#endif /*CONFIG_ARCH_CKMMU_CYGNUS || CONFIG_ARCH_ARM_CANOPUS || CONFIG_ARCH_ARM_VEGA || CONFIG_ARCH_CKMMU_SCORPIO */
	return 0;
}

/* for decode another jpeg */
static int gx_jpeg_set_next_bs_buf(u8 *buf, u32 size)
{
	u32 offset, left;
	offset = jpeg_get_rd_ptr();

	if(offset)
	{
		left = p_dec_par->bs_buffer_size - offset;
		memcpy((u8 *)p_dec_par->bs_buffer_start_addr + offset, buf, left);
		return offset;
	}
	return 0;
}
static int show_jpeg_par_init(unsigned char *jpeg_buf, unsigned int jpeg_size,
		JpegOutBuf *out_buf, unsigned int output_w, unsigned int output_h)
{
	u32 size, real_size = 0;
	p_dec_par->frame_buffer_W       = output_w;
	p_dec_par->frame_buffer_H       = output_h;
	p_dec_par->frame_buffer_stride  = output_w;

	size = gx_jpeg_set_next_bs_buf(jpeg_buf, jpeg_size);
	if(!size)
	{
		p_dec_par->bs_buffer_size       = jpeg_size;
		p_dec_par->bs_buffer_start_addr = (u32)jpeg_buf;
	}

	unsigned long alloc_size = (LOGO_PIXEL_SIZE + 4096) / 4096 * 4096;
	if(output_w % 16) {
		output_w = (output_w + 16) / 16 * 16;
	}
	if(output_h % 16) {
		output_h = (output_h + 16) / 16 * 16;
	}
	real_size = (output_w * output_h + 4096) / 4096 * 4096;
	if(real_size > alloc_size) {
		alloc_size = real_size;
	}

	p_dec_par->frame_buffer_Y_addr  = (u32)malloc(alloc_size);    //buf size: 1 * alloc_size
	p_dec_par->frame_buffer_Cb_addr = (u32)malloc(alloc_size);    //buf size: 1 * alloc_size
	p_dec_par->frame_buffer_Cr_addr = (u32)malloc(alloc_size);    //buf size: 1 * alloc_size
	if((0 == p_dec_par->frame_buffer_Y_addr) ||
	   (0 == p_dec_par->frame_buffer_Cb_addr) ||
	   (0 == p_dec_par->frame_buffer_Cr_addr)) {
		printf("[logo] show_jpeg_par_init malloc failed\n");
		return -1;
	}
	p_dec_par->VPU_disp_UYVY_addr   = SPP_BUF_START_ADDR;         //buf size: 3 * alloc_size
	CHECK_BUF_MALLOC(p_dec_par->frame_buffer_Y_addr);
	CHECK_BUF_MALLOC(p_dec_par->frame_buffer_Cb_addr);
	CHECK_BUF_MALLOC(p_dec_par->frame_buffer_Cr_addr);

	return 0;
}

static void jpeg_dec_resources_release(DecoderParameter *p_dec_par)
{
	/* memory free */
	free((void *)p_dec_par->frame_buffer_Y_addr);
	free((void *)p_dec_par->frame_buffer_Cb_addr);
	free((void *)p_dec_par->frame_buffer_Cr_addr);
}

static void copy_src_2_dst(JpegOutBuf *src, JpegOutBuf *dst, int first)
{
	u32 copy_x, copy_y, i, j;
	u8 *src_y, *src_cb, *src_cr;
	u8 *dest_y, *dest_cb, *dest_cr;

	copy_x  = src->x - dst->x;
	copy_y  = src->y - dst->y;
	src_y   = (u8 *)src->y_buf;
	src_cb  = (u8 *)src->u_buf;
	src_cr  = (u8 *)src->v_buf;
	dest_y  = (u8 *)dst->y_buf;
	dest_cb = (u8 *)dst->u_buf;
	dest_cr = (u8 *)dst->v_buf;

	if((src->w + copy_x > dst->w) || (src->h + copy_y > dst->h))
	{
		printf("width or height is out of bounds\n");
		return;
	}
	if(first)
	{
		for(j = 0; j < copy_y; j++)
		{
			for(i = 0; i < dst->w; i++)
			{
				*dest_y++  = 0x80;
				*dest_cb++ = 0x80;
				*dest_cr++ = 0x80;
			}
			dest_y += dst->stride - dst->w;
		}
	}
	else
	{
		dest_y  += copy_y*dst->stride;
		dest_cb += copy_y*dst->w;
		dest_cr += copy_y*dst->w;
	}

	for(j = copy_y; j < src->h + copy_y; j++)
	{
		if(first)
		{
			for(i = 0; i < copy_x; i++)
			{
				*dest_y++  = 0x80;
				*dest_cb++ = 0x80;
				*dest_cr++ = 0x80;
			}
		}
		else
		{
			dest_y  += copy_x;
			dest_cb += copy_x;
			dest_cr += copy_x;
		}

		for(i = copy_x; i < src->w + copy_x; i++)
		{
			*dest_y++  = *src_y++;
			*dest_cb++ = *src_cb++;
			*dest_cr++ = *src_cr++;
		}

		if(first)
		{
			for(i = src->w + copy_x; i < dst->w; i++)
			{
				*dest_y++  = 0x80;
				*dest_cb++ = 0x80;
				*dest_cr++ = 0x80;
			}
		}
		else
		{
			dest_y  += dst->w - (src->w + copy_x);
			dest_cb += dst->w - (src->w + copy_x);
			dest_cr += dst->w - (src->w + copy_x);
		}
		src_y  += src->stride - src->w;
		dest_y += dst->stride - dst->w;
	}
	if(first)
	{
		for(j = src->h + copy_y; j < dst->h; j++)
		{
			for(i = 0; i < dst->w; i++)
			{
				*dest_y++  = 0x80;
				*dest_cb++ = 0x80;
				*dest_cr++ = 0x80;
			}
			dest_y += dst->stride - dst->w;
		}
	}
}

static void gx3201_jpeg_mix(JpegOutBuf *srcA, JpegOutBuf *srcB, JpegOutBuf *dst_buf)
{
	u32 buf_size;
	u32 dst_w, dst_h, dst_x, dst_y;
	dst_x = 0;
	dst_y = 0;
	dst_w = (((srcA->x + srcA->w) > (srcB->x + srcB->w)) ? (srcA->x + srcA->w) : (srcB->x + srcB->w));
	dst_h = (((srcA->y + srcA->h) > (srcB->y + srcB->h)) ? (srcA->y + srcA->h) : (srcB->y + srcB->h));
	buf_size = dst_w * dst_h;

	dst_buf->x      = dst_x;
	dst_buf->y      = dst_y;
	dst_buf->w      = dst_w;
	dst_buf->h      = dst_h;
	dst_buf->stride = srcA->stride;

	copy_src_2_dst(srcA, dst_buf, 1);
	copy_src_2_dst(srcB, dst_buf, 0);
}

static int gx3201_jpeg_decode(unsigned char *jpeg_buf, unsigned int jpeg_size, JpegOutBuf *out_buf)
{
	show_jpeg_par_init(jpeg_buf, jpeg_size, out_buf, g_hdmi_output_w, g_hdmi_output_h);
	gx_allocate_bs_buffer(p_dec_par);
	gx_allocate_frame_buffer(p_dec_par);
	if(gx_jpeg_run() == 0)
	{
		gx_get_jpeg_info(p_dec_par, out_buf);
		gx_jpeg_extend_cbcr_444((void *)p_dec_par->frame_buffer_Y_addr,
				(void *)p_dec_par->frame_buffer_Cb_addr,
				(void *)p_dec_par->frame_buffer_Cr_addr,
				out_buf, p_dec_par);
		goto decode_ok;
	} else {
		printf("decode error!\n");
	}

	jpeg_dec_resources_release(p_dec_par);
	return 1;

decode_ok:
	jpeg_dec_resources_release(p_dec_par);
	return 0;
}

static void gx3201_show_jpeg(JpegOutBuf *src)
{
	struct mem_info info = {0};

	info.type = SPP_BUF;
	get_mem_info(&info);
	if(src->w * src->h * 2 > info.len) {
		printf("[LOGO] width = %d, height = %d(%d x %d x 2 = %d) is more than SPP length(%d)!",
		       src->w, src->h, src->w, src->h, src->w * src->h * 2, info.len);
		while(1);
	}
	gx_jpeg_UYVY(src, (void *)p_dec_par->VPU_disp_UYVY_addr, src->stride);
	gx_layer_zoom( 0, src->w, src->h, p_dec_par->frame_buffer_W, p_dec_par->frame_buffer_H, 0, 0);
	gx_jpeg_disp(src->w, p_dec_par->VPU_disp_UYVY_addr, src->h,2);

#if defined(CONFIG_ARCH_CKMMU_GX3211) || defined(CONFIG_ARCH_CKMMU_GX6605S) || defined(CONFIG_ARCH_ARM_SIRIUS) || defined(CONFIG_ARCH_CKMMU_TAURUS) \
	|| defined(CONFIG_ARCH_CKMMU_GEMINI)
	gx3211_svpu_do_run();
#endif
}

static void platform_jpeg_decode(u8 *logo_buf, u32 logo_size, JpegOutBuf *buf)
{
	CHECK_BUF(buf);
	gx3201_jpeg_decode(logo_buf, logo_size, buf);
}

static void platform_jpeg_mix(JpegOutBuf *srcA, JpegOutBuf *srcB, JpegOutBuf *dst)
{
	CHECK_BUF(srcA);
	CHECK_BUF(srcB);
	CHECK_BUF(dst);
	if((srcA->x + srcA->w > srcA->stride) || (srcB->x + srcB->w > srcB->stride))
	{
		printf("x + w > stride\n");
		return;
	}

	return gx3201_jpeg_mix(srcA, srcB, dst);
}

static void platform_jpeg_show(JpegOutBuf *buf)
{
	CHECK_BUF(buf);
	printf("show jpeg logo ...\n");
	return gx3201_show_jpeg(buf);
}

void platform_show_logo(unsigned char *logo_buf, unsigned int logo_size)
{
	JpegOutBuf show_buf = {0};
	unsigned long alloc_size = (1920 * 1080 + 4096) / 4096 * 4096;

	show_buf.y_buf = (u32)malloc(alloc_size);
	show_buf.u_buf = (u32)malloc(alloc_size);
	show_buf.v_buf = (u32)malloc(alloc_size);
	if((0 == show_buf.y_buf) || (0 == show_buf.u_buf) || (0 == show_buf.v_buf)) {
		printf("[logo] platform_show_logo malloc failed\n");
		return;
	}

	vpu_svpu_hdmi_init();
	jpeg_install(JPEG_BASE_ADDR);
	platform_jpeg_decode(logo_buf, logo_size, &show_buf);
	platform_jpeg_show(&show_buf);

	free((void *)show_buf.y_buf);
	free((void *)show_buf.u_buf);
	free((void *)show_buf.v_buf);
}

