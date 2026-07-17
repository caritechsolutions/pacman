#include "app.h"
#include "app_module.h"
#include "app_msg.h"
#include "app_send_msg.h"
#include "app_pop.h"
#include "app_default_params.h"

#ifdef ECOS_OS
#include "gxcore_hw_bsp.h"
#include <cyg/ppp/ppp.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include "module/app_gprs.h"

#if NETWORK_SUPPORT && GPRS_SUPPORT

#define CMD_MAX_LEN             1024
#define RESULT_MAX_LEN          1023

#define GPRS_RETURN_TAIL_OK      "OK"
#define GPRS_RETURN_CR           "\r\n"
#define GPRS_RETURN_HEAD_CPIN    "+CPIN:"
#define GPRS_RETURN_HEAD_CREG	"+CREG:"
#define GPRS_RETURN_HEAD_CSQ    "CSQ:"

static int s_uart_handle = -1;
static handle_t s_gprs_mutex = -1;
static handle_t s_gprs_printf_mutex = -1;
static char cmd_buf[CMD_MAX_LEN];
static char buf[RESULT_MAX_LEN+1];
static bool force_exit = false;

static int gx_uart_init(unsigned int Baudrate)
{
	int Ret = 0;
	unsigned int BRate = Baudrate;

	if(s_uart_handle > 0)
	{
		return -1;
	}

	s_uart_handle = open(GPRS_DEVICE_PATH,O_RDWR|O_NONBLOCK);
	if(s_uart_handle < 0)
	{
		return -1;
	}

	Ret = ioctl(s_uart_handle,0x0191/*CYG_IO_SET_CONFIG_SERIAL_BAUD_RATE*/, (void*)&BRate);
	if(Ret < 0)
	{
		return -1;
	}

	return s_uart_handle;
}

static int gx_uart_receive(char *Buffer, unsigned int byteToRead)
{
	int RealByteRead = 0;

	if((Buffer == NULL) || (s_uart_handle < 0))
	{
		return -1;
	}

	RealByteRead = read(s_uart_handle,Buffer,byteToRead);
	return RealByteRead;
}

static int gx_uart_send(char *Buffer, unsigned int byteToWrite)
{
	int RealByteGet = 0;

	if((Buffer == NULL) || (s_uart_handle <= 0))
	{
		return -1;
	}

	RealByteGet = write(s_uart_handle, Buffer, byteToWrite);
	return RealByteGet;
}

static int gx_uart_close(void)
{
	if(s_uart_handle <= 0)
	{
		return -1;
	}
	close(s_uart_handle);
	s_uart_handle = -1;
	return 0;
}

#define USB_PRINT_BUFFER_LEN (128*1024)
#define USB_PRINT_PATH "/mnt/usb01/gxlog.txt"
static char usb_print_buffer[USB_PRINT_BUFFER_LEN];
void gprs_printf(const char *fmt, ...)
{
	va_list args;
	handle_t logfile_fd=-1;
	GxCore_MutexLock(s_gprs_printf_mutex);
	va_start(args, fmt);
	vsnprintf(usb_print_buffer, sizeof(usb_print_buffer), fmt, args);
	va_end(args);
	app_log_debug("%s", usb_print_buffer);

	logfile_fd=GxCore_Open(USB_PRINT_PATH,"a+");
	if(logfile_fd>0)
	{
		GxCore_Write(logfile_fd,usb_print_buffer,strlen(usb_print_buffer),1);
		GxCore_Sync(logfile_fd);
		GxCore_Close(logfile_fd);
	}
	GxCore_MutexUnlock(s_gprs_printf_mutex);
}

void gprs_display(char *string)
{
	//if(GUI_CheckPrompt("uart_displayxx") == GXCORE_SUCCESS)
	//{
	//	GUI_EndPrompt("uart_displayxx");
	//}

	//if(GUI_CheckPrompt("uart_displayxx") != GXCORE_SUCCESS)
	//{
	//	GUI_CreatePrompt(100, 100, "uart_displayxx", "uart_display", string, "ontop");
	//}
}

static gx_gprs_ret gx_gprs_cmd_exec(GxExecAtCmd_t* cmd, GxExecAtReturn_t* ret)
{
	int32_t i = 0;
	uint32_t len = 0, expect_head_len = 0, expect_tail_len = 0, expect_len = 0;
	char ret_buf[512];

	if (cmd == NULL || cmd->atcmd == NULL )
		return APP_GPRS_ERROR_ARGUMENT;

	if (cmd->expect_head)
		expect_head_len = strlen(cmd->expect_head);
	if (cmd->expect_tail)
		expect_tail_len = strlen(cmd->expect_tail);
	expect_len = expect_head_len + expect_tail_len;

	int cmdlen = cmd->cmdlen;
	if (cmdlen > CMD_MAX_LEN)
		return APP_GPRS_ERROR_ARGUMENT;

	//prepare
	gx_uart_receive(buf, RESULT_MAX_LEN+1); //clear fifo
	gx_uart_receive(buf, RESULT_MAX_LEN+1); //clear fifo
	force_exit = false;
	bzero(ret, sizeof(GxExecAtReturn_t));
    	bzero(buf, RESULT_MAX_LEN+1);
	ret->buff_ptr = buf;

	gx_uart_send(cmd->atcmd, cmdlen);

	for (i = 0; i < (cmd->timeout_ms+(cmd->delay_ms-1))/cmd->delay_ms; i++)
	{
        		GxCore_ThreadDelay(cmd->delay_ms);

		if (force_exit == true)
			return APP_GPRS_FORCE_EXIT;

		int32_t curlen = gx_uart_receive(buf+len, RESULT_MAX_LEN-len);
		gprs_printf("\r\n[%s %d] AT receive: %s\r\n",__FUNCTION__, __LINE__,buf);
		sprintf(ret_buf,"%s:%s",cmd->atcmd, buf);
		gprs_display(ret_buf);

		if (curlen > 0) {
		len += curlen;
		}
		else
			continue;

		// error exit
		STRCASESTR_ERROR(buf, len);

		if (cmd->expect_head == NULL &&  cmd->expect_tail == NULL)
			continue;

        		if (len < expect_len)
			continue;

		if (cmd->expect_head) {
			ret->head_ptr = strstr(buf, cmd->expect_head);
			if (NULL == ret->head_ptr)
				continue;
		}

		if (cmd->expect_tail) {
			if (cmd->expect_head)
				ret->tail_ptr = strstr(ret->head_ptr+expect_head_len, cmd->expect_tail);
			else
				ret->tail_ptr = strstr(buf, cmd->expect_tail);
			if (NULL == ret->tail_ptr)
				continue;
		}

	   	 break;
	}

	if (len == 0)
	{
		gprs_printf("\r\n[%s %d] NO RSP\r\n",__FUNCTION__, __LINE__);
		gprs_display(" NO RSP");
		return APP_GPRS_ERROR_NOT_RSP;
	}

    	if (cmd->expect_head)
	{
         	if (NULL == ret->head_ptr)
            		 return APP_GPRS_ERROR_NO_HEAD;
    	}
    	if (cmd->expect_tail)
	{
         	if (NULL == ret->tail_ptr)
             		return APP_GPRS_ERROR_NO_TAIL;
    	}

	// get body
	if (cmd->expect_head)
		ret->body_ptr = ret->head_ptr + expect_head_len;
	else
		ret->body_ptr = buf;
	while(*(ret->body_ptr) == ' ')
		ret->body_ptr++;

	//split
	int32_t n = strlen(buf);
	for(i = 0; i < n; i++) {
		if(':' == buf[i] || '\r' == buf[i] || '\n' == buf[i])
			buf[i] = '\0';
	}

	return APP_GPRS_OK;
}

static gx_gprs_ret gx_gprs_init(unsigned int Baudrate)
{
	gx_uart_init(Baudrate);
	if(s_gprs_mutex < 0)
	{
		GxCore_MutexCreate(&s_gprs_mutex);
	}
	if(s_gprs_printf_mutex < 0)
	{
		GxCore_MutexCreate(&s_gprs_printf_mutex);
	}
	return APP_GPRS_OK;
}

static gx_gprs_ret gx_gprs_force_exit(void)
{
	force_exit = true;
	gprs_printf("gx_gprs_force_exit\n");
	gx_uart_close();
	return APP_GPRS_OK;
}

//指令格式: AT+CFUN=1,1<CR>
static gx_gprs_ret gx_gprs_cmd_reset(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_reset\n");

	INIT_CMD(cmd, 1000, NULL, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT+CFUN=1,1\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: AT<CR>
static gx_gprs_ret gx_gprs_cmd_at(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_at\n");

	INIT_CMD(cmd, 5000, NULL, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: AT+CPIN?<CR>
static gx_gprs_ret gx_gprs_cmd_cpin(void)
{
	return APP_GPRS_OK;
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_cpin\n");

	INIT_CMD(cmd, 10000, GPRS_RETURN_HEAD_CPIN, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT+CPIN?\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	if(strstr(result.body_ptr,"READY"))
		ret = APP_GPRS_OK;
	else
		ret = APP_GPRS_RETURN_ERROR;
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: AT+CREG?<CR>
static gx_gprs_ret gx_gprs_cmd_creg(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_creg\n");

	INIT_CMD(cmd, 1000, GPRS_RETURN_HEAD_CREG, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT+CREG?\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	if(strstr(result.body_ptr,"0,1") || strstr(result.body_ptr,"0,5"))
		ret = APP_GPRS_OK;
	else
		ret = APP_GPRS_RETURN_ERROR;
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: AT+CSQ<CR>
static gx_gprs_ret gx_gprs_cmd_csq(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret= APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_csq\n");

	INIT_CMD(cmd, 1000, GPRS_RETURN_HEAD_CSQ, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT+CSQ\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	//CSQ: 26,0 信号强度 解析。。。

	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: AT+CGDCONT=1,\"IP\",\"CMNET\"<CR>
static gx_gprs_ret gx_gprs_cmd_cgdcont(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_cgdcont\n");

	INIT_CMD(cmd, 3000, NULL, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "AT+CGDCONT=1,\"IP\",\"UNINET\"\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: ATD*99#<CR>
static gx_gprs_ret gx_gprs_cmd_atd(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_atd\n");

	INIT_CMD(cmd, 10000, NULL, "CONNECT");

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "ATD*99#\r");
	cmd.cmdlen = strlen(cmd.atcmd);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	g_AppGprs.mode = 1;
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

//指令格式: +++
static gx_gprs_ret gx_gprs_cmd_switchtocmd(void)
{
	GxExecAtCmd_t cmd;
	GxExecAtReturn_t result;
	gx_gprs_ret ret = APP_GPRS_RETURN_ERROR;
	gprs_printf("gx_gprs_cmd_switchtocmd\n");

	INIT_CMD(cmd, 2000, NULL, GPRS_RETURN_TAIL_OK);

	GxCore_MutexLock(s_gprs_mutex);
	bzero(cmd_buf, sizeof(cmd_buf));
	sprintf(cmd_buf, "+++");
	cmd.cmdlen = strlen(cmd.atcmd);
	GxCore_ThreadDelay(500);
	ret = gx_gprs_cmd_exec(&cmd, &result);
	GPRS_RET(ret);
	g_AppGprs.mode = 0;
	GxCore_MutexUnlock(s_gprs_mutex);
	return ret;
}

AppGprs g_AppGprs= {
	.mode = 0,
	.init = gx_gprs_init,
	.force_exit= gx_gprs_force_exit,
	.reset = gx_gprs_cmd_reset,
	.at = gx_gprs_cmd_at,
	.cpin = gx_gprs_cmd_cpin,
	.creg = gx_gprs_cmd_creg,
	.csq = gx_gprs_cmd_csq,
	.cgdcont = gx_gprs_cmd_cgdcont,
	.atd = gx_gprs_cmd_atd,
	.switchtocmd = gx_gprs_cmd_switchtocmd,
};

static IfState s_gprs_state = IF_STATE_INVALID;
static cyg_ppp_handle_t s_gprs_state_handle = -1;
static int s_gprs_dailing = 0;
char* s_gprs_script[] =
{
	"TIMEOUT",      "30",
	"ABORT",        "BUSY",
	"ABORT",        "NO CARRIER",
	"ABORT",        "ERROR",
	"ABORT",        "NO DIALTONE",
	"ABORT",        "NO ANSWER",
	"",                  "AT\r",
	"OK",                  "AT+CGDCONT=1,\"IP\",\"UNINET\"\r",
	"OK",                  "ATD*99#\r",
	"CONNECT",      "",
	0
};

void gprs_connect(void)
{
	cyg_int32 ret;
	cyg_ppp_options_t options;
	//char apn[128];
	//char dail[64];

	if (s_gprs_dailing || (s_gprs_state == IF_STATE_CONNECTING)||(s_gprs_state == IF_STATE_CONNECTED))
	{
		return;
	}
	gprs_printf("gprs_connect\n");
	s_gprs_dailing = 1;
	cyg_ppp_options_init(&options);
	s_gprs_state = IF_STATE_CONNECTING;

	/* never shut down */
	options.idle_time_limit = 0;
	options.debug = 0;
	options.kdebugflag      = 0;
	options.user[0] = 0;
	options.passwd[0] = 0;

	//sprintf(apn, "\rAT+CGDCONT=1,\"IP\",\"%s\"", "UNINET");
	//sprintf(dail, "ATD%s", "*99#");

	options.baud = CYGNUM_SERIAL_BAUD_115200;
	options.modem = 1;
	options.neg_accm = 1;
	options.script = s_gprs_script;

	//if (apn[0])  options.script[15] = apn;
	//if (dail[0]) options.script[17] = dail;

	/* if pppd running, cyg_ppp_up won't start one new dailing */
	s_gprs_state_handle = cyg_ppp_up(GPRS_DEVICE_PATH, &options);

	gprs_printf("cyg_ppp_up,s_gprs_state_handle=0x%x\n", s_gprs_state_handle);
	/* if cyg_ppp_wait_up not called, some scripts won't be executed */
	gprs_printf("\r\n[%s %d]  Waiting for PPP to come up ...\r\n",__FUNCTION__, __LINE__);
	ret = cyg_ppp_wait_up(s_gprs_state_handle);
	GxCore_ThreadDelay(2000);

	s_gprs_dailing = 0;
	if (ret)
	{
		gprs_printf("\r\n[%s %d]  DAILING Fail,ret=%d\r\n",__FUNCTION__, __LINE__, ret);
		s_gprs_state = IF_STATE_IDLE;//IF_STATE_REJ;//
		return;
	}

	gprs_printf("\r\n[%s %d]  DAILING Success\r\n",__FUNCTION__, __LINE__);
	s_gprs_state = IF_STATE_CONNECTED;
}

void gprs_disconnect(void)
{
	if((s_gprs_state==IF_STATE_IDLE)||(s_gprs_state==IF_STATE_INVALID))
	{
		return;
	}
	gprs_printf("gprs_disconnect\n");

	/* if dailing, don't disconnect, or future dails would fail */
	if (s_gprs_dailing)  {
		gprs_printf("\r\n[%s %d]  DAILING ,disconnect wait\r\n",__FUNCTION__, __LINE__);
		return;
	}

	if (s_gprs_state_handle)
	{
		cyg_ppp_down(s_gprs_state_handle,DEVICE_GONE);
		cyg_ppp_wait_down(s_gprs_state_handle);
	}
	GxCore_ThreadDelay(2000);
	s_gprs_state = IF_STATE_IDLE;
}

IfState gprs_getstate(void)
{
	gprs_printf("\r\n[%s %d]  state =%d\r\n",__FUNCTION__, __LINE__,s_gprs_state);
	return s_gprs_state;
}

#endif
