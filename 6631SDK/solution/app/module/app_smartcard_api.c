#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "module/app_smartcard_api.h"
#include <gxcore.h>
#include "module/app_log.h"

unsigned char debug_level[128];

#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE        (0)
#endif

#ifndef INVALID_HANDLE
#define INVALID_HANDLE (0x12345678) //the value is useless, just return to caller
#endif

#define SMC_CHECK_P(p, r) if(NULL == p){\
                            app_log_debug("\033[033m");\
                            app_log_debug("\n[%s: %d] %s\n", __FILE__, __LINE__, __FUNCTION__);\
                            app_log_debug("%s is NULL\n", #p);\
                            app_log_debug("\033[0m");\
                            return r;}

#define SMC_ERR(fmt, args...)  do {                                 \
                                    app_log_error("\033[31m");                                             \
                                    app_log_error("[SMC_ERR][%s:%d]: ", __func__, __LINE__);               \
                                    app_log_error(fmt, ##args);                                            \
                                    app_log_error("\033[0m\n");                                            \
                                } while(0)

/**
* @brief       智能卡打开函数
* @param [in]  name                    智能卡设备名
* @param [in]  sc_notify               智能卡状态通知函数指针
* @return      handle_t 智能卡句柄E_INVALID_HANDLE 执行失败
* @note     第一次调用时,将对内部资源进行初始化
*/
handle_t GxSmc_Open(const char*             name,
                            GxSmcParams*       param)
{
    GxSciPol        detect     = GXSCI_HIGH_LEVEL;
    GxSciPol        vcc        = GXSCI_HIGH_LEVEL;
    GxSciParams     sci_params = {0};

    SMC_CHECK_P(param, E_INVALID_HANDLE);

    if(GXSMC_DETECT_HIGH_LEVEL == param->detect_pole)
        detect = GXSCI_HIGH_LEVEL;
    else
        detect = GXSCI_LOW_LEVEL;

    if(GXSMC_VCC_HIGH_LEVEL == param->vcc_pole)
        vcc = GXSCI_HIGH_LEVEL;
    else
        vcc = GXSCI_LOW_LEVEL;

    if(GxSci_Open(detect, vcc) != 0)
        return E_INVALID_HANDLE;

    sci_params.io_conv     = param->io_conv;
    sci_params.parity      = param->parity;
    sci_params.protocol    = param->protocol;
    sci_params.stop_len    = param->stop_len;
    sci_params.default_etu = param->default_etu;
    sci_params.auto_etu    = param->auto_etu;
    sci_params.auto_io_conv_and_parity = param->auto_parity;

    if(GxSci_Setup(&sci_params) != 0)
        return E_INVALID_HANDLE;

    return INVALID_HANDLE;
}


/**
* @brief       智能卡复位函数
* @param [in]   handle     智能卡句柄
* @param [out]  AtrBuf    ATR缓冲,缓冲区大小建议不小于33字节
* @param [in]  BufSize     ATR缓冲大小
* @param [out]  RetLen     实际获取的atr长度
* @return      int >=0 执行成功;<0 执行失败
* note: reset函数将使用默认的参数配置进行操作，reset后可以调用config修改成自己
* 想要到参数配置。
*/
int GxSmc_Reset(handle_t handle, uint8_t *AtrBuf, size_t BufSize, size_t *RetLen)
{
    return GxSci_Reset(AtrBuf, BufSize, (int32_t *)RetLen);
}

/**
* @brief       智能卡时序参数配置
* @param [in]   handle     智能卡句柄
* @param [in]  time    智能卡时序参数
* @return      int >=0 执行成功;<0 执行失败
*/
int GxSmc_Config(handle_t handle, const GxSmcTimeParams* time)
{
    GxSciTimeParams params = {0};

    SMC_CHECK_P(time, -1);

    params.clock = time->baud_rate;
    params.etu      = time->etu;
    params.egt      = time->egt;
    params.tgt      = time->tgt;
    params.wdt      = time->wdt;
    params.twdt     = time->twdt;

    return GxSci_Config(&params);
}



/**
* @brief       智能卡参数配置新接口，尽量使用此接口代替GxSmc_Config
* @param [in]   handle     智能卡句柄
* @param [in]  config    智能卡参数
* @return      int >=0 执行成功;<0 执行失败
*/
int GxSmc_ConfigAll(handle_t handle, const GxSmcConfigs *config)
{
    int             ret    = 0;
    GxSciParams     params = {0};
    GxSciTimeParams time   = {0};

    SMC_CHECK_P(config, -1);

    GxSci_GetParams(&time, &params);

    if ((config->flags & SMCC_AUTO_ENABLE)
            && (config->flags & SMCC_AUTO_DISABLE)){
        SMC_ERR("do you want enable or disable auto\n");
        return -1;
    }
    if (config->flags & SMCC_AUTO_ENABLE){
        params.auto_etu = 1;
    }
    if (config->flags & SMCC_AUTO_DISABLE){
        params.auto_etu = 0;
    }

    if ((config->flags & SMCC_AUTO_ETU_ENABLE)
            && (config->flags & SMCC_AUTO_ETU_DISABLE)){
        SMC_ERR("do you want enable or disable auto etu\n");
        return -1;
    }
    if (config->flags & SMCC_AUTO_ETU_ENABLE){
        params.auto_etu = 1;
    }
    if (config->flags & SMCC_AUTO_ETU_DISABLE){
        params.auto_etu = 0;
    }

    if ((config->flags & SMCC_AUTO_CONV_PARITY_ENABLE)
            && (config->flags & SMCC_AUTO_CONV_PARITY_DISABLE)){
        SMC_ERR("do you want enable or disable auto conv and parity\n");
        return -1;
    }
    if (config->flags & SMCC_AUTO_CONV_PARITY_ENABLE){
        params.auto_io_conv_and_parity = 1;
    }
    if (config->flags & SMCC_AUTO_CONV_PARITY_DISABLE){
        params.auto_io_conv_and_parity = 0;
    }

    if (config->flags & SMCC_PROTOCOL){
        params.protocol = (GxSciRepeatMode)config->protocol;
    }

    if (config->flags & SMCC_PARITYTYPE){
        params.parity = (GxSciParityType)config->parity;
    }

    if (config->flags & SMCC_STOPLEN){
        params.stop_len = (GxSciStopLen)config->stop_len;
    }

    if (config->flags & SMCC_DATACONV){
        params.io_conv = (GxSciParityType)config->io_conv;
    }

    if (config->flags & SMCC_TIME){
        if (config->time.flags & SMCT_FRE){
            time.clock = config->time.baud_rate;
        }
        if (config->time.flags & SMCT_ETU){
            time.etu = config->time.etu;
        }
        if (config->time.flags & SMCT_EGT){
            time.egt = config->time.egt;
        }
        if (config->time.flags & SMCT_TGT){
            time.tgt = config->time.tgt;
        }
        if (config->time.flags & SMCT_WDT){
            time.wdt = config->time.wdt;
        }
        if (config->time.flags & SMCT_TWDT){
            time.twdt = config->time.twdt;
        }

    }

    ret |= GxSci_Setup(&params);
    ret |= GxSci_Config(&time);

    if(ret != 0)
        return -1;

    return 0;
}

/**
* @brief       智能卡发送命令函数
* @param [in]  handle     智能卡句柄
* @param [in]  Cmd  智能卡命令指针
* @param [in]  CmdLen       智能卡命令长度
* @param [out]  SendLen   发送给智能卡的实际长度
* @return      int 返回发送成功到字节数，<0，则发送失败
*/
int GxSmc_SendCmd(handle_t handle, const uint8_t *Cmd, size_t CmdLen)
{
    return GxSci_Write((uint8_t *)Cmd, CmdLen, 0);
}
/**
* @brief       智能卡发送命令函数
* @param [in]  handle     智能卡句柄
* @param [in]  Cmd  智能卡命令指针
* @param [in]  CmdLen       智能卡命令长度
* @param [in]  timeout 超时时间
* @return      int 返回发送成功的字节数，<0，则发送失败
*/
int GxSmc_SendCmd_Extend(handle_t handle, const uint8_t *Cmd,size_t CmdLen, size_t timeout)
{
    return GxSci_Write((uint8_t *)Cmd, CmdLen, timeout);
}

/**
* @brief       获取智能卡响应
* @param [in]  handle      智能卡句柄
* @param [out] ReplyBuf      智能卡响应缓冲
* @param [out] BufSize    智能卡响应长度缓冲
* @param [in]  timeout 超时时间
* @return      int 返回获取到的字节数，<0则获取失败
*/
int GxSmc_GetReply_Extend(handle_t handle, uint8_t* ReplyBuf, size_t BufSize,size_t timeout)
{
    int ret;

    ret = GxSci_Read(ReplyBuf, BufSize, timeout);
    if(ret < 0)
    {
        SMC_ERR("get reply failure!\n");
        return -1;
    }
    return ret;
}

/**
* @brief       获取智能卡响应
* @param [in]  handle      智能卡句柄
* @param [out] ReplyBuf      智能卡响应缓冲
* @param [out] BufSize    智能卡响应长度缓冲
* @param [out] RetLen    智能卡响应的实际长度
* @return      int 返回获取到的字节数，<0则获取失败
*/
int GxSmc_GetReply(handle_t handle, uint8_t* ReplyBuf, size_t BufSize)
{
    int ret;

    ret = GxSci_Read(ReplyBuf, BufSize, 10000);
    if(ret < 0)
    {
        SMC_ERR("get reply failure!\n");
        return -1;
    }
    return ret;
}

/**
* @brief       处理智能卡命令
* @param [in]  handle      智能卡句柄
* @param [in] CmdLen   处理的命令长度
* @param [in] pCmd      需要处理的命令指针
* @param [out] pResponse    输出的响应缓存
* @param [out] RespLen   处理完成后返回的响应长度
* @param [out] pSW1   处理完成后返回的状态字1
* @param [out] pSW2   处理完成后返回的状态字2
* @return      int >=0 执行成功;<0 执行失败
*@note    此函数完成了简单的apdu部分处理,比如0x61, sw1 == INS,等待
          等等.相当于GxSmc_SendCmd,GxCA_SmcGetReply两函数的再次组合,
          使用时请根据cas的情况作出选择.
*/
#define MAX_REPLY_BYTES 1024
static unsigned char thiz_reply_buf[MAX_REPLY_BYTES] = {0};
int GxSmc_SendReceiveData(handle_t handle,
                            size_t CmdLen,
                            const uint8_t *pCmd,
                            uint8_t *pResponse,
                            size_t *RespLen,
                            uint8_t *pSW1,
                            uint8_t *pSW2)
{
    int ret = 0;
    unsigned char cmd[10] = {0};
    int cmd_len = 0;
    int len = 0;
    ret = GxSci_Apdu((uint8_t *)pCmd, CmdLen, pResponse, RespLen, pSW1, pSW2);
    if(ret == 0)
    {// success
        if(*RespLen == 0)
        {
            /*ACK*/
            if (*pSW1 == 0x61)
            {
                cmd[0] = 0x00;
                cmd[1] = 0xc0;
                cmd[2] = 0x00;
                cmd[3] = 0x00;
                cmd[4] = *pSW2;
                cmd_len = 5;
                len = GxSmc_SendCmd(handle, cmd, cmd_len);
                if(len != cmd_len)
                    SMC_ERR("Send cmd again failure\n");

                len = GxSmc_GetReply(handle, thiz_reply_buf, MAX_REPLY_BYTES);
                if(len == 2)
                {
                    *RespLen = 0;
                    *pSW1 = thiz_reply_buf[len - 2];
                    *pSW2 = thiz_reply_buf[len - 1];
                    return 0;
                }
                else if (len > 2)
                {
                    *pSW1 = thiz_reply_buf[len - 2];
                    *pSW2 = thiz_reply_buf[len - 1];
                    if (thiz_reply_buf[0] == cmd[1])
                    {
                        /*INS*/
                        //SMC_ERR("Sw1 is the INS\n");
                        *RespLen = (unsigned short)len - 3;
                        memcpy(pResponse, thiz_reply_buf + 1, *RespLen);
                    }
                    else
                    {
                        *RespLen = (unsigned short)len - 2;
                        memcpy(pResponse, thiz_reply_buf, *RespLen);
                    }
                    //SMC_ERR("sw1 = %#x, sw2 = %#x\n",thiz_reply_buf[len - 2], thiz_reply_buf[len - 1]);
                    return 0;
                }
                else
                {
                    SMC_ERR("Get reply failure\n");
                    return -1;
                }
            }
        }
    }
    return ret;
}

/**
* @brief       获取智能卡状态
* @param [in]  handle     智能卡句柄
* @param [out]  state     智能卡状态
* @return      int >=0 执行成功;<0 执行失败
* @note   模块使用者必须及时调用此函数以获取智能卡状态
          并完成相关的操作.响应的实时性由使用者调用此函数
          的实时性决定.
*/
int GxSmc_GetStatus(handle_t handle, GxSmcCardStatus *state)
{
    int32_t ret = 0;
    GxSciCardStatus status = 0;

    SMC_CHECK_P(state, -1);

    ret = GxSci_GetStatus(&status);

    if(GXSCI_CARD_INIT == status)
        *state = GXSMC_CARD_INIT;
    else if(GXSCI_CARD_IN == status)
        *state = GXSMC_CARD_IN;
    else if (GXSCI_CARD_OUT == status)
        *state = GXSMC_CARD_OUT;
    else
        return -1;

    return ret;
}

/**
* @brief       关闭智能卡
* @param [in]  handle     智能卡句柄
* @return      int >=0 执行成功;<0 执行失败
*/
int GxSmc_Close(handle_t handle)
{
    return GxSci_Close();
}

/**
* @brief       解析智能卡的复位应答串
* @param [in]  Atr_p       应答内容指针
* @param []    AtrLength 应答内容的长度
* @param [out]    etu 输出相应的值
* @param [out]    egt 输出相应的值
* @param [out]    tgt 输出相应的值
* @param [out]    wdt 输出相应的值
* @param [out]    twdt 输出相应的值
* @param [out]    t01 输出相应的值,0 = T0,1 = T1

* @return   int >=0 执行成功;<0 执行失败
* @note     -1:SMART_PARSE_TS_ERROR       : TS字符不为0x3B或者0x3F
            -2:SMART_PARSE_LENGTH_ERROR   : AtrLength不合法
            -3:SMART_PARSE_TCK_ERROR      : 校检字符Tck不正确
            -4:SMART_PARSE_TA1_ERROR      : TA(1)字符中含有未使用的编码
*/

int GxSmc_ParseAtr(const uint8_t *Atr_p,
                uint32_t AtrLength,
                uint32_t *etu,
                uint32_t *egt,
                uint32_t *tgt,
                uint32_t *wdt,
                uint32_t *twdt,
                uint32_t *t01)
{
    int32_t ret = 0;
    GxSciTimeParams time = {0};

    ret = GxSci_AnalyseAtr((uint8_t *)Atr_p, AtrLength, &time);

    *etu  = time.etu;
    *egt  = time.egt;
    *tgt  = time.tgt;
    *wdt  = time.wdt;
    *twdt = time.twdt;
    *t01  = time.protocol;

    return ret;
}
/* End of file -------------------------------------------------------------*/
