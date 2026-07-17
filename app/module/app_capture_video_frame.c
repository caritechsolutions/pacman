#include "gdi.h"
#include "app_module.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

#define MAX_RESOLUTION_WIDTH (1920)
#define MAX_RESOLUTION_HEIGHT (1080)

// #define ZOOM_BY_GA
#define ZOOM_SIMPLE_ALG
//#define ZOOM_COST_TIME_STATISTICS

#if ((defined CANOPUS || defined VEGA) && (defined LINUX_OS))
#define OMP_SUPPORT
#endif

#if (defined(ECOS_OS) && !defined(ZOOM_SIMPLE_ALG))
/**
 * 自定义的向上取整函数，替代标准库的 ceilf
 *
 * @param x 输入的浮点数
 * @return 大于或等于x的最小整数值（作为float返回）
 */
static float ceilf(float x) {
    // 处理特殊情况
    if (x == 0.0f || x != x) { // x == 0 或 x 是 NaN
        return x;
    }

    // 获取整数部分
    int integer_part = (int)x;

    // 如果 x 已经是整数，直接返回
    if (x == (float)integer_part) {
        return x;
    }

    // 如果 x 是负数，向上取整就是整数部分
    // 如果 x 是正数，向上取整是整数部分加1
    if (x < 0.0f) {
        return (float)integer_part;
    } else {
        return (float)(integer_part + 1);
    }
}
#endif

typedef struct {
    int device_handle;
    int vpu_handle;
    GxHwMallocObj buf;
    char *path;  // path for save capture file
    int width;
    int height;
    int thread_id;
    bool inited;
} CaptureVideoFrameCtrl;

static CaptureVideoFrameCtrl s_capture_video_frame_ctrl = {
    .device_handle = 0,
    .vpu_handle = 0,
    .buf = {
        .usr_p = NULL,
        .kernel_p = NULL,
        .size = 0,
    },
    .path = NULL,
    .width = 0,
    .height = 0,
    .thread_id = 0,
};

static int _capture_device_and_buf_init(void);

GxVpuProperty_CreateSurface *app_create_capture_surface(GXGDI_Rect *rect, GxColorFormat format, unsigned int color_depth)
{
    uint32_t size = 0;
    unsigned char *temp_buffer = NULL;
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

    if(NULL == rect)
        return NULL;

    _capture_device_and_buf_init();  // ensure capture deivce open and hw buf malloc before use

    if(GX_COLOR_FMT_YCBCR422 == format)
        size = rect->w * rect->h * 2;
    else if(GX_COLOR_FMT_YCBCR420_Y_UV == format)
        size = rect->w * rect->h + rect->w * rect->h / 2;
    else
        return NULL;

    GxVpuProperty_CreateSurface *CreateSurface = GxCore_Calloc(1, sizeof(GxVpuProperty_CreateSurface));
    if(NULL == CreateSurface) {
        app_log_error("create GxVpuProperty_CreateSurface failed\n");
        return NULL;
    }

    CreateSurface->format = format;
    CreateSurface->width = rect->w;
    CreateSurface->height = rect->h;
    CreateSurface->mode = GX_SURFACE_MODE_IMAGE;
    CreateSurface->buffer = NULL;
    CreateSurface->ext_param.colordepth = color_depth;

    if(ctrl->buf.usr_p) {
        memset(ctrl->buf.usr_p, 0, ctrl->buf.size);
        CreateSurface->buffer = ctrl->buf.kernel_p;
    }

    if(GxAVGetProperty(ctrl->device_handle, ctrl->vpu_handle, GxVpuPropertyID_CreateSurface, CreateSurface, sizeof(GxVpuProperty_CreateSurface)) < 0) {
        app_log_error("Create surface error\n");
        GxCore_Free(CreateSurface);
        return NULL;
    }

    if(!ctrl->buf.usr_p) {
        temp_buffer = (uint8_t *)GxCore_Map(ctrl->device_handle, (uint32_t)CreateSurface->buffer, size);
        memset(temp_buffer, 0, size);
        GxCore_UnMap(ctrl->device_handle, temp_buffer, size);
    }

    return CreateSurface;
}

int app_capture_video_layer(GxVpuProperty_CreateSurface *CreateSurface)
{
    int ret = 0;
    GxVpuProperty_LayerCapture Capture = { 0 };
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

    if(NULL == CreateSurface) {
        return GXCORE_ERROR;
    }

    _capture_device_and_buf_init();  // ensure capture deivce open and hw buf malloc before use

    Capture.layer = GX_LAYER_VPP;
    Capture.surface = CreateSurface->surface;
    Capture.rect.x = 0;
    Capture.rect.y = 0;
    Capture.rect.width = CreateSurface->width;
    Capture.rect.height = CreateSurface->height;
    Capture.soft_cap = 1;
    Capture.svpu_cap = 0;

    ret = GxAVGetProperty(ctrl->device_handle, ctrl->vpu_handle, GxVpuPropertyID_LayerCapture, &Capture, sizeof(GxVpuProperty_LayerCapture));

    return ret;
}

int app_destroy_capture_surface(GxVpuProperty_CreateSurface *CreateSurface)
{
    int ret = GXCORE_ERROR;
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;
    GxVpuProperty_DestroySurface DesSurface = { 0 };

    if(NULL == CreateSurface)
        return GXCORE_ERROR;

    _capture_device_and_buf_init();  // ensure capture deivce open and hw buf malloc before use

    memset(&DesSurface, 0, sizeof(DesSurface));
    DesSurface.surface = CreateSurface->surface;
    ret = GxAVSetProperty(ctrl->device_handle, ctrl->vpu_handle, GxVpuPropertyID_DestroySurface, &DesSurface, sizeof(GxVpuProperty_DestroySurface));
    GxCore_Free(CreateSurface);
    return ret;
}

static int clamp(int value)
{
    if(value < 0)
        return 0;
    if(value > 255)
        return 255;
    return value;
}

static void _ycbcr2rgb(unsigned char y, unsigned char cb, unsigned char cr, int *r, int *g, int *b)
{
    short tcr = 0, tcb = 0;

    tcb = cb - 128;
    tcr = cr - 128;

    *r = clamp(y + (177 * tcr) / 100);
    *g = clamp(y - (71 * tcr + 34 * tcb) / 100);
    *b = clamp(y + (140 * tcb) / 100);
}

void app_convert_yuv420_to_rgb(GxVpuProperty_CreateSurface *surface, unsigned char *dst_buffer, int width, int height, int is_bgr)
{
    int i = 0, j = 0;
    int r = 0, g = 0, b = 0;
    unsigned char y = 0, u = 0, v = 0;
    unsigned char *y_buffer = NULL, *uv_buffer = NULL;
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;
    unsigned int size = width * height * 3 / 2;
    unsigned char *src_buffer = NULL;

    if(NULL == surface || NULL == dst_buffer)
        return;

    if(ctrl->buf.usr_p) {
        src_buffer = ctrl->buf.usr_p;
    } else {
        src_buffer = GxCore_Map(ctrl->device_handle, (uint32_t)surface->buffer, size);
    }

    y_buffer = src_buffer;
    uv_buffer = y_buffer + width * height;

    for(i = 0; i < height; i++) {
        for(j = 0; j < width; j++) {
            y = y_buffer[i * width + j];
            u = uv_buffer[(i / 2) * width + (j / 2) * 2];
            v = uv_buffer[(i / 2) * width + (j / 2) * 2 + 1];
            _ycbcr2rgb(y, u, v, &r, &g, &b);
            if(is_bgr) {
                dst_buffer[(i * width + j) * 3] = b;
                dst_buffer[(i * width + j) * 3 + 1] = g;
                dst_buffer[(i * width + j) * 3 + 2] = r;
            } else {
                dst_buffer[(i * width + j) * 3] = r;
                dst_buffer[(i * width + j) * 3 + 1] = g;
                dst_buffer[(i * width + j) * 3 + 2] = b;
            }
        }
    }

    if(!ctrl->buf.usr_p) {
        GxCore_UnMap(ctrl->device_handle, src_buffer, size);
    }
    return;
}

void app_save_bgr_data_as_bmp(char *temp_path, unsigned char *bgr, int width, int height)
{
    unsigned char *temp_file = NULL;
    FILE *file = NULL;
    unsigned char *fill = NULL;
    unsigned short word = 0;
    unsigned int size = 0, dword = 0, i = 0, offset = 0;

    if((NULL == temp_path) || (NULL == bgr))
        return;

    temp_file = (unsigned char *)temp_path;

    file = fopen((char *)temp_file, "w");
    if(NULL == file)
        return;

    fwrite("BM", 2, 1, file);
    offset = (((width * 24) + 31) >> 5) << 2;
    size = 0x36 + offset * height;
    fwrite(&size, 4, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);
    dword = 0x36;
    fwrite(&dword, 4, 1, file);
    dword = 0x28;
    fwrite(&dword, 4, 1, file);
    dword = width;
    fwrite(&dword, 4, 1, file);
    dword = height;
    fwrite(&dword, 4, 1, file);
    word = 1;
    fwrite(&word, 2, 1, file);
    word = 24;
    fwrite(&word, 2, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);
    dword = (((width * 24) + 31) >> 5) << 2;
    dword = dword * height;
    fwrite(&dword, 4, 1, file);
    dword = width;
    fwrite(&dword, 4, 1, file);
    dword = height;
    fwrite(&dword, 4, 1, file);
    dword = 2 << 24;
    fwrite(&dword, 4, 1, file);
    dword = 0;
    fwrite(&dword, 4, 1, file);

    i = 0;
    offset = (((width * 24) + 31) >> 5) << 2;
    offset = offset - width * 3;
    if(offset) {
        fill = (unsigned char *)GxCore_Malloc(offset);
        if(NULL == fill) {
            fclose(file);
            return;
        }
        memset(fill, 0xFF, offset);
    }
    while(i < height) {
        fwrite(bgr + (width * 3) * (height - i - 1), width * 3, 1, file);
        if(offset)
            fwrite(fill, offset, 1, file);
        i++;
    }
    GxCore_Free(fill);

    fclose(file);
}

#ifdef ZOOM_SIMPLE_ALG
/**
 * 简化版BGR24图像缩放函数 - 使用最近邻插值算法
 * 当对性能要求极高或在资源极为受限的环境中使用
 *
 * @param src_data      BGR24格式的源图像数据
 * @param src_width     源图像宽度
 * @param src_height    源图像高度
 * @param dst_data      目标图像缓冲区，若为NULL则函数会分配内存
 * @param dst_width     目标图像宽度
 * @param dst_height    目标图像高度
 * @return              如果dst_data为NULL，返回新分配的内存，否则返回dst_data。失败时返回NULL
 */
static unsigned char *scale_bgr24_image_nearest(const unsigned char *src_data, int src_width, int src_height, unsigned char *dst_data, int dst_width, int dst_height)
{
    int x = 0, y = 0;
    // 参数验证
    if(!src_data || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
        return NULL;
    }

    // 分配输出缓冲区（如果没有提供）
    if(dst_data == NULL) {
        dst_data = (unsigned char *)GxCore_Malloc((size_t)dst_width * dst_height * 3);
        if(!dst_data) {
            return NULL;  // 内存分配失败
        }
    }

    // 相同尺寸直接复制
    if(src_width == dst_width && src_height == dst_height) {
        memcpy(dst_data, src_data, (size_t)src_width * src_height * 3);
        return dst_data;
    }

    // 计算缩放比例
    const float x_ratio = (float)src_width / dst_width;
    const float y_ratio = (float)src_height / dst_height;

// 对每个目标像素进行处理
#ifdef OMP_SUPPORT
// 使用OpenMP并行处理
#pragma omp parallel for collapse(2) if(dst_width * dst_height > 100000)
#endif
    for(y = 0; y < dst_height; y++) {
        for(x = 0; x < dst_width; x++) {
            // 计算对应的源像素位置
            const int src_x = (int)(x * x_ratio);
            const int src_y = (int)(y * y_ratio);

            // 计算源像素和目标像素的索引
            const int src_idx = (src_y * src_width + src_x) * 3;
            const int dst_idx = (y * dst_width + x) * 3;

            // 复制像素值
            dst_data[dst_idx] = src_data[src_idx];          // B
            dst_data[dst_idx + 1] = src_data[src_idx + 1];  // G
            dst_data[dst_idx + 2] = src_data[src_idx + 2];  // R
        }
    }

    return dst_data;
}
#else

/**
 * BGR24图像缩放函数 - 使用双线性插值算法
 *
 * @param src_data      BGR24格式的源图像数据
 * @param src_width     源图像宽度
 * @param src_height    源图像高度
 * @param dst_data      目标图像缓冲区，若为NULL则函数会分配内存
 * @param dst_width     目标图像宽度
 * @param dst_height    目标图像高度
 * @return              如果dst_data为NULL，返回新分配的内存，否则返回dst_data。失败时返回NULL
 */
static unsigned char *scale_bgr24_image(const unsigned char *src_data, int src_width, int src_height, unsigned char *dst_data, int dst_width, int dst_height)
{
    int x, y, sx, sy;

    // 参数验证
    if(!src_data || src_width <= 0 || src_height <= 0 || dst_width <= 0 || dst_height <= 0) {
        return NULL;
    }

    // 分配输出缓冲区（如果没有提供）
    bool allocated_memory = false;
    if(dst_data == NULL) {
        dst_data = (unsigned char *)GxCore_Malloc((size_t)dst_width * dst_height * 3);
        if(!dst_data) {
            return NULL;  // 内存分配失败
        }
        allocated_memory = true;
    }

    // 选择最合适的缩放方法
    // 1. 如果是缩小图像，使用区域采样以获得更好的抗锯齿效果
    // 2. 如果是放大图像，使用双线性插值以获得平滑结果
    // 3. 如果是相同尺寸，直接复制

    // 直接复制
    if(src_width == dst_width && src_height == dst_height) {
        memcpy(dst_data, src_data, (size_t)src_width * src_height * 3);
        return dst_data;
    }

    // 计算缩放比例
    const float x_ratio = src_width > 1 ? ((float)(src_width - 1)) / (dst_width - 1) : 0;
    const float y_ratio = src_height > 1 ? ((float)(src_height - 1)) / (dst_height - 1) : 0;

    // 缩小图像 - 使用区域采样
    if(dst_width < src_width && dst_height < src_height) {
        const float x_scale = (float)src_width / dst_width;
        const float y_scale = (float)src_height / dst_height;

        // 每个目标像素周围的采样区域（向上取整）
        const int sample_width = (int)ceilf(x_scale);
        const int sample_height = (int)ceilf(y_scale);

// 对每个目标像素进行处理
#ifdef OMP_SUPPORT
// 使用OpenMP并行处理
#pragma omp parallel for collapse(2) if(dst_width * dst_height > 100000)
#endif
        for(y = 0; y < dst_height; y++) {
            for(x = 0; x < dst_width; x++) {
                // 目标像素对应的原图区域中心
                const int src_x_center = (int)(x * x_scale);
                const int src_y_center = (int)(y * y_scale);

                // 计算采样区域范围（确保不超出边界）
                const int x_start = src_x_center - sample_width / 2;
                const int y_start = src_y_center - sample_height / 2;
                const int x_end = x_start + sample_width;
                const int y_end = y_start + sample_height;

                // 采样区域内的有效范围
                const int valid_x_start = x_start < 0 ? 0 : x_start;
                const int valid_y_start = y_start < 0 ? 0 : y_start;
                const int valid_x_end = x_end > src_width ? src_width : x_end;
                const int valid_y_end = y_end > src_height ? src_height : y_end;

                // 累加区域内所有像素的颜色值
                int sum_b = 0, sum_g = 0, sum_r = 0;
                int sample_count = 0;

                for(sy = valid_y_start; sy < valid_y_end; sy++) {
                    for(sx = valid_x_start; sx < valid_x_end; sx++) {
                        const int src_idx = (sy * src_width + sx) * 3;
                        sum_b += src_data[src_idx];      // B
                        sum_g += src_data[src_idx + 1];  // G
                        sum_r += src_data[src_idx + 2];  // R
                        sample_count++;
                    }
                }

                // 计算平均颜色值
                const int dst_idx = (y * dst_width + x) * 3;
                if(sample_count > 0) {
                    dst_data[dst_idx] = (unsigned char)(sum_b / sample_count);
                    dst_data[dst_idx + 1] = (unsigned char)(sum_g / sample_count);
                    dst_data[dst_idx + 2] = (unsigned char)(sum_r / sample_count);
                } else {
                    // 采样区域完全超出边界的极端情况
                    dst_data[dst_idx] = 0;
                    dst_data[dst_idx + 1] = 0;
                    dst_data[dst_idx + 2] = 0;
                }
            }
        }
    }
    // 放大图像或不等比例变换 - 使用双线性插值
    else {
        const float src_width_minus_one = src_width - 1.0f;
        const float src_height_minus_one = src_height - 1.0f;

        // 优化：预计算每个目标x坐标对应的源x坐标及权重
        float *src_x_coords = (float *)GxCore_Malloc(dst_width * sizeof(float));
        int *src_x1 = (int *)GxCore_Malloc(dst_width * sizeof(int));
        int *src_x2 = (int *)GxCore_Malloc(dst_width * sizeof(int));
        float *x_weight = (float *)GxCore_Malloc(dst_width * sizeof(float));

        if(!src_x_coords || !src_x1 || !src_x2 || !x_weight) {
            if(allocated_memory)
                free(dst_data);
            if(src_x_coords)
                free(src_x_coords);
            if(src_x1)
                free(src_x1);
            if(src_x2)
                free(src_x2);
            if(x_weight)
                free(x_weight);
            return NULL;
        }

        for(x = 0; x < dst_width; x++) {
            src_x_coords[x] = x * x_ratio;
            src_x1[x] = (int)src_x_coords[x];
            src_x2[x] = (src_x1[x] == src_width_minus_one) ? src_x1[x] : src_x1[x] + 1;
            x_weight[x] = src_x_coords[x] - src_x1[x];
        }

// 对每个目标像素进行处理
#ifdef OMP_SUPPORT
// 使用OpenMP并行处理
#pragma omp parallel for if(dst_width * dst_height > 100000)
#endif
        for(y = 0; y < dst_height; y++) {
            int c = 0;
            // 计算源y坐标及权重
            const float src_y = y * y_ratio;
            const int src_y1 = (int)src_y;
            const int src_y2 = (src_y1 == src_height_minus_one) ? src_y1 : src_y1 + 1;
            const float y_weight = src_y - src_y1;

            // 源图像中y1行和y2行的起始位置
            const int src_y1_offset = src_y1 * src_width * 3;
            const int src_y2_offset = src_y2 * src_width * 3;

            // 目标图像中当前行的起始位置
            const int dst_y_offset = y * dst_width * 3;

            for(x = 0; x < dst_width; x++) {
                // 获取预计算的源x坐标和权重
                const int x1 = src_x1[x];
                const int x2 = src_x2[x];
                const float xw = x_weight[x];

                // 计算四个源像素的索引
                const int idx_11 = src_y1_offset + x1 * 3;
                const int idx_21 = src_y1_offset + x2 * 3;
                const int idx_12 = src_y2_offset + x1 * 3;
                const int idx_22 = src_y2_offset + x2 * 3;

                // 目标像素的索引
                const int dst_idx = dst_y_offset + x * 3;

                // 对每个通道进行双线性插值
                for(c = 0; c < 3; c++) {
                    const float top = (1.0f - xw) * src_data[idx_11 + c] + xw * src_data[idx_21 + c];
                    const float bottom = (1.0f - xw) * src_data[idx_12 + c] + xw * src_data[idx_22 + c];
                    const float pixel = (1.0f - y_weight) * top + y_weight * bottom;

                    dst_data[dst_idx + c] = (unsigned char)(pixel + 0.5f);  // 四舍五入
                }
            }
        }

        // 释放临时缓冲区
        free(src_x_coords);
        free(src_x1);
        free(src_x2);
        free(x_weight);
    }

    return dst_data;
}
#endif

#ifdef ZOOM_BY_GA
static void _convert_argb_to_bgr(unsigned char *argb, unsigned char *bgr, int width, int height)
{
    int i = 0, j = 0;
    unsigned R, G, B;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            int argbIndex = (j * width + i) * 4;  // 每个 ARGB 像素占 4 字节
            int rgbIndex = (j * width + i) * 3;   // 每个 RGB 像素占 3 字节

            // 提取 R、G、B 值
            B = argb[argbIndex + 3];  // Alpha 通道
            G = argb[argbIndex + 2];
            R = argb[argbIndex + 1];

            // 将 RGB 值存储到 RGB888 中
            bgr[rgbIndex + 0] = B;  // R
            bgr[rgbIndex + 1] = G;  // G
            bgr[rgbIndex + 2] = R;  // B
        }
    }
}

void _convert_bgr_to_argb(const unsigned char *bgr24, unsigned int *argb32, int width, int height)
{
    size_t i;
    int num_pixels = width * height;
    for(i = 0; i < num_pixels; ++i) {
        unsigned char b = bgr24[i * 3 + 0];
        unsigned char g = bgr24[i * 3 + 1];
        unsigned char r = bgr24[i * 3 + 2];
        argb32[i] = (255u << 24) | (r << 16) | (g << 8) | b;
    }
}
#endif

void app_zoom_frame(unsigned char *bgr_buffer, GAL_Rect *src, GAL_Rect *dst, char *path)
{
    unsigned char *dst_bgr = NULL;
#ifdef ZOOM_COST_TIME_STATISTICS
    unsigned int m1, m2;
#endif

    if(NULL == bgr_buffer || NULL == src || NULL == dst || NULL == path) {
        return;
    }

#ifdef ZOOM_COST_TIME_STATISTICS
    m1 = app_get_tick_time();
#endif
#ifdef ZOOM_BY_GA
    GuiSurface *src_surface = NULL, *dst_surface = NULL;
    src_surface = hd_get_surface(src, 32, NULL, 1);
    if(NULL == src_surface)
        goto end;

    dst_surface = hd_get_surface(dst, 32, NULL, 1);
    if(NULL == dst_surface)
        goto end;

    dst_bgr = GxCore_Calloc(dst->w * dst->h * 3, sizeof(unsigned char));
    if(NULL == dst_bgr)
        goto end;

    _convert_bgr_to_argb(bgr_buffer, (unsigned int *)src_surface->data, src->w, src->h);
    gdi_begin();
    GXGDI_Blit(src_surface, (GXGDI_Rect *)src, dst_surface, (GXGDI_Rect *)dst, GDI_ZOOM);
    gdi_end();
    _convert_argb_to_bgr(dst_surface->data, dst_bgr, dst->w, dst->h);
    app_save_bgr_data_as_bmp(path, dst_bgr, dst->w, dst->h);
end:
    hd_free_surface(src_surface);
    hd_free_surface(dst_surface);
#else
    dst_bgr = GxCore_Malloc(dst->w * dst->h * 3 * sizeof(unsigned char));
    if(NULL == dst_bgr)
        return;
#ifdef ZOOM_SIMPLE_ALG
    scale_bgr24_image_nearest(bgr_buffer, src->w, src->h, dst_bgr, dst->w, dst->h);
#else
    scale_bgr24_image(bgr_buffer, src->w, src->h, dst_bgr, dst->w, dst->h);
#endif
    app_save_bgr_data_as_bmp(path, dst_bgr, dst->w, dst->h);
#endif
    GxCore_Free(dst_bgr);

#ifdef ZOOM_COST_TIME_STATISTICS
    m2 = app_get_tick_time();
    app_log_min("\033[31m zoom frame cost %u ms!\033[0m\n", m2 - m1);
#endif
}

void app_capture_video_frame_stop(void)
{
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

    while(ctrl->thread_id) {
        GxCore_ThreadDelay(10);
    }

    if(ctrl->vpu_handle > 0) {
        GxAvdev_CloseModule(ctrl->device_handle, ctrl->vpu_handle);
        ctrl->vpu_handle = 0;
    }

    if(ctrl->device_handle > 0) {
        GxAvdev_DestroyDevice(ctrl->device_handle);
        ctrl->device_handle = 0;
    }

    APP_FREE(ctrl->path);
    ctrl->width = 0;
    ctrl->height = 0;
#ifdef LINUX_OS
    GxCore_HwFree(&ctrl->buf);
    memset(&ctrl->buf, 0, sizeof(GxHwMallocObj));
#endif
}

static int _capture_device_and_buf_init(void)
{
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

#ifdef LINUX_OS
    if(ctrl->buf.usr_p == NULL) {
        ctrl->buf.size = MAX_RESOLUTION_WIDTH * MAX_RESOLUTION_HEIGHT * 2;  // size satisfy ycbcr420 or ycbcr422
        GxCore_HwMalloc(&ctrl->buf, MALLOC_WITH_CACHE);
        if(NULL == ctrl->buf.usr_p) {
            app_log_error("hw malloc failed\n");
        }
    }
#endif

    if(ctrl->device_handle <= 0) {
        ctrl->device_handle = GxAvdev_CreateDevice(0);
        if(ctrl->device_handle <= 0) {
            app_log_error("create device failed\n");
            goto err;
        }
    }

    if(ctrl->vpu_handle <= 0) {
        ctrl->vpu_handle = GxAvdev_OpenModule(ctrl->device_handle, GXAV_MOD_VPU, 0);
        if(ctrl->vpu_handle <= 0) {
            app_log_error("open vpu module failed\n");
            goto err;
        }
    }

    return 0;
err:
    return -1;
}

static void _capture_video_frame(void)
{
    PlayerProgInfo *info = NULL;
    GXGDI_Rect rect = { 0 }, zoom_rect = { 0 };
    unsigned char *bgr_buffer = NULL;
    GxVpuProperty_CreateSurface *CreateSurface = NULL;
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

    zoom_rect.w = ctrl->width;
    zoom_rect.h = ctrl->height;

    info = GxCore_Calloc(1, sizeof(PlayerProgInfo));
    if(NULL == info) {
        goto end;
    }

    if(GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info) < 0) {
        goto end;
    }

    rect.w = info->video.width;
    rect.h = info->video.height;

    bgr_buffer = GxCore_Calloc(rect.h * rect.w * 3, sizeof(unsigned char));
    if(NULL == bgr_buffer) {
        app_log_error("calloc failed\n");
        goto end;
    }

    CreateSurface = app_create_capture_surface(&rect, GX_COLOR_FMT_YCBCR420_Y_UV, info->video.bpp);
    if(NULL == CreateSurface) {
        app_log_error("app_create_capture_surface failed\n");
        goto end;
    }

    if(GXCORE_ERROR == app_capture_video_layer(CreateSurface)) {
        app_log_error("app_capture_video_layer failed\n");
        goto end;
    }

    if(CreateSurface->format != GX_COLOR_FMT_YCBCR420_Y_UV) {
        goto end;
    }

    app_convert_yuv420_to_rgb(CreateSurface, bgr_buffer, rect.w, rect.h, 1);
    app_zoom_frame(bgr_buffer, (GAL_Rect *)&rect, (GAL_Rect *)&zoom_rect, ctrl->path);
end:
    if(CreateSurface)
        app_destroy_capture_surface(CreateSurface);
    if(bgr_buffer)
        GxCore_Free(bgr_buffer);
    if(info)
        GxCore_Free(info);

    ctrl->thread_id = 0;
}

static void _capture_video_frame_thread(void *usrdata)
{
    GxCore_ThreadDetach();

    _capture_video_frame();
}

int app_caputre_video_frame(bool async, const char *path, int width, int height)
{
    CaptureVideoFrameCtrl *ctrl = &s_capture_video_frame_ctrl;

    if(_capture_device_and_buf_init() < 0) {
        app_log_error("capture ctrl init failed\n");
        goto err;
    }

    APP_FREE(ctrl->path);
    if(path) {
        ctrl->path = GxCore_Calloc(strlen(path) + 1, sizeof(char));
        strcpy(ctrl->path, path);
        if(NULL == ctrl->path) {
            app_log_error("malloc failed\n");
            goto err;
        }
        ctrl->width = width;
        ctrl->height = height;
    }

    if(async == true) {
        if(GXCORE_ERROR == GxCore_ThreadCreate("capture_video_frame_thread", &ctrl->thread_id, _capture_video_frame_thread, NULL, 16 * 1024, GXOS_DEFAULT_PRIORITY - 1)) {
            app_log_error("thread create failed\n");
            goto err;
        }
    } else {
        _capture_video_frame();
        app_capture_video_frame_stop();
    }

    return 0;
err:
    app_capture_video_frame_stop();
    return -1;
}
