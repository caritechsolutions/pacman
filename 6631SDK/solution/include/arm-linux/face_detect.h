/* Copyright (C) 2001-2025 NationalChip Co., Ltd
 * ALL RIGHTS RESERVED!
 *
 * face_detect.h:
 *
 */

#ifndef __FACE_DETECT_H__
#define __FACE_DETECT_H__

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FACE_DETECT_VERSION "0.3.0"

// 人脸区域
typedef struct {
    float x;
    float y;
    float w;
    float h;
} FaceRect;

// 人脸坐标
typedef struct {
    float x;
    float y;
} FacePoint;

// 人脸信息
typedef struct {
    FaceRect rect;          // 人脸区域
    FacePoint landmark[5];  // 人脸关键点坐标
    float prob;             // 置信度
} FaceObj;

// 人脸检测模型输入数据
typedef struct {
    unsigned char* data;
    int height;
    int width;
} InputFrame;

/***************************************************
Description: 创建一个人脸检测模型实例
Params: 无
Return: 返回指向创建的模型实例的指针，创建失败则返回NULL
****************************************************/
void* face_detect_create_model(void);

/***************************************************
Description: 加载人脸检测模型
Params:
    model: 模型实例指针
    param_path: 模型参数文件路径
    model_path: 模型文件路径
    low_memory: 是否使用低内存模式，开启后模型内存占用会减少，但运行速度会变慢
Return: 正确返回0
****************************************************/
int face_detect_load_model(void* model, const char* param_path, const char* model_path, bool low_memory);

/***************************************************
Description: 加载人脸检测模型
Params:
    model: 模型实例指针
    frame: 输入数据信息，数据排布：[height, width, channel]，channel必须是3
    faces: 人脸信息数组
    face_num: 最大检测数
    act_face_num: 实际检测到的人脸
    prob_threshold: 置信度阈值，范围[0, 1]，高于该阈值的人脸将被检测
Return: 正确返回0
****************************************************/
int face_detect_infer_model(void* model, InputFrame* frame, FaceObj* faces, int face_num, int* act_face_num,
                            float prob_threshold);

/***************************************************
Description: 销毁人脸检测模型实例
Params:
    model: 模型实例指针
Return: 无
****************************************************/
void face_detect_destroy_model(void* model);

#ifdef __cplusplus
}
#endif

#endif