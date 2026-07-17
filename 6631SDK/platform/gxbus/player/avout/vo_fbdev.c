

#include "vo.h"
#include "../decoder/vd_mpeg_es.h"
#include "../include/gx_mem.h"


static char* fb_dev_name = NULL;	// such as /dev/fb0
static int fb_dev_fd;		// handle for fb_dev_name
static uint8_t* frame_buffer = NULL;	// mmap'd access to fbdev
static uint8_t* center = NULL;	// where to begin writing our image (centered?)
static struct fb_fix_screeninfo fb_finfo;	// fixed info
static struct fb_var_screeninfo fb_vinfo;	// variable info
static struct fb_var_screeninfo fb_orig_vinfo;	// variable info to restore later
static unsigned short fb_ored[256], fb_ogreen[256], fb_oblue[256];
static struct fb_cmap fb_oldcmap = { 0, 256, fb_ored, fb_ogreen, fb_oblue };

static int fb_cmap_changed = 0;	//  to restore map
static int fb_pixel_size;	// 32:  4  24:  3  16:  2  15:  2
static int fb_bpp;		// 32: 32  24: 24  16: 16  15: 15
static size_t fb_size;		// size of frame_buffer
static int fb_line_len;		// length of one line in bytes

static uint8_t* next_frame = NULL;	// for double buffering
static int in_width;
static int in_height;
static int out_width;
static int out_height;

extern int vo_config_count;
#if 1
uint8_t* table_rV[256];
uint8_t* table_gU[256];
int table_gV[256];
uint8_t* table_bU[256];

const int Inverse_Table[4] = { 104597, 132201, 25675, 53279 };

static int div_round(int dividend, int divisor)
{
	if (dividend > 0)
		return (dividend + (divisor >> 1)) / divisor;
	else
		return -((-dividend + (divisor >> 1)) / divisor);
}

#define isBGR(x)        (           \
           (x)==PIX_FMT_RGB32       \
        || (x)==PIX_FMT_BGR24       \
        || (x)==PIX_FMT_BGR565      \
        || (x)==PIX_FMT_BGR555      \
        || (x)==PIX_FMT_BGR8        \
        || (x)==PIX_FMT_BGR4        \
        || (x)==PIX_FMT_BGR4_BYTE   \
        || (x)==PIX_FMT_MONOBLACK   \
    )

void yuv2rgb_c_init_tables(const int inv_table[4])
{
	int i = 0;
	void* table_r = 0, *table_g = 0, *table_b = 0;
	int entry_size = 0;
	uint32_t* table_32 = 0;
	void* table_start;
	int64_t crv = inv_table[0];
	int64_t cbu = inv_table[1];
	int64_t cgu = -inv_table[2];
	int64_t cgv = -inv_table[3];
	int64_t cy = 1 << 16;
	int64_t oy = 0;
	uint8_t table_Y[1024];
	const int isRgb = isBGR(0);

	table_start = table_32 = av_malloc((197 + 2*  682 + 256 + 132) * sizeof(uint32_t));
	entry_size = sizeof(uint32_t);
	table_r = table_32 + 197;
	table_b = table_32 + 197 + 685;
	table_g = table_32 + 197 + 2*  682;

	cy = (cy*  255) / 219;
	oy = 16 << 16;

	cy = (cy*  65535) >> 16;
	crv = (crv*  65535 * 65535) >> 32;
	cbu = (cbu*  65535 * 65535) >> 32;
	cgu = (cgu*  65535 * 65535) >> 32;
	cgv = (cgv*  65535 * 65535) >> 32;

	oy -= 256*  0;

	for (i = 0; i < 1024; i++) {
		int j;

		j = (cy*  (((i - 384) << 16) - oy) + (1 << 31)) >> 32;
		j = (j < 0) ? 0 : ((j > 255) ? 255 : j);
		table_Y[i] = j;
	}

	for (i = -197; i < 256 + 197; i++)
		((uint32_t* ) table_r)[i] = table_Y[i + 384] << (isRgb ? 16 : 0);
	for (i = -132; i < 256 + 132; i++)
		((uint32_t* ) table_g)[i] = table_Y[i + 384] << 8;
	for (i = -232; i < 256 + 232; i++)
		((uint32_t* ) table_b)[i] = table_Y[i + 384] << (isRgb ? 0 : 16);

	for (i = 0; i < 256; i++) {
		table_rV[i] = (uint8_t* ) table_r + entry_size * div_round(crv * (i - 128), 76309);
		table_gU[i] = (uint8_t* ) table_g + entry_size * div_round(cgu * (i - 128), 76309);
		table_gV[i] = entry_size*  div_round(cgv * (i - 128), 76309);
		table_bU[i] = (uint8_t* ) table_b + entry_size * div_round(cbu * (i - 128), 76309);
	}

}

int yuv2rgb_c(uint8_t*  src[], int stride[], int w, int h, int x, int y, char *bmpbuf)
{
	static char table_inited = 0;
	int i, j, k;
	char* p1 = NULL, *p2 = NULL;

	if (w < 0 || h < 0 || !bmpbuf)
		return -1;

	if (!table_inited) {
		yuv2rgb_c_init_tables(Inverse_Table);
		table_inited = 1;
	}

	if (!bmpbuf)
		return -1;

	for (i = 0; i < h; i += 2) {
		uint32_t* r, *b;
		uint32_t* g;
		uint8_t* py_1 = src[0] + i * stride[0];
		uint8_t* py_2 = py_1 + stride[0];
		uint8_t* pu = src[1] + (i >> 1) * stride[1];
		uint8_t* pv = src[2] + (i >> 1) * stride[2];

		p1 = &(bmpbuf[(i + 1)*  w * 3]);
		p2 = &(bmpbuf[(i)*  w * 3]);

		j = 0;
		k = 0;

		unsigned int h_size = w >> 3;
		while (h_size--) {
			int U, V;
			int Y;

			U = pu[0];
			V = pv[0];
			r = (void* )table_rV[V];
			g = (void* )(table_gU[U] + table_gV[V]);
			b = (void* )table_bU[U];

			Y = py_1[2*  0];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];

			Y = py_1[2*  0 + 1];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];

			Y = py_2[2*  0];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];
			Y = py_2[2*  0 + 1];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];

			U = pu[1];
			V = pv[1];
			r = (void* )table_rV[V];
			g = (void* )(table_gU[U] + table_gV[V]);
			b = (void* )table_bU[U];

			Y = py_2[2*  1];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];
			Y = py_2[2*  1 + 1];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];

			Y = py_1[2*  1];
			Y = py_1[2*  1 + 1];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];

			U = pu[2];
			V = pv[2];
			r = (void* )table_rV[V];
			g = (void* )(table_gU[U] + table_gV[V]);
			b = (void* )table_bU[U];

			Y = py_1[2*  2];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];
			Y = py_1[2*  2 + 1];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];

			Y = py_2[2*  2];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];
			Y = py_2[2*  2 + 1];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];

			U = pu[3];
			V = pv[3];
			r = (void* )table_rV[V];
			g = (void* )(table_gU[U] + table_gV[V]);
			b = (void* )table_bU[U];

			Y = py_2[2*  3];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];
			Y = py_2[2*  3 + 1];
			p2[k++] = b[Y] >> 16;
			p2[k++] = g[Y] >> 8;
			p2[k++] = r[Y];

			Y = py_1[2*  3];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];
			Y = py_1[2*  3 + 1];
			p1[j++] = b[Y] >> 16;
			p1[j++] = g[Y] >> 8;
			p1[j++] = r[Y];

			pu += 4;
			pv += 4;
			py_1 += 8;
			py_2 += 8;
		}
	}
	return 0;
}
#endif

// 24bpp ->>16bpp or 8bpp
void fb_copy_data_to_buffer(char* dst, char *src, int len)
{
	int i = 0, k = 0;;
	unsigned short j = 0;
	int width = in_width - in_width % 4;
	switch (fb_bpp) {
	case 16:
		for (i = 0; i < len / 3; i++) {
			j = ((src[3*  i] & 0xf8) >> 3) | ((src[3 * i + 1] & 0xfc) << 3) | ((src[3 * i + 2] & 0xf8) <<
											   8);
			if ((i % width == 0) && (i >= width))
				k++;
			*((unsigned short* )(dst + (i + k * (fb_vinfo.xres_virtual - width)) * 2)) = j;
		}
		break;
	case 24:
		memcpy(dst, src, len);
	}

}

static void set_bpp(struct fb_var_screeninfo* p, int bpp)
{
	p->bits_per_pixel = (bpp + 1) & ~1;
	p->red.msb_right = p->green.msb_right = p->blue.msb_right = p->transp.msb_right = 0;
	p->transp.offset = p->transp.length = 0;
	p->blue.offset = 0;
	switch (bpp) {
	case 32:
		p->transp.offset = 24;
		p->transp.length = 8;
	case 24:
		p->red.offset = 16;
		p->red.length = 8;
		p->green.offset = 8;
		p->green.length = 8;
		p->blue.length = 8;
		break;
	case 16:
		p->red.offset = 11;
		p->green.length = 6;
		p->red.length = 5;
		p->green.offset = 5;
		p->blue.length = 5;
		break;
	case 15:
		p->red.offset = 10;
		p->green.length = 5;
		p->red.length = 5;
		p->green.offset = 5;
		p->blue.length = 5;
		break;
	}
}

static int fb_preinit(int reset)
{
	static int fb_preinit_done = 0;
	static int fb_err = -1;

	if (reset) {
		fb_preinit_done = 0;
		return 0;
	}

	if (fb_preinit_done)
		return fb_err;
	fb_preinit_done = 1;

	if (!fb_dev_name && !(fb_dev_name = getenv("FRAMEBUFFER")))
		fb_dev_name = av_strdup("/dev/fb0");

	if ((fb_dev_fd = open(fb_dev_name, O_RDWR)) == -1) {
		goto err_out;
	}
	if (ioctl(fb_dev_fd, FBIOGET_VSCREENINFO, &fb_vinfo)) {
		goto err_out;
	}
	fb_orig_vinfo = fb_vinfo;

	fb_bpp = fb_vinfo.bits_per_pixel;

	/* 16 and 15 bpp is reported as 16 bpp*/
	if (fb_bpp == 16)
		fb_bpp = fb_vinfo.red.length + fb_vinfo.green.length + fb_vinfo.blue.length;

	fb_err = 0;
	av_free(fb_dev_name);
	return 0;
      err_out:
	if (fb_dev_fd >= 0)
		close(fb_dev_fd);
	fb_dev_fd = -1;
	fb_err = -1;
	av_free(fb_dev_name);
	return -1;
}

static struct fb_cmap* make_directcolor_cmap(struct fb_var_screeninfo *var)
{
	int i, cols, rcols, gcols, bcols;
	uint16_t* red, *green, *blue;
	struct fb_cmap* cmap;

	rcols = 1 << var->red.length;
	gcols = 1 << var->green.length;
	bcols = 1 << var->blue.length;

	/* Make our palette the length of the deepest color*/
	cols = (rcols > gcols ? rcols : gcols);
	cols = (cols > bcols ? cols : bcols);

	red = av_malloc(cols*  sizeof(red[0]));
	if (!red) {
		return NULL;
	}
	for (i = 0; i < rcols; i++)
		red[i] = (65535 / (rcols - 1))*  i;

	green = av_malloc(cols*  sizeof(green[0]));
	if (!green) {
		av_free(red);
		return NULL;
	}
	for (i = 0; i < gcols; i++)
		green[i] = (65535 / (gcols - 1))*  i;

	blue = av_malloc(cols*  sizeof(blue[0]));
	if (!blue) {
		av_free(red);
		av_free(green);
		return NULL;
	}
	for (i = 0; i < bcols; i++)
		blue[i] = (65535 / (bcols - 1))*  i;

	cmap = av_malloc(sizeof(struct fb_cmap));
	if (!cmap) {
		av_free(red);
		av_free(green);
		av_free(blue);
		return NULL;
	}
	cmap->start = 0;
	cmap->transp = 0;
	cmap->len = cols;
	cmap->red = red;
	cmap->blue = blue;
	cmap->green = green;
	cmap->transp = NULL;

	return cmap;
}

int fb_config(uint32_t d_width, uint32_t d_height, uint32_t flags, char* title, uint32_t format)
{
	struct fb_cmap* cmap;

	out_width = d_width;
	out_height = d_height;

	if (flags) {
		out_width = fb_vinfo.xres;
		out_height = fb_vinfo.yres;
	}

	if (out_width < in_width || out_height < in_height) {
		return 1;
	}

	if (vo_config_count == 0) {
		if (ioctl(fb_dev_fd, FBIOGET_FSCREENINFO, &fb_finfo)) {
			return 1;
		}

		if (fb_finfo.type != FB_TYPE_PACKED_PIXELS) {
			return 1;
		}

		switch (fb_finfo.visual) {
		case FB_VISUAL_TRUECOLOR:
			break;
		case FB_VISUAL_DIRECTCOLOR:
			if (ioctl(fb_dev_fd, FBIOGETCMAP, &fb_oldcmap)) {
				return 1;
			}
			if (!(cmap = make_directcolor_cmap(&fb_vinfo)))
				return 1;
			if (ioctl(fb_dev_fd, FBIOPUTCMAP, cmap)) {
				return 1;
			}
			fb_cmap_changed = 1;
			av_free(cmap->red);
			av_free(cmap->green);
			av_free(cmap->blue);
			av_free(cmap);
			break;
		default:
			return 1;
		}

		fb_size = fb_finfo.smem_len;
		fb_line_len = fb_finfo.line_length;
		if ((frame_buffer =
		     (uint8_t* ) mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_dev_fd,
				      0)) == (uint8_t* ) - 1) {
			return 1;
		}
	}

	center = frame_buffer +
	    ((out_width - in_width) / 2)*  fb_pixel_size + ((out_height - in_height) / 2) * fb_line_len;

	memset(frame_buffer, '\0', fb_line_len*  fb_vinfo.yres);

	return 0;
}

int vout_fbdev_open(GxVideoOut*  vo, void *arg)
{
	if (vo->name) {
		if (fb_dev_name)
			av_free(fb_dev_name);
		fb_dev_name = av_strdup(vo->name);
	}
	return fb_preinit(0);
}

int query_format(uint32_t format)
{
	// open the device, etc.
	if (fb_preinit(0))
		return 0;
	if ((format & IMGFMT_BGR_MASK) == IMGFMT_BGR) {
		int fb_target_bpp = format & 0xff;
		set_bpp(&fb_vinfo, fb_target_bpp);
		fb_vinfo.xres_virtual = fb_vinfo.xres;
		fb_vinfo.yres_virtual = fb_vinfo.yres;
		if (ioctl(fb_dev_fd, FBIOPUT_VSCREENINFO, &fb_vinfo)) {
			return -1;
		}
		fb_pixel_size = fb_vinfo.bits_per_pixel / 8;
		fb_bpp = fb_vinfo.red.length + fb_vinfo.green.length + fb_vinfo.blue.length + fb_vinfo.transp.length;

	} else if (format == IMGFMT_YV12) {
		fb_vinfo.xres_virtual = fb_vinfo.xres;
		fb_vinfo.yres_virtual = fb_vinfo.yres;
		fb_pixel_size = fb_vinfo.bits_per_pixel / 8;
		fb_bpp = fb_vinfo.red.length + fb_vinfo.green.length + fb_vinfo.blue.length + fb_vinfo.transp.length;
	}
	return 0;
}

int vout_fbdev_config(GxVideoOut*  vo, GxStreamVideoHeader * sh_video, GxRect * out_win)
{
	int ret = query_format(vo->img_format);
	if (ret)
		return ret;
	return fb_config(out_win->width, out_win->height, vo->full_screen, NULL, vo->img_format);
}

int vout_fbdev_draw_frame(GxVideoOut*  vo, uint8_t * frame, int len)
{
	char* bmpbuf = NULL;
	GxImage* image = (GxImage *) frame;
	if (len != sizeof(GxImage))
		return -1;

	in_width = image->width;
	in_height = image->height;

	if (image->imgfmt == IMGFMT_YV12) {
		bmpbuf = (char* )av_malloc(image->width * image->height * 3);
		if (bmpbuf == NULL)
			return -1;
		if (yuv2rgb_c(image->planes, image->stride, image->width, image->height, image->x, image->y, bmpbuf))
			return -1;
		if (frame_buffer)
			fb_copy_data_to_buffer((char*)frame_buffer, bmpbuf, image->width*  image->height * 3);
		av_free(bmpbuf);
		return 0;
	}

	fb_copy_data_to_buffer((char*)frame_buffer, bmpbuf, image->width*  image->height * 3);

	return 0;
}

int vout_fbdev_draw_sub(GxVideoOut*  vo, uint8_t * data, int len)
{
	return 0;
}

int vout_fbdev_draw_slice(uint8_t*  src[], int stride[], int w, int h, int x, int y)
{
	uint8_t* dest = frame_buffer + (fb_line_len * y) + (x * fb_pixel_size);
	char* bmpbuf = (char *)av_malloc(w * h * 3);

	if (yuv2rgb_c(src, stride, w, h, x, y, bmpbuf))
		return -1;

	fb_copy_data_to_buffer((char*)dest, bmpbuf, w*  h * 3);

	av_free(bmpbuf);

	return 0;
}

void vout_fbdev_check_events(GxVideoOut*  vo)
{

}

static void vout_fbdev_close(GxVideoOut*  vo)
{
	if (fb_cmap_changed) {
		fb_cmap_changed = 0;
	}
	if (next_frame)
		av_free(next_frame);
	if (fb_dev_fd >= 0) {
		close(fb_dev_fd);
		fb_dev_fd = -1;
	}
	if (frame_buffer)
		munmap(frame_buffer, fb_size);
	next_frame = frame_buffer = NULL;
	fb_preinit(1);		
}

static int vout_fbdev_control(GxVideoOut*  vo, int cmd, va_list arg)
{
	return 0;
}

GxVideoOutClass gx_vout_fbdev = {
	._inherit = {		// GxMediaFilter
		._inherit = {	// GxObject
			.name    = "FBDEV",
			.parent  = &gx_VideoOutBase,
			.size    = sizeof(GxVideoOut),
			.create  = NULL,
			.release = NULL,
			.event   = NULL,
		},
		.run   = NULL,
		.pause = NULL,
		.stop  = NULL,
	},
	.author = {
		.info    = "VideoOut FBDEV",
		.name    = "",
		.desc    = "",
		.author  = "L.F.",
		.comment = "based on the code from mplayer (probably Arpi)"},

	.name         = "FBDEV",
	.open         = vout_fbdev_open,
	.close        = vout_fbdev_close,
	.config       = vout_fbdev_config,
	.draw_frame   = vout_fbdev_draw_frame,
	.draw_sub     = vout_fbdev_draw_sub,
	.control      = vout_fbdev_control,
	.check_events = vout_fbdev_check_events,
};
