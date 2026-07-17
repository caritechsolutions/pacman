#include "app_config.h"
#include <string.h>
#if FACE_DETECT_SUPPORT
#include "app_face_detect.h"
#include "face_detect_math_lib.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_utility.h"
#include "gdi_core.h"
#include "face_detect.h"
#include "gxos/gxcore_os_core.h"
#include "module/app_log.h"
#include "app_module.h"
#include "app_wnd_file_list.h"
#include "full_screen.h"
#include "app_windows.h"
#include "app_capture_video_frame.h"
#include "module/app_log.h"

//#define FACE_DETECT_DEBUG
#ifdef FACE_DETECT_DEBUG
#ifdef ECOS_OS
#define DEBUG_PATH_PREFIX "/mnt/usb01"
#else
#define DEBUG_PATH_PREFIX "/home/gx"
#endif
#endif
#ifdef ECOS_OS
#define MODEL_PATH "/models/"
#else
#define MODEL_PATH "/lib/firmware/models/"
#endif
#define FACE_DETECT_PROMPT_POP "face_detect_prompt"

#define MAX_RESOLUTION_WIDTH (1920)
#define MAX_RESOLUTION_HEIGHT (1080)

typedef struct {
    int width;
    int height;
}ZoomRect;

typedef enum {
    FACE_DETECT_STATUS_IDLE = 0,
    FACE_DETECT_STATUS_DETECTING,
} FaceDetectStatus;

typedef struct {
    int thread_id;
    int status;
    char *path;
    ZoomRect zoom_rect;
} FaceDetectCtrl;

static FaceDetectCtrl s_face_detect_ctrl = {
    .thread_id = 0,
    .status = FACE_DETECT_STATUS_IDLE,
    .path = NULL,
    .zoom_rect = {
        .width = 0,
        .height = 0,
    },
};

#ifdef FACE_DETECT_DEBUG
static int _write_to_file(unsigned char *buffer, int size, char *path)
{
    if(!buffer || !path || !size)
        return -1;

    FILE *fp = fopen(path, "w");
    if(fp == NULL) {
        printf("file open error\n");
        return -1;
    }
    fwrite(buffer, 1, size, fp);
    fclose(fp);
    return 0;
}
#endif

void _convert_bgra_to_bgr(unsigned char *argb, unsigned char *bgr, int width, int height)
{
    int i = 0, j = 0;
    unsigned R, G, B;

    for(j = 0; j < height; j++) {
        for(i = 0; i < width; i++) {
            int argbIndex = (j * width + i) * 4;  // 每个 ARGB 像素占 4 字节
            int rgbIndex = (j * width + i) * 3;   // 每个 RGB 像素占 3 字节

            // 提取 R、G、B 值
            B = argb[argbIndex + 0];  // Alpha 通道
            G = argb[argbIndex + 1];
            R = argb[argbIndex + 2];

            // 将 RGB 值存储到 RGB888 中
            bgr[rgbIndex + 0] = B;  // R
            bgr[rgbIndex + 1] = G;  // G
            bgr[rgbIndex + 2] = R;  // B
        }
    }
}

#ifdef FACE_DETECT_DEBUG
static void _rectangle_frame(unsigned char *bgr, GXGDI_Rect *src_rect, FaceRect *face_rect)
{
#define RECTANGLE_WIDTH (5)
    int i = 0, j = 0;
    GXGDI_Rect tmp_rect = { 0 };
    int x1, x2, y1, y2;

    if(NULL == bgr || NULL == src_rect || NULL == face_rect)
        return;

    tmp_rect.x = (int)face_rect->x;
    tmp_rect.y = (int)face_rect->y;
    tmp_rect.w = (int)face_rect->w;
    tmp_rect.h = (int)face_rect->h;

    x1 = tmp_rect.x - RECTANGLE_WIDTH;
    x2 = tmp_rect.x + tmp_rect.w + RECTANGLE_WIDTH;
    y1 = tmp_rect.y - RECTANGLE_WIDTH;
    y2 = tmp_rect.y + tmp_rect.h + RECTANGLE_WIDTH;

    for(i = x1; i < x2; i++) {
        for(j = y1; j < y2; j++) {
            if((j < tmp_rect.y || j >= y2 - RECTANGLE_WIDTH) || (i < tmp_rect.x || i >= x2 - RECTANGLE_WIDTH)) {
                bgr[(j * src_rect->w + i) * 3] = 0x00;
                bgr[(j * src_rect->w + i) * 3 + 1] = 0x00;
                bgr[(j * src_rect->w + i) * 3 + 2] = 0xff;
            }
        }
    }
}
#endif

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
unsigned char *scale_bgr24_image_nearest(const unsigned char *src_data, int src_width, int src_height, unsigned char *dst_data, int dst_width, int dst_height)
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
#ifdef LINUX_OS
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

static void _face_detect_thread(void *usrdata)
{
    void *detect = NULL;
    PlayerProgInfo *info = NULL;
    GXGDI_Rect rect = { 0 }, zoom_rect = { 0 };
    unsigned char *bgr_buffer = NULL;
    int j = 0;
    FaceObj faces[10];
    int face_num = 0;
    unsigned int time = 0;
    GxVpuProperty_CreateSurface *CreateSurface = NULL;
    FaceDetectCtrl *ctrl = &s_face_detect_ctrl;
    bool first_capture = true;

    const char *param_path = MODEL_PATH "model_detect.ncnn.param";
    const char *model_path = MODEL_PATH "model_detect.ncnn.bin";

    GxCore_ThreadDetach();

    detect = face_detect_create_model();
    if(detect == NULL) {
        app_log_error("create model failed\n");
        return;
    }
    if(face_detect_load_model(detect, param_path, model_path, false) != 0) {
        app_log_error("load detect model failed\n");
        goto end;
    }

    info = GxCore_Calloc(1, sizeof(PlayerProgInfo));
    if(NULL == info) {
        app_log_error("calloc failed\n");
        goto end;
    }

    if(GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info) < 0)
        goto end;

    rect.w = info->video.width;
    rect.h = info->video.height;

    bgr_buffer = GxCore_Calloc(rect.h * rect.w * 3, sizeof(unsigned char));
    if(NULL == bgr_buffer) {
        app_log_error("malloc failed\n");
        goto end;
    }

    CreateSurface = app_create_capture_surface(&rect, GX_COLOR_FMT_YCBCR420_Y_UV, info->video.bpp);
    if(NULL == CreateSurface) {
        app_log_error("_create_capture_surface failed\n");
        goto end;
    }

    zoom_rect.w = ctrl->zoom_rect.width;
    zoom_rect.h = ctrl->zoom_rect.height;

    while(ctrl->status == FACE_DETECT_STATUS_DETECTING) {
#ifdef FACE_DETECT_DEBUG
        unsigned int m1, m2, m3;

        m1 = app_get_tick_time();
        char debug_path[50] = {0};
#endif
        time = app_tick_time_sec_get();
        if(GXCORE_ERROR == app_capture_video_layer(CreateSurface)) {
            app_log_error("_capture_video_frame failed\n");
            break;
        }

#ifdef FACE_DETECT_DEBUG
        m2 = app_get_tick_time();
#endif
        if(CreateSurface->format != GX_COLOR_FMT_YCBCR420_Y_UV)
            break;

        app_convert_yuv420_to_rgb(CreateSurface, bgr_buffer, rect.w, rect.h, 1);
        if(first_capture == true) {
            app_zoom_frame(bgr_buffer, (GAL_Rect *)&rect, (GAL_Rect *)&zoom_rect, ctrl->path);
            first_capture = false;
        }

        InputFrame frame = {
            .data = bgr_buffer,
            .height = rect.h,
            .width = rect.w,
        };

        memset(faces, 0, sizeof(faces));
        face_detect_infer_model(detect, &frame, faces, sizeof(faces) / sizeof(faces[0]), &face_num, 0.5);
        app_log_debug("Number of faces recognized: %d\n", face_num);
        for(j = 0; j < face_num; j++) {
            app_log_debug("%.5f at %.2f %.2f %.2f x %.2f\n", faces[j].prob, faces[j].rect.x, faces[j].rect.y, faces[j].rect.w, faces[j].rect.h);
            app_log_debug("landmark %.2f %.2f, %.2f %.2f, %.2f %.2f, %.2f %.2f, %.2f "
                          "%.2f\n",
                          faces[j].landmark[0].x, faces[j].landmark[0].y, faces[j].landmark[1].x, faces[j].landmark[1].y, faces[j].landmark[2].x, faces[j].landmark[2].y, faces[j].landmark[3].x,
                          faces[j].landmark[3].y, faces[j].landmark[4].x, faces[j].landmark[4].y);

#ifdef FACE_DETECT_DEBUG
            snprintf(debug_path, sizeof(debug_path), "%s/capture-orginal_%d_%dx%d.bgr", DEBUG_PATH_PREFIX, time, rect.w, rect.h);
            _write_to_file(bgr_buffer, rect.w * rect.h * 3, debug_path);
            _rectangle_frame(bgr_buffer, &rect, &faces[j].rect);
#endif
        }
#ifdef FACE_DETECT_DEBUG
        app_log_min("\n");
        m3 = app_get_tick_time();
        app_log_min("\033[31mcapture and recognized cost %u + %u ms!\033[0m\n", m2 - m1, m3 - m2);
        snprintf(debug_path, sizeof(debug_path), "%s/capture_%d.bgr", DEBUG_PATH_PREFIX, time);
        _write_to_file(bgr_buffer, rect.w * rect.h * 3, debug_path);
        snprintf(debug_path, sizeof(debug_path), "%s/capture_%d.bmp", DEBUG_PATH_PREFIX, time);
        app_save_bgr_data_as_bmp(debug_path, bgr_buffer, rect.w, rect.h);
#endif
        if(face_num > 0) {
            break;
        }
        GxCore_ThreadDelay(40);
    }

end:
    if(detect)
        face_detect_destroy_model(detect);
    if(info)
        GxCore_Free(info);
    if(CreateSurface)
        app_destroy_capture_surface(CreateSurface);
    if(face_num > 0) {
#ifdef FACE_DETECT_DEBUG
        unsigned int m4 = app_get_tick_time();
#endif
        app_zoom_frame(bgr_buffer, (GAL_Rect *)&rect, (GAL_Rect *)&zoom_rect, ctrl->path);
#ifdef FACE_DETECT_DEBUG
        unsigned int m5 = app_get_tick_time();
        printf("\033[31mzoom_frame cost %u ms!\033[0m\n", m5 - m4);
#endif
        AppMsgMessageClass *msg = GxCore_Calloc(1, sizeof(AppMsgMessageClass));
        if (NULL != msg) {
            FaceDetectMsgClass face_detect_msg = {0};
            face_detect_msg.time = time;
            msg->type = APPMSG_MESSAGE_FD;
            memcpy(msg->data, &face_detect_msg, sizeof(FaceDetectMsgClass));
            app_send_msg_exec(APPMSG_MESSAGE, msg);
            GxCore_Free(msg);
        }
    }
    if(bgr_buffer)
        GxCore_Free(bgr_buffer);
    ctrl->thread_id = 0;
}

/**
 * @brief 启动人脸检测功能
 *
 * 该函数启动一个后台线程进行实时人脸检测。当检测到人脸时，会保存缩放后的图像到指定路径，
 * 并发送消息通知其他模块。
 *
 * @param path 保存检测到人脸时图像的文件路径，可以为NULL
 * @param width 保存图像的目标宽度
 * @param height 保存图像的目标高度
 * @return 成功返回0，失败返回-1
 *
 * @note
 * - 如果已有检测线程在运行，则返回错误
 * - 检测线程会持续运行直到调用app_face_detect_stop()停止
 * - 当检测到人脸时，会自动保存图像并发送APPMSG_MESSAGE_FD消息
 */
int app_face_detect_start(const char *path, int width, int height)
{
    FaceDetectCtrl *ctrl = &s_face_detect_ctrl;

    if(ctrl->thread_id != 0) {
        app_log_error("please wait previous face detect finish\n");
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
        ctrl->zoom_rect.width = width;
        ctrl->zoom_rect.height = height;
    }
    ctrl->status = FACE_DETECT_STATUS_DETECTING;
    if(GXCORE_ERROR == GxCore_ThreadCreate("face_detect_thread", &ctrl->thread_id, _face_detect_thread, NULL, 32 * 1024, GXOS_DEFAULT_PRIORITY - 1)) {
        app_log_error("thread create failed\n");
        goto err;
    }
    return 0;
err:
    app_face_detect_stop();
    return -1;
}

/**
 * @brief 停止人脸检测功能
 *
 * 该函数停止正在运行的人脸检测线程，并清理相关资源。
 * 函数会等待检测线程完全退出后再返回。
 *
 * @note
 * - 如果当前没有运行的检测线程，该函数仍可安全调用
 * - 函数会阻塞等待检测线程退出，通常耗时很短
 * - 清理所有相关资源，包括路径内存和缩放参数
 */
void app_face_detect_stop(void)
{
    FaceDetectCtrl *ctrl = &s_face_detect_ctrl;

    ctrl->status = FACE_DETECT_STATUS_IDLE;
    while(ctrl->thread_id != 0) {
        GxCore_ThreadDelay(20);
    }
    GxCore_Free(ctrl->path);
    ctrl->path = NULL;
    ctrl->zoom_rect.width = 0;
    ctrl->zoom_rect.height = 0;
}

static event_list *s_face_detect_info_timer = NULL;

static int _face_detect_info_timer_cb(void *usrdata)
{
    if(s_face_detect_info_timer) {
        s_face_detect_info_timer = NULL;
    }
    if(GUI_CheckPrompt(FACE_DETECT_PROMPT_POP) == GXCORE_SUCCESS) {
        GUI_EndPrompt(FACE_DETECT_PROMPT_POP);
    }
    return 0;
}

static void _face_detect_prompt_pop(void)
{
    if(g_AppFullArb.state.ttx == STATE_ON) {
        g_AppTtxSubt.ttx_magz_opt(&g_AppTtxSubt, STBK_EXIT);
        GUI_SetProperty(WND_FULL_SCREEN, "update", NULL);
        GUI_SetProperty("img_full_mute", "img", "s_mute");
        GUI_SetProperty("img_full_pause", "img", "s_pause");
    }
    APP_TIMER_REMOVE(s_face_detect_info_timer);
    if(GUI_CheckPrompt(FACE_DETECT_PROMPT_POP) == GXCORE_SUCCESS) {
        GUI_EndPrompt(FACE_DETECT_PROMPT_POP);
    }
    GUI_CreatePrompt(APP_WND_FACE_PROMPT_POS_X, APP_WND_FACE_PROMPT_POS_Y, FACE_DETECT_PROMPT_POP, FACE_DETECT_PROMPT_POP, "", "ontop");
    APP_TIMER_ADD(s_face_detect_info_timer, _face_detect_info_timer_cb, 3000, TIMER_ONCE);
}

/**
 * @brief 处理人脸检测消息
 *
 * 该函数处理来自人脸检测线程的消息，当检测到人脸时被调用。
 * 主要功能包括更新PVR预览时间和显示人脸检测提示界面。
 *
 * @param message 指向AppMsgMessageClass类型的消息指针
 * @return 成功返回0，失败返回-1
 *
 * @note
 * - 仅处理APPMSG_MESSAGE_FD类型的消息
 * - 如果启用PVR_PREVIEW_SUPPORT，会更新PVR文件的预览时间
 * - 自动显示人脸检测提示弹窗，3秒后自动消失
 * - 如果当前显示TTX字幕，会先退出TTX模式
 */
int app_face_detect_msg_process(void *message)
{
    AppMsgMessageClass *msg = NULL;

    if(NULL == message)
        return -1;
    msg = (AppMsgMessageClass*)message;
    if (msg->type != APPMSG_MESSAGE_FD) {
        return -1;  // 检查消息类型
    }
#if PVR_PREVIEW_SUPPORT
    FaceDetectMsgClass *face_detect_msg = NULL;
    face_detect_msg = (FaceDetectMsgClass*)(msg->data);
    extern int app_pvr_file_info_update_preview_time(time_t time);
    app_pvr_file_info_update_preview_time(face_detect_msg->time);
#endif
    _face_detect_prompt_pop();
    return 0;
}

#ifdef FACE_DETECT_DEBUG
static char *s_file_path_bak = NULL;
static char *s_file_path = NULL;

static int extract_width_height_from_file_path(const char *file_path, int *width, int *height)
{
    if(file_path == NULL || width == NULL || height == NULL) {
        return -1;  // 检查无效输入
    }

    // 查找最后一个斜杠字符
    const char *filename = strrchr(file_path, '/');
    if(filename == NULL) {
        filename = file_path;  // 如果没有斜杠，则使用整个路径
    } else {
        filename++;  // 移动到斜杠后的第一个字符
    }

    int num_read = sscanf(filename, "%*[^_]_%*d_%dx%d", width, height);

    if(num_read == 2) {
        // 成功提取宽度和高度
        return 0;
    } else {
        // 提取失败
        return -1;
    }
}

static void _face_detect_from_file(const char *file_path, int width, int height)
{
    int j = 0;
    unsigned char *bgr_buffer = NULL;
    const char *param_path = MODEL_PATH "model_detect.ncnn.param";
    const char *model_path = MODEL_PATH "model_detect.ncnn.bin";
    FILE *fp = NULL;
    size_t bytes_read;
    char debug_path[50] = {0};

    // Calculate the expected buffer size
    size_t expected_size = (size_t)width * (size_t)height * 3;

    // Allocate memory for the BGR buffer
    bgr_buffer = GxCore_Calloc(expected_size, sizeof(unsigned char));
    if(NULL == bgr_buffer) {
        app_log_error("Memory allocation failed\n");
        return;
    }

    // Create face detection model
    void *detect = face_detect_create_model();
    if(detect == NULL) {
        app_log_error("create model failed\n");
        GxCore_Free(bgr_buffer);
        return;
    }

    // Load face detection model
    if(face_detect_load_model(detect, param_path, model_path, false) != 0) {
        app_log_error("load detect model failed\n");
        face_detect_destroy_model(detect);
        GxCore_Free(bgr_buffer);
        return;
    }

    FaceObj faces[10];
    int face_num;

    // Open the file in binary read mode
    fp = fopen(file_path, "rb");
    if(fp == NULL) {
        app_log_error("file open error\n");
        face_detect_destroy_model(detect);
        GxCore_Free(bgr_buffer);
        return;
    }

    // Read the file content into the buffer
    bytes_read = fread(bgr_buffer, 1, expected_size, fp);
    if(bytes_read != expected_size) {
        app_log_error("Error reading file: read %zu bytes, expected %zu bytes\n", bytes_read, expected_size);
        fclose(fp);
        face_detect_destroy_model(detect);
        GxCore_Free(bgr_buffer);
        return;
    }
    fclose(fp);

    InputFrame frame = {
        .data = bgr_buffer,
        .height = height,
        .width = width,
    };
    GXGDI_Rect rect = {
        .x = 0,
        .y = 0,
        .w = width,
        .h = height,
    };

    // Perform face detection
    face_detect_infer_model(detect, &frame, faces, sizeof(faces) / sizeof(faces[0]), &face_num, 0.5);
    app_log_info("Number of faces recognized: %d\n", face_num);
    for(j = 0; j < face_num; j++) {
        app_log_info("%.5f at %.2f %.2f %.2f x %.2f\n", faces[j].prob, faces[j].rect.x, faces[j].rect.y, faces[j].rect.w, faces[j].rect.h);
        app_log_info("landmark %.2f %.2f, %.2f %.2f, %.2f %.2f, %.2f %.2f, %.2f "
               "%.2f\n",
               faces[j].landmark[0].x, faces[j].landmark[0].y, faces[j].landmark[1].x, faces[j].landmark[1].y, faces[j].landmark[2].x, faces[j].landmark[2].y, faces[j].landmark[3].x,
               faces[j].landmark[3].y, faces[j].landmark[4].x, faces[j].landmark[4].y);
        _rectangle_frame(bgr_buffer, &rect, &faces[j].rect);
    }

    snprintf(debug_path, sizeof(debug_path), "%s/capture_modify.bgr", DEBUG_PATH_PREFIX);
    // Write the modified BGR buffer to a file
    _write_to_file(bgr_buffer, width * height * 3, debug_path);

    // Clean up resources
    face_detect_destroy_model(detect);
    GxCore_Free(bgr_buffer);
}

static int _face_detect_file_selected_cb(WndStatus ret)
{
    if(ret == WND_OK) {
        app_get_file_real_path(&s_file_path);

        if(s_file_path != NULL) {
            int str_len = 0;
            int width = 0, height = 0;

            if(0 == extract_width_height_from_file_path(s_file_path, &width, &height)) {
                app_log_error("detect file path: %s, Extracted width: %d, height: %d\n", s_file_path, width, height);
                _face_detect_from_file(s_file_path, width, height);
            } else {
                app_log_error("Failed to extract width and height from filename.\n");
            }

            if(s_file_path_bak != NULL) {
                GxCore_Free(s_file_path_bak);
                s_file_path_bak = NULL;
            }

            str_len = strlen(s_file_path);
            s_file_path_bak = (char *)GxCore_Malloc(str_len + 1);
            if(s_file_path_bak != NULL) {
                memcpy(s_file_path_bak, s_file_path, str_len);
                s_file_path_bak[str_len] = '\0';
            }
        }
    }

    return 0;
}

void app_face_detect_from_choose_file(void)
{
    FileListParam file_para;
    memset(&file_para, 0, sizeof(file_para));

    file_para.cur_path = s_file_path_bak;
    file_para.dest_path = &s_file_path;
    file_para.suffix = "bgr;BGR";
    file_para.pos_x = WND_FILE_LIST_POS_X;
    file_para.pos_y = WND_FILE_LIST_POS_Y;
    file_para.exit_cb = _face_detect_file_selected_cb;

    app_get_file_path_dlg(&file_para);
}
#endif
#endif
