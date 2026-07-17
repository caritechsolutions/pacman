#ifndef __GXCAS_SMARTCARD_H__
#define __GXCAS_SMARTCARD_H__
#include "gxcas.h"
#include "autoconf.h"

#include "gxtype.h"
#ifdef CONFIG_SCI_NEW
#include "gxsci_api.h"
#endif
#ifdef CONFIG_SCI_OLD
/**智能卡通讯重发协议*/
typedef enum {
	ENABLE_REPEAT_WHEN_ERR, /*校验错误时重发一个字节,这样每次交互都会多发一个应答位(T0)*/
	DISABLE_REPEAT_WHEN_ERR /*校验错误时也不重发,一般建议使用此模式(T1)*/
} GxSmcRepeat_t;
#endif

/**智能卡通讯停止位*/
typedef enum {
	GXSMC_STOPLEN_0BIT,
	GXSMC_STOPLEN_1BIT,
	GXSMC_STOPLEN_1D5BIT,
	GXSMC_STOPLEN_2BIT
}GxSmcStopLen;

/**智能卡通讯数据反转*/
typedef enum {
	GXSMC_DATA_CONV_DIRECT       = 0,
	GXSMC_DATA_CONV_INVERSE      = 1
}GxSmcDataConv;

/**智能卡通讯校验*/
typedef enum {
	GXSMC_PARITY_ODD             = 0,
	GXSMC_PARITY_EVEN            = 1
}GxSmcParityType;

/**智能卡状态*/
typedef enum {
	GXSMC_CARD_INIT              = 0,
	GXSMC_CARD_IN                = 1,
	GXSMC_CARD_OUT               = 2
}GxSmcCardStatus;

typedef enum
{
	GXSMC_PROTCL_T0 = 0,
	GXSMC_PROTCL_T1 = 1,
	GXSMC_PROTCL_T14 = 14,
	GXSMC_PROTCL_INVALID,
}GxSmcProtocol;

/**智能卡接口通讯时间参数*/
typedef struct {
	uint32_t                baud_rate;  /*智能卡工作频率,比如9600*372*/
	uint32_t                etu;
	uint32_t                egt;
	uint32_t                tgt;
	uint32_t                wdt;
	uint32_t                twdt;
	GxSmcProtocol           protocol;
}GxSmcTimeParams;

typedef struct {
#ifdef CONFIG_SCI_OLD
	GxSmcRepeat_t      protocol;
#else
	GxSciRepeatMode    protocol;
#endif
	GxSmcStopLen       stop_len;
	GxSmcDataConv      io_conv;
	GxSmcParityType    parity;
	uint32_t           default_etu;/* 默认etu值 */
	uint32_t           debug;/* 打开驱动的调试信息 */
}GxSmcParams;

typedef void (*GxSmcChange)(uint32_t flag);

int32_t GxCas_Smc_Init(GxCasSmcPol detect_pole, GxCasSmcPol vcc_pole);
int32_t GxCas_Smc_Open(GxSmcParams *param,GxSmcChange change);
int32_t GxCas_Smc_Reset(uint8_t *AtrBuf, int32_t BufSize, int32_t *RetLen);
int32_t GxCas_Smc_Config(GxSmcTimeParams *time);
int32_t GxCas_Smc_SendCmd(uint8_t *Cmd,size_t CmdLen,size_t timeout_ms);
int32_t GxCas_Smc_GetReply(uint8_t *ReplyBuf, size_t BufSize,size_t timeout_ms);
int32_t GxCas_Smc_Apdu(uint8_t *pbyCommand, uint32_t wCommandLen, uint8_t *pbyReply, uint32_t *pwReplyLen);
int32_t GxCas_Smc_Close(void);
int32_t GxCas_Smc_AnalyseAtr(uint8_t *atr,uint32_t len,GxSmcTimeParams *time);
#endif
#ifdef CONFIG_SCI_NEW
/**
 * 智能卡工作模式
 */
typedef enum {
    GXSMC_MODE_NORMAL,  ///< 普通模式，读操作时间必定大于 WDT 超时时间
    GXSMC_MODE_BOOST    ///< 增强模式，读操作基于当前智能卡传输速率，快于普通模式
} GxSmcMode;
int32_t GxCas_Smc_ATR_GetInterfaceByte_TA(uint8_t* AtrBuf,int32_t AtrLen, uint8_t *ib);
int32_t GxCas_Smc_SetMode(GxSmcMode mode);
int32_t GxCas_Smc_NegotiatePPS(uint8_t *atr, uint32_t len);
int32_t GxCas_Smc_CACardSetup(uint8_t *atr, uint32_t len);
int32_t GxCas_Smc_Poweroff(void);
#endif



