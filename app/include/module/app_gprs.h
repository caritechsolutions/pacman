#ifndef __APP_GPRS_H__
#define __APP_GPRS_H__

#if NETWORK_SUPPORT && GPRS_SUPPORT

#define GPRS_DEVICE_PATH "/dev/ser0"

typedef struct {
	char*    atcmd;
	uint32_t cmdlen;
	uint32_t timeout_ms;
	uint32_t delay_ms;
	char*    expect_head;
	char*    expect_tail;
} GxExecAtCmd_t;

typedef struct {
	char* head_ptr;
	char* body_ptr;
	char* tail_ptr;
	char* buff_ptr;
} GxExecAtReturn_t;

typedef  enum{
	APP_GPRS_OK = 0,
	APP_GPRS_RETURN_OK,
	APP_GPRS_RETURN_ERROR,
	APP_GPRS_FORCE_EXIT,
	APP_GPRS_ERROR_NOT_RSP,
	APP_GPRS_ERROR_ARGUMENT,
	APP_GPRS_ERROR_NO_TAIL,
	APP_GPRS_ERROR_NO_HEAD,
}gx_gprs_ret;

extern  gx_gprs_ret gx_gprs_display(char *info);

#define INIT_CMD(cmd, timeout, head, tail) \
do {                                   \
	cmd.atcmd       = cmd_buf;         \
	cmd.timeout_ms  = timeout;         \
	cmd.delay_ms    = 500;             \
	cmd.expect_head = head;            \
	cmd.expect_tail = tail;            \
} while(0)

#define GPRS_RET( ret)                               \
    do{                                               \
        if(ret){                                        \
            GxCore_MutexUnlock(s_gprs_mutex);     \
            return ret;                               \
        }                                             \
    } while(0)

#define STRCASESTR_OK(buff, len)           \
do {						   \
	if (len >= 2) { 		   \
		if (strstr(buff, "OK"))\
			return APP_GPRS_RETURN_OK;\
		if (strstr(buff, "ok"))\
			return APP_GPRS_RETURN_OK;\
	}						   \
} while(0)

#define STRCASESTR_ERROR(buff, len)        \
do {								   \
	if (len >= 5) { 				   \
		if (strstr(buff, "ERROR") && !strstr(buff, "APPERROR")) {	\
			gprs_printf("\r\n[%s %d] ERROR :%s \r\n",__FUNCTION__, __LINE__,buff);\
			return APP_GPRS_RETURN_ERROR;}\
		if (strstr(buff, "error") && !strstr(buff, "apperror")) {	\
			gprs_printf("\r\n[%s %d] ERROR :%s\r\n",__FUNCTION__, __LINE__,buff);\
			return APP_GPRS_RETURN_ERROR;}\
	}								   \
} while(0)

typedef gx_gprs_ret (*gprs_fun)(void);

typedef struct {
	int mode;//0 cmd 1 data
	gx_gprs_ret (*init)(unsigned int Baudrate);
	gx_gprs_ret (*force_exit)(void);
	gx_gprs_ret (*reset)(void);
	gx_gprs_ret (*at)(void);
	gx_gprs_ret (*cpin)(void);
	gx_gprs_ret (*creg)(void);
	gx_gprs_ret (*csq)(void);
	gx_gprs_ret (*cgdcont)(void);
	gx_gprs_ret (*atd)(void);
	gx_gprs_ret (*switchtocmd)(void);
} AppGprs;

extern AppGprs g_AppGprs;
void gprs_printf(const char *fmt, ...);

extern void gprs_connect(void);
extern void gprs_disconnect(void);
extern IfState gprs_getstate(void);

#endif

#endif /* __APP_GPRS_H__ */

