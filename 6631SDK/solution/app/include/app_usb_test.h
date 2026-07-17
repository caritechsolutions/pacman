#ifndef __APP_USB_TEST_H__
#define __APP_USB_TEST_H__
#include "app_config.h"
#if USB_TEST_SUPPORT

typedef enum {
    USB_TEST_DEL_STAGE,
    USB_TEST_COPY_STAGE,
    USB_TEST_CHECK_STAGE
} UsbTesStage;

typedef enum {
    MSG_USB_TEST_STAGE,
    MSG_USB_TEST_FAILURE,
    MSG_USB_TEST_SPEED,
    MSG_USB_TEST_PROGRESS,
    MSG_USB_TEST_COUNT
} UsbTestMsgType;

typedef union {
    UsbTesStage stage;
    UsbTesStage failure;
    double speed;
    int percent;
    int count;
} UsbTestMsgValue;

typedef struct {
    UsbTestMsgType type;
    UsbTestMsgValue value;
} GxMsgProperty_UsbTest;

int app_usb_test_msg_handle(GxMsgProperty_UsbTest param);
#endif
#endif
