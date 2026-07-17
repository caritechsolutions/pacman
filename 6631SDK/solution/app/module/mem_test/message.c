#include "app_send_msg.h"
#include "message.h"

#ifdef MEMORY_TEST_OUTPUT_OSD
extern int app_mt_message_dialog_is_exist(void);
int app_message_to_osd(char *str, unsigned int error_flag, unsigned int err_count)
{
    AppMsgMessageClass *msg = NULL;
    MTMessageClass *message = NULL;
    int ret = 0, len = 0;
    unsigned int size = strlen(str);
    if(size >= MAX_MT_STR_LEN)
    {
        return -1;
    }
    if(0 == app_mt_message_dialog_is_exist())
    {
        return -2;
    }
    msg = (AppMsgMessageClass*)GxCore_Mallocz(sizeof(AppMsgMessageClass));
    if(NULL == msg)
    {
        return -3;
    }
    message = (MTMessageClass*)GxCore_Mallocz(sizeof(MTMessageClass));
    if(NULL == message)
    {
        return -3;
        GxCore_Free(msg);
    }
    memcpy(message->str, str, size);
    message->err_flag = (error_flag == 1?MT_ERR:0);
    message->err_count = err_count;
    msg->type = APPMSG_MESSAGE_MT;
    len = (sizeof(MTMessageClass) > APP_MSG_SIZE?APP_MSG_SIZE:sizeof(MTMessageClass));
    memcpy(msg->data, message, len);
    ret = app_send_msg_exec(APPMSG_MESSAGE, msg);
    GxCore_Free(message);
    GxCore_Free(msg);
    return ret;
}
#endif
