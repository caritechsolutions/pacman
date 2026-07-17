#include "app.h"
#include "module/app_dir_module.h"
#include "bus/gui_core/gdi_core.h"
#include "module/app_log.h"
#include "app_capture_video_frame.h"
#include <stdbool.h>
#include "app_face_detect.h"

#if PVR_RECORDING_POSTER_STYLE
#define CAPTURE_VPP_SUPPORT 1
#else
#define CAPTURE_VPP_SUPPORT 0
#endif

#if CAPTURE_VPP_SUPPORT
#ifdef ECOS_OS
#define CAPTURE_TEMP_PATH "/mnt/capture.bmp"
#else
#define CAPTURE_TEMP_PATH "/tmp/capture.bmp"
#endif

#define ZOOM_RECT_WIDTH (292)
#define ZOOM_RECT_HEIGHT (164)

//#define ZOOM_COST_TIME_STATISTICS

typedef struct {
    char *logo_path;
    event_list *timer;
} CaptureVppCtrl;

static CaptureVppCtrl s_capture_vpp_ctrl;

int app_av_get_rect(GAL_Rect *rect)
{
    int vout_mode = 0;
    int ret = 0;

    if(!rect)
        return -1;

    memset(rect, 0, sizeof(GAL_Rect));
    GxBus_ConfigGetInt(TV_STANDARD_KEY, &vout_mode, TV_STANDARD);

    switch(vout_mode) {
#ifdef SUPPORT_480_I_P_OUTPUT
        case GX_VIDEO_OUTPUT_480I:
        case GX_VIDEO_OUTPUT_480P:
            rect->w = 720;
            rect->h = 480;
            break;
#endif
        case GX_VIDEO_OUTPUT_576I:
        case GX_VIDEO_OUTPUT_576P:
            rect->w = 720;
            rect->h = 576;
            break;
        case GX_VIDEO_OUTPUT_720P_50HZ:
        case GX_VIDEO_OUTPUT_720P_60HZ:
            rect->w = 1280;
            rect->h = 720;
            break;
        case GX_VIDEO_OUTPUT_1080I_50HZ:
        case GX_VIDEO_OUTPUT_1080I_60HZ:
            rect->w = 1920;
            rect->h = 1080;
            break;
#ifdef SUPPORT_1080P_OUTPUT
        case GX_VIDEO_OUTPUT_1080P_50HZ:
        case GX_VIDEO_OUTPUT_1080P_60HZ:
            rect->w = 1920;
            rect->h = 1080;
            break;
#endif
        default:
            ret = -1;
            break;
    }

    return ret;
}

static int _mini_memory_capture_vpp(void)
{
    GuiSurface *surface = NULL;
    CaptureVppCtrl *cvc = &s_capture_vpp_ctrl;
    int vpu_handle = 0, av_device_handle = 0;
    GAL_Rect rect = { 0 }, dst_rect = { 0 };
#ifdef ZOOM_COST_TIME_STATISTICS
    uint32_t m1 = 0, m2 = 0, m3 = 0;
#endif

    if(NULL == cvc->logo_path)
        return -1;

#ifdef ZOOM_COST_TIME_STATISTICS
    m1 = app_tick_time_msec_get();
#endif

    av_device_handle = GxAvdev_CreateDevice(0);
    if(av_device_handle <= 0)
        return -1;

    vpu_handle = GxAvdev_OpenModule(av_device_handle, GXAV_MOD_VPU, 0);
    if(0 == vpu_handle) {
        app_log_error("\033[31mopen vpu module failed!\033[0m\n");
        goto err;
    }

    if(GxAvdev_LayerEnable(av_device_handle, vpu_handle, GX_LAYER_OSD, 0) != 0) {
        app_log_error("\033[31mdisable OSD layer failed!\033[0m\n");
        goto err;
    }

    GxCore_ThreadDelay(40);
    rect.w = 720;
    rect.h = 576;
    dst_rect.w = 292;
    dst_rect.h = 166;

#ifdef ZOOM_COST_TIME_STATISTICS
    m2 = app_tick_time_msec_get();
#endif
    surface = gal_capture_surface_sd(&rect, &dst_rect);
    if(!surface) {
        app_log_error("\033[31mgal_capture_surface failed!\033[0m\n");
        goto err;
    }

    if(GxAvdev_LayerEnable(av_device_handle, vpu_handle, GX_LAYER_OSD, 1) != 0) {
        app_log_error("\033[31menable OSD layer failed!\033[0m\n");
        goto err;
    }

    gal_save_surface_to_bmp(surface, (const unsigned char *)CAPTURE_TEMP_PATH);
    hd_free_surface(surface);

    GxAvdev_CloseModule(av_device_handle, vpu_handle);
    GxAvdev_DestroyDevice(av_device_handle);

#ifdef ZOOM_COST_TIME_STATISTICS
    m3 = app_tick_time_msec_get();
    app_log_info("\033[31mgal_capture_surface cost %u + %u ms!\033[0m\n", m2 - m1, m3 - m2);
#endif
    return 0;
err:
    hd_free_surface(surface);
    if(vpu_handle > 0 && av_device_handle > 0)
        GxAvdev_CloseModule(av_device_handle, vpu_handle);
    if(av_device_handle > 0)
        GxAvdev_DestroyDevice(av_device_handle);
    return -1;
}

static int _large_memory_capture_vpp(void)
{
#if FACE_DETECT_SUPPORT
    return app_face_detect_start(CAPTURE_TEMP_PATH, ZOOM_RECT_WIDTH, ZOOM_RECT_HEIGHT);
#else
    return app_caputre_video_frame(true, CAPTURE_TEMP_PATH, ZOOM_RECT_WIDTH, ZOOM_RECT_HEIGHT);
#endif
}

static int _capture_vpp_timer_exec(void *userdata)
{
    int ret = -1;
    CaptureVppCtrl *cvc = &s_capture_vpp_ctrl;

    APP_TIMER_REMOVE(cvc->timer);

    if(app_get_ddr_size() < 128 * SIZE_UNIT_M)
        ret = _mini_memory_capture_vpp();
    else
        ret = _large_memory_capture_vpp();

    return ret;
}

int app_capture_vpp_start(const char *path, uint32_t sec)
{
    CaptureVppCtrl *cvc = &s_capture_vpp_ctrl;
    GAL_Rect rect = { 0 };

    if(path == NULL)
        return -1;

    if(app_av_get_rect(&rect) < 0)
        return -1;

    if((cvc->logo_path = GxCore_Strdup(path)) == NULL) {
        return -1;
    }

    if(sec > 0) {
        APP_TIMER_ADD(cvc->timer, _capture_vpp_timer_exec, sec * 1000, TIMER_REPEAT);
    } else {
        _capture_vpp_timer_exec(NULL);
    }

    return 0;
}

static void _video_capture_check(void)
{
    CaptureVppCtrl *cvc = &s_capture_vpp_ctrl;

    APP_TIMER_REMOVE(cvc->timer);
    if(app_get_ddr_size() < 128 * SIZE_UNIT_M) {
        _mini_memory_capture_vpp();
    } else {
        app_caputre_video_frame(false, CAPTURE_TEMP_PATH, ZOOM_RECT_WIDTH, ZOOM_RECT_HEIGHT);
    }
}

int app_capture_vpp_stop(void)
{
    CaptureVppCtrl *cvc = &s_capture_vpp_ctrl;

    // if capture is aysnc, wait for thread exit first
#if FACE_DETECT_SUPPORT
    app_face_detect_stop();
#else
    app_capture_video_frame_stop();
#endif
    if (cvc->timer) {
        _video_capture_check();
    }

    if(cvc->logo_path != NULL && GxCore_FileExists(CAPTURE_TEMP_PATH) == GXCORE_FILE_EXIST) {
        app_copy_file_to_file(CAPTURE_TEMP_PATH, cvc->logo_path, 8192);
    }
    APP_FREE(cvc->logo_path);
    GxCore_FileDelete(CAPTURE_TEMP_PATH);

    return 0;
}

#else
int app_capture_vpp_start(const char *path, uint32_t sec)
{
    return 0;
}

int app_capture_vpp_stop(void)
{
    return 0;
}
#endif
