#include "app_send_msg.h"

#if VOICE_CONTROL_SUPPORT
#include "vc_cmd.h"
typedef struct _VCMessageClass{
#define VC_DES_LEN  1000
    unsigned int cmd;// ui cmd
    unsigned int des_len;
    char des[VC_DES_LEN];
}VCMessageClass;

int app_vc_cloud_msg_process(void *message)
{
extern int app_vc_cloud_cmd_process(unsigned int cmd_id, char* des, unsigned int des_len);
    int ret = 0;
    AppMsgMessageClass *msg = NULL;
    VCMessageClass *vc_msg = NULL;
    if(NULL == message)
        return -1;
    msg = (AppMsgMessageClass*)message;
    if((APPMSG_MESSAGE_VC != msg->type) || (NULL == msg->data))
        return -1;

    vc_msg = (VCMessageClass*)(msg->data);
    ret = app_vc_cloud_cmd_process(vc_msg->cmd, vc_msg->des, vc_msg->des_len);
    return ret;
}

int app_vc_cloud_msg_transfer(void *message, unsigned int *cmd, char **content)
{
    VCMessageClass *vc_msg = NULL;
    VoiceCMDParamsClass *params = NULL;

    vc_msg = (VCMessageClass*)message;
    if(vc_msg)
    {
        *cmd = vc_msg->cmd;
        params = (VoiceCMDParamsClass*)vc_msg->des;
        *content = (char *)params->str;
        return 0;
    }
    return -1;
}

int app_voice_control_msg_send(unsigned int cmd, char* des, unsigned int des_len)
{
    AppMsgMessageClass msg = {0};
    VCMessageClass message;
    int ret = 0;
    int len = 0;

    memset(&msg, 0, sizeof(AppMsgMessageClass));
    memset(&message, 0, sizeof(VCMessageClass));
    message.cmd = cmd;
    if((NULL != des) && (des_len > 0))
    {
        message.des_len = (VC_DES_LEN > des_len?des_len:(VC_DES_LEN - 1));
        memcpy(message.des, des, message.des_len);
    }
    msg.type = APPMSG_MESSAGE_VC;
    len = (sizeof(VCMessageClass) > APP_MSG_SIZE?APP_MSG_SIZE:sizeof(VCMessageClass));
    memcpy(msg.data, &message, len);
    ret = app_send_msg_exec(APPMSG_MESSAGE, &msg);

    return ret;
}
#endif
