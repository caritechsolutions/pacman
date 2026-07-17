#include "autoconf.h"
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "gxsci_atr.h" 
#include "gxcas.h"
#include "gxcas_smartcard.h"
#include "gxcas_dbg.h"
#include "gxcore.h"

#ifdef CONFIG_SCI_OLD
#ifdef   ECOS_OS
#include "cyg/io/smartcard_params.h"
#define SMC_SET_CD_POLARITY		SET_CD_POLARITY
#define SMC_SET_VCCEN_POLARITY	        SET_VCCEN_POLARITY
#define SMC_SET_OPEN_PARAMS		SET_OPEN_PARAMS
#define SMC_CARD_COLD_RESET		CARD_COLD_RESET
#define SMC_GET_CARD_STATUS		GET_CARD_STATUS
#define SMC_SET_OUTPUT_DEBUG            SET_OUTPUT_DEBUG_INFO
#endif

#ifdef LINUX_OS
#include "smartcard_params.h"
#define SMC_SET_CD_POLARITY		IOCTL_SET_CD_POLARITY
#define SMC_SET_VCCEN_POLARITY	        IOCTL_SET_VCCEN_POLARITY
#define SMC_SET_OPEN_PARAMS		IOCTL_SET_PARAMETERS
#define SMC_CARD_COLD_RESET		IOCTL_SET_RESET
#define SMC_GET_CARD_STATUS		IOCTL_GET_IS_CARD_PRESENT
#define SMC_SET_OUTPUT_DEBUG            IOCTL_SET_DEBUG


static int select_write(int fd,char *buff,int len,int timeout)
{
	fd_set wfds;
	fd_set erfds;
	struct timeval tv;
	int32_t select_ret;
	FD_ZERO(&wfds);
	FD_ZERO(&erfds);
	FD_SET(fd,&wfds);
	FD_SET(fd,&erfds);
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;
	select_ret = select(fd + 1,NULL,&wfds,&erfds,&tv);
	if(select_ret < 0){
		CAS_ERR(SMC,"select write error");
		return -1;
	} else if (select_ret == 0){
		CAS_ERR(SMC,"select write timeout");
		return -1;
	}else {
		return write(fd,buff,len);
	}
}

static int select_read(int fd,char *buff,int len,int timeout)
{
	fd_set rfds;
	fd_set erfds;
	struct timeval tv;
	int32_t select_ret;
	FD_ZERO(&rfds);
	FD_ZERO(&erfds);
	FD_SET(fd,&rfds);
	FD_SET(fd,&erfds);
	tv.tv_sec = timeout/1000;
	tv.tv_usec = (timeout%1000)*1000;
	select_ret = select(fd + 1,&rfds,NULL,&erfds,&tv);
	if (select_ret < 0){
		CAS_ERR(SMC,"select read error");
		return -1;
	} else if(select_ret == 0) {
		CAS_ERR(SMC,"select read timeout");
		return -1;
	} else {
		return read(fd,buff,len);
	}
}

#endif
static GxSmcCardStatus card_status = GXSMC_CARD_INIT;

#define DEFAULT_BAUD_RATE       (9600 * 372)
#define DEFAULT_ETU	        (372)
#define DEFAULT_EGT	        (DEFAULT_ETU * 12)
#define DEFAULT_WDT	        (9600* DEFAULT_ETU)
#define DEFAULT_TGT	        (0)
#define DEFAULT_TWDT            (0x8FFFFF)

struct gxcas_smc{
	int32_t                 fd;
	handle_t                lock;
	GXSMART_OpenParams_t    param;
	GXSMART_DetectPol_t     detect;
	GXSMART_VccPol_t        vcc;
	GxSmcChange             do_status_change;
};

static struct gxcas_smc _smc;


int32_t GxCas_Smc_GetStatus (GxSmcCardStatus *status)
{
	int32_t ret;
	GXSMART_CardStatus_t  getstatus;

	if(status == NULL){
		CAS_ERR(SMC,"param satus err");
		return -1;
	}
	GxCore_MutexLock(_smc.lock);
	ret = ioctl(_smc.fd, SMC_GET_CARD_STATUS, (void*)&getstatus, sizeof(GXSMART_CardStatus_t));
	if(ret < 0) {
		CAS_ERR(SMC,"Get satus err");
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
	*status = getstatus;
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

static void _smc_task(void *args)
{
	int32_t ret = 0;
	GxSmcCardStatus status;
	while(1) {
		if (_smc.fd >= 0){
			ret = GxCas_Smc_GetStatus(&status);
			if (card_status != status) {
				card_status = status;
				if (status == GXSMC_CARD_IN) {
					_smc.do_status_change(TRUE);
				} else if (status == GXSMC_CARD_OUT) {
					_smc.do_status_change(FALSE);
				}
			}
		}
		GxCore_ThreadDelay(200);
	}

}

int32_t GxCas_Smc_Init(GxCasSmcPol detect_pole, GxCasSmcPol vcc_pole)
{
	if (detect_pole == GXCAS_SMC_LOW_LEVEL)
		_smc.detect = GXSMART_DETECT_LOW_LEVEL;
	else
		_smc.detect = GXSMART_DETECT_HIGH_LEVEL;

	if (vcc_pole == GXCAS_SMC_LOW_LEVEL)
		_smc.vcc = GXSMART_VCC_LOW_LEVEL;
	else
		_smc.vcc = GXSMART_VCC_HIGH_LEVEL;

	_smc.fd = -1;
	GxCore_MutexCreate(&_smc.lock);
	return 0;
}

int32_t GxCas_Smc_Open(GxSmcParams *param,GxSmcChange change)
{
	int32_t ret;
	handle_t card_thread;

	if(param == NULL || change == NULL ) {
		CAS_DBG(SMC,"Pls Init Smc First");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);

	if(_smc.fd > 0){
		GxCore_MutexUnlock(_smc.lock);
		return 0;
	}

	_smc.fd = open("/dev/gxsmartcard0", O_RDWR);
	if(_smc.fd < 0)
		goto err;

	_smc.param.IoConv        = param->io_conv;
	_smc.param.ParityType    = param->parity;
	_smc.param.Protocol      = param->protocol;
	_smc.param.StopLen       = param->stop_len;
	_smc.param.Etu           = param->default_etu;
	_smc.param.AutoEtu	 = 1;
	_smc.param.AutoIoConvAndParity = 1;

	ret = ioctl(_smc.fd, SMC_SET_CD_POLARITY, &(_smc.detect), sizeof(GXSMART_DetectPol_t));
	if(ret<0)goto err;

	ret = ioctl(_smc.fd, SMC_SET_VCCEN_POLARITY, &(_smc.vcc), sizeof(GXSMART_VccPol_t));
	if(ret<0)goto err;

	if (param->debug){
		int enable_debug = 1;
		ret = ioctl(_smc.fd, SMC_SET_OUTPUT_DEBUG, &enable_debug);
		if(ret<0)goto err;
	}

	_smc.do_status_change = change;
	GxCore_ThreadCreate("smart",&card_thread, _smc_task, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);

	GxCore_MutexUnlock(_smc.lock);
	return 0;

err:
	CAS_ERR(SMC,"Config device failure");
	GxCore_MutexUnlock(_smc.lock);
	return -1;
}


int GxCas_Smc_Reset(uint8_t* AtrBuf,int32_t BufSize,int32_t *RetLen)
{
	int                     ret;
	GXSMART_OpenParams_t    open_param;
	GXSMART_AtrParams_t     atr_params ={0,};

	if(AtrBuf == NULL || BufSize < 33)
	{
		CAS_ERR(SMC,"Buffer is NULL or size < 33, maybe can't carry the atr string");
		goto reset_err;
	}

	GxCore_MutexLock(_smc.lock);
	open_param.Protocol            = _smc.param.Protocol;
	open_param.Etu                 = _smc.param.Etu;
	open_param.StopLen             = _smc.param.StopLen;
	open_param.IoConv              = _smc.param.IoConv;
	open_param.ParityType          = _smc.param.ParityType;
	open_param.AutoEtu	       = _smc.param.AutoEtu;
	open_param.AutoIoConvAndParity = _smc.param.AutoIoConvAndParity;

	open_param.BaudRate         = DEFAULT_BAUD_RATE;
	open_param.TimeParams.Egt   = DEFAULT_EGT;
	open_param.TimeParams.Tgt   = DEFAULT_TGT;
	open_param.TimeParams.Wdt   = DEFAULT_WDT;
	open_param.TimeParams.Twdt  = DEFAULT_TWDT;

	ret = ioctl(_smc.fd,SMC_SET_OPEN_PARAMS, &open_param, sizeof(GXSMART_OpenParams_t));
	if(ret<0) goto reset_err;

	atr_params.priv = AtrBuf;
	ret = ioctl(_smc.fd, SMC_CARD_COLD_RESET, &atr_params, sizeof(GXSMART_AtrParams_t));
	if(ret<0) goto reset_err;
	*RetLen = atr_params.len;

#ifdef LINUX_OS
	ret = select_read(_smc.fd, (char*)AtrBuf, 33,10000);
	if(ret<0)goto reset_err;
	*RetLen = ret;
#endif

	GxCore_MutexUnlock(_smc.lock);
	return 1;

reset_err:
	*RetLen = 0;
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

int32_t GxCas_Smc_Config(GxSmcTimeParams* time)
{
	int                     ret;
	GXSMART_OpenParams_t    open_param;
	if(time == NULL){
		CAS_ERR(SMC,"param is NULL");
	}

	GxCore_MutexLock(_smc.lock);

	open_param.AutoEtu             = _smc.param.AutoEtu;
	open_param.AutoIoConvAndParity = _smc.param.AutoIoConvAndParity;
	open_param.Protocol            = _smc.param.Protocol;
	open_param.StopLen             = _smc.param.StopLen;
	open_param.IoConv              = _smc.param.IoConv;
	open_param.ParityType          = _smc.param.ParityType;
	open_param.BaudRate            = time->baud_rate;
	open_param.Etu                 = time->etu;
	open_param.TimeParams.Egt      = time->egt;
	open_param.TimeParams.Tgt      = time->tgt;
	open_param.TimeParams.Wdt      = time->wdt;
	open_param.TimeParams.Twdt     = time->twdt;

	ret = ioctl(_smc.fd, SMC_SET_OPEN_PARAMS, &open_param);
	if(ret < 0) {
		CAS_ERR(SMC,"Config smc failure");
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
	_smc.param.BaudRate         = time->baud_rate;
	_smc.param.Etu              = time->etu;
	_smc.param.TimeParams.Egt   = time->egt;
	_smc.param.TimeParams.Tgt   = time->tgt;
	_smc.param.TimeParams.Wdt   = time->wdt;
	_smc.param.TimeParams.Twdt  = time->twdt;

	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

int32_t GxCas_Smc_SendCmd(uint8_t *Cmd,size_t CmdLen,size_t timeout_ms)
{
	int ret;

	if(Cmd == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	if (card_status == GXSMC_CARD_OUT) {
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
#ifdef LINUX_OS
	ret = select_write(_smc.fd,(char*)Cmd,CmdLen,timeout_ms);
#else
	ret = write(_smc.fd, (uint8_t*)Cmd, CmdLen);
#endif
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_GetReply(uint8_t *ReplyBuf, size_t BufSize,size_t timeout_ms)
{
	int ret;

	if(ReplyBuf == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	if (card_status == GXSMC_CARD_OUT) {
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
#ifdef LINUX_OS
	ret = select_read(_smc.fd,(char*)ReplyBuf,BufSize,timeout_ms);
#else
	ret = read(_smc.fd, ReplyBuf, BufSize);
#endif
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0)
		return -1;
	else
		return ret;
}

/**
 * @brief       处理智能卡命令
 *
 * @param [in]  CmdLen    处理的命令长度
 * @param [in]  pCmd      需要处理的命令指针
 * @param [out] pResponse 输出的响应缓存
 * @param [out] RespLen   处理完成后返回的响应长度
 * @param [out] pSW1      处理完成后返回的状态字1
 * @param [out] pSW2      处理完成后返回的状态字2
 *
 * @return      int >=0 执行成功;<0 执行失败
 *
 * @note  此函数完成了简单的apdu部分处理,比如0x61, sw1 == INS,等待
 *        等等.相当于GxCas_Smc_SendCmd,GxCA_SmcGetReply两函数的再次组合,
 *        使用时请根据cas的情况作出选择.
 */
static int32_t GxCas_Smc_SendReceiveData( size_t CmdLen, const uint8_t *pCmd,
		uint8_t *pResponse, size_t *RespLen, uint8_t *pSW1, uint8_t *pSW2)
{
	int  len = 0;
#define SMC_MAX_REPLY			(1024)
#define	SMC_MAX_CMD_LEN			(512)
	uint8_t cmd[SMC_MAX_CMD_LEN];
	static uint8_t reply[SMC_MAX_REPLY];

	if((pCmd == NULL) || (pResponse == NULL)|| (RespLen == NULL)  ||
			(pSW1 == NULL)    || (pSW2 == NULL) || card_status == GXSMC_CARD_OUT)
		return -1;

	memset(cmd,0,sizeof(cmd));
	memcpy(cmd,pCmd,CmdLen);

	CAS_DBG(SMC, "Send Command ");
	CAS_DBG(SMC, "Command len = %d:",CmdLen);
	CAS_DBG(SMC, "Command info:");
	CAS_DUMP(SMC, cmd, CmdLen, "CMD");

	*RespLen = 0;
	*pSW1 = *pSW2 = 0;
	if(CmdLen <=5 ) {
		CAS_DBG(SMC,"Cmdlen <= 5!");
		len = GxCas_Smc_SendCmd(cmd, CmdLen,3000);
		if (len == CmdLen) {
			CAS_DBG(SMC,"Send Command OK");
			len = GxCas_Smc_GetReply(reply, SMC_MAX_REPLY,10000);
			CAS_DBG(SMC,"Reply len = %d",len);
			CAS_DBG(SMC,"sw1 = %#x, sw2 = %#x",reply[len - 2],reply[len - 1]);
			*pSW1 = reply[len - 2];
			*pSW2 = reply[len - 1];
			if (len > 2) {
				if (reply[0] == cmd[1]) { /*INS*/
					CAS_DBG(SMC,"Sw1 is the INS");
					*RespLen = (unsigned short)len - 3;
					memcpy(pResponse, reply + 1, *RespLen);
				} else {
					*RespLen = (unsigned short)len - 2;
					memcpy(pResponse, reply, *RespLen);
				}
			} else if (len == 2) {
				return 0;
			} else {
				CAS_ERR(SMC,"Get reply failure");
				return -1;
			}
			CAS_DBG(SMC, "SW1=%#x,SW2=%#x", *pSW1, *pSW2);
			CAS_DBG(SMC, "Reply info = %d:", *RespLen);
			CAS_DUMP(SMC, pResponse, *RespLen, "reply");
			return 0;
		}
		CAS_ERR(SMC,"Send cmd failure");
		return -1;
	} else {
		CAS_DBG(SMC, "Cmdlen > 5!send 5 bytes first!");
		len = GxCas_Smc_SendCmd(cmd, 5, 3000);
		if (len == 5) {
			CAS_DBG(SMC,"Send Command OK");
			len = GxCas_Smc_GetReply(reply, SMC_MAX_REPLY,10000);
			CAS_DUMP(SMC, reply, len, "reply");
			if(cmd[1] == reply[0]) {
				CAS_DBG(SMC,"Sw1 is the INS");
				len = GxCas_Smc_SendCmd(cmd  + 5, CmdLen - 5,3000);
				if(len != CmdLen - 5)
					CAS_ERR(SMC,"Send cmd again failure");

				len = GxCas_Smc_GetReply(reply, SMC_MAX_REPLY,10000);
				CAS_DBG(SMC,"Get reply len = %d",len);
				if(len > 0)
				{
					if(2 == len) {
						*pSW1 = reply[0];
						*pSW2 = reply[1];
						CAS_DBG(SMC,"SW1=%#x,SW2=%#x", *pSW1, *pSW2);
						return 0;
					} else if(len > 2) {
						*pSW1 = reply[len - 2];
						*pSW2 = reply[len - 1];
						memcpy(pResponse, reply, len - 2);
						*RespLen = len - 2;
						CAS_DBG(SMC,"SW1=%#x,SW2=%#x", *pSW1, *pSW2);
						return 0;
					} else {
						CAS_ERR(SMC,"Get reply failure");
						return -1;
					}
				}

				CAS_ERR(SMC,"Get reply failure");
				return -1;
			} else if (len == 2){
				*pSW1 = reply[len - 2];
				*pSW2 = reply[len - 1];
				CAS_DBG(SMC,"SW1=%#x,SW2=%#x", *pSW1, *pSW2);
				return 0;
			}
			return 0;
		}
	}
	CAS_DBG(SMC,"Send Command err");
	return -1;
}


int32_t GxCas_Smc_Apdu(uint8_t *pbyCommand, uint32_t wCommandLen, uint8_t *pbyReply, uint32_t *pwReplyLen)
{
	uint8_t sw1,sw2;
	size_t len;
	size_t cmd_len = wCommandLen;

	CAS_DUMP(SMC, pbyCommand, wCommandLen, "IN");
	while(1) {
		if (GxCas_Smc_SendReceiveData(cmd_len,pbyCommand,pbyReply,&len,&sw1,&sw2) >= 0) {
#if 0

			if (sw1 == 0x61) {
				uint8_t cmd[32] = {0x00, 0xc0, 0x00, 0x00,};
				cmd[4] = sw2;
				cmd_len = 5;

				if (GxCas_Smc_SendReceiveData(cmd_len, cmd, pbyReply, &len, &sw1, &sw2) < 0)
					break;
			}
#endif
			if (sw1 < 0x60)
				continue;

			*pwReplyLen = (uint16_t)len;
			pbyReply[len]=sw1;
			pbyReply[len+1]=sw2;
			*pwReplyLen +=2;
			CAS_DUMP(SMC, pbyReply, *pwReplyLen, "OUT");
			return 0;
		} else
			break;
	}

	*pwReplyLen = 0;
	return -1;

}

int32_t GxCas_Smc_Close(void)
{
	GxCore_MutexLock(_smc.lock);
	close(_smc.fd);
	_smc.fd = -1;
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}
#endif
#ifdef CONFIG_SCI_NEW
#include "gxsci_api.h"

static GxSmcCardStatus card_status = GXSMC_CARD_INIT;

#define DEFAULT_BAUD_RATE       (9600 * 372)
#define DEFAULT_ETU	        (372)
#define DEFAULT_EGT	        (DEFAULT_ETU * 12)
#define DEFAULT_WDT	        (9600* DEFAULT_ETU)
#define DEFAULT_TGT	        (0)
#define DEFAULT_TWDT            (0x8FFFFF)

struct gxcas_smc{
	int32_t                 fd;
	handle_t                lock;
//	GXSMART_OpenParams_t    param;
	GxSciPol                detect;
	GxSciPol                vcc;
	GxSmcChange             do_status_change;
};

static struct gxcas_smc _smc;


int32_t GxCas_Smc_GetStatus (GxSmcCardStatus *status)
{
	int32_t ret;
	GxSciCardStatus getstatus;

	if(status == NULL){
		CAS_ERR(SMC,"param satus err");
		return -1;
	}
	GxCore_MutexLock(_smc.lock);
	ret = GxSci_GetStatus(&getstatus);
	if(ret < 0) {
		CAS_ERR(SMC,"Get satus err");
		return -1;
	}
	*status = getstatus;
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

static void _smc_task(void *args)
{
	int32_t ret = 0;
	GxSmcCardStatus status;
	while(1) {
		if (_smc.fd >= 0){
			ret = GxCas_Smc_GetStatus(&status);
			if (card_status != status) {
				card_status = status;
				if (status == GXSMC_CARD_IN) {
					_smc.do_status_change(TRUE);
				} else if (status == GXSMC_CARD_OUT) {
					_smc.do_status_change(FALSE);
				}
			}
		}
		GxCore_ThreadDelay(200);
	}

}

int32_t GxCas_Smc_Init(GxCasSmcPol detect_pole, GxCasSmcPol vcc_pole)
{
	if (detect_pole == GXCAS_SMC_LOW_LEVEL)
		_smc.detect = GXSCI_LOW_LEVEL;
	else
		_smc.detect = GXSCI_HIGH_LEVEL;

	if (vcc_pole == GXCAS_SMC_LOW_LEVEL)
		_smc.vcc = GXSCI_LOW_LEVEL;
	else
		_smc.vcc = GXSCI_HIGH_LEVEL;

	_smc.fd = -1;
	GxCore_MutexCreate(&_smc.lock);
	return 0;
}

int32_t GxCas_Smc_Open(GxSmcParams *param,GxSmcChange change)
{
	int32_t ret;
	handle_t card_thread;

	if(param == NULL || change == NULL ) {
		CAS_DBG(SMC,"Pls Init Smc First");
		return -1;
	}


	if(_smc.fd > 0){
		return 0;
	}

	_smc.fd = 1;
	GxCore_MutexLock(_smc.lock);
	ret = GxSci_Open(_smc.detect, _smc.vcc);
	if(ret<0)goto err;

	_smc.do_status_change = change;
	GxCore_ThreadCreate("smart",&card_thread, _smc_task, NULL, 10 * 1024, GXOS_DEFAULT_PRIORITY);

	GxCore_MutexUnlock(_smc.lock);
	return 0;

err:
	CAS_ERR(SMC,"Config device failure");
	GxCore_MutexUnlock(_smc.lock);
	return -1;
}

int GxCas_Smc_Reset(uint8_t* AtrBuf,int32_t BufSize,int32_t *RetLen)
{
	int                     ret;

	if(AtrBuf == NULL || BufSize < 33 || RetLen == NULL)
	{
		CAS_ERR(SMC,"Buffer or RetLen is NULL or size < 33, maybe can't carry the atr string");
		goto reset_err;
	}

	GxCore_MutexLock(_smc.lock);
	ret = GxSci_Reset(AtrBuf, BufSize, RetLen);
	if(ret<0) goto reset_err;

	GxCore_MutexUnlock(_smc.lock);
	return 1;

reset_err:
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

int32_t GxCas_Smc_ATR_GetInterfaceByte_TA(uint8_t* AtrBuf,int32_t AtrLen, uint8_t *ib)
{
	GxSciATR _atr = {0};
	int	ret;

	if((AtrBuf == NULL) || (ib == NULL)) {
		CAS_ERR(SMC,"Buffer or RetLen is NULL or size < 33, maybe can't carry the atr string");
		goto reset_err;
	}

	GxCore_MutexLock(_smc.lock);
	if (0 == GxSci_ATR_InitFromArray(&_atr, AtrBuf, AtrLen)) {
		ret = GxSci_ATR_GetInterfaceByte(&_atr, 1, GXSCI_ATR_INTERFACE_BYTE_TA, ib);
		if(ret<0) goto reset_err;
		
		GxCore_MutexUnlock(_smc.lock);
		return 0;
	}

reset_err:
	GxCore_MutexUnlock(_smc.lock);
	return -1;
}

int32_t GxCas_Smc_Config(GxSmcTimeParams* time)
{
	int                     ret;
	GxSciTimeParams config_time;
	if(time == NULL){
		CAS_ERR(SMC,"param is NULL");
	}

	GxCore_MutexLock(_smc.lock);

	config_time.clock               = time->baud_rate;
	config_time.etu                 = time->etu;
	config_time.egt      = time->egt;
	config_time.tgt      = time->tgt;
	config_time.wdt      = time->wdt;
	config_time.twdt     = time->twdt;

	ret = GxSci_Config(&config_time);
	if(ret < 0) {
		CAS_ERR(SMC,"Config smc failure");
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}

	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

int32_t GxCas_Smc_SendCmd(uint8_t *Cmd,size_t CmdLen,size_t timeout_ms)
{
	int ret;

	if(Cmd == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	if (card_status == GXSMC_CARD_OUT) {
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
	ret = GxSci_Write(Cmd, CmdLen, timeout_ms);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_GetReply(uint8_t *ReplyBuf, size_t BufSize,size_t timeout_ms)
{
	int ret;

	if(ReplyBuf == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	if (card_status == GXSMC_CARD_OUT) {
		GxCore_MutexUnlock(_smc.lock);
		return -1;
	}
	ret = GxSci_Read(ReplyBuf, BufSize, timeout_ms);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0)
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_Apdu(uint8_t *pbyCommand, uint32_t wCommandLen, uint8_t *pbyReply, uint32_t *pwReplyLen)
{
	size_t len;
	uint8_t sw1,sw2;
	int32_t ret = 0;

	if(pbyCommand == NULL || pbyReply == NULL || pwReplyLen == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}
	CAS_DUMP(SMC, pbyCommand, wCommandLen, "IN");
	ret = GxSci_Apdu(pbyCommand, wCommandLen, pbyReply, &len, &sw1, &sw2);
	*pwReplyLen = (uint16_t)len;
	pbyReply[len]=sw1;
	pbyReply[len+1]=sw2;
	*pwReplyLen +=2;

	CAS_DUMP(SMC, pbyReply, *pwReplyLen, "OUT");

	return ret;
}

int32_t GxCas_Smc_Close(void)
{
	GxCore_MutexLock(_smc.lock);
	GxSci_Close();
	_smc.fd = -1;
	GxCore_MutexUnlock(_smc.lock);
	return 0;
}

int32_t GxCas_Smc_Setup(GxSmcParams *param)
{
	int32_t ret = 0;
	GxSciParams setup_param = {0,};
	if(param == NULL){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	setup_param.io_conv                 = param->io_conv;
	setup_param.parity                  = param->parity;
	setup_param.protocol                = param->protocol;
	setup_param.stop_len                = param->stop_len;
	setup_param.default_etu             = param->default_etu;
	setup_param.auto_etu                = 1;
	setup_param.auto_io_conv_and_parity = 1;
	ret = GxSci_Setup(&setup_param);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_SetMode(GxSmcMode mode)
{
	int32_t ret = 0;
	GxSciMode sci_mode = mode;

	GxCore_MutexLock(_smc.lock);
	ret = GxSci_SetMode(sci_mode);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_NegotiatePPS(uint8_t *atr, uint32_t len)
{
	int32_t ret = 0;
	if(atr  == NULL || len > 33){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	ret = GxSci_NegotiatePPS(atr, len);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_CACardSetup(uint8_t *atr, uint32_t len)
{
	int32_t ret = 0;
	if(atr  == NULL || len > 33){
		CAS_ERR(SMC,"param is NULL");
		return -1;
	}

	GxCore_MutexLock(_smc.lock);
	ret = GxSci_CACardSetup(atr, len);
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}

int32_t GxCas_Smc_Poweroff(void)
{
	int32_t ret = 0;

	GxCore_MutexLock(_smc.lock);
	ret = GxSci_PowerOff();
	GxCore_MutexUnlock(_smc.lock);
	if(ret < 0 )
		return -1;
	else
		return ret;
}
#endif

