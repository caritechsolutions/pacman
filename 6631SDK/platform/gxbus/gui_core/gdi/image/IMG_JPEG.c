/*
   ---------------------------START-OF-HEADER------------------------------
   ----------------------------------------------------------------------
File        : IMG_JPEG.c
Purpose     : Implementation of IMG_JPEG... functions
---------------------------END-OF-HEADER------------------------------
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "gui_debug.h"
#include "gal.h"
#include <setjmp.h>
#include "hd_gal.h"
#include "SDL.h"

#ifndef NO_OS
#include "jpeglib.h"
#include "jpegint.h"
#include "jhead.h"
#include <sys/stat.h>
#include "hd_system.h"

typedef struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */

	jmp_buf setjmp_buffer;	/* for return to caller */
} *my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

METHODDEF(void) my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 0);
}

static unsigned int soft_jpeg_check(const char *path)
{
    FILE * fp = NULL;
    unsigned char data[2];

    if(NULL == path)
	return GUI_FALSE;

    fp = fopen(path, "r");
    if(NULL == fp)
	return GUI_FALSE;

    memset(data,0x00,sizeof(data));
    if(!fread(data, 1, 2, fp))
    {
	fclose(fp);
	return GUI_TRUE;
    }

    if((0xFF != data[0]) || (0xD8 != data[1]))
    {
	fclose(fp);
	return GUI_FALSE;
    }

    fclose(fp);
    return GUI_TRUE;
}

/*
 * SOME FINE POINTS:
 *
 * In the above loop, we ignored the return value of jpeg_write_scanlines,
 * which is the number of scanlines actually written.  We could get away
 * with this because we were only relying on the value of cinfo.next_scanline,
 * which will be incremented correctly.  If you maintain additional loop
 * variables then you should be careful to increment them properly.
 * Actually, for output to a stdio stream you needn't worry, because
 * then jpeg_write_scanlines will write all the lines passed (or else exit
 * with a fatal error).  Partial writes can only occur if you use a data
 * destination module that can demand suspension of the compressor.
 * (If you don't know what that's for, you don't need it.)
 *
 * If the compressor requires full-image buffers (for entropy-coding
 * optimization or a multi-scan JPEG file), it will create temporary
 * files for anything that doesn't fit within the maximum-memory setting.
 * (Note that temp files are NOT needed if you use the default parameters.)
 * On some systems you may need to set up a signal handler to ensure that
 * temporary files are deleted if the program is interrupted.  See libjpeg.doc.
 *
 * Scanlines MUST be supplied in top-to-bottom order if you want your JPEG
 * files to be compatible with everyone else's.  If you cannot readily read
 * your data in that order, you'll need an intermediate array to hold the
 * image.  See rdtarga.c or rdbmp.c for examples of handling bottom-to-top
 * source data using the JPEG code's internal virtual-array mechanisms.
 */

/******************** JPEG DECOMPRESSION SAMPLE INTERFACE *******************/

/* This half of the example shows how to read data from the JPEG decompressor.
 * It's a bit more refined than the above, in that we show:
 *   (a) how to modify the JPEG library's standard error-reporting behavior;
 *   (b) how to allocate workspace using the library's memory manager.
 *
 * Just to make this example a little different from the first one, we'll
 * assume that we do not intend to put the whole image into an in-memory
 * buffer, but to send it line-by-line someplace else.  We need a one-
 * scanline-high JSAMPLE array as a work buffer, and we will let the JPEG
 * memory manager allocate it for us.  This approach is actually quite useful
 * because we don't need to remember to deallocate the buffer separately: it
 * will go away automatically when the JPEG object is cleaned up.
 */

/*
 * ERROR HANDLING:
 *
 * The JPEG library's standard error handler (jerror.c) is divided into
 * several "methods" which you can override individually.  This lets you
 * adjust the behavior without duplicating a lot of code, which you might
 * have to update with each future release.
 *
 * Our example here shows how to override the "error_exit" method so that
 * control is returned to the library's caller when a fatal error occurs,
 * rather than calling exit() as the standard error_exit method does.
 *
 * We use C's setjmp/longjmp facility to return control.  This means that the
 * routine which calls the JPEG library must first execute a setjmp() call to
 * establish the return point.  We want the replacement error_exit to do a
 * longjmp().  But we need to make the setjmp buffer accessible to the
 * error_exit routine.  To do this, we make a private extension of the
 * standard JPEG error handler object.  (If we were using C++, we'd say we
 * were making a subclass of the regular error handler.)
 *
 * Here's the extended error handler struct:
 */

static int scale_width;
static int scale_height;

static void image_zoom(struct jpeg_decompress_struct *cinfo)
{
	GuiCore *gui_core = GUI_GetCurrent();

	cinfo->scale_num = 1;
	cinfo->scale_denom = 1;

	unsigned int adjust_width = 0, adjust_height = 0;

	adjust_width = cinfo->image_width / 8;
	adjust_height = cinfo->image_height / 8;
	if(((adjust_width > gui_core->config.width) && (0 == (cinfo->image_width % 8))) ||
	   ((adjust_height > gui_core->config.height) && (0 == (cinfo->image_height % 8))))
	{
		cinfo->scale_denom = 8;
		return;
	}

	adjust_width = cinfo->image_width / 4;
	adjust_height = cinfo->image_height / 4;
	if((adjust_width > gui_core->config.width) ||
	   (adjust_height > gui_core->config.height))
	{
		cinfo->scale_denom = 4;
		return;
	}

	adjust_width = cinfo->image_width / 2;
	adjust_height = cinfo->image_height / 2;
	if((adjust_width > gui_core->config.width) ||
	   (adjust_height > gui_core->config.height))
	{
		cinfo->scale_denom = 2;
		return;
	}

	return;

}

#if 0
static void _RGB_to_YUV422(Uint8 *src, Uint8 *dst,
		int src_width, int src_height,
		int width, int height)
{
	int i = 0, j = 0;
	Uint8 r = 0, g = 0, b = 0, y = 0, u = 0, v = 0;

	if((NULL == src) || (NULL == dst))
		return;

	for(j = 0; j < height; j++) {
		for(i = 0; i < width; i++) {
			r = src[(j * src_width + i) * 3];
			g = src[(j * src_width + i) * 3 + 1];
			b = src[(j * src_width + i) * 3 + 2];

			y = (299 * r + 578 * g + 114 *b) / 1000;
			u = (500 * r - 419 * g - 81 * b) / 1000 + 128;
			v = (500 * b - 169 * r - 331 * g) / 1000 + 128;

			if(i % 2) {
				dst[(j * width + i) * 2] = v;
				dst[(j * width + i) * 2 + 1] = y;
			}
			else {
				dst[(j * width + i) * 2] = u;
				dst[(j * width + i) * 2 + 1] = y;
			}
		}
	}

}

static int _YUV_overlay_scale(image_desc *image_desc)
{
	SDL_Overlay *overlay = NULL;
	SDL_Surface *over_surface = NULL;
	SDL_Rect  rect = {0}, src_rect = {0};
	unsigned char *pptr = NULL, *ptr = NULL;
	unsigned int pitch = 0;
	int i = 0, ret = 0;

	if(NULL == image_desc)
		return (1);

	if((0 == scale_width) && (0 == scale_height))
		return (0);

	if((image_desc->width == scale_width) && (image_desc->height == scale_height))
		return (0);

	scale_width = (scale_width >> 2) << 2;
	pitch = (image_desc->width >> 2) << 2;
	over_surface =  SDL_CreateRGBSurface(SDL_RESIZABLE | SDL_SWSURFACE,
			pitch,
			image_desc->height,
			24,
			0xFF0000, 0x00FF00, 0x0000FF, 0);
	if(NULL == over_surface)
	{
	    return (1);
	}

	src_rect.x = src_rect.y = 0;
	src_rect.w = pitch;
	src_rect.h = image_desc->height;

	overlay = SDL_CreateYUVOverlay(pitch, image_desc->height, SDL_UYVY_OVERLAY, over_surface);
	if(NULL == overlay)
	{
	    SDL_FreeSurface(over_surface);
	    return (1);
	}

	ptr = (unsigned char *)(image_desc->data);
	pptr = overlay->pixels[0];
	for(i = 0; i < image_desc->height; i++)
		memcpy(pptr + i * overlay->pitches[0], ptr + image_desc->width * 2 * i, pitch * 2);

	rect.x = 0;
	rect.y = 0;
	rect.w = scale_width;
	rect.h = scale_height;

	ret = SDL_DisplayYUVOverlay(overlay, &rect);
	if(ret)
	{
	    if(overlay)
	    {
		SDL_FreeYUVOverlay(overlay);
	    }

	    if(over_surface)
	    {
		SDL_FreeSurface(over_surface);
	    }

	    return (1);
	}

	pptr = over_surface->pixels;
	//pptr = overlay->pixels[0];

	_RGB_to_YUV422(pptr,
			(Uint8 *)(image_desc->data),
			pitch,
			image_desc->height,
			scale_width,
			scale_height);
	image_desc->width = scale_width;
	image_desc->height = scale_height;

	if(overlay)
	{
	    SDL_FreeYUVOverlay(overlay);
	}

	if(over_surface)
	{
	    SDL_FreeSurface(over_surface);
	}

	return (0);
}
#endif

int IMG_scale(int width, int height)
{
	scale_width = width;
	scale_height = height;

	return (0);
}

#define CMKY_MIN(a,b)	    ((a) < (b)) ? (a) : (b)

static void cmyk_to_yuv(void *buffer, unsigned int width)
{
    unsigned char C = 0, M = 0, Yr = 0, K = 0, Max = 0;
    int R = 0, G = 0, B = 0;
    int Y = 0, Cb = 0, Cr = 0;
    unsigned int i = 0;
    unsigned char *ptr = NULL;

    if(NULL == buffer)
	return;

    ptr = (unsigned char *)buffer;

    i = 0;
    while(i < width)
    {
	C = ptr[i * 4];
	M = ptr[i * 4 + 1];
	Yr = ptr[i * 4 + 2];
	K = ptr[i * 4 + 3];

	Max = (C > K) ? C : K;
	Max = (M > Max) ? M : Max;
	Max = (Yr > Max) ? Yr : Max;
#if 1
	B   =   C * K / 255;//255 - (CMKY_MIN(255, (C * (255 - K) + K) / 255));//(255   -   C)   *  (255 - K)   /   255;
	G   =   M * K / 255;//255 - (CMKY_MIN(255, (M * (255 - K) + K) / 255));//(255   -   M)   *  (255 - K)   /   255;
	R   =   Yr * K / 255;//255 - (CMKY_MIN(255, (Yr * (255 - K) + K) / 255));//(255   -   Yr)  *  (255 - K)   /   255;
#else
	B = C * K / Max;
	G = M * K / Max;
	R = Yr * K / Max;
#endif

	Y  = (299 * R + 578 * G + 114 * B) / 1000;
	Cb = (500 * R - 419 * G - 81  * B) / 1000 + 128;
	Cr = (500 * B - 169 * R - 331 * G) / 1000 + 128;

	if(i % 2)
	    ptr[i * 2] = Cr;
	else
	    ptr[i * 2] = Cb;
	ptr[i * 2 + 1] = Y;

	i++;
#ifdef ECOS_OS
	GxCore_ThreadYield();
#endif
    }

}

#ifdef CONFIG_SOFT_JPEG
static void cmyk_to_rgb(void *buffer, unsigned int width)
{
    unsigned char C = 0, M = 0, Yr = 0, K = 0, Max = 0;
    int R = 0, G = 0, B = 0;
    unsigned int i = 0;
    unsigned char *ptr = NULL;

    if(NULL == buffer)
	return;

    ptr = (unsigned char *)buffer;

    i = 0;
    while(i < width)
    {
	C = ptr[i * 4];
	M = ptr[i * 4 + 1];
	Yr = ptr[i * 4 + 2];
	K = ptr[i * 4 + 3];

	Max = (C > K) ? C : K;
	Max = (M > Max) ? M : Max;
	Max = (Yr > Max) ? Yr : Max;
#if 1
	B   =   C * K / 255;//255 - (CMKY_MIN(255, (C * (255 - K) + K) / 255));//(255   -   C)   *  (255 - K)   /   255;
	G   =   M * K / 255;//255 - (CMKY_MIN(255, (M * (255 - K) + K) / 255));//(255   -   M)   *  (255 - K)   /   255;
	R   =   Yr * K / 255;//255 - (CMKY_MIN(255, (Yr * (255 - K) + K) / 255));//(255   -   Yr)  *  (255 - K)   /   255;
#else
	B = C * K / Max;
	G = M * K / Max;
	R = Yr * K / Max;
#endif

	ptr[i * 3] = B;
	ptr[i * 3 + 1] = G;
	ptr[i * 3 + 2] = R;

	i++;
    }

}
#endif /*CONFIG_SOFT_JPEG*/

static int sg_jpeg_malloc_total;
void *_jpeg_malloc_cb(int size)
{
	if((image_config.threshold_size) &&
	   ((sg_jpeg_malloc_total + size) > image_config.threshold_size)) {
		gxlogi("Soft decode JPEG malloc failed(Need memory is more than threshold 0x%x you set by gal_img_threshod_size)!\n", image_config.threshold_size);
		return (NULL);
	}
	sg_jpeg_malloc_total += size;
	return (GUI_MALLOCZ(size));
}

void _jpeg_free_cb(void *ptr, int size)
{
	sg_jpeg_malloc_total -= size;
	GUI_FREE(ptr);
}

void ycc_rgb_ycc422 (j_decompress_ptr cinfo,
		     JSAMPIMAGE input_buf, JDIMENSION input_row,
		     JSAMPARRAY output_buf, int num_rows);
extern void process_gray_levels(image_desc *img_desc);

/*********************************************************************
 *
 *       IMG_JPEG_SOFT_Draw
 */
int IMG_JPEG_SOFT_Draw(void *screen, const char *pJPEG, int x0, int y0, int screen_width, int screen_height)
{
	int buffer_width = 0, buffer_height = 0, ret = 0;
	int zoom_width = 0, zoom_height = 0, zoom = 0, area_width = 0, area_height = 0;
	int start_ms = 0, end_ms = 0;
	image_desc img_desc = {0};
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile = NULL;		/* source file */
	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride, size = 0, component = 0, display_stride = 0;		/* physical row width in output buffer */
	unsigned int x_pos = 0, y_pos = 0, i = 0;
	static char *sample_buf = NULL;
	static GxHwMallocObj *vq = NULL;


	sample_buf = NULL;
	vq = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if(GUI_FALSE == soft_jpeg_check(pJPEG))
	    return 1;

	if ((infile = fopen((char *)pJPEG, "rb")) == NULL) {
		gxloge ( "[GUI]can't open %s\n", pJPEG);
		return 0;
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) 	/* Now we can initialize the JPEG decompression object. */
	{
	    ret = 1;
	    goto out;
	}
	cinfo.malloc_func = _jpeg_malloc_cb;
	cinfo.free_func = _jpeg_free_cb;
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */
	if((cinfo.image_width > gui_core->config.width) ||
			(cinfo.image_height > gui_core->config.height))
	{
	    image_zoom(&cinfo);
	}
#if 0
	if(jpeg_has_multiple_scans(&cinfo))
	{
	    ret = 1;
	    goto out;
	}
#endif

	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	if(cinfo.out_color_space != JCS_CMYK) {
		cinfo.out_color_space = JCS_YCbCr;
		((struct jpeg_color_deconverter *)(cinfo.cconvert))->color_convert = ycc_rgb_ycc422;
	}

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	component = (cinfo.output_components >= 2) ? (cinfo.output_components) : (2);
	row_stride = cinfo.output_width * component;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

	size = cinfo.output_height * cinfo.output_width * component;
	vq = hd_malloc(&vq, size);
	if(vq) {
		sample_buf = vq->usr_p;
	}
	if(sample_buf == NULL) {
		sample_buf = (char *)GUI_MALLOC(size);
		if(NULL == sample_buf) {
			ret = 1;
			goto out;
		}
	}
	memset(sample_buf, 0, size);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	if(cinfo.output_width % 2)
	    display_stride = (cinfo.output_width - 1) * 2;
	else
	    display_stride = cinfo.output_width * 2;

	if((!cinfo.output_width) ||
	   (!cinfo.output_height) ||
	   (!cinfo.output_components))
	{
		ret = 1;
		goto out;
	}

	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
#ifdef ECOS_OS
		if(0 == start_ms) {
			start_ms = hd_get_tick();
		}
#endif
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);

		/* Assume put_scanline_someplace wants a pointer and sample count. */
		if(cinfo.output_components >= 1) {
			if(JCS_CMYK == cinfo.out_color_space)
				cmyk_to_yuv(*buffer, cinfo.output_width);
			memcpy(&(sample_buf[(cinfo.output_scanline - 1) * display_stride]),
					*buffer,
					display_stride);
		}
#ifdef ECOS_OS
		end_ms = hd_get_tick();
		if((end_ms - start_ms) > 500) {
			gui_delay(10);
			end_ms = start_ms = 0;
		}
#endif
	}


	if(screen_width != -1)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(screen_height != -1)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	if(cinfo.output_width < buffer_width) {
		x_pos = x0 + (buffer_width - cinfo.output_width) / 2;
		x_pos = x_pos - (x_pos % 2);
	}

	if(cinfo.output_height < buffer_height) {
		y_pos =y0 + (buffer_height - cinfo.output_height) / 2;
		y_pos = y_pos - (y_pos % 2);
	}

	img_desc.data   = sample_buf;
	img_desc.width  = display_stride / 2;
	img_desc.height = cinfo.output_height;
	img_desc.type   = JPEG_TYPE;
	img_desc.bpp    = 16;
	if(vq) {
		img_desc.vq = vq;
	}
	zoom_width = img_desc.width;
	zoom_height = img_desc.height;

	if(zoom_width > gui_core->config.width)
	{
	    zoom_width = gui_core->config.width;
	    zoom_height = zoom_height * gui_core->config.width / img_desc.width;
	    zoom = GUI_TRUE;
	}

	if(zoom_height > gui_core->config.height)
	{
	    zoom_height = gui_core->config.height;
	    zoom_width = zoom_width * gui_core->config.height / zoom_height;
	    zoom = GUI_TRUE;
	}

#if 0
	IMG_scale(zoom_width, zoom_height);
	ret = _YUV_overlay_scale(&img_desc);
	if(ret)
	{
	    jpeg_destroy_decompress(&cinfo);
	    GUI_FREE(sample_buf);
	    fclose(infile);
	    return (1);
	}
#endif

	img_desc.size = img_desc.width * img_desc.height * 2;

	area_width = img_desc.width;
	area_height = img_desc.height;
	if(zoom)
	{
	    if(gui_core->config.width >= img_desc.width)
		{
			x_pos = (gui_core->config.width - img_desc.width) / 2;
			x_pos = x_pos - (x_pos % 2);
		}
	    else
	    {
		x_pos = 0;
		area_width = gui_core->config.width;
		area_height = area_height * area_width / img_desc.width;
	    }

	    if(gui_core->config.height >= area_height)
		y_pos = (area_height - img_desc.height) / 2;
	    else
	    {
		y_pos = 0;
		area_height = gui_core->config.height;
		area_width = img_desc.width * area_height / img_desc.height;
	    }
	    area_width = (area_width >> 1) << 1;
	}

	if(((area_width != img_desc.width) ||
	   (area_height != img_desc.height)) &&
	   (((GuiSurface *)screen)->sf.color_format == GX_COLOR_FMT_YCBCR422))
	{
		GuiSurface *src_surface = NULL, *dst_surface = NULL;
		image_desc tmp_desc = {0};
		GAL_Rect src_rect = {0}, dst_rect = {0};

		hd_commit_blit();
		src_rect.x = src_rect.y = 0;
		src_rect.w = img_desc.width;
		src_rect.h = img_desc.height;
		src_surface = hd_get_surface(&src_rect, 32, NULL, GUI_TRUE);
		if(src_surface == NULL) {
			ret = 1;
			goto out;
		}

		process_gray_levels(&img_desc);
		hd_new_blit_list();
		hd_img_desc(src_surface, &img_desc, 0, 0, src_rect.w, src_rect.h);
		hd_end_blit_list();

		dst_rect.x = dst_rect.y = 0;
		dst_rect.w = area_width;
		dst_rect.h = area_height;
		dst_surface = hd_get_surface(&dst_rect, 32, NULL, GUI_TRUE);
		if(dst_surface == NULL)
		{
			hd_free_surface(src_surface);
			ret = 1;
			goto out;
		}

		hd_new_blit_list();
		hd_zoom_surface(src_surface, &src_rect, dst_surface, &dst_rect);
		hd_end_blit_list();

#if 0
		hd_new_blit_list();
		src_rect = dst_rect;
		dst_rect.x = (((GuiSurface *)screen)->sf.width - dst_surface->sf.width) / 2;
		dst_rect.x = (dst_rect.x / 2) * 2;
		dst_rect.y = (((GuiSurface *)screen)->sf.height - dst_surface->sf.height) / 2;
		hd_copy_surface(dst_surface, &src_rect, screen, &dst_rect);
		hd_end_blit_list();
#endif
		tmp_desc.data = dst_surface->data;
		tmp_desc.vq = dst_surface->vq;
		tmp_desc.width = dst_surface->sf.width;
		tmp_desc.height = dst_surface->sf.height;
		tmp_desc.bpp = dst_surface->bpp;
		tmp_desc.fmt = dst_surface->sf.color_format;
		tmp_desc.size = dst_surface->buffer_size;
		dst_rect.x = (((GuiSurface *)screen)->sf.width - dst_surface->sf.width) / 2;
		dst_rect.x = (dst_rect.x / 2) * 2;
		dst_rect.y = (((GuiSurface *)screen)->sf.height - dst_surface->sf.height) / 2;
		hd_img_desc_gradiant(screen, &tmp_desc, dst_rect.x, dst_rect.y, tmp_desc.width, tmp_desc.height, gui_core->config.play_image_mode);

		hd_free_surface(src_surface);
		hd_free_surface(dst_surface);
	}
	else
	{
		process_gray_levels(&img_desc);
		hd_img_desc_gradiant(screen, &img_desc, x_pos, y_pos, area_width, area_height, gui_core->config.play_image_mode);
	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

out:
	if(vq) {
		hd_free(vq);
		GUI_FREE(img_desc.vq);
		vq = NULL;
	} else {
		GUI_FREE(sample_buf);
	}
	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fclose(infile);

	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return (ret);
}

int IMG_JPEG_Draw(void *screen, const char *pJPEG, int x0, int y0, int screen_width, int screen_height)
{
	int buffer_width = 0, buffer_height = 0;
	int ret = 0;
	GuiCore *gui_core = NULL;

	gui_core = GUI_GetCurrent();
	if(screen_width != -1)
		buffer_width = screen_width;
	else
		buffer_width = gui_core->config.width;

	if(screen_height != -1)
		buffer_height = screen_height;
	else
		buffer_height = gui_core->config.height;

	ret = hd_jpeg_decode(screen, pJPEG, x0, y0, buffer_width, buffer_height);

	return (ret);
}

#ifdef CONFIG_SOFT_JPEG
static void own_image_zoom(struct jpeg_decompress_struct *cinfo, int zw, int zh)
{
	int scale_x,scale_y;
	GuiCore *gui_core = GUI_GetCurrent();

	cinfo->scale_denom = 8;

	if(zw > gui_core->config.width)
		zw = gui_core->config.width;

	if(zh > gui_core->config.height)
		zh = gui_core->config.height;

	scale_x = 8 * zw / cinfo->image_width;
	scale_y = 8 * zh / cinfo->image_height;

	cinfo->scale_num = scale_x > scale_y ? scale_x : scale_y;
}
#endif /*CONFIG_SOFT_JPEG*/

image_desc *IMG_JPEG_Load_and_Cut(const char *pFile, int zw, int zh)
{
#ifdef CONFIG_SOFT_JPEG
	image_desc *img_desc = NULL;

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile;		/* source file */
	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride, size = 0, display_stride = 0;		/* physical row width in output buffer */
	static char *sample_buf = NULL;

	sample_buf = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if(NULL == pFile || zw < 0 || zh < 0)
		return (NULL);


	if(GUI_FALSE == soft_jpeg_check(pFile))
	    return (NULL);

	if ((infile = fopen((char *)pFile, "rb")) == NULL) {
		gxloge ( "[GUI]can't open %s\n", pFile);
		return (NULL);
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		if(sample_buf)
			GUI_FREE(sample_buf);
		return (NULL);
	}
	cinfo.malloc_func = _jpeg_malloc_cb;
	cinfo.free_func = _jpeg_free_cb;
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */
	own_image_zoom(&cinfo, zw, zh);

	if(JCS_CMYK != cinfo.out_color_space)
	{
	    cinfo.out_color_space = JCS_RGB;
#ifndef NDS_SUPPORT
	   cinfo.uncompressed = 1;
#endif
	}

	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	GUI_Printf("####jpeg cinfo->  w=%d h=%d output_components=%d\n", cinfo.output_width,
			cinfo.output_height, cinfo.output_components);

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

	size = cinfo.output_height * row_stride;
	sample_buf = (char *)GUI_MALLOC(size);
	if(NULL == sample_buf)
	{
	    fclose(infile);
	    return (NULL);
	}

	memset(sample_buf, 0, size);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */

		if(JCS_CMYK == cinfo.out_color_space)
		{
		    cmyk_to_rgb(*buffer, cinfo.output_width);
		    display_stride = 3 * cinfo.output_width;
		}
		else
		{
		    display_stride = row_stride;
		}
		memcpy(&(sample_buf[(cinfo.output_scanline - 1) * display_stride]),
				buffer[0],
				display_stride);
	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fclose(infile);

	img_desc = gal_img_new();
	if(NULL == img_desc)
		return (NULL);

	memset(img_desc, 0, sizeof(image_desc));

	img_desc->type = JPEG_TYPE;
	img_desc->filename = NULL;
	img_desc->width = cinfo.output_width;
	img_desc->height = cinfo.output_height;
	img_desc->data = sample_buf;
	img_desc->pal = NULL;
	img_desc->bpp = 16;


	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return (img_desc);
#else
	return NULL;
#endif /*CONFIG_SOFT_JPEG*/
}

#ifdef CONFIG_SOFT_JPEG
static void own_image_zoom2(struct jpeg_decompress_struct *cinfo, int zw, int zh)
{
	int i=0;
	cinfo->scale_num = 1;
	cinfo->scale_denom = 8;
	GuiCore *gui_core = GUI_GetCurrent();

	if(zw > gui_core->config.width)
		zw = gui_core->config.width;

	if(zh > gui_core->config.height)
		zh = gui_core->config.height;

	for(i=8; i>0; i--) {
		if((cinfo->image_width * i / 8) <= zw && (cinfo->image_height * i / 8) <= zh ) {
			cinfo->scale_num = i;
			break;
		}
	}

	//gxlogd("||---scale:  %d/8\n", cinfo->scale_num);
}
#endif /*CONFIG_SOFT_JPEG*/

image_desc *IMG_JPEG_Load_and_Zoom(const char *pFile, int zw, int zh)
{
#ifdef CONFIG_SOFT_JPEG
	image_desc *img_desc = NULL;

	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile;		/* source file */
	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride, size = 0, display_stride = 0;		/* physical row width in output buffer */
	static char *sample_buf = NULL;

	sample_buf = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if(NULL == pFile || zw < 0 || zh < 0)
		return (NULL);

	if ((infile = fopen((char *)pFile, "rb")) == NULL) {
		gxloge ( "[GUI]can't open %s\n", pFile);
		return (NULL);
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		if(sample_buf)
			GUI_FREE(sample_buf);
		return (NULL);
	}
	cinfo.malloc_func = _jpeg_malloc_cb;
	cinfo.free_func = _jpeg_free_cb;
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */

	own_image_zoom2(&cinfo, zw, zh);

	if(JCS_CMYK != cinfo.out_color_space)
	{
	    cinfo.out_color_space = JCS_RGB;
#ifndef NDS_SUPPORT
	    cinfo.uncompressed = 1;
#endif
	}

	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * cinfo.output_components;

	GUI_Printf("####jpeg cinfo->  w=%d h=%d output_components=%d\n", cinfo.output_width,
			cinfo.output_height, cinfo.output_components);

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

	size = cinfo.output_height * row_stride;
	sample_buf = (char *)GUI_MALLOC(size);
	if(NULL == sample_buf)
	{
	    fclose(infile);
	    return (NULL);
	}

	memset(sample_buf, 0, size);

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		if(JCS_CMYK == cinfo.out_color_space)
		{
		    cmyk_to_rgb(*buffer, cinfo.output_width);
		    display_stride = 3 * cinfo.output_width;
		}
		else
		{
		    display_stride = row_stride;
		}
		memcpy(&(sample_buf[(cinfo.output_scanline - 1) * display_stride]),
				buffer[0],
				display_stride);
	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fclose(infile);

	img_desc = gal_img_new();
	if(NULL == img_desc)
		return (NULL);

	memset(img_desc, 0, sizeof(image_desc));

	img_desc->type = JPEG_TYPE;
	img_desc->filename = NULL;
	img_desc->width = cinfo.output_width;
	img_desc->height = cinfo.output_height;
	img_desc->data = sample_buf;
	img_desc->pal = NULL;
	img_desc->bpp = 16;//16;


	/* At this point you may want to check to see whether any corrupt-data
	 * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	 */

	/* And we're done! */
	return (img_desc);
#else
	return NULL;
#endif /*CONFIG_SOFT_JPEG*/
}

void ycc_rgb_ycc422 (j_decompress_ptr cinfo,
		             JSAMPIMAGE input_buf, JDIMENSION input_row,
		             JSAMPARRAY output_buf, int num_rows)
{
	register int y, cb, cr;
	register JSAMPROW outptr;
	register JSAMPROW inptr0 = NULL, inptr1 = NULL, inptr2 = NULL;
	register JDIMENSION col;
	JDIMENSION num_cols = cinfo->output_width;

	SHIFT_TEMPS while (--num_rows >= 0) {
	if(cinfo->num_components > 0)
	{
		inptr0 = input_buf[0][input_row];
		if(cinfo->num_components > 1)
		{
			inptr1 = input_buf[1][input_row];
			if(cinfo->num_components > 2)
				inptr2 = input_buf[2][input_row];
		}
	}
	input_row++;
	outptr = *output_buf++;
	for (col = 0; col < num_cols; col++) {
		if(inptr0)
			y = GETJSAMPLE(inptr0[col]);
		else
			y = 0x10;
		if(inptr1)
			cb = GETJSAMPLE(inptr1[col]);
		else
			cb = 0x80;
		if(inptr2)
			cr = GETJSAMPLE(inptr2[col]);
		else
			cr = 0x80;
		if(0 == (col % 2))
		{
			outptr[0] = cb;
			outptr[1] = y;
		}
		else
		{
		    outptr[0] = cr;
		    outptr[1] = y;
		}
		outptr += 2;
	}
	}
}

image_desc *IMG_JPEG_SOFT_Load(image_desc *img_desc, const char *pFile)
{
	GuiCore *gui_core = GUI_GetCurrent();
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile;		/* source file */
	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride, size = 0, display_width = 0;		/* physical row width in output buffer */
	static char *sample_buf = NULL;

	sample_buf = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if(pFile == NULL)
		return (img_desc);

	if ((infile = fopen((char *)pFile, "rb")) == NULL) {
		gxloge ( "[GUI]can't open %s\n", pFile);
		return (img_desc);
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		if(img_desc && sample_buf)
			gal_img_free_memory(img_desc);
		return (img_desc);
	}
	cinfo.malloc_func = _jpeg_malloc_cb;
	cinfo.free_func = _jpeg_free_cb;
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */
	if((cinfo.image_width > gui_core->config.width) || (cinfo.image_height > gui_core->config.height))
		image_zoom(&cinfo);

	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	//修改颜色空间转换格式
	cinfo.out_color_space = JCS_YCbCr;
	((struct jpeg_color_deconverter *)(cinfo.cconvert))->color_convert = ycc_rgb_ycc422;
	//((struct jpeg_color_deconverter *)(cinfo.cconvert))->color_conver = ycc_rgb_ycc422;
	//cconvert = ((gx_color_deconverter *)(&cinfo.cconvert));
	//cconvert->pub.color_convert = ycc_rgb_ycc422;

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * 3;//cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

	size = cinfo.output_height * cinfo.output_width * 3;//cinfo.output_components;

	if (img_desc == NULL) {
		img_desc = (image_desc*)GUI_MALLOCZ(sizeof(image_desc));
		if(NULL == img_desc)
			return (NULL);

		memset(img_desc, 0, sizeof(image_desc));

		img_desc->filename = GUI_STRDUP(pFile);
	}

	display_width = cinfo.output_width;
	if(display_width % 2)
		display_width -= 1;

	img_desc->type     = JPEG_TYPE;
	img_desc->width    = display_width;
	img_desc->height   = cinfo.output_height;
	img_desc->bpp      = 16;
	img_desc->size     = size;
	img_desc->effect   = EFFECT_COPY;
	img_desc->fmt      = GX_COLOR_FMT_YCBCR422;
	sample_buf = (char *)gal_img_alloc_memory(img_desc);

	if(NULL == sample_buf)
		return (img_desc);
	img_desc->data = sample_buf;

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */

		memcpy(&(sample_buf[(cinfo.output_scanline - 1) * display_width * 2]),
				*buffer,
				display_width * 2);

	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fclose(infile);

	process_gray_levels(img_desc);
	return img_desc;
}

image_desc *IMG_JPEG_NO_Zoom_Load(image_desc *img_desc, const char *pFile)
{
	/* This struct contains the JPEG decompression parameters and pointers to
	 * working space (which is allocated as needed by the JPEG library).
	 */
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	 * Note that this struct must live as long as the main JPEG parameter
	 * struct, to avoid dangling-pointer problems.
	 */
	struct my_error_mgr jerr;
	/* More stuff */
	FILE *infile;		/* source file */
	JSAMPARRAY buffer;	/* Output row buffer */
	int row_stride, size = 0, display_width = 0;		/* physical row width in output buffer */
	static char *sample_buf = NULL;

	sample_buf = NULL;
	/* In this example we want to open the input file before doing anything else,
	 * so that the setjmp() error recovery below can assume the file is open.
	 * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	 * requires it in order to read binary files.
	 */
	if(pFile == NULL)
		return (img_desc);

	if ((infile = fopen((char *)pFile, "rb")) == NULL) {
		gxloge ( "[GUI]can't open %s\n", pFile);
		return (img_desc);
	}

	/* Step 1: allocate and initialize JPEG decompression object */

	/* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		 * We need to clean up the JPEG object, close the input file, and return.
		 */
		jpeg_destroy_decompress(&cinfo);
		fclose(infile);
		if(img_desc && sample_buf)
			gal_img_free_memory(img_desc);
		return (img_desc);
	}
	cinfo.malloc_func = _jpeg_malloc_cb;
	cinfo.free_func = _jpeg_free_cb;
	/* Now we can initialize the JPEG decompression object. */
	jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */

	jpeg_stdio_src(&cinfo, infile);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	 *   (a) suspension is not possible with the stdio data source, and
	 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	 * See libjpeg.doc for more info.
	 */

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	 * jpeg_read_header(), so we do nothing here.
	 */

	/* Step 5: Start decompressor */
	(void)jpeg_start_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */
	//修改颜色空间转换格式
	cinfo.out_color_space = JCS_YCbCr;
	((struct jpeg_color_deconverter *)(cinfo.cconvert))->color_convert = ycc_rgb_ycc422;
	//((struct jpeg_color_deconverter *)(cinfo.cconvert))->color_conver = ycc_rgb_ycc422;
	//cconvert = ((gx_color_deconverter *)(&cinfo.cconvert));
	//cconvert->pub.color_convert = ycc_rgb_ycc422;

	/* We may need to do some setup of our own at this point before reading
	 * the data.  After jpeg_start_decompress() we have the correct scaled
	 * output image dimensions available, as well as the output colormap
	 * if we asked for color quantization.
	 * In this example, we need to make an output work buffer of the right size.
	 */
	/* JSAMPLEs per row in output buffer */
	row_stride = cinfo.output_width * 3;//cinfo.output_components;
	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) & cinfo, JPOOL_IMAGE, row_stride, 1);

	size = cinfo.output_height * cinfo.output_width * 3;//cinfo.output_components;

	if (img_desc == NULL) {
		img_desc = (image_desc*)GUI_MALLOCZ(sizeof(image_desc));
		if(NULL == img_desc)
			return (NULL);

		memset(img_desc, 0, sizeof(image_desc));

		img_desc->filename = GUI_STRDUP(pFile);
	}

	display_width = cinfo.output_width;
	if(display_width % 2)
		display_width -= 1;

	img_desc->type     = JPEG_TYPE;
	img_desc->width    = display_width;
	img_desc->height   = cinfo.output_height;
	img_desc->bpp      = 16;
	img_desc->size     = size;
	img_desc->effect   = EFFECT_COPY;
	sample_buf = (char *)gal_img_alloc_memory(img_desc);

	if(NULL == sample_buf)
		return (img_desc);
	img_desc->data = sample_buf;

	/* Step 6: while (scan lines remain to be read) */
	/*           jpeg_read_scanlines(...); */

	/* Here we use the library's state variable cinfo.output_scanline as the
	 * loop counter, so that we don't have to keep track ourselves.
	 */
	while (cinfo.output_scanline < cinfo.output_height) {
		/* jpeg_read_scanlines expects an array of pointers to scanlines.
		 * Here the array is only one element long, but you could ask for
		 * more than one scanline at a time if that's more convenient.
		 */
		(void)jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */

		memcpy(&(sample_buf[(cinfo.output_scanline - 1) * display_width * 2]),
				*buffer,
				display_width * 2);

	}

	/* Step 7: Finish decompression */

	(void)jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	 * with the stdio data source.
	 */

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	 * Here we postpone it until after no more JPEG errors are possible,
	 * so as to simplify the setjmp error logic above.  (Actually, I don't
	 * think that jpeg_destroy can do an error exit, but why assume anything...)
	 */
	fclose(infile);

	return img_desc;
}

int IMG_JPEG_SOFT_STOP(void)
{
	return (0);
}

int IMG_JPEG_STOP(void)
{
	return (hd_jpeg_stop());
}
#else
#include "mini_gui_private.h"
#include "gdi_play.h"
#include "hd_gal.h"
#endif /*NO_OS*/

#ifdef GUI_JPEG_INFO
extern ImageInfo_t ImageInfo;
extern void ProcessFile(const char * FileName);
#endif /*GUI_JPEG_INFO*/

image_desc *IMG_JPEG_Load(image_desc *img_desc, const char *pFile)
{
	image_desc *desc = NULL;

	desc = hd_jpeg_load(img_desc, pFile);

	if(desc)
		desc->effect = EFFECT_COPY;

	return (desc);
}

int IMG_JPEG_GetInfo(const char *pFile, Image_Exif *image_info)
{
	int ret = 0, offset = 0, file_len = 0;
#ifndef NO_OS
	struct stat buf;
#endif /*NO_OS*/
	int size  = 0;
	char *data = NULL, *ptr = NULL;
	handle_t f_handle = 0;

	if ((NULL == pFile))
		return (1);

#ifndef NO_OS
	if(GUI_FALSE == soft_jpeg_check(pFile))
	{
	    memset(image_info, 0, sizeof(Image_Exif));
	    return 1;
	}

	ret = stat(pFile, &buf);
	if (ret < 0)
		return (1);
	size = buf.st_size;
#else
	FILE *pf = fopen(pFile, "r");
	if(NULL == pf)
	{
		return (1);
	}

	fseek(pf, 0, SEEK_END);
	size = ftell(pf);
	fseek(pf, 0, SEEK_SET);

	fclose(pf);
#endif /*NO_OS*/

	f_handle = GxCore_Open((char *)pFile, "r");

	if(f_handle) {
		data = (char *)GUI_MALLOC(1024);
		if(NULL == data) {
			GxCore_Close(f_handle);
			return (1);
		}

		memset(data, 0, 1024);
		ret = GxCore_Read(f_handle, data, 1, 2);
		if(ret <= 0)
		{
			GxCore_Close(f_handle);
			GUI_FREE(data);
			return (1);
		}
		file_len += 2;
		ptr = data;
		/*APP0*/
		if((0xFF == ptr[0]) && (0xE0 == ptr[1])) {
			memset(data, 0, 1024);
			ret = GxCore_Read(f_handle, data, 1, 2);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
			file_len += 2;
			ptr = data;
			offset = (ptr[0] << 8) | ptr[1];

			memset(data, 0, 1024);
			ret = GxCore_Read(f_handle, data, 1, offset - 2);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
			file_len += (offset - 2);
			ptr = data;
			image_info->horizontal_pixel = ((ptr[8] << 8) | ptr[9]);
			image_info->vertical_pixel = ((ptr[10] << 8) | ptr[12]);
		}

		memset(data, 0, 1024);
		ret = GxCore_Read(f_handle, data, 1, 2);
		if(ret <= 0)
		{
			GxCore_Close(f_handle);
			GUI_FREE(data);
			return (1);
		}
		ptr = data;
		while(((0xFF != ptr[0]) || (0xC0 != ptr[1])) &&
		       ((0xFF != ptr[0]) || (0xC2 != ptr[1])) &&
		       (file_len < size)) {
		        memset(data, 0, 1024);
		    ret = GxCore_Read(f_handle, data, 1, 2);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
		    file_len += 2;
		    ptr = data;
		    offset = (ptr[0] << 8) | ptr[1];

		    memset(data, 0, 1024);
		    GxCore_Seek(f_handle, offset - 2, SEEK_CUR);
		    file_len += (offset - 2);
		    memset(data, 0, 1024);
		    ret = GxCore_Read(f_handle, data, 1, 2);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
		    file_len += 2;
		    ptr = data;
		}
		/*SOF*/
		if(((0xFF == ptr[0]) && (0xC0 == ptr[1])) ||
		    ((0xFF == ptr[0]) && (0xC2 == ptr[1]))) {
		    memset(data, 0, 1024);
		    GxCore_Seek(f_handle, 3, SEEK_CUR);
		    ret = GxCore_Read(f_handle, data, 1, 4);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
		    ptr = data;
		    image_info->height = (ptr[0] << 8) | ptr[1];
		    image_info->width = (ptr[2] << 8) | ptr[3];
		    memset(data, 0, 1024);
		    ret = GxCore_Read(f_handle, data, 1, 4);
			if(ret <= 0)
			{
				GxCore_Close(f_handle);
				GUI_FREE(data);
				return (1);
			}
		    ptr = data;
		    image_info->bpp = (*ptr) * 8;
		}

		GxCore_Close(f_handle);
		GUI_FREE(data);
	}
#ifdef GUI_JPEG_INFO
#ifndef NO_OS
	memset(&ImageInfo, 0, sizeof(ImageInfo));
	ProcessFile(pFile);
	image_info->image_ext_info = ImageInfo;
#endif /*NO_OS*/
#endif  /*GUI_JPEG_INFO*/

	return (0);
}

