#ifndef NO_OS
#include <sys/stat.h>
#include "gxavdev.h"
#include "gui_private.h"
#include "gui.h"
#else
#include "mini_gui_private.h"
#include "gui_core.h"
extern GuiCore gui;
#endif /*NO_OS*/

#include "gxcore.h"
#include "av/avapi.h"
#include "av/gxav.h"
#include "hd_gal.h"
#include "av/gxav_jpeg_propertytypes.h"
#include "image_cache.h"
#include "gdi_play.h"

#ifdef LINUX_OS
#include <unistd.h> //for mmap

#if (_FILE_OFFSET_BITS==64)
#undef __USE_FILE_OFFSET64
#include <sys/mman.h> //for mmap

#define __USE_FILE_OFFSET64

#else
#include <sys/mman.h> //for mmap
#endif

#endif


static int s_jpeg_state;
static handle_t jpeg_fifo = 0;

#define JPEG_MAX_TRUNK                (32  * 1024)
#define JPEG_DRIVER_FIFO_SIZE         (256 * 1024)
#define JPEG_DRIVER_FIFO_CLIP         (JPEG_DRIVER_FIFO_SIZE/JPEG_MAX_TRUNK)
#define JPEG_DRIVER_FIFO_ALMOST_FULL  ( (JPEG_DRIVER_FIFO_SIZE * ( JPEG_DRIVER_FIFO_CLIP-1)) / JPEG_DRIVER_FIFO_CLIP)
#define JPEG_DRIVER_FIFO_ALMOST_EMPTY ( JPEG_DRIVER_FIFO_SIZE / 2)

#define JPEG_DISPLAY_444            (0)
#define JPEG_DISPLAY_422            (1)
#define JPEG_DISPLAY_420            (0)
#define JPEG_DISPLAY_411            (0)

#if (JPEG_DISPLAY_444)
#define JPEG_COMPONENT          (3)
#elif (JPEG_DISPLAY_422)
#define JPEG_COMPONENT          (2)
#elif (JPEG_DISPLAY_420)
#define JPEG_COMPONENT          (1)
#elif (JPEG_DISPLAY_411)
#define JPEG_COMPONENT          (1)
#endif

#define JPEG_ERROR_CLOSE_FILE       (1 << 0)
#define JPEG_ERROR_UNLINK_FIFO      (1 << 1)
#define JPEG_ERROR_DESTROY_FIFO     (1 << 2)
#define JPEG_ERROR_FREE_TRUNK       (1 << 3)
#define JPEG_ERROR_STOP             (1 << 4)
#define JPEG_ERROR_DESTROY          (1 << 5)
#define JPEG_ERROR_FREE_IMAGE       (1 << 6)
#define JPEG_ERROR_FREE_DATA        (1 << 7)

#define JPEG_FRAME_BUFFER_SIZE	    (12 * 1024 * 1024)
#define JPEG_DISPLAY_SIZE	    (8 * 1024 * 1024)


static const char * error_string = 0;
inline static void _set_exception(const char* str)
{
	error_string = str;
}

inline static const char* _get_exception_str(void)
{
	return error_string;
}

int _jpeg_error_exception(handle_t hfile,
		void *config,
		unsigned char *src,
		image_desc *image,
		char *string,
		unsigned int flag)
{
	int ret = 0;
	GxJpegProperty_Create *pconfig = NULL;
	GxJpegProperty_Destroy destroy_config = {0};

	if(string)
		gxlogd("[GUI] [JPEG]########%s########\n", string);

	if(flag & JPEG_ERROR_CLOSE_FILE)
		GxCore_Close(hfile);

	if(flag & JPEG_ERROR_STOP)
		GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);

	if(flag & JPEG_ERROR_UNLINK_FIFO)
		ret = GxAVUnLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo);

	if(flag & JPEG_ERROR_DESTROY_FIFO) {
		ret = GxAVFifoDestroy(g_device_handle, jpeg_fifo);
		jpeg_fifo = 0;
	}

	if(flag & JPEG_ERROR_FREE_TRUNK)
		GUI_FREE(src);

	if(flag & JPEG_ERROR_DESTROY) {
		pconfig = (GxJpegProperty_Create *)config;
		if(NULL == pconfig)
			return (0);

		destroy_config = *pconfig;
		ret = GxAVSetProperty(g_device_handle,
				jpeg_handle,
				GxJPEGPropertyID_Destroy,
				&destroy_config,
				sizeof(GxJpegProperty_Destroy));
	}

	if(flag & JPEG_ERROR_FREE_DATA) {
		if(NULL == image)
			return (0);

		GUI_FREE(image->data);
	}

	if(flag & JPEG_ERROR_FREE_IMAGE) {
		if(NULL == image)
			return (0);

		GUI_FREE(image);
	}

	return (ret);
}

static unsigned int _get_jpeg_weight(unsigned int width, unsigned int height)
{
	GuiCore *gui_core = NULL;
	unsigned int adjust_width = 0, adjust_height = 0;

	gui_core = GUI_GetCurrent();
	if((0 == gui_core->config.width)  && (0 == gui_core->config.height))
		return 1;

	adjust_width = width / 8;
	adjust_height = height / 8;
	if(((adjust_width >= gui_core->config.width) && (0 == (width % 8))) ||
	   ((adjust_height >= gui_core->config.height) && (0 == (height % 8))))
		return 8;

	adjust_width = width / 4;
	adjust_height = height / 4;
	if((adjust_width >= gui_core->config.width) || (adjust_height >= gui_core->config.height))
		return 4;

	adjust_width = width / 2;
	adjust_height = height / 2;
	if((adjust_width >= gui_core->config.width) || (adjust_height >= gui_core->config.height))
		return 2;

	return 1;
}


static int hd_jpeg_zoom(unsigned int dst_width,
		unsigned int dst_height,
		unsigned int width,
		unsigned int height,
		unsigned int stride_width,
		unsigned int stride_height,
		unsigned int format,
		void *buffer_y,
		void *buffer_u,
		void *buffer_v,
		void *temp_buffer)
{
	int ret = 0, hcoeffecent = 0, vcoefficent = 0;
	size_t image_size, prive_size;
	GuiSurface *dst = NULL, *srcy = NULL,*srcu = NULL, *srcv = NULL;
	GuiSurface *tempy = NULL, *tempu = NULL, *tempv = NULL;
	GAL_Rect dst_rect = {0}, src_rect = {0};
	GxVpuProperty_ZoomSurface zoom_surface = {0};
	GxVpuProperty_Complet complet = {0};

	if((NULL == buffer_y) ||
	   (NULL == buffer_u) ||
	   (NULL == buffer_v) ||
	   (NULL == temp_buffer))
		return 1;

	prive_size = stride_width * stride_height;
	image_size = dst_width * dst_height;

#define RELASE_SURFACE(y, u, v) \
	if(y) \
		hd_free_surface(y); \
	if(u) \
		hd_free_surface(u); \
	if(v) \
		hd_free_surface(v); \

	switch(format)
	{
		case GX_COLOR_FMT_YCBCR420:
			vcoefficent = 2;
			hcoeffecent = 2;
			break;
		case GX_COLOR_FMT_YCBCR422v:
			vcoefficent = 1;
			hcoeffecent = 2;
			break;
		case GX_COLOR_FMT_YCBCR422h:
			vcoefficent = 2;
			hcoeffecent = 1;
			break;
		case GX_COLOR_FMT_YCBCR400:
			hcoeffecent = vcoefficent = 0;
			break;
		default:
			hcoeffecent = vcoefficent = 1;
			break;
	}

	src_rect.x = src_rect.y = 0;
	src_rect.w = stride_width;
	src_rect.h = height;
	srcy = hd_get_surface(&src_rect, 3, buffer_y, GUI_TRUE);
	if(NULL == srcy) {
		RELASE_SURFACE(srcy, srcu, srcv);
		return 1;
	}

	src_rect.w = stride_width;
	src_rect.h = height;

	srcu = hd_get_surface(&src_rect, 3, buffer_u, GUI_TRUE);
	if(NULL == srcu) {
		RELASE_SURFACE(srcy, srcu, srcv);
		return 1;
	}

	src_rect.w = stride_width;
	src_rect.h = height;

	srcv = hd_get_surface(&src_rect, 3, buffer_v, GUI_TRUE);
	if(NULL == srcv) {
		RELASE_SURFACE(srcy, srcu, srcv);
		return 1;
	}

	/*Patch shencz July 8th, 2013 for GX3201NRE*/
	if((dst_width < (width / 4)) ||
			(dst_height < (height / 4)))
	{
		GuiSurface *quar_y = NULL, *quar_u = NULL, *quar_v = NULL;
		void *quar_buf = NULL;
		GAL_Rect quar_rect = {0};

		quar_rect.x = quar_rect.y = 0;
		quar_rect.w = width / 4;
		if(quar_rect.w % 2)
			quar_rect.w += 1;
		quar_rect.h = height / 4;

		quar_buf = buffer_y;
		quar_y = hd_get_surface(&quar_rect, 3, quar_buf, GUI_TRUE);

		quar_buf = buffer_u;
		quar_u = hd_get_surface(&quar_rect, 3, quar_buf, GUI_TRUE);

		quar_buf = buffer_v;
		quar_v = hd_get_surface(&quar_rect, 3, quar_buf, GUI_TRUE);

		gdi_begin();

		zoom_surface.src_surface = srcy->hw_surface;
		zoom_surface.src_is_ksurface = is_ksurface(srcy);
		zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
		zoom_surface.src_rect.width = width;
		zoom_surface.src_rect.height = height;
		zoom_surface.dst_surface = quar_y->hw_surface;
		zoom_surface.dst_is_ksurface = is_ksurface(quar_y);
		zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
		zoom_surface.dst_rect.width = quar_rect.w;
		zoom_surface.dst_rect.height = quar_rect.h;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_ZoomSurface,
				&zoom_surface,
				sizeof(GxVpuProperty_ZoomSurface));
		if(ret)
		{
			gxlogd("[GUI] JPEG Y 1/4 zoom failed.\n");
			RELASE_SURFACE(quar_y, quar_u, quar_v);
			return 1;
		}

		zoom_surface.src_surface = srcu->hw_surface;
		zoom_surface.src_is_ksurface = is_ksurface(srcu);
		zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
		if(hcoeffecent)
			zoom_surface.src_rect.width = width / hcoeffecent;
		else
			zoom_surface.src_rect.width = width;
		if(vcoefficent)
			zoom_surface.src_rect.height = height / vcoefficent;
		else
			zoom_surface.src_rect.height = height;

		zoom_surface.dst_surface = quar_u->hw_surface;
		zoom_surface.dst_is_ksurface = is_ksurface(quar_u);
		zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
		if(hcoeffecent)
			zoom_surface.dst_rect.width = quar_rect.w / hcoeffecent;
		else
			zoom_surface.dst_rect.width = quar_rect.w;
		if(vcoefficent)
			zoom_surface.dst_rect.height = quar_rect.h / vcoefficent;
		else
			zoom_surface.dst_rect.height = quar_rect.h;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_ZoomSurface,
				&zoom_surface,
				sizeof(GxVpuProperty_ZoomSurface));
		if(ret)
		{
			gxlogd("[GUI] JPEG U 1/4 zoom failed.\n");
			RELASE_SURFACE(quar_y, quar_u, quar_v);
			return 1;
		}

		zoom_surface.src_surface = srcv->hw_surface;
		zoom_surface.src_is_ksurface = is_ksurface(srcv);
		zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
		if(hcoeffecent)
			zoom_surface.src_rect.width = width / hcoeffecent;
		else
			zoom_surface.src_rect.width = width;
		if(vcoefficent)
			zoom_surface.src_rect.height = height / vcoefficent;
		else
			zoom_surface.src_rect.height = height;

		zoom_surface.dst_surface = quar_v->hw_surface;
		zoom_surface.dst_is_ksurface = is_ksurface(quar_v);
		zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
		if(hcoeffecent)
			zoom_surface.dst_rect.width = quar_rect.w / hcoeffecent;
		else
			zoom_surface.dst_rect.width = quar_rect.w;
		if(vcoefficent)
			zoom_surface.dst_rect.height = quar_rect.h / vcoefficent;
		else
			zoom_surface.dst_rect.height = quar_rect.h;
		ret = GxAVSetProperty(g_device_handle,
				vpu_handle,
				GxVpuPropertyID_ZoomSurface,
				&zoom_surface,
				sizeof(GxVpuProperty_ZoomSurface));
		if(ret)
		{
			gxlogd("[GUI] JPEG V 1/4 zoom failed.\n");
			RELASE_SURFACE(quar_y, quar_u, quar_v);
			return 1;
		}

		gdi_end();

		hd_free_surface(srcy);
		hd_free_surface(srcu);
		hd_free_surface(srcv);
		src_rect.w = width = quar_rect.w;
		src_rect.h = height = quar_rect.h;
		srcy = quar_y;
		srcu = quar_u;
		srcv = quar_v;
	}

	dst_rect.x = dst_rect.y = 0;
	dst_rect.w = dst_width;
	dst_rect.h = dst_height;
	tempy = hd_get_surface(&dst_rect, 3, temp_buffer, GUI_TRUE);
	if(NULL == tempy) {
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}

	dst_rect.w = dst_width;
	dst_rect.h = dst_height;
	tempu = hd_get_surface(&dst_rect, 3, temp_buffer + image_size, GUI_TRUE);
	if(NULL == tempu) {
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}

	dst_rect.w = dst_width;
	dst_rect.h = dst_height;
	tempv = hd_get_surface(&dst_rect, 3, temp_buffer + image_size * 2, GUI_TRUE);
	if(NULL == tempv) {
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}

	gdi_begin();
	zoom_surface.src_surface = srcy->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(srcy);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	zoom_surface.src_rect.width = width;
	zoom_surface.src_rect.height = height;
	zoom_surface.dst_surface = tempy->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(tempy);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	zoom_surface.dst_rect.width = dst_width;
	zoom_surface.dst_rect.height = dst_height;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_ZoomSurface,
			&zoom_surface,
			sizeof(GxVpuProperty_ZoomSurface));
	if(ret)
	{
		gxlogd("[GUI] JPEG Y zoom failed.\n");
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}
	zoom_surface.src_surface = srcu->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(srcu);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	if(hcoeffecent)
		zoom_surface.src_rect.width = width / hcoeffecent;
	else
		zoom_surface.src_rect.width = width;
	if(vcoefficent)
		zoom_surface.src_rect.height = height / vcoefficent;
	else
		zoom_surface.src_rect.height = height;
	zoom_surface.dst_surface = tempu->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(tempu);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	if(hcoeffecent)
		zoom_surface.dst_rect.width = dst_width / hcoeffecent;
	else
		zoom_surface.dst_rect.width = dst_width;
	if(vcoefficent)
		zoom_surface.dst_rect.height = dst_height / vcoefficent;
	else
		zoom_surface.dst_rect.height = dst_height;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_ZoomSurface,
			&zoom_surface,
			sizeof(GxVpuProperty_ZoomSurface));
	if(ret)
	{
		gxlogd("[GUI] JPEG U zoom failed.\n");
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}
	zoom_surface.src_surface = srcv->hw_surface;
	zoom_surface.src_is_ksurface = is_ksurface(srcv);
	zoom_surface.src_rect.x = zoom_surface.src_rect.y = 0;
	if(hcoeffecent)
		zoom_surface.src_rect.width = width / hcoeffecent;
	else
		zoom_surface.src_rect.width = width;
	if(vcoefficent)
		zoom_surface.src_rect.height = height / vcoefficent;
	else
		zoom_surface.src_rect.height = height;
	zoom_surface.dst_surface = tempv->hw_surface;
	zoom_surface.dst_is_ksurface = is_ksurface(tempv);
	zoom_surface.dst_rect.x = zoom_surface.dst_rect.y = 0;
	if(hcoeffecent)
		zoom_surface.dst_rect.width = dst_width / hcoeffecent;
	else
		zoom_surface.dst_rect.width = dst_width;
	if(vcoefficent)
		zoom_surface.dst_rect.height = dst_height / vcoefficent;
	else
		zoom_surface.dst_rect.height = dst_height;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_ZoomSurface,
			&zoom_surface,
			sizeof(GxVpuProperty_ZoomSurface));
	if(ret)
	{
		gxlogd("[GUI] JPEG V zoom failed.\n");
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}
	gdi_end();

	dst_rect.w = dst_width;
	dst_rect.h = dst_height;
	dst = hd_get_surface(&dst_rect, 0, buffer_y, GUI_TRUE);
	if(NULL == dst) {
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		return 1;
	}

	gdi_begin();
	complet.src_buf_Y = tempy->sf.buffer;
	complet.src_buf_U = tempu->sf.buffer;
	complet.src_buf_V = tempv->sf.buffer;
	complet.src_base_width = dst_width;
	complet.src_width = dst_width;
	complet.src_height = dst_height;
	complet.src_color = format;
	complet.dst_surface = dst->hw_surface;
	complet.dst_rect.x = complet.dst_rect.y = 0;
	complet.dst_rect.width = dst_width;
	complet.dst_rect.height = dst_height;
	ret = GxAVSetProperty(g_device_handle,
			vpu_handle,
			GxVpuPropertyID_Complet,
			&complet,
			sizeof(GxVpuProperty_Complet));
	if(ret)
	{
		RELASE_SURFACE(srcy, srcu, srcv);
		RELASE_SURFACE(tempy, tempu, tempv);
		gxlogd("[GUI] JPEG zoom interlace.\n");
		return 1;
	}
	gdi_end();

	hd_add_blit_element(dst);
	RELASE_SURFACE(srcy, srcu, srcv);
	RELASE_SURFACE(tempy, tempu, tempv);

	return 0;
}

#if 0
static int jpeg_get_info(const char *path, Image_Exif *exif)
{
	GxJpegProperty_Info info = {0};
	GxJpegProperty_Create config = {0};
	GxJpegProperty_Config jpeg_para = {0};
	GxJpegProperty_Destroy destroy_config = {0};
	GxAvGateInfo jpeg_info = {0};
	GxJpegProperty_Status status = {0};

	handle_t hfile;
	struct stat buf;
	int size = 0, ret = 0;
	unsigned int config_size = 0, event = 0;
	unsigned char *src = NULL, *psrc = NULL;

	if(NULL == path)
		return (1);

	if(0 == g_device_handle)
		g_device_handle = GxAvdev_CreateDevice(0);

	if(0 == jpeg_handle) {
		jpeg_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
		if(0 == jpeg_handle)
			return (1);
	}

	ret = stat((char *)path, &buf);
	if (ret < 0)
		return (1);

	size = buf.st_size;

	if(0 == size)
		return (1);

	hfile = GxCore_Open((char *)path, "r");
	if(0 == hfile)
		return (1);


	psrc = src = (unsigned char *)GxCore_Mallocz(JPEG_MAX_TRUNK);
	if(NULL == src) {
		GxCore_Close(hfile);
		return (1);
	}

	if (jpeg_fifo == 0){
		jpeg_fifo = GxAVFifoCreate(g_device_handle, JPEG_DRIVER_FIFO_SIZE, GXAV_NO_PTS_FIFO);
		if(jpeg_fifo == 0) {
			_set_exception("Fifo create error!");
			goto err_out;
		}

		jpeg_info.almost_empty = JPEG_DRIVER_FIFO_ALMOST_EMPTY;
		jpeg_info.almost_full = JPEG_DRIVER_FIFO_ALMOST_FULL;
		GxAVFifoConfig(g_device_handle,jpeg_fifo,&jpeg_info);
	}
	else{
		GxAVFifoReset(g_device_handle,jpeg_fifo);
	}

	config.width = 1024;
	config.height = 1024;

	config.enable_interlace = GUI_TRUE;
	config_size = (config.width * config.height * JPEG_COMPONENT);
	config.buffer = NULL;

	GxAVGetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Create, &config, sizeof(GxJpegProperty_Create));

	jpeg_para.mode          = 0;
	jpeg_para.resample_mode = 1;
	jpeg_para.stride        = config.width;
	jpeg_para.max_width     = config.width;
	jpeg_para.max_height    = config.height;
	//jpeg_para.sof_pause	    = GUI_TRUE;
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Config,&jpeg_para,sizeof(GxJpegProperty_Config));
	GxAVLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo, 0, GXAV_PIN_INPUT);

	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Run, NULL, 0);

	for(;size>0;) {
		signed int send  = 0;
		unsigned char *ptr = NULL;

		if (size > JPEG_MAX_TRUNK)
			send = JPEG_MAX_TRUNK;
		else
			send = size;

		memset(src, 0, JPEG_MAX_TRUNK);
		ret = GxCore_Read(hfile, src, 1, send);
		if(0 == ret){
			_set_exception("Read jpeg data err!");
			goto err_out0;
		}

		while(send > 0)
		{
			ret = GxAVFifoWriteData(g_device_handle, jpeg_fifo, ptr, send, 1000000);
			if(ret != send)
			{
				gxlogd("[JPEG] Force Write!\n");
				GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_WriteData, NULL, 0);
			}

			send -= ret;
			size -= ret;
			ptr += ret;
		}

		GxAVGetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Status,&status,sizeof(GxJpegProperty_Status));
		if((status.status & DECODER_OVER) ||
				(status.status & SOF_OVER)){
			goto OVER;
		}
	}

OVER:
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_End, NULL, 0);
	gxlogd("~~~~~~status = %x~~~~~~\n",status.status);
	if(!(status.status & SOF_OVER)){
		GxAVWaitEvents(g_device_handle, jpeg_handle, EVENT_JPEG_FRAMEINFO, 1000000000, &event);
	}
	gxlogd("%s %d\n",__FILE__,__LINE__);
	status.status |= DECODER_OVER;

	ret = GxAVGetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Info, &info, sizeof(GxJpegProperty_Info));
	if(ret < 0){
		_set_exception("Get information error!");
		goto err_out0;
	}

	destroy_config = config;
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);
	GxAVUnLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo);
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Destroy,&destroy_config,sizeof(GxJpegProperty_Destroy));

err_out0:
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);
	GxAVUnLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo);
	destroy_config = config;
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Destroy,&destroy_config,sizeof(GxJpegProperty_Destroy));
err_out:
	if(!(status.status & DECODER_OVER))
		gxlogd("[GUI] [JPEG]:%s\n",_get_exception_str());
	if (psrc)
		GUI_FREE(psrc);

	GxCore_Close(hfile);
	exif->width = info.original_width;
	exif->height = info.original_height;
	return 0;
}
#endif

static unsigned int _get_frame_buffer_size(unsigned int width, unsigned int height)
{
	unsigned int frame_buffer_size = 0;

	frame_buffer_size = (width * height);
	if(frame_buffer_size % 8)
		frame_buffer_size = ((frame_buffer_size / 8) + 1) * 8;

	frame_buffer_size = frame_buffer_size * 3;

	return (frame_buffer_size);
}

image_desc *jpeg_image_descriptor(const char *path,
		image_desc *img_desc,
		int original_width,
		int original_height,
		int width,
		int height)
{
	GuiCore *gui_core = GUI_GetCurrent();
	GxJpegProperty_Info info = {0};
	GxJpegProperty_Create config = {0};
	GxJpegProperty_Config jpeg_para = {0};
	GxJpegProperty_Destroy destroy_config = {0};
	GxAvGateInfo jpeg_info = {0};
	GxJpegProperty_Status status = {0};

	handle_t hfile;
#ifndef NO_OS
	struct stat buf;
#else
	void *fb_buf = NULL;
	void *dsp_buf = NULL;
#endif /*NO_OS*/
	GAL_Rect rect = {0};
	int size = 0, ret = 0, i = 0;
	unsigned int weight = 0, adjust_width = 0;/*for Odd width*/
	void *data = NULL, *mem_buf = NULL;
	unsigned int config_size = 0, event = 0;
	unsigned char *src = NULL, *psrc = NULL;
	char *filename = NULL;
#ifndef NO_OS
	GxHwMallocObj *fifo_vq = NULL, *fb_vq = NULL, *dsp_vq = NULL;
#endif /*NO_OS*/

	if(NULL == path)
		return (NULL);

	if(0 == g_device_handle)
	{
#ifndef NO_OS
		g_device_handle = GxAvdev_CreateDevice(0);
#else
		g_device_handle = GxAVCreateDevice(0);
#endif /*NO_OS*/
	}

	if(0 >= jpeg_handle) {
#ifndef NO_OS
		jpeg_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
#else
		jpeg_handle = GxAVOpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
#endif /*NO_OS*/
		if(0 >= jpeg_handle) {
			gxlogd("@@@@@@@@@ jpeg_handle = %d\n", jpeg_handle);
			return (NULL);
		}
	}
	gxlogd("@@@@@@@@@ jpeg_handle = %d\n", jpeg_handle);
	_set_exception(NULL);

#ifndef NO_OS
	ret = stat((char *)path, &buf);
	if (ret < 0)
		return (NULL);

	size = buf.st_size;
#else
	FILE *pFile = fopen(path, "r");
	if(NULL == pFile)
	{
		return (NULL);
	}

	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	fclose(pFile);
#endif /*NO_OS*/

	if(0 == size)
		return (NULL);

	hfile = GxCore_Open((char *)path, "r");
	if(0 == hfile)
		return (NULL);

	psrc = src = (unsigned char *)GxCore_Mallocz(JPEG_MAX_TRUNK);
	if(NULL == src) {
		GxCore_Close(hfile);
		return (NULL);
	}

#ifndef NO_OS
	fifo_vq = hd_malloc(&fifo_vq, JPEG_DRIVER_FIFO_SIZE + 2 * 1024);
	if((NULL == fifo_vq) || ((fifo_vq) && (fifo_vq->usr_p == NULL)))
	    goto err_out;
	jpeg_fifo = GxAVFifoCreate(g_device_handle,
				   (void *)(((int)fifo_vq->kernel_p / 1024 + 1) * 1024),
				   JPEG_DRIVER_FIFO_SIZE,
				   GXAV_NO_PTS_FIFO);
#else
	void *fifo_buf = GxCore_Mallocz(JPEG_DRIVER_FIFO_SIZE);
	if(NULL == fifo_buf)
		goto err_out;
	jpeg_fifo = GxAVFifoCreate(g_device_handle, (void *)fifo_buf, JPEG_DRIVER_FIFO_SIZE, GXAV_NO_PTS_FIFO);
#endif /*NO_OS*/
	if(jpeg_fifo == 0) {
	    _set_exception("Fifo create error!");
	    goto err_out;
	}

	jpeg_info.almost_empty = JPEG_DRIVER_FIFO_ALMOST_EMPTY;
	jpeg_info.almost_full = JPEG_DRIVER_FIFO_ALMOST_FULL;
	GxAVFifoConfig(g_device_handle,jpeg_fifo,&jpeg_info);
	GxAVFifoReset(g_device_handle,jpeg_fifo);

	weight = _get_jpeg_weight(original_width, original_height);

	if((gui_core->config.width < original_width) || (gui_core->config.height < original_height))
	{
		config.width = original_width / weight / 16 * 16;
		if((original_width % weight) || ((original_width / weight) % 16))
		{
			config.width += 16;
		}

		config.height = original_height / weight / 16 * 16;
		if((original_height % weight) || ((original_height / weight) % 16))
		{
			config.height += 16;
		}

		/*Recheck width weight*/
		original_width = config.width;
		original_height = config.height;
	}
	else
	{
		config.width = ((original_width >> 4) + 1) << 4;
		config.height = ((original_height >> 4) + 1) << 4;
	}

	if(((0 == width) && (0 == height)) ||
	   ((original_width == width) ||
		(original_height == height))){
		config.enable_interlace = GUI_TRUE;
		config.dst_color = GX_COLOR_FMT_YCBCR422;
	}else{
		config.enable_interlace = GUI_FALSE;
		config.dst_color = GX_COLOR_FMT_YCBCR444;
	}
	config_size = (config.width * config.height * JPEG_COMPONENT);
	config.buffer = NULL;

	jpeg_para.mode			    = 0;
	jpeg_para.resample_mode		    = 1;
	jpeg_para.dst_color = config.dst_color;
	jpeg_para.enable_interlace = config.enable_interlace;
	jpeg_para.stride			    = config.width;
	jpeg_para.max_width			    = config.width;
	jpeg_para.max_height		    = config.height;
	jpeg_para.frame_buffer_size		    = _get_frame_buffer_size(jpeg_para.stride, config.height);
	jpeg_para.display_buffer_size	    = jpeg_para.frame_buffer_size;
#ifndef NO_OS
	fb_vq				    = hd_malloc_nocache(&fb_vq, jpeg_para.frame_buffer_size + 2 * 1024);
	if((NULL == fb_vq) || ((fb_vq) && (fb_vq->usr_p == NULL)))
		goto err_out0;
	dsp_vq				    = hd_malloc_nocache(&dsp_vq, jpeg_para.display_buffer_size);
	if((NULL == dsp_vq) || ((dsp_vq) && (dsp_vq->usr_p == NULL)))
		goto err_out0;
	jpeg_para.frame_buffer_address	    = (char *)(((int)fb_vq->kernel_p / 1024 + 1) * 1024);
	jpeg_para.display_buffer_address	    = dsp_vq->kernel_p;
#else
	fb_buf = GxCore_Mallocz(jpeg_para.frame_buffer_size + 8);
	if(NULL == fb_buf)
		goto err_out0;
	dsp_buf = GxCore_Mallocz(jpeg_para.display_buffer_size);
	if(NULL == dsp_buf)
		goto err_out0;
	jpeg_para.frame_buffer_address	    = fb_buf;
	jpeg_para.display_buffer_address	    = dsp_buf;
#endif /*NO_OS*/
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Config,&jpeg_para,sizeof(GxJpegProperty_Config));

	GxAVLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo, 0, GXAV_PIN_INPUT);

	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Run, NULL, 0);
	s_jpeg_state = GUI_TRUE;

	for(;size>0;) {
		signed int send  = 0;
		unsigned char* ptr = NULL;
		if(!s_jpeg_state)
			goto err_out0;
		if (size > JPEG_MAX_TRUNK)
			send = JPEG_MAX_TRUNK;
		else
			send = size;

		memset(src, 0, JPEG_MAX_TRUNK);
		ret = GxCore_Read(hfile, src, 1, send);
		ptr = src;
		if(0 >= ret){
			_set_exception("Read jpeg data err!");
			goto err_out0;
		}
		ret = 0;
		i = 0;
		while(send > 0)
		{
			if(i == 20) {
				goto err_out0;
			}

			i++;
			ret = GxAVFifoWriteData(g_device_handle, jpeg_fifo, ptr, send, 1000000);
			if(ret != send)
			{
				gxlogd("[JPEG] Force Write!\n");
				GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_WriteData, NULL, 0);
			}

			GxAVGetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Status,&status,sizeof(GxJpegProperty_Status));
			if(status.status & STANDARD_UNSUPPORT){
				_set_exception("Format cannot be supported!");
				goto err_out0;
			}
			if(status.status & DECODER_ERROR){
				_set_exception("Decoder error!");
				goto err_out0;
			}
			if(status.status & DECODER_FRAME_ERROR)
			{
				_set_exception("Frame buffer error!");
				goto err_out0;
			}
			if(status.status & DECODER_OVER){
				goto OVER;
			}

			send -= ret;
			size -= ret;
			ptr += ret;
		}
		GxAVGetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Status,&status,sizeof(GxJpegProperty_Status));
		if(status.status & STANDARD_UNSUPPORT){
			_set_exception("Format cannot be supported!");
			goto err_out0;
		}
		if(status.status & DECODER_ERROR){
			_set_exception("Decoder error!");
			goto err_out0;
		}
		if(status.status & DECODER_FRAME_ERROR)
		{
			_set_exception("Frame buffer error!");
			goto err_out0;
		}
		if(status.status & DECODER_OVER){
			goto OVER;
		}
	}

OVER:
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_End, NULL, 0);

	GxAVWaitEvents(g_device_handle, jpeg_handle, EVENT_JPEG_DECOVER, 1000000, &event);
	if(!(event & EVENT_JPEG_DECOVER))
		GxAVWaitEvents(g_device_handle, jpeg_handle, EVENT_JPEG_UNSUPPORT, 1000000, &event);

	if(event & EVENT_JPEG_DECOVER)
		status.status |= DECODER_OVER;

	ret = GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Update, NULL, 0);
	if(ret < 0){
		_set_exception("Update error!");
		goto err_out0;
	}
	GxAVGetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Create, &config, sizeof(GxJpegProperty_Create));

	ret = GxAVGetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Info, &info, sizeof(GxJpegProperty_Info));
	if(ret < 0){
		_set_exception("Get information error!");
		goto err_out0;
	}

	if (width < 4)
		width = 4;
	else if(width % 4)
		width = (width >> 2) << 2;

	rect.x = rect.y = 0;

	if (img_desc == NULL)
	{
		img_desc = gal_img_new();
		if(NULL == img_desc){
#ifndef NO_OS
			if(fb_vq)
			{
				hd_free(fb_vq);
				GUI_FREE(fb_vq);
				fb_vq = NULL;
			}
			if(dsp_vq)
			{
				hd_free(dsp_vq);
				GUI_FREE(dsp_vq);
				dsp_vq = NULL;
			}
#else
			if(fb_buf)
			{
				GxCore_Free(fb_buf);
			}

			if(dsp_buf)
			{
				GxCore_Free(dsp_buf);
			}
#endif /*NO_OS*/
			goto err_out0;
		}
	}

	if(path)
	{
	    filename = GxCore_Strdup(path);
	    GUI_FREE(img_desc->filename);
	    img_desc->filename = filename;
	}
	img_desc->type     = JPEG_TYPE;
	img_desc->bpp      = 16;
	img_desc->effect   = EFFECT_COPY;

	if(config.enable_interlace)
	{
		rect.w = info.width;
		rect.h = info.height;

		img_desc->width    = info.width;
		img_desc->height   = info.height;
		img_desc->size	   = info.width * info.height * 2;
#ifndef NO_OS
		mem_buf = dsp_vq->usr_p;
#else
		mem_buf = dsp_buf;
#endif /*NO_OS*/
	}
	else
	{
		rect.w = width;
		rect.h = height;

		adjust_width = info.width;
		if(adjust_width > (info.original_width / weight))
		{
			adjust_width = info.original_width / weight;
			adjust_width = (adjust_width / 2) * 2;
		}
		if (adjust_width < 4)
			adjust_width = 4;

		hd_jpeg_zoom(width,
				height,
				adjust_width,
				info.height,
				jpeg_para.stride,
				config.height,
				info.source_format,
				(void *)info.fb_y_addrs,
				(void *)info.fb_cb_addrs,
				(void *)info.fb_cr_addrs,
				jpeg_para.display_buffer_address);

		img_desc->width    = width;
		img_desc->height   = height;
		img_desc->size	   = width * height * 2;
#ifndef NO_OS
		mem_buf = fb_getmmap(&(gui.fb), (int)jpeg_para.frame_buffer_address);
#else
		mem_buf = fb_buf;
#endif /*NO_OS*/
	}

	data		   = gal_img_alloc_memory(img_desc);
	if(data)
	{
		memcpy(data, mem_buf, img_desc->size);
	}
	else
	{
		gal_img_release(img_desc);
		return (NULL);
	}
	img_desc->fmt = GX_COLOR_FMT_YCBCR422;

	destroy_config = config;
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);
	GxAVUnLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo);
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Destroy,&destroy_config,sizeof(GxJpegProperty_Destroy));

err_out0:
	GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);
	GxAVUnLinkFifo(g_device_handle, jpeg_handle, jpeg_fifo);
	ret = GxAVFifoDestroy(g_device_handle, jpeg_fifo);
	jpeg_fifo = 0;
#ifndef NO_OS
	hd_free(fifo_vq);
	GUI_FREE(fifo_vq);
	fifo_vq = NULL;
#else
	GxCore_Free(fifo_buf);
	fifo_buf = NULL;
#endif /*NO_OS*/
	destroy_config = config;
	GxAVSetProperty(g_device_handle,jpeg_handle,GxJPEGPropertyID_Destroy,&destroy_config,sizeof(GxJpegProperty_Destroy));
err_out:
	if(!(status.status & DECODER_OVER))
		gxlogd("[GUI] [JPEG]:%s\n",_get_exception_str());
	if (psrc)
		GUI_FREE(psrc);

	GxCore_Close(hfile);

#ifndef NO_OS
	if(fb_vq)
	{
		hd_free(fb_vq);
		GUI_FREE(fb_vq);
		fb_vq = NULL;
	}
	if(dsp_vq)
	{
		hd_free(dsp_vq);
		GUI_FREE(dsp_vq);
		dsp_vq = NULL;
	}
#else
	if(fb_buf)
		GxCore_Free(fb_buf);
	if(dsp_buf)
		GxCore_Free(dsp_buf);
#endif /*NO_OS*/
	return (img_desc);
}

GuiSurface *jpeg_surface(const char *path,
		int original_width,
		int original_height,
		int width,
		int height)
{
	image_desc *img_desc = NULL;
	GuiSurface *dst = NULL;

	img_desc = jpeg_image_descriptor(path, img_desc, original_width, original_height, width, height);

	if(img_desc) {
		dst = image_desc_surface(img_desc);
		gal_img_release(img_desc);
	}
		
	return dst;
}

static unsigned int jpeg_check(const char *path)
{
	unsigned int ret = GUI_TRUE;
	FILE * fp = NULL;
	unsigned char data[2];
	unsigned int  app2_time=1;
	unsigned short  short_data;
	unsigned char char_data;
	unsigned int int_data;
	unsigned int id_data[3];
	unsigned char Nf;
	unsigned char Ns;
	unsigned char i;
	unsigned char H[3],V[3];
	unsigned int app_length;
	unsigned int color_space = 0;
	unsigned int is_progressive = 0;
	unsigned int id_of_sos = 0;

	if(NULL == path)
		return GUI_FALSE;

	fp = fopen(path, "r");
	if(NULL == fp)
		return GUI_FALSE;

	memset(data,0x00,sizeof(data));
	if(!fread(data, 1, 2, fp)) {
		fclose(fp);
		return GUI_TRUE;
	}
	if((0xFF != data[0]) || (0xD8 != data[1])) {
		fclose(fp);
		return GUI_FALSE;
	}

	while(1) {
		data[1]=data[0];
		if(!fread(&data[0], 1, 1, fp)) {
			fclose(fp);
			return GUI_TRUE;
		}

		if((data[1]==0xFF)&(data[0]>=0xE0)&(data[0]<=0xEF)) {
			fread(&char_data, 1, 1, fp);
			short_data = char_data<<8;
			fread(&char_data, 1, 1, fp);
			app_length = (char_data) + short_data;

			fread(&id_data[0], 1 ,4, fp);
			fread(&id_data[1], 1 ,4, fp);
			fread(&id_data[2], 1 ,4, fp);

			if(app2_time && (id_data[0] == 0x5f434349)&&(id_data[1] ==  0x464f5250)&&(id_data[2] == 0x454c49)) {
				fread(&short_data, 1 ,2, fp);
				fread(&int_data, 1 ,4, fp);
				fread(&int_data, 1 ,4, fp);
				fread(&int_data, 1 ,4, fp);
				fread(&int_data, 1 ,4, fp);
				fread(&int_data, 1 ,4, fp);

				if(int_data == 0x4b594d43)
					color_space = 1;

				app_length = app_length - 14 -22;
				app2_time = 0;
			} else {
				app_length = app_length -14;
			}

			fseek(fp,app_length, 1);
			fread(&data[0], 1, 1, fp);
		}

		if((data[1]==0xFF)&((data[0]==0xC2)|| (data[0]==0xC0))) {

			if(data[0] == 0xC2) {
				is_progressive = 1;
			} else {
				is_progressive = 0;
			}

			fread(&char_data, 1, 1, fp);
			short_data = char_data<<8;

			fread(&char_data, 1, 1, fp);
			short_data = (char_data) + short_data; //Lf

			fread(&char_data, 1, 1, fp);           //P

			fread(&char_data, 1, 1, fp);
			short_data = char_data<<8;

			fread(&char_data, 1, 1, fp);
			short_data = (char_data) + short_data;//Y

			fread(&char_data, 1, 1, fp);
			short_data = char_data<<8;

			fread(&char_data, 1, 1, fp);
			short_data = (char_data) + short_data;//X

			fread(&char_data, 1, 1, fp);
			Nf = char_data;                       //Nf

			for(i=0; i<Nf;i++) {
				fread(&char_data, 1, 1, fp);
				if(0 == char_data) {
					fclose(fp);
					return GUI_FALSE;
				}
				fread(&char_data, 1, 1, fp);

				H[i] = char_data & 0xF;
				V[i] = (char_data>>4) & 0xF;

				fread(&char_data, 1, 1, fp) ;

				if(H[i] >2 || V[i] >2 || color_space  ) {
					if(color_space)
						gxlogi("[Hardware Unsupport] CMYK format.\n");

					fclose(fp);
					return GUI_FALSE;
				}
			}

			//fclose(fp);
			//return ret;
		}
        // find first sos is one component and pic is progressive
		if( (data[1]==0xFF) & (data[0]==0xDA) ) {
			id_of_sos++;

			fread(&char_data, 1, 1, fp);
			short_data = char_data<<8;

			fread(&char_data, 1, 1, fp);
			short_data = (char_data) + short_data; //Ls

			fread(&char_data, 1, 1, fp);
			Ns = char_data;                        //Ns

			if( (Nf>1) && is_progressive && ( (id_of_sos==1) && (Ns==1) ) ) {
				gxlogi("[Hardware Unsupport] SOF2 first SOS'Ns is 1.\n");
				fclose(fp);
				return GUI_FALSE;
			}
			fclose(fp);
			return ret;
		}
        //end
	}
	fclose(fp);
	return ret;

}

static bool s_gray_levels = GUI_FALSE;
void process_gray_levels(image_desc *img_desc)
{
	int i = 0;
	char *data = NULL;

	if(GUI_FALSE == s_gray_levels) {
		return;
	}

	if(NULL == img_desc) {
		return;
	}

	data = (char *)img_desc->data;
	for(i = 0; i < img_desc->size; i++) {
		if(i % 2) {
			// Y
			data[i] = 16 + data[i] * 219 / 255;
		} else {
			// Cb、Cr
			data[i] = 128 + (data[i] - 128) * 224 / 255 ;
		}
	}
}

void IMG_GrayLevelEnable(bool flag)
{
	s_gray_levels = flag;
}

extern int IMG_JPEG_GetInfo(const char *pFile, Image_Exif *image_info);

static image_desc* jpeg_decode(image_desc *img_desc, const char *pFile)
{
	GuiCore *gui_core = NULL;
	Image_Exif exif;
	unsigned int zoom_width = 0, zoom_height = 0;

	gui_core = GUI_GetCurrent();
	if(0 == g_device_handle)
	{
#ifndef NO_OS
		g_device_handle = GxAvdev_CreateDevice(0);
#else
		g_device_handle = GxAVCreateDevice(0);
#endif /*NO_OS*/
	}

	if(0 == jpeg_handle) {
#ifndef NO_OS
		jpeg_handle = GxAvdev_OpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
#else
		jpeg_handle = GxAVOpenModule(g_device_handle, GXAV_MOD_JPEG_DECODER, 0);
#endif /*NO_OS*/
		if(0 == jpeg_handle)
			return (NULL);
	}

	if(GUI_FALSE == jpeg_check(pFile))
		return NULL;

	memset(&exif, 0, sizeof(Image_Exif));
	//jpeg_get_info(pFile, &exif);
	IMG_JPEG_GetInfo(pFile, &exif);
	if((0 == exif.width) || (0 == exif.height))
	{
		return (NULL);
	}

	if((exif.width > gui_core->config.width) ||
			(exif.height > gui_core->config.height))
	{
		zoom_width  = exif.width;
		zoom_height = exif.height;
		if(exif.width > gui_core->config.width)
		{
			zoom_width = gui_core->config.width;
			zoom_height = exif.height * gui_core->config.width / exif.width;
		}

		if(zoom_height > gui_core->config.height)
		{
			zoom_width = zoom_width * gui_core->config.height / zoom_height;
			zoom_height = gui_core->config.height;
		}

		zoom_width = ((zoom_width) >> 3) << 3;
		zoom_height = ((zoom_height) >> 1) << 1;
	}
	else
		zoom_width = zoom_height = 0;
	if(0 == zoom_width)
		zoom_width = exif.width;
	if(0 == zoom_height)
		zoom_height = exif.height;
	img_desc = jpeg_image_descriptor(pFile,
			img_desc,
			exif.width,
			exif.height,
			zoom_width,
			zoom_height);
	if(NULL == img_desc)
		return NULL;

	process_gray_levels(img_desc);
	return img_desc;
}

image_desc *hd_jpeg_load(image_desc *img_desc, const char *pFile)
{
	if( pFile == NULL)
		return (NULL);

	return jpeg_decode(img_desc, pFile);
}

#ifndef NO_OS
int hd_img_desc_gradiant(GuiSurface *surface, const image_desc *pDesc, int x, int y, int w, int h, ImagePlayMode mode)
{
	int ret = 0, steps = 0, i = 0, j = 0, step_width = 0, step_height = 0;
	GuiSurface *tmp_surface = NULL;
	GAL_Rect src_rect = {0}, step_rect = {0};

	if((NULL == surface) || (NULL == pDesc))
		return (-1);

	if(IMAGE_EFFECT_NONE == mode) {
		ret  = hd_img_desc(surface, pDesc, x, y, w, h);
		ret |= gdi_commit();
		ret |= hd_flip();
	} else {
		src_rect.x = src_rect.y = 0;
		src_rect.w = surface->sf.width;
		src_rect.h = surface->sf.height;
		tmp_surface = hd_get_surface0(&src_rect, 16, surface->sf.color_format, NULL, GUI_FALSE);
		if(NULL == tmp_surface)
			return (-1);
		gdi_begin();
		if(GX_COLOR_FMT_YCBCR422 == surface->sf.color_format)
			hd_fillrect(tmp_surface, &src_rect, 0x108080);
		else
			hd_fillrect(tmp_surface, &src_rect, 0);
		hd_img_desc(tmp_surface, pDesc, x, y, w, h);
		gdi_end();
		gal_enable_img(GUI_TRUE);
		hd_enable_video(GUI_FALSE);
		ret |= hd_copy_surface_gradiant(tmp_surface, &src_rect, surface, &src_rect, mode, 64);
		hd_free_surface(tmp_surface);
	}

	return (ret);
}

int hd_jpeg_decode(GuiSurface *surface, const char *pJPEG, int x0, int y0, int screen_width, int screen_height)
{
	GuiCore *gui_core = NULL;
	image_desc *img_desc = NULL;
	int x_pos = 0, y_pos = 0, width = 0, height = 0;

	if( pJPEG == NULL)
		return 1;

	s_jpeg_state = GUI_TRUE;
	img_desc = jpeg_decode(NULL, pJPEG);
	gui_core = GUI_GetCurrent();

	if(NULL == img_desc){
		if(!s_jpeg_state){
			widget_set_update(stack_peek(gui_core->win_stack), GUI_TRUE);
			hd_flip();
			return (0);
		}
		else{
			hd_img_clear();
		}

		s_jpeg_state = 1;
		return (1);
	}

	width = img_desc->width;
	height = img_desc->height;

	if((width > gui_core->config.width) ||	(height > gui_core->config.height)){
		gal_img_release(img_desc);
		return (1);
	}

	if(width && height){
		if(width < screen_width){
			x_pos = x0 + (screen_width - width) / 2;
			x_pos = x_pos - (x_pos % 2);
		}

		if(height < screen_height){
			y_pos =y0 + (screen_height - height) / 2;
			y_pos = y_pos - (y_pos % 2);
		}
	}

	if(s_jpeg_state){
		hd_img_desc_gradiant(surface, img_desc, x_pos, y_pos, img_desc->width, img_desc->height, gui_core->config.play_image_mode);
		hd_flip();
	}

	if(!s_jpeg_state){
		widget_set_update(stack_peek(gui_core->win_stack), GUI_TRUE);
		hd_flip();
	}

	gal_img_release(img_desc);

	s_jpeg_state = 1;

	return (0);
}
#endif /*NO_OS*/

int hd_jpeg_stop(void)
{
	int ret = 0;

	s_jpeg_state = 0;
	ret = GxAVSetProperty(g_device_handle, jpeg_handle, GxJPEGPropertyID_Stop, NULL, 0);
	if(0 != ret)
		gxlogd("[GUI]Stop failed!\n");
	return (ret);
}

