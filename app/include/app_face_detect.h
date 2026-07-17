#ifndef __APP_FACE_DETECT_H__
#define __APP_FACE_DETECT_H__
#include <time.h>

typedef struct {
    time_t time;
} FaceDetectMsgClass;

extern int app_face_detect_start(const char *path, int width, int height);
extern void app_face_detect_stop(void);
extern int app_face_detect_msg_process(void *message);
#endif // !__APP_FACE_DETECT_H__
